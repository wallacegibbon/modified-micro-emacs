/*
 * The functions in this file handle redisplay.  There are two halves, the
 * ones that update the virtual display screen, and the ones that make the
 * physical display screen the same as the virtual display screen.
 */

#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include "wrapper.h"
#include <errno.h>
#include <stdio.h>
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

static struct video **vscreen;	/* Virtual screen. */
static struct video **pscreen;	/* Physical screen. */

#if UNIX
#include <signal.h>
#endif

#ifdef SIGWINCH
/* Variables that will be accessed by signal handler */
static volatile sig_atomic_t screen_size_changed = 0;
static volatile sig_atomic_t is_updating = 0;
#endif

static int reframe(struct window *wp);
static int flush_to_physcr(void);
static void update_one(struct window *wp);
static void update_all(struct window *wp);
static int update_line(int row, struct video *vp1, struct video *vp2);
static void modeline(struct window *wp);

static void update_extended(void);
static void update_de_extend(void);
static void update_pos(void);

static int mlputs(char *s);
static int mlputi(int i, int r);
static int mlputli(long l, int r);
static int mlputf(int s);

#ifdef SIGWINCH
static void newscreensize(void);
#endif

static struct video *video_new(size_t text_size)
{
	struct video *vp = xmalloc(sizeof(*vp) + text_size);
	vp->v_flag = 0;
	return vp;
}

static void screen_init(void)
{
	int i;
	vscreen = xmalloc(term.t_nrow * sizeof(struct video *));
	pscreen = xmalloc(term.t_nrow * sizeof(struct video *));

	for (i = 0; i < term.t_nrow; ++i) {
		vscreen[i] = video_new(term.t_ncol);
		pscreen[i] = video_new(term.t_ncol);
	}
}

