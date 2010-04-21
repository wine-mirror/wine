/*
 * Copyright 2009 Maarten Lankhorst
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

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

extern HRESULT MMDevEnum_Create(REFIID riid, void **ppv);
extern void MMDevEnum_Free(void);

extern HRESULT MMDevice_GetPropValue(const GUID *devguid, DWORD flow, REFPROPERTYKEY key, PROPVARIANT *pv);
extern HRESULT MMDevice_SetPropValue(const GUID *devguid, DWORD flow, REFPROPERTYKEY key, REFPROPVARIANT pv);

typedef struct MMDevice {
    const IMMDeviceVtbl *lpVtbl;
    const IMMEndpointVtbl *lpEndpointVtbl;
    LONG ref;

    CRITICAL_SECTION crst;

    EDataFlow flow;
    DWORD state;
    GUID devguid;
    WCHAR *alname;
    void *device, *ctx;
} MMDevice;

extern HRESULT AudioClient_Create(MMDevice *parent, IAudioClient **ppv);
extern HRESULT AudioEndpointVolume_Create(MMDevice *parent, IAudioEndpointVolume **ppv);

#ifdef HAVE_OPENAL

#include "alext.h"

/* All openal functions */
extern int openal_loaded;
#ifdef SONAME_LIBOPENAL
extern LPALCCREATECONTEXT palcCreateContext;
extern LPALCMAKECONTEXTCURRENT palcMakeContextCurrent;
extern LPALCPROCESSCONTEXT palcProcessContext;
extern LPALCSUSPENDCONTEXT palcSuspendContext;
extern LPALCDESTROYCONTEXT palcDestroyContext;
extern LPALCGETCURRENTCONTEXT palcGetCurrentContext;
extern LPALCGETCONTEXTSDEVICE palcGetContextsDevice;
extern LPALCOPENDEVICE palcOpenDevice;
extern LPALCCLOSEDEVICE palcCloseDevice;
extern LPALCGETERROR palcGetError;
extern LPALCISEXTENSIONPRESENT palcIsExtensionPresent;
extern LPALCGETPROCADDRESS palcGetProcAddress;
extern LPALCGETENUMVALUE palcGetEnumValue;
extern LPALCGETSTRING palcGetString;
extern LPALCGETINTEGERV palcGetIntegerv;
extern LPALCCAPTUREOPENDEVICE palcCaptureOpenDevice;
extern LPALCCAPTURECLOSEDEVICE palcCaptureCloseDevice;
extern LPALCCAPTURESTART palcCaptureStart;
extern LPALCCAPTURESTOP palcCaptureStop;
extern LPALCCAPTURESAMPLES palcCaptureSamples;
extern LPALENABLE palEnable;
extern LPALDISABLE palDisable;
extern LPALISENABLED palIsEnabled;
extern LPALGETSTRING palGetString;
extern LPALGETBOOLEANV palGetBooleanv;
extern LPALGETINTEGERV palGetIntegerv;
extern LPALGETFLOATV palGetFloatv;
extern LPALGETDOUBLEV palGetDoublev;
extern LPALGETBOOLEAN palGetBoolean;
extern LPALGETINTEGER palGetInteger;
extern LPALGETFLOAT palGetFloat;
extern LPALGETDOUBLE palGetDouble;
extern LPALGETERROR palGetError;
extern LPALISEXTENSIONPRESENT palIsExtensionPresent;
extern LPALGETPROCADDRESS palGetProcAddress;
extern LPALGETENUMVALUE palGetEnumValue;
extern LPALLISTENERF palListenerf;
extern LPALLISTENER3F palListener3f;
extern LPALLISTENERFV palListenerfv;
extern LPALLISTENERI palListeneri;
extern LPALLISTENER3I palListener3i;
extern LPALLISTENERIV palListeneriv;
extern LPALGETLISTENERF palGetListenerf;
extern LPALGETLISTENER3F palGetListener3f;
extern LPALGETLISTENERFV palGetListenerfv;
extern LPALGETLISTENERI palGetListeneri;
extern LPALGETLISTENER3I palGetListener3i;
extern LPALGETLISTENERIV palGetListeneriv;
extern LPALGENSOURCES palGenSources;
extern LPALDELETESOURCES palDeleteSources;
extern LPALISSOURCE palIsSource;
extern LPALSOURCEF palSourcef;
extern LPALSOURCE3F palSource3f;
extern LPALSOURCEFV palSourcefv;
extern LPALSOURCEI palSourcei;
extern LPALSOURCE3I palSource3i;
extern LPALSOURCEIV palSourceiv;
extern LPALGETSOURCEF palGetSourcef;
extern LPALGETSOURCE3F palGetSource3f;
extern LPALGETSOURCEFV palGetSourcefv;
extern LPALGETSOURCEI palGetSourcei;
extern LPALGETSOURCE3I palGetSource3i;
extern LPALGETSOURCEIV palGetSourceiv;
extern LPALSOURCEPLAYV palSourcePlayv;
extern LPALSOURCESTOPV palSourceStopv;
extern LPALSOURCEREWINDV palSourceRewindv;
extern LPALSOURCEPAUSEV palSourcePausev;
extern LPALSOURCEPLAY palSourcePlay;
extern LPALSOURCESTOP palSourceStop;
extern LPALSOURCEREWIND palSourceRewind;
extern LPALSOURCEPAUSE palSourcePause;
extern LPALSOURCEQUEUEBUFFERS palSourceQueueBuffers;
extern LPALSOURCEUNQUEUEBUFFERS palSourceUnqueueBuffers;
extern LPALGENBUFFERS palGenBuffers;
extern LPALDELETEBUFFERS palDeleteBuffers;
extern LPALISBUFFER palIsBuffer;
extern LPALBUFFERF palBufferf;
extern LPALBUFFER3F palBuffer3f;
extern LPALBUFFERFV palBufferfv;
extern LPALBUFFERI palBufferi;
extern LPALBUFFER3I palBuffer3i;
extern LPALBUFFERIV palBufferiv;
extern LPALGETBUFFERF palGetBufferf;
extern LPALGETBUFFER3F palGetBuffer3f;
extern LPALGETBUFFERFV palGetBufferfv;
extern LPALGETBUFFERI palGetBufferi;
extern LPALGETBUFFER3I palGetBuffer3i;
extern LPALGETBUFFERIV palGetBufferiv;
extern LPALBUFFERDATA palBufferData;
extern LPALDOPPLERFACTOR palDopplerFactor;
extern LPALDOPPLERVELOCITY palDopplerVelocity;
extern LPALDISTANCEMODEL palDistanceModel;
extern LPALSPEEDOFSOUND palSpeedOfSound;
#else
#define palcCreateContext alcCreateContext
#define palcMakeContextCurrent alcMakeContextCurrent
#define palcProcessContext alcProcessContext
#define palcSuspendContext alcSuspendContext
#define palcDestroyContext alcDestroyContext
#define palcGetCurrentContext alcGetCurrentContext
#define palcGetContextsDevice alcGetContextsDevice
#define palcOpenDevice alcOpenDevice
#define palcCloseDevice alcCloseDevice
#define palcGetError alcGetError
#define palcIsExtensionPresent alcIsExtensionPresent
#define palcGetProcAddress alcGetProcAddress
#define palcGetEnumValue alcGetEnumValue
#define palcGetString alcGetString
#define palcGetIntegerv alcGetIntegerv
#define palcCaptureOpenDevice alcCaptureOpenDevice
#define palcCaptureCloseDevice alcCaptureCloseDevice
#define palcCaptureStart alcCaptureStart
#define palcCaptureStop alcCaptureStop
#define palcCaptureSamples alcCaptureSamples
#define palEnable alEnable
#define palDisable alDisable
#define palIsEnabled alIsEnabled
#define palGetString alGetString
#define palGetBooleanv alGetBooleanv
#define palGetIntegerv alGetIntegerv
#define palGetFloatv alGetFloatv
#define palGetDoublev alGetDoublev
#define palGetBoolean alGetBoolean
#define palGetInteger alGetInteger
#define palGetFloat alGetFloat
#define palGetDouble alGetDouble
#define palGetError alGetError
#define palIsExtensionPresent alIsExtensionPresent
#define palGetProcAddress alGetProcAddress
#define palGetEnumValue alGetEnumValue
#define palListenerf alListenerf
#define palListener3f alListener3f
#define palListenerfv alListenerfv
#define palListeneri alListeneri
#define palListener3i alListener3i
#define palListeneriv alListeneriv
#define palGetListenerf alGetListenerf
#define palGetListener3f alGetListener3f
#define palGetListenerfv alGetListenerfv
#define palGetListeneri alGetListeneri
#define palGetListener3i alGetListener3i
#define palGetListeneriv alGetListeneriv
#define palGenSources alGenSources
#define palDeleteSources alDeleteSources
#define palIsSource alIsSource
#define palSourcef alSourcef
#define palSource3f alSource3f
#define palSourcefv alSourcefv
#define palSourcei alSourcei
#define palSource3i alSource3i
#define palSourceiv alSourceiv
#define palGetSourcef alGetSourcef
#define palGetSource3f alGetSource3f
#define palGetSourcefv alGetSourcefv
#define palGetSourcei alGetSourcei
#define palGetSource3i alGetSource3i
#define palGetSourceiv alGetSourceiv
#define palSourcePlayv alSourcePlayv
#define palSourceStopv alSourceStopv
#define palSourceRewindv alSourceRewindv
#define palSourcePausev alSourcePausev
#define palSourcePlay alSourcePlay
#define palSourceStop alSourceStop
#define palSourceRewind alSourceRewind
#define palSourcePause alSourcePause
#define palSourceQueueBuffers alSourceQueueBuffers
#define palSourceUnqueueBuffers alSourceUnqueueBuffers
#define palGenBuffers alGenBuffers
#define palDeleteBuffers alDeleteBuffers
#define palIsBuffer alIsBuffer
#define palBufferf alBufferf
#define palBuffer3f alBuffer3f
#define palBufferfv alBufferfv
#define palBufferi alBufferi
#define palBuffer3i alBuffer3i
#define palBufferiv alBufferiv
#define palGetBufferf alGetBufferf
#define palGetBuffer3f alGetBuffer3f
#define palGetBufferfv alGetBufferfv
#define palGetBufferi alGetBufferi
#define palGetBuffer3i alGetBuffer3i
#define palGetBufferiv alGetBufferiv
#define palBufferData alBufferData
#define palDopplerFactor alDopplerFactor
#define palDopplerVelocity alDopplerVelocity
#define palDistanceModel alDistanceModel
#define palSpeedOfSound alSpeedOfSound
#endif

