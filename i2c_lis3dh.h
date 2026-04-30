/* 
 * File:   i2c_lis3dh.h
 * Author: natha
 *
 * Created on April 13, 2026, 11:55 AM
 */

#ifndef I2C_LIS3DH_H
#define	I2C_LIS3DH_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lis3dh_reg.h" // ST's official LIS3DH driver header

// 7-bit address of LIS3DH is 0x18 (if SDO is tied to GND).
// Shifted left by 1 and appended with R/W bit for PIC24.
#define LIS3DH_I2C_WRITE 0x30  // (0x18 << 1) | 0
#define LIS3DH_I2C_READ  0x31  // (0x18 << 1) | 1

// ST Platform Interface Functions (mapped to stm32dev_ctx_t)
int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len);
int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
void platform_delay(uint32_t millisec);

#ifdef	__cplusplus
}
#endif

#endif	/* I2C_LIS3DH_H */

