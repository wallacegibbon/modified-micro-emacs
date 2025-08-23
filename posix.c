/*
 * The functions in this file negotiate with the operating system for
 * characters, and write characters in a barely buffered fashion on the display.
 */

#include "me.h"
#include <termios.h>
#include <unistd.h>
#include <errno.h>

/* Have to include this file since TIOCGWINSZ is define in this file */
#include <sys/ioctl.h>

static struct termios otermios, ntermios;

/* This function is called once to set up the terminal device streams. */
void ttopen(void)
{
	tcgetattr(0, &otermios);
	ntermios = otermios;

	/* Input: disable all processing except keep 7-bit characters */
	ntermios.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK |
				INLCR | IGNCR | ICRNL);

	/* Output: Disables all output processing */
	ntermios.c_oflag &= ~OPOST;

	/* Local: Raw mode, no signals, no echo, no canonical mode */
	ntermios.c_lflag &= ~(ISIG | ICANON | ECHO | IEXTEN);

	/* Read blocks until at least 1 character is available. */
	ntermios.c_cc[VMIN] = 1;
	/* No timeout */
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
