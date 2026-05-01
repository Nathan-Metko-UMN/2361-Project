/*
 * File:   led.c
 * Author: Mohak Dua
 */


#include "xc.h"
#include "led.h"
#include <stdlib.h>

#ifndef FCY
#define FCY 16000000UL 
#endif
#include <libpic30.h>


static uint8_t led_buffer[LED_HEIGHT]; // 8x8 matrix buffer

void delay_ms(void *ctx, uint32_t ms)
{
    (void) ctx;
    __delay_ms(ms);
}

// I2C Functions
void led_i2c_start(void) {
    I2C1CONbits.SEN = 1;
    while (I2C1CONbits.SEN);
}

void led_i2c_stop(void) {
    I2C1CONbits.PEN = 1;
    while (I2C1CONbits.PEN);
}

void led_i2c_write(uint8_t data) {
    _MI2C1IF = 0;
    I2C1TRN = data;
    while (!_MI2C1IF);
    _MI2C1IF = 0;
}

void led_cmd(uint8_t command)
{
    led_i2c_start();

    led_i2c_write(LED_I2C_WRITE_ADDR);
    led_i2c_write(command);

    led_i2c_stop();
}

void led_writeDisplay(void)
{
    led_i2c_start();

    led_i2c_write(LED_I2C_WRITE_ADDR);
    led_i2c_write(LED_DISPLAY_RAM_ADDR);

    for (uint8_t row = 0; row < LED_HEIGHT; row++)
    {
        led_i2c_write(led_buffer[row]);
        led_i2c_write(0x00);
    }

    led_i2c_stop();
}

void led_init(void)
{
    delay_ms(NULL, 10);

    led_cmd(LED_OSCILLATOR_ON);
    led_cmd(LED_DISPLAY_ON);
    led_setBrightness(4);

    led_clear();
    led_writeDisplay();
}

void led_clear(void)
{
    for (uint8_t row = 0; row < LED_HEIGHT; row++)
    {
        led_buffer[row] = 0x00;
    }
}

void led_fill(void)
{
    for (uint8_t row = 0; row < LED_HEIGHT; row++)
    {
        led_buffer[row] = 0xFF;
    }
}

void led_setBrightness(uint8_t brightness)
{
    if (brightness > 15)
    {
        brightness = 15;
    }

    led_cmd(LED_BRIGHTNESS_BASE | brightness);
}

void led_drawPixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= LED_WIDTH || y >= LED_HEIGHT)
    {
        return;
    }

    uint8_t mapped_x = (x+7) % 8;

    if (on)
    {
        led_buffer[y] |= (1 << mapped_x);
    }
    else
    {
        led_buffer[y] &= ~(1 << mapped_x);
    }
}

void led_setRow(uint8_t row, uint8_t value)
{
    if (row >= LED_HEIGHT)
    {
        return;
    }

    led_buffer[row] = value;
}

// Helpers for acceleration stuff
int16_t clamp_val(int16_t val, int16_t min, int16_t max)
{
    if(val < min)
    {
        return min;
    }
    
    if(val > max)
    {
        return max;
    }
    
    return val;
}

uint8_t mapAccelToCoord(int16_t accel_mg) {
    if (accel_mg > -100 && accel_mg < 100) accel_mg = 0;
    accel_mg = clamp_val(accel_mg, ACCEL_MIN_MG*2, ACCEL_MAX_MG*2);

    int32_t shifted = (int32_t) accel_mg - ACCEL_MIN_MG;
    int32_t span = (int32_t) ACCEL_MAX_MG - ACCEL_MIN_MG;

    return (uint8_t) ((shifted * 7L + (span / 2)) / span);
}

uint8_t mapAccelToBrightness(int16_t z_mg)
{
    z_mg = clamp_val(z_mg, ACCEL_MIN_MG*2, ACCEL_MAX_MG*2);

    int32_t shifted = (int32_t)z_mg - ACCEL_MIN_MG;
    int32_t span = (int32_t)ACCEL_MAX_MG - ACCEL_MIN_MG;

    return (uint8_t)((shifted * 15L + (span / 2)) / span);
}
void mapZToBallSize(int16_t z_mg, uint8_t *width, uint8_t *height)
{
    z_mg = clamp_val(z_mg, ACCEL_MIN_MG*2, ACCEL_MAX_MG*2);
    
    int16_t size = z_mg / 235;

    *width = (uint8_t)abs(size); 

    *height = *width;
}

// Main function for demo
void led_showAccelBall(int16_t x_mg, int16_t y_mg, int16_t z_mg)
{
    uint8_t x = mapAccelToCoord(x_mg);
    uint8_t y = mapAccelToCoord(y_mg);
    uint8_t brightness = mapAccelToBrightness(z_mg);

    uint8_t ball_width;
    uint8_t ball_height;

    mapZToBallSize(z_mg, &ball_width, &ball_height);
    
    int16_t start_x = (int16_t)x - (ball_width / 2);
    int16_t start_y = (int16_t)y - (ball_height / 2);

    led_clear();
    led_setBrightness(brightness);


    for (uint8_t dy = 0; dy < ball_height; dy++)
    {
        for (uint8_t dx = 0; dx < ball_width; dx++)
        {
            int16_t draw_x = start_x + dx;
            int16_t draw_y = start_y + dy;

            if (draw_x >= 0 && draw_x < LED_WIDTH && draw_y >= 0 && draw_y < LED_HEIGHT)
            {
                led_drawPixel((uint8_t)draw_x, (uint8_t)draw_y, 1);
            }
        }
        
        led_writeDisplay();
    }
}
// Testing functions
void led_allOn(void)
{
    led_fill();
    led_writeDisplay();
}

void led_allOff(void)
{
    led_clear();
    led_writeDisplay();
}

void led_diagonal(void)
{
    led_clear();

    for (uint8_t i = 0; i < 8; i++)
    {
        led_drawPixel(i, i, 1);
    }

    led_writeDisplay();
}

void led_border(void)
{
    led_clear();

    for (uint8_t x = 0; x < 8; x++)
    {
        led_drawPixel(x, 0, 1);
        led_drawPixel(x, 7, 1);
    }

    for (uint8_t y = 0; y < 8; y++)
    {
        led_drawPixel(0, y, 1);
        led_drawPixel(7, y, 1);
    }

    led_writeDisplay();
}

void led_checkerboard(void)
{
    led_clear();

    for (uint8_t y = 0; y < 8; y++)
    {
        for (uint8_t x = 0; x < 8; x++)
        {
            if ((x + y) % 2)
            {
                led_drawPixel(x, y, 1);
            }
        }
    }

    led_writeDisplay();
}