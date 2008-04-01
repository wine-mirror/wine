/*
 * ACM Wrapper
 *
 * Copyright 2005 Christian Costa
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

#include "config.h"

#include "quartz_private.h"
#include "pin.h"

#include "uuids.h"
#include "mmreg.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "msacm.h"

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"

#include "transform.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct ACMWrapperImpl
{
    TransformFilterImpl tf;
    HACMSTREAM has;
    LPWAVEFORMATEX pWfIn;
    LPWAVEFORMATEX pWfOut;
    BOOL reinit_codec; /* FIXME: Should use sync points instead */
} ACMWrapperImpl;

static HRESULT ACMWrapper_ProcessSampleData(TransformFilterImpl* pTransformFilter, IMediaSample *pSample)
{
    ACMWrapperImpl* This = (ACMWrapperImpl*)pTransformFilter;
    AM_MEDIA_TYPE amt;
    IMediaSample* pOutSample = NULL;
    DWORD cbDstStream, cbSrcStream;
    LPBYTE pbDstStream;
    LPBYTE pbSrcStream = NULL;
    ACMSTREAMHEADER ash;
    BOOL unprepare_header = FALSE;
    MMRESULT res;
    HRESULT hr;
    LONGLONG tStart = -1, tStop = -1, tMed;

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
	return hr;
    }

    IMediaSample_GetTime(pSample, &tStart, &tStop);
    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("Sample data ptr = %p, size = %ld\n", pbSrcStream, (long)cbSrcStream);

    hr = IPin_ConnectionMediaType(This->tf.ppPins[0], &amt);
    if (FAILED(hr)) {
	ERR("Unable to retrieve media type\n");
	return hr;
    }

    ash.pbSrc = pbSrcStream;
    ash.cbSrcLength = cbSrcStream;

    while(hr == S_OK && ash.cbSrcLength)
    {
	hr = OutputPin_GetDeliveryBuffer((OutputPin*)This->tf.ppPins[1], &pOutSample, NULL, NULL, 0);
	if (FAILED(hr)) {
	    ERR("Unable to get delivery buffer (%x)\n", hr);
	    return hr;
	}

	hr = IMediaSample_SetActualDataLength(pOutSample, 0);
	assert(hr == S_OK);

	hr = IMediaSample_GetPointer(pOutSample, &pbDstStream);
	if (FAILED(hr)) {
	    ERR("Unable to get pointer to buffer (%x)\n", hr);
	    goto error;
	}
	cbDstStream = IMediaSample_GetSize(pOutSample);

	ash.cbStruct = sizeof(ash);
	ash.fdwStatus = 0;
	ash.dwUser = 0;
	ash.pbDst = pbDstStream;
	ash.cbDstLength = cbDstStream;

	if ((res = acmStreamPrepareHeader(This->has, &ash, 0))) {
	    ERR("Cannot prepare header %d\n", res);
	    goto error;
	}
	unprepare_header = TRUE;

	if ((res = acmStreamConvert(This->has, &ash, This->reinit_codec ? ACM_STREAMCONVERTF_START : 0))) {
	    if(res != MMSYSERR_MOREDATA)
	        ERR("Cannot convert data header %d\n", res);
	    goto error;
	}
	This->reinit_codec = FALSE;

	TRACE("used in %u/%u, used out %u/%u\n", ash.cbSrcLengthUsed, ash.cbSrcLength, ash.cbDstLengthUsed, ash.cbDstLength);

	hr = IMediaSample_SetActualDataLength(pOutSample, ash.cbDstLengthUsed);
	assert(hr == S_OK);

        assert(ash.cbSrcLengthUsed);
        TRACE("Sample start time: %u.%03u\n", (DWORD)(tStart/10000000), (DWORD)((tStart/10000)%1000));
        if (ash.cbSrcLengthUsed == cbSrcStream)
        {
            IMediaSample_SetTime(pOutSample, &tStart, &tStop);
            tStart = tStop;
        }
        else if (tStop != tStart)
        {
            tMed = tStop - tStart;
            tMed = tStart + tMed * ash.cbSrcLengthUsed / cbSrcStream;
            IMediaSample_SetTime(pOutSample, &tStart, &tMed);
            tStart = tMed;
        }
        else
        {
            ERR("No valid timestamp found\n");
            IMediaSample_SetTime(pOutSample, NULL, NULL);
        }
        TRACE("Sample stop time: %u.%03u\n", (DWORD)(tStart/10000000), (DWORD)((tStart/10000)%1000));

        hr = OutputPin_SendSample((OutputPin*)This->tf.ppPins[1], pOutSample);
        if (hr != S_OK && hr != VFW_E_NOT_CONNECTED) {
            if (FAILED(hr))
                ERR("Error sending sample (%x)\n", hr);
            goto error;
        }

error:
        if (unprepare_header && (res = acmStreamUnprepareHeader(This->has, &ash, 0)))
            ERR("Cannot unprepare header %d\n", res);
        unprepare_header = FALSE;
        ash.pbSrc += ash.cbSrcLengthUsed;
        ash.cbSrcLength -= ash.cbSrcLengthUsed;

        if (pOutSample)
            IMediaSample_Release(pOutSample);
        pOutSample = NULL;
    }

    return hr;
}

