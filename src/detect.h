#ifndef DETECT_H
#define DETECT_H 1

void initDetect(void);
typedef struct {
	unsigned char videoMode; // 'X', 'S', 'A', 'L'
	unsigned char fullAscii; // Whether { } ~ are available
	unsigned char chbas, fullChbas;
	unsigned char osType;
} detectStruct;
extern detectStruct detect;

void initDetect(void);
void closeDetect(void);

#endif