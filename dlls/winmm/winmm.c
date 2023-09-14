/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * WINMM functions
 *
 * Copyright 1993      Martin Ayotte
 *           1998-2002 Eric Pouech
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

/*
 * Eric POUECH :
 * 	98/9	added Win32 MCI support
 *  	99/4	added midiStream support
 *      99/9	added support for loadable low level drivers
 */

/* TODO
 *      + it seems that some programs check what's installed in
 *        registry against the value returned by drivers. Wine is
 *        currently broken regarding this point.
 *      + check thread-safeness for MMSYSTEM and WINMM entry points
 *      + unicode entry points are badly supported (would require
 *        moving 32 bit drivers as Unicode as they are supposed to be)
 *      + allow joystick and timer external calls as we do for wave,
 *        midi, mixer and aux
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winuser.h"
#include "winnls.h"
#include "winternl.h"
#include "winemm.h"

#include "wine/debug.h"
#include "wine/rbtree.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

/* ========================================================================
 *                   G L O B A L   S E T T I N G S
 * ========================================================================*/

HINSTANCE hWinMM32Instance;
HANDLE psLastEvent;

static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &WINMM_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": WINMM_cs") }
};
CRITICAL_SECTION WINMM_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static struct wine_rb_tree wine_midi_streams;
static int wine_midi_stream_compare(const void *key, const struct wine_rb_entry *entry);

/**************************************************************************
 * 			WINMM_CreateIData			[internal]
 */
static	BOOL	WINMM_CreateIData(HINSTANCE hInstDLL)
{
    hWinMM32Instance = hInstDLL;
    wine_rb_init(&wine_midi_streams, wine_midi_stream_compare);
    psLastEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    return psLastEvent != NULL;
}

/******************************************************************
 *             WINMM_ErrorToString
 */
const char* WINMM_ErrorToString(MMRESULT error)
{
#define ERR_TO_STR(dev) case dev: return #dev
    switch (error) {
    ERR_TO_STR(MMSYSERR_NOERROR);
    ERR_TO_STR(MMSYSERR_ERROR);
    ERR_TO_STR(MMSYSERR_BADDEVICEID);
    ERR_TO_STR(MMSYSERR_NOTENABLED);
    ERR_TO_STR(MMSYSERR_ALLOCATED);
    ERR_TO_STR(MMSYSERR_INVALHANDLE);
    ERR_TO_STR(MMSYSERR_NODRIVER);
    ERR_TO_STR(MMSYSERR_NOMEM);
    ERR_TO_STR(MMSYSERR_NOTSUPPORTED);
    ERR_TO_STR(MMSYSERR_BADERRNUM);
    ERR_TO_STR(MMSYSERR_INVALFLAG);
    ERR_TO_STR(MMSYSERR_INVALPARAM);
    ERR_TO_STR(MMSYSERR_HANDLEBUSY);
    ERR_TO_STR(MMSYSERR_INVALIDALIAS);
    ERR_TO_STR(MMSYSERR_BADDB);
    ERR_TO_STR(MMSYSERR_KEYNOTFOUND);
    ERR_TO_STR(MMSYSERR_READERROR);
    ERR_TO_STR(MMSYSERR_WRITEERROR);
    ERR_TO_STR(MMSYSERR_DELETEERROR);
    ERR_TO_STR(MMSYSERR_VALNOTFOUND);
    ERR_TO_STR(MMSYSERR_NODRIVERCB);
    ERR_TO_STR(WAVERR_BADFORMAT);
    ERR_TO_STR(WAVERR_STILLPLAYING);
    ERR_TO_STR(WAVERR_UNPREPARED);
    ERR_TO_STR(WAVERR_SYNC);
    ERR_TO_STR(MIDIERR_INVALIDSETUP);
    ERR_TO_STR(MIDIERR_NODEVICE);
    ERR_TO_STR(MIDIERR_STILLPLAYING);
    ERR_TO_STR(MIDIERR_UNPREPARED);
    }
#undef ERR_TO_STR
    return wine_dbg_sprintf("Unknown(0x%08x)", error);
}

/**************************************************************************
 *		DllMain (WINMM.init)
 *
 * WINMM DLL entry point
 *
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%lx %p\n", hInstDLL, fdwReason, fImpLoad);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);

        if (!WINMM_CreateIData(hInstDLL))
            return FALSE;
        break;
    case DLL_PROCESS_DETACH:
        if(fImpLoad)
            break;

        joystick_unload();
        MCI_SendCommand(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_WAIT, 0L);
        MMDRV_Exit();
        DRIVER_UnloadAll();
        WINMM_DeleteWaveform();
        TIME_MMTimeStop();
        CloseHandle(psLastEvent);
        DeleteCriticalSection(&WINMM_cs);
        break;
    }
    return TRUE;
}

/**************************************************************************
 * 			WINMM_CheckCallback			[internal]
 */
MMRESULT WINMM_CheckCallback(DWORD_PTR dwCallback, DWORD fdwOpen, BOOL mixer)
{
    switch (fdwOpen & CALLBACK_TYPEMASK) {
    case CALLBACK_NULL:     /* dwCallback need not be NULL */
        break;
    case CALLBACK_WINDOW:
        if (dwCallback && !IsWindow((HWND)dwCallback))
            return MMSYSERR_INVALPARAM;
        break;

    case CALLBACK_FUNCTION:
        /* a NULL cb is acceptable since w2k, MMSYSERR_INVALPARAM earlier */
        if (mixer)
            return MMSYSERR_INVALFLAG; /* since w2k, MMSYSERR_NOTSUPPORTED earlier */
        break;
    case CALLBACK_THREAD:
    case CALLBACK_EVENT:
        if (mixer) /* FIXME: mixer supports THREAD+EVENT since w2k */
            return MMSYSERR_NOTSUPPORTED; /* w9X */
        break;
    default:
        WARN("Unknown callback type %d\n", HIWORD(fdwOpen));
    }
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				auxGetNumDevs		[WINMM.@]
 */
UINT WINAPI auxGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_AUX);
}

/**************************************************************************
 * 				auxGetDevCapsW		[WINMM.@]
 */
UINT WINAPI auxGetDevCapsW(UINT_PTR uDeviceID, LPAUXCAPSW lpCaps, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04IX, %p, %d) !\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;
    return MMDRV_Message(wmld, AUXDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize);
}

/**************************************************************************
 * 				auxGetDevCapsA		[WINMM.@]
 */
UINT WINAPI auxGetDevCapsA(UINT_PTR uDeviceID, LPAUXCAPSA lpCaps, UINT uSize)
{
    AUXCAPSW	acW;
    UINT	ret;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    ret = auxGetDevCapsW(uDeviceID, &acW, sizeof(acW));

    if (ret == MMSYSERR_NOERROR) {
	AUXCAPSA acA;
	acA.wMid           = acW.wMid;
	acA.wPid           = acW.wPid;
	acA.vDriverVersion = acW.vDriverVersion;
	WideCharToMultiByte( CP_ACP, 0, acW.szPname, -1, acA.szPname,
                             sizeof(acA.szPname), NULL, NULL );
	acA.wTechnology    = acW.wTechnology;
	acA.wReserved1     = acW.wReserved1;
	acA.dwSupport      = acW.dwSupport;
	memcpy(lpCaps, &acA, min(uSize, sizeof(acA)));
    }
    return ret;
}

/**************************************************************************
 * 				auxGetVolume		[WINMM.@]
 */
