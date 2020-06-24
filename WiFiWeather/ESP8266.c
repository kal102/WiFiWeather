/*
 * ESP8266.c
 *
 * Created: 15.02.2018 19:15:25
 *  Author: Lukasz
 */ 

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "UART.h"
#include "ESP8266.h"

void ESP8266_Reset()
{
	UART_sendString(PSTR("AT+RST\r\n"));
	_delay_ms(WAIT_FOR_RESPONSE_DELAY);
	UART_sendString(PSTR("ATE0\r\n"));
	_delay_ms(WAIT_FOR_RESPONSE_DELAY);
	UART_clearReceiver();
}

void ESP8266_ConfigureServer(uint16_t portNumber)
{
	char serverString[30];
	char ipString[30];
	
	UART_sendString(PSTR("AT+CIPMUX=1\r\n"));
	_delay_ms(WAIT_FOR_RESPONSE_DELAY);
	sprintf(serverString, "AT+CIPSERVER=1,%d\r\n", portNumber);
	UART_sendString(serverString);
	_delay_ms(WAIT_FOR_RESPONSE_DELAY);
	sprintf(ipString, "AT+CIPSTA=%s\r\n", IP_ADDRESS);
	UART_sendString(ipString);
	_delay_ms(WAIT_FOR_RESPONSE_DELAY);	
	UART_clearReceiver();
}

CMD ESP8266_ReadCommand()
{
	CMD command = {.cmd = NONE, .connectionNr = -1, .cmdParams = {NA}, .cmdError = NO_ERROR};
	MESSAGE message;
	
	while(msgCounter)
	{
		message = UART_scanCmd();
		switch(message.commandIndex)
		{
			case 0:
				command.cmd = NAME;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 1:
				command.cmd = TEMPERATURE;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 2:
				command.cmd = HUMIDITY;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 3:
				command.cmd = RAIN;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 4:
				command.cmd = OVERLAP;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 5:
				command.cmd = WEATHER;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 6:
				command.cmd = WRITE_CALIBRATION_DATA;
				command.connectionNr = message.connectionNr;
				for (uint8_t i = 0; i < MAX_PARAM_NR; i++)
				{
					command.cmdParams[i] = message.commandParameters[i];
				}
				command.cmdError = message.commandError;
				UART_clearReceiver();
				return command;
			case 7:
				command.cmd = READ_CALIBRATION_DATA;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 8:
				command.cmd = HEATER_OFF;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 9:
				command.cmd = HEATER_ON;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 10:
				command.cmd = HEATER_AUTO;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 11:
				command.cmd = HEATER_STATE;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 12:
				command.cmd = WRITE_HEATER_THRESHOLD;
				command.connectionNr = message.connectionNr;
				for (uint8_t i = 0; i < MAX_PARAM_NR; i++)
				{
					command.cmdParams[i] = message.commandParameters[i];
				}
				command.cmdError = message.commandError;
				UART_clearReceiver();
				return command;
			case 13:
				command.cmd = READ_HEATER_THRESHOLD;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			case 14:
				command.cmd = PRESSURE;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;	
			case 15:
				command.cmd = STATUS;
				command.connectionNr = message.connectionNr;
				UART_clearReceiver();
				return command;
			default: break;
		}
	}
	UART_clearReceiver();
	return command;
}

void ESP8266_SendString(const char __memx *stringToSend, bool stringInPgmspace, int16_t connectionNr)
{
	char sendCmd[25];
	
	if (stringInPgmspace)
		sprintf_P(sendCmd, PSTR("AT+CIPSEND=%d,%d\r\n"), connectionNr, strlen_P(stringToSend));
	else
		sprintf_P(sendCmd, PSTR("AT+CIPSEND=%d,%d\r\n"), connectionNr, strlen(stringToSend));
	
	UART_sendString(sendCmd);
	_delay_ms(WAIT_FOR_RESPONSE_DELAY);
	UART_sendString(stringToSend);
	_delay_ms(WAIT_FOR_RESPONSE_DELAY);
	UART_clearReceiver();
}

