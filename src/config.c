
#include "main.h"

#define MAXARGV 8
#define MAXLEN 255
typedef struct {
	char *argv[MAXARGV];
	unsigned char *url; // [URLLEN+1];
	unsigned char *baud; // [BAUDSTRLEN + 4];
	unsigned char *user; // [USERPASSLEN+4];
	unsigned char *passwd; // [USERPASSLEN+4];
	openIoStruct openIo;
} configStruct;

configStruct configData = {0};

void freeConfig(void)
{
	if (configData.url) {
		free(configData.url);
		configData.url = NULL;
	}
	if (configData.baud) {
		free(configData.baud);
		configData.baud = NULL;
	}
	if (configData.user) {
		free(configData.user);
		configData.user = NULL;
	}
	if (configData.passwd) {
		free(configData.passwd);
		configData.passwd = NULL;
	}
}

void crToZero(unsigned char *s, unsigned char len)
{
	for (;*s && len--;s++) {
		if (*s == 0x9b)break;
	}
	*s = 0;
}

unsigned char sanitizeUrl(unsigned char *s)
{
	unsigned char n, m;
	unsigned char foundColon = 0, foundDevice = 0;
	s[0] = toupper(s[0]);
	for (m = 0, n = 0;s[n];n++) {
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

unsigned char sanitizeBaud(unsigned char *s)
{
	unsigned char n, m;
	unsigned char foundColon = 0;
	s[0] = toupper(s[0]);
	for (m = 0, n = 0;s[n];n++) {
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


unsigned char *parseParam(unsigned char *prompt, unsigned char p, unsigned char *err)
{
	unsigned char *param = malloc(MAXLEN);
	unsigned char pLen = p?3:0, len;
	if (!param) {
		*err = ERR_OUTOFMEMORY;
		freeConfig();
		return NULL;
	}
	drawString(prompt);
	len = getline(&param[pLen], MAXLEN - pLen - 1, err);
	if (*err != ERR_NONE) {
		free(param);
		freeConfig();
		return NULL;
	}
	if (len > 1) {
		crToZero(&param[pLen], len);
		if (p) {
			param[0] = '/';
			param[1] = p;
			param[2] = '=';
		}
		return realloc(param, strlen(param) + 1);
	} else {
		free(param);
		return NULL;
	}
}

void geturl(int *argc, char ***argv, unsigned char *err)
{
	static unsigned char error[] = "ERROR: 000\x9b";
	unsigned char baud;
	unsigned char dev;
	freeConfig();
	*argc = 0;
	*argv = &configData.argv[0];
	drawChar(clearScreenChar);
	if (*err != ERR_NONE) {
		charToA(*err, error, 7);
		drawString(error);
		*err = ERR_NONE;
	}
	drawString("VT -- (c) 2021 Cathryn Mataga\x9b ");
	drawString(__DATE__);
	drawString(" ");
	drawString(__TIME__);
	drawString("\x9b BREAK to quit\x9b");
	do {
		if (configData.url)free(configData.url);
		configData.url = parseParam(" R: or N:ssh://[fqdn]\x9b URL:", 0, err);
		if (*err != ERR_NONE) return;
		if (!configData.url || !sanitizeUrl(configData.url))continue;
		dev = configData.url[0];
	}while((dev != 'N') && (dev != 'R'));
	*argc = 1;
	configData.argv[(*argc)++] = configData.url;

	if (dev == 'R') {
		do {
			if (configData.baud)free(configData.baud);
			configData.baud = parseParam(" 300,1200,2400,4800,9600,19200\x9b BAUD:", 'B', err);
			if (*err != ERR_NONE) return;
			if (!configData.baud || !sanitizeBaud(&configData.baud[3]))continue;
			baud = stringToBaud(&configData.baud[3]);
		}while(baud == BAUD_NONE);
		configData.argv[(*argc)++] = configData.baud;
	} else if (dev == 'N') {
		if (urlIsType(configData.url, "ssh://")) {
			configData.user = parseParam(" USERNAME:", 'U', err);
			if (*err != ERR_NONE) return;
			if (configData.user)configData.argv[(*argc)++] = configData.user;
			configData.passwd = parseParam(" PASSWORD:", 'P', err);
			if (*err != ERR_NONE) return;
			if (configData.passwd)configData.argv[(*argc)++] = configData.passwd;
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