static HRESULT ACMWrapper_ConnectInput(TransformFilterImpl* pTransformFilter, const AM_MEDIA_TYPE * pmt)
{
    ACMWrapperImpl* This = (ACMWrapperImpl*)pTransformFilter;
    MMRESULT res;

    TRACE("(%p)->(%p)\n", This, pmt);

    /* Check root (GUID w/o FOURCC) */
    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Audio)) &&
        (!memcmp(((const char *)&pmt->subtype)+4, ((const char *)&MEDIATYPE_Audio)+4, sizeof(GUID)-4)) &&
        (IsEqualIID(&pmt->formattype, &FORMAT_WaveFormatEx)))
    {
        HACMSTREAM drv;
        AM_MEDIA_TYPE* outpmt = &((OutputPin*)This->tf.ppPins[1])->pin.mtCurrent;
        This->pWfIn = (LPWAVEFORMATEX)pmt->pbFormat;

	/* HACK */
	/* TRACE("ALIGN = %d\n", pACMWrapper->pWfIn->nBlockAlign); */
	/* pACMWrapper->pWfIn->nBlockAlign = 1; */

	/* Set output audio data to PCM */
        CopyMediaType(outpmt, pmt);
        outpmt->subtype.Data1 = WAVE_FORMAT_PCM;
	This->pWfOut = (WAVEFORMATEX*)outpmt->pbFormat;
	This->pWfOut->wFormatTag = WAVE_FORMAT_PCM;
	This->pWfOut->wBitsPerSample = 16;
	This->pWfOut->nBlockAlign = 4;
	This->pWfOut->cbSize = 0;
	This->pWfOut->nAvgBytesPerSec = This->pWfOut->nChannels * This->pWfOut->nSamplesPerSec
						* (This->pWfOut->wBitsPerSample/8);

        if (!(res = acmStreamOpen(&drv, NULL, This->pWfIn, This->pWfOut, NULL, 0, 0, 0)))
        {
            This->has = drv;

            /* Update buffer size of media samples in output */
            ((OutputPin*)This->tf.ppPins[1])->allocProps.cbBuffer = This->pWfOut->nAvgBytesPerSec / 2;
            TRACE("Connection accepted\n");
            return S_OK;
        }
	else
	    FIXME("acmStreamOpen returned %d\n", res);
        FreeMediaType(outpmt);
        TRACE("Unable to find a suitable ACM decompressor\n");
    }

    TRACE("Connection refused\n");
    return VFW_E_TYPE_NOT_ACCEPTED;
}

static HRESULT ACMWrapper_Cleanup(TransformFilterImpl* pTransformFilter)
{
    ACMWrapperImpl* This = (ACMWrapperImpl*)pTransformFilter;

    TRACE("(%p)->()\n", This);
    
    if (This->has)
	acmStreamClose(This->has, 0);

    This->has = 0;
    
    return S_OK;
}

static const TransformFuncsTable ACMWrapper_FuncsTable = {
    NULL,
    ACMWrapper_ProcessSampleData,
    NULL,
    NULL,
    ACMWrapper_ConnectInput,
    ACMWrapper_Cleanup
};

HRESULT ACMWrapper_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    ACMWrapperImpl* This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Note: This memory is managed by the transform filter once created */
    This = CoTaskMemAlloc(sizeof(ACMWrapperImpl));
    ZeroMemory(This, sizeof(ACMWrapperImpl));

    This->reinit_codec = TRUE;

    hr = TransformFilter_Create(&(This->tf), &CLSID_ACMWrapper, &ACMWrapper_FuncsTable, NULL, NULL, NULL);

    if (FAILED(hr))
        return hr;

    *ppv = (LPVOID)This;

    return hr;
}