UINT WINAPI auxGetVolume(UINT uDeviceID, DWORD* lpdwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p) !\n", uDeviceID, lpdwVolume);

    if ((wmld = MMDRV_Get((HANDLE)(DWORD_PTR)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETVOLUME, (DWORD_PTR)lpdwVolume, 0L);
}

/**************************************************************************
 * 				auxSetVolume		[WINMM.@]
 */
UINT WINAPI auxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %lu) !\n", uDeviceID, dwVolume);

    if ((wmld = MMDRV_Get((HANDLE)(DWORD_PTR)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_SETVOLUME, dwVolume, 0L);
}

/**************************************************************************
 * 				auxOutMessage		[WINMM.@]
 */
UINT WINAPI auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD_PTR dw1, DWORD_PTR dw2)
{
    LPWINE_MLD		wmld;

    if ((wmld = MMDRV_Get((HANDLE)(DWORD_PTR)uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dw1, dw2);
}

/**************************************************************************
 * 				midiOutGetNumDevs	[WINMM.@]
 */
UINT WINAPI midiOutGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_MIDIOUT);
}

/**************************************************************************
 * 				midiOutGetDevCapsW	[WINMM.@]
 */
UINT WINAPI midiOutGetDevCapsW(UINT_PTR uDeviceID, LPMIDIOUTCAPSW lpCaps,
			       UINT uSize)
{
    LPWINE_MLD	wmld;

    TRACE("(%Iu, %p, %u);\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_MIDIOUT, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, MODM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize);
}

/**************************************************************************
 * 				midiOutGetDevCapsA	[WINMM.@]
 */
UINT WINAPI midiOutGetDevCapsA(UINT_PTR uDeviceID, LPMIDIOUTCAPSA lpCaps,
			       UINT uSize)
{
    MIDIOUTCAPSW	mocW;
    UINT		ret;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    ret = midiOutGetDevCapsW(uDeviceID, &mocW, sizeof(mocW));

    if (ret == MMSYSERR_NOERROR) {
	MIDIOUTCAPSA mocA;
	mocA.wMid		= mocW.wMid;
	mocA.wPid		= mocW.wPid;
	mocA.vDriverVersion	= mocW.vDriverVersion;
	WideCharToMultiByte( CP_ACP, 0, mocW.szPname, -1, mocA.szPname,
                             sizeof(mocA.szPname), NULL, NULL );
	mocA.wTechnology        = mocW.wTechnology;
	mocA.wVoices		= mocW.wVoices;
	mocA.wNotes		= mocW.wNotes;
	mocA.wChannelMask	= mocW.wChannelMask;
	mocA.dwSupport	        = mocW.dwSupport;
	memcpy(lpCaps, &mocA, min(uSize, sizeof(mocA)));
    }
    return ret;
}

/**************************************************************************
 * 				midiOutGetErrorTextA 	[WINMM.@]
 * 				midiInGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI midiOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    UINT	ret;

    if (lpText == NULL) ret = MMSYSERR_INVALPARAM;
    else if (uSize == 0) ret = MMSYSERR_NOERROR;
    else
    {
        WCHAR *xstr = malloc(uSize * sizeof(WCHAR));
        if (!xstr) ret = MMSYSERR_NOMEM;
        else
        {
            ret = midiOutGetErrorTextW(uError, xstr, uSize);
            if (ret == MMSYSERR_NOERROR)
                WideCharToMultiByte(CP_ACP, 0, xstr, -1, lpText, uSize, NULL, NULL);
            free(xstr);
        }
    }
    return ret;
}

/**************************************************************************
 * 				midiOutGetErrorTextW 	[WINMM.@]
 * 				midiInGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI midiOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    UINT        ret = MMSYSERR_BADERRNUM;

    if (lpText == NULL) ret = MMSYSERR_INVALPARAM;
    else if (uSize == 0) ret = MMSYSERR_NOERROR;
    else if (
	       /* test has been removed because MMSYSERR_BASE is 0, and gcc did emit
		* a warning for the test was always true */
	       (/*uError >= MMSYSERR_BASE && */ uError <= MMSYSERR_LASTERROR) ||
	       (uError >= MIDIERR_BASE  && uError <= MIDIERR_LASTERROR)) {
	if (LoadStringW(hWinMM32Instance, uError, lpText, uSize) > 0) {
	    ret = MMSYSERR_NOERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				MIDI_OutAlloc    		[internal]
 */
static	LPWINE_MIDI	MIDI_OutAlloc(HMIDIOUT* lphMidiOut, DWORD_PTR* lpdwCallback,
				      DWORD_PTR* lpdwInstance, LPDWORD lpdwFlags,
				      DWORD cIDs, MIDIOPENSTRMID* lpIDs)
{
    HANDLE	      	hMidiOut;
    LPWINE_MIDI		lpwm;
    UINT		size;

    size = sizeof(WINE_MIDI) + (cIDs ? (cIDs-1) : 0) * sizeof(MIDIOPENSTRMID);

    lpwm = (LPWINE_MIDI)MMDRV_Alloc(size, MMDRV_MIDIOUT, &hMidiOut, lpdwFlags,
				    lpdwCallback, lpdwInstance);
    *lphMidiOut = hMidiOut;

    if (lpwm) {
        lpwm->mod.hMidi = hMidiOut;
	lpwm->mod.dwCallback = *lpdwCallback;
	lpwm->mod.dwInstance = *lpdwInstance;
	lpwm->mod.dnDevNode = 0;
	lpwm->mod.cIds = cIDs;
	if (cIDs)
	    memcpy(&(lpwm->mod.rgIds), lpIDs, cIDs * sizeof(MIDIOPENSTRMID));
    }
    return lpwm;
}

/**************************************************************************
 * 				midiOutOpen    		[WINMM.@]
 */
MMRESULT WINAPI midiOutOpen(LPHMIDIOUT lphMidiOut, UINT uDeviceID,
                       DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD dwFlags)
{
    HMIDIOUT		hMidiOut;
    LPWINE_MIDI		lpwm;
    MMRESULT		dwRet;

    TRACE("(%p, %d, %08IX, %08IX, %08lX);\n",
	  lphMidiOut, uDeviceID, dwCallback, dwInstance, dwFlags);

    if (lphMidiOut != NULL) *lphMidiOut = 0;

    dwRet = WINMM_CheckCallback(dwCallback, dwFlags, FALSE);
    if (dwRet != MMSYSERR_NOERROR)
	return dwRet;

    lpwm = MIDI_OutAlloc(&hMidiOut, &dwCallback, &dwInstance, &dwFlags, 0, NULL);

    if (lpwm == NULL)
	return MMSYSERR_NOMEM;

    lpwm->mld.uDeviceID = uDeviceID;

    dwRet = MMDRV_Open((LPWINE_MLD)lpwm, MODM_OPEN, (DWORD_PTR)&lpwm->mod, dwFlags);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMidiOut, (LPWINE_MLD)lpwm);
	hMidiOut = 0;
    }

    if (lphMidiOut) *lphMidiOut = hMidiOut;
    TRACE("=> %d hMidi=%p\n", dwRet, hMidiOut);

    return dwRet;
}

/**************************************************************************
 * 				midiOutClose		[WINMM.@]
 */
UINT WINAPI midiOutClose(HMIDIOUT hMidiOut)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hMidiOut);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MODM_CLOSE);
    MMDRV_Free(hMidiOut, wmld);

    return dwRet;
}

/**************************************************************************
 * 				midiOutPrepareHeader	[WINMM.@]
 */
