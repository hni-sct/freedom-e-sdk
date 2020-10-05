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
#include <stdint.h>
#include "platform.h"

#define BIT(x) (1 << x)

#define DIN PIN_4_OFFSET
#define CS PIN_5_OFFSET
#define CLK PIN_6_OFFSET

#define max7219_reg_noop        0x00
#define max7219_reg_digit0      0x01
#define max7219_reg_digit1      0x02
#define max7219_reg_digit2      0x03
#define max7219_reg_digit3      0x04
#define max7219_reg_digit4      0x05
#define max7219_reg_digit5      0x06
#define max7219_reg_digit6      0x07
#define max7219_reg_digit7      0x08
#define max7219_reg_decodeMode  0x09
#define max7219_reg_intensity   0x0a
#define max7219_reg_scanLimit   0x0b
#define max7219_reg_shutdown    0x0c
#define max7219_reg_displayTest 0x0f

#define HIGH 1
#define LOW 0

const int num_matrices = 4;
uint8_t matrix[32][8];

void set_pin(uint32_t pin, uint8_t val)
{
    if (val == 0) {
        GPIO_REG(GPIO_OUTPUT_VAL) &= ~BIT(pin);
    } else {
        GPIO_REG(GPIO_OUTPUT_VAL) |= BIT(pin);
    }
}

void send_byte(uint8_t b)
{
    for (int i = 7; i >= 0; i--) {
        set_pin(DIN, (b >> i) & 1);
        set_pin(CLK, HIGH);
        set_pin(CLK, LOW);
    }
}

void send_command(uint8_t cmd, uint8_t val)
{
    set_pin(CS, LOW);
    for(int i=0; i < num_matrices; i++) {
        send_byte(cmd);
        send_byte(val);
    }
    set_pin(CS, HIGH);
}

#define assign_bit(res, n, v) do {\
    res ^= (-v ^ res) & (1UL << n); \
} while (0)

void set_column(uint8_t col, uint8_t val)
{
    uint8_t n = col / 8;
    uint8_t c = col % 8;

    set_pin(CS, LOW);
    for (int i=0; i < num_matrices; i++) {
        if (i == n) {
            send_byte(c + 1);
            send_byte(val);
        } else {
            send_byte(0);
            send_byte(0);
        }
    }
    set_pin(CS, HIGH);
}

void set_column_all(uint8_t col, uint8_t val)
{
    set_pin(CS, LOW);
    for (int i=0; i<num_matrices; i++) {
        send_byte(col + 1);
        send_byte(val);
    }
    set_pin(CS, HIGH);
}

void clear(void)
{
    for (int i=0; i < 8; ++i) {
        set_column_all(i, 0);
    }
}

void set_intensity(uint8_t intensity)
{
    send_command(max7219_reg_intensity, intensity);
}

void setup_matrix(void)
{
    GPIO_REG(GPIO_OUTPUT_EN) |= BIT(PIN_4_OFFSET);
    GPIO_REG(GPIO_INPUT_EN) &= ~BIT(PIN_4_OFFSET);

    GPIO_REG(GPIO_OUTPUT_EN) |= BIT(PIN_5_OFFSET);
    GPIO_REG(GPIO_INPUT_EN) &= ~BIT(PIN_5_OFFSET);

    GPIO_REG(GPIO_OUTPUT_EN) |= BIT(PIN_6_OFFSET);
    GPIO_REG(GPIO_INPUT_EN) &= ~BIT(PIN_6_OFFSET);

    set_pin(CS, HIGH);

    send_command(max7219_reg_scanLimit, 0x07);
    send_command(max7219_reg_decodeMode, 0x00);
    send_command(max7219_reg_shutdown, 0x01);
    send_command(max7219_reg_displayTest, 0x00);

    clear();

    set_intensity(0x0f);
}

void print_at(uint32_t x, uint32_t y)
{
    uint32_t rx = x % 8;
    uint32_t yf = x / 8;
    uint8_t val = (1 << (7 - rx));
    set_column(y + 8 * yf, val);
}

void set_point(uint32_t x, uint32_t y, uint8_t val)
{
    if ( (x < 32 && x >= 0) && ((y < 8) && (y >=0)) ) {
        matrix[x][y] = val;
    }
}

void draw_matrix()
{
    uint32_t yf, x, y;
    uint8_t val;

    for (int dy=0; dy < 32; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            yf = dy / 8;
            y = dy % 8;
            x = (8 - dx) + (yf * 8);
            if (matrix[x][y] == 0) {
                assign_bit(val, matrix[x][y], x);
            }
        }
    }
}
