/*
 * AVI Splitter Filter
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2004-2005 Christian Costa
 * Copyright 2008 Maarten Lankhorst
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
 * - Memory leaks, and lots of them
 */

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "vfw.h"
#include "aviriff.h"
#include "vfwmsgs.h"
#include "amvideo.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include <math.h>
#include <assert.h>

#include "parser.h"

#define TWOCCFromFOURCC(fcc) HIWORD(fcc)

/* four character codes used in AVI files */
#define ckidINFO       mmioFOURCC('I','N','F','O')
#define ckidREC        mmioFOURCC('R','E','C',' ')

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct StreamData
{
    DWORD dwSampleSize;
    FLOAT fSamplesPerSec;
    DWORD dwLength;

    AVISTREAMHEADER streamheader;
    DWORD entries;
    AVISTDINDEX **stdindex;
    DWORD frames;
} StreamData;

typedef struct AVISplitterImpl
{
    ParserImpl Parser;
    IMediaSample * pCurrentSample;
    RIFFCHUNK CurrentChunk;
    LONGLONG CurrentChunkOffset; /* in media time */
    LONGLONG EndOfFile;
    AVIMAINHEADER AviHeader;
    AVIEXTHEADER ExtHeader;

    /* TODO: Handle old style index, probably by creating an opendml style new index from it for within StreamData */
    AVIOLDINDEX *oldindex;
    DWORD offset;

    StreamData *streams;
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

static HRESULT AVISplitter_Sample(LPVOID iface, IMediaSample * pSample, DWORD_PTR cookie)
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
        BYTE *fcc = (BYTE *)&This->CurrentChunk.fcc;

        if (This->CurrentChunkOffset >= tStart)
            offset_src = (long)BYTES_FROM_MEDIATIME(This->CurrentChunkOffset - tStart) + sizeof(RIFFCHUNK);
        else
            offset_src = 0;

        switch (This->CurrentChunk.fcc)
        {
        case ckidAVIOLDINDEX: /* Should not be popping up here! */
            ERR("There should be no index in the stream data!\n");
        case ckidAVIPADDING:
            /* silently ignore */
            if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream, FALSE))
                bMoreData = FALSE;
            continue;
        case FOURCC_LIST:
	    /* We only handle the 'rec ' list which contains the stream data */
	    if ((*(DWORD*)(pbSrcStream + BYTES_FROM_MEDIATIME(This->CurrentChunkOffset-tStart) + sizeof(RIFFCHUNK))) == ckidREC)
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

        if (fcc[0] == 'i' && fcc[1] == 'x')
        {
            if (S_FALSE == AVISplitter_NextChunk(&This->CurrentChunkOffset, &This->CurrentChunk, &tStart, &tStop, pbSrcStream, FALSE))
                bMoreData = FALSE;
            continue;
        }

        streamId = StreamFromFOURCC(This->CurrentChunk.fcc);

        if (streamId > This->Parser.cStreams)
        {
            ERR("Corrupted AVI file (contains stream id (%s) %d, but supposed to only have %d streams)\n", debugstr_an((char *)&This->CurrentChunk.fcc, 4), streamId, This->Parser.cStreams);
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
                StreamData *stream = This->streams + streamId;

                /* FIXME: hack */
                if (pOutputPin->dwSamplesProcessed == 0)
                    IMediaSample_SetDiscontinuity(This->pCurrentSample, TRUE);

                IMediaSample_SetSyncPoint(This->pCurrentSample, bSyncPoint);

                pOutputPin->dwSamplesProcessed++;

                if (stream->dwSampleSize)
                    tAviStart = (LONGLONG)ceil(10000000.0 * (float)(pOutputPin->dwSamplesProcessed - 1) * (float)IMediaSample_GetActualDataLength(This->pCurrentSample) / ((float)stream->dwSampleSize * stream->fSamplesPerSec));
                else
                    tAviStart = (LONGLONG)ceil(10000000.0 * (float)(pOutputPin->dwSamplesProcessed - 1) / (float)stream->fSamplesPerSec);
                if (stream->dwSampleSize)
                    tAviStop = (LONGLONG)ceil(10000000.0 * (float)pOutputPin->dwSamplesProcessed * (float)IMediaSample_GetActualDataLength(This->pCurrentSample) / ((float)stream->dwSampleSize * stream->fSamplesPerSec));
                else
                    tAviStop = (LONGLONG)ceil(10000000.0 * (float)pOutputPin->dwSamplesProcessed / (float)stream->fSamplesPerSec);

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

static HRESULT AVISplitter_ProcessIndex(AVISplitterImpl *This, AVISTDINDEX **index, LONGLONG qwOffset, DWORD cb)
{
    AVISTDINDEX *pIndex;
    int x;
    long rest;

    *index = NULL;
    if (cb < sizeof(AVISTDINDEX))
    {
        FIXME("size %u too small\n", cb);
        return E_INVALIDARG;
    }

    pIndex = CoTaskMemAlloc(cb);
    if (!pIndex)
        return E_OUTOFMEMORY;

    IAsyncReader_SyncRead(((PullPin *)This->Parser.ppPins[0])->pReader, qwOffset, cb, (BYTE *)pIndex);
    pIndex = CoTaskMemRealloc(pIndex, pIndex->cb);
    if (!pIndex)
        return E_OUTOFMEMORY;

    IAsyncReader_SyncRead(((PullPin *)This->Parser.ppPins[0])->pReader, qwOffset, pIndex->cb, (BYTE *)pIndex);
    rest = pIndex->cb - sizeof(AVISUPERINDEX) + sizeof(RIFFCHUNK) + sizeof(pIndex->aIndex[0]) * ANYSIZE_ARRAY;

    TRACE("wLongsPerEntry: %hd\n", pIndex->wLongsPerEntry);
    TRACE("bIndexSubType: %hd\n", pIndex->bIndexSubType);
    TRACE("bIndexType: %hd\n", pIndex->bIndexType);
    TRACE("nEntriesInUse: %u\n", pIndex->nEntriesInUse);
    TRACE("dwChunkId: %.4s\n", (char *)&pIndex->dwChunkId);
    TRACE("qwBaseOffset: %x%08x\n", (DWORD)(pIndex->qwBaseOffset >> 32), (DWORD)pIndex->qwBaseOffset);
    TRACE("dwReserved_3: %u\n", pIndex->dwReserved_3);

    if (pIndex->bIndexType != AVI_INDEX_OF_CHUNKS
        || pIndex->wLongsPerEntry != 2
        || rest < (pIndex->nEntriesInUse * sizeof(DWORD) * pIndex->wLongsPerEntry)
        || (pIndex->bIndexSubType != AVI_INDEX_SUB_DEFAULT))
    {
        FIXME("Invalid index chunk encountered\n");
        return E_INVALIDARG;
    }

    for (x = 0; x < pIndex->nEntriesInUse; ++x)
    {
        BOOL keyframe = !(pIndex->aIndex[x].dwOffset >> 31);
        DWORDLONG offset = pIndex->qwBaseOffset + (pIndex->aIndex[x].dwOffset & ~(1<<31));
        TRACE("dwOffset: %x%08x\n", (DWORD)(offset >> 32), (DWORD)offset);
        TRACE("dwSize: %u\n", pIndex->aIndex[x].dwSize);
        TRACE("Frame is a keyframe: %s\n", keyframe ? "yes" : "no");
    }

    *index = pIndex;
    return S_OK;
}

static HRESULT AVISplitter_ProcessOldIndex(AVISplitterImpl *This)
{
    ULONGLONG mov_pos = BYTES_FROM_MEDIATIME(This->CurrentChunkOffset) - sizeof(DWORD);
    AVIOLDINDEX *pAviOldIndex = This->oldindex;
    int relative = -1;
    int x;

    for (x = 0; x < pAviOldIndex->cb / sizeof(pAviOldIndex->aIndex[0]); ++x)
    {
        DWORD temp, temp2 = 0, offset, chunkid;
        PullPin *pin = This->Parser.pInputPin;

        offset = pAviOldIndex->aIndex[x].dwOffset;
        chunkid = pAviOldIndex->aIndex[x].dwChunkId;

        /* Only scan once, or else this will take too long */
        if (relative == -1)
        {
            IAsyncReader_SyncRead(pin->pReader, offset, sizeof(DWORD), (BYTE *)&temp);
            relative = (chunkid != temp);

            TRACE("dwChunkId: %.4s\n", (char *)&chunkid);
            if (chunkid == mmioFOURCC('7','F','x','x')
                && ((char *)&temp)[0] == 'i' && ((char *)&temp)[1] == 'x')
                relative = FALSE;

            if (relative)
            {
                if (offset + mov_pos < BYTES_FROM_MEDIATIME(This->EndOfFile))
                    IAsyncReader_SyncRead(pin->pReader, offset + mov_pos, sizeof(DWORD), (BYTE *)&temp2);

                if (chunkid == mmioFOURCC('7','F','x','x')
                    && ((char *)&temp2)[0] == 'i' && ((char *)&temp2)[1] == 'x')
                {
                    /* Do nothing, all is great */
                }
                else if (temp2 != chunkid)
                {
                    ERR("Faulty index or bug in handling: Wanted FCC: %s, Abs FCC: %s (@ %x), Rel FCC: %s (@ %.0x%08x)\n",
                        debugstr_an((char *)&chunkid, 4), debugstr_an((char *)&temp, 4), offset,
                        debugstr_an((char *)&temp2, 4), (DWORD)((mov_pos + offset) >> 32), (DWORD)(mov_pos + offset));
                    relative = -1;
                }
                else
                    TRACE("Scanned dwChunkId: %s\n", debugstr_an((char *)&temp2, 4));
            }
            else if (!relative)
               TRACE("Scanned dwChunkId: %s\n", debugstr_an((char *)&temp, 4));
            TRACE("dwFlags: %08x\n", pAviOldIndex->aIndex[x].dwFlags);
            TRACE("dwOffset (%s): %08x\n", relative ? "relative" : "absolute", offset);
            TRACE("dwSize: %08x\n", pAviOldIndex->aIndex[x].dwSize);
        }
        else break;
    }

    if (relative == -1)
    {
        FIXME("Dropping index: no idea whether it is relative or absolute\n");
        CoTaskMemFree(This->oldindex);
        This->oldindex = NULL;
    }
    else if (!relative)
        This->offset = 0;
    else
        This->offset = (DWORD)mov_pos;

    return S_OK;
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
    StreamData *stream;

    AVISTDINDEX **stdindex = NULL;
    DWORD nstdindex = 0;

    props.cbAlign = 1;
    props.cbPrefix = 0;
    props.cbBuffer = 0x20000;
    props.cBuffers = 2;

    ZeroMemory(&amt, sizeof(amt));
    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)This;
    wsprintfW(piOutput.achName, wszStreamTemplate, This->Parser.cStreams);
    This->streams = CoTaskMemRealloc(This->streams, sizeof(StreamData) * (This->Parser.cStreams+1));
    stream = This->streams + This->Parser.cStreams;

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
                stream->streamheader = *pStrHdr;

                fSamplesPerSec = (float)pStrHdr->dwRate / (float)pStrHdr->dwScale;
                CoTaskMemFree(amt.pbFormat);
                amt.pbFormat = NULL;
                amt.cbFormat = 0;

                switch (pStrHdr->fccType)
                {
                case streamtypeVIDEO:
                    amt.formattype = FORMAT_VideoInfo;
                    break;
                case streamtypeAUDIO:
                    amt.formattype = FORMAT_WaveFormatEx;
                    break;
                default:
                    FIXME("fccType %.4s not handled yet\n", (char *)&pStrHdr->fccType);
                    amt.formattype = FORMAT_None;
                }
                amt.majortype = MEDIATYPE_Video;
                amt.majortype.Data1 = pStrHdr->fccType;
                amt.subtype = MEDIATYPE_Video;
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
        case ckidAVIPADDING:
            TRACE("JUNK chunk ignored\n");
            break;
        case ckidAVISUPERINDEX:
        {
            const AVISUPERINDEX *pIndex = (const AVISUPERINDEX *)pChunk;
            int x;
            long rest = pIndex->cb - sizeof(AVISUPERINDEX) + sizeof(RIFFCHUNK) + sizeof(pIndex->aIndex[0]) * ANYSIZE_ARRAY;

            if (pIndex->cb < sizeof(AVISUPERINDEX) - sizeof(RIFFCHUNK))
            {
                FIXME("size %u\n", pIndex->cb);
                break;
            }

            if (nstdindex > 0)
            {
                ERR("Stream %d got more than 1 superindex?\n", This->Parser.cStreams);
                break;
            }

            TRACE("wLongsPerEntry: %hd\n", pIndex->wLongsPerEntry);
            TRACE("bIndexSubType: %hd\n", pIndex->bIndexSubType);
            TRACE("bIndexType: %hd\n", pIndex->bIndexType);
            TRACE("nEntriesInUse: %u\n", pIndex->nEntriesInUse);
            TRACE("dwChunkId: %.4s\n", (char *)&pIndex->dwChunkId);
            if (pIndex->dwReserved[0])
                TRACE("dwReserved[0]: %u\n", pIndex->dwReserved[0]);
            if (pIndex->dwReserved[2])
                TRACE("dwReserved[1]: %u\n", pIndex->dwReserved[1]);
            if (pIndex->dwReserved[2])
                TRACE("dwReserved[2]: %u\n", pIndex->dwReserved[2]);

            if (pIndex->bIndexType != AVI_INDEX_OF_INDEXES
                || pIndex->wLongsPerEntry != 4
                || rest < (pIndex->nEntriesInUse * sizeof(DWORD) * pIndex->wLongsPerEntry)
                || (pIndex->bIndexSubType != AVI_INDEX_SUB_2FIELD && pIndex->bIndexSubType != AVI_INDEX_SUB_DEFAULT))
            {
                FIXME("Invalid index chunk encountered\n");
                break;
            }

            for (x = 0; x < pIndex->nEntriesInUse; ++x)
            {
                TRACE("qwOffset: %x%08x\n", (DWORD)(pIndex->aIndex[x].qwOffset >> 32), (DWORD)pIndex->aIndex[x].qwOffset);
                TRACE("dwSize: %u\n", pIndex->aIndex[x].dwSize);
                TRACE("dwDuration: %u (unreliable)\n", pIndex->aIndex[x].dwDuration);

                ++nstdindex;
                stdindex = CoTaskMemRealloc(stdindex, sizeof(*stdindex) * nstdindex);
                AVISplitter_ProcessIndex(This, &stdindex[nstdindex-1], pIndex->aIndex[x].qwOffset, pIndex->aIndex[x].dwSize);
            }
            break;
        }
        default:
            FIXME("unknown chunk type \"%.04s\" ignored\n", (LPCSTR)&pChunk->fcc);
        }
    }

    if (IsEqualGUID(&amt.formattype, &FORMAT_WaveFormatEx))
    {
        amt.subtype = MEDIATYPE_Video;
        amt.subtype.Data1 = ((WAVEFORMATEX *)amt.pbFormat)->wFormatTag;
    }

    dump_AM_MEDIA_TYPE(&amt);
    TRACE("fSamplesPerSec = %f\n", (double)fSamplesPerSec);
    TRACE("dwSampleSize = %x\n", dwSampleSize);
    TRACE("dwLength = %x\n", dwLength);

    stream->fSamplesPerSec = fSamplesPerSec;
    stream->dwSampleSize = dwSampleSize;
    stream->dwLength = dwLength; /* TODO: Use this for mediaseeking */
    stream->entries = nstdindex;
    stream->stdindex = stdindex;

    hr = Parser_AddPin(&(This->Parser), &piOutput, &props, &amt);
    CoTaskMemFree(amt.pbFormat);

    return hr;
}

