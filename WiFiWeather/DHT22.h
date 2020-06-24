/*
 * DHT22_basic.h
 *
 * Created: 19.02.2018 17:47:26
 *  Author: Lukasz
 */ 


#ifndef DHT22_H_
#define DHT22_H_

#include <stdint.h>
#include <stdbool.h>
#include "weather.h"

enum Error {OK, NO_PRESENCE, BUS_SHORTED};
typedef struct
{
	uint8_t integral;
	uint8_t decimal;
} FXP_DATA;

extern enum Error TransmissionError;

void DHT22_Init(void);
uint8_t DHT22_GetData(volatile Weather*);

#endif /* DHT22_H_ */