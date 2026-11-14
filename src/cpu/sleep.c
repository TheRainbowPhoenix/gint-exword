#include <gint/cpu.h>
#include <gint/config.h>

volatile int cpu_sleep_block_counter = 0;

void sleep(void)
{
	#if GINT_HW_EX
	extern int keysc_tick(void);
	keysc_tick();
	#else
	if(cpu_sleep_block_counter <= 0) __asm__("sleep");
	#endif
}

void sleep_block(void)
{
	cpu_atomic_start();
	cpu_sleep_block_counter++;
	cpu_atomic_end();
}

void sleep_unblock(void)
{
	cpu_atomic_start();
	cpu_sleep_block_counter--;
	cpu_atomic_end();
}
