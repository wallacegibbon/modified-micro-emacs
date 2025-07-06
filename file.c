#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include <unistd.h>
#include <stdio.h>

/* Max number of lines from one file. */
#define MAXNLINE 10000000

int fileread(int f, int n)
{
	char fname[NFILEN];
	int s;

	if ((s = mlreply("Read file: ", fname, NFILEN)) != TRUE)
		return s;

	return readin(fname, TRUE);
}

/* Insert a file into the current buffer. */
int insfile(int f, int n)
{
	char fname[NFILEN];
	int s;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if ((s = mlreply("Insert file: ", fname, NFILEN)) != TRUE)
		return s;

	return ifile(fname);
}

/*
 * Select a file for editing.
 * Look around to see if you can find the fine in another buffer;
 * if you can find it just switch to the buffer.
 * If you cannot find the file, create a new buffer, read in the text,
 * and switch to the new buffer.
 */
int filefind(int f, int n)
{
	char fname[NFILEN];
	int s;

	if ((s = mlreply("Find file: ", fname, NFILEN)) != TRUE)
		return s;
	return getfile(fname, TRUE);
}

int viewfile(int f, int n)
{
	struct window *wp;
	char fname[NFILEN];
	int s;

	if ((s = mlreply("View file: ", fname, NFILEN)) != TRUE)
		return s;

	s = getfile(fname, FALSE);
	if (s) {
		curwp->w_bufp->b_mode |= MDVIEW;
		for_each_wind(wp)
			wp->w_flag |= WFMODE;
	}
	return s;
}

int getfile(char *fname, int lockfl)
{
	struct buffer *bp;
	struct line *lp;
	char bname[NBUFN];
	int i, s;

	for_each_buff(bp) {
		if ((bp->b_flag & BFINVS) == 0 &&
				strcmp(bp->b_fname, fname) == 0) {
			swbuffer(bp);
			lp = curwp->w_dotp;
			i = curwp->w_ntrows / 2;
			while (i-- && lback(lp) != curbp->b_linep)
				lp = lback(lp);
			curwp->w_linep = lp;
			curwp->w_flag |= WFMODE | WFHARD;
			mlwrite("(Old buffer)");
			return TRUE;
		}
	}
	makename(bname, fname);	/* New buffer name. */
	while ((bp = bfind(bname, FALSE, 0)) != NULL) {
		/* old buffer name conflict code */
		s = mlreply("Buffer name: ", bname, NBUFN);
		if (s == ABORT)	/* ^G to just quit */
			return s;
		if (s == FALSE) {	/* CR to clobber it */
			makename(bname, fname);
			break;
		}
	}
	if (bp == NULL && (bp = bfind(bname, TRUE, 0)) == NULL) {
		mlwrite("Cannot create buffer");
		return FALSE;
	}
	if (--curbp->b_nwnd == 0) {	/* Undisplay. */
		curbp->b_dotp = curwp->w_dotp;
		curbp->b_doto = curwp->w_doto;
		curbp->b_markp = curwp->w_markp;
		curbp->b_marko = curwp->w_marko;
	}

	prevbp = curbp;
	curbp = bp;

	curwp->w_bufp = bp;
	curbp->b_nwnd++;

	s = readin(fname, lockfl);
	return s;
}

/*
 * Read file "fname" into the current buffer, blowing away any text
 * found there.  Called by both the read and find commands.  Return
 * the final status of the read.  Also called by the mainline, to
 * read in a file specified on the command line as an argument.
 */
