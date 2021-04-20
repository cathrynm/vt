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

#define IOCB_SERIAL 3
typedef struct serialDataStruct serialStruct;
#define RBUFFERSIZE 0x1000
struct serialDataStruct {
	unsigned char *buffer;
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
	OS.iocb[IOCB_SERIAL].buffer = device;
	OS.iocb[IOCB_SERIAL].buflen = deviceLen;
	OS.iocb[IOCB_SERIAL].aux1 = aux1;
	OS.iocb[IOCB_SERIAL].aux2 = 0;
	OS.iocb[IOCB_SERIAL].command = IOCB_CONTROLLINES;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, &err);
	return err;
}

void serialXlat(unsigned char *device, unsigned char deviceLen, unsigned char aux1, unsigned char wontTranslate, unsigned char *err)
{
	OS.iocb[IOCB_SERIAL].buffer = device;
	OS.iocb[IOCB_SERIAL].buflen = deviceLen;
	OS.iocb[IOCB_SERIAL].aux1 = aux1;
	OS.iocb[IOCB_SERIAL].aux2 = wontTranslate;
	OS.iocb[IOCB_SERIAL].command = IOCB_XLAT;
	serial.xlat = aux1;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, err);
}

unsigned char serialFlush(unsigned char *device, unsigned char deviceLen)
{
	unsigned char err = ERR_NONE;
	OS.iocb[IOCB_SERIAL].buffer = device;
	OS.iocb[IOCB_SERIAL].buflen = deviceLen;
	OS.iocb[IOCB_SERIAL].aux1 = 0;
	OS.iocb[IOCB_SERIAL].aux2 = 0;
	OS.iocb[IOCB_SERIAL].command = IOCB_FLUSH;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, &err);
	return err;
}

void serialClose(unsigned char *device, unsigned char deviceLen, unsigned char *err)
{
	OS.iocb[IOCB_SERIAL].buffer = device;
	OS.iocb[IOCB_SERIAL].buflen = deviceLen;
	OS.iocb[IOCB_SERIAL].aux1 = 0;
	OS.iocb[IOCB_SERIAL].aux2 = 0;
	OS.iocb[IOCB_SERIAL].command = IOCB_CLOSE;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, err);
	if (serial.buffer) {
		free(serial.buffer);
		serial.buffer = NULL;
	}
}


void serialOpen(unsigned char *device, unsigned char deviceLen, unsigned char baudWordStop, unsigned char mon, unsigned char *err)
{
	serial.baudWordStop = baudWordStop;
	OS.iocb[IOCB_SERIAL].buffer = device;
	OS.iocb[IOCB_SERIAL].buflen = deviceLen;
	OS.iocb[IOCB_SERIAL].aux1 = baudWordStop;
	OS.iocb[IOCB_SERIAL].aux2 = mon;
	OS.iocb[IOCB_SERIAL].command = IOCB_BAUDMON;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, err);
	if (*err != ERR_NONE)return;
	serialXlat(device, deviceLen, RXLAT_NOXLAT, 0, err);
	if (*err != ERR_NONE)return;
	OS.iocb[IOCB_SERIAL].buffer = device;
	OS.iocb[IOCB_SERIAL].buflen = deviceLen;
	OS.iocb[IOCB_SERIAL].aux1 = IOCB_READBIT|IOCB_WRITEBIT|IOCB_CONCURRENTBIT;
	OS.iocb[IOCB_SERIAL].aux2 = 0;
	OS.iocb[IOCB_SERIAL].command = IOCB_OPEN;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, err);
	if (*err != ERR_NONE)return;
	serial.buffer = malloc(RBUFFERSIZE);
	if (!serial.buffer) {
		*err = ERR_OUTOFMEMORY;
		serialClose(device, deviceLen, err);
		return;
	}
	OS.iocb[IOCB_SERIAL].buffer = serial.buffer;
	OS.iocb[IOCB_SERIAL].buflen = RBUFFERSIZE;
	OS.iocb[IOCB_SERIAL].aux1 = IOCB_READBIT|IOCB_WRITEBIT|IOCB_CONCURRENTBIT;
	OS.iocb[IOCB_SERIAL].aux2 = 0;
	OS.iocb[IOCB_SERIAL].command = IOCB_CONCURRENT;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, err);
	if (*err != ERR_NONE) {
		serialClose(device, deviceLen, err);
	}
	serial.rts = 1;
}

unsigned short serialStatus(unsigned char *err)
{
	OS.iocb[IOCB_SERIAL].command = IOCB_STATIS;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, err);
	if (*err != ERR_NONE) return 0;
	return * (unsigned short *) &OS.dvstat[1];
}

void serialFlow(unsigned short inputReady, unsigned char *err)
{		
	if (inputReady >= ((RBUFFERSIZE * 3) >> 2)) {
		if (serial.rts) {
			serial.rts = 0;
			sendSerialResponse("\023", 1, err);
		}
	} else {
		if (!serial.rts) {
			serial.rts = 1;
			sendSerialResponse("\021", 1, err);
		}
	}
}

void serialRead(unsigned char *data, unsigned short len, unsigned char *err)
{
	OS.iocb[IOCB_SERIAL].buffer = data;
	OS.iocb[IOCB_SERIAL].buflen = len;
	OS.iocb[IOCB_SERIAL].command = IOCB_GETCHR;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, err);
}

void sendSerialResponse(unsigned char *s, unsigned char len, unsigned char *err)
{
	OS.iocb[IOCB_SERIAL].buffer = s;
	OS.iocb[IOCB_SERIAL].buflen = len;
	OS.iocb[IOCB_SERIAL].command = IOCB_PUTCHR;
	cio(IOCB_SERIAL);
	iocbErrUpdate(IOCB_SERIAL, err);
}