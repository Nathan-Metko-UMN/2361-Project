/* 
 * File:   driver_DOGS104.c
 * Author: henry-goldberg
 *
 * Created on April 29, 2026, 11:29 AM
 */

#include "driver_DOGS104.h"
#include <string.h>

/**
 * @breif   helper function to check if handle has been initlized correctly
 * @param[in] *handle pointer to dogs104 data struct
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t check_handle(dogs104_handle_t *handle)
{
    if (!(handle) || !(handle->write) || !(handle->set_rst) || !(handle->delay_ms))
        return DOGS104_ERR_ARG;
    return DOGS104_OK;
}

/**
 * @breif   helper function to send word in terms of mode and patload
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] ctrl
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t send_byte(dogs104_handle_t *handle, uint8_t ctrl, uint8_t payload)
{
    uint8_t buf[2] = { ctrl, payload };
    return handle->write(handle->ctx, buf, sizeof(buf));
}

/**
 * @breif   main function to send cmds 
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] cmd 
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t dogs104_cmd(dogs104_handle_t *handle, uint8_t cmd)
{
    dogs104_err_t err = check_handle(handle);
    if (err != DOGS104_OK) return err;
    
    return send_byte(handle, MODE_COMMAND, cmd);
}

/*
 * @breif   function to write data
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] *data pointer to data
 * @breif   function to write data@param[in] len of data to avoid string manip by user.
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */

dogs104_err_t dogs104_write_data(dogs104_handle_t *handle, const uint8_t *data, size_t len)
{
    dogs104_err_t err = check_handle(handle);
    if (err != DOGS104_OK) return err;
    if (!data || len == 0) return DOGS104_ERR_ARG;
    
    for (size_t i = 0; i < len; i++) {
        err = send_byte(handle, MODE_DATA, data[i]);
        if (err != DOGS104_OK) return err;
    }
    return DOGS104_OK;
}

/**
 * @breif   function to set cursor in 4 line mode normal oreination
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] rows of display
 * @param[in] cols of display
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
const uint8_t ROW_ADDR[4] = {0x00, 0x20, 0x40, 0x60};

dogs104_err_t dogs104_set_cursor(dogs104_handle_t *handle, uint8_t row, uint8_t col)
{
    if (handle.orientation == 1) {
        col += 0x0A;
    }
    
    if (row >= 4) return DOGS104_ERR_ARG;
    return dogs104_cmd(handle, ADDRESS_DDRAM | (ROW_ADDR[row] + col));
}

/**
 * @breif   function to do simple initlization of ssd1803a
 * @param[in] *handle pointer to dogs104 data struct
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 * @note I kinda got issues with this because it goes againts
 * my initial mentality of this project of very versatile code
 */
