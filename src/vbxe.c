#include "main.h"

typedef struct {
    union {
        unsigned char VIDEO_CONTROL;
        unsigned char CORE_VERSION;
    };
    union {
        unsigned char XDL_ADR0;
        unsigned char MINOR_REVISION;
    };
    unsigned char XDL_ADR1;
    unsigned char XDL_ADR2;
    unsigned char CSEL;
    unsigned char PSEL;
    unsigned char CR;
    unsigned char CG;
    unsigned char CB;
    unsigned char COLMASK;
    union {
       unsigned char COLCLR;
       unsigned char COLDETECT;
    };
    unsigned char _unused0[5];
    union {
        unsigned char BL_ADR0;
        unsigned char BLT_COLLISION_CODE;
    };
    unsigned char BL_ADR1;
    unsigned char BL_ADR2;
    union {
        unsigned char BLITTER_START;
        volatile unsigned char BLITTER_BUSY;
    };
    union {
        unsigned char IRQ_CONTROL;
        unsigned char IRQ_STATUS;
    };
    unsigned char P0;
    unsigned char P1;
    unsigned char P2;
    unsigned char P3;
    unsigned char _unused1[4];
    unsigned char MEMAC_B_CONTROL;
    unsigned char MEMAC_CONTROL;
    unsigned char MEMAC_BANK_SEL;
} VBXE_REGS;


typedef struct {
    union {
        struct {
            unsigned char source_adr0;
            unsigned char source_adr1;
        };
        unsigned short source_adr;
    };
    unsigned char source_adr2;
    unsigned char source_step_y_0;
    unsigned char source_step_y_1;
    unsigned char source_step_x;
    union {
        struct {
            unsigned char dest_adr0;
            unsigned char dest_adr1;
        };
        unsigned short dest_adr;
    };
    unsigned char dest_adr2;
    unsigned char dest_step_y_0;
    unsigned char dest_step_y_1;
    unsigned char dest_step_x;
    union {
        struct {
            unsigned char blt_width0;
            unsigned char blt_width1;
        };
        unsigned short blt_width;
    };
    unsigned char blt_height;
    unsigned char blt_and_mask;
    unsigned char blt_xor_mask;
    unsigned char blt_collison_mask;
    unsigned char blt_zoom;
    unsigned char pattern_feature;
    unsigned char blt_control;
} blitterStruct;


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
    unsigned char MEMAC_CONTROL;
    unsigned char MEMAC_BANK_SEL;
    VBXE_REGS *regs;
} vbxeStruct;

vbxeStruct vbxe;

#define XDLC    (XDLC_TMON+XDLC_RPTL+XDLC_OVADR+XDLC_CHBASE+XDLC_OVATT+XDLC_END)

static unsigned char xdl[] = { (XDLC % 256), (XDLC / 256), 
	       215, 0x20, 0x0E, 0x00, 160, 0, 1, 1, 255 };


unsigned char testForVbxe(void) {
    unsigned char ver;
    vbxe.regs->CORE_VERSION = 0;
    ver = vbxe.regs->CORE_VERSION;
    return (ver == 0x10) || (ver == 0x11);
}

unsigned char vbxeTest(void)
{
    unsigned char y;
    static VBXE_REGS *baseAddr[2] = {(VBXE_REGS *) 0xd640, (VBXE_REGS *) 0xd740};
    vbxe.sdmctl = OS.sdmctl;
    for (y = 0;y<2;y++) {
        vbxe.regs = baseAddr[y];
        if (testForVbxe()) {
            vbxe.MEMAC_CONTROL = vbxe.regs->MEMAC_CONTROL;
            vbxe.MEMAC_BANK_SEL = vbxe.regs->MEMAC_BANK_SEL;
            vbxe.regs->VIDEO_CONTROL = 0x05;
            return 1;
        }
    }
    vbxe.regs = NULL;
    return 0;
}

