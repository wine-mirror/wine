/*
 * MMIO functions
 *
 * Copyright 1998 Andrew Taylor
 * Copyright 1998 Ove Kåven
 *
 * Buffering is not guaranteed to work properly yet, but if it does...
 * Buffering should almost give us memory files for free.
 */


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "windows.h"
#include "win.h"
#include "heap.h"
#include "user.h"
#include "file.h"
#include "mmsystem.h"
#include "debug.h"
#include "xmalloc.h"

/**************************************************************************
 *               mmioDosIOProc           [internal]
 */
static LRESULT mmioDosIOProc(LPMMIOINFO16 lpmmioinfo, UINT16 uMessage, LPARAM lParam1, LPARAM lParam2) {
	TRACE(mmio, "(%p, %X, %ld, %ld);\n", lpmmioinfo, uMessage, lParam1, lParam2);

	switch (uMessage) {

		case MMIOM_OPEN: {
			/* Parameters:
			 * lParam1 = szFileName parameter from mmioOpen
			 * lParam2 = unused
			 * Returns: zero on success, error code on error
			 * NOTE: lDiskOffset automatically set to zero
			 */

			OFSTRUCT ofs;
			LPSTR szFileName = (LPSTR) lParam1;

			if (lpmmioinfo->dwFlags & MMIO_GETTEMP) {
				FIXME(mmio, "MMIO_GETTEMP not implemented\n");
				return MMIOERR_CANNOTOPEN;
			}

			/* if filename NULL, assume open file handle in adwInfo[0] */
			if (!szFileName)
				return 0;

			lpmmioinfo->adwInfo[0] =
				(DWORD) OpenFile32(szFileName, &ofs, lpmmioinfo->dwFlags);
			if (lpmmioinfo->adwInfo[0] == -1)
				return MMIOERR_CANNOTOPEN;

			return 0;
		}

		case MMIOM_CLOSE: {
			/* Parameters:
			 * lParam1 = wFlags parameter from mmioClose
			 * lParam2 = unused
			 * Returns: zero on success, error code on error
			 */

			UINT16 uFlags = (UINT16) lParam1;

			if (uFlags & MMIO_FHOPEN)
				return 0;

			_lclose32((HFILE32)lpmmioinfo->adwInfo[0]);
			return 0;

		}

		case MMIOM_READ: {
			/* Parameters:
			 * lParam1 = huge pointer to read buffer
			 * lParam2 = number of bytes to read
			 * Returns: number of bytes read, 0 for EOF, -1 for error (error code
			 *	   in wErrorRet)
			 * NOTE: lDiskOffset should be updated
			 */

			HPSTR pch = (HPSTR) lParam1;
			LONG cch = (LONG) lParam2;
			LONG count;

			count = _lread32((HFILE32)lpmmioinfo->adwInfo[0], pch, cch);
			if (count != -1)
				lpmmioinfo->lDiskOffset += count;

			return count;
		}

		case MMIOM_WRITE:
		case MMIOM_WRITEFLUSH: { 
			/* no internal buffering, so WRITEFLUSH handled same as WRITE */

			/* Parameters:
			 * lParam1 = huge pointer to write buffer
			 * lParam2 = number of bytes to write
			 * Returns: number of bytes written, -1 for error (error code in
			 *		wErrorRet)
			 * NOTE: lDiskOffset should be updated
			 */

			HPSTR pch = (HPSTR) lParam1;
			LONG cch = (LONG) lParam2;
			LONG count;

			count = _hwrite16((HFILE32)lpmmioinfo->adwInfo[0], pch, cch);
			if (count != -1)
				lpmmioinfo->lDiskOffset += count;

			return count;
		}

		case MMIOM_SEEK: {
            /* Parameters:
             * lParam1 = new position
             * lParam2 = from whence to seek (SEEK_SET, SEEK_CUR, SEEK_END)
             * Returns: new file postion, -1 on error
             * NOTE: lDiskOffset should be updated
             */

            LONG Offset = (LONG) lParam1; 
            LONG Whence = (LONG) lParam2; 
            LONG pos;

            pos = _llseek32((HFILE32)lpmmioinfo->adwInfo[0], Offset, Whence);
            if (pos != -1)
                lpmmioinfo->lDiskOffset = pos;

            return pos;
		}
		  
		case MMIOM_RENAME: {
            /* Parameters:
             * lParam1 = old name
             * lParam2 = new name
             * Returns: zero on success, non-zero on failure
             */

            FIXME(mmio, "MMIOM_RENAME unimplemented\n");
            return MMIOERR_FILENOTFOUND;
		}

		default:
			FIXME(mmio, "unexpected message %u\n", uMessage);
			return 0;
	}
	
	return 0;
}

