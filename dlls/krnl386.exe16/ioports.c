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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_PPDEV
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_IOCTL_H
# include <linux/ioctl.h>
#endif
#include <linux/ppdev.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "winternl.h"
#include "kernel16_private.h"
#include "dosexe.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

#if defined(linux) && !defined(__ANDROID__)
# define DIRECT_IO_ACCESS
#else
# undef DIRECT_IO_ACCESS
#endif

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

#ifdef DIRECT_IO_ACCESS

extern int iopl(int level);
static char do_direct_port_access = -1;
static char port_permissions[0x10000];

#define IO_READ  1
#define IO_WRITE 2

#endif  /* DIRECT_IO_ACCESS */

#ifdef HAVE_PPDEV
static int do_pp_port_access = -1; /* -1: uninitialized, 1: not available
				       0: available);*/
#endif

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
                TRACE("Beep (freq: %d) !\n", 1193180 / val);
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
    int val, val1;
    unsigned int i;
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
    HANDLE root, hkey;
    DWORD dummy;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    static const WCHAR portsW[] = {'S','o','f','t','w','a','r','e','\\',
                                   'W','i','n','e','\\','V','D','M','\\','P','o','r','t','s',0};
    static const WCHAR readW[] = {'r','e','a','d',0};
    static const WCHAR writeW[] = {'w','r','i','t','e',0};

    do_direct_port_access = 0;
    /* Can we do that? */
    if (!iopl(3))
    {
        iopl(0);

        RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
        attr.Length = sizeof(attr);
        attr.RootDirectory = root;
        attr.ObjectName = &nameW;
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        RtlInitUnicodeString( &nameW, portsW );

        /* @@ Wine registry key: HKCU\Software\Wine\VDM\Ports */
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
        NtClose( root );
    }
}

#endif  /* DIRECT_IO_ACCESS */


#ifdef HAVE_PPDEV

typedef struct _PPDEVICESTRUCT{
  int fd; /* NULL if device not available */
  char *devicename;
  int userbase; /* where wine thinks the ports are */
  DWORD lastaccess; /* or NULL if release */
  int timeout; /* time in second of inactivity to release the port */
} PPDeviceStruct;

static PPDeviceStruct PPDeviceList[5];
static int PPDeviceNum=0;

static int IO_pp_sort(const void *p1,const  void *p2)
{
    return ((const PPDeviceStruct*)p1)->userbase - ((const PPDeviceStruct*)p2)->userbase;
}

/* IO_pp_init
 *
 * Read the ppdev entries from registry, open the device and check
 * for necessary IOCTRL
 * Report verbose about possible errors
 */
