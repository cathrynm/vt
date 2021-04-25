#ifndef VT_H
#define VT_H 1
// character attributes
#define SGR_NORMAL 0
#define SGR_BOLD 1
#define SGR_UNDERLINE 4
#define SGR_BLINK 5
#define SGR_REVERSE 7
#define SGR_INVISIBLE 8
#define SGR_BOLDOFF 22
#define SGR_UNDERLINEOFF 24
#define SGR_BLINKOFF 25
#define SGR_REVERSEOFF 27
#define SGR_INVISBLEOFF 28
#define SGR_FBLACK 30
#define SGR_FRED 31
#define SGR_FGREEN 32
#define SGR_FYELLOW 33
#define SGR_FBLUE 34
#define SGR_FMAGENTA 35
#define SGR_FCYAN 36
#define SGR_FWHITE 37
#define SGR_BBLACK 40
#define SGR_BRED 41
#define SGR_BGREEN 42
#define SGR_BYELLOW 43
#define SGR_BBLUE 44
#define SGR_BMAGENTA 45
#define SGR_BCYAN 46
#define SGR_BWHITE 47


#define DEFAULTCOLOR 7

// For characters 0-127
void processChar(unsigned char c, unsigned char *err);
// for characters 128+.  These are drawn.
void displayUtf8Char(unsigned char c, unsigned char attribute);
void processUndrawableChar();
void vtSendCursor(unsigned char cursor, unsigned char *err);
void vtSendPgUpDown(unsigned char key, unsigned char *err);
void vtSendCr(unsigned char *err);
void resetVt(void);
void processChars(unsigned char *s, unsigned char len, unsigned char *err);
void sendResponse(unsigned char *s, unsigned char len, unsigned char *err);
void fixCursor(void);
unsigned char charToA(unsigned char v, unsigned char *s, unsigned char m);

#endif