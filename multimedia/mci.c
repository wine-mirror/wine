/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MCI internal functions
 *
 * Copyright 1998/1999 Eric Pouech
 */

#include <string.h>
#include <stdlib.h>

#include "winbase.h"
#include "winuser.h"
#include "heap.h"
#include "driver.h"
#include "mmsystem.h"
#include "multimedia.h"
#include "selectors.h"
#include "digitalv.h"
#include "options.h"
#include "wine/winbase16.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mci)

WINE_MCIDRIVER mciDrv[MAXMCIDRIVERS];

int	mciInstalledCount;
int	mciInstalledListLen;
LPSTR	lpmciInstallNames = NULL;

static	struct MCI_StringType {
    LPCSTR	str;
    UINT	type;
} MCI_StringType_List[] = {
    /* MCI types that are working */
    {"CDAUDIO", MCI_DEVTYPE_CD_AUDIO},
    {"WAVEAUDIO", MCI_DEVTYPE_WAVEFORM_AUDIO},
    {"SEQUENCER", MCI_DEVTYPE_SEQUENCER},

    /* MCI types that should be working */
    {"ANIMATION1", MCI_DEVTYPE_ANIMATION},
    {"MPEGVIDEO", MCI_DEVTYPE_DIGITAL_VIDEO},
    {"AVIVIDEO", MCI_DEVTYPE_DIGITAL_VIDEO},

    /* MCI types not likely to be supported */
    {"VCR", MCI_DEVTYPE_VCR},
    {"VIDEODISC", MCI_DEVTYPE_VIDEODISC},
    {"OVERLAY", MCI_DEVTYPE_OVERLAY},
    {"DAT", MCI_DEVTYPE_DAT},
    {"SCANNER", MCI_DEVTYPE_SCANNER},

    {NULL, 0}
};

WORD	MCI_GetDevTypeFromString(LPCSTR str)
{
    struct MCI_StringType*	mst = MCI_StringType_List;

    while (mst->str && strcmp(mst->str, str)) mst++;
    return mst->type;
}

LPCSTR	MCI_GetStringFromDevType(WORD type)
{
    struct MCI_StringType*	mst = MCI_StringType_List;

    while (mst->str && mst->type != type) mst++;
    return mst->str;
}

/* The wDevID's returned by wine were originally in the range 
 * 0 - (MAXMCIDRIVERS - 1) and used directly as array indices.
 * Unfortunately, ms-windows uses wDevID of zero to indicate
 * errors.  Now, multimedia drivers must pass the wDevID through
 * MCI_DevIDToIndex to get an index in that range.  An
 * arbitrary value, MCI_MAGIC is added to the wDevID seen
 * by the windows programs.
 */

#define MCI_MAGIC 0x0F00

/**************************************************************************
 * 				MCI_DevIDToIndex		[internal]
 */
int MCI_DevIDToIndex(UINT16 wDevID) 
{
    return wDevID - MCI_MAGIC;
}

/**************************************************************************
 * 				MCI_FirstDevId			[internal]
 */
UINT16 MCI_FirstDevID(void)
{
    return MCI_MAGIC;
}

/**************************************************************************
 * 				MCI_NextDevId			[internal]
 */
UINT16 MCI_NextDevID(UINT16 wDevID) 
{
    return wDevID + 1;
}

/**************************************************************************
 * 				MCI_DevIDValid			[internal]
 */
BOOL MCI_DevIDValid(UINT16 wDevID) 
{
    return wDevID >= MCI_MAGIC && wDevID < (MCI_MAGIC + MAXMCIDRIVERS);
}

/**************************************************************************
 * 			MCI_CommandToString			[internal]
 */
const char* MCI_CommandToString(UINT16 wMsg)
{
    static char buffer[100];
    
#define CASE(s) case (s): return #s
    
    switch (wMsg) {
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
	CASE(MCI_SPIN);
	CASE(MCI_STATUS);
	CASE(MCI_STEP);
	CASE(MCI_STOP);
	CASE(MCI_SYSINFO);
	CASE(MCI_UNFREEZE);
	CASE(MCI_UPDATE);
	CASE(MCI_WHERE);
	CASE(MCI_WINDOW);
    default:
	sprintf(buffer, "MCI_<<%04X>>", wMsg);
	return buffer;
    }
#undef CASE
}

/**************************************************************************
 * 			MCI_MapMsg16To32A			[internal]
 */
