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


static HRESULT MPEGSplitter_process_sample(LPVOID iface, IMediaSample * pSample)
{
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;
    LPBYTE pbSrcStream = NULL;
    DWORD cbSrcStream = 0;
    DWORD used_bytes = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;
    BYTE *pbDstStream;
    DWORD cbDstStream;
    long remaining_bytes = 0;
    Parser_OutputPin * pOutputPin;
    DWORD bytes_written = 0;

    pOutputPin = (Parser_OutputPin*)This->Parser.ppPins[1];

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (SUCCEEDED(hr))
    {
        cbSrcStream = IMediaSample_GetActualDataLength(pSample);
        hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    }

    /* trace removed for performance reasons */
    /* TRACE("(%p), %llu -> %llu\n", pSample, tStart, tStop); */

    if (This->pCurrentSample)
        bytes_written = IMediaSample_GetActualDataLength(This->pCurrentSample);

    while (hr == S_OK && used_bytes < cbSrcStream)
    {
        remaining_bytes = (long)(This->EndOfFile - BYTES_FROM_MEDIATIME(tStart) - used_bytes);
        if (remaining_bytes <= 0)
            break;

        if (!This->pCurrentSample)
        {
            /* cache media sample until it is ready to be despatched
             * (i.e. we reach the end of the chunk) */
            hr = OutputPin_GetDeliveryBuffer(&pOutputPin->pin, &This->pCurrentSample, NULL, NULL, 0);
            if (FAILED(hr))
            {
                TRACE("Skipping sending sample due to error (%x)\n", hr);
                This->pCurrentSample = NULL;
                break;
            }

            IMediaSample_SetTime(This->pCurrentSample, NULL, NULL);
            hr = IMediaSample_SetActualDataLength(This->pCurrentSample, 0);
            assert(hr == S_OK);
            bytes_written = 0;
        }

        hr = IMediaSample_GetPointer(This->pCurrentSample, &pbDstStream);
        if (SUCCEEDED(hr))
        {
            cbDstStream = IMediaSample_GetSize(This->pCurrentSample);
            remaining_bytes = min(remaining_bytes, (long)(cbDstStream - bytes_written));

            assert(remaining_bytes >= 0);

            /* trace removed for performance reasons */
            /* TRACE("remaining_bytes: %d, cbSrcStream: 0x%x\n", remaining_bytes, cbSrcStream); */
        }

        if (remaining_bytes <= (long)(cbSrcStream-used_bytes))
        {
            if (SUCCEEDED(hr))
            {
                memcpy(pbDstStream + bytes_written, pbSrcStream + used_bytes, remaining_bytes);
                bytes_written += remaining_bytes;

                hr = IMediaSample_SetActualDataLength(This->pCurrentSample, bytes_written);
                assert(hr == S_OK);

                used_bytes += remaining_bytes;
            }

            if (SUCCEEDED(hr))
            {
                REFERENCE_TIME tMPEGStart, tMPEGStop;

                pOutputPin->dwSamplesProcessed = (BYTES_FROM_MEDIATIME(tStart)+used_bytes) / pOutputPin->dwSampleSize;

                tMPEGStart = (tStart + MEDIATIME_FROM_BYTES(used_bytes-bytes_written)) /
                             (pOutputPin->fSamplesPerSec*pOutputPin->dwSampleSize);
                tMPEGStop  = (tStart + MEDIATIME_FROM_BYTES(used_bytes)) /
                             (pOutputPin->fSamplesPerSec*pOutputPin->dwSampleSize);

                /* If the start of the sample has a valid MPEG header, it's a
                 * sync point */
                if (MPEGSplitter_head_check(pbDstStream) == MPEG_AUDIO_HEADER)
                    IMediaSample_SetSyncPoint(This->pCurrentSample, TRUE);
                else
                    IMediaSample_SetSyncPoint(This->pCurrentSample, FALSE);
                IMediaSample_SetTime(This->pCurrentSample, &tMPEGStart, &tMPEGStop);

                hr = OutputPin_SendSample(&pOutputPin->pin, This->pCurrentSample);
                if (FAILED(hr))
                    WARN("Error sending sample (%x)\n", hr);
            }

            if (This->pCurrentSample)
                IMediaSample_Release(This->pCurrentSample);
            This->pCurrentSample = NULL;
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                memcpy(pbDstStream + bytes_written, pbSrcStream + used_bytes, cbSrcStream - used_bytes);
                bytes_written += cbSrcStream - used_bytes;
                IMediaSample_SetActualDataLength(This->pCurrentSample, bytes_written);

                used_bytes += cbSrcStream - used_bytes;
            }
        }
    }

    if (BYTES_FROM_MEDIATIME(tStop) >= This->EndOfFile)
    {
        int i;

        TRACE("End of file reached (%d out of %d bytes used)\n", used_bytes, cbSrcStream);

        if (This->pCurrentSample)
        {
            /* Make sure the last bit of data, if any, is sent */
            if (IMediaSample_GetActualDataLength(This->pCurrentSample) > 0)
            {
                REFERENCE_TIME tMPEGStart, tMPEGStop;

                pOutputPin->dwSamplesProcessed = (BYTES_FROM_MEDIATIME(tStart)+used_bytes) / pOutputPin->dwSampleSize;

                tMPEGStart = (tStart + MEDIATIME_FROM_BYTES(used_bytes-bytes_written)) /
                             (pOutputPin->fSamplesPerSec*pOutputPin->dwSampleSize);
                tMPEGStop  = (tStart + MEDIATIME_FROM_BYTES(used_bytes)) /
                             (pOutputPin->fSamplesPerSec*pOutputPin->dwSampleSize);

                if (MPEGSplitter_head_check(pbDstStream) == MPEG_AUDIO_HEADER)
                    IMediaSample_SetSyncPoint(This->pCurrentSample, TRUE);
                else
                    IMediaSample_SetSyncPoint(This->pCurrentSample, FALSE);
                IMediaSample_SetTime(This->pCurrentSample, &tMPEGStart, &tMPEGStop);

                hr = OutputPin_SendSample(&pOutputPin->pin, This->pCurrentSample);
                if (FAILED(hr))
                    WARN("Error sending sample (%x)\n", hr);
            }
            IMediaSample_Release(This->pCurrentSample);
        }
        This->pCurrentSample = NULL;

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


static const WCHAR wszAudioStream[] = {'A','u','d','i','o',0};
static const WCHAR wszVideoStream[] = {'V','i','d','e','o',0};

static HRESULT MPEGSplitter_init_audio(MPEGSplitterImpl *This, const BYTE *header, PIN_INFO *ppiOutput, AM_MEDIA_TYPE *pamt)
{
    static const DWORD freqs[10] = { 44100, 48000, 32000, 22050, 24000,
                                     16000, 11025, 12000,  8000, 0 };
    static const DWORD tabsel_123[2][3][16] = {
        { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
          {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
          {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

        { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
          {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
          {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
    };

    WAVEFORMATEX *format;
    int bitrate_index;
    int freq_index;
    int mode_ext;
    int emphasis;
    int padding;
    int lsf = 1;
    int mpeg1;
    int layer;
    int mode;

    ZeroMemory(pamt, sizeof(*pamt));
    ppiOutput->dir = PINDIR_OUTPUT;
    ppiOutput->pFilter = (IBaseFilter*)This;
    wsprintfW(ppiOutput->achName, wszAudioStream);

    memcpy(&pamt->formattype, &FORMAT_WaveFormatEx, sizeof(GUID));
    memcpy(&pamt->majortype, &MEDIATYPE_Audio, sizeof(GUID));
    memcpy(&pamt->subtype, &MEDIASUBTYPE_MPEG1AudioPayload, sizeof(GUID));

    pamt->lSampleSize = 0;
    pamt->bFixedSizeSamples = TRUE;
    pamt->bTemporalCompression = 0;

    mpeg1 = (header[1]>>4)&0x1;
    if (mpeg1)
        lsf = ((header[1]>>3)&0x1)^1;

    layer         = 4-((header[1]>>1)&0x3);
    bitrate_index =   ((header[2]>>4)&0xf);
    freq_index    =   ((header[2]>>2)&0x3) + (mpeg1?(lsf*3):6);
    padding       =   ((header[2]>>1)&0x1);
    mode          =   ((header[3]>>6)&0x3);
    mode_ext      =   ((header[3]>>4)&0x3);
    emphasis      =   ((header[3]>>0)&0x3);


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
    if (format->nAvgBytesPerSec == 0)
    {
        WARN("Variable-bitrate audio is not supported.\n");
        return E_FAIL;
    }

    if (layer == 3)
        format->nBlockAlign = format->nAvgBytesPerSec * 8 * 144 /
                              (format->nSamplesPerSec<<lsf) +
                              (padding - 4);
    else if (layer == 2)
        format->nBlockAlign = format->nAvgBytesPerSec * 8 * 144 /
                              format->nSamplesPerSec + (padding - 4);
    else
        format->nBlockAlign = ((format->nAvgBytesPerSec * 8 * 12 /
                                format->nSamplesPerSec + padding) << 2) - 4;

    format->wBitsPerSample = 0;

    if (layer == 3)
    {
        MPEGLAYER3WAVEFORMAT *mp3format = (MPEGLAYER3WAVEFORMAT*)format;

        format->cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;

        mp3format->wID = MPEGLAYER3_ID_MPEG;
        mp3format->fdwFlags = (padding ?
                               MPEGLAYER3_FLAG_PADDING_OFF :
                               MPEGLAYER3_FLAG_PADDING_ON);
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
    BYTE header[4];
    int streamtype = 0;
    LONGLONG total, avail;
    AM_MEDIA_TYPE amt;
    PIN_INFO piOutput;

    IAsyncReader_Length(pPin->pReader, &total, &avail);
    This->EndOfFile = total;

    hr = IAsyncReader_SyncRead(pPin->pReader, pos, 4, (LPVOID)&header[0]);
    if (SUCCEEDED(hr))
        pos += 4;

    while(SUCCEEDED(hr) && !(streamtype=MPEGSplitter_head_check(header)))
    {
        /* No valid header yet; shift by a byte and check again */
        memcpy(header, header+1, 3);
        hr = IAsyncReader_SyncRead(pPin->pReader, pos++, 1, (LPVOID)&header[3]);
    }
    if (FAILED(hr))
        return hr;

    switch(streamtype)
    {
        case MPEG_AUDIO_HEADER:
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

                hr = Parser_AddPin(&(This->Parser), &piOutput, &props, &amt,
                                   (float)format->nAvgBytesPerSec /
                                    (float)format->nBlockAlign,
                                   format->nBlockAlign,
                                   total);
            }

            if (FAILED(hr))
            {
                if (amt.pbFormat)
                    CoTaskMemFree(amt.pbFormat);
                ERR("Could not create pin for MPEG audio stream (%x)\n", hr);
            }
            break;
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

    return hr;
}

static HRESULT MPEGSplitter_cleanup(LPVOID iface)
{
    MPEGSplitterImpl *This = (MPEGSplitterImpl*)iface;

    TRACE("(%p)->()\n", This);

    if (This->pCurrentSample)
        IMediaSample_Release(This->pCurrentSample);
    This->pCurrentSample = NULL;

    return S_OK;
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
    hr = Parser_Create(&(This->Parser), &CLSID_MPEG1Splitter, MPEGSplitter_process_sample, MPEGSplitter_query_accept, MPEGSplitter_pre_connect, MPEGSplitter_cleanup);
    if (FAILED(hr))
    {
        CoTaskMemFree(This);
        return hr;
    }

    /* Note: This memory is managed by the parser filter once created */
    *ppv = (LPVOID)This;

    return hr;
}
