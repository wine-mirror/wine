/*
 * Unit test suite for kernel mode graphics driver
 *
 * Copyright 2019 Zhiyi Zhang
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

#include <stdarg.h>

#define COBJMACROS
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "dwmapi.h"
#include "ddk/d3dkmthk.h"
#include "initguid.h"
#include "setupapi.h"
#include "ntddvdeo.h"
#include "devpkey.h"
#include "cfgmgr32.h"

#include "d3d9.h"
#include "dxgi1_3.h"
#include "wine/test.h"

#define D3DUSAGE_SURFACE    0x8000000
#define D3DUSAGE_LOCKABLE   0x4000000
#define D3DUSAGE_OFFSCREEN  0x0400000

static const WCHAR display1W[] = L"\\\\.\\DISPLAY1";

DEFINE_DEVPROPKEY(DEVPROPKEY_GPU_LUID, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2);

static const LUID luid_zero;
const GUID GUID_NULL = {0};

static const char *debugstr_luid( const LUID *luid )
{
    if (!luid) return "(null)";
    return wine_dbg_sprintf( "%04lx:%04lx", luid->LowPart, luid->HighPart );
}

static BOOL luid_equals( const LUID *a, const LUID *b )
{
    return a->HighPart == b->HighPart && a->LowPart == b->LowPart;
}

static const char *debugstr_ok( const char *cond )
{
    int c, n = 0;
    /* skip possible casts */
    while ((c = *cond++))
    {
        if (c == '(') n++;
        if (!n) break;
        if (c == ')') n--;
    }
    if (!strchr( cond - 1, '(' )) return wine_dbg_sprintf( "got %s", cond - 1 );
    return wine_dbg_sprintf( "%.*s returned", (int)strcspn( cond - 1, "( " ), cond - 1 );
}

