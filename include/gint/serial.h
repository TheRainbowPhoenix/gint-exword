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
#include <gint/defs/timeout.h>

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
    /* Serial port isn't opened */
    SERIAL_ERROR_NOT_OPENED                 = -2,
    /* Transfer size too large */
    SERIAL_ERROR_DATA_SIZE                  = -3,

    /* Incorrect parity detected */
    SERIAL_ERROR_PARITY                     = -4,
    /* Incorrect start/stop-bits found */
    SERIAL_ERROR_FRAMING                    = -5,
    /* Overrun error */
    SERIAL_ERROR_OVERRUN                    = -6,
    /* Read attempt timed out */
    SERIAL_ERROR_TIMEOUT                    = -7
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
   -> Returns 0 on success, or a negative error code otherwise. */
int serial_open(serial_config_t const *config);

/* serial_is_open(): Checks if the serial channel is active */
bool serial_is_open(void);

/* serial_write_async(): Send data asynchronously
   
   This function sends data and returns immediately, with the FIFO being 
   filled in the background when needed. Once finished, the caller is notified
   through a given callback with GINT_CALL, or just ignored with GINT_CALL_NULL.

   @data            Source data
   @size            Source size
   @callback        Callback used to notify the caller 
   -> Returns 0 on success or a negative error code otherwise. */
int serial_write_async(void const *data, size_t size, gint_call_t callback);

/* serial_write_sync(): Send data synchronously
   
   This function behaves like serial_write_async(), except that it shall block 
   until the requested amount of bytes are received. Keep in mind that this 
   function shall freeze if the other device cannot send anything, and that you 
   may rather want to just use serial_write_async() instead.

   @data            Source data
   @size            Source size
   @callback        Callback used to notify the caller */
void serial_write_sync(void const *data, size_t size);

/* serial_read_async(): Receive data asynchronously without any timeout
 *
 * This function reads a specified amount of data from the receive buffer,
 * and returns immediately, reading the rest of the data in the background.
 * When the specified amount of data has been read, the caller shall be 
 * notified through a specified callback function, which may be GINT_CALL_NULL.
 *
 * @data            Destination data
 * @size            Destination size
 * @callback        Callback used to notify the caller 
 * TODO: make this look more like usb_read_async (with an rc parameter) */
int serial_read_async(void *data, size_t size, int *rc, gint_call_t callback);

/* serial_read_sync_timeout(): Receive data synchronously
   
   This functions reads a specified amount data from the receive buffer, waiting 
   if no data is available. If a timeout is specified, then this function will 
   exit early with an error if said timeout expires during the receive.
   If timeout is NULL, then this function behaves like serial_read_sync().

   @data            Destination data
   @size            Destination size
   @timeout         Timeout, can be NULL. 
   -> Returns 0 on success, or a negative error code on failure. */
int serial_read_sync_timeout(void *data, size_t size, timeout_t const *timeout);

/* serial_read_sync(): Receive data synchronously without any timeout
   
   This function behaves like serial_read_sync_timeout(), except that timeouts 
   cannot occur. If no data is available and there is no other party to send 
   data, then this function will freeze. In that case, serial_read_sync_timeout() 
   and serial_read_async() should be used.

   @data            Destination data
   @size            Destination size 
   -> Returns 0 on success, or a negative error code on failure. */
int serial_read_sync(void *data, size_t size);

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
