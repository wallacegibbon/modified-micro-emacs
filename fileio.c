#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include <stdio.h>

static FILE *ffp;			/* File pointer, all functions. */
static int eofflag;			/* end-of-file flag */

int ffropen(char *fn)
{
	if ((ffp = fopen(fn, "r")) == NULL) {
		mlwrite("Cannot open file for reading");
		return FIOFNF;
	}
	eofflag = FALSE;
	return FIOSUC;
}

int ffwopen(char *fn)
{
	if ((ffp = fopen(fn, "w")) == NULL) {
		mlwrite("Cannot open file for writing");
		return FIOERR;
	}
	return FIOSUC;
}

int ffclose(void)
{
	/* free this since we do not need it anymore */
	if (fline) {
		free(fline);
		fline = NULL;
	}
	eofflag = FALSE;

#if UNIX
	if (fclose(ffp) != 0) {
		mlwrite("Error closing file");
		return FIOERR;
	}
	return FIOSUC;
#else
	fclose(ffp);
	return FIOSUC;
#endif
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
	char *tmpline;
	int c, i;

	if (eofflag)
		return FIOEOF;

	if (flen > NSTRING) {
		/* dump fline in case it ended up too big */
		free(fline);
		fline = NULL;
	}
	if (fline == NULL) {
		if ((fline = malloc(flen = NSTRING)) == NULL)
			return FIOMEM;
	}

	i = 0;
	while ((c = fgetc(ffp)) != EOF && c != '\n') {
		fline[i++] = c;
		if (i >= flen) {
			if ((tmpline = malloc(flen + NSTRING)) == NULL)
				return FIOMEM;
			strncpy_safe(tmpline, fline, flen);
			flen += NSTRING;
			free(fline);
			fline = tmpline;
		}
	}

	if (c == EOF) {
		if (ferror(ffp)) {
			mlwrite("File read error");
			return FIOERR;
		}
		if (i != 0)
			eofflag = TRUE;
		else
			return FIOEOF;
	}

	*count = i;
	return FIOSUC;
}
