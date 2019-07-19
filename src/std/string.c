//---
//	gint:core:string - replicas of a few string functions
//---

#include <gint/defs/types.h>
#include <gint/defs/attributes.h>
#include <stdarg.h>

GWEAK size_t strlen(const char *str)
{
	int len = 0;
	while(str[len]) len++;
	return len;
}

GWEAK char *strncpy(char *dst, const char *src, size_t n)
{
	size_t i = 0;
	while(i < n && (dst[i] = src[i])) i++;
	return dst;
}

GWEAK char *strcat(char *dest, const char *src)
{
    unsigned long fin_dest = strlen(dest);
    unsigned int i;
    for (i = 0 ; i <= strlen(src) ; i++) dest[fin_dest + i] = src[i];
    return dest;
}