MCI_MapType	MCI_MapMsg16To32A(WORD uDevType, WORD wMsg, DWORD* lParam)
{
    if (*lParam == 0)
	return MCI_MAP_OK;
    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LIST, MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE
     * MCI_SETAUDIO, MCI_SETTUNER, MCI_SETVIDEO
     */
    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:		
    case MCI_CLOSE_DRIVER:	
	/* case MCI_CONFIGURE:*/
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
	*lParam = (DWORD)PTR_SEG_TO_LIN(*lParam);
	return MCI_MAP_OK;
    case MCI_WINDOW:
	/* in fact, I would also need the dwFlags... to see 
	 * which members of lParam are effectively used 
	 */
	*lParam = (DWORD)PTR_SEG_TO_LIN(*lParam);
	FIXME("Current mapping may be wrong\n");
	break;
    case MCI_BREAK:
	{
            LPMCI_BREAK_PARMS		mbp32 = HeapAlloc(SystemHeap, 0, sizeof(MCI_BREAK_PARMS));
	    LPMCI_BREAK_PARMS16		mbp16 = PTR_SEG_TO_LIN(*lParam);

	    if (mbp32) {
		mbp32->dwCallback = mbp16->dwCallback;
		mbp32->nVirtKey = mbp16->nVirtKey;
		mbp32->hwndBreak = mbp16->hwndBreak;
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mbp32;
	}
	return MCI_MAP_OKMEM;
    case MCI_ESCAPE:
	{
            LPMCI_VD_ESCAPE_PARMSA	mvep32a = HeapAlloc(SystemHeap, 0, sizeof(MCI_VD_ESCAPE_PARMSA));
	    LPMCI_VD_ESCAPE_PARMS16	mvep16  = PTR_SEG_TO_LIN(*lParam);

	    if (mvep32a) {
		mvep32a->dwCallback       = mvep16->dwCallback;
		mvep32a->lpstrCommand     = PTR_SEG_TO_LIN(mvep16->lpstrCommand);
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mvep32a;
	}
	return MCI_MAP_OKMEM;
    case MCI_INFO:
	{
            LPMCI_INFO_PARMSA	mip32a = HeapAlloc(SystemHeap, 0, sizeof(MCI_INFO_PARMSA));
	    LPMCI_INFO_PARMS16	mip16  = PTR_SEG_TO_LIN(*lParam);

	    /* FIXME this is wrong if device is of type 
	     * MCI_DEVTYPE_DIGITAL_VIDEO, some members are not mapped
	     */
	    if (mip32a) {
		mip32a->dwCallback  = mip16->dwCallback;
		mip32a->lpstrReturn = PTR_SEG_TO_LIN(mip16->lpstrReturn);
		mip32a->dwRetSize   = mip16->dwRetSize;
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mip32a;
	}
	return MCI_MAP_OKMEM;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	{
            LPMCI_OPEN_PARMSA	mop32a = HeapAlloc(SystemHeap, 0, sizeof(LPMCI_OPEN_PARMS16) + sizeof(MCI_OPEN_PARMSA) + 2 * sizeof(DWORD));
	    LPMCI_OPEN_PARMS16	mop16  = PTR_SEG_TO_LIN(*lParam);

	    if (mop32a) {
		*(LPMCI_OPEN_PARMS16*)(mop32a) = mop16;
		mop32a = (LPMCI_OPEN_PARMSA)((char*)mop32a + sizeof(LPMCI_OPEN_PARMS16));
		mop32a->dwCallback       = mop16->dwCallback;
		mop32a->wDeviceID        = mop16->wDeviceID;
		mop32a->lpstrDeviceType  = PTR_SEG_TO_LIN(mop16->lpstrDeviceType);
		mop32a->lpstrElementName = PTR_SEG_TO_LIN(mop16->lpstrElementName);
		mop32a->lpstrAlias       = PTR_SEG_TO_LIN(mop16->lpstrAlias);
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
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)mop32a;
	}
	return MCI_MAP_OKMEM;
    case MCI_SYSINFO:
	{
            LPMCI_SYSINFO_PARMSA	msip32a = HeapAlloc(SystemHeap, 0, sizeof(MCI_SYSINFO_PARMSA));
	    LPMCI_SYSINFO_PARMS16	msip16  = PTR_SEG_TO_LIN(*lParam);

	    if (msip32a) {
		msip32a->dwCallback       = msip16->dwCallback;
		msip32a->lpstrReturn      = PTR_SEG_TO_LIN(msip16->lpstrReturn);
		msip32a->dwRetSize        = msip16->dwRetSize;
		msip32a->dwNumber         = msip16->dwNumber;
		msip32a->wDeviceType      = msip16->wDeviceType;
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (DWORD)msip32a;
	}
	return MCI_MAP_OKMEM;
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
	return MCI_MAP_OK;

    default:
	WARN("Don't know how to map msg=%s\n", MCI_CommandToString(wMsg));
    }
    return MCI_MAP_MSGERROR;
}

/**************************************************************************
 * 			MCI_UnMapMsg16To32A			[internal]
 */
MCI_MapType	MCI_UnMapMsg16To32A(WORD uDevType, WORD wMsg, DWORD lParam)
{
    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:	
	/* case MCI_CONFIGURE:*/
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
	return MCI_MAP_OK;

    case MCI_WINDOW:
	/* FIXME ?? see Map function */
	return MCI_MAP_OK;

    case MCI_BREAK:
    case MCI_ESCAPE:
    case MCI_INFO:
    case MCI_SYSINFO:
	HeapFree(SystemHeap, 0, (LPVOID)lParam);
	return MCI_MAP_OK;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	if (lParam) {
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)lParam;
	    LPMCI_OPEN_PARMS16	mop16  = *(LPMCI_OPEN_PARMS16*)((char*)mop32a - sizeof(LPMCI_OPEN_PARMS16*));
	    
	    mop16->wDeviceID = mop32a->wDeviceID;
	    if (!HeapFree(SystemHeap, 0, (LPVOID)(lParam - sizeof(LPMCI_OPEN_PARMS16))))
		FIXME("bad free line=%d\n", __LINE__);
	}
	return MCI_MAP_OK;
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
	return MCI_MAP_OK;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_CommandToString(wMsg));
    }
    return MCI_MAP_MSGERROR;
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
static	MCI_MapType	MCI_MsgMapper32To16_Create(void** ptr, int size16, DWORD map, BOOLEAN keep)
{
    void*	lp = SEGPTR_ALLOC((keep ? sizeof(void**) : 0) + size16);
    LPBYTE	p16, p32;

    if (!lp) {
	return MCI_MAP_NOMEM;
    }
    p32 = (LPBYTE)(*ptr);
    if (keep) {
	*(void**)lp = *ptr;
	p16 = (LPBYTE)lp + sizeof(void**);
	*ptr = (char*)SEGPTR_GET(lp) + sizeof(void**);
    } else {
	p16 = lp;
	*ptr = (void*)SEGPTR_GET(lp);
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
		case 0x1:	*( LPINT16)p16 = ( INT16)*( LPINT16)p32;		p16 += 2;	p32 += 4;	size16 -= 2;	break;
		case 0x2:	*(LPUINT16)p16 = (UINT16)*(LPUINT16)p32;		p16 += 2;     	p32 += 4;	size16 -= 2;	break;
		case 0x6:	*(LPDWORD)p16 = 0;					p16 += 4;     	p32 += 4;	size16 -= 4;	break;
		case 0x7:	*(LPDWORD)p16 = SEGPTR_GET(SEGPTR_STRDUP(*(LPSTR*)p32));p16 += 4;	p32 += 4;	size16 -= 4;	break;
		default:	FIXME("Unknown nibble for mapping (%x)\n", nibble);
		}
	    }
	    map >>= 4;
	}
	if (size16 != 0) /* DEBUG only */
	    FIXME("Mismatch between 16 bit struct size and map nibbles serie\n");
    }
    return MCI_MAP_OKMEM;
}

