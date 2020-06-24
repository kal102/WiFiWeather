/*
 * UART.h
 *
 * Created: 29.11.2017 21:02:38
 *  Author: Lukasz
 */ 


#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include "RingBuffer.h"

#define NA 0x8000
#define MAX_MSG_LEN 30
#define MAX_PARAM_NR 2

extern CircBuffer recBuff, sendBuff;
extern volatile uint8_t msgCounter;
extern bool connectionFlag;

enum CommandError {NO_ERROR, TOO_MANY_PARAMS, OUT_OF_RANGE};

typedef struct  
{
	int8_t commandIndex;
	int16_t connectionNr;
	int16_t commandParameters[MAX_PARAM_NR];
	enum CommandError commandError;
} MESSAGE;
 
void UART_sendByte(uint8_t byte);
void UART_sendWord(uint16_t word);
void UART_clearReceiver(void);
enum bufferResult UART_sendString(const char __memx *txt);
MESSAGE UART_scanCmd(void);

#endif /* UART_H_ */