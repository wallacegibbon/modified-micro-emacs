#include "estruct.h"
#include <stddef.h>

char hex[] = "0123456789ABCDEF";

int kbdm[NKBDM];		/* Keyboard Macro */
int *kbdptr;			/* current position in keyboard buf */
int *kbdend = kbdm;		/* ptr to end of the keyboard */
int kbdmode = STOP;		/* current keyboard macro mode */
int kbdrep;			/* number of repetitions */

int sgarbf = TRUE;		/* TRUE if screen is garbage */
int mpresf = FALSE;		/* TRUE if message in last line */

short term_nrow = 24;		/* Terminal rows, update on startup */
short term_ncol = 80;		/* Terminal columns, update on startup */
short term_margin = 8;		/* min margin for extended lines */
short term_scrsiz = 64;		/* size of scroll region */

int ttrow = HUGE;		/* Row location of HW cursor */
int ttcol = HUGE;		/* Column location of HW cursor */
int vtrow;			/* Row location of SW cursor */
int vtcol;			/* Column location of SW cursor */

int display_ok = 0;		/* Display resources is ready or not */

int currow;			/* Cursor row */
int curcol;			/* Cursor column */

int lbound;			/* leftmost column of current line being displayed */
int taboff;			/* tab offset for display */

struct kill *kbufp;		/* current kill buffer chunk pointer */
struct kill *kbufh;		/* kill buffer header pointer */
int kused = KBLOCK;		/* # of bytes used in kill buffer */

long envram = 0;		/* # of bytes current in use by malloc */

char *fline = NULL;		/* dynamic return line */
int flen = 0;			/* current length of fline */

int scrollcount = 1;		/* number of lines to scroll */

struct window *curwp;		/* Current window */

struct window *wheadp;		/* Head of list of windows */
struct buffer *bheadp;		/* Head of list of buffers */

int curgoal;			/* Goal for C-P, C-N */

int thisflag;			/* Flags, this command */
int lastflag;			/* Flags, last command */

char pat[NPAT];			/* Search pattern */
char rpat[NPAT];		/* Replacement pattern */
