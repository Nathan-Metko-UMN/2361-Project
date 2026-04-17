/* 
 * File:   i2c_lis3dh.c
 * Author: natha
 *
 * Created on April 13, 2026, 11:56 AM
 */

#include <xc.h>
#include <stdint.h>
#include "i2c_lis3dh.h"

// Define FCY for the __delay_ms() macro (e.g., 16 MHz instruction clock)
#ifndef FCY
#define FCY 16000000UL
#endif
#include <libpic30.h>

// =====================================================================
// Internal I2C Bare-Metal Primitives (Not exposed in .h)
// =====================================================================

static void i2c_wait(void) {
    while (!IFS1bits.MI2C1IF); // Wait for the I2C interrupt flag
    IFS1bits.MI2C1IF = 0;      // Clear the flag before the next operation
}

static void i2c_start(void) {
    I2C1CONbits.SEN = 1;       // Initiate START condition
    while (I2C1CONbits.SEN);
    IFS1bits.MI2C1IF = 0; 
}

static void i2c_restart(void) {
    I2C1CONbits.RSEN = 1;      // Initiate Repeated START condition
    while (I2C1CONbits.RSEN);
    IFS1bits.MI2C1IF = 0; 
}

static void i2c_stop(void) {
    I2C1CONbits.PEN = 1;       // Initiate STOP condition 
    while (I2C1CONbits.PEN);
}

static void i2c_write(uint8_t data) {
    I2C1TRN = data; 
    i2c_wait();
}

static uint8_t i2c_read(uint8_t ack) {
    uint8_t data;
    
    I2C1CONbits.RCEN = 1;      // Enable I2C Receive mode
    i2c_wait();                // Wait for data to arrive
    data = I2C1RCV;            // Read the buffer
    
    // Send ACK (0) if we want more bytes, or NACK (1) if it's the last byte
    I2C1CONbits.ACKDT = ack;   
    I2C1CONbits.ACKEN = 1;     // Initiate Acknowledge sequence
    while(I2C1CONbits.ACKEN);  // Wait for ACK to complete
    
    return data;
}

// =====================================================================
// Public Functions (Exposed in .h)
// =====================================================================

int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    // LIS3DH Multi-byte write requires the MSB of the register address to be set to 1
    if (len > 1) {
        reg |= 0x80;
    }

    i2c_start();
    i2c_write(LIS3DH_I2C_WRITE); // Send Device Address + Write Bit
    i2c_write(reg);              // Send Register Address
    
    for(uint16_t i = 0; i < len; i++) {
        i2c_write(bufp[i]);      // Send Data payload sequentially
    }
    
    i2c_stop();
    return 0; // Return 0 -> no Error
}

int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    // LIS3DH Multi-byte read requires the MSB of the register address to be set to 1
    if (len > 1) {
        reg |= 0x80;
    }

    i2c_start();
    i2c_write(LIS3DH_I2C_WRITE); // Send Device Address + Write Bit
    i2c_write(reg);              // Send Register Address
    
    i2c_restart();               // Repeated start to switch to Read Mode
    i2c_write(LIS3DH_I2C_READ);  // Send Device Address + Read Bit
    
    for(uint16_t i = 0; i < len; i++) {
        // If it's the last byte to read, send NACK (1). Otherwise, send ACK (0).
        uint8_t ack_bit = (i == (len - 1)) ? 1 : 0; 
        bufp[i] = i2c_read(ack_bit);
    }
    
    i2c_stop();
    return 0; // Return 0 -> no Error
}

void platform_delay(uint32_t millisec) {
    // Requires FCY and <libpic30.h> to be defined at the top of the file
    for(uint32_t i = 0; i < millisec; i++){
        __delay_ms(1); 
    }
}