static HRESULT AVISplitter_ProcessODML(AVISplitterImpl * This, const BYTE * pData, DWORD cb)
{
    const RIFFCHUNK * pChunk;

    for (pChunk = (const RIFFCHUNK *)pData;
         ((const BYTE *)pChunk >= pData) && ((const BYTE *)pChunk + sizeof(RIFFCHUNK) < pData + cb) && (pChunk->cb > 0);
         pChunk = (const RIFFCHUNK *)((const BYTE*)pChunk + sizeof(RIFFCHUNK) + pChunk->cb)
        )
    {
        switch (pChunk->fcc)
        {
        case ckidAVIEXTHEADER:
            {
                int x;
                const AVIEXTHEADER * pExtHdr = (const AVIEXTHEADER *)pChunk;

                TRACE("processing extension header\n");
                if (pExtHdr->cb != sizeof(AVIEXTHEADER) - sizeof(RIFFCHUNK))
                {
                    FIXME("Size: %u\n", pExtHdr->cb);
                    break;
                }
                TRACE("dwGrandFrames: %u\n", pExtHdr->dwGrandFrames);
                for (x = 0; x < 61; ++x)
                    if (pExtHdr->dwFuture[x])
                        FIXME("dwFuture[%i] = %u (0x%08x)\n", x, pExtHdr->dwFuture[x], pExtHdr->dwFuture[x]);
                This->ExtHeader = *pExtHdr;
                break;
            }
        default:
            FIXME("unknown chunk type \"%.04s\" ignored\n", (LPCSTR)&pChunk->fcc);
        }
    }

    return S_OK;
}

