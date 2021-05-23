#include "main.h"
#if IOBUFFER
#define MAXBANKS 8

typedef struct {
	unsigned char numBanks;
	unsigned char exVals[MAXBANKS];
	unsigned char readBank;
	unsigned char *readBankAddr;
	unsigned char writeBank;
	unsigned char *writeBankAddr;
} ioBufferStruct;
ioBufferStruct ioBuffer;

void ioBufferInit(void)
{
	unsigned char n;
	for (n=0;n<MAXBANKS;n++) {
		ioBuffer.exVals[n] = getExtraBank();
		if (!ioBuffer.exVals[n])break;
		ioBuffer.numBanks = n + 1; 
	}
	ioBuffer.readBank = 0;
	ioBuffer.readBankAddr  = (unsigned char *)0x4000;
	ioBuffer.writeBank = 0;
	ioBuffer.writeBankAddr = (unsigned char *)0x4000;
}

unsigned short ioBufferStatus(unsigned char *err)
{
	unsigned short outputReady, bankTop;
	unsigned short inputReady;
	unsigned char len, bank;
	if (!ioBuffer.numBanks)return 0;  // No ext memory
	for(;;) {
		inputReady = ioStatus(err);
		if (*err != ERR_NONE)return 0;
		if (!inputReady) break;
		for (;inputReady;inputReady -= len) {
			len = inputReady < READBUFFERSIZE? inputReady : READBUFFERSIZE;
			bankTop = (ioBuffer.readBank != ioBuffer.writeBank) || (ioBuffer.writeBankAddr >= ioBuffer.readBankAddr) ? 0x8000: (ioBuffer.readBankAddr - ioBuffer.writeBankAddr - 1);
			len = ((unsigned short)ioBuffer.writeBankAddr + len <= bankTop)?  len : (bankTop - (unsigned short) ioBuffer.writeBankAddr);
			if (!len)break;
			ioRead(readBuffer, len, err);
			if (*err != ERR_NONE)return 0;
			copyReadBufferToBankTo = ioBuffer.writeBankAddr;
			copyReadBufferToBank(len | (ioBuffer.exVals[ioBuffer.writeBank]<< 8));
			ioBuffer.writeBankAddr += len;
			if ((unsigned short)ioBuffer.writeBankAddr >= 0x8000) {
				ioBuffer.writeBankAddr = (unsigned char *)0x4000;
				if (++ioBuffer.writeBank >= ioBuffer.numBanks) ioBuffer.writeBank = 0;
			}
		}
	}
	if  ((ioBuffer.writeBank == ioBuffer.readBank) &&
		  ((unsigned short) ioBuffer.readBankAddr <= (unsigned short) ioBuffer.writeBankAddr))
		return (unsigned short) ioBuffer.writeBankAddr - (unsigned short) ioBuffer.readBankAddr;
	outputReady = 0x8000 - (unsigned short) ioBuffer.readBankAddr;
	outputReady += (unsigned short) ioBuffer.writeBankAddr - 0x4000;
	for(bank = ioBuffer.readBank+1;;) {
		if (bank >= ioBuffer.numBanks)bank = 0;
		if (bank == ioBuffer.writeBank) break;
		if (outputReady >= 0xbfff)return 0xffff; // maxed out.  Return 0xffff for more than 65535 bytes held
		outputReady += 0x4000;
	}
	return outputReady;
}

// readBuffer should be outside of 0x4000 to 0x8000
void ioBufferRead(unsigned char *readBuffer, unsigned char len, unsigned char *err)
{
	unsigned char len1;
	if (len + (unsigned short) ioBuffer.readBankAddr > 0x8000) {
		len1 = 0x8000 -  (unsigned short) ioBuffer.readBankAddr;
		copyBankToBufferTo = readBuffer;
		copyBankToBufferFrom = ioBuffer.readBankAddr;
		copyBankToBuffer(len1 | (ioBuffer.exVals[ioBuffer.readBank]<< 8));
		ioBuffer.readBankAddr += len1;
		if ((unsigned short)ioBuffer.readBankAddr >= 0x8000) {
			ioBuffer.readBankAddr = (unsigned char *)0x4000;
			if (++ioBuffer.readBank >= ioBuffer.numBanks) ioBuffer.readBank = 0;
		}
		len -= len1;
		readBuffer += len1;
	}
	copyBankToBufferTo = readBuffer;
	copyBankToBufferFrom = ioBuffer.readBankAddr;
	copyBankToBuffer(len | (ioBuffer.exVals[ioBuffer.readBank]<< 8));
	ioBuffer.readBankAddr += len;
	if ((unsigned short)ioBuffer.readBankAddr >= 0x8000) {
		ioBuffer.readBankAddr = (unsigned char *)0x4000;
		if (++ioBuffer.readBank >= ioBuffer.numBanks) ioBuffer.readBank = 0;
	}
}
#endif