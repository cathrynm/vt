#include "main.h"

#define VT_BEL 7
#define VT_BS 8
#define VT_HT 9
#define VT_LF 10
#define VT_VT 11
#define VT_FF 12
#define VT_CR 13
#define VT_SO 14
#define VT_SI 15
#define VT_XON 17
#define VT_XOFF 19
#define VT_CAN 24
#define VT_SUB 26
#define VT_ESC 27
#define VT_SPACE 32
#define VT_DEL 127

#define MAXPARAMS 8
#define VTSCREENLINES 24
#define MAXLINELEN 132
#define NUMPRIVATE 20


// character attributes
#define SGR_NORMAL 0
#define SGR_BOLD 1
#define SGR_UNDERLINE 4
#define SGR_BLINK 5
#define SGR_REVERSE 7
#define SGR_INVISIBLE 8
#define SGR_BOLDOFF 22
#define SGR_UNDERLINEOFF 24
#define SGR_BLINKOFF 25
#define SGR_REVERSEOFF 27
#define SGR_INVISBLEOFF 28

// Line attributes
#define LINEATTR_DECDHLT 1
#define LINEATTR_DECDHLB 2
#define LINEATTR_DECSWL 3


#define MODE_KAM 2
#define MODE_IRM 4
#define MODE_SRM 12
#define MODE_LNM 20
#define NUMMODES 21

#define MODEP_DECCKM 1
#define MODEP_DECANM 2
#define MODEP_DECCOLM 3
#define MODEP_SCLM 4
#define MODEP_DECSCNM 5
#define MODEP_DECOM 6
#define MODEP_DECAWM 7
#define MODEP_DECARM 8
#define MODEP_DECPFF 18
#define MODEP_DECPEX 19
#define MODEP_SHOWCURSOR 25
#define NUMMODEP 26
// This is, I think, secreat of VT102 emulation, I discover
// https://github.com/mattiase/wraptest

typedef struct escDataStruct escStructType;
struct escDataStruct {
	unsigned char secondChar, thirdChar, commandIndex, paramCount;
	unsigned char params[MAXPARAMS];
};

typedef struct vtScreenStruct vtScreen;
struct vtScreenStruct {
	unsigned char x, y;
	unsigned char tMargin, bMargin, rMargin;
	unsigned char attribute, attribute1, LED, keypadAlt;
	unsigned char mode[NUMMODES];
	unsigned char modeP[NUMPRIVATE];
	unsigned char charSet[2], charSetPick;
	unsigned char tabs[(MAXLINELEN+7)/8];
	unsigned char lineAttributes[VTSCREENLINES];
	unsigned char singleShift;
	unsigned char lastColumn;

	// saved cursor data
	unsigned char xSave, ySave, attributeSave, attribute1Save;
	unsigned char charsetSave[4];
	unsigned char saveWrap, saveOriginMode;
	unsigned char saveCharSetPick;
	unsigned char saveLastColumn;
};

escStructType esc = {0};
vtScreen vt = {0};

void cursorSave(void);
void clearScreen(unsigned char which);

// 0 = UP 1 = DOWN 2 = LEFT 3 = RIGHT
void vtSendCursor(unsigned char cursor)
{
	static unsigned char *cusorCodes[2][4] = {
		{
			"\033[A", "\033[B", "\033[D", "\033[C"  
		},
		{
			"\033OA", "\033OB", "\033OD", "\033OC"
		}};
	unsigned char *s = cusorCodes[vt.modeP[MODEP_DECCKM]][cursor];
	sendResponse(s, strlen(s));
}

// 0= PgUp 1=PgDown 2=Home 3=End 
void vtSendPgUpDown(unsigned char key)
{
	static unsigned char *charCodes[4] = {
		"\033[3~", "\033[6~", "\033[H", "\033[F"
	};
	unsigned char *s =  charCodes[key];
	sendResponse(s, strlen(s));
}

