/*
 * Copyright (C) 2025 Mohamad Al-Jaf
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
#include "initguid.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winstring.h"

#include "roapi.h"

#define WIDL_using_Windows_Foundation
#include "windows.foundation.h"
#define WIDL_using_Windows_System
#include "windows.system.h"

#include "dispatcherqueue.h"

#include "wine/test.h"

#define check_interface( obj, iid ) check_interface_( __LINE__, obj, iid )
static void check_interface_( unsigned int line, void *obj, const IID *iid )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    IUnknown_Release( unk );
}

struct typed_event_handler_dispatcher_queue
{
    ITypedEventHandler_DispatcherQueue_IInspectable ITypedEventHandler_DispatcherQueue_IInspectable_iface;
    LONG ref;

    HANDLE event;
};

static struct typed_event_handler_dispatcher_queue *impl_from_ITypedEventHandler_DispatcherQueue_IInspectable( ITypedEventHandler_DispatcherQueue_IInspectable *iface )
{
    return CONTAINING_RECORD( iface, struct typed_event_handler_dispatcher_queue, ITypedEventHandler_DispatcherQueue_IInspectable_iface );
}

static HRESULT WINAPI typed_event_handler_dispatcher_queue_QueryInterface( ITypedEventHandler_DispatcherQueue_IInspectable *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_ITypedEventHandler_DispatcherQueue_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IUnknown ))
    {
        *out = iface;
        ITypedEventHandler_DispatcherQueue_IInspectable_AddRef( iface );
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI typed_event_handler_dispatcher_queue_AddRef( ITypedEventHandler_DispatcherQueue_IInspectable *iface )
{
    struct typed_event_handler_dispatcher_queue *handler = impl_from_ITypedEventHandler_DispatcherQueue_IInspectable( iface );
    return InterlockedIncrement( &handler->ref );
}

static ULONG WINAPI typed_event_handler_dispatcher_queue_Release( ITypedEventHandler_DispatcherQueue_IInspectable *iface )
{
    struct typed_event_handler_dispatcher_queue *handler = impl_from_ITypedEventHandler_DispatcherQueue_IInspectable( iface );
    ULONG ref = InterlockedDecrement( &handler->ref );

    if (!ref)
    {
        CloseHandle( handler->event );
        free( handler );
    }

    return ref;
}

static HRESULT WINAPI typed_event_handler_dispatcher_queue_Invoke( ITypedEventHandler_DispatcherQueue_IInspectable *iface, IDispatcherQueue *queue, IInspectable *inspectable )
{
    struct typed_event_handler_dispatcher_queue *handler = impl_from_ITypedEventHandler_DispatcherQueue_IInspectable( iface );

    SetEvent( handler->event );
    return S_OK;
}

static const ITypedEventHandler_DispatcherQueue_IInspectableVtbl typed_event_handler_dispatcher_queue_vtbl =
{
    typed_event_handler_dispatcher_queue_QueryInterface,
    typed_event_handler_dispatcher_queue_AddRef,
    typed_event_handler_dispatcher_queue_Release,
    typed_event_handler_dispatcher_queue_Invoke,
};

static HRESULT create_typed_event_handler_dispatcher_queue( ITypedEventHandler_DispatcherQueue_IInspectable **handler )
{
    struct typed_event_handler_dispatcher_queue *impl;

    *handler = NULL;

    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;

    impl->ITypedEventHandler_DispatcherQueue_IInspectable_iface.lpVtbl = &typed_event_handler_dispatcher_queue_vtbl;
    impl->ref = 1;
    impl->event = CreateEventW( NULL, TRUE, FALSE, NULL );

    *handler = &impl->ITypedEventHandler_DispatcherQueue_IInspectable_iface;
    return S_OK;
}

struct dispatcher_queue_handler
{
    IDispatcherQueueHandler IDispatcherQueueHandler_iface;
    LONG ref;

    HANDLE event;
};

static struct dispatcher_queue_handler *impl_from_IDispatcherQueueHandler( IDispatcherQueueHandler *iface )
{
    return CONTAINING_RECORD( iface, struct dispatcher_queue_handler, IDispatcherQueueHandler_iface );
}

static HRESULT WINAPI dispatcher_queue_handler_QueryInterface( IDispatcherQueueHandler *iface, REFIID iid, void **out )
{
    if (IsEqualGUID( iid, &IID_IDispatcherQueueHandler ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IUnknown ))
    {
        *out = iface;
        IDispatcherQueueHandler_AddRef( iface );
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dispatcher_queue_handler_AddRef( IDispatcherQueueHandler *iface )
{
    struct dispatcher_queue_handler *handler = impl_from_IDispatcherQueueHandler( iface );
    return InterlockedIncrement( &handler->ref );
}

static ULONG WINAPI dispatcher_queue_handler_Release( IDispatcherQueueHandler *iface )
{
    struct dispatcher_queue_handler *handler = impl_from_IDispatcherQueueHandler( iface );
    ULONG ref = InterlockedDecrement( &handler->ref );

    if (!ref)
    {
        CloseHandle( handler->event );
        free( handler );
    }

    return ref;
}

static HRESULT WINAPI dispatcher_queue_handler_Invoke( IDispatcherQueueHandler *iface )
{
    struct dispatcher_queue_handler *handler = impl_from_IDispatcherQueueHandler( iface );
    DWORD ret;

    ret = WaitForSingleObject( handler->event, 5000 );
    ok( !ret, "Unexpected wait result %lu.\n", ret );

    return S_OK;
}

static const IDispatcherQueueHandlerVtbl dispatcher_queue_handler_vtbl =
{
    dispatcher_queue_handler_QueryInterface,
    dispatcher_queue_handler_AddRef,
    dispatcher_queue_handler_Release,
    dispatcher_queue_handler_Invoke,
};

static HRESULT create_dispatcher_queue_handler( IDispatcherQueueHandler **handler )
{
    struct dispatcher_queue_handler *impl;

    *handler = NULL;

    if (!(impl = calloc( 1, sizeof( *impl ) ))) return E_OUTOFMEMORY;

    impl->IDispatcherQueueHandler_iface.lpVtbl = &dispatcher_queue_handler_vtbl;
    impl->ref = 1;
    impl->event = CreateEventW( NULL, TRUE, FALSE, NULL );

    *handler = &impl->IDispatcherQueueHandler_iface;
    return S_OK;
}

#define wait_messages( a ) msg_wait_for_events_( __FILE__, __LINE__, 0, NULL, a )
#define msg_wait_for_events( a, b, c ) msg_wait_for_events_( __FILE__, __LINE__, a, b, c )
static DWORD msg_wait_for_events_( const char *file, int line, DWORD count, HANDLE *events, DWORD timeout )
{
    DWORD ret, end = GetTickCount() + min( timeout, 5000 );
    MSG msg;

    while ((ret = MsgWaitForMultipleObjects( count, events, FALSE, min( timeout, 5000 ), QS_ALLINPUT )) <= count)
    {
        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
        if (ret < count) return ret;
        if (timeout >= 5000) continue;
        if (end <= GetTickCount()) timeout = 0;
        else timeout = end - GetTickCount();
    }

    if (timeout >= 5000) ok_(file, line)( 0, "MsgWaitForMultipleObjects returned %#lx\n", ret );
    else ok_(file, line)( ret == WAIT_TIMEOUT, "MsgWaitForMultipleObjects returned %#lx\n", ret );
    return ret;
}

#define check_create_dispatcher_queue_controller( size, thread_type, apartment_type, expected_hr ) \
        check_create_dispatcher_queue_controller_( __LINE__, size, thread_type, apartment_type, expected_hr )
static void check_create_dispatcher_queue_controller_( unsigned int line, DWORD size, DISPATCHERQUEUE_THREAD_TYPE thread_type,
                                                       DISPATCHERQUEUE_THREAD_APARTMENTTYPE apartment_type, HRESULT expected_hr )
{
    ITypedEventHandler_DispatcherQueue_IInspectable *event_handler_iface = NULL;
    struct typed_event_handler_dispatcher_queue *event_handler = NULL;
    IDispatcherQueueController *dispatcher_queue_controller = NULL;
    struct dispatcher_queue_handler *queue_handler = NULL;
    IDispatcherQueueHandler *handler_iface = NULL;
    struct DispatcherQueueOptions options = { 0 };
    IDispatcherQueue2 *dispatcher_queue2 = NULL;
    IDispatcherQueue *dispatcher_queue = NULL;
    IAsyncAction *operation = NULL;
    IAsyncInfo *async_info = NULL;
    EventRegistrationToken token;
    AsyncStatus status;
    boolean result;
    HRESULT hr;
    DWORD ret;
    LONG ref;

    options.dwSize = size;
    options.threadType = thread_type;
    options.apartmentType = apartment_type;

    hr = CreateDispatcherQueueController( options, &dispatcher_queue_controller );
    ok_(__FILE__, line)( hr == expected_hr, "got CreateDispatcherQueueController hr %#lx.\n", hr );
    if (hr == E_INVALIDARG) return;

    hr = IDispatcherQueueController_get_DispatcherQueue( dispatcher_queue_controller, &dispatcher_queue );
    todo_wine
    ok_(__FILE__, line)( hr == S_OK, "got IDispatcherQueueController_get_DispatcherQueue hr %#lx.\n", hr );
    if (FAILED(hr)) goto done;

    hr = IDispatcherQueue_QueryInterface( dispatcher_queue, &IID_IDispatcherQueue2, (void **)&dispatcher_queue2 );
    ok_(__FILE__, line)( hr == S_OK || broken(hr == E_NOINTERFACE) /* w1064v1809 */, "got IDispatcherQueue_QueryInterface hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
    {
        hr = IDispatcherQueue2_get_HasThreadAccess( dispatcher_queue2, &result );
        ok_(__FILE__, line)( hr == S_OK, "got IDispatcherQueue2_get_HasThreadAccess hr %#lx.\n", hr );
        ok_(__FILE__, line)( result == (thread_type == DQTYPE_THREAD_CURRENT ? TRUE : FALSE), "got IDispatcherQueue2_get_HasThreadAccess result %d.\n", result );
        ref = IDispatcherQueue2_Release( dispatcher_queue2 );
        ok_(__FILE__, line)( ref == (thread_type == DQTYPE_THREAD_CURRENT ? 2 : 3), "got IDispatcherQueue2_Release ref %ld.\n", ref );
    }

    hr = create_dispatcher_queue_handler( &handler_iface );
    ok_(__FILE__, line)( hr == S_OK, "create_dispatcher_queue_handler failed, hr %#lx.\n", hr );
    queue_handler = impl_from_IDispatcherQueueHandler( handler_iface );

    hr = IDispatcherQueue_TryEnqueue( dispatcher_queue, handler_iface, &result );
    ok_(__FILE__, line)( hr == S_OK, "got IDispatcherQueue_TryEnqueue hr %#lx.\n", hr );
    ok_(__FILE__, line)( result == TRUE, "got IDispatcherQueue_TryEnqueue result %d.\n", result );

    hr = create_typed_event_handler_dispatcher_queue( &event_handler_iface );
    ok_(__FILE__, line)( hr == S_OK, "create_typed_event_handler_dispatcher_queue failed, hr %#lx.\n", hr );
    event_handler = impl_from_ITypedEventHandler_DispatcherQueue_IInspectable( event_handler_iface );

    hr = IDispatcherQueue_add_ShutdownCompleted( dispatcher_queue, event_handler_iface, &token );
    ok_(__FILE__, line)( hr == S_OK, "got IDispatcherQueue_add_ShutdownCompleted hr %#lx.\n", hr );
    hr = IDispatcherQueueController_ShutdownQueueAsync( dispatcher_queue_controller, &operation );
    ok_(__FILE__, line)( hr == S_OK, "got IDispatcherQueueController_ShutdownQueueAsync hr %#lx.\n", hr );

    hr = IAsyncAction_QueryInterface( operation, &IID_IAsyncInfo, (void **)&async_info );
    ok_(__FILE__, line)( hr == S_OK, "got IAsyncAction_QueryInterface hr %#lx.\n", hr );

    hr = IAsyncInfo_get_Status( async_info, &status );
    ok_(__FILE__, line)( hr == S_OK, "got IAsyncInfo_get_Status hr %#lx.\n", hr );
    ok_(__FILE__, line)( status == Started, "got IAsyncInfo_get_Status status %d.\n", status );

    /* shutdown waits for queued handlers */
    if (winetest_platform_is_wine) Sleep( 200 );
    ret = WaitForSingleObject( event_handler->event, 100 );
    todo_wine ok_(__FILE__, line)( ret == WAIT_TIMEOUT, "Unexpected wait result %lu.\n", ret );
    SetEvent( queue_handler->event );

    /* queue uses the message loop when dispatched on current thread */
    if (thread_type == DQTYPE_THREAD_CURRENT)
    {
        ret = WaitForSingleObject( event_handler->event, 100 );
        ok_(__FILE__, line)( ret == WAIT_TIMEOUT, "Unexpected wait result %lu.\n", ret );
        ret = msg_wait_for_events( 1, &event_handler->event, 5000 );
        ok_(__FILE__, line)( !ret, "Unexpected wait result %lu.\n", ret );
    }
    else
    {
        ret = WaitForSingleObject( event_handler->event, 5000 );
        ok_(__FILE__, line)( !ret, "Unexpected wait result %lu.\n", ret );
    }

    hr = IAsyncInfo_get_Status( async_info, &status );
    ok_(__FILE__, line)( hr == S_OK, "got IAsyncInfo_get_Status hr %#lx.\n", hr );
    ok_(__FILE__, line)( status == Completed, "got IAsyncInfo_get_Status status %d.\n", status );

    hr = IAsyncInfo_Close( async_info );
    ok_(__FILE__, line)( hr == S_OK, "got IAsyncInfo_Close hr %#lx.\n", hr );
    ref = IAsyncInfo_Release( async_info );
    ok_(__FILE__, line)( ref == 2, "got IAsyncInfo_Release ref %ld.\n", ref );
    ref = IAsyncAction_Release( operation );
    ok_(__FILE__, line)( ref == 1, "got IAsyncAction_Release ref %ld.\n", ref );
    hr = IDispatcherQueue_remove_ShutdownCompleted( dispatcher_queue, token );
    ok_(__FILE__, line)( hr == S_OK, "got IDispatcherQueue_remove_ShutdownCompleted hr %#lx.\n", hr );

    ref = ITypedEventHandler_DispatcherQueue_IInspectable_Release( event_handler_iface );
    ok_(__FILE__, line)( ref == 0, "got ITypedEventHandler_DispatcherQueue_IInspectable_Release ref %ld.\n", ref );
    ref = IDispatcherQueueHandler_Release( handler_iface );
    ok_(__FILE__, line)( ref == 0, "got IDispatcherQueueHandler_Release ref %ld.\n", ref );
    IDispatcherQueue_Release( dispatcher_queue );
