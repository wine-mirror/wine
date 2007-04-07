/*
 * AVI Splitter Filter
 *
 * Copyright 2003 Robert Shearman
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
/* FIXME:
 * - we don't do anything with indices yet (we could use them when seeking)
 * - we don't support multiple RIFF sections (i.e. large AVI files > 2Gb)
 */

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "aviriff.h"
#include "mmreg.h"
#include "vfwmsgs.h"
#include "amvideo.h"

#include "fourcc.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include <math.h>
#include <assert.h>

#include "parser.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct AVISplitterImpl
{
    ParserImpl Parser;
    IMediaSample * pCurrentSample;
    RIFFCHUNK CurrentChunk;
    LONGLONG CurrentChunkOffset; /* in media time */
    LONGLONG EndOfFile;
    AVIMAINHEADER AviHeader;
} AVISplitterImpl;

static HRESULT AVISplitter_NextChunk(LONGLONG * pllCurrentChunkOffset, RIFFCHUNK * pCurrentChunk, const REFERENCE_TIME * tStart, const REFERENCE_TIME * tStop, const BYTE * pbSrcStream, int inner)
{
    if (inner)
        *pllCurrentChunkOffset += MEDIATIME_FROM_BYTES(sizeof(RIFFLIST));
    else
        *pllCurrentChunkOffset += MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK) + RIFFROUND(pCurrentChunk->cb));
    
    if (*pllCurrentChunkOffset >= *tStop)
        return S_FALSE; /* no more data - we couldn't even get the next chunk header! */
    else if (*pllCurrentChunkOffset + MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK)) >= *tStop)
    {
        memcpy(pCurrentChunk, pbSrcStream + (DWORD)BYTES_FROM_MEDIATIME(*pllCurrentChunkOffset - *tStart), (DWORD)BYTES_FROM_MEDIATIME(*tStop - *pllCurrentChunkOffset));
        return S_FALSE; /* no more data */
    }
    else
        memcpy(pCurrentChunk, pbSrcStream + (DWORD)BYTES_FROM_MEDIATIME(*pllCurrentChunkOffset - *tStart), sizeof(RIFFCHUNK));

    return S_OK;
}

static HRESULT AVISplitter_Sample(LPVOID iface, IMediaSample * pSample)
{
    AVISplitterImpl *This = (AVISplitterImpl *)iface;
    LPBYTE pbSrcStream = NULL;
    long cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;
    BOOL bMoreData = TRUE;

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);

    /* trace removed for performance reasons */
    /* TRACE("(%p)\n", pSample); */

    assert(BYTES_FROM_MEDIATIME(tStop - tStart) == cbSrcStream);

    if (This->CurrentChunkOffset <= tStart && This->CurrentChunkOffset + MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK)) > tStart)
    {
        DWORD offset = (DWORD)BYTES_FROM_MEDIATIME(tStart - This->CurrentChunkOffset);
        assert(offset <= sizeof(RIFFCHUNK));
        memcpy((BYTE *)&This->CurrentChunk + offset, pbSrcStream, sizeof(RIFFCHUNK) - offset);
    }
    else if (This->CurrentChunkOffset > tStart)
    {
        DWORD offset = (DWORD)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset - tStart);
        if (offset >= (DWORD)cbSrcStream)
        {
            FIXME("large offset\n");
            hr = S_OK;
            goto skip;
        }

        memcpy(&This->CurrentChunk, pbSrcStream + offset, sizeof(RIFFCHUNK));
    }

    assert(This->CurrentChunkOffset + MEDIATIME_FROM_BYTES(sizeof(RIFFCHUNK)) < tStop);

    while (bMoreData)
    {
        BYTE * pbDstStream;
        long cbDstStream;
        long chunk_remaining_bytes = 0;
        long offset_src;
        WORD streamId;
        Parser_OutputPin * pOutputPin;
        BOOL bSyncPoint = TRUE;

        if (This->CurrentChunkOffset >= tStart)
            offset_src = (long)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset - tStart) + sizeof(RIFFCHUNK);
        else
            offset_src = 0;

        switch (This->CurrentChunk.fcc)
        {
        case ckidJUNK:
        case aviFCC('i','d','x','1'): /* Index is not handled */
            /* silently ignore */
            if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream, FALSE))
                bMoreData = FALSE;
            continue;
        case ckidLIST:
	    /* We only handle the 'rec ' list which contains the stream data */
	    if ((*(DWORD*)(pbSrcStream + BYTES_FROM_MEDIATIME(This->CurrentChunkOffset-tStart) + sizeof(RIFFCHUNK))) == aviFCC('r','e','c',' '))
	    {
		/* FIXME: We only advanced to the first chunk inside the list without keeping track that we are in it.
		 *        This is not clean and the parser should be improved for that but it is enough for most AVI files. */
                if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream, TRUE))
                {
                    bMoreData = FALSE;
                    continue;
                }
		This->CurrentChunk = *(RIFFCHUNK*) (pbSrcStream + BYTES_FROM_MEDIATIME(This->CurrentChunkOffset-tStart));
            	offset_src = (long)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset - tStart) + sizeof(RIFFCHUNK);
	        break;
	    }
	    else if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream, FALSE))
                bMoreData = FALSE;
            continue;
        default:
            break;
