/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM functions
 *
 * Copyright 1993 Martin Ayotte
 */

/* 
 * Eric POUECH : 
 * 98/9	added support for Win32 MCI
 */

/* FIXME: I think there are some segmented vs. linear pointer weirdnesses 
 *        and long term pointers to 16 bit space in here
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "winbase.h"
#include "windef.h"
#include "wine/winbase16.h"
#include "heap.h"
#include "user.h"
#include "driver.h"
#include "multimedia.h"
#include "callback.h"
#include "module.h"
#include "selectors.h"
#include "debugstr.h"
#include "debug.h"

UINT16 WINAPI midiGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize);
static UINT16 waveGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize);
LONG   WINAPI DrvDefDriverProc(DWORD dwDevID, HDRVR16 hDrv, WORD wMsg, 
			       DWORD dwParam1, DWORD dwParam2);

/**************************************************************************
 * 				MMSYSTEM_WEP		[MMSYSTEM.1]
 */
int WINAPI MMSYSTEM_WEP(HINSTANCE16 hInstance, WORD wDataSeg,
                        WORD cbHeapSize, LPSTR lpCmdLine)
{
    FIXME(mmsys, "STUB: Unloading MMSystem DLL ... hInst=%04X \n", hInstance);
    return(TRUE);
}

static void MMSYSTEM_MMTIME32to16(LPMMTIME16 mmt16, LPMMTIME mmt32) 
{
    mmt16->wType = mmt32->wType;
    /* layout of rest is the same for 32/16 */
    memcpy(&(mmt32->u), &(mmt16->u), sizeof(mmt16->u));
}

static void MMSYSTEM_MMTIME16to32(LPMMTIME mmt32, LPMMTIME16 mmt16) 
{
    mmt32->wType = mmt16->wType;
    /* layout of rest is the same for 32/16,
     * Note: mmt16->u is 2 bytes smaller than mmt32->u
     */
    memcpy(&(mmt16->u), &(mmt32->u), sizeof(mmt16->u));
}

static HANDLE		PlaySound_hThread = 0;
static HANDLE		PlaySound_hPlayEvent = 0;
static HANDLE		PlaySound_hReadyEvent = 0;
static HANDLE		PlaySound_hMiddleEvent = 0;
static BOOL		PlaySound_Result = FALSE;
static int		PlaySound_Stop = FALSE;
static int		PlaySound_Playing = FALSE;

static LPCSTR		PlaySound_pszSound = NULL;
static HMODULE		PlaySound_hmod = 0;
static DWORD		PlaySound_fdwSound = 0;
static int		PlaySound_Loop = FALSE;
static int		PlaySound_SearchMode = 0; /* 1 - sndPlaySound search order
						     2 - PlaySound order */

static HMMIO16	get_mmioFromFile(LPCSTR lpszName)
{
    return mmioOpen16((LPSTR)lpszName, NULL,
		      MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
}

static HMMIO16 get_mmioFromProfile(UINT uFlags, LPCSTR lpszName) 
{
    char	str[128];
    LPSTR	ptr;
    HMMIO16  	hmmio;
    
    TRACE(mmsys, "searching in SystemSound List !\n");
    GetProfileStringA("Sounds", (LPSTR)lpszName, "", str, sizeof(str));
    if (strlen(str) == 0) {
	if (uFlags & SND_NODEFAULT) return 0;
	GetProfileStringA("Sounds", "Default", "", str, sizeof(str));
	if (strlen(str) == 0) return 0;
    }
    if ((ptr = (LPSTR)strchr(str, ',')) != NULL) *ptr = '\0';
    hmmio = get_mmioFromFile(str);
    if (hmmio == 0) {
	WARN(mmsys, "can't find SystemSound='%s' !\n", str);
	return 0;
    }
    return hmmio;
}

static BOOL16 WINAPI proc_PlaySound(LPCSTR lpszSoundName, UINT uFlags)
{
    BOOL16		bRet = FALSE;
    HMMIO16		hmmio;
    MMCKINFO		ckMainRIFF;
    
    TRACE(mmsys, "SoundName='%s' uFlags=%04X !\n", lpszSoundName, uFlags);
    if (lpszSoundName == NULL) {
	TRACE(mmsys, "Stop !\n");
	return FALSE;
    }
    if (uFlags & SND_MEMORY) {
	MMIOINFO16 mminfo;
	memset(&mminfo, 0, sizeof(mminfo));
	mminfo.fccIOProc = FOURCC_MEM;
	mminfo.pchBuffer = (LPSTR)lpszSoundName;
	mminfo.cchBuffer = -1;
	TRACE(mmsys, "Memory sound %p\n", lpszSoundName);
	hmmio = mmioOpen16(NULL, &mminfo, MMIO_READ);
    } else {
	hmmio = 0;
	if (uFlags & SND_ALIAS)
	    if ((hmmio=get_mmioFromProfile(uFlags, lpszSoundName)) == 0) 
		return FALSE;
	
	if (uFlags & SND_FILENAME)
	    if ((hmmio=get_mmioFromFile(lpszSoundName)) == 0) return FALSE;
	
	if (PlaySound_SearchMode == 1) {
	    PlaySound_SearchMode = 0;
	    if ((hmmio=get_mmioFromFile(lpszSoundName)) == 0) 
		if ((hmmio=get_mmioFromProfile(uFlags, lpszSoundName)) == 0) 
		    return FALSE;
	}
	
	if (PlaySound_SearchMode == 2) {
	    PlaySound_SearchMode = 0;
	    if ((hmmio=get_mmioFromProfile(uFlags | SND_NODEFAULT, lpszSoundName)) == 0) 
		if ((hmmio=get_mmioFromFile(lpszSoundName)) == 0)	
		    if ((hmmio=get_mmioFromProfile(uFlags, lpszSoundName)) == 0) return FALSE;
	}
    }
    
    if (mmioDescend(hmmio, &ckMainRIFF, NULL, 0) == 0) 
        do {
	    TRACE(mmsys, "ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
		  (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, 
		  ckMainRIFF.cksize);
	    
	    if ((ckMainRIFF.ckid == FOURCC_RIFF) &&
	    	(ckMainRIFF.fccType == mmioFOURCC('W', 'A', 'V', 'E'))) {
		MMCKINFO        mmckInfo;
		
		mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
		
		if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) == 0) {
		    PCMWAVEFORMAT           pcmWaveFormat;
		    
		    TRACE(mmsys, "Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
			  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);
		    
		    if (mmioRead(hmmio, (HPSTR)&pcmWaveFormat,
				   (long) sizeof(PCMWAVEFORMAT)) == (long) sizeof(PCMWAVEFORMAT)) {
			TRACE(mmsys, "wFormatTag=%04X !\n", pcmWaveFormat.wf.wFormatTag);
			TRACE(mmsys, "nChannels=%d \n", pcmWaveFormat.wf.nChannels);
			TRACE(mmsys, "nSamplesPerSec=%ld\n", pcmWaveFormat.wf.nSamplesPerSec);
			TRACE(mmsys, "nAvgBytesPerSec=%ld\n", pcmWaveFormat.wf.nAvgBytesPerSec);
			TRACE(mmsys, "nBlockAlign=%d \n", pcmWaveFormat.wf.nBlockAlign);
			TRACE(mmsys, "wBitsPerSample=%u !\n", pcmWaveFormat.wBitsPerSample);
			
			mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
			if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) == 0) {
			    WAVEOPENDESC	waveDesc;
			    DWORD		dwRet;
			    
			    TRACE(mmsys, "Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX\n", 
				  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);
			    
			    pcmWaveFormat.wf.nAvgBytesPerSec = pcmWaveFormat.wf.nSamplesPerSec * 
				pcmWaveFormat.wf.nBlockAlign;
			    waveDesc.hWave    = 0;
			    waveDesc.lpFormat = (LPWAVEFORMAT)&pcmWaveFormat;
			    
			    dwRet = wodMessage(0, WODM_OPEN, 0, (DWORD)&waveDesc, CALLBACK_NULL);
			    if (dwRet == MMSYSERR_NOERROR) {
				WAVEHDR		waveHdr;
				HGLOBAL16	hData;
				INT		count, bufsize, left = mmckInfo.cksize;
				
				bufsize = 64000;
				hData = GlobalAlloc16(GMEM_MOVEABLE, bufsize);
				waveHdr.lpData = (LPSTR)GlobalLock16(hData);
				waveHdr.dwBufferLength = bufsize;
				waveHdr.dwUser = 0L;
				waveHdr.dwFlags = 0L;
				waveHdr.dwLoops = 0L;
				
				dwRet = wodMessage(0, WODM_PREPARE, 0, (DWORD)&waveHdr, sizeof(WAVEHDR));
				if (dwRet == MMSYSERR_NOERROR) {
				    while (left) {
                                        if (PlaySound_Stop) {
					    PlaySound_Stop = FALSE;
					    PlaySound_Loop = FALSE;
					    break;
					}
					if (bufsize > left) bufsize = left;
					count = mmioRead(hmmio, waveHdr.lpData,bufsize);
					if (count < 1) break;
					left -= count;
					waveHdr.dwBufferLength = count;
				/*	waveHdr.dwBytesRecorded = count; */
					/* FIXME: doesn't expect async ops */ 
					wodMessage(0, WODM_WRITE, 0, (DWORD)&waveHdr, sizeof(WAVEHDR));
				    }
				    wodMessage(0, WODM_UNPREPARE, 0, (DWORD)&waveHdr, sizeof(WAVEHDR));
				    wodMessage(0, WODM_CLOSE, 0, 0L, 0L);
				    
				    bRet = TRUE;
				} else 
				    WARN(mmsys, "can't prepare WaveOut device !\n");
				
				GlobalUnlock16(hData);
				GlobalFree16(hData);
			    }
			}
		    }
		}
	    }
	} while (PlaySound_Loop);
    
    if (hmmio != 0) mmioClose(hmmio, 0);
    return bRet;
}

static DWORD WINAPI PlaySound_Thread(LPVOID arg) 
{
    DWORD     res;
    
    for (;;) {
	PlaySound_Playing = FALSE;
	SetEvent(PlaySound_hReadyEvent);
	res = WaitForSingleObject(PlaySound_hPlayEvent, INFINITE);
	ResetEvent(PlaySound_hReadyEvent);
	SetEvent(PlaySound_hMiddleEvent);
	if (res == WAIT_FAILED) ExitThread(2);
	if (res != WAIT_OBJECT_0) continue;
	PlaySound_Playing = TRUE;
	
	if ((PlaySound_fdwSound & SND_RESOURCE) == SND_RESOURCE) {
	    HRSRC	hRES;
	    HGLOBAL	hGLOB;
	    void*	ptr;

	    if ((hRES = FindResourceA(PlaySound_hmod, PlaySound_pszSound, "WAVE")) == 0) {
		PlaySound_Result = FALSE;
		continue;
	    }
	    if ((hGLOB = LoadResource(PlaySound_hmod, hRES)) == 0) {
		PlaySound_Result = FALSE;
		continue;
	    }
	    if ((ptr = LockResource(hGLOB)) == NULL) {
		FreeResource(hGLOB);
		PlaySound_Result = FALSE;
		continue;
	    }
	    PlaySound_Result = proc_PlaySound(ptr, 
					      ((UINT16)PlaySound_fdwSound ^ SND_RESOURCE) | SND_MEMORY);
	    FreeResource(hGLOB);
	    continue;
	}
	PlaySound_Result=proc_PlaySound(PlaySound_pszSound, (UINT16)PlaySound_fdwSound);
    }
}

/**************************************************************************
 * 				PlaySoundA		[WINMM.1]
 */
BOOL WINAPI PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
    static LPSTR StrDup = NULL;
    
    TRACE(mmsys, "pszSound='%p' hmod=%04X fdwSound=%08lX\n",
	  pszSound, hmod, fdwSound);
    
    if (PlaySound_hThread == 0) { /* This is the first time they called us */
	DWORD	id;
	if ((PlaySound_hReadyEvent = CreateEventA(NULL, TRUE, FALSE, NULL)) == 0)
	    return FALSE;
	if ((PlaySound_hMiddleEvent = CreateEventA(NULL, FALSE, FALSE, NULL)) == 0)
	    return FALSE;
	if ((PlaySound_hPlayEvent = CreateEventA(NULL, FALSE, FALSE, NULL)) == 0)
	    return FALSE;
	if ((PlaySound_hThread = CreateThread(NULL, 0, PlaySound_Thread, 0, 0, &id)) == 0) 
	    return FALSE;
    }
    
    /* FIXME? I see no difference between SND_WAIT and SND_NOSTOP ! */ 
    if ((fdwSound & (SND_NOWAIT | SND_NOSTOP)) && PlaySound_Playing) 
	return FALSE;
    
    /* Trying to stop if playing */
    if (PlaySound_Playing) PlaySound_Stop = TRUE;
    
    /* Waiting playing thread to get ready. I think 10 secs is ok & if not then leave*/
    if (WaitForSingleObject(PlaySound_hReadyEvent, 1000*10) != WAIT_OBJECT_0)
	return FALSE;
    
    if (!pszSound || (fdwSound & SND_PURGE)) 
	return FALSE; /* We stoped playing so leaving */
    
    if (PlaySound_SearchMode != 1) PlaySound_SearchMode = 2;
    if (!(fdwSound & SND_ASYNC)) {
	if (fdwSound & SND_LOOP) 
	    return FALSE;
	PlaySound_pszSound = pszSound;
	PlaySound_hmod = hmod;
	PlaySound_fdwSound = fdwSound;
	PlaySound_Result = FALSE;
	SetEvent(PlaySound_hPlayEvent);
	if (WaitForSingleObject(PlaySound_hMiddleEvent, INFINITE) != WAIT_OBJECT_0) 
	    return FALSE;
	if (WaitForSingleObject(PlaySound_hReadyEvent, INFINITE) != WAIT_OBJECT_0) 
	    return FALSE;
	return PlaySound_Result;
    } else {
	PlaySound_hmod = hmod;
	PlaySound_fdwSound = fdwSound;
	PlaySound_Result = FALSE;
	if (StrDup) {
	    HeapFree(GetProcessHeap(), 0, StrDup);
	    StrDup = NULL;
	}
	if (!((fdwSound & SND_MEMORY) || ((fdwSound & SND_RESOURCE) && 
					  !((DWORD)pszSound >> 16)) || !pszSound)) {
	    StrDup = HEAP_strdupA(GetProcessHeap(), 0,pszSound);
	    PlaySound_pszSound = StrDup;
	} else PlaySound_pszSound = pszSound;
	PlaySound_Loop = fdwSound & SND_LOOP;
	SetEvent(PlaySound_hPlayEvent);
	ResetEvent(PlaySound_hMiddleEvent);
	return TRUE;
    }
    return FALSE;
}

/**************************************************************************
 * 				PlaySoundW		[WINMM.18]
 */
BOOL WINAPI PlaySoundW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
    LPSTR 	pszSoundA;
    BOOL	bSound;
    
    if (!((fdwSound & SND_MEMORY) || ((fdwSound & SND_RESOURCE) && 
				      !((DWORD)pszSound >> 16)) || !pszSound)) {
	pszSoundA = HEAP_strdupWtoA(GetProcessHeap(), 0,pszSound);
	bSound = PlaySoundA(pszSoundA, hmod, fdwSound);
	HeapFree(GetProcessHeap(), 0,pszSoundA);
    } else  
	bSound = PlaySoundA((LPCSTR)pszSound, hmod, fdwSound);
    
    return bSound;
}

/**************************************************************************
 * 				sndPlaySoundA		[MMSYSTEM.2][WINMM135]
 */
BOOL16 WINAPI sndPlaySoundA(LPCSTR lpszSoundName, UINT16 uFlags)
{
    PlaySound_SearchMode = 1;
    return PlaySoundA(lpszSoundName, 0, uFlags);
}

/**************************************************************************
 * 				sndPlaySoundW		[WINMM.136]
 */
BOOL16 WINAPI sndPlaySoundW(LPCWSTR lpszSoundName, UINT16 uFlags)
{
    PlaySound_SearchMode = 1;
    return PlaySoundW(lpszSoundName, 0, uFlags);
}

/**************************************************************************
 * 				mmsystemGetVersion	[WINMM.134]
 */
UINT WINAPI mmsystemGetVersion()
{
    return mmsystemGetVersion16();
}

/**************************************************************************
 * 				mmsystemGetVersion	[MMSYSTEM.5]
 * return value borrowed from Win95 winmm.dll ;)
 */
UINT16 WINAPI mmsystemGetVersion16()
{
    TRACE(mmsys, "3.10 (Win95?)\n");
    return 0x030a;
}

/**************************************************************************
 * 				DriverProc			[MMSYSTEM.6]
 */
