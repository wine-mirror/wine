/*
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winuser.h"
#include "winnls.h"
#include "winternl.h"
#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

static UINT WAVE_Open(HANDLE* lphndl, UINT uDeviceID, UINT uType,
                      LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
                      DWORD_PTR dwInstance, DWORD dwFlags)
{
    HANDLE		handle;
    LPWINE_MLD		wmld;
    DWORD		dwRet;
    WAVEOPENDESC	wod;

    TRACE("(%p, %d, %s, %p, %08lX, %08lX, %08X);\n",
	  lphndl, (int)uDeviceID, (uType==MMDRV_WAVEOUT)?"Out":"In", lpFormat, dwCallback,
	  dwInstance, dwFlags);

    if (dwFlags & WAVE_FORMAT_QUERY)
        TRACE("WAVE_FORMAT_QUERY requested !\n");

    dwRet = WINMM_CheckCallback(dwCallback, dwFlags, FALSE);
    if (dwRet != MMSYSERR_NOERROR)
        return dwRet;

    if (lpFormat == NULL) {
        WARN("bad format\n");
        return WAVERR_BADFORMAT;
    }

    if ((dwFlags & WAVE_MAPPED) && (uDeviceID == (UINT)-1)) {
        WARN("invalid parameter\n");
	return MMSYSERR_INVALPARAM;
    }

    /* may have a PCMWAVEFORMAT rather than a WAVEFORMATEX so don't read cbSize */
    TRACE("wFormatTag=%u, nChannels=%u, nSamplesPerSec=%u, nAvgBytesPerSec=%u, nBlockAlign=%u, wBitsPerSample=%u\n",
	  lpFormat->wFormatTag, lpFormat->nChannels, lpFormat->nSamplesPerSec,
	  lpFormat->nAvgBytesPerSec, lpFormat->nBlockAlign, lpFormat->wBitsPerSample);

    if ((wmld = MMDRV_Alloc(sizeof(WINE_WAVE), uType, &handle,
			    &dwFlags, &dwCallback, &dwInstance)) == NULL) {
	return MMSYSERR_NOMEM;
    }

    wod.hWave = handle;
    wod.lpFormat = (LPWAVEFORMATEX)lpFormat;  /* should the struct be copied iso pointer? */
    wod.dwCallback = dwCallback;
    wod.dwInstance = dwInstance;
    wod.dnDevNode = 0L;

    TRACE("cb=%08lx\n", wod.dwCallback);

    for (;;) {
        if (dwFlags & WAVE_MAPPED) {
            wod.uMappedDeviceID = uDeviceID;
            uDeviceID = WAVE_MAPPER;
        } else {
            wod.uMappedDeviceID = -1;
        }
        wmld->uDeviceID = uDeviceID;
    
        dwRet = MMDRV_Open(wmld, (uType == MMDRV_WAVEOUT) ? WODM_OPEN : WIDM_OPEN,
                           (DWORD_PTR)&wod, dwFlags);

        TRACE("dwRet = %s\n", WINMM_ErrorToString(dwRet));
        if (dwRet != WAVERR_BADFORMAT ||
            ((dwFlags & (WAVE_MAPPED|WAVE_FORMAT_DIRECT)) != 0) || (uDeviceID == WAVE_MAPPER)) break;
        /* if we ask for a format which isn't supported by the physical driver, 
         * let's try to map it through the wave mapper (except, if we already tried
         * or user didn't allow us to use acm codecs or the device is already the mapper)
         */
        dwFlags |= WAVE_MAPPED;
        /* we shall loop only one */
    }

    if ((dwFlags & WAVE_FORMAT_QUERY) || dwRet != MMSYSERR_NOERROR) {
        MMDRV_Free(handle, wmld);
        handle = 0;
    }

    if (lphndl != NULL) *lphndl = handle;
    TRACE("=> %s hWave=%p\n", WINMM_ErrorToString(dwRet), handle);

    return dwRet;
}

/**************************************************************************
 * 				waveOutGetNumDevs		[WINMM.@]
 */
UINT WINAPI waveOutGetNumDevs(void)
{
    TRACE("()\n");
    return 0;
}

/**************************************************************************
 * 				waveOutGetDevCapsA		[WINMM.@]
 */
