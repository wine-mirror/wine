/*
 * Wine Driver for CoreAudio based on Jack Driver
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1999 Eric Pouech (async playing in waveOut/waveIn)
 * Copyright 2000 Eric Pouech (loops in waveOut)
 * Copyright 2002 Chris Morgan (jack version of this file)
 * Copyright 2005, 2006 Emmanuel Maillard
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <assert.h>

#ifdef HAVE_COREAUDIO_COREAUDIO_H
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <libkern/OSAtomic.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winerror.h"
#include "mmddk.h"
#include "mmreg.h"
#include "dsound.h"
#include "dsdriver.h"
#include "ks.h"
#include "ksguid.h"
#include "ksmedia.h"
#include "coreaudio.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#if defined(HAVE_COREAUDIO_COREAUDIO_H) && defined(HAVE_AUDIOUNIT_AUDIOUNIT_H)

WINE_DECLARE_DEBUG_CHANNEL(coreaudio);

/*
    Due to AudioUnit headers conflict define some needed types.
*/

typedef void *AudioUnit;

/* From AudioUnit/AUComponents.h */
enum
{
    kAudioUnitRenderAction_OutputIsSilence  = (1 << 4),
        /* provides hint on return from Render(): if set the buffer contains all zeroes */
};
typedef UInt32 AudioUnitRenderActionFlags;

typedef long ComponentResult;
extern ComponentResult
AudioUnitRender(                    AudioUnit                       ci,
                                    AudioUnitRenderActionFlags *    ioActionFlags,
                                    const AudioTimeStamp *          inTimeStamp,
                                    UInt32                          inOutputBusNumber,
                                    UInt32                          inNumberFrames,
                                    AudioBufferList *               ioData)         AVAILABLE_MAC_OS_X_VERSION_10_2_AND_LATER;

/* only allow 10 output devices through this driver, this ought to be adequate */
#define MAX_WAVEOUTDRV  (1)
#define MAX_WAVEINDRV   (1)

/* state diagram for waveOut writing:
*
* +---------+-------------+---------------+---------------------------------+
* |  state  |  function   |     event     |            new state	     |
* +---------+-------------+---------------+---------------------------------+
* |	    | open()	   |		   | PLAYING		       	     |
* | PAUSED  | write()	   | 		   | PAUSED		       	     |
* | PLAYING | write()	   | HEADER        | PLAYING		  	     |
* | (other) | write()	   | <error>       |		       		     |
* | (any)   | pause()	   | PAUSING	   | PAUSED		       	     |
* | PAUSED  | restart()    | RESTARTING    | PLAYING                         |
* | (any)   | reset()	   | RESETTING     | PLAYING		      	     |
* | (any)   | close()	   | CLOSING	   | <deallocated>	      	     |
* +---------+-------------+---------------+---------------------------------+
*/

/* states of the playing device */
#define	WINE_WS_PLAYING   0 /* for waveOut: lpPlayPtr == NULL -> stopped */
#define	WINE_WS_PAUSED    1
#define	WINE_WS_STOPPED   2 /* Not used for waveOut */
#define WINE_WS_CLOSED    3 /* Not used for waveOut */
#define WINE_WS_OPENING   4
#define WINE_WS_CLOSING   5

typedef struct tagCoreAudio_Device {
    char                        dev_name[32];
    char                        mixer_name[32];
    unsigned                    open_count;
    char*                       interface_name;
    
    WAVEOUTCAPSW                out_caps;
    WAVEINCAPSW                 in_caps;
    DWORD                       in_caps_support;
    int                         sample_rate;
    int                         stereo;
    int                         format;
    unsigned                    audio_fragment;
    BOOL                        full_duplex;
    BOOL                        bTriggerSupport;
    BOOL                        bOutputEnabled;
    BOOL                        bInputEnabled;
    DSDRIVERDESC                ds_desc;
    DSDRIVERCAPS                ds_caps;
    DSCDRIVERCAPS               dsc_caps;
    GUID                        ds_guid;
    GUID                        dsc_guid;
    
    AudioDeviceID outputDeviceID;
    AudioDeviceID inputDeviceID;
    AudioStreamBasicDescription streamDescription;
} CoreAudio_Device;

/* for now use the default device */
static CoreAudio_Device CoreAudio_DefaultDevice;

typedef struct {
    struct list                 entry;

    volatile int                state;      /* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC                waveDesc;
    WORD                        wFlags;
    PCMWAVEFORMAT               format;
    DWORD                       woID;
    AudioUnit                   audioUnit;
    AudioStreamBasicDescription streamDescription;

    LPWAVEHDR                   lpQueuePtr;             /* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR                   lpPlayPtr;              /* start of not yet fully played buffers */
    DWORD                       dwPartialOffset;        /* Offset of not yet written bytes in lpPlayPtr */

    LPWAVEHDR                   lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD                       dwLoops;                /* private copy of loop counter */

    DWORD                       dwPlayedTotal;          /* number of bytes actually played since opening */

    OSSpinLock                  lock;         /* synchronization stuff */
} WINE_WAVEOUT_INSTANCE;

typedef struct {
    CoreAudio_Device            *cadev;
    WAVEOUTCAPSW                caps;
    char                        interface_name[32];
    DWORD                       device_volume;

    BOOL trace_on;
    BOOL warn_on;
    BOOL err_on;

    struct list                 instances;
    OSSpinLock                  lock;         /* guards the instances list */
} WINE_WAVEOUT;

typedef struct {
    /* This device's device number */
    DWORD           wiID;

    /* Access to the following fields is synchronized across threads. */
    volatile int    state;
    LPWAVEHDR       lpQueuePtr;
    DWORD           dwTotalRecorded;

    /* Synchronization mechanism to protect above fields */
    OSSpinLock      lock;

    /* Capabilities description */
    WAVEINCAPSW     caps;
    char            interface_name[32];

    /* Record the arguments used when opening the device. */
    WAVEOPENDESC    waveDesc;
    WORD            wFlags;
    PCMWAVEFORMAT   format;

    AudioUnit       audioUnit;
    AudioBufferList*bufferList;
    AudioBufferList*bufferListCopy;

    /* Record state of debug channels at open.  Used to control fprintf's since
     * we can't use Wine debug channel calls in non-Wine AudioUnit threads. */
    BOOL            trace_on;
    BOOL            warn_on;
    BOOL            err_on;

/* These fields aren't used. */
#if 0
    CoreAudio_Device *cadev;

    AudioStreamBasicDescription streamDescription;
#endif
} WINE_WAVEIN;

static WINE_WAVEOUT WOutDev   [MAX_WAVEOUTDRV];
static WINE_WAVEIN  WInDev    [MAX_WAVEINDRV];

static HANDLE hThread = NULL; /* Track the thread we create so we can clean it up later */
static CFMessagePortRef Port_SendToMessageThread;

static void wodHelper_PlayPtrNext(WINE_WAVEOUT_INSTANCE* wwo);
static void wodHelper_NotifyDoneForList(WINE_WAVEOUT_INSTANCE* wwo, LPWAVEHDR lpWaveHdr);
static void wodHelper_NotifyCompletions(WINE_WAVEOUT_INSTANCE* wwo, BOOL force);
static void widHelper_NotifyCompletions(WINE_WAVEIN* wwi);

extern int AudioUnit_CreateDefaultAudioUnit(void *wwo, AudioUnit *au);
extern int AudioUnit_CloseAudioUnit(AudioUnit au);
extern int AudioUnit_InitializeWithStreamDescription(AudioUnit au, AudioStreamBasicDescription *streamFormat);

extern OSStatus AudioOutputUnitStart(AudioUnit au);
extern OSStatus AudioOutputUnitStop(AudioUnit au);
extern OSStatus AudioUnitUninitialize(AudioUnit au);

extern int AudioUnit_SetVolume(AudioUnit au, float left, float right);
extern int AudioUnit_GetVolume(AudioUnit au, float *left, float *right);

extern int AudioUnit_GetInputDeviceSampleRate(void);

extern int AudioUnit_CreateInputUnit(void* wwi, AudioUnit* out_au,
        WORD nChannels, DWORD nSamplesPerSec, WORD wBitsPerSample,
        UInt32* outFrameCount);

OSStatus CoreAudio_woAudioUnitIOProc(void *inRefCon, 
                                     AudioUnitRenderActionFlags *ioActionFlags, 
                                     const AudioTimeStamp *inTimeStamp, 
                                     UInt32 inBusNumber, 
                                     UInt32 inNumberFrames, 
                                     AudioBufferList *ioData);
OSStatus CoreAudio_wiAudioUnitIOProc(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList *ioData);

/* These strings used only for tracing */

static const char * getMessage(UINT msg)
{
#define MSG_TO_STR(x) case x: return #x
    switch(msg) {
        MSG_TO_STR(DRVM_INIT);
        MSG_TO_STR(DRVM_EXIT);
        MSG_TO_STR(DRVM_ENABLE);
        MSG_TO_STR(DRVM_DISABLE);
        MSG_TO_STR(WIDM_OPEN);
        MSG_TO_STR(WIDM_CLOSE);
        MSG_TO_STR(WIDM_ADDBUFFER);
        MSG_TO_STR(WIDM_PREPARE);
        MSG_TO_STR(WIDM_UNPREPARE);
        MSG_TO_STR(WIDM_GETDEVCAPS);
        MSG_TO_STR(WIDM_GETNUMDEVS);
        MSG_TO_STR(WIDM_GETPOS);
        MSG_TO_STR(WIDM_RESET);
        MSG_TO_STR(WIDM_START);
        MSG_TO_STR(WIDM_STOP);
        MSG_TO_STR(WODM_OPEN);
        MSG_TO_STR(WODM_CLOSE);
        MSG_TO_STR(WODM_WRITE);
        MSG_TO_STR(WODM_PAUSE);
        MSG_TO_STR(WODM_GETPOS);
        MSG_TO_STR(WODM_BREAKLOOP);
        MSG_TO_STR(WODM_PREPARE);
        MSG_TO_STR(WODM_UNPREPARE);
        MSG_TO_STR(WODM_GETDEVCAPS);
        MSG_TO_STR(WODM_GETNUMDEVS);
        MSG_TO_STR(WODM_GETPITCH);
        MSG_TO_STR(WODM_SETPITCH);
        MSG_TO_STR(WODM_GETPLAYBACKRATE);
        MSG_TO_STR(WODM_SETPLAYBACKRATE);
        MSG_TO_STR(WODM_GETVOLUME);
        MSG_TO_STR(WODM_SETVOLUME);
        MSG_TO_STR(WODM_RESTART);
        MSG_TO_STR(WODM_RESET);
        MSG_TO_STR(DRV_QUERYDEVICEINTERFACESIZE);
        MSG_TO_STR(DRV_QUERYDEVICEINTERFACE);
        MSG_TO_STR(DRV_QUERYDSOUNDIFACE);
        MSG_TO_STR(DRV_QUERYDSOUNDDESC);
    }
#undef MSG_TO_STR
    return wine_dbg_sprintf("UNKNOWN(0x%04x)", msg);
}

#define kStopLoopMessage 0
#define kWaveOutNotifyCompletionsMessage 1
#define kWaveInNotifyCompletionsMessage 2