done:
    IDispatcherQueueController_Release( dispatcher_queue_controller );
}

static void test_CreateDispatcherQueueController(void)
{
    IDispatcherQueueController *dispatcher_queue_controller = (void *)0xdeadbeef;
    struct DispatcherQueueOptions options = { 0 };
    HRESULT hr;

    hr = CreateDispatcherQueueController( options, NULL );
    ok( hr == E_POINTER || hr == 0x80000005 /* win10 22h2 */, "got hr %#lx.\n", hr );
    hr = CreateDispatcherQueueController( options, &dispatcher_queue_controller );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    ok( dispatcher_queue_controller == (void *)0xdeadbeef, "got dispatcher_queue_controller %p.\n", dispatcher_queue_controller );

    /* Invalid args */

    check_create_dispatcher_queue_controller( 0,                                    0,                       DQTAT_COM_NONE, E_INVALIDARG );
    check_create_dispatcher_queue_controller( 0,                                    0,                       DQTAT_COM_ASTA, E_INVALIDARG );
    check_create_dispatcher_queue_controller( 0,                                    0,                       DQTAT_COM_STA,  E_INVALIDARG );
    check_create_dispatcher_queue_controller( 0,                                    DQTYPE_THREAD_CURRENT,   DQTAT_COM_NONE, E_INVALIDARG );
    check_create_dispatcher_queue_controller( 0,                                    DQTYPE_THREAD_CURRENT,   DQTAT_COM_ASTA, E_INVALIDARG );
    check_create_dispatcher_queue_controller( 0,                                    DQTYPE_THREAD_CURRENT,   DQTAT_COM_STA,  E_INVALIDARG );
    check_create_dispatcher_queue_controller( 0,                                    DQTYPE_THREAD_DEDICATED, DQTAT_COM_NONE, E_INVALIDARG );
    check_create_dispatcher_queue_controller( 0,                                    DQTYPE_THREAD_DEDICATED, DQTAT_COM_ASTA, E_INVALIDARG );
    check_create_dispatcher_queue_controller( 0,                                    DQTYPE_THREAD_DEDICATED, DQTAT_COM_STA,  E_INVALIDARG );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ),     0,                       DQTAT_COM_NONE, E_INVALIDARG );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ),     0,                       DQTAT_COM_ASTA, E_INVALIDARG );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ),     0,                       DQTAT_COM_STA,  E_INVALIDARG );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ),     0xdeadbeef,              DQTAT_COM_NONE, E_INVALIDARG );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ),     0xdeadbeef,              DQTAT_COM_ASTA, E_INVALIDARG );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ),     0xdeadbeef,              DQTAT_COM_STA,  E_INVALIDARG );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ) - 1, DQTYPE_THREAD_CURRENT,   DQTAT_COM_NONE, E_INVALIDARG );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ) + 1, DQTYPE_THREAD_CURRENT,   DQTAT_COM_NONE, E_INVALIDARG );

    if (0) /* Silently crashes in Windows */
    {
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ), DQTYPE_THREAD_DEDICATED, 0xdeadbeef,     S_OK );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ), DQTYPE_THREAD_DEDICATED, DQTAT_COM_NONE, S_OK );
    }

    /* Valid args */

    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ), DQTYPE_THREAD_CURRENT,   0xdeadbeef,     S_OK );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ), DQTYPE_THREAD_CURRENT,   DQTAT_COM_NONE, S_OK );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ), DQTYPE_THREAD_CURRENT,   DQTAT_COM_ASTA, S_OK );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ), DQTYPE_THREAD_CURRENT,   DQTAT_COM_STA,  S_OK );

    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ), DQTYPE_THREAD_DEDICATED, DQTAT_COM_ASTA, S_OK );
    check_create_dispatcher_queue_controller( sizeof( DispatcherQueueOptions ), DQTYPE_THREAD_DEDICATED, DQTAT_COM_STA,  S_OK );
}

