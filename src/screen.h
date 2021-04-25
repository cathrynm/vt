#ifndef SCREEN_H
#define SCREEN_H 1

#define SCREENLINES 24

typedef struct {
	unsigned char screenWidth;
	unsigned char lineLength[SCREENLINES];
	unsigned char lineWidth;
	unsigned short lineTab[SCREENLINES];
}screenXStruct;

extern screenXStruct screenX;

void initScreen(void);
void cursorUpdate(unsigned char x, unsigned char y);
void drawCharAt(unsigned char c, unsigned char attribute, unsigned char color, unsigned char x, unsigned char y);
void drawClearCharsAt(unsigned char len, unsigned char x, unsigned char y, unsigned char color);
void drawClearLine(unsigned char y, unsigned char color);
void drawInsertLine(unsigned char y, unsigned char yBottom, unsigned char color);
void drawDeleteLine(unsigned char y, unsigned char yBottom, unsigned char color);
void drawInsertChar(unsigned char x, unsigned char y, unsigned char color);
void drawDeleteChar(unsigned char x, unsigned char y, unsigned char color);
void drawDarkLight(unsigned char val);
void drawBell(void);
unsigned char iocbErrUpdate(unsigned char iocb, unsigned char *oldErr);
void drawScrollScreenDown(unsigned char yTop, unsigned char yBottom);
void drawScrollScreen(unsigned char yTop, unsigned char yBottom);
void screenRestore(void);
void drawClearScreen(unsigned char color);
void flushBuffer(void);
void cursorHide(void);
void callEColonSpecial(unsigned char command, unsigned char aux1, unsigned char aux2);
void callEColonPutBytes(unsigned char *buf, unsigned char len);
void callEColonPutByte(unsigned char ch);

#endif