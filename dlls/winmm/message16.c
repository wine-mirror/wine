/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM MCI and low level mapping functions
 *
 * Copyright 1999 Eric Pouech
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

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "wine/winbase16.h"
#include "winreg.h"
#include "winver.h"
#include "winemm.h"
#include "digitalv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

/**************************************************************************
 * 			MCI_MapMsg16To32A			[internal]
 */
WINMM_MapType	MCI_MapMsg16To32A(WORD uDevType, WORD wMsg, DWORD* lParam)
{
    if (*lParam == 0)
	return WINMM_MAP_OK;
    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LIST, MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE
     * MCI_SETAUDIO, MCI_SETTUNER, MCI_SETVIDEO
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
    case MCI_FREEZE:
    case MCI_GETDEVCAPS:
	/* case MCI_INDEX: */
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_PASTE:
    case MCI_PAUSE:
    case MCI_PLAY:
    case MCI_PUT:
    case MCI_REALIZE:
    case MCI_RECORD:
    case MCI_RESUME:
    case MCI_SEEK:
    case MCI_SET:
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
    case MCI_STATUS:		/* FIXME: is wrong for digital video */
    case MCI_STEP:
    case MCI_STOP:
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
	*lParam = (DWORD)MapSL(*lParam);
	return WINMM_MAP_OK;
    case MCI_WINDOW:
	/* in fact, I would also need the dwFlags... to see
	 * which members of lParam are effectively used
	 */
	*lParam = (DWORD)MapSL(*lParam);
	FIXME("Current mapping may be wrong\n");
	break;
    case MCI_BREAK:
	{
            LPMCI_BREAK_PARMS		mbp32 = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_BREAK_PARMS));
	    LPMCI_BREAK_PARMS16		mbp16 = MapSL(*lParam);

	    if (mbp32) {
		mbp32->dwCallback = mbp16->dwCallback;
		mbp32->nVirtKey = mbp16->nVirtKey;
		mbp32->hwndBreak = HWND_32(mbp16->hwndBreak);
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mbp32;
	}
	return WINMM_MAP_OKMEM;
    case MCI_ESCAPE:
	{
            LPMCI_VD_ESCAPE_PARMSA	mvep32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_VD_ESCAPE_PARMSA));
	    LPMCI_VD_ESCAPE_PARMS16	mvep16  = MapSL(*lParam);

	    if (mvep32a) {
		mvep32a->dwCallback       = mvep16->dwCallback;
		mvep32a->lpstrCommand     = MapSL(mvep16->lpstrCommand);
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mvep32a;
	}
	return WINMM_MAP_OKMEM;
    case MCI_INFO:
	{
            LPMCI_INFO_PARMSA	mip32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_INFO_PARMSA));
	    LPMCI_INFO_PARMS16	mip16  = MapSL(*lParam);

	    /* FIXME this is wrong if device is of type
	     * MCI_DEVTYPE_DIGITAL_VIDEO, some members are not mapped
	     */
	    if (mip32a) {
		mip32a->dwCallback  = mip16->dwCallback;
		mip32a->lpstrReturn = MapSL(mip16->lpstrReturn);
		mip32a->dwRetSize   = mip16->dwRetSize;
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mip32a;
	}
	return WINMM_MAP_OKMEM;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	{
            LPMCI_OPEN_PARMSA	mop32a = HeapAlloc(GetProcessHeap(), 0, sizeof(LPMCI_OPEN_PARMS16) + sizeof(MCI_OPEN_PARMSA) + 2 * sizeof(DWORD));
	    LPMCI_OPEN_PARMS16	mop16  = MapSL(*lParam);

	    if (mop32a) {
		*(LPMCI_OPEN_PARMS16*)(mop32a) = mop16;
		mop32a = (LPMCI_OPEN_PARMSA)((char*)mop32a + sizeof(LPMCI_OPEN_PARMS16));
		mop32a->dwCallback       = mop16->dwCallback;
		mop32a->wDeviceID        = mop16->wDeviceID;
		mop32a->lpstrDeviceType  = MapSL(mop16->lpstrDeviceType);
		mop32a->lpstrElementName = MapSL(mop16->lpstrElementName);
		mop32a->lpstrAlias       = MapSL(mop16->lpstrAlias);
		/* copy extended information if any...
		 * FIXME: this may seg fault if initial structure does not contain them and
		 * the reads after msip16 fail under LDT limits...
		 * NOTE: this should be split in two. First pass, while calling MCI_OPEN, and
		 * should not take care of extended parameters, and should be used by MCI_Open
		 * to fetch uDevType. When, this is known, the mapping for sending the
		 * MCI_OPEN_DRIVER shall be done depending on uDevType.
		 */
		memcpy(mop32a + 1, mop16 + 1, 2 * sizeof(DWORD));
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mop32a;
	}
	return WINMM_MAP_OKMEM;
    case MCI_SYSINFO:
	{
            LPMCI_SYSINFO_PARMSA	msip32a = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_SYSINFO_PARMSA));
	    LPMCI_SYSINFO_PARMS16	msip16  = MapSL(*lParam);

	    if (msip32a) {
		msip32a->dwCallback       = msip16->dwCallback;
		msip32a->lpstrReturn      = MapSL(msip16->lpstrReturn);
		msip32a->dwRetSize        = msip16->dwRetSize;
		msip32a->dwNumber         = msip16->dwNumber;
		msip32a->wDeviceType      = msip16->wDeviceType;
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (DWORD)msip32a;
	}
	return WINMM_MAP_OKMEM;
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
	return WINMM_MAP_OK;

    default:
	WARN("Don't know how to map msg=%s\n", MCI_MessageToString(wMsg));
    }
    return WINMM_MAP_MSGERROR;
}

