/**************************************************************************
ASPI routines
(C) 2000 David Elliott <dfe@netnitco.net>
Licensed under the WINE (X11) license
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
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "debugtools.h"
#include "winreg.h"
#include "winerror.h"
#include "winescsi.h"
#include "file.h"

DEFAULT_DEBUG_CHANNEL(aspi);

/* Internal function prototypes */
static void
SCSI_GetProcinfo();

/* Exported functions */
void
SCSI_Init()
{
	/* For now we just call SCSI_GetProcinfo */
	SCSI_GetProcinfo();
}

int
ASPI_GetNumControllers()
{
	HKEY hkeyScsi;
	DWORD type = REG_DWORD;
	DWORD num_ha = 0;
	DWORD cbData = sizeof(num_ha);

	if( RegOpenKeyExA(HKEY_DYN_DATA, KEYNAME_SCSI, 0, KEY_ALL_ACCESS, &hkeyScsi ) != ERROR_SUCCESS )
	{
		ERR("Could not open HEKY_DYN_DATA\\%s\n",KEYNAME_SCSI);
		return 0;
	}

	if( RegQueryValueExA(hkeyScsi, NULL, NULL, &type, (LPBYTE)&num_ha, &cbData ) != ERROR_SUCCESS )
	{
		ERR("Could not query value HEKY_DYN_DATA\\%s\n",KEYNAME_SCSI);
		num_ha=0;
	}
	RegCloseKey(hkeyScsi);
	FIXME("Please fix to return number of controllers\n");
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
		ERR("Could not open HEKY_DYN_DATA\\%s\n",KEYNAME_SCSI);
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
	DWORD retval;
	FIXME("Please fix to map each channel of each host adapter to the proper ASPI controller number!\n");
	retval = (controller << 16);
	return retval;
};

int
SCSI_OpenDevice( int h, int c, int t, int d )
{
	char devstr[20];
	DWORD cbData = 20;
	int fd = -1;

	if(!SCSI_GetDeviceName( h, c, t, d, devstr, &cbData ))
	{
		WARN("Could not get device name for h%02dc%02dt%02dd%02d\n", h, c, t, d);
		return -1;
	}

	TRACE("Opening device %s mode O_RDWR\n",devstr);
	fd = open(devstr, O_RDWR);

	if( fd < 0 )
	{
		TRACE("open failed\n");
		FILE_SetDosError(); /* SetLastError() to errno */
		TRACE("GetLastError: %ld\n", GetLastError());
	}
	return fd;
}

#ifdef linux
int
SCSI_LinuxSetTimeout( int fd, int timeout )
{
	int retval;
	TRACE("Setting timeout to %d jiffies\n", timeout);
	retval=ioctl(fd,SG_SET_TIMEOUT,&timeout);
	if(retval)
	{
		WARN("Could not set timeout errno=%d!\n",errno);
	}
	return retval;
	
}

/* This function takes care of the write/read to the linux sg device.
 * It returns TRUE or FALSE and uses FILE_SetDosError() to convert
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

	TRACE("Writing to Liunx sg device\n");
	dwBytes = write( fd, lpInBuffer, cbInBuffer );
	if( dwBytes != cbInBuffer )
	{
		FILE_SetDosError();
		save_error = GetLastError();
		WARN("Not enough bytes written to scsi device. bytes=%ld .. %ld\n", cbInBuffer, dwBytes );
		if( save_error == ERROR_NOT_ENOUGH_MEMORY )
			MESSAGE("Your Linux kernel was not able to handle the amount of data sent to the scsi device.  Try recompiling with a larger SG_BIG_BUFF value (kernel 2.0.x sg.h");
		WARN("error= %ld\n", save_error );
		*lpcbBytesReturned = 0;
		return FALSE;
	}
	
	TRACE("Reading reply from Linux sg device\n");
	*lpcbBytesReturned = read( fd, lpOutBuffer, cbOutBuffer );
	if( *lpcbBytesReturned != cbOutBuffer )
	{
		FILE_SetDosError();
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

static int
SCSI_getprocentry( FILE * procfile, struct LinuxProcScsiDevice * dev )
{
	int result;
	result = fscanf( procfile,
		"Host: scsi%d Channel: %d Id: %d Lun: %d\n",
		&dev->host,
		&dev->channel,
		&dev->target,
		&dev->lun );
	if( result == EOF )
		return EOF;
	if( result != 4 )
		return 0;
	result = fscanf( procfile,
		"  Vendor: %8c Model: %16c Rev: %4c\n",
		dev->vendor,
		dev->model,
		dev->rev );
	if( result != 3 )
		return 0;

	result = fscanf( procfile,
		"  Type:   %32c ANSI SCSI revision: %d\n",
		dev->type,
		&dev->ansirev );
	if( result != 2 )
		return 0;
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

static void
SCSI_GetProcinfo()
/* I'll admit, this function is somewhat of a mess... it was originally
 * designed to make some sort of linked list then I realized that
 * HKEY_DYN_DATA would be a lot less messy
 */
{
	FILE * procfile = NULL;

	int result = 0;

	struct LinuxProcScsiDevice dev;

	char idstr[20];
	char devstr[20];

	int devnum=0;
	int num_ha = 0;

	HKEY hkeyScsi;
	DWORD disposition;

	procfile = fopen( "/proc/scsi/scsi", "r" );
	if( !procfile )
	{
		ERR("Could not open /proc/scsi/scsi\n");
		return;
	}

	result = fscanf( procfile, "Attached devices: \n");
	if( result != 0 )
	{
		ERR("Incorrect /proc/scsi/scsi format");
		return;
	}

	if( RegCreateKeyExA(HKEY_DYN_DATA, KEYNAME_SCSI, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyScsi, &disposition ) != ERROR_SUCCESS )
	{
		ERR("Could not create HEKY_DYN_DATA\\%s\n",KEYNAME_SCSI);
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
			ERR("Could not set value HEKY_DYN_DATA\\%s\\%s\n",KEYNAME_SCSI, idstr);
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
		ERR("Incorrect /proc/scsi/scsi format");
	}
	fclose( procfile );
	if( RegSetValueExA(hkeyScsi, NULL, 0, REG_DWORD, (LPBYTE)&num_ha, sizeof(num_ha) ) != ERROR_SUCCESS )
	{
		ERR("Could not set value HEKY_DYN_DATA\\%s\n",KEYNAME_SCSI);
	}
	RegCloseKey(hkeyScsi);
	return;
}
#endif
