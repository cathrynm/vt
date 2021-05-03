#include "main.h"

#if VBXE_ON

#define VBXE_WIDTH 80
#define VBXE_HEIGHT 24

#define VBXE_BANKSHIFT 12
#define VBXE_BANKSIZE (1<<VBXE_BANKSHIFT) // 0x1000
#define VBXE_BANKMASK (VBXE_BANKSIZE-1) // 0xfff

#define VBXE_XDLADDR 0x0000
#define VBXE_XDLXBANK 0
#define VBXE_XDLBANK ((VBXE_XDLXBANK << (16 - VBXE_BANKSHIFT)) | (VBXE_XDLADDR >> VBXE_BANKSHIFT) | 0x80)
#define VBXE_XDLMEM (vbxe.bankTop + (VBXE_XDLADDR & VBXE_BANKMASK))

#define VBXE_BLITADDR 0x0100 // address in VBXE memory of blitter data
#define VBXE_BLITXBANK 0
#define VBXE_BLITBANK ((VBXE_BLITXBANK << (16 - VBXE_BANKSHIFT)) | (VBXE_BLITADDR >> VBXE_BANKSHIFT) | 0x80) // Bank of VBXE blitter
#define VBXE_BLITMEM ((blitterStruct *)(vbxe.bankTop + (VBXE_BLITADDR & VBXE_BANKMASK))) // 6502 address of blitter memory

#define VBXE_FONTADDR 0x0800
#define VBXE_FONTXBANK 0
#define VBXE_FONTBANK ((VBXE_FONTXBANK << (16 - VBXE_BANKSHIFT)) | (VBXE_FONTADDR >> VBXE_BANKSHIFT) | 0x80)
#define VBXE_FONTMEM (vbxe.bankTop + (VBXE_FONTADDR & VBXE_BANKMASK))

#define VBXE_SCREENADDR 0x1000 // address in VBXE memory of screen
#define VBXE_SCREENXBANK 0
#define VBXE_SCREENBANK ((VBXE_SCREENXBANK << (16 - VBXE_BANKSHIFT)) | (VBXE_SCREENADDR >> VBXE_BANKSHIFT) | 0x80) // Bank of VBXE Screen memory
#define VBXE_SCREENMEM (vbxe.bankTop + (VBXE_SCREENADDR & VBXE_BANKMASK)) // 6502 address of Screen


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
    union {
        struct {
            unsigned char source_step_y_0;
            unsigned char source_step_y_1;
        };
        signed short source_step_y;
    };
    unsigned char source_step_x;
    union {
        struct {
            unsigned char dest_adr0;
            unsigned char dest_adr1;
        };
        unsigned short dest_adr;
    };
    unsigned char dest_adr2;
    union {
        struct {
            unsigned char dest_step_y_0;
            unsigned char dest_step_y_1;
        };
        signed short dest_step_y;
    };
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
    unsigned char blt_collision_mask;
    unsigned char blt_zoom;
    unsigned char pattern_feature;
    unsigned char blt_control;
} blitterStruct;

typedef struct {
    unsigned char *bankTop;
    unsigned char sdmctl;
    unsigned char MEMAC_CONTROL;
    unsigned char MEMAC_BANK_SEL;
    VBXE_REGS *regs;
    unsigned char cursorX, cursorY, cursorOn, cursorColor;
    unsigned char bios;
} vbxeStruct;

vbxeStruct vbxe;


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
#define XDLC_ATT      0x0800
#define XDLC_HR         0x1000
#define XDLC_LR         0x2000
// 0x4000 is reserved
#define XDLC_END        0x8000

#define XDLCBLANK  (XDLC_RPTL)
#define XDLC    (XDLC_TMON+XDLC_RPTL+XDLC_OVADR+XDLC_CHBASE+XDLC_ATT+XDLC_END)

// XDLC_RPTL    (1 byte)
// XDLC_OVADR   (5 bytes)
// XDLC_OVSCRL  (2 bytes)
// XDLC_CHBASE  (1 byte)
// XDLC_MAPADR  (5 bytes)
// XDLC_MAPPAR  (4 bytes)
// XDLC_ATT   (2 bytes)

static unsigned char displayList[] = { 
    XDLCBLANK & 0xff, XDLCBLANK >> 8,
    23,  // RPTL
    XDLC & 0xff, XDLC >> 8, 
    192, // RPTL
    VBXE_SCREENADDR & 0xff, VBXE_SCREENADDR/256, VBXE_SCREENXBANK,  // OVADR
    VBXE_WIDTH*2, 0, 
    VBXE_FONTADDR/0x800,  // CHBASE
    1, 255 // ATT
};


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