UINT WINAPI midiOutPrepareHeader(HMIDIOUT hMidiOut,
				 MIDIHDR* lpMidiOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    /* FIXME: detect MIDIStream handles and enforce 64KB buffer limit on those */

    return MMDRV_Message(wmld, MODM_PREPARE, (DWORD_PTR)lpMidiOutHdr, uSize);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[WINMM.@]
 */
UINT WINAPI midiOutUnprepareHeader(HMIDIOUT hMidiOut,
				   MIDIHDR* lpMidiOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_UNPREPARE, (DWORD_PTR)lpMidiOutHdr, uSize);
}

/**************************************************************************
 * 				midiOutShortMsg		[WINMM.@]
 */
UINT WINAPI midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %08lX)\n", hMidiOut, dwMsg);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_DATA, dwMsg, 0L);
}

/**************************************************************************
 * 				midiOutLongMsg		[WINMM.@]
 */
UINT WINAPI midiOutLongMsg(HMIDIOUT hMidiOut,
			   MIDIHDR* lpMidiOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_LONGDATA, (DWORD_PTR)lpMidiOutHdr, uSize);
}

/**************************************************************************
 * 				midiOutReset		[WINMM.@]
 */
UINT WINAPI midiOutReset(HMIDIOUT hMidiOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%p)\n", hMidiOut);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_RESET, 0L, 0L);
}

/**************************************************************************
 * 				midiOutGetVolume	[WINMM.@]
 */
UINT WINAPI midiOutGetVolume(HMIDIOUT hMidiOut, DWORD* lpdwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p);\n", hMidiOut, lpdwVolume);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_GETVOLUME, (DWORD_PTR)lpdwVolume, 0L);
}

/**************************************************************************
 * 				midiOutSetVolume	[WINMM.@]
 */
UINT WINAPI midiOutSetVolume(HMIDIOUT hMidiOut, DWORD dwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %ld);\n", hMidiOut, dwVolume);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_SETVOLUME, dwVolume, 0L);
}

/**************************************************************************
 * 				midiOutCachePatches		[WINMM.@]
 */
UINT WINAPI midiOutCachePatches(HMIDIOUT hMidiOut, UINT uBank,
				WORD* lpwPatchArray, UINT uFlags)
{
    /* not really necessary to support this */
    FIXME("(%p, %u, %p, %x): Stub\n", hMidiOut, uBank, lpwPatchArray, uFlags);
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[WINMM.@]
 */
UINT WINAPI midiOutCacheDrumPatches(HMIDIOUT hMidiOut, UINT uPatch,
				    WORD* lpwKeyArray, UINT uFlags)
{
    FIXME("(%p, %u, %p, %x): Stub\n", hMidiOut, uPatch, lpwKeyArray, uFlags);
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutGetID		[WINMM.@]
 */
UINT WINAPI midiOutGetID(HMIDIOUT hMidiOut, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p)\n", hMidiOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiOutMessage		[WINMM.@]
 */
UINT WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage,
                           DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %04X, %08IX, %08IX)\n", hMidiOut, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) {
	/* HACK... */
	if (uMessage == 0x0001) {
	    *(LPDWORD)dwParam1 = 1;
	    return 0;
	}
	if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, TRUE)) != NULL) {
	    return MMDRV_PhysicalFeatures(wmld, uMessage, dwParam1, dwParam2);
	}
	return MMSYSERR_INVALHANDLE;
    }

    switch (uMessage) {
    case MODM_OPEN:
    case MODM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2);
}

/**************************************************************************
 * 				midiInGetNumDevs	[WINMM.@]
 */
UINT WINAPI midiInGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_MIDIIN);
}

/**************************************************************************
 * 				midiInGetDevCapsW	[WINMM.@]
 */
UINT WINAPI midiInGetDevCapsW(UINT_PTR uDeviceID, LPMIDIINCAPSW lpCaps, UINT uSize)
{
    LPWINE_MLD	wmld;

    TRACE("(%Id, %p, %d);\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_MIDIIN, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;

   return MMDRV_Message(wmld, MIDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize);
}

/**************************************************************************
 * 				midiInGetDevCapsA	[WINMM.@]
 */
UINT WINAPI midiInGetDevCapsA(UINT_PTR uDeviceID, LPMIDIINCAPSA lpCaps, UINT uSize)
{
    MIDIINCAPSW		micW;
    UINT		ret;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    ret = midiInGetDevCapsW(uDeviceID, &micW, sizeof(micW));

    if (ret == MMSYSERR_NOERROR) {
	MIDIINCAPSA micA;
	micA.wMid           = micW.wMid;
	micA.wPid           = micW.wPid;
	micA.vDriverVersion = micW.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, micW.szPname, -1, micA.szPname,
                             sizeof(micA.szPname), NULL, NULL );
	micA.dwSupport      = micW.dwSupport;
	memcpy(lpCaps, &micA, min(uSize, sizeof(micA)));
    }
    return ret;
}

/**************************************************************************
 * 				midiInOpen		[WINMM.@]
 */
MMRESULT WINAPI midiInOpen(HMIDIIN* lphMidiIn, UINT uDeviceID,
		       DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD dwFlags)
{
    HANDLE		hMidiIn;
    LPWINE_MIDI		lpwm;
    MMRESULT		dwRet;

    TRACE("(%p, %d, %08IX, %08IX, %08lX);\n",
	  lphMidiIn, uDeviceID, dwCallback, dwInstance, dwFlags);

    if (lphMidiIn != NULL) *lphMidiIn = 0;

    dwRet = WINMM_CheckCallback(dwCallback, dwFlags, FALSE);
    if (dwRet != MMSYSERR_NOERROR)
	return dwRet;

    lpwm = (LPWINE_MIDI)MMDRV_Alloc(sizeof(WINE_MIDI), MMDRV_MIDIIN, &hMidiIn,
				    &dwFlags, &dwCallback, &dwInstance);

    if (lpwm == NULL)
	return MMSYSERR_NOMEM;

    lpwm->mod.hMidi = hMidiIn;
    lpwm->mod.dwCallback = dwCallback;
    lpwm->mod.dwInstance = dwInstance;

    lpwm->mld.uDeviceID = uDeviceID;
    dwRet = MMDRV_Open(&lpwm->mld, MIDM_OPEN, (DWORD_PTR)&lpwm->mod, dwFlags);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMidiIn, &lpwm->mld);
	hMidiIn = 0;
    }
    if (lphMidiIn != NULL) *lphMidiIn = hMidiIn;
    TRACE("=> %d hMidi=%p\n", dwRet, hMidiIn);

    return dwRet;
}

/**************************************************************************
 * 				midiInClose		[WINMM.@]
 */
UINT WINAPI midiInClose(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MIDM_CLOSE);
    MMDRV_Free(hMidiIn, wmld);
    return dwRet;
}

/**************************************************************************
 * 				midiInPrepareHeader	[WINMM.@]
 */
UINT WINAPI midiInPrepareHeader(HMIDIIN hMidiIn,
				MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_PREPARE, (DWORD_PTR)lpMidiInHdr, uSize);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[WINMM.@]
 */
UINT WINAPI midiInUnprepareHeader(HMIDIIN hMidiIn,
				  MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_UNPREPARE, (DWORD_PTR)lpMidiInHdr, uSize);
}

/**************************************************************************
 * 				midiInAddBuffer		[WINMM.@]
 */
UINT WINAPI midiInAddBuffer(HMIDIIN hMidiIn,
			    MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_ADDBUFFER, (DWORD_PTR)lpMidiInHdr, uSize);
}

/**************************************************************************
 * 				midiInStart			[WINMM.@]
 */
UINT WINAPI midiInStart(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_START, 0L, 0L);
}

/**************************************************************************
 * 				midiInStop			[WINMM.@]
 */
