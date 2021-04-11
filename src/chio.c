
#include "main.h"
#define XEPCH_ATASCII 0xd4
#define XEPCH_ATINT 0xd5
#define XEPCH_INTERN 0xd6


typedef struct chioDataStruct chioStruct;
struct chioDataStruct {
	unsigned char utfType;
	unsigned char utfIndex;
	unsigned short utfWord;
	unsigned long utfLong; // Super slow on 6502, but these are rare, so whatever.
	unsigned char osType;
	unsigned char *keyTab;
	unsigned char chbas; // Original character set coming in here.
	unsigned char fullAscii; // {}~` characters are copied in.
	unsigned char xep80;
	unsigned char xepCharset;
};

chioStruct chio;
unsigned char *eColon = "E:";

unsigned char isXep80(void) {
	return chio.xep80;
}

unsigned char isXep80Internal(void) {
	return chio.xepCharset == XEPCH_INTERN;
}

void drawChar(unsigned char ch)
{
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}


unsigned char setXepCharSet(unsigned char which)
{
	unsigned char err = ERR_NONE;
	if (!chio.xep80 || (which == chio.xepCharset))return err;
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = strlen(eColon);
	OS.iocb[0].aux1 = 12;
	OS.iocb[0].aux2 = which;
	OS.iocb[0].command = 20;
	cio(0);
	iocbErrUpdate(0, &err);
	if (err == ERR_NONE) {
		chio.xepCharset = which;
	}
	return err;
}

unsigned char isIntl(void)
{
	return (chio.chbas == 204);
}

unsigned char closeChio(void)
{
	unsigned char err = ERR_NONE;
	if (chio.xep80) {
		setXepCharSet(isIntl()? XEPCH_ATINT: XEPCH_ATASCII); 
	}
	if (chio.fullAscii) {
		OS.chbas = chio.chbas;
	}
	OS.iocb[2].command = IOCB_CLOSE;
	cio(2);
	iocbErrUpdate(2, &err);
	return err;
}
// https://en.wikipedia.org/wiki/ATASCII
// Note, only need to check ones that go past 16 bits here.  These all get processed as drawable, not controls.
void convertLongToVisibleChar(unsigned long c, unsigned char *ch, unsigned char *attrib)
{
	if (isIntl()) { // international 
		switch(c) {
			case 0x1F8B0:
				*ch = 0x7d;
				*attrib = 0;
				return;
			default:
				break;
		}
	} else {
		switch(c) {
			case 0x1FB87: // someone got these into UTF8 for Atascii, so going to use them
				*ch = 2;
				*attrib = 0;
				return;
			case 0x1FB82:
				*ch = 0xd;
				*attrib = 0;
				return;
			case 0x1F8B0:
				*ch = 0x7d;
				*attrib = 0;
				return;
			default:
				break;
		}
	}
	*ch = ERRCHAR;
	*attrib = ERRATTRIB;
}

void convertAsciiToVisibleChar(unsigned char *ch, unsigned char *attrib)
{
	if ((*ch >= 32 && *ch <=95) || (*ch >= 97 && *ch <= 122) || (*ch == 124))return;
	if (chio.fullAscii && ((*ch == 116) || (*ch == 123) || (*ch == 125) || (*ch == 126)))return;
	*attrib = ERRATTRIB;  // Atari are missing { } ` ~.  Show as undrawable.  127, 0-31 are undrawable anyway.
	*ch = ERRCHAR;
}

