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
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "file.h"
#include "winioctl.h"
#include "winnt.h"
#include "msdos.h"
#include "miscemu.h"
#include "stackframe.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "callback.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);


static BOOL DeviceIo_VTDAPI(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped);
static BOOL DeviceIo_MONODEBG(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped);
static BOOL DeviceIo_MMDEVLDR(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped);

static DWORD VxDCall_VMM( DWORD service, CONTEXT86 *context );

static BOOL DeviceIo_IFSMgr(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped);

static BOOL DeviceIo_VCD(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped);

static DWORD VxDCall_VWin32( DWORD service, CONTEXT86 *context );

static BOOL DeviceIo_VWin32(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped);

static BOOL DeviceIo_PCCARD (DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped);

static BOOL DeviceIo_HASP (DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped);
/*
 * VxD names are taken from the Win95 DDK
 */

struct VxDInfo
{
    LPCSTR  name;
    WORD    id;
    DWORD (*vxdcall)(DWORD, CONTEXT86 *);
    BOOL  (*deviceio)(DWORD, LPVOID, DWORD,
                        LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
};

static const struct VxDInfo VxDList[] =
{
    /* Standard VxD IDs */
    { "VMM",      0x0001, VxDCall_VMM, NULL },
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
    { "VCD",      0x000E, NULL, DeviceIo_VCD },
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
    { "VWIN32",   0x002A, VxDCall_VWin32, DeviceIo_VWin32 },
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
    { "IFSMgr",   0x0040, NULL, DeviceIo_IFSMgr },
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
    { "VTDAPI",   0x0442, NULL, DeviceIo_VTDAPI },
    { "MMDEVLDR", 0x044A, NULL, DeviceIo_MMDEVLDR },

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

    { "PCCARD",   0x097C, NULL, DeviceIo_PCCARD },
    { "HASP95",   0x3721, NULL, DeviceIo_HASP },

    /* WINE additions, ids unknown */
    { "MONODEBG.VXD", 0x4242, NULL, DeviceIo_MONODEBG },

    { NULL,       0,      NULL, NULL }
};

/*
 * VMM VxDCall service names are (mostly) taken from Stan Mitchell's
 * "Inside the Windows 95 File System"
 */

#define N_VMM_SERVICE 41

LPCSTR VMM_Service_Name[N_VMM_SERVICE] =
{
    "PageReserve",            /* 0x0000 */
    "PageCommit",             /* 0x0001 */
    "PageDecommit",           /* 0x0002 */
    "PagerRegister",          /* 0x0003 */
    "PagerQuery",             /* 0x0004 */
    "HeapAllocate",           /* 0x0005 */
    "ContextCreate",          /* 0x0006 */
    "ContextDestroy",         /* 0x0007 */
    "PageAttach",             /* 0x0008 */
    "PageFlush",              /* 0x0009 */
    "PageFree",               /* 0x000A */
    "ContextSwitch",          /* 0x000B */
    "HeapReAllocate",         /* 0x000C */
    "PageModifyPermissions",  /* 0x000D */
    "PageQuery",              /* 0x000E */
    "GetCurrentContext",      /* 0x000F */
    "HeapFree",               /* 0x0010 */
    "RegOpenKey",             /* 0x0011 */
    "RegCreateKey",           /* 0x0012 */
    "RegCloseKey",            /* 0x0013 */
    "RegDeleteKey",           /* 0x0014 */
    "RegSetValue",            /* 0x0015 */
    "RegDeleteValue",         /* 0x0016 */
    "RegQueryValue",          /* 0x0017 */
    "RegEnumKey",             /* 0x0018 */
    "RegEnumValue",           /* 0x0019 */
    "RegQueryValueEx",        /* 0x001A */
    "RegSetValueEx",          /* 0x001B */
    "RegFlushKey",            /* 0x001C */
    "RegQueryInfoKey",        /* 0x001D */
    "GetDemandPageInfo",      /* 0x001E */
    "BlockOnID",              /* 0x001F */
    "SignalID",               /* 0x0020 */
    "RegLoadKey",             /* 0x0021 */
    "RegUnLoadKey",           /* 0x0022 */
    "RegSaveKey",             /* 0x0023 */
    "RegRemapPreDefKey",      /* 0x0024 */
    "PageChangePager",        /* 0x0025 */
    "RegQueryMultipleValues", /* 0x0026 */
    "RegReplaceKey",          /* 0x0027 */
    "<KERNEL32.101>"          /* 0x0028 -- What does this do??? */
};

/* PageReserve arena values */
#define PR_PRIVATE  0x80000400	/* anywhere in private arena */
#define PR_SHARED   0x80060000	/* anywhere in shared arena */
#define PR_SYSTEM   0x80080000	/* anywhere in system arena */

/* PageReserve flags */
#define PR_FIXED    0x00000008	/* don't move during PageReAllocate */
#define PR_4MEG     0x00000001	/* allocate on 4mb boundary */
#define PR_STATIC   0x00000010	/* see PageReserve documentation */

/* PageCommit default pager handle values */
#define PD_ZEROINIT 0x00000001	/* swappable zero-initialized pages */
#define PD_NOINIT   0x00000002	/* swappable uninitialized pages */
#define PD_FIXEDZERO	0x00000003  /* fixed zero-initialized pages */
#define PD_FIXED    0x00000004	/* fixed uninitialized pages */

/* PageCommit flags */
#define PC_FIXED    0x00000008	/* pages are permanently locked */
#define PC_LOCKED   0x00000080	/* pages are made present and locked */
#define PC_LOCKEDIFDP	0x00000100  /* pages are locked if swap via DOS */
#define PC_WRITEABLE	0x00020000  /* make the pages writeable */
#define PC_USER     0x00040000	/* make the pages ring 3 accessible */
#define PC_INCR     0x40000000	/* increment "pagerdata" each page */
#define PC_PRESENT  0x80000000	/* make pages initially present */
#define PC_STATIC   0x20000000	/* allow commit in PR_STATIC object */
#define PC_DIRTY    0x08000000	/* make pages initially dirty */
#define PC_CACHEDIS 0x00100000  /* Allocate uncached pages - new for WDM */
#define PC_CACHEWT  0x00080000  /* Allocate write through cache pages - new for WDM */
#define PC_PAGEFLUSH 0x00008000 /* Touch device mapped pages on alloc - new for WDM */

/* PageCommitContig additional flags */
#define PCC_ZEROINIT	0x00000001  /* zero-initialize new pages */
#define PCC_NOLIN   0x10000000	/* don't map to any linear address */



HANDLE DEVICE_Open( LPCWSTR filenameW, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    const struct VxDInfo *info;
    char filename[MAX_PATH];

    if (!WideCharToMultiByte(CP_ACP, 0, filenameW, -1, filename, MAX_PATH, NULL, NULL))
    {
        SetLastError( ERROR_FILE_NOT_FOUND );
        return 0;
    }

    for (info = VxDList; info->name; info++)
        if (!strncasecmp( info->name, filename, strlen(info->name) ))
            return FILE_CreateDevice( info->id | 0x10000, access, sa );

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
                char str[3];

                strcpy(str,  "A:");
                str[0] += LOBYTE(clientID);
                if (GetDriveTypeA(str) == DRIVE_CDROM)
                {
                    NTSTATUS status;
                    status = CDROM_DeviceIoControl(clientID, hDevice, dwIoControlCode, lpvInBuffer, cbInBuffer,
                                                   lpvOutBuffer, cbOutBuffer, lpcbBytesReturned,
                                                   lpOverlapped);
                    if (status) SetLastError(RtlNtStatusToDosError(status));
                    return !status;
                }
                else switch( dwIoControlCode )
		{
		case FSCTL_DELETE_REPARSE_POINT:
		case FSCTL_DISMOUNT_VOLUME:
		case FSCTL_GET_COMPRESSION:
		case FSCTL_GET_REPARSE_POINT:
		case FSCTL_LOCK_VOLUME:
		case FSCTL_QUERY_ALLOCATED_RANGES:
		case FSCTL_SET_COMPRESSION:
		case FSCTL_SET_REPARSE_POINT:
		case FSCTL_SET_SPARSE:
		case FSCTL_SET_ZERO_DATA:
		case FSCTL_UNLOCK_VOLUME:
		case IOCTL_DISK_CHECK_VERIFY:
		case IOCTL_DISK_EJECT_MEDIA:
		case IOCTL_DISK_FORMAT_TRACKS:
		case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		case IOCTL_DISK_GET_DRIVE_LAYOUT:
		case IOCTL_DISK_GET_MEDIA_TYPES:
		case IOCTL_DISK_GET_PARTITION_INFO:
		case IOCTL_DISK_LOAD_MEDIA:
		case IOCTL_DISK_MEDIA_REMOVAL:
		case IOCTL_DISK_PERFORMANCE:
		case IOCTL_DISK_REASSIGN_BLOCKS:
		case IOCTL_DISK_SET_DRIVE_LAYOUT:
		case IOCTL_DISK_SET_PARTITION_INFO:
		case IOCTL_DISK_VERIFY:
		case IOCTL_SERIAL_LSRMST_INSERT:
		case IOCTL_STORAGE_CHECK_VERIFY:
		case IOCTL_STORAGE_EJECT_MEDIA:
		case IOCTL_STORAGE_GET_MEDIA_TYPES:
		case IOCTL_STORAGE_LOAD_MEDIA:
		case IOCTL_STORAGE_MEDIA_REMOVAL:
    			FIXME( "unimplemented dwIoControlCode=%08lx\n", dwIoControlCode);
    			SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    			return FALSE;
    			break;
		default:
    			FIXME( "ignored dwIoControlCode=%08lx\n",dwIoControlCode);
    			SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    			return FALSE;
    			break;
		}
	}
   	return FALSE;
}

