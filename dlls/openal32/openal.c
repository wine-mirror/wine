/*
 * OpenAL32.dll thunk. Wraps Win32 OpenAL function calls around a native
 * implementation.
 *
 * Copyright 2007 Nick Burns (adger44@hotmail.com)
 * Copyright 2007,2009 Chris Robinson
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

#include <stdarg.h>
#include <string.h>

#ifdef HAVE_AL_AL_H
#include <AL/al.h>
#include <AL/alc.h>
#elif defined(HAVE_OPENAL_AL_H)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(openal32);

static ALCboolean  (ALC_APIENTRY*alcSetThreadContext)(ALCcontext *context);
static ALCcontext* (ALC_APIENTRY*alcGetThreadContext)(ALCvoid);

static ALboolean loaded_procs;

static ALvoid (AL_APIENTRY*alBufferDataStatic)(const ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq);
static ALvoid (AL_APIENTRY*alGenFilters)(ALsizei n, ALuint* filters);
static ALvoid (AL_APIENTRY*alDeleteFilters)(ALsizei n, const ALuint* filters);
static ALboolean (AL_APIENTRY*alIsFilter)(ALuint fid);
static ALvoid (AL_APIENTRY*alFilterf)(ALuint fid, ALenum param, ALfloat value);
static ALvoid (AL_APIENTRY*alFilterfv)(ALuint fid, ALenum param, const ALfloat* values);
static ALvoid (AL_APIENTRY*alFilteri)(ALuint fid, ALenum param, ALint value);
static ALvoid (AL_APIENTRY*alFilteriv)(ALuint fid, ALenum param, const ALint* values);
static ALvoid (AL_APIENTRY*alGetFilterf)(ALuint fid, ALenum param, ALfloat* value);
static ALvoid (AL_APIENTRY*alGetFilterfv)(ALuint fid, ALenum param, ALfloat* values);
static ALvoid (AL_APIENTRY*alGetFilteri)(ALuint fid, ALenum param, ALint* value);
static ALvoid (AL_APIENTRY*alGetFilteriv)(ALuint fid, ALenum param, ALint* values);
static ALvoid (AL_APIENTRY*alGenEffects)(ALsizei n, ALuint* effects);
static ALvoid (AL_APIENTRY*alDeleteEffects)(ALsizei n, const ALuint* effects);
static ALboolean (AL_APIENTRY*alIsEffect)(ALuint eid);
static ALvoid (AL_APIENTRY*alEffectf)(ALuint eid, ALenum param, ALfloat value);
static ALvoid (AL_APIENTRY*alEffectfv)(ALuint eid, ALenum param, const ALfloat* values);
static ALvoid (AL_APIENTRY*alEffecti)(ALuint eid, ALenum param, ALint value);
static ALvoid (AL_APIENTRY*alEffectiv)(ALuint eid, ALenum param, const ALint* values);
static ALvoid (AL_APIENTRY*alGetEffectf)(ALuint eid, ALenum param, ALfloat* value);
static ALvoid (AL_APIENTRY*alGetEffectfv)(ALuint eid, ALenum param, ALfloat* values);
static ALvoid (AL_APIENTRY*alGetEffecti)(ALuint eid, ALenum param, ALint* value);
static ALvoid (AL_APIENTRY*alGetEffectiv)(ALuint eid, ALenum param, ALint* values);
static ALvoid (AL_APIENTRY*alGenAuxiliaryEffectSlots)(ALsizei n, ALuint* slots);
static ALvoid (AL_APIENTRY*alDeleteAuxiliaryEffectSlots)(ALsizei n, const ALuint* slots);
static ALboolean (AL_APIENTRY*alIsAuxiliaryEffectSlot)(ALuint sid);
static ALvoid (AL_APIENTRY*alAuxiliaryEffectSlotf)(ALuint sid, ALenum param, ALfloat value);
static ALvoid (AL_APIENTRY*alAuxiliaryEffectSlotfv)(ALuint sid, ALenum param, const ALfloat* values);
static ALvoid (AL_APIENTRY*alAuxiliaryEffectSloti)(ALuint sid, ALenum param, ALint value);
static ALvoid (AL_APIENTRY*alAuxiliaryEffectSlotiv)(ALuint sid, ALenum param, const ALint* values);
static ALvoid (AL_APIENTRY*alGetAuxiliaryEffectSlotf)(ALuint sid, ALenum param, ALfloat* value);
static ALvoid (AL_APIENTRY*alGetAuxiliaryEffectSlotfv)(ALuint sid, ALenum param, ALfloat* values);
static ALvoid (AL_APIENTRY*alGetAuxiliaryEffectSloti)(ALuint sid, ALenum param, ALint* value);
static ALvoid (AL_APIENTRY*alGetAuxiliaryEffectSlotiv)(ALuint sid, ALenum param, ALint* values);

extern ALCvoid* CDECL wine_alcGetProcAddress(ALCdevice *, const ALCchar *);
extern ALvoid*  CDECL wine_alGetProcAddress(const ALchar*);

static CRITICAL_SECTION openal_cs;
static CRITICAL_SECTION_DEBUG openal_cs_debug =
{
    0, 0, &openal_cs,
    {&openal_cs_debug.ProcessLocksList,
    &openal_cs_debug.ProcessLocksList},
    0, 0, {(DWORD_PTR)(__FILE__ ": openal_cs")}
};
static CRITICAL_SECTION openal_cs = {&openal_cs_debug, -1, 0, 0, 0, 0};

/***********************************************************************
 *           OpenAL initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
#define LOADFUNC(x) x = alcGetProcAddress(NULL, #x)
        LOADFUNC(alcSetThreadContext);
        LOADFUNC(alcGetThreadContext);
#undef LOADFUNC
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        DeleteCriticalSection(&openal_cs);
    }

    return TRUE;
}


static void LoadProcs(void)
{
#define LOADFUNC(x) x = alGetProcAddress(#x)
    LOADFUNC(alBufferDataStatic);
    LOADFUNC(alGenFilters);
    LOADFUNC(alDeleteFilters);
    LOADFUNC(alIsFilter);
    LOADFUNC(alFilterf);
    LOADFUNC(alFilterfv);
    LOADFUNC(alFilteri);
    LOADFUNC(alFilteriv);
    LOADFUNC(alGetFilterf);
    LOADFUNC(alGetFilterfv);
    LOADFUNC(alGetFilteri);
    LOADFUNC(alGetFilteriv);
    LOADFUNC(alGenEffects);
    LOADFUNC(alDeleteEffects);
    LOADFUNC(alIsEffect);
    LOADFUNC(alEffectf);
    LOADFUNC(alEffectfv);
    LOADFUNC(alEffecti);
    LOADFUNC(alEffectiv);
    LOADFUNC(alGetEffectf);
    LOADFUNC(alGetEffectfv);
    LOADFUNC(alGetEffecti);
    LOADFUNC(alGetEffectiv);
    LOADFUNC(alGenAuxiliaryEffectSlots);
    LOADFUNC(alDeleteAuxiliaryEffectSlots);
    LOADFUNC(alIsAuxiliaryEffectSlot);
    LOADFUNC(alAuxiliaryEffectSlotf);
    LOADFUNC(alAuxiliaryEffectSlotfv);
    LOADFUNC(alAuxiliaryEffectSloti);
    LOADFUNC(alAuxiliaryEffectSlotiv);
    LOADFUNC(alGetAuxiliaryEffectSlotf);
    LOADFUNC(alGetAuxiliaryEffectSlotfv);
    LOADFUNC(alGetAuxiliaryEffectSloti);
    LOADFUNC(alGetAuxiliaryEffectSlotiv);
#undef LOADFUNC
}

/***********************************************************************
 *           OpenAL thunk routines
 */

