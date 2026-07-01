/*
 * Copyright (C) 2025 Mohamad Al-Jaf
 * Copyright 2026 Conor McCarthy for CodeWeavers
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
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Storage_Streams
#include "windows.storage.streams.h"
#include "robuffer.h"

#include "wine/test.h"

#define check_interface( obj, iid, is_broken ) check_interface_( __LINE__, obj, iid, is_broken )
static void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL is_broken )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK || broken( is_broken && hr == E_NOINTERFACE ), "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

#define DEFINE_ASYNC_COMPLETED_HANDLER( name, iface_type, async_type )                              \
    struct name                                                                                     \
    {                                                                                               \
        iface_type iface_type##_iface;                                                              \
        LONG ref;                                                                                   \
        BOOL invoked;                                                                               \
        HANDLE event;                                                                               \
    };                                                                                              \
                                                                                                    \
    static HRESULT WINAPI name##_QueryInterface( iface_type *iface, REFIID iid, void **out )        \
    {                                                                                               \
        if (IsEqualGUID( iid, &IID_IUnknown ) || IsEqualGUID( iid, &IID_IAgileObject ) ||           \
            IsEqualGUID( iid, &IID_##iface_type ))                                                  \
        {                                                                                           \
            IUnknown_AddRef( iface );                                                               \
            *out = iface;                                                                           \
            return S_OK;                                                                            \
        }                                                                                           \
                                                                                                    \
        trace( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );            \
        *out = NULL;                                                                                \
        return E_NOINTERFACE;                                                                       \
    }                                                                                               \
                                                                                                    \
    static ULONG WINAPI name##_AddRef( iface_type *iface )                                          \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        return InterlockedIncrement( &impl->ref );                                                  \
    }                                                                                               \
                                                                                                    \
    static ULONG WINAPI name##_Release( iface_type *iface )                                         \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        ULONG ref = InterlockedDecrement( &impl->ref );                                             \
        if (!ref) free( impl );                                                                     \
        return ref;                                                                                 \
    }                                                                                               \
                                                                                                    \
    static HRESULT WINAPI name##_Invoke( iface_type *iface, async_type *async, AsyncStatus status ) \
    {                                                                                               \
        struct name *impl = CONTAINING_RECORD( iface, struct name, iface_type##_iface );            \
        ok( !impl->invoked, "invoked twice\n" );                                                    \
        impl->invoked = TRUE;                                                                       \
        if (impl->event) SetEvent( impl->event );                                                   \
        return S_OK;                                                                                \
    }                                                                                               \
                                                                                                    \
    static iface_type##Vtbl name##_vtbl =                                                           \
    {                                                                                               \
        name##_QueryInterface,                                                                      \
        name##_AddRef,                                                                              \
        name##_Release,                                                                             \
        name##_Invoke,                                                                              \
    };                                                                                              \
                                                                                                    \
    static iface_type *name##_create( HANDLE event )                                                \
    {                                                                                               \
        struct name *impl;                                                                          \
                                                                                                    \
        if (!(impl = calloc( 1, sizeof(*impl) ))) return NULL;                                      \
        impl->iface_type##_iface.lpVtbl = &name##_vtbl;                                             \
        impl->event = event;                                                                        \
        impl->ref = 1;                                                                              \
                                                                                                    \
        return &impl->iface_type##_iface;                                                           \
    }                                                                                               \
                                                                                                    \
    static DWORD await_##async_type( async_type *async, DWORD timeout )                             \
    {                                                                                               \
        iface_type *handler;                                                                        \
        HANDLE event;                                                                               \
        HRESULT hr;                                                                                 \
        DWORD ret;                                                                                  \
                                                                                                    \
        event = CreateEventW( NULL, FALSE, FALSE, NULL );                                           \
        ok( !!event, "CreateEventW failed, error %lu\n", GetLastError() );                          \
        handler = name##_create( event );                                                           \
        ok( !!handler, "Failed to create completion handler\n" );                                   \
        hr = async_type##_put_Completed( async, handler );                                          \
        ok( hr == S_OK, "put_Completed returned %#lx\n", hr );                                      \
        ret = WaitForSingleObject( event, timeout );                                                \
        ok( !ret, "WaitForSingleObject returned %#lx\n", ret );                                     \
        CloseHandle( event );                                                                       \
        iface_type##_Release( handler );                                                            \
                                                                                                    \
        return ret;                                                                                 \
    }

static HRESULT get_activation_factory( const WCHAR *name, IActivationFactory **factory )
{
    HSTRING str = NULL;
    HRESULT hr;

    hr = WindowsCreateString( name, wcslen( name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );

    if (hr == REGDB_E_CLASSNOTREG)
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( name ) );
    else if (hr == CLASS_E_CLASSNOTAVAILABLE)
        skip( "%s runtimeclass not available, skipping tests.\n", wine_dbgstr_w( name ) );

    return hr;
}

static void test_RandomAccessStreamReference(void)
{
    static const WCHAR *random_access_stream_reference_statics_name = L"Windows.Storage.Streams.RandomAccessStreamReference";
    IRandomAccessStreamReferenceStatics *random_access_stream_reference_statics = (void *)0xdeadbeef;
    IRandomAccessStreamReference *random_access_stream_reference = (void *)0xdeadbeef;
    IActivationFactory *factory = (void *)0xdeadbeef;
    HSTRING str = NULL;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( random_access_stream_reference_statics_name, wcslen( random_access_stream_reference_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( random_access_stream_reference_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, FALSE );
    check_interface( factory, &IID_IInspectable, FALSE );
    check_interface( factory, &IID_IAgileObject, TRUE /* Supported after Windows 10 1607 */ );

    hr = IActivationFactory_QueryInterface( factory, &IID_IRandomAccessStreamReferenceStatics, (void **)&random_access_stream_reference_statics );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = IRandomAccessStreamReferenceStatics_CreateFromStream( random_access_stream_reference_statics, NULL, &random_access_stream_reference );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    ok( random_access_stream_reference == NULL, "IRandomAccessStreamReferenceStatics_CreateFromStream returned %p.\n", random_access_stream_reference );

    ref = IRandomAccessStreamReferenceStatics_Release( random_access_stream_reference_statics );
    ok( ref == 1, "got ref %ld.\n", ref );
    ref = IActivationFactory_Release( factory );
    ok( ref == 0, "got ref %ld.\n", ref );
}

DEFINE_ASYNC_COMPLETED_HANDLER( async_uint32_uint32_completed_handler, \
        IAsyncOperationWithProgressCompletedHandler_UINT32_UINT32, IAsyncOperationWithProgress_UINT32_UINT32 )

DEFINE_ASYNC_COMPLETED_HANDLER( async_buffer_uint32_completed_handler, \
        IAsyncOperationWithProgressCompletedHandler_IBuffer_UINT32, IAsyncOperationWithProgress_IBuffer_UINT32 )

#define check_async_info( a, b, c ) check_async_info_( __LINE__, a, b, c )
static void check_async_info_( int line, void *async, AsyncStatus expect_status, HRESULT expect_hr )
{
    AsyncStatus async_status;
    IAsyncInfo *async_info;
    HRESULT hr, async_hr;
    UINT32 async_id;

    hr = IInspectable_QueryInterface( async, &IID_IAsyncInfo, (void **)&async_info );
    ok_(__FILE__, line)( hr == S_OK, "QueryInterface returned %#lx\n", hr );

    async_id = 0xdeadbeef;
    hr = IAsyncInfo_get_Id( async_info, &async_id );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Id returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Id returned %#lx\n", hr );
    ok_(__FILE__, line)( !!async_id, "got id %u\n", async_id );

    async_status = 0xdeadbeef;
    hr = IAsyncInfo_get_Status( async_info, &async_status );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_Status returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_Status returned %#lx\n", hr );
    ok_(__FILE__, line)( async_status == expect_status, "got status %u\n", async_status );

    async_hr = 0xdeadbeef;
    hr = IAsyncInfo_get_ErrorCode( async_info, &async_hr );
    if (expect_status < 4) ok_(__FILE__, line)( hr == S_OK, "get_ErrorCode returned %#lx\n", hr );
    else ok_(__FILE__, line)( hr == E_ILLEGAL_METHOD_CALL, "get_ErrorCode returned %#lx\n", hr );
    if (expect_status < 4) ok_(__FILE__, line)( async_hr == expect_hr, "got error %#lx\n", async_hr );
    else ok_(__FILE__, line)( async_hr == E_ILLEGAL_METHOD_CALL, "got error %#lx\n", async_hr );

    IAsyncInfo_Release( async_info );
}