/***********************************************************************
 *           DeviceIo_VTDAPI
 */
static BOOL DeviceIo_VTDAPI(DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
    BOOL retv = TRUE;

    switch (dwIoControlCode)
    {
    case 5:
        if (lpvOutBuffer && (cbOutBuffer>=4))
            *(DWORD*)lpvOutBuffer = GetTickCount();

        if (lpcbBytesReturned)
            *lpcbBytesReturned = 4;

        break;

    default:
        FIXME( "Control %ld not implemented\n", dwIoControlCode);
        retv = FALSE;
        break;
    }

    return retv;
}

/***********************************************************************
 *		VxDCall0 (KERNEL32.1)
 *		VxDCall1 (KERNEL32.2)
 *		VxDCall2 (KERNEL32.3)
 *		VxDCall3 (KERNEL32.4)
 *		VxDCall4 (KERNEL32.5)
 *		VxDCall5 (KERNEL32.6)
 *		VxDCall6 (KERNEL32.7)
 *		VxDCall7 (KERNEL32.8)
 *		VxDCall8 (KERNEL32.9)
 */
void VxDCall( DWORD service, CONTEXT86 *context )
{
    DWORD ret = 0xffffffff; /* FIXME */
    int i;

    TRACE( "(%08lx, ...)\n", service);

    for (i = 0; VxDList[i].name; i++)
        if (VxDList[i].id == HIWORD(service))
            break;

    if (!VxDList[i].name)
        FIXME( "Unknown VxD (%08lx)\n", service);
    else if (!VxDList[i].vxdcall)
        FIXME( "Unimplemented VxD (%08lx)\n", service);
    else
        ret = VxDList[i].vxdcall( service, context );

    context->Eax = ret;
}


/******************************************************************************
 * The following is a massive duplication of the advapi32 code.
 * Unfortunately sharing the code is not possible since the native
 * Win95 advapi32 depends on it. Someday we should probably stop
 * supporting native Win95 advapi32 altogether...
 */


#define HKEY_SPECIAL_ROOT_FIRST   HKEY_CLASSES_ROOT
#define HKEY_SPECIAL_ROOT_LAST    HKEY_DYN_DATA
#define NB_SPECIAL_ROOT_KEYS      ((UINT)HKEY_SPECIAL_ROOT_LAST - (UINT)HKEY_SPECIAL_ROOT_FIRST + 1)

static HKEY special_root_keys[NB_SPECIAL_ROOT_KEYS];

static const WCHAR name_CLASSES_ROOT[] =
    {'M','a','c','h','i','n','e','\\',
     'S','o','f','t','w','a','r','e','\\',
     'C','l','a','s','s','e','s',0};
static const WCHAR name_LOCAL_MACHINE[] =
    {'M','a','c','h','i','n','e',0};
static const WCHAR name_USERS[] =
    {'U','s','e','r',0};
static const WCHAR name_PERFORMANCE_DATA[] =
    {'P','e','r','f','D','a','t','a',0};
static const WCHAR name_CURRENT_CONFIG[] =
    {'M','a','c','h','i','n','e','\\',
     'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'H','a','r','d','w','a','r','e','P','r','o','f','i','l','e','s','\\',
     'C','u','r','r','e','n','t',0};
static const WCHAR name_DYN_DATA[] =
    {'D','y','n','D','a','t','a',0};

#define DECL_STR(key) { sizeof(name_##key)-sizeof(WCHAR), sizeof(name_##key), (LPWSTR)name_##key }
static UNICODE_STRING root_key_names[NB_SPECIAL_ROOT_KEYS] =
{
    DECL_STR(CLASSES_ROOT),
    { 0, 0, NULL },         /* HKEY_CURRENT_USER is determined dynamically */
    DECL_STR(LOCAL_MACHINE),
    DECL_STR(USERS),
    DECL_STR(PERFORMANCE_DATA),
    DECL_STR(CURRENT_CONFIG),
    DECL_STR(DYN_DATA)
};
#undef DECL_STR


/* check if value type needs string conversion (Ansi<->Unicode) */
inline static int is_string( DWORD type )
{
    return (type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ);
}