// Player typed Enter on Atari
void vtSendCr(void)
{
	static unsigned char crlf[2] = {13, 10}; // cc65 crazy "\012 gets converted to 0x9b
	if (vt.mode[MODE_LNM]) {
		sendResponse(crlf, 2);
	} else {
		sendResponse(&crlf[0], 1);
	}
}

void resetTabs(void)
{
	unsigned char n;
	for (n = 0;n<sizeof(vt.tabs);n++) vt.tabs[n] = 1;
}

void resetVt(void)
{
	unsigned char n;
	vt.x = 0;
	vt.y = 0;
	vt.tMargin = 0;
	vt.bMargin = VTSCREENLINES - 1;
	vt.rMargin = 79;
	vt.attribute = 0;
	vt.attribute1 = 0;
	vt.LED = 0;
	vt.charSet[0] = 'B';
	vt.charSet[1] = 'B';
	vt.charSet[2] = 'B';
	vt.charSet[3] = 'B';
	vt.charSetPick = 0;
	vt.keypadAlt = 0;
	vt.singleShift = 0;
	vt.lastColumn = 0;
	resetTabs();
	for (n = 0;n<VTSCREENLINES;n++)vt.lineAttributes[n] = 0;
	for (n = 0;n<NUMMODES;n++)vt.mode[n] = 0;
	for (n = 0;n<NUMPRIVATE;n++)vt.modeP[n] = 0;
	vt.modeP[MODEP_DECANM] = 1; 
	vt.mode[MODE_SRM] = 1; // Disable local echo
	vt.modeP[MODEP_SHOWCURSOR] = 1;
	cursorSave();
	clearScreen(2);

}

void fixCursor(void)
{
	if (vt.modeP[MODEP_SHOWCURSOR])
		cursorUpdate(vt.x, vt.y);
	else cursorHide();
}

void scrollDown(unsigned char lines)
{
	for(;lines--;) {
		drawInsertLine(vt.tMargin, vt.bMargin);
	}
}

void scrollUp(unsigned char lines)
{
	for(;lines--;) {
		drawDeleteLine(vt.tMargin, vt.bMargin);
	}
}




void setMode(unsigned char mode, unsigned char val)
{
	vt.mode[mode] = val;
	switch(mode) {
		default:break;
	}
}


void setModeP(unsigned char mode, unsigned char val)
{
	vt.modeP[mode] = val;
	switch(mode) {
		case MODEP_DECCOLM: // 0 = 80 column, 1 = 132 column
			vt.lastColumn = 0;
			if (val) vt.rMargin = 131;
			else vt.rMargin = 79;
			break;
		case MODEP_DECAWM:
			vt.lastColumn = 0;
			break;
		case MODEP_DECSCNM:
			drawDarkLight(val);
			break;
		case MODEP_DECOM:
			vt.lastColumn = 0;
			if (val) vt.y = 0;
			else vt.y = vt.tMargin;
			vt.x = 0;
			break;

		default:break;
	}
}

void moveUp(unsigned char num, unsigned char noScroll)
{
	if (!num)num = 1;
	vt.lastColumn = 0;
	if (vt.y >= num + vt.tMargin) {
		vt.y -= num;
		return;
	}
	if (!noScroll)scrollDown(num + vt.tMargin - vt.y);
	vt.y = vt.tMargin;
}

void moveDown(unsigned char num, unsigned char noScroll)
{
	if (!num)num = 1;
	vt.lastColumn = 0;
	if (vt.y + num  <= vt.bMargin) {
		vt.y += num;
		return;
	}
	if (!noScroll)scrollUp(vt.y + num - vt.bMargin);
	vt.y = vt.bMargin;
}

void moveLeft(unsigned char num)
{
	vt.lastColumn = 0;
	if (!num)num = 1;
	if (vt.x >= num) vt.x -= num;
	else vt.x = 0;
}

void moveRight(unsigned char num)
{
	vt.lastColumn = 0;
	if (!num)num = 1;
	if (vt.x + num <= vt.rMargin) vt.x += num;
	else vt.x = vt.rMargin;
}