/* OpenAL ALC 1.0 functions */
ALCcontext* CDECL wine_alcCreateContext(ALCdevice *device, const ALCint* attrlist)
{
    return alcCreateContext(device, attrlist);
}

ALCboolean CDECL wine_alcMakeContextCurrent(ALCcontext *context)
{
    EnterCriticalSection(&openal_cs);
    if(alcMakeContextCurrent(context) == ALC_FALSE)
    {
        WARN("Failed to make context %p current\n", context);
        LeaveCriticalSection(&openal_cs);
        return ALC_FALSE;
    }

    if(context && !loaded_procs)
    {
        loaded_procs = AL_TRUE;
        LoadProcs();
    }
    LeaveCriticalSection(&openal_cs);

    return ALC_TRUE;
}

ALvoid CDECL wine_alcProcessContext(ALCcontext *context)
{
    alcProcessContext(context);
}

ALvoid CDECL wine_alcSuspendContext(ALCcontext *context)
{
    alcSuspendContext(context);
}

ALvoid CDECL wine_alcDestroyContext(ALCcontext *context)
{
    alcDestroyContext(context);
}

ALCcontext* CDECL wine_alcGetCurrentContext(ALCvoid)
{
    return alcGetCurrentContext();
}

ALCdevice* CDECL wine_alcGetContextsDevice(ALCcontext *context)
{
    return alcGetContextsDevice(context);
}

ALCdevice* CDECL wine_alcOpenDevice(const ALCchar *devicename)
{
    if(devicename != NULL && (strcmp(devicename, "DirectSound3D") == 0 ||
                              strcmp(devicename, "DirectSound") == 0 ||
                              strcmp(devicename, "Generic Hardware") == 0 ||
                              strcmp(devicename, "Generic Software") == 0)) {
        devicename = NULL;
    }
    return alcOpenDevice(devicename);
}

