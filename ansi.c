#include "me.h"

static inline void
ascr_init(void)	/* Not ANSI, but widely supported */
{
	ttputs("\033[?1049h");
}

static inline void
ascr_end(void)	/* Not ANSI, but widely supported */
{
	ttputs("\033[?1049l");
}

void
ansiopen(void)
{
	int cols, rows;

	getscreensize(&cols, &rows);
	term_nrow = atleast(rows - 1, SCR_MIN_ROWS - 1);
	term_ncol = atleast(cols, SCR_MIN_COLS);

	ttopen();
	ascr_init();
	ttflush();
	sgarbf = TRUE;
}

void
ansiclose(void)
{
	/* Move cursor to bottom in case that ascr is not supported. */
	ansimove(term_nrow, 0);
	ascr_end();
	ttflush();
	ttclose();
}

void
ansimove(int row, int col)
{
	char buf[32];
	snprintf(buf, 32, "\033[%d;%dH", row + 1, col + 1);
	ttputs(buf);
}

void
ansirev(int is_rev)
{
	ttputs(is_rev ? "\033[7m" : "\033[0m");
}

void
ansibeep(void)
{
	ttputc(BELL);
	ttflush();
}

void
ansieeol(void)
{
	ttputs("\033[K");
}

void
ansieeop(void)
{
	ttputs("\033[J");
}
