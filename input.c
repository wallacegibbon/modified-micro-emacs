#include "me.h"

/*
CAUTION: Prefixed chars (e.g. `CTL | 'A'`) may be stored in this variable,
which should be okay since functions like `ctoec` will keep it unchanged.
*/

int reeat_char = -1;

/* Gets a key from the terminal driver, resolve any keyboard macro action */
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
	c = ttgetc();
	if (kbdmode == RECORD) {
		*kbdptr++ = c;
		kbdend = kbdptr;
		if (kbdptr == &kbdm[NKBDM - 1]) {
			kbdmode = STOP;
			ansibeep();
		}
	}
	return c;
}

/* Gets one keystroke.  The only prefixs legal here are the CTL prefixes. */
int get1key(void)
{
	return ctoec(tgetc());
}

static int csi_input_map[][2] = {
	{ 'A', CTL | 'P' }, { 'B', CTL | 'N' },
};

static int csi_map(int c)
{
	int count = sizeof(csi_input_map) / sizeof(csi_input_map[0]);
	if (count > 0) {
		while (count--) {
			if (csi_input_map[count][0] == c)
				return csi_input_map[count][1];
		}
	}
	return CUSTOMFLAG | 255;	/* bound to nullproc */
}

/* Gets a command from the keyboard.  CSI input sequence is handled here */
int getcmd(void)
{
	int cmask = 0, c;
	c = get1key();

	/*
	ESCAPE is not used directly, But escape sequences like CSI and SS3
	should be handled here even if we do not use them.
	*/
	if (c == ESCAPEC) {
escape_loop:
		c = get1key();
		if (c == ESCAPEC)
			goto escape_loop;
		if (c == '[' || c == 'O') {
			do { c = get1key(); }
			while (isdigit(c) || c == ';');
			return csi_map(c);
		}
	}
	if (c == CTLXC && cmask == 0) {
		cmask |= CTLX;
		c = get1key();
		if (c == ESCAPEC)
			goto escape_loop;
	}
	/* Ignore the mask when it's Ctrl-g */
	if (c == ABORTC)
		return c;
	if (cmask)
		return cmask | ensure_upper(c);
	else
		return c;
}

int mlgetstring(char *buf, int nbuf, int eolchar, const char *fmt, ...)
{
	int cpos = 0, c, expc;
	va_list ap;

	va_start(ap, fmt);
	mlvwrite(fmt, ap);
	va_end(ap);

	/*
	Initialize buf so that we can tell whether user have input something.
	fmt and buf can be the same address, so we do this after mlwrite.
	*/
	buf[0] = '\0';

char_loop:
	ttflush();
	c = ectoc(expc = get1key());
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
		buf[cpos] = '\0';
		goto char_loop;
	}

char_append:
	buf[cpos++] = c;
	buf[cpos] = '\0';
	ttcol += put_c(c, ttputc);
	if (cpos < nbuf - 1)
		goto char_loop;

	mlwrite("Input too long");

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
	mlerase();
	return c;
}

int mlreply(char *prompt, char *buf, int nbuf)
{
	return mlgetstring(buf, nbuf, ENTERC, "%s", prompt);
}

int mlyesno(char *prompt)
{
	for (;;) {
		switch (mlgetchar("%s (y/n)? ", prompt)) {
		case 'y':
			return TRUE;
		case 'n':
			return FALSE;
		case ABORTC:
			return ABORT;
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
