#include <gint/hardware.h>
#include <gint/drivers.h>
#include <gint/drivers/scif.h>
#include <gint/serial.h>
#include <gint/drivers/states.h>
#include <gint/mpu/power.h>
#include <gint/intc.h>

#include <string.h>

#define BUFFER_SIZE             1024
// Used to enable loopback testing
#define SELF_TESTING

#define POWER SH7305_POWER
#define SCIF0 SH7305_SCIF

static bool opened = false;

typedef struct 
{
    uint8_t buffer[BUFFER_SIZE];
    size_t writehead, readhead;
} ringbuffer_t;

// TODO: this should be replaced with a more robust mechanism (and also just checking TE itself...)
static bool writing = false;

static ringbuffer_t tx_ring = { .readhead = 0, .writehead = 0 };
static gint_call_t tx_callback = GINT_CALL_NULL;

static ringbuffer_t rx_ring = { .readhead = 0, .writehead = 0 };
static gint_call_t rx_callback = GINT_CALL_NULL;
static size_t rx_needed = 0;
static void *rx_dest = NULL;

//---
// Ring buffer implementation
//---
static bool ring_push(ringbuffer_t *ring, uint8_t recv)
{
    size_t next = (ring->writehead + 1) % BUFFER_SIZE;
    if (next == ring->readhead)
    {
        // We're about to collide with the read head.
        return false;
    }

    // Write that byte into our ringbuffer.
    ring->buffer[ring->writehead] = recv;
    ring->writehead = next;
    return true;
}
static bool ring_pull(ringbuffer_t *ring, uint8_t *to)
{
    if (ring->writehead == ring->readhead)
    {
        // We have to wait for data.
        return false;
    }

    if (to) *to = ring->buffer[ring->readhead];
    ring->readhead = (ring->readhead + 1) % BUFFER_SIZE;
    return true;
}
static size_t ring_readable(ringbuffer_t *ring)
{
    if (ring->readhead <= ring->writehead)
        return ring->writehead - ring->readhead;
    return BUFFER_SIZE - ring->readhead + ring->writehead;
}
static size_t ring_writable(ringbuffer_t *ring)
{
    return BUFFER_SIZE - ring_readable(ring);
}

#include <gint/exc.h>
//---
// Transmit handler
//---
static void scif_inth_tx(void)
{
    size_t bytes_max;
    if (ring_readable(&tx_ring) == 0 && SCIF0.SCFSR.TEND)
    {
        if (SCIF0.SCFDR.TFDC != 0)
            return;

        if (tx_callback.function != NULL)
            gint_call(tx_callback);
        tx_callback = GINT_CALL_NULL;

        /* TODO: This is the part where you're _supposed_ to disable TE, since there is no 
         * data to be read anymore. However, I have found that data still _somehow_ keeps 
         * being transmitted even after TEND and TDFC seem to indicate otherwise. Previously, 
         * a short pause was added at the end, before disabling TE, but as it turns out, this 
         * tends to cause issues. */
        SCIF0.SCSCR.TIE = 0;
        SCIF0.SCFSR.TEND = 0;
        return;
    }
    if (ring_readable(&tx_ring) == 0) return;
    if (!writing) return;

    bytes_max = 16 - SCIF0.SCFDR.TFDC;
    for (size_t i = 0; i < bytes_max && ring_readable(&tx_ring) != 0; i++)
        ring_pull(&tx_ring, (void *) &SCIF0.SCFTDR);
    SCIF0.SCFSR.TDFE = 0;
    if (ring_readable(&tx_ring) == 0)
        writing = false;
}
//---
// Receive handler
//---
static void scif_inth_rx(void)
{
    size_t bytes_max = SCIF0.SCFDR.RFDC;
    for (size_t i = 0; i < bytes_max; i++)
    {
        if (!ring_push(&rx_ring, SCIF0.SCFRDR))
        {
            // TODO: we may want to notify the other party, maybe with an 
            // XON/XOFF byte if the user so wishes.
        }
    }
    if (rx_needed != 0 && ring_readable(&rx_ring) >= rx_needed)
    {
        uint8_t *rx_bytes = rx_dest;
        for (size_t i = 0; i < rx_needed; i++)
            ring_pull(&rx_ring, &rx_bytes[i]);
        if (rx_callback.function != NULL)
            gint_call(rx_callback);

        rx_callback = GINT_CALL_NULL;
        rx_needed = 0;
        rx_dest = NULL;
    }

    SCIF0.SCFSR.DR = 0;
    SCIF0.SCFSR.RDF = 0;
}

