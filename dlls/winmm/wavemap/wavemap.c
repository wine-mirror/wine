/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*				   
 * Wine Wave mapper driver
 *
 * Copyright 	1999 Eric Pouech
 */

/* TODOs
 *	+ implement wavein as waveout has been implemented
 *	+ better protection against evilish dwUser parameters
 *	+ use asynchronous ACM conversion
 */

#include "winuser.h"
#include "driver.h"
#include "mmddk.h"
#include "msacm.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msacm)

typedef	struct tagWAVEMAPDATA {
    struct tagWAVEMAPDATA*	self;
    HWAVE	hWave;
    HACMSTREAM	hAcmStream;
    /* needed data to filter callbacks. Only needed when hAcmStream is not 0 */
    DWORD	dwCallback;
    DWORD	dwClientInstance;
    DWORD	dwFlags;
    MMRESULT (WINAPI *acmStreamOpen)(LPHACMSTREAM, HACMDRIVER, LPWAVEFORMATEX, LPWAVEFORMATEX, LPWAVEFILTER, DWORD, DWORD, DWORD);
    MMRESULT (WINAPI *acmStreamClose)(HACMSTREAM, DWORD);
    MMRESULT (WINAPI *acmStreamSize)(HACMSTREAM, DWORD, LPDWORD, DWORD);
    MMRESULT (WINAPI *acmStreamConvert)(HACMSTREAM, PACMSTREAMHEADER, DWORD);
    MMRESULT (WINAPI *acmStreamPrepareHeader)(HACMSTREAM, PACMSTREAMHEADER, DWORD);
    MMRESULT (WINAPI *acmStreamUnprepareHeader)(HACMSTREAM, PACMSTREAMHEADER, DWORD);
} WAVEMAPDATA;

static	BOOL	WAVEMAP_IsData(WAVEMAPDATA* wm)
{
    return (!IsBadReadPtr(wm, sizeof(WAVEMAPDATA)) && wm->self == wm);
}

/*======================================================================*
 *                  WAVE OUT part                                       *
 *======================================================================*/

static void	CALLBACK WAVEMAP_DstCallback(HDRVR hDev, UINT uMsg, DWORD dwInstance, 
					     DWORD dwParam1, DWORD dwParam2)
{
    WAVEMAPDATA*	wom = (WAVEMAPDATA*)dwInstance;

    TRACE("(0x%x %u %ld %ld %ld);\n", hDev, uMsg, dwInstance, dwParam1, dwParam2);

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	/* dwParam1 & dwParam2 are supposed to be 0, nothing to do */
	break;
    case WOM_DONE:
	{
	    LPWAVEHDR		lpWaveHdrDst = (LPWAVEHDR)dwParam1;
	    PACMSTREAMHEADER	ash = (PACMSTREAMHEADER)((LPSTR)lpWaveHdrDst - sizeof(ACMSTREAMHEADER));
	    LPWAVEHDR		lpWaveHdrSrc = (LPWAVEHDR)ash->dwUser;

	    lpWaveHdrSrc->dwFlags &= ~WHDR_INQUEUE;
	    lpWaveHdrSrc->dwFlags |= WHDR_DONE;
	    dwParam1 = (DWORD)lpWaveHdrSrc;
	}
	break;
    default:
	ERR("Unknown msg %u\n", uMsg);
    }

    DriverCallback(wom->dwCallback, wom->dwFlags, hDev, uMsg, 
		   wom->dwClientInstance, dwParam1, dwParam2);
}

static	DWORD	wodOpenHelper(WAVEMAPDATA* wom, UINT idx, 
			      LPWAVEOPENDESC lpDesc, LPWAVEFORMATEX lpwfx, 
			      DWORD dwFlags)
{
    DWORD	ret;

    /* destination is always PCM, so the formulas below apply */
    lpwfx->nBlockAlign = (lpwfx->nChannels * lpwfx->wBitsPerSample) / 8;
    lpwfx->nAvgBytesPerSec = lpwfx->nSamplesPerSec * lpwfx->nBlockAlign;
    ret = wom->acmStreamOpen(&wom->hAcmStream, 0, lpDesc->lpFormat, lpwfx, NULL, 0L, 0L, 0L);
    if (ret != MMSYSERR_NOERROR)
	return ret;
    return waveOutOpen(&wom->hWave, idx, lpwfx, (DWORD)WAVEMAP_DstCallback, 
		       (DWORD)wom, (dwFlags & CALLBACK_TYPEMASK) | CALLBACK_FUNCTION);
}