int readin(char *fname, int lockfl /* check for file locks ? */)
{
	struct line *lp1, *lp2;
	struct window *wp;
	struct buffer *bp;
	int s, i, nbytes, nline;
	char mesg[NSTRING];

#if (FILOCK && BSD) || SVR4
	if (lockfl && lockchk(fname) == ABORT) {
		s = FIOFNF;
		bp = curbp;
		strcpy(bp->b_fname, "");
		goto out;
	}
#endif
	bp = curbp;
	if ((s = bclear(bp)) != TRUE)	/* Might be old. */
		return s;
	bp->b_flag &= ~(BFINVS | BFCHG);
	strncpy(bp->b_fname, fname, NFILEN - 1);

	if ((s = ffropen(fname)) == FIOERR)	/* Hard file open. */
		goto out;

	if (s == FIOFNF) {	/* File not found. */
		mlwrite("(New file)");
		goto out;
	}

	/* read the file in */
	mlwrite("(Reading file)");
	nline = 0;
	while ((s = ffgetline(&nbytes)) == FIOSUC) {
		if ((lp1 = lalloc(nbytes)) == NULL) {
			s = FIOMEM;	/* Keep message on the */
			break;	/* display. */
		}
		if (nline > MAXNLINE) {
			s = FIOMEM;
			break;
		}
		lp2 = lback(curbp->b_linep);
		lp2->l_fp = lp1;
		lp1->l_fp = curbp->b_linep;
		lp1->l_bp = lp2;
		curbp->b_linep->l_bp = lp1;
		for (i = 0; i < nbytes; ++i)
			lputc(lp1, i, fline[i]);
		++nline;
	}
	ffclose();		/* Ignore errors. */
	strcpy(mesg, "(");
	if (s == FIOERR) {
		strcat(mesg, "I/O ERROR, ");
		curbp->b_flag |= BFTRUNC;
	}
	if (s == FIOMEM) {
		strcat(mesg, "OUT OF MEMORY, ");
		curbp->b_flag |= BFTRUNC;
	}
	sprintf(&mesg[strlen(mesg)], "Read %d line", nline);
	if (nline != 1)
		strcat(mesg, "s");
	strcat(mesg, ")");
	mlwrite(mesg);

out:
	for_each_wind(wp) {
		if (wp->w_bufp == curbp) {
			wp->w_linep = lforw(curbp->b_linep);
			wp->w_dotp = lforw(curbp->b_linep);
			wp->w_doto = 0;
			wp->w_markp = NULL;
			wp->w_marko = 0;
			wp->w_flag |= WFMODE | WFHARD;
		}
	}
	if (s == FIOERR || s == FIOFNF)	/* False if error. */
		return FALSE;
	return TRUE;
}

/*
 * Take a file name, and from it fabricate a buffer name.
 * This routine knows about the syntax of file names on the target system.
 * I suppose that this information could be put in a better place
 * than a line of code.
 */
void makename(char *bname, char *fname)
{
	char *cp1, *cp2;

	cp1 = fname;
	while (*cp1 != 0)
		++cp1;

#if UNIX
	while (cp1 != fname && cp1[-1] != '/')
		--cp1;
#endif
	cp2 = bname;
	while (cp2 != &bname[NBUFN - 1] && *cp1 != 0 && *cp1 != ';')
		*cp2++ = *cp1++;
	*cp2 = 0;
}

/* make sure a buffer name is unique */
void unqname(char *name)
{
	char *sp;

	/* check to see if it is in the buffer list */
	while (bfind(name, 0, FALSE) != NULL) {
		/* go to the end of the name */
		sp = name;
		while (*sp)
			++sp;
		if (sp == name || (*(sp - 1) < '0' || *(sp - 1) > '8')) {
			*sp++ = '0';
			*sp = 0;
		} else {
			*(--sp) += 1;
		}
	}
}

/*
 * Ask for a file name, and write the contents of the current buffer to that
 * file.  Update the remembered file name and clear the buffer changed flag.
 * This handling of file names is different from the earlier versions,
 * and is more compatable with Gosling EMACS than with ITS EMACS.
 */
int filewrite(int f, int n)
{
	struct window *wp;
	char fname[NFILEN];
	int s;

	if ((s = mlreply("Write file: ", fname, NFILEN)) != TRUE)
		return s;
	if ((s = writeout(fname)) == TRUE) {
		strcpy(curbp->b_fname, fname);
		curbp->b_flag &= ~BFCHG;
		for_each_wind(wp) {
			if (wp->w_bufp == curbp)
				wp->w_flag |= WFMODE;
		}
	}
	return s;
}

/*
 * Save the contents of the current buffer in its associatd file.
 * No nothing if nothing has changed (this may be a bug, not a feature).
 * Error if there is no remembered file name for the buffer.
 */
