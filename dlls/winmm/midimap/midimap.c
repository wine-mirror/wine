/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*				   
 * Wine Midi mapper driver
 *
 * Copyright 	1999 Eric Pouech
 */

#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "driver.h"
#include "mmddk.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msacm)

typedef	struct tagMIDIMAPDATA {
    struct tagMIDIMAPDATA*	self;
    HMIDI	hMidi;
} MIDIMAPDATA;

static	BOOL	MIDIMAP_IsData(MIDIMAPDATA* mm)
{
    return (!IsBadReadPtr(mm, sizeof(MIDIMAPDATA)) && mm->self == mm);
}

/*======================================================================*
 *                  MIDI OUT part                                       *
 *======================================================================*/

static	DWORD	modOpen(LPDWORD lpdwUser, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    UINT 		nd = midiOutGetNumDevs();
    UINT		i;
    MIDIMAPDATA*	mom = HeapAlloc(GetProcessHeap(), 0, sizeof(MIDIMAPDATA));

    TRACE("(%p %p %08lx\n", lpdwUser, lpDesc, dwFlags);

    for (i = 0; i < nd; i++) {
	if (midiOutOpen(&mom->hMidi, i, lpDesc->dwCallback, 
			lpDesc->dwInstance, dwFlags) == MMSYSERR_NOERROR) {
	    lpDesc->hMidi = mom->hMidi;
	    *lpdwUser = (DWORD)mom;
	    return MMSYSERR_NOERROR;
	}
    }
    HeapFree(GetProcessHeap(), 0, mom);
    return MMSYSERR_ALLOCATED;
}

static	DWORD	modClose(MIDIMAPDATA* mom)
{
    DWORD ret = midiOutClose(mom->hMidi);
    if (ret == MMSYSERR_NOERROR)
	HeapFree(GetProcessHeap(), 0, mom);
    return ret;
}

static	DWORD	modLongData(MIDIMAPDATA* mom, LPMIDIHDR lpMidiHdr, DWORD dwParam2)
{
    return midiOutLongMsg(mom->hMidi, lpMidiHdr, dwParam2);
}

static	DWORD	modData(MIDIMAPDATA* mom, DWORD dwParam)
{
    return midiOutShortMsg(mom->hMidi, dwParam);
}

static	DWORD	modPrepare(MIDIMAPDATA* mom, LPMIDIHDR lpMidiHdr, DWORD dwParam2)
{
    return midiOutPrepareHeader(mom->hMidi, lpMidiHdr, dwParam2);
}

static	DWORD	modUnprepare(MIDIMAPDATA* mom, LPMIDIHDR lpMidiHdr, DWORD dwParam2)
{
    return midiOutUnprepareHeader(mom->hMidi, lpMidiHdr, dwParam2);
}

static	DWORD	modGetDevCaps(UINT wDevID, MIDIMAPDATA* mom, LPMIDIOUTCAPSA lpMidiCaps, DWORD dwParam2)
{
    /* if opened low driver, forward message */
    if (MIDIMAP_IsData(mom))
	return midiOutGetDevCapsA(mom->hMidi, lpMidiCaps, dwParam2);
    /* otherwise, return caps of mapper itself */
    if (wDevID == (UINT)-1 || wDevID == (UINT16)-1) {
	lpMidiCaps->wMid = 0x00FF;
	lpMidiCaps->wPid = 0x0001;
	lpMidiCaps->vDriverVersion = 0x0100;
	strcpy(lpMidiCaps->szPname, "Wine midi out mapper");
	lpMidiCaps->wTechnology = MOD_MAPPER;
	lpMidiCaps->wVoices = 0;
	lpMidiCaps->wNotes = 0;
	lpMidiCaps->wChannelMask = 0xFFFF;
	lpMidiCaps->dwSupport = MIDICAPS_LRVOLUME | MIDICAPS_VOLUME;

	return MMSYSERR_NOERROR;
    }
    ERR("This shouldn't happen\n");
    return MMSYSERR_ERROR; 
}

