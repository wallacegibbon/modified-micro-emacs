/*
Functions in this file negotiate with the operating system for characters,
and write characters in a barely buffered fashion on the display.
*/

#include "me.h"
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>

static struct termios otermios, ntermios;

/* This function is called once to set up the terminal device streams. */
void
ttopen(void)
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
This function gets called just before we go back to the command interpreter.
*/
void
ttclose(void)
{
	tcsetattr(0, TCSADRAIN, &otermios);
}

void
ttputc(int c)
{
	fputc(c, stdout);
}

void
ttputs(const char *str)
{
	int c;
	while ((c = *str++) != '\0')
		ttputc(c);
}

int
ttgetc(void)
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

void
ttflush(void)
{
	int status;

	status = fflush(stdout);
	while (status < 0 && errno == EAGAIN) {
		sleep(1);
		status = fflush(stdout);
	}
}

static char is_suspended = 0;

/* CONT signal is catchable, but we can not call `ansiopen` twice. */
static void
continue_handler(int signum)
{
	if (!is_suspended)
		return;
	is_suspended = 0;
	ansiopen();
	update(TRUE);
}

static void
suspend_handler(int signum)
{
	is_suspended = 1;
	ansiclose();
	kill(0, SIGSTOP);
}

void
suspend_self(void)
{
	kill(0, SIGTSTP);
}

void
bind_exithandler(void (*fn)(int))
{
	signal(SIGHUP, fn);
	signal(SIGTERM, fn);
	signal(SIGTSTP, suspend_handler);
	signal(SIGCONT, continue_handler);
}
