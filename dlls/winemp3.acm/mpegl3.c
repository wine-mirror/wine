/*
 * MPEG Layer 3 handling
 *
 * Copyright (C) 2002 Eric Pouech
 * Copyright (C) 2009 CodeWeavers, Aric Stewart
 *
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
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#ifdef HAVE_MPG123_H
# include <mpg123.h>
#else
# ifdef HAVE_COREAUDIO_COREAUDIO_H
#  include <CoreFoundation/CoreFoundation.h>
#  include <CoreAudio/CoreAudio.h>
# endif
# ifdef HAVE_AUDIOTOOLBOX_AUDIOCONVERTER_H
#  include <AudioToolbox/AudioConverter.h>
# endif
# ifdef HAVE_AUDIOTOOLBOX_AUDIOFILE_H
#  include <AudioToolbox/AudioFile.h>
# endif
# ifdef HAVE_AUDIOTOOLBOX_AUDIOFILESTREAM_H
#  include <AudioToolbox/AudioFileStream.h>
# endif
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpeg3);

/* table to list all supported formats... those are the basic ones. this
 * also helps given a unique index to each of the supported formats
 */
typedef	struct
{
    int		nChannels;
    int		nBits;
    int		rate;
} Format;

static const Format PCM_Formats[] =
{
    {1,  8,  8000}, {2,  8,  8000}, {1, 16,  8000}, {2, 16,  8000},
    {1,  8, 11025}, {2,  8, 11025}, {1, 16, 11025}, {2, 16, 11025},
    {1,  8, 12000}, {2,  8, 12000}, {1, 16, 12000}, {2, 16, 12000},
    {1,  8, 16000}, {2,  8, 16000}, {1, 16, 16000}, {2, 16, 16000},
    {1,  8, 22050}, {2,  8, 22050}, {1, 16, 22050}, {2, 16, 22050},
    {1,  8, 24000}, {2,  8, 24000}, {1, 16, 24000}, {2, 16, 24000},
    {1,  8, 32000}, {2,  8, 32000}, {1, 16, 32000}, {2, 16, 32000},
    {1,  8, 44100}, {2,  8, 44100}, {1, 16, 44100}, {2, 16, 44100},
    {1,  8, 48000}, {2,  8, 48000}, {1, 16, 48000}, {2, 16, 48000}
};

static const Format MPEG3_Formats[] =
{
    {1,  0,  8000}, {2,  0,  8000},
    {1,  0, 11025}, {2,  0, 11025},
    {1,  0, 12000}, {2,  0, 12000},
    {1,  0, 16000}, {2,  0, 16000},
    {1,  0, 22050}, {2,  0, 22050},
    {1,  0, 24000}, {2,  0, 24000},
    {1,  0, 32000}, {2,  0, 32000},
    {1,  0, 44100}, {2,  0, 44100},
    {1,  0, 48000}, {2,  0, 48000}
};

#define	NUM_PCM_FORMATS		(sizeof(PCM_Formats) / sizeof(PCM_Formats[0]))
#define	NUM_MPEG3_FORMATS	(sizeof(MPEG3_Formats) / sizeof(MPEG3_Formats[0]))

/***********************************************************************
 *           MPEG3_GetFormatIndex
 */
static	DWORD	MPEG3_GetFormatIndex(LPWAVEFORMATEX wfx)
{
    int 	i, hi;
    const Format *fmts;

    switch (wfx->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
	hi = NUM_PCM_FORMATS;
	fmts = PCM_Formats;
	break;
    case WAVE_FORMAT_MPEGLAYER3:
	hi = NUM_MPEG3_FORMATS;
	fmts = MPEG3_Formats;
	break;
    default:
	return 0xFFFFFFFF;
    }

    for (i = 0; i < hi; i++)
    {
	if (wfx->nChannels == fmts[i].nChannels &&
	    wfx->nSamplesPerSec == fmts[i].rate &&
	    (wfx->wBitsPerSample == fmts[i].nBits || !fmts[i].nBits))
	    return i;
    }

    return 0xFFFFFFFF;
}

