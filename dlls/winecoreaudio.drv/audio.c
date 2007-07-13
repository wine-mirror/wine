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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winerror.h"
#include "mmddk.h"
#include "dsound.h"
#include "dsdriver.h"
#include "coreaudio.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);


#if defined(HAVE_COREAUDIO_COREAUDIO_H) && defined(HAVE_AUDIOUNIT_AUDIOUNIT_H)
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#include <libkern/OSAtomic.h>

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
* |	     | open()	   |		   | STOPPED		       	     |
* | PAUSED  | write()	   | 		   | PAUSED		       	     |
* | STOPPED | write()	   | <thrd create> | PLAYING		  	     |
* | PLAYING | write()	   | HEADER        | PLAYING		  	     |
* | (other) | write()	   | <error>       |		       		     |
* | (any)   | pause()	   | PAUSING	   | PAUSED		       	     |
* | PAUSED  | restart()   | RESTARTING    | PLAYING (if no thrd => STOPPED) |
* | (any)   | reset()	   | RESETTING     | STOPPED		      	     |
* | (any)   | close()	   | CLOSING	   | CLOSED		      	     |
* +---------+-------------+---------------+---------------------------------+
*/

/* states of the playing device */
#define	WINE_WS_PLAYING   0
#define	WINE_WS_PAUSED    1
#define	WINE_WS_STOPPED   2
#define WINE_WS_CLOSED    3

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
    volatile int                state;      /* one of the WINE_WS_ manifest constants */
    CoreAudio_Device            *cadev;
    WAVEOPENDESC                waveDesc;
    WORD                        wFlags;
    PCMWAVEFORMAT               format;
    DWORD                       woID;
    AudioUnit                   audioUnit;
    AudioStreamBasicDescription streamDescription;
    
    WAVEOUTCAPSW                caps;
    char                        interface_name[32];
    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */
    DWORD			dwPartialOffset;	/* Offset of not yet written bytes in lpPlayPtr */
    
    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD			dwLoops;		/* private copy of loop counter */
    
    DWORD			dwPlayedTotal;		/* number of bytes actually played since opening */
    DWORD                       dwWrittenTotal;         /* number of bytes written to OSS buffer since opening */
        
    DWORD                       tickCountMS; /* time in MS of last AudioUnit callback */

    OSSpinLock                  lock;         /* synchronization stuff */

    BOOL trace_on;
    BOOL warn_on;
    BOOL err_on;
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

