//---
//	gint:serial - Serial operation
//---

#ifndef GINT_SERIAL
#define GINT_SERIAL

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/mpu/scif.h>
#include <gint/mpu/pfc.h>
#include <gint/hardware.h>
#include <gint/defs/types.h>
#include <gint/defs/call.h>


// Prototypes of functions expected to appear in the serial module
// aims at reproducing the OS syscalls

int gint_serial_read_one_byte(unsigned char *dest);

int gint_serial_write_one_byte(unsigned char byte);

int gint_serial_write_bytes(unsigned char *src, int size);

int gint_serial_clear_receive_buffer(void);

int gint_serial_clear_transmit_buffer(void);

int gint_serial_open(unsigned char *conf);

int gint_serial_close(int mode);


#ifdef __cplusplus
}
#endif

#endif /* GINT_SERIAL */