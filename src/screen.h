#ifndef SCREEN_H
#define SCREEN_H 1

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
#define XEP_WRITEBYTE   	0xe3
#define XEP_SETEXTRABYTE	0xe4
#define XEP_WRITEINTERNALBYTE 0xe5
#define XEP_SETATTRLATCH0 0xf4
#define XEP_SETATTRLATCH1 0xf5
#define XEP_SETTCP 0xf6
#define XEP_WRITETCP 0xf7

void initScreen(void);
void cursorUpdate(unsigned char x, unsigned char y);
void drawCharAt(unsigned char c, unsigned char attribute, unsigned char x, unsigned char y);
void drawClearCharsAt(unsigned char len, unsigned char x, unsigned char y);
void drawClearLine(unsigned char y);
void drawInsertLine(unsigned char y, unsigned char yBottom);
void drawDeleteLine(unsigned char y, unsigned char yBottom);
void drawInsertChar(unsigned char x, unsigned char y);
void drawDeleteChar(unsigned char x, unsigned char y);
void drawDarkLight(unsigned char val);
void drawBell(void);
unsigned char iocbErrUpdate(unsigned char iocb, unsigned char *oldErr);
void drawScrollScreenDown(unsigned char yTop, unsigned char yBottom);
void drawScrollScreen(unsigned char yTop, unsigned char yBottom);
void screenRestore(void);
void drawClearScreen(void);
void flushBuffer(void);
void cursorHide(void);
unsigned char setXepCharSet(unsigned char which);
void setXepRowPtr(unsigned char y, unsigned char val);
unsigned char isXep80Internal(void);
void setXEPLastChar(unsigned char c);
void setXEPCommand(unsigned char c, unsigned char command);
unsigned char isXep80Internal(void);

#endif