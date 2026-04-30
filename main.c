#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <xc.h>

#include "spi_lib.h"
#include "RFM69.h"

#ifndef FCY
#define FCY 16000000UL
#endif
#include <libpic30.h>

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
#define MY_NODE_ID    2    // The ID of this specific radio
#define TARGET_ID     1    // The ID of the radio you want to talk to
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
bool transmitMessage(uint8_t targetNode, const char* message) {
    
    if (sendWithRetry(targetNode, message, strlen(message), 3, 40)) {
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

        // Automatically reply if the sender asked for a read receipt (ACK)
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
    spi_init();
    millis_init();
    reset_radio();
    rfm69_init(FREQUENCY, MY_NODE_ID, NETWORK_ID);
                
    while(1) {
        char rx_buffer[50]; 
        uint8_t senderId;
        int16_t signalStrength;

        struct AccelData accel_data;
        bool recived = receiveMessage(&accel_data,&senderId,&signalStrength);
        if(recived){
            LATBbits.LATB5 = (rx_buffer[0] == '1') ? 1 : 0; // Turn OFF
        }
        for(long i=0; i<150000; i++);
    }
}