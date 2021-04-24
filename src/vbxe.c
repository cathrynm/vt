#include "main.h"

#define VBXE_WIDTH 80
#define VBXE_HEIGHT 24
#define VBXE_BANKTOP 0x4000

#define VBXE_BANKSHIFT 12
#define VBXE_BANKSIZE (1<<VBXE_BANKSHIFT) // 0x1000
#define VBXE_BANKMASK (VBXE_BANKSIZE-1) // 0xfff

#define VBXE_XDLADDR 0x0000
#define VBXE_XDLXBANK 0
#define VBXE_XDLBANK ((VBXE_XDLXBANK << (16 - VBXE_BANKSHIFT)) | (VBXE_XDLADDR >> VBXE_BANKSHIFT) | 0x80)
#define VBXE_XDLMEM ((unsigned char *)(VBXE_BANKTOP + (VBXE_XDLADDR & VBXE_BANKMASK)))

#define VBXE_BLITADDR 0x0100 // address in VBXE memory of blitter data
#define VBXE_BLITXBANK 0
#define VBXE_BLITBANK ((VBXE_BLITXBANK << (16 - VBXE_BANKSHIFT)) | (VBXE_BLITADDR >> VBXE_BANKSHIFT) | 0x80) // Bank of VBXE blitter
#define VBXE_BLITMEM ((blitterStruct *)(VBXE_BANKTOP + (VBXE_BLITADDR & VBXE_BANKMASK))) // 6502 address of blitter memory

#define VBXE_FONTADDR 0x0800
#define VBXE_FONTXBANK 0
#define VBXE_FONTBANK ((VBXE_FONTXBANK << (16 - VBXE_BANKSHIFT)) | (VBXE_FONTADDR >> VBXE_BANKSHIFT) | 0x80)
#define VBXE_FONTMEM ((unsigned char *)(VBXE_BANKTOP + (VBXE_FONTADDR & VBXE_BANKMASK)))

#define VBXE_SCREENADDR 0x1000 // address in VBXE memory of screen
#define VBXE_SCREENXBANK 0
#define VBXE_SCREENBANK ((VBXE_SCREENXBANK << (16 - VBXE_BANKSHIFT)) | (VBXE_SCREENADDR >> VBXE_BANKSHIFT) | 0x80) // Bank of VBXE Screen memory
#define VBXE_SCREENMEM ((unsigned char *)(VBXE_BANKTOP + (VBXE_SCREENADDR & VBXE_BANKMASK))) // 6502 address of Screen




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
    union {
        unsigned char VIDEO_CONTROL;
        unsigned char CORE_VERSION;
    };
    union {
        struct {
            union {
                unsigned char XDL_ADR0;
                unsigned char MINOR_REVISION;
            };
            unsigned char XDL_ADR1;
        };
        unsigned short XDL_ADR;
    };
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
        struct {
            union {
                unsigned char BL_ADR0;
                unsigned char BLT_COLLISION_CODE;
            };
            unsigned char BL_ADR1;
        };
        unsigned short BL_ADR;
    };
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

typedef struct {
    unsigned char sdmctl;
    unsigned char MEMAC_CONTROL;
    unsigned char MEMAC_BANK_SEL;
    VBXE_REGS *regs;
    unsigned char cursorX, cursorY, cursorOn;
} vbxeStruct;

vbxeStruct vbxe;

#define XDLC    (XDLC_TMON+XDLC_RPTL+XDLC_OVADR+XDLC_CHBASE+XDLC_OVATT+XDLC_END)
// Order of XDL data (if required)
// XDLC_RPTL    (1 byte)
// XDLC_OVADR   (5 bytes)
// XDLC_OVSCRL  (2 bytes)
// XDLC_CHBASE  (1 byte)
// XDLC_MAPADR  (5 bytes)
// XDLC_MAPPAR  (4 bytes)
// XDLC_OVATT   (2 bytes)
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
    blitterStruct *blitter = VBXE_BLITMEM;
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
    vbxe.regs->BL_ADR = VBXE_BLITADDR;
    vbxe.regs->BL_ADR2 = VBXE_BLITXBANK;
}