UINT WINAPI waveOutGetDevCapsA(UINT_PTR uDeviceID, LPWAVEOUTCAPSA lpCaps,
			       UINT uSize)
{
    WAVEOUTCAPSW	wocW;
    UINT 		ret;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    ret = waveOutGetDevCapsW(uDeviceID, &wocW, sizeof(wocW));

    if (ret == MMSYSERR_NOERROR) {
	WAVEOUTCAPSA wocA;
	wocA.wMid           = wocW.wMid;
	wocA.wPid           = wocW.wPid;
	wocA.vDriverVersion = wocW.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, wocW.szPname, -1, wocA.szPname,
                             sizeof(wocA.szPname), NULL, NULL );
	wocA.dwFormats      = wocW.dwFormats;
	wocA.wChannels      = wocW.wChannels;
	wocA.dwSupport      = wocW.dwSupport;
	memcpy(lpCaps, &wocA, min(uSize, sizeof(wocA)));
    }
    return ret;
}

/**************************************************************************
 * 				waveOutGetDevCapsW		[WINMM.@]
 */
UINT WINAPI waveOutGetDevCapsW(UINT_PTR uDeviceID, LPWAVEOUTCAPSW lpCaps,
			       UINT uSize)
{
    TRACE("(%lu %p %u)\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    return MMSYSERR_BADDEVICEID;
}

/**************************************************************************
 * 				waveOutGetErrorTextA 	[WINMM.@]
 * 				waveInGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI waveOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    UINT	ret;

    if (lpText == NULL) ret = MMSYSERR_INVALPARAM;
    else if (uSize == 0) ret = MMSYSERR_NOERROR;
    else
    {
        LPWSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize * sizeof(WCHAR));
        if (!xstr) ret = MMSYSERR_NOMEM;
        else
        {
            ret = waveOutGetErrorTextW(uError, xstr, uSize);
            if (ret == MMSYSERR_NOERROR)
                WideCharToMultiByte(CP_ACP, 0, xstr, -1, lpText, uSize, NULL, NULL);
            HeapFree(GetProcessHeap(), 0, xstr);
        }
    }
    return ret;
}

/**************************************************************************
 * 				waveOutGetErrorTextW 	[WINMM.@]
 * 				waveInGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI waveOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    UINT        ret = MMSYSERR_BADERRNUM;

    if (lpText == NULL) ret = MMSYSERR_INVALPARAM;
    else if (uSize == 0) ret = MMSYSERR_NOERROR;
    else if (
	       /* test has been removed because MMSYSERR_BASE is 0, and gcc did emit
		* a warning for the test was always true */
	       (/*uError >= MMSYSERR_BASE && */ uError <= MMSYSERR_LASTERROR) ||
	       (uError >= WAVERR_BASE  && uError <= WAVERR_LASTERROR)) {
	if (LoadStringW(hWinMM32Instance,
			uError, lpText, uSize) > 0) {
	    ret = MMSYSERR_NOERROR;
	}
    }
    return ret;
}

/**************************************************************************
 *			waveOutOpen			[WINMM.@]
 * All the args/structs have the same layout as the win16 equivalents
 */
MMRESULT WINAPI waveOutOpen(LPHWAVEOUT lphWaveOut, UINT uDeviceID,
                       LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
                       DWORD_PTR dwInstance, DWORD dwFlags)
{
    TRACE("(%p, %u, %p, %lx, %lx, %08x)\n", lphWaveOut, uDeviceID, lpFormat,
            dwCallback, dwInstance, dwFlags);

    return MMSYSERR_BADDEVICEID;
}

/**************************************************************************
 * 				waveOutClose		[WINMM.@]
 */
