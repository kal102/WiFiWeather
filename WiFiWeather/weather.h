/*
 * weather.h
 *
 * Created: 23.02.2018 11:14:55
 *  Author: Lukasz
 */ 


#ifndef WEATHER_H_
#define WEATHER_H_

#include <stdint.h>
#include <stdbool.h>

#define MEASUREMENT_PERIOD 10

typedef struct
{
	uint16_t humidity;
	int16_t temperature;
	uint16_t pressure;
	bool isRaining;
	uint16_t rain;
	uint16_t overlap;
} Weather;

typedef struct
{
	/* Mniej wiêcej 20 dla pomiaru w cieniu. Dotyczy czujnika o czystym polu widzenia. */
	uint8_t overlapScale;
	int8_t overlapOffset;
} CalibrationSet;

extern CalibrationSet calibrationSet;

#endif /* WEATHER_H_ */