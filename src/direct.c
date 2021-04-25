#include "main.h"

typedef struct {
	unsigned char lineWidth;
} directStruct;

directStruct direct;
unsigned char sAtascii[4] = {0x40, 0x00, 0x20, 0x60};

void drawCharsAtDirect(unsigned char *s, unsigned char len)
{
	unsigned char *p, *pStart, c;
	pStart = OS.savmsc + OS.colcrs + screenX.lineTab[OS.rowcrs];
	for (p = pStart;len--;) {
		c = *s++;
		*p++ = sAtascii[(c & 0x60) >> 5] | (c & 0x9f);
	}
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) p)) {
		OS.oldchr = pStart[(unsigned short)OS.oldadr - (unsigned short)pStart];
	}
}


void deleteLineDirect(unsigned char topY, unsigned char bottomY)
{
	unsigned char y, len;
	unsigned char *p, *pStart, *pEnd, *pBottom;
	pStart = OS.savmsc + screenX.lineTab[topY];
	pBottom =  OS.savmsc + screenX.lineTab[bottomY];
	pEnd = pBottom + screenX.screenWidth;
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		pStart[(unsigned short)OS.oldadr - (unsigned short)pStart] = OS.oldchr;
	}
	if (bottomY > topY) {
		// memmove(pStart, &pStart[screenX.screenWidth], screenX.lineTab[bottomY - topY]);
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

void insertLineDirect(unsigned char topY, unsigned char bottomY)
{
	unsigned char *pStart, *pEnd;
	pStart = OS.savmsc + screenX.lineTab[topY];
	pEnd =  OS.savmsc + screenX.lineTab[bottomY] + screenX.screenWidth;
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		pStart[(unsigned short)OS.oldadr - (unsigned short)pStart] = OS.oldchr;
	}
	if (bottomY > topY) {
		 memmove(&pStart[screenX.screenWidth], pStart, screenX.lineTab[bottomY - topY]);
	}
	if (screenX.lineLength[topY] > 0)
		memset(pStart, 0, screenX.lineLength[topY]);
	if (((unsigned short) OS.oldadr >= (unsigned short) pStart)  && 
		 ((unsigned short) OS.oldadr < (unsigned short) pEnd)) {
		OS.oldchr = pStart[(unsigned short)OS.oldadr - (unsigned short)pStart];
	}
}

unsigned char directDrawTest(void)
{
	unsigned char *scrnPtr;
	unsigned char n, y;
	static const unsigned char drawTest[]  = {CH_CLR, '0', CH_EOL, '1'};
	OS.dspflg = 0;
	callEColonPutBytes((unsigned char *)drawTest, 4);
	scrnPtr = OS.sdlst;
	scrnPtr += 32;
	if (scrnPtr[0] + 32 != '0')return 0;

	for (n = 1;n < 255;n++) {
		if (scrnPtr[n]  + 32 == '1') {
			for (y = 0;y< SCREENLINES;y++) {
				screenX.lineTab[y] = (unsigned short) y * (unsigned short) n;
			}
			direct.lineWidth = n;
			return 1;
		}
	}
	return 0;
}

void initDirect(void)
{
	unsigned char y;
	if (detect.fullChbas) {
		initAscii(detect.fullChbas);
		OS.chbas = detect.fullChbas;
	}
	for (y = 0;y< SCREENLINES;y++) {
		screenX.lineTab[y] = (unsigned short) y * (unsigned short) direct.lineWidth;
	}
}

void restoreDirect(void)
{
	if (detect.fullChbas)OS.chbas = detect.chbas;
}