#if 0 /* According to the AVI specs, a stream data chunk should be ABXX where AB is the stream number and X means don't care */
            switch (TWOCCFromFOURCC(This->CurrentChunk.fcc))
            {
            case cktypeDIBcompressed:
                bSyncPoint = FALSE;
                /* fall-through */
            case cktypeDIBbits:
                /* FIXME: check that pin is of type video */
                break;
            case cktypeWAVEbytes:
                /* FIXME: check that pin is of type audio */
                break;
            case cktypePALchange:
                FIXME("handle palette change\n");
                break;
            default:
                FIXME("Skipping unknown chunk type: %s at file offset 0x%x\n", debugstr_an((LPSTR)&This->CurrentChunk.fcc, 4), (DWORD)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset));
                if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream, FALSE))
                    bMoreData = FALSE;
                continue;
            }
#endif
        }

        streamId = StreamFromFOURCC(This->CurrentChunk.fcc);

        if (streamId > This->Parser.cStreams)
        {
            ERR("Corrupted AVI file (contains stream id %d, but supposed to only have %d streams)\n", streamId, This->Parser.cStreams);
            hr = E_FAIL;
            break;
        }

        pOutputPin = (Parser_OutputPin *)This->Parser.ppPins[streamId + 1];

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
                TRACE("Skipping sending sample for stream %02d due to error (%x)\n", streamId, hr);
                This->pCurrentSample = NULL;
                if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream, FALSE))
                    bMoreData = FALSE;
                continue;
            }
        }

        hr = IMediaSample_GetPointer(This->pCurrentSample, &pbDstStream);

        if (SUCCEEDED(hr))
        {
            cbDstStream = IMediaSample_GetSize(This->pCurrentSample);

            chunk_remaining_bytes = (long)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset + MEDIATIME_FROM_BYTES(This->CurrentChunk.cb + sizeof(RIFFCHUNK)) - tStart) - offset_src;
        
            assert(chunk_remaining_bytes >= 0);
            assert(chunk_remaining_bytes <= cbDstStream - IMediaSample_GetActualDataLength(This->pCurrentSample));

            /* trace removed for performance reasons */
/*          TRACE("chunk_remaining_bytes: 0x%x, cbSrcStream: 0x%x, offset_src: 0x%x\n", chunk_remaining_bytes, cbSrcStream, offset_src); */
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
                if (pOutputPin->dwSamplesProcessed == 0)
                    IMediaSample_SetDiscontinuity(This->pCurrentSample, TRUE);

                IMediaSample_SetSyncPoint(This->pCurrentSample, bSyncPoint);

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

            if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream, FALSE))
                bMoreData = FALSE;
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
    }

skip:
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

