#ifndef DIRECT_H
#define DIRECT_H 1

void writeScreen(unsigned char *s, unsigned char len, unsigned char x, unsigned char y);
void directScrollUp(unsigned char topY, unsigned char bottomY);
void directScrollDown(unsigned char topY, unsigned char bottomY);
unsigned char directDrawTest(void);
void initDirect(void);

#endif