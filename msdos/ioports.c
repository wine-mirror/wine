/*
 * Emulation of processor ioports.
 *
 * Copyright 1995 Morten Welinder
 * Copyright 1998 Andreas Mohr, Ove Kaaven
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Known problems:
   - only a few ports are emulated.
   - real-time clock in "cmos" is bogus.  A nifty alarm() setup could
     fix that, I guess.
*/

#include "config.h"
#include "wine/port.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "windef.h"
#include "winnls.h"
#include "winternl.h"
#include "callback.h"
#include "miscemu.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

static struct {
    WORD	countmax;
    BOOL16	byte_toggle; /* if TRUE, then hi byte has already been written */
    WORD	latch;
    BOOL16	latched;
    BYTE	ctrlbyte_ch;
    WORD	oldval;
} tmr_8253[3] = {
    {0xFFFF,	FALSE,	0,	FALSE,	0x36,	0},
    {0x0012,	FALSE,	0,	FALSE,	0x74,	0},
    {0x0001,	FALSE,	0,	FALSE,	0xB6,	0},
};

static int dummy_ctr = 0;

static BYTE parport_8255[4] = {0x4f, 0x20, 0xff, 0xff};

static BYTE cmosaddress;

static BYTE cmosimage[64] =
{
  0x27, 0x34, 0x31, 0x47, 0x16, 0x15, 0x00, 0x01,
  0x04, 0x94, 0x26, 0x02, 0x50, 0x80, 0x00, 0x00,
  0x40, 0xb1, 0x00, 0x9c, 0x01, 0x80, 0x02, 0x00,
  0x1c, 0x00, 0x00, 0xad, 0x02, 0x10, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x03, 0x19,
  0x00, 0x1c, 0x19, 0x81, 0x00, 0x0e, 0x00, 0x80,
  0x1b, 0x7b, 0x21, 0x00, 0x00, 0x00, 0x05, 0x5f
};

#if defined(linux) && defined(__i386__)
# define DIRECT_IO_ACCESS
#else
# undef DIRECT_IO_ACCESS
# undef PP_IO_ACCESS
#endif  /* linux && __i386__ */

#ifdef HAVE_PPDEV
static int do_pp_port_access = -1; /* -1: uninitialized, 1: not available
				       0: available);*/
#endif

#ifdef DIRECT_IO_ACCESS

extern int iopl(int level);
static char do_direct_port_access = -1;
static char port_permissions[0x10000];

#define IO_READ  1
#define IO_WRITE 2

static void IO_FixCMOSCheckSum(void)
{
	WORD sum = 0;
	int i;

	for (i=0x10; i < 0x2d; i++)
		sum += cmosimage[i];
	cmosimage[0x2e] = sum >> 8; /* yes, this IS hi byte !! */
	cmosimage[0x2f] = sum & 0xff;
	TRACE("calculated hi %02x, lo %02x\n", cmosimage[0x2e], cmosimage[0x2f]);
}

#endif  /* DIRECT_IO_ACCESS */

static void set_timer_maxval(unsigned timer, unsigned maxval)
{
    switch (timer) {
        case 0: /* System timer counter divisor */
            if (Dosvm.SetTimer) Dosvm.SetTimer(maxval);
            break;
        case 1: /* RAM refresh */
            FIXME("RAM refresh counter handling not implemented !\n");
            break;
        case 2: /* cassette & speaker */
            /* speaker on ? */
            if (((BYTE)parport_8255[1] & 3) == 3)
            {
                TRACE("Beep (freq: %d) !\n", 1193180 / maxval );
                Beep(1193180 / maxval, 20);
            }
            break;
    }
}

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

static void do_IO_port_init_read_or_write(const WCHAR *str, char rw)
{
    int val, val1, i;
    WCHAR *end;
    static const WCHAR allW[] = {'a','l','l',0};

    if (!strcmpiW(str, allW))
    {
        for (i=0; i < sizeof(port_permissions); i++)
            port_permissions[i] |= rw;
    }
    else
    {
        val = -1;
        val1 = -1;
        while (*str)
        {
            switch(*str)
            {
            case ',':
            case ' ':
            case '\t':
                set_IO_permissions(val1, val, rw);
                val1 = -1;
                val = -1;
                str++;
                break;
            case '-':
                val1 = val;
                if (val1 == -1) val1 = 0;
                str++;
                break;
            default:
                if (isdigitW(*str))
                {
                    val = strtoulW( str, &end, 0 );
                    if (end == str)
                    {
                        val = -1;
                        str++;
                    }
                    else str = end;
                }
                break;
            }
        }
        set_IO_permissions(val1, val, rw);
    }
}

static inline BYTE inb( WORD port )
{
    BYTE b;
    __asm__ __volatile__( "inb %w1,%0" : "=a" (b) : "d" (port) );
    return b;
}

static inline WORD inw( WORD port )
{
    WORD w;
    __asm__ __volatile__( "inw %w1,%0" : "=a" (w) : "d" (port) );
    return w;
}

static inline DWORD inl( WORD port )
{
    DWORD dw;
    __asm__ __volatile__( "inl %w1,%0" : "=a" (dw) : "d" (port) );
    return dw;
}

