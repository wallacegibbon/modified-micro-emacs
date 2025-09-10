#ifndef __ESTRUCT_H
#define __ESTRUCT_H

#define VISMAC		0	/* Update display during keyboard macros */

#define NFILEN		256	/* # of bytes, file name */
#define NSTRING		128	/* # of bytes, string buffers */
#define NKBDM		256	/* # of strokes, keyboard macro */
#define NPAT		64	/* # of bytes, pattern */

#define KBLOCK		256	/* Size of kill buffer chunks */

#define HUGE		1000	/* Huge number */

#define CTLX		0x8000	/* ^X flag, or'ed in */
#define CTL		0x4000	/* Control flag, or'ed in */

#define CTLXC		(CTL | 'X')	/* CTL-X prefix char */
#define ESCAPEC		(CTL | '[')	/* ESCAPE character */
#define ABORTC		(CTL | 'G')	/* ABORT command char */
#define ENTERC		(CTL | 'M')	/* ENTER char */
#define QUOTEC		(CTL | 'Q')	/* QUOTE char */
#define REPTC		(CTL | 'U')	/* Universal repeat char */

#define FALSE		0	/* False, no, bad, etc. */
#define TRUE		1	/* True, yes, good, etc. */

#define ABORT		2	/* Death, ^G, abort, etc. */
#define FAILED		3	/* Not quite fatal false return */

#define STOP		0	/* Keyboard macro not in use */
#define PLAY		1	/* Playing */
#define RECORD		2	/* Recording */

#define PTBEG		0	/* Leave the point at the beginning */
#define PTEND		1	/* Leave the point at the end */
#define FORWARD		0	/* Forward direction */
#define REVERSE		1	/* Backwards direction */

#define FIOSUC		0	/* File I/O, success. */
#define FIOFNF		1	/* File I/O, file not found. */
#define FIOEOF		2	/* File I/O, end of file. */
#define FIOERR		3	/* File I/O, error. */
#define FIOMEM		4	/* File I/O, out of memory */

#define CFCPCN		0x0001	/* Last command was C-P, C-N */
#define CFKILL		0x0002	/* Last command was a kill */

#define BELL		0x07	/* BELL character */

#define DIFCASE		0x20	/* 'a' - 'A' */

typedef struct e_Line e_Line;

struct e_Line {
	e_Line *l_fp;		/* Link to the next line */
	e_Line *l_bp;		/* Link to the previous line */
	int l_size;		/* Allocated size */
	int l_used;		/* Used size */
	char l_text[];		/* A bunch of characters. */
};

typedef struct e_Buffer e_Buffer;

struct e_Buffer {
	e_Buffer *b_bufp;	/* Link to the next buffer */
	e_Line *b_linep;	/* Link to the header line */
	e_Line *b_dotp;		/* Line containing "." */
	e_Line *b_markp;	/* Line containing "mark" */
	int b_doto;		/* Offset of "." */
	int b_marko;		/* Offset of "mark" */
	char b_nwnd;		/* Count of windows on buffer */
	char b_flag;		/* Flags */
	char b_fname[NFILEN];	/* File name */
};

#define BFACTIVE	0x01	/* Window activated flag */
#define BFCHG   	0x02	/* Changed since last write */
#define BFTRUNC		0x04	/* Buffer was truncated when read */
#define BFRDONLY	0x08	/* Buffer is readonly */

typedef struct e_Window e_Window;

struct e_Window {
	e_Window *w_wndp;	/* Link to the next window */
	e_Buffer *w_bufp;	/* Link to the buffer displayed in window */
	e_Line *w_linep;	/* Link to the top line in the window */
	e_Line *w_dotp;		/* Line containing "." */
	e_Line *w_markp;	/* Line containing "mark" */
	int w_doto;		/* Offset of "." */
	int w_marko;		/* Offset of "mark" */
	int w_toprow;		/* Row of window (physical screen) */
	int w_ntrows;		/* # of rows of text in window */
	char w_force;		/* If NZ, forcing row. */
	char w_flag;		/* Flags. */
};

#define WFFORCE		0x01	/* Window needs forced reframe */
#define WFMOVE		0x02	/* Movement from line to line */
#define WFEDIT		0x04	/* Editing within a line */
#define WFHARD		0x08	/* Better to a full display */
#define WFMODE		0x10	/* Update mode line. */

typedef struct e_Region e_Region;

struct e_Region {
	e_Line *r_linep;	/* Origin line address. */
	int r_offset;		/* Origin line offset. */
	long r_size;		/* Length in characters. */
};

typedef struct e_Kill e_Kill;

struct e_Kill {
	e_Kill *k_next;		/* Link to next chunk, NULL if last. */
	char k_chunk[KBLOCK];	/* Text. */
};

typedef struct e_KeyBind e_KeyBind;

struct e_KeyBind {
	int code;
	int (*fn)(int, int);
};

#define CMDBUFLEN	256	/* Length of our command buffer */

#define IS_REVERSE	0x12	/* Incremental search backward */
#define IS_FORWARD	0x13	/* Incremental search forward */

#define SCR_MIN_ROWS	3
#define SCR_MIN_COLS	8

#define TABMASK		0x07

#define NULLPROC_KEY	1	/* Should be less than 0x20 */

#endif
