
#include "main.h"

#define MAXARGV 8
#define MAXLEN 120
typedef struct {
	unsigned char *url;
#if SERIAL_ON
	unsigned char *baud;
#endif
#if FUJINET_ON
	unsigned char *user;
	unsigned char *passwd;
#endif
	char *argv[MAXARGV];
	openIoStruct openIo;
} configStruct;

configStruct configData = {0, 0, 0, 0};

void freeConfig(void)
{
	if (configData.url) {
		free(configData.url);
		configData.url = NULL;
	}
#if SERIAL_ON
	if (configData.baud) {
		free(configData.baud);
		configData.baud = NULL;
	}
#endif
#if FUJINET_ON
	if (configData.user) {
		free(configData.user);
		configData.user = NULL;
	}
	if (configData.passwd) {
		free(configData.passwd);
		configData.passwd = NULL;
	}
#endif
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

#if SERIAL_ON
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
#endif

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
	unsigned char *param, *r;
	unsigned char pLen = p?3:0, len;
	param = malloc(MAXLEN);
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
		r = malloc(strlen(param) + 1);
		if (!r) {
			*err = ERR_OUTOFMEMORY;
			free(param);
			return NULL;
		}
		strcpy(r, param);
		free(param);
		return r;
	} else {
		free(param);
		return NULL;
	}
}

#if SERIAL_ON && FUJINET_ON
#define PARSESTRING " R: or N:ssh://[fqdn]\x9b URL:"
#else 
	#if SERIAL_ON
	#define PARSESTRING " R:\0x9b URL:"
	#else
		#if  FUJINET_ON
			#define PARSESTRING " N:ssh://[fqdn]\0x9b URL:"
		#else
			#define PARSESTRING " \0x9b URL:"
		#endif
	#endif
#endif

void geturl(int *argc, char ***argv, unsigned char *err)
{
	static unsigned char error[] = "ERROR:    \x9b";
	unsigned char baud;
	unsigned char dev, n;
	freeConfig();
	*argc = 0;
	*argv = &configData.argv[0];
	drawChar(clearScreenChar);
	if (*err != ERR_NONE) {
		n = charToA(*err, error, 7);
		for (;n<10;n++) {
			error[10] = ' ';
		}
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
		configData.url = parseParam(PARSESTRING, 0, err);
		if (*err != ERR_NONE) return;
		if (!configData.url || !sanitizeUrl(configData.url))continue;
		dev = configData.url[0];
	}while((dev != 'N') && (dev != 'R'));
	*argc = 1;
	configData.argv[(*argc)++] = configData.url;
	switch(dev) {
#if SERIAL_ON
		case 'R':
			do {
				if (configData.baud)free(configData.baud);
				configData.baud = parseParam(" 300,1200,2400,4800,9600,19200\x9b BAUD:", 'B', err);
				if (*err != ERR_NONE) return;
				if (!configData.baud || !sanitizeBaud(&configData.baud[3]))continue;
				baud = stringToBaud(&configData.baud[3]);
			}while(baud == BAUD_NONE);
			configData.argv[(*argc)++] = configData.baud;
			break;
#endif 
#if FUJINET_ON
		case 'N': 
			if (urlIsType(configData.url, "ssh://")) {
				configData.user = parseParam(" USERNAME:", 'U', err);
				if (*err != ERR_NONE) return;
				if (configData.user)configData.argv[(*argc)++] = configData.user;
				configData.passwd = parseParam(" PASSWORD:", 'P', err);
				if (*err != ERR_NONE) return;
				if (configData.passwd)configData.argv[(*argc)++] = configData.passwd;
			}
			break;
#endif
	}
}

void parseCommandLine(int argc, char **argv, unsigned char **device, openIoStruct *openIo, unsigned char *err)
{
	int n, cnt = 0;
	unsigned char *s;
#if SERIAL_ON
	openIo->baudWordStop = BAUD_9600;
	openIo->mon = 0;
#endif
#if FUJINET_ON
	openIo->user = NULL;
	openIo->passwd = NULL;
#endif
	for (n = 1;n<argc;n++) if (argv[n][0] == '/') {
		s = &argv[n][2];
		if (*s == '=')s++;
		switch(toupper(argv[n][1])) {
#if SERIAL_ON
			case 'B':
				openIo->baudWordStop = stringToBaud(s);
				if (openIo->baudWordStop == BAUD_NONE) {
					*err = ERR_NOTIMPLEMENTED;
					return;
				}
				break;
#endif
#if FUJINET_ON
			case 'U':
				openIo->user = s;
				break;
			case 'P':
				openIo->passwd = s;
				break;
#endif
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
