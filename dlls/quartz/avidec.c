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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "aviriff.h"
#include "mmreg.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "evcode.h"
#include "vfw.h"

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"

#include "transform.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct AVIDecImpl
{
    TransformFilterImpl tf;
    HIC hvid;
    BITMAPINFOHEADER* pBihIn;
    BITMAPINFOHEADER* pBihOut;
} AVIDecImpl;

static DWORD AVIDec_SendSampleData(TransformFilterImpl* sub, LPBYTE data, DWORD size)
{
    AVIDecImpl* This = (AVIDecImpl*)sub;
    VIDEOINFOHEADER* format;
    AM_MEDIA_TYPE amt;
    HRESULT hr;
    DWORD res;
    IMediaSample* pSample = NULL;
    DWORD cbDstStream;
    LPBYTE pbDstStream;

    TRACE("%p %p %ld\n", sub, data, size);
    
    hr = IPin_ConnectionMediaType(This->tf.ppPins[0], &amt);
    if (FAILED(hr)) {
	ERR("Unable to retrieve media type\n");
	goto error;
    }
    format = (VIDEOINFOHEADER*)amt.pbFormat;

    /* Update input size to match sample size */
    This->pBihIn->biSizeImage = size;

    hr = OutputPin_GetDeliveryBuffer((OutputPin*)This->tf.ppPins[1], &pSample, NULL, NULL, 0);
    if (FAILED(hr)) {
	ERR("Unable to get delivery buffer (%lx)\n", hr);
	goto error;
    }

    hr = IMediaSample_SetActualDataLength(pSample, 0);
    assert(hr == S_OK);

    hr = IMediaSample_GetPointer(pSample, &pbDstStream);
    if (FAILED(hr)) {
	ERR("Unable to get pointer to buffer (%lx)\n", hr);
	goto error;
    }
    cbDstStream = IMediaSample_GetSize(pSample);
    if (cbDstStream < This->pBihOut->biSizeImage) {
        ERR("Sample size is too small %ld < %ld\n", cbDstStream, This->pBihOut->biSizeImage);
	hr = E_FAIL;
	goto error;
    }

    res = ICDecompress(This->hvid, 0, This->pBihIn, data, This->pBihOut, pbDstStream);
    if (res != ICERR_OK)
        ERR("Error occurred during the decompression (%lx)\n", res);

    hr = OutputPin_SendSample((OutputPin*)This->tf.ppPins[1], pSample);
    if (hr != S_OK && hr != VFW_E_NOT_CONNECTED) {
        ERR("Error sending sample (%lx)\n", hr);
	goto error;
    }

error:
    if (pSample)
        IMediaSample_Release(pSample);

    return hr;
}

static HRESULT AVIDec_ConnectInput(TransformFilterImpl* iface, const AM_MEDIA_TYPE * pmt)
{
    AVIDecImpl* pAVIDec = (AVIDecImpl*)iface;
    TRACE("%p\n", iface);
    dump_AM_MEDIA_TYPE(pmt);

    if ((IsEqualIID(&pmt->majortype, &MEDIATYPE_Video)) &&
        (!memcmp(((char*)&pmt->subtype)+4, ((char*)&MEDIATYPE_Video)+4, sizeof(GUID)-4)) && /* Check root (GUID w/o FOURCC) */
        (IsEqualIID(&pmt->formattype, &FORMAT_VideoInfo)))
    {
        HIC drv;
        VIDEOINFOHEADER* format = (VIDEOINFOHEADER*)pmt->pbFormat;

        drv = ICLocate(pmt->majortype.Data1, pmt->subtype.Data1, &format->bmiHeader, NULL, ICMODE_DECOMPRESS);
        if (drv)
        {
            AM_MEDIA_TYPE* outpmt = &((OutputPin*)pAVIDec->tf.ppPins[1])->pin.mtCurrent;
            const CLSID* outsubtype;
            DWORD bih_size;

            switch(format->bmiHeader.biBitCount)
            {
                case 32: outsubtype = &MEDIASUBTYPE_RGB32; break;
                case 24: outsubtype = &MEDIASUBTYPE_RGB24; break;
                case 16: outsubtype = &MEDIASUBTYPE_RGB565; break;
                case 8:  outsubtype = &MEDIASUBTYPE_RGB8; break;
                default:
                    FIXME("Depth %d not supported\n", format->bmiHeader.biBitCount);
                    ICClose(drv);
                    return S_FALSE;
            }
            CopyMediaType(outpmt, pmt);
            outpmt->subtype = *outsubtype;
            pAVIDec->hvid = drv;

            /* Copy bitmap header from media type to 1 for input and 1 for output */
            if (pAVIDec->pBihIn) {
                CoTaskMemFree(pAVIDec->pBihIn);
                CoTaskMemFree(pAVIDec->pBihOut);
            }
            bih_size = format->bmiHeader.biSize + format->bmiHeader.biClrUsed * 4;
            pAVIDec->pBihIn = (BITMAPINFOHEADER*)CoTaskMemAlloc(bih_size);
            if (!pAVIDec->pBihIn)
            {
                ICClose(drv);
                return E_OUTOFMEMORY;
            }
            pAVIDec->pBihOut = (BITMAPINFOHEADER*)CoTaskMemAlloc(bih_size);
            if (!pAVIDec->pBihOut)
            {
                CoTaskMemFree(pAVIDec->pBihIn);
                pAVIDec->pBihIn = NULL;
                ICClose(drv);
                return E_OUTOFMEMORY;
            }
            memcpy(pAVIDec->pBihIn, &format->bmiHeader, bih_size);
            memcpy(pAVIDec->pBihOut, &format->bmiHeader, bih_size);

            /* Update output format as non compressed bitmap */
            pAVIDec->pBihOut->biCompression = 0;
            pAVIDec->pBihOut->biSizeImage = pAVIDec->pBihOut->biWidth * pAVIDec->pBihOut->biHeight * pAVIDec->pBihOut->biBitCount / 8;

            /* Update buffer size of media samples in output */
            ((OutputPin*)pAVIDec->tf.ppPins[1])->allocProps.cbBuffer = pAVIDec->pBihOut->biSizeImage;

            TRACE("Connection accepted\n");
            return S_OK;
        }
        TRACE("Unable to find a suitable VFW decompressor\n");
    }

    TRACE("Connection refused\n");
    return S_FALSE;
}

static HRESULT AVIDec_Cleanup(TransformFilterImpl* This)
{
    AVIDecImpl* pAVIDec = (AVIDecImpl*)This;
	
    if (pAVIDec->hvid)
        ICClose(pAVIDec->hvid);

    if (pAVIDec->pBihIn) {
        CoTaskMemFree(pAVIDec->pBihIn);
        CoTaskMemFree(pAVIDec->pBihOut);
    }

    pAVIDec->hvid = NULL;
    pAVIDec->pBihIn = NULL;

    return S_OK;
}

HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    AVIDecImpl * pAVIDec;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Note: This memory is managed by the transform filter once created */
    pAVIDec = CoTaskMemAlloc(sizeof(AVIDecImpl));

    pAVIDec->hvid = NULL;
    pAVIDec->pBihIn = NULL;

    hr = TransformFilter_Create(&(pAVIDec->tf), &CLSID_AVIDec, AVIDec_SendSampleData, AVIDec_ConnectInput, AVIDec_Cleanup);

    if (FAILED(hr))
        return hr;

    *ppv = (LPVOID)pAVIDec;

    return hr;
}
