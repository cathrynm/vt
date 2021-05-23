#ifndef IOBUFFER_H
#define IOBUFFER_H 1

void ioBufferInit(void);
unsigned short ioBufferStatus(unsigned char *err);
void ioBufferRead(unsigned char *readBuffer, unsigned char len, unsigned char *err);

#endif