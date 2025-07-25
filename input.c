#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "wrapper.h"

/*
 * CAUTION: Prefixed chars (e.g. `CTL | 'A'`) may be stored in this variable,
 * which should be okay since functions like `ctoec` will keep it unchanged.
 */
int reeat_char = -1;

/* Get a key from the terminal driver, resolve any keyboard macro action */
int tgetc(void)
{
	int c = reeat_char;
	if (c != -1) {
		reeat_char = -1;
		return c;
	}

	if (kbdmode == PLAY) {
		if (kbdptr < kbdend)
			return (int)*kbdptr++;

		if (--kbdrep < 1) {
			kbdmode = STOP;
#if VISMAC == 0
			update(FALSE);
#endif
		} else {
			/* reset the macro to the begining for the next rep */
			kbdptr = kbdm;
			return (int)*kbdptr++;
		}
	}

	c = TTgetc();
	lastkey = c;

	if (kbdmode == RECORD) {
		*kbdptr++ = c;
		kbdend = kbdptr;
		if (kbdptr == &kbdm[NKBDM - 1]) {
			kbdmode = STOP;
			TTbeep();
		}
	}

	return c;
}

/* Get one keystroke.  The only prefixs legal here are the CTL prefixes. */
int get1key(void)
{
	return ctoec(tgetc());
}

/*
 * Drop CSI arguments and return the CSI command char.
 * Escape sequences are messes of `CSI`, `SS3`, .... and non-standard stuff.
 * We only handle a tiny subset of them to support mouse/touchpad scrolling.
 */
static int csi_drop_args(void)
{
	int c;
	do { c = get1key(); }
	while (isdigit(c) || c == ';');
	return c;
}

/* Get a command from the keyboard.  Process all applicable prefix keys. */
int getcmd(void)
{
	int cmask = 0, c;
	c = get1key();
#if UNIX
	if (c == ESCAPEC) {
		c = get1key();
		if (c == '[' || c == 'O') {
			switch (csi_drop_args()) {
			case 'A':	return CTL | 'P';
			case 'B':	return CTL | 'N';
			default:	return NULLPROC_KEY;
			}
		}
	}
#endif
	if (c == CTLXC) {
		cmask |= CTLX;
ctlx_loop:
		c = get1key();
		if (c == ESCAPEC)
			goto ctlx_loop;

		return cmask | ensure_upper(c);
	}

	return c;
}

/*
 * A more generalized prompt/reply function allowing the caller to specify
 * the proper terminator.
 */
int mlgetstring(const char *prompt, char *buf, int nbuf, int eolchar)
{
	int cpos = 0, c, expc;

	mlwrite("%s", prompt);
	buf[0] = '\0';
char_loop:
	TTflush();
	c = ectoc(expc = get1key());

	/* All return points should clear message line */
	if (expc == eolchar)
		goto normal_exit;

	if (expc == ABORTC) {
		mlerase();
		return ctrlg(FALSE, 0);
	}

	if (expc == QUOTEC) {
		c = ectoc(expc = get1key());
		goto char_append;
	}

	if (c == '\b' || c == 0x7F) {
		if (cpos > 0)
			ttcol -= unput_c(buf[--cpos]);
		goto char_loop;
	}

char_append:
	buf[cpos++] = c;
	buf[cpos] = '\0';
	ttcol += put_c(c, TTputc);

	if (cpos < nbuf - 1)
		goto char_loop;

	mlwrite("? Input too long");

normal_exit:
	mlerase();
	return buf[0] != '\0';
}

int mlgetchar(const char *fmt, ...)
{
	va_list ap;
	int c;

	va_start(ap, fmt);
	mlvwrite(fmt, ap);
	va_end(ap);

	c = ctoec(tgetc());

	/* Clear message line after `tgetc` */
	mlerase();
	return c;
}

int mlreply(char *prompt, char *buf, int nbuf)
{
	return mlgetstring(prompt, buf, nbuf, ENTERC);
}

int mlyesno(char *prompt)
{
	for (;;) {
		switch (mlgetchar("%s (y/n)? ", prompt)) {
		case 'y':	return TRUE;
		case 'n':	return FALSE;
		case ABORTC:	return ABORT;
		}
	}
}

int ectoc(int c)
{
	if (c & CTL)
		c = ~CTL & (c - '@');
	return c;
}

int ctoec(int c)
{
	if (c >= 0x00 && c <= 0x1F)
		c = CTL | (c + '@');
	return c;
}