UINT WINAPI midiInStop(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_STOP, 0L, 0L);
}

/**************************************************************************
 * 				midiInReset			[WINMM.@]
 */
UINT WINAPI midiInReset(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_RESET, 0L, 0L);
}

/**************************************************************************
 * 				midiInGetID			[WINMM.@]
 */
UINT WINAPI midiInGetID(HMIDIIN hMidiIn, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p)\n", hMidiIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiInMessage		[WINMM.@]
 */
UINT WINAPI midiInMessage(HMIDIIN hMidiIn, UINT uMessage,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %04X, %08IX, %08IX)\n", hMidiIn, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    switch (uMessage) {
    case MIDM_OPEN:
    case MIDM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2);
}

/**************************************************************************
 * 				midiConnect			[WINMM.@]
 */
MMRESULT WINAPI midiConnect(HMIDI hMidi, HMIDIOUT hmo, LPVOID pReserved)
{
    FIXME("(%p, %p, %p): Stub\n", hMidi, hmo, pReserved);
    return MMSYSERR_ERROR;
}

/**************************************************************************
 * 				midiDisconnect			[WINMM.@]
 */
MMRESULT WINAPI midiDisconnect(HMIDI hMidi, HMIDIOUT hmo, LPVOID pReserved)
{
    FIXME("(%p, %p, %p): Stub\n", hMidi, hmo, pReserved);
    return MMSYSERR_ERROR;
}

typedef struct WINE_MIDIStream {
    HMIDIOUT			hDevice;
    HANDLE			hThread;
    DWORD			dwThreadID;
    CRITICAL_SECTION		lock;
    DWORD			dwTempo;
    DWORD			dwTimeDiv;
    ULONGLONG			position_usec;
    DWORD			dwPulses;
    DWORD			dwStartTicks;
    DWORD			dwElapsedMS;
    DWORD			dwLastPositionMS;
    WORD			wFlags;
    WORD			status;
    HANDLE			hEvent;
    LPMIDIHDR			lpMidiHdr;
    DWORD			dwStreamID;
    struct wine_rb_entry	entry;
} WINE_MIDIStream;

#define WINE_MSM_HEADER		(WM_USER+0)
#define WINE_MSM_STOP		(WM_USER+1)
#define WINE_MSM_PAUSE		(WM_USER+2)
#define WINE_MSM_RESUME		(WM_USER+3)

#define MSM_STATUS_STOPPED	WINE_MSM_STOP
#define MSM_STATUS_PAUSED	WINE_MSM_PAUSE
#define MSM_STATUS_PLAYING	WINE_MSM_RESUME

static int wine_midi_stream_compare(const void *key, const struct wine_rb_entry *entry)
{
    WINE_MIDIStream *stream = WINE_RB_ENTRY_VALUE(entry, struct WINE_MIDIStream, entry);
    return memcmp(key, &stream->dwStreamID, sizeof(stream->dwStreamID));
}

static WINE_MIDIStream *wine_midi_stream_allocate(void)
{
    DWORD stream_id = 1;
    WINE_MIDIStream *stream = NULL;

    EnterCriticalSection(&WINMM_cs);

    while (stream_id < 0xFFFFFFFF && wine_rb_get(&wine_midi_streams, &stream_id))
        stream_id++;

    if (stream_id < 0xFFFFFFFF &&
        (stream = malloc(sizeof(WINE_MIDIStream))))
    {
        stream->dwStreamID = stream_id;
        wine_rb_put(&wine_midi_streams, &stream_id, &stream->entry);
    }

    LeaveCriticalSection(&WINMM_cs);
    return stream;
}

static void wine_midi_stream_free(WINE_MIDIStream *stream)
{
    EnterCriticalSection(&WINMM_cs);

    wine_rb_remove(&wine_midi_streams, &stream->entry);
    free(stream);

    LeaveCriticalSection(&WINMM_cs);
}

/**************************************************************************
 * 				MMSYSTEM_GetMidiStream		[internal]
 */
static	BOOL	MMSYSTEM_GetMidiStream(HMIDISTRM hMidiStrm, WINE_MIDIStream** lpMidiStrm, WINE_MIDI** lplpwm)
{
    WINE_MIDI* lpwm = (LPWINE_MIDI)MMDRV_Get(hMidiStrm, MMDRV_MIDIOUT, FALSE);
    struct wine_rb_entry *entry;

    if (lplpwm)
	*lplpwm = lpwm;

    if (lpwm == NULL) {
	return FALSE;
    }

    EnterCriticalSection(&WINMM_cs);
    if ((entry = wine_rb_get(&wine_midi_streams, &lpwm->mod.rgIds.dwStreamID)))
        *lpMidiStrm = WINE_RB_ENTRY_VALUE(entry, struct WINE_MIDIStream, entry);
    LeaveCriticalSection(&WINMM_cs);

    return *lpMidiStrm != NULL;
}

/**************************************************************************
 * 				MMSYSTEM_MidiStream_Convert	[internal]
 */
static	DWORD	MMSYSTEM_MidiStream_Convert(WINE_MIDIStream* lpMidiStrm, DWORD pulse)
{
    DWORD	ret = 0;

    if (lpMidiStrm->dwTimeDiv == 0) {
	FIXME("Shouldn't happen. lpMidiStrm->dwTimeDiv = 0\n");
    } else if (lpMidiStrm->dwTimeDiv > 0x8000) { /* SMPTE, unchecked FIXME? */
	int	nf = 256 - HIBYTE(lpMidiStrm->dwTimeDiv);	/* number of frames     */
	int	nsf = LOBYTE(lpMidiStrm->dwTimeDiv);		/* number of sub-frames */
	ret = (pulse * 1000000) / (nf * nsf);
    } else {
	ret = (DWORD)((double)pulse * (double)lpMidiStrm->dwTempo /
		      (double)lpMidiStrm->dwTimeDiv);
    }

    return ret; /* in microseconds */
}

static DWORD midistream_get_playing_position(WINE_MIDIStream* lpMidiStrm)
{
    switch (lpMidiStrm->status) {
    case MSM_STATUS_STOPPED:
    case MSM_STATUS_PAUSED:
        return lpMidiStrm->dwElapsedMS;
    case MSM_STATUS_PLAYING:
        return timeGetTime() - lpMidiStrm->dwStartTicks;
    default:
        FIXME("Unknown playing status %hu\n", lpMidiStrm->status);
        return 0;
    }
}

/**************************************************************************
 * 			MMSYSTEM_MidiStream_MessageHandler	[internal]
 */
