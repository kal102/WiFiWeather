/*
 * ESP8266.h
 *
 * Created: 15.02.2018 19:15:37
 *  Author: Lukasz
 */ 


#ifndef ESP8266_H_
#define ESP8266_H_

#include <stdbool.h>
#include "UART.h"

/* Adres IP modu³u */
#define IP_ADDRESS "192.168.1.30"
/* Czas oczekiwania na odpowiedŸ - minimum 300 ms */
#define WAIT_FOR_RESPONSE_DELAY 400
/* Czas oczekiwania na ustabilizowanie siê po³¹czenia */
#define  WAIT_FOR_CONNECTION_STABLE 1000

enum Command {NONE, NAME, STATUS, TEMPERATURE, HUMIDITY, PRESSURE, RAIN, OVERLAP, WEATHER, WRITE_CALIBRATION_DATA, READ_CALIBRATION_DATA,
	 HEATER_OFF, HEATER_ON, HEATER_AUTO, HEATER_STATE, WRITE_HEATER_THRESHOLD, READ_HEATER_THRESHOLD};

typedef struct
{
	enum Command cmd;
	int16_t connectionNr;
	int16_t cmdParams[MAX_PARAM_NR];
	enum CommandError cmdError;
} CMD;

void ESP8266_Reset(void);
void ESP8266_ConfigureServer(uint16_t portNumber);
CMD ESP8266_ReadCommand(void);
void ESP8266_SendString(const char __memx *stringToSend, bool stringInPgmspace, int16_t connectionNr);

#endif /* ESP8266_H_ */