void convertShortToVisibleChar(unsigned short c, unsigned char *ch, unsigned char *attrib)
{
	*attrib = 0;
	if (isIntl()) { // international 
		switch(c) {
			case 0x00a0:
				*ch = ' ';
				return;
			case 0x00e1: // a 
				*ch = 0;
				return;
			case 0x00f9: // u
				*ch = 1;
				return;
			case 0x00d1: // N
				*ch = 2;
				return;
			case 0x00c9:  // E
				*ch = 3;
				return;
			case 0x00e7: // c
				*ch = 4;
				return;
			case 0x00f4: // o
				*ch = 5;
				return;
			case 0x00f2: // o
				*ch = 6;
				return;
			case 0x00ec: // i
				*ch = 7;
				return;

			case 0x00a3: // L
				*ch = 8;
				return;
			case 0x00ef: // i
				*ch = 9;
				return;
			case 0x00fc: // u
				*ch = 0xa;
				return;
			case 0x00e4: // a
				*ch = 0xb;
				return;
			case 0x00d6: // O
				*ch = 0xc;
				return;
			case 0x00fa: // u
				*ch = 0xd;
				return;
			case 0x00f3: // o
				*ch = 0xe;
				return;
			case 0x00f6: // o
				*ch = 0xf;
				return;

			case 0x00Dc: // U
				*ch = 0x10;
				return;
			case 0x00e2: // a
				*ch = 0x11;
				return;
			case 0x00fb: // u
				*ch = 0x12;
				return;
			case 0x00ee: // i
				*ch = 0x13;
				return;
			case 0x00e9: // e
				*ch = 0x14;
				return;
			case 0x00e8: // e
				*ch = 0x15;
				return;
			case 0x00f1: //n 
				*ch = 0x16;
				return;
			case 0x00ea: // e
				*ch = 0x17;
				return;

			case 0x00e5: // a
				*ch = 0x18;
				return;
			case 0x0032: // a
				*ch = 0x19;
				return; 
			case 0x00c5: // A
				*ch = 0x1a;
				return;

			case 0x00a1: // !
				*ch = 0x60;
				return;

			case 0x00c4: // A
				*ch = 0xfb;
				return;




			case 0x241B:
				*ch = 0x1b;
				return;
			case 0x2191:
				*ch = 0x1c;
				return;
			case 0x2193:
				*ch = 0x1d;
				return;
			case 0x2190:
				*ch = 0x1e;
				return;
			case 0x2192:
				*ch = 0x1f;
				return;
			case 0x25c0:
				*ch = 0x7e;
				return;
			case 0x25b6:
				*ch = 0x7f;
				return;
			default:
				break;
		}
	} else {
		switch(c) {
			case 0x00a0:
				*ch = ' ';
				return;

			case 0x2665: // cntrl characters
				*ch = 0;
				return;
			case 0x251C:
				*ch = 1;
				return;
			case 0x2518:
				*ch = 3;
				return;
			case 0x2524:
				*ch = 4;
				return;
			case 0x2510:
				*ch = 5;
				return;
			case 0x2571:
				*ch = 6;
				return;
			case 0x2572:
				*ch = 7;
				return;
			case 0x25E2:
				*ch = 8;
				return;
			case 0x2597:
				*ch = 9;
				return;
			case 0x25E3:
				*ch = 0xa;
				return;
			case 0x259D:
				*ch = 0xb;
				return;
			case 0x2598:
				*ch = 0xc;
				return;
			case 0x2582:
				*ch = 0xe;
				return;
			case 0x2596:
				*ch = 0xf;
				return;
			case 0x2663:
				*ch = 0x10;
				return;
			case 0x250C:
				*ch = 0x11;
				return;
			case 0x2500:
				*ch = 0x12;
				return;
			case 0x253C:
				*ch = 0x13;
				return;
			case 0x2022:
				*ch = 0x14;
				return;
			case 0x2584:
				*ch = 0x15;
				return;
			case 0x258E:
				*ch = 0x16;
				return;
			case 0x252C:
				*ch = 0x17;
				return;
			case 0x2534:
				*ch = 0x18;
				return;
			case 0x258C:
				*ch = 0x19;
				return;
			case 0x2514:
				*ch = 0x1a;
				return;
			case 0x241B:
				*ch = 0x1b;
				return;
			case 0x2191:
				*ch = 0x1c;
				return;
			case 0x2193:
				*ch = 0x1d;
				return;
			case 0x2190:
				*ch = 0x1e;
				return;
			case 0x2192:
				*ch = 0x1f;
				return;
			case 0x2666: // diamond
				*ch = 0x60;
				return;
			case 0x2660: //spade
				*ch = 0x7b;
				return;
			case 0x25c0:
				*ch = 0x7e;
				return;
			case 0x25b6:
				*ch = 0x7f;
				return;
			case 0x258a:
				*ch = 0x02;
				*attrib = 0x80;
				return;
			case 0x25e4:
				*ch = 0x08;
				*attrib = 0x80;
				return;
			case 0x259b:
				*ch = 0x09;
				*attrib = 0x80;
				return;
			case 0x25e5:
				*ch = 0x0a;
				*attrib = 0x80;
				return;
			case 0x2599:
				*ch = 0x0b;
				*attrib = 0x80;
				return;
			case 0x259f:
				*ch = 0x0c;
				*attrib = 0x80;
				return;
			case 0x2586:
				*ch = 0x0d;
				*attrib = 0x80;
				return;
			case 0x259c:
				*ch = 0x0f;
				*attrib = 0x80;
				return;

			case 0x25d8:
				*ch = 0x14;
				*attrib = 0x80;
				return;
			case 0x2580:
				*ch = 0x15;
				*attrib = 0x80;
				return;
			case 0x2590:
				*ch = 0x19;
				*attrib = 0x80;
				return;
			case 0x2588:
				*ch = 0x20;
				*attrib = 0x80;
				return;
			default:
				break;
		}
	}
	*ch = ' '; // inverse space for undrawable
	*attrib = 0x80;
}


