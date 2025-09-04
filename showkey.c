#include "me.h"

int main(void)
{
	int c;
	ansiopen();
	for (;;) {
		if ((c = ttgetc()) == 3 /* Ctrl + C */)
			break;
		printf("%02X\r\n", c);
	}
	ansiclose();
	printf("\r\n");
	return 0;
}
