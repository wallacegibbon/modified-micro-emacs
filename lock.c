#include "efunc.h"
#include "edef.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define LOCK_POSTFIX	".lock~"

/* lock name buffer shared by dolock and undolock */
static char lname_buf[NFILEN + 8];	/* filename + LOCK_POSTFIX */

static char *lname[NLOCKS];		/* names of all locked files */
static int numlocks;			/* # of current locks active */

static int dolock(char *fname, char **errstr);
static int undolock(char *fname, char **errstr);

/* check a file for locking and add it to the list */
int lockchk(char *fname)
{
	char *errstr;
	int s, i;

	/* check to see if that file is already locked here */
	if (numlocks > 0) {
		for (i = 0; i < numlocks; ++i)
			if (strcmp(fname, lname[i]) == 0)
				return TRUE;
	}

	/* if we have a full locking table, bitch and leave */
	if (numlocks == NLOCKS) {
		mlwrite("LOCK ERROR: Lock table full");
		return ABORT;
	}

	/* next, try to lock it */
	s = lock(fname);
	if (s == ABORT)	/* file is locked, no override */
		return ABORT;
	if (s == FALSE)	/* locked, overriden, dont add to table */
		return TRUE;

	/* we have now locked it, add it to our table */
	lname[++numlocks - 1] = malloc(strlen(fname) + 1);
	if (lname[numlocks - 1] == NULL) {	/* malloc failure */
		undolock(fname, &errstr);	/* free the lock */
		mlwrite("Cannot lock, out of memory");
		--numlocks;
		return ABORT;
	}

	/* everthing is cool, add it to the table */
	strcpy(lname[numlocks - 1], fname);
	return TRUE;
}

/* Release all the file locks so others may edit */
int lockrel(void)
{
	int s1, s2, i;

	s1 = TRUE;
	if (numlocks > 0) {
		for (i = 0; i < numlocks; ++i) {
			if ((s2 = unlock(lname[i])) != TRUE)
				s1 = s2;
			free(lname[i]);
		}
	}
	numlocks = 0;
	return s1;
}

static inline void lckerror(char *errstr)
{
	mlwrite("%s - %s", errstr, strerror(errno));
}

/*
 * Check and lock a file from access by others.  Returns
 * TRUE  = files was not locked and now is;
 * FALSE = file was locked and overridden;
 * ABORT = file was locked, abort command.
 */
int lock(char *fname)
{
	char *errstr;
	int s;

	if ((s = dolock(fname, &errstr)) == 0)
		return TRUE;

	if (s < 0) {
		lckerror(errstr);
		return ABORT;
	}

	if (mlyesno("File in use, override") == TRUE)
		return FALSE;
	else
		return ABORT;
}

/* Unlock a file.  This only warns the user if it fails */
int unlock(char *fname)
{
	char *errstr;

	if (undolock(fname, &errstr) == 0)
		return TRUE;

	lckerror(errstr);
	return FALSE;
}


/* Return 0 on success, 1 on fail, -1 on error. */
static int dolock(char *fname, char **errstr)
{
	struct stat sbuf;
	int mask, fd;

	strcat(strcpy(lname_buf, fname), LOCK_POSTFIX);

#if defined(S_IFLNK)
	if (lstat(lname_buf, &sbuf) == 0) {
#else
	if (stat(lname_buf, &sbuf) == 0) {
#endif

#if defined(S_ISREG)
		if (!S_ISREG(sbuf.st_mode)) {
#else
		if (!(((sbuf.st_mode) & 070000) == 0)) {	/* SysV R2 */
#endif
			*errstr = "LOCK ERROR: not a regular file";
			return -1;
		} else {
			return 1;
		}
	}

	mask = umask(0);
	fd = open(lname_buf, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		*errstr = "LOCK ERROR: cannot access lock file";
		return -1;
	}
	umask(mask);
	if (close(fd)) {
		*errstr = "LOCK ERROR: cannot close lock file";
		return -1;
	}

	return 0;
}

static int undolock(char *fname, char **errstr)
{
	strcat(strcpy(lname_buf, fname), LOCK_POSTFIX);
	if (unlink(lname_buf) != 0) {
		*errstr = "LOCK ERROR: cannot remove lock file";
		return -1;
	}
	return 0;
}
