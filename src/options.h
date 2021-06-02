#ifndef OPTIONS_H
#define OPTIONS_H 1

#define ATARIINTERNATIONAL 0 // Enable Atari International character mapping
#define FONT45BIT 0 // This works, but is pretty ugly.  How to get to the original font?
#ifndef VBXE_ON
#define VBXE_ON 0
#endif
#ifndef XEP_ON
#define XEP_ON 0
#endif
#define XEPINTERNALFONT (XEP_ON && 1)
#ifndef DIRECT_ON
#define DIRECT_ON 0
#endif
#ifndef RAWCON_ON
#define RAWCON_ON 0
#endif
#ifndef CIO_ON
#define CIO_ON 0
#endif
#ifndef FUJINET_ON
#define FUJINET_ON 0
#endif
#ifndef SERIAL_ON
#define SERIAL_ON 0
#endif
#define RBUFFERSIZE 0x800 // Buffer size for serial input
#define IOBUFFER (SERIAL_ON && 1) /* preload all serial data into EXT mem */
#define FLOWXONOFF 0 /* send ctrl-s when buffer is full. This doesn't seem to do anything useful */
#endif