ALCboolean CDECL wine_alcCloseDevice(ALCdevice *device)
{
    return alcCloseDevice(device);
}

ALCenum CDECL wine_alcGetError(ALCdevice *device)
{
    return alcGetError(device);
}

ALCboolean CDECL wine_alcIsExtensionPresent(ALCdevice *device, const ALCchar *extname)
{
    return alcIsExtensionPresent(device, extname);
}

ALCenum CDECL wine_alcGetEnumValue(ALCdevice *device, const ALCchar *enumname)
{
    return alcGetEnumValue(device, enumname);
}

const ALCchar* CDECL wine_alcGetString(ALCdevice *device, ALCenum param)
{
    return alcGetString(device, param);
}

ALvoid CDECL wine_alcGetIntegerv(ALCdevice *device, ALCenum param, ALCsizei size, ALCint *dest)
{
    alcGetIntegerv(device, param, size, dest);
}


/* OpenAL 1.0 functions */
ALvoid CDECL wine_alEnable(ALenum capability)
{
    alEnable(capability);
}

ALvoid CDECL wine_alDisable(ALenum capability)
{
    alDisable(capability);
}

ALboolean CDECL wine_alIsEnabled(ALenum capability)
{
    return alIsEnabled(capability);
}

const ALchar* CDECL wine_alGetString(ALenum param)
{
    return alGetString(param);
}

ALvoid CDECL wine_alGetBooleanv(ALenum param, ALboolean* data)
{
    alGetBooleanv(param, data);
}

ALvoid CDECL wine_alGetIntegerv(ALenum param, ALint* data)
{
    alGetIntegerv(param, data);
}

ALvoid CDECL wine_alGetFloatv(ALenum param, ALfloat* data)
{
    alGetFloatv(param, data);
}

ALvoid CDECL wine_alGetDoublev(ALenum param, ALdouble* data)
{
    alGetDoublev(param, data);
}

ALboolean CDECL wine_alGetBoolean(ALenum param)
{
    return alGetBoolean(param);
}

ALint CDECL wine_alGetInteger(ALenum param)
{
    return alGetInteger(param);
}

ALfloat CDECL wine_alGetFloat(ALenum param)
{
    return alGetFloat(param);
}

ALdouble CDECL wine_alGetDouble(ALenum param)
{
    return alGetDouble(param);
}

ALenum CDECL wine_alGetError(ALvoid)
{
    return alGetError();
}

ALboolean CDECL wine_alIsExtensionPresent(const ALchar* extname)
{
    return alIsExtensionPresent(extname);
}

ALenum CDECL wine_alGetEnumValue(const ALchar* ename)
{
    return alGetEnumValue(ename);
}

ALvoid CDECL wine_alListenerf(ALenum param, ALfloat value)
{
    alListenerf(param, value);
}

ALvoid CDECL wine_alListener3f(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
    alListener3f(param, value1, value2, value3);
}

ALvoid CDECL wine_alListenerfv(ALenum param, const ALfloat* values)
{
    alListenerfv(param, values);
}

ALvoid CDECL wine_alListeneri(ALenum param, ALint value)
{
    alListeneri(param, value);
}

ALvoid CDECL wine_alGetListenerf(ALenum param, ALfloat* value)
{
    alGetListenerf(param, value);
}

ALvoid CDECL wine_alGetListener3f(ALenum param, ALfloat *value1, ALfloat *value2, ALfloat *value3)
{
    alGetListener3f(param, value1, value2, value3);
}

ALvoid CDECL wine_alGetListenerfv(ALenum param, ALfloat* values)
{
    alGetListenerfv(param, values);
}

ALvoid CDECL wine_alGetListeneri(ALenum param, ALint* value)
{
    alGetListeneri(param, value);
}

ALvoid CDECL wine_alGetListeneriv(ALenum param, ALint* values)
{
    alGetListeneriv(param, values);
}

ALvoid CDECL wine_alGenSources(ALsizei n, ALuint* sources)
{
    alGenSources(n, sources);
}

ALvoid CDECL wine_alDeleteSources(ALsizei n, const ALuint* sources)
{
    alDeleteSources(n, sources);
}

ALboolean CDECL wine_alIsSource(ALuint sid)
{
    return alIsSource(sid);
}

ALvoid CDECL wine_alSourcef(ALuint sid, ALenum param, ALfloat value)
{
    alSourcef(sid, param, value);
}

ALvoid CDECL wine_alSource3f(ALuint sid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
    alSource3f(sid, param, value1, value2, value3);
}

ALvoid CDECL wine_alSourcefv(ALuint sid, ALenum param, const ALfloat* values)
{
    alSourcefv(sid, param, values);
}

