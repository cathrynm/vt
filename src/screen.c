
#include "main.h"
#include <string.h>



#define SCREENCOLUMNS 255


typedef struct {
	unsigned char logMapTrick;
	unsigned char bufferLen, bufferX, bufferY;
	unsigned char directDraw;
	unsigned char clearBuffer[SCREENCOLUMNS];
	unsigned char buffer[SCREENCOLUMNS];
	void (*eColonSpecial)();
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


unsigned char logMapTrickTest(void)
{
	static const unsigned char drawTest[]  = {CH_CLR, CH_CURS_LEFT, ' '};
	static const unsigned char drawTest2[] = {CH_CURS_RIGHT, CH_CURS_LEFT};
	OS.dspflg = 0;
	OS.logmap[0] = 0xff - (1 << (7-1));
	callEColonPutBytes((unsigned char *)drawTest, 3);
	OS.logmap[0] = 0xff;
	callEColonPutBytes((unsigned char *)drawTest2, 2);
	return (OS.logcol == OS.lmargn);  // IF logmap trick works, then a line is not inserted here.
}

void initScreen(void)
{
	devhdl_t *devhdl;
	unsigned char n;
	for (n = 0;n < sizeof(screen.clearBuffer);n++)screen.clearBuffer[n] = ' ';
	screenX.screenWidth = OS.rmargn + 1; 
	screen.bufferLen = 0;
	screen.bufferX = 255;
	screen.bufferY = 0;
	for (n = 0;n < 11;n++) {
		if (OS.hatabs[n].id != 'E')continue;
		devhdl = OS.hatabs[n].devhdl;
		screen.eColonSpecial =  (void (*)(void)) ( (unsigned short) devhdl->special + 1);
		break;
	}
	if (isXep80()) {
		initXep();
	}
	screen.directDraw = directDrawTest();
	if (screen.directDraw) {
		initDirect();
	}
	if (!isXep80() && !screen.directDraw) {
		screen.logMapTrick = logMapTrickTest(); // Spartdos gr8 80 col doesn't do this.  Maybe it could?
	}
	drawClearScreen();
}

void screenRestore(void)
{
	flushBuffer();
	if (isXep80()) {
		restoreXep();
	}

	OS.dspflg = 0;
	callEColonPutByte(clearScreenChar);
}

void drawClearScreen(void)
{
	unsigned char y;
	cursorHide();
	flushBuffer();
	if (isXep80()) {
		clearScreenXep();
	} else {
		OS.dspflg = 0;
		callEColonPutByte(clearScreenChar);
	}
	for (y = 0;y< 24;y++) screenX.lineLength[y] = 0;
}

void cursorHide(void)
{
	if (OS.crsinh) return;
	OS.crsinh = 1;
}


void cursorUpdate(unsigned char x, unsigned char y)
{
	if (x >= screenX.screenWidth || y >= SCREENLINES) {
		cursorHide();
		return;
	}
	flushBuffer();
	if ((OS.crsinh == 0) && (OS.colcrs == x + OS.lmargn) && (OS.rowcrs == y))return;
	if (isXep80()) {
		cursorUpdateXep(x, y);
		return;
	}
	OS.crsinh = 0;
	OS.colcrs = x + OS.lmargn;
	if (OS.colcrs < OS.rmargn)OS.colcrs++;
	else OS.colcrs = OS.lmargn;
	OS.rowcrs = y;
	OS.dspflg = 0;
	callEColonPutByte( CH_CURS_LEFT);
}


void drawCharsAt(unsigned char *buffer, unsigned char bufferLen, unsigned char x, unsigned char y)
{
	unsigned char logMapTouch = 0, xp;
	if ((x >= screenX.screenWidth) || !bufferLen)return;
	if (x + bufferLen > screenX.screenWidth)bufferLen = screenX.screenWidth - x;
	if (screen.directDraw) {
		writeScreen(buffer, bufferLen, x, y);
		return;
	}
	OS.dspflg = 1;
	OS.rowcrs = y;
	OS.colcrs = x;
	if (isXep80()) {
		drawCharsAtXep(buffer, bufferLen);
		return;
	}
	if (x + bufferLen < screenX.screenWidth) {
		callEColonPutBytes(buffer, bufferLen);
		return;
	}
	if (screen.logMapTrick && (y < SCREENLINES-1)) {
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
}


void flushBuffer(void)
{
	if (!screen.bufferLen)return;
	cursorHide();
	drawCharsAt(screen.buffer, screen.bufferLen, OS.lmargn + screen.bufferX - screen.bufferLen, screen.bufferY);

	screen.bufferLen = 0;
}

void drawCharAt(unsigned char c, unsigned char attribute, unsigned char x, unsigned char y)
{
	if ((y >= SCREENLINES) || (x >= screenX.screenWidth))return;
	if (c == 0x9b)return;
	if (y != screen.bufferY || x != screen.bufferX) flushBuffer();
	if (!screen.bufferLen) {
		screen.bufferX = x;
		screen.bufferY = y;
	}
	screen.buffer[screen.bufferLen++] = c ^(attribute & 0x80);
	screen.bufferX++;
	if ((c != ' ') && (screen.bufferX > screenX.lineLength[screen.bufferY]))
		screenX.lineLength[screen.bufferY] = screen.bufferX;
}

void drawClearCharsAt(unsigned char len, unsigned char x, unsigned char y)
{
	unsigned char oldLen;
	if ((y >= SCREENLINES) || (x >= screenX.screenWidth) || (x >= screenX.lineLength[y]))return;
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

void drawClearLine(unsigned char y)
{
	drawClearCharsAt(screenX.screenWidth - OS.lmargn, 0, y);
}



void drawInsertLine(unsigned char y, unsigned char yBottom)
{
	unsigned char yp;
	if (isXep80()) {
		insertLineXep(y, yBottom);
		return;
	}
	flushBuffer();
	if (screen.directDraw) {
		directScrollDown(y, yBottom);
	} else {
		OS.dspflg = 0;
		OS.colcrs = OS.lmargn;
		if (yBottom < SCREENLINES -1) {
			cursorHide();
			OS.rowcrs = yBottom;
			callEColonPutByte(CH_DELLINE);
		}
		OS.rowcrs = y;
		callEColonPutByte(CH_INSLINE);
	}
	for (yp = yBottom;yp > y;yp--) screenX.lineLength[yp] = screenX.lineLength[yp-1];
	screenX.lineLength[y] = 0;
}

void drawDeleteLine(unsigned char y, unsigned char yBottom)
{
	unsigned char yp;
	if (isXep80()) {
		deleteLineXep(y, yBottom);
		return;
	}
	flushBuffer();
	if (screen.directDraw) {
		directScrollUp(y, yBottom);
	} else {
		OS.dspflg = 0;
		OS.rowcrs = y;
		OS.colcrs = OS.lmargn;
		callEColonPutByte(CH_DELLINE);
		if (yBottom < SCREENLINES - 1) { // Probably will blink but have no choice.
			cursorHide();
			OS.rowcrs = yBottom;
			callEColonPutByte(CH_INSLINE);
		}
	}
	for (yp = y;yp+1 <= yBottom;yp++) screenX.lineLength[yp] = screenX.lineLength[yp+1];
	screenX.lineLength[yBottom] = 0;
}

void drawInsertChar(unsigned char x, unsigned char y)
{
	if ((x >= screenX.screenWidth) || (x >= screenX.lineLength[y]))return;
	flushBuffer();
	if (isXep80()) {
		insertCharXep(x, y);
		return;
	}
	screenX.lineLength[y]++;
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;;
	callEColonPutByte(CH_INSCHR);
}

void drawDeleteChar(unsigned char x, unsigned char y)
{
	static unsigned char ch = CH_DELCHR;
	if ((x >= screenX.screenWidth) || (x >= screenX.lineLength[y]))return;
	flushBuffer();
	if (isXep80()) {
		deleteCharXep(x, y);
		return;
	}
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;
	callEColonPutByte(ch);
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

