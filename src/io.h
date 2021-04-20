#ifndef IO_H
#define IO_H

#define IOCB_WRITEBITS 8
#define IOCB_READBITS 4
#define IOCB_DIRBITS 2
#define IOCB_LONGDIRECTORY 0x80
#define AOS_DIRECTORY 1

void readData(unsigned char *err);
void ioOpen(unsigned char *device, unsigned char deviceLen, unsigned char baudWordStop, unsigned char mon, unsigned char *err);
void ioClose(unsigned char *err);
void sendIoResponse(unsigned char *s, unsigned char len, unsigned char *err);

#endif