/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 *		  1999	Eric Pouech
 */

/* TODO
 * 	+ asynchronous conversion is not implemented
 *	+ callback/notification
 *	* acmStreamMessage
 *	+ properly close ACM streams
 */

#include <string.h>
#include "winbase.h"
#include "winerror.h"
#include "windef.h"
#include "debugtools.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"

DEFAULT_DEBUG_CHANNEL(msacm);
    
static PWINE_ACMSTREAM	ACM_GetStream(HACMSTREAM has)
{
    return (PWINE_ACMSTREAM)has;
}

/***********************************************************************
 *           acmStreamClose (MSACM32.37)
 */
MMRESULT WINAPI acmStreamClose(HACMSTREAM has, DWORD fdwClose)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret;
		
    TRACE("(0x%08x, %ld)\n", has, fdwClose);
    
    if ((was = ACM_GetStream(has)) == NULL) {
	return MMSYSERR_INVALHANDLE;
    }
    ret = SendDriverMessage(was->pDrv->hDrvr, ACMDM_STREAM_CLOSE, (DWORD)&was->drvInst, 0);
    if (ret == MMSYSERR_NOERROR) {
	if (was->hAcmDriver)
	    acmDriverClose(was->hAcmDriver, 0L);	
	HeapFree(MSACM_hHeap, 0, was);
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}

/***********************************************************************
 *           acmStreamConvert (MSACM32.38)
 */
MMRESULT WINAPI acmStreamConvert(HACMSTREAM has, PACMSTREAMHEADER pash, 
				 DWORD fdwConvert)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret = MMSYSERR_NOERROR;
    PACMDRVSTREAMHEADER	padsh;

    TRACE("(0x%08x, %p, %ld)\n", has, pash, fdwConvert);
    
    if ((was = ACM_GetStream(has)) == NULL)
	return MMSYSERR_INVALHANDLE;
    if (!pash || pash->cbStruct < sizeof(ACMSTREAMHEADER))
	return MMSYSERR_INVALPARAM;

    if (!(pash->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED))
	return ACMERR_UNPREPARED;

    /* Note: the ACMSTREAMHEADER and ACMDRVSTREAMHEADER structs are of same
     * size. some fields are private to msacm internals, and are exposed
     * in ACMSTREAMHEADER in the dwReservedDriver array
     */
    padsh = (PACMDRVSTREAMHEADER)pash;

    /* check that pointers have not been modified */
    if (padsh->pbPreparedSrc != padsh->pbSrc ||
	padsh->cbPreparedSrcLength < padsh->cbSrcLength ||
	padsh->pbPreparedDst != padsh->pbDst ||
	padsh->cbPreparedDstLength < padsh->cbDstLength) {
	return MMSYSERR_INVALPARAM;
    }	

    padsh->fdwConvert = fdwConvert;

    ret = SendDriverMessage(was->pDrv->hDrvr, ACMDM_STREAM_CONVERT, (DWORD)&was->drvInst, (DWORD)padsh);
    if (ret == MMSYSERR_NOERROR) {
	padsh->fdwStatus |= ACMSTREAMHEADER_STATUSF_DONE;
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}

/***********************************************************************
 *           acmStreamMessage (MSACM32.39)
 */