/**************************************************************************
 * 			MCI_UnMapMsg16To32A			[internal]
 */
WINMM_MapType	MCI_UnMapMsg16To32A(WORD uDevType, WORD wMsg, DWORD lParam)
{
    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CONFIGURE:
    case MCI_COPY:
    case MCI_CUE:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_FREEZE:
    case MCI_GETDEVCAPS:
	/* case MCI_INDEX: */
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_PASTE:
    case MCI_PAUSE:
    case MCI_PLAY:
    case MCI_PUT:
    case MCI_REALIZE:
    case MCI_RECORD:
    case MCI_RESUME:
    case MCI_SEEK:
    case MCI_SET:
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
    case MCI_STATUS:
    case MCI_STEP:
    case MCI_STOP:
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
	return WINMM_MAP_OK;

    case MCI_WINDOW:
	/* FIXME ?? see Map function */
	return WINMM_MAP_OK;

    case MCI_BREAK:
    case MCI_ESCAPE:
    case MCI_INFO:
    case MCI_SYSINFO:
	HeapFree(GetProcessHeap(), 0, (LPVOID)lParam);
	return WINMM_MAP_OK;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	if (lParam) {
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)lParam;
	    LPMCI_OPEN_PARMS16	mop16  = *(LPMCI_OPEN_PARMS16*)((char*)mop32a - sizeof(LPMCI_OPEN_PARMS16));

	    mop16->wDeviceID = mop32a->wDeviceID;
	    if (!HeapFree(GetProcessHeap(), 0, (LPVOID)(lParam - sizeof(LPMCI_OPEN_PARMS16))))
		FIXME("bad free line=%d\n", __LINE__);
	}
	return WINMM_MAP_OK;
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
	return WINMM_MAP_OK;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_MessageToString(wMsg));
    }
    return WINMM_MAP_MSGERROR;
}

/*
 * 0000 stop
 * 0001 squeeze   signed 4 bytes to 2 bytes     *( LPINT16)D = ( INT16)*( LPINT16)S; D += 2;     S += 4
 * 0010 squeeze unsigned 4 bytes to 2 bytes     *(LPUINT16)D = (UINT16)*(LPUINT16)S; D += 2;     S += 4
 * 0100
 * 0101
 * 0110 zero 4 bytes                            *(DWORD)D = 0                        D += 4;     S += 4
 * 0111 copy string                             *(LPSTR*)D = seg dup(*(LPSTR*)S)     D += 4;     S += 4
 * 1xxx copy xxx + 1 bytes                      memcpy(D, S, xxx + 1);               D += xxx+1; S += xxx+1
 */

/**************************************************************************
 * 			MCI_MsgMapper32To16_Create		[internal]
 *
 * Helper for MCI_MapMsg32ATo16.
 * Maps the 32 bit pointer (*ptr), of size bytes, to an allocated 16 bit
 * segmented pointer.
 * map contains a list of action to be performed for the mapping (see list
 * above)
 * if keep is TRUE, keeps track of in 32 bit ptr in allocated 16 bit area.
 */