void cursorPosition(unsigned char x, unsigned char y)
{
	unsigned char topY;
	vt.lastColumn = 0;
	if (!x) x = 1;
	if (!y) y = 1;
	x--;y--;
	if (vt.modeP[MODEP_DECOM]) {
		y += vt.tMargin;
		topY = vt.bMargin;
	} else topY = VTSCREENLINES-1;
	if (y > topY) y = topY;
	if ( x> vt.rMargin)x = vt.rMargin;
	vt.x = x;
	vt.y = y;
}


void eraseCharacters(unsigned char num)
{
	vt.lastColumn = 0;
	if (!num)num++;
	if (vt.x <= vt.rMargin)	drawClearCharsAt(num <= (vt.rMargin + 1 - vt.x)? num : vt.rMargin + 1 - vt.x , vt.x, vt.y);
}

void clearScreen(unsigned char which)
{
	unsigned char y;
	vt.lastColumn = 0;
	switch(which) {
		case 0:
			if (vt.x <= vt.rMargin)	drawClearCharsAt( vt.rMargin + 1 - vt.x, vt.x, vt.y);
			for (y = vt.y+1;y<VTSCREENLINES;y++) drawClearLine(y);
			break;
		case 1:
			for (y = 0;y< vt.y;y++) drawClearLine(y);
			drawClearCharsAt( vt.x + 1, 0, vt.y);
			break;
		case 2:
			drawClearScreen();
			break;
	}
}

void clearLine(unsigned char which)
{
	vt.lastColumn = 0;
	switch(which) {
		case 0:
			if (vt.x <= vt.rMargin)	drawClearCharsAt( vt.rMargin + 1 - vt.x, vt.x, vt.y);
			break;
		case 1:
			drawClearCharsAt( vt.x + 1, 0, vt.y);
			break;
		case 2:
			drawClearLine(vt.y);
			break;
	}
}

void insertLines(unsigned char num)
{
	if (!num)num++;
	for (;num--;)drawInsertLine(vt.y, vt.bMargin);
}

void deleteLines(unsigned char num)
{
	if (!num)num++;
	for (;num--;)drawDeleteLine(vt.y, vt.bMargin);
}

void deleteCharacters(unsigned char num)
{
	vt.lastColumn = 0;
	if (!num)num++;
	for (;num--;)drawDeleteChar(vt.x, vt.y);
}

unsigned char charToA(unsigned char v, unsigned char *s, unsigned char m)
{
	unsigned char n;
	unsigned char dig[3] = {100, 10, 1};
	for ( n = 0;n<3;n++) {
		if (v < dig[n] && n< 2)continue;
		s[m] = '0';
		while (v >= dig[n]) {
			v -= dig[n];
			s[m]++;
		}
		m++;
	}
	return m;
}

void reportStatus(void)
{
	static unsigned char reportChar[4] = "\033[0n";
	sendResponse(reportChar, 4);
}

void reportCursor(void)
{
	unsigned char off;
	static unsigned char reportChar[10] = "\033[";
	off = charToA(vt.y+1  - (vt.modeP[MODEP_DECOM]? vt.tMargin: 0), reportChar, 2);
	reportChar[off++] = ';';
	off = charToA(vt.x+1, reportChar, off);
	reportChar[off++] = 'R';
	sendResponse(reportChar, off);
}

void reportTerminal(void)
{
	static unsigned char reportTerminal[5] = "\033[?6c"; // are we ever not okay?
	sendResponse(reportTerminal, 5);
}

void reportTerminalType(void)
{
	static unsigned char reportTerminal[4] = "\033[0n"; // are we ever not okay?
	sendResponse(reportTerminal, 4);
}

