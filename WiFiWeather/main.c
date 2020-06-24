/*
 * WiFiWeather.c
 *
 * Created: 12.02.2018 16:06:18
 * Author : Lukasz
 */ 

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/pgmspace.h>
#include "weather.h"
#include "DHT22.h"
#include "Raindrops.h"
#include "UART.h"
#include "I2C.h"
#include "MLX90614.h"
#include "bme280_user.h"
#include "ESP8266.h"

/* Zmienne w pamiêci SRAM */
volatile Weather weather;
volatile bool updateWeatherFlag = 0;
bool connectionFlag = 0;

/* Zmienne w pamiêci EEPROM */
CalibrationSet EEMEM calibrationSet = {.overlapScale = 3, .overlapOffset = 0};
enum HeaterMode EEMEM heaterMode = MODE_AUTO;
int8_t EEMEM heaterThreshold = 15;

ISR(TIMER1_COMPA_vect)
{
	static uint8_t timer;
	if (timer == MEASUREMENT_PERIOD)
	{
		updateWeatherFlag = 1;
		timer = 0;
	}
	else
		timer++;
}

void Timer1_Init()
{
	TIMSK1 = (1 << OCIE1A);
	/* Przerwanie raz na sekundê */
	OCR1A = F_CPU/1024;
	/* Tryb prosty, preskaler 1024 */
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);
}

void ADC_Init()
{
	/* Wy³¹czenie funkcji cyfrowej pinu */
	DIDR0 = (1 << ADC0D);
	/* AVcc jako Ÿród³o odniesienia, pin PC0 */
	ADMUX = (1 << REFS0);
	/* Tryb Free Running, przerwania, preskaler 128 */
	ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);
	ADCSRA |= (1 << ADATE) | (1 << ADSC);
}

void uart_57600()
{
	#define BAUD 57600
	#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	#if USE_2X
	UCSR0A |= (1 << U2X0);
	#else
	UCSR0A &= ~(1 << U2X0);
	#endif
}

void UART_init()
{
	uart_57600();
	UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << TXCIE0);
	/* Format ramki: 8 bitów danych, no parity, 1 bit stopu */
	UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
}

_Bool InRange(int16_t number, int16_t min, int16_t max)
{
	if ((number >= min) && (number <= max))
		return true;
	else
		return false;
}

void Callibrate(CMD *cmd)
{
	CalibrationSet calibrationFactors;
	
	for (uint8_t index = 0; (cmd->cmdParams[index] != NA) && (index < MAX_PARAM_NR); index++)
	{
		switch(index)
		{
			case 0:
				if (InRange(cmd->cmdParams[index], 1, UINT8_MAX))
				{
					calibrationFactors.overlapScale = (uint8_t)(cmd->cmdParams[index]);
					eeprom_update_byte(&(calibrationSet.overlapScale), calibrationFactors.overlapScale);
					break;
				}
				else
				{
					cmd->cmdError = OUT_OF_RANGE;
					return;
				}
			case 1:
				if (InRange(cmd->cmdParams[index], -100, 100))
				{
					calibrationFactors.overlapOffset = cmd->cmdParams[index];
					eeprom_update_byte(&(calibrationSet.overlapOffset), calibrationFactors.overlapOffset);
					break;
				}
				else
				{
					cmd->cmdError = OUT_OF_RANGE;
					return;
				}
			default:
				break;
		}
	}
}

void SetHeaterThreshold(CMD *cmd)
{
	int8_t threshold;
	
	for (uint8_t index = 0; (cmd->cmdParams[index] != NA) && (index < MAX_PARAM_NR); index++)
	{
		switch(index)
		{
			case 0:
			if (InRange(cmd->cmdParams[index], HEATER_THRESHOLD_MIN, HEATER_THRESHOLD_MAX))
			{
				threshold = cmd->cmdParams[index];
				eeprom_update_byte(&heaterThreshold, threshold);
				break;
			}
			else
			{
				cmd->cmdError = OUT_OF_RANGE;
				return;
			}
			default:
			break;
		}
	}
}

void SendName(int16_t connectionNumber)
{
	if (connectionNumber == -1)
		return;
	ESP8266_SendString(PSTR("Name: Dom\r\n"), true, connectionNumber);
}

void SendStatus(int16_t connectionNumber, uint8_t status)
{
	if (connectionNumber == -1)
		return;
	if (status)
		ESP8266_SendString(PSTR("Status: ERROR\r\n"), true, connectionNumber);
	else
		ESP8266_SendString(PSTR("Status: OK\r\n"), true, connectionNumber);		
}

