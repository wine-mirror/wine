/*
 * MMIO functions
 *
 * Copyright 1998 Andrew Taylor
 * Copyright 1998 Ove Kåven
 *
 */


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "heap.h"
#include "selectors.h"
#include "file.h"
#include "mmsystem.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mmio)

/**************************************************************************
 *               mmioDosIOProc           [internal]
 */
static LRESULT mmioDosIOProc(LPMMIOINFO16 lpmmioinfo, UINT16 uMessage, LPARAM lParam1, LPARAM lParam2) {
	TRACE("(%p, %X, %ld, %ld);\n", lpmmioinfo, uMessage, lParam1, lParam2);

	switch (uMessage) {

		case MMIOM_OPEN: {
			/* Parameters:
			 * lParam1 = szFileName parameter from mmioOpen
			 * lParam2 = reserved (we use it for 16-bitness)
			 * Returns: zero on success, error code on error
			 * NOTE: lDiskOffset automatically set to zero
			 */

			OFSTRUCT ofs;
			LPSTR szFileName = (LPSTR) lParam1;

			if (lpmmioinfo->dwFlags & MMIO_GETTEMP) {
				FIXME("MMIO_GETTEMP not implemented\n");
				return MMIOERR_CANNOTOPEN;
			}

			/* if filename NULL, assume open file handle in adwInfo[0] */
			if (!szFileName) {
				if (lParam2) lpmmioinfo->adwInfo[0] =
					FILE_GetHandle(lpmmioinfo->adwInfo[0]);
				return 0;
			}

			lpmmioinfo->adwInfo[0] =
				(DWORD) OpenFile(szFileName, &ofs, lpmmioinfo->dwFlags);
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

			_lclose((HFILE)lpmmioinfo->adwInfo[0]);
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

			count = _lread((HFILE)lpmmioinfo->adwInfo[0], pch, cch);
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

			count = _hwrite((HFILE)lpmmioinfo->adwInfo[0], pch, cch);
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

            pos = _llseek((HFILE)lpmmioinfo->adwInfo[0], Offset, Whence);
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

            FIXME("MMIOM_RENAME unimplemented\n");
            return MMIOERR_FILENOTFOUND;
		}

		default:
			FIXME("unexpected message %u\n", uMessage);
			return 0;
	}
	
	return 0;
}

/**************************************************************************
*               mmioMemIOProc           [internal]
*/
static LRESULT mmioMemIOProc(LPMMIOINFO16 lpmmioinfo, UINT16 uMessage, LPARAM lParam1, LPARAM lParam2) {
	TRACE("(%p,0x%04x,0x%08lx,0x%08lx)\n",lpmmioinfo,uMessage,lParam1,lParam2);
	switch (uMessage) {

		case MMIOM_OPEN: {
			/* Parameters:
			 * lParam1 = filename (must be NULL)
			 * lParam2 = reserved (we use it for 16-bitness)
			 * Returns: zero on success, error code on error
			 * NOTE: lDiskOffset automatically set to zero
			 */

			if (!(lpmmioinfo->dwFlags & MMIO_CREATE))
				lpmmioinfo->pchEndRead = lpmmioinfo->pchEndWrite;

			return 0;
		}

		case MMIOM_CLOSE: {
			/* Parameters:
			 * lParam1 = wFlags parameter from mmioClose
			 * lParam2 = unused
			 * Returns: zero on success, error code on error
			 */

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

			/* HPSTR pch = (HPSTR) lParam1; */
			/* LONG cch = (LONG) lParam2; */

			FIXME("MMIOM_READ on memory files should not occur, buffer may be lost!\n");
			return 0;
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

			/* HPSTR pch = (HPSTR) lParam1; */
			/* LONG cch = (LONG) lParam2; */

			FIXME("MMIOM_WRITE on memory files should not occur, buffer may be lost!\n");
			return 0;
		}

		case MMIOM_SEEK: {
			/* Parameters:
			 * lParam1 = new position
			 * lParam2 = from whence to seek (SEEK_SET, SEEK_CUR, SEEK_END)
			 * Returns: new file postion, -1 on error
			 * NOTE: lDiskOffset should be updated
			 */

			/* LONG Offset = (LONG) lParam1; */
			/* LONG Whence = (LONG) lParam2; */

			FIXME("MMIOM_SEEK on memory files should not occur, buffer may be lost!\n");
			return -1;
		}
		  
		default:
			FIXME("unexpected message %u\n", uMessage);
			return 0;
	}
	
	return 0;
}

/**************************************************************************
 * 		MMIO_Open      		[internal]
 */