/* Mach Message Handling */
static CFDataRef wodMessageHandler(CFMessagePortRef port_ReceiveInMessageThread, SInt32 msgid, CFDataRef data, void *info)
{
    UInt32 *buffer = NULL;

    switch (msgid)
    {
        case kWaveOutNotifyCompletionsMessage:
            wodHelper_NotifyCompletions(*(WINE_WAVEOUT_INSTANCE**)CFDataGetBytePtr(data), FALSE);
            break;
        case kWaveInNotifyCompletionsMessage:
            buffer = (UInt32 *) CFDataGetBytePtr(data);
            widHelper_NotifyCompletions(&WInDev[buffer[0]]);
            break;
        default:
            CFRunLoopStop(CFRunLoopGetCurrent());
            break;
    }
    
    return NULL;
}

static DWORD WINAPI messageThread(LPVOID p)
{
    CFMessagePortRef port_ReceiveInMessageThread = (CFMessagePortRef) p;
    CFRunLoopSourceRef source;

    source = CFMessagePortCreateRunLoopSource(kCFAllocatorDefault, port_ReceiveInMessageThread, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);

    CFRunLoopRun();

    CFRunLoopSourceInvalidate(source);
    CFRelease(source);
    CFRelease(port_ReceiveInMessageThread);

    return 0;
}

/**************************************************************************
* 			wodSendNotifyCompletionsMessage			[internal]
*   Call from AudioUnit IO thread can't use Wine debug channels.
*/
static void wodSendNotifyCompletionsMessage(WINE_WAVEOUT_INSTANCE* wwo)
{
    CFDataRef data;

    if (!Port_SendToMessageThread)
        return;

    data = CFDataCreate(kCFAllocatorDefault, (UInt8 *)&wwo, sizeof(wwo));
    if (!data)
        return;

    CFMessagePortSendRequest(Port_SendToMessageThread, kWaveOutNotifyCompletionsMessage, data, 0.0, 0.0, NULL, NULL);
    CFRelease(data);
}

/**************************************************************************
*                       wodSendNotifyInputCompletionsMessage     [internal]
*   Call from AudioUnit IO thread can't use Wine debug channels.
*/
static void wodSendNotifyInputCompletionsMessage(WINE_WAVEIN* wwi)
{
    CFDataRef data;
    UInt32 buffer;

    if (!Port_SendToMessageThread)
        return;

    buffer = (UInt32) wwi->wiID;

    data = CFDataCreate(kCFAllocatorDefault, (UInt8 *)&buffer, sizeof(buffer));
    if (!data)
        return;

    CFMessagePortSendRequest(Port_SendToMessageThread, kWaveInNotifyCompletionsMessage, data, 0.0, 0.0, NULL, NULL);
    CFRelease(data);
}

static DWORD bytes_to_mmtime(LPMMTIME lpTime, DWORD position,
                             PCMWAVEFORMAT* format)
{
    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%u nChannels=%u nAvgBytesPerSec=%u\n",
          lpTime->wType, format->wBitsPerSample, format->wf.nSamplesPerSec,
          format->wf.nChannels, format->wf.nAvgBytesPerSec);
    TRACE("Position in bytes=%u\n", position);

    switch (lpTime->wType) {
    case TIME_SAMPLES:
        lpTime->u.sample = position / (format->wBitsPerSample / 8 * format->wf.nChannels);
        TRACE("TIME_SAMPLES=%u\n", lpTime->u.sample);
        break;
    case TIME_MS:
        lpTime->u.ms = 1000.0 * position / (format->wBitsPerSample / 8 * format->wf.nChannels * format->wf.nSamplesPerSec);
        TRACE("TIME_MS=%u\n", lpTime->u.ms);
        break;
    case TIME_SMPTE:
        lpTime->u.smpte.fps = 30;
        position = position / (format->wBitsPerSample / 8 * format->wf.nChannels);
        position += (format->wf.nSamplesPerSec / lpTime->u.smpte.fps) - 1; /* round up */
        lpTime->u.smpte.sec = position / format->wf.nSamplesPerSec;
        position -= lpTime->u.smpte.sec * format->wf.nSamplesPerSec;
        lpTime->u.smpte.min = lpTime->u.smpte.sec / 60;
        lpTime->u.smpte.sec -= 60 * lpTime->u.smpte.min;
        lpTime->u.smpte.hour = lpTime->u.smpte.min / 60;
        lpTime->u.smpte.min -= 60 * lpTime->u.smpte.hour;
        lpTime->u.smpte.fps = 30;
        lpTime->u.smpte.frame = position * lpTime->u.smpte.fps / format->wf.nSamplesPerSec;
        TRACE("TIME_SMPTE=%02u:%02u:%02u:%02u\n",
              lpTime->u.smpte.hour, lpTime->u.smpte.min,
              lpTime->u.smpte.sec, lpTime->u.smpte.frame);
        break;
    default:
        WARN("Format %d not supported, using TIME_BYTES !\n", lpTime->wType);
        lpTime->wType = TIME_BYTES;
        /* fall through */
    case TIME_BYTES:
        lpTime->u.cb = position;
        TRACE("TIME_BYTES=%u\n", lpTime->u.cb);
        break;
    }
    return MMSYSERR_NOERROR;
}

static BOOL supportedFormat(LPWAVEFORMATEX wf)
{
    if (wf->nSamplesPerSec < DSBFREQUENCY_MIN || wf->nSamplesPerSec > DSBFREQUENCY_MAX)
        return FALSE;

    if (wf->wFormatTag == WAVE_FORMAT_PCM) {
        if (wf->nChannels >= 1 && wf->nChannels <= 2) {
            if (wf->wBitsPerSample==8||wf->wBitsPerSample==16)
                return TRUE;
	}
    } else if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        WAVEFORMATEXTENSIBLE * wfex = (WAVEFORMATEXTENSIBLE *)wf;

        if (wf->cbSize == 22 && IsEqualGUID(&wfex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) {
            if (wf->nChannels >=1 && wf->nChannels <= 8) {
                if (wf->wBitsPerSample==wfex->Samples.wValidBitsPerSample) {
                    if (wf->wBitsPerSample==8||wf->wBitsPerSample==16)
                        return TRUE;
                } else
                    WARN("wBitsPerSample != wValidBitsPerSample not supported yet\n");
            }
        } else
            WARN("only KSDATAFORMAT_SUBTYPE_PCM supported\n");
    } else
        WARN("only WAVE_FORMAT_PCM and WAVE_FORMAT_EXTENSIBLE supported\n");

    return FALSE;
}

