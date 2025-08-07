/*
 * The functions in this file handle redisplay.  There are two halves, the
 * ones that update the virtual display screen, and the ones that make the
 * physical display screen the same as the virtual display screen.
 */

#include "me.h"
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

struct video {
	int v_flag;		/* Flags */
	char v_text[];		/* Row data on screen. */
};

#define VFCHG   0x0001		/* Changed flag */
#define VFEXT	0x0002		/* extended (beyond max column) */
#define VFREV	0x0004		/* reverse video status */
#define VFREQ	0x0008		/* reverse video request */

static void reframe(struct window *wp);
static void flush_to_physcr(void);
static void update_one(struct window *wp);
static void update_all(struct window *wp);
static void update_line(int row, struct video *vp1, struct video *vp2);
static void modeline(struct window *wp);

static void update_extended(void);
static void update_de_extend(void);
static void update_pos(void);

static int mlflush(void);

static struct video **vscreen;	/* Virtual screen. */
static struct video **pscreen;	/* Physical screen. */
static char *mlbuf = NULL;	/* Message line buffer */
static size_t mlbuf_size = 0;

static struct video *video_new(size_t text_size)
{
	struct video *vp;
	if ((vp = malloc(sizeof(*vp) + text_size)) == NULL)
		return NULL;

	vp->v_flag = 0;
	return vp;
}

static void screen_init(void)
{
	int i, failflag = 0;
	char *mlbuf_old;

	display_ok = 0;

	if ((vscreen = malloc(term.t_nrow * sizeof(struct video *))) == NULL)
		return;
	if ((pscreen = malloc(term.t_nrow * sizeof(struct video *))) == NULL)
		goto fail1;

	for (i = 0; i < term.t_nrow; ++i) {
		if ((vscreen[i] = video_new(term.t_ncol)) == NULL)
			failflag = 1;
		if ((pscreen[i] = video_new(term.t_ncol)) == NULL)
			failflag = 1;
	}
	if (failflag)
		goto fail2;

	mlbuf_old = mlbuf;
	mlbuf_size = term.t_ncol + 1;
	if ((mlbuf = malloc(mlbuf_size)) == NULL)
		goto fail2;

	if (mlbuf_old != NULL) {
		strncpy_safe(mlbuf, mlbuf_old, mlbuf_size);
		free(mlbuf_old);
	} else {
		mlbuf[0] = '\0';
	}

	display_ok = 1;
	return;

fail2:
	for (i = 0; i < term.t_nrow; ++i) {
		free(pscreen[i]);
		free(vscreen[i]);
	}
	free(pscreen);

fail1:
	free(vscreen);
}

static void screen_deinit(void)
{
	int i;
	display_ok = 0;
	for (i = 0; i < term.t_nrow; ++i) {
		free(pscreen[i]);
		free(vscreen[i]);
	}
	free(pscreen);
	free(vscreen);
}

void vtinit(void)
{
	TTopen();
	TTrev(FALSE);
	screen_init();
}

void vtdeinit(void)
{
	screen_deinit();
	/* mlbuf is not like screen lines, it keeps contents on resizing */
	free(mlbuf);
	TTclose();
}

/*
 * Clean up the virtual terminal system, in anticipation for a return to the
 * operating system.  Move down to the last line and clear it out (the next
 * system prompt will be written in the line).  Shut down the channel to the
 * terminal.
 */
void vttidy(void)
{
	int r;
	mlerase();
	movecursor(term.t_nrow, 0);
	TTflush();
	TTclose();
	r = write(1, "\r", 1);
	(void)r;
}

/*
 * Write a character to the virtual screen.
 * If the line is too long put a "$" in the last column.
 */
static void vtputc(int c)
{
	struct video *vp = vscreen[vtrow];
	if (vtcol >= term.t_ncol) {
		vp->v_text[term.t_ncol - 1] = '$';
		++vtcol;
		return;
	}
	if (c == '\t') {
		do { vtputc(' '); }
		while (((vtcol + taboff) & TABMASK) != 0);
		return;
	}
	if (!isvisible(c)) {
		put_c(c, vtputc);
		return;
	}
	if (vtcol >= 0) {
		vp->v_text[vtcol] = c;
	}
	++vtcol;
}

static void vtputs(const char *s)
{
	int c;
	while ((c = *s++) != '\0')
		vtputc(c);
}

