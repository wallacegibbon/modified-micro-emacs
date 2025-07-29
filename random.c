#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"

/* Show the line info (the current line number and the total line number) */
int showpos(int flag, int n)
{
	struct line *lp;
	int curline = -1, numlines = 0;

	for (lp = lforw(curbp->b_linep); lp != curbp->b_linep; lp = lforw(lp)) {
		if (lp == curwp->w_dotp)
			curline = numlines;
		++numlines;
	}
	if (curline == -1)
		curline = numlines;

	mlwrite("Position: %d/%d", curline + 1, numlines);
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
	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	c = tgetc();
	if (n < 0)
		return FALSE;
	if (n == 0)
		return TRUE;
	if (c == '\n') {
		do { s = lnewline(); }
		while (s == TRUE && --n);
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

	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	if (n < 0)
		return FALSE;
	if (n == 0)
		return TRUE;

	i = n;
	do { s = lnewline(); }
	while (s == TRUE && --i);

	if (s == TRUE)
		s = backchar(f, n);

	return s;
}

int newline(int f, int n)
{
	int s;

	if (curbp->b_flag & BFRDONLY)
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

static int newline_and_indent_one(void)
{
	int nicol = 0, c, i;

	/* Calculate the leading white space count of current line */
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

	/* Insert leading TABs (and white spaces) */

	if ((i = nicol / 8) != 0) {
		if ((linsert(i, '\t') == FALSE))
			return FALSE;
	}

	if ((i = nicol % 8) != 0)
		return linsert(i, ' ');

	return TRUE;
}

int newline_and_indent(int f, int n)
{

	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	if (n < 0)
		return FALSE;

	while (n--) {
		if (newline_and_indent_one() == FALSE)
			return FALSE;
	}

	return TRUE;
}

/* Delete forward.  If any argument is present, it kills rather than deletes. */
int forwdel(int f, int n)
{
	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	if (n < 0)
		return backdel(f, -n);

	if (f != FALSE) {
		if ((lastflag & CFKILL) == 0)
			kdelete();
		thisflag |= CFKILL;
	}

	return ldelete((long)n, f);
}

/* Delete backward.  If any argument is present, it kills rather than deletes. */
int backdel(int f, int n)
{
	int s = TRUE;
	long nn = 0;

	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	if (n < 0)
		return forwdel(f, -n);

	if (f != FALSE) {
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
	struct line *nextp;
	long chunk;

	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	if (n <= 0)
		return FALSE;

	/* If last command is not a kill, clear the kill buffer */
	if ((lastflag & CFKILL) == 0)
		kdelete();

	thisflag |= CFKILL;

	if (f == FALSE) {
		chunk = llength(curwp->w_dotp) - curwp->w_doto;
		if (chunk == 0)
			chunk = 1;
	} else {
		chunk = llength(curwp->w_dotp) - curwp->w_doto + 1;
		nextp = lforw(curwp->w_dotp);
		while (--n) {
			if (nextp == curbp->b_linep)
				break;
			chunk += llength(nextp) + 1;
			nextp = lforw(nextp);
		}
	}
	return ldelete(chunk, TRUE);
}

/* Yank text back from the kill buffer. */
int yank(int f, int n)
{
	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	if (n < 0)
		return FALSE;

	while (n--) {
		if (linsert_kbuf() == FALSE)
			return FALSE;
	}

	return TRUE;
}

int show_raminfo(int f, int n)
{
#define GB (1024 * 1024 * 1024)
#define MB (1024 * 1024)
#define KB 1024
#define I(v, unit) ((v) / (unit))
#define D(v, unit) ((v) * 10 / (unit) % 10)

	if (envram >= 1000 * MB)
		mlwrite("%d.%dG (%ld)", I(envram, GB), D(envram, GB), envram);
	else if (envram >= 1000 * KB)
		mlwrite("%d.%dM (%ld)", I(envram, MB), D(envram, MB), envram);
	else if (envram >= 100000)
		mlwrite("%d.%dK (%ld)", I(envram, KB), D(envram, KB), envram);
	else
		mlwrite("%ld", envram);

	return TRUE;
#undef D
#undef I
#undef KB
#undef MB
#undef GB
}