/* create one of the HKEY_* special root keys */
static HKEY create_special_root_hkey( HKEY hkey, DWORD access )
{
    HKEY ret = 0;
    int idx = (UINT)hkey - (UINT)HKEY_SPECIAL_ROOT_FIRST;

    if (hkey == HKEY_CURRENT_USER)
    {
        if (RtlOpenCurrentUser( access, &hkey )) return 0;
    }
    else
    {
        OBJECT_ATTRIBUTES attr;

        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.ObjectName = &root_key_names[idx];
        attr.Attributes = 0;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        if (NtCreateKey( &hkey, access, &attr, 0, NULL, 0, NULL )) return 0;
    }

    if (!(ret = InterlockedCompareExchangePointer( (PVOID) &special_root_keys[idx], hkey, 0 )))
        ret = hkey;
    else
        NtClose( hkey );  /* somebody beat us to it */
    return ret;
}

/* map the hkey from special root to normal key if necessary */
inline static HKEY get_special_root_hkey( HKEY hkey )
{
    HKEY ret = hkey;

    if ((hkey >= HKEY_SPECIAL_ROOT_FIRST) && (hkey <= HKEY_SPECIAL_ROOT_LAST))
    {
        if (!(ret = special_root_keys[(UINT)hkey - (UINT)HKEY_SPECIAL_ROOT_FIRST]))
            ret = create_special_root_hkey( hkey, KEY_ALL_ACCESS );
    }
    return ret;
}


/******************************************************************************
 *           VMM_RegCreateKeyA
 */
static DWORD VMM_RegCreateKeyA( HKEY hkey, LPCSTR name, PHKEY retkey )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    ANSI_STRING nameA;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitAnsiString( &nameA, name );

    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtCreateKey( retkey, KEY_ALL_ACCESS, &attr, 0, NULL,
                              REG_OPTION_NON_VOLATILE, NULL );
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegOpenKeyExA
 */
DWORD WINAPI VMM_RegOpenKeyExA(HKEY hkey, LPCSTR name, DWORD reserved, REGSAM access, PHKEY retkey)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    STRING nameA;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtOpenKey( retkey, access, &attr );
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegCloseKey
 */
static DWORD VMM_RegCloseKey( HKEY hkey )
{
    if (!hkey || hkey >= (HKEY)0x80000000) return ERROR_SUCCESS;
    return RtlNtStatusToDosError( NtClose( hkey ) );
}


/******************************************************************************
 *           VMM_RegDeleteKeyA
 */
static DWORD VMM_RegDeleteKeyA( HKEY hkey, LPCSTR name )
{
    DWORD ret;
    HKEY tmp;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    if (!name || !*name) return RtlNtStatusToDosError( NtDeleteKey( hkey ) );
    if (!(ret = VMM_RegOpenKeyExA( hkey, name, 0, 0, &tmp )))
    {
        ret = RtlNtStatusToDosError( NtDeleteKey( tmp ) );
        NtClose( tmp );
    }
    return ret;
}


/******************************************************************************
 *           VMM_RegSetValueExA
 */
static DWORD VMM_RegSetValueExA( HKEY hkey, LPCSTR name, DWORD reserved, DWORD type,
                                 CONST BYTE *data, DWORD count )
{
    UNICODE_STRING nameW;
    ANSI_STRING nameA;
    WCHAR *dataW = NULL;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    if (is_string(type))
    {
        DWORD lenW;

        if (count)
        {
            /* if user forgot to count terminating null, add it (yes NT does this) */
            if (data[count-1] && !data[count]) count++;
        }
        RtlMultiByteToUnicodeSize( &lenW, data, count );
        if (!(dataW = HeapAlloc( GetProcessHeap(), 0, lenW ))) return ERROR_OUTOFMEMORY;
        RtlMultiByteToUnicodeN( dataW, lenW, NULL, data, count );
        count = lenW;
        data = (BYTE *)dataW;
    }

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtSetValueKey( hkey, &nameW, 0, type, data, count );
        RtlFreeUnicodeString( &nameW );
    }
    if (dataW) HeapFree( GetProcessHeap(), 0, dataW );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegSetValueA
 */
static DWORD VMM_RegSetValueA( HKEY hkey, LPCSTR name, DWORD type, LPCSTR data, DWORD count )
{
    HKEY subkey = hkey;
    DWORD ret;

    if (type != REG_SZ) return ERROR_INVALID_PARAMETER;

    if (name && name[0])  /* need to create the subkey */
    {
        if ((ret = VMM_RegCreateKeyA( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }
    ret = VMM_RegSetValueExA( subkey, NULL, 0, REG_SZ, (LPBYTE)data, strlen(data)+1 );
    if (subkey != hkey) NtClose( subkey );
    return ret;
}


/******************************************************************************
 *           VMM_RegDeleteValueA
 */
static DWORD VMM_RegDeleteValueA( HKEY hkey, LPCSTR name )
{
    UNICODE_STRING nameW;
    STRING nameA;
    NTSTATUS status;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    RtlInitAnsiString( &nameA, name );
    if (!(status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
    {
        status = NtDeleteValueKey( hkey, &nameW );
        RtlFreeUnicodeString( &nameW );
    }
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegQueryValueExA
 */
static DWORD VMM_RegQueryValueExA( HKEY hkey, LPCSTR name, LPDWORD reserved, LPDWORD type,
                                   LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    ANSI_STRING nameA;
    UNICODE_STRING nameW;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data );

    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    RtlInitAnsiString( &nameA, name );
    if ((status = RtlAnsiStringToUnicodeString( &nameW, &nameA, TRUE )))
        return RtlNtStatusToDosError(status);

    status = NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                              buffer, sizeof(buffer), &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    /* we need to fetch the contents for a string type even if not requested,
     * because we need to compute the length of the ASCII string. */
    if (data || is_string(info->Type))
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
            if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            info = (KEY_VALUE_PARTIAL_INFORMATION *)buf_ptr;
            status = NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                                      buf_ptr, total_size, &total_size );
        }

        if (!status)
        {
            if (is_string(info->Type))
            {
                DWORD len = WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(buf_ptr + info_size),
                                                 (total_size - info_size) /sizeof(WCHAR),
                                                 NULL, 0, NULL, NULL );
                if (data && len)
                {
                    if (len > *count) status = STATUS_BUFFER_OVERFLOW;
                    else
                    {
                        WideCharToMultiByte( CP_ACP, 0, (WCHAR *)(buf_ptr + info_size),
                                             (total_size - info_size) /sizeof(WCHAR),
                                             data, len, NULL, NULL );
                        /* if the type is REG_SZ and data is not 0-terminated
                         * and there is enough space in the buffer NT appends a \0 */
                        if (len < *count && data[len-1]) data[len] = 0;
                    }
                }
                total_size = len + info_size;
            }
            else if (data)
            {
                if (total_size - info_size > *count) status = STATUS_BUFFER_OVERFLOW;
                else memcpy( data, buf_ptr + info_size, total_size - info_size );
            }
        }
        else if (status != STATUS_BUFFER_OVERFLOW) goto done;
    }

    if (type) *type = info->Type;
    if (count) *count = total_size - info_size;

 done:
    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    RtlFreeUnicodeString( &nameW );
    return RtlNtStatusToDosError(status);
}


/******************************************************************************
 *           VMM_RegQueryValueA
 */
static DWORD VMM_RegQueryValueA( HKEY hkey, LPCSTR name, LPSTR data, LPLONG count )
{
    DWORD ret;
    HKEY subkey = hkey;

    if (name && name[0])
    {
        if ((ret = VMM_RegOpenKeyExA( hkey, name, 0, KEY_ALL_ACCESS, &subkey )) != ERROR_SUCCESS)
            return ret;
    }
    ret = VMM_RegQueryValueExA( subkey, NULL, NULL, NULL, (LPBYTE)data, count );
    if (subkey != hkey) NtClose( subkey );
    if (ret == ERROR_FILE_NOT_FOUND)
    {
        /* return empty string if default value not found */
        if (data) *data = 0;
        if (count) *count = 1;
        ret = ERROR_SUCCESS;
    }
    return ret;
}


/******************************************************************************
 *           VMM_RegEnumValueA
 */
static DWORD VMM_RegEnumValueA( HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
                                LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    NTSTATUS status;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_FULL_INFORMATION, Name );

    TRACE("(%p,%ld,%p,%p,%p,%p,%p,%p)\n",
          hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    total_size = info_size + (MAX_PATH + 1) * sizeof(WCHAR);
    if (data) total_size += *count;
    total_size = min( sizeof(buffer), total_size );

    status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                  buffer, total_size, &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    /* we need to fetch the contents for a string type even if not requested,
     * because we need to compute the length of the ASCII string. */
    if (value || data || is_string(info->Type))
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
            if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_VALUE_FULL_INFORMATION *)buf_ptr;
            status = NtEnumerateValueKey( hkey, index, KeyValueFullInformation,
                                          buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        if (is_string(info->Type))
        {
            DWORD len;
            RtlUnicodeToMultiByteSize( &len, (WCHAR *)(buf_ptr + info->DataOffset),
                                       total_size - info->DataOffset );
            if (data && len)
            {
                if (len > *count) status = STATUS_BUFFER_OVERFLOW;
                else
                {
                    RtlUnicodeToMultiByteN( data, len, NULL, (WCHAR *)(buf_ptr + info->DataOffset),
                                            total_size - info->DataOffset );
                    /* if the type is REG_SZ and data is not 0-terminated
                     * and there is enough space in the buffer NT appends a \0 */
                    if (len < *count && data[len-1]) data[len] = 0;
                }
            }
            info->DataLength = len;
        }
        else if (data)
        {
            if (total_size - info->DataOffset > *count) status = STATUS_BUFFER_OVERFLOW;
            else memcpy( data, buf_ptr + info->DataOffset, total_size - info->DataOffset );
        }

        if (value && !status)
        {
            DWORD len;

            RtlUnicodeToMultiByteSize( &len, info->Name, info->NameLength );
            if (len >= *val_count)
            {
                status = STATUS_BUFFER_OVERFLOW;
                if (*val_count)
                {
                    len = *val_count - 1;
                    RtlUnicodeToMultiByteN( value, len, NULL, info->Name, info->NameLength );
                    value[len] = 0;
                }
            }
            else
            {
                RtlUnicodeToMultiByteN( value, len, NULL, info->Name, info->NameLength );
                value[len] = 0;
                *val_count = len;
            }
        }
    }
    else status = STATUS_SUCCESS;

    if (type) *type = info->Type;
    if (count) *count = info->DataLength;

 done:
    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError(status);
}


