
#include "Adafruit_LED_Backpack.h"
#include "xc.h"
#include <stdio.h>
#include <stdlib.h>


#define HT16K33_ADDR 0x70   // default address

// Display buffer for 8x8 matrix
uint16_t displaybuffer[8];

// ---------------- I2C FUNCTIONS ----------------
void I2C1_Init(void) {
    I2C1BRG = 157;              // ~100kHz @ 4MHz FCY
    I2C1CONbits.I2CEN = 1;
}

void I2C1_Start(void) {
    I2C1CONbits.SEN = 1;
    while(I2C1CONbits.SEN);
}

void I2C1_Stop(void) {
    I2C1CONbits.PEN = 1;
    while(I2C1CONbits.PEN);
}

void I2C1_Write(uint8_t data) {
    I2C1TRN = data;
    IFS1bits.MI2C1IF = 0;
    while(I2C1STATbits.TRSTAT);
    while (!_MI2C1IF);
}

void HT16K33_WriteCmd(uint8_t cmd) {
    I2C1_Start();
    I2C1_Write(HT16K33_ADDR);
    I2C1_Write(cmd);
    I2C1_Stop();
}

// ---------------- MATRIX FUNCTIONS ----------------
void matrix_init(void) {
    HT16K33_WriteCmd(0x21); // turn on oscillator
    HT16K33_WriteCmd(0x81); // display on, no blink
    HT16K33_WriteCmd(0xEF); // max brightness

    for(int i=0;i<8;i++) displaybuffer[i]=0;
}

void matrix_writeDisplay(void) {
    I2C1_Start();
    I2C1_Write(HT16K33_ADDR);
    I2C1_Write(0x00); //start at top row

    for(int i=0;i<8;i++) {
        I2C1_Write(displaybuffer[i] & 0xFF); //turn on or off the led
        I2C1_Write(displaybuffer[i] >> 8); //go to next row
    }

    I2C1_Stop();
}

void matrix_clear(void) {
    for(int i=0;i<8;i++) {
        displaybuffer[i] = 0;
    }
}

void matrix_setBrightness(uint8_t b) {
    if(b > 15) b = 15;
    HT16K33_WriteCmd(0xE0 | b);
}

void matrix_blinkRate(uint8_t b) {
    if(b > 3) b = 0;
    HT16K33_WriteCmd(0x80 | 0x01 | (b << 1));
}

//updates array of led that need to be turned on
void drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((y < 0) || (y >= 8))
    return;
  if ((x < 0) || (x >= 8))
    return;


  // wrap around the x
  x += 7;
  x %= 8;

  if (color) {
    displaybuffer[y] |= 1 << x;
  } else {
    displaybuffer[y] &= ~(1 << x);
  }
}
