
#include "main.h"


typedef struct chioDataStruct chioStruct;
struct chioDataStruct {
	unsigned char utfType;
	unsigned char utfIndex;
	unsigned short utfWord;
	unsigned long utfLong; // Super slow on 6502, but these are rare, so whatever.
	unsigned char *keyTab;
	void *vprced;
	unsigned char pactl;
	unsigned char interruptOn;
	unsigned char fullAscii;
};

chioStruct chio;
unsigned char *eColon = "E:";
unsigned char clearScreenChar = CH_CLR;


void drawChar(unsigned char ch)
{
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void drawString(unsigned char *s)
{
	for (;*s;s++)drawChar(*s);
}

unsigned char isIntl(void)
{
	return (detect.chbas == 204);
}

unsigned char closeChio(void)
{
	unsigned char err = ERR_NONE;
	closeInterrupt();
	OS.iocb[2].command = IOCB_CLOSE;
	cio(2);
	iocbErrUpdate(2, &err);
	return err;
}
// https://en.wikipedia.org/wiki/ATASCII
// Note, only need to check ones that go past 16 bits here.  These all get processed as drawable, not controls.
void convertLongToVisibleChar(unsigned long c, unsigned char *ch, unsigned char *attrib)
{
	switch(screenX.charSet) {
#if XEPINTERNALFONT
		case 'X':
			switch(c) {
				default:
					break;
			}
			break;
#endif
#if ATARIINTERNATIONAL
		case 'I':
			switch(c) {
				case 0x1F8B0:
					if (!chio.fullAscii) {
						*ch = 0x7d;
						*attrib = 0;
						return;
					}
					break;
				default:
					break;
			}
			break;
#endif
		case 'A':
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
					if (!chio.fullAscii) {
						*ch = 0x7d;
						*attrib = 0;
						return;
					}
					break;
				default:
					break;
			}
			break;
	}
	*ch = ERRCHAR;
	*attrib = ERRATTRIB;
}

void convertAsciiToVisibleChar(unsigned char *ch, unsigned char *attrib)
{
	if ((*ch >= 32 && *ch <=95) || (*ch >= 97 && *ch <= 122) || (*ch == 124))return;
	if (chio.fullAscii && ((*ch == 96) || (*ch == 123) || (*ch == 125) || (*ch == 126)))return;
	if (*ch == 127) {
		*ch = ' ';
		return;
	}
	*attrib = ERRATTRIB;  // Atari are missing { } ` ~.  Show as undrawable.  127, 0-31 are undrawable anyway.
	*ch = ERRCHAR;
}


// XEP80Internal
#define XEPICH_CIJ 0
#define XEPICH_UPARROW 1
#define XEPICH_0SLASH 2
#define XEPICH_POUND 3
#define XEPICH_XTHING 4
#define XEPICH_OTHERQUOTE 5
#define XEPICH_SMALLPLUS 6
#define XEPICH_SQUIGGLE 7
#define XEPICH_CTAIL 8
#define XEPICH_CNTILDA 9
#define XEPICH_CAE 10
#define XEPICH_CADOTS 11
#define XEPICH_CODOTS 12
#define XEPICH_CADOT 13
#define XEPICH_CUDOTS 14
#define XEPICH_COE 15
#define XEPICH_LIJ 16
#define XEPICH_BETA 17
#define XEPICH_LADOTLEFT 18
#define XEPICH_LEDOTLEFT 19
#define XEPICH_LIDOTLEFT 20
#define XEPICH_LIDOTS 21
#define XEPICH_LUDOTLEFT 22
#define XEPICH_LEDOTRIGHT 23
#define XEPICH_LCTAIL 24
#define XEPICH_LNTILDA 25
#define XEPICH_LAE 26
#define XEPICH_LADOTS 27
#define XEPICH_LODOTS 28
#define XEPICH_LADOT 29
#define XEPICH_LUDOTS 30
#define XEPICH_LOE 31

