
#include "main.h"
#include <string.h>



#define SCREENCOLUMNS 80
#if VBXE_ON
#define ATTRIBS 2
#else
#define ATTRIBS 1
#endif

typedef struct {
	unsigned char bufferLen, bufferX, bufferY, bufferLineLen;
	unsigned char clearBuffer[SCREENCOLUMNS*ATTRIBS];
	unsigned char buffer[SCREENCOLUMNS*ATTRIBS];
	void (*eColonSpecial)();
	unsigned char altScreen;
}screenStruct;

screenXStruct screenX;
screenStruct screen;

unsigned char sAtascii[4] = {0x40, 0x00, 0x20, 0x60};



void callEColonSpecial(unsigned char command, unsigned char aux1, unsigned char aux2)
{
	if (!screen.eColonSpecial) {
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = 2;
		OS.iocb[0].aux1 = aux1;
		OS.iocb[0].aux2 = aux2;
		OS.iocb[0].command = command;
		cio(0);
	} else {
		OS.ziocb.command = command;
		OS.ziocb.aux1 = aux1;
		OS.ziocb.aux2 = aux2;
		screen.eColonSpecial();
	}
}

void callEColonPutByte(unsigned char ch)
{
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void callEColonPutBytes(unsigned char *buf, unsigned char len)
{
	OS.iocb[0].buffer = buf;
	OS.iocb[0].buflen = len;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void initScreen(void)
{
	devhdl_t *devhdl;
	unsigned char n;
	for (n = 0;n < SCREENCOLUMNS;n++) {
		if (!detect.hasColor)screen.clearBuffer[n] = ' ';
		else {
			screen.clearBuffer[n<<1] = ' ';
			screen.clearBuffer[(n<<1) + 1] = DEFAULTCOLOR;
		}
	}
	screenX.screenWidth = OS.rmargn + 1; 
	screen.bufferLen = 0;
	screen.bufferLineLen = 0;
	screen.bufferX = 255;
	screen.bufferY = 0;
	screen.altScreen = 0;
	for (n = 0;n < 11;n++) {
		if (OS.hatabs[n].id != 'E')continue;
		devhdl = OS.hatabs[n].devhdl;
		screen.eColonSpecial =  (void (*)(void)) ( (unsigned short) devhdl->special + 1);
		break;
	}
	switch(detect.videoMode) {
#if XEP_ON
		case 'X':
			initXep();
			break;
#endif
#if DIRECT_ON
		case 'D':
			initDirect();
			break;
#endif
#if VBXE_ON
		case 'V':
			initVbxe();
			break;
#endif
#if RAWCON_ON
		case 'R':
			initRawCon();
			break;
#endif
		default: {
			break;
		}
	}
	drawClearScreen(0);
}

void screenRestore(void)
{
	flushBuffer();
	switch(detect.videoMode) {
#if XEP_ON
		case 'X':
			restoreXep();
			break;
#endif
#if DIRECT_ON
		case 'D':
			restoreDirect();
			break;
#endif
#if VBXE_ON
		case 'V':
			restoreVbxe();
			break;
#endif
		default:break;
	}
	OS.dspflg = 0;
	callEColonPutByte(clearScreenChar);
}

void drawClearScreen(unsigned char color)
{
	unsigned char y;
	cursorHide();
	flushBuffer();

	switch(detect.videoMode) {
#if XEP_ON
		case 'X':
			clearScreenXep();
			break;
#endif
#if VBXE_ON
		case 'V': {
			clearScreenVbxe(color);
			break;
		}
#endif
#if CIO_ON || DIRECT_ON || RAWCON_ON
		default:
			OS.dspflg = 0;
			callEColonPutByte(clearScreenChar);
			break;
#endif
	}
	for (y = 0;y< 24;y++) screenX.lineLength[y] = 0;
}

void bumpCursor(void)
{
	if (OS.colcrs < OS.rmargn)OS.colcrs++;
	else OS.colcrs = OS.lmargn;
	OS.dspflg = 0;
	callEColonPutByte(CH_CURS_LEFT);
}

void cursorHide(void)
{
	if (OS.crsinh) return;
	OS.crsinh = 1;
	switch(detect.videoMode) {
#if VBXE_ON
		case 'V':
			cursorHideVbxe();
			break;
#endif
		default:
			break;
	}
}

void cursorUpdate(unsigned char x, unsigned char y)
{
	if (x >= screenX.screenWidth || y >= SCREENLINES) {
		cursorHide();
		return;
	}
	flushBuffer();
	screenX.cursX = x + OS.lmargn; screenX.cursY = y;
	if ((OS.crsinh == 0) && (OS.colcrs == x + OS.lmargn) && (OS.rowcrs == y))return;
	OS.crsinh = 0;
	OS.colcrs = x + OS.lmargn;
	switch(detect.videoMode) {
#if XEP_ON
		case 'X':
			cursorUpdateXep(x, y);
			break;
#endif
#if VBXE_ON
		case 'V':
			cursorUpdateVbxe(x, y);
			break;
#endif
#if CIO_ON || DIRECT_ON || RAWCON_ON
		default:
			OS.rowcrs = y;
			bumpCursor();
			break;
#endif
	}
}


void drawCharsAt(unsigned char *buffer, unsigned char bufferLen, unsigned char x, unsigned char y)
{
	unsigned char logMapTouch = 0;
#if CIO_ON
	unsigned char xp;
#endif
	if ((x >= screenX.screenWidth) || !bufferLen)return;
	if (x + bufferLen > screenX.screenWidth)bufferLen = screenX.screenWidth - x;
	OS.dspflg = 1;
	OS.rowcrs = y;
	OS.colcrs = x;
	switch(detect.videoMode) {
#if DIRECT_ON
		case 'D':
			drawCharsAtDirect(buffer, bufferLen);
			break;
#endif
#if XEP_ON
		case 'X':
			drawCharsAtXep(buffer, bufferLen);
			break;
#endif
#if VBXE_ON
		case 'V':
			drawCharsAtVbxe(buffer, bufferLen);
			break;
#endif
#if RAWCON_ON
		case 'R':
			drawCharsAtRawCon(buffer, bufferLen);
			break;
#endif
#if CIO_ON
		case 'A':
		case 'G':
			if (x + bufferLen < screenX.screenWidth) {
				callEColonPutBytes(buffer, bufferLen);
			} else if ((detect.videoMode == 'A') && (y < SCREENLINES-1)) {
				logMapTouch = y + 1;
				OS.logmap[logMapTouch >> 3] &= ~(1 << (7-( logMapTouch & 7))); // Fake out OS, tell it next line is continuation, just for this. 
				callEColonPutBytes(buffer, bufferLen);
				OS.logmap[logMapTouch >>3] = 0xff;
			} else {
				if (bufferLen > 2) {
					callEColonPutBytes(buffer, bufferLen - 2);
				}
				if (bufferLen >= 2) { // Goofy way to draw in last column, but maybe works?
					xp = x + bufferLen - 2;
					cursorHide();
					OS.colcrs = xp;
					OS.dspflg = 0;
					callEColonPutByte(CH_DELCHR);
					OS.colcrs = xp;
					OS.dspflg = 1;
					callEColonPutByte(buffer[bufferLen-1]);
					OS.colcrs = xp;
					OS.dspflg = 0;
					callEColonPutByte(CH_INSCHR);
					OS.colcrs = x + bufferLen - 2;
					OS.dspflg = 1;
					callEColonPutByte(buffer[bufferLen-2]);
				} else {
					// I have one character to draw on the right edge, and Atari makes this so hard.
				}
			}
			break;
#endif
	}
}

void flushBuffer(void)
{
	if (!screen.bufferLen)return;
	cursorHide();
	drawCharsAt(screen.buffer, screen.bufferLen, OS.lmargn + screen.bufferX - screen.bufferLen, screen.bufferY);
	screen.bufferLen = 0;
	screenX.lineLength[screen.bufferY] = screen.bufferLineLen;
}

void drawCharAt(unsigned char c, unsigned char attribute, unsigned char color, unsigned char x, unsigned char y)
{
	if ((y >= SCREENLINES) || (x >= screenX.screenWidth - OS.lmargn) || (c == CH_EOL))return;
	if (y != screen.bufferY || x != screen.bufferX || (screen.bufferLen >= SCREENCOLUMNS)) flushBuffer();
	if (!screen.bufferLen) {
		screen.bufferX = x;
		screen.bufferY = y;
		screen.bufferLineLen = screenX.lineLength[y];
	}
	if (!detect.hasColor) {
		screen.buffer[screen.bufferLen] = c ^(attribute & 0x80);
	} else {
		screen.buffer[screen.bufferLen << 1] = c ^(attribute & 0x80);
		screen.buffer[(screen.bufferLen << 1) + 1] = color;
	}
	screen.bufferLen++;
	screen.bufferX++;
	if (((c != ' ') || color) && (screen.bufferX > screen.bufferLineLen))
		screen.bufferLineLen = screen.bufferX;
}

void drawClearCharsAt(unsigned char len, unsigned char x, unsigned char y, unsigned char color)
{
	unsigned char lineLength;
	if ((y >= SCREENLINES) || (x >= screenX.screenWidth - OS.lmargn))return;
	if (!detect.hasColor)color = 0;
	if (x + len > screenX.screenWidth - OS.lmargn)len = screenX.screenWidth - OS.lmargn - x;
	if (color) {
		for (;len--;)drawCharAt(' ', 0, color, x++, y);
		return;
	}
	flushBuffer();
	lineLength = screenX.lineLength[y];
	if (x >= lineLength)return;
	cursorHide();
	if (len >= lineLength - x) { // Clear to end of line.
		len = lineLength - x;
		lineLength = x;
	}
	drawCharsAt(screen.clearBuffer, len, x + OS.lmargn, y);
	screenX.lineLength[y] = lineLength; // UPdate this here, in case lower level draw code needs original line length
}

void drawClearLine(unsigned char y, unsigned char color)
{
	drawClearCharsAt(screenX.screenWidth - OS.lmargn, 0, y, color);
}

void drawInsertLine(unsigned char y, unsigned char yBottom, unsigned char color)
{
	unsigned char yp;
	flushBuffer();
	switch(detect.videoMode) {
#if XEP_ON
		case 'X':
			insertLineXep(y, yBottom);
			break;
#endif
#if DIRECT_ON
		case 'D':
			insertLineDirect(y, yBottom);
			break;
#endif
#if VBXE_ON
		case 'V':
			insertLineVbxe(y, yBottom, color);
			break;
#endif
#if RAWCON_ON
		case 'R':
			insertLineRawCon(y, yBottom);
			break;
#endif
#if CIO_ON
		case 'A':
		case 'G':
			OS.dspflg = 0;
			OS.colcrs = OS.lmargn;
			if (yBottom < SCREENLINES -1) {
				cursorHide();
				OS.rowcrs = yBottom;
				callEColonPutByte(CH_DELLINE);
			}
			OS.rowcrs = y;
			callEColonPutByte(CH_INSLINE);
			break;
#endif
	}
	for (yp = yBottom;yp > y;yp--) screenX.lineLength[yp] = screenX.lineLength[yp-1];
	screenX.lineLength[y] = 0;
}

void drawDeleteLine(unsigned char y, unsigned char yBottom, unsigned char color)
{
	unsigned char yp;
	flushBuffer();
	switch (detect.videoMode) {
#if XEP_ON
		case 'X':
			deleteLineXep(y, yBottom);
			break;
#endif
#if DIRECT_ON
		case 'D':
			deleteLineDirect(y, yBottom);
			break;
#endif
#if VBXE_ON
		case 'V':
			deleteLineVbxe(y, yBottom, color);
			break;
#endif
#if RAWCON_ON
		case 'R':
			deleteLineRawCon(y, yBottom);
			break;
#endif
#if CIO_ON
		default:
			OS.dspflg = 0;
			OS.rowcrs = y;
			OS.colcrs = OS.lmargn;
			callEColonPutByte(CH_DELLINE);
			if (yBottom < SCREENLINES - 1) { // Probably will blink but have no choice.
				cursorHide();
				OS.rowcrs = yBottom;
				callEColonPutByte(CH_INSLINE);
			}
			break;
#endif
	}
	for (yp = y;yp+1 <= yBottom;yp++) screenX.lineLength[yp] = screenX.lineLength[yp+1];
	screenX.lineLength[yBottom] = 0;
}

void drawInsertChar(unsigned char x, unsigned char y, unsigned char color)
{
	flushBuffer();
	if (x >= screenX.lineLength[y])return;
	switch(detect.videoMode) {
#if XEP_ON
		case 'X':
			insertCharXep(x + OS.lmargn, y);
			break;
#endif
#if VBXE_ON
		case 'V':
			insertCharVbxe(x + OS.lmargn, y, color);
			break;
#endif
#if CIO_ON || DIRECT_ON || RAWCON_ON
		default:
			OS.dspflg = 0;
			OS.rowcrs = y;
			if (screenX.lineLength[y] > OS.rmargn - OS.lmargn) {
				cursorHide();
				OS.colcrs = OS.rmargn;
				callEColonPutByte(CH_DELCHR);
			}
			OS.colcrs = OS.lmargn + x;;
			callEColonPutByte(CH_INSCHR);
			break;
#endif
	}
	if (screenX.lineLength[y] < screenX.screenWidth - OS.lmargn)screenX.lineLength[y]++;
}

void drawDeleteChar(unsigned char x, unsigned char y, unsigned char color)
{
	flushBuffer();
	if (x >= screenX.lineLength[y])return;
	switch(detect.videoMode) {
#if XEP_ON
		case 'X':
			deleteCharXep(x + OS.lmargn, y);
			break;
#endif
#if VBXE_ON
		case 'V':
			deleteCharVbxe(x + OS.lmargn, y, screenX.lineLength[y] - x, color);
			break;
#endif
#if CIO_ON || DIRECT_ON || RAWCON_ON
		default:
			OS.dspflg = 0;
			OS.rowcrs = y;
			OS.colcrs = OS.lmargn + x;
			callEColonPutByte(CH_DELCHR);
			break;
#endif
	}
	screenX.lineLength[y]--;
}

void drawBell(void)
{
}

// 1 = black on white.  0 = white on black
void drawDarkLight(unsigned char val)  // XEP 80 does this. 
{
	val = val;
}

void drawAltScreen(unsigned char alt, unsigned char clear) // 1 for alt, 0 for main.
{
	switch(detect.videoMode) {
		default:
			if ((screen.altScreen != alt) || clear) {
				drawClearScreen(0);  // VBXE/Direct, maybe can do altscreen proper.
				screen.altScreen = alt;
			}
			break;
	}
}