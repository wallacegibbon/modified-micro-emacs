#ifndef __LINE_H
#define __LINE_H

struct line {
	struct line *l_fp;	/* Link to the next line */
	struct line *l_bp;	/* Link to the previous line */
	int l_size;		/* Allocated size */
	int l_used;		/* Used size */
	char l_text[];		/* A bunch of characters. */
};

#define lforw(lp)       ((lp)->l_fp)
#define lback(lp)       ((lp)->l_bp)
#define lgetc(lp, n)    ((lp)->l_text[(n)] & 0xFF)
#define lputc(lp, n, c) ((lp)->l_text[(n)] = (c))
#define llength(lp)     ((lp)->l_used)

struct line *lalloc(int);
void lfree(struct line *lp);

int linstr(char *instr);
int linsert(int n, int c);
int ldelete(long n, int kflag);

void lchange(int flag);

int linsert_kbuf(void);

int kinsert(int c);
void kdelete(void);

#endif
