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
#include "d3d11_1.h"
#include "d3d12.h"

#include "wine/vulkan.h"
#include "wine/wgl.h"
#include "wine/test.h"

#define D3DUSAGE_SURFACE    0x8000000
#define D3DUSAGE_LOCKABLE   0x4000000
#define D3DUSAGE_OFFSCREEN  0x0400000

static const WCHAR display1W[] = L"\\\\.\\DISPLAY1";

DEFINE_DEVPROPKEY(DEVPROPKEY_GPU_LUID, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2);

static PFN_vkGetInstanceProcAddr p_vkGetInstanceProcAddr;
static PFN_vkGetDeviceProcAddr p_vkGetDeviceProcAddr;

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

struct d3d11_runtime_desc
{
    struct dxgi_runtime_desc    dxgi;
    D3D11_RESOURCE_DIMENSION    dimension;
    union
    {
        D3D10_BUFFER_DESC       d3d10_buf;
        D3D10_TEXTURE1D_DESC    d3d10_1d;
        D3D10_TEXTURE2D_DESC    d3d10_2d;
        D3D10_TEXTURE3D_DESC    d3d10_3d;
        D3D11_BUFFER_DESC       d3d11_buf;
        D3D11_TEXTURE1D_DESC    d3d11_1d;
        D3D11_TEXTURE2D_DESC    d3d11_2d;
        D3D11_TEXTURE3D_DESC    d3d11_3d;
    };
};

C_ASSERT( sizeof(struct d3d11_runtime_desc) == 0x68 );

#define check_d3d11_runtime_desc( a, b ) check_d3d11_runtime_desc_( a, b, FALSE )
static void check_d3d11_runtime_desc_( struct d3d11_runtime_desc *desc, const struct d3d11_runtime_desc *expect, BOOL d3d12 )
{
    check_dxgi_runtime_desc_( &desc->dxgi, &expect->dxgi, d3d12 );

    ok_x4( desc->dimension, ==, expect->dimension );
    switch (desc->dimension)
    {
    case D3D11_RESOURCE_DIMENSION_BUFFER:
        ok_x4( desc->d3d11_buf.ByteWidth, ==, expect->d3d11_buf.ByteWidth );
        ok_x4( desc->d3d11_buf.Usage, ==, expect->d3d11_buf.Usage );
        ok_x4( desc->d3d11_buf.BindFlags, ==, expect->d3d11_buf.BindFlags );
        ok_x4( desc->d3d11_buf.CPUAccessFlags, ==, expect->d3d11_buf.CPUAccessFlags );
        ok_x4( desc->d3d11_buf.MiscFlags, ==, expect->d3d11_buf.MiscFlags );
        ok_x4( desc->d3d11_buf.StructureByteStride, ==, expect->d3d11_buf.StructureByteStride );
        break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        ok_x4( desc->d3d11_1d.Width, ==, expect->d3d11_1d.Width );
        ok_x4( desc->d3d11_1d.MipLevels, ==, expect->d3d11_1d.MipLevels );
        ok_x4( desc->d3d11_1d.ArraySize, ==, expect->d3d11_1d.ArraySize );
        ok_x4( desc->d3d11_1d.Format, ==, expect->d3d11_1d.Format );
        ok_x4( desc->d3d11_1d.Usage, ==, expect->d3d11_1d.Usage );
        ok_x4( desc->d3d11_1d.BindFlags, ==, expect->d3d11_1d.BindFlags );
        ok_x4( desc->d3d11_1d.CPUAccessFlags, ==, expect->d3d11_1d.CPUAccessFlags );
        ok_x4( desc->d3d11_1d.MiscFlags, ==, expect->d3d11_1d.MiscFlags );
        break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        ok_x4( desc->d3d11_2d.Width, ==, expect->d3d11_2d.Width );
        ok_x4( desc->d3d11_2d.Height, ==, expect->d3d11_2d.Height );
        ok_x4( desc->d3d11_2d.MipLevels, ==, expect->d3d11_2d.MipLevels );
        ok_x4( desc->d3d11_2d.ArraySize, ==, expect->d3d11_2d.ArraySize );
        ok_x4( desc->d3d11_2d.Format, ==, expect->d3d11_2d.Format );
        ok_x4( desc->d3d11_2d.SampleDesc.Count, ==, expect->d3d11_2d.SampleDesc.Count );
        ok_x4( desc->d3d11_2d.SampleDesc.Quality, ==, expect->d3d11_2d.SampleDesc.Quality );
        ok_x4( desc->d3d11_2d.Usage, ==, expect->d3d11_2d.Usage );
        ok_x4( desc->d3d11_2d.BindFlags, ==, expect->d3d11_2d.BindFlags );
        ok_x4( desc->d3d11_2d.CPUAccessFlags, ==, expect->d3d11_2d.CPUAccessFlags );
        ok_x4( desc->d3d11_2d.MiscFlags, ==, expect->d3d11_2d.MiscFlags );
        break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        ok_x4( desc->d3d11_3d.Width, ==, expect->d3d11_3d.Width );
        ok_x4( desc->d3d11_3d.Height, ==, expect->d3d11_3d.Height );
        ok_x4( desc->d3d11_3d.Depth, ==, expect->d3d11_3d.Depth );
        ok_x4( desc->d3d11_3d.MipLevels, ==, expect->d3d11_3d.MipLevels );
        ok_x4( desc->d3d11_3d.Format, ==, expect->d3d11_3d.Format );
        ok_x4( desc->d3d11_3d.Usage, ==, expect->d3d11_3d.Usage );
        ok_x4( desc->d3d11_3d.BindFlags, ==, expect->d3d11_3d.BindFlags );
        ok_x4( desc->d3d11_3d.CPUAccessFlags, ==, expect->d3d11_3d.CPUAccessFlags );
        ok_x4( desc->d3d11_3d.MiscFlags, ==, expect->d3d11_3d.MiscFlags );
        break;
    default:
        ok( 0, "unexpected\n" );
        break;
    }
}

struct d3d12_runtime_desc
{
    struct d3d11_runtime_desc   d3d11;
    UINT                        unknown_5[4];
    UINT                        resource_size;
    UINT                        unknown_6[7];
    UINT                        resource_align;
    UINT                        unknown_7[9];
    union
    {
        D3D12_RESOURCE_DESC     desc;
        D3D12_RESOURCE_DESC1    desc1;
    };
    UINT64                      unknown_8[1];
};

C_ASSERT( sizeof(struct d3d12_runtime_desc) == 0x108 );
C_ASSERT( offsetof(struct d3d12_runtime_desc, unknown_5) == sizeof(struct d3d11_runtime_desc) );

static void check_d3d12_runtime_desc( struct d3d12_runtime_desc *desc, const struct d3d12_runtime_desc *expect )
{
    if (desc->d3d11.dxgi.version == 4) check_d3d11_runtime_desc_( &desc->d3d11, &expect->d3d11, TRUE );
    else check_dxgi_runtime_desc_( &desc->d3d11.dxgi, &expect->d3d11.dxgi, TRUE );

    ok_x4( desc->desc.Dimension, ==, expect->desc.Dimension );
    ok_x4( desc->desc.Alignment, ==, expect->resource_align );
    ok_x4( desc->desc.Width, ==, expect->desc.Width );
    ok_x4( desc->desc.Height, ==, expect->desc.Height );
    ok_x4( desc->desc.DepthOrArraySize, ==, expect->desc.DepthOrArraySize );
    ok_x4( desc->desc.MipLevels, ==, expect->desc.MipLevels );
    ok_x4( desc->desc.Format, ==, expect->desc.Format );
    ok_x4( desc->desc.SampleDesc.Count, ==, expect->desc.SampleDesc.Count );
    ok_x4( desc->desc.SampleDesc.Quality, ==, expect->desc.SampleDesc.Quality );
    ok_x4( desc->desc.Layout, ==, expect->desc.Layout );
    ok_x4( desc->desc.Flags, ==, expect->desc.Flags );
    ok_x4( desc->desc1.SamplerFeedbackMipRegion.Width, ==, expect->desc1.SamplerFeedbackMipRegion.Width );
    ok_x4( desc->desc1.SamplerFeedbackMipRegion.Height, ==, expect->desc1.SamplerFeedbackMipRegion.Height );
    ok_x4( desc->desc1.SamplerFeedbackMipRegion.Depth, ==, expect->desc1.SamplerFeedbackMipRegion.Depth );
    ok_x4( desc->resource_size, ==, expect->resource_size );
    ok_x4( desc->resource_align, ==, expect->resource_align );
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
        else todo_wine ok( query.PrivateRuntimeDataSize == 0 || query.PrivateRuntimeDataSize == 0x68 /* NVIDIA */, "got %#x\n", query.PrivateRuntimeDataSize );
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
        else todo_wine ok( open.PrivateRuntimeDataSize == 0 || open.PrivateRuntimeDataSize == 0x68 /* NVIDIA */, "got %#x\n", open.PrivateRuntimeDataSize );
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
        else todo_wine ok( query_nt.PrivateRuntimeDataSize == 0 || query_nt.PrivateRuntimeDataSize == 0x68 /* NVIDIA */, "got %#x\n", query_nt.PrivateRuntimeDataSize );
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
        else todo_wine ok( open_nt.PrivateRuntimeDataSize == 0 || open_nt.PrivateRuntimeDataSize == 0x68 /* NVIDIA */, "got %#x\n", open_nt.PrivateRuntimeDataSize );
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
    if (stencil_broken) *stencil_broken = !winetest_platform_is_wine && adapter_id.VendorId == 0x1002;

    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.hDeviceWindow = hwnd;
    params.Windowed = TRUE;
    hr = IDirect3D9Ex_CreateDeviceEx( d3d, i, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                     &params, NULL, &device );
    ok_hr( S_OK, hr );
    IDirect3D9Ex_Release( d3d );

    return device;
}

