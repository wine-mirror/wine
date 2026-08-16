/* WinRT Windows.Xbox.Networking stub implementation
 *
 * Stub factories for Xbox One ERA secure networking / QoS APIs.
 * Based on WinDurango Windows.Xbox.Networking.idl (MIT).
 */

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

static const WCHAR RC_ManagedNet[] = L"Windows.Xbox.Networking.ManagedNetwork";
static const WCHAR RC_SecureDevice[] = L"Windows.Xbox.Networking.SecureDeviceAssociation";

/* IManagedNetworkStatics */
typedef struct IManagedNetworkStatics IManagedNetworkStatics;
typedef struct IManagedNetworkStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IManagedNetworkStatics*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IManagedNetworkStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(IManagedNetworkStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(IManagedNetworkStatics*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IManagedNetworkStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IManagedNetworkStatics*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *SetMaxLocalUdpMulticastDatagramSize)(IManagedNetworkStatics*, UINT32);
    HRESULT (STDMETHODCALLTYPE *get_MaxLocalUdpMulticastDatagramSize)(IManagedNetworkStatics*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *StartAdvertising)(IManagedNetworkStatics*, void* tmpl, void** async_op);
    HRESULT (STDMETHODCALLTYPE *StopAdvertising)(IManagedNetworkStatics*);
    HRESULT (STDMETHODCALLTYPE *FindAssociationTemplateAsync)(IManagedNetworkStatics*, HSTRING name, void** async_op);
} IManagedNetworkStaticsVtbl;
struct IManagedNetworkStatics { CONST_VTBL IManagedNetworkStaticsVtbl *lpVtbl; };

static const GUID IID_IManagedNetworkStatics =
    {0xb2d4e8f1, 0x5c3a, 0x7f9e, {0x0b,0x1c,0x2d,0x3e,0x4f,0x50,0x61,0x72}};

