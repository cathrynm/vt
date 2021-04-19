#ifndef MAIN_H
#define MAIN_H 1

#include <atari.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "screen.h"
#include "vt.h"
#include "chio.h"
#include "serial.h"
#include "sio.h"
#include "io.h"

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

extern unsigned char *_STARTADDRESS__; // Bottom address of code.  Memory bewween OS.memlo and here is free
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























#endif