//---
// Error handler
//---
#include <gint/display.h>
static void scif_inth_err(void)
{
    /* TODO: actually manage the error instead of sweeping it under the rug...
     * but we really need to reform the API for this. I should definitely contact 
     * Lephé' about actually designing this well but my access to the actual Internet 
     * is pretty intermittent at the moment :3 */
#if 1
#define YESNO(flag) ((flag) ? "YES" : "NO")
    /* TODO: if we choose to _display_ the error, there better be another way... */
    static int count = 0;
    dclear(C_RED);
    dprint(1, 1,C_WHITE,"ERROR n°%d! SCFSR=0x%04X", ++count, SCIF0.SCFSR.word);
    dprint(1,11,C_WHITE,"PARITY ERRORS=%d", SCIF0.SCFSR.PERC);
    dprint(1,22,C_WHITE,"FRAMES ERRORS=%d", SCIF0.SCFSR.FERC);
    dprint(1,33,C_WHITE,"PER=%s FER=%s ER=%s DR=%s BRK=%s", 
        YESNO(SCIF0.SCFSR.PER), 
        YESNO(SCIF0.SCFSR.FER), 
        YESNO(SCIF0.SCFSR.ER), 
        YESNO(SCIF0.SCFSR.DR), 
        YESNO(SCIF0.SCFSR.BRK)
    );
    dprint(1,55,C_WHITE,"rx_index=%d", rx_ring.writehead);
    dupdate();

#undef YESNO
#endif
    SCIF0.SCFSR.DR = 0;
    SCIF0.SCFSR.ER = 0;
    SCIF0.SCFSR.BRK = 0;
    SCIF0.SCLSR.ORER = 0;
}

//---
// scif_inth: Manages all interrupts for the SCIF0.
//---
static void scif_inth(void)
{
    /* Manage parity errors here */
    // TODO: DR seems to mean that there's not been any data lately?
    SCIF0.SCFSR.DR = 0;
    if (SCIF0.SCFSR.ER /*|| SCIF0.SCFSR.DR*/ || SCIF0.SCFSR.BRK)
        scif_inth_err();
    
    if (SCIF0.SCFSR.TDFE)
        scif_inth_tx();
    if (SCIF0.SCFSR.RDF || SCIF0.SCFSR.DR)
        scif_inth_rx();
}

