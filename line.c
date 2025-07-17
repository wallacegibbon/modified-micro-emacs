/*
 * Note that this code only updates the dot and mark values in the window list.
 * Since all the code acts on the current window, the buffer that we are
 * editing must be being displayed, which means that "b_nwnd" is non zero,
 * which means that the dot and mark values in the buffer headers are nonsense.
 *
 * There are routines in this file that handle the kill buffer too.
 * It isn't here for any good reason.
 */

#include "estruct.h"
#include "line.h"
#include "edef.h"
#include "efunc.h"

#define BLOCK_SIZE 16 /* Line block chunk size. */

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
 * buffer.  It updates all of the required flags in the buffer and window
 * system.  The flag used is passed as an argument; if the buffer is being
 * displayed in more than 1 window we change EDIT t HARD.  Set MODE if the
 * mode line needs to be updated (the "*" has to be set).
 */
void lchange(int flag)
{
	struct window *wp;

	if (curbp->b_nwnd != 1)	/* Ensure hard. */
		flag = WFHARD;
	if ((curbp->b_flag & BFCHG) == 0) {	/* First change, so */
		flag |= WFMODE;	/* update mode lines. */
		curbp->b_flag |= BFCHG;
	}
	for_each_wind(wp) {
		if (wp->w_bufp == curbp)
			wp->w_flag |= flag;
	}
}

/* linstr -- Insert a string at the current point. */
int linstr(char *instr)
{
	int status = TRUE;
	char tmpc;

	if (instr != NULL) {
		while ((tmpc = *instr) && status == TRUE) {
			status = (tmpc == '\n' ? lnewline() : linsert(1, tmpc));
			if (status != TRUE) {
				mlwrite("%%Out of memory while inserting");
				break;
			}
			++instr;
		}
	}
	return status;
}

/*
 * Insert "n" copies of the character "c" at the current location of dot.
 * In the easy case all that happens is the text is stored in the line.
 * In the hard case, the line has to be reallocated.
 * When the window list is updated, take special care; I screwed it up once.
 * You always update dot in the current window.
 * You update mark, and a dot in another window, if it is greater than
 * the place where you did the insert.
 * Return TRUE if all is well, and FALSE on errors.
 */
int linsert(int n, int c)
{
	struct line *lp1, *lp2, *lp3;
	struct window *wp;
	char *cp1, *cp2;
	int doto, i;

	if (curbp->rdonly)
		return rdonly();

	lchange(WFEDIT);
	lp1 = curwp->w_dotp;	/* Current line */
	if (lp1 == curbp->b_linep) {	/* At the end: special */
		if (curwp->w_doto != 0) {
			mlwrite("bug: linsert");
			return FALSE;
		}
		if ((lp2 = lalloc(n)) == NULL)	/* Allocate new line */
			return FALSE;
		lp3 = lp1->l_bp;	/* Previous line */
		lp3->l_fp = lp2;	/* Link in */
		lp2->l_fp = lp1;
		lp1->l_bp = lp2;
		lp2->l_bp = lp3;
		for (i = 0; i < n; ++i)
			lp2->l_text[i] = c;
		curwp->w_dotp = lp2;
		curwp->w_doto = n;
		return TRUE;
	}
	doto = curwp->w_doto;
	if (lp1->l_used + n > lp1->l_size) {	/* Hard: reallocate */
		if ((lp2 = lalloc(lp1->l_used + n)) == NULL)
			return FALSE;
		cp1 = &lp1->l_text[0];
		cp2 = &lp2->l_text[0];
		while (cp1 != &lp1->l_text[doto])
			*cp2++ = *cp1++;
		cp2 += n;
		while (cp1 != &lp1->l_text[lp1->l_used])
			*cp2++ = *cp1++;
		lp1->l_bp->l_fp = lp2;
		lp2->l_fp = lp1->l_fp;
		lp1->l_fp->l_bp = lp2;
		lp2->l_bp = lp1->l_bp;
		free(lp1);
	} else {		/* Easy: in place */
		lp2 = lp1;	/* Pretend new line */
		lp2->l_used += n;
		cp2 = &lp1->l_text[lp1->l_used];
		cp1 = cp2 - n;
		while (cp1 != &lp1->l_text[doto])
			*--cp2 = *--cp1;
	}

	for (i = 0; i < n; ++i)
		lp2->l_text[doto + i] = c;

	for_each_wind(wp) {
		if (wp->w_linep == lp1)
			wp->w_linep = lp2;
		if (wp->w_dotp == lp1) {
			wp->w_dotp = lp2;
			if (wp == curwp || wp->w_doto > doto)
				wp->w_doto += n;
		}
		if (wp->w_markp == lp1) {
			wp->w_markp = lp2;
			if (wp->w_marko > doto)
				wp->w_marko += n;
		}
	}
	return TRUE;
}