UINT WINAPI waveOutClose(HWAVEOUT hWaveOut)
{
    TRACE("(%p)\n", hWaveOut);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutPrepareHeader	[WINMM.@]
 */
UINT WINAPI waveOutPrepareHeader(HWAVEOUT hWaveOut,
				 WAVEHDR* lpWaveOutHdr, UINT uSize)
{
    TRACE("(%p, %p, %u)\n", hWaveOut, lpWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[WINMM.@]
 */
UINT WINAPI waveOutUnprepareHeader(HWAVEOUT hWaveOut,
				   LPWAVEHDR lpWaveOutHdr, UINT uSize)
{
    TRACE("(%p, %p, %u)\n", hWaveOut, lpWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;
    
    if (!(lpWaveOutHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutWrite		[WINMM.@]
 */
UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, LPWAVEHDR lpWaveOutHdr,
			 UINT uSize)
{
    TRACE("(%p, %p, %u)\n", hWaveOut, lpWaveOutHdr, uSize);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutBreakLoop	[WINMM.@]
 */
UINT WINAPI waveOutBreakLoop(HWAVEOUT hWaveOut)
{
    TRACE("(%p)\n", hWaveOut);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutPause		[WINMM.@]
 */
UINT WINAPI waveOutPause(HWAVEOUT hWaveOut)
{
    TRACE("(%p)\n", hWaveOut);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutReset		[WINMM.@]
 */
UINT WINAPI waveOutReset(HWAVEOUT hWaveOut)
{
    TRACE("(%p)\n", hWaveOut);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutRestart		[WINMM.@]
 */
UINT WINAPI waveOutRestart(HWAVEOUT hWaveOut)
{
    TRACE("(%p)\n", hWaveOut);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutGetPosition	[WINMM.@]
 */
UINT WINAPI waveOutGetPosition(HWAVEOUT hWaveOut, LPMMTIME lpTime,
			       UINT uSize)
{
    TRACE("(%p, %p, %u)\n", hWaveOut, lpTime, uSize);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutGetPitch		[WINMM.@]
 */
UINT WINAPI waveOutGetPitch(HWAVEOUT hWaveOut, LPDWORD lpdw)
{
    TRACE("(%p, %p)\n", hWaveOut, lpdw);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutSetPitch		[WINMM.@]
 */
UINT WINAPI waveOutSetPitch(HWAVEOUT hWaveOut, DWORD dw)
{
    TRACE("(%p, %08x)\n", hWaveOut, dw);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutGetPlaybackRate	[WINMM.@]
 */
UINT WINAPI waveOutGetPlaybackRate(HWAVEOUT hWaveOut, LPDWORD lpdw)
{
    TRACE("(%p, %p)\n", hWaveOut, lpdw);

    if(!lpdw)
        return MMSYSERR_INVALPARAM;

    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutSetPlaybackRate	[WINMM.@]
 */
UINT WINAPI waveOutSetPlaybackRate(HWAVEOUT hWaveOut, DWORD dw)
{
    TRACE("(%p, %08x)\n", hWaveOut, dw);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutGetVolume	[WINMM.@]
 */
UINT WINAPI waveOutGetVolume(HWAVEOUT hWaveOut, LPDWORD lpdw)
{
    TRACE("(%p, %p)\n", hWaveOut, lpdw);

    if (lpdw == NULL) {
        WARN("invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutSetVolume	[WINMM.@]
 */
UINT WINAPI waveOutSetVolume(HWAVEOUT hWaveOut, DWORD dw)
{
    TRACE("(%p, %08x)\n", hWaveOut, dw);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutGetID		[WINMM.@]
 */
UINT WINAPI waveOutGetID(HWAVEOUT hWaveOut, UINT* lpuDeviceID)
{
    TRACE("(%p, %p)\n", hWaveOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutMessage 		[WINMM.@]
 */
UINT WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage,
                           DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%p, %u, %lx, %lx)\n", hWaveOut, uMessage, dwParam1, dwParam2);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveInGetNumDevs 		[WINMM.@]
 */
UINT WINAPI waveInGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_WAVEIN);
}

/**************************************************************************
 * 				waveInGetDevCapsW 		[WINMM.@]
 */
UINT WINAPI waveInGetDevCapsW(UINT_PTR uDeviceID, LPWAVEINCAPSW lpCaps, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%lu %p %u)!\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_WAVEIN, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, WIDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize);
}

/**************************************************************************
 * 				waveInGetDevCapsA 		[WINMM.@]
 */
UINT WINAPI waveInGetDevCapsA(UINT_PTR uDeviceID, LPWAVEINCAPSA lpCaps, UINT uSize)
{
    WAVEINCAPSW		wicW;
    UINT		ret;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = waveInGetDevCapsW(uDeviceID, &wicW, sizeof(wicW));

    if (ret == MMSYSERR_NOERROR) {
	WAVEINCAPSA wicA;
	wicA.wMid           = wicW.wMid;
	wicA.wPid           = wicW.wPid;
	wicA.vDriverVersion = wicW.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, wicW.szPname, -1, wicA.szPname,
                             sizeof(wicA.szPname), NULL, NULL );
	wicA.dwFormats      = wicW.dwFormats;
	wicA.wChannels      = wicW.wChannels;
	memcpy(lpCaps, &wicA, min(uSize, sizeof(wicA)));
    }
    return ret;
}

/**************************************************************************
 * 				waveInOpen			[WINMM.@]
 */
MMRESULT WINAPI waveInOpen(HWAVEIN* lphWaveIn, UINT uDeviceID,
                           LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback,
                           DWORD_PTR dwInstance, DWORD dwFlags)
{
    return WAVE_Open((HANDLE*)lphWaveIn, uDeviceID, MMDRV_WAVEIN, lpFormat,
                     dwCallback, dwInstance, dwFlags);
}

/**************************************************************************
 * 				waveInClose			[WINMM.@]
 */
UINT WINAPI waveInClose(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Message(wmld, WIDM_CLOSE, 0L, 0L);
    if (dwRet != WAVERR_STILLPLAYING)
	MMDRV_Free(hWaveIn, wmld);
    return dwRet;
}

/**************************************************************************
 * 				waveInPrepareHeader		[WINMM.@]
 */
UINT WINAPI waveInPrepareHeader(HWAVEIN hWaveIn, WAVEHDR* lpWaveInHdr,
				UINT uSize)
{
    LPWINE_MLD		wmld;
    UINT                result;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if ((result = MMDRV_Message(wmld, WIDM_PREPARE, (DWORD_PTR)lpWaveInHdr,
                                uSize)) != MMSYSERR_NOTSUPPORTED)
        return result;

    if (lpWaveInHdr->dwFlags & WHDR_INQUEUE)
        return WAVERR_STILLPLAYING;

    lpWaveInHdr->dwFlags |= WHDR_PREPARED;
    lpWaveInHdr->dwFlags &= ~WHDR_DONE;
    lpWaveInHdr->dwBytesRecorded = 0;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInUnprepareHeader	[WINMM.@]
 */
UINT WINAPI waveInUnprepareHeader(HWAVEIN hWaveIn, WAVEHDR* lpWaveInHdr,
				  UINT uSize)
{
    LPWINE_MLD		wmld;
    UINT                result;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL || uSize < sizeof (WAVEHDR))
	return MMSYSERR_INVALPARAM;

    if (!(lpWaveInHdr->dwFlags & WHDR_PREPARED))
	return MMSYSERR_NOERROR;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    if ((result = MMDRV_Message(wmld, WIDM_UNPREPARE, (DWORD_PTR)lpWaveInHdr,
                                uSize)) != MMSYSERR_NOTSUPPORTED)
        return result;

    if (lpWaveInHdr->dwFlags & WHDR_INQUEUE)
        return WAVERR_STILLPLAYING;

    lpWaveInHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveInHdr->dwFlags |= WHDR_DONE;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInAddBuffer		[WINMM.@]
 */
UINT WINAPI waveInAddBuffer(HWAVEIN hWaveIn,
			    WAVEHDR* lpWaveInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_ADDBUFFER, (DWORD_PTR)lpWaveInHdr, uSize);
}

/**************************************************************************
 * 				waveInReset		[WINMM.@]
 */
UINT WINAPI waveInReset(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_RESET, 0L, 0L);
}

/**************************************************************************
 * 				waveInStart		[WINMM.@]
 */
UINT WINAPI waveInStart(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_START, 0L, 0L);
}

/**************************************************************************
 * 				waveInStop		[WINMM.@]
 */
UINT WINAPI waveInStop(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%p);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld,WIDM_STOP, 0L, 0L);
}

/**************************************************************************
 * 				waveInGetPosition	[WINMM.@]
 */
UINT WINAPI waveInGetPosition(HWAVEIN hWaveIn, LPMMTIME lpTime,
			      UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p, %u);\n", hWaveIn, lpTime, uSize);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_GETPOS, (DWORD_PTR)lpTime, uSize);
}

/**************************************************************************
 * 				waveInGetID			[WINMM.@]
 */
UINT WINAPI waveInGetID(HWAVEIN hWaveIn, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %p);\n", hWaveIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInMessage 		[WINMM.@]
 */
UINT WINAPI waveInMessage(HWAVEIN hWaveIn, UINT uMessage,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %u, %ld, %ld)\n", hWaveIn, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) {
	if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, TRUE)) != NULL) {
	    return MMDRV_PhysicalFeatures(wmld, uMessage, dwParam1, dwParam2);
	}
	return MMSYSERR_INVALHANDLE;
    }

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER))
	return MMSYSERR_INVALPARAM;


    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2);
}

/**************************************************************************
 * find out the real mixer ID depending on hmix (depends on dwFlags)
 */
static UINT MIXER_GetDev(HMIXEROBJ hmix, DWORD dwFlags, LPWINE_MIXER * lplpwm)
{
    LPWINE_MIXER	lpwm = NULL;
    UINT		uRet = MMSYSERR_NOERROR;

    switch (dwFlags & 0xF0000000ul) {
    case MIXER_OBJECTF_MIXER:
	lpwm = (LPWINE_MIXER)MMDRV_Get(hmix, MMDRV_MIXER, TRUE);
	break;
    case MIXER_OBJECTF_HMIXER:
	lpwm = (LPWINE_MIXER)MMDRV_Get(hmix, MMDRV_MIXER, FALSE);
	break;
    case MIXER_OBJECTF_WAVEOUT:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_WAVEOUT, TRUE,  MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_HWAVEOUT:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_WAVEOUT, FALSE, MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_WAVEIN:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_WAVEIN,  TRUE,  MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_HWAVEIN:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_WAVEIN,  FALSE, MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_MIDIOUT:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_MIDIOUT, TRUE,  MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_HMIDIOUT:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_MIDIOUT, FALSE, MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_MIDIIN:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_MIDIIN,  TRUE,  MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_HMIDIIN:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_MIDIIN,  FALSE, MMDRV_MIXER);
	break;
    case MIXER_OBJECTF_AUX:
	lpwm = (LPWINE_MIXER)MMDRV_GetRelated(hmix, MMDRV_AUX,     TRUE,  MMDRV_MIXER);
	break;
    default:
	WARN("Unsupported flag (%08lx)\n", dwFlags & 0xF0000000ul);
        lpwm = 0;
        uRet = MMSYSERR_INVALFLAG;
	break;
    }
    *lplpwm = lpwm;
    if (lpwm == 0 && uRet == MMSYSERR_NOERROR)
        uRet = MMSYSERR_INVALPARAM;
    return uRet;
}

/**************************************************************************
 * 				mixerGetNumDevs			[WINMM.@]
 */
UINT WINAPI mixerGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_MIXER);
}

