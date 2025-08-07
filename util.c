#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

char *strncpy_safe(char *dest, const char *src, size_t size)
{
	if (!size)
		return dest;
	dest[--size] = '\0';
	return strncpy(dest, src, size);
}

/* Reverse string copy. */
void rvstrcpy(char *rvstr, const char *str)
{
	int i;
	str += (i = strlen(str));
	while (i-- > 0)
		*rvstr++ = *--str;

	*rvstr = '\0';
}

void die(int code, void (*deinit)(void), const char *fmt, ...)
{
	va_list args;

	if (deinit != NULL)
		deinit();

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	exit(code);
}