void copyFormat(LPWAVEFORMATEX wf1, LPPCMWAVEFORMAT wf2)
{
    memcpy(wf2, wf1, sizeof(PCMWAVEFORMAT));
    /* Downgrade WAVE_FORMAT_EXTENSIBLE KSDATAFORMAT_SUBTYPE_PCM
     * to smaller yet compatible WAVE_FORMAT_PCM structure */
    if (wf2->wf.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        wf2->wf.wFormatTag = WAVE_FORMAT_PCM;
}

/**************************************************************************
* 			CoreAudio_GetDevCaps            [internal]
*/
BOOL CoreAudio_GetDevCaps (void)
{
    OSStatus status;
    UInt32 propertySize;
    AudioDeviceID devId = CoreAudio_DefaultDevice.outputDeviceID;
    
    char name[MAXPNAMELEN];
    
    propertySize = MAXPNAMELEN;
    status = AudioDeviceGetProperty(devId, 0 , FALSE, kAudioDevicePropertyDeviceName, &propertySize, name);
    if (status) {
        ERR("AudioHardwareGetProperty for kAudioDevicePropertyDeviceName return %s\n", wine_dbgstr_fourcc(status));
        return FALSE;
    }
    
    memcpy(CoreAudio_DefaultDevice.ds_desc.szDesc, name, sizeof(name));
    strcpy(CoreAudio_DefaultDevice.ds_desc.szDrvname, "winecoreaudio.drv");
    MultiByteToWideChar(CP_UNIXCP, 0, name, sizeof(name),
                        CoreAudio_DefaultDevice.out_caps.szPname, 
                        sizeof(CoreAudio_DefaultDevice.out_caps.szPname) / sizeof(WCHAR));
    memcpy(CoreAudio_DefaultDevice.dev_name, name, 32);
    
    propertySize = sizeof(CoreAudio_DefaultDevice.streamDescription);
    status = AudioDeviceGetProperty(devId, 0, FALSE , kAudioDevicePropertyStreamFormat, &propertySize, &CoreAudio_DefaultDevice.streamDescription);
    if (status != noErr) {
        ERR("AudioHardwareGetProperty for kAudioDevicePropertyStreamFormat return %s\n", wine_dbgstr_fourcc(status));
        return FALSE;
    }
    
    TRACE("Device Stream Description mSampleRate : %f\n mFormatID : %s\n"
            "mFormatFlags : %lX\n mBytesPerPacket : %lu\n mFramesPerPacket : %lu\n"
            "mBytesPerFrame : %lu\n mChannelsPerFrame : %lu\n mBitsPerChannel : %lu\n",
                               CoreAudio_DefaultDevice.streamDescription.mSampleRate,
                               wine_dbgstr_fourcc(CoreAudio_DefaultDevice.streamDescription.mFormatID),
                               CoreAudio_DefaultDevice.streamDescription.mFormatFlags,
                               CoreAudio_DefaultDevice.streamDescription.mBytesPerPacket,
                               CoreAudio_DefaultDevice.streamDescription.mFramesPerPacket,
                               CoreAudio_DefaultDevice.streamDescription.mBytesPerFrame,
                               CoreAudio_DefaultDevice.streamDescription.mChannelsPerFrame,
                               CoreAudio_DefaultDevice.streamDescription.mBitsPerChannel);
    
    CoreAudio_DefaultDevice.out_caps.wMid = 0xcafe;
    CoreAudio_DefaultDevice.out_caps.wPid = 0x0001;
    
    CoreAudio_DefaultDevice.out_caps.vDriverVersion = 0x0001;
    CoreAudio_DefaultDevice.out_caps.dwFormats = 0x00000000;
    CoreAudio_DefaultDevice.out_caps.wReserved1 = 0;
    CoreAudio_DefaultDevice.out_caps.dwSupport = WAVECAPS_VOLUME;
    CoreAudio_DefaultDevice.out_caps.dwSupport |= WAVECAPS_LRVOLUME;
    
    CoreAudio_DefaultDevice.out_caps.wChannels = 2;
    CoreAudio_DefaultDevice.out_caps.dwFormats|= WAVE_FORMAT_4S16;

    TRACE_(coreaudio)("out dwFormats = %08x, dwSupport = %08x\n",
           CoreAudio_DefaultDevice.out_caps.dwFormats, CoreAudio_DefaultDevice.out_caps.dwSupport);
    
    return TRUE;
}

/******************************************************************
*		CoreAudio_WaveInit
*
* Initialize CoreAudio_DefaultDevice
*/
LONG CoreAudio_WaveInit(void)
{
    OSStatus status;
    UInt32 propertySize;
    int i;
    CFStringRef  messageThreadPortName;
    CFMessagePortRef port_ReceiveInMessageThread;
    int inputSampleRate;

    TRACE("()\n");
    
    /* number of sound cards */
    AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &propertySize, NULL);
    propertySize /= sizeof(AudioDeviceID);
    TRACE("sound cards : %lu\n", propertySize);
    
    /* Get the output device */
    propertySize = sizeof(CoreAudio_DefaultDevice.outputDeviceID);
    status = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &propertySize, &CoreAudio_DefaultDevice.outputDeviceID);
    if (status) {
        ERR("AudioHardwareGetProperty return %s for kAudioHardwarePropertyDefaultOutputDevice\n", wine_dbgstr_fourcc(status));
        return DRV_FAILURE;
    }
    if (CoreAudio_DefaultDevice.outputDeviceID == kAudioDeviceUnknown) {
        ERR("AudioHardwareGetProperty: CoreAudio_DefaultDevice.outputDeviceID == kAudioDeviceUnknown\n");
        return DRV_FAILURE;
    }
    
    if ( ! CoreAudio_GetDevCaps() )
        return DRV_FAILURE;
    
    CoreAudio_DefaultDevice.interface_name=HeapAlloc(GetProcessHeap(),0,strlen(CoreAudio_DefaultDevice.dev_name)+1);
    strcpy(CoreAudio_DefaultDevice.interface_name, CoreAudio_DefaultDevice.dev_name);
    
    for (i = 0; i < MAX_WAVEOUTDRV; ++i)
    {
        static const WCHAR wszWaveOutFormat[] =
            {'C','o','r','e','A','u','d','i','o',' ','W','a','v','e','O','u','t',' ','%','d',0};

        list_init(&WOutDev[i].instances);
        WOutDev[i].cadev = &CoreAudio_DefaultDevice; 
        
        memset(&WOutDev[i].caps, 0, sizeof(WOutDev[i].caps));
        
        WOutDev[i].caps.wMid = 0xcafe; 	/* Manufac ID */
        WOutDev[i].caps.wPid = 0x0001; 	/* Product ID */
        snprintfW(WOutDev[i].caps.szPname, sizeof(WOutDev[i].caps.szPname)/sizeof(WCHAR), wszWaveOutFormat, i);
        snprintf(WOutDev[i].interface_name, sizeof(WOutDev[i].interface_name), "winecoreaudio: %d", i);
        
        WOutDev[i].caps.vDriverVersion = 0x0001;
        WOutDev[i].caps.dwFormats = 0x00000000;
        WOutDev[i].caps.dwSupport = WAVECAPS_VOLUME;
        
        WOutDev[i].caps.wChannels = 2;
      /*  WOutDev[i].caps.dwSupport |= WAVECAPS_LRVOLUME; */ /* FIXME */
        
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_96M08;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_96S08;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_96M16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_96S16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_48M08;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_48S08;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_48M16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_48S16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4M08;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4S08; 
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4S16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_4M16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2M08;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2S08; 
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2M16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_2S16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1M08;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1S08;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1M16;
        WOutDev[i].caps.dwFormats |= WAVE_FORMAT_1S16;

        WOutDev[i].device_volume = 0xffffffff;

        WOutDev[i].lock = 0; /* initialize the mutex */
    }

    /* FIXME: implement sample rate conversion on input */
    inputSampleRate = AudioUnit_GetInputDeviceSampleRate();

    for (i = 0; i < MAX_WAVEINDRV; ++i)
    {
        static const WCHAR wszWaveInFormat[] =
            {'C','o','r','e','A','u','d','i','o',' ','W','a','v','e','I','n',' ','%','d',0};

        memset(&WInDev[i], 0, sizeof(WInDev[i]));
        WInDev[i].wiID = i;

        /* Establish preconditions for widOpen */
        WInDev[i].state = WINE_WS_CLOSED;
        WInDev[i].lock = 0; /* initialize the mutex */

        /* Fill in capabilities.  widGetDevCaps can be called at any time. */
        WInDev[i].caps.wMid = 0xcafe; 	/* Manufac ID */
        WInDev[i].caps.wPid = 0x0001; 	/* Product ID */
        WInDev[i].caps.vDriverVersion = 0x0001;

        snprintfW(WInDev[i].caps.szPname, sizeof(WInDev[i].caps.szPname)/sizeof(WCHAR), wszWaveInFormat, i);
        snprintf(WInDev[i].interface_name, sizeof(WInDev[i].interface_name), "winecoreaudio in: %d", i);

        if (inputSampleRate == 96000)
        {
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_96M08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_96S08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_96M16;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_96S16;
        }
        if (inputSampleRate == 48000)
        {
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_48M08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_48S08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_48M16;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_48S16;
        }
        if (inputSampleRate == 44100)
        {
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_4M08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_4S08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_4M16;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_4S16;
        }
        if (inputSampleRate == 22050)
        {
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_2M08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_2S08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_2M16;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_2S16;
        }
        if (inputSampleRate == 11025)
        {
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_1M08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_1S08;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_1M16;
            WInDev[i].caps.dwFormats |= WAVE_FORMAT_1S16;
        }

        WInDev[i].caps.wChannels = 2;
    }

    /* create mach messages handler */
    srandomdev();
    messageThreadPortName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
        CFSTR("WaveMessagePort.%d.%lu"), getpid(), (unsigned long)random());
    if (!messageThreadPortName)
    {
        ERR("Can't create message thread port name\n");
        return DRV_FAILURE;
    }

    port_ReceiveInMessageThread = CFMessagePortCreateLocal(kCFAllocatorDefault, messageThreadPortName,
                                        &wodMessageHandler, NULL, NULL);
    if (!port_ReceiveInMessageThread)
    {
        ERR("Can't create message thread local port\n");
        CFRelease(messageThreadPortName);
        return DRV_FAILURE;
    }

    Port_SendToMessageThread = CFMessagePortCreateRemote(kCFAllocatorDefault, messageThreadPortName);
    CFRelease(messageThreadPortName);
    if (!Port_SendToMessageThread)
    {
        ERR("Can't create port for sending to message thread\n");
        CFRelease(port_ReceiveInMessageThread);
        return DRV_FAILURE;
    }

    /* Cannot WAIT for any events because we are called from the loader (which has a lock on loading stuff) */
    /* We might want to wait for this thread to be created -- but we cannot -- not here at least */
    /* Instead track the thread so we can clean it up later */
    if ( hThread )
    {
        ERR("Message thread already started -- expect problems\n");
    }
    hThread = CreateThread(NULL, 0, messageThread, (LPVOID)port_ReceiveInMessageThread, 0, NULL);
    if ( !hThread )
    {
        ERR("Can't create message thread\n");
        CFRelease(port_ReceiveInMessageThread);
        CFRelease(Port_SendToMessageThread);
        Port_SendToMessageThread = NULL;
        return DRV_FAILURE;
    }

    /* The message thread is responsible for releasing port_ReceiveInMessageThread. */

    return DRV_SUCCESS;
}

void CoreAudio_WaveRelease(void)
{
    /* Stop CFRunLoop in messageThread */
    TRACE("()\n");

    if (!Port_SendToMessageThread)
        return;

    CFMessagePortSendRequest(Port_SendToMessageThread, kStopLoopMessage, NULL, 0.0, 0.0, NULL, NULL);
    CFRelease(Port_SendToMessageThread);
    Port_SendToMessageThread = NULL;

    /* Wait for the thread to finish and clean it up */
    /* This rids us of any quick start/shutdown driver crashes */
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    hThread = NULL;
}

/*======================================================================*
*                  Low level WAVE OUT implementation			*
*======================================================================*/

