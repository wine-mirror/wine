/**************************************************************************
 * ASPI routines
 * Copyright (C) 2000 David Elliott <dfe@infinite-internet.net>
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

/* These routines are to be called from either WNASPI32 or WINASPI */

/* FIXME:
 * - Registry format is stupid for now.. fix that later
 * - No way to override automatic /proc detection, maybe provide an
 *   HKEY_LOCAL_MACHINE\Software\Wine\Wine\Scsi regkey
 * - Somewhat debating an #ifdef linux... technically all this code will
 *   run on another UNIX.. it will fail nicely.
 * - Please add support for mapping multiple channels on host adapters to
 *   aspi controllers, e-mail me if you need help.
 */

/*
Registry format is currently:
HKEY_DYN_DATA
	WineScsi
		(default)=number of host adapters
		hHHcCCtTTdDD=linux device name
*/

#include "config.h"

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <fcntl.h>
#include <dirent.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winescsi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(aspi);

/* Internal function prototypes */
static void
SCSI_GetProcinfo();

#ifdef linux
static void
SCSI_MapHCtoController();
#endif

static void set_last_error(void)
{
    int save_errno = errno; /* errno gets overwritten by printf */
    switch (errno)
    {
    case EAGAIN:
        SetLastError( ERROR_SHARING_VIOLATION );
        break;
    case EBADF:
        SetLastError( ERROR_INVALID_HANDLE );
        break;
    case ENOSPC:
        SetLastError( ERROR_HANDLE_DISK_FULL );
        break;
    case EACCES:
    case EPERM:
    case EROFS:
        SetLastError( ERROR_ACCESS_DENIED );
        break;
    case EBUSY:
        SetLastError( ERROR_LOCK_VIOLATION );
        break;
    case ENOENT:
        SetLastError( ERROR_FILE_NOT_FOUND );
        break;
    case EISDIR:
        SetLastError( ERROR_CANNOT_MAKE );
        break;
    case ENFILE:
    case EMFILE:
        SetLastError( ERROR_NO_MORE_FILES );
        break;
    case EEXIST:
        SetLastError( ERROR_FILE_EXISTS );
        break;
    case EINVAL:
    case ESPIPE:
        SetLastError( ERROR_SEEK );
        break;
    case ENOTEMPTY:
        SetLastError( ERROR_DIR_NOT_EMPTY );
        break;
    case ENOEXEC:
        SetLastError( ERROR_BAD_FORMAT );
        break;
    default:
        WARN( "unknown file error: %s\n", strerror(save_errno) );
        SetLastError( ERROR_GEN_FAILURE );
        break;
    }
    errno = save_errno;
}

/* Exported functions */
void
SCSI_Init()
{
	/* For now we just call SCSI_GetProcinfo */
	SCSI_GetProcinfo();
#ifdef linux
	SCSI_MapHCtoController();
#endif
}