/**************************************************************************
 * 				mixerGetDevCapsA		[WINMM.@]
 */
UINT WINAPI mixerGetDevCapsA(UINT_PTR uDeviceID, LPMIXERCAPSA lpCaps, UINT uSize)
{
    MIXERCAPSW micW;
    UINT       ret;

    if (lpCaps == NULL) return MMSYSERR_INVALPARAM;

    ret = mixerGetDevCapsW(uDeviceID, &micW, sizeof(micW));

    if (ret == MMSYSERR_NOERROR) {
        MIXERCAPSA micA;
        micA.wMid           = micW.wMid;
        micA.wPid           = micW.wPid;
        micA.vDriverVersion = micW.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, micW.szPname, -1, micA.szPname,
                             sizeof(micA.szPname), NULL, NULL );
        micA.fdwSupport     = micW.fdwSupport;
        micA.cDestinations  = micW.cDestinations;
        memcpy(lpCaps, &micA, min(uSize, sizeof(micA)));
    }
    return ret;
}

/**************************************************************************
 * 				mixerGetDevCapsW		[WINMM.@]
 */
UINT WINAPI mixerGetDevCapsW(UINT_PTR uDeviceID, LPMIXERCAPSW lpCaps, UINT uSize)
{
    LPWINE_MLD wmld;

    if (lpCaps == NULL)        return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get((HANDLE)uDeviceID, MMDRV_MIXER, TRUE)) == NULL)
        return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, MXDM_GETDEVCAPS, (DWORD_PTR)lpCaps, uSize);
}

