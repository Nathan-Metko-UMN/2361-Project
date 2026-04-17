/* 
 * File:   main.c
 * Author: natha
 *
 * Created on April 13, 2026, 12:00 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

#include "i2c_lis3dh.h"
#include "lis3dh_reg.h"
#include "uart_lib.h"

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

void setup(){
    CLKDIVbits.RCDIV = 0; 

    AD1PCFG = 0xFFFF;

    // Configure Heartbeat LED RB5 as output
    TRISBbits.TRISB5 = 0;
    LATBbits.LATB5 = 0;

}

int main(void) {
    CLKDIVbits.RCDIV = 0; 
    
    // 1. Initialize basics first
    setup();
    setup_uart();
    
    // 2. TEST UART IMMEDIATELY
    // If you don't see this on your logic analyzer, the UART wiring or baud is wrong.
    raw_print("HELLO WORLD BARE METAL\r\n");

    // 3. Now try the I2C
    i2c_lis3dh_init();
    
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
        
        
        // FIX: Add a physical delay so the LED has time to stay off before the loop repeats
        for(long i=0; i<150000; i++); 
    }
}