void decodeUtf8Char(unsigned char c)
{
	unsigned char ch, attrib;
	switch(chio.utfType) {
		case 0:
			if (!(c & 0x80))processChar(c);
			else if ((c & 0xe0) == 0xc0) {
				chio.utfType = 1;
				chio.utfWord = ((unsigned short)c) << 6;
			} else if ((c & 0xf0) == 0xe0) {
				chio.utfType = 2;
				chio.utfWord = ((unsigned short)c) << 12;
				chio.utfIndex = 1;
			} else if ((c & 0xf8) == 0xf0) {
				chio.utfType = 3;
				chio.utfLong = ((unsigned long)c) << 18;
				chio.utfIndex = 1;
			} // else Invalid UTF8, just drop it.
			break;
		case 1:
			if ((c & 0xc0) == 0xc0) {
				chio.utfWord |= (c & 0x3f);
				convertShortToVisibleChar(chio.utfWord, &ch, &attrib);
				displayUtf8Char(ch, attrib);
			} 	// else invalid utftype
			chio.utfType = 0;
			break;
		case 2:
			if ((c & 0xc0) == 0xc0) {
				c &= 0x3f;
				if (chio.utfIndex == 1) {
					chio.utfWord |= ((unsigned short)c) << 6;
					chio.utfIndex++;
				} else {
					chio.utfWord |= ((unsigned short)c);
					convertShortToVisibleChar(chio.utfWord, &ch, &attrib);
					chio.utfType = 0;
				}
			} else chio.utfType = 0; // invalid utf8
			break;
		case 3:
			if ((c & 0xc0) == 0xc0) {
				c &= 0x3f;
				if (chio.utfIndex == 1) {
					chio.utfLong |= ((unsigned long)c) << 12;
					chio.utfIndex++;
				} else if (chio.utfIndex == 2) {
					chio.utfLong |= ((unsigned long)c) << 6;
					chio.utfIndex++;
				} else {
					chio.utfLong |= (unsigned long)c;
					convertLongToVisibleChar(chio.utfLong, &ch, &attrib);
					displayUtf8Char(ch, attrib);
					chio.utfType = 0;
				}
			} else chio.utfType = 0; // invalid utf8
			break;
	}
}


unsigned char getChar(unsigned char *err)
{
	unsigned char r;
	OS.iocb[2].buffer = &r;
	OS.iocb[2].buflen = 1;
	OS.iocb[2].command = IOCB_GETCHR;
	cio(2);
	iocbErrUpdate(2, err);
	return r;
}

// 0x80 = NULL key
// 0x81 = Atari key
//0x82 = Caps/Lower
// 83 = shift caps lower

#define AKEY_NULL 0x80
#define AKEY_ATR  0x81
#define AKEY_CLOW 0x82
#define AKEY_SCLOW 0x83
#define AKEY_CCLOW 0x84
#define AKEY_EOF 0x85
#define AKEY_CLICKTOG 0x89
#define AKEY_F1 0x8a // Up
#define AKEY_F2 0x8b // Down
#define AKEY_F3 0x8c // Left
#define AKEY_F4 0x8d // Right
#define AKEY_F1S 0x8e // PgUp
#define AKEY_F2S 0x8f // PgDown
#define AKEY_F3S 0x90 // Home
#define AKEY_F4S 0x91 // End


