#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include <stdio.h>

#if USE_ANSI

#define SCRSIZ	64		/* scroll size for extended lines */
#define MARGIN	8		/* size of minimim margin and */

static void ansiopen(void);
static void ansimove(int, int);
static void ansieeol(void);
static void ansieeop(void);
static void ansibeep(void);
static void ansirev(int);
static int ansicres(char *);

static void ansiparm(int n);

struct terminal term = {
	0, 0,			/* These 2 values are set at open time. */
	MARGIN,
	SCRSIZ,
	ansiopen,
	ttclose,
	ttgetc,
	ttputc,
	ttflush,
	ansimove,
	ansieeol,
	ansieeop,
	ansibeep,
	ansirev,
	ansicres
};

static void ansiopen(void)
{
#if UNIX
	char *cp;
	int cols, rows;

	if ((cp = getenv("TERM")) == NULL) {
		puts("Shell variable TERM not defined!");
		exit(1);
	}
	if ((strncmp(cp, "vt100", 5) != 0) &&
			(strncmp(cp, "xterm", 5) != 0) &&
			(strncmp(cp, "linux", 5) != 0)) {
		puts("Terminal type not 'vt100'!");
		exit(1);
	}

	getscreensize(&cols, &rows);
	term.t_nrow = atleast(rows - 1, SCR_MIN_ROWS - 1);
	term.t_ncol = atleast(cols, SCR_MIN_COLS);
#endif
	revexist = TRUE;
	ttopen();
}

static void ansimove(int row, int col)
{
	ttputc(ESC);
	ttputc('[');
	ansiparm(row + 1);
	ttputc(';');
	ansiparm(col + 1);
	ttputc('H');
}

static void ansieeol(void)
{
	ttputc(ESC);
	ttputc('[');
	ttputc('K');
}

static void ansieeop(void)
{
	ttputc(ESC);
	ttputc('[');
	ttputc('J');
}

static void ansirev(int is_rev)
{
	ttputc(ESC);
	ttputc('[');
	ttputc(is_rev ? '7' : '0');
	ttputc('m');
}

static int ansicres(char *res)
{
	return TRUE;
}

static void ansibeep(void)
{
	ttputc(BELL);
	ttflush();
}

static void ansiparm(int n)
{
	int q, r;

	q = n / 10;
	if (q != 0) {
		r = q / 10;
		if (r != 0) {
			ttputc((r % 10) + '0');
		}
		ttputc((q % 10) + '0');
	}
	ttputc((n % 10) + '0');
}

#endif
