
#include "main.h"

#define XEPLINES 25
#define SCREENLINES 24
#define SCREENCOLUMNS 255
#define XEPRMARGIN 81 // Two extra for delete character. 

typedef struct screenDataStruct screenStruct;
struct screenDataStruct {
	unsigned char screenWidth;
	unsigned char lastCharX, lastCharY, bufferLen, bufferX, bufferY;
	unsigned char directDraw;
	unsigned char clearBuffer[SCREENCOLUMNS];
	unsigned char buffer[SCREENCOLUMNS];
	unsigned short lineTab[SCREENLINES];
	unsigned char xepLines[XEPLINES];
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
	OS.iocb[0].buffer = NULL;
	OS.iocb[0].buflen = 0;
	OS.iocb[0].command = 0x15;
	OS.iocb[0].aux1 = 0xc;
	OS.iocb[0].aux2 = on;
	cio(0);
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
	OS.dspflg = 1; // Just presume draw
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
	unsigned char n;
	for (n = 0;n < sizeof(screen.clearBuffer);n++)screen.clearBuffer[n] = ' ';
	screen.screenWidth = OS.rmargn + 1; 
	screen.bufferLen = 0;
	screen.bufferX = 255;
	screen.bufferY = 0;
	if (isXep80()) {
		OS.rmargn = XEPRMARGIN;
		screen.screenWidth = XEPRMARGIN - 1;
	}
	screen.directDraw = directDrawTest();
	drawClearScreen();
	OS.dspflg = 1; // Just presume draw
}

void screenRestore(void)
{
	OS.dspflg = 0;
	if (isXep80()) setBurstMode(0);
}

