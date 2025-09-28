#include <gint/drivers/states.h>
#include <gint/drivers/scif.h>
#include <gint/mpu/power.h>
#include <gint/hardware.h>
#include <gint/mpu/pfc.h>
#include <gint/drivers.h>
#include <gint/serial.h>
#include <gint/clock.h>
#include <gint/intc.h>
#include <gint/exc.h>

#include <string.h>

/* Ringbuffer sizes */
#define BUFFER_SIZE             1024
/* Amount of data in the RX ringbuffer to trigger a XON/XOFF softlimit  */
#define RX_SOFTLIMIT            128

#define ASCII_XOFF              '\x13'
#define ASCII_XON               '\x11'

#define POWER SH7305_POWER
#define SCIF0 SH7305_SCIF
#define PFC SH7305_PFC

static bool opened = false;

typedef struct 
{
    uint8_t buffer[BUFFER_SIZE];
    size_t writehead, readhead;

    size_t written, read;

    /* Whenever the buffer was 'blocked'. Only used for flowcontrol */
    bool blocked;
} ringbuffer_t;

// TODO: this should be replaced with a more robust mechanism
static bool writing = false;

static ringbuffer_t tx_ring = { .readhead = 0, .writehead = 0 };
static gint_call_t tx_callback = GINT_CALL_NULL;
static serial_flow_control_t flow_control_flags = SERIAL_FLOWCONTROL_NONE;

static ringbuffer_t rx_ring = { .readhead = 0, .writehead = 0 };
static gint_call_t rx_callback = GINT_CALL_NULL;
static size_t rx_needed = 0;
static void *rx_dest = NULL;
static int *rx_rc = NULL;

static size_t rx_parity_errs = 0, rx_framing_errs = 0;

