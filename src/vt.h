#ifndef VT_H
#define VT_H 1

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