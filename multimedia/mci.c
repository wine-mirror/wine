/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MCI internal functions
 *
 * Copyright 1998/1999 Eric Pouech
 */

#include <string.h>

#include "winbase.h"
#include "winuser.h"
#include "heap.h"
#include "driver.h"
#include "mmsystem.h"
#include "multimedia.h"
#include "selectors.h"
#include "debug.h"
#include "digitalv.h"

struct WINE_MCIDRIVER mciDrv[MAXMCIDRIVERS];

int	mciInstalledCount;
int	mciInstalledListLen;
LPSTR	lpmciInstallNames = NULL;

/* The wDevID's returned by wine were originally in the range 
 * 0 - (MAXMCIDRIVERS - 1) and used directly as array indices.
 * Unfortunately, ms-windows uses wDevID of zero to indicate
 * errors.  Now, multimedia drivers must pass the wDevID through
 * MCI_DevIDToIndex to get an index in that range.  An
 * arbitrary value, MCI_MAGIC is added to the wDevID seen
 * by the windows programs.
 */

#define MCI_MAGIC 0x0F00

MCI_WineDesc	MCI_InternalDescriptors[] = {
    {MCI_DEVTYPE_CD_AUDIO,		"CDAUDIO", 	MCICDAUDIO_DriverProc},
    {MCI_DEVTYPE_WAVEFORM_AUDIO,	"WAVEAUDIO",	MCIWAVE_DriverProc},
    {MCI_DEVTYPE_SEQUENCER,		"SEQUENCER", 	MCIMIDI_DriverProc},
    {MCI_DEVTYPE_ANIMATION,		"ANIMATION1", 	MCIANIM_DriverProc},
    {MCI_DEVTYPE_DIGITAL_VIDEO,		"AVIVIDEO", 	MCIAVI_DriverProc},
    {0xFFFF,				NULL,		NULL}	/* sentinel */
};

/**************************************************************************
 * 				MCI_GetDevTypeString		[internal]
 */
static	LPCSTR		MCI_GetDevTypeString(WORD uDevType)
{
    LPCSTR	str = "??? MCI ???";
    int		i;

    for (i = 0; MCI_InternalDescriptors[i].uDevType != 0xFFFF; i++) {
	if (MCI_InternalDescriptors[i].uDevType != 0 && MCI_InternalDescriptors[i].uDevType == uDevType) {
	    str = MCI_InternalDescriptors[i].lpstrName;
	    break;
	}
    }
    /*    TRACE(mci, "devType=%u => %s\n", uDevType, str);*/
    return str;
}

/**************************************************************************
 * 				MCI_GetProc			[internal]
 */
static	MCIPROC		MCI_GetProc(UINT16 uDevType)
{
    MCIPROC	proc = 0;
    int		i;

    for (i = 0; MCI_InternalDescriptors[i].uDevType != 0xFFFF; i++) {
	if (MCI_InternalDescriptors[i].uDevType != 0 && 
	    MCI_InternalDescriptors[i].uDevType == uDevType) {
	    proc = MCI_InternalDescriptors[i].lpfnProc;
	    break;
	}
    }
    return proc;
}

/**************************************************************************
 * 				MCI_GetDevType			[internal]
 */
WORD		MCI_GetDevType(LPCSTR str)
{
    WORD 	uDevType = 0;
    int		i;

    for (i = 0; MCI_InternalDescriptors[i].uDevType != 0xFFFF; i++) {
	if (MCI_InternalDescriptors[i].uDevType != 0 && 
	    strcmp(str, MCI_InternalDescriptors[i].lpstrName) == 0) {
	    uDevType = MCI_InternalDescriptors[i].uDevType;
	    break;
	}
    }
    return uDevType;
}

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
int	MCI_MapMsg16To32A(WORD uDevType, WORD wMsg, DWORD* lParam)
{
    if (*lParam == 0)
	return 0;
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
	return 0;
    case MCI_WINDOW:
	/* in fact, I would also need the dwFlags... to see 
	 * which members of lParam are effectively used 
	 */
	*lParam = (DWORD)PTR_SEG_TO_LIN(*lParam);
	FIXME(mci, "Current mapping may be wrong\n");
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
		return -2;
	    }
	    *lParam = (DWORD)mbp32;
	}
	return 1;
    case MCI_ESCAPE:
	{
            LPMCI_VD_ESCAPE_PARMSA	mvep32a = HeapAlloc(SystemHeap, 0, sizeof(MCI_VD_ESCAPE_PARMSA));
	    LPMCI_VD_ESCAPE_PARMS16	mvep16  = PTR_SEG_TO_LIN(*lParam);

	    if (mvep32a) {
		mvep32a->dwCallback       = mvep16->dwCallback;
		mvep32a->lpstrCommand     = PTR_SEG_TO_LIN(mvep16->lpstrCommand);
	    } else {
		return -2;
	    }
	    *lParam = (DWORD)mvep32a;
	}
	return 1;
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
		return -2;
	    }
	    *lParam = (DWORD)mip32a;
	}
	return 1;
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
		return -2;
	    }
	    *lParam = (DWORD)mop32a;
	}
	return 1;
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
		return -2;
	    }
	    *lParam = (DWORD)msip32a;
	}
	return 1;
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
	FIXME(mci, "This is a hack\n");
	return 0;

    default:
	WARN(mci, "Don't know how to map msg=%s\n", MCI_CommandToString(wMsg));
    }
    return -1;
}

