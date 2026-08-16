/* WinRT Windows.Xbox.Multiplayer stub implementation
 *
 * Stub factories for Xbox One ERA multiplayer/session APIs.
 * Based on WinDurango Windows.Xbox.Multiplayer.idl (MIT).
 */

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

static const WCHAR RC_MultiplayerManager[] = L"Windows.Xbox.Multiplayer.Manager";
static const WCHAR RC_MultiplayerSession[] = L"Windows.Xbox.Multiplayer.Session";

/* IMultiplayerManagerStatics */
typedef struct IMultiplayerManagerStatics IMultiplayerManagerStatics;
typedef struct IMultiplayerManagerStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IMultiplayerManagerStatics*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IMultiplayerManagerStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(IMultiplayerManagerStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(IMultiplayerManagerStatics*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IMultiplayerManagerStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IMultiplayerManagerStatics*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *get_IsJoinable)(IMultiplayerManagerStatics*, boolean*);
    HRESULT (STDMETHODCALLTYPE *put_IsJoinable)(IMultiplayerManagerStatics*, boolean);
    HRESULT (STDMETHODCALLTYPE *get_SessionCount)(IMultiplayerManagerStatics*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *GetSession)(IMultiplayerManagerStatics*, UINT32, void**);
    HRESULT (STDMETHODCALLTYPE *CreateSession)(IMultiplayerManagerStatics*, HSTRING, void**);
    HRESULT (STDMETHODCALLTYPE *RemoveSession)(IMultiplayerManagerStatics*, void*);
} IMultiplayerManagerStaticsVtbl;
struct IMultiplayerManagerStatics { CONST_VTBL IMultiplayerManagerStaticsVtbl *lpVtbl; };

static const GUID IID_IMultiplayerManagerStatics =
    {0xa1f3c2e7, 0x4b5d, 0x6e8f, {0x9a,0x0b,0x1c,0x2d,0x3e,0x4f,0x50,0x61}};

struct mplayer_statics {
    IActivationFactory          IActivationFactory_iface;
    IMultiplayerManagerStatics  IMultiplayerManagerStatics_iface;
    LONG ref;
};

static inline struct mplayer_statics *impl_af(IActivationFactory *iface)
{ return CONTAINING_RECORD(iface, struct mplayer_statics, IActivationFactory_iface); }
static inline struct mplayer_statics *impl_ms(IMultiplayerManagerStatics *iface)
{ return CONTAINING_RECORD(iface, struct mplayer_statics, IMultiplayerManagerStatics_iface); }

/* IActivationFactory */
static HRESULT STDMETHODCALLTYPE mp_af_QI(IActivationFactory *iface, REFIID iid, void **out)
{
    struct mplayer_statics *impl = impl_af(iface);
    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    { *out = &impl->IActivationFactory_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    if (IsEqualGUID(iid, &IID_IMultiplayerManagerStatics))
    { *out = &impl->IMultiplayerManagerStatics_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    FIXME("%s not implemented\n", debugstr_guid(iid));
    *out = NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE mp_af_AddRef(IActivationFactory *iface)
{ return InterlockedIncrement(&impl_af(iface)->ref); }
static ULONG STDMETHODCALLTYPE mp_af_Release(IActivationFactory *iface)
{ return InterlockedDecrement(&impl_af(iface)->ref); }
static HRESULT STDMETHODCALLTYPE mp_af_GetIids(IActivationFactory *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE mp_af_GetRTCN(IActivationFactory *iface, HSTRING *cn)
{ return WindowsCreateString(RC_MultiplayerManager, wcslen(RC_MultiplayerManager), cn); }
static HRESULT STDMETHODCALLTYPE mp_af_GetTL(IActivationFactory *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }
static HRESULT STDMETHODCALLTYPE mp_af_Activate(IActivationFactory *iface, IInspectable **inst)
{ *inst = NULL; return E_NOTIMPL; }

static const IActivationFactoryVtbl mp_af_vtbl =
{
    mp_af_QI, mp_af_AddRef, mp_af_Release,
    mp_af_GetIids, mp_af_GetRTCN, mp_af_GetTL,
    mp_af_Activate,
};

/* IMultiplayerManagerStatics methods */
static HRESULT STDMETHODCALLTYPE mp_QI(IMultiplayerManagerStatics *iface, REFIID iid, void **out)
{ return IActivationFactory_QueryInterface(&impl_ms(iface)->IActivationFactory_iface, iid, out); }
static ULONG STDMETHODCALLTYPE mp_AddRef(IMultiplayerManagerStatics *iface)
{ return IActivationFactory_AddRef(&impl_ms(iface)->IActivationFactory_iface); }
static ULONG STDMETHODCALLTYPE mp_Release(IMultiplayerManagerStatics *iface)
{ return IActivationFactory_Release(&impl_ms(iface)->IActivationFactory_iface); }
static HRESULT STDMETHODCALLTYPE mp_GetIids(IMultiplayerManagerStatics *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE mp_GetRTCN(IMultiplayerManagerStatics *iface, HSTRING *cn)
{ return WindowsCreateString(RC_MultiplayerManager, wcslen(RC_MultiplayerManager), cn); }
static HRESULT STDMETHODCALLTYPE mp_GetTL(IMultiplayerManagerStatics *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }

static HRESULT STDMETHODCALLTYPE mp_get_IsJoinable(IMultiplayerManagerStatics *iface, boolean *v)
{ FIXME("stub\n"); *v = FALSE; return S_OK; }
static HRESULT STDMETHODCALLTYPE mp_put_IsJoinable(IMultiplayerManagerStatics *iface, boolean v)
{ FIXME("stub\n"); return S_OK; }
static HRESULT STDMETHODCALLTYPE mp_get_SessionCount(IMultiplayerManagerStatics *iface, UINT32 *v)
{ *v = 0; return S_OK; }
static HRESULT STDMETHODCALLTYPE mp_GetSession(IMultiplayerManagerStatics *iface, UINT32 idx, void **out)
{ FIXME("stub\n"); *out = NULL; return E_BOUNDS; }
static HRESULT STDMETHODCALLTYPE mp_CreateSession(IMultiplayerManagerStatics *iface, HSTRING tmpl, void **out)
{ FIXME("stub\n"); *out = NULL; return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE mp_RemoveSession(IMultiplayerManagerStatics *iface, void *session)
{ FIXME("stub\n"); return S_OK; }

static const IMultiplayerManagerStaticsVtbl mp_vtbl =
{
    mp_QI, mp_AddRef, mp_Release,
    mp_GetIids, mp_GetRTCN, mp_GetTL,
    mp_get_IsJoinable, mp_put_IsJoinable,
    mp_get_SessionCount, mp_GetSession,
    mp_CreateSession, mp_RemoveSession,
};

static struct mplayer_statics mplayer_statics_instance =
{
    {&mp_af_vtbl},
    {&mp_vtbl},
    1
};

IActivationFactory *xbox_multiplayer_factory = &mplayer_statics_instance.IActivationFactory_iface;
