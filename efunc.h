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
	for ((kp) = kheadp; (kp) != NULL; (kp) = (kp)->k_next)

#define atleast(n, limit) \
	((n) > (limit) ? (n) : (limit))

#define inside(n, min, max) \
	((n) < (min) ? (min) : (n) > (max) ? (max) : n)

#define isvisible(c) \
	(((c) >= 0x20 && (c) <= 0x7E) || (c) == '\t')

/* The simplified macro version of functions in ctype.h */
#define islower(c)	('a' <= (c) && (c) <= 'z')
#define isupper(c)	('A' <= (c) && (c) <= 'Z')
#define isalpha(c)	(islower(c) || isupper(c))
#define isdigit(c)	('0' <= (c) && (c) <= '9')

#define malloc		allocate
#define free		release

static inline int ensure_lower(int c)
{
	return isupper(c) ? c ^ DIFCASE : c;
}

static inline int ensure_upper(int c)
{
	return islower(c) ? c ^ DIFCASE : c;
}

static inline void line_insert(e_Line *lp, e_Line *lp_new)
{
	e_Line *tmp = lp->l_bp;

	lp_new->l_bp = tmp;
	lp_new->l_fp = lp;
	tmp->l_fp = lp_new;
	lp->l_bp = lp_new;
}

static inline void line_replace(e_Line *lp, e_Line *lp_new)
{
	lp->l_bp->l_fp = lp_new;
	lp->l_fp->l_bp = lp_new;
	lp_new->l_fp = lp->l_fp;
	lp_new->l_bp = lp->l_bp;
}

static inline void line_unlink(e_Line *lp)
{
	lp->l_bp->l_fp = lp->l_fp;
	lp->l_fp->l_bp = lp->l_bp;
}

static inline void wstate_restore(e_Window *wp, e_Buffer *bp)
{
	wp->w_dotp = bp->b_dotp;
	wp->w_doto = bp->b_doto;
	wp->w_markp = bp->b_markp;
	wp->w_marko = bp->b_marko;
}

static inline void wstate_save(e_Window *wp)
{
	e_Buffer *bp = wp->w_bufp;
	bp->b_dotp = wp->w_dotp;
	bp->b_doto = wp->w_doto;
	bp->b_markp = wp->w_markp;
	bp->b_marko = wp->w_marko;
}

static inline void wstate_copy(e_Window *wp, e_Window *wp2)
{
	wp->w_dotp = wp2->w_dotp;
	wp->w_doto = wp2->w_doto;
	wp->w_markp = wp2->w_markp;
	wp->w_marko = wp2->w_marko;
}

/* line.c */
e_Line *lalloc(int);
void lfree(e_Line *lp);
int linstr(char *instr);
int linsert(int n, int c);
int ldelete(long n, int kflag);
void lchange(int flag);
int linsert_kbuf(void);

/* Kill buffer functions */
int kinsert(int c);
void kdelete(void);

/* buffer.c */
int nextbuffer(int f, int n);
int swbuffer(e_Buffer *bp);
int zotbuf(e_Buffer *bp);
int toggle_rdonly(int f, int n);
void e_ltoa(char *buf, int width, long num);
int anycb(void);
int bclear(e_Buffer *bp);
e_Buffer *bfind(char *bname, int cflag);

/* window.c */
int splitwind(int f, int n);
int onlywind(int f, int n);
int redraw(int f, int n);
int nextwind(int f, int n);
int prevwind(int f, int n);
void resetwind(e_Window *wp);

/* command.c */
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
int show_misc_info(int f, int n);
int quote(int f, int n);
int newline(int f, int n);
int newline_and_indent(int f, int n);
int forwdel(int f, int n);
int backdel(int f, int n);
int yank(int f, int n);
int killtext(int f, int n);
int killregion(int f, int n);
int copyregion(int f, int n);
int lowerregion(int f, int n);
int upperregion(int f, int n);
int spawncli(int f, int n);
int ctlxlp(int f, int n);
int ctlxrp(int f, int n);
int ctlxe(int f, int n);
int ctrlg(int f, int n);
int suspend(int f, int n);
int nullproc(int f, int n);
int rdonly(void);

/* main.c */
int quit(int f, int n);

/* display.c */
void vtinit(void);
void vtdeinit(void);
void vtmove(int row, int col);
int update(int force);
void update_garbage(void);
void update_modelines(void);
void movecursor(int row, int col);
void mlerase(void);
int mlwrite(const char *fmt, ...);
int mlvwrite(const char *fmt, va_list ap);
int unput_c(unsigned char c);
int put_c(unsigned char c, void (*p)(int));
int next_col(int col, unsigned char c);

/* posix.c */
void ttopen(void);
void ttclose(void);
void ttflush(void);
int ttgetc(void);
void ttputc(int c);
void ttputs(const char *str);
void suspend_self(void);
void bind_exithandler(void (*fn)(int));

/* unix.c */
void getscreensize(int *widthp, int *heightp);

/* ansi.c */
void ansiopen(void);
void ansiclose(void);
void ansimove(int, int);
void ansieeol(void);
void ansieeop(void);
void ansibeep(void);
void ansirev(int);

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

/* file.c */
int readin(char *fname);
int filefind(int f, int n);
int filewrite(int f, int n);
int filesave(int f, int n);
int writeout(char *fn);

/* fileio.c */
int ffropen(char *fn);
int ffwopen(char *fn);
int ffclose(void);
int ffputline(char *buf, int nbuf);
int ffgetline(int *count);

/* search.c */
int toggle_exact_search(int f, int n);
int search_next(const char *pattern, int direct, int beg_or_end);
int clear_rpat(int f, int n);
int qreplace(int f, int n);

/* isearch.c */
int fisearch(int f, int n);
int risearch(int f, int n);

/* memory.c */
void *allocate(unsigned long nbytes);
void release(void *mp);

/* util.c */
char *trim_spaces(char *dest, const char *src, size_t size, int *trunc);
char *strncpy_safe(char *dest, const char *src, size_t size);
void rvstrcpy(char *rvstr, char *str);
void die(int code, void (*deinit)(void), const char *fmt, ...);

#endif