static void wodHelper_PlayPtrNext(WINE_WAVEOUT* wwo);
static void wodHelper_NotifyDoneForList(WINE_WAVEOUT* wwo, LPWAVEHDR lpWaveHdr);
static void wodHelper_NotifyCompletions(WINE_WAVEOUT* wwo, BOOL force);
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
    static char unknown[32];
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
    sprintf(unknown, "UNKNOWN(0x%04x)", msg);
    return unknown;
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
            buffer = (UInt32 *) CFDataGetBytePtr(data);
            wodHelper_NotifyCompletions(&WOutDev[buffer[0]], FALSE);
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
    
    source = CFMessagePortCreateRunLoopSource(kCFAllocatorDefault, port_ReceiveInMessageThread, (CFIndex)0);
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
static void wodSendNotifyCompletionsMessage(WINE_WAVEOUT* wwo)
{
    CFDataRef data;
    UInt32 buffer;

    buffer = (UInt32) wwo->woID;

    data = CFDataCreate(kCFAllocatorDefault, (UInt8 *)&buffer, sizeof(buffer));
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
        ERR("AudioHardwareGetProperty for kAudioDevicePropertyDeviceName return %c%c%c%c\n", (char) (status >> 24),
                                                                                             (char) (status >> 16),
                                                                                             (char) (status >> 8),
                                                                                             (char) status);
        return FALSE;
    }
    
    memcpy(CoreAudio_DefaultDevice.ds_desc.szDesc, name, sizeof(name));
    strcpy(CoreAudio_DefaultDevice.ds_desc.szDrvname, "winecoreaudio.drv");
    MultiByteToWideChar(CP_ACP, 0, name, sizeof(name), 
                        CoreAudio_DefaultDevice.out_caps.szPname, 
                        sizeof(CoreAudio_DefaultDevice.out_caps.szPname) / sizeof(WCHAR));
    memcpy(CoreAudio_DefaultDevice.dev_name, name, 32);
    
    propertySize = sizeof(CoreAudio_DefaultDevice.streamDescription);
    status = AudioDeviceGetProperty(devId, 0, FALSE , kAudioDevicePropertyStreamFormat, &propertySize, &CoreAudio_DefaultDevice.streamDescription);
    if (status != noErr) {
        ERR("AudioHardwareGetProperty for kAudioDevicePropertyStreamFormat return %c%c%c%c\n", (char) (status >> 24),
                                                                                                (char) (status >> 16),
                                                                                                (char) (status >> 8),
                                                                                                (char) status);
        return FALSE;
    }
    
    TRACE("Device Stream Description mSampleRate : %f\n mFormatID : %c%c%c%c\n"
            "mFormatFlags : %lX\n mBytesPerPacket : %lu\n mFramesPerPacket : %lu\n"
            "mBytesPerFrame : %lu\n mChannelsPerFrame : %lu\n mBitsPerChannel : %lu\n",
                               CoreAudio_DefaultDevice.streamDescription.mSampleRate,
                               (char) (CoreAudio_DefaultDevice.streamDescription.mFormatID >> 24),
                               (char) (CoreAudio_DefaultDevice.streamDescription.mFormatID >> 16),
                               (char) (CoreAudio_DefaultDevice.streamDescription.mFormatID >> 8),
                               (char) CoreAudio_DefaultDevice.streamDescription.mFormatID,
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
    CHAR szPname[MAXPNAMELEN];
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
        ERR("AudioHardwareGetProperty return %c%c%c%c for kAudioHardwarePropertyDefaultOutputDevice\n", (char) (status >> 24),
                                                                                                (char) (status >> 16),
                                                                                                (char) (status >> 8),
                                                                                                (char) status);
        return 1;
    }
    if (CoreAudio_DefaultDevice.outputDeviceID == kAudioDeviceUnknown) {
        ERR("AudioHardwareGetProperty: CoreAudio_DefaultDevice.outputDeviceID == kAudioDeviceUnknown\n");
        return 1;
    }
    
    if ( ! CoreAudio_GetDevCaps() )
        return 1;
    
    CoreAudio_DefaultDevice.interface_name=HeapAlloc(GetProcessHeap(),0,strlen(CoreAudio_DefaultDevice.dev_name)+1);
    sprintf(CoreAudio_DefaultDevice.interface_name, "%s", CoreAudio_DefaultDevice.dev_name);
    
    for (i = 0; i < MAX_WAVEOUTDRV; ++i)
    {
        WOutDev[i].state = WINE_WS_CLOSED;
        WOutDev[i].cadev = &CoreAudio_DefaultDevice; 
        WOutDev[i].woID = i;
        
        memset(&WOutDev[i].caps, 0, sizeof(WOutDev[i].caps));
        
        WOutDev[i].caps.wMid = 0xcafe; 	/* Manufac ID */
        WOutDev[i].caps.wPid = 0x0001; 	/* Product ID */
        snprintf(szPname, sizeof(szPname), "CoreAudio WaveOut %d", i);
        MultiByteToWideChar(CP_ACP, 0, szPname, -1, WOutDev[i].caps.szPname, sizeof(WOutDev[i].caps.szPname)/sizeof(WCHAR));
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

        WOutDev[i].lock = 0; /* initialize the mutex */
    }

    /* FIXME: implement sample rate conversion on input */
    inputSampleRate = AudioUnit_GetInputDeviceSampleRate();

    for (i = 0; i < MAX_WAVEINDRV; ++i)
    {
        memset(&WInDev[i], 0, sizeof(WInDev[i]));
        WInDev[i].wiID = i;

        /* Establish preconditions for widOpen */
        WInDev[i].state = WINE_WS_CLOSED;
        WInDev[i].lock = 0; /* initialize the mutex */

        /* Fill in capabilities.  widGetDevCaps can be called at any time. */
        WInDev[i].caps.wMid = 0xcafe; 	/* Manufac ID */
        WInDev[i].caps.wPid = 0x0001; 	/* Product ID */
        WInDev[i].caps.vDriverVersion = 0x0001;

        snprintf(szPname, sizeof(szPname), "CoreAudio WaveIn %d", i);
        MultiByteToWideChar(CP_ACP, 0, szPname, -1, WInDev[i].caps.szPname, sizeof(WInDev[i].caps.szPname)/sizeof(WCHAR));
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
        return 1;
    }

    port_ReceiveInMessageThread = CFMessagePortCreateLocal(kCFAllocatorDefault, messageThreadPortName,
                                        &wodMessageHandler, NULL, NULL);
    if (!port_ReceiveInMessageThread)
    {
        ERR("Can't create message thread local port\n");
        CFRelease(messageThreadPortName);
        return 1;
    }

    Port_SendToMessageThread = CFMessagePortCreateRemote(kCFAllocatorDefault, messageThreadPortName);
    CFRelease(messageThreadPortName);
    if (!Port_SendToMessageThread)
    {
        ERR("Can't create port for sending to message thread\n");
        CFRelease(port_ReceiveInMessageThread);
        return 1;
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
        return 1;
    }

    /* The message thread is responsible for releasing port_ReceiveInMessageThread. */

    return 0;
}

