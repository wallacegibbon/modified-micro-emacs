#include "me.h"

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

	if ((bp = bfind(filename, FALSE)) != NULL) {
		mlwrite("Old buffer");
	} else if ((bp = bfind(filename, TRUE)) == NULL) {
		mlwrite("Cannot create buffer");
		return FALSE;
	}
	return swbuffer(bp);
}

/* Read file into the current buffer, blowing away any existing text. */
int readin(char *filename)
{
	struct window *wp;
	struct line *lp;
	int nline = 0, nbytes, s;
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
	while ((s = ffgetline(&nbytes)) == FIOSUC) {
		if ((lp = lalloc(nbytes)) == NULL) {
			s = FIOMEM;
			break;
		}
		line_insert(curbp->b_linep, lp);
		memcpy(lp->l_text, fline, nbytes);
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
		if (wp->w_bufp == curbp)
			resetwind(wp);
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
	for (lp = curbp->b_linep->l_fp; lp != curbp->b_linep; lp = lp->l_fp) {
		if ((s = ffputline(&lp->l_text[0], lp->l_used)) != FIOSUC)
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