static void configure(void)
{
	if(isSH3()) return;
    SCIF0.SCSCR.TIE = 0;
    SCIF0.SCSCR.RIE = 0;
    SCIF0.SCSCR.REIE = 0;
    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.RE = 0;
    intc_handler_function(0xC00, GINT_CALL(scif_inth));
    intc_priority(INTC_SCIF0, 0);
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
    
    SCIF0.SCSCR.TIE = 0;
    SCIF0.SCSCR.RIE = 0;
    SCIF0.SCSCR.REIE = 0;
    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.RE = 0;
    POWER.MSTPCR0.SCIF = 1;
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
        if (potential_scbrr < 0 || potential_scbrr >= 255)
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
    uint8_t best_n = 0b00, best_scbrr = 0x00;
    if (!config || serial_is_open()) return 0;
    if (config->parity > SERIAL_PARITY_ODD)
        return 0;
    if (config->stop_bits != 1 && config->stop_bits != 2)
        return 0;
    if (config->data_width != 7 && config->data_width != 8)
        return 0;

    if (!scif_best_scbrr(config->baudrate, &best_n, &best_scbrr))
        return 0;

    SCIF0.SCBRR = 0; 
    SCIF0.SCSCR.word = 0;
    SCIF0.SCSMR.word = 0;


    SCIF0.SCSCR.TIE = 0;
    SCIF0.SCSCR.RIE = 0;
    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.RE = 0;
    SCIF0.SCSCR.CKE = 0b00;

    SCIF0.SCFCR.TFRST = 1;
    SCIF0.SCFCR.RFRST = 1;

    // TODO: custom N/CKE value
    SCIF0.SCSMR.CKS = best_n;
    SCIF0.SCSMR.CHR = config->data_width == 7;
    SCIF0.SCSMR.STOP = config->stop_bits == 2;

    SCIF0.SCSMR.PE = config->parity != SERIAL_PARITY_NONE;
    SCIF0.SCSMR.OE = !!(config->parity - 1);
    SCIF0.SCSMR.CA = 0;
    SCIF0.SCBRR = best_scbrr;

    sleep_ms( 10);                  // TODO: this isn't quite correct. we should wait for one bit.
                                    // ig you could compute its length on the spot?
    
    SCIF0.SCFCR.RTRG = 0b10;        // Full at 8 bytes
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

    SCIF0.SCSCR.REIE = 1;
    SCIF0.SCSCR.RIE = 1;
    SCIF0.SCSCR.RE = 1;
    SCIF0.SCLSR.ORER = 0;

    // Setup ringbuffers
    rx_ring.readhead = 0;
    rx_ring.writehead = 0;
    rx_dest = NULL;
    rx_needed = 0;
    rx_callback = GINT_CALL_NULL;

    opened = true;
    intc_priority(INTC_SCIF0, 1);
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
    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.RE = 0;
    SCIF0.SCSCR.TIE = 0;
    SCIF0.SCSCR.RIE = 0;
    serial_wait();
    intc_priority(INTC_SCIF0, 0);
}

static void set_flag(volatile bool *flag)
{
    *flag = true;
}
void serial_write_sync(void const *data, size_t size)
{
    volatile bool flag = false;
    if (!serial_is_open() || !data || size == 0)
        return;
    serial_write_async(data, size, GINT_CALL(set_flag, &flag));
    while (!flag);
}
void serial_write_async(void const *data, size_t size, gint_call_t cb)
{
    uint8_t const *bytes = data;

    // TODO: error statuses
    if (!bytes || !serial_is_open()) return;
    if (size >= ring_writable(&tx_ring)) return;
    serial_wait();

    // TODO: should we have a cap? i feel like we need to copy data elsewhere, 
    // since it could be modified...
    tx_callback = cb;
    while (size--)
        ring_push(&tx_ring, *bytes++);

    // Go!
    writing = true;
    SCIF0.SCSCR.TE = 1;
    SCIF0.SCSCR.TIE = 1;
}

void serial_read_sync(void *data, size_t size)
{
    // Attempt to read from the ring buffer
    uint8_t *bytes = data;
    if (!bytes || !serial_is_open()) return;

    while (size != 0)
    {
        if (ring_pull(&rx_ring, bytes))
        {
            bytes++;
            size--;
        }
    }
}

void serial_read_async(void *data, size_t size, gint_call_t callback)
{
    uint8_t *bytes = data;
    if (!bytes || !serial_is_open()) return;
    if (ring_readable(&rx_ring) >= size)
    {
        /* Attempt to check the ring buffer early */
        serial_read_sync(data, size);
        if (callback.function != NULL)
            gint_call(callback);
        return;
    }

    rx_callback = callback;
    rx_dest = data;
    rx_needed = size;
}

gint_driver_t drv_scif = {
    .name           = "SCIF",
    .configure      = configure,
    .bind           = bind,
    .unbind         = unbind,
    .funbind        = funbind,

    .hpowered       = check_power,
    .hpoweroff      = poweroff,
    .hpoweron       = poweron,
    .hsave          = (void *)save_ctx,
    .hrestore       = (void *)restore_ctx,
    .state_size     = sizeof(scif_state_t)
};
GINT_DECLARE_DRIVER(16, drv_scif);
