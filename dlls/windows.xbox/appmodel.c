/* WinRT Windows.Xbox.ApplicationModel.Core implementation
 *
 * Minimal stubs for ICoreApplicationContext and ICoreApplicationContextStatics
 * so ERA games can query the current view without crashing.  Based on
 * interface definitions from WinDurango (MIT).
 */

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

static const WCHAR RuntimeClass_CoreAppContext[] =
    L"Windows.Xbox.ApplicationModel.Core.CoreApplicationContext";
static const WCHAR RuntimeClass_CoreAppView[] =
    L"Windows.Xbox.ApplicationModel.Core.CoreApplicationView";

/* ======================================================================
 * ICoreApplicationView — one fake application view
 * ====================================================================== */
typedef struct ICoreApplicationView ICoreApplicationView;
typedef struct ICoreApplicationViewVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreApplicationView*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreApplicationView*);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreApplicationView*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(ICoreApplicationView*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(ICoreApplicationView*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(ICoreApplicationView*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *get_TitleId)(ICoreApplicationView*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *get_TitleName)(ICoreApplicationView*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *get_User)(ICoreApplicationView*, void**);
    HRESULT (STDMETHODCALLTYPE *get_IsMaster)(ICoreApplicationView*, boolean*);
} ICoreApplicationViewVtbl;
struct ICoreApplicationView { CONST_VTBL ICoreApplicationViewVtbl *lpVtbl; };

static const GUID IID_ICoreApplicationView_Xbox =
    {0x3f5acaf4, 0x7bed, 0x5c4e, {0x8c, 0x4d, 0x8b, 0xc3, 0xdd, 0x83, 0xe4, 0x51}};

struct cav_obj {
    ICoreApplicationView ICoreApplicationView_iface;
    LONG ref;
};

static inline struct cav_obj *impl_from_cav(ICoreApplicationView *iface)
{ return CONTAINING_RECORD(iface, struct cav_obj, ICoreApplicationView_iface); }

static HRESULT STDMETHODCALLTYPE cav_QI(ICoreApplicationView *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ICoreApplicationView_Xbox))
    { *out = iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    *out = NULL; return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE cav_AddRef(ICoreApplicationView *iface)
{ return InterlockedIncrement(&impl_from_cav(iface)->ref); }

static ULONG STDMETHODCALLTYPE cav_Release(ICoreApplicationView *iface)
{
    struct cav_obj *impl = impl_from_cav(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    if (!ref) HeapFree(GetProcessHeap(), 0, impl);
    return ref;
}

static HRESULT STDMETHODCALLTYPE cav_GetIids(ICoreApplicationView *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }

static HRESULT STDMETHODCALLTYPE cav_GetRTCN(ICoreApplicationView *iface, HSTRING *cn)
{ return WindowsCreateString(RuntimeClass_CoreAppView, wcslen(RuntimeClass_CoreAppView), cn); }

static HRESULT STDMETHODCALLTYPE cav_GetTL(ICoreApplicationView *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }

static HRESULT STDMETHODCALLTYPE cav_get_TitleId(ICoreApplicationView *iface, UINT32 *value)
{ *value = 0x00000001; return S_OK; }

static HRESULT STDMETHODCALLTYPE cav_get_TitleName(ICoreApplicationView *iface, HSTRING *value)
{ return WindowsCreateString(L"WineEX Title", 12, value); }

static HRESULT STDMETHODCALLTYPE cav_get_User(ICoreApplicationView *iface, void **value)
{
    /* Delegate to the User factory's static user */
    FIXME("(%p, %p): stub — no user\n", iface, value);
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE cav_get_IsMaster(ICoreApplicationView *iface, boolean *value)
{ *value = TRUE; return S_OK; }

static const ICoreApplicationViewVtbl cav_vtbl =
{
    cav_QI, cav_AddRef, cav_Release,
    cav_GetIids, cav_GetRTCN, cav_GetTL,
    cav_get_TitleId,
    cav_get_TitleName,
    cav_get_User,
    cav_get_IsMaster,
};

static HRESULT cav_create(ICoreApplicationView **out)
{
    struct cav_obj *impl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*impl));
    if (!impl) return E_OUTOFMEMORY;
    impl->ICoreApplicationView_iface.lpVtbl = &cav_vtbl;
    impl->ref = 1;
    *out = &impl->ICoreApplicationView_iface;
    return S_OK;
}

/* ======================================================================
 * ICoreApplicationContextStatics — static factory
 * ====================================================================== */
typedef struct ICoreApplicationContextStatics ICoreApplicationContextStatics;
typedef struct ICoreApplicationContextStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ICoreApplicationContextStatics*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ICoreApplicationContextStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(ICoreApplicationContextStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(ICoreApplicationContextStatics*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(ICoreApplicationContextStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(ICoreApplicationContextStatics*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *GetCurrentView)(ICoreApplicationContextStatics*, ICoreApplicationView**);
} ICoreApplicationContextStaticsVtbl;
struct ICoreApplicationContextStatics { CONST_VTBL ICoreApplicationContextStaticsVtbl *lpVtbl; };

static const GUID IID_ICoreApplicationContextStatics =
    {0xd7fb5f8a, 0x3c44, 0x5e4e, {0x8a, 0x3a, 0x57, 0x2b, 0x04, 0x37, 0xf4, 0xc8}};

struct appmodel_statics {
    IActivationFactory                IActivationFactory_iface;
    ICoreApplicationContextStatics   ICoreApplicationContextStatics_iface;
    LONG ref;
};

static inline struct appmodel_statics *impl_af_from_am(IActivationFactory *iface)
{ return CONTAINING_RECORD(iface, struct appmodel_statics, IActivationFactory_iface); }

static inline struct appmodel_statics *impl_cas_from_am(ICoreApplicationContextStatics *iface)
{ return CONTAINING_RECORD(iface, struct appmodel_statics, ICoreApplicationContextStatics_iface); }

static HRESULT STDMETHODCALLTYPE am_af_QI(IActivationFactory *iface, REFIID iid, void **out)
{
    struct appmodel_statics *impl = impl_af_from_am(iface);
    if (IsEqualGUID(iid, &IID_IUnknown)        ||
        IsEqualGUID(iid, &IID_IInspectable)    ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    { *out = &impl->IActivationFactory_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    if (IsEqualGUID(iid, &IID_ICoreApplicationContextStatics))
    { *out = &impl->ICoreApplicationContextStatics_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    FIXME("%s not implemented\n", debugstr_guid(iid));
    *out = NULL; return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE am_af_AddRef(IActivationFactory *iface)
{ return InterlockedIncrement(&impl_af_from_am(iface)->ref); }

static ULONG STDMETHODCALLTYPE am_af_Release(IActivationFactory *iface)
{ return InterlockedDecrement(&impl_af_from_am(iface)->ref); }

static HRESULT STDMETHODCALLTYPE am_af_GetIids(IActivationFactory *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }

static HRESULT STDMETHODCALLTYPE am_af_GetRTCN(IActivationFactory *iface, HSTRING *cn)
{ return WindowsCreateString(RuntimeClass_CoreAppContext, wcslen(RuntimeClass_CoreAppContext), cn); }

static HRESULT STDMETHODCALLTYPE am_af_GetTL(IActivationFactory *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }

static HRESULT STDMETHODCALLTYPE am_af_Activate(IActivationFactory *iface, IInspectable **inst)
{ *inst = NULL; return E_NOTIMPL; }

static const IActivationFactoryVtbl am_af_vtbl =
{
    am_af_QI, am_af_AddRef, am_af_Release,
    am_af_GetIids, am_af_GetRTCN, am_af_GetTL,
    am_af_Activate,
};

static HRESULT STDMETHODCALLTYPE cas_QI(ICoreApplicationContextStatics *iface, REFIID iid, void **out)
{
    struct appmodel_statics *impl = impl_cas_from_am(iface);
    return IActivationFactory_QueryInterface(&impl->IActivationFactory_iface, iid, out);
}
static ULONG STDMETHODCALLTYPE cas_AddRef(ICoreApplicationContextStatics *iface)
{ return IActivationFactory_AddRef(&impl_cas_from_am(iface)->IActivationFactory_iface); }
static ULONG STDMETHODCALLTYPE cas_Release(ICoreApplicationContextStatics *iface)
{ return IActivationFactory_Release(&impl_cas_from_am(iface)->IActivationFactory_iface); }
static HRESULT STDMETHODCALLTYPE cas_GetIids(ICoreApplicationContextStatics *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE cas_GetRTCN(ICoreApplicationContextStatics *iface, HSTRING *cn)
{ return WindowsCreateString(RuntimeClass_CoreAppContext, wcslen(RuntimeClass_CoreAppContext), cn); }
static HRESULT STDMETHODCALLTYPE cas_GetTL(ICoreApplicationContextStatics *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }

static HRESULT STDMETHODCALLTYPE cas_GetCurrentView(ICoreApplicationContextStatics *iface,
    ICoreApplicationView **out)
{
    TRACE("(%p, %p)\n", iface, out);
    return cav_create(out);
}

static const ICoreApplicationContextStaticsVtbl cas_vtbl =
{
    cas_QI, cas_AddRef, cas_Release,
    cas_GetIids, cas_GetRTCN, cas_GetTL,
    cas_GetCurrentView,
};

static struct appmodel_statics appmodel_statics_instance =
{
    {&am_af_vtbl},
    {&cas_vtbl},
    1
};

IActivationFactory *xbox_appmodel_core_factory =
    &appmodel_statics_instance.IActivationFactory_iface;
