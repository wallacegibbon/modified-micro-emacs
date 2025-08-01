#ifndef __EFUNC_H
#define __EFUNC_H

#include "estruct.h"
#include <stddef.h>
#include <stdarg.h>

/* Loop utilities */
#define for_each_wind(wp) \
	for ((wp) = wheadp; (wp) != NULL; (wp) = (wp)->w_wndp)

#define for_each_buff(bp) \
	for ((bp) = bheadp; (bp) != NULL; (bp) = (bp)->b_bufp)

#define for_each_kbuf(kp) \
	for ((kp) = kbufh; (kp) != NULL; (kp) = (kp)->d_next)

#define isvisible(c) \
	(((c) >= 0x20 && (c) <= 0x7E) || (c) == '\t')

#define malloc	allocate
#define free	release

#if CLEAN
#define exit(a)	cexit(a)
#endif

#ifdef islower
#undef islower
#endif

#ifdef isupper
#undef isupper
#endif

/* The simplified macro version of functions in ctype.h */
#define islower(c)	('a' <= (c) && (c) <= 'z')
#define isupper(c)	('A' <= (c) && (c) <= 'Z')
#define isalpha(c)	(islower(c) || isupper(c))
#define isdigit(c)	('0' <= (c) && (c) <= '9')

#define TTopen		(term.t_open)
#define TTclose		(term.t_close)
#define TTgetc		(term.t_getchar)
#define TTputc		(term.t_putchar)
#define TTflush		(term.t_flush)
#define TTmove		(term.t_move)
#define TTeeol		(term.t_eeol)
#define TTeeop		(term.t_eeop)
#define TTbeep		(term.t_beep)
#define TTrev		(term.t_rev)


/* Useful inline functions */
static inline int ensure_lower(int c)
{
	return isupper(c) ? c ^ DIFCASE : c;
}

static inline int ensure_upper(int c)
{
	return islower(c) ? c ^ DIFCASE : c;
}

static inline void wstate_restore(struct window *wp, struct buffer *bp)
{
	wp->w_dotp = bp->b_dotp;
	wp->w_doto = bp->b_doto;
	wp->w_markp = bp->b_markp;
	wp->w_marko = bp->b_marko;
}

static inline void wstate_save(struct window *wp, struct buffer *bp)
{
	bp->b_dotp = wp->w_dotp;
	bp->b_doto = wp->w_doto;
	bp->b_markp = wp->w_markp;
	bp->b_marko = wp->w_marko;
}

static inline void wstate_copy(struct window *wp, struct window *wp2)
{
	wp->w_dotp = wp2->w_dotp;
	wp->w_doto = wp2->w_doto;
	wp->w_markp = wp2->w_markp;
	wp->w_marko = wp2->w_marko;
}


/* window.c */
int redraw(int f, int n);
int nextwind(int f, int n);
int prevwind(int f, int n);
int onlywind(int f, int n);
int delwind(int f, int n);
int splitwind(int f, int n);
int adjust_on_scr_resize(void);

/* basic.c */
int forwchar(int f, int n);
int backchar(int f, int n);
int forwline(int f, int n);
int backline(int f, int n);
int forwpage(int f, int n);
int backpage(int f, int n);
int gotobol(int f, int n);
int gotoeol(int f, int n);
int gotobob(int f, int n);
int gotoeob(int f, int n);
int gotoline(int f, int n);
int setmark(int f, int n);
int swapmark(int f, int n);

/* random.c */
int showpos(int flag, int n);
int getccol(int bflg);
int quote(int f, int n);
int openline(int f, int n);
int newline(int f, int n);
int newline_and_indent(int f, int n);
int forwdel(int f, int n);
int backdel(int f, int n);
int killtext(int f, int n);
int yank(int f, int n);
int show_raminfo(int f, int n);

/* main.c */
int (*getbind(int c))(int, int);
void edinit(char *bname);
int execute(int c, int f, int n);
int quickexit(int f, int n);
int quit(int f, int n);
int ctlxlp(int f, int n);
int ctlxrp(int f, int n);
int ctlxe(int f, int n);
int ctrlg(int f, int n);
int rdonly(void);
int nullproc(int f, int n);
int cexit(int status);

/* display.c */
void vtinit(void);
void vtdeinit(void);
void vttidy(void);
void vtmove(int row, int col);
int update(int force);
void update_garbage(void);
void update_modelines(void);
void movecursor(int row, int col);
void mlerase(void);
int mlwrite(const char *fmt, ...);
int mlvwrite(const char *fmt, va_list ap);
int unput_c(unsigned char c);
int put_c(unsigned char c, int (*p)(int));
int next_col(int col, unsigned char c);

/* region.c */
int killregion(int f, int n);
int copyregion(int f, int n);
int lowerregion(int f, int n);
int upperregion(int f, int n);
int getregion(struct region *rp);

/* posix.c */
void ttopen(void);
void ttclose(void);
int ttputc(int c);
void ttflush(void);
int ttgetc(void);
void getscreensize(int *widthp, int *heightp);

/* input.c */
int mlyesno(char *prompt);
int mlreply(char *prompt, char *buf, int nbuf);
int ectoc(int c);
int ctoec(int c);
int tgetc(void);
int get1key(void);
int getcmd(void);
int mlgetstring(char *buf, int nbuf, int eolchar, const char *fmt, ...);
int mlgetchar(const char *fmt, ...);

/* buffer.c */
int nextbuffer(int f, int n);
int swbuffer(struct buffer *bp);
int killbuffer(int f, int n);
int zotbuf(struct buffer *bp);
int bufrdonly(int f, int n);
void e_ltoa(char *buf, int width, long num);
int anycb(void);
int bclear(struct buffer *bp);
struct buffer *bfind(char *bname, int cflag, int bflag);

/* file.c */
int filefind(int f, int n);
int readin(char *fname, int lockfl);
void makename(char *bname, char *fname);
void unqname(char *name);
int filewrite(int f, int n);
int filesave(int f, int n);
int writeout(char *fn);

/* fileio.c */
int ffropen(char *fn);
int ffwopen(char *fn);
int ffclose(void);
int ffputline(char *buf, int nbuf);
int ffgetline(int *count);

/* spawn.c */
int spawncli(int f, int n);

/* search.c */
int search_next(const char *pattern, int direct, int beg_or_end);
void rvstrcpy(char *rvstr, char *str);
int qreplace(int f, int n);

/* isearch.c */
int risearch(int f, int n);
int fisearch(int f, int n);

/* lock.c */
int lockchk(char *fname);
int lockrel(void);
int lock(char *fname);
int unlock(char *fname);

/* memory.c */
void *allocate(unsigned long nbytes);
void release(void *mp);

/* util.c */
char *strncpy_safe(char *dest, const char *src, size_t size);

#endif
