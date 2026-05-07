/* 
 * File:   main.c
 * Author: henry-goldberg
 *
 * Created on April 29, 2026, 9:55 PM
 * 
 * This is an example of attempting at writing very modular code
 * for the DOGS104 ssd1803a combonation. I based it off of similar
 * systems as the esp32 family but I could not acheive exactly what I wanted
 * especially with i2c initilization. 
 * 
 * Current issues that I have with it is ctx should contain all the information for i2c initilization however
 * the way Microchip has the registers mapped makes this very difficult and
 * out of my abilities (and time) however I hope that the code written below
 * is a good example of how to use this code.
 * 
 * Second issue that I have is that I could not think of a good way to do
 * the command mapping in a timely manner.
 * 
 * General description: i2c_ctx_t is a user defined structure that
 * would in theory hold i2c and pin mapping information. I2c functions
 * timer and pin reset are user defined as per my goal.
 * 
 */
#pragma config ICS = PGx1          // Comm Channel Select (Emulator EMUC1/EMUD1 pins are shared with PGC1/PGD1)
#pragma config FWDTEN = OFF        // Watchdog Timer Enable (Watchdog Timer is disabled)
#pragma config GWRP = OFF          // General Code Segment Write Protect (Writes to program memory are allowed)
#pragma config GCP = OFF           // General Code Segment Code Protect (Code protection is disabled)
#pragma config JTAGEN = OFF        // JTAG Port Enable (JTAG port is disabled)


// CW2: FLASH CONFIGURATION WORD 2 (see PIC24 Family Reference Manual 24.1)
#pragma config I2C1SEL = PRI       // I2C1 Pin Location Select (Use default SCL1/SDA1 pins)
#pragma config IOL1WAY = OFF       // IOLOCK Protection (IOLOCK may be changed via unlocking seq)
#pragma config OSCIOFNC = ON       // Primary Oscillator I/O Function (CLKO/RC15 functions as I/O pin)
#pragma config FCKSM = CSECME      // Clock Switching and Monitor (Clock switching is enabled, 
                                       // Fail-Safe Clock Monitor is enabled)
#pragma config FNOSC = FRCPLL      // Oscillator Select (Fast RC Oscillator with PLL module (FRCPLL))

#define FCY 16000000
#include "xc.h"
#include "driver_DOGS104.h"
#include <libpic30.h>

typedef struct {
    
} i2c_ctx_t;

void i2c_init(void)
{
    I2C1CON = 0;
    I2C1RCV = 0;
    I2C1TRN = 0;
    I2C1BRG = 156;

    I2C1CONbits.I2CEN = 1;
}

void pin_init(void)
{
    CLKDIVbits.RCDIV = 0;       
    AD1PCFG = 0x9FFF;
    
    TRISBbits.TRISB6 = 0;
    LATBbits.LATB6 = 1;
}

void I2C1_WRITE(uint8_t byte)
{
    I2C1TRN = byte;
    while (I2C1STATbits.TRSTAT);
}

static dogs104_err_t write(void *ctx, const uint8_t *buf, size_t len)
{
    (void) ctx;
    
    I2C1CONbits.SEN = 1;                        //START
    while (I2C1CONbits.SEN);
    
    I2C1_WRITE((ADDRESS << 1) | 0x00);
    
    for (size_t i = 0; i < len; ++i) {      //Start 
        I2C1_WRITE(buf[i]);
        if (I2C1STATbits.ACKSTAT) {
            I2C1CONbits.PEN = 1;
            while (I2C1CONbits.PEN);
            return DOGS104_ERR_IO;
        }
    }
    
    I2C1CONbits.PEN = 1;                        //STOP
    while (I2C1CONbits.PEN);
    
    return DOGS104_OK;
}

static void set_rst(void *ctx, bool high)
{
    (void) ctx;
    LATBbits.LATB6 = high ? 1 : 0;
}

static void set_rs(void *ctx, bool high)
{
    (void) ctx;
    LATBbits.LATB5 = high ? 1 : 0;
}

static void delay_ms(void *ctx, uint32_t ms)
{
    (void) ctx;
    __delay_ms(ms);
}

static dogs104_handle_t hand;

int main(void)
{
    //Initlizae your own pins and i2c or SPI depending on what user wants to use
    pin_init();
    i2c_init();
    
    //initlizae LCD with its address and with functions relevent
    dogs104_handle_t lcd = {
        .write   = write,
        .set_rst = set_rst,
        .set_rs = set_rs,
        .delay_ms = delay_ms,
        .ctx      = &hand,
        .i2c_addr = ADDRESS,
    };
    
    //Example of user defined character of a gumball machine
    const uint8_t bitmap[8] = {
        0b10001,
        0b01110,
        0b10001,
        0b10001,
        0b01110,
        0b10001,
        0b10001,
    };
    
    //Check if functions are all set up and good
    if (dogs104_standard_init(&lcd) != DOGS104_OK) {
        while (1);
    }
    dogs104_clear(&lcd);
    
    //Initlize lines and varibles youd like to use 
    int result = 50;
    char line1[16];
    char line2[16];
    char line3[16];
    char line4[16];
    
    //Example of printing out strings
    line1 = "F=";

    dogs104_set_cursor(&lcd, 0, 0);
    dogs104_puts(&lcd, line1);
    
    //Example of printing out custom chars
    dogs104_set_cursor(&lcd, 1, 0);
    dogs104_create_char(&lcd, 0, bitmap);
    dogs104_puts(&my_lcd, "\x00");
    
    //Printf example
    while(1) {
        dogs104_printf("%d", result);
        dogs104_set_cursor(&lcd, 0, 2);
        
        result += 4;
        
        delay_ms(NULL, 250);
    }
    return 0;
}

