/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM functions
 *
 * Copyright 1993 Martin Ayotte
 */

/* 
 * Eric POUECH : 
 * 	98/9	added Win32 MCI support
 *  	99/4	added mmTask and mmThread functions support
 *		added midiStream support
 *      99/9	added support for loadable low level drivers
 */

/* FIXME: I think there are some segmented vs. linear pointer weirdnesses 
 *        and long term pointers to 16 bit space in here
 */

#include <string.h>

#include "winbase.h"
#include "wine/winbase16.h"
#include "wownt32.h"
#include "heap.h"
#include "driver.h"
#include "winemm.h"
#include "syslevel.h"
#include "callback.h"
#include "selectors.h"
#include "module.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mmsys)

LONG   WINAPI DrvDefDriverProc(DWORD dwDevID, HDRVR16 hDrv, WORD wMsg, 
			       DWORD dwParam1, DWORD dwParam2);

static LPWINE_MM_IDATA		lpFirstIData = NULL;

static	LPWINE_MM_IDATA	MULTIMEDIA_GetIDataNoCheck(void)
{
    DWORD		pid = GetCurrentProcessId();
    LPWINE_MM_IDATA	iData;

    for (iData = lpFirstIData; iData; iData = iData->lpNextIData) {
	if (iData->dwThisProcess == pid)
	    break;
    }
    return iData;
}

/**************************************************************************
 * 			MULTIMEDIA_GetIData			[internal]
 */
LPWINE_MM_IDATA	MULTIMEDIA_GetIData(void)
{
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIDataNoCheck();

    if (!iData) {
	ERR("IData not found. Suicide !!!\n");
	ExitProcess(0);
    }
    return iData;
}

/**************************************************************************
 * 			MULTIMEDIA_CreateIData			[internal]
 */
static	BOOL	MULTIMEDIA_CreateIData(HINSTANCE hInstDLL)
{
    LPWINE_MM_IDATA	iData;
	
    iData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MM_IDATA));

    if (!iData)
	return FALSE;
    iData->hWinMM32Instance = hInstDLL;
    iData->dwThisProcess = GetCurrentProcessId();
    iData->lpNextIData = lpFirstIData;
    lpFirstIData = iData;
    InitializeCriticalSection(&iData->cs);
    return TRUE;
}

/**************************************************************************
 * 			MULTIMEDIA_DeleteIData			[internal]
 */
static	void MULTIMEDIA_DeleteIData(void)
{
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIDataNoCheck();
    LPWINE_MM_IDATA*	ppid;
	    
    if (iData) {
	for (ppid = &lpFirstIData; *ppid; ppid = &(*ppid)->lpNextIData) {
	    if (*ppid == iData) {
		*ppid = iData->lpNextIData;
		break;
	    }
	}
	/* FIXME: should also free content and resources allocated 
	 * inside iData */
	HeapFree(GetProcessHeap(), 0, iData);
    }
}

/**************************************************************************
 * 			WINMM_LibMain				[EntryPoint]
 *
 * WINMM DLL entry point
 *
 */
BOOL WINAPI WINMM_LibMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    static BOOL     		bInitDone = FALSE;

    TRACE("0x%x 0x%lx %p\n", hInstDLL, fdwReason, fImpLoad);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
	if (!MULTIMEDIA_CreateIData(hInstDLL))
	    return FALSE;
	if (!bInitDone) { /* to be done only once */
	    if (!MULTIMEDIA_MciInit() || !MMDRV_Init()) {
		MULTIMEDIA_DeleteIData();
		return FALSE;
	    }
	    bInitDone = TRUE;	
	}
	break;
    case DLL_PROCESS_DETACH:
	MULTIMEDIA_DeleteIData();
	break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
	break;
    }
    return TRUE;
}

/**************************************************************************
 * 			MMSYSTEM_LibMain			[EntryPoint]
 *
 * MMSYSTEM DLL entry point
 *
 */
BOOL WINAPI MMSYSTEM_LibMain(DWORD fdwReason, HINSTANCE hinstDLL, WORD ds, 
			     WORD wHeapSize, DWORD dwReserved1, WORD wReserved2)
{
    HANDLE			hndl;
    LPWINE_MM_IDATA		iData;

    TRACE("0x%x 0x%lx\n", hinstDLL, fdwReason);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
	/* need to load WinMM in order to:
	 * - correctly initiates shared variables (MULTIMEDIA_Init())
	 * - correctly creates the per process WINE_MM_IDATA chunk
	 */
	hndl = LoadLibraryA("WINMM.DLL");
	
	if (!hndl) {
	    ERR("Could not load sibling WinMM.dll\n");
	    return FALSE;
	}
	iData = MULTIMEDIA_GetIData();
	iData->hWinMM16Instance = hinstDLL;
	iData->h16Module32 = hndl;
	break;
    case DLL_PROCESS_DETACH:
	iData = MULTIMEDIA_GetIData();
	FreeLibrary(iData->h16Module32);
	break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
	break;
    }
    return TRUE;
}

/**************************************************************************
 * 				MMSYSTEM_WEP			[MMSYSTEM.1]
 */
int WINAPI MMSYSTEM_WEP(HINSTANCE16 hInstance, WORD wDataSeg,
                        WORD cbHeapSize, LPSTR lpCmdLine)
{
    FIXME("STUB: Unloading MMSystem DLL ... hInst=%04X \n", hInstance);
    return TRUE;
}

void MMSYSTEM_MMTIME32to16(LPMMTIME16 mmt16, const MMTIME* mmt32) 
{
    mmt16->wType = mmt32->wType;
    /* layout of rest is the same for 32/16,
     * Note: mmt16->u is 2 bytes smaller than mmt32->u, which has padding
     */
    memcpy(&(mmt16->u), &(mmt32->u), sizeof(mmt16->u));
}

