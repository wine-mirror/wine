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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "windows.h"
#include "options.h"
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

#if defined(linux) && defined(__i386__)
# define DIRECT_IO_ACCESS
#else
# undef DIRECT_IO_ACCESS
#endif  /* linux && __i386__ */

#ifdef DIRECT_IO_ACCESS

extern int iopl(int level);

static char do_direct_port_access = 0;
static char port_permissions[0x10000];

#define IO_READ  1
#define IO_WRITE 2

#endif  /* DIRECT_IO_ACCESS */

/**********************************************************************
 *	    IO_port_init
 */

/* set_IO_permissions(int val1, int val)
 * Helper function for IO_port_init
 */
#ifdef DIRECT_IO_ACCESS
static void set_IO_permissions(int val1, int val, char rw)
{
	int j;
	if (val1 != -1) {
		if (val == -1) val = 0x3ff;		
		for (j = val1; j <= val; j++)
			port_permissions[j] |= rw;		

		do_direct_port_access = 1;

		val1 = -1;
	} else if (val != -1) {		
		do_direct_port_access = 1;

		port_permissions[val] |= rw;
	}

}

/* do_IO_port_init_read_or_write(char* temp, char rw)
 * Helper function for IO_port_init
 */

static void do_IO_port_init_read_or_write(char* temp, char rw)
{
	int val, val1, i, len;
	if (!strcasecmp(temp, "all")) {
		MSG("Warning!!! Granting FULL IO port access to"
			" windoze programs!\nWarning!!! "
			"*** THIS IS NOT AT ALL "
			"RECOMMENDED!!! ***\n");
		for (i=0; i < sizeof(port_permissions); i++)
			port_permissions[i] |= rw;

	} else if (!(!strcmp(temp, "*") || *temp == '\0')) {
		len = strlen(temp);
		val = -1;
		val1 = -1;		
		for (i = 0; i < len; i++) {
			switch (temp[i]) {
			case '0':
				if (temp[i+1] == 'x' || temp[i+1] == 'X') {
					sscanf(temp+i, "%x", &val);
					i += 2;
				} else {
					sscanf(temp+i, "%d", &val);
				}
				while (isxdigit(temp[i]))
					i++;
				i--;
				break;
			case ',':
			case ' ':
			case '\t':
				set_IO_permissions(val1, val, rw);
				val1 = -1; val = -1;
				break;
			case '-':
				val1 = val;
				if (val1 == -1) val1 = 0;
				break;
			default:
				if (temp[i] >= '0' && temp[i] <= '9') {
					sscanf(temp+i, "%d", &val);
					while (isdigit(temp[i]))
						i++;
				}
			}
		}
		set_IO_permissions(val1, val, rw);		
	}
}

static __inline__ BYTE inb( WORD port )
{
    BYTE b;
    __asm__ __volatile__( "inb %w1,%0" : "=a" (b) : "d" (port) );
    return b;
}

static __inline__ WORD inw( WORD port )
{
    WORD w;
    __asm__ __volatile__( "inw %w1,%0" : "=a" (w) : "d" (port) );
    return w;
}

static __inline__ DWORD inl( WORD port )
{
    DWORD dw;
    __asm__ __volatile__( "inl %w1,%0" : "=a" (dw) : "d" (port) );
    return dw;
}

static __inline__ void outb( BYTE value, WORD port )
{
    __asm__ __volatile__( "outb %b0,%w1" : : "a" (value), "d" (port) );
}

static __inline__ void outw( WORD value, WORD port )
{
    __asm__ __volatile__( "outw %w0,%w1" : : "a" (value), "d" (port) );
}

static __inline__ void outl( DWORD value, WORD port )
{
    __asm__ __volatile__( "outl %0,%w1" : : "a" (value), "d" (port) );
}

#endif  /* DIRECT_IO_ACCESS */

void IO_port_init()
{
#ifdef DIRECT_IO_ACCESS
	char temp[1024];

	/* Can we do that? */
	if (!iopl(3)) {
		iopl(0);

		PROFILE_GetWineIniString( "ports", "read", "*",
					 temp, sizeof(temp) );
		do_IO_port_init_read_or_write(temp, IO_READ);
		PROFILE_GetWineIniString( "ports", "write", "*",
					 temp, sizeof(temp) );
		do_IO_port_init_read_or_write(temp, IO_WRITE);
	}
#endif  /* DIRECT_IO_ACCESS */
}


/**********************************************************************
 *	    IO_inport
 */
DWORD IO_inport( int port, int count )
{
    DWORD res = 0;
    BYTE b;    

#ifdef DIRECT_IO_ACCESS    
    if (do_direct_port_access)
    {
        /* Make sure we have access to the whole range */
        int i;
        for (i = 0; i < count; i++)
            if (!(port_permissions[port+i] & IO_READ)) break;
        if (i == count)
        {
            iopl(3);
            switch(count)
            {
                case 1: res = inb( port ); break;
                case 2: res = inw( port ); break;
                case 4: res = inl( port ); break;
                default:
                    ERR(int, "invalid count %d\n", count);
            }
            iopl(0);
            return res;
        }
    }
#endif

    TRACE(int, "%d bytes from port 0x%02x\n", count, port );

    while (count-- > 0)
    {
        switch (port)
        {
        case 0x70:
            b = cmosaddress;
            break;
        case 0x71:
            b = cmosimage[cmosaddress & 0x3f];
            break;
        default:
            WARN( int, "Direct I/O read attempted from port %x\n", port);
            b = 0xff;
            break;
        }
        port++;
        res = (res << 8) | b;
    }
    TRACE(int, "  returning ( 0x%lx )\n", res );
    return res;
}


/**********************************************************************
 *	    IO_outport
 */
void IO_outport( int port, int count, DWORD value )
{
    BYTE b;

    TRACE(int, "IO: 0x%lx (%d bytes) to port 0x%02x\n",
                 value, count, port );

#ifdef DIRECT_IO_ACCESS
    if (do_direct_port_access)
    {
        /* Make sure we have access to the whole range */
        int i;
        for (i = 0; i < count; i++)
            if (!(port_permissions[port+i] & IO_WRITE)) break;
        if (i == count)
        {
            iopl(3);
            switch(count)
            {
                case 1: outb( LOBYTE(value), port ); break;
                case 2: outw( LOWORD(value), port ); break;
                case 4: outl( value, port ); break;
                default:
                    WARN(int, "Invalid count %d\n", count);
            }
            iopl(0);
            return;
        }
    }
#endif

    while (count-- > 0)
    {
        b = value & 0xff;
        value >>= 8;
        switch (port)
        {
        case 0x70:
            cmosaddress = b & 0x7f;
            break;
        case 0x71:
            cmosimage[cmosaddress & 0x3f] = b;
            break;
        default:
            WARN(int, "Direct I/O write attempted to port %x\n", port );
            break;
        }
	port++;
    }
}