static IDXGIAdapter *create_dxgi_adapter( IDXGIFactory3 *dxgi, LUID *luid )
{
    IDXGIAdapter *adapter = NULL;
    DXGI_ADAPTER_DESC desc;

    for (UINT i = 0; SUCCEEDED(IDXGIFactory3_EnumAdapters( dxgi, i, &adapter )); i++)
    {
        IDXGIAdapter_GetDesc( adapter, &desc );
        if (luid_equals( luid, &luid_zero )) *luid = desc.AdapterLuid;
        if (luid_equals( luid, &desc.AdapterLuid )) break;
        IDXGIAdapter_Release( adapter );
        adapter = NULL;
    }

    ok_ptr( adapter, !=, NULL );
    return adapter;
}

static ID3D11Device1 *create_d3d11_device( IDXGIAdapter *adapter )
{
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    ID3D11Device1 *device1;
    ID3D11Device *device;
    HRESULT hr;

    hr = D3D11CreateDevice( adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, &feature_level, 1,
                            D3D11_SDK_VERSION, &device, NULL, NULL );
    ok_hr( S_OK, hr );

    hr = ID3D11Device_QueryInterface( device, &IID_ID3D11Device1, (void **)&device1 );
    ok_hr( S_OK, hr );
    ID3D11Device_Release( device );

    return device1;
}

struct vulkan_device
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
};

struct vulkan_buffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
};

struct vulkan_image
{
    VkImage image;
    VkDeviceMemory memory;
};

static VkResult create_vulkan_instance( uint32_t extension_count, const char *const *extensions,
                                        VkInstance *instance )
{
    VkInstanceCreateInfo create_info = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    VkApplicationInfo app_info = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO};
    PFN_vkCreateInstance p_vkCreateInstance;
    HMODULE vulkan;

    if (!(vulkan = LoadLibraryW( L"vulkan-1" ))) return VK_ERROR_INITIALIZATION_FAILED;
    if (!(p_vkCreateInstance = (void *)GetProcAddress( vulkan, "vkCreateInstance" ))) return 0;
    if (!(p_vkGetInstanceProcAddr = (void *)GetProcAddress( vulkan, "vkGetInstanceProcAddr" ))) return 0;
    if (!(p_vkGetDeviceProcAddr = (void *)GetProcAddress( vulkan, "vkGetDeviceProcAddr" ))) return 0;

    app_info.apiVersion = VK_API_VERSION_1_1;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = extension_count;
    create_info.ppEnabledExtensionNames = extensions;

    return p_vkCreateInstance( &create_info, NULL, instance );
}

static void get_vulkan_physical_device_luid( VkInstance instance, VkPhysicalDevice physical_device, LUID *luid )
{
    PFN_vkGetPhysicalDeviceProperties2 p_vkGetPhysicalDeviceProperties2;

    VkPhysicalDeviceIDProperties id_props = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
    VkPhysicalDeviceVulkan11Properties vk11_props = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, .pNext = &id_props};
    VkPhysicalDeviceProperties2 props = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &vk11_props};

    if (!(p_vkGetPhysicalDeviceProperties2 = (void *)p_vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceProperties2" ))) return;

    p_vkGetPhysicalDeviceProperties2( physical_device, &props );
    ok_u4( props.properties.apiVersion, !=, 0 );
    ok_u4( props.properties.driverVersion, !=, 0 );
    ok_u4( props.properties.vendorID, !=, 0 );

    ok( !IsEqualGUID( &GUID_NULL, (GUID *)id_props.deviceUUID ), "got deviceUUID %s\n", debugstr_guid( (GUID *)id_props.deviceUUID ) );
    ok( !IsEqualGUID( &GUID_NULL, (GUID *)id_props.driverUUID ), "got driverUUID %s\n", debugstr_guid( (GUID *)id_props.driverUUID ) );
    ok( !luid_equals( &luid_zero, (LUID *)id_props.deviceLUID ), "got deviceLUID %s\n", debugstr_luid( (LUID *)id_props.deviceLUID ) );
    ok_u4( id_props.deviceNodeMask, ==, 1 );
    ok_u4( id_props.deviceLUIDValid, ==, 1 );

    ok( !IsEqualGUID( &GUID_NULL, (GUID *)vk11_props.deviceUUID ), "got deviceUUID %s\n", debugstr_guid( (GUID *)vk11_props.deviceUUID ) );
    ok( !IsEqualGUID( &GUID_NULL, (GUID *)vk11_props.driverUUID ), "got driverUUID %s\n", debugstr_guid( (GUID *)vk11_props.driverUUID ) );
    ok( !luid_equals( &luid_zero, (LUID *)vk11_props.deviceLUID ), "got deviceLUID %s\n", debugstr_luid( (LUID *)vk11_props.deviceLUID ) );
    ok_u4( vk11_props.deviceNodeMask, ==, 1 );
    ok_u4( vk11_props.deviceLUIDValid, ==, 1 );

    *luid = *(LUID *)vk11_props.deviceLUID;
}

static VkExternalMemoryHandleTypeFlags get_vulkan_external_image_types( VkInstance instance, VkPhysicalDevice physical_device,
                                                                        VkExternalMemoryFeatureFlags feature_flags )
{
    static const VkExternalMemoryHandleTypeFlagBits bits[] =
    {
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT,
    };

    VkPhysicalDeviceExternalImageFormatInfo external_info = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO};
    VkPhysicalDeviceImageFormatInfo2 format_info = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, .pNext = &external_info};

    VkExternalImageFormatProperties external_props = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES};
    VkImageFormatProperties2 format_props = {.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, .pNext = &external_props};

    PFN_vkGetPhysicalDeviceImageFormatProperties2 p_vkGetPhysicalDeviceImageFormatProperties2;
    VkExternalMemoryHandleTypeFlags handle_types = 0;
    VkResult vr;

    if (!(p_vkGetPhysicalDeviceImageFormatProperties2 = (void *)p_vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceImageFormatProperties2" ))) return 0;

    format_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    format_info.type = VK_IMAGE_TYPE_2D;
    format_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    format_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    for (const VkExternalMemoryHandleTypeFlagBits *bit = bits; bit < bits + ARRAY_SIZE(bits); bit++)
    {
        winetest_push_context( "%#x", *bit );

        external_info.handleType = *bit;
        memset( &external_props.externalMemoryProperties, 0, sizeof(external_props.externalMemoryProperties) );
        vr = p_vkGetPhysicalDeviceImageFormatProperties2( physical_device, &format_info, &format_props );
        ok( vr == VK_SUCCESS || vr == VK_ERROR_FORMAT_NOT_SUPPORTED, "got %#x\n", vr );

        if (!(~external_props.externalMemoryProperties.externalMemoryFeatures & feature_flags))
        {
            ok_u4( external_props.externalMemoryProperties.compatibleHandleTypes, ==, external_info.handleType );
            handle_types |= external_info.handleType;
        }

        winetest_pop_context();
    }

    return handle_types;
}

static void destroy_vulkan_device( struct vulkan_device *dev )
{
    if (dev->instance)
    {
        PFN_vkDestroyInstance p_vkDestroyInstance;
        PFN_vkDestroyDevice p_vkDestroyDevice;

        p_vkDestroyDevice = (void *)p_vkGetDeviceProcAddr( dev->device, "vkDestroyDevice" );
        p_vkDestroyDevice( dev->device, NULL );

        p_vkDestroyInstance = (void *)p_vkGetInstanceProcAddr( dev->instance, "vkDestroyInstance" );
        p_vkDestroyInstance( dev->instance, NULL );
    }

    free( dev );
}

static uint32_t get_vulkan_queue_family( VkInstance instance, VkPhysicalDevice physical_device )
{
    PFN_vkGetPhysicalDeviceQueueFamilyProperties p_vkGetPhysicalDeviceQueueFamilyProperties;

    VkQueueFamilyProperties *props;
    uint32_t i, count;

    p_vkGetPhysicalDeviceQueueFamilyProperties = (void *)p_vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceQueueFamilyProperties" );

    p_vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &count, NULL );
    props = calloc( count, sizeof(*props) );
    ok_ptr( props, !=, NULL );
    p_vkGetPhysicalDeviceQueueFamilyProperties( physical_device, &count, props );
    for (i = 0; i < count; ++i) if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) break;
    free( props );

    return i;
}

static struct vulkan_device *create_vulkan_device( LUID *luid )
{
    static const char *instance_extensions[] =
    {
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_get_physical_device_properties2",
    };
    static const char *device_extensions[] =
    {
        "VK_KHR_get_memory_requirements2",
        "VK_KHR_dedicated_allocation",
        "VK_KHR_external_memory",
        "VK_KHR_external_memory_win32",
    };

