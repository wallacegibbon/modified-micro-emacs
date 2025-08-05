#ifndef __EDEF_H
#define __EDEF_H

#include "estruct.h"
#include <stdlib.h>
#include <string.h>

extern struct key_tab keytab[];	/* key to function table */

extern int kbdm[];		/* Holds keyboard macro data */

extern int *kbdptr;		/* current position in keyboard buf */
extern int *kbdend;		/* ptr to end of the keyboard */
extern int kbdmode;		/* current keyboard macro mode */
extern int kbdrep;		/* number of repetitions */

extern char hex[];

extern int sgarbf;		/* State of screen unknown */
extern int mpresf;		/* Stuff in message line */

extern int ttrow;		/* Row location of HW cursor */
extern int ttcol;		/* Column location of HW cursor */
extern int vtrow;		/* Row location of SW cursor */
extern int vtcol;		/* Column location of SW cursor */

extern int display_ok;		/* Display resources is ready or not */

extern int currow;		/* Cursor row */
extern int curcol;		/* Cursor column */

extern int lbound;		/* leftmost column of current line displayed */
extern int taboff;		/* tab offset for display */

extern struct kill *kbufp;	/* current kill buffer chunk pointer */
extern struct kill *kbufh;	/* kill buffer header pointer */
extern int kused;		/* # of bytes used in KB */

extern long envram;		/* # of bytes current in use by malloc */

extern char *fline;		/* dynamic return line */
extern int flen;		/* current length of fline */

extern int scrollcount;		/* number of lines to scroll */

extern struct window *curwp;	/* Current window */
extern struct window *wheadp;	/* Head of list of windows */

extern struct buffer *curbp;	/* Current buffer */
extern struct buffer *bheadp;	/* Head of list of buffers */

extern int curgoal;		/* Goal for C-P, C-N */

extern int thisflag;		/* Flags, this command */
extern int lastflag;		/* Flags, last command */

extern char pat[];		/* Search pattern. */
extern char rpat[];		/* Replacement pattern. */

extern struct terminal term;

extern int reeat_char;

#endif