unsigned char serialToVt(unsigned short speed)
{
	switch(speed) {
		case 50:return 0;
		case 75:return 8;
		case 110:return 16;
		case 134:return 24;
		case 150:return 32;
		case 200: return 40;
		case 300: return 48;
		case 600: return 56;
		case 1200: return 64;
		case 1800:return 72;
		case 2000:return 80;
		case 2400:return 88;
		case 3600:return 96;
		case 4800:return 104;
		case 9600:return 112;
		case 19200:return 120;
		default:return 0;
	}
}

unsigned bitsToVt(unsigned char bits)
{
	switch(bits) {
		case 8:return 1;
		case 7: return 2;
		default:return 0;
	}
}

unsigned parityToVt(unsigned char parity)
{
	switch(parity) {
		case RXLAT_NOCHGPAR:return 1;
		case RXLAT_SETPARODD:return 4;
		case RXLAT_SETPAREVEN: return 5;
		case RXLAT_SETPARITY1: return 0;
		default:return 0;
	}
}

void reportTerminalParams(unsigned char mode) 
{
	unsigned char off;
	static unsigned char reportTerminal[32] = "\033[";
	off = charToA(mode, reportTerminal, 2);
	reportTerminal[off++] = ';';
	off = charToA(parityToVt(getParity()), reportTerminal, off); // parity
	reportTerminal[off++] = ';';
	off = charToA(bitsToVt(getBits()), reportTerminal, off); // bits
	reportTerminal[off++] = ';';
	off = charToA(serialToVt(getBaud()), reportTerminal, off); // xspd
	reportTerminal[off++] = ';';
	off = charToA(serialToVt(getBaud()), reportTerminal, off); // rspd
	reportTerminal[off++] = ';';
	off = charToA(1, reportTerminal, off); // cmul
	reportTerminal[off++] = ';';
	off = charToA(15, reportTerminal, off); // flags
	reportTerminal[off++] = 'x';
	sendResponse(reportTerminal, off);
}

void setAttributes(void)
{
	unsigned char n;
	for (n = 0;n< esc.paramCount;n++) {
		if (esc.params[n] == 0){
			vt.attribute = 0;
			vt.attribute1 = 0;
		} else if (esc.params[n] < 8)vt.attribute |= (1 << esc.params[n]);
		else if (esc.params[n] < 16) vt.attribute1 |= (1 << (esc.params[n] - 8));
		else switch(esc.params[n]) {
			case SGR_BOLDOFF:
				vt.attribute &= ~(1 << SGR_BOLD);
				break;
			case SGR_UNDERLINEOFF:
				vt.attribute &= ~(1 << SGR_UNDERLINE);
				break;
			case SGR_BLINKOFF:
				vt.attribute &= ~(1 << SGR_BLINK);
				break;
			case SGR_REVERSEOFF:
				vt.attribute &= ~(1 << SGR_REVERSE);
				break;
			case SGR_INVISBLEOFF:
				vt.attribute1 &= ~(1 << (SGR_INVISIBLE - 8));
				break;
			default:
				break;
		}
	}
}

void setLED(unsigned char status)
{
	if (status < 2)vt.LED = status;
}

void setVMargins(unsigned char top, unsigned char bottom)
{
	vt.lastColumn = 0;
	if (!top)top++;
	if (!bottom)bottom = VTSCREENLINES - 1;
	if (bottom <= top)return;
	bottom--;top--;
	if (bottom >= VTSCREENLINES) return;
	vt.tMargin = top;
	vt.bMargin = bottom;
	vt.x = 0;
	vt.y = vt.modeP[MODEP_DECOM]? vt.tMargin : 0;
}

void cursorSave(void)
{
	vt.xSave = vt.x;
	vt.ySave = vt.y;
	vt.attributeSave = vt.attribute;
	vt.attribute1Save = vt.attribute1;
	vt.charsetSave[0] = vt.charSet[0];
	vt.charsetSave[1] = vt.charSet[1];
	vt.saveWrap = vt.modeP[MODEP_DECAWM];
	vt.saveOriginMode = vt.modeP[MODEP_DECOM];
	vt.saveCharSetPick = vt.charSetPick;
	vt.saveLastColumn = vt.lastColumn;
}

