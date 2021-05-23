extern void ih(void);
extern unsigned char trip;
extern unsigned char jfsymbol_memoryIndex;
void * __fastcall__ jfsymbol(unsigned char *name);

void __fastcall__ writeVBXE(unsigned short bankLen);
unsigned char __fastcall__ readVBXE(unsigned char bank);

extern unsigned char vbxeData[160];
extern void *vbxeAddr, *vbxeReadAddr;
extern void *memacReg;
extern unsigned char SHFLOK_save;
extern unsigned char LMARGN_save;
extern unsigned char ASMEND[1]; // VBXE Swap area must be above this address
#define READBUFFERSIZE 255
extern unsigned char readBuffer[READBUFFERSIZE];

extern unsigned char *copyBankToBufferFrom;
extern unsigned char *copyBankToBufferTo;
void __fastcall__ copyBankToBuffer(unsigned short bankLen);
extern unsigned char *copyReadBufferToBankTo;
void __fastcall__ copyReadBufferToBank(unsigned short bankLen);