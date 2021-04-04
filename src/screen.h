#ifndef SCREEN_H
#define SCREEN_H 1

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

#endif