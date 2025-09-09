#include <gint/gint.h>

int debug_syscall_count = 0;
int debug_syscall_lastnr = 0;

void syscall_entry(gint_inth_callback_context_t *ptr)
{
	/* TODO: Currently, only user's r0...r7, spc, and ssr are saved here */
	(void)ptr;

	++debug_syscall_count;
	debug_syscall_lastnr = ptr->r[3];
}
