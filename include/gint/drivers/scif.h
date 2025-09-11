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

// Writes the best SCBRR/CKS for the specified baudrate.
extern bool scif_best_scbrr(uint32_t baudrate, uint8_t *n, uint8_t *scbrr);

#ifdef __cplusplus
}
#endif

#endif /* GINT_SCIF */
