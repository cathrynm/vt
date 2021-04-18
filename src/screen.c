
#include "main.h"
#include <string.h>

#define XEPLINES 25
#define SCREENLINES 24
#define SCREENCOLUMNS 255
#define XEPRMARGIN 81 // Two extra for delete character. 
#define XEPATR_INVERSE 0x1
#define XEPATR_BLINK 0x4
#define XEPATR_DOUBLEWIDE 0x10
#define XEPATR_UNDERLINE 0x20
#define XEPATR_BLANK 0x40
#define XEPATR_GRAPHICS 0x80

typedef struct screenDataStruct screenStruct;
struct screenDataStruct {
	unsigned char screenWidth;
	unsigned char lastCharX, lastCharY, bufferLen, bufferX, bufferY;
	unsigned char directDraw, origRMargn;
	unsigned char xepCharset;
	unsigned char burst;
	unsigned char xepX, xepY; // xepX, xepY shadow cached value of colcrs, rowcrs in xep80 driver.
	unsigned char fillFlag;
	unsigned char clearBuffer[SCREENCOLUMNS];
	unsigned char buffer[SCREENCOLUMNS];
	unsigned short lineTab[SCREENLINES];
	unsigned char xepLines[XEPLINES];
	unsigned char lineLength[XEPLINES];
};

screenStruct screen;

void writeScreen(unsigned char *s, unsigned char len, unsigned char x, unsigned char y)
{
	static unsigned char sAtascii[4] = {0x40, 0x00, 0x20, 0x60};
	unsigned char *p, *pStart, c;
	pStart = OS.savmsc + x + screen.lineTab[y];
	if (len > screen.screenWidth - x)len = screen.screenWidth - x;

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
	pEnd = pBottom + screen.screenWidth;
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		pStart[(unsigned short)OS.oldadr - (unsigned short)pStart] = OS.oldchr;
	}
	if (bottomY > topY) {
		// memmove(pStart, &pStart[screen.screenWidth], screen.lineTab[bottomY - topY]);
		p = pStart;
		for (y = topY;y+1 <= bottomY;y++, p += screen.screenWidth) {
			len = (screen.lineLength[y] > screen.lineLength[y+1])? screen.lineLength[y]: screen.lineLength[y+1];
			if (len > 0)memcpy(p, &p[screen.screenWidth], len);
		}
	}
	if (screen.lineLength[bottomY] > 0)
		memset(pBottom, 0, screen.lineLength[bottomY]);
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		OS.oldchr = pStart[(unsigned short)OS.oldadr - (unsigned short)pStart];
	}
}

void directScrollDown(unsigned char topY, unsigned char bottomY)
{
	unsigned char *pStart, *pEnd;
	pStart = OS.savmsc + screen.lineTab[topY];
	pEnd =  OS.savmsc + screen.lineTab[bottomY] + screen.screenWidth;
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		pStart[(unsigned short)OS.oldadr - (unsigned short)pStart] = OS.oldchr;
	}
	if (bottomY > topY) {
		 memmove(&pStart[screen.screenWidth], pStart, screen.lineTab[bottomY - topY]);
	}
	if (screen.lineLength[topY] > 0)
		memset(pStart, 0, screen.lineLength[topY]);
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		OS.oldchr = pStart[(unsigned short)OS.oldadr - (unsigned short)pStart];
	}
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

void setBurstMode(unsigned char on)
{
	if (screen.burst == on)return;
	OS.iocb[0].buffer = NULL;
	OS.iocb[0].buflen = 0;
	OS.iocb[0].command = 0x15;
	OS.iocb[0].aux1 = 0xc;
	OS.iocb[0].aux2 = on;
	cio(0);
	screen.burst = on;
}


