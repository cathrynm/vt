#ifndef IO_H
#define IO_H

unsigned char readData(void);
void ioOpen(unsigned char *device, unsigned char deviceLen, unsigned char baudWordStop, unsigned char mon, unsigned char *err);
void ioClose(unsigned char *err);

#endif