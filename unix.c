/* We have to include this file since TIOCGWINSZ is define in this file */
#include <sys/ioctl.h>
#include <stdlib.h>

static inline int
getenv_num(const char *envvar)
{
	const char *v = getenv(envvar);
	return v != NULL ? atoi(v) : 0;
}

void
getscreensize(int *widthp, int *heightp)
{
#ifdef TIOCGWINSZ
	struct winsize size;
	if (ioctl(0, TIOCGWINSZ, &size) >= 0) {
		*widthp = size.ws_col;
		*heightp = size.ws_row;
	} else {
#endif
		*widthp = getenv_num("COLUMNS");
		*heightp = getenv_num("LINES");
#ifdef TIOCGWINSZ
	}
#endif
}