static	BOOL	MMSYSTEM_MidiStream_MessageHandler(WINE_MIDIStream* lpMidiStrm, LPWINE_MIDI lpwm, LPMSG msg)
{
    LPMIDIHDR	lpMidiHdr;
    LPMIDIHDR*	lpmh;
    LPBYTE	lpData;

    for (;;) {
        switch (msg->message) {
        case WM_QUIT:
            return FALSE;
        case WINE_MSM_STOP:
            TRACE("STOP\n");
            EnterCriticalSection(&lpMidiStrm->lock);
            lpMidiStrm->status = MSM_STATUS_STOPPED;
            lpMidiStrm->dwPulses = 0;
            lpMidiStrm->dwElapsedMS = 0;
            lpMidiStrm->position_usec = 0;
            lpMidiStrm->dwLastPositionMS = 0;
            LeaveCriticalSection(&lpMidiStrm->lock);
            /* this is not quite what MS doc says... */
            midiOutReset(lpMidiStrm->hDevice);
            /* empty list of already submitted buffers */
            lpMidiHdr = lpMidiStrm->lpMidiHdr;
            lpMidiStrm->lpMidiHdr = NULL;
            while (lpMidiHdr) {
                LPMIDIHDR lphdr = lpMidiHdr;
                lpMidiHdr = lpMidiHdr->lpNext;
                lphdr->dwFlags |= MHDR_DONE;
                lphdr->dwFlags &= ~MHDR_INQUEUE;

                DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
                               (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
                               lpwm->mod.dwInstance, (DWORD_PTR)lphdr, 0);
            }
            SetEvent((HANDLE)msg->wParam);
            return TRUE;
        case WINE_MSM_RESUME:
            /* FIXME: send out cc64 0 (turn off sustain pedal) on every channel */
            if (lpMidiStrm->status != MSM_STATUS_PLAYING) {
                EnterCriticalSection(&lpMidiStrm->lock);
                lpMidiStrm->dwStartTicks = timeGetTime() - lpMidiStrm->dwElapsedMS;
                lpMidiStrm->status = MSM_STATUS_PLAYING;
                LeaveCriticalSection(&lpMidiStrm->lock);
            }
            SetEvent((HANDLE)msg->wParam);
            return TRUE;
        case WINE_MSM_PAUSE:
            /* FIXME: send out cc64 0 (turn off sustain pedal) on every channel */
            if (lpMidiStrm->status != MSM_STATUS_PAUSED) {
                EnterCriticalSection(&lpMidiStrm->lock);
                lpMidiStrm->dwElapsedMS = timeGetTime() - lpMidiStrm->dwStartTicks;
                lpMidiStrm->status = MSM_STATUS_PAUSED;
                LeaveCriticalSection(&lpMidiStrm->lock);
            }
            SetEvent((HANDLE)msg->wParam);
            break;
	/* FIXME(EPP): "I don't understand the content of the first MIDIHDR sent
	 * by native mcimidi, it doesn't look like a correct one".
	 * this trick allows us to throw it away... but I don't like it.
	 * It looks like part of the file I'm trying to play and definitively looks
	 * like raw midi content.
	 * I'd really like to understand why native mcimidi sends it. Perhaps a bad
	 * synchronization issue where native mcimidi is still processing raw MIDI
	 * content before generating MIDIEVENTs ?
	 *
	 * 4c 04 89 3b 00 81 7c 99 3b 43 00 99 23 5e 04 89 L..;..|.;C..#^..
	 * 3b 00 00 89 23 00 7c 99 3b 45 00 99 28 62 04 89 ;...#.|.;E..(b..
	 * 3b 00 00 89 28 00 81 7c 99 3b 4e 00 99 23 5e 04 ;...(..|.;N..#^.
	 * 89 3b 00 00 89 23 00 7c 99 3b 45 00 99 23 78 04 .;...#.|.;E..#x.
	 * 89 3b 00 00 89 23 00 81 7c 99 3b 48 00 99 23 5e .;...#..|.;H..#^
	 * 04 89 3b 00 00 89 23 00 7c 99 3b 4e 00 99 28 62 ..;...#.|.;N..(b
	 * 04 89 3b 00 00 89 28 00 81 7c 99 39 4c 00 99 23 ..;...(..|.9L..#
	 * 5e 04 89 39 00 00 89 23 00 82 7c 99 3b 4c 00 99 ^..9...#..|.;L..
	 * 23 5e 04 89 3b 00 00 89 23 00 7c 99 3b 48 00 99 #^..;...#.|.;H..
	 * 28 62 04 89 3b 00 00 89 28 00 81 7c 99 3b 3f 04 (b..;...(..|.;?.
	 * 89 3b 00 1c 99 23 5e 04 89 23 00 5c 99 3b 45 00 .;...#^..#.\.;E.
	 * 99 23 78 04 89 3b 00 00 89 23 00 81 7c 99 3b 46 .#x..;...#..|.;F
	 * 00 99 23 5e 04 89 3b 00 00 89 23 00 7c 99 3b 48 ..#^..;...#.|.;H
	 * 00 99 28 62 04 89 3b 00 00 89 28 00 81 7c 99 3b ..(b..;...(..|.;
	 * 46 00 99 23 5e 04 89 3b 00 00 89 23 00 7c 99 3b F..#^..;...#.|.;
	 * 48 00 99 23 78 04 89 3b 00 00 89 23 00 81 7c 99 H..#x..;...#..|.
	 * 3b 4c 00 99 23 5e 04 89 3b 00 00 89 23 00 7c 99 ;L..#^..;...#.|.
	 */
        case WINE_MSM_HEADER:
            /* sets initial tick count for first MIDIHDR */
            if (!lpMidiStrm->dwStartTicks)
                lpMidiStrm->dwStartTicks = timeGetTime();
            lpMidiHdr = (LPMIDIHDR)msg->lParam;
            lpData = (LPBYTE)lpMidiHdr->lpData;
            TRACE("Adding %s lpMidiHdr=%p [lpData=0x%p dwBytesRecorded=%lu/%lu dwFlags=0x%08lx size=%Iu]\n",
                  (lpMidiHdr->dwFlags & MHDR_ISSTRM) ? "stream" : "regular", lpMidiHdr,
                  lpData, lpMidiHdr->dwBytesRecorded, lpMidiHdr->dwBufferLength,
                  lpMidiHdr->dwFlags, msg->wParam);
#if 0
            /* dumps content of lpMidiHdr->lpData
             * FIXME: there should be a debug routine somewhere that already does this
             */
            for (dwToGo = 0; dwToGo < lpMidiHdr->dwBufferLength; dwToGo += 16) {
                DWORD       i;
                BYTE        ch;

                for (i = 0; i < min(16, lpMidiHdr->dwBufferLength - dwToGo); i++)
                    printf("%02x ", lpData[dwToGo + i]);
                for (; i < 16; i++)
                    printf("   ");
                for (i = 0; i < min(16, lpMidiHdr->dwBufferLength - dwToGo); i++) {
                    ch = lpData[dwToGo + i];
                    printf("%c", (ch >= 0x20 && ch <= 0x7F) ? ch : '.');
                }
                printf("\n");
            }
#endif
            if (((LPMIDIEVENT)lpData)->dwStreamID != 0 &&
                ((LPMIDIEVENT)lpData)->dwStreamID != 0xFFFFFFFF &&
                ((LPMIDIEVENT)lpData)->dwStreamID != lpMidiStrm->dwStreamID) {
                FIXME("Dropping bad %s lpMidiHdr (streamID=%08lx)\n",
                      (lpMidiHdr->dwFlags & MHDR_ISSTRM) ? "stream" : "regular",
                      ((LPMIDIEVENT)lpData)->dwStreamID);
                lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
                lpMidiHdr->dwFlags |= MHDR_DONE;

                DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
                               (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
                               lpwm->mod.dwInstance, (DWORD_PTR)lpMidiHdr, 0);
                break;
            }

            lpMidiHdr->lpNext = 0;
            for (lpmh = &lpMidiStrm->lpMidiHdr; *lpmh; lpmh = &(*lpmh)->lpNext);
            *lpmh = lpMidiHdr;
            break;
        default:
            FIXME("Unknown message %d\n", msg->message);
            break;
        }
        if (lpMidiStrm->status != MSM_STATUS_PAUSED)
            return TRUE;
        GetMessageA(msg, 0, 0, 0);
    }
}

/**************************************************************************
 * 				MMSYSTEM_MidiStream_Player	[internal]
 */