/**************************************************************************
* 			wodNotifyClient			[internal]
*/
static DWORD wodNotifyClient(WINE_WAVEOUT_INSTANCE* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE_(coreaudio)("wMsg = 0x%04x dwParm1 = %04x dwParam2 = %04x\n", wMsg, dwParam1, dwParam2);

    switch (wMsg) {
        case WOM_OPEN:
        case WOM_CLOSE:
        case WOM_DONE:
            if (wwo->wFlags != DCB_NULL &&
                !DriverCallback(wwo->waveDesc.dwCallback, wwo->wFlags,
                                (HDRVR)wwo->waveDesc.hWave, wMsg, wwo->waveDesc.dwInstance,
                                dwParam1, dwParam2))
            {
                ERR("can't notify client !\n");
                return MMSYSERR_ERROR;
            }
            break;
        default:
            ERR("Unknown callback message %u\n", wMsg);
            return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}


/**************************************************************************
* 			wodGetDevCaps               [internal]
*/
static DWORD wodGetDevCaps(WORD wDevID, LPWAVEOUTCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);
    
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        TRACE("MAX_WAVOUTDRV reached !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    TRACE("dwSupport=(0x%x), dwFormats=(0x%x)\n", WOutDev[wDevID].caps.dwSupport, WOutDev[wDevID].caps.dwFormats);
    memcpy(lpCaps, &WOutDev[wDevID].caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodOpen				[internal]
*/
static DWORD wodOpen(WORD wDevID, WINE_WAVEOUT_INSTANCE** pInstance, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    WINE_WAVEOUT_INSTANCE*      wwo;
    DWORD               ret;
    AudioStreamBasicDescription streamFormat;
    AudioUnit           audioUnit = NULL;
    BOOL                auInited  = FALSE;

    TRACE("(%u, %p, %p, %08x);\n", wDevID, pInstance, lpDesc, dwFlags);
    if (lpDesc == NULL)
    {
        WARN("Invalid Parameter !\n");
        return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
        TRACE("MAX_WAVOUTDRV reached !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    TRACE("Format: tag=%04X nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d !\n",
          lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
          lpDesc->lpFormat->nSamplesPerSec, lpDesc->lpFormat->wBitsPerSample);
    
    if (!supportedFormat(lpDesc->lpFormat))
    {
        WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d !\n",
             lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
             lpDesc->lpFormat->nSamplesPerSec, lpDesc->lpFormat->wBitsPerSample);
        return WAVERR_BADFORMAT;
    }
    
    if (dwFlags & WAVE_FORMAT_QUERY)
    {
        TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%d !\n",
              lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
              lpDesc->lpFormat->nSamplesPerSec);
        return MMSYSERR_NOERROR;
    }

    /* nBlockAlign and nAvgBytesPerSec are output variables for dsound */
    if (lpDesc->lpFormat->nBlockAlign != lpDesc->lpFormat->nChannels*lpDesc->lpFormat->wBitsPerSample/8) {
        lpDesc->lpFormat->nBlockAlign  = lpDesc->lpFormat->nChannels*lpDesc->lpFormat->wBitsPerSample/8;
        WARN("Fixing nBlockAlign\n");
    }
    if (lpDesc->lpFormat->nAvgBytesPerSec!= lpDesc->lpFormat->nSamplesPerSec*lpDesc->lpFormat->nBlockAlign) {
        lpDesc->lpFormat->nAvgBytesPerSec = lpDesc->lpFormat->nSamplesPerSec*lpDesc->lpFormat->nBlockAlign;
        WARN("Fixing nAvgBytesPerSec\n");
    }

    /* We proceed in three phases:
     * o Allocate the device instance, marking it as opening
     * o Create, configure, and start the Audio Unit.  To avoid deadlock,
     *   this has to be done without holding wwo->lock.
     * o If that was successful, finish setting up our instance and add it
     *   to the device's list.
     *   Otherwise, clean up and deallocate the instance.
     */
    wwo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wwo));
    if (!wwo)
        return MMSYSERR_NOMEM;

    wwo->woID = wDevID;
    wwo->state = WINE_WS_OPENING;

    if (!AudioUnit_CreateDefaultAudioUnit((void *) wwo, &audioUnit))
    {
        ERR("CoreAudio_CreateDefaultAudioUnit(0x%04x) failed\n", wDevID);
        ret = MMSYSERR_ERROR;
        goto error;
    }

    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kLinearPCMFormatFlagIsPacked;
    /* FIXME check for 32bits float -> kLinearPCMFormatFlagIsFloat */
    if (lpDesc->lpFormat->wBitsPerSample != 8)
        streamFormat.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
# ifdef WORDS_BIGENDIAN
    streamFormat.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian; /* FIXME Wave format is little endian */
# endif

    streamFormat.mSampleRate = lpDesc->lpFormat->nSamplesPerSec;
    streamFormat.mChannelsPerFrame = lpDesc->lpFormat->nChannels;	
    streamFormat.mFramesPerPacket = 1;	
    streamFormat.mBitsPerChannel = lpDesc->lpFormat->wBitsPerSample;
    streamFormat.mBytesPerFrame = streamFormat.mBitsPerChannel * streamFormat.mChannelsPerFrame / 8;	
    streamFormat.mBytesPerPacket = streamFormat.mBytesPerFrame * streamFormat.mFramesPerPacket;		

    ret = AudioUnit_InitializeWithStreamDescription(audioUnit, &streamFormat);
    if (!ret) 
    {
        ret = WAVERR_BADFORMAT; /* FIXME return an error based on the OSStatus */
        goto error;
    }
    auInited = TRUE;

    AudioUnit_SetVolume(audioUnit, LOWORD(WOutDev[wDevID].device_volume) / 65535.0f,
                        HIWORD(WOutDev[wDevID].device_volume) / 65535.0f);

    /* Our render callback CoreAudio_woAudioUnitIOProc may be called before
     * AudioOutputUnitStart returns.  Core Audio will grab its own internal
     * lock before calling it and the callback grabs wwo->lock.  This would
     * deadlock if we were holding wwo->lock.
     * Also, the callback has to safely do nothing in that case, because
     * wwo hasn't been completely filled out, yet. This is assured by state
     * being WINE_WS_OPENING. */
    ret = AudioOutputUnitStart(audioUnit);
    if (ret)
    {
        ERR("AudioOutputUnitStart failed: %08x\n", ret);
        ret = MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
        goto error;
    }


    OSSpinLockLock(&wwo->lock);
    assert(wwo->state == WINE_WS_OPENING);

    wwo->audioUnit = audioUnit;
    wwo->streamDescription = streamFormat;

    wwo->state = WINE_WS_PLAYING;

    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    wwo->waveDesc = *lpDesc;
    copyFormat(lpDesc->lpFormat, &wwo->format);

    WOutDev[wDevID].trace_on = TRACE_ON(wave);
    WOutDev[wDevID].warn_on  = WARN_ON(wave);
    WOutDev[wDevID].err_on   = ERR_ON(wave);

    OSSpinLockUnlock(&wwo->lock);

    OSSpinLockLock(&WOutDev[wDevID].lock);
    list_add_head(&WOutDev[wDevID].instances, &wwo->entry);
    OSSpinLockUnlock(&WOutDev[wDevID].lock);

    *pInstance = wwo;
    TRACE("opened instance %p\n", wwo);

    ret = wodNotifyClient(wwo, WOM_OPEN, 0L, 0L);
    
    return ret;

error:
    if (audioUnit)
    {
        if (auInited)
            AudioUnitUninitialize(audioUnit);
        AudioUnit_CloseAudioUnit(audioUnit);
    }

    OSSpinLockLock(&wwo->lock);
    assert(wwo->state == WINE_WS_OPENING);
    /* OSSpinLockUnlock(&wwo->lock); *//* No need, about to free */
    HeapFree(GetProcessHeap(), 0, wwo);

    return ret;
}

/**************************************************************************
* 				wodClose			[internal]
*/
static DWORD wodClose(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo)
{
    DWORD		ret = MMSYSERR_NOERROR;
    
    TRACE("(%u, %p);\n", wDevID, wwo);
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    OSSpinLockLock(&wwo->lock);
    if (wwo->lpQueuePtr)
    {
        OSSpinLockUnlock(&wwo->lock);
        WARN("buffers still playing !\n");
        ret = WAVERR_STILLPLAYING;
    } else
    {
        OSStatus err;
        AudioUnit audioUnit = wwo->audioUnit;

        /* sanity check: this should not happen since the device must have been reset before */
        if (wwo->lpQueuePtr || wwo->lpPlayPtr) ERR("out of sync\n");
        
        wwo->state = WINE_WS_CLOSING; /* mark the device as closing */
        wwo->audioUnit = NULL;
        
        OSSpinLockUnlock(&wwo->lock);

        err = AudioUnitUninitialize(audioUnit);
        if (err) {
            ERR("AudioUnitUninitialize return %s\n", wine_dbgstr_fourcc(err));
            return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
        }
        
        if ( !AudioUnit_CloseAudioUnit(audioUnit) )
        {
            ERR("Can't close AudioUnit\n");
            return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
        }  
        
        OSSpinLockLock(&WOutDev[wDevID].lock);
        list_remove(&wwo->entry);
        OSSpinLockUnlock(&WOutDev[wDevID].lock);

        ret = wodNotifyClient(wwo, WOM_CLOSE, 0L, 0L);

        HeapFree(GetProcessHeap(), 0, wwo);
    }
    
    return ret;
}

/**************************************************************************
* 				wodPrepare			[internal]
*/
static DWORD wodPrepare(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %p, %08x);\n", wDevID, wwo, lpWaveHdr, dwSize);
    
    if (wDevID >= MAX_WAVEOUTDRV) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    
    lpWaveHdr->dwFlags |= WHDR_PREPARED;
    lpWaveHdr->dwFlags &= ~WHDR_DONE;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodUnprepare			[internal]
*/
static DWORD wodUnprepare(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %p, %08x);\n", wDevID, wwo, lpWaveHdr, dwSize);
    
    if (wDevID >= MAX_WAVEOUTDRV) {
	WARN("bad device ID !\n");
	return MMSYSERR_BADDEVICEID;
    }
    
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
	return WAVERR_STILLPLAYING;
    
    lpWaveHdr->dwFlags &= ~WHDR_PREPARED;
    lpWaveHdr->dwFlags |= WHDR_DONE;
   
    return MMSYSERR_NOERROR;
}


/**************************************************************************
* 				wodHelper_CheckForLoopBegin	        [internal]
*
* Check if the new waveheader is the beginning of a loop, and set up
* state if so.
* This is called with the WAVEOUT_INSTANCE lock held.
* Call from AudioUnit IO thread can't use Wine debug channels.
*/
static void wodHelper_CheckForLoopBegin(WINE_WAVEOUT_INSTANCE* wwo)
{
    LPWAVEHDR lpWaveHdr = wwo->lpPlayPtr;

    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)
    {
        if (wwo->lpLoopPtr)
        {
            if (WOutDev[wwo->woID].warn_on)
                fprintf(stderr, "warn:winecoreaudio:wodHelper_CheckForLoopBegin Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
        }
        else
        {
            if (WOutDev[wwo->woID].trace_on)
                fprintf(stderr, "trace:winecoreaudio:wodHelper_CheckForLoopBegin Starting loop (%dx) with %p\n", lpWaveHdr->dwLoops, lpWaveHdr);

            wwo->lpLoopPtr = lpWaveHdr;
            /* Windows does not touch WAVEHDR.dwLoops,
                * so we need to make an internal copy */
            wwo->dwLoops = lpWaveHdr->dwLoops;
        }
    }
}


/**************************************************************************
* 				wodHelper_PlayPtrNext	        [internal]
*
* Advance the play pointer to the next waveheader, looping if required.
* This is called with the WAVEOUT_INSTANCE lock held.
* Call from AudioUnit IO thread can't use Wine debug channels.
*/
static void wodHelper_PlayPtrNext(WINE_WAVEOUT_INSTANCE* wwo)
{
    BOOL didLoopBack = FALSE;

    wwo->dwPartialOffset = 0;
    if ((wwo->lpPlayPtr->dwFlags & WHDR_ENDLOOP) && wwo->lpLoopPtr)
    {
        /* We're at the end of a loop, loop if required */
        if (wwo->dwLoops > 1)
        {
            wwo->dwLoops--;
            wwo->lpPlayPtr = wwo->lpLoopPtr;
            didLoopBack = TRUE;
        }
        else
        {
            wwo->lpLoopPtr = NULL;
        }
    }
    if (!didLoopBack)
    {
        /* We didn't loop back.  Advance to the next wave header */
        wwo->lpPlayPtr = wwo->lpPlayPtr->lpNext;

        if (wwo->lpPlayPtr)
            wodHelper_CheckForLoopBegin(wwo);
    }
}

/* Send the "done" notification for each WAVEHDR in a list.  The list must be
 * free-standing.  It should not be part of a device instance's queue.
 * This function must be called with the WAVEOUT_INSTANCE lock *not* held.
 * Furthermore, it does not lock it, itself.  That's because the callback to the
 * application may prompt the application to operate on the device, and we don't
 * want to deadlock.
 */
static void wodHelper_NotifyDoneForList(WINE_WAVEOUT_INSTANCE* wwo, LPWAVEHDR lpWaveHdr)
{
    while (lpWaveHdr)
    {
        LPWAVEHDR lpNext = lpWaveHdr->lpNext;

        lpWaveHdr->lpNext = NULL;
        lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
        lpWaveHdr->dwFlags |= WHDR_DONE;
        wodNotifyClient(wwo, WOM_DONE, (DWORD)lpWaveHdr, 0);

        lpWaveHdr = lpNext;
    }
}

/* if force is TRUE then notify the client that all the headers were completed
 */
static void wodHelper_NotifyCompletions(WINE_WAVEOUT_INSTANCE* wwo, BOOL force)
{
    LPWAVEHDR lpFirstDoneWaveHdr = NULL;

    OSSpinLockLock(&wwo->lock);

    /* First, excise all of the done headers from the queue into
     * a free-standing list. */
    if (force)
    {
        lpFirstDoneWaveHdr = wwo->lpQueuePtr;
        wwo->lpQueuePtr = NULL;
    }
    else
    {
        LPWAVEHDR lpWaveHdr;
        LPWAVEHDR lpLastDoneWaveHdr = NULL;

        /* Start from lpQueuePtr and keep notifying until:
            * - we hit an unwritten wavehdr
            * - we hit the beginning of a running loop
            * - we hit a wavehdr which hasn't finished playing
            */
        for (
            lpWaveHdr = wwo->lpQueuePtr;
            lpWaveHdr &&
                lpWaveHdr != wwo->lpPlayPtr &&
                lpWaveHdr != wwo->lpLoopPtr;
            lpWaveHdr = lpWaveHdr->lpNext
            )
        {
            if (!lpFirstDoneWaveHdr)
                lpFirstDoneWaveHdr = lpWaveHdr;
            lpLastDoneWaveHdr = lpWaveHdr;
        }

        if (lpLastDoneWaveHdr)
        {
            wwo->lpQueuePtr = lpLastDoneWaveHdr->lpNext;
            lpLastDoneWaveHdr->lpNext = NULL;
        }
    }
    
    OSSpinLockUnlock(&wwo->lock);

    /* Now, send the "done" notification for each header in our list. */
    wodHelper_NotifyDoneForList(wwo, lpFirstDoneWaveHdr);
}


