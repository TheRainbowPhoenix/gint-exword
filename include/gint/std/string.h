//---
//	gint:std:string - a few <string.h> functions provided by gint
//---

#ifndef GINT_STD_STRING
#define GINT_STD_STRING

#include <stddef.h>

/* memcpy(): Copy a chunk of memory to a non-overlapping destination */
void *memcpy(void * restrict dest, void const * restrict src, size_t n);

/* memset(): Fill a chunk of memory with a single byte */
void *memset(void *dest, int byte, size_t n);

/* strlen(): Length of a NUL-terminated string */
size_t strlen(char const *str);

/* strncpy(): Copy a string with a size limit*/
char *strncpy(char *dst, char const *src, size_t n);

/* strcat(): Concatenation of src in dest*/
char *strcat(char *dest, const char *src);

#endif /* GINT_STD_STRING */
