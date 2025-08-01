/*
 * Note that this code only updates the dot and mark values in the window list.
 * Since all the code acts on the current window, the buffer that we are
 * editing must be being displayed, which means that "b_nwnd" is non zero,
 * which means that the dot and mark values in the buffer headers are nonsense.
 *
 * There are routines in this file that handle the kill buffer too.
 * It isn't here for any good reason.
 */

#include "efunc.h"
#include "edef.h"
#include "line.h"

#define BLOCK_SIZE	16	/* Line block chunk size. */

struct line *lalloc(int used)
{
	struct line *lp;
	int size;

	size = (used + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);
	if (size == 0)
		size = BLOCK_SIZE;

	if ((lp = malloc(sizeof(struct line) + size)) == NULL) {
		mlwrite("(OUT OF MEMORY)");
		return NULL;
	}

	lp->l_size = size;
	lp->l_used = used;
	return lp;
}

/* Delete line "lp".  Fix all of the links that might point at it. */
void lfree(struct line *lp)
{
	struct buffer *bp;
	struct window *wp;

	for_each_wind(wp) {
		if (wp->w_linep == lp)
			wp->w_linep = lp->l_fp;
		if (wp->w_dotp == lp) {
			wp->w_dotp = lp->l_fp;
			wp->w_doto = 0;
		}
		if (wp->w_markp == lp) {
			wp->w_markp = lp->l_fp;
			wp->w_marko = 0;
		}
	}
	for_each_buff(bp) {
		if (bp->b_nwnd == 0) {
			if (bp->b_dotp == lp) {
				bp->b_dotp = lp->l_fp;
				bp->b_doto = 0;
			}
			if (bp->b_markp == lp) {
				bp->b_markp = lp->l_fp;
				bp->b_marko = 0;
			}
		}
	}
	lp->l_bp->l_fp = lp->l_fp;
	lp->l_fp->l_bp = lp->l_bp;
	free(lp);
}

/*
 * This routine gets called when a character is changed in place in the current
 * buffer.  It updates required flags in the buffer and window system.
 */
void lchange(int flag)
{
	struct window *wp;

	if (curbp->b_nwnd != 1)	/* Ensure hard. */
		flag = WFHARD;
	if (!(curbp->b_flag & BFCHG)) {
		flag |= WFMODE;
		curbp->b_flag |= BFCHG;
	}
	for_each_wind(wp) {
		if (wp->w_bufp == curbp)
			wp->w_flag |= flag;
	}
}

static inline void line_insert(struct line *lp, struct line *lp_new)
{
	struct line *tmp = lp->l_bp;

	lp_new->l_bp = tmp;
	lp_new->l_fp = lp;
	tmp->l_fp = lp_new;
	lp->l_bp = lp_new;
}

static inline void line_replace(struct line *lp, struct line *lp_new)
{
	lp->l_bp->l_fp = lp_new;
	lp->l_fp->l_bp = lp_new;
	lp_new->l_fp = lp->l_fp;
	lp_new->l_bp = lp->l_bp;
}

static inline void line_unlink(struct line *lp)
{
	lp->l_bp->l_fp = lp->l_fp;
	lp->l_fp->l_bp = lp->l_bp;
}

static int linsert_end(int n, int c)
{
	struct line *lp = curwp->w_dotp, *lp_new;
	int i;

	if (curwp->w_doto != 0) {
		mlwrite("bug: linsert, w_doto of end is not 0");
		return FALSE;
	}

	if ((lp_new = lalloc(n)) == NULL)
		return FALSE;

	line_insert(lp, lp_new);

	for (i = 0; i < n; ++i)
		lp_new->l_text[i] = c;

	curwp->w_dotp = lp_new;
	curwp->w_doto = n;
	return TRUE;
}

static int linsert_realloc(int n, int c, struct line **lp_new)
{
	struct line *lp1 = curwp->w_dotp, *lp2;
	int doto = curwp->w_doto, i;
	char *cp1, *cp2;

	if ((lp2 = lalloc(lp1->l_used + n)) == NULL)
		return FALSE;

	cp1 = &lp1->l_text[0];
	cp2 = &lp2->l_text[0];
	while (cp1 != &lp1->l_text[doto])
		*cp2++ = *cp1++;

	cp2 += n;
	while (cp1 != &lp1->l_text[lp1->l_used])
		*cp2++ = *cp1++;

	line_replace(lp1, lp2);
	free(lp1);

	for (i = 0; i < n; ++i)
		lp2->l_text[doto + i] = c;

	*lp_new = lp2;
	return TRUE;
}

/* CAUTION: Make sure that curwp->w_dotp have enough for this insertion */
static int linsert_inplace(int n, int c)
{
	struct line *lp = curwp->w_dotp;
	int doto = curwp->w_doto, i;
	char *cp1, *cp2;

	lp->l_used += n;

	cp2 = &lp->l_text[lp->l_used];
	cp1 = cp2 - n;
	while (cp1 != &lp->l_text[doto])
		*--cp2 = *--cp1;

	for (i = 0; i < n; ++i)
		lp->l_text[doto + i] = c;

	return TRUE;
}

