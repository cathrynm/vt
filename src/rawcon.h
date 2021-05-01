#ifndef RAWCON_H
#define RAWCON_H 1

unsigned char rawConTest(void);
void drawCharsAtRawCon(unsigned char *buffer, unsigned char bufferLen);
void initRawCon(void);
void deleteLineRawCon(unsigned char topY, unsigned char bottomY);
void insertLineRawCon(unsigned char topY, unsigned char bottomY);

#endif