/**************************************************************************
*               mmioMemIOProc           [internal]
*/
static LRESULT mmioMemIOProc(LPMMIOINFO16 lpmmioinfo, UINT16 uMessage, LPARAM lParam1, LPARAM lParam2) {
	FIXME(mmio,"(%p,0x%04x,0x%08lx,0x%08lx), stub!\n",lpmmioinfo,uMessage,lParam1,lParam2);
	return 0;
}

/**************************************************************************
 * 				mmioOpenW       		[WINMM.123]
 */
HMMIO32 WINAPI mmioOpen32W(LPWSTR szFileName, MMIOINFO32 * lpmmioinfo,
                          DWORD dwOpenFlags)
{
	LPSTR	szFn = HEAP_strdupWtoA(GetProcessHeap(),0,szFileName);
	HMMIO32 ret = mmioOpen16(szFn,(LPMMIOINFO16)lpmmioinfo,dwOpenFlags);

	HeapFree(GetProcessHeap(),0,szFn);
	return ret;
}

/**************************************************************************
 * 				mmioOpenA       		[WINMM.122]
 */
HMMIO32 WINAPI mmioOpen32A(LPSTR szFileName, MMIOINFO32 * lpmmioinfo,
                          DWORD dwOpenFlags)
{
	return mmioOpen16(szFileName,(LPMMIOINFO16)lpmmioinfo,dwOpenFlags);
}

/**************************************************************************
 * 				mmioOpen       		[MMSYSTEM.1210]
 */
HMMIO16 WINAPI mmioOpen16(LPSTR szFileName, MMIOINFO16 * lpmmioinfo,
                          DWORD dwOpenFlags)
{
	LPMMIOINFO16 lpmminfo;
	HMMIO16 hmmio;
	UINT16 result;

	TRACE(mmio, "('%s', %p, %08lX);\n", szFileName, lpmmioinfo, dwOpenFlags);

	hmmio = GlobalAlloc16(GHND, sizeof(MMIOINFO16));
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return 0;
	memset(lpmminfo, 0, sizeof(MMIOINFO16));

	/* assume DOS file if not otherwise specified */
	if (!lpmmioinfo ||
		(lpmmioinfo->fccIOProc == 0 && lpmmioinfo->pIOProc == NULL)) {

		lpmminfo->fccIOProc = mmioFOURCC('D', 'O', 'S', ' ');
		lpmminfo->pIOProc = (LPMMIOPROC16) mmioDosIOProc;
	}
	/* if just the four character code is present, look up IO proc */
	else if (lpmmioinfo->pIOProc == NULL) {

		lpmminfo->fccIOProc = lpmmioinfo->fccIOProc;
		lpmminfo->pIOProc = mmioInstallIOProc16(lpmmioinfo->fccIOProc, NULL, MMIO_FINDPROC);

	} 
	/* if IO proc specified, use it and specified four character code */
	else {

		lpmminfo->fccIOProc = lpmmioinfo->fccIOProc;
		lpmminfo->pIOProc = lpmmioinfo->pIOProc;
	}

	if (dwOpenFlags & MMIO_ALLOCBUF) {
		if ((result = mmioSetBuffer(hmmio, NULL, MMIO_DEFAULTBUFFER, 0))) {
			if (lpmmioinfo)
				lpmmioinfo->wErrorRet = result;
			return 0;
		}
	}

	lpmminfo->dwFlags = dwOpenFlags;
	lpmminfo->hmmio = hmmio;

	/* call IO proc to actually open file */
	result = (UINT16) mmioSendMessage(hmmio, MMIOM_OPEN, (LPARAM) szFileName, (LPARAM) 0);

	GlobalUnlock16(hmmio);

	if (result != 0) {
		GlobalFree16(hmmio);
		return 0;
	}

	return hmmio;
}

    
/**************************************************************************
 * 				mmioClose      		[WINMM.114]
 */
MMRESULT32 WINAPI mmioClose32(HMMIO32 hmmio, UINT32 uFlags)
{
	LPMMIOINFO16 lpmminfo;
	MMRESULT32 result;

	TRACE(mmio, "(%04X, %04X);\n", hmmio, uFlags);

	lpmminfo = (LPMMIOINFO16) GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return 0;

	/* flush the file - if error reported, ignore */
	if (mmioFlush32(hmmio, MMIO_EMPTYBUF) != 0)
		lpmminfo->dwFlags &= ~MMIO_DIRTY;

	result = mmioSendMessage(hmmio,MMIOM_CLOSE,(LPARAM)uFlags,(LPARAM)0);

	mmioSetBuffer(hmmio, NULL, 0, 0);

	GlobalUnlock16(hmmio);
	GlobalFree16(hmmio);

	return result;
}


