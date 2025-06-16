#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include <term.h>
#include <signal.h>
#include <stdio.h>

#if TCAP

#ifdef SIGWINCH
#include <sys/ioctl.h>
#endif

#define NPAUSE	10    /* # times thru update to pause. */
#define MARGIN	8
#define SCRSIZ	64

static void tcapkopen(void);
static void tcapkclose(void);
static void tcapmove(int, int);
static void tcapeeol(void);
static void tcapeeop(void);
static void tcapbeep(void);
static void tcaprev(int);
static int tcapcres(char *);
static void putpad(char *str);

static void tcapopen(void);
static void tcapclose(void);

#define TCAPSLEN 315
static char tcapbuf[TCAPSLEN];
static char *UP, PC, *CM, *CE, *CL, *SO, *SE, *TI, *TE;

static void getscreensize(int *widthp, int *heightp);

struct terminal term = {
	0, 0,		/* These 2 values are set at open time. */
	MARGIN,
	SCRSIZ,
	NPAUSE,
	tcapopen,
	tcapclose,
	tcapkopen,
	tcapkclose,
	ttgetc,
	ttputc,
	ttflush,
	tcapmove,
	tcapeeol,
	tcapeeop,
	tcapbeep,
	tcaprev,
	tcapcres,
};

static void tcapopen(void)
{
	char tcbuf[1024];
	char *tv_stype, *t, *p;
	int cols, rows;

	if ((tv_stype = getenv("TERM")) == NULL) {
		fputs("Environment variable TERM not defined!", stderr);
		exit(1);
	}

	if ((tgetent(tcbuf, tv_stype)) != 1) {
		fprintf(stderr, "Unknown terminal type %s!", tv_stype);
		exit(1);
	}

	/* Get screen size from system, or else from termcap. */
	getscreensize(&cols, &rows);
	term.t_nrow = atleast(rows - 1, SCR_MIN_ROWS - 1);
	term.t_ncol = atleast(cols, SCR_MIN_COLS);

	p = tcapbuf;
	t = tgetstr("pc", &p);
	PC = t ? *t : 0;
	CL = tgetstr("cl", &p);
	CM = tgetstr("cm", &p);
	CE = tgetstr("ce", &p);
	UP = tgetstr("up", &p);
	SE = tgetstr("se", &p);
	SO = tgetstr("so", &p);
	if (SO != NULL)
		revexist = TRUE;
	if (tgetnum("sg") > 0) {	/* can reverse be used? */
		revexist = FALSE;
		SE = NULL;
		SO = NULL;
	}
	TI = tgetstr("ti", &p);	/* terminal init and exit */
	TE = tgetstr("te", &p);

	if (CL == NULL || CM == NULL || UP == NULL) {
		fputs("Incomplete termcap entry\n", stderr);
		exit(1);
	}

	if (CE == NULL)	/* will we be able to use clear to EOL? */
		eolexist = FALSE;

	if (p >= &tcapbuf[TCAPSLEN]) {
		fputs("Terminal description too big!\n", stderr);
		exit(1);
	}
	ttopen();
}

static void getscreensize(int *widthp, int *heightp)
{
#ifdef TIOCGWINSZ
	struct winsize size;
	*widthp = 0;
	*heightp = 0;
	if (ioctl(0, TIOCGWINSZ, &size) < 0)
		return;
	*widthp = size.ws_col;
	*heightp = size.ws_row;
#else
	*widthp = 0;
	*heightp = 0;
#endif
}

static void tcapclose(void)
{
	putpad(tgoto(CM, 0, term.t_nrow));
	putpad(TE);
	ttflush();
	ttclose();
}

static void tcapkopen(void)
{
	putpad(TI);
	ttflush();
	ttrow = 999;
	ttcol = 999;
	sgarbf = TRUE;
}

static void tcapkclose(void)
{
	putpad(TE);
	ttflush();
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

/*
 * Change reverse video status
 *
 * @state: FALSE = normal video, TRUE = reverse video.
 */
static void tcaprev(int state)
{
	if (state) {
		if (SO != NULL)
			putpad(SO);
	} else if (SE != NULL) {
		putpad(SE);
	}
}

/* Change screen resolution. */
static int tcapcres(char *res)
{
	return TRUE;
}

static void tcapbeep(void)
{
	ttputc(BELL);
}

static void putpad(char *str)
{
	tputs(str, 1, ttputc);
}

#endif
