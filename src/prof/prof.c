#include <gint/timer.h>
#include <gint/clock.h>
#include <gint/mpu/tmu.h>
#include <gint/hardware.h>
#include <gint/prof.h>

/* Timer counter */
uint32_t volatile *prof_tcnt = NULL;
/* Timer ID */
static int prof_timer = -1;

static int callback(void)
{
	return TIMER_CONTINUE;
}

/* prof_init(): Initialize the profiler's timer */
int prof_init(void)
{
	/* Get a TMU with the exact constant 0xffffffff */
	int timer = -1;
	for(int t = 2; t >= 0 && timer == -1; t--)
	{
		timer = timer_configure(t | TIMER_Pphi_4, 0xffffffff,
			GINT_CALL(callback));
	}
	if(timer == -1)
	{
		prof_quit();
		return 1;
	}

	/* Keep the address of the TCNT register */
	prof_tcnt = isSH3()
		? &SH7705_TMU.TMU[timer].TCNT
		: &SH7305_TMU.TMU[timer].TCNT;

	timer_start(timer);
	prof_timer = timer;

	return 0;
}

/* prof_quit(): Free the profiler's timer */
void prof_quit(void)
{
	if(prof_timer >= 0) timer_stop(prof_timer);
	prof_timer = -1;
}

/* prof_time(): Time spent in a given context, in microseconds */
uint32_t prof_time(prof_t prof)
{
	int Pphi = clock_freq()->Pphi_f;
	return ((uint64_t)prof.elapsed * 4 * 1000000) / Pphi;
}