static void CALLBACK MIXER_WCallback(HMIXEROBJ hmx, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam, DWORD_PTR param2)
{
    HWND hWnd = (HWND)dwInstance;

    if (!dwInstance)
        return;

    PostMessageW(hWnd, uMsg, (WPARAM)hmx, (LPARAM)dwParam);
}

/**************************************************************************
 * 				mixerOpen			[WINMM.@]
 */
UINT WINAPI mixerOpen(LPHMIXER lphMix, UINT uDeviceID, DWORD_PTR dwCallback,
                      DWORD_PTR dwInstance, DWORD fdwOpen)
{
    HANDLE		hMix;
    LPWINE_MLD		wmld;
    DWORD		dwRet;
    MIXEROPENDESC	mod;

    TRACE("(%p, %d, %08lx, %08lx, %08x)\n",
	  lphMix, uDeviceID, dwCallback, dwInstance, fdwOpen);

    dwRet = WINMM_CheckCallback(dwCallback, fdwOpen, TRUE);
    if (dwRet != MMSYSERR_NOERROR)
        return dwRet;

    mod.dwCallback = (DWORD_PTR)MIXER_WCallback;
    if ((fdwOpen & CALLBACK_TYPEMASK) == CALLBACK_WINDOW)
        mod.dwInstance = dwCallback;
    else
        mod.dwInstance = 0;

    /* We're remapping to CALLBACK_FUNCTION because that's what old winmm is
     * documented to do when opening the mixer driver.
     * FIXME: Native supports CALLBACK_EVENT + CALLBACK_THREAD flags since w2k.
     * FIXME: The non ALSA drivers ignore callback requests - bug.
     */

    wmld = MMDRV_Alloc(sizeof(WINE_MIXER), MMDRV_MIXER, &hMix, &fdwOpen,
		       &dwCallback, &dwInstance);
    wmld->uDeviceID = uDeviceID;
    mod.hmx = hMix;

    dwRet = MMDRV_Open(wmld, MXDM_OPEN, (DWORD_PTR)&mod, CALLBACK_FUNCTION);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMix, wmld);
	hMix = 0;
    }
    if (lphMix) *lphMix = hMix;
    TRACE("=> %d hMixer=%p\n", dwRet, hMix);

    return dwRet;
}

