#include <gint/keyboard.h>
#include <gint/drivers/keydev.h>
#include <string.h>

static keydev_t exword_keyboard;
static int exword_keyboard_ready;

keydev_t *keydev_std(void)
{
    return &exword_keyboard;
}

static void keysc_scan(uint8_t scan[12])
{
    volatile uint16_t const *keys = (void *)0xa44b0000;
    for(int i = 0; i < 6; i++) {
        uint16_t word = keys[i];
        scan[2 * i] = word & 0xff;
        scan[2 * i + 1] = word >> 8;
    }
}

static void keysc_prepare(void)
{
    if(exword_keyboard_ready) return;
    keydev_init(&exword_keyboard);
    keysc_scan(exword_keyboard.state_now);
    memcpy(exword_keyboard.state_queue, exword_keyboard.state_now, 12);
    exword_keyboard_ready = 1;
}

int keysc_tick(void)
{
    uint8_t scan[12];
    keysc_prepare();
    keysc_scan(scan);
    keydev_process_state(&exword_keyboard, scan);
    keydev_tick(&exword_keyboard, 7812);
    return 0;
}

key_event_t pollevent(void)
{
    keysc_tick();
    return keydev_unqueue_event(&exword_keyboard);
}

key_event_t waitevent(volatile int *timeout)
{
    for(;;) {
        key_event_t ev = pollevent();
        if(ev.type != KEYEV_NONE || (timeout && *timeout))
            return ev;
    }
}

void clearevents(void)
{
    while(pollevent().type != KEYEV_NONE);
}

void cleareventflips(void)
{
    keysc_prepare();
    keydev_clear_flips(&exword_keyboard);
}

int keydown(int key)
{
    keysc_prepare();
    return keydev_keydown(&exword_keyboard, key);
}

int keypressed(int key)
{
    keysc_prepare();
    return keydev_keypressed(&exword_keyboard, key);
}

int keyreleased(int key)
{
    keysc_prepare();
    return keydev_keyreleased(&exword_keyboard, key);
}

key_event_t getkey(void)
{
    for(;;) {
        key_event_t ev = waitevent(NULL);
        if(ev.type == KEYEV_DOWN) return ev;
    }
}

key_event_t getkey_opt(int opt, volatile int *timeout)
{
    (void)opt;
    for(;;) {
        key_event_t ev = waitevent(timeout);
        if(ev.type == KEYEV_NONE || ev.type == KEYEV_DOWN)
            return ev;
    }
}

/* The hosted EX-word keyboard has no gint timer driver yet. */
void sleep(void)
{
    keysc_tick();
}

int keysc_scan_frequency(void) { return 128; }
uint32_t keysc_scan_frequency_us(void) { return 7812; }
void keysc_set_scan_frequency(int frequency) { (void)frequency; }
