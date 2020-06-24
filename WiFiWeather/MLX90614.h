/*
 * MLX90614.h
 *
 * Created: 13.07.2018 18:50:26
 *  Author: Lukasz
 */ 


#ifndef MLX90614_H_
#define MLX90614_H_

#include <stdint.h>
#include "I2C.h"
#include "weather.h"

#define READ 0x01
#define WRITE 0x00

#define MLX90614_ADDRESS 0x5A //0x00
#define MLX90614_AMBIENT_TEMP_ADDR 0x06

enum IRSensor {AMBIENT, OBJECT1, OBJECT2};

uint32_t MLX90614_ReadTemperature(enum IRSensor sensor);
uint8_t MLX90614_GetData(volatile Weather*);

extern CalibrationSet callibrationSet;

#endif /* MLX90614_H_ */