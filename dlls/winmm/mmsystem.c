/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 * MMSYTEM functions
 *
 * Copyright 1993 Martin Ayotte
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

#include "mmsystem.h"
#include "winbase.h"
#include "wingdi.h"

#include "wine/mmsystem16.h"
#include "wine/winuser16.h"
#include "heap.h"
#include "winternl.h"
#include "winemm.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmsys);

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
	ERR("IData not found for pid=%08lx. Suicide !!!\n", GetCurrentProcessId());
	DbgBreakPoint();
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
    iData->cs.DebugInfo = (void*)__FILE__ ": WinMM";
    iData->psStopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    iData->psLastEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
    TRACE("Created IData (%p) for pid %08lx\n", iData, iData->dwThisProcess);
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
	TIME_MMTimeStop();

	for (ppid = &lpFirstIData; *ppid; ppid = &(*ppid)->lpNextIData) {
	    if (*ppid == iData) {
		*ppid = iData->lpNextIData;
		break;
	    }
	}
	/* FIXME: should also free content and resources allocated
	 * inside iData */
        CloseHandle(iData->psStopEvent);
        CloseHandle(iData->psLastEvent);
        DeleteCriticalSection(&iData->cs);
	HeapFree(GetProcessHeap(), 0, iData);
    }
}

/**************************************************************************
 *		DllEntryPoint (WINMM.init)
 *
 * WINMM DLL entry point
 *
 */