dogs104_err_t dogs104_standard_init(dogs104_handle_t *handle)
{
    //Chek if handle has been filled out properly
    dogs104_err_t err = check_handle(handle);
    if (err != DOGS104_OK) return err;

    /*  Hardware reset  */
    handle->set_rst(handle->ctx, false);
    handle->delay_ms(handle->ctx, 10);
    handle->set_rst(handle->ctx, true);
    handle->delay_ms(handle->ctx, 50);   
    
    //Set register extenstion to 1 and instruction set to 0
    err = dogs104_cmd(handle, COMMAND_8BIT_4LINES_RE1_IS0);
    if (err != DOGS104_OK) return err;

    //use four lines (if you would like to use 1, 2, or 3 lines consult data sheet and tear out this initlization)
    err = dogs104_cmd(handle, COMMAND_4LINES);
    if (err != DOGS104_OK) return err;

    //Set mode bottom
    err = dogs104_cmd(handle, COMMAND_BOTTOM_VIEW);
    if (err != DOGS104_OK) return err;

    //Set BS1 (bias bit 1) to 1
    err = dogs104_cmd(handle, COMMAND_BS1_1);
    if (err != DOGS104_OK) return err;
    handle->delay_ms(handle->ctx, 2);

    //8bit mode 4 line instruction set 1 extended regsiter 0
    err = dogs104_cmd(handle, COMMAND_8BIT_4LINES_RE0_IS1);
    if (err != DOGS104_OK) return err;
    
    //Set BS0 (bias bit 0) to 1
    err = dogs104_cmd(handle, COMMAND_BS0_1);
    if (err != DOGS104_OK) return err;
    
    //Follower control settings (look at data sheet)
    err = dogs104_cmd(handle, COMMAND_FOLLOWER_CONTROL_DOGS104);
    if (err != DOGS104_OK) return err;
    
    //Booster on (look at data sheet for setup) contrast)
    err = dogs104_cmd(handle, COMMAND_POWER_CONTROL_DOGS104);
    if (err != DOGS104_OK) return err;
    
    //Set contrast to default (spread out between two cmds (idk why))
    err = dogs104_cmd(handle, COMMAND_CONTRAST_DEFAULT_DOGS104);
    if (err != DOGS104_OK) return err;
    
    //Set register and instruction set 0
    err = dogs104_cmd(handle, COMMAND_8BIT_4LINES_RE0_IS0);
    if (err != DOGS104_OK) return err;
    
    //Display on cursor on, cursor blink off
    err = dogs104_cmd(handle, 0x0E);
    if (err != DOGS104_OK) return err;
    
    return DOGS104_OK;
}

/**
 * @breif   function to spit out strings at cursor on dogs104
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] const char *str pointer
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t dogs104_puts(dogs104_handle_t *handle, const char *str)
{
    if (!str) return DOGS104_ERR_ARG;
    return dogs104_write_data(handle, (const uint8_t *)str, strlen(str));
}

/**
 * @breif   function to spit out chars at cursor on dogs104
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] const char *str pointer
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t dogs104_putchar(dogs104_handle_t *handle, char c)
{
    return send_byte(handle, MODE_DATA, c);
}

/**
 * @breif   function to completley clear dogs104 regs
 * @param[in] *handle pointer to dogs104 data struct
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t dogs104_clear(dogs104_handle_t *handle)
{
    dogs104_err_t err;
    err = check_handle(handle);
    if (err != DOGS104_OK) return err;
    
    err = dogs104_cmd(handle, COMMAND_CLEAR_DISPLAY);
    handle->delay_ms(handle->ctx, 10);
    return err;
}

/**
 * @breif   function to spit out reset ssd1803a/dogs104
 * @param[in] *handle pointer to dogs104 data struct
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t dogs104_reset(dogs104_handle_t *handle)
{
    dogs104_err_t;
    err = check_handle(handle);
    if (err != DOGS104_OK) return err;
    
    handle->set_rst(handle->ctx, false);
    handle->delay_ms(handle->ctx, 10);
    handle->set_rst(handle->ctx, true);
    handle->delay_ms(handle->ctx, 50);
    
    return err;
}

/**
 * @breif   function to set the contrast for the dogs104
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] uint8_t contrast level
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t dogs104_set_constrast(dogs104_handle_t *handle, uint8_t level)
{
    dogs104_err_t err;
    
    handle->contrast = level;
            
    level &= 0b00011111;
    uint8_t msb = 0x5C |= (level >> 3);
    level &= 0x0F;
    uint8_t lsb = 0x70 |= (level);
    
    err = dogs104_cmd(handle, COMMAND_8BIT_4LINES_RE0_IS1);
    if (err != DOGS104_OK) return err;
    
    err = dogs104_cmd(handle, msb);
    if (err != DOGS104_OK) return err;
    
    err= dogs104_cmd(handle, lsb);
    if (err != DOGS104_OK) return err;
    
    return err;
}


/**
 * @breif   Set cursor blink and visability
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] cursor true = cursor on, false = cursor off
 * @param[in] blink true = blink on, false = blink off
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t dogs104_set_display_mode(dogs104_handle_t *handle, bool cursor, bool blink) 
{
    uint8_t mask = 0b00001100;
    mask |= blink;
    mask |= (cursor << 1);
    
    return dogs104_cmd(handle, mask);
}

/**
 * @breif   Set cursor blink and visability
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] orientation true = right side up, false = flipped
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 */
dogs104_err_t dogs104_viewing_angle(dogs104_handle_t *handle, bool orientation)
{
    dogs104_err_t err;
    err = dogs104_cmd(handle, COMMAND_8BIT_4LINES_RE1_IS0); 
    
    if (orientation) {
        err = dogs104_cmd(handle, 0x06); 
    } else {
        err = dogs104_cmd(handle, 0x05); 
    }
    
    err = dogs104_cmd(handle, COMMAND_8BIT_4LINES_RE0_IS0);
    
    handle->viewangle = orientation;
    
    return err;
}

