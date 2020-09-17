/*
 *  Copyright (c) 2018 Bastian Koppelmann Paderborn Univeristy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "plic/plic_driver.h"
#include "platform.h"
#include "encoding.h"
#include "spi.h"
#include "common.h"
#include "UART_driver.h"
#include "led-matrix.h"
#include "bmi160.h"
#include "watchdog.h"

asm (".global _printf_float");

/* this is global as this address is set up to be faulty */
extern float *sensor_x_ptr;
extern float *matrix_fx_ptr;

void delay_ms(uint32_t period)
{
    uint64_t end = (period >> 1) + get_timer_value();
    while (get_timer_value() < end);
}

/* setup code */
void config_sensors(struct bmi160_dev *sensor)
{
    int8_t rslt = BMI160_OK;

    /* Select the Output data rate, range of accelerometer sensor */
    sensor->accel_cfg.odr = BMI160_ACCEL_ODR_1600HZ;
    sensor->accel_cfg.range = BMI160_ACCEL_RANGE_2G;
    sensor->accel_cfg.bw = BMI160_ACCEL_BW_NORMAL_AVG4;

    /* Sel->ct the power mode of accelerometer sensor */
    sensor->accel_cfg.power = BMI160_ACCEL_NORMAL_MODE;

    /* Sel->ct the Output data rate, range of Gyroscope sensor */
    sensor->gyro_cfg.odr = BMI160_GYRO_ODR_3200HZ;
    sensor->gyro_cfg.range = BMI160_GYRO_RANGE_2000_DPS;
    sensor->gyro_cfg.bw = BMI160_GYRO_BW_NORMAL_MODE;

    /* Select the power mode of Gyroscope sensor */
    sensor->gyro_cfg.power = BMI160_GYRO_NORMAL_MODE;

    /* Set the sensor configuration */
    rslt = bmi160_set_sens_conf(sensor);
}

void get_sensor_data(struct bmi160_dev *sensor, int16_t *x, int16_t *y, int16_t *z)
{
    int8_t rslt = BMI160_OK;
    struct bmi160_sensor_data accel;
    struct bmi160_sensor_data gyro;

    /* To read only Accel data */
    rslt = bmi160_get_sensor_data(BMI160_ACCEL_SEL, &accel, NULL, sensor);

    *x = accel.x;
    *y = accel.y;
    *z = accel.z;
}

/* rendering code for the led-matrix */

typedef struct {
    float delta_x, delta_y;
    float fx, fy;
    uint8_t x, y;
} LEDCoordinates;

void render(LEDCoordinates *coord)
{
    coord->x = (uint8_t) coord->fx;
    coord->y = (uint8_t) coord->fy;

    clear();
    print_at(coord->x, coord->y);
}

void move_by_x(LEDCoordinates *led)
{
    float nx = led->fx + led->delta_x;
    if ( nx > 0.0 && nx < 32.0) {
        led->fx = nx;
    }
}

void move_by_y(LEDCoordinates *led)
{
    float ny = led->fy + led->delta_y;
    if ( ny > 0.0 && ny < 8.0) {
        led->fy = ny;
    }
}

float smooth_data(int16_t x)
{
    return (x / 17000.0);
}

int main()
{
    int16_t x, y, z;
    struct bmi160_dev sensor;

    UART_init(115200, 0);

    spi_begin(PIN_10_OFFSET, 65000000);
    setup_matrix();

    sensor.id = 0;
    sensor.interface = BMI160_SPI_INTF;
    sensor.read = &spi_read;
    sensor.write = &spi_write;
    sensor.delay_ms = &delay_ms;

    bmi160_init(&sensor);
    config_sensors(&sensor);
    init_plic();

    wdt_disable();
    wdt_configure(0, 32768, 16384);

    LEDCoordinates led;
    led.fx = 15.0;
    led.fy = 7.0;
    sensor_x_ptr = &led.delta_x;
    matrix_fx_ptr = &led.fx;
    while (1) {
        // for safety measures:
        // LEDCoordinates led_dup = led; // should be led_dup(led)
        get_sensor_data(&sensor, &x, &y, &z);
        led.delta_x = smooth_data(x);
        led.delta_y = smooth_data(y);

        if (led.delta_x > 0.03 || led.delta_x < -0.03) {
            move_by_x(&led);
        }
        if (led.delta_y > 0.03 || led.delta_y < -0.03) {
            move_by_y(&led);
        }
        render(&led);
    }
    return 0;
}