void convertShortToVisibleChar(unsigned short c, unsigned char *ch, unsigned char *attrib)
{
	*attrib = 0;
	switch(screenX.charSet) {
#if XEPINTERNALFONT
		case 'X':
			switch(c) {
				case 0x0132:
		            *ch = XEPICH_CIJ;
		            return;
		        case 0x21e7:
		            *ch = XEPICH_UPARROW;
		            return;
		        case 0x00f8:
		            *ch = XEPICH_0SLASH;
		            return;
		        case 0x00a3:
		            *ch = XEPICH_POUND;
					return;
				case 0x29bb:  // what is this strange character?  #4 of XEP80 internal set. Calling it X over circle.
				    *ch = XEPICH_XTHING;
					return;
				case 0x00a8: // diaeresis?
		            *ch = XEPICH_OTHERQUOTE;
					return;
				case 0x00b0: // degree symbol
		            *ch = XEPICH_SMALLPLUS;
					return;
				case 0x00a7:
		            *ch = XEPICH_SQUIGGLE;
					return;
				case 0x00c7:
		            *ch = XEPICH_CTAIL;
					return;
				case 0x00D1:
		            *ch = XEPICH_CNTILDA;
					return;
				case 0x00c6:
		            *ch = XEPICH_CAE;
					return;
				case 0x00c4:
		            *ch = XEPICH_CADOTS;
					return;
				case 0x00d6:
		            *ch = XEPICH_CODOTS;
					return;
				case 0x00c5:
		            *ch = XEPICH_CADOT;
					return;
				case 0x00DC:
		            *ch = XEPICH_CUDOTS;
					return;
				case 0x0152:
		            *ch = XEPICH_COE;
					return;
				case 0x0133:
		            *ch = XEPICH_LIJ;
					return;
				case 0x00df:
		            *ch = XEPICH_BETA;
					return;
				case 0x00e0:
		            *ch = XEPICH_LADOTLEFT;
					return;
				case 0x00e8:
		            *ch = XEPICH_LEDOTLEFT;
					return;
				case 0x00ec:
		            *ch = XEPICH_LIDOTLEFT;
					return;
				case 0x00ef:
		            *ch = XEPICH_LIDOTS;
					return;
				case 0x00f9:
		            *ch = XEPICH_LUDOTLEFT;
					return;
				case 0x00e9:
		            *ch = XEPICH_LEDOTRIGHT;
					return;
				case 0x00e7:
		            *ch = XEPICH_LCTAIL;
					return;
				case 0x00f1:
		            *ch = XEPICH_LNTILDA;
					return;
				case 0x00e6:
		            *ch = XEPICH_LAE;
					return;
				case 0x00e4:
		            *ch = XEPICH_LADOTS;
					return;
				case 0x00f6:
		            *ch = XEPICH_LODOTS;
					return;
				case 0x00e5:
		            *ch = XEPICH_LADOT;
					return;
				case 0x00fc:
		            *ch = XEPICH_LUDOTS;
					return;
				case 0x0153:
		            *ch = XEPICH_LOE;
		            return;
				default:break;
			}
			break;
#endif
#if ATARIINTERNATIONAL
		case 'I':	
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
					if (!chio.fullAscii) {
						*ch = 0x60;
						return;
					}
					break;
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
					if (!chio.fullAscii) {
						*ch = 0x7e;
						return;
					}
					break;
				case 0x25b6:
					if (!chio.fullAscii) {
						*ch = 0x7f;
						return;
					}
					break;
				default:
					break;
			}
			break;
#endif
		case 'A':
			switch(c) {
				case 0x00a0:
					*ch = ' ';
					return;

				case 0x2665: // cntrl characters
					*ch = 0;
					return;
				case 0x251C: // VBar+Right
				case 0x255e:
				case 0x255f:
				case 0x2560:
					*ch = 1;
					return;
				case 0x255d:
				case 0x255c:
				case 0x255b:
				case 0x2518: // up left corner
					*ch = 3;
					return;
				case 0x2561:
				case 0x2562:
				case 0x2563:
				case 0x2524: // VBar + left
					*ch = 4;
					return;
				case 0x2556:
				case 0x2555:
				case 0x2557:
				case 0x2510: // down-left corner
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
				case 0x2552:
				case 0x2553:
				case 0x2554:
				case 0x250C: // down right corner
					*ch = 0x11;
					return;
				case 0x2550: // double horizontal bar
				case 0x2500: // horizontal bar
					*ch = 0x12;
					return;
				case 0x253C:
				case 0x256c:
				case 0x256b:
				case 0x256a:
					*ch = 0x13; // cross
					return;
				case 0x2022:
					*ch = 0x14;
					return;
				case 0x2584:
					*ch = 0x15;
					return;
				case 0x258E:
					*ch = 0x16; // vbar left
					return;
				case 0x252C: // HBAR Down
				case 0x2566:
				case 0x2564:
				case 0x2565:
					*ch = 0x17;
					return;
				case 0x2534: // hbar UP
				case 0x2569:
				case 0x2567:
				case 0x2568:
					*ch = 0x18;
					return;
				case 0x258C:
					*ch = 0x19;
					return;
				case 0x255a:
				case 0x2558:
				case 0x2559:
				case 0x2514: // up right corner
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
					if (!chio.fullAscii) {
						*ch = 0x60;
						return;
					}
					break;
				case 0x2660: //spade
					if (!chio.fullAscii) {
						*ch = 0x7b;
						return;
					}
					break;
				case 0x2551:
				case 0x2502:
					*ch = 0x7c; // vertical bar
					return;
				case 0x25c0:
					if (!chio.fullAscii) {
						*ch = 0x7e;
						return;
					}
					break;
				case 0x25b6:
					if (!chio.fullAscii) {
						*ch = 0x7f;
						return;
					}
					break;
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
				case 0x2591:
				case 0x2592:
				case 0x2593:
					if (chio.fullAscii) {
						*ch = 0x7f;
					} else {
						*ch = ' ';
						*attrib = 0x80;
					}
					return;
				default:
					break;
			}
			break;
	}
	*ch = ERRCHAR;
	*attrib = ERRATTRIB;
}
void debugCh(unsigned char c)
{
#if 1
	c = c;
#else
	static unsigned char stop = 0;
	static unsigned char debugBuffer[256];
	static unsigned char debugIndex = 0;
	if (stop)return;
//	OS.stack[0]  = (unsigned char) debugBuffer;
//	OS.stack[1] = (unsigned char) (((unsigned short) debugBuffer) >> 8);
//	OS.stack[2] = debugIndex;
	if (c == 'e') {
		if ((debugBuffer[(debugIndex-1) & 0xff] == 'n') &&
			(debugBuffer[(debugIndex-2) & 0xff] == 'o') &&
			(debugBuffer[(debugIndex-3) & 0xff] == 'h') &&
			(debugBuffer[(debugIndex-4) & 0xff] == 'P')) {
			stop = 1;
		}
	}
	debugBuffer[debugIndex++] = c;

#endif
}