/* ISecureDeviceAssociationStatics */
typedef struct ISecureDeviceAssocStatics ISecureDeviceAssocStatics;
typedef struct ISecureDeviceAssocStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ISecureDeviceAssocStatics*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ISecureDeviceAssocStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(ISecureDeviceAssocStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(ISecureDeviceAssocStatics*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(ISecureDeviceAssocStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(ISecureDeviceAssocStatics*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *get_Count)(ISecureDeviceAssocStatics*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *GetAt)(ISecureDeviceAssocStatics*, UINT32, void**);
    HRESULT (STDMETHODCALLTYPE *CreateAsync)(ISecureDeviceAssocStatics*, void* tmpl, void* addr, void** async_op);
} ISecureDeviceAssocStaticsVtbl;
struct ISecureDeviceAssocStatics { CONST_VTBL ISecureDeviceAssocStaticsVtbl *lpVtbl; };

static const GUID IID_ISecureDeviceAssocStatics =
    {0xc3e5f9a2, 0x6d4b, 0x8e0f, {0x1b,0x2c,0x3d,0x4e,0x5f,0x60,0x71,0x82}};

struct net_statics {
    IActivationFactory        IActivationFactory_iface;
    IManagedNetworkStatics    IManagedNetworkStatics_iface;
    ISecureDeviceAssocStatics ISecureDeviceAssocStatics_iface;
    LONG ref;
};

static inline struct net_statics *impl_af(IActivationFactory *iface)
{ return CONTAINING_RECORD(iface, struct net_statics, IActivationFactory_iface); }
static inline struct net_statics *impl_mn(IManagedNetworkStatics *iface)
{ return CONTAINING_RECORD(iface, struct net_statics, IManagedNetworkStatics_iface); }
static inline struct net_statics *impl_sd(ISecureDeviceAssocStatics *iface)
{ return CONTAINING_RECORD(iface, struct net_statics, ISecureDeviceAssocStatics_iface); }

/* IActivationFactory */
static HRESULT STDMETHODCALLTYPE net_af_QI(IActivationFactory *iface, REFIID iid, void **out)
{
    struct net_statics *impl = impl_af(iface);
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    { *out = &impl->IActivationFactory_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    if (IsEqualGUID(iid, &IID_IManagedNetworkStatics))
    { *out = &impl->IManagedNetworkStatics_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    if (IsEqualGUID(iid, &IID_ISecureDeviceAssocStatics))
    { *out = &impl->ISecureDeviceAssocStatics_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    FIXME("%s not implemented\n", debugstr_guid(iid));
    *out = NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE net_af_AddRef(IActivationFactory *iface)
{ return InterlockedIncrement(&impl_af(iface)->ref); }
static ULONG STDMETHODCALLTYPE net_af_Release(IActivationFactory *iface)
{ return InterlockedDecrement(&impl_af(iface)->ref); }
static HRESULT STDMETHODCALLTYPE net_af_GetIids(IActivationFactory *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE net_af_GetRTCN(IActivationFactory *iface, HSTRING *cn)
{ return WindowsCreateString(RC_ManagedNet, wcslen(RC_ManagedNet), cn); }
static HRESULT STDMETHODCALLTYPE net_af_GetTL(IActivationFactory *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }
static HRESULT STDMETHODCALLTYPE net_af_Activate(IActivationFactory *iface, IInspectable **inst)
{ *inst = NULL; return E_NOTIMPL; }

static const IActivationFactoryVtbl net_af_vtbl =
{
    net_af_QI, net_af_AddRef, net_af_Release,
    net_af_GetIids, net_af_GetRTCN, net_af_GetTL,
    net_af_Activate,
};

/* IManagedNetworkStatics methods */
static HRESULT STDMETHODCALLTYPE mn_QI(IManagedNetworkStatics *iface, REFIID iid, void **out)
{ return IActivationFactory_QueryInterface(&impl_mn(iface)->IActivationFactory_iface, iid, out); }
static ULONG STDMETHODCALLTYPE mn_AddRef(IManagedNetworkStatics *iface)
{ return IActivationFactory_AddRef(&impl_mn(iface)->IActivationFactory_iface); }
static ULONG STDMETHODCALLTYPE mn_Release(IManagedNetworkStatics *iface)
{ return IActivationFactory_Release(&impl_mn(iface)->IActivationFactory_iface); }
static HRESULT STDMETHODCALLTYPE mn_GetIids(IManagedNetworkStatics *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE mn_GetRTCN(IManagedNetworkStatics *iface, HSTRING *cn)
{ return WindowsCreateString(RC_ManagedNet, wcslen(RC_ManagedNet), cn); }
static HRESULT STDMETHODCALLTYPE mn_GetTL(IManagedNetworkStatics *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }

static HRESULT STDMETHODCALLTYPE mn_SetMaxSize(IManagedNetworkStatics *iface, UINT32 sz)
{ FIXME("stub\n"); return S_OK; }
static HRESULT STDMETHODCALLTYPE mn_GetMaxSize(IManagedNetworkStatics *iface, UINT32 *sz)
{ FIXME("stub\n"); *sz = 1400; return S_OK; }
static HRESULT STDMETHODCALLTYPE mn_StartAdvertising(IManagedNetworkStatics *iface, void *tmpl, void **out)
{ FIXME("stub\n"); *out = NULL; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE mn_StopAdvertising(IManagedNetworkStatics *iface)
{ FIXME("stub\n"); return S_OK; }
static HRESULT STDMETHODCALLTYPE mn_FindAssocTemplateAsync(IManagedNetworkStatics *iface, HSTRING name, void **out)
{ FIXME("stub\n"); *out = NULL; return E_NOTIMPL; }

static const IManagedNetworkStaticsVtbl mn_vtbl =
{
    mn_QI, mn_AddRef, mn_Release,
    mn_GetIids, mn_GetRTCN, mn_GetTL,
    mn_SetMaxSize, mn_GetMaxSize,
    mn_StartAdvertising, mn_StopAdvertising,
    mn_FindAssocTemplateAsync,
};

/* ISecureDeviceAssocStatics methods */
static HRESULT STDMETHODCALLTYPE sd_QI(ISecureDeviceAssocStatics *iface, REFIID iid, void **out)
{ return IActivationFactory_QueryInterface(&impl_sd(iface)->IActivationFactory_iface, iid, out); }
static ULONG STDMETHODCALLTYPE sd_AddRef(ISecureDeviceAssocStatics *iface)
{ return IActivationFactory_AddRef(&impl_sd(iface)->IActivationFactory_iface); }
static ULONG STDMETHODCALLTYPE sd_Release(ISecureDeviceAssocStatics *iface)
{ return IActivationFactory_Release(&impl_sd(iface)->IActivationFactory_iface); }
static HRESULT STDMETHODCALLTYPE sd_GetIids(ISecureDeviceAssocStatics *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE sd_GetRTCN(ISecureDeviceAssocStatics *iface, HSTRING *cn)
{ return WindowsCreateString(RC_SecureDevice, wcslen(RC_SecureDevice), cn); }
static HRESULT STDMETHODCALLTYPE sd_GetTL(ISecureDeviceAssocStatics *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }

static HRESULT STDMETHODCALLTYPE sd_get_Count(ISecureDeviceAssocStatics *iface, UINT32 *v)
{ *v = 0; return S_OK; }
static HRESULT STDMETHODCALLTYPE sd_GetAt(ISecureDeviceAssocStatics *iface, UINT32 idx, void **out)
{ FIXME("stub\n"); *out = NULL; return E_BOUNDS; }
static HRESULT STDMETHODCALLTYPE sd_CreateAsync(ISecureDeviceAssocStatics *iface, void *tmpl, void *addr, void **out)
{ FIXME("stub\n"); *out = NULL; return E_NOTIMPL; }

static const ISecureDeviceAssocStaticsVtbl sd_vtbl =
{
    sd_QI, sd_AddRef, sd_Release,
    sd_GetIids, sd_GetRTCN, sd_GetTL,
    sd_get_Count, sd_GetAt, sd_CreateAsync,
};

static struct net_statics net_statics_instance =
{
    {&net_af_vtbl},
    {&mn_vtbl},
    {&sd_vtbl},
    1
};

IActivationFactory *xbox_networking_factory = &net_statics_instance.IActivationFactory_iface;
