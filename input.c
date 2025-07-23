#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "wrapper.h"

/*
 * CAUTION: Prefixed chars (e.g. `CTL | 'A'`) may be stored in this variable,
 * which should be okay since functions like `ctoec` will keep it unchanged.
 */
int reeat_char = -1;

static inline void TTputs(char *s)
{
	int (*p)(int) = TTputc;
	while (*s)
		p(*s++);
}

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
int getstring(char *prompt, char *buf, int nbuf, int eolchar)
{
	int quotef = FALSE, cpos = 0, c, expc;

	mlwrite(prompt);

	for (;;) {
		c = ectoc(expc = get1key());

		/* if they hit the line terminate, wrap it up */
		if (expc == eolchar && quotef == FALSE) {
			buf[cpos++] = 0;
			mlwrite("");
			TTflush();

			/* if we default the buffer, return FALSE */
			if (buf[0] == 0)
				return FALSE;

			return TRUE;
		}

		if (expc == ABORTC && quotef == FALSE)
			return ctrlg(FALSE, 0);

		if ((c == 0x7F || c == '\b') && quotef == FALSE) {
			if (cpos != 0) {
				TTputs("\b \b");
				--ttcol;

				if (buf[--cpos] < 0x20) {
					TTputs("\b \b");
					--ttcol;
				}
				if (buf[cpos] == '\n') {
					TTputs("\b\b  \b\b");
					ttcol -= 2;
				}
				TTflush();
			}

		} else if (expc == QUOTEC && quotef == FALSE) {
			quotef = TRUE;

		} else {
			quotef = FALSE;
			if (cpos < nbuf - 1) {
				buf[cpos++] = c;
				ttcol += put_c(c, TTputc);
				TTflush();
			}
		}
	}
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

	if (getstring(": ", buf, NSTRING, ENTERC) != TRUE)
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
	return getstring(prompt, buf, nbuf, ENTERC);
}

int mlyesno(char *prompt)
{
	char buf[64 /* prompt */ + 8 /* " (y/n)? " */ + 1];
	for (;;) {
		strncpy_safe(buf, prompt, 65);
		strcat(buf, " (y/n)? ");
		mlwrite(buf);

		switch (ctoec(tgetc())) {
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