BOOL WINAPI WINMM_LibMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("0x%x 0x%lx %p\n", hInstDLL, fdwReason, fImpLoad);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
	if (!MULTIMEDIA_CreateIData(hInstDLL))
	    return FALSE;
        if (!MULTIMEDIA_MciInit() || !MMDRV_Init()) {
            MULTIMEDIA_DeleteIData();
            return FALSE;
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
 * 			DllEntryPoint (MMSYSTEM.2046)
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
	 * - initiate correctly shared variables (MULTIMEDIA_Init())
	 * - create correctly the per process WINE_MM_IDATA chunk
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

static HMMIO	get_mmioFromFile(LPCWSTR lpszName)
{
    HMMIO       ret;
    WCHAR       buf[256];
    LPWSTR      dummy;

    ret = mmioOpenW((LPWSTR)lpszName, NULL,
                    MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    if (ret != 0) return ret;
    if (SearchPathW(NULL, lpszName, NULL, sizeof(buf)/sizeof(buf[0]), buf, &dummy))
    {
        return mmioOpenW(buf, NULL,
                         MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    }
    return 0;
}

static HMMIO	get_mmioFromProfile(UINT uFlags, LPCWSTR lpszName)
{
    WCHAR	str[128];
    LPWSTR	ptr;
    HMMIO  	hmmio;
    HKEY        hRegSnd, hRegApp, hScheme, hSnd;
    DWORD       err, type, count;

    static  WCHAR       wszSounds[] = {'S','o','u','n','d','s',0};
    static  WCHAR       wszDefault[] = {'D','e','f','a','u','l','t',0};
    static  WCHAR       wszKey[] = {'A','p','p','E','v','e','n','t','s','\\',
                                    'S','c','h','e','m','e','s','\\',
                                    'A','p','p','s',0};
    static  WCHAR       wszDotDefault[] = {'.','D','e','f','a','u','l','t',0};
    static  WCHAR       wszNull[] = {0};

    TRACE("searching in SystemSound list for %s\n", debugstr_w(lpszName));
    GetProfileStringW(wszSounds, (LPWSTR)lpszName, wszNull, str, sizeof(str)/sizeof(str[0]));
    if (lstrlenW(str) == 0)
    {
	if (uFlags & SND_NODEFAULT) goto next;
	GetProfileStringW(wszSounds, wszDefault, wszNull, str, sizeof(str)/sizeof(str[0]));
	if (lstrlenW(str) == 0) goto next;
    }
    for (ptr = str; *ptr && *ptr != ','; ptr++);
    if (*ptr) *ptr = 0;
    hmmio = mmioOpenW(str, NULL, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    if (hmmio != 0) return hmmio;
 next:
    /* we look up the registry under
     *      HKCU\AppEvents\Schemes\Apps\.Default
     *      HKCU\AppEvents\Schemes\Apps\<AppName>
     */
    if (RegOpenKeyW(HKEY_CURRENT_USER, wszKey, &hRegSnd) != 0) goto none;
    if (uFlags & SND_APPLICATION)
    {
        err = 1; /* error */
        if (GetModuleFileNameW(0, str, sizeof(str)/sizeof(str[0])))
        {
            for (ptr = str + lstrlenW(str) - 1; ptr >= str; ptr--)
            {
                if (*ptr == '.') *ptr = 0;
                if (*ptr == '\\')
                {
                    err = RegOpenKeyW(hRegSnd, str, &hRegApp);
                    break;
                }
            }
        }
    }
    else
    {
        err = RegOpenKeyW(hRegSnd, wszDotDefault, &hRegApp);
    }
    RegCloseKey(hRegSnd);
    if (err != 0) goto none;
    err = RegOpenKeyW(hRegApp, lpszName, &hScheme);
    RegCloseKey(hRegApp);
    if (err != 0) goto none;
    err = RegOpenKeyW(hScheme, wszDotDefault, &hSnd);
    RegCloseKey(hScheme);
    if (err != 0) goto none;
    count = sizeof(str)/sizeof(str[0]);
    err = RegQueryValueExW(hSnd, NULL, 0, &type, (LPBYTE)str, &count);
    RegCloseKey(hSnd);
    if (err != 0 || !*str) goto none;
    hmmio = mmioOpenW(str, NULL, MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
    if (hmmio) return hmmio;
 none:
    WARN("can't find SystemSound='%s' !\n", debugstr_w(lpszName));
    return 0;
}

struct playsound_data
{
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
	if (InterlockedDecrement(&s->dwEventCount) >= 0) break;
	InterlockedIncrement(&s->dwEventCount);

	WaitForSingleObject(s->hEvent, INFINITE);
    }
}

static BOOL PlaySound_IsString(DWORD fdwSound, const void* psz)
{
    /* SND_RESOURCE is 0x40004 while
     * SND_MEMORY is 0x00004
     */
    switch (fdwSound & (SND_RESOURCE|SND_ALIAS|SND_FILENAME))
    {
    case SND_RESOURCE:  return HIWORD(psz) != 0; /* by name or by ID ? */
    case SND_MEMORY:    return FALSE;
    case SND_ALIAS:     /* what about ALIAS_ID ??? */
    case SND_FILENAME:
    case 0:             return TRUE;
    default:            FIXME("WTF\n"); return FALSE;
    }
}

static void     PlaySound_Free(WINE_PLAYSOUND* wps)
{
    LPWINE_MM_IDATA     iData = MULTIMEDIA_GetIData();
    WINE_PLAYSOUND**    p;

    EnterCriticalSection(&iData->cs);
    for (p = &iData->lpPlaySound; *p && *p != wps; p = &((*p)->lpNext));
    if (*p) *p = (*p)->lpNext;
    if (iData->lpPlaySound == NULL) SetEvent(iData->psLastEvent);
    LeaveCriticalSection(&iData->cs);
    if (wps->bAlloc) HeapFree(GetProcessHeap(), 0, (void*)wps->pszSound);
    HeapFree(GetProcessHeap(), 0, wps);
}

static WINE_PLAYSOUND*  PlaySound_Alloc(const void* pszSound, HMODULE hmod,
                                        DWORD fdwSound, BOOL bUnicode)
{
    WINE_PLAYSOUND* wps;

    wps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wps));
    if (!wps) return NULL;

    wps->hMod = hmod;
    wps->fdwSound = fdwSound;
    if (PlaySound_IsString(fdwSound, pszSound))
    {
        if (bUnicode)
        {
            if (fdwSound & SND_ASYNC)
            {
                wps->pszSound = HeapAlloc(GetProcessHeap(), 0,
                                          (lstrlenW(pszSound)+1) * sizeof(WCHAR));
                if (!wps->pszSound) goto oom_error;
                lstrcpyW((LPWSTR)wps->pszSound, pszSound);
                wps->bAlloc = TRUE;
            }
            else
                wps->pszSound = pszSound;
        }
        else
        {
            wps->pszSound = HEAP_strdupAtoW(GetProcessHeap(), 0, pszSound);
            if (!wps->pszSound) goto oom_error;
            wps->bAlloc = TRUE;
        }
    }
    else
        wps->pszSound = pszSound;

    return wps;
 oom_error:
    PlaySound_Free(wps);
    return NULL;
}

static DWORD WINAPI proc_PlaySound(LPVOID arg)
{
    WINE_PLAYSOUND*     wps = (WINE_PLAYSOUND*)arg;
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIData();
    BOOL		bRet = FALSE;
    HMMIO		hmmio = 0;
    MMCKINFO		ckMainRIFF;
    MMCKINFO        	mmckInfo;
    LPWAVEFORMATEX      lpWaveFormat = NULL;
    HWAVEOUT		hWave = 0;
    LPWAVEHDR		waveHdr = NULL;
    INT			count, bufsize, left, index;
    struct playsound_data	s;
    void*               data;

    s.hEvent = 0;

    TRACE("SoundName='%s' !\n", debugstr_w(wps->pszSound));

    /* if resource, grab it */
    if ((wps->fdwSound & SND_RESOURCE) == SND_RESOURCE) {
        static WCHAR wszWave[] = {'W','A','V','E',0};
        HRSRC	hRes;
        HGLOBAL	hGlob;

        if ((hRes = FindResourceW(wps->hMod, wps->pszSound, wszWave)) == 0 ||
            (hGlob = LoadResource(wps->hMod, hRes)) == 0)
            goto errCleanUp;
        if ((data = LockResource(hGlob)) == NULL) {
            FreeResource(hGlob);
            goto errCleanUp;
        }
        FreeResource(hGlob);
    } else
        data = (void*)wps->pszSound;

    /* construct an MMIO stream (either in memory, or from a file */
    if (wps->fdwSound & SND_MEMORY)
    { /* NOTE: SND_RESOURCE has the SND_MEMORY bit set */
	MMIOINFO	mminfo;

	memset(&mminfo, 0, sizeof(mminfo));
	mminfo.fccIOProc = FOURCC_MEM;
	mminfo.pchBuffer = (LPSTR)data;
	mminfo.cchBuffer = -1; /* FIXME: when a resource, could grab real size */
	TRACE("Memory sound %p\n", data);
	hmmio = mmioOpenW(NULL, &mminfo, MMIO_READ);
    }
    else if (wps->fdwSound & SND_ALIAS)
    {
        hmmio = get_mmioFromProfile(wps->fdwSound, wps->pszSound);
    }
    else if (wps->fdwSound & SND_FILENAME)
    {
        hmmio = get_mmioFromFile(wps->pszSound);
    }
    else
    {
        if ((hmmio = get_mmioFromProfile(wps->fdwSound | SND_NODEFAULT, wps->pszSound)) == 0)
        {
            if ((hmmio = get_mmioFromFile(wps->pszSound)) == 0)
            {
                hmmio = get_mmioFromProfile(wps->fdwSound, wps->pszSound);
            }
        }
    }
    if (hmmio == 0) goto errCleanUp;

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

    s.dwEventCount = 1L; /* for first buffer */

    do {
	index = 0;
	left = mmckInfo.cksize;

	mmioSeek(hmmio, mmckInfo.dwDataOffset, SEEK_SET);
	while (left)
        {
	    if (WaitForSingleObject(iData->psStopEvent, 0) == WAIT_OBJECT_0)
            {
		wps->bLoop = FALSE;
		break;
	    }
	    count = mmioRead(hmmio, waveHdr[index].lpData, min(bufsize, left));
	    if (count < 1) break;
	    left -= count;
	    waveHdr[index].dwBufferLength = count;
	    waveHdr[index].dwFlags &= ~WHDR_DONE;
	    if (waveOutWrite(hWave, &waveHdr[index], sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                index ^= 1;
                PlaySound_WaitDone(&s);
            }
            else FIXME("Couldn't play header\n");
	}
	bRet = TRUE;
    } while (wps->bLoop);

    PlaySound_WaitDone(&s); /* for last buffer */
    waveOutReset(hWave);

    waveOutUnprepareHeader(hWave, &waveHdr[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(hWave, &waveHdr[1], sizeof(WAVEHDR));

errCleanUp:
    TRACE("Done playing='%s' => %s!\n", debugstr_w(wps->pszSound), bRet ? "ok" : "ko");
    CloseHandle(s.hEvent);
    if (waveHdr)        HeapFree(GetProcessHeap(), 0, waveHdr);
    if (lpWaveFormat)   HeapFree(GetProcessHeap(), 0, lpWaveFormat);
    if (hWave)		while (waveOutClose(hWave) == WAVERR_STILLPLAYING) Sleep(100);
    if (hmmio) 		mmioClose(hmmio, 0);

    PlaySound_Free(wps);

    return bRet;
}

static BOOL MULTIMEDIA_PlaySound(const void* pszSound, HMODULE hmod, DWORD fdwSound, BOOL bUnicode)
{
    WINE_PLAYSOUND*     wps = NULL;
    LPWINE_MM_IDATA	iData = MULTIMEDIA_GetIData();

    TRACE("pszSound='%p' hmod=%04X fdwSound=%08lX\n",
	  pszSound, hmod, fdwSound);

    /* FIXME? I see no difference between SND_NOWAIT and SND_NOSTOP !
     * there could be one if several sounds can be played at once...
     */
    if ((fdwSound & (SND_NOWAIT | SND_NOSTOP)) && iData->lpPlaySound != NULL)
	return FALSE;

    /* alloc internal structure, if we need to play something */
    if (pszSound && !(fdwSound & SND_PURGE))
    {
        if (!(wps = PlaySound_Alloc(pszSound, hmod, fdwSound, bUnicode)))
            return FALSE;
    }

    EnterCriticalSection(&iData->cs);
    /* since several threads can enter PlaySound in parallel, we're not
     * sure, at this point, that another thread didn't start a new playsound
     */
    while (iData->lpPlaySound != NULL)
    {
        ResetEvent(iData->psLastEvent);
        /* FIXME: doc says we have to stop all instances of pszSound if it's non
         * NULL... as of today, we stop all playing instances */
        SetEvent(iData->psStopEvent);

        LeaveCriticalSection(&iData->cs);
        WaitForSingleObject(iData->psLastEvent, INFINITE);
        EnterCriticalSection(&iData->cs);

        ResetEvent(iData->psStopEvent);
    }

    if (wps) wps->lpNext = iData->lpPlaySound;
    iData->lpPlaySound = wps;
    LeaveCriticalSection(&iData->cs);

    if (!pszSound || (fdwSound & SND_PURGE)) return TRUE;

    if (fdwSound & SND_ASYNC)
    {
        DWORD       id;
        wps->bLoop = (fdwSound & SND_LOOP) ? TRUE : FALSE;
        if (CreateThread(NULL, 0, proc_PlaySound, wps, 0, &id) != 0)
            return TRUE;
    }
    else return proc_PlaySound(wps);

    /* error cases */
    PlaySound_Free(wps);
    return FALSE;
}

/**************************************************************************
 * 				PlaySoundA		[WINMM.@]
 */
BOOL WINAPI PlaySoundA(LPCSTR pszSoundA, HMODULE hmod, DWORD fdwSound)
{
    return MULTIMEDIA_PlaySound(pszSoundA, hmod, fdwSound, FALSE);
}

/**************************************************************************
 * 				PlaySoundW		[WINMM.@]
 */
BOOL WINAPI PlaySoundW(LPCWSTR pszSoundW, HMODULE hmod, DWORD fdwSound)
{
    return MULTIMEDIA_PlaySound(pszSoundW, hmod, fdwSound, TRUE);
}

/**************************************************************************
 * 				PlaySound		[MMSYSTEM.3]
 */
BOOL16 WINAPI PlaySound16(LPCSTR pszSound, HMODULE16 hmod, DWORD fdwSound)
{
    BOOL16	retv;
    DWORD	lc;

    ReleaseThunkLock(&lc);
    retv = PlaySoundA(pszSound, hmod, fdwSound);
    RestoreThunkLock(lc);

    return retv;
}

/**************************************************************************
 * 				sndPlaySoundA		[WINMM.@]
 */
BOOL WINAPI sndPlaySoundA(LPCSTR pszSoundA, UINT uFlags)
{
    uFlags &= SND_ASYNC|SND_LOOP|SND_MEMORY|SND_NODEFAULT|SND_NOSTOP|SND_SYNC;
    return MULTIMEDIA_PlaySound(pszSoundA, 0, uFlags, FALSE);
}

/**************************************************************************
 * 				sndPlaySoundW		[WINMM.@]
 */
BOOL WINAPI sndPlaySoundW(LPCWSTR pszSound, UINT uFlags)
{
    uFlags &= SND_ASYNC|SND_LOOP|SND_MEMORY|SND_NODEFAULT|SND_NOSTOP|SND_SYNC;
    return MULTIMEDIA_PlaySound(pszSound, 0, uFlags, TRUE);
}

/**************************************************************************
 * 				sndPlaySound		[MMSYSTEM.2]
 */
BOOL16 WINAPI sndPlaySound16(LPCSTR lpszSoundName, UINT16 uFlags)
{
    BOOL16	retv;
    DWORD	lc;

    ReleaseThunkLock(&lc);
    retv = sndPlaySoundA(lpszSoundName, uFlags);
    RestoreThunkLock(lc);

    return retv;
}

/**************************************************************************
 * 				mmsystemGetVersion	[MMSYSTEM.5]
 * return value borrowed from Win95 winmm.dll ;)
 */
UINT16 WINAPI mmsystemGetVersion16(void)
{
    return mmsystemGetVersion();
}

/**************************************************************************
 * 				mmsystemGetVersion	[WINMM.@]
 */
UINT WINAPI mmsystemGetVersion(void)
{
    TRACE("3.10 (Win95?)\n");
    return 0x030a;
}

/**************************************************************************
 * 				DriverCallback			[WINMM.@]
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
	PostMessageA((HWND)dwCallBack, wMsg, (WPARAM)hDev, dwParam1);
	break;
    case DCB_TASK: /* aka DCB_THREAD */
	TRACE("Task(%04lx) !\n", dwCallBack);
	PostThreadMessageA(dwCallBack, wMsg, (WPARAM)hDev, dwParam1);
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
	    WINE_MMTHREAD*	lpMMThd = MapSL( MAKESEGPTR(LOWORD(dwCallBack), 0) );

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
    return DriverCallback(dwCallBack, uFlags, HDRVR_32(hDev), wMsg, dwUser, dwParam1, dwParam2);
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
 * 				mixerGetNumDevs			[WINMM.@]
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
 * 				mixerGetDevCapsA		[WINMM.@]
 */
UINT WINAPI mixerGetDevCapsA(UINT devid, LPMIXERCAPSA mixcaps, UINT size)
{
    LPWINE_MLD	wmld;

    if ((wmld = MMDRV_Get(devid, MMDRV_MIXER, TRUE)) == NULL)
	return MMSYSERR_BADDEVICEID;

    return MMDRV_Message(wmld, MXDM_GETDEVCAPS, (DWORD)mixcaps, size, TRUE);
}

/**************************************************************************
 * 				mixerGetDevCapsW		[WINMM.@]
 */
UINT WINAPI mixerGetDevCapsW(UINT devid, LPMIXERCAPSW mixcaps, UINT size)
{
    MIXERCAPSA	micA;
    UINT	ret = mixerGetDevCapsA(devid, &micA, sizeof(micA));

    if (ret == MMSYSERR_NOERROR) {
	mixcaps->wMid           = micA.wMid;
	mixcaps->wPid           = micA.wPid;
	mixcaps->vDriverVersion = micA.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, micA.szPname, -1, mixcaps->szPname,
                             sizeof(mixcaps->szPname)/sizeof(WCHAR) );
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
    HMIXER		hMix;
    LPWINE_MLD		wmld;
    DWORD		dwRet = 0;
    MIXEROPENDESC	mod;

    TRACE("(%p, %d, %08lx, %08lx, %08lx)\n",
	  lphMix, uDeviceID, dwCallback, dwInstance, fdwOpen);

    wmld = MMDRV_Alloc(sizeof(WINE_MIXER), MMDRV_MIXER, &hMix, &fdwOpen,
		       &dwCallback, &dwInstance, bFrom32);

    wmld->uDeviceID = uDeviceID;
    mod.hmx = (HMIXEROBJ)hMix;
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
 * 				mixerOpen			[WINMM.@]
 */
UINT WINAPI mixerOpen(LPHMIXER lphMix, UINT uDeviceID, DWORD dwCallback,
		      DWORD dwInstance, DWORD fdwOpen)
{
    return MMSYSTEM_mixerOpen(lphMix, uDeviceID,
			      dwCallback, dwInstance, fdwOpen, TRUE);
}

/**************************************************************************
 * 				mixerOpen			[MMSYSTEM.802]
 */
UINT16 WINAPI mixerOpen16(LPHMIXER16 lphmix, UINT16 uDeviceID, DWORD dwCallback,
			  DWORD dwInstance, DWORD fdwOpen)
{
    HMIXER	hmix;
    UINT	ret;

    ret = MMSYSTEM_mixerOpen(&hmix, uDeviceID,
			     dwCallback, dwInstance, fdwOpen, FALSE);
    if (lphmix) *lphmix = HMIXER_16(hmix);
    return ret;
}

/**************************************************************************
 * 				mixerClose			[WINMM.@]
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
    return mixerClose(HMIXER_32(hMix));
}

/**************************************************************************
 * 				mixerGetID			[WINMM.@]
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
 * 				mixerGetID (MMSYSTEM.806)
 */
UINT16 WINAPI mixerGetID16(HMIXEROBJ16 hmix, LPUINT16 lpid, DWORD fdwID)
{
    UINT	xid;
    UINT	ret = mixerGetID(HMIXEROBJ_32(hmix), &xid, fdwID);

    if (lpid)
	*lpid = xid;
    return ret;
}

/**************************************************************************
 * 				mixerGetControlDetailsA		[WINMM.@]
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
 * 				mixerGetControlDetailsW	[WINMM.@]
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
	    MIXERCONTROLDETAILS_LISTTEXTW *pDetailsW = (MIXERCONTROLDETAILS_LISTTEXTW *)lpmcd->paDetails;
            MIXERCONTROLDETAILS_LISTTEXTA *pDetailsA;
	    int size = max(1, lpmcd->cChannels) * sizeof(MIXERCONTROLDETAILS_LISTTEXTA);
            int i;

	    if (lpmcd->u.cMultipleItems != 0) {
		size *= lpmcd->u.cMultipleItems;
	    }
	    pDetailsA = (MIXERCONTROLDETAILS_LISTTEXTA *)HeapAlloc(GetProcessHeap(), 0, size);
            lpmcd->paDetails = pDetailsA;
            lpmcd->cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTA);
	    /* set up lpmcd->paDetails */
	    ret = mixerGetControlDetailsA(hmix, lpmcd, fdwDetails);
	    /* copy from lpmcd->paDetails back to paDetailsW; */
            if(ret == MMSYSERR_NOERROR) {
                for(i=0;i<lpmcd->u.cMultipleItems*lpmcd->cChannels;i++) {
                    pDetailsW->dwParam1 = pDetailsA->dwParam1;
                    pDetailsW->dwParam2 = pDetailsA->dwParam2;
                    MultiByteToWideChar( CP_ACP, 0, pDetailsA->szName, -1,
                                         pDetailsW->szName,
                                         sizeof(pDetailsW->szName)/sizeof(WCHAR) );
                    pDetailsA++;
                    pDetailsW++;
                }
                pDetailsA -= lpmcd->u.cMultipleItems*lpmcd->cChannels;
                pDetailsW -= lpmcd->u.cMultipleItems*lpmcd->cChannels;
            }
	    HeapFree(GetProcessHeap(), 0, pDetailsA);
	    lpmcd->paDetails = pDetailsW;
            lpmcd->cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXTW);
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
    lpmcd->paDetails = MapSL(sppaDetails);
    ret = mixerGetControlDetailsA(HMIXEROBJ_32(hmix),
			         (LPMIXERCONTROLDETAILS)lpmcd, fdwDetails);
    lpmcd->paDetails = (LPVOID)sppaDetails;

    return ret;
}

/**************************************************************************
 * 				mixerGetLineControlsA	[WINMM.@]
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
 * 				mixerGetLineControlsW		[WINMM.@]
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
            MultiByteToWideChar( CP_ACP, 0, mlcA.pamxctrl[i].szShortName, -1,
                                 lpmlcW->pamxctrl[i].szShortName,
                                 sizeof(lpmlcW->pamxctrl[i].szShortName)/sizeof(WCHAR) );
            MultiByteToWideChar( CP_ACP, 0, mlcA.pamxctrl[i].szName, -1,
                                 lpmlcW->pamxctrl[i].szName,
                                 sizeof(lpmlcW->pamxctrl[i].szName)/sizeof(WCHAR) );
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

    ret = mixerGetLineControlsA(HMIXEROBJ_32(hmix), &mlcA, fdwControls);

    if (ret == MMSYSERR_NOERROR) {
	lpmlc16->dwLineID = mlcA.dwLineID;
	lpmlc16->u.dwControlID = mlcA.u.dwControlID;
	lpmlc16->u.dwControlType = mlcA.u.dwControlType;
	lpmlc16->cControls = mlcA.cControls;

	lpmc16 = MapSL(lpmlc16->pamxctrl);

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
 * 				mixerGetLineInfoA		[WINMM.@]
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
 * 				mixerGetLineInfoW		[WINMM.@]
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
        WideCharToMultiByte( CP_ACP, 0, lpmliW->Target.szPname, -1, mliA.Target.szPname, sizeof(mliA.Target.szPname), NULL, NULL);
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
    MultiByteToWideChar( CP_ACP, 0, mliA.szShortName, -1, lpmliW->szShortName,
                         sizeof(lpmliW->szShortName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, mliA.szName, -1, lpmliW->szName,
                         sizeof(lpmliW->szName)/sizeof(WCHAR) );
    lpmliW->Target.dwType = mliA.Target.dwType;
    lpmliW->Target.dwDeviceID = mliA.Target.dwDeviceID;
    lpmliW->Target.wMid = mliA.Target.wMid;
    lpmliW->Target.wPid = mliA.Target.wPid;
    lpmliW->Target.vDriverVersion = mliA.Target.vDriverVersion;
    MultiByteToWideChar( CP_ACP, 0, mliA.Target.szPname, -1, lpmliW->Target.szPname,
                         sizeof(lpmliW->Target.szPname)/sizeof(WCHAR) );

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

    ret = mixerGetLineInfoA(HMIXEROBJ_32(hmix), &mliA, fdwInfo);

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
 * 				mixerSetControlDetails	[WINMM.@]
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
 * 				mixerMessage		[WINMM.@]
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
DWORD WINAPI mixerMessage16(HMIXER16 hmix, UINT16 uMsg, DWORD dwParam1,
			     DWORD dwParam2)
{
    return mixerMessage(HMIXER_32(hmix), uMsg, dwParam1, dwParam2);
}

/**************************************************************************
 * 				auxGetNumDevs		[WINMM.@]
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
 * 				auxGetDevCapsW		[WINMM.@]
 */
UINT WINAPI auxGetDevCapsW(UINT uDeviceID, LPAUXCAPSW lpCaps, UINT uSize)
{
    AUXCAPSA	acA;
    UINT	ret = auxGetDevCapsA(uDeviceID, &acA, sizeof(acA));

    lpCaps->wMid = acA.wMid;
    lpCaps->wPid = acA.wPid;
    lpCaps->vDriverVersion = acA.vDriverVersion;
    MultiByteToWideChar( CP_ACP, 0, acA.szPname, -1, lpCaps->szPname,
                         sizeof(lpCaps->szPname)/sizeof(WCHAR) );
    lpCaps->wTechnology = acA.wTechnology;
    lpCaps->dwSupport = acA.dwSupport;
    return ret;
}

/**************************************************************************
 * 				auxGetDevCapsA		[WINMM.@]
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
 * 				auxGetVolume		[WINMM.@]
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
 * 				auxSetVolume		[WINMM.@]
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
 * 				auxOutMessage		[WINMM.@]
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
	return auxGetVolume16(uDeviceID, MapSL(dw1));
    case AUXDM_GETDEVCAPS:
	return auxGetDevCaps16(uDeviceID, MapSL(dw1), dw2);
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
 * 				mciGetErrorStringW		[WINMM.@]
 */
BOOL WINAPI mciGetErrorStringW(DWORD wError, LPWSTR lpstrBuffer, UINT uLength)
{
    LPSTR	bufstr = HeapAlloc(GetProcessHeap(), 0, uLength);
    BOOL	ret = mciGetErrorStringA(wError, bufstr, uLength);

    MultiByteToWideChar( CP_ACP, 0, bufstr, -1, lpstrBuffer, uLength );
    HeapFree(GetProcessHeap(), 0, bufstr);
    return ret;
}

/**************************************************************************
 * 				mciGetErrorString		[MMSYSTEM.706]
 */
BOOL16 WINAPI mciGetErrorString16(DWORD wError, LPSTR lpstrBuffer, UINT16 uLength)
{
    return mciGetErrorStringA(wError, lpstrBuffer, uLength);
}

/**************************************************************************
 * 				mciGetErrorStringA		[WINMM.@]
 */
BOOL WINAPI mciGetErrorStringA(DWORD dwError, LPSTR lpstrBuffer, UINT uLength)
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

    return PostMessageA(HWND_32(hWndCallBack), MM_MCINOTIFY, wStatus, wDevID);
}

/**************************************************************************
 *			mciDriverNotify				[WINMM.@]
 */
BOOL WINAPI mciDriverNotify(HWND hWndCallBack, UINT wDevID, UINT wStatus)
{

    TRACE("(%08X, %04x, %04X)\n", hWndCallBack, wDevID, wStatus);

    return PostMessageA(hWndCallBack, MM_MCINOTIFY, wStatus, wDevID);
}

/**************************************************************************
 * 			mciGetDriverData			[MMSYSTEM.708]
 */
DWORD WINAPI mciGetDriverData16(UINT16 uDeviceID)
{
    return mciGetDriverData(uDeviceID);
}

/**************************************************************************
 * 			mciGetDriverData			[WINMM.@]
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
 * 			mciSetDriverData			[WINMM.@]
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
 * 				mciSendCommandA			[WINMM.@]
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
 * 				mciSendCommandW			[WINMM.@]
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
 * 				mciGetDeviceIDA    		[WINMM.@]
 */
UINT WINAPI mciGetDeviceIDA(LPCSTR lpstrName)
{
    return MCI_GetDriverFromString(lpstrName);
}

/**************************************************************************
 * 				mciGetDeviceIDW		       	[WINMM.@]
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
UINT WINAPI MCI_DefYieldProc(MCIDEVICEID wDevID, DWORD data)
{
    INT16	ret;

    TRACE("(0x%04x, 0x%08lx)\n", wDevID, data);

    if ((HIWORD(data) != 0 && HWND_16(GetActiveWindow()) != HIWORD(data)) ||
	(GetAsyncKeyState(LOWORD(data)) & 1) == 0) {
	UserYield16();
	ret = 0;
    } else {
	MSG		msg;

	msg.hwnd = HWND_32(HIWORD(data));
	while (!PeekMessageA(&msg, msg.hwnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE));
	ret = -1;
    }
    return ret;
}

/**************************************************************************
 * 				mciSetYieldProc			[MMSYSTEM.714]
 */
BOOL16 WINAPI mciSetYieldProc16(UINT16 uDeviceID, YIELDPROC16 fpYieldProc, DWORD dwYieldData)
{
    LPWINE_MCIDRIVER	wmd;

    TRACE("(%u, %p, %08lx)\n", uDeviceID, fpYieldProc, dwYieldData);

    if (!(wmd = MCI_GetDriver(uDeviceID))) {
	WARN("Bad uDeviceID\n");
	return FALSE;
    }

    wmd->lpfnYieldProc = (YIELDPROC)fpYieldProc;
    wmd->dwYieldData   = dwYieldData;
    wmd->bIs32         = FALSE;

    return TRUE;
}

/**************************************************************************
 * 				mciSetYieldProc			[WINMM.@]
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
 * 				mciGetDeviceIDFromElementIDW	[WINMM.@]
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
YIELDPROC16 WINAPI mciGetYieldProc16(UINT16 uDeviceID, DWORD* lpdwYieldData)
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
    return (YIELDPROC16)wmd->lpfnYieldProc;
}

/**************************************************************************
 * 				mciGetYieldProc			[WINMM.@]
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
    LPWINE_MCIDRIVER wmd;
    HTASK16 ret = 0;

    if ((wmd = MCI_GetDriver(uDeviceID))) ret = wmd->hCreatorTask;

    TRACE("(%u) => %04x\n", uDeviceID, ret);
    return ret;
}

/**************************************************************************
 * 				mciGetCreatorTask		[WINMM.@]
 */
HTASK WINAPI mciGetCreatorTask(UINT uDeviceID)
{
    LPWINE_MCIDRIVER	wmd;
    HTASK ret = 0;

    if ((wmd = MCI_GetDriver(uDeviceID))) ret = (HTASK)wmd->CreatorThread;

    TRACE("(%u) => %08x\n", uDeviceID, ret);
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
 * 			mciDriverYield				[WINMM.@]
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
 * 				midiOutGetNumDevs	[WINMM.@]
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
 * 				midiOutGetDevCapsW	[WINMM.@]
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
    MultiByteToWideChar( CP_ACP, 0, mocA.szPname, -1, lpCaps->szPname,
                         sizeof(lpCaps->szPname)/sizeof(WCHAR) );
    lpCaps->wTechnology	        = mocA.wTechnology;
    lpCaps->wVoices		= mocA.wVoices;
    lpCaps->wNotes		= mocA.wNotes;
    lpCaps->wChannelMask	= mocA.wChannelMask;
    lpCaps->dwSupport	        = mocA.dwSupport;
    return ret;
}

/**************************************************************************
 * 				midiOutGetDevCapsA	[WINMM.@]
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
 * 				midiOutGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI midiOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return MIDI_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				midiOutGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI midiOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret;

    ret = MIDI_GetErrorText(uError, xstr, uSize);
    MultiByteToWideChar( CP_ACP, 0, xstr, -1, lpText, uSize );
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
    HMIDIOUT	      	hMidiOut;
    LPWINE_MIDI		lpwm;
    UINT		size;

    size = sizeof(WINE_MIDI) + (cIDs ? (cIDs-1) : 0) * sizeof(MIDIOPENSTRMID);

    lpwm = (LPWINE_MIDI)MMDRV_Alloc(size, MMDRV_MIDIOUT, &hMidiOut, lpdwFlags,
				    lpdwCallback, lpdwInstance, bFrom32);

    if (lphMidiOut != NULL)
	*lphMidiOut = hMidiOut;

    if (lpwm) {
	lpwm->mod.hMidi = (HMIDI) hMidiOut;
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
 * 				midiOutOpen    		[WINMM.@]
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

    if (lphMidiOut != NULL) *lphMidiOut = HMIDIOUT_16(hmo);
    return ret;
}

/**************************************************************************
 * 				midiOutClose		[WINMM.@]
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
    return midiOutClose(HMIDIOUT_32(hMidiOut));
}

/**************************************************************************
 * 				midiOutPrepareHeader	[WINMM.@]
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
UINT16 WINAPI midiOutPrepareHeader16(HMIDIOUT16 hMidiOut,         /* [in] */
                                     SEGPTR lpsegMidiOutHdr,      /* [???] */
				     UINT16 uSize)                /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_PREPARE, lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutUnprepareHeader	[WINMM.@]
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
UINT16 WINAPI midiOutUnprepareHeader16(HMIDIOUT16 hMidiOut,         /* [in] */
				       SEGPTR lpsegMidiOutHdr,      /* [???] */
				       UINT16 uSize)                /* [in] */
{
    LPWINE_MLD		wmld;
    LPMIDIHDR16		lpMidiOutHdr = MapSL(lpsegMidiOutHdr);

    TRACE("(%04X, %08lx, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if (!(lpMidiOutHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_UNPREPARE, (DWORD)lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutShortMsg		[WINMM.@]
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
    return midiOutShortMsg(HMIDIOUT_32(hMidiOut), dwMsg);
}

/**************************************************************************
 * 				midiOutLongMsg		[WINMM.@]
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
UINT16 WINAPI midiOutLongMsg16(HMIDIOUT16 hMidiOut,          /* [in] */
                               LPMIDIHDR16 lpsegMidiOutHdr,  /* [???] NOTE: SEGPTR */
			       UINT16 uSize)                 /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiOut, lpsegMidiOutHdr, uSize);

    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MODM_LONGDATA, (DWORD)lpsegMidiOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiOutReset		[WINMM.@]
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
    return midiOutReset(HMIDIOUT_32(hMidiOut));
}

