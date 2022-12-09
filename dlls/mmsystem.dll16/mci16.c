/*
 * MMSYSTEM functions
 *
 * Copyright 1993      Martin Ayotte
 *           1998-2003,2009 Eric Pouech
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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winternl.h"
#include "wownt32.h"
#include "winnls.h"

#include "wine/winuser16.h"
#include "winemm16.h"
#include "digitalv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmsys);

/**************************************************************************
 * 			MCI_MessageToString			[internal]
 */
static const char* MCI_MessageToString(UINT wMsg)
{
    static char buffer[100];

#define CASE(s) case (s): return #s

    switch (wMsg) {
        CASE(DRV_LOAD);
        CASE(DRV_ENABLE);
        CASE(DRV_OPEN);
        CASE(DRV_CLOSE);
        CASE(DRV_DISABLE);
        CASE(DRV_FREE);
        CASE(DRV_CONFIGURE);
        CASE(DRV_QUERYCONFIGURE);
        CASE(DRV_INSTALL);
        CASE(DRV_REMOVE);
        CASE(DRV_EXITSESSION);
        CASE(DRV_EXITAPPLICATION);
        CASE(DRV_POWER);
	CASE(MCI_BREAK);
	CASE(MCI_CLOSE);
	CASE(MCI_CLOSE_DRIVER);
	CASE(MCI_COPY);
	CASE(MCI_CUE);
	CASE(MCI_CUT);
	CASE(MCI_DELETE);
	CASE(MCI_ESCAPE);
	CASE(MCI_FREEZE);
	CASE(MCI_PAUSE);
	CASE(MCI_PLAY);
	CASE(MCI_GETDEVCAPS);
	CASE(MCI_INFO);
	CASE(MCI_LOAD);
	CASE(MCI_OPEN);
	CASE(MCI_OPEN_DRIVER);
	CASE(MCI_PASTE);
	CASE(MCI_PUT);
	CASE(MCI_REALIZE);
	CASE(MCI_RECORD);
	CASE(MCI_RESUME);
	CASE(MCI_SAVE);
	CASE(MCI_SEEK);
	CASE(MCI_SET);
	CASE(MCI_SOUND);
	CASE(MCI_SPIN);
	CASE(MCI_STATUS);
	CASE(MCI_STEP);
	CASE(MCI_STOP);
	CASE(MCI_SYSINFO);
	CASE(MCI_UNFREEZE);
	CASE(MCI_UPDATE);
	CASE(MCI_WHERE);
	CASE(MCI_WINDOW);
	/* constants for digital video */
	CASE(MCI_CAPTURE);
	CASE(MCI_MONITOR);
	CASE(MCI_RESERVE);
	CASE(MCI_SETAUDIO);
	CASE(MCI_SIGNAL);
	CASE(MCI_SETVIDEO);
	CASE(MCI_QUALITY);
	CASE(MCI_LIST);
	CASE(MCI_UNDO);
	CASE(MCI_CONFIGURE);
	CASE(MCI_RESTORE);
#undef CASE
    default:
	sprintf(buffer, "MCI_<<%04X>>", wMsg);
	return buffer;
    }
}

static LPWSTR MCI_strdupAtoW( LPCSTR str )
{
    LPWSTR ret;
    INT len;

    if (!str) return NULL;
    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (ret) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    return ret;
}

/**************************************************************************
 * 			MCI_MapMsg16To32W			[internal]
 */
