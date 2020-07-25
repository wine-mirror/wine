/*
 * Copyright 2019 Alistair Leslie-Hughes
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

#define COBJMACROS
#include "dmo.h"
#include "mmreg.h"
#include "mmsystem.h"
#include "initguid.h"
#include "dsound.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsdmo);

static HINSTANCE dsdmo_instance;

struct effect
{
    IMediaObject IMediaObject_iface;
    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;

    const struct effect_ops *ops;
};

struct effect_ops
{
    void *(*query_interface)(struct effect *effect, REFIID iid);
    void (*destroy)(struct effect *effect);
};

static struct effect *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct effect, IUnknown_inner);
}

static HRESULT WINAPI effect_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct effect *effect = impl_from_IUnknown(iface);

    TRACE("effect %p, iid %s, out %p.\n", effect, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IMediaObject))
        *out = &effect->IMediaObject_iface;
    else if (!(*out = effect->ops->query_interface(effect, iid)))
    {
        WARN("%s not implemented; returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI effect_inner_AddRef(IUnknown *iface)
{
    struct effect *effect = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&effect->refcount);

    TRACE("%p increasing refcount to %u.\n", effect, refcount);
    return refcount;
}

static ULONG WINAPI effect_inner_Release(IUnknown *iface)
{
    struct effect *effect = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&effect->refcount);

    TRACE("%p decreasing refcount to %u.\n", effect, refcount);

    if (!refcount)
    {
        effect->ops->destroy(effect);
    }
    return refcount;
}

static const IUnknownVtbl effect_inner_vtbl =
{
    effect_inner_QueryInterface,
    effect_inner_AddRef,
    effect_inner_Release,
};

static struct effect *impl_from_IMediaObject(IMediaObject *iface)
{
    return CONTAINING_RECORD(iface, struct effect, IMediaObject_iface);
}

static HRESULT WINAPI effect_QueryInterface(IMediaObject *iface, REFIID iid, void **out)
{
    struct effect *effect = impl_from_IMediaObject(iface);
    return IUnknown_QueryInterface(effect->outer_unk, iid, out);
}

static ULONG WINAPI effect_AddRef(IMediaObject *iface)
{
    struct effect *effect = impl_from_IMediaObject(iface);
    return IUnknown_AddRef(effect->outer_unk);
}

static ULONG WINAPI effect_Release(IMediaObject *iface)
{
    struct effect *effect = impl_from_IMediaObject(iface);
    return IUnknown_Release(effect->outer_unk);
}

static HRESULT WINAPI effect_GetStreamCount(IMediaObject *iface, DWORD *input, DWORD *output)
{
    FIXME("iface %p, input %p, output %p, stub!\n", iface, input, output);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %u, flags %p, stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetOutputStreamInfo(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %u, flags %p, stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %u, type_index %u, type %p, stub!\n", iface, index, type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetOutputType(IMediaObject *iface, DWORD index, DWORD type_index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %u, type_index %u, type %p, stub!\n", iface, index, type_index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_SetInputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("iface %p, index %u, type %p, flags %#x, stub!\n", iface, index, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_SetOutputType(IMediaObject *iface, DWORD index, const DMO_MEDIA_TYPE *type, DWORD flags)
{
    FIXME("iface %p, index %u, type %p, flags %#x, stub!\n", iface, index, type, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %u, type %p, stub!\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetOutputCurrentType(IMediaObject *iface, DWORD index, DMO_MEDIA_TYPE *type)
{
    FIXME("iface %p, index %u, type %p, stub!\n", iface, index, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputSizeInfo(IMediaObject *iface, DWORD index,
        DWORD *size, DWORD *lookahead, DWORD *alignment)
{
    FIXME("iface %p, index %u, size %p, lookahead %p, alignment %p, stub!\n", iface, index, size, lookahead, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetOutputSizeInfo(IMediaObject *iface, DWORD index, DWORD *size, DWORD *alignment)
{
    FIXME("iface %p, index %u, size %p, alignment %p, stub!\n", iface, index, size, alignment);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME *latency)
{
    FIXME("iface %p, index %u, latency %p, stub!\n", iface, index, latency);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_SetInputMaxLatency(IMediaObject *iface, DWORD index, REFERENCE_TIME latency)
{
    FIXME("iface %p, index %u, latency %s, stub!\n", iface, index, wine_dbgstr_longlong(latency));
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_Flush(IMediaObject *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_Discontinuity(IMediaObject *iface, DWORD index)
{
    FIXME("iface %p, index %u, stub!\n", iface, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_AllocateStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_FreeStreamingResources(IMediaObject *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_GetInputStatus(IMediaObject *iface, DWORD index, DWORD *flags)
{
    FIXME("iface %p, index %u, flags %p, stub!\n", iface, index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_ProcessInput(IMediaObject *iface, DWORD index,
    IMediaBuffer *buffer, DWORD flags, REFERENCE_TIME timestamp, REFERENCE_TIME timelength)
{
    FIXME("iface %p, index %u, buffer %p, flags %#x, timestamp %s, timelength %s, stub!\n",
            iface, index, buffer, flags, wine_dbgstr_longlong(timestamp), wine_dbgstr_longlong(timelength));
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_ProcessOutput(IMediaObject *iface, DWORD flags,
        DWORD count, DMO_OUTPUT_DATA_BUFFER *buffers, DWORD *status)
{
    FIXME("iface %p, flags %#x, count %u, buffers %p, status %p, stub!\n", iface, flags, count, buffers, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI effect_Lock(IMediaObject *iface, LONG lock)
{
    FIXME("iface %p, lock %d, stub!\n", iface, lock);
    return E_NOTIMPL;
}

static const IMediaObjectVtbl effect_vtbl =
{
    effect_QueryInterface,
    effect_AddRef,
    effect_Release,
    effect_GetStreamCount,
    effect_GetInputStreamInfo,
    effect_GetOutputStreamInfo,
    effect_GetInputType,
    effect_GetOutputType,
    effect_SetInputType,
    effect_SetOutputType,
    effect_GetInputCurrentType,
    effect_GetOutputCurrentType,
    effect_GetInputSizeInfo,
    effect_GetOutputSizeInfo,
    effect_GetInputMaxLatency,
    effect_SetInputMaxLatency,
    effect_Flush,
    effect_Discontinuity,
    effect_AllocateStreamingResources,
    effect_FreeStreamingResources,
    effect_GetInputStatus,
    effect_ProcessInput,
    effect_ProcessOutput,
    effect_Lock,
};

static void effect_init(struct effect *effect, IUnknown *outer, const struct effect_ops *ops)
{
    effect->outer_unk = outer ? outer : &effect->IUnknown_inner;
    effect->refcount = 1;
    effect->IUnknown_inner.lpVtbl = &effect_inner_vtbl;
    effect->IMediaObject_iface.lpVtbl = &effect_vtbl;

    effect->ops = ops;
}

struct waves_reverb
{
    struct effect effect;
};

static struct waves_reverb *impl_waves_reverb_from_effect(struct effect *iface)
{
    return CONTAINING_RECORD(iface, struct waves_reverb, effect);
}

static void *waves_reverb_query_interface(struct effect *iface, REFIID iid)
{
    return NULL;
}

static void waves_reverb_destroy(struct effect *iface)
{
    struct waves_reverb *effect = impl_waves_reverb_from_effect(iface);

    free(effect);
}

static const struct effect_ops waves_reverb_ops =
{
    .destroy = waves_reverb_destroy,
    .query_interface = waves_reverb_query_interface,
};

static HRESULT waves_reverb_create(IUnknown *outer, IUnknown **out)
{
    struct waves_reverb *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    effect_init(&object->effect, outer, &waves_reverb_ops);

    TRACE("Created waves reverb effect %p.\n", object);
    *out = &object->effect.IUnknown_inner;
    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        dsdmo_instance = instance;
        DisableThreadLibraryCalls(dsdmo_instance);
        break;
    }

    return TRUE;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);
    IUnknown *unk;
    HRESULT hr;

    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    *out = NULL;

    if (outer && !IsEqualGUID(iid, &IID_IUnknown))
        return E_NOINTERFACE;

    if (SUCCEEDED(hr = factory->create_instance(outer, &unk)))
    {
        hr = IUnknown_QueryInterface(unk, iid, out);
        IUnknown_Release(unk);
    }
    return hr;
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("lock %d, stub!\n", lock);
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

static struct
{
    const GUID *clsid;
    struct class_factory factory;
}
class_factories[] =
{
    {&GUID_DSFX_WAVES_REVERB,           {{&class_factory_vtbl}, waves_reverb_create}},
};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    unsigned int i;

    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    for (i = 0; i < ARRAY_SIZE(class_factories); ++i)
    {
        if (IsEqualGUID(clsid, class_factories[i].clsid))
            return IClassFactory_QueryInterface(&class_factories[i].factory.IClassFactory_iface, iid, out);
    }

    FIXME("%s not available, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    TRACE("()\n");
    return __wine_register_resources(dsdmo_instance);
}

HRESULT WINAPI DllUnregisterServer(void)
{
    TRACE("()\n");
    return __wine_unregister_resources(dsdmo_instance);
}