void initBlit(void)
{
    blitterStruct *blitter = (blitterStruct *) 0x4100;
    blitter->source_adr = 0; 
    blitter->source_adr2 = 0;
    blitter->source_step_y_0 = 0;
    blitter->source_step_y_1 = 0;
    blitter->source_step_x = 1;
    blitter->dest_adr = 0;
    blitter->dest_adr2 = 0;
    blitter->dest_step_y_0 = 0;
    blitter->dest_step_y_1 = 0;
    blitter->dest_step_x = 1;
    blitter->blt_width = 0;
    blitter->blt_height = 0; 
    blitter->blt_and_mask = 0xff;
    blitter->blt_xor_mask = 0;
    blitter->blt_control = 0; // Next = 0 Mode = 0 (Copy)
    blitter->blt_zoom = 0;
    vbxe.regs->BL_ADR0 = 0;
    vbxe.regs->BL_ADR1 = 0x1;
    vbxe.regs->BL_ADR2 = 0;
}

void blit(unsigned short dest, unsigned short source, unsigned char wid, unsigned char hi)
{
    static blitterStruct *blitter = (blitterStruct *) 0x4100;
    while (vbxe.regs->BLITTER_BUSY);
    vbxe.regs->MEMAC_BANK_SEL = 0x80;
    blitter->source_adr = source;
    blitter->source_step_y_0 = wid << 1;
    blitter->dest_adr = dest;
    blitter->dest_step_y_0 = wid << 1;
    blitter->blt_width = wid << 1;
    blitter->blt_height = hi - 1;
    blitter->blt_and_mask = (source != 0)? 0xff: 0;
    vbxe.regs->BLITTER_START = 1;
    vbxe.regs->MEMAC_BANK_SEL = 0;
}

void initVbxe(void)
{
    unsigned char y;
    OS.appmhi = ((unsigned char *)OS.memtop) - 0x1000;
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
    initBlit();
    vbxe.regs->MEMAC_BANK_SEL = 0x0;
    for (y = 0;y< SCREENLINES;y++) {
        screenX.lineTab[y] = (unsigned short) y * 160;
    }
    OS.sdmctl = 0;
}

void restoreVbxe(void)
{
    vbxe.regs->MEMAC_CONTROL = vbxe.MEMAC_CONTROL;
    vbxe.regs->MEMAC_BANK_SEL = vbxe.MEMAC_BANK_SEL;
    if (vbxe.sdmctl) {
        vbxe.regs->VIDEO_CONTROL = 0;
        OS.sdmctl = vbxe.sdmctl;
    } else  {
// Where S_VBXE.SYS leaves things
// XDL base address 7e130
// Screen is at 7ec00  // 7e
// Character set is at 7c000-7c800 7c
        vbxe.regs->VIDEO_CONTROL = 0x05;
        vbxe.regs->XDL_ADR0 = 0x30; 
        vbxe.regs->XDL_ADR1 = 0xe1;
        vbxe.regs->XDL_ADR2 = 0x07;
    }
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
    if (y < yBottom) {
        blit(0x1000 + screenX.lineTab[y + 1] , 0x1000 + screenX.lineTab[y], 80, yBottom - y);
    }
    blit(0x1000 + screenX.lineTab[y] , 0,  80, 1);
}

void deleteLineVbxe(unsigned char y, unsigned char yBottom)
{
    if (y < yBottom) {
        blit(0x1000 + screenX.lineTab[y] , 0x1000 + screenX.lineTab[y+1], 80, yBottom - y);
    }
    blit(0x1000 + screenX.lineTab[yBottom] , 0,  80, 1);
}

void insertCharVbxe(unsigned char x, unsigned char y)
{
    unsigned char *pStart;
    if (x < 79) {
        blit(0x1000 + screenX.lineTab[y] + x, 0x1000 + screenX.lineTab[y+1] + 1, 79-x, 1);
    }
    pStart = (unsigned char *) 0x4000 + screenX.lineTab[y] + (x << 1);
    vbxe.regs->MEMAC_BANK_SEL = 0x81;
    *pStart++ = 0;
    *pStart = 0;
    vbxe.regs->MEMAC_BANK_SEL = 0x81;
}

void deleteCharVbxe(unsigned char x, unsigned char y)
{
    unsigned char *pStart;
    if (x < 79) {
        blit(0x1000 + screenX.lineTab[y] + x + 1, 0x1000 + screenX.lineTab[y+1], 79-x, 1);
    }
    pStart = (unsigned char *) 0x4000 + screenX.lineTab[y] + (79 << 1);
    vbxe.regs->MEMAC_BANK_SEL = 0x81;
    *pStart++ = 0;
    *pStart = 0;
    vbxe.regs->MEMAC_BANK_SEL = 0x81;
}

// Try attrib 0x23 and 0xf
