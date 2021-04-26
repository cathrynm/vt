#include "main.h"

#define crt0 _BSS_LOAD__ // wretched hack, but whatever.
extern struct {
	unsigned char SP_save;
	unsigned char SHFLOK_save; // dependent on crt0.s. For uknown reason cc65 monkeys with these.
	unsigned char LMARGN_save;
} crt0;


openIoStruct openIo;

void main(int argc, char **argv)
{
	unsigned char n;
	unsigned char *device;
	unsigned char err = ERR_NONE;
	OS.appmhi = OS.memtop;
	OS.shflok = crt0.SHFLOK_save;
	OS.lmargn = crt0.LMARGN_save;
	for (n= 0;n<argc;n++) crToZero(argv[n], strlen(argv[n]));
	initDetect();

	for(;;) {
		if (argc < 2) {
			geturl(&argc, &argv, &err);
		}
		if ((argc < 2) || (err != ERR_NONE))break;
		err = ERR_NONE;
		parseCommandLine(argc, argv, &device, &openIo, &err);
		if (err != ERR_NONE) {
			argc = 0;
			continue;
		}
		initChio();
		initScreen();
		resetVt();
		ioOpen(device, strlen(device), &openIo, &err);
		for(;err == ERR_NONE;) {
			flushBuffer();
			fixCursor();
			handleInput(&err);
			if (err != ERR_NONE)break;
			readData(&err);
			if (err != ERR_NONE)break;
			if (OS.brkkey != 0x80) {err = ERR_BREAK;break;}
		}
		OS.brkkey = 0x80; // Turn off break flag
		ioClose(&err);
		argc = 0;
		screenRestore();
		closeChio();
	}
	closeDetect();
}