#include "main.h"

#define crt0 _BSS_LOAD__ // wretched hack, but whatever.
extern struct {
	unsigned char SP_save;
	unsigned char SHFLOK_save; // dependent on crt0.s. For uknown reason cc65 monkeys with these.
	unsigned char LMARGN_save;
} crt0;

void main(int argc, char **argv)
{
	unsigned char err = ERR_NONE;
	argc=argc;argv=argv;
	OS.appmhi = OS.memtop;
	OS.shflok = crt0.SHFLOK_save;
	OS.lmargn = crt0.LMARGN_save;
	initChio();
	initScreen();
	resetVt();
	err = serialOpen("R:", strlen("R:"), BAUD_9600, 0);
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