/**************************************************************************
 * 				midiOutGetVolume	[WINMM.@]
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
 * 				midiOutSetVolume	[WINMM.@]
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
 * 				midiOutCachePatches		[WINMM.@]
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
    return midiOutCachePatches(HMIDIOUT_32(hMidiOut), uBank, lpwPatchArray,
			       uFlags);
}

/**************************************************************************
 * 				midiOutCacheDrumPatches	[WINMM.@]
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
    return midiOutCacheDrumPatches(HMIDIOUT_32(hMidiOut), uPatch, lpwKeyArray, uFlags);
}

/**************************************************************************
 * 				midiOutGetID		[WINMM.@]
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
    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiOutMessage		[WINMM.@]
 */
DWORD WINAPI midiOutMessage(HMIDIOUT hMidiOut, UINT uMessage,
			    DWORD dwParam1, DWORD dwParam2)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %04X, %08lX, %08lX)\n", hMidiOut, uMessage, dwParam1, dwParam2);

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

    if ((wmld = MMDRV_Get(HMIDIOUT_32(hMidiOut), MMDRV_MIDIOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    switch (uMessage) {
    case MODM_OPEN:
    case MODM_CLOSE:
	FIXME("can't handle OPEN or CLOSE message!\n");
	return MMSYSERR_NOTSUPPORTED;

    case MODM_GETVOLUME:
        return midiOutGetVolume16(hMidiOut, MapSL(dwParam1));
    case MODM_LONGDATA:
        return midiOutLongMsg16(hMidiOut, MapSL(dwParam1), dwParam2);
    case MODM_PREPARE:
        /* lpMidiOutHdr is still a segmented pointer for this function */
        return midiOutPrepareHeader16(hMidiOut, dwParam1, dwParam2);
    case MODM_UNPREPARE:
        return midiOutUnprepareHeader16(hMidiOut, dwParam1, dwParam2);
    }
    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
}

/**************************************************************************
 * 				midiInGetNumDevs	[WINMM.@]
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
 * 				midiInGetDevCapsW	[WINMM.@]
 */
UINT WINAPI midiInGetDevCapsW(UINT uDeviceID, LPMIDIINCAPSW lpCaps, UINT uSize)
{
    MIDIINCAPSA		micA;
    UINT		ret = midiInGetDevCapsA(uDeviceID, &micA, uSize);

    if (ret == MMSYSERR_NOERROR) {
	lpCaps->wMid = micA.wMid;
	lpCaps->wPid = micA.wPid;
	lpCaps->vDriverVersion = micA.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, micA.szPname, -1, lpCaps->szPname,
                             sizeof(lpCaps->szPname)/sizeof(WCHAR) );
	lpCaps->dwSupport = micA.dwSupport;
    }
    return ret;
}

/**************************************************************************
 * 				midiInGetDevCapsA	[WINMM.@]
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
 * 				midiInGetErrorTextW 		[WINMM.@]
 */
UINT WINAPI midiInGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = MIDI_GetErrorText(uError, xstr, uSize);

    MultiByteToWideChar( CP_ACP, 0, xstr, -1, lpText, uSize );
    HeapFree(GetProcessHeap(), 0, xstr);
    return ret;
}