/**************************************************************************
 * 			MCI_UnMapMsg16To32A			[internal]
 */
int	MCI_UnMapMsg16To32A(WORD uDevType, WORD wMsg, DWORD lParam)
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
	return 0;

    case MCI_WINDOW:
	/* FIXME ?? see Map function */
	return 0;

    case MCI_BREAK:
    case MCI_ESCAPE:
    case MCI_INFO:
    case MCI_SYSINFO:
	HeapFree(SystemHeap, 0, (LPVOID)lParam);
	return 0;
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	if (lParam) {
            LPMCI_OPEN_PARMSA	mop32a = (LPMCI_OPEN_PARMSA)lParam;
	    LPMCI_OPEN_PARMS16	mop16  = *(LPMCI_OPEN_PARMS16*)((char*)mop32a - sizeof(LPMCI_OPEN_PARMS16*));
	    
	    mop16->wDeviceID = mop32a->wDeviceID;
	    if (!HeapFree(SystemHeap, 0, (LPVOID)(lParam - sizeof(LPMCI_OPEN_PARMS16))))
		FIXME(mci, "bad free line=%d\n", __LINE__);
	}
	return 0;
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
	FIXME(mci, "This is a hack\n");
	return 0;
    default:
	FIXME(mci, "Map/Unmap internal error on msg=%s\n", MCI_CommandToString(wMsg));
    }
    return -1;
}

/**************************************************************************
 * 			MCI_MsgMapper32To16_Create		[internal]
 *
 * Helper for MCI_MapMsg32ATo16. 
 * Maps the 32 bit pointer (*ptr), of size bytes, to an allocated 16 bit segmented pointer.
 * if keep is TRUE, keeps track of in 32 bit ptr in allocated 16 bit area.
 *  1 : ok, some memory allocated
 * -2 : ko, memory problem
 */
static	int	MCI_MsgMapper32To16_Create(void** ptr, int size, BOOLEAN keep)
{
    void*	lp = SEGPTR_ALLOC(sizeof(void**) + size);

    if (!lp) {
	return -2;
    }
    if (keep) {
	*(void**)lp = *ptr;
	memcpy((char*)lp + sizeof(void**), *ptr, size);
	*ptr = (char*)SEGPTR_GET(lp) + sizeof(void**);
    } else {
	memcpy((char*)lp, *ptr, size);
	*ptr = (void*)SEGPTR_GET(lp);
    }
    return 1;
   
}

/**************************************************************************
 * 			MCI_MsgMapper32To16_Destroy		[internal]
 *
 * Helper for MCI_UnMapMsg32ATo16. 
 */
static	int	MCI_MsgMapper32To16_Destroy(void* ptr, int size, BOOLEAN kept)
{
    if (ptr) {
	void*	msg16 = PTR_SEG_TO_LIN(ptr);
	void*	alloc;

	if (kept) {
	    alloc = (char*)msg16 - sizeof(void**);
	    memcpy(*(void**)alloc, msg16, size);
	} else {
	    alloc = msg16;
	}

	if (!SEGPTR_FREE(alloc)) {
	    FIXME(mci, "bad free\n");
	} 
    }	
    return 0;
}

/**************************************************************************
 * 			MCI_MapMsg32ATo16			[internal]
 *
 * Map a 32-A bit MCI message to a 16 bit MCI message.
 *  1 : ok, some memory allocated, need to call MCI_UnMapMsg32ATo16
 *  0 : ok, no memory allocated
 * -1 : ko, unknown message
 * -2 : ko, memory problem
 */
