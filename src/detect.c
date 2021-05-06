#include "main.h"

detectStruct detect;

#if CIO_ON

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

#endif


void initDetect(void)
{
	unsigned short startAddress = (unsigned short)&_STARTADDRESS__; // Start the program on 0x400 boundary.  So 0x400 below is good
	detect.osType = get_ostype() & AT_OS_TYPE_MAIN;
	detect.origChbas = OS.chbas;
	detect.logMapTrick = 0;
#if ATARIINTERNATIONAL
	detect.chbas = OS.chbas;
#else
	detect.chbas = 0xe0;
#endif
	detect.fullChbas = 0;
	detect.hasColor  = 0;
	detect.videoMode = 0;
#if DIRECT_ON
	if (!detect.videoMode && directDrawTest()) {
		detect.videoMode = 'D';
	}
#endif
#if XEP_ON
	if (!detect.videoMode && XEP80Test()) {
		detect.videoMode = 'X';
	}
#endif
#if VBXE_ON
	if (!detect.videoMode && vbxeTest()) {
		detect.videoMode = 'V';
		detect.hasColor = 1;
	}
#endif
#if RAWCON_ON
	if (!detect.videoMode && rawConTest()) {
		detect.videoMode = 'R';
	}
#endif
#if CIO_ON
	if (!detect.videoMode) {
		if (logMapTrickTest()) detect.logMapTrick = 1; // Real atari OS can draw in 39th column by monkeying with logmap
		detect.videoMode = 'A'; // This is normal Atari mode, but not direct drawing.  I think everything that does this passes directDrawTest, but maybe someday...
	}
#endif

#if DIRECT_ON || RAWCON_ON
	if (
#if DIRECT_ON
		  (detect.videoMode == 'D')
#endif
#if DIRECTON && RAWCON_ON
 || 
#endif
#if RAWCON_ON
 		  ((detect.videoMode == 'R') &&  supportsCharacterSet())
#endif
 		) {
		if (((unsigned short) OS.memlo + 0x400 <= startAddress)  && !(startAddress & 0x3ff)) {
			startAddress -= 0x400;
			detect.fullChbas = startAddress >> 8;
		}
	}
#endif
	if ((unsigned short) OS.memlo < startAddress)
		_heapadd(OS.memlo, startAddress - (unsigned short) OS.memlo); // recover memory below font and above lomem
}

void closeDetect(void)
{
}