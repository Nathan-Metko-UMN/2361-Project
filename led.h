/*
 * File:   led.h
 * Author: Mohak Dua
 */

#ifndef LED_H
#define LED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Address
#define LED_I2C_ADDR              0x70
#define LED_I2C_WRITE_ADDR        (LED_I2C_ADDR << 1)

// LED commands
#define LED_OSCILLATOR_ON         0x21
#define LED_DISPLAY_ON            0x81
#define LED_DISPLAY_OFF           0x80
#define LED_BLINK_OFF             0x81
#define LED_BRIGHTNESS_BASE       0xE0


#define LED_DISPLAY_RAM_ADDR      0x00

// Matrix dimensions
#define LED_WIDTH          8
#define LED_HEIGHT         8
    
#define ACCEL_MIN_MG              (-2000)
#define ACCEL_MAX_MG              (2000)

void delay_ms(uint32_t ms);
void led_cmd(uint8_t command);
void led_writeDisplay(void);

void led_init(void);
void led_clear(void);
void led_fill(void);
void led_setBrightness(uint8_t brightness);
void led_drawPixel(uint8_t x, uint8_t y, uint8_t on);
void led_setRow(uint8_t row, uint8_t value);

void led_showAccelBall(int16_t x_mg, int16_t y_mg, int16_t z_mg);

// Demo stuff
void led_allOn(void);
void led_allOff(void);
void led_diagonal(void);
void led_border(void);
void led_checkerboard(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */