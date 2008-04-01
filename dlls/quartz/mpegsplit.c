/*
 * MPEG Splitter Filter
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2004-2005 Christian Costa
 * Copyright 2007 Chris Robinson
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

#include <assert.h>
#include <math.h>

#include "quartz_private.h"
#include "control_private.h"
#include "pin.h"

#include "uuids.h"
#include "mmreg.h"
#include "mmsystem.h"

#include "winternl.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "parser.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#define SEQUENCE_HEADER_CODE     0xB3
#define PACK_START_CODE          0xBA

#define SYSTEM_START_CODE        0xBB
#define AUDIO_ELEMENTARY_STREAM  0xC0
#define VIDEO_ELEMENTARY_STREAM  0xE0

#define MPEG_SYSTEM_HEADER 3
#define MPEG_VIDEO_HEADER 2
#define MPEG_AUDIO_HEADER 1
#define MPEG_NO_HEADER 0

typedef struct MPEGSplitterImpl
{
    ParserImpl Parser;
    IMediaSample *pCurrentSample;
    LONGLONG EndOfFile;
    LONGLONG duration;
    LONGLONG position;
    DWORD skipbytes;
    DWORD header_bytes;
    DWORD remaining_bytes;
    BOOL seek;
} MPEGSplitterImpl;

static int MPEGSplitter_head_check(const BYTE *header)
{
    /* If this is a possible start code, check for a system or video header */
    if (header[0] == 0 && header[1] == 0 && header[2] == 1)
    {
        /* Check if we got a system or elementary stream start code */
        if (header[3] == PACK_START_CODE ||
            header[3] == VIDEO_ELEMENTARY_STREAM ||
            header[3] == AUDIO_ELEMENTARY_STREAM)
            return MPEG_SYSTEM_HEADER;

        /* Check for a MPEG video sequence start code */
        if (header[3] == SEQUENCE_HEADER_CODE)
            return MPEG_VIDEO_HEADER;
    }

    /* This should give a good guess if we have an MPEG audio header */
    if(header[0] == 0xff && ((header[1]>>5)&0x7) == 0x7 &&
       ((header[1]>>1)&0x3) != 0 && ((header[2]>>4)&0xf) != 0xf &&
       ((header[2]>>2)&0x3) != 0x3)
        return MPEG_AUDIO_HEADER;

    /* Nothing yet.. */
    return MPEG_NO_HEADER;
}

static const WCHAR wszAudioStream[] = {'A','u','d','i','o',0};
static const WCHAR wszVideoStream[] = {'V','i','d','e','o',0};

static const DWORD freqs[10] = { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000,  8000, 0 };