void cursorRestore(void)
{
	vt.x = vt.xSave;
	vt.y = vt.ySave;
	vt.attribute = vt.attributeSave;
	vt.attribute1 = vt.attribute1Save;
	vt.charSet[0] = vt.charsetSave[0];
	vt.charSet[1] = vt.charsetSave[1];
	vt.modeP[MODEP_DECAWM] = vt.saveWrap;
	vt.modeP[MODEP_DECOM] = vt.saveOriginMode;
	vt.charSetPick = vt.saveCharSetPick;
	vt.lastColumn = vt.saveLastColumn;
}

void fillWithE(void)
{
	unsigned char y, x;
	vt.x = 0;
	vt.y = 0;
	vt.tMargin = 0;
	vt.bMargin = VTSCREENLINES - 1;
	for (y =0;y<VTSCREENLINES;y++) {
		for (x = 0;x<vt.rMargin;x++)drawCharAt('E', vt.attribute, x, y);
	}
}

unsigned short charToUtf(unsigned char c)
{
	static const unsigned short specialGraphics[126-95+1] = { // conversion of special graphics to UTF8
		0x00a0,
		0x25C6,0x2592,0x2409,0x240C, 0x240D,0x240A,0x00B0,0x00B1,
		0x2424,0x240B,0x2518,0x2510, 0x250C,0x2514,0x253C,0x23BA,
		0x23BB,0x2500,0x23BC,0x23BD, 0x251C,0x2524,0x2534,0x252C,
		0x2502,0x2264,0x2265,0x03C0, 0x2260,0x00A3,0x00B7};
	unsigned char set;
	if (vt.singleShift){
		set = vt.singleShift;
		vt.singleShift = 0;
	} else set = vt.charSet[vt.charSetPick];
	if ((set == 'A') && (c == '#')) return 0x00a3; // Pound symbol
	else if ((set == '0') && (c >= 95) && (c <= 126)) return specialGraphics[c - 95];
	return c; // B 1, 2


}

void insertCharacters(unsigned char num)
{
	vt.lastColumn = 0;
	if (!num)num++;
	for(;num--;)drawInsertChar(vt.x, vt.y);
}

void addChar(unsigned char c, unsigned char attrib, unsigned char attrib1)
{
	if (vt.mode[MODE_IRM]) drawInsertChar(vt.x, vt.y);
	if ((vt.x == vt.rMargin) && vt.lastColumn) {
		if (vt.modeP[MODEP_DECAWM]) {
			vt.lastColumn = 0;
			vt.x = 0;
			if (vt.y+1 < vt.bMargin)vt.y++;
			else scrollUp(1);
		} else vt.x = vt.rMargin;
	}
	if (attrib1 & (1 << (SGR_INVISIBLE-8)))c = ' ';
	drawCharAt(c, attrib, vt.x, vt.y);
	if (vt.x < vt.rMargin) {
		vt.x++;
		vt.lastColumn = 0;
	} else vt.lastColumn = 1;
}

void cancelEscape(void)
{
	vt.lastColumn = 0;
	if (esc.commandIndex) {
		esc.commandIndex = 0;
		addChar(ERRCHAR, ERRATTRIB, 0);
	}
}

void tabClear(unsigned char mode)
{
	unsigned char n;
	if (mode == 0)vt.tabs[vt.x>>3] &= ~(1 << (vt.x & 7));
	else if (mode == 3){
		for (n = 0;n<sizeof(vt.tabs);n++)vt.tabs[n] = 0;
	}
}

void tabSet(void)
{
	vt.tabs[vt.x>>3] |= (1 << (vt.x & 7));
}

void nextTab(void)
{
	vt.lastColumn = 0;
	if (vt.x >= vt.rMargin) return;
	for (;;) {
		vt.x++;
		if (vt.x >= vt.rMargin)return;
		if (vt.tabs[vt.x>>3] & (1 << (vt.x & 7)))break;
	}
}