static	DWORD	wodOpen(LPDWORD lpdwUser, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    UINT 		nd = waveOutGetNumDevs();
    UINT		i;
    WAVEMAPDATA*	wom = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEMAPDATA));
    WAVEFORMATEX	wfx;

    TRACE("(%p %p %08lx\n", lpdwUser, lpDesc, dwFlags);

    if (!wom)
	return MMSYSERR_NOMEM;

    wom->self = wom;

    for (i = 0; i < nd; i++) {
	/* if no ACM stuff is involved, no need to handle callbacks at this
	 * level, this will be done transparently
	 */
	if (waveOutOpen(&wom->hWave, i, lpDesc->lpFormat, lpDesc->dwCallback, 
			lpDesc->dwInstance, dwFlags) == MMSYSERR_NOERROR) {
	    wom->hAcmStream = 0;
	    goto found;
	}
    }

    /* temporary hack until real builtin dll loading is available */
    do {
	HMODULE hModule = LoadLibraryA("msacm32.dll");
	
	wom->acmStreamOpen            = (void*)GetProcAddress(hModule, "acmStreamOpen");
	wom->acmStreamClose           = (void*)GetProcAddress(hModule, "acmStreamClose");
	wom->acmStreamSize            = (void*)GetProcAddress(hModule, "acmStreamSize");
	wom->acmStreamConvert         = (void*)GetProcAddress(hModule, "acmStreamConvert");
	wom->acmStreamPrepareHeader   = (void*)GetProcAddress(hModule, "acmStreamPrepareHeader");
	wom->acmStreamUnprepareHeader = (void*)GetProcAddress(hModule, "acmStreamUnprepareHeader");
    } while (0);

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.cbSize = 0; /* normally, this field is not used for PCM format, just in case */
    /* try some ACM stuff */

    wom->dwCallback = lpDesc->dwCallback;
    wom->dwFlags = dwFlags;
    wom->dwClientInstance = lpDesc->dwInstance;

#define	TRY(sps,bps)    wfx.nSamplesPerSec = (sps); wfx.wBitsPerSample = (bps); \
                        if (wodOpenHelper(wom, i, lpDesc, &wfx, dwFlags) == MMSYSERR_NOERROR) goto found;

    for (i = 0; i < nd; i++) {
	/* first try with same stereo/mono option as source */
	wfx.nChannels = lpDesc->lpFormat->nChannels;
	TRY(44100, 8);
	TRY(22050, 8);
	TRY(11025, 8);

	/* 2^3 => 1, 1^3 => 2, so if stereo, try mono (and the other way around) */
	wfx.nChannels ^= 3; 
	TRY(44100, 8);
	TRY(22050, 8);
	TRY(11025, 8);
    }
#undef TRY
		      
    HeapFree(GetProcessHeap(), 0, wom);
    return MMSYSERR_ALLOCATED;
found:
    lpDesc->hWave = wom->hWave;
    *lpdwUser = (DWORD)wom;
    return MMSYSERR_NOERROR;
}

static	DWORD	wodClose(WAVEMAPDATA* wom)
{
    DWORD ret = waveOutClose(wom->hWave);

    if (ret == MMSYSERR_NOERROR) {
	if (wom->hAcmStream) {
	    ret = wom->acmStreamClose(wom->hAcmStream, 0);
	}
	if (ret == MMSYSERR_NOERROR) {
	    HeapFree(GetProcessHeap(), 0, wom);
	}
    }
    return ret;
}

static	DWORD	wodWrite(WAVEMAPDATA* wom, LPWAVEHDR lpWaveHdrSrc, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    LPWAVEHDR		lpWaveHdrDst;

    if (!wom->hAcmStream) {
	return waveOutWrite(wom->hWave, lpWaveHdrSrc, dwParam2);
    }
    
    lpWaveHdrSrc->dwFlags |= WHDR_INQUEUE;
    ash = (PACMSTREAMHEADER)lpWaveHdrSrc->reserved;
    if (wom->acmStreamConvert(wom->hAcmStream, ash, 0L) != MMSYSERR_NOERROR)
	return MMSYSERR_ERROR;
    
    lpWaveHdrDst = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    lpWaveHdrDst->dwBufferLength = ash->cbDstLengthUsed;
    return waveOutWrite(wom->hWave, lpWaveHdrDst, sizeof(*lpWaveHdrDst));
}