ALvoid CDECL wine_alSourcei(ALuint sid, ALenum param, ALint value)
{
    alSourcei(sid, param, value);
}

ALvoid CDECL wine_alGetSourcef(ALuint sid, ALenum param, ALfloat* value)
{
    alGetSourcef(sid, param, value);
}

ALvoid CDECL wine_alGetSource3f(ALuint sid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3)
{
    alGetSource3f(sid, param, value1, value2, value3);
}

ALvoid CDECL wine_alGetSourcefv(ALuint sid, ALenum param, ALfloat* values)
{
    alGetSourcefv(sid, param, values);
}

ALvoid CDECL wine_alGetSourcei(ALuint sid, ALenum param, ALint* value)
{
    alGetSourcei(sid, param, value);
}

ALvoid CDECL wine_alGetSourceiv(ALuint sid, ALenum param, ALint* values)
{
    alGetSourceiv(sid, param, values);
}

ALvoid CDECL wine_alSourcePlayv(ALsizei ns, const ALuint *sids)
{
    alSourcePlayv(ns, sids);
}

ALvoid CDECL wine_alSourceStopv(ALsizei ns, const ALuint *sids)
{
    alSourceStopv(ns, sids);
}

ALvoid CDECL wine_alSourceRewindv(ALsizei ns, const ALuint *sids)
{
    alSourceRewindv(ns, sids);
}

ALvoid CDECL wine_alSourcePausev(ALsizei ns, const ALuint *sids)
{
    alSourcePausev(ns, sids);
}

ALvoid CDECL wine_alSourcePlay(ALuint sid)
{
    alSourcePlay(sid);
}

ALvoid CDECL wine_alSourceStop(ALuint sid)
{
    alSourceStop(sid);
}

ALvoid CDECL wine_alSourceRewind(ALuint sid)
{
    alSourceRewind(sid);
}

ALvoid CDECL wine_alSourcePause(ALuint sid)
{
    alSourcePause(sid);
}

ALvoid CDECL wine_alSourceQueueBuffers(ALuint sid, ALsizei numEntries, const ALuint *bids)
{
    alSourceQueueBuffers(sid, numEntries, bids);
}

ALvoid CDECL wine_alSourceUnqueueBuffers(ALuint sid, ALsizei numEntries, ALuint *bids)
{
    alSourceUnqueueBuffers(sid, numEntries, bids);
}

ALvoid CDECL wine_alGenBuffers(ALsizei n, ALuint* buffers)
{
    alGenBuffers(n, buffers);
}

ALvoid CDECL wine_alDeleteBuffers(ALsizei n, const ALuint* buffers)
{
    alDeleteBuffers(n, buffers);
}

ALboolean CDECL wine_alIsBuffer(ALuint bid)
{
    return alIsBuffer(bid);
}

ALvoid CDECL wine_alBufferData(ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq)
{
    alBufferData(bid, format, data, size, freq);
}

static ALvoid CDECL wine_alBufferDataStatic(const ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq)
{
    alBufferDataStatic(bid, format, data, size, freq);
}

ALvoid CDECL wine_alGetBufferf(ALuint bid, ALenum param, ALfloat* value)
{
    alGetBufferf(bid, param, value);
}

ALvoid CDECL wine_alGetBufferfv(ALuint bid, ALenum param, ALfloat* values)
{
    alGetBufferfv(bid, param, values);
}

ALvoid CDECL wine_alGetBufferi(ALuint bid, ALenum param, ALint* value)
{
    alGetBufferi(bid, param, value);
}

ALvoid CDECL wine_alGetBufferiv(ALuint bid, ALenum param, ALint* values)
{
    alGetBufferiv(bid, param, values);
}

ALvoid CDECL wine_alDopplerFactor(ALfloat value)
{
    alDopplerFactor(value);
}

ALvoid CDECL wine_alDopplerVelocity(ALfloat value)
{
    alDopplerVelocity(value);
}

ALvoid CDECL wine_alDistanceModel(ALenum distanceModel)
{
    alDistanceModel(distanceModel);
}


/* OpenAL ALC 1.1 functions */
ALCdevice* CDECL wine_alcCaptureOpenDevice(const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize)
{
     return alcCaptureOpenDevice(devicename, frequency, format, buffersize);
}

ALCboolean CDECL wine_alcCaptureCloseDevice(ALCdevice *device)
{
    return alcCaptureCloseDevice(device);
}

ALCvoid CDECL wine_alcCaptureStart(ALCdevice *device)
{
    alcCaptureStart(device);
}

ALCvoid CDECL wine_alcCaptureStop(ALCdevice *device)
{
    alcCaptureStop(device);
}

ALCvoid CDECL wine_alcCaptureSamples(ALCdevice *device, ALCvoid *buffer, ALCsizei samples)
{
    alcCaptureSamples(device, buffer, samples);
}