void vt52CursorPosition(unsigned char x, unsigned char y)
{
	if (x > vt.rMargin)return;
	if (y >= VTSCREENLINES)return;
	vt.x = x;
	vt.y = y;
}

void processQuestion(unsigned char c)
{
	switch(c) {
		case 'h':
			if ((esc.params[0] < NUMMODEP) && !vt.modeP[esc.params[0]]) {
				setModeP(esc.params[0], 1);
			}
			break;
		case 'i': // printer stuff
			switch(esc.params[0]) {
					case 4: // auto print off
						break;
					case 5: // auto print on
						break;
			}
			break;
		case 'l': // rest mode
			if ((esc.params[0] < NUMMODEP) && vt.modeP[esc.params[0]]) {
				setModeP(esc.params[0], 0);
			}
			break;
		case 'n': // printer stuff
			switch(esc.params[0]) {
				case 13: // printer status
					break;
			}
			break;

		default:
			break;
	}
}

void processCommand(unsigned char c)
{
	switch(c) {
		case '@':
			insertCharacters(esc.params[0]);
			break;
		case 'A': // UP
			moveUp(esc.params[0], 1);
			break;
		case 'B': // DOWN
			moveDown(esc.params[0], 1);
			break;
		case 'C': // RIGHT
			moveRight(esc.params[0]);
			break;
		case 'D': // left
			moveLeft(esc.params[0]);
			break;
		case 'E':
			moveDown(esc.params[0], 1);
			vt.x = 0;
			break;
		case 'F':
			moveUp(esc.params[0], 1);
			vt.x = 0;
			break;
		case 'G':
			cursorPosition(esc.params[0], vt.y);
			break;

		case 'f':
		case 'H': // x, y cursor position
			cursorPosition(esc.params[1], esc.params[0]);
			break;
		case 'J': // clear screen
			clearScreen(esc.params[0]);
			break;
		case 'K': // clear line  
			clearLine(esc.params[0]);
			break;
		case 'L': // insert lines
			insertLines(esc.params[0]);
			break;
		case 'M': // Delete lines
			deleteLines(esc.params[0]);
			break;
		case 'P': // Delete character.
			deleteCharacters(esc.params[0]);
			break;
		case 'X':
			eraseCharacters(esc.params[0]);
			break;
		case 'c': 
			reportTerminal();
			break; // \033[6c for VT102
		case 'g':
			tabClear(esc.params[0]);
			break;
		case 'h': // set mode  20 = linefeed newline mode // 4 = insert mode  4l = replace mode.
			if ((esc.params[0] < NUMMODES) && !vt.mode[esc.params[0]]) {
				setMode(esc.params[0], 1);
			}
			break;
		case 'i': // printer 
			break;
		case 'l': // reset mode
			if (( esc.params[0] < NUMMODES) && vt.mode[esc.params[0]]) {
				setMode(esc.params[0], 0);
			}
			break;
		case 'm': // turn on character attributes.  0 = all off 1=bold, 2 = low intensity, 4 = underline 5= blinking 7 = reverse 8 = invisible
			setAttributes();
			break;
		case 'n': // device status
			switch(esc.params[0]) {
				case 5: // terminal status
					reportStatus();
					break;
				case 6: //
					reportCursor();
					break;
			}
			break;
		case 'q': // LED colors
			setLED(esc.params[0]);
			break;
		case 's': // ansi.sys
			cursorSave();
			break;
		case 'r': // set top and bottom margins
			setVMargins(esc.params[0], esc.params[1]);
			break;
		case 'u': // ansi.sys
			cursorRestore();
			break;
		case 'x':
			reportTerminalParams(esc.params[0]);
			break;

		case 'y': // confidence test
			break;
		default:
			break;
	}
}

