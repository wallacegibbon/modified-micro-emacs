#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"

/*
 * Move the cursor backward by "n" words.  All of the details of motion are
 * performed by the "backchar" and "forwchar" routines.  Error if you try to
 * move beyond the buffers.
 */
int backword(int f, int n)
{
	if (n < 0)
		return forwword(f, -n);
	if (backchar(FALSE, 1) == FALSE)
		return FALSE;
	while (n--) {
		while (inword() == FALSE) {
			if (backchar(FALSE, 1) == FALSE)
				return FALSE;
		}
		while (inword() == TRUE) {
			if (backchar(FALSE, 1) == FALSE)
				return FALSE;
		}
	}
	return forwchar(FALSE, 1);
}

/*
 * Move the cursor forward by the specified number of words.  All of the motion
 * is done by "forwchar".  Error if you try and move beyond the buffer's end.
 */
int forwword(int f, int n)
{
	if (n < 0)
		return backword(f, -n);
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
		while (inword() == TRUE) {
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
	}
	return TRUE;
}

/*
 * Move the cursor forward by the specified number of words.  As you move,
 * convert any characters to upper case.  Error if you try and move beyond the
 * end of the buffer.
 */
int upperword(int f, int n)
{
	int c;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return FALSE;

	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
		while (inword() == TRUE) {
			c = lgetc(curwp->w_dotp, curwp->w_doto);
			if (islower(c)) {
				c ^= DIFCASE;
				lputc(curwp->w_dotp, curwp->w_doto, c);
				lchange(WFHARD);
			}
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
	}
	return TRUE;
}

/*
 * Move the cursor forward by the specified number of words.  As you move
 * convert characters to lower case.  Error if you try and move over the end of
 * the buffer.
 */
int lowerword(int f, int n)
{
	int c;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return FALSE;

	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
		while (inword() == TRUE) {
			c = lgetc(curwp->w_dotp, curwp->w_doto);
			if (isupper(c)) {
				c ^= DIFCASE;
				lputc(curwp->w_dotp, curwp->w_doto, c);
				lchange(WFHARD);
			}
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
	}
	return TRUE;
}

/*
 * Move the cursor forward by the specified number of words.  As you move
 * convert the first character of the word to upper case, and subsequent
 * characters to lower case.  Error if you try and move past the end of the
 * buffer.
 */
int capword(int f, int n)
{
	int c;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return FALSE;

	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
		}
		if (inword() == TRUE) {
			c = lgetc(curwp->w_dotp, curwp->w_doto);
			if (islower(c)) {
				c ^= DIFCASE;
				lputc(curwp->w_dotp, curwp->w_doto, c);
				lchange(WFHARD);
			}
			if (forwchar(FALSE, 1) == FALSE)
				return FALSE;
			while (inword() == TRUE) {
				c = lgetc(curwp->w_dotp, curwp->w_doto);
				if (isupper(c)) {
					c ^= DIFCASE;
					lputc(curwp->w_dotp, curwp->w_doto, c);
					lchange(WFHARD);
				}
				if (forwchar(FALSE, 1) == FALSE)
					return FALSE;
			}
		}
	}
	return TRUE;
}

/* Kill forward by "n" words. */
int delfword(int f, int n)
{
	struct line *dotp = curwp->w_dotp;
	int doto = curwp->w_doto;
	long size;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n < 0)
		return FALSE;

	/* Clear the kill buffer if last command wasn't a kill */
	if ((lastflag & CFKILL) == 0)
		kdelete();

	thisflag |= CFKILL;

	size = 0;
	while (n--) {
		while (inword() == FALSE) {
			if (forwchar(FALSE, 1) == FALSE)
				goto ready;
			++size;
		}
		while (inword() == TRUE) {
			if (forwchar(FALSE, 1) == FALSE)
				goto ready;
			++size;
		}
	}

ready:
	/* restore the original position and delete the words */
	curwp->w_dotp = dotp;
	curwp->w_doto = doto;
	return ldelete(size, TRUE);
}

/* Kill backwards by "n" words. */
int delbword(int f, int n)
{
	long size;
	int s = FALSE;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if (n <= 0)
		return FALSE;

	/* Clear the kill buffer if last command wasn't a kill */
	if ((lastflag & CFKILL) == 0)
		kdelete();

	thisflag |= CFKILL;

	if (backchar(FALSE, 1) == FALSE)
		return FALSE;
	size = 1;
	while (n--) {
		while (inword() == FALSE) {
			if ((s = backchar(FALSE, 1)) == FALSE)
				goto ready;
			++size;
		}
		while (inword() == TRUE) {
			if ((s = backchar(FALSE, 1)) == FALSE)
				goto ready;
			++size;
		}
	}
	if (s == TRUE) {
		forwchar(FALSE, 1);
		--size;
	}
ready:
	return ldelete(size, TRUE);
}

/*
 * Return TRUE if the character at dot is a character that is considered to be
 * part of a word.  The word character list is hard coded.  Should be setable.
 */
int inword(void)
{
	int c;
	if (curwp->w_doto == llength(curwp->w_dotp))
		return FALSE;
	c = lgetc(curwp->w_dotp, curwp->w_doto);
	if (isalpha(c))
		return TRUE;
	if (c >= '0' && c <= '9')
		return TRUE;
	return FALSE;
}
