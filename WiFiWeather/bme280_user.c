/*
 * bme280_user.c
 *
 * Created: 23.02.2020 12:38:39
 *  Author: Lukasz
 */ 

#include <stdint.h>
#include <util/delay.h>
#include <math.h>
#include "I2C.h"
#include "bme280_user.h"

struct bme280_dev dev;
struct bme280_data comp_data;

void user_delay_ms(uint32_t period)
{
    /*
     * Return control or wait,
     * for a period amount of milliseconds
     */
	
	while(period--)
	{
		_delay_ms(1);
	}
}

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Stop       | -                   |
     * | Start      | -                   |
     * | Read       | (reg_data[0])       |
     * | Read       | (....)              |
     * | Read       | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */
	
	rslt = (int8_t)I2C_Read(dev_id, reg_addr, reg_data, len);
    return rslt;
}

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
    int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

    /*
     * The parameter dev_id can be used as a variable to store the I2C address of the device
     */

    /*
     * Data on the bus should be like
     * |------------+---------------------|
     * | I2C action | Data                |
     * |------------+---------------------|
     * | Start      | -                   |
     * | Write      | (reg_addr)          |
     * | Write      | (reg_data[0])       |
     * | Write      | (....)              |
     * | Write      | (reg_data[len - 1]) |
     * | Stop       | -                   |
     * |------------+---------------------|
     */

	rslt = (int8_t)I2C_Write(dev_id, reg_addr, reg_data, len);
    return rslt;
}

int8_t user_bme280_init(void)
{
	int8_t rslt = BME280_OK;

	dev.dev_id = BME280_I2C_ADDR_PRIM;
	dev.intf = BME280_I2C_INTF;
	dev.read = user_i2c_read;
	dev.write = user_i2c_write;
	dev.delay_ms = user_delay_ms;

	rslt = bme280_init(&dev);
	
	return rslt;
}

int8_t user_bme280_configure(void)
{
	int8_t rslt = BME280_OK;
	uint8_t settings_sel;

	rslt = bme280_get_sensor_settings(&dev);

	dev.settings.osr_h = BME280_OVERSAMPLING_1X;
	dev.settings.osr_p = BME280_OVERSAMPLING_1X;
	dev.settings.osr_t = BME280_OVERSAMPLING_1X;
	dev.settings.filter = BME280_FILTER_COEFF_4;
	dev.settings.standby_time = BME280_STANDBY_TIME_250_MS;

	settings_sel = BME280_OSR_PRESS_SEL;
	settings_sel |= BME280_OSR_TEMP_SEL;
	settings_sel |= BME280_OSR_HUM_SEL;
	settings_sel |= BME280_STANDBY_SEL;
	settings_sel |= BME280_FILTER_SEL;
	rslt = bme280_set_sensor_settings(settings_sel, &dev);
	rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, &dev);

	return rslt;
}

int8_t user_bme280_read(struct bme280_data *comp_data)
{
	int8_t rslt = BME280_OK;
	
	rslt = bme280_get_sensor_settings(&dev);
	rslt = bme280_get_sensor_data(BME280_ALL, comp_data, &dev);
	return rslt;
}

uint8_t user_bme280_get_data(volatile Weather *weather)
{
	int8_t rslt = BME280_OK;
	
	rslt = user_bme280_read(&comp_data);
	
	if (!I2C_Error)
	{
		weather->temperature = (int16_t)(comp_data.temperature / 10);
		weather->humidity = (uint16_t)((comp_data.humidity >> 10) * 10 + (comp_data.humidity % 1024) / 100) ;
		weather->pressure = (uint16_t)(((float)(comp_data.pressure) / 100) * powf(1.0 - (0.0065 * ALTITUDE)/(((float)(weather->temperature) / 10.0) + 273.15), -5.257));
		return 0;	
	}
	return 1;
}
