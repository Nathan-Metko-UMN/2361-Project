<<<<<<< HEAD
/* 
 * File:   get_millis.c
 * Author: natha
 *
 * Created on April 17, 2026, 5:16 PM
 */

=======
>>>>>>> 26ebaea92124e1c2927101cab310b0e937cf297f
#include <xc.h>
#include "get_millis.h"

// Volatile variable because it is modified inside an Interrupt Service Routine
volatile unsigned long _millis_counter = 0;

void millis_init(void) {
    // 1. Disable Timer1 while we configure it
    T1CONbits.TON = 0;      
    
    // 2. Configure Timer1 settings
    T1CONbits.TCS = 0;      // Use Internal Instruction Clock (FCY)
    T1CONbits.TGATE = 0;    // Disable Gated Timer mode
    T1CONbits.TCKPS = 0;    // 1:1 Prescaler (Timer counts at 4MHz)
    
    // 3. Clear the timer and set the period match value
    TMR1 = 0;               // Reset the counter
    PR1 = 3999;             // 4000 cycles for 1ms (0 to 3999)
    
    // 4. Configure the Timer1 Interrupt
    IFS0bits.T1IF = 0;      // Clear Timer1 Interrupt Flag
    IPC0bits.T1IP = 2;      // Set priority to 2 (Lower than INT0 to not block radio)
    IEC0bits.T1IE = 1;      // Enable Timer1 Interrupt
    
    // 5. Turn Timer1 ON
    T1CONbits.TON = 1;      
}

unsigned long millis(void) {
    unsigned long current_millis;
    
    // CRITICAL FIX: The PIC24 is a 16-bit processor, but 'unsigned long' is 32-bits.
    // It takes the PIC two separate instructions to read this variable. 
    // We must briefly disable the Timer1 interrupt so it doesn't accidentally 
    // increment the counter halfway through our read operation and corrupt the data.
    IEC0bits.T1IE = 0; 
    current_millis = _millis_counter;
    IEC0bits.T1IE = 1;
    
    return current_millis;
}

// Timer1 Interrupt Service Routine
// Fires exactly once every 1 millisecond
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
    _millis_counter++;      // Add 1 millisecond
    IFS0bits.T1IF = 0;      // Clear the interrupt flag so it can fire again
}