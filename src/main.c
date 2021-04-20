#include "main.h"

#define crt0 _BSS_LOAD__ // wretched hack, but whatever.
extern struct {
	unsigned char SP_save;
	unsigned char SHFLOK_save; // dependent on crt0.s. For uknown reason cc65 monkeys with these.
	unsigned char LMARGN_save;
} crt0;


#define BAUDSTRLEN 16
#define URLLEN 254
#define MAXARGV 8
typedef struct {
	char *argv[MAXARGV];
	unsigned char url[URLLEN+1];
	unsigned char baud[BAUDSTRLEN+4];
} mainStruct;

mainStruct mainData;

void crToZero(unsigned char *s, unsigned char len)
{
	s[0] = toupper(s[0]);
	for (;len--;s++) {
		if (*s == 0x9b) {
			*s = 0;
			return;
		}
	}
	*s = 0;
}

void geturl(int *argc, char ***argv, unsigned char *err)
{
	unsigned char baud;
	unsigned char dev;
	unsigned char len;
	*argc = 0;
	*argv = &mainData.argv[0];
	drawChar(clearScreenChar);
	drawString("VT -- by Cathryn Mataga\x9b");
	drawString(" BREAK to quit\x9b");
	do {
		drawString(" R: or N:TCP://[fqdn]\x9b");
		drawString(" URL:");
		len = getline(mainData.url,URLLEN, err);
		if (*err != ERR_NONE)return;
		dev = toupper(mainData.url[0]);
	}while((dev != 'N') && (dev != 'R'));
	*argc = 1;
	crToZero(mainData.url, len);
	mainData.argv[(*argc)++] = mainData.url;

	if (dev == 'R') {
		do {
			drawString(" 300,1200,2400,4800,9600,19200\x9b");
			drawString(" BAUD:");
			len = getline(&mainData.baud[3], BAUDSTRLEN, err);
			if (*err != ERR_NONE)return;
			crToZero(&mainData.baud[3], len);
			baud = stringToBaud(&mainData.baud[3]);
		}while(baud == BAUD_NONE);
		mainData.baud[0] = '/';
		mainData.baud[1] = 'B';
		mainData.baud[2] = '=';
		mainData.argv[(*argc)++] = mainData.baud;
	}
}

void parseCommandLine(int argc, char **argv, unsigned char **device, unsigned char *baud, unsigned char *err)
{
	int n, cnt = 0;
	unsigned char *s;
	*baud = BAUD_9600;
	for (n = 1;n<argc;n++) if (argv[n][0] == '/') {
		switch(toupper(argv[n][1])) {
			case 'B':
				s = &argv[n][2];
				if (*s == '=')s++;
				*baud = stringToBaud(s);
				if (*baud == BAUD_NONE) {
					*err = ERR_NOTIMPLEMENTED;
					return;
				}
				break;
			default:
				break;
		}
	} else if (cnt++ == 0) {
		*device = &argv[n][0];
	}
	if (!cnt) {
		*err = ERR_NOTIMPLEMENTED;
	}
}

void main(int argc, char **argv)
{
	unsigned char baud;unsigned char *device;
	unsigned char err = ERR_NONE;
	OS.appmhi = OS.memtop;
	OS.shflok = crt0.SHFLOK_save;
	OS.lmargn = crt0.LMARGN_save;
	initChio();

	for(;;) {
		err = ERR_NONE;
		if (argc < 2) {
			geturl(&argc, &argv, &err);
		}
		if ((argc < 2) || (err != ERR_NONE))break;
		err = ERR_NONE;
		parseCommandLine(argc, argv, &device, &baud, &err);
		if (err != ERR_NONE) {
			argc = 0;
			continue;
		}
		initScreen();
		resetVt();
		ioOpen(device, strlen(device), baud, 0, &err);
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
	}
	closeChio();
}