/******************************************************************************
 *           VMM_RegEnumKeyA
 */
static DWORD VMM_RegEnumKeyA( HKEY hkey, DWORD index, LPSTR name, DWORD name_len )
{
    NTSTATUS status;
    char buffer[256], *buf_ptr = buffer;
    KEY_NODE_INFORMATION *info = (KEY_NODE_INFORMATION *)buffer;
    DWORD total_size;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    status = NtEnumerateKey( hkey, index, KeyNodeInformation,
                             buffer, sizeof(buffer), &total_size );

    while (status == STATUS_BUFFER_OVERFLOW)
    {
        /* retry with a dynamically allocated buffer */
        if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
        if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
            return ERROR_NOT_ENOUGH_MEMORY;
        info = (KEY_NODE_INFORMATION *)buf_ptr;
        status = NtEnumerateKey( hkey, index, KeyNodeInformation,
                                 buf_ptr, total_size, &total_size );
    }

    if (!status)
    {
        DWORD len;

        RtlUnicodeToMultiByteSize( &len, info->Name, info->NameLength );
        if (len >= name_len) status = STATUS_BUFFER_OVERFLOW;
        else
        {
            RtlUnicodeToMultiByteN( name, len, NULL, info->Name, info->NameLength );
            name[len] = 0;
        }
    }

    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError( status );
}


/******************************************************************************
 *           VMM_RegQueryInfoKeyA
 *
 * NOTE: This VxDCall takes only a subset of the parameters that the
 * corresponding Win32 API call does. The implementation in Win95
 * ADVAPI32 sets all output parameters not mentioned here to zero.
 */
static DWORD VMM_RegQueryInfoKeyA( HKEY hkey, LPDWORD subkeys, LPDWORD max_subkey,
                                   LPDWORD values, LPDWORD max_value, LPDWORD max_data )
{
    NTSTATUS status;
    KEY_FULL_INFORMATION info;
    DWORD total_size;

    if (!(hkey = get_special_root_hkey( hkey ))) return ERROR_INVALID_HANDLE;

    status = NtQueryKey( hkey, KeyFullInformation, &info, sizeof(info), &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) return RtlNtStatusToDosError( status );

    if (subkeys) *subkeys = info.SubKeys;
    if (max_subkey) *max_subkey = info.MaxNameLen;
    if (values) *values = info.Values;
    if (max_value) *max_value = info.MaxValueNameLen;
    if (max_data) *max_data = info.MaxValueDataLen;
    return ERROR_SUCCESS;
}


/***********************************************************************
 *           VxDCall_VMM
 */