/**************************************************************************
 * 				mmioClose      		[MMSYSTEM.1211]
 */
MMRESULT16 WINAPI mmioClose16(HMMIO16 hmmio, UINT16 uFlags)
{
	return mmioClose32(hmmio,uFlags);
}



/**************************************************************************
 * 				mmioRead	       	[WINM.124]
 */
LONG WINAPI mmioRead32(HMMIO32 hmmio, HPSTR pch, LONG cch)
{
	LONG count;
	LPMMIOINFO16 lpmminfo;

	TRACE(mmio, "(%04X, %p, %ld);\n", hmmio, pch, cch);

	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return -1;

	if (lpmminfo->pchNext != lpmminfo->pchEndRead) {
		count = lpmminfo->pchEndRead - lpmminfo->pchNext;
		if (count > cch) count = cch;
		memcpy(pch, lpmminfo->pchNext, count);
		lpmminfo->pchNext += count;
		pch += count;
		cch -= count;
	} else
		count = 0;

	if (cch) {
		if (lpmminfo->cchBuffer) {
			mmioFlush32(hmmio, MMIO_EMPTYBUF);

			while (cch) {
				LONG size = mmioSendMessage(hmmio, MMIOM_READ,
						(LPARAM) lpmminfo->pchBuffer,
						(LPARAM) lpmminfo->cchBuffer);
				lpmminfo->pchNext = lpmminfo->pchBuffer;
				lpmminfo->pchEndRead = lpmminfo->pchBuffer + size;
				if (size > cch) size = cch;
				memcpy(pch, lpmminfo->pchNext, size);
				lpmminfo->pchNext += size;
				pch += size;
				cch -= size;
				count += size;
			}
		} else {
			count += mmioSendMessage(hmmio, MMIOM_READ, (LPARAM) pch, (LPARAM) cch);
		}
	}

	GlobalUnlock16(hmmio);
	TRACE(mmio, "count=%ld\n", count);
	return count;
}

/**************************************************************************
 * 				mmioRead	       	[MMSYSTEM.1212]
 */
LONG WINAPI mmioRead16(HMMIO16 hmmio, HPSTR pch, LONG cch)
{
	return mmioRead32(hmmio,pch,cch);
}

/**************************************************************************
 * 				mmioWrite      		[WINMM.133]
 */
LONG WINAPI mmioWrite32(HMMIO32 hmmio, HPCSTR pch, LONG cch)
{
	LONG count;
	LPMMIOINFO16 lpmminfo;

	TRACE(mmio, "(%04X, %p, %ld);\n", hmmio, pch, cch);

	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return -1;

	if (lpmminfo->cchBuffer) {
		count = 0;
		while (cch) {
			if (lpmminfo->pchNext != lpmminfo->pchEndWrite) {
				count = lpmminfo->pchEndWrite - lpmminfo->pchNext;
				if (count > cch) count = cch;
				memcpy(lpmminfo->pchNext, pch, count);
				lpmminfo->pchNext += count;
				pch += count;
				cch -= count;
				lpmminfo->dwFlags |= MMIO_DIRTY;
			}

			if (lpmminfo->pchNext == lpmminfo->pchEndWrite
				&& mmioFlush32(hmmio, MMIO_EMPTYBUF)) break;
		}
	} else {
		count = mmioSendMessage(hmmio, MMIOM_WRITE, (LPARAM) pch, (LPARAM) cch);
	}

	GlobalUnlock16(hmmio);
	TRACE(mmio, "count=%ld\n", count);
	return count;
}

/**************************************************************************
 * 				mmioWrite      		[MMSYSTEM.1213]
 */
LONG WINAPI mmioWrite16(HMMIO16 hmmio, HPCSTR pch, LONG cch)
{
	return mmioWrite32(hmmio,pch,cch);
}

/**************************************************************************
 * 				mmioSeek       		[MMSYSTEM.1214]
 */
LONG WINAPI mmioSeek32(HMMIO32 hmmio, LONG lOffset, INT32 iOrigin)
{
	int offset;
	LPMMIOINFO16 lpmminfo;

	TRACE(mmio, "(%04X, %08lX, %d);\n", hmmio, lOffset, iOrigin);

	if (mmioFlush32(hmmio, MMIO_EMPTYBUF))
		return -1;

	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return 0;

	offset = mmioSendMessage(hmmio, MMIOM_SEEK, (LPARAM) lOffset, (LPARAM) iOrigin);

	GlobalUnlock16(hmmio);
	return offset;
}