/**************************************************************************
* 				wodWrite			[internal]
* 
*/
static DWORD wodWrite(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    LPWAVEHDR*wh;
    
    TRACE("(%u, %p, %p, %lu, %08X);\n", wDevID, wwo, lpWaveHdr, (unsigned long)lpWaveHdr->dwBufferLength, dwSize);
    
    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad dev ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    if (lpWaveHdr->lpData == NULL || !(lpWaveHdr->dwFlags & WHDR_PREPARED))
    {
        TRACE("unprepared\n");
        return WAVERR_UNPREPARED;
    }
    
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE) 
    {
        TRACE("still playing\n");
        return WAVERR_STILLPLAYING;
    }
    
    lpWaveHdr->dwFlags &= ~WHDR_DONE;
    lpWaveHdr->dwFlags |= WHDR_INQUEUE;
    lpWaveHdr->lpNext = 0;

    OSSpinLockLock(&wwo->lock);
    /* insert buffer at the end of queue */
    for (wh = &(wwo->lpQueuePtr); *wh; wh = &((*wh)->lpNext))
        /* Do nothing */;
    *wh = lpWaveHdr;
    
    if (!wwo->lpPlayPtr)
    {
        wwo->lpPlayPtr = lpWaveHdr;

        wodHelper_CheckForLoopBegin(wwo);

        wwo->dwPartialOffset = 0;
    }
    OSSpinLockUnlock(&wwo->lock);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 			wodPause				[internal]
