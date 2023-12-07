/*              DirectShow Base Functions (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
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

#include "wine/debug.h"

#include "quartz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

extern HRESULT WINAPI QUARTZ_DllGetClassObject(REFCLSID, REFIID, LPVOID *);
extern BOOL WINAPI QUARTZ_DllMain(HINSTANCE, DWORD, LPVOID);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)
{
    if (reason == DLL_PROCESS_DETACH && !reserved)
    {
        video_window_unregister_class();
        strmbase_release_typelibs();
    }
    return QUARTZ_DllMain(instance, reason, reserved);
}

/******************************************************************************
 * DirectShow ClassFactory
 */
typedef struct {
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

struct object_creation_info
{
    const CLSID *clsid;
    HRESULT (*create_instance)(IUnknown *outer, IUnknown **out);
};

static const struct object_creation_info object_creation[] =
{
    { &CLSID_ACMWrapper, acm_wrapper_create },
    { &CLSID_AllocPresenter, vmr7_presenter_create },
    { &CLSID_AsyncReader, async_reader_create },
    { &CLSID_AudioRender, dsound_render_create },
    { &CLSID_AVIDec, avi_dec_create },
    { &CLSID_DSoundRender, dsound_render_create },
    { &CLSID_FilterGraph, filter_graph_create },
    { &CLSID_FilterGraphNoThread, filter_graph_no_thread_create },
    { &CLSID_FilterMapper, filter_mapper_create },
    { &CLSID_FilterMapper2, filter_mapper_create },
    { &CLSID_MemoryAllocator, mem_allocator_create },
    { &CLSID_SeekingPassThru, seeking_passthrough_create },
    { &CLSID_SystemClock, system_clock_create },
    { &CLSID_VideoRenderer, video_renderer_create },
    { &CLSID_VideoMixingRenderer, vmr7_create },
    { &CLSID_VideoMixingRenderer9, vmr9_create },
    { &CLSID_VideoRendererDefault, video_renderer_default_create },
};

static HRESULT WINAPI DSCF_QueryInterface(IClassFactory *iface, REFIID riid, void **ppobj)
{
    if (IsEqualGUID(riid, &IID_IUnknown)
	|| IsEqualGUID(riid, &IID_IClassFactory))
    {
	IClassFactory_AddRef(iface);
        *ppobj = iface;
	return S_OK;
    }

    *ppobj = NULL;
    WARN("(%p)->(%s,%p), not found\n", iface, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI DSCF_AddRef(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI DSCF_Release(IClassFactory *iface)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
	CoTaskMemFree(This);

    return ref;
}


static HRESULT WINAPI DSCF_CreateInstance(IClassFactory *iface, IUnknown *pOuter,
                                          REFIID riid, void **ppobj)
{
    IClassFactoryImpl *This = impl_from_IClassFactory(iface);
    HRESULT hres;
    LPUNKNOWN punk;

    TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);

    *ppobj = NULL;
    if(pOuter && !IsEqualGUID(&IID_IUnknown, riid))
        return E_NOINTERFACE;

    if (SUCCEEDED(hres = This->create_instance(pOuter, &punk)))
    {
        hres = IUnknown_QueryInterface(punk, riid, ppobj);
        IUnknown_Release(punk);
    }
    return hres;
}

static HRESULT WINAPI DSCF_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("iface %p, lock %d, stub!\n", iface, lock);
    return S_OK;
}

static const IClassFactoryVtbl DSCF_Vtbl =
{
    DSCF_QueryInterface,
    DSCF_AddRef,
    DSCF_Release,
    DSCF_CreateInstance,
    DSCF_LockServer
};

/*******************************************************************************
 * DllGetClassObject [QUARTZ.@]
 * Retrieves class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    unsigned int i;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (IsEqualGUID( &IID_IClassFactory, riid ) || IsEqualGUID( &IID_IUnknown, riid))
    {
        for (i = 0; i < ARRAY_SIZE(object_creation); i++)
        {
            if (IsEqualGUID(object_creation[i].clsid, rclsid))
            {
                IClassFactoryImpl *factory = CoTaskMemAlloc(sizeof(*factory));
                if (factory == NULL) return E_OUTOFMEMORY;

                factory->IClassFactory_iface.lpVtbl = &DSCF_Vtbl;
                factory->ref = 1;

                factory->create_instance = object_creation[i].create_instance;

                *ppv = &factory->IClassFactory_iface;
                return S_OK;
            }
        }
    }
    return QUARTZ_DllGetClassObject( rclsid, riid, ppv );
}

#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } , #name },

static const struct {
	const GUID	riid;
	const char 	*name;
} InterfaceDesc[] =
{
#include "uuids.h"
    { { 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} }, NULL }
};

/***********************************************************************
 *              proxies
 */
HRESULT CALLBACK ICaptureGraphBuilder_FindInterface_Proxy( ICaptureGraphBuilder *This,
                                                           const GUID *pCategory,
                                                           IBaseFilter *pf,
                                                           REFIID riid,
                                                           void **ppint )
{
    return ICaptureGraphBuilder_RemoteFindInterface_Proxy( This, pCategory, pf,
                                                           riid, (IUnknown **)ppint );
}

HRESULT __RPC_STUB ICaptureGraphBuilder_FindInterface_Stub( ICaptureGraphBuilder *This,
                                                            const GUID *pCategory,
                                                            IBaseFilter *pf,
                                                            REFIID riid,
                                                            IUnknown **ppint )
{
    return ICaptureGraphBuilder_FindInterface( This, pCategory, pf, riid, (void **)ppint );
}

HRESULT CALLBACK ICaptureGraphBuilder2_FindInterface_Proxy( ICaptureGraphBuilder2 *This,
                                                            const GUID *pCategory,
                                                            const GUID *pType,
                                                            IBaseFilter *pf,
                                                            REFIID riid,
                                                            void **ppint )
{
    return ICaptureGraphBuilder2_RemoteFindInterface_Proxy( This, pCategory, pType,
                                                            pf, riid, (IUnknown **)ppint );
}

HRESULT __RPC_STUB ICaptureGraphBuilder2_FindInterface_Stub( ICaptureGraphBuilder2 *This,
                                                             const GUID *pCategory,
                                                             const GUID *pType,
                                                             IBaseFilter *pf,
                                                             REFIID riid,
                                                             IUnknown **ppint )
{
    return ICaptureGraphBuilder2_FindInterface( This, pCategory, pType, pf, riid, (void **)ppint );
}

/***********************************************************************
 *              qzdebugstr_guid (internal)
 *
 * Gives a text version of DirectShow GUIDs
 */
const char * qzdebugstr_guid( const GUID * id )
{
    int i;

    for (i=0; InterfaceDesc[i].name; i++)
        if (IsEqualGUID(&InterfaceDesc[i].riid, id)) return InterfaceDesc[i].name;

    return debugstr_guid(id);
}

int WINAPI AmpFactorToDB(int ampfactor)
{
    FIXME("(%d) Stub!\n", ampfactor);
    return 0;
}

int WINAPI DBToAmpFactor(int db)
{
    FIXME("(%d) Stub!\n", db);
    /* Avoid divide by zero (probably during range computation) in Windows Media Player 6.4 */
    if (db < -1000)
	return 0;
    return 100;
}

/***********************************************************************
 *              AMGetErrorTextA (QUARTZ.@)
 */
DWORD WINAPI AMGetErrorTextA(HRESULT hr, LPSTR buffer, DWORD maxlen)
{
    DWORD res;
    WCHAR errorW[MAX_ERROR_TEXT_LEN];

    TRACE("hr %#lx, buffer %p, maxlen %lu.\n", hr, buffer, maxlen);

    if (!buffer)
        return 0;

    res = AMGetErrorTextW(hr, errorW, ARRAY_SIZE(errorW));
    if (!res)
        return 0;

    res = WideCharToMultiByte(CP_ACP, 0, errorW, -1, NULL, 0, 0, 0);
    if (res > maxlen || !res)
        return 0;
    return WideCharToMultiByte(CP_ACP, 0, errorW, -1, buffer, maxlen, 0, 0) - 1;
}

/***********************************************************************
 *              AMGetErrorTextW (QUARTZ.@)
 */
DWORD WINAPI AMGetErrorTextW(HRESULT hr, LPWSTR buffer, DWORD maxlen)
{
    unsigned int len;
    WCHAR error[MAX_ERROR_TEXT_LEN];

    TRACE("hr %#lx, buffer %p, maxlen %lu.\n", hr, buffer, maxlen);

    if (!buffer) return 0;
    swprintf(error, ARRAY_SIZE(error), L"Error: 0x%lx", hr);
    if ((len = wcslen(error)) >= maxlen)
        return 0;
    wcscpy(buffer, error);
    return len;
}