int	MCI_MapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD* lParam)
{
    int		size;
    BOOLEAN     keep = FALSE;

    if (*lParam == 0)
	return 0;
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
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_CUE_PARMS);	break;*/	FIXME(mci, "NIY vcr\n");	return -2;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_CUT:*/
    case MCI_DELETE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_DELETE_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_DELETE_PARMS);	break;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
	/* case MCI_ESCAPE: */
    case MCI_FREEZE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_FREEZE_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	case MCI_DEVTYPE_OVERLAY:	/*size = sizeof(MCI_OVLY_FREEZE_PARMS);	FIXME(mci, "NIY rect\n");	return -2;*/
	                                FIXME(mci, "NIY ovly\n");	return -2;
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
		return -2;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_INFO_PARMSA);
	}
	return 1;
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
		mop16->lpstrDeviceType  = SEGPTR_STRDUP(mop32a->lpstrDeviceType);
		mop16->lpstrElementName = SEGPTR_STRDUP(mop32a->lpstrElementName);
		mop16->lpstrAlias       = SEGPTR_STRDUP(mop32a->lpstrAlias);
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
		return -2;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_OPEN_PARMSA);
	}
	return 1;
	/* case MCI_PASTE:*/
    case MCI_PAUSE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_PLAY:
	switch (uDevType) {
	case MCI_DEVTYPE_OVERLAY:	/*size = sizeof(MCI_OVLY_PLAY_PARMS);	break;*/FIXME(mci, "NIY ovly\n");	return -2;
	default:			size = sizeof(MCI_PLAY_PARMS);		break;
	}
	break;
    case MCI_PUT:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_PUT_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	case MCI_DEVTYPE_OVERLAY:	/*size = sizeof(MCI_OVLY_PUT_PARMS);	FIXME(mci, "NIY rect\n");	return -2;*/
	                                FIXME(mci, "NIY ovly\n");	return -2;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_REALIZE:
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_RECORD:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECORD_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_RECORD_PARMS);	break;*/FIXME(mci, "NIY vcr\n");	return -2;
	default:			size = sizeof(MCI_RECORD_PARMS);	break;
	}
	break;
    case MCI_RESUME:		
	size = sizeof(MCI_GENERIC_PARMS);
	break;
    case MCI_SEEK:
	switch (uDevType) {
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SEEK_PARMS);	break;*/FIXME(mci, "NIY vcr\n");	return -2;
	default:			size = sizeof(MCI_SEEK_PARMS);		break;
	}
	break;
    case MCI_SET:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_SET_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_SET_PARMS);	break;*/FIXME(mci, "NIY vcr\n");	return -2;
	case MCI_DEVTYPE_SEQUENCER:	size = sizeof(MCI_SEQ_SET_PARMS);	break;
	case MCI_DEVTYPE_WAVEFORM_AUDIO:size = sizeof(MCI_WAVE_SET_PARMS);	FIXME(mci, "NIY UINT\n");	return -2;
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
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STATUS_PARMS16);	FIXME(mci, "NIY lpstrDevice\n");return -2;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME(mci, "NIY vcr\n");	return -2;
	default:			size = sizeof(MCI_STATUS_PARMS);	break;
	}
	break;
    case MCI_STEP:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STEP_PARMS);	break;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STEP_PARMS);	break;*/FIXME(mci, "NIY vcr\n");	return -2;
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
		return -2;
	    }
	    *lParam = (LPARAM)SEGPTR_GET(ptr) + sizeof(LPMCI_SYSINFO_PARMSA);
	}
	return 1;
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_UPDATE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_UPDATE_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_WHERE:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_RECT_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	case MCI_DEVTYPE_OVERLAY:	size = sizeof(MCI_OVLY_RECT_PARMS);	FIXME(mci, "NIY rect\n");	return -2;
	default:			size = sizeof(MCI_GENERIC_PARMS);	break;
	}
	break;
    case MCI_WINDOW:
	switch (uDevType) {
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_WINDOW_PARMS16);	FIXME(mci, "NIY lpstrText\n");	return -2;
	case MCI_DEVTYPE_OVERLAY:	/*size = sizeof(MCI_OVLY_WINDOW_PARMS);	FIXME(mci, "NIY lpstrText\n");	return -2;*/
	                                FIXME(mci, "NIY ovly\n");	return -2;
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
	FIXME(mci, "This is a hack\n");
	return 0;

    default:
	WARN(mci, "Don't know how to map msg=%s\n", MCI_CommandToString(wMsg));
	return -1;
    }
    return MCI_MsgMapper32To16_Create((void**)lParam, size, keep);
}