static void test_DispatcherQueueController_Statics(void)
{
    static const WCHAR *dispatcher_queue_controller_statics_name = L"Windows.System.DispatcherQueueController";
    IDispatcherQueueControllerStatics *dispatcher_queue_controller_statics = (void *)0xdeadbeef;
    ITypedEventHandler_DispatcherQueue_IInspectable *event_handler_iface = (void *)0xdeadbeef;
    struct typed_event_handler_dispatcher_queue *event_handler = (void *)0xdeadbeef;
    IDispatcherQueueController *dispatcher_queue_controller = (void *)0xdeadbeef;
    struct dispatcher_queue_handler *queue_handler = (void *)0xdeadbeef;
    IDispatcherQueueHandler *handler_iface = (void *)0xdeadbeef;
    IDispatcherQueue2 *dispatcher_queue2 = (void *)0xdeadbeef;
    IDispatcherQueue *dispatcher_queue = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    IAsyncAction *operation = (void *)0xdeadbeef;
    IAsyncInfo *async_info = (void *)0xdeadbeef;
    EventRegistrationToken token;
    AsyncStatus status;
    HSTRING str = NULL;
    boolean result;
    HRESULT hr;
    DWORD ret;
    LONG ref;

    hr = WindowsCreateString( dispatcher_queue_controller_statics_name, wcslen( dispatcher_queue_controller_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( dispatcher_queue_controller_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown );
    check_interface( factory, &IID_IInspectable );
    check_interface( factory, &IID_IAgileObject );

    hr = IActivationFactory_QueryInterface( factory, &IID_IDispatcherQueueControllerStatics, (void **)&dispatcher_queue_controller_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IDispatcherQueueControllerStatics_CreateOnDedicatedThread( dispatcher_queue_controller_statics, NULL );
    todo_wine
    ok( hr == E_POINTER || hr == 0x80000005 /* win10 22h2 */, "got hr %#lx.\n", hr );
    hr = IDispatcherQueueControllerStatics_CreateOnDedicatedThread( dispatcher_queue_controller_statics, &dispatcher_queue_controller );
    todo_wine
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    if (FAILED(hr)) goto done;

    hr = IDispatcherQueueController_get_DispatcherQueue( dispatcher_queue_controller, NULL );
    todo_wine
    ok( hr == E_POINTER || hr == 0x80000005 /* win10 22h2 */, "got hr %#lx.\n", hr );
    hr = IDispatcherQueueController_get_DispatcherQueue( dispatcher_queue_controller, &dispatcher_queue );
    todo_wine
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( dispatcher_queue, &IID_IUnknown );
    check_interface( dispatcher_queue, &IID_IInspectable );
    check_interface( dispatcher_queue, &IID_IAgileObject );

    hr = create_dispatcher_queue_handler( &handler_iface );
    ok( hr == S_OK, "Unexpected hr %#lx.\n", hr );
    queue_handler = impl_from_IDispatcherQueueHandler( handler_iface );

    hr = IDispatcherQueue_TryEnqueue( dispatcher_queue, handler_iface, &result );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( result == TRUE, "got result %d.\n", result );

    hr = IDispatcherQueue_QueryInterface( dispatcher_queue, &IID_IDispatcherQueue2, (void **)&dispatcher_queue2 );
    ok( hr == S_OK || broken(hr == E_NOINTERFACE) /* w1064v1809 */, "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
    {
        hr = IDispatcherQueue2_get_HasThreadAccess( dispatcher_queue2, &result );
        ok( hr == S_OK, "got hr %#lx.\n", hr );
        ok( result == FALSE, "got result %d.\n", result );
        ref = IDispatcherQueue2_Release( dispatcher_queue2 );
        ok( ref == 3, "got ref %ld.\n", ref );
    }

    hr = create_typed_event_handler_dispatcher_queue( &event_handler_iface );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    event_handler = impl_from_ITypedEventHandler_DispatcherQueue_IInspectable( event_handler_iface );
    hr = IDispatcherQueue_add_ShutdownCompleted( dispatcher_queue, event_handler_iface, &token );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IDispatcherQueueController_ShutdownQueueAsync( dispatcher_queue_controller, NULL );
    ok( hr == E_POINTER || hr == 0x80000005 /* win10 22h2 */, "got hr %#lx.\n", hr );
    hr = IDispatcherQueueController_ShutdownQueueAsync( dispatcher_queue_controller, &operation );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_interface( operation, &IID_IInspectable );
    check_interface( operation, &IID_IAgileObject );
    check_interface( operation, &IID_IAsyncAction );

    hr = IAsyncAction_QueryInterface( operation, &IID_IAsyncInfo, (void **)&async_info );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IAsyncInfo_get_Status( async_info, &status );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( status == Started, "got status %d.\n", status );

    /* shutdown waits for queued handlers */
    if (winetest_platform_is_wine) Sleep( 200 );
    ret = WaitForSingleObject( event_handler->event, 100 );
    todo_wine ok( ret == WAIT_TIMEOUT, "Unexpected wait result %lu.\n", ret );

    SetEvent( queue_handler->event );
    ret = WaitForSingleObject( event_handler->event, 5000 );
    ok( !ret, "Unexpected wait result %lu.\n", ret );

    hr = IAsyncInfo_get_Status( async_info, &status );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( status == Completed, "got status %d.\n", status );

    hr = IAsyncInfo_Close( async_info );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ref = IAsyncInfo_Release( async_info );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IAsyncAction_Release( operation );
    ok( ref == 1, "got ref %ld.\n", ref );
    hr = IDispatcherQueue_remove_ShutdownCompleted( dispatcher_queue, token );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IDispatcherQueue_TryEnqueue( dispatcher_queue, handler_iface, &result );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( result == FALSE, "got result %d.\n", result );

    ref = ITypedEventHandler_DispatcherQueue_IInspectable_Release( event_handler_iface );
    ok( ref == 0, "got ref %ld.\n", ref );
    ref = IDispatcherQueueHandler_Release( handler_iface );
    ok( ref == 0, "got ref %ld.\n", ref );
    IDispatcherQueue_Release( dispatcher_queue );
    IDispatcherQueueController_Release( dispatcher_queue_controller );
done:
    ref = IDispatcherQueueControllerStatics_Release( dispatcher_queue_controller_statics );
    ok( ref == 2, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(coremessaging)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_CreateDispatcherQueueController();
    test_DispatcherQueueController_Statics();

    RoUninitialize();
}
