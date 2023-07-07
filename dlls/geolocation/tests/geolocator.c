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

#include "weakreference.h"
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
    IGeolocator *geolocator2;
    IWeakReferenceSource *weak_reference_source;
    IWeakReference *weak_reference;
    IUnknown* unknown;
    void *dummy;
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

    hr = IGeolocator_QueryInterface(geolocator, &IID_IWeakReferenceSource, (void **)&weak_reference_source);
    ok(hr == S_OK && weak_reference_source, "got hr %#lx.\n", hr);

    hr = IWeakReferenceSource_GetWeakReference(weak_reference_source, &weak_reference);
    ok(hr == S_OK && weak_reference, "got hr %#lx.\n", hr);
    IWeakReferenceSource_Release(weak_reference_source);

    hr = IWeakReference_Resolve(weak_reference, &IID_IUnknown, (IInspectable **)&unknown);
    ok(hr == S_OK && unknown, "got hr %#lx.\n", hr);
    hr = IWeakReference_Resolve(weak_reference, &IID_IGeolocator, (IInspectable **)&geolocator2);
    ok(hr == S_OK && geolocator2, "got hr %#lx.\n", hr);
    hr = IWeakReference_Resolve(weak_reference, &IID_IInspectable, &inspectable);
    ok(hr == S_OK && inspectable, "got hr %#lx.\n", hr);
    ok((void *)inspectable == (void *)geolocator, "Interfaces are not the same\n");
    ok((void *)unknown == (void *)geolocator, "Interfaces are not the same\n");
    IUnknown_Release(unknown);
    IGeolocator_Release(geolocator2);
    geolocator2 = 0;
    IInspectable_Release(inspectable);
    inspectable = 0;

    dummy = (void *)0xdeadbeef;
    hr = IWeakReference_Resolve(weak_reference, &IID_IWeakReference, (IInspectable **)&dummy);
    ok(hr == E_NOINTERFACE && !dummy, "got hr %#lx.\n", hr);

    check_interface(weak_reference, &IID_IUnknown);
    check_interface(weak_reference, &IID_IWeakReference);
    hr = IWeakReference_QueryInterface(weak_reference, &IID_IGeolocator, &dummy);
    ok(hr == E_NOINTERFACE && !dummy, "got hr %#lx.\n", hr);
    hr = IWeakReference_QueryInterface(weak_reference, &IID_IAgileObject, &dummy);
    ok(hr == E_NOINTERFACE && !dummy, "got hr %#lx.\n", hr);
    hr = IWeakReference_QueryInterface(weak_reference, &IID_IInspectable, &dummy);
    ok(hr == E_NOINTERFACE && !dummy, "got hr %#lx.\n", hr);

    /* Free geolocator, weak reference should fail to resolve now */
    IGeolocator_Release(geolocator);

    hr = IWeakReference_Resolve(weak_reference, &IID_IGeolocator, (IInspectable **)&geolocator2);
    ok(hr == S_OK && !geolocator2, "got hr %#lx.\n", hr);
    hr = IWeakReference_Resolve(weak_reference, &IID_IWeakReference, (IInspectable **)&dummy);
    ok(hr == S_OK && !dummy, "got hr %#lx.\n", hr);

    IWeakReference_Release(weak_reference);
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
