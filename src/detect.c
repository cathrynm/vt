#include "main.h"

detectStruct detect;

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


void initDetect(void)
{
	unsigned short startAddress = (unsigned short)_STARTADDRESS__; // Start the program on 0x400 boundary.  So 0x400 below is good
	detect.osType = get_ostype() & AT_OS_TYPE_MAIN;
	detect.fullAscii = 0; // Set for whether {}~ chars are available.
	if (directDrawTest()) {
		detect.videoMode = 'D';
	} else if (XEP80Test()) {
		detect.videoMode = 'X';
	} else if (vbxeTest()) {
		detect.videoMode = 'V';
	} else if (logMapTrickTest()) {
		detect.videoMode = 'A'; // This is normal Atari mode, but not direct drawing.  I think everything that does this passes directDrawTest, but maybe someday...
	} else {
		detect.videoMode = 'S'; // The Spartdos 80 column mode lands here. 
	}
	detect.chbas = OS.chbas;
	if (detect.videoMode == 'D') {  // Can VBXE display custom charsets?
		if (((unsigned short) OS.memlo + 0x400 <= startAddress)) {
			startAddress -= 0x400;
			initAscii(startAddress >> 8);
		}
	}
	if ((unsigned short) OS.memlo < startAddress)
		_heapadd(OS.memlo, startAddress - (unsigned short) OS.memlo); // recover memory below font and above lomem
}

void closeDetect(void)
{
	OS.chbas = detect.chbas;
}