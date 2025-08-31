#include "me.h"

static FILE *ffp;		/* File pointer, all functions. */
static int eofflag;		/* end-of-file flag */

/* Increase the buffer (fline) by NSTRING bytes. (realloc and copy) */
static int fline_extend(void)
{
	char *tmpline;
	if ((tmpline = malloc(flen + NSTRING)) == NULL)
		return FIOMEM;
	if (flen > 0)
		memcpy(tmpline, fline, flen);

	if (fline != NULL)
		free(fline);

	fline = tmpline;
	flen += NSTRING;
	return 0;
}

/* The full reset contains 2 parts: Free and memory; Reset fline and flen */
static inline void fline_reset(void)
{
	free(fline);
	fline = NULL;
	flen = 0;
}

int ffropen(char *fn)
{
	if ((ffp = fopen(fn, "rb")) == NULL) {
		mlwrite("Cannot open file for reading");
		return FIOFNF;
	}
	eofflag = FALSE;
	return FIOSUC;
}

int ffwopen(char *fn)
{
	if ((ffp = fopen(fn, "wb")) == NULL) {
		mlwrite("Cannot open file for writing");
		return FIOERR;
	}
	return FIOSUC;
}

int ffclose(void)
{
	/* free the buffer (fline) since we do not need it anymore */
	if (fline)
		fline_reset();

	eofflag = FALSE;

	if (fclose(ffp) != 0) {
		mlwrite("Error closing file");
		return FIOERR;
	}

	return FIOSUC;
}

int ffputline(char *buf, int nbuf)
{
	int i;
	for (i = 0; i < nbuf; ++i)
		fputc(buf[i] & 0xFF, ffp);

	fputc('\n', ffp);

	if (ferror(ffp)) {
		mlwrite("Write I/O error");
		return FIOERR;
	}

	return FIOSUC;
}

int ffgetline(int *count)
{
	int c, i, s;

	if (eofflag)
		return FIOEOF;
	if (fline == NULL) {
		if ((s = fline_extend()))
			return s;
	}

	i = 0;
	while ((c = fgetc(ffp)) != EOF && c != '\n') {
		fline[i++] = c;
		if (i == flen) {
			if ((s = fline_extend()))
				return s;
		}
	}

	if (c == EOF) {
		if (ferror(ffp)) {
			mlwrite("File read error");
			return FIOERR;
		}
		if (i == 0)
			return FIOEOF;
		eofflag = TRUE;
	}

	*count = i;
	return FIOSUC;
}