int
ASPI_GetNumControllers()
{
	HKEY hkeyScsi;
	HKEY hkeyControllerMap;
	DWORD error;
	DWORD type = REG_DWORD;
	DWORD num_ha = 0;
	DWORD cbData = sizeof(num_ha);

	if( RegOpenKeyExA(HKEY_DYN_DATA, KEYNAME_SCSI, 0, KEY_ALL_ACCESS, &hkeyScsi ) != ERROR_SUCCESS )
	{
		ERR("Could not open HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
		return 0;
	}

	if( (error=RegOpenKeyExA(hkeyScsi, KEYNAME_SCSI_CONTROLLERMAP, 0, KEY_ALL_ACCESS, &hkeyControllerMap )) != ERROR_SUCCESS )
	{
		ERR("Could not open HKEY_DYN_DATA\\%s\\%s\n", KEYNAME_SCSI, KEYNAME_SCSI_CONTROLLERMAP);
		RegCloseKey(hkeyScsi);
		SetLastError(error);
		return 0;
	}
	if( RegQueryValueExA(hkeyControllerMap, NULL, NULL, &type, (LPBYTE)&num_ha, &cbData ) != ERROR_SUCCESS )
	{
		ERR("Could not query value HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
		num_ha=0;
	}
	RegCloseKey(hkeyControllerMap);
	RegCloseKey(hkeyScsi);
	TRACE("Returning %ld host adapters\n", num_ha );
	return num_ha;
}

BOOL
SCSI_GetDeviceName( int h, int c, int t, int d, LPSTR devstr, LPDWORD lpcbData )
{

	char idstr[20];
	HKEY hkeyScsi;
	DWORD type;

	if( RegOpenKeyExA(HKEY_DYN_DATA, KEYNAME_SCSI, 0, KEY_ALL_ACCESS, &hkeyScsi ) != ERROR_SUCCESS )
	{
		ERR("Could not open HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
		return FALSE;
	}


	sprintf(idstr, "h%02dc%02dt%02dd%02d", h, c, t, d);

	if( RegQueryValueExA(hkeyScsi, idstr, NULL, &type, devstr, lpcbData) != ERROR_SUCCESS )
	{
		WARN("Could not query value HKEY_DYN_DATA\\%s\\%s\n",KEYNAME_SCSI, idstr);
		RegCloseKey(hkeyScsi);
		return FALSE;
	}
	RegCloseKey(hkeyScsi);

	TRACE("scsi %s: Device name: %s\n",idstr,devstr);
	return TRUE;
}

/* SCSI_GetHCforController
 * RETURNS
 * 	HIWORD: Host Adapter
 * 	LOWORD: Channel
 */
DWORD
ASPI_GetHCforController( int controller )
{
	DWORD hc = 0xFFFFFFFF;
	char cstr[20];
	DWORD error;
	HKEY hkeyScsi;
	HKEY hkeyControllerMap;
	DWORD type = REG_DWORD;
	DWORD cbData = sizeof(DWORD);
	DWORD disposition;
#if 0
	FIXME("Please fix to map each channel of each host adapter to the proper ASPI controller number!\n");
	hc = (controller << 16);
	return hc;
#endif
	if( (error=RegCreateKeyExA(HKEY_DYN_DATA, KEYNAME_SCSI, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyScsi, &disposition )) != ERROR_SUCCESS )
	{
		ERR("Could not open HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
		SetLastError(error);
		return hc;
	}
	if( disposition != REG_OPENED_EXISTING_KEY )
	{
		WARN("Created HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
	}
	if( (error=RegCreateKeyExA(hkeyScsi, KEYNAME_SCSI_CONTROLLERMAP, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyControllerMap, &disposition )) != ERROR_SUCCESS )
	{
		ERR("Could not open HKEY_DYN_DATA\\%s\\%s\n", KEYNAME_SCSI, KEYNAME_SCSI_CONTROLLERMAP);
		RegCloseKey(hkeyScsi);
		SetLastError(error);
		return hc;
	}
	if( disposition != REG_OPENED_EXISTING_KEY )
	{
		WARN("Created HKEY_DYN_DATA\\%s\\%s\n",KEYNAME_SCSI, KEYNAME_SCSI_CONTROLLERMAP);
	}

	sprintf(cstr, "c%02d", controller);
	if( (error=RegQueryValueExA( hkeyControllerMap, cstr, 0, &type, (LPBYTE)&hc, &cbData)) != ERROR_SUCCESS )
	{
		ERR("Could not open HKEY_DYN_DATA\\%s\\%s\\%s, error=%lx\n", KEYNAME_SCSI, KEYNAME_SCSI_CONTROLLERMAP, cstr, error );
		SetLastError( error );
	}
	RegCloseKey(hkeyControllerMap);
	RegCloseKey(hkeyScsi);
	return hc;

};

int
SCSI_OpenDevice( int h, int c, int t, int d )
{
	char devstr[20];
	DWORD cbData = 20;
	int fd = -1;
	char dainbread_linux_hack = 1;

	if(!SCSI_GetDeviceName( h, c, t, d, devstr, &cbData ))
	{
		WARN("Could not get device name for h%02dc%02dt%02dd%02d\n", h, c, t, d);
		return -1;
	}

linux_hack:
	TRACE("Opening device %s mode O_RDWR\n",devstr);
	fd = open(devstr, O_RDWR);

	if( fd < 0 )
	{
		int len = strlen(devstr);
		set_last_error(); /* SetLastError() to errno */
		TRACE("Open failed (%s)\n", strerror(errno));

		/* in case of "/dev/sgX", convert from sga to sg0
		 * and the other way around.
		 * FIXME: remove it if the distributions
		 * finally manage to agree on something.
		 * The best would probably be a complete
		 * rewrite of the Linux SCSI layer
		 * to use CAM + devfs :-) */
		if ( (dainbread_linux_hack) &&
		     (len >= 3) &&
		     (devstr[len-3] == 's') && (devstr[len-2] == 'g') )
		{
			char *p = &devstr[len-1];
			*p = (*p >= 'a') ? *p - 'a' + '0' : *p - '0' + 'a';
			dainbread_linux_hack = 0;
			TRACE("Retry with \"equivalent\" Linux device '%s'\n", devstr);
			goto linux_hack;
		}
	}
	return fd;
}

#ifdef linux
/* SCSI_Fix_CMD_LEN
 * 	Checks to make sure the CMD_LEN is correct
 */
void
SCSI_Fix_CMD_LEN(int fd, int cmd, int len)
{
	int index=(cmd>>5)&7;

	if (len!=scsi_command_size[index])
	{
		TRACE("CDBLen for command %d claims to be %d, expected %d\n",
				cmd, len, scsi_command_size[index]);
		ioctl(fd,SG_NEXT_CMD_LEN,&len);
	}
}

int
SCSI_LinuxSetTimeout( int fd, int timeout )
{
	int retval;
	TRACE("Setting timeout to %d jiffies\n", timeout);
	retval=ioctl(fd,SG_SET_TIMEOUT,&timeout);
	if(retval)
	{
		WARN("Could not set timeout ! (%s)\n", strerror(errno));
	}
	return retval;

}

/* This function takes care of the write/read to the linux sg device.
 * It returns TRUE or FALSE and uses set_last_error() to convert
 * UNIX errno to Windows GetLastError().  The reason for that is that
 * several programs will check that error and we might as well set
 * it here.  We also return the value of the read call in
 * lpcbBytesReturned.
 */
BOOL /* NOTE: This function SHOULD BLOCK */
SCSI_LinuxDeviceIo( int fd,
		struct sg_header * lpInBuffer, DWORD cbInBuffer,
		struct sg_header * lpOutBuffer, DWORD cbOutBuffer,
		LPDWORD lpcbBytesReturned )
{
	DWORD dwBytes;
	DWORD save_error;

	TRACE("Writing to Linux sg device\n");
	dwBytes = write( fd, lpInBuffer, cbInBuffer );
	if( dwBytes != cbInBuffer )
	{
		set_last_error();
		save_error = GetLastError();
		WARN("Not enough bytes written to scsi device. bytes=%ld .. %ld\n", cbInBuffer, dwBytes );
                /* FIXME: set_last_error() never sets error to ERROR_NOT_ENOUGH_MEMORY... */
		if( save_error == ERROR_NOT_ENOUGH_MEMORY )
			MESSAGE("Your Linux kernel was not able to handle the amount of data sent to the scsi device. Try recompiling with a larger SG_BIG_BUFF value (kernel 2.0.x sg.h)");
		WARN("error= %ld\n", save_error );
		*lpcbBytesReturned = 0;
		return FALSE;
	}

	TRACE("Reading reply from Linux sg device\n");
	*lpcbBytesReturned = read( fd, lpOutBuffer, cbOutBuffer );
	if( *lpcbBytesReturned != cbOutBuffer )
	{
		set_last_error();
		save_error = GetLastError();
		WARN("Not enough bytes read from scsi device. bytes=%ld .. %ld\n", cbOutBuffer, *lpcbBytesReturned);
		WARN("error= %ld\n", save_error );
		return FALSE;
	}
	return TRUE;
}

/* Internal functions */
struct LinuxProcScsiDevice
{
	int host;
	int channel;
	int target;
	int lun;
	char vendor[9];
	char model[17];
	char rev[5];
	char type[33];
	int ansirev;
};

/*
 * we need to declare white spaces explicitly via %*1[ ],
 * as there are very dainbread CD-ROM devices out there
 * which have their manufacturer name blanked out via spaces,
 * which confuses fscanf's parsing (skips all blank spaces)
 */
static int
SCSI_getprocentry( FILE * procfile, struct LinuxProcScsiDevice * dev )
{
	int result;
	result = fscanf( procfile,
		"Host:%*1[ ]scsi%d%*1[ ]Channel:%*1[ ]%d%*1[ ]Id:%*1[ ]%d%*1[ ]Lun:%*1[ ]%d\n",
		&dev->host,
		&dev->channel,
		&dev->target,
		&dev->lun );
	if( result == EOF )
	{
		/* "end of entries" return, so no TRACE() here */
		return EOF;
	}
	if( result != 4 )
	{
		ERR("bus id line scan count error\n");
		return 0;
	}
	result = fscanf( procfile,
		"  Vendor:%*1[ ]%8c%*1[ ]Model:%*1[ ]%16c%*1[ ]Rev:%*1[ ]%4c\n",
		dev->vendor,
		dev->model,
		dev->rev );
	if( result != 3 )
	{
		ERR("model line scan count error\n");
		return 0;
	}

	result = fscanf( procfile,
		"  Type:%*3[ ]%32c%*1[ ]ANSI%*1[ ]SCSI%*1[ ]revision:%*1[ ]%d\n",
		dev->type,
		&dev->ansirev );
	if( result != 2 )
	{
		ERR("SCSI type line scan count error\n");
		return 0;
	}
	/* Since we fscanf with %XXc instead of %s.. put a NULL at end */
	dev->vendor[8] = 0;
	dev->model[16] = 0;
	dev->rev[4] = 0;
	dev->type[32] = 0;

	return 1;
}

static void
SCSI_printprocentry( const struct LinuxProcScsiDevice * dev )
{
	TRACE( "Host: scsi%d Channel: %02d Id: %02d Lun: %02d\n",
		dev->host,
		dev->channel,
		dev->target,
		dev->lun );
	TRACE( "  Vendor: %s Model: %s Rev: %s\n",
		dev->vendor,
		dev->model,
		dev->rev );
	TRACE( "  Type:   %s ANSI SCSI revision: %02d\n",
		dev->type,
		dev->ansirev );
}

static BOOL
SCSI_PutRegControllerMap( HKEY hkeyControllerMap, int num_controller, int ha, int chan)
{
	DWORD error;
	char cstr[20];
	DWORD hc;
	hc = (ha << 16) + chan;
	sprintf(cstr, "c%02d", num_controller);
	if( (error=RegSetValueExA( hkeyControllerMap, cstr, 0, REG_DWORD, (LPBYTE)&hc, sizeof(DWORD))) != ERROR_SUCCESS )
	{
		ERR("Could not create HKEY_DYN_DATA\\%s\\%s\\%s\n", KEYNAME_SCSI, KEYNAME_SCSI_CONTROLLERMAP, cstr );
	}
	return error;
}

static void
SCSI_MapHCtoController()
{
	HKEY hkeyScsi;
	HKEY hkeyControllerMap;
	DWORD disposition;

	char idstr[20];
	DWORD cbIdStr;
	int i = 0;
	DWORD type = 0;
	DWORD error;

	DWORD num_controller = 0;
	int last_ha = -1;
	int last_chan = -1;

	int ha = 0;
	int chan = 0;

	if( RegCreateKeyExA(HKEY_DYN_DATA, KEYNAME_SCSI, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyScsi, &disposition ) != ERROR_SUCCESS )
	{
		ERR("Could not open HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
		return;
	}
	if( disposition != REG_OPENED_EXISTING_KEY )
	{
		WARN("Created HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
	}
	if( RegCreateKeyExA(hkeyScsi, KEYNAME_SCSI_CONTROLLERMAP, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyControllerMap, &disposition ) != ERROR_SUCCESS )
	{
		ERR("Could not create HKEY_DYN_DATA\\%s\\%s\n", KEYNAME_SCSI, KEYNAME_SCSI_CONTROLLERMAP);
		RegCloseKey(hkeyScsi);
		return;
	}

	for( i=0; cbIdStr = sizeof(idstr), (error=RegEnumValueA( hkeyScsi, i, idstr, &cbIdStr, NULL, &type, NULL, NULL )) == ERROR_SUCCESS; i++ )
	{
	        if(idstr[0] == '\0') continue; /* skip the default value */

		if(sscanf(idstr, "h%02dc%02dt%*02dd%*02d", &ha, &chan) != 2) {
			ERR("incorrect reg. value %s\n", debugstr_a(idstr));
			continue;
		}

		if( last_ha < ha )
		{	/* Next HA */
			last_ha = ha;
			last_chan = chan;
			SCSI_PutRegControllerMap( hkeyControllerMap, num_controller, ha, chan);
			num_controller++;
		}
		else if( last_ha > ha )
		{
			FIXME("Expected registry to be sorted\n");
		}
		/* last_ha == ha */
		else if( last_chan < chan )
		{
			last_chan = chan;
			SCSI_PutRegControllerMap( hkeyControllerMap, num_controller, ha, chan);
			num_controller++;
		}
		else if( last_chan > chan )
		{
			FIXME("Expected registry to be sorted\n");
		}
		/* else last_ha == ha && last_chan == chan so do nothing */
	}
	/* Set (default) value to number of controllers */
	if( RegSetValueExA(hkeyControllerMap, NULL, 0, REG_DWORD, (LPBYTE)&num_controller, sizeof(DWORD) ) != ERROR_SUCCESS )
	{
		ERR("Could not set value HKEY_DYN_DATA\\%s\\%s\n",KEYNAME_SCSI, KEYNAME_SCSI_CONTROLLERMAP);
	}
	RegCloseKey(hkeyControllerMap);
	RegCloseKey(hkeyScsi);
	return;
}
#endif

int SCSI_Linux_CheckDevices(void)
{
    DIR *devdir;
    struct dirent *dent = NULL;

    devdir = opendir("/dev");
    for (dent=readdir(devdir);dent;dent=readdir(devdir))
    {
        if (!(strncmp(dent->d_name, "sg", 2)))
            break;
    }
    closedir(devdir);

    if (dent == NULL)
    {
	TRACE("WARNING: You don't have any /dev/sgX generic scsi devices ! \"man MAKEDEV\" !\n");
	return 0;
    }
    return 1;
}

static void
SCSI_GetProcinfo()
/* I'll admit, this function is somewhat of a mess... it was originally
 * designed to make some sort of linked list then I realized that
 * HKEY_DYN_DATA would be a lot less messy
 */
{
#ifdef linux
	static const char procname[] = "/proc/scsi/scsi";
	FILE * procfile = NULL;

	char read_line[40], read1[10] = "\0", read2[10] = "\0";
	int result = 0;

	struct LinuxProcScsiDevice dev;

	char idstr[20];
	char devstr[20];

	int devnum=0;
	int num_ha = 0;

	HKEY hkeyScsi;
	DWORD disposition;

	/* Check whether user has generic scsi devices at all */
	if (!(SCSI_Linux_CheckDevices()))
	    return;

	procfile = fopen( procname, "r" );
	if( !procfile )
	{
		ERR("Could not open %s\n", procname);
		return;
	}

	fgets(read_line, 40, procfile);
	sscanf( read_line, "Attached %9s %9s", read1, read2);

	if(strcmp(read1, "devices:"))
	{
		ERR("Incorrect %s format\n", procname);
		return;
	}

	if(!(strcmp(read2, "none")))
	{
		ERR("No devices found in %s. Make sure you loaded your SCSI driver or set up ide-scsi emulation for your IDE device if this app needs it !\n", procname);
		return;
	}

	if( RegCreateKeyExA(HKEY_DYN_DATA, KEYNAME_SCSI, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyScsi, &disposition ) != ERROR_SUCCESS )
	{
		ERR("Could not create HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
		return;
	}

	/* Read info for one device */
	while( (result = SCSI_getprocentry(procfile, &dev)) > 0 )
	{
		/* Add to registry */

		sprintf(idstr, "h%02dc%02dt%02dd%02d", dev.host, dev.channel, dev.target, dev.lun);
		sprintf(devstr, "/dev/sg%c", 'a'+devnum);
		if( RegSetValueExA(hkeyScsi, idstr, 0, REG_SZ, devstr, strlen(devstr)+1 ) != ERROR_SUCCESS )
		{
			ERR("Could not set value HKEY_DYN_DATA\\%s\\%s\n",KEYNAME_SCSI, idstr);
		}

		/* Debug output */
		SCSI_printprocentry( &dev );

		/* FIXME: We *REALLY* need number of controllers.. not ha */
		/* num of hostadapters is highest ha + 1 */
		if( dev.host >= num_ha )
			num_ha = dev.host+1;
		devnum++;
	} /* while(1) */
	if( result != EOF )
	{
		ERR("Sorry, incorrect %s format\n", procname);
	}
	fclose( procfile );
	if( RegSetValueExA(hkeyScsi, NULL, 0, REG_DWORD, (LPBYTE)&num_ha, sizeof(num_ha) ) != ERROR_SUCCESS )
	{
		ERR("Could not set value HKEY_DYN_DATA\\%s\n",KEYNAME_SCSI);
	}
	RegCloseKey(hkeyScsi);
	return;
#endif
}
