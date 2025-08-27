#include "me.h"

int main(void)
{
	int c;
	TTopen();
	for (;;) {
		if ((c = TTgetc()) == 3 /* Ctrl + C */)
			break;
		printf("%02X\r\n", c);
	}
	TTclose();
	printf("\r\n");
	return 0;
}
