#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include "wrapper.h"

/*
 * With no argument, it just does the refresh.  With an argument "n",
 * reposition dot to line "n" of the screen.
 */
int redraw(int f, int n)
{
	if (f == FALSE) {
		sgarbf = TRUE;
	} else {
		curwp->w_force = n;	/* Center dot. */
		curwp->w_flag |= WFFORCE;
	}
	return TRUE;
}

/*
 * The command make the next window the current window.
 * With an positive argument, finds the <n>th window from the top.
 * With a negative argument, finds the <n>th window from the bottom
 */
int nextwind(int f, int n)
{
	struct window *wp;
	int nwindows;

	if (f) {
		nwindows = 1;
		for (wp = wheadp; wp->w_wndp != NULL; wp = wp->w_wndp)
			++nwindows;

		if (n < 0)
			n = nwindows + n + 1;

		/* if an argument, give them that window from the top */
		if (n > 0 && n <= nwindows) {
			wp = wheadp;
			while (--n)
				wp = wp->w_wndp;
		} else {
			mlwrite("Window number out of range");
			return FALSE;
		}
	} else if ((wp = curwp->w_wndp) == NULL) {
		wp = wheadp;
	}
	curwp = wp;
	curbp = wp->w_bufp;
	update_modelines();
	return TRUE;
}

/* The reverse of nextwind */
int prevwind(int f, int n)
{
	struct window *wp1, *wp2;

	/* if we have an argument, we mean the nth window from the bottom */
	if (f)
		return nextwind(f, -n);

	wp1 = wheadp;
	wp2 = curwp;

	if (wp1 == wp2)
		wp2 = NULL;

	while (wp1->w_wndp != wp2)
		wp1 = wp1->w_wndp;

	curwp = wp1;
	curbp = wp1->w_bufp;
	update_modelines();
	return TRUE;
}

/*
 * This command makes the current window the only window on the screen.
 * Try to set the framing so that "." does not have to move on the display.
 * Some care has to be taken to keep the values of dot and mark in the buffer
 * structures right if the distruction of a window makes a buffer become
 * undisplayed.
 */
int onlywind(int f, int n)
{
	struct window *wp;
	struct line *lp;
	int i;

	while (wheadp != curwp) {
		wp = wheadp;
		wheadp = wp->w_wndp;
		if (--wp->w_bufp->b_nwnd == 0) {
			wp->w_bufp->b_dotp = wp->w_dotp;
			wp->w_bufp->b_doto = wp->w_doto;
			wp->w_bufp->b_markp = wp->w_markp;
			wp->w_bufp->b_marko = wp->w_marko;
		}
		free(wp);
	}
	while (curwp->w_wndp != NULL) {
		wp = curwp->w_wndp;
		curwp->w_wndp = wp->w_wndp;
		if (--wp->w_bufp->b_nwnd == 0) {
			wp->w_bufp->b_dotp = wp->w_dotp;
			wp->w_bufp->b_doto = wp->w_doto;
			wp->w_bufp->b_markp = wp->w_markp;
			wp->w_bufp->b_marko = wp->w_marko;
		}
		free(wp);
	}
	lp = curwp->w_linep;
	i = curwp->w_toprow;
	while (i != 0 && lback(lp) != curbp->b_linep) {
		--i;
		lp = lback(lp);
	}
	curwp->w_toprow = 0;
	curwp->w_ntrows = term.t_nrow - 1;
	curwp->w_linep = lp;
	curwp->w_flag |= WFMODE | WFHARD;
	return TRUE;
}

/*
 * Delete the current window, placing its space in the window above,
 * or, if it is the top window, the window below.
 */
int delwind(int f, int n)
{
	struct window *wp;	/* window to recieve deleted space */
	struct window *lwp;	/* ptr window before curwp */
	int target;		/* target line to search for */

	/* if there is only one window, don't delete it */
	if (wheadp->w_wndp == NULL) {
		mlwrite("Can not delete this window");
		return FALSE;
	}

	/* find window before curwp in linked list */
	wp = wheadp;
	lwp = NULL;
	while (wp != NULL) {
		if (wp == curwp)
			break;
		lwp = wp;
		wp = wp->w_wndp;
	}

	/* find recieving window and give up our space */
	wp = wheadp;
	if (curwp->w_toprow == 0) {
		/* find the next window down */
		target = curwp->w_ntrows + 1;
		while (wp != NULL) {
			if (wp->w_toprow == target)
				break;
			wp = wp->w_wndp;
		}
		if (wp == NULL)
			return FALSE;
		wp->w_toprow = 0;
		wp->w_ntrows += target;
	} else {
		/* find the next window up */
		target = curwp->w_toprow - 1;
		while (wp != NULL) {
			if ((wp->w_toprow + wp->w_ntrows) == target)
				break;
			wp = wp->w_wndp;
		}
		if (wp == NULL)
			return FALSE;
		wp->w_ntrows += 1 + curwp->w_ntrows;
	}

	/* get rid of the current window */
	if (--curwp->w_bufp->b_nwnd == 0) {
		curwp->w_bufp->b_dotp = curwp->w_dotp;
		curwp->w_bufp->b_doto = curwp->w_doto;
		curwp->w_bufp->b_markp = curwp->w_markp;
		curwp->w_bufp->b_marko = curwp->w_marko;
	}
	if (lwp == NULL)
		wheadp = curwp->w_wndp;
	else
		lwp->w_wndp = curwp->w_wndp;
	free(curwp);
	curwp = wp;
	wp->w_flag |= WFHARD;
	curbp = wp->w_bufp;
	update_modelines();
	return TRUE;
}

