#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include <unistd.h>

static int readpattern(char *prompt, char *apat, int srch);
static int nextch(struct line **pcurline, int *pcuroff, int dir);

/*
 * Search for a pattern in either direction.  If found, reset the "." to be at
 * the start or just after the match string, and (perhaps) repaint the display.
 */
int scanner(const char *pattern, int direct, int beg_or_end)
{
	struct line *curline = curwp->w_dotp;
	int curoff = curwp->w_doto;
	struct line *scanline;
	const char *patptr;
	int scanoff, c;

	/*
	 * If we are going in reverse, then the 'end' is actually the
	 * beginning of the pattern.  Toggle it.
	 */
	beg_or_end ^= direct;

	while (!boundry(curline, curoff, direct)) {
		matchline = curline;
		matchoff = curoff;

		c = nextch(&curline, &curoff, direct);
		if (eq(c, pattern[0])) {
			scanline = curline;
			scanoff = curoff;
			patptr = pattern;

			while (*++patptr != '\0') {
				c = nextch(&scanline, &scanoff, direct);
				if (!eq(c, *patptr))
					goto fail;
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
fail:
		;/* continue to search */
	}
	return FALSE;
}

/* "bc" comes from the buffer, "pc" from the pattern. */
int eq(unsigned char bc, unsigned char pc)
{
	return ensure_upper(bc) == ensure_upper(pc);
	/*
	return bc == pc;
	*/
}

/*
 * Read a pattern.  Stash it in apat.
 * Apat is not updated if the user types in an empty line.  If the user typed
 * an empty line, and there is no old pattern, it is an error.
 */
static int readpattern(char *prompt, char *apat, int is_search)
{
	char tpat[NPAT + 64 /* prompt */ + 5 /* " (", "): " */ + 1];
	int status;

	strncpy_safe(tpat, prompt, 65);
	strcat(tpat, " (");
	strcat(tpat, apat);
	strcat(tpat, "): ");

	/* Read a pattern.  Either we get one, or we use the previous one */
	if ((status = mlgetstring(tpat, tpat, NPAT, ENTERC)) == TRUE) {
		strcpy(apat, tpat);
		if (is_search) {
			rvstrcpy(tap, apat);
			matchlen = strlen(apat);
		}
	} else if (status == FALSE && apat[0] != 0) {	/* Old one */
		status = TRUE;
	}
	return status;
}

/* Reverse string copy. */
void rvstrcpy(char *rvstr, char *str)
{
	int i;
	str += (i = strlen(str));
	while (i-- > 0)
		*rvstr++ = *--str;

	*rvstr = '\0';
}

/*
 * Delete a specified length from the current point then either insert the
 * string directly, or make use of replacement meta-array.
 */
int delins(int dlength, char *instr, int use_meta)
{
       int status;

       /* Zap what we gotta, and insert its replacement. */
       if ((status = ldelete((long)dlength, FALSE)) != TRUE)
               mlwrite("%%ERROR while deleting");
       else
               status = linstr(instr);

       return status;
}

/*
 * Return value meaning: 0: continue; 1: ignore this one; 2: finish
 */
static int qreplace_interact(int *keep_interactive)
{
	int c;
	if (!*keep_interactive)
		return 0;

	mlwrite("Replace (%s) with (%s)? ", pat, rpat);
qprompt:
	update(TRUE);
	switch ((c = tgetc())) {
	case 'y':
	case ' ':
		return 0;
	case 'n':
		forwchar(FALSE, 1);
		return 1;
	case '!':
		*keep_interactive = 0;
		return 0;
	case 0x07: /* ASCII code of ^G is 0x07 ('\a') */
		return 2;
	default:
		TTbeep();
		mlwrite("(Y)es, (N)o, (!)All, (^G)Abort");
		goto qprompt;
	}
}

/* Query search for a string and replace it with another string. */
int qreplace(int f, int n)
{
	int nlflag, nlrepl, numsub, nummatch, status;
	int keep_interactive = 1, interact_val = 0;

	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	if (f && n < 0)
		return FALSE;

	if ((status = readpattern("Query replace", pat, TRUE)) != TRUE)
		return status;
	if ((status = readpattern("with", rpat, FALSE)) == ABORT)
		return status;

	/*
	 * Set up flags so we can make sure not to do a recursive
	 * replace on the last line.
	 */
	nlflag = (pat[matchlen - 1] == '\n');
	nlrepl = FALSE;

	numsub = 0;
	nummatch = 0;

	while ((f == FALSE || n > nummatch) &&
			(nlflag == FALSE || nlrepl == FALSE)) {
		if (!scanner(pat, FORWARD, PTBEG))
			break;	/* all done */

		++nummatch;

		/* Check if we are on the last line. */
		nlrepl = (lforw(curwp->w_dotp) == curwp->w_bufp->b_linep);

		if ((interact_val = qreplace_interact(&keep_interactive)) == 1)
			continue;
		if (interact_val == 2)
			goto finish;

		/* Do one replacement */
		if ((status = delins(matchlen, rpat, TRUE)) != TRUE)
			return status;

		++numsub;
	}

	/* If it's the last one that we say `n`, back a char */
	if (interact_val == 1)
		backchar(FALSE, 1);

finish:
	mlwrite("%d substitutions", numsub);
	return TRUE;
}

/*
 * Return information depending on whether we may search no further.
 * Beginning of file and end of file are the obvious cases.
 */
int boundry(struct line *curline, int curoff, int dir)
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
