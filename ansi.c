#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include <stdio.h>

#if !USE_TERMCAP

#define SCRSIZ	64		/* scroll size for extended lines */
#define MARGIN	8		/* size of minimim margin and */

static void ansiopen(void);
static void ansiclose(void);
static void ansimove(int, int);
static void ansieeol(void);
static void ansieeop(void);
static void ansibeep(void);
static void ansirev(int);
static int ansicres(char *);

static void ansiparm(int n);

/* Not ANSI, but supported by many terminals. */
static void alternate_screen_init(void);
static void alternate_screen_end(void);

struct terminal term = {
	0, 0,			/* These 2 values are set at open time. */
	MARGIN,
	SCRSIZ,
	ansiopen,
	ansiclose,
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

static inline void ttputs(char *s)
{
	int c;
	while ((c = *s++))
		ttputc(c);
}

static inline int ansi_compatible(const char *name)
{
	return !strncmp(name, "vt100", 5) ||
		!strncmp(name, "xterm", 5) ||
		!strncmp(name, "linux", 5);
}

static void ansiopen(void)
{
	char *cp;
	int cols, rows;

	if ((cp = getenv("TERM")) == NULL) {
		puts("Shell variable TERM not defined!");
		exit(1);
	}
	if (!ansi_compatible(cp)) {
		puts("Terminal type not ANSI-compatible!");
		exit(1);
	}

	getscreensize(&cols, &rows);
	term.t_nrow = atleast(rows - 1, SCR_MIN_ROWS - 1);
	term.t_ncol = atleast(cols, SCR_MIN_COLS);

	ttopen();
	alternate_screen_init();
	ttflush();

	sgarbf = TRUE;
}

static void ansiclose(void)
{
	ansimove(term.t_nrow, 0);
	alternate_screen_end();
	ttflush();
	ttclose();
}

static void ansimove(int row, int col)
{
	ttputs("\033[");
	ansiparm(row + 1);
	ttputc(';');
	ansiparm(col + 1);
	ttputc('H');
}

/* The following 2 are not in ANSI, but useful in modern terminals */
static void alternate_screen_init(void)
{
	ttputs("\033[?1049h");
}

static void alternate_screen_end(void)
{
	ttputs("\033[?1049l");
}

static void ansieeol(void)
{
	ttputs("\033[K");
}

static void ansieeop(void)
{
	ttputs("\033[J");
}

static void ansirev(int is_rev)
{
	ttputs("\033[");
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
		if (r != 0)
			ttputc((r % 10) + '0');
		ttputc((q % 10) + '0');
	}
	ttputc((n % 10) + '0');
}

#endif