void MMSYSTEM_MMTIME16to32(LPMMTIME mmt32, const MMTIME16* mmt16) 
{
    mmt32->wType = mmt16->wType;
    /* layout of rest is the same for 32/16,
     * Note: mmt16->u is 2 bytes smaller than mmt32->u, which has padding
     */
    memcpy(&(mmt32->u), &(mmt16->u), sizeof(mmt16->u));
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

static HMMIO	get_mmioFromFile(LPCSTR lpszName)
{
    return mmioOpenA((LPSTR)lpszName, NULL,
		     MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
}

static HMMIO	get_mmioFromProfile(UINT uFlags, LPCSTR lpszName) 
{
    char	str[128];
    LPSTR	ptr;
    HMMIO  	hmmio;
    
    TRACE("searching in SystemSound List !\n");
    GetProfileStringA("Sounds", (LPSTR)lpszName, "", str, sizeof(str));
    if (strlen(str) == 0) {
	if (uFlags & SND_NODEFAULT) return 0;
	GetProfileStringA("Sounds", "Default", "", str, sizeof(str));
	if (strlen(str) == 0) return 0;
    }
    if ((ptr = (LPSTR)strchr(str, ',')) != NULL) *ptr = '\0';
    hmmio = get_mmioFromFile(str);
    if (hmmio == 0) {
	WARN("can't find SystemSound='%s' !\n", str);
	return 0;
    }
    return hmmio;
}

struct playsound_data {
    HANDLE	hEvent;
    DWORD	dwEventCount;
};

static void CALLBACK PlaySound_Callback(HWAVEOUT hwo, UINT uMsg, 
					DWORD dwInstance,  
					DWORD dwParam1, DWORD dwParam2)
{
    struct playsound_data*	s = (struct playsound_data*)dwInstance;

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	break;
    case WOM_DONE:
	InterlockedIncrement(&s->dwEventCount);
	TRACE("Returning waveHdr=%lx\n", dwParam1);
	SetEvent(s->hEvent);
	break;
    default:
	ERR("Unknown uMsg=%d\n", uMsg);
    }
}

static void PlaySound_WaitDone(struct playsound_data* s) 
{
    for (;;) {
	ResetEvent(s->hEvent);
	if (InterlockedDecrement(&s->dwEventCount) >= 0) {
	    break;
	}
	InterlockedIncrement(&s->dwEventCount);
	
	WaitForSingleObject(s->hEvent, INFINITE);
    }
}

static BOOL WINAPI proc_PlaySound(LPCSTR lpszSoundName, UINT uFlags)
{
    BOOL		bRet = FALSE;
    HMMIO		hmmio = 0;
    MMCKINFO		ckMainRIFF;
    MMCKINFO        	mmckInfo;
    LPWAVEFORMATEX      lpWaveFormat = NULL;
    HWAVE		hWave = 0;
    LPWAVEHDR		waveHdr = NULL;
    INT			count, bufsize, left, index;
    struct playsound_data	s;

    s.hEvent = 0;

    TRACE("SoundName='%s' uFlags=%04X !\n", lpszSoundName, uFlags);
    if (lpszSoundName == NULL) {
	TRACE("Stop !\n");
	return FALSE;
    }
    if (uFlags & SND_MEMORY) {
	MMIOINFO	mminfo;
	memset(&mminfo, 0, sizeof(mminfo));
	mminfo.fccIOProc = FOURCC_MEM;
	mminfo.pchBuffer = (LPSTR)lpszSoundName;
	mminfo.cchBuffer = -1;
	TRACE("Memory sound %p\n", lpszSoundName);
	hmmio = mmioOpenA(NULL, &mminfo, MMIO_READ);
    } else {
	hmmio = 0;
	if (uFlags & SND_ALIAS)
	    if ((hmmio = get_mmioFromProfile(uFlags, lpszSoundName)) == 0) 
		return FALSE;
	
	if (uFlags & SND_FILENAME)
	    if ((hmmio=get_mmioFromFile(lpszSoundName)) == 0) return FALSE;
	
	if (PlaySound_SearchMode == 1) {
	    PlaySound_SearchMode = 0;
	    if ((hmmio = get_mmioFromFile(lpszSoundName)) == 0) 
		hmmio = get_mmioFromProfile(uFlags, lpszSoundName);
	}
	
	if (PlaySound_SearchMode == 2) {
	    PlaySound_SearchMode = 0;
	    if ((hmmio = get_mmioFromProfile(uFlags | SND_NODEFAULT, lpszSoundName)) == 0) 
		if ((hmmio = get_mmioFromFile(lpszSoundName)) == 0)	
		    hmmio = get_mmioFromProfile(uFlags, lpszSoundName);
	}
    }
    if (hmmio == 0) return FALSE;

    if (mmioDescend(hmmio, &ckMainRIFF, NULL, 0))
	goto errCleanUp;

    TRACE("ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX \n",
	  (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, ckMainRIFF.cksize);

    if ((ckMainRIFF.ckid != FOURCC_RIFF) ||
	(ckMainRIFF.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
	goto errCleanUp;

    mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK))
	goto errCleanUp;

    TRACE("Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX \n",
	  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);

    lpWaveFormat = HeapAlloc(GetProcessHeap(), 0, mmckInfo.cksize);
    if (mmioRead(hmmio, (HPSTR)lpWaveFormat, mmckInfo.cksize) < sizeof(WAVEFORMAT))
	goto errCleanUp;

    TRACE("wFormatTag=%04X !\n", 	lpWaveFormat->wFormatTag);
    TRACE("nChannels=%d \n", 		lpWaveFormat->nChannels);
    TRACE("nSamplesPerSec=%ld\n", 	lpWaveFormat->nSamplesPerSec);
    TRACE("nAvgBytesPerSec=%ld\n", 	lpWaveFormat->nAvgBytesPerSec);
    TRACE("nBlockAlign=%d \n", 		lpWaveFormat->nBlockAlign);
    TRACE("wBitsPerSample=%u !\n", 	lpWaveFormat->wBitsPerSample);

    /* move to end of 'fmt ' chunk */
    mmioAscend(hmmio, &mmckInfo, 0);

    mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(hmmio, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK))
	goto errCleanUp;

    TRACE("Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX\n", 
	  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);

    s.hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

    if (waveOutOpen(&hWave, WAVE_MAPPER, lpWaveFormat, (DWORD)PlaySound_Callback,
		    (DWORD)&s, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	goto errCleanUp;

    /* make it so that 3 buffers per second are needed */
    bufsize = (((lpWaveFormat->nAvgBytesPerSec / 3) - 1) / lpWaveFormat->nBlockAlign + 1) *
	lpWaveFormat->nBlockAlign;
    waveHdr = HeapAlloc(GetProcessHeap(), 0, 2 * sizeof(WAVEHDR) + 2 * bufsize);
    waveHdr[0].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR);
    waveHdr[1].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR) + bufsize;
    waveHdr[0].dwUser = waveHdr[1].dwUser = 0L;
    waveHdr[0].dwLoops = waveHdr[1].dwLoops = 0L;
    waveHdr[0].dwFlags = waveHdr[1].dwFlags = 0L;
    waveHdr[0].dwBufferLength = waveHdr[1].dwBufferLength = bufsize;
    if (waveOutPrepareHeader(hWave, &waveHdr[0], sizeof(WAVEHDR)) || 
	waveOutPrepareHeader(hWave, &waveHdr[1], sizeof(WAVEHDR))) {
	goto errCleanUp;
    }

    do {
	index = 0;
	left = mmckInfo.cksize;
	s.dwEventCount = 1L; /* for first buffer */

	mmioSeek(hmmio, mmckInfo.dwDataOffset, SEEK_SET);
	while (left) {
	    if (PlaySound_Stop) {
		PlaySound_Stop = PlaySound_Loop = FALSE;
		break;
	    }
	    count = mmioRead(hmmio, waveHdr[index].lpData, min(bufsize, left));
	    if (count < 1) break;
	    left -= count;
	    waveHdr[index].dwBufferLength = count;
	    waveHdr[index].dwFlags &= ~WHDR_DONE;
	    waveOutWrite(hWave, &waveHdr[index], sizeof(WAVEHDR));
	    index ^= 1;
	    PlaySound_WaitDone(&s);
	}
	bRet = TRUE;
    } while (PlaySound_Loop);

    PlaySound_WaitDone(&s);
    waveOutReset(hWave);

    waveOutUnprepareHeader(hWave, &waveHdr[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(hWave, &waveHdr[1], sizeof(WAVEHDR));

errCleanUp:
    CloseHandle(s.hEvent);
    HeapFree(GetProcessHeap(), 0, waveHdr);
    HeapFree(GetProcessHeap(), 0, lpWaveFormat);
    if (hWave)		while (waveOutClose(hWave) == WAVERR_STILLPLAYING) Sleep(100);
    if (hmmio) 		mmioClose(hmmio, 0);

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
	PlaySound_Result = proc_PlaySound(PlaySound_pszSound, (UINT16)PlaySound_fdwSound);
    }
}

/**************************************************************************
 * 				PlaySoundA		[WINMM.1]
 */
BOOL WINAPI PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
    static LPSTR StrDup = NULL;
    
    TRACE("pszSound='%p' hmod=%04X fdwSound=%08lX\n",
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
 * 				PlaySound16		[MMSYSTEM.3]
 */
BOOL16 WINAPI PlaySound16(LPCSTR pszSound, HMODULE16 hmod, DWORD fdwSound)
{
    BOOL16 retv;

    SYSLEVEL_ReleaseWin16Lock();
    retv = PlaySoundA( pszSound, hmod, fdwSound );
    SYSLEVEL_RestoreWin16Lock();

    return retv;
}

/**************************************************************************
 * 				sndPlaySoundA		[WINMM135]
 */
BOOL WINAPI sndPlaySoundA(LPCSTR lpszSoundName, UINT uFlags)
{
    PlaySound_SearchMode = 1;
    return PlaySoundA(lpszSoundName, 0, uFlags);
}

/**************************************************************************
 * 				sndPlaySoundW		[WINMM.136]
 */
BOOL WINAPI sndPlaySoundW(LPCWSTR lpszSoundName, UINT uFlags)
{
    PlaySound_SearchMode = 1;
    return PlaySoundW(lpszSoundName, 0, uFlags);
}

/**************************************************************************
 * 				sndPlaySound16		[MMSYSTEM.2]
 */
BOOL16 WINAPI sndPlaySound16(LPCSTR lpszSoundName, UINT16 uFlags)
{
    BOOL16 retv;

    SYSLEVEL_ReleaseWin16Lock();
    retv = sndPlaySoundA( lpszSoundName, uFlags );
    SYSLEVEL_RestoreWin16Lock();

    return retv;
}


/**************************************************************************
 * 				mmsystemGetVersion	[WINMM.134]
 */
UINT WINAPI mmsystemGetVersion(void)
{
    return mmsystemGetVersion16();
}

/**************************************************************************
 * 				mmsystemGetVersion	[MMSYSTEM.5]
 * return value borrowed from Win95 winmm.dll ;)
 */
UINT16 WINAPI mmsystemGetVersion16(void)
{
    TRACE("3.10 (Win95?)\n");
    return 0x030a;
}

/**************************************************************************
 * 				DriverProc			[MMSYSTEM.6]
 */
LRESULT WINAPI DriverProc16(DWORD dwDevID, HDRVR16 hDrv, WORD wMsg, 
			    DWORD dwParam1, DWORD dwParam2)
{
    TRACE("dwDevID=%08lx hDrv=%04x wMsg=%04x dwParam1=%08lx dwParam2=%08lx\n",
	  dwDevID, hDrv, wMsg, dwParam1, dwParam2);

    return DrvDefDriverProc(dwDevID, hDrv, wMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				DriverCallback			[MMSYSTEM.31]
 */
BOOL WINAPI DriverCallback(DWORD dwCallBack, UINT uFlags, HDRVR hDev, 
			   UINT wMsg, DWORD dwUser, DWORD dwParam1, 
			   DWORD dwParam2)
{
    TRACE("(%08lX, %04X, %04X, %04X, %08lX, %08lX, %08lX); !\n",
	  dwCallBack, uFlags, hDev, wMsg, dwUser, dwParam1, dwParam2);

    switch (uFlags & DCB_TYPEMASK) {
    case DCB_NULL:
	TRACE("Null !\n");
	if (dwCallBack)
	    WARN("uFlags=%04X has null DCB value, but dwCallBack=%08lX is not null !\n", uFlags, dwCallBack);
	break;
    case DCB_WINDOW:
	TRACE("Window(%04lX) handle=%04X!\n", dwCallBack, hDev);
	if (!IsWindow(dwCallBack))
	    return FALSE;
	Callout.PostMessageA((HWND16)dwCallBack, wMsg, hDev, dwParam1);
	break;
    case DCB_TASK: /* aka DCB_THREAD */
	TRACE("Task(%04lx) !\n", dwCallBack);
	Callout.PostThreadMessageA(dwCallBack, wMsg, hDev, dwParam1);
	break;
    case DCB_FUNCTION:
	TRACE("Function (32 bit) !\n");
	((LPDRVCALLBACK)dwCallBack)(hDev, wMsg, dwUser, dwParam1, dwParam2);
	break;
    case DCB_EVENT:
	TRACE("Event(%08lx) !\n", dwCallBack);
	SetEvent((HANDLE)dwCallBack);
	break;
    case 6: /* I would dub it DCB_MMTHREADSIGNAL */
	/* this is an undocumented DCB_ value used for mmThreads
	 * loword of dwCallBack contains the handle of the lpMMThd block
	 * which dwSignalCount has to be incremented
	 */
	{
	    WINE_MMTHREAD*	lpMMThd = (WINE_MMTHREAD*)PTR_SEG_OFF_TO_LIN(LOWORD(dwCallBack), 0);

	    TRACE("mmThread (%04x, %p) !\n", LOWORD(dwCallBack), lpMMThd);
	    /* same as mmThreadSignal16 */
	    InterlockedIncrement(&lpMMThd->dwSignalCount);
	    SetEvent(lpMMThd->hEvent);
	    /* some other stuff on lpMMThd->hVxD */
	}
	break;	
#if 0
    case 4:
	/* this is an undocumented DCB_ value for... I don't know */
	break;
#endif
    default:
	WARN("Unknown callback type %d\n", uFlags & DCB_TYPEMASK);
	return FALSE;
    }
    TRACE("Done\n");
    return TRUE;
}

/**************************************************************************
 * 				DriverCallback			[MMSYSTEM.31]
 */
BOOL16 WINAPI DriverCallback16(DWORD dwCallBack, UINT16 uFlags, HDRVR16 hDev, 
			       WORD wMsg, DWORD dwUser, DWORD dwParam1, 
			       DWORD dwParam2)
{
    return DriverCallback(dwCallBack, uFlags, hDev, wMsg, dwUser, dwParam1, dwParam2);
}

/**************************************************************************
 * 	Mixer devices. New to Win95
 */

/**************************************************************************
 * find out the real mixer ID depending on hmix (depends on dwFlags)
 */
static LPWINE_MIXER MIXER_GetDev(HMIXEROBJ hmix, DWORD dwFlags) 
{
    LPWINE_MIXER	lpwm = NULL;

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
	FIXME("Unsupported flag (%08lx)\n", dwFlags & 0xF0000000ul);
	break;
    }
    return lpwm;
}

/**************************************************************************
 * 				mixerGetNumDevs			[WINMM.108]
 */
UINT WINAPI mixerGetNumDevs(void) 
{
    return MMDRV_GetNum(MMDRV_MIXER);
}

/**************************************************************************
 * 				mixerGetNumDevs			[MMSYSTEM.800]
 */
UINT16 WINAPI mixerGetNumDevs16(void) 
{
    return MMDRV_GetNum(MMDRV_MIXER);
}

/**************************************************************************
 * 				mixerGetDevCapsA		[WINMM.101]
 */
UINT WINAPI mixerGetDevCapsA(UINT devid, LPMIXERCAPSA mixcaps, UINT size) 
{
    LPWINE_MLD	wmld;

    if ((wmld = MMDRV_Get(devid, MMDRV_MIXER, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, MXDM_GETDEVCAPS, (DWORD)mixcaps, size, TRUE);
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
UINT16 WINAPI mixerGetDevCaps16(UINT16 devid, LPMIXERCAPS16 mixcaps, 
				UINT16 size) 
{
    MIXERCAPSA  micA;
    UINT        ret = mixerGetDevCapsA(devid, &micA, sizeof(micA));
    
    if (ret == MMSYSERR_NOERROR) {
        mixcaps->wMid           = micA.wMid;
        mixcaps->wPid           = micA.wPid;
        mixcaps->vDriverVersion = micA.vDriverVersion;
        strcpy(mixcaps->szPname, micA.szPname);
        mixcaps->fdwSupport     = micA.fdwSupport;
        mixcaps->cDestinations  = micA.cDestinations;
    }
    return ret;
}

static	UINT  MMSYSTEM_mixerOpen(LPHMIXER lphMix, UINT uDeviceID, DWORD dwCallback,
				 DWORD dwInstance, DWORD fdwOpen, BOOL bFrom32) 
{
    HANDLE		hMix;
    LPWINE_MLD		wmld;
    DWORD		dwRet = 0;
    MIXEROPENDESC	mod;

    TRACE("(%p, %d, %08lx, %08lx, %08lx)\n",
	  lphMix, uDeviceID, dwCallback, dwInstance, fdwOpen);

    wmld = MMDRV_Alloc(sizeof(WINE_MIXER), MMDRV_MIXER, &hMix, &fdwOpen,
		       &dwCallback, &dwInstance, bFrom32);

    wmld->uDeviceID = uDeviceID;
    mod.hmx = hMix;
    mod.dwCallback = dwCallback;
    mod.dwInstance = dwInstance;

    dwRet = MMDRV_Open(wmld, MXDM_OPEN, (DWORD)&mod, fdwOpen);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMix, wmld);
	hMix = 0;
    }
    if (lphMix) *lphMix = hMix;
    TRACE("=> %ld hMixer=%04x\n", dwRet, hMix);

    return dwRet;
}

/**************************************************************************
 * 				mixerOpen			[WINMM.110]
 */
UINT WINAPI mixerOpen(LPHMIXER lphMix, UINT uDeviceID, DWORD dwCallback,
		      DWORD dwInstance, DWORD fdwOpen) 
{
    return MMSYSTEM_mixerOpen(lphMix, uDeviceID, 
			      dwCallback, dwInstance, fdwOpen, TRUE);
}

/**************************************************************************
 * 				mixerOpen			[MMSYSTEM.803]
 */
UINT16 WINAPI mixerOpen16(LPHMIXER16 lphmix, UINT16 uDeviceID, DWORD dwCallback,
			  DWORD dwInstance, DWORD fdwOpen) 
{
    HMIXER	hmix;
    UINT	ret;
    
    ret = MMSYSTEM_mixerOpen(&hmix, uDeviceID, 
			     dwCallback, dwInstance, fdwOpen, FALSE);
    if (lphmix) *lphmix = hmix;
    return ret;
}

/**************************************************************************
 * 				mixerClose			[WINMM.98]
 */
UINT WINAPI mixerClose(HMIXER hMix) 
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;    
    
    TRACE("(%04x)\n", hMix);

    if ((wmld = MMDRV_Get(hMix, MMDRV_MIXER, FALSE)) == NULL) return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MXDM_CLOSE);
    MMDRV_Free(hMix, wmld);

    return dwRet;
}

/**************************************************************************
 * 				mixerClose			[MMSYSTEM.803]
 */
UINT16 WINAPI mixerClose16(HMIXER16 hMix) 
{
    return mixerClose(hMix);
}

/**************************************************************************
 * 				mixerGetID			[WINMM.103]
 */
UINT WINAPI mixerGetID(HMIXEROBJ hmix, LPUINT lpid, DWORD fdwID) 
{
    LPWINE_MIXER	lpwm;

    TRACE("(%04x %p %08lx)\n", hmix, lpid, fdwID);

    if ((lpwm = MIXER_GetDev(hmix, fdwID)) == NULL) {
	return MMSYSERR_INVALHANDLE;
    }

    if (lpid)
      *lpid = lpwm->mld.uDeviceID;

    return MMSYSERR_NOERROR;
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
 * 				mixerGetControlDetailsA		[WINMM.99]
 */
UINT WINAPI mixerGetControlDetailsA(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcdA,
				    DWORD fdwDetails) 
{
    LPWINE_MIXER	lpwm;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmcdA, fdwDetails);

    if ((lpwm = MIXER_GetDev(hmix, fdwDetails)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    if (lpmcdA == NULL || lpmcdA->cbStruct != sizeof(*lpmcdA))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(&lpwm->mld, MXDM_GETCONTROLDETAILS, (DWORD)lpmcdA, 
			 fdwDetails, TRUE);
}

/**************************************************************************
 * 				mixerGetControlDetailsW	[WINMM.100]
 */
UINT WINAPI mixerGetControlDetailsW(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcd, DWORD fdwDetails) 
{
    DWORD			ret = MMSYSERR_NOTENABLED;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmcd, fdwDetails);

    if (lpmcd == NULL || lpmcd->cbStruct != sizeof(*lpmcd))
	return MMSYSERR_INVALPARAM;

    switch (fdwDetails & MIXER_GETCONTROLDETAILSF_QUERYMASK) {
    case MIXER_GETCONTROLDETAILSF_VALUE:
	/* can savely use W structure as it is, no string inside */
	ret = mixerGetControlDetailsA(hmix, lpmcd, fdwDetails);
	break;
    case MIXER_GETCONTROLDETAILSF_LISTTEXT:
	{
	    LPVOID	paDetailsW = lpmcd->paDetails;
	    int		size = max(1, lpmcd->cChannels) * sizeof(MIXERCONTROLDETAILS_LISTTEXTA);

	    if (lpmcd->u.cMultipleItems != 0 && lpmcd->u.cMultipleItems != lpmcd->u.hwndOwner) {
		size *= lpmcd->u.cMultipleItems;
	    }
	    lpmcd->paDetails = HeapAlloc(GetProcessHeap(), 0, size);
	    /* set up lpmcd->paDetails */
	    ret = mixerGetControlDetailsA(hmix, lpmcd, fdwDetails);
	    /* copy from lpmcd->paDetails back to paDetailsW; */
	    HeapFree(GetProcessHeap(), 0, lpmcd->paDetails);
	    lpmcd->paDetails = paDetailsW;
	}
	break;
    default:
	ERR("Unsupported fdwDetails=0x%08lx\n", fdwDetails);
    }

    return ret;
}

/**************************************************************************
 * 				mixerGetControlDetails	[MMSYSTEM.808]
 */
UINT16 WINAPI mixerGetControlDetails16(HMIXEROBJ16 hmix, 
				       LPMIXERCONTROLDETAILS16 lpmcd, 
				       DWORD fdwDetails) 
{
    DWORD	ret = MMSYSERR_NOTENABLED;
    SEGPTR	sppaDetails;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmcd, fdwDetails);

    if (lpmcd == NULL || lpmcd->cbStruct != sizeof(*lpmcd))
	return MMSYSERR_INVALPARAM;

    sppaDetails = (SEGPTR)lpmcd->paDetails;
    lpmcd->paDetails = PTR_SEG_TO_LIN(sppaDetails);
    ret = mixerGetControlDetailsA(hmix, (LPMIXERCONTROLDETAILS)lpmcd, fdwDetails);
    lpmcd->paDetails = (LPVOID)sppaDetails;

    return ret;
}

/**************************************************************************
 * 				mixerGetLineControlsA	[WINMM.104]
 */
UINT WINAPI mixerGetLineControlsA(HMIXEROBJ hmix, LPMIXERLINECONTROLSA lpmlcA, 
				  DWORD fdwControls) 
{
    LPWINE_MIXER	lpwm;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmlcA, fdwControls);

    if ((lpwm = MIXER_GetDev(hmix, fdwControls)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    if (lpmlcA == NULL || lpmlcA->cbStruct != sizeof(*lpmlcA))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(&lpwm->mld, MXDM_GETLINECONTROLS, (DWORD)lpmlcA, 
			 fdwControls, TRUE);
}

/**************************************************************************
 * 				mixerGetLineControlsW		[WINMM.105]
 */
UINT WINAPI mixerGetLineControlsW(HMIXEROBJ hmix, LPMIXERLINECONTROLSW lpmlcW, 
				  DWORD fdwControls) 
{
    MIXERLINECONTROLSA	mlcA;
    DWORD		ret;
    int			i;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmlcW, fdwControls);

    if (lpmlcW == NULL || lpmlcW->cbStruct != sizeof(*lpmlcW) || 
	lpmlcW->cbmxctrl != sizeof(MIXERCONTROLW))
	return MMSYSERR_INVALPARAM;

    mlcA.cbStruct = sizeof(mlcA);
    mlcA.dwLineID = lpmlcW->dwLineID;
    mlcA.u.dwControlID = lpmlcW->u.dwControlID;
    mlcA.u.dwControlType = lpmlcW->u.dwControlType;
    mlcA.cControls = lpmlcW->cControls;
    mlcA.cbmxctrl = sizeof(MIXERCONTROLA);
    mlcA.pamxctrl = HeapAlloc(GetProcessHeap(), 0, 
			      mlcA.cControls * mlcA.cbmxctrl);

    ret = mixerGetLineControlsA(hmix, &mlcA, fdwControls);

    if (ret == MMSYSERR_NOERROR) {
	lpmlcW->dwLineID = mlcA.dwLineID;
	lpmlcW->u.dwControlID = mlcA.u.dwControlID;
	lpmlcW->u.dwControlType = mlcA.u.dwControlType;
	lpmlcW->cControls = mlcA.cControls;
	
	for (i = 0; i < mlcA.cControls; i++) {
	    lpmlcW->pamxctrl[i].cbStruct = sizeof(MIXERCONTROLW);
	    lpmlcW->pamxctrl[i].dwControlID = mlcA.pamxctrl[i].dwControlID;
	    lpmlcW->pamxctrl[i].dwControlType = mlcA.pamxctrl[i].dwControlType;
	    lpmlcW->pamxctrl[i].fdwControl = mlcA.pamxctrl[i].fdwControl;
	    lpmlcW->pamxctrl[i].cMultipleItems = mlcA.pamxctrl[i].cMultipleItems;
	    lstrcpyAtoW(lpmlcW->pamxctrl[i].szShortName, 
			mlcA.pamxctrl[i].szShortName);
	    lstrcpyAtoW(lpmlcW->pamxctrl[i].szName, mlcA.pamxctrl[i].szName);
	    /* sizeof(lpmlcW->pamxctrl[i].Bounds) == 
	     * sizeof(mlcA.pamxctrl[i].Bounds) */
	    memcpy(&lpmlcW->pamxctrl[i].Bounds, &mlcA.pamxctrl[i].Bounds, 
		   sizeof(mlcA.pamxctrl[i].Bounds));
	    /* sizeof(lpmlcW->pamxctrl[i].Metrics) == 
	     * sizeof(mlcA.pamxctrl[i].Metrics) */
	    memcpy(&lpmlcW->pamxctrl[i].Metrics, &mlcA.pamxctrl[i].Metrics, 
		   sizeof(mlcA.pamxctrl[i].Metrics));
	}
    }

    HeapFree(GetProcessHeap(), 0, mlcA.pamxctrl);

    return ret;
}

/**************************************************************************
 * 				mixerGetLineControls		[MMSYSTEM.807]
 */
UINT16 WINAPI mixerGetLineControls16(HMIXEROBJ16 hmix, 
				     LPMIXERLINECONTROLS16 lpmlc16, 
				     DWORD fdwControls) 
{
    MIXERLINECONTROLSA	mlcA;
    DWORD		ret;
    int			i;
    LPMIXERCONTROL16	lpmc16;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmlc16, fdwControls);

    if (lpmlc16 == NULL || lpmlc16->cbStruct != sizeof(*lpmlc16) || 
	lpmlc16->cbmxctrl != sizeof(MIXERCONTROL16))
	return MMSYSERR_INVALPARAM;

    mlcA.cbStruct = sizeof(mlcA);
    mlcA.dwLineID = lpmlc16->dwLineID;
    mlcA.u.dwControlID = lpmlc16->u.dwControlID;
    mlcA.u.dwControlType = lpmlc16->u.dwControlType;
    mlcA.cControls = lpmlc16->cControls;
    mlcA.cbmxctrl = sizeof(MIXERCONTROLA);
    mlcA.pamxctrl = HeapAlloc(GetProcessHeap(), 0, 
			      mlcA.cControls * mlcA.cbmxctrl);

    ret = mixerGetLineControlsA(hmix, &mlcA, fdwControls);

    if (ret == MMSYSERR_NOERROR) {
	lpmlc16->dwLineID = mlcA.dwLineID;
	lpmlc16->u.dwControlID = mlcA.u.dwControlID;
	lpmlc16->u.dwControlType = mlcA.u.dwControlType;
	lpmlc16->cControls = mlcA.cControls;
	
	lpmc16 = PTR_SEG_TO_LIN(lpmlc16->pamxctrl);
	
	for (i = 0; i < mlcA.cControls; i++) {
	    lpmc16[i].cbStruct = sizeof(MIXERCONTROL16);
	    lpmc16[i].dwControlID = mlcA.pamxctrl[i].dwControlID;
	    lpmc16[i].dwControlType = mlcA.pamxctrl[i].dwControlType;
	    lpmc16[i].fdwControl = mlcA.pamxctrl[i].fdwControl;
	    lpmc16[i].cMultipleItems = mlcA.pamxctrl[i].cMultipleItems;
	    strcpy(lpmc16[i].szShortName, mlcA.pamxctrl[i].szShortName);
	    strcpy(lpmc16[i].szName, mlcA.pamxctrl[i].szName);
	    /* sizeof(lpmc16[i].Bounds) == sizeof(mlcA.pamxctrl[i].Bounds) */
	    memcpy(&lpmc16[i].Bounds, &mlcA.pamxctrl[i].Bounds, 
		   sizeof(mlcA.pamxctrl[i].Bounds));
	    /* sizeof(lpmc16[i].Metrics) == sizeof(mlcA.pamxctrl[i].Metrics) */
	    memcpy(&lpmc16[i].Metrics, &mlcA.pamxctrl[i].Metrics, 
		   sizeof(mlcA.pamxctrl[i].Metrics));
	}
    }

    HeapFree(GetProcessHeap(), 0, mlcA.pamxctrl);

    return ret;
}

/**************************************************************************
 * 				mixerGetLineInfoA		[WINMM.106]
 */
UINT WINAPI mixerGetLineInfoA(HMIXEROBJ hmix, LPMIXERLINEA lpmliW, DWORD fdwInfo)
{
    LPWINE_MIXER	lpwm;
    
    TRACE("(%04x, %p, %08lx)\n", hmix, lpmliW, fdwInfo);
    
    if ((lpwm = MIXER_GetDev(hmix, fdwInfo)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(&lpwm->mld, MXDM_GETLINEINFO, (DWORD)lpmliW, 
			 fdwInfo, TRUE);
}

/**************************************************************************
 * 				mixerGetLineInfoW		[WINMM.107]
 */
UINT WINAPI mixerGetLineInfoW(HMIXEROBJ hmix, LPMIXERLINEW lpmliW, 
			      DWORD fdwInfo) 
{
    MIXERLINEA		mliA;
    UINT		ret;
    
    TRACE("(%04x, %p, %08lx)\n", hmix, lpmliW, fdwInfo);

    if (lpmliW == NULL || lpmliW->cbStruct != sizeof(*lpmliW)) 
	return MMSYSERR_INVALPARAM;

    mliA.cbStruct = sizeof(mliA);
    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	mliA.dwComponentType = lpmliW->dwComponentType;
	break;
    case MIXER_GETLINEINFOF_DESTINATION:
	mliA.dwDestination = lpmliW->dwDestination;
	break;
    case MIXER_GETLINEINFOF_LINEID:
	mliA.dwLineID = lpmliW->dwLineID;
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	mliA.dwDestination = lpmliW->dwDestination;
	mliA.dwSource = lpmliW->dwSource;
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	mliA.Target.dwType = lpmliW->Target.dwType;
	mliA.Target.wMid = lpmliW->Target.wMid;
	mliA.Target.wPid = lpmliW->Target.wPid;
	mliA.Target.vDriverVersion = lpmliW->Target.vDriverVersion;
	lstrcpyWtoA(mliA.Target.szPname, lpmliW->Target.szPname);
	break;
    default:
	FIXME("Unsupported fdwControls=0x%08lx\n", fdwInfo);
    }

    ret = mixerGetLineInfoA(hmix, &mliA, fdwInfo);

    lpmliW->dwDestination = mliA.dwDestination;
    lpmliW->dwSource = mliA.dwSource;
    lpmliW->dwLineID = mliA.dwLineID;
    lpmliW->fdwLine = mliA.fdwLine;
    lpmliW->dwUser = mliA.dwUser;
    lpmliW->dwComponentType = mliA.dwComponentType;
    lpmliW->cChannels = mliA.cChannels;
    lpmliW->cConnections = mliA.cConnections;
    lpmliW->cControls = mliA.cControls;
    lstrcpyAtoW(lpmliW->szShortName, mliA.szShortName);
    lstrcpyAtoW(lpmliW->szName, mliA.szName);
    lpmliW->Target.dwType = mliA.Target.dwType;
    lpmliW->Target.dwDeviceID = mliA.Target.dwDeviceID;
    lpmliW->Target.wMid = mliA.Target.wMid;
    lpmliW->Target.wPid = mliA.Target.wPid;
    lpmliW->Target.vDriverVersion = mliA.Target.vDriverVersion;
    lstrcpyAtoW(lpmliW->Target.szPname, mliA.Target.szPname);

    return ret;
}

/**************************************************************************
 * 				mixerGetLineInfo	[MMSYSTEM.805]
 */
UINT16 WINAPI mixerGetLineInfo16(HMIXEROBJ16 hmix, LPMIXERLINE16 lpmli16, 
				 DWORD fdwInfo) 
{
    MIXERLINEA		mliA;
    UINT		ret;
    
    TRACE("(%04x, %p, %08lx)\n", hmix, lpmli16, fdwInfo);

    if (lpmli16 == NULL || lpmli16->cbStruct != sizeof(*lpmli16)) 
	return MMSYSERR_INVALPARAM;

    mliA.cbStruct = sizeof(mliA);
    switch (fdwInfo & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
	mliA.dwComponentType = lpmli16->dwComponentType;
	break;
    case MIXER_GETLINEINFOF_DESTINATION:
	mliA.dwDestination = lpmli16->dwDestination;
	break;
    case MIXER_GETLINEINFOF_LINEID:
	mliA.dwLineID = lpmli16->dwLineID;
	break;
    case MIXER_GETLINEINFOF_SOURCE:
	mliA.dwDestination = lpmli16->dwDestination;
	mliA.dwSource = lpmli16->dwSource;
	break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
	mliA.Target.dwType = lpmli16->Target.dwType;
	mliA.Target.wMid = lpmli16->Target.wMid;
	mliA.Target.wPid = lpmli16->Target.wPid;
	mliA.Target.vDriverVersion = lpmli16->Target.vDriverVersion;
	strcpy(mliA.Target.szPname, lpmli16->Target.szPname);
	break;
    default:
	FIXME("Unsupported fdwControls=0x%08lx\n", fdwInfo);
    }

    ret = mixerGetLineInfoA(hmix, &mliA, fdwInfo);

    lpmli16->dwDestination     	= mliA.dwDestination;
    lpmli16->dwSource          	= mliA.dwSource;
    lpmli16->dwLineID          	= mliA.dwLineID;
    lpmli16->fdwLine           	= mliA.fdwLine;
    lpmli16->dwUser            	= mliA.dwUser;
    lpmli16->dwComponentType   	= mliA.dwComponentType;
    lpmli16->cChannels         	= mliA.cChannels;
    lpmli16->cConnections      	= mliA.cConnections;
    lpmli16->cControls         	= mliA.cControls;
    strcpy(lpmli16->szShortName, mliA.szShortName);
    strcpy(lpmli16->szName, mliA.szName);
    lpmli16->Target.dwType     	= mliA.Target.dwType;
    lpmli16->Target.dwDeviceID 	= mliA.Target.dwDeviceID;
    lpmli16->Target.wMid       	= mliA.Target.wMid;
    lpmli16->Target.wPid        = mliA.Target.wPid;
    lpmli16->Target.vDriverVersion = mliA.Target.vDriverVersion;
    strcpy(lpmli16->Target.szPname, mliA.Target.szPname);

    return ret;
}

/**************************************************************************
 * 				mixerSetControlDetails	[WINMM.111]
 */
UINT WINAPI mixerSetControlDetails(HMIXEROBJ hmix, LPMIXERCONTROLDETAILS lpmcdA, 
				   DWORD fdwDetails) 
{
    LPWINE_MIXER	lpwm;

    TRACE("(%04x, %p, %08lx)\n", hmix, lpmcdA, fdwDetails);

    if ((lpwm = MIXER_GetDev(hmix, fdwDetails)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(&lpwm->mld, MXDM_SETCONTROLDETAILS, (DWORD)lpmcdA, 
			 fdwDetails, TRUE);
}

/**************************************************************************
 * 				mixerSetControlDetails	[MMSYSTEM.809]
 */
UINT16 WINAPI mixerSetControlDetails16(HMIXEROBJ16 hmix, 
				       LPMIXERCONTROLDETAILS16 lpmcd, 
				       DWORD fdwDetails) 
{
    TRACE("(%04x, %p, %08lx)\n", hmix, lpmcd, fdwDetails);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
 * 				mixerMessage		[WINMM.109]
 */
UINT WINAPI mixerMessage(HMIXER hmix, UINT uMsg, DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04lx, %d, %08lx, %08lx): semi-stub?\n",
	  (DWORD)hmix, uMsg, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hmix, MMDRV_MIXER, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMsg, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				mixerMessage		[MMSYSTEM.804]
 */
UINT16 WINAPI mixerMessage16(HMIXER16 hmix, UINT16 uMsg, DWORD dwParam1, 
			     DWORD dwParam2) 
{
    return mixerMessage(hmix, uMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				auxGetNumDevs		[WINMM.22]
 */
UINT WINAPI auxGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_AUX);
}

/**************************************************************************
 * 				auxGetNumDevs		[MMSYSTEM.350]
 */
UINT16 WINAPI auxGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_AUX);
}

/**************************************************************************
 * 				auxGetDevCapsW		[WINMM.20]
 */
UINT WINAPI auxGetDevCapsW(UINT uDeviceID, LPAUXCAPSW lpCaps, UINT uSize)
{
    AUXCAPSA	acA;
    UINT	ret = auxGetDevCapsA(uDeviceID, &acA, sizeof(acA));
    
    lpCaps->wMid = acA.wMid;
    lpCaps->wPid = acA.wPid;
    lpCaps->vDriverVersion = acA.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname, acA.szPname);
    lpCaps->wTechnology = acA.wTechnology;
    lpCaps->dwSupport = acA.dwSupport;
    return ret;
}

/**************************************************************************
 * 				auxGetDevCapsA		[WINMM.21]
 */
UINT WINAPI auxGetDevCapsA(UINT uDeviceID, LPAUXCAPSA lpCaps, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d) !\n", uDeviceID, lpCaps, uSize);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETDEVCAPS, (DWORD)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				auxGetDevCaps		[MMSYSTEM.351]
 */
UINT16 WINAPI auxGetDevCaps16(UINT16 uDeviceID, LPAUXCAPS16 lpCaps, UINT16 uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d) !\n", uDeviceID, lpCaps, uSize);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETDEVCAPS, (DWORD)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				auxGetVolume		[WINM.23]
 */
UINT WINAPI auxGetVolume(UINT uDeviceID, DWORD* lpdwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p) !\n", uDeviceID, lpdwVolume);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETVOLUME, (DWORD)lpdwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				auxGetVolume		[MMSYSTEM.352]
 */
UINT16 WINAPI auxGetVolume16(UINT16 uDeviceID, LPDWORD lpdwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p) !\n", uDeviceID, lpdwVolume);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_GETVOLUME, (DWORD)lpdwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				auxSetVolume		[WINMM.25]
 */
UINT WINAPI auxSetVolume(UINT uDeviceID, DWORD dwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %lu) !\n", uDeviceID, dwVolume);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_SETVOLUME, dwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				auxSetVolume		[MMSYSTEM.353]
 */
UINT16 WINAPI auxSetVolume16(UINT16 uDeviceID, DWORD dwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %lu) !\n", uDeviceID, dwVolume);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, AUXDM_SETVOLUME, dwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				auxOutMessage		[MMSYSTEM.354]
 */
DWORD WINAPI auxOutMessage(UINT uDeviceID, UINT uMessage, DWORD dw1, DWORD dw2)
{
    LPWINE_MLD		wmld;

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dw1, dw2, TRUE);
}

/**************************************************************************
 * 				auxOutMessage		[MMSYSTEM.354]
 */
DWORD WINAPI auxOutMessage16(UINT16 uDeviceID, UINT16 uMessage, DWORD dw1, DWORD dw2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", uDeviceID, uMessage, dw1, dw2);

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
	TRACE("(%04x, %04x, %08lx, %08lx): unhandled message\n",
	      uDeviceID, uMessage, dw1, dw2);
	break;
    }
    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_AUX, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dw1, dw2, TRUE);
}

