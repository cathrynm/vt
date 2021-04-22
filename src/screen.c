
#include "main.h"
#include <string.h>



#define SCREENCOLUMNS 255


typedef struct {
	unsigned char bufferLen, bufferX, bufferY;
	unsigned char directDraw;
	unsigned char clearBuffer[SCREENCOLUMNS];
	unsigned char buffer[SCREENCOLUMNS];
	unsigned short lineTab[SCREENLINES];
	void (*eColonSpecial)();
	void *eColonPutByte;
}screenStruct;

screenXStruct screenX;
screenStruct screen;

void writeScreen(unsigned char *s, unsigned char len, unsigned char x, unsigned char y)
{
	static unsigned char sAtascii[4] = {0x40, 0x00, 0x20, 0x60};
	unsigned char *p, *pStart, c;
	pStart = OS.savmsc + x + screen.lineTab[y];
	if (len > screenX.screenWidth - x)len = screenX.screenWidth - x;

	for (p = pStart;len--;) {
		c = *s++;
		*p++ = sAtascii[(c & 0x60) >> 5] | (c & 0x9f);
	}
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) p)) {
		OS.oldchr = pStart[(unsigned short)OS.oldadr - (unsigned short)pStart];
	}
}


void directScrollUp(unsigned char topY, unsigned char bottomY)
{
	unsigned char y, len;
	unsigned char *p, *pStart, *pEnd, *pBottom;
	pStart = OS.savmsc + screen.lineTab[topY];
	pBottom =  OS.savmsc + screen.lineTab[bottomY];
	pEnd = pBottom + screenX.screenWidth;
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		pStart[(unsigned short)OS.oldadr - (unsigned short)pStart] = OS.oldchr;
	}
	if (bottomY > topY) {
		// memmove(pStart, &pStart[screenX.screenWidth], screen.lineTab[bottomY - topY]);
		p = pStart;
		for (y = topY;y+1 <= bottomY;y++, p += screenX.screenWidth) {
			len = (screenX.lineLength[y] > screenX.lineLength[y+1])? screenX.lineLength[y]: screenX.lineLength[y+1];
			if (len > 0)memcpy(p, &p[screenX.screenWidth], len);
		}
	}
	if (screenX.lineLength[bottomY] > 0)
		memset(pBottom, 0, screenX.lineLength[bottomY]);
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		OS.oldchr = pStart[(unsigned short)OS.oldadr - (unsigned short)pStart];
	}
}

void directScrollDown(unsigned char topY, unsigned char bottomY)
{
	unsigned char *pStart, *pEnd;
	pStart = OS.savmsc + screen.lineTab[topY];
	pEnd =  OS.savmsc + screen.lineTab[bottomY] + screenX.screenWidth;
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		pStart[(unsigned short)OS.oldadr - (unsigned short)pStart] = OS.oldchr;
	}
	if (bottomY > topY) {
		 memmove(&pStart[screenX.screenWidth], pStart, screen.lineTab[bottomY - topY]);
	}
	if (screenX.lineLength[topY] > 0)
		memset(pStart, 0, screenX.lineLength[topY]);
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		OS.oldchr = pStart[(unsigned short)OS.oldadr - (unsigned short)pStart];
	}
}

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




unsigned char directDrawTest(void)
{
	unsigned char n, y;
	static const unsigned char drawTest[]  = {CH_CLR, '0', CH_EOL, '1'};
	OS.dspflg = 0;
	callEColonPutBytes((unsigned char *)drawTest, 4);
	if (OS.savmsc[0] + 32 != '0')return 0;
	for (n = 1;n < 255;n++) {
		if (OS.savmsc[n]  + 32 == '1') {

			for (y = 0;y< SCREENLINES;y++) {
				screen.lineTab[y] = (unsigned short) y * (unsigned short) n;
			}
			return n;
		}
	}
	return 0;
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
		screen.eColonPutByte = (void *) ( (unsigned short) devhdl->put + 1);
		screen.eColonSpecial =  (void (*)(void)) ( (unsigned short) devhdl->special + 1);
		break;
	}
	if (isXep80()) {
		initXep();
	}
	screen.directDraw = directDrawTest();
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
	unsigned char logMapTouch = 0;
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
	if (x + bufferLen >= screenX.screenWidth) {
		if (y >= SCREENLINES -1) {
			bufferLen--;
			if (!bufferLen)return;  // Never draw into the very last character of the last line
		}
		logMapTouch = y + 1;
		OS.logmap[0] = OS.logmap[1] = OS.logmap[2] = OS.logmap[3] = 0; // HACK, prevents atari from wrapping
	}
	callEColonPutBytes(buffer, bufferLen);
	if (logMapTouch) {
		OS.logmap[0] = OS.logmap[1] = OS.logmap[2] =  OS.logmap[3] = 0xff; 
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

