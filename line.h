#ifndef __LINE_H
#define __LINE_H

struct line {
	struct line *l_fp;	/* Link to the next line */
	struct line *l_bp;	/* Link to the previous line */
	int l_size;		/* Allocated size */
	int l_used;		/* Used size */
	char l_text[];		/* A bunch of characters. */
};

static inline void line_insert(struct line *lp, struct line *lp_new)
{
	struct line *tmp = lp->l_bp;

	lp_new->l_bp = tmp;
	lp_new->l_fp = lp;
	tmp->l_fp = lp_new;
	lp->l_bp = lp_new;
}

static inline void line_replace(struct line *lp, struct line *lp_new)
{
	lp->l_bp->l_fp = lp_new;
	lp->l_fp->l_bp = lp_new;
	lp_new->l_fp = lp->l_fp;
	lp_new->l_bp = lp->l_bp;
}

static inline void line_unlink(struct line *lp)
{
	lp->l_bp->l_fp = lp->l_fp;
	lp->l_fp->l_bp = lp->l_bp;
}

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