static	DWORD	CALLBACK	MMSYSTEM_MidiStream_Player(LPVOID pmt)
{
    WINE_MIDIStream* 	lpMidiStrm = pmt;
    WINE_MIDI*		lpwm;
    MSG			msg;
    DWORD		dwToGo;
    DWORD		dwCurrTC;
    LPMIDIHDR		lpMidiHdr;
    DWORD		dwOffset;

    TRACE("(%p)!\n", lpMidiStrm);

    if (!lpMidiStrm ||
	(lpwm = (LPWINE_MIDI)MMDRV_Get(lpMidiStrm->hDevice, MMDRV_MIDIOUT, FALSE)) == NULL)
	goto the_end;

    /* force thread's queue creation */
    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);

    lpMidiStrm->dwStartTicks = 0;
    lpMidiStrm->dwPulses = 0;

    lpMidiStrm->lpMidiHdr = 0;

    /* midiStreamOpen is waiting for ack */
    SetEvent(lpMidiStrm->hEvent);

start_header:
    lpMidiHdr = lpMidiStrm->lpMidiHdr;
    if (!lpMidiHdr) {
	/* for first message, block until one arrives, then process all that are available */
	GetMessageA(&msg, 0, 0, 0);
	do {
	    if (!MMSYSTEM_MidiStream_MessageHandler(lpMidiStrm, lpwm, &msg))
		goto the_end;
	} while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE));
	goto start_header;
    }

    dwOffset = 0;
    while (dwOffset + offsetof(MIDIEVENT,dwParms) <= lpMidiHdr->dwBytesRecorded) {
	LPMIDIEVENT me = (LPMIDIEVENT)(lpMidiHdr->lpData+dwOffset);

	/* do we have to wait ? */
	if (me->dwDeltaTime) {
	    EnterCriticalSection(&lpMidiStrm->lock);
	    lpMidiStrm->position_usec += MMSYSTEM_MidiStream_Convert(lpMidiStrm, me->dwDeltaTime);
	    LeaveCriticalSection(&lpMidiStrm->lock);

	    dwToGo = lpMidiStrm->dwStartTicks + lpMidiStrm->position_usec / 1000;

	    TRACE("%lu/%lu/%lu\n", dwToGo, timeGetTime(), me->dwDeltaTime);
	    while (dwToGo - (dwCurrTC = timeGetTime()) <= MAXLONG) {
		if (MsgWaitForMultipleObjects(0, NULL, FALSE, dwToGo - dwCurrTC, QS_ALLINPUT) == WAIT_OBJECT_0) {
		    /* got a message, handle it */
		    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
			if (!MMSYSTEM_MidiStream_MessageHandler(lpMidiStrm, lpwm, &msg))
			    goto the_end;
			/* is lpMidiHdr still current? */
			if (lpMidiHdr != lpMidiStrm->lpMidiHdr) {
			    goto start_header;
			}
		    }
		    /* reset dwToGo because dwStartTicks might be updated */
		    dwToGo = lpMidiStrm->dwStartTicks + lpMidiStrm->position_usec / 1000;
		} else {
		    /* timeout, so me->dwDeltaTime is elapsed, can break the while loop */
		    break;
		}
	    }
	    EnterCriticalSection(&lpMidiStrm->lock);
	    lpMidiStrm->dwPulses += me->dwDeltaTime;
	    lpMidiStrm->dwLastPositionMS = midistream_get_playing_position(lpMidiStrm);
	    LeaveCriticalSection(&lpMidiStrm->lock);
	}
	switch (MEVT_EVENTTYPE(me->dwEvent & ~MEVT_F_CALLBACK)) {
	case MEVT_COMMENT:
	    FIXME("NIY: MEVT_COMMENT\n");
	    /* do nothing, skip bytes */
	    break;
	case MEVT_LONGMSG:
        {
            MIDIHDR mh;
            memset(&mh, 0, sizeof(mh));
            mh.lpData = (LPSTR)me->dwParms;
            mh.dwBufferLength = MEVT_EVENTPARM(me->dwEvent);
            midiOutPrepareHeader(lpMidiStrm->hDevice, &mh, sizeof(mh));
            midiOutLongMsg(lpMidiStrm->hDevice, &mh, sizeof(mh));
            midiOutUnprepareHeader(lpMidiStrm->hDevice, &mh, sizeof(mh));
            break;
        }
	case MEVT_NOP:
	    break;
	case MEVT_SHORTMSG:
	    midiOutShortMsg(lpMidiStrm->hDevice, MEVT_EVENTPARM(me->dwEvent));
	    break;
	case MEVT_TEMPO:
	    EnterCriticalSection(&lpMidiStrm->lock);
	    lpMidiStrm->dwTempo = MEVT_EVENTPARM(me->dwEvent);
	    LeaveCriticalSection(&lpMidiStrm->lock);
	    break;
	case MEVT_VERSION:
	    break;
	default:
	    FIXME("Unknown MEVT (0x%02x)\n", MEVT_EVENTTYPE(me->dwEvent & ~MEVT_F_CALLBACK));
	    break;
	}
	if (me->dwEvent & MEVT_F_CALLBACK) {
	    /* native fills dwOffset regardless of the cbMidiHdr size argument to midiStreamOut */
	    lpMidiHdr->dwOffset = dwOffset;
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_POSITIONCB,
			   lpwm->mod.dwInstance, (LPARAM)lpMidiHdr, 0L);
	}
	dwOffset += offsetof(MIDIEVENT,dwParms);
	if (me->dwEvent & MEVT_F_LONG)
	    dwOffset += (MEVT_EVENTPARM(me->dwEvent) + 3) & ~3;
    }
    /* done with this header */
    lpMidiStrm->lpMidiHdr = lpMidiHdr->lpNext;
    lpMidiHdr->dwFlags |= MHDR_DONE;
    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;

    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
		   (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
		   lpwm->mod.dwInstance, (DWORD_PTR)lpMidiHdr, 0);
    goto start_header;

the_end:
    TRACE("End of thread\n");
    return 0;
}

/**************************************************************************
 * 				midiStreamClose			[WINMM.@]
 */
MMRESULT WINAPI midiStreamClose(HMIDISTRM hMidiStrm)
{
    WINE_MIDI*		lpwm;
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret = 0;

    TRACE("(%p)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, &lpwm))
	return MMSYSERR_INVALHANDLE;

    midiStreamStop(hMidiStrm);
    PostThreadMessageA(lpMidiStrm->dwThreadID, WM_QUIT, 0, 0);
    CloseHandle(lpMidiStrm->hEvent);
    if (lpMidiStrm->hThread) {
        if (GetCurrentThreadId() != lpMidiStrm->dwThreadID)
            WaitForSingleObject(lpMidiStrm->hThread, INFINITE);
        else {
            FIXME("leak from call within function callback\n");
            ret = MMSYSERR_HANDLEBUSY; /* yet don't signal it to app */
        }
        CloseHandle(lpMidiStrm->hThread);
    }
    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
                   (HDRVR)lpMidiStrm->hDevice, MM_MOM_CLOSE,
                   lpwm->mod.dwInstance, 0, 0);
    if(!ret) {
        lpMidiStrm->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&lpMidiStrm->lock);
        wine_midi_stream_free(lpMidiStrm);
    }

    return midiOutClose((HMIDIOUT)hMidiStrm);
}

/**************************************************************************
 * 				midiStreamOpen			[WINMM.@]
 */