/**************************************************************************
 * 				mciGetErrorStringW		[WINMM.46]
 */
BOOL WINAPI mciGetErrorStringW(DWORD wError, LPWSTR lpstrBuffer, UINT uLength)
{
    LPSTR	bufstr = HeapAlloc(GetProcessHeap(), 0, uLength);
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
    return mciGetErrorString16(wError, lpstrBuffer, uLength);
}

/**************************************************************************
 * 				mciGetErrorString		[MMSYSTEM.706]
 */
BOOL16 WINAPI mciGetErrorString16(DWORD dwError, LPSTR lpstrBuffer, UINT16 uLength)
{
    BOOL16		ret = FALSE;

    if (lpstrBuffer != NULL && uLength > 0 && 
	dwError >= MCIERR_BASE && dwError <= MCIERR_CUSTOM_DRIVER_BASE) {

	if (LoadStringA(MULTIMEDIA_GetIData()->hWinMM32Instance, 
			dwError, lpstrBuffer, uLength) > 0) {
	    ret = TRUE;
	}
    }
    return ret;
}

/**************************************************************************
 * 				mciDriverNotify			[MMSYSTEM.711]
 */
BOOL16 WINAPI mciDriverNotify16(HWND16 hWndCallBack, UINT16 wDevID, UINT16 wStatus)
{
    TRACE("(%04X, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    if (!IsWindow(hWndCallBack)) {
	WARN("bad hWnd for call back (0x%04x)\n", hWndCallBack);
	return FALSE;
    }
    TRACE("before PostMessage\n");
    Callout.PostMessageA(hWndCallBack, MM_MCINOTIFY, wStatus, wDevID);
    return TRUE;
}

/**************************************************************************
 *			mciDriverNotify				[WINMM.36]
 */
BOOL WINAPI mciDriverNotify(HWND hWndCallBack, UINT wDevID, UINT wStatus)
{

    TRACE("(%08X, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    if (!IsWindow(hWndCallBack)) {
	WARN("bad hWnd for call back (0x%04x)\n", hWndCallBack);
	return FALSE;
    }
    TRACE("before PostMessage\n");
    Callout.PostMessageA(hWndCallBack, MM_MCINOTIFY, wStatus, wDevID);
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
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x)\n", uDeviceID);

    wmd = MCI_GetDriver(uDeviceID);

    if (!wmd) {
	WARN("Bad uDeviceID\n");
	return 0L;
    }
    
    return wmd->dwPrivate;
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
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%04x, %08lx)\n", uDeviceID, data);

    wmd = MCI_GetDriver(uDeviceID);

    if (!wmd) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }
    
    wmd->dwPrivate = data;
    return TRUE;
}

/**************************************************************************
 * 				mciSendCommandA			[WINMM.49]
 */
DWORD WINAPI mciSendCommandA(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD	dwRet;

    TRACE("(%08x, %s, %08lx, %08lx)\n", 
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2, TRUE);
    dwRet = MCI_CleanUp(dwRet, wMsg, dwParam2, TRUE);
    TRACE("=> %08lx\n", dwRet);
    return dwRet;
}

/**************************************************************************
 * 				mciSendCommandW			[WINMM.50]
 */
DWORD WINAPI mciSendCommandW(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%08x, %s, %08lx, %08lx): stub\n", 
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);
    return MCIERR_UNSUPPORTED_FUNCTION;
}

/**************************************************************************
 * 				mciSendCommand			[MMSYSTEM.701]
 */
DWORD WINAPI mciSendCommand16(UINT16 wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2)
{
    DWORD		dwRet;

    TRACE("(%04X, %s, %08lX, %08lX)\n", 
	  wDevID, MCI_MessageToString(wMsg), dwParam1, dwParam2);

    dwRet = MCI_SendCommand(wDevID, wMsg, dwParam1, dwParam2, FALSE);
    dwRet = MCI_CleanUp(dwRet, wMsg, dwParam2, FALSE);
    TRACE("=> %ld\n", dwRet);
    return dwRet;
}
    
/**************************************************************************
 * 				mciGetDeviceID		       	[MMSYSTEM.703]
 */
UINT16 WINAPI mciGetDeviceID16(LPCSTR lpstrName)
{
    TRACE("(\"%s\")\n", lpstrName);

    return MCI_GetDriverFromString(lpstrName);
}

/**************************************************************************
 * 				mciGetDeviceIDA    		[WINMM.41]
 */
UINT WINAPI mciGetDeviceIDA(LPCSTR lpstrName)
{
    return MCI_GetDriverFromString(lpstrName);
}

/**************************************************************************
 * 				mciGetDeviceIDW		       	[WINMM.43]
 */
UINT WINAPI mciGetDeviceIDW(LPCWSTR lpwstrName)
{
    LPSTR 	lpstrName;
    UINT	ret;

    lpstrName = HEAP_strdupWtoA(GetProcessHeap(), 0, lpwstrName);
    ret = MCI_GetDriverFromString(lpstrName);
    HeapFree(GetProcessHeap(), 0, lpstrName);
    return ret;
}

/**************************************************************************
 * 				MCI_DefYieldProc	       	[internal]
 */
UINT16	WINAPI MCI_DefYieldProc(UINT16 wDevID, DWORD data)
{
    INT16	ret;
    
    TRACE("(0x%04x, 0x%08lx)\n", wDevID, data);

    if ((HIWORD(data) != 0 && GetActiveWindow16() != HIWORD(data)) ||
	(GetAsyncKeyState(LOWORD(data)) & 1) == 0) {
	UserYield16();
	ret = 0;
    } else {
	MSG		msg;

	msg.hwnd = HIWORD(data);
	while (!PeekMessageA(&msg, HIWORD(data), WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));
	ret = -1;
    }
    return ret;
}

/**************************************************************************
 * 				mciSetYieldProc			[MMSYSTEM.714]
 */
BOOL16 WINAPI mciSetYieldProc16(UINT16 uDeviceID, YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }
    
    wmd->lpfnYieldProc = fpYieldProc;
    wmd->dwYieldData   = dwYieldData;
    wmd->bIs32         = FALSE;

    return TRUE;
}

/**************************************************************************
 * 				mciSetYieldProc			[WINMM.54]
 */
BOOL WINAPI mciSetYieldProc(UINT uDeviceID, YIELDPROC fpYieldProc, DWORD dwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }
    
    wmd->lpfnYieldProc = fpYieldProc;
    wmd->dwYieldData   = dwYieldData;
    wmd->bIs32         = TRUE;

    return TRUE;
}

/**************************************************************************
 * 				mciGetDeviceIDFromElementID	[MMSYSTEM.715]
 */
UINT16 WINAPI mciGetDeviceIDFromElementID16(DWORD dwElementID, LPCSTR lpstrType)
{
    FIXME("(%lu, %s) stub\n", dwElementID, lpstrType);
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
    FIXME("(%lu, %p) stub\n", dwElementID, lpstrType);
    return 0;
}
	
/**************************************************************************
 * 				mciGetYieldProc			[MMSYSTEM.716]
 */
YIELDPROC WINAPI mciGetYieldProc16(UINT16 uDeviceID, DWORD* lpdwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p)\n", uDeviceID, lpdwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return NULL;
    }
    if (!wmd->lpfnYieldProc) {
	WARN("No proc set\n");
	return NULL;
    }
    if (wmd->bIs32) {
	WARN("Proc is 32 bit\n");
	return NULL;
    }
    return wmd->lpfnYieldProc;
}
    
/**************************************************************************
 * 				mciGetYieldProc			[WINMM.47]
 */
YIELDPROC WINAPI mciGetYieldProc(UINT uDeviceID, DWORD* lpdwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p)\n", uDeviceID, lpdwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return NULL;
    }
    if (!wmd->lpfnYieldProc) {
	WARN("No proc set\n");
	return NULL;
    }
    if (!wmd->bIs32) {
	WARN("Proc is 32 bit\n");
	return NULL;
    }
    return wmd->lpfnYieldProc;
}

/**************************************************************************
 * 				mciGetCreatorTask		[MMSYSTEM.717]
 */
HTASK16 WINAPI mciGetCreatorTask16(UINT16 uDeviceID)
{
    return mciGetCreatorTask(uDeviceID);
}

/**************************************************************************
 * 				mciGetCreatorTask		[WINMM.40]
 */
HTASK WINAPI mciGetCreatorTask(UINT uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;
    HTASK		ret;

    TRACE("(%u)\n", uDeviceID);

    ret = (!(wmd = MCI_GetDriver(uDeviceID))) ? 0 : wmd->hCreatorTask;

    TRACE("=> %04x\n", ret);
    return ret;
}

/**************************************************************************
 * 				mciDriverYield			[MMSYSTEM.710]
 */
UINT16 WINAPI mciDriverYield16(UINT16 uDeviceID) 
{
    LPWINE_MCIDRIVER	wmd;
    UINT16		ret = 0;

    /*    TRACE("(%04x)\n", uDeviceID); */

    if (!(wmd = MCI_GetDriver(uDeviceID)) || !wmd->lpfnYieldProc || wmd->bIs32) {
	UserYield16();
    } else {
	ret = wmd->lpfnYieldProc(uDeviceID, wmd->dwYieldData);
    }

    return ret;
}

/**************************************************************************
 * 			mciDriverYield				[WINMM.37]
 */
UINT WINAPI mciDriverYield(UINT uDeviceID) 
{
    LPWINE_MCIDRIVER	wmd;
    UINT		ret = 0;

    TRACE("(%04x)\n", uDeviceID);

    if (!(wmd = MCI_GetDriver(uDeviceID)) || !wmd->lpfnYieldProc || !wmd->bIs32) {
	UserYield16();
    } else {
	ret = wmd->lpfnYieldProc(uDeviceID, wmd->dwYieldData);
    }

    return ret;
}

