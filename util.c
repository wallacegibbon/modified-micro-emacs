#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* Trim preceding and trailing spaces.  dest and src can be the same address */
char*
trim_spaces(char *dest, const char *src, size_t size, int *trunc)
{
	const char *start = src, *end;
	char *cp = dest;

	if (dest == NULL || src == NULL)
		return NULL;

	/* Find the first non-space starting position of src */
	while (*start == ' ')
		++start;

	/* Move to the end of the source string */
	end = start;
	while (*end)
		++end;

	/* Find the first non-space ending position of src */
	while (end[-1] == ' ')
		--end;

	/* Copy the source string to destination */
	while (start < end && size-- > 1)
		*cp++ = *start++;
	*cp = '\0';

	if (trunc != NULL)
		*trunc = size == 0;

	return dest;
}

char*
strncpy_safe(char *dest, const char *src, size_t size)
{
	if (!size)
		return dest;
	dest[--size] = '\0';
	return strncpy(dest, src, size);
}

/* Reverse string copy. */
void
rvstrcpy(char *rvstr, const char *str)
{
	int i;
	str += (i = strlen(str));
	while (i-- > 0)
		*rvstr++ = *--str;

	*rvstr = '\0';
}

void
die(int code, void (*deinit)(void), const char *fmt, ...)
{
	va_list args;

	if (deinit != NULL)
		deinit();

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	exit(code);
}