MMRESULT WINAPI midiStreamOpen(HMIDISTRM* lphMidiStrm, LPUINT lpuDeviceID,
			       DWORD cMidi, DWORD_PTR dwCallback,
			       DWORD_PTR dwInstance, DWORD fdwOpen)
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret;
    MIDIOPENSTRMID	mosm;
    LPWINE_MIDI		lpwm;
    HMIDIOUT		hMidiOut;

    TRACE("(%p, %p, %ld, 0x%08Ix, 0x%08Ix, 0x%08lx)!\n",
	  lphMidiStrm, lpuDeviceID, cMidi, dwCallback, dwInstance, fdwOpen);

    if (cMidi != 1 || lphMidiStrm == NULL || lpuDeviceID == NULL)
	return MMSYSERR_INVALPARAM;

    ret = WINMM_CheckCallback(dwCallback, fdwOpen, FALSE);
    if (ret != MMSYSERR_NOERROR)
	return ret;

    lpMidiStrm = wine_midi_stream_allocate();
    if (!lpMidiStrm)
	return MMSYSERR_NOMEM;

    lpMidiStrm->dwTempo = 500000;  /* microseconds per quarter note, i.e. 120 BPM */
    lpMidiStrm->dwTimeDiv = 24;    /* ticks per quarter note */
    lpMidiStrm->position_usec = 0;
    lpMidiStrm->dwLastPositionMS = 0;
    lpMidiStrm->status = MSM_STATUS_PAUSED;
    lpMidiStrm->dwElapsedMS = 0;

    mosm.dwStreamID = lpMidiStrm->dwStreamID;
    /* FIXME: the correct value is not allocated yet for MAPPER */
    mosm.wDeviceID  = *lpuDeviceID;
    lpwm = MIDI_OutAlloc(&hMidiOut, &dwCallback, &dwInstance, &fdwOpen, 1, &mosm);
    if (!lpwm) {
	free(lpMidiStrm);
	return MMSYSERR_NOMEM;
    }
    lpMidiStrm->hDevice = hMidiOut;
    *lphMidiStrm = (HMIDISTRM)hMidiOut;

    lpwm->mld.uDeviceID = *lpuDeviceID;

    /* don't rely on midiOut callbacks */
    ret = MMDRV_Open(&lpwm->mld, MODM_OPEN, (DWORD_PTR)&lpwm->mod, CALLBACK_NULL);
    if (ret != MMSYSERR_NOERROR) {
	MMDRV_Free(hMidiOut, &lpwm->mld);
	free(lpMidiStrm);
	return ret;
    }

    InitializeCriticalSection(&lpMidiStrm->lock);
    lpMidiStrm->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": WINMM_MidiStream.lock");

    lpMidiStrm->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    lpMidiStrm->wFlags = HIWORD(fdwOpen);

    lpMidiStrm->hThread = CreateThread(NULL, 0, MMSYSTEM_MidiStream_Player,
				       lpMidiStrm, 0, &(lpMidiStrm->dwThreadID));

    if (!lpMidiStrm->hThread) {
	midiStreamClose((HMIDISTRM)hMidiOut);
	return MMSYSERR_NOMEM;
    }
    SetThreadPriority(lpMidiStrm->hThread, THREAD_PRIORITY_TIME_CRITICAL);

    /* wait for thread to have started, and for its queue to be created */
    WaitForSingleObject(lpMidiStrm->hEvent, INFINITE);

    TRACE("=> (%u/%d) hMidi=%p ret=%d lpMidiStrm=%p\n",
	  *lpuDeviceID, lpwm->mld.uDeviceID, *lphMidiStrm, ret, lpMidiStrm);

    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
                   (HDRVR)lpMidiStrm->hDevice, MM_MOM_OPEN,
                   lpwm->mod.dwInstance, 0, 0);
    return ret;
}

/**************************************************************************
 * 				midiStreamOut			[WINMM.@]
 */
MMRESULT WINAPI midiStreamOut(HMIDISTRM hMidiStrm, LPMIDIHDR lpMidiHdr,
			      UINT cbMidiHdr)
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %u)!\n", hMidiStrm, lpMidiHdr, cbMidiHdr);

    if (cbMidiHdr < offsetof(MIDIHDR,dwOffset) || !lpMidiHdr || !lpMidiHdr->lpData
	|| lpMidiHdr->dwBufferLength < lpMidiHdr->dwBytesRecorded
	|| lpMidiHdr->dwBytesRecorded % 4 /* player expects DWORD padding */)
	return MMSYSERR_INVALPARAM;
    /* FIXME: Native additionally checks if the MIDIEVENTs in lpData
     * exactly fit dwBytesRecorded. */

    if (!(lpMidiHdr->dwFlags & MHDR_PREPARED))
	return MIDIERR_UNPREPARED;

    if (lpMidiHdr->dwFlags & MHDR_INQUEUE)
	return MIDIERR_STILLPLAYING;

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else {
	lpMidiHdr->dwFlags |= MHDR_ISSTRM | MHDR_INQUEUE;
	lpMidiHdr->dwFlags &= ~MHDR_DONE;
	if (!PostThreadMessageA(lpMidiStrm->dwThreadID,
                                WINE_MSM_HEADER, cbMidiHdr,
                                (LPARAM)lpMidiHdr)) {
	    ERR("bad PostThreadMessageA\n");
	    ret = MMSYSERR_ERROR;
	}
    }
    return ret;
}

static MMRESULT midistream_post_message_and_wait(WINE_MIDIStream* lpMidiStrm, UINT msg, LPARAM lParam)
{
    HANDLE hObjects[2];

    hObjects[0] = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!hObjects[0])
        return MMSYSERR_ERROR;

    if (!PostThreadMessageA(lpMidiStrm->dwThreadID, msg, (WPARAM)hObjects[0], lParam)) {
        WARN("bad PostThreadMessage\n");
        CloseHandle(hObjects[0]);
        return MMSYSERR_ERROR;
    }

    if (GetCurrentThreadId() != lpMidiStrm->dwThreadID) {
        DWORD ret;
        hObjects[1] = lpMidiStrm->hThread;
        ret = WaitForMultipleObjects(ARRAY_SIZE(hObjects), hObjects, FALSE, INFINITE);
        if (ret != WAIT_OBJECT_0) {
            CloseHandle(hObjects[0]);
            WARN("bad WaitForSingleObject (%lu)\n", ret);
            return MMSYSERR_ERROR;
        }
    }

    CloseHandle(hObjects[0]);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiStreamPause			[WINMM.@]
 */
