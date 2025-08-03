#include "efunc.h"
#include "edef.h"
#include "line.h"
#include <stdio.h>
#include <stdlib.h>

#if UNIX
#include <signal.h>
static void emergencyexit(int);
#ifdef SIGWINCH
static void resizeexit(int);
#endif
#endif

static const char *quitmsg = NULL;

void usage(const char *program_name, int status)
{
	printf("USAGE: %s [OPTIONS] [FILENAMES]\n\n", program_name);
	fputs("\t+<n>\tGo to line <n>\n", stdout);
	fputs("\t-v\tOpen read only\n", stdout);
	fputs("\t--help\tDisplay this help and exit\n", stdout);
	fputs("\n", stdout);
	exit(status);
}

static int get_universal_arg(int *arg);

int main(int argc, char **argv)
{
	struct buffer *firstbp = NULL, *bp;
	char bname[NBUFN];
	int firstfile = TRUE, rdonlyflag = FALSE, gotoflag = FALSE, gline = 0;
	int c = 0, i, f, n;

#if UNIX
#ifdef SIGWINCH
	signal(SIGWINCH, resizeexit);
#endif
#endif
	if (argc == 2) {
		if (strcmp(argv[1], "--help") == 0)
			usage(argv[0], EXIT_SUCCESS);
	}

	vtinit();
	if (!display_ok)
		die(1, "Failed initializing virtual terminal\n");

	if (edinit("main"))
		die(1, "Failed initializing editor\n");

	for (i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && (argv[i][1] | DIFCASE) == 'v') {
			rdonlyflag = TRUE;
		} else if (argv[i][0] == '+') {
			gotoflag = TRUE;
			gline = atoi(&argv[i][1]);
		} else {
			makename(bname, argv[i]);
			unqname(bname);
			bp = bfind(bname, TRUE, rdonlyflag ? BFRDONLY : 0);
			strncpy_safe(bp->b_fname, argv[i], NFILEN);
			bp->b_flag &= ~BFACTIVE;
			if (firstfile) {
				firstbp = bp;
				firstfile = FALSE;
			}
		}
	}

#if UNIX
	signal(SIGHUP, emergencyexit);
	signal(SIGTERM, emergencyexit);
#endif

	/* If there are any files to read, read the first one! */
	if (firstfile == FALSE) {
		swbuffer(firstbp);
		if ((bp = bfind("main", FALSE, 0)) != NULL)
			zotbuf(bp);
	}

	/* Deal with startup gotos */
	if (gotoflag) {
		if (gotoline(TRUE, gline) == FALSE) {
			update(FALSE);
			mlwrite("(Bogus goto argument)");
		}
	}

	for (;;) {
		update(FALSE);
		c = getcmd();
		if (mpresf != FALSE)
			mlerase();

		f = FALSE;
		n = 1;
		if (c == REPTC) {
			c = get_universal_arg(&n);
			f = TRUE;
		}

		execute(c, f, n);
	}
}

static int get_universal_arg(int *arg)
{
	int n = 4, first_flag = 0, c;
	for (;;) {
		mlwrite("Arg: %d", n);
		c = getcmd();
		if (isdigit(c)) {
			n = !first_flag ? (c - '0') : (10 * n + c - '0');
			first_flag = 1;
		} else if (c == REPTC) {
			n *= 4;
			first_flag = 1;
		} else {
			*arg = n;
			return c;
		}
	}
}

/*
 * Initialize all of the buffers and windows.  The buffer name is passed down
 * as an argument, because the main routine may have been told to read in a
 * file by default, and we want the buffer name to be right.
 */
int edinit(char *bname)
{
	struct buffer *bp;
	struct window *wp;

	bp = bfind(bname, TRUE, 0); /* First buffer */
	wp = malloc(sizeof(struct window)); /* First window */
	if (bp == NULL || wp == NULL)
		return 1;

	curbp = bp;
	wheadp = wp;
	curwp = wp;
	wp->w_wndp = NULL;
	wp->w_bufp = bp;
	bp->b_nwnd = 1;
	wp->w_linep = bp->b_linep;
	wp->w_dotp = bp->b_linep;
	wp->w_doto = 0;
	wp->w_markp = NULL;
	wp->w_marko = 0;
	wp->w_toprow = 0;
	wp->w_ntrows = term.t_nrow - 1;	/* "-1" for mode line. */
	wp->w_force = 0;
	wp->w_flag = WFMODE | WFHARD;
	return 0;
}

/* This function looks a key binding up in the binding table. */
int (*getbind(int c))(int, int)
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
int execute(int c, int f, int n)
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
		mlwrite("(Key not bound)");
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
			mlwrite("(Saving %s)", bp->b_fname);
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

static void resizeexit(int signr)
{
	save_buffers();
	quitmsg = "me is exited since terminal is resized.";
	quit(TRUE, 0);
}

/*
 * Quit command.  If an argument, always quit.  Otherwise confirm if a buffer
 * has been changed and not written out.
 */
int quit(int f, int n)
{
	int s;

	if (f != FALSE || anycb() == FALSE /* All buffers clean. */
			|| (s = mlyesno("Modified buffers exist.  Quit"))
				== TRUE) {
#if UNIX
		if (lockrel() != TRUE) {
			TTputc('\n');
			TTputc('\r');
			TTclose();
			exit(1);
		}
#endif
		vttidy();
		if (quitmsg != NULL)
			fprintf(stderr, "%s\n", quitmsg);
		exit(f ? n : 0);
	}
	mlerase();
	return s;
}

/* Begin a keyboard macro. */
int ctlxlp(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite("%%Macro already active");
		return FALSE;
	}
	mlwrite("(Start macro)");
	kbdptr = kbdm;
	kbdend = kbdptr;
	kbdmode = RECORD;
	return TRUE;
}

/* End keyboard macro. */
int ctlxrp(int f, int n)
{
	if (kbdmode == STOP) {
		mlwrite("%%Macro not active");
		return FALSE;
	}
	if (kbdmode == RECORD) {
		mlwrite("(End macro)");
		kbdmode = STOP;
	}
	return TRUE;
}

/* Execute a macro. */
int ctlxe(int f, int n)
{
	if (kbdmode != STOP) {
		mlwrite("%%Macro already active");
		return FALSE;
	}
	if (n <= 0)
		return TRUE;
	kbdrep = n;
	kbdmode = PLAY;
	kbdptr = kbdm;
	return TRUE;
}

/*
 * Abort.  Kill off any keyboard macro, etc., that is in progress.
 * Sometimes called as a routine, to do general aborting of stuff.
 */
int ctrlg(int f, int n)
{
	TTbeep();
	kbdmode = STOP;
	mlwrite("(Aborted)");
	return ABORT;
}

/* Tell the user that this command is illegal in read-only mode. */
int rdonly(void)
{
	TTbeep();
	mlwrite("(Key illegal in read-only mode)");
	return FALSE;
}

int nullproc(int f, int n)
{
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
		bp->b_flag = 0;	/* ignore changed buffers */
		zotbuf(bp);
		bp = bheadp;
	}
	kdelete();
	vtdeinit();
#undef exit
	exit(status);
}
#endif
