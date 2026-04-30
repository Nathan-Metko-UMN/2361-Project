/* 
 * File:   uart_lib.c
 * Author: natha
 *
 * Created on April 16, 2026, 11:37 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>


#include "uart_lib.h"

volatile unsigned char buffer[64];
volatile unsigned char front = 0;
volatile unsigned char back = 0;

void __attribute__((__interrupt__, __auto_psv__)) _U1RXInterrupt(void) {
    IFS0bits.U1RXIF = 0;
    buffer[front++] = U1RXREG;
    front &= 63;
}

void putch(unsigned char c) { // Renamed for printf compatibility
    while (U1STAbits.UTXBF);
    U1TXREG = c;
}

void raw_print(char* str) {
    while(*str != '\0') {
        while (U1STAbits.UTXBF); // Wait if the buffer is full
        U1TXREG = *str++;        // Shove the character directly into the hardware
    }
}
unsigned char PGetch(void) {
    unsigned char x;

    while (front == back);
    x = buffer[back++];
    back &= 63;
    return x;
}

void setup_uart(void) {
    AD1PCFG = 0x9fff; // For digital I/O.

    // I think the following two lines are irrelevant. The UART doc
    // Page 3, when describing the UARTEN bit in UxMODE says that when UARTEN
    // is set, TRISx are ignored and instead UEN and UTXEN control pins.
    _TRISB6 = 0; // U1TX output
    _TRISB10 = 1; // U1RX input

    U1MODE = 0; // UEN<1:0> bits control the pins
    // U1BRG = 34; // 115200 baud,
    // U1MODEbits.BRGH = 1;
    U1MODEbits.BRGH = 0;
    U1BRG = 25; // 38400 baud (check the calculation with the datasheet)
    U1MODEbits.UEN = 0;
    U1MODEbits.UARTEN = 1;
    U1STAbits.UTXEN = 1;

    // Peripheral Pin Select 
    __builtin_write_OSCCONL(OSCCON & 0xbf); // unlock PPS
    _RP6R = 0x0003; //RB6->UART1:U1TX; See Table 10-3 on P109 of the datasheet
    _U1RXR = 10; //RB10->UART1:U1RX;
    __builtin_write_OSCCONL(OSCCON | 0x40); // lock   PPS

    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1;
}
