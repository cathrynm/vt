#include "main.h"


typedef struct {
	unsigned char *deviceName;
	unsigned char deviceLen;
	unsigned char deviceType;
#if FUJINET_ON
	unsigned char sioDevice;
#endif
#if FLOWXONOFF
	unsigned char pause;
#endif
} ioStruct;
ioStruct io;



void ioOpen(unsigned char *deviceName, unsigned char deviceLen, openIoStruct *openIo, unsigned char *err) {
#if FUJINET_ON
	unsigned char *buff;
#endif
	io.deviceType = toupper((deviceLen > 0)? deviceName[0]: 0);
	switch(io.deviceType) {
#if SERIAL_ON
		case 'R':
			serialOpen(deviceName, deviceLen, openIo->baudWordStop, openIo->mon, err);
			break;
#endif
#if FUJINET_ON
		case 'N':
			deviceName = processFilename(deviceName, deviceLen, &io.sioDevice, &buff, err);
			if ((*err == ERR_NONE)  && openIo->user)
				sioSpecial(openIo->user, strlen(openIo->user) + 1, io.sioDevice, SIOCOMMAND_FUJIUSER, SIO_WRITE, 0, 0, err);
			if ((*err == ERR_NONE)  && openIo->passwd)
				sioSpecial(openIo->passwd, strlen(openIo->passwd) + 1, io.sioDevice, SIOCOMMAND_FUJIPASSWORD, SIO_WRITE, 0, 0, err);
			if (*err == ERR_NONE) 
				sioOpen(deviceName, deviceLen, io.sioDevice, IOCB_WRITEBITS|IOCB_READBITS, 0, err);
			if (*err == ERR_NONE) enableInterrupt();
			if (buff)free(buff);
			break;
#endif
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);
			break;
	}
	if (*err == ERR_NONE) {
		io.deviceName = malloc(deviceLen);
		memcpy(io.deviceName, deviceName, deviceLen);
		io.deviceLen = deviceLen;
	} else {
		io.deviceType = 0;
	}
#if FLOWXONOFF
	io.pause = 0;
#endif
#if IOBUFFER
	ioBufferInit();
#endif
}

unsigned short ioStatus(unsigned char *err) {
	switch(io.deviceType) {
#if SERIAL_ON
		case 'R':
			return serialStatus(err);
#endif
#if FUJINET_ON
		case 'N':
			return sioStatus(io.sioDevice, err);
#endif
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);
			break;
	}
}

void ioRead(unsigned char *data, unsigned short len, unsigned char *err) {
	switch(io.deviceType) {
#if SERIAL_ON
		case 'R':
			serialRead(data, len, err);
			break;
#endif
#if FUJINET_ON
		case 'N':
			sioRead(data, len, io.sioDevice, err);
			break;
#endif
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);;
			break;
	}
}

void readData(unsigned char *err) {
	static unsigned char ctrlS = 19, ctrlQ = 17;
	unsigned short n;
	unsigned short inputReady;

	if (!proceedReady())return;
	for (;;) { // Just keep going until drained.
#if IOBUFFER
		inputReady = ioBufferStatus(err);
		if (*err != ERR_NONE) break;
		if (inputReady) {
			inputReady = inputReady <= READBUFFERSIZE ? inputReady: READBUFFERSIZE;
			ioBufferRead(readBuffer, inputReady, err);
			if (*err != ERR_NONE) break;
		} 
#else
		inputReady = 0;
#endif
		if (!inputReady) {
			inputReady = ioStatus(err);
			if (*err != ERR_NONE) break;
			if (!inputReady) {
#if FLOWXONOFF
				if (io.pause) {
					io.pause = 0;
					sendIoResponse(&ctrlQ, 1, err);
				}
#endif
				break;
			}
			inputReady = inputReady <= READBUFFERSIZE ? inputReady: READBUFFERSIZE;
			ioRead(readBuffer, inputReady, err);
			if (*err != ERR_NONE) break;
		}
#if FLOWXONOFF
		if (!io.pause && (inputReady >= READBUFFERSIZE)) {
			io.pause = 1;
			sendIoResponse(&ctrlS, 1, err);
			if (*err != ERR_NONE) break;
		}
#endif
		for (n = 0;n < inputReady && (*err == ERR_NONE);n++) {
			decodeUtf8Char(readBuffer[n], err);
		}
		if (*err != ERR_NONE)break;
		if (anyKey())break;
	}
	doneProceeed();
}

void ioClose(unsigned char *err)
{
	switch(io.deviceType) {
#if SERIAL_ON
		case 'R':
			serialClose(io.deviceName, io.deviceLen, err);
			break;
#endif
#if FUJINET_ON
		case 'N':
			sioClose(io.sioDevice, err);
			closeInterrupt();
			break;
#endif
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);
			break;
	}
}

void sendIoResponse(unsigned char *s, unsigned char len, unsigned char *err)
{
	switch(io.deviceType) {
#if SERIAL_ON
		case 'R':
			sendSerialResponse(s, len, err);
			break;
#endif
#if FUJINET_ON
		case 'N':
			sioWrite(s, len, io.sioDevice, err);
			break;
#endif
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);
			break;
	}
}