/**************************************************************************
 * 				mixerClose			[WINMM.@]
 */
UINT WINAPI mixerClose(HMIXER hMix)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%p)\n", hMix);

    if ((wmld = MMDRV_Get(hMix, MMDRV_MIXER, FALSE)) == NULL) return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MXDM_CLOSE);
    MMDRV_Free(hMix, wmld);

    return dwRet;
}

/**************************************************************************
 * 				mixerGetID			[WINMM.@]
 */
UINT WINAPI mixerGetID(HMIXEROBJ hmix, LPUINT lpid, DWORD fdwID)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p %p %08x)\n", hmix, lpid, fdwID);

    if ((uRet = MIXER_GetDev(hmix, fdwID, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    if (lpid)
      *lpid = lpwm->mld.uDeviceID;

    return uRet;
}

/**************************************************************************
 * 				mixerGetControlDetailsW		[WINMM.@]
 */
UINT WINAPI mixerGetControlDetailsW(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcdW,
				    DWORD fdwDetails)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %08x)\n", hmix, lpmcdW, fdwDetails);

    if ((uRet = MIXER_GetDev(hmix, fdwDetails, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    if (lpmcdW == NULL || lpmcdW->cbStruct != sizeof(*lpmcdW))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(&lpwm->mld, MXDM_GETCONTROLDETAILS, (DWORD_PTR)lpmcdW,
			 fdwDetails);
}

/**************************************************************************
 * 				mixerGetControlDetailsA	[WINMM.@]
 */
UINT WINAPI mixerGetControlDetailsA(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcdA,
                                    DWORD fdwDetails)
{
    DWORD			ret = MMSYSERR_NOTENABLED;

    TRACE("(%p, %p, %08x)\n", hmix, lpmcdA, fdwDetails);

    if (lpmcdA == NULL || lpmcdA->cbStruct != sizeof(*lpmcdA))
	return MMSYSERR_INVALPARAM;

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK) {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	/* can safely use A structure as it is, no string inside */
	ret = mixerGetControlDetailsW(hmix, lpmcdA, fdwDetails);
	break;
    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
	{
            MIXERCONTROLDETAILS_LISTTEXTA *pDetailsA = lpmcdA->paDetails;
            MIXERCONTROLDETAILS_LISTTEXTW *pDetailsW;
	    int size = max(1, lpmcdA->cChannels) * sizeof(MIXERCONTROLDETAILS_LISTTEXTW);
            unsigned int i;

	    if (lpmcdA->u.cMultipleItems != 0) {
		size *= lpmcdA->u.cMultipleItems;
	    }
	    pDetailsW = HeapAlloc(GetProcessHeap(), 0, size);
            lpmcdA->paDetails = pDetailsW;
            lpmcdA->cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTW);
	    /* set up lpmcd->paDetails */
	    ret = mixerGetControlDetailsW(hmix, lpmcdA, fdwDetails);
	    /* copy from lpmcd->paDetails back to paDetailsW; */
            if (ret == MMSYSERR_NOERROR) {
                for (i = 0; i < lpmcdA->u.cMultipleItems * lpmcdA->cChannels; i++) {
                    pDetailsA->dwParam1 = pDetailsW->dwParam1;
                    pDetailsA->dwParam2 = pDetailsW->dwParam2;
                    WideCharToMultiByte( CP_ACP, 0, pDetailsW->szName, -1,
                                         pDetailsA->szName,
                                         sizeof(pDetailsA->szName), NULL, NULL );
                    pDetailsA++;
                    pDetailsW++;
                }
                pDetailsA -= lpmcdA->u.cMultipleItems * lpmcdA->cChannels;
                pDetailsW -= lpmcdA->u.cMultipleItems * lpmcdA->cChannels;
            }
	    HeapFree(GetProcessHeap(), 0, pDetailsW);
	    lpmcdA->paDetails = pDetailsA;
            lpmcdA->cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTA);
	}
	break;
    default:
	ERR("Unsupported fdwDetails=0x%08x\n", fdwDetails);
    }

    return ret;
}

/**************************************************************************
 * 				mixerGetLineControlsA	[WINMM.@]
 */
