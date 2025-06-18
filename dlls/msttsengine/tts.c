/*
 * MSTTSEngine SAPI engine implementation.
 *
 * Copyright 2023 Shaun Ren for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "sapiddk.h"
#include "sperror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msttsengine);

struct ttsengine
{
    ISpTTSEngine ISpTTSEngine_iface;
    ISpObjectWithToken ISpObjectWithToken_iface;
    LONG ref;

    ISpObjectToken *token;
};

static inline struct ttsengine *impl_from_ISpTTSEngine(ISpTTSEngine *iface)
{
    return CONTAINING_RECORD(iface, struct ttsengine, ISpTTSEngine_iface);
}

static inline struct ttsengine *impl_from_ISpObjectWithToken(ISpObjectWithToken *iface)
{
    return CONTAINING_RECORD(iface, struct ttsengine, ISpObjectWithToken_iface);
}

static HRESULT WINAPI ttsengine_QueryInterface(ISpTTSEngine *iface, REFIID iid, void **obj)
{
    struct ttsengine *This = impl_from_ISpTTSEngine(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(iid), obj);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_ISpTTSEngine))
    {
        *obj = &This->ISpTTSEngine_iface;
    }
    else if (IsEqualIID(iid, &IID_ISpObjectWithToken))
        *obj = &This->ISpObjectWithToken_iface;
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI ttsengine_AddRef(ISpTTSEngine *iface)
{
    struct ttsengine *This = impl_from_ISpTTSEngine(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI ttsengine_Release(ISpTTSEngine *iface)
{
    struct ttsengine *This = impl_from_ISpTTSEngine(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    if (!ref)
    {
        if (This->token) ISpObjectToken_Release(This->token);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ttsengine_Speak(ISpTTSEngine *iface, DWORD flags, REFGUID fmtid,
                                      const WAVEFORMATEX *wfx, const SPVTEXTFRAG *frag_list,
                                      ISpTTSEngineSite *site)
{
    FIXME("(%p, %#lx, %s, %p, %p, %p): stub.\n", iface, flags, debugstr_guid(fmtid), wfx, frag_list, site);

    return E_NOTIMPL;
}

static HRESULT WINAPI ttsengine_GetOutputFormat(ISpTTSEngine *iface, const GUID *fmtid,
                                                const WAVEFORMATEX *wfx, GUID *out_fmtid,
                                                WAVEFORMATEX **out_wfx)
{
    FIXME("(%p, %s, %p, %p, %p): stub.\n", iface, debugstr_guid(fmtid), wfx, out_fmtid, out_wfx);

    return E_NOTIMPL;
}

static ISpTTSEngineVtbl ttsengine_vtbl =
{
    ttsengine_QueryInterface,
    ttsengine_AddRef,
    ttsengine_Release,
    ttsengine_Speak,
    ttsengine_GetOutputFormat,
};

static HRESULT WINAPI objwithtoken_QueryInterface(ISpObjectWithToken *iface, REFIID iid, void **obj)
{
    struct ttsengine *This = impl_from_ISpObjectWithToken(iface);

    TRACE("(%p, %s, %p).\n", iface, debugstr_guid(iid), obj);

    return ISpTTSEngine_QueryInterface(&This->ISpTTSEngine_iface, iid, obj);
}

static ULONG WINAPI objwithtoken_AddRef(ISpObjectWithToken *iface)
{
    struct ttsengine *This = impl_from_ISpObjectWithToken(iface);

    TRACE("(%p).\n", iface);

    return ISpTTSEngine_AddRef(&This->ISpTTSEngine_iface);
}

static ULONG WINAPI objwithtoken_Release(ISpObjectWithToken *iface)
{
    struct ttsengine *This = impl_from_ISpObjectWithToken(iface);

    TRACE("(%p).\n", iface);

    return ISpTTSEngine_Release(&This->ISpTTSEngine_iface);
}

static HRESULT WINAPI objwithtoken_SetObjectToken(ISpObjectWithToken *iface, ISpObjectToken *token)
{
    struct ttsengine *This = impl_from_ISpObjectWithToken(iface);

    FIXME("(%p, %p): semi-stub.\n", iface, token);

    if (!token)
        return E_INVALIDARG;
    if (This->token)
        return SPERR_ALREADY_INITIALIZED;

    ISpObjectToken_AddRef(token);
    This->token = token;
    return S_OK;
}

static HRESULT WINAPI objwithtoken_GetObjectToken(ISpObjectWithToken *iface, ISpObjectToken **token)
{
    struct ttsengine *This = impl_from_ISpObjectWithToken(iface);

    TRACE("(%p, %p).\n", iface, token);

    if (!token)
        return E_POINTER;

    *token = This->token;
    if (*token)
    {
        ISpObjectToken_AddRef(*token);
        return S_OK;
    }
    else
        return S_FALSE;
}

static const ISpObjectWithTokenVtbl objwithtoken_vtbl =
{
    objwithtoken_QueryInterface,
    objwithtoken_AddRef,
    objwithtoken_Release,
    objwithtoken_SetObjectToken,
    objwithtoken_GetObjectToken
};

HRESULT ttsengine_create(REFIID iid, void **obj)
{
    struct ttsengine *This;
    HRESULT hr;

    if (!(This = malloc(sizeof(*This))))
        return E_OUTOFMEMORY;

    This->ISpTTSEngine_iface.lpVtbl = &ttsengine_vtbl;
    This->ISpObjectWithToken_iface.lpVtbl = &objwithtoken_vtbl;
    This->ref = 1;

    This->token = NULL;

    hr = ISpTTSEngine_QueryInterface(&This->ISpTTSEngine_iface, iid, obj);
    ISpTTSEngine_Release(&This->ISpTTSEngine_iface);
    return hr;
}
