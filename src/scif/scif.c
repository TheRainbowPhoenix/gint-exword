#include <gint/hardware.h>
#include <gint/drivers.h>
#include <gint/drivers/scif.h>
#include <gint/drivers/states.h>
#include <gint/mpu/power.h>
#include <gint/intc.h>

#define POWER SH7305_POWER
#define SCIF0 SH7305_SCIF

// --- TODO: this is only to test! do not push this! ---
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
// --- TODO: this is only to test! do not push this! ---

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
void scif_setup_async(uint8_t scbrr, int parity, int stop_bits, int bit_count)
{
    // SCSMR.CHR = bit_count == 7 (otherwise assumed 8)
    // SCSMR.STOP = stop_bits == 2 (otherwise assumed 1)
    // SCSMR.PE = parity present? (doesnt seem to care about which type though...)
    POWER.MSTPCR0.SCIF = 0;

    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.RE = 0;

    SCIF0.SCFCR.TFRST = 1;
    SCIF0.SCFCR.RFRST = 1;

    // TODO: allow the user to pick n. We'll assume Pphi/1 (CKE=n=0)
    SCIF0.SCSCR.CKE = 0b00;
    SCIF0.SCSMR.CHR = bit_count == 7;
    SCIF0.SCSMR.STOP = stop_bits == 2;

    // -1 = no parity
    // otherwise, parity depends on the last bit of it.
    SCIF0.SCSMR.PE = parity != -1;
    SCIF0.SCSMR.OE = (parity & 1);
    SCIF0.SCSMR.CA = 0;             // Async

    SCIF0.SCBRR = scbrr;
    sleep_ms( 10);                  // XXX: we should wait for 1-bit, not some arbitrary value.

    SCIF0.SCFCR.RTRG = 0b11;        // Full at 14 bytes
    SCIF0.SCFCR.TTRG = 0b00;        // Empty at 0 bytes (TODO: this is inadequate)
    // what does MCE do????
    SCIF0.SCFCR.TFRST = 0;
    SCIF0.SCFCR.RFRST = 0;

    SCIF0.SCFCR.LOOP = 0;
    SCIF0.SCSCR.TE = 1;
    SCIF0.SCSCR.RE = 0;

    SCIF0.SCSCR.TIE = 1;            // TODO: don't enable TIE right away
    SCIF0.SCSCR.RIE = 0;            // TODO: work on receiving (the LOOP flag sounds useful for testing 
                                    // without an extra calculator)
    
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
