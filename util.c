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

void die(int code, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(code);
}
