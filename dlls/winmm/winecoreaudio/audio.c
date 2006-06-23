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
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

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

/*
    Due to AudioUnit headers conflict define some needed types.
*/

typedef void *AudioUnit;

/* From AudioUnit/AUComponents.h */
typedef UInt32 AudioUnitRenderActionFlags;

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

    pthread_mutex_t             lock;         /* synchronization stuff */
} WINE_WAVEOUT;

typedef struct {
    volatile int    state;
    CoreAudio_Device *cadev;
    WAVEOPENDESC    waveDesc;
    WORD            wFlags;
    PCMWAVEFORMAT   format;
    LPWAVEHDR       lpQueuePtr;
    DWORD           dwTotalRecorded;
    WAVEINCAPSW     caps;
    
    AudioUnit                   audioUnit;
    AudioStreamBasicDescription streamDescription;
        
    /*  BOOL            bTriggerSupport;
    WORD              wDevID;
    char              interface_name[32];*/
        
    /* synchronization stuff */
    pthread_mutex_t lock;
} WINE_WAVEIN;

static WINE_WAVEOUT WOutDev   [MAX_WAVEOUTDRV];

static CFStringRef MessageThreadPortName;

static LPWAVEHDR wodHelper_PlayPtrNext(WINE_WAVEOUT* wwo);
static DWORD wodHelper_NotifyCompletions(WINE_WAVEOUT* wwo, BOOL force);

extern int AudioUnit_CreateDefaultAudioUnit(void *wwo, AudioUnit *au);
extern int AudioUnit_CloseAudioUnit(AudioUnit au);
extern int AudioUnit_InitializeWithStreamDescription(AudioUnit au, AudioStreamBasicDescription *streamFormat);

extern OSStatus AudioOutputUnitStart(AudioUnit au);
extern OSStatus AudioOutputUnitStop(AudioUnit au);
extern OSStatus AudioUnitUninitialize(AudioUnit au);

extern int AudioUnit_SetVolume(AudioUnit au, float left, float right);
extern int AudioUnit_GetVolume(AudioUnit au, float *left, float *right);

OSStatus CoreAudio_woAudioUnitIOProc(void *inRefCon, 
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
#define kWaveOutCallbackMessage 1
#define kWaveInCallbackMessage 2

/* Mach Message Handling */
static CFDataRef wodMessageHandler(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void *info)
{
    UInt32 *buffer = NULL;
    WINE_WAVEOUT* wwo = NULL;
    
    switch (msgid)
    {
        case kWaveOutCallbackMessage:
            buffer = (UInt32 *) CFDataGetBytePtr(data);
            wwo = &WOutDev[buffer[0]];
            
            pthread_mutex_lock(&wwo->lock);
            DriverCallback(wwo->waveDesc.dwCallback, wwo->wFlags,
                           (HDRVR)wwo->waveDesc.hWave, (WORD)buffer[1], wwo->waveDesc.dwInstance,
                           (DWORD)buffer[2], (DWORD)buffer[3]);
            pthread_mutex_unlock(&wwo->lock);
            break;
        case kWaveInCallbackMessage:
        default:
            CFRunLoopStop(CFRunLoopGetCurrent());
            break;
    }
    
    return NULL;
}

static DWORD WINAPI messageThread(LPVOID p)
{
    CFMessagePortRef local;
    CFRunLoopSourceRef source;
    Boolean info;
    
    local = CFMessagePortCreateLocal(kCFAllocatorDefault, MessageThreadPortName,
                                        &wodMessageHandler, NULL, &info);
                                        
    source = CFMessagePortCreateRunLoopSource(kCFAllocatorDefault, local, (CFIndex)0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), source, kCFRunLoopDefaultMode);
        
    CFRunLoopRun();

    CFRunLoopSourceInvalidate(source);
    CFRelease(source);
    CFRelease(local);
    CFRelease(MessageThreadPortName);
    MessageThreadPortName = NULL;

    return 0;
}

