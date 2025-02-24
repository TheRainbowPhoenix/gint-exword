//---
//	core:kernel - Kernel functions
//---

#ifndef GINT_CORE_KERNEL
#define GINT_CORE_KERNEL

#include <gint/defs/types.h>

/* gint_load_onchip_sections(): Initialize on-chip memory sections */
void gint_load_onchip_sections(void);

/* gint_copy_vram(): Copy gint's VRAM to the OS to avoid flickering during
   certain world switches. */
void gint_copy_vram(void);

/* kinit(): Install and start gint */
void kinit(u32 *load_info);

/* kquit(): Quit gint and give back control to the system */
void kquit(void);

/* Select the CASIOWIN API for syscalls. */
void gint_set_CASIOWIN_API(int API);

#endif /* GINT_CORE_KERNEL */
