#include "me.h"

#if UNIX
#include <signal.h>
static void emergencyexit(int);
#endif

static int get_universal_arg(int *arg);
static int command_loop(void);
static int edinit(struct buffer *bp);
static int execute(int c, int f, int n);

int main(int argc, char **argv)
{
	struct buffer *firstbp = NULL, *bp;
	int i;

	if (argc < 2)
		die(1, NULL, "Please give me at least one file\n");

	/* Initialize the virtual terminal, this have to be successful */
	vtinit();
	if (!display_ok)
		die(1, vtdeinit, "Failed initializing virtual terminal\n");

	for (i = 1; i < argc; ++i) {
		if ((bp = bfind(argv[i], TRUE)) == NULL)
			die(2, vtdeinit, "Failed opening file %s\n", argv[i]);
		if (firstbp == NULL)
			firstbp = bp;
	}

	if (edinit(firstbp))
		die(3, vtdeinit, "Failed initializing editor\n");

	swbuffer(firstbp);

#if UNIX
	signal(SIGHUP, emergencyexit);
	signal(SIGTERM, emergencyexit);
#endif
	for (;;)
		command_loop();
}

static int command_loop(void)
{
	int f = FALSE, n = 1, c;

	/* The `update` should be called in each command loop. */
	update(FALSE);

	/* Loop pauses on `getcmd` to get the next keyboard input. */
	c = getcmd();

	/* Call mlerase after getcmd, so messages exist until next input */
	if (mpresf != FALSE)
		mlerase();

	if (c == REPTC) {
		c = get_universal_arg(&n);
		f = TRUE;
	}

	return execute(c, f, n);
}

static int get_universal_arg(int *arg)
{
	int n = 4, first_flag = 1, c;
	for (;;) {
		mlwrite("Argument: %d", n);
		c = getcmd();
		if (isdigit(c)) {
			n = first_flag ? (c - '0') : (10 * n + c - '0');
			first_flag = 0;
		} else if (c == REPTC) {
			n *= 4;
			first_flag = 0;
		} else {
			*arg = n;
			mlerase();
			return c;
		}
	}
}

static int edinit(struct buffer *bp)
{
	struct window *wp;
	if ((wp = malloc(sizeof(struct window))) == NULL) /* First window */
		return 1;

	wheadp = wp;
	curwp = wp;
	curbp = bp;
	wp->w_wndp = NULL;
	wp->w_bufp = bp;
	wp->w_linep = bp->b_linep;
	wp->w_dotp = bp->b_linep;
	wp->w_doto = 0;
	wp->w_markp = NULL;
	wp->w_marko = 0;
	wp->w_toprow = 0;
	wp->w_ntrows = term.t_nrow - 1;	/* "-1" for mode line. */
	wp->w_force = 0;
	wp->w_flag = WFMODE | WFHARD;
	bp->b_nwnd = 1;
	return 0;
}

/* This function looks a key binding up in the binding table. */
static int (*getbind(int c))(int, int)
{
	struct key_tab *ktp = keytab;

	while (ktp->fn != NULL) {
		if (ktp->k_code == c)
			return ktp->fn;
		++ktp;
	}
	return NULL;
}

/*
 * This is the general command execution routine.  It handles the fake binding
 * of all the keys to "self-insert".  It also clears out the "thisflag" word,
 * and arranges to move it to the "lastflag", so that the next command can
 * look at it.  Return the status of command.
 */
static int execute(int c, int f, int n)
{
	int status;
	int (*execfunc)(int, int);

	/* If the keystroke is a bound function.  Do it. */
	execfunc = getbind(c);
	if (execfunc != NULL) {
		thisflag = 0;
		status = execfunc(f, n);
		lastflag = thisflag;
		return status;
	}

	/* No binding found, self inserting. */

	/* If C-I is not bound, turn it into untagged value */
	if (c == (CTL | 'I'))
		c = '\t';

	if (c > 0xFF) {
		TTbeep();
		mlwrite("Key not bound");
		lastflag = 0;	/* Fake last flags. */
		return FALSE;
	}

	/* For self-insert keys, do not insert when n <= 0 */
	if (n <= 0) {
		lastflag = 0;
		return n == 0 ? TRUE : FALSE;
	}

	thisflag = 0;
	status = linsert(n, c);
	lastflag = thisflag;
	return status;
}

static void save_buffers(void)
{
	struct buffer *bp;
	for_each_buff(bp) {
		if ((bp->b_flag & BFCHG) && !(bp->b_flag & BFTRUNC)) {
			mlwrite("Saving %s", bp->b_fname);
			filesave(FALSE, 0);
		}
	}
}

int quickexit(int f, int n)
{
	save_buffers();
	return quit(f, n);
}

static void emergencyexit(int signr)
{
	save_buffers();
	quit(TRUE, 0);
}

/*
 * Quit command.  If an argument, always quit.  Otherwise confirm if a buffer
 * has been changed and not written out.
 */
int quit(int f, int n)
{
	if (f != FALSE || anycb() == FALSE ||
			mlyesno("Modified buffers exist.  Quit") == TRUE) {
		vttidy();
		exit(f ? n : 0);
	}
	mlerase();
	return TRUE;
}

/*
 * On some primitave operation systems, and when emacs is used as a subprogram
 * to a larger project, emacs needs to de-alloc its own used memory.
 */
#if CLEAN
int cexit(int status)
{
	struct window *wp, *tp;
	struct buffer *bp;

	wp = wheadp;
	while (wp) {
		tp = wp->w_wndp;
		free(wp);
		wp = tp;
	}
	wheadp = NULL;

	bp = bheadp;
	while (bp) {
		bp->b_nwnd = 0;
		/* clear b_flag to make `zotbuf` run without prompt */
		bp->b_flag = 0;
		zotbuf(bp);
		bp = bheadp;
	}
	kdelete();
	vtdeinit();
#undef exit
	exit(status);
}
#endif
