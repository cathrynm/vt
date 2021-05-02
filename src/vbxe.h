
#ifndef VBXE_H
#define VBXE_H

unsigned char vbxeTest(void);
void initVbxe(void);
void restoreVbxe(void);
void clearScreenVbxe(unsigned char color);
void cursorUpdateVbxe(unsigned char x, unsigned char y);
void drawCharsAtVbxe(unsigned char *buffer, unsigned char bufferLen);
void insertLineVbxe(unsigned char y, unsigned char yBottom, unsigned char color);
void deleteLineVbxe(unsigned char y, unsigned char yBottom, unsigned char color);
void insertCharVbxe(unsigned char x, unsigned char y, unsigned char color);
void deleteCharVbxe(unsigned char x, unsigned char y, unsigned char len, unsigned char color);
void cursorHideVbxe(void);

#endif