/**************************************************************************
 * 				midiOutGetNumDevs	[WINMM.80]
 */
UINT WINAPI midiOutGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_MIDIOUT);
}

/**************************************************************************
 * 				midiOutGetNumDevs	[MMSYSTEM.201]
 */
UINT16 WINAPI midiOutGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_MIDIOUT);
}

/**************************************************************************
 * 				midiOutGetDevCapsW	[WINMM.76]
 */
UINT WINAPI midiOutGetDevCapsW(UINT uDeviceID, LPMIDIOUTCAPSW lpCaps, 
			       UINT uSize)
{
    MIDIOUTCAPSA	mocA;
    UINT		ret;
    
    ret = midiOutGetDevCapsA(uDeviceID, &mocA, sizeof(mocA));
    lpCaps->wMid		= mocA.wMid;
    lpCaps->wPid		= mocA.wPid;
    lpCaps->vDriverVersion	= mocA.vDriverVersion;
    lstrcpyAtoW(lpCaps->szPname, mocA.szPname);
    lpCaps->wTechnology	        = mocA.wTechnology;
    lpCaps->wVoices		= mocA.wVoices;
    lpCaps->wNotes		= mocA.wNotes;
    lpCaps->wChannelMask	= mocA.wChannelMask;
    lpCaps->dwSupport	        = mocA.dwSupport;
    return ret;
}

/**************************************************************************
 * 				midiOutGetDevCapsA	[WINMM.75]
 */
UINT WINAPI midiOutGetDevCapsA(UINT uDeviceID, LPMIDIOUTCAPSA lpCaps, 
			       UINT uSize)
{
    LPWINE_MLD	wmld;

    TRACE("(%u, %p, %u);\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_MIDIOUT, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_GETDEVCAPS, (DWORD)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				midiOutGetDevCaps	[MMSYSTEM.202]
 */
UINT16 WINAPI midiOutGetDevCaps16(UINT16 uDeviceID, LPMIDIOUTCAPS16 lpCaps, 
				  UINT16 uSize)
{
    MIDIOUTCAPSA	capsA;
    UINT		dwRet;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    dwRet = midiOutGetDevCapsA(uDeviceID, &capsA, sizeof(capsA));
    if (dwRet == MMSYSERR_NOERROR) {
	lpCaps->wMid            = capsA.wMid;
	lpCaps->wPid            = capsA.wPid;
	lpCaps->vDriverVersion  = capsA.vDriverVersion;
	strcpy(lpCaps->szPname, capsA.szPname);
	lpCaps->wTechnology 	= capsA.wTechnology;
	lpCaps->wVoices         = capsA.wVoices;
	lpCaps->wNotes          = capsA.wNotes;
	lpCaps->wChannelMask    = capsA.wChannelMask;
	lpCaps->dwSupport	= capsA.dwSupport;
    }
    return dwRet;
 }

/**************************************************************************
 * 				MIDI_GetErrorText       	[internal]
 */
static	UINT16	MIDI_GetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    UINT16		ret = MMSYSERR_BADERRNUM;

    if (lpText == NULL) {
	ret = MMSYSERR_INVALPARAM;
    } else if (uSize == 0) {
	ret = MMSYSERR_NOERROR;
    } else if (
	       /* test has been removed 'coz MMSYSERR_BASE is 0, and gcc did emit
		* a warning for the test was always true */
	       (/*uError >= MMSYSERR_BASE && */ uError <= MMSYSERR_LASTERROR) ||
	       (uError >= MIDIERR_BASE  && uError <= MIDIERR_LASTERROR)) {
	
	if (LoadStringA(MULTIMEDIA_GetIData()->hWinMM32Instance, 
			uError, lpText, uSize) > 0) {
	    ret = MMSYSERR_NOERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiOutGetErrorTextA 	[WINMM.77]
 */
UINT WINAPI midiOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return MIDI_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiOutGetErrorTextW 	[WINMM.78]
 */
UINT WINAPI midiOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret;
    
    ret = MIDI_GetErrorText(uError, xstr, uSize);
    lstrcpyAtoW(lpText, xstr);
    HeapFree(GetProcessHeap(), 0, xstr);
    return ret;
}

/**************************************************************************
 * 				midiOutGetErrorText 	[MMSYSTEM.203]
 */
UINT16 WINAPI midiOutGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    return MIDI_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				MIDI_OutAlloc    		[internal]
 */
static	LPWINE_MIDI	MIDI_OutAlloc(HMIDIOUT* lphMidiOut, LPDWORD lpdwCallback, 
				      LPDWORD lpdwInstance, LPDWORD lpdwFlags, 
				      DWORD cIDs, MIDIOPENSTRMID* lpIDs, BOOL bFrom32)
{
    HANDLE	      	hMidiOut;
    LPWINE_MIDI		lpwm;
    UINT		size;
    
    size = sizeof(WINE_MIDI) + (cIDs ? (cIDs-1) : 0) * sizeof(MIDIOPENSTRMID);

    lpwm = (LPWINE_MIDI)MMDRV_Alloc(size, MMDRV_MIDIOUT, &hMidiOut, lpdwFlags, 
				    lpdwCallback, lpdwInstance, bFrom32);

    if (lphMidiOut != NULL) 
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

UINT MMSYSTEM_midiOutOpen(HMIDIOUT* lphMidiOut, UINT uDeviceID, DWORD dwCallback, 
			  DWORD dwInstance, DWORD dwFlags, BOOL bFrom32)
{
    HMIDIOUT		hMidiOut;
    LPWINE_MIDI		lpwm;
    UINT		dwRet = 0;
    
    TRACE("(%p, %d, %08lX, %08lX, %08lX);\n", 
	  lphMidiOut, uDeviceID, dwCallback, dwInstance, dwFlags);

    if (lphMidiOut != NULL) *lphMidiOut = 0;

    lpwm = MIDI_OutAlloc(&hMidiOut, &dwCallback, &dwInstance, &dwFlags, 
			 0, NULL, bFrom32);

    if (lpwm == NULL)
	return MMSYSERR_NOMEM;

    lpwm->mld.uDeviceID = uDeviceID;

    dwRet = MMDRV_Open((LPWINE_MLD)lpwm, MODM_OPEN, (DWORD)&lpwm->mod, 
		       dwFlags);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMidiOut, (LPWINE_MLD)lpwm);
	hMidiOut = 0;
    }

    if (lphMidiOut) *lphMidiOut = hMidiOut;
    TRACE("=> %d hMidi=%04x\n", dwRet, hMidiOut);

    return dwRet;
}

/**************************************************************************
 * 				midiOutOpen    		[WINM.84]
 */
UINT WINAPI midiOutOpen(HMIDIOUT* lphMidiOut, UINT uDeviceID,
			DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    return MMSYSTEM_midiOutOpen(lphMidiOut, uDeviceID, dwCallback, 
				dwInstance, dwFlags, TRUE);
}

/**************************************************************************
 * 				midiOutOpen    		[MMSYSTEM.204]
 */
UINT16 WINAPI midiOutOpen16(HMIDIOUT16* lphMidiOut, UINT16 uDeviceID,
                            DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDIOUT	hmo;
    UINT	ret;
    
    ret = MMSYSTEM_midiOutOpen(&hmo, uDeviceID, dwCallback, dwInstance, 
			       dwFlags, FALSE);
    
    if (lphMidiOut != NULL) *lphMidiOut = hmo;
    return ret;
}

/**************************************************************************
 * 				midiOutClose		[WINMM.74]
 */
UINT WINAPI midiOutClose(HMIDIOUT hMidiOut)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;

    TRACE("(%04X)\n", hMidiOut);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MODM_CLOSE);
    MMDRV_Free(hMidiOut, wmld);

    return dwRet;
}

/**************************************************************************
 * 				midiOutClose		[MMSYSTEM.205]
 */
UINT16 WINAPI midiOutClose16(HMIDIOUT16 hMidiOut)
{
    return midiOutClose(hMidiOut);
}

/**************************************************************************
 * 				midiOutPrepareHeader	[WINMM.85]
 */