*/
static DWORD wodPause(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo)
{
    OSStatus status;

    TRACE("(%u, %p);!\n", wDevID, wwo);
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    /* The order of the following operations is important since we can't hold
     * the mutex while we make an Audio Unit call.  Stop the Audio Unit before
     * setting the PAUSED state.  In wodRestart, the order is reversed.  This
     * guarantees that we can't get into a situation where the state is
     * PLAYING but the Audio Unit isn't running.  Although we can be in PAUSED
     * state with the Audio Unit still running, that's harmless because the
     * render callback will just produce silence.
     */
    status = AudioOutputUnitStop(wwo->audioUnit);
    if (status)
        WARN("AudioOutputUnitStop return %s\n", wine_dbgstr_fourcc(status));

    OSSpinLockLock(&wwo->lock);
    if (wwo->state == WINE_WS_PLAYING)
        wwo->state = WINE_WS_PAUSED;
    OSSpinLockUnlock(&wwo->lock);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 			wodRestart				[internal]
*/
static DWORD wodRestart(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo)
{
    OSStatus status;

    TRACE("(%u, %p);\n", wDevID, wwo);
    
    if (wDevID >= MAX_WAVEOUTDRV )
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    /* The order of the following operations is important since we can't hold
     * the mutex while we make an Audio Unit call.  Set the PLAYING
     * state before starting the Audio Unit.  In wodPause, the order is
     * reversed.  This guarantees that we can't get into a situation where
     * the state is PLAYING but the Audio Unit isn't running.
     * Although we can be in PAUSED state with the Audio Unit still running,
     * that's harmless because the render callback will just produce silence.
     */
    OSSpinLockLock(&wwo->lock);
    if (wwo->state == WINE_WS_PAUSED)
        wwo->state = WINE_WS_PLAYING;
    OSSpinLockUnlock(&wwo->lock);

    status = AudioOutputUnitStart(wwo->audioUnit);
    if (status) {
        ERR("AudioOutputUnitStart return %s\n", wine_dbgstr_fourcc(status));
        return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 			wodReset				[internal]
*/
static DWORD wodReset(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo)
{
    OSStatus status;
    LPWAVEHDR lpSavedQueuePtr;

    TRACE("(%u, %p);\n", wDevID, wwo);

    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    OSSpinLockLock(&wwo->lock);

    if (wwo->state == WINE_WS_CLOSING || wwo->state == WINE_WS_OPENING)
    {
        OSSpinLockUnlock(&wwo->lock);
        WARN("resetting a closed device\n");
        return MMSYSERR_INVALHANDLE;
    }

    lpSavedQueuePtr = wwo->lpQueuePtr;
    wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
    wwo->state = WINE_WS_PLAYING;
    wwo->dwPlayedTotal = 0;

    wwo->dwPartialOffset = 0;        /* Clear partial wavehdr */

    OSSpinLockUnlock(&wwo->lock);

    status = AudioOutputUnitStart(wwo->audioUnit);

    if (status) {
        ERR( "AudioOutputUnitStart return %s\n", wine_dbgstr_fourcc(status));
        return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
    }

    /* Now, send the "done" notification for each header in our list. */
    /* Do this last so the reset operation is effectively complete before the
     * app does whatever it's going to do in response to these notifications. */
    wodHelper_NotifyDoneForList(wwo, lpSavedQueuePtr);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
*           wodBreakLoop                [internal]
*/
static DWORD wodBreakLoop(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo)
{
    TRACE("(%u, %p);\n", wDevID, wwo);

    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    OSSpinLockLock(&wwo->lock);

    if (wwo->lpLoopPtr != NULL)
    {
        /* ensure exit at end of current loop */
        wwo->dwLoops = 1;
    }

    OSSpinLockUnlock(&wwo->lock);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodGetPosition			[internal]
*/
static DWORD wodGetPosition(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo, LPMMTIME lpTime, DWORD uSize)
{
    DWORD		val;

    TRACE("(%u, %p, %p, %u);\n", wDevID, wwo, lpTime, uSize);
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    /* if null pointer to time structure return error */
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
    
    OSSpinLockLock(&wwo->lock);
    val = wwo->dwPlayedTotal;
    OSSpinLockUnlock(&wwo->lock);
    
    return bytes_to_mmtime(lpTime, val, &wwo->format);
}

/**************************************************************************
* 				wodGetVolume			[internal]
*/
static DWORD wodGetVolume(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo, LPDWORD lpdwVol)
{
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }    
    
    TRACE("(%u, %p, %p);\n", wDevID, wwo, lpdwVol);

    if (wwo)
    {
        float left;
        float right;

        AudioUnit_GetVolume(wwo->audioUnit, &left, &right);
        *lpdwVol = (WORD)(left * 0xFFFFl) + ((WORD)(right * 0xFFFFl) << 16);
    }
    else
        *lpdwVol = WOutDev[wDevID].device_volume;

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodSetVolume			[internal]
*/
static DWORD wodSetVolume(WORD wDevID, WINE_WAVEOUT_INSTANCE* wwo, DWORD dwParam)
{
    float left;
    float right;
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    left  = LOWORD(dwParam) / 65535.0f;
    right = HIWORD(dwParam) / 65535.0f;
    
    TRACE("(%u, %p, %08x);\n", wDevID, wwo, dwParam);

    if (wwo)
        AudioUnit_SetVolume(wwo->audioUnit, left, right);
    else
    {
        OSSpinLockLock(&WOutDev[wDevID].lock);
        LIST_FOR_EACH_ENTRY(wwo, &WOutDev[wDevID].instances, WINE_WAVEOUT_INSTANCE, entry)
            AudioUnit_SetVolume(wwo->audioUnit, left, right);
        OSSpinLockUnlock(&WOutDev[wDevID].lock);

        WOutDev[wDevID].device_volume = dwParam;
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodGetNumDevs			[internal]
*/
static DWORD wodGetNumDevs(void)
{
    TRACE("\n");
    return MAX_WAVEOUTDRV;
}

/**************************************************************************
*                              wodDevInterfaceSize             [internal]
*/
static DWORD wodDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);
    
    *dwParam1 = MultiByteToWideChar(CP_UNIXCP, 0, WOutDev[wDevID].cadev->interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
*                              wodDevInterface                 [internal]
*/
static DWORD wodDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    TRACE("\n");
    if (dwParam2 >= MultiByteToWideChar(CP_UNIXCP, 0, WOutDev[wDevID].cadev->interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_UNIXCP, 0, WOutDev[wDevID].cadev->interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
        return MMSYSERR_NOERROR;
    }
    return MMSYSERR_INVALPARAM;
}

/**************************************************************************
 *                              widDsCreate                     [internal]
 */
static DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv)
{
    TRACE("(%d,%p)\n",wDevID,drv);

    FIXME("DirectSound not implemented\n");
    FIXME("The (slower) DirectSound HEL mode will be used instead.\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
*                              wodDsDesc                 [internal]
*/
static DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    /* The DirectSound HEL will automatically wrap a non-DirectSound-capable
     * driver in a DirectSound adaptor, thus allowing the driver to be used by
     * DirectSound clients.  However, it only does this if we respond
     * successfully to the DRV_QUERYDSOUNDDESC message.  It's enough to fill in
     * the driver and device names of the description output parameter. */
    *desc = WOutDev[wDevID].cadev->ds_desc;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodMessage (WINECOREAUDIO.7)
*/
DWORD WINAPI CoreAudio_wodMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
                                  DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    WINE_WAVEOUT_INSTANCE* wwo = (WINE_WAVEOUT_INSTANCE*)dwUser;

    TRACE("(%u, %s, %p, %p, %p);\n",
          wDevID, getMessage(wMsg), (void*)dwUser, (void*)dwParam1, (void*)dwParam2);
    
    switch (wMsg) {
        case DRVM_INIT:
        case DRVM_EXIT:
        case DRVM_ENABLE:
        case DRVM_DISABLE:
            
            /* FIXME: Pretend this is supported */
            return 0;
        case WODM_OPEN:         return wodOpen(wDevID, (WINE_WAVEOUT_INSTANCE**)dwUser, (LPWAVEOPENDESC) dwParam1, dwParam2);
        case WODM_CLOSE:        return wodClose(wDevID, wwo);
        case WODM_WRITE:        return wodWrite(wDevID, wwo, (LPWAVEHDR) dwParam1, dwParam2);
        case WODM_PAUSE:        return wodPause(wDevID, wwo);
        case WODM_GETPOS:       return wodGetPosition(wDevID, wwo, (LPMMTIME) dwParam1, dwParam2);
        case WODM_BREAKLOOP:    return wodBreakLoop(wDevID, wwo);
        case WODM_PREPARE:      return wodPrepare(wDevID, wwo, (LPWAVEHDR)dwParam1, dwParam2);
        case WODM_UNPREPARE:    return wodUnprepare(wDevID, wwo, (LPWAVEHDR)dwParam1, dwParam2);
            
        case WODM_GETDEVCAPS:   return wodGetDevCaps(wDevID, (LPWAVEOUTCAPSW) dwParam1, dwParam2);
        case WODM_GETNUMDEVS:   return wodGetNumDevs();  
            
        case WODM_GETPITCH:         
        case WODM_SETPITCH:        
        case WODM_GETPLAYBACKRATE:	
        case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
        case WODM_GETVOLUME:    return wodGetVolume(wDevID, wwo, (LPDWORD)dwParam1);
        case WODM_SETVOLUME:    return wodSetVolume(wDevID, wwo, dwParam1);
        case WODM_RESTART:      return wodRestart(wDevID, wwo);
        case WODM_RESET:    	return wodReset(wDevID, wwo);
            
        case DRV_QUERYDEVICEINTERFACESIZE:  return wodDevInterfaceSize (wDevID, (LPDWORD)dwParam1);
        case DRV_QUERYDEVICEINTERFACE:      return wodDevInterface (wDevID, (PWCHAR)dwParam1, dwParam2);
        case DRV_QUERYDSOUNDIFACE:  return wodDsCreate  (wDevID, (PIDSDRIVER*)dwParam1);
        case DRV_QUERYDSOUNDDESC:   return wodDsDesc    (wDevID, (PDSDRIVERDESC)dwParam1);
            
        default:
            FIXME("unknown message %d!\n", wMsg);
    }
    
    return MMSYSERR_NOTSUPPORTED;
}

/*======================================================================*
*                  Low level DSOUND implementation			*
*======================================================================*/

typedef struct IDsDriverImpl IDsDriverImpl;
typedef struct IDsDriverBufferImpl IDsDriverBufferImpl;

struct IDsDriverImpl
{
    /* IUnknown fields */
    const IDsDriverVtbl *lpVtbl;
    DWORD		ref;
    /* IDsDriverImpl fields */
    UINT		wDevID;
    IDsDriverBufferImpl*primary;
};

struct IDsDriverBufferImpl
{
    /* IUnknown fields */
    const IDsDriverBufferVtbl *lpVtbl;
    DWORD ref;
    /* IDsDriverBufferImpl fields */
    IDsDriverImpl* drv;
    DWORD buflen;
};


/*
    CoreAudio IO threaded callback,
    we can't call Wine debug channels, critical section or anything using NtCurrentTeb here.
*/
OSStatus CoreAudio_woAudioUnitIOProc(void *inRefCon, 
                                     AudioUnitRenderActionFlags *ioActionFlags, 
                                     const AudioTimeStamp *inTimeStamp, 
                                     UInt32 inBusNumber, 
                                     UInt32 inNumberFrames, 
                                     AudioBufferList *ioData)
{
    UInt32 buffer;
    WINE_WAVEOUT_INSTANCE* wwo = (WINE_WAVEOUT_INSTANCE*)inRefCon;
    int needNotify = 0;

    unsigned int dataNeeded = ioData->mBuffers[0].mDataByteSize;
    unsigned int dataProvided = 0;

    OSSpinLockLock(&wwo->lock);

    /* We might have been called before wwo has been completely filled out by
     * wodOpen, or while it's being closed in wodClose.  We have to do nothing
     * in that case.  The check of wwo->state below ensures that. */
    while (dataNeeded > 0 && wwo->state == WINE_WS_PLAYING && wwo->lpPlayPtr)
    {
        unsigned int available = wwo->lpPlayPtr->dwBufferLength - wwo->dwPartialOffset;
        unsigned int toCopy;

        if (available >= dataNeeded)
            toCopy = dataNeeded;
        else
            toCopy = available;

        if (toCopy > 0)
        {
            memcpy((char*)ioData->mBuffers[0].mData + dataProvided,
                wwo->lpPlayPtr->lpData + wwo->dwPartialOffset, toCopy);
            wwo->dwPartialOffset += toCopy;
            wwo->dwPlayedTotal += toCopy;
            dataProvided += toCopy;
            dataNeeded -= toCopy;
            available -= toCopy;
        }

        if (available == 0)
        {
            wodHelper_PlayPtrNext(wwo);
            needNotify = 1;
        }
    }
    ioData->mBuffers[0].mDataByteSize = dataProvided;

    OSSpinLockUnlock(&wwo->lock);

    /* We can't provide any more wave data.  Fill the rest with silence. */
    if (dataNeeded > 0)
    {
        if (!dataProvided)
            *ioActionFlags |= kAudioUnitRenderAction_OutputIsSilence;
        memset((char*)ioData->mBuffers[0].mData + dataProvided, 0, dataNeeded);
        dataProvided += dataNeeded;
        dataNeeded = 0;
    }

    /* We only fill buffer 0.  Set any others that might be requested to 0. */
    for (buffer = 1; buffer < ioData->mNumberBuffers; buffer++)
    {
        memset(ioData->mBuffers[buffer].mData, 0, ioData->mBuffers[buffer].mDataByteSize);
    }

    if (needNotify) wodSendNotifyCompletionsMessage(wwo);
    return noErr;
}


/*======================================================================*
 *                  Low level WAVE IN implementation                    *
 *======================================================================*/

/**************************************************************************
 *                      widNotifyClient                 [internal]
 */
static DWORD widNotifyClient(WINE_WAVEIN* wwi, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    TRACE("wMsg = 0x%04x dwParm1 = %04X dwParam2 = %04X\n", wMsg, dwParam1, dwParam2);

    switch (wMsg)
    {
        case WIM_OPEN:
        case WIM_CLOSE:
        case WIM_DATA:
            if (wwi->wFlags != DCB_NULL &&
                !DriverCallback(wwi->waveDesc.dwCallback, wwi->wFlags,
                                (HDRVR)wwi->waveDesc.hWave, wMsg, wwi->waveDesc.dwInstance,
                                dwParam1, dwParam2))
            {
                WARN("can't notify client !\n");
                return MMSYSERR_ERROR;
            }
            break;
        default:
            FIXME("Unknown callback message %u\n", wMsg);
            return MMSYSERR_INVALPARAM;
    }
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 *                      widHelper_NotifyCompletions              [internal]
 */
static void widHelper_NotifyCompletions(WINE_WAVEIN* wwi)
{
    LPWAVEHDR       lpWaveHdr;
    LPWAVEHDR       lpFirstDoneWaveHdr = NULL;
    LPWAVEHDR       lpLastDoneWaveHdr = NULL;

    OSSpinLockLock(&wwi->lock);

    /* First, excise all of the done headers from the queue into
     * a free-standing list. */

    /* Start from lpQueuePtr and keep notifying until:
        * - we hit an unfilled wavehdr
        * - we hit the end of the list
        */
    for (
        lpWaveHdr = wwi->lpQueuePtr;
        lpWaveHdr &&
            lpWaveHdr->dwBytesRecorded >= lpWaveHdr->dwBufferLength;
        lpWaveHdr = lpWaveHdr->lpNext
        )
    {
        if (!lpFirstDoneWaveHdr)
            lpFirstDoneWaveHdr = lpWaveHdr;
        lpLastDoneWaveHdr = lpWaveHdr;
    }

    if (lpLastDoneWaveHdr)
    {
        wwi->lpQueuePtr = lpLastDoneWaveHdr->lpNext;
        lpLastDoneWaveHdr->lpNext = NULL;
    }

    OSSpinLockUnlock(&wwi->lock);

    /* Now, send the "done" notification for each header in our list. */
    lpWaveHdr = lpFirstDoneWaveHdr;
    while (lpWaveHdr)
    {
        LPWAVEHDR lpNext = lpWaveHdr->lpNext;

        lpWaveHdr->lpNext = NULL;
        lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
        lpWaveHdr->dwFlags |= WHDR_DONE;
        widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);

        lpWaveHdr = lpNext;
    }
}


/**************************************************************************
 *                      widGetDevCaps                           [internal]
 */
static DWORD widGetDevCaps(WORD wDevID, LPWAVEINCAPSW lpCaps, DWORD dwSize)
{
    TRACE("(%u, %p, %u);\n", wDevID, lpCaps, dwSize);

    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;

    if (wDevID >= MAX_WAVEINDRV)
    {
        TRACE("MAX_WAVEINDRV reached !\n");
        return MMSYSERR_BADDEVICEID;
    }

    memcpy(lpCaps, &WInDev[wDevID].caps, min(dwSize, sizeof(*lpCaps)));
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 *                    widHelper_DestroyAudioBufferList           [internal]
 * Convenience function to dispose of our audio buffers
 */
static void widHelper_DestroyAudioBufferList(AudioBufferList* list)
{
    if (list)
    {
        UInt32 i;
        for (i = 0; i < list->mNumberBuffers; i++)
        {
            if (list->mBuffers[i].mData)
                HeapFree(GetProcessHeap(), 0, list->mBuffers[i].mData);
        }
        HeapFree(GetProcessHeap(), 0, list);
    }
}


#define AUDIOBUFFERLISTSIZE(numBuffers) (offsetof(AudioBufferList, mBuffers) + (numBuffers) * sizeof(AudioBuffer))

/**************************************************************************
 *                    widHelper_AllocateAudioBufferList          [internal]
 * Convenience function to allocate our audio buffers
 */
static AudioBufferList* widHelper_AllocateAudioBufferList(UInt32 numChannels, UInt32 bitsPerChannel, UInt32 bufferFrames, BOOL interleaved)
{
    UInt32                      numBuffers;
    UInt32                      channelsPerFrame;
    UInt32                      bytesPerFrame;
    UInt32                      bytesPerBuffer;
    AudioBufferList*            list;
    UInt32                      i;

    if (interleaved)
    {
        /* For interleaved audio, we allocate one buffer for all channels. */
        numBuffers = 1;
        channelsPerFrame = numChannels;
    }
    else
    {
        numBuffers = numChannels;
        channelsPerFrame = 1;
    }

    bytesPerFrame = bitsPerChannel * channelsPerFrame / 8;
    bytesPerBuffer = bytesPerFrame * bufferFrames;

    list = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, AUDIOBUFFERLISTSIZE(numBuffers));
    if (list == NULL)
        return NULL;

    list->mNumberBuffers = numBuffers;
    for (i = 0; i < numBuffers; ++i)
    {
        list->mBuffers[i].mNumberChannels = channelsPerFrame;
        list->mBuffers[i].mDataByteSize = bytesPerBuffer;
        list->mBuffers[i].mData = HeapAlloc(GetProcessHeap(), 0, bytesPerBuffer);
        if (list->mBuffers[i].mData == NULL)
        {
            widHelper_DestroyAudioBufferList(list);
            return NULL;
        }
    }
    return list;
}


/**************************************************************************
 *                              widOpen                         [internal]
 */
static DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    WINE_WAVEIN*    wwi;
    UInt32          frameCount;

    TRACE("(%u, %p, %08X);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL)
    {
        WARN("Invalid Parameter !\n");
        return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEINDRV)
    {
        TRACE ("MAX_WAVEINDRV reached !\n");
        return MMSYSERR_BADDEVICEID;
    }

    TRACE("Format: tag=%04X nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d !\n",
          lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
          lpDesc->lpFormat->nSamplesPerSec, lpDesc->lpFormat->wBitsPerSample);

    if (!supportedFormat(lpDesc->lpFormat) ||
        lpDesc->lpFormat->nSamplesPerSec != AudioUnit_GetInputDeviceSampleRate()
        )
    {
        WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%d wBitsPerSample=%d !\n",
             lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
             lpDesc->lpFormat->nSamplesPerSec, lpDesc->lpFormat->wBitsPerSample);
        return WAVERR_BADFORMAT;
    }

    if (dwFlags & WAVE_FORMAT_QUERY)
    {
        TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%d !\n",
              lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
              lpDesc->lpFormat->nSamplesPerSec);
        return MMSYSERR_NOERROR;
    }

    /* nBlockAlign and nAvgBytesPerSec are output variables for dsound */
    if (lpDesc->lpFormat->nBlockAlign != lpDesc->lpFormat->nChannels*lpDesc->lpFormat->wBitsPerSample/8) {
        lpDesc->lpFormat->nBlockAlign  = lpDesc->lpFormat->nChannels*lpDesc->lpFormat->wBitsPerSample/8;
        WARN("Fixing nBlockAlign\n");
    }
    if (lpDesc->lpFormat->nAvgBytesPerSec!= lpDesc->lpFormat->nSamplesPerSec*lpDesc->lpFormat->nBlockAlign) {
        lpDesc->lpFormat->nAvgBytesPerSec = lpDesc->lpFormat->nSamplesPerSec*lpDesc->lpFormat->nBlockAlign;
        WARN("Fixing nAvgBytesPerSec\n");
    }

    wwi = &WInDev[wDevID];
    if (!OSSpinLockTry(&wwi->lock))
        return MMSYSERR_ALLOCATED;

    if (wwi->state != WINE_WS_CLOSED)
    {
        OSSpinLockUnlock(&wwi->lock);
        return MMSYSERR_ALLOCATED;
    }

    wwi->state = WINE_WS_STOPPED;
    wwi->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);

    wwi->waveDesc = *lpDesc;
    copyFormat(lpDesc->lpFormat, &wwi->format);

    wwi->dwTotalRecorded = 0;

    wwi->trace_on = TRACE_ON(wave);
    wwi->warn_on  = WARN_ON(wave);
    wwi->err_on   = ERR_ON(wave);

    if (!AudioUnit_CreateInputUnit(wwi, &wwi->audioUnit,
        wwi->format.wf.nChannels, wwi->format.wf.nSamplesPerSec,
        wwi->format.wBitsPerSample, &frameCount))
    {
        OSSpinLockUnlock(&wwi->lock);
        ERR("AudioUnit_CreateInputUnit failed\n");
        return MMSYSERR_ERROR;
    }

    /* Allocate our audio buffers */
    wwi->bufferList = widHelper_AllocateAudioBufferList(wwi->format.wf.nChannels,
        wwi->format.wBitsPerSample, frameCount, TRUE);
    if (wwi->bufferList == NULL)
    {
        AudioUnitUninitialize(wwi->audioUnit);
        AudioUnit_CloseAudioUnit(wwi->audioUnit);
        OSSpinLockUnlock(&wwi->lock);
        ERR("Failed to allocate buffer list\n");
        return MMSYSERR_NOMEM;
    }

    /* Keep a copy of the buffer list structure (but not the buffers themselves)
     * in case AudioUnitRender clobbers the original, as it tends to do. */
    wwi->bufferListCopy = HeapAlloc(GetProcessHeap(), 0, AUDIOBUFFERLISTSIZE(wwi->bufferList->mNumberBuffers));
    if (wwi->bufferListCopy == NULL)
    {
        widHelper_DestroyAudioBufferList(wwi->bufferList);
        AudioUnitUninitialize(wwi->audioUnit);
        AudioUnit_CloseAudioUnit(wwi->audioUnit);
        OSSpinLockUnlock(&wwi->lock);
        ERR("Failed to allocate buffer list copy\n");
        return MMSYSERR_NOMEM;
    }
    memcpy(wwi->bufferListCopy, wwi->bufferList, AUDIOBUFFERLISTSIZE(wwi->bufferList->mNumberBuffers));

    OSSpinLockUnlock(&wwi->lock);

    return widNotifyClient(wwi, WIM_OPEN, 0L, 0L);
}


/**************************************************************************
 *                              widClose                        [internal]
 */
static DWORD widClose(WORD wDevID)
{
    DWORD           ret = MMSYSERR_NOERROR;
    WINE_WAVEIN*    wwi;
    OSStatus        err;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEINDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    wwi = &WInDev[wDevID];
    OSSpinLockLock(&wwi->lock);
    if (wwi->state == WINE_WS_CLOSED || wwi->state == WINE_WS_CLOSING)
    {
        WARN("Device already closed.\n");
        ret = MMSYSERR_INVALHANDLE;
    }
    else if (wwi->lpQueuePtr)
    {
        WARN("Buffers in queue.\n");
        ret = WAVERR_STILLPLAYING;
    }
    else
    {
        wwi->state = WINE_WS_CLOSING;
    }

    OSSpinLockUnlock(&wwi->lock);

    if (ret != MMSYSERR_NOERROR)
        return ret;


    /* Clean up and close the audio unit.  This has to be done without
     * wwi->lock being held to avoid deadlock.  AudioUnitUninitialize will
     * grab an internal Core Audio lock while waiting for the device work
     * thread to exit.  Meanwhile the device work thread may be holding
     * that lock and trying to grab the wwi->lock in the callback. */
    err = AudioUnitUninitialize(wwi->audioUnit);
    if (err)
        ERR("AudioUnitUninitialize return %s\n", wine_dbgstr_fourcc(err));

    if (!AudioUnit_CloseAudioUnit(wwi->audioUnit))
        ERR("Can't close AudioUnit\n");


    OSSpinLockLock(&wwi->lock);
    assert(wwi->state == WINE_WS_CLOSING);

    /* Dellocate our audio buffers */
    widHelper_DestroyAudioBufferList(wwi->bufferList);
    wwi->bufferList = NULL;
    HeapFree(GetProcessHeap(), 0, wwi->bufferListCopy);
    wwi->bufferListCopy = NULL;

    wwi->audioUnit = NULL;
    wwi->state = WINE_WS_CLOSED;
    OSSpinLockUnlock(&wwi->lock);

    ret = widNotifyClient(wwi, WIM_CLOSE, 0L, 0L);

    return ret;
}


/**************************************************************************
 *                              widAddBuffer            [internal]
 */
static DWORD widAddBuffer(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    DWORD           ret = MMSYSERR_NOERROR;
    WINE_WAVEIN*    wwi;

    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);

    if (wDevID >= MAX_WAVEINDRV)
    {
        WARN("invalid device ID\n");
        return MMSYSERR_INVALHANDLE;
    }
    if (!(lpWaveHdr->dwFlags & WHDR_PREPARED))
    {
        TRACE("never been prepared !\n");
        return WAVERR_UNPREPARED;
    }
    if (lpWaveHdr->dwFlags & WHDR_INQUEUE)
    {
        TRACE("header already in use !\n");
        return WAVERR_STILLPLAYING;
    }

    wwi = &WInDev[wDevID];
    OSSpinLockLock(&wwi->lock);

    if (wwi->state == WINE_WS_CLOSED || wwi->state == WINE_WS_CLOSING)
    {
        WARN("Trying to add buffer to closed device.\n");
        ret = MMSYSERR_INVALHANDLE;
    }
    else
    {
        LPWAVEHDR* wh;

        lpWaveHdr->dwFlags |= WHDR_INQUEUE;
        lpWaveHdr->dwFlags &= ~WHDR_DONE;
        lpWaveHdr->dwBytesRecorded = 0;
        lpWaveHdr->lpNext = NULL;

        /* insert buffer at end of queue */
        for (wh = &(wwi->lpQueuePtr); *wh; wh = &((*wh)->lpNext))
            /* Do nothing */;
        *wh = lpWaveHdr;
    }

    OSSpinLockUnlock(&wwi->lock);

    return ret;
}


/**************************************************************************
 *                      widStart                                [internal]
 */
static DWORD widStart(WORD wDevID)
{
    DWORD           ret = MMSYSERR_NOERROR;
    WINE_WAVEIN*    wwi;

    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV)
    {
        WARN("invalid device ID\n");
        return MMSYSERR_INVALHANDLE;
    }

    /* The order of the following operations is important since we can't hold
     * the mutex while we make an Audio Unit call.  Set the PLAYING state
     * before starting the Audio Unit.  In widStop, the order is reversed.
     * This guarantees that we can't get into a situation where the state is
     * PLAYING but the Audio Unit isn't running.  Although we can be in STOPPED
     * state with the Audio Unit still running, that's harmless because the
     * input callback will just throw away the sound data.
     */
    wwi = &WInDev[wDevID];
    OSSpinLockLock(&wwi->lock);

    if (wwi->state == WINE_WS_CLOSED || wwi->state == WINE_WS_CLOSING)
    {
        WARN("Trying to start closed device.\n");
        ret = MMSYSERR_INVALHANDLE;
    }
    else
        wwi->state = WINE_WS_PLAYING;

    OSSpinLockUnlock(&wwi->lock);

    if (ret == MMSYSERR_NOERROR)
    {
        /* Start pulling for audio data */
        OSStatus err = AudioOutputUnitStart(wwi->audioUnit);
        if (err != noErr)
            ERR("Failed to start AU: %08lx\n", err);

        TRACE("Recording started...\n");
    }

    return ret;
}


/**************************************************************************
 *                      widStop                                 [internal]
 */
static DWORD widStop(WORD wDevID)
{
    DWORD           ret = MMSYSERR_NOERROR;
    WINE_WAVEIN*    wwi;
    WAVEHDR*        lpWaveHdr = NULL;
    OSStatus        err;

    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV)
    {
        WARN("invalid device ID\n");
        return MMSYSERR_INVALHANDLE;
    }

    wwi = &WInDev[wDevID];

    /* The order of the following operations is important since we can't hold
     * the mutex while we make an Audio Unit call.  Stop the Audio Unit before
     * setting the STOPPED state.  In widStart, the order is reversed.  This
     * guarantees that we can't get into a situation where the state is
     * PLAYING but the Audio Unit isn't running.  Although we can be in STOPPED
     * state with the Audio Unit still running, that's harmless because the
     * input callback will just throw away the sound data.
     */
    err = AudioOutputUnitStop(wwi->audioUnit);
    if (err != noErr)
        WARN("Failed to stop AU: %08lx\n", err);

    TRACE("Recording stopped.\n");

    OSSpinLockLock(&wwi->lock);

    if (wwi->state == WINE_WS_CLOSED || wwi->state == WINE_WS_CLOSING)
    {
        WARN("Trying to stop closed device.\n");
        ret = MMSYSERR_INVALHANDLE;
    }
    else if (wwi->state != WINE_WS_STOPPED)
    {
        wwi->state = WINE_WS_STOPPED;
        /* If there's a buffer in progress, it's done.  Remove it from the
         * queue so that we can return it to the app, below. */
        if (wwi->lpQueuePtr)
        {
            lpWaveHdr = wwi->lpQueuePtr;
            wwi->lpQueuePtr = lpWaveHdr->lpNext;
        }
    }

    OSSpinLockUnlock(&wwi->lock);

    if (lpWaveHdr)
    {
        lpWaveHdr->lpNext = NULL;
        lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
        lpWaveHdr->dwFlags |= WHDR_DONE;
        widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);
    }

    return ret;
}

