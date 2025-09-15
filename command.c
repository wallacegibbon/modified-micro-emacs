#include "me.h"

/* Convert character index/offset into terminal column number of this line. */
static int
get_col(e_Line *lp, int offset)
{
	int col = 0, i = 0;
	while (i < offset)
		col = next_col(col, lp->l_text[i++]);
	return col;
}

/* Convert terminal column number of this line into character index/offset. */
static int
get_idx(e_Line *lp, int col)
{
	int c = 0, i = 0, len = lp->l_used;
	while (i < len) {
		if ((c = next_col(c, lp->l_text[i])) > col)
			break;
		else
			++i;
	}
	return i;
}

/* Goto the beginning of the buffer */
int
gotobob(int f, int n)
{
	curwp->w_dotp = curwp->w_bufp->b_linep->l_fp;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/* Move to the end of the buffer.  Dot is always put at the end of the file */
int
gotoeob(int f, int n)
{
	curwp->w_dotp = curwp->w_bufp->b_linep;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

/* Move the cursor to the beginning of the current line. */
int
gotobol(int f, int n)
{
	curwp->w_doto = 0;
	return TRUE;
}

/* Move the cursor to the end of the current line */
int
gotoeol(int f, int n)
{
	curwp->w_doto = curwp->w_dotp->l_used;
	return TRUE;
}

int
backchar(int f, int n)
{
	e_Line *lp;

	if (n < 0)
		return forwchar(f, -n);
	while (n--) {
		if (curwp->w_doto == 0) {
			lp = curwp->w_dotp->l_bp;
			if (lp == curwp->w_bufp->b_linep)
				return FALSE;
			curwp->w_dotp = lp;
			curwp->w_doto = lp->l_used;
			curwp->w_flag |= WFMOVE;
		} else {
			--curwp->w_doto;
		}
	}
	return TRUE;
}

int
forwchar(int f, int n)
{
	if (n < 0)
		return backchar(f, -n);
	while (n--) {
		if (curwp->w_doto == curwp->w_dotp->l_used) {
			if (curwp->w_dotp == curwp->w_bufp->b_linep)
				return FALSE;
			curwp->w_dotp = curwp->w_dotp->l_fp;
			curwp->w_doto = 0;
			curwp->w_flag |= WFMOVE;
		} else {
			++curwp->w_doto;
		}
	}
	return TRUE;
}

int
gotoline(int f, int n)
{
	char arg[NSTRING];
	int status;

	if (f == FALSE) {
		if ((status = mlreply("GO: ", arg, NSTRING)) != TRUE) {
			mlwrite("Aborted");
			return status;
		}
		n = atoi(arg);
	}
	/* If a bogus argument was passed, then returns false. */
	if (n < 0)
		return FALSE;

	gotobob(f, n);
	return forwline(f, n - 1);
}

int
forwline(int f, int n)
{
	e_Line *lp;

	if (n < 0)
		return backline(f, -n);

	thisflag |= CFCPCN;
	if (curwp->w_dotp == curwp->w_bufp->b_linep)
		return FALSE;

	/* If the last command was not a line move, update the goal */
	if (!(lastflag & CFCPCN))
		curgoal = get_col(curwp->w_dotp, curwp->w_doto);

	lp = curwp->w_dotp;
	while (n-- && lp != curwp->w_bufp->b_linep)
		lp = lp->l_fp;

	curwp->w_dotp = lp;
	curwp->w_doto = get_idx(lp, curgoal);
	curwp->w_flag |= WFMOVE;
	return TRUE;
}

int
backline(int f, int n)
{
	e_Line *lp;

	if (n < 0)
		return forwline(f, -n);

	thisflag |= CFCPCN;
	if (curwp->w_dotp->l_bp == curwp->w_bufp->b_linep)
		return FALSE;

	/* If the last command was not a line move, update the goal */
	if (!(lastflag & CFCPCN))
		curgoal = get_col(curwp->w_dotp, curwp->w_doto);

	lp = curwp->w_dotp;
	while (n-- && lp->l_bp != curwp->w_bufp->b_linep)
		lp = lp->l_bp;

	curwp->w_dotp = lp;
	curwp->w_doto = get_idx(lp, curgoal);
	curwp->w_flag |= WFMOVE;
	return TRUE;
}

int
forwpage(int f, int n)
{
	e_Line *lp;

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
	while (n-- && lp != curwp->w_bufp->b_linep)
		lp = lp->l_fp;
	curwp->w_linep = lp;
	curwp->w_dotp = lp;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

int
backpage(int f, int n)
{
	e_Line *lp;

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
	while (n-- && lp->l_bp != curwp->w_bufp->b_linep)
		lp = lp->l_bp;
	curwp->w_linep = lp;
	curwp->w_dotp = lp;
	curwp->w_doto = 0;
	curwp->w_flag |= WFHARD;
	return TRUE;
}

int
setmark(int f, int n)
{
	curwp->w_markp = curwp->w_dotp;
	curwp->w_marko = curwp->w_doto;
	mlwrite("Mark set");
	return TRUE;
}

/* Swap the values of "." and "mark" in the current window. */
int
swapmark(int f, int n)
{
	e_Line *odotp;
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

int
show_misc_info(int f, int n)
{
	e_Line *lp;
	int curline = -1, numlines = 0;

	lp = curwp->w_bufp->b_linep->l_fp;
	while (lp != curwp->w_bufp->b_linep) {
		if (lp == curwp->w_dotp)
			curline = numlines;
		lp = lp->l_fp;
		++numlines;
	}
	if (curline == -1)
		curline = numlines;

	mlwrite("Row: %d/%d, Column: %d (%d), Malloc: %ld",
		curline + 1, numlines, get_col(curwp->w_dotp, curwp->w_doto),
		curwp->w_doto, envram);

	return TRUE;
}

/*
Quote the next character, and insert it into the buffer.  All the characters
are taken literally, with the exception of the newline, which always has its
line splitting meaning.  The character is always read, even if it is inserted 0
times, for regularity.
*/
int
quote(int f, int n)
{
	int c = tgetc();
	if (n < 0)
		return FALSE;

	return linsert(n, c);
}

int
newline(int f, int n)
{
	if (n < 0)
		return FALSE;

	return linsert(n, '\n');
}

static int
newline_and_indent_one(void)
{
	int nicol = 0, c, i;

	/* Calculate the leading white space count of current line */
	for (i = 0; i < curwp->w_dotp->l_used; ++i) {
		c = curwp->w_dotp->l_text[i];
		if (c != ' ' && c != '\t')
			break;
		if (c == '\t')
			nicol |= TABMASK;
		++nicol;
	}

	if (linsert(1, '\n') == FALSE)
		return FALSE;

	/* Insert leading TABs (and white spaces) */

	if ((i = nicol / 8) != 0) {
		if ((linsert(i, '\t') == FALSE))
			return FALSE;
	}

	if ((i = nicol % 8) != 0)
		return linsert(i, ' ');

	return TRUE;
}

int
newline_and_indent(int f, int n)
{
	if (n < 0)
		return FALSE;

	while (n--) {
		if (newline_and_indent_one() == FALSE)
			return FALSE;
	}

	return TRUE;
}

int
forwdel(int f, int n)
{
	if (n < 0)
		return backdel(f, -n);

	return ldelete((long)n, FALSE);
}

int
backdel(int f, int n)
{
	long nn = 0;
	if (n < 0)
		return forwdel(f, -n);
	while (n-- && (backchar(f, 1) == TRUE))
		nn++;

	return ldelete(nn, FALSE);
}

/* Yank text back from the kill buffer. */
int
yank(int f, int n)
{
	if (n < 0)
		return FALSE;
	while (n--) {
		if (linsert_kbuf() == FALSE)
			return FALSE;
	}

	return TRUE;
}

/* Kills from dot to the end of the line */
int
killtext(int f, int n)
{
	e_Line *nextp;
	long chunk;

	if (n <= 0)
		return FALSE;

	/* If last command is not a kill, clear the kill buffer */
	if (!(lastflag & CFKILL))
		kdelete();

	thisflag |= CFKILL;

	if (f == FALSE) {
		chunk = curwp->w_dotp->l_used - curwp->w_doto;
		if (chunk == 0)
			chunk = 1;
	} else {
		chunk = curwp->w_dotp->l_used - curwp->w_doto + 1;
		nextp = curwp->w_dotp->l_fp;
		while (--n) {
			if (nextp == curwp->w_bufp->b_linep)
				break;
			chunk += nextp->l_used + 1;
			nextp = nextp->l_fp;
		}
	}
	return ldelete(chunk, TRUE);
}

/*
This routine figures out the bounds of the region in the current window, and
fills in the fields of the "e_Region" structure pointed to by "rp".
Because the dot and mark are usually very close together, we scan outward from
dot looking for mark.
*/
static int
getregion(e_Region *rp)
{
	e_Line *flp, *blp, *tmplp;
	long fsize, bsize;

	if (curwp->w_markp == NULL) {
		mlwrite("No mark set in this window");
		return FALSE;
	}
	if (curwp->w_dotp == curwp->w_markp) {
		rp->r_linep = curwp->w_dotp;
		if (curwp->w_doto < curwp->w_marko) {
			rp->r_offset = curwp->w_doto;
			rp->r_size = (long)(curwp->w_marko - curwp->w_doto);
		} else {
			rp->r_offset = curwp->w_marko;
			rp->r_size = (long)(curwp->w_doto - curwp->w_marko);
		}
		return TRUE;
	}
	blp = curwp->w_dotp;
	bsize = (long)curwp->w_doto;
	flp = curwp->w_dotp;
	fsize = (long)(flp->l_used - curwp->w_doto + 1);
	tmplp = curwp->w_bufp->b_linep;
	while (flp != tmplp || blp->l_bp != tmplp) {
		if (flp != tmplp) {
			flp = flp->l_fp;
			if (flp == curwp->w_markp) {
				rp->r_linep = curwp->w_dotp;
				rp->r_offset = curwp->w_doto;
				rp->r_size = fsize + curwp->w_marko;
				return TRUE;
			}
			fsize += flp->l_used + 1;
		}
		if (blp->l_bp != tmplp) {
			blp = blp->l_bp;
			bsize += blp->l_used + 1;
			if (blp == curwp->w_markp) {
				rp->r_linep = blp;
				rp->r_offset = curwp->w_marko;
				rp->r_size = bsize - curwp->w_marko;
				return TRUE;
			}
		}
	}
	mlwrite("Bug: lost mark");
	return FALSE;
}

/*
Kill the region.  Ask "getregion" to figure out the bounds of the region.
Move "." to the start, and kill the characters.
*/
int
killregion(int f, int n)
{
	e_Region region;
	int s;

	if ((s = getregion(&region)) != TRUE)
		return s;

	/* Always clear kbuf, we don't want to mess up region copying */
	kdelete();
	thisflag |= CFKILL;

	curwp->w_dotp = region.r_linep;
	curwp->w_doto = region.r_offset;
	return ldelete(region.r_size, TRUE);
}

/*
Copy all of the characters in the region to the kill buffer.
Don't move dot at all.  This is a bit like a kill region followed by a yank.
*/
int
copyregion(int f, int n)
{
	e_Line *linep;
	e_Region region;
	int loffs, s;

	if ((s = getregion(&region)) != TRUE)
		return s;

	/* Always clear kbuf, we don't want to mess up region copying */
	kdelete();
	thisflag |= CFKILL;

	linep = region.r_linep;
	loffs = region.r_offset;
	while (region.r_size--) {
		if (loffs == linep->l_used) {
			if ((s = kinsert('\n')) != TRUE)
				return s;
			linep = linep->l_fp;
			loffs = 0;
		} else {
			if ((s = kinsert(linep->l_text[loffs])) != TRUE)
				return s;
			++loffs;
		}
	}
	mlwrite("Region copied");
	return TRUE;
}

static int
toggle_region_case(int start, int end)
{
	e_Line *linep;
	e_Region region;
	int loffs, c, s;

	/* linsert or ldelete is not invoked, rdonly check is necessary here */
	if (curwp->w_bufp->b_flag & BFRDONLY)
		return rdonly();

	if ((s = getregion(&region)) != TRUE)
		return s;
	lchange(WFHARD);
	linep = region.r_linep;
	loffs = region.r_offset;
	while (region.r_size--) {
		if (loffs == linep->l_used) {
			linep = linep->l_fp;
			loffs = 0;
		} else {
			c = linep->l_text[loffs];
			if (c >= start && c <= end)
				linep->l_text[loffs] = c ^ DIFCASE;
			++loffs;
		}
	}
	return TRUE;
}

/* Zap all of the upper case characters in the region to lower case. */
int
lowerregion(int f, int n)
{
	return toggle_region_case('A', 'Z');
}

/* Zap all of the lower case characters in the region to upper case. */
int
upperregion(int f, int n)
{
	return toggle_region_case('a', 'z');
}

/* This command will be executed when terminal window is resized */
int
terminal_reinit(int f, int n)
{
	vtdeinit();
	vtinit();
	mlwrite("Terminal size: %dx%d", term_ncol, term_nrow + 1);
	rebuild_windows();
	return TRUE;
}

/* Begin a keyboard macro. */
int
ctlxlp(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite("Macro is already active");
		return FALSE;
	}
	mlwrite("Start macro");
	kbdptr = kbdm;
	kbdend = kbdptr;
	kbdmode = RECORD;
	return TRUE;
}

/* End keyboard macro. */
int
ctlxrp(int f, int n)
{
	if (kbdmode == STOP) {
		mlwrite("Macro is not active");
		return FALSE;
	}
	if (kbdmode == RECORD) {
		mlwrite("End macro");
		kbdmode = STOP;
	}
	return TRUE;
}

/* Execute a macro. */
int
ctlxe(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite("Macro already active");
		return FALSE;
	}
	if (n <= 0)
		return TRUE;
	kbdrep = n;
	kbdmode = PLAY;
	kbdptr = kbdm;
	return TRUE;
}

/*
Abort.  Kill off any keyboard macro, etc., that is in progress.  Sometimes
called as a routine, to do general aborting of stuff.
*/
int
ctrlg(int f, int n)
{
	ansibeep();
	kbdmode = STOP;
	mlwrite("Aborted");
	return ABORT;
}

int
suspend(int f, int n)
{
	suspend_self();
	return TRUE;
}

int
nullproc(int f, int n)
{
	return TRUE;
}

/* Non command helper functions */

int
rdonly(void)
{
	ansibeep();
	mlwrite("Illegal in read-only mode");
	return FALSE;
}
