//---
// gint:mpu:keysc - Key Scan Controller
//---

#ifndef GINT_MPU_KEYSC
#define GINT_MPU_KEYSC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

typedef volatile struct {
    uint16_t KIUDATA[6];
    uint16_t KIUCNTREG;
    uint16_t KIAUTOFIXREG;
    uint16_t KIUMODEREG;
    uint16_t KIUSTATEREG;
    uint16_t KIUINTREG;
    uint16_t KIUWSETREG;
    uint16_t KIUINTERVALREG;
    uint16_t KOUTPINSET;
    uint16_t KINPINSET;
} GPACKED(4) sh7305_keysc_t;

#define SH7305_KEYSC (*(sh7305_keysc_t *)0xa44b0000)

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_KEYSC */