static	DWORD	wodPrepare(WAVEMAPDATA* wom, LPWAVEHDR lpWaveHdrSrc, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    DWORD		size;
    DWORD		dwRet;
    LPWAVEHDR		lpWaveHdrDst;

    if (!wom->hAcmStream) {
	return waveOutPrepareHeader(wom->hWave, lpWaveHdrSrc, dwParam2);
    }
    if (wom->acmStreamSize(wom->hAcmStream, lpWaveHdrSrc->dwBufferLength, &size, ACM_STREAMSIZEF_SOURCE) != MMSYSERR_NOERROR)
	return MMSYSERR_ERROR;

    ash = HeapAlloc(GetProcessHeap(), 0, sizeof(ACMSTREAMHEADER) + sizeof(WAVEHDR) + size);
    if (ash == NULL)
	return MMSYSERR_NOMEM;

    ash->cbStruct = sizeof(*ash);
    ash->fdwStatus = 0L;
    ash->dwUser = (DWORD)lpWaveHdrSrc;
    ash->pbSrc = lpWaveHdrSrc->lpData;
    ash->cbSrcLength = lpWaveHdrSrc->dwBufferLength;
    /* ash->cbSrcLengthUsed */
    ash->dwSrcUser = lpWaveHdrSrc->dwUser; /* FIXME ? */
    ash->pbDst = (LPSTR)ash + sizeof(ACMSTREAMHEADER) + sizeof(WAVEHDR);
    ash->cbDstLength = size;
    /* ash->cbDstLengthUsed */
    ash->dwDstUser = 0; /* FIXME ? */
    dwRet = wom->acmStreamPrepareHeader(wom->hAcmStream, ash, 0L);
    if (dwRet != MMSYSERR_NOERROR)
	goto errCleanUp;

    lpWaveHdrDst = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    lpWaveHdrDst->lpData = ash->pbDst;
    lpWaveHdrDst->dwBufferLength = size; /* conversion is not done yet */
    lpWaveHdrDst->dwFlags = 0;
    lpWaveHdrDst->dwLoops = 0;
    dwRet = waveOutPrepareHeader(wom->hWave, lpWaveHdrDst, sizeof(*lpWaveHdrDst));
    if (dwRet != MMSYSERR_NOERROR)
	goto errCleanUp;

    lpWaveHdrSrc->reserved = (DWORD)ash;
    lpWaveHdrSrc->dwFlags = WHDR_PREPARED;
    TRACE("=> (0)\n");
    return MMSYSERR_NOERROR;
errCleanUp:
    TRACE("=> (%ld)\n", dwRet);
    HeapFree(GetProcessHeap(), 0, ash);
    return dwRet;
}

static	DWORD	wodUnprepare(WAVEMAPDATA* wom, LPWAVEHDR lpWaveHdrSrc, DWORD dwParam2)
{
    PACMSTREAMHEADER	ash;
    LPWAVEHDR		lpWaveHdrDst;
    DWORD		dwRet1, dwRet2;

    if (!wom->hAcmStream) {
	return waveOutUnprepareHeader(wom->hWave, lpWaveHdrSrc, dwParam2);
    }
    ash = (PACMSTREAMHEADER)lpWaveHdrSrc->reserved;
    dwRet1 = wom->acmStreamUnprepareHeader(wom->hAcmStream, ash, 0L);
    
    lpWaveHdrDst = (LPWAVEHDR)((LPSTR)ash + sizeof(ACMSTREAMHEADER));
    dwRet2 = waveOutUnprepareHeader(wom->hWave, lpWaveHdrDst, sizeof(*lpWaveHdrDst));

    HeapFree(GetProcessHeap(), 0, ash);
    
    lpWaveHdrSrc->dwFlags &= ~WHDR_PREPARED;
    return (dwRet1 == MMSYSERR_NOERROR) ? dwRet2 : dwRet1;
}

static	DWORD	wodGetPosition(WAVEMAPDATA* wom, LPMMTIME lpTime, DWORD dwParam2)
{
    return waveOutGetPosition(wom->hWave, lpTime, dwParam2);
}

static	DWORD	wodGetDevCaps(UINT wDevID, WAVEMAPDATA* wom, LPWAVEOUTCAPSA lpWaveCaps, DWORD dwParam2)
{
    /* if opened low driver, forward message */
    if (WAVEMAP_IsData(wom))
	return waveOutGetDevCapsA(wom->hWave, lpWaveCaps, dwParam2);
    /* otherwise, return caps of mapper itself */
    if (wDevID == (UINT)-1 || wDevID == (UINT16)-1) {
	lpWaveCaps->wMid = 0x00FF;
	lpWaveCaps->wPid = 0x0001;
	lpWaveCaps->vDriverVersion = 0x0100;
	strcpy(lpWaveCaps->szPname, "Wine wave out mapper");
	lpWaveCaps->dwFormats = 
	    WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 | WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16 |
	    WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 |
	    WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 | WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16;
	lpWaveCaps->wChannels = 2;
	lpWaveCaps->dwSupport = WAVECAPS_VOLUME | WAVECAPS_LRVOLUME;

	return MMSYSERR_NOERROR;
    }
    ERR("This shouldn't happen\n");
    return MMSYSERR_ERROR; 
}

