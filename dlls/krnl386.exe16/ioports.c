/*
 * Emulation of processor ioports.
 *
 * Copyright 1995 Morten Welinder
 * Copyright 1998 Andreas Mohr, Ove Kaaven
 * Copyright 2001 Uwe Bonnes
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Known problems:
   - only a few ports are emulated.
   - real-time clock in "cmos" is bogus.  A nifty alarm() setup could
     fix that, I guess.
*/

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "kernel16_private.h"
#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

static struct {
    WORD	countmax;
    WORD	latch;
    BYTE	ctrlbyte_ch;
    BYTE	flags;
    LONG64	start_time;
} tmr_8253[3] = {
    {0xFFFF,	0,	0x36,	0,	0},
    {0x0012,	0,	0x74,	0,	0},
    {0x0001,	0,	0xB6,	0,	0},
};
/* two byte read in progress */
#define TMR_RTOGGLE 0x01
/* two byte write in progress */
#define TMR_WTOGGLE 0x02
/* latch contains data */
#define TMR_LATCHED 0x04
/* counter is in update phase */
#define TMR_UPDATE  0x08
/* readback status request */
#define TMR_STATUS  0x10


static BYTE parport_8255[4] = {0x4f, 0x20, 0xff, 0xff};

static BYTE cmosaddress;

static BOOL cmos_image_initialized = FALSE;

static BYTE cmosimage[64] =
{
  0x27, /* 0x00: seconds */
  0x34, /* 0X01: seconds alarm */
  0x31, /* 0x02: minutes */
  0x47, /* 0x03: minutes alarm */
  0x16, /* 0x04: hour */
  0x15, /* 0x05: hour alarm */
  0x00, /* 0x06: week day */
  0x01, /* 0x07: month day */
  0x04, /* 0x08: month */
  0x94, /* 0x09: year */
  0x26, /* 0x0a: state A */
  0x02, /* 0x0b: state B */
  0x50, /* 0x0c: state C */
  0x80, /* 0x0d: state D */
  0x00, /* 0x0e: state diagnostic */
  0x00, /* 0x0f: state state shutdown */
  0x40, /* 0x10: floppy type */
  0xb1, /* 0x11: reserved */
  0x00, /* 0x12: HD type */
  0x9c, /* 0x13: reserved */
  0x01, /* 0x14: equipment */
  0x80, /* 0x15: low base memory */
  0x02, /* 0x16: high base memory (0x280 => 640KB) */
  0x00, /* 0x17: low extended memory */
  0x3b, /* 0x18: high extended memory (0x3b00 => 15MB) */
  0x00, /* 0x19: HD 1 extended type byte */
  0x00, /* 0x1a: HD 2 extended type byte */
  0xad, /* 0x1b: reserved */
  0x02, /* 0x1c: reserved */
  0x10, /* 0x1d: reserved */
  0x00, /* 0x1e: reserved */
  0x00, /* 0x1f: installed features */
  0x08, /* 0x20: HD 1 low cylinder number */
  0x00, /* 0x21: HD 1 high cylinder number */
  0x00, /* 0x22: HD 1 heads */
  0x26, /* 0x23: HD 1 low pre-compensation start */
  0x00, /* 0x24: HD 1 high pre-compensation start */
  0x00, /* 0x25: HD 1 low landing zone */
  0x00, /* 0x26: HD 1 high landing zone */
  0x00, /* 0x27: HD 1 sectors */
  0x00, /* 0x28: options 1 */
  0x00, /* 0x29: reserved */
  0x00, /* 0x2a: reserved */
  0x00, /* 0x2b: options 2 */
  0x00, /* 0x2c: options 3 */
  0x3f, /* 0x2d: reserved  */
  0xcc, /* 0x2e: low CMOS ram checksum (computed automatically) */
  0xcc, /* 0x2f: high CMOS ram checksum (computed automatically) */
  0x00, /* 0x30: low extended memory byte */
  0x1c, /* 0x31: high extended memory byte */
  0x19, /* 0x32: century byte */
  0x81, /* 0x33: setup information */
  0x00, /* 0x34: CPU speed */
  0x0e, /* 0x35: HD 2 low cylinder number */
  0x00, /* 0x36: HD 2 high cylinder number */
  0x80, /* 0x37: HD 2 heads */
  0x1b, /* 0x38: HD 2 low pre-compensation start */
  0x7b, /* 0x39: HD 2 high pre-compensation start */
  0x21, /* 0x3a: HD 2 low landing zone */
  0x00, /* 0x3b: HD 2 high landing zone */
  0x00, /* 0x3c: HD 2 sectors */
  0x00, /* 0x3d: reserved */
  0x05, /* 0x3e: reserved */
  0x5f  /* 0x3f: reserved */
};

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

