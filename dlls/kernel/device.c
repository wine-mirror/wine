/*
 * Win32 device functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Ulrich Weigand
 * Copyright 1998 Patrik Stridvall
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

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winnls.h"
#include "file.h"
#include "kernel_private.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);


/*
 * VxD names are taken from the Win95 DDK
 */

struct VxDInfo
{
    LPCSTR  name;
    WORD    id;
    HMODULE module;
    BOOL  (*deviceio)(DWORD, LPVOID, DWORD,
                        LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
};

static struct VxDInfo VxDList[] =
{
    /* Standard VxD IDs */
    { "VMM",      0x0001, NULL, NULL },
    { "DEBUG",    0x0002, NULL, NULL },
    { "VPICD",    0x0003, NULL, NULL },
    { "VDMAD",    0x0004, NULL, NULL },
    { "VTD",      0x0005, NULL, NULL },
    { "V86MMGR",  0x0006, NULL, NULL },
    { "PAGESWAP", 0x0007, NULL, NULL },
    { "PARITY",   0x0008, NULL, NULL },
    { "REBOOT",   0x0009, NULL, NULL },
    { "VDD",      0x000A, NULL, NULL },
    { "VSD",      0x000B, NULL, NULL },
    { "VMD",      0x000C, NULL, NULL },
    { "VKD",      0x000D, NULL, NULL },
    { "VCD",      0x000E, NULL, NULL },
    { "VPD",      0x000F, NULL, NULL },
    { "BLOCKDEV", 0x0010, NULL, NULL },
    { "VMCPD",    0x0011, NULL, NULL },
    { "EBIOS",    0x0012, NULL, NULL },
    { "BIOSXLAT", 0x0013, NULL, NULL },
    { "VNETBIOS", 0x0014, NULL, NULL },
    { "DOSMGR",   0x0015, NULL, NULL },
    { "WINLOAD",  0x0016, NULL, NULL },
    { "SHELL",    0x0017, NULL, NULL },
    { "VMPOLL",   0x0018, NULL, NULL },
    { "VPROD",    0x0019, NULL, NULL },
    { "DOSNET",   0x001A, NULL, NULL },
    { "VFD",      0x001B, NULL, NULL },
    { "VDD2",     0x001C, NULL, NULL },
    { "WINDEBUG", 0x001D, NULL, NULL },
    { "TSRLOAD",  0x001E, NULL, NULL },
    { "BIOSHOOK", 0x001F, NULL, NULL },
    { "INT13",    0x0020, NULL, NULL },
    { "PAGEFILE", 0x0021, NULL, NULL },
    { "SCSI",     0x0022, NULL, NULL },
    { "MCA_POS",  0x0023, NULL, NULL },
    { "SCSIFD",   0x0024, NULL, NULL },
    { "VPEND",    0x0025, NULL, NULL },
    { "VPOWERD",  0x0026, NULL, NULL },
    { "VXDLDR",   0x0027, NULL, NULL },
    { "NDIS",     0x0028, NULL, NULL },
    { "BIOS_EXT", 0x0029, NULL, NULL },
    { "VWIN32",   0x002A, NULL, NULL },
    { "VCOMM",    0x002B, NULL, NULL },
    { "SPOOLER",  0x002C, NULL, NULL },
    { "WIN32S",   0x002D, NULL, NULL },
    { "DEBUGCMD", 0x002E, NULL, NULL },

    { "VNB",      0x0031, NULL, NULL },
    { "SERVER",   0x0032, NULL, NULL },
    { "CONFIGMG", 0x0033, NULL, NULL },
    { "DWCFGMG",  0x0034, NULL, NULL },
    { "SCSIPORT", 0x0035, NULL, NULL },
    { "VFBACKUP", 0x0036, NULL, NULL },
    { "ENABLE",   0x0037, NULL, NULL },
    { "VCOND",    0x0038, NULL, NULL },

