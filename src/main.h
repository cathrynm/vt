#ifndef MAIN_H
#define MAIN_H 1

#include <atari.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "options.h"
#include "screen.h"
#include "xep80.h"
#include "direct.h"
#include "vbxe.h"
#include "rawcon.h"
#include "vt.h"
#include "chio.h"
#include "serial.h"
#include "sio.h"
#include "io.h"
#include "detect.h"
#include "asm.h"
#include "config.h"

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

#endif