/* OpenAL 1.1 functions */
ALvoid CDECL wine_alListener3i(ALenum param, ALint value1, ALint value2, ALint value3)
{
    alListener3i(param, value1, value2, value3);
}

ALvoid CDECL wine_alListeneriv(ALenum param, const ALint* values)
{
    alListeneriv(param, values);
}

ALvoid CDECL wine_alGetListener3i(ALenum param, ALint *value1, ALint *value2, ALint *value3)
{
    alGetListener3i(param, value1, value2, value3);
}

ALvoid CDECL wine_alSource3i(ALuint sid, ALenum param, ALint value1, ALint value2, ALint value3)
{
    alSource3i(sid, param, value1, value2, value3);
}

ALvoid CDECL wine_alSourceiv(ALuint sid, ALenum param, const ALint* values)
{
    alSourceiv(sid, param, values);
}

ALvoid CDECL wine_alGetSource3i(ALuint sid, ALenum param, ALint* value1, ALint* value2, ALint* value3)
{
    alGetSource3i(sid, param, value1, value2, value3);
}

ALvoid CDECL wine_alBufferf(ALuint bid, ALenum param, ALfloat value)
{
    alBufferf(bid, param, value);
}

ALvoid CDECL wine_alBuffer3f(ALuint bid, ALenum param, ALfloat value1, ALfloat value2, ALfloat value3)
{
    alBuffer3f(bid, param, value1, value2, value3);
}

ALvoid CDECL wine_alBufferfv(ALuint bid, ALenum param, const ALfloat* values)
{
    alBufferfv(bid, param, values);
}

ALvoid CDECL wine_alBufferi(ALuint bid, ALenum param, ALint value)
{
    alBufferi(bid, param, value);
}

ALvoid CDECL wine_alBuffer3i(ALuint bid, ALenum param, ALint value1, ALint value2, ALint value3)
{
    alBuffer3i(bid, param, value1, value2, value3);
}

ALvoid CDECL wine_alBufferiv(ALuint bid, ALenum param, const ALint* values)
{
    alBufferiv(bid, param, values);
}

ALvoid CDECL wine_alGetBuffer3f(ALuint bid, ALenum param, ALfloat* value1, ALfloat* value2, ALfloat* value3)
{
    alGetBuffer3f(bid, param, value1, value2, value3);
}

ALvoid CDECL wine_alGetBuffer3i(ALuint bid, ALenum param, ALint* value1, ALint* value2, ALint* value3)
{
    alGetBuffer3i(bid, param, value1, value2, value3);
}

ALvoid CDECL wine_alSpeedOfSound(ALfloat value)
{
    alSpeedOfSound(value);
}

/* OpenAL EFX extension functions */
static ALvoid CDECL wine_alGenFilters(ALsizei n, ALuint* filters)
{
    alGenFilters(n, filters);
}

static ALvoid CDECL wine_alDeleteFilters(ALsizei n, const ALuint* filters)
{
    alDeleteFilters(n, filters);
}

static ALboolean CDECL wine_alIsFilter(ALuint fid)
{
    return alIsFilter(fid);
}

static ALvoid CDECL wine_alFilterf(ALuint fid, ALenum param, ALfloat value)
{
    alFilterf(fid, param, value);
}

static ALvoid CDECL wine_alFilterfv(ALuint fid, ALenum param, const ALfloat* values)
{
    alFilterfv(fid, param, values);
}

static ALvoid CDECL wine_alFilteri(ALuint fid, ALenum param, ALint value)
{
    alFilteri(fid, param, value);
}

static ALvoid CDECL wine_alFilteriv(ALuint fid, ALenum param, const ALint* values)
{
    alFilteriv(fid, param, values);
}

static ALvoid CDECL wine_alGetFilterf(ALuint fid, ALenum param, ALfloat* value)
{
    alGetFilterf(fid, param, value);
}

static ALvoid CDECL wine_alGetFilterfv(ALuint fid, ALenum param, ALfloat* values)
{
    alGetFilterfv(fid, param, values);
}

static ALvoid CDECL wine_alGetFilteri(ALuint fid, ALenum param, ALint* value)
{
    alGetFilteri(fid, param, value);
}

static ALvoid CDECL wine_alGetFilteriv(ALuint fid, ALenum param, ALint* values)
{
    alGetFilteriv(fid, param, values);
}


static ALvoid CDECL wine_alGenEffects(ALsizei n, ALuint* effects)
{
    alGenEffects(n, effects);
}

static ALvoid CDECL wine_alDeleteEffects(ALsizei n, const ALuint* effects)
{
    alDeleteEffects(n, effects);
}

static ALboolean CDECL wine_alIsEffect(ALuint eid)
{
    return alIsEffect(eid);
}