void blit(unsigned short dest, unsigned short source, unsigned char wid, unsigned char hi)
{
    blitterStruct *blitter = VBXE_BLITMEM;
    while (vbxe.regs->BLITTER_BUSY);
    vbxe.regs->MEMAC_BANK_SEL = VBXE_BLITBANK;
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
    unsigned short n;
    unsigned char y;
    vbxe.regs->MEMAC_CONTROL = (VBXE_BANKTOP >> 8) | 0x8 | (VBXE_BANKSHIFT - 12);       //  0x8 = CPU
    vbxe.regs->MEMAC_BANK_SEL = VBXE_XDLBANK;
    memcpy(VBXE_FONTMEM, (unsigned char*)0xE000, 1024);
    for (n = 0;n<1024;n++ ) {
        VBXE_FONTMEM[n + 1024] = VBXE_FONTMEM[n] ^ 0xff; 
    }
    memcpy(VBXE_XDLMEM, xdl, 11);
    vbxe.regs->XDL_ADR = VBXE_XDLADDR;
    vbxe.regs->XDL_ADR2 = VBXE_XDLXBANK;
    // turn on XDL processing
    vbxe.regs->VIDEO_CONTROL = 0x01;
    initBlit();
    vbxe.regs->MEMAC_BANK_SEL = 0x0;
    for (y = 0;y < SCREENLINES;y++) {
        screenX.lineTab[y] = (unsigned short) y * VBXE_WIDTH * 2;
    }
    OS.sdmctl = 0;
    vbxe.cursorOn = 0;
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
    blit(VBXE_SCREENADDR, 0, VBXE_WIDTH, VBXE_HEIGHT);
}

void cursorUpdateVbxe(unsigned char x, unsigned char y)
{
    unsigned char *pStart = VBXE_SCREENMEM + screenX.lineTab[y] + (x << 1);
    if (vbxe.cursorOn)cursorHideVbxe();
    vbxe.regs->MEMAC_BANK_SEL = VBXE_SCREENBANK;
    *pStart++ |= 0x80;
    *pStart = 0xf;
    vbxe.regs->MEMAC_BANK_SEL = 0;
    vbxe.cursorX = x;
    vbxe.cursorY = y;
    vbxe.cursorOn = 1;
}

void cursorHideVbxe(void)
{
    unsigned char *pStart;
    if (!vbxe.cursorOn)return;
    pStart = VBXE_SCREENMEM + screenX.lineTab[vbxe.cursorY] + (vbxe.cursorX << 1);
    vbxe.regs->MEMAC_BANK_SEL = VBXE_SCREENBANK;
    *pStart = *pStart & 0x7f;
    vbxe.regs->MEMAC_BANK_SEL = 0;
    vbxe.cursorOn = 0;
}

void drawCharsAtVbxe(unsigned char *s, unsigned char len)
{
    unsigned char *p, *pStart, c;
    pStart = VBXE_SCREENMEM + screenX.lineTab[OS.rowcrs] + (OS.colcrs << 1);
    vbxe.regs->MEMAC_BANK_SEL = VBXE_SCREENBANK;
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
        blit(VBXE_SCREENADDR + screenX.lineTab[y + 1], VBXE_SCREENADDR + screenX.lineTab[y], VBXE_WIDTH, yBottom - y);
    }
    blit(VBXE_SCREENADDR + screenX.lineTab[y], 0, VBXE_WIDTH, 1);
}

void deleteLineVbxe(unsigned char y, unsigned char yBottom)
{
    if (y < yBottom) {
        blit(VBXE_SCREENADDR + screenX.lineTab[y], VBXE_SCREENADDR + screenX.lineTab[y+1], VBXE_WIDTH, yBottom - y);
    }
    blit(VBXE_SCREENADDR + screenX.lineTab[yBottom], 0, VBXE_WIDTH, 1);
}

void insertCharVbxe(unsigned char x, unsigned char y, unsigned char len)
{
    unsigned char *pStart;
    if (x + len >= VBXE_WIDTH)len--;
    if (len > 0)
        blit(VBXE_SCREENADDR + screenX.lineTab[y] + (x << 1) + 2, VBXE_SCREENADDR + screenX.lineTab[y+1] + (x << 1), len, 1);
    pStart = VBXE_SCREENMEM + screenX.lineTab[y] + (x << 1);
    vbxe.regs->MEMAC_BANK_SEL = VBXE_SCREENBANK;
    *pStart++ = 0;
    *pStart = 0;
    vbxe.regs->MEMAC_BANK_SEL = 0;
}

void deleteCharVbxe(unsigned char x, unsigned char y, unsigned char len)
{
    unsigned char *pStart;
    if (len > 1)
        blit(VBXE_SCREENADDR + screenX.lineTab[y] + (x << 1), VBXE_SCREENADDR + screenX.lineTab[y+1] + (x << 1) + 2, len - 1, 1);
    pStart = VBXE_SCREENMEM + screenX.lineTab[y] + ((x + len - 1) << 1);
    vbxe.regs->MEMAC_BANK_SEL = VBXE_SCREENBANK;
    *pStart++ = 0;
    *pStart = 0;
    vbxe.regs->MEMAC_BANK_SEL = 0;
}

// Try attrib 0x23 and 0xf
