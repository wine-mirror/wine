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
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Storage_Streams
#include "windows.storage.streams.h"

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

START_TEST(storage)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_RandomAccessStreamReference();

    RoUninitialize();
}
