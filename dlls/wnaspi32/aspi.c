/**************************************************************************
 * ASPI routines
 * Copyright (C) 2000 David Elliott <dfe@infinite-internet.net>
 * Copyright (C) 2005 Vitaliy Margolen
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

/* These routines are to be called from either WNASPI32 or WINASPI */

/* FIXME:
 * - No way to override automatic /proc detection, maybe provide an
 *   HKEY_LOCAL_MACHINE\Software\Wine\Wine\Scsi regkey
 * - Somewhat debating an #ifdef linux... technically all this code will
 *   run on another UNIX.. it will fail nicely.
 * - Please add support for mapping multiple channels on host adapters to
 *   aspi controllers, e-mail me if you need help.
 */


#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <fcntl.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#endif
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
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(aspi);

#ifdef linux
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
#endif

static const WCHAR wDevicemapScsi[] = {'H','A','R','D','W','A','R','E','\\','D','E','V','I','C','E','M','A','P','\\','S','c','s','i',0};

/* Exported functions */
int ASPI_GetNumControllers(void)
{
    HKEY hkeyScsi, hkeyPort;
    DWORD i = 0, numPorts, num_ha = 0;
    WCHAR wPortName[15];

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wDevicemapScsi, 0,
        KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkeyScsi) != ERROR_SUCCESS )
    {
        ERR("Could not open HKLM\\%s\n", debugstr_w(wDevicemapScsi));
        return 0;
    }
    while (RegEnumKeyW(hkeyScsi, i++, wPortName, ARRAY_SIZE(wPortName)) == ERROR_SUCCESS)
    {
        if (RegOpenKeyExW(hkeyScsi, wPortName, 0, KEY_QUERY_VALUE, &hkeyPort) == ERROR_SUCCESS)
        {
            if (RegQueryInfoKeyW(hkeyPort, NULL, NULL, NULL, &numPorts, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                num_ha += numPorts;
            }
            RegCloseKey(hkeyPort);
        }
    }

    RegCloseKey(hkeyScsi);
    TRACE("Returning %d host adapters\n", num_ha );
    return num_ha;
}

static BOOL SCSI_GetDeviceName( int h, int c, int t, int d, LPSTR devstr, LPDWORD lpcbData )
{
    char buffer[200];
    HKEY hkeyScsi;
    DWORD type;

    snprintf(buffer, sizeof(buffer), KEYNAME_SCSI, h, c, t, d);
    if( RegOpenKeyExA(HKEY_LOCAL_MACHINE, buffer, 0, KEY_ALL_ACCESS, &hkeyScsi ) != ERROR_SUCCESS )
    {
        TRACE("Could not open HKLM\\%s; device does not exist\n", buffer);
        return FALSE;
    }

    if( RegQueryValueExA(hkeyScsi, "UnixDeviceName", NULL, &type, (LPBYTE)devstr, lpcbData) != ERROR_SUCCESS )
    {
        WARN("Could not query value HKLM\\%s\\UnixDeviceName\n", buffer);
        RegCloseKey(hkeyScsi);
        return FALSE;
    }
    RegCloseKey(hkeyScsi);

    TRACE("Device name: %s\n", devstr);
    return TRUE;
}

/* SCSI_GetHCforController
 * RETURNS
 * 	HIWORD: Host Adapter
 * 	LOWORD: Channel
 */
DWORD ASPI_GetHCforController( int controller )
{
    HKEY hkeyScsi, hkeyPort;
    DWORD i = 0, numPorts;
    int num_ha = controller + 1;
    WCHAR wPortName[15];
    WCHAR wBusName[15];

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, wDevicemapScsi, 0,
        KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkeyScsi) != ERROR_SUCCESS )
    {
        ERR("Could not open HKLM\\%s\n", debugstr_w(wDevicemapScsi));
        return 0xFFFFFFFF;
    }
    while (RegEnumKeyW(hkeyScsi, i++, wPortName, ARRAY_SIZE(wPortName)) == ERROR_SUCCESS)
    {
        if (RegOpenKeyExW(hkeyScsi, wPortName, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                          &hkeyPort) == ERROR_SUCCESS)
        {
            if (RegQueryInfoKeyW(hkeyPort, NULL, NULL, NULL, &numPorts, NULL, NULL,
                             NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                num_ha -= numPorts;
                if (num_ha <= 0) break;
            }
            else
            RegCloseKey(hkeyPort);
        }
    }
    RegCloseKey(hkeyScsi);

    if (num_ha > 0)
    {
        ERR("Invalid controller(%d)\n", controller);
        return 0xFFFFFFFF;
    }

    if (RegEnumKeyW(hkeyPort, -num_ha, wBusName, ARRAY_SIZE(wBusName)) != ERROR_SUCCESS)
    {
        ERR("Failed to enumerate keys\n");
        RegCloseKey(hkeyPort);
        return 0xFFFFFFFF;
    }
    RegCloseKey(hkeyPort);

    return (atoiW(&wPortName[9]) << 16) + atoiW(&wBusName[9]);
}

int SCSI_OpenDevice( int h, int c, int t, int d )
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
    if (fd == -1) {
        char *errstring = strerror(errno);
        ERR("Failed to open device %s: %s\n", devstr, errstring);
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
	/* This is what the linux kernel thinks.... */
	static const unsigned char scsi_command_size[8] =
	{
        	6, 10, 10, 12,
        	12, 12, 10, 10
	};

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
		WARN("Not enough bytes written to scsi device. bytes=%d .. %d\n", cbInBuffer, dwBytes );
                /* FIXME: set_last_error() never sets error to ERROR_NOT_ENOUGH_MEMORY... */
		if( save_error == ERROR_NOT_ENOUGH_MEMORY )
			MESSAGE("Your Linux kernel was not able to handle the amount of data sent to the scsi device. Try recompiling with a larger SG_BIG_BUFF value (kernel 2.0.x sg.h)\n");
		WARN("error= %d\n", save_error );
		*lpcbBytesReturned = 0;
		return FALSE;
	}

	TRACE("Reading reply from Linux sg device\n");
	*lpcbBytesReturned = read( fd, lpOutBuffer, cbOutBuffer );
	if( *lpcbBytesReturned != cbOutBuffer )
	{
		set_last_error();
		save_error = GetLastError();
		WARN("Not enough bytes read from scsi device. bytes=%d .. %d\n", cbOutBuffer, *lpcbBytesReturned);
		WARN("error= %d\n", save_error );
		return FALSE;
	}
	return TRUE;
}

static void SCSI_Linux_CheckDevices(void)
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
	return;
    }
}
#endif

void SCSI_Init(void)
{
#ifdef linux
    SCSI_Linux_CheckDevices();
#endif
}
