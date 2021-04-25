#include "main.h"
#define SIO_TIMEOUT 5

#define sio()     \
  (   \
  __asm__ ("jsr $e459") \
  )

unsigned char dcbErrUpdate(unsigned char *oldErr)
{
  return errUpdate(OS.dcb.dstats, oldErr);
}

void bufferFixDone(unsigned char *data, unsigned char len, unsigned short outLen, unsigned char dStats, unsigned char **buff, unsigned char err) 
{
  if (!(dStats & (SIO_WRITE | SIO_READ)))return;
  if ((dStats & SIO_READ) && (err != ERR_NONE)) {
    if (*buff)memCopy(data, *buff, (len <= outLen)? len: outLen);
    if (outLen < len) memClear(&data[outLen], len - outLen);
  }
  if (*buff)free(*buff);
}

// Work around irritating SIO issue. SIO barfs if last character is on 0xff boundary.
unsigned char *bufferFix(unsigned char *data, unsigned char len, unsigned short outLen, unsigned char dStats, unsigned char **buff, unsigned char *err)
{
  unsigned char *buffer;
  *buff = NULL;
  if (!(dStats & (SIO_WRITE | SIO_READ)))return data;
  if ((outLen <= len) && ((((unsigned char) data +  outLen + (unsigned char)0xff) & 0xff) != 0xff))return data;
  *buff = malloc(outLen + 1);
  if (!(*buff)) {
    errUpdate(ERR_OUTOFMEMORY, err);
    return NULL;
  }
  buffer = *buff;
  if ((((unsigned char) buffer + outLen + (unsigned char)0xff) & 0xff) == 0xff) buffer++;
  if (dStats & SIO_WRITE) {
    memCopy(buffer, data, (len <= outLen)? len: outLen);
    if (outLen > len) 
      memClear(&buffer[len], outLen - len);
  }
  return buffer;
}

void extendedError(unsigned char device, unsigned char *err)
{
  if (*err != ERR_DEVICEDONE) return;
  OS.dvstat[3] = ERR_NONE;
  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'S';
  OS.dcb.dstats = SIO_READ;
  OS.dcb.dbuf = &OS.dvstat;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = 4;
  OS.dcb.daux1 = 0;
  OS.dcb.daux2 = 0;
  sio();
  if (OS.dvstat[3] > ERR_NONE) *err = OS.dvstat[3];
}

void sioOpen(unsigned char *fName, unsigned char fLen, unsigned char device, unsigned char aux1, unsigned char aux2, unsigned char *err)
{
  unsigned char *buff;
  fName = bufferFix(fName, fLen, 256, SIO_WRITE, &buff, err);
  if (*err != ERR_NONE)return;

  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'O';
  OS.dcb.dstats = SIO_WRITE;
  OS.dcb.dbuf = fName;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = 256;
  OS.dcb.daux1 = aux1;
  OS.dcb.daux2 = aux2;
  sio();
  dcbErrUpdate(err);
  if (buff)free(buff);
  if (*err == ERR_DEVICEDONE)extendedError(device, err);
}

void sioSpecial(unsigned char *fName, unsigned char fLen, unsigned char device, unsigned char special, unsigned char dStats, unsigned char aux1, unsigned char aux2, unsigned char *err)
{
  unsigned char *buff;
  fName = bufferFix(fName, fLen, 256, dStats, &buff, err);
  if (*err != ERR_NONE)return;
  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = special;
  OS.dcb.dstats = dStats;
  if ((dStats & (SIO_WRITE | SIO_READ)) != 0) {
    OS.dcb.dbuf = fName;
    OS.dcb.dbyt = 256;
  } else {
    OS.dcb.dbuf = NULL;
    OS.dcb.dbyt = 0;
  }
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.daux1 = aux1;
  OS.dcb.daux2 = aux2;
  sio();
  dcbErrUpdate(err);
  bufferFixDone(fName, fLen, 256, dStats, &buff, *err);
  if (*err == ERR_DEVICEDONE)extendedError(device, err);
}

void sioClose(unsigned char device, unsigned char *err)
{
  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'C';
  OS.dcb.dstats = 0x00;
  OS.dcb.dbuf = NULL;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = 0;
  OS.dcb.daux1 = 0;
  OS.dcb.daux2 = 0;
  sio();
  dcbErrUpdate(err);
  if (*err == ERR_DEVICEDONE)extendedError(device, err);
}


unsigned char sioRead(unsigned char *buffer, unsigned char bufLen, unsigned char device, unsigned char *err) {
  unsigned char *buff;
  unsigned char *aBuffer = NULL;
  aBuffer = bufferFix(buffer, bufLen, bufLen, SIO_READ, &buff, err);
  if (*err != ERR_NONE)return 0;

  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'R';
  OS.dcb.dstats = SIO_READ;
  OS.dcb.dbuf = aBuffer;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = bufLen;
  OS.dcb.daux1 = bufLen;
  OS.dcb.daux2 = 0;
  sio();
  dcbErrUpdate(err);
  bufferFixDone(buffer, bufLen, bufLen, SIO_READ, &buff, *err);
  if (*err == ERR_DEVICEDONE)extendedError(device, err);
  return (*err == ERR_NONE)?bufLen:0;
}

unsigned char sioWrite(unsigned char *buffer, unsigned char bufLen, unsigned char device, unsigned char *err) {
  unsigned char *buff;
  buffer = bufferFix(buffer,bufLen, bufLen, 1, &buff, err);
  if (*err != ERR_NONE)return 0;
  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'W';
  OS.dcb.dstats = SIO_WRITE;
  OS.dcb.dbuf = buffer;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = bufLen;
  OS.dcb.daux1 = bufLen;
  OS.dcb.daux2 = 0;
  sio();
  dcbErrUpdate(err);
  if (*buff) free(buff);
  if (*err == ERR_DEVICEDONE)extendedError(device, err);
  return (*err == ERR_NONE)?bufLen:0;
}

unsigned short sioStatus(unsigned char device, unsigned char *err) {
  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'S';
  OS.dcb.dstats = SIO_READ;
  OS.dcb.dbuf = &OS.dvstat;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = 4;
  OS.dcb.daux1 = 0;
  OS.dcb.daux2 = 0;
  sio();
  dcbErrUpdate(err);
  if (*err != ERR_NONE)return 0;
  if (OS.dvstat[3]) *err = OS.dvstat[3]; // dvstat[3] is error for Fujinet and daux2 = 0
  return * (unsigned short *) &OS.dvstat[0];
}