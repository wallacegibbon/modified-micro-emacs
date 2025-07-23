#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"

struct key_tab keytab[] = {
	{ 0x7F, backdel },

	{ CTL | 'A', gotobol },
	{ CTL | 'B', backchar },
	{ CTL | 'D', forwdel },
	{ CTL | 'E', gotoeol },
	{ CTL | 'F', forwchar },
	{ CTL | 'G', ctrlg },
	{ CTL | 'H', backdel },
	{ CTL | 'J', newline_and_indent },
	{ CTL | 'K', killtext },
	{ CTL | 'L', redraw },
	{ CTL | 'M', newline },
	{ CTL | 'N', forwline },
	{ CTL | 'O', openline },
	{ CTL | 'P', backline },
	{ CTL | 'Q', quote },
	{ CTL | 'R', risearch },
	{ CTL | 'S', fisearch },
	{ CTL | 'V', forwpage },
	{ CTL | 'W', killregion },
	{ CTL | 'Y', yank },
	{ CTL | 'Z', backpage },

	{ META | ' ', setmark },
	{ META | '>', gotoeob },
	{ META | '<', gotobob },
	{ META | '%', qreplace },
	{ META | 'B', backword },
	{ META | 'C', capword },
	{ META | 'D', delfword },
	{ META | 'F', forwword },
	{ META | 'G', gotoline },
	{ META | 'L', lowerword },
	{ META | 'P', lastbuffer },
	{ META | 'U', upperword },
	{ META | 'W', copyregion },
#if NAMED_CMD
	{ META | 'X', namedcmd },
#endif
	{ META | 'Z', quickexit },
	{ META | 0x7F, delbword },

	{ META | CTL | 'H', delbword },
	{ META | CTL | 'V', scrnextdw },
	{ META | CTL | 'Z', scrnextup },

	{ CTLX | '=', showcpos },
	{ CTLX | '(', ctlxlp },
	{ CTLX | ')', ctlxrp },
	{ CTLX | '0', delwind },
	{ CTLX | '1', onlywind },
	{ CTLX | '2', splitwind },
	{ CTLX | 'B', usebuffer },
	{ CTLX | 'C', spawncli },
	{ CTLX | 'E', ctlxe },
	{ CTLX | 'K', killbuffer },
	{ CTLX | 'O', nextwind },
	{ CTLX | 'P', prevwind },
	{ CTLX | 'X', nextbuffer },

	{ CTLX | CTL | 'C', quit },
	{ CTLX | CTL | 'F', filefind },
	{ CTLX | CTL | 'L', lowerregion },
	{ CTLX | CTL | 'Q', bufrdonly },
	{ CTLX | CTL | 'S', filesave },
	{ CTLX | CTL | 'U', upperregion },
	{ CTLX | CTL | 'W', filewrite },
	{ CTLX | CTL | 'X', swapmark },

	{ CTLX | META | CTL | 'Z', nullproc },

	{ 0, NULL },	/* the end tag */
};