static const DWORD tabsel_123[2][3][16] = {
    { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
      {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
      {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

    { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
      {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
      {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};


static HRESULT parse_header(BYTE *header, LONGLONG *plen, LONGLONG *pduration)
{
    LONGLONG duration = *pduration;

    int bitrate_index, freq_index, mode_ext, emphasis, lsf = 1, mpeg1, layer, mode, padding, bitrate, length;

    if (!(header[0] == 0xff && ((header[1]>>5)&0x7) == 0x7 &&
       ((header[1]>>1)&0x3) != 0 && ((header[2]>>4)&0xf) != 0xf &&
       ((header[2]>>2)&0x3) != 0x3))
    {
        FIXME("Not a valid header: %02x:%02x\n", header[0], header[1]);
        return E_INVALIDARG;
    }

    mpeg1 = (header[1]>>4)&0x1;
    if (mpeg1)
        lsf = ((header[1]>>3)&0x1)^1;

    layer = 4-((header[1]>>1)&0x3);
    bitrate_index = ((header[2]>>4)&0xf);
    freq_index = ((header[2]>>2)&0x3) + (mpeg1?(lsf*3):6);
    padding = ((header[2]>>1)&0x1);
    mode = ((header[3]>>6)&0x3);
    mode_ext = ((header[3]>>4)&0x3);
    emphasis = ((header[3]>>0)&0x3);

    bitrate = tabsel_123[lsf][layer-1][bitrate_index] * 1000;
    if (!bitrate || layer != 3)
    {
        ERR("Not a valid header: %02x:%02x:%02x:%02x\n", header[0], header[1], header[2], header[3]);
        return E_INVALIDARG;
    }


    if (layer == 3 || layer == 2)
        length = 144 * bitrate / freqs[freq_index] + padding;
    else
        length = 4 * (12 * bitrate / freqs[freq_index] + padding);

    duration = (ULONGLONG)10000000 * (ULONGLONG)(length) / (ULONGLONG)(bitrate/8);
    *plen = length;
    *pduration += duration;
    return S_OK;
}


static void skip_data(BYTE** from, DWORD *flen, DWORD amount)
{
    *flen -= amount;
    if (!*flen)
        *from = NULL;
    else
        *from += amount;
}

static HRESULT copy_data(IMediaSample *to, BYTE** from, DWORD *flen, DWORD amount)
{
    HRESULT hr = S_OK;
    BYTE *ptr = NULL;
    DWORD oldlength = IMediaSample_GetActualDataLength(to);

    hr = IMediaSample_SetActualDataLength(to, oldlength + amount);
    if (FAILED(hr))
    {
        if (!oldlength || oldlength <= 4)
            WARN("Could not set require length\n");
        return hr;
    }

    IMediaSample_GetPointer(to, &ptr);
    memcpy(ptr + oldlength, *from, amount);
    skip_data(from, flen, amount);
    return hr;
}

static HRESULT FillBuffer(MPEGSplitterImpl *This, BYTE** fbuf, DWORD *flen, IMediaSample *pCurrentSample)
{
    Parser_OutputPin * pOutputPin = (Parser_OutputPin*)This->Parser.ppPins[1];
    LONGLONG length = 0;
    HRESULT hr = S_OK;
    DWORD dlen;
    LONGLONG time = This->position, sampleduration = 0;

    TRACE("Source length: %u, skip length: %u, remaining: %u\n", *flen, This->skipbytes, This->remaining_bytes);

    /* Case where bytes are skipped */
    if (This->skipbytes)
    {
        DWORD skip = min(This->skipbytes, *flen);
        skip_data(fbuf, flen, skip);
        This->skipbytes -= skip;
        return S_OK;
    }

    /* Case where there is already an output sample being held */
    if (This->remaining_bytes)
    {
        DWORD towrite = min(This->remaining_bytes, *flen);

        hr = copy_data(pCurrentSample, fbuf, flen, towrite);
        if (FAILED(hr))
        {
            WARN("Could not resize sample: %08x\n", hr);
            return hr;
        }

        This->remaining_bytes -= towrite;
        if (This->remaining_bytes)
            return hr;

        /* Optimize: Try appending more samples to the stream */
        goto out_append;
    }

    /* Special case, last source sample might (or might not have) had a header, and now we want to retrieve it */
    dlen = IMediaSample_GetActualDataLength(pCurrentSample);
    if (dlen > 0 && dlen < 4)
    {
        BYTE *header = NULL;
        DWORD attempts = 0;

        /* Shoot anyone with a small sample! */
        assert(*flen >= 6);

        hr = IMediaSample_GetPointer(pCurrentSample, &header);

        if (SUCCEEDED(hr))
            hr = IMediaSample_SetActualDataLength(pCurrentSample, 7);

        if (FAILED(hr))
        {
            WARN("Could not resize sample: %08x\n", hr);
            return hr;
        }

        memcpy(header + dlen, *fbuf, 6 - dlen);

        while (FAILED(parse_header(header+attempts, &length, &This->position)) && attempts < dlen)
        {
            attempts++;
        }

        /* No header found */
        if (attempts == dlen)
        {
            hr = IMediaSample_SetActualDataLength(pCurrentSample, 0);
            return hr;
        }

        IMediaSample_SetActualDataLength(pCurrentSample, 4);
        IMediaSample_SetTime(pCurrentSample, &time, &This->position);

        /* Move header back to beginning */
        if (attempts)
            memmove(header, header+attempts, 4);

        This->remaining_bytes = length - 4;
        *flen -= (4 - dlen + attempts);
        *fbuf += (4 - dlen + attempts);
        return hr;
    }

    /* Destination sample should contain no data! But the source sample should */
    assert(!dlen);
    assert(*flen);

    /* Find the next valid header.. it <SHOULD> be right here */
    while (*flen > 3 && FAILED(parse_header(*fbuf, &length, &This->position)))
    {
        skip_data(fbuf, flen, 1);
    }

    /* Uh oh, no header found! */
    if (*flen < 4)
    {
        assert(!length);
        hr = copy_data(pCurrentSample, fbuf, flen, *flen);
        return hr;
    }

    IMediaSample_SetTime(pCurrentSample, &time, &This->position);

    if (*flen < length)
    {
        /* Partial copy: Copy 4 bytes, the rest will be copied by the logic for This->remaining_bytes */
        This->remaining_bytes = length - 4;
        copy_data(pCurrentSample, fbuf, flen, 4);
        return hr;
    }

    hr = copy_data(pCurrentSample, fbuf, flen, length);
    if (FAILED(hr))
    {
        WARN("Couldn't set data size to %x%08x\n", (DWORD)(length >> 32), (DWORD)length);
        This->skipbytes = length;
        return hr;
    }

out_append:
    /* Optimize: Send multiple samples! */
    while (*flen >= 4)
    {
        if (FAILED(parse_header(*fbuf, &length, &sampleduration)))
            break;

        if (length > *flen)
            break;

        if (FAILED(copy_data(pCurrentSample, fbuf, flen, length)))
            break;

        This->position += sampleduration;
        sampleduration = 0;
        IMediaSample_SetTime(pCurrentSample, &time, &This->position);
    }
    TRACE("Media time: %u.%03u\n", (DWORD)(This->position/10000000), (DWORD)((This->position/10000)%1000));

    IMediaSample_AddRef(pCurrentSample);
    LeaveCriticalSection(&This->Parser.csFilter);

    hr = OutputPin_SendSample(&pOutputPin->pin, pCurrentSample);

    EnterCriticalSection(&This->Parser.csFilter);
    IMediaSample_Release(pCurrentSample);

    if (FAILED(hr))
    {
        WARN("Error sending sample (%x)\n", hr);
        return hr;
    }
    if (This->pCurrentSample)
    {
        IMediaSample_Release(pCurrentSample);
        This->pCurrentSample = NULL;
    }
    return hr;
}


static HRESULT MPEGSplitter_process_sample(LPVOID iface, IMediaSample * pSample)
{
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;
    BYTE *pbSrcStream;
    DWORD cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    Parser_OutputPin * pOutputPin;
    HRESULT hr;

    pOutputPin = (Parser_OutputPin*)This->Parser.ppPins[1];

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (SUCCEEDED(hr))
    {
        cbSrcStream = IMediaSample_GetActualDataLength(pSample);
        hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    }

    /* trace removed for performance reasons */
    /* TRACE("(%p), %llu -> %llu\n", pSample, tStart, tStop); */

    EnterCriticalSection(&This->Parser.csFilter);
    /* Now, try to find a new header */
    while (cbSrcStream > 0)
    {
        if (!This->pCurrentSample)
        {
            if (FAILED(hr = OutputPin_GetDeliveryBuffer(&pOutputPin->pin, &This->pCurrentSample, NULL, NULL, 0)))
            {
                FIXME("Failed with hres: %08x!\n", hr);
                break;
            }

            IMediaSample_SetTime(This->pCurrentSample, NULL, NULL);
            if (FAILED(hr = IMediaSample_SetActualDataLength(This->pCurrentSample, 0)))
                goto fail;
            IMediaSample_SetSyncPoint(This->pCurrentSample, TRUE);
            IMediaSample_SetDiscontinuity(This->pCurrentSample, This->seek);
            This->seek = FALSE;
        }
        hr = FillBuffer(This, &pbSrcStream, &cbSrcStream, This->pCurrentSample);
        if (SUCCEEDED(hr) && hr != S_FALSE)
            continue;

fail:
        if (hr != S_FALSE)
            FIXME("Failed with hres: %08x!\n", hr);
        This->skipbytes += This->remaining_bytes;
        This->remaining_bytes = 0;
        if (This->pCurrentSample)
        {
            IMediaSample_SetActualDataLength(This->pCurrentSample, 0);
            IMediaSample_Release(This->pCurrentSample);
            This->pCurrentSample = NULL;
        }
        break;
    }

    if (BYTES_FROM_MEDIATIME(tStop) >= This->EndOfFile)
    {
        int i;

        TRACE("End of file reached\n");

        if (This->pCurrentSample)
        {
            /* Drop last data, it's likely to be garbage anyway */
            IMediaSample_SetActualDataLength(This->pCurrentSample, 0);
            IMediaSample_Release(This->pCurrentSample);
            This->pCurrentSample = NULL;
        }

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
                WARN("Error sending EndOfStream to pin %d (%x)\n", i, hr);
        }

        /* Force the pullpin thread to stop */
        hr = S_FALSE;
    }

    LeaveCriticalSection(&This->Parser.csFilter);
    return hr;
}


static HRESULT MPEGSplitter_query_accept(LPVOID iface, const AM_MEDIA_TYPE *pmt)
{
    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Stream))
        return S_FALSE;

    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_MPEG1Audio))
        return S_OK;

    if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_MPEG1Video))
        FIXME("MPEG-1 video streams not yet supported.\n");
    else if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_MPEG1System))
        FIXME("MPEG-1 system streams not yet supported.\n");
    else if (IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_MPEG1VideoCD))
        FIXME("MPEG-1 VideoCD streams not yet supported.\n");

    return S_FALSE;
}


static HRESULT MPEGSplitter_init_audio(MPEGSplitterImpl *This, const BYTE *header, PIN_INFO *ppiOutput, AM_MEDIA_TYPE *pamt)
{
    WAVEFORMATEX *format;
    int bitrate_index;
    int freq_index;
    int mode_ext;
    int emphasis;
    int lsf = 1;
    int mpeg1;
    int layer;
    int mode;

    ZeroMemory(pamt, sizeof(*pamt));
    ppiOutput->dir = PINDIR_OUTPUT;
    ppiOutput->pFilter = (IBaseFilter*)This;
    wsprintfW(ppiOutput->achName, wszAudioStream);

    pamt->formattype = FORMAT_WaveFormatEx;
    pamt->majortype = MEDIATYPE_Audio;
    pamt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;

    pamt->lSampleSize = 0;
    pamt->bFixedSizeSamples = FALSE;
    pamt->bTemporalCompression = 0;

    mpeg1 = (header[1]>>4)&0x1;
    if (mpeg1)
        lsf = ((header[1]>>3)&0x1)^1;

    layer         = 4-((header[1]>>1)&0x3);
    bitrate_index =   ((header[2]>>4)&0xf);
    freq_index    =   ((header[2]>>2)&0x3) + (mpeg1?(lsf*3):6);
    mode          =   ((header[3]>>6)&0x3);
    mode_ext      =   ((header[3]>>4)&0x3);
    emphasis      =   ((header[3]>>0)&0x3);

    if (!bitrate_index)
    {
        /* Set to highest bitrate so samples will fit in for sure */
        FIXME("Variable-bitrate audio not fully supported.\n");
        bitrate_index = 15;
    }

    pamt->cbFormat = ((layer==3)? sizeof(MPEGLAYER3WAVEFORMAT) :
                                  sizeof(MPEG1WAVEFORMAT));
    pamt->pbFormat = CoTaskMemAlloc(pamt->cbFormat);
    if (!pamt->pbFormat)
        return E_OUTOFMEMORY;
    ZeroMemory(pamt->pbFormat, pamt->cbFormat);
    format = (WAVEFORMATEX*)pamt->pbFormat;

    format->wFormatTag      = ((layer == 3) ? WAVE_FORMAT_MPEGLAYER3 :
                                              WAVE_FORMAT_MPEG);
    format->nChannels       = ((mode == 3) ? 1 : 2);
    format->nSamplesPerSec  = freqs[freq_index];
    format->nAvgBytesPerSec = tabsel_123[lsf][layer-1][bitrate_index] * 1000 / 8;

    if (layer == 3)
        format->nBlockAlign = format->nAvgBytesPerSec * 8 * 144 /
                              (format->nSamplesPerSec<<lsf) + 1;
    else if (layer == 2)
        format->nBlockAlign = format->nAvgBytesPerSec * 8 * 144 /
                              format->nSamplesPerSec + 1;
    else
        format->nBlockAlign = 4 * (format->nAvgBytesPerSec * 8 * 12 / format->nSamplesPerSec + 1);

    format->wBitsPerSample = 0;

    if (layer == 3)
    {
        MPEGLAYER3WAVEFORMAT *mp3format = (MPEGLAYER3WAVEFORMAT*)format;

        format->cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;

        mp3format->wID = MPEGLAYER3_ID_MPEG;
        mp3format->fdwFlags = MPEGLAYER3_FLAG_PADDING_ON;
        mp3format->nBlockSize = format->nBlockAlign;
        mp3format->nFramesPerBlock = 1;

        /* Beware the evil magic numbers. This struct is apparently horribly
         * under-documented, and the only references I could find had it being
         * set to this with no real explanation. It works fine though, so I'm
         * not complaining (yet).
         */
        mp3format->nCodecDelay = 1393;
    }
    else
    {
        MPEG1WAVEFORMAT *mpgformat = (MPEG1WAVEFORMAT*)format;

        format->cbSize = 22;

        mpgformat->fwHeadLayer   = ((layer == 1) ? ACM_MPEG_LAYER1 :
                                    ((layer == 2) ? ACM_MPEG_LAYER2 :
                                     ACM_MPEG_LAYER3));
        mpgformat->dwHeadBitrate = format->nAvgBytesPerSec * 8;
        mpgformat->fwHeadMode    = ((mode == 3) ? ACM_MPEG_SINGLECHANNEL :
                                    ((mode == 2) ? ACM_MPEG_DUALCHANNEL :
                                     ((mode == 1) ? ACM_MPEG_JOINTSTEREO :
                                      ACM_MPEG_STEREO)));
        mpgformat->fwHeadModeExt = ((mode == 1) ? 0x0F : (1<<mode_ext));
        mpgformat->wHeadEmphasis = emphasis + 1;
        mpgformat->fwHeadFlags   = ACM_MPEG_ID_MPEG1;
    }
    pamt->subtype.Data1 = format->wFormatTag;

    TRACE("MPEG audio stream detected:\n"
          "\tLayer %d (%#x)\n"
          "\tFrequency: %d\n"
          "\tChannels: %d (%d)\n"
          "\tBytesPerSec: %d\n",
          layer, format->wFormatTag, format->nSamplesPerSec,
          format->nChannels, mode, format->nAvgBytesPerSec);

    dump_AM_MEDIA_TYPE(pamt);

    return S_OK;
}


