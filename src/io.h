#ifndef IO_H
#define IO_H

#define IOCB_WRITEBITS 8
#define IOCB_READBITS 4
#define IOCB_DIRBITS 2
#define IOCB_LONGDIRECTORY 0x80
#define AOS_DIRECTORY 1

typedef struct {
	unsigned char baudWordStop, mon;
	unsigned char *user, *passwd;
} openIoStruct;

void readData(unsigned char *err);
void ioOpen(unsigned char *device, unsigned char deviceLen, openIoStruct *openIo, unsigned char *err);
void ioClose(unsigned char *err);
void sendIoResponse(unsigned char *s, unsigned char len, unsigned char *err);
unsigned short ioStatus(unsigned char *err);
void ioRead(unsigned char *data, unsigned short len, unsigned char *err);

#endif