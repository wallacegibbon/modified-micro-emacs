#ifndef __EDEF_H
#define __EDEF_H

#include "estruct.h"

extern e_KeyBind bindings[];	/* Key to function table */

extern int kbdm[];		/* Holds keyboard macro data */

extern int *kbdptr;		/* Current position in keyboard buf */
extern int *kbdend;		/* Ptr to end of the keyboard */
extern int kbdmode;		/* Current keyboard macro mode */
extern int kbdrep;		/* Number of repetitions */

extern char hex[];

extern int sgarbf;		/* State of screen unknown */
extern int mpresf;		/* Stuff in message line */

extern short term_nrow;		/* Terminal rows, update on startup */
extern short term_ncol;		/* Terminal columns, update on startup */
extern short term_margin;	/* Min margin for extended lines */
extern short term_scrsiz;	/* Size of scroll region */

extern int ttrow;		/* Row location of HW cursor */
extern int ttcol;		/* Column location of HW cursor */
extern int vtrow;		/* Row location of SW cursor */
extern int vtcol;		/* Column location of SW cursor */

extern int display_ok;		/* Display resources is ready or not */

extern int currow;		/* Cursor row */
extern int curcol;		/* Cursor column */

extern int lbound;		/* Leftmost column of current line displayed */
extern int taboff;		/* Tab offset for display */

extern e_Kill *kbufp;		/* Current kill buffer chunk pointer */
extern e_Kill *kheadp;		/* Head of kill buffer chunks */
extern int kused;		/* # of bytes used in KB */

extern long envram;		/* # of bytes current in use by malloc */

extern char *fline;		/* Dynamic return line */
extern int flen;		/* Current length of fline */

extern int scrollcount;		/* Number of lines to scroll */

extern e_Window *curwp;		/* Current window */

extern e_Window *wheadp;	/* Head of list of windows */
extern e_Buffer *bheadp;	/* Head of list of buffers */

extern int curgoal;		/* Goal for C-P, C-N */

extern int thisflag;		/* Flags, this command */
extern int lastflag;		/* Flags, last command */

extern char pat[];		/* Search pattern. */
extern char rpat[];		/* Replacement pattern. */

extern char exact_search;	/* Do case sensitive search */

extern int reeat_char;

#endif