void click(void) {
	unsigned char n;
	if (chio.osType < 2 || !OS.noclik) {
		for (n = 0;n<128;n++) {
			GTIA_READ.consol = 0x7f;
			ANTIC.wsync = 0x7f;
		}
	}
}

unsigned char isKeyReady(void) {
	unsigned char ch = OS.ch, code;
	if (ch >= 0xff)return 0; // NO key pressed
	if (ch < 0xc0) {
		if (!chio.keyTab) return 1;
		code = chio.keyTab[ch];
		if (chio.osType >= 2 && code >= AKEY_F1 && code <= AKEY_F4)
			code = OS.fkdef[code - AKEY_F1];
		if ((code != AKEY_NULL) && (code != AKEY_ATR) && (code != AKEY_CLOW) && (code != AKEY_SCLOW) &&
			(code != AKEY_CCLOW) && (code != AKEY_CLICKTOG))return 1; // Shift functions turn into arrow keys with the 'super set' 
	} else code = AKEY_NULL;
	OS.holdch = ch; // Need to handle inv/capslock/clicktog here since this will never make it to getchar.
	OS.ch = 0xff;
	switch(code) {
		case AKEY_NULL: break;
		case AKEY_ATR:
			OS.invflg ^= 0x80;
			break;
		case AKEY_CLOW: 
			OS.shflok = 0;
			break;
		case AKEY_SCLOW:
			OS.shflok = 0x40;
			break;
		case AKEY_CCLOW:
			OS.shflok = 0x80;
			break;
		case AKEY_CLICKTOG:
			if (chio.osType >= 2) OS.noclik ^= 0xff;
			break;
		default:
			break;
	} 
	click();
	return 0;
}


unsigned char extraKey(void)
{
	unsigned char extra = 0;
	unsigned char ch = OS.ch;
	if ((ch == 0xff) || !chio.keyTab)return 0;
	if (ch >= 0xc0) {
		switch(chio.keyTab[ch & 0x3f]) {
			case ',':
				extra = '{';
				break;
			case '.':
				extra = '}'; 
				break;
			case '-':
				extra = CH_CURS_UP;
				OS.superf = 1;
				break;
			case '=':
				extra = CH_CURS_DOWN;
				OS.superf = 1;
				break;
			case '<':
				extra = CH_CURS_LEFT;
				OS.superf = 1;
				break;
			case '>':
				extra = CH_CURS_RIGHT;
				OS.superf = 1;
				break;
			case '\033':
				extra = '~';
				break;
			case '7': // ctrl-shift on apostrophe generates backwards apostraphe.
				extra = '`';
				break;
			default:
				break;
		}
	}
	if (!extra)return 0;
	OS.holdch = ch;
	OS.ch = 0xff;
	OS.atachr = extra;
	click(); // Swallow up the key here.
	return extra;
}

unsigned char convertAtasciiToUtf8(unsigned char c, unsigned char *utf8, unsigned char *utf8Len)
{
	c=c;utf8=utf8;utf8Len=utf8Len;
	return 0;
}

unsigned char handleInput(void)
{
	unsigned char err = ERR_NONE;
	static unsigned char cr = 0x9b, bs = 8, del = 0x7f, null = 0;
	unsigned char utf8[4];
	unsigned char superF, ch, shift = OS.ch & 0xc0, utf8Len;
	ch = extraKey(); // Check for any special keys first. 
	if (!ch) {
		if (!isKeyReady())return ERR_NONE;
		ch = getChar(&err);
		if (err != ERR_NONE)return err;
	}
	superF = OS.superf;
	switch(ch) {
		case CH_CURS_UP:
		case CH_CURS_DOWN:
		case CH_CURS_LEFT:
		case CH_CURS_RIGHT:
			if (superF) {
				vtSendPgUpDown(ch - CH_CURS_UP);
			} else {
				vtSendCursor(ch - CH_CURS_UP);
			}
			break;
		case CH_DEL:
			sendResponse(&bs, 1);
			break;
		case CH_DELCHR:
			sendResponse(&del, 1);
			break;
		case ' ':
			if ((shift & 0xc0) == 0x80) {
				sendResponse(&null, 1);
			} else sendResponse(" ", 1);
			break;
		case '.':
			if ((shift & 0xc0) == 0x80) sendResponse("\033", 1);
			else sendResponse(".", 1);
			break;
		case ',':
			if ((shift & 0xc0) == 0x80) sendResponse("\035", 1);
			else sendResponse(",", 1);
			break;
		case '?':
			if ((shift & 0xc0) == 0x80) sendResponse("\037", 1);
			else sendResponse("?", 1);
			break;
		case CH_EOL:
			vtSendCr();
			break;
		default: {
			// Control characters go through, also 
			if (((ch >= 1) && (ch <= 26)) || (ch >= 32 && ch <=95) || (ch >= 97 && ch <= 122) || (ch == 124) || (ch == CH_ESC)) {
				sendResponse(&ch, 1);
				break;
			}
			if (convertAtasciiToUtf8(ch, &utf8[0], &utf8Len)) {
				sendResponse(utf8, utf8Len);
			}
			break;
		}
	}
	return err;
}


