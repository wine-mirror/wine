/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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
#define WIDL_using_Windows_Perception_Spatial_Surfaces
#include "windows.perception.spatial.surfaces.h"
#define WIDL_using_Windows_Graphics_Holographic
#include "windows.graphics.holographic.h"
#include "holographicspaceinterop.h"

#include "wine/test.h"

#define check_interface( obj, iid, can_be_broken ) check_interface_( __LINE__, obj, iid, can_be_broken )
static void check_interface_( unsigned int line, void *obj, const IID *iid, BOOL can_be_broken )
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface( iface, iid, (void **)&unk );
    ok_(__FILE__, line)( hr == S_OK || broken( can_be_broken && hr == E_NOINTERFACE ), "got hr %#lx.\n", hr );
    if (SUCCEEDED(hr))
        IUnknown_Release( unk );
}

static void test_ObserverStatics(void)
{
    static const WCHAR *observer_statics_name = L"Windows.Perception.Spatial.Surfaces.SpatialSurfaceObserver";
    ISpatialSurfaceObserverStatics2 *observer_statics2;
    IActivationFactory *factory;
    BOOLEAN value;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( observer_statics_name, wcslen( observer_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( observer_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, FALSE );
    check_interface( factory, &IID_IInspectable, FALSE );
    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_ISpatialSurfaceObserverStatics, FALSE );

    hr = IActivationFactory_QueryInterface( factory, &IID_ISpatialSurfaceObserverStatics2, (void **)&observer_statics2 );
    if (hr == E_NOINTERFACE) /* win1607 */
    {
        win_skip( "ISpatialSurfaceObserverStatics2 is not supported, skipping tests.\n" );
        goto done;
    }

    ok( hr == S_OK, "got hr %#lx.\n", hr );

    value = TRUE;
    hr = ISpatialSurfaceObserverStatics2_IsSupported( observer_statics2, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( !value, "got %d.\n", value );

    ref = ISpatialSurfaceObserverStatics2_Release( observer_statics2 );
    ok( ref == 2, "got ref %ld.\n", ref );
done:
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

static void test_HolographicSpaceStatics(void)
{
    static const WCHAR *holographicspace_statics_name = L"Windows.Graphics.Holographic.HolographicSpace";
    IHolographicSpaceStatics2 *holographicspace_statics2;
    IHolographicSpaceStatics3 *holographicspace_statics3;
    IActivationFactory *factory;
    BOOLEAN value;
    HSTRING str;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString( holographicspace_statics_name, wcslen( holographicspace_statics_name ), &str );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = RoGetActivationFactory( str, &IID_IActivationFactory, (void **)&factory );
    WindowsDeleteString( str );
    ok( hr == S_OK || broken( hr == REGDB_E_CLASSNOTREG ), "got hr %#lx.\n", hr );
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip( "%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w( holographicspace_statics_name ) );
        return;
    }

    check_interface( factory, &IID_IUnknown, FALSE );
    check_interface( factory, &IID_IInspectable, FALSE );
    check_interface( factory, &IID_IAgileObject, FALSE );
    check_interface( factory, &IID_IHolographicSpaceInterop, TRUE /* broken on Testbot Win1607 */ );

    hr = IActivationFactory_QueryInterface( factory, &IID_IHolographicSpaceStatics2, (void **)&holographicspace_statics2 );
    if (hr == E_NOINTERFACE) /* win1607 */
    {
        win_skip( "IHolographicSpaceStatics2 is not supported, skipping tests.\n" );
        goto done;
    }

    ok( hr == S_OK, "got hr %#lx.\n", hr );

    value = 2;
    hr = IHolographicSpaceStatics2_get_IsSupported( holographicspace_statics2, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    todo_wine ok( value == TRUE, "got %d.\n", value );

    value = 2;
    hr = IHolographicSpaceStatics2_get_IsAvailable( holographicspace_statics2, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got %d.\n", value );

    ref = IHolographicSpaceStatics2_Release( holographicspace_statics2 );
    ok( ref == 2, "got ref %ld.\n", ref );

    hr = IActivationFactory_QueryInterface( factory, &IID_IHolographicSpaceStatics3, (void **)&holographicspace_statics3 );
    if (hr == E_NOINTERFACE) /* win1703 */
    {
        win_skip( "IHolographicSpaceStatics3 is not supported, skipping tests.\n" );
        goto done;
    }

    ok( hr == S_OK, "got hr %#lx.\n", hr );

    value = 2;
    hr = IHolographicSpaceStatics3_get_IsConfigured( holographicspace_statics3, &value );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( value == FALSE, "got %d.\n", value );

    ref = IHolographicSpaceStatics3_Release( holographicspace_statics3 );
    ok( ref == 2, "got ref %ld.\n", ref );
done:
    ref = IActivationFactory_Release( factory );
    ok( ref == 1, "got ref %ld.\n", ref );
}

START_TEST(perception)
{
    HRESULT hr;

    hr = RoInitialize( RO_INIT_MULTITHREADED );
    ok( hr == S_OK, "RoInitialize failed, hr %#lx\n", hr );

    test_ObserverStatics();
    test_HolographicSpaceStatics();

    RoUninitialize();
}