/*
 * Insert "n" copies of the character "c" at the current location of dot.
 * In the easy case all that happens is the text is stored in the line.
 * In the hard case, the line has to be reallocated.
 */
int lnonnewline(int n, int c)
{
	struct window *wp;
	struct line *lp = curwp->w_dotp, *lp_new;
	int doto = curwp->w_doto;

	if (curbp->b_flag & BFRDONLY)
		return rdonly();

	lchange(WFEDIT);

	/* Inserting at the end of the buffer does not need window updating */
	if (lp == curbp->b_linep)
		return linsert_end(n, c);

	/* Set the default value for lp_new forlinsert_inplace */
	lp_new = lp;

	if (lp->l_used + n > lp->l_size) {
		if (linsert_realloc(n, c, &lp_new) == FALSE)
			return FALSE;
	} else {
		linsert_inplace(n, c);
	}

	/* Different windows of the same buffer can have different cursor */
	/* If the cursor of the window is after insertion point, update it */

	for_each_wind(wp) {
		if (wp->w_linep == lp)
			wp->w_linep = lp_new;
		if (wp->w_dotp == lp) {
			wp->w_dotp = lp_new;
			if (wp->w_doto >= doto)
				wp->w_doto += n;
		}
		if (wp->w_markp == lp) {
			wp->w_markp = lp_new;
			if (wp->w_marko >= doto)
				wp->w_marko += n;
		}
	}
	return TRUE;
}

/*
 * Insert a newline into the buffer at the current location of dot in the
 * current window.
 */
static int lnewline(void)
{
	struct window *wp;
	struct line *lp = curwp->w_dotp, *lp_new;
	int doto = curwp->w_doto;
	char *cp1, *cp2;

	if (curbp->b_flag & BFRDONLY)
		return rdonly();

	lchange(WFHARD | WFINS);

	if ((lp_new = lalloc(doto)) == NULL)
		return FALSE;

	/* Copy content before doto to the new allocated line */
	cp1 = &lp->l_text[0];
	cp2 = &lp_new->l_text[0];
	while (cp1 != &lp->l_text[doto])
		*cp2++ = *cp1++;

	/* Shift content left to the origin */
	cp2 = &lp->l_text[0];
	while (cp1 != &lp->l_text[lp->l_used])
		*cp2++ = *cp1++;

	lp->l_used -= doto;

	line_insert(lp, lp_new);

	for_each_wind(wp) {
		if (wp->w_linep == lp)
			wp->w_linep = lp_new;
		if (wp->w_dotp == lp) {
			if (wp->w_doto < doto)
				wp->w_dotp = lp_new;
			else
				wp->w_doto -= doto;
		}
		if (wp->w_markp == lp) {
			if (wp->w_marko < doto)
				wp->w_markp = lp_new;
			else
				wp->w_marko -= doto;
		}
	}

	return TRUE;
}

int linsert(int n, int c)
{
	int s = TRUE;
	if (c == '\n') {
		while (s && n--)
			s = lnewline();
		return s;
	} else {
		return lnonnewline(n, c);
	}
}

/* linstr -- Insert a string at the current point. */
int linstr(char *instr)
{
	int c;

	if (instr == NULL)
		return TRUE;

	while ((c = *instr++)) {
		if (linsert(1, c) != TRUE)
			return FALSE;
	}

	return TRUE;
}

static int ljoin_nextline_try(struct line *lp)
{
	struct window *wp;
	struct line *lp_next = lp->l_fp;
	char *cp1, *cp2;

	/* If the rest size of lp is not enough to hold next line */
	if (lp_next->l_used > lp->l_size - lp->l_used)
		return FALSE;

	cp1 = &lp->l_text[lp->l_used];
	cp2 = &lp_next->l_text[0];

	while (cp2 != &lp_next->l_text[lp_next->l_used])
		*cp1++ = *cp2++;

	for_each_wind(wp) {
		if (wp->w_linep == lp_next)
			wp->w_linep = lp;
		if (wp->w_dotp == lp_next) {
			wp->w_dotp = lp;
			wp->w_doto += lp->l_used;
		}
		if (wp->w_markp == lp_next) {
			wp->w_markp = lp;
			wp->w_marko += lp->l_used;
		}
	}

	lp->l_used += lp_next->l_used;

	line_unlink(lp_next);
	free(lp_next);
	return TRUE;
}

/*
 * Delete a newline.  Join the current line with the next line.
 * If the next line is the magic header line always return TRUE.
 * even if nothing is done, and this makes the kill buffer work "right".
 */
