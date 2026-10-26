/*
 * Copyright 2026 Vibhav Pant for CodeWeavers
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
#include <winstring.h>

#include "private.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(wpnapps);

struct toast_manager_statics
{
    IActivationFactory IActivationFactory_iface;
    IToastNotificationManagerStatics IToastNotificationManagerStatics_iface;
    LONG ref;
};

static inline struct toast_manager_statics *impl_toast_manager_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct toast_manager_statics, IActivationFactory_iface );
}

static HRESULT WINAPI toast_manager_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct toast_manager_statics *impl = impl_toast_manager_from_IActivationFactory( iface );

    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) || IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IActivationFactory_AddRef( (*out = &impl->IActivationFactory_iface ) );
        return S_OK;
    }
    if (IsEqualGUID( iid, &IID_IToastNotificationManagerStatics ))
    {
        IToastNotificationManagerStatics_AddRef( (*out = &impl->IToastNotificationManagerStatics_iface) );
        return S_OK;
    }

    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI toast_manager_factory_AddRef( IActivationFactory *iface )
{
    struct toast_manager_statics *impl = impl_toast_manager_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI toast_manager_factory_Release( IActivationFactory *iface )
{
    struct toast_manager_statics *impl = impl_toast_manager_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedDecrement( &impl->ref );
}

static HRESULT WINAPI toast_manager_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_manager_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *name )
{
    FIXME( "(%p, %p): stub!\n", iface, name );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_manager_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_manager_factory_ActivateInstance( IActivationFactory *iface, IInspectable **out )
{
    FIXME("(%p, %p): stub!\n", iface, out );
    return E_NOTIMPL;
}

static const IActivationFactoryVtbl toast_manager_factory_vtbl =
{
    /* IUnknown */
    toast_manager_factory_QueryInterface,
    toast_manager_factory_AddRef,
    toast_manager_factory_Release,
    /* IInspectable */
    toast_manager_factory_GetIids,
    toast_manager_factory_GetRuntimeClassName,
    toast_manager_factory_GetTrustLevel,
    /* IActivationFactory */
    toast_manager_factory_ActivateInstance,
};

DEFINE_IINSPECTABLE( toast_manager_statics, IToastNotificationManagerStatics, struct toast_manager_statics, IActivationFactory_iface );

static HRESULT WINAPI toast_manager_statics_CreateToastNotifier( IToastNotificationManagerStatics *iface, IToastNotifier **out )
{
    FIXME( "(%p, %p): stub!\n", iface, out );
    return E_NOTIMPL;
}

static HRESULT create_toast_notifier( IToastNotifier ** );

static HRESULT WINAPI toast_manager_statics_CreateToastNotifierWithId( IToastNotificationManagerStatics *iface, HSTRING app_id, IToastNotifier **out )
{
    FIXME( "(%p, %s, %p): semi-stub!\n", iface, debugstr_hstring( app_id ), out );
    return create_toast_notifier( out );
}

static const char *debugstr_ToastTemplateType( ToastTemplateType type )
{
    static const char *str[] =
    {
        "ToastImageAndText01",
        "ToastImageAndText02",
        "ToastImageAndText03",
        "ToastImageAndText04",
        "ToastText01",
        "ToastText02",
        "ToastText03",
        "ToastText04"
    };
    if (type < ARRAY_SIZE(str)) return wine_dbg_sprintf( "ToastTemplateType_%s", str[type] );
    return wine_dbg_sprintf( "{unknown %#x}", type );
}

static HRESULT WINAPI toast_manager_statics_GetTemplateContent( IToastNotificationManagerStatics *iface, ToastTemplateType type, IXmlDocument **out )
{
    FIXME( "(%p, %s, %p): stub!\n", iface, debugstr_ToastTemplateType( type ), out );
    return E_NOTIMPL;
}