static char IO_pp_init(void)
{
    char name[80];
    char buffer[256];
    HANDLE root, hkey;
    int i,idx=0,fd,res,userbase,nports=0;
    char * timeout;
    char ret=1;
    int lasterror;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    static const WCHAR configW[] = {'S','o','f','t','w','a','r','e','\\',
                                    'W','i','n','e','\\','V','D','M','\\','p','p','d','e','v',0};

    TRACE("\n");

    RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, configW );

    /* @@ Wine registry key: HKCU\Software\Wine\VDM\ppdev */
    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) hkey = 0;
    NtClose( root );
    if (!hkey) return 1;

    for (;;)
    {
        DWORD total_size, len;
        char temp[256];
        KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)temp;

        if (NtEnumerateValueKey( hkey, idx, KeyValueFullInformation,
                                 temp, sizeof(temp), &total_size )) break;
        if (info->Type != REG_SZ) break;

        RtlUnicodeToMultiByteN( name, sizeof(name)-1, &len, info->Name, info->NameLength );
        name[len] = 0;
        RtlUnicodeToMultiByteN( buffer, sizeof(buffer)-1, &len,
                                (WCHAR *)(temp + info->DataOffset), total_size-info->DataOffset );
        buffer[len] = 0;

	idx++;
	if(nports >4)
	  {
	    FIXME("Make the PPDeviceList larger than 5 elements\n");
	    break;
	  }
	TRACE("Device '%s' at virtual userbase '%s'\n", buffer,name);
	timeout = strchr(buffer,',');
	if (timeout)
	  *timeout++=0;
	fd=open(buffer,O_RDWR);
	lasterror=errno;
	if (fd == -1)
	  {
	    WARN("Configuration: No access to %s Cause: %s\n",buffer,strerror(lasterror));
	    WARN("Rejecting configuration item\n");
	    if (lasterror == ENODEV)
	      ERR("Is the ppdev module loaded?\n");
	    continue;
	  }
        userbase = strtol(name, NULL, 16);
	if ( errno == ERANGE)
	  {
	    WARN("Configuration: Invalid base %s for %s\n",name,buffer);
	    WARN("Rejecting configuration item\n");
	    continue;
	  }
	if (ioctl (fd,PPCLAIM,0))
	  {
	    ERR("PPCLAIM rejected %s\n",buffer);
	    ERR("Perhaps the device is already in use or nonexistent\n");
	    continue;
	  }
	if (nports > 0)
	  {
	    for (i=0; i<= nports; i++)
	      {
		if (PPDeviceList[i].userbase == userbase)
		  {
		    WARN("Configuration: %s uses the same virtual ports as %s\n",
			 buffer,PPDeviceList[0].devicename);
		    WARN("Configuration: Rejecting configuration item\n");
		    userbase = 0;
		    break;
		  }
	      }
	    if (!userbase) continue;
	  }
	/* Check for the minimum required IOCTLS */
	if ((ioctl(fd,PPRDATA,&res))||
	    (ioctl(fd,PPRSTATUS,&res))||
	    (ioctl(fd,PPRCONTROL,&res)))
	  {
	    ERR("PPUSER IOCTL not available for parport device %s\n",buffer);
	    continue;
	  }
	if (ioctl (fd,PPRELEASE,0))
	  {
	    ERR("PPRELEASE rejected %s\n",buffer);
	    ERR("Perhaps the device is already in use or nonexistent\n");
	    continue;
	  }
	PPDeviceList[nports].devicename = HeapAlloc(GetProcessHeap(), 0, sizeof(buffer)+1);
	if (!PPDeviceList[nports].devicename)
	  {
	    ERR("No (more) space for devicename\n");
	    break;
	  }
	strcpy(PPDeviceList[nports].devicename,buffer);
	PPDeviceList[nports].fd = fd;
	PPDeviceList[nports].userbase = userbase;
	PPDeviceList[nports].lastaccess=GetTickCount();
	if (timeout)
	  {
            PPDeviceList[nports].timeout = strtol(timeout, NULL, 10);
	    if (errno == ERANGE)
	      {
		WARN("Configuration: Invalid timeout %s in configuration for %s, Setting to 0\n",
		     timeout,buffer);
		PPDeviceList[nports].timeout = 0;
	      }
	  }
	else
	  PPDeviceList[nports].timeout = 0;
	nports++;
    }
    TRACE("found %d ports\n",nports);
    NtClose( hkey );

    PPDeviceNum= nports;
    if (nports > 1)
      /* sort in ascending order for userbase for faster access */
      qsort (PPDeviceList,PPDeviceNum,sizeof(PPDeviceStruct),IO_pp_sort);

    if (nports)
      ret=0;
    for (idx= 0;idx<PPDeviceNum; idx++)
      TRACE("found device %s userbase %x fd %x timeout %d\n",
	    PPDeviceList[idx].devicename, PPDeviceList[idx].userbase,
	    PPDeviceList[idx].fd,PPDeviceList[idx].timeout);
    /* FIXME:
       register a timer callback perhaps every 30 seconds to release unused ports
       Set lastaccess = 0 as indicator when port was released
    */
    return ret;
}

/* IO_pp_do_access
 *
 * Do the actual IOCTL
 * Return NULL on success
 */
static int IO_pp_do_access(int idx,int ppctl, DWORD* res)
{
  int ret;
  if (ioctl(PPDeviceList[idx].fd,PPCLAIM,0))
    {
      ERR("Can't reclaim device %s, PPUSER/PPDEV handling confused\n",
	  PPDeviceList[idx].devicename);
      return 1;
    }
  ret = ioctl(PPDeviceList[idx].fd,ppctl,res);
  if (ioctl(PPDeviceList[idx].fd,PPRELEASE,0))
    {
      ERR("Can't release device %s, PPUSER/PPDEV handling confused\n",
	  PPDeviceList[idx].devicename);
      return 1;
    }
  return ret;

}

/* IO_pp_inp
 *
 * Check if we can satisfy the INP command with some of the configured PPDEV device
 * Return NULL on success
 */