/*
 * Split the current window.  A window smaller than 3 lines cannot be
 * split.  An argument of 1 forces the cursor into the upper window, an
 * argument of two forces the cursor to the lower window.  The only
 * other error that is possible is a "malloc" failure allocating the
 * structure for the new window.
 */
int splitwind(int f, int n)
{
	struct window *wp, *wp1, *wp2;
	struct line *lp;
	int ntru, ntrl, ntrd;

	if (curwp->w_ntrows < 3) {
		mlwrite("Cannot split a %d line window", curwp->w_ntrows);
		return FALSE;
	}
	if ((wp = malloc(sizeof(struct window))) == NULL) {
		mlwrite("Failed allocating memory for new window");
		return FALSE;
	}

	++curbp->b_nwnd;	/* Displayed twice. */
	wp->w_bufp = curbp;
	wp->w_dotp = curwp->w_dotp;
	wp->w_doto = curwp->w_doto;
	wp->w_markp = curwp->w_markp;
	wp->w_marko = curwp->w_marko;
	wp->w_flag = 0;
	wp->w_force = 0;
	ntru = (curwp->w_ntrows - 1) / 2;	/* Upper size */
	ntrl = (curwp->w_ntrows - 1) - ntru;	/* Lower size */
	lp = curwp->w_linep;
	ntrd = 0;
	while (lp != curwp->w_dotp) {
		++ntrd;
		lp = lforw(lp);
	}
	lp = curwp->w_linep;
	if (((f == FALSE) && (ntrd <= ntru)) || ((f == TRUE) && (n == 1))) {
		/* Old is upper window. */
		if (ntrd == ntru)	/* Hit mode line. */
			lp = lforw(lp);
		curwp->w_ntrows = ntru;
		wp->w_wndp = curwp->w_wndp;
		curwp->w_wndp = wp;
		wp->w_toprow = curwp->w_toprow + ntru + 1;
		wp->w_ntrows = ntrl;
	} else {		/* Old is lower window */
		wp1 = NULL;
		wp2 = wheadp;
		while (wp2 != curwp) {
			wp1 = wp2;
			wp2 = wp2->w_wndp;
		}
		if (wp1 == NULL)
			wheadp = wp;
		else
			wp1->w_wndp = wp;
		wp->w_wndp = curwp;
		wp->w_toprow = curwp->w_toprow;
		wp->w_ntrows = ntru;
		++ntru;		/* Mode line. */
		curwp->w_toprow += ntru;
		curwp->w_ntrows = ntrl;
		while (ntru--)
			lp = lforw(lp);
	}
	curwp->w_linep = lp;	/* Adjust the top lines */
	wp->w_linep = lp;	/* if necessary. */
	curwp->w_flag |= WFMODE | WFHARD;
	wp->w_flag |= WFMODE | WFHARD;
	return TRUE;
}

/*
 * Enlarge the current window.  Find the window that loses space.
 * Make sure it is big enough.
 */
int enlargewind(int f, int n)
{
	struct window *adjwp;
	struct line *lp;
	int i;

	if (n < 0)
		return shrinkwind(f, -n);
	if (wheadp->w_wndp == NULL) {
		mlwrite("Only one window");
		return FALSE;
	}
	if ((adjwp = curwp->w_wndp) == NULL) {
		mlwrite("The bottom window can not enlarge");
		return FALSE;
	}
	if (adjwp->w_ntrows <= n) {
		mlwrite("Impossible change");
		return FALSE;
	}

	lp = adjwp->w_linep;
	for (i = 0; i < n && lp != adjwp->w_bufp->b_linep; ++i)
		lp = lforw(lp);
	adjwp->w_linep = lp;
	adjwp->w_toprow += n;

	curwp->w_ntrows += n;
	adjwp->w_ntrows -= n;
	curwp->w_flag |= WFMODE | WFHARD | WFINS;
	adjwp->w_flag |= WFMODE | WFHARD | WFKILLS;
	return TRUE;
}

