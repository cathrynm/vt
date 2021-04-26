#ifndef CONFIG_H
#define CONFIG_H 1

void crToZero(unsigned char *s, unsigned char len);
void geturl(int *argc, char ***argv, unsigned char *err);
void parseCommandLine(int argc, char **argv, unsigned char **device, openIoStruct *openIo, unsigned char *err);
void crToZero(unsigned char *s, unsigned char len);

#endif