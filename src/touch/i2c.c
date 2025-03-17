//---
// gint:touchscreen:i2c - I2C driver
//---
#include <gint/defs/attributes.h>
#include <gint/defs/types.h>
#include <gint/defs/call.h>
#include <gint/cpu.h>
#include <gint/intc.h>

#include "./i2c.h"

//---
// Internals
//---

/* __i2c_request - internal I2C request information */
volatile struct i2c_request_info __i2c_request;

/* i2c_hw_enable() - enable the I2C peripheral and configure the clock
 *
 * notes
 *   Clock configuration are the same used by Casio. Investigation must be
 *   made to ensure overclock/underclock validity */
static void _i2c_hw_enable(void)
{
    SH7305_I2C.ICCR.ICE = 1;
    while(true) {
        if (SH7305_I2C.ICSR.BUSY == 0)
            break;
    }
    SH7305_I2C.ICCL = 0x29;
    SH7305_I2C.ICCH = 0x22;
}

/* i2c_hw_start_operation() - start I2C operation
 *
 * notes
 *   Enable only data transmit and arbitration lost interrupt then perform
 *   a start condition (0x94) */
static void _i2c_hw_start_operation(void)
{
    SH7305_I2C.ICIC.ALE     = 1;
    SH7305_I2C.ICIC.TACKE   = 1;
    SH7305_I2C.ICIC.WAITE   = 0;
    SH7305_I2C.ICIC.DTEE    = 1;

    __auto_type iccr = SH7305_I2C.ICCR;
    iccr.ICE     = 1;
    iccr.RACK    = 0;
    iccr.TRS     = 1;
    iccr.BBSY    = 1;
    iccr.SCP     = 0;
    SH7305_I2C.ICCR = iccr;
}

/* i2c_request_await() - await async operation */
static int _i2c_request_await(void)
{
    int status;

    while (true)
    {
        cpu_atomic_start();
        status = (__i2c_request.status == I2C_REQ_STATUS_FINISHED);
        cpu_atomic_end();
        if (status)
            break;
        __asm__ volatile ("sleep");
    }
    return 0;
}

//---
// Public
//---

/* i2c_power_on() - power on the I2C peripheral and install handlers */
void i2c_power_on(void)
{
    volatile uint16_t *INTC_IPRH    = (void*)0xa408001c;
    volatile uint8_t *INTC_IMCR7    = (void*)0xa40800dc;
    volatile uint8_t *INTC_IMR7     = (void*)0xa408009c;
    volatile uint16_t *PFC_PJCR     = (void*)0xa4050110;
    volatile uint32_t *MSTPCR2      = (void*)0xa4150038;
    extern void i2c_inth_tack(void);
    extern void i2c_inth_wait(void);
    extern void i2c_inth_trans(void);
    extern void i2c_inth_al(void);

    // force-mask I2C interrupts
    *INTC_IPRH &= 0xfff0;
    *INTC_IMR7 |= 0xf0;

    // install I2C handler
    intc_handler_function(0xe00, GINT_CALL(i2c_inth_al));
    intc_handler_function(0xe20, GINT_CALL(i2c_inth_tack));
    intc_handler_function(0xe40, GINT_CALL(i2c_inth_wait));
    intc_handler_function(0xe60, GINT_CALL(i2c_inth_trans));

    // power-on I2C module
    *MSTPCR2 &= 0xfffff7ff;

    // configure I2C PIN
    *PFC_PJCR &= 0xf0ff;

    // configure I2C module
    SH7305_I2C.ICCR.byte &= 0x7f;
    SH7305_I2C.ICSR.byte = 0x00;
    SH7305_I2C.ICIC.byte = 0x00;

    // Enable interrupt
    *INTC_IPRH |= 1;
    *INTC_IMCR7 |= 0xf0;
}

// user API

/* i2c_reg_select() - select a register */
int i2c_reg_select(int reg)
{
    cpu_atomic_start();
    _i2c_hw_enable();

    __i2c_request.mode                  = I2C_REQ_MODE_REG_SELECT;
    __i2c_request.target_register       = reg;
    __i2c_request.buffer                = NULL;
    __i2c_request.buffer_size           = 0;
    __i2c_request.buffer_size_remaning  = 0;
    __i2c_request.buffer_cursor         = 0;
    __i2c_request.status                = I2C_REQ_STATUS_START;
    __i2c_request.state                 = I2C_REQ_STATE_START;

    cpu_atomic_end();
    _i2c_hw_start_operation();
    return _i2c_request_await();
}

/* i2c_reg_read() - register read operation */
int i2c_reg_read(int reg, void *buffer, size_t size)
{
    if (buffer == NULL)
        return -1;
    if (size == 0)
        return -2;

    cpu_atomic_start();
    _i2c_hw_enable();

    __i2c_request.mode                  = I2C_REQ_MODE_REG_READ;
    __i2c_request.target_register       = reg;
    __i2c_request.buffer                = buffer;
    __i2c_request.buffer_size           = size;
    __i2c_request.buffer_size_remaning  = size;
    __i2c_request.buffer_cursor         = 0;
    __i2c_request.status                = I2C_REQ_STATUS_START;
    __i2c_request.state                 = I2C_REQ_STATE_START;

    cpu_atomic_end();
    _i2c_hw_start_operation();
    return _i2c_request_await();
}

/* i2c_read_stream() - "stream" read operation (skip reg selection) */
int i2c_read_stream(void *buffer, size_t size)
{
    if (buffer == NULL)
        return -1;
    if (size == 0)
        return -2;

    cpu_atomic_start();
    _i2c_hw_enable();

    __i2c_request.mode                  = I2C_REQ_MODE_READ_STREAM;
    __i2c_request.target_register       = 0x00;
    __i2c_request.buffer                = buffer;
    __i2c_request.buffer_size           = size;
    __i2c_request.buffer_size_remaning  = size;
    __i2c_request.buffer_cursor         = 0;
    __i2c_request.status                = I2C_REQ_STATUS_START;
    __i2c_request.state                 = I2C_REQ_STATE_START;

    cpu_atomic_end();
    _i2c_hw_start_operation();
    return _i2c_request_await();
}