void SendHumidity(int16_t connectionNumber)
{
	char humidityString[20];
	
	if (connectionNumber == -1)
		return;
	sprintf_P(humidityString, PSTR("Humidity: %u.%u%%\r\n"), weather.humidity / 10, weather.humidity % 10);
	ESP8266_SendString(humidityString, false, connectionNumber);
}

void SendTemperature(int16_t connectionNumber)
{
	char temperatureString[20];
	
	if (connectionNumber == -1)
		return;
	if (InRange(weather.temperature, -9, -1))
		sprintf_P(temperatureString, PSTR("Temperature: -%d.%d\r\n"), weather.temperature / 10, abs(weather.temperature % 10));
	else
		sprintf_P(temperatureString, PSTR("Temperature: %d.%d\r\n"), weather.temperature / 10, abs(weather.temperature % 10));
	ESP8266_SendString(temperatureString, false, connectionNumber);
}

void SendPressure(int16_t connectionNumber)
{
	char pressureString[25];
	
	if (connectionNumber == -1)
		return;
	sprintf_P(pressureString, PSTR("Pressure: %u hPa\r\n"), weather.pressure);
	ESP8266_SendString(pressureString, false, connectionNumber);
}

void SendRainData(int16_t connectionNumber)
{
	char rainString[30];
	
	if (connectionNumber == -1)
		return;
	if(weather.isRaining)
		sprintf_P(rainString, PSTR("Rain: YES, %u%%\r\n"), weather.rain);
	else
		sprintf_P(rainString, PSTR("Rain: NO, %u%%\r\n"), weather.rain);
	ESP8266_SendString(rainString, false, connectionNumber);
}

void SendOverlapData(int16_t connectionNumber)
{
	char overlapString[20];
	
	if (connectionNumber == -1)
		return;
	sprintf_P(overlapString, PSTR("Sky overlap: %d%%\r\n"), weather.overlap);
	ESP8266_SendString(overlapString, false, connectionNumber);
}

void SendWeatherData(int16_t connectionNumber)
{
	char weatherString[100];
	char rainString[5];
	
	if (connectionNumber == -1)
		return;
		
	if (weather.isRaining)
		strcpy(rainString, "YES");
	else
		strcpy(rainString, "NO");
		
	if (InRange(weather.temperature, -9, -1))
	{
		sprintf_P(weatherString, PSTR("Temperature: -%d.%d\r\nHumidity: %u.%u%%\r\nRain: %s, %u%%\r\nSky overlap: %d%%\r\nPressure: %u hPa\r\n"),
		weather.temperature / 10, abs(weather.temperature % 10), weather.humidity / 10, weather.humidity % 10, rainString, weather.rain, weather.overlap, weather.pressure);
	}
	else
	{
		sprintf_P(weatherString, PSTR("Temperature: %d.%d\r\nHumidity: %u.%u%%\r\nRain: %s, %u%%\r\nSky overlap: %d%%\r\nPressure: %u hPa\r\n"),
		weather.temperature / 10, abs(weather.temperature % 10), weather.humidity / 10, weather.humidity % 10, rainString, weather.rain, weather.overlap, weather.pressure);
	}
	
	ESP8266_SendString(weatherString, false, connectionNumber);
}

void SendCallibrationConfirmation(int16_t connectionNumber, enum CommandError commandError)
{
	if (connectionNumber == -1)
		return;
	if (commandError == OUT_OF_RANGE)
		ESP8266_SendString(PSTR("Calibration invalid. One of given parameters was out of range\r\n"), true, connectionNumber);
	else if (commandError == TOO_MANY_PARAMS)
		ESP8266_SendString(PSTR("Device calibrated. Too many parameters given\r\n"), true, connectionNumber);
	else
		ESP8266_SendString(PSTR("Device calibrated\r\n"), true, connectionNumber);
}

void SendCallibrationData(int16_t connectionNumber)
{
	char calibrationString[50];
	CalibrationSet calibrationFactors;
	
	if (connectionNumber == -1)
		return;
	eeprom_read_block(&calibrationFactors, &calibrationSet, sizeof(calibrationSet));
	sprintf_P(calibrationString, PSTR("SKY OVERLAP\r\nScale: %u\r\nOffset: %i\r\n"), calibrationFactors.overlapScale, calibrationFactors.overlapOffset);
	ESP8266_SendString(calibrationString, false, connectionNumber);
}

