#include "main.h"

// define some aliases for read register names
#define CORE_VERSION        VIDEO_CONTROL
#define MINOR_VERSION       XDL_ADR0
#define COLDETECT           COLCLR
#define BLT_COLLISION_CODE  BL_ADR0
#define BLITTER_BUSY        BLITTER_START
#define IRQ_STATUS          IRQ_CONTROL


typedef struct _VBXE_REGS {
    unsigned char VIDEO_CONTROL;
    unsigned char XDL_ADR0;
    unsigned char XDL_ADR1;
    unsigned char XDL_ADR2;
    unsigned char CSEL;
    unsigned char PSEL;
    unsigned char CR;
    unsigned char CG;
    unsigned char CB;
    unsigned char COLMASK;
    unsigned char COLCLR;
    unsigned char _unused0[5];
    unsigned char BL_ADR0;
    unsigned char BL_ADR1;
    unsigned char BL_ADR2;
    unsigned char BLITTER_START;
    unsigned char IRQ_CONTROL;
    unsigned char P0;
    unsigned char P1;
    unsigned char P2;
    unsigned char P3;
    unsigned char _unused1[4];
    unsigned char MEMAC_B_CONTROL;
    unsigned char MEMAC_CONTROL;
    unsigned char MEMAC_BANK_SEL;
} VBXE_REGS;


//* XDL
//
// Order of XDL data (if required)
//
// XDLC_RPTL    (1 byte)
// XDLC_OVADR   (5 bytes)
// XDLC_OVSCRL  (2 bytes)
// XDLC_CHBASE  (1 byte)
// XDLC_MAPADR  (5 bytes)
// XDLC_MAPPAR  (4 bytes)
// XDLC_OVATT   (2 bytes)

#define XDLC_TMON       0x0001
#define XDLC_GMON       0x0002
#define XDLC_OVOFF      0x0004
#define XDLC_MAPON      0x0008
#define XDLC_MAPOFF     0x0010
#define XDLC_RPTL       0x0020
#define XDLC_OVADR      0x0040
#define XDLC_OVSCRL     0x0080
#define XDLC_CHBASE     0x0100
#define XDLC_MAPADR     0x0200
#define XDLC_MAPPAR     0x0400
#define XDLC_OVATT      0x0800
#define XDLC_HR         0x1000
#define XDLC_LR         0x2000
// 0x4000 is reserved
#define XDLC_END        0x8000

typedef struct {
    unsigned char sdmctl;
    VBXE_REGS *regs;
} vbxeStruct;

vbxeStruct vbxe;

#define XDLC    (XDLC_TMON+XDLC_RPTL+XDLC_OVADR+XDLC_CHBASE+XDLC_OVATT+XDLC_END)

char xdl[] = { (XDLC % 256), (XDLC / 256), 
	       215, 0x20, 0x0E, 0x00, 160, 0, 1, 1, 255 };


unsigned char testForVbxe(void) {
    vbxe.regs->CORE_VERSION = 0;
    return vbxe.regs->CORE_VERSION == 0x10;
}

unsigned char vbxeTest(void)
{
    vbxe.sdmctl = OS.sdmctl;
    vbxe.regs = (VBXE_REGS *) 0xd640;
    if (testForVbxe()) {
        restoreVbxe();
        return 1;
    }
    vbxe.regs = (VBXE_REGS *) 0xd740;
    if (testForVbxe()) {
        restoreVbxe();
        return 1;
    }
    vbxe.regs = NULL;
    return 0;
}

void initVbxe(void)
{
    unsigned char y;
    vbxe.regs->MEMAC_CONTROL = 0x48;       // 4k banks, CPU only, $4000
    vbxe.regs->MEMAC_BANK_SEL = 0x80;      // bank zero
    // copy font to $0800 VBXE memory, bank 0
    memcpy((unsigned char *)0x4800, (unsigned char*)0xE000, 1024); // 
    // copy XDL to VBXE memory, bank 0
    memcpy((unsigned char *)0x4000, xdl, 11);
    // set XDL address
    vbxe.regs->XDL_ADR0 = 0x00;
    vbxe.regs->XDL_ADR1 = 0x00;
    vbxe.regs->XDL_ADR2 = 0x00;
    // turn on XDL processing
    vbxe.regs->VIDEO_CONTROL = 0x01;
    vbxe.regs->MEMAC_BANK_SEL = 0x0;      // bank zero
    for (y = 0;y< SCREENLINES;y++) {
        screenX.lineTab[y] = (unsigned short) y * 160;
    }
    OS.sdmctl = 0;
}

void restoreVbxe(void)
{
    if (vbxe.sdmctl) {
        vbxe.regs->VIDEO_CONTROL = 0;
        OS.sdmctl = vbxe.sdmctl;
    } else  vbxe.regs->VIDEO_CONTROL = 0x05;
}

void clearScreenVbxe(void)
{
    vbxe.regs->MEMAC_BANK_SEL = 0x81;
    memset((unsigned char *) 0x4000, 0, 3180);
    vbxe.regs->MEMAC_BANK_SEL = 0x0;
}

void cursorUpdateVbxe(unsigned char x, unsigned char y)
{
}

void drawCharsAtVbxe(unsigned char *s, unsigned char len)
{
    unsigned char *p, *pStart, c;
    static unsigned char sAtascii[4] = {0x40, 0x00, 0x20, 0x60};
    pStart = (unsigned char *) 0x4000 + screenX.lineTab[OS.rowcrs] + (OS.colcrs << 1);
    vbxe.regs->MEMAC_BANK_SEL = 0x81;
    for (p = pStart;len--;) {
        c = *s++;
        *p++ = sAtascii[(c & 0x60) >> 5] | (c & 0x9f);
        *p++ = 0xf;
    }
    vbxe.regs->MEMAC_BANK_SEL = 0x0;
}

void insertLineVbxe(unsigned char y, unsigned char yBottom)
{
}

void deleteLineVbxe(unsigned char y, unsigned char yBottom)
{
}

void insertCharVbxe(unsigned char x, unsigned char y)
{
}

void deleteCharVbxe(unsigned char x, unsigned char y)
{
}

// Try attrib 0x23 and 0xf
