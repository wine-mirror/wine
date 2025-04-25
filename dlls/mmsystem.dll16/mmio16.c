/*
 * MMSYSTEM mmio* functions
 *
 * Copyright 1993               Martin Ayotte
 *           1998-2003,2009     Eric Pouech
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winternl.h"
#include "wownt32.h"
#include "winnls.h"

#include "wine/winuser16.h"
#include "winemm16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmsys);

/* ###################################################
 * #                     MMIO                        #
 * ###################################################
 */
#pragma pack(push,1)
#define MMIO_MAX_THUNKS      32

static struct mmio_thunk
{
    BYTE        popl_eax;       /* popl  %eax (return address) */
    BYTE        pushl_func;     /* pushl $pfn16 (16bit callback function) */
    LPMMIOPROC16 pfn16;
    BYTE        pushl_eax;      /* pushl %eax */
    BYTE        jmp;            /* ljmp MMIO_Callback1632 */
    DWORD       callback;
    HMMIO       hMmio;          /* Handle to 32bit mmio object */
    SEGPTR      segbuffer;      /* actual segmented ptr to buffer */
} *MMIO_Thunks;

#pragma pack(pop)

static CRITICAL_SECTION mmio_cs;
static CRITICAL_SECTION_DEBUG mmio_critsect_debug =
{
    0, 0, &mmio_cs,
    { &mmio_critsect_debug.ProcessLocksList, &mmio_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": mmsystem_mmio_cs") }
};
static CRITICAL_SECTION mmio_cs = { &mmio_critsect_debug, -1, 0, 0, 0, 0 };

/****************************************************************
 *       		MMIO_Map32To16			[INTERNAL]
 */
static LRESULT	MMIO_Map32To16(DWORD wMsg, LPARAM* lp1, LPARAM* lp2)
{
    switch (wMsg) {
    case MMIOM_CLOSE:
    case MMIOM_SEEK:
	/* nothing to do */
	break;
    case MMIOM_OPEN:
    case MMIOM_READ:
    case MMIOM_WRITE:
    case MMIOM_WRITEFLUSH:
        *lp1 = MapLS( (void *)*lp1 );
	break;
    case MMIOM_RENAME:
        *lp1 = MapLS( (void *)*lp1 );
        *lp2 = MapLS( (void *)*lp2 );
        break;
    default:
        if (wMsg < MMIOM_USER)
            TRACE("Not a mappable message (%ld)\n", wMsg);
    }
    return MMSYSERR_NOERROR;
}

/****************************************************************
 *       	MMIO_UnMap32To16 			[INTERNAL]
 */
static LRESULT	MMIO_UnMap32To16(DWORD wMsg, LPARAM lParam1, LPARAM lParam2,
				 LPARAM lp1, LPARAM lp2)
{
    switch (wMsg) {
    case MMIOM_CLOSE:
    case MMIOM_SEEK:
	/* nothing to do */
	break;
    case MMIOM_OPEN:
    case MMIOM_READ:
    case MMIOM_WRITE:
    case MMIOM_WRITEFLUSH:
        UnMapLS( lp1 );
	break;
    case MMIOM_RENAME:
        UnMapLS( lp1 );
        UnMapLS( lp2 );
	break;
    default:
        if (wMsg < MMIOM_USER)
            TRACE("Not a mappable message (%ld)\n", wMsg);
    }
    return MMSYSERR_NOERROR;
}

/******************************************************************
 *		MMIO_Callback3216
 *
 *
 */
