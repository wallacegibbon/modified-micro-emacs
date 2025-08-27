#include "me.h"

/* Split the current window.  Window smaller than 3 lines cannot be splited. */
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

	++curwp->w_bufp->b_nwnd;
	wp->w_bufp = curwp->w_bufp;
	wstate_copy(wp, curwp);
	wp->w_flag = 0;
	wp->w_force = 0;

	ntru = (curwp->w_ntrows - 1) / 2;	/* Upper */
	ntrl = (curwp->w_ntrows - 1) - ntru;	/* Lower */

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

/* Makes the current window the only window on the screen. */
int onlywind(int f, int n)
{
	struct window *wp;
	struct line *lp1, *lp2;
	int i;

	/* Delete all other windows except the current one */
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

	/* Adjust w_linep to keep the cursor unmoving when it is possible. */
	lp1 = curwp->w_linep;
	lp2 = curwp->w_bufp->b_linep;
	i = curwp->w_toprow;
	while ( i-- && lp1->l_bp != lp2)
		lp1 = lp1->l_bp;

	curwp->w_linep = lp1;
	curwp->w_toprow = 0;
	curwp->w_ntrows = term.t_nrow - 1;
	curwp->w_flag |= WFMODE | WFHARD;
	return TRUE;
}

/*
With no argument, it just does the refresh.
With an argument "n", reposition dot to line "n" of the screen.
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
	update_modelines();
	return TRUE;
}

int prevwind(int f, int n)
{
	return nextwind(f, -n);
}

void resetwind(struct window *wp)
{
	wp->w_linep = wp->w_bufp->b_linep->l_fp;
	wp->w_dotp = wp->w_linep;
	wp->w_doto = 0;
	wp->w_markp = NULL;
	wp->w_marko = 0;
	wp->w_flag |= WFMODE | WFHARD;
}
