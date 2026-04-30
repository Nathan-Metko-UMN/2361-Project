/* 
 * File:   spi_lib.c
 * Author: natha
 *
 * Created on April 17, 2026, 4:59 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

#include "spi_lib.h"

void spi_init(void) {
    // 1. Disable the SPI1 module before configuring it
    SPI1STATbits.SPIEN = 0;

    // 2. Clear the SPI buffer
    SPI1BUF = 0;

    // 3. Clear overflow flag
    SPI1STATbits.SPIROV = 0;

    // 4. Configure SPI1 Control Register 1
    SPI1CON1bits.MSTEN = 1; // Set PIC24 as Master
    SPI1CON1bits.MODE16 = 0; // 8-bit communication mode

    // Configure for SPI Mode 0 (Required by RFM69)
    // CPOL = 0 (Clock idle is low), CPHA = 0 (Data changes on active-to-idle)
    SPI1CON1bits.CKP = 0;
    SPI1CON1bits.CKE = 1;

    // 5. Set SPI Clock Speed 
    // Max for RFM69 is 10MHz. Assuming FCY = 16MHz:
    // Primary 1:1, Secondary 4:1 = 4MHz SPI Clock. 
    SPI1CON1bits.PPRE = 3; // Primary prescaler 1:1
    SPI1CON1bits.SPRE = 5; // Secondary prescaler 4:1

    // 6. Turn the SPI module back on
    SPI1STATbits.SPIEN = 1;
}

uint8_t spi_transfer(uint8_t data) {
    uint8_t dummy;

    // Write the byte out to the PIC24's hardware SPI transmit buffer
    SPI1BUF = data;

    // Wait until the hardware sets the "Receive Buffer Full" flag
    // This indicates the byte was sent AND a byte was simultaneously received
    while (!SPI1STATbits.SPIRBF);

    // Read the received byte from the hardware buffer
    dummy = SPI1BUF;

    return dummy;
}