#define output_stream_write( a, b, c ) output_stream_write_(__LINE__, a, b, c )
static void output_stream_write_( unsigned int line, IOutputStream *output_stream, IBuffer *buffer, UINT32 count )
{
    IAsyncOperationWithProgress_UINT32_UINT32 *operation;
    UINT32 written;
    HRESULT hr;
    UINT res;

    hr = IBuffer_put_Length( buffer, count );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IOutputStream_WriteAsync( output_stream, buffer, &operation );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    res = await_IAsyncOperationWithProgress_UINT32_UINT32( operation, 1000 );
    ok( res == 0, "await_IAsyncOperationWithProgress_UINT32_UINT32 returned %#x\n", res );
    check_async_info( operation, Completed, S_OK );
    hr = IAsyncOperationWithProgress_UINT32_UINT32_GetResults( operation, &written );
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    ok_(__FILE__, line)( written == count, "wrote %u bytes.\n", written );
    IAsyncOperationWithProgress_UINT32_UINT32_Release( operation );
}

#define input_stream_read( a, b, c, d, e, f ) input_stream_read_(__LINE__, a, b, c, d, e, f )
static void input_stream_read_( unsigned int line, IInputStream *input_stream, IBuffer *buffer, UINT32 count,
        AsyncStatus expect_status, HRESULT expect_hr, IBuffer **res_buffer )
{
    IAsyncOperationWithProgress_IBuffer_UINT32 *operation;
    HRESULT hr;
    UINT res;

    hr = IInputStream_ReadAsync( input_stream, buffer, count, 0, &operation );
    todo_wine
    ok_(__FILE__, line)( hr == S_OK, "got hr %#lx.\n", hr );
    if (FAILED(hr)) return;
    res = await_IAsyncOperationWithProgress_IBuffer_UINT32( operation, 1000 );
    ok_(__FILE__, line)( res == 0, "await_IAsyncOperationWithProgress_IBuffer_UINT32 returned %#x\n", res );
    check_async_info( operation, expect_status, expect_hr );
    hr = IAsyncOperationWithProgress_IBuffer_UINT32_GetResults( operation, res_buffer );
    ok_(__FILE__, line)( hr == expect_hr, "got hr %#lx.\n", hr );
    IAsyncOperationWithProgress_IBuffer_UINT32_Release( operation );
}

static BYTE *buffer_get_data( IBuffer *buffer )
{
    IBufferByteAccess *access;
    BYTE *data = NULL;
    HRESULT hr;

    hr = IBuffer_QueryInterface( buffer, &IID_IBufferByteAccess, (void **)&access );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IBufferByteAccess_Buffer( access, &data );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IBufferByteAccess_Release( access );

    return data;
}

