/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "winbase.h"
#include "winerror.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wineacm.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msacm);

/***********************************************************************
 *           acmFilterChooseA (MSACM32.13)
 */
MMRESULT WINAPI acmFilterChooseA(PACMFILTERCHOOSEA pafltrc)
{
    FIXME("(%p): stub\n", pafltrc);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterChooseW (MSACM32.14)
 */
MMRESULT WINAPI acmFilterChooseW(PACMFILTERCHOOSEW pafltrc)
{
    FIXME("(%p): stub\n", pafltrc);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterDetailsA (MSACM32.15)
 */
MMRESULT WINAPI acmFilterDetailsA(HACMDRIVER had, PACMFILTERDETAILSA pafd, 
				  DWORD fdwDetails)
{
    ACMFILTERDETAILSW	afdw;
    MMRESULT		mmr;

    memset(&afdw, 0, sizeof(afdw));
    afdw.cbStruct = sizeof(afdw);
    afdw.dwFilterIndex = pafd->dwFilterIndex;
    afdw.dwFilterTag = pafd->dwFilterTag; 
    afdw.pwfltr = pafd->pwfltr;
    afdw.cbwfltr = pafd->cbwfltr;

    mmr = acmFilterDetailsW(had, &afdw, fdwDetails);
    if (mmr == MMSYSERR_NOERROR) {
	pafd->dwFilterTag = afdw.dwFilterTag; 
	pafd->fdwSupport = afdw.fdwSupport; 
	lstrcpyWtoA(pafd->szFilter, afdw.szFilter);
    }
    return mmr;
}

/***********************************************************************
 *           acmFilterDetailsW (MSACM32.16)
 */
MMRESULT WINAPI acmFilterDetailsW(HACMDRIVER had, PACMFILTERDETAILSW pafd, 
				  DWORD fdwDetails)
{
    TRACE("(0x%08x, %p, %ld)\n", had, pafd, fdwDetails);

    if (fdwDetails & ~(ACM_FILTERDETAILSF_FILTER))
	return MMSYSERR_INVALFLAG;
    
    return MSACM_Message(had, ACMDM_FILTER_DETAILS,
			 (LPARAM) pafd,  (LPARAM) fdwDetails);
}

/***********************************************************************
 *           acmFilterEnumA (MSACM32.17)
 */
MMRESULT WINAPI acmFilterEnumA(HACMDRIVER had, PACMFILTERDETAILSA pafd, 
			       ACMFILTERENUMCBA fnCallback, DWORD dwInstance, 
			       DWORD fdwEnum)
{
    FIXME("(0x%08x, %p, %p, %ld, %ld): stub\n",
	  had, pafd, fnCallback, dwInstance, fdwEnum);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterEnumW (MSACM32.18)
 */
MMRESULT WINAPI acmFilterEnumW(HACMDRIVER had, PACMFILTERDETAILSW pafd, 
			       ACMFILTERENUMCBW fnCallback, DWORD dwInstance, 
			       DWORD fdwEnum)
{
    FIXME("(0x%08x, %p, %p, %ld, %ld): stub\n",
	  had, pafd, fnCallback, dwInstance, fdwEnum);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterTagDetailsA (MSACM32.19)
 */
MMRESULT WINAPI acmFilterTagDetailsA(HACMDRIVER had, PACMFILTERTAGDETAILSA paftda, 
				     DWORD fdwDetails)
{
    ACMFILTERTAGDETAILSW	aftdw;
    MMRESULT			mmr;

    memset(&aftdw, 0, sizeof(aftdw));
    aftdw.cbStruct = sizeof(aftdw);
    aftdw.dwFilterTagIndex = paftda->dwFilterTagIndex;
    aftdw.dwFilterTag = paftda->dwFilterTag;

    mmr = acmFilterTagDetailsW(had, &aftdw, fdwDetails);
    if (mmr == MMSYSERR_NOERROR) {
	paftda->dwFilterTag = aftdw.dwFilterTag; 
	paftda->dwFilterTagIndex = aftdw.dwFilterTagIndex;
	paftda->cbFilterSize = aftdw.cbFilterSize; 
	paftda->fdwSupport = aftdw.fdwSupport; 
	paftda->cStandardFilters = aftdw.cStandardFilters; 
	lstrcpyWtoA(paftda->szFilterTag, aftdw.szFilterTag);
    }
    return mmr;
}

/***********************************************************************
 *           acmFilterTagDetailsW (MSACM32.20)
 */
MMRESULT WINAPI acmFilterTagDetailsW(HACMDRIVER had, PACMFILTERTAGDETAILSW paftd, 
				     DWORD fdwDetails)
{
    TRACE("(0x%08x, %p, %ld)\n", had, paftd, fdwDetails);

    if (fdwDetails & 
	~(ACM_FILTERTAGDETAILSF_FILTERTAG|
	  ACM_FILTERTAGDETAILSF_LARGESTSIZE))
	return MMSYSERR_INVALFLAG;
    
    return MSACM_Message(had, ACMDM_FILTERTAG_DETAILS,
			 (LPARAM)paftd,  (LPARAM)fdwDetails);
}

/***********************************************************************
 *           acmFilterTagEnumA (MSACM32.21)
 */
MMRESULT WINAPI acmFilterTagEnumA(HACMDRIVER had, PACMFILTERTAGDETAILSA  paftd,
				  ACMFILTERTAGENUMCBA fnCallback, DWORD dwInstance, 
				  DWORD fdwEnum)
{
    FIXME("(0x%08x, %p, %p, %ld, %ld): stub\n",
	  had, paftd, fnCallback, dwInstance, fdwEnum);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}

/***********************************************************************
 *           acmFilterTagEnumW (MSACM32.22)
 */
MMRESULT WINAPI acmFilterTagEnumW(HACMDRIVER had, PACMFILTERTAGDETAILSW paftd,
				  ACMFILTERTAGENUMCBW fnCallback, DWORD dwInstance, 
				  DWORD fdwEnum)
{
    FIXME("(0x%08x, %p, %p, %ld, %ld): stub\n",
	  had, paftd, fnCallback, dwInstance, fdwEnum);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MMSYSERR_ERROR;
}
