#ifndef SIO_H
#define SIO_H

unsigned char sioWrite(unsigned char *buffer, unsigned char bufLen, unsigned char device, unsigned char *err);
unsigned char sioRead(unsigned char *buffer, unsigned char bufLen, unsigned char device, unsigned char *err);
void sioClose(unsigned char device, unsigned char *err);
void sioOpen(unsigned char *fName, unsigned char fLen, unsigned char device, unsigned char aux1, unsigned char aux2, unsigned char *err);
void sioSpecial(unsigned char *fName, unsigned char fLen, unsigned char device, unsigned char special, unsigned char dStats, unsigned char aux1, unsigned char aux2, unsigned char *err);

#endif