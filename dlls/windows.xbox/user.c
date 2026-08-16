/* WinRT Windows.Xbox.System.User implementation
 *
 * Provides a minimal single signed-in user so ERA games can proceed past
 * user-presence checks. Based on interface definitions from WinDurango
 * (MIT licensed) and xodus-gaming/XWine1.
 *
 * GUIDs: taken from WinDurango Windows.Xbox.System.idl (MIT).
 */

#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

/* ======================================================================
 * IUser — one user object (fake, always signed in)
 * ====================================================================== */

typedef struct IUser IUser;
typedef struct IUserVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IUser*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IUser*);
    ULONG   (STDMETHODCALLTYPE *Release)(IUser*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(IUser*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IUser*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IUser*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *get_Id)(IUser*, UINT32*);
    HRESULT (STDMETHODCALLTYPE *get_NonRoamableId)(IUser*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *get_IsSignedIn)(IUser*, boolean*);
    HRESULT (STDMETHODCALLTYPE *get_IsGuest)(IUser*, boolean*);
} IUserVtbl;
struct IUser { CONST_VTBL IUserVtbl *lpVtbl; };

static const WCHAR RuntimeClass_Windows_Xbox_System_User[] = L"Windows.Xbox.System.User";

static const GUID IID_IVectorView_IUser =
    {0x6fb6a997,0x2843,0x59d7,{0x9a,0x07,0x4e,0x19,0x97,0x5e,0x03,0x7e}};
static const GUID IID_IIterable_IUser =
    {0x17a4c4dc,0x6b4f,0x5cd4,{0x8c,0x08,0x1a,0x6b,0x8c,0x5f,0xa3,0xc6}};
static const GUID IID_IIterator_IUser =
    {0x8d5e1a38,0x7c78,0x5b3e,{0x8e,0xca,0xb8,0xb5,0x73,0xca,0xca,0xa1}};

static const struct vector_view_iids user_vector_iids =
{
    &IID_IVectorView_IUser,
    &IID_IIterable_IUser,
    &IID_IIterator_IUser,
};

struct user_object
{
    IUser IUser_iface;
    LONG ref;
};

static inline struct user_object *impl_from_IUser( IUser *iface )
{
    return CONTAINING_RECORD( iface, struct user_object, IUser_iface );
}

