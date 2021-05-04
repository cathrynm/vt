
#ifndef VBXE_H
#define VBXE_H

#define VBXEBIOS_DETECT 96
#define VBXEBIOS_SCROLLUP 97
#define VBXEBIOS_SCROLLDOWN 98
#define VBXEBIOS_CLEARLINE 99
#define VBXEBIOS_PALETTERESET 100
#define VBXEBIOS_PALETTEWRITE 101
#define VBXEBIOS_PALETTEREAD 102
#define VBXEBIOS_FONTLOAD 103
#define VBXEBIOS_PUTCHAR 105
#define VBXEBIOS_GETXDL 107
#define VBXEBIOS_GETVCTRL 108
#define VBXEBIOS_ALLOC 109
#define VBXEBIOS_TEXTFAST 111
#define VBXEBIOS_TEXTFASTOFF 112
#define VBXEBIOS_PALETTELOADSWITHFONT 113
#define VBXEBIOS_GETCHARCELL 114

unsigned char vbxeTest(void);
void initVbxe(void);
void restoreVbxe(void);
void clearScreenVbxe(unsigned char color);
void cursorUpdateVbxe(unsigned char x, unsigned char y);
void drawCharsAtVbxe(unsigned char *buffer, unsigned char bufferLen);
void insertLineVbxe(unsigned char y, unsigned char yBottom, unsigned char color);
void deleteLineVbxe(unsigned char y, unsigned char yBottom, unsigned char color);
void insertCharVbxe(unsigned char x, unsigned char y, unsigned char color);
void deleteCharVbxe(unsigned char x, unsigned char y, unsigned char len, unsigned char color);
void cursorHideVbxe(void);

#endif