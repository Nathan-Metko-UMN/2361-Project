/* 
 * File:   driver_DOGS104.h
 * Author: henry-goldberg
 *
 *IMPORTANT NOTE I TOOK THE VERY NICELY WRITTEN REGSITERS FROM
 * "https://github.com/sstaub/SSD1803A_I2C/tree/main"
 *  
 * printf code is heavily based on this stackoverflow post
 * https://stackoverflow.com/questions/1735236/how-to-write-my-own-printf-in-c
 */

#ifndef DRIVER_DOGS104_H
#define	DRIVER_DOGS104_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>   
    

#define ADDRESS 0b00111100

#define MAX_CUSTOM_CHAR                           8

#define MODE_COMMAND                              0x00
#define MODE_COMMAND_CONT                         0x80
#define MODE_DATA                                 0x40
#define MODE_DATA_CONT                            0xC0

#define COMMAND_CLEAR_DISPLAY                     0x01
#define COMMAND_RETURN_HOME                       0x02
#define COMMAND_ENTRY_MODE_SET                    0x04
#define ENTRY_MODE_LEFT_TO_RIGHT                  0x02
#define ENTRY_MODE_SHIFT_INCREMENT                0x01
#define COMMAND_SHIFT                             0x10
#define COMMAND_DISPLAY_SHIFT_LEFT                0x08
#define COMMAND_DISPLAY_SHIFT_RIGHT               0x0C
#define COMMAND_CURSOR_SHIFT_LEFT                 0x00
#define COMMAND_CURSOR_SHIFT_RIGHT                0x04

#define ADDRESS_CGRAM                             0x40
#define ADDRESS_DDRAM                             0x80
#define ADDRESS_DDRAM_DOGS164_TOP_OFFSET          0x04
#define ADDRESS_DDRAM_DOGS104_TOP_OFFSET          0x0A

#define COMMAND_8BIT_4LINES_RE1_IS0               0x3A
#define COMMAND_8BIT_4LINES_RE0_IS0_DH1           0x3C
#define COMMAND_8BIT_4LINES_RE0_IS1               0x39
#define COMMAND_8BIT_4LINES_RE0_IS1_DH1           0x3D
#define COMMAND_8BIT_4LINES_RE0_IS0               0x38

// Command from extended set (RE = 1, IS = 0)
#define COMMAND_BS1_1                             0x1E
#define COMMAND_POWER_DOWN_DISABLE                0x02
#define COMMAND_TOP_VIEW                          0x05
#define COMMAND_BOTTOM_VIEW                       0x06
#define COMMAND_4LINES                            0x09
#define COMMAND_3LINES_TOP                        0x1F
#define COMMAND_3LINES_MIDDLE                     0x17
#define COMMAND_3LINES_BOTTOM                     0x13
#define COMMAND_2LINES                            0x1B

// Command from extended set (RE = 0, IS = 1)
#define COMMAND_DISPLAY                           0x08
#define COMMAND_DISPLAY_ON                        0x04
#define COMMAND_DISPLAY_OFF                       0x00
#define COMMAND_CURSOR_ON                         0x02
#define COMMAND_CURSOR_OFF                        0x00
#define COMMAND_BLINK_ON                          0x01
#define COMMAND_BLINK_OFF                         0x00

// Command from extended set (RE = 1, IS = 1)
#define COMMAND_SHIFT_SCROLL_ENABLE               0x10
#define COMMAND_SHIFT_SCROLL_ALL_LINES            0x0F
#define COMMAND_SHIFT_SCROLL_LINE_1               0x01
#define COMMAND_SHIFT_SCROLL_LINE_2               0x02
#define COMMAND_SHIFT_SCROLL_LINE_3               0x04
#define COMMAND_SHIFT_SCROLL_LINE_4               0x08


#define COMMAND_BS0_1                             0x1B
#define COMMAND_INTERNAL_DIVIDER                  0x13
#define COMMAND_CONTRAST_DEFAULT_DOGM204          0x72
#define COMMAND_CONTRAST_DEFAULT_DOGS164          0x6B
#define COMMAND_CONTRAST_DEFAULT_DOGS104          0x7A
#define COMMAND_POWER_CONTROL_DOGM204             0x57
#define COMMAND_POWER_CONTROL_DOGS164             0x56
#define COMMAND_POWER_CONTROL_DOGS104             0x56
#define COMMAND_POWER_ICON_CONTRAST               0x5C
#define COMMAND_FOLLOWER_CONTROL_DOGM204          0x6E
#define COMMAND_FOLLOWER_CONTROL_DOGS164          0x6C
#define COMMAND_FOLLOWER_CONTROL_DOGS104          0x6E
#define COMMAND_ROM_SELECT                        0x72
#define COMMAND_ROM_A                             0x00
#define COMMAND_ROM_B                             0x04
#define COMMAND_ROM_C                             0x08

/*
 * enum for error returns
 */
typedef enum {
    DOGS104_OK      = 0,
    DOGS104_ERR_IO  = 1,
    DOGS104_ERR_ARG = 2,
} dogs104_err_t;

/*
 * main structure for initlization
 */
typedef struct dogs104_handle_s
{
    dogs104_err_t (*write)(void *ctx, uint8_t *buf, size_t len);
    void    (*set_rst)(void *ctx, bool high);
    void    (*set_rs)(void *ctx, bool high);
    void    (*delay_ms)(void *ctx, uint32_t ms);
    
    void *ctx;
    uint8_t i2c_addr;
    uint8_t viewangle;
    uint8_t contrast;
} dogs104_handle_t;

// Lifecycle
dogs104_err_t dogs104_standard_init(dogs104_handle_t *handle);
dogs104_err_t dogs104_reset(dogs104_handle_t *handle);
dogs104_err_t dogs104_clear(dogs104_handle_t *handle);

//Write by i2c
dogs104_err_t dogs104_cmd(dogs104_handle_t *handle, uint8_t cmd);
dogs104_err_t dogs104_write_data(dogs104_handle_t *handle, const uint8_t *data, size_t len);

// Cursor
dogs104_err_t dogs104_set_cursor(dogs104_handle_t *handle, uint8_t col, uint8_t row);

// Writing
dogs104_err_t dogs104_puts(dogs104_handle_t *handle, const char *str);
dogs104_err_t dogs104_printf(char *format, ...);
dogs104_err_t dogs104_putchar(dogs104_handle_t *handle, char c);

// Custom characters
dogs104_err_t dogs104_create_char(dogs104_handle_t *handle, uint8_t slot, const uint8_t bitmap[8]);
dogs104_err_t dogs104_write_char(dogs104_handle_t *handle, uint8_t slot);

//view
dogs104_err_t dogs104_viewing_angle(dogs104_handle_t *handle, bool orientation);
dogs104_err_t dogs104_set_display_mode(dogs104_handle_t *handle, bool cursor, bool blink);
dogs104_err_t dogs104_set_constrast(dogs104_handle_t *handle, uint8_t level);


#endif	/* DRIVER_DOGS104_H */