static HRESULT STDMETHODCALLTYPE user_QueryInterface( IUser *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IUnknown )     ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IUser ))
    {
        *out = iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }
    FIXME("%s not implemented\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE user_AddRef( IUser *iface )
{
    struct user_object *impl = impl_from_IUser( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE user_Release( IUser *iface )
{
    struct user_object *impl = impl_from_IUser( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    if (!ref) HeapFree( GetProcessHeap(), 0, impl );
    return ref;
}

static HRESULT STDMETHODCALLTYPE user_GetIids( IUser *iface, ULONG *count, IID **iids )
{
    *count = 0; *iids = NULL; return S_OK;
}

static HRESULT STDMETHODCALLTYPE user_GetRuntimeClassName( IUser *iface, HSTRING *name )
{
    return WindowsCreateString( RuntimeClass_Windows_Xbox_System_User,
                                wcslen(RuntimeClass_Windows_Xbox_System_User), name );
}

static HRESULT STDMETHODCALLTYPE user_GetTrustLevel( IUser *iface, TrustLevel *level )
{
    *level = BaseTrust; return S_OK;
}

static HRESULT STDMETHODCALLTYPE user_get_Id( IUser *iface, UINT32 *value )
{
    *value = 1; return S_OK;
}

static HRESULT STDMETHODCALLTYPE user_get_NonRoamableId( IUser *iface, HSTRING *value )
{
    /* Fake XUID in string form */
    return WindowsCreateString( L"0000000000000001", 16, value );
}

static HRESULT STDMETHODCALLTYPE user_get_IsSignedIn( IUser *iface, boolean *value )
{
    *value = TRUE; return S_OK;
}

static HRESULT STDMETHODCALLTYPE user_get_IsGuest( IUser *iface, boolean *value )
{
    *value = FALSE; return S_OK;
}

static const IUserVtbl user_vtbl =
{
    user_QueryInterface,
    user_AddRef,
    user_Release,
    user_GetIids,
    user_GetRuntimeClassName,
    user_GetTrustLevel,
    user_get_Id,
    user_get_NonRoamableId,
    user_get_IsSignedIn,
    user_get_IsGuest,
};

static HRESULT user_create( IUser **out )
{
    struct user_object *impl;
    if (!(impl = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*impl) )))
        return E_OUTOFMEMORY;
    impl->IUser_iface.lpVtbl = &user_vtbl;
    impl->ref = 1;
    *out = &impl->IUser_iface;
    return S_OK;
}

/* ======================================================================
 * IUserStatics — static factory for User
 * ====================================================================== */

typedef struct IUserStatics IUserStatics;
typedef struct IUserStaticsVtbl {
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(IUserStatics*, REFIID, void**);
    ULONG   (STDMETHODCALLTYPE *AddRef)(IUserStatics*);
    ULONG   (STDMETHODCALLTYPE *Release)(IUserStatics*);
    HRESULT (STDMETHODCALLTYPE *GetIids)(IUserStatics*, ULONG*, IID**);
    HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IUserStatics*, HSTRING*);
    HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IUserStatics*, TrustLevel*);
    HRESULT (STDMETHODCALLTYPE *get_Users)(IUserStatics*, void**);
    HRESULT (STDMETHODCALLTYPE *add_UserAdded)(IUserStatics*, void*, EventRegistrationToken*);
    HRESULT (STDMETHODCALLTYPE *remove_UserAdded)(IUserStatics*, EventRegistrationToken);
    HRESULT (STDMETHODCALLTYPE *add_UserRemoved)(IUserStatics*, void*, EventRegistrationToken*);
    HRESULT (STDMETHODCALLTYPE *remove_UserRemoved)(IUserStatics*, EventRegistrationToken);
    HRESULT (STDMETHODCALLTYPE *add_SignInCompleted)(IUserStatics*, void*, EventRegistrationToken*);
    HRESULT (STDMETHODCALLTYPE *remove_SignInCompleted)(IUserStatics*, EventRegistrationToken);
    HRESULT (STDMETHODCALLTYPE *add_SignOutStarted)(IUserStatics*, void*, EventRegistrationToken*);
    HRESULT (STDMETHODCALLTYPE *remove_SignOutStarted)(IUserStatics*, EventRegistrationToken);
    HRESULT (STDMETHODCALLTYPE *add_SignOutCompleted)(IUserStatics*, void*, EventRegistrationToken*);
    HRESULT (STDMETHODCALLTYPE *remove_SignOutCompleted)(IUserStatics*, EventRegistrationToken);
} IUserStaticsVtbl;
struct IUserStatics { CONST_VTBL IUserStaticsVtbl *lpVtbl; };

struct user_statics
{
    IActivationFactory IActivationFactory_iface;
    IUserStatics IUserStatics_iface;
    LONG ref;
};

static inline struct user_statics *impl_af_from_user( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct user_statics, IActivationFactory_iface );
}

static inline struct user_statics *impl_us_from_user( IUserStatics *iface )
{
    return CONTAINING_RECORD( iface, struct user_statics, IUserStatics_iface );
}

