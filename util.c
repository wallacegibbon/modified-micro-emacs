#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

char *strncpy_safe(char *dest, const char *src, size_t size)
{
	dest[size - 1] = '\0';
	return strncpy(dest, src, size - 1);
}

void die(int code, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	exit(code);
}