static MMSYSTEM_MapType	MCI_MapMsg16To32W(WORD wMsg, DWORD dwFlags, DWORD_PTR* lParam)
{
    if (*lParam == 0)
	return MMSYSTEM_MAP_OK;
    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE, MCI_SETTUNER
     */
    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CONFIGURE:
    case MCI_COPY:
    case MCI_CUE:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_GETDEVCAPS:
	/* case MCI_INDEX: */
    case MCI_LIST:
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_PASTE:
    case MCI_PAUSE:
    case MCI_PLAY:
    case MCI_REALIZE:
    case MCI_RECORD:
    case MCI_RESUME:
    case MCI_SEEK:
    case MCI_SET:
    case MCI_SETAUDIO:
    case MCI_SETVIDEO:
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
    case MCI_STEP:
    case MCI_STOP:
	/* case MCI_UNDO: */
    case MCI_UPDATE:
	*lParam = (DWORD)MapSL(*lParam);
	return MMSYSTEM_MAP_OK;
    case MCI_WHERE:
    case MCI_FREEZE:
    case MCI_UNFREEZE:
    case MCI_PUT:
        {
            LPMCI_DGV_RECT_PARMS mdrp32 = HeapAlloc(GetProcessHeap(), 0,
                sizeof(LPMCI_DGV_RECT_PARMS16) + sizeof(MCI_DGV_RECT_PARMS));
            LPMCI_DGV_RECT_PARMS16 mdrp16 = MapSL(*lParam);
            if (mdrp32) {
                *(LPMCI_DGV_RECT_PARMS16*)(mdrp32) = mdrp16;
                mdrp32 = (LPMCI_DGV_RECT_PARMS)((char*)mdrp32 + sizeof(LPMCI_DGV_RECT_PARMS16));
                mdrp32->dwCallback = mdrp16->dwCallback;
                mdrp32->rc.left = mdrp16->rc.left;
                mdrp32->rc.top = mdrp16->rc.top;
                mdrp32->rc.right = mdrp16->rc.right;
                mdrp32->rc.bottom = mdrp16->rc.bottom;
            } else {
                return MMSYSTEM_MAP_NOMEM;
            }
            *lParam = (DWORD)mdrp32;
        }
        return MMSYSTEM_MAP_OKMEM;
    case MCI_STATUS:
        {
            if (dwFlags & (MCI_DGV_STATUS_REFERENCE | MCI_DGV_STATUS_DISKSPACE)) {
                LPMCI_DGV_STATUS_PARMSW mdsp32w = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                    sizeof(LPMCI_DGV_STATUS_PARMS16) + sizeof(MCI_DGV_STATUS_PARMSW));
                LPMCI_DGV_STATUS_PARMS16 mdsp16 = MapSL(*lParam);
                if (mdsp32w) {
                    *(LPMCI_DGV_STATUS_PARMS16*)(mdsp32w) = mdsp16;
                    mdsp32w = (LPMCI_DGV_STATUS_PARMSW)((char*)mdsp32w + sizeof(LPMCI_DGV_STATUS_PARMS16));
                    mdsp32w->dwCallback = mdsp16->dwCallback;
                    mdsp32w->dwReturn = mdsp16->dwReturn;
                    mdsp32w->dwItem = mdsp16->dwItem;
                    mdsp32w->dwTrack = mdsp16->dwTrack;
                    if (dwFlags & MCI_DGV_STATUS_DISKSPACE)
                        mdsp32w->lpstrDrive = MCI_strdupAtoW(MapSL(mdsp16->lpstrDrive));
                    if (dwFlags & MCI_DGV_STATUS_REFERENCE)
                        mdsp32w->dwReference = mdsp16->dwReference;
                    *lParam = (DWORD)mdsp32w;
                } else {
                    return MMSYSTEM_MAP_NOMEM;
                }
            } else {
                *lParam = (DWORD)MapSL(*lParam);
                return MMSYSTEM_MAP_OK;
            }
        }
        return MMSYSTEM_MAP_OKMEM;
    case MCI_WINDOW:
        {
            LPMCI_OVLY_WINDOW_PARMSW mowp32w = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MCI_OVLY_WINDOW_PARMSW));
            LPMCI_OVLY_WINDOW_PARMS16 mowp16 = MapSL(*lParam);
            if (mowp32w) {
                mowp32w->dwCallback = mowp16->dwCallback;
                mowp32w->hWnd = HWND_32(mowp16->hWnd);
                mowp32w->nCmdShow = mowp16->nCmdShow;
                if (dwFlags & (MCI_DGV_WINDOW_TEXT | MCI_OVLY_WINDOW_TEXT))
                    mowp32w->lpstrText = MCI_strdupAtoW(MapSL(mowp16->lpstrText));
            } else {
                return MMSYSTEM_MAP_NOMEM;
            }
            *lParam = (DWORD)mowp32w;
        }
        return MMSYSTEM_MAP_OKMEM;
    case MCI_BREAK:
	{
            LPMCI_BREAK_PARMS		mbp32 = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_BREAK_PARMS));
	    LPMCI_BREAK_PARMS16		mbp16 = MapSL(*lParam);

	    if (mbp32) {
		mbp32->dwCallback = mbp16->dwCallback;
		mbp32->nVirtKey = mbp16->nVirtKey;
		mbp32->hwndBreak = HWND_32(mbp16->hwndBreak);
	    } else {
		return MMSYSTEM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mbp32;
	}
	return MMSYSTEM_MAP_OKMEM;
    case MCI_ESCAPE:
	{
            LPMCI_VD_ESCAPE_PARMSW	mvep32w = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_VD_ESCAPE_PARMSW));
	    LPMCI_VD_ESCAPE_PARMS16	mvep16  = MapSL(*lParam);

	    if (mvep32w) {
		mvep32w->dwCallback       = mvep16->dwCallback;
		mvep32w->lpstrCommand     = MCI_strdupAtoW(MapSL(mvep16->lpstrCommand));
	    } else {
		return MMSYSTEM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mvep32w;
	}
	return MMSYSTEM_MAP_OKMEM;
    case MCI_INFO:
	{
            LPMCI_DGV_INFO_PARMSW	mip32w = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMCI_DGV_INFO_PARMS16) + sizeof(MCI_DGV_INFO_PARMSW));
	    LPMCI_DGV_INFO_PARMS16	mip16  = MapSL(*lParam);

	    if (mip32w) {
		*(LPMCI_DGV_INFO_PARMS16*)(mip32w) = mip16;
		mip32w = (LPMCI_DGV_INFO_PARMSW)((char*)mip32w + sizeof(LPMCI_DGV_INFO_PARMS16));
		mip32w->dwCallback  = mip16->dwCallback;
		mip32w->lpstrReturn = HeapAlloc(GetProcessHeap(), 0, mip16->dwRetSize * sizeof(WCHAR));
		mip32w->dwRetSize   = mip16->dwRetSize;
		if (dwFlags & MCI_DGV_INFO_ITEM)
		    mip32w->dwItem  = mip16->dwItem;
	    } else {
		return MMSYSTEM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mip32w;
	}
	return MMSYSTEM_MAP_OKMEM;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	{
            LPMCI_OPEN_PARMSW	mop32w = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMCI_OPEN_PARMS16) + sizeof(MCI_ANIM_OPEN_PARMSW));
	    LPMCI_OPEN_PARMS16	mop16  = MapSL(*lParam);

	    if (mop32w) {
		*(LPMCI_OPEN_PARMS16*)(mop32w) = mop16;
		mop32w = (LPMCI_OPEN_PARMSW)((char*)mop32w + sizeof(LPMCI_OPEN_PARMS16));
		mop32w->dwCallback       = mop16->dwCallback;
		mop32w->wDeviceID        = mop16->wDeviceID;
                if( ( dwFlags & ( MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID)) == MCI_OPEN_TYPE)
                    mop32w->lpstrDeviceType  = MCI_strdupAtoW(MapSL(mop16->lpstrDeviceType));
                else
                    mop32w->lpstrDeviceType  = (LPWSTR) mop16->lpstrDeviceType;
                if( ( dwFlags & ( MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID)) == MCI_OPEN_ELEMENT)
                    mop32w->lpstrElementName = MCI_strdupAtoW(MapSL(mop16->lpstrElementName));
                else
                    mop32w->lpstrElementName = (LPWSTR) mop16->lpstrElementName;
                if( ( dwFlags &  MCI_OPEN_ALIAS))
                    mop32w->lpstrAlias = MCI_strdupAtoW(MapSL(mop16->lpstrAlias));
                else
                    mop32w->lpstrAlias = (LPWSTR) mop16->lpstrAlias;
		/* copy extended information if any...
		 * FIXME: this may seg fault if initial structure does not contain them and
		 * the reads after msip16 fail under LDT limits...
		 * NOTE: this should be split in two. First pass, while calling MCI_OPEN, and
		 * should not take care of extended parameters, and should be used by MCI_Open
		 * to fetch uDevType. When, this is known, the mapping for sending the
		 * MCI_OPEN_DRIVER shall be done depending on uDevType.
		 */
		if (HIWORD(dwFlags))
		    memcpy(mop32w + 1, mop16 + 1, sizeof(MCI_ANIM_OPEN_PARMS16) - sizeof(MCI_OPEN_PARMS16));
	    } else {
		return MMSYSTEM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mop32w;
	}
	return MMSYSTEM_MAP_OKMEM;
    case MCI_SYSINFO:
    {
        MCI_SYSINFO_PARMSW *origmsip32w;
        MCI_SYSINFO_PARMSW *msip32w = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_OPEN_PARMS16 *) + sizeof(MCI_SYSINFO_PARMSW));
        MCI_SYSINFO_PARMS16 *msip16 = MapSL(*lParam);

        if (!msip32w)
            return MMSYSTEM_MAP_NOMEM;

        origmsip32w = msip32w;
        *(MCI_SYSINFO_PARMS16 **)msip32w = msip16;
        msip32w = (MCI_SYSINFO_PARMSW *)((char *)msip32w + sizeof(MCI_OPEN_PARMS16 *));
        msip32w->dwCallback       = msip16->dwCallback;
        msip32w->lpstrReturn      = HeapAlloc(GetProcessHeap(), 0, (dwFlags & MCI_SYSINFO_QUANTITY) ?
                                                                    sizeof(DWORD) :
                                                                    msip16->dwRetSize * sizeof(WCHAR));
        if (!msip32w->lpstrReturn)
        {
            HeapFree(GetProcessHeap(), 0, origmsip32w);
            return MMSYSTEM_MAP_NOMEM;
        }
        msip32w->dwRetSize        = (dwFlags & MCI_SYSINFO_QUANTITY) ? sizeof(DWORD) : msip16->dwRetSize;
        msip32w->dwNumber         = msip16->dwNumber;
        msip32w->wDeviceType      = msip16->wDeviceType;

        *lParam = (DWORD)msip32w;
    }
	return MMSYSTEM_MAP_OKMEM;
    case MCI_SOUND:
	{
            LPMCI_SOUND_PARMSW		mbp32 = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_SOUND_PARMSW));
	    LPMCI_SOUND_PARMS16		mbp16 = MapSL(*lParam);

	    if (mbp32) {
		mbp32->dwCallback = mbp16->dwCallback;
		mbp32->lpstrSoundName = MCI_strdupAtoW(MapSL(mbp16->lpstrSoundName));
	    } else {
		return MMSYSTEM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mbp32;
	}
	return MMSYSTEM_MAP_OKMEM;
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
	FIXME("This is a hack\n");
	return MMSYSTEM_MAP_OK;
    default:
	FIXME("Don't know how to map msg=%s\n", MCI_MessageToString(wMsg));
    }
    return MMSYSTEM_MAP_MSGERROR;
}