void CoreAudio_WaveRelease(void)
{
    /* Stop CFRunLoop in messageThread */
    TRACE("()\n");

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
static DWORD wodNotifyClient(WINE_WAVEOUT* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    switch (wMsg) {
        case WOM_OPEN:
        case WOM_CLOSE:
        case WOM_DONE:
            if (wwo->wFlags != DCB_NULL &&
                !DriverCallback(wwo->waveDesc.dwCallback, wwo->wFlags,
                                (HDRVR)wwo->waveDesc.hWave, wMsg, wwo->waveDesc.dwInstance,
                                dwParam1, dwParam2))
            {
                return MMSYSERR_ERROR;
            }
            break;
        default:
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
static DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags)
{
    WINE_WAVEOUT*	wwo;
    DWORD retval;
    DWORD               ret;
    AudioStreamBasicDescription streamFormat;

    TRACE("(%u, %p, %08x);\n", wDevID, lpDesc, dwFlags);
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
    
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
        lpDesc->lpFormat->nChannels == 0 ||
        lpDesc->lpFormat->nSamplesPerSec == 0
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
    
    wwo = &WOutDev[wDevID];
    if (!OSSpinLockTry(&wwo->lock))
        return MMSYSERR_ALLOCATED;

    if (wwo->state != WINE_WS_CLOSED)
    {
        OSSpinLockUnlock(&wwo->lock);
        return MMSYSERR_ALLOCATED;
    }

    if (!AudioUnit_CreateDefaultAudioUnit((void *) wwo, &wwo->audioUnit))
    {
        ERR("CoreAudio_CreateDefaultAudioUnit(%p) failed\n", wwo);
        OSSpinLockUnlock(&wwo->lock);
        return MMSYSERR_ERROR;
    }

    if ((dwFlags & WAVE_DIRECTSOUND) && 
        !(wwo->caps.dwSupport & WAVECAPS_DIRECTSOUND))
	/* not supported, ignore it */
	dwFlags &= ~WAVE_DIRECTSOUND;

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

    ret = AudioUnit_InitializeWithStreamDescription(wwo->audioUnit, &streamFormat);
    if (!ret) 
    {
        AudioUnit_CloseAudioUnit(wwo->audioUnit);
        OSSpinLockUnlock(&wwo->lock);
        return WAVERR_BADFORMAT; /* FIXME return an error based on the OSStatus */
    }
    wwo->streamDescription = streamFormat;

    ret = AudioOutputUnitStart(wwo->audioUnit);
    if (ret)
    {
        ERR("AudioOutputUnitStart failed: %08x\n", ret);
        AudioUnitUninitialize(wwo->audioUnit);
        AudioUnit_CloseAudioUnit(wwo->audioUnit);
        OSSpinLockUnlock(&wwo->lock);
        return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
    }

    wwo->state = WINE_WS_STOPPED;
                
    wwo->wFlags = HIWORD(dwFlags & CALLBACK_TYPEMASK);
    
    memcpy(&wwo->waveDesc, lpDesc, 	     sizeof(WAVEOPENDESC));
    memcpy(&wwo->format,   lpDesc->lpFormat, sizeof(PCMWAVEFORMAT));
    
    if (wwo->format.wBitsPerSample == 0) {
	WARN("Resetting zeroed wBitsPerSample\n");
	wwo->format.wBitsPerSample = 8 *
	    (wwo->format.wf.nAvgBytesPerSec /
	     wwo->format.wf.nSamplesPerSec) /
	    wwo->format.wf.nChannels;
    }
    
    wwo->dwPlayedTotal = 0;
    wwo->dwWrittenTotal = 0;

    wwo->trace_on = TRACE_ON(wave);
    wwo->warn_on  = WARN_ON(wave);
    wwo->err_on   = ERR_ON(wave);

    OSSpinLockUnlock(&wwo->lock);
    
    retval = wodNotifyClient(wwo, WOM_OPEN, 0L, 0L);
    
    return retval;
}

/**************************************************************************
* 				wodClose			[internal]
*/
static DWORD wodClose(WORD wDevID)
{
    DWORD		ret = MMSYSERR_NOERROR;
    WINE_WAVEOUT*	wwo;
    
    TRACE("(%u);\n", wDevID);
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    wwo = &WOutDev[wDevID];
    OSSpinLockLock(&wwo->lock);
    if (wwo->lpQueuePtr)
    {
        WARN("buffers still playing !\n");
        OSSpinLockUnlock(&wwo->lock);
        ret = WAVERR_STILLPLAYING;
    } else
    {
        OSStatus err;
        /* sanity check: this should not happen since the device must have been reset before */
        if (wwo->lpQueuePtr || wwo->lpPlayPtr) ERR("out of sync\n");
        
        wwo->state = WINE_WS_CLOSED; /* mark the device as closed */
        
        OSSpinLockUnlock(&wwo->lock);

        err = AudioUnitUninitialize(wwo->audioUnit);
        if (err) {
            ERR("AudioUnitUninitialize return %c%c%c%c\n", (char) (err >> 24),
                                                            (char) (err >> 16),
                                                            (char) (err >> 8),
                                                            (char) err);
            return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
        }
        
        if ( !AudioUnit_CloseAudioUnit(wwo->audioUnit) )
        {
            ERR("Can't close AudioUnit\n");
            return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
        }  
        
        ret = wodNotifyClient(wwo, WOM_CLOSE, 0L, 0L);
    }
    
    return ret;
}

/**************************************************************************
* 				wodPrepare			[internal]
*/
static DWORD wodPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08x);\n", wDevID, lpWaveHdr, dwSize);
    
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
static DWORD wodUnprepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08x);\n", wDevID, lpWaveHdr, dwSize);
    
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
* This is called with the WAVEOUT lock held.
* Call from AudioUnit IO thread can't use Wine debug channels.
*/
static void wodHelper_CheckForLoopBegin(WINE_WAVEOUT* wwo)
{
    LPWAVEHDR lpWaveHdr = wwo->lpPlayPtr;

    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)
    {
        if (wwo->lpLoopPtr)
        {
            if (wwo->warn_on)
                fprintf(stderr, "warn:winecoreaudio:wodHelper_CheckForLoopBegin Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
        }
        else
        {
            if (wwo->trace_on)
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
* This is called with the WAVEOUT lock held.
* Call from AudioUnit IO thread can't use Wine debug channels.
*/
static void wodHelper_PlayPtrNext(WINE_WAVEOUT* wwo)
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

        if (!wwo->lpPlayPtr)
            wwo->state = WINE_WS_STOPPED;
        else
            wodHelper_CheckForLoopBegin(wwo);
    }
}

/* Send the "done" notification for each WAVEHDR in a list.  The list must be
 * free-standing.  It should not be part of a device's queue.
 * This function must be called with the WAVEOUT lock *not* held.  Furthermore,
 * it does not lock it, itself.  That's because the callback to the application
 * may prompt the application to operate on the device, and we don't want to
 * deadlock.
 */
static void wodHelper_NotifyDoneForList(WINE_WAVEOUT* wwo, LPWAVEHDR lpWaveHdr)
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
static void wodHelper_NotifyCompletions(WINE_WAVEOUT* wwo, BOOL force)
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
static DWORD wodWrite(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    LPWAVEHDR*wh;
    WINE_WAVEOUT *wwo;
    
    TRACE("(%u, %p, %08X);\n", wDevID, lpWaveHdr, dwSize);
    
    /* first, do the sanity checks... */
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad dev ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    wwo = &WOutDev[wDevID];
    
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

        if (wwo->state == WINE_WS_STOPPED)
            wwo->state = WINE_WS_PLAYING;

        wodHelper_CheckForLoopBegin(wwo);

        wwo->dwPartialOffset = 0;
    }
    OSSpinLockUnlock(&wwo->lock);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 			wodPause				[internal]
*/
static DWORD wodPause(WORD wDevID)
{
    OSStatus status;

    TRACE("(%u);!\n", wDevID);
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    /* The order of the following operations is important since we can't hold
     * the mutex while we make an Audio Unit call.  Stop the Audio Unit before
     * setting the PAUSED state.  In wodRestart, the order is reversed.  This
     * guarantees that we can't get into a situation where the state is
     * PLAYING or STOPPED but the Audio Unit isn't running.  Although we can
     * be in PAUSED state with the Audio Unit still running, that's harmless
     * because the render callback will just produce silence.
     */
    status = AudioOutputUnitStop(WOutDev[wDevID].audioUnit);
    if (status) {
        WARN("AudioOutputUnitStop return %c%c%c%c\n",
             (char) (status >> 24), (char) (status >> 16), (char) (status >> 8), (char) status);
    }

    OSSpinLockLock(&WOutDev[wDevID].lock);
    if (WOutDev[wDevID].state == WINE_WS_PLAYING || WOutDev[wDevID].state == WINE_WS_STOPPED)
        WOutDev[wDevID].state = WINE_WS_PAUSED;
    OSSpinLockUnlock(&WOutDev[wDevID].lock);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 			wodRestart				[internal]
*/
static DWORD wodRestart(WORD wDevID)
{
    OSStatus status;

    TRACE("(%u);\n", wDevID);
    
    if (wDevID >= MAX_WAVEOUTDRV )
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    /* The order of the following operations is important since we can't hold
     * the mutex while we make an Audio Unit call.  Set the PLAYING/STOPPED
     * state before starting the Audio Unit.  In wodPause, the order is
     * reversed.  This guarantees that we can't get into a situation where
     * the state is PLAYING or STOPPED but the Audio Unit isn't running.
     * Although we can be in PAUSED state with the Audio Unit still running,
     * that's harmless because the render callback will just produce silence.
     */
    OSSpinLockLock(&WOutDev[wDevID].lock);
    if (WOutDev[wDevID].state == WINE_WS_PAUSED)
    {
        if (WOutDev[wDevID].lpPlayPtr)
            WOutDev[wDevID].state = WINE_WS_PLAYING;
        else
            WOutDev[wDevID].state = WINE_WS_STOPPED;
    }
    OSSpinLockUnlock(&WOutDev[wDevID].lock);

    status = AudioOutputUnitStart(WOutDev[wDevID].audioUnit);
    if (status) {
        ERR("AudioOutputUnitStart return %c%c%c%c\n",
            (char) (status >> 24), (char) (status >> 16), (char) (status >> 8), (char) status);
        return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
    }

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 			wodReset				[internal]
*/
static DWORD wodReset(WORD wDevID)
{
    WINE_WAVEOUT* wwo;
    OSStatus status;
    LPWAVEHDR lpSavedQueuePtr;

    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    wwo = &WOutDev[wDevID];

    OSSpinLockLock(&wwo->lock);

    if (wwo->state == WINE_WS_CLOSED)
    {
        OSSpinLockUnlock(&wwo->lock);
        WARN("resetting a closed device\n");
        return MMSYSERR_INVALHANDLE;
    }

    lpSavedQueuePtr = wwo->lpQueuePtr;
    wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
    wwo->state = WINE_WS_STOPPED;
    wwo->dwPlayedTotal = wwo->dwWrittenTotal = 0;

    wwo->dwPartialOffset = 0;        /* Clear partial wavehdr */

    OSSpinLockUnlock(&wwo->lock);

    status = AudioOutputUnitStart(wwo->audioUnit);

    if (status) {
        ERR( "AudioOutputUnitStart return %c%c%c%c\n",
             (char) (status >> 24), (char) (status >> 16), (char) (status >> 8), (char) status);
        return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
    }

    /* Now, send the "done" notification for each header in our list. */
    /* Do this last so the reset operation is effectively complete before the
     * app does whatever it's going to do in response to these notifications. */
    wodHelper_NotifyDoneForList(wwo, lpSavedQueuePtr);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodGetPosition			[internal]
*/
static DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    DWORD		val;
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %u);\n", wDevID, lpTime, uSize);
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    /* if null pointer to time structure return error */
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
    
    wwo = &WOutDev[wDevID];
    
    OSSpinLockLock(&WOutDev[wDevID].lock);
    val = wwo->dwPlayedTotal;
    OSSpinLockUnlock(&WOutDev[wDevID].lock);
    
    return bytes_to_mmtime(lpTime, val, &wwo->format);
}

/**************************************************************************
* 				wodGetVolume			[internal]
*/
static DWORD wodGetVolume(WORD wDevID, LPDWORD lpdwVol)
{
    float left;
    float right;
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }    
    
    TRACE("(%u, %p);\n", wDevID, lpdwVol);

    AudioUnit_GetVolume(WOutDev[wDevID].audioUnit, &left, &right); 

    *lpdwVol = ((WORD) left * 0xFFFFl) + (((WORD) right * 0xFFFFl) << 16);
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodSetVolume			[internal]
*/
static DWORD wodSetVolume(WORD wDevID, DWORD dwParam)
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
    
    TRACE("(%u, %08x);\n", wDevID, dwParam);

    AudioUnit_SetVolume(WOutDev[wDevID].audioUnit, left, right); 

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
    
    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].cadev->interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
*                              wodDevInterface                 [internal]
*/
static DWORD wodDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    TRACE("\n");
    if (dwParam2 >= MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].cadev->interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_ACP, 0, WOutDev[wDevID].cadev->interface_name, -1,
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
    memcpy(desc, &(WOutDev[wDevID].cadev->ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 				wodMessage (WINECOREAUDIO.7)
*/
DWORD WINAPI CoreAudio_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser, 
                                  DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %s, %08x, %08x, %08x);\n",
          wDevID, getMessage(wMsg), dwUser, dwParam1, dwParam2);
    
    switch (wMsg) {
        case DRVM_INIT:
        case DRVM_EXIT:
        case DRVM_ENABLE:
        case DRVM_DISABLE:
            
            /* FIXME: Pretend this is supported */
            return 0;
        case WODM_OPEN:         return wodOpen(wDevID, (LPWAVEOPENDESC) dwParam1, dwParam2);          
        case WODM_CLOSE:        return wodClose(wDevID);
        case WODM_WRITE:        return wodWrite(wDevID, (LPWAVEHDR) dwParam1, dwParam2);
        case WODM_PAUSE:        return wodPause(wDevID);  
        case WODM_GETPOS:       return wodGetPosition(wDevID, (LPMMTIME) dwParam1, dwParam2);
        case WODM_BREAKLOOP:    return MMSYSERR_NOTSUPPORTED;
        case WODM_PREPARE:      return wodPrepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
        case WODM_UNPREPARE:    return wodUnprepare(wDevID, (LPWAVEHDR)dwParam1, dwParam2);
            
        case WODM_GETDEVCAPS:   return wodGetDevCaps(wDevID, (LPWAVEOUTCAPSW) dwParam1, dwParam2);
        case WODM_GETNUMDEVS:   return wodGetNumDevs();  
            
        case WODM_GETPITCH:         
        case WODM_SETPITCH:        
        case WODM_GETPLAYBACKRATE:	
        case WODM_SETPLAYBACKRATE:	return MMSYSERR_NOTSUPPORTED;
        case WODM_GETVOLUME:    return wodGetVolume(wDevID, (LPDWORD)dwParam1);
        case WODM_SETVOLUME:    return wodSetVolume(wDevID, dwParam1);
        case WODM_RESTART:      return wodRestart(wDevID);
        case WODM_RESET:    	return wodReset(wDevID);
            
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
    WINE_WAVEOUT *wwo = (WINE_WAVEOUT *) inRefCon;
    int needNotify = 0;

    unsigned int dataNeeded = ioData->mBuffers[0].mDataByteSize;
    unsigned int dataProvided = 0;

    OSSpinLockLock(&wwo->lock);

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

    list = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, offsetof(AudioBufferList, mBuffers) + numBuffers * sizeof(AudioBuffer));
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

    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
        lpDesc->lpFormat->nChannels == 0 ||
        lpDesc->lpFormat->nSamplesPerSec == 0
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

    memcpy(&wwi->waveDesc, lpDesc,              sizeof(WAVEOPENDESC));
    memcpy(&wwi->format,   lpDesc->lpFormat,    sizeof(PCMWAVEFORMAT));

    if (wwi->format.wBitsPerSample == 0)
    {
        WARN("Resetting zeroed wBitsPerSample\n");
        wwi->format.wBitsPerSample = 8 *
            (wwi->format.wf.nAvgBytesPerSec /
            wwi->format.wf.nSamplesPerSec) /
            wwi->format.wf.nChannels;
    }

    wwi->dwTotalRecorded = 0;

    wwi->trace_on = TRACE_ON(wave);
    wwi->warn_on  = WARN_ON(wave);
    wwi->err_on   = ERR_ON(wave);

    if (!AudioUnit_CreateInputUnit(wwi, &wwi->audioUnit,
        wwi->format.wf.nChannels, wwi->format.wf.nSamplesPerSec,
        wwi->format.wBitsPerSample, &frameCount))
    {
        ERR("AudioUnit_CreateInputUnit failed\n");
        OSSpinLockUnlock(&wwi->lock);
        return MMSYSERR_ERROR;
    }

    /* Allocate our audio buffers */
    wwi->bufferList = widHelper_AllocateAudioBufferList(wwi->format.wf.nChannels,
        wwi->format.wBitsPerSample, frameCount, TRUE);
    if (wwi->bufferList == NULL)
    {
        ERR("Failed to allocate buffer list\n");
        AudioUnitUninitialize(wwi->audioUnit);
        AudioUnit_CloseAudioUnit(wwi->audioUnit);
        OSSpinLockUnlock(&wwi->lock);
        return MMSYSERR_NOMEM;
    }

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

    TRACE("(%u);\n", wDevID);

    if (wDevID >= MAX_WAVEINDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }

    wwi = &WInDev[wDevID];
    OSSpinLockLock(&wwi->lock);
    if (wwi->state == WINE_WS_CLOSED)
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
        wwi->state = WINE_WS_CLOSED;
    }

    OSSpinLockUnlock(&wwi->lock);

    if (ret == MMSYSERR_NOERROR)
    {
        OSStatus err = AudioUnitUninitialize(wwi->audioUnit);
        if (err)
        {
            ERR("AudioUnitUninitialize return %c%c%c%c\n", (char) (err >> 24),
                                                           (char) (err >> 16),
                                                           (char) (err >> 8),
                                                           (char) err);
        }

        if (!AudioUnit_CloseAudioUnit(wwi->audioUnit))
        {
            ERR("Can't close AudioUnit\n");
        }

        /* Dellocate our audio buffers */
        widHelper_DestroyAudioBufferList(wwi->bufferList);
        wwi->bufferList = NULL;

        ret = widNotifyClient(wwi, WIM_CLOSE, 0L, 0L);
    }

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

    if (wwi->state == WINE_WS_CLOSED)
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

    if (wwi->state == WINE_WS_CLOSED)
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

    if (wwi->state == WINE_WS_CLOSED)
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

    if (wwi->state == WINE_WS_CLOSED)
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

    *dwParam1 = MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
                                    NULL, 0 ) * sizeof(WCHAR);
    return MMSYSERR_NOERROR;
}


/**************************************************************************
 *                              widDevInterface                 [internal]
 */
static DWORD widDevInterface(UINT wDevID, PWCHAR dwParam1, DWORD dwParam2)
{
    if (dwParam2 >= MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
                                        NULL, 0 ) * sizeof(WCHAR))
    {
        MultiByteToWideChar(CP_ACP, 0, WInDev[wDevID].interface_name, -1,
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
