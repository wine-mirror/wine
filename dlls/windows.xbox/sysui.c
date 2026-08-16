/* WinRT Windows.Xbox.UI.SystemUI implementation
 *
 * Stub factory: all system UI methods return S_OK so ERA games can proceed
 * past UI-availability checks.  Based on WinDurango Windows.Xbox.UI.idl (MIT).
 */

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

static const WCHAR RuntimeClass_SystemUI[] = L"Windows.Xbox.UI.SystemUI";

/* ======================================================================
 * ISystemUIStatics — all show-* methods stub as FIXME
 * ====================================================================== */
typedef struct ISystemUIStatics ISystemUIStatics;
typedef struct ISystemUIStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(ISystemUIStatics*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(ISystemUIStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(ISystemUIStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(ISystemUIStatics*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(ISystemUIStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(ISystemUIStatics*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *ShowProfileCardAsync)(ISystemUIStatics*, void* user, void** async_op);
    HRESULT (STDMETHODCALLTYPE *ShowChangeFriendRelationshipAsync)(ISystemUIStatics*, void* user, void** async_op);
    HRESULT (STDMETHODCALLTYPE *ShowTitleAchievementsAsync)(ISystemUIStatics*, UINT32 title_id, void** async_op);
    HRESULT (STDMETHODCALLTYPE *HasSyncOccurred)(ISystemUIStatics*, boolean*);
    HRESULT (STDMETHODCALLTYPE *ShowSendGameInvitesAsync)(ISystemUIStatics*, void* user, void* session_ref, void** async_op);
    HRESULT (STDMETHODCALLTYPE *ShowAchievementNotificationAsync)(ISystemUIStatics*, void* user, UINT32 title_id, HSTRING achievement_id, void** async_op);
} ISystemUIStaticsVtbl;
struct ISystemUIStatics { CONST_VTBL ISystemUIStaticsVtbl *lpVtbl; };

static const GUID IID_ISystemUIStatics =
    {0x7c2d4cf7, 0x1e8f, 0x5dd7, {0xa3, 0xb4, 0x62, 0x13, 0xd0, 0xb4, 0xde, 0x62}};

struct sysui_statics {
    IActivationFactory IActivationFactory_iface;
    ISystemUIStatics   ISystemUIStatics_iface;
    LONG ref;
};

static inline struct sysui_statics *impl_af_from_su(IActivationFactory *iface)
{ return CONTAINING_RECORD(iface, struct sysui_statics, IActivationFactory_iface); }

static inline struct sysui_statics *impl_su_from_su(ISystemUIStatics *iface)
{ return CONTAINING_RECORD(iface, struct sysui_statics, ISystemUIStatics_iface); }

/* ---- IActivationFactory ---- */
static HRESULT STDMETHODCALLTYPE su_af_QI(IActivationFactory *iface, REFIID iid, void **out)
{
    struct sysui_statics *impl = impl_af_from_su(iface);
    if (IsEqualGUID(iid, &IID_IUnknown)        ||
        IsEqualGUID(iid, &IID_IInspectable)    ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    { *out = &impl->IActivationFactory_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    if (IsEqualGUID(iid, &IID_ISystemUIStatics))
    { *out = &impl->ISystemUIStatics_iface; IUnknown_AddRef((IUnknown *)*out); return S_OK; }
    FIXME("%s not implemented\n", debugstr_guid(iid));
    *out = NULL; return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE su_af_AddRef(IActivationFactory *iface)
{ return InterlockedIncrement(&impl_af_from_su(iface)->ref); }
static ULONG STDMETHODCALLTYPE su_af_Release(IActivationFactory *iface)
{ return InterlockedDecrement(&impl_af_from_su(iface)->ref); }
static HRESULT STDMETHODCALLTYPE su_af_GetIids(IActivationFactory *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE su_af_GetRTCN(IActivationFactory *iface, HSTRING *cn)
{ return WindowsCreateString(RuntimeClass_SystemUI, wcslen(RuntimeClass_SystemUI), cn); }
static HRESULT STDMETHODCALLTYPE su_af_GetTL(IActivationFactory *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }
static HRESULT STDMETHODCALLTYPE su_af_Activate(IActivationFactory *iface, IInspectable **inst)
{ *inst = NULL; return E_NOTIMPL; }

static const IActivationFactoryVtbl su_af_vtbl =
{
    su_af_QI, su_af_AddRef, su_af_Release,
    su_af_GetIids, su_af_GetRTCN, su_af_GetTL,
    su_af_Activate,
};

/* ---- ISystemUIStatics methods ---- */
static HRESULT STDMETHODCALLTYPE su_QI(ISystemUIStatics *iface, REFIID iid, void **out)
{ return IActivationFactory_QueryInterface(&impl_su_from_su(iface)->IActivationFactory_iface, iid, out); }
static ULONG STDMETHODCALLTYPE su_AddRef(ISystemUIStatics *iface)
{ return IActivationFactory_AddRef(&impl_su_from_su(iface)->IActivationFactory_iface); }
static ULONG STDMETHODCALLTYPE su_Release(ISystemUIStatics *iface)
{ return IActivationFactory_Release(&impl_su_from_su(iface)->IActivationFactory_iface); }
static HRESULT STDMETHODCALLTYPE su_GetIids(ISystemUIStatics *iface, ULONG *n, IID **ids)
{ *n = 0; *ids = NULL; return S_OK; }
static HRESULT STDMETHODCALLTYPE su_GetRTCN(ISystemUIStatics *iface, HSTRING *cn)
{ return WindowsCreateString(RuntimeClass_SystemUI, wcslen(RuntimeClass_SystemUI), cn); }
static HRESULT STDMETHODCALLTYPE su_GetTL(ISystemUIStatics *iface, TrustLevel *tl)
{ *tl = BaseTrust; return S_OK; }

/* All show operations: FIXME-log and return a completed-null async op */
static HRESULT make_null_async(void **out)
{
    /* Re-use the async_op type from storage.c is not accessible here.
     * Return E_NOTIMPL — the game will handle missing system UI gracefully. */
    FIXME("system UI operation not implemented\n");
    *out = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE su_ShowProfileCard(ISystemUIStatics *iface, void *user, void **out)
{ return make_null_async(out); }

static HRESULT STDMETHODCALLTYPE su_ShowChangeFriendRelationship(ISystemUIStatics *iface, void *user, void **out)
{ return make_null_async(out); }

static HRESULT STDMETHODCALLTYPE su_ShowTitleAchievements(ISystemUIStatics *iface, UINT32 tid, void **out)
{ return make_null_async(out); }

static HRESULT STDMETHODCALLTYPE su_HasSyncOccurred(ISystemUIStatics *iface, boolean *value)
{ *value = TRUE; return S_OK; }

static HRESULT STDMETHODCALLTYPE su_ShowSendGameInvites(ISystemUIStatics *iface, void *user,
    void *session_ref, void **out)
{ return make_null_async(out); }

static HRESULT STDMETHODCALLTYPE su_ShowAchievementNotification(ISystemUIStatics *iface,
    void *user, UINT32 tid, HSTRING achievement_id, void **out)
{ return make_null_async(out); }

static const ISystemUIStaticsVtbl su_vtbl =
{
    su_QI, su_AddRef, su_Release,
    su_GetIids, su_GetRTCN, su_GetTL,
    su_ShowProfileCard,
    su_ShowChangeFriendRelationship,
    su_ShowTitleAchievements,
    su_HasSyncOccurred,
    su_ShowSendGameInvites,
    su_ShowAchievementNotification,
};

static struct sysui_statics sysui_statics_instance =
{
    {&su_af_vtbl},
    {&su_vtbl},
    1
};

IActivationFactory *xbox_sysui_factory = &sysui_statics_instance.IActivationFactory_iface;