    VkDeviceQueueCreateInfo queue_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    VkDeviceCreateInfo create_info = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};

    PFN_vkEnumeratePhysicalDevices p_vkEnumeratePhysicalDevices;
    PFN_vkCreateDevice p_vkCreateDevice;

    VkPhysicalDevice *physical_devices;
    struct vulkan_device *dev;
    float priority = 0.0f;
    uint32_t count;
    VkResult vr;

    dev = calloc( 1, sizeof(*dev) );
    ok_ptr( dev, !=, NULL );

    vr = create_vulkan_instance( ARRAY_SIZE(instance_extensions), instance_extensions, &dev->instance );
    if (vr != VK_SUCCESS) return NULL;

    p_vkEnumeratePhysicalDevices = (void *)p_vkGetInstanceProcAddr( dev->instance, "vkEnumeratePhysicalDevices" );
    vr = p_vkEnumeratePhysicalDevices( dev->instance, &count, NULL );
    ok_vk( VK_SUCCESS, vr );

    physical_devices = calloc( count, sizeof(*physical_devices) );
    ok_ptr( physical_devices, !=, NULL );
    vr = p_vkEnumeratePhysicalDevices( dev->instance, &count, physical_devices );
    ok_vk( VK_SUCCESS, vr );

    for (uint32_t i = 0; i < count; ++i)
    {
        static const VkExternalMemoryHandleTypeFlags expect_export_types = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT |
                                                                           VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
        static const VkExternalMemoryHandleTypeFlags expect_import_types = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT |
                                                                           VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT |
                                                                           VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT |
                                                                           VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT |
                                                                           VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT |
                                                                           VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
        VkExternalMemoryHandleTypeFlags types;

        winetest_push_context( "export" );
        types = get_vulkan_external_image_types( dev->instance, physical_devices[i], VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR );
        todo_wine ok( !(~types & expect_export_types), "got types %#x\n", types );
        winetest_pop_context();

        winetest_push_context( "import" );
        types = get_vulkan_external_image_types( dev->instance, physical_devices[i], VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR );
        todo_wine ok( !(~types & expect_import_types), "got types %#x\n", types );
        winetest_pop_context();
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        LUID device_luid;

        dev->physical_device = physical_devices[i];
        get_vulkan_physical_device_luid( dev->instance, physical_devices[i], &device_luid );
        if (luid_equals( luid, &luid_zero )) *luid = device_luid;
        if (luid_equals( luid, &device_luid )) break;
        dev->physical_device = VK_NULL_HANDLE;
    }
    ok_ptr( dev->physical_device, !=, VK_NULL_HANDLE );

    free( physical_devices );

    queue_info.queueFamilyIndex = get_vulkan_queue_family( dev->instance, dev->physical_device );
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;

    create_info.enabledExtensionCount = ARRAY_SIZE(device_extensions);
    create_info.ppEnabledExtensionNames = device_extensions;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_info;

    p_vkCreateDevice = (void *)p_vkGetInstanceProcAddr( dev->instance, "vkCreateDevice" );
    vr = p_vkCreateDevice( dev->physical_device, &create_info, NULL, &dev->device );
    todo_wine ok_vk( VK_SUCCESS, vr );
    todo_wine ok_ptr( dev->device, !=, VK_NULL_HANDLE );
    if (dev->device == VK_NULL_HANDLE)
    {
        PFN_vkDestroyInstance p_vkDestroyInstance = (void *)p_vkGetInstanceProcAddr( dev->instance, "vkDestroyInstance" );
        p_vkDestroyInstance( dev->instance, NULL );
        free( dev );
        return NULL;
    }

    return dev;
}

static uint32_t find_vulkan_memory_type( VkInstance instance, VkPhysicalDevice physical_device, VkMemoryPropertyFlagBits flags, uint32_t mask )
{
    PFN_vkGetPhysicalDeviceMemoryProperties p_vkGetPhysicalDeviceMemoryProperties;
    VkPhysicalDeviceMemoryProperties properties = {0};
    unsigned int i;

    p_vkGetPhysicalDeviceMemoryProperties = (void *)p_vkGetInstanceProcAddr( instance, "vkGetPhysicalDeviceMemoryProperties" );

    p_vkGetPhysicalDeviceMemoryProperties( physical_device, &properties );
    for (i = 0; i < properties.memoryTypeCount; i++)
    {
        if (!(properties.memoryTypes[i].propertyFlags & flags)) continue;
        if ((1u << i) & mask) return i;
    }

    return -1;
}

static void destroy_vulkan_buffer( struct vulkan_device *dev, struct vulkan_buffer *buf )
{
    PFN_vkFreeMemory p_vkFreeMemory;
    PFN_vkDestroyBuffer p_vkDestroyBuffer;

    p_vkFreeMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkFreeMemory" );
    p_vkDestroyBuffer = (void *)p_vkGetDeviceProcAddr( dev->device, "vkDestroyBuffer" );

    p_vkDestroyBuffer( dev->device, buf->buffer, NULL );
    p_vkFreeMemory( dev->device, buf->memory, NULL );
    free( buf );
}

static struct vulkan_buffer *export_vulkan_buffer( struct vulkan_device *dev, UINT width, const WCHAR *name,
                                                   UINT handle_type, HANDLE *handle )
{
    VkExternalMemoryBufferCreateInfo external_info = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO};
    VkBufferCreateInfo create_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .pNext = &external_info};

    VkMemoryDedicatedAllocateInfoKHR dedicated_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    VkExportMemoryAllocateInfoKHR export_info = {.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR, .pNext = &dedicated_info};
    VkExportMemoryWin32HandleInfoKHR handle_info = {.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR, .pNext = &export_info};
    VkMemoryAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &handle_info};

    VkMemoryGetWin32HandleInfoKHR get_handle_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR};

    PFN_vkAllocateMemory p_vkAllocateMemory;
    PFN_vkBindBufferMemory p_vkBindBufferMemory;
    PFN_vkCreateBuffer p_vkCreateBuffer;
    PFN_vkGetBufferMemoryRequirements p_vkGetBufferMemoryRequirements;
    PFN_vkGetMemoryWin32HandleKHR p_vkGetMemoryWin32HandleKHR;

    VkMemoryRequirements requirements;
    struct vulkan_buffer *buf;
    VkResult vr;

    buf = calloc( 1, sizeof(*buf) );
    ok_ptr( buf, !=, NULL );

    p_vkAllocateMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkAllocateMemory" );
    p_vkBindBufferMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkBindBufferMemory" );
    p_vkCreateBuffer = (void *)p_vkGetDeviceProcAddr( dev->device, "vkCreateBuffer" );
    p_vkGetBufferMemoryRequirements = (void *)p_vkGetDeviceProcAddr( dev->device, "vkGetBufferMemoryRequirements" );
    p_vkGetMemoryWin32HandleKHR = (void *)p_vkGetDeviceProcAddr( dev->device, "vkGetMemoryWin32HandleKHR" );

    external_info.handleTypes = handle_type;
    create_info.size = width;
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vr = p_vkCreateBuffer( dev->device, &create_info, NULL, &buf->buffer );
    ok_vk( VK_SUCCESS, vr );
    p_vkGetBufferMemoryRequirements( dev->device, buf->buffer, &requirements );

    dedicated_info.buffer = buf->buffer;
    export_info.handleTypes = handle_type;
    handle_info.dwAccess = GENERIC_ALL;
    handle_info.name = name;
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = find_vulkan_memory_type( dev->instance, dev->physical_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, requirements.memoryTypeBits );

    vr = p_vkAllocateMemory( dev->device, &alloc_info, NULL, &buf->memory );
    ok_vk( VK_SUCCESS, vr );
    vr = p_vkBindBufferMemory( dev->device, buf->buffer, buf->memory, 0 );
    ok_vk( VK_SUCCESS, vr );

    get_handle_info.memory = buf->memory;
    get_handle_info.handleType = handle_type;
    vr = p_vkGetMemoryWin32HandleKHR( dev->device, &get_handle_info, handle );
    ok_vk( VK_SUCCESS, vr );

    return buf;
}

static VkResult import_vulkan_buffer( struct vulkan_device *dev, UINT width, const WCHAR *name,
                                      HANDLE handle, UINT handle_type, struct vulkan_buffer **out )
{
    VkExternalMemoryBufferCreateInfo external_info = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO};
    VkBufferCreateInfo create_info = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .pNext = &external_info};

    VkMemoryDedicatedAllocateInfoKHR dedicated_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    VkImportMemoryWin32HandleInfoKHR handle_info = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, .pNext = &dedicated_info};
    VkMemoryAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &handle_info};

    PFN_vkAllocateMemory p_vkAllocateMemory;
    PFN_vkBindBufferMemory p_vkBindBufferMemory;
    PFN_vkCreateBuffer p_vkCreateBuffer;
    PFN_vkGetBufferMemoryRequirements p_vkGetBufferMemoryRequirements;

    VkMemoryRequirements requirements;
    struct vulkan_buffer *buf;
    VkResult vr;

    buf = calloc( 1, sizeof(*buf) );
    ok_ptr( buf, !=, NULL );

    p_vkAllocateMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkAllocateMemory" );
    p_vkBindBufferMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkBindBufferMemory" );
    p_vkCreateBuffer = (void *)p_vkGetDeviceProcAddr( dev->device, "vkCreateBuffer" );
    p_vkGetBufferMemoryRequirements = (void *)p_vkGetDeviceProcAddr( dev->device, "vkGetBufferMemoryRequirements" );

    external_info.handleTypes = handle_type;
    create_info.size = width;
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vr = p_vkCreateBuffer( dev->device, &create_info, NULL, &buf->buffer );
    ok_vk( VK_SUCCESS, vr );
    p_vkGetBufferMemoryRequirements( dev->device, buf->buffer, &requirements );

    dedicated_info.buffer = buf->buffer;
    handle_info.handle = name ? NULL : handle;
    handle_info.name = name;
    handle_info.handleType = handle_type;
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = find_vulkan_memory_type( dev->instance, dev->physical_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, requirements.memoryTypeBits );

    if ((vr = p_vkAllocateMemory( dev->device, &alloc_info, NULL, &buf->memory )))
    {
        PFN_vkDestroyBuffer p_vkDestroyBuffer;
        p_vkDestroyBuffer = (void *)p_vkGetDeviceProcAddr( dev->device, "vkDestroyBuffer" );
        p_vkDestroyBuffer( dev->device, buf->buffer, NULL );
        free( buf );
        *out = NULL;
        return vr;
    }
    vr = p_vkBindBufferMemory( dev->device, buf->buffer, buf->memory, 0 );
    ok_vk( VK_SUCCESS, vr );

    *out = buf;
    return VK_SUCCESS;
}

