/*
 * MMSYSTEM mmio* functions
 *
 * Copyright 1993      Martin Ayotte
 *           1998-2003 Eric Pouech
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winternl.h"
#include "wownt32.h"
#include "winnls.h"

#include "wine/winuser16.h"
#include "winemm.h"
#include "winemm16.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmsys);

/* ###################################################
 * #                     MMIO                        #
 * ###################################################
 */

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
            TRACE("Not a mappable message (%d)\n", wMsg);
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
            TRACE("Not a mappable message (%d)\n", wMsg);
    }
    return MMSYSERR_NOERROR;
}

/******************************************************************
 *		MMIO_Callback16
 *
 *
 */
LRESULT MMIO_Callback16(SEGPTR cb16, LPMMIOINFO lpmmioinfo, UINT uMessage,
                        LPARAM lParam1, LPARAM lParam2)
{
    DWORD 		result;
    MMIOINFO16          mmioInfo16;
    SEGPTR		segmmioInfo16;
    LPARAM		lp1 = lParam1, lp2 = lParam2;
    WORD args[7];

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
 *             MMIO_ResetSegmentedData
 *
 */
static LRESULT     MMIO_SetSegmentedBuffer(HMMIO hmmio, SEGPTR ptr, BOOL release)
{
    LPWINE_MMIO		wm;

    if ((wm = MMIO_Get(hmmio)) == NULL)
	return MMSYSERR_INVALHANDLE;
    if (release) UnMapLS(wm->segBuffer16);
    wm->segBuffer16 = ptr;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				mmioOpen       		[MMSYSTEM.1210]
 */
HMMIO16 WINAPI mmioOpen16(LPSTR szFileName, MMIOINFO16* lpmmioinfo16,
			  DWORD dwOpenFlags)
{
    HMMIO 	ret;

    if (lpmmioinfo16) {
	MMIOINFO	mmioinfo;

	memset(&mmioinfo, 0, sizeof(mmioinfo));

	mmioinfo.dwFlags     = lpmmioinfo16->dwFlags;
	mmioinfo.fccIOProc   = lpmmioinfo16->fccIOProc;
	mmioinfo.pIOProc     = (LPMMIOPROC)lpmmioinfo16->pIOProc;
	mmioinfo.cchBuffer   = lpmmioinfo16->cchBuffer;
	mmioinfo.pchBuffer   = MapSL((DWORD)lpmmioinfo16->pchBuffer);
        mmioinfo.adwInfo[0]  = lpmmioinfo16->adwInfo[0];
        /* if we don't have a file name, it's likely a passed open file descriptor */
        if (!szFileName)
            mmioinfo.adwInfo[0] = (DWORD)DosFileHandleToWin32Handle(mmioinfo.adwInfo[0]);
	mmioinfo.adwInfo[1]  = lpmmioinfo16->adwInfo[1];
	mmioinfo.adwInfo[2]  = lpmmioinfo16->adwInfo[2];

	ret = MMIO_Open(szFileName, &mmioinfo, dwOpenFlags, MMIO_PROC_16);
        MMIO_SetSegmentedBuffer(mmioinfo.hmmio, (SEGPTR)lpmmioinfo16->pchBuffer, FALSE);

	lpmmioinfo16->wErrorRet = mmioinfo.wErrorRet;
        lpmmioinfo16->hmmio     = HMMIO_16(mmioinfo.hmmio);
    } else {
	ret = MMIO_Open(szFileName, NULL, dwOpenFlags, MMIO_PROC_32A);
    }
    return HMMIO_16(ret);
}

/**************************************************************************
 * 				mmioClose      		[MMSYSTEM.1211]
 */
MMRESULT16 WINAPI mmioClose16(HMMIO16 hmmio, UINT16 uFlags)
{
    MMIO_SetSegmentedBuffer(HMMIO_32(hmmio), 0, TRUE);
    return mmioClose(HMMIO_32(hmmio), uFlags);
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
    LPWINE_MMIO		wm;

    TRACE("(0x%04x,%p,0x%08x)\n", hmmio, lpmmioinfo, uFlags);

    if ((wm = MMIO_Get(HMMIO_32(hmmio))) == NULL)
	return MMSYSERR_INVALHANDLE;

    ret = mmioGetInfo(HMMIO_32(hmmio), &mmioinfo, uFlags);
    if (ret != MMSYSERR_NOERROR) return ret;

    lpmmioinfo->dwFlags     = mmioinfo.dwFlags;
    lpmmioinfo->fccIOProc   = mmioinfo.fccIOProc;
    lpmmioinfo->pIOProc     = (wm->ioProc->type == MMIO_PROC_16) ?
        (LPMMIOPROC16)wm->ioProc->pIOProc : NULL;
    lpmmioinfo->wErrorRet   = mmioinfo.wErrorRet;
    lpmmioinfo->hTask       = HTASK_16(mmioinfo.hTask);
    lpmmioinfo->cchBuffer   = mmioinfo.cchBuffer;
    lpmmioinfo->pchBuffer   = (void*)wm->segBuffer16;
    lpmmioinfo->pchNext     = (void*)(wm->segBuffer16 + (mmioinfo.pchNext - mmioinfo.pchBuffer));
    lpmmioinfo->pchEndRead  = (void*)(wm->segBuffer16 + (mmioinfo.pchEndRead - mmioinfo.pchBuffer));
    lpmmioinfo->pchEndWrite = (void*)(wm->segBuffer16 + (mmioinfo.pchEndWrite - mmioinfo.pchBuffer));
    lpmmioinfo->lBufOffset  = mmioinfo.lBufOffset;
    lpmmioinfo->lDiskOffset = mmioinfo.lDiskOffset;
    lpmmioinfo->adwInfo[0]  = mmioinfo.adwInfo[0];
    lpmmioinfo->adwInfo[1]  = mmioinfo.adwInfo[1];
    lpmmioinfo->adwInfo[2]  = mmioinfo.adwInfo[2];
    lpmmioinfo->dwReserved1 = 0;
    lpmmioinfo->dwReserved2 = 0;
    lpmmioinfo->hmmio = HMMIO_16(mmioinfo.hmmio);

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
MMRESULT16 WINAPI mmioSetBuffer16(HMMIO16 hmmio, LPSTR pchBuffer,
                                  LONG cchBuffer, UINT16 uFlags)
{
    MMRESULT    ret = mmioSetBuffer(HMMIO_32(hmmio), MapSL((DWORD)pchBuffer),
                                    cchBuffer, uFlags);

    if (ret == MMSYSERR_NOERROR)
        MMIO_SetSegmentedBuffer(HMMIO_32(hmmio), (DWORD)pchBuffer, TRUE);
    else
        UnMapLS((DWORD)pchBuffer);
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
    return (LPMMIOPROC16)MMIO_InstallIOProc(fccIOProc, (LPMMIOPROC)pIOProc,
                                            dwFlags, MMIO_PROC_16);
}

/**************************************************************************
 * 				mmioSendMessage	[MMSYSTEM.1222]
 */
LRESULT WINAPI mmioSendMessage16(HMMIO16 hmmio, UINT16 uMessage,
				 LPARAM lParam1, LPARAM lParam2)
{
    return MMIO_SendMessage(HMMIO_32(hmmio), uMessage,
                            lParam1, lParam2, MMIO_PROC_16);
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
        MMIO_InstallIOProc(lpmmioinfo->fccIOProc, (LPMMIOPROC)lpmmioinfo->pIOProc,
                           MMIO_INSTALLPROC, MMIO_PROC_16);
        inst = TRUE;
    }
    memset(&mmioinfo, 0, sizeof(mmioinfo));
    mmioinfo.fccIOProc = lpmmioinfo->fccIOProc;
    ret = mmioRenameA(szFileName, szNewFileName, &mmioinfo, dwRenameFlags);
    if (inst) {
        MMIO_InstallIOProc(lpmmioinfo->fccIOProc, NULL,
                           MMIO_REMOVEPROC, MMIO_PROC_16);
    }
    return ret;
}