static HMMIO16 MMIO_Open(LPSTR szFileName, MMIOINFO* refmminfo, DWORD dwOpenFlags, int use16)
{
	LPMMIOINFO16 lpmminfo;
	HMMIO16 hmmio;
	
	TRACE("('%s', %08lX);\n", szFileName, dwOpenFlags);
	
	if (dwOpenFlags & MMIO_PARSE) {
		char	buffer[MAX_PATH];
		
		if (GetFullPathNameA(szFileName, sizeof(buffer), buffer, NULL) >= sizeof(buffer))
			return (HMMIO16)FALSE;
		strcpy(szFileName, buffer);
		return (HMMIO16)TRUE;
	}

	hmmio = GlobalAlloc16(GHND, sizeof(MMIOINFO16));
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return 0;
	memset(lpmminfo, 0, sizeof(MMIOINFO16));

	/* assume DOS file if not otherwise specified */
	if (refmminfo->fccIOProc == 0 && refmminfo->pIOProc == NULL) {
		lpmminfo->fccIOProc = FOURCC_DOS;
		lpmminfo->pIOProc = (LPMMIOPROC16) mmioDosIOProc;
	}
	/* if just the four character code is present, look up IO proc */
	else if (refmminfo->pIOProc == NULL) {

		lpmminfo->fccIOProc = refmminfo->fccIOProc;
		lpmminfo->pIOProc = mmioInstallIOProc16(refmminfo->fccIOProc, NULL, MMIO_FINDPROC);

	} 
	/* if IO proc specified, use it and specified four character code */
	else {

		lpmminfo->fccIOProc = refmminfo->fccIOProc;
		lpmminfo->pIOProc = (LPMMIOPROC16)refmminfo->pIOProc;
	}

	if (dwOpenFlags & MMIO_ALLOCBUF) {
		if ((refmminfo->wErrorRet = mmioSetBuffer16(hmmio, NULL, MMIO_DEFAULTBUFFER, 0))) {
			return 0;
		}
	} else if (lpmminfo->fccIOProc == FOURCC_MEM) {
		if ((refmminfo->wErrorRet = mmioSetBuffer16(hmmio, refmminfo->pchBuffer, refmminfo->cchBuffer, 0))) {
			return 0;
		}
	}
	
	/* see mmioDosIOProc for that one */
	lpmminfo->adwInfo[0] = refmminfo->adwInfo[0];
	lpmminfo->dwFlags = dwOpenFlags;
	lpmminfo->hmmio = hmmio;
	
	/* call IO proc to actually open file */
	refmminfo->wErrorRet = (UINT16) mmioSendMessage(hmmio, MMIOM_OPEN, (LPARAM) szFileName, (LPARAM) use16);
	
	GlobalUnlock16(hmmio);
	
	if (refmminfo->wErrorRet != 0) {
		GlobalFree16(hmmio);
		return 0;
	}
	
	return hmmio;
}

/**************************************************************************
 * 				mmioOpenW       		[WINMM.123]
 */
HMMIO WINAPI mmioOpenW(LPWSTR szFileName, MMIOINFO* lpmmioinfo,
		       DWORD dwOpenFlags)
{
	LPSTR	szFn = HEAP_strdupWtoA(GetProcessHeap(),0,szFileName);
	HMMIO 	ret = mmioOpenA(szFn, lpmmioinfo, dwOpenFlags);

	HeapFree(GetProcessHeap(),0,szFn);
	return ret;
}

/**************************************************************************
 * 				mmioOpenA       		[WINMM.122]
 */
HMMIO WINAPI mmioOpenA(LPSTR szFileName, MMIOINFO* lpmmioinfo, 
		       DWORD dwOpenFlags)
{
	HMMIO 	ret;
	
	if (lpmmioinfo) {
		ret = MMIO_Open(szFileName, lpmmioinfo, dwOpenFlags, FALSE);
	} else {
	   MMIOINFO	mmioinfo;

	   mmioinfo.fccIOProc = 0;
	   mmioinfo.pIOProc = NULL;
	   mmioinfo.pchBuffer = NULL;
	   mmioinfo.cchBuffer = 0;

	   ret = MMIO_Open(szFileName, &mmioinfo, dwOpenFlags, FALSE);
	}
	return ret;
}

/**************************************************************************
 * 				mmioOpen       		[MMSYSTEM.1210]
 */
HMMIO16 WINAPI mmioOpen16(LPSTR szFileName, MMIOINFO16* lpmmioinfo, 
			  DWORD dwOpenFlags)
{
	HMMIO 		ret;
	MMIOINFO	mmio;

	if (lpmmioinfo) {
		mmio.fccIOProc = lpmmioinfo->fccIOProc;
		mmio.pIOProc = (LPMMIOPROC)lpmmioinfo->pIOProc;
		mmio.pchBuffer = PTR_SEG_TO_LIN(lpmmioinfo->pchBuffer);
		mmio.cchBuffer = lpmmioinfo->cchBuffer;
		mmio.adwInfo[0] = lpmmioinfo->adwInfo[0];
		ret = MMIO_Open(szFileName, &mmio, dwOpenFlags, FALSE);
		lpmmioinfo->wErrorRet = mmio.wErrorRet;
	} else {
	        return mmioOpenA(szFileName, NULL, dwOpenFlags);
	}
	return ret;
}

    
/**************************************************************************
 * 				mmioClose      		[WINMM.114]
 */
