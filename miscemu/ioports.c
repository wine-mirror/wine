/*
 * Emulation of processor ioports.
 *
 * Copyright 1995 Morten Welinder
 */

/* Known problems:
   - only a few ports are emulated.
   - real-time clock in "cmos" is bogus.  A nifty alarm() setup could
     fix that, I guess.
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "windows.h"
#include "stddebug.h"
/* #define DEBUG_INT */
#include "debug.h"

static BYTE cmosaddress;

static BYTE cmosimage[64] =
{
  0x27, 0x34, 0x31, 0x47, 0x16, 0x15, 0x00, 0x01,
  0x04, 0x94, 0x26, 0x02, 0x50, 0x80, 0x00, 0x00,
  0x40, 0xb1, 0x00, 0x9c, 0x01, 0x80, 0x02, 0x00,
  0x1c, 0x00, 0x00, 0xad, 0x02, 0x10, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x03, 0x58,
  0x00, 0x1c, 0x19, 0x81, 0x00, 0x0e, 0x00, 0x80,
  0x1b, 0x7b, 0x21, 0x00, 0x00, 0x00, 0x05, 0x5f
};


DWORD inport( int port, int count )
{
    DWORD res = 0;
    BYTE b;

    dprintf_int(stddeb, "IO: %d bytes from port 0x%02x\n", count, port );

    while (count-- > 0)
    {
        switch (port++)
	{
	case 0x70:
            b = cmosaddress;
            break;
	case 0x71:
            b = cmosimage[cmosaddress & 0x3f];
            break;
	default:
	  b = 0xff;
	}
        res = (res << 8) | b;
    }
    return res;
}


void outport( int port, int count, DWORD value )
{
    BYTE b;

    dprintf_int( stddeb, "IO: 0x%lx (%d bytes) to port 0x%02x\n",
                 value, count, port );

    while (count-- > 0)
    {
        b = value & 0xff;
        value >>= 8;
        switch (port++)
	{
	case 0x70:
            cmosaddress = b & 0x7f;
            break;
	case 0x71:
            cmosimage[cmosaddress & 0x3f] = b;
            break;
	default:
            /* Rien du tout.  */
	}
    }
}
