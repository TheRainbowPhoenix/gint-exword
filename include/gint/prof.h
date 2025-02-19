//---
//	prof: A manual profiling interface for gint
//---

#ifndef GINT_PROF
#define GINT_PROF

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <gint/defs/attributes.h>
#include <gint/config.h>

/* This provides the old interface for <libprof.h>. */
#define LIBPROF_LIBPROF
/* Advertise libprof version as gint's version. */
#define LIBPROF_VERSION GINT_VERSION

//---
//	Initialization
//---

/* prof_init(): Initialize the profiler's timer

   Starts a timer to count time. The profiling interface automatically
   selects a TMU and tries to use TMU2 before TMU1 before TMU0 so that
   high-priority interrupts remain available, and sets an accurate clock
   configuration.

   Returns non-zero if no timer is available. */
int prof_init(void);

/* prof_quit(): Free the profiler's timer */
void prof_quit(void);

//---
//	Runtime time measurement
//---

/* Context object, has an elasped delay and a recursion level. This object can
   be created on the stack of a function that measures its execution time,
   except if the function is recursive, in which case it should be either
   static or global. */
typedef struct prof_t
{
	uint32_t rec;
	uint32_t elapsed;

} GPACKED(4) prof_t;

/* prof_make(): Create a new context object
   A context can be cleared by assigning it prof_make() again. */
#define prof_make() ((prof_t){ 0, 0 })

/* Timer counter */
extern uint32_t volatile *prof_tcnt;

/* prof_enter(): Start counting time for a function
   This macro should be called at the start of the context scope. If the
   function was already executing then the deepest instance in the stack is
   used instead of creating a new counter. */
#define prof_enter(prof) {				\
	if(!(prof).rec++) (prof).elapsed += *prof_tcnt;	\
}

/* prof_leave(): Stop counting time for a function
   This should be called at the end of the context scope; it only actually
   stops if there is no deeper instance of the context in the stack. If there
   are not as exactly as many prof_leave()'s as prof_enter()'s then the
   resulting time measure will not be relevant at all. */
#define prof_leave(prof) {				\
	if(!--(prof).rec) (prof).elapsed -= *prof_tcnt;	\
}

/* Variant of prof_enter()/prof_leave() for non-recursive contexts */
#define prof_enter_norec(prof) {			\
	(prof).elapsed += *prof_tcnt;			\
}
#define prof_leave_norec(prof) {			\
	(prof).elapsed -= *prof_tcnt;			\
}

/* prof_exec(): Measure a single block of code
   This operation can be used when profiling is not required, and instead
   used to measure the performance of a single bit of code.
   Use it like this:

       uint32_t elasped_us = prof_exec({
           exec_code();
       }); */
#define prof_exec(code) ({		\
	prof_t _prof = prof_make();	\
	prof_enter(_prof);		\
	code;				\
	prof_leave(_prof);		\
	prof_time(_prof);		\
})

//---
//	Post-measurement analysis
//---

/* prof_time(): Time spent in a given context, in microseconds
   Should only be called when the context is not currently executing. */
uint32_t prof_time(prof_t prof);

#ifdef __cplusplus
}
#endif

#endif /* GINT_PROF */