#ifdef HAVE_MPG123_H

typedef struct tagAcmMpeg3Data
{
    void (*convert)(PACMDRVSTREAMINSTANCE adsi,
		    const unsigned char*, LPDWORD, unsigned char*, LPDWORD);
    mpg123_handle *mh;
} AcmMpeg3Data;

/***********************************************************************
 *           MPEG3_drvOpen
 */
static LRESULT MPEG3_drvOpen(LPCSTR str)
{
    mpg123_init();
    return 1;
}

/***********************************************************************
 *           MPEG3_drvClose
 */
static LRESULT MPEG3_drvClose(DWORD_PTR dwDevID)
{
    mpg123_exit();
    return 1;
}


static void mp3_horse(PACMDRVSTREAMINSTANCE adsi,
                      const unsigned char* src, LPDWORD nsrc,
                      unsigned char* dst, LPDWORD ndst)
{
    AcmMpeg3Data*       amd = (AcmMpeg3Data*)adsi->dwDriver;
    int                 ret;
    size_t              size;
    DWORD               dpos = 0;


    if (*nsrc > 0)
    {
        ret = mpg123_feed(amd->mh, src, *nsrc);
        if (ret != MPG123_OK)
        {
            ERR("Error feeding data\n");
            *ndst = *nsrc = 0;
            return;
        }
    }

    do {
        size = 0;
        ret = mpg123_read(amd->mh, dst + dpos, *ndst - dpos, &size);
        if (ret == MPG123_ERR)
        {
            FIXME("Error occurred during decoding!\n");
            *ndst = *nsrc = 0;
            return;
        }

        if (ret == MPG123_NEW_FORMAT)
        {
            long rate;
            int channels, enc;
            mpg123_getformat(amd->mh, &rate, &channels, &enc);
            TRACE("New format: %li Hz, %i channels, encoding value %i\n", rate, channels, enc);
        }
        dpos += size;
        if (dpos > *ndst) break;
    } while (ret != MPG123_ERR && ret != MPG123_NEED_MORE);
    *ndst = dpos;
}

/***********************************************************************
 *           MPEG3_Reset
 *
 */
static void MPEG3_Reset(PACMDRVSTREAMINSTANCE adsi, AcmMpeg3Data* aad)
{
    mpg123_feedseek(aad->mh, 0, SEEK_SET, NULL);
    mpg123_close(aad->mh);
    mpg123_open_feed(aad->mh);
}

/***********************************************************************
 *           MPEG3_StreamOpen
 *
 */
static	LRESULT	MPEG3_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    AcmMpeg3Data*	aad;
    int err;

    assert(!(adsi->fdwOpen & ACM_STREAMOPENF_ASYNC));

    if (MPEG3_GetFormatIndex(adsi->pwfxSrc) == 0xFFFFFFFF ||
	MPEG3_GetFormatIndex(adsi->pwfxDst) == 0xFFFFFFFF)
	return ACMERR_NOTPOSSIBLE;

    aad = HeapAlloc(GetProcessHeap(), 0, sizeof(AcmMpeg3Data));
    if (aad == 0) return MMSYSERR_NOMEM;

    adsi->dwDriver = (DWORD_PTR)aad;

    if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
	goto theEnd;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEGLAYER3 &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
	/* resampling or mono <=> stereo not available
         * MPEG3 algo only define 16 bit per sample output
         */
	if (adsi->pwfxSrc->nSamplesPerSec != adsi->pwfxDst->nSamplesPerSec ||
	    adsi->pwfxSrc->nChannels != adsi->pwfxDst->nChannels ||
            adsi->pwfxDst->wBitsPerSample != 16)
	    goto theEnd;
        aad->convert = mp3_horse;
        aad->mh = mpg123_new(NULL,&err);
        mpg123_open_feed(aad->mh);
    }
    /* no encoding yet
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
    */
    else goto theEnd;
    MPEG3_Reset(adsi, aad);

    return MMSYSERR_NOERROR;

 theEnd:
    HeapFree(GetProcessHeap(), 0, aad);
    adsi->dwDriver = 0L;
    return MMSYSERR_NOTSUPPORTED;
}

