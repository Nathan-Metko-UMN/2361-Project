#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <xc.h>

#include "i2c_lis3dh.h"
#include "lis3dh_reg.h"
#include "get_millis.h"
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

    AD1PCFG = 0xFFFF;

    // --- RFM69 Radio Pins ---
    TRISBbits.TRISB11 = 0; // Pin 22 (RST) is an Output
    TRISBbits.TRISB12 = 0; // Pin 23 (CS)  is an Output
    TRISBbits.TRISB13  = 0; // Pin 24  (MOSI) is an Output
    TRISBbits.TRISB14  = 1; // Pin 25 (MISO) is an Input
    TRISBbits.TRISB15  = 0; // Pin 26 (SCK) is an Output
    TRISBbits.TRISB7  = 1; // Pin 16 (G0/INT0) is an Input

    LATBbits.LATB11 = 0; // Keep RST low by default
    LATBbits.LATB12 = 1; // Keep CS high (unselected) by default

    // --- SPI Hardware Routing (PPS) ---
    __builtin_write_OSCCONL(OSCCON & 0xbf); // Unlock the PPS registers
    RPINR20bits.SDI1R = 14;                  // Map MISO to Pin 25 (RP14)
    RPOR6bits.RP13R = 7;                     // Map MOSI to Pin 24 (RP13)
    RPOR7bits.RP15R = 8;                     // Map SCK to Pin 26 (RP15)
    __builtin_write_OSCCONL(OSCCON | 0x40); // Lock the PPS registers
        
    // --- Heartbeat LED ---
    TRISBbits.TRISB5 = 0; 
    LATBbits.LATB5 = 0;
    
     // --- I2C Setup ---
    I2C1CONbits.I2CEN = 0; // Disable I2C to configure it safely
    // I2C1BRG = (16,000,000 / 100,000) - 1.6 - 1 = 157.4 -> 157
    I2C1BRG = 157; 
    IFS1bits.MI2C1IF = 0;  // Clear Interrupt Flags
    I2C1CONbits.I2CEN = 1; // Enable I2C Module
}

void reset_radio(){
    LATBbits.LATB11 = 1;      // Pull RST high
    __delay_ms(10);           // Hold high for 10ms
    LATBbits.LATB11 = 0;      // Pull RST low
    __delay_ms(10);           // Wait for the chip to settle
}

// Expose the internal library variables so we can read them in main
extern volatile uint8_t DATALEN;
extern volatile uint8_t SENDERID;
extern volatile int16_t RSSI;

// TRANSMIT DATA
// Returns true if the message was delivered and acknowledged
bool transmitMessage(uint8_t targetNode, const void* data, uint16_t size) {
    
    if (sendWithRetry(targetNode, data, size, 3, 40)) {
        return true;  // Delivered successfully
    }
    return false;     // No response from the target
}

// RECEIVE DATA
// Returns true if a new message arrived.
// Extracts the message, the sender's ID, and signal strength.
bool receiveMessage(char* messageBuffer, uint8_t* senderId, int16_t* signalStrength) {
    
    // Check if the INT0 hardware interrupt caught a new packet
    if (receiveDone()) {
        
        memcpy(messageBuffer, (void*)DATA, DATALEN);
        messageBuffer[DATALEN] = '\0'; // Null-terminate to make it a safe string
        
        *senderId = SENDERID;
        *signalStrength = RSSI;

        if (ACKRequested()) {
            sendACK("ACK", 3);
        }
        
        return true;
    }
    
    return false;
}

struct AccelData
{
    float x;
    float y;
    float z;
};


int main(void) {
    CLKDIVbits.RCDIV = 0; 
    
    setup();
    setup_uart();
    spi_init();
    millis_init();
    
    reset_radio();
    rfm69_init(FREQUENCY, MY_NODE_ID, NETWORK_ID);
        
    // object for accelerometer 
    stmdev_ctx_t dev_ctx;
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg  = platform_read;
    dev_ctx.mdelay    = platform_delay;
    dev_ctx.handle    = NULL; 

    uint8_t whoamI;
    lis3dh_device_id_get(&dev_ctx, &whoamI);
    
    if (whoamI != LIS3DH_ID) {
        //raw_print("ERROR: LIS3DH not found!\r\n"); //uart debugging
        while(1) {
            // Blink the LED very fast to signal an error
            LATBbits.LATB5 = ~LATBbits.LATB5; 
            for(long i=0; i<100000; i++); // Dumb delay
        }
    }

    // raw_print("LIS3DH Found! Starting loop...\r\n");

    // Configure the sensor
    lis3dh_block_data_update_set(&dev_ctx, PROPERTY_ENABLE); // output registers are not updated until both MSB and LSB of the sample have been read, preventing the reading of fragmented data between cycles.
    lis3dh_data_rate_set(&dev_ctx, LIS3DH_ODR_100Hz); // samples acceleration data at a frequency of 100 Hz
    lis3dh_operating_mode_set(&dev_ctx, LIS3DH_HR_12bit); // High-Resolution mode, providing 12-bit data precision
    lis3dh_full_scale_set(&dev_ctx, LIS3DH_2g); // sets the full-scale range to ∓2g, providing the highest sensitivity for low-acceleration applications.
    
    bool on = false;
    while(1) {
        LATBbits.LATB5 = 1; // Turn ON
                    
        uint8_t dataready;
        lis3dh_xl_data_ready_get(&dev_ctx, &dataready); //Returns 1 if data is ready to be sampled.
        float pitch_deg=100;
        if (dataready) {
            int16_t data_raw_acceleration[3];
            struct AccelData accel_data;

            lis3dh_acceleration_raw_get(&dev_ctx, data_raw_acceleration);

            accel_data.x = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[0]);
            accel_data.y = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[1]);
            accel_data.z = lis3dh_from_fs2_hr_to_mg(data_raw_acceleration[2]);

            transmitMessage(2, &accel_data, sizeof(accel_data));

             LATBbits.LATB5 = 0; // Turn OFF
        }        
        __delay_ms(10); 
    }
}