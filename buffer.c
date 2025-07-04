#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include "wrapper.h"

/*
 * Attach a buffer to a window.
 * The values of dot and mark come from the buffer if the use count is 0.
 * Otherwise, they come from some other window.
 */
int usebuffer(int f, int n)
{
	struct buffer *bp;
	char bufn[NBUFN];
	int s;

	if ((s = mlreply("Use buffer: ", bufn, NBUFN)) != TRUE)
		return s;
	if ((bp = bfind(bufn, TRUE, 0)) == NULL)
		return FALSE;
	return swbuffer(bp);
}

int lastbuffer(int f, int n)
{
	if (prevbp != NULL)
		return swbuffer(prevbp);
	else
		return FALSE;
}

int nextbuffer(int f, int n)
{
	struct buffer *bp = NULL, *bbp;

	if (f == FALSE)
		n = 1;
	if (n < 1)
		return FALSE;

	bbp = curbp;
	while (n-- > 0) {
		bp = bbp->b_bufp;

		/* cycle through the buffers to find an eligable one */
		while (bp == NULL || bp->b_flag & BFINVS) {
			if (bp == NULL)
				bp = bheadp;
			else
				bp = bp->b_bufp;

			/* don't get caught in an infinite loop! */
			if (bp == bbp)
				return FALSE;
		}
		bbp = bp;
	}
	return swbuffer(bp);
}

/* make buffer BP current. */
int swbuffer(struct buffer *bp)
{
	struct window *wp;

	if (--curbp->b_nwnd == 0) {	/* Last use. */
		curbp->b_dotp = curwp->w_dotp;
		curbp->b_doto = curwp->w_doto;
		curbp->b_markp = curwp->w_markp;
		curbp->b_marko = curwp->w_marko;
	}

	prevbp = curbp;
	curbp = bp;

	if (curbp->b_active != TRUE) {	/* buffer not active yet */
		/* read it in and activate it */
		readin(curbp->b_fname, TRUE);
		curbp->b_dotp = lforw(curbp->b_linep);
		curbp->b_doto = 0;
		curbp->b_active = TRUE;
		curbp->b_mode |= gmode;
	}
	curwp->w_bufp = bp;
	curwp->w_linep = bp->b_linep;	/* For macros, ignored. */
	curwp->w_flag |= WFMODE | WFFORCE | WFHARD;	/* Quite nasty. */
	if (bp->b_nwnd++ == 0) {	/* First use. */
		curwp->w_dotp = bp->b_dotp;
		curwp->w_doto = bp->b_doto;
		curwp->w_markp = bp->b_markp;
		curwp->w_marko = bp->b_marko;
		return TRUE;
	}
	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp != curwp && wp->w_bufp == bp) {
			curwp->w_dotp = wp->w_dotp;
			curwp->w_doto = wp->w_doto;
			curwp->w_markp = wp->w_markp;
			curwp->w_marko = wp->w_marko;
			break;
		}
	}
	return TRUE;
}

int killbuffer(int f, int n)
{
	struct buffer *bp;
	char bufn[NBUFN];
	int s;

	if ((s = mlreply("Kill buffer: ", bufn, NBUFN)) != TRUE)
		return s;
	if ((bp = bfind(bufn, FALSE, 0)) == NULL)
		return TRUE;

	if (bp->b_flag & BFINVS)
		return TRUE;

	return zotbuf(bp);
}

/* Kill the buffer pointed to by bp, and update bheadp when necessary */
int zotbuf(struct buffer *bp)
{
	struct buffer *bp1, *bp2;
	int s;

	/* Reset prevbp when that buffer is killed */
	if (bp == prevbp)
		prevbp = NULL;

	if (bp->b_nwnd != 0) {	/* Error if on screen. */
		mlwrite("Buffer is being displayed");
		return FALSE;
	}
	if ((s = bclear(bp)) != TRUE)	/* Blow text away. */
		return s;
	free(bp->b_linep);	/* Release header line. */
	bp1 = NULL;		/* Find the header. */
	bp2 = bheadp;
	while (bp2 != bp) {
		bp1 = bp2;
		bp2 = bp2->b_bufp;
	}
	bp2 = bp2->b_bufp;	/* Next one in chain. */
	if (bp1 == NULL)	/* Unlink it. */
		bheadp = bp2;
	else
		bp1->b_bufp = bp2;

	free(bp);

#if RAMSHOW
	curwp->w_flag |= WFMODE;	/* update memory usage */
#endif
	return TRUE;
}

/*
 * List all of the active buffers.  First update the special
 * buffer that holds the list.  Next make sure at least 1 window is displaying
 * the buffer list, splitting the screen if this is what it takes.
 * Lastly, repaint all of the windows that are displaying the list.
 * A numeric argument forces it to list invisible buffers as well.
 */
