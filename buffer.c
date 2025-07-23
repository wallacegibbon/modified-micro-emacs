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
		readin(curbp->b_fname, TRUE);
		curbp->b_dotp = lforw(curbp->b_linep);
		curbp->b_doto = 0;
		curbp->b_active = TRUE;
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
	for_each_wind(wp) {
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

int bufrdonly(int f, int n)
{
	curbp->rdonly = !curbp->rdonly;
	curwp->w_flag |= WFMODE;
	return TRUE;
}

/* Kill the buffer pointed to by bp, and update bheadp when necessary */
int zotbuf(struct buffer *bp)
{
	struct buffer *bp1, *bp2;

	/* Reset prevbp when that buffer is killed */
	if (bp == prevbp)
		prevbp = NULL;

	if (bp->b_nwnd != 0) {	/* Error if on screen. */
		mlwrite("Buffer is being displayed");
		return FALSE;
	}
	if (bclear(bp) != TRUE)	/* Blow text away. */
		return FALSE;
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
	for_each_buff(bp) {
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

	for_each_buff(bp) {
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
	bp->rdonly = 0;
	bp->b_flag = bflag;
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

	if ((bp->b_flag & BFINVS) == 0	/* Not scratch buffer. */
			&& (bp->b_flag & BFCHG) != 0	/* Something changed */
			&& mlyesno("Discard changes") != TRUE)
		return FALSE;
	bp->b_flag &= ~BFCHG;	/* Not changed */
	while ((lp = lforw(bp->b_linep)) != bp->b_linep)
		lfree(lp);
	bp->b_dotp = bp->b_linep;	/* Fix "." */
	bp->b_doto = 0;
	bp->b_markp = NULL;	/* Invalidate "mark" */
	bp->b_marko = 0;
	return TRUE;
}