void vbxeWrite(unsigned char *dest, unsigned char bank, unsigned char len) // Does a double copy, but I'm tired of random caused by code address/bank overlap
{
    vbxeAddr = (void *) dest;
    memacReg = (void *)&vbxe.regs->MEMAC_BANK_SEL;
    writeVBXE(bank | (len << 8));
}

void vbxeWriteFrom(unsigned char *dest, unsigned char *src, unsigned char bank, unsigned char len)
{
    memcpy(&vbxeData[0], src, len);
    vbxeWrite(dest, bank, len);
}

unsigned char vbxeRead(unsigned char *dest, unsigned char bank)
{
    vbxeReadAddr = (void *) dest;
    memacReg = (void *)&vbxe.regs->MEMAC_BANK_SEL;
    return readVBXE(bank);
}

void blit(unsigned short dest, unsigned short source, unsigned char wid, unsigned char hi, unsigned char rev)
{
    blitterStruct *blitter = (blitterStruct *) &vbxeData[0];
    blitter->source_adr = source;
    blitter->source_adr2 = 0;
    blitter->source_step_y = (rev? -wid: wid) << 1;
    blitter->source_step_x = rev? -1:1;
    blitter->dest_adr = dest;
    blitter->dest_adr2 = 0;
    blitter->dest_step_y = blitter->source_step_y;
    blitter->dest_step_x = blitter->source_step_x;
    blitter->blt_width = (wid << 1) - 1;
    blitter->blt_height = hi - 1;
    blitter->blt_and_mask = (source != 0)? 0xff: 0;
    blitter->blt_xor_mask = 0;
    blitter->blt_collision_mask = 0;
    blitter->blt_zoom = 0;
    blitter->pattern_feature = 0;
    blitter->blt_control = 0;
    vbxeAddr = VBXE_BLITMEM;
    memacReg = (void *) &vbxe.regs->MEMAC_BANK_SEL;
    while (vbxe.regs->BLITTER_BUSY);
    writeVBXE(VBXE_BLITBANK | ( sizeof(blitterStruct) << 8));
    vbxe.regs->BL_ADR = VBXE_BLITADDR;
    vbxe.regs->BL_ADR2 = VBXE_BLITXBANK;
    vbxe.regs->BLITTER_START = 1;
}

void initPalette(void)
{
    unsigned char colorNumber;
    for (colorNumber = 0;colorNumber<255;colorNumber++) {
        vbxe.regs->PSEL = 0;
        vbxe.regs->CSEL = colorNumber;
        if (colorNumber < 128) {
            vbxe.regs->CR = (((colorNumber & 1)!= 0)?171:0) + (((colorNumber & 8) != 0)?84:0);
            vbxe.regs->CG = (((colorNumber & 2)!= 0)?171:0) + (((colorNumber & 8) != 0)?84:0);
            vbxe.regs->CB = (((colorNumber & 4)!= 0)?171:0) + (((colorNumber & 8) != 0)?84:0);
        } else {
            vbxe.regs->CR = (((colorNumber & 0x10)!= 0)?171:0);
            vbxe.regs->CG = (((colorNumber & 0x20)!= 0)?171:0);
            vbxe.regs->CB = (((colorNumber & 0x40)!= 0)?171:0);
        }
    }
}

void copyCharVbxe(unsigned char ch, unsigned char *from)
{
    vbxeWriteFrom(VBXE_FONTMEM + ((unsigned short) ch << 3), from, VBXE_FONTBANK, 8);
}


