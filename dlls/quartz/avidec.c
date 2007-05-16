/*
 * AVI Decompressor (VFW decompressors wrapper)
 *
 * Copyright 2004-2005 Christian Costa
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
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "vfw.h"

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"

#include "transform.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static HRESULT AVIDec_Cleanup(TransformFilterImpl* pTransformFilter);

typedef struct AVIDecImpl
{
    TransformFilterImpl tf;
    HIC hvid;
    BITMAPINFOHEADER* pBihIn;
    BITMAPINFOHEADER* pBihOut;
} AVIDecImpl;

static HRESULT AVIDec_ProcessBegin(TransformFilterImpl* pTransformFilter)
{
    AVIDecImpl* This = (AVIDecImpl*)pTransformFilter;
    DWORD result;

    TRACE("(%p)->()\n", This);

    result = ICDecompressBegin(This->hvid, This->pBihIn, This->pBihOut);
    if (result != ICERR_OK)
    {
        ERR("Cannot start processing (%d)\n", result);
	return E_FAIL;
    }
    return S_OK;
}

static HRESULT AVIDec_ProcessSampleData(TransformFilterImpl* pTransformFilter, IMediaSample *pSample)
{
    AVIDecImpl* This = (AVIDecImpl*)pTransformFilter;
    VIDEOINFOHEADER* format;
    AM_MEDIA_TYPE amt;
    HRESULT hr;
    DWORD res;
    IMediaSample* pOutSample = NULL;
    DWORD cbDstStream;
    LPBYTE pbDstStream;
    DWORD cbSrcStream;
    LPBYTE pbSrcStream;

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
	return hr;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    TRACE("Sample data ptr = %p, size = %ld\n", pbSrcStream, (long)cbSrcStream);

    hr = IPin_ConnectionMediaType(This->tf.ppPins[0], &amt);
    if (FAILED(hr)) {
	ERR("Unable to retrieve media type\n");
	goto error;
    }
    format = (VIDEOINFOHEADER*)amt.pbFormat;

    /* Update input size to match sample size */
    This->pBihIn->biSizeImage = cbSrcStream;

    hr = OutputPin_GetDeliveryBuffer((OutputPin*)This->tf.ppPins[1], &pOutSample, NULL, NULL, 0);
    if (FAILED(hr)) {
	ERR("Unable to get delivery buffer (%x)\n", hr);
	goto error;
    }

    hr = IMediaSample_SetActualDataLength(pOutSample, 0);
    assert(hr == S_OK);

    hr = IMediaSample_GetPointer(pOutSample, &pbDstStream);
    if (FAILED(hr)) {
	ERR("Unable to get pointer to buffer (%x)\n", hr);
	goto error;
    }
    cbDstStream = IMediaSample_GetSize(pOutSample);
    if (cbDstStream < This->pBihOut->biSizeImage) {
        ERR("Sample size is too small %d < %d\n", cbDstStream, This->pBihOut->biSizeImage);
	hr = E_FAIL;
	goto error;
    }

    res = ICDecompress(This->hvid, 0, This->pBihIn, pbSrcStream, This->pBihOut, pbDstStream);
    if (res != ICERR_OK)
        ERR("Error occurred during the decompression (%x)\n", res);

    hr = OutputPin_SendSample((OutputPin*)This->tf.ppPins[1], pOutSample);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED) {
        ERR("Error sending sample (%x)\n", hr);
	goto error;
    }

error:
    if (pOutSample)
        IMediaSample_Release(pOutSample);

    return hr;
}

static HRESULT AVIDec_ProcessEnd(TransformFilterImpl* pTransformFilter)
{
    AVIDecImpl* This = (AVIDecImpl*)pTransformFilter;
    DWORD result;

    TRACE("(%p)->()\n", This);

    result = ICDecompressEnd(This->hvid);
    if (result != ICERR_OK)
    {
        ERR("Cannot stop processing (%d)\n", result);
	return E_FAIL;
    }
    return S_OK;
}

