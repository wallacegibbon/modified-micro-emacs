#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"
#include <unistd.h>
#include <stdio.h>

/* Max number of lines from one file. */
#define MAXNLINE 10000000

/*
 * If the buffer can be found, just switch to the buffer.  Other wise create
 * a new buffer, read in the text, and switch to the new buffer.
 */
int filefind(int f, int n)
{
	struct buffer *bp;
	struct line *lp;
	char bname[NBUFN], fname[NFILEN];
	int i, s;

	if ((s = mlreply("Find file: ", fname, NFILEN)) != TRUE)
		return s;

	for_each_buff(bp) {
		if (!(bp->b_flag & BFINVS) &&
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
	makename(bname, fname);
	while ((bp = bfind(bname, FALSE, 0)) != NULL) {
		/* old buffer name conflict code */
		s = mlreply("Buffer name: ", bname, NBUFN);
		if (s == ABORT)
			return s;
		if (s == FALSE) {
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

	s = readin(fname, TRUE);
	return s;
}

/*
 * Read file "fname" into the current buffer, blowing away any existing text.
 * Called by both the read and find commands.  Return the status of the read.
 * Also called by the mainline, to read in a file specified on the command line
 * as an argument.
 */
int readin(char *fname, int lockfl)
{
	struct line *lp1, *lp2;
	struct window *wp;
	int s, i, nbytes, nline;
	char mesg[NSTRING];

#if (FILOCK && BSD) || SVR4
	if (lockfl && lockchk(fname) == ABORT) {
		strcpy(curbp->b_fname, "");
		s = FIOFNF;
		goto out;
	}
#endif
	if (bclear(curbp) != TRUE)	/* Might be old. */
		return FALSE;

	curbp->b_flag &= ~(BFINVS | BFCHG);
	strncpy_safe(curbp->b_fname, fname, NFILEN);

	if ((s = ffropen(fname)) == FIOERR)
		return FALSE;

	if (s == FIOFNF) {
		mlwrite("(New file)");
		s = FIOSUC;
		goto out;
	}

	mlwrite("(Reading file)");
	nline = 0;
	while ((s = ffgetline(&nbytes)) == FIOSUC) {
		if ((lp1 = lalloc(nbytes)) == NULL) {
			s = FIOMEM;	/* Keep message on the display */
			break;
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
	mlwrite("%s", mesg);

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
	if (s == FIOERR || s == FIOFNF)
		return FALSE;
	else
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
	while (bfind(name, FALSE, 0) != NULL) {
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

int filesave(int f, int n)
{
	struct window *wp;
	int s;

	if (curbp->b_flag & BFRDONLY)
		return rdonly();
	if (!(curbp->b_flag & BFCHG))
		return TRUE;
	if (curbp->b_fname[0] == 0) {
		mlwrite("No file name");
		return FALSE;
	}

	if (curbp->b_flag & BFTRUNC) {
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

int writeout(char *fn)
{
	struct line *lp;
	int nline, s;

	if ((s = ffwopen(fn)) != FIOSUC)
		return FALSE;

	mlwrite("(Writing...)");
	nline = 0;
	for (lp = lforw(curbp->b_linep); lp != curbp->b_linep; lp = lforw(lp)) {
		if ((s = ffputline(&lp->l_text[0], llength(lp))) != FIOSUC)
			break;
		++nline;
	}
	if (s == FIOSUC) {
		s = ffclose();
		if (s == FIOSUC) {
			if (nline == 1)
				mlwrite("(Wrote 1 line)");
			else
				mlwrite("(Wrote %d lines)", nline);
		}
	} else {
		ffclose();
	}

	if (s != FIOSUC)
		return FALSE;
	else
		return TRUE;
}