static const IToastNotificationManagerStaticsVtbl toast_manager_statics_vtbl =
{
    /* IUnknown */
    toast_manager_statics_QueryInterface,
    toast_manager_statics_AddRef,
    toast_manager_statics_Release,
    /* IInspectable */
    toast_manager_statics_GetIids,
    toast_manager_statics_GetRuntimeClassName,
    toast_manager_statics_GetTrustLevel,
    /* IToastNotificationManagerStatics */
    toast_manager_statics_CreateToastNotifier,
    toast_manager_statics_CreateToastNotifierWithId,
    toast_manager_statics_GetTemplateContent,
};

static struct toast_manager_statics toast_manager_statics =
{
    { &toast_manager_factory_vtbl },
    { &toast_manager_statics_vtbl },
    1
};

IActivationFactory *toast_manager_factory = &toast_manager_statics.IActivationFactory_iface;

struct toast_factory
{
    IActivationFactory IActivationFactory_iface;
    IToastNotificationFactory IToastNotificationFactory_iface;
    LONG ref;
};

static inline struct toast_factory *impl_toast_factory_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD( iface, struct toast_factory, IActivationFactory_iface );
}

static HRESULT WINAPI toast_act_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct toast_factory *impl = impl_toast_factory_from_IActivationFactory( iface );

    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) || IsEqualGUID( iid, &IID_IActivationFactory ))
    {
        IActivationFactory_AddRef( (*out = &impl->IActivationFactory_iface) );
        return S_OK;
    }
    if (IsEqualGUID( iid, &IID_IToastNotificationFactory ))
    {
        IToastNotificationFactory_AddRef( (*out = &impl->IToastNotificationFactory_iface) );
        return S_OK;
    }

    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI toast_act_factory_AddRef( IActivationFactory *iface )
{
    struct toast_factory *impl = impl_toast_factory_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI toast_act_factory_Release( IActivationFactory *iface )
{
    struct toast_factory *impl = impl_toast_factory_from_IActivationFactory( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedDecrement( &impl->ref );
}

static HRESULT WINAPI toast_act_factory_GetIids( IActivationFactory *iface, ULONG *iids_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iids_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_act_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *name )
{
    FIXME( "(%p, %p): stub!\n", iface, name );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_act_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_act_factory_ActivateInstance( IActivationFactory *iface, IInspectable **out )
{
    FIXME( "(%p, %p): stub!\n", iface, out );
    return E_NOTIMPL;
}

static const IActivationFactoryVtbl toast_act_factory_vtbl =
{
    /* IUnknown */
    toast_act_factory_QueryInterface,
    toast_act_factory_AddRef,
    toast_act_factory_Release,
    /* IInspectable */
    toast_act_factory_GetIids,
    toast_act_factory_GetRuntimeClassName,
    toast_act_factory_GetTrustLevel,
    /* IActivactionFactory */
    toast_act_factory_ActivateInstance
};

DEFINE_IINSPECTABLE( toast_factory, IToastNotificationFactory, struct toast_factory, IActivationFactory_iface );

static HRESULT WINAPI toast_factory_CreateToastNotification( IToastNotificationFactory *iface, IXmlDocument *content, IToastNotification **out )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, content, out );
    return E_NOTIMPL;
}

static const IToastNotificationFactoryVtbl toast_factory_vtbl =
{
    /* IUnknown */
    toast_factory_QueryInterface,
    toast_factory_AddRef,
    toast_factory_Release,
    /* IInspectable */
    toast_factory_GetIids,
    toast_factory_GetRuntimeClassName,
    toast_factory_GetTrustLevel,
    /* IToastNotificationFactory */
    toast_factory_CreateToastNotification
};

static struct toast_factory toast_factory =
{
    { &toast_act_factory_vtbl },
    { &toast_factory_vtbl },
    1
};

IActivationFactory *toast_notification_factory = &toast_factory.IActivationFactory_iface;

struct toast_notifier
{
    IToastNotifier IToastNotifier_iface;
    LONG ref;
};

static inline struct toast_notifier *impl_from_IToastNotifier( IToastNotifier *iface )
{
    return CONTAINING_RECORD( iface, struct toast_notifier, IToastNotifier_iface );
}

static HRESULT WINAPI toast_notifier_QueryInterface( IToastNotifier *iface, REFIID iid, void **out )
{
    struct toast_notifier *impl = impl_from_IToastNotifier( iface );

    TRACE( "(%p, %s, %p)\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IInspectable ) || IsEqualGUID( iid, &IID_IToastNotifier ))
    {
        IToastNotifier_AddRef( (*out = &impl->IToastNotifier_iface) );
        return S_OK;
    }

    *out = NULL;
    FIXME( "%s not implemented, returning E_NOINTERFACE\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI toast_notifier_AddRef( IToastNotifier *iface )
{
    struct toast_notifier *impl = impl_from_IToastNotifier( iface );
    TRACE( "(%p)\n", iface );
    return InterlockedIncrement( &impl->ref );
}

static ULONG WINAPI toast_notifier_Release( IToastNotifier *iface )
{
    struct toast_notifier *impl = impl_from_IToastNotifier( iface );
    ULONG ref;

    TRACE( "(%p)\n", iface );

    ref = InterlockedDecrement( &impl->ref );
    if (!ref) free( iface );
    return ref;
}

static HRESULT WINAPI toast_notifier_GetIids( IToastNotifier *iface, ULONG *iids_count, IID **iids )
{
    FIXME( "(%p, %p, %p): stub!\n", iface, iids_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_notifier_GetRuntimeClassName( IToastNotifier *iface, HSTRING *out )
{
    FIXME( "(%p, %p): stub!\n", iface, out );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_notifier_GetTrustLevel( IToastNotifier *iface, TrustLevel *level )
{
    FIXME( "(%p, %p): stub!\n", iface, level );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_notifier_Show( IToastNotifier *iface, IToastNotification *notification )
{
    FIXME( "(%p, %p): stub!\n", iface, notification );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_notifier_Hide( IToastNotifier *iface, IToastNotification *notification )
{
    FIXME( "(%p, %p): stub!\n", iface, notification );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_notifier_get_Setting( IToastNotifier *iface, NotificationSetting *setting )
{
    FIXME( "(%p, %p): stub!\n", iface, setting );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_notifier_AddToSchedule( IToastNotifier *iface, IScheduledToastNotification *scheduled )
{
    FIXME( "(%p, %p): stub!\n", iface, scheduled );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_notifier_RemoveFromSchedule( IToastNotifier *iface, IScheduledToastNotification *scheduled )
{
    FIXME( "(%p, %p): stub!\n", iface, scheduled );
    return E_NOTIMPL;
}

static HRESULT WINAPI toast_notifier_GetScheduledToastNotifications( IToastNotifier *iface, IVectorView_ScheduledToastNotification **notifications )
{
    FIXME( "(%p, %p): stub!\n", iface, notifications );
    return E_NOTIMPL;
}

static const IToastNotifierVtbl toast_notifier_vtbl =
{
    /* IUnknown */
    toast_notifier_QueryInterface,
    toast_notifier_AddRef,
    toast_notifier_Release,
    /* IInspectable */
    toast_notifier_GetIids,
    toast_notifier_GetRuntimeClassName,
    toast_notifier_GetTrustLevel,
    /* IToastNotifier */
    toast_notifier_Show,
    toast_notifier_Hide,
    toast_notifier_get_Setting,
    toast_notifier_AddToSchedule,
    toast_notifier_RemoveFromSchedule,
    toast_notifier_GetScheduledToastNotifications
};

static HRESULT create_toast_notifier( IToastNotifier **out )
{
    struct toast_notifier *impl;

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;

    impl->IToastNotifier_iface.lpVtbl = &toast_notifier_vtbl;
    impl->ref = 1;
    *out = &impl->IToastNotifier_iface;
    return S_OK;
}
