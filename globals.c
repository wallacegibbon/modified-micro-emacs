#include "estruct.h"
#include "edef.h"

char hex[] = "0123456789ABCDEF";

int kbdm[NKBDM];		/* Keyboard Macro */
int *kbdptr;			/* current position in keyboard buf */
int *kbdend = kbdm;		/* ptr to end of the keyboard */
int kbdmode = STOP;		/* current keyboard macro mode */
int kbdrep;			/* number of repetitions */

int sgarbf = TRUE;		/* TRUE if screen is garbage */
int mpresf = FALSE;		/* TRUE if message in last line */

int ttrow = HUGE;		/* Row location of HW cursor */
int ttcol = HUGE;		/* Column location of HW cursor */
int vtrow;			/* Row location of SW cursor */
int vtcol;			/* Column location of SW cursor */

int currow;			/* Cursor row */
int curcol;			/* Cursor column */

int lbound;			/* leftmost column of current line being displayed */
int taboff;			/* tab offset for display */

struct kill *kbufp;		/* current kill buffer chunk pointer */
struct kill *kbufh;		/* kill buffer header pointer */
int kused = KBLOCK;		/* # of bytes used in kill buffer */

int lastkey;			/* last keystoke */

long envram;			/* # of bytes current in use by malloc */

char *fline;			/* dynamic return line */
int flen;			/* current length of fline */

int scrollcount = 1;		/* number of lines to scroll */

unsigned int matchlen;		/* The length of the matched string */
unsigned int mlenold;
char *patmatch;			/* The string that satisfies the search */
struct line *matchline;		/* The line of the *start* of match */
int matchoff;			/* The offset of the *start* of match */

struct window *curwp;		/* Current window */
struct buffer *curbp;		/* Current buffer */
struct buffer *prevbp;		/* Previous buffer */
struct window *wheadp;		/* Head of list of windows */
struct buffer *bheadp;		/* Head of list of buffers */
struct buffer *blistp;		/* Buffer for C-X C-B */

int curgoal;			/* Goal for C-P, C-N */

int thisflag;			/* Flags, this command */
int lastflag;			/* Flags, last command */
int saveflag;

char pat[NPAT];			/* Search pattern */
char tap[NPAT];			/* Reversed pattern array. */
char rpat[NPAT];		/* Replacement pattern */
