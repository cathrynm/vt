#include "main.h"


#define NUMBANKS 64
typedef struct {
	unsigned char *t;
	unsigned char tIndex;
	unsigned char numBanks;
	unsigned char usedBank;
} extraRamStruct;

extraRamStruct extraRam;
// aux4/5 ($0785/6) block size;
// -the Y register must be zeroed;
// -the X register must contain the index of the desired memory: 
// for the conventionalRAM its value is $00. 
// If the block has to be page-aligned, 
// add $40 to the index(alignment works as of SDX 4.43).

// free banks occupy the range of indices from 4 to (COMTAB+$1D)+3 – if thevalue of COMTAB+$1D is not a zero

#define NBANKS 0x1d // Free banks
void extRamDetect(void)
{
	unsigned char *comtab;
	extraRam.numBanks = 0;
	extraRam.usedBank = 0;
	if (_dos_type == SPARTADOS) {
		comtab = (unsigned char *)OS.dosvec; 
		extraRam.numBanks =  comtab[NBANKS];
		if (extraRam.numBanks > 0) {
			extraRam.t = jfsymbol("T_       ");
			extraRam.tIndex = jfsymbol_memoryIndex;
			if (!extraRam.t) extraRam.numBanks = 0;
		}
	}
}

void extRamInit(void) // Restart to no banks used.
{
	extraRam.usedBank = 0;
}

unsigned char getExtraBank(void) {
	unsigned char bank;
	if (extraRam.usedBank >= extraRam.numBanks) return 0;
	bank = extraRam.usedBank++ + 4;
	return extraRam.t[8 - 1 + (bank >> 2)] | ((bank & 3) << 2);
}