#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include <stdio.h>
#include <unistd.h>

/*
 * Create a subjob with a copy of the command intrepreter in it.  When the
 * command interpreter exits, mark the screen as garbage so that you do a full
 * repaint.
 */
int spawncli(int f, int n)
{
#if UNIX
	char *cp;
	int r;

	movecursor(term.t_nrow, 0);
	TTflush();
	TTclose();
	if ((cp = getenv("SHELL")) != NULL && *cp != '\0')
		r = system(cp);
	else
		r = system("exec /bin/sh");

	sgarbf = TRUE;

	TTopen();
	update(TRUE);

	if (r == 0)
		return TRUE;

	mlwrite("Failed running external command");
	return FALSE;
#endif
}
