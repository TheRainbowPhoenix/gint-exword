#include <gint/hardware.h>
#include <gint/drivers.h>
#include <gint/drivers/scif.h>
#include <gint/serial.h>
#include <gint/drivers/states.h>
#include <gint/mpu/power.h>
#include <gint/intc.h>
#include <gint/exc.h>

#include <string.h>

#define BUFFER_SIZE             4096
// Used to enable loopback testing
#define SELF_TESTING

#define POWER SH7305_POWER
#define SCIF0 SH7305_SCIF

static bool opened = false;

static bool writing = false;
static gint_call_t tx_callback = GINT_CALL_NULL;
static uint8_t tx_buffer[BUFFER_SIZE];
static size_t tx_index = 0, tx_count = 0;

// NOTE: this is a ring buffer.
uint8_t rx_buffer[BUFFER_SIZE];
static size_t rx_index = 0;

#include <gint/display.h>
static void scif_inth(void)
{
    if (SCIF0.SCFSR.TDFE && tx_count != 0)
    {
        // TODO: TXI generated...
        // we should most likely have a buffer around that we read from.
        // if it is too empty, then we request the remaining bytes from a 
        // user callback.
        size_t bytes_max = 16 - SCIF0.SCFDR.TFDC;
        for (size_t i = 0; i < bytes_max && tx_index < tx_count; i++)
            SCIF0.SCFTDR = tx_buffer[tx_index++];
        SCIF0.SCFSR.TDFE = 0;
        if (tx_index == tx_count)
        {
            // We're out of data.
            gint_call(tx_callback);
            tx_callback = GINT_CALL_NULL;
            tx_index = 0;
            tx_count = 0;
            writing = false;
            SCIF0.SCSCR.TE = 0;
            SCIF0.SCSCR.TIE = 0;
        }
    }

    if (SCIF0.SCFSR.RDF)
    {
        size_t bytes_max = SCIF0.SCFDR.RFDC;
        for (size_t i = 0; i < bytes_max; i++)
        {
            rx_buffer[rx_index] = SCIF0.SCFRDR;
            rx_index = (rx_index + 1) % BUFFER_SIZE;
        }

        // TODO: it looks like we ARE receiving bytes (which is INCREDIBLE!), but 
        // the current driver seems to NOT want to do anything else after this :(
        // I'll work on this later, this is basically a success.
        dclear(C_WHITE);
        dprint(1,1,C_BLACK,"'%s'", rx_buffer);
        dupdate();
        SCIF0.SCFSR.RDF = 0;
    }
}

static void configure(void)
{
	if(isSH3()) return;
    intc_handler_function(0xC00, GINT_CALL(scif_inth));
    intc_priority(INTC_SCIF0, 1);
}

static bool check_power(void)
{
	if(isSH3()) return false;
    return POWER.MSTPCR0.SCIF == 0;
}
static void poweroff(void)
{
	if(isSH3()) return;
    // TODO: we may have to do more with the SCIF, 
    // but that sounds like a fine way to save everything
    POWER.MSTPCR0.SCIF = 1;
}
static void poweron(void)
{
	if(isSH3()) return;
    POWER.MSTPCR0.SCIF = 0;
}

static void save_ctx(scif_state_t *state)
{
	if(isSH3()) return;
    state->SCBRR = SCIF0.SCBRR; 
    state->SCSCR = SCIF0.SCSCR.word;
    state->SCSMR = SCIF0.SCSMR.word;
    // TODO: do we need to save more?
}
static void restore_ctx(const scif_state_t *state)
{
	if(isSH3()) return;
    SCIF0.SCBRR = state->SCBRR; 
    SCIF0.SCSCR.word = state->SCSCR;
    SCIF0.SCSMR.word = state->SCSMR;
}

static void bind(void)
{
	if(isSH3()) return;
    // TODO: do we need to bind in gint-world?
}
static void unbind(void)
{
	if(isSH3()) return;
    // TODO: _wait_ until the FIFO is entirely empty
    // (busy-loop while watching hardware registers?)
}
static void funbind(void)
{
	if(isSH3()) return;
    // TODO: wait until the OS has nothing to send (are there even scenarios where 
    // a gint addin runs and yet the OS serial still wants to mingle?), by busy-looping
}

//--- 
// "Public" SCIF functions
//---

#include <gint/clock.h>
// scale_by_n: Applies some scaling to x depending on the value of n(clock divider)
static int32_t scale_by_n(int32_t x, int n)
{
    n = 2 * n - 1;
    if (n == 0) return x;
    if (n > 0) return x << n;
    return x >> (-n); // n < 0
}
// scif_best_scbrr: Finds the best value for SCBRR and n
bool scif_best_scbrr(uint32_t baudrate, uint8_t *n, uint8_t *scbrr)
{
    const clock_frequency_t *info = clock_freq();
    // TODO: note that this is async only...
    // sync uses a different formula

    int32_t best_deviation = INT32_MAX;
    uint8_t best_scbrr = 0xFF;
    uint8_t best_n = 0;
    for (int potential_n = 0; potential_n < 4; potential_n++)
    {
        // Figure out the value of SCBRR
        // The multiplication by 1M is already implied by the fact that Pphi_f is in Hz, not MHz.
        int32_t potential_scbrr = (info->Pphi_f / scale_by_n(64 * baudrate, potential_n)) - 1;
        int32_t potential_deviation = (info->Pphi_f % scale_by_n(64 * baudrate, potential_n));

        // This value of SCBRR is completely invalid.
        if (potential_scbrr < 0 || potential_n >= 255)
            continue;

        // Check the amount of "error" by looking at the remainder.
        if (potential_deviation < best_deviation)
        {
            best_deviation = potential_deviation;
            best_scbrr = potential_scbrr;
            best_n = potential_n;
        }
    }
    if (best_deviation != INT32_MAX && n) *n = best_n;
    if (best_deviation != INT32_MAX && scbrr) *scbrr = best_scbrr;
    return best_deviation != INT32_MAX; // TODO: change this!
}

