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

#ifdef linux
#include <ctype.h>
#include <unistd.h>
#include <asm/io.h>
#include <string.h>
#include "options.h"
#endif

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

#ifdef linux
char do_direct_port_access = 0;
char port_permissions[0x10000];
#endif

/**********************************************************************
 *	    IO_port_init
 */

/* set_IO_permissions(int val1, int val)
 * Helper function for IO_port_init
 */
#ifdef linux
void set_IO_permissions(int val1, int val, char rw)
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

void do_IO_port_init_read_or_write(char* temp, char rw)
{
	int val, val1, i, len;
	if (!strcasecmp(temp, "all")) {
		fprintf(stderr, "Warning!!! Granting FULL IO port access to"
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

#endif

void IO_port_init()
{
#ifdef linux
	char temp[1024];

	memset(port_permissions, 0, sizeof(port_permissions));
	do_direct_port_access = 0;

	/* Can we do that? */
	if (!iopl(3)) {
		iopl(0);

		PROFILE_GetWineIniString( "ports", "read", "*",
					 temp, sizeof(temp) );
		do_IO_port_init_read_or_write(temp, 1);
		PROFILE_GetWineIniString( "ports", "write", "*",
					 temp, sizeof(temp) );
		do_IO_port_init_read_or_write(temp, 2);
	}


#endif
}

/**********************************************************************
 *	    IO_inport
 */
DWORD IO_inport( int port, int count )
{
    DWORD res = 0;
    BYTE b;    

#ifdef linux    
    if (do_direct_port_access) iopl(3);
#endif

    dprintf_int(stddeb, "IO: %d bytes from port 0x%02x ", count, port );

    while (count-- > 0)
    {
#ifdef linux
	    if(port_permissions[port] & 1) {		    
		    b = inb(port);
	    } else 
#endif
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
			    fprintf( stderr, 
				    "Direct I/O read attempted "
				    "from port %x\n", port);
			    b = 0xff;
			    break;
		    }
	    }

	    port++;
	    res = (res << 8) | b;
    }
#ifdef linux
    if (do_direct_port_access) iopl(0);
#endif
    dprintf_int(stddeb, "( 0x%x )\n", res );
    return res;
}


/**********************************************************************
 *	    IO_outport
 */
void IO_outport( int port, int count, DWORD value )
{
    BYTE b;

    dprintf_int( stddeb, "IO: 0x%lx (%d bytes) to port 0x%02x\n",
                 value, count, port );

#ifdef linux
    if (do_direct_port_access) iopl(3);
#endif

    while (count-- > 0)
    {
        b = value & 0xff;
        value >>= 8;
#ifdef linux
	if (port_permissions[port] & 2) {
		outb(b, port);
	} else 
#endif
	{
		switch (port)
		{
		case 0x70:
			cmosaddress = b & 0x7f;
			break;
		case 0x71:
			cmosimage[cmosaddress & 0x3f] = b;
			break;
		default:
			fprintf( stderr, "Direct I/O write attempted "
				"to port %x\n", port );
			break;
		}
	}
	port++;
    }
#ifdef linux
    if (do_direct_port_access) iopl(0);
#endif
}
