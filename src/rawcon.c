#include "main.h"

#if RAWCON_ON

typedef struct {
	rawTabStruct *rawTab;
	unsigned char memoryIndex;
	unsigned char charCellX, charCellY;
} rawConStruct;

rawConStruct rawcon;

/*
In ICAX3/4 a pointer is returned that points to the internal function table (also pointed to by the symbol _RAWCON under SpartaDOS X):

	+$00 - number of entries below (9):

		+$01 - enable the display mode defined by ICAX1Z (attributes) and ICAX2Z (mode number).
		+$03 - disable the mode.
		+$05 - shut off the display temporarily.
		+$07 - reactivate after a temporary shutoff.
		+$09 - clear screen and reload colors from 709/710.
		+$0b - reload colors only.
		+$0d - get character from CRSCOL/CRSROW, return its value in A and store its color to 708.
		+$0f - print char given in A at CRSCOL/CRSROW.
		+$11 - call special operation (XIO) defined by parameters passed in ZIOCB.


The ENABLE function returns status in ICSTZ ($23). It not always was so, versions before 1.02 did not touch ICSTZ at all, so to avoid getting garbage there you should zero out ICSTZ before the call.

GET and PUT functions DO NOT automatically advance the cursor to next available position, nor they sanity-check given coordinates etc. These are simple, raw, lowest possible level GET and PUT functions (this is a BIOS, at last). It is programmer's responsibility to verify if the parameters passed to are valid, if they are not, there will be no errors, just a crash. So, contrary to the regular CIO calls, these two "unsafe". But they are also much faster.

ZIOCB parameters relevant to the last vector are the following locations:

* ICCOMZ $22 - opcode (e.g. 96 for XIO 96)
* ICAX1Z $2A - aux1 (3rd parameter for XIO)
* ICAX2Z $2B - aux2 (4th parameter for XIO)
* ICAX5Z $2E - aux5, IOCB number (only required for XIO 103 called this way).

When calling XIO via CIO, not via _RAWCON vector, you do not care about ZIOCB, just pass the parameters the usual way.

b) XIO 111,#n,0,0,"S2:"

This function is available as of driver version 1.01 (it is harmless to call it on older versions, where this opcode does nothing). In VBXE text mode, when called, it switches to a 'fast' text output routine. While in this mode, you cannot change fonts or change global colors of the screen. You still can change local character color though. In pixel modes this function does nothing.

The 'fast' mode may be disabled by re-enabling a VBXE text mode, or by calling XIO 112 (below).

c) XIO 112,#n,0,0,"S2:"

Disables the 'fast' text output mode previously enabled by XIO 111. In pixel mode this call does nothing.

d) XIO 113,#n,m,0,"S2:"

When m=0, loading a 256-character font will not switch the palette to 64 colors. When m=1 (default), it will.

e) XIO 114,#n,0,0,"S2:"

This call is available as of driver version 1.01. It will return (in ICAX2) a byte of information on the default size of a character cell, encoded as follows:

- bits 7-4: character width, less 2
- bits 3-0: character heigth, less 2

When any of the returned values is 0, the information is not available or not applicable. Values of $F are reserved. Therefore, the maximum character cell size possible here is 16x16 pixels.

This allows programs to select and load fonts depending on what is the character cell allowed by the video hardware. For example, the S_VBXE driver returns $66, for standard modes, which means that the character cell is 8x8 pixels (both sizes are decremented by 2), and $65 for "condensed" modes, where the character cell is 8x7. Software 80-column drivers may return 2x6 (character cell 4x8), and 64-column ones: 3x6 (character cell 5x8), and so on.

XIO f,#n,a,l,"S2:"

where:

- f is the function number:

97 - scroll one line up
98 - scroll one line down
99 - clear the specified line

- n is the channel number being in use for the VBXE text mode

- a is the I/O access code, it must be the same, as it was used to OPEN the screen for "S2:"

- l is the line number

XIO 97 & 98: The screen will scroll one line up (97) or down (98) starting at the top line number "l", and ending at the bottom line number contained in BOTSCR ($02BF or 703), less 1. BOTSCR normally contains a value of 24, so that if you give 0 as the "l" in the XIO 97 call, the entire screen will scroll one line up.

The driver uses the VBXE blitter to perform the operations so that they are rather fast.

These functions are primarily designed for text modes, but will also work in pixel modes. On that you can see how fast the VBXE blitter is: it takes it about 15325 CPU clocks to scroll the entire "standard" (320x192/256, 60 KB) display, which is 6,76 MB/s (yes, megabytes per second).

XIO f,#n,a,b,"S2:"

where for f stands for the folowing:

103 - load font specified by "b". The "b" is CHBAS value for font stored in the memory. It works like CHBAS in character modes (see the main document for details). "a" should be 0. NOTE: this must be invoked AFTER the screen mode has been initialized (in other words: screen OPEN loads system font).

XIO 103 also works in text mode, and it is actually a preferred method of changing fonts which is "portable" across different video drivers - as not every driver/hardware combo is fast enough to make the CHBAS method available. See also XIO 114.

*/
 // memoryindex of 2 means?