int serial_open(serial_config_t const *config)
{
    // TODO: have a routine to determine this value
    uint8_t best_n = 0b00, best_scbrr = 95;
    if (!config || serial_is_open()) return 0;
    if (config->parity > SERIAL_PARITY_ODD)
        return 0;
    if (config->stop_bits != 1 && config->stop_bits != 2)
        return 0;
    if (config->data_width != 7 && config->data_width != 8)
        return 0;

    if (!scif_best_scbrr(config->baudrate, &best_n, &best_scbrr))
        return 0;

    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.RE = 0;

    SCIF0.SCFCR.TFRST = 1;
    SCIF0.SCFCR.RFRST = 1;

    // TODO: custom N/CKE value
    SCIF0.SCSCR.CKE = best_n;
    SCIF0.SCSMR.CHR = config->stop_bits == 7;
    SCIF0.SCSMR.STOP = config->stop_bits == 2;

    SCIF0.SCSMR.PE = config->parity != SERIAL_PARITY_NONE;
    SCIF0.SCSMR.OE = !!(config->parity - 1);
    SCIF0.SCSMR.CA = 0;
    SCIF0.SCBRR = best_scbrr;

    sleep_ms( 10);              // TODO: this isn't quite correct. we should wait for one bit.
                                // ig you could compute its length on the spot?
    
    SCIF0.SCFCR.RTRG = 0b11;        // Full at 14 bytes
    SCIF0.SCFCR.TTRG = 0b00;        // Empty at 8 bytes

    SCIF0.SCFCR.TFRST = 0;
    SCIF0.SCFCR.RFRST = 0;

#ifdef SELF_TESTING
    SCIF0.SCFCR.LOOP = 1;
#else
    SCIF0.SCFCR.LOOP = 0;
#endif

    // We purposefully disable sending/interrupts. We don't need them right now.
    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.TIE = 0;

    SCIF0.SCSCR.RIE = 1;
    SCIF0.SCSCR.RE = 1;

    opened = true;
    return 1;
}

#include <gint/display.h>
#include <gint/keyboard.h>
void serial_wait(void)
{
    // TODO: manage comms happening in async
    // Right now, I'm only going to busy-loop until TDFC is empty...
    while (writing) sleep();        // If we're writing here, it must be an async transfer!
    while (SCIF0.SCFDR.TFDC != 0);  // TODO: we should probably sleep
}
bool serial_is_open(void)
{
    return opened;
}
void serial_close(void)
{
    if (!serial_is_open()) return;
    opened = false;
    serial_wait();
    // TODO: disable TE/RE/TIE/RIE
}
void serial_write_sync(void const *data, size_t size)
{
    uint8_t const *bytes = data;

    if (!bytes || !serial_is_open()) return;
    serial_wait();

    // TODO: maybe this warrants atomic execution
    // (maybe a timer might just race condition its way into starting an async 
    // transfer right as we're doing a sync one?)
    writing = true;
    SCIF0.SCSCR.TE = 1;
    SCIF0.SCSCR.TIE = 0;                        // TX interrupts are not needed!
    for (size_t i = 0; i < size; i++)
    {
        while (SCIF0.SCFSR.TDFE != 1);
        SCIF0.SCFTDR = bytes[i];
        SCIF0.SCFSR.TDFE = SCIF0.SCFSR.TEND = 0;
    }
    while (SCIF0.SCFSR.TEND != 1);
    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.TIE = 0;
    // TODO: should we really disable TE?
    writing = false;
}
void serial_write_async(void const *data, size_t size, gint_call_t cb)
{
    uint8_t const *bytes = data;

    if (!bytes || !serial_is_open()) return;
    if (size >= BUFFER_SIZE) return;            // TODO: return an error...
    serial_wait();

    // TODO: should we have a cap? i feel like we need to copy data elsewhere, 
    // since it could be modified...
    memcpy(tx_buffer, data, size);
    tx_count = 0;
    tx_count = size;
    tx_callback = cb;

    // Go!
    writing = true;
    SCIF0.SCSCR.TE = 1;
    SCIF0.SCSCR.TIE = 1;
}

// TODO: the driver messes some things up REAL baaaad...
gint_driver_t drv_scif = {
    .name           = "SCIF",
    .configure      = configure,

    // Used when working in gint-world
    .bind           = bind,
    .unbind         = unbind,

    // Used when dealing with "foreign" code (the OS)
    .funbind        = funbind,

    .hpowered       = check_power,
    .hpoweroff      = poweroff,
    .hpoweron       = poweron,

    // TODO: hsave + hrestore
    .hsave          = (void *)save_ctx,
    .hrestore       = (void *)restore_ctx,
    .state_size     = sizeof(scif_state_t)
};
GINT_DECLARE_DRIVER(16, drv_scif);