UINT WINAPI mixerGetLineControlsA(HMIXEROBJ hmix, LPMIXERLINECONTROLSA lpmlcA,
				  DWORD fdwControls)
{
    MIXERLINECONTROLSW	mlcW;
    DWORD		ret;
    unsigned int	i;

    TRACE("(%p, %p, %08x)\n", hmix, lpmlcA, fdwControls);

    if (lpmlcA == NULL || lpmlcA->cbStruct != sizeof(*lpmlcA) ||
	lpmlcA->cbmxctrl != sizeof(MIXERCONTROLA))
	return MMSYSERR_INVALPARAM;

    mlcW.cbStruct = sizeof(mlcW);
    mlcW.dwLineID = lpmlcA->dwLineID;
    mlcW.u.dwControlID = lpmlcA->u.dwControlID;
    mlcW.u.dwControlType = lpmlcA->u.dwControlType;

    /* Debugging on Windows shows for MIXER_GETLINECONTROLSF_ONEBYTYPE only,
       the control count is assumed to be 1 - This is relied upon by a game,
       "Dynomite Deluze"                                                    */
    if (MIXER_GETLINECONTROLSF_ONEBYTYPE == (fdwControls & MIXER_GETLINECONTROLSF_QUERYMASK)) {
        mlcW.cControls = 1;
    } else {
        mlcW.cControls = lpmlcA->cControls;
    }
    mlcW.cbmxctrl = sizeof(MIXERCONTROLW);
    mlcW.pamxctrl = HeapAlloc(GetProcessHeap(), 0,
			      mlcW.cControls * mlcW.cbmxctrl);

    ret = mixerGetLineControlsW(hmix, &mlcW, fdwControls);

    if (ret == MMSYSERR_NOERROR) {
	lpmlcA->dwLineID = mlcW.dwLineID;
	lpmlcA->u.dwControlID = mlcW.u.dwControlID;
	lpmlcA->u.dwControlType = mlcW.u.dwControlType;

	for (i = 0; i < mlcW.cControls; i++) {
	    lpmlcA->pamxctrl[i].cbStruct = sizeof(MIXERCONTROLA);
	    lpmlcA->pamxctrl[i].dwControlID = mlcW.pamxctrl[i].dwControlID;
	    lpmlcA->pamxctrl[i].dwControlType = mlcW.pamxctrl[i].dwControlType;
	    lpmlcA->pamxctrl[i].fdwControl = mlcW.pamxctrl[i].fdwControl;
	    lpmlcA->pamxctrl[i].cMultipleItems = mlcW.pamxctrl[i].cMultipleItems;
            WideCharToMultiByte( CP_ACP, 0, mlcW.pamxctrl[i].szShortName, -1,
                                 lpmlcA->pamxctrl[i].szShortName,
                                 sizeof(lpmlcA->pamxctrl[i].szShortName), NULL, NULL );
            WideCharToMultiByte( CP_ACP, 0, mlcW.pamxctrl[i].szName, -1,
                                 lpmlcA->pamxctrl[i].szName,
                                 sizeof(lpmlcA->pamxctrl[i].szName), NULL, NULL );
	    /* sizeof(lpmlcA->pamxctrl[i].Bounds) ==
	     * sizeof(mlcW.pamxctrl[i].Bounds) */
	    memcpy(&lpmlcA->pamxctrl[i].Bounds, &mlcW.pamxctrl[i].Bounds,
		   sizeof(mlcW.pamxctrl[i].Bounds));
	    /* sizeof(lpmlcA->pamxctrl[i].Metrics) ==
	     * sizeof(mlcW.pamxctrl[i].Metrics) */
	    memcpy(&lpmlcA->pamxctrl[i].Metrics, &mlcW.pamxctrl[i].Metrics,
		   sizeof(mlcW.pamxctrl[i].Metrics));
	}
    }

    HeapFree(GetProcessHeap(), 0, mlcW.pamxctrl);

    return ret;
}

/**************************************************************************
 * 				mixerGetLineControlsW		[WINMM.@]
 */