static HRESULT AVISplitter_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    if (IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream) && IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_Avi))
        return S_OK;
    return S_FALSE;
}

static HRESULT AVISplitter_ProcessStreamList(AVISplitterImpl * This, const BYTE * pData, DWORD cb)
{
    PIN_INFO piOutput;
    const RIFFCHUNK * pChunk;
    HRESULT hr;
    AM_MEDIA_TYPE amt;
    float fSamplesPerSec = 0.0f;
    DWORD dwSampleSize = 0;
    DWORD dwLength = 0;
    ALLOCATOR_PROPERTIES props;
    static const WCHAR wszStreamTemplate[] = {'S','t','r','e','a','m',' ','%','0','2','d',0};

    props.cbAlign = 1;
    props.cbPrefix = 0;
    props.cbBuffer = 0x20000;
    props.cBuffers = 2;
    
    ZeroMemory(&amt, sizeof(amt));
    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)This;
    wsprintfW(piOutput.achName, wszStreamTemplate, This->Parser.cStreams);

    for (pChunk = (const RIFFCHUNK *)pData; 
         ((const BYTE *)pChunk >= pData) && ((const BYTE *)pChunk + sizeof(RIFFCHUNK) < pData + cb) && (pChunk->cb > 0); 
         pChunk = (const RIFFCHUNK *)((const BYTE*)pChunk + sizeof(RIFFCHUNK) + pChunk->cb)     
        )
    {
        switch (pChunk->fcc)
        {
        case ckidSTREAMHEADER:
            {
                const AVISTREAMHEADER * pStrHdr = (const AVISTREAMHEADER *)pChunk;
                TRACE("processing stream header\n");

                fSamplesPerSec = (float)pStrHdr->dwRate / (float)pStrHdr->dwScale;

                switch (pStrHdr->fccType)
                {
                case streamtypeVIDEO:
                    memcpy(&amt.formattype, &FORMAT_VideoInfo, sizeof(GUID));
                    amt.pbFormat = NULL;
                    amt.cbFormat = 0;
                    break;
                case streamtypeAUDIO:
                    memcpy(&amt.formattype, &FORMAT_WaveFormatEx, sizeof(GUID));
                    break;
                default:
                    memcpy(&amt.formattype, &FORMAT_None, sizeof(GUID));
                }
                memcpy(&amt.majortype, &MEDIATYPE_Video, sizeof(GUID));
                amt.majortype.Data1 = pStrHdr->fccType;
                memcpy(&amt.subtype, &MEDIATYPE_Video, sizeof(GUID));
                amt.subtype.Data1 = pStrHdr->fccHandler;
                TRACE("Subtype FCC: %.04s\n", (LPCSTR)&pStrHdr->fccHandler);
                amt.lSampleSize = pStrHdr->dwSampleSize;
                amt.bFixedSizeSamples = (amt.lSampleSize != 0);

                /* FIXME: Is this right? */
                if (!amt.lSampleSize)
                {
                    amt.lSampleSize = 1;
                    dwSampleSize = 1;
                }

                amt.bTemporalCompression = IsEqualGUID(&amt.majortype, &MEDIATYPE_Video); /* FIXME? */
                dwSampleSize = pStrHdr->dwSampleSize;
                dwLength = pStrHdr->dwLength;
                if (!dwLength)
                    dwLength = This->AviHeader.dwTotalFrames;

                if (pStrHdr->dwSuggestedBufferSize)
                    props.cbBuffer = pStrHdr->dwSuggestedBufferSize;

                break;
            }
        case ckidSTREAMFORMAT:
            TRACE("processing stream format data\n");
            if (IsEqualIID(&amt.formattype, &FORMAT_VideoInfo))
            {
                VIDEOINFOHEADER * pvi;
                /* biCompression member appears to override the value in the stream header.
                 * i.e. the stream header can say something completely contradictory to what
                 * is in the BITMAPINFOHEADER! */
                if (pChunk->cb < sizeof(BITMAPINFOHEADER))
                {
                    ERR("Not enough bytes for BITMAPINFOHEADER\n");
                    return E_FAIL;
                }
                amt.cbFormat = sizeof(VIDEOINFOHEADER) - sizeof(BITMAPINFOHEADER) + pChunk->cb;
                amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
                ZeroMemory(amt.pbFormat, amt.cbFormat);
                pvi = (VIDEOINFOHEADER *)amt.pbFormat;
                pvi->AvgTimePerFrame = (LONGLONG)(10000000.0 / fSamplesPerSec);
                CopyMemory(&pvi->bmiHeader, (const BYTE *)(pChunk + 1), pChunk->cb);
                if (pvi->bmiHeader.biCompression)
                    amt.subtype.Data1 = pvi->bmiHeader.biCompression;
            }
            else
            {
                amt.cbFormat = pChunk->cb;
                amt.pbFormat = CoTaskMemAlloc(amt.cbFormat);
                CopyMemory(amt.pbFormat, (const BYTE *)(pChunk + 1), amt.cbFormat);
            }
            break;
        case ckidSTREAMNAME:
            TRACE("processing stream name\n");
            /* FIXME: this doesn't exactly match native version (we omit the "##)" prefix), but hey... */
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)(pChunk + 1), pChunk->cb, piOutput.achName, sizeof(piOutput.achName) / sizeof(piOutput.achName[0]));
            break;
        case ckidSTREAMHANDLERDATA:
            FIXME("process stream handler data\n");
            break;
        case ckidJUNK:
            TRACE("JUNK chunk ignored\n");
            break;
        default:
            FIXME("unknown chunk type \"%.04s\" ignored\n", (LPCSTR)&pChunk->fcc);
        }
    }

    if (IsEqualGUID(&amt.formattype, &FORMAT_WaveFormatEx))
    {
        memcpy(&amt.subtype, &MEDIATYPE_Video, sizeof(GUID));
        amt.subtype.Data1 = ((WAVEFORMATEX *)amt.pbFormat)->wFormatTag;
    }

    dump_AM_MEDIA_TYPE(&amt);
    TRACE("fSamplesPerSec = %f\n", (double)fSamplesPerSec);
    TRACE("dwSampleSize = %x\n", dwSampleSize);
    TRACE("dwLength = %x\n", dwLength);

    hr = Parser_AddPin(&(This->Parser), &piOutput, &props, &amt, fSamplesPerSec, dwSampleSize, dwLength);

    return hr;
}

