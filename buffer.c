#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include "wrapper.h"

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
		while (bp == NULL) {
			bp = (bp == NULL) ? bheadp : bp->b_bufp;
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

	if (--curbp->b_nwnd == 0) {
		curbp->b_dotp = curwp->w_dotp;
		curbp->b_doto = curwp->w_doto;
		curbp->b_markp = curwp->w_markp;
		curbp->b_marko = curwp->w_marko;
	}

	curbp = bp;
	if (!(curbp->b_flag & BFACTIVE)) {
		readin(curbp->b_fname, TRUE);
		curbp->b_dotp = lforw(curbp->b_linep);
		curbp->b_doto = 0;
		curbp->b_flag |= BFACTIVE;
	}

	curwp->w_bufp = bp;
	curwp->w_linep = bp->b_linep;
	curwp->w_flag |= WFMODE | WFFORCE | WFHARD;
	if (bp->b_nwnd++ == 0) {
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

	return zotbuf(bp);
}

int bufrdonly(int f, int n)
{
	if (curbp->b_flag & BFRDONLY)
		curbp->b_flag &= ~BFRDONLY;
	else
		curbp->b_flag |= BFRDONLY;
	curwp->w_flag |= WFMODE;
	return TRUE;
}

/* Kill the buffer pointed to by bp, and update bheadp when necessary */
int zotbuf(struct buffer *bp)
{
	struct buffer *bp1, *bp2;

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
	bp2 = bp2->b_bufp;
	if (bp1 == NULL)
		bheadp = bp2;
	else
		bp1->b_bufp = bp2;

	free(bp);
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
		if ((bp->b_flag & BFCHG))
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
	bp->b_dotp = lp;
	bp->b_doto = 0;
	bp->b_markp = NULL;
	bp->b_marko = 0;
	bp->b_flag = bflag | BFACTIVE;
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

	if ((bp->b_flag & BFCHG) && mlyesno("Discard changes") != TRUE)
		return FALSE;
	bp->b_flag &= ~BFCHG;
	while ((lp = lforw(bp->b_linep)) != bp->b_linep)
		lfree(lp);
	bp->b_dotp = bp->b_linep;
	bp->b_doto = 0;
	bp->b_markp = NULL;
	bp->b_marko = 0;
	return TRUE;
}