    { "EFAX",     0x003A, NULL, NULL },
    { "DSVXD",    0x003B, NULL, NULL },
    { "ISAPNP",   0x003C, NULL, NULL },
    { "BIOS",     0x003D, NULL, NULL },
    { "WINSOCK",  0x003E, NULL, NULL },
    { "WSOCK",    0x003E, NULL, NULL },
    { "WSIPX",    0x003F, NULL, NULL },
    { "IFSMgr",   0x0040, NULL, NULL },
    { "VCDFSD",   0x0041, NULL, NULL },
    { "MRCI2",    0x0042, NULL, NULL },
    { "PCI",      0x0043, NULL, NULL },
    { "PELOADER", 0x0044, NULL, NULL },
    { "EISA",     0x0045, NULL, NULL },
    { "DRAGCLI",  0x0046, NULL, NULL },
    { "DRAGSRV",  0x0047, NULL, NULL },
    { "PERF",     0x0048, NULL, NULL },
    { "AWREDIR",  0x0049, NULL, NULL },

    /* Far East support */
    { "ETEN",     0x0060, NULL, NULL },
    { "CHBIOS",   0x0061, NULL, NULL },
    { "VMSGD",    0x0062, NULL, NULL },
    { "VPPID",    0x0063, NULL, NULL },
    { "VIME",     0x0064, NULL, NULL },
    { "VHBIOSD",  0x0065, NULL, NULL },

    /* Multimedia OEM IDs */
    { "VTDAPI",   0x0442, NULL, NULL },
    { "MMDEVLDR", 0x044A, NULL, NULL },

    /* Network Device IDs */
    { "VNetSup",  0x0480, NULL, NULL },
    { "VRedir",   0x0481, NULL, NULL },
    { "VBrowse",  0x0482, NULL, NULL },
    { "VSHARE",   0x0483, NULL, NULL },
    { "IFSMgr",   0x0484, NULL, NULL },
    { "MEMPROBE", 0x0485, NULL, NULL },
    { "VFAT",     0x0486, NULL, NULL },
    { "NWLINK",   0x0487, NULL, NULL },
    { "VNWLINK",  0x0487, NULL, NULL },
    { "NWSUP",    0x0487, NULL, NULL },
    { "VTDI",     0x0488, NULL, NULL },
    { "VIP",      0x0489, NULL, NULL },
    { "VTCP",     0x048A, NULL, NULL },
    { "VCache",   0x048B, NULL, NULL },
    { "VUDP",     0x048C, NULL, NULL },
    { "VAsync",   0x048D, NULL, NULL },
    { "NWREDIR",  0x048E, NULL, NULL },
    { "STAT80",   0x048F, NULL, NULL },
    { "SCSIPORT", 0x0490, NULL, NULL },
    { "FILESEC",  0x0491, NULL, NULL },
    { "NWSERVER", 0x0492, NULL, NULL },
    { "SECPROV",  0x0493, NULL, NULL },
    { "NSCL",     0x0494, NULL, NULL },
    { "WSTCP",    0x0495, NULL, NULL },
    { "NDIS2SUP", 0x0496, NULL, NULL },
    { "MSODISUP", 0x0497, NULL, NULL },
    { "Splitter", 0x0498, NULL, NULL },
    { "PPP",      0x0499, NULL, NULL },
    { "VDHCP",    0x049A, NULL, NULL },
    { "VNBT",     0x049B, NULL, NULL },
    { "LOGGER",   0x049D, NULL, NULL },
    { "EFILTER",  0x049E, NULL, NULL },
    { "FFILTER",  0x049F, NULL, NULL },
    { "TFILTER",  0x04A0, NULL, NULL },
    { "AFILTER",  0x04A1, NULL, NULL },
    { "IRLAMP",   0x04A2, NULL, NULL },

    { "PCCARD",   0x097C, NULL, NULL },
    { "HASP95",   0x3721, NULL, NULL },

    /* WINE additions, ids unknown */
    { "MONODEBG", 0x4242, NULL, NULL },