#define ok_ex( r, op, e, t, f, ... )                                                               \
    do                                                                                             \
    {                                                                                              \
        t v = (r);                                                                                 \
        ok( v op (e), "%s " f "\n", debugstr_ok( #r ), v, ##__VA_ARGS__ );                         \
    } while (0)
#define ok_i4( r, op, e )   ok_ex( r, op, e, int, "%i" )
#define ok_u4( r, op, e )   ok_ex( r, op, e, UINT, "%u" )
#define ok_x4( r, op, e )   ok_ex( r, op, e, UINT, "%#x" )
#define ok_u1( r, op, e )   ok_ex( r, op, e, unsigned char, "%u" )
#define ok_ptr( r, op, e )  ok_ex( r, op, e, const void *, "%p" )

#define ok_ret( e, r )      ok_ex( r, ==, e, UINT_PTR, "%Iu, error %ld", GetLastError() )
#define ok_ref( e, r )      ok_ex( r, ==, e, LONG, "%ld" )
#define ok_hr( e, r )       ok_ex( r, ==, e, HRESULT, "%#lx" )
#define ok_nt( e, r )       ok_ex( r, ==, e, NTSTATUS, "%#lx" )
#define ok_vk( e, r )       ok_ex( r, ==, e, VkResult, "%d" )

static BOOL is_d3dkmt_handle( HANDLE handle )
{
    return (ULONG_PTR)handle & 0xc0000000;
}

#define run_in_process( a ) run_in_process_( __FILE__, __LINE__, a )
static void run_in_process_( const char *file, int line, const char *args )
{
    char cmdline[MAX_PATH * 2], test[MAX_PATH], *tmp, **argv;
    STARTUPINFOA startup = {.cb = sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION info = {0};
    const char *name;
    DWORD ret;
    int argc;

    name = file;
    if ((tmp = strrchr( name, '\\' ))) name = tmp;
    if ((tmp = strrchr( name, '/' ))) name = tmp;
    strcpy( test, name );
    if ((tmp = strrchr( test, '.' ))) *tmp = 0;

    argc = winetest_get_mainargs( &argv );
    sprintf( cmdline, "%s %s %s", argv[0], argc > 1 ? argv[1] : test, args );
    ret = CreateProcessA( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
    ok_(file, line)( ret, "CreateProcessA failed, error %lu\n", GetLastError() );
    if (!ret) return;

    wait_child_process( info.hProcess );
    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );
}

#define check_d3dkmt_global( a ) check_d3dkmt_global_( __LINE__, a )
static void check_d3dkmt_global_( int line, UINT_PTR handle )
{
    ok_( __FILE__, line )( handle & 0xc0000000, "got %#Ix\n", handle );
    ok_( __FILE__, line )( (handle & 0x3f) == 2, "got %#Ix\n", handle );
}

#define check_d3dkmt_local( a, b ) check_d3dkmt_local_( __LINE__, a, b )
static void check_d3dkmt_local_( int line, D3DKMT_HANDLE handle, D3DKMT_HANDLE *next_local )
{
    ok_(__FILE__, line)( handle & 0xc0000000, "got %#x\n", handle );
    ok_(__FILE__, line)( handle && !(handle & 0x3f), "got %#x\n", handle );
    if (next_local && *next_local) ok_(__FILE__, line)( handle == *next_local, "got %#x, expected %#x\n", handle, *next_local );
    if (next_local) *next_local = handle + 0x40;
}

static BOOL compare_unicode_string( const UNICODE_STRING *string, const WCHAR *expect )
{
    return string->Length == wcslen( expect ) * sizeof(WCHAR) &&
           !wcsnicmp( string->Buffer, expect, string->Length / sizeof(WCHAR) );
}

#define check_object_type( a, b ) _check_object_type( __LINE__, a, b )
static void _check_object_type( unsigned line, HANDLE handle, const WCHAR *expected_name )
{
    char buffer[1024];
    OBJECT_TYPE_INFORMATION *type = (OBJECT_TYPE_INFORMATION *)buffer;
    UNICODE_STRING expect;
    ULONG len = 0;
    NTSTATUS status;

    RtlInitUnicodeString( &expect, expected_name );

    memset( buffer, 0, sizeof(buffer) );
    status = NtQueryObject( handle, ObjectTypeInformation, buffer, sizeof(buffer), &len );
    ok_(__FILE__, line)( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    ok_(__FILE__, line)( len > sizeof(UNICODE_STRING), "unexpected len %lu\n", len );
    ok_(__FILE__, line)( len >= sizeof(*type) + type->TypeName.Length, "unexpected len %lu\n", len );
    ok_(__FILE__, line)( compare_unicode_string( &type->TypeName, expected_name ),
                         "wrong name %s\n", debugstr_w( type->TypeName.Buffer ) );
}

#define check_object_name( a, b ) _check_object_name( __LINE__, a, b )
static void _check_object_name( unsigned line, HANDLE handle, const WCHAR *expected_name )
{
    char buffer[1024];
    UNICODE_STRING *str = (UNICODE_STRING *)buffer, expect;
    ULONG len = 0;
    NTSTATUS status;

    RtlInitUnicodeString( &expect, expected_name );

    memset( buffer, 0, sizeof(buffer) );
    status = NtQueryObject( handle, ObjectNameInformation, buffer, sizeof(buffer), &len );
    ok_(__FILE__, line)( status == STATUS_SUCCESS, "NtQueryObject failed %lx\n", status );
    ok_(__FILE__, line)( len >= sizeof(OBJECT_NAME_INFORMATION) + str->Length, "unexpected len %lu\n", len );
    ok_(__FILE__, line)( compare_unicode_string( str, expected_name ), "got %s, expected %s\n",
                         debugstr_w(str->Buffer), debugstr_w(expected_name) );
}

struct dxgi_runtime_desc
{
    UINT                        size;
    UINT                        version;
    UINT                        width;
    UINT                        height;
    DXGI_FORMAT                 format;
    UINT                        unknown_0;
    UINT                        unknown_1;
    UINT                        keyed_mutex;
    D3DKMT_HANDLE               mutex_handle;
    D3DKMT_HANDLE               sync_handle;
    UINT                        nt_shared;
    UINT                        unknown_2;
    UINT                        unknown_3;
    UINT                        unknown_4;
};

#define check_dxgi_runtime_desc( a, b ) check_dxgi_runtime_desc_( a, b, FALSE )
static void check_dxgi_runtime_desc_( struct dxgi_runtime_desc *desc, const struct dxgi_runtime_desc *expect, BOOL d3d12 )
{
    ok_x4( desc->size, ==, expect->size );
    ok_x4( desc->version, ==, expect->version );
    ok_x4( desc->width, ==, expect->width );
    ok_x4( desc->height, ==, expect->height );
    ok_x4( desc->format, ==, expect->format );
    if (expect->version == 1) ok_x4( desc->unknown_0, ==, 1 );
    else if (!d3d12) ok_x4( desc->unknown_0, ==, 0 );
    ok_x4( desc->unknown_1, ==, 0 );
    ok_x4( desc->keyed_mutex, ==, expect->keyed_mutex );
    if (desc->keyed_mutex) check_d3dkmt_global( desc->mutex_handle );
    if (desc->keyed_mutex) check_d3dkmt_global( desc->sync_handle );
    ok_x4( desc->nt_shared, ==, expect->nt_shared );
    ok_x4( desc->unknown_2, ==, 0 );
    ok_x4( desc->unknown_3, ==, 0 );
    if (!d3d12) ok_x4( desc->unknown_4, ==, 0 );
}

struct d3d9_runtime_desc
{
    struct dxgi_runtime_desc    dxgi;
    D3DFORMAT                   format;
    D3DRESOURCETYPE             type;
    UINT                        usage;
    union
    {
        struct
        {
            UINT                unknown_0;
            UINT                width;
            UINT                height;
            UINT                levels;
            UINT                depth;
        } texture;
        struct
        {
            UINT                unknown_0;
            UINT                unknown_1;
            UINT                unknown_2;
            UINT                width;
            UINT                height;
        } surface;
        struct
        {
            UINT                unknown_0;
            UINT                width;
            UINT                format;
            UINT                unknown_1;
            UINT                unknown_2;
        } buffer;
    };
};

C_ASSERT( sizeof(struct d3d9_runtime_desc) == 0x58 );

static void check_d3d9_runtime_desc( struct d3d9_runtime_desc *desc, const struct d3d9_runtime_desc *expect )
{
    check_dxgi_runtime_desc( &desc->dxgi, &expect->dxgi );

    ok_x4( desc->format, ==, expect->format );
    ok_x4( desc->type, ==, expect->type );
    ok_x4( desc->usage, ==, expect->usage );
    switch (desc->type)
    {
    case D3DRTYPE_TEXTURE:
        ok_x4( desc->texture.unknown_0, ==, 0 );
        ok_x4( desc->texture.width, ==, expect->dxgi.width );
        ok_x4( desc->texture.height, ==, expect->dxgi.height );
        ok_x4( desc->texture.levels, ==, 1 );
        ok_x4( desc->texture.depth, ==, 0 );
        break;
    case D3DRTYPE_CUBETEXTURE:
        ok_x4( desc->texture.unknown_0, ==, 0 );
        ok_x4( desc->texture.width, ==, expect->dxgi.width );
        ok_x4( desc->texture.height, ==, expect->dxgi.width );
        ok_x4( desc->texture.levels, ==, 1 );
        ok_x4( desc->texture.depth, ==, 0 );
        break;
    case D3DRTYPE_VOLUMETEXTURE:
        ok_x4( desc->texture.unknown_0, ==, 0 );
        ok_x4( desc->texture.width, ==, expect->dxgi.width );
        ok_x4( desc->texture.height, ==, expect->dxgi.height );
        ok_x4( desc->texture.levels, ==, 1 );
        ok_x4( desc->texture.depth, ==, expect->texture.depth );
        break;
    case D3DRTYPE_SURFACE:
        ok_x4( desc->surface.unknown_0, ==, 0 );
        ok_x4( desc->surface.unknown_1, ==, 0 );
        ok_x4( desc->surface.unknown_2, ==, 0 );
        ok_x4( desc->surface.width, ==, expect->dxgi.width );
        ok_x4( desc->surface.height, ==, expect->dxgi.height );
        break;
    case D3DRTYPE_VERTEXBUFFER:
    case D3DRTYPE_INDEXBUFFER:
        ok_x4( desc->buffer.unknown_0, ==, 0 );
        ok_x4( desc->buffer.width, ==, expect->dxgi.width );
        ok_x4( desc->buffer.format, ==, expect->buffer.format );
        ok_x4( desc->buffer.unknown_1, ==, 0 );
        ok_x4( desc->buffer.unknown_2, ==, 0 );
        break;
    default:
        ok( 0, "unexpected\n" );
        break;
    }
}

static void get_d3dkmt_resource_desc( LUID luid, HANDLE handle, BOOL expect_global, UINT size, char *buffer )
{
    D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE query_nt = {0};
    D3DKMT_OPENADAPTERFROMLUID open_adapter = {0};
    D3DKMT_OPENRESOURCEFROMNTHANDLE open_nt = {0};
    D3DDDI_OPENALLOCATIONINFO2 open_alloc = {0};
    D3DKMT_DESTROYDEVICE destroy_device = {0};
    D3DKMT_CREATEDEVICE create_device = {0};
    D3DKMT_CLOSEADAPTER close_adapter = {0};
    D3DKMT_DESTROYALLOCATION destroy = {0};
    D3DKMT_QUERYRESOURCEINFO query = {0};
    D3DKMT_OPENRESOURCE open = {0};

    char resource_data[0x100] = {0};
    char driver_data[0x4000] = {0};
    D3DKMT_HANDLE resource;
    NTSTATUS status;

    todo_wine ok_ptr( handle, !=, NULL );
    if (!handle) return;

    open_adapter.AdapterLuid = luid;
    status = D3DKMTOpenAdapterFromLuid(&open_adapter);
    ok_nt( status, STATUS_SUCCESS );
    check_d3dkmt_local(open_adapter.hAdapter, NULL);
    create_device.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCreateDevice(&create_device);
    ok_nt( status, STATUS_SUCCESS );
    check_d3dkmt_local(create_device.hDevice, NULL);

    if (expect_global) check_d3dkmt_global( (UINT_PTR)handle );
    else check_object_type( handle, L"DxgkSharedResource" );

    if (is_d3dkmt_handle( handle ))
    {
        memset( buffer, 0xcd, size );
        query.hDevice = create_device.hDevice;
        query.hGlobalShare = HandleToUlong( handle );
        query.pPrivateRuntimeData = buffer;
        query.PrivateRuntimeDataSize = size;
        status = D3DKMTQueryResourceInfo( &query );
        ok_nt( STATUS_SUCCESS, status );
        if (size) ok_u4( query.PrivateRuntimeDataSize, ==, size );
        else ok( query.PrivateRuntimeDataSize == 0 || query.PrivateRuntimeDataSize == 0x68 /* NVIDIA */, "got %#x\n", query.PrivateRuntimeDataSize );
        ok_u4( query.TotalPrivateDriverDataSize, <, sizeof(driver_data) );
        ok_u4( query.ResourcePrivateDriverDataSize, <, sizeof(driver_data) );
        ok_u4( query.NumAllocations, ==, 1 );
        if (size) ok_u1( *buffer, ==, 0xcd );

        memset( buffer, 0xcd, size );
        open.hDevice = query.hDevice;
        open.hGlobalShare = query.hGlobalShare;
        open.NumAllocations = 1;
        open.pOpenAllocationInfo2 = &open_alloc;
        open.pPrivateRuntimeData = buffer;
        open.PrivateRuntimeDataSize = query.PrivateRuntimeDataSize;
        open.pResourcePrivateDriverData = resource_data;
        open.ResourcePrivateDriverDataSize = query.ResourcePrivateDriverDataSize;
        open.pTotalPrivateDriverDataBuffer = driver_data;
        open.TotalPrivateDriverDataBufferSize = query.TotalPrivateDriverDataSize;
        status = D3DKMTOpenResource2( &open );
        ok_nt( STATUS_SUCCESS, status );
        check_d3dkmt_local( open.hResource, NULL );
        if (size) ok_u4( open.PrivateRuntimeDataSize, ==, size );
        else ok( open.PrivateRuntimeDataSize == 0 || open.PrivateRuntimeDataSize == 0x68 /* NVIDIA */, "got %#x\n", open.PrivateRuntimeDataSize );
        ok_u4( open.TotalPrivateDriverDataBufferSize, <, sizeof(driver_data) );
        ok_u4( open.ResourcePrivateDriverDataSize, <, sizeof(driver_data) );
        ok_u4( open.NumAllocations, ==, 1 );
        check_d3dkmt_local( open_alloc.hAllocation, NULL );

        resource = open.hResource;
    }
    else
    {
        memset( buffer, 0xcd, size );
        query_nt.hDevice = create_device.hDevice;
        query_nt.hNtHandle = handle;
        query_nt.pPrivateRuntimeData = buffer;
        query_nt.PrivateRuntimeDataSize = size;
        status = D3DKMTQueryResourceInfoFromNtHandle( &query_nt );
        ok_nt( STATUS_SUCCESS, status );
        if (size) ok_u4( query_nt.PrivateRuntimeDataSize, ==, size );
        else ok( query_nt.PrivateRuntimeDataSize == 0 || query_nt.PrivateRuntimeDataSize == 0x68 /* NVIDIA */, "got %#x\n", query_nt.PrivateRuntimeDataSize );
        ok_u4( query_nt.TotalPrivateDriverDataSize, <, sizeof(driver_data) );
        ok_u4( query_nt.ResourcePrivateDriverDataSize, <, sizeof(driver_data) );
        ok_u4( query_nt.NumAllocations, ==, 1 );
        if (size) ok_u1( *buffer, ==, 0xcd );

        memset( buffer, 0xcd, size );
        open_nt.hDevice = query_nt.hDevice;
        open_nt.hNtHandle = query_nt.hNtHandle;
        open_nt.NumAllocations = 1;
        open_nt.pOpenAllocationInfo2 = &open_alloc;
        open_nt.pPrivateRuntimeData = buffer;
        open_nt.PrivateRuntimeDataSize = query_nt.PrivateRuntimeDataSize;
        open_nt.pResourcePrivateDriverData = resource_data;
        open_nt.ResourcePrivateDriverDataSize = query_nt.ResourcePrivateDriverDataSize;
        open_nt.pTotalPrivateDriverDataBuffer = driver_data;
        open_nt.TotalPrivateDriverDataBufferSize = query_nt.TotalPrivateDriverDataSize;
        status = D3DKMTOpenResourceFromNtHandle( &open_nt );
        ok_nt( STATUS_SUCCESS, status );
        check_d3dkmt_local( open_nt.hResource, NULL );
        if (size) ok_u4( open_nt.PrivateRuntimeDataSize, ==, size );
        else ok( open_nt.PrivateRuntimeDataSize == 0 || open_nt.PrivateRuntimeDataSize == 0x68 /* NVIDIA */, "got %#x\n", open_nt.PrivateRuntimeDataSize );
        ok_u4( open_nt.TotalPrivateDriverDataBufferSize, <, sizeof(driver_data) );
        ok_u4( open_nt.ResourcePrivateDriverDataSize, <, sizeof(driver_data) );
        ok_u4( open_nt.NumAllocations, ==, 1 );
        todo_wine check_d3dkmt_local( open_alloc.hAllocation, NULL );

        resource = open_nt.hResource;
    }

    destroy.hDevice = create_device.hDevice;
    destroy.hResource = resource;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    destroy_device.hDevice = create_device.hDevice;
    status = D3DKMTDestroyDevice(&destroy_device);
    ok_nt( status, STATUS_SUCCESS );
    close_adapter.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCloseAdapter(&close_adapter);
    ok_nt( status, STATUS_SUCCESS );
}

static BOOL get_primary_adapter_name( WCHAR *name )
{
    DISPLAY_DEVICEW dd;
    DWORD adapter_idx;

    dd.cb = sizeof(dd);
    for (adapter_idx = 0; EnumDisplayDevicesW( NULL, adapter_idx, &dd, 0 ); ++adapter_idx)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyW( name, dd.DeviceName );
            return TRUE;
        }
    }

    return FALSE;
}

static void test_D3DKMTOpenAdapterFromGdiDisplayName(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;
    DWORD i;

    lstrcpyW( open_adapter_gdi_desc.DeviceName, display1W );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );
    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    /* Invalid parameters */
    status = D3DKMTOpenAdapterFromGdiDisplayName( NULL );
    ok( status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status );

    memset( &open_adapter_gdi_desc, 0, sizeof(open_adapter_gdi_desc) );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status );

    /* Open adapter */
    for (i = 0; EnumDisplayDevicesW( NULL, i, &display_device, 0 ); ++i)
    {
        lstrcpyW( open_adapter_gdi_desc.DeviceName, display_device.DeviceName );
        status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
        if (display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
            ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        else
        {
            ok( status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#lx.\n", status );
            continue;
        }
        check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );
        ok( open_adapter_gdi_desc.AdapterLuid.LowPart || open_adapter_gdi_desc.AdapterLuid.HighPart,
            "Expect LUID not zero.\n" );

        close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
        status = D3DKMTCloseAdapter( &close_adapter_desc );
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    }
}

static void test_D3DKMTOpenAdapterFromHdc(void)
{
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_OPENADAPTERFROMHDC open_adapter_hdc_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    INT adapter_count = 0;
    NTSTATUS status;
    HDC hdc;
    DWORD i;

    /* Invalid parameters */
    /* Passing a NULL pointer crashes on Windows 10 >= 2004 */
    if (0) status = D3DKMTOpenAdapterFromHdc( NULL );

    memset( &open_adapter_hdc_desc, 0, sizeof(open_adapter_hdc_desc) );
    status = D3DKMTOpenAdapterFromHdc( &open_adapter_hdc_desc );
    todo_wine ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    /* Open adapter */
    for (i = 0; EnumDisplayDevicesW( NULL, i, &display_device, 0 ); ++i)
    {
        if (!(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) continue;

        adapter_count++;

        hdc = CreateDCW( 0, display_device.DeviceName, 0, NULL );
        open_adapter_hdc_desc.hDc = hdc;
        status = D3DKMTOpenAdapterFromHdc( &open_adapter_hdc_desc );
        todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        todo_wine check_d3dkmt_local( open_adapter_hdc_desc.hAdapter, &next_local );
        DeleteDC( hdc );

        if (status == STATUS_SUCCESS)
        {
            close_adapter_desc.hAdapter = open_adapter_hdc_desc.hAdapter;
            status = D3DKMTCloseAdapter( &close_adapter_desc );
            ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        }
    }

    /* HDC covering more than two adapters is invalid for D3DKMTOpenAdapterFromHdc */
    hdc = GetDC( 0 );
    open_adapter_hdc_desc.hDc = hdc;
    status = D3DKMTOpenAdapterFromHdc( &open_adapter_hdc_desc );
    ReleaseDC( 0, hdc );
    todo_wine ok( status == (adapter_count > 1 ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS),
                  "Got unexpected return code %#lx.\n", status );
    if (status == STATUS_SUCCESS)
    {
        check_d3dkmt_local( open_adapter_hdc_desc.hAdapter, &next_local );
        close_adapter_desc.hAdapter = open_adapter_hdc_desc.hAdapter;
        status = D3DKMTCloseAdapter( &close_adapter_desc );
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    }
}

static void test_D3DKMTEnumAdapters2(void)
{
    D3DKMT_ENUMADAPTERS2 enum_adapters_2_desc = {0};
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;
    UINT i;

    /* Invalid parameters */
    status = D3DKMTEnumAdapters2( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    /* Query the array to allocate */
    memset( &enum_adapters_2_desc, 0, sizeof(enum_adapters_2_desc) );
    status = D3DKMTEnumAdapters2( &enum_adapters_2_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ok( enum_adapters_2_desc.NumAdapters == 32 /* win10 and older */ || enum_adapters_2_desc.NumAdapters == 34 /* win11 */,
        "Got unexpected value %lu.\n", enum_adapters_2_desc.NumAdapters );

    /* Allocate the array */
    enum_adapters_2_desc.pAdapters = calloc( enum_adapters_2_desc.NumAdapters, sizeof(D3DKMT_ADAPTERINFO) );
    ok( !!enum_adapters_2_desc.pAdapters, "Expect not null.\n" );

    /* Enumerate adapters */
    status = D3DKMTEnumAdapters2( &enum_adapters_2_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ok( enum_adapters_2_desc.NumAdapters, "Expect not zero.\n" );

    for (i = 0; i < enum_adapters_2_desc.NumAdapters; ++i)
    {
        check_d3dkmt_local( enum_adapters_2_desc.pAdapters[i].hAdapter, &next_local );
        ok( enum_adapters_2_desc.pAdapters[i].AdapterLuid.LowPart ||
            enum_adapters_2_desc.pAdapters[i].AdapterLuid.HighPart,
            "Expect LUID not zero.\n" );

        close_adapter_desc.hAdapter = enum_adapters_2_desc.pAdapters[i].hAdapter;
        status = D3DKMTCloseAdapter( &close_adapter_desc );
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    }

    /* Check for insufficient buffer */
    enum_adapters_2_desc.NumAdapters = 0;
    status = D3DKMTEnumAdapters2( &enum_adapters_2_desc );
    ok( status == STATUS_BUFFER_TOO_SMALL, "Got unexpected return code %#lx.\n", status );

    free( enum_adapters_2_desc.pAdapters );
}

static void test_D3DKMTCloseAdapter(void)
{
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    NTSTATUS status;

    /* Invalid parameters */
    status = D3DKMTCloseAdapter( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    memset( &close_adapter_desc, 0, sizeof(close_adapter_desc) );
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTCreateDevice(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CREATEDEVICE create_device_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;

    /* Invalid parameters */
    status = D3DKMTCreateDevice( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    memset( &create_device_desc, 0, sizeof(create_device_desc) );
    status = D3DKMTCreateDevice( &create_device_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    lstrcpyW( open_adapter_gdi_desc.DeviceName, display1W );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );

    /* Create device */
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCreateDevice( &create_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( create_device_desc.hDevice, &next_local );
    ok( create_device_desc.pCommandBuffer == NULL, "Expect null.\n" );
    ok( create_device_desc.CommandBufferSize == 0, "Got wrong value %#x.\n", create_device_desc.CommandBufferSize );
    ok( create_device_desc.pAllocationList == NULL, "Expect null.\n" );
    ok( create_device_desc.AllocationListSize == 0, "Got wrong value %#x.\n", create_device_desc.AllocationListSize );
    ok( create_device_desc.pPatchLocationList == NULL, "Expect null.\n" );
    ok( create_device_desc.PatchLocationListSize == 0, "Got wrong value %#x.\n",
        create_device_desc.PatchLocationListSize );

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTDestroyDevice(void)
{
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    NTSTATUS status;

    /* Invalid parameters */
    status = D3DKMTDestroyDevice( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    memset( &destroy_device_desc, 0, sizeof(destroy_device_desc) );
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTCheckVidPnExclusiveOwnership(void)
{
    static const DWORD timeout = 1000;
    static const DWORD wait_step = 100;
    D3DKMT_CREATEDEVICE create_device_desc, create_device_desc2;
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP check_owner_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc;
    D3DKMT_HANDLE next_local = 0;
    DWORD total_time;
    NTSTATUS status;
    INT i;

    /* Test cases using single device */
    static const struct test_data1
    {
        D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
        NTSTATUS expected_set_status;
        NTSTATUS expected_check_status;
    }
    tests1[] =
    {
        /* 0 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVEGDI, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 10 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 20 */
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        /* 30 */
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_INVALID_PARAMETER, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        /* 40 */
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_INVALID_PARAMETER, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        /* 50 */
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS},
        {-1, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED + 1, STATUS_INVALID_PARAMETER, STATUS_SUCCESS},
    };

    /* Test cases using two devices consecutively */
    static const struct test_data2
    {
        D3DKMT_VIDPNSOURCEOWNER_TYPE set_owner_type1;
        D3DKMT_VIDPNSOURCEOWNER_TYPE set_owner_type2;
        NTSTATUS expected_set_status1;
        NTSTATUS expected_set_status2;
        NTSTATUS expected_check_status;
    }
    tests2[] =
    {
        /* 0 */
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_UNOWNED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_SHARED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        /* 10 */
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_UNOWNED, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_SHARED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {D3DKMT_VIDPNSOURCEOWNER_EMULATED, D3DKMT_VIDPNSOURCEOWNER_EMULATED, STATUS_SUCCESS, STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE, STATUS_SUCCESS},
        {-1, D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, -1, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
        {D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE, -1, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_GRAPHICS_PRESENT_OCCLUDED},
    };

    /* Invalid parameters */
    status = D3DKMTCheckVidPnExclusiveOwnership( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    memset( &check_owner_desc, 0, sizeof(check_owner_desc) );
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    /* Test cases */
    lstrcpyW( open_adapter_gdi_desc.DeviceName, display1W );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );

    memset( &create_device_desc, 0, sizeof(create_device_desc) );
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCreateDevice( &create_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( create_device_desc.hDevice, &next_local );

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    for (i = 0; i < ARRAY_SIZE(tests1); ++i)
    {
        set_owner_desc.hDevice = create_device_desc.hDevice;
        if (tests1[i].owner_type != -1)
        {
            owner_type = tests1[i].owner_type;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
        }
        else
        {
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
        }
        status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
        ok( status == tests1[i].expected_set_status, "Got unexpected return code %#lx at test %d.\n", status, i );

        status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
        /* If don't sleep, D3DKMTCheckVidPnExclusiveOwnership may get
         * STATUS_GRAPHICS_PRESENT_UNOCCLUDED instead of STATUS_SUCCESS */
        if ((tests1[i].expected_check_status == STATUS_SUCCESS && status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED))
        {
            total_time = 0;
            do
            {
                Sleep( wait_step );
                total_time += wait_step;
                status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
            } while (status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED && total_time < timeout);
        }
        ok( status == tests1[i].expected_check_status, "Got unexpected return code %#lx at test %d.\n", status, i );
    }

    /* Set owner and unset owner using different devices */
    memset( &create_device_desc2, 0, sizeof(create_device_desc2) );
    create_device_desc2.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCreateDevice( &create_device_desc2 );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( create_device_desc2.hDevice, &next_local );

    /* Set owner with the first device */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    ok( status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status );

    /* Unset owner with the second device */
    set_owner_desc.hDevice = create_device_desc2.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    /* No effect */
    ok( status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status );

    /* Unset owner with the first device */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    /* Proves that the correct device is needed to unset owner */
    ok( status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED,
        "Got unexpected return code %#lx.\n", status );

    /* Set owner with the first device, set owner again with the second device */
    for (i = 0; i < ARRAY_SIZE(tests2); ++i)
    {
        if (tests2[i].set_owner_type1 != -1)
        {
            set_owner_desc.hDevice = create_device_desc.hDevice;
            owner_type = tests2[i].set_owner_type1;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
            /* If don't sleep, D3DKMTSetVidPnSourceOwner may return STATUS_OK for
             * D3DKMT_VIDPNSOURCEOWNER_SHARED. Other owner type doesn't seems to be affected. */
            if (tests2[i].set_owner_type1 == D3DKMT_VIDPNSOURCEOWNER_SHARED) Sleep( timeout );
            status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
            ok( status == tests2[i].expected_set_status1, "Got unexpected return code %#lx at test %d.\n", status, i );
        }

        if (tests2[i].set_owner_type2 != -1)
        {
            set_owner_desc.hDevice = create_device_desc2.hDevice;
            owner_type = tests2[i].set_owner_type2;
            set_owner_desc.pType = &owner_type;
            set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
            set_owner_desc.VidPnSourceCount = 1;
            status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
            ok( status == tests2[i].expected_set_status2, "Got unexpected return code %#lx at test %d.\n", status, i );
        }

        status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
        if ((tests2[i].expected_check_status == STATUS_SUCCESS && status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED))
        {
            total_time = 0;
            do
            {
                Sleep( wait_step );
                total_time += wait_step;
                status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
            } while (status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED && total_time < timeout);
        }
        ok( status == tests2[i].expected_check_status, "Got unexpected return code %#lx at test %d.\n", status, i );

        /* Unset owner with first device */
        if (tests2[i].set_owner_type1 != -1)
        {
            set_owner_desc.hDevice = create_device_desc.hDevice;
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
            status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
            ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx at test %d.\n", status, i );
        }

        /* Unset owner with second device */
        if (tests2[i].set_owner_type2 != -1)
        {
            set_owner_desc.hDevice = create_device_desc2.hDevice;
            set_owner_desc.pType = NULL;
            set_owner_desc.pVidPnSourceId = NULL;
            set_owner_desc.VidPnSourceCount = 0;
            status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
            ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx at test %d.\n", status, i );
        }
    }

    /* Destroy devices holding ownership */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    set_owner_desc.hDevice = create_device_desc2.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    /* So ownership is released when device is destroyed. otherwise the return code should be
     * STATUS_GRAPHICS_VIDPN_SOURCE_IN_USE */
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    destroy_device_desc.hDevice = create_device_desc2.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTSetVidPnSourceOwner(void)
{
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc = {0};
    NTSTATUS status;

    /* Invalid parameters */
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTCheckOcclusion(void)
{
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP check_owner_desc;
    D3DKMT_SETVIDPNSOURCEOWNER set_owner_desc;
    D3DKMT_DESTROYDEVICE destroy_device_desc;
    D3DKMT_VIDPNSOURCEOWNER_TYPE owner_type;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_CREATEDEVICE create_device_desc;
    D3DKMT_CHECKOCCLUSION occlusion_desc;
    NTSTATUS expected_occlusion, status;
    D3DKMT_HANDLE next_local = 0;
    INT i, adapter_count = 0;
    HWND hwnd, hwnd2;
    HRESULT hr;

    /* NULL parameter check */
    status = D3DKMTCheckOcclusion( NULL );
    todo_wine ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    occlusion_desc.hWnd = NULL;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    hwnd = CreateWindowA( "static", "static1", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 200, 200, 0, 0, 0, 0 );
    ok( hwnd != NULL, "Failed to create window.\n" );

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    /* Minimized state doesn't affect D3DKMTCheckOcclusion */
    ShowWindow( hwnd, SW_MINIMIZE );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    flaky ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ShowWindow( hwnd, SW_SHOWNORMAL );

    /* Invisible state doesn't affect D3DKMTCheckOcclusion */
    ShowWindow( hwnd, SW_HIDE );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ShowWindow( hwnd, SW_SHOW );

    /* hwnd2 covers hwnd */
    hwnd2 = CreateWindowA( "static", "static2", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200,
                           200, 0, 0, 0, 0 );
    ok( hwnd2 != NULL, "Failed to create window.\n" );

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    occlusion_desc.hWnd = hwnd2;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    /* Composition doesn't affect D3DKMTCheckOcclusion */
    hr = DwmEnableComposition( DWM_EC_DISABLECOMPOSITION );
    ok( hr == S_OK, "Failed to disable composition.\n" );

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    /* This result means that D3DKMTCheckOcclusion doesn't check composition status despite MSDN says it will */
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    occlusion_desc.hWnd = hwnd2;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    ShowWindow( hwnd, SW_MINIMIZE );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    flaky ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ShowWindow( hwnd, SW_SHOWNORMAL );

    ShowWindow( hwnd, SW_HIDE );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    ShowWindow( hwnd, SW_SHOW );

    hr = DwmEnableComposition( DWM_EC_ENABLECOMPOSITION );
    ok( hr == S_OK, "Failed to enable composition.\n" );

    lstrcpyW( open_adapter_gdi_desc.DeviceName, display1W );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_gdi_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_gdi_desc.hAdapter, &next_local );

    memset( &create_device_desc, 0, sizeof(create_device_desc) );
    create_device_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCreateDevice( &create_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( create_device_desc.hDevice, &next_local );

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    /* D3DKMTCheckVidPnExclusiveOwnership gets STATUS_GRAPHICS_PRESENT_UNOCCLUDED sometimes and with
     * some delay, it will always return STATUS_SUCCESS. So there are some timing issues here. */
    ok( status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED,
        "Got unexpected return code %#lx.\n", status );

    /* Test D3DKMTCheckOcclusion relationship with video present source owner */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    owner_type = D3DKMT_VIDPNSOURCEOWNER_EXCLUSIVE;
    set_owner_desc.pType = &owner_type;
    set_owner_desc.pVidPnSourceId = &open_adapter_gdi_desc.VidPnSourceId;
    set_owner_desc.VidPnSourceCount = 1;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    for (i = 0; EnumDisplayDevicesW( NULL, i, &display_device, 0 ); ++i)
        if ((display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) adapter_count++;
    /* STATUS_GRAPHICS_PRESENT_OCCLUDED on single monitor system. STATUS_SUCCESS on multiple monitor system. */
    expected_occlusion = adapter_count > 1 ? STATUS_SUCCESS : STATUS_GRAPHICS_PRESENT_OCCLUDED;

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == expected_occlusion, "Got unexpected return code %#lx.\n", status );

    /* Note hwnd2 is not actually occluded but D3DKMTCheckOcclusion reports STATUS_GRAPHICS_PRESENT_OCCLUDED as well */
    SetWindowPos( hwnd2, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
    ShowWindow( hwnd2, SW_SHOW );
    occlusion_desc.hWnd = hwnd2;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == expected_occlusion, "Got unexpected return code %#lx.\n", status );

    /* Now hwnd is HWND_TOPMOST. Still reports STATUS_GRAPHICS_PRESENT_OCCLUDED */
    ok( SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE ),
        "Failed to SetWindowPos.\n" );
    ok( GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_TOPMOST, "No WS_EX_TOPMOST style.\n" );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == expected_occlusion, "Got unexpected return code %#lx.\n", status );

    DestroyWindow( hwnd2 );
    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    todo_wine ok( status == expected_occlusion, "Got unexpected return code %#lx.\n", status );

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    ok( status == STATUS_GRAPHICS_PRESENT_OCCLUDED, "Got unexpected return code %#lx.\n", status );

    /* Unset video present source owner */
    set_owner_desc.hDevice = create_device_desc.hDevice;
    set_owner_desc.pType = NULL;
    set_owner_desc.pVidPnSourceId = NULL;
    set_owner_desc.VidPnSourceCount = 0;
    status = D3DKMTSetVidPnSourceOwner( &set_owner_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    occlusion_desc.hWnd = hwnd;
    status = D3DKMTCheckOcclusion( &occlusion_desc );
    flaky ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    check_owner_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    check_owner_desc.VidPnSourceId = open_adapter_gdi_desc.VidPnSourceId;
    status = D3DKMTCheckVidPnExclusiveOwnership( &check_owner_desc );
    flaky ok( status == STATUS_SUCCESS || status == STATUS_GRAPHICS_PRESENT_UNOCCLUDED,
              "Got unexpected return code %#lx.\n", status );

    destroy_device_desc.hDevice = create_device_desc.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    DestroyWindow( hwnd );
}

static void test_D3DKMTOpenAdapterFromDeviceName_deviface( const GUID *devinterface_guid, NTSTATUS expected_status, BOOL todo )
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    D3DKMT_OPENADAPTERFROMDEVICENAME device_name;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    DEVPROPTYPE type;
    NTSTATUS status;
    unsigned int i;
    HDEVINFO set;
    LUID luid;
    BOOL ret;

    set = SetupDiGetClassDevsW( devinterface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    ok( set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed, error %lu.\n", GetLastError() );

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;
    iface_data->cbSize = sizeof(*iface_data);
    device_name.pDeviceName = iface_data->DevicePath;

    i = 0;
    while (SetupDiEnumDeviceInterfaces( set, NULL, devinterface_guid, i, &iface ))
    {
        ret = SetupDiGetDeviceInterfaceDetailW( set, &iface, iface_data, sizeof(iface_detail_buffer), NULL, &device_data );
        ok( ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError() );

        status = D3DKMTOpenAdapterFromDeviceName( &device_name );
        ok( status == expected_status, "Got status %#lx, expected %#lx.\n", status, expected_status );

        if (!status)
        {
            check_d3dkmt_local( device_name.hAdapter, &next_local );

            ret = SetupDiGetDevicePropertyW( set, &device_data, &DEVPROPKEY_GPU_LUID, &type,
                                             (BYTE *)&luid, sizeof(luid), NULL, 0 );
            ok( ret || GetLastError() == ERROR_NOT_FOUND, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError() );

            if (ret)
            {
                ret = RtlEqualLuid( &luid, &device_name.AdapterLuid );
                ok( ret, "Luid does not match.\n" );
            }
            else
            {
                skip( "Luid not found.\n" );
            }

            close_adapter_desc.hAdapter = device_name.hAdapter;
            status = D3DKMTCloseAdapter( &close_adapter_desc );
            ok( !status, "Got unexpected status %#lx.\n", status );
        }
        ++i;
    }
    if (!i) win_skip( "No devices found.\n" );

    SetupDiDestroyDeviceInfoList( set );
}

static void test_D3DKMTOpenAdapterFromDeviceName(void)
{
    D3DKMT_OPENADAPTERFROMDEVICENAME device_name;
    NTSTATUS status;

    /* Make sure display devices are initialized. */
    SendMessageW( GetDesktopWindow(), WM_NULL, 0, 0 );

    status = D3DKMTOpenAdapterFromDeviceName( NULL );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );

    memset( &device_name, 0, sizeof(device_name) );
    status = D3DKMTOpenAdapterFromDeviceName( &device_name );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx.\n", status );

    winetest_push_context( "GUID_DEVINTERFACE_DISPLAY_ADAPTER" );
    test_D3DKMTOpenAdapterFromDeviceName_deviface( &GUID_DEVINTERFACE_DISPLAY_ADAPTER, STATUS_INVALID_PARAMETER, TRUE );
    winetest_pop_context();

    winetest_push_context( "GUID_DISPLAY_DEVICE_ARRIVAL" );
    test_D3DKMTOpenAdapterFromDeviceName_deviface( &GUID_DISPLAY_DEVICE_ARRIVAL, STATUS_SUCCESS, FALSE );
    winetest_pop_context();
}

static void test_D3DKMTQueryVideoMemoryInfo(void)
{
    static const D3DKMT_MEMORY_SEGMENT_GROUP groups[] = {D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL, D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL};
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_desc;
    D3DKMT_QUERYVIDEOMEMORYINFO query_memory_info;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;
    unsigned int i;
    BOOL ret;

    ret = get_primary_adapter_name( open_adapter_desc.DeviceName );
    ok( ret, "Failed to get primary adapter name.\n" );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_desc.hAdapter, &next_local );

    /* Normal query */
    for (i = 0; i < ARRAY_SIZE(groups); ++i)
    {
        winetest_push_context( "group %d", groups[i] );

        query_memory_info.hProcess = NULL;
        query_memory_info.hAdapter = open_adapter_desc.hAdapter;
        query_memory_info.PhysicalAdapterIndex = 0;
        query_memory_info.MemorySegmentGroup = groups[i];
        status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
        todo_wine_if( status == STATUS_INVALID_PARAMETER ) /* fails on Wine without a Vulkan adapter */
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        ok( query_memory_info.Budget >= query_memory_info.AvailableForReservation,
            "Unexpected budget %I64u and reservation %I64u.\n", query_memory_info.Budget,
            query_memory_info.AvailableForReservation );
        ok( query_memory_info.CurrentUsage <= query_memory_info.Budget,
            "Unexpected current usage %I64u.\n", query_memory_info.CurrentUsage );
        ok( query_memory_info.CurrentReservation == 0, "Unexpected current reservation %I64u.\n",
            query_memory_info.CurrentReservation );

        winetest_pop_context();
    }

    /* Query using the current process handle */
    query_memory_info.hProcess = GetCurrentProcess();
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    todo_wine_if( status == STATUS_INVALID_PARAMETER ) /* fails on Wine without a Vulkan adapter */
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );

    /* Query using a process handle without PROCESS_QUERY_INFORMATION privilege */
    query_memory_info.hProcess = OpenProcess( PROCESS_SET_INFORMATION, FALSE, GetCurrentProcessId() );
    ok( !!query_memory_info.hProcess, "OpenProcess failed, error %ld.\n", GetLastError() );
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_ACCESS_DENIED, "Got unexpected return code %#lx.\n", status );
    CloseHandle( query_memory_info.hProcess );
    query_memory_info.hProcess = NULL;

    /* Query using an invalid process handle */
    query_memory_info.hProcess = (HANDLE)0xdeadbeef;
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_INVALID_HANDLE, "Got unexpected return code %#lx.\n", status );
    query_memory_info.hProcess = NULL;

    /* Query using an invalid adapter handle */
    query_memory_info.hAdapter = (D3DKMT_HANDLE)0xdeadbeef;
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
    query_memory_info.hAdapter = open_adapter_desc.hAdapter;

    /* Query using an invalid adapter index */
    query_memory_info.PhysicalAdapterIndex = 99;
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );
    query_memory_info.PhysicalAdapterIndex = 0;

    /* Query using an invalid memory segment group */
    query_memory_info.MemorySegmentGroup = D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL + 1;
    status = D3DKMTQueryVideoMemoryInfo( &query_memory_info );
    ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

    close_adapter_desc.hAdapter = open_adapter_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
}

static void test_gpu_device_properties_guid( const GUID *devinterface_guid )
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    WCHAR device_id[256];
    DEVPROPTYPE type;
    unsigned int i;
    UINT32 value;
    HDEVINFO set;
    BOOL ret;

    /* Make sure display devices are initialized. */
    SendMessageW( GetDesktopWindow(), WM_NULL, 0, 0 );

    set = SetupDiGetClassDevsW( devinterface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    ok( set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed, error %lu.\n", GetLastError() );

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;
    iface_data->cbSize = sizeof(*iface_data);

    i = 0;
    while (SetupDiEnumDeviceInterfaces( set, NULL, devinterface_guid, i, &iface ))
    {
        ret = SetupDiGetDeviceInterfaceDetailW( set, &iface, iface_data, sizeof(iface_detail_buffer), NULL, &device_data );
        ok( ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError() );

        ret = SetupDiGetDevicePropertyW( set, &device_data, &DEVPKEY_Device_MatchingDeviceId, &type,
                                         (BYTE *)device_id, sizeof(device_id), NULL, 0 );
        ok( ret, "Got unexpected ret %d, GetLastError() %lu.\n", ret, GetLastError() );
        ok( type == DEVPROP_TYPE_STRING, "Got type %ld.\n", type );

        ret = SetupDiGetDevicePropertyW( set, &device_data, &DEVPKEY_Device_BusNumber, &type,
                                         (BYTE *)&value, sizeof(value), NULL, 0 );
        if (!wcsicmp( device_id, L"root\\basicrender" ) ||
            !wcsicmp( device_id, L"root\\basicdisplay" ))
            ok( !ret, "Found Bus Id.\n" );
        else
        {
            ok( ret, "Got unexpected ret %d, GetLastError() %lu, %s.\n", ret, GetLastError(),
                debugstr_w(device_id));
            ok( type == DEVPROP_TYPE_UINT32, "Got type %ld.\n", type );
        }

        ret = SetupDiGetDevicePropertyW( set, &device_data, &DEVPKEY_Device_RemovalPolicy, &type,
                                         (BYTE *)&value, sizeof(value), NULL, 0 );
        ok( ret, "Got unexpected ret %d, GetLastError() %lu, %s.\n", ret, GetLastError(), debugstr_w(device_id));
        ok( value == CM_REMOVAL_POLICY_EXPECT_NO_REMOVAL || value == CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL ||
            value == CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL, "Got value %d.\n", value );
        ok( type == DEVPROP_TYPE_UINT32, "Got type %ld.\n", type );
        ++i;
    }
    SetupDiDestroyDeviceInfoList( set );
}

static void test_gpu_device_properties(void)
{
    winetest_push_context( "GUID_DEVINTERFACE_DISPLAY_ADAPTER" );
    test_gpu_device_properties_guid( &GUID_DEVINTERFACE_DISPLAY_ADAPTER );
    winetest_pop_context();
    winetest_push_context( "GUID_DISPLAY_DEVICE_ARRIVAL" );
    test_gpu_device_properties_guid( &GUID_DISPLAY_DEVICE_ARRIVAL );
    winetest_pop_context();
}

static void test_D3DKMTQueryAdapterInfo(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_desc;
    D3DKMT_QUERYADAPTERINFO query_adapter_info;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    D3DKMT_HANDLE next_local = 0;
    unsigned char buffer[1024];
    NTSTATUS status;
    unsigned int i;
    BOOL ret;

    static const struct
    {
        KMTQUERYADAPTERINFOTYPE type;
        UINT size;
    }
    tests[] =
    {
        {KMTQAITYPE_CHECKDRIVERUPDATESTATUS, sizeof(BOOL)},
        {KMTQAITYPE_DRIVERVERSION, sizeof(D3DKMT_DRIVERVERSION)},
    };

    ret = get_primary_adapter_name( open_adapter_desc.DeviceName );
    ok( ret, "Failed to get primary adapter name.\n" );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
    check_d3dkmt_local( open_adapter_desc.hAdapter, &next_local );

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context( "type %d", tests[i].type );

        /* NULL buffer */
        query_adapter_info.hAdapter = open_adapter_desc.hAdapter;
        query_adapter_info.Type = tests[i].type;
        query_adapter_info.pPrivateDriverData = NULL;
        query_adapter_info.PrivateDriverDataSize = tests[i].size;
        status = D3DKMTQueryAdapterInfo( &query_adapter_info );
        ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

        /* Insufficient buffer size */
        query_adapter_info.pPrivateDriverData = buffer;
        query_adapter_info.PrivateDriverDataSize = tests[i].size - 1;
        status = D3DKMTQueryAdapterInfo( &query_adapter_info );
        ok( status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#lx.\n", status );

        /* Normal */
        query_adapter_info.pPrivateDriverData = buffer;
        query_adapter_info.PrivateDriverDataSize = tests[i].size;
        status = D3DKMTQueryAdapterInfo( &query_adapter_info );
        ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
        if (status != STATUS_SUCCESS)
        {
            winetest_pop_context();
            continue;
        }

        switch (tests[i].type)
        {
        case KMTQAITYPE_CHECKDRIVERUPDATESTATUS:
        {
            BOOL *value = query_adapter_info.pPrivateDriverData;
            ok( *value == FALSE, "Expected %d, got %d.\n", FALSE, *value );
            break;
        }
        case KMTQAITYPE_DRIVERVERSION:
        {
            D3DKMT_DRIVERVERSION *value = query_adapter_info.pPrivateDriverData;
            ok( *value >= KMT_DRIVERVERSION_WDDM_1_3, "Expected %d >= %d.\n", *value, KMT_DRIVERVERSION_WDDM_1_3 );
            break;
        }
        default:
            ok( 0, "Type %d is not handled.\n", tests[i].type );
            break;
        }

        winetest_pop_context();
    }

    close_adapter_desc.hAdapter = open_adapter_desc.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter_desc );
    ok( status == STATUS_SUCCESS, "Got unexpected return code %#lx.\n", status );
}

static void test_D3DKMTCreateSynchronizationObject_process( const char *arg )
{
    D3DKMT_OPENSYNCHRONIZATIONOBJECT open = {0};
    D3DKMT_HANDLE global = atoi( arg );
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;

    open.hSharedHandle = global;
    open.hSyncObject = 0x1eadbeed;
    status = D3DKMTOpenSynchronizationObject( &open );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open.hSyncObject, &next_local );
    /* leak the object */
}

static void test_D3DKMTCreateSynchronizationObject( void )
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter = {0};
    D3DKMT_DESTROYSYNCHRONIZATIONOBJECT destroy = {0};
    D3DKMT_CREATESYNCHRONIZATIONOBJECT2 create2 = {0};
    D3DKMT_CREATESYNCHRONIZATIONOBJECT create = {0};
    D3DKMT_OPENSYNCHRONIZATIONOBJECT open = {0};
    D3DKMT_DESTROYDEVICE destroy_device = {0};
    D3DKMT_CREATEDEVICE create_device = {0};
    D3DKMT_CLOSEADAPTER close_adapter = {0};
    D3DKMT_HANDLE next_local = 0;
    char buffer[256];
    NTSTATUS status;

    wcscpy( open_adapter.DeviceName, L"\\\\.\\DISPLAY1" );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open_adapter.hAdapter, &next_local );
    create_device.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCreateDevice( &create_device );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create_device.hDevice, &next_local );

    /* D3DKMTCreateSynchronizationObject creates a local D3DKMT_HANDLE */
    status = D3DKMTCreateSynchronizationObject( NULL );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    status = D3DKMTCreateSynchronizationObject( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hDevice = create_device.hDevice;
    status = D3DKMTCreateSynchronizationObject( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hDevice = 0;
    create.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    status = D3DKMTCreateSynchronizationObject( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hDevice = create_device.hDevice;
    create.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    create.hSyncObject = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject( &create );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create.hSyncObject, &next_local );
    destroy.hSyncObject = create.hSyncObject;

    /* local handles are monotonically increasing */
    create.hSyncObject = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject( &create );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create.hSyncObject, &next_local );

    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    /* destroying multiple times fails */
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    destroy.hSyncObject = create.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    create.Info.Type = D3DDDI_SEMAPHORE;
    create.hSyncObject = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject( &create );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create.hSyncObject, &next_local );
    destroy.hSyncObject = create.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    create.Info.Type = D3DDDI_FENCE;
    status = D3DKMTCreateSynchronizationObject( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );


    /* D3DKMTCreateSynchronizationObject2 can create local D3DKMT_HANDLE */
    status = D3DKMTCreateSynchronizationObject2( NULL );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create2.hDevice = create_device.hDevice;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create2.hDevice = 0;
    create2.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create2.hDevice = create_device.hDevice;
    create2.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );

    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );


    create2.Info.Type = D3DDDI_SEMAPHORE;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );
    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    create2.Info.Type = D3DDDI_FENCE;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );
    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    create2.Info.Type = D3DDDI_CPU_NOTIFICATION;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_INVALID_HANDLE, status );
    create2.Info.CPUNotification.Event = CreateEventW( NULL, FALSE, FALSE, NULL );
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );
    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    CloseHandle( create2.Info.CPUNotification.Event );
    create2.Info.CPUNotification.Event = NULL;

    create2.Info.Type = D3DDDI_MONITORED_FENCE;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );
    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );


    create2.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    create2.Info.Flags.Shared = 1;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );
    check_d3dkmt_global( create2.Info.SharedHandle );
    destroy.hSyncObject = create2.hSyncObject;

    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );
    check_d3dkmt_global( create2.Info.SharedHandle );

    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    /* cannot destroy the global D3DKMT_HANDLE */
    destroy.hSyncObject = create2.Info.SharedHandle;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );


    /* D3DKMTOpenSynchronizationObject creates a new local D3DKMT_HANDLE */
    open.hSharedHandle = 0x1eadbeed;
    status = D3DKMTOpenSynchronizationObject( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open.hSharedHandle = 0;
    status = D3DKMTOpenSynchronizationObject( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open.hSyncObject = create2.hSyncObject;
    status = D3DKMTOpenSynchronizationObject( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open.hSyncObject = 0x1eadbeed;
    open.hSharedHandle = create2.Info.SharedHandle;
    status = D3DKMTOpenSynchronizationObject( &open );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open.hSyncObject, &next_local );

    destroy.hSyncObject = open.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    /* destroying multiple times fails */
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    /* the D3DKMT object can still be opened */
    open.hSyncObject = 0x1eadbeed;
    status = D3DKMTOpenSynchronizationObject( &open );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open.hSyncObject, &next_local );

    destroy.hSyncObject = open.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    /* the global D3DKMT_HANDLE is destroyed with last reference */
    status = D3DKMTOpenSynchronizationObject( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );


    /* NtSecuritySharing requires Shared, doesn't creates a global handle */
    create2.Info.Flags.Shared = 0;
    create2.Info.Flags.NtSecuritySharing = 1;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    if (broken( !status ))
    {
        check_d3dkmt_local( create2.hSyncObject, &next_local );
        destroy.hSyncObject = create2.hSyncObject;
        status = D3DKMTDestroySynchronizationObject( &destroy );
        ok_nt( STATUS_SUCCESS, status );
    }

    create2.Info.Flags.Shared = 1;
    create2.Info.Flags.NtSecuritySharing = 1;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );
    ok( create2.Info.SharedHandle == 0x1eadbeed || !create2.Info.SharedHandle,
        "got Info.SharedHandle %#x\n", create2.Info.SharedHandle );

    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );


    create2.Info.Flags.NtSecuritySharing = 0;
    create2.hSyncObject = create2.Info.SharedHandle = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hSyncObject, &next_local );
    check_d3dkmt_global( create2.Info.SharedHandle );

    sprintf( buffer, "test_D3DKMTCreateSynchronizationObject %u", create2.Info.SharedHandle );
    run_in_process( buffer );

    destroy.hSyncObject = create2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    open.hSharedHandle = create2.Info.SharedHandle;
    status = D3DKMTOpenSynchronizationObject( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );


    destroy_device.hDevice = create_device.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device );
    ok_nt( STATUS_SUCCESS, status );
    close_adapter.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter );
    ok_nt( STATUS_SUCCESS, status );
}

