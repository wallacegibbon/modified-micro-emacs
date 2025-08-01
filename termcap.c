#include "efunc.h"
#include "edef.h"
#include <term.h>
#include <signal.h>
#include <stdio.h>

#if USE_TERMCAP

#define SCRSIZ	64
#define MARGIN	8

static void tcapopen(void);
static void tcapclose(void);
static void tcapmove(int, int);
static void tcapeeol(void);
static void tcapeeop(void);
static void tcapbeep(void);
static void tcaprev(int);

/* On Ubuntu 24.04 x86-64 GNOME Terminal, tcapbuf takes 76 bytes */
#define TCAPSLEN 128
static char tcapbuf[TCAPSLEN];

static char *CM, *CE, *CL, *SO, *SE, *TI, *TE;

struct terminal term = {
	0, 0,			/* These 2 values are set at open time. */
	MARGIN,
	SCRSIZ,
	tcapopen,
	tcapclose,
	ttgetc,
	ttputc,
	ttflush,
	tcapmove,
	tcapeeol,
	tcapeeop,
	tcapbeep,
	tcaprev
};

static inline void putpad(char *str)
{
	tputs(str, 1, ttputc);
}

static void init_termcap(void)
{
	char *p = tcapbuf;

	CM = tgetstr("cm", &p);	/* Cursor move */
	CL = tgetstr("cl", &p);	/* Clear screen */
	CE = tgetstr("ce", &p);	/* Clear to end of line */
	SO = tgetstr("so", &p);	/* Begin standout mode */
	SE = tgetstr("se", &p);	/* End standout mode */
	TI = tgetstr("ti", &p);	/* Enter alternate screen */
	TE = tgetstr("te", &p);	/* Exit alternate screen */

	if (p >= &tcapbuf[TCAPSLEN]) {
		fputs("Terminal description too big!\n", stderr);
		exit(1);
	}
	if (CM == NULL || CL == NULL) {
		fputs("Incomplete termcap entry\n", stderr);
		exit(1);
	}
}

static void tcapopen(void)
{
	char tcbuf[1024];
	char *cp;
	int cols, rows;

	if ((cp = getenv("TERM")) == NULL) {
		fputs("Environment variable TERM not defined!", stderr);
		exit(1);
	}
	if ((tgetent(tcbuf, cp)) != 1) {
		fprintf(stderr, "Unknown terminal type %s!", cp);
		exit(1);
	}

	getscreensize(&cols, &rows);
	term.t_nrow = atleast(rows - 1, SCR_MIN_ROWS - 1);
	term.t_ncol = atleast(cols, SCR_MIN_COLS);

	init_termcap();

	ttopen();
	putpad(TI);
	ttflush();

	sgarbf = TRUE;
}

static void tcapclose(void)
{
	tcapmove(term.t_nrow, 0);
	putpad(TE);
	ttflush();
	ttclose();
}

static void tcapmove(int row, int col)
{
	putpad(tgoto(CM, col, row));
}

static void tcapeeol(void)
{
	putpad(CE);
}

static void tcapeeop(void)
{
	putpad(CL);
}

static void tcaprev(int is_rev)
{
	putpad(is_rev ? SO : SE);
}

static void tcapbeep(void)
{
	ttputc(BELL);
}

#endif