static HRESULT AVISplitter_InitializeStreams(AVISplitterImpl *This)
{
    int x;

    if (This->oldindex)
    {
        DWORD nMax, n;

        for (x = 0; x < This->Parser.cStreams; ++x)
        {
            This->streams[x].frames = 0;
        }

        nMax = This->oldindex->cb / sizeof(This->oldindex->aIndex[0]);

        /* Ok, maybe this is more of an excercise to see if I interpret everything correctly or not, but that is useful for now. */
        for (n = 0; n < nMax; ++n)
        {
            DWORD streamId = StreamFromFOURCC(This->oldindex->aIndex[n].dwChunkId);
            if (streamId >= This->Parser.cStreams)
            {
                FIXME("Stream id %s ignored\n", debugstr_an((char*)&This->oldindex->aIndex[n].dwChunkId, 4));
                continue;
            }

            if (This->streams[streamId].streamheader.dwSampleSize)
                This->streams[streamId].frames += This->oldindex->aIndex[n].dwSize / This->streams[streamId].streamheader.dwSampleSize;
            else
                ++This->streams[streamId].frames;
        }

        for (x = 0; x < This->Parser.cStreams; ++x)
        {
            if ((DWORD)This->streams[x].frames != This->streams[x].streamheader.dwLength)
            {
                FIXME("stream %u: frames found: %u, frames meant to be found: %u\n", x, (DWORD)This->streams[x].frames, This->streams[x].streamheader.dwLength);
            }
        }

    }
    else if (!This->streams[0].entries)
    {
        for (x = 0; x < This->Parser.cStreams; ++x)
        {
            This->streams[x].frames = This->streams[x].streamheader.dwLength;
        }
    }

    /* Not much here yet */
    for (x = 0; x < This->Parser.cStreams; ++x)
    {
        StreamData *stream = This->streams + x;
        /* WOEI! */
        double fps;
        int y;
        DWORD64 frames = 0;

        fps = (double)stream->streamheader.dwRate / (float)stream->streamheader.dwScale;
        if (stream->stdindex)
        {
            for (y = 0; y < stream->entries; ++y)
            {
                frames += stream->stdindex[y]->nEntriesInUse;
            }
        }
        else frames = stream->frames;

        frames *= stream->streamheader.dwScale;
        /* Keep accuracy as high as possible for duration */
        This->Parser.mediaSeeking.llDuration = frames * 10000000;
        This->Parser.mediaSeeking.llDuration /= stream->streamheader.dwRate;
        This->Parser.mediaSeeking.llStop = This->Parser.mediaSeeking.llDuration;
        This->Parser.mediaSeeking.llCurrent = 0;

        frames /= stream->streamheader.dwRate;

        TRACE("fps: %f\n", fps);
        TRACE("Duration: %d days, %d hours, %d minutes and %d seconds\n", (DWORD)(frames / 86400),
        (DWORD)((frames % 86400) / 3600), (DWORD)((frames % 3600) / 60), (DWORD)(frames % 60));
    }

    return S_OK;
}