static void test_D3DKMTCreateKeyedMutex( void )
{
    D3DKMT_DESTROYKEYEDMUTEX destroy = {0};
    D3DKMT_CREATEKEYEDMUTEX2 create2 = {0};
    D3DKMT_CREATEKEYEDMUTEX create = {0};
    D3DKMT_OPENKEYEDMUTEX2 open2 = {0};
    D3DKMT_OPENKEYEDMUTEX open = {0};
    char runtime_data[] = {1, 2, 3, 4, 5, 6}, buffer[64];
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;

    status = D3DKMTCreateKeyedMutex( NULL );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hKeyedMutex = create.hSharedHandle = 0x1eadbeed;
    status = D3DKMTCreateKeyedMutex( &create );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create.hKeyedMutex, &next_local );
    check_d3dkmt_global( create.hSharedHandle );

    status = D3DKMTOpenKeyedMutex( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open.hKeyedMutex = create.hKeyedMutex;
    status = D3DKMTOpenKeyedMutex( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open.hKeyedMutex = 0x1eadbeed;
    open.hSharedHandle = create.hSharedHandle;
    status = D3DKMTOpenKeyedMutex( &open );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open.hKeyedMutex, &next_local );

    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    if (0)
    {
        /* older W10 lets you destroy the global D3DKMT_HANDLE, it causes random failures in the tests below */
        destroy.hKeyedMutex = create.hSharedHandle;
        status = D3DKMTDestroyKeyedMutex( &destroy );
        ok_nt( STATUS_INVALID_PARAMETER, status );
    }

    destroy.hKeyedMutex = open.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    /* destroying multiple times fails */
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    destroy.hKeyedMutex = create.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    /* the global D3DKMT_HANDLE is destroyed with last reference */
    status = D3DKMTOpenKeyedMutex( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );


    status = D3DKMTCreateKeyedMutex2( NULL );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create2.hKeyedMutex = create2.hSharedHandle = 0x1eadbeed;
    status = D3DKMTCreateKeyedMutex2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hKeyedMutex, &next_local );
    check_d3dkmt_global( create2.hSharedHandle );
    destroy.hKeyedMutex = create2.hKeyedMutex;

    create2.hKeyedMutex = create2.hSharedHandle = 0x1eadbeed;
    status = D3DKMTCreateKeyedMutex2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hKeyedMutex, &next_local );
    check_d3dkmt_global( create2.hSharedHandle );

    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open2.hKeyedMutex = create2.hKeyedMutex;
    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open2.hKeyedMutex = 0x1eadbeed;
    open2.hSharedHandle = create2.hSharedHandle;
    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open2.hKeyedMutex, &next_local );

    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    destroy.hKeyedMutex = create2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    destroy.hKeyedMutex = open2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );


    /* PrivateRuntimeDataSize must be 0 if no buffer is provided */

    status = D3DKMTCreateKeyedMutex2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hKeyedMutex, &next_local );
    check_d3dkmt_global( create2.hSharedHandle );

    open2.hKeyedMutex = 0x1eadbeed;
    open2.hSharedHandle = create2.hSharedHandle;
    open2.PrivateRuntimeDataSize = sizeof(buffer);
    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open2.pPrivateRuntimeData = buffer;
    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open2.hKeyedMutex, &next_local );
    ok_x4( open2.PrivateRuntimeDataSize, ==, sizeof(buffer) );

    destroy.hKeyedMutex = open2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    destroy.hKeyedMutex = create2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );


    create2.PrivateRuntimeDataSize = sizeof(runtime_data);
    create2.pPrivateRuntimeData = runtime_data;
    status = D3DKMTCreateKeyedMutex2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hKeyedMutex, &next_local );
    check_d3dkmt_global( create2.hSharedHandle );

    open2.hKeyedMutex = 0x1eadbeed;
    open2.hSharedHandle = create2.hSharedHandle;
    open2.PrivateRuntimeDataSize = 0;
    open2.pPrivateRuntimeData = NULL;
    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open2.hKeyedMutex, &next_local );
    ok_x4( open2.PrivateRuntimeDataSize, ==, 0 );

    open2.PrivateRuntimeDataSize = sizeof(buffer);
    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open2.PrivateRuntimeDataSize = sizeof(runtime_data) - 1;
    open2.pPrivateRuntimeData = buffer;
    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open2.PrivateRuntimeDataSize = sizeof(runtime_data);
    memset( buffer, 0xcd, sizeof(buffer) );
    status = D3DKMTOpenKeyedMutex2( &open2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open2.hKeyedMutex, &next_local );
    ok_x4( open2.PrivateRuntimeDataSize, ==, sizeof(runtime_data) );
    todo_wine ok_u1( buffer[0], ==, 0xcd );

    destroy.hKeyedMutex = open2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    destroy.hKeyedMutex = create2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );


    /* doesn't return a global D3DKMT_HANDLE with NtSecuritySharing = 1 */
    create2.Flags.NtSecuritySharing = 1;
    create2.hKeyedMutex = create2.hSharedHandle = 0x1eadbeed;
    status = D3DKMTCreateKeyedMutex2( &create2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create2.hKeyedMutex, &next_local );
    ok_x4( create2.hSharedHandle, ==, 0 );

    destroy.hKeyedMutex = create2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy );
    ok_nt( STATUS_SUCCESS, status );
}