static DWORD wodSendDriverCallbackMessage(WINE_WAVEOUT* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    CFDataRef data;
    UInt32 buffer[4];
    SInt32 ret;
    
    CFMessagePortRef messagePort;
    messagePort = CFMessagePortCreateRemote(kCFAllocatorDefault, MessageThreadPortName);
        
    buffer[0] = (UInt32) wwo->woID;
    buffer[1] = (UInt32) wMsg;
    buffer[2] = (UInt32) dwParam1;
    buffer[3] = (UInt32) dwParam2;

    data = CFDataCreate(kCFAllocatorDefault, (UInt8 *)buffer, sizeof(buffer));
    if (!data)
        return 0;
    
    ret = CFMessagePortSendRequest(messagePort, kWaveOutCallbackMessage, data, 0.0, 0.0, NULL, NULL);
    CFRelease(data);
    CFRelease(messagePort);
    
    return (ret == kCFMessagePortSuccess)?1:0;
}

static DWORD bytes_to_mmtime(LPMMTIME lpTime, DWORD position,
                             PCMWAVEFORMAT* format)
{
    TRACE("wType=%04X wBitsPerSample=%u nSamplesPerSec=%lu nChannels=%u nAvgBytesPerSec=%lu\n",
          lpTime->wType, format->wBitsPerSample, format->wf.nSamplesPerSec,
          format->wf.nChannels, format->wf.nAvgBytesPerSec);
    TRACE("Position in bytes=%lu\n", position);

    switch (lpTime->wType) {
    case TIME_SAMPLES:
        lpTime->u.sample = position / (format->wBitsPerSample / 8 * format->wf.nChannels);
        TRACE("TIME_SAMPLES=%lu\n", lpTime->u.sample);
        break;
    case TIME_MS:
        lpTime->u.ms = 1000.0 * position / (format->wBitsPerSample / 8 * format->wf.nChannels * format->wf.nSamplesPerSec);
        TRACE("TIME_MS=%lu\n", lpTime->u.ms);
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
        TRACE("TIME_BYTES=%lu\n", lpTime->u.cb);
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
    pthread_mutexattr_t mutexattr;
    int i;
    HANDLE hThread;

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
    
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);

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

        pthread_mutex_init(&WOutDev[i].lock, &mutexattr); /* initialize the mutex */
    }

    pthread_mutexattr_destroy(&mutexattr);
    
    /* create mach messages handler */
    srandomdev();
    MessageThreadPortName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
        CFSTR("WaveMessagePort.%d.%lu"), getpid(), (unsigned long)random());
    if (!MessageThreadPortName)
    {
        ERR("Can't create message thread port name\n");
        return 1;
    }

    hThread = CreateThread(NULL, 0, messageThread, NULL, 0, NULL);
    if ( !hThread )
    {
        ERR("Can't create message thread\n");
        return 1;
    }
    
    return 0;
}

void CoreAudio_WaveRelease(void)
{
    /* Stop CFRunLoop in messageThread */
    CFMessagePortRef messagePort;
    int i;

    TRACE("()\n");

    messagePort = CFMessagePortCreateRemote(kCFAllocatorDefault, MessageThreadPortName);
    CFMessagePortSendRequest(messagePort, kStopLoopMessage, NULL, 0.0, 0.0, NULL, NULL);
    CFRelease(messagePort);

    for (i = 0; i < MAX_WAVEOUTDRV; ++i)
    {
        pthread_mutex_destroy(&WOutDev[i].lock);
    }
}

/*======================================================================*
*                  Low level WAVE OUT implementation			*
*======================================================================*/