/**************************************************************************
 *                      widGetPos                                 [internal]
 */
static DWORD widGetPos(WORD wDevID, LPMMTIME lpTime, UINT size)
{
    DWORD		    val;
    WINE_WAVEIN*    wwi;

    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV)
    {
        WARN("invalid device ID\n");
        return MMSYSERR_INVALHANDLE;
    }

    wwi = &WInDev[wDevID];

    OSSpinLockLock(&WInDev[wDevID].lock);
    val = wwi->dwTotalRecorded;
    OSSpinLockUnlock(&WInDev[wDevID].lock);

    return bytes_to_mmtime(lpTime, val, &wwi->format);
}

/**************************************************************************
 *                      widReset                                [internal]
 */
static DWORD widReset(WORD wDevID)
{
    DWORD           ret = MMSYSERR_NOERROR;
    WINE_WAVEIN*    wwi;
    WAVEHDR*        lpWaveHdr = NULL;

    TRACE("(%u);\n", wDevID);
    if (wDevID >= MAX_WAVEINDRV)
    {
        WARN("invalid device ID\n");
        return MMSYSERR_INVALHANDLE;
    }

    wwi = &WInDev[wDevID];
    OSSpinLockLock(&wwi->lock);

    if (wwi->state == WINE_WS_CLOSED || wwi->state == WINE_WS_CLOSING)
    {
        WARN("Trying to reset a closed device.\n");
        ret = MMSYSERR_INVALHANDLE;
    }
    else
    {
        lpWaveHdr               = wwi->lpQueuePtr;
        wwi->lpQueuePtr         = NULL;
        wwi->state              = WINE_WS_STOPPED;
        wwi->dwTotalRecorded    = 0;
    }

    OSSpinLockUnlock(&wwi->lock);

    if (ret == MMSYSERR_NOERROR)
    {
        OSStatus err = AudioOutputUnitStop(wwi->audioUnit);
        if (err != noErr)
            WARN("Failed to stop AU: %08lx\n", err);

        TRACE("Recording stopped.\n");
    }

    while (lpWaveHdr)
    {
        WAVEHDR* lpNext = lpWaveHdr->lpNext;

        lpWaveHdr->lpNext = NULL;
        lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
        lpWaveHdr->dwFlags |= WHDR_DONE;
        widNotifyClient(wwi, WIM_DATA, (DWORD)lpWaveHdr, 0);

        lpWaveHdr = lpNext;
    }

    return ret;
}