int listbuffers(int f, int n)
{
	struct window *wp;
	struct buffer *bp;
	int s;

	if ((s = makelist(f)) != TRUE)
		return s;
	if (blistp->b_nwnd == 0) {	/* Not on screen yet. */
		if ((wp = wpopup()) == NULL)
			return FALSE;
		bp = wp->w_bufp;
		if (--bp->b_nwnd == 0) {
			bp->b_dotp = wp->w_dotp;
			bp->b_doto = wp->w_doto;
			bp->b_markp = wp->w_markp;
			bp->b_marko = wp->w_marko;
		}
		wp->w_bufp = blistp;
		++blistp->b_nwnd;
	}
	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_bufp == blistp) {
			wp->w_linep = lforw(blistp->b_linep);
			wp->w_dotp = lforw(blistp->b_linep);
			wp->w_doto = 0;
			wp->w_markp = NULL;
			wp->w_marko = 0;
			wp->w_flag |= WFMODE | WFHARD;
		}
	}
	return TRUE;
}

/*
 * This routine rebuilds the text in the special secret buffer that holds the
 * buffer list.  It is called by the list buffers command.
 * Iflag indicates wether to list hidden buffers.
 */
int makelist(int iflag)
{
#define CHAR_WIDTH_FOR_SIZE 10
	struct buffer *bp;
	struct line *lp;
	char b[CHAR_WIDTH_FOR_SIZE + 1];
	char *cp1, *cp2, *line;
	int c, s, i;
	long nbytes;

	if ((line = malloc(term.t_ncol)) == NULL)
		return FALSE;

	blistp->b_flag &= ~BFCHG;
	if ((s = bclear(blistp)) != TRUE)	/* Blow old text away */
		return s;
	strcpy(blistp->b_fname, "");
	if (addline("ACT MODE       SIZE BUFFER          FILE") == FALSE)
		return FALSE;
	if (addline("--- ----       ---- ------          ----") == FALSE)
		return FALSE;

	bp = bheadp;		/* For all buffers */

	/* build line to report global mode settings */
	cp1 = line;
	for (i = 0; i < 4; ++i)
		*cp1++ = ' ';

	/* output the mode codes */
	for (i = 0; i < NMODES; ++i) {
		if (gmode & modevalue[i])
			*cp1++ = modecode[i];
		else
			*cp1++ = '.';
	}
	strcpy(cp1, "             Global Modes");
	if (addline(line) == FALSE)
		return FALSE;

	/* output the list of buffers */
	while (bp != NULL) {
		/* skip invisable buffers if iflag is false */
		if (((bp->b_flag & BFINVS) != 0) && (iflag != TRUE)) {
			bp = bp->b_bufp;
			continue;
		}
		cp1 = line;	/* Start at left edge */

		/* output status of ACTIVE flag (has the file been read in? */
		if (bp->b_active == TRUE)	/* "@" if activated */
			*cp1++ = '@';
		else
			*cp1++ = ' ';

		/* output status of changed flag */
		if ((bp->b_flag & BFCHG) != 0)	/* "*" if changed */
			*cp1++ = '*';
		else
			*cp1++ = ' ';

		/* report if the file is truncated */
		if ((bp->b_flag & BFTRUNC) != 0)
			*cp1++ = '#';
		else
			*cp1++ = ' ';

		*cp1++ = ' ';	/* space */

		/* output the mode codes */
		for (i = 0; i < NMODES; ++i) {
			if (bp->b_mode & modevalue[i])
				*cp1++ = modecode[i];
			else
				*cp1++ = '.';
		}
		for (i = NMODES; i < 5 /* 4 + 1 */; ++i)
			*cp1++ = ' ';
		nbytes = 0L;	/* Count bytes in buf. */
		lp = lforw(bp->b_linep);
		while (lp != bp->b_linep) {
			nbytes += (long)llength(lp) + 1L;
			lp = lforw(lp);
		}
		e_ltoa(b, CHAR_WIDTH_FOR_SIZE, nbytes);
		cp2 = b;
		while ((c = *cp2++) != 0)
			*cp1++ = c;
		*cp1++ = ' ';	/* Gap. */
		cp2 = &bp->b_bname[0];	/* Buffer name */
		while ((c = *cp2++) != 0)
			*cp1++ = c;
		cp2 = &bp->b_fname[0];	/* File name */
		if (*cp2 != 0) {
			while (cp1 < &line[3 + 1 + 4 + 1 + 10 + 1 + NBUFN])
				*cp1++ = ' ';
			while ((c = *cp2++) != 0) {
				if (cp1 < &line[term.t_ncol - 1])
					*cp1++ = c;
			}
		}
		*cp1 = 0;	/* Add to the buffer. */
		if (addline(line) == FALSE)
			return FALSE;
		bp = bp->b_bufp;
	}
	return TRUE;
}

