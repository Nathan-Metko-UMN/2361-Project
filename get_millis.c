/* 
 * File:   get_millis.c
 * Author: natha
 *
 * Created on April 17, 2026, 5:16 PM
 */

#include <xc.h>
#include "get_millis.h"

// Volatile variable because it is modified inside an Interrupt Service Routine
volatile unsigned long _millis_counter = 0;

void millis_init(void) {
    T1CONbits.TON = 0;      
    
    T1CONbits.TCS = 0;     
    T1CONbits.TGATE = 0;    
    T1CONbits.TCKPS = 0;    
    
    TMR1 = 0;
    PR1 = 15999;
    
    IFS0bits.T1IF = 0;
    IPC0bits.T1IP = 2;      // Set priority to 2 (Lower than INT0 to not block radio)
    IEC0bits.T1IE = 1;
    
    T1CONbits.TON = 1;      
}

unsigned long millis(void) {
    unsigned long current_millis;
    
    IEC0bits.T1IE = 0; 
    current_millis = _millis_counter;
    IEC0bits.T1IE = 1;
    
    return current_millis;
}

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {
    _millis_counter++;
    IFS0bits.T1IF = 0;
}