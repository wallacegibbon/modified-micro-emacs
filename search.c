#include "efunc.h"
#include "edef.h"
#include "line.h"
#include <unistd.h>

static int readpattern(char *prompt, char *apat);
static int nextch(struct line **pcurline, int *pcuroff, int dir);
static int boundry(struct line *curline, int curoff, int dir);
static int delins(int dlength, char *instr, int use_meta);

/* "bc" comes from the buffer, "pc" from the pattern. */
static inline int eq(unsigned char bc, unsigned char pc)
{
	/* return bc == pc; */
	return ensure_upper(bc) == ensure_upper(pc);
}

/*
 * Search for a pattern in either direction.  If found, reset the "." to be at
 * the start or just after the match string, and (perhaps) repaint the display.
 */
int search_next(const char *pattern, int direct, int beg_or_end)
{
	struct line *curline = curwp->w_dotp, *scanline, *matchline;
	int curoff = curwp->w_doto, scanoff, matchoff;
	const char *patptr;
	int c;

	/*
	 * If we are going in reverse, then the 'end' is actually the
	 * beginning of the pattern.  Toggle it.
	 */
	beg_or_end ^= direct;

loop:
	if (boundry(curline, curoff, direct))
		return FALSE;

	matchline = curline;
	matchoff = curoff;

	c = nextch(&curline, &curoff, direct);
	if (!eq(c, pattern[0]))
		goto loop;

	scanline = curline;
	scanoff = curoff;
	patptr = pattern;

	while (*++patptr != '\0') {
		c = nextch(&scanline, &scanoff, direct);
		if (!eq(c, *patptr))
			goto loop;
	}

	/* A SUCCESSFULL MATCH!!! */

	if (beg_or_end == PTEND) {
		curwp->w_dotp = scanline;
		curwp->w_doto = scanoff;
	} else {
		curwp->w_dotp = matchline;
		curwp->w_doto = matchoff;
	}

	curwp->w_flag |= WFMOVE;
	return TRUE;
}

/* If the user did not give a string, use the old one (if there is one) */
static int readpattern(char *prompt, char *apat)
{
	char tpat[NPAT];
	int status;

	status = mlgetstring(tpat, NPAT, ENTERC, "%s (%s): ", prompt, apat);
	if (status == TRUE)
		strcpy(apat, tpat);
	else if (status == FALSE && apat[0] != 0)	/* Use old pattern */
		status = TRUE;

	return status;
}

/*
 * Delete a specified length from the current point then either insert the
 * string directly, or make use of replacement meta-array.
 */
static int delins(int dlength, char *instr, int use_meta)
{
	int status;
	if ((status = ldelete((long)dlength, FALSE)) != TRUE)
		mlwrite("ERROR while deleting");
	else
		status = linstr(instr);
	return status;
}

/* Query search for a string and replace it with another string. */
int qreplace(int f, int n)
{
	int numsub, nummatch, interactive, status, dlength, c = 0;

	if (curbp->b_flag & BFRDONLY)
		return rdonly();

	if ((status = readpattern("Query replace", pat)) != TRUE)
		return status;
	if ((status = readpattern("with", rpat)) == ABORT)
		return status;

	dlength = strlen(pat);
	numsub = 0;
	nummatch = 0;
	interactive = 1;

replace_loop:
	if (curwp->w_dotp == curwp->w_bufp->b_linep)
		goto loop_done;
	if (!search_next(pat, FORWARD, PTBEG))
		goto loop_done;

	++nummatch;
	if (!interactive)
		goto do_replace;

	mlwrite("Replace (%s) with (%s)? ", pat, rpat);
qprompt:
	update(TRUE);
	switch ((c = tgetc())) {
	case '!':
		interactive = 0;
		/* fall through */
	case ' ':
		goto do_replace;
	case 'n':
		forwchar(FALSE, 1);
		goto replace_loop;
	case 0x07: /* ASCII code of ^G is 0x07 ('\a') */
		goto finish;
	default:
		TTbeep();
		mlwrite("(SPACE)Ok, (N)ext, (!)All, (^G)Done");
		goto qprompt;
	}

do_replace:
	if ((status = delins(dlength, rpat, TRUE)) != TRUE)
		return status;

	++numsub;
	goto replace_loop;

loop_done:
	/* If it's the last one that we say `n`, back a char */
	if (c == 'n')
		backchar(FALSE, 1);

finish:
	mlwrite("%d substitutions (in %d matches)", numsub, nummatch);
	return TRUE;
}

/*
 * Return information depending on whether we may search no further.
 * Beginning of file and end of file are the obvious cases.
 */
static int boundry(struct line *curline, int curoff, int dir)
{
	if (dir == FORWARD)
		return (curoff == llength(curline)) &&
			(lforw(curline) == curbp->b_linep);
	else
		return (curoff == 0) &&
			(lback(curline) == curbp->b_linep);
}

static int nextch(struct line **pcurline, int *pcuroff, int dir)
{
	struct line *curline = *pcurline;
	int curoff = *pcuroff;
	int c;

	if (dir == FORWARD) {
		if (curoff == llength(curline)) {
			curline = lforw(curline);
			curoff = 0;
			c = '\n';
		} else {
			c = lgetc(curline, curoff++);
		}
	} else {
		if (curoff == 0) {
			curline = lback(curline);
			curoff = llength(curline);
			c = '\n';
		} else {
			c = lgetc(curline, --curoff);
		}

	}
	*pcurline = curline;
	*pcuroff = curoff;
	return c;
}
