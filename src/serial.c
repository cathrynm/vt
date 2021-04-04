#include "main.h"



#define cio(iocb)			\
	(__AX__ = (iocb),		\
	__asm__ ("asl"),		\
	__asm__ ("asl"),		\
	__asm__ ("asl"),		\
	__asm__ ("asl"),		\
	__asm__ ("tax"),		\
	__asm__ ("jsr $e456"),	\
	__asm__ ("tya"),        \
	__asm__ ("ldx #0"),   \
	__AX__)



typedef struct serialDataStruct serialStruct;
#define RBUFFERSIZE 255
struct serialDataStruct {
	unsigned char buffer[RBUFFERSIZE];
	unsigned char readBuffer[RBUFFERSIZE];
};

serialStruct serial;


unsigned char controlLines(unsigned char *device, unsigned char deviceLen, unsigned char aux1) {
	unsigned char err = ERR_NONE;
	OS.iocb[3].buffer = device;
	OS.iocb[3].buflen = deviceLen;
	OS.iocb[3].aux1 = aux1;
	OS.iocb[3].aux2 = 0;
	OS.iocb[3].command = IOCB_CONTROLLINES;
	cio(3);
	iocbErrUpdate(3, &err);
	return err;
}

unsigned char serialXlat(unsigned char *device, unsigned char deviceLen, unsigned char aux1, unsigned char wontTranslate)
{
	unsigned char err = ERR_NONE;
	OS.iocb[3].buffer = device;
	OS.iocb[3].buflen = deviceLen;
	OS.iocb[3].aux1 = aux1;
	OS.iocb[3].aux2 = wontTranslate;
	OS.iocb[3].command = IOCB_XLAT;
	cio(3);
	iocbErrUpdate(3, &err);
	return err;
}


unsigned char serialFlush(unsigned char *device, unsigned char deviceLen)
{
	unsigned char err = ERR_NONE;
	OS.iocb[3].buffer = device;
	OS.iocb[3].buflen = deviceLen;
	OS.iocb[3].aux1 = 0;
	OS.iocb[3].aux2 = 0;
	OS.iocb[3].command = IOCB_FLUSH;
	cio(3);
	iocbErrUpdate(3, &err);
	return err;
}

unsigned char serialClose(unsigned char *device, unsigned char deviceLen)
{
	unsigned char err = ERR_NONE; // I think not required to flush before close.
	OS.iocb[3].buffer = device;
	OS.iocb[3].buflen = deviceLen;
	OS.iocb[3].aux1 = 0;
	OS.iocb[3].aux2 = 0;
	OS.iocb[3].command = IOCB_CLOSE;
	cio(3);
	iocbErrUpdate(3, &err);
	return err;
}


unsigned char serialOpen(unsigned char *device, unsigned char deviceLen, unsigned char baudWordStop, unsigned char mon)
{
	unsigned char err = ERR_NONE;
	OS.iocb[3].buffer = device;
	OS.iocb[3].buflen = deviceLen;
	OS.iocb[3].aux1 = baudWordStop;
	OS.iocb[3].aux2 = mon;
	OS.iocb[3].command = IOCB_BAUDMON;
	cio(3);
	iocbErrUpdate(3, &err);
	if (err != ERR_NONE)return err;
	err = serialXlat(device, deviceLen, RXLAT_NOXLAT, 0);
	if (err != ERR_NONE)return err;
	OS.iocb[3].buffer = device;
	OS.iocb[3].buflen = deviceLen;
	OS.iocb[3].aux1 = IOCB_READBIT|IOCB_WRITEBIT|IOCB_CONCURRENTBIT;
	OS.iocb[3].aux2 = 0;
	OS.iocb[3].command = IOCB_OPEN;
	cio(3);
	iocbErrUpdate(3, &err);
	if (err != ERR_NONE)return err;

	OS.iocb[3].buffer = serial.buffer;
	OS.iocb[3].buflen = RBUFFERSIZE;
	OS.iocb[3].aux1 = IOCB_READBIT|IOCB_WRITEBIT|IOCB_CONCURRENTBIT;
	OS.iocb[3].aux2 = 0;
	OS.iocb[3].command = IOCB_CONCURRENT;
	cio(3);
	iocbErrUpdate(3, &err);
	return err;
}


unsigned char readData(void) {
	unsigned char err = ERR_NONE, n;
	unsigned short readLen, inputReady;unsigned char outputWaiting;
	for (;;) { // Just keep going until drained.
		OS.iocb[3].command = IOCB_STATIS;
		cio(3);
		iocbErrUpdate(3, &err);
		if (err != ERR_NONE) break;;
		inputReady = * (unsigned short *) &OS.dvstat[1];
		outputWaiting = OS.dvstat[3];
		if (!inputReady) break;

		readLen = inputReady < sizeof(serial.readBuffer)? inputReady: sizeof(serial.readBuffer);
		OS.iocb[3].buffer = serial.readBuffer;
		OS.iocb[3].buflen = readLen;
		OS.iocb[3].command = IOCB_GETCHR;
		cio(3);
		iocbErrUpdate(3, &err);
		if (err != ERR_NONE) break;
		for (n = 0;n < readLen;n++) {
			decodeUtf8Char(serial.readBuffer[n]);
		} 
	}
	return err;
}


void sendSerialResponse(unsigned char *s, unsigned char len)
{
	unsigned char err = ERR_NONE;
	OS.iocb[3].buffer = s;
	OS.iocb[3].buflen = len;
	OS.iocb[3].command = IOCB_PUTCHR;
	cio(3);
	iocbErrUpdate(3, &err);
}