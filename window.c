#include "me.h"

/*
 * With no argument, it just does the refresh.  With an argument "n",
 * reposition dot to line "n" of the screen.  (n == 0 means centering it)
 */
int redraw(int f, int n)
{
	if (f == FALSE) {
		sgarbf = TRUE;
	} else {
		curwp->w_force = n;
		curwp->w_flag |= WFFORCE;
	}
	return TRUE;
}

static int count_window(void)
{
	struct window *wp = wheadp;
	int n = 1;
	while ((wp = wp->w_wndp) != NULL)
		++n;
	return n;
}

/* Make the nth next window (or -nth prev window) the current window. */
int nextwind(int f, int n)
{
	struct window *wp = curwp;
	int wcount;

	if (wheadp->w_wndp == NULL)
		return FALSE;

	wcount = count_window();
	n %= wcount;
	if (n < 0)
		n += wcount;

	while (n--) {
		if ((wp = wp->w_wndp) == NULL)
			wp = wheadp;
	}
	if (wp == curwp) {
		mlwrite("Already in this window");
		return TRUE;
	}
	curwp = wp;
	curbp = wp->w_bufp;
	update_modelines();
	return TRUE;
}

int prevwind(int f, int n)
{
	return nextwind(f, -n);
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
		if (--wp->w_bufp->b_nwnd == 0)
			wstate_save(wp);
		free(wp);
	}
	while (curwp->w_wndp != NULL) {
		wp = curwp->w_wndp;
		curwp->w_wndp = wp->w_wndp;
		if (--wp->w_bufp->b_nwnd == 0)
			wstate_save(wp);
		free(wp);
	}
	lp = curwp->w_linep;
	i = curwp->w_toprow;
	while (i != 0 && lp->l_bp != curbp->b_linep) {
		--i;
		lp = lp->l_bp;
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
	struct window *wp, *lwp;
	int target;

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
	if (--curwp->w_bufp->b_nwnd == 0)
		wstate_save(curwp);
	if (lwp == NULL)
		wheadp = curwp->w_wndp;
	else
		lwp->w_wndp = curwp->w_wndp;

	free(curwp);
	wp->w_flag |= WFHARD;
	curwp = wp;
	curbp = wp->w_bufp;
	update_modelines();
	return TRUE;
}

/*
 * Split the current window.  A window smaller than 3 lines cannot be splited.
 * An argument of 1 forces the cursor into the upper window, an argument of 2
 * forces the cursor to the lower window.  The only other error that is
 * possible is a "malloc" failure allocating the structure for the new window.
 */
int splitwind(int f, int n)
{
	struct window *wp;
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
	wstate_copy(wp, curwp);
	wp->w_flag = 0;
	wp->w_force = 0;
	ntru = (curwp->w_ntrows - 1) / 2;	/* Upper size */
	ntrl = (curwp->w_ntrows - 1) - ntru;	/* Lower size */
	lp = curwp->w_linep;
	ntrd = 0;
	while (lp != curwp->w_dotp) {
		++ntrd;
		lp = lp->l_fp;
	}
	lp = curwp->w_linep;

	if (ntrd == ntru)	/* Hit mode line. */
		lp = lp->l_fp;

	curwp->w_ntrows = ntru;
	wp->w_wndp = curwp->w_wndp;
	curwp->w_wndp = wp;
	wp->w_toprow = curwp->w_toprow + ntru + 1;
	wp->w_ntrows = ntrl;

	curwp->w_linep = lp;
	wp->w_linep = lp;
	curwp->w_flag |= WFMODE | WFHARD;
	wp->w_flag |= WFMODE | WFHARD;
	return TRUE;
}
