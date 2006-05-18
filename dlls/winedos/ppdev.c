/*
 * Parallel port device support
 *
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

#include "config.h"
#include "wine/port.h"

#include "windef.h"

#ifdef HAVE_PPDEV

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_IOCTL_H
# include <linux/ioctl.h>
#endif
#include <linux/ppdev.h>

#include "winerror.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

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
 * Read the ppdev entries from wine.conf, open the device and check
 * for necessary IOCTRL
 * Report verbose about possible errors
 */
char IO_pp_init(void)
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
	userbase = strtol(name,(char **)NULL, 16);
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
	PPDeviceList[nports].devicename = malloc(sizeof(buffer)+1);
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
	    PPDeviceList[nports].timeout = strtol(timeout,(char **)NULL, 10);
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
 * Check if we can satisfy the INP command with some of the configured PPDEV deviced
 * Return NULL on success
 */
int IO_pp_inp(int port, DWORD* res)
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
 * Check if we can satisfy the OUTP command with some of the configured PPDEV deviced
 * Return NULL on success
 */
BOOL IO_pp_outp(int port, DWORD* res)
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


#else /* HAVE_PPDEV */


char IO_pp_init(void)
{
  return 1;
}

int IO_pp_inp(int port, DWORD* res)
{
  return 1;
}

BOOL IO_pp_outp(int port, DWORD* res)
{
  return TRUE;
}
#endif  /* HAVE_PPDEV */