#define BCD2BIN(a) \
((a)%10 + ((a)>>4)%10*10 + ((a)>>8)%10*100 + ((a)>>12)%10*1000)
#define BIN2BCD(a) \
((a)%10 | (a)/10%10<<4 | (a)/100%10<<8 | (a)/1000%10<<12)


static void set_timer(unsigned timer)
{
    DWORD val = tmr_8253[timer].countmax;

    if (tmr_8253[timer].ctrlbyte_ch & 0x01)
        val = BCD2BIN(val);

    tmr_8253[timer].flags &= ~TMR_UPDATE;
    if (!QueryPerformanceCounter((LARGE_INTEGER*)&tmr_8253[timer].start_time))
        WARN("QueryPerformanceCounter should not fail!\n");

    switch (timer) {
        case 0: /* System timer counter divisor */
            break;
        case 1: /* RAM refresh */
            FIXME("RAM refresh counter handling not implemented !\n");
            break;
        case 2: /* cassette & speaker */
            /* speaker on ? */
            if ((parport_8255[1] & 3) == 3)
            {
                TRACE("Beep (freq: %ld) !\n", 1193180 / val);
                Beep(1193180 / val, 20);
            }
            break;
    }
}


static WORD get_timer_val(unsigned timer)
{
    LARGE_INTEGER time;
    WORD maxval, val = tmr_8253[timer].countmax;
    BYTE mode = tmr_8253[timer].ctrlbyte_ch >> 1 & 0x07;

    /* This is not strictly correct. In most cases the old countdown should
     * finish normally (by counting down to 0) or halt and not jump to 0.
     * But we are calculating and not counting, so this seems to be a good
     * solution and should work well with most (all?) programs
     */
    if (tmr_8253[timer].flags & TMR_UPDATE)
        return 0;

    if (!QueryPerformanceCounter(&time))
        WARN("QueryPerformanceCounter should not fail!\n");

    time.QuadPart -= tmr_8253[timer].start_time;
    if (tmr_8253[timer].ctrlbyte_ch & 0x01)
        val = BCD2BIN(val);

    switch ( mode )
    {
        case 0:
        case 1:
        case 4:
        case 5:
            maxval = tmr_8253[timer].ctrlbyte_ch & 0x01 ? 9999 : 0xFFFF;
            break;
        case 2:
        case 3:
            maxval = val;
            break;
        default:
            ERR("Invalid PIT mode: %d\n", mode);
            return 0;
    }

    val = (val - time.QuadPart) % (maxval + 1);
    if (tmr_8253[timer].ctrlbyte_ch & 0x01)
        val = BIN2BCD(val);

    return val;
}



/**********************************************************************
 *	    DOSVM_inport
 *
 * Note: The size argument has to be handled correctly _externally_
 * (as we always return a DWORD)
 */
DWORD DOSVM_inport( int port, int size )
{
    DWORD res = ~0U;

    TRACE("%d-byte value from port 0x%04x\n", size, port );

    DOSMEM_InitDosMemory();

    switch (port)
    {
    case 0x40:
    case 0x41:
    case 0x42:
        {
            BYTE chan = port & 3;
            WORD tempval = tmr_8253[chan].flags & TMR_LATCHED
                ? tmr_8253[chan].latch : get_timer_val(chan);

            if (tmr_8253[chan].flags & TMR_STATUS)
            {
                WARN("Read-back status\n");
                /* We differ slightly from the spec:
                 * - TMR_UPDATE is already set with the first write
                 *   of a two byte counter update
                 * - 0x80 should be set if OUT signal is 1 (high)
                 */
                tmr_8253[chan].flags &= ~TMR_STATUS;
                res = (tmr_8253[chan].ctrlbyte_ch & 0x3F) |
                    (tmr_8253[chan].flags & TMR_UPDATE ? 0x40 : 0x00);
                break;
            }
            switch ((tmr_8253[chan].ctrlbyte_ch & 0x30) >> 4)
            {
            case 0:
                res = 0; /* shouldn't happen? */
                break;
            case 1: /* read lo byte */
                res = (BYTE)tempval;
                tmr_8253[chan].flags &= ~TMR_LATCHED;
                break;
            case 3: /* read lo byte, then hi byte */
                tmr_8253[chan].flags ^= TMR_RTOGGLE; /* toggle */
                if (tmr_8253[chan].flags & TMR_RTOGGLE)
                {
                    res = (BYTE)tempval;
                    break;
                }
                /* else [fall through if read hi byte !] */
            case 2: /* read hi byte */
                res = (BYTE)(tempval >> 8);
                tmr_8253[chan].flags &= ~TMR_LATCHED;
                break;
            }
        }
        break;
    case 0x60:
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
        if (!cmos_image_initialized)
        {
            IO_FixCMOSCheckSum();
            cmos_image_initialized = TRUE;
        }
        res = (DWORD)cmosimage[cmosaddress & 0x3f];
        break;
    case 0x200:
    case 0x201:
        res = ~0U; /* no joystick */
        break;
    case 0x3da:
        res = GetTickCount() % 17 == 0 ? 0x4 : 0; /* report vblank about 60 times per second */
        break;
    default:
        WARN("Direct I/O read attempted from port %x\n", port);
        break;
    }
    return res;
}