static ALvoid CDECL wine_alEffectf(ALuint eid, ALenum param, ALfloat value)
{
    alEffectf(eid, param, value);
}

static ALvoid CDECL wine_alEffectfv(ALuint eid, ALenum param, const ALfloat* values)
{
    alEffectfv(eid, param, values);
}

static ALvoid CDECL wine_alEffecti(ALuint eid, ALenum param, ALint value)
{
    alEffecti(eid, param, value);
}

static ALvoid CDECL wine_alEffectiv(ALuint eid, ALenum param, const ALint* values)
{
    alEffectiv(eid, param, values);
}

static ALvoid CDECL wine_alGetEffectf(ALuint eid, ALenum param, ALfloat* value)
{
    alGetEffectf(eid, param, value);
}

static ALvoid CDECL wine_alGetEffectfv(ALuint eid, ALenum param, ALfloat* values)
{
    alGetEffectfv(eid, param, values);
}

static ALvoid CDECL wine_alGetEffecti(ALuint eid, ALenum param, ALint* value)
{
    alGetEffecti(eid, param, value);
}

static ALvoid CDECL wine_alGetEffectiv(ALuint eid, ALenum param, ALint* values)
{
    alGetEffectiv(eid, param, values);
}


static ALvoid CDECL wine_alGenAuxiliaryEffectSlots(ALsizei n, ALuint* slots)
{
    alGenAuxiliaryEffectSlots(n, slots);
}

static ALvoid CDECL wine_alDeleteAuxiliaryEffectSlots(ALsizei n, const ALuint* slots)
{
    alDeleteAuxiliaryEffectSlots(n, slots);
}

static ALboolean CDECL wine_alIsAuxiliaryEffectSlot(ALuint sid)
{
    return alIsAuxiliaryEffectSlot(sid);
}

static ALvoid CDECL wine_alAuxiliaryEffectSlotf(ALuint sid, ALenum param, ALfloat value)
{
    alAuxiliaryEffectSlotf(sid, param, value);
}

static ALvoid CDECL wine_alAuxiliaryEffectSlotfv(ALuint sid, ALenum param, const ALfloat* values)
{
    alAuxiliaryEffectSlotfv(sid, param, values);
}

static ALvoid CDECL wine_alAuxiliaryEffectSloti(ALuint sid, ALenum param, ALint value)
{
    alAuxiliaryEffectSloti(sid, param, value);
}

static ALvoid CDECL wine_alAuxiliaryEffectSlotiv(ALuint sid, ALenum param, const ALint* values)
{
    alAuxiliaryEffectSlotiv(sid, param, values);
}

static ALvoid CDECL wine_alGetAuxiliaryEffectSlotf(ALuint sid, ALenum param, ALfloat* value)
{
    alGetAuxiliaryEffectSlotf(sid, param, value);
}

static ALvoid CDECL wine_alGetAuxiliaryEffectSlotfv(ALuint sid, ALenum param, ALfloat* values)
{
    alGetAuxiliaryEffectSlotfv(sid, param, values);
}

static ALvoid CDECL wine_alGetAuxiliaryEffectSloti(ALuint sid, ALenum param, ALint* value)
{
    alGetAuxiliaryEffectSloti(sid, param, value);
}

static ALvoid CDECL wine_alGetAuxiliaryEffectSlotiv(ALuint sid, ALenum param, ALint* values)
{
    alGetAuxiliaryEffectSlotiv(sid, param, values);
}


/* Thread-local context functions */
static ALCboolean CDECL wine_alcSetThreadContext(ALCcontext *context)
{
    EnterCriticalSection(&openal_cs);
    if(alcSetThreadContext(context) == ALC_FALSE)
    {
        WARN("Failed to make context %p current\n", context);
        LeaveCriticalSection(&openal_cs);
        return ALC_FALSE;
    }

    if(context && !loaded_procs)
    {
        loaded_procs = AL_TRUE;
        LoadProcs();
    }
    LeaveCriticalSection(&openal_cs);

    return ALC_TRUE;
}

static ALCcontext* CDECL wine_alcGetThreadContext(ALCvoid)
{
    return alcGetThreadContext();
}

struct FuncList {
    const char *name;
    void *proc;
};

