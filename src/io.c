#include "main.h"

typedef struct ioDataStruct ioStruct;
#define READBUFFERLEN 255
struct ioDataStruct {
	unsigned char readBuffer[READBUFFERLEN];
	unsigned char *deviceName;
	unsigned char deviceLen;
	unsigned char deviceType;
};
ioStruct io;

void ioOpen(unsigned char *device, unsigned char deviceLen, unsigned char baudWordStop, unsigned char mon, unsigned char *err) {
	io.deviceType = (deviceLen > 0)? device[0]: 0;
	if (io.deviceType == 'R') {
		serialOpen(device, deviceLen, baudWordStop, mon, err);
	} else {
		*err = ERR_DEVICENOTEXIST;
	}
	if (*err == ERR_NONE) {
		io.deviceName = malloc(deviceLen);
		memcpy(io.deviceName, device, deviceLen);
		io.deviceLen = deviceLen;
	} else {
		io.deviceType = 0;
	}

}

unsigned char readData(void) {
	unsigned char err = ERR_NONE;
	unsigned short n;
	unsigned short readLen, inputReady;
	for (;;) { // Just keep going until drained.
		inputReady = serialStatus(&err);
		serialFlow(inputReady, &err);

		if (!inputReady) break;
		readLen = inputReady < sizeof(io.readBuffer)? inputReady: sizeof(io.readBuffer);
		serialRead(io.readBuffer, readLen, &err);
		if (err != ERR_NONE) break;
		for (n = 0;n < readLen;n++) {
			decodeUtf8Char(io.readBuffer[n]);
		}
	}
	return err;
}

void ioClose(unsigned char *err)
{
	switch(io.deviceType) {
		case 'R':
			serialClose(io.deviceName, io.deviceLen, err);
			break;
		default:
			*err = ERR_DEVICENOTEXIST;
			break;
	}
}