static void destroy_vulkan_image( struct vulkan_device *dev, struct vulkan_image *img )
{
    PFN_vkFreeMemory p_vkFreeMemory;
    PFN_vkDestroyImage p_vkDestroyImage;

    p_vkFreeMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkFreeMemory" );
    p_vkDestroyImage = (void *)p_vkGetDeviceProcAddr( dev->device, "vkDestroyImage" );

    p_vkDestroyImage( dev->device, img->image, NULL );
    p_vkFreeMemory( dev->device, img->memory, NULL );
    free( img );
}

static struct vulkan_image *export_vulkan_image( struct vulkan_device *dev, UINT width, UINT height, UINT depth,
                                                 const WCHAR *name, UINT handle_type, HANDLE *handle )
{
    VkExternalMemoryImageCreateInfo external_info = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
    VkImageCreateInfo create_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = &external_info};

    VkMemoryDedicatedAllocateInfoKHR dedicated_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    VkExportMemoryAllocateInfoKHR export_info = {.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR, .pNext = &dedicated_info};
    VkExportMemoryWin32HandleInfoKHR handle_info = {.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR, .pNext = &export_info};
    VkMemoryAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &handle_info};

    VkMemoryGetWin32HandleInfoKHR get_handle_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR};

    PFN_vkAllocateMemory p_vkAllocateMemory;
    PFN_vkBindImageMemory p_vkBindImageMemory;
    PFN_vkCreateImage p_vkCreateImage;
    PFN_vkGetImageMemoryRequirements p_vkGetImageMemoryRequirements;
    PFN_vkGetMemoryWin32HandleKHR p_vkGetMemoryWin32HandleKHR;

    VkMemoryRequirements requirements;
    struct vulkan_image *img;
    VkResult vr;

    img = calloc( 1, sizeof(*img) );
    ok_ptr( img, !=, NULL );

    p_vkAllocateMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkAllocateMemory" );
    p_vkBindImageMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkBindImageMemory" );
    p_vkCreateImage = (void *)p_vkGetDeviceProcAddr( dev->device, "vkCreateImage" );
    p_vkGetImageMemoryRequirements = (void *)p_vkGetDeviceProcAddr( dev->device, "vkGetImageMemoryRequirements" );
    p_vkGetMemoryWin32HandleKHR = (void *)p_vkGetDeviceProcAddr( dev->device, "vkGetMemoryWin32HandleKHR" );

    external_info.handleTypes = handle_type;
    if (depth > 1 && height > 1) create_info.imageType = VK_IMAGE_TYPE_3D;
    else if (height > 1) create_info.imageType = VK_IMAGE_TYPE_2D;
    else create_info.imageType = VK_IMAGE_TYPE_1D;
    create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = height > 1 ? depth : 1;
    create_info.arrayLayers = height == 1 ? depth : 1;
    create_info.mipLevels = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vr = p_vkCreateImage( dev->device, &create_info, NULL, &img->image );
    ok_vk( VK_SUCCESS, vr );
    p_vkGetImageMemoryRequirements( dev->device, img->image, &requirements );

    dedicated_info.image = img->image;
    export_info.handleTypes = handle_type;
    handle_info.dwAccess = GENERIC_ALL;
    handle_info.name = name;
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = find_vulkan_memory_type( dev->instance, dev->physical_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, requirements.memoryTypeBits );

    vr = p_vkAllocateMemory( dev->device, &alloc_info, NULL, &img->memory );
    ok_vk( VK_SUCCESS, vr );
    vr = p_vkBindImageMemory( dev->device, img->image, img->memory, 0 );
    ok_vk( VK_SUCCESS, vr );

    get_handle_info.memory = img->memory;
    get_handle_info.handleType = handle_type;
    vr = p_vkGetMemoryWin32HandleKHR( dev->device, &get_handle_info, handle );
    ok_vk( VK_SUCCESS, vr );

    return img;
}

static VkResult import_vulkan_image( struct vulkan_device *dev, UINT width, UINT height, UINT depth,
                                     const WCHAR *name, HANDLE handle, UINT handle_type, struct vulkan_image **out )
{
    VkExternalMemoryImageCreateInfo external_info = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
    VkImageCreateInfo create_info = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = &external_info};

    VkMemoryDedicatedAllocateInfoKHR dedicated_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    VkImportMemoryWin32HandleInfoKHR handle_info = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, .pNext = &dedicated_info};
    VkMemoryAllocateInfo alloc_info = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &handle_info};

    PFN_vkAllocateMemory p_vkAllocateMemory;
    PFN_vkBindImageMemory p_vkBindImageMemory;
    PFN_vkCreateImage p_vkCreateImage;
    PFN_vkGetImageMemoryRequirements p_vkGetImageMemoryRequirements;

    VkMemoryRequirements requirements;
    struct vulkan_image *img;
    VkResult vr;

    img = calloc( 1, sizeof(*img) );
    ok_ptr( img, !=, NULL );

    p_vkAllocateMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkAllocateMemory" );
    p_vkBindImageMemory = (void *)p_vkGetDeviceProcAddr( dev->device, "vkBindImageMemory" );
    p_vkCreateImage = (void *)p_vkGetDeviceProcAddr( dev->device, "vkCreateImage" );
    p_vkGetImageMemoryRequirements = (void *)p_vkGetDeviceProcAddr( dev->device, "vkGetImageMemoryRequirements" );

    external_info.handleTypes = handle_type;
    if (depth > 1 && height > 1) create_info.imageType = VK_IMAGE_TYPE_3D;
    else if (height > 1) create_info.imageType = VK_IMAGE_TYPE_2D;
    else create_info.imageType = VK_IMAGE_TYPE_1D;
    create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    create_info.extent.width = width;
    create_info.extent.height = height;
    create_info.extent.depth = height > 1 ? depth : 1;
    create_info.arrayLayers = height == 1 ? depth : 1;
    create_info.mipLevels = 1;
    create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    vr = p_vkCreateImage( dev->device, &create_info, NULL, &img->image );
    ok_vk( VK_SUCCESS, vr );
    p_vkGetImageMemoryRequirements( dev->device, img->image, &requirements );

    dedicated_info.image = img->image;
    handle_info.handle = name ? NULL : handle;
    handle_info.name = name;
    handle_info.handleType = handle_type;
    alloc_info.allocationSize = requirements.size;
    alloc_info.memoryTypeIndex = find_vulkan_memory_type( dev->instance, dev->physical_device, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, requirements.memoryTypeBits );

    if ((vr = p_vkAllocateMemory( dev->device, &alloc_info, NULL, &img->memory )))
    {
        PFN_vkDestroyImage p_vkDestroyImage;
        p_vkDestroyImage = (void *)p_vkGetDeviceProcAddr( dev->device, "vkDestroyImage" );
        p_vkDestroyImage( dev->device, img->image, NULL );
        free( img );
        *out = NULL;
        return vr;
    }
    vr = p_vkBindImageMemory( dev->device, img->image, img->memory, 0 );
    ok_vk( VK_SUCCESS, vr );

    *out = img;
    return VK_SUCCESS;
}

C_ASSERT( sizeof(GUID) == GL_UUID_SIZE_EXT );
C_ASSERT( sizeof(LUID) == GL_LUID_SIZE_EXT );

struct opengl_device
{
    HWND  hwnd;
    HDC   hdc;
    HGLRC rc;
    BOOL  broken;
};

static void destroy_opengl_device( struct opengl_device *dev )
{
    UINT ret;

    ret = wglMakeCurrent( dev->hdc, NULL );
    ok_u4( ret, !=, 0 );
    ret = wglDeleteContext( dev->rc );
    ok_u4( ret, !=, 0 );
    ret = ReleaseDC( dev->hwnd, dev->hdc );
    ok_u4( ret, !=, 0 );

    free( dev );
}

static const char *find_opengl_extension( const char *extensions, const char *name )
{
    size_t len = strlen( name );
    const char *ptr;
    if (!(ptr = strstr( extensions, name ))) return NULL;
    if (ptr != extensions && ptr[-1] != ' ') return NULL;
    if (ptr[len] != 0 && ptr[len] != ' ') return NULL;
    return ptr;
}

static struct opengl_device *create_opengl_device( HWND hwnd, LUID *luid )
{
    static const PIXELFORMATDESCRIPTOR pfd = {.dwFlags = PFD_DRAW_TO_WINDOW, .cColorBits = 24, .cRedBits = 8};

    PFN_glGetUnsignedBytevEXT p_glGetUnsignedBytevEXT;
    PFN_glGetUnsignedBytei_vEXT p_glGetUnsignedBytei_vEXT;

    const char *extensions, *ptr;
    struct opengl_device *dev;
    int format, count;
    GUID guid;
    BOOL ret;

    dev = calloc( 1, sizeof(*dev) );
    ok_ptr( dev, !=, NULL );