LRESULT WINAPI DriverProc16(DWORD dwDevID, HDRVR16 hDrv, WORD wMsg, 
			    DWORD dwParam1, DWORD dwParam2)
{
    return DrvDefDriverProc(dwDevID, hDrv, wMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				DriverCallback			[MMSYSTEM.31]
 */
BOOL16 WINAPI DriverCallback16(DWORD dwCallBack, UINT16 uFlags, HANDLE16 hDev, 
			       WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
    TRACE(mmsys, "(%08lX, %04X, %04X, %04X, %08lX, %08lX, %08lX); !\n",
	  dwCallBack, uFlags, hDev, wMsg, dwUser, dwParam1, dwParam2);

    switch (uFlags & DCB_TYPEMASK) {
    case DCB_NULL:
	TRACE(mmsys, "CALLBACK_NULL !\n");
	break;
    case DCB_WINDOW:
	TRACE(mmsys, "CALLBACK_WINDOW = %04lX handle = %04X!\n",
	      dwCallBack, hDev);
	if (!IsWindow(dwCallBack) || USER_HEAP_LIN_ADDR(hDev) == NULL)
	    return FALSE;
	
	PostMessageA((HWND16)dwCallBack, wMsg, hDev, dwParam1);
	break;
    case DCB_TASK:
	TRACE(mmsys, "CALLBACK_TASK !\n");
	return FALSE;
    case DCB_FUNCTION:
	TRACE(mmsys, "CALLBACK_FUNCTION !\n");
	Callbacks->CallDriverCallback((FARPROC16)dwCallBack,
				       hDev, wMsg, dwUser,
				       dwParam1, dwParam2 );
	break;
    case DCB_FUNC32:
	TRACE(mmsys, "CALLBACK_FUNCTION !\n");
	((LPDRVCALLBACK)dwCallBack)(hDev, wMsg, dwUser,
				       dwParam1, dwParam2 );
	break;
    default:
	WARN(mmsys, "Unknown callback type\n");
	break;
    }
    return TRUE;
}

/**************************************************************************
 * 	Mixer devices. New to Win95
 */

/**************************************************************************
 * find out the real mixer ID depending on hmix (depends on dwFlags)
 * FIXME: also fix dwInstance passing to mixMessage 
 */
static UINT MIXER_GetDevID(HMIXEROBJ hmix, DWORD dwFlags) 
{
    /* FIXME: Check dwFlags for MIXER_OBJSECTF_xxxx entries and modify hmix 
     * accordingly. For now we always use mixerdevice 0. 
     */
    return 0;
}

/**************************************************************************
 * 				mixerGetNumDevs			[WINMM.108]
 */
UINT WINAPI mixerGetNumDevs(void) 
{
    UINT16	count = mixMessage(0, MXDM_GETNUMDEVS, 0L, 0L, 0L);
    
    TRACE(mmaux,"mixerGetNumDevs returns %d\n",count);
    return count;
}

/**************************************************************************
 * 				mixerGetNumDevs			[MMSYSTEM.800]
 */
UINT16 WINAPI mixerGetNumDevs16() 
{
    return mixerGetNumDevs();
}

/**************************************************************************
 * 				mixerGetDevCapsA		[WINMM.101]
 */
UINT WINAPI mixerGetDevCapsA(UINT devid, LPMIXERCAPSA mixcaps, UINT size) 
{
    return mixMessage(devid, MXDM_GETDEVCAPS, 0L, (DWORD)mixcaps, (DWORD)size);
}

/**************************************************************************
 * 				mixerGetDevCapsW		[WINMM.102]
 */
UINT WINAPI mixerGetDevCapsW(UINT devid, LPMIXERCAPSW mixcaps, UINT size) 
{
    MIXERCAPSA	micA;
    UINT	ret = mixerGetDevCapsA(devid, &micA, sizeof(micA));

    if (ret == MMSYSERR_NOERROR) {
	mixcaps->wMid           = micA.wMid;
	mixcaps->wPid           = micA.wPid;
	mixcaps->vDriverVersion = micA.vDriverVersion;
	lstrcpyAtoW(mixcaps->szPname, micA.szPname);
	mixcaps->fdwSupport     = micA.fdwSupport;
	mixcaps->cDestinations  = micA.cDestinations;
    }
    return ret;
}

/**************************************************************************
 * 				mixerGetDevCaps			[MMSYSTEM.801]
 */
UINT16 WINAPI mixerGetDevCaps16(UINT16 devid, LPMIXERCAPS16 mixcaps, UINT16 size) 
{
    MIXERCAPSA	micA;
    UINT	ret = mixerGetDevCapsA(devid, &micA, sizeof(micA));
    
    if (ret == MMSYSERR_NOERROR) {
	mixcaps->wMid           = micA.wMid;
	mixcaps->wPid           = micA.wPid;
	mixcaps->vDriverVersion = micA.vDriverVersion;
	strcpy(PTR_SEG_TO_LIN(mixcaps->szPname), micA.szPname);
	mixcaps->fdwSupport     = micA.fdwSupport;
	mixcaps->cDestinations  = micA.cDestinations;
    }
    return ret;
}

/**************************************************************************
 * 				mixerOpen			[WINMM.110]
 */
UINT WINAPI mixerOpen(LPHMIXER lphmix, UINT uDeviceID, DWORD dwCallback,
		      DWORD dwInstance, DWORD fdwOpen) 
{
    HMIXER16	hmix16;
    UINT	ret;
    
    FIXME(mmsys,"(%p,%d,%08lx,%08lx,%08lx): semi stub?\n",
	  lphmix,uDeviceID, dwCallback, dwInstance, fdwOpen);
    ret = mixerOpen16(&hmix16,uDeviceID, dwCallback, dwInstance,fdwOpen);
    if (lphmix) *lphmix = hmix16;
    return ret;
}

/**************************************************************************
 * 				mixerOpen			[MMSYSTEM.803]
 */
UINT16 WINAPI mixerOpen16(LPHMIXER16 lphmix, UINT16 uDeviceID, DWORD dwCallback,
			  DWORD dwInstance, DWORD fdwOpen) 
{
    HMIXER16		hmix;
    LPMIXEROPENDESC	lpmod;
    BOOL		mapperflag = (uDeviceID==0);
    DWORD		dwRet=0;
    
    TRACE(mmsys,"(%p,%d,%08lx,%08lx,%08lx)\n",
	  lphmix,uDeviceID, dwCallback, dwInstance, fdwOpen);
    hmix = USER_HEAP_ALLOC(sizeof(MIXEROPENDESC));
    if (lphmix) *lphmix = hmix;
    lpmod = (LPMIXEROPENDESC)USER_HEAP_LIN_ADDR(hmix);
    lpmod->hmx = hmix;
    lpmod->dwCallback = dwCallback;
    lpmod->dwInstance = dwInstance;
    if (uDeviceID >= MAXMIXERDRIVERS)
	uDeviceID = 0;
    while (uDeviceID < MAXMIXERDRIVERS) {
	dwRet = mixMessage(uDeviceID, MXDM_OPEN, dwInstance, (DWORD)lpmod, fdwOpen);
	if (dwRet == MMSYSERR_NOERROR) break;
	if (!mapperflag) break;
	uDeviceID++;
    }
    lpmod->uDeviceID = uDeviceID;
    return dwRet;
}

/**************************************************************************
 * 				mixerClose			[WINMM.98]
 */
UINT WINAPI mixerClose(HMIXER hmix) 
{
    LPMIXEROPENDESC	lpmod;
    
    FIXME(mmsys,"(%04x): semi-stub?\n", hmix);

    lpmod = (LPMIXEROPENDESC)USER_HEAP_LIN_ADDR(hmix);
    return mixMessage(lpmod->uDeviceID, MXDM_CLOSE, lpmod->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				mixerClose			[MMSYSTEM.803]
 */
UINT16 WINAPI mixerClose16(HMIXER16 hmix) 
{
    return mixerClose(hmix);
}

/**************************************************************************
 * 				mixerGetID			[WINMM.103]
 */
UINT WINAPI mixerGetID(HMIXEROBJ hmix, LPUINT lpid, DWORD fdwID) 
{
    FIXME(mmsys,"(%04x %p %08lx): semi-stub\n", hmix, lpid, fdwID);

    if (lpid)
      *lpid = MIXER_GetDevID(hmix, fdwID);

    return MMSYSERR_NOERROR; /* FIXME: many error possibilities */
}

/**************************************************************************
 * 				mixerGetID
 */
UINT16 WINAPI mixerGetID16(HMIXEROBJ16 hmix, LPUINT16 lpid, DWORD fdwID) 
{
    UINT	xid;    
    UINT	ret = mixerGetID(hmix, &xid, fdwID);

    if (lpid) 
	*lpid = xid;
    return ret;
}

/**************************************************************************
 * 				mixerGetControlDetailsA	[WINMM.99]
 */
UINT WINAPI mixerGetControlDetailsA(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcd, DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n", hmix, lpmcd, fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetControlDetailsW	[WINMM.100]
 */
UINT WINAPI mixerGetControlDetailsW(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcd, DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n", hmix, lpmcd, fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetControlDetails	[MMSYSTEM.808]
 */
UINT16 WINAPI mixerGetControlDetails16(HMIXEROBJ16 hmix, LPMIXERCONTROLDETAILS16 lpmcd, DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n", hmix, lpmcd, fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetLineControlsA	[WINMM.104]
 */
UINT WINAPI mixerGetLineControlsA(HMIXEROBJ hmix, LPMIXERLINECONTROLSA lpmlc, DWORD fdwControls) 
{
    UINT	uDevID;

    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n", hmix, lpmlc, fdwControls);

    uDevID = MIXER_GetDevID(hmix, 0);

    return mixMessage(uDevID, MXDM_GETLINECONTROLS, 0, (DWORD)lpmlc, fdwControls);
}

/**************************************************************************
 * 				mixerGetLineControlsW		[WINMM.105]
 */
UINT WINAPI mixerGetLineControlsW(HMIXEROBJ hmix, LPMIXERLINECONTROLSW lpmlc, DWORD fdwControls) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n", hmix, lpmlc, fdwControls);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetLineControls		[MMSYSTEM.807]
 */
UINT16 WINAPI mixerGetLineControls16(HMIXEROBJ16 hmix, LPMIXERLINECONTROLS16 lpmlc, DWORD fdwControls) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n", hmix, lpmlc, fdwControls);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetLineInfoA		[WINMM.106]
 */
UINT WINAPI mixerGetLineInfoA(HMIXEROBJ hmix, LPMIXERLINEA lpml, DWORD fdwInfo) 
{
    UINT16 devid;
    
    TRACE(mmsys, "(%04x, %p, %08lx)\n", hmix, lpml, fdwInfo);
    
    /* FIXME: I'm not sure of the flags */
    devid = MIXER_GetDevID(hmix, fdwInfo);
    return mixMessage(devid, MXDM_GETLINEINFO, 0, (DWORD)lpml, fdwInfo);
}

/**************************************************************************
 * 				mixerGetLineInfoW		[WINMM.107]
 */
UINT WINAPI mixerGetLineInfoW(HMIXEROBJ hmix, LPMIXERLINEW lpml, DWORD fdwInfo) 
{
    MIXERLINEA		mlA;
    UINT		ret;
    
    TRACE(mmsys,"(%04x,%p,%08lx)\n", hmix, lpml, fdwInfo);

    if (lpml == NULL || lpml->cbStruct != sizeof(*lpml)) 
	return MMSYSERR_INVALPARAM;

    mlA.cbStruct = sizeof(mlA);
    mlA.dwDestination = lpml->dwDestination;
    mlA.dwSource = lpml->dwSource; 
    mlA.dwLineID = lpml->dwLineID; 
    mlA.dwUser = lpml->dwUser; 
    mlA.dwComponentType = lpml->dwComponentType; 
    mlA.cChannels = lpml->cChannels; 
    mlA.cConnections = lpml->cConnections; 
    mlA.cControls = lpml->cControls; 

    ret = mixerGetLineInfoA(hmix, &mlA, fdwInfo);

    lpml->dwDestination = mlA.dwDestination;
    lpml->dwSource = mlA.dwSource;
    lpml->dwLineID = mlA.dwLineID;
    lpml->fdwLine = mlA.fdwLine;
    lpml->dwUser = mlA.dwUser;
    lpml->dwComponentType = mlA.dwComponentType;
    lpml->cChannels = mlA.cChannels;
    lpml->cConnections = mlA.cConnections;
    lpml->cControls = mlA.cControls;
    lstrcpyAtoW(lpml->szShortName, mlA.szShortName);
    lstrcpyAtoW(lpml->szName, mlA.szName);
    lpml->Target.dwType = mlA.Target.dwType;
    lpml->Target.dwDeviceID = mlA.Target.dwDeviceID;
    lpml->Target.wMid = mlA.Target.wMid;
    lpml->Target.wPid = mlA.Target.wPid;
    lpml->Target.vDriverVersion = mlA.Target.vDriverVersion;
    lstrcpyAtoW(lpml->Target.szPname, mlA.Target.szPname);

    return ret;
}

/**************************************************************************
 * 				mixerGetLineInfo	[MMSYSTEM.805]
 */
UINT16 WINAPI mixerGetLineInfo16(HMIXEROBJ16 hmix, LPMIXERLINE16 lpml, DWORD fdwInfo) 
{
    MIXERLINEA		mlA;
    UINT		ret;
    
    TRACE(mmsys, "(%04x,%p,%08lx)\n", hmix, lpml, fdwInfo);

    if (lpml == NULL || lpml->cbStruct != sizeof(*lpml)) 
	return MMSYSERR_INVALPARAM;

    mlA.cbStruct           	= sizeof(mlA);
    mlA.dwDestination      	= lpml->dwDestination;
    mlA.dwSource           	= lpml->dwSource; 
    mlA.dwLineID           	= lpml->dwLineID; 
    mlA.dwUser             	= lpml->dwUser; 
    mlA.dwComponentType    	= lpml->dwComponentType; 
    mlA.cChannels          	= lpml->cChannels; 
    mlA.cConnections       	= lpml->cConnections; 
    mlA.cControls          	= lpml->cControls; 

    ret = mixerGetLineInfoA(hmix, &mlA, fdwInfo);

    lpml->dwDestination     	= mlA.dwDestination;
    lpml->dwSource          	= mlA.dwSource;
    lpml->dwLineID          	= mlA.dwLineID;
    lpml->fdwLine           	= mlA.fdwLine;
    lpml->dwUser            	= mlA.dwUser;
    lpml->dwComponentType   	= mlA.dwComponentType;
    lpml->cChannels         	= mlA.cChannels;
    lpml->cConnections      	= mlA.cConnections;
    lpml->cControls         	= mlA.cControls;
    strcpy(PTR_SEG_TO_LIN(lpml->szShortName), mlA.szShortName);
    strcpy(PTR_SEG_TO_LIN(lpml->szName), mlA.szName);
    lpml->Target.dwType     	= mlA.Target.dwType;
    lpml->Target.dwDeviceID 	= mlA.Target.dwDeviceID;
    lpml->Target.wMid       	= mlA.Target.wMid;
    lpml->Target.wPid           = mlA.Target.wPid;
    lpml->Target.vDriverVersion = mlA.Target.vDriverVersion;
    strcpy(PTR_SEG_TO_LIN(lpml->Target.szPname), mlA.Target.szPname);
    return ret;
}

/**************************************************************************
 * 				mixerSetControlDetails	[WINMM.111]
 */
UINT WINAPI mixerSetControlDetails(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcd, DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n", hmix, lpmcd, fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerSetControlDetails	[MMSYSTEM.809]
 */
UINT16 WINAPI mixerSetControlDetails16(HMIXEROBJ16 hmix, LPMIXERCONTROLDETAILS16 lpmcd, DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n", hmix, lpmcd, fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerMessage		[WINMM.109]
 */
UINT WINAPI mixerMessage(HMIXER hmix, UINT uMsg, DWORD dwParam1, DWORD dwParam2) 
{
    LPMIXEROPENDESC	lpmod;
    UINT16		uDeviceID;
    
    lpmod = (LPMIXEROPENDESC)USER_HEAP_LIN_ADDR(hmix);
    if (lpmod)
	uDeviceID = lpmod->uDeviceID;
    else
	uDeviceID = 0;
    FIXME(mmsys,"(%04lx,%d,%08lx,%08lx): semi-stub?\n",
	  (DWORD)hmix,uMsg, dwParam1, dwParam2);
    return mixMessage(uDeviceID,uMsg, 0L, dwParam1, dwParam2);
}

/**************************************************************************
 * 				mixerMessage		[MMSYSTEM.804]
 */
UINT16 WINAPI mixerMessage16(HMIXER16 hmix, UINT16 uMsg, DWORD dwParam1, DWORD dwParam2) 
{
    LPMIXEROPENDESC	lpmod;
    UINT16		uDeviceID;
    
    lpmod = (LPMIXEROPENDESC)USER_HEAP_LIN_ADDR(hmix);
    uDeviceID = (lpmod) ? lpmod->uDeviceID : 0;
    FIXME(mmsys,"(%04x,%d,%08lx,%08lx) - semi-stub?\n",
	  hmix,uMsg, dwParam1, dwParam2);
    return mixMessage(uDeviceID,uMsg, 0L, dwParam1, dwParam2);
}

/**************************************************************************
 * 				auxGetNumDevs		[WINMM.22]
 */
UINT WINAPI auxGetNumDevs()
{
    return auxGetNumDevs16();
}

/**************************************************************************
 * 				auxGetNumDevs		[MMSYSTEM.350]
 */
UINT16 WINAPI auxGetNumDevs16()
{
    UINT16	count;

    TRACE(mmsys, "\n");
    count = auxMessage(0, AUXDM_GETNUMDEVS, 0L, 0L, 0L);
    TRACE(mmsys, "=> %u\n", count);
    return count;
}

/**************************************************************************
 * 				auxGetDevCaps		[WINMM.20]
 */
UINT WINAPI auxGetDevCapsW(UINT uDeviceID, LPAUXCAPSW lpCaps, UINT uSize)
{
    AUXCAPS16	ac16;
    UINT	ret = auxGetDevCaps16(uDeviceID, &ac16, sizeof(ac16));
    
    lpCaps->wMid = ac16.wMid;
    lpCaps->wPid = ac16.wPid;
    lpCaps->vDriverVersion = ac16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname,ac16.szPname);
    lpCaps->wTechnology = ac16.wTechnology;
    lpCaps->dwSupport = ac16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				auxGetDevCaps		[WINMM.21]
 */
UINT WINAPI auxGetDevCapsA(UINT uDeviceID, LPAUXCAPSA lpCaps, UINT uSize)
{
    AUXCAPS16	ac16;
    UINT	ret = auxGetDevCaps16(uDeviceID, &ac16, sizeof(ac16));
    
    lpCaps->wMid = ac16.wMid;
    lpCaps->wPid = ac16.wPid;
    lpCaps->vDriverVersion = ac16.vDriverVersion;
    strcpy(lpCaps->szPname,ac16.szPname);
    lpCaps->wTechnology = ac16.wTechnology;
    lpCaps->dwSupport = ac16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				auxGetDevCaps		[MMSYSTEM.351]
 */
UINT16 WINAPI auxGetDevCaps16(UINT16 uDeviceID, LPAUXCAPS16 lpCaps, UINT16 uSize)
{
    TRACE(mmsys, "(%04X, %p, %d) !\n", uDeviceID, lpCaps, uSize);

    return auxMessage(uDeviceID, AUXDM_GETDEVCAPS,
		      0L, (DWORD)lpCaps, (DWORD)uSize);
}

/**************************************************************************
 * 				auxGetVolume		[WINM.23]
 */
UINT WINAPI auxGetVolume(UINT uDeviceID, DWORD* lpdwVolume)
{
    return auxGetVolume16(uDeviceID, lpdwVolume);
}

/**************************************************************************
 * 				auxGetVolume		[MMSYSTEM.352]
 */
UINT16 WINAPI auxGetVolume16(UINT16 uDeviceID, DWORD* lpdwVolume)
{
    TRACE(mmsys, "(%04X, %p) !\n", uDeviceID, lpdwVolume);

    return auxMessage(uDeviceID, AUXDM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
}

/**************************************************************************
 * 				auxSetVolume		[WINMM.25]
 */
UINT WINAPI auxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
    return auxSetVolume16(uDeviceID, dwVolume);
}

/**************************************************************************
 * 				auxSetVolume		[MMSYSTEM.353]
 */
UINT16 WINAPI auxSetVolume16(UINT16 uDeviceID, DWORD dwVolume)
{
    TRACE(mmsys, "(%04X, %08lX) !\n", uDeviceID, dwVolume);

    return auxMessage(uDeviceID, AUXDM_SETVOLUME, 0L, dwVolume, 0L);
}

/**************************************************************************
 * 				auxOutMessage		[MMSYSTEM.354]
 */
DWORD WINAPI auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD dw1, DWORD dw2)
{
    switch (uMessage) {
    case AUXDM_GETNUMDEVS:
    case AUXDM_GETVOLUME:
    case AUXDM_SETVOLUME:
	/* no argument conversion needed */
	break;
    case AUXDM_GETDEVCAPS:
	return auxGetDevCapsA(uDeviceID, (LPAUXCAPSA)dw1, dw2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    uDeviceID,uMessage, dw1, dw2);
	break;
    }
    return auxMessage(uDeviceID,uMessage, 0L, dw1, dw2);
}

/**************************************************************************
 * 				auxOutMessage		[MMSYSTEM.354]
 */
DWORD WINAPI auxOutMessage16(UINT16 uDeviceID, UINT16 uMessage, DWORD dw1, DWORD dw2)
{
    TRACE(mmsys, "(%04X, %04X, %08lX, %08lX)\n", uDeviceID, uMessage, dw1, dw2);

    switch (uMessage) {
    case AUXDM_GETNUMDEVS:
    case AUXDM_SETVOLUME:
	/* no argument conversion needed */
	break;
    case AUXDM_GETVOLUME:
	return auxGetVolume16(uDeviceID, (LPDWORD)PTR_SEG_TO_LIN(dw1));
    case AUXDM_GETDEVCAPS:
	return auxGetDevCaps16(uDeviceID, (LPAUXCAPS16)PTR_SEG_TO_LIN(dw1), dw2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    uDeviceID,uMessage, dw1, dw2);
	break;
    }
    return auxMessage(uDeviceID, uMessage, 0L, dw1, dw2);
}

/**************************************************************************
 * 				mciGetErrorStringW		[WINMM.46]
 */
BOOL WINAPI mciGetErrorStringW(DWORD wError, LPWSTR lpstrBuffer, UINT uLength)
{
    LPSTR	bufstr = HeapAlloc(GetProcessHeap(), 0,uLength);
    BOOL	ret = mciGetErrorStringA(wError, bufstr, uLength);
    
    lstrcpyAtoW(lpstrBuffer, bufstr);
    HeapFree(GetProcessHeap(), 0, bufstr);
    return ret;
}

/**************************************************************************
 * 				mciGetErrorStringA		[WINMM.45]
 */
BOOL WINAPI mciGetErrorStringA(DWORD wError, LPSTR lpstrBuffer, UINT uLength)
{
    return mciGetErrorString16(wError, lpstrBuffer,uLength);
}

/**************************************************************************
 * 				mciGetErrorString		[MMSYSTEM.706]
 */
BOOL16 WINAPI mciGetErrorString16(DWORD wError, LPSTR lpstrBuffer, UINT16 uLength)
{
    LPSTR	msgptr;
    BOOL16	ret = TRUE;

    TRACE(mmsys, "(%08lX, %p, %d);\n", wError, lpstrBuffer, uLength);

    if ((lpstrBuffer == NULL) || (uLength < 1)) 
	return FALSE;

    lpstrBuffer[0] = '\0';

    switch (wError) {
    case 0:
	msgptr = "The specified command has been executed.";
	break;
    case MCIERR_INVALID_DEVICE_ID:
	msgptr = "Invalid MCI device ID. Use the ID returned when opening the MCI device.";
	break;
    case MCIERR_UNRECOGNIZED_KEYWORD:
	msgptr = "The driver cannot recognize the specified command parameter.";
	break;
    case MCIERR_UNRECOGNIZED_COMMAND:
	msgptr = "The driver cannot recognize the specified command.";
	break;
    case MCIERR_HARDWARE:
	msgptr = "There is a problem with your media device. Make sure it is working correctly or contact the device manufacturer.";
	break;
    case MCIERR_INVALID_DEVICE_NAME:
	msgptr = "The specified device is not open or is not recognized by MCI.";
	break;
    case MCIERR_OUT_OF_MEMORY:
	msgptr = "Not enough memory available for this task. \nQuit one or more applications to increase available memory, and then try again.";
	break;
    case MCIERR_DEVICE_OPEN:
	msgptr = "The device name is already being used as an alias by this application. Use a unique alias.";
	break;
    case MCIERR_CANNOT_LOAD_DRIVER:
	msgptr = "There is an undetectable problem in loading the specified device driver.";
	break;
    case MCIERR_MISSING_COMMAND_STRING:
	msgptr = "No command was specified.";
	break;
    case MCIERR_PARAM_OVERFLOW:
	msgptr = "The output string was to large to fit in the return buffer. Increase the size of the buffer.";
	break;
    case MCIERR_MISSING_STRING_ARGUMENT:
	msgptr = "The specified command requires a character-string parameter. Please provide one.";
	break;
    case MCIERR_BAD_INTEGER:
	msgptr = "The specified integer is invalid for this command.";
	break;
    case MCIERR_PARSER_INTERNAL:
	msgptr = "The device driver returned an invalid return type. Check with the device manufacturer about obtaining a new driver.";
	break;
    case MCIERR_DRIVER_INTERNAL:
	msgptr = "There is a problem with the device driver. Check with the device manufacturer about obtaining a new driver.";
	break;
    case MCIERR_MISSING_PARAMETER:
	msgptr = "The specified command requires a parameter. Please supply one.";
	break;
    case MCIERR_UNSUPPORTED_FUNCTION:
	msgptr = "The MCI device you are using does not support the specified command.";
	break;
    case MCIERR_FILE_NOT_FOUND:
	msgptr = "Cannot find the specified file. Make sure the path and filename are correct.";
	break;
    case MCIERR_DEVICE_NOT_READY:
	msgptr = "The device driver is not ready.";
	break;
    case MCIERR_INTERNAL:
	msgptr = "A problem occurred in initializing MCI. Try restarting Windows.";
	break;
    case MCIERR_DRIVER:
	msgptr = "There is a problem with the device driver. The driver has closed. Cannot access error.";
	break;
    case MCIERR_CANNOT_USE_ALL:
	msgptr = "Cannot use 'all' as the device name with the specified command.";
	break;
    case MCIERR_MULTIPLE:
	msgptr = "Errors occurred in more than one device. Specify each command and device separately to determine which devices caused the error";
	break;
    case MCIERR_EXTENSION_NOT_FOUND:
	msgptr = "Cannot determine the device type from the given filename extension.";
	break;
    case MCIERR_OUTOFRANGE:
	msgptr = "The specified parameter is out of range for the specified command.";
	break;
    case MCIERR_FLAGS_NOT_COMPATIBLE:
	msgptr = "The specified parameters cannot be used together.";
	break;
    case MCIERR_FILE_NOT_SAVED:
	msgptr = "Cannot save the specified file. Make sure you have enough disk space or are still connected to the network.";
	break;
    case MCIERR_DEVICE_TYPE_REQUIRED:
	msgptr = "Cannot find the specified device. Make sure it is installed or that the device name is spelled correctly.";
	break;
    case MCIERR_DEVICE_LOCKED:
	msgptr = "The specified device is now being closed. Wait a few seconds, and then try again.";
	break;
    case MCIERR_DUPLICATE_ALIAS:
	msgptr = "The specified alias is already being used in this application. Use a unique alias.";
	break;
    case MCIERR_BAD_CONSTANT:
	msgptr = "The specified parameter is invalid for this command.";
	break;
    case MCIERR_MUST_USE_SHAREABLE:
	msgptr = "The device driver is already in use. To share it, use the 'shareable' parameter with each 'open' command.";
	break;
    case MCIERR_MISSING_DEVICE_NAME:
	msgptr = "The specified command requires an alias, file, driver, or device name. Please supply one.";
	break;
    case MCIERR_BAD_TIME_FORMAT:
	msgptr = "The specified value for the time format is invalid. Refer to the MCI documentation for valid formats.";
	break;
    case MCIERR_NO_CLOSING_QUOTE:
	msgptr = "A closing double-quotation mark is missing from the parameter value. Please supply one.";
	break;
    case MCIERR_DUPLICATE_FLAGS:
	msgptr = "A parameter or value was specified twice. Only specify it once.";
	break;
    case MCIERR_INVALID_FILE:
	msgptr = "The specified file cannot be played on the specified MCI device. The file may be corrupt, or not in the correct format.";
	break;
    case MCIERR_NULL_PARAMETER_BLOCK:
	msgptr = "A null parameter block was passed to MCI.";
	break;
    case MCIERR_UNNAMED_RESOURCE:
	msgptr = "Cannot save an unnamed file. Supply a filename.";
	break;
    case MCIERR_NEW_REQUIRES_ALIAS:
	msgptr = "You must specify an alias when using the 'new' parameter.";
	break;
    case MCIERR_NOTIFY_ON_AUTO_OPEN:
	msgptr = "Cannot use the 'notify' flag with auto-opened devices.";
	break;
    case MCIERR_NO_ELEMENT_ALLOWED:
	msgptr = "Cannot use a filename with the specified device.";
	break;
    case MCIERR_NONAPPLICABLE_FUNCTION:
	msgptr = "Cannot carry out the commands in the order specified. Correct the command sequence, and then try again.";
	break;
    case MCIERR_ILLEGAL_FOR_AUTO_OPEN:
	msgptr = "Cannot carry out the specified command on an auto-opened device. Wait until the device is closed, and then try again.";
	break;
    case MCIERR_FILENAME_REQUIRED:
	msgptr = "The filename is invalid. Make sure the filename is not longer than 8 characters, followed by a period and an extension.";
	break;
    case MCIERR_EXTRA_CHARACTERS:
	msgptr = "Cannot specify extra characters after a string enclosed in quotation marks.";
	break;
    case MCIERR_DEVICE_NOT_INSTALLED:
	msgptr = "The specified device is not installed on the system. Use the Drivers option in Control Panel to install the device.";
	break;
    case MCIERR_GET_CD:
	msgptr = "Cannot access the specified file or MCI device. Try changing directories or restarting your computer.";
	break;
    case MCIERR_SET_CD:
	msgptr = "Cannot access the specified file or MCI device because the application cannot change directories.";
	break;
    case MCIERR_SET_DRIVE:
	msgptr = "Cannot access specified file or MCI device because the application cannot change drives.";
	break;
    case MCIERR_DEVICE_LENGTH:
	msgptr = "Specify a device or driver name that is less than 79 characters.";
	break;
    case MCIERR_DEVICE_ORD_LENGTH:
	msgptr = "Specify a device or driver name that is less than 69 characters.";
	break;
    case MCIERR_NO_INTEGER:
	msgptr = "The specified command requires an integer parameter. Please provide one.";
	break;
    case MCIERR_WAVE_OUTPUTSINUSE:
	msgptr = "All wave devices that can play files in the current format are in use. Wait until a wave device is free, and then try again.";
	break;
    case MCIERR_WAVE_SETOUTPUTINUSE:
	msgptr = "Cannot set the current wave device for play back because it is in use. Wait until the device is free, and then try again.";
	break;
    case MCIERR_WAVE_INPUTSINUSE:
	msgptr = "All wave devices that can record files in the current format are in use. Wait until a wave device is free, and then try again.";
	break;
    case MCIERR_WAVE_SETINPUTINUSE:
	msgptr = "Cannot set the current wave device for recording because it is in use. Wait until the device is free, and then try again.";
	break;
    case MCIERR_WAVE_OUTPUTUNSPECIFIED:
	msgptr = "Any compatible waveform playback device may be used.";
	break;
    case MCIERR_WAVE_INPUTUNSPECIFIED:
	msgptr = "Any compatible waveform recording device may be used.";
	break;
    case MCIERR_WAVE_OUTPUTSUNSUITABLE:
	msgptr = "No wave device that can play files in the current format is installed. Use the Drivers option to install the wave device.";
	break;
    case MCIERR_WAVE_SETOUTPUTUNSUITABLE:
	msgptr = "The device you are trying to play to cannot recognize the current file format.";
	break;
    case MCIERR_WAVE_INPUTSUNSUITABLE:
	msgptr = "No wave device that can record files in the current format is installed. Use the Drivers option to install the wave device.";
	break;
    case MCIERR_WAVE_SETINPUTUNSUITABLE:
	msgptr = "The device you are trying to record from cannot recognize the current file format.";
	break;
    case MCIERR_NO_WINDOW:
	msgptr = "There is no display window.";
	break;
    case MCIERR_CREATEWINDOW:
	msgptr = "Could not create or use window.";
	break;
    case MCIERR_FILE_READ:
	msgptr = "Cannot read the specified file. Make sure the file is still present, or check your disk or network connection.";
	break;
    case MCIERR_FILE_WRITE:
	msgptr = "Cannot write to the specified file. Make sure you have enough disk space or are still connected to the network.";
	break;
    case MCIERR_SEQ_DIV_INCOMPATIBLE:
	msgptr = "The time formats of the \"song pointer\" and SMPTE are mutually exclusive. You can't use them together.";
	break;
    case MCIERR_SEQ_NOMIDIPRESENT:
	msgptr = "The system has no installed MIDI devices. Use the Drivers option from the Control Panel to install a MIDI driver.";
	break;
    case MCIERR_SEQ_PORT_INUSE:
	msgptr = "The specified MIDI port is already in use. Wait until it is free; the try again.";
	break;
    case MCIERR_SEQ_PORT_MAPNODEVICE:
	msgptr = "The current MIDI Mapper setup refers to a MIDI device that is not installed on the system. Use the MIDI Mapper option from the Control Panel to edit the setup.";
	break;
    case MCIERR_SEQ_PORT_MISCERROR:
	msgptr = "An error occurred with the specified port.";
	break;
    case MCIERR_SEQ_PORT_NONEXISTENT:
	msgptr = "The specified MIDI device is not installed on the system. Use the Drivers option from the Control Panel to install a MIDI device.";
	break;
    case MCIERR_SEQ_PORTUNSPECIFIED:
	msgptr = "The system doesnot have a current MIDI port specified.";
	break;
    case MCIERR_SEQ_TIMER:
	msgptr = "All multimedia timers are being used by other applications. Quit one of these applications; then, try again.";
	break;
	
	/* 
	   msg# 513 : vcr
	   msg# 514 : videodisc
	   msg# 515 : overlay
	   msg# 516 : cdaudio
	   msg# 517 : dat
	   msg# 518 : scanner
	   msg# 519 : animation
	   msg# 520 : digitalvideo
	   msg# 521 : other
	   msg# 522 : waveaudio
	   msg# 523 : sequencer
	   msg# 524 : not ready
	   msg# 525 : stopped
	   msg# 526 : playing
	   msg# 527 : recording
	   msg# 528 : seeking
	   msg# 529 : paused
	   msg# 530 : open
	   msg# 531 : false
	   msg# 532 : true
	   msg# 533 : milliseconds
	   msg# 534 : hms
	   msg# 535 : msf
	   msg# 536 : frames
	   msg# 537 : smpte 24
	   msg# 538 : smpte 25
	   msg# 539 : smpte 30
	   msg# 540 : smpte 30 drop
	   msg# 541 : bytes
	   msg# 542 : samples
	   msg# 543 : tmsf
	*/
    default:
	msgptr = "Unknown MCI Error !\n";
	ret = FALSE;
	break;
    }
    lstrcpynA(lpstrBuffer, msgptr, uLength);
    TRACE(mmsys, "msg = %s;\n", msgptr);
    return ret;
}

/**************************************************************************
 * 				mciDriverNotify			[MMSYSTEM.711]
 */
BOOL16 WINAPI mciDriverNotify16(HWND16 hWndCallBack, UINT16 wDevID, UINT16 wStatus)
{
    TRACE(mmsys, "(%04X, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    if (!IsWindow(hWndCallBack)) {
	WARN(mmsys, "bad hWnd for call back\n");
	return FALSE;
    }
    TRACE(mmsys, "before PostMessage\n");
    PostMessageA(hWndCallBack, MM_MCINOTIFY, wStatus, wDevID);
    return TRUE;
}

/**************************************************************************
 *			mciDriverNotify				[WINMM.36]
 */
BOOL WINAPI mciDriverNotify(HWND hWndCallBack, UINT wDevID, UINT wStatus)
{
    FIXME(mmsys, "(%08X, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    if (!IsWindow(hWndCallBack)) {
	WARN(mmsys, "bad hWnd for call back\n");
	return FALSE;
    }
    TRACE(mmsys, "before PostMessage\n");
    PostMessageA(hWndCallBack, MM_MCINOTIFY, wStatus, wDevID);
    return TRUE;
}

/**************************************************************************
 * 			mciGetDriverData			[MMSYSTEM.708]
 */
DWORD WINAPI mciGetDriverData16(UINT16 uDeviceID) 
{
    return mciGetDriverData(uDeviceID);
}

/**************************************************************************
 * 			mciGetDriverData			[WINMM.44]
 */
DWORD WINAPI mciGetDriverData(UINT uDeviceID) 
{
    TRACE(mmsys,"(%04x)\n", uDeviceID);
    if (!MCI_DevIDValid(uDeviceID) || MCI_GetDrv(uDeviceID)->modp.wType == 0) {
	WARN(mmsys, "Bad uDeviceID\n");
	return 0L;
    }
    
    return MCI_GetDrv(uDeviceID)->dwPrivate;
}

/**************************************************************************
 * 			mciSetDriverData			[MMSYSTEM.707]
 */
BOOL16 WINAPI mciSetDriverData16(UINT16 uDeviceID, DWORD data) 
{
    return mciSetDriverData(uDeviceID, data);
}

/**************************************************************************
 * 			mciSetDriverData			[WINMM.53]
 */
BOOL WINAPI mciSetDriverData(UINT uDeviceID, DWORD data) 
{
    TRACE(mmsys, "(%04x,%08lx)\n", uDeviceID, data);
    if (!MCI_DevIDValid(uDeviceID) || MCI_GetDrv(uDeviceID)->modp.wType == 0) {
	WARN(mmsys, "Bad uDeviceID\n");
	return FALSE;
    }
    
    MCI_GetDrv(uDeviceID)->dwPrivate = data;
    return TRUE;
}

/**************************************************************************
 *                    	mciLoadCommandResource			[MMSYSTEM.705]
 */
UINT16 WINAPI mciLoadCommandResource16(HANDLE16 hinst, LPCSTR resname, UINT16 type)
{
    char            buf[200];
    OFSTRUCT        ofs;
    HANDLE16        xhinst;
    HRSRC16         hrsrc;
    HGLOBAL16       hmem;
    LPSTR           segstr;
    SEGPTR          xmem;
    LPBYTE          lmem;
    static UINT16   mcidevtype = 0;
    
    FIXME(mmsys,"(%04x,%s,%d): stub!\n", hinst, resname, type);
    if (!lstrcmpiA(resname, "core")) {
	FIXME(mmsys, "(...,\"core\",...), have to use internal tables... (not there yet)\n");
	return 0;
    }
    return ++mcidevtype;
    /* if file exists "resname.mci", then load resource "resname" from it
     * otherwise directly from driver
     */
    strcpy(buf,resname);
    strcat(buf,".mci");
    if (OpenFile(buf, &ofs,OF_EXIST) != HFILE_ERROR) {
	xhinst = LoadLibrary16(buf);
	if (xhinst > 32)
	    hinst = xhinst;
    } /* else use passed hinst */
    segstr = SEGPTR_STRDUP(resname);
    hrsrc = FindResource16(hinst, SEGPTR_GET(segstr), type);
    SEGPTR_FREE(segstr);
    if (!hrsrc) {
	WARN(mmsys, "no special commandlist found in resource\n");
	return MCI_NO_COMMAND_TABLE;
    }
    hmem = LoadResource16(hinst, hrsrc);
    if (!hmem) {
	WARN(mmsys, "couldn't load resource??\n");
	return MCI_NO_COMMAND_TABLE;
    }
    xmem = WIN16_LockResource16(hmem);
    if (!xmem) {
	WARN(mmsys, "couldn't lock resource??\n");
	FreeResource16(hmem);
	return MCI_NO_COMMAND_TABLE;
    }
    lmem = PTR_SEG_TO_LIN(xmem);
    TRACE(mmsys, "first resource entry is %s\n", (char*)lmem);
    /* parse resource, register stuff, return unique id */
    return ++mcidevtype;
}

/**************************************************************************
 *                    	mciFreeCommandResource			[MMSYSTEM.713]
 */
BOOL16 WINAPI mciFreeCommandResource16(UINT16 uTable)
{
    FIXME(mci, "(%04x) stub\n", uTable);
    return 0;
}
 
/**************************************************************************
 *                    	mciFreeCommandResource			[WINMM.39]
 */
BOOL WINAPI mciFreeCommandResource(UINT uTable)
{
    FIXME(mci, "(%08x) stub\n", uTable);
    return 0;
}

/**************************************************************************
 *                    	mciLoadCommandResource  		[WINMM.48]
 */
UINT WINAPI mciLoadCommandResource(HANDLE hinst, LPCWSTR resname, UINT type)
{
    FIXME(mci, "(%04x,%s,%d): stub!\n", hinst, debugstr_w(resname), type);
    return 0;
}

/**************************************************************************
 * 				mciSendCommandA			[WINMM.49]
 */
DWORD WINAPI mciSendCommandA(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD	dwRet;

    TRACE(mci, "(%08x,%s,%08lx,%08lx)\n", wDevID, MCI_CommandToString(wMsg), dwParam1, dwParam2);

    switch (wMsg) {
    case MCI_OPEN:
	dwRet = MCI_Open(dwParam1, (LPMCI_OPEN_PARMSA)dwParam2);
	break;
    case MCI_CLOSE:
	dwRet = MCI_Close(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
	break;
    case MCI_SYSINFO:
	dwRet = MCI_SysInfo(wDevID, dwParam1, (LPMCI_SYSINFO_PARMSA)dwParam2);
	break;
    default:
	if (wDevID == MCI_ALL_DEVICE_ID) {
	    FIXME(mci, "unhandled MCI_ALL_DEVICE_ID\n");
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else {
	    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2);
	}
	break;
    }
    dwRet = MCI_CleanUp(dwRet, wMsg, dwParam2, TRUE);
    TRACE(mci, "=> %08ld\n", dwRet);
    return dwRet;
}

/**************************************************************************
 * 				mciSendCommandW			[WINMM.50]
 */
DWORD WINAPI mciSendCommandW(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2)
{
    return 0x1; /* !ok */
}

/**************************************************************************
 * 				mciSendCommand			[MMSYSTEM.701]
 */
DWORD WINAPI mciSendCommand16(UINT16 wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD	dwRet = MCIERR_UNRECOGNIZED_COMMAND;

    TRACE(mmsys, "(%04X, %s, %08lX, %08lX)\n", 
	  wDevID, MCI_CommandToString(wMsg), dwParam1, dwParam2);

    switch (wMsg) {
    case MCI_OPEN:
	if (MCI_MapMsg16To32A(MCI_GetDrv(wDevID)->modp.wType, wMsg, &dwParam2) >= 0) {
	    dwRet = MCI_Open(dwParam1, (LPMCI_OPEN_PARMSA)dwParam2);
	    MCI_UnMapMsg16To32A(MCI_GetDrv(wDevID)->modp.wType, wMsg, dwParam2);
	}
	break;
    case MCI_CLOSE:
	if (wDevID == MCI_ALL_DEVICE_ID) {
	    FIXME(mci, "unhandled MCI_ALL_DEVICE_ID\n");
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else if (!MCI_DevIDValid(wDevID)) {
	    dwRet = MCIERR_INVALID_DEVICE_ID;
	} else if (MCI_MapMsg16To32A(MCI_GetDrv(wDevID)->modp.wType, wMsg, &dwParam2) >= 0) {
	    dwRet = MCI_Close(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
	    MCI_UnMapMsg16To32A(MCI_GetDrv(wDevID)->modp.wType, wMsg, dwParam2);
	}
	break;
    case MCI_SYSINFO:
	if (MCI_MapMsg16To32A(0, wDevID, &dwParam2) >= 0) {
	    dwRet = MCI_SysInfo(wDevID, dwParam1, (LPMCI_SYSINFO_PARMSA)dwParam2);
	    MCI_UnMapMsg16To32A(0, wDevID, dwParam2);
	}
	break;
    /* FIXME: it seems that MCI_BREAK and MCI_SOUND need the same handling */
    default:
	if (wDevID == MCI_ALL_DEVICE_ID) {
	    FIXME(mci, "unhandled MCI_ALL_DEVICE_ID\n");
	    dwRet = MCIERR_CANNOT_USE_ALL;
	} else if (!MCI_DevIDValid(wDevID)) {
	    dwRet = MCIERR_INVALID_DEVICE_ID;
	} else {
	    int			res;
		    
	    switch (DRIVER_GetType(MCI_GetDrv(wDevID)->hDrv)) {
	    case WINE_DI_TYPE_16:		
		dwRet = SendDriverMessage16(MCI_GetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
		break;
	    case WINE_DI_TYPE_32:
		switch (res = MCI_MapMsg16To32A(MCI_GetDrv(wDevID)->modp.wType, wMsg, &dwParam2)) {
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
		    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2);
		    if (res)
			MCI_UnMapMsg16To32A(MCI_GetDrv(wDevID)->modp.wType, wMsg, dwParam2);
		    break;
		}
		break;
	    default:
		WARN(mmsys, "Unknown driver type=%u\n", DRIVER_GetType(MCI_GetDrv(wDevID)->hDrv));
		dwRet = MCIERR_DRIVER_INTERNAL;
	    }
	}
    }
    dwRet = MCI_CleanUp(dwRet, wMsg, dwParam2, FALSE);
    TRACE(mmsys, "=> %ld\n", dwRet);
    return dwRet;
}
    
/**************************************************************************
 * 				mciGetDeviceID		       	[MMSYSTEM.703]
 */
UINT16 WINAPI mciGetDeviceID16(LPCSTR lpstrName)
{
    UINT16	wDevID;
    TRACE(mmsys, "(\"%s\")\n", lpstrName);

    if (!lpstrName)
	return 0;
    
    if (!lstrcmpiA(lpstrName, "ALL"))
	return MCI_ALL_DEVICE_ID;
    
    for (wDevID = MCI_FirstDevID(); MCI_DevIDValid(wDevID); wDevID = MCI_NextDevID(wDevID)) {
	if (MCI_GetDrv(wDevID)->modp.wType) {
	    FIXME(mmsys, "This is wrong for compound devices\n");
	    /* FIXME: for compound devices, lpstrName is matched against 
	     * the name of the file, not the name of the device... 
	     */
	    if (MCI_GetOpenDrv(wDevID)->lpstrDeviceType && 
		strcmp(MCI_GetOpenDrv(wDevID)->lpstrDeviceType, lpstrName) == 0)
		return wDevID;
    
	    if (MCI_GetOpenDrv(wDevID)->lpstrAlias && 
		strcmp(MCI_GetOpenDrv(wDevID)->lpstrAlias, lpstrName) == 0)
		return wDevID;
	}
    }
    
    return 0;
}

/**************************************************************************
 * 				mciGetDeviceIDA    		[WINMM.41]
 */
UINT WINAPI mciGetDeviceIDA(LPCSTR lpstrName)
{
    return mciGetDeviceID16(lpstrName);
}

/**************************************************************************
 * 				mciGetDeviceIDW		       	[WINMM.43]
 */
UINT WINAPI mciGetDeviceIDW(LPCWSTR lpwstrName)
{
    LPSTR 	lpstrName;
    UINT	ret;

    lpstrName = HEAP_strdupWtoA(GetProcessHeap(), 0, lpwstrName);
    ret = mciGetDeviceID16(lpstrName);
    HeapFree(GetProcessHeap(), 0, lpstrName);
    return ret;
}

/**************************************************************************
 * 				mciSetYieldProc			[MMSYSTEM.714]
 */
BOOL16 WINAPI mciSetYieldProc16(UINT16 uDeviceID, YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    TRACE(mmsys, "(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);
    if (!MCI_DevIDValid(uDeviceID) || MCI_GetDrv(uDeviceID)->modp.wType == 0) {
	WARN(mmsys, "Bad uDeviceID\n");
	return FALSE;
    }
    
    MCI_GetDrv(uDeviceID)->lpfnYieldProc = fpYieldProc;
    MCI_GetDrv(uDeviceID)->dwYieldData   = dwYieldData;
    MCI_GetDrv(uDeviceID)->bIs32         = FALSE;

    return TRUE;
}

/**************************************************************************
 * 				mciSetYieldProc			[WINMM.54]
 */
BOOL WINAPI mciSetYieldProc(UINT uDeviceID, YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    TRACE(mmsys, "(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);
    if (!MCI_DevIDValid(uDeviceID) || MCI_GetDrv(uDeviceID)->modp.wType == 0) {
	WARN(mmsys, "Bad uDeviceID\n");
	return FALSE;
    }
    
    MCI_GetDrv(uDeviceID)->lpfnYieldProc = fpYieldProc;
    MCI_GetDrv(uDeviceID)->dwYieldData   = dwYieldData;
    MCI_GetDrv(uDeviceID)->bIs32         = TRUE;

    return TRUE;
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
 */
UINT16 WINAPI mciGetDeviceIDFromElementID16(DWORD dwElementID, LPCSTR lpstrType)
{
    FIXME(mci, "(%lu, %s) stub\n", dwElementID, lpstrType);
    return 0;
}
	
/**************************************************************************
 * 				mciGetDeviceIDFromElementIDW	[WINMM.42]
 */
UINT WINAPI mciGetDeviceIDFromElementIDW(DWORD dwElementID, LPCWSTR lpstrType)
{
    /* FIXME: that's rather strange, there is no 
     * mciGetDeviceIDFromElementID32A in winmm.spec
     */
    FIXME(mci, "(%lu, %p) stub\n", dwElementID, lpstrType);
    return 0;
}
	
/**************************************************************************
 * 				mciGetYieldProc			[MMSYSTEM.716]
 */
YIELDPROC WINAPI mciGetYieldProc16(UINT16 uDeviceID, DWORD* lpdwYieldData)
{
    TRACE(mmsys, "(%u, %p)\n", uDeviceID, lpdwYieldData);
    if (!MCI_DevIDValid(uDeviceID) || MCI_GetDrv(uDeviceID)->modp.wType == 0) {
	WARN(mmsys, "Bad uDeviceID\n");
	return NULL;
    }
    if (!MCI_GetDrv(uDeviceID)->lpfnYieldProc) {
	WARN(mmsys, "No proc set\n");
	return NULL;
    }
    if (MCI_GetDrv(uDeviceID)->bIs32) {
	WARN(mmsys, "Proc is 32 bit\n");
	return NULL;
    }
    return MCI_GetDrv(uDeviceID)->lpfnYieldProc;
}
    
/**************************************************************************
 * 				mciGetYieldProc			[WINMM.47]
 */
YIELDPROC WINAPI mciGetYieldProc(UINT uDeviceID, DWORD* lpdwYieldData)
{
    TRACE(mmsys, "(%u, %p)\n", uDeviceID, lpdwYieldData);
    if (!MCI_DevIDValid(uDeviceID) || MCI_GetDrv(uDeviceID)->modp.wType == 0) {
	WARN(mmsys, "Bad uDeviceID\n");
	return NULL;
    }
    if (!MCI_GetDrv(uDeviceID)->lpfnYieldProc) {
	WARN(mmsys, "No proc set\n");
	return NULL;
    }
    if (!MCI_GetDrv(uDeviceID)->bIs32) {
	WARN(mmsys, "Proc is 32 bit\n");
	return NULL;
    }
    return MCI_GetDrv(uDeviceID)->lpfnYieldProc;
}

/**************************************************************************
 * 				mciGetCreatorTask		[MMSYSTEM.717]
 */
HTASK16 WINAPI mciGetCreatorTask16(UINT16 uDeviceID)
{
    FIXME(mci, "(%u) stub\n", uDeviceID);
    return 0;
}

/**************************************************************************
 * 				mciGetCreatorTask		[WINMM.40]
 */
HTASK WINAPI mciGetCreatorTask(UINT uDeviceID)
{
    FIXME(mci, "(%u) stub\n", uDeviceID);
    return 0;
}

/**************************************************************************
 * 				mciDriverYield			[MMSYSTEM.710]
 */
UINT16 WINAPI mciDriverYield16(UINT16 uDeviceID) 
{
    UINT16	ret = 0;

    TRACE(mmsys,"(%04x)\n", uDeviceID);
    if (!MCI_DevIDValid(uDeviceID) || MCI_GetDrv(uDeviceID)->modp.wType == 0 ||
	!MCI_GetDrv(uDeviceID)->lpfnYieldProc || MCI_GetDrv(uDeviceID)->bIs32) {
	OldYield16();
    } else {
	ret = MCI_GetDrv(uDeviceID)->lpfnYieldProc(uDeviceID, MCI_GetDrv(uDeviceID)->dwYieldData);
    }

    return ret;
}

/**************************************************************************
 * 			mciDriverYield				[WINMM.37]
 */
UINT WINAPI mciDriverYield(UINT uDeviceID) 
{
    UINT	ret = 0;

    TRACE(mmsys,"(%04x)\n", uDeviceID);
    if (!MCI_DevIDValid(uDeviceID) || MCI_GetDrv(uDeviceID)->modp.wType == 0 ||
	!MCI_GetDrv(uDeviceID)->lpfnYieldProc || !MCI_GetDrv(uDeviceID)->bIs32) {
	OldYield16();
    } else {
	ret = MCI_GetDrv(uDeviceID)->lpfnYieldProc(uDeviceID, MCI_GetDrv(uDeviceID)->dwYieldData);
    }

    return ret;
}

/**************************************************************************
 * 				midiOutGetNumDevs	[WINMM.80]
 */
UINT WINAPI midiOutGetNumDevs(void)
{
    return midiOutGetNumDevs16();
}

/**************************************************************************
 * 				midiOutGetNumDevs	[MMSYSTEM.201]
 */
UINT16 WINAPI midiOutGetNumDevs16(void)
{
    UINT16	count = modMessage(0, MODM_GETNUMDEVS, 0L, 0L, 0L);

    TRACE(mmsys, "returns %u\n", count);
    return count;
}

/**************************************************************************
 * 				midiOutGetDevCapsW	[WINMM.76]
 */
UINT WINAPI midiOutGetDevCapsW(UINT uDeviceID, LPMIDIOUTCAPSW lpCaps, UINT uSize)
{
    MIDIOUTCAPS16	moc16;
    UINT		ret;
    
    ret = midiOutGetDevCaps16(uDeviceID, &moc16, sizeof(moc16));
    lpCaps->wMid		= moc16.wMid;
    lpCaps->wPid		= moc16.wPid;
    lpCaps->vDriverVersion	= moc16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname, moc16.szPname);
    lpCaps->wTechnology	= moc16.wTechnology;
    lpCaps->wVoices		= moc16.wVoices;
    lpCaps->wNotes		= moc16.wNotes;
    lpCaps->wChannelMask	= moc16.wChannelMask;
    lpCaps->dwSupport	= moc16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				midiOutGetDevCapsA	[WINMM.75]
 */
UINT WINAPI midiOutGetDevCapsA(UINT uDeviceID, LPMIDIOUTCAPSA lpCaps, UINT uSize)
{
    MIDIOUTCAPS16	moc16;
    UINT		ret;
    
    ret = midiOutGetDevCaps16(uDeviceID, &moc16, sizeof(moc16));
    lpCaps->wMid		= moc16.wMid;
    lpCaps->wPid		= moc16.wPid;
    lpCaps->vDriverVersion	= moc16.vDriverVersion;
    strcpy(lpCaps->szPname, moc16.szPname);
    lpCaps->wTechnology	= moc16.wTechnology;
    lpCaps->wVoices		= moc16.wVoices;
    lpCaps->wNotes		= moc16.wNotes;
    lpCaps->wChannelMask	= moc16.wChannelMask;
    lpCaps->dwSupport	= moc16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				midiOutGetDevCaps	[MMSYSTEM.202]
 */
UINT16 WINAPI midiOutGetDevCaps16(UINT16 uDeviceID, LPMIDIOUTCAPS16 lpCaps, UINT16 uSize)
{
    TRACE(mmsys, "midiOutGetDevCaps\n");
    return modMessage(uDeviceID, MODM_GETDEVCAPS, 0, (DWORD)lpCaps,uSize);
}

/**************************************************************************
 * 				midiOutGetErrorTextA 	[WINMM.77]
 */
UINT WINAPI midiOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    TRACE(mmsys, "midiOutGetErrorText\n");
    return midiGetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiOutGetErrorTextW 	[WINMM.78]
 */
UINT WINAPI midiOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0,uSize);
    UINT	ret;
    
    TRACE(mmsys, "midiOutGetErrorText\n");
    ret = midiGetErrorText(uError, xstr, uSize);
    lstrcpyAtoW(lpText,xstr);
    HeapFree(GetProcessHeap(), 0,xstr);
    return ret;
}

/**************************************************************************
 * 				midiOutGetErrorText 	[MMSYSTEM.203]
 */
UINT16 WINAPI midiOutGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    TRACE(mmsys, "midiOutGetErrorText\n");
    return midiGetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiGetErrorText       	[internal]
 */
UINT16 WINAPI midiGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    LPSTR	msgptr;
    if ((lpText == NULL) || (uSize < 1)) return(FALSE);
    lpText[0] = '\0';
    switch (uError) {
    case MIDIERR_UNPREPARED:
	msgptr = "The MIDI header was not prepared. Use the Prepare function to prepare the header, and then try again.";
	break;
    case MIDIERR_STILLPLAYING:
	msgptr = "Cannot perform this operation while media data is still playing. Reset the device, or wait until the data is finished playing.";
	break;
    case MIDIERR_NOMAP:
	msgptr = "A MIDI map was not found. There may be a problem with the driver, or the MIDIMAP.CFG file may be corrupt or missing.";
	break;
    case MIDIERR_NOTREADY:
	msgptr = "The port is transmitting data to the device. Wait until the data has been transmitted, and then try again.";
	break;
    case MIDIERR_NODEVICE:
	msgptr = "The current MIDI Mapper setup refers to a MIDI device that is not installed on the system. Use MIDI Mapper to edit the setup.";
	break;
    case MIDIERR_INVALIDSETUP:
	msgptr = "The current MIDI setup is damaged. Copy the original MIDIMAP.CFG file to the Windows SYSTEM directory, and then try again.";
	break;
	/*
	  msg# 336 : Cannot use the song-pointer time format and the SMPTE time-format together.
	  msg# 337 : The specified MIDI device is already in use. Wait until it is free, and then try again.
	  msg# 338 : The specified MIDI device is not installed on the system. Use the Drivers option in Control Panel to install the driver.
	  msg# 339 : The current MIDI Mapper setup refers to a MIDI device that is not installed on the system. Use MIDI Mapper to edit the setup.
	  msg# 340 : An error occurred using the specified port.
	  msg# 341 : All multimedia timers are being used by other applications. Quit one of these applications, and then try again.
	  msg# 342 : There is no current MIDI port.
	  msg# 343 : There are no MIDI devices installed on the system. Use the Drivers option in Control Panel to install the driver.
	*/
    default:
	msgptr = "Unknown MIDI Error !\n";
	break;
    }
    lstrcpynA(lpText, msgptr, uSize);
    return TRUE;
}

/**************************************************************************
 * 				midiOutOpen    		[WINM.84]
 */
UINT WINAPI midiOutOpen(HMIDIOUT* lphMidiOut, UINT uDeviceID,
			DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDIOUT16	hmo16;
    UINT	ret;
    
    ret = midiOutOpen16(&hmo16,uDeviceID, dwCallback, dwInstance,
			CALLBACK32CONV(dwFlags));
    if (lphMidiOut) *lphMidiOut = hmo16;
    return ret;
}

/**************************************************************************
 * 				midiOutOpen    		[MMSYSTEM.204]
 */
UINT16 WINAPI midiOutOpen16(HMIDIOUT16* lphMidiOut, UINT16 uDeviceID,
                            DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDI16			hMidiOut;
    LPMIDIOPENDESC		lpDesc;
    DWORD			dwRet = 0;
    BOOL			bMapperFlg = FALSE;
    
    if (lphMidiOut != NULL) *lphMidiOut = 0;
    TRACE(mmsys, "(%p, %d, %08lX, %08lX, %08lX);\n", 
	  lphMidiOut, uDeviceID, dwCallback, dwInstance, dwFlags);
    if (uDeviceID == (UINT16)MIDI_MAPPER) {
	TRACE(mmsys, "MIDI_MAPPER mode requested !\n");
	bMapperFlg = TRUE;
	uDeviceID = 0;
    }
    hMidiOut = USER_HEAP_ALLOC(sizeof(MIDIOPENDESC));
    if (lphMidiOut != NULL) 
	*lphMidiOut = hMidiOut;
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL)
	return MMSYSERR_NOMEM;
    lpDesc->hMidi = hMidiOut;
    lpDesc->dwCallback = dwCallback;
    lpDesc->dwInstance = dwInstance;
    
    while (uDeviceID < MAXMIDIDRIVERS) {
	dwRet = modMessage(uDeviceID, MODM_OPEN, 
			   lpDesc->dwInstance, (DWORD)lpDesc, dwFlags);
	if (dwRet == MMSYSERR_NOERROR) break;
	if (!bMapperFlg) break;
	uDeviceID++;
	TRACE(mmsys, "MIDI_MAPPER mode ! try next driver...\n");
    }
    lpDesc->wDevID = uDeviceID;
    return dwRet;
}

/**************************************************************************
 * 				midiOutClose		[WINMM.74]
 */
UINT WINAPI midiOutClose(HMIDIOUT hMidiOut)
{
    return midiOutClose16(hMidiOut);
}

/**************************************************************************
 * 				midiOutClose		[MMSYSTEM.205]
 */
UINT16 WINAPI midiOutClose16(HMIDIOUT16 hMidiOut)
{
    LPMIDIOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X)\n", hMidiOut);

    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return modMessage(lpDesc->wDevID, MODM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				midiOutPrepareHeader	[WINMM.85]
 */
UINT WINAPI midiOutPrepareHeader(HMIDIOUT hMidiOut,
				 MIDIHDR16* lpMidiOutHdr, UINT uSize)
{
    LPMIDIOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpMidiOutHdr->reserved = (DWORD)lpMidiOutHdr->lpData;
    return modMessage(lpDesc->wDevID, MODM_PREPARE, lpDesc->dwInstance, 
		      (DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiOutPrepareHeader	[MMSYSTEM.206]
 */
UINT16 WINAPI midiOutPrepareHeader16(HMIDIOUT16 hMidiOut,
                                     MIDIHDR16* lpMidiOutHdr, UINT16 uSize)
{
    LPMIDIOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpMidiOutHdr->reserved = (DWORD)PTR_SEG_TO_LIN(lpMidiOutHdr->lpData);
    return modMessage(lpDesc->wDevID, MODM_PREPARE, lpDesc->dwInstance, 
		      (DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[WINMM.89]
 */
UINT WINAPI midiOutUnprepareHeader(HMIDIOUT hMidiOut,
				   MIDIHDR16* lpMidiOutHdr, UINT uSize)
{
    return midiOutUnprepareHeader16(hMidiOut, lpMidiOutHdr,uSize);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[MMSYSTEM.207]
 */
UINT16 WINAPI midiOutUnprepareHeader16(HMIDIOUT16 hMidiOut,
				       MIDIHDR16* lpMidiOutHdr, UINT16 uSize)
{
    LPMIDIOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return modMessage(lpDesc->wDevID, MODM_UNPREPARE, lpDesc->dwInstance, 
		      (DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiOutShortMsg		[WINMM.88]
 */
UINT WINAPI midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
{
    return midiOutShortMsg16(hMidiOut, dwMsg);
}

/**************************************************************************
 * 				midiOutShortMsg		[MMSYSTEM.208]
 */
UINT16 WINAPI midiOutShortMsg16(HMIDIOUT16 hMidiOut, DWORD dwMsg)
{
    LPMIDIOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %08lX)\n", hMidiOut, dwMsg);

    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return modMessage(lpDesc->wDevID, MODM_DATA, lpDesc->dwInstance, dwMsg, 0L);
}

/**************************************************************************
 * 				midiOutLongMsg		[WINMM.82]
 */
UINT WINAPI midiOutLongMsg(HMIDIOUT hMidiOut,
			   MIDIHDR16* lpMidiOutHdr, UINT uSize)
{
    return midiOutLongMsg16(hMidiOut, lpMidiOutHdr,uSize);
}

/**************************************************************************
 * 				midiOutLongMsg		[MMSYSTEM.209]
 */
UINT16 WINAPI midiOutLongMsg16(HMIDIOUT16 hMidiOut,
                               MIDIHDR16* lpMidiOutHdr, UINT16 uSize)
{
    LPMIDIOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return modMessage(lpDesc->wDevID, MODM_LONGDATA, lpDesc->dwInstance, 
		      (DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiOutReset		[WINMM.86]
 */
UINT WINAPI midiOutReset(HMIDIOUT hMidiOut)
{
    return midiOutReset16(hMidiOut);
}

/**************************************************************************
 * 				midiOutReset		[MMSYSTEM.210]
 */
UINT16 WINAPI midiOutReset16(HMIDIOUT16 hMidiOut)
{
    LPMIDIOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X)\n", hMidiOut);

    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return modMessage(lpDesc->wDevID, MODM_RESET, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				midiOutGetVolume	[WINM.81]
 */
UINT WINAPI midiOutGetVolume(UINT uDeviceID, DWORD* lpdwVolume)
{
    return midiOutGetVolume16(uDeviceID, lpdwVolume);
}

/**************************************************************************
 * 				midiOutGetVolume	[MMSYSTEM.211]
 */
UINT16 WINAPI midiOutGetVolume16(UINT16 uDeviceID, DWORD* lpdwVolume)
{
    TRACE(mmsys, "(%04X, %p);\n", uDeviceID, lpdwVolume);
    return modMessage(uDeviceID, MODM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
}

/**************************************************************************
 * 				midiOutSetVolume	[WINMM.87]
 */
UINT WINAPI midiOutSetVolume(UINT uDeviceID, DWORD dwVolume)
{
    return midiOutSetVolume16(uDeviceID, dwVolume);
}

/**************************************************************************
 * 				midiOutSetVolume	[MMSYSTEM.212]
 */
UINT16 WINAPI midiOutSetVolume16(UINT16 uDeviceID, DWORD dwVolume)
{
    TRACE(mmsys, "(%04X, %08lX);\n", uDeviceID, dwVolume);
    return modMessage(uDeviceID, MODM_SETVOLUME, 0L, dwVolume, 0L);
}

/**************************************************************************
 * 				midiOutCachePatches		[WINMM.73]
 */
UINT WINAPI midiOutCachePatches(HMIDIOUT hMidiOut, UINT uBank,
				WORD* lpwPatchArray, UINT uFlags)
{
    return midiOutCachePatches16(hMidiOut,uBank, lpwPatchArray,uFlags);
}

/**************************************************************************
 * 				midiOutCachePatches		[MMSYSTEM.213]
 */
UINT16 WINAPI midiOutCachePatches16(HMIDIOUT16 hMidiOut, UINT16 uBank,
                                    WORD* lpwPatchArray, UINT16 uFlags)
{
    /* not really necessary to support this */
    FIXME(mmsys, "not supported yet\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[WINMM.72]
 */
UINT WINAPI midiOutCacheDrumPatches(HMIDIOUT hMidiOut, UINT uPatch,
				    WORD* lpwKeyArray, UINT uFlags)
{
    return midiOutCacheDrumPatches16(hMidiOut,uPatch, lpwKeyArray,uFlags);
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[MMSYSTEM.214]
 */
UINT16 WINAPI midiOutCacheDrumPatches16(HMIDIOUT16 hMidiOut, UINT16 uPatch,
                                        WORD* lpwKeyArray, UINT16 uFlags)
{
    FIXME(mmsys, "not supported yet\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutGetID		[WINMM.79]
 */
UINT WINAPI midiOutGetID(HMIDIOUT hMidiOut, UINT* lpuDeviceID)
{
    UINT16	xid;
    UINT	ret;
    
    ret = midiOutGetID16(hMidiOut, &xid);
    *lpuDeviceID = xid;
    return ret;
}

/**************************************************************************
 * 				midiOutGetID		[MMSYSTEM.215]
 */
UINT16 WINAPI midiOutGetID16(HMIDIOUT16 hMidiOut, UINT16* lpuDeviceID)
{
    TRACE(mmsys, "midiOutGetID\n");
    return 0;
}

/**************************************************************************
 * 				midiOutMessage		[WINMM.83]
 */
DWORD WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage, 
			    DWORD dwParam1, DWORD dwParam2)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %04X, %08lX, %08lX)\n", 
	  hMidiOut, uMessage, dwParam1, dwParam2);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case MODM_OPEN:
	FIXME(mmsys,"can't handle MODM_OPEN!\n");
	return 0;
    case MODM_GETDEVCAPS:
	return midiOutGetDevCapsA(hMidiOut, (LPMIDIOUTCAPSA)dwParam1, dwParam2);
    case MODM_GETNUMDEVS:
    case MODM_RESET:
    case MODM_CLOSE:
    case MODM_GETVOLUME:
    case MODM_SETVOLUME:
    case MODM_LONGDATA:
    case MODM_PREPARE:
    case MODM_UNPREPARE:
	/* no argument conversion needed */
	break;
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hMidiOut,uMessage, dwParam1, dwParam2);
	break;
    }
    return modMessage(lpDesc->wDevID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				midiOutMessage		[MMSYSTEM.216]
 */
DWORD WINAPI midiOutMessage16(HMIDIOUT16 hMidiOut, UINT16 uMessage, 
                              DWORD dwParam1, DWORD dwParam2)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %04X, %08lX, %08lX)\n", 
	  hMidiOut, uMessage, dwParam1, dwParam2);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case MODM_OPEN:
	FIXME(mmsys,"can't handle MODM_OPEN!\n");
	return 0;
    case MODM_GETNUMDEVS:
    case MODM_RESET:
    case MODM_CLOSE:
    case MODM_SETVOLUME:
	/* no argument conversion needed */
	break;
    case MODM_GETVOLUME:
	return midiOutGetVolume16(hMidiOut, (LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case MODM_LONGDATA:
	return midiOutLongMsg16(hMidiOut, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MODM_PREPARE:
	return midiOutPrepareHeader16(hMidiOut, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MODM_UNPREPARE:
	return midiOutUnprepareHeader16(hMidiOut, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hMidiOut,uMessage, dwParam1, dwParam2);
	break;
    }
    return modMessage(lpDesc->wDevID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				midiInGetNumDevs	[WINMM.64]
 */
UINT WINAPI midiInGetNumDevs(void)
{
    return midiInGetNumDevs16();
}

/**************************************************************************
 * 				midiInGetNumDevs	[MMSYSTEM.301]
 */
UINT16 WINAPI midiInGetNumDevs16(void)
{
    UINT16	count = 0;
    TRACE(mmsys, "midiInGetNumDevs\n");
    count += midMessage(0, MIDM_GETNUMDEVS, 0L, 0L, 0L);
    TRACE(mmsys, "midiInGetNumDevs return %u \n", count);
    return count;
}

/**************************************************************************
 * 				midiInGetDevCaps	[WINMM.60]
 */
UINT WINAPI midiInGetDevCapsW(UINT uDeviceID, LPMIDIINCAPSW lpCaps, UINT uSize)
{
    MIDIINCAPS16	mic16;
    UINT		ret = midiInGetDevCaps16(uDeviceID, &mic16,uSize);
    
    lpCaps->wMid = mic16.wMid;
    lpCaps->wPid = mic16.wPid;
    lpCaps->vDriverVersion = mic16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname, mic16.szPname);
    lpCaps->dwSupport = mic16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				midiInGetDevCaps	[WINMM.59]
 */
UINT WINAPI midiInGetDevCapsA(UINT uDeviceID, LPMIDIINCAPSA lpCaps, UINT uSize)
{
    MIDIINCAPS16	mic16;
    UINT		ret = midiInGetDevCaps16(uDeviceID, &mic16,uSize);
    
    lpCaps->wMid = mic16.wMid;
    lpCaps->wPid = mic16.wPid;
    lpCaps->vDriverVersion = mic16.vDriverVersion;
    strcpy(lpCaps->szPname, mic16.szPname);
    lpCaps->dwSupport = mic16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				midiInGetDevCaps	[MMSYSTEM.302]
 */
UINT16 WINAPI midiInGetDevCaps16(UINT16 uDeviceID,
				 LPMIDIINCAPS16 lpCaps, UINT16 uSize)
{
    TRACE(mmsys, "midiInGetDevCaps\n");
    return midMessage(uDeviceID, MIDM_GETDEVCAPS, 0, (DWORD)lpCaps, uSize);
}

/**************************************************************************
 * 				midiInGetErrorText 		[WINMM.62]
 */
UINT WINAPI midiInGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0,uSize);
    UINT	ret = midiInGetErrorText16(uError,xstr,uSize);
    lstrcpyAtoW(lpText,xstr);
    HeapFree(GetProcessHeap(), 0,xstr);
    return ret;
}

/**************************************************************************
 * 				midiInGetErrorText 		[WINMM.61]
 */
UINT WINAPI midiInGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return midiInGetErrorText16(uError, lpText,uSize);
}

/**************************************************************************
 * 				midiInGetErrorText 		[MMSYSTEM.303]
 */
UINT16 WINAPI midiInGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    TRACE(mmsys, "midiInGetErrorText\n");
    return (midiGetErrorText(uError, lpText, uSize));
}

/**************************************************************************
 * 				midiInOpen		[WINMM.66]
 */
UINT WINAPI midiInOpen(HMIDIIN* lphMidiIn, UINT uDeviceID,
		       DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDIIN16	xhmid16;
    UINT 		ret = midiInOpen16(&xhmid16,uDeviceID, dwCallback, dwInstance,
					   CALLBACK32CONV(dwFlags));
    if (lphMidiIn) 
	*lphMidiIn = xhmid16;
    return ret;
}

/**************************************************************************
 * 				midiInOpen		[MMSYSTEM.304]
 */
UINT16 WINAPI midiInOpen16(HMIDIIN16* lphMidiIn, UINT16 uDeviceID,
                           DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDI16			hMidiIn;
    LPMIDIOPENDESC		lpDesc;
    DWORD			dwRet = 0;
    BOOL			bMapperFlg = FALSE;
    
    if (lphMidiIn != NULL) 
	*lphMidiIn = 0;
    TRACE(mmsys, "(%p, %d, %08lX, %08lX, %08lX);\n", 
	  lphMidiIn, uDeviceID, dwCallback, dwInstance, dwFlags);
    if (uDeviceID == (UINT16)MIDI_MAPPER) {
	TRACE(mmsys, "MIDI_MAPPER mode requested !\n");
	bMapperFlg = TRUE;
	uDeviceID = 0;
    }
    hMidiIn = USER_HEAP_ALLOC(sizeof(MIDIOPENDESC));
    if (lphMidiIn != NULL) 
	*lphMidiIn = hMidiIn;
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) 
	return MMSYSERR_NOMEM;
    lpDesc->hMidi = hMidiIn;
    lpDesc->dwCallback = dwCallback;
    lpDesc->dwInstance = dwInstance;
    
    while (uDeviceID < MAXMIDIDRIVERS) {
	dwRet = midMessage(uDeviceID, MIDM_OPEN, 
			   lpDesc->dwInstance, (DWORD)lpDesc, dwFlags);
	if (dwRet == MMSYSERR_NOERROR) 
	    break;
	if (!bMapperFlg) 
	    break;
	uDeviceID++;
	TRACE(mmsys, "MIDI_MAPPER mode ! try next driver...\n");
    }
    lpDesc->wDevID = uDeviceID;
    return dwRet;
}

/**************************************************************************
 * 				midiInClose		[WINMM.58]
 */
UINT WINAPI midiInClose(HMIDIIN hMidiIn)
{
    return midiInClose16(hMidiIn);
}

/**************************************************************************
 * 				midiInClose		[MMSYSTEM.305]
 */
UINT16 WINAPI midiInClose16(HMIDIIN16 hMidiIn)
{
    LPMIDIOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X)\n", hMidiIn);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return midMessage(lpDesc->wDevID, MIDM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				midiInPrepareHeader	[WINMM.67]
 */
UINT WINAPI midiInPrepareHeader(HMIDIIN hMidiIn,
				MIDIHDR16* lpMidiInHdr, UINT uSize)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %d)\n", 
	  hMidiIn, lpMidiInHdr, uSize);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpMidiInHdr->reserved = (DWORD)lpMidiInHdr->lpData;
    return midMessage(lpDesc->wDevID, MIDM_PREPARE, lpDesc->dwInstance, 
		      (DWORD)lpMidiInHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiInPrepareHeader	[MMSYSTEM.306]
 */
UINT16 WINAPI midiInPrepareHeader16(HMIDIIN16 hMidiIn,
                                    MIDIHDR16* lpMidiInHdr, UINT16 uSize)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %d)\n", 
	  hMidiIn, lpMidiInHdr, uSize);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpMidiInHdr->reserved = (DWORD)PTR_SEG_TO_LIN(lpMidiInHdr->lpData);
    return midMessage(lpDesc->wDevID, MIDM_PREPARE, lpDesc->dwInstance, 
		      (DWORD)lpMidiInHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[WINMM.71]
 */
UINT WINAPI midiInUnprepareHeader(HMIDIIN hMidiIn,
				  MIDIHDR16* lpMidiInHdr, UINT uSize)
{
    return midiInUnprepareHeader16(hMidiIn, lpMidiInHdr,uSize);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[MMSYSTEM.307]
 */
UINT16 WINAPI midiInUnprepareHeader16(HMIDIIN16 hMidiIn,
                                      MIDIHDR16* lpMidiInHdr, UINT16 uSize)
{
    LPMIDIOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X, %p, %d)\n", 
	  hMidiIn, lpMidiInHdr, uSize);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return midMessage(lpDesc->wDevID, MIDM_UNPREPARE, lpDesc->dwInstance, 
		      (DWORD)lpMidiInHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiInAddBuffer		[WINMM.57]
 */
UINT WINAPI midiInAddBuffer(HMIDIIN hMidiIn,
			    MIDIHDR16* lpMidiInHdr, UINT uSize)
{
    return midiInAddBuffer16(hMidiIn, lpMidiInHdr,uSize);
}

/**************************************************************************
 * 				midiInAddBuffer		[MMSYSTEM.308]
 */
UINT16 WINAPI midiInAddBuffer16(HMIDIIN16 hMidiIn,
                                MIDIHDR16* lpMidiInHdr, UINT16 uSize)
{
    TRACE(mmsys, "midiInAddBuffer\n");
    return 0;
}

/**************************************************************************
 * 				midiInStart			[WINMM.69]
 */
UINT WINAPI midiInStart(HMIDIIN hMidiIn)
{
    return midiInStart16(hMidiIn);
}

/**************************************************************************
 * 				midiInStart			[MMSYSTEM.309]
 */
UINT16 WINAPI midiInStart16(HMIDIIN16 hMidiIn)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hMidiIn);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) 
	return MMSYSERR_INVALHANDLE;
    return midMessage(lpDesc->wDevID, MIDM_START, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				midiInStop			[WINMM.70]
 */
UINT WINAPI midiInStop(HMIDIIN hMidiIn)
{
    return midiInStop16(hMidiIn);
}

/**************************************************************************
 * 				midiInStop			[MMSYSTEM.310]
 */
UINT16 WINAPI midiInStop16(HMIDIIN16 hMidiIn)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hMidiIn);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) 
	return MMSYSERR_INVALHANDLE;
    return midMessage(lpDesc->wDevID, MIDM_STOP, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				midiInReset			[WINMM.68]
 */
UINT WINAPI midiInReset(HMIDIIN hMidiIn)
{
    return midiInReset16(hMidiIn);
}

/**************************************************************************
 * 				midiInReset			[MMSYSTEM.311]
 */
UINT16 WINAPI midiInReset16(HMIDIIN16 hMidiIn)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hMidiIn);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) 
	return MMSYSERR_INVALHANDLE;
    return midMessage(lpDesc->wDevID, MIDM_RESET, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				midiInGetID			[WINMM.63]
 */
UINT WINAPI midiInGetID(HMIDIIN hMidiIn, UINT* lpuDeviceID)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p)\n", hMidiIn, lpuDeviceID);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) 
	return MMSYSERR_INVALHANDLE;
    if (lpuDeviceID == NULL) 
	return MMSYSERR_INVALPARAM;
    *lpuDeviceID = lpDesc->wDevID;
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiInGetID			[MMSYSTEM.312]
 */
UINT16 WINAPI midiInGetID16(HMIDIIN16 hMidiIn, UINT16* lpuDeviceID)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p)\n", hMidiIn, lpuDeviceID);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) 
	return MMSYSERR_INVALHANDLE;
    if (lpuDeviceID == NULL) 
	return MMSYSERR_INVALPARAM;
    *lpuDeviceID = lpDesc->wDevID;
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiInMessage		[WINMM.65]
 */
DWORD WINAPI midiInMessage(HMIDIIN hMidiIn, UINT uMessage, 
			   DWORD dwParam1, DWORD dwParam2)
{
    LPMIDIOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %04X, %08lX, %08lX)\n", 
	  hMidiIn, uMessage, dwParam1, dwParam2);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) 
	return MMSYSERR_INVALHANDLE;
    
    switch (uMessage) {
    case MIDM_OPEN:
	FIXME(mmsys,"can't handle MIDM_OPEN!\n");
	return 0;
    case MIDM_GETDEVCAPS:
	return midiInGetDevCapsA(hMidiIn, (LPMIDIINCAPSA)dwParam1, dwParam2);
    case MIDM_GETNUMDEVS:
    case MIDM_RESET:
    case MIDM_STOP:
    case MIDM_START:
    case MIDM_CLOSE:
	/* no argument conversion needed */
	break;
    case MIDM_PREPARE:
	return midiInPrepareHeader(hMidiIn, (LPMIDIHDR16)dwParam1, dwParam2);
    case MIDM_UNPREPARE:
	return midiInUnprepareHeader(hMidiIn, (LPMIDIHDR16)dwParam1, dwParam2);
    case MIDM_ADDBUFFER:
	return midiInAddBuffer(hMidiIn, (LPMIDIHDR16)dwParam1, dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hMidiIn,uMessage, dwParam1, dwParam2);
	break;
    }
    return midMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				midiInMessage		[MMSYSTEM.313]
 */
DWORD WINAPI midiInMessage16(HMIDIIN16 hMidiIn, UINT16 uMessage, 
                             DWORD dwParam1, DWORD dwParam2)
{
    LPMIDIOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %04X, %08lX, %08lX)\n", hMidiIn, uMessage, dwParam1, dwParam2);

    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case MIDM_OPEN:
	WARN(mmsys,"can't handle MIDM_OPEN!\n");
	return 0;
    case MIDM_GETDEVCAPS:
	return midiInGetDevCaps16(hMidiIn, (LPMIDIINCAPS16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MIDM_GETNUMDEVS:
    case MIDM_RESET:
    case MIDM_STOP:
    case MIDM_START:
    case MIDM_CLOSE:
	/* no argument conversion needed */
	break;
    case MIDM_PREPARE:
	return midiInPrepareHeader16(hMidiIn, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MIDM_UNPREPARE:
	return midiInUnprepareHeader16(hMidiIn, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MIDM_ADDBUFFER:
	return midiInAddBuffer16(hMidiIn, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hMidiIn,uMessage, dwParam1, dwParam2);
	break;
    }
    return midMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

#define	MIDI_STREAM_MAGIC	0xC9080F17

typedef struct {
    DWORD		dwMagic;
    HMIDIOUT		hDevice;
    /*     thread ID */
    DWORD		dwTempo;
    DWORD		dwTimeDiv;
    DWORD		dwMS;
    DWORD		dwStatus; 
    LPMIDIHDR	lpMidiHdr;
} WINE_MIDIStream;

/**************************************************************************
 * 				MMSYSTEM_MidiStreamPlayer	[internal]
 */
static	void	WINE_UNUSED MMSYSTEM_MidiStreamPlayer(WINE_MIDIStream* ms)
{
    /* FIXME: should I sleep a bit when there is no pending lpMidiHdr ? 
     * => provide a better synchronization system
     * midiStreamPause/midiStreamRestart should freeze/thaw this thread
     * global resource should guard access to ms->lpMidiHdr list, or 
     * use a MB to send new LPMIDIHDR
     * an empty ms->lpMidiHdr list should freeze this thread 
     */
    for (;;) {
	if (ms->dwStatus && ms->lpMidiHdr) {
	    LPMIDIEVENT 	me = (LPMIDIEVENT)(ms->lpMidiHdr->lpData + ms->lpMidiHdr->dwOffset);

	    FIXME(mmsys, "Should wait for current time to be in sync with me->dwDeltaTime\n");

	    if (me->dwEvent & MEVT_F_CALLBACK) {
		WARN(mmsys, "Should generate callback when encountering F_CALLBACK flagged midi events...\n");
	    }
	    switch (MEVT_EVENTTYPE(me->dwEvent)) {
	    case MEVT_COMMENT:
		/* do nothing, skip bytes */
		break;
	    case MEVT_LONGMSG:
		WARN(mmsys, "NIY: sending Sysex Event");
		break;
	    case MEVT_NOP:
		break;
	    case MEVT_SHORTMSG:
		midiOutShortMsg(ms->hDevice, MEVT_EVENTPARM(me->dwEvent));
		break;
	    case MEVT_TEMPO:
		ms->dwTempo = MEVT_EVENTPARM(me->dwEvent);
		break;
	    case MEVT_VERSION:
		break;    
	    }
	    ms->lpMidiHdr->dwOffset += sizeof(MIDIEVENT);
	    if (me->dwEvent & MEVT_F_LONG) {
		ms->lpMidiHdr->dwOffset += MEVT_EVENTPARM(me->dwEvent);
	    }
	    if (ms->lpMidiHdr->dwOffset >= ms->lpMidiHdr->dwBufferLength) {
		ms->lpMidiHdr->dwFlags |= MHDR_DONE;
		ms->lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
		ms->lpMidiHdr = (LPMIDIHDR)ms->lpMidiHdr->lpNext; 
		WARN(mmsys, "Should generate callback when done with MIDIHDR...\n");
	    }
	    FIXME(mmsys, "Should accordingly update ms->dwMS with me->dwDeltaTime\n");
	}
    }
}

/**************************************************************************
 * 				midiStreamClose			[WINMM.90]
 */
MMRESULT WINAPI midiStreamClose(HMIDISTRM hms)
{
    WINE_MIDIStream*	ms;

    TRACE(midi, "(%08x)!\n", hms);

    ms = (WINE_MIDIStream*)hms;
    if (!ms || ms->dwMagic != MIDI_STREAM_MAGIC) 	
	return MMSYSERR_INVALHANDLE;

    free(ms);

    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiStreamOpen			[WINMM.91]
 */
MMRESULT WINAPI midiStreamOpen(HMIDISTRM* phms, LPUINT lpuDeviceID, 
			       DWORD cMidi, DWORD dwCallback, 
			       DWORD dwInstance, DWORD fdwOpen) 
{
    WINE_MIDIStream*	ms;

    TRACE(midi, "(%p,%p,%ld, 0x%08lx, 0x%08lx, 0x%08lx), stub!\n",
	  phms, lpuDeviceID,cMidi, dwCallback, dwInstance, fdwOpen);

    if (cMidi != 1 || phms == NULL || lpuDeviceID == NULL)
	return MMSYSERR_INVALPARAM;
    
    ms = malloc(sizeof(WINE_MIDIStream));
    *phms = (HMIDISTRM)ms;
    ms->dwMagic = MIDI_STREAM_MAGIC;
    ms->dwTempo = 50000;	/* FIXME */
    ms->dwTimeDiv = 1;		/* FIXME */
    ms->dwMS = 0;
    ms->dwStatus = 0;

    /* FIXME is this correct ? */
    *lpuDeviceID = 0;
    /* FIXME is this correct ? */
    return midiOutOpen(&(ms->hDevice), *lpuDeviceID, dwCallback, dwInstance, fdwOpen);
}

/**************************************************************************
 * 				midiStreamOut			[WINMM.92]
 */
MMRESULT WINAPI midiStreamOut(HMIDISTRM hms, LPMIDIHDR16 lpMidiHdr, UINT cbMidiHdr) 
{
    FIXME(midi, "(%08x, %p, %u) stub!\n", hms, lpMidiHdr, cbMidiHdr);
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiStreamPause			[WINMM.93]
 */
MMRESULT WINAPI midiStreamPause(HMIDISTRM hms) 
{
    WINE_MIDIStream*	ms;

    TRACE(midi, "(%08x)!\n", hms);

    ms = (WINE_MIDIStream*)hms;
    if (!ms || ms->dwMagic != MIDI_STREAM_MAGIC) 	
	return MMSYSERR_INVALHANDLE;

    ms->dwStatus = 0;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiStreamPosition		[WINMM.94]
 */
MMRESULT WINAPI midiStreamPosition(HMIDISTRM hms, LPMMTIME lpMMT, UINT cbmmt) 
{
    WINE_MIDIStream*	ms;
    DWORD		ret = MMSYSERR_NOERROR;

    FIXME(midi, "(%08x, %p, %u) stub!\n", hms, lpMMT, cbmmt);

    ms = (WINE_MIDIStream*)hms;

    if (!ms || ms->dwMagic != MIDI_STREAM_MAGIC) {
	ret = MMSYSERR_INVALHANDLE;
    } else if (lpMMT == NULL || cbmmt != sizeof(MMTIME)) {
	ret = MMSYSERR_INVALPARAM;
    } else {
	switch (lpMMT->wType) {
	case TIME_MS:	
	    lpMMT->u.ms = ms->dwMS;	
	    break;
	default:		
	    lpMMT->wType = TIME_MS;
	    ret = MMSYSERR_INVALPARAM;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamProperty		[WINMM.95]
 */
MMRESULT WINAPI midiStreamProperty(HMIDISTRM hms, LPBYTE lpPropData, DWORD dwProperty) 
{
    WINE_MIDIStream*	ms = (WINE_MIDIStream*)hms;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE(midi, "(%08x, %p, %lx)\n", hms, lpPropData, dwProperty);

    if (!ms || ms->dwMagic != MIDI_STREAM_MAGIC) {	
	ret = MMSYSERR_INVALHANDLE;
    } else if ((dwProperty & (MIDIPROP_GET|MIDIPROP_SET)) == 0) {
	return MMSYSERR_INVALPARAM;
    } else if (dwProperty & MIDIPROP_TEMPO) {
	MIDIPROPTEMPO*	mpt = (MIDIPROPTEMPO*)lpPropData;
	
	if (sizeof(MIDIPROPTEMPO) != mpt->cbStruct) {
	    ret = MMSYSERR_INVALPARAM;
	} else if (dwProperty & MIDIPROP_GET) {
	    ms->dwTempo = mpt->dwTempo;
	} else if (dwProperty & MIDIPROP_SET) {
	    mpt->dwTempo = ms->dwTempo;
	}
    } else if (dwProperty & MIDIPROP_TIMEDIV) {
	MIDIPROPTIMEDIV*	mptd = (MIDIPROPTIMEDIV*)lpPropData;
	
	if (sizeof(MIDIPROPTIMEDIV) != mptd->cbStruct) {
	    ret = MMSYSERR_INVALPARAM;
	} else if (dwProperty & MIDIPROP_GET) {
	    ms->dwTimeDiv = mptd->dwTimeDiv;
	} else if (dwProperty & MIDIPROP_SET) {
	    mptd->dwTimeDiv = ms->dwTimeDiv;
	}    
    } else {
	ret = MMSYSERR_INVALPARAM;
    }

    return ret;
}

/**************************************************************************
 * 				midiStreamRestart		[WINMM.96]
 */
MMRESULT WINAPI midiStreamRestart(HMIDISTRM hms) 
{
    WINE_MIDIStream*	ms;

    TRACE(midi, "(%08x)!\n", hms);

    ms = (WINE_MIDIStream*)hms;
    if (!ms || ms->dwMagic != MIDI_STREAM_MAGIC) 	
	return MMSYSERR_INVALHANDLE;

    ms->dwStatus = 1;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiStreamStop			[WINMM.97]
 */
MMRESULT WINAPI midiStreamStop(HMIDISTRM hms) 
{
    FIXME(midi, "(%08x stub!)\n", hms);
	return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiStreamClose			[MMSYSTEM.252]
 */
MMRESULT16 WINAPI midiStreamClose16(HMIDISTRM16 hms)
{
    return midiStreamClose(hms);
}

/**************************************************************************
 * 				midiStreamOpen			[MMSYSTEM.251]
 */
MMRESULT16 WINAPI midiStreamOpen16(HMIDISTRM16* phms, LPUINT16 devid, 
				   DWORD cMidi, DWORD dwCallback, 
				   DWORD dwInstance, DWORD fdwOpen) 
{
    HMIDISTRM	hms32;
    MMRESULT 	ret;
    UINT	devid32;

    if (!phms || !devid)
	return MMSYSERR_INVALPARAM;
    devid32 = *devid;
    ret = midiStreamOpen(&hms32, &devid32, cMidi, dwCallback, dwInstance, fdwOpen);
    *phms = hms32;
    *devid = devid32;
    return ret;
}

/**************************************************************************
 * 				midiStreamOut			[MMSYSTEM.254]
 */
MMRESULT16 WINAPI midiStreamOut16(HMIDISTRM16 hms, LPMIDIHDR16 lpMidiHdr, UINT16 cbMidiHdr) 
{
    return midiStreamOut(hms, lpMidiHdr, cbMidiHdr);
}

/**************************************************************************
 * 				midiStreamPause			[MMSYSTEM.255]
 */
MMRESULT16 WINAPI midiStreamPause16(HMIDISTRM16 hms) 
{
    return midiStreamPause(hms);
}

/**************************************************************************
 * 				midiStreamPosition		[MMSYSTEM.253]
 */
MMRESULT16 WINAPI midiStreamPosition16(HMIDISTRM16 hms, LPMMTIME16 lpmmt, UINT16 cbmmt) 
{
    MMTIME	mmt32;
    MMRESULT	ret;

    if (!lpmmt)
	return MMSYSERR_INVALPARAM;
    MMSYSTEM_MMTIME16to32(&mmt32, lpmmt);
    ret = midiStreamPosition(hms, &mmt32, cbmmt);
    MMSYSTEM_MMTIME32to16(lpmmt, &mmt32);
    return ret;
}

/**************************************************************************
 * 				midiStreamProperty		[MMSYSTEM.250]
 */
MMRESULT16 WINAPI midiStreamProperty16(HMIDISTRM16 hms, LPBYTE lpPropData, DWORD dwProperty) 
{
    return midiStreamProperty(hms, lpPropData, dwProperty);
}

/**************************************************************************
 * 				midiStreamRestart		[MMSYSTEM.256]
 */
MMRESULT16 WINAPI midiStreamRestart16(HMIDISTRM16 hms) 
{
    return midiStreamRestart(hms);
}

/**************************************************************************
 * 				midiStreamStop			[MMSYSTEM.257]
 */
MMRESULT16 WINAPI midiStreamStop16(HMIDISTRM16 hms) 
{
    return midiStreamStop(hms);
}

/**************************************************************************
 * 				waveOutGetNumDevs		[MMSYSTEM.401]
 */
UINT WINAPI waveOutGetNumDevs() 
{
    return waveOutGetNumDevs16();
}

/**************************************************************************
 * 				waveOutGetNumDevs		[WINMM.167]
 */
UINT16 WINAPI waveOutGetNumDevs16()
{
    UINT16	count = 0;
    TRACE(mmsys, "waveOutGetNumDevs\n");
    /* FIXME: I'm not sure MCI_FirstDevID() is correct */
    count += wodMessage(MCI_FirstDevID(), WODM_GETNUMDEVS, 0L, 0L, 0L);
    TRACE(mmsys, "waveOutGetNumDevs return %u \n", count);
    return count;
}

/**************************************************************************
 * 				waveOutGetDevCaps		[MMSYSTEM.402]
 */
UINT16 WINAPI waveOutGetDevCaps16(UINT16 uDeviceID, LPWAVEOUTCAPS16 lpCaps,
				  UINT16 uSize)
{
    if (uDeviceID > waveOutGetNumDevs16() - 1) return MMSYSERR_BADDEVICEID;
    if (uDeviceID == (UINT16)WAVE_MAPPER) return MMSYSERR_BADDEVICEID; /* FIXME: do we have a wave mapper ? */
    TRACE(mmsys, "waveOutGetDevCaps\n");
    return wodMessage(uDeviceID, WODM_GETDEVCAPS, 0L, (DWORD)lpCaps, uSize);
}

/**************************************************************************
 * 				waveOutGetDevCapsA		[WINMM.162]
 */
UINT WINAPI waveOutGetDevCapsA(UINT uDeviceID, LPWAVEOUTCAPSA lpCaps,
			       UINT uSize)
{
    WAVEOUTCAPS16	woc16;
    UINT16 ret = waveOutGetDevCaps16(uDeviceID, &woc16, sizeof(woc16));
    
    lpCaps->wMid = woc16.wMid;
    lpCaps->wPid = woc16.wPid;
    lpCaps->vDriverVersion = woc16.vDriverVersion;
    strcpy(lpCaps->szPname, woc16.szPname);
    lpCaps->dwFormats = woc16.dwFormats;
    lpCaps->wChannels = woc16.wChannels;
    lpCaps->dwSupport = woc16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				waveOutGetDevCapsW		[WINMM.163]
 */
UINT WINAPI waveOutGetDevCapsW(UINT uDeviceID, LPWAVEOUTCAPSW lpCaps,
				   UINT uSize)
{
    WAVEOUTCAPS16	woc16;
    UINT ret = waveOutGetDevCaps16(uDeviceID, &woc16, sizeof(woc16));
    
    lpCaps->wMid = woc16.wMid;
    lpCaps->wPid = woc16.wPid;
    lpCaps->vDriverVersion = woc16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname, woc16.szPname);
    lpCaps->dwFormats = woc16.dwFormats;
    lpCaps->wChannels = woc16.wChannels;
    lpCaps->dwSupport = woc16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				waveOutGetErrorText 	[MMSYSTEM.403]
 */
UINT16 WINAPI waveOutGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    TRACE(mmsys, "waveOutGetErrorText\n");
    return(waveGetErrorText(uError, lpText, uSize));
}

/**************************************************************************
 * 				waveOutGetErrorTextA 	[WINMM.164]
 */
UINT WINAPI waveOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return(waveOutGetErrorText16(uError, lpText, uSize));
}

/**************************************************************************
 * 				waveOutGetErrorTextW 	[WINMM.165]
 */
UINT WINAPI waveOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0,uSize);
    UINT	ret = waveOutGetErrorTextA(uError, xstr, uSize);
    
    lstrcpyAtoW(lpText,xstr);
    HeapFree(GetProcessHeap(), 0,xstr);
    return ret;
}

/**************************************************************************
 * 				waveGetErrorText 		[internal]
 */
static UINT16 waveGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    LPSTR	msgptr;
    TRACE(mmsys, "(%04X, %p, %d);\n", 
	  uError, lpText, uSize);
    if ((lpText == NULL) || (uSize < 1)) return(FALSE);
    lpText[0] = '\0';
    switch (uError) {
    case MMSYSERR_NOERROR:
	msgptr = "The specified command was carried out.";
	break;
    case MMSYSERR_ERROR:
	msgptr = "Undefined external error.";
	break;
    case MMSYSERR_BADDEVICEID:
	msgptr = "A device ID has been used that is out of range for your system.";
	break;
    case MMSYSERR_NOTENABLED:
	msgptr = "The driver was not enabled.";
	break;
    case MMSYSERR_ALLOCATED:
	msgptr = "The specified device is already in use. Wait until it is free, and then try again.";
	break;
    case MMSYSERR_INVALHANDLE:
	msgptr = "The specified device handle is invalid.";
	break;
    case MMSYSERR_NODRIVER:
	msgptr = "There is no driver installed on your system !\n";
	break;
    case MMSYSERR_NOMEM:
	msgptr = "Not enough memory available for this task. Quit one or more applications to increase available memory, and then try again.";
	break;
    case MMSYSERR_NOTSUPPORTED:
	msgptr = "This function is not supported. Use the Capabilities function to determine which functions and messages the driver supports.";
	break;
    case MMSYSERR_BADERRNUM:
	msgptr = "An error number was specified that is not defined in the system.";
	break;
    case MMSYSERR_INVALFLAG:
	msgptr = "An invalid flag was passed to a system function.";
	break;
    case MMSYSERR_INVALPARAM:
	msgptr = "An invalid parameter was passed to a system function.";
	break;
    case WAVERR_BADFORMAT:
	msgptr = "The specified format is not supported or cannot be translated. Use the Capabilities function to determine the supported formats";
	break;
    case WAVERR_STILLPLAYING:
	msgptr = "Cannot perform this operation while media data is still playing. Reset the device, or wait until the data is finished playing.";
	break;
    case WAVERR_UNPREPARED:
	msgptr = "The wave header was not prepared. Use the Prepare function to prepare the header, and then try again.";
	break;
    case WAVERR_SYNC:
	msgptr = "Cannot open the device without using the WAVE_ALLOWSYNC flag. Use the flag, and then try again.";
	break;
    default:
	msgptr = "Unknown MMSYSTEM Error !\n";
	break;
    }
    lstrcpynA(lpText, msgptr, uSize);
    return TRUE;
}

/**************************************************************************
 *			waveOutOpen			[WINMM.173]
 * All the args/structs have the same layout as the win16 equivalents
 */
UINT WINAPI waveOutOpen(HWAVEOUT* lphWaveOut, UINT uDeviceID,
			const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
			DWORD dwInstance, DWORD dwFlags)
{
    HWAVEOUT16	hwo16;
    UINT	ret = waveOutOpen16(&hwo16,uDeviceID, lpFormat, dwCallback, dwInstance,
				    CALLBACK32CONV(dwFlags));

    if (lphWaveOut) *lphWaveOut=hwo16;
    return ret;
}

/**************************************************************************
 *			waveOutOpen			[MMSYSTEM.404]
 */
UINT16 WINAPI waveOutOpen16(HWAVEOUT16* lphWaveOut, UINT16 uDeviceID,
                            const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
                            DWORD dwInstance, DWORD dwFlags)
{
    HWAVEOUT16	hWaveOut;
    LPWAVEOPENDESC	lpDesc;
    DWORD		dwRet = 0;
    BOOL		bMapperFlg = FALSE;
    
    TRACE(mmsys, "(%p, %d, %p, %08lX, %08lX, %08lX);\n", 
	  lphWaveOut, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
    if (dwFlags & WAVE_FORMAT_QUERY)
	TRACE(mmsys, "WAVE_FORMAT_QUERY requested !\n");
    if (uDeviceID == (UINT16)WAVE_MAPPER) {
	TRACE(mmsys, "WAVE_MAPPER mode requested !\n");
	bMapperFlg = TRUE;
	uDeviceID = 0;
    }
    if (lpFormat == NULL) return WAVERR_BADFORMAT;
    
    hWaveOut = USER_HEAP_ALLOC(sizeof(WAVEOPENDESC));
    if (lphWaveOut != NULL) *lphWaveOut = hWaveOut;
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_NOMEM;
    lpDesc->hWave = hWaveOut;
    lpDesc->lpFormat = (LPWAVEFORMAT)lpFormat;  /* should the struct be copied iso pointer? */
    lpDesc->dwCallBack = dwCallback;
    lpDesc->dwInstance = dwInstance;
    if (uDeviceID >= MAXWAVEDRIVERS)
	uDeviceID = 0;
    while (uDeviceID < MAXWAVEDRIVERS) {
	dwRet = wodMessage(uDeviceID, WODM_OPEN, 
			   lpDesc->dwInstance, (DWORD)lpDesc, dwFlags);
	if (dwRet == MMSYSERR_NOERROR) break;
	if (!bMapperFlg) break;
	uDeviceID++;
	TRACE(mmsys, "WAVE_MAPPER mode ! try next driver...\n");
    }
    lpDesc->uDeviceID = uDeviceID;  /* save physical Device ID */
    if (dwFlags & WAVE_FORMAT_QUERY) {
	TRACE(mmsys, "End of WAVE_FORMAT_QUERY !\n");
	dwRet = waveOutClose(hWaveOut);
    }
    return dwRet;
}

/**************************************************************************
 * 				waveOutClose		[WINMM.161]
 */
UINT WINAPI waveOutClose(HWAVEOUT hWaveOut)
{
    return waveOutClose16(hWaveOut);
}

/**************************************************************************
 * 				waveOutClose		[MMSYSTEM.405]
 */
UINT16 WINAPI waveOutClose16(HWAVEOUT16 hWaveOut)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hWaveOut);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return wodMessage(lpDesc->uDeviceID, WODM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveOutPrepareHeader	[WINMM.175]
 */
UINT WINAPI waveOutPrepareHeader(HWAVEOUT hWaveOut,
				 WAVEHDR* lpWaveOutHdr, UINT uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->reserved = (DWORD)lpWaveOutHdr->lpData;
    return wodMessage(lpDesc->uDeviceID, WODM_PREPARE, lpDesc->dwInstance, 
		       (DWORD)lpWaveOutHdr,uSize);
}

/**************************************************************************
 * 				waveOutPrepareHeader	[MMSYSTEM.406]
 */
UINT16 WINAPI waveOutPrepareHeader16(HWAVEOUT16 hWaveOut,
                                     WAVEHDR* lpWaveOutHdr, UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    UINT16		ret;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->reserved = (DWORD)lpWaveOutHdr->lpData;
    lpWaveOutHdr->lpData = PTR_SEG_TO_LIN(lpWaveOutHdr->lpData);
    ret = wodMessage(lpDesc->uDeviceID, WODM_PREPARE, lpDesc->dwInstance, 
		      (DWORD)lpWaveOutHdr,uSize);
    lpWaveOutHdr->lpData = lpWaveOutHdr->lpData;
    return ret;
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[WINMM.181]
 */
UINT WINAPI waveOutUnprepareHeader(HWAVEOUT hWaveOut,
				   WAVEHDR* lpWaveOutHdr, UINT uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->reserved = (DWORD)lpWaveOutHdr->lpData;
    return wodMessage(lpDesc->uDeviceID, WODM_UNPREPARE, lpDesc->dwInstance, 
		      (DWORD)lpWaveOutHdr, uSize);
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[MMSYSTEM.407]
 */
UINT16 WINAPI waveOutUnprepareHeader16(HWAVEOUT16 hWaveOut,
				       WAVEHDR* lpWaveOutHdr, UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    UINT16		ret;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->reserved = (DWORD)lpWaveOutHdr->lpData;
    lpWaveOutHdr->lpData = PTR_SEG_TO_LIN(lpWaveOutHdr->lpData);
    ret = wodMessage(lpDesc->uDeviceID, WODM_UNPREPARE, lpDesc->dwInstance, 
		     (DWORD)lpWaveOutHdr, uSize);
    lpWaveOutHdr->lpData = (LPBYTE)lpWaveOutHdr->reserved;
    return ret;
}

/**************************************************************************
 * 				waveOutWrite		[MMSYSTEM.408]
 */
UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, WAVEHDR* lpWaveOutHdr,
			 UINT uSize)
{
    LPWAVEOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->reserved = (DWORD)lpWaveOutHdr->lpData;
    return wodMessage(lpDesc->uDeviceID, WODM_WRITE, lpDesc->dwInstance, (DWORD)lpWaveOutHdr, uSize);
}

/**************************************************************************
 * 				waveOutWrite		[MMSYSTEM.408]
 */
UINT16 WINAPI waveOutWrite16(HWAVEOUT16 hWaveOut, WAVEHDR * lpWaveOutHdr,
			     UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    UINT16		ret;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->reserved =(DWORD)lpWaveOutHdr->lpData;/*save original ptr*/
    lpWaveOutHdr->lpData = PTR_SEG_TO_LIN(lpWaveOutHdr->lpData);
    ret = wodMessage(lpDesc->uDeviceID, WODM_WRITE, lpDesc->dwInstance, (DWORD)lpWaveOutHdr, uSize);
    lpWaveOutHdr->lpData = (LPBYTE)lpWaveOutHdr->reserved;
    return ret;
}

/**************************************************************************
 * 				waveOutPause		[WINMM.174]
 */
UINT WINAPI waveOutPause(HWAVEOUT hWaveOut)
{
    return waveOutPause16(hWaveOut);
}

/**************************************************************************
 * 				waveOutPause		[MMSYSTEM.409]
 */
UINT16 WINAPI waveOutPause16(HWAVEOUT16 hWaveOut)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hWaveOut);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return wodMessage(lpDesc->uDeviceID, WODM_PAUSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveOutRestart		[WINMM.177]
 */
UINT WINAPI waveOutRestart(HWAVEOUT hWaveOut)
{
    return waveOutRestart16(hWaveOut);
}

/**************************************************************************
 * 				waveOutRestart		[MMSYSTEM.410]
 */
UINT16 WINAPI waveOutRestart16(HWAVEOUT16 hWaveOut)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hWaveOut);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return wodMessage(lpDesc->uDeviceID, WODM_RESTART, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveOutReset		[WINMM.176]
 */
UINT WINAPI waveOutReset(HWAVEOUT hWaveOut)
{
    return waveOutReset16(hWaveOut);
}

/**************************************************************************
 * 				waveOutReset		[MMSYSTEM.411]
 */
UINT16 WINAPI waveOutReset16(HWAVEOUT16 hWaveOut)
{
    LPWAVEOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X)\n", hWaveOut);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return wodMessage(lpDesc->uDeviceID, WODM_RESET, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveOutGetPosition	[WINMM.170]
 */
UINT WINAPI waveOutGetPosition(HWAVEOUT hWaveOut, LPMMTIME lpTime,
			       UINT uSize)
{
    MMTIME16	mmt16;
    UINT ret;
    
    mmt16.wType = lpTime->wType;
    ret = waveOutGetPosition16(hWaveOut, &mmt16, sizeof(mmt16));
    MMSYSTEM_MMTIME16to32(lpTime, &mmt16);
    return ret;
}

/**************************************************************************
 * 				waveOutGetPosition	[MMSYSTEM.412]
 */
UINT16 WINAPI waveOutGetPosition16(HWAVEOUT16 hWaveOut, LPMMTIME16 lpTime,
                                   UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpTime, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return wodMessage(lpDesc->uDeviceID, WODM_GETPOS, lpDesc->dwInstance, 
		       (DWORD)lpTime, (DWORD)uSize);
}

#define WAVEOUT_SHORTCUT_1(xx,XX,atype) \
	UINT WINAPI waveOut##xx(HWAVEOUT hWaveOut, atype x)	\
{									\
	return waveOut##xx##16(hWaveOut,x);				\
}									\
UINT16 WINAPI waveOut##xx##16(HWAVEOUT16 hWaveOut, atype x)		\
{									\
	LPWAVEOPENDESC	lpDesc;						\
	TRACE(mmsys, "(%04X, %08lx);\n", hWaveOut, (DWORD)x);           \
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);		\
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;		\
	return wodMessage(lpDesc->uDeviceID, WODM_##XX, 		\
			  lpDesc->dwInstance, (DWORD)x, 0L);		\
}

WAVEOUT_SHORTCUT_1(GetPitch,GETPITCH, DWORD*)
WAVEOUT_SHORTCUT_1(SetPitch,SETPITCH, DWORD)
WAVEOUT_SHORTCUT_1(GetPlaybackRate,GETPLAYBACKRATE, DWORD*)
WAVEOUT_SHORTCUT_1(SetPlaybackRate,SETPLAYBACKRATE, DWORD)
    
#define WAVEOUT_SHORTCUT_2(xx,XX,atype) \
	UINT WINAPI waveOut##xx(UINT devid, atype x)		\
{									\
	return waveOut##xx##16(devid,x);				\
}									\
UINT16 WINAPI waveOut##xx##16(UINT16 devid, atype x)			\
{									\
	TRACE(mmsys, "(%04X, %08lx);\n", devid, (DWORD)x);	        \
	return wodMessage(devid, WODM_##XX, 0L,	(DWORD)x, 0L);		\
}
    
WAVEOUT_SHORTCUT_2(GetVolume,GETVOLUME, DWORD*)
WAVEOUT_SHORTCUT_2(SetVolume,SETVOLUME, DWORD)
    
/**************************************************************************
 * 				waveOutBreakLoop 	[MMSYSTEM.419]
 */
UINT WINAPI waveOutBreakLoop(HWAVEOUT hWaveOut)
{
    return waveOutBreakLoop16(hWaveOut);
}

/**************************************************************************
 * 				waveOutBreakLoop 	[MMSYSTEM.419]
 */
UINT16 WINAPI waveOutBreakLoop16(HWAVEOUT16 hWaveOut)
{
    TRACE(mmsys, "(%04X)\n", hWaveOut);
    return MMSYSERR_INVALHANDLE;
}

/**************************************************************************
 * 				waveOutGetID	 	[MMSYSTEM.420]
 */
UINT WINAPI waveOutGetID(HWAVEOUT hWaveOut, UINT* lpuDeviceID)
{
    LPWAVEOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %p);\n", hWaveOut, lpuDeviceID);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;
    *lpuDeviceID = lpDesc->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveOutGetID	 	[MMSYSTEM.420]
 */
UINT16 WINAPI waveOutGetID16(HWAVEOUT16 hWaveOut, UINT16* lpuDeviceID)
{
    LPWAVEOPENDESC	lpDesc;

    TRACE(mmsys, "(%04X, %p);\n", hWaveOut, lpuDeviceID);

    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;
    *lpuDeviceID = lpDesc->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveOutMessage 		[MMSYSTEM.421]
 */
DWORD WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage, 
                              DWORD dwParam1, DWORD dwParam2)
{
    LPWAVEOPENDESC	lpDesc;
    
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case WODM_GETNUMDEVS:
    case WODM_GETPOS:
    case WODM_GETVOLUME:
    case WODM_GETPITCH:
    case WODM_GETPLAYBACKRATE:
    case WODM_SETVOLUME:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_RESET:
    case WODM_PAUSE:
    case WODM_PREPARE:
    case WODM_UNPREPARE:
    case WODM_STOP:
    case WODM_CLOSE:
	/* no argument conversion needed */
	break;
    case WODM_WRITE:
	return waveOutWrite(hWaveOut, (LPWAVEHDR)dwParam1, dwParam2);
    case WODM_GETDEVCAPS:
	/* FIXME: UNICODE/ANSI? */
	return waveOutGetDevCapsA(hWaveOut, (LPWAVEOUTCAPSA)dwParam1, dwParam2);
    case WODM_OPEN:
	FIXME(mmsys,"can't handle WODM_OPEN, please report.\n");
	break;
    default:
	ERR(mmsys,"(0x%04x, 0x%04x,%08lx,%08lx): unhandled message\n",
	    hWaveOut,uMessage, dwParam1, dwParam2);
	break;
    }
    return wodMessage(lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				waveOutMessage 		[MMSYSTEM.421]
 */
DWORD WINAPI waveOutMessage16(HWAVEOUT16 hWaveOut, UINT16 uMessage, 
                              DWORD dwParam1, DWORD dwParam2)
{
    LPWAVEOPENDESC	lpDesc;
    
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case WODM_GETNUMDEVS:
    case WODM_SETVOLUME:
    case WODM_SETPITCH:
    case WODM_SETPLAYBACKRATE:
    case WODM_RESET:
    case WODM_PAUSE:
    case WODM_STOP:
    case WODM_CLOSE:
	/* no argument conversion needed */
	break;
    case WODM_GETPOS:
	return waveOutGetPosition16(hWaveOut, (LPMMTIME16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WODM_GETVOLUME:
	return waveOutGetVolume16(hWaveOut, (LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case WODM_GETPITCH:
	return waveOutGetPitch16(hWaveOut, (LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case WODM_GETPLAYBACKRATE:
	return waveOutGetPlaybackRate16(hWaveOut, (LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case WODM_GETDEVCAPS:
	return waveOutGetDevCaps16(hWaveOut, (LPWAVEOUTCAPS16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WODM_PREPARE:
	return waveOutPrepareHeader16(hWaveOut, (LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WODM_UNPREPARE:
	return waveOutUnprepareHeader16(hWaveOut, (LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WODM_WRITE:
	return waveOutWrite16(hWaveOut, (LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WODM_OPEN:
	FIXME(mmsys,"can't handle WODM_OPEN, please report.\n");
	break;
    default:
	ERR(mmsys,"(0x%04x, 0x%04x,%08lx,%08lx): unhandled message\n",
	    hWaveOut,uMessage, dwParam1, dwParam2);
    }
    return wodMessage(lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				waveInGetNumDevs 		[WINMM.151]
 */
UINT WINAPI waveInGetNumDevs()
{
    return waveInGetNumDevs16();
}

/**************************************************************************
 * 				waveInGetNumDevs 		[MMSYSTEM.501]
 */
UINT16 WINAPI waveInGetNumDevs16()
{
    UINT16	count = 0;

    TRACE(mmsys, "waveInGetNumDevs\n");
    count += widMessage(0, WIDM_GETNUMDEVS, 0L, 0L, 0L);
    TRACE(mmsys, "waveInGetNumDevs return %u \n", count);
    return count;
}

/**************************************************************************
 * 				waveInGetDevCapsA 		[WINMM.147]
 */
UINT WINAPI waveInGetDevCapsW(UINT uDeviceID, LPWAVEINCAPSW lpCaps, UINT uSize)
{
    WAVEINCAPS16	wic16;
    UINT		ret = waveInGetDevCaps16(uDeviceID, &wic16,uSize);
    
    lpCaps->wMid = wic16.wMid;
    lpCaps->wPid = wic16.wPid;
    lpCaps->vDriverVersion = wic16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname, wic16.szPname);
    lpCaps->dwFormats = wic16.dwFormats;
    lpCaps->wChannels = wic16.wChannels;
    
    return ret;
}

/**************************************************************************
 * 				waveInGetDevCapsA 		[WINMM.146]
 */
UINT WINAPI waveInGetDevCapsA(UINT uDeviceID, LPWAVEINCAPSA lpCaps, UINT uSize)
{
    WAVEINCAPS16	wic16;
    UINT		ret = waveInGetDevCaps16(uDeviceID, &wic16,uSize);
    
    lpCaps->wMid = wic16.wMid;
    lpCaps->wPid = wic16.wPid;
    lpCaps->vDriverVersion = wic16.vDriverVersion;
    strcpy(lpCaps->szPname, wic16.szPname);
    lpCaps->dwFormats = wic16.dwFormats;
    lpCaps->wChannels = wic16.wChannels;
    return ret;
}

/**************************************************************************
 * 				waveInGetDevCaps 		[MMSYSTEM.502]
 */
UINT16 WINAPI waveInGetDevCaps16(UINT16 uDeviceID, LPWAVEINCAPS16 lpCaps, UINT16 uSize)
{
    TRACE(mmsys, "waveInGetDevCaps\n");

    return widMessage(uDeviceID, WIDM_GETDEVCAPS, 0L, (DWORD)lpCaps, uSize);
}

/**************************************************************************
 * 				waveInGetErrorTextA 	[WINMM.148]
 */
UINT WINAPI waveInGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    TRACE(mmsys, "waveInGetErrorText\n");
    return(waveGetErrorText(uError, lpText, uSize));
}

/**************************************************************************
 * 				waveInGetErrorTextW 	[WINMM.149]
 */
UINT WINAPI waveInGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR txt = HeapAlloc(GetProcessHeap(), 0,uSize);
    UINT	ret = waveGetErrorText(uError, txt, uSize);
    
    lstrcpyAtoW(lpText, txt);
    HeapFree(GetProcessHeap(), 0, txt);
    return ret;
}

/**************************************************************************
 * 				waveInGetErrorText 	[MMSYSTEM.503]
 */
UINT16 WINAPI waveInGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    TRACE(mmsys, "waveInGetErrorText\n");
    return(waveGetErrorText(uError, lpText, uSize));
}

/**************************************************************************
 * 				waveInOpen			[WINMM.154]
 */
UINT WINAPI waveInOpen(HWAVEIN* lphWaveIn, UINT uDeviceID,
		       const LPWAVEFORMAT lpFormat, DWORD dwCallback,
		       DWORD dwInstance, DWORD dwFlags)
{
    HWAVEIN16	hwin16;
    UINT	ret = waveInOpen16(&hwin16,uDeviceID, lpFormat, dwCallback, dwInstance,
				 CALLBACK32CONV(dwFlags));
    if (lphWaveIn) *lphWaveIn = hwin16;
    return ret;
}

/**************************************************************************
 * 				waveInOpen			[MMSYSTEM.504]
 */
UINT16 WINAPI waveInOpen16(HWAVEIN16* lphWaveIn, UINT16 uDeviceID,
                           const LPWAVEFORMAT lpFormat, DWORD dwCallback,
                           DWORD dwInstance, DWORD dwFlags)
{
    HWAVEIN16 hWaveIn;
    LPWAVEOPENDESC	lpDesc;
    DWORD	dwRet = 0;
    BOOL	bMapperFlg = FALSE;

    TRACE(mmsys, "(%p, %d, %p, %08lX, %08lX, %08lX);\n", 
	  lphWaveIn, uDeviceID, lpFormat, dwCallback, dwInstance, dwFlags);
    if (dwFlags & WAVE_FORMAT_QUERY)
	TRACE(mmsys, "WAVE_FORMAT_QUERY requested !\n");
    if (uDeviceID == (UINT16)WAVE_MAPPER) {
	TRACE(mmsys, "WAVE_MAPPER mode requested !\n");
	bMapperFlg = TRUE;
	uDeviceID = 0;
    }
    if (lpFormat == NULL) return WAVERR_BADFORMAT;
    hWaveIn = USER_HEAP_ALLOC(sizeof(WAVEOPENDESC));
    if (lphWaveIn != NULL) *lphWaveIn = hWaveIn;
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_NOMEM;
    lpDesc->hWave = hWaveIn;
    lpDesc->lpFormat = lpFormat;
    lpDesc->dwCallBack = dwCallback;
    lpDesc->dwInstance = dwInstance;
    while (uDeviceID < MAXWAVEDRIVERS) {
	dwRet = widMessage(uDeviceID, WIDM_OPEN, 
			   lpDesc->dwInstance, (DWORD)lpDesc, 0L);
	if (dwRet == MMSYSERR_NOERROR) break;
	if (!bMapperFlg) break;
	uDeviceID++;
	TRACE(mmsys, "WAVE_MAPPER mode ! try next driver...\n");
    }
    lpDesc->uDeviceID = uDeviceID;
    if (dwFlags & WAVE_FORMAT_QUERY) {
	TRACE(mmsys, "End of WAVE_FORMAT_QUERY !\n");
	dwRet = waveInClose16(hWaveIn);
    }
    return dwRet;
}

/**************************************************************************
 * 				waveInClose			[WINMM.145]
 */
UINT WINAPI waveInClose(HWAVEIN hWaveIn)
{
    return waveInClose16(hWaveIn);
}

/**************************************************************************
 * 				waveInClose			[MMSYSTEM.505]
 */
UINT16 WINAPI waveInClose16(HWAVEIN16 hWaveIn)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hWaveIn);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return widMessage(lpDesc->uDeviceID, WIDM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveInPrepareHeader		[WINMM.155]
 */
UINT WINAPI waveInPrepareHeader(HWAVEIN hWaveIn,
				WAVEHDR* lpWaveInHdr, UINT uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", 
	  hWaveIn, lpWaveInHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveInHdr = lpWaveInHdr;
    lpWaveInHdr->lpNext = NULL;
    lpWaveInHdr->dwBytesRecorded = 0;
    TRACE(mmsys, "lpData=%p size=%lu \n", 
	  lpWaveInHdr->lpData, lpWaveInHdr->dwBufferLength);
    return widMessage(lpDesc->uDeviceID, WIDM_PREPARE, lpDesc->dwInstance, 
		      (DWORD)lpWaveInHdr, uSize);
}

/**************************************************************************
 * 				waveInPrepareHeader		[MMSYSTEM.506]
 */
UINT16 WINAPI waveInPrepareHeader16(HWAVEIN16 hWaveIn,
				    WAVEHDR* lpWaveInHdr, UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    LPBYTE		saveddata = lpWaveInHdr->lpData;
    UINT16		ret;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", 
	  hWaveIn, lpWaveInHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveInHdr = lpWaveInHdr;
    lpWaveInHdr->lpNext = NULL;
    lpWaveInHdr->dwBytesRecorded = 0;
    
    TRACE(mmsys, "lpData=%p size=%lu \n", 
	  lpWaveInHdr->lpData, lpWaveInHdr->dwBufferLength);
    lpWaveInHdr->lpData = PTR_SEG_TO_LIN(lpWaveInHdr->lpData);
    ret = widMessage(lpDesc->uDeviceID, WIDM_PREPARE, lpDesc->dwInstance, 
		     (DWORD)lpWaveInHdr,uSize);
    lpWaveInHdr->lpData = saveddata;
    return ret;
}

/**************************************************************************
 * 				waveInUnprepareHeader	[WINMM.159]
 */
UINT WINAPI waveInUnprepareHeader(HWAVEIN hWaveIn,
				  WAVEHDR* lpWaveInHdr, UINT uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", 
	  hWaveIn, lpWaveInHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    /*USER_HEAP_FREE(HIWORD((DWORD)lpWaveInHdr->lpData)); FIXME */
    lpWaveInHdr->lpData = NULL;
    lpWaveInHdr->lpNext = NULL;
    return widMessage(lpDesc->uDeviceID, WIDM_UNPREPARE, lpDesc->dwInstance, 
		      (DWORD)lpWaveInHdr, uSize);
}

/**************************************************************************
 * 				waveInUnprepareHeader	[MMSYSTEM.507]
 */
UINT16 WINAPI waveInUnprepareHeader16(HWAVEIN16 hWaveIn,
                                      WAVEHDR* lpWaveInHdr, UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", 
	  hWaveIn, lpWaveInHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    /*USER_HEAP_FREE(HIWORD((DWORD)lpWaveInHdr->lpData)); FIXME */
    lpWaveInHdr->lpData = NULL;
    lpWaveInHdr->lpNext = NULL;
    return widMessage(lpDesc->uDeviceID, WIDM_UNPREPARE, lpDesc->dwInstance, 
		      (DWORD)lpWaveInHdr, uSize);
}

/**************************************************************************
 * 				waveInAddBuffer		[WINMM.144]
 */
UINT WINAPI waveInAddBuffer(HWAVEIN hWaveIn,
			    WAVEHDR* lpWaveInHdr, UINT uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveInHdr->lpNext = NULL;
    lpWaveInHdr->dwBytesRecorded = 0;
    TRACE(mmsys, "lpData=%p size=%lu \n", 
	  lpWaveInHdr->lpData, lpWaveInHdr->dwBufferLength);
    return widMessage(lpDesc->uDeviceID, WIDM_ADDBUFFER, lpDesc->dwInstance,
		      (DWORD)lpWaveInHdr, uSize);
    
}

/**************************************************************************
 * 				waveInAddBuffer		[MMSYSTEM.508]
 */
UINT16 WINAPI waveInAddBuffer16(HWAVEIN16 hWaveIn,
                                WAVEHDR* lpWaveInHdr, UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    UINT16		ret;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveInHdr->lpNext = NULL;
    lpWaveInHdr->dwBytesRecorded = 0;
    lpWaveInHdr->lpData = PTR_SEG_TO_LIN(lpWaveInHdr->lpData);
    TRACE(mmsys, "lpData=%p size=%lu \n", 
	  lpWaveInHdr->lpData, lpWaveInHdr->dwBufferLength);
    ret = widMessage(lpDesc->uDeviceID, WIDM_ADDBUFFER, lpDesc->dwInstance,
		     (DWORD)lpWaveInHdr, uSize);
    /*lpWaveInHdr->lpData = saveddata;*/
    return ret;
}

/**************************************************************************
 * 				waveInStart			[WINMM.157]
 */
UINT WINAPI waveInStart(HWAVEIN hWaveIn)
{
    return waveInStart16(hWaveIn);
}

/**************************************************************************
 * 				waveInStart			[MMSYSTEM.509]
 */
UINT16 WINAPI waveInStart16(HWAVEIN16 hWaveIn)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hWaveIn);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return widMessage(lpDesc->uDeviceID, WIDM_START, lpDesc->dwInstance, 0, 0);
}

/**************************************************************************
 * 				waveInStop			[WINMM.158]
 */
UINT WINAPI waveInStop(HWAVEIN hWaveIn)
{
    return waveInStop16(hWaveIn);
}

/**************************************************************************
 * 				waveInStop			[MMSYSTEM.510]
 */
UINT16 WINAPI waveInStop16(HWAVEIN16 hWaveIn)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hWaveIn);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return widMessage(lpDesc->uDeviceID, WIDM_STOP, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveInReset			[WINMM.156]
 */
UINT WINAPI waveInReset(HWAVEIN hWaveIn)
{
    return waveInReset16(hWaveIn);
}

/**************************************************************************
 * 				waveInReset			[MMSYSTEM.511]
 */
UINT16 WINAPI waveInReset16(HWAVEIN16 hWaveIn)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X)\n", hWaveIn);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return widMessage(lpDesc->uDeviceID, WIDM_RESET, lpDesc->dwInstance, 0, 0);
}

/**************************************************************************
 * 				waveInGetPosition	[WINMM.152]
 */
UINT WINAPI waveInGetPosition(HWAVEIN hWaveIn, LPMMTIME lpTime,
			      UINT uSize)
{
    MMTIME16	mmt16;
    UINT 	ret;
    
    mmt16.wType = lpTime->wType;
    ret = waveInGetPosition16(hWaveIn, &mmt16,uSize);
    
    MMSYSTEM_MMTIME16to32(lpTime, &mmt16);
    return ret;
}

/**************************************************************************
 * 				waveInGetPosition	[MMSYSTEM.512]
 */
UINT16 WINAPI waveInGetPosition16(HWAVEIN16 hWaveIn, LPMMTIME16 lpTime,
                                  UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveIn, lpTime, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return widMessage(lpDesc->uDeviceID, WIDM_GETPOS, lpDesc->dwInstance,
		      (DWORD)lpTime, (DWORD)uSize);
}

/**************************************************************************
 * 				waveInGetID			[WINMM.150]
 */
UINT WINAPI waveInGetID(HWAVEIN hWaveIn, UINT* lpuDeviceID)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "waveInGetID\n");
    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    *lpuDeviceID = lpDesc->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveInGetID			[MMSYSTEM.513]
 */
UINT16 WINAPI waveInGetID16(HWAVEIN16 hWaveIn, UINT16* lpuDeviceID)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "waveInGetID\n");
    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    *lpuDeviceID = lpDesc->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveInMessage 		[WINMM.153]
 */
DWORD WINAPI waveInMessage(HWAVEIN hWaveIn, UINT uMessage,
			   DWORD dwParam1, DWORD dwParam2)
{
    LPWAVEOPENDESC	lpDesc;
    
    FIXME(mmsys, "(%04X, %04X, %08lX, %08lX)\n", 
	  hWaveIn, uMessage, dwParam1, dwParam2);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case WIDM_OPEN:
	FIXME(mmsys, "cannot handle WIDM_OPEN, please report.\n");
	break;
    case WIDM_GETNUMDEVS:
    case WIDM_GETPOS:
    case WIDM_CLOSE:
    case WIDM_STOP:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_PREPARE:
    case WIDM_UNPREPARE:
    case WIDM_ADDBUFFER:
    case WIDM_PAUSE:
	/* no argument conversion needed */
	break;
    case WIDM_GETDEVCAPS:
	/*FIXME: ANSI/UNICODE */
	return waveInGetDevCapsA(hWaveIn, (LPWAVEINCAPSA)dwParam1, dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hWaveIn,uMessage, dwParam1, dwParam2);
	break;
    }
    return widMessage(lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				waveInMessage 		[MMSYSTEM.514]
 */
DWORD WINAPI waveInMessage16(HWAVEIN16 hWaveIn, UINT16 uMessage,
                             DWORD dwParam1, DWORD dwParam2)
{
    LPWAVEOPENDESC	lpDesc;
    
    FIXME(mmsys, "(%04X, %04X, %08lX, %08lX)\n", 
	  hWaveIn, uMessage, dwParam1, dwParam2);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case WIDM_OPEN:
	FIXME(mmsys,"cannot handle WIDM_OPEN, please report.\n");
	break;
    case WIDM_GETNUMDEVS:
    case WIDM_CLOSE:
    case WIDM_STOP:
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_PAUSE:
	/* no argument conversion needed */
	break;
    case WIDM_GETDEVCAPS:
	return waveInGetDevCaps16(hWaveIn, (LPWAVEINCAPS16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WIDM_GETPOS:
	return waveInGetPosition16(hWaveIn, (LPMMTIME16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WIDM_PREPARE:
	return waveInPrepareHeader16(hWaveIn, (LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WIDM_UNPREPARE:
	return waveInUnprepareHeader16(hWaveIn, (LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case WIDM_ADDBUFFER:
	return waveInAddBuffer16(hWaveIn, (LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hWaveIn,uMessage, dwParam1, dwParam2);
	break;
    }
    return widMessage(lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				DrvOpen	       		[MMSYSTEM.1100]
 */
HDRVR16 WINAPI DrvOpen(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
    TRACE(mmsys,"('%s','%s',%08lX);\n", lpDriverName, lpSectionName, lParam);

    return OpenDriver16(lpDriverName, lpSectionName, lParam);
}

/**************************************************************************
 * 				DrvClose       		[MMSYSTEM.1101]
 */
LRESULT WINAPI DrvClose(HDRVR16 hDrv, LPARAM lParam1, LPARAM lParam2)
{
    TRACE(mmsys, "(%04X, %08lX, %08lX);\n", hDrv, lParam1, lParam2);

    return CloseDriver16(hDrv, lParam1, lParam2);
}

/**************************************************************************
 * 				DrvSendMessage		[MMSYSTEM.1102]
 */
LRESULT WINAPI DrvSendMessage(HDRVR16 hDrv, WORD msg, LPARAM lParam1,
                              LPARAM lParam2)
{
    /* DWORD 	dwDriverID = 0; */

    FIXME(mmsys, "(%04X, %04X, %08lX, %08lX);\n", hDrv, msg, lParam1, lParam2);
    return MMSYSERR_NOTENABLED;
    /* FIXME: wrong ... */
    /* should known the mapping between hDrv and wDevIDs */
    /* MCICDAUDIO_DriverProc16(dwDriverID, hDrv, msg, lParam1, lParam2); */
}

/**************************************************************************
 * 				DrvGetModuleHandle	[MMSYSTEM.1103]
 */
HANDLE16 WINAPI DrvGetModuleHandle16(HDRVR16 hDrv)
{
    return GetDriverModuleHandle16(hDrv);
}

/**************************************************************************
 * 				DrvDefDriverProc	[MMSYSTEM.1104]
 */
LRESULT WINAPI DrvDefDriverProc(DWORD dwDriverID, HDRVR16 hDrv, WORD wMsg, 
                                DWORD dwParam1, DWORD dwParam2)
{
    /* FIXME : any mapping from 32 to 16 bit structure ? */
    return DefDriverProc16(dwDriverID, hDrv, wMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				DefDriverProc			  [WINMM.5]
 */
LRESULT WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR hDrv,
			     UINT Msg, LPARAM lParam1, LPARAM lParam2)
{
    switch (Msg) {
    case DRV_LOAD:
    case DRV_FREE:
    case DRV_ENABLE:
    case DRV_DISABLE:
        return 1;
    case DRV_INSTALL:
    case DRV_REMOVE:
        return DRV_SUCCESS;
    default:
        return 0;
    }
}

/**************************************************************************
 * 				mmThreadCreate		[MMSYSTEM.1120]
 */
LRESULT WINAPI mmThreadCreate16(LPVOID x1, LPWORD x2, DWORD x3, DWORD x4) 
{
    FIXME(mmsys, "(%p,%p,%08lx,%08lx): stub!\n",x1,x2,x3,x4);
    *x2 = 0xbabe;
    return 0;
}

/**************************************************************************
 * 				mmThreadSignal		[MMSYSTEM.1121]
 */
LRESULT WINAPI mmThreadSignal16(HANDLE16 hnd) 
{
    FIXME(mmsys,"(%04x): stub!\n", hnd);
    return 0;
}

/**************************************************************************
 * 				mmThreadBlock		[MMSYSTEM.1122]
 */
void	WINAPI mmThreadBlock16(HANDLE16 hnd) 
{
    FIXME(mmsys,"(%04x): stub!\n", hnd);
}

/**************************************************************************
 * 				mmThreadIsCurrent	[MMSYSTEM.1123]
 */
BOOL16	WINAPI mmThreadIsCurrent16(HANDLE16 hnd) 
{
    FIXME(mmsys,"(%04x): stub!\n", hnd);
    return FALSE;
}

/**************************************************************************
 * 				mmThreadGetTask		[MMSYSTEM.1125]
 */
LRESULT WINAPI mmThreadGetTask16(HANDLE16 hnd) 
{
    FIXME(mmsys,"(%04x): stub!\n", hnd);
    return GetCurrentTask();
}

/**************************************************************************
 * 				mmThreadIsValid		[MMSYSTEM.1124]
 */
BOOL16	WINAPI	mmThreadIsValid16(HANDLE16 handle)
{
    FIXME(mmsys, "(%04x): stub!\n", handle);
    return 0;
}

/**************************************************************************
 * 				mmTaskCreate		[MMSYSTEM.900]
 *
 * Creates a 16 bit MM task. It's entry point is lpFunc, and it should be
 * called upon creation with dwPmt as parameter.
 */
HINSTANCE16 WINAPI mmTaskCreate16(SEGPTR lphnd, HINSTANCE16 *hMmTask, DWORD dwPmt)
{
#if 1
    DWORD showCmd = 0x40002;
    LPSTR cmdline;
    WORD sel1, sel2;
    LOADPARAMS16 *lp;
    HINSTANCE16 ret, handle;
    
    TRACE(mmsys, "(%08lx,%p,%08lx);\n", lphnd, hMmTask, dwPmt);
    cmdline = (LPSTR)HeapAlloc(GetProcessHeap(), 0, 0x0d);
    cmdline[0] = 0x0d;
    (DWORD)cmdline[1] = (DWORD)lphnd;
    (DWORD)cmdline[5] = dwPmt;
    (DWORD)cmdline[9] = 0;
    
    sel1 = SELECTOR_AllocBlock(cmdline, 0x0d, SEGMENT_DATA, FALSE, FALSE);
    sel2 = SELECTOR_AllocBlock(&showCmd, sizeof(showCmd),
			       SEGMENT_DATA, FALSE, FALSE);
    
    lp = (LOADPARAMS16 *)HeapAlloc(GetProcessHeap(), 0, sizeof(LOADPARAMS16));
    lp->hEnvironment = 0;
    lp->cmdLine = PTR_SEG_OFF_TO_SEGPTR(sel1, 0);
    lp->showCmd = PTR_SEG_OFF_TO_SEGPTR(sel2, 0);
    lp->reserved = 0;
    
    ret = LoadModule16("c:\\windows\\system\\mmtask.tsk", lp);
    TRACE(mmsys, "MMtask's handle = 0x%04x\n", ret);
    if (ret < 32) {
	if (ret)
	    ret = 1;
	else
	    ret = 2;
	handle = 0;
    } else {
	handle = ret;
	ret = 0;
    }
    if (hMmTask)
	*hMmTask = handle;
    
    UnMapLS(PTR_SEG_OFF_TO_SEGPTR(sel2, 0));
    UnMapLS(PTR_SEG_OFF_TO_SEGPTR(sel1, 0));
    
    HeapFree(GetProcessHeap(), 0, lp);
    HeapFree(GetProcessHeap(), 0, cmdline);
    
    TRACE(mmsys, "=> 0x%04x/%d %04x\n", handle, ret, *(LPWORD)PTR_SEG_TO_LIN(lphnd));
    return ret;
#else
    TRACE(mmsys, "(%p,%p,%08lx);\n", lphnd, hMmTask, x2);
    if (hMmTask)
	*hMmTask = 0xfade;
    return 0;
#endif
}

/**************************************************************************
 * 				mmTaskBlock		[MMSYSTEM.902]
 */
void	WINAPI	mmTaskBlock16(HINSTANCE16 WINE_UNUSED hInst)
{
    MSG		msg;

    do {
	GetMessageA(&msg, 0, 0, 0);
	if (msg.hwnd) {
	    TranslateMessage(&msg);
	    DispatchMessageA(&msg);
	}
    } while (msg.message < 0x3A0);
}

/**************************************************************************
 * 				mmTaskSignal		[MMSYSTEM.903]
 */
LRESULT WINAPI mmTaskSignal16(HTASK16 ht) 
{
    TRACE(mmsys,"(%04x);\n", ht);
    return PostAppMessageA(ht, WM_USER, 0, 0);
}

/**************************************************************************
 * 				mmTaskYield16		[MMSYSTEM.905]
 */
void	WINAPI	mmTaskYield16(void)
{
    MSG		msg;

    if (PeekMessageA(&msg, 0, 0, 0, 0)) {
	Yield16();
    }
}

/**************************************************************************
 * 			mmShowMMCPLPropertySheet	[MMSYSTEM.1150]
 */
BOOL16	WINAPI	mmShowMMCPLPropertySheet16(WORD w1, WORD w2, WORD w3, WORD w4, WORD w5, WORD w6, WORD w7)
{
    FIXME(mmsys, "%04x %04x %04x %04x %04x %04x %04x\n", w1, w2, w3, w4, w5, w6, w7);
    return TRUE;
}