void e_ltoa(char *buf, int width, long num)
{
	buf[width] = '\0';
	while (num >= 10 && width > 1) {
		buf[--width] = (int)(num % 10L) + '0';
		num /= 10L;
	}
	if (num < 10)
		buf[--width] = (int)num + '0';
	else
		buf[--width] = '?';

	while (width != 0)
		buf[--width] = ' ';
}

/*
 * The argument "text" points to a string.
 * Append this line to the buffer list buffer.  Handcraft the EOL on the end.
 * Return TRUE if it worked and FALSE if you ran out of room.
 */
int addline(char *text)
{
	struct line *lp;
	int i, ntext;

	ntext = strlen(text);
	if ((lp = lalloc(ntext)) == NULL)
		return FALSE;
	for (i = 0; i < ntext; ++i)
		lputc(lp, i, text[i]);
	blistp->b_linep->l_bp->l_fp = lp;	/* Hook onto the end */
	lp->l_bp = blistp->b_linep->l_bp;
	blistp->b_linep->l_bp = lp;
	lp->l_fp = blistp->b_linep;
	if (blistp->b_dotp == blistp->b_linep)	/* If "." is at the end */
		blistp->b_dotp = lp;	/* move it to new line */
	return TRUE;
}

/*
 * Look through the list of buffers, return TRUE if there are any changed
 * buffers.  Return FALSE if no buffers have been changed.
 * Buffers that hold magic internal stuff are not considered.
 */
int anycb(void)
{
	struct buffer *bp;

	for (bp = bheadp; bp != NULL; bp = bp->b_bufp) {
		if ((bp->b_flag & BFINVS) == 0 && (bp->b_flag & BFCHG) != 0)
			return TRUE;
	}
	return FALSE;
}

/*
 * Find a buffer, by name.  Return a pointer to the buffer structure
 * associated with it.
 * If the buffer is not found and the "cflag" is TRUE, create it.
 * The "bflag" is the settings for the flags in in buffer.
 */
struct buffer *bfind(char *bname, int cflag, int bflag)
{
	struct buffer *bp, *sb;
	struct line *lp;

	for (bp = bheadp; bp != NULL; bp = bp->b_bufp) {
		if (strcmp(bname, bp->b_bname) == 0)
			return bp;
	}

	/* not existing buffer found */

	if (cflag == FALSE)
		return NULL;

	/* create a buffer */

	if ((bp = malloc(sizeof(struct buffer))) == NULL)
		return NULL;
	if ((lp = lalloc(0)) == NULL) {
		free(bp);
		return NULL;
	}
	/* find the place in the list to insert this buffer */
	if (bheadp == NULL || strcmp(bheadp->b_bname, bname) > 0) {
		/* insert at the beginning */
		bp->b_bufp = bheadp;
		bheadp = bp;
	} else {
		sb = bheadp;
		while (sb->b_bufp != NULL) {
			if (strcmp(sb->b_bufp->b_bname, bname) > 0)
				break;
			sb = sb->b_bufp;
		}
		/* and insert it */
		bp->b_bufp = sb->b_bufp;
		sb->b_bufp = bp;
	}

	/* and set up the other buffer fields */
	bp->b_active = TRUE;
	bp->b_dotp = lp;
	bp->b_doto = 0;
	bp->b_markp = NULL;
	bp->b_marko = 0;
	bp->b_flag = bflag;
	bp->b_mode = gmode;
	bp->b_nwnd = 0;
	bp->b_linep = lp;
	strcpy(bp->b_fname, "");
	strcpy(bp->b_bname, bname);

	lp->l_fp = lp;
	lp->l_bp = lp;

	return bp;
}

/*
 * This routine blows away all of the text in a buffer.
 * If the buffer is marked as changed then we ask if it is ok to blow it away;
 * this is to save the user the grief of losing text.
 * The window chain is nearly always wrong if this gets called;
 * the caller must arrange for the updates that are required.
 * Return TRUE if everything looks good.
 */
int bclear(struct buffer *bp)
{
	struct line *lp;
	int s;

	if ((bp->b_flag & BFINVS) == 0	/* Not scratch buffer. */
			&& (bp->b_flag & BFCHG) != 0	/* Something changed */
			&& (s = mlyesno("Discard changes")) != TRUE)
		return s;
	bp->b_flag &= ~BFCHG;	/* Not changed */
	while ((lp = lforw(bp->b_linep)) != bp->b_linep)
		lfree(lp);
	bp->b_dotp = bp->b_linep;	/* Fix "." */
	bp->b_doto = 0;
	bp->b_markp = NULL;	/* Invalidate "mark" */
	bp->b_marko = 0;
	return TRUE;
}