UINT WINAPI midiOutPrepareHeader(HMIDIOUT hMidiOut,
				 MIDIHDR* lpMidiOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_PREPARE, (DWORD)lpMidiOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiOutPrepareHeader	[MMSYSTEM.206]
 */
UINT16 WINAPI midiOutPrepareHeader16(HMIDIOUT16 hMidiOut,
                                     LPMIDIHDR16 /*SEGPTR*/ lpsegMidiOutHdr, 	
				     UINT16 uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_PREPARE, (DWORD)lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[WINMM.89]
 */
UINT WINAPI midiOutUnprepareHeader(HMIDIOUT hMidiOut,
				   MIDIHDR* lpMidiOutHdr, UINT uSize)
{	
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_UNPREPARE, (DWORD)lpMidiOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[MMSYSTEM.207]
 */
UINT16 WINAPI midiOutUnprepareHeader16(HMIDIOUT16 hMidiOut,
				       LPMIDIHDR16 /*SEGPTR*/ lpsegMidiOutHdr, 
				       UINT16 uSize)
{
    LPWINE_MLD		wmld;
    LPMIDIHDR16		lpMidiOutHdr = PTR_SEG_TO_LIN(lpsegMidiOutHdr);

    TRACE("(%04X, %p, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_UNPREPARE, (DWORD)lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutShortMsg		[WINMM.88]
 */
UINT WINAPI midiOutShortMsg(HMIDIOUT hMidiOut, DWORD dwMsg)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lX)\n", hMidiOut, dwMsg);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_DATA, dwMsg, 0L, FALSE);
}

/**************************************************************************
 * 				midiOutShortMsg		[MMSYSTEM.208]
 */
UINT16 WINAPI midiOutShortMsg16(HMIDIOUT16 hMidiOut, DWORD dwMsg)
{
    return midiOutShortMsg(hMidiOut, dwMsg);
}

/**************************************************************************
 * 				midiOutLongMsg		[WINMM.82]
 */
UINT WINAPI midiOutLongMsg(HMIDIOUT hMidiOut,
			   MIDIHDR* lpMidiOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiOut, lpMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_LONGDATA, (DWORD)lpMidiOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiOutLongMsg		[MMSYSTEM.209]
 */
UINT16 WINAPI midiOutLongMsg16(HMIDIOUT16 hMidiOut,
                               LPMIDIHDR16 /* SEGPTR */ lpsegMidiOutHdr, 
			       UINT16 uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_LONGDATA, (DWORD)lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutReset		[WINMM.86]
 */
UINT WINAPI midiOutReset(HMIDIOUT hMidiOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X)\n", hMidiOut);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_RESET, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				midiOutReset		[MMSYSTEM.210]
 */
UINT16 WINAPI midiOutReset16(HMIDIOUT16 hMidiOut)
{
    return midiOutReset(hMidiOut);
}

/**************************************************************************
 * 				midiOutGetVolume	[WINM.81]
 */
UINT WINAPI midiOutGetVolume(UINT uDeviceID, DWORD* lpdwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p);\n", uDeviceID, lpdwVolume);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_MIDIOUT, TRUE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_GETVOLUME, (DWORD)lpdwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				midiOutGetVolume	[MMSYSTEM.211]
 */
UINT16 WINAPI midiOutGetVolume16(UINT16 uDeviceID, DWORD* lpdwVolume)
{
    return midiOutGetVolume(uDeviceID, lpdwVolume);
}

/**************************************************************************
 * 				midiOutSetVolume	[WINMM.87]
 */
UINT WINAPI midiOutSetVolume(UINT uDeviceID, DWORD dwVolume)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %ld);\n", uDeviceID, dwVolume);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_MIDIOUT, TRUE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_SETVOLUME, dwVolume, 0L, TRUE);
}

/**************************************************************************
 * 				midiOutSetVolume	[MMSYSTEM.212]
 */
UINT16 WINAPI midiOutSetVolume16(UINT16 uDeviceID, DWORD dwVolume)
{
    return midiOutSetVolume(uDeviceID, dwVolume);
}

/**************************************************************************
 * 				midiOutCachePatches		[WINMM.73]
 */
UINT WINAPI midiOutCachePatches(HMIDIOUT hMidiOut, UINT uBank,
				WORD* lpwPatchArray, UINT uFlags)
{
    /* not really necessary to support this */
    FIXME("not supported yet\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutCachePatches		[MMSYSTEM.213]
 */
UINT16 WINAPI midiOutCachePatches16(HMIDIOUT16 hMidiOut, UINT16 uBank,
                                    WORD* lpwPatchArray, UINT16 uFlags)
{
    return midiOutCachePatches(hMidiOut, uBank, lpwPatchArray, uFlags);
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[WINMM.72]
 */
UINT WINAPI midiOutCacheDrumPatches(HMIDIOUT hMidiOut, UINT uPatch,
				    WORD* lpwKeyArray, UINT uFlags)
{
    FIXME("not supported yet\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[MMSYSTEM.214]
 */
UINT16 WINAPI midiOutCacheDrumPatches16(HMIDIOUT16 hMidiOut, UINT16 uPatch,
                                        WORD* lpwKeyArray, UINT16 uFlags)
{
    return midiOutCacheDrumPatches16(hMidiOut, uPatch, lpwKeyArray, uFlags);
}

/**************************************************************************
 * 				midiOutGetID		[WINMM.79]
 */
UINT WINAPI midiOutGetID(HMIDIOUT hMidiOut, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p)\n", hMidiOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiOutGetID		[MMSYSTEM.215]
 */
UINT16 WINAPI midiOutGetID16(HMIDIOUT16 hMidiOut, UINT16* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p)\n", hMidiOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiOutMessage		[WINMM.83]
 */
DWORD WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage, 
			    DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", hMidiOut, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    switch (uMessage) {
    case MODM_OPEN:
    case MODM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				midiOutMessage		[MMSYSTEM.216]
 */
DWORD WINAPI midiOutMessage16(HMIDIOUT16 hMidiOut, UINT16 uMessage, 
                              DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", hMidiOut, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hMidiOut, MMDRV_MIDIOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    switch (uMessage) {
    case MODM_OPEN:
    case MODM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;

    case MODM_GETVOLUME:
        return midiOutGetVolume16(hMidiOut, (LPDWORD)PTR_SEG_TO_LIN(dwParam1));
    case MODM_LONGDATA:
        return midiOutLongMsg16(hMidiOut, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MODM_PREPARE:
        /* lpMidiOutHdr is still a segmented pointer for this function */
        return midiOutPrepareHeader16(hMidiOut, (LPMIDIHDR16)dwParam1, dwParam2);
    case MODM_UNPREPARE:
        return midiOutUnprepareHeader16(hMidiOut, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);	
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				midiInGetNumDevs	[WINMM.64]
 */
UINT WINAPI midiInGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_MIDIIN);
}

/**************************************************************************
 * 				midiInGetNumDevs	[MMSYSTEM.301]
 */
UINT16 WINAPI midiInGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_MIDIIN);
}

/**************************************************************************
 * 				midiInGetDevCapsW	[WINMM.60]
 */
UINT WINAPI midiInGetDevCapsW(UINT uDeviceID, LPMIDIINCAPSW lpCaps, UINT uSize)
{
    MIDIINCAPSA		micA;
    UINT		ret = midiInGetDevCapsA(uDeviceID, &micA, uSize);
    
    if (ret == MMSYSERR_NOERROR) {
	lpCaps->wMid = micA.wMid;
	lpCaps->wPid = micA.wPid;
	lpCaps->vDriverVersion = micA.vDriverVersion;
	lstrcpyAtoW(lpCaps->szPname, micA.szPname);
	lpCaps->dwSupport = micA.dwSupport;
    }
    return ret;
}

/**************************************************************************
 * 				midiInGetDevCapsA	[WINMM.59]
 */
UINT WINAPI midiInGetDevCapsA(UINT uDeviceID, LPMIDIINCAPSA lpCaps, UINT uSize)
{
    LPWINE_MLD	wmld;

    TRACE("(%d, %p, %d);\n", uDeviceID, lpCaps, uSize);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_MIDIIN, TRUE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

   return MMDRV_Message(wmld, MIDM_GETDEVCAPS, (DWORD)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				midiInGetDevCaps	[MMSYSTEM.302]
 */
UINT16 WINAPI midiInGetDevCaps16(UINT16 uDeviceID, LPMIDIINCAPS16 lpCaps, 
				 UINT16 uSize)
{
    MIDIINCAPSA		micA;
    UINT		ret = midiInGetDevCapsA(uDeviceID, &micA, uSize);
    
    if (ret == MMSYSERR_NOERROR) {
	lpCaps->wMid = micA.wMid;
	lpCaps->wPid = micA.wPid;
	lpCaps->vDriverVersion = micA.vDriverVersion;
	strcpy(lpCaps->szPname, micA.szPname);
	lpCaps->dwSupport = micA.dwSupport;
    }
 
    return ret;
}

/**************************************************************************
 * 				midiInGetErrorTextW 		[WINMM.62]
 */
UINT WINAPI midiInGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = MIDI_GetErrorText(uError, xstr, uSize);

    lstrcpyAtoW(lpText, xstr);
    HeapFree(GetProcessHeap(), 0, xstr);
    return ret;
}

/**************************************************************************
 * 				midiInGetErrorTextA 		[WINMM.61]
 */
UINT WINAPI midiInGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return MIDI_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiInGetErrorText 		[MMSYSTEM.303]
 */
UINT16 WINAPI midiInGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    return MIDI_GetErrorText(uError, lpText, uSize);
}

static	UINT MMSYSTEM_midiInOpen(HMIDIIN* lphMidiIn, UINT uDeviceID, DWORD dwCallback, 
				 DWORD dwInstance, DWORD dwFlags, BOOL bFrom32)
{
    HMIDI		hMidiIn;
    LPWINE_MIDI		lpwm;
    DWORD		dwRet = 0;
    
    TRACE("(%p, %d, %08lX, %08lX, %08lX);\n", 
	  lphMidiIn, uDeviceID, dwCallback, dwInstance, dwFlags);

    if (lphMidiIn != NULL) *lphMidiIn = 0;

    lpwm = (LPWINE_MIDI)MMDRV_Alloc(sizeof(WINE_MIDI), MMDRV_MIDIIN, &hMidiIn,
				    &dwFlags, &dwCallback, &dwInstance, bFrom32);

    if (lpwm == NULL) 
	return MMSYSERR_NOMEM;

    lpwm->mod.hMidi = hMidiIn;
    lpwm->mod.dwCallback = dwCallback;
    lpwm->mod.dwInstance = dwInstance;

    lpwm->mld.uDeviceID = uDeviceID;
    dwRet = MMDRV_Open(&lpwm->mld, MIDM_OPEN, (DWORD)&lpwm->mod, dwFlags);

    if (dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(hMidiIn, &lpwm->mld);
	hMidiIn = 0;
    }
    if (lphMidiIn != NULL) *lphMidiIn = hMidiIn;
    TRACE("=> %ld hMidi=%04x\n", dwRet, hMidiIn);

    return dwRet;
}

/**************************************************************************
 * 				midiInOpen		[WINMM.66]
 */
UINT WINAPI midiInOpen(HMIDIIN* lphMidiIn, UINT uDeviceID,
		       DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    return MMSYSTEM_midiInOpen(lphMidiIn, uDeviceID, dwCallback, 
			       dwInstance, dwFlags, TRUE);
}

/**************************************************************************
 * 				midiInOpen		[MMSYSTEM.304]
 */
UINT16 WINAPI midiInOpen16(HMIDIIN16* lphMidiIn, UINT16 uDeviceID,
			   DWORD dwCallback, DWORD dwInstance, DWORD dwFlags)
{
    HMIDIIN	xhmid;
    UINT 	ret;

    ret = MMSYSTEM_midiInOpen(&xhmid, uDeviceID, dwCallback, dwInstance, 
			      dwFlags, FALSE);
    
    if (lphMidiIn) *lphMidiIn = xhmid;
    return ret;
}

/**************************************************************************
 * 				midiInClose		[WINMM.58]
 */
UINT WINAPI midiInClose(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;    

    TRACE("(%04X)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, MIDM_CLOSE);
    MMDRV_Free(hMidiIn, wmld);
    return dwRet;
}

/**************************************************************************
 * 				midiInClose		[MMSYSTEM.305]
 */
UINT16 WINAPI midiInClose16(HMIDIIN16 hMidiIn)
{
    return midiInClose(hMidiIn);
}

/**************************************************************************
 * 				midiInPrepareHeader	[WINMM.67]
 */
UINT WINAPI midiInPrepareHeader(HMIDIIN hMidiIn, 
				MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_PREPARE, (DWORD)lpMidiInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiInPrepareHeader	[MMSYSTEM.306]
 */
UINT16 WINAPI midiInPrepareHeader16(HMIDIIN16 hMidiIn,
                                    MIDIHDR16* /*SEGPTR*/ lpsegMidiInHdr, 
				    UINT16 uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_PREPARE, (DWORD)lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[WINMM.71]
 */
UINT WINAPI midiInUnprepareHeader(HMIDIIN hMidiIn,
				  MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_UNPREPARE, (DWORD)lpMidiInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[MMSYSTEM.307]
 */
UINT16 WINAPI midiInUnprepareHeader16(HMIDIIN16 hMidiIn,
                                      MIDIHDR16* /* SEGPTR */ lpsegMidiInHdr, 
				      UINT16 uSize)
{
    LPWINE_MLD		wmld;
    LPMIDIHDR16		lpMidiInHdr = PTR_SEG_TO_LIN(lpsegMidiInHdr);

    TRACE("(%04X, %p, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_UNPREPARE, (DWORD)lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInAddBuffer		[WINMM.57]
 */
UINT WINAPI midiInAddBuffer(HMIDIIN hMidiIn,
			    MIDIHDR* lpMidiInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiIn, lpMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_ADDBUFFER, (DWORD)lpMidiInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				midiInAddBuffer		[MMSYSTEM.308]
 */
UINT16 WINAPI midiInAddBuffer16(HMIDIIN16 hMidiIn,
                                MIDIHDR16* /* SEGPTR */ lpsegMidiInHdr, 
				UINT16 uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_ADDBUFFER, (DWORD)lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInStart			[WINMM.69]
 */
UINT WINAPI midiInStart(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_START, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				midiInStart			[MMSYSTEM.309]
 */
UINT16 WINAPI midiInStart16(HMIDIIN16 hMidiIn)
{
    return midiInStart(hMidiIn);
}

/**************************************************************************
 * 				midiInStop			[WINMM.70]
 */
UINT WINAPI midiInStop(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_STOP, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				midiInStop			[MMSYSTEM.310]
 */
UINT16 WINAPI midiInStop16(HMIDIIN16 hMidiIn)
{
    return midiInStop(hMidiIn);
}

/**************************************************************************
 * 				midiInReset			[WINMM.68]
 */
UINT WINAPI midiInReset(HMIDIIN hMidiIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X)\n", hMidiIn);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_RESET, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				midiInReset			[MMSYSTEM.311]
 */
UINT16 WINAPI midiInReset16(HMIDIIN16 hMidiIn)
{
    return midiInReset(hMidiIn);
}

/**************************************************************************
 * 				midiInGetID			[WINMM.63]
 */
UINT WINAPI midiInGetID(HMIDIIN hMidiIn, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p)\n", hMidiIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, TRUE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiInGetID			[MMSYSTEM.312]
 */
UINT16 WINAPI midiInGetID16(HMIDIIN16 hMidiIn, UINT16* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p)\n", hMidiIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, TRUE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiInMessage		[WINMM.65]
 */
DWORD WINAPI midiInMessage(HMIDIIN hMidiIn, UINT uMessage, 
			   DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", hMidiIn, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    switch (uMessage) {
    case MIDM_OPEN:
    case MIDM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				midiInMessage		[MMSYSTEM.313]
 */
DWORD WINAPI midiInMessage16(HMIDIIN16 hMidiIn, UINT16 uMessage, 
                             DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", hMidiIn, uMessage, dwParam1, dwParam2);

    switch (uMessage) {
    case MIDM_OPEN:
    case MIDM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;

    case MIDM_GETDEVCAPS:
        return midiInGetDevCaps16(hMidiIn, (LPMIDIINCAPS16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MIDM_PREPARE:
        return midiInPrepareHeader16(hMidiIn, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MIDM_UNPREPARE:
        return midiInUnprepareHeader16(hMidiIn, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);
    case MIDM_ADDBUFFER:
        return midiInAddBuffer16(hMidiIn, (LPMIDIHDR16)PTR_SEG_TO_LIN(dwParam1), dwParam2);    
    }

    if ((wmld = MMDRV_Get(hMidiIn, MMDRV_MIDIIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, FALSE);
}

typedef struct WINE_MIDIStream {
    HMIDIOUT			hDevice;
    HANDLE			hThread;
    DWORD			dwThreadID;
    DWORD			dwTempo;
    DWORD			dwTimeDiv;
    DWORD			dwPositionMS;
    DWORD			dwPulses;
    DWORD			dwStartTicks;
    WORD			wFlags;
    HANDLE			hEvent;
    LPMIDIHDR			lpMidiHdr;
} WINE_MIDIStream;

#define WINE_MSM_HEADER		(WM_USER+0)
#define WINE_MSM_STOP		(WM_USER+1)

/**************************************************************************
 * 				MMSYSTEM_GetMidiStream		[internal]
 */
static	BOOL	MMSYSTEM_GetMidiStream(HMIDISTRM hMidiStrm, WINE_MIDIStream** lpMidiStrm, WINE_MIDI** lplpwm)
{
    WINE_MIDI* lpwm = (LPWINE_MIDI)MMDRV_Get(hMidiStrm, MMDRV_MIDIOUT, FALSE);

    if (lplpwm)
	*lplpwm = lpwm;

    if (lpwm == NULL) {
	return FALSE;
    }

    *lpMidiStrm = (WINE_MIDIStream*)lpwm->mod.rgIds.dwStreamID;

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
	int	nf = -(char)HIBYTE(lpMidiStrm->dwTimeDiv);	/* number of frames     */
	int	nsf = LOBYTE(lpMidiStrm->dwTimeDiv);		/* number of sub-frames */
	ret = (pulse * 1000) / (nf * nsf);
    } else {
	ret = (DWORD)((double)pulse * ((double)lpMidiStrm->dwTempo / 1000) /	
		      (double)lpMidiStrm->dwTimeDiv);
    }
    
    return ret;
}

/**************************************************************************
 * 			MMSYSTEM_MidiStream_MessageHandler	[internal]
 */
static	BOOL	MMSYSTEM_MidiStream_MessageHandler(WINE_MIDIStream* lpMidiStrm, LPWINE_MIDI lpwm, LPMSG msg)
{
    LPMIDIHDR	lpMidiHdr;
    LPMIDIHDR*	lpmh;
    LPBYTE	lpData;

    switch (msg->message) {
    case WM_QUIT:
	SetEvent(lpMidiStrm->hEvent);
	return FALSE;
    case WINE_MSM_STOP:
	TRACE("STOP\n");
	/* this is not quite what MS doc says... */
	midiOutReset(lpMidiStrm->hDevice);
	/* empty list of already submitted buffers */
	for (lpMidiHdr = lpMidiStrm->lpMidiHdr; lpMidiHdr; lpMidiHdr = (LPMIDIHDR)lpMidiHdr->lpNext) {
	    lpMidiHdr->dwFlags |= MHDR_DONE;
	    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	    
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags, lpMidiStrm->hDevice, 
			   MM_MOM_DONE, lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
	}
	lpMidiStrm->lpMidiHdr = 0;
	SetEvent(lpMidiStrm->hEvent);
	break;
    case WINE_MSM_HEADER:
	/* sets initial tick count for first MIDIHDR */
	if (!lpMidiStrm->dwStartTicks)
	    lpMidiStrm->dwStartTicks = GetTickCount();
	
	/* FIXME(EPP): "I don't understand the content of the first MIDIHDR sent
	 * by native mcimidi, it doesn't look like a correct one".
	 * this trick allows to throw it away... but I don't like it. 
	 * It looks like part of the file I'm trying to play and definitively looks 
	 * like raw midi content
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
	lpMidiHdr = (LPMIDIHDR)msg->lParam;
	lpData = lpMidiHdr->lpData;
	TRACE("Adding %s lpMidiHdr=%p [lpData=0x%08lx dwBufferLength=%lu/%lu dwFlags=0x%08lx size=%u]\n", 
	      (lpMidiHdr->dwFlags & MHDR_ISSTRM) ? "stream" : "regular", lpMidiHdr, 
	      (DWORD)lpMidiHdr, lpMidiHdr->dwBufferLength, lpMidiHdr->dwBytesRecorded, 
	      lpMidiHdr->dwFlags, msg->wParam);
#if 0
	/* dumps content of lpMidiHdr->lpData
	 * FIXME: there should be a debug routine somewhere that already does this
	 * I hate spreading this type of shit all around the code 
	 */ 
	for (dwToGo = 0; dwToGo < lpMidiHdr->dwBufferLength; dwToGo += 16) {
	    DWORD	i;
	    BYTE	ch;
	    
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
	    ((LPMIDIEVENT)lpData)->dwStreamID != (DWORD)lpMidiStrm) {
	    FIXME("Dropping bad %s lpMidiHdr (streamID=%08lx)\n", 
		  (lpMidiHdr->dwFlags & MHDR_ISSTRM) ? "stream" : "regular", 
		  ((LPMIDIEVENT)lpData)->dwStreamID);
	    lpMidiHdr->dwFlags |= MHDR_DONE;
	    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	    
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags, lpMidiStrm->hDevice, 
			   MM_MOM_DONE, lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
	    break;
	} 
	
	for (lpmh = &lpMidiStrm->lpMidiHdr; *lpmh; lpmh = (LPMIDIHDR*)&((*lpmh)->lpNext));
	*lpmh = lpMidiHdr;
	lpMidiHdr = (LPMIDIHDR)msg->lParam;
	lpMidiHdr->lpNext = 0;
	lpMidiHdr->dwFlags |= MHDR_INQUEUE;
	lpMidiHdr->dwFlags &= MHDR_DONE;
	lpMidiHdr->dwOffset = 0;
	
	break;
    default:
	FIXME("Unknown message %d\n", msg->message);
	break;
    }
    return TRUE;
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
    LPMIDIEVENT 	me;
    LPBYTE		lpData = 0;

    TRACE("(%p)!\n", lpMidiStrm);

    if (!lpMidiStrm || 
	(lpwm = (LPWINE_MIDI)MMDRV_Get(lpMidiStrm->hDevice, MMDRV_MIDIOUT, FALSE)) == NULL)
	goto the_end;

    /* force thread's queue creation */
    /* Used to be InitThreadInput16(0, 5); */
    /* but following works also with hack in midiStreamOpen */
    Callout.PeekMessageA(&msg, 0, 0, 0, 0);

    /* FIXME: this next line must be called before midiStreamOut or midiStreamRestart are called */
    SetEvent(lpMidiStrm->hEvent);
    TRACE("Ready to go 1\n");
    /* thread is started in paused mode */
    SuspendThread(lpMidiStrm->hThread);
    TRACE("Ready to go 2\n");

    lpMidiStrm->dwStartTicks = 0;    
    lpMidiStrm->dwPulses = 0;

    lpMidiStrm->lpMidiHdr = 0;

    for (;;) {
	lpMidiHdr = lpMidiStrm->lpMidiHdr;
	if (!lpMidiHdr) {
	    /* for first message, block until one arrives, then process all that are available */
	    Callout.GetMessageA(&msg, 0, 0, 0);
	    do {
		if (!MMSYSTEM_MidiStream_MessageHandler(lpMidiStrm, lpwm, &msg))
		    goto the_end;
	    } while (Callout.PeekMessageA(&msg, 0, 0, 0, PM_REMOVE));
	    lpData = 0;
	    continue;
	}

	if (!lpData)
	    lpData = lpMidiHdr->lpData;
	    
	me = (LPMIDIEVENT)(lpData + lpMidiHdr->dwOffset);
	
	/* do we have to wait ? */
	if (me->dwDeltaTime) {
	    lpMidiStrm->dwPositionMS += MMSYSTEM_MidiStream_Convert(lpMidiStrm, me->dwDeltaTime);
	    lpMidiStrm->dwPulses += me->dwDeltaTime;

	    dwToGo = lpMidiStrm->dwStartTicks + lpMidiStrm->dwPositionMS;
			    
	    TRACE("%ld/%ld/%ld\n", dwToGo, GetTickCount(), me->dwDeltaTime);
	    while ((dwCurrTC = GetTickCount()) < dwToGo) {
		if (MsgWaitForMultipleObjects(0, NULL, FALSE, dwToGo - dwCurrTC, QS_ALLINPUT) == WAIT_OBJECT_0) {
		    /* got a message, handle it */
		    while (Callout.PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
			if (!MMSYSTEM_MidiStream_MessageHandler(lpMidiStrm, lpwm, &msg))
			    goto the_end;
		    }
		    lpData = 0;
		} else {
		    /* timeout, so me->dwDeltaTime is elapsed, can break the while loop */
		    break;
		}
	    }		    
	}
	switch (MEVT_EVENTTYPE(me->dwEvent & ~MEVT_F_CALLBACK)) {
	case MEVT_COMMENT:
	    FIXME("NIY: MEVT_COMMENT\n");
	    /* do nothing, skip bytes */
	    break;
	case MEVT_LONGMSG:
	    FIXME("NIY: MEVT_LONGMSG, aka sending Sysex event\n");
	    break;
	case MEVT_NOP:
	    break;
	case MEVT_SHORTMSG:
	    midiOutShortMsg(lpMidiStrm->hDevice, MEVT_EVENTPARM(me->dwEvent));
	    break;
	case MEVT_TEMPO:
	    lpMidiStrm->dwTempo = MEVT_EVENTPARM(me->dwEvent);
	    break;
	case MEVT_VERSION:
	    break;
	default:
	    FIXME("Unknown MEVT (0x%02x)\n", MEVT_EVENTTYPE(me->dwEvent & ~MEVT_F_CALLBACK));
	    break;
	}
	if (me->dwEvent & MEVT_F_CALLBACK) {
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags, lpMidiStrm->hDevice, 
			   MM_MOM_POSITIONCB, lpwm->mod.dwInstance, (LPARAM)lpMidiHdr, 0L);
	}
	lpMidiHdr->dwOffset += sizeof(MIDIEVENT);
	if (me->dwEvent & MEVT_F_LONG) 
	    lpMidiHdr->dwOffset += (MEVT_EVENTPARM(me->dwEvent) + 3) & ~3;
	if (lpMidiHdr->dwOffset >= lpMidiHdr->dwBufferLength) {
	    /* done with this header */
	    lpMidiHdr->dwFlags |= MHDR_DONE;
	    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;
	    
	    lpMidiStrm->lpMidiHdr = (LPMIDIHDR)lpMidiHdr->lpNext;
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags, lpMidiStrm->hDevice, 
			   MM_MOM_DONE, lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
	    lpData = 0;
	}
    }
the_end:
    TRACE("End of thread\n");
    ExitThread(0);
    return 0;	/* for removing the warning, never executed */
}

/**************************************************************************
 * 				MMSYSTEM_MidiStream_PostMessage	[internal]
 */
static	BOOL MMSYSTEM_MidiStream_PostMessage(WINE_MIDIStream* lpMidiStrm, WORD msg, DWORD pmt1, DWORD pmt2)
{
    if (Callout.PostThreadMessageA(lpMidiStrm->dwThreadID, msg, pmt1, pmt2)) {
	DWORD	count;
	BOOL	bHasWin16Lock;

	/* FIXME: should use the new syslevel APIs */
	if ((bHasWin16Lock = _ConfirmWin16Lock()) != 0) {
	    ReleaseThunkLock(&count);
	}
	WaitForSingleObject(lpMidiStrm->hEvent, INFINITE);
	if (bHasWin16Lock) {
	    RestoreThunkLock(count);
	}
    } else {
	WARN("bad PostThreadMessageA\n");
	return FALSE;
    }	
    return TRUE;
}

/**************************************************************************
 * 				midiStreamClose			[WINMM.90]
 */
MMRESULT WINAPI midiStreamClose(HMIDISTRM hMidiStrm)
{
    WINE_MIDIStream*	lpMidiStrm;

    TRACE("(%08x)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL))
	return MMSYSERR_INVALHANDLE;

    midiStreamStop(hMidiStrm);
    MMSYSTEM_MidiStream_PostMessage(lpMidiStrm, WM_QUIT, 0, 0);
    HeapFree(GetProcessHeap(), 0, lpMidiStrm);
    CloseHandle(lpMidiStrm->hEvent);

    return midiOutClose(hMidiStrm);
}

/**************************************************************************
 * 				MMSYSTEM_MidiStream_Open	[internal]
 */
static	MMRESULT WINAPI MMSYSTEM_MidiStream_Open(HMIDISTRM* lphMidiStrm, LPUINT lpuDeviceID, 
						 DWORD cMidi, DWORD dwCallback, 
						 DWORD dwInstance, DWORD fdwOpen, BOOL bFrom32) 
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret;
    MIDIOPENSTRMID	mosm;
    LPWINE_MIDI		lpwm;
    HMIDIOUT		hMidiOut;

    TRACE("(%p, %p, %ld, 0x%08lx, 0x%08lx, 0x%08lx)!\n",
	  lphMidiStrm, lpuDeviceID, cMidi, dwCallback, dwInstance, fdwOpen);

    if (cMidi != 1 || lphMidiStrm == NULL || lpuDeviceID == NULL)
	return MMSYSERR_INVALPARAM;

    lpMidiStrm = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_MIDIStream));
    if (!lpMidiStrm)
	return MMSYSERR_NOMEM;

    lpMidiStrm->dwTempo = 500000;
    lpMidiStrm->dwTimeDiv = 480; 	/* 480 is 120 quater notes per minute *//* FIXME ??*/
    lpMidiStrm->dwPositionMS = 0;

    mosm.dwStreamID = (DWORD)lpMidiStrm;
    /* FIXME: the correct value is not allocated yet for MAPPER */
    mosm.wDeviceID  = *lpuDeviceID;
    lpwm = MIDI_OutAlloc(&hMidiOut, &dwCallback, &dwInstance, &fdwOpen, 1, &mosm, bFrom32);
    lpMidiStrm->hDevice = hMidiOut;
    if (lphMidiStrm)
	*lphMidiStrm = hMidiOut;

    /* FIXME: is lpuDevice initialized upon entering midiStreamOpen ? */
    FIXME("*lpuDeviceID=%u\n", *lpuDeviceID);
    lpwm->mld.uDeviceID = *lpuDeviceID = 0;

    ret = MMDRV_Open(&lpwm->mld, MODM_OPEN, (DWORD)&lpwm->mod, fdwOpen);
    lpMidiStrm->hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
    lpMidiStrm->wFlags = HIWORD(fdwOpen);

    lpMidiStrm->hThread = CreateThread(NULL, 0, MMSYSTEM_MidiStream_Player, 
				       lpMidiStrm, 0, &(lpMidiStrm->dwThreadID));

    if (!lpMidiStrm->hThread) {
	midiStreamClose((HMIDISTRM)hMidiOut);
	return MMSYSERR_NOMEM;	
    }

    /* wait for thread to have started, and for it's queue to be created */
    {
	DWORD	count;
	BOOL	bHasWin16Lock;

	/* (Release|Restore)ThunkLock() is needed when this method is called from 16 bit code, 
	 * (meaning the Win16Lock is set), so that it's released and the 32 bit thread running 
	 * MMSYSTEM_MidiStreamPlayer can acquire Win16Lock to create its queue.
	 */
	if ((bHasWin16Lock = _ConfirmWin16Lock()) != 0) {
	    ReleaseThunkLock(&count);
	}
	WaitForSingleObject(lpMidiStrm->hEvent, INFINITE);
	if (bHasWin16Lock) {
	    RestoreThunkLock(count);
	}
    }

    TRACE("=> (%u/%d) hMidi=0x%04x ret=%d lpMidiStrm=%p\n", 
	  *lpuDeviceID, lpwm->mld.uDeviceID, *lphMidiStrm, ret, lpMidiStrm);	
    return ret;
}

/**************************************************************************
 * 				midiStreamOpen			[WINMM.91]
 */
MMRESULT WINAPI midiStreamOpen(HMIDISTRM* lphMidiStrm, LPUINT lpuDeviceID, 
			       DWORD cMidi, DWORD dwCallback, 
			       DWORD dwInstance, DWORD fdwOpen) 
{
    return MMSYSTEM_MidiStream_Open(lphMidiStrm, lpuDeviceID, cMidi, dwCallback, 
				    dwInstance, fdwOpen, TRUE);
}

/**************************************************************************
 * 				midiStreamOut			[WINMM.92]
 */
MMRESULT WINAPI midiStreamOut(HMIDISTRM hMidiStrm, LPMIDIHDR lpMidiHdr, 
			      UINT cbMidiHdr) 
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%08x, %p, %u)!\n", hMidiStrm, lpMidiHdr, cbMidiHdr);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else {
	if (!Callout.PostThreadMessageA(lpMidiStrm->dwThreadID, 
					WINE_MSM_HEADER, cbMidiHdr, 
					(DWORD)lpMidiHdr)) {
	    WARN("bad PostThreadMessageA\n");
	    ret = MMSYSERR_ERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamPause			[WINMM.93]
 */
MMRESULT WINAPI midiStreamPause(HMIDISTRM hMidiStrm) 
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%08x)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else {
	if (SuspendThread(lpMidiStrm->hThread) == 0xFFFFFFFF) {
	    WARN("bad Suspend (%ld)\n", GetLastError());
	    ret = MMSYSERR_ERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamPosition		[WINMM.94]
 */
MMRESULT WINAPI midiStreamPosition(HMIDISTRM hMidiStrm, LPMMTIME lpMMT, UINT cbmmt) 
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%08x, %p, %u)!\n", hMidiStrm, lpMMT, cbmmt);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else if (lpMMT == NULL || cbmmt != sizeof(MMTIME)) {
	ret = MMSYSERR_INVALPARAM;
    } else {
	switch (lpMMT->wType) {
	case TIME_MS:	
	    lpMMT->u.ms = lpMidiStrm->dwPositionMS;	
	    TRACE("=> %ld ms\n", lpMMT->u.ms);
	    break;
	case TIME_TICKS:
	    lpMMT->u.ticks = lpMidiStrm->dwPulses;	
	    TRACE("=> %ld ticks\n", lpMMT->u.ticks);
	    break;
	default:
	    WARN("Unsupported time type %d\n", lpMMT->wType);
	    lpMMT->wType = TIME_MS;
	    ret = MMSYSERR_INVALPARAM;
	    break;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamProperty		[WINMM.95]
 */
MMRESULT WINAPI midiStreamProperty(HMIDISTRM hMidiStrm, LPBYTE lpPropData, DWORD dwProperty) 
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(%08x, %p, %lx)\n", hMidiStrm, lpPropData, dwProperty);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else if ((dwProperty & (MIDIPROP_GET|MIDIPROP_SET)) == 0) {
	ret = MMSYSERR_INVALPARAM;
    } else if (dwProperty & MIDIPROP_TEMPO) {
	MIDIPROPTEMPO*	mpt = (MIDIPROPTEMPO*)lpPropData;
	
	if (sizeof(MIDIPROPTEMPO) != mpt->cbStruct) {
	    ret = MMSYSERR_INVALPARAM;
	} else if (dwProperty & MIDIPROP_SET) {
	    lpMidiStrm->dwTempo = mpt->dwTempo;
	    TRACE("Setting tempo to %ld\n", mpt->dwTempo);
	} else if (dwProperty & MIDIPROP_GET) {
	    mpt->dwTempo = lpMidiStrm->dwTempo;
	    TRACE("Getting tempo <= %ld\n", mpt->dwTempo);
	}
    } else if (dwProperty & MIDIPROP_TIMEDIV) {
	MIDIPROPTIMEDIV*	mptd = (MIDIPROPTIMEDIV*)lpPropData;
	
	if (sizeof(MIDIPROPTIMEDIV) != mptd->cbStruct) {
	    ret = MMSYSERR_INVALPARAM;
	} else if (dwProperty & MIDIPROP_SET) {
	    lpMidiStrm->dwTimeDiv = mptd->dwTimeDiv;
	    TRACE("Setting time div to %ld\n", mptd->dwTimeDiv);
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
 * 				midiStreamRestart		[WINMM.96]
 */
MMRESULT WINAPI midiStreamRestart(HMIDISTRM hMidiStrm) 
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(%08x)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else {
	DWORD	ret;

	/* since we increase the thread suspend count on each midiStreamPause
	 * there may be a need for several midiStreamResume
	 */
	do {
	    ret = ResumeThread(lpMidiStrm->hThread);
	} while (ret != 0xFFFFFFFF && ret != 0);
	if (ret == 0xFFFFFFFF) {
	    WARN("bad Resume (%ld)\n", GetLastError());
	    ret = MMSYSERR_ERROR;
	} else {
	    lpMidiStrm->dwStartTicks = GetTickCount() - lpMidiStrm->dwPositionMS;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamStop			[WINMM.97]
 */
MMRESULT WINAPI midiStreamStop(HMIDISTRM hMidiStrm) 
{
    WINE_MIDIStream*	lpMidiStrm;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(%08x)!\n", hMidiStrm);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else {
	/* in case stream has been paused... FIXME is the current state correct ? */
	midiStreamRestart(hMidiStrm);
	MMSYSTEM_MidiStream_PostMessage(lpMidiStrm, WINE_MSM_STOP, 0, 0);
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamClose			[MMSYSTEM.252]
 */
MMRESULT16 WINAPI midiStreamClose16(HMIDISTRM16 hMidiStrm)
{
    return midiStreamClose(hMidiStrm);
}

/**************************************************************************
 * 				midiStreamOpen			[MMSYSTEM.251]
 */
MMRESULT16 WINAPI midiStreamOpen16(HMIDISTRM16* phMidiStrm, LPUINT16 devid, 
				   DWORD cMidi, DWORD dwCallback, 
				   DWORD dwInstance, DWORD fdwOpen) 
{
    HMIDISTRM	hMidiStrm32;
    MMRESULT 	ret;
    UINT	devid32;

    if (!phMidiStrm || !devid)
	return MMSYSERR_INVALPARAM;
    devid32 = *devid;
    ret = MMSYSTEM_MidiStream_Open(&hMidiStrm32, &devid32, cMidi, dwCallback, 
				   dwInstance, fdwOpen, FALSE);
    *phMidiStrm = hMidiStrm32;
    *devid = devid32;
    return ret;
}

/**************************************************************************
 * 				midiStreamOut			[MMSYSTEM.254]
 */
MMRESULT16 WINAPI midiStreamOut16(HMIDISTRM16 hMidiStrm, LPMIDIHDR16 lpMidiHdr, UINT16 cbMidiHdr) 
{
    return midiStreamOut(hMidiStrm, (LPMIDIHDR)lpMidiHdr, cbMidiHdr);
}

/**************************************************************************
 * 				midiStreamPause			[MMSYSTEM.255]
 */
MMRESULT16 WINAPI midiStreamPause16(HMIDISTRM16 hMidiStrm) 
{
    return midiStreamPause(hMidiStrm);
}

/**************************************************************************
 * 				midiStreamPosition		[MMSYSTEM.253]
 */
MMRESULT16 WINAPI midiStreamPosition16(HMIDISTRM16 hMidiStrm, LPMMTIME16 lpmmt16, UINT16 cbmmt) 
{
    MMTIME	mmt32;
    MMRESULT	ret;

    if (!lpmmt16)
	return MMSYSERR_INVALPARAM;
    MMSYSTEM_MMTIME16to32(&mmt32, lpmmt16);
    ret = midiStreamPosition(hMidiStrm, &mmt32, sizeof(MMTIME));
    MMSYSTEM_MMTIME32to16(lpmmt16, &mmt32);
    return ret;
}

/**************************************************************************
 * 				midiStreamProperty		[MMSYSTEM.250]
 */
MMRESULT16 WINAPI midiStreamProperty16(HMIDISTRM16 hMidiStrm, LPBYTE lpPropData, DWORD dwProperty) 
{
    return midiStreamProperty(hMidiStrm, lpPropData, dwProperty);
}

/**************************************************************************
 * 				midiStreamRestart		[MMSYSTEM.256]
 */
MMRESULT16 WINAPI midiStreamRestart16(HMIDISTRM16 hMidiStrm) 
{
    return midiStreamRestart(hMidiStrm);
}

/**************************************************************************
 * 				midiStreamStop			[MMSYSTEM.257]
 */
MMRESULT16 WINAPI midiStreamStop16(HMIDISTRM16 hMidiStrm) 
{
    return midiStreamStop(hMidiStrm);
}

static	UINT WINAPI MMSYSTEM_waveOpen(HANDLE* lphndl, UINT uDeviceID, UINT uType,
				      const LPWAVEFORMATEX lpFormat, 
				      DWORD dwCallback, DWORD dwInstance, 
				      DWORD dwFlags, BOOL bFrom32)
{
    HANDLE		handle;
    LPWINE_MLD		wmld;
    DWORD		dwRet = MMSYSERR_NOERROR;
    WAVEOPENDESC	wod;

    TRACE("(%p, %d, %s, %p, %08lX, %08lX, %08lX, %d);\n", 
	  lphndl, (int)uDeviceID, (uType==MMDRV_WAVEOUT)?"Out":"In", lpFormat, dwCallback, 
	  dwInstance, dwFlags, bFrom32?32:16);

    if (dwFlags & WAVE_FORMAT_QUERY)	TRACE("WAVE_FORMAT_QUERY requested !\n");

    if (lpFormat == NULL) return WAVERR_BADFORMAT;

    TRACE("wFormatTag=%u, nChannels=%u, nSamplesPerSec=%lu, nAvgBytesPerSec=%lu, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u\n", 
	  lpFormat->wFormatTag, lpFormat->nChannels, lpFormat->nSamplesPerSec, 
	  lpFormat->nAvgBytesPerSec, lpFormat->nBlockAlign, lpFormat->wBitsPerSample, lpFormat->cbSize);
    
    if ((wmld = MMDRV_Alloc(sizeof(WINE_WAVE), uType, &handle, 
			    &dwFlags, &dwCallback, &dwInstance, bFrom32)) == NULL) 
	return MMSYSERR_NOMEM;

    wod.hWave = handle;
    wod.lpFormat = lpFormat;  /* should the struct be copied iso pointer? */
    wod.dwCallback = dwCallback;
    wod.dwInstance = dwInstance;
    wod.uMappedDeviceID = 0;
    wod.dnDevNode = 0L;

    /* when called from 16 bit code, mapper will be 0x0000FFFF instead of 0xFFFFFFFF */
    /* this should fix it */
    wmld->uDeviceID = (uDeviceID == (UINT16)-1 && !bFrom32) ? (UINT)-1 : uDeviceID;

    dwRet = MMDRV_Open(wmld, (uType==MMDRV_WAVEOUT)?WODM_OPEN:WIDM_OPEN, (DWORD)&wod, dwFlags);

    if ((dwFlags & WAVE_FORMAT_QUERY) || dwRet != MMSYSERR_NOERROR) {
	MMDRV_Free(handle, wmld);
	handle = 0;
    } 

    if (lphndl != NULL) *lphndl = handle;
    TRACE("=> %ld hWave=%04x\n", dwRet, handle);

    return dwRet;
}

/**************************************************************************
 * 				waveOutGetNumDevs		[MMSYSTEM.401]
 */
UINT WINAPI waveOutGetNumDevs(void) 
{
    return MMDRV_GetNum(MMDRV_WAVEOUT);
}

/**************************************************************************
 * 				waveOutGetNumDevs		[WINMM.167]
 */
UINT16 WINAPI waveOutGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_WAVEOUT);
}

/**************************************************************************
 * 				waveOutGetDevCaps		[MMSYSTEM.402]
 */
UINT16 WINAPI waveOutGetDevCaps16(UINT16 uDeviceID, 
				  LPWAVEOUTCAPS16 lpCaps, UINT16 uSize)
{
    WAVEOUTCAPSA	wocA;
    UINT 		ret;

    TRACE("(%u %p %u)!\n", uDeviceID, lpCaps, uSize);
    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    ret = waveOutGetDevCapsA(uDeviceID, &wocA, sizeof(wocA));

    if (ret == MMSYSERR_NOERROR) {
	lpCaps->wMid = wocA.wMid;
	lpCaps->wPid = wocA.wPid;
	lpCaps->vDriverVersion = wocA.vDriverVersion;
	strcpy(lpCaps->szPname, wocA.szPname);
	lpCaps->dwFormats = wocA.dwFormats;
	lpCaps->wChannels = wocA.wChannels;
	lpCaps->dwSupport = wocA.dwSupport;
    }
    return ret;
}

/**************************************************************************
 * 				waveOutGetDevCapsA		[WINMM.162]
 */
UINT WINAPI waveOutGetDevCapsA(UINT uDeviceID, LPWAVEOUTCAPSA lpCaps,
			       UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%u %p %u)!\n", uDeviceID, lpCaps, uSize);

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;
    
    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_WAVEOUT, TRUE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_GETDEVCAPS, (DWORD)lpCaps, uSize, TRUE);

}

/**************************************************************************
 * 				waveOutGetDevCapsW		[WINMM.163]
 */
UINT WINAPI waveOutGetDevCapsW(UINT uDeviceID, LPWAVEOUTCAPSW lpCaps,
			       UINT uSize)
{
    WAVEOUTCAPSA	wocA;
    UINT 		ret;

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;
    
    ret = waveOutGetDevCapsA(uDeviceID, &wocA, sizeof(wocA));

    if (ret == MMSYSERR_NOERROR) {
	lpCaps->wMid = wocA.wMid;
	lpCaps->wPid = wocA.wPid;
	lpCaps->vDriverVersion = wocA.vDriverVersion;
	lstrcpyAtoW(lpCaps->szPname, wocA.szPname);
	lpCaps->dwFormats = wocA.dwFormats;
	lpCaps->wChannels = wocA.wChannels;
	lpCaps->dwSupport = wocA.dwSupport;
    }
    return ret;
}

/**************************************************************************
 * 				WAVE_GetErrorText       	[internal]
 */
static	UINT16	WAVE_GetErrorText(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    UINT16		ret = MMSYSERR_BADERRNUM;

    if (lpText == NULL) {
	ret = MMSYSERR_INVALPARAM;
    } else if (uSize == 0) {
	ret = MMSYSERR_NOERROR;
    } else if (
	       /* test has been removed 'coz MMSYSERR_BASE is 0, and gcc did emit
		* a warning for the test was always true */
	       (/*uError >= MMSYSERR_BASE && */uError <= MMSYSERR_LASTERROR) ||
	       (uError >= WAVERR_BASE  && uError <= WAVERR_LASTERROR)) {
	
	if (LoadStringA(MULTIMEDIA_GetIData()->hWinMM32Instance, 
			uError, lpText, uSize) > 0) {
	    ret = MMSYSERR_NOERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				waveOutGetErrorText 	[MMSYSTEM.403]
 */
UINT16 WINAPI waveOutGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    return WAVE_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveOutGetErrorTextA 	[WINMM.164]
 */
UINT WINAPI waveOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return WAVE_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveOutGetErrorTextW 	[WINMM.165]
 */
UINT WINAPI waveOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = WAVE_GetErrorText(uError, xstr, uSize);
    
    lstrcpyAtoW(lpText, xstr);
    HeapFree(GetProcessHeap(), 0, xstr);
    return ret;
}

/**************************************************************************
 *			waveOutOpen			[WINMM.173]
 * All the args/structs have the same layout as the win16 equivalents
 */
UINT WINAPI waveOutOpen(HWAVEOUT* lphWaveOut, UINT uDeviceID,
			const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
			DWORD dwInstance, DWORD dwFlags)
{
    return MMSYSTEM_waveOpen(lphWaveOut, uDeviceID, MMDRV_WAVEOUT, lpFormat, 
			     dwCallback, dwInstance, dwFlags, TRUE);
}

/**************************************************************************
 *			waveOutOpen			[MMSYSTEM.404]
 */
UINT16 WINAPI waveOutOpen16(HWAVEOUT16* lphWaveOut, UINT16 uDeviceID,
                            const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
			    DWORD dwInstance, DWORD dwFlags)
{
    HWAVEOUT		hWaveOut;
    UINT		ret;

    /* since layout of WAVEOFORMATEX is the same for 16/32 bits, we directly
     * call the 32 bit version
     */
    ret = MMSYSTEM_waveOpen(&hWaveOut, uDeviceID, MMDRV_WAVEOUT, lpFormat, 
			    dwCallback, dwInstance, dwFlags, FALSE);

    if (lphWaveOut != NULL) *lphWaveOut = hWaveOut;
    return ret;
}

/**************************************************************************
 * 				waveOutClose		[WINMM.161]
 */
UINT WINAPI waveOutClose(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;
    
    TRACE("(%04X)\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Close(wmld, WODM_CLOSE);
    MMDRV_Free(hWaveOut, wmld);

    return dwRet;
}

/**************************************************************************
 * 				waveOutClose		[MMSYSTEM.405]
 */
UINT16 WINAPI waveOutClose16(HWAVEOUT16 hWaveOut)
{
    return waveOutClose(hWaveOut);
}

/**************************************************************************
 * 				waveOutPrepareHeader	[WINMM.175]
 */
UINT WINAPI waveOutPrepareHeader(HWAVEOUT hWaveOut,
				 WAVEHDR* lpWaveOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_PREPARE, (DWORD)lpWaveOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveOutPrepareHeader	[MMSYSTEM.406]
 */
UINT16 WINAPI waveOutPrepareHeader16(HWAVEOUT16 hWaveOut,
                                     WAVEHDR* /*SEGPTR*/ lpsegWaveOutHdr, 
				     UINT16 uSize)
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveOutHdr = (LPWAVEHDR)PTR_SEG_TO_LIN(lpsegWaveOutHdr);
    
    TRACE("(%04X, %p, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_PREPARE, (DWORD)lpsegWaveOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[WINMM.181]
 */
UINT WINAPI waveOutUnprepareHeader(HWAVEOUT hWaveOut,
				   LPWAVEHDR lpWaveOutHdr, UINT uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    if (!(lpWaveOutHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_UNPREPARE, (DWORD)lpWaveOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[MMSYSTEM.407]
 */
UINT16 WINAPI waveOutUnprepareHeader16(HWAVEOUT16 hWaveOut,
				       LPWAVEHDR /*SEGPTR*/ lpsegWaveOutHdr, 
				       UINT16 uSize)
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveOutHdr = (LPWAVEHDR)PTR_SEG_TO_LIN(lpsegWaveOutHdr);
    
    TRACE("(%04X, %p, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if (!(lpWaveOutHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_UNPREPARE, (DWORD)lpsegWaveOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveOutWrite		[MMSYSTEM.408]
 */
UINT WINAPI waveOutWrite(HWAVEOUT hWaveOut, LPWAVEHDR lpWaveOutHdr,
			 UINT uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %u);\n", hWaveOut, lpWaveOutHdr, uSize);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_WRITE, (DWORD)lpWaveOutHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveOutWrite		[MMSYSTEM.408]
 */
UINT16 WINAPI waveOutWrite16(HWAVEOUT16 hWaveOut, 
			     LPWAVEHDR /*SEGPTR*/ lpsegWaveOutHdr,
			     UINT16 uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_WRITE, (DWORD)lpsegWaveOutHdr, uSize, FALSE);
}

#define WAVEOUT_SHORTCUT_0(xx, XX) 				\
UINT WINAPI waveOut##xx(HWAVEOUT hWaveOut)			\
{								\
    LPWINE_MLD		wmld;					\
    TRACE("(%04X);\n", hWaveOut);           			\
    if ((wmld = MMDRV_Get(hWaveOut,MMDRV_WAVEOUT,FALSE))==NULL)	\
        return MMSYSERR_INVALHANDLE;				\
    return MMDRV_Message(wmld,WODM_##XX,0L,0L,TRUE);		\
}								\
UINT16 WINAPI waveOut##xx##16(HWAVEOUT16 hWaveOut16)		\
{return waveOut##xx(hWaveOut16);}

WAVEOUT_SHORTCUT_0(BreakLoop,	BREAKLOOP)
WAVEOUT_SHORTCUT_0(Pause,	PAUSE)
WAVEOUT_SHORTCUT_0(Reset,	RESET)
WAVEOUT_SHORTCUT_0(Restart,	RESTART)
#undef WAVEOUT_SHORTCUT_0

/**************************************************************************
 * 				waveOutGetPosition	[WINMM.170]
 */
UINT WINAPI waveOutGetPosition(HWAVEOUT hWaveOut, LPMMTIME lpTime,
			       UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %u);\n", hWaveOut, lpTime, uSize);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_GETPOS, (DWORD)lpTime, uSize, TRUE);
}

/**************************************************************************
 * 				waveOutGetPosition	[MMSYSTEM.412]
 */
UINT16 WINAPI waveOutGetPosition16(HWAVEOUT16 hWaveOut, LPMMTIME16 lpTime,
                                   UINT16 uSize)
{
    UINT	ret;
    MMTIME	mmt;

    mmt.wType = lpTime->wType;
    ret = waveOutGetPosition(hWaveOut, &mmt, sizeof(mmt));
    MMSYSTEM_MMTIME32to16(lpTime, &mmt);
    return ret;
}

#define WAVEOUT_SHORTCUT_1(xx, XX, atype) 			\
UINT WINAPI waveOut##xx(HWAVEOUT hWaveOut, atype x)		\
{								\
    LPWINE_MLD		wmld;					\
    TRACE("(%04X, %08lx);\n", hWaveOut, (DWORD)x);           	\
    if ((wmld = MMDRV_Get(hWaveOut,MMDRV_WAVEOUT,FALSE))==NULL)	\
        return MMSYSERR_INVALHANDLE;				\
    return MMDRV_Message(wmld,WODM_##XX,(DWORD)x,0L,TRUE);	\
}								\
UINT16 WINAPI waveOut##xx##16(HWAVEOUT16 hWaveOut16, atype x)	\
{return waveOut##xx(hWaveOut16, x);}

WAVEOUT_SHORTCUT_1(GetPitch, 	    GETPITCH, 	     LPDWORD)
WAVEOUT_SHORTCUT_1(SetPitch, 	    SETPITCH, 	     DWORD)
WAVEOUT_SHORTCUT_1(GetPlaybackRate, GETPLAYBACKRATE, LPDWORD)
WAVEOUT_SHORTCUT_1(SetPlaybackRate, SETPLAYBACKRATE, DWORD)
#undef WAVEOUT_SHORTCUT_1
    
#define WAVEOUT_SHORTCUT_2(xx, XX, atype) 			\
UINT WINAPI waveOut##xx(UINT devid, atype x)			\
{								\
    LPWINE_MLD		wmld;					\
    TRACE("(%04X, %08lx);\n", devid, (DWORD)x);	        	\
     if ((wmld = MMDRV_Get(devid,MMDRV_WAVEOUT,TRUE))==NULL)	\
        return MMSYSERR_INVALHANDLE;				\
    return MMDRV_Message(wmld, WODM_##XX, (DWORD)x, 0L, TRUE);	\
}								\
UINT16 WINAPI waveOut##xx##16(UINT16 devid, atype x)		\
{return waveOut##xx(devid, x);}
    
WAVEOUT_SHORTCUT_2(GetVolume, GETVOLUME, LPDWORD)
WAVEOUT_SHORTCUT_2(SetVolume, SETVOLUME, DWORD)
#undef WAVEOUT_SHORTCUT_2
    
/**************************************************************************
 * 				waveOutGetID	 	[MMSYSTEM.420]
 */
UINT WINAPI waveOutGetID(HWAVEOUT hWaveOut, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p);\n", hWaveOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveOutGetID	 	[MMSYSTEM.420]
 */
UINT16 WINAPI waveOutGetID16(HWAVEOUT16 hWaveOut, UINT16* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p);\n", hWaveOut, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveOutMessage 		[WINMM.172]
 */
DWORD WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage, 
			    DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04x, %u, %ld, %ld)\n", hWaveOut, uMessage, dwParam1, dwParam2);

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				waveOutMessage 		[MMSYSTEM.421]
 */
DWORD WINAPI waveOutMessage16(HWAVEOUT16 hWaveOut, UINT16 uMessage, 
                              DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04x, %u, %ld, %ld)\n", hWaveOut, uMessage, dwParam1, dwParam2);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL) {
	if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, TRUE)) != NULL) {
	    return MMDRV_PhysicalFeatures(wmld, uMessage, dwParam1, dwParam2);
	}
	return MMSYSERR_INVALHANDLE;
    }

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER))
	return MMSYSERR_INVALPARAM;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, FALSE);
}

/**************************************************************************
 * 				waveInGetNumDevs 		[WINMM.151]
 */
UINT WINAPI waveInGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_WAVEIN);
}

/**************************************************************************
 * 				waveInGetNumDevs 		[MMSYSTEM.501]
 */
UINT16 WINAPI waveInGetNumDevs16(void)
{
    return MMDRV_GetNum(MMDRV_WAVEIN);
}

/**************************************************************************
 * 				waveInGetDevCapsW 		[WINMM.147]
 */
UINT WINAPI waveInGetDevCapsW(UINT uDeviceID, LPWAVEINCAPSW lpCaps, UINT uSize)
{
    WAVEINCAPSA		wicA;
    UINT		ret = waveInGetDevCapsA(uDeviceID, &wicA, uSize);

    if (ret == MMSYSERR_NOERROR) {
	lpCaps->wMid = wicA.wMid;
	lpCaps->wPid = wicA.wPid;
	lpCaps->vDriverVersion = wicA.vDriverVersion;
	lstrcpyAtoW(lpCaps->szPname, wicA.szPname);
	lpCaps->dwFormats = wicA.dwFormats;
	lpCaps->wChannels = wicA.wChannels;
    }
    
    return ret;
}

/**************************************************************************
 * 				waveInGetDevCapsA 		[WINMM.146]
 */
UINT WINAPI waveInGetDevCapsA(UINT uDeviceID, LPWAVEINCAPSA lpCaps, UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%u %p %u)!\n", uDeviceID, lpCaps, uSize);

    if ((wmld = MMDRV_Get(uDeviceID, MMDRV_WAVEIN, TRUE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_GETDEVCAPS, (DWORD)lpCaps, uSize, TRUE);
}

/**************************************************************************
 * 				waveInGetDevCaps 		[MMSYSTEM.502]
 */
UINT16 WINAPI waveInGetDevCaps16(UINT16 uDeviceID, LPWAVEINCAPS16 lpCaps, 
				 UINT16 uSize)
{
    WAVEINCAPSA	wicA;
    UINT	ret = waveInGetDevCapsA(uDeviceID, &wicA, sizeof(wicA));

    if (lpCaps == NULL)	return MMSYSERR_INVALPARAM;

    if (ret == MMSYSERR_NOERROR) {
	lpCaps->wMid = wicA.wMid;
	lpCaps->wPid = wicA.wPid;
	lpCaps->vDriverVersion = wicA.vDriverVersion;
	strcpy(lpCaps->szPname, wicA.szPname);
	lpCaps->dwFormats = wicA.dwFormats;
	lpCaps->wChannels = wicA.wChannels;
    }
    return ret;
}

/**************************************************************************
 * 				waveInGetErrorTextA 	[WINMM.148]
 */
UINT WINAPI waveInGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return WAVE_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveInGetErrorTextW 	[WINMM.149]
 */
UINT WINAPI waveInGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR txt = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = WAVE_GetErrorText(uError, txt, uSize);
    
    lstrcpyAtoW(lpText, txt);
    HeapFree(GetProcessHeap(), 0, txt);
    return ret;
}

/**************************************************************************
 * 				waveInGetErrorText 	[MMSYSTEM.503]
 */
UINT16 WINAPI waveInGetErrorText16(UINT16 uError, LPSTR lpText, UINT16 uSize)
{
    return WAVE_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveInOpen			[WINMM.154]
 */
UINT WINAPI waveInOpen(HWAVEIN* lphWaveIn, UINT uDeviceID,
		       const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
		       DWORD dwInstance, DWORD dwFlags)
{
    return MMSYSTEM_waveOpen(lphWaveIn, uDeviceID, MMDRV_WAVEIN, lpFormat, 
			     dwCallback, dwInstance, dwFlags, TRUE);
}

/**************************************************************************
 * 				waveInOpen			[MMSYSTEM.504]
 */
UINT16 WINAPI waveInOpen16(HWAVEIN16* lphWaveIn, UINT16 uDeviceID,
                           const LPWAVEFORMATEX lpFormat, DWORD dwCallback,
                           DWORD dwInstance, DWORD dwFlags)
{
    HWAVEIN		hWaveIn;
    UINT		ret;

    /* since layout of WAVEOFORMATEX is the same for 16/32 bits, we directly
     * call the 32 bit version
     */
    ret = MMSYSTEM_waveOpen(&hWaveIn, uDeviceID, MMDRV_WAVEIN, lpFormat, 
			    dwCallback, dwInstance, dwFlags, FALSE);

    if (lphWaveIn != NULL) *lphWaveIn = hWaveIn;
    return ret;
}

/**************************************************************************
 * 				waveInClose			[WINMM.145]
 */
UINT WINAPI waveInClose(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;
    DWORD		dwRet;
    
    TRACE("(%04X)\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    dwRet = MMDRV_Message(wmld, WIDM_CLOSE, 0L, 0L, TRUE);
    MMDRV_Free(hWaveIn, wmld);
    return dwRet;
}

/**************************************************************************
 * 				waveInClose			[MMSYSTEM.505]
 */
UINT16 WINAPI waveInClose16(HWAVEIN16 hWaveIn)
{
    return waveOutClose(hWaveIn);
}

/**************************************************************************
 * 				waveInPrepareHeader		[WINMM.155]
 */
UINT WINAPI waveInPrepareHeader(HWAVEIN hWaveIn, WAVEHDR* lpWaveInHdr, 
				UINT uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    lpWaveInHdr->dwBytesRecorded = 0;

    return MMDRV_Message(wmld, WIDM_PREPARE, (DWORD)lpWaveInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveInPrepareHeader		[MMSYSTEM.506]
 */
UINT16 WINAPI waveInPrepareHeader16(HWAVEIN16 hWaveIn,
				    WAVEHDR* /* SEGPTR */ lpsegWaveInHdr, 
				    UINT16 uSize)
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveInHdr = (LPWAVEHDR)PTR_SEG_TO_LIN(lpsegWaveInHdr);
    UINT16		ret;
    
    TRACE("(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    lpWaveInHdr->dwBytesRecorded = 0;

    ret = MMDRV_Message(wmld, WIDM_PREPARE, (DWORD)lpsegWaveInHdr, uSize, FALSE);
    return ret;
}

/**************************************************************************
 * 				waveInUnprepareHeader	[WINMM.159]
 */
UINT WINAPI waveInUnprepareHeader(HWAVEIN hWaveIn, WAVEHDR* lpWaveInHdr, 
				  UINT uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALPARAM;
    if (!(lpWaveInHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_UNPREPARE, (DWORD)lpWaveInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveInUnprepareHeader	[MMSYSTEM.507]
 */
UINT16 WINAPI waveInUnprepareHeader16(HWAVEIN16 hWaveIn, 
				      WAVEHDR* /* SEGPTR */ lpsegWaveInHdr, 
				      UINT16 uSize)
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveInHdr = (LPWAVEHDR)PTR_SEG_TO_LIN(lpsegWaveInHdr);
    
    TRACE("(%04X, %p, %u);\n", hWaveIn, lpsegWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALPARAM;

    if (!(lpWaveInHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_UNPREPARE, (DWORD)lpsegWaveInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveInAddBuffer		[WINMM.144]
 */
UINT WINAPI waveInAddBuffer(HWAVEIN hWaveIn,
			    WAVEHDR* lpWaveInHdr, UINT uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_ADDBUFFER, (DWORD)lpWaveInHdr, uSize, TRUE);
}

/**************************************************************************
 * 				waveInAddBuffer		[MMSYSTEM.508]
 */
UINT16 WINAPI waveInAddBuffer16(HWAVEIN16 hWaveIn, 
				WAVEHDR* /* SEGPTR */ lpsegWaveInHdr, 
				UINT16 uSize)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04X, %p, %u);\n", hWaveIn, lpsegWaveInHdr, uSize);

    if (lpsegWaveInHdr == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_ADDBUFFER, (DWORD)lpsegWaveInHdr, uSize, FALSE);
}

#define WAVEIN_SHORTCUT_0(xx, XX) 				\
UINT WINAPI waveIn##xx(HWAVEIN hWaveIn)				\
{								\
    LPWINE_MLD		wmld;					\
    TRACE("(%04X);\n", hWaveIn);           			\
    if ((wmld=MMDRV_Get(hWaveIn,MMDRV_WAVEIN,FALSE)) == NULL) 	\
	return MMSYSERR_INVALHANDLE;				\
    return MMDRV_Message(wmld,WIDM_##XX,0L,0L,TRUE);		\
}								\
UINT16 WINAPI waveIn##xx##16(HWAVEIN16 hWaveIn16)		\
{return waveIn##xx(hWaveIn16);}

WAVEIN_SHORTCUT_0(Reset,	RESET)
WAVEIN_SHORTCUT_0(Start,	START)
WAVEIN_SHORTCUT_0(Stop,		STOP)
#undef WAVEIN_SHORTCUT_0

/**************************************************************************
 * 				waveInGetPosition	[WINMM.152]
 */
UINT WINAPI waveInGetPosition(HWAVEIN hWaveIn, LPMMTIME lpTime,
			      UINT uSize)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %u);\n", hWaveIn, lpTime, uSize);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_GETPOS, (DWORD)lpTime, uSize, TRUE);
}

/**************************************************************************
 * 				waveInGetPosition	[MMSYSTEM.512]
 */
UINT16 WINAPI waveInGetPosition16(HWAVEIN16 hWaveIn, LPMMTIME16 lpTime,
                                  UINT16 uSize)
{
    UINT	ret;
    MMTIME	mmt;

    mmt.wType = lpTime->wType;
    ret = waveInGetPosition(hWaveIn, &mmt, sizeof(mmt));
    MMSYSTEM_MMTIME32to16(lpTime, &mmt);
    return ret;
}

/**************************************************************************
 * 				waveInGetID			[WINMM.150]
 */
UINT WINAPI waveInGetID(HWAVEIN hWaveIn, UINT* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p);\n", hWaveIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInGetID			[MMSYSTEM.513]
 */
UINT16 WINAPI waveInGetID16(HWAVEIN16 hWaveIn, UINT16* lpuDeviceID)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p);\n", hWaveIn, lpuDeviceID);

    if (lpuDeviceID == NULL) return MMSYSERR_INVALHANDLE;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInMessage 		[WINMM.153]
 */
DWORD WINAPI waveInMessage(HWAVEIN hWaveIn, UINT uMessage,
			   DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;
    
    TRACE("(%04x, %u, %ld, %ld)\n", hWaveIn, uMessage, dwParam1, dwParam2);

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				waveInMessage 		[MMSYSTEM.514]
 */
DWORD WINAPI waveInMessage16(HWAVEIN16 hWaveIn, UINT16 uMessage,
                             DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04x, %u, %ld, %ld)\n", hWaveIn, uMessage, dwParam1, dwParam2);

    /* from M$ KB */
    if (uMessage < DRVM_IOCTL || (uMessage >= DRVM_IOCTL_LAST && uMessage < DRVM_MAPPER))
	return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL) 
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				DrvOpen	       		[MMSYSTEM.1100]
 */
HDRVR16 WINAPI DrvOpen(LPSTR lpDriverName, LPSTR lpSectionName, LPARAM lParam)
{
    TRACE("('%s','%s', %08lX);\n", lpDriverName, lpSectionName, lParam);

    return OpenDriver16(lpDriverName, lpSectionName, lParam);
}

/**************************************************************************
 * 				DrvClose       		[MMSYSTEM.1101]
 */
LRESULT WINAPI DrvClose(HDRVR16 hDrv, LPARAM lParam1, LPARAM lParam2)
{
    TRACE("(%04X, %08lX, %08lX);\n", hDrv, lParam1, lParam2);

    return CloseDriver16(hDrv, lParam1, lParam2);
}

/**************************************************************************
 * 				DrvSendMessage		[MMSYSTEM.1102]
 */
LRESULT WINAPI DrvSendMessage(HDRVR16 hDrv, WORD msg, LPARAM lParam1,
                              LPARAM lParam2)
{
    return SendDriverMessage(hDrv, msg, lParam1, lParam2);
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

/*#define USE_MM_TSK_WINE*/

/**************************************************************************
 * 				mmTaskCreate		[MMSYSTEM.900]
 *
 * Creates a 16 bit MM task. It's entry point is lpFunc, and it should be
 * called upon creation with dwPmt as parameter.
 */
HINSTANCE16 WINAPI mmTaskCreate16(SEGPTR spProc, HINSTANCE16 *lphMmTask, DWORD dwPmt)
{
    DWORD 		showCmd = 0x40002;
    LPSTR 		cmdline;
    WORD 		sel1, sel2;
    LOADPARAMS16*	lp;
    HINSTANCE16 	ret;
    HINSTANCE16		handle;
    
    TRACE("(%08lx, %p, %08lx);\n", spProc, lphMmTask, dwPmt);
    /* This to work requires NE modules to be started with a binary command line
     * which is not currently the case. A patch exists but has never been committed.
     * A workaround would be to integrate code for mmtask.tsk into Wine, but
     * this requires tremendous work (starting with patching tools/build to
     * create NE executables (and not only DLLs) for builtins modules.
     * EP 99/04/25
     */
    FIXME("This is currently broken. It will fail\n");

    cmdline = (LPSTR)HeapAlloc(GetProcessHeap(), 0, 0x0d);
    cmdline[0] = 0x0d;
    *(LPDWORD)(cmdline + 1) = (DWORD)spProc;
    *(LPDWORD)(cmdline + 5) = dwPmt;
    *(LPDWORD)(cmdline + 9) = 0;

    sel1 = SELECTOR_AllocBlock(cmdline, 0x0d, SEGMENT_DATA, FALSE, FALSE);
    sel2 = SELECTOR_AllocBlock(&showCmd, sizeof(showCmd),
			       SEGMENT_DATA, FALSE, FALSE);
    
    lp = (LOADPARAMS16*)HeapAlloc(GetProcessHeap(), 0, sizeof(LOADPARAMS16));
    lp->hEnvironment = 0;
    lp->cmdLine = PTR_SEG_OFF_TO_SEGPTR(sel1, 0);
    lp->showCmd = PTR_SEG_OFF_TO_SEGPTR(sel2, 0);
    lp->reserved = 0;
    
#ifndef USE_MM_TSK_WINE
    handle = LoadModule16("c:\\windows\\system\\mmtask.tsk", lp);
#else
    handle = LoadModule16("mmtask.tsk", lp);
#endif
    if (handle < 32) {
	ret = (handle) ? 1 : 2;
	handle = 0;
    } else {
	ret = 0;
    }
    if (lphMmTask)
	*lphMmTask = handle;
    
    UnMapLS(PTR_SEG_OFF_TO_SEGPTR(sel2, 0));
    UnMapLS(PTR_SEG_OFF_TO_SEGPTR(sel1, 0));
    
    HeapFree(GetProcessHeap(), 0, lp);
    HeapFree(GetProcessHeap(), 0, cmdline);

    TRACE("=> 0x%04x/%d\n", handle, ret);
    return ret;
}

#ifdef USE_MM_TSK_WINE
/* C equivalent to mmtask.tsk binary content */
void	mmTaskEntryPoint16(LPSTR cmdLine, WORD di, WORD si)
{
    int	len = cmdLine[0x80];

    if (len / 2 == 6) {
	void	(*fpProc)(DWORD) = (void (*)(DWORD))PTR_SEG_TO_LIN(*((DWORD*)(cmdLine + 1)));
	DWORD	dwPmt  = *((DWORD*)(cmdLine + 5));

#if 0
	InitTask16(); /* fixme: pmts / from context ? */
	InitApp(di);
#endif
	if (SetMessageQueue16(0x40)) {
	    WaitEvent16(0);
	    if (HIWORD(fpProc)) {
		OldYield16();
/* EPP 		StackEnter16(); */
		(fpProc)(dwPmt);
	    }
	}
    }
    OldYield16();
    OldYield16();
    OldYield16();
    ExitProcess(0);
}
#endif

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
LRESULT	WINAPI mmTaskSignal16(HTASK16 ht) 
{
    TRACE("(%04x);\n", ht);
    return Callout.PostAppMessage16(ht, WM_USER, 0, 0);
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

DWORD	WINAPI	GetProcessFlags(DWORD);

/**************************************************************************
 * 				mmThreadCreate		[MMSYSTEM.1120]
 *
 * undocumented
 * Creates a MM thread, calling fpThreadAddr(dwPmt). 
 * dwFlags: 
 * 	bit.0 set means create a 16 bit task instead of thread calling a 16 bit proc
 *	bit.1 set means to open a VxD for this thread (unsupported) 
 */
LRESULT	WINAPI mmThreadCreate16(FARPROC16 fpThreadAddr, LPHANDLE lpHndl, DWORD dwPmt, DWORD dwFlags) 
{
    HANDLE16		hndl;
    LRESULT		ret;

    TRACE("(%p, %p, %08lx, %08lx)!\n", fpThreadAddr, lpHndl, dwPmt, dwFlags);

    hndl = GlobalAlloc16(sizeof(WINE_MMTHREAD), GMEM_SHARE|GMEM_ZEROINIT);

    if (hndl == 0) {
	ret = 2;
    } else {
	WINE_MMTHREAD*	lpMMThd = (WINE_MMTHREAD*)PTR_SEG_OFF_TO_LIN(hndl, 0);

#if 0
	/* force mmtask routines even if mmthread is required */
	/* this will work only if the patch about binary cmd line and NE tasks 
	 * is committed
	 */
	dwFlags |= 1;
#endif

	lpMMThd->dwSignature 	= WINE_MMTHREAD_CREATED;
	lpMMThd->dwCounter   	= 0;
	lpMMThd->hThread     	= 0;
	lpMMThd->dwThreadID  	= 0;
	lpMMThd->fpThread    	= fpThreadAddr;
	lpMMThd->dwThreadPmt 	= dwPmt;
	lpMMThd->dwSignalCount	= 0;
	lpMMThd->hEvent      	= 0;
	lpMMThd->hVxD        	= 0;
	lpMMThd->dwStatus    	= 0;
	lpMMThd->dwFlags     	= dwFlags;
	lpMMThd->hTask       	= 0;
	
	if ((dwFlags & 1) == 0 && (GetProcessFlags(GetCurrentThreadId()) & 8) == 0) {
	    lpMMThd->hEvent = CreateEventA(0, 0, 1, 0);

	    TRACE("Let's go crazy... trying new MM thread. lpMMThd=%p\n", lpMMThd);
	    if (lpMMThd->dwFlags & 2) {
		/* as long as we don't support MM VxD in wine, we don't need 
		 * to care about this flag
		 */
		/* FIXME("Don't know how to properly open VxD handles\n"); */
		/* lpMMThd->hVxD = OpenVxDHandle(lpMMThd->hEvent); */
	    }

	    lpMMThd->hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)WINE_mmThreadEntryPoint, 
					    (LPVOID)(DWORD)hndl, CREATE_SUSPENDED, &lpMMThd->dwThreadID);
	    if (lpMMThd->hThread == 0) {
		WARN("Couldn't create thread\n");
		/* clean-up(VxDhandle...); devicedirectio... */
		if (lpMMThd->hEvent != 0)
		    CloseHandle(lpMMThd->hEvent);
		ret = 2;
	    } else {
		TRACE("Got a nice thread hndl=0x%04x id=0x%08lx\n", lpMMThd->hThread, lpMMThd->dwThreadID);
		ret = 0;
	    }
	} else {
	    /* get WINE_mmThreadEntryPoint() 
	     * 2047 is its ordinal in mmsystem.spec
	     */
	    FARPROC16	fp = GetProcAddress16(GetModuleHandle16("MMSYSTEM"), (SEGPTR)2047);

	    TRACE("farproc seg=0x%08lx lin=%p\n", (DWORD)fp, PTR_SEG_TO_LIN(fp));

	    ret = (fp == 0) ? 2 : mmTaskCreate16((DWORD)fp, 0, hndl);
	}

	if (ret == 0) {
	    if (lpMMThd->hThread && !ResumeThread(lpMMThd->hThread))
		WARN("Couldn't resume thread\n");

	    while (lpMMThd->dwStatus != 0x10) { /* test also HIWORD of dwStatus */
		UserYield16();
	    }
	}
    }

    if (ret != 0) {
	GlobalFree16(hndl);
	hndl = 0;
    }

    if (lpHndl)
	*lpHndl = hndl;

    TRACE("ok => %ld\n", ret);
    return ret;
}

/**************************************************************************
 * 				mmThreadSignal		[MMSYSTEM.1121]
 */
void WINAPI mmThreadSignal16(HANDLE16 hndl) 
{
    TRACE("(%04x)!\n", hndl);

    if (hndl) {
	WINE_MMTHREAD*	lpMMThd = (WINE_MMTHREAD*)PTR_SEG_OFF_TO_LIN(hndl, 0);

	lpMMThd->dwCounter++;
	if (lpMMThd->hThread != 0) {
	    InterlockedIncrement(&lpMMThd->dwSignalCount);
	    SetEvent(lpMMThd->hEvent);
	} else {
	    mmTaskSignal16(lpMMThd->hTask);
	}
	lpMMThd->dwCounter--;
    }
}

/**************************************************************************
 * 				MMSYSTEM_ThreadBlock		[internal]
 */
static	void	MMSYSTEM_ThreadBlock(WINE_MMTHREAD* lpMMThd)
{
    MSG		msg;
    DWORD	ret;

    if (lpMMThd->dwThreadID != GetCurrentThreadId())
	ERR("Not called by thread itself\n");

    for (;;) {
	ResetEvent(lpMMThd->hEvent);
	if (InterlockedDecrement(&lpMMThd->dwSignalCount) >= 0)
	    break;
	InterlockedIncrement(&lpMMThd->dwSignalCount);
	
	TRACE("S1\n");
	
	ret = MsgWaitForMultipleObjects(1, &lpMMThd->hEvent, FALSE, INFINITE, QS_ALLINPUT);
	switch (ret) {
	case WAIT_OBJECT_0:	/* Event */
	    TRACE("S2.1\n");
	    break;
	case WAIT_OBJECT_0 + 1:	/* Msg */
	    TRACE("S2.2\n");
	    if (Callout.PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
		Callout.TranslateMessage(&msg);
		Callout.DispatchMessageA(&msg);
	    }
	    break;
	default:
	    WARN("S2.x unsupported ret val 0x%08lx\n", ret);
	}
	TRACE("S3\n");
    }
}

/**************************************************************************
 * 				mmThreadBlock		[MMSYSTEM.1122]
 */
void	WINAPI mmThreadBlock16(HANDLE16 hndl) 
{
    TRACE("(%04x)!\n", hndl);

    if (hndl) {
	WINE_MMTHREAD*	lpMMThd = (WINE_MMTHREAD*)PTR_SEG_OFF_TO_LIN(hndl, 0);
	
	if (lpMMThd->hThread != 0) {
	    SYSLEVEL_ReleaseWin16Lock();
	    MMSYSTEM_ThreadBlock(lpMMThd);
	    SYSLEVEL_RestoreWin16Lock();
	} else {
	    mmTaskBlock16(lpMMThd->hTask);
	}
    }
    TRACE("done\n");
}

/**************************************************************************
 * 				mmThreadIsCurrent	[MMSYSTEM.1123]
 */
BOOL16	WINAPI mmThreadIsCurrent16(HANDLE16 hndl) 
{
    BOOL16		ret = FALSE;

    TRACE("(%04x)!\n", hndl);

    if (hndl && mmThreadIsValid16(hndl)) {
	WINE_MMTHREAD*	lpMMThd = (WINE_MMTHREAD*)PTR_SEG_OFF_TO_LIN(hndl, 0);
	ret = (GetCurrentThreadId() == lpMMThd->dwThreadID);
    }
    TRACE("=> %d\n", ret);
    return ret;
}

/**************************************************************************
 * 				mmThreadIsValid		[MMSYSTEM.1124]
 */
BOOL16	WINAPI	mmThreadIsValid16(HANDLE16 hndl)
{
    BOOL16		ret = FALSE;

    TRACE("(%04x)!\n", hndl);

    if (hndl) {
	WINE_MMTHREAD*	lpMMThd = (WINE_MMTHREAD*)PTR_SEG_OFF_TO_LIN(hndl, 0);

	if (!IsBadWritePtr(lpMMThd, sizeof(WINE_MMTHREAD)) &&
	    lpMMThd->dwSignature == WINE_MMTHREAD_CREATED &&
	    IsTask16(lpMMThd->hTask)) {
	    lpMMThd->dwCounter++;
	    if (lpMMThd->hThread != 0) {
		DWORD	dwThreadRet;
		if (GetExitCodeThread(lpMMThd->hThread, &dwThreadRet) &&
		    dwThreadRet == STATUS_PENDING) {
		    ret = TRUE;
		}
	    } else {
		ret = TRUE;
	    }
	    lpMMThd->dwCounter--;
	}
    }
    TRACE("=> %d\n", ret);
    return ret;
}

/**************************************************************************
 * 				mmThreadGetTask		[MMSYSTEM.1125]
 */
HANDLE16 WINAPI mmThreadGetTask16(HANDLE16 hndl) 
{
    HANDLE16	ret = 0;

    TRACE("(%04x)\n", hndl);

    if (mmThreadIsValid16(hndl)) {
	WINE_MMTHREAD*	lpMMThd = (WINE_MMTHREAD*)PTR_SEG_OFF_TO_LIN(hndl, 0);
	ret = lpMMThd->hTask;
    }
    return ret;
}

/* ### start build ### */
extern LONG CALLBACK MMSYSTEM_CallTo16_long_l    (FARPROC16,LONG);
/* ### stop build ### */

/**************************************************************************
 * 				WINE_mmThreadEntryPoint		[internal]
 */
void WINAPI WINE_mmThreadEntryPoint(DWORD _pmt)
{
    HANDLE16		hndl = (HANDLE16)_pmt;
    WINE_MMTHREAD*	lpMMThd = (WINE_MMTHREAD*)PTR_SEG_OFF_TO_LIN(hndl, 0);

    TRACE("(%04x %p)\n", hndl, lpMMThd);

    lpMMThd->hTask = LOWORD(GetCurrentTask());
    TRACE("[10-%08x] setting hTask to 0x%08x\n", lpMMThd->hThread, lpMMThd->hTask);
    lpMMThd->dwStatus = 0x10;
    MMSYSTEM_ThreadBlock(lpMMThd);
    TRACE("[20-%08x]\n", lpMMThd->hThread);
    lpMMThd->dwStatus = 0x20;
    if (lpMMThd->fpThread) {
	MMSYSTEM_CallTo16_long_l(lpMMThd->fpThread, lpMMThd->dwThreadPmt);
    }
    lpMMThd->dwStatus = 0x30;
    TRACE("[30-%08x]\n", lpMMThd->hThread);
    while (lpMMThd->dwCounter) {
	Sleep(1);
	/* Yield16();*/
    }
    TRACE("[XX-%08x]\n", lpMMThd->hThread);
    /* paranoia */
    lpMMThd->dwSignature = WINE_MMTHREAD_DELETED;
    /* close lpMMThread->hVxD directio */
    if (lpMMThd->hEvent)
	CloseHandle(lpMMThd->hEvent);
    GlobalFree16(hndl);
    TRACE("done\n");
}

typedef	BOOL16 (WINAPI *MMCPLCALLBACK)(HWND, LPCSTR, LPCSTR, LPCSTR);

/**************************************************************************
 * 			mmShowMMCPLPropertySheet	[MMSYSTEM.1150]
 */
BOOL16	WINAPI	mmShowMMCPLPropertySheet16(HWND hWnd, LPCSTR lpStrDevice, 
					   LPCSTR lpStrTab, LPCSTR lpStrTitle)
{
    HANDLE	hndl;
    BOOL16	ret = FALSE;

    TRACE("(%04x \"%s\" \"%s\" \"%s\")\n", hWnd, lpStrDevice, lpStrTab, lpStrTitle);

    hndl = LoadLibraryA("MMSYS.CPL");
    if (hndl != 0) {
	MMCPLCALLBACK	fp = (MMCPLCALLBACK)GetProcAddress(hndl, "ShowMMCPLPropertySheet");
	if (fp != NULL) {
	    SYSLEVEL_ReleaseWin16Lock();
	    ret = (fp)(hWnd, lpStrDevice, lpStrTab, lpStrTitle);
	    SYSLEVEL_RestoreWin16Lock();
	}
	FreeLibrary(hndl);
    }
    
    return ret;
}

/**************************************************************************
 * 			StackEnter		[MMSYSTEM.32]
 */
void WINAPI StackEnter16(void)
{
#ifdef __i386__
    /* mmsystem.dll from Win 95 does only this: so does Wine */
    __asm__("stc");
#endif
}

/**************************************************************************
 * 			StackLeave		[MMSYSTEM.33]
 */
void WINAPI StackLeave16(void)
{
#ifdef __i386__
    /* mmsystem.dll from Win 95 does only this: so does Wine */
    __asm__("stc");
#endif
}

/**************************************************************************
 * 			WMMMidiRunOnce	 	[MMSYSTEM.8]
 */
void WINAPI WMMMidiRunOnce16(void)
{
	FIXME("(), stub!\n");
}