MMRESULT WINAPI acmStreamMessage(HACMSTREAM has, UINT uMsg, LPARAM lParam1, 
				 LPARAM lParam2)
{
    FIXME("(0x%08x, %u, %ld, %ld): stub\n", has, uMsg, lParam1, lParam2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmStreamOpen (MSACM32.40)
 */
MMRESULT WINAPI acmStreamOpen(PHACMSTREAM phas, HACMDRIVER had, PWAVEFORMATEX pwfxSrc,
			      PWAVEFORMATEX pwfxDst, PWAVEFILTER pwfltr, DWORD dwCallback,
			      DWORD dwInstance, DWORD fdwOpen)
{
    PWINE_ACMSTREAM	was;
    PWINE_ACMDRIVER	wad;
    MMRESULT		ret;
    int			wfxSrcSize;
    int			wfxDstSize;
    
    TRACE("(%p, 0x%08x, %p, %p, %p, %ld, %ld, %ld)\n",
	  phas, had, pwfxSrc, pwfxDst, pwfltr, dwCallback, dwInstance, fdwOpen);

    TRACE("src [wFormatTag=%u, nChannels=%u, nSamplesPerSec=%lu, nAvgBytesPerSec=%lu, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u]\n", 
	  pwfxSrc->wFormatTag, pwfxSrc->nChannels, pwfxSrc->nSamplesPerSec, pwfxSrc->nAvgBytesPerSec, 
	  pwfxSrc->nBlockAlign, pwfxSrc->wBitsPerSample, pwfxSrc->cbSize);

    TRACE("dst [wFormatTag=%u, nChannels=%u, nSamplesPerSec=%lu, nAvgBytesPerSec=%lu, nBlockAlign=%u, wBitsPerSample=%u, cbSize=%u]\n", 
	  pwfxDst->wFormatTag, pwfxDst->nChannels, pwfxDst->nSamplesPerSec, pwfxDst->nAvgBytesPerSec, 
	  pwfxDst->nBlockAlign, pwfxDst->wBitsPerSample, pwfxDst->cbSize);

    if ((fdwOpen & ACM_STREAMOPENF_QUERY) && phas) return MMSYSERR_INVALPARAM;
    if (pwfltr && (pwfxSrc->wFormatTag != pwfxDst->wFormatTag)) return MMSYSERR_INVALPARAM;

    wfxSrcSize = wfxDstSize = sizeof(WAVEFORMATEX);
    if (pwfxSrc->wFormatTag != WAVE_FORMAT_PCM) wfxSrcSize += pwfxSrc->cbSize;
    if (pwfxDst->wFormatTag != WAVE_FORMAT_PCM) wfxDstSize += pwfxDst->cbSize;

    was = HeapAlloc(MSACM_hHeap, 0, sizeof(*was) + wfxSrcSize + wfxDstSize + ((pwfltr) ? sizeof(WAVEFILTER) : 0));
    if (was == NULL)
	return MMSYSERR_NOMEM;
    
    was->drvInst.cbStruct = sizeof(was->drvInst);
    was->drvInst.pwfxSrc = (PWAVEFORMATEX)((LPSTR)was + sizeof(*was));
    memcpy(was->drvInst.pwfxSrc, pwfxSrc, wfxSrcSize);
    was->drvInst.pwfxDst = (PWAVEFORMATEX)((LPSTR)was + sizeof(*was) + wfxSrcSize);
    memcpy(was->drvInst.pwfxDst, pwfxDst, wfxDstSize);
    if (pwfltr) {
	was->drvInst.pwfltr = (PWAVEFILTER)((LPSTR)was + sizeof(*was) + wfxSrcSize + wfxDstSize);
	memcpy(was->drvInst.pwfltr, pwfltr, sizeof(WAVEFILTER));
    } else {
	was->drvInst.pwfltr = NULL;
    }
    was->drvInst.dwCallback = dwCallback;    
    was->drvInst.dwInstance = dwInstance;
    was->drvInst.fdwOpen = fdwOpen;
    was->drvInst.fdwDriver = 0L;  
    was->drvInst.dwDriver = 0L;     
    was->drvInst.has = (HACMSTREAM)was;
    
    if (had) {
	if (!(wad = MSACM_GetDriver(had))) {
	    ret = MMSYSERR_INVALPARAM;
	    goto errCleanUp;
	}
	
	was->obj.dwType = WINE_ACMOBJ_STREAM;
	was->obj.pACMDriverID = wad->obj.pACMDriverID;
	was->pDrv = wad;
	was->hAcmDriver = 0; /* not to close it in acmStreamClose */

	ret = SendDriverMessage(wad->hDrvr, ACMDM_STREAM_OPEN, (DWORD)&was->drvInst, 0L);
	if (ret != MMSYSERR_NOERROR)
	    goto errCleanUp;
    } else {
	PWINE_ACMDRIVERID wadi;
	
	ret = ACMERR_NOTPOSSIBLE;
	for (wadi = MSACM_pFirstACMDriverID; wadi; wadi = wadi->pNextACMDriverID) {
	    ret = acmDriverOpen(&had, (HACMDRIVERID)wadi, 0L);
	    if (ret == MMSYSERR_NOERROR) {
		if ((wad = MSACM_GetDriver(had)) != 0) {
		    was->obj.dwType = WINE_ACMOBJ_STREAM;
		    was->obj.pACMDriverID = wad->obj.pACMDriverID;
		    was->pDrv = wad;
		    was->hAcmDriver = had;
		    
		    ret = SendDriverMessage(wad->hDrvr, ACMDM_STREAM_OPEN, (DWORD)&was->drvInst, 0L);
		    if (ret == MMSYSERR_NOERROR) {
			if (fdwOpen & ACM_STREAMOPENF_QUERY) {
			    acmDriverClose(had, 0L);
			}
			break;
		    }
		}
		/* no match, close this acm driver and try next one */
		acmDriverClose(had, 0L);
	    }
	}
	if (ret != MMSYSERR_NOERROR) {
	    ret = ACMERR_NOTPOSSIBLE;
	    goto errCleanUp;
	}
    }
    ret = MMSYSERR_NOERROR;
    if (!(fdwOpen & ACM_STREAMOPENF_QUERY)) {
	if (phas)
	    *phas = (HACMSTREAM)was;
	TRACE("=> (%d)\n", ret);
	return ret;
    }
errCleanUp:		
    if (phas)
	*phas = (HACMSTREAM)0;
    HeapFree(MSACM_hHeap, 0, was);
    TRACE("=> (%d)\n", ret);
    return ret;
}


/***********************************************************************
 *           acmStreamPrepareHeader (MSACM32.41)
 */
MMRESULT WINAPI acmStreamPrepareHeader(HACMSTREAM has, PACMSTREAMHEADER pash, 
				       DWORD fdwPrepare)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret = MMSYSERR_NOERROR;
    PACMDRVSTREAMHEADER	padsh;

    TRACE("(0x%08x, %p, %ld)\n", has, pash, fdwPrepare);
    
    if ((was = ACM_GetStream(has)) == NULL)
	return MMSYSERR_INVALHANDLE;
    if (!pash || pash->cbStruct < sizeof(ACMSTREAMHEADER))
	return MMSYSERR_INVALPARAM;
    if (fdwPrepare)
	ret = MMSYSERR_INVALFLAG;

    if (pash->fdwStatus & ACMSTREAMHEADER_STATUSF_DONE)
	return MMSYSERR_NOERROR;

    /* Note: the ACMSTREAMHEADER and ACMDRVSTREAMHEADER structs are of same
     * size. some fields are private to msacm internals, and are exposed
     * in ACMSTREAMHEADER in the dwReservedDriver array
     */
    padsh = (PACMDRVSTREAMHEADER)pash;

    padsh->fdwConvert = fdwPrepare;
    padsh->padshNext = NULL;
    padsh->fdwDriver = padsh->dwDriver = 0L;

    padsh->fdwPrepared = 0;
    padsh->dwPrepared = 0;
    padsh->pbPreparedSrc = 0;
    padsh->cbPreparedSrcLength = 0;
    padsh->pbPreparedDst = 0;
    padsh->cbPreparedDstLength = 0;

    ret = SendDriverMessage(was->pDrv->hDrvr, ACMDM_STREAM_PREPARE, (DWORD)&was->drvInst, (DWORD)padsh);
    if (ret == MMSYSERR_NOERROR || ret == MMSYSERR_NOTSUPPORTED) {
	ret = MMSYSERR_NOERROR;
	padsh->fdwStatus &= ~(ACMSTREAMHEADER_STATUSF_DONE|ACMSTREAMHEADER_STATUSF_INQUEUE);
	padsh->fdwStatus |= ACMSTREAMHEADER_STATUSF_PREPARED;
	padsh->fdwPrepared = padsh->fdwStatus;
	padsh->dwPrepared = 0;
	padsh->pbPreparedSrc = padsh->pbSrc;
	padsh->cbPreparedSrcLength = padsh->cbSrcLength;
	padsh->pbPreparedDst = padsh->pbDst;
	padsh->cbPreparedDstLength = padsh->cbDstLength;
    } else {
	padsh->fdwPrepared = 0;
	padsh->dwPrepared = 0;
	padsh->pbPreparedSrc = 0;
	padsh->cbPreparedSrcLength = 0;
	padsh->pbPreparedDst = 0;
	padsh->cbPreparedDstLength = 0;
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}

/***********************************************************************
 *           acmStreamReset (MSACM32.42)
 */
MMRESULT WINAPI acmStreamReset(HACMSTREAM has, DWORD fdwReset)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret = MMSYSERR_NOERROR;

    TRACE("(0x%08x, %ld)\n", has, fdwReset);

    if (fdwReset) {
	ret = MMSYSERR_INVALFLAG;
    } else if ((was = ACM_GetStream(has)) == NULL) {
	return MMSYSERR_INVALHANDLE;
    } else if (was->drvInst.fdwOpen & ACM_STREAMOPENF_ASYNC) {
	ret = SendDriverMessage(was->pDrv->hDrvr, ACMDM_STREAM_RESET, (DWORD)&was->drvInst, 0);
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}

/***********************************************************************
 *           acmStreamSize (MSACM32.43)
 */
MMRESULT WINAPI acmStreamSize(HACMSTREAM has, DWORD cbInput, 
			      LPDWORD pdwOutputBytes, DWORD fdwSize)
{
    PWINE_ACMSTREAM	was;
    ACMDRVSTREAMSIZE	adss;
    MMRESULT		ret;
    
    TRACE("(0x%08x, %ld, %p, %ld)\n", has, cbInput, pdwOutputBytes, fdwSize);
    
    if ((was = ACM_GetStream(has)) == NULL) {
	return MMSYSERR_INVALHANDLE;
    }
    if ((fdwSize & ~ACM_STREAMSIZEF_QUERYMASK) != 0) {
	return MMSYSERR_INVALFLAG;
    }

    *pdwOutputBytes = 0L;
    
    switch (fdwSize & ACM_STREAMSIZEF_QUERYMASK) {
    case ACM_STREAMSIZEF_DESTINATION:
	adss.cbDstLength = cbInput;
	adss.cbSrcLength = 0;
	break;
    case ACM_STREAMSIZEF_SOURCE:
	adss.cbSrcLength = cbInput;
	adss.cbDstLength = 0;
	break;
    default:	
	return MMSYSERR_INVALFLAG;
    }
    
    adss.cbStruct = sizeof(adss);
    adss.fdwSize = fdwSize;
    ret = SendDriverMessage(was->pDrv->hDrvr, ACMDM_STREAM_SIZE, 
			    (DWORD)&was->drvInst, (DWORD)&adss);
    if (ret == MMSYSERR_NOERROR) {
	switch (fdwSize & ACM_STREAMSIZEF_QUERYMASK) {
	case ACM_STREAMSIZEF_DESTINATION:
	    *pdwOutputBytes = adss.cbSrcLength;
	    break;
	case ACM_STREAMSIZEF_SOURCE:
	    *pdwOutputBytes = adss.cbDstLength;
	    break;
	}
    }
    TRACE("=> (%d) [%lu]\n", ret, *pdwOutputBytes);
    return ret;
}

/***********************************************************************
 *           acmStreamUnprepareHeader (MSACM32.44)
 */
MMRESULT WINAPI acmStreamUnprepareHeader(HACMSTREAM has, PACMSTREAMHEADER pash, 
					 DWORD fdwUnprepare)
{
    PWINE_ACMSTREAM	was;
    MMRESULT		ret = MMSYSERR_NOERROR;
    PACMDRVSTREAMHEADER	padsh;

    TRACE("(0x%08x, %p, %ld)\n", has, pash, fdwUnprepare);
    
    if ((was = ACM_GetStream(has)) == NULL)
	return MMSYSERR_INVALHANDLE;
    if (!pash || pash->cbStruct < sizeof(ACMSTREAMHEADER))
	return MMSYSERR_INVALPARAM;

    if (!(pash->fdwStatus & ACMSTREAMHEADER_STATUSF_PREPARED))
	return ACMERR_UNPREPARED;

    /* Note: the ACMSTREAMHEADER and ACMDRVSTREAMHEADER structs are of same
     * size. some fields are private to msacm internals, and are exposed
     * in ACMSTREAMHEADER in the dwReservedDriver array
     */
    padsh = (PACMDRVSTREAMHEADER)pash;

    /* check that pointers have not been modified */
    if (padsh->pbPreparedSrc != padsh->pbSrc ||
	padsh->cbPreparedSrcLength < padsh->cbSrcLength ||
	padsh->pbPreparedDst != padsh->pbDst ||
	padsh->cbPreparedDstLength < padsh->cbDstLength) {
	return MMSYSERR_INVALPARAM;
    }	

    padsh->fdwConvert = fdwUnprepare;

    ret = SendDriverMessage(was->pDrv->hDrvr, ACMDM_STREAM_UNPREPARE, (DWORD)&was->drvInst, (DWORD)padsh);
    if (ret == MMSYSERR_NOERROR || ret == MMSYSERR_NOTSUPPORTED) {
	ret = MMSYSERR_NOERROR;
	padsh->fdwStatus &= ~(ACMSTREAMHEADER_STATUSF_DONE|ACMSTREAMHEADER_STATUSF_INQUEUE|ACMSTREAMHEADER_STATUSF_PREPARED);
    }
    TRACE("=> (%d)\n", ret);
    return ret;
}
