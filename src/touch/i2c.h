#ifndef GINT_TOUCH_I2C_H
#define GINT_TOUCH_I2C_H 1

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

//---
// User API
//---

/* i2c_reg_select() - select a register */
extern int i2c_reg_select(int reg);

/* i2c_reg_read() - register read operation */
extern int i2c_reg_read(int reg, void *buffer, size_t size);

/* i2c_read_stream() - "stream" read operation (skip reg selection) */
extern int i2c_read_stream(void *buffer, size_t size);

//---
// Internals
//---

/* i2c_request_mode - enumerate request mode */
enum i2c_request_mode {
    I2C_REQ_MODE_REG_READ      = 0,
    I2C_REQ_MODE_REG_WRITE     = 1,
    I2C_REQ_MODE_REG_SELECT    = 2,
    I2C_REQ_MODE_READ_STREAM   = 3,
};

/* i2c_request_status - request status */
enum i2c_request_status {
    I2C_REQ_STATUS_START        = 0,
    I2C_REQ_STATUS_FINISHED     = 1,
};

enum i2c_request_state {
    I2C_REQ_STATE_START                     = 0,
    I2C_REQ_STATE_REG_SELECT                = 10,
    I2C_REQ_STATE_REG_SELECT_WAIT           = 11,
    I2C_REQ_STATE_SWITCH_TO_RECEIVE         = 20,
    I2C_REQ_STATE_SWITCH_TO_RECEIVE_WAIT    = 21,
    I2C_REQ_STATE_READ                      = 30,
    I2C_REQ_STATE_READ_LAST                 = 31,
    I2C_REQ_STATE_ZOMBIE                    = 667,
    I2C_REQ_STATE_DEAD                      = 2617,
};

/* struct i2c_request_info - internal I2C request information */
struct i2c_request_info {
    enum i2c_request_mode mode;
    int target_register;
    uint8_t *buffer;
    size_t buffer_size;
    size_t buffer_size_remaning;
    size_t buffer_cursor;
    enum i2c_request_status status;
    int state;
};

/* __i2c_request - internal current I2C request */
extern volatile struct i2c_request_info __i2c_request;

//---
// Hardware information
//---

typedef struct {
    // read/write register
    uint8_t ICDR;
    pad(3);

    // control register
    byte_union(ICCR,
        uint8_t ICE     :1;
        uint8_t RACK    :1;
        uint8_t         :1;
        uint8_t TRS     :1;
        uint8_t         :1;
        uint8_t BBSY    :1;
        uint8_t         :1;
        uint8_t SCP     :1;
    );
    pad(3);

    // status register
    byte_union(ICSR,
        uint8_t SCLM    :1;
        uint8_t SDAM    :1;
        uint8_t         :1;
        uint8_t BUSY    :1;
        uint8_t AL      :1;
        uint8_t TACK    :1;
        uint8_t WAIT    :1;
        uint8_t DTE     :1;
    );
    pad(3);

    // interrupt control register
    byte_union(ICIC,
        uint8_t         :1;
        uint8_t         :1;
        uint8_t         :1;
        uint8_t         :1;
        uint8_t ALE     :1;
        uint8_t TACKE   :1;
        uint8_t WAITE   :1;
        uint8_t DTEE    :1;
    );
    pad(3);

    // clock control registers
    uint8_t ICCL;
    pad(3);
    uint8_t ICCH;
} GPACKED(1) sh7305_i2c_t;

#define SH7305_I2C  (*((volatile sh7305_i2c_t *)0xa4470000))

// state and world-switch

#include <gint/drivers/states.h>

/* i2c_configure() - driver/hardware configuration */
extern void i2c_configure(void);

/* i2c_hsave() - save hardware information */
extern void i2c_hsave(touch_state_t *states);

/* i2c_hrestore() - restore hardware information */
extern void i2c_hrestore(touch_state_t *states);

/* i2c_hpowered() - check if the module is powered */
extern bool i2c_hpowered(void);

/* i2c_hpoweron() - power on the module */
extern void i2c_hpoweron(void);

/* i2c_hpoweroff() - power off the module */
extern void i2c_hpoweroff(void);

/* i2c_funbin() - funbind from casio to gint */
extern void i2c_funbin(void);

/* i2c_unbin() - unbind from gint to casio */
extern void i2c_unbin(void);

#endif /* GINT_TOUCH_I2C_H */