static void test_D3DKMTCreateAllocation( void )
{
    OBJECT_ATTRIBUTES attr = {.Length = sizeof(attr)};
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter = {0};
    D3DKMT_CREATESTANDARDALLOCATION standard[2] = {0};
    D3DDDI_OPENALLOCATIONINFO2 open_alloc2 = {0};
    D3DDDI_OPENALLOCATIONINFO open_alloc = {0};
    D3DKMT_DESTROYDEVICE destroy_device = {0};
    D3DKMT_DESTROYALLOCATION2 destroy2 = {0};
    D3DKMT_CREATEDEVICE create_device = {0};
    D3DKMT_CLOSEADAPTER close_adapter = {0};
    D3DDDI_ALLOCATIONINFO2 allocs2[2] = {0};
    D3DKMT_DESTROYALLOCATION destroy = {0};
    D3DDDI_ALLOCATIONINFO allocs[2] = {0};
    D3DKMT_CREATEALLOCATION create = {0};
    D3DKMT_QUERYRESOURCEINFO query = {0};
    D3DKMT_OPENRESOURCE open = {0};
    const char expect_alloc_data[] = {1, 2, 3, 4, 5, 6};
    const char expect_runtime_data[] = {7, 8, 9, 10};
    char driver_data[0x1000], resource_data[0x100], runtime_data[0x100];
    char alloc_data[0x400], alloc2_data[0x400];
    D3DKMT_HANDLE next_local = 0;
    NTSTATUS status;

    /* static NTSTATUS (WINAPI *D3DKMTOpenResourceFromNtHandle)( D3DKMT_OPENRESOURCEFROMNTHANDLE *params ); */
    /* static NTSTATUS (WINAPI *D3DKMTQueryResourceInfoFromNtHandle)( D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE *params ); */

    if (0) /* crashes */
        status = D3DKMTCreateAllocation( NULL );

    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    wcscpy( open_adapter.DeviceName, L"\\\\.\\DISPLAY1" );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open_adapter.hAdapter, &next_local );
    create_device.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCreateDevice( &create_device );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create_device.hDevice, &next_local );


    allocs[0].pSystemMem = allocs2[0].pSystemMem = VirtualAlloc( NULL, 0x10000, MEM_COMMIT, PAGE_READWRITE );
    allocs[1].pSystemMem = allocs2[1].pSystemMem = VirtualAlloc( NULL, 0x10000, MEM_COMMIT, PAGE_READWRITE );
    standard[0].Type = D3DKMT_STANDARDALLOCATIONTYPE_EXISTINGHEAP;
    standard[0].ExistingHeapData.Size = 0x10000;
    standard[1].Type = D3DKMT_STANDARDALLOCATIONTYPE_EXISTINGHEAP;
    standard[1].ExistingHeapData.Size = 0x10000;


    create.hDevice = create_device.hDevice;
    create.Flags.ExistingSysMem = 1;
    create.Flags.StandardAllocation = 1;
    create.pStandardAllocation = standard;
    create.NumAllocations = 1;
    create.pAllocationInfo = allocs;
    allocs[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    create.hPrivateRuntimeResourceHandle = (HANDLE)0xdeadbeef;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    ok_x4( create.hResource, ==, 0 );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );
    ok_ptr( create.hPrivateRuntimeResourceHandle, ==, (HANDLE)0xdeadbeef );

    destroy.hDevice = create_device.hDevice;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    destroy.AllocationCount = 1;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    destroy.phAllocationList = &allocs[0].hAllocation;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    /* allocation has already been destroyed */
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );


    /* D3DKMTCreateAllocation2 also works with the same parameters, with extra alloc info */
    create.pAllocationInfo2 = allocs2;
    allocs2[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation2( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    ok_x4( create.hResource, ==, 0 );
    check_d3dkmt_local( allocs2[0].hAllocation, &next_local );
    ok_x4( create.PrivateRuntimeDataSize, ==, 0 );
    ok_x4( create.PrivateDriverDataSize, ==, 0 );
    ok_x4( allocs2[0].PrivateDriverDataSize, ==, 0 );
    create.pAllocationInfo = allocs;

    destroy.phAllocationList = &allocs2[0].hAllocation;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    destroy.phAllocationList = &allocs[0].hAllocation;


    /* D3DKMTDestroyAllocation2 works as well */
    allocs[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    ok_x4( create.hResource, ==, 0 );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );

    destroy2.hDevice = create_device.hDevice;
    destroy2.AllocationCount = 1;
    destroy2.phAllocationList = &allocs[0].hAllocation;
    status = D3DKMTDestroyAllocation2( &destroy2 );
    ok_nt( STATUS_SUCCESS, status );


    /* alloc PrivateDriverDataSize can be set */
    allocs[0].pPrivateDriverData = (char *)expect_alloc_data;
    allocs[0].PrivateDriverDataSize = sizeof(expect_alloc_data);
    allocs[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    ok_x4( create.hResource, ==, 0 );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );

    /* PrivateRuntimeDataSize can be set */
    create.pPrivateRuntimeData = expect_runtime_data;
    create.PrivateRuntimeDataSize = sizeof(expect_runtime_data);
    allocs[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    ok_x4( create.hResource, ==, 0 );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );


    /* PrivateDriverDataSize must be 0 for standard allocations */
    create.PrivateDriverDataSize = 64;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.PrivateDriverDataSize = 0;

    /* device handle is required */
    create.hDevice = 0;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hDevice = create_device.hDevice;

    /* hResource must be valid or 0 */
    create.hResource = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_HANDLE, status );
    create.hResource = 0;

    /* NumAllocations is required */
    create.NumAllocations = 0;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.NumAllocations = 1;

    /* standard.Type must be set */
    standard[0].Type = 0;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    standard[0].Type = D3DKMT_STANDARDALLOCATIONTYPE_EXISTINGHEAP;

    /* pSystemMem must be set */
    allocs[0].pSystemMem = 0;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    allocs[0].pSystemMem = allocs2[0].pSystemMem;

    /* creating multiple allocations doesn't work */
    create.NumAllocations = 2;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.NumAllocations = 1;

    /* ExistingHeapData.Size must be page aligned */
    standard[0].ExistingHeapData.Size = 0x1100;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    standard[0].ExistingHeapData.Size = 0x10000;

    /* specific flags are required for standard allocations */
    create.Flags.ExistingSysMem = 0;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.Flags.ExistingSysMem = 1;
    create.Flags.StandardAllocation = 0;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.Flags.StandardAllocation = 1;

    /* CreateShared doesn't work without CreateResource */
    create.Flags.CreateShared = 1;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.Flags.CreateShared = 0;


    /* test creating a resource with the allocations */
    create.Flags.CreateResource = 1;
    allocs[0].hAllocation = create.hGlobalShare = create.hResource = 0x1eadbeed;
    create.hPrivateRuntimeResourceHandle = (HANDLE)0xdeadbeef;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_HANDLE, status );
    create.hResource = 0; /* hResource must be set to 0, even with CreateResource */
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    check_d3dkmt_local( create.hResource, &next_local );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );
    ok_ptr( create.hPrivateRuntimeResourceHandle, ==, (HANDLE)0xdeadbeef );

    /* destroying the allocation doesn't destroys the resource */
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    destroy.hResource = create.hResource;
    destroy.AllocationCount = 0;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    destroy.AllocationCount = 1;
    create.hResource = 0;


    create.Flags.CreateResource = 1;
    allocs[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    check_d3dkmt_local( create.hResource, &next_local );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );

    /* cannot create allocations with an existing resource */
    create.Flags.CreateResource = 0;
    create.pAllocationInfo = &allocs[1];
    allocs[1].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.pAllocationInfo = &allocs[0];

    /* destroying resource destroys its allocations */
    destroy.hResource = create.hResource;
    destroy.AllocationCount = 0;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    destroy.hResource = 0;
    destroy.AllocationCount = 1;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.hResource = 0;


    /* cannot create a resource without allocations */
    create.Flags.CreateResource = 1;
    create.NumAllocations = 0;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    create.NumAllocations = 1;

    /* destroy resource at once from here */
    destroy.AllocationCount = 0;


    allocs[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    check_d3dkmt_local( create.hResource, &next_local );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );

    /* D3DKMTQueryResourceInfo requires a global handle */
    query.hDevice = create_device.hDevice;
    query.hGlobalShare = create.hResource;
    status = D3DKMTQueryResourceInfo( &query );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    /* D3DKMTOpenResource requires a global handle */
    open.hDevice = create_device.hDevice;
    open.hGlobalShare = create.hResource;
    status = D3DKMTOpenResource( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    destroy.hResource = create.hResource;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    create.hResource = 0;


    /* test creating global shared resource */
    create.Flags.CreateShared = 1;
    allocs[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_global( create.hGlobalShare );
    check_d3dkmt_local( create.hResource, &next_local );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );

    /* D3DKMTQueryResourceInfo works with global handle */
    memset( runtime_data, 0xcd, sizeof(runtime_data) );
    query.hDevice = create_device.hDevice;
    query.hGlobalShare = create.hGlobalShare;
    query.pPrivateRuntimeData = runtime_data;
    query.PrivateRuntimeDataSize = sizeof(runtime_data);
    query.ResourcePrivateDriverDataSize = 0;
    status = D3DKMTQueryResourceInfo( &query );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( query.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    todo_wine ok_x4( query.TotalPrivateDriverDataSize, >, 0 );
    ok_x4( query.TotalPrivateDriverDataSize, <, sizeof(driver_data) );
    ok_x4( query.ResourcePrivateDriverDataSize, ==, 0 );
    ok_u4( query.NumAllocations, ==, 1 );
    /* runtime data doesn't get updated ? */
    ok_u1( runtime_data[0], ==, 0xcd );

    /* D3DKMTOpenResource works with a global handle */
    memset( runtime_data, 0xcd, sizeof(runtime_data) );
    memset( driver_data, 0xcd, sizeof(driver_data) );
    memset( resource_data, 0xcd, sizeof(resource_data) );
    memset( alloc_data, 0xcd, sizeof(alloc_data) );
    open.hDevice = create_device.hDevice;
    open.hGlobalShare = create.hGlobalShare;
    open.pPrivateRuntimeData = runtime_data;
    open.PrivateRuntimeDataSize = query.PrivateRuntimeDataSize;
    open.pTotalPrivateDriverDataBuffer = driver_data;
    open.TotalPrivateDriverDataBufferSize = sizeof(driver_data);
    open.pResourcePrivateDriverData = resource_data;
    open.ResourcePrivateDriverDataSize = query.ResourcePrivateDriverDataSize;
    open.NumAllocations = 1;
    open.pOpenAllocationInfo = &open_alloc;
    open_alloc.pPrivateDriverData = alloc_data;
    open_alloc.PrivateDriverDataSize = sizeof(alloc_data);
    open.hResource = 0x1eadbeed;
    status = D3DKMTOpenResource( &open );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( open.hGlobalShare, ==, create.hGlobalShare );
    check_d3dkmt_local( open.hResource, &next_local );
    ok_x4( open.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    todo_wine ok_x4( open.TotalPrivateDriverDataBufferSize, >, 0 );
    ok_x4( open.TotalPrivateDriverDataBufferSize, <, sizeof(driver_data) );
    ok_x4( open.ResourcePrivateDriverDataSize, ==, 0 );
    ok_u4( open.NumAllocations, ==, 1 );
    check_d3dkmt_local( open_alloc.hAllocation, &next_local );
    todo_wine ok_x4( open_alloc.PrivateDriverDataSize, >, 0 );
    ok_x4( open_alloc.PrivateDriverDataSize, <, sizeof(alloc_data) );
    ok( !memcmp( runtime_data, expect_runtime_data, sizeof(expect_runtime_data) ), "got data %#x\n", runtime_data[0] );
    todo_wine ok_u1( driver_data[0], !=, 0xcd );
    ok_u1( resource_data[0], ==, 0xcd );
    ok_u1( alloc_data[0], ==, 0xcd );

    destroy.hResource = open.hResource;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    open.hResource = 0;

    /* NumAllocations must be set */
    open.NumAllocations = 0;
    status = D3DKMTOpenResource( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open.NumAllocations = 1;

    /* buffer sizes must match exactly */
    open.PrivateRuntimeDataSize += 1;
    status = D3DKMTOpenResource( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open.PrivateRuntimeDataSize -= 1;
    open.ResourcePrivateDriverDataSize += 1;
    status = D3DKMTOpenResource( &open );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open.ResourcePrivateDriverDataSize -= 1;

    /* D3DKMTOpenResource2 works as well */
    open.pOpenAllocationInfo2 = &open_alloc2;
    open_alloc2.pPrivateDriverData = alloc2_data;
    open_alloc2.PrivateDriverDataSize = sizeof(alloc2_data);
    open_alloc2.hAllocation = 0x1eadbeed;
    status = D3DKMTOpenResource2( &open );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( open.hGlobalShare, ==, create.hGlobalShare );
    check_d3dkmt_local( open.hResource, &next_local );
    ok_x4( open.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    todo_wine ok_x4( open.TotalPrivateDriverDataBufferSize, >, 0 );
    ok_x4( open.TotalPrivateDriverDataBufferSize, <, sizeof(driver_data) );
    ok_x4( open.ResourcePrivateDriverDataSize, ==, 0 );
    ok_u4( open.NumAllocations, ==, 1 );
    check_d3dkmt_local( open_alloc2.hAllocation, &next_local );
    todo_wine ok_x4( open_alloc2.PrivateDriverDataSize, >, 0 );
    ok_x4( open_alloc2.PrivateDriverDataSize, <, sizeof(alloc_data) );
    open.pOpenAllocationInfo = &open_alloc;

    destroy.hResource = open.hResource;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    open.hResource = 0;

    destroy.hResource = create.hResource;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    create.hResource = 0;


    /* test creating nt shared resource */
    create.Flags.NtSecuritySharing = 1;
    create.Flags.CreateShared = 1;
    allocs[0].hAllocation = create.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create.hGlobalShare, ==, 0 );
    check_d3dkmt_local( create.hResource, &next_local );
    check_d3dkmt_local( allocs[0].hAllocation, &next_local );

    destroy.hResource = create.hResource;
    status = D3DKMTDestroyAllocation( &destroy );
    ok_nt( STATUS_SUCCESS, status );
    create.hResource = 0;


    VirtualFree( (void *)allocs[0].pSystemMem, 0, MEM_RELEASE );
    VirtualFree( (void *)allocs[1].pSystemMem, 0, MEM_RELEASE );


    destroy_device.hDevice = create_device.hDevice;
    D3DKMTDestroyDevice( &destroy_device );
    close_adapter.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter );
    ok_nt( STATUS_SUCCESS, status );
}

static void test_D3DKMTShareObjects( void )
{
    OBJECT_ATTRIBUTES attr = {.Length = sizeof(attr)};
    UNICODE_STRING name = RTL_CONSTANT_STRING( L"\\BaseNamedObjects\\__WineTest_D3DKMT" );
    UNICODE_STRING name2 = RTL_CONSTANT_STRING( L"\\BaseNamedObjects\\__WineTest_D3DKMT_2" );
    UNICODE_STRING name_lower = RTL_CONSTANT_STRING( L"\\BaseNamedObjects\\__winetest_d3dkmt" );
    UNICODE_STRING name_invalid = RTL_CONSTANT_STRING( L"\\BaseNamedObjects\\__winetest_invalid" );
    D3DKMT_OPENSYNCOBJECTNTHANDLEFROMNAME open_sync_name = {0};
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter = {0};
    D3DKMT_CREATESYNCHRONIZATIONOBJECT2 create_sync2 = {0};
    D3DKMT_DESTROYSYNCHRONIZATIONOBJECT destroy_sync = {0};
    D3DKMT_CREATESYNCHRONIZATIONOBJECT create_sync = {0};
    D3DKMT_OPENNTHANDLEFROMNAME open_resource_name = {0};
    D3DKMT_OPENSYNCOBJECTFROMNTHANDLE2 open_sync2 = {0};
    D3DKMT_OPENSYNCOBJECTFROMNTHANDLE open_sync = {0};
    D3DKMT_DESTROYKEYEDMUTEX destroy_mutex = {0};
    D3DKMT_CREATEKEYEDMUTEX2 create_mutex2 = {0};
    D3DKMT_CREATEKEYEDMUTEX create_mutex = {0};
    D3DKMT_DESTROYDEVICE destroy_device = {0};
    D3DKMT_CREATEDEVICE create_device = {0};
    D3DKMT_CLOSEADAPTER close_adapter = {0};

    D3DKMT_QUERYRESOURCEINFOFROMNTHANDLE query_resource = {0};
    D3DKMT_OPENRESOURCEFROMNTHANDLE open_resource = {0};
    D3DKMT_CREATESTANDARDALLOCATION standard = {0};
    D3DKMT_DESTROYALLOCATION destroy_alloc = {0};
    D3DDDI_OPENALLOCATIONINFO2 open_alloc = {0};
    D3DKMT_CREATEALLOCATION create_alloc = {0};
    D3DDDI_ALLOCATIONINFO alloc = {0};
    char expect_runtime_data[] = {9, 8, 7, 6};
    char expect_mutex_data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    char expect_alloc_data[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    char driver_data[0x1000], runtime_data[0x100];
    char alloc_data[0x400], mutex_data[0x100];
    D3DKMT_HANDLE objects[4], next_local = 0;

    NTSTATUS status;
    HANDLE handle;

    wcscpy( open_adapter.DeviceName, L"\\\\.\\DISPLAY1" );
    status = D3DKMTOpenAdapterFromGdiDisplayName( &open_adapter );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open_adapter.hAdapter, &next_local );
    create_device.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCreateDevice( &create_device );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( create_device.hDevice, &next_local );


    /* D3DKMTShareObjects doesn't work with D3DKMTCreateSynchronizationObject created objects */
    create_sync.hDevice = create_device.hDevice;
    create_sync.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    create_sync.hSyncObject = 0x1eadbeed;
    status = D3DKMTCreateSynchronizationObject( &create_sync );
    ok_nt( STATUS_SUCCESS, status );
    handle = (HANDLE)0xdeadbeef;
    status = D3DKMTShareObjects( 1, &create_sync.hSyncObject, &attr, STANDARD_RIGHTS_WRITE, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    if (broken( !status )) CloseHandle( handle );
    destroy_sync.hSyncObject = create_sync.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );

    /* D3DKMTShareObjects doesn't work with Shared = 1/0 alone */
    create_sync2.hDevice = create_device.hDevice;
    create_sync2.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    status = D3DKMTCreateSynchronizationObject2( &create_sync2 );
    ok_nt( STATUS_SUCCESS, status );
    handle = (HANDLE)0xdeadbeef;
    status = D3DKMTShareObjects( 1, &create_sync2.hSyncObject, &attr, STANDARD_RIGHTS_WRITE, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    if (broken( !status )) CloseHandle( handle );
    destroy_sync.hSyncObject = create_sync2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );

    create_sync2.Info.Flags.Shared = 1;
    status = D3DKMTCreateSynchronizationObject2( &create_sync2 );
    ok_nt( STATUS_SUCCESS, status );
    handle = (HANDLE)0xdeadbeef;
    status = D3DKMTShareObjects( 1, &create_sync2.hSyncObject, &attr, STANDARD_RIGHTS_WRITE, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    if (broken( !status )) CloseHandle( handle );
    destroy_sync.hSyncObject = create_sync2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );

    /* D3DKMTShareObjects requires NtSecuritySharing (which requires Shared = 1) */
    create_sync2.Info.Flags.NtSecuritySharing = 1;
    status = D3DKMTCreateSynchronizationObject2( &create_sync2 );
    ok_nt( STATUS_SUCCESS, status );
    InitializeObjectAttributes( &attr, &name, 0, 0, NULL );
    handle = (HANDLE)0xdeadbeef;
    status = D3DKMTShareObjects( 1, &create_sync2.hSyncObject, &attr, STANDARD_RIGHTS_WRITE, &handle );
    ok_nt( STATUS_SUCCESS, status );
    /* handle isn't a D3DKMT_HANDLE */
    ok( !((UINT_PTR)handle & 0xc0000000), "got %p\n", handle );

    check_object_type( handle, L"DxgkSharedSyncObject" );
    check_object_name( handle, name.Buffer );

    /* cannot destroy the handle */
    destroy_sync.hSyncObject = (UINT_PTR)handle;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    destroy_sync.hSyncObject = create_sync2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );


    /* the sync object can be opened from the NT handle */
    status = D3DKMTOpenSyncObjectFromNtHandle( &open_sync );
    ok_nt( STATUS_INVALID_HANDLE, status );
    open_sync.hNtHandle = handle;
    open_sync.hSyncObject = 0x1eadbeed;
    status = D3DKMTOpenSyncObjectFromNtHandle( &open_sync );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open_sync.hSyncObject, NULL );

    /* objects opened with D3DKMTOpenSyncObjectFromNtHandle cannot be reshared */
    InitializeObjectAttributes( &attr, &name, 0, 0, NULL );
    status = D3DKMTShareObjects( 1, &create_sync2.hSyncObject, &attr, STANDARD_RIGHTS_WRITE, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    destroy_sync.hSyncObject = open_sync.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );


    status = D3DKMTOpenSyncObjectFromNtHandle2( &open_sync2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open_sync2.hNtHandle = open_sync.hNtHandle;
    status = D3DKMTOpenSyncObjectFromNtHandle2( &open_sync2 );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open_sync2.hDevice = create_device.hDevice;
    open_sync2.hSyncObject = 0x1eadbeed;
    status = D3DKMTOpenSyncObjectFromNtHandle2( &open_sync2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open_sync2.hSyncObject, NULL );

    /* objects opened with Shared/NtSecuritySharing flags don't seem to matter */
    ok_x4( open_sync2.Flags.Shared, ==, 0 );
    ok_x4( open_sync2.Flags.NtSecuritySharing, ==, 0 );
    InitializeObjectAttributes( &attr, &name2, 0, 0, NULL );
    handle = (HANDLE)0xdeadbeef;
    status = D3DKMTShareObjects( 1, &open_sync2.hSyncObject, &attr, STANDARD_RIGHTS_READ, &handle );
    ok_nt( STATUS_SUCCESS, status );
    CloseHandle( handle );

    destroy_sync.hSyncObject = open_sync2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );


    status = D3DKMTOpenSyncObjectNtHandleFromName( &open_sync_name );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    InitializeObjectAttributes( &attr, &name_invalid, 0, 0, NULL );
    open_sync_name.pObjAttrib = &attr;
    status = D3DKMTOpenSyncObjectNtHandleFromName( &open_sync_name );
    ok_nt( STATUS_OBJECT_NAME_NOT_FOUND, status );
    InitializeObjectAttributes( &attr, &name_lower, 0, 0, NULL );
    open_sync_name.pObjAttrib = &attr;
    status = D3DKMTOpenSyncObjectNtHandleFromName( &open_sync_name );
    ok_nt( STATUS_ACCESS_DENIED, status );
    open_sync_name.dwDesiredAccess = STANDARD_RIGHTS_WRITE;
    status = D3DKMTOpenSyncObjectNtHandleFromName( &open_sync_name );
    ok_nt( STATUS_SUCCESS, status );
    ok( !((UINT_PTR)open_sync_name.hNtHandle & 0xc0000000), "got %p\n", open_sync_name.hNtHandle );


    CloseHandle( open_sync2.hNtHandle );

    status = D3DKMTOpenSyncObjectFromNtHandle2( &open_sync2 );
    ok_nt( STATUS_INVALID_HANDLE, status );
    open_sync2.hNtHandle = open_sync_name.hNtHandle;
    open_sync2.hSyncObject = 0x1eadbeed;
    status = D3DKMTOpenSyncObjectFromNtHandle2( &open_sync2 );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_local( open_sync2.hSyncObject, NULL );

    CloseHandle( open_sync_name.hNtHandle );


    /* cannot open the object anymore after all the NT handles have been closed */
    InitializeObjectAttributes( &attr, &name_lower, 0, 0, NULL );
    open_sync_name.pObjAttrib = &attr;
    status = D3DKMTOpenSyncObjectNtHandleFromName( &open_sync_name );
    ok_nt( STATUS_OBJECT_NAME_NOT_FOUND, status );

    /* but object still exists and can be re-shared */
    InitializeObjectAttributes( &attr, &name, 0, 0, NULL );
    handle = (HANDLE)0xdeadbeef;
    status = D3DKMTShareObjects( 1, &open_sync2.hSyncObject, &attr, STANDARD_RIGHTS_READ, &handle );
    ok_nt( STATUS_SUCCESS, status );

    /* can be opened again by name */
    open_sync_name.pObjAttrib = &attr;
    open_sync_name.dwDesiredAccess = STANDARD_RIGHTS_READ;
    status = D3DKMTOpenSyncObjectNtHandleFromName( &open_sync_name );
    ok_nt( STATUS_SUCCESS, status );

    CloseHandle( open_sync_name.hNtHandle );
    CloseHandle( handle );

    destroy_sync.hSyncObject = open_sync2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );


    /* D3DKMTShareObjects doesn't work with keyed mutex objects alone */
    status = D3DKMTCreateKeyedMutex( &create_mutex );
    ok_nt( STATUS_SUCCESS, status );
    status = D3DKMTShareObjects( 1, &create_mutex.hKeyedMutex, &attr, STANDARD_RIGHTS_WRITE, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    status = D3DKMTShareObjects( 1, &create_mutex.hSharedHandle, &attr, STANDARD_RIGHTS_WRITE, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    destroy_mutex.hKeyedMutex = create_mutex.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy_mutex );
    ok_nt( STATUS_SUCCESS, status );

    create_mutex2.Flags.NtSecuritySharing = 1;
    status = D3DKMTCreateKeyedMutex2( &create_mutex2 );
    ok_nt( STATUS_SUCCESS, status );
    status = D3DKMTShareObjects( 1, &create_mutex2.hKeyedMutex, &attr, STANDARD_RIGHTS_WRITE, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    destroy_mutex.hKeyedMutex = create_mutex2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy_mutex );
    ok_nt( STATUS_SUCCESS, status );


    /* NtSecuritySharing = 1 is required for D3DKMTShareObjects */

    alloc.pSystemMem = VirtualAlloc( NULL, 0x10000, MEM_COMMIT, PAGE_READWRITE );
    standard.Type = D3DKMT_STANDARDALLOCATIONTYPE_EXISTINGHEAP;
    standard.ExistingHeapData.Size = 0x10000;
    create_alloc.hDevice = create_device.hDevice;
    create_alloc.Flags.ExistingSysMem = 1;
    create_alloc.Flags.StandardAllocation = 1;
    create_alloc.Flags.CreateResource = 1;
    create_alloc.pStandardAllocation = &standard;
    create_alloc.NumAllocations = 1;
    create_alloc.pAllocationInfo = &alloc;
    create_alloc.pPrivateRuntimeData = expect_runtime_data;
    create_alloc.PrivateRuntimeDataSize = sizeof(expect_runtime_data);
    create_alloc.hPrivateRuntimeResourceHandle = (HANDLE)0xdeadbeef;
    alloc.pPrivateDriverData = expect_alloc_data;
    alloc.PrivateDriverDataSize = sizeof(expect_alloc_data);
    alloc.hAllocation = create_alloc.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create_alloc );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create_alloc.hGlobalShare, ==, 0 );
    check_d3dkmt_local( create_alloc.hResource, NULL );
    check_d3dkmt_local( alloc.hAllocation, NULL );

    status = D3DKMTShareObjects( 1, &alloc.hAllocation, &attr, STANDARD_RIGHTS_READ, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    status = D3DKMTShareObjects( 1, &create_alloc.hResource, &attr, STANDARD_RIGHTS_READ, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    destroy_alloc.hDevice = create_device.hDevice;
    destroy_alloc.hResource = create_alloc.hResource;
    status = D3DKMTDestroyAllocation( &destroy_alloc );
    ok_nt( STATUS_SUCCESS, status );
    create_alloc.hResource = 0;


    create_alloc.Flags.CreateShared = 1;
    alloc.hAllocation = create_alloc.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create_alloc );
    ok_nt( STATUS_SUCCESS, status );
    check_d3dkmt_global( create_alloc.hGlobalShare );
    check_d3dkmt_local( create_alloc.hResource, NULL );
    check_d3dkmt_local( alloc.hAllocation, NULL );

    status = D3DKMTShareObjects( 1, &alloc.hAllocation, &attr, STANDARD_RIGHTS_READ, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    status = D3DKMTShareObjects( 1, &create_alloc.hResource, &attr, STANDARD_RIGHTS_READ, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    destroy_alloc.hResource = create_alloc.hResource;
    status = D3DKMTDestroyAllocation( &destroy_alloc );
    ok_nt( STATUS_SUCCESS, status );
    create_alloc.hResource = 0;


    create_alloc.Flags.NtSecuritySharing = 1;
    alloc.hAllocation = create_alloc.hGlobalShare = 0x1eadbeed;
    status = D3DKMTCreateAllocation( &create_alloc );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( create_alloc.hGlobalShare, ==, 0 );
    check_d3dkmt_local( create_alloc.hResource, NULL );
    check_d3dkmt_local( alloc.hAllocation, NULL );

    /* can only share resources, not allocations */
    status = D3DKMTShareObjects( 1, &alloc.hAllocation, &attr, STANDARD_RIGHTS_READ, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    status = D3DKMTShareObjects( 1, &create_alloc.hResource, &attr, STANDARD_RIGHTS_READ, &handle );
    ok_nt( STATUS_SUCCESS, status );

    check_object_type( handle, L"DxgkSharedResource" );
    check_object_name( handle, name.Buffer );


    status = D3DKMTOpenNtHandleFromName( &open_resource_name );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    InitializeObjectAttributes( &attr, &name_invalid, 0, 0, NULL );
    open_resource_name.pObjAttrib = &attr;
    status = D3DKMTOpenNtHandleFromName( &open_resource_name );
    ok_nt( STATUS_OBJECT_NAME_NOT_FOUND, status );
    InitializeObjectAttributes( &attr, &name_lower, 0, 0, NULL );
    open_resource_name.pObjAttrib = &attr;
    status = D3DKMTOpenNtHandleFromName( &open_resource_name );
    ok_nt( STATUS_ACCESS_DENIED, status );
    open_resource_name.dwDesiredAccess = GENERIC_ALL;
    status = D3DKMTOpenNtHandleFromName( &open_resource_name );
    ok_nt( STATUS_SUCCESS, status );
    ok( !((UINT_PTR)open_resource_name.hNtHandle & 0xc0000000), "got %p\n", open_resource_name.hNtHandle );

    query_resource.hDevice = create_device.hDevice;
    query_resource.hNtHandle = open_resource_name.hNtHandle;
    query_resource.PrivateRuntimeDataSize = 0xdeadbeef;
    query_resource.TotalPrivateDriverDataSize = 0xdeadbeef;
    query_resource.ResourcePrivateDriverDataSize = 0xdeadbeef;
    query_resource.NumAllocations = 0xdeadbeef;
    status = D3DKMTQueryResourceInfoFromNtHandle( &query_resource );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( query_resource.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    todo_wine ok_x4( query_resource.TotalPrivateDriverDataSize, >, 0 );
    ok_x4( query_resource.TotalPrivateDriverDataSize, <, sizeof(driver_data) );
    ok_x4( query_resource.ResourcePrivateDriverDataSize, ==, 0 );
    ok_u4( query_resource.NumAllocations, ==, 1 );

    memset( runtime_data, 0xcd, sizeof(runtime_data) );
    query_resource.pPrivateRuntimeData = runtime_data;
    query_resource.PrivateRuntimeDataSize = 0; /* sizeof(runtime_data); */
    status = D3DKMTQueryResourceInfoFromNtHandle( &query_resource );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( query_resource.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    ok_u1( runtime_data[0], ==, 0xcd );

    CloseHandle( open_resource_name.hNtHandle );


    memset( runtime_data, 0xcd, sizeof(runtime_data) );
    memset( driver_data, 0xcd, sizeof(driver_data) );
    open_resource.hDevice = create_device.hDevice;
    open_resource.hNtHandle = handle;
    open_resource.hResource = open_resource.hKeyedMutex = open_resource.hSyncObject = 0x1eadbeed;
    open_resource.NumAllocations = 1;
    open_resource.pOpenAllocationInfo2 = &open_alloc;
    open_alloc.hAllocation = 0x1eadbeed;
    open_resource.pPrivateRuntimeData = runtime_data;
    open_resource.PrivateRuntimeDataSize = query_resource.PrivateRuntimeDataSize;
    open_resource.pTotalPrivateDriverDataBuffer = driver_data;
    open_resource.TotalPrivateDriverDataBufferSize = query_resource.TotalPrivateDriverDataSize;
    status = D3DKMTOpenResourceFromNtHandle( &open_resource );
    ok_nt( STATUS_SUCCESS, status );
    ok_u4( open_resource.NumAllocations, ==, 1 );
    ok_x4( open_resource.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    ok_x4( open_resource.ResourcePrivateDriverDataSize, ==, 0 );
    todo_wine ok_x4( open_resource.TotalPrivateDriverDataBufferSize, >, 0 );
    ok_x4( open_resource.TotalPrivateDriverDataBufferSize, <, sizeof(driver_data) );
    check_d3dkmt_local( open_resource.hResource, NULL );
    ok_x4( open_resource.hKeyedMutex, ==, 0 );
    ok_x4( open_resource.KeyedMutexPrivateRuntimeDataSize, ==, 0 );
    ok_x4( open_resource.hSyncObject, ==, 0 );
    todo_wine check_d3dkmt_local( open_alloc.hAllocation, NULL );
    todo_wine ok_x4( open_alloc.PrivateDriverDataSize, >, 0 );
    ok_x4( open_alloc.PrivateDriverDataSize, <, sizeof(alloc_data) );
    ok( !memcmp( runtime_data, expect_runtime_data, sizeof(expect_runtime_data) ), "got data %#x\n", runtime_data[0] );
    todo_wine ok_u1( driver_data[0], !=, 0xcd );

    destroy_alloc.hResource = open_resource.hResource;
    status = D3DKMTDestroyAllocation( &destroy_alloc );
    ok_nt( STATUS_SUCCESS, status );
    open_resource.hResource = 0;

    open_resource.pOpenAllocationInfo2 = NULL;
    status = D3DKMTOpenResourceFromNtHandle( &open_resource );
    ok( status == STATUS_INVALID_PARAMETER || broken(status == STATUS_ACCESS_VIOLATION) /* win32 */, "got %#lx\n", status );
    open_resource.pOpenAllocationInfo2 = &open_alloc;

    open_resource.NumAllocations = 0;
    status = D3DKMTOpenResourceFromNtHandle( &open_resource );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open_resource.NumAllocations = 1;

    open_resource.pPrivateRuntimeData = NULL;
    status = D3DKMTOpenResourceFromNtHandle( &open_resource );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open_resource.pPrivateRuntimeData = runtime_data;

    open_resource.PrivateRuntimeDataSize += 1;
    status = D3DKMTOpenResourceFromNtHandle( &open_resource );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open_resource.PrivateRuntimeDataSize = query_resource.PrivateRuntimeDataSize;

    open_resource.pTotalPrivateDriverDataBuffer = NULL;
    status = D3DKMTOpenResourceFromNtHandle( &open_resource );
    ok_nt( STATUS_INVALID_PARAMETER, status );
    open_resource.pTotalPrivateDriverDataBuffer = driver_data;

    open_resource.TotalPrivateDriverDataBufferSize -= 1;
    status = D3DKMTOpenResourceFromNtHandle( &open_resource );
    todo_wine ok_nt( STATUS_NO_MEMORY, status );
    open_resource.TotalPrivateDriverDataBufferSize = query_resource.TotalPrivateDriverDataSize;

    memset( &open_resource, 0, sizeof(open_resource) );
    CloseHandle( handle );

    /* with combined objects only resource/mutex/sync order and combination works */

    create_sync2.hDevice = create_device.hDevice;
    create_sync2.Info.Type = D3DDDI_SYNCHRONIZATION_MUTEX;
    status = D3DKMTCreateSynchronizationObject2( &create_sync2 );
    ok_nt( STATUS_SUCCESS, status );
    handle = (HANDLE)0xdeadbeef;

    create_mutex2.Flags.NtSecuritySharing = 1;
    create_mutex2.PrivateRuntimeDataSize = sizeof(expect_mutex_data);
    create_mutex2.pPrivateRuntimeData = expect_mutex_data;
    status = D3DKMTCreateKeyedMutex2( &create_mutex2 );
    ok_nt( STATUS_SUCCESS, status );

    objects[0] = create_alloc.hResource;
    objects[1] = create_alloc.hResource;
    objects[2] = create_alloc.hResource;
    status = D3DKMTShareObjects( 3, objects, &attr, STANDARD_RIGHTS_ALL, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    objects[0] = create_alloc.hResource;
    objects[1] = create_mutex2.hKeyedMutex;
    status = D3DKMTShareObjects( 2, objects, &attr, STANDARD_RIGHTS_ALL, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    objects[0] = create_alloc.hResource;
    objects[1] = create_sync2.hSyncObject;
    status = D3DKMTShareObjects( 2, objects, &attr, STANDARD_RIGHTS_ALL, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    objects[0] = create_mutex2.hKeyedMutex;
    objects[1] = create_sync2.hSyncObject;
    status = D3DKMTShareObjects( 2, objects, &attr, STANDARD_RIGHTS_ALL, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    objects[0] = create_alloc.hResource;
    objects[1] = create_mutex2.hKeyedMutex;
    objects[2] = create_sync2.hSyncObject;
    objects[3] = create_sync2.hSyncObject;
    status = D3DKMTShareObjects( 4, objects, &attr, STANDARD_RIGHTS_ALL, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    objects[0] = create_alloc.hResource;
    objects[1] = create_sync2.hSyncObject;
    objects[2] = create_mutex2.hKeyedMutex;
    status = D3DKMTShareObjects( 3, objects, &attr, STANDARD_RIGHTS_ALL, &handle );
    ok_nt( STATUS_INVALID_PARAMETER, status );

    objects[0] = create_alloc.hResource;
    objects[1] = create_mutex2.hKeyedMutex;
    objects[2] = create_sync2.hSyncObject;
    handle = (HANDLE)0xdeadbeef;
    status = D3DKMTShareObjects( 3, objects, &attr, STANDARD_RIGHTS_ALL, &handle );
    ok_nt( STATUS_SUCCESS, status );

    check_object_type( handle, L"DxgkSharedResource" );
    check_object_name( handle, name.Buffer );

    destroy_mutex.hKeyedMutex = create_mutex2.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy_mutex );
    ok_nt( STATUS_SUCCESS, status );
    create_mutex2.hKeyedMutex = 0;

    destroy_sync.hSyncObject = create_sync2.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );
    create_sync2.hSyncObject = 0;

    destroy_alloc.hResource = create_alloc.hResource;
    status = D3DKMTDestroyAllocation( &destroy_alloc );
    ok_nt( STATUS_SUCCESS, status );
    create_alloc.hResource = 0;


    query_resource.hDevice = create_device.hDevice;
    query_resource.hNtHandle = handle;
    query_resource.PrivateRuntimeDataSize = 0xdeadbeef;
    query_resource.TotalPrivateDriverDataSize = 0xdeadbeef;
    query_resource.ResourcePrivateDriverDataSize = 0xdeadbeef;
    query_resource.NumAllocations = 0xdeadbeef;
    status = D3DKMTQueryResourceInfoFromNtHandle( &query_resource );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( query_resource.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    todo_wine ok_x4( query_resource.TotalPrivateDriverDataSize, >, 0 );
    ok_x4( query_resource.TotalPrivateDriverDataSize, <, sizeof(driver_data) );
    ok_x4( query_resource.ResourcePrivateDriverDataSize, ==, 0 );
    ok_u4( query_resource.NumAllocations, ==, 1 );

    memset( runtime_data, 0xcd, sizeof(runtime_data) );
    query_resource.pPrivateRuntimeData = runtime_data;
    query_resource.PrivateRuntimeDataSize = sizeof(runtime_data);
    status = D3DKMTQueryResourceInfoFromNtHandle( &query_resource );
    ok_nt( STATUS_SUCCESS, status );
    ok_x4( query_resource.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    ok_u1( runtime_data[0], ==, 0xcd );

    memset( runtime_data, 0xcd, sizeof(runtime_data) );
    memset( driver_data, 0xcd, sizeof(driver_data) );
    memset( mutex_data, 0xcd, sizeof(mutex_data) );
    open_resource.hDevice = create_device.hDevice;
    open_resource.hNtHandle = handle;
    open_resource.hResource = open_resource.hKeyedMutex = open_resource.hSyncObject = 0x1eadbeed;
    open_resource.NumAllocations = 1;
    open_resource.pOpenAllocationInfo2 = &open_alloc;
    open_alloc.hAllocation = 0x1eadbeed;
    open_resource.pPrivateRuntimeData = runtime_data;
    open_resource.PrivateRuntimeDataSize = query_resource.PrivateRuntimeDataSize;
    open_resource.pTotalPrivateDriverDataBuffer = driver_data;
    open_resource.TotalPrivateDriverDataBufferSize = query_resource.TotalPrivateDriverDataSize;
    open_resource.pKeyedMutexPrivateRuntimeData = mutex_data;
    open_resource.KeyedMutexPrivateRuntimeDataSize = sizeof(expect_mutex_data);
    status = D3DKMTOpenResourceFromNtHandle( &open_resource );
    ok_nt( STATUS_SUCCESS, status );
    ok_u4( open_resource.NumAllocations, ==, 1 );
    ok_x4( open_resource.PrivateRuntimeDataSize, ==, sizeof(expect_runtime_data) );
    ok_x4( open_resource.ResourcePrivateDriverDataSize, ==, 0 );
    todo_wine ok_x4( open_resource.TotalPrivateDriverDataBufferSize, >, 0 );
    ok_x4( open_resource.TotalPrivateDriverDataBufferSize, <, sizeof(driver_data) );
    check_d3dkmt_local( open_resource.hResource, NULL );
    check_d3dkmt_local( open_resource.hKeyedMutex, NULL );
    ok_x4( open_resource.KeyedMutexPrivateRuntimeDataSize, ==, sizeof(expect_mutex_data) );
    check_d3dkmt_local( open_resource.hSyncObject, NULL );
    todo_wine check_d3dkmt_local( open_alloc.hAllocation, NULL );
    todo_wine ok_x4( open_alloc.PrivateDriverDataSize, >, 0 );

    destroy_alloc.hResource = open_resource.hResource;
    status = D3DKMTDestroyAllocation( &destroy_alloc );
    ok_nt( STATUS_SUCCESS, status );
    open_resource.hResource = 0;

    destroy_mutex.hKeyedMutex = open_resource.hKeyedMutex;
    status = D3DKMTDestroyKeyedMutex( &destroy_mutex );
    ok_nt( STATUS_SUCCESS, status );
    open_resource.hKeyedMutex = 0;

    destroy_sync.hSyncObject = open_resource.hSyncObject;
    status = D3DKMTDestroySynchronizationObject( &destroy_sync );
    ok_nt( STATUS_SUCCESS, status );
    open_resource.hSyncObject = 0;

    memset( &open_resource, 0, sizeof(open_resource) );
    CloseHandle( handle );


    VirtualFree( (void *)alloc.pSystemMem, 0, MEM_RELEASE );

    destroy_device.hDevice = create_device.hDevice;
    status = D3DKMTDestroyDevice( &destroy_device );
    ok_nt( STATUS_SUCCESS, status );
    close_adapter.hAdapter = open_adapter.hAdapter;
    status = D3DKMTCloseAdapter( &close_adapter );
    ok_nt( STATUS_SUCCESS, status );
}

static IDirect3DDevice9Ex *create_d3d9ex_device( HWND hwnd, LUID *luid, BOOL *stencil_broken )
{
    D3DPRESENT_PARAMETERS params = {0};
    D3DADAPTER_IDENTIFIER9 adapter_id;
    IDirect3DDevice9Ex *device;
    IDirect3D9Ex *d3d;
    LUID adapter_luid;
    UINT i, count;
    HRESULT hr;

    hr = Direct3DCreate9Ex( D3D_SDK_VERSION, &d3d );
    ok_hr( S_OK, hr );
    count = IDirect3D9Ex_GetAdapterCount( d3d );
    ok_u4( count, >, 0 );

    for (i = 0; i < count; i++)
    {
        hr = IDirect3D9Ex_GetAdapterLUID( d3d, i, &adapter_luid );
        ok_hr( S_OK, hr );

        if (luid_equals( luid, &luid_zero )) *luid = adapter_luid;
        if (luid_equals( luid, &adapter_luid )) break;
    }
    ok( i < count, "d3d9ex adapter not found\n" );

    hr = IDirect3D9Ex_GetAdapterIdentifier( d3d, i, 0, &adapter_id );
    ok_hr( S_OK, hr );
    if (stencil_broken) *stencil_broken = adapter_id.VendorId == 0x1002;

    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.hDeviceWindow = hwnd;
    params.Windowed = TRUE;
    hr = IDirect3D9Ex_CreateDeviceEx( d3d, i, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                     &params, NULL, &device );
    ok_hr( S_OK, hr );
    IDirect3D9Ex_Release( d3d );

    return device;
}

static void test_shared_resources(void)
{
    IDirect3DDevice9Ex *d3d9_exp, *d3d9_imp;
    BOOL stencil_broken;
    LUID luid = {0};
    HRESULT hr;
    HWND hwnd;
    MSG msg;

    hwnd = CreateWindowW( L"static", NULL, WS_POPUP | WS_VISIBLE, 100, 100, 200, 200, NULL, NULL, NULL, NULL );
    ok_ptr( hwnd, !=, NULL );
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    d3d9_exp = create_d3d9ex_device( hwnd, &luid, &stencil_broken );
    ok_ptr( d3d9_exp, !=, NULL );
    d3d9_imp = create_d3d9ex_device( hwnd, &luid, NULL );
    ok_ptr( d3d9_imp, !=, NULL );

    trace( "adapter luid %s\n", debugstr_luid( &luid ) );

#define MAKETEST(api, dim, idx) (((api & 7) << 7) | ((dim & 7) << 4) | (idx & 15))
#define GET_API(test) ((test >> 7) & 7)
#define GET_DIM(test) ((test >> 4) & 7)

    for (UINT test = 0; test <= MAKETEST(7,7,0xf); test++)
    {
        static const UINT resource_size = 0x100000, array_1d = 0x20, width_2d = 0x40, height_3d = 0x20, width_cube = 0x40;
        const UINT width_3d = width_2d, depth_3d = resource_size / 4 / width_2d / height_3d;
        const UINT height_2d = resource_size / 4 / width_2d;
        const UINT width_1d = resource_size / 4 / array_1d;

        char runtime_desc[0x1000] = {0};
        IUnknown *export = NULL;
        HANDLE handle = NULL;

        winetest_push_context( "%u:%u:%u", GET_API(test), GET_DIM(test), test & 15 );

        switch (test)
        {
        case MAKETEST(0, 0, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = resource_size / (4 * sizeof(float)), .height = 1, .format = DXGI_FORMAT_UNKNOWN};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_VERTEXDATA, .type = D3DRTYPE_VERTEXBUFFER, .usage = D3DUSAGE_LOCKABLE, .buffer.format = D3DFVF_XYZ};
            hr = IDirect3DDevice9Ex_CreateVertexBuffer( d3d9_exp, dxgi.width, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT,
                                                        (IDirect3DVertexBuffer9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 0, 1):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = resource_size / 2, .height = 1, .format = DXGI_FORMAT_UNKNOWN};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_INDEX16, .type = D3DRTYPE_INDEXBUFFER, .usage = D3DUSAGE_LOCKABLE};
            hr = IDirect3DDevice9Ex_CreateIndexBuffer( d3d9_exp, dxgi.width, 0, desc.format, D3DPOOL_DEFAULT,
                                                       (IDirect3DIndexBuffer9 **)&export, &handle );
            ok( hr == S_OK || broken( hr == D3DERR_NOTAVAILABLE ), "got %#lx\n", hr );
            if (hr != S_OK) break;
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_B8G8R8A8_UNORM};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_TEXTURE, .usage = D3DUSAGE_SURFACE};
            hr = IDirect3DDevice9Ex_CreateTexture( d3d9_exp, dxgi.width, dxgi.height, 1, 0, desc.format, D3DPOOL_DEFAULT,
                                                   (IDirect3DTexture9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 1):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_B8G8R8A8_UNORM};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_TEXTURE, .usage = D3DUSAGE_SURFACE | D3DUSAGE_LOCKABLE | D3DUSAGE_DYNAMIC};
            hr = IDirect3DDevice9Ex_CreateTexture( d3d9_exp, dxgi.width, dxgi.height, 1, D3DUSAGE_DYNAMIC, desc.format, D3DPOOL_DEFAULT,
                                                   (IDirect3DTexture9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 2):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_B8G8R8A8_UNORM};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_TEXTURE, .usage = D3DUSAGE_SURFACE | D3DUSAGE_RENDERTARGET};
            hr = IDirect3DDevice9Ex_CreateTexture( d3d9_exp, dxgi.width, dxgi.height, 1, D3DUSAGE_RENDERTARGET, desc.format, D3DPOOL_DEFAULT,
                                                   (IDirect3DTexture9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 3):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_B8G8R8A8_UNORM};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_SURFACE, .usage = D3DUSAGE_SURFACE | D3DUSAGE_RENDERTARGET};
            hr = IDirect3DDevice9Ex_CreateRenderTarget( d3d9_exp, dxgi.width, dxgi.height, desc.format, D3DMULTISAMPLE_NONE, 0,
                                                        FALSE, (IDirect3DSurface9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 4):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_B8G8R8A8_UNORM};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_SURFACE, .usage = D3DUSAGE_SURFACE | D3DUSAGE_LOCKABLE | D3DUSAGE_RENDERTARGET};
            hr = IDirect3DDevice9Ex_CreateRenderTarget( d3d9_exp, dxgi.width, dxgi.height, desc.format, D3DMULTISAMPLE_NONE, 0,
                                                        TRUE, (IDirect3DSurface9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 5):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_D24_UNORM_S8_UINT};
            struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_D24S8, .type = D3DRTYPE_SURFACE, .usage = D3DUSAGE_SURFACE | D3DUSAGE_DEPTHSTENCIL};
            hr = IDirect3DDevice9Ex_CreateDepthStencilSurface( d3d9_exp, dxgi.width, dxgi.height, desc.format, D3DMULTISAMPLE_NONE, 0,
                                                               FALSE, (IDirect3DSurface9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            if (stencil_broken) { desc.dxgi.format = 0; desc.format = D3DFMT_D24S8 - 1; }
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 6):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_B8G8R8A8_UNORM};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_SURFACE, .usage = D3DUSAGE_LOCKABLE | D3DUSAGE_OFFSCREEN};
            hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface( d3d9_exp, dxgi.width, dxgi.height, desc.format, D3DPOOL_DEFAULT,
                                                                 (IDirect3DSurface9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 7):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_B8G8R8A8_UNORM};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_SURFACE, .usage = D3DUSAGE_SURFACE | D3DUSAGE_RENDERTARGET};
            hr = IDirect3DDevice9Ex_CreateRenderTargetEx( d3d9_exp, dxgi.width, dxgi.height, desc.format, D3DMULTISAMPLE_NONE, 0,
                                                          FALSE, (IDirect3DSurface9 **)&export, &handle, 0 );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 8):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_B8G8R8A8_UNORM};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_SURFACE, .usage = D3DUSAGE_LOCKABLE | D3DUSAGE_OFFSCREEN};
            hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx( d3d9_exp, dxgi.width, dxgi.height, desc.format, D3DPOOL_DEFAULT,
                                                                   (IDirect3DSurface9 **)&export, &handle, 0 );
            todo_wine ok_hr( S_OK, hr );
            todo_wine todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 2, 9):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_D24_UNORM_S8_UINT};
            struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_D24S8, .type = D3DRTYPE_SURFACE, .usage = D3DUSAGE_SURFACE | D3DUSAGE_DEPTHSTENCIL};
            hr = IDirect3DDevice9Ex_CreateDepthStencilSurfaceEx( d3d9_exp, dxgi.width, dxgi.height, desc.format, D3DMULTISAMPLE_NONE, 0,
                                                                 FALSE, (IDirect3DSurface9 **)&export, &handle, 0 );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            if (stencil_broken) { desc.dxgi.format = 0; desc.format = D3DFMT_D24S8 - 1; }
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 3, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_3d, .height = height_3d, .format = DXGI_FORMAT_UNKNOWN};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_VOLUMETEXTURE, .usage = D3DUSAGE_SURFACE, .texture.depth = depth_3d};
            hr = IDirect3DDevice9Ex_CreateVolumeTexture( d3d9_exp, dxgi.width, dxgi.height, desc.texture.depth, 1, 0, desc.format, D3DPOOL_DEFAULT,
                                                         (IDirect3DVolumeTexture9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(0, 3, 1):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x58, .version = 1, .width = width_cube, .height = width_cube, .format = DXGI_FORMAT_UNKNOWN};
            const struct d3d9_runtime_desc desc = {.dxgi = dxgi, .format = D3DFMT_A8R8G8B8, .type = D3DRTYPE_CUBETEXTURE, .usage = D3DUSAGE_SURFACE};
            hr = IDirect3DDevice9Ex_CreateCubeTexture( d3d9_exp, dxgi.width, 1, 0, desc.format, D3DPOOL_DEFAULT,
                                                       (IDirect3DCubeTexture9 **)&export, &handle );
            ok_hr( S_OK, hr );
            todo_wine ok_ptr( handle, !=, NULL );
            if (!handle) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d9_runtime_desc( (struct d3d9_runtime_desc *)runtime_desc, &desc );
            break;
        }
        }

        if (handle && !is_d3dkmt_handle( handle )) CloseHandle( handle );
        if (export) ok_ref( 0, IUnknown_Release( export ) );
        winetest_pop_context();
    }

