#OpenAL ALC_1_0
@ cdecl alcCreateContext(ptr ptr) wine_alcCreateContext
@ cdecl alcMakeContextCurrent(ptr) wine_alcMakeContextCurrent
@ cdecl alcProcessContext(ptr) wine_alcProcessContext
@ cdecl alcSuspendContext(ptr) wine_alcSuspendContext
@ cdecl alcDestroyContext(ptr) wine_alcDestroyContext
@ cdecl alcGetCurrentContext() wine_alcGetCurrentContext
@ cdecl alcGetContextsDevice(ptr) wine_alcGetContextsDevice
@ cdecl alcOpenDevice(str) wine_alcOpenDevice
@ cdecl alcCloseDevice(ptr) wine_alcCloseDevice
@ cdecl alcGetError(ptr) wine_alcGetError
@ cdecl alcIsExtensionPresent(ptr str) wine_alcIsExtensionPresent
@ cdecl alcGetProcAddress(ptr str) wine_alcGetProcAddress
@ cdecl alcGetEnumValue(ptr str) wine_alcGetEnumValue
@ cdecl alcGetString(ptr long) wine_alcGetString
@ cdecl alcGetIntegerv(ptr long long ptr) wine_alcGetIntegerv
#OpenAL AL_1_0
@ cdecl alEnable(long) wine_alEnable
@ cdecl alDisable(long) wine_alDisable
@ cdecl alIsEnabled(long) wine_alIsEnabled
@ cdecl alGetString(long) wine_alGetString
@ cdecl alGetBooleanv(long ptr) wine_alGetBooleanv
@ cdecl alGetIntegerv(long ptr) wine_alGetIntegerv
@ cdecl alGetFloatv(long ptr) wine_alGetFloatv
@ cdecl alGetDoublev(long ptr) wine_alGetDoublev
@ cdecl alGetBoolean(long) wine_alGetBoolean
@ cdecl alGetInteger(long) wine_alGetInteger
@ cdecl alGetFloat(long) wine_alGetFloat
@ cdecl alGetDouble(long) wine_alGetDouble
@ cdecl alGetError() wine_alGetError
@ cdecl alIsExtensionPresent(str) wine_alIsExtensionPresent
@ cdecl alGetProcAddress(str) wine_alGetProcAddress
@ cdecl alGetEnumValue(str) wine_alGetEnumValue
@ cdecl alListenerf(long long) wine_alListenerf
@ cdecl alListener3f(long long long long) wine_alListener3f
@ cdecl alListenerfv(long ptr) wine_alListenerfv
@ cdecl alListeneri(long long) wine_alListeneri
@ cdecl alGetListenerf(long ptr) wine_alGetListenerf
@ cdecl alGetListener3f(long ptr ptr ptr) wine_alGetListener3f
@ cdecl alGetListenerfv(long ptr) wine_alGetListenerfv
@ cdecl alGetListeneri(long ptr) wine_alGetListeneri
@ cdecl alGetListeneriv(long ptr) wine_alGetListeneriv
@ cdecl alGenSources(long ptr) wine_alGenSources
@ cdecl alDeleteSources(long ptr) wine_alDeleteSources
@ cdecl alIsSource(long) wine_alIsSource
@ cdecl alSourcef(long long long) wine_alSourcef
@ cdecl alSource3f(long long long long long) wine_alSource3f
@ cdecl alSourcefv(long long ptr) wine_alSourcefv
@ cdecl alSourcei(long long long) wine_alSourcei
@ cdecl alGetSourcef(long long ptr) wine_alGetSourcef
@ cdecl alGetSource3f(long long ptr ptr ptr) wine_alGetSource3f
@ cdecl alGetSourcefv(long long ptr) wine_alGetSourcefv
@ cdecl alGetSourcei(long long ptr) wine_alGetSourcei
@ cdecl alGetSourceiv(long long ptr) wine_alGetSourceiv
@ cdecl alSourcePlayv(long ptr) wine_alSourcePlayv
@ cdecl alSourceStopv(long ptr) wine_alSourceStopv
@ cdecl alSourceRewindv(long ptr) wine_alSourceRewindv
@ cdecl alSourcePausev(long ptr) wine_alSourcePausev
@ cdecl alSourcePlay(long) wine_alSourcePlay
@ cdecl alSourceStop(long) wine_alSourceStop
@ cdecl alSourceRewind(long) wine_alSourceRewind
@ cdecl alSourcePause(long) wine_alSourcePause
@ cdecl alSourceQueueBuffers(long long ptr) wine_alSourceQueueBuffers
@ cdecl alSourceUnqueueBuffers(long long ptr) wine_alSourceUnqueueBuffers
@ cdecl alGenBuffers(long ptr) wine_alGenBuffers
@ cdecl alDeleteBuffers(long ptr) wine_alDeleteBuffers
@ cdecl alIsBuffer(long) wine_alIsBuffer
@ cdecl alBufferData(long long ptr long long) wine_alBufferData
@ cdecl alGetBufferf(long long ptr) wine_alGetBufferf
@ cdecl alGetBufferfv(long long ptr) wine_alGetBufferfv
@ cdecl alGetBufferi(long long ptr) wine_alGetBufferi
@ cdecl alGetBufferiv(long long ptr) wine_alGetBufferiv
@ cdecl alDopplerFactor(long) wine_alDopplerFactor
@ cdecl alDopplerVelocity(long) wine_alDopplerVelocity
@ cdecl alDistanceModel(long) wine_alDistanceModel
#OpenAL ALC_1_1
@ cdecl alcCaptureOpenDevice(str long long long) wine_alcCaptureOpenDevice
@ cdecl alcCaptureCloseDevice(ptr) wine_alcCaptureCloseDevice
@ cdecl alcCaptureStart(ptr) wine_alcCaptureStart
@ cdecl alcCaptureStop(ptr) wine_alcCaptureStop
@ cdecl alcCaptureSamples(ptr ptr long) wine_alcCaptureSamples
#OpenAL AL_1_1
@ cdecl alListener3i(long long long long) wine_alListener3i
@ cdecl alListeneriv(long ptr) wine_alListeneriv
@ cdecl alGetListener3i(long ptr ptr ptr) wine_alGetListener3i
@ cdecl alSource3i(long long long long long) wine_alSource3i
@ cdecl alSourceiv(long long ptr) wine_alSourceiv
@ cdecl alGetSource3i(long long ptr ptr ptr) wine_alGetSource3i
@ cdecl alBufferf(long long long) wine_alBufferf
@ cdecl alBuffer3f(long long long long long) wine_alBuffer3f
@ cdecl alBufferfv(long long ptr) wine_alBufferfv
@ cdecl alBufferi(long long long) wine_alBufferi
@ cdecl alBuffer3i(long long long long long) wine_alBuffer3i
@ cdecl alBufferiv(long long ptr) wine_alBufferiv
@ cdecl alGetBuffer3f(long long ptr ptr ptr) wine_alGetBuffer3f
@ cdecl alGetBuffer3i(long long ptr ptr ptr) wine_alGetBuffer3i
@ cdecl alSpeedOfSound(long) wine_alSpeedOfSound