/*
 * Erase from the end of the software cursor to the end of the line on which
 * the software cursor is located.
 */
static void vteeol(void)
{
	char *vcp = vscreen[vtrow]->v_text;
	while (vtcol < term.t_ncol)
		vcp[vtcol++] = ' ';
}

int update(int force)
{
	struct window *wp, *w;

	if (!display_ok) {
		fprintf(stderr, "Display is not ready, nothing to update.");
		return FALSE;
	}

#if VISMAC == 0
	if (force == FALSE && kbdmode == PLAY)
		return TRUE;
#endif

	/*
	 * first, propagate mode line changes to all instances of a buffer
	 * displayed in more than one window.
	 */
	for_each_wind(wp) {
		if (wp->w_flag & WFMODE) {
			if (wp->w_bufp->b_nwnd > 1) {
				for_each_wind(w) {
					if (w->w_bufp == wp->w_bufp)
						w->w_flag |= WFMODE;
				}
			}
		}
	}

	/* update any windows that need refreshing */
	for_each_wind(wp) {
		if (wp->w_flag) {
			reframe(wp);
			if ((wp->w_flag & ~WFMODE) == WFEDIT)
				update_one(wp);
			else if (wp->w_flag & ~WFMOVE)
				update_all(wp);
			if (wp->w_flag & WFMODE)
				modeline(wp);
			wp->w_flag = 0;
			wp->w_force = 0;
		}
	}

	update_pos();
	update_de_extend();

	if (sgarbf != FALSE)
		update_garbage();

	flush_to_physcr();
	movecursor(currow, curcol - lbound);

	TTflush();
	return TRUE;
}

/* Check to see if the cursor is in the window and re-frame it if needed. */
static void reframe(struct window *wp)
{
	struct line *lp;
	int i;

	if (!(wp->w_flag & WFFORCE)) {
		lp = wp->w_linep;
		for (i = wp->w_ntrows; i && lp != wp->w_bufp->b_linep; --i) {
			if (lp == wp->w_dotp)
				return;
			lp = lforw(lp);
		}
		if (i > 0 && lp == wp->w_dotp)	/* w_dotp == b_linep */
			return;
	}

	if (wp->w_flag & WFFORCE)
		i = inside(wp->w_force - 1, 0, wp->w_ntrows - 1);
	else
		i = wp->w_ntrows / 2;

	lp = wp->w_dotp;
	while (i-- && lback(lp) != wp->w_bufp->b_linep)
		lp = lback(lp);

	wp->w_linep = lp;
	wp->w_flag |= WFHARD;
	wp->w_flag &= ~WFFORCE;
}

static void show_line(struct line *lp)
{
	int i, len;
	for (i = 0, len = llength(lp); i < len; ++i)
		vtputc(lgetc(lp, i));
}

/* Update the current line to the virtual screen */
static void update_one(struct window *wp)
{
	struct line *lp = wp->w_linep;
	int i = wp->w_toprow;

	for (; lp != wp->w_dotp; lp = lforw(lp))
		++i;

	/* and update the virtual line */
	vscreen[i]->v_flag |= VFCHG;
	vscreen[i]->v_flag &= ~VFREQ;
	vtmove(i, 0);
	show_line(lp);
	vteeol();
}

/* Update all the lines in a window on the virtual screen */
static void update_all(struct window *wp)
{
	struct line *lp = wp->w_linep;
	int i = wp->w_toprow, j = i + wp->w_ntrows;

	for (; i < j; ++i) {
		vscreen[i]->v_flag |= VFCHG;
		vscreen[i]->v_flag &= ~VFREQ;
		vtmove(i, 0);
		if (lp != wp->w_bufp->b_linep) {
			show_line(lp);
			lp = lforw(lp);
		}
		vteeol();
	}
}

/*
 * Update the position of the hardware cursor and handle extended lines.
 * This is the only update for simple moves.
 */
static void update_pos(void)
{
	struct line *lp = curwp->w_linep;
	int i;

	currow = curwp->w_toprow;
	for (; lp != curwp->w_dotp; lp = lforw(lp))
		++currow;

	curcol = 0;
	for (i = 0; i < curwp->w_doto; ++i)
		curcol = next_col(curcol, lgetc(lp, i));

	/* if extended, flag so and update the virtual line image */
	if (curcol >= term.t_ncol - 1) {
		vscreen[currow]->v_flag |= (VFEXT | VFCHG);
		update_extended();
	} else {
		lbound = 0;
	}
}

