//---
// gint:drivers:scif - Low-level SCIF utilities
//
// This driver is used to manage the low-level parts of the SCIF interface
//---

#ifndef GINT_SCIF
#define GINT_SCIF

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/mpu/scif.h>

// TODO: rework this entirely
extern void scif_setup_async(uint8_t scbrr, int parity, int stop_bits, int bit_count);

#ifdef __cplusplus
}
#endif

#endif /* GINT_SCIF */
