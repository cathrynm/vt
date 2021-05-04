#ifndef RAWCON_H
#define RAWCON_H 1

typedef struct {
	unsigned char numEntries;
	void __fastcall__ (*enable)(void);
	void __fastcall__ (*disable)(void);
	void __fastcall__ (*tempShutoff)(void);
	void __fastcall__ (*reActivate)(void);
	void __fastcall__ (*clearScreen)(void);
	void __fastcall__ (*reloadColors)(void);
	unsigned char __fastcall__ (*getChar)(void);
	void __fastcall__ (*putChar)(unsigned char c);
	void __fastcall__ (*xio)(void);
} rawTabStruct;

unsigned char rawConTest(void);
void drawCharsAtRawCon(unsigned char *buffer, unsigned char bufferLen);
void initRawCon(void);
void deleteLineRawCon(unsigned char topY, unsigned char bottomY);
void insertLineRawCon(unsigned char topY, unsigned char bottomY);
unsigned char supportsCharacterSet(void);
void cursorHideRawCon(void);
void cursorUpdateRawCon(unsigned char x, unsigned char y);
void insertCharRawCon(unsigned char x, unsigned char y, unsigned char len);
void deleteCharRawCon(unsigned char x, unsigned char y, unsigned char len);
void restoreRawCon(void);
void callIocb6(unsigned char command, unsigned char aux1, unsigned char aux2, unsigned char *err);

#endif