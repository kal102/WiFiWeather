/*
 * I2C.h
 *
 * Created: 13.07.2018 17:20:22
 *  Author: Lukasz
 */ 


#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include <avr/io.h>
#include <util/twi.h>

/* Sta³a reguluj¹ca timeout magistrali */
#define I2C_TIMEOUT 0xFFFF

/* Czêstotliwoœc magistrali w kHz */
#define I2C_BUS_CLOCK 24

/* Kody b³êdów magistrali I2C */
#define I2C_OK 0
#define I2C_START_ERROR 1
#define I2C_NO_RESPONSE 2
#define I2C_NO_NACK 3
#define I2C_NO_ACK 4

extern uint8_t I2C_Error;

void I2C_Init(void);
void I2C_Start(void);
void I2C_SendAddr(uint8_t address);
void I2C_SendStartAndSelect(uint8_t address);
void I2C_StartSelectWait(uint8_t address);
void I2C_SendByte(uint8_t byte);
uint8_t I2C_ReceiveByte_NACK(void);
uint8_t I2C_ReceiveByte_ACK(void);
static inline void I2C_Stop() { TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO); };
static inline void I2C_WaitTillStopWasSent() { while (TWCR & (1 << TWSTO)); };

uint8_t I2C_SetRWAddress(uint8_t id, uint8_t address);
uint8_t I2C_Read(uint8_t id, uint8_t address, uint8_t *data, uint16_t len);
uint8_t I2C_Write(uint8_t id, uint8_t address, uint8_t *data, uint16_t len);

#endif /* I2C_H_ */