#include "main.h"

#define XEPLINES 25
#define XEPRMARGIN 81 // Two extra for delete character. 
#define XEPATR_INVERSE 0x1
#define XEPATR_BLINK 0x4
#define XEPATR_DOUBLEWIDE 0x10
#define XEPATR_UNDERLINE 0x20
#define XEPATR_BLANK 0x40
#define XEPATR_GRAPHICS 0x80

typedef struct {
	unsigned char origRMargn;
	unsigned char xepCharset;
	unsigned char burst;
	unsigned char xepX, currentXepX; // xepX, shadow cached value of colcrs, rowcrs in xep80 driver.
	unsigned char fillFlag;
	unsigned char xepLines[XEPLINES];
}xepStruct;

xepStruct xep;

void setBurstMode(unsigned char on)
{
	if (xep.burst == on)return;
	callEColonSpecial(0x15, 0xc, on);
	xep.burst = on;
}

unsigned char isXep80Internal(void) {
	return xep.xepCharset == XEPCH_INTERN;
}

void setXEPXPos(unsigned char hpos) {
	if (hpos < 80) {
		callEColonSpecial(20, 12, XEP_SETCURSORHPOS | hpos);
	} else {
		callEColonSpecial(20, 12, XEP_SETCURSORHPOS | (hpos & 0xf));
		callEColonSpecial(20, 12, XEP_SETCURSORHPOSHI + (hpos >> 4));
	}
	xep.currentXepX = hpos;
}

void setXEPYPos(unsigned char y) {
	callEColonSpecial(20, 12, XEP_SETCURSORVPOS + y);
}


// This is secret of XEP80 burst mode here. 
// Shadow the cursor shadow that's internal to the XEP driver in xepX and keep xep.currentXepX updated with where the cursor really is. 
// if we're setting a cursor x position that's equal to the internal shadow in the driver, that cursor position is not set
// But in burst mode, current cursor position is mostly just wrong, so set the hardware.
// This code never sends any CIO commands that affect Y.

void xepCursorShadow(void)
{
	if (!isXep80())return;
	if (OS.colcrs == xep.xepX) {
		if (xep.xepX != xep.currentXepX) {
			setXEPXPos(xep.xepX);
		}
	} else {
		xep.xepX = OS.colcrs;
	}
}

void setXEPLastChar(unsigned char c)
{ // 80 = cr
	OS.dspflg = 1;
	OS.rowcrs = XEPLINES-1;
	OS.colcrs = XEPRMARGIN+1;
	xepCursorShadow();
	callEColonPutByte(c);
	OS.colcrs++; 
	xep.currentXepX = OS.colcrs;
}

void setXEPCommand(unsigned char c, unsigned char command)
{
	setXEPLastChar(c);
	callEColonSpecial(20, 12, command);
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
	callEColonSpecial(20, 12, XEP_WRITEBYTE);
}

void setXepRowPtr(unsigned char y, unsigned char val) {
	setXEPExtraByte(y + 0x20);
	setXEPLastChar(val);
	callEColonSpecial(20, 12, XEP_WRITEINTERNALBYTE);
	xep.xepLines[y] = val;
}

void setXEPRMargin(unsigned char x) {
	OS.iocb[0].buffer = eColon;
	OS.iocb[0].buflen = 2;
	if ((x >= 0x40) && (x < 0x50)) {
		callEColonSpecial(20, 0xc, XEP_SETRIGHTMARGINLO | (x-0x40));
	} else {
		callEColonSpecial(20, 0xc, XEP_SETRIGHTMARGINLO | (x & 0xf));
		callEColonSpecial(20, 0xc, XEP_SETRIGHTMARGINHI + (x >> 4));
	}
}



unsigned char setXepCharSet(unsigned char which)
{
	unsigned char err = ERR_NONE;
	if (!isXep80() || (which == xep.xepCharset))return err;
	callEColonSpecial(20, 12, which);
	xep.xepCharset = which;
	setFullAscii((which == XEPCH_INTERN));
	return err;
}


