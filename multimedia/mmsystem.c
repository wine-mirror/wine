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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "win.h"
#include "heap.h"
#include "ldt.h"
#include "user.h"
#include "driver.h"
#include "file.h"
#include "mmsystem.h"
#include "multimedia.h"
#include "debug.h"
#include "xmalloc.h"
#include "callback.h"
#include "module.h"
#include "selectors.h"

int	mciInstalledCount;
int	mciInstalledListLen;
LPSTR	lpmciInstallNames = NULL;

struct WINE_MCIDRIVER mciDrv[MAXMCIDRIVERS];

UINT16 WINAPI midiGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize);
static UINT16 waveGetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize);
LONG   WINAPI DrvDefDriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
			       DWORD dwParam1, DWORD dwParam2);

#define mciGetDrv(wDevID) 	(&mciDrv[MMSYSTEM_DevIDToIndex(wDevID)])
#define mciGetOpenDrv(wDevID)	(&(mciGetDrv(wDevID)->mop))

/* The wDevID's returned by wine were originally in the range 
 * 0 - (MAXMCIDRIVERS - 1) and used directly as array indices.
 * Unfortunately, ms-windows uses wDevID of zero to indicate
 * errors.  Now, multimedia drivers must pass the wDevID through
 * MMSYSTEM_DevIDToIndex to get an index in that range.  An
 * arbitrary value, MMSYSTEM_MAGIC is added to the wDevID seen
 * by the windows programs.
 */

#define MMSYSTEM_MAGIC 0x0F00

/**************************************************************************
 * 				MMSYSTEM_DevIDToIndex	[internal]
 */
int MMSYSTEM_DevIDToIndex(UINT16 wDevID) 
{
    return wDevID - MMSYSTEM_MAGIC;
}

/**************************************************************************
 * 				MMSYSTEM_FirstDevId	[internal]
 */
UINT16 MMSYSTEM_FirstDevID(void)
{
    return MMSYSTEM_MAGIC;
}

/**************************************************************************
 * 				MMSYSTEM_NextDevId	[internal]
 */
UINT16 MMSYSTEM_NextDevID(UINT16 wDevID) 
{
    return wDevID + 1;
}

/**************************************************************************
 * 				MMSYSTEM_DevIDValid	[internal]
 */
BOOL32 MMSYSTEM_DevIDValid(UINT16 wDevID) 
{
    return wDevID >= 0x0F00 && wDevID < (0x0F00 + MAXMCIDRIVERS);
}

/**************************************************************************
 * 				MMSYSTEM_WEP		[MMSYSTEM.1]
 */
int WINAPI MMSYSTEM_WEP(HINSTANCE16 hInstance, WORD wDataSeg,
                        WORD cbHeapSize, LPSTR lpCmdLine)
{
    FIXME(mmsys, "STUB: Unloading MMSystem DLL ... hInst=%04X \n", hInstance);
    return(TRUE);
}

static void MMSYSTEM_MMTIME32to16(LPMMTIME16 mmt16,LPMMTIME32 mmt32) 
{
    mmt16->wType = mmt32->wType;
    /* layout of rest is the same for 32/16 */
    memcpy(&(mmt32->u),&(mmt16->u),sizeof(mmt16->u));
}

static void MMSYSTEM_MMTIME16to32(LPMMTIME32 mmt32,LPMMTIME16 mmt16) 
{
    mmt32->wType = mmt16->wType;
    /* layout of rest is the same for 32/16,
     * Note: mmt16->u is 2 bytes smaller than mmt32->u
     */
    memcpy(&(mmt16->u),&(mmt32->u),sizeof(mmt16->u));
}

static HANDLE32		PlaySound_hThread = 0;
static HANDLE32		PlaySound_hPlayEvent = 0;
static HANDLE32		PlaySound_hReadyEvent = 0;
static HANDLE32		PlaySound_hMiddleEvent = 0;
static BOOL32		PlaySound_Result = FALSE;
static int		PlaySound_Stop = FALSE;
static int		PlaySound_Playing = FALSE;

static LPCSTR		PlaySound_pszSound = NULL;
static HMODULE32	PlaySound_hmod = 0;
static DWORD		PlaySound_fdwSound = 0;
static int		PlaySound_Loop = FALSE;
static int		PlaySound_SearchMode = 0; /* 1 - sndPlaySound search order
						     2 - PlaySound order */