static	WINMM_MapType	MCI_MsgMapper32To16_Create(void** ptr, int size16, DWORD map, BOOLEAN keep)
{
    void*	lp = HeapAlloc( GetProcessHeap(), 0, (keep ? sizeof(void**) : 0) + size16 );
    LPBYTE	p16, p32;

    if (!lp) {
	return WINMM_MAP_NOMEM;
    }
    p32 = (LPBYTE)(*ptr);
    if (keep) {
	*(void**)lp = *ptr;
	p16 = (LPBYTE)lp + sizeof(void**);
	*ptr = (char*)MapLS(lp) + sizeof(void**);
    } else {
	p16 = lp;
	*ptr = (void*)MapLS(lp);
    }

    if (map == 0) {
	memcpy(p16, p32, size16);
    } else {
	unsigned	nibble;
	unsigned	sz;

	while (map & 0xF) {
	    nibble = map & 0xF;
	    if (nibble & 0x8) {
		sz = (nibble & 7) + 1;
		memcpy(p16, p32, sz);
		p16 += sz;
		p32 += sz;
		size16 -= sz;	/* DEBUG only */
	    } else {
		switch (nibble) {
		case 0x1:
                    *(LPINT16)p16 = *(LPINT)p32;
                    p16 += sizeof(INT16);
                    p32 += sizeof(INT);
                    size16 -= sizeof(INT16);
                    break;
		case 0x2:
                    *(LPUINT16)p16 = *(LPUINT)p32;
                    p16 += sizeof(UINT16);
                    p32 += sizeof(UINT);
                    size16 -= sizeof(UINT16);
                    break;
		case 0x6:
                    *(LPDWORD)p16 = 0;
                    p16 += sizeof(DWORD);
                    p32 += sizeof(DWORD);
                    size16 -= sizeof(DWORD);
                    break;
		case 0x7:
                    *(SEGPTR *)p16 = MapLS( *(LPSTR *)p32 );
                    p16 += sizeof(SEGPTR);
                    p32 += sizeof(LPSTR);
                    size16 -= sizeof(SEGPTR);
                    break;
		default:
                    FIXME("Unknown nibble for mapping (%x)\n", nibble);
		}
	    }
	    map >>= 4;
	}
	if (size16 != 0) /* DEBUG only */
	    FIXME("Mismatch between 16 bit struct size and map nibbles serie\n");
    }
    return WINMM_MAP_OKMEM;
}

/**************************************************************************
 * 			MCI_MsgMapper32To16_Destroy		[internal]
 *
 * Helper for MCI_UnMapMsg32ATo16.
 */
static	WINMM_MapType	MCI_MsgMapper32To16_Destroy(void* ptr, int size16, DWORD map, BOOLEAN kept)
{
    if (ptr) {
	void*		msg16 = MapSL((SEGPTR)ptr);
	void*		alloc;
	LPBYTE		p32, p16;
	unsigned	nibble;

        UnMapLS( (SEGPTR)ptr );
	if (kept) {
	    alloc = (char*)msg16 - sizeof(void**);
	    p32 = *(void**)alloc;
	    p16 = msg16;

	    if (map == 0) {
		memcpy(p32, p16, size16);
	    } else {
		while (map & 0xF) {
		    nibble = map & 0xF;
		    if (nibble & 0x8) {
			memcpy(p32, p16, (nibble & 7) + 1);
			p16 += (nibble & 7) + 1;
			p32 += (nibble & 7) + 1;
			size16 -= (nibble & 7) + 1;
		    } else {
			switch (nibble) {
			case 0x1:
                            *(LPINT)p32 = *(LPINT16)p16;
                            p16 += sizeof(INT16);
                            p32 += sizeof(INT);
                            size16 -= sizeof(INT16);
                            break;
			case 0x2:
                            *(LPUINT)p32 = *(LPUINT16)p16;
                            p16 += sizeof(UINT16);
                            p32 += sizeof(UINT);
                            size16 -= sizeof(UINT16);
                            break;
			case 0x6:
                            p16 += sizeof(UINT);
                            p32 += sizeof(UINT);
                            size16 -= sizeof(UINT);
                            break;
			case 0x7:
                            UnMapLS( *(SEGPTR *)p16 );
                            p16 += sizeof(SEGPTR);
                            p32 += sizeof(char*);
                            size16 -= sizeof(SEGPTR);
                            break;
			default:
                            FIXME("Unknown nibble for mapping (%x)\n", nibble);
			}
		    }
		    map >>= 4;
		}
		if (size16 != 0) /* DEBUG only */
		    FIXME("Mismatch between 16 bit struct size and map nibbles serie\n");
	    }
	} else {
	    alloc = msg16;
	}

        HeapFree( GetProcessHeap(), 0, alloc );
    }
    return WINMM_MAP_OK;
}

