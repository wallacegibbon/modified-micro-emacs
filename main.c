#include "efunc.h"
#include "edef.h"
#include "line.h"
#include <stdio.h>
#include <stdlib.h>

#if UNIX
#include <signal.h>
static void emergencyexit(int);
#ifdef SIGWINCH
void sizesignal(int);
#endif
#endif

void usage(const char *program_name, int status)
{
	printf("USAGE: %s [OPTIONS] [FILENAMES]\n\n", program_name);
	fputs("\t+<n>\tGo to line <n>\n", stdout);
	fputs("\t-v\tOpen read only\n", stdout);
	fputs("\t--help\tDisplay this help and exit\n", stdout);
	fputs("\n", stdout);
	exit(status);
}

int main(int argc, char **argv)
{
	struct buffer *firstbp = NULL, *bp;
	char bname[NBUFN];
	int firstfile = TRUE, rdonlyflag = FALSE, gotoflag = FALSE, gline = 0;
	int c = 0, i;
	int f, n;

#if UNIX
#ifdef SIGWINCH
	signal(SIGWINCH, sizesignal);
#endif
#endif
	if (argc == 2) {
		if (strcmp(argv[1], "--help") == 0)
			usage(argv[0], EXIT_SUCCESS);
	}

	vtinit();
	edinit("main");

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

loop:
	update(FALSE);
	c = getcmd();

	/* if there is something on the command line, clear it */
	if (mpresf != FALSE)
		mlerase();

	f = FALSE;
	n = 1;

	/* do ^U repeat argument processing */
	if (c == REPTC) {
		i = 0;	/* A sign for the first loop */
		f = TRUE;
		n = 4;
		mlwrite("Arg: 4");
		c = getcmd();
		while (isdigit(c) || c == REPTC) {
			if (c == REPTC) {
				n = n * 4;
			} else {
				if (i == 0)
					n = c - '0';
				else
					n = 10 * n + c - '0';
			}
			i = 1;
			mlwrite("Arg: %d", n);
			c = getcmd();
		}
	}

	execute(c, f, n);
	goto loop;
}

/*
 * Initialize all of the buffers and windows.  The buffer name is passed down
 * as an argument, because the main routine may have been told to read in a
 * file by default, and we want the buffer name to be right.
 */
void edinit(char *bname)
{
	struct buffer *bp;
	struct window *wp;

	bp = bfind(bname, TRUE, 0); /* First buffer */
	wp = malloc(sizeof(struct window)); /* First window */
	if (bp == NULL || wp == NULL)
		exit(1);
	curbp = bp;
	wheadp = wp;
	curwp = wp;
	wp->w_wndp = NULL;	/* Initialize window */
	wp->w_bufp = bp;
	bp->b_nwnd = 1;		/* Displayed. */
	wp->w_linep = bp->b_linep;
	wp->w_dotp = bp->b_linep;
	wp->w_doto = 0;
	wp->w_markp = NULL;
	wp->w_marko = 0;
	wp->w_toprow = 0;
	wp->w_ntrows = term.t_nrow - 1;	/* "-1" for mode line. */
	wp->w_force = 0;
	wp->w_flag = WFMODE | WFHARD;	/* Full. */
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

/*
 * Fancy quit command, as implemented by Norm.  If the any buffer has
 * changed do a write on that buffer and exit emacs, otherwise simply exit.
 */
int quickexit(int f, int n)
{
	struct buffer *curbp_bak = curbp, *bp;
	int status;

	for_each_buff(bp) {
		if ((bp->b_flag & BFCHG) && !(bp->b_flag & BFTRUNC)) {
			curbp = bp;
			mlwrite("(Saving %s)", bp->b_fname);
			if ((status = filesave(f, n)) != TRUE) {
				curbp = curbp_bak;
				return status;
			}
		}
	}
	quit(f, n);
	return TRUE;
}

static void emergencyexit(int signr)
{
	quickexit(FALSE, 0);
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
		if (f)
			exit(n);
		else
			exit(0);
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

	/* First clean up the windows */
	wp = wheadp;
	while (wp) {
		tp = wp->w_wndp;
		free(wp);
		wp = tp;
	}
	wheadp = NULL;

	/* Then the buffers */
	bp = bheadp;
	while (bp) {
		bp->b_nwnd = 0;
		/* don't say anything about a changed buffer! */
		bp->b_flag = 0;
		zotbuf(bp);
		bp = bheadp;
	}

	/* and the kill buffer */
	kdelete();
	/* and the video buffers */
	vtdeinit();
#undef exit
	exit(status);
}
#endif
