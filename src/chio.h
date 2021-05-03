#ifndef CHIO_H
#define CHIO_H

#define ERRCHAR 0x7f
#define ERRATTRIB 0

#define CH_INSCHR 0xff
#define ERR_NONE 1
#define ERR_LASTDATA 3
#define ERR_OUTOFBUFFERS 4
#define ERR_OUTOFMEMORY 158
#define ERR_BREAK 128 // BREAK occurred during I/O
#define ERR_ALREADYOPEN 129 // IOCB already open
#define ERR_DEVICENOTEXIST 130 // Specified device does not exist
#define ERR_READWRITEONLY 131 // Attempted to read a write-only device
#define ERR_INVALIDIO 132 // Invalid I/O command
#define ERR_NOTOPEN 133 // File or device is not open
#define ERR_INVALIDIOCB 134 // Invalid IOCB number
#define ERR_WRITEREADONLY 135 // Attempted to write to a read-only device
#define ERR_ENDOFFILE 136 // End of file
#define ERR_TRUNCATED 137 // Truncated Record: tried to read a record longer than allowed
#define ERR_TIMEOUT 138 // Device Timeout: Device did not respond to I/O commands
#define ERR_NAK 139 // Device NAK: I/O error or faulty device
#define ERR_FRAMING 140 // Serial bus input framing error
#define ERR_CURSOR 141 // Cursor exceeded range of graphics mode
#define ERR_SERIALOVERRUN 142 // Serial bus data frame overrun
#define ERR_CHECKSUM 143 // Serial bus data frame checksum error
#define ERR_DEVICEDONE 144 // Device done error, bad sector, or write-protected disk
#define ERR_COMPARE 145 // Read after write compare error
#define ERR_NOTIMPLEMENTED 146 // Function not implemented in handler

// converts UTF8 16-bit to visible. 
void convertShortToVisibleChar(unsigned short c, unsigned char *ch, unsigned char *attrib);
void convertAsciiToVisibleChar(unsigned char *ch, unsigned char *attrib);
void processChar(unsigned char c, unsigned char *err);
unsigned char initChio(void);
void handleInput(unsigned char *err);
unsigned char closeChio(void);
unsigned char isIntl(void);
void setFullAscii(unsigned char fullAscii);
unsigned char errUpdate(unsigned char err, unsigned char *oldErr);
unsigned char iocbErrUpdate(unsigned char iocb, unsigned char *oldErr);
void memCopy(unsigned char *top, const unsigned char *fromp, unsigned short len);
void memClear(unsigned char *top, unsigned short len);
void decodeUtf8Char(unsigned char c, unsigned char *err);
unsigned char anyKey(void);
unsigned char getline(unsigned char *buf, unsigned char len, unsigned char *err);
void drawString(unsigned char *s);
void drawChar(unsigned char ch);
void initAscii(unsigned char fontBase, void(*copyChar)(unsigned char, unsigned char *));
void enableInterrupt(void);
unsigned char proceedReady(void);
void doneProceeed(void);
void closeInterrupt(void);

extern unsigned char *eColon;
extern unsigned char clearScreenChar;

#endif