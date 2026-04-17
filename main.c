/* 
 * File:   main.c
 * Author: natha
 *
 * Created on April 13, 2026, 12:00 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <xc.h>

#include "i2c_lis3dh.h"
#include "lis3dh_reg.h"
#include "uart_lib.h"
#include "spi_lib.h"
#include "RFM69.h"

#ifndef FCY
#define FCY 16000000UL
#endif
#include <libpic30.h>

#define PI 3.14159265f

// CW1: FLASH CONFIGURATION WORD 1 (see PIC24 Family Reference Manual 24.1)
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


#define NETWORK_ID    100  // Must be the same for all nodes
#define MY_NODE_ID    1    // The ID of this specific radio
#define TARGET_ID     2    // The ID of the radio you want to talk to
#define FREQUENCY     433  // 433MHz

void setup(){
    CLKDIVbits.RCDIV = 0; 

    // Make all pins digital (Crucial for pins 25, 26, 6, and 7)
    AD1PCFG = 0xFFFF;

    // --- RFM69 Radio Pins ---
    TRISBbits.TRISB15 = 0; // Pin 26 (RST) is an Output
    TRISBbits.TRISB14 = 0; // Pin 25 (CS)  is an Output
    TRISBbits.TRISB2  = 0; // Pin 6  (MOSI) is an Output
    TRISBbits.TRISB3  = 1; // Pin 7  (MISO) is an Input
    TRISBbits.TRISB4  = 0; // Pin 11 (SCK) is an Output
    TRISBbits.TRISB7  = 1; // Pin 16 (G0/INT0) is an Input

    LATBbits.LATB15 = 0; // Keep RST low by default
    LATBbits.LATB14 = 1; // Keep CS high (unselected) by default

    // --- SPI Hardware Routing (PPS) ---
    __builtin_write_OSCCONL(OSCCON & 0xbf); // Unlock the PPS registers
    RPINR20bits.SDI1R = 3;                  // Map MISO to Pin 7 (RP3)
    RPOR1bits.RP2R = 7;                     // Map MOSI to Pin 6 (RP2)
    RPOR2bits.RP4R = 8;                     // Map SCK to Pin 11 (RP4)
    __builtin_write_OSCCONL(OSCCON | 0x40); // Lock the PPS registers
    
    // --- I2C Setup ---
    I2C1CONbits.I2CEN = 0; // Disable I2C to configure it safely
    // I2C1BRG = (16,000,000 / 100,000) - 1.6 - 1 = 157.4 -> 157
    I2C1BRG = 157; 
    IFS1bits.MI2C1IF = 0;  // Clear Interrupt Flags
    I2C1CONbits.I2CEN = 1; // Enable I2C Module
    
    // --- Heartbeat LED ---
    TRISBbits.TRISB5 = 0; 
    LATBbits.LATB5 = 0;
}

void reset_radio(){
    LATBbits.LATB15 = 1;      // Pull RST high
    __delay_ms(10);           // Hold high for 10ms
    LATBbits.LATB15 = 0;      // Pull RST low
    __delay_ms(10);           // Wait for the chip to settle
}

// Expose the internal library variables so we can read them in main.c
extern volatile uint8_t DATALEN;
extern volatile uint8_t SENDERID;
extern volatile int16_t RSSI;

// ==========================================================
// TRANSMIT DATA
// Returns true if the message was delivered and acknowledged
// ==========================================================
bool transmitMessage(uint8_t targetNode, const char* message) {
    
    // sendWithRetry(Target ID, Data, Length, Retries, Timeout between retries in ms)
    // We will try to send the message 3 times, waiting 40ms for an ACK each time.
    if (sendWithRetry(targetNode, message, strlen(message), 3, 40)) {
        return true;  // Delivered successfully!
    }
    return false;     // No response from the target
}

// ==========================================================
// RECEIVE DATA
// Returns true if a new message arrived.
// Extracts the message, the sender's ID, and signal strength.
// ==========================================================
bool receiveMessage(char* messageBuffer, uint8_t* senderId, int16_t* signalStrength) {
    
    // Check if the INT0 hardware interrupt caught a new packet
    if (receiveDone()) {
        
        // 1. Copy the library's internal DATA array to your own buffer
        memcpy(messageBuffer, (void*)DATA, DATALEN);
        messageBuffer[DATALEN] = '\0'; // Null-terminate to make it a safe string
        
        // 2. Save the sender details
        *senderId = SENDERID;
        *signalStrength = RSSI;

        // 3. Automatically reply if the sender asked for a read receipt (ACK)
        if (ACKRequested()) {
            sendACK("ACK", 3);
        }
        
        return true; // We caught a message!
    }
    
    return false; // No new messages
}

int main(void) {
    CLKDIVbits.RCDIV = 0; 
    
    // 1. Initialize basics first
    setup();
    setup_uart();
    spi_init();
    millis_init();
    
    reset_radio();
    rfm69_init(FREQUENCY, MY_NODE_ID, NETWORK_ID);
        
    stmdev_ctx_t dev_ctx;
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg  = platform_read;
    dev_ctx.mdelay    = platform_delay;
    dev_ctx.handle    = NULL; 

    uint8_t whoamI;
    lis3dh_device_id_get(&dev_ctx, &whoamI);
    
    // 4. Graceful Error Handling
    if (whoamI != LIS3DH_ID) {
        raw_print("ERROR: LIS3DH not found!\r\n");
        while(1) {
            // Blink the LED very fast so you visually know there is an I2C error
            LATBbits.LATB5 = ~LATBbits.LATB5; 
            for(long i=0; i<100000; i++); // Dumb delay
        }
    }

    raw_print("LIS3DH Found! Starting loop...\r\n");

    // Configure the sensor
    lis3dh_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
    lis3dh_data_rate_set(&dev_ctx, LIS3DH_ODR_100Hz);
    lis3dh_operating_mode_set(&dev_ctx, LIS3DH_HR_12bit);
    lis3dh_full_scale_set(&dev_ctx, LIS3DH_2g);
    
    while(1) {
        LATBbits.LATB5 = 1; // Turn ON
                    
        uint8_t dataready;
        lis3dh_xl_data_ready_get(&dev_ctx, &dataready);

        if (dataready) {
            int16_t data_raw_acceleration[3];
            float acceleration_mg[3];

            lis3dh_acceleration_raw_get(&dev_ctx, data_raw_acceleration);

            acceleration_mg[0] = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[0]);
            acceleration_mg[1] = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[1]);
            acceleration_mg[2] = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[2]);

            float roll_rad = atan2(acceleration_mg[1], acceleration_mg[2]);
            float pitch_rad = atan2(-acceleration_mg[0], sqrt((acceleration_mg[1] * acceleration_mg[1]) + (acceleration_mg[2] * acceleration_mg[2])));

            char tx_buffer[50]; 

            float roll_deg = roll_rad * (180.0f / PI);
            float pitch_deg = pitch_rad * (180.0f / PI);

            sprintf(tx_buffer, "Roll: %d\r\n", (int)roll_deg);
            raw_print(tx_buffer);

            sprintf(tx_buffer, "Pitch: %d\r\n", (int)pitch_deg);
            raw_print(tx_buffer);            
            LATBbits.LATB5 = 0; // Turn OFF
        }
        
        transmitMessage(2,"POOP");
        
        // FIX: Add a physical delay so the LED has time to stay off before the loop repeats
        for(long i=0; i<150000; i++); 
    }
}