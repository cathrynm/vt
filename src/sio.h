#ifndef SIO_H
#define SIO_H

#define SIO_WRITE 0x80
#define SIO_READ 0x40
#define SIOCOMMAND_FUJIUSER 0xfd
#define SIOCOMMAND_FUJIPASSWORD 0xfe

unsigned char sioWrite(unsigned char *buffer, unsigned char bufLen, unsigned char device, unsigned char *err);
unsigned char sioRead(unsigned char *buffer, unsigned short bufLen, unsigned char device, unsigned char *err);
void sioClose(unsigned char device, unsigned char *err);
void sioOpen(unsigned char *fName, unsigned char fLen, unsigned char device, unsigned char aux1, unsigned char aux2, unsigned char *err);
void sioSpecial(unsigned char *fName, unsigned char fLen, unsigned char device, unsigned char special, unsigned char dStats, unsigned char aux1, unsigned char aux2, unsigned char *err);
unsigned short sioStatus(unsigned char device, unsigned char *err);
unsigned char *processFilename(unsigned char *fName, unsigned char len, unsigned char *device, unsigned char **buff, unsigned char *err);

#endif