/**************************************************************************
 * 				midiInGetErrorTextA 		[WINMM.@]
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
    HMIDIIN		hMidiIn;
    LPWINE_MIDI		lpwm;
    DWORD		dwRet = 0;

    TRACE("(%p, %d, %08lX, %08lX, %08lX);\n",
	  lphMidiIn, uDeviceID, dwCallback, dwInstance, dwFlags);

    if (lphMidiIn != NULL) *lphMidiIn = 0;

    lpwm = (LPWINE_MIDI)MMDRV_Alloc(sizeof(WINE_MIDI), MMDRV_MIDIIN, &hMidiIn,
				    &dwFlags, &dwCallback, &dwInstance, bFrom32);

    if (lpwm == NULL)
	return MMSYSERR_NOMEM;

    lpwm->mod.hMidi = (HMIDI) hMidiIn;
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
 * 				midiInOpen		[WINMM.@]
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

    if (lphMidiIn) *lphMidiIn = HMIDIIN_16(xhmid);
    return ret;
}

/**************************************************************************
 * 				midiInClose		[WINMM.@]
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
    return midiInClose(HMIDIIN_32(hMidiIn));
}

/**************************************************************************
 * 				midiInPrepareHeader	[WINMM.@]
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
UINT16 WINAPI midiInPrepareHeader16(HMIDIIN16 hMidiIn,         /* [in] */
                                    SEGPTR lpsegMidiInHdr,     /* [???] */
				    UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_PREPARE, (DWORD)lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInUnprepareHeader	[WINMM.@]
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
UINT16 WINAPI midiInUnprepareHeader16(HMIDIIN16 hMidiIn,         /* [in] */
                                      SEGPTR lpsegMidiInHdr,     /* [???] */
				      UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;
    LPMIDIHDR16		lpMidiInHdr = MapSL(lpsegMidiInHdr);

    TRACE("(%04X, %08lx, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if (!(lpMidiInHdr->dwFlags & MHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_UNPREPARE, (DWORD)lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInAddBuffer		[WINMM.@]
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
UINT16 WINAPI midiInAddBuffer16(HMIDIIN16 hMidiIn,         /* [in] */
                                MIDIHDR16* lpsegMidiInHdr, /* [???] NOTE: SEGPTR */
				UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %d)\n", hMidiIn, lpsegMidiInHdr, uSize);

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, MIDM_ADDBUFFER, (DWORD)lpsegMidiInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				midiInStart			[WINMM.@]
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
    return midiInStart(HMIDIIN_32(hMidiIn));
}

/**************************************************************************
 * 				midiInStop			[WINMM.@]
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
    return midiInStop(HMIDIIN_32(hMidiIn));
}

/**************************************************************************
 * 				midiInReset			[WINMM.@]
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
    return midiInReset(HMIDIIN_32(hMidiIn));
}

/**************************************************************************
 * 				midiInGetID			[WINMM.@]
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

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, TRUE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				midiInMessage		[WINMM.@]
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
        return midiInGetDevCaps16(hMidiIn, MapSL(dwParam1), dwParam2);
    case MIDM_PREPARE:
        return midiInPrepareHeader16(hMidiIn, dwParam1, dwParam2);
    case MIDM_UNPREPARE:
        return midiInUnprepareHeader16(hMidiIn, dwParam1, dwParam2);
    case MIDM_ADDBUFFER:
        return midiInAddBuffer16(hMidiIn, MapSL(dwParam1), dwParam2);
    }

    if ((wmld = MMDRV_Get(HMIDIIN_32(hMidiIn), MMDRV_MIDIIN, FALSE)) == NULL)
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

	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
			   lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
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

	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
			   lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
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
    PeekMessageA(&msg, 0, 0, 0, 0);

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
	    GetMessageA(&msg, 0, 0, 0);
	    do {
		if (!MMSYSTEM_MidiStream_MessageHandler(lpMidiStrm, lpwm, &msg))
		    goto the_end;
	    } while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE));
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
		    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
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
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_POSITIONCB,
			   lpwm->mod.dwInstance, (LPARAM)lpMidiHdr, 0L);
	}
	lpMidiHdr->dwOffset += sizeof(MIDIEVENT) - sizeof(me->dwParms);
	if (me->dwEvent & MEVT_F_LONG)
	    lpMidiHdr->dwOffset += (MEVT_EVENTPARM(me->dwEvent) + 3) & ~3;
	if (lpMidiHdr->dwOffset >= lpMidiHdr->dwBufferLength) {
	    /* done with this header */
	    lpMidiHdr->dwFlags |= MHDR_DONE;
	    lpMidiHdr->dwFlags &= ~MHDR_INQUEUE;

	    lpMidiStrm->lpMidiHdr = (LPMIDIHDR)lpMidiHdr->lpNext;
	    DriverCallback(lpwm->mod.dwCallback, lpMidiStrm->wFlags,
			   (HDRVR)lpMidiStrm->hDevice, MM_MOM_DONE,
			   lpwm->mod.dwInstance, (DWORD)lpMidiHdr, 0L);
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
    if (PostThreadMessageA(lpMidiStrm->dwThreadID, msg, pmt1, pmt2)) {
	DWORD	count;

	ReleaseThunkLock(&count);
	WaitForSingleObject(lpMidiStrm->hEvent, INFINITE);
	RestoreThunkLock(count);
    } else {
	WARN("bad PostThreadMessageA\n");
	return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 * 				midiStreamClose			[WINMM.@]
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

    return midiOutClose((HMIDIOUT)hMidiStrm);
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
	*lphMidiStrm = (HMIDISTRM)hMidiOut;

    /* FIXME: is lpuDevice initialized upon entering midiStreamOpen ? */
    FIXME("*lpuDeviceID=%x\n", *lpuDeviceID);
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

    /* wait for thread to have started, and for its queue to be created */
    {
	DWORD	count;

	/* (Release|Restore)ThunkLock() is needed when this method is called from 16 bit code,
	 * (meaning the Win16Lock is set), so that it's released and the 32 bit thread running
	 * MMSYSTEM_MidiStreamPlayer can acquire Win16Lock to create its queue.
	 */
	ReleaseThunkLock(&count);
	WaitForSingleObject(lpMidiStrm->hEvent, INFINITE);
	RestoreThunkLock(count);
    }

    TRACE("=> (%u/%d) hMidi=0x%04x ret=%d lpMidiStrm=%p\n",
	  *lpuDeviceID, lpwm->mld.uDeviceID, *lphMidiStrm, ret, lpMidiStrm);
    return ret;
}

/**************************************************************************
 * 				midiStreamOpen			[WINMM.@]
 */
MMRESULT WINAPI midiStreamOpen(HMIDISTRM* lphMidiStrm, LPUINT lpuDeviceID,
			       DWORD cMidi, DWORD dwCallback,
			       DWORD dwInstance, DWORD fdwOpen)
{
    return MMSYSTEM_MidiStream_Open(lphMidiStrm, lpuDeviceID, cMidi, dwCallback,
				    dwInstance, fdwOpen, TRUE);
}

/**************************************************************************
 * 				midiStreamOut			[WINMM.@]
 */
MMRESULT WINAPI midiStreamOut(HMIDISTRM hMidiStrm, LPMIDIHDR lpMidiHdr,
			      UINT cbMidiHdr)
{
    WINE_MIDIStream*	lpMidiStrm;
    DWORD		ret = MMSYSERR_NOERROR;

    TRACE("(%08x, %p, %u)!\n", hMidiStrm, lpMidiHdr, cbMidiHdr);

    if (!MMSYSTEM_GetMidiStream(hMidiStrm, &lpMidiStrm, NULL)) {
	ret = MMSYSERR_INVALHANDLE;
    } else if (!lpMidiHdr) {
        ret = MMSYSERR_INVALPARAM;
    } else {
	if (!PostThreadMessageA(lpMidiStrm->dwThreadID,
                                WINE_MSM_HEADER, cbMidiHdr,
                                (DWORD)lpMidiHdr)) {
	    WARN("bad PostThreadMessageA\n");
	    ret = MMSYSERR_ERROR;
	}
    }
    return ret;
}

/**************************************************************************
 * 				midiStreamPause			[WINMM.@]
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
 * 				midiStreamPosition		[WINMM.@]
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
 * 				midiStreamProperty		[WINMM.@]
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
 * 				midiStreamRestart		[WINMM.@]
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
 * 				midiStreamStop			[WINMM.@]
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
    return midiStreamClose(HMIDISTRM_32(hMidiStrm));
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
    *phMidiStrm = HMIDISTRM_16(hMidiStrm32);
    *devid = devid32;
    return ret;
}

/**************************************************************************
 * 				midiStreamOut			[MMSYSTEM.254]
 */
MMRESULT16 WINAPI midiStreamOut16(HMIDISTRM16 hMidiStrm, LPMIDIHDR16 lpMidiHdr, UINT16 cbMidiHdr)
{
    return midiStreamOut(HMIDISTRM_32(hMidiStrm), (LPMIDIHDR)lpMidiHdr,
		         cbMidiHdr);
}

/**************************************************************************
 * 				midiStreamPause			[MMSYSTEM.255]
 */
MMRESULT16 WINAPI midiStreamPause16(HMIDISTRM16 hMidiStrm)
{
    return midiStreamPause(HMIDISTRM_32(hMidiStrm));
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
    ret = midiStreamPosition(HMIDISTRM_32(hMidiStrm), &mmt32, sizeof(MMTIME));
    MMSYSTEM_MMTIME32to16(lpmmt16, &mmt32);
    return ret;
}

/**************************************************************************
 * 				midiStreamProperty		[MMSYSTEM.250]
 */
MMRESULT16 WINAPI midiStreamProperty16(HMIDISTRM16 hMidiStrm, LPBYTE lpPropData, DWORD dwProperty)
{
    return midiStreamProperty(HMIDISTRM_32(hMidiStrm), lpPropData, dwProperty);
}

/**************************************************************************
 * 				midiStreamRestart		[MMSYSTEM.256]
 */
MMRESULT16 WINAPI midiStreamRestart16(HMIDISTRM16 hMidiStrm)
{
    return midiStreamRestart(HMIDISTRM_32(hMidiStrm));
}

/**************************************************************************
 * 				midiStreamStop			[MMSYSTEM.257]
 */
MMRESULT16 WINAPI midiStreamStop16(HMIDISTRM16 hMidiStrm)
{
    return midiStreamStop(HMIDISTRM_32(hMidiStrm));
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
    if ((dwFlags & WAVE_MAPPED) && (uDeviceID == (UINT)-1))
	return MMSYSERR_INVALPARAM;

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
    wod.dnDevNode = 0L;

    for (;;) {
        if (dwFlags & WAVE_MAPPED) {
            wod.uMappedDeviceID = uDeviceID;
            uDeviceID = WAVE_MAPPER;
        } else {
            wod.uMappedDeviceID = -1;
        }
        wmld->uDeviceID = uDeviceID;
    
        dwRet = MMDRV_Open(wmld, (uType == MMDRV_WAVEOUT) ? WODM_OPEN : WIDM_OPEN, 
                           (DWORD)&wod, dwFlags);

        if (dwRet != WAVERR_BADFORMAT ||
            (dwFlags & (WAVE_MAPPED|WAVE_FORMAT_DIRECT)) != 0) break;
        /* if we ask for a format which isn't supported by the physical driver, 
         * let's try to map it through the wave mapper (except, if we already tried
         * or user didn't allow us to use acm codecs)
         */
        dwFlags |= WAVE_MAPPED;
        /* we shall loop only one */
    }

    if ((dwFlags & WAVE_FORMAT_QUERY) || dwRet != MMSYSERR_NOERROR) {
        MMDRV_Free(handle, wmld);
        handle = 0;
    }

    if (lphndl != NULL) *lphndl = handle;
    TRACE("=> %ld hWave=%04x\n", dwRet, handle);

    return dwRet;
}

/**************************************************************************
 * 				waveOutGetNumDevs		[WINMM.@]
 */
UINT WINAPI waveOutGetNumDevs(void)
{
    return MMDRV_GetNum(MMDRV_WAVEOUT);
}

/**************************************************************************
 * 				waveOutGetNumDevs		[MMSYSTEM.401]
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
 * 				waveOutGetDevCapsA		[WINMM.@]
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
 * 				waveOutGetDevCapsW		[WINMM.@]
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
        MultiByteToWideChar( CP_ACP, 0, wocA.szPname, -1, lpCaps->szPname,
                             sizeof(lpCaps->szPname)/sizeof(WCHAR) );
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
 * 				waveOutGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI waveOutGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return WAVE_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveOutGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI waveOutGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR	xstr = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = WAVE_GetErrorText(uError, xstr, uSize);

    MultiByteToWideChar( CP_ACP, 0, xstr, -1, lpText, uSize );
    HeapFree(GetProcessHeap(), 0, xstr);
    return ret;
}

/**************************************************************************
 *			waveOutOpen			[WINMM.@]
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

    /* since layout of WAVEFORMATEX is the same for 16/32 bits, we directly
     * call the 32 bit version
     * however, we need to promote correctly the wave mapper id
     * (0xFFFFFFFF and not 0x0000FFFF)
     */
    ret = MMSYSTEM_waveOpen(&hWaveOut, (uDeviceID == (UINT16)-1) ? (UINT)-1 : uDeviceID,
                            MMDRV_WAVEOUT, lpFormat, dwCallback, dwInstance, dwFlags, FALSE);

    if (lphWaveOut != NULL) *lphWaveOut = HWAVEOUT_16(hWaveOut);
    return ret;
}