static int ldelnewline(void)
{
	struct window *wp;
	struct line *lp = curwp->w_dotp, *lp2 = lp->l_fp, *lp_new;
	char *cp1, *cp2;
	int total_used;

	if (curbp->b_flag & BFRDONLY)
		return rdonly();

	/* Merging the last line and the magic line is always successful */
	if (lp2 == curbp->b_linep) {
		if (lp->l_used == 0)
			lfree(lp);
		return TRUE;
	}

	if (ljoin_nextline_try(lp))
		return TRUE;

	/* For simplicity, we create a new line to hold lp and lp2 */

	total_used = lp->l_used + lp2->l_used;
	if ((lp_new = lalloc(total_used)) == NULL)
		return FALSE;

	cp1 = &lp->l_text[0];
	cp2 = &lp_new->l_text[0];
	while (cp1 != &lp->l_text[lp->l_used])
		*cp2++ = *cp1++;

	cp1 = &lp2->l_text[0];
	while (cp1 != &lp2->l_text[lp2->l_used])
		*cp2++ = *cp1++;

	lp_new->l_used = total_used;
	line_unlink(lp2);
	line_replace(lp, lp_new);

	for_each_wind(wp) {
		if (wp->w_linep == lp || wp->w_linep == lp2)
			wp->w_linep = lp_new;
		if (wp->w_dotp == lp) {
			wp->w_dotp = lp_new;
		} else if (wp->w_dotp == lp2) {
			wp->w_dotp = lp_new;
			wp->w_doto += lp->l_used;
		}
		if (wp->w_markp == lp) {
			wp->w_markp = lp_new;
		} else if (wp->w_markp == lp2) {
			wp->w_markp = lp_new;
			wp->w_marko += lp->l_used;
		}
	}

	free(lp);
	free(lp2);
	return TRUE;
}

static int ldelete_once(int n, int kflag)
{
	struct window *wp;
	struct line *lp = curwp->w_dotp;
	int doto = curwp->w_doto, chunk;
	char *cp1, *cp2;

	if (lp == curbp->b_linep)
		return -1;

	chunk = lp->l_used - doto;

	/*
	 * Line end means a newline in the buffer/file.  For simplicity,
	 * We call `ldelnewline`, and leave the rest work to the next loop.
	 */
	if (chunk == 0) {
		lchange(WFHARD | WFKILLS);
		if (ldelnewline() == FALSE)
			return -1;
		if (kflag && kinsert('\n') == FALSE)
			return -1;
		return n - 1;
	}

	lchange(WFEDIT);
	if (chunk > n)
		chunk = n;

	cp1 = &lp->l_text[doto];
	cp2 = cp1 + chunk;

	/* If kflag is active, call kinsert and then reset `cp1` */
	if (kflag) {
		while (cp1 != cp2) {
			if (kinsert(*cp1++) == FALSE)
				return -1;
		}
		cp1 = &lp->l_text[doto];
	}

	while (cp2 != &lp->l_text[lp->l_used])
		*cp1++ = *cp2++;

	lp->l_used -= chunk;

	for_each_wind(wp) {
		if (wp->w_dotp == lp && wp->w_doto >= doto) {
			wp->w_doto -= chunk;
			if (wp->w_doto < doto)
				wp->w_doto = doto;
		}
		if (wp->w_markp == lp && wp->w_marko >= doto) {
			wp->w_marko -= chunk;
			if (wp->w_marko < doto)
				wp->w_marko = doto;
		}
	}

	return n - chunk;
}

/*
 * This function deletes "n" bytes, starting at dot.
 * The "kflag" is TRUE if the text should be put in the kill buffer.
 */
int ldelete(long n, int kflag)
{
	if (curbp->b_flag & BFRDONLY)
		return rdonly();

	while (n > 0) {
		if ((n = ldelete_once(n, kflag)) < 0)
			return FALSE;
	}

	return TRUE;
}

/* Copy contents of the kill buffer into current buffer */
int linsert_kbuf(void)
{
	struct kill *kp;
	char *sp;
	int n;

	for_each_kbuf(kp) {
		n = kp->d_next == NULL ? kused : KBLOCK;
		sp = kp->d_chunk;
		while (n--) {
			if (linsert(1, *sp++) == FALSE)
				return FALSE;
		}
	}

	return TRUE;
}

/* Insert a character to the kill buffer, allocating new chunks as needed. */
int kinsert(int c)
{
	struct kill *nchunk;

	if (kused >= KBLOCK) {
		if ((nchunk = malloc(sizeof(struct kill))) == NULL)
			return FALSE;
		if (kbufh == NULL)
			kbufh = nchunk;
		if (kbufp != NULL)
			kbufp->d_next = nchunk;
		kbufp = nchunk;
		kbufp->d_next = NULL;
		kused = 0;
	}
	kbufp->d_chunk[kused++] = c;
	return TRUE;
}

/*
 * Delete all of the text saved in the kill buffer.  Called by commands when a
 * new kill context is being created.  The kill buffer array is released, just
 * in case the buffer has grown to immense size.
 */
void kdelete(void)
{
	struct kill *kp;

	if (kbufh == NULL)
		return;

	for (kbufp = kbufh; kbufp != NULL; kbufp = kp) {
		kp = kbufp->d_next;
		free(kbufp);
	}

	/* Reset all the kill buffer pointers */
	kbufh = NULL;
	kbufp = NULL;
	kused = KBLOCK;
}
