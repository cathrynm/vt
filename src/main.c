#include "main.h"

void main(int argc, char **argv)
{
	unsigned char err = ERR_NONE;
	argc=argc;argv=argv;
	OS.appmhi = OS.memtop;
	initChio();
	initScreen();
	resetVt();
	err = serialOpen("R:", strlen("R:"), BAUD_19200, 0);
	for(;err == ERR_NONE;) {
		flushBuffer();
		fixCursor();
		err = handleInput();
		if (err != ERR_NONE)break;
		err = readData();
	}
	serialClose("R:", strlen("R:"));
	screenRestore();
	closeChio();
}