/**************************************************************************
* 			wodNotifyClient			[internal]
*   Call from AudioUnit IO thread can't use Wine debug channels.
*/
static DWORD wodNotifyClient(WINE_WAVEOUT* wwo, WORD wMsg, DWORD dwParam1, DWORD dwParam2)
{
    switch (wMsg) {
        case WOM_OPEN:
        case WOM_CLOSE:
            if (wwo->wFlags != DCB_NULL &&
                !DriverCallback(wwo->waveDesc.dwCallback, wwo->wFlags,
                                (HDRVR)wwo->waveDesc.hWave, wMsg, wwo->waveDesc.dwInstance,
                                dwParam1, dwParam2))
            {
                return MMSYSERR_ERROR;
            }
            break;
        case WOM_DONE:
            if (wwo->wFlags != DCB_NULL &&
                ! wodSendDriverCallbackMessage(wwo, wMsg, dwParam1, dwParam2))
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
    TRACE("(%u, %p, %lu);\n", wDevID, lpCaps, dwSize);
    
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        TRACE("MAX_WAVOUTDRV reached !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    TRACE("dwSupport=(0x%lx), dwFormats=(0x%lx)\n", WOutDev[wDevID].caps.dwSupport, WOutDev[wDevID].caps.dwFormats);
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

    TRACE("(%u, %p, %08lX);\n", wDevID, lpDesc, dwFlags);
    if (lpDesc == NULL)
    {
        WARN("Invalid Parameter !\n");
        return MMSYSERR_INVALPARAM;
    }
    if (wDevID >= MAX_WAVEOUTDRV) {
        TRACE("MAX_WAVOUTDRV reached !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    TRACE("Format: tag=%04X nChannels=%d nSamplesPerSec=%ld wBitsPerSample=%d !\n",
          lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
          lpDesc->lpFormat->nSamplesPerSec, lpDesc->lpFormat->wBitsPerSample);
    
    if (lpDesc->lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
        lpDesc->lpFormat->nChannels == 0 ||
        lpDesc->lpFormat->nSamplesPerSec == 0
         )
    {
        WARN("Bad format: tag=%04X nChannels=%d nSamplesPerSec=%ld wBitsPerSample=%d !\n",
             lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
             lpDesc->lpFormat->nSamplesPerSec, lpDesc->lpFormat->wBitsPerSample);
        return WAVERR_BADFORMAT;
    }
    
    if (dwFlags & WAVE_FORMAT_QUERY)
    {
        TRACE("Query format: tag=%04X nChannels=%d nSamplesPerSec=%ld !\n",
              lpDesc->lpFormat->wFormatTag, lpDesc->lpFormat->nChannels,
              lpDesc->lpFormat->nSamplesPerSec);
        return MMSYSERR_NOERROR;
    }
    
    wwo = &WOutDev[wDevID];
    pthread_mutex_lock(&wwo->lock);

    if (wwo->state != WINE_WS_CLOSED)
    {
        pthread_mutex_unlock(&wwo->lock);
        return MMSYSERR_ALLOCATED;
    }

    if (!AudioUnit_CreateDefaultAudioUnit((void *) wwo, &wwo->audioUnit))
    {
        ERR("CoreAudio_CreateDefaultAudioUnit(%p) failed\n", wwo);
        pthread_mutex_unlock(&wwo->lock);
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
        pthread_mutex_unlock(&wwo->lock);
        return WAVERR_BADFORMAT; /* FIXME return an error based on the OSStatus */
    }
    wwo->streamDescription = streamFormat;
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
        
    pthread_mutex_unlock(&wwo->lock);
    
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
    pthread_mutex_lock(&wwo->lock);
    if (wwo->lpQueuePtr)
    {
        WARN("buffers still playing !\n");
        pthread_mutex_unlock(&wwo->lock);
        ret = WAVERR_STILLPLAYING;
    } else
    {
        OSStatus err;
        /* sanity check: this should not happen since the device must have been reset before */
        if (wwo->lpQueuePtr || wwo->lpPlayPtr) ERR("out of sync\n");
        
        wwo->state = WINE_WS_CLOSED; /* mark the device as closed */
        
        err = AudioUnitUninitialize(wwo->audioUnit);
        if (err) {
            ERR("AudioUnitUninitialize return %c%c%c%c\n", (char) (err >> 24),
                                                            (char) (err >> 16),
                                                            (char) (err >> 8),
                                                            (char) err);
            pthread_mutex_unlock(&wwo->lock);
            return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
        }
        
        if ( !AudioUnit_CloseAudioUnit(wwo->audioUnit) )
        {
            ERR("Can't close AudioUnit\n");
            pthread_mutex_unlock(&wwo->lock);
            return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
        }  
        pthread_mutex_unlock(&wwo->lock);
        
        ret = wodNotifyClient(wwo, WOM_CLOSE, 0L, 0L);
    }
    
    return ret;
}

/**************************************************************************
* 				wodPrepare			[internal]
*/
static DWORD wodPrepare(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
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
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
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
* 				wodHelper_BeginWaveHdr          [internal]
*
* Makes the specified lpWaveHdr the currently playing wave header.
* If the specified wave header is a begin loop and we're not already in
* a loop, setup the loop.
* Call from AudioUnit IO thread can't use Wine debug channels.
*/
static void wodHelper_BeginWaveHdr(WINE_WAVEOUT* wwo, LPWAVEHDR lpWaveHdr)
{
    OSStatus status;

    wwo->lpPlayPtr = lpWaveHdr;
    
    if (!lpWaveHdr)
    {
        if (wwo->state == WINE_WS_PLAYING)
        {
            wwo->state = WINE_WS_STOPPED;
            status = AudioOutputUnitStop(wwo->audioUnit);
            if (status)
                fprintf(stderr, "err:winecoreaudio:wodHelper_BeginWaveHdr AudioOutputUnitStop return %c%c%c%c\n",
                        (char) (status >> 24), (char) (status >> 16), (char) (status >> 8), (char) status);
        }
        return;
    }

    if (wwo->state == WINE_WS_STOPPED)
    {
        status = AudioOutputUnitStart(wwo->audioUnit);
        if (status) {
            fprintf(stderr, "err:winecoreaudio:AudioOutputUnitStart return %c%c%c%c\n",
                    (char) (status >> 24), (char) (status >> 16), (char) (status >> 8), (char) status);
        }
        else wwo->state = WINE_WS_PLAYING;
    }
    
    if (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)
    {
        if (wwo->lpLoopPtr)
        {
            fprintf(stderr, "trace:winecoreaudio:wodHelper_BeginWaveHdr Already in a loop. Discarding loop on this header (%p)\n", lpWaveHdr);
        } else
        {
            fprintf(stderr, "trace:winecoreaudio:wodHelper_BeginWaveHdr Starting loop (%ldx) with %p\n", lpWaveHdr->dwLoops, lpWaveHdr);

            wwo->lpLoopPtr = lpWaveHdr;
            /* Windows does not touch WAVEHDR.dwLoops,
                * so we need to make an internal copy */
            wwo->dwLoops = lpWaveHdr->dwLoops;
        }
    }
    wwo->dwPartialOffset = 0;
}


/**************************************************************************
* 				wodHelper_PlayPtrNext	        [internal]
*
* Advance the play pointer to the next waveheader, looping if required.
* Call from AudioUnit IO thread can't use Wine debug channels.
*/
static LPWAVEHDR wodHelper_PlayPtrNext(WINE_WAVEOUT* wwo)
{
    LPWAVEHDR lpWaveHdr;

    pthread_mutex_lock(&wwo->lock);
    
    lpWaveHdr = wwo->lpPlayPtr;
    if (!lpWaveHdr)
    {
        pthread_mutex_unlock(&wwo->lock);
        return NULL;
    }

    wwo->dwPartialOffset = 0;
    if ((lpWaveHdr->dwFlags & WHDR_ENDLOOP) && wwo->lpLoopPtr)
    {
        /* We're at the end of a loop, loop if required */
        if (wwo->dwLoops > 1)
        {
            wwo->dwLoops--;
            wwo->lpPlayPtr = wwo->lpLoopPtr;
        } else
        {
            /* Handle overlapping loops correctly */
            if (wwo->lpLoopPtr != lpWaveHdr && (lpWaveHdr->dwFlags & WHDR_BEGINLOOP)) {
                /* shall we consider the END flag for the closing loop or for
                * the opening one or for both ???
                * code assumes for closing loop only
                */
            } else
            {
                lpWaveHdr = lpWaveHdr->lpNext;
            }
            wwo->lpLoopPtr = NULL;
            wodHelper_BeginWaveHdr(wwo, lpWaveHdr);
        }
    } else
    {
        /* We're not in a loop.  Advance to the next wave header */
        wodHelper_BeginWaveHdr(wwo, lpWaveHdr = lpWaveHdr->lpNext);
    }
    
    pthread_mutex_unlock(&wwo->lock);
    
    return lpWaveHdr;
}

/* if force is TRUE then notify the client that all the headers were completed
 * Call from AudioUnit IO thread can't use Wine debug channels.
 */
static DWORD wodHelper_NotifyCompletions(WINE_WAVEOUT* wwo, BOOL force)
{
    LPWAVEHDR		lpWaveHdr;
    DWORD retval;

    pthread_mutex_lock(&wwo->lock);

    /* Start from lpQueuePtr and keep notifying until:
        * - we hit an unwritten wavehdr
        * - we hit the beginning of a running loop
        * - we hit a wavehdr which hasn't finished playing
        */
    while ((lpWaveHdr = wwo->lpQueuePtr) &&
           (force || 
            (lpWaveHdr != wwo->lpPlayPtr &&
             lpWaveHdr != wwo->lpLoopPtr)))
    {
        wwo->lpQueuePtr = lpWaveHdr->lpNext;
        
        lpWaveHdr->dwFlags &= ~WHDR_INQUEUE;
        lpWaveHdr->dwFlags |= WHDR_DONE;
        
        wodNotifyClient(wwo, WOM_DONE, (DWORD)lpWaveHdr, 0);
    }

    retval = (lpWaveHdr && lpWaveHdr != wwo->lpPlayPtr && lpWaveHdr != 
              wwo->lpLoopPtr) ? 0 : INFINITE;
    
    pthread_mutex_unlock(&wwo->lock);
    
    return retval;
}

/**************************************************************************
* 				wodHelper_Reset			[internal]
*
* Resets current output stream.
*/
static  DWORD  wodHelper_Reset(WINE_WAVEOUT* wwo, BOOL reset)
{
    OSStatus status;

    FIXME("\n");
   
    /* updates current notify list */
    /* if resetting, remove all wave headers and notify client that all headers were completed */
    wodHelper_NotifyCompletions(wwo, reset);
    
    pthread_mutex_lock(&wwo->lock);
    
    if (reset)
    {
        wwo->lpPlayPtr = wwo->lpQueuePtr = wwo->lpLoopPtr = NULL;
        wwo->state = WINE_WS_STOPPED;
        wwo->dwPlayedTotal = wwo->dwWrittenTotal = 0;
        
        wwo->dwPartialOffset = 0;        /* Clear partial wavehdr */
    } 
    else
    {
        if (wwo->lpLoopPtr)
        {
            /* complicated case, not handled yet (could imply modifying the loop counter) */
            FIXME("Pausing while in loop isn't correctly handled yet, except strange results\n");
            wwo->lpPlayPtr = wwo->lpLoopPtr;
            wwo->dwPartialOffset = 0;
            wwo->dwWrittenTotal = wwo->dwPlayedTotal; /* this is wrong !!! */
        } else
        {
            LPWAVEHDR   ptr;
            DWORD       sz = wwo->dwPartialOffset;
            
            /* reset all the data as if we had written only up to lpPlayedTotal bytes */
            /* compute the max size playable from lpQueuePtr */
            for (ptr = wwo->lpQueuePtr; ptr && ptr != wwo->lpPlayPtr; ptr = ptr->lpNext)
            {
                sz += ptr->dwBufferLength;
            }
            
            /* because the reset lpPlayPtr will be lpQueuePtr */
            if (wwo->dwWrittenTotal > wwo->dwPlayedTotal + sz) ERR("doh\n");
            wwo->dwPartialOffset = sz - (wwo->dwWrittenTotal - wwo->dwPlayedTotal);
            wwo->dwWrittenTotal = wwo->dwPlayedTotal;
            wwo->lpPlayPtr = wwo->lpQueuePtr;
        }
        
        wwo->state = WINE_WS_PAUSED;
    }

    status = AudioOutputUnitStop(wwo->audioUnit);

    pthread_mutex_unlock(&wwo->lock);

    if (status) {
        ERR( "AudioOutputUnitStop return %c%c%c%c\n",
             (char) (status >> 24), (char) (status >> 16), (char) (status >> 8), (char) status);
        return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
    }

    return MMSYSERR_NOERROR;
}


/**************************************************************************
* 				wodWrite			[internal]
* 
*/
static DWORD wodWrite(WORD wDevID, LPWAVEHDR lpWaveHdr, DWORD dwSize)
{
    LPWAVEHDR*wh;
    WINE_WAVEOUT *wwo;
    
    TRACE("(%u, %p, %08lX);\n", wDevID, lpWaveHdr, dwSize);
    
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

    pthread_mutex_lock(&wwo->lock);
    /* insert buffer at the end of queue */
    for (wh = &(wwo->lpQueuePtr); *wh; wh = &((*wh)->lpNext))
        /* Do nothing */;
    *wh = lpWaveHdr;
    
    if (!wwo->lpPlayPtr)
        wodHelper_BeginWaveHdr(wwo,lpWaveHdr);
    pthread_mutex_unlock(&wwo->lock);
    
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
    
    pthread_mutex_lock(&WOutDev[wDevID].lock);
    if (WOutDev[wDevID].state == WINE_WS_PLAYING || WOutDev[wDevID].state == WINE_WS_STOPPED)
    {
        WOutDev[wDevID].state = WINE_WS_PAUSED;
        status = AudioOutputUnitStop(WOutDev[wDevID].audioUnit);
        if (status) {
            ERR( "AudioOutputUnitStop return %c%c%c%c\n",
                 (char) (status >> 24), (char) (status >> 16), (char) (status >> 8), (char) status);
            pthread_mutex_unlock(&WOutDev[wDevID].lock);
            return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
        }
    }
    pthread_mutex_unlock(&WOutDev[wDevID].lock);
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 			wodRestart				[internal]
*/
static DWORD wodRestart(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    
    if (wDevID >= MAX_WAVEOUTDRV )
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    pthread_mutex_lock(&WOutDev[wDevID].lock);
    if (WOutDev[wDevID].state == WINE_WS_PAUSED)
    {
        if (WOutDev[wDevID].lpPlayPtr)
        {
            OSStatus status = AudioOutputUnitStart(WOutDev[wDevID].audioUnit);
            if (status) {
                ERR("AudioOutputUnitStart return %c%c%c%c\n",
                    (char) (status >> 24), (char) (status >> 16), (char) (status >> 8), (char) status);
                pthread_mutex_unlock(&WOutDev[wDevID].lock);
                return MMSYSERR_ERROR; /* FIXME return an error based on the OSStatus */
            }
            WOutDev[wDevID].state = WINE_WS_PLAYING;
        }
        else
            WOutDev[wDevID].state = WINE_WS_STOPPED;
    }
    pthread_mutex_unlock(&WOutDev[wDevID].lock);
    
    return MMSYSERR_NOERROR;
}

/**************************************************************************
* 			wodReset				[internal]
*/
static DWORD wodReset(WORD wDevID)
{
    TRACE("(%u);\n", wDevID);
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    return wodHelper_Reset(&WOutDev[wDevID], TRUE);
}

/**************************************************************************
* 				wodGetPosition			[internal]
*/
static DWORD wodGetPosition(WORD wDevID, LPMMTIME lpTime, DWORD uSize)
{
    DWORD		val;
    WINE_WAVEOUT*	wwo;

    TRACE("(%u, %p, %lu);\n", wDevID, lpTime, uSize);
    
    if (wDevID >= MAX_WAVEOUTDRV)
    {
        WARN("bad device ID !\n");
        return MMSYSERR_BADDEVICEID;
    }
    
    /* if null pointer to time structure return error */
    if (lpTime == NULL)	return MMSYSERR_INVALPARAM;
    
    wwo = &WOutDev[wDevID];
    
    pthread_mutex_lock(&WOutDev[wDevID].lock);
    val = wwo->dwPlayedTotal;
    pthread_mutex_unlock(&WOutDev[wDevID].lock);
    
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
    
    pthread_mutex_lock(&WOutDev[wDevID].lock);
    
    AudioUnit_GetVolume(WOutDev[wDevID].audioUnit, &left, &right); 
        
    pthread_mutex_unlock(&WOutDev[wDevID].lock);
        
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
    
    TRACE("(%u, %08lX);\n", wDevID, dwParam);
    
    pthread_mutex_lock(&WOutDev[wDevID].lock);
    
    AudioUnit_SetVolume(WOutDev[wDevID].audioUnit, left, right); 
        
    pthread_mutex_unlock(&WOutDev[wDevID].lock);
    
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
* 				wodMessage (WINEJACK.7)
*/
DWORD WINAPI CoreAudio_wodMessage(UINT wDevID, UINT wMsg, DWORD dwUser, 
                                  DWORD dwParam1, DWORD dwParam2)
{
    TRACE("(%u, %s, %08lX, %08lX, %08lX);\n",
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
        case DRV_QUERYDSOUNDIFACE:	
        case DRV_QUERYDSOUNDDESC:	
            return MMSYSERR_NOTSUPPORTED;
            
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
    int nextPtr = 0;
    int needNotify = 0;

    unsigned int dataNeeded = ioData->mBuffers[0].mDataByteSize;
    unsigned int dataProvided = 0;

    while (dataNeeded > 0)
    {
        pthread_mutex_lock(&wwo->lock);

        if (wwo->state == WINE_WS_PLAYING && wwo->lpPlayPtr)
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
                nextPtr = 1;

            needNotify = 1;
        }
        else
        {
            memset((char*)ioData->mBuffers[0].mData + dataProvided, 0, dataNeeded);
            dataProvided += dataNeeded;
            dataNeeded = 0;
        }

        pthread_mutex_unlock(&wwo->lock);

        if (nextPtr)
        {
            wodHelper_PlayPtrNext(wwo);
            nextPtr = 0;
        }
    }

    /* We only fill buffer 0.  Set any others that might be requested to 0. */
    for (buffer = 1; buffer < ioData->mNumberBuffers; buffer++)
    {
        memset(ioData->mBuffers[buffer].mData, 0, ioData->mBuffers[buffer].mDataByteSize);
    }

    if (needNotify) wodHelper_NotifyCompletions(wwo, FALSE);
    return noErr;
}
#else

/**************************************************************************
* 				wodMessage (WINECOREAUDIO.7)
*/
DWORD WINAPI CoreAudio_wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
                                  DWORD dwParam1, DWORD dwParam2)
{
    FIXME("(%u, %04X, %08lX, %08lX, %08lX): CoreAudio support not compiled into wine\n", wDevID, wMsg, dwUser, dwParam1, dwParam2);
    return MMSYSERR_NOTENABLED;
}

#endif