static DWORD VxDCall_VMM( DWORD service, CONTEXT86 *context )
{
    switch ( LOWORD(service) )
    {
    case 0x0011:  /* RegOpenKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        PHKEY   retkey     = (PHKEY)stack32_pop( context );
        return VMM_RegOpenKeyExA( hkey, lpszSubKey, 0, KEY_ALL_ACCESS, retkey );
    }

    case 0x0012:  /* RegCreateKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        PHKEY   retkey     = (PHKEY)stack32_pop( context );
        return VMM_RegCreateKeyA( hkey, lpszSubKey, retkey );
    }

    case 0x0013:  /* RegCloseKey */
    {
        HKEY    hkey       = (HKEY)stack32_pop( context );
        return VMM_RegCloseKey( hkey );
    }

    case 0x0014:  /* RegDeleteKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        return VMM_RegDeleteKeyA( hkey, lpszSubKey );
    }

    case 0x0015:  /* RegSetValue */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        DWORD   dwType     = (DWORD) stack32_pop( context );
        LPCSTR  lpszData   = (LPCSTR)stack32_pop( context );
        DWORD   cbData     = (DWORD) stack32_pop( context );
        return VMM_RegSetValueA( hkey, lpszSubKey, dwType, lpszData, cbData );
    }

    case 0x0016:  /* RegDeleteValue */
    {
        HKEY    hkey       = (HKEY) stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR)stack32_pop( context );
        return VMM_RegDeleteValueA( hkey, lpszValue );
    }

    case 0x0017:  /* RegQueryValue */
    {
        HKEY    hkey       = (HKEY)   stack32_pop( context );
        LPSTR   lpszSubKey = (LPSTR)  stack32_pop( context );
        LPSTR   lpszData   = (LPSTR)  stack32_pop( context );
        LPDWORD lpcbData   = (LPDWORD)stack32_pop( context );
        return VMM_RegQueryValueA( hkey, lpszSubKey, lpszData, lpcbData );
    }

    case 0x0018:  /* RegEnumKey */
    {
        HKEY    hkey       = (HKEY) stack32_pop( context );
        DWORD   iSubkey    = (DWORD)stack32_pop( context );
        LPSTR   lpszName   = (LPSTR)stack32_pop( context );
        DWORD   lpcchName  = (DWORD)stack32_pop( context );
        return VMM_RegEnumKeyA( hkey, iSubkey, lpszName, lpcchName );
    }

    case 0x0019:  /* RegEnumValue */
    {
        HKEY    hkey       = (HKEY)   stack32_pop( context );
        DWORD   iValue     = (DWORD)  stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR)  stack32_pop( context );
        LPDWORD lpcchValue = (LPDWORD)stack32_pop( context );
        LPDWORD lpReserved = (LPDWORD)stack32_pop( context );
        LPDWORD lpdwType   = (LPDWORD)stack32_pop( context );
        LPBYTE  lpbData    = (LPBYTE) stack32_pop( context );
        LPDWORD lpcbData   = (LPDWORD)stack32_pop( context );
        return VMM_RegEnumValueA( hkey, iValue, lpszValue, lpcchValue,
                                  lpReserved, lpdwType, lpbData, lpcbData );
    }

    case 0x001A:  /* RegQueryValueEx */
    {
        HKEY    hkey       = (HKEY)   stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR)  stack32_pop( context );
        LPDWORD lpReserved = (LPDWORD)stack32_pop( context );
        LPDWORD lpdwType   = (LPDWORD)stack32_pop( context );
        LPBYTE  lpbData    = (LPBYTE) stack32_pop( context );
        LPDWORD lpcbData   = (LPDWORD)stack32_pop( context );
        return VMM_RegQueryValueExA( hkey, lpszValue, lpReserved,
                                     lpdwType, lpbData, lpcbData );
    }

    case 0x001B:  /* RegSetValueEx */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR) stack32_pop( context );
        DWORD   dwReserved = (DWORD) stack32_pop( context );
        DWORD   dwType     = (DWORD) stack32_pop( context );
        LPBYTE  lpbData    = (LPBYTE)stack32_pop( context );
        DWORD   cbData     = (DWORD) stack32_pop( context );
        return VMM_RegSetValueExA( hkey, lpszValue, dwReserved,
                                   dwType, lpbData, cbData );
    }

    case 0x001C:  /* RegFlushKey */
    {
        HKEY hkey = (HKEY)stack32_pop( context );
        FIXME( "RegFlushKey(%p): stub\n", hkey );
        return ERROR_SUCCESS;
    }

    case 0x001D:  /* RegQueryInfoKey */
    {
        /* NOTE: This VxDCall takes only a subset of the parameters that the
                 corresponding Win32 API call does. The implementation in Win95
                 ADVAPI32 sets all output parameters not mentioned here to zero. */

        HKEY    hkey              = (HKEY)   stack32_pop( context );
        LPDWORD lpcSubKeys        = (LPDWORD)stack32_pop( context );
        LPDWORD lpcchMaxSubKey    = (LPDWORD)stack32_pop( context );
        LPDWORD lpcValues         = (LPDWORD)stack32_pop( context );
        LPDWORD lpcchMaxValueName = (LPDWORD)stack32_pop( context );
        LPDWORD lpcchMaxValueData = (LPDWORD)stack32_pop( context );
        return VMM_RegQueryInfoKeyA( hkey, lpcSubKeys, lpcchMaxSubKey,
                                     lpcValues, lpcchMaxValueName, lpcchMaxValueData );
    }

    case 0x0021:  /* RegLoadKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        LPCSTR  lpszFile   = (LPCSTR)stack32_pop( context );
        FIXME("RegLoadKey(%p,%s,%s): stub\n",hkey, debugstr_a(lpszSubKey), debugstr_a(lpszFile));
        return ERROR_SUCCESS;
    }

    case 0x0022:  /* RegUnLoadKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        FIXME("RegUnLoadKey(%p,%s): stub\n",hkey, debugstr_a(lpszSubKey));
        return ERROR_SUCCESS;
    }

    case 0x0023:  /* RegSaveKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszFile   = (LPCSTR)stack32_pop( context );
        LPSECURITY_ATTRIBUTES sa = (LPSECURITY_ATTRIBUTES)stack32_pop( context );
        FIXME("RegSaveKey(%p,%s,%p): stub\n",hkey, debugstr_a(lpszFile),sa);
        return ERROR_SUCCESS;
    }

#if 0 /* Functions are not yet implemented in misc/registry.c */
    case 0x0024:  /* RegRemapPreDefKey */
    case 0x0026:  /* RegQueryMultipleValues */