/**************************************************************************
 * 			MCI_UnMapMsg32ATo16			[internal]
 */
int	MCI_UnMapMsg32ATo16(WORD uDevType, WORD wMsg, DWORD lParam)
{
    int		size = 0;
    BOOLEAN     kept = FALSE;	/* there is no need to compute size when kept is FALSE */

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
		FIXME(mci, "bad free line=%d\n", __LINE__);
	    if (!SEGPTR_FREE((char*)mip16 - sizeof(LPMCI_INFO_PARMSA)))
		FIXME(mci, "bad free line=%d\n", __LINE__);
	}
	return 0;
	/* case MCI_MARK: */
	/* case MCI_MONITOR: */
    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	
	if (lParam) {
            LPMCI_OPEN_PARMS16	mop16  = (LPMCI_OPEN_PARMS16)PTR_SEG_TO_LIN(lParam);
	    LPMCI_OPEN_PARMSA	mop32a = *(LPMCI_OPEN_PARMSA*)((char*)mop16 - sizeof(LPMCI_OPEN_PARMSA));
	    
	    mop32a->wDeviceID = mop16->wDeviceID;
	    if (!SEGPTR_FREE(mop16->lpstrDeviceType))
		FIXME(mci, "bad free line=%d\n", __LINE__);
	    if (!SEGPTR_FREE(mop16->lpstrElementName))
		FIXME(mci, "bad free line=%d\n", __LINE__);
	    if (!SEGPTR_FREE(mop16->lpstrAlias))
		FIXME(mci, "bad free line=%d\n", __LINE__);

	    if (!SEGPTR_FREE((LPVOID)((char*)mop16 - sizeof(LPMCI_OPEN_PARMSA))))
		FIXME(mci, "bad free line=%d\n", __LINE__);
	}
	return 0;
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
	case MCI_DEVTYPE_DIGITAL_VIDEO:	size = sizeof(MCI_DGV_STATUS_PARMS16);	FIXME(mci, "NIY lpstrDevice\n");return -2;
	case MCI_DEVTYPE_VCR:		/*size = sizeof(MCI_VCR_STATUS_PARMS);	break;*/FIXME(mci, "NIY vcr\n");	return -2;
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
		if (!SEGPTR_FREE(msip16->lpstrReturn))
		    FIXME(mci, "bad free line=%d\n", __LINE__);

		if (!SEGPTR_FREE((LPVOID)(lParam - sizeof(LPMCI_SYSINFO_PARMSA))))
		    FIXME(mci, "bad free line=%d\n", __LINE__);
	    } else {
		return -2;
	    }
	}
	return 1;
	/* case MCI_UNDO: */
    case MCI_UNFREEZE:
	break;
    case MCI_UPDATE:
	break;
    case MCI_WHERE:
	break;
    case MCI_WINDOW:
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
	FIXME(mci, "This is a hack\n");
	return 0;
    default:
	FIXME(mci, "Map/Unmap internal error on msg=%s\n", MCI_CommandToString(wMsg));
	return -1;
    }
    return MCI_MsgMapper32To16_Destroy((void*)lParam, size, kept);
}

/**************************************************************************
 * 			MCI_SendCommand				[internal]
 */
