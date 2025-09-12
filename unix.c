/* Have to include this file since TIOCGWINSZ is define in this file */
#include <sys/ioctl.h>

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
		*widthp = 80;
		*heightp = 24;
#ifdef TIOCGWINSZ
	}
#endif
}
