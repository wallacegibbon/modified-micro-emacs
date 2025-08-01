#ifndef __ESTRUCT_H
#define __ESTRUCT_H

#if defined(BSD) || defined(sun) || defined(ultrix) || defined(__osf__) || \
		(defined(vax) && defined(unix))
	#ifndef BSD
	#define BSD 1
	#endif
#else
	#define BSD 0
#endif

#if defined(SVR4) || defined(__linux__)	/* ex. SunOS 5.3 */
	#define SVR4 1
	#define SYSV 1
	#undef BSD
#endif

#if defined(SYSV) || defined(u3b2) || defined(_AIX) || defined(__hpux) || \
		(defined(i386) && defined(unix))
	#define USG 1
#else
	#define USG 0
#endif

#define UNIX	(BSD | USG)

#define VISMAC	0		/* update display during keyboard macros */

#ifdef SVR4
#define FILOCK  1
#else
#define FILOCK	BSD
#endif

#define CLEAN	0  /* de-alloc memory on exit */

#define XONXOFF	UNIX

#define NFILEN  256		/* # of bytes, file name */
#define NBUFN   16		/* # of bytes, buffer name */
#define NLINE   256		/* # of bytes, input line */
#define NSTRING	128		/* # of bytes, string buffers */
#define NKBDM   256		/* # of strokes, keyboard macro */
#define NPAT    128		/* # of bytes, pattern */
#define NLOCKS	100		/* max # of file locks active */

#define KBLOCK	250		/* sizeof kill buffer chunks */

#define HUGE    1000		/* Huge number */

#define CTLX	0x8000		/* ^X flag, or'ed in */
#define CTL	0x4000		/* Control flag, or'ed in */

#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif

#define FALSE   0		/* False, no, bad, etc. */
#define TRUE    1		/* True, yes, good, etc. */
#define ABORT   2		/* Death, ^G, abort, etc. */
#define FAILED	3		/* not-quite fatal false return */

#define STOP	0		/* keyboard macro not in use */
#define PLAY	1		/* playing */
#define RECORD	2		/* recording */

#define PTBEG	0		/* Leave the point at the beginning on search */
#define PTEND	1		/* Leave the point at the end on search */
#define FORWARD	0		/* forward direction */
#define REVERSE	1		/* backwards direction */

#define FIOSUC	0		/* File I/O, success. */
#define FIOFNF	1		/* File I/O, file not found. */
#define FIOEOF	2		/* File I/O, end of file. */
#define FIOERR	3		/* File I/O, error. */
#define FIOMEM	4		/* File I/O, out of memory */

#define CFCPCN  0x0001		/* Last command was C-P, C-N */
#define CFKILL  0x0002		/* Last command was a kill */

#define BELL	0x07		/* BELL character */

/* Integer difference between upper and lower case letters. */
#define DIFCASE	0x20

/*
 * The windows are kept in a big list, in top to bottom screen order, with the
 * listhead at "wheadp".
 * The flag field contains some bits that are set by commands to guide
 * redisplay.  Although this is a bit of a compromise in terms of decoupling,
 * the full blown redisplay is just too expensive to run for every input
 * character.
 */
struct window {
	struct window *w_wndp;	/* Next window */
	struct buffer *w_bufp;	/* Buffer displayed in window */
	struct line *w_linep;	/* Top line in the window */
	struct line *w_dotp;	/* Line containing "." */
	struct line *w_markp;	/* Line containing "mark" */
	int w_doto;		/* Byte offset for "." */
	int w_marko;		/* Byte offset for "mark" */
	int w_toprow;		/* row of window (physical screen) */
	int w_ntrows;		/* # of rows of text in window */
	char w_force;		/* If NZ, forcing row. */
	char w_flag;		/* Flags. */
};

#define WFFORCE 0x01		/* Window needs forced reframe */
#define WFMOVE  0x02		/* Movement from line to line */
#define WFEDIT  0x04		/* Editing within a line */
#define WFHARD  0x08		/* Better to a full display */
#define WFMODE  0x10		/* Update mode line. */
#define WFKILLS 0x40		/* Something was deleted */
#define WFINS   0x80		/* Something was inserted */

/*
 * Buffers may be "Inactive" which means the files associated with them
 * have not been read in yet.  These get read in at "use buffer" time.
 */
struct buffer {
        struct buffer *b_bufp;	/* Link to next struct buffer */
	struct line *b_linep;	/* Link to the header struct line */
	struct line *b_dotp;	/* Link to "." struct line structure */
	struct line *b_markp;	/* The same as the above two, */
	int b_doto;		/* Offset of "." in above struct line */
	int b_marko;		/* but for the "mark" */
	char b_nwnd;		/* Count of windows on buffer */
	char b_flag;		/* Flags */
	char b_fname[NFILEN];	/* File name */
	char b_bname[NBUFN];	/* Buffer name */
};

#define BFACTIVE	0x01	/* window activated flag */
#define BFCHG   	0x02	/* Changed since last write */
#define BFTRUNC		0x04	/* Buffer was truncated when read */
#define BFRDONLY	0x08	/* Buffer is readonly */

struct region {
	struct line *r_linep;	/* Origin struct line address. */
	int r_offset;		/* Origin struct line offset. */
	long r_size;		/* Length in characters. */
};

struct terminal {
	short t_nrow;		/* current number of rows used */
	short t_ncol;		/* current Number of columns. */
	short t_margin;		/* min margin for extended lines */
	short t_scrsiz;		/* size of scroll region " */
	void (*t_open)(void);	/* Open terminal at the start. */
	void (*t_close)(void);	/* Close terminal at end. */
	int (*t_getchar)(void);	/* Get character from keyboard. */
	int (*t_putchar)(int);	/* Put character to display. */
	void (*t_flush)(void);	/* Flush output buffers. */
	void (*t_move)(int, int);	/* Move the cursor, origin 0. */
	void (*t_eeol)(void);	/* Erase to end of line. */
	void (*t_eeop)(void);	/* Erase to end of page. */
	void (*t_beep)(void);	/* Beep. */
	void (*t_rev)(int);	/* set reverse video state */
};

struct key_tab {
	int k_code;
	int (*fn)(int, int);
};

struct name_bind {
	const char *f_name;
	int (*fn)(int, int);
};

/*
 * The kill buffer is logically a stream of ascii characters, however
 * due to its unpredicatable size, it gets implemented as a linked
 * list of chunks.
 * `d_` prefix is for "deleted" text, as `k_` was taken up by the keycode.
 */
struct kill {
	struct kill *d_next;   /* Link to next chunk, NULL if last. */
	char d_chunk[KBLOCK];  /* Deleted text. */
};

#define CMDBUFLEN	256	/* Length of our command buffer */

/* Incremental search defines. */
#define IS_REVERSE	0x12	/* Search backward */
#define IS_FORWARD	0x13	/* Search forward */

#define CTLXC		(CTL | 'X')	/* CTL-X prefix char */
#define ESCAPEC		(CTL | '[')	/* ESCAPE character */
#define ABORTC		(CTL | 'G')	/* ABORT command char */
#define ENTERC		(CTL | 'M')	/* ENTER char */
#define QUOTEC		(CTL | 'Q')	/* QUOTE char */
#define REPTC		(CTL | 'U')	/* Universal repeat char */

#define atleast(n, limit)	((n) > (limit) ? (n) : (limit))

#define SCR_MIN_ROWS	3
#define SCR_MIN_COLS	8

/* Miscellaneous */
#define TABMASK		0x07

#define NULLPROC_KEY	1

#endif