DWORD MCI_SendCommand(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD		dwRet = MCIERR_DEVICE_NOT_INSTALLED;
    
    if (!MCI_DevIDValid(wDevID)) {
	dwRet = MCIERR_INVALID_DEVICE_ID;
    } else {
	MCIPROC		proc = MCI_GetProc(MCI_GetDrv(wDevID)->modp.wType);
	
	if (proc) {
	    dwRet = (*proc)(MCI_GetDrv(wDevID)->modp.wDeviceID, 
			    MCI_GetDrv(wDevID)->hDrv, 
			    wMsg, dwParam1, dwParam2);
	} else if (MCI_GetDrv(wDevID)->hDrv) {
	    switch (DRIVER_GetType(MCI_GetDrv(wDevID)->hDrv)) {
	    case WINE_DI_TYPE_16:
		{
		    int			res;
		    
		    switch (res = MCI_MapMsg32ATo16(MCI_GetDrv(wDevID)->modp.wType, wMsg, &dwParam2)) {
		    case -1:
			TRACE(mci, "Not handled yet (%s)\n", MCI_CommandToString(wMsg));
			dwRet = MCIERR_DRIVER_INTERNAL;
			break;
		    case -2:
			TRACE(mci, "Problem mapping msg=%s from 16 to 32a\n", MCI_CommandToString(wMsg));
			dwRet = MCIERR_OUT_OF_MEMORY;
			break;
		    case 0:
		    case 1:
			dwRet = SendDriverMessage16(MCI_GetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
			if (res)
			    MCI_UnMapMsg32ATo16(MCI_GetDrv(wDevID)->modp.wType, wMsg, dwParam2);
			break;
		    }
		}
		break;
	    case WINE_DI_TYPE_32:
		dwRet = SendDriverMessage(MCI_GetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
		break;
	    default:
		WARN(mci, "Unknown driver type=%u\n", DRIVER_GetType(MCI_GetDrv(wDevID)->hDrv));
		dwRet = MCIERR_DRIVER_INTERNAL;
	    }
	} else {
	    WARN(mci, "unknown device type=%04X !\n", MCI_GetDrv(wDevID)->modp.wType);
	}
    }
    return dwRet;
}

/**************************************************************************
 * 			MCI_Open				[internal]
 */
DWORD MCI_Open(DWORD dwParam, LPMCI_OPEN_PARMSA lpParms)
{
    char		strDevTyp[128];
    UINT16		uDevType = 0;
    UINT16		wDevID = MCI_FirstDevID();
    DWORD 		dwRet;
    
    TRACE(mci, "(%08lX, %p)\n", dwParam, lpParms);
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    
    if ((dwParam & ~(MCI_OPEN_SHAREABLE|MCI_OPEN_ELEMENT|MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_NOTIFY|MCI_WAIT)) != 0) {
	FIXME(mci, "unsupported yet dwFlags=%08lX\n", 
	      (dwParam & ~(MCI_OPEN_SHAREABLE|MCI_OPEN_ELEMENT|MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_NOTIFY|MCI_WAIT)));
    }

    while (MCI_GetDrv(wDevID)->modp.wType != 0) {
	wDevID = MCI_NextDevID(wDevID);
	if (!MCI_DevIDValid(wDevID)) {
	    TRACE(mci, "MAXMCIDRIVERS reached !\n");
	    return MCIERR_OUT_OF_MEMORY;
	}
    }

    TRACE(mci, "wDevID=%04X \n", wDevID);
    memcpy(MCI_GetOpenDrv(wDevID), lpParms, sizeof(*lpParms));
    
    strDevTyp[0] = 0;

    if (dwParam & MCI_OPEN_ELEMENT) {
	char*	t;
	
	TRACE(mci, "lpstrElementName='%s'\n", lpParms->lpstrElementName);
	t = strrchr(lpParms->lpstrElementName, '.');
	if (t) {
	    GetProfileStringA("mci extensions", t+1, "*", strDevTyp, sizeof(strDevTyp));
	    if (strcmp(strDevTyp, "*") == 0) {
		TRACE(mci,"No [mci extensions] entry for %s found.\n", t);
		return MCIERR_EXTENSION_NOT_FOUND;
	    }
	    TRACE(mci, "Extension %s is mapped to type %s\n", t, strDevTyp);
	} else if (GetDriveTypeA(lpParms->lpstrElementName) == DRIVE_CDROM) {
	    /* FIXME: this will not work if several CDROM drives are installed on the machine */
	    strcpy(strDevTyp, "CDAUDIO");
	} else {
	    return MCIERR_EXTENSION_NOT_FOUND;
	}
    }
    
    if (dwParam & MCI_OPEN_ALIAS) {
	TRACE(mci, "Alias='%s' !\n", lpParms->lpstrAlias);
	/* FIXME is there any memory leak here ? */
	MCI_GetOpenDrv(wDevID)->lpstrAlias = strdup(lpParms->lpstrAlias);
	/* mplayer does allocate alias to CDAUDIO */
    }
    if (dwParam & MCI_OPEN_TYPE) {
	if (dwParam & MCI_OPEN_TYPE_ID) {
#if 0
	    TRACE(mci, "Dev=%08lx!\n", (DWORD)lpParms->lpstrDeviceType);
	    uDevType = LOWORD((DWORD)lpParms->lpstrDeviceType);
	    MCI_GetOpenDrv(wDevID)->lpstrDeviceType = lpParms->lpstrDeviceType;
#endif
	    if (LOWORD((DWORD)lpParms->lpstrDeviceType) != MCI_DEVTYPE_CD_AUDIO) {
		FIXME(mci, "MCI_OPEN_TYPE_ID is no longer properly supported\n");
	    }
	} else {
	    if (lpParms->lpstrDeviceType == NULL) 
		return MCIERR_NULL_PARAMETER_BLOCK;
	    TRACE(mci, "Dev='%s' !\n", lpParms->lpstrDeviceType);
	    /* FIXME is there any memory leak here ? */
	    MCI_GetOpenDrv(wDevID)->lpstrDeviceType = strdup(lpParms->lpstrDeviceType);
	    strcpy(strDevTyp, lpParms->lpstrDeviceType);
	}
    }

    if (uDevType == 0 && strDevTyp[0] != 0) {
	CharUpperA(strDevTyp);
	/* try Wine internal MCI drivers */
	uDevType = MCI_GetDevType(strDevTyp);
	if (uDevType == 0) { /* Nope, load external */
	    HDRVR			hDrv;
	    MCI_OPEN_DRIVER_PARMSA	modp;
	    
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
		FIXME(mci, "Couldn't load driver for type %s.\n", strDevTyp);
		return MCIERR_DEVICE_NOT_INSTALLED;
	    }			
	    uDevType = modp.wType;
	    MCI_GetDrv(wDevID)->hDrv = hDrv;

	    TRACE(mci, "Loaded driver %u (%s), type is %d\n", hDrv, strDevTyp, uDevType);
	}
    } else {
	MCI_GetDrv(wDevID)->hDrv = 0;
    }

    MCI_GetDrv(wDevID)->modp.wType = uDevType;
    MCI_GetDrv(wDevID)->modp.wDeviceID = 0;  /* FIXME? for multiple devices */

    lpParms->wDeviceID = wDevID;

    TRACE(mci, "mcidev=%d, uDevType=%04X wDeviceID=%04X !\n", 
	  wDevID, uDevType, lpParms->wDeviceID);

    dwRet = MCI_SendCommand(wDevID, MCI_OPEN_DRIVER, dwParam, (DWORD)lpParms);

    if (dwRet == 0) {
	/* only handled devices fall through */
	TRACE(mci, "wDevID = %04X wDeviceID = %d dwRet = %ld\n", wDevID, lpParms->wDeviceID, dwRet);
    } else {
	TRACE(mci, "failed to open driver (MCI_OPEN_DRIVER msg) [%08lx], closing\n", dwRet);
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
    
    TRACE(mci, "(%04x, %08lX, %p)\n", wDevID, dwParam, lpParms);

    if (wDevID == MCI_ALL_DEVICE_ID) {
	FIXME(mci, "unhandled MCI_ALL_DEVICE_ID\n");
	return MCIERR_CANNOT_USE_ALL;
    }

    dwRet = MCI_SendCommand(wDevID, MCI_CLOSE_DRIVER, dwParam, (DWORD)lpParms);
    if (MCI_GetDrv(wDevID)->hDrv) {
#if 0
	CloseDriver(MCI_GetDrv(wDevID)->hDrv, 0, 0);
#endif
    }
    MCI_GetDrv(wDevID)->modp.wType = 0;
    
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
    
    TRACE(mci, "(%08x, %08lX, %08lX[num=%ld, wDevTyp=%u])\n", 
	  uDevID, dwFlags, (DWORD)lpParms, lpParms->dwNumber, lpParms->wDeviceType);
    
    switch (dwFlags & ~MCI_SYSINFO_OPEN) {
    case MCI_SYSINFO_QUANTITY:
	{
	    DWORD	cnt = 0;
	    WORD	i;
	    
	    if (lpParms->wDeviceType < MCI_DEVTYPE_FIRST || lpParms->wDeviceType > MCI_DEVTYPE_LAST) {
		if (dwFlags & MCI_SYSINFO_OPEN) {
		    TRACE(mci, "MCI_SYSINFO_QUANTITY: # of open MCI drivers\n");
		    for (i = 0; i < MAXMCIDRIVERS; i++) {
			if (mciDrv[i].modp.wType != 0) cnt++;
		    }
		} else {
		    TRACE(mci, "MCI_SYSINFO_QUANTITY: # of installed MCI drivers\n");
		    cnt = mciInstalledCount;
		}
	    } else {
		if (dwFlags & MCI_SYSINFO_OPEN) {
		    TRACE(mci, "MCI_SYSINFO_QUANTITY: # of open MCI drivers of type %u\n", lpParms->wDeviceType);
		    for (i = 0; i < MAXMCIDRIVERS; i++) {
			if (mciDrv[i].modp.wType == lpParms->wDeviceType) cnt++;
		    }
		} else {
		    TRACE(mci, "MCI_SYSINFO_QUANTITY: # of installed MCI drivers of type %u\n", lpParms->wDeviceType);
		    FIXME(mci, "Don't know how to get # of MCI devices of a given type\n");
		    cnt = 1;
		}
	    }
	    *(DWORD*)lpParms->lpstrReturn = cnt;
	}
	TRACE(mci, "(%ld) => '%ld'\n", lpParms->dwNumber, *(DWORD*)lpParms->lpstrReturn);
	ret = 0;
	break;
    case MCI_SYSINFO_INSTALLNAME:
	TRACE(mci, "MCI_SYSINFO_INSTALLNAME \n");
	if (MCI_DevIDValid(uDevID)) {
	    LPCSTR	str = MCI_GetDevTypeString(MCI_GetDrv(uDevID)->modp.wType);
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, str);
	} else {
	    *lpParms->lpstrReturn = 0;
	    ret = MCIERR_INVALID_DEVICE_ID;
	}
	TRACE(mci, "(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	break;
    case MCI_SYSINFO_NAME:
	TRACE(mci, "MCI_SYSINFO_NAME\n");
	if (dwFlags & MCI_SYSINFO_OPEN) {
	    FIXME(mci, "Don't handle MCI_SYSINFO_NAME|MCI_SYSINFO_OPEN (yet)\n");
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
	} else if (lpParms->dwNumber > mciInstalledCount) {
	    ret = MCIERR_OUTOFRANGE;
	} else {
	    DWORD	count = lpParms->dwNumber;
	    LPSTR	ptr = lpmciInstallNames;

	    while (--count > 0) ptr += strlen(ptr) + 1;
	    ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, ptr);
	}
	TRACE(mci, "(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	break;
    default:
	TRACE(mci, "Unsupported flag value=%08lx\n", dwFlags);
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
static DWORD WINAPI MCI_SCAStarter(LPVOID arg)
{
    struct SCA*	sca = (struct SCA*)arg;
    DWORD		ret;

    TRACE(mci, "In thread before async command (%08x,%s,%08lx,%08lx)\n",
	  sca->wDevID, MCI_CommandToString(sca->wMsg), sca->dwParam1, sca->dwParam2);
    ret = mciSendCommandA(sca->wDevID, sca->wMsg, sca->dwParam1 | MCI_WAIT, sca->dwParam2);
    TRACE(mci, "In thread after async command (%08x,%s,%08lx,%08lx)\n",
	  sca->wDevID, MCI_CommandToString(sca->wMsg), sca->dwParam1, sca->dwParam2);
    if (sca->allocatedCopy)
	HeapFree(GetProcessHeap(), 0, (LPVOID)sca->dwParam2);
    HeapFree(GetProcessHeap(), 0, sca);
    ExitThread(ret);
    WARN(mci, "Should not happen ? what's wrong \n");
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
	WARN(mci, "Couldn't allocate thread for async command handling, sending synchonously\n");
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
		TRACE(mci, "Changing %08lx to %08lx\n", lmgp->dwReturn, (DWORD)LOWORD(lmgp->dwReturn));

		lmgp->dwReturn = LOWORD(lmgp->dwReturn);
	    } 
	    break;
	default:
	    FIXME(mci, "Unsupported value for hiword (%04x) returned by DriverProc\n", HIWORD(dwRet));
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
		TRACE(mci, "Changing %08lx to %08lx\n", lsp->dwReturn,(DWORD) LOWORD(lsp->dwReturn));
		lsp->dwReturn = LOWORD(lsp->dwReturn);
	    }
	    break;
	default:
	    FIXME(mci, "Unsupported value for hiword (%04x) returned by DriverProc\n", HIWORD(dwRet));
	}
	break;
    default:
	break;
    }
    return dwRet;
}