/* OpenAL only allows for 1 single access to the device at the same time */
extern CRITICAL_SECTION openal_crst;
extern int local_contexts;
extern typeof(alcGetCurrentContext) *get_context;
extern typeof(alcMakeContextCurrent) *set_context;

#define getALError() \
do { \
    ALenum err = palGetError(); \
    if(err != AL_NO_ERROR) \
    { \
        ERR(">>>>>>>>>>>> Received AL error %#x on context %p, %s:%u\n", err, get_context(), __FUNCTION__, __LINE__); \
    } \
} while (0)

#define getALCError(dev) \
do { \
    ALenum err = palcGetError(dev); \
    if(err != ALC_NO_ERROR) \
    { \
        ERR(">>>>>>>>>>>> Received ALC error %#x on device %p, %s:%u\n", err, dev, __FUNCTION__, __LINE__); \
    } \
} while(0)

#define setALContext(actx) \
    do { \
        ALCcontext *__old_ctx, *cur_ctx = actx ; \
        if (!local_contexts) EnterCriticalSection(&openal_crst); \
        __old_ctx = get_context(); \
        if (__old_ctx != cur_ctx && set_context(cur_ctx) == ALC_FALSE) {\
            ERR("Couldn't set current context!!\n"); \
            getALCError(palcGetContextsDevice(cur_ctx)); \
        }

/* Only restore a NULL context if using global contexts, for TLS contexts always restore */
#define popALContext() \
        if (__old_ctx != cur_ctx \
            && (local_contexts || __old_ctx) \
            && set_context(__old_ctx) == ALC_FALSE) { \
            ERR("Couldn't restore old context!!\n"); \
            getALCError(palcGetContextsDevice(__old_ctx)); \
        } \
        if (!local_contexts) LeaveCriticalSection(&openal_crst); \
    } while (0)

#endif
