#include "me.h"

static int get_universal_arg(int *arg);
static int command_loop(void);
static int window_init(void);
static int execute(int c, int f, int n);
static void cleanup(void);
static void emergencyexit(int);

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

	bind_exithandler(emergencyexit);

	for (i = 1; i < argc; ++i) {
		if ((bp = bfind(argv[i], TRUE)) == NULL)
			die(2, cleanup, "Failed opening file %s\n", argv[i]);
		if (firstbp == NULL)
			firstbp = bp;
	}

	if (window_init())
		die(3, cleanup, "Failed initializing window\n");

	swbuffer(firstbp);
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

static int window_init(void)
{
	struct window *wp;
	if ((wp = malloc(sizeof(struct window))) == NULL) /* First window */
		return 1;

	memset(wp, 0, sizeof(*wp));
	wp->w_ntrows = term_nrow - 1;	/* "-1" for mode line. */
	wheadp = wp;
	curwp = wp;
	return 0;
}

/* This function looks a key binding up in the binding table. */
static commandfn_t getbind(int c)
{
	struct keybind *p;
	for (p = bindings; p->fn != NULL; ++p) {
		if (p->code == c)
			return p->fn;
	}
	return NULL;
}

/*
Executes a command, updates flags and returns status of the command execution.
*/
static int execute(int c, int f, int n)
{
	commandfn_t execfunc;
	int status;
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
		ansibeep();
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

static void emergencyexit(int unused)
{
	save_buffers();
	quit(TRUE, 0);
}

/*
If there is an argument, always quit.  Otherwise confirm if a buffer has been
changed and not written out.
*/
int quit(int f, int n)
{
	if (f != FALSE || anycb() == FALSE ||
			mlyesno("Modified buffers exist.  Quit") == TRUE) {
		cleanup();
		exit(f ? n : 0);
	}
	/* Do not quit (user say no) */
	mlerase();
	return TRUE;
}

static void cleanup(void)
{
	struct window *wp, *tp;
	struct buffer *bp;
	while ((bp = bheadp)) {
		/* clear b_flag to make `zotbuf` run without prompt */
		bp->b_flag = 0;
		bp->b_nwnd = 0;
		zotbuf(bp);
	}
	for (wp = wheadp; wp; wp = tp) {
		tp = wp->w_wndp;
		free(wp);
	}
	kdelete();
	vtdeinit();
	wheadp = NULL;
	bheadp = NULL;
	kheadp = NULL;
}