static LRESULT MMIO_Callback3216(SEGPTR cb16, LPMMIOINFO lpmmioinfo, UINT uMessage,
                                 LPARAM lParam1, LPARAM lParam2)
{
    DWORD 		result;
    MMIOINFO16          mmioInfo16;
    SEGPTR		segmmioInfo16;
    LPARAM		lp1 = lParam1, lp2 = lParam2;
    WORD                args[7];

    if (!cb16) return MMSYSERR_INVALPARAM;

    memset(&mmioInfo16, 0, sizeof(MMIOINFO16));
    mmioInfo16.lDiskOffset = lpmmioinfo->lDiskOffset;
    mmioInfo16.adwInfo[0]  = lpmmioinfo->adwInfo[0];
    mmioInfo16.adwInfo[1]  = lpmmioinfo->adwInfo[1];
    mmioInfo16.adwInfo[2]  = lpmmioinfo->adwInfo[2];
    /* map (lParam1, lParam2) into (lp1, lp2) 32=>16 */
    if ((result = MMIO_Map32To16(uMessage, &lp1, &lp2)) != MMSYSERR_NOERROR)
        return result;

    segmmioInfo16 = MapLS(&mmioInfo16);
    args[6] = HIWORD(segmmioInfo16);
    args[5] = LOWORD(segmmioInfo16);
    args[4] = uMessage;
    args[3] = HIWORD(lp1);
    args[2] = LOWORD(lp1);
    args[1] = HIWORD(lp2);
    args[0] = LOWORD(lp2);
    WOWCallback16Ex( cb16, WCB16_PASCAL, sizeof(args), args, &result );
    UnMapLS(segmmioInfo16);
    MMIO_UnMap32To16(uMessage, lParam1, lParam2, lp1, lp2);

    lpmmioinfo->lDiskOffset = mmioInfo16.lDiskOffset;
    lpmmioinfo->adwInfo[0]  = mmioInfo16.adwInfo[0];
    lpmmioinfo->adwInfo[1]  = mmioInfo16.adwInfo[1];
    lpmmioinfo->adwInfo[2]  = mmioInfo16.adwInfo[2];

    return result;
}

/******************************************************************
 *		MMIO_AddThunk
 *
 */
static struct mmio_thunk*       MMIO_AddThunk(LPMMIOPROC16 pfn16, HPSTR segbuf)
{
    struct mmio_thunk* thunk;

    if (!MMIO_Thunks)
    {
        MMIO_Thunks = VirtualAlloc(NULL, MMIO_MAX_THUNKS * sizeof(*MMIO_Thunks), MEM_COMMIT,
                                   PAGE_EXECUTE_READWRITE);
        if (!MMIO_Thunks) return NULL;
        for (thunk = MMIO_Thunks; thunk < &MMIO_Thunks[MMIO_MAX_THUNKS]; thunk++)
        {
            thunk->popl_eax     = 0x58;   /* popl  %eax */
            thunk->pushl_func   = 0x68;   /* pushl $pfn16 */
            thunk->pfn16        = 0;
            thunk->pushl_eax    = 0x50;   /* pushl %eax */
            thunk->jmp          = 0xe9;   /* jmp MMIO_Callback3216 */
            thunk->callback     = (char *)MMIO_Callback3216 - (char *)(&thunk->callback + 1);
            thunk->hMmio        = NULL;
            thunk->segbuffer    = 0;
        }
    }
    for (thunk = MMIO_Thunks; thunk < &MMIO_Thunks[MMIO_MAX_THUNKS]; thunk++)
    {
        if (thunk->pfn16 == 0 && thunk->hMmio == NULL)
        {
            thunk->pfn16 = pfn16;
            thunk->hMmio = NULL;
            thunk->segbuffer = (SEGPTR)segbuf;
            return thunk;
        }
    }
    FIXME("Out of mmio-thunks. Bump MMIO_MAX_THUNKS\n");
    return NULL;
}

/******************************************************************
 *		MMIO_HasThunk
 *
 */
static struct mmio_thunk*    MMIO_HasThunk(HMMIO hmmio)
{
    struct mmio_thunk* thunk;

    if (!MMIO_Thunks) return NULL;
    for (thunk = MMIO_Thunks; thunk < &MMIO_Thunks[MMIO_MAX_THUNKS]; thunk++)
    {
        if (thunk->hMmio == hmmio) return thunk;
    }
    return NULL;
}

/******************************************************************
 *             MMIO_SetSegmentedBuffer
 *
 */
static void     MMIO_SetSegmentedBuffer(struct mmio_thunk* thunk, SEGPTR ptr, BOOL release)
{
    if (release) UnMapLS(thunk->segbuffer);
    thunk->segbuffer = ptr;
}

/**************************************************************************
 * 				mmioOpen       		[MMSYSTEM.1210]
 */
