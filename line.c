/*
There are routines in this file that handle the kill buffer, which is not here
for any good reason.
*/

#include "me.h"

#define BLOCK_SIZE	16	/* Line block chunk size. */

struct line *lalloc(int used)
{
	struct line *lp;
	int size;
	size = (used + BLOCK_SIZE - 1) & ~(BLOCK_SIZE - 1);
	if (size == 0)
		size = BLOCK_SIZE;
	if ((lp = malloc(sizeof(struct line) + size)) == NULL) {
		mlwrite("OUT OF MEMORY");
		return NULL;
	}
	lp->l_size = size;
	lp->l_used = used;
	return lp;
}

/* Deletes line "lp".  Fixes all of the links that might point at it. */
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
Gets called when a character is changed in place in the current buffer.
Updates required flags in the buffer and window system.
*/
void lchange(int flag)
{
	struct window *wp;
	if (curwp->w_bufp->b_nwnd != 1)	/* Ensure hard. */
		flag = WFHARD;
	if (!(curwp->w_bufp->b_flag & BFCHG)) {
		flag |= WFMODE;
		curwp->w_bufp->b_flag |= BFCHG;
	}
	for_each_wind(wp) {
		if (wp->w_bufp == curwp->w_bufp)
			wp->w_flag |= flag;
	}
}

static int linsert_simple(int n, int c)
{
	struct line *lp;
	int i;
	if (curwp->w_doto != 0) {
		mlwrite("bug: linsert, w_doto of end is not 0");
		return FALSE;
	}
	if ((lp = lalloc(n)) == NULL)
		return FALSE;
	for (i = 0; i < n; ++i)
		lp->l_text[i] = c;

	line_insert(curwp->w_dotp, lp);
	curwp->w_dotp = lp;
	curwp->w_doto = n;
	return TRUE;
}

static int linsert_realloc(int n, int c, struct line **lp_new)
{
	struct line *lp1 = curwp->w_dotp, *lp2;
	int doto = curwp->w_doto;
	char *cp1, *cp2;

	if ((lp2 = lalloc(lp1->l_used + n)) == NULL)
		return FALSE;

	cp1 = &lp1->l_text[0];
	cp2 = &lp2->l_text[0];
	while (cp1 != &lp1->l_text[doto])
		*cp2++ = *cp1++;
	while (n--)
		*cp2++ = c;
	while (cp1 != &lp1->l_text[lp1->l_used])
		*cp2++ = *cp1++;

	line_replace(lp1, lp2);
	free(lp1);
	*lp_new = lp2;
	return TRUE;
}

/* CAUTION: Makes sure that curwp->w_dotp have enough for this insertion */
static int linsert_inplace(int n, int c)
{
	struct line *lp = curwp->w_dotp;
	int doto = curwp->w_doto;
	char *cp1, *cp2;

	lp->l_used += n;
	cp2 = &lp->l_text[lp->l_used];
	cp1 = cp2 - n;
	while (cp1 != &lp->l_text[doto])
		*--cp2 = *--cp1;
	while (n--)
		*--cp2 = c;

	return TRUE;
}

/*
Inserts "n" copies of the character "c" at the current location of dot.
In the easy case all that happens is the text is stored in the line.
In the hard case, the line has to be reallocated.
*/
static int lnonnewline(int n, int c)
{
	struct window *wp;
	struct line *lp = curwp->w_dotp, *lp_new;
	int doto = curwp->w_doto;

	lchange(WFEDIT);

	/* Inserting at the end of the buffer does not need window updating */
	if (lp == curwp->w_bufp->b_linep)
		return linsert_simple(n, c);

	/* Sets the default value for lp_new forlinsert_inplace */
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
Inserts a newline into the buffer at the current location in current window.
*/
static int lnewline(void)
{
	struct window *wp;
	struct line *lp = curwp->w_dotp, *lp_new;
	int doto = curwp->w_doto;
	char *cp1, *cp2;

	lchange(WFHARD);

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
	if (curwp->w_bufp->b_flag & BFRDONLY)
		return rdonly();
	if (n == 0)
		return TRUE;
	if (c == '\n') {
		while (s && n--)
			s = lnewline();
		return s;
	} else {
		return lnonnewline(n, c);
	}
}

/* Inserts a string at the current point. */
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
Deletes a newline and joins the current line with the next line.
If the next line is the magic header line always return TRUE even if nothing
is done, and this makes the kill buffer work "right".
*/
static int ldelnewline(void)
{
	struct window *wp;
	struct line *lp = curwp->w_dotp, *lp2 = lp->l_fp, *lp_new;
	char *cp1, *cp2;
	int total_used;

	/* Merging the last line and the magic line is always successful */
	if (lp2 == curwp->w_bufp->b_linep) {
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

	if (lp == curwp->w_bufp->b_linep)
		return -1;

	chunk = lp->l_used - doto;

	/* Handle the newline deleting in a lazy way for simplicity */
	if (chunk == 0) {
		lchange(WFHARD);
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
Deletes "n" bytes, starting at dot.  The "kflag" is TRUE if the text should
be put in the kill buffer.
*/
int ldelete(long n, int kflag)
{
	if (curwp->w_bufp->b_flag & BFRDONLY)
		return rdonly();
	if (n == 0)
		return TRUE;
	while (n > 0) {
		if ((n = ldelete_once(n, kflag)) < 0)
			return FALSE;
	}
	return TRUE;
}

/* Copies contents of the kill buffer into current buffer */
int linsert_kbuf(void)
{
	struct kill *kp;
	char *sp;
	int n;
	for_each_kbuf(kp) {
		n = kp->k_next == NULL ? kused : KBLOCK;
		sp = kp->k_chunk;
		while (n--) {
			if (linsert(1, *sp++) == FALSE)
				return FALSE;
		}
	}
	return TRUE;
}

/* Inserts a character to the kill buffer, allocates new chunks as needed. */
int kinsert(int c)
{
	struct kill *nchunk;
	if (kused >= KBLOCK) {
		if ((nchunk = malloc(sizeof(struct kill))) == NULL)
			return FALSE;
		if (kheadp == NULL)
			kheadp = nchunk;
		if (kbufp != NULL)
			kbufp->k_next = nchunk;
		kbufp = nchunk;
		kbufp->k_next = NULL;
		kused = 0;
	}
	kbufp->k_chunk[kused++] = c;
	return TRUE;
}

/*
Deletes all of the text saved in the kill buffer.  Gets called by commands when
a new kill context is being created.  The kill buffer array is released just
in case the buffer has grown to immense size.
*/
void kdelete(void)
{
	struct kill *kp;
	if (kheadp == NULL)
		return;
	for (kbufp = kheadp; kbufp != NULL; kbufp = kp) {
		kp = kbufp->k_next;
		free(kbufp);
	}
	/* Resets all the kill buffer pointers */
	kheadp = NULL;
	kbufp = NULL;
	kused = KBLOCK;
}