static HMMIO16	get_mmioFromFile(LPCSTR lpszName)
{
    return mmioOpen16((LPSTR)lpszName, NULL,
		      MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
}

static HMMIO16 get_mmioFromProfile(UINT32 uFlags, LPCSTR lpszName) 
{
    char	str[128];
    LPSTR	ptr;
    HMMIO16  	hmmio;
    
    TRACE(mmsys, "searching in SystemSound List !\n");
    GetProfileString32A("Sounds", (LPSTR)lpszName, "", str, sizeof(str));
    if (strlen(str) == 0) {
	if (uFlags & SND_NODEFAULT) return 0;
	GetProfileString32A("Sounds", "Default", "", str, sizeof(str));
	if (strlen(str) == 0) return 0;
    }
    if ( (ptr = (LPSTR)strchr(str, ',')) != NULL) *ptr = '\0';
    hmmio = get_mmioFromFile(str);
    if (hmmio == 0) {
	WARN(mmsys, "can't find SystemSound='%s' !\n", str);
	return 0;
    }
    return hmmio;
}

static BOOL16 WINAPI proc_PlaySound(LPCSTR lpszSoundName, UINT32 uFlags)
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
	TRACE(mmsys, "Memory sound %p\n",lpszSoundName);
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
		  (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, ckMainRIFF.cksize);
	    
	    if ((ckMainRIFF.ckid == FOURCC_RIFF) &&
	    	(ckMainRIFF.fccType == mmioFOURCC('W', 'A', 'V', 'E'))) {
		MMCKINFO        mmckInfo;
		
		mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
		
		if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) == 0) {
		    PCMWAVEFORMAT           pcmWaveFormat;
		    
		    TRACE(mmsys, "Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
			  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);
		    
		    if (mmioRead32(hmmio,(HPSTR)&pcmWaveFormat,
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
			    
			    TRACE(mmsys, "Chunk Found \
ckid=%.4s fccType=%.4s cksize=%08lX \n", (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);
			    
			    pcmWaveFormat.wf.nAvgBytesPerSec = pcmWaveFormat.wf.nSamplesPerSec * 
				pcmWaveFormat.wf.nBlockAlign;
			    waveDesc.hWave    = 0;
			    waveDesc.lpFormat = (LPWAVEFORMAT)&pcmWaveFormat;
			    
			    dwRet = wodMessage( 0, WODM_OPEN, 0, (DWORD)&waveDesc, CALLBACK_NULL);
			    if (dwRet == MMSYSERR_NOERROR) {
				WAVEHDR		waveHdr;
				HGLOBAL16	hData;
				INT32		count, bufsize, left = mmckInfo.cksize;
				
				bufsize = 64000;
				hData = GlobalAlloc16(GMEM_MOVEABLE, bufsize);
				waveHdr.lpData = (LPSTR)GlobalLock16(hData);
				waveHdr.dwBufferLength = bufsize;
				waveHdr.dwUser = 0L;
				waveHdr.dwFlags = 0L;
				waveHdr.dwLoops = 0L;
				
				dwRet = wodMessage(0,WODM_PREPARE,0,(DWORD)&waveHdr,sizeof(WAVEHDR));
				if (dwRet == MMSYSERR_NOERROR) {
				    while (left) {
                                        if (PlaySound_Stop) {
					    PlaySound_Stop = FALSE;
					    PlaySound_Loop = FALSE;
					    break;
					}
					if (bufsize > left) bufsize = left;
					count = mmioRead32(hmmio,waveHdr.lpData,bufsize);
					if (count < 1) break;
					left -= count;
					waveHdr.dwBufferLength = count;
				/*	waveHdr.dwBytesRecorded = count; */
					/* FIXME: doesn't expect async ops */ 
					wodMessage( 0, WODM_WRITE, 0, (DWORD)&waveHdr, sizeof(WAVEHDR));
				    }
				    wodMessage( 0, WODM_UNPREPARE, 0, (DWORD)&waveHdr, sizeof(WAVEHDR));
				    wodMessage( 0, WODM_CLOSE, 0, 0L, 0L);
				    
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
    
    if (hmmio != 0) mmioClose32(hmmio, 0);
    return bRet;
}

static DWORD WINAPI PlaySound_Thread(LPVOID arg) 
{
    DWORD     res;
    
    for (;;) {
	PlaySound_Playing = FALSE;
	SetEvent(PlaySound_hReadyEvent);
	res = WaitForSingleObject(PlaySound_hPlayEvent, INFINITE32);
	ResetEvent(PlaySound_hReadyEvent);
	SetEvent(PlaySound_hMiddleEvent);
	if (res == WAIT_FAILED) ExitThread(2);
	if (res != WAIT_OBJECT_0) continue;
	PlaySound_Playing = TRUE;
	
	if ((PlaySound_fdwSound & SND_RESOURCE) == SND_RESOURCE) {
	    HRSRC32   hRES;
	    HGLOBAL32 hGLOB;
	    void      *ptr;

	    if ((hRES = FindResource32A(PlaySound_hmod, PlaySound_pszSound, "WAVE")) == 0) {
		PlaySound_Result = FALSE;
		continue;
	    }
	    if ((hGLOB = LoadResource32(PlaySound_hmod, hRES)) == 0) {
		PlaySound_Result = FALSE;
		continue;
	    }
	    if ((ptr = LockResource32(hGLOB)) == NULL) {
		FreeResource32(hGLOB);
		PlaySound_Result = FALSE;
		continue;
	    }
	    PlaySound_Result = proc_PlaySound(ptr, 
					      ((UINT16)PlaySound_fdwSound ^ SND_RESOURCE) | SND_MEMORY);
	    FreeResource32(hGLOB);
	    continue;
	}
	PlaySound_Result=proc_PlaySound(PlaySound_pszSound, (UINT16)PlaySound_fdwSound);
    }
}

/**************************************************************************
 * 				PlaySoundA		[WINMM.1]
 */
BOOL32 WINAPI PlaySound32A(LPCSTR pszSound, HMODULE32 hmod, DWORD fdwSound)
{
    static LPSTR StrDup = NULL;
    
    TRACE(mmsys, "pszSound='%p' hmod=%04X fdwSound=%08lX\n",
	  pszSound, hmod, fdwSound);
    
    if (PlaySound_hThread == 0) { /* This is the first time they called us */
	DWORD	id;
	if ((PlaySound_hReadyEvent = CreateEvent32A(NULL, TRUE, FALSE, NULL)) == 0)
	    return FALSE;
	if ((PlaySound_hMiddleEvent = CreateEvent32A(NULL, FALSE, FALSE, NULL)) == 0)
	    return FALSE;
	if ((PlaySound_hPlayEvent = CreateEvent32A(NULL, FALSE, FALSE, NULL)) == 0)
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
	if (WaitForSingleObject(PlaySound_hMiddleEvent, INFINITE32) != WAIT_OBJECT_0) 
	    return FALSE;
	if (WaitForSingleObject(PlaySound_hReadyEvent, INFINITE32) != WAIT_OBJECT_0) 
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
	    StrDup = HEAP_strdupA(GetProcessHeap(),0,pszSound);
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
BOOL32 WINAPI PlaySound32W(LPCWSTR pszSound, HMODULE32 hmod, DWORD fdwSound)
{
    LPSTR 	pszSoundA;
    BOOL32	bSound;
    
    if (!((fdwSound & SND_MEMORY) || ((fdwSound & SND_RESOURCE) && 
				      !((DWORD)pszSound >> 16)) || !pszSound)) {
	pszSoundA = HEAP_strdupWtoA(GetProcessHeap(),0,pszSound);
	bSound = PlaySound32A(pszSoundA, hmod, fdwSound);
	HeapFree(GetProcessHeap(),0,pszSoundA);
    } else  
	bSound = PlaySound32A((LPCSTR)pszSound, hmod, fdwSound);
    
    return bSound;
}

/**************************************************************************
 * 				sndPlaySound		[MMSYSTEM.2]
 */
BOOL16 WINAPI sndPlaySound(LPCSTR lpszSoundName, UINT16 uFlags)
{
    PlaySound_SearchMode = 1;
    return PlaySound32A(lpszSoundName, 0, uFlags);
}

/**************************************************************************
 * 				mmsystemGetVersion	[WINMM.134]
 */
UINT32 WINAPI mmsystemGetVersion32()
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
 * 				DriverProc	[MMSYSTEM.6]
 */
LRESULT WINAPI DriverProc(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
                          DWORD dwParam1, DWORD dwParam2)
{
    return DrvDefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				DriverCallback	[MMSYSTEM.31]
 */
BOOL16 WINAPI DriverCallback(DWORD dwCallBack, UINT16 uFlags, HANDLE16 hDev, 
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
	      dwCallBack,hDev);
	if (!IsWindow32(dwCallBack) || USER_HEAP_LIN_ADDR(hDev) == NULL)
	    return FALSE;
	
	PostMessage16((HWND16)dwCallBack, wMsg, hDev, dwParam1);
	break;
    case DCB_TASK:
	TRACE(mmsys, "CALLBACK_TASK !\n");
	return FALSE;
    case DCB_FUNCTION:
	TRACE(mmsys, "CALLBACK_FUNCTION !\n");
	Callbacks->CallDriverCallback( (FARPROC16)dwCallBack,
				       hDev, wMsg, dwUser,
				       dwParam1, dwParam2 );
	break;
    case DCB_FUNC32:
	TRACE(mmsys, "CALLBACK_FUNCTION !\n");
	((LPDRVCALLBACK32)dwCallBack)( hDev, wMsg, dwUser,
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
static UINT32 _get_mixerID_from_handle(HMIXEROBJ32 hmix,DWORD dwFlags) {
    /* FIXME: Check dwFlags for MIXER_OBJECTF_xxxx entries and modify hmix 
     * accordingly. For now we always use mixerdevice 0. 
     */
    return 0;
}
/**************************************************************************
 * 				mixerGetNumDevs		[WINMM.108]
 */
UINT32 WINAPI mixerGetNumDevs32() 
{
    return mixerGetNumDevs16();
}

/**************************************************************************
 * 				mixerGetNumDevs	
 */
UINT16 WINAPI mixerGetNumDevs16() 
{
    UINT16	count;
    
    count = mixMessage(0,MXDM_GETNUMDEVS,0L,0L,0L);
    TRACE(mmaux,"mixerGetNumDevs returns %d\n",count);
    return count;
}

/**************************************************************************
 * 				mixerGetDevCapsW		[WINMM.102]
 */
UINT32 WINAPI mixerGetDevCaps32W(UINT32 devid,LPMIXERCAPS32W mixcaps,UINT32 size) 
{
    MIXERCAPS16	mic16;
    UINT32	ret = mixerGetDevCaps16(devid,&mic16,sizeof(mic16));
    
    mixcaps->wMid = mic16.wMid;
    mixcaps->wPid = mic16.wPid;
    mixcaps->vDriverVersion = mic16.vDriverVersion;
    lstrcpyAtoW(mixcaps->szPname,mic16.szPname);
    mixcaps->fdwSupport = mic16.fdwSupport;
    mixcaps->cDestinations = mic16.cDestinations;
    return ret;
}
/**************************************************************************
 * 				mixerGetDevCaps		[WINMM.101]
 */
UINT32 WINAPI mixerGetDevCaps32A(UINT32 devid,LPMIXERCAPS32A mixcaps,UINT32 size) 
{
    MIXERCAPS16	mic16;
    UINT32	ret = mixerGetDevCaps16(devid,&mic16,sizeof(mic16));
    
    mixcaps->wMid = mic16.wMid;
    mixcaps->wPid = mic16.wPid;
    mixcaps->vDriverVersion = mic16.vDriverVersion;
    strcpy(mixcaps->szPname,mic16.szPname);
    mixcaps->fdwSupport = mic16.fdwSupport;
    mixcaps->cDestinations = mic16.cDestinations;
    return ret;
}

/**************************************************************************
 * 				mixerGetDevCaps	
 */
UINT16 WINAPI mixerGetDevCaps16(UINT16 devid,LPMIXERCAPS16 mixcaps,UINT16 size) 
{
    FIXME(mmsys,"should this be a fixme?\n");
    return mixMessage(devid,MXDM_GETDEVCAPS,0L,(DWORD)mixcaps,(DWORD)size);
}

/**************************************************************************
 * 				mixerOpen		[WINMM.110]
 */
UINT32 WINAPI mixerOpen32(LPHMIXER32 lphmix,UINT32 uDeviceID,DWORD dwCallback,
			  DWORD dwInstance,DWORD fdwOpen) 
{
    HMIXER16	hmix16;
    UINT32	ret;
    
    FIXME(mmsys,"(%p,%d,%08lx,%08lx,%08lx): semi stub?\n",
	  lphmix,uDeviceID,dwCallback,dwInstance,fdwOpen);
    ret = mixerOpen16(&hmix16,uDeviceID,dwCallback,dwInstance,fdwOpen);
    if (lphmix) *lphmix = hmix16;
    return ret;
}

/**************************************************************************
 * 				mixerOpen
 */
UINT16 WINAPI mixerOpen16(LPHMIXER16 lphmix,UINT16 uDeviceID,DWORD dwCallback,
			  DWORD dwInstance,DWORD fdwOpen) 
{
    HMIXER16		hmix;
    LPMIXEROPENDESC	lpmod;
    BOOL32		mapperflag = (uDeviceID==0);
    DWORD		dwRet;
    
    TRACE(mmsys,"(%p,%d,%08lx,%08lx,%08lx)\n",
	  lphmix,uDeviceID,dwCallback,dwInstance,fdwOpen);
    hmix = USER_HEAP_ALLOC(sizeof(MIXEROPENDESC));
    if (lphmix) *lphmix = hmix;
    lpmod = (LPMIXEROPENDESC)USER_HEAP_LIN_ADDR(hmix);
    lpmod->hmx = hmix;
    lpmod->dwCallback = dwCallback;
    lpmod->dwInstance = dwInstance;
    if (uDeviceID >= MAXMIXERDRIVERS)
	uDeviceID = 0;
    while (uDeviceID < MAXMIXERDRIVERS) {
	dwRet=mixMessage(uDeviceID,MXDM_OPEN,dwInstance,(DWORD)lpmod,fdwOpen);
	if (dwRet == MMSYSERR_NOERROR) break;
	if (!mapperflag) break;
	uDeviceID++;
    }
    lpmod->uDeviceID = uDeviceID;
    return dwRet;
}

/**************************************************************************
 * 				mixerClose		[WINMM.98]
 */
UINT32 WINAPI mixerClose32(HMIXER32 hmix) 
{
    return mixerClose16(hmix);
}

/**************************************************************************
 * 				mixerClose
 */
UINT16 WINAPI mixerClose16(HMIXER16 hmix) 
{
    LPMIXEROPENDESC	lpmod;
    
    FIXME(mmsys,"(%04x): semi-stub?\n",hmix);
    lpmod = (LPMIXEROPENDESC)USER_HEAP_LIN_ADDR(hmix);
    return mixMessage(lpmod->uDeviceID,MXDM_CLOSE,lpmod->dwInstance,0L,0L);
}

/**************************************************************************
 * 				mixerGetID		[WINMM.103]
 */
UINT32 WINAPI mixerGetID32(HMIXEROBJ32 hmix,LPUINT32 lpid,DWORD fdwID) 
{
    UINT16	xid;    
    UINT32	ret = mixerGetID16(hmix,&xid,fdwID);

    if (*lpid) *lpid = xid;
    return ret;
}

/**************************************************************************
 * 				mixerGetID
 */
UINT16 WINAPI mixerGetID16(HMIXEROBJ16 hmix,LPUINT16 lpid,DWORD fdwID) 
{
    FIXME(mmsys,"(%04x): semi-stub\n",hmix);
    return _get_mixerID_from_handle(hmix,fdwID);
}

/**************************************************************************
 * 				mixerGetControlDetailsA	[WINMM.99]
 */
UINT32 WINAPI mixerGetControlDetails32A(HMIXEROBJ32 hmix,LPMIXERCONTROLDETAILS32 lpmcd,DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpmcd,fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetControlDetailsW	[WINMM.100]
 */
UINT32 WINAPI mixerGetControlDetails32W(HMIXEROBJ32 hmix,LPMIXERCONTROLDETAILS32 lpmcd,DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",	hmix,lpmcd,fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetControlDetails	[MMSYSTEM.808]
 */
UINT16 WINAPI mixerGetControlDetails16(HMIXEROBJ16 hmix,LPMIXERCONTROLDETAILS16 lpmcd,DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpmcd,fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetLineControlsA	[WINMM.104]
 */
UINT32 WINAPI mixerGetLineControls32A(HMIXEROBJ32 hmix,LPMIXERLINECONTROLS32A lpmlc,DWORD fdwControls) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpmlc,fdwControls);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetLineControlsW	[WINMM.105]
 */
UINT32 WINAPI mixerGetLineControls32W(HMIXEROBJ32 hmix,LPMIXERLINECONTROLS32W lpmlc,DWORD fdwControls) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpmlc,fdwControls);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetLineControls	[MMSYSTEM.807]
 */
UINT16 WINAPI mixerGetLineControls16(HMIXEROBJ16 hmix,LPMIXERLINECONTROLS16 lpmlc,DWORD fdwControls) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpmlc,fdwControls);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerGetLineInfoA	[WINMM.106]
 */
UINT32 WINAPI mixerGetLineInfo32A(HMIXEROBJ32 hmix,LPMIXERLINE32A lpml,DWORD fdwInfo) 
{
    MIXERLINE16		ml16;
    UINT32		ret;
    
    ml16.dwDestination = lpml->dwDestination;
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpml,fdwInfo);
    ret = mixerGetLineInfo16(hmix,&ml16,fdwInfo);
    lpml->cbStruct = sizeof(*lpml);
    lpml->dwSource = ml16.dwSource;
    lpml->dwLineID = ml16.dwLineID;
    lpml->fdwLine = ml16.fdwLine;
    lpml->dwUser = ml16.dwUser;
    lpml->dwComponentType = ml16.dwComponentType;
    lpml->cChannels = ml16.cChannels;
    lpml->cConnections = ml16.cConnections;
    lpml->cControls = ml16.cControls;
    strcpy(lpml->szShortName,ml16.szShortName);
    strcpy(lpml->szName,ml16.szName);
    lpml->Target.dwType = ml16.Target.dwType;
    lpml->Target.dwDeviceID = ml16.Target.dwDeviceID;
    lpml->Target.wMid = ml16.Target.wMid;
    lpml->Target.wPid = ml16.Target.wPid;
    lpml->Target.vDriverVersion = ml16.Target.vDriverVersion;
    strcpy(lpml->Target.szPname,ml16.Target.szPname);
    return ret;
}

/**************************************************************************
 * 				mixerGetLineInfoW	[WINMM.107]
 */
UINT32 WINAPI mixerGetLineInfo32W(HMIXEROBJ32 hmix,LPMIXERLINE32W lpml,DWORD fdwInfo) 
{
    MIXERLINE16		ml16;
    UINT32		ret;
    
    ml16.dwDestination = lpml->dwDestination;
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpml,fdwInfo);
    ret = mixerGetLineInfo16(hmix,&ml16,fdwInfo);
    lpml->cbStruct = sizeof(*lpml);
    lpml->dwSource = ml16.dwSource;
    lpml->dwLineID = ml16.dwLineID;
    lpml->fdwLine = ml16.fdwLine;
    lpml->dwUser = ml16.dwUser;
    lpml->dwComponentType = ml16.dwComponentType;
    lpml->cChannels = ml16.cChannels;
    lpml->cConnections = ml16.cConnections;
    lpml->cControls = ml16.cControls;
    lstrcpyAtoW(lpml->szShortName,ml16.szShortName);
    lstrcpyAtoW(lpml->szName,ml16.szName);
    lpml->Target.dwType = ml16.Target.dwType;
    lpml->Target.dwDeviceID = ml16.Target.dwDeviceID;
    lpml->Target.wMid = ml16.Target.wMid;
    lpml->Target.wPid = ml16.Target.wPid;
    lpml->Target.vDriverVersion = ml16.Target.vDriverVersion;
    /*lstrcpyAtoW(lpml->Target.szPname,ml16.Target.szPname);*/
    return ret;
}

/**************************************************************************
 * 				mixerGetLineInfo	[MMSYSTEM.805]
 */
UINT16 WINAPI mixerGetLineInfo16(HMIXEROBJ16 hmix,LPMIXERLINE16 lpml,DWORD fdwInfo) 
{
    UINT16 devid =  _get_mixerID_from_handle(hmix,fdwInfo);
    
    FIXME(mmsys,"(%04x,%p[line %08lx],%08lx) - semi-stub?\n",
	  hmix,lpml,lpml->dwDestination,fdwInfo);
    return mixMessage(devid,MXDM_GETLINEINFO,0,(DWORD)lpml,fdwInfo);
}

/**************************************************************************
 * 				mixerSetControlDetails	[WINMM.111]
 */
UINT32 WINAPI mixerSetControlDetails32(HMIXEROBJ32 hmix,LPMIXERCONTROLDETAILS32 lpmcd,DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpmcd,fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerSetControlDetails	[MMSYSTEM.809]
 */
UINT16 WINAPI mixerSetControlDetails16(HMIXEROBJ16 hmix,LPMIXERCONTROLDETAILS16 lpmcd,DWORD fdwDetails) 
{
    FIXME(mmsys,"(%04x,%p,%08lx): stub!\n",hmix,lpmcd,fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerMessage		[WINMM.109]
 */
UINT32 WINAPI mixerMessage32(HMIXER32 hmix,UINT32 uMsg,DWORD dwParam1,DWORD dwParam2) 
{
    LPMIXEROPENDESC	lpmod;
    UINT16		uDeviceID;
    
    lpmod = (LPMIXEROPENDESC)USER_HEAP_LIN_ADDR(hmix);
    if (lpmod)
	uDeviceID = lpmod->uDeviceID;
    else
	uDeviceID = 0;
    FIXME(mmsys,"(%04lx,%d,%08lx,%08lx): semi-stub?\n",
	  (DWORD)hmix,uMsg,dwParam1,dwParam2);
    return mixMessage(uDeviceID,uMsg,0L,dwParam1,dwParam2);
}

/**************************************************************************
 * 				mixerMessage		[MMSYSTEM.804]
 */
UINT16 WINAPI mixerMessage16(HMIXER16 hmix,UINT16 uMsg,DWORD dwParam1,DWORD dwParam2) 
{
    LPMIXEROPENDESC	lpmod;
    UINT16		uDeviceID;
    
    lpmod = (LPMIXEROPENDESC)USER_HEAP_LIN_ADDR(hmix);
    if (lpmod)
	uDeviceID = lpmod->uDeviceID;
    else
	uDeviceID = 0;
    FIXME(mmsys,"(%04x,%d,%08lx,%08lx) - semi-stub?\n",
	  hmix,uMsg,dwParam1,dwParam2);
    return mixMessage(uDeviceID,uMsg,0L,dwParam1,dwParam2);
}

/**************************************************************************
 * 				auxGetNumDevs		[WINMM.22]
 */
UINT32 WINAPI auxGetNumDevs32()
{
    return auxGetNumDevs16();
}

/**************************************************************************
 * 				auxGetNumDevs		[MMSYSTEM.350]
 */
UINT16 WINAPI auxGetNumDevs16()
{
    UINT16	count = 0;
    TRACE(mmsys, "auxGetNumDevs !\n");
    count += auxMessage(0, AUXDM_GETNUMDEVS, 0L, 0L, 0L);
    TRACE(mmsys, "auxGetNumDevs return %u \n", count);
    return count;
}

/**************************************************************************
 * 				auxGetDevCaps		[WINMM.20]
 */
UINT32 WINAPI auxGetDevCaps32W(UINT32 uDeviceID,LPAUXCAPS32W lpCaps,UINT32 uSize)
{
    AUXCAPS16	ac16;
    UINT32	ret = auxGetDevCaps16(uDeviceID,&ac16,sizeof(ac16));
    
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
UINT32 WINAPI auxGetDevCaps32A(UINT32 uDeviceID,LPAUXCAPS32A lpCaps,UINT32 uSize)
{
    AUXCAPS16	ac16;
    UINT32	ret = auxGetDevCaps16(uDeviceID,&ac16,sizeof(ac16));
    
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
UINT16 WINAPI auxGetDevCaps16(UINT16 uDeviceID,LPAUXCAPS16 lpCaps, UINT16 uSize)
{
    TRACE(mmsys, "(%04X, %p, %d) !\n", 
	  uDeviceID, lpCaps, uSize);
    return auxMessage(uDeviceID, AUXDM_GETDEVCAPS,
		      0L, (DWORD)lpCaps, (DWORD)uSize);
}

/**************************************************************************
 * 				auxGetVolume		[WINM.23]
 */
UINT32 WINAPI auxGetVolume32(UINT32 uDeviceID, DWORD * lpdwVolume)
{
    return auxGetVolume16(uDeviceID,lpdwVolume);
}

/**************************************************************************
 * 				auxGetVolume		[MMSYSTEM.352]
 */
UINT16 WINAPI auxGetVolume16(UINT16 uDeviceID, DWORD * lpdwVolume)
{
    TRACE(mmsys, "(%04X, %p) !\n", uDeviceID, lpdwVolume);
    return auxMessage(uDeviceID, AUXDM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
}

/**************************************************************************
 * 				auxSetVolume		[WINMM.25]
 */
UINT32 WINAPI auxSetVolume32(UINT32 uDeviceID, DWORD dwVolume)
{
    return auxSetVolume16(uDeviceID,dwVolume);
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
DWORD WINAPI auxOutMessage32(UINT32 uDeviceID,UINT32 uMessage,DWORD dw1,DWORD dw2)
{
    switch (uMessage) {
    case AUXDM_GETNUMDEVS:
    case AUXDM_GETVOLUME:
    case AUXDM_SETVOLUME:
	/* no argument conversion needed */
	break;
    case AUXDM_GETDEVCAPS:
	return auxGetDevCaps32A(uDeviceID,(LPAUXCAPS32A)dw1,dw2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    uDeviceID,uMessage,dw1,dw2);
	break;
    }
    return auxMessage(uDeviceID,uMessage,0L,dw1,dw2);
}

/**************************************************************************
 * 				auxOutMessage		[MMSYSTEM.354]
 */
DWORD WINAPI auxOutMessage16(UINT16 uDeviceID, UINT16 uMessage, DWORD dw1, DWORD dw2)
{
    TRACE(mmsys, "(%04X, %04X, %08lX, %08lX)\n", 
	  uDeviceID, uMessage, dw1, dw2);
    switch (uMessage) {
    case AUXDM_GETNUMDEVS:
    case AUXDM_SETVOLUME:
	/* no argument conversion needed */
	break;
    case AUXDM_GETVOLUME:
	return auxGetVolume16(uDeviceID,(LPDWORD)PTR_SEG_TO_LIN(dw1));
    case AUXDM_GETDEVCAPS:
	return auxGetDevCaps16(uDeviceID,(LPAUXCAPS16)PTR_SEG_TO_LIN(dw1),dw2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    uDeviceID,uMessage,dw1,dw2);
	break;
    }
    return auxMessage(uDeviceID, uMessage, 0L, dw1, dw2);
}

/**************************************************************************
 * 				mciGetErrorStringW		[WINMM.46]
 */
BOOL32 WINAPI mciGetErrorString32W(DWORD wError,LPWSTR lpstrBuffer,UINT32 uLength)
{
    LPSTR	bufstr = HeapAlloc(GetProcessHeap(),0,uLength);
    BOOL32	ret = mciGetErrorString32A(wError,bufstr,uLength);
    
    lstrcpyAtoW(lpstrBuffer,bufstr);
    HeapFree(GetProcessHeap(),0,bufstr);
    return ret;
}

/**************************************************************************
 * 				mciGetErrorStringA		[WINMM.45]
 */
BOOL32 WINAPI mciGetErrorString32A(DWORD wError,LPSTR lpstrBuffer,UINT32 uLength)
{
    return mciGetErrorString16(wError,lpstrBuffer,uLength);
}

/**************************************************************************
 * 				mciGetErrorString		[MMSYSTEM.706]
 */
BOOL16 WINAPI mciGetErrorString16(DWORD wError,LPSTR lpstrBuffer,UINT16 uLength)
{
    LPSTR	msgptr;
    TRACE(mmsys, "(%08lX, %p, %d);\n", 
	  wError, lpstrBuffer, uLength);
    if ((lpstrBuffer == NULL) || (uLength < 1)) return(FALSE);
    lpstrBuffer[0] = '\0';
    switch (wError) {
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
	break;
    }
    lstrcpyn32A(lpstrBuffer, msgptr, uLength);
    TRACE(mmsys, "msg = %s;\n", msgptr);
    return TRUE;
}


/**************************************************************************
 * 				mciDriverNotify			[MMSYSTEM.711]
 */
BOOL16 WINAPI mciDriverNotify(HWND16 hWndCallBack, UINT16 wDevID, UINT16 wStatus)
{
    TRACE(mmsys, "(%04X, %u, %04X)\n", hWndCallBack, wDevID, wStatus);
    if (!IsWindow32(hWndCallBack)) return FALSE;
    TRACE(mmsys, "before PostMessage\n");
    PostMessage16( hWndCallBack, MM_MCINOTIFY, wStatus, 
		   MAKELONG(wDevID, 0));
    return TRUE;
}

/**************************************************************************
 * 			mciOpen16				[internal]
 */
static DWORD mciOpen16(DWORD dwParam, LPMCI_OPEN_PARMS16 lp16Parms)
{
    char		str[128];
    LPMCI_OPEN_PARMS16	lpParms;
    UINT16		uDevTyp = 0;
    UINT16		wDevID = MMSYSTEM_FirstDevID();
    DWORD 		dwret;
    
    lpParms = PTR_SEG_TO_LIN(lp16Parms);
    TRACE(mmsys, "(%08lX, %p (%p))\n", dwParam, lp16Parms, lpParms);
    if (lp16Parms == NULL) return MCIERR_INTERNAL;
    
    while (mciGetDrv(wDevID)->modp.wType != 0) {
	wDevID = MMSYSTEM_NextDevID(wDevID);
	if (!MMSYSTEM_DevIDValid(wDevID)) {
	    TRACE(mmsys, "MAXMCIDRIVERS reached !\n");
	    return MCIERR_INTERNAL;
	}
    }
    TRACE(mmsys, "wDevID=%04X \n", wDevID);
    memcpy(mciGetOpenDrv(wDevID), lpParms, sizeof(*lpParms));
    
    if (dwParam & MCI_OPEN_ELEMENT) {
	char	*s,*t;
	
	TRACE(mmsys,"lpstrElementName='%s'\n",
	      (char*)PTR_SEG_TO_LIN(lpParms->lpstrElementName)
	      );
	s = (char*)PTR_SEG_TO_LIN(lpParms->lpstrElementName);
	t = strrchr(s,'.');
	if (t) {
	    GetProfileString32A("mci extensions",t+1,"*",str,sizeof(str));
	    CharUpper32A(str);
	    TRACE(mmsys, "str = %s \n", str);
	    if (strcmp(str, "CDAUDIO") == 0) {
		uDevTyp = MCI_DEVTYPE_CD_AUDIO;
	    } else if (strcmp(str, "WAVEAUDIO") == 0) {
		uDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
	    } else if (strcmp(str, "SEQUENCER") == 0)	{
		uDevTyp = MCI_DEVTYPE_SEQUENCER;
	    } else if (strcmp(str, "ANIMATION1") == 0) {
		uDevTyp = MCI_DEVTYPE_ANIMATION;
	    } else if (strcmp(str, "AVIVIDEO") == 0) {
		uDevTyp = MCI_DEVTYPE_DIGITAL_VIDEO;
	    } else if (strcmp(str,"*") == 0) {
		TRACE(mmsys,"No [mci extensions] entry for %s found.\n",t);
		return MCIERR_EXTENSION_NOT_FOUND;
#if testing16
	    } else  {
		HDRVR16 hdrv = OpenDriver(str,"mci",NULL);
		if (hdrv) {
		    HMODULE16	hmod;
		    
		    hmod = GetDriverModuleHandle(hdrv);
		    mciGetDrv(wDevID)->hDrv = hdrv;
		    mciGetDrv(wDevID)->driverProc = GetProcAddress16(hmod,SEGPTR_GET(SEGPTR_STRDUP("DRIVERPROC")));
		    uDevTyp = MCI_DEVTYPE_OTHER;
		} else {
		    FIXME(mmsys, "[mci extensions] entry %s for %s not supported.\n",str,t);
		    return MCIERR_DEVICE_NOT_INSTALLED;
		}
#endif
	    }
	} else
	    return MCIERR_EXTENSION_NOT_FOUND;
    }
    
    if (dwParam & MCI_OPEN_ALIAS) {
	TRACE(mmsys, "Alias='%s' !\n",
	      (char*)PTR_SEG_TO_LIN(lpParms->lpstrAlias));
	mciGetOpenDrv(wDevID)->lpstrAlias = strdup(PTR_SEG_TO_LIN(lpParms->lpstrAlias));
	/* mplayer does allocate alias to CDAUDIO */
    }
    if (dwParam & MCI_OPEN_TYPE) {
	if (dwParam & MCI_OPEN_TYPE_ID) {
	    TRACE(mmsys, "Dev=%08lx!\n", (DWORD)PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
	    uDevTyp = LOWORD((DWORD)lpParms->lpstrDeviceType);
	    mciGetOpenDrv(wDevID)->lpstrDeviceType=strdup(PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
	} else {
	    if (lpParms->lpstrDeviceType == NULL) return MCIERR_INTERNAL;
	    TRACE(mmsys, "Dev='%s' !\n",
		  (char*)PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
	    mciGetOpenDrv(wDevID)->lpstrDeviceType=strdup(PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
	    strcpy(str, PTR_SEG_TO_LIN(lpParms->lpstrDeviceType));
	    CharUpper32A(str);
	    if (strcmp(str, "CDAUDIO") == 0) {
		uDevTyp = MCI_DEVTYPE_CD_AUDIO;
	    } else if (strcmp(str, "WAVEAUDIO") == 0) {
		uDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
	    } else if (strcmp(str, "SEQUENCER") == 0)	{
		uDevTyp = MCI_DEVTYPE_SEQUENCER;
	    } else if (strcmp(str, "ANIMATION1") == 0) {
		uDevTyp = MCI_DEVTYPE_ANIMATION;
	    } else if (strcmp(str, "AVIVIDEO") == 0) {
		uDevTyp = MCI_DEVTYPE_DIGITAL_VIDEO;
	    } else {
#if testing16
		HDRVR16 hdrv;
		TRACE(mmsys,"trying to load driver...\n");
		hdrv = OpenDriver(str,"mci",NULL);
		if (hdrv) {
		    HMODULE16	hmod;
		    
		    hmod = GetDriverModuleHandle(hdrv);
		    mciGetDrv(wDevID)->hDrv = hdrv;
		    mciGetDrv(wDevID)->driverProc = GetProcAddress16(hmod,SEGPTR_GET(SEGPTR_STRDUP("DRIVERPROC")));
		    uDevTyp = MCI_DEVTYPE_OTHER;
		} else
#endif
		    return MCIERR_DEVICE_NOT_INSTALLED;
	    }
	}
    }
    mciGetDrv(wDevID)->modp.wType = uDevTyp;
    mciGetDrv(wDevID)->modp.wDeviceID = 0;  /* FIXME? for multiple devices */
    lpParms->wDeviceID = wDevID;
    TRACE(mmsys, "mcidev=%d, uDevTyp=%04X wDeviceID=%04X !\n", 
	  wDevID, uDevTyp, lpParms->wDeviceID);
    switch (uDevTyp) {
    case MCI_DEVTYPE_CD_AUDIO:
	dwret = CDAUDIO_DriverProc16(0, 0, MCI_OPEN_DRIVER, dwParam, (DWORD)lp16Parms);
	break;
    case MCI_DEVTYPE_WAVEFORM_AUDIO:
	dwret =  WAVE_DriverProc16(0, 0, MCI_OPEN_DRIVER, dwParam, (DWORD)lp16Parms);
	break;
    case MCI_DEVTYPE_SEQUENCER:
	dwret = MIDI_DriverProc16(0, 0, MCI_OPEN_DRIVER, dwParam, (DWORD)lp16Parms);
	break;
    case MCI_DEVTYPE_ANIMATION:
	dwret = ANIM_DriverProc16(0, 0, MCI_OPEN_DRIVER, dwParam, (DWORD)lp16Parms);
	break;
    case MCI_DEVTYPE_DIGITAL_VIDEO:
	TRACE(mmsys, "No DIGITAL_VIDEO yet !\n");
	return MCIERR_DEVICE_NOT_INSTALLED;
    default:
#if testing16
	dwret = Callbacks->CallDriverProc(mciGetDrv(wDevID)->driverProc,0,mciGetDrv(wDevID)->hDrv,MCI_OPEN_DRIVER,dwParam,(DWORD)lp16Parms);
	WARN(mmsys, "Invalid Device Name '%08lx' !\n", (DWORD)lpParms->lpstrDeviceType);
#endif
	return MCIERR_INVALID_DEVICE_NAME;
    }
    
    
    if (dwParam & MCI_NOTIFY)
	mciDriverNotify(lpParms->dwCallback, wDevID,
			(dwret==0?MCI_NOTIFY_SUCCESSFUL:MCI_NOTIFY_FAILURE));
    
    /* only handled devices fall through */
    TRACE(mmsys, "wDevID = %04X wDeviceID = %d dwret = %ld\n",wDevID, lpParms->wDeviceID, dwret);
    return dwret;
}

/**************************************************************************
 * 			mciGetDriverData			[MMSYSTEM.708]
 */
DWORD WINAPI mciGetDriverData16(HDRVR16 hdrv) 
{
    FIXME(mmsys,"(%04x): stub!\n",hdrv);
    return 0x42;
}

/**************************************************************************
 * 			mciSetDriverData			[MMSYSTEM.707]
 */
DWORD WINAPI mciSetDriverData16(HDRVR16 hdrv,DWORD data) 
{
    FIXME(mmsys,"(%04x,%08lx): stub!\n",hdrv,data);
    return 0;
}

/**************************************************************************
 * 			mciClose16				[internal]
 */
static DWORD mciClose16(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD	dwRet = MCIERR_INTERNAL;
    
    TRACE(mmsys, "(%04x, %08lX, %p)\n", wDevID, dwParam, lpParms);

    if(wDevID == MCI_ALL_DEVICE_ID) {
	FIXME(mmsys, "unhandled MCI_ALL_DEVICE_ID\n");
	return MCIERR_CANNOT_USE_ALL;
    }

    switch (mciGetDrv(wDevID)->modp.wType) {
    case MCI_DEVTYPE_CD_AUDIO:
	dwRet = CDAUDIO_DriverProc16(mciGetDrv(wDevID)->modp.wDeviceID,0,
				   MCI_CLOSE, dwParam, (DWORD)lpParms);
	break;
    case MCI_DEVTYPE_WAVEFORM_AUDIO:
	dwRet = WAVE_DriverProc16(mciGetDrv(wDevID)->modp.wDeviceID, 0, 
				MCI_CLOSE, dwParam,
				(DWORD)lpParms);
	break;
    case MCI_DEVTYPE_SEQUENCER:
	dwRet = MIDI_DriverProc16(mciGetDrv(wDevID)->modp.wDeviceID, 0, 
				MCI_CLOSE, dwParam,
				(DWORD)lpParms);
	break;
	/*
	  case MCI_DEVTYPE_ANIMATION:
	  dwRet = ANIM_DriverProc16(mciGetDrv(wDevID)->modp.wDeviceID, 0, 
	  MCI_CLOSE, dwParam,
	  (DWORD)lpParms);
	  break;
	*/
    default:
	dwRet = Callbacks->CallDriverProc(mciGetDrv(wDevID)->driverProc,
					  mciGetDrv(wDevID)->modp.wDeviceID,
					  mciGetDrv(wDevID)->hDrv,MCI_CLOSE,dwParam,
					  (DWORD)lpParms);
    }
    mciGetDrv(wDevID)->modp.wType = 0;
    
    if (dwParam&MCI_NOTIFY)
	mciDriverNotify(lpParms->dwCallback,wDevID,
			(dwRet==0?MCI_NOTIFY_SUCCESSFUL:MCI_NOTIFY_FAILURE));
    
    TRACE(mmsys, "returns %ld\n",dwRet);
    return dwRet;
}


/**************************************************************************
 * 			mciSysinfo16				[internal]
 */
static DWORD mciSysInfo16(DWORD dwFlags, LPMCI_SYSINFO_PARMS16 lpParms)
{
    LPSTR	lpstrReturn;
    DWORD	*lpdwRet;
    
    TRACE(mci, "(%08lX, %08lX)\n", dwFlags, (DWORD)lpParms);
    lpstrReturn = PTR_SEG_TO_LIN(lpParms->lpstrReturn);
    switch (dwFlags) {
    case MCI_SYSINFO_QUANTITY:
	TRACE(mci, "MCI_SYSINFO_QUANTITY \n");
	lpdwRet = (DWORD *)lpstrReturn;
	*(lpdwRet) = mciInstalledCount;
	return 0;
    case MCI_SYSINFO_INSTALLNAME:
	TRACE(mci, "MCI_SYSINFO_INSTALLNAME \n");
	if (lpParms->dwRetSize < mciInstalledListLen)
	    lstrcpyn32A(lpstrReturn, lpmciInstallNames, lpParms->dwRetSize - 1);
	else
	    strcpy(lpstrReturn, lpmciInstallNames);
	return 0;
    case MCI_SYSINFO_NAME:
	TRACE(mci, "MCI_SYSINFO_NAME\n");
	if (lpParms->dwNumber > mciInstalledCount)
	    return MMSYSERR_INVALPARAM;
	{
	    DWORD	count = lpParms->dwNumber;
	    LPSTR	ptr = lpmciInstallNames;

	    while (--count > 0) 
		ptr += strlen(ptr) + 1;
	    if (lpParms->dwRetSize < strlen(ptr))
		lstrcpyn32A(lpstrReturn, ptr, lpParms->dwRetSize - 1);
	    else
		strcpy(lpstrReturn, ptr);
	}
	TRACE(mci, "(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	return 0;
    case MCI_SYSINFO_OPEN:
	TRACE(mci, "MCI_SYSINFO_OPEN \n");
	return 0;
    }
    return MMSYSERR_INVALPARAM;
}

/**************************************************************************
 *                    	mciLoadCommandResource16  [MMSYSTEM.705]
 */
UINT16 WINAPI mciLoadCommandResource16(HANDLE16 hinst,LPCSTR resname,UINT16 type)
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
    
    FIXME(mmsys,"(%04x,%s,%d): stub!\n",hinst,resname,type);
    if (!lstrcmpi32A(resname,"core")) {
	FIXME(mmsys,"(...,\"core\",...), have to use internal tables... (not there yet)\n");
	return 0;
    }
    /* if file exists "resname.mci", then load resource "resname" from it
     * otherwise directly from driver
     */
    strcpy(buf,resname);
    strcat(buf,".mci");
    if (OpenFile32(buf,&ofs,OF_EXIST)!=HFILE_ERROR32) {
	xhinst = LoadLibrary16(buf);
	if (xhinst >32)
	    hinst = xhinst;
    } /* else use passed hinst */
    segstr = SEGPTR_STRDUP(resname);
    hrsrc = FindResource16(hinst,SEGPTR_GET(segstr),type);
    SEGPTR_FREE(segstr);
    if (!hrsrc) {
	WARN(mmsys,"no special commandlist found in resource\n");
	return MCI_NO_COMMAND_TABLE;
    }
    hmem = LoadResource16(hinst,hrsrc);
    if (!hmem) {
	WARN(mmsys,"couldn't load resource??\n");
	return MCI_NO_COMMAND_TABLE;
    }
    xmem = WIN16_LockResource16(hmem);
    if (!xmem) {
	WARN(mmsys,"couldn't lock resource??\n");
	FreeResource16(hmem);
	return MCI_NO_COMMAND_TABLE;
    }
    lmem = PTR_SEG_TO_LIN(xmem);
    TRACE(mmsys,"first resource entry is %s\n",(char*)lmem);
    /* parse resource, register stuff, return unique id */
    return ++mcidevtype;
}


/**************************************************************************
 * 			mciSound				[internal]
 *  not used anymore ??
 
 DWORD mciSound(UINT16 wDevID, DWORD dwParam, LPMCI_SOUND_PARMS lpParms)
 {
 if (lpParms == NULL) return MCIERR_INTERNAL;
 if (dwParam & MCI_SOUND_NAME)
 TRACE(mci, "file='%s' !\n", lpParms->lpstrSoundName);
 return MCIERR_INVALID_DEVICE_ID;
 }
 *
 */

static const char *_mciCommandToString(UINT16 wMsg)
{
    static char buffer[100];
    
#define CASE(s) case (s): return #s
    
    switch (wMsg) {
	CASE(MCI_OPEN);
	CASE(MCI_CLOSE);
	CASE(MCI_ESCAPE);
	CASE(MCI_PLAY);
	CASE(MCI_SEEK);
	CASE(MCI_STOP);
	CASE(MCI_PAUSE);
	CASE(MCI_INFO);
	CASE(MCI_GETDEVCAPS);
	CASE(MCI_SPIN);
	CASE(MCI_SET);
	CASE(MCI_STEP);
	CASE(MCI_RECORD);
	CASE(MCI_SYSINFO);
	CASE(MCI_BREAK);
	CASE(MCI_SAVE);
	CASE(MCI_STATUS);
	CASE(MCI_CUE);
	CASE(MCI_REALIZE);
	CASE(MCI_WINDOW);
	CASE(MCI_PUT);
	CASE(MCI_WHERE);
	CASE(MCI_FREEZE);
	CASE(MCI_UNFREEZE);
	CASE(MCI_LOAD);
	CASE(MCI_CUT);
	CASE(MCI_COPY);
	CASE(MCI_PASTE);
	CASE(MCI_UPDATE);
	CASE(MCI_RESUME);
	CASE(MCI_DELETE);
    default:
	sprintf(buffer, "%04X", wMsg);
	return buffer;
	
    }
}

/**************************************************************************
 * 			mciOpen32				[internal]
 */

static DWORD mciOpen32(DWORD dwParam, LPMCI_OPEN_PARMS32A lpParms)
{
    char		str[128];
    UINT16		uDevTyp = 0;
    UINT16		wDevID = MMSYSTEM_FirstDevID();
    DWORD 		dwret;
    
    TRACE(mmsys, "(%08lX, %p)\n", dwParam, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    
    if ((dwParam & ~(MCI_OPEN_ELEMENT|MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_NOTIFY)) != 0) {
	FIXME(mmsys, "unsupported yet dwFlags=%08lX\n", 
	      (dwParam & ~(MCI_OPEN_ELEMENT|MCI_OPEN_ALIAS|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID|MCI_NOTIFY)));
    }

    while (mciGetDrv(wDevID)->modp.wType != 0) {
	wDevID = MMSYSTEM_NextDevID(wDevID);
	if (!MMSYSTEM_DevIDValid(wDevID)) {
	    TRACE(mmsys, "MAXMCIDRIVERS reached !\n");
	    return MCIERR_INTERNAL;
	}
    }
    TRACE(mmsys, "wDevID=%04X \n", wDevID);
    memcpy(mciGetOpenDrv(wDevID), lpParms, sizeof(*lpParms));
    
    if (dwParam & MCI_OPEN_ELEMENT) {
	char	*s,*t;
	
	TRACE(mmsys,"lpstrElementName='%s'\n", lpParms->lpstrElementName);
	s = lpParms->lpstrElementName;
	t = strrchr(s, '.');
	if (t) {
	    GetProfileString32A("mci extensions", t+1, "*", str, sizeof(str));
	    CharUpper32A(str);
	    TRACE(mmsys, "str = %s \n", str);
	    if (strcmp(str, "CDAUDIO") == 0) {
		uDevTyp = MCI_DEVTYPE_CD_AUDIO;
	    } else if (strcmp(str, "WAVEAUDIO") == 0) {
		uDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
	    } else if (strcmp(str, "SEQUENCER") == 0)	{
		uDevTyp = MCI_DEVTYPE_SEQUENCER;
	    } else if (strcmp(str, "ANIMATION1") == 0) {
		uDevTyp = MCI_DEVTYPE_ANIMATION;
	    } else if (strcmp(str, "AVIVIDEO") == 0) {
		uDevTyp = MCI_DEVTYPE_DIGITAL_VIDEO;
	    } else if (strcmp(str,"*") == 0) {
		TRACE(mmsys,"No [mci extensions] entry for %s found.\n",t);
		return MCIERR_EXTENSION_NOT_FOUND;
#if testing32
		/* FIXME has to written, seems to be experimental 16 bit code anyway */
	    } else  {
		HDRVR16 hdrv = OpenDriver(str, "mci", NULL);
		if (hdrv) {
		    HMODULE16	hmod;
		    
		    hmod = GetDriverModuleHandle(hdrv);
		    mciGetDrv(wDevID)->hDrv = hdrv;
		    mciGetDrv(wDevID)->driverProc = GetProcAddress16(hmod,oouch SEGPTR_GET(SEGPTR_STRDUP("DRIVERPROC")));
		    uDevTyp = MCI_DEVTYPE_OTHER;
		} else {
		    FIXME(mmsys, "[mci extensions] entry %s for %s not supported.\n",str,t);
		    return MCIERR_DEVICE_NOT_INSTALLED;
		}
#endif
	    }
	} else
	    return MCIERR_EXTENSION_NOT_FOUND;
    }
    
    if (dwParam & MCI_OPEN_ALIAS) {
	TRACE(mmsys, "Alias='%s' !\n", lpParms->lpstrAlias);
	/* FIXME is there any memory leak here ? */
	mciGetOpenDrv(wDevID)->lpstrAlias = strdup(lpParms->lpstrAlias);
	/* mplayer does allocate alias to CDAUDIO */
    }
    if (dwParam & MCI_OPEN_TYPE) {
	if (dwParam & MCI_OPEN_TYPE_ID) {
	    TRACE(mmsys, "Dev=%08lx!\n", (DWORD)lpParms->lpstrDeviceType);
	    uDevTyp = LOWORD((DWORD)lpParms->lpstrDeviceType);
	    mciGetOpenDrv(wDevID)->lpstrDeviceType = lpParms->lpstrDeviceType;
	} else {
	    if (lpParms->lpstrDeviceType == NULL) 
		return MCIERR_INTERNAL;
	    TRACE(mmsys, "Dev='%s' !\n", lpParms->lpstrDeviceType);
	    /* FIXME is there any memory leak here ? */
	    mciGetOpenDrv(wDevID)->lpstrDeviceType = strdup(lpParms->lpstrDeviceType);
	    strcpy(str, lpParms->lpstrDeviceType);
	    CharUpper32A(str);
	    if (strcmp(str, "CDAUDIO") == 0) {
		uDevTyp = MCI_DEVTYPE_CD_AUDIO;
	    } else if (strcmp(str, "WAVEAUDIO") == 0) {
		uDevTyp = MCI_DEVTYPE_WAVEFORM_AUDIO;
	    } else if (strcmp(str, "SEQUENCER") == 0)	{
		uDevTyp = MCI_DEVTYPE_SEQUENCER;
	    } else if (strcmp(str, "ANIMATION1") == 0) {
		uDevTyp = MCI_DEVTYPE_ANIMATION;
	    } else if (strcmp(str, "AVIVIDEO") == 0) {
		uDevTyp = MCI_DEVTYPE_DIGITAL_VIDEO;
	    } else {
#if testing32
		/* FIXME has to written, seems to be experimental 16 bit code anyway */
		HDRVR16 hdrv;
		TRACE(mmsys,"trying to load driver...\n");
		hdrv = OpenDriver(str,"mci",NULL);
		if (hdrv) {
		    HMODULE16	hmod;
		    
		    hmod = GetDriverModuleHandle(hdrv);
		    mciGetDrv(wDevID)->hDrv = hdrv;
		    mciGetDrv(wDevID)->driverProc = GetProcAddress16(hmod,oouch SEGPTR_GET(SEGPTR_STRDUP("DRIVERPROC")));
		    uDevTyp = MCI_DEVTYPE_OTHER;
		} else
#endif
		    return MCIERR_DEVICE_NOT_INSTALLED;
	    }
	}
    }
    mciGetDrv(wDevID)->modp.wType = uDevTyp;
    mciGetDrv(wDevID)->modp.wDeviceID = 0;  /* FIXME? for multiple devices */
    lpParms->wDeviceID = wDevID;
    TRACE(mmsys, "mcidev=%d, uDevTyp=%04X wDeviceID=%04X !\n", 
	  wDevID, uDevTyp, lpParms->wDeviceID);
    switch (uDevTyp) {
    case MCI_DEVTYPE_CD_AUDIO:
	dwret = CDAUDIO_DriverProc32( 0, 0, MCI_OPEN_DRIVER,
				    dwParam, (DWORD)lpParms);
	break;
    case MCI_DEVTYPE_WAVEFORM_AUDIO:
	dwret =  WAVE_DriverProc32( 0, 0, MCI_OPEN_DRIVER, 
				  dwParam, (DWORD)lpParms);
	break;
    case MCI_DEVTYPE_SEQUENCER:
	dwret = MIDI_DriverProc32( 0, 0, MCI_OPEN_DRIVER, 
				 dwParam, (DWORD)lpParms);
	break;
    case MCI_DEVTYPE_ANIMATION:
	dwret = ANIM_DriverProc32( 0, 0, MCI_OPEN_DRIVER, 
				 dwParam, (DWORD)lpParms);
	break;
    case MCI_DEVTYPE_DIGITAL_VIDEO:
	TRACE(mmsys, "No DIGITAL_VIDEO yet !\n");
	return MCIERR_DEVICE_NOT_INSTALLED;
    default:
#if testing32
	dwret = Callbacks->CallDriverProc(mciGetDrv(wDevID)->driverProc,0,mciGetDrv(wDevID)->hdrv,MCI_OPEN_DRIVER,dwParam,(DWORD)lpParms);
	WARN(mmsys, "Invalid Device Name '%08lx' !\n", (DWORD)lpParms->lpstrDeviceType);
#endif
	return MCIERR_INVALID_DEVICE_NAME;
    }
    
    
    if (dwParam & MCI_NOTIFY)
	mciDriverNotify(lpParms->dwCallback,wDevID,
			(dwret==0?MCI_NOTIFY_SUCCESSFUL:MCI_NOTIFY_FAILURE));
    
    /* only handled devices fall through */
    TRACE(mmsys, "wDevID = %04X wDeviceID = %d dwret = %ld\n",wDevID, lpParms->wDeviceID, dwret);
    return dwret;
}

/**************************************************************************
 * 			mciClose32				[internal]
 */
static DWORD mciClose32(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD	dwRet = MCIERR_INTERNAL;
    
    TRACE(mmsys, "(%04x, %08lX, %p)\n", wDevID, dwParam, lpParms);

    if (wDevID == MCI_ALL_DEVICE_ID) {
	FIXME(mmsys, "unhandled MCI_ALL_DEVICE_ID\n");
	return MCIERR_CANNOT_USE_ALL;
    }

    switch (mciGetDrv(wDevID)->modp.wType) {
    case MCI_DEVTYPE_CD_AUDIO:
	dwRet = CDAUDIO_DriverProc32(mciGetDrv(wDevID)->modp.wDeviceID,0,
				   MCI_CLOSE, dwParam, (DWORD)lpParms);
	break;
    case MCI_DEVTYPE_WAVEFORM_AUDIO:
	dwRet = WAVE_DriverProc32(mciGetDrv(wDevID)->modp.wDeviceID, 0, 
				MCI_CLOSE, dwParam,
				(DWORD)lpParms);
	break;
    case MCI_DEVTYPE_SEQUENCER:
	dwRet = MIDI_DriverProc32(mciGetDrv(wDevID)->modp.wDeviceID, 0, 
				MCI_CLOSE, dwParam,
				(DWORD)lpParms);
	break;
	/*
	  case MCI_DEVTYPE_ANIMATION:
	  dwRet = ANIM_DriverProc32(mciGetDrv(wDevID)->modp.wDeviceID, 0, 
	  MCI_CLOSE, dwParam,
	  (DWORD)lpParms);
	  break;
	*/
    default:
	dwRet = Callbacks->CallDriverProc(mciGetDrv(wDevID)->driverProc,
					  mciGetDrv(wDevID)->modp.wDeviceID,
					  mciGetDrv(wDevID)->hDrv,MCI_CLOSE,dwParam,
					  (DWORD)lpParms);
    }
    mciGetDrv(wDevID)->modp.wType = 0;
    
    if (dwParam&MCI_NOTIFY)
	mciDriverNotify(lpParms->dwCallback,wDevID,
			(dwRet==0?MCI_NOTIFY_SUCCESSFUL:MCI_NOTIFY_FAILURE));
    
    TRACE(mmsys, "returns %ld\n",dwRet);
    return dwRet;
}

/**************************************************************************
 * 			mciSysinfo32				[internal]
 */
static DWORD mciSysInfo32(DWORD dwFlags, LPMCI_SYSINFO_PARMS32A lpParms)
{
    TRACE(mci, "(%08lX, %08lX)\n", dwFlags, (DWORD)lpParms);
    switch (dwFlags) {
    case MCI_SYSINFO_QUANTITY:
	TRACE(mci, "MCI_SYSINFO_QUANTITY \n");
	*(DWORD*)lpParms->lpstrReturn = mciInstalledCount;
	return 0;
    case MCI_SYSINFO_INSTALLNAME:
	TRACE(mci, "MCI_SYSINFO_INSTALLNAME \n");
	if (lpParms->dwRetSize < mciInstalledListLen)
	    lstrcpyn32A(lpParms->lpstrReturn, lpmciInstallNames, lpParms->dwRetSize - 1);
	else
	    strcpy(lpParms->lpstrReturn, lpmciInstallNames);
	return 0;
    case MCI_SYSINFO_NAME:
	TRACE(mci, "MCI_SYSINFO_NAME\n");	fflush(stddeb);
	if (lpParms->dwNumber > mciInstalledCount)
	    return MMSYSERR_INVALPARAM;
	{
	    DWORD	count = lpParms->dwNumber;
	    LPSTR	ptr = lpmciInstallNames;

	    while (--count > 0) 
		ptr += strlen(ptr) + 1;
	    if (lpParms->dwRetSize < strlen(ptr))
		lstrcpyn32A(lpParms->lpstrReturn, ptr, lpParms->dwRetSize - 1);
	    else
		strcpy(lpParms->lpstrReturn, ptr);
	}
	TRACE(mci, "(%ld) => '%s'\n", lpParms->dwNumber, lpParms->lpstrReturn);
	return 0;
    case MCI_SYSINFO_OPEN:
	TRACE(mci, "MCI_SYSINFO_OPEN \n");
	return 0;
    }
    return MMSYSERR_INVALPARAM;
}

struct SCA32 {
    UINT32 	wDevID;
    UINT32 	wMsg;
    DWORD 	dwParam1;
    DWORD 	dwParam2;
};

DWORD WINAPI mciSendCommand32A(UINT32 wDevID, UINT32 wMsg, DWORD dwParam1, DWORD dwParam2);

static DWORD WINAPI mciSCAStarter32(LPVOID arg)
{
    struct SCA32*	sca = (struct SCA32*)arg;
    DWORD		ret;

    TRACE(mci, "In thread for async command (%08x,%s,%08lx,%08lx)\n",
	  sca->wDevID, _mciCommandToString(sca->wMsg), sca->dwParam1, sca->dwParam2);
    ret = mciSendCommand32A(sca->wDevID, sca->wMsg, sca->dwParam1 | MCI_WAIT, sca->dwParam2);
    free(sca);
    return ret;
}

/**************************************************************************
 * 				mciSendCommandAsync32		[internal]
 */
DWORD mciSendCommandAsync32(UINT32 wDevID, UINT32 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    struct SCA32*	sca = malloc(sizeof(struct SCA32));

    if (sca == 0)	return MCIERR_OUT_OF_MEMORY;

    sca->wDevID   = wDevID;
    sca->wMsg     = wMsg;
    sca->dwParam1 = dwParam1;
    sca->dwParam2 = dwParam2;
    
    if (CreateThread(NULL, 0, mciSCAStarter32, sca, 0, NULL) == 0) {
	WARN(mci, "Couldn't allocate thread for async command handling, sending synchonously\n");
	return mciSCAStarter32(&sca);
    }
    return 0;
}

/**************************************************************************
 * 				mciSendCommand32A		[WINMM.49]
 */
DWORD WINAPI mciSendCommand32A(UINT32 wDevID, UINT32 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE(mci,"(%08x,%s,%08lx,%08lx)\n",
	  wDevID,_mciCommandToString(wMsg),dwParam1,dwParam2);

    switch (wMsg) {
    case MCI_OPEN:
	return mciOpen32(dwParam1, (LPMCI_OPEN_PARMS32A)dwParam2);
    case MCI_CLOSE:
	return mciClose32(wDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_SYSINFO:
	return mciSysInfo32(dwParam1, (LPMCI_SYSINFO_PARMS32A)dwParam2);
    default:
	{
	    DWORD		dwRet = MCIERR_INTERNAL;
	    LONG 		(*proc)(DWORD, HDRVR16, DWORD, DWORD, DWORD) = 0;

	    if (wDevID == MCI_ALL_DEVICE_ID) {
		FIXME(mci, "unhandled MCI_ALL_DEVICE_ID\n");
		return MCIERR_CANNOT_USE_ALL;
	    }
	    
	    if (!MMSYSTEM_DevIDValid(wDevID))
		return MMSYSERR_INVALPARAM;

	    switch (mciGetDrv(wDevID)->modp.wType) {
	    case MCI_DEVTYPE_CD_AUDIO:		proc = CDAUDIO_DriverProc32;	break;
	    case MCI_DEVTYPE_WAVEFORM_AUDIO:	proc = WAVE_DriverProc32;	break;
	    case MCI_DEVTYPE_SEQUENCER:		proc = MIDI_DriverProc32;	break;
		/*  case MCI_DEVTYPE_ANIMATION: proc = ANIM_DriverProc32;	break;*/
	    }
	    if (proc) {
		dwRet = (proc)(mciGetDrv(wDevID)->modp.wDeviceID, mciGetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
	    } else if (mciGetDrv(wDevID)->driverProc) {
		FIXME(mmsys, "is that correct ?\n");
		dwRet = Callbacks->CallDriverProc(mciGetDrv(wDevID)->driverProc,
						  mciGetDrv(wDevID)->modp.wDeviceID,
						  mciGetDrv(wDevID)->hDrv,wMsg, dwParam1, dwParam2);
	    } else {
		WARN(mmsys, "unknown device type=%04X !\n", mciGetDrv(wDevID)->modp.wType);
	    }
	    return dwRet;
	}
    }
    return 0x1; /* !ok */
}
/**************************************************************************
 * 				mciSendCommand			[MMSYSTEM.701]
 */
DWORD WINAPI mciSendCommand(UINT16 wDevID, UINT16 wMsg, DWORD dwParam1,
                            DWORD dwParam2)
{
    TRACE(mmsys, "(%04X, %s, %08lX, %08lX)\n", 
	  wDevID, _mciCommandToString(wMsg), dwParam1, dwParam2);

    switch (wMsg) {
    case MCI_OPEN:
	return mciOpen16(dwParam1, (LPMCI_OPEN_PARMS16)dwParam2);
    case MCI_CLOSE:
	return mciClose16(wDevID, dwParam1,
			 (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_SYSINFO:
	return mciSysInfo16(dwParam1,
			   (LPMCI_SYSINFO_PARMS16)PTR_SEG_TO_LIN(dwParam2));
    default:
	{
	    DWORD		dwRet = MCIERR_INTERNAL;
	    LONG 		(*proc)(DWORD, HDRVR16, WORD, DWORD, DWORD) = 0;

	    if (wDevID == MCI_ALL_DEVICE_ID) {
		FIXME(mci, "unhandled MCI_ALL_DEVICE_ID\n");
		return MCIERR_CANNOT_USE_ALL;
	    }
    
	    if (!MMSYSTEM_DevIDValid(wDevID))
		return MMSYSERR_INVALPARAM;
    
	    switch (mciGetDrv(wDevID)->modp.wType) {
	    case MCI_DEVTYPE_CD_AUDIO:		proc = CDAUDIO_DriverProc16;	break;
	    case MCI_DEVTYPE_WAVEFORM_AUDIO:	proc = WAVE_DriverProc16;	break;
	    case MCI_DEVTYPE_SEQUENCER:		proc = MIDI_DriverProc16;	break;
		/*  case MCI_DEVTYPE_ANIMATION: proc = ANIM_DriverProc16;	break;*/
	    }
	    if (proc) {
		dwRet = (proc)(mciGetDrv(wDevID)->modp.wDeviceID, mciGetDrv(wDevID)->hDrv, wMsg, dwParam1, dwParam2);
	    } else if (mciGetDrv(wDevID)->driverProc) {
		dwRet = Callbacks->CallDriverProc(mciGetDrv(wDevID)->driverProc,
						  mciGetDrv(wDevID)->modp.wDeviceID,
						  mciGetDrv(wDevID)->hDrv,wMsg, dwParam1, dwParam2);
	    } else {
		WARN(mmsys, "unknown device type=%04X !\n", mciGetDrv(wDevID)->modp.wType);
	    }
	    return dwRet;
	}
    }
    return MMSYSERR_INVALPARAM;
}

/**************************************************************************
 * 				mciGetDeviceID	       	[MMSYSTEM.703]
 */
UINT16 WINAPI mciGetDeviceID(LPCSTR lpstrName)
{
    UINT16 wDevID;
    
    TRACE(mmsys, "(\"%s\")\n", lpstrName);

    if (!lpstrName)
	return 0;
    
    if (!lstrcmpi32A(lpstrName, "ALL"))
	return MCI_ALL_DEVICE_ID;
    
    wDevID = MMSYSTEM_FirstDevID();
    while (MMSYSTEM_DevIDValid(wDevID) && mciGetDrv(wDevID)->modp.wType) {
	if (mciGetOpenDrv(wDevID)->lpstrDeviceType && strcmp(mciGetOpenDrv(wDevID)->lpstrDeviceType, lpstrName) == 0)
	    return wDevID;
	
	if (mciGetOpenDrv(wDevID)->lpstrAlias && strcmp(mciGetOpenDrv(wDevID)->lpstrAlias, lpstrName) == 0)
	    return wDevID;
	
	wDevID = MMSYSTEM_NextDevID(wDevID);
    }
    
    return 0;
}

/**************************************************************************
 * 				mciSetYieldProc		[MMSYSTEM.714]
 */
BOOL16 WINAPI mciSetYieldProc (UINT16 uDeviceID, 
                               YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    return FALSE;
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
 */
UINT16 WINAPI mciGetDeviceIDFromElementID(DWORD dwElementID, LPCSTR lpstrType)
{
    return 0;
}

/**************************************************************************
 * 				mciGetYieldProc		[MMSYSTEM.716]
 */
YIELDPROC WINAPI mciGetYieldProc(UINT16 uDeviceID, DWORD * lpdwYieldData)
{
    return NULL;
}

/**************************************************************************
 * 				mciGetCreatorTask	[MMSYSTEM.717]
 */
HTASK16 WINAPI mciGetCreatorTask(UINT16 uDeviceID)
{
    return 0;
}

/**************************************************************************
 * 				midiOutGetNumDevs	[WINMM.80]
 */
UINT32 WINAPI midiOutGetNumDevs32(void)
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
UINT32 WINAPI midiOutGetDevCaps32W(UINT32 uDeviceID,LPMIDIOUTCAPS32W lpCaps, UINT32 uSize)
{
    MIDIOUTCAPS16	moc16;
    UINT32		ret;
    
    ret = midiOutGetDevCaps16(uDeviceID,&moc16,sizeof(moc16));
    lpCaps->wMid		= moc16.wMid;
    lpCaps->wPid		= moc16.wPid;
    lpCaps->vDriverVersion	= moc16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname,moc16.szPname);
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
UINT32 WINAPI midiOutGetDevCaps32A(UINT32 uDeviceID,LPMIDIOUTCAPS32A lpCaps, UINT32 uSize)
{
    MIDIOUTCAPS16	moc16;
    UINT32		ret;
    
    ret = midiOutGetDevCaps16(uDeviceID,&moc16,sizeof(moc16));
    lpCaps->wMid		= moc16.wMid;
    lpCaps->wPid		= moc16.wPid;
    lpCaps->vDriverVersion	= moc16.vDriverVersion;
    strcpy(lpCaps->szPname,moc16.szPname);
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
UINT16 WINAPI midiOutGetDevCaps16(UINT16 uDeviceID,LPMIDIOUTCAPS16 lpCaps, UINT16 uSize)
{
    TRACE(mmsys, "midiOutGetDevCaps\n");
    return modMessage(uDeviceID,MODM_GETDEVCAPS,0,(DWORD)lpCaps,uSize);
}

/**************************************************************************
 * 				midiOutGetErrorTextA 	[WINMM.77]
 */
UINT32 WINAPI midiOutGetErrorText32A(UINT32 uError, LPSTR lpText, UINT32 uSize)
{
    TRACE(mmsys, "midiOutGetErrorText\n");
    return midiGetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiOutGetErrorTextW 	[WINMM.78]
 */
UINT32 WINAPI midiOutGetErrorText32W(UINT32 uError, LPWSTR lpText, UINT32 uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(),0,uSize);
    UINT32	ret;
    
    TRACE(mmsys, "midiOutGetErrorText\n");
    ret = midiGetErrorText(uError, xstr, uSize);
    lstrcpyAtoW(lpText,xstr);
    HeapFree(GetProcessHeap(),0,xstr);
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
    lstrcpyn32A(lpText, msgptr, uSize);
    return TRUE;
}

/**************************************************************************
 * 				midiOutOpen    		[WINM.84]
 */
UINT32 WINAPI midiOutOpen32(HMIDIOUT32 * lphMidiOut, UINT32 uDeviceID,
                            DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDIOUT16	hmo16;
    UINT32	ret;
    
    ret = midiOutOpen16(&hmo16,uDeviceID,dwCallback,dwInstance,
			CALLBACK32CONV(dwFlags));
    if (lphMidiOut) *lphMidiOut = hmo16;
    return ret;
}

/**************************************************************************
 * 				midiOutOpen    		[MMSYSTEM.204]
 */
UINT16 WINAPI midiOutOpen16(HMIDIOUT16 * lphMidiOut, UINT16 uDeviceID,
                            DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDI16			hMidiOut;
    LPMIDIOPENDESC		lpDesc;
    DWORD			dwRet = 0;
    BOOL32			bMapperFlg = FALSE;
    
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
UINT32 WINAPI midiOutClose32(HMIDIOUT32 hMidiOut)
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
UINT32 WINAPI midiOutPrepareHeader32(HMIDIOUT32 hMidiOut,
                                     MIDIHDR * lpMidiOutHdr, UINT32 uSize)
{
    LPMIDIOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X, %p, %d)\n", 
	  hMidiOut, lpMidiOutHdr, uSize);
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
                                     MIDIHDR * lpMidiOutHdr, UINT16 uSize)
{
    LPMIDIOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X, %p, %d)\n", 
	  hMidiOut, lpMidiOutHdr, uSize);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpMidiOutHdr->reserved = (DWORD)PTR_SEG_TO_LIN(lpMidiOutHdr->lpData);
    return modMessage(lpDesc->wDevID, MODM_PREPARE, lpDesc->dwInstance, 
		      (DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[WINMM.89]
 */
UINT32 WINAPI midiOutUnprepareHeader32(HMIDIOUT32 hMidiOut,
                                       MIDIHDR * lpMidiOutHdr, UINT32 uSize)
{
    return midiOutUnprepareHeader16(hMidiOut,lpMidiOutHdr,uSize);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[MMSYSTEM.207]
 */
UINT16 WINAPI midiOutUnprepareHeader16(HMIDIOUT16 hMidiOut,
				       MIDIHDR * lpMidiOutHdr, UINT16 uSize)
{
    LPMIDIOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X, %p, %d)\n", 
	  hMidiOut, lpMidiOutHdr, uSize);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return modMessage(lpDesc->wDevID, MODM_UNPREPARE, lpDesc->dwInstance, 
		      (DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiOutShortMsg		[WINMM.88]
 */
UINT32 WINAPI midiOutShortMsg32(HMIDIOUT32 hMidiOut, DWORD dwMsg)
{
    return midiOutShortMsg16(hMidiOut,dwMsg);
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
UINT32 WINAPI midiOutLongMsg32(HMIDIOUT32 hMidiOut,
                               MIDIHDR * lpMidiOutHdr, UINT32 uSize)
{
    return midiOutLongMsg16(hMidiOut,lpMidiOutHdr,uSize);
}

/**************************************************************************
 * 				midiOutLongMsg		[MMSYSTEM.209]
 */
UINT16 WINAPI midiOutLongMsg16(HMIDIOUT16 hMidiOut,
                               MIDIHDR * lpMidiOutHdr, UINT16 uSize)
{
    LPMIDIOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X, %p, %d)\n", 
	  hMidiOut, lpMidiOutHdr, uSize);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return modMessage(lpDesc->wDevID, MODM_LONGDATA, lpDesc->dwInstance, 
		      (DWORD)lpMidiOutHdr, (DWORD)uSize);
}

/**************************************************************************
 * 				midiOutReset		[WINMM.86]
 */
UINT32 WINAPI midiOutReset32(HMIDIOUT32 hMidiOut)
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
UINT32 WINAPI midiOutGetVolume32(UINT32 uDeviceID, DWORD * lpdwVolume)
{
    return midiOutGetVolume16(uDeviceID,lpdwVolume);
}
/**************************************************************************
 * 				midiOutGetVolume	[MMSYSTEM.211]
 */
UINT16 WINAPI midiOutGetVolume16(UINT16 uDeviceID, DWORD * lpdwVolume)
{
    TRACE(mmsys, "(%04X, %p);\n", uDeviceID, lpdwVolume);
    return modMessage(uDeviceID, MODM_GETVOLUME, 0L, (DWORD)lpdwVolume, 0L);
}

/**************************************************************************
 * 				midiOutSetVolume	[WINMM.87]
 */
UINT32 WINAPI midiOutSetVolume32(UINT32 uDeviceID, DWORD dwVolume)
{
    return midiOutSetVolume16(uDeviceID,dwVolume);
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
UINT32 WINAPI midiOutCachePatches32(HMIDIOUT32 hMidiOut, UINT32 uBank,
                                    WORD * lpwPatchArray, UINT32 uFlags)
{
    return midiOutCachePatches16(hMidiOut,uBank,lpwPatchArray,uFlags);
}

/**************************************************************************
 * 				midiOutCachePatches		[MMSYSTEM.213]
 */
UINT16 WINAPI midiOutCachePatches16(HMIDIOUT16 hMidiOut, UINT16 uBank,
                                    WORD * lpwPatchArray, UINT16 uFlags)
{
    /* not really necessary to support this */
    FIXME(mmsys, "not supported yet\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[WINMM.72]
 */
UINT32 WINAPI midiOutCacheDrumPatches32(HMIDIOUT32 hMidiOut, UINT32 uPatch,
                                        WORD * lpwKeyArray, UINT32 uFlags)
{
    return midiOutCacheDrumPatches16(hMidiOut,uPatch,lpwKeyArray,uFlags);
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[MMSYSTEM.214]
 */
UINT16 WINAPI midiOutCacheDrumPatches16(HMIDIOUT16 hMidiOut, UINT16 uPatch,
                                        WORD * lpwKeyArray, UINT16 uFlags)
{
    FIXME(mmsys, "not supported yet\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutGetID		[WINMM.79]
 */
UINT32 WINAPI midiOutGetID32(HMIDIOUT32 hMidiOut, UINT32 * lpuDeviceID)
{
    UINT16	xid;
    UINT32	ret;
    
    ret = midiOutGetID16(hMidiOut,&xid);
    *lpuDeviceID = xid;
    return ret;
}

/**************************************************************************
 * 				midiOutGetID		[MMSYSTEM.215]
 */
UINT16 WINAPI midiOutGetID16(HMIDIOUT16 hMidiOut, UINT16 * lpuDeviceID)
{
    TRACE(mmsys, "midiOutGetID\n");
    return 0;
}

/**************************************************************************
 * 				midiOutMessage		[WINMM.83]
 */
DWORD WINAPI midiOutMessage32(HMIDIOUT32 hMidiOut, UINT32 uMessage, 
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
	return midiOutGetDevCaps32A(hMidiOut,(LPMIDIOUTCAPS32A)dwParam1,dwParam2);
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
	    hMidiOut,uMessage,dwParam1,dwParam2);
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
	return midiOutGetVolume16(hMidiOut,(LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case MODM_LONGDATA:
	return midiOutLongMsg16(hMidiOut,(LPMIDIHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case MODM_PREPARE:
	return midiOutPrepareHeader16(hMidiOut,(LPMIDIHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case MODM_UNPREPARE:
	return midiOutUnprepareHeader16(hMidiOut,(LPMIDIHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hMidiOut,uMessage,dwParam1,dwParam2);
	break;
    }
    return modMessage(lpDesc->wDevID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				midiInGetNumDevs	[WINMM.64]
 */
UINT32 WINAPI midiInGetNumDevs32(void)
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
UINT32 WINAPI midiInGetDevCaps32W(UINT32 uDeviceID,
                                  LPMIDIINCAPS32W lpCaps, UINT32 uSize)
{
    MIDIINCAPS16	mic16;
    UINT32		ret = midiInGetDevCaps16(uDeviceID,&mic16,uSize);
    
    lpCaps->wMid = mic16.wMid;
    lpCaps->wPid = mic16.wPid;
    lpCaps->vDriverVersion = mic16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname,mic16.szPname);
    lpCaps->dwSupport = mic16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				midiInGetDevCaps	[WINMM.59]
 */
UINT32 WINAPI midiInGetDevCaps32A(UINT32 uDeviceID,
                                  LPMIDIINCAPS32A lpCaps, UINT32 uSize)
{
    MIDIINCAPS16	mic16;
    UINT32		ret = midiInGetDevCaps16(uDeviceID,&mic16,uSize);
    
    lpCaps->wMid = mic16.wMid;
    lpCaps->wPid = mic16.wPid;
    lpCaps->vDriverVersion = mic16.vDriverVersion;
    strcpy(lpCaps->szPname,mic16.szPname);
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
    return midMessage(uDeviceID,MIDM_GETDEVCAPS,0,(DWORD)lpCaps,uSize);;
}

/**************************************************************************
 * 				midiInGetErrorText 		[WINMM.62]
 */
UINT32 WINAPI midiInGetErrorText32W(UINT32 uError, LPWSTR lpText, UINT32 uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(),0,uSize);
    UINT32	ret = midiInGetErrorText16(uError,xstr,uSize);
    lstrcpyAtoW(lpText,xstr);
    HeapFree(GetProcessHeap(),0,xstr);
    return ret;
}
/**************************************************************************
 * 				midiInGetErrorText 		[WINMM.61]
 */
UINT32 WINAPI midiInGetErrorText32A(UINT32 uError, LPSTR lpText, UINT32 uSize)
{
    return midiInGetErrorText16(uError,lpText,uSize);
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
UINT32 WINAPI midiInOpen32(HMIDIIN32 * lphMidiIn, UINT32 uDeviceID,
                           DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDIIN16	xhmid16;
    UINT32 		ret = midiInOpen16(&xhmid16,uDeviceID,dwCallback,dwInstance,
					   CALLBACK32CONV(dwFlags));
    if (lphMidiIn) 
	*lphMidiIn = xhmid16;
    return ret;
}

/**************************************************************************
 * 				midiInOpen		[MMSYSTEM.304]
 */
UINT16 WINAPI midiInOpen16(HMIDIIN16 * lphMidiIn, UINT16 uDeviceID,
                           DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDI16			hMidiIn;
    LPMIDIOPENDESC		lpDesc;
    DWORD			dwRet = 0;
    BOOL32			bMapperFlg = FALSE;
    
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
UINT32 WINAPI midiInClose32(HMIDIIN32 hMidiIn)
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
UINT32 WINAPI midiInPrepareHeader32(HMIDIIN32 hMidiIn,
                                    MIDIHDR * lpMidiInHdr, UINT32 uSize)
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
                                    MIDIHDR * lpMidiInHdr, UINT16 uSize)
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
UINT32 WINAPI midiInUnprepareHeader32(HMIDIIN32 hMidiIn,
                                      MIDIHDR * lpMidiInHdr, UINT32 uSize)
{
    return midiInUnprepareHeader16(hMidiIn,lpMidiInHdr,uSize);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[MMSYSTEM.307]
 */
UINT16 WINAPI midiInUnprepareHeader16(HMIDIIN16 hMidiIn,
                                      MIDIHDR * lpMidiInHdr, UINT16 uSize)
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
UINT32 WINAPI midiInAddBuffer32(HMIDIIN32 hMidiIn,
                                MIDIHDR * lpMidiInHdr, UINT32 uSize)
{
    return midiInAddBuffer16(hMidiIn,lpMidiInHdr,uSize);
}

/**************************************************************************
 * 				midiInAddBuffer		[MMSYSTEM.308]
 */
UINT16 WINAPI midiInAddBuffer16(HMIDIIN16 hMidiIn,
                                MIDIHDR * lpMidiInHdr, UINT16 uSize)
{
    TRACE(mmsys, "midiInAddBuffer\n");
    return 0;
}

/**************************************************************************
 * 				midiInStart			[WINMM.69]
 */
UINT32 WINAPI midiInStart32(HMIDIIN32 hMidiIn)
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
UINT32 WINAPI midiInStop32(HMIDIIN32 hMidiIn)
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
UINT32 WINAPI midiInReset32(HMIDIIN32 hMidiIn)
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
UINT32 WINAPI midiInGetID32(HMIDIIN32 hMidiIn, UINT32* lpuDeviceID)
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
DWORD WINAPI midiInMessage32(HMIDIIN32 hMidiIn, UINT32 uMessage, 
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
	return midiInGetDevCaps32A(hMidiIn,(LPMIDIINCAPS32A)dwParam1,dwParam2);
    case MIDM_GETNUMDEVS:
    case MIDM_RESET:
    case MIDM_STOP:
    case MIDM_START:
    case MIDM_CLOSE:
	/* no argument conversion needed */
	break;
    case MIDM_PREPARE:
	return midiInPrepareHeader32(hMidiIn,(LPMIDIHDR)dwParam1,dwParam2);
    case MIDM_UNPREPARE:
	return midiInUnprepareHeader32(hMidiIn,(LPMIDIHDR)dwParam1,dwParam2);
    case MIDM_ADDBUFFER:
	return midiInAddBuffer32(hMidiIn,(LPMIDIHDR)dwParam1,dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hMidiIn,uMessage,dwParam1,dwParam2);
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
    TRACE(mmsys, "(%04X, %04X, %08lX, %08lX)\n", 
	  hMidiIn, uMessage, dwParam1, dwParam2);
    lpDesc = (LPMIDIOPENDESC) USER_HEAP_LIN_ADDR(hMidiIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case MIDM_OPEN:
	WARN(mmsys,"can't handle MIDM_OPEN!\n");
	return 0;
    case MIDM_GETDEVCAPS:
	return midiInGetDevCaps16(hMidiIn,(LPMIDIINCAPS16)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case MIDM_GETNUMDEVS:
    case MIDM_RESET:
    case MIDM_STOP:
    case MIDM_START:
    case MIDM_CLOSE:
	/* no argument conversion needed */
	break;
    case MIDM_PREPARE:
	return midiInPrepareHeader16(hMidiIn,(LPMIDIHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case MIDM_UNPREPARE:
	return midiInUnprepareHeader16(hMidiIn,(LPMIDIHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case MIDM_ADDBUFFER:
	return midiInAddBuffer16(hMidiIn,(LPMIDIHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hMidiIn,uMessage,dwParam1,dwParam2);
	break;
    }
    return midMessage(0, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				midiStreamOpen		[MMSYSTEM.91]
 */
MMRESULT32 WINAPI midiStreamOpen32(HMIDISTRM32 *phms,LPUINT32 devid,DWORD cMidi,DWORD dwCallback,DWORD dwInstance,DWORD fdwOpen) {
	FIXME(midi,"(%p,%p,%d,%p,%p,0x%08lx),stub!\n",phms,devid,cMidi,dwCallback,dwInstance,fdwOpen);
	return MMSYSERR_NOTSUPPORTED;
}


/**************************************************************************
 * 				waveOutGetNumDevs		[MMSYSTEM.401]
 */
UINT32 WINAPI waveOutGetNumDevs32() {
    return waveOutGetNumDevs16();
}

/**************************************************************************
 * 				waveOutGetNumDevs		[WINMM.167]
 */
UINT16 WINAPI waveOutGetNumDevs16()
{
    UINT16	count = 0;
    TRACE(mmsys, "waveOutGetNumDevs\n");
    count += wodMessage( MMSYSTEM_FirstDevID(), WODM_GETNUMDEVS, 0L, 0L, 0L);
    TRACE(mmsys, "waveOutGetNumDevs return %u \n", count);
    return count;
}

/**************************************************************************
 * 				waveOutGetDevCaps		[MMSYSTEM.402]
 */
UINT16 WINAPI waveOutGetDevCaps16(UINT16 uDeviceID, WAVEOUTCAPS16 * lpCaps,
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
UINT32 WINAPI waveOutGetDevCaps32A(UINT32 uDeviceID, LPWAVEOUTCAPS32A lpCaps,
				   UINT32 uSize)
{
    WAVEOUTCAPS16	woc16;
    UINT16 ret = waveOutGetDevCaps16(uDeviceID,&woc16,sizeof(woc16));
    
    lpCaps->wMid = woc16.wMid;
    lpCaps->wPid = woc16.wPid;
    lpCaps->vDriverVersion = woc16.vDriverVersion;
    strcpy(lpCaps->szPname,woc16.szPname);
    lpCaps->dwFormats = woc16.dwFormats;
    lpCaps->wChannels = woc16.wChannels;
    lpCaps->dwSupport = woc16.dwSupport;
    return ret;
}

/**************************************************************************
 * 				waveOutGetDevCapsW		[WINMM.163]
 */
UINT32 WINAPI waveOutGetDevCaps32W(UINT32 uDeviceID, LPWAVEOUTCAPS32W lpCaps,
				   UINT32 uSize)
{
    WAVEOUTCAPS16	woc16;
    UINT32 ret = waveOutGetDevCaps16(uDeviceID,&woc16,sizeof(woc16));
    
    lpCaps->wMid = woc16.wMid;
    lpCaps->wPid = woc16.wPid;
    lpCaps->vDriverVersion = woc16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname,woc16.szPname);
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
UINT32 WINAPI waveOutGetErrorText32A(UINT32 uError, LPSTR lpText, UINT32 uSize)
{
    return(waveOutGetErrorText16(uError, lpText, uSize));
}

/**************************************************************************
 * 				waveOutGetErrorTextW 	[WINMM.165]
 */
UINT32 WINAPI waveOutGetErrorText32W(UINT32 uError, LPWSTR lpText, UINT32 uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(),0,uSize);
    UINT32	ret = waveOutGetErrorText32A(uError, xstr, uSize);
    
    lstrcpyAtoW(lpText,xstr);
    HeapFree(GetProcessHeap(),0,xstr);
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
    lstrcpyn32A(lpText, msgptr, uSize);
    return TRUE;
}

/**************************************************************************
 *			waveOutOpen			[WINMM.173]
 * All the args/structs have the same layout as the win16 equivalents
 */
UINT32 WINAPI waveOutOpen32(HWAVEOUT32 * lphWaveOut, UINT32 uDeviceID,
                            const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
                            DWORD dwInstance, DWORD dwFlags)
{
    HWAVEOUT16	hwo16;
    UINT32	ret = waveOutOpen16(&hwo16,uDeviceID,lpFormat,dwCallback,dwInstance,
				    CALLBACK32CONV(dwFlags));

    if (lphWaveOut) *lphWaveOut=hwo16;
    return ret;
}
/**************************************************************************
 *			waveOutOpen			[MMSYSTEM.404]
 */
UINT16 WINAPI waveOutOpen16(HWAVEOUT16 * lphWaveOut, UINT16 uDeviceID,
                            const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
                            DWORD dwInstance, DWORD dwFlags)
{
    HWAVEOUT16	hWaveOut;
    LPWAVEOPENDESC	lpDesc;
    DWORD		dwRet = 0;
    BOOL32		bMapperFlg = FALSE;
    
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
	dwRet = waveOutClose32(hWaveOut);
    }
    return dwRet;
}

/**************************************************************************
 * 				waveOutClose		[WINMM.161]
 */
UINT32 WINAPI waveOutClose32(HWAVEOUT32 hWaveOut)
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
    return wodMessage( lpDesc->uDeviceID, WODM_CLOSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveOutPrepareHeader	[WINMM.175]
 */
UINT32 WINAPI waveOutPrepareHeader32(HWAVEOUT32 hWaveOut,
				     WAVEHDR * lpWaveOutHdr, UINT32 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", 
	  hWaveOut, lpWaveOutHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return wodMessage( lpDesc->uDeviceID, WODM_PREPARE, lpDesc->dwInstance, 
		       (DWORD)lpWaveOutHdr,uSize);
}
/**************************************************************************
 * 				waveOutPrepareHeader	[MMSYSTEM.406]
 */
UINT16 WINAPI waveOutPrepareHeader16(HWAVEOUT16 hWaveOut,
                                     WAVEHDR * lpWaveOutHdr, UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    LPBYTE		saveddata = lpWaveOutHdr->lpData;
    UINT16		ret;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", 
	  hWaveOut, lpWaveOutHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->lpData = PTR_SEG_TO_LIN(lpWaveOutHdr->lpData);
    ret = wodMessage( lpDesc->uDeviceID, WODM_PREPARE, lpDesc->dwInstance, 
		      (DWORD)lpWaveOutHdr,uSize);
    lpWaveOutHdr->lpData = saveddata;
    return ret;
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[WINMM.181]
 */
UINT32 WINAPI waveOutUnprepareHeader32(HWAVEOUT32 hWaveOut,
                                       WAVEHDR * lpWaveOutHdr, UINT32 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", 
	  hWaveOut, lpWaveOutHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return wodMessage(lpDesc->uDeviceID,WODM_UNPREPARE,lpDesc->dwInstance, 
		      (DWORD)lpWaveOutHdr, uSize);
}
/**************************************************************************
 * 				waveOutUnprepareHeader	[MMSYSTEM.407]
 */
UINT16 WINAPI waveOutUnprepareHeader16(HWAVEOUT16 hWaveOut,
				       WAVEHDR * lpWaveOutHdr, UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    LPBYTE		saveddata = lpWaveOutHdr->lpData;
    UINT16		ret;
    
    TRACE(mmsys, "(%04X, %p, %u);\n", 
	  hWaveOut, lpWaveOutHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->lpData = PTR_SEG_TO_LIN(lpWaveOutHdr->lpData);
    ret = wodMessage(lpDesc->uDeviceID,WODM_UNPREPARE,lpDesc->dwInstance, 
		     (DWORD)lpWaveOutHdr, uSize);
    lpWaveOutHdr->lpData = saveddata;
    return ret;
}

/**************************************************************************
 * 				waveOutWrite		[MMSYSTEM.408]
 */
UINT32 WINAPI waveOutWrite32(HWAVEOUT32 hWaveOut, WAVEHDR * lpWaveOutHdr,
                             UINT32 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    lpWaveOutHdr->reserved = (DWORD)lpWaveOutHdr->lpData;
    return wodMessage( lpDesc->uDeviceID, WODM_WRITE, lpDesc->dwInstance, (DWORD)lpWaveOutHdr, uSize);
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
    lpWaveOutHdr->reserved=(DWORD)lpWaveOutHdr->lpData;/*save original ptr*/
    lpWaveOutHdr->lpData = PTR_SEG_TO_LIN(lpWaveOutHdr->lpData);
    ret = wodMessage( lpDesc->uDeviceID, WODM_WRITE, lpDesc->dwInstance, (DWORD)lpWaveOutHdr, uSize);
    lpWaveOutHdr->lpData = (LPBYTE)lpWaveOutHdr->reserved;
    return ret;
}

/**************************************************************************
 * 				waveOutPause		[WINMM.174]
 */
UINT32 WINAPI waveOutPause32(HWAVEOUT32 hWaveOut)
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
    return wodMessage( lpDesc->uDeviceID, WODM_PAUSE, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveOutRestart		[WINMM.177]
 */
UINT32 WINAPI waveOutRestart32(HWAVEOUT32 hWaveOut)
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
    return wodMessage( lpDesc->uDeviceID, WODM_RESTART, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveOutReset		[WINMM.176]
 */
UINT32 WINAPI waveOutReset32(HWAVEOUT32 hWaveOut)
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
    return wodMessage( lpDesc->uDeviceID, WODM_RESET, lpDesc->dwInstance, 0L, 0L);
}

/**************************************************************************
 * 				waveOutGetPosition	[WINMM.170]
 */
UINT32 WINAPI waveOutGetPosition32(HWAVEOUT32 hWaveOut, LPMMTIME32 lpTime,
                                   UINT32 uSize)
{
    MMTIME16	mmt16;
    UINT32 ret;
    
    mmt16.wType = lpTime->wType;
    ret = waveOutGetPosition16(hWaveOut,&mmt16,sizeof(mmt16));
    MMSYSTEM_MMTIME16to32(lpTime,&mmt16);
    return ret;
}
/**************************************************************************
 * 				waveOutGetPosition	[MMSYSTEM.412]
 */
UINT16 WINAPI waveOutGetPosition16(HWAVEOUT16 hWaveOut,LPMMTIME16 lpTime,
                                   UINT16 uSize)
{
    LPWAVEOPENDESC	lpDesc;
    TRACE(mmsys, "(%04X, %p, %u);\n", hWaveOut, lpTime, uSize);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    return wodMessage( lpDesc->uDeviceID, WODM_GETPOS, lpDesc->dwInstance, 
		       (DWORD)lpTime, (DWORD)uSize);
}

#define WAVEOUT_SHORTCUT_1(xx,XX,atype) \
	UINT32 WINAPI waveOut##xx##32(HWAVEOUT32 hWaveOut, atype x)	\
{									\
	return waveOut##xx##16(hWaveOut,x);				\
}									\
UINT16 WINAPI waveOut##xx##16(HWAVEOUT16 hWaveOut, atype x)		\
{									\
	LPWAVEOPENDESC	lpDesc;						\
	TRACE(mmsys, "waveOut"#xx"(%04X, %08lx);\n", hWaveOut,(DWORD)x);\
	lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveOut);		\
	if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;		\
	return wodMessage(lpDesc->uDeviceID, WODM_##XX, lpDesc->dwInstance,\
			  (DWORD)x, 0L);				\
}

WAVEOUT_SHORTCUT_1(GetPitch,GETPITCH,DWORD*)
WAVEOUT_SHORTCUT_1(SetPitch,SETPITCH,DWORD)
WAVEOUT_SHORTCUT_1(GetPlaybackRate,GETPLAYBACKRATE,DWORD*)
WAVEOUT_SHORTCUT_1(SetPlaybackRate,SETPLAYBACKRATE,DWORD)
    
#define WAVEOUT_SHORTCUT_2(xx,XX,atype) \
	UINT32 WINAPI waveOut##xx##32(UINT32 devid, atype x)		\
{									\
	return waveOut##xx##16(devid,x);				\
}									\
UINT16 WINAPI waveOut##xx##16(UINT16 devid, atype x)			\
{									\
	TRACE(mmsys, "waveOut"#xx"(%04X, %08lx);\n", devid,(DWORD)x);	\
	return wodMessage(devid, WODM_##XX, 0L,	(DWORD)x, 0L);		\
}
    
    
WAVEOUT_SHORTCUT_2(GetVolume,GETVOLUME,DWORD*)
WAVEOUT_SHORTCUT_2(SetVolume,SETVOLUME,DWORD)
    
    
/**************************************************************************
 * 				waveOutBreakLoop 	[MMSYSTEM.419]
 */
UINT32 WINAPI waveOutBreakLoop32(HWAVEOUT32 hWaveOut)
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
UINT32 WINAPI waveOutGetID32(HWAVEOUT32 hWaveOut, UINT32 * lpuDeviceID)
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
UINT16 WINAPI waveOutGetID16(HWAVEOUT16 hWaveOut, UINT16 * lpuDeviceID)
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
DWORD WINAPI waveOutMessage32(HWAVEOUT32 hWaveOut, UINT32 uMessage, 
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
	return waveOutWrite32(hWaveOut,(LPWAVEHDR)dwParam1,dwParam2);
    case WODM_GETDEVCAPS:
	/* FIXME: UNICODE/ANSI? */
	return waveOutGetDevCaps32A(hWaveOut,(LPWAVEOUTCAPS32A)dwParam1,dwParam2);
    case WODM_OPEN:
	FIXME(mmsys,"can't handle WODM_OPEN, please report.\n");
	break;
    default:
	ERR(mmsys,"(0x%04x,0x%04x,%08lx,%08lx): unhandled message\n",
	    hWaveOut,uMessage,dwParam1,dwParam2);
	break;
    }
    return wodMessage( lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
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
	return waveOutGetPosition16(hWaveOut,(LPMMTIME16)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WODM_GETVOLUME:
	return waveOutGetVolume16(hWaveOut,(LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case WODM_GETPITCH:
	return waveOutGetPitch16(hWaveOut,(LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case WODM_GETPLAYBACKRATE:
	return waveOutGetPlaybackRate16(hWaveOut,(LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case WODM_GETDEVCAPS:
	return waveOutGetDevCaps16(hWaveOut,(LPWAVEOUTCAPS16)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WODM_PREPARE:
	return waveOutPrepareHeader16(hWaveOut,(LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WODM_UNPREPARE:
	return waveOutUnprepareHeader16(hWaveOut,(LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WODM_WRITE:
	return waveOutWrite16(hWaveOut,(LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WODM_OPEN:
	FIXME(mmsys,"can't handle WODM_OPEN, please report.\n");
	break;
    default:
	ERR(mmsys,"(0x%04x,0x%04x,%08lx,%08lx): unhandled message\n",
	    hWaveOut,uMessage,dwParam1,dwParam2);
    }
    return wodMessage( lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				waveInGetNumDevs 		[WINMM.151]
 */
UINT32 WINAPI waveInGetNumDevs32()
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
UINT32 WINAPI waveInGetDevCaps32W(UINT32 uDeviceID, LPWAVEINCAPS32W lpCaps, UINT32 uSize)
{
    WAVEINCAPS16	wic16;
    UINT32	ret = waveInGetDevCaps16(uDeviceID,&wic16,uSize);
    
    lpCaps->wMid = wic16.wMid;
    lpCaps->wPid = wic16.wPid;
    lpCaps->vDriverVersion = wic16.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname,wic16.szPname);
    lpCaps->dwFormats = wic16.dwFormats;
    lpCaps->wChannels = wic16.wChannels;
    
    return ret;
}
/**************************************************************************
 * 				waveInGetDevCapsA 		[WINMM.146]
 */
UINT32 WINAPI waveInGetDevCaps32A(UINT32 uDeviceID, LPWAVEINCAPS32A lpCaps, UINT32 uSize)
{
    WAVEINCAPS16	wic16;
    UINT32	ret = waveInGetDevCaps16(uDeviceID,&wic16,uSize);
    
    lpCaps->wMid = wic16.wMid;
    lpCaps->wPid = wic16.wPid;
    lpCaps->vDriverVersion = wic16.vDriverVersion;
    strcpy(lpCaps->szPname,wic16.szPname);
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
UINT32 WINAPI waveInGetErrorText32A(UINT32 uError, LPSTR lpText, UINT32 uSize)
{
    TRACE(mmsys, "waveInGetErrorText\n");
    return(waveGetErrorText(uError, lpText, uSize));
}

/**************************************************************************
 * 				waveInGetErrorTextW 	[WINMM.149]
 */
UINT32 WINAPI waveInGetErrorText32W(UINT32 uError, LPWSTR lpText, UINT32 uSize)
{
    LPSTR txt = HeapAlloc(GetProcessHeap(),0,uSize);
    UINT32	ret = waveGetErrorText(uError, txt, uSize);
    
    lstrcpyAtoW(lpText,txt);
    HeapFree(GetProcessHeap(),0,txt);
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
UINT32 WINAPI waveInOpen32(HWAVEIN32 * lphWaveIn, UINT32 uDeviceID,
                           const LPWAVEFORMAT lpFormat, DWORD dwCallback,
                           DWORD dwInstance, DWORD dwFlags)
{
    HWAVEIN16	hwin16;
    UINT32	ret = waveInOpen16(&hwin16,uDeviceID,lpFormat,dwCallback,dwInstance,
				 CALLBACK32CONV(dwFlags));
    if (lphWaveIn) *lphWaveIn = hwin16;
    return ret;
}

/**************************************************************************
 * 				waveInOpen			[MMSYSTEM.504]
 */
UINT16 WINAPI waveInOpen16(HWAVEIN16 * lphWaveIn, UINT16 uDeviceID,
                           const LPWAVEFORMAT lpFormat, DWORD dwCallback,
                           DWORD dwInstance, DWORD dwFlags)
{
    HWAVEIN16 hWaveIn;
    LPWAVEOPENDESC	lpDesc;
    DWORD	dwRet = 0;
    BOOL32	bMapperFlg = FALSE;
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
UINT32 WINAPI waveInClose32(HWAVEIN32 hWaveIn)
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
UINT32 WINAPI waveInPrepareHeader32(HWAVEIN32 hWaveIn,
				    WAVEHDR * lpWaveInHdr, UINT32 uSize)
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
    return widMessage(lpDesc->uDeviceID,WIDM_PREPARE,lpDesc->dwInstance, 
		      (DWORD)lpWaveInHdr, uSize);
}
/**************************************************************************
 * 				waveInPrepareHeader		[MMSYSTEM.506]
 */
UINT16 WINAPI waveInPrepareHeader16(HWAVEIN16 hWaveIn,
				    WAVEHDR * lpWaveInHdr, UINT16 uSize)
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
    ret = widMessage(lpDesc->uDeviceID,WIDM_PREPARE,lpDesc->dwInstance, 
		     (DWORD)lpWaveInHdr,uSize);
    lpWaveInHdr->lpData = saveddata;
    return ret;
}


/**************************************************************************
 * 				waveInUnprepareHeader	[WINMM.159]
 */
UINT32 WINAPI waveInUnprepareHeader32(HWAVEIN32 hWaveIn,
                                      WAVEHDR * lpWaveInHdr, UINT32 uSize)
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
    return widMessage(lpDesc->uDeviceID,WIDM_UNPREPARE,lpDesc->dwInstance, 
		      (DWORD)lpWaveInHdr, uSize);
}
/**************************************************************************
 * 				waveInUnprepareHeader	[MMSYSTEM.507]
 */
UINT16 WINAPI waveInUnprepareHeader16(HWAVEIN16 hWaveIn,
                                      WAVEHDR * lpWaveInHdr, UINT16 uSize)
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
    return widMessage(lpDesc->uDeviceID,WIDM_UNPREPARE,lpDesc->dwInstance, 
		      (DWORD)lpWaveInHdr, uSize);
}

/**************************************************************************
 * 				waveInAddBuffer		[WINMM.144]
 */
UINT32 WINAPI waveInAddBuffer32(HWAVEIN32 hWaveIn,
                                WAVEHDR * lpWaveInHdr, UINT32 uSize)
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
                                WAVEHDR * lpWaveInHdr, UINT16 uSize)
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
UINT32 WINAPI waveInStart32(HWAVEIN32 hWaveIn)
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
    return widMessage(lpDesc->uDeviceID,WIDM_START,lpDesc->dwInstance,0,0);
}

/**************************************************************************
 * 				waveInStop			[WINMM.158]
 */
UINT32 WINAPI waveInStop32(HWAVEIN32 hWaveIn)
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
UINT32 WINAPI waveInReset32(HWAVEIN32 hWaveIn)
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
    return widMessage(lpDesc->uDeviceID,WIDM_RESET,lpDesc->dwInstance,0,0);
}

/**************************************************************************
 * 				waveInGetPosition	[WINMM.152]
 */
UINT32 WINAPI waveInGetPosition32(HWAVEIN32 hWaveIn, LPMMTIME32 lpTime,
                                  UINT32 uSize)
{
    MMTIME16	mmt16;
    UINT32	ret = waveInGetPosition16(hWaveIn,&mmt16,uSize);
    
    MMSYSTEM_MMTIME16to32(lpTime,&mmt16);
    return ret;
}

/**************************************************************************
 * 				waveInGetPosition	[MMSYSTEM.512]
 */
UINT16 WINAPI waveInGetPosition16(HWAVEIN16 hWaveIn,LPMMTIME16 lpTime,
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
UINT32 WINAPI waveInGetID32(HWAVEIN32 hWaveIn, UINT32 * lpuDeviceID)
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
UINT16 WINAPI waveInGetID16(HWAVEIN16 hWaveIn, UINT16 * lpuDeviceID)
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
DWORD WINAPI waveInMessage32(HWAVEIN32 hWaveIn, UINT32 uMessage,
                             DWORD dwParam1, DWORD dwParam2)
{
    LPWAVEOPENDESC	lpDesc;
    
    FIXME(mmsys, "(%04X, %04X, %08lX, %08lX),FIXME!\n", 
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
    case WIDM_STOP :
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
	return waveInGetDevCaps32A(hWaveIn,(LPWAVEINCAPS32A)dwParam1,dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hWaveIn,uMessage,dwParam1,dwParam2);
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
    
    FIXME(mmsys, "(%04X, %04X, %08lX, %08lX),FIXME!\n", 
	  hWaveIn, uMessage, dwParam1, dwParam2);
    lpDesc = (LPWAVEOPENDESC) USER_HEAP_LIN_ADDR(hWaveIn);
    if (lpDesc == NULL) return MMSYSERR_INVALHANDLE;
    switch (uMessage) {
    case WIDM_OPEN:
	FIXME(mmsys,"cannot handle WIDM_OPEN, please report.\n");
	break;
    case WIDM_GETNUMDEVS:
    case WIDM_CLOSE:
    case WIDM_STOP :
    case WIDM_RESET:
    case WIDM_START:
    case WIDM_PAUSE:
	/* no argument conversion needed */
	break;
    case WIDM_GETDEVCAPS:
	return waveInGetDevCaps16(hWaveIn,(LPWAVEINCAPS16)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WIDM_GETPOS:
	return waveInGetPosition16(hWaveIn,(LPMMTIME16)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WIDM_PREPARE:
	return waveInPrepareHeader16(hWaveIn,(LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WIDM_UNPREPARE:
	return waveInUnprepareHeader16(hWaveIn,(LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    case WIDM_ADDBUFFER:
	return waveInAddBuffer16(hWaveIn,(LPWAVEHDR)PTR_SEG_TO_LIN(dwParam1),dwParam2);
    default:
	ERR(mmsys,"(%04x,%04x,%08lx,%08lx): unhandled message\n",
	    hWaveIn,uMessage,dwParam1,dwParam2);
	break;
    }
    return widMessage(lpDesc->uDeviceID, uMessage, lpDesc->dwInstance, dwParam1, dwParam2);
}

/**************************************************************************
 * 				DrvOpen	       		[MMSYSTEM.1100]
 */
HDRVR16 WINAPI DrvOpen(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
    TRACE(mmsys,"('%s','%s',%08lX);\n",lpDriverName,lpSectionName,lParam);
    return OpenDriver16(lpDriverName, lpSectionName, lParam);
}


/**************************************************************************
 * 				DrvClose       		[MMSYSTEM.1101]
 */
LRESULT WINAPI DrvClose(HDRVR16 hDrvr, LPARAM lParam1, LPARAM lParam2)
{
    TRACE(mmsys, "(%04X, %08lX, %08lX);\n", hDrvr, lParam1, lParam2);
    return CloseDriver16(hDrvr, lParam1, lParam2);
}


/**************************************************************************
 * 				DrvSendMessage		[MMSYSTEM.1102]
 */
LRESULT WINAPI DrvSendMessage(HDRVR16 hDriver, WORD msg, LPARAM lParam1,
                              LPARAM lParam2)
{
    DWORD 	dwDriverID = 0;
    FIXME(mmsys, "(%04X, %04X, %08lX, %08lX);\n",
	  hDriver, msg, lParam1, lParam2);
    /* FIXME: wrong ... */
    return CDAUDIO_DriverProc16(dwDriverID, hDriver, msg, lParam1, lParam2);
}

/**************************************************************************
 * 				DrvGetModuleHandle	[MMSYSTEM.1103]
 */
HANDLE16 WINAPI DrvGetModuleHandle16(HDRVR16 hDrvr)
{
    return GetDriverModuleHandle16(hDrvr);
}

/**************************************************************************
 * 				DrvDefDriverProc	[MMSYSTEM.1104]
 */
LRESULT WINAPI DrvDefDriverProc(DWORD dwDriverID, HDRVR16 hDriv, WORD wMsg, 
                                DWORD dwParam1, DWORD dwParam2)
{
    return DefDriverProc16(dwDriverID, hDriv, wMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				DefDriverProc32  [WINMM.5]
 */
LRESULT WINAPI DefDriverProc32(DWORD dwDriverIdentifier, HDRVR32 hdrvr,
                               UINT32 Msg, LPARAM lParam1, LPARAM lParam2)
{
    switch (Msg) {
      case DRV_LOAD:
      case DRV_DISABLE:
      case DRV_INSTALL:
        return 0;
      case DRV_ENABLE:
      case DRV_FREE:
      case DRV_REMOVE:
        return 1;
      default:
        return 0;
    }
}

/**************************************************************************
 * 				mmThreadCreate		[MMSYSTEM.1120]
 */
LRESULT WINAPI mmThreadCreate16(LPVOID x1, LPWORD x2, DWORD x3, DWORD x4) {
    FIXME(mmsys,"(%p,%p,%08lx,%08lx): stub!\n",x1,x2,x3,x4);
    *x2 = 0xbabe;
    return 0;
}

/**************************************************************************
 * 				mmThreadGetTask		[MMSYSTEM.1125]
 */
LRESULT WINAPI mmThreadGetTask16(WORD hnd) {
    FIXME(mmsys,"(%04x): stub!\n",hnd);
    return GetCurrentTask();
}

/**************************************************************************
 * 				mmThreadSignal		[MMSYSTEM.1121]
 */
LRESULT WINAPI mmThreadSignal16(WORD hnd) {
    FIXME(mmsys,"(%04x): stub!\n",hnd);
    return 0;
}

/**************************************************************************
 * 				mmTaskCreate		[MMSYSTEM.900]
 */
HINSTANCE16 WINAPI mmTaskCreate16(LPWORD lphnd,HINSTANCE16 *hMmTask,DWORD x2)
{
    DWORD showCmd = 0x40002;
    LPSTR cmdline;
    WORD sel1, sel2;
    LOADPARAMS *lp;
    HINSTANCE16 ret, handle;
    
    TRACE(mmsys,"(%p,%p,%08lx);\n",lphnd,hMmTask,x2);
    cmdline = (LPSTR)HeapAlloc(GetProcessHeap(), 0, 0x0d);
    cmdline[0] = 0x0d;
    (DWORD)cmdline[1] = (DWORD)lphnd;
    (DWORD)cmdline[5] = x2;
    (DWORD)cmdline[9] = 0;
    
    sel1 = SELECTOR_AllocBlock(cmdline, 0x0d, SEGMENT_DATA, FALSE, FALSE);
    sel2 = SELECTOR_AllocBlock(&showCmd, sizeof(showCmd),
			       SEGMENT_DATA, FALSE, FALSE);
    
    lp = (LOADPARAMS *)HeapAlloc(GetProcessHeap(), 0, sizeof(LOADPARAMS));
    lp->hEnvironment = 0;
    lp->cmdLine = PTR_SEG_OFF_TO_SEGPTR(sel1, 0);
    lp->showCmd = PTR_SEG_OFF_TO_SEGPTR(sel2, 0);
    lp->reserved = 0;
    
    ret = LoadModule16("c:\\windows\\mmtask.tsk", lp);
    if (ret < 32) {
	if (ret)
	    ret = 1;
	else
	    ret = 2;
	handle = 0;
    }
    else {
	handle = ret;
	ret = 0;
    }
    if (hMmTask)
	*(HINSTANCE16 *)PTR_SEG_TO_LIN(hMmTask) = handle;
    
    UnMapLS(PTR_SEG_OFF_TO_SEGPTR(sel2, 0));
    UnMapLS(PTR_SEG_OFF_TO_SEGPTR(sel1, 0));
    
    HeapFree(GetProcessHeap(), 0, lp);
    HeapFree(GetProcessHeap(), 0, cmdline);
    
    return ret;
}

/**************************************************************************
 * 				mmTaskSignal		[MMSYSTEM.903]
 */
LRESULT WINAPI mmTaskSignal16(HTASK16 ht) 
{
    TRACE(mmsys,"(%04x);\n",ht);
    return PostAppMessage16(ht,WM_USER,0,0);
}

/**************************************************************************
 * 				mciDriverYield		[MMSYSTEM.710]
 */
LRESULT WINAPI mciDriverYield16(HANDLE16 hnd) 
{
    FIXME(mmsys,"(%04x): stub!\n",hnd);
    return 0;
}