/**************************************************************************
 * 			MCI_UnMapMsg16To32W			[internal]
 */
static  void	MCI_UnMapMsg16To32W(WORD wMsg, DWORD dwFlags, DWORD_PTR lParam, DWORD result)
{
    switch (wMsg) {
    case MCI_WHERE:
    case MCI_FREEZE:
    case MCI_UNFREEZE:
    case MCI_PUT:
        if (lParam) {
            LPMCI_DGV_RECT_PARMS mdrp32 = (LPMCI_DGV_RECT_PARMS)lParam;
            char *base = (char*)lParam - sizeof(LPMCI_DGV_RECT_PARMS16);
            LPMCI_DGV_RECT_PARMS16 mdrp16 = *(LPMCI_DGV_RECT_PARMS16*)base;
            mdrp16->rc.left = mdrp32->rc.left;
            mdrp16->rc.top = mdrp32->rc.top;
            mdrp16->rc.right = mdrp32->rc.right;
            mdrp16->rc.bottom = mdrp32->rc.bottom;
            HeapFree(GetProcessHeap(), 0, base);
        }
        break;
    case MCI_STATUS:
        if (lParam) {
            LPMCI_DGV_STATUS_PARMSW mdsp32w = (LPMCI_DGV_STATUS_PARMSW)lParam;
            char *base = (char*)lParam - sizeof(LPMCI_DGV_STATUS_PARMS16);
            LPMCI_DGV_STATUS_PARMS16 mdsp16 = *(LPMCI_DGV_STATUS_PARMS16*)base;
            mdsp16->dwReturn = mdsp32w->dwReturn;
            HeapFree(GetProcessHeap(), 0, (LPVOID)mdsp32w->lpstrDrive);
            HeapFree(GetProcessHeap(), 0, base);
        }
        break;
    case MCI_WINDOW:
        if (lParam) {
            LPMCI_OVLY_WINDOW_PARMSW mowp32w = (LPMCI_OVLY_WINDOW_PARMSW)lParam;
            HeapFree(GetProcessHeap(), 0, (LPVOID)mowp32w->lpstrText);
            HeapFree(GetProcessHeap(), 0, mowp32w);
        }
        break;
    case MCI_BREAK:
	HeapFree(GetProcessHeap(), 0, (LPVOID)lParam);
        break;
    case MCI_ESCAPE:
        if (lParam) {
            LPMCI_VD_ESCAPE_PARMSW	mvep32W = (LPMCI_VD_ESCAPE_PARMSW)lParam;
            HeapFree(GetProcessHeap(), 0, (LPVOID)mvep32W->lpstrCommand);
            HeapFree(GetProcessHeap(), 0, (LPVOID)lParam);
        }
        break;
    case MCI_INFO:
        if (lParam) {
            LPMCI_INFO_PARMSW	        mip32w = (LPMCI_INFO_PARMSW)lParam;
            char                       *base   = (char*)lParam - sizeof(LPMCI_INFO_PARMS16);
	    LPMCI_INFO_PARMS16          mip16  = *(LPMCI_INFO_PARMS16*)base;

            if (result == MMSYSERR_NOERROR)
                WideCharToMultiByte(CP_ACP, 0,
                                    mip32w->lpstrReturn, mip32w->dwRetSize,
                                    MapSL(mip16->lpstrReturn), mip16->dwRetSize,
                                    NULL, NULL);
            mip16->dwRetSize = mip32w->dwRetSize; /* never update prior to NT? */
            HeapFree(GetProcessHeap(), 0, mip32w->lpstrReturn);
            HeapFree(GetProcessHeap(), 0, base);
        }
        break;
    case MCI_SYSINFO:
        if (lParam) {
            MCI_SYSINFO_PARMSW *msip32w = (MCI_SYSINFO_PARMSW *)lParam;
            char               *base    = (char*)lParam - sizeof(MCI_SYSINFO_PARMS16 *);
            MCI_SYSINFO_PARMS16 *msip16  = *(MCI_SYSINFO_PARMS16 **)base;

            if (dwFlags & MCI_SYSINFO_QUANTITY) {
                DWORD *quantity = MapSL(msip16->lpstrReturn);

                *quantity = *(DWORD *)msip32w->lpstrReturn;
            }
            else if (result == MMSYSERR_NOERROR) {
                WideCharToMultiByte(CP_ACP, 0,
                                    msip32w->lpstrReturn, msip32w->dwRetSize,
                                    MapSL(msip16->lpstrReturn), msip16->dwRetSize,
                                    NULL, NULL);
            }

            HeapFree(GetProcessHeap(), 0, msip32w->lpstrReturn);
            HeapFree(GetProcessHeap(), 0, base);
        }
        break;
    case MCI_SOUND:
        if (lParam) {
            LPMCI_SOUND_PARMSW          msp32W = (LPMCI_SOUND_PARMSW)lParam;
            HeapFree(GetProcessHeap(), 0, (LPVOID)msp32W->lpstrSoundName);
            HeapFree(GetProcessHeap(), 0, (LPVOID)lParam);
        }
        break;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	if (lParam) {
            LPMCI_OPEN_PARMSW	mop32w = (LPMCI_OPEN_PARMSW)lParam;
            char               *base   = (char*)lParam - sizeof(LPMCI_OPEN_PARMS16);
	    LPMCI_OPEN_PARMS16	mop16  = *(LPMCI_OPEN_PARMS16*)base;

	    mop16->wDeviceID = mop32w->wDeviceID;
            if( ( dwFlags & ( MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID)) == MCI_OPEN_TYPE)
                HeapFree(GetProcessHeap(), 0, (LPWSTR)mop32w->lpstrDeviceType);
            if( ( dwFlags & ( MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID)) == MCI_OPEN_ELEMENT)
                HeapFree(GetProcessHeap(), 0, (LPWSTR)mop32w->lpstrElementName);
            if( ( dwFlags &  MCI_OPEN_ALIAS))
                HeapFree(GetProcessHeap(), 0, (LPWSTR)mop32w->lpstrAlias);
            HeapFree(GetProcessHeap(), 0, base);
	}
        break;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_MessageToString(wMsg));
    }
}