static const unsigned char lCurly[8] =  {0x07, 0x18, 0x18, 0x70, 0x18, 0x18, 0x07, 0x00};
static const unsigned char rCurly[8] =  {0x70, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x70, 0x00};
static const unsigned char tilda[8] =   {0x00, 0x3d, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00};
static const unsigned char bapost[8] =  {0x30, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00};
static const unsigned char hashBox[8] = {0x00, 0x54, 0x2a, 0x54, 0x2a, 0x54, 0x2a, 0x00};

void memCopy(unsigned char *top, const unsigned char *fromp, unsigned short len)
{
	while(len--)*top++ = *fromp++;
}

void memClear(unsigned char *top, unsigned short len)
{
	while(len--)*top++ = 0;
}


void initAscii(unsigned char fontBase) {
	unsigned char *top  = (unsigned char *)(fontBase << 8);
	memCopy( top,  (unsigned char *) (OS.chbas <<8), 128 * 8);
	memCopy(&top[96 * 8], bapost, 8);
	memCopy(&top[123 * 8], lCurly, 8);
	memCopy(&top[125 * 8], rCurly, 8);
	memCopy(&top[126 * 8], tilda, 8);
	memCopy(&top[127 * 8], hashBox, 8);
	OS.chbas = fontBase;
	chio.fullAscii = 1;
}



unsigned char XEP80Test(void)
{
	unsigned char err;
	if (OS.rmargn > 39) {
		err = ERR_NONE;
		OS.rowcrs = 0;OS.colcrs = OS.lmargn;
		drawChar(CH_ESC);drawChar(255);
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = strlen(eColon);
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = 245;
		OS.iocb[0].command = 20;
		cio(0);
		iocbErrUpdate(0, &err);
		return err == ERR_NONE;
	}
	return 0;
}

unsigned char initChio(void) // Don't use malloc from here.
{
	unsigned char err = ERR_NONE;
	unsigned short startAddress = (unsigned short)_STARTADDRESS__; // Start the program on 0x400 boundary.  So 0x400 below is good
	memClear((unsigned char *)&chio, sizeof(chio));
	chio.xep80 = XEP80Test();
	chio.chbas = OS.chbas;
	if (((unsigned short) OS.memlo + 0x400 <= startAddress) && !chio.xep80) {
		startAddress -= 0x400;
		initAscii(startAddress >> 8);
	}

	if ((unsigned short) OS.memlo < startAddress)
		_heapadd(OS.memlo, startAddress - (unsigned short) OS.memlo); // recover memory below font and above lomem
	if (chio.xep80) {
		setXepCharSet(XEPCH_INTERN);
	}
	chio.utfType  = 0;
	chio.osType = get_ostype() & AT_OS_TYPE_MAIN;
	if (chio.osType >= 2) chio.keyTab = OS.keydef;
	else  {
		chio.keyTab = (unsigned char *)0xfefe;  // Into the OS, but I think there's
		if ((chio.keyTab[0] != 0x6c) || (chio.keyTab[1] != 0x6a) || (chio.keyTab[2] != 0x3B)) {
			chio.keyTab = NULL; // Weird OS?  Don't do this.
		}
	}
	OS.iocb[2].buffer = "K:";
	OS.iocb[2].buflen = strlen("K:");
	OS.iocb[2].command = IOCB_OPEN;
	OS.iocb[2].aux1 = IOCB_READBIT;
	OS.iocb[2].aux2 = 0;
	cio(2);
	iocbErrUpdate(2, &err);
	return err;
}
