#include "me.h"

#define SCRSIZ	64		/* scroll size for extended lines */
#define MARGIN	8		/* size of minimim margin and */

static void ansiopen(void);
static void ansiclose(void);
static void ansimove(int, int);
static void ansieeol(void);
static void ansieeop(void);
static void ansibeep(void);
static void ansirev(int);

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
	ansirev
};

static inline void putpad(char *str)
{
	int c;
	while ((c = *str++))
		ttputc(c);
}

static inline void ascr_init(void) /* Not ANSI, but widely supported */
{
	putpad("\033[?1049h");
}

static inline void ascr_end(void) /* Not ANSI, but widely supported */
{
	putpad("\033[?1049l");
}

static inline int ansi_compatible(const char *name)
{
	return !strncmp(name, "vt100", 5) ||
		!strncmp(name, "xterm", 5) ||
		!strncmp(name, "linux", 5);
}

static void ansiopen(void)
{
	int cols, rows;
	char *cp;

	if ((cp = getenv("TERM")) == NULL) {
		fputs("Shell variable TERM not defined!", stderr);
		return;
	}
	if (!ansi_compatible(cp)) {
		fputs("Terminal type not ANSI-compatible!", stderr);
		return;
	}

	getscreensize(&cols, &rows);
	term.t_nrow = atleast(rows - 1, SCR_MIN_ROWS - 1);
	term.t_ncol = atleast(cols, SCR_MIN_COLS);

	ttopen();
	ascr_init();
	ttflush();
	sgarbf = TRUE;
}

static void ansiclose(void)
{
	/* Move cursor to bottom in case that ascr is not supported. */
	ansimove(term.t_nrow, 0);
	ascr_end();
	ttflush();
	ttclose();
}

static void ansimove(int row, int col)
{
	char buf[32];
	snprintf(buf, 32, "\033[%d;%dH", row + 1, col + 1);
	putpad(buf);
}

static void ansirev(int is_rev)
{
	putpad(is_rev ? "\033[7m" : "\033[0m");
}

static void ansibeep(void)
{
	ttputc(BELL);
	ttflush();
}

static void ansieeol(void)
{
	putpad("\033[K");
}

static void ansieeop(void)
{
	putpad("\033[J");
}