static	DWORD	modGetVolume(UINT wDevID, MIDIMAPDATA* mom, LPDWORD lpVol)
{
    if (MIDIMAP_IsData(mom))
	return midiOutGetVolume(mom->hMidi, lpVol);
    return MMSYSERR_ERROR;
}

static	DWORD	modSetVolume(UINT wDevID, MIDIMAPDATA* mom, DWORD vol)
{
    if (MIDIMAP_IsData(mom))
	return midiOutSetVolume(mom->hMidi, vol);
    return MMSYSERR_ERROR;
}

static	DWORD	modReset(MIDIMAPDATA* mom)
{
    return midiOutReset(mom->hMidi);
}

/**************************************************************************
 * 				MIDIMAP_modMessage	[sample driver]
 */
DWORD WINAPI MIDIMAP_modMessage(UINT wDevID, UINT wMsg, DWORD dwUser, 
				DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);
    
    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;

    case MODM_OPEN:	 	return modOpen		((LPDWORD)dwUser,      (LPMIDIOPENDESC)dwParam1,dwParam2);
    case MODM_CLOSE:	 	return modClose		((MIDIMAPDATA*)dwUser);

    case MODM_DATA:		return modData		((MIDIMAPDATA*)dwUser, dwParam1);
    case MODM_LONGDATA:		return modLongData      ((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1,     dwParam2);
    case MODM_PREPARE:	 	return modPrepare	((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1, 	dwParam2);
    case MODM_UNPREPARE: 	return modUnprepare	((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1, 	dwParam2);

    case MODM_GETDEVCAPS:	return modGetDevCaps	(wDevID, (MIDIMAPDATA*)dwUser, (LPMIDIOUTCAPSA)dwParam1,dwParam2);
    case MODM_GETNUMDEVS:	return 1;
    case MODM_GETVOLUME:	return modGetVolume	(wDevID, (MIDIMAPDATA*)dwUser, (LPDWORD)dwParam1);
    case MODM_SETVOLUME:	return modSetVolume	(wDevID, (MIDIMAPDATA*)dwUser, dwParam1);
    case MODM_RESET:		return modReset		((MIDIMAPDATA*)dwUser);
    default:
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  MIDI IN part                                        *
 *======================================================================*/

static	DWORD	midOpen(LPDWORD lpdwUser, LPMIDIOPENDESC lpDesc, DWORD dwFlags)
{
    UINT 		nd = midiInGetNumDevs();
    UINT		i;
    MIDIMAPDATA*	mim = HeapAlloc(GetProcessHeap(), 0, sizeof(MIDIMAPDATA));

    TRACE("(%p %p %08lx\n", lpdwUser, lpDesc, dwFlags);

    for (i = 0; i < nd; i++) {
	if (midiInOpen(&mim->hMidi, i, lpDesc->dwCallback, 
			lpDesc->dwInstance, dwFlags) == MMSYSERR_NOERROR) {
	    lpDesc->hMidi = mim->hMidi;
	    *lpdwUser = (DWORD)mim;
	    return MMSYSERR_NOERROR;
	}
    }
    HeapFree(GetProcessHeap(), 0, mim);
    return MMSYSERR_ALLOCATED;
}

static	DWORD	midClose(MIDIMAPDATA* mim)
{
    DWORD ret = midiInClose(mim->hMidi);
    if (ret == MMSYSERR_NOERROR)
	HeapFree(GetProcessHeap(), 0, mim);
    return ret;
}

static	DWORD	midAddBuffer(MIDIMAPDATA* mim, LPMIDIHDR lpMidiHdr, DWORD dwParam2)
{
    return midiInAddBuffer(mim->hMidi, lpMidiHdr, dwParam2);
}

static	DWORD	midPrepare(MIDIMAPDATA* mim, LPMIDIHDR lpMidiHdr, DWORD dwParam2)
{
    return midiInPrepareHeader(mim->hMidi, lpMidiHdr, dwParam2);
}

static	DWORD	midUnprepare(MIDIMAPDATA* mim, LPMIDIHDR lpMidiHdr, DWORD dwParam2)
{
    return midiInUnprepareHeader(mim->hMidi, lpMidiHdr, dwParam2);
}

static	DWORD	midGetDevCaps(UINT wDevID, MIDIMAPDATA* mim, LPMIDIINCAPSA lpMidiCaps, DWORD dwParam2)
{
    /* if opened low driver, forward message */
    if (MIDIMAP_IsData(mim))
	return midiInGetDevCapsA(mim->hMidi, lpMidiCaps, dwParam2);
    /* otherwise, return caps of mapper itself */
    if (wDevID == (UINT)-1 || wDevID == (UINT16)-1) {
	lpMidiCaps->wMid = 0x00FF;
	lpMidiCaps->wPid = 0x0001;
	lpMidiCaps->vDriverVersion = 0x0100;
	strcpy(lpMidiCaps->szPname, "Wine midi int mapper");
	lpMidiCaps->dwSupport = 0;

	return MMSYSERR_NOERROR;
    }
    ERR("This shouldn't happen\n");
    return MMSYSERR_ERROR; 
}

static	DWORD	midStop(MIDIMAPDATA* mim)
{
    return midiInStop(mim->hMidi);
}

static	DWORD	midStart(MIDIMAPDATA* mim)
{
    return midiInStart(mim->hMidi);
}

static	DWORD	midReset(MIDIMAPDATA* mim)
{
    return midiInReset(mim->hMidi);
}

/**************************************************************************
 * 				MIDIMAP_midMessage	[sample driver]
 */
DWORD WINAPI MIDIMAP_midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
				DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
    case DRVM_EXIT:
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;

    case MIDM_OPEN:		return midOpen          ((LPDWORD)dwUser,     (LPMIDIOPENDESC)dwParam1, dwParam2);
    case MIDM_CLOSE:		return midClose         ((MIDIMAPDATA*)dwUser);

    case MIDM_ADDBUFFER:	return midAddBuffer     ((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_PREPARE:		return midPrepare       ((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_UNPREPARE:	return midUnprepare     ((MIDIMAPDATA*)dwUser, (LPMIDIHDR)dwParam1, dwParam2);
    case MIDM_GETDEVCAPS:	return midGetDevCaps    (wDevID, (MIDIMAPDATA*)dwUser, (LPMIDIINCAPSA)dwParam1, dwParam2);
    case MIDM_GETNUMDEVS:	return 1;
    case MIDM_RESET:		return midReset         ((MIDIMAPDATA*)dwUser);
    case MIDM_START:		return midStart         ((MIDIMAPDATA*)dwUser);
    case MIDM_STOP:		return midStop          ((MIDIMAPDATA*)dwUser);
    default:
	FIXME("unknown message %u!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Driver part                                         *
 *======================================================================*/

static	struct WINE_MIDIMAP* oss = NULL;

/**************************************************************************
 * 				MIDIMAP_drvOpen			[internal]	
 */
static	DWORD	MIDIMAP_drvOpen(LPSTR str)
{
    if (oss)
	return 0;
    
    /* I know, this is ugly, but who cares... */
    oss = (struct WINE_MIDIMAP*)1;
    return 1;
}

/**************************************************************************
 * 				MIDIMAP_drvClose		[internal]	
 */
static	DWORD	MIDIMAP_drvClose(DWORD dwDevID)
{
    if (oss) {
	oss = NULL;
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				MIDIMAP_DriverProc		[internal]
 */
LONG CALLBACK	MIDIMAP_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
				   DWORD dwParam1, DWORD dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */
    
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return MIDIMAP_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return MIDIMAP_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MIDIMAP MultiMedia Driver !", "OSS Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}