    dev->hwnd = hwnd;
    dev->hdc = GetWindowDC( hwnd );
    ok_ptr( dev->hdc, !=, NULL );

    format = ChoosePixelFormat( dev->hdc, &pfd );
    ok_u4( format, !=, 0 );
    SetPixelFormat( dev->hdc, format, &pfd );

    dev->rc = wglCreateContext( dev->hdc );
    ok_ptr( dev->rc, !=, NULL );
    ret = wglMakeCurrent( dev->hdc, dev->rc );
    ok_u4( ret, !=, 0 );

    p_glGetUnsignedBytevEXT = (void *)wglGetProcAddress( "glGetUnsignedBytevEXT" );
    if (!p_glGetUnsignedBytevEXT)
    {
        destroy_opengl_device( dev );
        return NULL;
    }
    p_glGetUnsignedBytei_vEXT = (void *)wglGetProcAddress( "glGetUnsignedBytei_vEXT" );
    ok_ptr( p_glGetUnsignedBytei_vEXT, !=, NULL );

    extensions = (char *)glGetString( GL_EXTENSIONS );
    ok_x4( glGetError(), ==, 0 );
    ok_ptr( extensions, !=, NULL );

    ptr = find_opengl_extension( extensions, "GL_EXT_memory_object_win32" );
    todo_wine ok_ptr( ptr, !=, NULL );
    ptr = find_opengl_extension( extensions, "GL_EXT_semaphore_win32" );
    todo_wine ok_ptr( ptr, !=, NULL );
    ptr = find_opengl_extension( extensions, "GL_EXT_win32_keyed_mutex" );
    dev->broken = !winetest_platform_is_wine && ptr == NULL; /* missing on AMD, as is support for importing D3D handles */

    count = 0;
    glGetIntegerv( GL_NUM_DEVICE_UUIDS_EXT, &count );
    ok_x4( glGetError(), ==, 0 );
    ok_u4( count, ==, 1 );

    for (int i = 0; i < count; i++)
    {
        memset( &guid, 0, sizeof(guid) );
        p_glGetUnsignedBytei_vEXT( GL_DEVICE_UUID_EXT, i, (GLubyte *)&guid );
        ok_x4( glGetError(), ==, 0 );
        ok( !IsEqualGUID( &GUID_NULL, &guid ), "got GL_DEVICE_UUID_EXT[%u] %s\n", i, debugstr_guid( &guid ) );
    }

    memset( &guid, 0, sizeof(guid) );
    p_glGetUnsignedBytevEXT( GL_DRIVER_UUID_EXT, (GLubyte *)&guid );
    ok_x4( glGetError(), ==, 0 );
    ok( !IsEqualGUID( &GUID_NULL, &guid ), "got GL_DRIVER_UUID_EXT %s\n", debugstr_guid( &guid ) );

    if (!winetest_platform_is_wine) /* crashes in host drivers */
    {
        LUID device_luid = {0};
        unsigned int nodes = 0;

        p_glGetUnsignedBytevEXT( GL_DEVICE_LUID_EXT, (GLubyte *)&device_luid );
        if (!dev->broken) ok_x4( glGetError(), ==, 0 );
        else ok_x4( glGetError(), ==, GL_INVALID_ENUM );

        glGetIntegerv( GL_DEVICE_NODE_MASK_EXT, (GLint *)&nodes );
        if (!dev->broken) ok_x4( glGetError(), ==, 0 );
        else ok_x4( glGetError(), ==, GL_INVALID_ENUM );
        if (!dev->broken) ok_u4( nodes, !=, 0 );

        if (luid_equals( luid, &luid_zero )) *luid = device_luid;
    }

    return dev;
}

static void import_opengl_image( struct opengl_device *dev, UINT width, UINT height, UINT depth, UINT bpp,
                                 const WCHAR *name, HANDLE handle, UINT handle_type )
{
    PFN_glCreateMemoryObjectsEXT p_glCreateMemoryObjectsEXT;
    PFN_glDeleteMemoryObjectsEXT p_glDeleteMemoryObjectsEXT;
    GLuint memory;

    UINT ret;

    if (dev->broken && handle_type != GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT && handle_type != GL_HANDLE_TYPE_OPAQUE_WIN32_EXT)
    {
        win_skip( "Skipping unsupported handle types\n" );
        return;
    }

    ret = wglMakeCurrent( dev->hdc, dev->rc );
    todo_wine_if( ret == 0 ) ok_u4( ret, !=, 0 );

    p_glCreateMemoryObjectsEXT = (void *)wglGetProcAddress( "glCreateMemoryObjectsEXT" );
    ok_ptr( p_glCreateMemoryObjectsEXT, !=, NULL );
    p_glDeleteMemoryObjectsEXT = (void *)wglGetProcAddress( "glDeleteMemoryObjectsEXT" );
    ok_ptr( p_glDeleteMemoryObjectsEXT, !=, NULL );

    if (name)
    {
        PFN_glImportMemoryWin32NameEXT p_glImportMemoryWin32NameEXT = (void *)wglGetProcAddress( "glImportMemoryWin32NameEXT" );
        todo_wine ok_ptr( p_glImportMemoryWin32NameEXT, !=, NULL );
        if (!p_glImportMemoryWin32NameEXT) return;

        p_glCreateMemoryObjectsEXT( 1, &memory );
        p_glImportMemoryWin32NameEXT( memory, width * height * depth * bpp, handle_type, name );
        p_glDeleteMemoryObjectsEXT( 1, &memory );
    }
    else
    {
        PFN_glImportMemoryWin32HandleEXT p_glImportMemoryWin32HandleEXT = (void *)wglGetProcAddress( "glImportMemoryWin32HandleEXT" );
        todo_wine ok_ptr( p_glImportMemoryWin32HandleEXT, !=, NULL );
        if (!p_glImportMemoryWin32HandleEXT) return;

        p_glCreateMemoryObjectsEXT( 1, &memory );
        p_glImportMemoryWin32HandleEXT( memory, width * height * depth * bpp, handle_type, handle );
        p_glDeleteMemoryObjectsEXT( 1, &memory );
    }
}

static HRESULT get_dxgi_global_handle( IUnknown *obj, HANDLE *handle )
{
    IDXGIResource *res;
    HRESULT hr;

    hr = IUnknown_QueryInterface( obj, &IID_IDXGIResource, (void **)&res );
    ok_hr( S_OK, hr );
    hr = IDXGIResource_GetSharedHandle( res, handle );
    IDXGIResource_Release( res );

    return hr;
}

static HRESULT get_dxgi_shared_handle( IUnknown *obj, const WCHAR *name, HANDLE *handle )
{
    IDXGIResource1 *res;
    HRESULT hr;

    hr = IUnknown_QueryInterface( obj, &IID_IDXGIResource1, (void **)&res );
    ok_hr( S_OK, hr );
    hr = IDXGIResource1_CreateSharedHandle( res, NULL, GENERIC_ALL, name, handle );
    IDXGIResource1_Release( res );

    return hr;
}

static HRESULT get_d3d12_shared_handle( ID3D12Device *d3d12, IUnknown *obj, const WCHAR *name, HANDLE *handle )
{
    ID3D12DeviceChild *child;
    HRESULT hr;

    hr = IUnknown_QueryInterface( obj, &IID_ID3D12DeviceChild, (void **)&child );
    ok_hr( S_OK, hr );
    hr = ID3D12Device_CreateSharedHandle( d3d12, child, NULL, GENERIC_ALL, name, handle );
    ID3D12DeviceChild_Release( child );

    return hr;
}

