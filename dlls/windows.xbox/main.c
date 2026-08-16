/* WinRT Windows.Xbox.* namespace implementation
 *
 * Windows.Xbox.Input.Gamepad is a real, working implementation backed by
 * Windows.Gaming.Input.Gamepad (see gamepad.c / wgi_backend.c). The rest of
 * Windows.Xbox.Input.* (Controller, RacingWheel, ...) and the other Xbox One
 * ERA WinRT namespaces below remain stub activation factories.
 * Based on interface definitions from WinDurango (MIT).
 *
 * Namespaces covered:
 *   Windows.Xbox.Input.Gamepad (real)
 *   Windows.Xbox.Input.* (stub)
 *   Windows.Xbox.System
 *   Windows.Xbox.ApplicationModel
 *   Windows.Xbox.UI
 *   (others: generic stub)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "initguid.h"
#include "private.h"

WINE_DEFAULT_DEBUG_CHANNEL(xbox);

/* ---- helpers ---- */
#define DEFINE_STUB_FACTORY( name ) \
struct name##_factory { IActivationFactory IActivationFactory_iface; LONG ref; }; \
static const IActivationFactoryVtbl name##_vtbl; \
static struct name##_factory name##_factory_instance = {{&name##_vtbl},1}; \
static inline struct name##_factory *impl_from_##name( IActivationFactory *iface ) \
{ return CONTAINING_RECORD(iface, struct name##_factory, IActivationFactory_iface); } \
static HRESULT WINAPI name##_QI(IActivationFactory *iface,REFIID iid,void **out) \
{ \
    struct name##_factory *impl=impl_from_##name(iface); \
    if(IsEqualGUID(iid,&IID_IUnknown)||IsEqualGUID(iid,&IID_IInspectable)|| \
       IsEqualGUID(iid,&IID_IActivationFactory)) \
    { *out=&impl->IActivationFactory_iface; IUnknown_AddRef((IUnknown*)*out); return S_OK; } \
    FIXME("%s not implemented\n",debugstr_guid(iid)); *out=NULL; return E_NOINTERFACE; \
} \
static ULONG WINAPI name##_AddRef(IActivationFactory *iface) \
{ struct name##_factory *impl=impl_from_##name(iface); return InterlockedIncrement(&impl->ref); } \
static ULONG WINAPI name##_Release(IActivationFactory *iface) \
{ struct name##_factory *impl=impl_from_##name(iface); return InterlockedDecrement(&impl->ref); } \
static HRESULT WINAPI name##_GetIids(IActivationFactory *iface,ULONG *n,IID **ids){*n=0;*ids=NULL;return S_OK;} \
static HRESULT WINAPI name##_GetRTCN(IActivationFactory *iface,HSTRING *cn){FIXME("stub\n");return E_NOTIMPL;} \
static HRESULT WINAPI name##_GetTL(IActivationFactory *iface,TrustLevel *tl){*tl=BaseTrust;return S_OK;} \
static HRESULT WINAPI name##_Activate(IActivationFactory *iface,IInspectable **inst) \
{ FIXME("(%p): Xbox class not implemented\n",iface); *inst=NULL; return E_NOTIMPL; } \
static const IActivationFactoryVtbl name##_vtbl = { \
    name##_QI, name##_AddRef, name##_Release, \
    name##_GetIids, name##_GetRTCN, name##_GetTL, \
    name##_Activate };

/* ---- per-namespace stub factories ---- */
DEFINE_STUB_FACTORY(xbox_input)
DEFINE_STUB_FACTORY(xbox_system)
DEFINE_STUB_FACTORY(xbox_appmodel)
DEFINE_STUB_FACTORY(xbox_ui)
DEFINE_STUB_FACTORY(xbox_generic)

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID riid, void **out )
{
    FIXME( "clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out );
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

HRESULT WINAPI DllGetActivationFactory( HSTRING classid, IActivationFactory **factory )
{
    const WCHAR *name = WindowsGetStringRawBuffer( classid, NULL );

    TRACE( "class %s, factory %p\n", debugstr_hstring(classid), factory );

    *factory = NULL;

    if (!wcscmp( name, RuntimeClass_Windows_Xbox_Input_Gamepad ))
    {
        TRACE( "Windows.Xbox.Input.Gamepad\n" );
        *factory = xbox_gamepad_factory;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcscmp( name, RuntimeClass_Windows_Xbox_Input_Controller ))
    {
        TRACE( "Windows.Xbox.Input.Controller\n" );
        *factory = xbox_controller_factory;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcsncmp( name, L"Windows.Xbox.Input.", 19 ))
    {
        FIXME("Windows.Xbox.Input stub for %s\n", debugstr_w(name));
        *factory = &xbox_input_factory_instance.IActivationFactory_iface;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcscmp( name, L"Windows.Xbox.System.User" ))
    {
        TRACE( "Windows.Xbox.System.User\n" );
        *factory = xbox_user_factory;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcscmp( name, L"Windows.Xbox.Storage.ConnectedStorageSpace" ))
    {
        TRACE( "Windows.Xbox.Storage.ConnectedStorageSpace\n" );
        *factory = xbox_storage_factory;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcsncmp( name, L"Windows.Xbox.System.", 20 ))
    {
        FIXME("Windows.Xbox.System stub for %s\n", debugstr_w(name));
        *factory = &xbox_system_factory_instance.IActivationFactory_iface;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcscmp( name, L"Windows.Xbox.ApplicationModel.Core.CoreApplicationContext" ))
    {
        TRACE( "Windows.Xbox.ApplicationModel.Core.CoreApplicationContext\n" );
        *factory = xbox_appmodel_core_factory;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcsncmp( name, L"Windows.Xbox.ApplicationModel.", 30 ))
    {
        FIXME("Windows.Xbox.ApplicationModel stub for %s\n", debugstr_w(name));
        *factory = &xbox_appmodel_factory_instance.IActivationFactory_iface;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcscmp( name, L"Windows.Xbox.UI.SystemUI" ))
    {
        TRACE( "Windows.Xbox.UI.SystemUI\n" );
        *factory = xbox_sysui_factory;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcsncmp( name, L"Windows.Xbox.UI.", 16 ))
    {
        FIXME("Windows.Xbox.UI stub for %s\n", debugstr_w(name));
        *factory = &xbox_ui_factory_instance.IActivationFactory_iface;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcscmp( name, L"Windows.Xbox.Multiplayer.Manager" ) ||
        !wcsncmp( name, L"Windows.Xbox.Multiplayer.", 25 ))
    {
        if (!wcscmp( name, L"Windows.Xbox.Multiplayer.Manager" ))
            TRACE( "Windows.Xbox.Multiplayer.Manager\n" );
        else
            FIXME("Windows.Xbox.Multiplayer stub for %s\n", debugstr_w(name));
        *factory = xbox_multiplayer_factory;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcscmp( name, L"Windows.Xbox.Networking.ManagedNetwork" ) ||
        !wcscmp( name, L"Windows.Xbox.Networking.SecureDeviceAssociation" ) ||
        !wcsncmp( name, L"Windows.Xbox.Networking.", 24 ))
    {
        if (!wcsncmp( name, L"Windows.Xbox.Networking.", 24 ))
            FIXME("Windows.Xbox.Networking stub for %s\n", debugstr_w(name));
        *factory = xbox_networking_factory;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    if (!wcsncmp( name, L"Windows.Xbox.", 13 ))
    {
        FIXME("Windows.Xbox generic stub for %s\n", debugstr_w(name));
        *factory = &xbox_generic_factory_instance.IActivationFactory_iface;
        IActivationFactory_AddRef(*factory);
        return S_OK;
    }

    WARN("Unknown class %s\n", debugstr_hstring(classid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
