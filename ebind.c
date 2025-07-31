#include "estruct.h"
#include "edef.h"
#include "efunc.h"
#include "line.h"

struct key_tab keytab[] = {
	{ NULLPROC_KEY /* should be less than 0x20 */, nullproc },
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
	{ CTL | 'Y', yank },
	{ CTL | 'Z', backpage },

	{ CTLX | '(', ctlxlp },
	{ CTLX | ')', ctlxrp },
	{ CTLX | ',', gotobob },
	{ CTLX | '.', gotoeob },
	{ CTLX | '0', delwind },
	{ CTLX | '1', onlywind },
	{ CTLX | '2', splitwind },
	{ CTLX | '=', showpos },
	{ CTLX | '?', show_raminfo },
	{ CTLX | 'C', spawncli },
	{ CTLX | 'E', ctlxe },
	{ CTLX | 'G', gotoline },
	{ CTLX | 'J', copyregion },
	{ CTLX | 'K', killregion },
	{ CTLX | 'L', lowerregion },
	{ CTLX | 'M', setmark },
	{ CTLX | 'O', nextwind },
	{ CTLX | 'P', prevwind },
	{ CTLX | 'R', qreplace },
	{ CTLX | 'U', upperregion },
	{ CTLX | 'X', nextbuffer },

	{ CTLX | CTL | 'C', quit },
	{ CTLX | CTL | 'F', filefind },
	{ CTLX | CTL | 'K', killbuffer },
	{ CTLX | CTL | 'Q', bufrdonly },
	{ CTLX | CTL | 'S', filesave },
	{ CTLX | CTL | 'W', filewrite },
	{ CTLX | CTL | 'X', swapmark },
	{ CTLX | CTL | 'Z', quickexit },

	{ 0, NULL }
};
