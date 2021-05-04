#include "main.h"

#if XEP_ON

#define XEPLINES 25
#define XEPATR_INVERSE 0x1
#define XEPATR_BLINK 0x4
#define XEPATR_DOUBLEWIDE 0x10
#define XEPATR_UNDERLINE 0x20
#define XEPATR_BLANK 0x40
#define XEPATR_GRAPHICS 0x80

typedef struct {
	unsigned char origRMargn;
	unsigned char rMargn;
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
	return (xep.xepCharset == XEPCH_INTERN) && (detect.videoMode == 'X');
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

void __fastcall__ xepCursorShadow(void)
{
	if (OS.colcrs == xep.xepX) {
		if (xep.xepX != xep.currentXepX) {
			setXEPXPos(xep.xepX);
		}
	} else {
		xep.xepX = OS.colcrs;
	}
}

void __fastcall__ setXEPLastChar(unsigned char c)
{ // 80 = cr
	OS.dspflg = 1;
	if ((OS.rowcrs != XEPLINES-1) || (c == CH_EOL)) {
		OS.rmargn = 255;
		OS.rowcrs = XEPLINES-1;
		OS.colcrs = xep.rMargn+3;
		xepCursorShadow();
		xep.currentXepX = OS.colcrs;
		if (c == CH_EOL)setXEPYPos(XEPLINES-1); // What exactly is going on with EOL on the last line?  I have no idea really, but this fixes a bug with deleteChar
	}
	callEColonPutByte(c);
	xep.currentXepX =  (c == CH_EOL)? OS.lmargn : xep.currentXepX+1;
}

void __fastcall__ setXEPCommand(unsigned char c, unsigned char command)
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

void __fastcall__ setXepRowPtr(unsigned char y, unsigned char val) {
	setXEPExtraByte(y + 0x20);
	setXEPLastChar(val);
	callEColonSpecial(20, 12, XEP_WRITEINTERNALBYTE);
	xep.xepLines[y] = val;
}

void setXEPRMargin(unsigned char x) {
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
	if (which == xep.xepCharset)return err;
	callEColonSpecial(20, 12, which);
	xep.xepCharset = which;
	switch(xep.xepCharset) {
		case XEPCH_INTERN:
			screenX.charSet = 'X';
			break;
		case XEPCH_ATASCII:
			screenX.charSet = 'A';
			break;
#if ATARIINTERNATIONAL
		case XEPCH_ATINT:
			screenX.charSet = 'I';
			break;
#endif
	}
	setFullAscii((which == XEPCH_INTERN)); // Internal charset has { } ~ characters
	return err;
}


void deleteLineXep(unsigned char y, unsigned char yBottom)
{
	unsigned char saveLine, yp;
	drawClearLine(y, 0);
	flushBuffer();
	saveLine = xep.xepLines[y];
	for (yp = y;yp +1 <= yBottom;yp++) {
		setXepRowPtr(yp, xep.xepLines[yp+1]);
	}
	setXepRowPtr(yBottom, saveLine);
}

void insertLineXep(unsigned char y, unsigned char yBottom)
{
	unsigned char yp, saveLine;
	drawClearLine(yBottom, 0);
	flushBuffer();
	saveLine = xep.xepLines[yBottom];
	for (yp = yBottom;yp > y;yp--) {
		setXepRowPtr(yp, xep.xepLines[yp-1]);
	}
	setXepRowPtr(y, saveLine);
}

void clearScreenXep(void)
{
	callEColonSpecial(20, 0xc, (screenX.charSet == 'X')? XEP_FILLSPACE:XEP_FILLEOL);
}

void deleteCharXep(unsigned char x, unsigned char y)
{
	cursorHide();
	drawXEPCharAt(CH_EOL, xep.rMargn, y);
	OS.rmargn = xep.rMargn;
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = x;
	xepCursorShadow();
	callEColonPutByte(CH_DELCHR);
	xep.currentXepX = OS.colcrs;
	drawXEPCharAt(' ', xep.rMargn - 1, y);
}

void insertCharXep(unsigned char x, unsigned char y)
{
	cursorHide();
	drawXEPCharAt(CH_EOL, xep.rMargn, y);
	drawXEPCharAt(CH_EOL, xep.rMargn-1, y);
	OS.rmargn = xep.rMargn;
	OS.dspflg = 0;
	OS.rowcrs = y;
	OS.colcrs = x;
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
	unsigned char y;
	xep.burst = 0;
	xep.origRMargn = OS.rmargn;
	xep.rMargn = OS.rmargn + 1;
	setBurstMode(1);
	setXepCharSet(XEPCH_INTERN);
	switch(screenX.charSet) {
		case 'X':
			xep.fillFlag = 0x40;
			break;
#if ATARIINTERNATIONAL
		case 'I':
			xep.fillFlag = 0x20;
			break;
#endif
		case 'A':
			xep.fillFlag = 0x00;	
			break;
	}
	for (y = 0;y<XEPLINES;y++)setXepRowPtr(y, xep.fillFlag| y);
	OS.colcrs = OS.lmargn;
	xep.xepX = OS.colcrs;
	setXEPXPos(OS.colcrs);
	OS.rowcrs = 0;
	setXEPYPos(OS.rowcrs);
	OS.rmargn = xep.rMargn;
	screenX.screenWidth = xep.rMargn - OS.lmargn; 
	setXEPRMargin(OS.rmargn);
	callEColonSpecial(20, 12, XEP_CURSORON);
}

void restoreXep(void)
{
	setXEPXPos(OS.colcrs);
	setXEPYPos(OS.rowcrs);
	setBurstMode(0);
	setXepCharSet(isIntl()? XEPCH_ATINT: XEPCH_ATASCII);
	OS.rmargn = xep.origRMargn;
}

unsigned char XEP80Test(void)
{
	unsigned char err;
	if (OS.rmargn >= 0x40) {
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
	return 0;
}

#endif