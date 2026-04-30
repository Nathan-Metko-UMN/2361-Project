/*
 * File: main.c
 * Merged LCD + LED matrix/RFM69 receiver main
 */

#include "xc.h"

#include "led.h"
#include "spi_lib.h"
#include "RFM69.h"
#include "get_millis.h"
#include "accel.h"
#include "driver_DOGS104.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#ifndef FCY
#define FCY 16000000UL
#endif
#include <libpic30.h>

// Configs: keep only one copy of each config bit
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

// DOGS104 LCD geometry
#define LCD_COLS      10

extern volatile uint8_t DATALEN;
extern volatile uint8_t SENDERID;
extern volatile int16_t RSSI;

static void i2c1_write_byte(uint8_t byte)
{
    I2C1TRN = byte;
    while (I2C1STATbits.TRSTAT);
}

static dogs104_err_t lcd_i2c_write(void *ctx, const uint8_t *buf, size_t len)
{
    dogs104_handle_t *lcd = (dogs104_handle_t *)ctx;
    uint8_t addr = lcd ? lcd->i2c_addr : ADDRESS;

    I2C1CONbits.SEN = 1;                    // START
    while (I2C1CONbits.SEN);

    i2c1_write_byte((addr << 1) | 0x00);     // 7-bit address + write bit
    if (I2C1STATbits.ACKSTAT) {
        I2C1CONbits.PEN = 1;
        while (I2C1CONbits.PEN);
        return DOGS104_ERR_IO;
    }

    for (size_t i = 0; i < len; ++i) {
        i2c1_write_byte(buf[i]);
        if (I2C1STATbits.ACKSTAT) {
            I2C1CONbits.PEN = 1;
            while (I2C1CONbits.PEN);
            return DOGS104_ERR_IO;
        }
    }

    I2C1CONbits.PEN = 1;                    // STOP
    while (I2C1CONbits.PEN);

    return DOGS104_OK;
}

static void lcd_set_rst(void *ctx, bool high)
{
    (void)ctx;
    LATBbits.LATB6 = high ? 1 : 0;
}

static void lcd_delay_ms(void *ctx, uint32_t ms)
{
    (void)ctx;
    __delay_ms(ms);
}

static void setup(void)
{
    CLKDIVbits.RCDIV = 0;
    AD1PCFG = 0xFFFF;        // all analog-capable pins digital

    // Radio pins
    TRISBbits.TRISB15 = 0;   // RFM69 reset
    TRISBbits.TRISB14 = 0;   // RFM69 chip select
    TRISBbits.TRISB2  = 0;   // SPI MOSI
    TRISBbits.TRISB3  = 1;   // SPI MISO
    TRISBbits.TRISB4  = 0;   // SPI SCK
    TRISBbits.TRISB7  = 1;   // RFM69 DIO0 / INT0

    LATBbits.LATB15 = 0;
    LATBbits.LATB14 = 1;

    // LCD reset pin
    TRISBbits.TRISB6 = 0;
    LATBbits.LATB6 = 1;

    // SPI1 PPS mapping for RFM69
    __builtin_write_OSCCONL(OSCCON & 0xBF);
    RPINR20bits.SDI1R = 3;   // SDI1 <- RP3/RB3
    RPOR1bits.RP2R = 7;      // RP2/RB2 -> SDO1
    RPOR2bits.RP4R = 8;      // RP4/RB4 -> SCK1OUT
    __builtin_write_OSCCONL(OSCCON | 0x40);

    // I2C1 setup for LCD
    I2C1CON = 0;
    I2C1RCV = 0;
    I2C1TRN = 0;
    I2C1BRG = 157;
    IFS1bits.MI2C1IF = 0;
    I2C1CONbits.I2CEN = 1;
}

static void reset_radio(void)
{
    LATBbits.LATB15 = 1;
    __delay_ms(10);
    LATBbits.LATB15 = 0;
    __delay_ms(10);
}

static void radio_init_receiver(void)
{
    reset_radio();
    rfm69_init(FREQUENCY, MY_NODE_ID, NETWORK_ID);
    receiveBegin();
}

static bool receiveAccelPacket(AccelData *packet)
{
    if (receiveDone()) {
        if (SENDERID != SENDER_ID) {
            if (ACKRequested()) {
                sendACK("BAD", 3);
            }
            receiveBegin();
            return false;
        }

        if (DATALEN == sizeof(AccelData)) {
            memcpy(packet, (const void *)DATA, sizeof(AccelData));

            if (ACKRequested()) {
                sendACK("ACK", 3);
            }

            receiveBegin();
            return true;
        }

        if (ACKRequested()) {
            sendACK("BAD", 3);
        }
        receiveBegin();
    }

    return false;
}

static void lcd_write_line(dogs104_handle_t *lcd, uint8_t row, const char *text)
{
    char line[LCD_COLS + 1];

    // Left-justify and pad with spaces so old longer values are overwritten.
    snprintf(line, sizeof(line), "%-10.10s", text);
    dogs104_set_cursor(lcd, row, 0);
    dogs104_puts(lcd, line);
}

static void lcd_show_packet(dogs104_handle_t *lcd, const AccelData *packet)
{
    char line[LCD_COLS + 1];

    snprintf(line, sizeof(line), "X:%d", packet->x_mg);
    lcd_write_line(lcd, 0, line);

    snprintf(line, sizeof(line), "Y:%d", packet->y_mg);
    lcd_write_line(lcd, 1, line);

    snprintf(line, sizeof(line), "Z:%d", packet->z_mg);
    lcd_write_line(lcd, 2, line);

    snprintf(line, sizeof(line), "R:%d", RSSI);
    lcd_write_line(lcd, 3, line);
}

int main(void)
{
    AccelData packet;

    setup();

    dogs104_handle_t lcd = {
        .write    = lcd_i2c_write,
        .set_rst  = lcd_set_rst,
        .delay_ms = lcd_delay_ms,
        .ctx      = NULL,       // assigned below so callback can read i2c_addr
        .i2c_addr = ADDRESS,
    };
    lcd.ctx = &lcd;

    if (dogs104_init(&lcd) != DOGS104_OK) {
        while (1);
    }

    dogs104_cmd(&lcd, COMMAND_CLEAR_DISPLAY);
    __delay_ms(50);
    lcd_write_line(&lcd, 0, "LCD ready");
    lcd_write_line(&lcd, 1, "Radio init");

    led_init();
    led_allOff();

    radio_init_receiver();
    lcd_write_line(&lcd, 1, "Waiting");

    while (1) {
        if (receiveAccelPacket(&packet)) {
            led_showAccelBall(packet.x_mg,
                              packet.y_mg,
                              packet.z_mg);

            lcd_show_packet(&lcd, &packet);
        }
    }

    return 0;
}
