#include "me.h"

/* Max number of lines from one file. */
#define MAXNLINE 10000000

/*
 * If the buffer can be found, just switch to the buffer.  Other wise create
 * a new buffer, read in the text, and switch to the new buffer.
 */
int filefind(int f, int n)
{
	struct buffer *bp;
	char filename[NFILEN];
	int s;

	if ((s = mlreply("Find file: ", filename, NFILEN)) != TRUE)
		return s;

	for_each_buff(bp) {
		if (strcmp(bp->b_fname, filename) == 0) {
			mlwrite("Old buffer");
			return swbuffer(bp);
		}
	}

	if ((bp = bfind(filename, TRUE)) == NULL) {
		mlwrite("Cannot create buffer");
		return FALSE;
	}
	if (--curbp->b_nwnd == 0)
		wstate_save(curwp);

	return swbuffer(bp);
}

/* Read file into the current buffer, blowing away any existing text. */
int readin(char *filename)
{
	struct line *lp1, *lp2;
	struct window *wp;
	int s, i, nbytes, nline;
	char mesg[NSTRING];

	if (bclear(curbp) != TRUE)	/* Might be old. */
		return FALSE;

	curbp->b_flag &= ~BFCHG;
	strncpy_safe(curbp->b_fname, filename, NFILEN);

	if ((s = ffropen(filename)) == FIOERR)
		return FALSE;

	if (s == FIOFNF) {
		mlwrite("New file");
		s = FIOSUC;
		goto out;
	}

	mlwrite("Reading file");
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
	mesg[0] = '\0';
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
 * Ask for a file name, and write the contents of the current buffer to that
 * file.  Update the remembered file name and clear the buffer changed flag.
 * This handling of file names is different from the earlier versions,
 * and is more compatable with Gosling EMACS than with ITS EMACS.
 */
int filewrite(int f, int n)
{
	struct window *wp;
	char filename[NFILEN];
	int s;

	if ((s = mlreply("Write file: ", filename, NFILEN)) != TRUE)
		return s;
	if ((s = writeout(filename)) == TRUE) {
		strcpy(curbp->b_fname, filename);
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
			mlwrite("Aborted");
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

	mlwrite("Writing...");
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
				mlwrite("Wrote 1 line");
			else
				mlwrite("Wrote %d lines", nline);
		}
	} else {
		ffclose();
	}

	if (s != FIOSUC)
		return FALSE;
	else
		return TRUE;
}
