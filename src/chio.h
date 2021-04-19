#ifndef CHIO_H
#define CHIO_H

#define ERRCHAR ' '
#define ERRATTRIB 0x80

// converts UTF8 16-bit to visible. 
void convertShortToVisibleChar(unsigned short c, unsigned char *ch, unsigned char *attrib);
void convertAsciiToVisibleChar(unsigned char *ch, unsigned char *attrib);
void decodeUtf8Char(unsigned char c);
unsigned char initChio(void);
unsigned char handleInput(void);
unsigned char closeChio(void);
unsigned char isXep80(void);
unsigned char isIntl(void);
void setFullAscii(unsigned char fullAscii);
unsigned char errUpdate(unsigned char err, unsigned char *oldErr);
unsigned char iocbErrUpdate(unsigned char iocb, unsigned char *oldErr);
void memCopy(unsigned char *top, const unsigned char *fromp, unsigned short len);
void memClear(unsigned char *top, unsigned short len);

extern unsigned char *eColon;
extern unsigned char clearScreenChar;

#endif