/*
 * Insert a newline into the buffer at the current location of dot in the
 * current window.  The funny ass-backwards way it does things is not a botch;
 * it just makes the last line in the file not a special case.  Return TRUE if
 * everything works out and FALSE on error (memory allocation failure).  The
 * update of dot and mark is a bit easier then in the above case, because the
 * split forces more updating.
 */
int lnewline(void)
{
	struct line *lp1, *lp2;
	struct window *wp;
	char *cp1, *cp2;
	int doto;

	if (curbp->rdonly)
		return rdonly();

	lchange(WFHARD | WFINS);
	lp1 = curwp->w_dotp;		/* Get the address and */
	doto = curwp->w_doto;		/* offset of "." */
	if ((lp2 = lalloc(doto)) == NULL)	/* New first half line */
		return FALSE;
	cp1 = &lp1->l_text[0];	/* Shuffle text around */
	cp2 = &lp2->l_text[0];
	while (cp1 != &lp1->l_text[doto])
		*cp2++ = *cp1++;
	cp2 = &lp1->l_text[0];
	while (cp1 != &lp1->l_text[lp1->l_used])
		*cp2++ = *cp1++;
	lp1->l_used -= doto;
	lp2->l_bp = lp1->l_bp;
	lp1->l_bp = lp2;
	lp2->l_bp->l_fp = lp2;
	lp2->l_fp = lp1;
	for_each_wind(wp) {
		if (wp->w_linep == lp1)
			wp->w_linep = lp2;
		if (wp->w_dotp == lp1) {
			if (wp->w_doto < doto)
				wp->w_dotp = lp2;
			else
				wp->w_doto -= doto;
		}
		if (wp->w_markp == lp1) {
			if (wp->w_marko < doto)
				wp->w_markp = lp2;
			else
				wp->w_marko -= doto;
		}
	}
	return TRUE;
}

/*
 * This function deletes "n" bytes, starting at dot.  It understands how do deal
 * with end of lines, etc.  It returns TRUE if all of the characters were
 * deleted, and FALSE if they were not (because dot ran into the end of the
 * buffer.  The "kflag" is TRUE if the text should be put in the kill buffer.
 *
 * long n;		# of chars to delete
 * int kflag;		 put killed text in kill buffer flag
 */
int ldelete(long n, int kflag)
{
	struct window *wp;
	struct line *dotp;
	char *cp1, *cp2;
	int doto, chunk;

	if (curbp->rdonly)
		return rdonly();
	while (n != 0) {
		dotp = curwp->w_dotp;
		doto = curwp->w_doto;
		if (dotp == curbp->b_linep)	/* Hit end of buffer. */
			return FALSE;
		chunk = dotp->l_used - doto;	/* Size of chunk. */
		if (chunk > n)
			chunk = n;
		if (chunk == 0) {	/* End of line, merge. */
			lchange(WFHARD | WFKILLS);
			if (ldelnewline() == FALSE ||
					(kflag != FALSE &&
					kinsert('\n') == FALSE))
				return FALSE;
			--n;
			continue;
		}
		lchange(WFEDIT);
		cp1 = &dotp->l_text[doto];	/* Scrunch text. */
		cp2 = cp1 + chunk;
		if (kflag != FALSE) {	/* Kill? */
			while (cp1 != cp2) {
				if (kinsert(*cp1) == FALSE)
					return FALSE;
				++cp1;
			}
			cp1 = &dotp->l_text[doto];
		}
		while (cp2 != &dotp->l_text[dotp->l_used])
			*cp1++ = *cp2++;
		dotp->l_used -= chunk;
		for_each_wind(wp) {
			if (wp->w_dotp == dotp && wp->w_doto >= doto) {
				wp->w_doto -= chunk;
				if (wp->w_doto < doto)
					wp->w_doto = doto;
			}
			if (wp->w_markp == dotp && wp->w_marko >= doto) {
				wp->w_marko -= chunk;
				if (wp->w_marko < doto)
					wp->w_marko = doto;
			}
		}
		n -= chunk;
	}
	return TRUE;
}

