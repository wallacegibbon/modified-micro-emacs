#include "me.h"

int main(void)
{
	int c;
	TTopen();
	for (;;) {
		c = TTgetc();
		if (c == 3 /* Ctrl + C */)
			break;
		printf("<%02X>", c);
		fflush(stdout);
	}
	TTclose();
	printf("\r\n");
	return 0;
}