static HRESULT MPEGSplitter_pre_connect(IPin *iface, IPin *pConnectPin)
{
    PullPin *pPin = (PullPin *)iface;
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)pPin->pin.pinInfo.pFilter;
    ALLOCATOR_PROPERTIES props;
    HRESULT hr;
    LONGLONG pos = 0; /* in bytes */
    BYTE header[10];
    int streamtype = 0;
    LONGLONG total, avail;
    AM_MEDIA_TYPE amt;
    PIN_INFO piOutput;

    IAsyncReader_Length(pPin->pReader, &total, &avail);
    This->EndOfFile = total;

    hr = IAsyncReader_SyncRead(pPin->pReader, pos, 4, header);
    if (SUCCEEDED(hr))
        pos += 4;

    /* Skip ID3 v2 tag, if any */
    if (SUCCEEDED(hr) && !strncmp("ID3", (char*)header, 3))
    do {
        UINT length;
        hr = IAsyncReader_SyncRead(pPin->pReader, pos, 6, header + 4);
        if (FAILED(hr))
            break;
        pos += 6;
        TRACE("Found ID3 v2.%d.%d\n", header[3], header[4]);
        length  = (header[6] & 0x7F) << 21;
        length += (header[7] & 0x7F) << 14;
        length += (header[8] & 0x7F) << 7;
        length += (header[9] & 0x7F);
        TRACE("Length: %u\n", length);
        pos += length;

        /* Read the real header for the mpeg splitter */
        hr = IAsyncReader_SyncRead(pPin->pReader, pos, 4, header);
        if (SUCCEEDED(hr))
            pos += 4;
        TRACE("%x:%x:%x:%x\n", header[0], header[1], header[2], header[3]);
    } while (0);

    while(SUCCEEDED(hr) && !(streamtype=MPEGSplitter_head_check(header)))
    {
        TRACE("%x:%x:%x:%x\n", header[0], header[1], header[2], header[3]);
        /* No valid header yet; shift by a byte and check again */
        memmove(header, header+1, 3);
        hr = IAsyncReader_SyncRead(pPin->pReader, pos++, 1, header + 3);
    }
    if (FAILED(hr))
        return hr;
    pos -= 4;
    This->header_bytes = This->skipbytes = pos;

    switch(streamtype)
    {
        case MPEG_AUDIO_HEADER:
        {
            LONGLONG duration = 0;
            DWORD ticks = GetTickCount();

            hr = MPEGSplitter_init_audio(This, header, &piOutput, &amt);
            if (SUCCEEDED(hr))
            {
                WAVEFORMATEX *format = (WAVEFORMATEX*)amt.pbFormat;

                props.cbAlign = 1;
                props.cbPrefix = 0;
                /* Make the output buffer a multiple of the frame size */
                props.cbBuffer = 0x4000 / format->nBlockAlign *
                                 format->nBlockAlign;
                props.cBuffers = 1;
                hr = Parser_AddPin(&(This->Parser), &piOutput, &props, &amt);
            }

            if (FAILED(hr))
            {
                if (amt.pbFormat)
                    CoTaskMemFree(amt.pbFormat);
                ERR("Could not create pin for MPEG audio stream (%x)\n", hr);
                break;
            }

            /* Check for idv1 tag, and remove it from stream if found */
            hr = IAsyncReader_SyncRead(pPin->pReader, This->EndOfFile-128, 3, header+4);
            if (FAILED(hr))
                break;
            if (!strncmp((char*)header+4, "TAG", 3))
                This->EndOfFile -= 128;

            /* http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm has a whole readup on audio headers */
            while (pos + 3 < This->EndOfFile)
            {
                LONGLONG length = 0;
                hr = IAsyncReader_SyncRead(pPin->pReader, pos, 4, header);
                if (hr != S_OK)
                    break;
                while (parse_header(header, &length, &duration))
                {
                    /* No valid header yet; shift by a byte and check again */
                    memmove(header, header+1, 3);
                    hr = IAsyncReader_SyncRead(pPin->pReader, pos++, 1, header + 3);
                    if (hr != S_OK || This->EndOfFile - pos < 4)
                       break;
                }
                pos += length;
                TRACE("Pos: %x%08x/%x%08x\n", (DWORD)(pos >> 32), (DWORD)pos, (DWORD)(This->EndOfFile>>32), (DWORD)This->EndOfFile);
            }
            hr = S_OK;
            TRACE("Duration: %d seconds\n", (DWORD)(duration / 10000000));
            TRACE("Parsing took %u ms\n", GetTickCount() - ticks);
            This->duration = duration;
            break;
        }
        case MPEG_VIDEO_HEADER:
            FIXME("MPEG video processing not yet supported!\n");
            hr = E_FAIL;
            break;
        case MPEG_SYSTEM_HEADER:
            FIXME("MPEG system streams not yet supported!\n");
            hr = E_FAIL;
            break;

        default:
            break;
    }
    This->remaining_bytes = 0;
    This->position = 0;

    return hr;
}

