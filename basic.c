#include "efunc.h"
#include "edef.h"
#include "line.h"

/*
 * Given a pointer to a struct line, and the current cursor goal column,
 * return the best choice for the offset.
 */
static int getgoal(struct line *lp)
{
	int col = 0, dbo, len;
	for (dbo = 0, len = llength(lp); dbo != len; dbo++) {
		int newcol = next_col(col, lgetc(lp, dbo));
		if (newcol > curgoal)
			break;
		col = newcol;
	}
	return dbo;
}

/* Move the cursor to the beginning of the current line. */
int gotobol(int f, int n)
{
	curwp->w_doto = 0;
	return TRUE;
}

int backchar(int f, int n)
{
	struct line *lp;

	if (n < 0)
		return forwchar(f, -n);
	while (n--) {
		if (curwp->w_doto == 0) {
			if ((lp = lback(curwp->w_dotp)) == curbp->b_linep)
				return FALSE;
			curwp->w_dotp = lp;
			curwp->w_doto = llength(lp);
			curwp->w_flag |= WFMOVE;
		} else {
			--curwp->w_doto;
		}
	}
	return TRUE;
}

/* Move the cursor to the end of the current line */
int gotoeol(int f, int n)
{
	curwp->w_doto = llength(curwp->w_dotp);
	return TRUE;
}

int forwchar(int f, int n)
{
	if (n < 0)
		return backchar(f, -n);
	while (n--) {
		int len = llength(curwp->w_dotp);
		if (curwp->w_doto == len) {
			if (curwp->w_dotp == curbp->b_linep)
				return FALSE;
			curwp->w_dotp = lforw(curwp->w_dotp);
			curwp->w_doto = 0;
			curwp->w_flag |= WFMOVE;
		} else {
			++curwp->w_doto;
		}
	}
	return TRUE;
}

int gotoline(int f, int n)
{
	char arg[NSTRING];
	int status;

	/* Get an argument if one doesnt exist. */
	if (f == FALSE) {
		if ((status = mlreply("Line to GO: ", arg, NSTRING)) != TRUE) {
			mlwrite("(Aborted)");
			return status;
		}
		n = atoi(arg);
	}
        /* Handle the case where `me` is called like this: me filename + */
	if (n == 0)
		return gotoeob(f, n);

	/* If a bogus argument was passed, then returns false. */
	if (n < 0)
		return FALSE;

	/* First, we go to the begin of the buffer. */
	gotobob(f, n);
	return forwline(f, n - 1);
}

/* Goto the beginning of the buffer */
int gotobob(int f, int n)
{
	curwp->w_dotp = lforw(curbp->b_linep);
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/* Move to the end of the buffer.  Dot is always put at the end of the file */
int gotoeob(int f, int n)
{
	curwp->w_dotp = curbp->b_linep;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

int forwline(int f, int n)
{
	struct line *lp;

	if (n < 0)
		return backline(f, -n);

	if (curwp->w_dotp == curbp->b_linep)
		return FALSE;

	/* if the last command was not note a line move */
	if (!(lastflag & CFCPCN))
		curgoal = getccol(FALSE);

	thisflag |= CFCPCN;
	lp = curwp->w_dotp;
	while (n-- && lp != curbp->b_linep)
		lp = lforw(lp);

	curwp->w_dotp = lp;
	curwp->w_doto = getgoal(lp);
	curwp->w_flag |= WFMOVE;
	return TRUE;
}

int backline(int f, int n)
{
	struct line *lp;

	if (n < 0)
		return forwline(f, -n);

	if (lback(curwp->w_dotp) == curbp->b_linep)
		return FALSE;

	/* if the last command was not note a line move */
	if (!(lastflag & CFCPCN))
		curgoal = getccol(FALSE);

	thisflag |= CFCPCN;
	lp = curwp->w_dotp;
	while (n-- && lback(lp) != curbp->b_linep)
		lp = lback(lp);

	curwp->w_dotp = lp;
	curwp->w_doto = getgoal(lp);
	curwp->w_flag |= WFMOVE;
	return TRUE;
}

int forwpage(int f, int n)
{
	struct line *lp;

	if (f == FALSE) {
		n = curwp->w_ntrows - 2;
		if (n <= 0)	/* Forget the overlap on tiny window. */
			n = 1;
	} else if (n < 0) {
		return backpage(f, -n);
	} else {
		n *= curwp->w_ntrows;
	}

	lp = curwp->w_linep;
	while (n-- && lp != curbp->b_linep)
		lp = lforw(lp);
	curwp->w_linep = lp;
	curwp->w_dotp = lp;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD | WFKILLS;
	return TRUE;
}

int backpage(int f, int n)
{
	struct line *lp;

	if (f == FALSE) {
		n = curwp->w_ntrows - 2;
		if (n <= 0)	/* Don't blow up on tiny window. */
			n = 1;
	} else if (n < 0) {
		return forwpage(f, -n);
	} else {
		n *= curwp->w_ntrows;
	}

	lp = curwp->w_linep;
	while (n-- && lback(lp) != curbp->b_linep)
		lp = lback(lp);
	curwp->w_linep = lp;
	curwp->w_dotp = lp;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD | WFINS;
	return TRUE;
}

int setmark(int f, int n)
{
	curwp->w_markp = curwp->w_dotp;
	curwp->w_marko = curwp->w_doto;
	mlwrite("(Mark set)");
	return TRUE;
}

/* Swap the values of "." and "mark" in the current window */
int swapmark(int f, int n)
{
	struct line *odotp;
	int odoto;

	if (curwp->w_markp == NULL) {
		mlwrite("No mark in this window");
		return FALSE;
	}
	odotp = curwp->w_dotp;
	odoto = curwp->w_doto;
	curwp->w_dotp = curwp->w_markp;
	curwp->w_doto = curwp->w_marko;
	curwp->w_markp = odotp;
	curwp->w_marko = odoto;
	curwp->w_flag |= WFMOVE;
	return TRUE;
}