/**************************************************************************
 * 			MCI_MsgMapper32To16_Destroy		[internal]
 *
 * Helper for MCI_UnMapMsg32ATo16. 
 */
static	MCI_MapType	MCI_MsgMapper32To16_Destroy(void* ptr, int size16, DWORD map, BOOLEAN kept)
{
    if (ptr) {
	void*		msg16 = PTR_SEG_TO_LIN(ptr);
	void*		alloc;
	LPBYTE		p32, p16;
	unsigned	nibble;

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
			case 0x1:	*( LPINT)p32 = *( LPINT16)p16;				p16 += 2;	p32 += 4;	size16 -= 2;	break;
			case 0x2:	*(LPUINT)p32 = *(LPUINT16)p16;				p16 += 2;     	p32 += 4;	size16 -= 2;	break;
			case 0x6:								p16 += 4;     	p32 += 4;	size16 -= 4;	break;
			case 0x7:	strcpy(*(LPSTR*)p32, PTR_SEG_TO_LIN(*(DWORD*)p16));
			    if (!SEGPTR_FREE(PTR_SEG_TO_LIN(*(DWORD*)p16))) {
				FIXME("bad free line=%d\n", __LINE__);
			    }
			    p16 += 4;	p32 += 4;	size16 -= 4;	break;
			default:	FIXME("Unknown nibble for mapping (%x)\n", nibble);
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

	if (!SEGPTR_FREE(alloc)) {
	    FIXME("bad free line=%d\n", __LINE__);
	} 
    }	
    return MCI_MAP_OK;
}

/**************************************************************************
 * 			MCI_MapMsg32ATo16			[internal]
 *
 * Map a 32-A bit MCI message to a 16 bit MCI message.
 */
MCI_MapType	MCI_MapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD dwFlags, DWORD* lParam)
{
    int		size;
    BOOLEAN     keep = FALSE;
    DWORD	map = 0;

    if (*lParam == 0)
	return MCI_MAP_OK;

    /* FIXME: to add also (with seg/linear modifications to do):
     * MCI_LIST, MCI_LOAD, MCI_QUALITY, MCI_RESERVE, MCI_RESTORE, MCI_SAVE
     * MCI_SETAUDIO, MCI_SETTUNER, MCI_SETVIDEO
     */
    switch (wMsg) {
	/* case MCI_BREAK: */
	/* case MCI_CAPTURE */
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:	
	size = sizeof(MCI_GENERIC_PARMS);	
	break;
	/* case MCI_CONFIGURE:*/
	/* case MCI_COPY: */
    case MCI_CUE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_CUE_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_CUE_PARMS);	break;*/	FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
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
	    char*		ptr;
	    LPMCI_INFO_PARMS16	mip16;
	    
	    switch (uDevType) {
	    case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_INFO_PARMS16);	break;
	    default:				size = sizeof(MCI_INFO_PARMS16);	break;
	    }
	    ptr = SEGPTR_ALLOC(sizeof(LPMCI_INFO_PARMSA) + size);

	    if (ptr) {
		*(LPMCI_INFO_PARMSA*)ptr = mip32a;
		mip16 = (LPMCI_INFO_PARMS16)(ptr + sizeof(LPMCI_INFO_PARMSA));
		mip16->dwCallback  = mip32a->dwCallback;
		mip16->lpstrReturn = (LPSTR)SEGPTR_GET(SEGPTR_ALLOC(mip32a->dwRetSize));
		mip16->dwRetSize   = mip32a->dwRetSize;
		if (uDevType == MCI_DEVTYPE_DIGITAL_VIDEO) {
		    ((LPMCI_DGV_INFO_PARMS16)mip16)->dwItem = ((LPMCI_DGV_INFO_PARMSA)mip32a)->dwItem;
		}
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_INFO_PARMSA);
	}
	return MCI_MAP_OKMEM;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	{
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)(*lParam);
	    char*		ptr    = SEGPTR_ALLOC(sizeof(LPMCI_OPEN_PARMSA) + sizeof(MCI_OPEN_PARMS16) + 2 * sizeof(DWORD));
	    LPMCI_OPEN_PARMS16	mop16;


	    if (ptr) {
		*(LPMCI_OPEN_PARMSA*)(ptr) = mop32a;
		mop16 = (LPMCI_OPEN_PARMS16)(ptr + sizeof(LPMCI_OPEN_PARMSA));
		mop16->dwCallback       = mop32a->dwCallback;
		mop16->wDeviceID        = mop32a->wDeviceID;
		if (dwFlags & MCI_OPEN_TYPE) {
		    if (dwFlags & MCI_OPEN_TYPE_ID) {
			/* dword "transparent" value */
			mop16->lpstrDeviceType = mop32a->lpstrDeviceType;
		    } else {
			/* string */
			mop16->lpstrDeviceType = mop32a->lpstrDeviceType ? (LPSTR)SEGPTR_GET(SEGPTR_STRDUP(mop32a->lpstrDeviceType)) : 0;
		    }
		} else {
		    /* nuthin' */
		    mop16->lpstrDeviceType = 0;
		}
		if (dwFlags & MCI_OPEN_ELEMENT) {
		    if (dwFlags & MCI_OPEN_ELEMENT_ID) {
			mop16->lpstrElementName = mop32a->lpstrElementName;
		    } else {
			mop16->lpstrElementName = mop32a->lpstrElementName ? (LPSTR)SEGPTR_GET(SEGPTR_STRDUP(mop32a->lpstrElementName)) : 0;
		    }
		} else {
		    mop16->lpstrElementName = 0;
		}
		if (dwFlags & MCI_OPEN_ALIAS) {
		    mop16->lpstrAlias = mop32a->lpstrAlias ? (LPSTR)SEGPTR_GET(SEGPTR_STRDUP(mop32a->lpstrAlias)) : 0;
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
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_OPEN_PARMSA);
	}
	return MCI_MAP_OKMEM;
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
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_RECORD_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_RECORD_PARMS);	break;
	}
	break;
    case MCI_RESUME:		
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_SEEK:
	switch (uDevType) {
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SEEK_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_SEEK_PARMS);		break;
	}
	break;
    case MCI_SET:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_SET_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SET_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	case MCI_DEVTYPE_SEQUENCER:	size = sizeof(MCI_SEQ_SET_PARMS);	break;
        /* FIXME: normally the 16 and 32 bit structures are byte by byte aligned, 
	 * so not doing anything should work...
	 */
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_SET_PARMS);	break;
	default:			size = sizeof(MCI_SET_PARMS);		break;
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
	 * don't know if buffer for value is the one passed thru lpstrDevice 
	 * or is provided by MCI driver.
	 * Assuming solution 2: provided by MCI driver, so zeroing on entry
	 */
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STATUS_PARMS16);	map = 0x0B6FF;		break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STEP_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STEP_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	case MCI_DEVTYPE_VIDEODISC:	size = sizeof(MCI_VD_STEP_PARMS);	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;	
    case MCI_STOP:		
	size = sizeof(MCI_SET_PARMS);		
	break;
    case MCI_SYSINFO:
	{
            LPMCI_SYSINFO_PARMSA	msip32a = (LPMCI_SYSINFO_PARMSA)(*lParam);
	    char*		      	ptr     = SEGPTR_ALLOC(sizeof(LPMCI_SYSINFO_PARMSA) + sizeof(MCI_SYSINFO_PARMS16));
	    LPMCI_SYSINFO_PARMS16	msip16;

	    if (ptr) {
		*(LPMCI_SYSINFO_PARMSA*)(ptr) = msip32a;
		msip16 = (LPMCI_SYSINFO_PARMS16)(ptr + sizeof(LPMCI_SYSINFO_PARMSA));

		msip16->dwCallback       = msip32a->dwCallback;
		msip16->lpstrReturn      = (LPSTR)SEGPTR_GET(SEGPTR_ALLOC(msip32a->dwRetSize));
		msip16->dwRetSize        = msip32a->dwRetSize;
		msip16->dwNumber         = msip32a->dwNumber;
		msip16->wDeviceType      = msip32a->wDeviceType;
	    } else {
		return MCI_MAP_NOMEM;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_SYSINFO_PARMSA);
	}
	return MCI_MAP_OKMEM;
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
	return MCI_MAP_PASS;

    default:
	WARN("Don't know how to map msg=%s\n", MCI_CommandToString(wMsg));
	return MCI_MAP_MSGERROR;
    }
    return MCI_MsgMapper32To16_Create((void**)lParam, size, map, keep);
}