HMMIO16 WINAPI mmioOpen16(LPSTR szFileName, MMIOINFO16* lpmmioinfo16,
			  DWORD dwOpenFlags)
{
    HMMIO 	ret;

    if (lpmmioinfo16) {
	MMIOINFO	        mmioinfo;
        struct mmio_thunk*      thunk = NULL;

	memset(&mmioinfo, 0, sizeof(mmioinfo));

        EnterCriticalSection(&mmio_cs);
        if (!(thunk = MMIO_AddThunk(lpmmioinfo16->pIOProc, lpmmioinfo16->pchBuffer)))
        {
            LeaveCriticalSection(&mmio_cs);
            return 0;
        }

	mmioinfo.dwFlags     = lpmmioinfo16->dwFlags;
	mmioinfo.fccIOProc   = lpmmioinfo16->fccIOProc;
	mmioinfo.pIOProc     = lpmmioinfo16->pIOProc ? (LPMMIOPROC)thunk : 0;
	mmioinfo.cchBuffer   = lpmmioinfo16->cchBuffer;
	mmioinfo.pchBuffer   = MapSL((DWORD)lpmmioinfo16->pchBuffer);
        mmioinfo.adwInfo[0]  = lpmmioinfo16->adwInfo[0];
        /* if we don't have a file name, it's likely a passed open file descriptor */
        if (!szFileName)
            mmioinfo.adwInfo[0] = (DWORD)DosFileHandleToWin32Handle(mmioinfo.adwInfo[0]);
	mmioinfo.adwInfo[1]  = lpmmioinfo16->adwInfo[1];
	mmioinfo.adwInfo[2]  = lpmmioinfo16->adwInfo[2];

	ret = mmioOpenA(szFileName, &mmioinfo, dwOpenFlags);
        if (!ret || (dwOpenFlags & (MMIO_PARSE|MMIO_EXIST)))
        {
            thunk->pfn16 = NULL;
            thunk->hMmio = NULL;
        }
        else thunk->hMmio = ret;
        if (ret && (dwOpenFlags & MMIO_ALLOCBUF))
        {
            MMIOINFO    m;
            if (lpmmioinfo16->pchBuffer) FIXME("ooch\n");
            /* FIXME: check whether mmioOpen should set pchBuffer */
            mmioGetInfo(ret, &m, 0);
            thunk->segbuffer = MapLS(m.pchBuffer);
        }
        LeaveCriticalSection(&mmio_cs);

	lpmmioinfo16->wErrorRet = mmioinfo.wErrorRet;
        lpmmioinfo16->hmmio     = HMMIO_16(mmioinfo.hmmio);
    } else {
	ret = mmioOpenA(szFileName, NULL, dwOpenFlags);
    }
    return HMMIO_16(ret);
}

/**************************************************************************
 * 				mmioClose      		[MMSYSTEM.1211]
 */
MMRESULT16 WINAPI mmioClose16(HMMIO16 hmmio, UINT16 uFlags)
{
    MMRESULT ret;

    EnterCriticalSection(&mmio_cs);
    ret = mmioClose(HMMIO_32(hmmio), uFlags);
    if (ret == MMSYSERR_NOERROR)
    {
        struct mmio_thunk* thunk;

        if ((thunk = MMIO_HasThunk(HMMIO_32(hmmio))))
        {
            MMIO_SetSegmentedBuffer(thunk, 0, TRUE);
            thunk->pfn16 = NULL;
            thunk->hMmio = NULL;
        }
    }
    LeaveCriticalSection(&mmio_cs);
    return ret;
}

/**************************************************************************
 * 				mmioRead	       	[MMSYSTEM.1212]
 */
LONG WINAPI mmioRead16(HMMIO16 hmmio, HPSTR pch, LONG cch)
{
    return mmioRead(HMMIO_32(hmmio), pch, cch);
}

/**************************************************************************
 * 				mmioWrite      		[MMSYSTEM.1213]
 */
LONG WINAPI mmioWrite16(HMMIO16 hmmio, HPCSTR pch, LONG cch)
{
    return mmioWrite(HMMIO_32(hmmio),pch,cch);
}

/**************************************************************************
 * 				mmioSeek       		[MMSYSTEM.1214]
 */
LONG WINAPI mmioSeek16(HMMIO16 hmmio, LONG lOffset, INT16 iOrigin)
{
    return mmioSeek(HMMIO_32(hmmio), lOffset, iOrigin);
}

/**************************************************************************
 * 				mmioGetInfo	       	[MMSYSTEM.1215]
 */
