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
#define USERPASSLEN 120
typedef struct {
	char *argv[MAXARGV];
	unsigned char url[URLLEN+1];
	unsigned char baud[BAUDSTRLEN + 4];
	unsigned char user[USERPASSLEN+4];
	unsigned char passwd[USERPASSLEN+4];
	openIoStruct openIo;
} mainStruct;

mainStruct mainData;

void crToZero(unsigned char *s, unsigned char len)
{
	for (;*s && len--;s++) {
		if (*s == 0x9b)break;
	}
	*s = 0;
}

unsigned char sanitizeUrl(unsigned char *s, unsigned char len)
{
	unsigned char n, m;
	unsigned char foundColon = 0, foundDevice = 0;
	s[0] = toupper(s[0]);
	for (m = 0, n = 0;n < len;n++) {
		if (s[n] == 0x9b) break;
		if (isspace(s[n]))continue;
		if (s[n] == ':') {
			foundColon = 1;
		}
		if (!m && isalpha(s[n])) {
			foundDevice = 1;
			s[m++] = toupper(s[n]);
		}
		else s[m++] = s[n];
	}
	s[m] = 0;
	return foundColon && foundDevice;
}

unsigned char sanitizeBaud(unsigned char *s, unsigned char len)
{
	unsigned char n, m;
	unsigned char foundColon = 0;
	s[0] = toupper(s[0]);
	for (m = 0, n = 0;n< len;n++) {
		if (s[n] == 0x9b) break;
		if (isspace(s[n]))continue;
		if (!isdigit(s[n]) && (s[n] != '.'))return 0;
		s[m++] = s[n];
	}
	s[m] = 0;
	return ( m > 0);
}

unsigned char urlIsType(unsigned char *s, unsigned char *typ) {
	for (;*s != ':';s++) {
		if (*s == 0)return 0;
	}
	s++;
	for (;*typ;typ++,s++) {
		if (toupper(*typ) != toupper(*s)) return 0;
	}
	return 1;
}

void geturl(int *argc, char ***argv, unsigned char *err)
{
	static unsigned char error[] = "ERROR: 000\x9b";
	unsigned char baud;
	unsigned char dev;
	unsigned char len;
	*argc = 0;
	*argv = &mainData.argv[0];
	drawChar(clearScreenChar);
	if (*err != ERR_NONE) {
		charToA(*err, error, 7);
		drawString(error);
		*err = ERR_NONE;
	}
	drawString("VT -- by Cathryn Mataga\x9b");
	drawString(" BREAK to quit\x9b");
	do {
		drawString(" R: or N:ssh://[fqdn]\x9b");
		drawString(" URL:");
		len = getline(mainData.url,URLLEN, err);
		if (*err != ERR_NONE)return;
		if (!sanitizeUrl(mainData.url, len))continue;
		dev = mainData.url[0];
	}while((dev != 'N') && (dev != 'R'));
	*argc = 1;

	mainData.argv[(*argc)++] = mainData.url;

	if (dev == 'R') {
		do {
			drawString(" 300,1200,2400,4800,9600,19200\x9b");
			drawString(" BAUD:");
			len = getline(&mainData.baud[3], BAUDSTRLEN, err);
			if (*err != ERR_NONE)return;
			if (!sanitizeBaud(&mainData.baud[3], len)) baud = BAUD_NONE;
			else baud = stringToBaud(&mainData.baud[3]);
		}while(baud == BAUD_NONE);
		mainData.baud[0] = '/';
		mainData.baud[1] = 'B';
		mainData.baud[2] = '=';
		mainData.argv[(*argc)++] = mainData.baud;
	} else if (dev == 'N') {
		if (urlIsType(mainData.url, "ssh://")) {
			drawString(" USERNAME:");
			len = getline(&mainData.user[3], USERPASSLEN, err);
			if (*err != ERR_NONE)return;
			mainData.user[0] = '/';
			mainData.user[1] = 'U';
			mainData.user[2] = '=';
			mainData.argv[(*argc)++] = mainData.user;
			drawString(" PASSWORD:");
			len = getline(&mainData.passwd[3], USERPASSLEN, err);
			if (*err != ERR_NONE)return;
			mainData.passwd[0] = '/';
			mainData.passwd[1] = 'P';
			mainData.passwd[2] = '=';
			mainData.argv[(*argc)++] = mainData.passwd;
		}
	}
}

void parseCommandLine(int argc, char **argv, unsigned char **device, openIoStruct *openIo, unsigned char *err)
{
	int n, cnt = 0;
	unsigned char *s;
	openIo->baudWordStop = BAUD_9600;
	openIo->mon = 0;
	openIo->user = NULL;
	openIo->passwd = NULL;
	for (n = 1;n<argc;n++) if (argv[n][0] == '/') {
		s = &argv[n][2];
		if (*s == '=')s++;
		switch(toupper(argv[n][1])) {
			case 'B':
				openIo->baudWordStop = stringToBaud(s);
				if (openIo->baudWordStop == BAUD_NONE) {
					*err = ERR_NOTIMPLEMENTED;
					return;
				}
				break;
			case 'U':
				openIo->user = s;
				break;
			case 'P':
				openIo->passwd = s;
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
		parseCommandLine(argc, argv, &device, &mainData.openIo, &err);
		if (err != ERR_NONE) {
			argc = 0;
			continue;
		}
		initChio();
		initScreen();
		resetVt();
		ioOpen(device, strlen(device), &mainData.openIo, &err);
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