static HRESULT AVISplitter_Disconnect(LPVOID iface);

/* FIXME: fix leaks on failure here */
static HRESULT AVISplitter_InputPin_PreConnect(IPin * iface, IPin * pConnectPin, ALLOCATOR_PROPERTIES *props)
{
    PullPin *This = (PullPin *)iface;
    HRESULT hr;
    RIFFLIST list;
    LONGLONG pos = 0; /* in bytes */
    BYTE * pBuffer;
    RIFFCHUNK * pCurrentChunk;
    LONGLONG total, avail;
    int x;
    DWORD indexes;

    AVISplitterImpl * pAviSplit = (AVISplitterImpl *)This->pin.pinInfo.pFilter;

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    pos += sizeof(list);

    if (list.fcc != FOURCC_RIFF)
    {
        ERR("Input stream not a RIFF file\n");
        return E_FAIL;
    }
    if (list.fccListType != formtypeAVI)
    {
        ERR("Input stream not an AVI RIFF file\n");
        return E_FAIL;
    }

    hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    if (list.fcc != FOURCC_LIST)
    {
        ERR("Expected LIST chunk, but got %.04s\n", (LPSTR)&list.fcc);
        return E_FAIL;
    }
    if (list.fccListType != listtypeAVIHEADER)
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
        case FOURCC_LIST:
            pList = (RIFFLIST *)pCurrentChunk;
            switch (pList->fccListType)
            {
            case ckidSTREAMLIST:
                hr = AVISplitter_ProcessStreamList(pAviSplit, (BYTE *)pCurrentChunk + sizeof(RIFFLIST), pCurrentChunk->cb + sizeof(RIFFCHUNK) - sizeof(RIFFLIST));
                break;
            case ckidODML:
                hr = AVISplitter_ProcessODML(pAviSplit, (BYTE *)pCurrentChunk + sizeof(RIFFLIST), pCurrentChunk->cb + sizeof(RIFFCHUNK) - sizeof(RIFFLIST));
                break;
            }
            break;
        case ckidAVIPADDING:
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

    while (list.fcc == ckidAVIPADDING || (list.fcc == FOURCC_LIST && list.fccListType == ckidINFO))
    {
        pos += sizeof(RIFFCHUNK) + list.cb;

        hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
    }

    if (list.fcc != FOURCC_LIST)
    {
        ERR("Expected LIST, but got %.04s\n", (LPSTR)&list.fcc);
        return E_FAIL;
    }
    if (list.fccListType != listtypeAVIMOVIE)
    {
        ERR("Expected AVI movie list, but got %.04s\n", (LPSTR)&list.fccListType);
        return E_FAIL;
    }

    IAsyncReader_Length(This->pReader, &total, &avail);

    /* FIXME: AVIX files are extended beyond the FOURCC chunk "AVI ", and thus won't be played here,
     * once I get one of the files I'll try to fix it */
    if (hr == S_OK)
    {
        This->rtStart = pAviSplit->CurrentChunkOffset = MEDIATIME_FROM_BYTES(pos + sizeof(RIFFLIST));
        pos += list.cb + sizeof(RIFFCHUNK);

        pAviSplit->EndOfFile = This->rtStop = MEDIATIME_FROM_BYTES(pos);
        if (pos > total)
        {
            ERR("File smaller (%x%08x) then EndOfFile (%x%08x)\n", (DWORD)(total >> 32), (DWORD)total, (DWORD)(pAviSplit->EndOfFile >> 32), (DWORD)pAviSplit->EndOfFile);
            return E_FAIL;
        }

        hr = IAsyncReader_SyncRead(This->pReader, BYTES_FROM_MEDIATIME(pAviSplit->CurrentChunkOffset), sizeof(pAviSplit->CurrentChunk), (BYTE *)&pAviSplit->CurrentChunk);
    }

    /* Now peek into the idx1 index, if available */
    if (hr == S_OK && (total - pos) > sizeof(RIFFCHUNK))
    {
        memset(&list, 0, sizeof(list));

        hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(list), (BYTE *)&list);
        if (list.fcc == ckidAVIOLDINDEX)
        {
            pAviSplit->oldindex = CoTaskMemRealloc(pAviSplit->oldindex, list.cb + sizeof(RIFFCHUNK));
            if (pAviSplit->oldindex)
            {
                hr = IAsyncReader_SyncRead(This->pReader, pos, sizeof(RIFFCHUNK) + list.cb, (BYTE *)pAviSplit->oldindex);
                if (hr == S_OK)
                {
                    hr = AVISplitter_ProcessOldIndex(pAviSplit);
                }
                else
                {
                    CoTaskMemFree(pAviSplit->oldindex);
                    pAviSplit->oldindex = NULL;
                    hr = S_OK;
                }
            }
        }
    }

    indexes = 0;
    for (x = 0; x < pAviSplit->Parser.cStreams; ++x)
        if (pAviSplit->streams[x].entries)
            ++indexes;

    if (indexes)
    {
        CoTaskMemFree(pAviSplit->oldindex);
        pAviSplit->oldindex = NULL;
        if (indexes < pAviSplit->Parser.cStreams)
        {
            /* This error could possible be survived by switching to old type index,
             * but I would rather find out why it doesn't find everything here
             */
            ERR("%d indexes expected, but only have %d\n", indexes, pAviSplit->Parser.cStreams);
            indexes = 0;
        }
    }
    else if (!indexes && pAviSplit->oldindex)
        indexes = pAviSplit->Parser.cStreams;

    if (!indexes && pAviSplit->AviHeader.dwFlags & AVIF_MUSTUSEINDEX)
    {
        FIXME("No usable index was found!\n");
        hr = E_FAIL;
    }

    /* Now, set up the streams */
    if (hr == S_OK)
        hr = AVISplitter_InitializeStreams(pAviSplit);

    if (hr != S_OK)
    {
        AVISplitter_Disconnect(pAviSplit);
        return E_FAIL;
    }

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