/**
 * @breif   Write custom chars
 * @param[in] *handle pointer to dogs104 data struct
 * @param[in] cursor true = cursor on, false = cursor off
 * @param[in] blink true = blink on, false = blink off
 * @return dogs104_err_t
 *          DOGS104_OK      success
 *          DOGS104_ERR_IO  i2c error
 *          DOGS104_ERR_ARG argument error
 * @notes print chars like so "dogs104_puts(&my_lcd, "\x01")" prints the second custom char
 */
dogs104_err_t dogs104_create_char(dogs104_handle_t *handle, uint8_t slot, const uint8_t *bitmap)
{
    handle->set_rs(handle->ctx, true);
    
    if (slot > 7) return DOGS104_ERR_ARG;
    dogs104_err_t err;
    err = dogs104_cmd(handle, 0x40 | (slot << 3));
    
    for (uint8_t i = 0; i < 8; i++) {
        err =dogs104_write_data(handle, &bitmap[i], 1);
        if (err != DOGS104_OK) return err;
    }
    
    handle->set_rs(handle->ctx, false);
    
    return dogs104_cmd(handle, 0x80);
}

/**
 * @breif   Helper function to transform base of number
 * @param[in] num num you want to convert
 * @param[in] num_base up to hex
 * @return char * in form of a string
 */
char *convert(unsigned int num, int num_base)
{
    static char Representation[] = "0123456789ABCDEF";
    static char buffer[50];
    char *ptr;
    
    do
    {
        *--ptr = Representation[num % num_base];
        num /= num_base;
    } while (num != 0);
    
    return ptr;
}

/**
 * @breif   Printf equivelant for dogs104
 * @param[in] char* format
 * @return dogs104_err_t
 * 
 * @notes works like printf types are %c, %d, %o, %s, and %x
 */
dogs104_err_t dogs104_printf(char *format, ...)
{
    char *traverse;
    uint16_t i;
    char *s;
    
    va_list arg;
    va_start(arg, format);
    
    for (traverse = format; *traverse !- '\0'; traverse++)
    {
        while (*traverse != '%') {
            dogs104_putchar(handle, *traverse)
            traverse++;
        }
        
        traverse++;
        
        switch (*traverse) {
            case 'c' : i = va_arg(arg,int);
                dogs104_putchar(handle, i);
                break;
                
            case 'd' : i = va_arg(arg, int);
                if (i < 0) {
                    i = -i;
                    dogs104_putchar(handle, '-');
                }
                dogs104_prints(handle, convert(i, 10));
                break;
            
            case 'o' : i = va_arg(arg, unsigned int);
                dogs104_prints(handle, convert(i, 8));
                break;
                
            case 's' : s = va_arg(arg, char *);
                dogs104_prints(handle, s);
            
            case 'x' : i = va_arg(arg, unsigned int);
                dogs104_prints(handle, convert(i, 16));
                break;
        }
    }
    
    va_end(arg);
}
