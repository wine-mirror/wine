/*
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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
#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "roapi.h"
#include "wine/test.h"

#define WIDL_using_Windows_UI_ViewManagement_Core
#define WIDL_using_Windows_Foundation_Collections
#include "windows.ui.viewmanagement.core.h"

#define check_interface(obj, iid, supported) _check_interface(__LINE__, obj, iid, supported)
static void _check_interface(unsigned int line, void *obj, const IID *iid, BOOL supported)
{
    IUnknown *iface = obj, *unknown;
    HRESULT hr;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unknown);
    ok_(__FILE__, line)(hr == S_OK || (!supported && hr == E_NOINTERFACE), "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unknown);
}

static void test_CoreInputViewStatics(void)
{
    IActivationFactory *factory;
    HSTRING str = NULL;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView,
                             wcslen(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView), &str);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    WindowsDeleteString(str);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "got hr %#lx.\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n",
                 wine_dbgstr_w(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView));
        return;
    }

    check_interface(factory, &IID_IUnknown, TRUE);
    check_interface(factory, &IID_IInspectable, TRUE);
    check_interface(factory, &IID_IActivationFactory, TRUE);
    check_interface(factory, &IID_ICoreInputViewStatics, TRUE);
    check_interface(factory, &IID_IAgileObject, FALSE);

    ref = IActivationFactory_Release(factory);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
}

static void test_CoreInputView(void)
{
    ICoreInputViewStatics *core_input_view_statics;
    IVectorView_CoreInputViewOcclusion *occlusions;
    ICoreInputView *core_input_view;
    IActivationFactory *factory;
    HSTRING str = NULL;
    HRESULT hr;
    LONG ref;

    hr = WindowsCreateString(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView,
                             wcslen(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView), &str);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    WindowsDeleteString(str);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "got hr %#lx.\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n",
                 wine_dbgstr_w(RuntimeClass_Windows_UI_ViewManagement_Core_CoreInputView));
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_ICoreInputViewStatics,
                                           (void **)&core_input_view_statics);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ICoreInputViewStatics_GetForCurrentView(core_input_view_statics, &core_input_view);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_interface(core_input_view, &IID_IUnknown, TRUE);
    check_interface(core_input_view, &IID_IInspectable, TRUE);
    check_interface(core_input_view, &IID_IAgileObject, TRUE);
    check_interface(core_input_view, &IID_ICoreInputView, TRUE);
    check_interface(core_input_view, &IID_ICoreInputView2, TRUE);
    check_interface(core_input_view, &IID_ICoreInputView3, TRUE);
    check_interface(core_input_view, &IID_ICoreInputView4, TRUE);

    hr = ICoreInputView_GetCoreInputViewOcclusions(core_input_view, &occlusions);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IVectorView_CoreInputViewOcclusion_Release(occlusions);

    ICoreInputView_Release(core_input_view);

    ref = ICoreInputViewStatics_Release(core_input_view_statics);
    ok(ref == 2, "Got unexpected refcount %ld.\n", ref);
    ref = IActivationFactory_Release(factory);
    ok(ref == 1, "Got unexpected refcount %ld.\n", ref);
}

START_TEST(textinput)
{
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    test_CoreInputViewStatics();
    test_CoreInputView();

    RoUninitialize();
}