static void test_shared_resources(void)
{
    struct vulkan_device *vulkan_imp = NULL, *vulkan_exp = NULL;
    struct opengl_device *opengl_imp = NULL;
    IDirect3DDevice9Ex *d3d9_exp = NULL, *d3d9_imp = NULL;
    ID3D11Device1 *d3d11_exp = NULL, *d3d11_imp = NULL;
    ID3D10Device *d3d10_exp = NULL, *d3d10_imp = NULL;
    ID3D12Device *d3d12_exp = NULL, *d3d12_imp = NULL;
    IDXGIFactory3 *dxgi = NULL;
    IDXGIAdapter *adapter;
    BOOL stencil_broken;
    LUID luid = {0};
    HRESULT hr;
    HWND hwnd;
    MSG msg;

    hwnd = CreateWindowW( L"static", NULL, WS_POPUP | WS_VISIBLE, 100, 100, 200, 200, NULL, NULL, NULL, NULL );
    ok_ptr( hwnd, !=, NULL );
    while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    if (!(opengl_imp = create_opengl_device( hwnd, &luid )))
    {
        win_skip( "Skipping tests on software renderer\n" );
        DestroyWindow( hwnd );
        return;
    }

    trace( "adapter luid %s\n", debugstr_luid( &luid ) );

    vulkan_exp = create_vulkan_device( &luid );
    vulkan_imp = create_vulkan_device( &luid );

    d3d9_exp = create_d3d9ex_device( hwnd, &luid, &stencil_broken );
    ok_ptr( d3d9_exp, !=, NULL );
    d3d9_imp = create_d3d9ex_device( hwnd, &luid, NULL );
    ok_ptr( d3d9_imp, !=, NULL );

    hr = CreateDXGIFactory1( &IID_IDXGIFactory3, (void **)&dxgi );
    ok_hr( S_OK, hr );

    adapter = create_dxgi_adapter( dxgi, &luid );
    ok_ptr( adapter, !=, NULL );

    hr = D3D10CreateDevice( adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, D3D10_CREATE_DEVICE_BGRA_SUPPORT,
                            D3D10_SDK_VERSION, &d3d10_imp );
    ok_hr( S_OK, hr );
    hr = D3D10CreateDevice( adapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, D3D10_CREATE_DEVICE_BGRA_SUPPORT,
                            D3D10_SDK_VERSION, &d3d10_exp );
    ok_hr( S_OK, hr );

    d3d11_imp = create_d3d11_device( adapter );
    ok_ptr( d3d11_imp, !=, NULL );
    d3d11_exp = create_d3d11_device( adapter );
    ok_ptr( d3d11_exp, !=, NULL );

    hr = D3D12CreateDevice( (IUnknown *)adapter, D3D_FEATURE_LEVEL_12_0,
                            &IID_ID3D12Device, (void **)&d3d12_imp );
    todo_wine ok_hr( S_OK, hr );
    hr = D3D12CreateDevice( (IUnknown *)adapter, D3D_FEATURE_LEVEL_12_0,
                            &IID_ID3D12Device, (void **)&d3d12_exp );
    todo_wine ok_hr( S_OK, hr );

    IDXGIAdapter_Release( adapter );

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
        IUnknown *export = NULL, *import = NULL;
        struct vulkan_buffer *buf = NULL;
        struct vulkan_image *img = NULL;
        const WCHAR *name = NULL;
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

        case MAKETEST(1, 0, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_BUFFER, .d3d10_buf = {
                .ByteWidth = resource_size, .Usage = D3D10_USAGE_DEFAULT, .BindFlags = D3D10_BIND_SHADER_RESOURCE, .CPUAccessFlags = D3D10_CPU_ACCESS_READ,
                .MiscFlags = D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            }};
            hr = ID3D10Device_CreateBuffer( d3d10_exp, &desc.d3d10_buf, NULL, (ID3D10Buffer **)&export );
            ok_hr( E_INVALIDARG, hr );
            if (!export) break;
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, (struct d3d11_runtime_desc *)&desc );
            break;
        }
        case MAKETEST(1, 1, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D, .d3d10_1d = {
                .Width = width_1d, .MipLevels = 1, .ArraySize = array_1d, .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .Usage = D3D10_USAGE_DEFAULT, .BindFlags = D3D10_BIND_RENDER_TARGET, .CPUAccessFlags = D3D10_CPU_ACCESS_READ,
                .MiscFlags = D3D10_RESOURCE_MISC_SHARED,
            }};
            hr = ID3D10Device_CreateTexture1D( d3d10_exp, &desc.d3d10_1d, NULL, (ID3D10Texture1D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, (struct d3d11_runtime_desc *)&desc );
            break;
        }
        case MAKETEST(1, 2, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D, .d3d10_2d = {
                .Width = width_2d, .Height = height_2d, .MipLevels = 1, .ArraySize = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1,
                .Usage = D3D10_USAGE_DEFAULT, .BindFlags = D3D10_BIND_RENDER_TARGET, .CPUAccessFlags = D3D10_CPU_ACCESS_READ,
                .MiscFlags = D3D10_RESOURCE_MISC_SHARED,
            }};
            hr = ID3D10Device_CreateTexture2D( d3d10_exp, &desc.d3d10_2d, NULL, (ID3D10Texture2D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, (struct d3d11_runtime_desc *)&desc );
            break;
        }
        case MAKETEST(1, 2, 1):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4, .keyed_mutex = 1};
            struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D, .d3d10_2d = {
                .Width = width_2d, .Height = height_2d, .MipLevels = 1, .ArraySize = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1,
                .Usage = D3D10_USAGE_DEFAULT, .BindFlags = D3D10_BIND_RENDER_TARGET, .CPUAccessFlags = D3D10_CPU_ACCESS_READ,
                .MiscFlags = D3D10_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            }};
            hr = ID3D10Device_CreateTexture2D( d3d10_exp, &desc.d3d10_2d, NULL, (ID3D10Texture2D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            desc.d3d11_2d.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX; /* flag gets updated to the d3d11 version */
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, (struct d3d11_runtime_desc *)&desc );
            break;
        }
        case MAKETEST(1, 3, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D, .d3d10_3d = {
                .Width = width_3d, .Height = height_3d, .Depth = depth_3d, .MipLevels = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .Usage = D3D10_USAGE_DEFAULT, .BindFlags = D3D10_BIND_RENDER_TARGET, .CPUAccessFlags = D3D10_CPU_ACCESS_READ,
                .MiscFlags = D3D10_RESOURCE_MISC_SHARED,
            }};
            hr = ID3D10Device_CreateTexture3D( d3d10_exp, &desc.d3d10_3d, NULL, (ID3D10Texture3D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, (struct d3d11_runtime_desc *)&desc );
            break;
        }

        case MAKETEST(2, 0, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_BUFFER, .d3d11_buf = {
                .ByteWidth = resource_size, .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_SHADER_RESOURCE, .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            }};
            hr = ID3D11Device1_CreateBuffer( d3d11_exp, &desc.d3d11_buf, NULL, (ID3D11Buffer **)&export );
            todo_wine ok_hr( E_INVALIDARG, hr );
            if (!export) break;
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(2, 1, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D, .d3d11_1d = {
                .Width = width_1d, .MipLevels = 1, .ArraySize = array_1d, .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_SHADER_RESOURCE, .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED,
            }};
            hr = ID3D11Device1_CreateTexture1D( d3d11_exp, &desc.d3d11_1d, NULL, (ID3D11Texture1D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(2, 1, 1):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4, .keyed_mutex = 1};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D, .d3d11_1d = {
                .Width = width_1d, .MipLevels = 1, .ArraySize = array_1d, .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_SHADER_RESOURCE, .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            }};
            hr = ID3D11Device1_CreateTexture1D( d3d11_exp, &desc.d3d11_1d, NULL, (ID3D11Texture1D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(2, 2, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D, .d3d11_2d = {
                .Width = width_2d, .Height = height_2d, .MipLevels = 1, .ArraySize = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_RENDER_TARGET, .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED,
            }};
            hr = ID3D11Device1_CreateTexture2D( d3d11_exp, &desc.d3d11_2d, NULL, (ID3D11Texture2D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(2, 2, 1):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4, .nt_shared = 1};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D, .d3d11_2d = {
                .Width = width_2d, .Height = height_2d, .MipLevels = 1, .ArraySize = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_RENDER_TARGET, .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
            }};
            hr = ID3D11Device1_CreateTexture2D( d3d11_exp, &desc.d3d11_2d, NULL, (ID3D11Texture2D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(2, 2, 2):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4, .keyed_mutex = 1};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D, .d3d11_2d = {
                .Width = width_2d, .Height = height_2d, .MipLevels = 1, .ArraySize = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_SHADER_RESOURCE, .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            }};
            hr = ID3D11Device1_CreateTexture2D( d3d11_exp, &desc.d3d11_2d, NULL, (ID3D11Texture2D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(2, 2, 3):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4, .nt_shared = 1};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D, .d3d11_2d = {
                .Width = width_2d, .Height = height_2d, .MipLevels = 1, .ArraySize = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_RENDER_TARGET, .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
            }};
            name = L"__winetest_d3d11_image";
            hr = ID3D11Device1_CreateTexture2D( d3d11_exp, &desc.d3d11_2d, NULL, (ID3D11Texture2D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_shared_handle( export, name, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(2, 3, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D, .d3d11_3d = {
                .Width = width_3d, .Height = height_3d, .Depth = depth_3d, .MipLevels = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_SHADER_RESOURCE, .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED,
            }};
            hr = ID3D11Device1_CreateTexture3D( d3d11_exp, &desc.d3d11_3d, NULL, (ID3D11Texture3D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(2, 3, 1):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4, .keyed_mutex = 1};
            const struct d3d11_runtime_desc desc = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D, .d3d11_3d = {
                .Width = width_3d, .Height = height_3d, .Depth = depth_3d, .MipLevels = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_SHADER_RESOURCE, .CPUAccessFlags = D3D11_CPU_ACCESS_READ,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            }};
            hr = ID3D11Device1_CreateTexture3D( d3d11_exp, &desc.d3d11_3d, NULL, (ID3D11Texture3D **)&export );
            ok_hr( S_OK, hr );
            hr = get_dxgi_shared_handle( export, NULL, &handle );
            todo_wine ok_hr( E_INVALIDARG, hr );
            hr = get_dxgi_global_handle( export, &handle );
            todo_wine ok_hr( S_OK, hr );
            if (hr != S_OK) break;
            get_d3dkmt_resource_desc( luid, handle, TRUE, sizeof(desc), runtime_desc );
            check_d3d11_runtime_desc( (struct d3d11_runtime_desc *)runtime_desc, &desc );
            break;
        }

        case MAKETEST(3, 0, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .nt_shared = 1};
            const struct d3d12_runtime_desc desc = {.d3d11.dxgi = dxgi, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER, .Width = resource_size, .Height = 1, .DepthOrArraySize = 1, .MipLevels = 1,
                .Format = DXGI_FORMAT_UNKNOWN, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, NULL, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(3, 1, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .nt_shared = 1};
            struct d3d12_runtime_desc desc = {.d3d11.dxgi = dxgi, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D, .Width = width_1d, .Height = 1, .DepthOrArraySize = array_1d, .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, NULL, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            if (!opengl_imp->broken) desc.resource_size = 0x800000; /* each plane is aligned on NVIDIA */
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(3, 2, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .nt_shared = 1};
            const struct d3d12_runtime_desc desc = {.d3d11.dxgi = dxgi, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D, .Width = width_2d, .Height = height_2d, .DepthOrArraySize = 1, .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, NULL, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(3, 2, 1):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .nt_shared = 1};
            const struct d3d12_runtime_desc desc = {.d3d11.dxgi = dxgi, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D, .Width = width_2d, .Height = height_2d, .DepthOrArraySize = 1, .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, NULL, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(3, 2, 2):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .nt_shared = 1, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_R8G8B8A8_UNORM};
            const struct d3d12_runtime_desc desc = {.d3d11.dxgi = dxgi, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D, .Width = width_2d, .Height = height_2d, .DepthOrArraySize = 1, .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, NULL, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(3, 2, 3):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .nt_shared = 1};
            const struct d3d11_runtime_desc d3d11 = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D, .d3d11_2d = {
                .Width = width_2d, .Height = height_2d, .MipLevels = 1, .ArraySize = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, .CPUAccessFlags = 0,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
            }};
            const struct d3d12_runtime_desc desc = {.d3d11 = d3d11, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D, .Width = width_2d, .Height = height_2d, .DepthOrArraySize = 1, .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, NULL, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(3, 2, 4):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .nt_shared = 1};
            const struct d3d12_runtime_desc desc = {.d3d11.dxgi = dxgi, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D, .Width = width_2d, .Height = height_2d, .DepthOrArraySize = 1, .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, NULL, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(3, 2, 5):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .version = 4, .width = width_2d, .height = height_2d, .format = DXGI_FORMAT_R8G8B8A8_UNORM, .nt_shared = 1};
            const struct d3d11_runtime_desc d3d11 = {.dxgi = dxgi, .dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D, .d3d11_2d = {
                .Width = width_2d, .Height = height_2d, .MipLevels = 1, .ArraySize = 1, .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1,
                .Usage = D3D11_USAGE_DEFAULT, .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, .CPUAccessFlags = 0,
                .MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
            }};
            const struct d3d12_runtime_desc desc = {.d3d11 = d3d11, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D, .Width = width_2d, .Height = height_2d, .DepthOrArraySize = 1, .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            name = L"__winetest_d3d12_image";
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, name, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }
        case MAKETEST(3, 3, 0):
        {
            const struct dxgi_runtime_desc dxgi = {.size = 0x68, .nt_shared = 1};
            const struct d3d12_runtime_desc desc = {.d3d11.dxgi = dxgi, .resource_size = resource_size, .resource_align = 0x10000, .desc = {
                .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D, .Width = width_3d, .Height = height_3d, .DepthOrArraySize = depth_3d, .MipLevels = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .SampleDesc.Count = 1, .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = D3D12_RESOURCE_FLAG_NONE,
            }};
            D3D12_HEAP_PROPERTIES heap_props = {.Type = D3D12_HEAP_TYPE_DEFAULT};
            if (!d3d12_exp) break;
            hr = ID3D12Device_CreateCommittedResource( d3d12_exp, &heap_props, D3D12_HEAP_FLAG_SHARED, &desc.desc, D3D12_RESOURCE_STATE_COMMON,
                                                       NULL, &IID_ID3D12Resource, (void **)&export );
            ok_hr( S_OK, hr );
            hr = get_d3d12_shared_handle( d3d12_exp, export, NULL, &handle );
            ok_hr( S_OK, hr );
            get_d3dkmt_resource_desc( luid, handle, FALSE, sizeof(desc), runtime_desc );
            check_d3d12_runtime_desc( (struct d3d12_runtime_desc *)runtime_desc, &desc );
            break;
        }

        case MAKETEST(4, 0, 0):
        {
            if (!vulkan_exp) break;
            buf = export_vulkan_buffer( vulkan_exp, resource_size, NULL, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, &handle );
            get_d3dkmt_resource_desc( luid, handle, FALSE, 0, runtime_desc );
            break;
        }
        case MAKETEST(4, 0, 1):
        {
            if (!vulkan_exp) break;
            buf = export_vulkan_buffer( vulkan_exp, resource_size, NULL, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT, &handle );
            get_d3dkmt_resource_desc( luid, handle, TRUE, 0, runtime_desc );
            break;
        }
        case MAKETEST(4, 1, 0):
        {
            if (!vulkan_exp) break;
            img = export_vulkan_image( vulkan_exp, width_1d, 1, array_1d, NULL, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, &handle );
            get_d3dkmt_resource_desc( luid, handle, FALSE, 0, runtime_desc );
            break;
        }
        case MAKETEST(4, 2, 0):
        {
            if (!vulkan_exp) break;
            img = export_vulkan_image( vulkan_exp, width_2d, height_2d, 1, NULL, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, &handle );
            get_d3dkmt_resource_desc( luid, handle, FALSE, 0, runtime_desc );
            break;
        }
        case MAKETEST(4, 2, 1):
        {
            if (!vulkan_exp) break;
            img = export_vulkan_image( vulkan_exp, width_2d, height_2d, 1, NULL, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT, &handle );
            get_d3dkmt_resource_desc( luid, handle, TRUE, 0, runtime_desc );
            break;
        }
        case MAKETEST(4, 3, 0):
        {
            if (!vulkan_exp) break;
            img = export_vulkan_image( vulkan_exp, width_3d, height_3d, depth_3d, NULL, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, &handle );
            get_d3dkmt_resource_desc( luid, handle, FALSE, 0, runtime_desc );
            break;
        }
        }
        if (!handle) goto skip_tests;

        if (name)
        {
            WCHAR path[MAX_PATH];
            swprintf( path, ARRAY_SIZE(path), L"\\Sessions\\1\\BaseNamedObjects\\%s", name );
            todo_wine check_object_name( handle, path );
        }

        if (d3d9_imp && GET_API(test) <= 3)
        {
            const struct d3d9_runtime_desc *desc = (struct d3d9_runtime_desc *)runtime_desc;

            hr = IDirect3DDevice9Ex_CreateTexture( d3d9_imp, width_2d, height_2d, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DTexture9 **)&import, &handle );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_TEXTURE) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->usage != D3DUSAGE_SURFACE) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateVolumeTexture( d3d9_imp, width_3d, height_3d, depth_3d, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DVolumeTexture9 **)&import, &handle );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_VOLUMETEXTURE) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateCubeTexture( d3d9_imp, width_cube, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DCubeTexture9 **)&import, &handle );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_CUBETEXTURE) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateVertexBuffer( d3d9_imp, resource_size / (4 * sizeof(float)), 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, (IDirect3DVertexBuffer9 **)&import, &handle );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_VERTEXBUFFER) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateIndexBuffer( d3d9_imp, resource_size / 2, 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, (IDirect3DIndexBuffer9 **)&import, &handle );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_INDEXBUFFER) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateRenderTarget( d3d9_imp, width_2d, height_2d, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, (IDirect3DSurface9 **)&import, &handle );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_SURFACE) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (!(desc->usage & D3DUSAGE_RENDERTARGET)) todo_wine ok_hr( E_INVALIDARG, hr );
            else if ((desc->usage & D3DUSAGE_LOCKABLE)) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateDepthStencilSurface( d3d9_imp, width_2d, height_2d, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, (IDirect3DSurface9 **)&import, &handle );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_SURFACE) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (!(desc->usage & D3DUSAGE_DEPTHSTENCIL)) todo_wine ok_hr( E_INVALIDARG, hr );
            else if ((desc->usage & D3DUSAGE_LOCKABLE)) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface( d3d9_imp, width_2d, height_2d, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DSurface9 **)&import, &handle );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_SURFACE) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (!(desc->usage & D3DUSAGE_OFFSCREEN)) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateRenderTargetEx( d3d9_imp, width_2d, height_2d, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, (IDirect3DSurface9 **)&import, &handle, 0 );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_SURFACE) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (!(desc->usage & D3DUSAGE_RENDERTARGET)) todo_wine ok_hr( E_INVALIDARG, hr );
            else if ((desc->usage & D3DUSAGE_LOCKABLE)) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx( d3d9_imp, width_2d, height_2d, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DSurface9 **)&import, &handle, 0 );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_SURFACE) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (!(desc->usage & D3DUSAGE_OFFSCREEN)) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = IDirect3DDevice9Ex_CreateDepthStencilSurfaceEx( d3d9_imp, width_2d, height_2d, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, FALSE, (IDirect3DSurface9 **)&import, &handle, 0 );
            if (desc->dxgi.version != 1) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->type != D3DRTYPE_SURFACE) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (!(desc->usage & D3DUSAGE_DEPTHSTENCIL)) todo_wine ok_hr( E_INVALIDARG, hr );
            else if ((desc->usage & D3DUSAGE_LOCKABLE)) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );
        }

        if (dxgi)
        {
            LUID adapter_luid = {0};
            hr = IDXGIFactory3_GetSharedResourceAdapterLuid( dxgi, handle, &adapter_luid );
            todo_wine ok_hr( S_OK, hr );
            todo_wine ok( luid_equals( &luid, &adapter_luid ), "got %s\n", debugstr_luid( &adapter_luid ) );
        }

        if (d3d10_imp && GET_API(test) <= 3)
        {
            const struct d3d11_runtime_desc *desc = (struct d3d11_runtime_desc *)runtime_desc;

            hr = ID3D10Device_OpenSharedResource( d3d10_imp, handle, &IID_ID3D10Resource, (void **)&import );
            if (!is_d3dkmt_handle( handle )) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->dxgi.version != 4) ok_hr( E_INVALIDARG, hr );
            else todo_wine ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );
        }

        if (d3d11_imp && GET_API(test) <= 3)
        {
            const struct dxgi_runtime_desc *desc = (struct dxgi_runtime_desc *)runtime_desc;

            hr = ID3D11Device1_OpenSharedResource( d3d11_imp, handle, &IID_ID3D11Resource, (void **)&import );
            if (!is_d3dkmt_handle( handle )) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->version == 1)
            {
                const struct d3d9_runtime_desc *d3d9_desc = (struct d3d9_runtime_desc *)runtime_desc;
                if (d3d9_desc->type != D3DRTYPE_TEXTURE && d3d9_desc->type != D3DRTYPE_SURFACE) ok_hr( E_INVALIDARG, hr );
                else if (d3d9_desc->usage & D3DUSAGE_DEPTHSTENCIL) ok_hr( E_INVALIDARG, hr );
                else ok_hr( S_OK, hr );
            }
            else if (desc->version != 4) ok_hr( E_INVALIDARG, hr );
            else todo_wine ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            hr = ID3D11Device1_OpenSharedResource1( d3d11_imp, handle, &IID_ID3D11Resource, (void **)&import );
            if (is_d3dkmt_handle( handle )) todo_wine ok_hr( E_INVALIDARG, hr );
            else if (desc->version == 4) todo_wine ok_hr( S_OK, hr );
            else if (!desc->format) todo_wine ok_hr( E_INVALIDARG, hr );
            else ok_hr( S_OK, hr );
            if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );

            if (name)
            {
                hr = ID3D11Device1_OpenSharedResourceByName( d3d11_imp, name, GENERIC_ALL, &IID_ID3D11Resource, (void **)&import );
                todo_wine ok_hr( S_OK, hr );
                if (hr == S_OK) ok_ref( 0, IUnknown_Release( import ) );
            }
        }

        if (d3d12_imp && GET_API(test) <= 3)
        {
            hr = ID3D12Device_OpenSharedHandle( d3d12_imp, handle, &IID_ID3D12Resource, (void **)&import );
            ok_hr( S_OK, hr );
            if (import) ok_ref( 0, IUnknown_Release( import ) );

            if (name)
            {
                HANDLE other;

                hr = ID3D12Device_OpenSharedHandleByName( d3d12_imp, name, GENERIC_ALL, &other );
                ok_hr( S_OK, hr );
                hr = ID3D12Device_OpenSharedHandle( d3d12_imp, other, &IID_ID3D12Resource, (void **)&import );
                ok_hr( S_OK, hr );
                CloseHandle( other );

                if (import) ok_ref( 0, IUnknown_Release( import ) );
            }
        }

        if (opengl_imp->broken ? (test == MAKETEST(0, 0, 0) || test == MAKETEST(0, 0, 1)) :
                                 (test == MAKETEST(0, 0, 0) || test == MAKETEST(0, 3, 1)))
        {
            todo_wine win_skip( "Skipping broken OpenGL/Vulkan import tests\n" );
            goto skip_tests;
        }

        if (vulkan_imp)
        {
            struct vulkan_buffer *buf_imp = NULL;
            struct vulkan_image *img_imp = NULL;
            VkResult vr;

            if (is_d3dkmt_handle( handle ))
            {
                switch (GET_DIM(test))
                {
                case 0:
                    vr = import_vulkan_buffer( vulkan_imp, resource_size, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT, &buf_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_buffer( vulkan_imp, buf_imp );
                    vr = import_vulkan_buffer( vulkan_imp, resource_size, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT, &buf_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_buffer( vulkan_imp, buf_imp );
                    break;
                case 1:
                    vr = import_vulkan_image( vulkan_imp, width_1d, 1, array_1d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_1d, 1, array_1d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    break;
                case 2:
                    vr = import_vulkan_image( vulkan_imp, width_2d, height_2d, 1, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_2d, height_2d, 1, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    break;
                case 3:
                    vr = import_vulkan_image( vulkan_imp, width_3d, height_3d, depth_3d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_3d, height_3d, depth_3d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    break;
                }
            }
            else
            {
                switch (GET_DIM(test))
                {
                case 0:
                    vr = import_vulkan_buffer( vulkan_imp, resource_size, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, &buf_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_buffer( vulkan_imp, buf_imp );
                    vr = import_vulkan_buffer( vulkan_imp, resource_size, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT, &buf_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_buffer( vulkan_imp, buf_imp );
                    vr = import_vulkan_buffer( vulkan_imp, resource_size, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, &buf_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_buffer( vulkan_imp, buf_imp );
                    break;
                case 1:
                    vr = import_vulkan_image( vulkan_imp, width_1d, 1, array_1d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_1d, 1, array_1d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_1d, 1, array_1d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    break;
                case 2:
                    vr = import_vulkan_image( vulkan_imp, width_2d, height_2d, 1, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_2d, height_2d, 1, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_2d, height_2d, 1, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    break;
                case 3:
                    vr = import_vulkan_image( vulkan_imp, width_3d, height_3d, depth_3d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_3d, height_3d, depth_3d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    vr = import_vulkan_image( vulkan_imp, width_3d, height_3d, depth_3d, name, handle, VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT, &img_imp );
                    ok_vk( VK_SUCCESS, vr );
                    destroy_vulkan_image( vulkan_imp, img_imp );
                    break;
                }
            }
        }

        if (opengl_imp)
        {
            if (is_d3dkmt_handle( handle ))
            {
                switch (GET_DIM(test))
                {
                case 0:
                    import_opengl_image( opengl_imp, resource_size, 1, 1, 1, name, handle, GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, resource_size, 1, 1, 1, name, handle, GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    break;
                case 1:
                    import_opengl_image( opengl_imp, width_1d, 1, array_1d, 4, name, handle, GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_1d, 1, array_1d, 4, name, handle, GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    break;
                case 2:
                    import_opengl_image( opengl_imp, width_2d, height_2d, 1, 4, name, handle, GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_2d, height_2d, 1, 4, name, handle, GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    break;
                case 3:
                    import_opengl_image( opengl_imp, width_3d, height_3d, depth_3d, 4, name, handle, GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_3d, height_3d, depth_3d, 4, name, handle, GL_HANDLE_TYPE_D3D11_IMAGE_KMT_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    break;
                }
            }
            else
            {
                switch (GET_DIM(test))
                {
                case 0:
                    import_opengl_image( opengl_imp, resource_size, 1, 1, 1, name, handle, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, resource_size, 1, 1, 1, name, handle, GL_HANDLE_TYPE_D3D11_IMAGE_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, resource_size, 1, 1, 1, name, handle, GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, resource_size, 1, 1, 1, name, handle, GL_HANDLE_TYPE_D3D12_RESOURCE_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    break;
                case 1:
                    import_opengl_image( opengl_imp, width_1d, 1, array_1d, 4, name, handle, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_1d, 1, array_1d, 4, name, handle, GL_HANDLE_TYPE_D3D11_IMAGE_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_1d, 1, array_1d, 4, name, handle, GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_1d, 1, array_1d, 4, name, handle, GL_HANDLE_TYPE_D3D12_RESOURCE_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    break;
                case 2:
                    import_opengl_image( opengl_imp, width_2d, height_2d, 1, 4, name, handle, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_2d, height_2d, 1, 4, name, handle, GL_HANDLE_TYPE_D3D11_IMAGE_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_2d, height_2d, 1, 4, name, handle, GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_2d, height_2d, 1, 4, name, handle, GL_HANDLE_TYPE_D3D12_RESOURCE_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    break;
                case 3:
                    import_opengl_image( opengl_imp, width_3d, height_3d, depth_3d, 4, name, handle, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_3d, height_3d, depth_3d, 4, name, handle, GL_HANDLE_TYPE_D3D11_IMAGE_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_3d, height_3d, depth_3d, 4, name, handle, GL_HANDLE_TYPE_D3D12_TILEPOOL_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    import_opengl_image( opengl_imp, width_3d, height_3d, depth_3d, 4, name, handle, GL_HANDLE_TYPE_D3D12_RESOURCE_EXT );
                    ok_x4( glGetError(), ==, 0 );
                    break;
                }
            }
        }

skip_tests:
        if (handle && !is_d3dkmt_handle( handle )) CloseHandle( handle );
        if (export) ok_ref( 0, IUnknown_Release( export ) );
        if (buf) destroy_vulkan_buffer( vulkan_exp, buf );
        if (img) destroy_vulkan_image( vulkan_exp, img );
        winetest_pop_context();
    }

#undef GET_DIM
#undef GET_API
#undef MAKETEST

    if (d3d12_imp) ID3D12Device_Release( d3d12_imp );
    if (d3d12_exp) ok_ref( 0, ID3D12Device_Release( d3d12_exp ) );
    if (d3d11_imp) ok_ref( 0, ID3D11Device1_Release( d3d11_imp ) );
    if (d3d11_exp) ok_ref( 0, ID3D11Device1_Release( d3d11_exp ) );
    if (d3d10_imp) ok_ref( 0, ID3D10Device_Release( d3d10_imp ) );
    if (d3d10_exp) ok_ref( 0, ID3D10Device_Release( d3d10_exp ) );
    ok_ref( 0, IDXGIFactory3_Release( dxgi ) );
    ok_ref( 0, IDirect3DDevice9Ex_Release( d3d9_imp ) );
    ok_ref( 0, IDirect3DDevice9Ex_Release( d3d9_exp ) );
    if (vulkan_imp) destroy_vulkan_device( vulkan_imp );
    if (vulkan_exp) destroy_vulkan_device( vulkan_exp );
    destroy_opengl_device( opengl_imp );
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
