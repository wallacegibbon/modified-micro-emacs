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

#define NULLPROC_KEYS	(CTLX | META | CTL | 'Z')

/*
 * Escape sequences are messes of `CSI`, `SS3`, .... and non-standard stuff.
 * We only handle a tiny subset of them to support mouse/touchpad scrolling.
 */
static int handle_special_esc(void)
{
	int c;

	/* We do not need CSI arguments (like "12;3;45") in input. */
	do c = get1key();
	while (isdigit(c) || c == ';');

	switch (c) {
	case 'A':	return CTL | 'P';
	case 'B':	return CTL | 'N';
	default:	return NULLPROC_KEYS;
	}
}

/* Get a command from the keyboard.  Process all applicable prefix keys. */
int getcmd(void)
{
	int cmask = 0, c;

	c = get1key();

	/* process META prefix */
	if (c == METAC) {
proc_metac:
		c = get1key();
#if UNIX
		if (c == '[' || c == 'O')
			return handle_special_esc();
		if (c == METAC) {
			cmask |= META;
			goto proc_metac;
		}
#endif
		cmask |= META;
		if (c == CTLXC)
			goto proc_ctlxc;
		return cmask | ensure_upper(c);
	}

	/* process CTLX prefix */
	if (c == CTLXC) {
proc_ctlxc:
		c = get1key();
		cmask |= CTLX;
		if (c == METAC)
			goto proc_metac;
		return cmask | ensure_upper(c);
	}

	/* otherwise, just return it */
	return c;
}

/*
 * A more generalized prompt/reply function allowing the caller to specify
 * the proper terminator.
 */
int mlgetstring(char *prompt, char *buf, int nbuf, int eolchar)
{
	int quote = 0, cpos = 0, c, expc;

	mlwrite("%s", prompt);
loop:
	c = ectoc(expc = get1key());

	/* All return points should clear message line */
	if (expc == eolchar && !quote) {
		buf[cpos++] = 0;
		mlerase();
		return buf[0] != 0;
	}
	if (expc == ABORTC && !quote) {
		mlerase();
		return ctrlg(FALSE, 0);
	}

	if ((c == 0x7F || c == '\b') && !quote) {
		if (cpos > 0)
			ttcol -= unput_c(buf[--cpos]);
		goto loop;
	}

	if (expc == QUOTEC && !quote) {
		quote = 1;
		goto loop;
	}

	/* Normal char or quoted char */

	quote = 0;
	if (cpos < nbuf - 1) {
		buf[cpos++] = c;
		ttcol += put_c(c, TTputc);
		TTflush();
	}
	goto loop;
}

int mlgetchar(char *fmt, ...)
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

#if NAMED_CMD
static int (*getfn_byname(char *fname))(int, int)
{
	struct name_bind *ffp;

	for (ffp = names; ffp->fn != NULL; ++ffp) {
		if (strcmp(fname, ffp->f_name) == 0)
			return ffp->fn;
	}
	return NULL;
}

/* Execute a named command even if it is not bound. */
int namedcmd(int f, int n)
{
	char buf[NSTRING];
	int (*fn)(int, int);

	if (mlgetstring(": ", buf, NSTRING, ENTERC) != TRUE)
		return FALSE;

	fn = getfn_byname(buf);
	if (fn == NULL) {
		mlwrite("(No such function)");
		return FALSE;
	}

	return fn(f, n);
}
#endif

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