#endif

    case 0x0027:  /* RegReplaceKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        LPCSTR  lpszNewFile= (LPCSTR)stack32_pop( context );
        LPCSTR  lpszOldFile= (LPCSTR)stack32_pop( context );
        FIXME("RegReplaceKey(%p,%s,%s,%s): stub\n", hkey, debugstr_a(lpszSubKey),
              debugstr_a(lpszNewFile),debugstr_a(lpszOldFile));
        return ERROR_SUCCESS;
    }

    case 0x0000: /* PageReserve */
      {
	LPVOID address;
	LPVOID ret;
	DWORD psize = getpagesize();
	ULONG page   = (ULONG) stack32_pop( context );
	ULONG npages = (ULONG) stack32_pop( context );
	ULONG flags  = (ULONG) stack32_pop( context );

	TRACE("PageReserve: page: %08lx, npages: %08lx, flags: %08lx partial stub!\n",
	      page, npages, flags );

	if ( page == PR_SYSTEM ) {
	  ERR("Can't reserve ring 1 memory\n");
	  return -1;
	}
	/* FIXME: This has to be handled separately for the separate
	   address-spaces we now have */
	if ( page == PR_PRIVATE || page == PR_SHARED ) page = 0;
	/* FIXME: Handle flags in some way */
	address = (LPVOID )(page * psize);
	ret = VirtualAlloc ( address, ( npages * psize ), MEM_RESERVE, 0 );
	TRACE("PageReserve: returning: %08lx\n", (DWORD )ret );
	if ( ret == NULL )
	  return -1;
	else
	  return (DWORD )ret;
      }

    case 0x0001: /* PageCommit */
      {
	LPVOID address;
	LPVOID ret;
	DWORD virt_perm;
	DWORD psize = getpagesize();
	ULONG page   = (ULONG) stack32_pop( context );
	ULONG npages = (ULONG) stack32_pop( context );
	ULONG hpd  = (ULONG) stack32_pop( context );
	ULONG pagerdata   = (ULONG) stack32_pop( context );
	ULONG flags  = (ULONG) stack32_pop( context );

	TRACE("PageCommit: page: %08lx, npages: %08lx, hpd: %08lx pagerdata: "
	      "%08lx, flags: %08lx partial stub\n",
	      page, npages, hpd, pagerdata, flags );

	if ( flags & PC_USER )
	  if ( flags & PC_WRITEABLE )
	    virt_perm = PAGE_EXECUTE_READWRITE;
	  else
	    virt_perm = PAGE_EXECUTE_READ;
	else
	  virt_perm = PAGE_NOACCESS;

	address = (LPVOID )(page * psize);
	ret = VirtualAlloc ( address, ( npages * psize ), MEM_COMMIT, virt_perm );
	TRACE("PageCommit: Returning: %08lx\n", (DWORD )ret );
	return (DWORD )ret;

      }
    case 0x0002: /* PageDecommit */
      {
	LPVOID address;
	BOOL ret;
	DWORD psize = getpagesize();
	ULONG page = (ULONG) stack32_pop( context );
	ULONG npages = (ULONG) stack32_pop( context );
	ULONG flags = (ULONG) stack32_pop( context );

	TRACE("PageDecommit: page: %08lx, npages: %08lx, flags: %08lx partial stub\n",
	      page, npages, flags );
	address = (LPVOID )( page * psize );
	ret = VirtualFree ( address, ( npages * psize ), MEM_DECOMMIT );
	TRACE("PageDecommit: Returning: %s\n", ret ? "TRUE" : "FALSE" );
	return ret;
      }
    case 0x000d: /* PageModifyPermissions */
      {
	DWORD pg_old_perm;
	DWORD pg_new_perm;
	DWORD virt_old_perm;
	DWORD virt_new_perm;
	MEMORY_BASIC_INFORMATION mbi;
	LPVOID address;
	DWORD psize = getpagesize();
	ULONG page = stack32_pop ( context );
	ULONG npages = stack32_pop ( context );
	ULONG permand = stack32_pop ( context );
	ULONG permor = stack32_pop ( context );

	TRACE("PageModifyPermissions %08lx %08lx %08lx %08lx partial stub\n",
	      page, npages, permand, permor );
	address = (LPVOID )( page * psize );

	VirtualQuery ( address, &mbi, sizeof ( MEMORY_BASIC_INFORMATION ));
	virt_old_perm = mbi.Protect;

	switch ( virt_old_perm & mbi.Protect ) {
	case PAGE_READONLY:
	case PAGE_EXECUTE:
	case PAGE_EXECUTE_READ:
	  pg_old_perm = PC_USER;
	  break;
	case PAGE_READWRITE:
	case PAGE_WRITECOPY:
	case PAGE_EXECUTE_READWRITE:
	case PAGE_EXECUTE_WRITECOPY:
	  pg_old_perm = PC_USER | PC_WRITEABLE;
	  break;
	case PAGE_NOACCESS:
	default:
	  pg_old_perm = 0;
	  break;
	}
	pg_new_perm = pg_old_perm;
	pg_new_perm &= permand & ~PC_STATIC;
	pg_new_perm |= permor  & ~PC_STATIC;

	virt_new_perm = ( virt_old_perm )  & ~0xff;
	if ( pg_new_perm & PC_USER )
	{
	  if ( pg_new_perm & PC_WRITEABLE )
	    virt_new_perm |= PAGE_EXECUTE_READWRITE;
	  else
	    virt_new_perm |= PAGE_EXECUTE_READ;
	}

	if ( ! VirtualProtect ( address, ( npages * psize ), virt_new_perm, &virt_old_perm ) ) {
	  ERR("Can't change page permissions for %08lx\n", (DWORD )address );
	  return 0xffffffff;
	}
	TRACE("Returning: %08lx\n", pg_old_perm );
	return pg_old_perm;
      }
    case 0x000a: /* PageFree */
      {
	BOOL ret;
	LPVOID hmem = (LPVOID) stack32_pop( context );
	DWORD flags = (DWORD ) stack32_pop( context );

	TRACE("PageFree: hmem: %08lx, flags: %08lx partial stub\n",
	      (DWORD )hmem, flags );

	ret = VirtualFree ( hmem, 0, MEM_RELEASE );
	context->Eax = ret;
	TRACE("Returning: %d\n", ret );

	return 0;
      }
    case 0x001e: /* GetDemandPageInfo */
      {
	 DWORD dinfo = (DWORD)stack32_pop( context );
         DWORD flags = (DWORD)stack32_pop( context );

	 /* GetDemandPageInfo is supposed to fill out the struct at
          * "dinfo" with various low-level memory management information.
          * Apps are certainly not supposed to call this, although it's
          * demoed and documented by Pietrek on pages 441-443 of "Windows
          * 95 System Programming Secrets" if any program needs a real
          * implementation of this.
	  */

	 FIXME("GetDemandPageInfo(%08lx %08lx): stub!\n", dinfo, flags);

	 return 0;
      }
    default:
        if (LOWORD(service) < N_VMM_SERVICE)
            FIXME( "Unimplemented service %s (%08lx)\n",
                          VMM_Service_Name[LOWORD(service)], service);
        else
            FIXME( "Unknown service %08lx\n", service);
        break;
    }

    return 0xffffffff;  /* FIXME */
}

/***********************************************************************
 *           DeviceIo_IFSMgr
 * NOTES
 *   These ioctls are used by 'MSNET32.DLL'.
 *
 *   I have been unable to uncover any documentation about the ioctls so
 *   the implementation of the cases IFS_IOCTL_21 and IFS_IOCTL_2F are
 *   based on reasonable guesses on information found in the Windows 95 DDK.
 *
 */

/*
 * IFSMgr DeviceIO service
 */

#define IFS_IOCTL_21                100
#define IFS_IOCTL_2F                101
#define	IFS_IOCTL_GET_RES           102
#define IFS_IOCTL_GET_NETPRO_NAME_A 103

struct win32apireq {
	unsigned long 	ar_proid;
	unsigned long  	ar_eax;
	unsigned long  	ar_ebx;
	unsigned long  	ar_ecx;
	unsigned long  	ar_edx;
	unsigned long  	ar_esi;
	unsigned long  	ar_edi;
	unsigned long  	ar_ebp;
	unsigned short 	ar_error;
	unsigned short  ar_pad;
};

static void win32apieq_2_CONTEXT(struct win32apireq *pIn,CONTEXT86 *pCxt)
{
	memset(pCxt,0,sizeof(*pCxt));

	pCxt->ContextFlags=CONTEXT86_INTEGER|CONTEXT86_CONTROL;
	pCxt->Eax = pIn->ar_eax;
	pCxt->Ebx = pIn->ar_ebx;
	pCxt->Ecx = pIn->ar_ecx;
	pCxt->Edx = pIn->ar_edx;
	pCxt->Esi = pIn->ar_esi;
	pCxt->Edi = pIn->ar_edi;

	/* FIXME: Only partial CONTEXT86_CONTROL */
	pCxt->Ebp = pIn->ar_ebp;

	/* FIXME: pIn->ar_proid ignored */
	/* FIXME: pIn->ar_error ignored */
	/* FIXME: pIn->ar_pad ignored */
}