/**************************************************************************
 * 				mmioSeek       		[MMSYSTEM.1214]
 */
LONG WINAPI mmioSeek16(HMMIO16 hmmio, LONG lOffset, INT16 iOrigin)
{
	return mmioSeek32(hmmio,lOffset,iOrigin);
}

/**************************************************************************
 * 				mmioGetInfo	       	[MMSYSTEM.1215]
 */
UINT16 WINAPI mmioGetInfo16(HMMIO16 hmmio, MMIOINFO16 * lpmmioinfo, UINT16 uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE(mmio, "mmioGetInfo\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	memcpy(lpmmioinfo, lpmminfo, sizeof(MMIOINFO16));
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
 * 				mmioGetInfo	       	[WINMM.118]
 */
UINT32 WINAPI mmioGetInfo32(HMMIO32 hmmio, MMIOINFO32*lpmmioinfo, UINT32 uFlags)
{
	MMIOINFO16	mmioinfo;
	LPMMIOINFO16	lpmminfo=&mmioinfo;
	UINT16		ret;

	TRACE(mmio, "(0x%04x,%p,0x%08x)\n",hmmio,lpmmioinfo,uFlags);
	ret = mmioGetInfo16(hmmio,&mmioinfo,uFlags);
	if (!ret)
		return 0;
	lpmmioinfo->dwFlags	= lpmminfo->dwFlags;
	lpmmioinfo->fccIOProc	= lpmminfo->fccIOProc;
	lpmmioinfo->pIOProc	= (LPMMIOPROC32)lpmminfo->pIOProc;
	lpmmioinfo->wErrorRet	= lpmminfo->wErrorRet;
	lpmmioinfo->htask	= lpmminfo->htask;
	lpmmioinfo->cchBuffer	= lpmminfo->cchBuffer;
	lpmmioinfo->pchBuffer	= lpmminfo->pchBuffer;
	lpmmioinfo->pchNext	= lpmminfo->pchNext;
	lpmmioinfo->pchEndRead	= lpmminfo->pchEndRead;
	lpmmioinfo->pchEndWrite	= lpmminfo->pchEndWrite;
	lpmmioinfo->lBufOffset	= lpmminfo->lBufOffset;
	lpmmioinfo->lDiskOffset	= lpmminfo->lDiskOffset;
	memcpy(lpmmioinfo->adwInfo,lpmminfo->adwInfo,sizeof(lpmminfo->adwInfo));
	lpmmioinfo->dwReserved1	= lpmminfo->dwReserved1;
	lpmmioinfo->dwReserved2	= lpmminfo->dwReserved2;
	lpmmioinfo->hmmio	= lpmminfo->hmmio;
	return 0;
}

/**************************************************************************
* 				mmioSetInfo    		[MMSYSTEM.1216]
*/
UINT16 WINAPI mmioSetInfo(HMMIO16 hmmio, const MMIOINFO16 * lpmmioinfo, UINT16 uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE(mmio, "mmioSetInfo\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	lpmminfo->pchNext	= lpmmioinfo->pchNext;
	lpmminfo->pchEndRead	= lpmmioinfo->pchEndRead;
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
* 				mmioSetBuffer		[MMSYSTEM.1217]
*/
UINT16 WINAPI mmioSetBuffer(HMMIO16 hmmio, LPSTR pchBuffer, 
                            LONG cchBuffer, UINT16 uFlags)
{
	LPMMIOINFO16	lpmminfo;

	if (mmioFlush32(hmmio, MMIO_EMPTYBUF) != 0)
		return MMIOERR_CANNOTWRITE;

	TRACE(mmio, "(hmmio=%04x, pchBuf=%p, cchBuf=%ld, uFlags=%#08x)\n",
	      hmmio, pchBuffer, cchBuffer, uFlags);

	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	if ((!cchBuffer || pchBuffer) && lpmminfo->dwFlags&MMIO_ALLOCBUF) {
		GlobalUnlock16(lpmminfo->dwReserved1);
		GlobalFree16(lpmminfo->dwReserved1);
		lpmminfo->dwFlags &= ~MMIO_ALLOCBUF;
	}
	if (pchBuffer) {
		lpmminfo->pchBuffer = pchBuffer;
	} else
	if (lpmminfo->dwFlags&MMIO_ALLOCBUF) {
		HGLOBAL16 hNewBuf;
		GlobalUnlock16(lpmminfo->dwReserved1);
		hNewBuf = GlobalReAlloc16(lpmminfo->dwReserved1, cchBuffer, 0);
		if (!hNewBuf) {
			/* FIXME: this assumes the memory block didn't move */
			GlobalLock16(lpmminfo->dwReserved1);
			GlobalUnlock16(hmmio);
			return MMIOERR_OUTOFMEMORY;
		}
		lpmminfo->dwReserved1 = hNewBuf;
		lpmminfo->pchBuffer = GlobalLock16(hNewBuf);
	} else
	if (cchBuffer) {
		HGLOBAL16 hNewBuf = GlobalAlloc16(GMEM_MOVEABLE, cchBuffer);
		if (!hNewBuf) {
			GlobalUnlock16(hmmio);
			return MMIOERR_OUTOFMEMORY;
		}
		lpmminfo->dwReserved1 = hNewBuf;
		lpmminfo->pchBuffer = GlobalLock16(hNewBuf);
		lpmminfo->dwFlags |= MMIO_ALLOCBUF;
	} else
		lpmminfo->pchBuffer = NULL;
	lpmminfo->cchBuffer = cchBuffer;
	lpmminfo->pchNext = lpmminfo->pchBuffer;
	lpmminfo->pchEndRead = lpmminfo->pchBuffer;
	lpmminfo->pchEndWrite = lpmminfo->pchBuffer + cchBuffer;
	lpmminfo->lBufOffset = 0;

	GlobalUnlock16(hmmio);
	return (UINT16) 0;
}

/**************************************************************************
 * 				mmioFlush      		[WINMM.117]
 */
UINT32 WINAPI mmioFlush32(HMMIO32 hmmio, UINT32 uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE(mmio, "(%04X, %04X)\n", hmmio, uFlags);
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;

	/* not quite sure what to do here, but I'll guess */
	if (lpmminfo->dwFlags&MMIO_DIRTY) {
		if (lpmminfo->pchNext != (lpmminfo->pchBuffer + lpmminfo->lBufOffset)) {
			if (lpmminfo->dwFlags&MMIO_READWRITE)
				mmioSendMessage(hmmio, MMIOM_SEEK,
					(LPARAM) (lpmminfo->pchBuffer - lpmminfo->pchEndRead) + lpmminfo->lBufOffset,
					(LPARAM) SEEK_CUR );
			mmioSendMessage(hmmio, MMIOM_WRITE,
				(LPARAM) lpmminfo->pchBuffer + lpmminfo->lBufOffset,
				(LPARAM) (lpmminfo->pchNext - lpmminfo->pchBuffer) - lpmminfo->lBufOffset );
		}
		lpmminfo->lBufOffset = lpmminfo->pchNext - lpmminfo->pchBuffer;
		lpmminfo->dwFlags &= ~MMIO_DIRTY;
	}
	if (uFlags & MMIO_EMPTYBUF) {
		lpmminfo->pchNext = lpmminfo->pchBuffer;
		lpmminfo->pchEndRead = lpmminfo->pchBuffer;
		lpmminfo->lBufOffset = 0;
	}

	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
 * 				mmioFlush      		[MMSYSTEM.1218]
 */
UINT16 WINAPI mmioFlush16(HMMIO16 hmmio, UINT16 uFlags)
{
	return mmioFlush32(hmmio,uFlags);
}

/**************************************************************************
 * 				mmioAdvance    		[MMSYSTEM.1219]
 */
UINT32 WINAPI mmioAdvance32(HMMIO32 hmmio,MMIOINFO32*lpmmioinfo,UINT32 uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE(mmio, "mmioAdvance\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	if (uFlags == MMIO_READ)
	        lpmmioinfo->pchEndRead = lpmmioinfo->pchBuffer +
	            mmioSendMessage(hmmio, MMIOM_READ,
	                (LPARAM) lpmmioinfo->pchBuffer,
	                (LPARAM) lpmmioinfo->cchBuffer);
	if (uFlags == MMIO_WRITE)
	            mmioSendMessage(hmmio, MMIOM_WRITE,
	                (LPARAM) lpmmioinfo->pchBuffer,
	                (LPARAM) lpmmioinfo->cchBuffer);
	lpmmioinfo->pchNext = lpmmioinfo->pchBuffer;
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
 * 				mmioAdvance    		[MMSYSTEM.1219]
 */
UINT16 WINAPI mmioAdvance16(HMMIO16 hmmio,MMIOINFO16*lpmmioinfo,UINT16 uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE(mmio, "mmioAdvance\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	if (uFlags == MMIO_READ)
	        lpmmioinfo->pchEndRead = lpmmioinfo->pchBuffer +
	            mmioSendMessage(hmmio, MMIOM_READ,
	                (LPARAM) lpmmioinfo->pchBuffer,
	                (LPARAM) lpmmioinfo->cchBuffer);
	if (uFlags == MMIO_WRITE)
	            mmioSendMessage(hmmio, MMIOM_WRITE,
	                (LPARAM) lpmmioinfo->pchBuffer,
	                (LPARAM) lpmmioinfo->cchBuffer);
	lpmmioinfo->pchNext = lpmmioinfo->pchBuffer;
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
 * 				mmioStringToFOURCCA	[WINMM.131]
 */
FOURCC WINAPI mmioStringToFOURCC32A(LPCSTR sz, UINT32 uFlags)
{
	return mmioStringToFOURCC16(sz,uFlags);
}

/**************************************************************************
 * 				mmioStringToFOURCCW	[WINMM.132]
 */
FOURCC WINAPI mmioStringToFOURCC32W(LPCWSTR sz, UINT32 uFlags)
{
	LPSTR	szA = HEAP_strdupWtoA(GetProcessHeap(),0,sz);
	FOURCC	ret = mmioStringToFOURCC32A(szA,uFlags);

	HeapFree(GetProcessHeap(),0,szA);
	return ret;
}

/**************************************************************************
 * 				mmioStringToFOURCC	[MMSYSTEM.1220]
 */
FOURCC WINAPI mmioStringToFOURCC16(LPCSTR sz, UINT16 uFlags)
{
	return mmioFOURCC(sz[0],sz[1],sz[2],sz[3]);
}

/**************************************************************************
* 				mmioInstallIOProc16	[MMSYSTEM.1221]
*/
LPMMIOPROC16 WINAPI mmioInstallIOProc16(FOURCC fccIOProc, 
                                        LPMMIOPROC16 pIOProc, DWORD dwFlags)
{
	TRACE(mmio, "(%ld, %p, %08lX)\n",
				 fccIOProc, pIOProc, dwFlags);

	if (dwFlags & MMIO_GLOBALPROC) {
		FIXME(mmio, " global procedures not implemented\n");
	}

	/* just handle the known procedures for now */
	switch(dwFlags & (MMIO_INSTALLPROC|MMIO_REMOVEPROC|MMIO_FINDPROC)) {
		case MMIO_INSTALLPROC:
			return NULL;
		case MMIO_REMOVEPROC:
			return NULL;
		case MMIO_FINDPROC:
			if (fccIOProc == FOURCC_DOS)
				return (LPMMIOPROC16) mmioDosIOProc;
			else if (fccIOProc == FOURCC_MEM)
				return (LPMMIOPROC16) mmioMemIOProc;
			else
				return NULL;
		default:
			return NULL;
	}
}

/**************************************************************************
 * 				mmioInstallIOProc32A   [WINMM.120]
 */
LPMMIOPROC32 WINAPI mmioInstallIOProc32A(FOURCC fccIOProc, 
                                         LPMMIOPROC32 pIOProc, DWORD dwFlags)
{
	FIXME(mmio, "(%c%c%c%c,%p,0x%08lx) -- empty stub \n",
                     (char)((fccIOProc&0xff000000)>>24),
                     (char)((fccIOProc&0x00ff0000)>>16),
                     (char)((fccIOProc&0x0000ff00)>> 8),
                     (char)(fccIOProc&0x000000ff),
                     pIOProc, dwFlags );
	return 0;
}

/**************************************************************************
* 				mmioSendMessage		[MMSYSTEM.1222]
*/
LRESULT WINAPI mmioSendMessage(HMMIO16 hmmio, UINT16 uMessage,
                               LPARAM lParam1, LPARAM lParam2)
{
	LPMMIOINFO16 lpmminfo;
	LRESULT result;
	const char *msg = NULL;

#ifdef DEBUG_RUNTIME
	switch (uMessage) {
#define msgname(x) case x: msg = #x; break;
		msgname(MMIOM_OPEN);
		msgname(MMIOM_CLOSE);
		msgname(MMIOM_READ);
		msgname(MMIOM_WRITE);
		msgname(MMIOM_WRITEFLUSH);
		msgname(MMIOM_SEEK);
		msgname(MMIOM_RENAME);
#undef msgname
	}
#endif

	if (msg)
		TRACE(mmio, "(%04X, %s, %ld, %ld)\n",
					 hmmio, msg, lParam1, lParam2);
	else
		TRACE(mmio, "(%04X, %u, %ld, %ld)\n",
					 hmmio, uMessage, lParam1, lParam2);
	
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);

	if (lpmminfo && lpmminfo->pIOProc)
		result = (*lpmminfo->pIOProc)((LPSTR)lpmminfo, uMessage, lParam1, lParam2);
	else
		result = MMSYSERR_INVALPARAM;

	GlobalUnlock16(hmmio);

	return result;
}

/**************************************************************************
* 				mmioDescend	       	[MMSYSTEM.1223]
*/
UINT16 WINAPI mmioDescend(HMMIO16 hmmio, MMCKINFO * lpck,
                          const MMCKINFO * lpckParent, UINT16 uFlags)
{
	DWORD	dwfcc, dwOldPos;

	TRACE(mmio, "(%04X, %p, %p, %04X);\n", 
				hmmio, lpck, lpckParent, uFlags);

	if (lpck == NULL)
	    return 0;

	dwfcc = lpck->ckid;
	TRACE(mmio, "dwfcc=%08lX\n", dwfcc);

	dwOldPos = mmioSeek32(hmmio, 0, SEEK_CUR);
	TRACE(mmio, "dwOldPos=%ld\n", dwOldPos);

	if (lpckParent != NULL) {
		TRACE(mmio, "seek inside parent at %ld !\n", lpckParent->dwDataOffset);
		dwOldPos = mmioSeek32(hmmio,lpckParent->dwDataOffset,SEEK_SET);
	}
/*

   It seems to be that FINDRIFF should not be treated the same as the 
   other FINDxxx so I treat it as a MMIO_FINDxxx

	if ((uFlags & MMIO_FINDCHUNK) || (uFlags & MMIO_FINDRIFF) || 
		(uFlags & MMIO_FINDLIST)) {
*/
	if ((uFlags & MMIO_FINDCHUNK) || (uFlags & MMIO_FINDLIST)) {
		TRACE(mmio, "MMIO_FINDxxxx dwfcc=%08lX !\n", dwfcc);
		while (TRUE) {
		        LONG ix;

			ix = mmioRead32(hmmio, (LPSTR)lpck, sizeof(MMCKINFO));
			TRACE(mmio, "after _lread32 ix = %ld req = %d, errno = %d\n",ix,sizeof(MMCKINFO),errno);
			if (ix < sizeof(MMCKINFO)) {

				mmioSeek32(hmmio, dwOldPos, SEEK_SET);
				WARN(mmio, "return ChunkNotFound\n");
				return MMIOERR_CHUNKNOTFOUND;
			}
			TRACE(mmio, "dwfcc=%08lX ckid=%08lX cksize=%08lX !\n", 
									dwfcc, lpck->ckid, lpck->cksize);
			if (dwfcc == lpck->ckid)
				break;

			dwOldPos += lpck->cksize + 2 * sizeof(DWORD);
			if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
				dwOldPos += sizeof(DWORD);
			mmioSeek32(hmmio, dwOldPos, SEEK_SET);
		}
	}
	else {
		if (mmioRead32(hmmio, (LPSTR)lpck, sizeof(MMCKINFO)) < sizeof(MMCKINFO)) {
			mmioSeek32(hmmio, dwOldPos, SEEK_SET);
			WARN(mmio, "return ChunkNotFound 2nd\n");
			return MMIOERR_CHUNKNOTFOUND;
		}
	}
	lpck->dwDataOffset = dwOldPos + 2 * sizeof(DWORD);
	if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
		lpck->dwDataOffset += sizeof(DWORD);
	mmioSeek32(hmmio, lpck->dwDataOffset, SEEK_SET);

	TRACE(mmio, "lpck->ckid=%08lX lpck->cksize=%ld !\n", 
								lpck->ckid, lpck->cksize);
	TRACE(mmio, "lpck->fccType=%08lX !\n", lpck->fccType);

	return 0;
}

/**************************************************************************
 * 				mmioAscend     		[WINMM.113]
 */
UINT32 WINAPI mmioAscend32(HMMIO32 hmmio, MMCKINFO * lpck, UINT32 uFlags)
{
	TRACE(mmio, "(%04X, %p, %04X);\n", 
				hmmio, lpck, uFlags);
	if (lpck->dwFlags&MMIO_DIRTY) {
		DWORD	dwOldPos, dwNewSize, dwSizePos;
		
		dwOldPos = mmioSeek32(hmmio, 0, SEEK_CUR);
		TRACE(mmio, "dwOldPos=%ld\n", dwOldPos);
		dwNewSize = dwOldPos - lpck->dwDataOffset;
		if (dwNewSize != lpck->cksize) {
			TRACE(mmio, "dwNewSize=%ld\n", dwNewSize);
			lpck->cksize = dwNewSize;

			dwSizePos = lpck->dwDataOffset - sizeof(DWORD);
			if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
				dwSizePos -= sizeof(DWORD);
			TRACE(mmio, "dwSizePos=%ld\n", dwSizePos);

			mmioSeek32(hmmio, dwSizePos, SEEK_SET);
			mmioWrite32(hmmio, (LPSTR)&dwNewSize, sizeof(DWORD));
		}
	}
	mmioSeek32(hmmio,lpck->dwDataOffset+lpck->cksize,SEEK_SET);

	return 0;
}

/**************************************************************************
 * 				mmioAscend     		[MMSYSTEM.1224]
 */
UINT16 WINAPI mmioAscend16(HMMIO16 hmmio, MMCKINFO * lpck, UINT16 uFlags)
{
	return mmioAscend32(hmmio,lpck,uFlags);
}

/**************************************************************************
 * 				mmioCreateChunk		[MMSYSTEM.1225]
 */
UINT16 WINAPI mmioCreateChunk(HMMIO16 hmmio, MMCKINFO * lpck, UINT16 uFlags)
{
	DWORD	dwOldPos;
	LONG ix;

	TRACE(mmio, "(%04X, %p, %04X);\n", 
				hmmio, lpck, uFlags);

	dwOldPos = mmioSeek32(hmmio, 0, SEEK_CUR);
	TRACE(mmio, "dwOldPos=%ld\n", dwOldPos);

	if (uFlags == MMIO_CREATELIST)
		lpck->ckid = FOURCC_LIST;
	else if (uFlags == MMIO_CREATERIFF)
		lpck->ckid = FOURCC_RIFF;

	lpck->dwDataOffset = dwOldPos + 2 * sizeof(DWORD);
	if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
		lpck->dwDataOffset += sizeof(DWORD);
	lpck->dwFlags = MMIO_DIRTY;

	ix = mmioWrite32(hmmio, (LPSTR)lpck, lpck->dwDataOffset - dwOldPos);
	TRACE(mmio, "after _lwrite32 ix = %ld req = %ld, errno = %d\n",ix,lpck->dwDataOffset - dwOldPos,errno);
	if (ix < lpck->dwDataOffset - dwOldPos) {

		mmioSeek32(hmmio, dwOldPos, SEEK_SET);
		WARN(mmio, "return CannotWrite\n");
		return MMIOERR_CANNOTWRITE;
	}

	return 0;
}


/**************************************************************************
 * 				mmioRename     		[MMSYSTEM.1226]
 */
UINT16 WINAPI mmioRename(LPCSTR szFileName, LPCSTR szNewFileName,
                         MMIOINFO16 * lpmmioinfo, DWORD dwRenameFlags)
{
	UINT16 result;
	LPMMIOINFO16 lpmminfo;
	HMMIO16 hmmio;

	TRACE(mmio, "('%s', '%s', %p, %08lX);\n",
				 szFileName, szNewFileName, lpmmioinfo, dwRenameFlags);

	hmmio = GlobalAlloc16(GHND, sizeof(MMIOINFO16));
	lpmminfo = (LPMMIOINFO16) GlobalLock16(hmmio);

	if (lpmmioinfo)
		memcpy(lpmminfo, lpmmioinfo, sizeof(MMIOINFO16));
	
	/* assume DOS file if not otherwise specified */
	if (lpmminfo->fccIOProc == 0 && lpmminfo->pIOProc == NULL) {

		lpmminfo->fccIOProc = mmioFOURCC('D', 'O', 'S', ' ');
		lpmminfo->pIOProc = (LPMMIOPROC16) mmioDosIOProc;

	}
	/* if just the four character code is present, look up IO proc */
	else if (lpmminfo->pIOProc == NULL) {

		lpmminfo->pIOProc = mmioInstallIOProc16(lpmminfo->fccIOProc, NULL, MMIO_FINDPROC);

	} 
	/* (if IO proc specified, use it and specified four character code) */

	result = (UINT16) mmioSendMessage(hmmio, MMIOM_RENAME, (LPARAM) szFileName, (LPARAM) szNewFileName);
	
	GlobalUnlock16(hmmio);
	GlobalFree16(hmmio);

	return result;
}