/* ###################################################
 * #                     MCI                         #
 * ###################################################
 */

#include <pshpack1.h>
#define MCI_MAX_THUNKS      32

static struct mci_thunk
{
    BYTE        popl_eax;       /* popl  %eax (return address) */
    BYTE        pushl_func;     /* pushl $pfn16 (16bit callback function) */
    YIELDPROC16 yield16;
    BYTE        pushl_eax;      /* pushl %eax */
    BYTE        jmp;            /* ljmp MCI_Yield1632 */
    DWORD       callback;
    MCIDEVICEID id;
} *MCI_Thunks;

#include <poppack.h>

static CRITICAL_SECTION mci_cs;
static CRITICAL_SECTION_DEBUG mci_critsect_debug =
{
    0, 0, &mci_cs,
    { &mci_critsect_debug.ProcessLocksList, &mci_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": mmsystem_mci_cs") }
};
static CRITICAL_SECTION mci_cs = { &mci_critsect_debug, -1, 0, 0, 0, 0 };

static UINT MCI_Yield1632(DWORD pfn16, MCIDEVICEID id, DWORD yield_data)
{
    WORD args[8];

    if (!pfn16)
    {
        MSG msg;
        PeekMessageW( &msg, 0, 0, 0, PM_REMOVE | PM_QS_SENDMESSAGE );
        return 0;
    }

    /* 16 bit func, call it */
    TRACE("Function (16 bit) !\n");

    args[2] = (MCIDEVICEID16)id;
    args[1] = HIWORD(yield_data);
    args[0] = LOWORD(yield_data);
    return WOWCallback16Ex(pfn16, WCB16_PASCAL, sizeof(args), args, NULL);
}

