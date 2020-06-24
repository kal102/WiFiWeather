/*
 * DHT22_basic.c
 *
 * Created: 19.02.2018 17:46:53
 *  Author: Lukasz
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include "DHT22.h"

FXP_DATA hum, temp;
enum Error TransmissionError;

void DHT22_Init()
{
	DDRD &= ~(1 << PD2);
	PORTD &= ~(1 << PD2);
}

bool DHT22_StartTransmission()
{
	TransmissionError = OK;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		/* Wys³anie zera na czas co najmniej 1 ms - sygna³ startowy */
		DDRD |= (1 << PD2);
		_delay_us(1100);
		DDRD &= ~(1 << PD2);
		
		/* Sprawdzenie, czy czujnik odpowiedzia³ */
		uint8_t counter = 0;
		_delay_us(20);
		while ((counter < 0xFF) && (PIND & (1 << PD2)))
		{
			_delay_us(1);
			counter++;
		}
		if (counter == 0xFF)
		{
			TransmissionError = NO_PRESENCE;
			return false;
		}
		
		/* Sprawdzenie, czy czujnik zwolni³ liniê po ustalonym czasie */
		counter = 0;
		while ((counter < 0xFF) && !(PIND & (1 << PD2)))
		{
			_delay_us(1);
			counter++;
		}
		if (counter == 0xFF)
		{
			TransmissionError = BUS_SHORTED;
			return false;
		}
	}
	return true;
}

bool DHT22_ReadBit()
{
	uint8_t counter = 0;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		/* Poczekaj, a¿ czujnik wyœle zero, oznaczaj¹ce nowy bit */
		while((counter < 0xFF) && (PIND & (1 << PD2)))
		{
			_delay_us(1);
			counter++;
		}
		if (counter == 0xFF)
		{
			TransmissionError = NO_PRESENCE;
			return false;
		}
		
		/* SprawdŸ, czy linia nie jest zwarta do masy */
		while((counter < 0xFF) && !(PIND & (1 << PD2)))
		{
			_delay_us(1);
			counter++;
		}
		if (counter == 0xFF)
		{
			TransmissionError = BUS_SHORTED;
			return false;
		}
		
		/* Odczytaj wartoœæ bitu - zero trwa 26-28 us, a jeden 70 us */
		_delay_us(50);
		if (PIND & (1 << PD2))
			return true;
	}
	return false;
}

int8_t DHT22_ReadByte()
{
	int8_t result = 0;
	for (uint8_t i = 0; i < 8; i++)
	{
		result <<= 1;
		if(DHT22_ReadBit())
			result |= 0x01;
	}
	return result;
}

/* Nale¿y zaczekaæ 2 sekundy przed ponownym odczytem! */
bool DHT22_ReadData()
{
	int8_t checksum;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (!DHT22_StartTransmission()) return false;
		hum.integral = DHT22_ReadByte();
		hum.decimal = DHT22_ReadByte();
		temp.integral = DHT22_ReadByte();
		temp.decimal = DHT22_ReadByte();
		checksum = DHT22_ReadByte();
		if (TransmissionError != OK) return false;
	}
	int8_t sum = hum.integral + hum.decimal + temp.integral + temp.decimal;
	if (sum == checksum)
		return true;
	else
		return false;
}

uint8_t DHT22_GetData(volatile Weather *weather)
{
	if(DHT22_ReadData())
	{
		weather->humidity = (hum.integral << 8) | hum.decimal;
		weather->temperature = (temp.integral << 8) | temp.decimal;
		return 0;
	}
	return 1;
}
