#include "me.h"

/* Creates a buffer and read file, or switches to a existing buffer. */
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

/* Reads file into the current buffer, blows away any existing text. */
int readin(char *filename)
{
	struct window *wp;
	struct line *lp;
	int nline = 0, nbytes, s;
	char mesg[NSTRING];

	if (bclear(curwp->w_bufp) != TRUE)	/* Might be old. */
		return FALSE;

	curwp->w_bufp->b_flag &= ~BFCHG;
	strncpy_safe(curwp->w_bufp->b_fname, filename, NFILEN);

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
		line_insert(curwp->w_bufp->b_linep, lp);
		memcpy(lp->l_text, fline, nbytes);
		++nline;
	}
	ffclose();	/* Ignore errors. */
	mesg[0] = '\0';
	if (s == FIOERR) {
		strcat(mesg, "I/O ERROR, ");
		curwp->w_bufp->b_flag |= BFTRUNC;
	}
	if (s == FIOMEM) {
		strcat(mesg, "OUT OF MEMORY, ");
		curwp->w_bufp->b_flag |= BFTRUNC;
	}
	sprintf(&mesg[strlen(mesg)], "Read %d line", nline);
	if (nline != 1)
		strcat(mesg, "s");
	mlwrite("%s", mesg);

out:
	for_each_wind(wp) {
		if (wp->w_bufp == curwp->w_bufp)
			resetwind(wp);
	}
	if (s == FIOERR || s == FIOFNF)
		return FALSE;
	else
		return TRUE;
}

/*
Askes for a file name, and writes the contents of the current buffer to that
file.  Updates the remembered file name and clears the buffer changed flag.
*/
int filewrite(int f, int n)
{
	struct window *wp;
	char filename[NFILEN];
	int s;

	if ((s = mlreply("Write file: ", filename, NFILEN)) != TRUE)
		return s;
	if ((s = writeout(filename)) == TRUE) {
		strcpy(curwp->w_bufp->b_fname, filename);
		curwp->w_bufp->b_flag &= ~BFCHG;
		for_each_wind(wp) {
			if (wp->w_bufp == curwp->w_bufp)
				wp->w_flag |= WFMODE;
		}
	}
	return s;
}

int filesave(int f, int n)
{
	struct window *wp;
	int s;

	if (!(curwp->w_bufp->b_flag & BFCHG))
		return TRUE;
	if (curwp->w_bufp->b_fname[0] == 0) {
		mlwrite("No file name");
		return FALSE;
	}

	if (curwp->w_bufp->b_flag & BFTRUNC) {
		if (mlyesno("Truncated file ... write it out") == FALSE) {
			mlwrite("Aborted");
			return FALSE;
		}
	}

	if ((s = writeout(curwp->w_bufp->b_fname)) == TRUE) {
		curwp->w_bufp->b_flag &= ~BFCHG;
		for_each_wind(wp) {
			if (wp->w_bufp == curwp->w_bufp)
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
	lp = curwp->w_bufp->b_linep->l_fp;
	nline = 0;
	while (lp != curwp->w_bufp->b_linep) {
		if ((s = ffputline(&lp->l_text[0], lp->l_used)) != FIOSUC)
			break;
		lp = lp->l_fp;
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
