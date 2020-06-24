/*
 * Raindrops.c
 *
 * Created: 23.02.2018 16:35:25
 *  Author: Lukasz
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/atomic.h>
#include "Raindrops.h"

#define NUMBER_OF_SAMPLES 128
volatile uint32_t ADCval;

ISR(ADC_vect)
{
	static uint32_t ADCaccum;
	static uint8_t sampleNumber;
	ADCaccum += ADC;
	sampleNumber++;
	if(NUMBER_OF_SAMPLES == sampleNumber)
	{
		ADCval = ADCaccum;
		ADCaccum = 0;
		sampleNumber = 0;
	}
}

void Raindrops_Init()
{
	DDRD &= ~(1 << PD3);
	DDRD |= (1 << PD4);
}

void Raindrops_GetData(volatile Weather *weather)
{
	weather->isRaining = !(PIND & (1 << PD3));
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		/* Napiêcie na wyjœciu analogowym zmienia siê w zakresie 0-5 V, maleje wraz ze wzrostem intensywnoœci opadów */
		weather->rain = 100 - (ADCval*100UL)/(1024UL*NUMBER_OF_SAMPLES);
	}
}

uint8_t Heater_SM(uint8_t heater, volatile Weather *weather)
{
	enum HeaterMode mode;
	int8_t thresholdRaw = eeprom_read_byte(&heaterThreshold);
	int16_t threshold = 10 * (int16_t)thresholdRaw;
	
	mode = eeprom_read_byte(&heaterMode);
	switch(mode)
	{
		case MODE_OFF:
			heater = 0;
			break;
		case MODE_ON:
			heater = 1;
			break;
		case MODE_AUTO:
			if (heater)
			{
				if (weather->temperature < (threshold + (HEATER_HYSTERESIS/2)))
					heater = 1;
				else
					heater = 0;
			}
			else
			{
				if (weather->temperature >= (threshold - (HEATER_HYSTERESIS/2)))
					heater = 0;
				else
					heater = 1;
			}
			break;
		default:
			break;
	}
	return heater;
}

void Heater_SetOutput(uint8_t heater)
{
	if (heater)
		PORTD |= (1 << PD4);
	else
		PORTD &= ~(1 << PD4);
}