void processCharSet(unsigned char second, unsigned char c)
{
	unsigned char set;
	switch(second) {
		case '(':set = 0;break;
		case ')':set = 1;break;
		case '*':set = 2;break;
		case '+':set = 3;break;
		default:return;
	}
	switch(c) {
		case 'A':
		case 'B':
		case '0':
		case '1':
		case '2':
			vt.charSet[set] = c;
			break;
		default:
			break;
	}
}


void processHash(unsigned char c)
{
	switch(c) {
		case '3': // double-height top half (double height double wide)
			vt.lastColumn = 0;
			vt.lineAttributes[vt.y] = LINEATTR_DECDHLT;
			break;
		case '4': // double-height bottom half
			vt.lastColumn = 0;
			vt.lineAttributes[vt.y] = LINEATTR_DECDHLT;
			break;
		case '5': // single width  signle height
			vt.lastColumn = 0;
			vt.lineAttributes[vt.y] = 0;
			break;
		case '6': // double width single height
			vt.lastColumn = 0;
			vt.lineAttributes[vt.y] = LINEATTR_DECSWL;
			break;
		case '8':
			fillWithE();
			break;
		default:
			break;
	}
}

void processOther(unsigned char c)
{
	switch(c) {
		case 'c': // reset terminal
			resetVt();
			break;
		case 'D': // Down one line. Scroll up if hits bottom
			moveDown(1, 0);
			break;

		case 'E': // first colunn of next line down.  Scroll up.
			moveDown(1, 0);
			vt.x = 0;
			break;
		case 'H': // set horizontal tab stop
			tabSet();
			break;
		case 'M':  //cursor up one line.
			moveUp(1, 0);
			break;
		case 'N':
			vt.singleShift = vt.charSet[2];
			break;
		case 'O':
			vt.singleShift = vt.charSet[3];
			break;
		case 'Z': // identify terminal // Response Esc/Z
			reportTerminal();
			break; 
		case '7': // save cursor position graphic rendition and charset
			cursorSave();
			break;
		case '8': // restore cursor position
			cursorRestore();
			break; 
		case '>': // keypad sends ascii codes.
			vt.keypadAlt = 0;
			break;
		case '=':
			vt.keypadAlt = 1;
			break;
		default:break;
	}
}




void processVt52Command(unsigned char c)
{
	switch(c) {
		case 'A':
			moveUp(1, 0);
			break;
		case 'B':
			moveDown(1, 0);
			break;
		case 'C':
			moveRight(1);
			break;
		case 'D':
			moveLeft(1);
			break;
		case 'E':
			break;
		case 'H':
			cursorPosition(1, 1);
			break;
		case 'I':
			moveUp(1, 0);
			break;
		case 'J':
			clearScreen(0);
			break;
		case 'K':
			clearLine(0); // to end of line
			break;

		case 'Y': // cursor Position
			esc.commandIndex++; // Get the positions next
			return;
		case 'Z':
			sendResponse("\033/Z", 3);
			break;
		case '<':
			vt.modeP[MODEP_DECANM] = 1; // Exit vt52 mode.
			break;
		default:
			break;
	}
	esc.commandIndex = 0;
}

// characters outside of 0-127 are sent here, to be drawn.  They're already translated to be drawable here. 
void displayUtf8Char(unsigned char c, unsigned char attribute)
{
	cursorHide();
	addChar(c, vt.attribute ^ attribute, vt.attribute1);
	esc.commandIndex = 0;
}

void debugVt(unsigned char c)
{
#if 1
	c = c;
#else
	static unsigned char debugBuffer[256];
	static unsigned char debugIndex = 0;
	OS.stack[0]  = (unsigned char) debugBuffer;
	OS.stack[1] = (unsigned char) (((unsigned short) debugBuffer) >> 8);
	OS.stack[2] = debugIndex;
	debugBuffer[debugIndex++] = c;
#endif
}