static HRESULT AVIDec_ConnectInput(TransformFilterImpl* pTransformFilter, const AM_MEDIA_TYPE * pmt)
{
    AVIDecImpl* This = (AVIDecImpl*)pTransformFilter;
    HRESULT hr = S_FALSE;

    TRACE("(%p)->(%p)\n", This, pmt);

    AVIDec_Cleanup(pTransformFilter);

    /* Check root (GUID w/o FOURCC) */
    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Video)) &&
        (!memcmp(((const char *)&pmt->subtype)+4, ((const char *)&MEDIATYPE_Video)+4, sizeof(GUID)-4)) &&
        (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)))
    {
        VIDEOINFOHEADER* format = (VIDEOINFOHEADER*)pmt->pbFormat;

        This->hvid = ICLocate(pmt->majortype.Data1, pmt->subtype.Data1, &format->bmiHeader, NULL, ICMODE_DECOMPRESS);
        if (This->hvid)
        {
            AM_MEDIA_TYPE* outpmt = &((OutputPin*)This->tf.ppPins[1])->pin.mtCurrent;
            const CLSID* outsubtype;
            DWORD bih_size;
            DWORD output_depth = format->bmiHeader.biBitCount;
            DWORD result;

            switch(format->bmiHeader.biBitCount)
            {
                case 32: outsubtype = &MEDIASUBTYPE_RGB32; break;
                case 24: outsubtype = &MEDIASUBTYPE_RGB24; break;
                case 16: outsubtype = &MEDIASUBTYPE_RGB565; break;
                case 8:  outsubtype = &MEDIASUBTYPE_RGB8; break;
                default:
                    TRACE("Non standard input depth %d, forced ouptut depth to 32\n", format->bmiHeader.biBitCount);
                    outsubtype = &MEDIASUBTYPE_RGB32;
                    output_depth = 32;
                    break;
            }

            /* Copy bitmap header from media type to 1 for input and 1 for output */
            bih_size = format->bmiHeader.biSize + format->bmiHeader.biClrUsed * 4;
            This->pBihIn = (BITMAPINFOHEADER*)CoTaskMemAlloc(bih_size);
            if (!This->pBihIn)
            {
                hr = E_OUTOFMEMORY;
                goto failed;
            }
            This->pBihOut = (BITMAPINFOHEADER*)CoTaskMemAlloc(bih_size);
            if (!This->pBihOut)
            {
                hr = E_OUTOFMEMORY;
                goto failed;
            }
            memcpy(This->pBihIn, &format->bmiHeader, bih_size);
            memcpy(This->pBihOut, &format->bmiHeader, bih_size);

            /* Update output format as non compressed bitmap */
            This->pBihOut->biCompression = 0;
            This->pBihOut->biBitCount = output_depth;
            This->pBihOut->biSizeImage = This->pBihOut->biWidth * This->pBihOut->biHeight * This->pBihOut->biBitCount / 8;

            result = ICDecompressQuery(This->hvid, This->pBihIn, This->pBihOut);
            if (result != ICERR_OK)
            {
                TRACE("Unable to found a suitable output format (%d)\n", result);
                goto failed;
            }

            /* Update output media type */
            CopyMediaType(outpmt, pmt);
            outpmt->subtype = *outsubtype;
            memcpy(&(((VIDEOINFOHEADER*)outpmt->pbFormat)->bmiHeader), This->pBihOut, This->pBihOut->biSize);

            /* Update buffer size of media samples in output */
            ((OutputPin*)This->tf.ppPins[1])->allocProps.cbBuffer = This->pBihOut->biSizeImage;

            TRACE("Connection accepted\n");
            return S_OK;
        }
        TRACE("Unable to find a suitable VFW decompressor\n");
    }

failed:
    AVIDec_Cleanup(pTransformFilter);
    
    TRACE("Connection refused\n");
    return hr;
}

static HRESULT AVIDec_Cleanup(TransformFilterImpl* pTransformFilter)
{
    AVIDecImpl* This = (AVIDecImpl*)pTransformFilter;

    TRACE("(%p)->()\n", This);
    
    if (This->hvid)
        ICClose(This->hvid);
    if (This->pBihIn)
        CoTaskMemFree(This->pBihIn);
    if (This->pBihOut)
        CoTaskMemFree(This->pBihOut);

    This->hvid = NULL;
    This->pBihIn = NULL;
    This->pBihOut = NULL;

    return S_OK;
}

static const TransformFuncsTable AVIDec_FuncsTable = {
    AVIDec_ProcessBegin,
    AVIDec_ProcessSampleData,
    AVIDec_ProcessEnd,
    NULL,
    AVIDec_ConnectInput,
    AVIDec_Cleanup
};

HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    AVIDecImpl * This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Note: This memory is managed by the transform filter once created */
    This = CoTaskMemAlloc(sizeof(AVIDecImpl));

    This->hvid = NULL;
    This->pBihIn = NULL;
    This->pBihOut = NULL;

    hr = TransformFilter_Create(&(This->tf), &CLSID_AVIDec, &AVIDec_FuncsTable);

    if (FAILED(hr))
        return hr;

    *ppv = (LPVOID)This;

    return hr;
}