static void CONTEXT_2_win32apieq(CONTEXT86 *pCxt,struct win32apireq *pOut)
{
	memset(pOut,0,sizeof(struct win32apireq));

	pOut->ar_eax = pCxt->Eax;
	pOut->ar_ebx = pCxt->Ebx;
	pOut->ar_ecx = pCxt->Ecx;
	pOut->ar_edx = pCxt->Edx;
	pOut->ar_esi = pCxt->Esi;
	pOut->ar_edi = pCxt->Edi;

	/* FIXME: Only partial CONTEXT86_CONTROL */
	pOut->ar_ebp = pCxt->Ebp;

	/* FIXME: pOut->ar_proid ignored */
	/* FIXME: pOut->ar_error ignored */
	/* FIXME: pOut->ar_pad ignored */
}

static BOOL DeviceIo_IFSMgr(DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
    BOOL retv = TRUE;
	TRACE("(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
			dwIoControlCode,
			lpvInBuffer,cbInBuffer,
			lpvOutBuffer,cbOutBuffer,
			lpcbBytesReturned,
			lpOverlapped);

    switch (dwIoControlCode)
    {
	case IFS_IOCTL_21:
	case IFS_IOCTL_2F:{
		CONTEXT86 cxt;
		struct win32apireq *pIn=(struct win32apireq *) lpvInBuffer;
		struct win32apireq *pOut=(struct win32apireq *) lpvOutBuffer;

		TRACE(
			"Control '%s': "
			"proid=0x%08lx, eax=0x%08lx, ebx=0x%08lx, ecx=0x%08lx, "
			"edx=0x%08lx, esi=0x%08lx, edi=0x%08lx, ebp=0x%08lx, "
			"error=0x%04x, pad=0x%04x\n",
			(dwIoControlCode==IFS_IOCTL_21)?"IFS_IOCTL_21":"IFS_IOCTL_2F",
			pIn->ar_proid, pIn->ar_eax, pIn->ar_ebx, pIn->ar_ecx,
			pIn->ar_edx, pIn->ar_esi, pIn->ar_edi, pIn->ar_ebp,
			pIn->ar_error, pIn->ar_pad
		);

		win32apieq_2_CONTEXT(pIn,&cxt);

		if(dwIoControlCode==IFS_IOCTL_21)
		{
                    if(Dosvm.CallBuiltinHandler || DPMI_LoadDosSystem())
                        Dosvm.CallBuiltinHandler( &cxt, 0x21 );
               } 
                else 
                {
                    if(Dosvm.CallBuiltinHandler || DPMI_LoadDosSystem())
                        Dosvm.CallBuiltinHandler( &cxt, 0x2f );
		}

		CONTEXT_2_win32apieq(&cxt,pOut);

        retv = TRUE;
	} break;
	case IFS_IOCTL_GET_RES:{
        FIXME( "Control 'IFS_IOCTL_GET_RES' not implemented\n");
        retv = FALSE;
	} break;
	case IFS_IOCTL_GET_NETPRO_NAME_A:{
        FIXME( "Control 'IFS_IOCTL_GET_NETPRO_NAME_A' not implemented\n");
        retv = FALSE;
	} break;
    default:
        FIXME( "Control %ld not implemented\n", dwIoControlCode);
        retv = FALSE;
    }

    return retv;
}

/********************************************************************************
 *      VxDCall_VWin32
 *
 *  Service numbers taken from page 448 of Pietrek's "Windows 95 System
 *  Programming Secrets".  Parameters from experimentation on real Win98.
 *
 */

static DWORD VxDCall_VWin32( DWORD service, CONTEXT86 *context )
{
  switch ( LOWORD(service) )
    {
    case 0x0000: /* GetVersion */
    {
        DWORD vers = GetVersion();
        return (LOBYTE(vers) << 8) | HIBYTE(vers);
    }
    break;

    case 0x0020: /* Get VMCPD Version */
    {
	DWORD parm = (DWORD) stack32_pop(context);

	FIXME("Get VMCPD Version(%08lx): partial stub!\n", parm);

	/* FIXME: This is what Win98 returns, it may
         *        not be correct in all situations.
         *        It makes Bleem! happy though.
         */

	return 0x0405;
    }

    case 0x0029: /* Int31/DPMI dispatch */
    {
	DWORD callnum = (DWORD) stack32_pop(context);
        DWORD parm    = (DWORD) stack32_pop(context);

        TRACE("Int31/DPMI dispatch(%08lx)\n", callnum);

	SET_AX( context, callnum );
        SET_CX( context, parm );
        if(Dosvm.CallBuiltinHandler || DPMI_LoadDosSystem())
            Dosvm.CallBuiltinHandler( context, 0x31 );

	return LOWORD(context->Eax);
    }
    break;

    case 0x002a: /* Int41 dispatch - parm = int41 service number */
    {
	DWORD callnum = (DWORD) stack32_pop(context);

	return callnum; /* FIXME: should really call INT_Int41Handler() */
    }
    break;

    default:
      FIXME("Unknown VWin32 service %08lx\n", service);
      break;
    }

  return 0xffffffff;
}


/***********************************************************************
 *           DeviceIo_VCD
 */
static BOOL DeviceIo_VCD(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
    BOOL retv = TRUE;

    switch (dwIoControlCode)
    {
    case IOCTL_SERIAL_LSRMST_INSERT:
    {
        FIXME( "IOCTL_SERIAL_LSRMST_INSERT NIY !\n");
        retv = FALSE;
    }
    break;

    default:
        FIXME( "Unknown Control %ld\n", dwIoControlCode);
        retv = FALSE;
        break;
    }

    return retv;
}


/***********************************************************************
 *           DeviceIo_VWin32
 */

static void DIOCRegs_2_CONTEXT( DIOC_REGISTERS *pIn, CONTEXT86 *pCxt )
{
    memset( pCxt, 0, sizeof(*pCxt) );
    /* Note: segment registers == 0 means that CTX_SEG_OFF_TO_LIN
             will interpret 32-bit register contents as linear pointers */

    pCxt->ContextFlags=CONTEXT86_INTEGER|CONTEXT86_CONTROL;
    pCxt->Eax = pIn->reg_EAX;
    pCxt->Ebx = pIn->reg_EBX;
    pCxt->Ecx = pIn->reg_ECX;
    pCxt->Edx = pIn->reg_EDX;
    pCxt->Esi = pIn->reg_ESI;
    pCxt->Edi = pIn->reg_EDI;

    /* FIXME: Only partial CONTEXT86_CONTROL */
    pCxt->EFlags = pIn->reg_Flags;
}

static void CONTEXT_2_DIOCRegs( CONTEXT86 *pCxt, DIOC_REGISTERS *pOut )
{
    memset( pOut, 0, sizeof(DIOC_REGISTERS) );

    pOut->reg_EAX = pCxt->Eax;
    pOut->reg_EBX = pCxt->Ebx;
    pOut->reg_ECX = pCxt->Ecx;
    pOut->reg_EDX = pCxt->Edx;
    pOut->reg_ESI = pCxt->Esi;
    pOut->reg_EDI = pCxt->Edi;

    /* FIXME: Only partial CONTEXT86_CONTROL */
    pOut->reg_Flags = pCxt->EFlags;
}