void initVbxe(void)
{
    unsigned char err = ERR_NONE;
    unsigned char y;
    vbxe.bankTop = (unsigned char *)0x2000; // Must be 0x1000 boundary
    OS.iocb[6].buffer = "S2:";
    OS.iocb[6].buflen = strlen("S:");
    OS.iocb[6].command = 96; // VBXE Bios detect
    OS.iocb[6].aux1 = 0;
    OS.iocb[6].aux2 = 0;
    cio(6);
    iocbErrUpdate(6, &err);
    vbxe.bios = ((err == ERR_NONE) && (OS.iocb[6].spare == 96));
    vbxe.regs->MEMAC_CONTROL = (((unsigned short)vbxe.bankTop) >> 8) | 0x8 | (VBXE_BANKSHIFT - 12);       //  0x8 = CPU 
    initAscii(detect.fullChbas, copyCharVbxe);
    vbxeWriteFrom(VBXE_XDLMEM, displayList, VBXE_XDLBANK, sizeof(displayList));
    vbxe.regs->XDL_ADR = VBXE_XDLADDR;
    vbxe.regs->XDL_ADR2 = VBXE_XDLXBANK;
    vbxe.regs->VIDEO_CONTROL = 0x01;

    initPalette();
    for (y = 0;y < SCREENLINES;y++) {
        screenX.lineTab[y] = (unsigned short) y * VBXE_WIDTH * 2;
        screenX.lineLength[y] = VBXE_WIDTH; // So first clear screen clears old junk
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

void clearScreenVbxe(unsigned char color)
{
    unsigned char yp;
    for (yp = 0;yp < SCREENLINES;yp++) {
        drawClearLine(yp, color);
    }
}

unsigned char invertColor(unsigned char color)
{
    unsigned char hi = (color & 0x70) >> 4;
    unsigned char lo = color & 7;
    return 0x80 | (lo << 4) | hi;
}


void cursorUpdateVbxe(unsigned char x, unsigned char y)
{
    unsigned char *pStart;
    if (vbxe.cursorOn) {
        if ((x == vbxe.cursorX) && (y == vbxe.cursorY))return;
        cursorHideVbxe();
    }
    pStart = VBXE_SCREENMEM + screenX.lineTab[y] + (x << 1) + 1;
    vbxe.cursorColor = vbxeRead(pStart, VBXE_SCREENBANK);
    vbxeData[0] = invertColor(vbxe.cursorColor);
    vbxeWrite(pStart, VBXE_SCREENBANK, 1);
    vbxe.cursorX = x;
    vbxe.cursorY = y;
    vbxe.cursorOn = 1;
}

void cursorHideVbxe(void)
{
    unsigned char *pStart;
    if (!vbxe.cursorOn)return;
    pStart = VBXE_SCREENMEM + screenX.lineTab[vbxe.cursorY] + (vbxe.cursorX << 1) + 1;
    vbxeData[0] = vbxe.cursorColor;
    vbxeWrite(pStart, VBXE_SCREENBANK, 1);
    vbxe.cursorOn = 0;
}

void drawCharsAtVbxe(unsigned char *s, unsigned char len)
{
    unsigned char cnt;
    unsigned char *p = &vbxeData[0], c, col;
    if (len + OS.colcrs > VBXE_WIDTH)len = VBXE_WIDTH - OS.colcrs;
    for (cnt = len;cnt--;) {
        c = *s++;
        *p++ = sAtascii[(c & 0x60) >> 5] | (c & 0x1f);
        col = *s++;
        *p++ = 0x80 | ((c & 0x80)? invertColor(col): col); 
    }
    vbxeWrite(VBXE_SCREENMEM + screenX.lineTab[OS.rowcrs] + (OS.colcrs << 1), VBXE_SCREENBANK, len << 1);
}

void insertLineVbxe(unsigned char y, unsigned char yBottom, unsigned char color)
{
    cursorHide();
    if (y < yBottom)
        blit(VBXE_SCREENADDR + screenX.lineTab[yBottom] + VBXE_WIDTH*2-1, VBXE_SCREENADDR + screenX.lineTab[yBottom - 1] + VBXE_WIDTH*2-1, VBXE_WIDTH, yBottom - y, 1);
    drawClearLine(y, color);
}

void deleteLineVbxe(unsigned char y, unsigned char yBottom, unsigned char color)
{
    cursorHide();
    if (y < yBottom)
        blit(VBXE_SCREENADDR + screenX.lineTab[y], VBXE_SCREENADDR + screenX.lineTab[y+1], VBXE_WIDTH, yBottom - y, 0);
    drawClearLine(yBottom, color);
}

void insertCharVbxe(unsigned char x, unsigned char y, unsigned char color)
{
    unsigned char len = VBXE_WIDTH - 1 - x;
    if (y == vbxe.cursorY)cursorHide();
    if (len > 0) 
        blit(VBXE_SCREENADDR + screenX.lineTab[y] +  ((VBXE_WIDTH-1) << 1)+1, VBXE_SCREENADDR + screenX.lineTab[y] + ((VBXE_WIDTH-2) << 1)+1, len, 1, 1);
    vbxeData[0] = 0;
    vbxeData[1] = 0xf | 0x80;
    vbxeWrite(VBXE_SCREENMEM + screenX.lineTab[y] + (x << 1), VBXE_SCREENBANK, 2);
}

void deleteCharVbxe(unsigned char x, unsigned char y, unsigned char len, unsigned char color)
{
    if (y == vbxe.cursorY)cursorHide();
    if (len > 1)
        blit(VBXE_SCREENADDR + screenX.lineTab[y] + (x << 1), VBXE_SCREENADDR + screenX.lineTab[y] + (x << 1) + 2, len - 1, 1, 0);
    vbxeData[0] = 0;
    vbxeData[1] = color | 0x80;
    vbxeWrite(VBXE_SCREENMEM+ screenX.lineTab[y] + ((x + len - 1) << 1), VBXE_SCREENBANK, 2);
}

#endif