void SendHeaterModeConfirmation(int16_t connectionNumber)
{
	enum HeaterMode mode;
	
	if (connectionNumber == -1)
		return;
	mode = eeprom_read_byte(&heaterMode);
	if (mode == MODE_OFF)
		ESP8266_SendString(PSTR("Heater set off\r\n"), true, connectionNumber);
	else if (mode == MODE_ON)
		ESP8266_SendString(PSTR("Heater set on\r\n"), true, connectionNumber);
	else if (mode == MODE_AUTO)
		ESP8266_SendString(PSTR("Heater set auto\r\n"), true, connectionNumber);
	else
		ESP8266_SendString(PSTR("Heater error\r\n"), true, connectionNumber);
}

void SendHeaterState(uint8_t heater,int16_t connectionNumber)
{
	if (connectionNumber == -1)
		return;
	if (heater)
		ESP8266_SendString(PSTR("Heater is now on\r\n"), true, connectionNumber);
	else
		ESP8266_SendString(PSTR("Heater is now off\r\n"), true, connectionNumber);
}

void SendHeaterThresholdConfirmation(int16_t connectionNumber, enum CommandError commandError)
{
	if (connectionNumber == -1)
		return;
	if (commandError == OUT_OF_RANGE)
		ESP8266_SendString(PSTR("Heater threshold hasn't been set. Given parameter was out of range\r\n"), true, connectionNumber);
	else if (commandError == TOO_MANY_PARAMS)
		ESP8266_SendString(PSTR("Heater threshold set. Too many parameters given\r\n"), true, connectionNumber);
	else
		ESP8266_SendString(PSTR("Heater threshold set\r\n"), true, connectionNumber);
}

void SendHeaterThresholdValue(int16_t connectionNumber)
{
	char thresholdString[35];
	int8_t threshold;
	
	if (connectionNumber == -1)
		return;
	threshold = eeprom_read_byte(&heaterThreshold);
	sprintf_P(thresholdString, PSTR("Heater threshold value: %d\r\n"), threshold);
	ESP8266_SendString(thresholdString, false, connectionNumber);
}

int main(void)
{
	CMD command;
	uint8_t status = 0;
	uint8_t heater = 0;

	DDRB = 0b00100111;
	Raindrops_Init();
	DHT22_Init();
	Timer1_Init();
	ADC_Init();
	UART_init();
	I2C_Init();
	user_bme280_init();
	user_bme280_configure();
	sei();
	ESP8266_Reset();
    while (1)
    {
		if (connectionFlag)
		{
			_delay_ms(WAIT_FOR_CONNECTION_STABLE);
			ESP8266_ConfigureServer(1234);
			connectionFlag = 0;
		}
		if (updateWeatherFlag)
		{
			status = 0;
			DHT22_GetData(&weather);
			status |= user_bme280_get_data(&weather);
			status |= MLX90614_GetData(&weather);
			Raindrops_GetData(&weather);
			heater = Heater_SM(heater, &weather);
			Heater_SetOutput(heater);
			updateWeatherFlag = 0;
		}
		if (msgCounter)
		{
			_delay_ms(WAIT_FOR_RESPONSE_DELAY);
			command = ESP8266_ReadCommand();
			switch(command.cmd)
			{
				case NAME: SendName(command.connectionNr); break;
				case STATUS: SendStatus(command.connectionNr, status); break;
				case TEMPERATURE: SendTemperature(command.connectionNr); break;
				case HUMIDITY: SendHumidity(command.connectionNr); break;
				case PRESSURE: SendPressure(command.connectionNr); break;
				case RAIN: SendRainData(command.connectionNr); break;
				case OVERLAP: SendOverlapData(command.connectionNr); break;
				case WEATHER: SendWeatherData(command.connectionNr); break;
				case WRITE_CALIBRATION_DATA: Callibrate(&command); SendCallibrationConfirmation(command.connectionNr, command.cmdError); break;
				case READ_CALIBRATION_DATA: SendCallibrationData(command.connectionNr); break;
				case HEATER_OFF: eeprom_write_byte(&heaterMode, MODE_OFF); SendHeaterModeConfirmation(command.connectionNr); break;
				case HEATER_ON: eeprom_write_byte(&heaterMode, MODE_ON); SendHeaterModeConfirmation(command.connectionNr); break;
				case HEATER_AUTO: eeprom_write_byte(&heaterMode, MODE_AUTO); SendHeaterModeConfirmation(command.connectionNr); break;
				case HEATER_STATE: SendHeaterState(heater, command.connectionNr); break;
				case WRITE_HEATER_THRESHOLD: SetHeaterThreshold(&command); SendHeaterThresholdConfirmation(command.connectionNr, command.cmdError); break;
				case READ_HEATER_THRESHOLD: SendHeaterThresholdValue(command.connectionNr); break;
				default: break;
			}
		}
    }
}

