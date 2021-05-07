#ifndef SERIAL_H
#define SERIAL_H 1

#define IOCB_WRITEBIT 8
#define IOCB_READBIT 4
#define IOCB_DIRBITS 2
#define IOCB_CONCURRENTBIT 1

#define IOCB_FLUSH 32
#define IOCB_CONTROLLINES  34
#define IOCB_BAUDMON 36
#define IOCB_XLAT 38
#define IOCB_CONCURRENT 40

// XIO36 aux1
#define BAUD_300 0
#define BAUD_45_5 1
#define BAUD_50 2
#define BAUD_56_875 3
#define BAUD_75 4
#define BAUD_110 5
#define BAUD_134_5 6
#define BAUD_150 7
#define BAUD_300A 8
#define BAUD_900 9
#define BAUD_1200 10
#define BAUD_1800 11
#define BAUD_2400 12
#define BAUD_4800 13
#define BAUD_9600 14
#define BAUD_19200 15
#define BAUD_230400 0x40
#define BAUD_57600 1
#define BAUD_115200 3
#define BAUD_NONE 255
#define RWORDSIZE_8 0
#define RWORDSIZE_7 16
#define RWORDSIZE_6 32
#define RWORDSIZE_5 48
#define RSTOP_1 0
#define RSTOP_2 128

//xio36 aux2
#define RMON_CRX 1
#define RMON_CTS 2
#define RMON_DSR 4

// XIO34 aux1
#define RMON_DTR_SAME 0
#define RMON_DTR_OFF 128
#define RMON_DTR_ON 192
#define RMON_RTS_SAME 0
#define RMON_RTS_OFF 32
#define RMON_RTS_ON 48

// XIO38 aux1
#define RXLAT_LIGHTXLAT 0
#define RXLAT_HEAVYXLAT 16
#define RXLAT_NOXLAT 32

#define RXLAT_IGNORPAR 0
#define RXLAT_ODDCHECK 4
#define RXLAT_EVENCHECK 8
#define RXLAT_IGNORCLEARPAR 12

#define RXLAT_NOCHGPAR 0
#define RXLAT_SETPARODD 1
#define RXLAT_SETPAREVEN 2
#define RXLAT_SETPARITY1 3

#define RXLAT_NOLF 0
#define RXLAT_APPENDLF 64

void sendSerialResponse(unsigned char *s, unsigned char len, unsigned char *err);
void serialOpen(unsigned char *device, unsigned char deviceLen, unsigned char baudWordStop, unsigned char mon, unsigned char *err);
void serialClose(unsigned char *device, unsigned char deviceLen, unsigned char *err);
unsigned short getBaud(void);
unsigned char getBits(void);
unsigned char getParity(void);
unsigned short serialStatus(unsigned char *err);
void serialRead(unsigned char *data, unsigned short len, unsigned char *err);
unsigned char stringToBaud(unsigned char *s);

#endif