static HRESULT STDMETHODCALLTYPE user_statics_af_QI( IActivationFactory *iface, REFIID iid, void **out )
{
    struct user_statics *impl = impl_af_from_user( iface );
    if (IsEqualGUID( iid, &IID_IUnknown )        ||
        IsEqualGUID( iid, &IID_IInspectable )    ||
        IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        *out = &impl->IActivationFactory_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }
    if (IsEqualGUID( iid, &IID_IUserStatics ))
    {
        *out = &impl->IUserStatics_iface;
        IUnknown_AddRef( (IUnknown *)*out );
        return S_OK;
    }
    FIXME("%s not implemented\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE user_statics_af_AddRef( IActivationFactory *iface )
{
    struct user_statics *impl = impl_af_from_user( iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG STDMETHODCALLTYPE user_statics_af_Release( IActivationFactory *iface )
{
    struct user_statics *impl = impl_af_from_user( iface );
    return InterlockedDecrement( &impl->ref );
}

static HRESULT STDMETHODCALLTYPE user_statics_af_GetIids( IActivationFactory *iface, ULONG *n, IID **ids )
{
    *n = 0; *ids = NULL; return S_OK;
}

static HRESULT STDMETHODCALLTYPE user_statics_af_GetRTCN( IActivationFactory *iface, HSTRING *cn )
{
    return WindowsCreateString( RuntimeClass_Windows_Xbox_System_User,
                                wcslen(RuntimeClass_Windows_Xbox_System_User), cn );
}

static HRESULT STDMETHODCALLTYPE user_statics_af_GetTL( IActivationFactory *iface, TrustLevel *tl )
{
    *tl = BaseTrust; return S_OK;
}

static HRESULT STDMETHODCALLTYPE user_statics_af_Activate( IActivationFactory *iface, IInspectable **inst )
{
    *inst = NULL; return E_NOTIMPL;
}

static const IActivationFactoryVtbl user_af_vtbl =
{
    user_statics_af_QI,
    user_statics_af_AddRef,
    user_statics_af_Release,
    user_statics_af_GetIids,
    user_statics_af_GetRTCN,
    user_statics_af_GetTL,
    user_statics_af_Activate,
};

static HRESULT STDMETHODCALLTYPE us_QI( IUserStatics *iface, REFIID iid, void **out )
{
    struct user_statics *impl = impl_us_from_user( iface );
    return IActivationFactory_QueryInterface( &impl->IActivationFactory_iface, iid, out );
}

static ULONG STDMETHODCALLTYPE us_AddRef( IUserStatics *iface )
{
    struct user_statics *impl = impl_us_from_user( iface );
    return IActivationFactory_AddRef( &impl->IActivationFactory_iface );
}

static ULONG STDMETHODCALLTYPE us_Release( IUserStatics *iface )
{
    struct user_statics *impl = impl_us_from_user( iface );
    return IActivationFactory_Release( &impl->IActivationFactory_iface );
}

static HRESULT STDMETHODCALLTYPE us_GetIids( IUserStatics *iface, ULONG *n, IID **ids )
{
    *n = 0; *ids = NULL; return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_GetRTCN( IUserStatics *iface, HSTRING *cn )
{
    return WindowsCreateString( RuntimeClass_Windows_Xbox_System_User,
                                wcslen(RuntimeClass_Windows_Xbox_System_User), cn );
}

static HRESULT STDMETHODCALLTYPE us_GetTL( IUserStatics *iface, TrustLevel *tl )
{
    *tl = BaseTrust; return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_get_Users( IUserStatics *iface, void **value )
{
    IUser *u;
    IInspectable *elem;
    HRESULT hr;

    TRACE( "(%p, %p)\n", iface, value );

    hr = user_create( &u );
    if (FAILED(hr)) return hr;

    elem = (IInspectable *)u;
    hr = vector_view_create( &user_vector_iids, &elem, 1, value );
    IInspectable_Release( elem );
    return hr;
}

static HRESULT STDMETHODCALLTYPE us_add_UserAdded( IUserStatics *iface, void *handler, EventRegistrationToken *token )
{
    FIXME("stub\n");
    token->value = 1;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_remove_UserAdded( IUserStatics *iface, EventRegistrationToken token )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_add_UserRemoved( IUserStatics *iface, void *handler, EventRegistrationToken *token )
{
    FIXME("stub\n");
    token->value = 1;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_remove_UserRemoved( IUserStatics *iface, EventRegistrationToken token )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_add_SignInCompleted( IUserStatics *iface, void *handler, EventRegistrationToken *token )
{
    FIXME("stub\n");
    token->value = 1;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_remove_SignInCompleted( IUserStatics *iface, EventRegistrationToken token )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_add_SignOutStarted( IUserStatics *iface, void *handler, EventRegistrationToken *token )
{
    FIXME("stub\n");
    token->value = 1;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_remove_SignOutStarted( IUserStatics *iface, EventRegistrationToken token )
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_add_SignOutCompleted( IUserStatics *iface, void *handler, EventRegistrationToken *token )
{
    FIXME("stub\n");
    token->value = 1;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE us_remove_SignOutCompleted( IUserStatics *iface, EventRegistrationToken token )
{
    return S_OK;
}

static const IUserStaticsVtbl user_statics_vtbl =
{
    us_QI,
    us_AddRef,
    us_Release,
    us_GetIids,
    us_GetRTCN,
    us_GetTL,
    us_get_Users,
    us_add_UserAdded,
    us_remove_UserAdded,
    us_add_UserRemoved,
    us_remove_UserRemoved,
    us_add_SignInCompleted,
    us_remove_SignInCompleted,
    us_add_SignOutStarted,
    us_remove_SignOutStarted,
    us_add_SignOutCompleted,
    us_remove_SignOutCompleted,
};

static struct user_statics user_statics_instance =
{
    {&user_af_vtbl},
    {&user_statics_vtbl},
    1
};

IActivationFactory *xbox_user_factory = &user_statics_instance.IActivationFactory_iface;
