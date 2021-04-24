#ifndef XEP80_H
#define XEP80_H 1

#define XEPCH_ATASCII 0xd4
#define XEPCH_ATINT 0xd5
#define XEPCH_INTERN 0xd6

#define XEP_SETCURSORHPOS 0
#define XEP_SETCURSORHPOSHI 0x50
#define XEP_SETCURSORVPOS	0x80
#define XEP_SETRIGHTMARGINLO 0xa0
#define XEP_SETRIGHTMARGINHI 0xb0
#define XEP_RESET			0xc2
#define XEP_FILLPREVCHAR	0xc4
#define XEP_FILLSPACE		0xc5
#define XEP_FILLEOL			0xc6
#define XEP_CURSOROFF		0xd8
#define XEP_CURSORON		0xd9
#define XEP_CURSORONBLINK		0xda
#define XEP_MOVETOLOGICALSTART 0xdb
#define XEP_SETSCROLLX 0xdc
#define XEP_SETREVERSEVIDEOOFF 0xde
#define XEP_SETREVERSEVIDEOON 0xdf
#define XEP_SETCUSSORADDR 0xe2
#define XEP_WRITEBYTE   	0xe3
#define XEP_SETEXTRABYTE	0xe4
#define XEP_WRITEINTERNALBYTE 0xe5
#define XEP_SETATTRLATCH0 0xf4
#define XEP_SETATTRLATCH1 0xf5
#define XEP_SETTCP 0xf6
#define XEP_WRITETCP 0xf7

unsigned char isXep80Internal(void);
void xepCursorShadow(void);
void initXep(void);
void restoreXep(void);
void clearScreenXep(void);
void deleteCharXep(unsigned char x, unsigned char y);
void insertCharXep(unsigned char x, unsigned char y);
unsigned char setXepCharSet(unsigned char which);
unsigned char isXep80Internal(void);
void cursorUpdateXep(unsigned char x, unsigned char y);
void drawCharsAtXep(unsigned char *buffer, unsigned char bufferLen);
void insertLineXep(unsigned char y, unsigned char yBottom);
void deleteLineXep(unsigned char y, unsigned char yBottom);
unsigned char XEP80Test(void);

#endif 