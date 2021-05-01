
#include "main.h"
#include <string.h>



#define SCREENCOLUMNS 80


typedef struct {
	unsigned char bufferLen, bufferX, bufferY;
	unsigned char clearBuffer[SCREENCOLUMNS*2];
	unsigned char buffer[SCREENCOLUMNS*2];
	void (*eColonSpecial)();
	unsigned char altScreen;
	unsigned char cursX, cursY;
}screenStruct;

screenXStruct screenX;
screenStruct screen;



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
		case 'X':
			initXep();
			break;
		case 'D':
			initDirect();
			break;
		case 'V':
			initVbxe();
			break;
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
		case 'X':
			restoreXep();
			break;
		case 'D':
			restoreDirect();
			break;
		case 'V':
			restoreVbxe();
			break;
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
		case 'X':
			clearScreenXep();
			break;
		case 'V': {
			clearScreenVbxe(color);
			break;
		}
		default:
			OS.dspflg = 0;
			callEColonPutByte(clearScreenChar);
			break;
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
		case 'V':
			cursorHideVbxe();
			break;
		case 'R':
			bumpCursor();
			break;
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
	screen.cursX = x; screen.cursY = y;
	if ((OS.crsinh == 0) && (OS.colcrs == x + OS.lmargn) && (OS.rowcrs == y))return;
	OS.crsinh = 0;
	OS.colcrs = x + OS.lmargn;
	switch(detect.videoMode) {
		case 'X':
			cursorUpdateXep(x, y);
			break;
		case 'V':
			cursorUpdateVbxe(x, y);
			break;
		default:
			OS.rowcrs = y;
			bumpCursor();
			break;
	}
}


void drawCharsAt(unsigned char *buffer, unsigned char bufferLen, unsigned char x, unsigned char y)
{
	unsigned char logMapTouch = 0, xp;
	if ((x >= screenX.screenWidth) || !bufferLen)return;
	if (x + bufferLen > screenX.screenWidth)bufferLen = screenX.screenWidth - x;
	OS.dspflg = 1;
	OS.rowcrs = y;
	OS.colcrs = x;
	switch(detect.videoMode) {
		case 'D':
			drawCharsAtDirect(buffer, bufferLen);
			break;
		case 'X':
			drawCharsAtXep(buffer, bufferLen);
			break;
		case 'V':
			drawCharsAtVbxe(buffer, bufferLen);
			break;
		case 'R':
			drawCharsAtRawCon(buffer, bufferLen);
			if ((screen.cursX >= x) && (screen.cursX < x + bufferLen) && (y == screen.cursY)) {
				OS.oldchr = buffer[screen.cursX - x];
			}
			break;
		default:
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
	}
}


void flushBuffer(void)
{
	if (!screen.bufferLen)return;
	cursorHide();
	drawCharsAt(screen.buffer, screen.bufferLen, OS.lmargn + screen.bufferX - screen.bufferLen, screen.bufferY);
	screen.bufferLen = 0;
}

void drawCharAt(unsigned char c, unsigned char attribute, unsigned char color, unsigned char x, unsigned char y)
{
	if ((y >= SCREENLINES) || (x >= screenX.screenWidth))return;
	if (c == 0x9b)return;
	if (y != screen.bufferY || x != screen.bufferX) flushBuffer();
	if (!screen.bufferLen) {
		screen.bufferX = x;
		screen.bufferY = y;
	}
	if (!detect.hasColor) {
		screen.buffer[screen.bufferLen] = c ^(attribute & 0x80);
	} else {
		screen.buffer[screen.bufferLen << 1] = c ^(attribute & 0x80);
		screen.buffer[(screen.bufferLen << 1) + 1] = color;
	}
	screen.bufferLen++;
	screen.bufferX++;
	if ((c != ' ') && (screen.bufferX > screenX.lineLength[screen.bufferY]))
		screenX.lineLength[screen.bufferY] = screen.bufferX;
}

void drawColorClearCharsAt(unsigned char len, unsigned char x, unsigned char y, unsigned char color) {
	for (;len--;)drawCharAt(' ', 0, color, x++, y);
}

void drawClearCharsAt(unsigned char len, unsigned char x, unsigned char y, unsigned char color)
{
	unsigned char oldLen;
	if ((y >= SCREENLINES) || (x >= screenX.screenWidth))return;
	if (!detect.hasColor)color = 0;

	if (color) {
		drawColorClearCharsAt(len, x, y, color);
		return;
	}
	if (x >= screenX.lineLength[y])return;
	cursorHide();
	flushBuffer();
	oldLen = screenX.lineLength[y];
	if (len >= screenX.screenWidth - x) {
		len = screenX.screenWidth - x;
		if (x < screenX.lineLength[y])screenX.lineLength[y] = x;
	}
	if (len > oldLen - x) len = oldLen - x;
	drawCharsAt(screen.clearBuffer, len, x + OS.lmargn, y);
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
		case 'X':
			insertLineXep(y, yBottom);
			break;
		case 'D':
			insertLineDirect(y, yBottom);
			break;
		case 'V':
			insertLineVbxe(y, yBottom, color);
			break;
		default:
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
	}
	for (yp = yBottom;yp > y;yp--) screenX.lineLength[yp] = screenX.lineLength[yp-1];
	screenX.lineLength[y] = 0;
}

void drawDeleteLine(unsigned char y, unsigned char yBottom, unsigned char color)
{
	unsigned char yp;
	flushBuffer();
	switch (detect.videoMode) {
		case 'X':
			deleteLineXep(y, yBottom);
			break;
		case 'D':
			deleteLineDirect(y, yBottom);
			break;
		case 'V':
			deleteLineVbxe(y, yBottom, color);
			break;
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
	}
	for (yp = y;yp+1 <= yBottom;yp++) screenX.lineLength[yp] = screenX.lineLength[yp+1];
	screenX.lineLength[yBottom] = 0;
}

void drawInsertChar(unsigned char x, unsigned char y, unsigned char color)
{
	if (x >= screenX.lineLength[y])return;
	flushBuffer();
	switch(detect.videoMode) {
		case 'X':
			insertCharXep(x, y);
			break;
		case 'V':
			insertCharVbxe(x, y, screenX.lineLength[y] - x, color);
			break;
		default:
			OS.dspflg = 0;
			OS.rowcrs = y;
			OS.colcrs = OS.lmargn + x;;
			callEColonPutByte(CH_INSCHR);
			break;
	}
	screenX.lineLength[y]++;
}

void drawDeleteChar(unsigned char x, unsigned char y, unsigned char color)
{
	if (x >= screenX.lineLength[y])return;
	flushBuffer();
	switch(detect.videoMode) {
		case 'X':
			deleteCharXep(x, y);
			break;
		case 'V':
			deleteCharVbxe(x, y, screenX.lineLength[y] - x, color);
			break;
		default:
			OS.dspflg = 0;
			OS.rowcrs = y;
			OS.colcrs = OS.lmargn + x;
			callEColonPutByte(CH_DELCHR);
			break;
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