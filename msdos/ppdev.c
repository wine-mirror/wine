/*
 * Parallel-port device support
 */

#include "config.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(int);

#ifdef  HAVE_PPDEV

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/ppdev.h>

#include "winerror.h"
#include "winreg.h"


typedef struct _PPDEVICESTRUCT{
  int fd; /* NULL if device not available */
  char *devicename;
  int userbase; /* where wine thinks the ports are*/
  DWORD lastaccess; /* or NULL if release */
  int timeout; /* time in second of inactivity to release the port*/
} PPDeviceStruct;

static PPDeviceStruct PPDeviceList[5];
static int PPDeviceNum=0;

static int IO_pp_sort(const void *p1,const  void *p2)
{
  return ((PPDeviceStruct*)p1)->userbase - ((PPDeviceStruct*)p2)->userbase;
}

/* IO_pp_init 
 *
 * Read the ppdev entries from wine.conf, open the device and check
 * for nescessary IOCTRL
 * Report verbose about possible errors
 */
char IO_pp_init(void)
{
    char name[80];
    char buffer[1024];
    HKEY hkey;
    char temp[256];
    int i,idx=0,fd,res,userbase,nports=0;
    char * timeout;
    char ret=1;
    int lasterror;

    TRACE("\n");
    if (RegOpenKeyA( HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\ppdev", &hkey ) != ERROR_SUCCESS)
      return 1;

    for (;;)
    {
        DWORD type, count = sizeof(buffer), name_len = sizeof(name);

        if (RegEnumValueA( hkey, idx, name, &name_len, NULL, &type, buffer, &count )!= ERROR_SUCCESS) 
	  break;
	
	idx++;
	if(nports >4)
	  {
	    FIXME("Make the PPDeviceList larger then 5 elements\n");
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
	      FIXME("Is the ppdev module loaded?\n");
	    continue;
	  }
	userbase = strtol(name,(char **)NULL, 16);
	if ( errno == ERANGE)
	  {
	    WARN("Configuration: Invalid base %s for %s\n",name,buffer);
	    WARN("Rejecting configuration item\n");
	    continue;
	  }
	if (ioctl (fd,PPEXCL,0)) 
	  {
	    ERR("PPEXCL IOCTL rejected for device %s\n",buffer);
	    ERR("Check that PPDEV module is loaded!\n");
	    ERR("Perhaps the device is already in use or non-existant\n");
	    continue;
	  }
	if (ioctl (fd,PPCLAIM,0)) 
	  {
	    ERR("PPCLAIM rejected %s\n",buffer);
	    ERR("Perhaps the device is already in use or non-existant\n");
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
		    WARN("Configuration: Rejecting configuration item");
		    userbase = 0;
		    break;
		  }
	      }
	    if (!userbase) continue;
	  }
	/* Check for the minimum required IOCTLS */
	if ((ioctl(fd,PPRDATA,&res))||
	    (ioctl(fd,PPRCONTROL,&res))||
	    (ioctl(fd,PPRCONTROL,&res)))
	  {
	    ERR("PPUSER IOCTL not available for parport device %s\n",temp);
	    continue;
	  }
	PPDeviceList[nports].devicename = malloc(sizeof(buffer)+1);
	if (!PPDeviceList[nports].devicename)
	  {
	    ERR("No (more)space for devicename\n");
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
		WARN("Configuration:Invalid timeout %s in configuration for %s, Setting to 0\n",
		     timeout,buffer);
		PPDeviceList[nports].timeout = 0;
	      }
	  }
	else
	  PPDeviceList[nports].timeout = 0;
	nports++;
    }
    TRACE("found %d ports\n",nports);
    RegCloseKey( hkey );

    PPDeviceNum= nports;
    if (nports > 1)
      /* sort in accending order for userbase for faster access*/
      qsort (PPDeviceList,PPDeviceNum,sizeof(PPDeviceStruct),IO_pp_sort);
    
    if (nports)
      ret=0;
    for (idx= 0;idx<PPDeviceNum; idx++)
      TRACE("found device %s userbase %x fd %x timeout %d\n",
	    PPDeviceList[idx].devicename, PPDeviceList[idx].userbase,
	    PPDeviceList[idx].fd,PPDeviceList[idx].timeout);
    /* FIXME:
       register a timer callback perhaps every 30 second to release unused ports
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
    if (!PPDeviceList[idx].lastaccess)
      /* TIMER callback has released the device after some time of inactivity
          Reclaim the device
          !!!!!!THIS MAY UNINTERRUPTIPLE BLOCK IF SOME OTHER DEVICE
           !!!!!! HAS CLAIMED THE SAME PORT
      */
      if (ioctl(PPDeviceList[idx].fd,PPCLAIM,0))
       {
         ERR("Can't reclaim device %s, PPUSER/PPDEV handling confused\n",
             PPDeviceList[idx].devicename);
         return 1;
       }
    PPDeviceList[idx].lastaccess=GetTickCount();
    return ioctl(PPDeviceList[idx].fd,ppctl,res);
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
 * Check if we can satisfy the INP command with some of the configured PPDEV deviced
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
           return IO_pp_do_access(idx,PPWCONTROL,res);
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