/*
 * Delete a newline.  Join the current line with the next line.  If the next line
 * is the magic header line always return TRUE; merging the last line with the
 * header line can be thought of as always being a successful operation, even
 * if nothing is done, and this makes the kill buffer work "right".  Easy cases
 * can be done by shuffling data around.  Hard cases require that lines be moved
 * about in memory.  Return FALSE on error and TRUE if all looks ok.  Called by
 * "ldelete" only.
 */
int ldelnewline(void)
{
	struct line *lp1, *lp2, *lp3;
	struct window *wp;
	char *cp1, *cp2;

	if (curbp->rdonly)
		return rdonly();
	lp1 = curwp->w_dotp;
	lp2 = lp1->l_fp;
	if (lp2 == curbp->b_linep) {	/* At the buffer end. */
		if (lp1->l_used == 0)	/* Blank line. */
			lfree(lp1);
		return TRUE;
	}
	if (lp2->l_used <= lp1->l_size - lp1->l_used) {
		cp1 = &lp1->l_text[lp1->l_used];
		cp2 = &lp2->l_text[0];
		while (cp2 != &lp2->l_text[lp2->l_used])
			*cp1++ = *cp2++;
		for_each_wind(wp) {
			if (wp->w_linep == lp2)
				wp->w_linep = lp1;
			if (wp->w_dotp == lp2) {
				wp->w_dotp = lp1;
				wp->w_doto += lp1->l_used;
			}
			if (wp->w_markp == lp2) {
				wp->w_markp = lp1;
				wp->w_marko += lp1->l_used;
			}
		}
		lp1->l_used += lp2->l_used;
		lp1->l_fp = lp2->l_fp;
		lp2->l_fp->l_bp = lp1;
		free(lp2);
		return TRUE;
	}
	if ((lp3 = lalloc(lp1->l_used + lp2->l_used)) == NULL)
		return FALSE;
	cp1 = &lp1->l_text[0];
	cp2 = &lp3->l_text[0];
	while (cp1 != &lp1->l_text[lp1->l_used])
		*cp2++ = *cp1++;
	cp1 = &lp2->l_text[0];
	while (cp1 != &lp2->l_text[lp2->l_used])
		*cp2++ = *cp1++;
	lp1->l_bp->l_fp = lp3;
	lp3->l_fp = lp2->l_fp;
	lp2->l_fp->l_bp = lp3;
	lp3->l_bp = lp1->l_bp;
	for_each_wind(wp) {
		if (wp->w_linep == lp1 || wp->w_linep == lp2)
			wp->w_linep = lp3;
		if (wp->w_dotp == lp1) {
			wp->w_dotp = lp3;
		} else if (wp->w_dotp == lp2) {
			wp->w_dotp = lp3;
			wp->w_doto += lp1->l_used;
		}
		if (wp->w_markp == lp1) {
			wp->w_markp = lp3;
		} else if (wp->w_markp == lp2) {
			wp->w_markp = lp3;
			wp->w_marko += lp1->l_used;
		}
	}
	free(lp1);
	free(lp2);
	return TRUE;
}

/*
 * Delete all of the text saved in the kill buffer.  Called by commands when a
 * new kill context is being created.  The kill buffer array is released, just
 * in case the buffer has grown to immense size.  No errors.
 */
void kdelete(void)
{
	struct kill *kp;

	if (kbufh == NULL)
		return;

	kbufp = kbufh;
	while (kbufp != NULL) {
		kp = kbufp->d_next;
		free(kbufp);
		kbufp = kp;
	}
	/* Reset all the kill buffer pointers */
	kbufh = kbufp = NULL;
	kused = KBLOCK;
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

/* Yank text back from the kill buffer. */
int yank(int f, int n)
{
	struct kill *kp;
	char *sp;
	int c, i;

	if (curbp->rdonly)
		return rdonly();
	if (n < 0)
		return FALSE;

	if (kbufh == NULL)
		return TRUE;	/* not an error, just nothing */

	while (n--) {
		for_each_kbuf(kp) {
			i = kp->d_next == NULL ? kused : KBLOCK;
			sp = kp->d_chunk;
			while (i--) {
				if ((c = *sp++) == '\n') {
					if (lnewline() == FALSE)
						return FALSE;
				} else {
					if (linsert(1, c) == FALSE)
						return FALSE;
				}
			}
		}
	}
	return TRUE;
}
