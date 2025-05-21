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

#define RAMSIZE	1		/* Dynamic RAM memory usage tracking */
#define RAMSHOW	1		/* Auto dynamic RAM reporting */

#define VT220	UNIX
#define VT100	0
#define ANSI	0
#define VT52	0
#define TCAP	UNIX

/* Size of terminal for a 1080p screen with terminus-font-16x32 is 120x33 */
#define MAXCOL	128
#define MAXROW	64

#define TYPEAH	1  /* type ahead causes update to be skipped */
#define VISMAC	0  /* update display during keyboard macros */

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
#define NMODES	3		/* # of modes */
#define NLOCKS	100		/* max # of file locks active */

#define HUGE    1000		/* Huge number */
#define KBLOCK	250		/* sizeof kill buffer chunks */

#define CTL	0x2000		/* Control flag, or'ed in */
#define META	0x4000		/* Meta flag, or'ed in */
#define CTLX	0x8000		/* ^X flag, or'ed in */

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

#define FIOSUC  0		/* File I/O, success. */
#define FIOFNF  1		/* File I/O, file not found. */
#define FIOEOF  2		/* File I/O, end of file. */
#define FIOERR  3		/* File I/O, error. */
#define FIOMEM	4		/* File I/O, out of memory */
#define FIOFUN	5		/* File I/O, eod of file/bad line */

#define CFCPCN  0x0001		/* Last command was C-P, C-N */
#define CFKILL  0x0002		/* Last command was a kill */

#define BELL	0x07		/* BELL character */
#define TAB	0x09		/* TAB character */
#define ESC     0x1B		/* ESC character. */

/* Integer difference between upper and lower case letters. */
#define DIFCASE		0x20

#ifdef islower
#undef islower
#endif

#ifdef isupper
#undef isupper
#endif

#define isvisible(c)	(((c) >= 0x20 && (c) <= 0x7E) || (c) == '\t')

/* The simplified macro version of functions in ctype.h */
#define islower(c)	('a' <= (c) && (c) <= 'z')
#define isupper(c)	('A' <= (c) && (c) <= 'Z')
#define isalpha(c)	(islower(c) || isupper(c))
#define isdigit(c)	('0' <= (c) && (c) <= '9')

#if RAMSIZE
#define malloc	allocate
#define free	release
#endif

#if CLEAN
#define exit(a)	cexit(a)
#endif

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
	int w_toprow;		/* Origin 0 top row of window (physical screen) */
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
	struct line *b_dotp;	/* Link to "." struct line structure */
	struct line *b_markp;	/* The same as the above two, */
	struct line *b_linep;	/* Link to the header struct line */
	int b_doto;		/* Offset of "." in above struct line */
	int b_marko;		/* but for the "mark" */
	int b_mode;		/* editor mode of this buffer */
	char b_active;		/* window activated flag */
	char b_nwnd;		/* Count of windows on buffer */
	char b_flag;		/* Flags */
	char b_fname[NFILEN];	/* File name */
	char b_bname[NBUFN];	/* Buffer name */
};

#define BFINVS  0x01		/* Internal invisable buffer */
#define BFCHG   0x02		/* Changed since last write */
#define BFTRUNC	0x04		/* buffer was truncated when read */

/* mode flags */
#define MDEXACT	0x0001		/* Exact matching for searches */
#define MDVIEW	0x0002		/* View (read-only) buffer */
#define MDASAVE	0x0004		/* Auto-save mode */

struct region {
	struct line *r_linep;	/* Origin struct line address. */
	int r_offset;		/* Origin struct line offset. */
	long r_size;		/* Length in characters. */
};

struct terminal {
	short t_mrow;		/* max number of rows allowable */
	short t_nrow;		/* current number of rows used */
	short t_mcol;		/* max Number of columns. */
	short t_ncol;		/* current Number of columns. */
	short t_margin;		/* min margin for extended lines */
	short t_scrsiz;		/* size of scroll region " */
	int t_pause;		/* # times thru update to pause */

	void (*t_open)(void);	/* Open terminal at the start. */
	void (*t_close)(void);	/* Close terminal at end. */
	void (*t_kopen)(void);	/* Open keyboard */
	void (*t_kclose)(void);	/* close keyboard */
	int (*t_getchar)(void);	/* Get character from keyboard. */
	int (*t_putchar)(int);	/* Put character to display. */
	void (*t_flush)(void);	/* Flush output buffers. */
	void (*t_move)(int, int);
				/* Move the cursor, origin 0. */
	void (*t_eeol)(void);	/* Erase to end of line. */
	void (*t_eeop)(void);	/* Erase to end of page. */
	void (*t_beep)(void);	/* Beep. */
	void (*t_rev)(int);	/* set reverse video state */
	int (*t_rez)(char *);	/* change screen resolution */
	void (*t_scroll)(int, int, int);
				/* scroll a region of the screen */
};

#define TTopen		(*term.t_open)
#define TTclose		(*term.t_close)
#define TTkopen		(*term.t_kopen)
#define TTkclose	(*term.t_kclose)
#define TTgetc		(*term.t_getchar)
#define TTputc		(*term.t_putchar)
#define TTflush		(*term.t_flush)
#define TTmove		(*term.t_move)
#define TTeeol		(*term.t_eeol)
#define TTeeop		(*term.t_eeop)
#define TTbeep		(*term.t_beep)
#define TTrev		(*term.t_rev)
#define TTrez		(*term.t_rez)
#define TTscroll	(*term.t_scroll)

struct key_tab {
	int k_code;		 /* Key code */
	int (*k_fp)(int, int);	 /* Routine to handle it */
};

struct name_bind {
	const char *n_name;	 /* name of function key */
	int (*n_func)(int, int); /* function name is bound to */
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
#define METAC		(CTL | '[')	/* META character */
#define ABORTC		(CTL | 'G')	/* ABORT command char */
#define ENTERC		(CTL | 'M')	/* ENTER char */
#define QUOTEC		(CTL | 'Q')	/* QUOTE char */
#define REPTC		(CTL | 'U')	/* Universal repeat char */

/* Miscellaneous */
#define TABMASK		0x07
#define INDENT_NO_SPACE	1

#define PROGRAM_NAME	"me"
#define PROGRAM_NAME_LONG	"Modified Micro Emacs"

#define VERSION	"0.1.0"
