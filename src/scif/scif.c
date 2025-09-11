#include <gint/hardware.h>
#include <gint/drivers.h>
#include <gint/drivers/scif.h>
#include <gint/serial.h>
#include <gint/drivers/states.h>
#include <gint/mpu/power.h>
#include <gint/intc.h>

#define POWER SH7305_POWER
#define SCIF0 SH7305_SCIF

// --- TODO: this is only to test! remove this! ---
static const char *message = 
    "Copy that. You're loud and clear. "
    "It's just who we excepted here. "
    "Now do your job and catch her in a perfect bullet storm.";
static int bytes = 1;
static int counter = 0;
#include <string.h>
static GCONSTRUCTOR void setup_pointless_stuff(void)
{
    bytes = strlen(message);
}

// --- TODO: this is only to test! remove this! ---

static bool flag = false;
static void scif_inth(void)
{
    if (SCIF0.SCFSR.TDFE)
    {
        // TODO: TXI generated...
        // we should most likely have a buffer around that we read from.
        // if it is too empty, then we request the remaining bytes from a 
        // user callback.
        int bytes_max = 16 - SCIF0.SCFDR.TFDC;
        for (int i = 0; i < bytes_max; i++, counter = (counter + 1) % bytes)
            SCIF0.SCFTDR = message[counter];                                // TODO: a pre-baked string 
                                                                            // is an AWFUL idea.
        SCIF0.SCFSR.TDFE = 0;
    }

    // TODO: we need to manage the receive FIFO...
    flag = true;
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

// TODO: function to determine the best SCBRR
#include <gint/clock.h>
static int32_t scale_by_n(int32_t x, int n)
{
    n = 2 * n - 1;
    if (n == 0) return x;
    if (n > 0) return x << n;
    return x >> (-n); // n < 0
}
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
    if (!config) return 0;
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
    SCIF0.SCFCR.TTRG = 0b00;        // Empty at 0 bytes (TODO: this is inadequate)

    SCIF0.SCFCR.TFRST = 0;
    SCIF0.SCFCR.RFRST = 0;

    // XXX: if you need to test the SCIF but don't have another calculator, set this to 1.
    SCIF0.SCFCR.LOOP = 0;

    // We purposefully disable sending/interrupts. We don't need them right now.
    SCIF0.SCSCR.TE = 1;
    SCIF0.SCSCR.RE = 0;
    SCIF0.SCSCR.TIE = 1;
    SCIF0.SCSCR.RIE = 0;
    return 1;
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
