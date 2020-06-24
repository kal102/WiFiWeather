/*
 * bme280_user.h
 *
 * Created: 23.02.2020 12:42:03
 *  Author: Lukasz
 */ 


#ifndef BME280_USER_H_
#define BME280_USER_H_

#include "bme280.h"
#include "bme280_defs.h"
#include "weather.h"

#define BME280_ADDRESS 0x60
#define ALTITUDE 230.0f

int8_t user_bme280_init(void);
int8_t user_bme280_configure(void);
int8_t user_bme280_read(struct bme280_data *comp_data);
uint8_t user_bme280_get_data(volatile Weather *weather);

#endif /* BME280_USER_H_ */