#include "me.h"

static int readpattern(char *prompt, char *apat);
static int nextch(struct line **pcurline, int *pcuroff, int dir);
static int delins(int dlength, char *instr, int use_meta);

/* "bc" comes from the buffer, "pc" from the pattern. */
static inline int eq(unsigned char bc, unsigned char pc)
{
	if (!exact_search)
		return ensure_upper(bc) == ensure_upper(pc);
	else
		return bc == pc;
}

/* Switch between case-sensitive and case-insensitive. */
int toggle_exact_search(int f, int n)
{
	struct window *wp;

	exact_search = !exact_search;
	for_each_wind(wp)
		curwp->w_flag |= WFMODE;

	return TRUE;
}

/*
Search for a pattern in either direction.  If found, reset the "." to be at
the start or just after the match string, and (perhaps) repaint the display.
*/
int search_next(const char *pattern, int direct, int beg_or_end)
{
	struct line *curline = curwp->w_dotp, *scanline, *matchline;
	int curoff = curwp->w_doto, scanoff, matchoff;
	const char *patptr;
	int c;

	beg_or_end ^= direct;
loop:
	matchline = curline;
	matchoff = curoff;

	if ((c = nextch(&curline, &curoff, direct)) == -1)
		return FALSE;
	if (!eq(c, pattern[0]))
		goto loop;

	scanline = curline;
	scanoff = curoff;
	patptr = pattern;

	while (*++patptr != '\0') {
		if ((c = nextch(&scanline, &scanoff, direct)) == -1)
			return FALSE;
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

/* Replacement buffer can be empty, which deletes the replacement content */
int clear_rpat(int f, int n)
{
	rpat[0] = '\0';
	mlwrite("Replacement buffer is cleared");
	return TRUE;
}

/*
Delete a specified length from the current point then either insert the string
directly, or make use of replacement meta-array.
*/
static int delins(int dlength, char *instr, int use_meta)
{
	int status;

	if ((status = ldelete((long)dlength, FALSE)) == TRUE)
		status = linstr(instr);

	return status;
}

/* Query search for a string and replace it with another string. */
int qreplace(int f, int n)
{
	int numsub, nummatch, interactive, status, dlength, c = 0;

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
		ansibeep();
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

/* Return -1 when it hits the boundry (start of buffer or end of buffer) */
static int nextch(struct line **pcurline, int *pcuroff, int dir)
{
	struct line *b_linep = curwp->w_bufp->b_linep;
	struct line *curline = *pcurline;
	int curoff = *pcuroff, c;

	if (dir == FORWARD) {
		if (curline == b_linep)
			return -1;
		if (curoff == curline->l_used) {
			curline = curline->l_fp;
			curoff = 0;
			c = '\n';
		} else {
			c = curline->l_text[curoff++];
		}
	} else {
		if (curoff == 0) {
			if (curline->l_bp == b_linep)
				return -1;
			curline = curline->l_bp;
			curoff = curline->l_used;
			c = '\n';
		} else {
			c = curline->l_text[--curoff];
		}
	}
	*pcurline = curline;
	*pcuroff = curoff;
	return c;
}
