/*
 * WAVE Parser Filter
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

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "aviriff.h"
#include "mmreg.h"
#include "vfwmsgs.h"
#include "mmsystem.h"

#include "fourcc.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include <math.h>
#include <assert.h>

#include "parser.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR wcsOutputPinName[] = {'o','u','t','p','u','t',' ','p','i','n',0};

typedef struct WAVEParserImpl
{
    ParserImpl Parser;
    IMediaSample * pCurrentSample;
    LONGLONG StartOfFile; /* in media time */
    LONGLONG EndOfFile;
} WAVEParserImpl;

static HRESULT WAVEParser_Sample(LPVOID iface, IMediaSample * pSample)
{
    WAVEParserImpl *This = (WAVEParserImpl *)iface;
    LPBYTE pbSrcStream = NULL;
    long cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;
    BOOL bMoreData = TRUE;
    Parser_OutputPin * pOutputPin;
    BYTE * pbDstStream;
    long cbDstStream;
    long chunk_remaining_bytes = 0;
    long offset_src = 0;
 
    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    assert(BYTES_FROM_MEDIATIME(tStop - tStart) == cbSrcStream);

    pOutputPin = (Parser_OutputPin *)This->Parser.ppPins[1];

    if (tStop < This->StartOfFile)
	return S_OK;

    if (tStart < This->StartOfFile)
	offset_src = BYTES_FROM_MEDIATIME(This->StartOfFile - tStart);

    while (bMoreData)
    {
        if (!This->pCurrentSample)
        {
            /* cache media sample until it is ready to be despatched
             * (i.e. we reach the end of the chunk) */
            hr = OutputPin_GetDeliveryBuffer(&pOutputPin->pin, &This->pCurrentSample, NULL, NULL, 0);

            if (SUCCEEDED(hr))
            {
                hr = IMediaSample_SetActualDataLength(This->pCurrentSample, 0);
                assert(hr == S_OK);
            }
            else
            {
                TRACE("Skipping sending sample due to error (%x)\n", hr);
                This->pCurrentSample = NULL;
                break;
            }
        }

        hr = IMediaSample_GetPointer(This->pCurrentSample, &pbDstStream);

        if (SUCCEEDED(hr))
        {
            cbDstStream = IMediaSample_GetSize(This->pCurrentSample);

            chunk_remaining_bytes = cbDstStream - IMediaSample_GetActualDataLength(This->pCurrentSample);
        
            assert(chunk_remaining_bytes >= 0);
            assert(chunk_remaining_bytes <= cbDstStream - IMediaSample_GetActualDataLength(This->pCurrentSample));
        }

        if (chunk_remaining_bytes <= cbSrcStream - offset_src)
        {
            if (SUCCEEDED(hr))
            {
                memcpy(pbDstStream + IMediaSample_GetActualDataLength(This->pCurrentSample), pbSrcStream + offset_src, chunk_remaining_bytes);
                hr = IMediaSample_SetActualDataLength(This->pCurrentSample, chunk_remaining_bytes + IMediaSample_GetActualDataLength(This->pCurrentSample));
                assert(hr == S_OK);
            }

            if (SUCCEEDED(hr))
            {
                REFERENCE_TIME tAviStart, tAviStop;

                /* FIXME: hack */
                if (pOutputPin->dwSamplesProcessed == 0) {
                    IMediaSample_SetDiscontinuity(This->pCurrentSample, TRUE);
            	    IMediaSample_SetSyncPoint(This->pCurrentSample, TRUE);
	        }

                pOutputPin->dwSamplesProcessed++;

                if (pOutputPin->dwSampleSize)
                    tAviStart = (LONGLONG)ceil(10000000.0 * (float)(pOutputPin->dwSamplesProcessed - 1) * (float)IMediaSample_GetActualDataLength(This->pCurrentSample) / ((float)pOutputPin->dwSampleSize * pOutputPin->fSamplesPerSec));
                else
                    tAviStart = (LONGLONG)ceil(10000000.0 * (float)(pOutputPin->dwSamplesProcessed - 1) / (float)pOutputPin->fSamplesPerSec);
                if (pOutputPin->dwSampleSize)
                    tAviStop = (LONGLONG)ceil(10000000.0 * (float)pOutputPin->dwSamplesProcessed * (float)IMediaSample_GetActualDataLength(This->pCurrentSample) / ((float)pOutputPin->dwSampleSize * pOutputPin->fSamplesPerSec));
                else
                    tAviStop = (LONGLONG)ceil(10000000.0 * (float)pOutputPin->dwSamplesProcessed / (float)pOutputPin->fSamplesPerSec);

                IMediaSample_SetTime(This->pCurrentSample, &tAviStart, &tAviStop);

                hr = OutputPin_SendSample(&pOutputPin->pin, This->pCurrentSample);
                if (hr != S_OK && hr != VFW_E_NOT_CONNECTED)
                    ERR("Error sending sample (%x)\n", hr);
            }

            if (This->pCurrentSample)
                IMediaSample_Release(This->pCurrentSample);
            	    
            This->pCurrentSample = NULL;
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                memcpy(pbDstStream + IMediaSample_GetActualDataLength(This->pCurrentSample), pbSrcStream + offset_src, cbSrcStream - offset_src);
                IMediaSample_SetActualDataLength(This->pCurrentSample, cbSrcStream - offset_src + IMediaSample_GetActualDataLength(This->pCurrentSample));
            }
	    bMoreData = FALSE;
        }
        offset_src += chunk_remaining_bytes;
    }

    if (tStop >= This->EndOfFile)
    {
        int i;

        TRACE("End of file reached\n");

        for (i = 0; i < This->Parser.cStreams; i++)
        {
            IPin* ppin;
            HRESULT hr;

            TRACE("Send End Of Stream to output pin %d\n", i);

            hr = IPin_ConnectedTo(This->Parser.ppPins[i+1], &ppin);
            if (SUCCEEDED(hr))
            {
                hr = IPin_EndOfStream(ppin);
                IPin_Release(ppin);
            }
            if (FAILED(hr))
            {
                ERR("%x\n", hr);
                break;
            }
        }

        /* Force the pullpin thread to stop */
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WAVEParser_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream))
	return S_FALSE;
    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_WAVE))
        return S_OK;
    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_AU) || IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_AIFF))
	FIXME("AU and AIFF files not supported yet!\n");
    return S_FALSE;
}

