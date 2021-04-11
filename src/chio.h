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
unsigned char isXep80Internal(void);

extern unsigned char *eColon;

#endif