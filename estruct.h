#ifndef __ESTRUCT_H
#define __ESTRUCT_H

#define VISMAC		0	/* Update display during keyboard macros */

#define NCMDBUFLEN	256	/* Length of our command buffer */
#define NFILEN		256	/* # of bytes, file name */
#define NSTRING		128	/* # of bytes, string buffers */
#define NKBDM		256	/* # of strokes, keyboard macro */
#define NPAT		64	/* # of bytes, pattern */

#define KBLOCK		256	/* Size of kill buffer chunks */

#define CUSTOMFLAG	0x8000	/* Custom flag */
#define CTLX		0x4000	/* ^X flag */
#define CTL		0x2000	/* Control flag */

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

#define FIOSUC		0	/* File I/O, success */
#define FIOFNF		1	/* File I/O, file not found */
#define FIOEOF		2	/* File I/O, end of file */
#define FIOERR		3	/* File I/O, error */
#define FIOMEM		4	/* File I/O, out of memory */

#define CFCPCN		0x0001	/* Last command was C-P, C-N */
#define CFKILL		0x0002	/* Last command was a kill */

#define BELL		0x07	/* BELL character */
#define TABMASK		0x07
#define DIFCASE		0x20	/* 'a' - 'A' */

typedef int (*commandfn_t)(int f, int n);

struct line {
	struct line *l_fp;	/* Link to the next line */
	struct line *l_bp;	/* Link to the previous line */
	int l_size;		/* Allocated size */
	int l_used;		/* Used size */
	char l_text[];		/* A bunch of characters */
};

struct buffer {
	struct buffer *b_bufp;	/* Link to the next buffer */
	struct line *b_linep;	/* Link to the header line */
	struct line *b_dotp;	/* Line containing "." */
	struct line *b_markp;	/* Line containing "mark" */
	int b_doto;		/* Offset of "." */
	int b_marko;		/* Offset of "mark" */
	char b_nwnd;		/* Count of windows on buffer */
	char b_flag;		/* Flags */
	char b_fname[NFILEN];	/* File name */
};

#define BFACTIVE	0x01	/* Window activated flag */
#define BFCHG		0x02	/* Changed since last write */
#define BFTRUNC		0x04	/* Buffer was truncated when read */
#define BFRDONLY	0x08	/* Buffer is readonly */

struct window {
	struct window *w_wndp;	/* Link to the next window */
	struct buffer *w_bufp;	/* Link to the buffer displayed in window */
	struct line *w_linep;	/* Link to the top line in the window */
	struct line *w_dotp;	/* Line containing "." */
	struct line *w_markp;	/* Line containing "mark" */
	int w_doto;		/* Offset of "." */
	int w_marko;		/* Offset of "mark" */
	int w_toprow;		/* Row of window (physical screen) */
	int w_ntrows;		/* # of rows of text in window */
	char w_force;		/* If NZ, forcing row */
	char w_flag;		/* Flags */
};

#define WFFORCE		0x01	/* Window needs forced reframe */
#define WFMOVE		0x02	/* Movement from line to line */
#define WFEDIT		0x04	/* Editing within a line */
#define WFHARD		0x08	/* Better to a full display */
#define WFMODE		0x10	/* Update mode line */

struct region {
	struct line *r_linep;	/* Origin line address */
	int r_offset;		/* Origin line offset */
	long r_size;		/* Length in characters */
};

struct kill {
	struct kill *k_next;	/* Link to next chunk, NULL if last */
	char k_chunk[KBLOCK];	/* Text */
};

struct keybind {
	int code;
	commandfn_t fn;
};

#define IS_REVERSE	0x12	/* Incremental search backward */
#define IS_FORWARD	0x13	/* Incremental search forward */

#define SCR_MIN_ROWS	3
#define SCR_MIN_COLS	8

#define TERM_REINIT_KEY	0xF8	/* Use 0xF8 ~ 0xFF to bypass UTF-8 */

#endif
