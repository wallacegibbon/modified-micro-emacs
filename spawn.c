#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include <stdio.h>
#include <unistd.h>

/*
 * Create a subjob with a copy of the command intrepreter in it.  When the
 * command interpreter exits, mark the screen as garbage so that you do a full
 * repaint.
 */
int spawncli(int f, int n)
{
	int r;
#if UNIX
	char *cp;
	movecursor(term.t_nrow, 0);
	TTflush();
	TTclose();
	if ((cp = getenv("SHELL")) != NULL && *cp != '\0')
		r = system(cp);
	else
#if BSD
		r = system("exec /bin/csh");
#else
		r = system("exec /bin/sh");
#endif
	sgarbf = TRUE;

	TTopen();
	update(TRUE);

	if (r == 0)
		return TRUE;

	mlwrite("Failed running external command");
	return FALSE;
#endif
}

/* Pipe a one line command into a window */
int pipecmd(int f, int n)
{
	struct window *wp;
	struct buffer *bp;
	char line[NLINE];
	int s, r;

	static char bname[] = "_me_cmd_tmp";
	static char filename[NSTRING] = "_me_cmd_tmp";

	/* get the command to pipe in */
	if ((s = mlreply("@", line, NLINE)) != TRUE)
		return s;

	/* get rid of the command output buffer if it exists */
	if ((bp = bfind(bname, FALSE, 0)) != FALSE) {
		for_each_wind(wp) {
			if (wp->w_bufp == bp) {
				if (wp == curwp)
					delwind(FALSE, 1);
				else
					onlywind(FALSE, 1);
				break;
			}
		}
		if (zotbuf(bp) != TRUE)
			return FALSE;
	}
#if UNIX
	TTflush();
	TTclose();
	strcat(line, ">");
	strcat(line, filename);
	r = system(line);
	TTopen();
	TTflush();
	sgarbf = TRUE;
	s = TRUE;
#endif

	if (s != TRUE)
		return s;

	/* split the current window to make room for the command output */
	if (splitwind(FALSE, 1) == FALSE)
		return FALSE;

	/* and read the stuff in */
	if (getfile(filename, FALSE) == FALSE)
		return FALSE;

	curwp->w_bufp->b_mode |= MDVIEW;
	for_each_wind(wp)
		wp->w_flag |= WFMODE;

	unlink(filename);

	if (r == 0)
		return TRUE;

	mlwrite("Failed running external command");
	return FALSE;
}

/* filter a buffer through an external program */
int filter_buffer(int f, int n)
{
	struct buffer *bp;
	char line[NLINE];
	char tmpnam[NFILEN];
	int s, r;

	static char bname[] = "_me_filter_tmp";
	static char filename_in[] = "_me_filter_tmp_in";
	static char filename_out[] = "_me_filter_tmp_out";

	if (curbp->b_mode & MDVIEW)
		return rdonly();

	/* get the filter name and its args */
	if ((s = mlreply("#", line, NLINE)) != TRUE)
		return s;

	/* setup the proper file names */
	bp = curbp;
	strcpy(tmpnam, bp->b_fname);	/* save the original name */
	strcpy(bp->b_fname, bname);	/* set it to our new one */

	/* write it out, checking for errors */
	if (writeout(filename_in) != TRUE) {
		mlwrite("(Cannot write filter file)");
		strcpy(bp->b_fname, tmpnam);
		return FALSE;
	}

#if UNIX
	TTputc('\n');			/* Already have '\r' */
	TTflush();
	TTclose();
	strcat(line, "<");
	strcat(line, filename_in);
	strcat(line, ">");
	strcat(line, filename_out);
	r = system(line);
	TTopen();
	TTflush();
	sgarbf = TRUE;
	s = TRUE;
#endif

	/* on failure, escape gracefully */
	if (s != TRUE || (readin(filename_out, FALSE) == FALSE)) {
		mlwrite("(Execution failed)");
		strcpy(bp->b_fname, tmpnam);
		unlink(filename_in);
		unlink(filename_out);
		return s;
	}

	strcpy(bp->b_fname, tmpnam);
	bp->b_flag |= BFCHG;

	unlink(filename_in);
	unlink(filename_out);

	if (r == 0)
		return TRUE;

	mlwrite("Failed running external command");
	return FALSE;
}