static void test_InMemoryRandomAccessStream(void)
{
    static const WCHAR *in_memory_stream_statics_name = L"Windows.Storage.Streams.InMemoryRandomAccessStream";
    static const WCHAR *buffer_statics_name = L"Windows.Storage.Streams.Buffer";
    IRandomAccessStream *in_memory_stream = (void *)0xdeadbeef;
    IInspectable *in_memory_inspectable = (void *)0xdeadbeef;
    IAsyncOperationWithProgress_IBuffer_UINT32 *operation;
    IAsyncOperationWithProgress_UINT32_UINT32 *write_op;
    IActivationFactory *factory = (void *)0xdeadbeef;
    const UINT64 uint64_value = 0xdeadbeefcafef00d;
    IBuffer *buffer, *res_buffer = NULL;
    IBufferFactory *buffer_factory;
    IOutputStream *output_stream;
    IInputStream *input_stream;
    BYTE byte_value = 0xab;
    BOOLEAN value_bool = 0;
    IClosable *closable;
    UINT64 value64 = 0;
    BYTE *data = NULL;
    UINT32 value = 0;
    HRESULT hr;
    LONG ref;

    if (FAILED(hr = get_activation_factory( in_memory_stream_statics_name, &factory )))
        return;

    check_interface( factory, &IID_IUnknown, FALSE );
    check_interface( factory, &IID_IInspectable, FALSE );
    check_interface( factory, &IID_IAgileObject, TRUE /* Supported after Windows 10 1607 */ );

    hr = IActivationFactory_ActivateInstance( factory, &in_memory_inspectable );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ref = IActivationFactory_Release( factory );
    flaky ok( ref == 1, "got ref %ld.\n", ref );
    hr = IInspectable_QueryInterface( in_memory_inspectable, &IID_IRandomAccessStream, (void **)&in_memory_stream );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IInspectable_Release( in_memory_inspectable );

    check_interface( in_memory_stream, &IID_IClosable, FALSE );

    hr = IRandomAccessStream_QueryInterface( in_memory_stream, &IID_IInputStream, (void **)&input_stream );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    check_interface( input_stream, &IID_IUnknown, FALSE );
    check_interface( input_stream, &IID_IInspectable, FALSE );
    check_interface( input_stream, &IID_IRandomAccessStream, FALSE );
    check_interface( input_stream, &IID_IOutputStream, FALSE );
    check_interface( input_stream, &IID_IClosable, FALSE );

    hr = IRandomAccessStream_QueryInterface( in_memory_stream, &IID_IOutputStream, (void **)&output_stream );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    check_interface( output_stream, &IID_IUnknown, FALSE );
    check_interface( output_stream, &IID_IInspectable, FALSE );
    check_interface( output_stream, &IID_IRandomAccessStream, FALSE );
    check_interface( output_stream, &IID_IInputStream, FALSE );

    if (FAILED(hr = get_activation_factory( buffer_statics_name, &factory )))
        return;

    hr = IActivationFactory_QueryInterface( factory, &IID_IBufferFactory, (void **)&buffer_factory );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IBufferFactory_Create( buffer_factory, 17, &buffer );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IBufferFactory_Release( buffer_factory );

    IActivationFactory_Release( factory );

    data = buffer_get_data( buffer );

    hr = IRandomAccessStream_get_Size( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 0, "got size %I64u.\n", value64 );
    hr = IRandomAccessStream_get_Position( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 0, "got pos %I64u.\n", value64 );
    hr = IRandomAccessStream_Seek( in_memory_stream, 16 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IRandomAccessStream_get_Size( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 0, "got size %I64u.\n", value64 );
    hr = IRandomAccessStream_get_Position( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 16, "got pos %I64u.\n", value64 );

    /* Write 0 bytes at position 16 */
    output_stream_write( output_stream, buffer, 0 );
    hr = IRandomAccessStream_get_Size( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 0, "got size %I64u.\n", value64 );
    /* Write 1 byte at position 16 */
    data[0] = byte_value;
    output_stream_write( output_stream, buffer, 1 );
    hr = IRandomAccessStream_get_Size( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 17, "got size %I64u.\n", value64 );

    hr = IRandomAccessStream_Seek( in_memory_stream, 0 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IRandomAccessStream_get_Position( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 0, "got pos %I64u.\n", value64 );
    memcpy( data, &uint64_value, 8 );
    output_stream_write( output_stream, buffer, 8 );
    hr = IRandomAccessStream_get_Size( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 17, "got size %I64u.\n", value64 );
    hr = IRandomAccessStream_get_Position( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 8, "got pos %I64u.\n", value64 );

    hr = IOutputStream_WriteAsync( output_stream, NULL, &write_op );
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    /* Crashes on Windows if the op pointer is null
     * hr = IOutputStream_WriteAsync( output_stream, NULL, NULL ); */

    memset( data, 0xcd, 17 );

    hr = IRandomAccessStream_Seek( in_memory_stream, 0 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    res_buffer = NULL;
    input_stream_read( input_stream, buffer, 0, Completed, S_OK, &res_buffer );
    todo_wine
    ok( res_buffer == buffer, "got different buffer.\n" );
    if (!res_buffer) IBuffer_AddRef( res_buffer = buffer );
    hr = IBuffer_get_Length( res_buffer, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine
    ok( value == 0, "got read %u.\n", value );
    IBuffer_Release( res_buffer );

    res_buffer = (void *)0xdeadbeef;
    input_stream_read( input_stream, buffer, 20, Error, E_INVALIDARG, &res_buffer );
    todo_wine
    ok( !res_buffer, "got res_buffer %p.\n", res_buffer );

    res_buffer = NULL;
    input_stream_read( input_stream, buffer, 17, Completed, S_OK, &res_buffer );
    todo_wine
    ok( res_buffer == buffer, "got different buffer.\n" );
    if (!res_buffer) IBuffer_AddRef( res_buffer = buffer );
    hr = IBuffer_get_Length( res_buffer, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine
    ok( value == 17, "got read %u.\n", value );
    hr = IRandomAccessStream_get_Position( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine
    ok( value64 == 17, "got pos %I64u.\n", value64 );
    data = buffer_get_data( res_buffer );
    memcpy( &value64, data, sizeof(value64 ) );
    todo_wine
    ok( value64 == uint64_value, "got value64 %#I64x.\n", value64 );
    todo_wine
    ok ( data[16] == byte_value, "got byte value %#x.\n", data[16]);
    IBuffer_Release( res_buffer );

    hr = IRandomAccessStream_Seek( in_memory_stream, 18 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    res_buffer = NULL;
    input_stream_read( input_stream, buffer, 1, Completed, S_OK, &res_buffer );
    todo_wine
    ok( res_buffer == buffer, "got different buffer.\n" );
    if (!res_buffer) IBuffer_AddRef( res_buffer = buffer );
    hr = IBuffer_get_Length( res_buffer, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine
    ok( value == 0, "got read %u.\n", value );
    IBuffer_Release( res_buffer );

    operation = (void *)0xdeadbeef;
    hr = IInputStream_ReadAsync( input_stream, NULL, 1, 0, &operation );
    todo_wine
    ok( hr == E_POINTER, "got hr %#lx.\n", hr );
    ok( !operation, "got operation %p.\n", operation );
    /* Crashes on Windows if the op pointer is null
     * hr = IInputStream_ReadAsync( input_stream, NULL, 1, 0, NULL ); */

    value_bool = 0;
    hr = IRandomAccessStream_get_CanRead( in_memory_stream, &value_bool );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value_bool == TRUE, "got bool %#x.\n", value_bool );
    value_bool = 0;
    hr = IRandomAccessStream_get_CanWrite( in_memory_stream, &value_bool );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value_bool == TRUE, "got bool %#x.\n", value_bool );

    /* Test large put size and read. Native truncates put_Size() to 32-bit,
     * but only allows reading from positions less than 1 << 31
     * Both of these seem potentially subject to change, so are untested. */
    hr = IRandomAccessStream_put_Size( in_memory_stream, 0x100000 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IRandomAccessStream_get_Size( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 0x100000, "got size %I64u.\n", value64 );
    hr = IRandomAccessStream_get_Position( in_memory_stream, &value64 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value64 == 18, "got pos %I64u.\n", value64 );

    hr = IRandomAccessStream_Seek( in_memory_stream, 0xffff8 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    res_buffer = NULL;
    input_stream_read( input_stream, buffer, 8, Completed, S_OK, &res_buffer );
    todo_wine
    ok( res_buffer == buffer, "got different buffer.\n" );
    if (!res_buffer) IBuffer_AddRef( res_buffer = buffer );
    hr = IBuffer_get_Length( res_buffer, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == 8, "got read %u.\n", value );
    data = buffer_get_data( res_buffer );
    memcpy( &value64, data, sizeof(value64) );
    todo_wine
    ok( value64 == 0, "got value64 %#I64x.\n", value64 );
    IBuffer_Release( res_buffer );

    hr = IOutputStream_QueryInterface( output_stream, &IID_IClosable, (void **)&closable );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    hr = IClosable_Close( closable );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    IClosable_Release( closable );

    write_op = (void *)0xdeadbeef;
    hr = IOutputStream_WriteAsync( output_stream, buffer, &write_op );
    ok( hr == RO_E_CLOSED, "got hr %#lx.\n", hr );
    ok( !write_op, "got operation %p.\n", write_op );
    operation = (void *)0xdeadbeef;
    hr = IInputStream_ReadAsync( input_stream, buffer, 0, 0, &operation );
    ok( hr == RO_E_CLOSED, "got hr %#lx.\n", hr );
    ok( !operation, "got operation %p.\n", operation );

    hr = IRandomAccessStream_Seek( in_memory_stream, 0 );
    ok( hr == RO_E_CLOSED, "got hr %#lx.\n", hr );
    value64 = uint64_value;
    hr = IRandomAccessStream_get_Size( in_memory_stream, &value64 );
    ok( hr == RO_E_CLOSED, "got hr %#lx.\n", hr );
    ok( value64 == 0, "got size %I64u.\n", value64 );
    hr = IRandomAccessStream_put_Size( in_memory_stream, 1 );
    ok( hr == RO_E_CLOSED, "got hr %#lx.\n", hr );
    value64 = uint64_value;
    hr = IRandomAccessStream_get_Position( in_memory_stream, &value64 );
    ok( hr == RO_E_CLOSED, "got hr %#lx.\n", hr );
    todo_wine
    ok( value64 == 0x100000, "got pos %I64u.\n", value64 );
    value_bool = 0;
    hr = IRandomAccessStream_get_CanRead( in_memory_stream, &value_bool );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value_bool == TRUE, "got bool %#x.\n", value_bool );
    value_bool = 0;
    hr = IRandomAccessStream_get_CanWrite( in_memory_stream, &value_bool );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value_bool == TRUE, "got bool %#x.\n", value_bool );

    IBuffer_Release( buffer );
    IOutputStream_Release( output_stream );
    IInputStream_Release( input_stream );

    ref = IRandomAccessStream_Release( in_memory_stream );
    ok( ref == 0, "got ref %ld.\n", ref );
}

START_TEST(storage)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_RandomAccessStreamReference();
    test_InMemoryRandomAccessStream();

    RoUninitialize();
}