MMRESULT WINAPI midiStreamPause(HMIDISTRM hMidiStrm)
{
    WINE_MIDIStream*	lpMidiStrm;

    TRACE("(%p)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL))
        return MMSYSERR_INVALHANDLE;
    return midistream_post_message_and_wait(lpMidiStrm, WINE_MSM_PAUSE, 0);
}

static DWORD midistream_get_current_pulse(WINE_MIDIStream* lpMidiStrm)
{
    DWORD pulses = 0;
    DWORD now = midistream_get_playing_position(lpMidiStrm);
    DWORD delta = now - lpMidiStrm->dwLastPositionMS;
    if (lpMidiStrm->dwTimeDiv > 0x8000) {
        /* SMPTE, unchecked FIXME */
        BYTE nf = 256 - HIBYTE(lpMidiStrm->dwTimeDiv);
        BYTE nsf = LOBYTE(lpMidiStrm->dwTimeDiv);
        pulses = (delta * nf * nsf) / 1000;
    }
    else if (lpMidiStrm->dwTimeDiv) {
        pulses = (DWORD)((double)(delta * lpMidiStrm->dwTimeDiv) *
                         1000.0 / (double)lpMidiStrm->dwTempo);
    }
    return lpMidiStrm->dwPulses + pulses;
}

/**************************************************************************
 * 				midiStreamPosition		[WINMM.@]
 */
MMRESULT WINAPI midiStreamPosition(HMIDISTRM hMidiStrm, LPMMTIME lpMMT, UINT cbmmt)
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %u)!\n", hMidiStrm, lpMMT, cbmmt);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else if (lpMMT == NULL || cbmmt != sizeof(MMTIME)) {
	ret = MMSYSERR_INVALPARAM;
    } else {
	EnterCriticalSection(&lpMidiStrm->lock);
	switch (lpMMT->wType) {
	case TIME_MIDI:
	    if (lpMidiStrm->dwTimeDiv < 0x8000) {
		DWORD tdiv, pulses;
		tdiv = (lpMidiStrm->dwTimeDiv > 24) ? lpMidiStrm->dwTimeDiv : 24;
		pulses = midistream_get_current_pulse(lpMidiStrm);
		lpMMT->u.midi.songptrpos = (pulses + tdiv/8) / (tdiv/4);
		if (!lpMMT->u.midi.songptrpos && pulses) lpMMT->u.midi.songptrpos++;
		TRACE("=> song position %ld (pulses %lu, tdiv %lu)\n", lpMMT->u.midi.songptrpos, pulses, tdiv);
		break;
	    }
	/* fall through */
	case TIME_BYTES:
	case TIME_SAMPLES:
	    lpMMT->wType = TIME_MS;
	    /* fall through to alternative format */
	case TIME_MS:
	    lpMMT->u.ms = midistream_get_playing_position(lpMidiStrm);
	    TRACE("=> %ld ms\n", lpMMT->u.ms);
	    break;
	case TIME_TICKS:
	    lpMMT->u.ticks = midistream_get_current_pulse(lpMidiStrm);
	    TRACE("=> %ld ticks\n", lpMMT->u.ticks);
	    break;
	default:
	    FIXME("Unsupported time type %x\n", lpMMT->wType);
	    /* use TIME_MS instead */
	    lpMMT->wType = TIME_MS;
	    lpMMT->u.ms = midistream_get_playing_position(lpMidiStrm);
	    TRACE("=> %ld ms\n", lpMMT->u.ms);
	    break;
	}
	LeaveCriticalSection(&lpMidiStrm->lock);
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamProperty		[WINMM.@]
 */
MMRESULT WINAPI midiStreamProperty(HMIDISTRM hMidiStrm, LPBYTE lpPropData, DWORD dwProperty)
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %lx)\n", hMidiStrm, lpPropData, dwProperty);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else if ((dwProperty & (MIDIPROP_GET|MIDIPROP_SET)) == 0) {
	ret = MMSYSERR_INVALPARAM;
    } else if (dwProperty & MIDIPROP_TEMPO) {
	MIDIPROPTEMPO*	mpt = (MIDIPROPTEMPO*)lpPropData;

	EnterCriticalSection(&lpMidiStrm->lock);
	if (sizeof(MIDIPROPTEMPO) != mpt->cbStruct) {
	    ret = MMSYSERR_INVALPARAM;
	} else if (dwProperty & MIDIPROP_SET) {
	    lpMidiStrm->dwTempo = mpt->dwTempo;
	    TRACE("Setting tempo to %ld\n", mpt->dwTempo);
	} else if (dwProperty & MIDIPROP_GET) {
	    mpt->dwTempo = lpMidiStrm->dwTempo;
	    TRACE("Getting tempo <= %ld\n", mpt->dwTempo);
	}
	LeaveCriticalSection(&lpMidiStrm->lock);
    } else if (dwProperty & MIDIPROP_TIMEDIV) {
	MIDIPROPTIMEDIV*	mptd = (MIDIPROPTIMEDIV*)lpPropData;

	if (sizeof(MIDIPROPTIMEDIV) != mptd->cbStruct) {
	    ret = MMSYSERR_INVALPARAM;
	} else if (dwProperty & MIDIPROP_SET) {
	    EnterCriticalSection(&lpMidiStrm->lock);
	    if (lpMidiStrm->status != MSM_STATUS_PLAYING) {
		lpMidiStrm->dwTimeDiv = mptd->dwTimeDiv;
		TRACE("Setting time div to %ld\n", mptd->dwTimeDiv);
	    }
	    else
		ret = MMSYSERR_INVALPARAM;
	    LeaveCriticalSection(&lpMidiStrm->lock);
	} else if (dwProperty & MIDIPROP_GET) {
	    mptd->dwTimeDiv = lpMidiStrm->dwTimeDiv;
	    TRACE("Getting time div <= %ld\n", mptd->dwTimeDiv);
	}
    } else {
	ret = MMSYSERR_INVALPARAM;
    }

    return ret;
}

/**************************************************************************
 * 				midiStreamRestart		[WINMM.@]
 */
MMRESULT WINAPI midiStreamRestart(HMIDISTRM hMidiStrm)
{
    WINE_MIDIStream*	lpMidiStrm;

    TRACE("(%p)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL))
        return MMSYSERR_INVALHANDLE;
    return midistream_post_message_and_wait(lpMidiStrm, WINE_MSM_RESUME, 0);
}

/**************************************************************************
 * 				midiStreamStop			[WINMM.@]
 */
MMRESULT WINAPI midiStreamStop(HMIDISTRM hMidiStrm)
{
    WINE_MIDIStream*	lpMidiStrm;

    TRACE("(%p)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL))
        return MMSYSERR_INVALHANDLE;
    return midistream_post_message_and_wait(lpMidiStrm, WINE_MSM_STOP, 0);
}

struct mm_starter
{
    LPTASKCALLBACK      cb;
    DWORD               client;
    HANDLE              event;
};

static DWORD WINAPI mmTaskRun(void* pmt)
{
    struct mm_starter mms;

    memcpy(&mms, pmt, sizeof(struct mm_starter));
    free(pmt);
    mms.cb(mms.client);
    if (mms.event) SetEvent(mms.event);
    return 0;
}

/******************************************************************
 *		mmTaskCreate (WINMM.@)
 */
UINT     WINAPI mmTaskCreate(LPTASKCALLBACK cb, HANDLE* ph, DWORD_PTR client)
{
    HANDLE               hThread;
    HANDLE               hEvent = 0;
    struct mm_starter   *mms;

    mms = malloc(sizeof(struct mm_starter));
    if (mms == NULL) return TASKERR_OUTOFMEMORY;

    mms->cb = cb;
    mms->client = client;
    if (ph) hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    mms->event = hEvent;

    hThread = CreateThread(0, 0, mmTaskRun, mms, 0, NULL);
    if (!hThread) {
        free(mms);
        if (hEvent) CloseHandle(hEvent);
        return TASKERR_OUTOFMEMORY;
    }
    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
    if (ph) *ph = hEvent;
    CloseHandle(hThread);
    return 0;
}

/******************************************************************
 *		mmTaskBlock (WINMM.@)
 */
VOID     WINAPI mmTaskBlock(DWORD tid)
{
    MSG		msg;

    do
    {
	GetMessageA(&msg, 0, 0, 0);
	if (msg.hwnd) DispatchMessageA(&msg);
    } while (msg.message != WM_USER);
}

/******************************************************************
 *		mmTaskSignal (WINMM.@)
 */
BOOL     WINAPI mmTaskSignal(DWORD tid)
{
    return PostThreadMessageW(tid, WM_USER, 0, 0);
}

/******************************************************************
 *		mmTaskYield (WINMM.@)
 */
VOID     WINAPI mmTaskYield(VOID) {}

/******************************************************************
 *		mmGetCurrentTask (WINMM.@)
 */
DWORD    WINAPI mmGetCurrentTask(VOID)
{
    return GetCurrentThreadId();
}
