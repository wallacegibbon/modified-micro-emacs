#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "wrapper.h"

/*
 * CAUTION: Prefixed chars (e.g. `CTL | 'A'`) may be stored in this variable,
 * which should be okay since functions like `ctoec` will keep it unchanged.
 */
int reeat_char = -1;

static inline void ttputs(char *s)
{
	while (*s)
		TTputc(*s++);
}

/* Get a key from the terminal driver, resolve any keyboard macro action */
int tgetc(void)
{
	int c;

	if ((c = reeat_char) != -1) {
		reeat_char = -1;
		return c;
	}

	/* if we are playing a keyboard macro back */
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

		/* don't overrun the buffer */
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

static int transform_csi_1(int ch)
{
	switch (ch) {
	case 0x41:	return CTL | 'P';
	case 0x42:	return CTL | 'N';
	case 0x43:	return CTL | 'F';
	case 0x44:	return CTL | 'B';
	case 0x46:	return CTL | 'E';
	case 0x48:	return CTL | 'A';
	default:	return NULLPROC_KEYS;
	}
}

static int transform_csi_2(int ch)
{
	switch (ch) {
	case 0x33:	return CTL | 'D';
	case 0x35:	return CTL | 'Z';
	case 0x36:	return CTL | 'V';
	default:	return NULLPROC_KEYS;
	}
}

/* Get a command from the keyboard.  Process all applicable prefix keys. */
int getcmd(void)
{
	int c, c2, cmask = 0;

	c = get1key();

	/* process META prefix */

	/* META is not like CTLX, a META may not be a META, it can be a CSI */

	if (c == METAC) {
proc_metac:
		c = get1key();

		/* For VT220, We need to handle CSI (starts with `ESC [`) */
#if VT220
		/* CAUTION: Only parts of CSI cursor commands are handled */
		/* `ESC O` is used by some terminals */
		if (c == '[' || c == 'O') {
			c = get1key();
			if (c >= 0x41 && c <= 0x48) {
				return cmask | transform_csi_1(c);
			} else if (c >= 0x31 && c <= 0x39) {
				c2 = get1key();
				if (c2 == 0x7E)
					return cmask | transform_csi_2(c);
				else
					return NULLPROC_KEYS;
			} else {
				return NULLPROC_KEYS;
			}
		} else if (c == METAC) {
			cmask |= META;
			goto proc_metac;
		}
#endif

		cmask |= META;
		if (c == CTLXC)
			goto proc_ctlxc;
		if (islower(c))
			c ^= DIFCASE;
		return cmask | c;
	}

	/* process CTLX prefix */
	if (c == CTLXC) {
proc_ctlxc:
		c = get1key();
		cmask |= CTLX;
		if (c == METAC)
			goto proc_metac;
		if (islower(c))
			c ^= DIFCASE;
		return cmask | c;
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

		if (expc == ABORTC && quotef == FALSE) {
			ctrlg(FALSE, 0);
			TTflush();
			return ABORT;

		} else if ((c == 0x7F || c == '\b') && quotef == FALSE) {
			if (cpos != 0) {
				ttputs("\b \b");
				--ttcol;

				if (buf[--cpos] < 0x20) {
					ttputs("\b \b");
					--ttcol;
				}
				if (buf[cpos] == '\n') {
					ttputs("\b\b  \b\b");
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

	for (ffp = names; ffp->n_func != NULL; ++ffp) {
		if (strcmp(fname, ffp->n_name) == 0)
			return ffp->n_func;
	}
	return NULL;
}

/* Execute a named command even if it is not bound. */
int namedcmd(int f, int n)
{
	char buf[NSTRING];
	fn_t fn;

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
		strncpy(buf, prompt, 64);
		strcat(buf, " (y/n)? ");
		mlwrite(buf);

		switch (ctoec(tgetc())) {
		case 'Y': case 'y':	return TRUE;
		case 'N': case 'n':	return FALSE;
		case ABORTC:		return ABORT;
		default:		/*ignore*/
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