void setXEPHPos(unsigned char hpos) {
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

void setXEPLastChar(unsigned char c)
{ // 80 = cr
	OS.dspflg = 1;
	OS.rowcrs = XEPLINES-1;
	OS.colcrs = 80;
	OS.iocb[0].buffer = &c;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void setXEPExtraByte(unsigned char c)
{
	setXEPLastChar(c);
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = 2;
	OS.iocb[0].aux1 = 12;
	OS.iocb[0].aux2 = XEP_SETEXTRABYTE;
	OS.iocb[0].command = 20;
	cio(0);
}


void drawXEPCharAt(unsigned char c, unsigned char x, unsigned char y)
{
	setXEPLastChar(c);
	setXEPHPos(x);
	setXEPYPos(y);
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = 2;
	OS.iocb[0].aux1 = 12;
	OS.iocb[0].aux2 = XEP_WRITEBYTE;
	OS.iocb[0].command = 20;
	cio(0);
	if (x != OS.colcrs) setXEPHPos(OS.colcrs);
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
	unsigned char fillFlag;
	unsigned char y;
	static const unsigned char clearScreen = CH_CLR;
	flushBuffer();
	if (isXep80()) {
		OS.iocb[0].buffer = eColon;
		OS.iocb[0].buflen = strlen(eColon);
		OS.iocb[0].aux1 = 12;
		OS.iocb[0].aux2 = isXep80Internal()? XEP_FILLSPACE:XEP_FILLEOL;
		OS.iocb[0].command = 20;
		cio(0);
		fillFlag = isXep80Internal()? 0x40: (isIntl()? 0x20: 0x00);
		for (y = 0;y<XEPLINES;y++)setXepRowPtr(y, fillFlag| y);
		for (y = 0;y< 24;y++) {
			drawXEPCharAt(CH_EOL, XEPRMARGIN-1, y);
			drawXEPCharAt(CH_EOL, XEPRMARGIN, y);
		}
	} else {
		OS.dspflg = 0;
		OS.iocb[0].buffer = &clearScreen;
		OS.iocb[0].buflen = 1;
		OS.iocb[0].command = IOCB_PUTCHR;
		cio(0);
	}
	OS.dspflg = 1;
	OS.rowcrs = 0;
	OS.colcrs = 0;
}

void cursorHide(void)
{
	if (OS.crsinh) return;
	OS.crsinh = 1;
}

void cursorUpdate(unsigned char x, unsigned char y)
{
	static const unsigned char moveCursorUp = CH_CURS_UP;
	if (x >= screen.screenWidth || y >= SCREENLINES) {
		OS.crsinh = 1;
		return;
	}
	if ((OS.crsinh == 0) && (OS.colcrs == x + OS.lmargn) && (OS.rowcrs ==y))return;
	OS.crsinh = 0;
	OS.colcrs = x + OS.lmargn;
	OS.rowcrs = y;
	OS.dspflg = 0;
	if (OS.rowcrs +1 < SCREENLINES)OS.rowcrs++;
	else OS.rowcrs = 0;
	OS.iocb[0].buffer = &moveCursorUp;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	OS.colcrs = x + OS.lmargn;
	OS.rowcrs = y;
	OS.dspflg = 1; // Just presume draw
}


void drawCharsAt(unsigned char *buffer, unsigned char bufferLen, unsigned char x, unsigned char y)
{
	unsigned char burst;
	unsigned char logMapTouch = 0;
	if ((x >= screen.screenWidth) || !bufferLen)return;
	if (screen.directDraw) {
		writeScreen(buffer, bufferLen, x, y);
		return;
	}
	OS.rowcrs = y;
	OS.colcrs = x;
	if (isXep80()) {
		if (x + bufferLen >= screen.screenWidth)bufferLen = screen.screenWidth - x;
		if (bufferLen > 1) {
			setBurstMode(1);
			burst = 1;
		} else burst = 0;
		OS.iocb[0].buffer = buffer;
		OS.iocb[0].buflen = bufferLen-burst;
		OS.iocb[0].command = IOCB_PUTCHR;
		cio(0);
		OS.colcrs = x + bufferLen-burst;
		if (burst) {
			setBurstMode(0);
			OS.iocb[0].buffer = buffer + bufferLen - 1;
			OS.iocb[0].buflen = 1;
			OS.iocb[0].command = IOCB_PUTCHR;
			cio(0);
		}
		return;
	}
	if (x + bufferLen >= screen.screenWidth) {
		bufferLen = screen.screenWidth - x;
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
	if (x + bufferLen >= screen.screenWidth) {
		OS.colcrs = OS.lmargn;
		if (y < SCREENLINES -1) OS.rowcrs = y + 1;
		else OS.rowcrs = SCREENLINES - 1;
	} else {
		OS.colcrs = x + bufferLen;
		OS.rowcrs = y;
	}
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
	if (y != screen.bufferY || x != screen.bufferX) flushBuffer();
	if (!screen.bufferLen) {
		screen.bufferX = x;
		screen.bufferY = y;
	}
	screen.buffer[screen.bufferLen++] = c ^(attribute & 0x80);
	screen.bufferX++;
}

void drawClearCharsAt(unsigned char len, unsigned char x, unsigned char y)
{
	if ((y >= SCREENLINES) || (x >= screen.screenWidth))return;
	OS.crsinh = 1;
	drawCharsAt(screen.clearBuffer, len > screen.screenWidth - x? screen.screenWidth - x: len, x + OS.lmargn, y);
}

void drawClearLine(unsigned char y)
{
	static unsigned char ch = CH_INSLINE, chD = CH_DELLINE;
	if (y >= SCREENLINES)return;
	OS.crsinh = 1;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn;
	if (isXep80()) {
		OS.iocb[0].buffer = screen.clearBuffer;
		OS.iocb[0].buflen =  screen.screenWidth - OS.lmargn;
		OS.iocb[0].command = IOCB_PUTCHR;
		cio(0);
		return;
	}
	OS.dspflg = 0;
	OS.iocb[0].buffer = &chD;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	OS.dspflg = 1;
}



void drawInsertLine(unsigned char y, unsigned char yBottom)
{
	unsigned char yp, saveLine;
	static unsigned char ch = CH_INSLINE, chD = CH_DELLINE;
	flushBuffer();
	if (isXep80()) {
		setBurstMode(1);
		drawClearLine(yBottom);
		saveLine = screen.xepLines[yBottom];
		for (yp = yBottom;yp > y;yp--) setXepRowPtr(yp, screen.xepLines[yp-1]);
		setXepRowPtr(y, saveLine);
		setBurstMode(0);
		return;
	}
	OS.dspflg = 0;
	OS.crsinh = 1;
	OS.colcrs = OS.lmargn;
	if (yBottom < SCREENLINES -1) {
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
	OS.dspflg = 1;
}

void drawDeleteLine(unsigned char y, unsigned char yBottom)
{
	unsigned char saveLine, yp;
	static unsigned char ch = CH_DELLINE, chI = CH_INSLINE;
	flushBuffer();
	if (isXep80()) {
		setBurstMode(1);
		drawClearLine(y);
		saveLine = screen.xepLines[y];
		for (yp = y;yp +1 <= yBottom;yp++) setXepRowPtr(yp, screen.xepLines[yp+1]);
		setXepRowPtr(yBottom, saveLine);
		setBurstMode(0);
		return;
	}
	OS.crsinh = 1;
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn;
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	if (yBottom < SCREENLINES - 1) { // Probably will blink but have no choice.
		OS.rowcrs = yBottom;
		OS.iocb[0].buffer = &chI;
		OS.iocb[0].buflen =  1;
		OS.iocb[0].command = IOCB_PUTCHR;
		cio(0);
	}
}

void drawInsertChar(unsigned char x, unsigned char y)
{
	static unsigned char ch = CH_INSCHR;
		if (x >= screen.screenWidth)return;
	flushBuffer();
	if (isXep80()) {
		drawXEPCharAt(CH_EOL, XEPRMARGIN-1, y);
		drawXEPCharAt(CH_EOL, XEPRMARGIN, y);
	}
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void drawDeleteChar(unsigned char x, unsigned char y)
{
	static unsigned char ch = CH_DELCHR;
	flushBuffer();
	if (x >= screen.screenWidth)return;
	if (isXep80()) {
		drawXEPCharAt(' ', XEPRMARGIN-1, y);
		drawXEPCharAt(CH_EOL, XEPRMARGIN, y);
	}
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void drawBell(void)
{
}

// 1 = black on white.  0 = white on black
void drawDarkLight(unsigned char val)  // XEP 80 does this. 
{
	val = val;
}

