/*
Functions in this file negotiate with the operating system for characters,
and write characters in a barely buffered fashion on the display.
*/

#include "me.h"
#include <errno.h>
#include <termios.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>

static struct termios otermios, ntermios;
static int cmdpipe[2];

static void window_change_handler(int signum);

/* Gets called once to set up the terminal device streams. */
void ttopen(void)
{
	tcgetattr(0, &otermios);
	ntermios = otermios;

	/* Input: disable all processing except keep 7-bit characters */
	ntermios.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK |
				INLCR | IGNCR | ICRNL | IXON);

	/* Output: Disables all output processing */
	ntermios.c_oflag &= ~OPOST;

	/* Local: Raw mode, no signals, no echo, no canonical mode */
	ntermios.c_lflag &= ~(ISIG | ICANON | ECHO | IEXTEN);

	/* Read blocks until at least 1 character is available. */
	ntermios.c_cc[VMIN] = 1;
	/* No timeout */
	ntermios.c_cc[VTIME] = 0;

	tcsetattr(0, TCSADRAIN, &ntermios);

	if (pipe(cmdpipe) < 0) {
		fprintf(stderr, "failed creating command pipe\n");
		return;
	}

	signal(SIGWINCH, window_change_handler);
}

/* Gets called just before we go back to the command interpreter. */
void ttclose(void)
{
	close(cmdpipe[0]);
	close(cmdpipe[1]);
	tcsetattr(0, TCSADRAIN, &otermios);
}

void ttputc(int c)
{
	fputc(c, stdout);
}

void ttputs(const char *str)
{
	int c;
	while ((c = *str++) != '\0')
		ttputc(c);
}

int ttgetc(void)
{
	static unsigned char buf[32];
	static int cursor = 0, len = 0;
	struct pollfd fds[2] = {{ 0, POLLIN, 0 }, { cmdpipe[0], POLLIN, 0 }};
	int fd;

	/* Check whether there are buffered keys */
	if (cursor < len)
		goto ret;

	if (poll(fds, 2, -1 /* wait forever */) < 0)
		return 0;

	/* Keys from stdin have higher priority */
	fd = fds[0].revents & POLLIN ? fds[0].fd : fds[1].fd;

	len = read(fd, buf, sizeof(buf));
	if (len <= 0)
		return 0;

	cursor = 0;
ret:
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

static void window_change_handler(int signum)
{
	static const unsigned char cmd = TERM_REINIT_KEY;
	if (write(cmdpipe[1], &cmd, sizeof(cmd)) < 0)
		mlwrite("failed writing to command pipe");

	signal(SIGWINCH, window_change_handler);
}

static char is_suspended = 0;

/* CONT signal is catchable, but we can not call `ansiopen` twice. */
static void continue_handler(int signum)
{
	if (!is_suspended)
		return;
	is_suspended = 0;
	ansiopen();
	update(TRUE);
}

static void suspend_handler(int signum)
{
	is_suspended = 1;
	ansiclose();
	kill(0, SIGSTOP);
}

void suspend_self(void)
{
	kill(0, SIGTSTP);
}

void bind_exithandler(void (*fn)(int))
{
	signal(SIGHUP, fn);
	signal(SIGTERM, fn);
	signal(SIGTSTP, suspend_handler);
	signal(SIGCONT, continue_handler);
}