void processChar(unsigned char c) {
	unsigned short wCh;
	unsigned char ch, attrib;
	unsigned short paramVal;
	debugVt(c);
	cursorHide();
	if (c < VT_SPACE)switch(c) {
		case VT_BEL:
			drawBell();
			break;
		case VT_BS:
			moveLeft(1);
			break;
		case VT_HT:
			nextTab();
			break;
		case VT_LF:
		case VT_VT:
		case VT_FF:
			moveDown(1, 0); // and auto print
			if (vt.mode[MODE_LNM]) vt.x = 0;
			break;
		case VT_CR:
			vt.lastColumn = 0;
			vt.x = 0;
			break;
		case VT_SO:
			vt.charSetPick = 1;
			break;
		case VT_SI:
			vt.charSetPick = 0;
			break;
		case VT_XON:
			break;
		case VT_XOFF:
			break;
		case VT_CAN:
		case VT_SUB:
			cancelEscape();
			break;
		case VT_ESC:
			esc.paramCount = 1;
			esc.params[0] = 0;
			esc.params[1] = 0;
			esc.commandIndex = 1;
			esc.secondChar = 0;
			esc.thirdChar = 0;
			break;
		default: break;
    }
    else if ((c >= VT_SPACE) && (c < VT_DEL)) {
		if (!vt.modeP[MODEP_DECANM]) {
			if (esc.commandIndex == 1) {
				processVt52Command(c);
			} else if (esc.commandIndex == 2) {
				esc.secondChar = c;
				vt52CursorPosition(esc.secondChar - 32, vt.y);
				esc.commandIndex = 0;
			} else {
				wCh = charToUtf(c);
				if (!(wCh & 0xff80)) {
					ch = (unsigned char) wCh;attrib = 0;
					convertAsciiToVisibleChar(&ch, &attrib);
				} else convertShortToVisibleChar(wCh, &ch, &attrib);
				addChar(ch, vt.attribute ^ attrib, vt.attribute1);
			}
		} else {
			if (esc.commandIndex) {
				if ((esc.commandIndex == 1) && (c == '[' || c == '(' || c == ')' || c == '#' || c == '*' || c == '+')) {
					esc.secondChar = c;
					esc.commandIndex++;
				} else if ((esc.secondChar == '[') && isdigit(c)) {
					if (esc.paramCount - 1 < MAXPARAMS) {
						paramVal = (unsigned short)esc.params[esc.paramCount - 1] * (unsigned short)10 + (c - (unsigned char)'0');
						esc.params[esc.paramCount - 1] = paramVal < 255? paramVal: 255;
					}
					esc.commandIndex++;
				} else if ((esc.secondChar == '[') && (c==';')) {
					if (esc.paramCount < MAXPARAMS) {
						esc.paramCount++;
						if (esc.paramCount < MAXPARAMS) {
							esc.params[esc.paramCount] = 0;
						}
					}
					esc.commandIndex++;
				} else if (esc.secondChar == '[' && (esc.commandIndex == 2) && (c == '?')) {
					esc.thirdChar = '?';
					esc.commandIndex++;
				} else {
					if ((esc.secondChar == '[') && (esc.thirdChar == '?'))processQuestion(c);
					else if (esc.secondChar == '[')processCommand(c);
					else if (esc.secondChar == '(' || esc.secondChar == ')' ||  esc.secondChar == '*' || esc.secondChar == '+' )processCharSet(esc.secondChar, c);
					else if (esc.secondChar == '#')processHash(c);
					else processOther(c);
					esc.commandIndex = 0;
				}
			} else {
				wCh = charToUtf(c);
				if (!(wCh & 0xff80)) {
					ch = (unsigned char) wCh;attrib = 0;
					convertAsciiToVisibleChar(&ch, &attrib);
				} else convertShortToVisibleChar(wCh, &ch, &attrib);
				addChar(ch, vt.attribute ^ attrib, vt.attribute1);
			}
		}
	}

}

void processChars(unsigned char *s, unsigned char len)
{
	for(;len--;)processChar(*s++);
}

void sendResponse(unsigned char *s, unsigned char len)
{
	if (!vt.mode[MODE_SRM])
		processChars(s, len);
	sendSerialResponse(s, len);
}