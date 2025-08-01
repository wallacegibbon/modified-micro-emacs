#include "efunc.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

static void report(const char* prefix, const char *err, va_list params)
{
	char msg[4096];
	vsnprintf(msg, sizeof(msg), err, params);
	fprintf(stderr, "%s%s\n", prefix, msg);
}

void die(const char* err, ...)
{
	va_list params;

	va_start(params, err);
	report("fatal: ", err, params);
	va_end(params);
	exit(128);
}

int xmkstemp(char *template)
{
	int fd = mkstemp(template);
	if (fd < 0)
		die("Unable to create temporary file");
	return fd;
}

void *xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (!ret)
		die("Out of memory");
	return ret;
}