MMRESULT16 WINAPI mmioGetInfo16(HMMIO16 hmmio, MMIOINFO16* lpmmioinfo, UINT16 uFlags)
{
    MMIOINFO            mmioinfo;
    MMRESULT            ret;
    struct mmio_thunk*  thunk;

    TRACE("(0x%04x,%p,0x%08x)\n", hmmio, lpmmioinfo, uFlags);

    EnterCriticalSection(&mmio_cs);
    if ((thunk = MMIO_HasThunk(HMMIO_32(hmmio))) == NULL)
    {
        LeaveCriticalSection(&mmio_cs);
	return MMSYSERR_INVALHANDLE;
    }

    ret = mmioGetInfo(HMMIO_32(hmmio), &mmioinfo, uFlags);
    if (ret != MMSYSERR_NOERROR)
    {
        LeaveCriticalSection(&mmio_cs);
        return ret;
    }

    lpmmioinfo->dwFlags     = mmioinfo.dwFlags;
    lpmmioinfo->fccIOProc   = mmioinfo.fccIOProc;
    lpmmioinfo->pIOProc     = thunk->pfn16;
    lpmmioinfo->wErrorRet   = mmioinfo.wErrorRet;
    lpmmioinfo->hTask       = HTASK_16(mmioinfo.hTask);
    lpmmioinfo->cchBuffer   = mmioinfo.cchBuffer;
    lpmmioinfo->pchBuffer   = (void*)thunk->segbuffer;
    lpmmioinfo->pchNext     = (void*)(thunk->segbuffer + (mmioinfo.pchNext - mmioinfo.pchBuffer));
    lpmmioinfo->pchEndRead  = (void*)(thunk->segbuffer + (mmioinfo.pchEndRead - mmioinfo.pchBuffer));
    lpmmioinfo->pchEndWrite = (void*)(thunk->segbuffer + (mmioinfo.pchEndWrite - mmioinfo.pchBuffer));
    lpmmioinfo->lBufOffset  = mmioinfo.lBufOffset;
    lpmmioinfo->lDiskOffset = mmioinfo.lDiskOffset;
    lpmmioinfo->adwInfo[0]  = mmioinfo.adwInfo[0];
    lpmmioinfo->adwInfo[1]  = mmioinfo.adwInfo[1];
    lpmmioinfo->adwInfo[2]  = mmioinfo.adwInfo[2];
    lpmmioinfo->dwReserved1 = 0;
    lpmmioinfo->dwReserved2 = 0;
    lpmmioinfo->hmmio = HMMIO_16(mmioinfo.hmmio);
    LeaveCriticalSection(&mmio_cs);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioSetInfo  		[MMSYSTEM.1216]
 */
MMRESULT16 WINAPI mmioSetInfo16(HMMIO16 hmmio, const MMIOINFO16* lpmmioinfo, UINT16 uFlags)
{
    MMIOINFO            mmioinfo;
    MMRESULT            ret;

    TRACE("(0x%04x,%p,0x%08x)\n",hmmio,lpmmioinfo,uFlags);

    ret = mmioGetInfo(HMMIO_32(hmmio), &mmioinfo, 0);
    if (ret != MMSYSERR_NOERROR) return ret;

    /* check if seg and lin buffers are the same */
    if (mmioinfo.cchBuffer != lpmmioinfo->cchBuffer  ||
        mmioinfo.pchBuffer != MapSL((DWORD)lpmmioinfo->pchBuffer))
	return MMSYSERR_INVALPARAM;

    /* check pointers coherence */
    if (lpmmioinfo->pchNext < lpmmioinfo->pchBuffer ||
	lpmmioinfo->pchNext > lpmmioinfo->pchBuffer + lpmmioinfo->cchBuffer ||
	lpmmioinfo->pchEndRead < lpmmioinfo->pchBuffer ||
	lpmmioinfo->pchEndRead > lpmmioinfo->pchBuffer + lpmmioinfo->cchBuffer ||
	lpmmioinfo->pchEndWrite < lpmmioinfo->pchBuffer ||
	lpmmioinfo->pchEndWrite > lpmmioinfo->pchBuffer + lpmmioinfo->cchBuffer)
	return MMSYSERR_INVALPARAM;

    mmioinfo.pchNext     = mmioinfo.pchBuffer + (lpmmioinfo->pchNext     - lpmmioinfo->pchBuffer);
    mmioinfo.pchEndRead  = mmioinfo.pchBuffer + (lpmmioinfo->pchEndRead  - lpmmioinfo->pchBuffer);
    mmioinfo.pchEndWrite = mmioinfo.pchBuffer + (lpmmioinfo->pchEndWrite - lpmmioinfo->pchBuffer);

    return mmioSetInfo(HMMIO_32(hmmio), &mmioinfo, uFlags);
}

/**************************************************************************
 * 				mmioSetBuffer		[MMSYSTEM.1217]
 */
MMRESULT16 WINAPI mmioSetBuffer16(HMMIO16 hmmio, SEGPTR pchBuffer,
                                  LONG cchBuffer, UINT16 uFlags)
{
    MMRESULT    ret = mmioSetBuffer(HMMIO_32(hmmio), MapSL(pchBuffer),
                                    cchBuffer, uFlags);

    if (ret == MMSYSERR_NOERROR)
    {
        struct mmio_thunk* thunk;

        if ((thunk = MMIO_HasThunk(HMMIO_32(hmmio))) == NULL)
        {
            FIXME("really ?\n");
            return MMSYSERR_INVALHANDLE;
        }
        MMIO_SetSegmentedBuffer(thunk, pchBuffer, TRUE);
    }
    else
        UnMapLS(pchBuffer);
    return ret;
}

/**************************************************************************
 * 				mmioFlush      		[MMSYSTEM.1218]
 */
MMRESULT16 WINAPI mmioFlush16(HMMIO16 hmmio, UINT16 uFlags)
{
    return mmioFlush(HMMIO_32(hmmio), uFlags);
}

/***********************************************************************
 * 				mmioAdvance    		[MMSYSTEM.1219]
 */
MMRESULT16 WINAPI mmioAdvance16(HMMIO16 hmmio, MMIOINFO16* lpmmioinfo, UINT16 uFlags)
{
    MMIOINFO    mmioinfo;
    LRESULT     ret;

    /* WARNING: this heavily relies on mmioAdvance implementation (for choosing which
     * fields to init
     */
    if (lpmmioinfo)
    {
        mmioinfo.pchBuffer = MapSL((DWORD)lpmmioinfo->pchBuffer);
        mmioinfo.pchNext = MapSL((DWORD)lpmmioinfo->pchNext);
        mmioinfo.dwFlags = lpmmioinfo->dwFlags;
        mmioinfo.lBufOffset = lpmmioinfo->lBufOffset;
        ret = mmioAdvance(HMMIO_32(hmmio), &mmioinfo, uFlags);
    }
    else
        ret = mmioAdvance(HMMIO_32(hmmio), NULL, uFlags);

    if (ret != MMSYSERR_NOERROR) return ret;

    if (lpmmioinfo)
    {
        lpmmioinfo->dwFlags = mmioinfo.dwFlags;
        lpmmioinfo->pchNext     = (void*)(lpmmioinfo->pchBuffer + (mmioinfo.pchNext - mmioinfo.pchBuffer));
        lpmmioinfo->pchEndRead  = (void*)(lpmmioinfo->pchBuffer + (mmioinfo.pchEndRead - mmioinfo.pchBuffer));
        lpmmioinfo->pchEndWrite = (void*)(lpmmioinfo->pchBuffer + (mmioinfo.pchEndWrite - mmioinfo.pchBuffer));
        lpmmioinfo->lBufOffset  = mmioinfo.lBufOffset;
        lpmmioinfo->lDiskOffset = mmioinfo.lDiskOffset;
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioStringToFOURCC	[MMSYSTEM.1220]
 */
FOURCC WINAPI mmioStringToFOURCC16(LPCSTR sz, UINT16 uFlags)
{
    return mmioStringToFOURCCA(sz, uFlags);
}

/**************************************************************************
 *              mmioInstallIOProc    [MMSYSTEM.1221]
 */
LPMMIOPROC16 WINAPI mmioInstallIOProc16(FOURCC fccIOProc, LPMMIOPROC16 pIOProc,
                                        DWORD dwFlags)
{
    struct mmio_thunk*  thunk = NULL;
    LPMMIOPROC pIOProc32;

    EnterCriticalSection(&mmio_cs);

    switch (dwFlags & (MMIO_INSTALLPROC|MMIO_REMOVEPROC|MMIO_FINDPROC)) {
    case MMIO_INSTALLPROC:
        if (!(thunk = MMIO_AddThunk(pIOProc, NULL)))
        {
            LeaveCriticalSection(&mmio_cs);
            return NULL;
        }
        if (!mmioInstallIOProcA(fccIOProc, (LPMMIOPROC)thunk, dwFlags))
        {
            thunk->pfn16 = NULL;
            pIOProc = NULL;
        }
        break;
    case MMIO_REMOVEPROC:
        if (MMIO_Thunks)
        {
            for (thunk = MMIO_Thunks; thunk < &MMIO_Thunks[MMIO_MAX_THUNKS]; thunk++)
            {
                if (thunk->pfn16 == pIOProc && thunk->segbuffer == 0)
                {
                    if (mmioInstallIOProcA(fccIOProc, (LPMMIOPROC)thunk, dwFlags))
                        thunk->pfn16 = NULL;
                    else
                        pIOProc = NULL;
                    break;
                }
            }
        }
        if (!thunk) pIOProc = NULL;
        break;
    case MMIO_FINDPROC:
        if ((pIOProc32 = mmioInstallIOProcA(fccIOProc, NULL, dwFlags)) && MMIO_Thunks)
        {
            for (thunk = MMIO_Thunks; thunk < &MMIO_Thunks[MMIO_MAX_THUNKS]; thunk++)
            {
                if ((LPMMIOPROC)thunk == pIOProc32)
                {
                    pIOProc = thunk->pfn16;
                    break;
                }
            }
        }
        break;
    default:
        FIXME("Unsupported flags %08lx\n", dwFlags);
        pIOProc = NULL;
    }
    LeaveCriticalSection(&mmio_cs);

    return pIOProc;
}

/**************************************************************************
 * 				mmioSendMessage	[MMSYSTEM.1222]
 */
LRESULT WINAPI mmioSendMessage16(HMMIO16 hmmio, UINT16 uMessage,
				 LPARAM lParam1, LPARAM lParam2)
{
    struct mmio_thunk*  thunk;

    if ((thunk = MMIO_HasThunk(HMMIO_32(hmmio))))
    {
        MMIOINFO        mmioinfo;
        if (mmioGetInfo(HMMIO_32(hmmio), &mmioinfo, 0) == MMSYSERR_NOERROR)
        {
            return MMIO_Callback3216((SEGPTR)thunk->pfn16, &mmioinfo, uMessage, lParam1, lParam2);
        }
        return MMSYSERR_INVALHANDLE;
    }
    else
    {
        /* FIXME: we need to map lParam1 and lParam2 to 32bit entities */
        return mmioSendMessage(HMMIO_32(hmmio), uMessage, lParam1, lParam2);
    }
}

/**************************************************************************
 * 				mmioDescend	       	[MMSYSTEM.1223]
 */
MMRESULT16 WINAPI mmioDescend16(HMMIO16 hmmio, LPMMCKINFO lpck,
                                const MMCKINFO* lpckParent, UINT16 uFlags)
{
    return mmioDescend(HMMIO_32(hmmio), lpck, lpckParent, uFlags);
}

/**************************************************************************
 * 				mmioAscend     		[MMSYSTEM.1224]
 */
MMRESULT16 WINAPI mmioAscend16(HMMIO16 hmmio, MMCKINFO* lpck, UINT16 uFlags)
{
    return mmioAscend(HMMIO_32(hmmio),lpck,uFlags);
}

/**************************************************************************
 * 				mmioCreateChunk		[MMSYSTEM.1225]
 */
MMRESULT16 WINAPI mmioCreateChunk16(HMMIO16 hmmio, MMCKINFO* lpck, UINT16 uFlags)
{
    return mmioCreateChunk(HMMIO_32(hmmio), lpck, uFlags);
}

/**************************************************************************
 * 				mmioRename     		[MMSYSTEM.1226]
 */
MMRESULT16 WINAPI mmioRename16(LPCSTR szFileName, LPCSTR szNewFileName,
                               MMIOINFO16* lpmmioinfo, DWORD dwRenameFlags)
{
    BOOL        inst = FALSE;
    MMRESULT    ret;
    MMIOINFO    mmioinfo;

    if (lpmmioinfo != NULL && lpmmioinfo->pIOProc != NULL &&
        lpmmioinfo->fccIOProc == 0) {
        FIXME("Can't handle this case yet\n");
        return MMSYSERR_ERROR;
    }

    /* this is a bit hacky, but it'll work if we get a fourCC code or nothing.
     * but a non installed ioproc without a fourcc won't do
     */
    if (lpmmioinfo && lpmmioinfo->fccIOProc && lpmmioinfo->pIOProc) {
        mmioInstallIOProc16(lpmmioinfo->fccIOProc, lpmmioinfo->pIOProc,
                           MMIO_INSTALLPROC);
        inst = TRUE;
    }
    memset(&mmioinfo, 0, sizeof(mmioinfo));
    if (lpmmioinfo)
        mmioinfo.fccIOProc = lpmmioinfo->fccIOProc;
    ret = mmioRenameA(szFileName, szNewFileName, &mmioinfo, dwRenameFlags);
    if (inst) {
        mmioInstallIOProc16(lpmmioinfo->fccIOProc, NULL, MMIO_REMOVEPROC);
    }
    return ret;
}
