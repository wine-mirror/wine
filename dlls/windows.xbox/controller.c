/* WinRT Windows.Xbox.Input.Controller implementation
 *
 * Backs IControllerStatics::Controllers with the current set of Xbox
 * controllers from the WGI backend (same physical controllers as Gamepad,
 * exposed under the more general IController interface that games use when
 * they don't need Gamepad-specific reading).
 *
 * Interface GUIDs are taken from WinDurango idl/Windows.Xbox.Input.idl
 * (MIT licensed).
 */

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

/* ======================================================================
 * IController — one controller object per physical gamepad
 * ====================================================================== */
struct controller
{
    IController IController_iface;
    LONG ref;
    UINT64 id;
};

static inline struct controller *impl_from_IController( IController *iface )
{
    return CONTAINING_RECORD( iface, struct controller, IController_iface );
}

static HRESULT STDMETHODCALLTYPE controller_QueryInterface( IController *iface, REFIID iid, void **out )
{
    struct controller *impl = impl_from_IController( iface );
    if (IsEqualGUID( iid, &IID_IUnknown )     ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IController ))
    {
        *out = &impl->IController_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }
    FIXME("%s not implemented\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE controller_AddRef( IController *iface )
{
    struct controller *impl = impl_from_IController( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE controller_Release( IController *iface )
{
    struct controller *impl = impl_from_IController( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    if (!ref) HeapFree( GetProcessHeap(), 0, impl );
    return ref;
}

static HRESULT STDMETHODCALLTYPE controller_GetIids( IController *iface, ULONG *count, IID **iids )
{
    *count = 0; *iids = NULL; return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_GetRuntimeClassName( IController *iface, HSTRING *name )
{
    return WindowsCreateString( RuntimeClass_Windows_Xbox_Input_Controller,
                                wcslen(RuntimeClass_Windows_Xbox_Input_Controller), name );
}

static HRESULT STDMETHODCALLTYPE controller_GetTrustLevel( IController *iface, TrustLevel *level )
{
    *level = BaseTrust; return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_get_Type( IController *iface, HSTRING *value )
{
    return WindowsCreateString( L"Windows.Xbox.Input.Gamepad", 26, value );
}

static HRESULT STDMETHODCALLTYPE controller_get_Id( IController *iface, UINT64 *value )
{
    struct controller *impl = impl_from_IController( iface );
    *value = impl->id;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_get_IsWireless( IController *iface, boolean *value )
{
    *value = TRUE;
    return S_OK;
}

static const IControllerVtbl controller_vtbl =
{
    controller_QueryInterface,
    controller_AddRef,
    controller_Release,
    controller_GetIids,
    controller_GetRuntimeClassName,
    controller_GetTrustLevel,
    controller_get_Type,
    controller_get_Id,
    controller_get_IsWireless,
};

static HRESULT controller_create( UINT64 id, IController **out )
{
    struct controller *impl;
    if (!(impl = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*impl) )))
        return E_OUTOFMEMORY;
    impl->IController_iface.lpVtbl = &controller_vtbl;
    impl->ref = 1;
    impl->id  = id;
    *out = &impl->IController_iface;
    return S_OK;
}

/* ======================================================================
 * IControllerStatics — static factory for Controller
 * ====================================================================== */

static const GUID IID_IVectorView_IController =
    {0x31a4c3dd,0x3e5e,0x5b78,{0x8b,0x8a,0x6b,0xb8,0xa2,0x5f,0x8b,0xa0}};
static const GUID IID_IIterable_IController =
    {0xd174cc0b,0xe34a,0x5c96,{0xb0,0x27,0x49,0x49,0x91,0xf5,0x0c,0xeb}};
static const GUID IID_IIterator_IController =
    {0x9d9df6fb,0xddaf,0x5438,{0x8b,0x87,0xb1,0x97,0x0d,0xe7,0x7c,0xa7}};

static const struct vector_view_iids controller_vector_iids =
{
    &IID_IVectorView_IController,
    &IID_IIterable_IController,
    &IID_IIterator_IController,
};

struct controller_statics
{
    IActivationFactory IActivationFactory_iface;
    IControllerStatics IControllerStatics_iface;
    LONG ref;
};

static inline struct controller_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct controller_statics, IActivationFactory_iface );
}

static inline struct controller_statics *impl_from_IControllerStatics( IControllerStatics *iface )
{
    return CONTAINING_RECORD( iface, struct controller_statics, IControllerStatics_iface );
}

static HRESULT STDMETHODCALLTYPE controller_statics_af_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct controller_statics *impl = impl_from_IActivationFactory( iface );
    if (IsEqualGUID( iid, &IID_IUnknown )           ||
        IsEqualGUID( iid, &IID_IInspectable )        ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }
    if (IsEqualGUID( iid, &IID_IControllerStatics ))
    {
        *out = &impl->IControllerStatics_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }
    FIXME( "%s not implemented\n", debugstr_guid(iid) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE controller_statics_af_AddRef( IActivationFactory *iface )
{
    struct controller_statics *impl = impl_from_IActivationFactory( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE controller_statics_af_Release( IActivationFactory *iface )
{
    struct controller_statics *impl = impl_from_IActivationFactory( iface );
    return InterlockedDecrement( &impl->ref );
}

static HRESULT STDMETHODCALLTYPE controller_statics_af_GetIids( IActivationFactory *iface, ULONG *n, IID **ids )
{
    *n = 0; *ids = NULL; return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_statics_af_GetRuntimeClassName( IActivationFactory *iface, HSTRING *cn )
{
    return WindowsCreateString( RuntimeClass_Windows_Xbox_Input_Controller,
                                wcslen(RuntimeClass_Windows_Xbox_Input_Controller), cn );
}

static HRESULT STDMETHODCALLTYPE controller_statics_af_GetTrustLevel( IActivationFactory *iface, TrustLevel *tl )
{
    *tl = BaseTrust; return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_statics_af_ActivateInstance( IActivationFactory *iface, IInspectable **inst )
{
    *inst = NULL; return E_NOTIMPL;
}

static const IActivationFactoryVtbl controller_af_vtbl =
{
    controller_statics_af_QueryInterface,
    controller_statics_af_AddRef,
    controller_statics_af_Release,
    controller_statics_af_GetIids,
    controller_statics_af_GetRuntimeClassName,
    controller_statics_af_GetTrustLevel,
    controller_statics_af_ActivateInstance,
};

static HRESULT STDMETHODCALLTYPE controller_statics_QueryInterface( IControllerStatics *iface, REFIID iid, void **out )
{
    struct controller_statics *impl = impl_from_IControllerStatics( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG STDMETHODCALLTYPE controller_statics_AddRef( IControllerStatics *iface )
{
    struct controller_statics *impl = impl_from_IControllerStatics( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG STDMETHODCALLTYPE controller_statics_Release( IControllerStatics *iface )
{
    struct controller_statics *impl = impl_from_IControllerStatics( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT STDMETHODCALLTYPE controller_statics_GetIids( IControllerStatics *iface, ULONG *n, IID **ids )
{
    *n = 0; *ids = NULL; return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_statics_GetRTCN( IControllerStatics *iface, HSTRING *cn )
{
    return WindowsCreateString( RuntimeClass_Windows_Xbox_Input_Controller,
                                wcslen(RuntimeClass_Windows_Xbox_Input_Controller), cn );
}

static HRESULT STDMETHODCALLTYPE controller_statics_GetTL( IControllerStatics *iface, TrustLevel *tl )
{
    *tl = BaseTrust; return S_OK;
}

static HRESULT STDMETHODCALLTYPE controller_statics_get_Controllers( IControllerStatics *iface, void **value )
{
    void **handles;
    UINT32 count, i;
    IInspectable **elems;
    HRESULT hr;

    TRACE( "(%p, %p)\n", iface, value );

    hr = wgi_backend_get_gamepads( &handles, &count );
    if (FAILED(hr)) return hr;

    if (!count)
    {
        hr = vector_view_create( &controller_vector_iids, NULL, 0, value );
        free( handles );
        return hr;
    }

    elems = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*elems) );
    if (!elems) { for (i = 0; i < count; i++) wgi_backend_release( handles[i] ); free( handles ); return E_OUTOFMEMORY; }

    for (i = 0; i < count; i++)
    {
        IController *ctrl;
        hr = controller_create( (UINT64)(ULONG_PTR)handles[i], &ctrl );
        if (FAILED(hr))
        {
            UINT32 j;
            for (j = 0; j < i; j++) IInspectable_Release( elems[j] );
            HeapFree( GetProcessHeap(), 0, elems );
            for (j = 0; j < count; j++) wgi_backend_release( handles[j] );
            free( handles );
            return hr;
        }
        elems[i] = (IInspectable *)ctrl;
    }

    for (i = 0; i < count; i++) wgi_backend_release( handles[i] );
    free( handles );

    hr = vector_view_create( &controller_vector_iids, elems, count, value );
    for (i = 0; i < count; i++) IInspectable_Release( elems[i] );
    HeapFree( GetProcessHeap(), 0, elems );
    return hr;
}

static const IControllerStaticsVtbl controller_statics_vtbl =
{
    controller_statics_QueryInterface,
    controller_statics_AddRef,
    controller_statics_Release,
    controller_statics_GetIids,
    controller_statics_GetRTCN,
    controller_statics_GetTL,
    controller_statics_get_Controllers,
};

static struct controller_statics controller_statics_instance =
{
    {&controller_af_vtbl},
    {&controller_statics_vtbl},
    1
};

IActivationFactory *xbox_controller_factory = &controller_statics_instance.IActivationFactory_iface;
