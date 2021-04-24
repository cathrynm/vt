
#ifndef VBXE_H
#define VBXE_H

unsigned char vbxeTest(void);
void initVbxe(void);
void restoreVbxe(void);
void clearScreenVbxe(void);
void cursorUpdateVbxe(unsigned char x, unsigned char y);
void drawCharsAtVbxe(unsigned char *buffer, unsigned char bufferLen);
void insertLineVbxe(unsigned char y, unsigned char yBottom);
void deleteLineVbxe(unsigned char y, unsigned char yBottom);
void insertCharVbxe(unsigned char x, unsigned char y, unsigned char len);
void deleteCharVbxe(unsigned char x, unsigned char y, unsigned char len);
void cursorHideVbxe(void);

#endif