//---
// Ring buffer implementation
//---
static void ring_reset(ringbuffer_t *ring)
{
    ring->readhead = 0;
    ring->writehead = 0;
    ring->blocked = false;
    ring->written = ring->read = 0;
}
static bool ring_push(ringbuffer_t *ring, uint8_t recv)
{
    size_t next = (ring->writehead + 1) % BUFFER_SIZE;
    if(next == ring->readhead) {
        return false;
    }

    // Write that byte into our ringbuffer.
    ring->buffer[ring->writehead] = recv;
    ring->writehead = next;
    ring->written ++;
    return true;
}
static bool ring_pull(ringbuffer_t *ring, uint8_t *to)
{
    if (ring->writehead == ring->readhead) {
        // We have to wait for data.
        return false;
    }

    if (to) *to = ring->buffer[ring->readhead];
    ring->read++;
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

//---
// Transmit handler
//---

/* scif_forcewrite(): Forces through a byte to the SCIF, ignoring buffers */
static void scif_forcewrite(uint8_t c)
{
    // TODO: we should probably manage things like TE/TIE being off when 
    // there's no initial transfers
    while (SCIF0.SCFDR.TFDC == 0x10);
    SCIF0.SCFTDR = c;
}
static void scif_inth_tx(void)
{
    size_t bytes_max;

    if (ring_readable(&tx_ring) == 0 && SCIF0.SCFSR.TEND) {
        if (SCIF0.SCFDR.TFDC != 0)
            return;

        if (tx_callback.function != NULL)
            gint_call(tx_callback);
        tx_callback = GINT_CALL_NULL;

        /* XXX: This is the part where you're _supposed_ to disable TE, since there is no 
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
static bool scif_check_flowcontrol(void)
{
    /* TODO: Deadlock! I have no idea why it happens but it does whenever you try to do flow control
     * (with some pretty weird values TBF but still) */
    if(!(flow_control_flags & SERIAL_FLOWCONTROL_SOFTWARE)) return false;
    bool blocked = rx_ring.blocked;
    bool threshold = ring_writable(&rx_ring) <= RX_SOFTLIMIT;

    if(!blocked && threshold) {
        /* XOFF the RX ringbuffer. We have to intentionally wait for our FIFO to have data */
        rx_ring.blocked = true;
        scif_forcewrite(ASCII_XOFF);                    // TX gets blocked
        return true;
    } else if(blocked && !threshold) {
        /* Clear our way, we can receive data. */
        rx_ring.blocked = false;
        scif_forcewrite(ASCII_XON);
        return true;
    }
    return false;
}

static void scif_inth_rx(void)
{
    size_t bytes_max;

    scif_check_flowcontrol();

    bytes_max = SCIF0.SCFDR.RFDC;
    for(size_t i = 0; i < bytes_max; i++) {
        uint8_t c = SCIF0.SCFRDR;
        bool software_fc = !!(flow_control_flags & SERIAL_FLOWCONTROL_SOFTWARE);
        if(software_fc) {
            if(c == ASCII_XOFF)         { tx_ring.blocked = true; continue; }
            else if(c == ASCII_XON)     { tx_ring.blocked = false; continue; }
        }
        if(!ring_push(&rx_ring, c) && software_fc) {
            /* TODO: Our XOFF/XON signaling was not respected! What should we do? */
        }
    }

    /* Disabling TIE still keeps some communication going, but doesn't make the SCIF complain 
     * and crash the entire calculator */
    SCIF0.SCSCR.TIE = !tx_ring.blocked;

    if(rx_needed != 0 && ring_readable(&rx_ring) >= rx_needed) {
        uint8_t *rx_bytes = rx_dest;
        for(size_t i = 0; i < rx_needed; i++)
            ring_pull(&rx_ring, &rx_bytes[i]);
        if(rx_rc)
            *rx_rc = 0;
        if(rx_callback.function != NULL)
            gint_call(rx_callback);

        rx_callback = GINT_CALL_NULL;
        rx_needed = 0;
        rx_dest = NULL;
    }

    scif_check_flowcontrol();

    SCIF0.SCFSR.DR = 0;
    SCIF0.SCFSR.RDF = 0;
}

//---
// Error handler
//---
static void scif_inth_err(void)
{
    /* TODO: actually manage the error instead of sweeping it under the rug...
     * but we really need to reform the API for this. I should definitely contact 
     * Lephé' about actually designing this well but my access to the actual Internet 
     * is pretty intermittent at the moment :3 */
    if(SCIF0.SCFSR.FER) {
        /* TODO: A framing error occured */
        if(rx_needed != 0) {
            if(rx_rc) *rx_rc = SERIAL_ERROR_FRAMING;
            if(rx_callback.function != NULL) gint_call(rx_callback);

            rx_dest = NULL;
            rx_callback = GINT_CALL_NULL;
            rx_needed = 0;
        }
        rx_framing_errs++;
    }
    if(SCIF0.SCFSR.PER) {
        /* TODO: A parity error occured */
        if(rx_needed != 0) {
            if(rx_rc) *rx_rc = SERIAL_ERROR_FRAMING;
            if(rx_callback.function != NULL) gint_call(rx_callback);

            rx_dest = NULL;
            rx_callback = GINT_CALL_NULL;
            rx_needed = 0;
        }
        rx_parity_errs++;
    }
    if(SCIF0.SCLSR.ORER) {
        /* TODO: Overrun! If flowcontrol is on, it probably wasn't respected.
         * How should this be managed? */
        gint_panic(1234);
    }
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
    SCIF0.SCFSR.DR = 0;

    if (SCIF0.SCFSR.ER || SCIF0.SCFSR.BRK || SCIF0.SCLSR.ORER)
        scif_inth_err();

    if (SCIF0.SCFSR.RDF || SCIF0.SCFSR.DR)
        scif_inth_rx();
    if (SCIF0.SCFSR.TDFE && !tx_ring.blocked)
        scif_inth_tx();


}

static void configure(void)
{
	if(isSH3()) return;

    PFC.PUCR.P3MD = 0b01;                       // Tie PU3MD as an output
                                                // The other pins serve as inputs, 
                                                // as default.
    PFC.PUDR.P3DT = 1;
    PFC.PUDR.P2DT = 1;

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
    for (int potential_n = 0; potential_n < 4; potential_n++) {
        // Figure out the value of SCBRR
        // The multiplication by 1M is already implied by the fact that Pphi_f is in Hz, not MHz.
        int32_t potential_scbrr = (info->Pphi_f / scale_by_n(64 * baudrate, potential_n)) - 1;
        int32_t potential_deviation = (info->Pphi_f % scale_by_n(64 * baudrate, potential_n));

        // This value of SCBRR is completely invalid.
        if (potential_scbrr < 0 || potential_scbrr >= 255)
            continue;

        // Check the amount of "error" by looking at the remainder.
        if (potential_deviation < best_deviation) {
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
    uint8_t best_n = 0b00, best_scbrr = 0x00;
    if (!config || serial_is_open()) return SERIAL_ERROR_INVALID_PARAMS;
    if (config->parity > SERIAL_PARITY_ODD)
        return SERIAL_ERROR_INVALID_PARAMS;
    if (config->stop_bits != 1 && config->stop_bits != 2)
        return SERIAL_ERROR_INVALID_PARAMS;
    if (config->data_width != 7 && config->data_width != 8)
        return SERIAL_ERROR_INVALID_PARAMS;

    /* TODO: should this warrant another error code? */
    if (!scif_best_scbrr(config->baudrate, &best_n, &best_scbrr))
        return SERIAL_ERROR_INVALID_PARAMS;

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

    SCIF0.SCSMR.CKS = best_n;
    SCIF0.SCSMR.CHR = config->data_width == 7;
    SCIF0.SCSMR.STOP = config->stop_bits == 2;

    SCIF0.SCSMR.PE = config->parity != SERIAL_PARITY_NONE;
    SCIF0.SCSMR.OE = !!(config->parity - 1);
    SCIF0.SCSMR.CA = 0;
    SCIF0.SCBRR = best_scbrr;

    sleep_ms( 10);                  /* TODO: this isn't quite correct. we should wait for one bit. */
    
    SCIF0.SCFCR.RTRG = 0b10;        // Full at 8 bytes
    SCIF0.SCFCR.TTRG = 0b00;        // Empty at 8 bytes

    SCIF0.SCFCR.TFRST = 0;
    SCIF0.SCFCR.RFRST = 0;

    // Add-ins which want to "test" the SCIF can simply enable it themselves
    SCIF0.SCFCR.LOOP = 0;

    SCIF0.SCFCR.MCE = 0;
    flow_control_flags = config->flow_control;
    if (config->flow_control & SERIAL_FLOWCONTROL_HARDWARE)
    {
        /* Hardware flow control. The SH7305 does seem to respond to flags like 
         * MCS, but they are completely useless on the mainline calculator line, 
         * as there's only TX/RX/GND connected.
         * Maybe some SH4 models have a fancy port and do care... */
        SCIF0.SCFCR.MCE = 1;                /* Enable flow-control */
        SCIF0.SCFCR.RSTRG = 0b110;          /* 12 bytes in buffer for ~RTS. */
    }

    // We purposefully disable sending/interrupts. We don't need them right now.
    SCIF0.SCSCR.TE = 0;
    SCIF0.SCSCR.TIE = 0;

    SCIF0.SCSCR.REIE = 1;
    SCIF0.SCSCR.RIE = 1;
    SCIF0.SCSCR.RE = 1;
    SCIF0.SCLSR.ORER = 0;

    // Setup ringbuffers
    ring_reset(&rx_ring);
    ring_reset(&tx_ring);
    rx_parity_errs = 0; 
    rx_framing_errs = 0;
    rx_dest = NULL;
    rx_needed = 0;
    rx_callback = GINT_CALL_NULL;

    opened = true;
    intc_priority(INTC_SCIF0, 1);
    return 0;
}

void serial_wait(void)
{
    // Right now, I'm only going to busy-loop until TDFC is empty...
    while (writing) sleep();
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
int serial_write_async(void const *data, size_t size, gint_call_t cb)
{
    uint8_t const *bytes = data;

    // TODO: error statuses
    if (!bytes || !serial_is_open()) return SERIAL_ERROR_NOT_OPENED;
    if (size >= ring_writable(&tx_ring)) return SERIAL_ERROR_DATA_SIZE;
    serial_wait();

    // TODO: should we have a cap? i feel like we need to copy data elsewhere, 
    // since it could be modified...
    tx_callback = cb;
    while(size--)
        ring_push(&tx_ring, *bytes++);

    // Go!
    writing = true;
    SCIF0.SCSCR.TE = 1;
    SCIF0.SCSCR.TIE = 1;
    return 0;
}

/* TODO: replace every version of serial_read_sync to a new serial_read_async_timeout() */
int serial_read_sync_timeout(void *data, size_t size, timeout_t const* timeout)
{
    // Attempt to read from the ring buffer
    uint8_t *bytes = data;
    size_t parity, framing;
    if (!bytes || !serial_is_open()) return SERIAL_ERROR_INVALID_PARAMS;

    parity = rx_parity_errs;
    framing = rx_parity_errs;
    while (size != 0) {
        if (timeout_elapsed(timeout))   return SERIAL_ERROR_TIMEOUT;
        /* TODO: should there be scenarios where we're allowed _not_ to care? */
        if (rx_parity_errs != parity)   return SERIAL_ERROR_PARITY;
        if (rx_framing_errs != framing) return SERIAL_ERROR_FRAMING;

        scif_check_flowcontrol();

        if (ring_pull(&rx_ring, bytes)) {
            bytes++;
            size--;
        }
    }
    return (int) (bytes - (uint8_t *) data);
}
int serial_read_sync(void *data, size_t size)
{
    return serial_read_sync_timeout(data, size, NULL);
}

int serial_read_async(void *data, size_t size, int *rc, gint_call_t callback)
{
    uint8_t *bytes = data;
    if (!bytes || !serial_is_open()) return SERIAL_ERROR_INVALID_PARAMS;
    if (ring_readable(&rx_ring) >= size) {
        /* Attempt to check the ring buffer early */
        int ret = serial_read_sync(data, size);
        if (callback.function != NULL)
            gint_call(callback);
        return ret;
    }

    rx_callback = callback;
    rx_dest = data;
    rx_needed = size;
    rx_rc = rc;

    /* Preliminary error code */
    return 0;
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