static HRESULT WAVEParser_InputPin_PreConnect(IPin * iface, IPin * pConnectPin)
{
    PullPin *This = (PullPin *)iface;
    HRESULT hr;
    RIFFLIST list;
    RIFFCHUNK chunk;
    LONGLONG pos = 0; /* in bytes */
    PIN_INFO piOutput;
    ALLOCATOR_PROPERTIES props;
    AM_MEDIA_TYPE amt;
    float fSamplesPerSec = 0.0f;
    DWORD dwSampleSize = 0;
    DWORD dwLength = 0;
    WAVEParserImpl * pWAVEParser = (WAVEParserImpl *)This->pin.pinInfo.pFilter;

    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)This;
    lstrcpynW(piOutput.achName, wcsOutputPinName, sizeof(piOutput.achName) / sizeof(piOutput.achName[0]));
    
    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    pos += sizeof(list);

    if (list.fcc != ckidRIFF)
    {
        ERR("Input stream not a RIFF file\n");
        return E_FAIL;
    }
    if (list.cb > 1 * 1024 * 1024 * 1024) /* cannot be more than 1Gb in size */
    {
        ERR("Input stream violates RIFF spec\n");
        return E_FAIL;
    }
    if (list.fccListType != mmioFOURCC('W','A','V','E'))
    {
        ERR("Input stream not an WAVE RIFF file\n");
        return E_FAIL;
    }

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(chunk), (BYTE *)&chunk);
    pos += sizeof(chunk);
    if (chunk.fcc != mmioFOURCC('f','m','t',' '))
    {
        ERR("Expected 'fmt ' chunk, but got %.04s\n", (LPSTR)&chunk.fcc);
        return E_FAIL;
    }

    memcpy(&amt.majortype, &MEDIATYPE_Audio, sizeof(GUID));
    memcpy(&amt.formattype, &FORMAT_WaveFormatEx, sizeof(GUID));
    amt.cbFormat = chunk.cb;
    amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
    amt.pUnk = NULL;
    hr = IAsyncReader_SyncRead(This->pReader, pos, amt.cbFormat, amt.pbFormat);
    memcpy(&amt.subtype, &MEDIATYPE_Audio, sizeof(GUID));
    amt.subtype.Data1 = ((WAVEFORMATEX*)amt.pbFormat)->wFormatTag;

    pos += chunk.cb;
    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(chunk), (BYTE *)&chunk);
    if (chunk.fcc == mmioFOURCC('f','a','c','t'))
    {
        FIXME("'fact' chunk not supported yet\n");
	pos += sizeof(chunk) + chunk.cb;
	hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(chunk), (BYTE *)&chunk);
    }
    if (chunk.fcc != mmioFOURCC('d','a','t','a'))
    {
        ERR("Expected 'data' chunk, but got %.04s\n", (LPSTR)&chunk.fcc);
        return E_FAIL;
    }

    if (hr == S_OK)
    {
        pWAVEParser->StartOfFile = MEDIATIME_FROM_BYTES(pos + sizeof(RIFFCHUNK));
        pWAVEParser->EndOfFile = MEDIATIME_FROM_BYTES(pos + chunk.cb + sizeof(RIFFCHUNK));
    }

    if (hr != S_OK)
        return E_FAIL;

    props.cbAlign = ((WAVEFORMATEX*)amt.pbFormat)->nBlockAlign;
    props.cbPrefix = 0;
    props.cbBuffer = 4096;
    props.cBuffers = 2;
    
    hr = Parser_AddPin(&(pWAVEParser->Parser), &piOutput, &props, &amt, fSamplesPerSec, dwSampleSize, dwLength);
    
    TRACE("WAVE File ok\n");

    return hr;
}

static HRESULT WAVEParser_Cleanup(LPVOID iface)
{
    WAVEParserImpl *This = (WAVEParserImpl*)iface;

    TRACE("(%p)->()\n", This);

    if (This->pCurrentSample)
        IMediaSample_Release(This->pCurrentSample);
    This->pCurrentSample = NULL;

    return S_OK;
}

HRESULT WAVEParser_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    WAVEParserImpl * This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Note: This memory is managed by the transform filter once created */
    This = CoTaskMemAlloc(sizeof(WAVEParserImpl));

    This->pCurrentSample = NULL;

    hr = Parser_Create(&(This->Parser), &CLSID_WAVEParser, WAVEParser_Sample, WAVEParser_QueryAccept, WAVEParser_InputPin_PreConnect, WAVEParser_Cleanup);

    if (FAILED(hr))
        return hr;

    *ppv = (LPVOID)This;

    return hr;
}