/**************************************************************************
 * 			MCI_UnMapMsg32ATo16			[internal]
 */
MCI_MapType	MCI_UnMapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD dwFlags, DWORD lParam)
{
    int		size = 0;
    BOOLEAN     kept = FALSE;	/* there is no need to compute size when kept is FALSE */
    DWORD 	map = 0;

    switch (wMsg) {
	/* case MCI_CAPTURE */
    case MCI_CLOSE:	
    case MCI_CLOSE_DRIVER:	
	break;
	/* case MCI_CONFIGURE:*/
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
            LPMCI_INFO_PARMS16	mip16  = (LPMCI_INFO_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_INFO_PARMSA	mip32a = *(LPMCI_INFO_PARMSA*)((char*)mip16 - sizeof(LPMCI_INFO_PARMSA));

	    memcpy(mip32a->lpstrReturn, PTR_SEG_TO_LIN(mip16->lpstrReturn), mip32a->dwRetSize);

	    if (!SEGPTR_FREE(PTR_SEG_TO_LIN(mip16->lpstrReturn)))
		FIXME("bad free line=%d\n", __LINE__);
	    if (!SEGPTR_FREE((char*)mip16 - sizeof(LPMCI_INFO_PARMSA)))
		FIXME("bad free line=%d\n", __LINE__);
	}
	return MCI_MAP_OK;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	if (lParam) {
            LPMCI_OPEN_PARMS16	mop16  = (LPMCI_OPEN_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_OPEN_PARMSA	mop32a = *(LPMCI_OPEN_PARMSA*)((char*)mop16 - sizeof(LPMCI_OPEN_PARMSA));
	    
	    mop32a->wDeviceID = mop16->wDeviceID;
	    if ((dwFlags & MCI_OPEN_TYPE) && ! 
		(dwFlags & MCI_OPEN_TYPE_ID) && 
		!SEGPTR_FREE(PTR_SEG_TO_LIN(mop16->lpstrDeviceType)))
		FIXME("bad free line=%d\n", __LINE__);
	    if ((dwFlags & MCI_OPEN_ELEMENT) && 
		!(dwFlags & MCI_OPEN_ELEMENT_ID) && 
		!SEGPTR_FREE(PTR_SEG_TO_LIN(mop16->lpstrElementName)))
		FIXME("bad free line=%d\n", __LINE__);
	    if ((dwFlags & MCI_OPEN_ALIAS) && 
		!SEGPTR_FREE(PTR_SEG_TO_LIN(mop16->lpstrAlias)))
		FIXME("bad free line=%d\n", __LINE__);

	    if (!SEGPTR_FREE((char*)mop16 - sizeof(LPMCI_OPEN_PARMSA)))
		FIXME("bad free line=%d\n", __LINE__);
	}
	return MCI_MAP_OK;
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
	/* case MCI_SETTIMECODE:*/
	/* case MCI_SIGNAL:*/
    case MCI_SPIN:
	break;
    case MCI_STATUS:
	kept = TRUE;
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	
	if (lParam) {
            LPMCI_DGV_STATUS_PARMS16	mdsp16  = (LPMCI_DGV_STATUS_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_DGV_STATUS_PARMSA	mdsp32a = *(LPMCI_DGV_STATUS_PARMSA*)((char*)mdsp16 - sizeof(LPMCI_DGV_STATUS_PARMSA));

	    if (mdsp16) {
		mdsp32a->dwReturn = mdsp16->dwReturn;
		if (dwFlags & MCI_DGV_STATUS_DISKSPACE) {
		    TRACE("MCI_STATUS (DGV) lpstrDrive=%p\n", mdsp16->lpstrDrive);
		    TRACE("MCI_STATUS (DGV) lpstrDrive=%s\n", (LPSTR)PTR_SEG_TO_LIN(mdsp16->lpstrDrive));

		    /* FIXME: see map function */
		    strcpy(mdsp32a->lpstrDrive, (LPSTR)PTR_SEG_TO_LIN(mdsp16->lpstrDrive));
		}

		if (!SEGPTR_FREE((char*)mdsp16 - sizeof(LPMCI_DGV_STATUS_PARMSA)))
		    FIXME("bad free line=%d\n", __LINE__);
	    } else {
		return MCI_MAP_NOMEM;
	    }
	}
	return MCI_MAP_OKMEM;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME("NIY vcr\n");	return MCI_MAP_NOMEM;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	break;
    case MCI_STOP:
	break;
    case MCI_SYSINFO:
	if (lParam) {
            LPMCI_SYSINFO_PARMS16	msip16  = (LPMCI_SYSINFO_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_SYSINFO_PARMSA	msip32a = *(LPMCI_SYSINFO_PARMSA*)((char*)msip16 - sizeof(LPMCI_SYSINFO_PARMSA));

	    if (msip16) {
		msip16->dwCallback       = msip32a->dwCallback;
		memcpy(msip32a->lpstrReturn, PTR_SEG_TO_LIN(msip16->lpstrReturn), msip32a->dwRetSize);
		if (!SEGPTR_FREE(PTR_SEG_TO_LIN(msip16->lpstrReturn)))
		    FIXME("bad free line=%d\n", __LINE__);

		if (!SEGPTR_FREE((char*)msip16 - sizeof(LPMCI_SYSINFO_PARMSA)))
		    FIXME("bad free line=%d\n", __LINE__);
	    } else {
		return MCI_MAP_NOMEM;
	    }
	}
	return MCI_MAP_OKMEM;
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
	return MCI_MAP_PASS;
    default:
	FIXME("Map/Unmap internal error on msg=%s\n", MCI_CommandToString(wMsg));
	return MCI_MAP_MSGERROR;
    }
    return MCI_MsgMapper32To16_Destroy((void*)lParam, size, map, kept);
}

/**************************************************************************
 * 			MCI_SendCommandFrom32			[internal]
 */
DWORD MCI_SendCommandFrom32(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD		dwRet = MCIERR_DEVICE_NOT_INSTALLED;
    
    if (!MCI_DevIDValid(wDevID)) {
	dwRet = MCIERR_INVALID_DEVICE_ID;
    } else {
	switch (DRIVER_GetType(MCI_GetDrv(wDevID)->hDrv)) {
	case WINE_DI_TYPE_16:
	    {
		MCI_MapType	res;
		
		switch (res = MCI_MapMsg32ATo16(MCI_GetDrv(wDevID)->modp.wType, wMsg, dwParam1, &dwParam2)) {
		case MCI_MAP_MSGERROR:
		    TRACE("Not handled yet (%s)\n", MCI_CommandToString(wMsg));
		    dwRet = MCIERR_DRIVER_INTERNAL;
		    break;
		case MCI_MAP_NOMEM:
		    TRACE("Problem mapping msg=%s from 32a to 16\n", MCI_CommandToString(wMsg));
		    dwRet = MCIERR_OUT_OF_MEMORY;
		    break;
		case MCI_MAP_OK:
		case MCI_MAP_OKMEM:
		    dwRet = SendDriverMessage16(MCI_GetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
		    if (res ==  MCI_MAP_OKMEM)
			MCI_UnMapMsg32ATo16(MCI_GetDrv(wDevID)->modp.wType, wMsg, dwParam1, dwParam2);
		    break;
		case MCI_MAP_PASS:
		    dwRet = SendDriverMessage(MCI_GetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
		    break;
		}
	    }
	    break;
	case WINE_DI_TYPE_32:
	    dwRet = SendDriverMessage(MCI_GetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
	    break;
	default:
	    WARN("Unknown driver type=%u\n", DRIVER_GetType(MCI_GetDrv(wDevID)->hDrv));
	    dwRet = MCIERR_DRIVER_INTERNAL;
	}
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_SendCommandFrom16			[internal]
 */
DWORD MCI_SendCommandFrom16(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD		dwRet = MCIERR_DEVICE_NOT_INSTALLED;
    
    if (!MCI_DevIDValid(wDevID)) {
	dwRet = MCIERR_INVALID_DEVICE_ID;
    } else {
	MCI_MapType		res;
	
	switch (DRIVER_GetType(MCI_GetDrv(wDevID)->hDrv)) {
	case WINE_DI_TYPE_16:		
	    dwRet = SendDriverMessage16(MCI_GetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
	    break;
	case WINE_DI_TYPE_32:
	    switch (res = MCI_MapMsg16To32A(MCI_GetDrv(wDevID)->modp.wType, wMsg, &dwParam2)) {
	    case MCI_MAP_MSGERROR:
		TRACE("Not handled yet (%s)\n", MCI_CommandToString(wMsg));
		dwRet = MCIERR_DRIVER_INTERNAL;
		break;
	    case MCI_MAP_NOMEM:
		TRACE("Problem mapping msg=%s from 16 to 32a\n", MCI_CommandToString(wMsg));
		dwRet = MCIERR_OUT_OF_MEMORY;
		break;
	    case MCI_MAP_OK:
	    case MCI_MAP_OKMEM:
		dwRet = SendDriverMessage(wDevID, wMsg, dwParam1, dwParam2);
		if (res == MCI_MAP_OKMEM)
		    MCI_UnMapMsg16To32A(MCI_GetDrv(wDevID)->modp.wType, wMsg, dwParam2);
		break;
	    case MCI_MAP_PASS:
		dwRet = SendDriverMessage16(MCI_GetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
		break;
	    }
	    break;
	default:
	    WARN("Unknown driver type=%u\n", DRIVER_GetType(MCI_GetDrv(wDevID)->hDrv));
	    dwRet = MCIERR_DRIVER_INTERNAL;
	}
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_Open				[internal]
 */
DWORD MCI_Open(DWORD dwParam, LPMCI_OPEN_PARMSA lpParms)
{
    char			strDevTyp[128];
    UINT16			uDevType = 0;
    UINT16			wDevID = MCI_FirstDevID();
    DWORD 			dwRet; 
    HDRVR			hDrv;
    MCI_OPEN_DRIVER_PARMSA	modp;
	    
   
    TRACE("(%08lX, %p)\n", dwParam, lpParms);
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    
    if ((dwParam & ~(MCI_OPEN_SHAREABLE|MCI_OPEN_ELEMENT|MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_NOTIFY|MCI_WAIT)) != 0) {
	FIXME("Unsupported yet dwFlags=%08lX\n", 
	      (dwParam & ~(MCI_OPEN_SHAREABLE|MCI_OPEN_ELEMENT|MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_NOTIFY|MCI_WAIT)));
    }

    while (MCI_GetDrv(wDevID)->modp.wType != 0) {
	wDevID = MCI_NextDevID(wDevID);
	if (!MCI_DevIDValid(wDevID)) {
	    TRACE("MAXMCIDRIVERS reached !\n");
	    return MCIERR_OUT_OF_MEMORY;
	}
    }

    TRACE("wDevID=%04X \n", wDevID);
    memcpy(MCI_GetOpenDrv(wDevID), lpParms, sizeof(*lpParms));
    
    strDevTyp[0] = 0;

    if (dwParam & MCI_OPEN_ELEMENT) {
	char*	t;
	
	TRACE("lpstrElementName='%s'\n", lpParms->lpstrElementName);
	t = strrchr(lpParms->lpstrElementName, '.');
	if (t) {
	    GetProfileStringA("mci extensions", t+1, "*", strDevTyp, sizeof(strDevTyp));
	    if (strcmp(strDevTyp, "*") == 0) {
		TRACE("No [mci extensions] entry for %s found.\n", t);
		return MCIERR_EXTENSION_NOT_FOUND;
	    }
	    TRACE("Extension %s is mapped to type %s\n", t, strDevTyp);
	} else if (GetDriveTypeA(lpParms->lpstrElementName) == DRIVE_CDROM) {
	    /* FIXME: this will not work if several CDROM drives are installed on the machine */
	    strcpy(strDevTyp, "CDAUDIO");
	} else {
	    return MCIERR_EXTENSION_NOT_FOUND;
	}
    }
    
    if (dwParam & MCI_OPEN_ALIAS) {
	TRACE("Alias='%s' !\n", lpParms->lpstrAlias);
	/* FIXME is there any memory leak here ? */
	MCI_GetOpenDrv(wDevID)->lpstrAlias = strdup(lpParms->lpstrAlias);
	/* mplayer does allocate alias to CDAUDIO */
    }
    if (dwParam & MCI_OPEN_TYPE) {
	if (dwParam & MCI_OPEN_TYPE_ID) {
#if 0
	    TRACE("Dev=%08lx!\n", (DWORD)lpParms->lpstrDeviceType);
	    uDevType = LOWORD((DWORD)lpParms->lpstrDeviceType);
	    MCI_GetOpenDrv(wDevID)->lpstrDeviceType = lpParms->lpstrDeviceType;
#endif
	    if (LOWORD((DWORD)lpParms->lpstrDeviceType) != MCI_DEVTYPE_CD_AUDIO) {
		FIXME("MCI_OPEN_TYPE_ID is no longer properly supported\n");
	    }
	    strcpy(strDevTyp, "CDAUDIO");
	} else {
	    if (lpParms->lpstrDeviceType == NULL) 
		return MCIERR_NULL_PARAMETER_BLOCK;
	    TRACE("Dev='%s' !\n", lpParms->lpstrDeviceType);
	    strcpy(strDevTyp, lpParms->lpstrDeviceType);
	}
    }

    if (strDevTyp[0] == 0) {
	FIXME("Couldn't load driver\n");
	return MCIERR_DRIVER_INTERNAL;
    }

    CharUpperA(strDevTyp);
    
    modp.wDeviceID = wDevID;
    modp.lpstrParams = NULL;
    
    /* FIXME: this is a hack... some MCI drivers, while being open, call
     * mciSetData, which lookup for non empty slots in MCI table list
     * Unfortunatly, open slots are known when wType == 0...
     * so use a dummy type, just to keep on going. May be wType == 0 is 
     * not the best solution to indicate empty slot in MCI drivers table
     */
    MCI_GetDrv(wDevID)->modp.wType = MCI_DEVTYPE_CD_AUDIO;
    hDrv = OpenDriverA(strDevTyp, "mci", (LPARAM)&modp);
    
    if (!hDrv) {
	FIXME("Couldn't load driver for type %s.\n", strDevTyp);
	return MCIERR_DEVICE_NOT_INSTALLED;
    }				
    uDevType = modp.wType;
    MCI_GetDrv(wDevID)->hDrv = hDrv;
    
    TRACE("Loaded driver %u (%s), type is %d\n", hDrv, strDevTyp, uDevType);
    
    MCI_GetDrv(wDevID)->mop.lpstrDeviceType = strdup(strDevTyp);
    MCI_GetDrv(wDevID)->modp.wType = uDevType;
    MCI_GetDrv(wDevID)->modp.wDeviceID = 0;  /* FIXME? for multiple devices */

    lpParms->wDeviceID = wDevID;

    TRACE("mcidev=%d, uDevType=%04X wDeviceID=%04X !\n", 
	  wDevID, uDevType, lpParms->wDeviceID);

    MCI_GetDrv(wDevID)->lpfnYieldProc = MCI_DefYieldProc;
    MCI_GetDrv(wDevID)->dwYieldData = VK_CANCEL;
    MCI_GetDrv(wDevID)->hCreatorTask = GetCurrentTask();
    MCI_GetDrv(wDevID)->dwPrivate = 0;
    
    dwRet = MCI_SendCommandFrom32(wDevID, MCI_OPEN_DRIVER, dwParam, (DWORD)lpParms);

    if (dwRet == 0) {
	/* only handled devices fall through */
	TRACE("wDevID = %04X wDeviceID = %d dwRet = %ld\n", wDevID, lpParms->wDeviceID, dwRet);
    } else {
	TRACE("Failed to open driver (MCI_OPEN_DRIVER msg) [%08lx], closing\n", dwRet);
	MCI_GetDrv(wDevID)->modp.wType = 0;
    }
    if (dwParam & MCI_NOTIFY)
	mciDriverNotify16(lpParms->dwCallback, wDevID, dwRet == 0 ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);
    
    return dwRet;
}

/**************************************************************************
 * 			MCI_Close				[internal]
 */
DWORD MCI_Close(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD	dwRet;
    
    TRACE("(%04x, %08lX, %p)\n", wDevID, dwParam, lpParms);

    if (wDevID == MCI_ALL_DEVICE_ID) {
	FIXME("unhandled MCI_ALL_DEVICE_ID\n");
	return MCIERR_CANNOT_USE_ALL;
    }

    dwRet = MCI_SendCommandFrom32(wDevID, MCI_CLOSE_DRIVER, dwParam, (DWORD)lpParms);
    if (MCI_GetDrv(wDevID)->hDrv) {
#if 1
	CloseDriver(MCI_GetDrv(wDevID)->hDrv, 0, 0);
#endif
    }
    MCI_GetDrv(wDevID)->modp.wType = 0;
    free(MCI_GetDrv(wDevID)->mop.lpstrDeviceType);

    if (dwParam & MCI_NOTIFY)
	mciDriverNotify16(lpParms->dwCallback, wDevID,
			  (dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);
    
    return dwRet;
}

/**************************************************************************
 * 			MCI_WriteString				[internal]
 */
DWORD	MCI_WriteString(LPSTR lpDstStr, DWORD dstSize, LPCSTR lpSrcStr)
{
    DWORD	ret;

    if (dstSize <= strlen(lpSrcStr)) {
	lstrcpynA(lpDstStr, lpSrcStr, dstSize - 1);
	ret = MCIERR_PARAM_OVERFLOW;
    } else {
	strcpy(lpDstStr, lpSrcStr);
	ret = 0;
    }
    return ret;
}

/**************************************************************************
 * 			MCI_Sysinfo				[internal]
 */
DWORD MCI_SysInfo(UINT uDevID, DWORD dwFlags, LPMCI_SYSINFO_PARMSA lpParms)
{
    DWORD	ret = MCIERR_INVALID_DEVICE_ID;
    
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    
    TRACE("(%08x, %08lX, %08lX[num=%ld, wDevTyp=%u])\n", 
	  uDevID, dwFlags, (DWORD)lpParms, lpParms->dwNumber, lpParms->wDeviceType);
    
    switch (dwFlags & ~MCI_SYSINFO_OPEN) {
    case MCI_SYSINFO_QUANTITY:
	{
	    DWORD	cnt = 0;
	    WORD	i;
	    
	    if (lpParms->wDeviceType < MCI_DEVTYPE_FIRST || lpParms->wDeviceType > MCI_DEVTYPE_LAST) {
		if (dwFlags & MCI_SYSINFO_OPEN) {
		    TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers\n");
		    for (i = 0; i < MAXMCIDRIVERS; i++) {
			if (mciDrv[i].modp.wType != 0) cnt++;
		    }
		} else {
		    TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers\n");
		    cnt = mciInstalledCount;
		}
	    } else {
		if (dwFlags & MCI_SYSINFO_OPEN) {
		    TRACE("MCI_SYSINFO_QUANTITY: # of open MCI drivers of type %u\n", lpParms->wDeviceType);
		    for (i = 0; i < MAXMCIDRIVERS; i++) {
			if (mciDrv[i].modp.wType == lpParms->wDeviceType) cnt++;
		    }
		} else {
		    TRACE("MCI_SYSINFO_QUANTITY: # of installed MCI drivers of type %u\n", lpParms->wDeviceType);
		    FIXME("Don't know how to get # of MCI devices of a given type\n");
		    cnt = 1;
		}
	    }
	    *(DWORD*)lpParms->lpstrReturn = cnt;
	}
	TRACE("(%ld) => '%ld'\n", lpParms->dwNumber, *(DWORD*)lpParms->lpstrReturn);
	ret = 0;
	break;
    case MCI_SYSINFO_INSTALLNAME:
	TRACE("MCI_SYSINFO_INSTALLNAME \n");
	if (MCI_DevIDValid(uDevID)) {
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, MCI_GetDrv(uDevID)->mop.lpstrDeviceType);
	} else {
	    *lpParms->lpstrReturn = 0;
	    ret = MCIERR_INVALID_DEVICE_ID;
	}
	TRACE("(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	break;
    case MCI_SYSINFO_NAME:
	TRACE("MCI_SYSINFO_NAME\n");
	if (dwFlags & MCI_SYSINFO_OPEN) {
	    FIXME("Don't handle MCI_SYSINFO_NAME|MCI_SYSINFO_OPEN (yet)\n");
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
	} else if (lpParms->dwNumber > mciInstalledCount) {
	    ret = MCIERR_OUTOFRANGE;
	} else {
	    DWORD	count = lpParms->dwNumber;
	    LPSTR	ptr = lpmciInstallNames;

	    while (--count > 0) ptr += strlen(ptr) + 1;
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, ptr);
	}
	TRACE("(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	break;
    default:
	TRACE("Unsupported flag value=%08lx\n", dwFlags);
	ret = MCIERR_UNRECOGNIZED_COMMAND;
    }
    return ret;
}

struct SCA {
    UINT 	wDevID;
    UINT 	wMsg;
    DWORD 	dwParam1;
    DWORD 	dwParam2;
    BOOL	allocatedCopy;
};

DWORD WINAPI mciSendCommandA(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2);

/**************************************************************************
 * 				MCI_SCAStarter			[internal]
 */
static DWORD CALLBACK	MCI_SCAStarter(LPVOID arg)
{
    struct SCA*	sca = (struct SCA*)arg;
    DWORD		ret;

    TRACE("In thread before async command (%08x,%s,%08lx,%08lx)\n",
	  sca->wDevID, MCI_CommandToString(sca->wMsg), sca->dwParam1, sca->dwParam2);
    ret = mciSendCommandA(sca->wDevID, sca->wMsg, sca->dwParam1 | MCI_WAIT, sca->dwParam2);
    TRACE("In thread after async command (%08x,%s,%08lx,%08lx)\n",
	  sca->wDevID, MCI_CommandToString(sca->wMsg), sca->dwParam1, sca->dwParam2);
    if (sca->allocatedCopy)
	HeapFree(GetProcessHeap(), 0, (LPVOID)sca->dwParam2);
    HeapFree(GetProcessHeap(), 0, sca);
    ExitThread(ret);
    WARN("Should not happen ? what's wrong \n");
    /* should not go after this point */
    return ret;
}

/**************************************************************************
 * 				MCI_SendCommandAsync		[internal]
 */
DWORD MCI_SendCommandAsync(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2, UINT size)
{
    struct SCA*	sca = HeapAlloc(GetProcessHeap(), 0, sizeof(struct SCA));

    if (sca == 0)
	return MCIERR_OUT_OF_MEMORY;

    sca->wDevID   = wDevID;
    sca->wMsg     = wMsg;
    sca->dwParam1 = dwParam1;
    
    if (size) {
	sca->dwParam2 = (DWORD)HeapAlloc(GetProcessHeap(), 0, size);
	if (sca->dwParam2 == 0) {
	    HeapFree(GetProcessHeap(), 0, sca);
	    return MCIERR_OUT_OF_MEMORY;
	}
	sca->allocatedCopy = TRUE;
	/* copy structure passed by program in dwParam2 to be sure 
	 * we can still use it whatever the program does 
	 */
	memcpy((LPVOID)sca->dwParam2, (LPVOID)dwParam2, size);
    } else {
	sca->dwParam2 = dwParam2;
	sca->allocatedCopy = FALSE;
    }

    if (CreateThread(NULL, 0, MCI_SCAStarter, sca, 0, NULL) == 0) {
	WARN("Couldn't allocate thread for async command handling, sending synchonously\n");
	return MCI_SCAStarter(&sca);
    }
    return 0;
}

/**************************************************************************
 * 				MCI_CleanUp			[internal]
 *
 * Some MCI commands need to be cleaned-up (when not called from 
 * mciSendString), because MCI drivers return extra information for string
 * transformation. This function gets read of them.
 */
LRESULT		MCI_CleanUp(LRESULT dwRet, UINT wMsg, DWORD dwParam2, BOOL bIs32)
{
    switch (wMsg) {
    case MCI_GETDEVCAPS:
	switch (dwRet & 0xFFFF0000ul) {
	case 0:
	    break;
	case MCI_RESOURCE_RETURNED:
	case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
	case MCI_COLONIZED3_RETURN:
	case MCI_COLONIZED4_RETURN:
	case MCI_INTEGER_RETURNED:
	    {
		LPMCI_GETDEVCAPS_PARMS	lmgp = (LPMCI_GETDEVCAPS_PARMS)(bIs32 ? (void*)dwParam2 : PTR_SEG_TO_LIN(dwParam2));

		dwRet = LOWORD(dwRet);
		TRACE("Changing %08lx to %08lx\n", lmgp->dwReturn, (DWORD)LOWORD(lmgp->dwReturn));

		lmgp->dwReturn = LOWORD(lmgp->dwReturn);
	    } 
	    break;
	default:
	    FIXME("Unsupported value for hiword (%04x) returned by DriverProc\n", HIWORD(dwRet));
	}
	break;
    case MCI_STATUS:
	switch (dwRet & 0xFFFF0000ul) {
	case 0:
	    break;
	case MCI_RESOURCE_RETURNED:
	case MCI_RESOURCE_RETURNED|MCI_RESOURCE_DRIVER:
	case MCI_COLONIZED3_RETURN:
	case MCI_COLONIZED4_RETURN:
	case MCI_INTEGER_RETURNED:
	    {
		LPMCI_STATUS_PARMS	lsp = (LPMCI_STATUS_PARMS)(bIs32 ? (void*)dwParam2 : PTR_SEG_TO_LIN(dwParam2));

		dwRet = LOWORD(dwRet);
		TRACE("Changing %08lx to %08lx\n", lsp->dwReturn,(DWORD) LOWORD(lsp->dwReturn));
		lsp->dwReturn = LOWORD(lsp->dwReturn);
	    }
	    break;
	default:
	    FIXME("Unsupported value for hiword (%04x) returned by DriverProc\n", HIWORD(dwRet));
	}
	break;
    default:
	break;
    }
    return dwRet;
}

/**************************************************************************
 * 			MULTIMEDIA_MciInit			[internal]
 *
 * Initializes the MCI internal variables.
 *
 */
BOOL MULTIMEDIA_MciInit(void)
{
    LPSTR	ptr1, ptr2;

    mciInstalledCount = 0;
    ptr1 = lpmciInstallNames = malloc(2048);

    if (!lpmciInstallNames)
	return FALSE;

    /* FIXME: should do also some registry diving here */
    if (PROFILE_GetWineIniString("options", "mci", "", lpmciInstallNames, 2048) > 0) {
	TRACE_(mci)("Wine => '%s' \n", ptr1);
	while ((ptr2 = strchr(ptr1, ':')) != 0) {
	    *ptr2++ = 0;
	    TRACE_(mci)("---> '%s' \n", ptr1);
	    mciInstalledCount++;
	    ptr1 = ptr2;
	}
	mciInstalledCount++;
	TRACE_(mci)("---> '%s' \n", ptr1);
	ptr1 += strlen(ptr1) + 1;
    } else {
	GetPrivateProfileStringA("mci", NULL, "", lpmciInstallNames, 2048, "SYSTEM.INI");
	while (strlen(ptr1) > 0) {
	    TRACE_(mci)("---> '%s' \n", ptr1);
	    ptr1 += (strlen(ptr1) + 1);
	    mciInstalledCount++;
	}
    }
    mciInstalledListLen = ptr1 - lpmciInstallNames;

    return TRUE;
}