/**************************************************************************
 * 			MCI_MapMsg32ATo16			[internal]
 *
 * Map a 32-A bit MCI message to a 16 bit MCI message.
 */
WINMM_MapType	MCI_MapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD dwFlags, DWORD* lParam)
{
    int		size;
    BOOLEAN     keep = FALSE;
    DWORD	map = 0;

    if (*lParam == 0)
	return WINMM_MAP_OK;

    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LIST, MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE
     * MCI_SETAUDIO, MCI_SETTUNER, MCI_SETVIDEO
     */
    switch (wMsg) {
    case MCI_BREAK:
	size = sizeof(MCI_BREAK_PARMS);
	break;
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CONFIGURE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
	/* case MCI_COPY: */
    case MCI_CUE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_CUE_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_CUE_PARMS);	break;*/	FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_CUT:*/
    case MCI_DELETE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_DELETE_PARMS16);	map = 0x0F1111FB;	break;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_DELETE_PARMS);	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_ESCAPE: */
    case MCI_FREEZE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_FREEZE_PARMS);	map = 0x0001111B; 	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_GETDEVCAPS:
	keep = TRUE;
	size = sizeof(MCI_GETDEVCAPS_PARMS);
	break;
	/* case MCI_INDEX: */
    case MCI_INFO:
	{
            LPMCI_INFO_PARMSA	mip32a = (LPMCI_INFO_PARMSA)(*lParam);
	    LPMCI_INFO_PARMS16	mip16;

	    switch (uDevType) {
	    case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_INFO_PARMS16);	break;
	    default:				size = sizeof(MCI_INFO_PARMS16);	break;
	    }
            mip16 = HeapAlloc( GetProcessHeap(), 0, size);
            if (mip16)
            {
		mip16->dwCallback  = mip32a->dwCallback;
		mip16->lpstrReturn = MapLS( mip32a->lpstrReturn );
		mip16->dwRetSize   = mip32a->dwRetSize;
		if (uDevType == MCI_DEVTYPE_DIGITAL_VIDEO) {
		    ((LPMCI_DGV_INFO_PARMS16)mip16)->dwItem = ((LPMCI_DGV_INFO_PARMSA)mip32a)->dwItem;
		}
	    } else {
		return WINMM_MAP_NOMEM;
	    }
            *lParam = MapLS(mip16);
	}
	return WINMM_MAP_OKMEM;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	{
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)(*lParam);
            char* ptr = HeapAlloc( GetProcessHeap(), 0,
                                   sizeof(LPMCI_OPEN_PARMSA) + sizeof(MCI_OPEN_PARMS16) + 2 * sizeof(DWORD));
	    LPMCI_OPEN_PARMS16	mop16;


	    if (ptr) {
		*(LPMCI_OPEN_PARMSA*)(ptr) = mop32a;
		mop16 = (LPMCI_OPEN_PARMS16)(ptr + sizeof(LPMCI_OPEN_PARMSA));
		mop16->dwCallback       = mop32a->dwCallback;
		mop16->wDeviceID        = mop32a->wDeviceID;
		if (dwFlags & MCI_OPEN_TYPE) {
		    if (dwFlags & MCI_OPEN_TYPE_ID) {
			/* dword "transparent" value */
			mop16->lpstrDeviceType = (SEGPTR)mop32a->lpstrDeviceType;
		    } else {
			/* string */
			mop16->lpstrDeviceType = MapLS( mop32a->lpstrDeviceType );
		    }
		} else {
		    /* nuthin' */
		    mop16->lpstrDeviceType = 0;
		}
		if (dwFlags & MCI_OPEN_ELEMENT) {
		    if (dwFlags & MCI_OPEN_ELEMENT_ID) {
			mop16->lpstrElementName = (SEGPTR)mop32a->lpstrElementName;
		    } else {
			mop16->lpstrElementName = MapLS( mop32a->lpstrElementName );
		    }
		} else {
		    mop16->lpstrElementName = 0;
		}
		if (dwFlags & MCI_OPEN_ALIAS) {
		    mop16->lpstrAlias = MapLS( mop32a->lpstrAlias );
		} else {
		    mop16->lpstrAlias = 0;
		}
		/* copy extended information if any...
		 * FIXME: this may seg fault if initial structure does not contain them and
		 * the reads after msip16 fail under LDT limits...
		 * NOTE: this should be split in two. First pass, while calling MCI_OPEN, and
		 * should not take care of extended parameters, and should be used by MCI_Open
		 * to fetch uDevType. When, this is known, the mapping for sending the
		 * MCI_OPEN_DRIVER shall be done depending on uDevType.
		 */
		memcpy(mop16 + 1, mop32a + 1, 2 * sizeof(DWORD));
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)MapLS(ptr) + sizeof(LPMCI_OPEN_PARMSA);
	}
	return WINMM_MAP_OKMEM;
	/* case MCI_PASTE:*/
    case MCI_PAUSE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_PLAY:
	size = sizeof(MCI_PLAY_PARMS);
	break;
    case MCI_PUT:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_REALIZE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_RECORD:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECORD_PARMS16);	map = 0x0F1111FB;	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_RECORD_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_RECORD_PARMS);	break;
	}
	break;
    case MCI_RESUME:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_SEEK:
	switch (uDevType) {
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SEEK_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_SEEK_PARMS);		break;
	}
	break;
    case MCI_SET:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_SET_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SET_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	case MCI_DEVTYPE_SEQUENCER:	size = sizeof(MCI_SEQ_SET_PARMS);	break;
        /* FIXME: normally the 16 and 32 bit structures are byte by byte aligned,
	 * so not doing anything should work...
	 */
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_SET_PARMS);	break;
	default:			size = sizeof(MCI_SET_PARMS);		break;
	}
	break;
    case MCI_SETAUDIO:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_SETAUDIO_PARMS16);map = 0x0000077FF;	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SETAUDIO_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
	size = sizeof(MCI_SET_PARMS);
	break;
    case MCI_STATUS:
	keep = TRUE;
	switch (uDevType) {
	/* FIXME:
	 * don't know if buffer for value is the one passed through lpstrDevice
	 * or is provided by MCI driver.
	 * Assuming solution 2: provided by MCI driver, so zeroing on entry
	 */
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STATUS_PARMS16);	map = 0x0B6FF;		break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STEP_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STEP_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	case MCI_DEVTYPE_VIDEODISC:	size = sizeof(MCI_VD_STEP_PARMS);	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_STOP:
	size = sizeof(MCI_SET_PARMS);
	break;
    case MCI_SYSINFO:
	{
            LPMCI_SYSINFO_PARMSA  msip32a = (LPMCI_SYSINFO_PARMSA)(*lParam);
            LPMCI_SYSINFO_PARMS16 msip16;
            char* ptr = HeapAlloc( GetProcessHeap(), 0,
                                   sizeof(LPMCI_SYSINFO_PARMSA) + sizeof(MCI_SYSINFO_PARMS16) );

	    if (ptr) {
		*(LPMCI_SYSINFO_PARMSA*)(ptr) = msip32a;
		msip16 = (LPMCI_SYSINFO_PARMS16)(ptr + sizeof(LPMCI_SYSINFO_PARMSA));

		msip16->dwCallback       = msip32a->dwCallback;
		msip16->lpstrReturn      = MapLS( msip32a->lpstrReturn );
		msip16->dwRetSize        = msip32a->dwRetSize;
		msip16->dwNumber         = msip32a->dwNumber;
		msip16->wDeviceType      = msip32a->wDeviceType;
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)MapLS(ptr) + sizeof(LPMCI_SYSINFO_PARMSA);
	}
	return WINMM_MAP_OKMEM;
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_UPDATE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_UPDATE_PARMS16);	map = 0x000B1111B;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_WHERE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	keep = TRUE;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	keep = TRUE;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_WINDOW:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_WINDOW_PARMS16);	if (dwFlags & MCI_DGV_WINDOW_TEXT)  map = 0x7FB;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_WINDOW_PARMS16);	if (dwFlags & MCI_OVLY_WINDOW_TEXT) map = 0x7FB;	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case DRV_OPEN:
	{
            LPMCI_OPEN_DRIVER_PARMSA  modp32a = (LPMCI_OPEN_DRIVER_PARMSA)(*lParam);
            LPMCI_OPEN_DRIVER_PARMS16 modp16;
            char *ptr = HeapAlloc( GetProcessHeap(), 0,
                                  sizeof(LPMCI_OPEN_DRIVER_PARMSA) + sizeof(MCI_OPEN_DRIVER_PARMS16));

	    if (ptr) {
		*(LPMCI_OPEN_DRIVER_PARMSA*)(ptr) = modp32a;
		modp16 = (LPMCI_OPEN_DRIVER_PARMS16)(ptr + sizeof(LPMCI_OPEN_DRIVER_PARMSA));
		modp16->wDeviceID = modp32a->wDeviceID;
		modp16->lpstrParams = MapLS( modp32a->lpstrParams );
		/* other fields are gonna be filled by the driver, don't copy them */
 	    } else {
		return WINMM_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)MapLS(ptr) + sizeof(LPMCI_OPEN_DRIVER_PARMSA);
	}
	return WINMM_MAP_OKMEM;
    case DRV_LOAD:
    case DRV_ENABLE:
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
	return WINMM_MAP_OK;

    default:
	WARN("Don't know how to map msg=%s\n", MCI_MessageToString(wMsg));
	return WINMM_MAP_MSGERROR;
    }
    return MCI_MsgMapper32To16_Create((void**)lParam, size, map, keep);
}

