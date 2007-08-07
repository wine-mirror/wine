/*
 * Filter Seeking and Control Interfaces
 *
 * Copyright 2003 Robert Shearman
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
/* FIXME: critical sections */

#include "quartz_private.h"
#include "control_private.h"

#include "uuids.h"
#include "wine/debug.h"

#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

HRESULT MediaSeekingImpl_Init(LPVOID pUserData, CHANGEPROC fnChangeStop, CHANGEPROC fnChangeStart, CHANGEPROC fnChangeRate, MediaSeekingImpl * pSeeking)
{
    assert(fnChangeStop && fnChangeStart && fnChangeRate);

    pSeeking->refCount = 1;
    pSeeking->pUserData = pUserData;
    pSeeking->fnChangeRate = fnChangeRate;
    pSeeking->fnChangeStop = fnChangeStop;
    pSeeking->fnChangeStart = fnChangeStart;
    pSeeking->dwCapabilities = AM_SEEKING_CanSeekForwards |
        AM_SEEKING_CanSeekBackwards |
        AM_SEEKING_CanSeekAbsolute |
        AM_SEEKING_CanGetStopPos |
        AM_SEEKING_CanGetDuration;
    pSeeking->llStart = 0;
    pSeeking->llStop = ((ULONGLONG)0x80000000) << 32;
    pSeeking->llDuration = pSeeking->llStop - pSeeking->llStart;
    pSeeking->dRate = 1.0;

    return S_OK;
}


HRESULT WINAPI MediaSeekingImpl_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", pCapabilities);

    *pCapabilities = This->dwCapabilities;

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    HRESULT hr;
    DWORD dwCommonCaps;

    TRACE("(%p)\n", pCapabilities);

    dwCommonCaps = *pCapabilities & This->dwCapabilities;

    if (!dwCommonCaps)
        hr = E_FAIL;
    else
        hr = (*pCapabilities == dwCommonCaps) ?  S_OK : S_FALSE;
    *pCapabilities = dwCommonCaps;

    return hr;
}

HRESULT WINAPI MediaSeekingImpl_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat)
{
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    return (IsEqualIID(pFormat, &TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE);
}

HRESULT WINAPI MediaSeekingImpl_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat)
{
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat)
{
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    return (IsEqualIID(pFormat, &TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE);
}

HRESULT WINAPI MediaSeekingImpl_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    return (IsEqualIID(pFormat, &TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE);
}

HRESULT WINAPI MediaSeekingImpl_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", pDuration);

    *pDuration = This->llDuration;

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", pStop);

    *pStop = This->llStop;

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", pCurrent);

    *pCurrent = This->llStart;

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat)
{
    if (IsEqualIID(pTargetFormat, &TIME_FORMAT_MEDIA_TIME) && IsEqualIID(pSourceFormat, &TIME_FORMAT_MEDIA_TIME))
    {
        *pTarget = Source;
        return S_OK;
    }
    /* FIXME: clear pTarget? */
    return E_INVALIDARG;
}

static inline LONGLONG Adjust(LONGLONG value, const LONGLONG * pModifier, DWORD dwFlags)
{
    switch (dwFlags & AM_SEEKING_PositioningBitsMask)
    {
    case AM_SEEKING_NoPositioning:
        return value;
    case AM_SEEKING_AbsolutePositioning:
        return *pModifier;
    case AM_SEEKING_RelativePositioning:
    case AM_SEEKING_IncrementalPositioning:
        return value + *pModifier;
    default:
        assert(FALSE);
        return 0;
    }
}

HRESULT WINAPI MediaSeekingImpl_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    BOOL bChangeStart = FALSE, bChangeStop = FALSE;
    LONGLONG llNewStart, llNewStop;

    TRACE("(%p, %x, %p, %x)\n", pCurrent, dwCurrentFlags, pStop, dwStopFlags);

    llNewStart = Adjust(This->llStart, pCurrent, dwCurrentFlags);
    llNewStop = Adjust(This->llStop, pStop, dwStopFlags);

    if (llNewStart != This->llStart)
        bChangeStart = TRUE;
    if (llNewStop != This->llStop)
        bChangeStop = TRUE;

    This->llStart = llNewStart;
    This->llStop = llNewStop;

    if (dwCurrentFlags & AM_SEEKING_ReturnTime)
        *pCurrent = llNewStart;
    if (dwStopFlags & AM_SEEKING_ReturnTime)
        *pStop = llNewStop;

    if (bChangeStart)
        This->fnChangeStart(This->pUserData);
    if (bChangeStop)
        This->fnChangeStop(This->pUserData);

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p, %p)\n", pCurrent, pStop);

    *pCurrent = This->llStart;
    *pStop = This->llStop;
	
    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p, %p)\n", pEarliest, pLatest);

    *pEarliest = 0;
    *pLatest = This->llDuration;

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_SetRate(IMediaSeeking * iface, double dRate)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    BOOL bChangeRate = (dRate != This->dRate);

    TRACE("(%e)\n", dRate);
	
    This->dRate = dRate;
	
    if (bChangeRate)
        return This->fnChangeRate(This->pUserData);
    else
        return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetRate(IMediaSeeking * iface, double * dRate)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", dRate);

    *dRate = This->dRate;

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll)
{
    TRACE("(%p)\n", pPreroll);

    *pPreroll = 0;
    return S_OK;
}