MMRESULT WINAPI mmioClose(HMMIO hmmio, UINT uFlags)
{
	LPMMIOINFO16 lpmminfo;
	MMRESULT result;

	TRACE("(%04X, %04X);\n", hmmio, uFlags);

	lpmminfo = (LPMMIOINFO16) GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return 0;

	/* flush the file - if error reported, ignore */
	if (mmioFlush(hmmio, MMIO_EMPTYBUF) != 0)
		lpmminfo->dwFlags &= ~MMIO_DIRTY;

	result = mmioSendMessage(hmmio,MMIOM_CLOSE,(LPARAM)uFlags,(LPARAM)0);

	mmioSetBuffer16(hmmio, NULL, 0, 0);

	GlobalUnlock16(hmmio);
	GlobalFree16(hmmio);

	return result;
}


/**************************************************************************
 * 				mmioClose      		[MMSYSTEM.1211]
 */
MMRESULT16 WINAPI mmioClose16(HMMIO16 hmmio, UINT16 uFlags)
{
	return mmioClose(hmmio,uFlags);
}



/**************************************************************************
 * 				mmioRead	       	[WINMM.124]
 */
LONG WINAPI mmioRead(HMMIO hmmio, HPSTR pch, LONG cch)
{
	LONG count;
	LPMMIOINFO16 lpmminfo;

	TRACE("(%04X, %p, %ld);\n", hmmio, pch, cch);

	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return -1;

	if (lpmminfo->pchNext != lpmminfo->pchEndRead) {
		count = lpmminfo->pchEndRead - lpmminfo->pchNext;
		if (count > cch || count < 0) count = cch;
		memcpy(pch, lpmminfo->pchNext, count);
		lpmminfo->pchNext += count;
		pch += count;
		cch -= count;
	} else
		count = 0;

	if (cch&&(lpmminfo->fccIOProc!=FOURCC_MEM)) {
		if (lpmminfo->cchBuffer) {
			mmioFlush(hmmio, MMIO_EMPTYBUF);

			while (cch) {
				LONG size;
				lpmminfo->lBufOffset = lpmminfo->lDiskOffset;
				lpmminfo->pchNext = lpmminfo->pchBuffer;
				lpmminfo->pchEndRead = lpmminfo->pchBuffer;
				size = mmioSendMessage(hmmio, MMIOM_READ,
						(LPARAM) lpmminfo->pchBuffer,
						(LPARAM) lpmminfo->cchBuffer);
				if (size<=0) break;
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
			if (count>0) lpmminfo->lBufOffset += count;
		}
	}

	GlobalUnlock16(hmmio);
	TRACE("count=%ld\n", count);
	return count;
}

/**************************************************************************
 * 				mmioRead	       	[MMSYSTEM.1212]
 */
LONG WINAPI mmioRead16(HMMIO16 hmmio, HPSTR pch, LONG cch)
{
	return mmioRead(hmmio,pch,cch);
}

/**************************************************************************
 * 				mmioWrite      		[WINMM.133]
 */
LONG WINAPI mmioWrite(HMMIO hmmio, HPCSTR pch, LONG cch)
{
	LONG count;
	LPMMIOINFO16 lpmminfo;

	TRACE("(%04X, %p, %ld);\n", hmmio, pch, cch);

	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return -1;

	if (lpmminfo->cchBuffer) {
		count = 0;
		while (cch) {
			if (lpmminfo->pchNext != lpmminfo->pchEndWrite) {
				count = lpmminfo->pchEndWrite - lpmminfo->pchNext;
				if (count > cch || count < 0) count = cch;
				memcpy(lpmminfo->pchNext, pch, count);
				lpmminfo->pchNext += count;
				pch += count;
				cch -= count;
				lpmminfo->dwFlags |= MMIO_DIRTY;
			} else
			if (lpmminfo->fccIOProc==FOURCC_MEM) {
				if (lpmminfo->adwInfo[0]) {
					/* from where would we get the memory handle? */
					FIXME("memory file expansion not implemented!\n");
				} else break;
			}

			if (lpmminfo->pchNext == lpmminfo->pchEndWrite
				&& mmioFlush(hmmio, MMIO_EMPTYBUF)) break;
		}
	} else {
		count = mmioSendMessage(hmmio, MMIOM_WRITE, (LPARAM) pch, (LPARAM) cch);
		lpmminfo->lBufOffset = lpmminfo->lDiskOffset;
	}

	GlobalUnlock16(hmmio);
	TRACE("count=%ld\n", count);
	return count;
}

/**************************************************************************
 * 				mmioWrite      		[MMSYSTEM.1213]
 */
LONG WINAPI mmioWrite16(HMMIO16 hmmio, HPCSTR pch, LONG cch)
{
	return mmioWrite(hmmio,pch,cch);
}

/**************************************************************************
 * 				mmioSeek       		[MMSYSTEM.1214]
 */
LONG WINAPI mmioSeek(HMMIO hmmio, LONG lOffset, INT iOrigin)
{
	int offset;
	LPMMIOINFO16 lpmminfo;

	TRACE("(%04X, %08lX, %d);\n", hmmio, lOffset, iOrigin);

	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL)
		return -1;

	offset = (iOrigin==SEEK_SET)?(lOffset - lpmminfo->lBufOffset):
		 (iOrigin==SEEK_CUR)?(lOffset +
		 (lpmminfo->pchNext - lpmminfo->pchBuffer)):-1;

	if ((lpmminfo->cchBuffer<0)||
	    ((offset>=0)&&(offset<=(lpmminfo->pchEndRead-lpmminfo->pchBuffer)))) {
		lpmminfo->pchNext = lpmminfo->pchBuffer + offset;
		GlobalUnlock16(hmmio);
		return lpmminfo->lBufOffset + offset;
	}

	if ((lpmminfo->fccIOProc==FOURCC_MEM)||mmioFlush(hmmio, MMIO_EMPTYBUF)) {
		GlobalUnlock16(hmmio);
		return -1;
	}

	offset = mmioSendMessage(hmmio, MMIOM_SEEK, (LPARAM) lOffset, (LPARAM) iOrigin);
	lpmminfo->lBufOffset = lpmminfo->lDiskOffset;

	GlobalUnlock16(hmmio);
	return offset;
}