static void update_de_extend_wind(struct window *wp)
{
	struct line *lp;
	int i = wp->w_toprow;
	int j = i + wp->w_ntrows;

	for (lp = wp->w_linep; i < j; ++i, lp = lforw(lp)) {
		if (!(vscreen[i]->v_flag & VFEXT))
			continue;
		if ((lp == wp->w_dotp) && (curcol >= term.t_ncol - 1))
			continue;

		/* Have VFEXT flag, and curcol is small, de-extend */
		vtmove(i, 0);
		show_line(lp);
		vteeol();
		vscreen[i]->v_flag &= ~VFEXT;
		vscreen[i]->v_flag |= VFCHG;
	}
}

/* de-extend any line that derserves it */
static void update_de_extend(void)
{
	struct window *wp;
	for_each_wind(wp)
		update_de_extend_wind(wp);
}

/*
 * If the screen is garbage, clear the physical screen and the virtual screen
 * and force a full update.
 */
void update_garbage(void)
{
	char *txt;
	int i, j;

	for (i = 0; i < term.t_nrow; ++i) {
		vscreen[i]->v_flag |= VFCHG;
		vscreen[i]->v_flag &= ~VFREV;
		txt = pscreen[i]->v_text;
		for (j = 0; j < term.t_ncol; ++j)
			txt[j] = ' ';
	}

	movecursor(0, 0);
	TTeeop();
	mlflush();
	sgarbf = FALSE;
}

static void flush_to_physcr(void)
{
	struct video *vp1;
	int i;
	for (i = 0; i < term.t_nrow; ++i) {
		vp1 = vscreen[i];
		if (vp1->v_flag & VFCHG)
			update_line(i, vp1, pscreen[i]);
	}
}

/*
 * Update the extended line which the cursor is currently on at a column
 * greater than the terminal width.  The line will be scrolled right or left
 * to let the user see where the cursor is.
 */
static void update_extended(void)
{
	struct line *lp = curwp->w_dotp;
	int rcursor;

	/* calculate what column the real cursor will end up in */
	rcursor = ((curcol - term.t_ncol) % term.t_scrsiz) + term.t_margin;
	taboff = lbound = curcol - rcursor + 1;

	/* scan through the line outputing characters to the virtual screen */
	/* once we reach the left edge */
	vtmove(currow, -lbound);	/* start scanning offscreen */
	show_line(lp);

	/* truncate the virtual line, restore tab offset */
	vteeol();
	taboff = 0;

	/* and put a '$' in column 1 */
	vscreen[currow]->v_text[0] = '$';
}

/* Update the line to terminal.  The physical column will be updated. */
static void update_line(int row, struct video *vp1, struct video *vp2)
{
	char *cp1, *cp2, *cp3, *cp4;
	int rev, req, should_send_rev;

	cp1 = &vp1->v_text[0];
	cp2 = &vp2->v_text[0];
	cp3 = &vp1->v_text[term.t_ncol];
	cp4 = &vp2->v_text[term.t_ncol];

	/*
	 * This is why we need 2 flags (rev and req):
	 *
	 * If we only have one flag, we can not tell the difference between
	 * a line becoming normal from reversed (e.g. when a window got killed,
	 * its modeline will be replaced by a normal line of another window)
	 * and a line who has always been normal, the former one need a full
	 * redraw to cleanup color.
	 */

	rev = (vp1->v_flag & VFREV) == VFREV;
	req = (vp1->v_flag & VFREQ) == VFREQ;
	if (rev != req) {
		/* Becoming reversed or becoming normal need a full update. */
		should_send_rev = req;
		goto full_update;
	} else if (rev) {
		/* Was reversed, and is still reversed. */
		/* Partial update of reversed content should send rev, too */
		should_send_rev = 1;
		goto partial_update;
	} else {
		should_send_rev = 0;
		goto partial_update;
	}

full_update:
	movecursor(row, 0);
	if (should_send_rev)
		TTrev(TRUE);

	/* Dump virtual line to both physical line and the terminal */
	while (cp1 < cp3) {
		TTputc(*cp1);
		++ttcol;
		*cp2++ = *cp1++;
	}
	if (should_send_rev)
		TTrev(FALSE);
	vp1->v_flag &= ~VFCHG;
	if (req)
		vp1->v_flag |= VFREV;
	else
		vp1->v_flag &= ~VFREV;

	return;

partial_update:
	/* Ignore common chars on the left */
	while (cp1 != cp3 && *cp1 == *cp2) {
		++cp1;
		++cp2;
	}
	/* if both lines are the same, no update needs to be done */
	if (cp1 == cp3) {
		vp1->v_flag &= ~VFCHG;
		return;
	}
	/* Ignore common chars on the right */
	while (cp3[-1] == cp4[-1]) {
		--cp3;
		--cp4;
	}
	movecursor(row, cp1 - &vp1->v_text[0]);
	if (should_send_rev)
		TTrev(TRUE);
	while (cp1 != cp3) {
		TTputc(*cp1);
		++ttcol;
		*cp2++ = *cp1++;
	}
	if (should_send_rev)
		TTrev(FALSE);

	vp1->v_flag &= ~VFCHG;
}