unsigned char rawConTest(void)
{
	unsigned char err = ERR_NONE;
	rawcon.rawTab = NULL;
	rawcon.memoryIndex = 0;
	if (_dos_type == SPARTADOS) {
		rawcon.rawTab = jfsymbol("_RAWCON  ");
		if (rawcon.rawTab) {
			rawcon.memoryIndex = jfsymbol_memoryIndex;
		}
	}
	if (!rawcon.rawTab) {
		OS.iocb[6].buffer = "S2:";
	    OS.iocb[6].buflen = strlen("S2:");
	    OS.iocb[6].command = VBXEBIOS_DETECT;
	    OS.iocb[6].aux1 = 0;
	    OS.iocb[6].aux2 = 0;
	    OS.ziocb.spare = 0;
	    cio(6);
	    iocbErrUpdate(6, &err);
	    if ((err == ERR_NONE) && (OS.ziocb.spare == 96)) {
	    	rawcon.rawTab = (rawTabStruct *) * (unsigned short *) &OS.iocb[6].aux3;
	    }
	}
	if (!rawcon.rawTab) return 0;
	rawcon.charCellY = rawcon.charCellX = 0;
	err = ERR_NONE;
	OS.iocb[6].buffer = "S2:";
    OS.iocb[6].buflen = strlen("S2:");
    OS.iocb[6].command = VBXEBIOS_GETCHARCELL;
    OS.iocb[6].aux1 = 0;
    OS.iocb[6].aux2 = 0;
    cio(6);
    iocbErrUpdate(6, &err);
    rawcon.charCellY = (OS.iocb[6].aux2 & 0xf) + 2;
    rawcon.charCellX = (OS.iocb[6].aux2 >> 4) + 2;
	return 1;
}

unsigned char supportsCharacterSet(void)
{
	if (rawcon.charCellY != 8) return 0;
	if (rawcon.charCellX == 8) return 1;
	return 0;
}

void drawCharsAtRawCon(unsigned char *buffer, unsigned char bufferLen)
{
	if ((screenX.cursX >= OS.colcrs) && (screenX.cursX < OS.colcrs + bufferLen) && (OS.rowcrs == screenX.cursY)) {
		OS.oldchr = buffer[screenX.cursX - OS.colcrs];
	}
	for (;bufferLen--;) {
		(rawcon.rawTab->putChar)(*buffer++);
		OS.colcrs++;
	}
}

void deleteLineRawCon(unsigned char topY, unsigned char bottomY)
{
	cursorHide();
	if (bottomY > topY) {
		OS.ziocb.command = 97;
		OS.ziocb.aux2 = topY;
		OS.botscr = bottomY + 1;
		(rawcon.rawTab->xio)();
		OS.botscr = SCREENLINES;
		if ((screenX.cursY > topY) && (screenX.cursY <= bottomY)) {
			OS.colcrs = screenX.cursX;OS.rowcrs = screenX.cursY-1;
			(rawcon.rawTab->putChar)(OS.oldchr);
		}
	}

	OS.ziocb.command = 99;
	OS.ziocb.aux2 = bottomY;
	(rawcon.rawTab->xio)();
	if (bottomY == screenX.cursY)OS.oldchr = ' ';
	else {
		OS.colcrs = screenX.cursX;OS.rowcrs = screenX.cursY;
		OS.oldchr = (rawcon.rawTab->getChar)();
	}
}

void insertLineRawCon(unsigned char topY, unsigned char bottomY)
{
	cursorHide();
	if (bottomY > topY) {
		OS.ziocb.command = 98;
		OS.ziocb.aux2 = topY;
		OS.botscr = bottomY + 1;
		(rawcon.rawTab->xio)();
		OS.botscr = SCREENLINES;
		if ((screenX.cursY >= topY) && (screenX.cursY < bottomY)) {
			OS.colcrs = screenX.cursX;OS.rowcrs = screenX.cursY+1;
			(rawcon.rawTab->putChar)(OS.oldchr);
		}
	}
	OS.ziocb.command = 99;
	OS.ziocb.aux2 = topY;
	(rawcon.rawTab->xio)();
	if (topY == screenX.cursY)OS.oldchr = ' ';
	else {
		OS.colcrs = screenX.cursX;OS.rowcrs = screenX.cursY;
		OS.oldchr = (rawcon.rawTab->getChar)();
	}
}

void initRawCon(void)
{
	unsigned char err = ERR_NONE;
	if (detect.fullChbas) {
		initAscii(detect.fullChbas, NULL);
		OS.iocb[6].buffer = "S2:";
   		OS.iocb[6].buflen = strlen("S2:");
 		OS.iocb[6].command = VBXEBIOS_FONTLOAD;
   		OS.iocb[6].aux1 = 0;
    	OS.iocb[6].aux2 = detect.fullChbas;
    	cio(6);
    	iocbErrUpdate(6, &err);
    	if (err != ERR_NONE) {
			setFullAscii(0);
    	}
	}
}

#endif