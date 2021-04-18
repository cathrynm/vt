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
#define RBUFFERSIZE 0x2000
#define READBUFFERLEN 255
struct serialDataStruct {
	unsigned char *buffer;
	unsigned char readBuffer[READBUFFERLEN];
	unsigned char baudWordStop, xlat;
	unsigned char rts;
};

serialStruct serial;

unsigned short getBaud(void)
{
	switch(serial.baudWordStop & 0xf) {
        case BAUD_300: return 300;
        case BAUD_45_5: return 45;
        case BAUD_50: return 50;
        case BAUD_56_875: return 56;
        case BAUD_75: return 75;
        case BAUD_110: return 110;
        case BAUD_134_5: return 134;
        case BAUD_150:return 150;
        case BAUD_300A:return 300;
        case BAUD_900:return 900;
        case BAUD_1200:return 1200;
        case BAUD_1800:return 1800;
        case BAUD_2400:return 2400;
        case BAUD_4800:return 4800;
        case BAUD_9600:return 9600;
        case BAUD_19200:return 19200;
        default: return 0;
	}
}

unsigned char getBits(void)
{
	switch(serial.baudWordStop & 0x30) {
		case RWORDSIZE_8: return 8;
		case RWORDSIZE_7: return 7;
		case RWORDSIZE_6: return 6;
		case RWORDSIZE_5: return 5;
		default: return 0;
	}
}

unsigned char getParity(void)
{
	return serial.xlat & 3;
}

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
	serial.xlat = aux1;
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
	if (serial.buffer) {
		free(serial.buffer);
		serial.buffer = NULL;
	}
	return err;
}


unsigned char serialOpen(unsigned char *device, unsigned char deviceLen, unsigned char baudWordStop, unsigned char mon)
{
	unsigned char err = ERR_NONE;
	serial.baudWordStop = baudWordStop;
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
	serial.buffer = malloc(RBUFFERSIZE);
	if (!serial.buffer) {
		return ERR_OUTOFMEMORY;
	}
	OS.iocb[3].buffer = serial.buffer;
	OS.iocb[3].buflen = RBUFFERSIZE;
	OS.iocb[3].aux1 = IOCB_READBIT|IOCB_WRITEBIT|IOCB_CONCURRENTBIT;
	OS.iocb[3].aux2 = 0;
	OS.iocb[3].command = IOCB_CONCURRENT;
	cio(3);
	iocbErrUpdate(3, &err);
	serial.rts = 1;
	return err;
}


unsigned char readData(void) {
	unsigned char err = ERR_NONE;
	unsigned short n;
	unsigned short readLen, inputReady;unsigned char outputWaiting;
	for (;;) { // Just keep going until drained.
		OS.iocb[3].command = IOCB_STATIS;
		cio(3);
		iocbErrUpdate(3, &err);
		if (err != ERR_NONE) break;;
		inputReady = * (unsigned short *) &OS.dvstat[1];
		outputWaiting = OS.dvstat[3];

		if (inputReady >= ((RBUFFERSIZE * 3) >> 2)) {
			if (serial.rts) {
				serial.rts = 0;
				sendSerialResponse("\023", 1);
			}
		} else {
			if (!serial.rts) {
				serial.rts = 1;
				sendSerialResponse("\021", 1);
			}
		}
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