static	DWORD	wodGetVolume(UINT wDevID, WAVEMAPDATA* wom, LPDWORD lpVol)
{
    if (WAVEMAP_IsData(wom))
	return waveOutGetVolume(wom->hWave, lpVol);
    return MMSYSERR_NOERROR;
}

static	DWORD	wodSetVolume(UINT wDevID, WAVEMAPDATA* wom, DWORD vol)
{
    if (WAVEMAP_IsData(wom))
	return waveOutSetVolume(wom->hWave, vol);
    return MMSYSERR_NOERROR;
}

static	DWORD	wodPause(WAVEMAPDATA* wom)
{
    return waveOutPause(wom->hWave);
}

static	DWORD	wodRestart(WAVEMAPDATA* wom)
{
    return waveOutRestart(wom->hWave);
}

static	DWORD	wodReset(WAVEMAPDATA* wom)
{
    return waveOutReset(wom->hWave);
}

/**************************************************************************
 * 				WAVEMAP_wodMessage	[sample driver]
 */
DWORD WINAPI WAVEMAP_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser, 
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
    case WODM_OPEN:	 	return wodOpen		((LPDWORD)dwUser,      (LPWAVEOPENDESC)dwParam1,dwParam2);
    case WODM_CLOSE:	 	return wodClose		((WAVEMAPDATA*)dwUser);
    case WODM_WRITE:	 	return wodWrite		((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1,	dwParam2);
    case WODM_PAUSE:	 	return wodPause		((WAVEMAPDATA*)dwUser);
    case WODM_GETPOS:	 	return wodGetPosition	((WAVEMAPDATA*)dwUser, (LPMMTIME)dwParam1, 	dwParam2);
    case WODM_BREAKLOOP: 	return MMSYSERR_NOTSUPPORTED;
    case WODM_PREPARE:	 	return wodPrepare	((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WODM_UNPREPARE: 	return wodUnprepare	((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WODM_GETDEVCAPS:	return wodGetDevCaps	(wDevID, (WAVEMAPDATA*)dwUser, (LPWAVEOUTCAPSA)dwParam1,dwParam2);
    case WODM_GETNUMDEVS:	return 1;
    case WODM_GETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPITCH:	 	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
    case WODM_GETVOLUME:	return wodGetVolume	(wDevID, (WAVEMAPDATA*)dwUser, (LPDWORD)dwParam1);
    case WODM_SETVOLUME:	return wodSetVolume	(wDevID, (WAVEMAPDATA*)dwUser, dwParam1);
    case WODM_RESTART:		return wodRestart	((WAVEMAPDATA*)dwUser);
    case WODM_RESET:		return wodReset		((WAVEMAPDATA*)dwUser);
    default:
	FIXME("unknown message %d!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  WAVE IN part                                        *
 *======================================================================*/

static	DWORD	widOpen(LPDWORD lpdwUser, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    UINT 		nd = waveInGetNumDevs();
    UINT		i;
    WAVEMAPDATA*	wim = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEMAPDATA));

    TRACE("(%p %p %08lx\n", lpdwUser, lpDesc, dwFlags);

    wim->self = wim;

    for (i = 0; i < nd; i++) {
	if (waveInOpen(&wim->hWave, i, lpDesc->lpFormat, lpDesc->dwCallback, 
			lpDesc->dwInstance, dwFlags) == MMSYSERR_NOERROR) {
	    lpDesc->hWave = wim->hWave;
	    *lpdwUser = (DWORD)wim;
	    return MMSYSERR_NOERROR;
	}
    }
    HeapFree(GetProcessHeap(), 0, wim);
    return MMSYSERR_ALLOCATED;
}

static	DWORD	widClose(WAVEMAPDATA* wim)
{
    DWORD ret = waveInClose(wim->hWave);
    if (ret == MMSYSERR_NOERROR)
	HeapFree(GetProcessHeap(), 0, wim);
    return ret;
}

static	DWORD	widAddBuffer(WAVEMAPDATA* wim, LPWAVEHDR lpWaveHdr, DWORD dwParam2)
{
    return waveInAddBuffer(wim->hWave, lpWaveHdr, dwParam2);
}

static	DWORD	widPrepare(WAVEMAPDATA* wim, LPWAVEHDR lpWaveHdr, DWORD dwParam2)
{
    return waveInPrepareHeader(wim->hWave, lpWaveHdr, dwParam2);
}

static	DWORD	widUnprepare(WAVEMAPDATA* wim, LPWAVEHDR lpWaveHdr, DWORD dwParam2)
{
    return waveInUnprepareHeader(wim->hWave, lpWaveHdr, dwParam2);
}

static	DWORD	widGetPosition(WAVEMAPDATA* wim, LPMMTIME lpTime, DWORD dwParam2)
{
    return waveInGetPosition(wim->hWave, lpTime, dwParam2);
}

static	DWORD	widGetDevCaps(UINT wDevID, WAVEMAPDATA* wim, LPWAVEINCAPSA lpWaveCaps, DWORD dwParam2)
{
    /* if opened low driver, forward message */
    if (WAVEMAP_IsData(wim))
	return waveInGetDevCapsA(wim->hWave, lpWaveCaps, dwParam2);
    /* otherwise, return caps of mapper itself */
    if (wDevID == (UINT)-1 || wDevID == (UINT16)-1) {
	lpWaveCaps->wMid = 0x00FF;
	lpWaveCaps->wPid = 0x0001;
	lpWaveCaps->vDriverVersion = 0x0001;
	strcpy(lpWaveCaps->szPname, "Wine wave in mapper");
	lpWaveCaps->dwFormats = 
	    WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 | WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16 |
	    WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 |
	    WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 | WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16;    
	lpWaveCaps->wChannels = 2;
	return MMSYSERR_NOERROR;
    }
    ERR("This shouldn't happen\n");
    return MMSYSERR_ERROR; 
}

static	DWORD	widStop(WAVEMAPDATA* wim)
{
    return waveInStop(wim->hWave);
}

static	DWORD	widStart(WAVEMAPDATA* wim)
{
    return waveInStart(wim->hWave);
}

static	DWORD	widReset(WAVEMAPDATA* wim)
{
    return waveInReset(wim->hWave);
}

/**************************************************************************
 * 				WAVEMAP_widMessage	[sample driver]
 */
DWORD WINAPI WAVEMAP_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
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

    case WIDM_OPEN:		return widOpen          ((LPDWORD)dwUser,     (LPWAVEOPENDESC)dwParam1, dwParam2);
    case WIDM_CLOSE:		return widClose         ((WAVEMAPDATA*)dwUser);

    case WIDM_ADDBUFFER:	return widAddBuffer     ((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WIDM_PREPARE:		return widPrepare       ((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WIDM_UNPREPARE:	return widUnprepare     ((WAVEMAPDATA*)dwUser, (LPWAVEHDR)dwParam1, 	dwParam2);
    case WIDM_GETDEVCAPS:	return widGetDevCaps    (wDevID, (WAVEMAPDATA*)dwUser, (LPWAVEINCAPSA)dwParam1, dwParam2);
    case WIDM_GETNUMDEVS:	return 1;
    case WIDM_GETPOS:		return widGetPosition   ((WAVEMAPDATA*)dwUser, (LPMMTIME)dwParam1, 	dwParam2);
    case WIDM_RESET:		return widReset         ((WAVEMAPDATA*)dwUser);
    case WIDM_START:		return widStart         ((WAVEMAPDATA*)dwUser);
    case WIDM_STOP:		return widStop          ((WAVEMAPDATA*)dwUser);
    default:
	FIXME("unknown message %u!\n", wMsg);
    }
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
 *                  Driver part                                         *
 *======================================================================*/

static	struct WINE_WAVEMAP* oss = NULL;

/**************************************************************************
 * 				WAVEMAP_drvOpen			[internal]	
 */
static	DWORD	WAVEMAP_drvOpen(LPSTR str)
{
    if (oss)
	return 0;
    
    /* I know, this is ugly, but who cares... */
    oss = (struct WINE_WAVEMAP*)1;
    return 1;
}

/**************************************************************************
 * 				WAVEMAP_drvClose		[internal]	
 */
static	DWORD	WAVEMAP_drvClose(DWORD dwDevID)
{
    if (oss) {
	oss = NULL;
	return 1;
    }
    return 0;
}

/**************************************************************************
 * 				WAVEMAP_DriverProc		[internal]
 */
LONG CALLBACK	WAVEMAP_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
				   DWORD dwParam1, DWORD dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */
    
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return WAVEMAP_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return WAVEMAP_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "WAVEMAP MultiMedia Driver !", "OSS Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}


