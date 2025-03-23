//---
// gint:touch:i2c_inth - I2C interrupt handlers
//---
#include <gint/exc.h>

#include "./i2c.h"

//---
// Private
//---

/* _i2c_inth_read() - generic read byte operation */
static int _i2c_inth_io_operation(bool read)
{
    if (read) {
        __i2c_request.buffer[
            __i2c_request.buffer_cursor
        ] = SH7305_I2C.ICDR;
    } else {
        SH7305_I2C.ICDR = __i2c_request.buffer[
            __i2c_request.buffer_cursor
        ];
    }
    __i2c_request.buffer_cursor += 1;
    __i2c_request.buffer_size_remaning -= 1;
    return __i2c_request.buffer_size_remaning;
}

/* _i2c_inth_stop() - stop I2C module (with force wait) */
static void _i2c_inth_stop(void)
{
    while(true) {
        if (SH7305_I2C.ICSR.BUSY == 0)
            break;
        SH7305_I2C.ICDR = SH7305_I2C.ICDR;
    }
    SH7305_I2C.ICIC.byte    = 0x00;
    SH7305_I2C.ICSR.byte    = 0x00;
    SH7305_I2C.ICCR.byte    = 0x00;
    __i2c_request.state     = I2C_REQ_STATE_DEAD;
    __i2c_request.status    = I2C_REQ_STATUS_FINISHED;
}

//---
// Public
//---

/* i2c_inth_trans() - TRANS interrupt handler */
void i2c_inth_trans(void)
{
    switch (__i2c_request.state)
    {
        // start byte (writing or reading) and select the proper mode
        // note that the "read stream" just changes the internal state
        // because the starting byte (0x81) is force-sent over there
        case I2C_REQ_STATE_START:
            SH7305_I2C.ICIC.DTEE    = 0;
            SH7305_I2C.ICIC.WAITE   = 1;
            if (__i2c_request.mode == I2C_REQ_MODE_READ_STREAM) {
                SH7305_I2C.ICDR     = 0x81;
                __i2c_request.state = I2C_REQ_STATE_SWITCH_TO_RECEIVE_WAIT;
            } else {
                SH7305_I2C.ICDR     = 0x80;
                __i2c_request.state = I2C_REQ_STATE_REG_SELECT;
            }
            break;

        // switch the I2C module to receive mode and setup the next state
        // use WAIT interrupt to really switch the I2C to receive
        case I2C_REQ_STATE_SWITCH_TO_RECEIVE:
            SH7305_I2C.ICIC.DTEE    = 0;
            SH7305_I2C.ICDR         = 0x81;
            __i2c_request.state     = I2C_REQ_STATE_SWITCH_TO_RECEIVE_WAIT;
            break;

        // for the read operation, the only thing to do is to send the
        // ACK at the end of the operation
        case I2C_REQ_STATE_READ:
            SH7305_I2C.ICIC.DTEE    = 0;
            if (_i2c_inth_io_operation(true) == 0)
                _i2c_inth_stop();
            break;

        // error, unsupported sequences display tests/debug information
        default:
            gint_panic(0x10e0);
    }
}

/* i2c_inth_wait() - WAIT interrupt handler */
void i2c_inth_wait(void)
{
    switch (__i2c_request.state)
    {
        // indicate which register to perform the operation
        // we will wait the next WAIT interrupt to ensure that the data has
        // been sent
        case I2C_REQ_STATE_REG_SELECT:
            SH7305_I2C.ICDR         = __i2c_request.target_register;
            __i2c_request.state     = I2C_REQ_STATE_REG_SELECT_WAIT;
            SH7305_I2C.ICSR.WAIT    = 0;
            break;

        // the selected register is confirmed, stop or restart
        // note that if the "select register" request is performed (or if
        // no buffer is provided) then we send the stop condition (0x90)
        // and force-stop the module.
        // Otherwise, we trigger a new request (0x94) to (after
        // the restart interruption) switch manually from "reading" or
        // "writing" mode
        case I2C_REQ_STATE_REG_SELECT_WAIT:
            if (
                   __i2c_request.mode == I2C_REQ_MODE_REG_SELECT
                || __i2c_request.buffer_size_remaning == 0
                || __i2c_request.buffer == NULL
            ) {
                SH7305_I2C.ICCR.byte    = 0x90;
                SH7305_I2C.ICSR.WAIT    = 0;
                _i2c_inth_stop();
            } else {
                SH7305_I2C.ICCR.byte    = 0x94;
                __i2c_request.state     = I2C_REQ_STATE_SWITCH_TO_RECEIVE;
                SH7305_I2C.ICIC.DTEE    = 1;
                SH7305_I2C.ICSR.WAIT    = 0;
            }
            break;

        // switch to receive mode
        // waiting the next WAIT interrupt to start reading
        case I2C_REQ_STATE_SWITCH_TO_RECEIVE_WAIT:
            SH7305_I2C.ICCR.byte    = 0x81;
            __i2c_request.state     = I2C_REQ_STATE_READ;
            SH7305_I2C.ICSR.WAIT    = 0;
            break;

        // read operation
        // can either be WAIT or DTE interrupt, a special case is
        // performed to avoid DTE interrupt
        case I2C_REQ_STATE_READ:
            if (__i2c_request.buffer_size_remaning > 1) {
                if (SH7305_I2C.ICSR.DTE != 0)
                    _i2c_inth_io_operation(true);
            }
            if (__i2c_request.buffer_size_remaning == 1)
                SH7305_I2C.ICCR.byte = 0xc0;
            SH7305_I2C.ICIC.DTEE = 1;
            SH7305_I2C.ICSR.WAIT = 0;
            break;

        // error cases, unable to handle the WAIT interrupt
        default:
            gint_panic(0x10e0);
    }
}

/* i2c_inth_tack() - TACK interrupt handler */
void i2c_inth_tack(void)
{
    gint_panic(0x10e0);
}

/* i2c_inth_al() - AL interrupt handler */
void i2c_inth_al(void)
{
    gint_panic(0x10e0);
}