/***********************************************************************
 *           MPEG3_StreamClose
 *
 */
static	LRESULT	MPEG3_StreamClose(PACMDRVSTREAMINSTANCE adsi)
{
    mpg123_close(((AcmMpeg3Data*)adsi->dwDriver)->mh);
    mpg123_delete(((AcmMpeg3Data*)adsi->dwDriver)->mh);
    HeapFree(GetProcessHeap(), 0, (void*)adsi->dwDriver);
    return MMSYSERR_NOERROR;
}

#elif defined(HAVE_AUDIOFILESTREAMOPEN)

typedef struct tagAcmMpeg3Data
{
    LRESULT (*convert)(PACMDRVSTREAMINSTANCE adsi,
            const unsigned char*, LPDWORD, unsigned char*, LPDWORD);
    AudioConverterRef acr;
    AudioStreamBasicDescription in,out;
    AudioFileStreamID afs;

    AudioBufferList outBuffer;
    AudioBuffer inBuffer;

    UInt32 NumberPackets;
    AudioStreamPacketDescription *PacketDescriptions;

    OSStatus lastError;
} AcmMpeg3Data;

static inline const char* wine_dbgstr_fourcc(ULONG fourcc)
{
    char buf[4] = { (char) (fourcc >> 24), (char) (fourcc >> 16),
                    (char) (fourcc >> 8),  (char) fourcc };
    return wine_dbgstr_an(buf, sizeof(buf));
}

/***********************************************************************
 *           MPEG3_drvOpen
 */
static LRESULT MPEG3_drvOpen(LPCSTR str)
{
    return 1;
}

/***********************************************************************
 *           MPEG3_drvClose
 */
static LRESULT MPEG3_drvClose(DWORD_PTR dwDevID)
{
    return 1;
}


static OSStatus Mp3AudioConverterComplexInputDataProc (
   AudioConverterRef             inAudioConverter,
   UInt32                        *ioNumberDataPackets,
   AudioBufferList               *ioData,
   AudioStreamPacketDescription  **outDataPacketDescription,
   void                          *inUserData
)
{
    AcmMpeg3Data *amd = (AcmMpeg3Data*)inUserData;

    if (amd->inBuffer.mDataByteSize > 0)
    {
        *ioNumberDataPackets = amd->NumberPackets;
        ioData->mNumberBuffers = 1;
        ioData->mBuffers[0].mDataByteSize =  amd->inBuffer.mDataByteSize;
        ioData->mBuffers[0].mData =  amd->inBuffer.mData;
        ioData->mBuffers[0].mNumberChannels =  amd->inBuffer.mNumberChannels;
        amd->inBuffer.mDataByteSize = 0;
        if (outDataPacketDescription)
            *outDataPacketDescription = amd->PacketDescriptions;
    }
    else
        *ioNumberDataPackets = 0;
    return noErr;
}

