/*
 * The functions in this file negotiate with the operating system for
 * characters, and write characters in a barely buffered fashion on the display.
 */

#include "efunc.h"
#include "edef.h"
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

/* Have to include this file since TIOCGWINSZ is define in this file */
#include <sys/ioctl.h>

/* Mac OS X's termios.h doesn't have the following 2 macros, define them. */
#if defined(_DARWIN_C_SOURCE) || defined(_FREEBSD_C_SOURCE)
#define OLCUC	0000002
#define XCASE	0000004
#endif

static struct termios otermios;	/* original terminal characteristics */
static struct termios ntermios;	/* charactoristics to use inside */

/* This function is called once to set up the terminal device streams. */
void ttopen(void)
{
	tcgetattr(0, &otermios);
	ntermios = otermios;

	/* Raw CR/NL etc input handling, but keep ISTRIP on a 7-bit line. */
	ntermios.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK
		| INPCK | INLCR | IGNCR | ICRNL);

	/* Raw CR/NR etc output handling */
	ntermios.c_oflag &=
		~(OPOST | ONLCR | OLCUC | OCRNL | ONOCR | ONLRET);

	/* No signal handling, no echo etc */
	ntermios.c_lflag &= ~(ISIG | ICANON | XCASE | ECHO | ECHOE | ECHOK
		| ECHONL | NOFLSH | TOSTOP | ECHOCTL
		| ECHOPRT | ECHOKE | FLUSHO | PENDIN | IEXTEN);

	/* One character, no timeout */
	ntermios.c_cc[VMIN] = 1;
	ntermios.c_cc[VTIME] = 0;

	tcsetattr(0, TCSADRAIN, &ntermios);
}

/*
 * This function gets called just before we go back to the command interpreter.
 * Return 0 on success.
 */
void ttclose(void)
{
	tcsetattr(0, TCSADRAIN, &otermios);
}

/* Return 0 on success */
void ttputc(int c)
{
	fputc(c, stdout);
}

int ttgetc(void)
{
	static unsigned char buf[32];
	static int cursor = 0, len = 0;

	if (cursor >= len) {
		len = read(0, buf, sizeof(buf));
		if (len <= 0)
			return 0;
		cursor = 0;
	}

	return buf[cursor++];
}

void ttflush(void)
{
	int status;

	status = fflush(stdout);
	while (status < 0 && errno == EAGAIN) {
		sleep(1);
		status = fflush(stdout);
	}
}

void getscreensize(int *widthp, int *heightp)
{
#ifdef TIOCGWINSZ
	struct winsize size;
	*widthp = 0;
	*heightp = 0;
	if (ioctl(0, TIOCGWINSZ, &size) < 0) {
		*widthp = 0;
		*heightp = 0;
		return;
	}
	*widthp = size.ws_col;
	*heightp = size.ws_row;
#else
	*widthp = 0;
	*heightp = 0;
#endif
}
