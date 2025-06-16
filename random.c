#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"

/*
 * Display the current position of the cursor, in origin 1 X-Y coordinates,
 * the character that is under the cursor (in hex), and the fraction of the
 * text that is before the cursor.  The displayed column is not the current
 * column, but the column that would be used on an infinite width display.
 */
int showcpos(int f, int n)
{
	long numchars = 0, numlines = 0, predchars = 0, predlines = 0;
	int ratio, col, ecol, saved_o, curchar = 0;
	struct line *lp = lforw(curbp->b_linep);

	while (lp != curbp->b_linep) {
		if (lp == curwp->w_dotp) {
			predlines = numlines;
			predchars = numchars + curwp->w_doto;
			if ((curwp->w_doto) == llength(lp))
				curchar = '\n';
			else
				curchar = lgetc(lp, curwp->w_doto);
		}
		++numlines;
		numchars += llength(lp) + 1;
		lp = lforw(lp);
	}

	if (curwp->w_dotp == curbp->b_linep) {
		predlines = numlines;
		predchars = numchars;
		curchar = 0;
	}

	/* Get real column and end-of-line column. */
	col = getccol(FALSE);
	saved_o = curwp->w_doto;
	curwp->w_doto = llength(curwp->w_dotp);
	ecol = getccol(FALSE);
	curwp->w_doto = saved_o;

	ratio = 0;		/* Ratio before dot. */
	if (numchars != 0)
		ratio = (100L * predchars) / numchars;

	mlwrite("Line %d/%d Col %d/%d Char %D/%D (%d%%) char = 0x%x",
		predlines + 1, numlines + 1, col, ecol,
		predchars, numchars, ratio, curchar);
	return TRUE;
}

/* Return current column.  Stop at first non-blank given TRUE argument. */
int getccol(int bflg)
{
	struct line *lp = curwp->w_dotp;
	int byte_offset = curwp->w_doto;
	int col = 0, i;

	for (i = 0; i < byte_offset; ++i) {
		unsigned char c = lgetc(lp, i);
		if (c != ' ' && c != '\t' && bflg)
			break;
		col = next_col(col, c);
	}
	return col;
}

/*
 * Quote the next character, and insert it into the buffer.  All the characters
 * are taken literally, with the exception of the newline, which always has
 * its line splitting meaning.  The character is always read, even if it is
 * inserted 0 times, for regularity.
 */
int quote(int f, int n)
{
	int s, c;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	c = tgetc();
	if (n < 0)
		return FALSE;
	if (n == 0)
		return TRUE;
	if (c == '\n') {
		do {
			s = lnewline();
		} while (s == TRUE && --n);
		return s;
	}
	return linsert(n, c);
}

/*
 * Open up some blank space.  The basic plan is to insert a bunch of newlines,
 * and then back up over them.
 */
int openline(int f, int n)
{
	int i, s;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return FALSE;
	if (n == 0)
		return TRUE;

	i = n;
	do {
		s = lnewline();
	} while (s == TRUE && --i);

	if (s == TRUE)
		s = backchar(f, n);

	return s;
}

int newline(int f, int n)
{
	int s;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return FALSE;

	while (n--) {
		if ((s = lnewline()) != TRUE)
			return s;
		curwp->w_flag |= WFINS;
	}
	return TRUE;
}

int newline_and_indent(int f, int n)
{
	int nicol, c, i;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return FALSE;

	while (n--) {
		nicol = 0;
		for (i = 0; i < llength(curwp->w_dotp); ++i) {
			c = lgetc(curwp->w_dotp, i);
			if (c != ' ' && c != '\t')
				break;
			if (c == '\t')
				nicol |= TABMASK;
			++nicol;
		}
		if (lnewline() == FALSE)
			return FALSE;
		curwp->w_flag |= WFINS;
		if ((i = nicol / 8) != 0) {
			if ((linsert(i, '\t') == FALSE))
				return FALSE;
		}
		if ((i = nicol % 8) != 0) {
#if (INDENT_NO_SPACE == 1)
			return linsert(1, '\t');
#else
			return linsert(i, ' ');
#endif
		}
	}
	return TRUE;
}

/* Delete forward.  If any argument is present, it kills rather than deletes. */
int forwdel(int f, int n)
{
	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return backdel(f, -n);
	if (f != FALSE) {	/* Really a kill. */
		if ((lastflag & CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}
	return ldelete((long)n, f);
}

/* Delete backwards. */
int backdel(int f, int n)
{
	int s = TRUE;
	long nn = 0;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return forwdel(f, -n);
	if (f != FALSE) {	/* Really a kill. */
		if ((lastflag & CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}

	while (n-- && (backchar(f, 1) == TRUE))
		nn++;

	s = ldelete(nn, f);
	return s;
}

/* Kills from dot to the end of the line */
int killtext(int f, int n)
{
	long chunk;

	if (curbp->b_mode & MDVIEW)
		return rdonly();

	/* If last command is not a kill, clear the kill buffer */
	if ((lastflag & CFKILL) == 0)
		kdelete();

	thisflag |= CFKILL;

	chunk = llength(curwp->w_dotp) - curwp->w_doto;
	if (chunk == 0)
		chunk = 1;

	return ldelete(chunk, TRUE);
}

int setemode(int f, int n)
{
	return adjustmode(TRUE, FALSE);
}

int delmode(int f, int n)
{
	return adjustmode(FALSE, FALSE);
}

int setgmode(int f, int n)
{
	return adjustmode(TRUE, TRUE);
}

int delgmode(int f, int n)
{
	return adjustmode(FALSE, TRUE);
}

/*
 * change the editor mode status
 *
 * int kind;		true = set,          false = delete
 * int global;		true = global flag,  false = current buffer flag
 */
int adjustmode(int kind, int global)
{
	char modname_buf[8];	/* 8 is enough for any mode name */
	char prompt[32];	/* 32 is enough for prompt in this function */
	char *scan;
	int status, i, *pmode;

	if (global) {
		strcpy(prompt, "Global mode to ");
		pmode = &gmode;
	} else {
		strcpy(prompt, "Mode to ");
		pmode = &curbp->b_mode;
	}

	if (kind == TRUE)
		strcat(prompt, "add: ");
	else
		strcat(prompt, "delete: ");

	status = mlreply(prompt, modname_buf, 8 - 1);
	if (status != TRUE)
		return status;

	/* make it uppercase */

	for (scan = modname_buf; *scan != 0; ++scan) {
		if (islower(*scan))
			*scan ^= DIFCASE;
	}

	/* test it against the modes we know */

	for (i = 0; i < NMODES; ++i) {
		if (strcmp(modname_buf, modename[i]) == 0) {
			int val = modevalue[i];
			if (kind == TRUE)
				*pmode |= val;
			else
				*pmode &= ~val;
			if (global == 0)
				update_modelines();
			mlerase();
			return TRUE;
		}
	}

	mlwrite("No such mode!");
	return FALSE;
}