#define DIOC_AH(regs) (((unsigned char*)&((regs)->reg_EAX))[1])
#define DIOC_AL(regs) (((unsigned char*)&((regs)->reg_EAX))[0])
#define DIOC_BH(regs) (((unsigned char*)&((regs)->reg_EBX))[1])
#define DIOC_BL(regs) (((unsigned char*)&((regs)->reg_EBX))[0])
#define DIOC_DH(regs) (((unsigned char*)&((regs)->reg_EDX))[1])
#define DIOC_DL(regs) (((unsigned char*)&((regs)->reg_EDX))[0])

#define DIOC_AX(regs) (((unsigned short*)&((regs)->reg_EAX))[0])
#define DIOC_BX(regs) (((unsigned short*)&((regs)->reg_EBX))[0])
#define DIOC_CX(regs) (((unsigned short*)&((regs)->reg_ECX))[0])
#define DIOC_DX(regs) (((unsigned short*)&((regs)->reg_EDX))[0])

#define DIOC_SET_CARRY(regs) (((regs)->reg_Flags)|=0x00000001)

static BOOL DeviceIo_VWin32(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
    BOOL retv = TRUE;

    switch (dwIoControlCode)
    {
    case VWIN32_DIOC_DOS_IOCTL:
    case 0x10: /* Int 0x21 call, call it VWIN_DIOC_INT21 ? */
    case VWIN32_DIOC_DOS_INT13:
    case VWIN32_DIOC_DOS_INT25:
    case VWIN32_DIOC_DOS_INT26:
    case 0x29: /* Int 0x31 call, call it VWIN_DIOC_INT31 ? */
    case VWIN32_DIOC_DOS_DRIVEINFO:
    {
        CONTEXT86 cxt;
        DIOC_REGISTERS *pIn  = (DIOC_REGISTERS *)lpvInBuffer;
        DIOC_REGISTERS *pOut = (DIOC_REGISTERS *)lpvOutBuffer;
        BYTE intnum = 0;

        TRACE( "Control '%s': "
               "eax=0x%08lx, ebx=0x%08lx, ecx=0x%08lx, "
               "edx=0x%08lx, esi=0x%08lx, edi=0x%08lx \n",
               (dwIoControlCode == VWIN32_DIOC_DOS_IOCTL)? "VWIN32_DIOC_DOS_IOCTL" :
               (dwIoControlCode == VWIN32_DIOC_DOS_INT25)? "VWIN32_DIOC_DOS_INT25" :
               (dwIoControlCode == VWIN32_DIOC_DOS_INT26)? "VWIN32_DIOC_DOS_INT26" :
               (dwIoControlCode == VWIN32_DIOC_DOS_DRIVEINFO)? "VWIN32_DIOC_DOS_DRIVEINFO" :  "???",
               pIn->reg_EAX, pIn->reg_EBX, pIn->reg_ECX,
               pIn->reg_EDX, pIn->reg_ESI, pIn->reg_EDI );

        DIOCRegs_2_CONTEXT( pIn, &cxt );

        switch (dwIoControlCode)
        {
        case VWIN32_DIOC_DOS_IOCTL: /* Call int 21h */
        case 0x10: /* Int 0x21 call, call it VWIN_DIOC_INT21 ? */
        case VWIN32_DIOC_DOS_DRIVEINFO:        /* Call int 21h 730x */
            intnum = 0x21;
            break;
        case VWIN32_DIOC_DOS_INT13:
            intnum = 0x13;
            break;
        case VWIN32_DIOC_DOS_INT25: 
            intnum = 0x25;
            break;
        case VWIN32_DIOC_DOS_INT26:
            intnum = 0x26;
            break;
        case 0x29: /* Int 0x31 call, call it VWIN_DIOC_INT31 ? */
            intnum = 0x31;
            break;
        }

        if(Dosvm.CallBuiltinHandler || DPMI_LoadDosSystem())
            Dosvm.CallBuiltinHandler( &cxt, intnum );

        CONTEXT_2_DIOCRegs( &cxt, pOut );
    }
    break;

    case VWIN32_DIOC_SIMCTRLC:
        FIXME( "Control VWIN32_DIOC_SIMCTRLC not implemented\n");
        retv = FALSE;
        break;

    default:
        FIXME( "Unknown Control %ld\n", dwIoControlCode);
        retv = FALSE;
        break;
    }

    return retv;
}

/* this is the main multimedia device loader */
static BOOL DeviceIo_MMDEVLDR(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
	FIXME("(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
	    dwIoControlCode,
	    lpvInBuffer,cbInBuffer,
	    lpvOutBuffer,cbOutBuffer,
	    lpcbBytesReturned,
	    lpOverlapped
	);
	switch (dwIoControlCode) {
	case 5:
		/* Hmm. */
		*(DWORD*)lpvOutBuffer=0;
		*lpcbBytesReturned=4;
		return TRUE;
	}
	return FALSE;
}
/* this is used by some Origin games */
static BOOL DeviceIo_MONODEBG(DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
	switch (dwIoControlCode) {
	case 1:	/* version */
		*(LPDWORD)lpvOutBuffer = 0x20004; /* WC SecretOps */
		break;
	case 9: /* debug output */
		ERR("MONODEBG: %s\n",debugstr_a(lpvInBuffer));
		break;
	default:
		FIXME("(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
			dwIoControlCode,
			lpvInBuffer,cbInBuffer,
			lpvOutBuffer,cbOutBuffer,
			lpcbBytesReturned,
			lpOverlapped
		);
		break;
	}
	return TRUE;
}
/* pccard */
static BOOL DeviceIo_PCCARD (DWORD dwIoControlCode,
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
	switch (dwIoControlCode) {
	case 0x0000: /* PCCARD_Get_Version */
	case 0x0001: /* PCCARD_Card_Services */
	default:
		FIXME( "(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
			dwIoControlCode,
			lpvInBuffer,cbInBuffer,
			lpvOutBuffer,cbOutBuffer,
			lpcbBytesReturned,
			lpOverlapped
		);
		break;
	}
	return FALSE;
}

/***********************************************************************
 *		OpenVxDHandle (KERNEL32.@)
 *
 *	This function is supposed to return the corresponding Ring 0
 *	("kernel") handle for a Ring 3 handle in Win9x.
 *	Evidently, Wine will have problems with this. But we try anyway,
 *	maybe it helps...
 */
HANDLE	WINAPI	OpenVxDHandle(HANDLE hHandleRing3)
{
	FIXME( "(%p), stub! (returning Ring 3 handle instead of Ring 0)\n", hHandleRing3);
	return hHandleRing3;
}

static BOOL DeviceIo_HASP(DWORD dwIoControlCode, LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
    BOOL retv = TRUE;
	FIXME("(%ld,%p,%ld,%p,%ld,%p,%p): stub\n",
			dwIoControlCode,
			lpvInBuffer,cbInBuffer,
			lpvOutBuffer,cbOutBuffer,
			lpcbBytesReturned,
			lpOverlapped);

    return retv;
}