static const struct FuncList ALCFuncs[] = {
    { "alcCreateContext",           wine_alcCreateContext        },
    { "alcMakeContextCurrent",      wine_alcMakeContextCurrent   },
    { "alcProcessContext",          wine_alcProcessContext       },
    { "alcSuspendContext",          wine_alcSuspendContext       },
    { "alcDestroyContext",          wine_alcDestroyContext       },
    { "alcGetCurrentContext",       wine_alcGetCurrentContext    },
    { "alcGetContextsDevice",       wine_alcGetContextsDevice    },
    { "alcOpenDevice",              wine_alcOpenDevice           },
    { "alcCloseDevice",             wine_alcCloseDevice          },
    { "alcGetError",                wine_alcGetError             },
    { "alcIsExtensionPresent",      wine_alcIsExtensionPresent   },
    { "alcGetProcAddress",          wine_alcGetProcAddress       },
    { "alcGetEnumValue",            wine_alcGetEnumValue         },
    { "alcGetString",               wine_alcGetString            },
    { "alcGetIntegerv",             wine_alcGetIntegerv          },
    { "alcCaptureOpenDevice",       wine_alcCaptureOpenDevice    },
    { "alcCaptureCloseDevice",      wine_alcCaptureCloseDevice   },
    { "alcCaptureStart",            wine_alcCaptureStart         },
    { "alcCaptureStop",             wine_alcCaptureStop          },
    { "alcCaptureSamples",          wine_alcCaptureSamples       },
    { "alcSetThreadContext",        wine_alcSetThreadContext     },
    { "alcGetThreadContext",        wine_alcGetThreadContext     },
    { NULL,                         NULL                         }
};
static const struct FuncList ALFuncs[] = {
    { "alEnable",                   wine_alEnable                },
    { "alDisable",                  wine_alDisable               },
    { "alIsEnabled",                wine_alIsEnabled             },
    { "alGetString",                wine_alGetString             },
    { "alGetBooleanv",              wine_alGetBooleanv           },
    { "alGetIntegerv",              wine_alGetIntegerv           },
    { "alGetFloatv",                wine_alGetFloatv             },
    { "alGetDoublev",               wine_alGetDoublev            },
    { "alGetBoolean",               wine_alGetBoolean            },
    { "alGetInteger",               wine_alGetInteger            },
    { "alGetFloat",                 wine_alGetFloat              },
    { "alGetDouble",                wine_alGetDouble             },
    { "alGetError",                 wine_alGetError              },
    { "alIsExtensionPresent",       wine_alIsExtensionPresent    },
    { "alGetProcAddress",           wine_alGetProcAddress        },
    { "alGetEnumValue",             wine_alGetEnumValue          },
    { "alListenerf",                wine_alListenerf             },
    { "alListener3f",               wine_alListener3f            },
    { "alListenerfv",               wine_alListenerfv            },
    { "alListeneri",                wine_alListeneri             },
    { "alListener3i",               wine_alListener3i            },
    { "alListeneriv",               wine_alListeneriv            },
    { "alGetListenerf",             wine_alGetListenerf          },
    { "alGetListener3f",            wine_alGetListener3f         },
    { "alGetListenerfv",            wine_alGetListenerfv         },
    { "alGetListeneri",             wine_alGetListeneri          },
    { "alGetListener3i",            wine_alGetListener3i         },
    { "alGetListeneriv",            wine_alGetListeneriv         },
    { "alGenSources",               wine_alGenSources            },
    { "alDeleteSources",            wine_alDeleteSources         },
    { "alIsSource",                 wine_alIsSource              },
    { "alSourcef",                  wine_alSourcef               },
    { "alSource3f",                 wine_alSource3f              },
    { "alSourcefv",                 wine_alSourcefv              },
    { "alSourcei",                  wine_alSourcei               },
    { "alSource3i",                 wine_alSource3i              },
    { "alSourceiv",                 wine_alSourceiv              },
    { "alGetSourcef",               wine_alGetSourcef            },
    { "alGetSource3f",              wine_alGetSource3f           },
    { "alGetSourcefv",              wine_alGetSourcefv           },
    { "alGetSourcei",               wine_alGetSourcei            },
    { "alGetSource3i",              wine_alGetSource3i           },
    { "alGetSourceiv",              wine_alGetSourceiv           },
    { "alSourcePlayv",              wine_alSourcePlayv           },
    { "alSourceStopv",              wine_alSourceStopv           },
    { "alSourceRewindv",            wine_alSourceRewindv         },
    { "alSourcePausev",             wine_alSourcePausev          },
    { "alSourcePlay",               wine_alSourcePlay            },
    { "alSourceStop",               wine_alSourceStop            },
    { "alSourceRewind",             wine_alSourceRewind          },
    { "alSourcePause",              wine_alSourcePause           },
    { "alSourceQueueBuffers",       wine_alSourceQueueBuffers    },
    { "alSourceUnqueueBuffers",     wine_alSourceUnqueueBuffers  },
    { "alGenBuffers",               wine_alGenBuffers            },
    { "alDeleteBuffers",            wine_alDeleteBuffers         },
    { "alIsBuffer",                 wine_alIsBuffer              },
    { "alBufferData",               wine_alBufferData            },
    { "alBufferDataStatic",         wine_alBufferDataStatic      },
    { "alBufferf",                  wine_alBufferf               },
    { "alBuffer3f",                 wine_alBuffer3f              },
    { "alBufferfv",                 wine_alBufferfv              },
    { "alBufferi",                  wine_alBufferi               },
    { "alBuffer3i",                 wine_alBuffer3i              },
    { "alBufferiv",                 wine_alBufferiv              },
    { "alGetBufferf",               wine_alGetBufferf            },
    { "alGetBuffer3f",              wine_alGetBuffer3f           },
    { "alGetBufferfv",              wine_alGetBufferfv           },
    { "alGetBufferi",               wine_alGetBufferi            },
    { "alGetBuffer3i",              wine_alGetBuffer3i           },
    { "alGetBufferiv",              wine_alGetBufferiv           },
    { "alDopplerFactor",            wine_alDopplerFactor         },
    { "alDopplerVelocity",          wine_alDopplerVelocity       },
    { "alSpeedOfSound",             wine_alSpeedOfSound          },
    { "alDistanceModel",            wine_alDistanceModel         },

    { "alGenFilters",               wine_alGenFilters            },
    { "alDeleteFilters",            wine_alDeleteFilters         },
    { "alIsFilter",                 wine_alIsFilter              },
    { "alFilterf",                  wine_alFilterf               },
    { "alFilterfv",                 wine_alFilterfv              },
    { "alFilteri",                  wine_alFilteri               },
    { "alFilteriv",                 wine_alFilteriv              },
    { "alGetFilterf",               wine_alGetFilterf            },
    { "alGetFilterfv",              wine_alGetFilterfv           },
    { "alGetFilteri",               wine_alGetFilteri            },
    { "alGetFilteriv",              wine_alGetFilteriv           },

    { "alGenEffects",               wine_alGenEffects            },
    { "alDeleteEffects",            wine_alDeleteEffects         },
    { "alIsEffect",                 wine_alIsEffect              },
    { "alEffectf",                  wine_alEffectf               },
    { "alEffectfv",                 wine_alEffectfv              },
    { "alEffecti",                  wine_alEffecti               },
    { "alEffectiv",                 wine_alEffectiv              },
    { "alGetEffectf",               wine_alGetEffectf            },
    { "alGetEffectfv",              wine_alGetEffectfv           },
    { "alGetEffecti",               wine_alGetEffecti            },
    { "alGetEffectiv",              wine_alGetEffectiv           },

    { "alGenAuxiliaryEffectSlots",  wine_alGenAuxiliaryEffectSlots},
    { "alDeleteAuxiliaryEffectSlots",wine_alDeleteAuxiliaryEffectSlots},
    { "alIsAuxiliaryEffectSlot",    wine_alIsAuxiliaryEffectSlot },
    { "alAuxiliaryEffectSlotf",     wine_alAuxiliaryEffectSlotf  },
    { "alAuxiliaryEffectSlotfv",    wine_alAuxiliaryEffectSlotfv },
    { "alAuxiliaryEffectSloti",     wine_alAuxiliaryEffectSloti  },
    { "alAuxiliaryEffectSlotiv",    wine_alAuxiliaryEffectSlotiv },
    { "alGetAuxiliaryEffectSlotf",  wine_alGetAuxiliaryEffectSlotf},
    { "alGetAuxiliaryEffectSlotfv", wine_alGetAuxiliaryEffectSlotfv},
    { "alGetAuxiliaryEffectSloti",  wine_alGetAuxiliaryEffectSloti},
    { "alGetAuxiliaryEffectSlotiv", wine_alGetAuxiliaryEffectSlotiv},

    { NULL,                         NULL                         }
};


ALCvoid* CDECL wine_alcGetProcAddress(ALCdevice *device, const ALCchar *funcname)
{
    void *proc;
    int i;

    /* Make sure the host implementation has the requested function */
    proc = alcGetProcAddress(device, funcname);
    if(!proc)
        return NULL;

    for(i = 0;ALCFuncs[i].name;i++)
    {
        if(strcmp(funcname, ALCFuncs[i].name) == 0)
            return ALCFuncs[i].proc;
    }
    FIXME("Could not find function in list: %s\n", funcname);
    return NULL;
}


ALvoid* CDECL wine_alGetProcAddress(const ALchar* funcname)
{
    void *proc;
    int i;

    /* Make sure the host implementation has the requested function. This will
     * also set the last AL error properly if the function should not be
     * returned (eg. no current context). */
    proc = alGetProcAddress(funcname);
    if(!proc)
        return NULL;

    for(i = 0;ALFuncs[i].name;i++)
    {
        if(strcmp(funcname, ALFuncs[i].name) == 0)
            return ALFuncs[i].proc;
    }
    FIXME("Could not find function in list: %s\n", funcname);
    return NULL;
}