#undef GET_DIM
#undef GET_API
#undef MAKETEST

    ok_ref( 0, IDirect3DDevice9Ex_Release( d3d9_imp ) );
    ok_ref( 0, IDirect3DDevice9Ex_Release( d3d9_exp ) );
    DestroyWindow( hwnd );
}

START_TEST( d3dkmt )
{
    char **argv;
    int argc;

    /* native win32u.dll fails if user32 is not loaded, so make sure it's fully initialized */
    GetDesktopWindow();

    argc = winetest_get_mainargs( &argv );
    if (argc > 3 && !strcmp( argv[2], "test_D3DKMTCreateSynchronizationObject" ))
        return test_D3DKMTCreateSynchronizationObject_process( argv[3] );

    test_D3DKMTOpenAdapterFromGdiDisplayName();
    test_D3DKMTOpenAdapterFromHdc();
    test_D3DKMTEnumAdapters2();
    test_D3DKMTCloseAdapter();
    test_D3DKMTCreateDevice();
    test_D3DKMTDestroyDevice();
    test_D3DKMTCheckVidPnExclusiveOwnership();
    test_D3DKMTSetVidPnSourceOwner();
    test_D3DKMTCheckOcclusion();
    test_D3DKMTOpenAdapterFromDeviceName();
    test_D3DKMTQueryAdapterInfo();
    test_D3DKMTQueryVideoMemoryInfo();
    test_gpu_device_properties();
    test_D3DKMTCreateSynchronizationObject();
    test_D3DKMTCreateKeyedMutex();
    test_D3DKMTCreateAllocation();
    test_D3DKMTShareObjects();
    test_shared_resources();
}
