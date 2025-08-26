#include "me.h"

/* Switch to buffer `bp`.  (Make buffer `bp` curwp->w_bufp) */
int swbuffer(struct buffer *bp)
{
	struct window *wp;

	if (bp == NULL || bp == curwp->w_bufp)
		return FALSE;
	if (curwp->w_bufp != NULL) {
		if (--curwp->w_bufp->b_nwnd == 0)
			wstate_save(curwp);
	}

	curwp->w_bufp = bp;
	if (!(curwp->w_bufp->b_flag & BFACTIVE)) {
		readin(curwp->w_bufp->b_fname);
		curwp->w_bufp->b_dotp = curwp->w_bufp->b_linep->l_fp;
		curwp->w_bufp->b_doto = 0;
		curwp->w_bufp->b_flag |= BFACTIVE;
	}

	curwp->w_bufp = bp;
	curwp->w_linep = bp->b_linep;
	curwp->w_flag |= WFMODE | WFFORCE | WFHARD;
	curwp->w_force = curwp->w_ntrows / 2;

	if (bp->b_nwnd++ == 0) {
		wstate_restore(curwp, bp);
		return TRUE;
	}

	/* If bp->b_nwnd was not 0, there must be window(s) displaying bp. */
	for_each_wind(wp) {
		if (wp != curwp && wp->w_bufp == bp) {
			wstate_copy(curwp, wp);
			return TRUE;
		}
	}

	/* Code should not reach here */
	mlwrite("Internal Error");
	return FALSE;
}

int nextbuffer(int f, int n)
{
	struct buffer *bp = curwp->w_bufp;

	if (n < 1 || bp == NULL)
		return FALSE;
	while (n-- > 0) {
		if ((bp = bp->b_bufp) == NULL)
			bp = bheadp;
	}

	return swbuffer(bp);
}

int bufrdonly(int f, int n)
{
	if (curwp->w_bufp->b_flag & BFRDONLY)
		curwp->w_bufp->b_flag &= ~BFRDONLY;
	else
		curwp->w_bufp->b_flag |= BFRDONLY;
	curwp->w_flag |= WFMODE;
	return TRUE;
}

/* Kill the buffer pointed to by bp, and update bheadp when necessary. */
int zotbuf(struct buffer *bp)
{
	struct buffer *bp1, *bp2;

	if (bp->b_nwnd != 0) {
		mlwrite("Buffer is being displayed");
		return FALSE;
	}
	if (bclear(bp) != TRUE)
		return FALSE;

	free(bp->b_linep);

	/* Iterate through the buffer list until we found bp. */
	bp1 = NULL;
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
 * Find a buffer by name.  Create it if buffer is not found and cflag is TRUE.
 */
struct buffer *bfind(char *raw_filename, int cflag)
{
	char filename[NFILEN];
	struct buffer *bp;
	struct line *lp;
	int trunc;

	trim_spaces(filename, raw_filename, NFILEN, &trunc);
	if (trunc) {
		mlwrite("Filename too long");
		return NULL;
	}
	for_each_buff(bp) {
		if (strcmp(filename, bp->b_fname) == 0)
			return bp;
	}
	if (cflag == FALSE)
		return NULL;
	if ((bp = malloc(sizeof(struct buffer))) == NULL)
		return NULL;
	memset(bp, 0, sizeof(*bp));

	if ((lp = lalloc(0)) == NULL)
		return NULL;
	lp->l_fp = lp;
	lp->l_bp = lp;

	/* The order of buffers does not matter, so we insert it at head */
	bp->b_bufp = bheadp;
	bheadp = bp;

	strncpy_safe(bp->b_fname, filename, NFILEN);
	bp->b_linep = lp;
	bp->b_dotp = lp;
	return bp;
}

/*
 * This routine blows away all of the text in a buffer.  The window chain is
 * nearly always wrong if this gets called, the caller must arrange for the
 * updates that are required.
 */
int bclear(struct buffer *bp)
{
	struct line *lp;

	if ((bp->b_flag & BFCHG) && mlyesno("Discard changes") != TRUE)
		return FALSE;

	bp->b_flag &= ~BFCHG;
	while ((lp = bp->b_linep->l_fp) != bp->b_linep)
		lfree(lp);

	bp->b_dotp = bp->b_linep;
	bp->b_doto = 0;
	bp->b_markp = NULL;
	bp->b_marko = 0;
	return TRUE;
}
