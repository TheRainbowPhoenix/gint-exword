#ifndef GINT_MPU_I2C_H
#define GINT_MPU_I2C_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

//---
//	SH7305 I2C Bus Interface. Refer to:
//	  "Renesas SH7724 User's Manual: Hardware"
//	  Section 32: "I2C Bus Interface (I2C)"
//---

/* sh7305_i2c_t - I2C peripheral definition */
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

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_I2C_H */
