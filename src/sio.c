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
  if (!(dStats & (0x80 | 0x40)))return;
  if ((dStats & 0x40) && (err != ERR_NONE)) {
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
  if (!(dStats & (0x80 | 0x40)))return data;
  if ((outLen <= len) && ((((unsigned char) data +  outLen + (unsigned char)0xff) & 0xff) != 0xff))return data;
  *buff = malloc(outLen + 1);
  if (!(*buff)) {
    errUpdate(ERR_OUTOFMEMORY, err);
    return NULL;
  }
  buffer = *buff;
  if ((((unsigned char) buffer + outLen + (unsigned char)0xff) & 0xff) == 0xff) buffer++;
  if (dStats & 0x80) {
    memCopy(buffer, data, (len <= outLen)? len: outLen);
    if (outLen > len) 
      memClear(&buffer[len], outLen - len);
  }
  return buffer;
}

void sioOpen(unsigned char *fName, unsigned char fLen, unsigned char device, unsigned char aux1, unsigned char aux2, unsigned char *err)
{
  unsigned char *buff;
  fName = bufferFix(fName, fLen, 256, 0x80, &buff, err);
  if (*err != ERR_NONE)return;

  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'O';
  OS.dcb.dstats = 0x80;
  OS.dcb.dbuf = fName;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = 256;
  OS.dcb.daux1 = aux1;
  OS.dcb.daux2 = aux2;
  sio();
  dcbErrUpdate(err);
  if (buff)free(buff);
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
  if ((dStats & (0x80 | 0x40)) != 0) {
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
}

unsigned char sioRead(unsigned char *buffer, unsigned char bufLen, unsigned char device, unsigned char *err) {
  unsigned char *buff;
  unsigned char *aBuffer = NULL;
  aBuffer = bufferFix(buffer, bufLen, bufLen, 0x40, &buff, err);
  if (*err != ERR_NONE)return 0;

  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'R';
  OS.dcb.dstats = 0x40;
  OS.dcb.dbuf = aBuffer;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = bufLen;
  OS.dcb.daux1 = bufLen;
  OS.dcb.daux2 = 0;
  sio();
  dcbErrUpdate(err);
  bufferFixDone(buffer, bufLen, bufLen, 0x40, &buff, *err);
  return (*err == ERR_NONE)?bufLen:0;
}

unsigned char sioWrite(unsigned char *buffer, unsigned char bufLen, unsigned char device, unsigned char *err) {
  unsigned char *buff;
  buffer = bufferFix(buffer,bufLen, bufLen, 1, &buff, err);
  if (*err != ERR_NONE)return 0;
  OS.dcb.ddevic = device;
  OS.dcb.dunit = 1;
  OS.dcb.dcomnd = 'W';
  OS.dcb.dstats = 0x80;
  OS.dcb.dbuf = buffer;
  OS.dcb.dtimlo = SIO_TIMEOUT;
  OS.dcb.dbyt = bufLen;
  OS.dcb.daux1 = bufLen;
  OS.dcb.daux2 = 0;
  sio();
  dcbErrUpdate(err);
  if (*buff) free(buff);
  return (*err == ERR_NONE)?bufLen:0;
}

