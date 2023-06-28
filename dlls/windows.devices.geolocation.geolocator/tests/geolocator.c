/*
 * Copyright 2023 Fabian Maurer
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
#include "winerror.h"
#include "winstring.h"

#include "initguid.h"
#include "roapi.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Devices_Geolocation
#include "windows.devices.geolocation.h"

#include "wine/test.h"

#define check_interface(obj, iid) check_interface_(__LINE__, obj, iid)
static void check_interface_(unsigned int line, void *obj, const IID *iid)
{
    IUnknown *iface = obj;
    IUnknown *unk;
    HRESULT hr;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == S_OK, "got hr %#lx.\n", hr);
    if (hr == S_OK)
        IUnknown_Release(unk);
}

void test_basic(void)
{
    static const WCHAR *geolocator_name = L"Windows.Devices.Geolocation.Geolocator";
    IActivationFactory *factory;
    IInspectable *inspectable;
    IGeolocator *geolocator;
    HSTRING str;
    HRESULT hr;

    hr = WindowsCreateString(geolocator_name, wcslen(geolocator_name), &str);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    WindowsDeleteString(str);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "got hr %#lx.\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
       win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(geolocator_name));
       return;
    }

    /* Don't check IID_IAgileObject since it's ignored on win 8 */
    check_interface(factory, &IID_IUnknown);
    check_interface(factory, &IID_IInspectable);
    check_interface(factory, &IID_IActivationFactory);

    hr = IActivationFactory_ActivateInstance(factory, &inspectable);
    ok(hr == S_OK && inspectable, "got hr %#lx.\n", hr);

    check_interface(inspectable, &IID_IUnknown);
    check_interface(inspectable, &IID_IInspectable);
    check_interface(inspectable, &IID_IAgileObject);

    hr = IInspectable_QueryInterface(inspectable, &IID_IGeolocator, (void **)&geolocator);
    ok(hr == S_OK && geolocator, "got hr %#lx.\n", hr);
    ok((void *)inspectable == (void *)geolocator, "Interfaces are not the same\n");

    IInspectable_Release(inspectable);
    inspectable = 0;

    IGeolocator_Release(geolocator);
    IActivationFactory_Release(factory);
}

START_TEST(geolocator)
{
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    test_basic();

    RoUninitialize();
}