/**************************************************************************
 * 			MCI_UnMapMsg32ATo16			[internal]
 */
WINMM_MapType	MCI_UnMapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD dwFlags, DWORD lParam)
{
    int		size = 0;
    BOOLEAN     kept = FALSE;	/* there is no need to compute size when kept is FALSE */
    DWORD 	map = 0;

    switch (wMsg) {
    case MCI_BREAK:
        break;
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CONFIGURE:
	break;
	/* case MCI_COPY: */
    case MCI_CUE:
	break;
	/* case MCI_CUT: */
    case MCI_DELETE:
	break;
	/* case MCI_ESCAPE: */
    case MCI_FREEZE:
	break;
    case MCI_GETDEVCAPS:
	kept = TRUE;
	size = sizeof(MCI_GETDEVCAPS_PARMS);
	break;
	/* case MCI_INDEX: */
    case MCI_INFO:
	{
            LPMCI_INFO_PARMS16 mip16  = (LPMCI_INFO_PARMS16)MapSL(lParam);
            UnMapLS( lParam );
            UnMapLS( mip16->lpstrReturn );
            HeapFree( GetProcessHeap(), 0, mip16 );
	}
	return WINMM_MAP_OK;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:
	if (lParam) {
            LPMCI_OPEN_PARMS16	mop16  = (LPMCI_OPEN_PARMS16)MapSL(lParam);
	    LPMCI_OPEN_PARMSA	mop32a = *(LPMCI_OPEN_PARMSA*)((char*)mop16 - sizeof(LPMCI_OPEN_PARMSA));
            UnMapLS( lParam );
	    mop32a->wDeviceID = mop16->wDeviceID;
            if ((dwFlags & MCI_OPEN_TYPE) && !(dwFlags & MCI_OPEN_TYPE_ID))
                UnMapLS( mop16->lpstrDeviceType );
            if ((dwFlags & MCI_OPEN_ELEMENT) && !(dwFlags & MCI_OPEN_ELEMENT_ID))
                UnMapLS( mop16->lpstrElementName );
            if (dwFlags & MCI_OPEN_ALIAS)
                UnMapLS( mop16->lpstrAlias );
            HeapFree( GetProcessHeap(), 0, (char*)mop16 - sizeof(LPMCI_OPEN_PARMSA) );
	}
	return WINMM_MAP_OK;
	/* case MCI_PASTE:*/
    case MCI_PAUSE:
	break;
    case MCI_PLAY:
	break;
    case MCI_PUT:
	break;
    case MCI_REALIZE:
	break;
    case MCI_RECORD:
	break;
    case MCI_RESUME:
	break;
    case MCI_SEEK:
	break;
    case MCI_SET:
	break;
    case MCI_SETAUDIO:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	map = 0x0000077FF;	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SETAUDIO_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	}
	break;
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
	break;
    case MCI_STATUS:
	kept = TRUE;
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:
	if (lParam) {
            LPMCI_DGV_STATUS_PARMS16	mdsp16  = (LPMCI_DGV_STATUS_PARMS16)MapSL(lParam);
	    LPMCI_DGV_STATUS_PARMSA	mdsp32a = *(LPMCI_DGV_STATUS_PARMSA*)((char*)mdsp16 - sizeof(LPMCI_DGV_STATUS_PARMSA));

            UnMapLS( lParam );
	    if (mdsp16) {
		mdsp32a->dwReturn = mdsp16->dwReturn;
		if (dwFlags & MCI_DGV_STATUS_DISKSPACE) {
		    TRACE("MCI_STATUS (DGV) lpstrDrive=%08lx\n", mdsp16->lpstrDrive);
		    TRACE("MCI_STATUS (DGV) lpstrDrive=%s\n", (LPSTR)MapSL(mdsp16->lpstrDrive));
                    UnMapLS( mdsp16->lpstrDrive );
		}
                HeapFree( GetProcessHeap(), 0, (char*)mdsp16 - sizeof(LPMCI_DGV_STATUS_PARMSA) );
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	}
	return WINMM_MAP_OKMEM;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME("NIY vcr\n");	return WINMM_MAP_NOMEM;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	break;
    case MCI_STOP:
	break;
    case MCI_SYSINFO:
	if (lParam) {
            LPMCI_SYSINFO_PARMS16	msip16  = (LPMCI_SYSINFO_PARMS16)MapSL(lParam);
	    LPMCI_SYSINFO_PARMSA	msip32a = *(LPMCI_SYSINFO_PARMSA*)((char*)msip16 - sizeof(LPMCI_SYSINFO_PARMSA));

            UnMapLS( lParam );
	    if (msip16) {
                msip16->dwCallback = msip32a->dwCallback;
                UnMapLS( msip16->lpstrReturn );
                HeapFree( GetProcessHeap(), 0, (char*)msip16 - sizeof(LPMCI_SYSINFO_PARMSA) );
	    } else {
		return WINMM_MAP_NOMEM;
	    }
	}
	return WINMM_MAP_OKMEM;
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
	break;
    case MCI_UPDATE:
	break;
    case MCI_WHERE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS16);	map = 0x0001111B;	kept = TRUE;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS16);	map = 0x0001111B;	kept = TRUE;	break;
	default:			break;
	}
	break;
    case MCI_WINDOW:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_WINDOW_PARMS16);	if (dwFlags & MCI_DGV_WINDOW_TEXT)  map = 0x7666;	break;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_WINDOW_PARMS16);	if (dwFlags & MCI_OVLY_WINDOW_TEXT) map = 0x7666;	break;
	default:			break;
	}
	/* FIXME: see map function */
	break;

    case DRV_OPEN:
	if (lParam) {
            LPMCI_OPEN_DRIVER_PARMS16	modp16  = (LPMCI_OPEN_DRIVER_PARMS16)MapSL(lParam);
	    LPMCI_OPEN_DRIVER_PARMSA	modp32a = *(LPMCI_OPEN_DRIVER_PARMSA*)((char*)modp16 - sizeof(LPMCI_OPEN_DRIVER_PARMSA));

            UnMapLS( lParam );
	    modp32a->wCustomCommandTable = modp16->wCustomCommandTable;
	    modp32a->wType = modp16->wType;
            UnMapLS( modp16->lpstrParams );
            HeapFree( GetProcessHeap(), 0, (char *)modp16 - sizeof(LPMCI_OPEN_DRIVER_PARMSA) );
	}
	return WINMM_MAP_OK;
    case DRV_LOAD:
    case DRV_ENABLE:
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
	return WINMM_MAP_OK;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_MessageToString(wMsg));
	return WINMM_MAP_MSGERROR;
    }
    return MCI_MsgMapper32To16_Destroy((void*)lParam, size, map, kept);
}