void decodeUtf8Char(unsigned char c, unsigned char *err)
{
	unsigned char ch, attrib;
	debugCh(c);
	switch(chio.utfType) { // e2 95 90
		case 0:
			if (!(c & 0x80))processChar(c, err);
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
			}
			break;
		case 1:
			if ((c & 0xc0) == 0x80) {
				chio.utfWord |= (c & 0x3f);
				convertShortToVisibleChar(chio.utfWord, &ch, &attrib);
				displayUtf8Char(ch, attrib);
			}
			chio.utfType = 0;
			break;
		case 2:
			if ((c & 0xc0) == 0x80) {
				c &= 0x3f;
				if (chio.utfIndex == 1) {
					chio.utfWord |= ((unsigned short)c) << 6;
					chio.utfIndex++;
				} else {
					chio.utfWord |= ((unsigned short)c);
					convertShortToVisibleChar(chio.utfWord, &ch, &attrib);
					displayUtf8Char(ch, attrib);
					chio.utfType = 0;
				}
			} else {
				chio.utfType = 0; // invalid utf8
			}
			break;
		case 3:
			if ((c & 0xc0) == 0x80) {
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
			} else {
				chio.utfType = 0; // invalid utf8
			}
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
	unsigned char vc;
	if (detect.osType < 2 || !OS.noclik) {
		for (n = 0;n<63*2;n+=2) {
			GTIA_WRITE.consol = n & 0x8;
			vc = ANTIC.vcount;
			while (vc == ANTIC.vcount);
		}
	}
}

