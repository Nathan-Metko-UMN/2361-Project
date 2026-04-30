/* 
 * File:   spi_lib.h
 * Author: natha
 *
 * Created on April 17, 2026, 5:00 PM
 */

#ifndef SPI_LIB_H
#define	SPI_LIB_H

#ifdef	__cplusplus
extern "C" {
#endif

void spi_init(void);
uint8_t spi_transfer(uint8_t data);


#ifdef	__cplusplus
}
#endif

#endif	/* SPI_LIB_H */

