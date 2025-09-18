//---
// gint:serial - Serial communication
//---

#ifndef GINT_SERIAL
#define GINT_SERIAL

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>
#include <gint/defs/call.h>

typedef enum {
	SERIAL_PARITY_NONE,
	SERIAL_PARITY_EVEN,
	SERIAL_PARITY_ODD,

} GPACKEDENUM serial_parity_t;

typedef struct {
	/* Baud rate (valid options are 300, 600, 1200, 2400, 4800, 9600,
	   19200, 38400, 57600 and 115200) */
	int baudrate;
	/* Parity */
	serial_parity_t parity;
	/* Data width (valid options are 7 and 8) */
	uint8_t data_width;
	/* Stop bits (valid options are 1 and 2) */
	uint8_t stop_bits;

} serial_config_t;

// TODO: Error management...

/* Error codes for serial functions */
enum {
    /* Inexistant/Impossible parameters (e.g: data_width not being 7 or 8 bits) */
    SERIAL_ERROR_INVALID_PARAMS             = -1,
    /* Transfer size too large */
    SERIAL_ERROR_DATA_SIZE                  = -2,

    /* Incorrect parity detected */
    SERIAL_ERROR_PARITY                     = -3,
    /* Incorrect start/stop-bits found */
    SERIAL_ERROR_FRAMING                    = -4,
    /* Overrun error */
    SERIAL_ERROR_OVERRUN                    = -5,
};

/* serial_open(): Initialise the serial driver

   Opens the serial communication with the given parameters. This function 
   will fail with a zero if the parameters given aren't valid values.

   No transmittions actually start once this function returns, but it will 
   listen to receptions.

   @baudrate        The bitrate at which communication should be happening.
   @parity          Parity configuration
   @data_width      Size taken by each 'character'. If 7, then the MSB will 
                    be ignored.
   @stop_bits       Number of stop bits at the end of a frame.
   -> Returns 1 on success. */
int serial_open(serial_config_t const *config);

/* serial_is_open(): Checks if the serial channel is active */
bool serial_is_open(void);

/* serial_write_async(): Send data asynchronously
   
   This function sends data and returns immediately, with the FIFO being 
   filled in the background when needed. Once finished, the caller is notified
   through a given callback with GINT_CALL, or just ignored with GINT_CALL_NULL.

   @data            Source data
   @size            Source size
   @callback        Callback used to notify the caller */
void serial_write_async(void const *data, size_t size, gint_call_t callback);

/* serial_write_sync(): Send data synchronously
   
   This function behaves like serial_write_async(), except that it shall block 
   until the requested amount of bytes are received. Keep in mind that this 
   function shall freeze if the other device cannot send anything, and that you 
   may rather want to just use serial_write_async() instead.

   @data            Source data
   @size            Source size
   @callback        Callback used to notify the caller */
void serial_write_sync(void const *data, size_t size);

void serial_read_async(void *data, size_t size, gint_call_t callback);

void serial_read_sync(void *data, size_t size);

// Waits for communications to finish
void serial_wait(void);

// Calls serial_wait() automatically
void serial_close(void);

// TODO: Info on how much data is pending in each buffer?
// Ultimately we'll bind this to a file descriptor so we can't really know.

#ifdef __cplusplus
}
#endif

#endif /* GINT_SERIAL */