void deleteLineXep(unsigned char y, unsigned char yBottom)
{
	unsigned char saveLine, yp;
	drawClearLine(y);
	flushBuffer();
	saveLine = xep.xepLines[y];
	for (yp = y;yp +1 <= yBottom;yp++) {
		setXepRowPtr(yp, xep.xepLines[yp+1]);
		screenX.lineLength[yp] = screenX.lineLength[yp+1];
	}
	setXepRowPtr(yBottom, saveLine);
	screenX.lineLength[yBottom] = 0;
}

void insertLineXep(unsigned char y, unsigned char yBottom)
{
	unsigned char yp, saveLine;
	drawClearLine(yBottom);
	flushBuffer();
	saveLine = xep.xepLines[yBottom];
	for (yp = yBottom;yp > y;yp--) {
		setXepRowPtr(yp, xep.xepLines[yp-1]);
		screenX.lineLength[yp] = screenX.lineLength[yp-1];
	}
	setXepRowPtr(y, saveLine);
	screenX.lineLength[y] = 0;
}

void clearScreenXep(void)
{
	unsigned char y;
	callEColonSpecial(20, 0xc, isXep80Internal()? XEP_FILLSPACE:XEP_FILLEOL);
	xep.fillFlag = isXep80Internal()? 0x40: (isIntl()? 0x20: 0x00);
	for (y = 0;y<XEPLINES;y++)setXepRowPtr(y, xep.fillFlag| y);
}

void deleteCharXep(unsigned char x, unsigned char y)
{
	cursorHide();
	drawXEPCharAt(' ', XEPRMARGIN-1, y);
	drawXEPCharAt(CH_EOL, XEPRMARGIN, y);
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;
	xepCursorShadow();
	callEColonPutByte(CH_DELCHR);
	xep.currentXepX = OS.colcrs;
	screenX.lineLength[y]--;
}

void insertCharXep(unsigned char x, unsigned char y)
{
	cursorHide();
	drawXEPCharAt(CH_EOL, XEPRMARGIN-1, y);
	drawXEPCharAt(CH_EOL, XEPRMARGIN, y);
	screenX.lineLength[y]++;
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = OS.lmargn + x;
	xepCursorShadow();
	callEColonPutByte(CH_INSCHR);
	xep.currentXepX = OS.colcrs;
}

void drawCharsAtXep(unsigned char *buffer, unsigned char bufferLen)
{
	xepCursorShadow();
	callEColonPutBytes(buffer, bufferLen);
	xep.currentXepX = OS.colcrs = OS.colcrs + bufferLen;
}

void cursorUpdateXep(unsigned char x, unsigned char y)
{
	OS.crsinh = 0;
	OS.colcrs = x + OS.lmargn;
	if (OS.colcrs < OS.rmargn)OS.colcrs++;
	else OS.colcrs = OS.lmargn;
	OS.rowcrs = y;
	OS.dspflg = 0;
	xepCursorShadow();
	callEColonPutByte( CH_CURS_LEFT);
	xep.currentXepX = OS.colcrs = x + OS.lmargn;
}

void initXep(void)
{
	xep.burst = 0;
	xep.origRMargn = OS.rmargn;
	setBurstMode(1);
	OS.colcrs = 0;
	setXEPXPos(OS.colcrs);
	OS.rowcrs = OS.lmargn;
	setXEPYPos(OS.rowcrs);
	OS.rmargn = XEPRMARGIN;
	setXEPRMargin(OS.rmargn);
	xep.xepX = OS.colcrs;
	xep.currentXepX = xep.xepX;
	screenX.screenWidth = XEPRMARGIN - 1;
	callEColonSpecial(20, 12, XEP_CURSORON);
	setXepCharSet(XEPCH_INTERN);
}

void restoreXep(void)
{
	setXEPXPos(OS.colcrs);
	setXEPYPos(OS.rowcrs);
	setBurstMode(0);
	setXepCharSet(isIntl()? XEPCH_ATINT: XEPCH_ATASCII);
	OS.rmargn = xep.origRMargn;
}