
#include "main.h"

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
} configStruct;

configStruct configData;


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
	*argv = &configData.argv[0];
	drawChar(clearScreenChar);
	if (*err != ERR_NONE) {
		charToA(*err, error, 7);
		drawString(error);
		*err = ERR_NONE;
	}
	drawString("VT -- (c) Cathryn Mataga\x9b ");
	drawString(__DATE__);
	drawString(" ");
	drawString(__TIME__);
	drawString("\x9b");
	drawString(" BREAK to quit\x9b");
	do {
		drawString(" R: or N:ssh://[fqdn]\x9b");
		drawString(" URL:");
		len = getline(configData.url,URLLEN, err);
		if (*err != ERR_NONE)return;
		if (!sanitizeUrl(configData.url, len))continue;
		dev = configData.url[0];
	}while((dev != 'N') && (dev != 'R'));
	*argc = 1;

	configData.argv[(*argc)++] = configData.url;

	if (dev == 'R') {
		do {
			drawString(" 300,1200,2400,4800,9600,19200\x9b");
			drawString(" BAUD:");
			len = getline(&configData.baud[3], BAUDSTRLEN, err);
			if (*err != ERR_NONE)return;
			if (!sanitizeBaud(&configData.baud[3], len)) baud = BAUD_NONE;
			else baud = stringToBaud(&configData.baud[3]);
		}while(baud == BAUD_NONE);
		configData.baud[0] = '/';
		configData.baud[1] = 'B';
		configData.baud[2] = '=';
		configData.argv[(*argc)++] = configData.baud;
	} else if (dev == 'N') {
		if (urlIsType(configData.url, "ssh://")) {
			drawString(" USERNAME:");
			len = getline(&configData.user[3], USERPASSLEN, err);
			if (*err != ERR_NONE)return;
			if (len > 1) {
				crToZero(&configData.user[3], len);
				configData.user[0] = '/';
				configData.user[1] = 'U';
				configData.user[2] = '=';
				configData.argv[(*argc)++] = configData.user;
			}
			drawString(" PASSWORD:");
			len = getline(&configData.passwd[3], USERPASSLEN, err);
			if (*err != ERR_NONE)return;
			if (len > 1) {
				crToZero(&configData.passwd[3], len);
				configData.passwd[0] = '/';
				configData.passwd[1] = 'P';
				configData.passwd[2] = '=';
				configData.argv[(*argc)++] = configData.passwd;
			}
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
