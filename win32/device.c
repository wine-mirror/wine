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
#include <unistd.h>
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

/* int 13 stuff */
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef linux
# include <linux/fd.h>
#endif
#include "drive.h"

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



HANDLE DEVICE_Open( LPCSTR filename, DWORD access, LPSECURITY_ATTRIBUTES sa )
{
    const struct VxDInfo *info;

    for (info = VxDList; info->name; info++)
        if (!strncasecmp( info->name, filename, strlen(info->name) ))
            return FILE_CreateDevice( info->id | 0x10000, access, sa );

    FIXME( "Unknown/unsupported VxD %s. Try --winver nt40 or win31 !\n", filename);
    SetLastError( ERROR_FILE_NOT_FOUND );
    return 0;
}

static DWORD DEVICE_GetClientID( HANDLE handle )
{
    DWORD       ret = 0;
    SERVER_START_REQ( get_file_info )
    {
        req->handle = handle;
        if (!wine_server_call( req ) && (reply->type == FILE_TYPE_UNKNOWN))
            ret = reply->attr;
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

        TRACE( "(%d,%ld,%p,%ld,%p,%ld,%p,%p)\n",
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
                    return CDROM_DeviceIoControl(clientID, hDevice, dwIoControlCode, lpvInBuffer, cbInBuffer,
                                                 lpvOutBuffer, cbOutBuffer, lpcbBytesReturned,
                                                 lpOverlapped);
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
        LPHKEY  retkey     = (LPHKEY)stack32_pop( context );
        return RegOpenKeyA( hkey, lpszSubKey, retkey );
    }

    case 0x0012:  /* RegCreateKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        LPHKEY  retkey     = (LPHKEY)stack32_pop( context );
        return RegCreateKeyA( hkey, lpszSubKey, retkey );
    }

    case 0x0013:  /* RegCloseKey */
    {
        HKEY    hkey       = (HKEY)stack32_pop( context );
        return RegCloseKey( hkey );
    }

    case 0x0014:  /* RegDeleteKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        return RegDeleteKeyA( hkey, lpszSubKey );
    }

    case 0x0015:  /* RegSetValue */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        DWORD   dwType     = (DWORD) stack32_pop( context );
        LPCSTR  lpszData   = (LPCSTR)stack32_pop( context );
        DWORD   cbData     = (DWORD) stack32_pop( context );
        return RegSetValueA( hkey, lpszSubKey, dwType, lpszData, cbData );
    }

    case 0x0016:  /* RegDeleteValue */
    {
        HKEY    hkey       = (HKEY) stack32_pop( context );
        LPSTR   lpszValue  = (LPSTR)stack32_pop( context );
        return RegDeleteValueA( hkey, lpszValue );
    }

    case 0x0017:  /* RegQueryValue */
    {
        HKEY    hkey       = (HKEY)   stack32_pop( context );
        LPSTR   lpszSubKey = (LPSTR)  stack32_pop( context );
        LPSTR   lpszData   = (LPSTR)  stack32_pop( context );
        LPDWORD lpcbData   = (LPDWORD)stack32_pop( context );
        return RegQueryValueA( hkey, lpszSubKey, lpszData, lpcbData );
    }

    case 0x0018:  /* RegEnumKey */
    {
        HKEY    hkey       = (HKEY) stack32_pop( context );
        DWORD   iSubkey    = (DWORD)stack32_pop( context );
        LPSTR   lpszName   = (LPSTR)stack32_pop( context );
        DWORD   lpcchName  = (DWORD)stack32_pop( context );
        return RegEnumKeyA( hkey, iSubkey, lpszName, lpcchName );
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
        return RegEnumValueA( hkey, iValue, lpszValue, lpcchValue, 
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
        return RegQueryValueExA( hkey, lpszValue, lpReserved, 
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
        return RegSetValueExA( hkey, lpszValue, dwReserved, 
                               dwType, lpbData, cbData );
    }

    case 0x001C:  /* RegFlushKey */
    {
        HKEY    hkey       = (HKEY)stack32_pop( context );
        return RegFlushKey( hkey );
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
        return RegQueryInfoKeyA( hkey, NULL, NULL, NULL, lpcSubKeys, lpcchMaxSubKey,
                                 NULL, lpcValues, lpcchMaxValueName, lpcchMaxValueData,
                                 NULL, NULL );
    }

    case 0x0021:  /* RegLoadKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        LPCSTR  lpszFile   = (LPCSTR)stack32_pop( context );
        return RegLoadKeyA( hkey, lpszSubKey, lpszFile );
    }

    case 0x0022:  /* RegUnLoadKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszSubKey = (LPCSTR)stack32_pop( context );
        return RegUnLoadKeyA( hkey, lpszSubKey );
    }

    case 0x0023:  /* RegSaveKey */
    {
        HKEY    hkey       = (HKEY)  stack32_pop( context );
        LPCSTR  lpszFile   = (LPCSTR)stack32_pop( context );
        LPSECURITY_ATTRIBUTES sa = (LPSECURITY_ATTRIBUTES)stack32_pop( context );
        return RegSaveKeyA( hkey, lpszFile, sa );
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
        return RegReplaceKeyA( hkey, lpszSubKey, lpszNewFile, lpszOldFile );
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
			DOS3Call(&cxt); /* Call int 21h */
		} else {
			INT_Int2fHandler(&cxt); /* Call int 2Fh */
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

	AX_reg(context) = callnum;
        CX_reg(context) = parm;  
	INT_Int31Handler(context);

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

static const DWORD VWIN32_DriveTypeInfo[7]={
    0x0000, /* none */
    0x2709, /* 360 K */
    0x4f0f, /* 1.2 M */
    0x4f09, /* 720 K */
    0x4f12, /* 1.44 M */
    0x4f24, /* 2.88 M */
    0x4f24  /* 2.88 M */
};

static BYTE floppy_params[2][13] =
{
    { 0xaf, 0x02, 0x25, 0x02, 0x12, 0x1b, 0xff, 0x6c, 0xf6, 0x0f, 0x08 },
    { 0xaf, 0x02, 0x25, 0x02, 0x12, 0x1b, 0xff, 0x6c, 0xf6, 0x0f, 0x08 }
};

/**********************************************************************
 *	    VWIN32_ReadFloppyParams
 *
 * Handler for int 13h (disk I/O).
 */
static VOID VWIN32_ReadFloppyParams(DIOC_REGISTERS *regs)
{
#ifdef linux
    unsigned int i, nr_of_drives = 0;
    BYTE drive_nr = DIOC_DL(regs);
    int floppy_fd,r;
    struct floppy_drive_params floppy_parm;
    char root[] = "A:\\";

    TRACE("in  [ EDX=%08lx ]\n", regs->reg_EDX );

    DIOC_AH(regs) = 0x00; /* success */

    for (i = 0; i < MAX_DOS_DRIVES; i++, root[0]++)
        if (GetDriveTypeA(root) == DRIVE_REMOVABLE) nr_of_drives++;
    DIOC_DL(regs) = nr_of_drives;

    if (drive_nr > 1) { /* invalid drive ? */
        DIOC_BX(regs) = 0;
        DIOC_CX(regs) = 0;
        DIOC_DH(regs) = 0;
        DIOC_SET_CARRY(regs);
        return;
    }

    if ( (floppy_fd = DRIVE_OpenDevice( drive_nr, O_NONBLOCK)) == -1)
    {
        WARN("Can't determine floppy geometry !\n");
        DIOC_BX(regs) = 0;
        DIOC_CX(regs) = 0;
        DIOC_DH(regs) = 0;
        DIOC_SET_CARRY(regs);
        return;
    }
    r = ioctl(floppy_fd, FDGETDRVPRM, &floppy_parm);

    close(floppy_fd);

    if(r<0)
    {
        DIOC_SET_CARRY(regs);
        return;
    }

    regs->reg_ECX = 0;
    DIOC_AL(regs) = 0;
    DIOC_BL(regs) = floppy_parm.cmos;

    /* CH = low eight bits of max cyl
       CL = max sec nr (bits 5-0),
       hi two bits of max cyl (bits 7-6)
       DH = max head nr */
    if(DIOC_BL(regs) && (DIOC_BL(regs)<7))
    {
        DIOC_DH(regs) = 0x01;
        DIOC_CX(regs) = VWIN32_DriveTypeInfo[DIOC_BL(regs)];
    }
    else
    {
        DIOC_CX(regs) = 0x0;
        DIOC_DX(regs) = 0x0;
    }

    regs->reg_EDI = (DWORD)floppy_params[drive_nr];

    if(!regs->reg_EDI)
    {
        ERR("Get floppy params failed for drive %d\n",drive_nr);
        DIOC_SET_CARRY(regs);
    }

    TRACE("out [ EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx EDI=%08lx ]\n",
          regs->reg_EAX, regs->reg_EBX, regs->reg_ECX, regs->reg_EDX, regs->reg_EDI);

    /* FIXME: Word exits quietly if we return with no error. Why? */
    FIXME("Returned ERROR!\n");
    DIOC_SET_CARRY(regs);

#else
    DIOC_AH(regs) = 0x01;
    DIOC_SET_CARRY(regs);
#endif
}

/**********************************************************************
 *	    VWIN32_Int13Handler
 *
 * Handler for VWIN32_DIOC_DOS_INT13 (disk I/O).
 */
static VOID VWIN32_Int13Handler( DIOC_REGISTERS *regs)
{
    TRACE("AH=%02x\n",DIOC_AH(regs));
    switch(DIOC_AH(regs)) /* AH */
    {
    case 0x00:             /* RESET DISK SYSTEM     */
        break; /* no return ? */

    case 0x01:             /* STATUS OF DISK SYSTEM */
        DIOC_AL(regs) = 0; /* successful completion */
        break;

    case 0x02:             /* READ SECTORS INTO MEMORY */
        DIOC_AL(regs) = 0; /* number of sectors read */
        DIOC_AH(regs) = 0; /* status */
        break;

    case 0x03:             /* WRITE SECTORS FROM MEMORY */
        break; /* no return ? */

    case 0x04:             /* VERIFY DISK SECTOR(S) */
        DIOC_AL(regs) = 0; /* number of sectors verified */
        DIOC_AH(regs) = 0;
        break;

    case 0x05:             /* FORMAT TRACK */
    case 0x06:             /* FORMAT TRACK AND SET BAD SECTOR FLAGS */
    case 0x07:             /* FORMAT DRIVE STARTING AT GIVEN TRACK  */
        /* despite what Ralf Brown says, 0x06 and 0x07 seem to
         * set CFLAG, too (at least my BIOS does that) */
        DIOC_AH(regs) = 0x0c;
        DIOC_SET_CARRY(regs);
        break;

    case 0x08:             /* GET DRIVE PARAMETERS  */
        if (DIOC_DL(regs) & 0x80) { /* hard disk ? */
            DIOC_AH(regs) = 0x07;
            DIOC_SET_CARRY(regs);
        }
        else  /* floppy disk */
            VWIN32_ReadFloppyParams(regs);
        break;

    case 0x09:         /* INITIALIZE CONTROLLER WITH DRIVE PARAMETERS */
    case 0x0a:         /* FIXED DISK - READ LONG (XT,AT,XT286,PS)     */
    case 0x0b:         /* FIXED DISK - WRITE LONG (XT,AT,XT286,PS)    */
    case 0x0c:         /* SEEK TO CYLINDER                            */
    case 0x0d:         /* ALTERNATE RESET HARD DISKS                  */
    case 0x10:         /* CHECK IF DRIVE READY                        */
    case 0x11:         /* RECALIBRATE DRIVE                           */
    case 0x14:         /* CONTROLLER INTERNAL DIAGNOSTIC              */
        DIOC_AH(regs) = 0;
        break;

    case 0x15:         /* GET DISK TYPE (AT,XT2,XT286,CONV,PS) */
        if (DIOC_DL(regs) & 0x80) { /* hard disk ? */
            DIOC_AH(regs) = 3; /* fixed disk */
            DIOC_SET_CARRY(regs);
        }
        else { /* floppy disk ? */
            DIOC_AH(regs) = 2; /* floppy with change detection */
            DIOC_SET_CARRY(regs);
        }
        break;

    case 0x0e:         /* READ SECTOR BUFFER (XT only)      */
    case 0x0f:         /* WRITE SECTOR BUFFER (XT only)     */
    case 0x12:         /* CONTROLLER RAM DIAGNOSTIC (XT,PS) */
    case 0x13:         /* DRIVE DIAGNOSTIC (XT,PS)          */
        DIOC_AH(regs) = 0x01;
        DIOC_SET_CARRY(regs);
        break;

    case 0x16:         /* FLOPPY - CHANGE OF DISK STATUS */
        DIOC_AH(regs) = 0; /* FIXME - no change */
        break;

    case 0x17:         /* SET DISK TYPE FOR FORMAT */
        if (DIOC_DL(regs) < 4)
            DIOC_AH(regs) = 0x00; /* successful completion */
        else
            DIOC_AH(regs) = 0x01; /* error */
        break;

    case 0x18:         /* SET MEDIA TYPE FOR FORMAT */
        if (DIOC_DL(regs) < 4)
            DIOC_AH(regs) = 0x00; /* successful completion */
        else
            DIOC_AH(regs) = 0x01; /* error */
        break;

    case 0x19:       /* FIXED DISK - PARK HEADS */
        break;

    default:
        FIXME("Unknown VWIN32 INT13 call AX=%04X\n",DIOC_AX(regs));
    }
}

static BOOL DeviceIo_VWin32(DWORD dwIoControlCode, 
			      LPVOID lpvInBuffer, DWORD cbInBuffer,
			      LPVOID lpvOutBuffer, DWORD cbOutBuffer,
			      LPDWORD lpcbBytesReturned,
			      LPOVERLAPPED lpOverlapped)
{
    BOOL retv = TRUE;

    switch (dwIoControlCode)
    {
    case VWIN32_DIOC_DOS_INT13:
    {
        DIOC_REGISTERS *pIn  = (DIOC_REGISTERS *)lpvInBuffer;
        DIOC_REGISTERS *pOut = (DIOC_REGISTERS *)lpvOutBuffer;

        memcpy(pOut, pIn, sizeof (DIOC_REGISTERS));
        VWIN32_Int13Handler(pOut);
        break;
    }

    case VWIN32_DIOC_DOS_IOCTL:
    case VWIN32_DIOC_DOS_INT25:
    case VWIN32_DIOC_DOS_INT26:
    case VWIN32_DIOC_DOS_DRIVEINFO:
    {
        CONTEXT86 cxt;
        DIOC_REGISTERS *pIn  = (DIOC_REGISTERS *)lpvInBuffer;
        DIOC_REGISTERS *pOut = (DIOC_REGISTERS *)lpvOutBuffer;

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
        case VWIN32_DIOC_DOS_IOCTL: DOS3Call( &cxt ); break; /* Call int 21h */
        case VWIN32_DIOC_DOS_INT25: INT_Int25Handler( &cxt ); break;
        case VWIN32_DIOC_DOS_INT26: INT_Int26Handler( &cxt ); break;
        case VWIN32_DIOC_DOS_DRIVEINFO:	DOS3Call( &cxt ); break; /* Call int 21h 730x */
        }

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
 */
DWORD	WINAPI	OpenVxDHandle(DWORD pmt)
{
	FIXME( "(0x%08lx) stub!\n", pmt);
	return 0;
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

