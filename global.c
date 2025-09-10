#include "estruct.h"
#include <stddef.h>

char hex[] = "0123456789ABCDEF";

int kbdm[NKBDM];		/* Keyboard Macro */
int *kbdptr;			/* Current position in keyboard buf */
int *kbdend = kbdm;		/* Ptr to end of the keyboard */
int kbdmode = STOP;		/* Current keyboard macro mode */
int kbdrep;			/* Number of repetitions */

int sgarbf = TRUE;		/* TRUE if screen is garbage */
int mpresf = FALSE;		/* TRUE if message in last line */

short term_nrow = 24;		/* Terminal rows, update on startup */
short term_ncol = 80;		/* Terminal columns, update on startup */
short term_margin = 8;		/* Min margin for extended lines */
short term_scrsiz = 64;		/* Size of scroll region */

int ttrow;			/* Row location of HW cursor */
int ttcol;			/* Column location of HW cursor */
int vtrow;			/* Row location of SW cursor */
int vtcol;			/* Column location of SW cursor */

int display_ok = 0;		/* Display resources is ready or not */

int currow;			/* Cursor row */
int curcol;			/* Cursor column */

int lbound;			/* Leftmost column of current line being displayed */
int taboff;			/* Tab offset for display */

e_Kill *kbufp;			/* Current kill buffer chunk pointer */
e_Kill *kheadp;			/* Head of kill buffer chunks */
int kused = KBLOCK;		/* # of bytes used in kill buffer */

long envram = 0;		/* # of bytes current in use by malloc */

char *fline = NULL;		/* Dynamic return line */
int flen = 0;			/* Current length of fline */

int scrollcount = 1;		/* Number of lines to scroll */

e_Window *curwp;		/* Current window */

e_Window *wheadp;		/* Head of list of windows */
e_Buffer *bheadp;		/* Head of list of buffers */

int curgoal;			/* Goal for C-P, C-N */

int thisflag;			/* Flags, this command */
int lastflag;			/* Flags, last command */

char pat[NPAT];			/* Search pattern */
char rpat[NPAT];		/* Replacement pattern */

char exact_search = 0;		/* Do case sensitive search */
