/*
 * I2C.c
 *
 * Created: 13.07.2018 17:18:43
 *  Author: Lukasz
 */ 

#include "I2C.h"

uint8_t I2C_Error;
uint16_t I2C_TimeoutCounter;

static inline void I2C_SetError(uint8_t error) { I2C_Error = error; };

void I2C_SetBusSpeed(uint16_t speed)
{
	speed = ((F_CPU/speed)/1000-16)/2;
	
	/* Jeœli wartoœæ speed jest wiêksza ni¿ 8 bitów, ustaw odpowiednio preskaler */
	uint8_t prescaler = 0;
	while (speed > 255)
	{
		prescaler++;
		speed = speed/4;
	}
	
	TWSR = (TWSR & (_BV(TWPS1) | _BV(TWPS0))) | prescaler;
	TWBR = speed;
}

void I2C_Init()
{
	//PORTC |= (1 << PC4) | (1 << PC5);
	TWCR = (1 << TWEA) | (1 << TWEN);
	I2C_SetBusSpeed(I2C_BUS_CLOCK);
}

static inline void I2C_WaitForComplete()
{
	I2C_TimeoutCounter = I2C_TIMEOUT;
	while (!(TWCR & (1 << TWINT)))
	{
		if(!I2C_TimeoutCounter)
		{
			I2C_SetError(I2C_NO_RESPONSE);
			return;
		}
		else
			I2C_TimeoutCounter--;
	}
	I2C_SetError(I2C_OK);
}

void I2C_Start()
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	I2C_WaitForComplete();
	if ((TW_STATUS != TW_START) && (TW_STATUS != TW_REP_START)) I2C_SetError(I2C_START_ERROR);
}

void I2C_SendAddr(uint8_t address)
{
	uint8_t status;
	if ((address & 0x01) == 0)
		status = TW_MT_SLA_ACK;
	else
		status = TW_MR_SLA_ACK;
	TWDR = address;
	TWCR = (1 << TWINT) | (1 << TWEN);
	I2C_WaitForComplete();
	if (TW_STATUS != status)
		I2C_SetError(I2C_NO_ACK);
	else
		I2C_SetError(I2C_OK);
}

void I2C_SendStartAndSelect(uint8_t address)
{
	I2C_Start();
	if (I2C_Error) return;
	I2C_SendAddr(address);
}

void I2C_SendByte(uint8_t byte)
{
	TWDR = byte;
	TWCR = (1 << TWINT) | (1 << TWEN);
	I2C_WaitForComplete();
	if (TW_STATUS != TW_MT_DATA_ACK) I2C_SetError(I2C_NO_ACK);	
}

uint8_t I2C_ReceiveByte_NACK()
{
	TWCR = (1 << TWINT) | (1 << TWEN);
	I2C_WaitForComplete();
	if (TW_STATUS != TW_MR_DATA_NACK) I2C_SetError(I2C_NO_NACK);
	return TWDR;
}

uint8_t I2C_ReceiveByte_ACK()
{
	TWCR = (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
	I2C_WaitForComplete();
	if (TW_STATUS != TW_MR_DATA_ACK) I2C_SetError(I2C_NO_ACK);
	return TWDR;
}

void I2C_StartSelectWait(uint8_t address)
{
	do
	{
		I2C_SendStartAndSelect(address);
	}
	while (I2C_Error == I2C_NO_ACK);
}

uint8_t I2C_SetRWAddress(uint8_t id, uint8_t address)
{
	I2C_SendStartAndSelect((id << 1) | TW_WRITE);
	I2C_SendByte(address);
	
	return I2C_Error;
}

uint8_t I2C_Read(uint8_t id, uint8_t address, uint8_t *data, uint16_t len)
{
	I2C_SetRWAddress(id, address);
	I2C_SendStartAndSelect((id << 1) | TW_READ);
	while(--len)
	{
		*data = I2C_ReceiveByte_ACK();
		data = data + 1;
	}
	*data = I2C_ReceiveByte_NACK();
	I2C_Stop();
	
	return I2C_Error;
}

uint8_t I2C_Write(uint8_t id, uint8_t address, uint8_t *data, uint16_t len)
{
	I2C_SetRWAddress(id, address);
	while(len--)
	{
		I2C_SendByte(*data);
		data = data + 1;
	}
	I2C_Stop();
	
	return I2C_Error;
}