/**********************************************************************
 *	    DOSVM_outport
 */
void DOSVM_outport( int port, int size, DWORD value )
{
    TRACE("IO: 0x%lx (%d-byte value) to port 0x%04x\n", value, size, port );

    DOSMEM_InitDosMemory();

    switch (port)
    {
    case 0x20:
        break;
    case 0x40:
    case 0x41:
    case 0x42:
        {
            BYTE chan = port & 3;

            tmr_8253[chan].flags |= TMR_UPDATE;
            switch ((tmr_8253[chan].ctrlbyte_ch & 0x30) >> 4)
            {
            case 0:
                break; /* shouldn't happen? */
            case 1: /* write lo byte */
                tmr_8253[chan].countmax =
                    (tmr_8253[chan].countmax & 0xff00) | (BYTE)value;
                break;
            case 3: /* write lo byte, then hi byte */
                tmr_8253[chan].flags ^= TMR_WTOGGLE; /* toggle */
                if (tmr_8253[chan].flags & TMR_WTOGGLE)
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
            /* if programming is finished, update to new value */
            if ((tmr_8253[chan].ctrlbyte_ch & 0x30) &&
                !(tmr_8253[chan].flags & TMR_WTOGGLE))
                set_timer(chan);
        }
        break;
    case 0x43:
       {
           BYTE chan = ((BYTE)value & 0xc0) >> 6;
           /* ctrl byte for specific timer channel */
           if (chan == 3)
           {
               if ( !(value & 0x20) )
               {
                   if ((value & 0x02) && !(tmr_8253[0].flags & TMR_LATCHED))
                   {
                       tmr_8253[0].flags |= TMR_LATCHED;
                       tmr_8253[0].latch = get_timer_val(0);
                   }
                   if ((value & 0x04) && !(tmr_8253[1].flags & TMR_LATCHED))
                   {
                       tmr_8253[1].flags |= TMR_LATCHED;
                       tmr_8253[1].latch = get_timer_val(1);
                   }
                   if ((value & 0x08) && !(tmr_8253[2].flags & TMR_LATCHED))
                   {
                       tmr_8253[2].flags |= TMR_LATCHED;
                       tmr_8253[2].latch = get_timer_val(2);
                   }
               }

               if ( !(value & 0x10) )
               {
                   if (value & 0x02)
                       tmr_8253[0].flags |= TMR_STATUS;
                   if (value & 0x04)
                       tmr_8253[1].flags |= TMR_STATUS;
                   if (value & 0x08)
                       tmr_8253[2].flags |= TMR_STATUS;
               }
               break;
           }
           switch (((BYTE)value & 0x30) >> 4)
           {
           case 0:	/* latch timer */
               if ( !(tmr_8253[chan].flags & TMR_LATCHED) )
               {
                   tmr_8253[chan].flags |= TMR_LATCHED;
                   tmr_8253[chan].latch = get_timer_val(chan);
               }
               break;
           case 1:	/* write lo byte only */
           case 2:	/* write hi byte only */
           case 3:	/* write lo byte, then hi byte */
               tmr_8253[chan].ctrlbyte_ch = (BYTE)value;
               tmr_8253[chan].countmax = 0;
               tmr_8253[chan].flags = TMR_UPDATE;
               break;
           }
       }
       break;
    case 0x61:
        parport_8255[1] = (BYTE)value;
        if (((parport_8255[1] & 3) == 3) && (tmr_8253[2].countmax != 1))
        {
            TRACE("Beep (freq: %d) !\n", 1193180 / tmr_8253[2].countmax);
            Beep(1193180 / tmr_8253[2].countmax, 20);
        }
        break;
    case 0x70:
        cmosaddress = (BYTE)value & 0x7f;
        break;
    case 0x71:
        if (!cmos_image_initialized)
        {
            IO_FixCMOSCheckSum();
            cmos_image_initialized = TRUE;
        }
        cmosimage[cmosaddress & 0x3f] = (BYTE)value;
        break;
    default:
        WARN("Direct I/O write attempted to port %x\n", port );
        break;
    }
}
