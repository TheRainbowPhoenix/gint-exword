#include <gint/config.h>

#if GINT_HW_EX
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

#else
//---
//	gint:keysc - The SH7305 and I/O Key Scan Interfaces
//---

#include <gint/drivers.h>
#include <gint/gint.h>
#include <gint/timer.h>
#include <gint/clock.h>
#include <gint/keyboard.h>
#include <gint/drivers/keydev.h>

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>
#include <gint/drivers/iokbd.h>
#include <gint/hardware.h>

#if GINT_HW_CP
#include <gint/drivers/touch.h>
#endif

#include <string.h>
#include <stdlib.h>

/* Keyboard input device for this Key Scan Interface */
static keydev_t keysc_dev;
/* Keyboard scanner timer */
int16_t keysc_tid = -1;

/* Keyboard scan frequency in Hertz. Start with 128 Hz, this frequency *must
   be high* for the keyboard to work! Reading at low frequencies produces a lot
   of artifacts. See https://www.casiopeia.net/forum/viewtopic.php?p=20592. */
int16_t keysc_scan_Hz = 128;
/* Approximation in microseconds, used by the timer and repeat delays */
uint32_t keysc_scan_us = 7812; /* 1000000 / keysc_scan_Hz */

/* keydev_std(): Standard keyboard input device */
keydev_t *keydev_std(void)
{
	return &keysc_dev;
}
__attribute__((alias("keydev_std")))
keydev_t *_WEAK_keydev_std(void);

/* keysc_scan(): Scand the keyboard */
static void keysc_scan(uint8_t *scan)
{
	if(isSH3())
	{
		iokbd_scan(scan);
		return;
	}

	volatile uint16_t *KEYSC = (void *)0xa44b0000;

	for(int i = 0; i < 6; i++) {
		int data = KEYSC[i];
		scan[2*i] = data & 0xff;
		scan[2*i+1] = data >> 8;
	}
}

/* keysc_tick(): Update the keyboard to the next state */
int keysc_tick(void)
{
	uint8_t scan[12] = { 0 };
	keysc_scan(scan);

	keydev_process_state(&keysc_dev, scan);
	keydev_tick(&keysc_dev, keysc_scan_us);

	/* Freeze abort key combo: SHIFT+7+3+AC/ON */
	if(keydown(KEY_SHIFT) && keydown(KEY_7) && keydown(KEY_3) &&
		keydown(KEY_ACON))
	{
		abort();
	}

#if GINT_HW_CP
	key_event_t ev = touch_scan();
	if(ev.type != KEYEV_NONE)
		keydev_queue_event(&keysc_dev, ev);
#endif

	return TIMER_CONTINUE;
}

/* pollevent() - poll the next keyboard event */
key_event_t pollevent(void)
{
	return keydev_unqueue_event(&keysc_dev);
}

/* waitevent() - wait for the next keyboard event */
key_event_t waitevent(volatile int *timeout)
{
	while(1)
	{
		key_event_t ev = pollevent();
		if(ev.type != KEYEV_NONE) return ev;

		if(timeout && *timeout) break;
		sleep();
	}

	key_event_t ev = { .type = KEYEV_NONE, .time = keysc_dev.time };
	return ev;
}

/* clearevents(): Read all events waiting in the queue */
void clearevents(void)
{
	while(pollevent().type != KEYEV_NONE);
}

void cleareventflips(void)
{
	keydev_clear_flips(&keysc_dev);
}

int keydown(int key)
{
	return keydev_keydown(&keysc_dev, key);
}

int keypressed(int key)
{
	return keydev_keypressed(&keysc_dev, key);
}

int keyreleased(int key)
{
	return keydev_keyreleased(&keysc_dev, key);
}

//---
// Driver initialization
//---

static void configure(void)
{
	keydev_init(&keysc_dev);

	/* Do a first scan to load the initial state (so that keys that are
           pressed at startup are not registered as new presses) */
	keysc_scan(keysc_dev.state_now);
	memcpy(keysc_dev.state_queue, keysc_dev.state_now, 12);

	/* Set the default repeat to 400/40 ms */
	keydev_t *d = keydev_std();
	keydev_set_transform(d, (keydev_transform_t){ KEYDEV_TR_REPEATS, NULL });
	keydev_set_standard_repeats(d, 400 * 1000, 40 * 1000);

	/* The timer will be stopped when the timer driver is unloaded */
	keysc_tid = timer_configure(TIMER_ANY, keysc_scan_us,
		GINT_CALL(keysc_tick));
	if(keysc_tid >= 0) timer_start(keysc_tid);

	gint[HWKBD] = HW_LOADED | (isSH3() ? HWKBD_IO : HWKBD_KSI);
}

//---
// State and driver metadata
//---

gint_driver_t drv_keysc = {
	.name       = "KEYSC",
	.configure  = configure,
};
GINT_DECLARE_DRIVER(23, drv_keysc);

#endif /* GINT_HW_EX */
