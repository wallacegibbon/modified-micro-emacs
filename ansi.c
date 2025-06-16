/*
 * The routines in this file provide support for ANSI style terminals over a
 * serial line.  The serial I/O services are provided by routines in
 * "termio.c".  It compiles into nothing if not an ANSI device.
 */

#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include <stdio.h>

#if ANSI

#define NROW    25		/* Screen size. */
#define NCOL    80		/* Edit if you want to. */

#define NPAUSE	100		/* # times thru update to pause */
#define MARGIN	8		/* size of minimim margin and */
#define SCRSIZ	64		/* scroll size for extended lines */

static void ansiopen(void);
static void ansikopen(void);
static void ansikclose(void);
static void ansimove(int, int);
static void ansieeol(void);
static void ansieeop(void);
static void ansibeep(void);
static void ansirev(int);
static int ansicres(char *);
static void ansiparm(int n);

/*
 * Standard terminal interface dispatch table.  Most of the fields point into
 * "termio" code.
 */
struct terminal term = {
	NROW - 1,
	NCOL,
	MARGIN,
	SCRSIZ,
	NPAUSE,
	ansiopen,
	ttclose,
	ansikopen,
	ansikclose,
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

/* Change reverse video state.
 * state: TRUE = reverse, FALSE = normal
 */
static void ansirev(int state)
{
	ttputc(ESC);
	ttputc('[');
	ttputc(state ? '7' : '0');
	ttputc('m');
}

/* Change screen resolution. */
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

static void ansiopen(void)
{
#if UNIX
	char *cp;

	if ((cp = getenv("TERM")) == NULL) {
		puts("Shell variable TERM not defined!");
		exit(1);
	}
	if (strcmp(cp, "vt100") != 0) {
		puts("Terminal type not 'vt100'!");
		exit(1);
	}
#endif
	revexist = TRUE;
	ttopen();
}

/* Open the keyboard (a noop here). */
static void ansikopen(void)
{
}

/* Close the keyboard (a noop here). */
static void ansikclose(void)
{
}

#endif