/**************************************************************************
 *                              widGetNumDevs                   [internal]
 */
static DWORD widGetNumDevs(void)
{
    return MAX_WAVEINDRV;
}


/**************************************************************************
 *                              widDevInterfaceSize             [internal]
 */
static DWORD widDevInterfaceSize(UINT wDevID, LPDWORD dwParam1)
{
    TRACE("(%u, %p)\n", wDevID, dwParam1);

    *dwParam1 = MultiByteToWideChar(CP_UNIXCP, 0, WInDev[wDevID].interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 *                              widDevInterface                 [internal]
 */
static DWORD widDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_UNIXCP, 0, WInDev[wDevID].interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_UNIXCP, 0, WInDev[wDevID].interface_name, -1,
                            dwParam1, dwParam2 / sizeof(WCHAR));
        return MMSYSERR_NOERROR;
    }
    return MMSYSERR_INVALPARAM;
}


/**************************************************************************
 *                              widDsCreate                     [internal]
 */
static DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv)
{
    TRACE("(%d,%p)\n",wDevID,drv);

    FIXME("DirectSoundCapture not implemented\n");
    FIXME("The (slower) DirectSound HEL mode will be used instead.\n");
    return MMSYSERR_NOTSUPPORTED;
}

/**************************************************************************
 *                              widDsDesc                       [internal]
 */
static DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    /* The DirectSound HEL will automatically wrap a non-DirectSound-capable
     * driver in a DirectSound adaptor, thus allowing the driver to be used by
     * DirectSound clients.  However, it only does this if we respond
     * successfully to the DRV_QUERYDSOUNDDESC message.  It's enough to fill in
     * the driver and device names of the description output parameter. */
    memset(desc, 0, sizeof(*desc));
    lstrcpynA(desc->szDrvname, "winecoreaudio.drv", sizeof(desc->szDrvname) - 1);
    lstrcpynA(desc->szDesc, WInDev[wDevID].interface_name, sizeof(desc->szDesc) - 1);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 *                              widMessage (WINECOREAUDIO.6)
 */
DWORD WINAPI CoreAudio_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
                            DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %04X, %08X, %08X, %08X);\n",
            wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg)
    {
        case DRVM_INIT:
        case DRVM_EXIT:
        case DRVM_ENABLE:
        case DRVM_DISABLE:
            /* FIXME: Pretend this is supported */
            return 0;
        case WIDM_OPEN:             return widOpen          (wDevID, (LPWAVEOPENDESC)dwParam1,  dwParam2);
        case WIDM_CLOSE:            return widClose         (wDevID);
        case WIDM_ADDBUFFER:        return widAddBuffer     (wDevID, (LPWAVEHDR)dwParam1,       dwParam2);
        case WIDM_PREPARE:          return MMSYSERR_NOTSUPPORTED;
        case WIDM_UNPREPARE:        return MMSYSERR_NOTSUPPORTED;
        case WIDM_GETDEVCAPS:       return widGetDevCaps    (wDevID, (LPWAVEINCAPSW)dwParam1,   dwParam2);
        case WIDM_GETNUMDEVS:       return widGetNumDevs    ();
        case WIDM_RESET:            return widReset         (wDevID);
        case WIDM_START:            return widStart         (wDevID);
        case WIDM_STOP:             return widStop          (wDevID);
        case WIDM_GETPOS:           return widGetPos        (wDevID, (LPMMTIME)dwParam1, (UINT)dwParam2  );
        case DRV_QUERYDEVICEINTERFACESIZE: return widDevInterfaceSize       (wDevID, (LPDWORD)dwParam1);
        case DRV_QUERYDEVICEINTERFACE:     return widDevInterface           (wDevID, (PWCHAR)dwParam1, dwParam2);
        case DRV_QUERYDSOUNDIFACE:  return widDsCreate   (wDevID, (PIDSCDRIVER*)dwParam1);
        case DRV_QUERYDSOUNDDESC:   return widDsDesc     (wDevID, (PDSDRIVERDESC)dwParam1);
        default:
            FIXME("unknown message %d!\n", wMsg);
    }

    return MMSYSERR_NOTSUPPORTED;
}


OSStatus CoreAudio_wiAudioUnitIOProc(void *inRefCon,
                                     AudioUnitRenderActionFlags *ioActionFlags,
                                     const AudioTimeStamp *inTimeStamp,
                                     UInt32 inBusNumber,
                                     UInt32 inNumberFrames,
                                     AudioBufferList *ioData)
{
    WINE_WAVEIN*    wwi = (WINE_WAVEIN*)inRefCon;
    OSStatus        err = noErr;
    BOOL            needNotify = FALSE;
    WAVEHDR*        lpStorePtr;
    unsigned int    dataToStore;
    unsigned int    dataStored = 0;


    if (wwi->trace_on)
        fprintf(stderr, "trace:wave:CoreAudio_wiAudioUnitIOProc (ioActionFlags = %08lx, "
	    "inTimeStamp = { %f, %x%08x, %f, %x%08x, %08lx }, inBusNumber = %lu, inNumberFrames = %lu)\n",
            *ioActionFlags, inTimeStamp->mSampleTime, (DWORD)(inTimeStamp->mHostTime >>32),
	    (DWORD)inTimeStamp->mHostTime, inTimeStamp->mRateScalar, (DWORD)(inTimeStamp->mWordClockTime >> 32),
	    (DWORD)inTimeStamp->mWordClockTime, inTimeStamp->mFlags, inBusNumber, inNumberFrames);

    /* Render into audio buffer */
    /* FIXME: implement sample rate conversion on input.  This will require
     * a different render strategy.  We'll need to buffer the sound data
     * received here and pass it off to an AUConverter in another thread. */
    err = AudioUnitRender(wwi->audioUnit, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, wwi->bufferList);
    if (err)
    {
        if (wwi->err_on)
            fprintf(stderr, "err:wave:CoreAudio_wiAudioUnitIOProc AudioUnitRender failed with error %li\n", err);
        return err;
    }

    /* Copy from audio buffer to the wavehdrs */
    dataToStore = wwi->bufferList->mBuffers[0].mDataByteSize;

    OSSpinLockLock(&wwi->lock);

    lpStorePtr = wwi->lpQueuePtr;

    /* We might have been called while the waveIn device is being closed in
     * widClose.  We have to do nothing in that case.  The check of wwi->state
     * below ensures that. */
    while (dataToStore > 0 && wwi->state == WINE_WS_PLAYING && lpStorePtr)
    {
        unsigned int room = lpStorePtr->dwBufferLength - lpStorePtr->dwBytesRecorded;
        unsigned int toCopy;

        if (wwi->trace_on)
            fprintf(stderr, "trace:wave:CoreAudio_wiAudioUnitIOProc Looking to store %u bytes to wavehdr %p, which has room for %u\n",
                dataToStore, lpStorePtr, room);

        if (room >= dataToStore)
            toCopy = dataToStore;
        else
            toCopy = room;

        if (toCopy > 0)
        {
            memcpy(lpStorePtr->lpData + lpStorePtr->dwBytesRecorded,
                (char*)wwi->bufferList->mBuffers[0].mData + dataStored, toCopy);
            lpStorePtr->dwBytesRecorded += toCopy;
            wwi->dwTotalRecorded += toCopy;
            dataStored += toCopy;
            dataToStore -= toCopy;
            room -= toCopy;
        }

        if (room == 0)
        {
            lpStorePtr = lpStorePtr->lpNext;
            needNotify = TRUE;
        }
    }

    OSSpinLockUnlock(&wwi->lock);

    /* Restore the audio buffer list structure from backup, in case
     * AudioUnitRender clobbered it.  (It modifies mDataByteSize and may even
     * give us a different mData buffer to avoid a copy.) */
    memcpy(wwi->bufferList, wwi->bufferListCopy, AUDIOBUFFERLISTSIZE(wwi->bufferList->mNumberBuffers));

    if (needNotify) wodSendNotifyInputCompletionsMessage(wwi);
    return err;
}

#else

/**************************************************************************
 *                              widMessage (WINECOREAUDIO.6)
 */
DWORD WINAPI CoreAudio_widMessage(WORD wDevID, WORD wMsg, DWORD dwUser,
                            DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X): CoreAudio support not compiled into wine\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

/**************************************************************************
* 				wodMessage (WINECOREAUDIO.7)
*/
DWORD WINAPI CoreAudio_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
                                  DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08X, %08X, %08X): CoreAudio support not compiled into wine\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif
