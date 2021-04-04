
#include "main.h"

#define SCREENLINES 24
#define SCREENCOLUMNS 80

typedef struct screenDataStruct screenStruct;
struct screenDataStruct {
	unsigned char screenWidth;
	unsigned char lastCharX, lastCharY, bufferLen, bufferX, bufferY;
	unsigned char xep80;
	unsigned char clearBuffer[SCREENCOLUMNS+1];
	unsigned char buffer[SCREENCOLUMNS];
};

screenStruct screen;

#define crt0 _BSS_LOAD__ // wretched hack, but whatever.
extern struct {
	unsigned char SP_save;
	unsigned char SHFLOK_save; // dependent on crt0.s. For uknown reason cc65 monkeys with these.
	unsigned char LMARGN_save;
} crt0;

static unsigned char sAtascii[4] = {0x20, 0x00, 0x40, 0x60};
static unsigned short lineTab[24] = {
			40* 0, 40* 1, 40* 2, 40* 3, 40 * 4, 40* 5, 40* 6, 40* 7, 40* 8, 40* 9,
			40*10, 40*11, 40*12, 40*13, 40 *14, 40*15, 40*16, 40*17, 40*18, 40*19,
			40*20, 40*21, 40*22, 40*23
			};

void writeScreen(unsigned char *s, unsigned char len, unsigned char x, unsigned char y)
{
	unsigned char *p = OS.savmsc + x + lineTab[y], n;
	for (;n--;) {
		c = *s++;
		*p++ = sAtascii[(c& 0x60) >> 5] | (c & ~0x60);
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
	OS.iocb[0].aux2 = on;
	cio(0);
}


void drawChar(unsigned char ch)
{
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}



unsigned char XEP80Test(void)
{
	static unsigned char *eColon = "E:";
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
	screen.bufferLen = 0;
	screen.bufferX = 255;
	screen.bufferY = 0;
	return 0;
}

void initScreen(void)
{
	unsigned char err = ERR_NONE;
	unsigned char n;
	static const unsigned char c = CH_EOL;
	OS.shflok = crt0.SHFLOK_save;
	OS.lmargn = crt0.LMARGN_save;
	for (n = 0;n < sizeof(screen.clearBuffer);n++)screen.clearBuffer[n] = ' ';
	screen.clearBuffer[sizeof(screen.clearBuffer)-1] = CH_ENTER;

	screen.xep80 = XEP80Test();
	if (screen.xep80) {
		screen.screenWidth = OS.rmargn + 1- OS.lmargn; 
		OS.rmargn = 255;
		setBurstMode(1); // Just keep the thing in Burst mode all the time.
	} else {
		screen.screenWidth = OS.rmargn + 1 - OS.lmargn; 
	}
	drawClearScreen();
	OS.dspflg = 1; // Just presume draw

}

void screenRestore(void)
{
	OS.dspflg = 0;
	if (screen.xep80) setBurstMode(0);
}

void drawClearScreen(void)
{
	static const unsigned char clearScreen = CH_CLR;
	OS.dspflg = 0;
	OS.iocb[0].buffer = &clearScreen;
	OS.iocb[0].buflen = 1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	OS.dspflg = 1; // Just presume draw
}

void cursorUpdate(unsigned char x, unsigned char y)
{
	static const unsigned char moveCursorUp = CH_CURS_UP;
	if (x > OS.rmargn || x >= SCREENCOLUMNS || y >= SCREENLINES) {
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
	OS.dspflg = 1; // Just presume draw
}


void drawCharsAt(unsigned char *buffer, unsigned char bufferLen, unsigned char x, unsigned char y)
{
	unsigned char logMapTouch = 0;
	if (x > OS.rmargn)return;
	if (x + bufferLen > OS.rmargn) {
		bufferLen = OS.rmargn + 1 - x;
		if (y >= SCREENLINES -1) {
			bufferLen--;
			if (!bufferLen)return;  // Never draw into the very last character of the last line
		}
		logMapTouch = y + 1;

	} 		OS.logmap[0] = OS.logmap[1] = OS.logmap[2] = OS.logmap[3] = 0; // HACK, prevents atari from wrapping
	OS.rowcrs = y;
	OS.colcrs = x;
	OS.iocb[0].buffer = buffer;
	OS.iocb[0].buflen = bufferLen;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
	//if (logMapTouch) {
		OS.logmap[0] = OS.logmap[1] = OS.logmap[2] =  OS.logmap[3] = 0xff; 
	//}
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
	flushBuffer(); // Does buffering even help I wonder?
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
	static unsigned char ch = CH_INSLINE, chD = CH_DELLINE;
	flushBuffer();
	OS.dspflg = 0;
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
	static unsigned char ch = CH_DELLINE, chI = CH_INSLINE;
	flushBuffer();
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
	flushBuffer();
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
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;
	OS.iocb[0].buffer = &ch;
	OS.iocb[0].buflen =  1;
	OS.iocb[0].command = IOCB_PUTCHR;
	cio(0);
}

void drawScrollScreenDown(unsigned char yTop, unsigned char yBottom)
{
	drawInsertLine(yTop, yBottom);
}

void drawScrollScreen(unsigned char yTop, unsigned char yBottom)
{
	drawDeleteLine(yTop, yBottom);
}

void drawBell(void)
{
}

// 1 = black on white.  0 = white on black
void drawDarkLight(unsigned char val)  // XEP 80 does this. 
{
	val = val;
}