unsigned char isKeyReady(void) { // handle cases where OS.ch != 0xff, but then getchar will still block.
	unsigned char ch = OS.ch, code;
	if (ch >= 0xff)return 0; // NO key pressed
	if (ch < 0xc0) {
		if (!chio.keyTab) return 1;
		code = chio.keyTab[ch];
		if (detect.osType >= 2 && code >= AKEY_F1 && code <= AKEY_F4)
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
			if (detect.osType >= 2) OS.noclik ^= 0xff;
			break;
		default:
			break;
	} 
	click();
	return 0;
}

unsigned char anyKey(void) {
	return OS.ch != 0xff;
}

unsigned char extraKey(unsigned char *extra)
{
	unsigned char ch = OS.ch, c;
	*extra = 0;
	if ((ch == 0xff) || !chio.keyTab)return 0;
	c = 0;
	switch(ch & 0xc0) {
		case 0xc0:
			switch(chio.keyTab[ch & 0x3f]) {
				case ',':
					*extra = '{'; // These overlap with atari keys
					break;
				case '.':
					*extra = '}'; 
					break;
				case '-':
					c = CH_CURS_UP; // Page Up
					OS.superf = 1;
					break;
				case '=':
					c = CH_CURS_DOWN; // Page Down
					OS.superf = 1;
					break;
				case '<':
					c = CH_CURS_LEFT; // Home
					OS.superf = 1;
					break;
				case '>':
					c = CH_CURS_RIGHT; // End
					OS.superf = 1;
					break;
				case '/':
					*extra = '\037';
					break;
				default:
					break;
			}
			break;
		case 0x40:
			switch(chio.keyTab[ch & 0x3f]) {
				case '\033': // Shift Esc for ~
					*extra = '~';
					break;
				default:
					break;
			}
			break;
		case 0x80: // control
			switch(chio.keyTab[ch & 0x3f]) {
				case '\033': // ctrl Esc for `.
					*extra = '`';
					break;
				default:
					break;
			}
			break;
		case 0:
			break;
	}
	if (!*extra && !c)return 0;
	OS.holdch = ch;
	OS.ch = 0xff;
	OS.atachr = c;
	click(); // Swallow up the key here.
	return c;
}

void handleInput(unsigned char *err)
{
	unsigned char extra;
	static unsigned char cr = 0x9b, bs = 8, del = 0x7f, null = 0, tab = 9;
	unsigned char superF, ch, shift = OS.ch & 0xc0;
	ch = extraKey(&extra); // Check for any special keys first. 
	if (!ch && !extra) {
		if (!isKeyReady())return;
		ch = getChar(err);
		if (*err != ERR_NONE)return;
	}
	superF = OS.superf;
	OS.superf = 0;
	if (extra) {
		sendResponse(&extra, 1, err);
	} else switch(ch) {
		case CH_CURS_UP:
		case CH_CURS_DOWN:
		case CH_CURS_LEFT:
		case CH_CURS_RIGHT:
			if (superF) {
				vtSendPgUpDown(ch - CH_CURS_UP, err);
			} else {
				vtSendCursor(ch - CH_CURS_UP, err);
			}
			break;
		case CH_DEL:
			sendResponse(&bs, 1, err);
			break;
		case CH_INSCHR:{
			sendResponse("\033[@", 3, err);
			break;
		}
		case CH_DELCHR:{
			sendResponse("\033[P", 3, err);
			break;
		}
		case CH_DELLINE:
			sendResponse("\033[M", 3, err);
			break;
		case CH_INSLINE:
			sendResponse("\033[L", 3, err);
			break;
		case CH_EOL:
			vtSendCr(err);
			break;
		case CH_TAB:
			sendResponse(&tab, 1, err);
			break;
		default: {
			// Control characters go through, also 
			if (((ch >= 1) && (ch <= 26)) || (ch >= 32 && ch <=95) || (ch >= 97 && ch <= 122) || (ch == 124) || (ch == CH_ESC)) {
				sendResponse(&ch, 1, err);
				break;
			}
			break;
		}
	}
}