/******************************************************************
 *		MCI_AddThunk
 *
 */
static struct mci_thunk*       MCI_AddThunk(MCIDEVICEID id, YIELDPROC16 pfn16)
{
     struct mci_thunk* thunk;

     if (!MCI_Thunks)
     {
         MCI_Thunks = VirtualAlloc(NULL, MCI_MAX_THUNKS * sizeof(*MCI_Thunks), MEM_COMMIT,
                                   PAGE_EXECUTE_READWRITE);
         if (!MCI_Thunks) return NULL;
         for (thunk = MCI_Thunks; thunk < &MCI_Thunks[MCI_MAX_THUNKS]; thunk++)
         {
             thunk->popl_eax     = 0x58;   /* popl  %eax */
             thunk->pushl_func   = 0x68;   /* pushl $pfn16 */
             thunk->yield16      = 0;
             thunk->pushl_eax    = 0x50;   /* pushl %eax */
             thunk->jmp          = 0xe9;   /* jmp MCI_Yield1632 */
             thunk->callback     = (char *)MCI_Yield1632 - (char *)(&thunk->callback + 1);
             thunk->id           = 0;
         }
     }
     for (thunk = MCI_Thunks; thunk < &MCI_Thunks[MCI_MAX_THUNKS]; thunk++)
     {
         if (thunk->yield16 == 0)
         {
             thunk->yield16 = pfn16;
             thunk->id      = id;
             return thunk;
         }
     }
     FIXME("Out of mci-thunks. Bump MCI_MAX_THUNKS\n");
     return NULL;
}