/* FIXME: fix leaks on failure here */
static HRESULT AVISplitter_InputPin_PreConnect(IPin * iface, IPin * pConnectPin)
{
    PullPin *This = (PullPin *)iface;
    HRESULT hr;
    RIFFLIST list;
    LONGLONG pos = 0; /* in bytes */
    BYTE * pBuffer;
    RIFFCHUNK * pCurrentChunk;
    AVISplitterImpl * pAviSplit = (AVISplitterImpl *)This->pin.pinInfo.pFilter;

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    pos += sizeof(list);

    if (list.fcc != ckidRIFF)
    {
        ERR("Input stream not a RIFF file\n");
        return E_FAIL;
    }
    if (list.fccListType != ckidAVI)
    {
        ERR("Input stream not an AVI RIFF file\n");
        return E_FAIL;
    }

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    if (list.fcc != ckidLIST)
    {
        ERR("Expected LIST chunk, but got %.04s\n", (LPSTR)&list.fcc);
        return E_FAIL;
    }
    if (list.fccListType != ckidHEADERLIST)
    {
        ERR("Header list expected. Got: %.04s\n", (LPSTR)&list.fccListType);
        return E_FAIL;
    }

    pBuffer = HeapAlloc(GetProcessHeap(), 0, list.cb - sizeof(RIFFLIST) + sizeof(RIFFCHUNK));
    hr = IAsyncReader_SyncRead(This->pReader, pos + sizeof(list), list.cb - sizeof(RIFFLIST) + sizeof(RIFFCHUNK), pBuffer);

    pAviSplit->AviHeader.cb = 0;

    for (pCurrentChunk = (RIFFCHUNK *)pBuffer; (BYTE *)pCurrentChunk + sizeof(*pCurrentChunk) < pBuffer + list.cb; pCurrentChunk = (RIFFCHUNK *)(((BYTE *)pCurrentChunk) + sizeof(*pCurrentChunk) + pCurrentChunk->cb))
    {
        RIFFLIST * pList;

        switch (pCurrentChunk->fcc)
        {
        case ckidMAINAVIHEADER:
            /* AVIMAINHEADER includes the structure that is pCurrentChunk at the moment */
            memcpy(&pAviSplit->AviHeader, pCurrentChunk, sizeof(pAviSplit->AviHeader));
            break;
        case ckidLIST:
            pList = (RIFFLIST *)pCurrentChunk;
            switch (pList->fccListType)
            {
            case ckidSTREAMLIST:
                hr = AVISplitter_ProcessStreamList(pAviSplit, (BYTE *)pCurrentChunk + sizeof(RIFFLIST), pCurrentChunk->cb + sizeof(RIFFCHUNK) - sizeof(RIFFLIST));
                break;
            case ckidODML:
                FIXME("process ODML header\n");
                break;
            }
            break;
        case ckidJUNK:
            /* ignore */
            break;
        default:
            FIXME("unrecognised header list type: %.04s\n", (LPSTR)&pCurrentChunk->fcc);
        }
    }
    HeapFree(GetProcessHeap(), 0, pBuffer);

    if (pAviSplit->AviHeader.cb != sizeof(pAviSplit->AviHeader) - sizeof(RIFFCHUNK))
    {
        ERR("Avi Header wrong size!\n");
        return E_FAIL;
    }

    pos += sizeof(RIFFCHUNK) + list.cb;
    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);

    while (list.fcc == ckidJUNK || (list.fcc == ckidLIST && list.fccListType == ckidINFO))
    {
        pos += sizeof(RIFFCHUNK) + list.cb;
        hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    }

    if (list.fcc != ckidLIST)
    {
        ERR("Expected LIST, but got %.04s\n", (LPSTR)&list.fcc);
        return E_FAIL;
    }
    if (list.fccListType != ckidAVIMOVIE)
    {
        ERR("Expected AVI movie list, but got %.04s\n", (LPSTR)&list.fccListType);
        return E_FAIL;
    }

    if (hr == S_OK)
    {
        pAviSplit->CurrentChunkOffset = MEDIATIME_FROM_BYTES(pos + sizeof(RIFFLIST));
        pAviSplit->EndOfFile = MEDIATIME_FROM_BYTES(pos + list.cb + sizeof(RIFFLIST));
        hr = IAsyncReader_SyncRead(This->pReader, BYTES_FROM_MEDIATIME(pAviSplit->CurrentChunkOffset), sizeof(pAviSplit->CurrentChunk), (BYTE *)&pAviSplit->CurrentChunk);
    }

    if (hr != S_OK)
        return E_FAIL;

    TRACE("AVI File ok\n");

    return hr;
}

static HRESULT AVISplitter_Cleanup(LPVOID iface)
{
    AVISplitterImpl *This = (AVISplitterImpl*)iface;

    TRACE("(%p)->()\n", This);

    if (This->pCurrentSample)
        IMediaSample_Release(This->pCurrentSample);
    This->pCurrentSample = NULL;

    return S_OK;
}

HRESULT AVISplitter_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    AVISplitterImpl * This;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Note: This memory is managed by the transform filter once created */
    This = CoTaskMemAlloc(sizeof(AVISplitterImpl));

    This->pCurrentSample = NULL;

    hr = Parser_Create(&(This->Parser), &CLSID_AviSplitter, AVISplitter_Sample, AVISplitter_QueryAccept, AVISplitter_InputPin_PreConnect, AVISplitter_Cleanup);

    if (FAILED(hr))
        return hr;

    *ppv = (LPVOID)This;

    return hr;
}