static unsigned char lCurly[8] =  {0x07, 0x18, 0x18, 0x70, 0x18, 0x18, 0x07, 0x00};
static unsigned char rCurly[8] =  {0x70, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x70, 0x00};
static unsigned char tilda[8] =   {0x00, 0x3d, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char bapost[8] =  {0x30, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00};
static unsigned char hashBox[8] = {0x00, 0x54, 0x2a, 0x54, 0x2a, 0x54, 0x2a, 0x00};

void memCopy(unsigned char *top, const unsigned char *fromp, unsigned short len)
{
	while(len--)*top++ = *fromp++;
}

void memClear(unsigned char *top, unsigned short len)
{
	while(len--)*top++ = 0;
}


void copyChar(unsigned char *top, unsigned char to, unsigned char *from, void(*copy)(unsigned char, unsigned char *))
{
	if (copy)(copy)(to, from);
	else memcpy(top + ((unsigned short)to << 3), from, 8);
}

void initAscii(unsigned char fontBase, void(*copy)(unsigned char, unsigned char *)) {
	unsigned char *top  = (unsigned char *)(fontBase << 8);
	unsigned char *fromP = (unsigned char *) (detect.chbas <<8);
	unsigned char n;
	for (n = 0;n<128;n++) {
		copyChar(top, n, &fromP[n<<3], copy);
	}
	copyChar(top, 96, bapost, copy);
	copyChar(top, 123, lCurly, copy);
	copyChar(top, 125, rCurly, copy);
	copyChar(top, 126, tilda, copy);
	copyChar(top, 127, hashBox, copy);
	setFullAscii(1);
}

void setFullAscii(unsigned char fullAscii)
{
	chio.fullAscii = fullAscii;
}

unsigned char initChio(void) // Don't use malloc from here.
{
	unsigned char err = ERR_NONE;
	memset((unsigned char *)&chio, 0, sizeof(chio));
	screenX.charSet = (detect.chbas == 204)? 'I': 'A';
	chio.utfType  = 0;
	chio.fullAscii = 0;
	if (detect.osType >= 2) chio.keyTab = OS.keydef;
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

unsigned char errUpdate(unsigned char err, unsigned char *oldErr)
{
	if (((*oldErr == ERR_NONE) || (*oldErr == ERR_ENDOFFILE)) && (err != ERR_NONE))*oldErr = err;
	return *oldErr;
}

unsigned char iocbErrUpdate(unsigned char iocb, unsigned char *oldErr)
{
	return errUpdate(OS.iocb[iocb].status, oldErr);
}

unsigned char getline(unsigned char *buf, unsigned char len, unsigned char *err)
{
	OS.iocb[0].buffer=buf;
	OS.iocb[0].buflen=len;
	OS.iocb[0].command=IOCB_GETREC;
	cio(0);
	iocbErrUpdate(0, err);
	if (*err != ERR_NONE) return 0;
	return OS.iocb[0].buflen;
}


void enableInterrupt(void)
{
	if (chio.interruptOn)return;
	chio.vprced = OS.vprced;
	chio.pactl = PIA.pactl;
	trip = 0;
	PIA.pactl  &= (~1);
	OS.vprced   = ih;            // Set PROCEED interrupt vector to our interrupt handler.
	PIA.pactl  |= 1;             // Indicate to PIA we are ready for PROCEED interrupt.
	chio.interruptOn = 1;
}

void closeInterrupt(void)
{
	if (chio.interruptOn) {
		chio.interruptOn = 0;
		PIA.pactl = chio.pactl;
		OS.vprced = chio.vprced;
	}
}

unsigned char proceedReady(void)
{
	if (!chio.interruptOn)return 1;
	return trip;
}

void doneProceeed(void)
{
	if (!chio.interruptOn)return;
	trip = 0;
	PIA.pactl |= 1;
}