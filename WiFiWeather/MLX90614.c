/*
 * MLX90614.c
 *
 * Created: 13.07.2018 18:50:08
 *  Author: Lukasz
 */ 

#include <avr/eeprom.h>
#include <util/delay.h>
#include "MLX90614.h"

uint32_t MLX90614_ReadTemperature(enum IRSensor sensor)
{
	uint32_t temperature;
	uint8_t ramAddress = (uint8_t)sensor + MLX90614_AMBIENT_TEMP_ADDR;
	uint8_t tempH, tempL, pec;
	
	I2C_SendStartAndSelect((MLX90614_ADDRESS << 1) | WRITE);
	I2C_SendByte(ramAddress);
	I2C_SendStartAndSelect((MLX90614_ADDRESS << 1) | READ);
	tempL = I2C_ReceiveByte_ACK();
	tempH = I2C_ReceiveByte_ACK();
	pec = I2C_ReceiveByte_NACK();
	I2C_Stop();
	I2C_WaitTillStopWasSent();
	
	temperature = (tempH << 8) | tempL;
	temperature = temperature * 2;
	
	if(I2C_Error)
		return 0xFFFFFFFF;
	else
		return temperature;
}

uint8_t MLX90614_CalculateSkyOverlap(uint32_t ambientTemp, uint32_t skyTemp)
{
	uint8_t skyOverlap;
	int16_t skyClearness;
	CalibrationSet calibrationFactors;
	
	if (ambientTemp < skyTemp)
	{
		skyOverlap = 100;
		return skyOverlap;
	}
	else
	{
		eeprom_read_block(&calibrationFactors, &calibrationSet, sizeof(calibrationSet));
		skyClearness = (int16_t)((ambientTemp - skyTemp)/(calibrationFactors.overlapScale)) + calibrationFactors.overlapOffset;
	}

	if (skyClearness > 100)
		skyOverlap = 0;
	else if (skyClearness < 0)
		skyOverlap = 100;
	else
		skyOverlap = 100 - skyClearness;
	
	return skyOverlap;
}

uint8_t MLX90614_GetData(volatile Weather *weather)
{
	uint32_t ambientTemp, skyTemp;
	
	ambientTemp = MLX90614_ReadTemperature(AMBIENT);
	skyTemp = MLX90614_ReadTemperature(OBJECT1);
	if (ambientTemp != 0xFFFFFFFF && skyTemp != 0xFFFFFFFF)
	{
		weather->overlap = MLX90614_CalculateSkyOverlap(ambientTemp, skyTemp);
		return 0;
	}
	return 1;
}
