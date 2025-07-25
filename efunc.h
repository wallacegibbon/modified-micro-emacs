#include <stdarg.h>

/* window.c */
int redraw(int f, int n);
int nextwind(int f, int n);
int prevwind(int f, int n);
int onlywind(int f, int n);
int delwind(int f, int n);
int splitwind(int f, int n);
int adjust_on_scr_resize(void);

/* basic.c */
int gotobol(int f, int n);
int backchar(int f, int n);
int gotoeol(int f, int n);
int forwchar(int f, int n);
int gotoline(int f, int n);
int gotobob(int f, int n);
int gotoeob(int f, int n);
int forwline(int f, int n);
int backline(int f, int n);
int gotobop(int f, int n);
int gotoeop(int f, int n);
int forwpage(int f, int n);
int backpage(int f, int n);
int setmark(int f, int n);
int swapmark(int f, int n);

/* random.c */
int getccol(int bflg);
int twiddle(int f, int n);
int quote(int f, int n);
int openline(int f, int n);
int newline(int f, int n);
int newline_and_indent(int f, int n);
int forwdel(int f, int n);
int backdel(int f, int n);
int killtext(int f, int n);

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
void sizesignal(int signr);
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
int mlgetstring(const char *prompt, char *buf, int nbuf, int eolchar);
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
int fexist(char *fname);

/* spawn.c */
int spawncli(int f, int n);
int execprg(int f, int n);
int pipecmd(int f, int n);
int filter_buffer(int f, int n);
int sys(char *cmd);
int shellprog(char *cmd);
int execprog(char *cmd);

/* search.c */
int scanner(const char *pattern, int direct, int beg_or_end);
int eq(unsigned char bc, unsigned char pc);
void rvstrcpy(char *rvstr, char *str);
int qreplace(int f, int n);
int delins(int dlength, char *instr, int use_meta);
int boundry(struct line *curline, int curoff, int dir);

/* isearch.c */
int risearch(int f, int n);
int fisearch(int f, int n);
int isearch(int f, int n);
int scanmore(char *pattern, int dir);
int get_char(void);

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
