/*
 * File:   main.c
 * Author: Mohak Dua
 */

// Includes

#include "xc.h"
#include "led.h"

#include "spi_lib.h"
#include "RFM69.h"
#include "get_millis.h"
#include "accel.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef FCY
#define FCY 16000000UL
#endif
#include <libpic30.h>

// Configs
#pragma config ICS = PGx1
#pragma config FWDTEN = OFF
#pragma config GWRP = OFF
#pragma config GCP = OFF
#pragma config JTAGEN = OFF

#pragma config I2C1SEL = PRI
#pragma config IOL1WAY = OFF
#pragma config OSCIOFNC = ON
#pragma config FCKSM = CSECME
#pragma config FNOSC = FRCPLL

// Radio IDs
#define NETWORK_ID    100
#define MY_NODE_ID    2
#define SENDER_ID     1
#define FREQUENCY     433

extern volatile uint8_t DATALEN;
extern volatile uint8_t SENDERID;
extern volatile int16_t RSSI;

/*
 * Radio Wiring
 * RB15 - RST
 * RB14 - CS / NSS
 * RB2  - MOSI
 * RB3  - MISO
 * RB4  - SCK
 * RB7  - DIO0 / G0 / INT0
 */

void setup(void)
{
    CLKDIVbits.RCDIV = 0;

    AD1PCFG = 0xFFFF;

    TRISBbits.TRISB15 = 0;
    TRISBbits.TRISB14 = 0;
    TRISBbits.TRISB2  = 0;
    TRISBbits.TRISB3  = 1;
    TRISBbits.TRISB4  = 0;
    TRISBbits.TRISB7  = 1;

    LATBbits.LATB15 = 0;
    LATBbits.LATB14 = 1;

    __builtin_write_OSCCONL(OSCCON & 0xBF);

    RPINR20bits.SDI1R = 3;
    RPOR1bits.RP2R = 7;
    RPOR2bits.RP4R = 8;

    __builtin_write_OSCCONL(OSCCON | 0x40);

    I2C1CONbits.I2CEN = 0;
    I2C1BRG = 157;
    IFS1bits.MI2C1IF = 0;
    I2C1CONbits.I2CEN = 1;
}

void reset_radio(void)
{
    LATBbits.LATB15 = 1;
    __delay_ms(10);

    LATBbits.LATB15 = 0;
    __delay_ms(10);
}

void radio_init_receiver(void)
{
    reset_radio();
    rfm69_init(FREQUENCY, MY_NODE_ID, NETWORK_ID);
    receiveBegin();
}

bool receiveAccelPacket(AccelData *packet)
{
    if (receiveDone())
    {
        if (SENDERID != SENDER_ID)
        {
            if (ACKRequested())
            {
                sendACK("BAD", 3);
            }

            receiveBegin();
            return false;
        }

        if (DATALEN == sizeof(AccelData))
        {
            memcpy(packet, (const void *)DATA, sizeof(AccelData));

            if (ACKRequested())
            {
                sendACK("ACK", 3);
            }

            receiveBegin();
            return true;
        }

        if (ACKRequested())
        {
            sendACK("BAD", 3);
        }

        receiveBegin();
    }

    return false;
}

int main(void)
{
    AccelData packet;

    setup();

    led_init();
    led_allOff();

    radio_init_receiver();

    while (1)
    {
        if (receiveAccelPacket(&packet))
        {
            led_showAccelBall(packet.x_mg,
                              packet.y_mg,
                              packet.z_mg);
        }
    }

    return 0;
}