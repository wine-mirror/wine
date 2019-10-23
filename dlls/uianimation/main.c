/*
 * Uianimation main file.
 *
 * Copyright (C) 2018 Louis Lenders
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "oaidl.h"
#include "ocidl.h"

#include "initguid.h"
#include "uianimation.h"

#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uianimation);

static HINSTANCE hinstance;

BOOL WINAPI DllMain( HINSTANCE dll, DWORD reason, LPVOID reserved )
{
    TRACE("(%p %d %p)\n", dll, reason, reserved);

    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        hinstance = dll;
        DisableThreadLibraryCalls( dll );
        break;
    }
    return TRUE;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
    HRESULT (*create_instance)(IUnknown *, REFIID, void **);
};

static inline struct class_factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct class_factory, IClassFactory_iface );
}

static HRESULT WINAPI class_factory_QueryInterface( IClassFactory *iface, REFIID iid, void **obj )
{
    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IClassFactory ))
    {
        IClassFactory_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI class_factory_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance( IClassFactory *iface,
                                                    IUnknown *outer, REFIID iid,
                                                    void **obj )
{
    struct class_factory *This = impl_from_IClassFactory( iface );

    TRACE( "%p %s %p\n", outer, debugstr_guid( iid ), obj );

    *obj = NULL;
    return This->create_instance( outer, iid, obj );
}

static HRESULT WINAPI class_factory_LockServer( IClassFactory *iface,
                                                BOOL lock )
{
    FIXME( "%d: stub!\n", lock );
    return S_OK;
}

static const struct IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer
};

/***********************************************************************
 *          IUIAnimationManager
 */
struct manager
{
    IUIAnimationManager IUIAnimationManager_iface;
    LONG ref;
};

struct manager *impl_from_IUIAnimationManager( IUIAnimationManager *iface )
{
    return CONTAINING_RECORD( iface, struct manager, IUIAnimationManager_iface );
}

static HRESULT WINAPI manager_QueryInterface( IUIAnimationManager *iface, REFIID iid, void **obj )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );

    TRACE( "(%p)->(%s %p)\n", This, debugstr_guid( iid ), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) ||
        IsEqualIID( iid, &IID_IUIAnimationManager ))
    {
        IUIAnimationManager_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME( "interface %s not implemented\n", debugstr_guid( iid ) );
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI manager_AddRef( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    ULONG ref = InterlockedIncrement( &This->ref );

    TRACE( "(%p) ref = %u\n", This, ref );
    return ref;
}

static ULONG WINAPI manager_Release( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE( "(%p) ref = %u\n", This, ref );

    if (!ref)
    {
        heap_free( This );
    }

    return ref;
}

static HRESULT WINAPI manager_CreateAnimationVariable( IUIAnimationManager *iface, DOUBLE initial_value, IUIAnimationVariable **variable )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%f, %p)\n", This, initial_value, variable );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_ScheduleTransition( IUIAnimationManager *iface, IUIAnimationVariable *variable, IUIAnimationTransition *transition, UI_ANIMATION_SECONDS current_time )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p, %p)\n", This, variable, transition );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_CreateStoryboard( IUIAnimationManager *iface, IUIAnimationStoryboard **storyboard )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, storyboard );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_FinishAllStoryboards( IUIAnimationManager *iface, UI_ANIMATION_SECONDS max_time )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%f)\n", This, max_time );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_AbandonAllStoryboards( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_Update( IUIAnimationManager *iface, UI_ANIMATION_SECONDS cur_time, UI_ANIMATION_UPDATE_RESULT *update_result )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%f, %p)\n", This, cur_time, update_result );
    if (update_result)
        *update_result = UI_ANIMATION_UPDATE_VARIABLES_CHANGED;
    return S_OK;
}

