/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef ACCEL_H
#define ACCEL_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    typedef struct {
        int16_t x_mg;
        int16_t y_mg;
        int16_t z_mg;
    } AccelData;

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* ACCEL_H */

