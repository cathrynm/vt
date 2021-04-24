#ifndef DIRECT_H
#define DIRECT_H 1

void drawCharsAtDirect(unsigned char *s, unsigned char len);
void deleteLineDirect(unsigned char topY, unsigned char bottomY);
void insertLineDirect(unsigned char topY, unsigned char bottomY);
unsigned char directDrawTest(void);
void initDirect(void);

extern unsigned char sAtascii[4];
#endif