/******************************************************************
 *		MCI_HasThunk
 *
 */
static struct mci_thunk*    MCI_HasThunk(YIELDPROC pfn)
{
    struct mci_thunk* thunk;

    if (!MCI_Thunks) return NULL;
    for (thunk = MCI_Thunks; thunk < &MCI_Thunks[MCI_MAX_THUNKS]; thunk++)
    {
        if ((YIELDPROC)thunk == pfn) return thunk;
    }
    return NULL;
}

/**************************************************************************
 * 				mciSetYieldProc			[MMSYSTEM.714]
 */
BOOL16 WINAPI mciSetYieldProc16(UINT16 uDeviceID, YIELDPROC16 fpYieldProc, DWORD dwYieldData)
{
    struct mci_thunk*   thunk;
    BOOL                ret;

    TRACE("(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(thunk = MCI_AddThunk(uDeviceID, fpYieldProc)))
        return FALSE;
    ret = mciSetYieldProc(uDeviceID, (YIELDPROC)thunk, dwYieldData);
    if (!ret) thunk->yield16 = NULL;
    return ret;
}

/**************************************************************************
 * 				mciGetYieldProc			[MMSYSTEM.716]
 */
YIELDPROC16 WINAPI mciGetYieldProc16(UINT16 uDeviceID, DWORD* lpdwYieldData)
{
    YIELDPROC           yield;
    DWORD               data;
    struct mci_thunk*   thunk;

    TRACE("(%u, %p)\n", uDeviceID, lpdwYieldData);

    yield = mciGetYieldProc(uDeviceID, &data);
    if (!yield || !(thunk = MCI_HasThunk(yield))) return NULL;

    if (lpdwYieldData) *lpdwYieldData = data;
    return thunk->yield16;
}

/**************************************************************************
 * 				mciGetErrorString		[MMSYSTEM.706]
 */
BOOL16 WINAPI mciGetErrorString16(DWORD wError, LPSTR lpstrBuffer, UINT16 uLength)
{
    return mciGetErrorStringA(wError, lpstrBuffer, uLength);
}

/**************************************************************************
 * 				mciDriverNotify			[MMSYSTEM.711]
 */
BOOL16 WINAPI mciDriverNotify16(HWND16 hWndCallBack, UINT16 wDevID, UINT16 wStatus)
{
    TRACE("(%04X, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    return PostMessageA(HWND_32(hWndCallBack), MM_MCINOTIFY, wStatus, wDevID);
}

/**************************************************************************
 * 			mciGetDriverData			[MMSYSTEM.708]
 */
DWORD WINAPI mciGetDriverData16(UINT16 uDeviceID)
{
    return mciGetDriverData(uDeviceID);
}

/**************************************************************************
 * 			mciSetDriverData			[MMSYSTEM.707]
 */
BOOL16 WINAPI mciSetDriverData16(UINT16 uDeviceID, DWORD data)
{
    return mciSetDriverData(uDeviceID, data);
}

/**************************************************************************
 * 				mciSendCommand			[MMSYSTEM.701]
 */
DWORD WINAPI mciSendCommand16(UINT16 wDevID, UINT16 wMsg, DWORD dwParam1, DWORD p2)
{
    DWORD		dwRet;
    BOOL                to32;
    DWORD_PTR           dwParam2 = p2;

    TRACE("(%04X, %s, %08lx, %08Ix)\n", wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    switch (wMsg) {
    case MCI_CLOSE:
    case MCI_OPEN:
    case MCI_SYSINFO:
    case MCI_BREAK:
    case MCI_SOUND:
        to32 = TRUE;
	break;
    default:
        /* FIXME: this is suboptimal. If MCI driver is a 16bit one, we'll be
         * doing 16=>32W, then 32W=>16 conversions.
         * We could directly call the 16bit driver if we had the information.
         */
        to32 = TRUE;
    }
    if (to32) {
        MMSYSTEM_MapType res;

	dwRet = MCIERR_INVALID_DEVICE_ID;

        switch (res = MCI_MapMsg16To32W(wMsg, dwParam1, &dwParam2)) {
        case MMSYSTEM_MAP_MSGERROR:
            dwRet = MCIERR_DRIVER_INTERNAL;
            break;
        case MMSYSTEM_MAP_NOMEM:
            TRACE("Problem mapping %s from 16 to 32\n", MCI_MessageToString(wMsg));
            dwRet = MCIERR_OUT_OF_MEMORY;
            break;
        case MMSYSTEM_MAP_OK:
        case MMSYSTEM_MAP_OKMEM:
            dwRet = mciSendCommandW(wDevID, wMsg, dwParam1, dwParam2);
            if (res == MMSYSTEM_MAP_OKMEM)
                MCI_UnMapMsg16To32W(wMsg, dwParam1, dwParam2, dwRet);
            break;
        }
    }
    else
    {
#if 0
	if (wDevID == MCI_ALL_DEVICE_ID) {
	    FIXME("unhandled MCI_ALL_DEVICE_ID\n");
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else {
            dwRet = SendDriverMessage(hdrv, wMsg, dwParam1, dwParam2);
	}
#endif
    }
    if (wMsg == MCI_CLOSE && dwRet == 0 && MCI_Thunks)
    {
        /* free yield thunks, if any */
        unsigned    i;
        for (i = 0; i < MCI_MAX_THUNKS; i++)
        {
            if (MCI_Thunks[i].id == wDevID)
                MCI_Thunks[i].yield16 = NULL;
        }
    }
    return dwRet;
}

/**************************************************************************
 * 				mciGetDeviceID		       	[MMSYSTEM.703]
 */
UINT16 WINAPI mciGetDeviceID16(LPCSTR lpstrName)
{
    TRACE("(\"%s\")\n", lpstrName);

    return mciGetDeviceIDA(lpstrName);
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
 */
UINT16 WINAPI mciGetDeviceIDFromElementID16(DWORD dwElementID, LPCSTR lpstrType)
{
    return mciGetDeviceIDFromElementIDA(dwElementID, lpstrType);
}

/**************************************************************************
 * 				mciGetCreatorTask		[MMSYSTEM.717]
 */
HTASK16 WINAPI mciGetCreatorTask16(UINT16 uDeviceID)
{
    return HTASK_16(mciGetCreatorTask(uDeviceID));
}

/**************************************************************************
 * 				mciDriverYield			[MMSYSTEM.710]
 */
UINT16 WINAPI mciDriverYield16(UINT16 uDeviceID)
{
    return mciDriverYield(uDeviceID);
}

/**************************************************************************
 * 				mciSendString			[MMSYSTEM.702]
 */
DWORD WINAPI mciSendString16(LPCSTR lpstrCommand, LPSTR lpstrRet,
			     UINT16 uRetLen, HWND16 hwndCallback)
{
    return mciSendStringA(lpstrCommand, lpstrRet, uRetLen, HWND_32(hwndCallback));
}

/**************************************************************************
 *                    	mciLoadCommandResource			[MMSYSTEM.705]
 */
UINT16 WINAPI mciLoadCommandResource16(HINSTANCE16 hInst, LPCSTR resname, UINT16 type)
{
    TRACE("(%04x, %s, %x)!\n", hInst, resname, type);
    return MCI_NO_COMMAND_TABLE;
}

/**************************************************************************
 *                    	mciFreeCommandResource			[MMSYSTEM.713]
 */
BOOL16 WINAPI mciFreeCommandResource16(UINT16 uTable)
{
    TRACE("(%04x)!\n", uTable);

    return FALSE;
}

/**************************************************************************
 * 				mciExecute			[MMSYSTEM.712]
 */
BOOL16 WINAPI mciExecute16(LPCSTR lpstrCommand)
{
    return mciExecute(lpstrCommand);
}