int filesave(int f, int n)
{
	struct window *wp;
	int s;

	if (curbp->b_mode & MDVIEW)
		return rdonly();
	if ((curbp->b_flag & BFCHG) == 0)	/* Return, no changes. */
		return TRUE;
	if (curbp->b_fname[0] == 0) {	/* Must have a name. */
		mlwrite("No file name");
		return FALSE;
	}

	/* complain about truncated files */
	if ((curbp->b_flag & BFTRUNC) != 0) {
		if (mlyesno("Truncated file ... write it out") == FALSE) {
			mlwrite("(Aborted)");
			return FALSE;
		}
	}

	if ((s = writeout(curbp->b_fname)) == TRUE) {
		curbp->b_flag &= ~BFCHG;
		for_each_wind(wp) {
			if (wp->w_bufp == curbp)
				wp->w_flag |= WFMODE;
		}
	}
	return s;
}

/*
 * This function performs the details of file writing.
 * Uses the file management routines in the "fileio.c" package.
 * The number of lines written is displayed.
 */
int writeout(char *fn)
{
	struct line *lp;
	int nline, s;

	if ((s = ffwopen(fn)) != FIOSUC) {	/* Open writes message. */
		return FALSE;
	}
	mlwrite("(Writing...)");
	lp = lforw(curbp->b_linep);	/* First line. */
	nline = 0;
	while (lp != curbp->b_linep) {
		if ((s = ffputline(&lp->l_text[0], llength(lp))) != FIOSUC)
			break;
		++nline;
		lp = lforw(lp);
	}
	if (s == FIOSUC) {	/* No write error. */
		s = ffclose();
		if (s == FIOSUC) {	/* No close error. */
			if (nline == 1)
				mlwrite("(Wrote 1 line)");
			else
				mlwrite("(Wrote %d lines)", nline);
		}
	} else {
		ffclose();
	}

	if (s != FIOSUC)	/* Some sort of error. */
		return FALSE;

	return TRUE;
}

/*
 * Insert file "fname" into the current buffer, Called by insert file command.
 * Return the final status of the read.
 */
int ifile(char *fname)
{
	struct line *lp0, *lp1, *lp2;
	struct buffer *bp;
	char mesg[NSTRING];
	int nbytes, nline, i, s;

	bp = curbp;
	bp->b_flag |= BFCHG;
	bp->b_flag &= ~BFINVS;
	if ((s = ffropen(fname)) == FIOERR)	/* Hard file open. */
		goto out;
	if (s == FIOFNF) {	/* File not found. */
		mlwrite("(No such file)");
		return FALSE;
	}
	mlwrite("(Inserting file)");

	/* back up a line and save the mark here */
	curwp->w_dotp = lback(curwp->w_dotp);
	curwp->w_doto = 0;
	curwp->w_markp = curwp->w_dotp;
	curwp->w_marko = 0;

	nline = 0;
	while ((s = ffgetline(&nbytes)) == FIOSUC) {
		if ((lp1 = lalloc(nbytes)) == NULL) {
			s = FIOMEM;
			break;
		}
		lp0 = curwp->w_dotp;
		lp2 = lp0->l_fp;
		lp2->l_bp = lp1;
		lp0->l_fp = lp1;
		lp1->l_bp = lp0;
		lp1->l_fp = lp2;
		curwp->w_dotp = lp1;
		for (i = 0; i < nbytes; ++i)
			lputc(lp1, i, fline[i]);
		++nline;
	}
	ffclose();		/* Ignore errors. */
	curwp->w_markp = lforw(curwp->w_markp);
	strcpy(mesg, "(");
	if (s == FIOERR) {
		strcat(mesg, "I/O ERROR, ");
		curbp->b_flag |= BFTRUNC;
	}
	if (s == FIOMEM) {
		strcat(mesg, "OUT OF MEMORY, ");
		curbp->b_flag |= BFTRUNC;
	}
	sprintf(&mesg[strlen(mesg)], "Inserted %d line", nline);
	if (nline > 1)
		strcat(mesg, "s");
	strcat(mesg, ")");
	mlwrite(mesg);

out:
	/* advance to the next line and mark the window for changes */
	curwp->w_dotp = lforw(curwp->w_dotp);
	curwp->w_flag |= WFHARD | WFMODE;

	/* copy window parameters back to the buffer structure */
	curbp->b_dotp = curwp->w_dotp;
	curbp->b_doto = curwp->w_doto;
	curbp->b_markp = curwp->w_markp;
	curbp->b_marko = curwp->w_marko;

	if (s == FIOERR)
		return FALSE;

	return TRUE;
}
