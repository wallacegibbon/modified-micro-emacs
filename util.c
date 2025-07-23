#include <string.h>
#include <stddef.h>

char *strncpy_safe(char *dest, const char *src, size_t size)
{
	dest[size - 1] = '\0';
	return strncpy(dest, src, size - 1);
}