    { NULL,       0,      NULL, NULL }
};


HANDLE DEVICE_Open( LPCWSTR filenameW, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    static const WCHAR dotVxDW[] = {'.','v','x','d',0};
    struct VxDInfo *info;
    WCHAR *p, buffer[16];
    char filename[32];

    if (strlenW( filenameW ) >= sizeof(buffer)/sizeof(WCHAR) - 4 ||
        strchrW( filenameW, '/' ) || strchrW( filenameW, '\\' ))
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }
    strcpyW( buffer, filenameW );
    p = strchrW( buffer, '.' );
    if (!p) strcatW( buffer, dotVxDW );
    else if (strcmpiW( p, dotVxDW ))  /* existing extension has to be .vxd */
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, buffer, -1, filename, sizeof(filename), NULL, NULL))
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }

    for (info = VxDList; info->name; info++)
        if (!strncasecmp( info->name, filename, strlen(info->name) ))
        {
            if (!info->module)
            {
                info->module = LoadLibraryW( buffer );
                if (info->module)
                    info->deviceio = (void *)GetProcAddress( info->module, "DeviceIoControl" );
            }
            return FILE_CreateDevice( info->id | 0x10000, access, sa );
        }

    FIXME( "Unknown/unsupported VxD %s. Try setting Windows version to 'nt40' or 'win31'.\n",
           filename);

    SetLastError( ERROR_FILE_NOT_FOUND );
    return 0;
}

static DWORD DEVICE_GetClientID( HANDLE handle )
{
    DWORD       ret = 0;
    SERVER_START_REQ( get_device_id )
    {
        req->handle = handle;
        if (!wine_server_call( req )) ret = reply->id;
    }
    SERVER_END_REQ;
    return ret;
}

static const struct VxDInfo *DEVICE_GetInfo( DWORD clientID )
{
    const struct VxDInfo *info = NULL;

    if (clientID & 0x10000)
    {
        for (info = VxDList; info->name; info++)
            if (info->id == LOWORD(clientID)) break;
    }
    return info;
}

/****************************************************************************
 *		DeviceIoControl (KERNEL32.@)
 * This is one of those big ugly nasty procedure which can do
 * a million and one things when it comes to devices. It can also be
 * used for VxD communication.
 *
 * A return value of FALSE indicates that something has gone wrong which
 * GetLastError can decipher.
 */
BOOL WINAPI DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
        DWORD clientID;

        TRACE( "(%p,%ld,%p,%ld,%p,%ld,%p,%p)\n",
               hDevice,dwIoControlCode,lpvInBuffer,cbInBuffer,
               lpvOutBuffer,cbOutBuffer,lpcbBytesReturned,lpOverlapped	);

	if (!(clientID = DEVICE_GetClientID( hDevice )))
	{
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	/* Check if this is a user defined control code for a VxD */
	if( HIWORD( dwIoControlCode ) == 0 )
	{
                const struct VxDInfo *info;
                if (!(info = DEVICE_GetInfo( clientID )))
                {
                        FIXME( "No device found for id %lx\n", clientID);
                }
                else if ( info->deviceio )
		{
			return info->deviceio( dwIoControlCode,
                                        lpvInBuffer, cbInBuffer,
                                        lpvOutBuffer, cbOutBuffer,
                                        lpcbBytesReturned, lpOverlapped );
		}
		else
		{
			FIXME( "Unimplemented control %ld for VxD device %s\n",
                               dwIoControlCode, info->name ? info->name : "???" );
			/* FIXME: this is for invalid calls on W98SE,
			 * but maybe we should use ERROR_CALL_NOT_IMPLEMENTED
			 * instead ? */
			SetLastError( ERROR_INVALID_FUNCTION );
		}
	}
	else
	{       
            NTSTATUS            status;

            if (lpOverlapped)
            {
                status = NtDeviceIoControlFile(hDevice, lpOverlapped->hEvent, 
                                               NULL, NULL, 
                                               (PIO_STATUS_BLOCK)lpOverlapped,
                                               dwIoControlCode, 
                                               lpvInBuffer, cbInBuffer,
                                               lpvOutBuffer, cbOutBuffer);
                if (status) SetLastError(RtlNtStatusToDosError(status));
                if (lpcbBytesReturned) *lpcbBytesReturned = lpOverlapped->InternalHigh;
                return !status;
            }
            else
            {
                IO_STATUS_BLOCK     iosb;

                status = NtDeviceIoControlFile(hDevice, NULL, NULL, NULL, &iosb,
                                               dwIoControlCode, 
                                               lpvInBuffer, cbInBuffer,
                                               lpvOutBuffer, cbOutBuffer);
                if (status) SetLastError(RtlNtStatusToDosError(status));
                if (lpcbBytesReturned) *lpcbBytesReturned = iosb.Information;
                return !status;
            }
	}
   	return FALSE;
}
