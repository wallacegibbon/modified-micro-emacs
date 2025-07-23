#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include <unistd.h>

static int readpattern(char *prompt, char *apat, int srch);
static int replaces(int kind, int f, int n);
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
static int readpattern(char *prompt, char *apat, int srch)
{
	char tpat[NPAT + 64 /* prompt */ + 5 /* " (", "): " */ + 1];
	int status;

	strncpy_safe(tpat, prompt, 65);
	strcat(tpat, " (");
	strcat(tpat, apat);
	strcat(tpat, "): ");

	/*
	 * Read a pattern.  Either we get one,
	 * or we just get the CR charater, and use the previous pattern.
	 * Then, if it's the search string, make a reversed pattern.
	 * *Then*, make the meta-pattern, if we are defined that way.
	 */
	if ((status = getstring(tpat, tpat, NPAT, ENTERC)) == TRUE) {
		strcpy(apat, tpat);
		if (srch) {	/* If we are doing the search string. */
			rvstrcpy(tap, apat);
			mlenold = matchlen = strlen(apat);
		}
	} else if (status == FALSE && apat[0] != 0) {	/* Old one */
		status = TRUE;
	}
	return status;
}

/* We found the pattern?  Let's save it away. */
void savematch(void)
{
	struct line *curline;	/* line of last match */
	int curoff;		/* offset of last match */
	char *ptr;		/* pointer to last match string */
	unsigned int j;

	if (patmatch != NULL)
		free(patmatch);

	ptr = patmatch = malloc(matchlen + 1);

	if (ptr != NULL) {
		curoff = matchoff;
		curline = matchline;

		for (j = 0; j < matchlen; ++j)
			*ptr++ = nextch(&curline, &curoff, FORWARD);

		*ptr = '\0';
	}
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

int qreplace(int f, int n)
{
	return replaces(TRUE, f, n);
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
 * Search for a string and replace it with another string.
 * Query might be enabled (according to kind).
 *
 * int kind;		Query enabled flag
 */
static int replaces(int kind, int f, int n)
{
	int nlflag, nlrepl, numsub, nummatch, status, c, last_char = 0;
	char tpat[NPAT + 64];

	if (curbp->rdonly)
		return rdonly();
	if (f && n < 0)
		return FALSE;

	/* Ask the user for the text of a pattern. */
	if ((status = readpattern((kind == FALSE ? "Replace" : "Query replace"),
			pat, TRUE)) != TRUE)
		return status;

	/* Ask for the replacement string. */
	if ((status = readpattern("with", rpat, FALSE)) == ABORT)
		return status;

	/*
	 * Set up flags so we can make sure not to do a recursive
	 * replace on the last line.
	 */
	nlflag = (pat[matchlen - 1] == '\n');
	nlrepl = FALSE;

	if (kind) {
		/* Build query replace question string. */
		strcpy(tpat, "Replace '");
		strcat(tpat, pat);
		strcat(tpat, "' with '");
		strcat(tpat, rpat);
		strcat(tpat, "'? ");
	}

	numsub = 0;
	nummatch = 0;

	while ((f == FALSE || n > nummatch) &&
			(nlflag == FALSE || nlrepl == FALSE)) {
		if (!scanner(pat, FORWARD, PTBEG))
			break;	/* all done */

		++nummatch;

		/* Check if we are on the last line. */
		nlrepl = (lforw(curwp->w_dotp) == curwp->w_bufp->b_linep);

		if (kind) {
			mlwrite(tpat, pat, rpat);
qprompt:
			update(TRUE);
			c = tgetc();
			mlwrite("");

			last_char = c;
			switch (c) {
			case 'y':
			case ' ':
				savematch();
				break;

			case 'n':
				forwchar(FALSE, 1);
				continue;

			case '!':
				kind = FALSE;
				break;

			case 0x07: /* ASCII code of ^G is 0x07 ('\a') */
				mlwrite("%d substitutions", numsub);
				return FALSE;

			default:
				TTbeep();
				/* fallthrough */

			case '?':
				mlwrite("(Y)es, (N)o, (!)Do rest, (^G)Abort, (?)Help: ");
				goto qprompt;

			}
		}

		/* Delete the sucker, and insert its replacement. */
		status = delins(matchlen, rpat, TRUE);
		if (status != TRUE)
			return status;

		++numsub;
	}

	if (last_char == 'n')
		backchar(FALSE, 1);

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