/*
 * Redisplay the mode line for the window pointed to by the "wp".  This is the
 * only routine that has any idea of how the modeline is formatted.
 */
static void modeline(struct window *wp)
{
	struct buffer *bp = wp->w_bufp;
	int n = wp->w_toprow + wp->w_ntrows;

	vscreen[n]->v_flag |= VFCHG | VFREQ;
	vtmove(n, 0);

	vtputs(wp == curwp ? "@" : " ");
	if (bp->b_flag & BFCHG)
		vtputs(bp->b_flag & BFRDONLY ? "%* " : "** ");
	else
		vtputs(bp->b_flag & BFRDONLY ? "%% " : "   ");

	if (bp->b_flag & BFTRUNC)
		vtputs("(TRUNC) ");
	vtputs(bp->b_fname);

	while (vtcol < term.t_ncol)
		vtputc(' ');
}

void update_modelines(void)
{
	struct window *wp;
	for_each_wind(wp)
		wp->w_flag |= WFMODE;
}

void vtmove(int row, int col)
{
	vtrow = row;
	vtcol = col;
}

/*
 * Send a command to the terminal to move the hardware cursor to row "row"
 * and column "col".  The row and column arguments are origin 0.
 */
void movecursor(int row, int col)
{
	if (row != ttrow || col != ttcol) {
		ttrow = row;
		ttcol = col;
		TTmove(row, col);
	}
}

/* The message line is not considered to be part of the virtual screen. */

void mlerase(void)
{
	if (mlbuf != NULL)
		mlbuf[0] = '\0';

	movecursor(term.t_nrow, 0);
	TTeeol();
	TTflush();
	mpresf = FALSE;
}

int mlwrite(const char *fmt, ...)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = mlvwrite(fmt, ap);
	va_end(ap);

	return n;
}

int mlvwrite(const char *fmt, va_list ap)
{
	int n;
	if (mlbuf == NULL)
		return 0;

	vsnprintf(mlbuf, mlbuf_size, fmt, ap);
	n = mlflush();

	TTflush();
	return n;
}

static int mlflush(void)
{
	char *s = mlbuf, c;

	movecursor(term.t_nrow, 0);
	while ((c = *s++))
		ttcol += put_c(c, TTputc);

	TTeeol();
	mpresf = TRUE;
	return ttcol;
}

static inline void ttputs(char *s)
{
	int c;
	while ((c = *s++))
		TTputc(c);
}

int unput_c(unsigned char c)
{
	if (c < 0x20 || c == 0x7F) {
		ttputs("\b\b  \b\b"); return 2;
	} else if (c < 0x7F) {
		ttputs("\b \b"); return 1;
	} else {
		ttputs("\b\b\b   \b\b\b"); return 3;
	}
}

int put_c(unsigned char c, void (*p)(int))
{
	if (c < 0x20 || c == 0x7F) {
		p('^'); p(c ^ 0x40); return 2;
	} else if (c < 0x7F) {
		p(c); return 1;
	} else {
		p('\\'); p(hex[c >> 4]); p(hex[c & 0xF]); return 3;
	}
}

int next_col(int col, unsigned char c)
{
	if (c == '\t')
		return (col | TABMASK) + 1;
	else if (c < 0x20 || c == 0x7F)
		return col + 2;
	else if (c < 0x7F)
		return col + 1;
	else
		return col + 3;
}