unsigned char directDrawTest(void)
{
	unsigned char n, y;
	static const unsigned char drawTest[]  = {CH_CLR, '0', CH_EOL, '1'};
	OS.dspflg = 0;
	OS.iocb[0].buffer = drawTest;
	OS.iocb[0].buflen = 4;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
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

void setXEPRMargin(unsigned char x) {
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = 2;
	if ((x >= 0x40) && (x < 0x50)) {
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = XEP_SETRIGHTMARGINLO | (x-0x40); // Set to low 4 bits
		OS.iocb[0].command = 20;
		cio(0);
	} else {
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = XEP_SETRIGHTMARGINLO | (x & 0xf); // Set to low 4 bits
		OS.iocb[0].command = 20;
		cio(0);
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = XEP_SETRIGHTMARGINHI + (x >> 4);
		OS.iocb[0].command = 20;
		cio(0);
	}
}

void setXEPXPos(unsigned char hpos) {
	if (hpos < 80) {
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = 2;
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = XEP_SETCURSORHPOS | hpos; // Set to low 4 bits
		OS.iocb[0].command = 20;
		cio(0);
	} else {
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = 2;
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = XEP_SETCURSORHPOS | (hpos & 0xf); // Set to low 4 bits
		OS.iocb[0].command = 20;
		cio(0);
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = 2;
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = XEP_SETCURSORHPOSHI + (hpos >> 4);
		OS.iocb[0].command = 20;
		cio(0);
	}
}

void setXEPYPos(unsigned char y) {
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = 2;
	OS.iocb[0].aux1 = 12;
	OS.iocb[0].aux2 = XEP_SETCURSORVPOS + y;
	OS.iocb[0].command = 20;
	cio(0);
}

unsigned char isXep80Internal(void) {
	return screen.xepCharset == XEPCH_INTERN;
}

unsigned char setXepCharSet(unsigned char which)
{
	unsigned char err = ERR_NONE;
	if (!isXep80() || (which == screen.xepCharset))return err;
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = strlen(eColon);
	OS.iocb[0].aux1 = 12;
	OS.iocb[0].aux2 = which;
	OS.iocb[0].command = 20;
	cio(0);
	iocbErrUpdate(0, &err);
	if (err == ERR_NONE) {
		screen.xepCharset = which;
		setFullAscii((which == XEPCH_INTERN)); 
	}
	return err;
}

void initScreen(void)
{
	unsigned char n;
	for (n = 0;n < sizeof(screen.clearBuffer);n++)screen.clearBuffer[n] = ' ';
	screen.screenWidth = OS.rmargn + 1; 
	screen.bufferLen = 0;
	screen.bufferX = 255;
	screen.bufferY = 0;
	screen.burst = 0;
	screen.origRMargn = OS.rmargn;
	if (isXep80()) {
		setBurstMode(1);
		OS.colcrs = 0;
		setXEPXPos(OS.colcrs);
		OS.rowcrs = OS.lmargn;
		setXEPYPos(OS.rowcrs);
		OS.rmargn = XEPRMARGIN;
		setXEPRMargin(OS.rmargn);
		screen.screenWidth = XEPRMARGIN - 1;
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = 2;
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = XEP_CURSORON;
		OS.iocb[0].command = 20;
		cio(0);
	}
	screen.directDraw = directDrawTest();
	drawClearScreen();
}

void screenRestore(void)
{
	flushBuffer();
	if (isXep80()) {
		setXEPXPos(OS.colcrs);
		setXEPYPos(OS.rowcrs);
		setBurstMode(0);
		setXepCharSet(isIntl()? XEPCH_ATINT: XEPCH_ATASCII);
		OS.rmargn = screen.origRMargn;
	}
	OS.dspflg = 0;
	OS.iocb[0].buffer = &clearScreenChar;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}


void setXEPLastChar(unsigned char c)
{ // 80 = cr
	OS.dspflg = 1;
	OS.rowcrs = XEPLINES-1;
	OS.colcrs = XEPRMARGIN+1;
	screen.xepX = OS.colcrs;screen.xepY = OS.rowcrs;
	OS.iocb[0].buffer = &c;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void setXEPCommand(unsigned char c, unsigned char command)
{
	setXEPLastChar(c);
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = 2;
	OS.iocb[0].aux1 = 12;
	OS.iocb[0].aux2 = command;
	OS.iocb[0].command = 20;
	cio(0);
}

void setXEPExtraByte(unsigned char c)
{
	setXEPCommand(c, XEP_SETEXTRABYTE);
}


void drawXEPCharAt(unsigned char c, unsigned char x, unsigned char y)
{
	setXEPLastChar(c);
	setXEPXPos(x);
	setXEPYPos(y);
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = 2;
	OS.iocb[0].aux1 = 12;
	OS.iocb[0].aux2 = XEP_WRITEBYTE;
	OS.iocb[0].command = 20;
	cio(0);
	if (x != OS.colcrs) setXEPXPos(OS.colcrs);
	if (y != OS.rowcrs) setXEPYPos(OS.rowcrs);
}

void setXepRowPtr(unsigned char y, unsigned char val) {
	setXEPExtraByte(y + 0x20);
	setXEPLastChar(val);
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = 2;
	OS.iocb[0].aux1 = 12;
	OS.iocb[0].aux2 = XEP_WRITEINTERNALBYTE;
	OS.iocb[0].command = 20;
	cio(0);
	screen.xepLines[y] = val;
}

void drawClearScreen(void)
{
	unsigned char y;
	cursorHide();
	flushBuffer();
	if (isXep80()) {
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = strlen(eColon);
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = isXep80Internal()? XEP_FILLSPACE:XEP_FILLEOL;
		OS.iocb[0].command = 20;
		cio(0);
		screen.fillFlag = isXep80Internal()? 0x40: (isIntl()? 0x20: 0x00);
		for (y = 0;y<XEPLINES;y++)setXepRowPtr(y, screen.fillFlag| y);
	} else {
		OS.dspflg = 0;
		OS.iocb[0].buffer = &clearScreenChar;
		OS.iocb[0].buflen = 1;
		OS.iocb[0].command = IOCB_PUTCHR;
		cio(0);
	}
	for (y = 0;y< 24;y++) screen.lineLength[y] = 0;
	OS.rowcrs = 0;
	OS.colcrs = 0;
}

void cursorHide(void)
{
	if (OS.crsinh) return;
	OS.crsinh = 1;
	if (isXep80()) { // XEP80 is insane in Burst mode. colcrs isn't updated, but it still checks colcrs to a hidden shadow when drawing
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = 2;
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = XEP_CURSOROFF;
		OS.iocb[0].command = 20;
		cio(0);
		setXEPXPos(OS.colcrs);
		setXEPYPos(OS.rowcrs);
	}
}

void cursorUpdate(unsigned char x, unsigned char y)
{
	unsigned char xp;
	static const unsigned char moveCursorLeft = CH_CURS_LEFT;
	if (x >= screen.screenWidth || y >= SCREENLINES) {
		cursorHide();
		return;
	}
	flushBuffer();
	xp = x + OS.lmargn;
	if (xp < OS.rmargn)xp++;
	else xp = OS.lmargn;
	if ((OS.crsinh == 0) && (OS.colcrs == xp) && (OS.rowcrs == y))return;
	OS.crsinh = 0;
	OS.colcrs = xp;
	OS.rowcrs = y;
	OS.dspflg = 0;
	screen.xepX = OS.colcrs;screen.xepY = OS.rowcrs;
	OS.iocb[0].buffer = &moveCursorLeft;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}


void drawCharsAt(unsigned char *buffer, unsigned char bufferLen, unsigned char x, unsigned char y)
{
	unsigned char logMapTouch = 0;
	if ((x >= screen.screenWidth) || !bufferLen)return;
	cursorHide();
	if (x + bufferLen > screen.screenWidth)bufferLen = screen.screenWidth - x;
	if (screen.directDraw) {
		writeScreen(buffer, bufferLen, x, y);
		return;
	}
	OS.dspflg = 1;
	OS.rowcrs = y;
	OS.colcrs = x;
	if (isXep80()) {
		screen.xepX = OS.colcrs;screen.xepY = OS.rowcrs;
		OS.iocb[0].buffer = buffer;
		OS.iocb[0].buflen = bufferLen;
		OS.iocb[0].command = IOCB_PUTCHR;
		cio(0);
		setXEPXPos(OS.colcrs); // Key to burst mode.  Need to put cursor back to OS.colcrs
		return;
	}
	if (x + bufferLen >= screen.screenWidth) {
		if (y >= SCREENLINES -1) {
			bufferLen--;
			if (!bufferLen)return;  // Never draw into the very last character of the last line
		}
		logMapTouch = y + 1;
		OS.logmap[0] = OS.logmap[1] = OS.logmap[2] = OS.logmap[3] = 0; // HACK, prevents atari from wrapping
	}
	OS.iocb[0].buffer = buffer;
	OS.iocb[0].buflen = bufferLen;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	if (logMapTouch) {
		OS.logmap[0] = OS.logmap[1] = OS.logmap[2] =  OS.logmap[3] = 0xff; 
	}
}


void flushBuffer(void)
{
	if (!screen.bufferLen)return;
	drawCharsAt(screen.buffer, screen.bufferLen, OS.lmargn + screen.bufferX - screen.bufferLen, screen.bufferY);

	screen.bufferLen = 0;
}

void drawCharAt(unsigned char c, unsigned char attribute, unsigned char x, unsigned char y)
{
	if ((y >= SCREENLINES) || (x >= screen.screenWidth))return;
	if (c == 0x9b)return;
	if (y != screen.bufferY || x != screen.bufferX) flushBuffer();
	if (!screen.bufferLen) {
		screen.bufferX = x;
		screen.bufferY = y;
	}
	screen.buffer[screen.bufferLen++] = c ^(attribute & 0x80);
	screen.bufferX++;
	if ((c != ' ') && (screen.bufferX > screen.lineLength[screen.bufferY]))
		screen.lineLength[screen.bufferY] = screen.bufferX;
}

void drawClearCharsAt(unsigned char len, unsigned char x, unsigned char y)
{
	unsigned char oldLen;
	if ((y >= SCREENLINES) || (x >= screen.screenWidth) || (x >= screen.lineLength[y]))return;
	cursorHide();
	if (y != screen.bufferY || x != screen.bufferX) flushBuffer();
	oldLen = screen.lineLength[y];
	if (len >= screen.screenWidth - x) {
		len = screen.screenWidth - x;
		if (x < screen.lineLength[y])screen.lineLength[y] = x;
	}
	if (len > oldLen - x) len = oldLen - x;
	drawCharsAt(screen.clearBuffer, len, x + OS.lmargn, y);
}

void drawClearLine(unsigned char y)
{
	static unsigned char ch = CH_INSLINE, chD = CH_DELLINE;
	if ((y >= SCREENLINES) || !screen.lineLength[y])return;
	if (isXep80()) {
		drawClearCharsAt(screen.screenWidth - OS.lmargn, OS.lmargn, y);
		return;
	}
	cursorHide();
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn;
	OS.dspflg = 0;
	OS.iocb[0].buffer = &chD;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	screen.lineLength[y] = 0;
}



void drawInsertLine(unsigned char y, unsigned char yBottom)
{
	unsigned char yp, saveLine;
	static unsigned char ch = CH_INSLINE, chD = CH_DELLINE;
	if (isXep80()) {
		drawClearLine(yBottom);
		flushBuffer();
		saveLine = screen.xepLines[yBottom];
		for (yp = yBottom;yp > y;yp--) {
			setXepRowPtr(yp, screen.xepLines[yp-1]);
			screen.lineLength[yp] = screen.lineLength[yp-1];
		}
		setXepRowPtr(y, saveLine);
		screen.lineLength[y] = 0;
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
			OS.iocb[0].buffer = &chD;
			OS.iocb[0].buflen =  1;
			OS.iocb[0].command = IOCB_PUTCHR;
			cio(0);
		}
		OS.rowcrs = y;
		OS.iocb[0].buffer = &ch;
		OS.iocb[0].buflen =  1;
		OS.iocb[0].command = IOCB_PUTCHR;
		cio(0);
	}
	for (yp = yBottom;yp > y;yp--) screen.lineLength[yp] = screen.lineLength[yp-1];
	screen.lineLength[y] = 0;
}

void drawDeleteLine(unsigned char y, unsigned char yBottom)
{
	unsigned char saveLine, yp;
	static unsigned char ch = CH_DELLINE, chI = CH_INSLINE;

	if (isXep80()) {
		drawClearLine(y);
		flushBuffer();
		saveLine = screen.xepLines[y];
		for (yp = y;yp +1 <= yBottom;yp++) {
			setXepRowPtr(yp, screen.xepLines[yp+1]);
			screen.lineLength[yp] = screen.lineLength[yp+1];
		}
		setXepRowPtr(yBottom, saveLine);
		screen.lineLength[yBottom] = 0;

		return;
	}
	flushBuffer();
	if (screen.directDraw) {
		directScrollUp(y, yBottom);
	} else {
		OS.dspflg = 0;
		OS.rowcrs = y;
		OS.colcrs = OS.lmargn;
		OS.iocb[0].buffer = &ch;
		OS.iocb[0].buflen =  1;
		OS.iocb[0].command = IOCB_PUTCHR;
		cio(0);
		if (yBottom < SCREENLINES - 1) { // Probably will blink but have no choice.
			cursorHide();
			OS.rowcrs = yBottom;
			OS.iocb[0].buffer = &chI;
			OS.iocb[0].buflen =  1;
			OS.iocb[0].command = IOCB_PUTCHR;
			cio(0);
		}
	}
	for (yp = y;yp+1 <= yBottom;yp++) screen.lineLength[yp] = screen.lineLength[yp+1];
	screen.lineLength[yBottom] = 0;
}

void drawInsertChar(unsigned char x, unsigned char y)
{
	static unsigned char ch = CH_INSCHR;
	if ((x >= screen.screenWidth) || (x >= screen.lineLength[y]))return;
	flushBuffer();
	if (isXep80()) {
		drawXEPCharAt(CH_EOL, XEPRMARGIN-1, y);
		drawXEPCharAt(CH_EOL, XEPRMARGIN, y);
	}
	screen.lineLength[y]++;
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;
	screen.xepX = OS.colcrs;screen.xepY = OS.rowcrs;
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void drawDeleteChar(unsigned char x, unsigned char y)
{
	static unsigned char ch = CH_DELCHR;
	if ((x >= screen.screenWidth) || (x >= screen.lineLength[y]))return;
	flushBuffer();
	if (isXep80()) {
		drawXEPCharAt(' ', XEPRMARGIN-1, y);
		drawXEPCharAt(CH_EOL, XEPRMARGIN, y);
	}
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;
	screen.xepX = OS.colcrs;screen.xepY = OS.rowcrs;
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	screen.lineLength[y]--;
}

void drawBell(void)
{
}

// 1 = black on white.  0 = white on black
void drawDarkLight(unsigned char val)  // XEP 80 does this. 
{
	val = val;
}

