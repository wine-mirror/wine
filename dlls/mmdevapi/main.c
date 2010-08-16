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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#ifdef HAVE_AL_AL_H
#include <AL/al.h>
#include <AL/alc.h>
#elif defined(HAVE_OPENAL_AL_H)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/library.h"

#include "ole2.h"
#include "olectl.h"
#include "propsys.h"
#include "initguid.h"
#include "propkeydef.h"
#include "mmdeviceapi.h"
#include "dshow.h"
#include "dsound.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"
#include "devpkey.h"

#include "mmdevapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

#ifdef HAVE_OPENAL

int local_contexts;

static CRITICAL_SECTION_DEBUG openal_crst_debug =
{
    0, 0, &openal_crst,
    { &openal_crst_debug.ProcessLocksList,
      &openal_crst_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": openal_crst_debug") }
};
CRITICAL_SECTION openal_crst = { &openal_crst_debug, -1, 0, 0, 0, 0 };

static void *openal_handle = RTLD_DEFAULT;
int openal_loaded;
#ifdef SONAME_LIBOPENAL
LPALCCREATECONTEXT palcCreateContext = NULL;
LPALCMAKECONTEXTCURRENT palcMakeContextCurrent = NULL;
LPALCPROCESSCONTEXT palcProcessContext = NULL;
LPALCSUSPENDCONTEXT palcSuspendContext = NULL;
LPALCDESTROYCONTEXT palcDestroyContext = NULL;
LPALCGETCURRENTCONTEXT palcGetCurrentContext = NULL;
LPALCGETCONTEXTSDEVICE palcGetContextsDevice = NULL;
LPALCOPENDEVICE palcOpenDevice = NULL;
LPALCCLOSEDEVICE palcCloseDevice = NULL;
LPALCGETERROR palcGetError = NULL;
LPALCISEXTENSIONPRESENT palcIsExtensionPresent = NULL;
LPALCGETPROCADDRESS palcGetProcAddress = NULL;
LPALCGETENUMVALUE palcGetEnumValue = NULL;
LPALCGETSTRING palcGetString = NULL;
LPALCGETINTEGERV palcGetIntegerv = NULL;
LPALCCAPTUREOPENDEVICE palcCaptureOpenDevice = NULL;
LPALCCAPTURECLOSEDEVICE palcCaptureCloseDevice = NULL;
LPALCCAPTURESTART palcCaptureStart = NULL;
LPALCCAPTURESTOP palcCaptureStop = NULL;
LPALCCAPTURESAMPLES palcCaptureSamples = NULL;
LPALENABLE palEnable = NULL;
LPALDISABLE palDisable = NULL;
LPALISENABLED palIsEnabled = NULL;
LPALGETSTRING palGetString = NULL;
LPALGETBOOLEANV palGetBooleanv = NULL;
LPALGETINTEGERV palGetIntegerv = NULL;
LPALGETFLOATV palGetFloatv = NULL;
LPALGETDOUBLEV palGetDoublev = NULL;
LPALGETBOOLEAN palGetBoolean = NULL;
LPALGETINTEGER palGetInteger = NULL;
LPALGETFLOAT palGetFloat = NULL;
LPALGETDOUBLE palGetDouble = NULL;
LPALGETERROR palGetError = NULL;
LPALISEXTENSIONPRESENT palIsExtensionPresent = NULL;
LPALGETPROCADDRESS palGetProcAddress = NULL;
LPALGETENUMVALUE palGetEnumValue = NULL;
LPALLISTENERF palListenerf = NULL;
LPALLISTENER3F palListener3f = NULL;
LPALLISTENERFV palListenerfv = NULL;
LPALLISTENERI palListeneri = NULL;
LPALLISTENER3I palListener3i = NULL;
LPALLISTENERIV palListeneriv = NULL;
LPALGETLISTENERF palGetListenerf = NULL;
LPALGETLISTENER3F palGetListener3f = NULL;
LPALGETLISTENERFV palGetListenerfv = NULL;
LPALGETLISTENERI palGetListeneri = NULL;
LPALGETLISTENER3I palGetListener3i = NULL;
LPALGETLISTENERIV palGetListeneriv = NULL;
LPALGENSOURCES palGenSources = NULL;
LPALDELETESOURCES palDeleteSources = NULL;
LPALISSOURCE palIsSource = NULL;
LPALSOURCEF palSourcef = NULL;
LPALSOURCE3F palSource3f = NULL;
LPALSOURCEFV palSourcefv = NULL;
LPALSOURCEI palSourcei = NULL;
LPALSOURCE3I palSource3i = NULL;
LPALSOURCEIV palSourceiv = NULL;
LPALGETSOURCEF palGetSourcef = NULL;
LPALGETSOURCE3F palGetSource3f = NULL;
LPALGETSOURCEFV palGetSourcefv = NULL;
LPALGETSOURCEI palGetSourcei = NULL;
LPALGETSOURCE3I palGetSource3i = NULL;
LPALGETSOURCEIV palGetSourceiv = NULL;
LPALSOURCEPLAYV palSourcePlayv = NULL;
LPALSOURCESTOPV palSourceStopv = NULL;
LPALSOURCEREWINDV palSourceRewindv = NULL;
LPALSOURCEPAUSEV palSourcePausev = NULL;
LPALSOURCEPLAY palSourcePlay = NULL;
LPALSOURCESTOP palSourceStop = NULL;
LPALSOURCEREWIND palSourceRewind = NULL;
LPALSOURCEPAUSE palSourcePause = NULL;
LPALSOURCEQUEUEBUFFERS palSourceQueueBuffers = NULL;
LPALSOURCEUNQUEUEBUFFERS palSourceUnqueueBuffers = NULL;
LPALGENBUFFERS palGenBuffers = NULL;
LPALDELETEBUFFERS palDeleteBuffers = NULL;
LPALISBUFFER palIsBuffer = NULL;
LPALBUFFERF palBufferf = NULL;
LPALBUFFER3F palBuffer3f = NULL;
LPALBUFFERFV palBufferfv = NULL;
LPALBUFFERI palBufferi = NULL;
LPALBUFFER3I palBuffer3i = NULL;
LPALBUFFERIV palBufferiv = NULL;
LPALGETBUFFERF palGetBufferf = NULL;
LPALGETBUFFER3F palGetBuffer3f = NULL;
LPALGETBUFFERFV palGetBufferfv = NULL;
LPALGETBUFFERI palGetBufferi = NULL;
LPALGETBUFFER3I palGetBuffer3i = NULL;
LPALGETBUFFERIV palGetBufferiv = NULL;
LPALBUFFERDATA palBufferData = NULL;
LPALDOPPLERFACTOR palDopplerFactor = NULL;
LPALDOPPLERVELOCITY palDopplerVelocity = NULL;
LPALDISTANCEMODEL palDistanceModel = NULL;
LPALSPEEDOFSOUND palSpeedOfSound = NULL;
#endif