static HRESULT AVISplitter_Disconnect(LPVOID iface)
{
    AVISplitterImpl *This = iface;
    int x;

    /* TODO: Remove other memory that's allocated during connect */
    CoTaskMemFree(This->oldindex);
    This->oldindex = NULL;

    for (x = 0; x < This->Parser.cStreams; ++x)
    {
        int i;

        StreamData *stream = &This->streams[x];

        for (i = 0; i < stream->entries; ++i)
            CoTaskMemFree(stream->stdindex[i]);

        CoTaskMemFree(stream->stdindex);
    }
    CoTaskMemFree(This->streams);
    This->streams = NULL;

    return S_OK;
}

static const IBaseFilterVtbl AVISplitter_Vtbl =
{
    Parser_QueryInterface,
    Parser_AddRef,
    Parser_Release,
    Parser_GetClassID,
    Parser_Stop,
    Parser_Pause,
    Parser_Run,
    Parser_GetState,
    Parser_SetSyncSource,
    Parser_GetSyncSource,
    Parser_EnumPins,
    Parser_FindPin,
    Parser_QueryFilterInfo,
    Parser_JoinFilterGraph,
    Parser_QueryVendorInfo
};

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
    This->streams = NULL;
    This->oldindex = NULL;

    hr = Parser_Create(&(This->Parser), &AVISplitter_Vtbl, &CLSID_AviSplitter, AVISplitter_Sample, AVISplitter_QueryAccept, AVISplitter_InputPin_PreConnect, AVISplitter_Cleanup, AVISplitter_Disconnect, NULL, NULL, NULL, NULL, NULL);

    if (FAILED(hr))
        return hr;

    *ppv = (LPVOID)This;

    return hr;
}