static void screen_deinit(void)
{
	int i;
	for (i = 0; i < term.t_nrow; ++i) {
		free(vscreen[i]);
		free(pscreen[i]);
	}
	free(vscreen);
	free(pscreen);
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
 * Write a character to the virtual screen.  The virtual row and column are
 * updated.  If we are not yet on left edge, don't print it yet.
 * If the line is too long put a "$" in the last column.
 *
 * This routine only puts printing characters into the virtual
 * terminal buffers.  Only column overflow is checked.
 */
static int vtputc(int c)
{
	struct video *vp;

	/* In case somebody passes us a signed char.. */
	if (c < 0) {
		c += 256;
		if (c < 0)
			return 0;
	}

	vp = vscreen[vtrow];

	if (vtcol >= term.t_ncol) {
		++vtcol;
		vp->v_text[term.t_ncol - 1] = '$';
		return 0;
	}

	if (c == '\t') {
		do {
			vtputc(' ');
		} while (((vtcol + taboff) & TABMASK) != 0);
		return 0;
	}

	if (!isvisible(c)) {
		put_c(c, vtputc);
		return 0;
	}

	if (vtcol >= 0)
		vp->v_text[vtcol] = c;

	++vtcol;
	return 0;
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

static int scrflags;

int update(int force)
{
	struct window *wp, *w;

#if VISMAC == 0
	if (force == FALSE && kbdmode == PLAY)
		return TRUE;
#endif

#if SIGWINCH
	/* It's NOT safe to change screen size from here. */
	is_updating = 1;
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
			if (wp->w_flag & (WFKILLS | WFINS)) {
				scrflags |= (wp->w_flag & (WFINS | WFKILLS));
				wp->w_flag &= ~(WFKILLS | WFINS);
			}
			if ((wp->w_flag & ~WFMODE) == WFEDIT)
				update_one(wp);
			else if (wp->w_flag & ~WFMOVE)
				update_all(wp);
			if (scrflags || (wp->w_flag & WFMODE))
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

#if SIGWINCH
	/* It's safe to change screen size now. */
	is_updating = 0;

	if (screen_size_changed)
		newscreensize();
#endif
	return TRUE;
}

/* Check to see if the cursor is on in the window and re-frame it if needed. */
static int reframe(struct window *wp)
{
	struct line *lp, *lp0;
	int i = 0;

	/* if not a requested reframe, check for a needed one */
	if ((wp->w_flag & WFFORCE) == 0) {
		/* loop from one line above the window to one line after */
		lp = wp->w_linep;
		lp0 = lback(lp);
		if (lp0 == wp->w_bufp->b_linep) {
			i = 0;
		} else {
			i = -1;
			lp = lp0;
		}
		for (; i <= wp->w_ntrows; ++i, lp = lforw(lp)) {
			/* if the line is in the window, no reframe */
			if (lp == wp->w_dotp) {
				/* if not _quite_ in, we'll reframe gently */
				if (i < 0 || i == wp->w_ntrows) {
					i = wp->w_force;
					break;
				}
				return TRUE;
			}
			/* if we are at the end of the file, reframe */
			if (lp == wp->w_bufp->b_linep)
				break;
		}
	}
	if (i == -1) {			/* we're just above the window */
		i = scrollcount;	/* put dot at first line */
		scrflags |= WFINS;
	} else if (i == wp->w_ntrows) {	/* we're just below the window */
		i = -scrollcount;	/* put dot at last line */
		scrflags |= WFKILLS;
	} else {			/* put dot where requested */
		i = wp->w_force;	/* (is 0, unless reposition() was called) */
	}

	wp->w_flag |= WFMODE;

	/* how far back to reframe? */
	if (i > 0) {		/* only one screen worth of lines max */
		if (--i >= wp->w_ntrows)
			i = wp->w_ntrows - 1;
	} else if (i < 0) {	/* negative update???? */
		i += wp->w_ntrows;
		if (i < 0)
			i = 0;
	} else {
		i = wp->w_ntrows / 2;
	}

	/* backup to new line at top of window */
	lp = wp->w_dotp;
	while (i-- && lback(lp) != wp->w_bufp->b_linep)
		lp = lback(lp);

	/* and reset the current line at top of window */
	wp->w_linep = lp;
	wp->w_flag |= WFHARD;
	wp->w_flag &= ~WFFORCE;
	return TRUE;
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
	sgarbf = FALSE;
	mpresf = FALSE;
}

static int flush_to_physcr(void)
{
	struct video *vp1;
	int i;
	for (i = 0; i < term.t_nrow; ++i) {
		vp1 = vscreen[i];
		if (vp1->v_flag & VFCHG)
			update_line(i, vp1, pscreen[i]);
	}
	return TRUE;
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
static int update_line(int row, struct video *vp1, struct video *vp2)
{
	char *cp1, *cp2, *cp3, *cp4;
	int rev, req, should_send_rev;

	cp1 = &vp1->v_text[0];
	cp2 = &vp2->v_text[0];

	cp3 = &vp1->v_text[term.t_ncol];
	cp4 = &vp2->v_text[term.t_ncol];

	/*
	 * This is why we need 2 flags for `rev`:
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

	/* update the needed flags */
	vp1->v_flag &= ~VFCHG;
	if (req)
		vp1->v_flag |= VFREV;
	else
		vp1->v_flag &= ~VFREV;

	return TRUE;

partial_update:
	/* Ignore common chars on the left */
	while (cp1 != cp3 && *cp1 == *cp2) {
		++cp1;
		++cp2;
	}

	/* if both lines are the same, no update needs to be done */
	if (cp1 == cp3) {
		vp1->v_flag &= ~VFCHG;
		return TRUE;
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

	vp1->v_flag &= ~VFCHG;	/* flag this line as updated */
	return TRUE;
}

/*
 * Redisplay the mode line for the window pointed to by the "wp".  This is the
 * only routine that has any idea of how the modeline is formatted.  You can
 * change the modeline format by hacking at this routine.  Called by "update"
 * any time there is a dirty window.
 */
static void modeline(struct window *wp)
{
	struct buffer *bp;
	char tline[NLINE];
	char *cp;
	int firstm = TRUE, lchar, c, n, i;

	n = wp->w_toprow + wp->w_ntrows;
	vscreen[n]->v_flag |= VFCHG | VFREQ;
	vtmove(n, 0);

	lchar = (wp == curwp) ? '-' : ' ';

	bp = wp->w_bufp;
	vtputc(lchar);

	if ((bp->b_flag & BFCHG) != 0)	/* "*" if changed. */
		vtputc('*');
	else
		vtputc(lchar);

	vtputc(' ');
	n = 3;

	cp = &bp->b_bname[0];
	while ((c = *cp++) != 0) {
		vtputc(c);
		++n;
	}

	strcpy(tline, " (");

	/* display the modes */

	if ((bp->b_flag & BFTRUNC) != 0) {
		firstm = FALSE;
		strcat(tline, "TRUNC");
	}
	for (i = 0; i < NMODES; ++i) {
		if (wp->w_bufp->b_mode & modevalue[i]) {
			if (firstm != TRUE)
				strcat(tline, " ");
			firstm = FALSE;
			strcat(tline, modename[i]);
		}
	}
	strcat(tline, ") ");

	cp = tline;
	while ((c = *cp++) != 0) {
		vtputc(c);
		++n;
	}

	if (bp->b_fname[0] != 0 && strcmp(bp->b_bname, bp->b_fname) != 0) {
		cp = &bp->b_fname[0];
		while ((c = *cp++) != 0) {
			vtputc(c);
			++n;
		}
		vtputc(' ');
		++n;
	}

	/* For very long filename, no space for other messages */
	if (n > term.t_ncol)
		return;

	while (n < term.t_ncol) {	/* Pad to full width. */
		vtputc(lchar);
		++n;
	}

#if RAMSHOW
	vtcol = n - 5 - 2 - 8 - 2;
#else
	vtcol = n - 5 - 2;
#endif

	{
		struct line *lp = wp->w_linep;
		int rows = wp->w_ntrows;
		char *msg = NULL;

		while (rows--) {
			lp = lforw(lp);
			if (lp == wp->w_bufp->b_linep) {
				msg = " Bot ";
				break;
			}
		}
		if (lback(wp->w_linep) == wp->w_bufp->b_linep) {
			if (msg) {
				if (wp->w_linep == wp->w_bufp->b_linep)
					msg = " Emp ";
				else
					msg = " All ";
			} else {
				msg = " Top ";
			}
		}
		if (!msg) {
			struct line *lp;
			int numlines, predlines, ratio;

			lp = lforw(bp->b_linep);
			numlines = 0;
			predlines = 0;
			while (lp != bp->b_linep) {
				if (lp == wp->w_linep) {
					predlines = numlines;
				}
				++numlines;
				lp = lforw(lp);
			}
			if (wp->w_dotp == bp->b_linep) {
				msg = " Bot ";
			} else {
				ratio = 0;
				if (numlines != 0)
					ratio = (100L * predlines) / numlines;
				if (ratio > 99)
					ratio = 99;
				sprintf(tline, " %2d%% ", ratio);
				msg = tline;
			}
		}

		for (cp = msg; (c = *cp) != 0; ++cp)
			vtputc(c);
	}

#if RAMSHOW
	vtcol += 2;

	{
#define GB (1024 * 1024 * 1024)
#define MB (1024 * 1024)
#define KB 1024
#define I(v, unit) ((short)((v) / (unit)))
#define D(v, unit) ((char)((v) * 10 / (unit) % 10))
		char s[16];
		if (envram >= 1000 * MB)
			sprintf(s, " %3d.%dG ", I(envram, GB), D(envram, GB));
		else if (envram >= 1000 * KB)
			sprintf(s, " %3d.%dM ", I(envram, MB), D(envram, MB));
		else if (envram >= 100000)
			sprintf(s, " %3d.%dK ", I(envram, KB), D(envram, KB));
		else
			sprintf(s, " %5d  ", (int)envram);
		for (cp = s; (c = *cp) != 0; ++cp)
			vtputc(c);
	}
#undef I
#undef D
#endif
}

void update_modelines(void)
{
	struct window *wp;
	for_each_wind(wp)
		wp->w_flag |= WFMODE;
}

/*
 * Set the virtual cursor to the specified row and column on the virtual screen.
 * There is no checking for nonsense values; this might be a good
 * idea during the early stages.
 */
void vtmove(int row, int col)
{
	vtrow = row;
	vtcol = col;
}

/*
 * Send a command to the terminal to move the hardware cursor to row "row"
 * and column "col".  The row and column arguments are origin 0.  Optimize out
 * random calls.  Update "ttrow" and "ttcol".
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
	movecursor(term.t_nrow, 0);
	TTeeol();
	TTflush();
	mpresf = FALSE;
}

int mlwrite(const char *fmt, ...)
{
	va_list ap;
	int n = 0, c, tmp;

	movecursor(term.t_nrow, 0);
	va_start(ap, fmt);
	while ((c = *fmt++) != 0) {
		if (c != '%') {
			tmp = put_c(c, TTputc);
			ttcol += tmp;
			n += tmp;
		} else {
			c = *fmt++;
			switch (c) {
			case 'd':
				n += mlputi(va_arg(ap, int), 10);
				break;

			case 'o':
				n += mlputi(va_arg(ap, int), 8);
				break;

			case 'x':
				n += mlputi(va_arg(ap, int), 16);
				break;

			case 'D':
				n += mlputli(va_arg(ap, long), 10);
				break;

			case 's':
				n += mlputs(va_arg(ap, char *));
				break;

			case 'f':
				n += mlputf(va_arg(ap, int));
				break;

			default:
				TTputc(c);
				++ttcol;
				++n;
			}
		}
	}
	va_end(ap);

	TTeeol();
	TTflush();
	mpresf = TRUE;
	return n;
}

static int mlputs(char *s)
{
	int n = 0, c, tmp;
	while ((c = *s++) != 0) {
		tmp = put_c(c, TTputc);
		n += tmp;
		ttcol += tmp;
	}
	return n;
}

static int mlputi(int i, int r)
{
	int q, n = 0;
	if (i < 0) {
		i = -i;
		TTputc('-');
		++n;
	}

	q = i / r;
	if (q != 0)
		n += mlputi(q, r);

	TTputc(hex[i % r]);
	++ttcol;
	return n + 1;
}

static int mlputli(long l, int r)
{
	long q, n = 0;
	if (l < 0) {
		l = -l;
		TTputc('-');
		++n;
	}

	q = l / r;
	if (q != 0)
		n += mlputli(q, r);

	TTputc((int)(l % r) + '0');
	++ttcol;
	return n + 1;
}

/* write out a scaled integer with two decimal places */
static int mlputf(int s)
{
	int i, f, n = 0;

	/* break it up */
	i = s / 100;
	f = s % 100;

	/* send out the integer portion */
	n += mlputi(i, 10);
	TTputc('.');
	TTputc((f / 10) + '0');
	TTputc((f % 10) + '0');
	ttcol += 3;
	return n + 3;
}

#ifdef SIGWINCH
void sizesignal(int signr)
{
	int old_errno = errno;

	screen_size_changed = 1;
	signal(SIGWINCH, sizesignal);

	errno = old_errno;

	/* Make `newscreensize` called in all cases */
	if (!is_updating)
		update(TRUE);
}

static void newscreensize(void)
{
	if (screen_size_changed == 0)
		return;

	screen_size_changed = 0;

	screen_deinit();

	/* Re-open the terminal to get the new size of the screen */
	TTclose();
	TTopen();

	screen_init();

	adjust_on_scr_resize();
	update(TRUE);
}

#endif

int put_c(unsigned char c, int (*p)(int))
{
	if (c < 0x20 || c == 0x7F) {
		p('^'); p(c ^ 0x40); return 2;
	} else if (c >= 0x20 && c < 0x7F) {
		p(c); return 1;
	} else {
		p('<'); p(hex[c >> 4]); p(hex[c & 0xF]); p('>'); return 4;
	}
}

/* Keep `next_col` sync with `put_c` */
int next_col(int col, unsigned char c)
{
	if (c == '\t')
		return (col | TABMASK) + 1;
	else if (c < 0x20 || c == 0x7F)
		return col + 2;
	else if (c >= 0x20 && c < 0x7F)
		return col + 1;
	else
		return col + 4;
}