static HRESULT MPEGSplitter_cleanup(LPVOID iface)
{
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;

    TRACE("(%p)->()\n", This);

    if (This->pCurrentSample)
        IMediaSample_Release(This->pCurrentSample);
    This->pCurrentSample = NULL;

    if (This->Parser.pInputPin && !This->seek)
    {
        This->skipbytes += This->remaining_bytes;
        This->Parser.pInputPin->rtCurrent += MEDIATIME_FROM_BYTES(This->skipbytes);
    }
    if (!This->seek)
        This->skipbytes = This->remaining_bytes = 0;

    return S_OK;
}

static HRESULT MPEGSplitter_seek(IBaseFilter *iface)
{
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;
    PullPin *pPin = This->Parser.pInputPin;
    LONGLONG newpos, timepos, bytepos;
    HRESULT hr = S_OK;
    BYTE header[4];

    /* Position, in bytes */
    bytepos = This->header_bytes;

    /* Position, in media time, current and new */
    timepos = 0;
    newpos = This->Parser.mediaSeeking.llCurrent;

    if (newpos > This->duration)
    {
        FIXME("Requesting position %x%08x beyond end of stream %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(This->duration>>32), (DWORD)This->duration);
        return E_INVALIDARG;
    }

    if (This->position/1000000 == newpos/1000000)
    {
        FIXME("Requesting position %x%08x same as current position %x%08x\n", (DWORD)(newpos>>32), (DWORD)newpos, (DWORD)(This->position>>32), (DWORD)This->position);
        return S_OK;
    }

    hr = IAsyncReader_SyncRead(pPin->pReader, bytepos, 4, header);

    while (bytepos < This->EndOfFile && SUCCEEDED(hr))
    {
        LONGLONG length = 0;
        hr = IAsyncReader_SyncRead(pPin->pReader, bytepos, 4, header);
        while (parse_header(header, &length, &timepos))
        {
            /* No valid header yet; shift by a byte and check again */
            memmove(header, header+1, 3);
            hr = IAsyncReader_SyncRead(pPin->pReader, ++bytepos, 1, header + 3);
            if (FAILED(hr))
                break;
         }
         bytepos += length;
         TRACE("Pos: %x%08x/%x%08x\n", (DWORD)(bytepos >> 32), (DWORD)bytepos, (DWORD)(This->EndOfFile>>32), (DWORD)This->EndOfFile);
         if (timepos >= newpos)
             break;
    }
    if (SUCCEEDED(hr))
    {
        FILTER_STATE state;
        PullPin *pin = This->Parser.pInputPin;

        TRACE("Moving sound to %x%08x bytes!\n", (DWORD)(bytepos>>32), (DWORD)bytepos);

        IPin_BeginFlush((IPin *)pin);
        IPin_NewSegment((IPin*)pin, newpos, This->duration, pin->dRate);
        IPin_EndFlush((IPin *)pin);

        /* Make sure this is done while stopped */
        EnterCriticalSection(&This->Parser.csFilter);
        pin->rtStart = pin->rtCurrent = MEDIATIME_FROM_BYTES(bytepos);
        pin->rtStop = MEDIATIME_FROM_BYTES((REFERENCE_TIME)This->EndOfFile);
        This->seek = TRUE;
        This->skipbytes = This->remaining_bytes = 0;
        This->position = newpos;
        if (This->pCurrentSample)
        {
            IMediaSample_Release(This->pCurrentSample);
            This->pCurrentSample = NULL;
        }
        state = This->Parser.state;
        LeaveCriticalSection(&This->Parser.csFilter);

        if (state == State_Running && pin->state == State_Paused)
            PullPin_StartProcessing(pin);

    }
    return hr;
}

HRESULT MPEGSplitter_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    MPEGSplitterImpl *This;
    HRESULT hr = E_FAIL;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = CoTaskMemAlloc(sizeof(MPEGSplitterImpl));
    if (!This)
        return E_OUTOFMEMORY;

    ZeroMemory(This, sizeof(MPEGSplitterImpl));
    hr = Parser_Create(&(This->Parser), &CLSID_MPEG1Splitter, MPEGSplitter_process_sample, MPEGSplitter_query_accept, MPEGSplitter_pre_connect, MPEGSplitter_cleanup, NULL, MPEGSplitter_seek, NULL);
    if (FAILED(hr))
    {
        CoTaskMemFree(This);
        return hr;
    }
    This->seek = TRUE;

    /* Note: This memory is managed by the parser filter once created */
    *ppv = (LPVOID)This;

    return hr;
}
