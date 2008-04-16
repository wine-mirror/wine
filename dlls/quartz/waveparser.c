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
#include "vfwmsgs.h"
#include "mmsystem.h"

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
    DWORD dwSampleSize;
    DWORD nSamplesPerSec;
    DWORD dwLength;
} WAVEParserImpl;

static LONGLONG bytepos_to_duration(WAVEParserImpl *This, LONGLONG bytepos)
{
    LONGLONG duration = BYTES_FROM_MEDIATIME(bytepos - This->StartOfFile);
    duration *= 10000000;
    duration /= (This->dwSampleSize * This->nSamplesPerSec);

    return duration;
}

static LONGLONG duration_to_bytepos(WAVEParserImpl *This, LONGLONG duration)
{
    LONGLONG bytepos;

    bytepos = (This->dwSampleSize * This->nSamplesPerSec);
    bytepos *= duration;
    bytepos /= 10000000;
    bytepos += BYTES_FROM_MEDIATIME(This->StartOfFile);
    bytepos -= bytepos % This->dwSampleSize;

    return MEDIATIME_FROM_BYTES(bytepos);
}

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

    /* Try to get rid of the current sample in case we had a S_FALSE last time */
    if (This->pCurrentSample && (IMediaSample_GetActualDataLength(This->pCurrentSample) == IMediaSample_GetSize(This->pCurrentSample)))
    {
        HRESULT hr;

        /* Unset advancement */
        This->Parser.pInputPin->rtCurrent -= MEDIATIME_FROM_BYTES(cbSrcStream);

        hr = OutputPin_SendSample(&pOutputPin->pin, This->pCurrentSample);

        if (hr != S_OK)
            return hr;

        IMediaSample_Release(This->pCurrentSample);
        This->pCurrentSample = NULL;

        This->Parser.pInputPin->rtCurrent += MEDIATIME_FROM_BYTES(cbSrcStream);
    }

    if (tStop < This->StartOfFile)
        return S_OK;

    if (tStart < This->StartOfFile)
        offset_src = BYTES_FROM_MEDIATIME(This->StartOfFile - tStart);

    while (bMoreData)
    {
        if (!This->pCurrentSample)
        {
            /* cache media sample until it is ready to be dispatched
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
                REFERENCE_TIME tAviStart, tAviStop, tOffset;

                /* FIXME: hack */
                if (pOutputPin->dwSamplesProcessed == 0) {
                    IMediaSample_SetDiscontinuity(This->pCurrentSample, TRUE);
	        }
                IMediaSample_SetSyncPoint(This->pCurrentSample, TRUE);
                pOutputPin->dwSamplesProcessed++;

                tOffset = MEDIATIME_FROM_BYTES(offset_src + chunk_remaining_bytes - IMediaSample_GetActualDataLength(This->pCurrentSample));
                tAviStart = bytepos_to_duration(This, tStart + tOffset);

                tOffset += MEDIATIME_FROM_BYTES(IMediaSample_GetActualDataLength(This->pCurrentSample));
                tAviStop = bytepos_to_duration(This, tStart + tOffset);

                IMediaSample_SetTime(This->pCurrentSample, &tAviStart, &tAviStop);

                hr = OutputPin_SendSample(&pOutputPin->pin, This->pCurrentSample);
                if (hr != S_OK && hr != VFW_E_NOT_CONNECTED && hr != S_FALSE)
                    ERR("Error sending sample (%x)\n", hr);
            }

            if (This->pCurrentSample && hr != S_FALSE)
            {
                IMediaSample_Release(This->pCurrentSample);
                This->pCurrentSample = NULL;
            }
            if (hr == S_FALSE)
            {
                /* Break out */
                offset_src += chunk_remaining_bytes;
                This->Parser.pInputPin->rtCurrent -= BYTES_FROM_MEDIATIME(cbSrcStream - offset_src);
                hr = S_OK;
                break;
            }

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

    if (tStop >= This->EndOfFile || (bytepos_to_duration(This, tStop) >= This->Parser.mediaSeeking.llStop))
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

static HRESULT WAVEParserImpl_seek(IBaseFilter *iface)
{
    WAVEParserImpl *This = (WAVEParserImpl *)iface;
    PullPin *pPin = This->Parser.pInputPin;
    IPin *victim = NULL;
    LONGLONG newpos, curpos, endpos, bytepos;

    newpos = This->Parser.mediaSeeking.llCurrent;
    curpos = bytepos_to_duration(This, pPin->rtCurrent);
    endpos = bytepos_to_duration(This, This->EndOfFile);
    bytepos = duration_to_bytepos(This, newpos);

    if (newpos > endpos)
    {
        WARN("Requesting position %x%08x beyond end of stream %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(endpos>>32), (DWORD)endpos);
        return E_INVALIDARG;
    }

    if (curpos/1000000 == newpos/1000000)
    {
        TRACE("Requesting position %x%08x same as current position %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(curpos>>32), (DWORD)curpos);
        return S_OK;
    }

    TRACE("Moving sound to %08u bytes!\n", (DWORD)BYTES_FROM_MEDIATIME(bytepos));

    EnterCriticalSection(&pPin->thread_lock);
    IPin_BeginFlush((IPin *)pPin);

    /* Make sure this is done while stopped, BeginFlush takes care of this */
    EnterCriticalSection(&This->Parser.csFilter);
    IPin_ConnectedTo(This->Parser.ppPins[1], &victim);
    if (victim)
    {
        IPin_NewSegment(victim, newpos, endpos, pPin->dRate);
        IPin_Release(victim);
    }

    pPin->rtStart = pPin->rtCurrent = bytepos;
    LeaveCriticalSection(&This->Parser.csFilter);

    TRACE("Done flushing\n");
    IPin_EndFlush((IPin *)pPin);
    LeaveCriticalSection(&pPin->thread_lock);

    return S_OK;
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
    WAVEParserImpl * pWAVEParser = (WAVEParserImpl *)This->pin.pinInfo.pFilter;
    LONGLONG length, avail;

    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)This;
    lstrcpynW(piOutput.achName, wcsOutputPinName, sizeof(piOutput.achName) / sizeof(piOutput.achName[0]));
    
    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    pos += sizeof(list);

   if (list.fcc != FOURCC_RIFF)
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

    amt.majortype = MEDIATYPE_Audio;
    amt.formattype = FORMAT_WaveFormatEx;
    amt.cbFormat = chunk.cb;
    amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
    amt.pUnk = NULL;
    hr = IAsyncReader_SyncRead(This->pReader, pos, amt.cbFormat, amt.pbFormat);
    amt.subtype = MEDIATYPE_Audio;
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
    pWAVEParser->dwSampleSize = ((WAVEFORMATEX*)amt.pbFormat)->nBlockAlign;
    IAsyncReader_Length(This->pReader, &length, &avail);
    pWAVEParser->dwLength = length / (ULONGLONG)pWAVEParser->dwSampleSize;
    pWAVEParser->nSamplesPerSec = ((WAVEFORMATEX*)amt.pbFormat)->nSamplesPerSec;
    hr = Parser_AddPin(&(pWAVEParser->Parser), &piOutput, &props, &amt);
    CoTaskMemFree(amt.pbFormat);

    pWAVEParser->Parser.mediaSeeking.llCurrent = 0;
    pWAVEParser->Parser.mediaSeeking.llStop = pWAVEParser->Parser.mediaSeeking.llDuration = bytepos_to_duration(pWAVEParser, pWAVEParser->EndOfFile);
    TRACE("Duration: %lld seconds\n", pWAVEParser->Parser.mediaSeeking.llDuration / (LONGLONG)10000000);

    This->rtStop = pWAVEParser->EndOfFile;
    This->rtStart = pWAVEParser->StartOfFile;

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

static HRESULT WAVEParser_disconnect(LPVOID iface)
{
    /* TODO: Find and plug memory leaks */
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

    hr = Parser_Create(&(This->Parser), &CLSID_WAVEParser, WAVEParser_Sample, WAVEParser_QueryAccept, WAVEParser_InputPin_PreConnect, WAVEParser_Cleanup, WAVEParser_disconnect, NULL, WAVEParserImpl_seek, NULL);

    if (FAILED(hr))
        return hr;

    *ppv = (LPVOID)This;

    return hr;
}
