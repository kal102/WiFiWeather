/*
 * UART.c
 *
 * Created: 29.11.2017 21:02:55
 *  Author: Lukasz
 */ 

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <string.h>
#include "RingBuffer.h"
#include "UART.h"

CircBuffer recBuff, sendBuff;
volatile uint8_t msgCounter = 0;
volatile _Bool TXFlag;
	
const char __flash message0[] = {":name"};
const char __flash message1[] = {":temp"};
const char __flash message2[] = {":humi"};
const char __flash message3[] = {":rain"};
const char __flash message4[] = {":skyo"};
const char __flash message5[] = {":wdat"};
const char __flash message6[] = {":wcal"};
const char __flash message7[] = {":rcal"};
const char __flash message8[] = {":hoff"};
const char __flash message9[] = {":honn"};
const char __flash message10[] = {":haut"};
const char __flash message11[] = {"hsta"};;
const char __flash message12[] = {":hwth"};
const char __flash message13[] = {":hrth"};
const char __flash message14[] = {":pres"};
const char __flash message15[] = {":stat"};
const char __flash * const __flash messages[] = {message0, message1, message2, message3, message4, message5, message6, message7, message8,
												message9, message10, message11, message12, message13, message14, message15};

ISR(USART_RX_vect)
{
	uint8_t byte;
	byte = UDR0;
	if (byte == '\r')
	{
		byte = 0;
		msgCounter++;
	}
	cbAdd(&recBuff, byte);
}

ISR(USART_TX_vect)
{
	if (!cbIsEmpty(&sendBuff))
	{
		TXFlag = true;
		UDR0 = cbRead(&sendBuff);
	}
	else
	{
		TXFlag = false;
	}
}

void UART_sendByte(uint8_t byte)
{
	enum bufferResult result = cbAdd(&sendBuff, byte);
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		if (!TXFlag)
		{
			USART_TX_vect();
		}
	}
	if (BUFFER_FULL == result)
	{
		while (cbIsFull(&sendBuff));
		cbAdd(&sendBuff, byte);
	}
}

void UART_sendWord(uint16_t word)
{
	uint8_t byteH = (word >> 8);
	uint8_t byteL = word;
	UART_sendByte(byteH);
	UART_sendByte(byteL);
}

void UART_clearReceiver()
{
	cbClear(&recBuff);
	msgCounter = 0;
}

enum bufferResult UART_sendString(const char __memx *txt)
{
	enum bufferResult result = BUFFER_OK;
	while(*txt != 0)
	{
		if(BUFFER_FULL == cbAdd(&sendBuff, *txt))
		{
			result = BUFFER_FULL;
		}
		txt++;
	}
	ATOMIC_BLOCK(ATOMIC_FORCEON)
	{
		if (!TXFlag)
		{
			USART_TX_vect();
		}
	}
	return result;
}

_Bool GetToken(char msg[MAX_MSG_LEN])
{
	char character;
	for (uint8_t i = 0; i < MAX_MSG_LEN; i++)
	{
		if (cbIsEmpty(&recBuff))
		{
			return false;
		}
		character = cbRead(&recBuff);
		msg[i] = character;
		if (character == 0)
		{
			break;
		}
	}
	return true;
}

MESSAGE UART_scanCmd()
{
	MESSAGE result = {.commandIndex = -1, .connectionNr = -1, .commandParameters = {NA, NA}, .commandError = NO_ERROR};
	uint8_t index, paramIndex = 0;
	char msg[MAX_MSG_LEN] = {0};
	char *p;
		
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		msgCounter--;
		if (!GetToken(msg))
		{
			return result;
		}
	}
	if (strcmp_P(msg, PSTR("WIFI GOT IP")) == 0)
	{
		connectionFlag = 1;
	}
	for (index = 0; index < (sizeof(messages)/sizeof(messages[0])); index++)
	{
		if (strstr_P(msg, messages[index]) != NULL)
		{
			result.commandIndex = index;
			sscanf_P(msg, PSTR("\n+IPD,%d,%*d:%*s"), &(result.connectionNr));
			for(p = strchr(msg, ' '); p != NULL; p = strchr(p+1, ' '))
			{
				if (paramIndex < MAX_PARAM_NR)
				{
					result.commandParameters[paramIndex] = atoi(p+1);
					paramIndex++;
				}
				else
				{
					result.commandError = TOO_MANY_PARAMS;
					break;
				}
			}
			return result;
		}
	}
	return result;
}