UINT WINAPI mixerGetLineControlsW(HMIXEROBJ hmix, LPMIXERLINECONTROLSW lpmlcW,
				  DWORD fdwControls)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %08x)\n", hmix, lpmlcW, fdwControls);

    if ((uRet = MIXER_GetDev(hmix, fdwControls, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    if (lpmlcW == NULL || lpmlcW->cbStruct != sizeof(*lpmlcW))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(&lpwm->mld, MXDM_GETLINECONTROLS, (DWORD_PTR)lpmlcW,
			 fdwControls);
}

/**************************************************************************
 * 				mixerGetLineInfoW		[WINMM.@]
 */
UINT WINAPI mixerGetLineInfoW(HMIXEROBJ hmix, LPMIXERLINEW lpmliW, DWORD fdwInfo)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %08x)\n", hmix, lpmliW, fdwInfo);

    if ((uRet = MIXER_GetDev(hmix, fdwInfo, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    return MMDRV_Message(&lpwm->mld, MXDM_GETLINEINFO, (DWORD_PTR)lpmliW,
			 fdwInfo);
}

/**************************************************************************
 * 				mixerGetLineInfoA		[WINMM.@]
 */
UINT WINAPI mixerGetLineInfoA(HMIXEROBJ hmix, LPMIXERLINEA lpmliA,
			      DWORD fdwInfo)
{
    MIXERLINEW		mliW;
    UINT		ret;

    TRACE("(%p, %p, %08x)\n", hmix, lpmliA, fdwInfo);

    if (lpmliA == NULL || lpmliA->cbStruct != sizeof(*lpmliA))
	return MMSYSERR_INVALPARAM;

    mliW.cbStruct = sizeof(mliW);
    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	mliW.dwComponentType = lpmliA->dwComponentType;
	break;
    case MIXER_GETLINEINFOF_DESTINATION:
	mliW.dwDestination = lpmliA->dwDestination;
	break;
    case MIXER_GETLINEINFOF_LINEID:
	mliW.dwLineID = lpmliA->dwLineID;
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	mliW.dwDestination = lpmliA->dwDestination;
	mliW.dwSource = lpmliA->dwSource;
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	mliW.Target.dwType = lpmliA->Target.dwType;
	mliW.Target.wMid = lpmliA->Target.wMid;
	mliW.Target.wPid = lpmliA->Target.wPid;
	mliW.Target.vDriverVersion = lpmliA->Target.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, lpmliA->Target.szPname, -1, mliW.Target.szPname, sizeof(mliW.Target.szPname)/sizeof(WCHAR));
	break;
    default:
	WARN("Unsupported fdwControls=0x%08x\n", fdwInfo);
        return MMSYSERR_INVALFLAG;
    }

    ret = mixerGetLineInfoW(hmix, &mliW, fdwInfo);

    if(ret == MMSYSERR_NOERROR)
    {
        lpmliA->dwDestination = mliW.dwDestination;
        lpmliA->dwSource = mliW.dwSource;
        lpmliA->dwLineID = mliW.dwLineID;
        lpmliA->fdwLine = mliW.fdwLine;
        lpmliA->dwUser = mliW.dwUser;
        lpmliA->dwComponentType = mliW.dwComponentType;
        lpmliA->cChannels = mliW.cChannels;
        lpmliA->cConnections = mliW.cConnections;
        lpmliA->cControls = mliW.cControls;
        WideCharToMultiByte( CP_ACP, 0, mliW.szShortName, -1, lpmliA->szShortName,
                             sizeof(lpmliA->szShortName), NULL, NULL);
        WideCharToMultiByte( CP_ACP, 0, mliW.szName, -1, lpmliA->szName,
                             sizeof(lpmliA->szName), NULL, NULL );
        lpmliA->Target.dwType = mliW.Target.dwType;
        lpmliA->Target.dwDeviceID = mliW.Target.dwDeviceID;
        lpmliA->Target.wMid = mliW.Target.wMid;
        lpmliA->Target.wPid = mliW.Target.wPid;
        lpmliA->Target.vDriverVersion = mliW.Target.vDriverVersion;
        WideCharToMultiByte( CP_ACP, 0, mliW.Target.szPname, -1, lpmliA->Target.szPname,
                             sizeof(lpmliA->Target.szPname), NULL, NULL );
    }
    return ret;
}

/**************************************************************************
 * 				mixerSetControlDetails	[WINMM.@]
 */
UINT WINAPI mixerSetControlDetails(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcd,
				   DWORD fdwDetails)
{
    LPWINE_MIXER	lpwm;
    UINT		uRet = MMSYSERR_NOERROR;

    TRACE("(%p, %p, %08x)\n", hmix, lpmcd, fdwDetails);

    if ((uRet = MIXER_GetDev(hmix, fdwDetails, &lpwm)) != MMSYSERR_NOERROR)
	return uRet;

    return MMDRV_Message(&lpwm->mld, MXDM_SETCONTROLDETAILS, (DWORD_PTR)lpmcd,
			 fdwDetails);
}

/**************************************************************************
 * 				mixerMessage		[WINMM.@]
 */
DWORD WINAPI mixerMessage(HMIXER hmix, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%p, %d, %08lx, %08lx): semi-stub?\n",
	  hmix, uMsg, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hmix, MMDRV_MIXER, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMsg, dwParam1, dwParam2);
}