/**************************************************************************
 * 				mmioSeek       		[MMSYSTEM.1214]
 */
LONG WINAPI mmioSeek16(HMMIO16 hmmio, LONG lOffset, INT16 iOrigin)
{
	return mmioSeek(hmmio,lOffset,iOrigin);
}

/**************************************************************************
 * 				mmioGetInfo	       	[MMSYSTEM.1215]
 */
UINT16 WINAPI mmioGetInfo16(HMMIO16 hmmio, MMIOINFO16 * lpmmioinfo, UINT16 uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE("mmioGetInfo\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	memcpy(lpmmioinfo, lpmminfo, sizeof(MMIOINFO16));
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
 * 				mmioGetInfo	       	[WINMM.118]
 */
UINT WINAPI mmioGetInfo(HMMIO hmmio, MMIOINFO*lpmmioinfo, UINT uFlags)
{
	MMIOINFO16	mmioinfo;
	LPMMIOINFO16	lpmminfo=&mmioinfo;
	UINT16		ret;

	TRACE("(0x%04x,%p,0x%08x)\n",hmmio,lpmmioinfo,uFlags);
	ret = mmioGetInfo16(hmmio,&mmioinfo,uFlags);
	if (!ret)
		return 0;
	lpmmioinfo->dwFlags	= lpmminfo->dwFlags;
	lpmmioinfo->fccIOProc	= lpmminfo->fccIOProc;
	lpmmioinfo->pIOProc	= (LPMMIOPROC)lpmminfo->pIOProc;
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
UINT16 WINAPI mmioSetInfo16(HMMIO16 hmmio, const MMIOINFO16 * lpmmioinfo, UINT16 uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE("mmioSetInfo\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	lpmminfo->pchNext	= lpmmioinfo->pchNext;
	lpmminfo->pchEndRead	= lpmmioinfo->pchEndRead;
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
 * 				mmioSetInfo    		[WINMM.130]
 */
UINT WINAPI mmioSetInfo(HMMIO hmmio, const MMIOINFO * lpmmioinfo, UINT uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE("mmioSetInfo\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	lpmminfo->pchNext	= lpmmioinfo->pchNext;
	lpmminfo->pchEndRead	= lpmmioinfo->pchEndRead;
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
* 				mmioSetBuffer		[WINMM.129]
*/
UINT WINAPI mmioSetBuffer(HMMIO hmmio, LPSTR pchBuffer, 
										LONG cchBuffer, UINT uFlags)
{
	LPMMIOINFO16	lpmminfo;

	if (mmioFlush(hmmio, MMIO_EMPTYBUF) != 0)
		return MMIOERR_CANNOTWRITE;

	TRACE("(hmmio=%04x, pchBuf=%p, cchBuf=%ld, uFlags=%#08x)\n",
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
	} else if (lpmminfo->dwFlags&MMIO_ALLOCBUF) {
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
	} else if (cchBuffer) {
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
* 				mmioSetBuffer		[MMSYSTEM.1217]
*/
UINT16 WINAPI mmioSetBuffer16(HMMIO16 hmmio, LPSTR pchBuffer, 
										LONG cchBuffer, UINT16 uFlags)
{
	return mmioSetBuffer(hmmio, pchBuffer, cchBuffer, uFlags);
}

/**************************************************************************
 * 				mmioFlush      		[WINMM.117]
 */
UINT WINAPI mmioFlush(HMMIO hmmio, UINT uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE("(%04X, %04X)\n", hmmio, uFlags);
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;

	if ((!lpmminfo->cchBuffer)||(lpmminfo->fccIOProc==FOURCC_MEM)) {
		GlobalUnlock16(hmmio);
		return 0;
	}
	/* not quite sure what to do here, but I'll guess */
	if (lpmminfo->dwFlags & MMIO_DIRTY) {
		mmioSendMessage(hmmio, MMIOM_SEEK,
			(LPARAM) lpmminfo->lBufOffset,
			(LPARAM) SEEK_SET);
		mmioSendMessage(hmmio, MMIOM_WRITE,
			(LPARAM) lpmminfo->pchBuffer,
			(LPARAM) (lpmminfo->pchNext - lpmminfo->pchBuffer) );
		lpmminfo->dwFlags &= ~MMIO_DIRTY;
	}
	if (uFlags & MMIO_EMPTYBUF) {
		/* seems Windows doesn't do any seeking here, hopefully this
		   won't matter, otherwise a slight rewrite is necessary */
		mmioSendMessage(hmmio, MMIOM_SEEK,
			(LPARAM) (lpmminfo->lBufOffset +
				  (lpmminfo->pchNext - lpmminfo->pchBuffer)),
			(LPARAM) SEEK_SET);
		lpmminfo->pchNext = lpmminfo->pchBuffer;
		lpmminfo->pchEndRead = lpmminfo->pchBuffer;
		lpmminfo->lBufOffset = lpmminfo->lDiskOffset;
	}

	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
 * 				mmioFlush      		[MMSYSTEM.1218]
 */
UINT16 WINAPI mmioFlush16(HMMIO16 hmmio, UINT16 uFlags)
{
	return mmioFlush(hmmio,uFlags);
}

/**************************************************************************
 * 				mmioAdvance    		[MMSYSTEM.1219]
 */
UINT WINAPI mmioAdvance(HMMIO hmmio,MMIOINFO*lpmmioinfo,UINT uFlags)
{
	LPMMIOINFO16	lpmminfo;
	TRACE("mmioAdvance\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	if (!lpmminfo->cchBuffer) {
		GlobalUnlock16(hmmio);
		return MMIOERR_UNBUFFERED;
	}
	lpmminfo->pchNext = lpmmioinfo->pchNext;
	if (mmioFlush(hmmio, MMIO_EMPTYBUF)) {
		GlobalUnlock16(hmmio);
		return MMIOERR_CANNOTWRITE;
	}
	if (uFlags == MMIO_READ)
	        lpmmioinfo->pchEndRead = lpmmioinfo->pchBuffer +
	            mmioSendMessage(hmmio, MMIOM_READ,
	                (LPARAM) lpmmioinfo->pchBuffer,
	                (LPARAM) lpmmioinfo->cchBuffer);
#if 0   /* mmioFlush already did the writing */
	if (uFlags == MMIO_WRITE)
	            mmioSendMessage(hmmio, MMIOM_WRITE,
	                (LPARAM) lpmmioinfo->pchBuffer,
	                (LPARAM) lpmmioinfo->cchBuffer);
#endif
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
	TRACE("mmioAdvance\n");
	lpmminfo = (LPMMIOINFO16)GlobalLock16(hmmio);
	if (lpmminfo == NULL) return 0;
	if (!lpmminfo->cchBuffer) {
		GlobalUnlock16(hmmio);
		return MMIOERR_UNBUFFERED;
	}
	lpmminfo->pchNext = lpmmioinfo->pchNext;
	if (mmioFlush(hmmio, MMIO_EMPTYBUF)) {
		GlobalUnlock16(hmmio);
		return MMIOERR_CANNOTWRITE;
	}
	if (uFlags == MMIO_READ)
	        lpmmioinfo->pchEndRead = lpmmioinfo->pchBuffer +
	            mmioSendMessage(hmmio, MMIOM_READ,
	                (LPARAM) lpmmioinfo->pchBuffer,
	                (LPARAM) lpmmioinfo->cchBuffer);
#if 0   /* mmioFlush already did the writing */
	if (uFlags == MMIO_WRITE)
	            mmioSendMessage(hmmio, MMIOM_WRITE,
	                (LPARAM) lpmmioinfo->pchBuffer,
	                (LPARAM) lpmmioinfo->cchBuffer);
#endif
	lpmmioinfo->pchNext = lpmmioinfo->pchBuffer;
	GlobalUnlock16(hmmio);
	return 0;
}

/**************************************************************************
 * 				mmioStringToFOURCCA	[WINMM.131]
 */
FOURCC WINAPI mmioStringToFOURCCA(LPCSTR sz, UINT uFlags)
{
	return mmioStringToFOURCC16(sz,uFlags);
}

/**************************************************************************
 * 				mmioStringToFOURCCW	[WINMM.132]
 */
FOURCC WINAPI mmioStringToFOURCCW(LPCWSTR sz, UINT uFlags)
{
	LPSTR	szA = HEAP_strdupWtoA(GetProcessHeap(),0,sz);
	FOURCC	ret = mmioStringToFOURCCA(szA,uFlags);

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


/* maximum number of I/O procedures which can be installed */

struct IOProcList
{
  struct IOProcList *pNext;      /* Next item in linked list */
  FOURCC            fourCC;     /* four-character code identifying IOProc */
  LPMMIOPROC16      pIOProc;    /* pointer to IProc */
};

/* This array will be the entire list for most apps */

static struct IOProcList defaultProcs[] = {
  { &defaultProcs[1], (FOURCC) FOURCC_DOS,(LPMMIOPROC16) mmioDosIOProc },
  { NULL, (FOURCC) FOURCC_MEM,(LPMMIOPROC16) mmioMemIOProc },
};

static struct IOProcList *pIOProcListAnchor = &defaultProcs[0];

LPMMIOPROC16 WINAPI mmioInstallIOProc16(FOURCC fccIOProc, 
                                        LPMMIOPROC16 pIOProc, DWORD dwFlags)
{
	LPMMIOPROC16        lpProc = NULL;
        struct IOProcList  *pListNode;

	TRACE("(%ld, %p, %08lX)\n",
				 fccIOProc, pIOProc, dwFlags);

	if (dwFlags & MMIO_GLOBALPROC) {
		FIXME(" global procedures not implemented\n");
	}

	/* just handle the known procedures for now */
	switch(dwFlags & (MMIO_INSTALLPROC|MMIO_REMOVEPROC|MMIO_FINDPROC)) {
		case MMIO_INSTALLPROC:
	          /* Create new entry for the IOProc list */
	          pListNode = HeapAlloc(GetProcessHeap(),0,sizeof(*pListNode));
	          if (pListNode)
	          {
	            /* Find the end of the list, so we can add our new entry to it */
	            struct IOProcList *pListEnd = pIOProcListAnchor;
	            while (pListEnd->pNext) 
	               pListEnd = pListEnd->pNext;
	    
	            /* Fill in this node */
	            pListNode->fourCC = fccIOProc;
	            pListNode->pIOProc = pIOProc;
	    
	            /* Stick it on the end of the list */
	            pListEnd->pNext = pListNode;
	            pListNode->pNext = NULL;
	    
	            /* Return this IOProc - that's how the caller knows we succeeded */
	            lpProc = pIOProc;
	          };  
	          break;
	          
		case MMIO_REMOVEPROC:
	          /* 
	           * Search for the node that we're trying to remove - note
	           * that this method won't find the first item on the list, but
	           * since the first two items on this list are ones we won't
	           * let the user delete anyway, that's okay
	           */
	    
	          pListNode = pIOProcListAnchor;
	          while (pListNode && pListNode->pNext->fourCC != fccIOProc)
	            pListNode = pListNode->pNext;
	    
	          /* If we found it, remove it, but only if it isn't builtin */
	          if (pListNode && 
	              ((pListNode >= defaultProcs) && (pListNode < defaultProcs + sizeof(defaultProcs))))
	          {
	            /* Okay, nuke it */
	            pListNode->pNext = pListNode->pNext->pNext;
	            HeapFree(GetProcessHeap(),0,pListNode);
	          };
	          break;
	    
		case MMIO_FINDPROC:
		      /* Iterate through the list looking for this proc identified by fourCC */
		      for (pListNode = pIOProcListAnchor; pListNode; pListNode=pListNode->pNext)
		      {
		        if (pListNode->fourCC == fccIOProc)
		        {
		          lpProc = pListNode->pIOProc;
		          break;
		        };
		      };
		      break;
	}

	return lpProc;
}

/**************************************************************************
 * 				mmioInstallIOProcA	   [WINMM.120]
 */
LPMMIOPROC WINAPI mmioInstallIOProcA(FOURCC fccIOProc, 
                                         LPMMIOPROC pIOProc, DWORD dwFlags)
{
	return (LPMMIOPROC) mmioInstallIOProc16(fccIOProc,
                                        (LPMMIOPROC16)pIOProc, dwFlags);
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
		TRACE("(%04X, %s, %ld, %ld)\n",
					 hmmio, msg, lParam1, lParam2);
	else
		TRACE("(%04X, %u, %ld, %ld)\n",
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
UINT16 WINAPI mmioDescend(HMMIO16 hmmio, LPMMCKINFO lpck,
                          const MMCKINFO * lpckParent, UINT16 uFlags)
{
	DWORD		dwOldPos;
	FOURCC		srchCkId;
	FOURCC		srchType;

	
	TRACE("(%04X, %p, %p, %04X);\n", hmmio, lpck, lpckParent, uFlags);
	
	if (lpck == NULL)
		return MMSYSERR_INVALPARAM;
	
	dwOldPos = mmioSeek(hmmio, 0, SEEK_CUR);
	TRACE("dwOldPos=%ld\n", dwOldPos);
	
	if (lpckParent != NULL) {
		TRACE("seek inside parent at %ld !\n", lpckParent->dwDataOffset);
		/* EPP: was dwOldPos = mmioSeek(hmmio,lpckParent->dwDataOffset,SEEK_SET); */
		if (dwOldPos < lpckParent->dwDataOffset || dwOldPos >= lpckParent->dwDataOffset + lpckParent->cksize) {
			WARN("outside parent chunk\n");
			return MMIOERR_CHUNKNOTFOUND;
		}
	}
	
	/* The SDK docu says 'ckid' is used for all cases. Real World
	 * examples disagree -Marcus,990216. 
	 */
	
	srchType = 0;
	/* find_chunk looks for 'ckid' */
	if (uFlags & MMIO_FINDCHUNK)
		srchCkId = lpck->ckid;
	/* find_riff and find_list look for 'fccType' */
	if (uFlags & MMIO_FINDLIST) {
		srchCkId = FOURCC_LIST;
		srchType = lpck->fccType;
	}
	if (uFlags & MMIO_FINDRIFF) {
		srchCkId = FOURCC_RIFF;
		srchType = lpck->fccType;
	}

	TRACE("searching for %.4s.%.4s\n", 
	      (LPSTR)&srchCkId,
	      srchType?(LPSTR)&srchType:"<any>");
	
	if (uFlags & (MMIO_FINDCHUNK|MMIO_FINDLIST|MMIO_FINDRIFF)) {
		while (TRUE) {
		        LONG ix;
			
			ix = mmioRead(hmmio, (LPSTR)lpck, 3 * sizeof(DWORD));
			if (ix < 2*sizeof(DWORD)) {
				mmioSeek(hmmio, dwOldPos, SEEK_SET);
				WARN("return ChunkNotFound\n");
				return MMIOERR_CHUNKNOTFOUND;
			}
			lpck->dwDataOffset = dwOldPos + 2 * sizeof(DWORD);
			if (ix < lpck->dwDataOffset - dwOldPos) {
				mmioSeek(hmmio, dwOldPos, SEEK_SET);
				WARN("return ChunkNotFound\n");
				return MMIOERR_CHUNKNOTFOUND;
			}
			TRACE("ckid=%.4ss fcc=%.4ss cksize=%08lX !\n",
			      (LPSTR)&lpck->ckid, 
			      srchType?(LPSTR)&lpck->fccType:"<unused>",
			      lpck->cksize);
			if ((srchCkId == lpck->ckid) &&
			    (!srchType || (srchType == lpck->fccType))
			    )
				break;
			
			dwOldPos = lpck->dwDataOffset + ((lpck->cksize + 1) & ~1);
			mmioSeek(hmmio, dwOldPos, SEEK_SET);
		}
	} else {
		/* FIXME: unverified, does it do this? */
		if (mmioRead(hmmio, (LPSTR)lpck, 3 * sizeof(DWORD)) < 3 * sizeof(DWORD)) {
			mmioSeek(hmmio, dwOldPos, SEEK_SET);
			WARN("return ChunkNotFound 2nd\n");
			return MMIOERR_CHUNKNOTFOUND;
		}
		lpck->dwDataOffset = dwOldPos + 2 * sizeof(DWORD);
	}
	lpck->dwFlags = 0;
	/* If we were looking for RIFF/LIST chunks, the final file position
	 * is after the chunkid. If we were just looking for the chunk
	 * it is after the cksize. So add 4 in RIFF/LIST case.
	 */
	if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST)
		mmioSeek(hmmio, lpck->dwDataOffset + sizeof(DWORD), SEEK_SET);
	else
		mmioSeek(hmmio, lpck->dwDataOffset, SEEK_SET);
	TRACE("lpck: ckid=%.4s, cksize=%ld, dwDataOffset=%ld fccType=%08lX (%.4s)!\n", 
	      (LPSTR)&lpck->ckid, lpck->cksize, lpck->dwDataOffset, 
	      lpck->fccType, srchType?(LPSTR)&lpck->fccType:"");
	return 0;
}

/**************************************************************************
 * 				mmioAscend     		[WINMM.113]
 */
UINT WINAPI mmioAscend(HMMIO hmmio, MMCKINFO * lpck, UINT uFlags)
{
	TRACE("(%04X, %p, %04X);\n", hmmio, lpck, uFlags);

	if (lpck->dwFlags & MMIO_DIRTY) {
		DWORD	dwOldPos, dwNewSize, dwSizePos;
		
		TRACE("chunk is marked MMIO_DIRTY, correcting chunk size\n");
		dwOldPos = mmioSeek(hmmio, 0, SEEK_CUR);
		TRACE("dwOldPos=%ld\n", dwOldPos);
		dwNewSize = dwOldPos - lpck->dwDataOffset;
		if (dwNewSize != lpck->cksize) {
			TRACE("dwNewSize=%ld\n", dwNewSize);
			lpck->cksize = dwNewSize;
			
			dwSizePos = lpck->dwDataOffset - sizeof(DWORD);
			TRACE("dwSizePos=%ld\n", dwSizePos);
			
			mmioSeek(hmmio, dwSizePos, SEEK_SET);
			mmioWrite(hmmio, (LPSTR)&dwNewSize, sizeof(DWORD));
		}
	}

	mmioSeek(hmmio, lpck->dwDataOffset + ((lpck->cksize + 1) & ~1), SEEK_SET);
	
	return 0;
}

/**************************************************************************
 * 				mmioAscend     		[MMSYSTEM.1224]
 */
UINT16 WINAPI mmioAscend16(HMMIO16 hmmio, MMCKINFO * lpck, UINT16 uFlags)
{
	return mmioAscend(hmmio,lpck,uFlags);
}

/**************************************************************************
 * 				mmioCreateChunk		[MMSYSTEM.1225]
 */
UINT16 WINAPI mmioCreateChunk16(HMMIO16 hmmio, MMCKINFO * lpck, UINT16 uFlags)
{
	DWORD	dwOldPos;
	LONG 	size;
	LONG 	ix;

	TRACE("(%04X, %p, %04X);\n", hmmio, lpck, uFlags);

	dwOldPos = mmioSeek(hmmio, 0, SEEK_CUR);
	TRACE("dwOldPos=%ld\n", dwOldPos);

	if (uFlags == MMIO_CREATELIST)
		lpck->ckid = FOURCC_LIST;
	else if (uFlags == MMIO_CREATERIFF)
		lpck->ckid = FOURCC_RIFF;

	TRACE("ckid=%08lX\n", lpck->ckid);

	size = 2 * sizeof(DWORD);
	lpck->dwDataOffset = dwOldPos + size;

	if (lpck->ckid == FOURCC_RIFF || lpck->ckid == FOURCC_LIST) 
		size += sizeof(DWORD);
	lpck->dwFlags = MMIO_DIRTY;

	ix = mmioWrite(hmmio, (LPSTR)lpck, size);
	TRACE("after mmioWrite ix = %ld req = %ld, errno = %d\n",ix, size, errno);
	if (ix < size) {
		mmioSeek(hmmio, dwOldPos, SEEK_SET);
		WARN("return CannotWrite\n");
		return MMIOERR_CANNOTWRITE;
	}

	return 0;
}

/**************************************************************************
 * 					mmioCreateChunk					[WINMM.115]
 */
UINT WINAPI mmioCreateChunk(HMMIO hmmio, MMCKINFO * lpck, UINT uFlags)
{
	return mmioCreateChunk16(hmmio, lpck, uFlags);
}

/**************************************************************************
 * 				mmioRename     		[MMSYSTEM.1226]
 */
UINT16 WINAPI mmioRename16(LPCSTR szFileName, LPCSTR szNewFileName,
                         MMIOINFO16 * lpmmioinfo, DWORD dwRenameFlags)
{
	UINT16 result;
	LPMMIOINFO16 lpmminfo;
	HMMIO16 hmmio;

	TRACE("('%s', '%s', %p, %08lX);\n",
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

/**************************************************************************
 * 				mmioRenameA     			[WINMM.125]
 */
UINT WINAPI mmioRenameA(LPCSTR szFileName, LPCSTR szNewFileName,
                         MMIOINFO* lpmmioinfo, DWORD dwRenameFlags)
{
	FIXME("This may fail\n");
	return mmioRename16(szFileName, szNewFileName, (MMIOINFO16*)lpmmioinfo, dwRenameFlags);
}

/**************************************************************************
 * 				mmioRenameW     			[WINMM.126]
 */
UINT WINAPI mmioRenameW(LPCWSTR szFileName, LPCWSTR szNewFileName,
									 MMIOINFO* lpmmioinfo, DWORD dwRenameFlags)
{
	LPSTR		szFn = HEAP_strdupWtoA(GetProcessHeap(), 0, szFileName);
	LPSTR		sznFn = HEAP_strdupWtoA(GetProcessHeap(), 0, szNewFileName);
	UINT	ret = mmioRenameA(szFn, sznFn, lpmmioinfo, dwRenameFlags);

	HeapFree(GetProcessHeap(),0,szFn);
	HeapFree(GetProcessHeap(),0,sznFn);
	return ret;
}