static int IO_pp_inp(int port, DWORD* res)
{
    int idx,j=0;

    for (idx=0;idx<PPDeviceNum ;idx++)
      {
       j = port - PPDeviceList[idx].userbase;
       if (j <0) return 1;
       switch (j)
         {
         case 0:
           return IO_pp_do_access(idx,PPRDATA,res);
         case 1:
           return IO_pp_do_access(idx,PPRSTATUS,res);
         case 2:
           return IO_pp_do_access(idx,PPRCONTROL,res);
         case 0x400:
         case 0x402:
         case 3:
         case 4:
         case 0x401:
           FIXME("Port 0x%x not accessible for reading with ppdev\n",port);
           FIXME("If this is causing problems, try direct port access\n");
           return 1;
         default:
           break;
         }
      }
    return 1;
}

/* IO_pp_outp
 *
 * Check if we can satisfy the OUTP command with some of the configured PPDEV device
 * Return NULL on success
 */
static BOOL IO_pp_outp(int port, DWORD* res)
{
    int idx,j=0;

    for (idx=0;idx<PPDeviceNum ;idx++)
      {
       j = port - PPDeviceList[idx].userbase;
       if (j <0) return 1;
       switch (j)
         {
         case 0:
           return IO_pp_do_access(idx,PPWDATA,res);
         case 2:
	   {
	     /* We can't switch port direction via PPWCONTROL,
		so do it via PPDATADIR
	     */
	     DWORD mode = *res & 0x20;
	     IO_pp_do_access(idx,PPDATADIR,&mode);
	     mode = (*res & ~0x20);
	     return IO_pp_do_access(idx,PPWCONTROL,&mode);
	   }

         case 1:
         case 0x400:
         case 0x402:
         case 3:
         case 4:
         case 0x401:
           FIXME("Port %d not accessible for writing with ppdev\n",port);
           FIXME("If this is causing problems, try direct port access\n");
           return 1;
         default:
           break;
         }
      }
    return TRUE;
}

#endif  /* HAVE_PPDEV */


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

#ifdef HAVE_PPDEV
    if (do_pp_port_access == -1) do_pp_port_access =IO_pp_init();
    if ((do_pp_port_access == 0 ) && (size == 1))
    {
        if (!IO_pp_inp(port,&res)) return res;
    }
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
        }
        iopl(0);
        return res;
    }
#endif

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
    case 0x22a:
    case 0x22c:
    case 0x22e:
        res = (DWORD)SB_ioport_in( port );
	break;
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0xC0:
    case 0xC2:
    case 0xC4:
    case 0xC6:
    case 0xC8:
    case 0xCA:
    case 0xCC:
    case 0xCE:
    case 0x87:
    case 0x83:
    case 0x81:
    case 0x82:
    case 0x8B:
    case 0x89:
    case 0x8A:
    case 0x487:
    case 0x483:
    case 0x481:
    case 0x482:
    case 0x48B:
    case 0x489:
    case 0x48A:
    case 0x08:
    case 0xD0:
    case 0x0D:
    case 0xDA:
        res = (DWORD)DMA_ioport_in( port );
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
    TRACE("IO: 0x%x (%d-byte value) to port 0x%04x\n", value, size, port );

    DOSMEM_InitDosMemory();

#ifdef HAVE_PPDEV
    if (do_pp_port_access == -1) do_pp_port_access = IO_pp_init();
    if ((do_pp_port_access == 0) && (size == 1))
    {
        if (!IO_pp_outp(port,&value)) return;
    }
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
        }
        iopl(0);
        return;
    }
#endif

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
    case 0x226:
    case 0x22c:
        SB_ioport_out( port, (BYTE)value );
        break;
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0xC0:
    case 0xC2:
    case 0xC4:
    case 0xC6:
    case 0xC8:
    case 0xCA:
    case 0xCC:
    case 0xCE:
    case 0x87:
    case 0x83:
    case 0x81:
    case 0x82:
    case 0x8B:
    case 0x89:
    case 0x8A:
    case 0x487:
    case 0x483:
    case 0x481:
    case 0x482:
    case 0x48B:
    case 0x489:
    case 0x48A:
    case 0x08:
    case 0xD0:
    case 0x0B:
    case 0xD6:
    case 0x0A:
    case 0xD4:
    case 0x0F:
    case 0xDE:
    case 0x09:
    case 0xD2:
    case 0x0C:
    case 0xD8:
    case 0x0D:
    case 0xDA:
    case 0x0E:
    case 0xDC:
        DMA_ioport_out( port, (BYTE)value );
        break;
    default:
        WARN("Direct I/O write attempted to port %x\n", port );
        break;
    }
}