static LRESULT mp3_leopard_horse(PACMDRVSTREAMINSTANCE adsi,
                      const unsigned char* src, LPDWORD nsrc,
                      unsigned char* dst, LPDWORD ndst)
{
    AcmMpeg3Data*       amd = (AcmMpeg3Data*)adsi->dwDriver;
    OSStatus            ret;

    TRACE("ndst %u %p  <-  %u %p\n",*ndst,dst,*nsrc, src);
    amd->outBuffer.mNumberBuffers = 1;
    amd->outBuffer.mBuffers[0].mDataByteSize =  *ndst;
    amd->outBuffer.mBuffers[0].mData = dst;
    amd->outBuffer.mBuffers[0].mNumberChannels =  amd->out.mChannelsPerFrame;

    memset(dst,0xff,*ndst);
    ret = AudioFileStreamParseBytes( amd->afs, *nsrc, src, 0 );

    if (ret != noErr)
    {
        *nsrc = 0;
        ERR("Feed Error %s\n", wine_dbgstr_fourcc(ret));
        return MMSYSERR_ERROR;
    }
    else if (amd->lastError != noErr)
    {
        *nsrc = 0;
        ERR("Error during feed %s\n", wine_dbgstr_fourcc(ret));
        return MMSYSERR_ERROR;
    }

    *ndst = amd->outBuffer.mBuffers[0].mDataByteSize;
    *nsrc = amd->inBuffer.mDataByteSize;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_Reset
 *
 */
static void MPEG3_Reset(PACMDRVSTREAMINSTANCE adsi, AcmMpeg3Data* aad)
{
    SInt64 offset;
    AudioConverterReset(aad->acr);
    AudioFileStreamSeek(aad->afs, 0, &offset, 0);
}

static void Mp3PropertyListenerProc ( void *inClientData,
           AudioFileStreamID inAudioFileStream, UInt32 inPropertyID, UInt32 *ioFlags)
{
    /* No operation at this time */
}

static void Mp3PacketsProc ( void *inClientData, UInt32 inNumberBytes,
   UInt32 inNumberPackets, const void *inInputData,
   AudioStreamPacketDescription *inPacketDescriptions)
{
    AcmMpeg3Data *amd = (AcmMpeg3Data*)inClientData;
    UInt32 size;

    amd->inBuffer.mDataByteSize = inNumberBytes;
    amd->inBuffer.mData = (void*)inInputData;
    amd->inBuffer.mNumberChannels =  amd->in.mChannelsPerFrame;

    amd->NumberPackets = inNumberPackets;
    amd->PacketDescriptions = inPacketDescriptions;

    size = amd->outBuffer.mBuffers[0].mDataByteSize / amd->out.mBytesPerPacket;
    amd->lastError = AudioConverterFillComplexBuffer(amd->acr, Mp3AudioConverterComplexInputDataProc, inClientData, &size, &amd->outBuffer, NULL);
}

/***********************************************************************
 *           MPEG3_StreamOpen
 *
 */
static LRESULT MPEG3_StreamOpen(PACMDRVSTREAMINSTANCE adsi)
{
    AcmMpeg3Data* aad;

    assert(!(adsi->fdwOpen & ACM_STREAMOPENF_ASYNC));

    if (MPEG3_GetFormatIndex(adsi->pwfxSrc) == 0xFFFFFFFF ||
        MPEG3_GetFormatIndex(adsi->pwfxDst) == 0xFFFFFFFF)
        return ACMERR_NOTPOSSIBLE;

    aad = HeapAlloc(GetProcessHeap(), 0, sizeof(AcmMpeg3Data));
    if (aad == 0) return MMSYSERR_NOMEM;

    adsi->dwDriver = (DWORD_PTR)aad;

    if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
        adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
        goto theEnd;
    }
    else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEGLAYER3 &&
             adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
    {
        OSStatus err;

        aad->in.mSampleRate = adsi->pwfxSrc->nSamplesPerSec;
        aad->out.mSampleRate = adsi->pwfxDst->nSamplesPerSec;
        aad->in.mBitsPerChannel = adsi->pwfxSrc->wBitsPerSample;
        aad->out.mBitsPerChannel = adsi->pwfxDst->wBitsPerSample;
        aad->in.mFormatID = kAudioFormatMPEGLayer3;
        aad->out.mFormatID = kAudioFormatLinearPCM;
        aad->in.mChannelsPerFrame = adsi->pwfxSrc->nChannels;
        aad->out.mChannelsPerFrame = adsi->pwfxDst->nChannels;
        aad->in.mFormatFlags = 0;
        aad->out.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
        aad->in.mBytesPerFrame = 0;

        aad->out.mBytesPerFrame = (aad->out.mBitsPerChannel * aad->out.mChannelsPerFrame) / 8;
        aad->in.mBytesPerPacket =  0;
        aad->out.mBytesPerPacket = aad->out.mBytesPerFrame;
        aad->in.mFramesPerPacket = 0;
        aad->out.mFramesPerPacket = 1;
        aad->in.mReserved = aad->out.mReserved = 0;

        aad->acr = NULL;

        err = AudioConverterNew(&aad->in, &aad->out ,&aad->acr);
        if (err != noErr)
        {
            ERR("Create failed: %s\n", wine_dbgstr_fourcc(err));
            goto theEnd;
        }
        else
        {
            aad->convert = mp3_leopard_horse;
            err = AudioFileStreamOpen(aad, Mp3PropertyListenerProc, Mp3PacketsProc, kAudioFormatMPEGLayer3, &aad->afs);
            if (err != noErr)
            {
                ERR("Stream Open failed: %s\n", wine_dbgstr_fourcc(err));
                goto theEnd;
            }
        }
    }
    else goto theEnd;
    MPEG3_Reset(adsi, aad);

    return MMSYSERR_NOERROR;

 theEnd:
    HeapFree(GetProcessHeap(), 0, aad);
    adsi->dwDriver = 0L;
    return MMSYSERR_NOTSUPPORTED;
}