static HRESULT WINAPI manager_GetVariableFromTag( IUIAnimationManager *iface, IUnknown *object, UINT32 id, IUIAnimationVariable **variable )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p, %p)\n", This, object, variable );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_GetStoryboardFromTag( IUIAnimationManager *iface, IUnknown *object, UINT32 id, IUIAnimationStoryboard **storyboard )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p, %d, %p)\n", This, object, id, storyboard );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_GetStatus( IUIAnimationManager *iface, UI_ANIMATION_MANAGER_STATUS *status )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, status );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetAnimationMode( IUIAnimationManager *iface, UI_ANIMATION_MODE mode )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%d)\n", This, mode );
    return S_OK;
}

static HRESULT WINAPI manager_Pause( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_Resume( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetManagerEventHandler( IUIAnimationManager *iface, IUIAnimationManagerEventHandler *handler )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, handler );
    return S_OK;
}

static HRESULT WINAPI manager_SetCancelPriorityComparison( IUIAnimationManager *iface, IUIAnimationPriorityComparison *comparison )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, comparison );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetTrimPriorityComparison( IUIAnimationManager *iface, IUIAnimationPriorityComparison *comparison )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, comparison );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetCompressPriorityComparison( IUIAnimationManager *iface, IUIAnimationPriorityComparison *comparison)
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, comparison );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetConcludePriorityComparison( IUIAnimationManager *iface, IUIAnimationPriorityComparison *comparison )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%p)\n", This, comparison );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_SetDefaultLongestAcceptableDelay( IUIAnimationManager *iface, UI_ANIMATION_SECONDS delay )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->(%f)\n", This, delay );
    return E_NOTIMPL;
}

static HRESULT WINAPI manager_Shutdown( IUIAnimationManager *iface )
{
    struct manager *This = impl_from_IUIAnimationManager( iface );
    FIXME( "stub (%p)->()\n", This );
    return E_NOTIMPL;
}

const struct IUIAnimationManagerVtbl manager_vtbl =
{
    manager_QueryInterface,
    manager_AddRef,
    manager_Release,
    manager_CreateAnimationVariable,
    manager_ScheduleTransition,
    manager_CreateStoryboard,
    manager_FinishAllStoryboards,
    manager_AbandonAllStoryboards,
    manager_Update,
    manager_GetVariableFromTag,
    manager_GetStoryboardFromTag,
    manager_GetStatus,
    manager_SetAnimationMode,
    manager_Pause,
    manager_Resume,
    manager_SetManagerEventHandler,
    manager_SetCancelPriorityComparison,
    manager_SetTrimPriorityComparison,
    manager_SetCompressPriorityComparison,
    manager_SetConcludePriorityComparison,
    manager_SetDefaultLongestAcceptableDelay,
    manager_Shutdown
};

static HRESULT manager_create( IUnknown *outer, REFIID iid, void **obj )
{
    struct manager *This = heap_alloc( sizeof(*This) );
    HRESULT hr;

    if (!This) return E_OUTOFMEMORY;
    This->IUIAnimationManager_iface.lpVtbl = &manager_vtbl;
    This->ref = 1;

    hr = IUIAnimationManager_QueryInterface( &This->IUIAnimationManager_iface, iid, obj );

    IUIAnimationManager_Release( &This->IUIAnimationManager_iface );
    return hr;
}

static struct class_factory manager_cf = { { &class_factory_vtbl }, manager_create };

/******************************************************************
 *             DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID iid, void **obj )
{
    IClassFactory *cf = NULL;

    TRACE( "(%s %s %p)\n", debugstr_guid( clsid ), debugstr_guid( iid ), obj );

    if (IsEqualCLSID( clsid, &CLSID_UIAnimationManager ))
        cf = &manager_cf.IClassFactory_iface;

    if (!cf)
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, iid, obj );
}

/******************************************************************
 *              DllCanUnloadNow
 */
HRESULT WINAPI DllCanUnloadNow( void )
{
    TRACE( "()\n" );
    return S_FALSE;
}

/***********************************************************************
 *          DllRegisterServer
 */
HRESULT WINAPI DllRegisterServer( void )
{
    return __wine_register_resources( hinstance );
}

/***********************************************************************
 *          DllUnregisterServer
 */
HRESULT WINAPI DllUnregisterServer( void )
{
    return __wine_unregister_resources( hinstance );
}