/**************************************************************************
 * 				waveOutClose		[WINMM.@]
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
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutClose(HWAVEOUT_32(hWaveOut));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutPrepareHeader	[WINMM.@]
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
UINT16 WINAPI waveOutPrepareHeader16(HWAVEOUT16 hWaveOut,      /* [in] */
                                     SEGPTR lpsegWaveOutHdr,   /* [???] */
				     UINT16 uSize)             /* [in] */
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveOutHdr = MapSL(lpsegWaveOutHdr);

    TRACE("(%04X, %08lx, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if (lpWaveOutHdr == NULL) return MMSYSERR_INVALPARAM;

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_PREPARE, (DWORD)lpsegWaveOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveOutUnprepareHeader	[WINMM.@]
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
UINT16 WINAPI waveOutUnprepareHeader16(HWAVEOUT16 hWaveOut,       /* [in] */
				       SEGPTR lpsegWaveOutHdr,    /* [???] */
				       UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveOutHdr = MapSL(lpsegWaveOutHdr);

    TRACE("(%04X, %08lx, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if (!(lpWaveOutHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_UNPREPARE, (DWORD)lpsegWaveOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveOutWrite		[WINMM.@]
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
UINT16 WINAPI waveOutWrite16(HWAVEOUT16 hWaveOut,       /* [in] */
			     LPWAVEHDR lpsegWaveOutHdr, /* [???] NOTE: SEGPTR */
			     UINT16 uSize)              /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %u);\n", hWaveOut, lpsegWaveOutHdr, uSize);

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_WRITE, (DWORD)lpsegWaveOutHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveOutBreakLoop	[WINMM.@]
 */
UINT WINAPI waveOutBreakLoop(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X);\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_BREAKLOOP, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutBreakLoop	[MMSYSTEM.419]
 */
UINT16 WINAPI waveOutBreakLoop16(HWAVEOUT16 hWaveOut16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutBreakLoop(HWAVEOUT_32(hWaveOut16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutPause		[WINMM.@]
 */
UINT WINAPI waveOutPause(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X);\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_PAUSE, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutPause		[MMSYSTEM.409]
 */
UINT16 WINAPI waveOutPause16(HWAVEOUT16 hWaveOut16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutPause(HWAVEOUT_32(hWaveOut16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutReset		[WINMM.@]
 */
UINT WINAPI waveOutReset(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X);\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_RESET, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutReset		[MMSYSTEM.411]
 */
UINT16 WINAPI waveOutReset16(HWAVEOUT16 hWaveOut16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutReset(HWAVEOUT_32(hWaveOut16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutRestart		[WINMM.@]
 */
UINT WINAPI waveOutRestart(HWAVEOUT hWaveOut)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X);\n", hWaveOut);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_RESTART, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutRestart	[MMSYSTEM.410]
 */
UINT16 WINAPI waveOutRestart16(HWAVEOUT16 hWaveOut16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveOutRestart(HWAVEOUT_32(hWaveOut16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveOutGetPosition	[WINMM.@]
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
    ret = waveOutGetPosition(HWAVEOUT_32(hWaveOut), &mmt, sizeof(mmt));
    MMSYSTEM_MMTIME32to16(lpTime, &mmt);
    return ret;
}

/**************************************************************************
 * 				waveOutGetPitch		[WINMM.@]
 */
UINT WINAPI waveOutGetPitch(HWAVEOUT hWaveOut, LPDWORD lpdw)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx);\n", hWaveOut, (DWORD)lpdw);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_GETPITCH, (DWORD)lpdw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutGetPitch		[MMSYSTEM.413]
 */
UINT16 WINAPI waveOutGetPitch16(HWAVEOUT16 hWaveOut16, LPDWORD lpdw)
{
    return waveOutGetPitch(HWAVEOUT_32(hWaveOut16), lpdw);
}

/**************************************************************************
 * 				waveOutSetPitch		[WINMM.@]
 */
UINT WINAPI waveOutSetPitch(HWAVEOUT hWaveOut, DWORD dw)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx);\n", hWaveOut, (DWORD)dw);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_SETPITCH, dw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutSetPitch		[MMSYSTEM.414]
 */
UINT16 WINAPI waveOutSetPitch16(HWAVEOUT16 hWaveOut16, DWORD dw)
{
    return waveOutSetPitch(HWAVEOUT_32(hWaveOut16), dw);
}

/**************************************************************************
 * 				waveOutGetPlaybackRate	[WINMM.@]
 */
UINT WINAPI waveOutGetPlaybackRate(HWAVEOUT hWaveOut, LPDWORD lpdw)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx);\n", hWaveOut, (DWORD)lpdw);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_GETPLAYBACKRATE, (DWORD)lpdw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutGetPlaybackRate	[MMSYSTEM.417]
 */
UINT16 WINAPI waveOutGetPlaybackRate16(HWAVEOUT16 hWaveOut16, LPDWORD lpdw)
{
    return waveOutGetPlaybackRate(HWAVEOUT_32(hWaveOut16), lpdw);
}

/**************************************************************************
 * 				waveOutSetPlaybackRate	[WINMM.@]
 */
UINT WINAPI waveOutSetPlaybackRate(HWAVEOUT hWaveOut, DWORD dw)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx);\n", hWaveOut, (DWORD)dw);

    if ((wmld = MMDRV_Get(hWaveOut, MMDRV_WAVEOUT, FALSE)) == NULL)
        return MMSYSERR_INVALHANDLE;
    return MMDRV_Message(wmld, WODM_SETPLAYBACKRATE, dw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutSetPlaybackRate	[MMSYSTEM.418]
 */
UINT16 WINAPI waveOutSetPlaybackRate16(HWAVEOUT16 hWaveOut16, DWORD dw)
{
    return waveOutSetPlaybackRate(HWAVEOUT_32(hWaveOut16), dw);
}

/**************************************************************************
 * 				waveOutGetVolume	[WINMM.@]
 */
UINT WINAPI waveOutGetVolume(UINT devid, LPDWORD lpdw)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx);\n", devid, (DWORD)lpdw);

     if ((wmld = MMDRV_Get(devid, MMDRV_WAVEOUT, TRUE)) == NULL)
        return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_GETVOLUME, (DWORD)lpdw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutGetVolume	[MMSYSTEM.415]
 */
UINT16 WINAPI waveOutGetVolume16(UINT16 devid, LPDWORD lpdw)
{
    return waveOutGetVolume(devid, lpdw);
}

/**************************************************************************
 * 				waveOutSetVolume	[WINMM.@]
 */
UINT WINAPI waveOutSetVolume(UINT devid, DWORD dw)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %08lx);\n", devid, dw);

     if ((wmld = MMDRV_Get(devid, MMDRV_WAVEOUT, TRUE)) == NULL)
        return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WODM_SETVOLUME, dw, 0L, TRUE);
}

/**************************************************************************
 * 				waveOutSetVolume	[MMSYSTEM.416]
 */
UINT16 WINAPI waveOutSetVolume16(UINT16 devid, DWORD dw)
{
    return waveOutSetVolume(devid, dw);
}

/**************************************************************************
 * 				waveOutGetID		[WINMM.@]
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

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return 0;
}

/**************************************************************************
 * 				waveOutMessage 		[WINMM.@]
 */
DWORD WINAPI waveOutMessage(HWAVEOUT hWaveOut, UINT uMessage,
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

    if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, FALSE)) == NULL) {
	if ((wmld = MMDRV_Get(HWAVEOUT_32(hWaveOut), MMDRV_WAVEOUT, TRUE)) != NULL) {
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
 * 				waveInGetNumDevs 		[WINMM.@]
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
 * 				waveInGetDevCapsW 		[WINMM.@]
 */
UINT WINAPI waveInGetDevCapsW(UINT uDeviceID, LPWAVEINCAPSW lpCaps, UINT uSize)
{
    WAVEINCAPSA		wicA;
    UINT		ret = waveInGetDevCapsA(uDeviceID, &wicA, uSize);

    if (ret == MMSYSERR_NOERROR) {
	lpCaps->wMid = wicA.wMid;
	lpCaps->wPid = wicA.wPid;
	lpCaps->vDriverVersion = wicA.vDriverVersion;
        MultiByteToWideChar( CP_ACP, 0, wicA.szPname, -1, lpCaps->szPname,
                             sizeof(lpCaps->szPname)/sizeof(WCHAR) );
	lpCaps->dwFormats = wicA.dwFormats;
	lpCaps->wChannels = wicA.wChannels;
    }

    return ret;
}

/**************************************************************************
 * 				waveInGetDevCapsA 		[WINMM.@]
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
 * 				waveInGetErrorTextA 	[WINMM.@]
 */
UINT WINAPI waveInGetErrorTextA(UINT uError, LPSTR lpText, UINT uSize)
{
    return WAVE_GetErrorText(uError, lpText, uSize);
}

/**************************************************************************
 * 				waveInGetErrorTextW 	[WINMM.@]
 */
UINT WINAPI waveInGetErrorTextW(UINT uError, LPWSTR lpText, UINT uSize)
{
    LPSTR txt = HeapAlloc(GetProcessHeap(), 0, uSize);
    UINT	ret = WAVE_GetErrorText(uError, txt, uSize);

    MultiByteToWideChar( CP_ACP, 0, txt, -1, lpText, uSize );
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
 * 				waveInOpen			[WINMM.@]
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

    /* since layout of WAVEFORMATEX is the same for 16/32 bits, we directly
     * call the 32 bit version
     * however, we need to promote correctly the wave mapper id
     * (0xFFFFFFFF and not 0x0000FFFF)
     */
    ret = MMSYSTEM_waveOpen(&hWaveIn, (uDeviceID == (UINT16)-1) ? (UINT)-1 : uDeviceID,
			    MMDRV_WAVEIN, lpFormat, dwCallback, dwInstance, dwFlags, FALSE);

    if (lphWaveIn != NULL) *lphWaveIn = HWAVEIN_16(hWaveIn);
    return ret;
}

/**************************************************************************
 * 				waveInClose			[WINMM.@]
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
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveInClose(HWAVEIN_32(hWaveIn));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveInPrepareHeader		[WINMM.@]
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
UINT16 WINAPI waveInPrepareHeader16(HWAVEIN16 hWaveIn,       /* [in] */
				    SEGPTR lpsegWaveInHdr,   /* [???] */
				    UINT16 uSize)            /* [in] */
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveInHdr = MapSL(lpsegWaveInHdr);
    UINT16		ret;

    TRACE("(%04X, %p, %u);\n", hWaveIn, lpWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALHANDLE;
    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    lpWaveInHdr->dwBytesRecorded = 0;

    ret = MMDRV_Message(wmld, WIDM_PREPARE, (DWORD)lpsegWaveInHdr, uSize, FALSE);
    return ret;
}

/**************************************************************************
 * 				waveInUnprepareHeader	[WINMM.@]
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
UINT16 WINAPI waveInUnprepareHeader16(HWAVEIN16 hWaveIn,       /* [in] */
				      SEGPTR lpsegWaveInHdr,   /* [???] */
				      UINT16 uSize)            /* [in] */
{
    LPWINE_MLD		wmld;
    LPWAVEHDR		lpWaveInHdr = MapSL(lpsegWaveInHdr);

    TRACE("(%04X, %08lx, %u);\n", hWaveIn, lpsegWaveInHdr, uSize);

    if (lpWaveInHdr == NULL) return MMSYSERR_INVALPARAM;

    if (!(lpWaveInHdr->dwFlags & WHDR_PREPARED)) {
	return MMSYSERR_NOERROR;
    }

    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_UNPREPARE, (DWORD)lpsegWaveInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveInAddBuffer		[WINMM.@]
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
UINT16 WINAPI waveInAddBuffer16(HWAVEIN16 hWaveIn,       /* [in] */
				WAVEHDR* lpsegWaveInHdr, /* [???] NOTE: SEGPTR */
				UINT16 uSize)            /* [in] */
{
    LPWINE_MLD		wmld;

    TRACE("(%04X, %p, %u);\n", hWaveIn, lpsegWaveInHdr, uSize);

    if (lpsegWaveInHdr == NULL) return MMSYSERR_INVALPARAM;
    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_ADDBUFFER, (DWORD)lpsegWaveInHdr, uSize, FALSE);
}

/**************************************************************************
 * 				waveInReset		[WINMM.@]
 */
UINT WINAPI waveInReset(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_RESET, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveInReset		[MMSYSTEM.511]
 */
UINT16 WINAPI waveInReset16(HWAVEIN16 hWaveIn16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveInReset(HWAVEIN_32(hWaveIn16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveInStart		[WINMM.@]
 */
UINT WINAPI waveInStart(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, WIDM_START, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveInStart		[MMSYSTEM.509]
 */
UINT16 WINAPI waveInStart16(HWAVEIN16 hWaveIn16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveInStart(HWAVEIN_32(hWaveIn16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveInStop		[WINMM.@]
 */
UINT WINAPI waveInStop(HWAVEIN hWaveIn)
{
    LPWINE_MLD		wmld;

    TRACE("(%04X);\n", hWaveIn);

    if ((wmld = MMDRV_Get(hWaveIn, MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld,WIDM_STOP, 0L, 0L, TRUE);
}

/**************************************************************************
 * 				waveInStop		[MMSYSTEM.510]
 */
UINT16 WINAPI waveInStop16(HWAVEIN16 hWaveIn16)
{
    DWORD	level;
    UINT16	ret;

    ReleaseThunkLock(&level);
    ret = waveInStop(HWAVEIN_32(hWaveIn16));
    RestoreThunkLock(level);
    return ret;
}

/**************************************************************************
 * 				waveInGetPosition	[WINMM.@]
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
    ret = waveInGetPosition(HWAVEIN_32(hWaveIn), &mmt, sizeof(mmt));
    MMSYSTEM_MMTIME32to16(lpTime, &mmt);
    return ret;
}

/**************************************************************************
 * 				waveInGetID			[WINMM.@]
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

    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    *lpuDeviceID = wmld->uDeviceID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				waveInMessage 		[WINMM.@]
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

    if ((wmld = MMDRV_Get(HWAVEIN_32(hWaveIn), MMDRV_WAVEIN, FALSE)) == NULL)
	return MMSYSERR_INVALHANDLE;

    return MMDRV_Message(wmld, uMessage, dwParam1, dwParam2, TRUE);
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
    HINSTANCE16 	ret;
    HINSTANCE16		handle;
    char cmdline[16];
    DWORD showCmd = 0x40002;
    LOADPARAMS16 lp;

    TRACE("(%08lx, %p, %08lx);\n", spProc, lphMmTask, dwPmt);
    /* This to work requires NE modules to be started with a binary command line
     * which is not currently the case. A patch exists but has never been committed.
     * A workaround would be to integrate code for mmtask.tsk into Wine, but
     * this requires tremendous work (starting with patching tools/build to
     * create NE executables (and not only DLLs) for builtins modules.
     * EP 99/04/25
     */
    FIXME("This is currently broken. It will fail\n");

    cmdline[0] = 0x0d;
    *(LPDWORD)(cmdline + 1) = (DWORD)spProc;
    *(LPDWORD)(cmdline + 5) = dwPmt;
    *(LPDWORD)(cmdline + 9) = 0;

    lp.hEnvironment = 0;
    lp.cmdLine = MapLS(cmdline);
    lp.showCmd = MapLS(&showCmd);
    lp.reserved = 0;

#ifndef USE_MM_TSK_WINE
    handle = LoadModule16("c:\\windows\\system\\mmtask.tsk", &lp);
#else
    handle = LoadModule16("mmtask.tsk", &lp);
#endif
    if (handle < 32) {
	ret = (handle) ? 1 : 2;
	handle = 0;
    } else {
	ret = 0;
    }
    if (lphMmTask)
	*lphMmTask = handle;

    UnMapLS( lp.cmdLine );
    UnMapLS( lp.showCmd );
    TRACE("=> 0x%04x/%d\n", handle, ret);
    return ret;
}

#ifdef USE_MM_TSK_WINE
/* C equivalent to mmtask.tsk binary content */
void	mmTaskEntryPoint16(LPSTR cmdLine, WORD di, WORD si)
{
    int	len = cmdLine[0x80];

    if (len / 2 == 6) {
	void	(*fpProc)(DWORD) = MapSL(*((DWORD*)(cmdLine + 1)));
	DWORD	dwPmt  = *((DWORD*)(cmdLine + 5));

#if 0
	InitTask16(); /* FIXME: pmts / from context ? */
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
    return PostAppMessage16(ht, WM_USER, 0, 0);
}

/**************************************************************************
 * 				mmGetCurrentTask	[MMSYSTEM.904]
 */
HTASK16 WINAPI mmGetCurrentTask16(void)
{
    return GetCurrentTask();
}

/**************************************************************************
 * 				mmTaskYield		[MMSYSTEM.905]
 */
void	WINAPI	mmTaskYield16(void)
{
    MSG		msg;

    if (PeekMessageA(&msg, 0, 0, 0, 0)) {
	K32WOWYield16();
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
	WINE_MMTHREAD*	lpMMThd = MapSL( MAKESEGPTR(hndl, 0) );

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
	    FARPROC16	fp = GetProcAddress16(GetModuleHandle16("MMSYSTEM"), (LPCSTR)2047);

	    TRACE("farproc seg=0x%08lx lin=%p\n", (DWORD)fp, MapSL((SEGPTR)fp));

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
	WINE_MMTHREAD*	lpMMThd = MapSL( MAKESEGPTR(hndl, 0) );

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
	    if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
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
	WINE_MMTHREAD*	lpMMThd = MapSL( MAKESEGPTR(hndl, 0) );

	if (lpMMThd->hThread != 0) {
	    DWORD	lc;

	    ReleaseThunkLock(&lc);
	    MMSYSTEM_ThreadBlock(lpMMThd);
	    RestoreThunkLock(lc);
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
	WINE_MMTHREAD*	lpMMThd = MapSL( MAKESEGPTR(hndl, 0) );
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
	WINE_MMTHREAD*	lpMMThd = MapSL( MAKESEGPTR(hndl, 0) );

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
	WINE_MMTHREAD*	lpMMThd = MapSL( MAKESEGPTR(hndl, 0) );
	ret = lpMMThd->hTask;
    }
    return ret;
}

/* ### start build ### */
extern LONG CALLBACK MMSYSTEM_CallTo16_long_l    (FARPROC16,LONG);
/* ### stop build ### */

/**************************************************************************
 * 				__wine_mmThreadEntryPoint (MMSYSTEM.2047)
 */
void WINAPI WINE_mmThreadEntryPoint(DWORD _pmt)
{
    HANDLE16		hndl = (HANDLE16)_pmt;
    WINE_MMTHREAD*	lpMMThd = MapSL( MAKESEGPTR(hndl, 0) );

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
	/* K32WOWYield16();*/
    }
    TRACE("[XX-%08x]\n", lpMMThd->hThread);
    /* paranoia */
    lpMMThd->dwSignature = WINE_MMTHREAD_DELETED;
    /* close lpMMThread->hVxD directIO */
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
	    DWORD	lc;
	    ReleaseThunkLock(&lc);
	    ret = (fp)(hWnd, lpStrDevice, lpStrTab, lpStrTitle);
	    RestoreThunkLock(lc);
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

/**************************************************************************
 * 			OutputDebugStr	 	[MMSYSTEM.30]
 */
void WINAPI OutputDebugStr16(
    LPCSTR str) /* [in] The message to be logged and given to the debugger. */
{
    OutputDebugStringA( str );
}