/***********************************************************************
 *           MPEG3_StreamClose
 *
 */
static LRESULT MPEG3_StreamClose(PACMDRVSTREAMINSTANCE adsi)
{
    AudioConverterDispose(((AcmMpeg3Data*)adsi->dwDriver)->acr);
    AudioFileStreamClose(((AcmMpeg3Data*)adsi->dwDriver)->afs);
    HeapFree(GetProcessHeap(), 0, (void*)adsi->dwDriver);
    return MMSYSERR_NOERROR;
}

#endif

/***********************************************************************
 *           MPEG3_DriverDetails
 *
 */
static	LRESULT MPEG3_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    add->wMid = 0xFF;
    add->wPid = 0x00;
    add->vdwACM = 0x01000000;
    add->vdwDriver = 0x01000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    add->cFormatTags = 2; /* PCM, MPEG3 */
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "WINE-MPEG3", -1,
                         add->szShortName, sizeof(add->szShortName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Wine MPEG3 decoder", -1,
                         add->szLongName, sizeof(add->szLongName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, sizeof(add->szCopyright)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, sizeof(add->szLicensing)/sizeof(WCHAR) );
    add->szFeatures[0] = 0;

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_FormatTagDetails
 *
 */
static	LRESULT	MPEG3_FormatTagDetails(PACMFORMATTAGDETAILSW aftd, DWORD dwQuery)
{
    static const WCHAR szPcm[]={'P','C','M',0};
    static const WCHAR szMpeg3[]={'M','P','e','g','3',0};

    switch (dwQuery)
    {
    case ACM_FORMATTAGDETAILSF_INDEX:
	if (aftd->dwFormatTagIndex >= 2) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATTAGDETAILSF_LARGESTSIZE:
	if (aftd->dwFormatTag == WAVE_FORMAT_UNKNOWN)
        {
            aftd->dwFormatTagIndex = 1; /* WAVE_FORMAT_MPEGLAYER3 is bigger than PCM */
	    break;
	}
	/* fall thru */
    case ACM_FORMATTAGDETAILSF_FORMATTAG:
	switch (aftd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:		aftd->dwFormatTagIndex = 0; break;
	case WAVE_FORMAT_MPEGLAYER3:    aftd->dwFormatTagIndex = 1; break;
	default:			return ACMERR_NOTPOSSIBLE;
	}
	break;
    default:
	WARN("Unsupported query %08x\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }

    aftd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    switch (aftd->dwFormatTagIndex)
    {
    case 0:
	aftd->dwFormatTag = WAVE_FORMAT_PCM;
	aftd->cbFormatSize = sizeof(PCMWAVEFORMAT);
	aftd->cStandardFormats = NUM_PCM_FORMATS;
        lstrcpyW(aftd->szFormatTag, szPcm);
        break;
    case 1:
	aftd->dwFormatTag = WAVE_FORMAT_MPEGLAYER3;
	aftd->cbFormatSize = sizeof(MPEGLAYER3WAVEFORMAT);
	aftd->cStandardFormats = NUM_MPEG3_FORMATS;
        lstrcpyW(aftd->szFormatTag, szMpeg3);
	break;
    }
    return MMSYSERR_NOERROR;
}

static void fill_in_wfx(unsigned cbwfx, WAVEFORMATEX* wfx, unsigned bit_rate)
{
    MPEGLAYER3WAVEFORMAT*   mp3wfx = (MPEGLAYER3WAVEFORMAT*)wfx;

    wfx->nAvgBytesPerSec = bit_rate / 8;
    if (cbwfx >= sizeof(WAVEFORMATEX))
        wfx->cbSize = sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX);
    if (cbwfx >= sizeof(MPEGLAYER3WAVEFORMAT))
    {
        mp3wfx->wID = MPEGLAYER3_ID_MPEG;
        mp3wfx->fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
        mp3wfx->nBlockSize = (bit_rate * 144) / wfx->nSamplesPerSec;
        mp3wfx->nFramesPerBlock = 1;
        mp3wfx->nCodecDelay = 0x0571;
    }
}

/***********************************************************************
 *           MPEG3_FormatDetails
 *
 */
static	LRESULT	MPEG3_FormatDetails(PACMFORMATDETAILSW afd, DWORD dwQuery)
{
    switch (dwQuery)
    {
    case ACM_FORMATDETAILSF_FORMAT:
	if (MPEG3_GetFormatIndex(afd->pwfx) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
	break;
    case ACM_FORMATDETAILSF_INDEX:
	afd->pwfx->wFormatTag = afd->dwFormatTag;
	switch (afd->dwFormatTag)
        {
	case WAVE_FORMAT_PCM:
	    if (afd->dwFormatIndex >= NUM_PCM_FORMATS) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = PCM_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nSamplesPerSec = PCM_Formats[afd->dwFormatIndex].rate;
	    afd->pwfx->wBitsPerSample = PCM_Formats[afd->dwFormatIndex].nBits;
	    /* native MSACM uses a PCMWAVEFORMAT structure, so cbSize is not accessible
	     * afd->pwfx->cbSize = 0;
	     */
	    afd->pwfx->nBlockAlign =
		(afd->pwfx->nChannels * afd->pwfx->wBitsPerSample) / 8;
	    afd->pwfx->nAvgBytesPerSec =
		afd->pwfx->nSamplesPerSec * afd->pwfx->nBlockAlign;
	    break;
	case WAVE_FORMAT_MPEGLAYER3:
	    if (afd->dwFormatIndex >= NUM_MPEG3_FORMATS) return ACMERR_NOTPOSSIBLE;
	    afd->pwfx->nChannels = MPEG3_Formats[afd->dwFormatIndex].nChannels;
	    afd->pwfx->nSamplesPerSec = MPEG3_Formats[afd->dwFormatIndex].rate;
	    afd->pwfx->wBitsPerSample = MPEG3_Formats[afd->dwFormatIndex].nBits;
	    afd->pwfx->nBlockAlign = 1;
            fill_in_wfx(afd->cbwfx, afd->pwfx, 192000);
	    break;
	default:
            WARN("Unsupported tag %08x\n", afd->dwFormatTag);
	    return MMSYSERR_INVALPARAM;
	}
	break;
    default:
	WARN("Unsupported query %08x\n", dwQuery);
	return MMSYSERR_NOTSUPPORTED;
    }
    afd->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    afd->szFormat[0] = 0; /* let MSACM format this for us... */

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_FormatSuggest
 *
 */
static	LRESULT	MPEG3_FormatSuggest(PACMDRVFORMATSUGGEST adfs)
{
    /* some tests ... */
    if (adfs->cbwfxSrc < sizeof(PCMWAVEFORMAT) ||
	adfs->cbwfxDst < sizeof(PCMWAVEFORMAT) ||
	MPEG3_GetFormatIndex(adfs->pwfxSrc) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;
    /* FIXME: should do those tests against the real size (according to format tag */

    /* If no suggestion for destination, then copy source value */
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NCHANNELS))
	adfs->pwfxDst->nChannels = adfs->pwfxSrc->nChannels;
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_NSAMPLESPERSEC))
        adfs->pwfxDst->nSamplesPerSec = adfs->pwfxSrc->nSamplesPerSec;

    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WBITSPERSAMPLE))
    {
	if (adfs->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
            adfs->pwfxDst->wBitsPerSample = 4;
        else
            adfs->pwfxDst->wBitsPerSample = 16;
    }
    if (!(adfs->fdwSuggest & ACM_FORMATSUGGESTF_WFORMATTAG))
    {
	if (adfs->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM)
            adfs->pwfxDst->wFormatTag = WAVE_FORMAT_MPEGLAYER3;
        else
            adfs->pwfxDst->wFormatTag = WAVE_FORMAT_PCM;
    }

    /* check if result is ok */
    if (MPEG3_GetFormatIndex(adfs->pwfxDst) == 0xFFFFFFFF) return ACMERR_NOTPOSSIBLE;

    /* recompute other values */
    switch (adfs->pwfxDst->wFormatTag)
    {
    case WAVE_FORMAT_PCM:
        adfs->pwfxDst->nBlockAlign = (adfs->pwfxDst->nChannels * adfs->pwfxDst->wBitsPerSample) / 8;
        adfs->pwfxDst->nAvgBytesPerSec = adfs->pwfxDst->nSamplesPerSec * adfs->pwfxDst->nBlockAlign;
        break;
    case WAVE_FORMAT_MPEGLAYER3:
        adfs->pwfxDst->nBlockAlign = 1;
        fill_in_wfx(adfs->cbwfxDst, adfs->pwfxDst, 192000);
        break;
    default:
        FIXME("\n");
        break;
    }

    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_StreamSize
 *
 */
static	LRESULT MPEG3_StreamSize(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMSIZE adss)
{
    DWORD nblocks;

    switch (adss->fdwSize)
    {
    case ACM_STREAMSIZEF_DESTINATION:
	/* cbDstLength => cbSrcLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	    adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
        {
            nblocks = (adss->cbDstLength - 3000) / (DWORD)(adsi->pwfxDst->nAvgBytesPerSec * 1152 / adsi->pwfxDst->nSamplesPerSec + 0.5);
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            adss->cbSrcLength = nblocks * 1152 * adsi->pwfxSrc->nBlockAlign;
	}
        else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEGLAYER3 &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
            nblocks = adss->cbDstLength / (adsi->pwfxDst->nBlockAlign * 1152);
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            adss->cbSrcLength = nblocks * (DWORD)(adsi->pwfxSrc->nAvgBytesPerSec * 1152 / adsi->pwfxSrc->nSamplesPerSec);
	}
        else
        {
	    return MMSYSERR_NOTSUPPORTED;
	}
	break;
    case ACM_STREAMSIZEF_SOURCE:
	/* cbSrcLength => cbDstLength */
	if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_PCM &&
	    adsi->pwfxDst->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
        {
            nblocks = adss->cbSrcLength / (adsi->pwfxSrc->nBlockAlign * 1152);
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            if (adss->cbSrcLength % (DWORD)(adsi->pwfxSrc->nBlockAlign * 1152))
                /* Round block count up. */
                nblocks++;
            adss->cbDstLength = 3000 + nblocks * (DWORD)(adsi->pwfxDst->nAvgBytesPerSec * 1152 / adsi->pwfxDst->nSamplesPerSec + 0.5);
	}
        else if (adsi->pwfxSrc->wFormatTag == WAVE_FORMAT_MPEGLAYER3 &&
                 adsi->pwfxDst->wFormatTag == WAVE_FORMAT_PCM)
        {
            nblocks = adss->cbSrcLength / (DWORD)(adsi->pwfxSrc->nAvgBytesPerSec * 1152 / adsi->pwfxSrc->nSamplesPerSec);
            if (nblocks == 0)
                return ACMERR_NOTPOSSIBLE;
            if (adss->cbSrcLength % (DWORD)(adsi->pwfxSrc->nAvgBytesPerSec * 1152 / adsi->pwfxSrc->nSamplesPerSec))
                /* Round block count up. */
                nblocks++;
            adss->cbDstLength = nblocks * 1152 * adsi->pwfxDst->nBlockAlign;
	}
        else
        {
	    return MMSYSERR_NOTSUPPORTED;
	}
	break;
    default:
	WARN("Unsupported query %08x\n", adss->fdwSize);
	return MMSYSERR_NOTSUPPORTED;
    }
    return MMSYSERR_NOERROR;
}

/***********************************************************************
 *           MPEG3_StreamConvert
 *
 */
static LRESULT MPEG3_StreamConvert(PACMDRVSTREAMINSTANCE adsi, PACMDRVSTREAMHEADER adsh)
{
    AcmMpeg3Data*	aad = (AcmMpeg3Data*)adsi->dwDriver;
    DWORD		nsrc = adsh->cbSrcLength;
    DWORD		ndst = adsh->cbDstLength;

    if (adsh->fdwConvert &
	~(ACM_STREAMCONVERTF_BLOCKALIGN|
	  ACM_STREAMCONVERTF_END|
	  ACM_STREAMCONVERTF_START))
    {
	FIXME("Unsupported fdwConvert (%08x), ignoring it\n", adsh->fdwConvert);
    }
    /* ACM_STREAMCONVERTF_BLOCKALIGN
     *	currently all conversions are block aligned, so do nothing for this flag
     * ACM_STREAMCONVERTF_END
     *	no pending data, so do nothing for this flag
     */
    if ((adsh->fdwConvert & ACM_STREAMCONVERTF_START))
    {
        MPEG3_Reset(adsi, aad);
    }

    aad->convert(adsi, adsh->pbSrc, &nsrc, adsh->pbDst, &ndst);
    adsh->cbSrcLengthUsed = nsrc;
    adsh->cbDstLengthUsed = ndst;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			MPEG3_DriverProc			[exported]
 */
LRESULT CALLBACK MPEG3_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
					 LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lx %p %04x %08lx %08lx);\n",
	  dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return MPEG3_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:		return MPEG3_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MPEG3 filter !", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;

    case ACMDM_DRIVER_DETAILS:
	return MPEG3_DriverDetails((PACMDRIVERDETAILSW)dwParam1);

    case ACMDM_FORMATTAG_DETAILS:
	return MPEG3_FormatTagDetails((PACMFORMATTAGDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_DETAILS:
	return MPEG3_FormatDetails((PACMFORMATDETAILSW)dwParam1, dwParam2);

    case ACMDM_FORMAT_SUGGEST:
	return MPEG3_FormatSuggest((PACMDRVFORMATSUGGEST)dwParam1);

    case ACMDM_STREAM_OPEN:
	return MPEG3_StreamOpen((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_CLOSE:
	return MPEG3_StreamClose((PACMDRVSTREAMINSTANCE)dwParam1);

    case ACMDM_STREAM_SIZE:
	return MPEG3_StreamSize((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMSIZE)dwParam2);

    case ACMDM_STREAM_CONVERT:
	return MPEG3_StreamConvert((PACMDRVSTREAMINSTANCE)dwParam1, (PACMDRVSTREAMHEADER)dwParam2);

    case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
    case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
	/* this converter is not a hardware driver */
    case ACMDM_FILTERTAG_DETAILS:
    case ACMDM_FILTER_DETAILS:
	/* this converter is not a filter */
    case ACMDM_STREAM_RESET:
	/* only needed for asynchronous driver... we aren't, so just say it */
	return MMSYSERR_NOTSUPPORTED;
    case ACMDM_STREAM_PREPARE:
    case ACMDM_STREAM_UNPREPARE:
	/* nothing special to do here... so don't do anything */
	return MMSYSERR_NOERROR;

    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
