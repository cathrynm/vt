#include "main.h"

#define READBUFFERLEN 255
typedef struct {
	unsigned char readBuffer[READBUFFERLEN];
	unsigned char *deviceName;
	unsigned char deviceLen;
	unsigned char deviceType;
	unsigned char sioDevice;
} ioStruct;
ioStruct io;

unsigned char *processFilename(unsigned char *fName, unsigned char len, unsigned char *device, unsigned char **buff, unsigned char *err) {
  *buff = 0;
  if (!len || (*err != ERR_NONE)) {
    errUpdate(ERR_DEVICENOTEXIST, err);
    return fName;
  }
  if (islower(fName[0])) {
    *buff = malloc(len);
    if (! *buff) {
      errUpdate(ERR_OUTOFMEMORY, err);
      return fName;
    }
    memCopy(*buff, fName, len);
    fName = *buff;
    fName[0] = toupper(fName[0]);
  }
  switch(fName[0]) {
    case 'N': // Only N: goes to SIO
      *device = 0x70 + (isdigit(fName[1])? (fName[1] - '0'):1);
      break;
    default:
      *device = 0;
      break;
  }
  return fName;
}

void ioOpen(unsigned char *deviceName, unsigned char deviceLen, openIoStruct *openIo, unsigned char *err) {
	unsigned char *buff;
	io.deviceType = toupper((deviceLen > 0)? deviceName[0]: 0);
	switch(io.deviceType) {
		case 'R':
			serialOpen(deviceName, deviceLen, openIo->baudWordStop, openIo->mon, err);
			break;
		case 'N':
			deviceName = processFilename(deviceName, deviceLen, &io.sioDevice, &buff, err);
			if (*err == ERR_NONE) {
				sioStatus(io.sioDevice, err);
				OS.stack[0] = *err;
				*err = ERR_NONE;
				sioOpen(deviceName, deviceLen, io.sioDevice, IOCB_WRITEBITS|IOCB_READBITS, 0, err);
				OS.stack[1] = *err;
			}
			if (buff)free(buff);
			break;
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

}

unsigned short ioStatus(unsigned char *err) {
	switch(io.deviceType) {
		case 'R':
			return serialStatus(err);
		case 'N':
			return sioStatus(io.sioDevice, err);
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);
			break;
	}
}

void ioFlow(unsigned short inputReady, unsigned char *err) {
	switch(io.deviceType) {
		case 'R':
			serialFlow(inputReady, err);
			break;
		case 'N':
			break;
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);
			break;
	}
}

void ioRead(unsigned char *data, unsigned short len, unsigned char *err) {
	switch(io.deviceType) {
		case 'R':
			serialRead(data, len, err);
			break;
		case 'N':
			sioRead(data, len, io.sioDevice, err);
			break;
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);;
			break;
	}
}

void readData(unsigned char *err) {
	unsigned short n;
	unsigned short readLen, inputReady;
	for (;;) { // Just keep going until drained.
		inputReady = ioStatus(err);
		if (*err != ERR_NONE)break;
		ioFlow(inputReady, err);
		if (*err != ERR_NONE)break;
		if (!inputReady)break;
		for (;inputReady;) {
			readLen = inputReady < sizeof(io.readBuffer)? inputReady: sizeof(io.readBuffer);
			ioRead(io.readBuffer, readLen, err);
			if (*err != ERR_NONE) break;
			for (n = 0;n < readLen;n++) {
				decodeUtf8Char(io.readBuffer[n], err);
			}
			if (*err != ERR_NONE)break;
			inputReady -= readLen;
			if (anyKey())break;
		}
		if (*err != ERR_NONE)break;
		if (anyKey())break;
	}
}

void ioClose(unsigned char *err)
{
	switch(io.deviceType) {
		case 'R':
			serialClose(io.deviceName, io.deviceLen, err);
			break;
		case 'N':
			sioClose(io.sioDevice, err);
			break;
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);
			break;
	}
}

void sendIoResponse(unsigned char *s, unsigned char len, unsigned char *err)
{
	switch(io.deviceType) {
		case 'R':
			sendSerialResponse(s, len, err);
			break;
		case 'N':
			sioWrite(s, len, io.sioDevice, err);
			break;
		default:
			errUpdate(ERR_DEVICENOTEXIST, err);
			break;
	}
}