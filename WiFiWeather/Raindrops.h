/*
 * Raindrops.h
 *
 * Created: 23.02.2018 16:41:27
 *  Author: Lukasz
 */ 


#ifndef RAINDROPS_H_
#define RAINDROPS_H_

#include <stdint.h>
#include "weather.h"

#define HEATER_HYSTERESIS 10
#define HEATER_THRESHOLD_MIN -100
#define HEATER_THRESHOLD_MAX 100

enum HeaterMode {MODE_OFF, MODE_ON, MODE_AUTO};

void Raindrops_Init(void);
void Raindrops_GetData(volatile Weather*);
uint8_t Heater_SM(uint8_t, volatile Weather*);
void Heater_SetOutput(uint8_t);

extern enum HeaterMode heaterMode;
extern int8_t heaterThreshold;

#endif /* RAINDROPS_H_ */