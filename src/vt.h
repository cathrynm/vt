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

// For characters 0-127
void processChar(unsigned char c);
// for characters 128+.  These are drawn.
void displayUtf8Char(unsigned char c, unsigned char attribute);
void processUndrawableChar();
void vtSendCursor(unsigned char cursor);
void vtSendPgUpDown(unsigned char key);
void vtSendCr(void);
void resetVt(void);
void processChars(unsigned char *s, unsigned char len);
void sendResponse(unsigned char *s, unsigned char len);
void fixCursor(void);

#endif