/* Shrink the current window.  Find the window that gains space */
int shrinkwind(int f, int n)
{
	struct window *adjwp;
	struct line *lp;
	int i;

	if (n < 0)
		return enlargewind(f, -n);
	if (wheadp->w_wndp == NULL) {
		mlwrite("Only one window");
		return FALSE;
	}
	if ((adjwp = curwp->w_wndp) == NULL) {
		mlwrite("The bottom window can not shrink");
		return FALSE;
	}
	if (curwp->w_ntrows <= n) {
		mlwrite("Impossible change");
		return FALSE;
	}

	lp = adjwp->w_linep;
	for (i = 0; i < n && lback(lp) != adjwp->w_bufp->b_linep; ++i)
		lp = lback(lp);
	adjwp->w_linep = lp;
	adjwp->w_toprow -= n;

	curwp->w_ntrows -= n;
	adjwp->w_ntrows += n;
	curwp->w_flag |= WFMODE | WFHARD | WFKILLS;
	adjwp->w_flag |= WFMODE | WFHARD | WFINS;
	return TRUE;
}

/*
 * Pick a window for a pop-up.  Split the screen if there is only one window.
 * Pick the uppermost window that isn't the current window.  An LRU algorithm
 * might be better.  Return a pointer, or NULL on error.
 */
struct window *wpopup(void)
{
	struct window *wp;

	if (wheadp->w_wndp == NULL &&	/* Only 1 window */
			splitwind(FALSE, 0) == FALSE) /* and it won't split */
		return NULL;

	/* Find window to use */
	for (wp = wheadp; wp != NULL && wp == curwp; wp = wp->w_wndp);

	return wp;
}

/* scroll the next window up (back) a page */
int scrnextup(int f, int n)
{
	nextwind(FALSE, 1);
	backpage(f, n);
	prevwind(FALSE, 1);
	return TRUE;
}

/* scroll the next window down (forward) a page */
int scrnextdw(int f, int n)
{
	nextwind(FALSE, 1);
	forwpage(f, n);
	prevwind(FALSE, 1);
	return TRUE;
}

int adjust_on_scr_resize(void)
{
	struct window *wp = NULL, *lastwp = NULL, *nextwp;
	int scr_rows = term.t_nrow, lastline;

	if (scr_rows < SCR_MIN_ROWS - 1)
		return FALSE;

	for (nextwp = wheadp; nextwp != NULL;) {
		wp = nextwp;
		/* Have to update nextwp here, since wp could change it. */
		nextwp = nextwp->w_wndp;

		/* get rid of it if it is too low */
		if (wp->w_toprow > scr_rows - 1) {
			/* save the point/mark if needed */
			if (--wp->w_bufp->b_nwnd == 0) {
				wp->w_bufp->b_dotp = wp->w_dotp;
				wp->w_bufp->b_doto = wp->w_doto;
				wp->w_bufp->b_markp = wp->w_markp;
				wp->w_bufp->b_marko = wp->w_marko;
			}

			/* update curwp and lastwp if needed */
			if (wp == curwp)
				curwp = wheadp;
			curbp = curwp->w_bufp;
			if (lastwp != NULL)
				lastwp->w_wndp = NULL;

			free(wp);
			wp = NULL;
		} else {
			/* need to change this window size? */
			lastline = wp->w_toprow + wp->w_ntrows - 1;
			if (lastline >= scr_rows - 1)
				wp->w_ntrows = scr_rows - wp->w_toprow - 1;
		}

		/* Every window should be redrawn during resizing */
		if (wp != NULL)
			wp->w_flag |= WFHARD | WFMODE;

		lastwp = wp;
	}

	/* free spaces are given to the bottom window */
	if (wp != NULL) {
		lastline = wp->w_toprow + wp->w_ntrows - 1;
		if (lastline < scr_rows - 1) {
			wp->w_ntrows = scr_rows - wp->w_toprow - 1;
			wp->w_flag |= WFHARD | WFMODE;
		}
	}

	sgarbf = TRUE;
	return TRUE;
}

/* get screen offset of current line in current window */
int getwpos(void)
{
	struct line *lp;
	int sline;

	lp = curwp->w_linep;
	sline = 1;
	while (lp != curwp->w_dotp) {
		++sline;
		lp = lforw(lp);
	}

	return sline;
}
