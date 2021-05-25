#ifndef OPTIONS_H
#define OPTIONS_H 1

#define ATARIINTERNATIONAL 0 // Enable Atari International character mapping
#define FONT45BIT 0 // This works, but is pretty ugly.  How to get to the original font?
#define VBXE_ON 1
#define XEP_ON 0
#define XEPINTERNALFONT (XEP_ON && 1)
#define DIRECT_ON 0
#define RAWCON_ON 0
#define CIO_ON 0
#define FUJINET_ON 0
#define SERIAL_ON 1
#define RBUFFERSIZE 0x800 // Buffer size for serial input
#define IOBUFFER (SERIAL_ON && 1) /* preload all serial data into EXT mem */
#define FLOWXONOFF 0 /* send ctrl-s when buffer is full. This acts weird */
#endif