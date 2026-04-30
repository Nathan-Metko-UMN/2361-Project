#ifndef ADAFRUIT_LED_BACKPACK_H
#define	ADAFRUIT_LED_BACKPACK_H
#include "xc.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef	__cplusplus
extern "C" {
#endif
void I2C1_Init(void);
void I2C1_Start(void);
void I2C1_stop(void);
void I2C1_Write(uint8_t data);
void HT16K33_WriteCmd(uint8_t cmd);
void matrix_init(void);
void matrix_writeDisplay(void);
void matrix_clear(void);
void matrix_setBrightness(uint8_t b);
void matrix_blinkRate(uint8_t b);
void drawPixel(int16_t x, int16_t y, uint16_t color);
#ifdef	__cplusplus
}
#endif

#endif	/* ADAFRUIT_LED_BACKPACK_H */

