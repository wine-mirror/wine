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

#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"
#include "winnls.h"
#include "file.h"
#include "winioctl.h"
#include "winnt.h"
#include "msdos.h"
#include "miscemu.h"
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
    BOOL  (*deviceio)(DWORD, LPVOID, DWORD,
                        LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
};

static const struct VxDInfo VxDList[] =
{
    /* Standard VxD IDs */
    { "VMM",      0x0001, NULL },
    { "DEBUG",    0x0002, NULL },
    { "VPICD",    0x0003, NULL },
    { "VDMAD",    0x0004, NULL },
    { "VTD",      0x0005, NULL },
    { "V86MMGR",  0x0006, NULL },
    { "PAGESWAP", 0x0007, NULL },
    { "PARITY",   0x0008, NULL },
    { "REBOOT",   0x0009, NULL },
    { "VDD",      0x000A, NULL },
    { "VSD",      0x000B, NULL },
    { "VMD",      0x000C, NULL },
    { "VKD",      0x000D, NULL },
    { "VCD",      0x000E, DeviceIo_VCD },
    { "VPD",      0x000F, NULL },
    { "BLOCKDEV", 0x0010, NULL },
    { "VMCPD",    0x0011, NULL },
    { "EBIOS",    0x0012, NULL },
    { "BIOSXLAT", 0x0013, NULL },
    { "VNETBIOS", 0x0014, NULL },
    { "DOSMGR",   0x0015, NULL },
    { "WINLOAD",  0x0016, NULL },
    { "SHELL",    0x0017, NULL },
    { "VMPOLL",   0x0018, NULL },
    { "VPROD",    0x0019, NULL },
    { "DOSNET",   0x001A, NULL },
    { "VFD",      0x001B, NULL },
    { "VDD2",     0x001C, NULL },
    { "WINDEBUG", 0x001D, NULL },
    { "TSRLOAD",  0x001E, NULL },
    { "BIOSHOOK", 0x001F, NULL },
    { "INT13",    0x0020, NULL },
    { "PAGEFILE", 0x0021, NULL },
    { "SCSI",     0x0022, NULL },
    { "MCA_POS",  0x0023, NULL },
    { "SCSIFD",   0x0024, NULL },
    { "VPEND",    0x0025, NULL },
    { "VPOWERD",  0x0026, NULL },
    { "VXDLDR",   0x0027, NULL },
    { "NDIS",     0x0028, NULL },
    { "BIOS_EXT", 0x0029, NULL },
    { "VWIN32",   0x002A, DeviceIo_VWin32 },
    { "VCOMM",    0x002B, NULL },
    { "SPOOLER",  0x002C, NULL },
    { "WIN32S",   0x002D, NULL },
    { "DEBUGCMD", 0x002E, NULL },

    { "VNB",      0x0031, NULL },
    { "SERVER",   0x0032, NULL },
    { "CONFIGMG", 0x0033, NULL },
    { "DWCFGMG",  0x0034, NULL },
    { "SCSIPORT", 0x0035, NULL },
    { "VFBACKUP", 0x0036, NULL },
    { "ENABLE",   0x0037, NULL },
    { "VCOND",    0x0038, NULL },

    { "EFAX",     0x003A, NULL },
    { "DSVXD",    0x003B, NULL },
    { "ISAPNP",   0x003C, NULL },
    { "BIOS",     0x003D, NULL },
    { "WINSOCK",  0x003E, NULL },
    { "WSOCK",    0x003E, NULL },
    { "WSIPX",    0x003F, NULL },
    { "IFSMgr",   0x0040, DeviceIo_IFSMgr },
    { "VCDFSD",   0x0041, NULL },
    { "MRCI2",    0x0042, NULL },
    { "PCI",      0x0043, NULL },
    { "PELOADER", 0x0044, NULL },
    { "EISA",     0x0045, NULL },
    { "DRAGCLI",  0x0046, NULL },
    { "DRAGSRV",  0x0047, NULL },
    { "PERF",     0x0048, NULL },
    { "AWREDIR",  0x0049, NULL },

    /* Far East support */
    { "ETEN",     0x0060, NULL },
    { "CHBIOS",   0x0061, NULL },
    { "VMSGD",    0x0062, NULL },
    { "VPPID",    0x0063, NULL },
    { "VIME",     0x0064, NULL },
    { "VHBIOSD",  0x0065, NULL },

    /* Multimedia OEM IDs */
    { "VTDAPI",   0x0442, DeviceIo_VTDAPI },
    { "MMDEVLDR", 0x044A, DeviceIo_MMDEVLDR },

    /* Network Device IDs */
    { "VNetSup",  0x0480, NULL },
    { "VRedir",   0x0481, NULL },
    { "VBrowse",  0x0482, NULL },
    { "VSHARE",   0x0483, NULL },
    { "IFSMgr",   0x0484, NULL },
    { "MEMPROBE", 0x0485, NULL },
    { "VFAT",     0x0486, NULL },
    { "NWLINK",   0x0487, NULL },
    { "VNWLINK",  0x0487, NULL },
    { "NWSUP",    0x0487, NULL },
    { "VTDI",     0x0488, NULL },
    { "VIP",      0x0489, NULL },
    { "VTCP",     0x048A, NULL },
    { "VCache",   0x048B, NULL },
    { "VUDP",     0x048C, NULL },
    { "VAsync",   0x048D, NULL },
    { "NWREDIR",  0x048E, NULL },
    { "STAT80",   0x048F, NULL },
    { "SCSIPORT", 0x0490, NULL },
    { "FILESEC",  0x0491, NULL },
    { "NWSERVER", 0x0492, NULL },
    { "SECPROV",  0x0493, NULL },
    { "NSCL",     0x0494, NULL },
    { "WSTCP",    0x0495, NULL },
    { "NDIS2SUP", 0x0496, NULL },
    { "MSODISUP", 0x0497, NULL },
    { "Splitter", 0x0498, NULL },
    { "PPP",      0x0499, NULL },
    { "VDHCP",    0x049A, NULL },
    { "VNBT",     0x049B, NULL },
    { "LOGGER",   0x049D, NULL },
    { "EFILTER",  0x049E, NULL },
    { "FFILTER",  0x049F, NULL },
    { "TFILTER",  0x04A0, NULL },
    { "AFILTER",  0x04A1, NULL },
    { "IRLAMP",   0x04A2, NULL },

    { "PCCARD",   0x097C, DeviceIo_PCCARD },
    { "HASP95",   0x3721, DeviceIo_HASP },

    /* WINE additions, ids unknown */
    { "MONODEBG.VXD", 0x4242, DeviceIo_MONODEBG },

    { NULL,       0,      NULL }
};

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