typeof(alcGetCurrentContext) *get_context;
typeof(alcMakeContextCurrent) *set_context;

static void load_libopenal(void)
{
    DWORD failed = 0;

#ifdef SONAME_LIBOPENAL
    char error[128];
    openal_handle = wine_dlopen(SONAME_LIBOPENAL, RTLD_NOW, error, sizeof(error));
    if (!openal_handle)
    {
        ERR("Couldn't load " SONAME_LIBOPENAL ": %s\n", error);
        return;
    }

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(openal_handle, #f, NULL, 0)) == NULL) { \
        ERR("Couldn't lookup %s in libopenal\n", #f); \
        failed = 1; \
    }

    LOAD_FUNCPTR(alcCreateContext);
    LOAD_FUNCPTR(alcMakeContextCurrent);
    LOAD_FUNCPTR(alcProcessContext);
    LOAD_FUNCPTR(alcSuspendContext);
    LOAD_FUNCPTR(alcDestroyContext);
    LOAD_FUNCPTR(alcGetCurrentContext);
    LOAD_FUNCPTR(alcGetContextsDevice);
    LOAD_FUNCPTR(alcOpenDevice);
    LOAD_FUNCPTR(alcCloseDevice);
    LOAD_FUNCPTR(alcGetError);
    LOAD_FUNCPTR(alcIsExtensionPresent);
    LOAD_FUNCPTR(alcGetProcAddress);
    LOAD_FUNCPTR(alcGetEnumValue);
    LOAD_FUNCPTR(alcGetString);
    LOAD_FUNCPTR(alcGetIntegerv);
    LOAD_FUNCPTR(alcCaptureOpenDevice);
    LOAD_FUNCPTR(alcCaptureCloseDevice);
    LOAD_FUNCPTR(alcCaptureStart);
    LOAD_FUNCPTR(alcCaptureStop);
    LOAD_FUNCPTR(alcCaptureSamples);
    LOAD_FUNCPTR(alEnable);
    LOAD_FUNCPTR(alDisable);
    LOAD_FUNCPTR(alIsEnabled);
    LOAD_FUNCPTR(alGetString);
    LOAD_FUNCPTR(alGetBooleanv);
    LOAD_FUNCPTR(alGetIntegerv);
    LOAD_FUNCPTR(alGetFloatv);
    LOAD_FUNCPTR(alGetDoublev);
    LOAD_FUNCPTR(alGetBoolean);
    LOAD_FUNCPTR(alGetInteger);
    LOAD_FUNCPTR(alGetFloat);
    LOAD_FUNCPTR(alGetDouble);
    LOAD_FUNCPTR(alGetError);
    LOAD_FUNCPTR(alIsExtensionPresent);
    LOAD_FUNCPTR(alGetProcAddress);
    LOAD_FUNCPTR(alGetEnumValue);
    LOAD_FUNCPTR(alListenerf);
    LOAD_FUNCPTR(alListener3f);
    LOAD_FUNCPTR(alListenerfv);
    LOAD_FUNCPTR(alListeneri);
    LOAD_FUNCPTR(alListener3i);
    LOAD_FUNCPTR(alListeneriv);
    LOAD_FUNCPTR(alGetListenerf);
    LOAD_FUNCPTR(alGetListener3f);
    LOAD_FUNCPTR(alGetListenerfv);
    LOAD_FUNCPTR(alGetListeneri);
    LOAD_FUNCPTR(alGetListener3i);
    LOAD_FUNCPTR(alGetListeneriv);
    LOAD_FUNCPTR(alGenSources);
    LOAD_FUNCPTR(alDeleteSources);
    LOAD_FUNCPTR(alIsSource);
    LOAD_FUNCPTR(alSourcef);
    LOAD_FUNCPTR(alSource3f);
    LOAD_FUNCPTR(alSourcefv);
    LOAD_FUNCPTR(alSourcei);
    LOAD_FUNCPTR(alSource3i);
    LOAD_FUNCPTR(alSourceiv);
    LOAD_FUNCPTR(alGetSourcef);
    LOAD_FUNCPTR(alGetSource3f);
    LOAD_FUNCPTR(alGetSourcefv);
    LOAD_FUNCPTR(alGetSourcei);
    LOAD_FUNCPTR(alGetSource3i);
    LOAD_FUNCPTR(alGetSourceiv);
    LOAD_FUNCPTR(alSourcePlayv);
    LOAD_FUNCPTR(alSourceStopv);
    LOAD_FUNCPTR(alSourceRewindv);
    LOAD_FUNCPTR(alSourcePausev);
    LOAD_FUNCPTR(alSourcePlay);
    LOAD_FUNCPTR(alSourceStop);
    LOAD_FUNCPTR(alSourceRewind);
    LOAD_FUNCPTR(alSourcePause);
    LOAD_FUNCPTR(alSourceQueueBuffers);
    LOAD_FUNCPTR(alSourceUnqueueBuffers);
    LOAD_FUNCPTR(alGenBuffers);
    LOAD_FUNCPTR(alDeleteBuffers);
    LOAD_FUNCPTR(alIsBuffer);
    LOAD_FUNCPTR(alBufferf);
    LOAD_FUNCPTR(alBuffer3f);
    LOAD_FUNCPTR(alBufferfv);
    LOAD_FUNCPTR(alBufferi);
    LOAD_FUNCPTR(alBuffer3i);
    LOAD_FUNCPTR(alBufferiv);
    LOAD_FUNCPTR(alGetBufferf);
    LOAD_FUNCPTR(alGetBuffer3f);
    LOAD_FUNCPTR(alGetBufferfv);
    LOAD_FUNCPTR(alGetBufferi);
    LOAD_FUNCPTR(alGetBuffer3i);
    LOAD_FUNCPTR(alGetBufferiv);
    LOAD_FUNCPTR(alBufferData);
    LOAD_FUNCPTR(alDopplerFactor);
    LOAD_FUNCPTR(alDopplerVelocity);
    LOAD_FUNCPTR(alDistanceModel);
    LOAD_FUNCPTR(alSpeedOfSound);
#undef LOAD_FUNCPTR
#endif

    if (failed)
    {
        WARN("Unloading openal\n");
        if (openal_handle != RTLD_DEFAULT)
            wine_dlclose(openal_handle, NULL, 0);
        openal_handle = NULL;
        openal_loaded = 0;
    }
    else
    {
        openal_loaded = 1;
        local_contexts = palcIsExtensionPresent(NULL, "ALC_EXT_thread_local_context");
        if (local_contexts)
        {
            set_context = palcGetProcAddress(NULL, "alcSetThreadContext");
            get_context = palcGetProcAddress(NULL, "alcGetThreadContext");
            if (!set_context || !get_context)
            {
                ERR("TLS advertised but functions not found, disabling thread local context\n");
                local_contexts = 0;
            }
        }
        if (!local_contexts)
        {
            set_context = palcMakeContextCurrent;
            get_context = palcGetCurrentContext;
        }
    }
}

#endif /*HAVE_OPENAL*/

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
#ifdef HAVE_OPENAL
            load_libopenal();
#endif /*HAVE_OPENAL*/
            break;
        case DLL_PROCESS_DETACH:
            MMDevEnum_Free();
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

typedef HRESULT (*FnCreateInstance)(REFIID riid, LPVOID *ppobj);

typedef struct {
    const IClassFactoryVtbl *lpVtbl;
    REFCLSID rclsid;
    FnCreateInstance pfnCreateInstance;
} IClassFactoryImpl;

static HRESULT WINAPI
MMCF_QueryInterface(LPCLASSFACTORY iface, REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);
    if (ppobj == NULL)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppobj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MMCF_AddRef(LPCLASSFACTORY iface)
{
    return 2;
}

static ULONG WINAPI MMCF_Release(LPCLASSFACTORY iface)
{
    /* static class, won't be freed */
    return 1;
}

static HRESULT WINAPI MMCF_CreateInstance(
    LPCLASSFACTORY iface,
    LPUNKNOWN pOuter,
    REFIID riid,
    LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    TRACE("(%p, %p, %s, %p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    if (ppobj == NULL) {
        WARN("invalid parameter\n");
        return E_POINTER;
    }
    *ppobj = NULL;
    return This->pfnCreateInstance(riid, ppobj);
}

static HRESULT WINAPI MMCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("(%p, %d) stub!\n", This, dolock);
    return S_OK;
}

static const IClassFactoryVtbl MMCF_Vtbl = {
    MMCF_QueryInterface,
    MMCF_AddRef,
    MMCF_Release,
    MMCF_CreateInstance,
    MMCF_LockServer
};

static IClassFactoryImpl MMDEVAPI_CF[] = {
    { &MMCF_Vtbl, &CLSID_MMDeviceEnumerator, (FnCreateInstance)MMDevEnum_Create }
};

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    int i = 0;
    TRACE("(%s, %s, %p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (ppv == NULL) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (!IsEqualIID(riid, &IID_IClassFactory) &&
        !IsEqualIID(riid, &IID_IUnknown)) {
        WARN("no interface for %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    for (i = 0; i < sizeof(MMDEVAPI_CF)/sizeof(MMDEVAPI_CF[0]); ++i)
    {
        if (IsEqualGUID(rclsid, MMDEVAPI_CF[i].rclsid)) {
            IUnknown_AddRef((IClassFactory*) &MMDEVAPI_CF[i]);
            *ppv = &MMDEVAPI_CF[i];
            return S_OK;
        }
        i++;
    }

    WARN("(%s, %s, %p): no class found.\n", debugstr_guid(rclsid),
         debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