static inline void outb( BYTE value, WORD port )
{
    __asm__ __volatile__( "outb %b0,%w1" : : "a" (value), "d" (port) );
}

static inline void outw( WORD value, WORD port )
{
    __asm__ __volatile__( "outw %w0,%w1" : : "a" (value), "d" (port) );
}

static inline void outl( DWORD value, WORD port )
{
    __asm__ __volatile__( "outl %0,%w1" : : "a" (value), "d" (port) );
}

static void IO_port_init(void)
{
    char tmp[1024];
    HKEY hkey;
    DWORD dummy;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    static const WCHAR portsW[] = {'M','a','c','h','i','n','e','\\',
                                   'S','o','f','t','w','a','r','e','\\',
                                   'W','i','n','e','\\','W','i','n','e','\\',
                                   'C','o','n','f','i','g','\\','P','o','r','t','s',0};
    static const WCHAR readW[] = {'r','e','a','d',0};
    static const WCHAR writeW[] = {'w','r','i','t','e',0};

    do_direct_port_access = 0;
    /* Can we do that? */
    if (!iopl(3))
    {
        iopl(0);

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &nameW;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        RtlInitUnicodeString( &nameW, portsW );

        if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
        {
            RtlInitUnicodeString( &nameW, readW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
            {
                WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                do_IO_port_init_read_or_write(str, IO_READ);
            }
            RtlInitUnicodeString( &nameW, writeW );
            if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
            {
                WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
                do_IO_port_init_read_or_write(str, IO_WRITE);
            }
            NtClose( hkey );
        }
    }
    IO_FixCMOSCheckSum();
}

#endif  /* DIRECT_IO_ACCESS */

/**********************************************************************
 *	    IO_inport
 *
 * Note: The size argument has to be handled correctly _externally_
 * (as we always return a DWORD)
 */
DWORD IO_inport( int port, int size )
{
    DWORD res = 0;

    TRACE("%d-byte value from port 0x%02x\n", size, port );

#ifdef HAVE_PPDEV
    if (do_pp_port_access == -1)
      do_pp_port_access =IO_pp_init();
    if ((do_pp_port_access == 0 ) && (size == 1))
      if (!IO_pp_inp(port,&res))
         return res;
#endif
#ifdef DIRECT_IO_ACCESS
    if (do_direct_port_access == -1) IO_port_init();
    if ((do_direct_port_access)
        /* Make sure we have access to the port */
        && (port_permissions[port] & IO_READ))
    {
        iopl(3);
        switch(size)
        {
        case 1: res = inb( port ); break;
        case 2: res = inw( port ); break;
        case 4: res = inl( port ); break;
        default:
            ERR("invalid data size %d\n", size);
        }
        iopl(0);
        return res;
    }
#endif

    /* first give the DOS VM a chance to handle it */
    if (Dosvm.inport || DPMI_LoadDosSystem())
        if (Dosvm.inport( port, size, &res )) 
            return res;

    switch (port)
    {
    case 0x40:
    case 0x41:
    case 0x42:
    {
        BYTE chan = port & 3;
        WORD tempval = 0;
	if (tmr_8253[chan].latched)
	    tempval = tmr_8253[chan].latch;
	else
	{
	    dummy_ctr -= 1 + (int)(10.0 * rand() / (RAND_MAX + 1.0));
	    if (chan == 0) /* System timer counter divisor */
	    {
		/* FIXME: Dosvm.GetTimer() returns quite rigid values */
	        if (Dosvm.GetTimer)
		  tempval = dummy_ctr + (WORD)Dosvm.GetTimer();
		else
		  tempval = dummy_ctr;
	    }
	    else
	    {
		/* FIXME: intelligent hardware timer emulation needed */
		tempval = dummy_ctr;
	    }
	}

        switch ((tmr_8253[chan].ctrlbyte_ch & 0x30) >> 4)
        {
        case 0:
	    res = 0; /* shouldn't happen? */
	    break;
        case 1: /* read lo byte */
	    res = (BYTE)tempval;
	    tmr_8253[chan].latched = FALSE;
	    break;
        case 3: /* read lo byte, then hi byte */
            tmr_8253[chan].byte_toggle ^= TRUE; /* toggle */
            if (tmr_8253[chan].byte_toggle)
            {
                res = (BYTE)tempval;
                break;
            }
            /* else [fall through if read hi byte !] */
        case 2: /* read hi byte */
	    res = (BYTE)(tempval >> 8);
 	    tmr_8253[chan].latched = FALSE;
	    break;
        }
    }
    break;
    case 0x60:
#if 0 /* what's this port got to do with parport ? */
        res = (DWORD)parport_8255[0];
#endif
        break;
    case 0x61:
        res = (DWORD)parport_8255[1];
        break;
    case 0x62:
        res = (DWORD)parport_8255[2];
        break;
    case 0x70:
        res = (DWORD)cmosaddress;
        break;
    case 0x71:
        res = (DWORD)cmosimage[cmosaddress & 0x3f];
        break;
    case 0x200:
    case 0x201:
        res = 0xffffffff; /* no joystick */
        break;
    default:
        WARN("Direct I/O read attempted from port %x\n", port);
        res = 0xffffffff;
        break;
    }
    TRACE("  returning ( 0x%lx )\n", res );
    return res;
}


/**********************************************************************
 *	    IO_outport
 */
void IO_outport( int port, int size, DWORD value )
{
    TRACE("IO: 0x%lx (%d-byte value) to port 0x%02x\n",
                 value, size, port );

#ifdef HAVE_PPDEV
    if (do_pp_port_access == -1)
      do_pp_port_access = IO_pp_init();
    if ((do_pp_port_access == 0) && (size == 1))
      if (!IO_pp_outp(port,&value))
         return;
#endif
#ifdef DIRECT_IO_ACCESS

    if (do_direct_port_access == -1) IO_port_init();
    if ((do_direct_port_access)
        /* Make sure we have access to the port */
        && (port_permissions[port] & IO_WRITE))
    {
        iopl(3);
        switch(size)
        {
        case 1: outb( LOBYTE(value), port ); break;
        case 2: outw( LOWORD(value), port ); break;
        case 4: outl( value, port ); break;
        default:
            WARN("Invalid data size %d\n", size);
        }
        iopl(0);
        return;
    }
#endif

    /* first give the DOS VM a chance to handle it */
    if (Dosvm.outport || DPMI_LoadDosSystem())
        if (Dosvm.outport( port, size, value )) 
            return;

    switch (port)
    {
    case 0x40:
    case 0x41:
    case 0x42:
    {
        BYTE chan = port & 3;

	/* we need to get the oldval before any lo/hi byte change has been made */
        if (((tmr_8253[chan].ctrlbyte_ch & 0x30) != 0x30) ||
	    !tmr_8253[chan].byte_toggle)
            tmr_8253[chan].oldval = tmr_8253[chan].countmax;
        switch ((tmr_8253[chan].ctrlbyte_ch & 0x30) >> 4)
        {
        case 0:
            break; /* shouldn't happen? */
        case 1: /* write lo byte */
            tmr_8253[chan].countmax =
                (tmr_8253[chan].countmax & 0xff00) | (BYTE)value;
            break;
        case 3: /* write lo byte, then hi byte */
            tmr_8253[chan].byte_toggle ^= TRUE; /* toggle */
            if (tmr_8253[chan].byte_toggle)
            {
                tmr_8253[chan].countmax =
                    (tmr_8253[chan].countmax & 0xff00) | (BYTE)value;
                break;
            }
            /* else [fall through if write hi byte !] */
        case 2: /* write hi byte */
            tmr_8253[chan].countmax =
                (tmr_8253[chan].countmax & 0x00ff) | ((BYTE)value << 8);
            break;
        }
	/* if programming is finished and value has changed
	   then update to new value */
        if ((((tmr_8253[chan].ctrlbyte_ch & 0x30) != 0x30) ||
	     !tmr_8253[chan].byte_toggle) &&
	    (tmr_8253[chan].countmax != tmr_8253[chan].oldval))
            set_timer_maxval(chan, tmr_8253[chan].countmax);
    }
    break;
    case 0x43:
    {
	BYTE chan = ((BYTE)value & 0xc0) >> 6;
	/* ctrl byte for specific timer channel */
	if (chan == 3)
	{
	    FIXME("8254 timer readback not implemented yet\n");
	    break;
	}
	switch (((BYTE)value & 0x30) >> 4)
	{
	case 0:	/* latch timer */
	    tmr_8253[chan].latched = TRUE;
	    dummy_ctr -= 1 + (int)(10.0 * rand() / (RAND_MAX + 1.0));
	    if (chan == 0) /* System timer divisor */
	        if (Dosvm.GetTimer)
		  tmr_8253[chan].latch = dummy_ctr + (WORD)Dosvm.GetTimer();
	        else
		  tmr_8253[chan].latch = dummy_ctr;
	    else
	    {
		/* FIXME: intelligent hardware timer emulation needed */
		tmr_8253[chan].latch = dummy_ctr;
	    }
	    break;
	case 3:	/* write lo byte, then hi byte */
	    tmr_8253[chan].byte_toggle = FALSE; /* init */
	    /* fall through */
	case 1:	/* write lo byte only */
	case 2:	/* write hi byte only */
	    tmr_8253[chan].ctrlbyte_ch = (BYTE)value;
	    break;
	}
    }
    break;
    case 0x61:
        parport_8255[1] = (BYTE)value;
        if ((((BYTE)parport_8255[1] & 3) == 3) && (tmr_8253[2].countmax != 1))
        {
            TRACE("Beep (freq: %d) !\n", 1193180 / tmr_8253[2].countmax);
            Beep(1193180 / tmr_8253[2].countmax, 20);
        }
        break;
    case 0x70:
        cmosaddress = (BYTE)value & 0x7f;
        break;
    case 0x71:
        cmosimage[cmosaddress & 0x3f] = (BYTE)value;
        break;
    default:
        WARN("Direct I/O write attempted to port %x\n", port );
        break;
    }
}
