/* 
 * File:   uart_lib.h
 * Author: natha
 *
 * Created on April 16, 2026, 11:38 AM
 */

#ifndef UART_LIB_H
#define	UART_LIB_H

#ifdef	__cplusplus
extern "C" {
#endif

void putch(unsigned char c);
void raw_print(char* str);
unsigned char PGetch(void);
void setup_uart(void);

#ifdef	__cplusplus
}
#endif

#endif	/* UART_LIB_H */

