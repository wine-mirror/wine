/*
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#define WIDL_using_Windows_Gaming_Input
#include "windows.gaming.input.h"

#include "wine/test.h"

struct gamepad_event_handler
{
    IEventHandler_Gamepad IEventHandler_Gamepad_iface;
    LONG ref;
};

static inline struct gamepad_event_handler *impl_from_IEventHandler_Gamepad(IEventHandler_Gamepad *iface)
{
    return CONTAINING_RECORD(iface, struct gamepad_event_handler, IEventHandler_Gamepad_iface);
}

static HRESULT STDMETHODCALLTYPE gamepad_event_handler_QueryInterface(
        IEventHandler_Gamepad *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IEventHandler_Gamepad))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    trace("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE gamepad_event_handler_AddRef(
        IEventHandler_Gamepad *iface)
{
    struct gamepad_event_handler *impl = impl_from_IEventHandler_Gamepad(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE gamepad_event_handler_Release(
        IEventHandler_Gamepad *iface)
{
    struct gamepad_event_handler *impl = impl_from_IEventHandler_Gamepad(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE gamepad_event_handler_Invoke(
        IEventHandler_Gamepad *iface, IInspectable *sender, IGamepad *args)
{
    trace("iface %p, sender %p, args %p\n", iface, sender, args);
    return S_OK;
}

static const IEventHandler_GamepadVtbl gamepad_event_handler_vtbl =
{
    gamepad_event_handler_QueryInterface,
    gamepad_event_handler_AddRef,
    gamepad_event_handler_Release,
    /*** IEventHandler<ABI::Windows::Gaming::Input::Gamepad* > methods ***/
    gamepad_event_handler_Invoke,
};

static void test_Gamepad(void)
{
    static const WCHAR *gamepad_name = L"Windows.Gaming.Input.Gamepad";

    struct gamepad_event_handler gamepad_event_handler;
    EventRegistrationToken token;
    IVectorView_Gamepad *gamepads = NULL;
    IActivationFactory *factory = NULL;
    IGamepadStatics *gamepad_statics = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    IGamepad *gamepad;
    BOOLEAN found;
    HSTRING str;
    HRESULT hr;
    UINT32 size;

    gamepad_event_handler.IEventHandler_Gamepad_iface.lpVtbl = &gamepad_event_handler_vtbl;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    hr = WindowsCreateString(gamepad_name, wcslen(gamepad_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(gamepad_name));
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IGamepadStatics, (void **)&gamepad_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IGamepadStatics failed, hr %#lx\n", hr);

    hr = IGamepadStatics_QueryInterface(gamepad_statics, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IGamepadStatics_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);
    ok(tmp_inspectable == inspectable, "IGamepadStatics_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IGamepadStatics_QueryInterface(gamepad_statics, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IGamepadStatics_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);
    ok(tmp_agile_object == agile_object, "IGamepadStatics_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    hr = IGamepadStatics_get_Gamepads(gamepad_statics, &gamepads);
    ok(hr == S_OK, "IGamepadStatics_get_Gamepads failed, hr %#lx\n", hr);

    hr = IVectorView_Gamepad_QueryInterface(gamepads, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IVectorView_Gamepad_QueryInterface failed, hr %#lx\n", hr);
    ok(tmp_inspectable != inspectable, "IVectorView_Gamepad_QueryInterface returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IVectorView_Gamepad_QueryInterface(gamepads, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IVectorView_Gamepad_QueryInterface failed, hr %#lx\n", hr);
    ok(tmp_agile_object != agile_object, "IVectorView_Gamepad_QueryInterface IID_IAgileObject returned agile_object\n");
    IAgileObject_Release(tmp_agile_object);

    size = 0xdeadbeef;
    hr = IVectorView_Gamepad_get_Size(gamepads, &size);
    ok(hr == S_OK, "IVectorView_Gamepad_get_Size failed, hr %#lx\n", hr);
    ok(size != 0xdeadbeef, "IVectorView_Gamepad_get_Size returned %u\n", size);

    gamepad = (IGamepad *)0xdeadbeef;
    hr = IVectorView_Gamepad_GetAt(gamepads, size, &gamepad);
    ok(hr == E_BOUNDS, "IVectorView_Gamepad_GetAt failed, hr %#lx\n", hr);
    ok(gamepad == NULL, "IVectorView_Gamepad_GetAt returned %p\n", gamepad);

    hr = IVectorView_Gamepad_GetMany(gamepads, size, 1, &gamepad, &size);
    ok(hr == E_BOUNDS, "IVectorView_Gamepad_GetMany failed, hr %#lx\n", hr);
    ok(size == 0, "IVectorView_Gamepad_GetMany returned count %u\n", size);

    size = 0xdeadbeef;
    found = TRUE;
    gamepad = (IGamepad *)0xdeadbeef;
    hr = IVectorView_Gamepad_IndexOf(gamepads, gamepad, &size, &found);
    ok(hr == S_OK, "IVectorView_Gamepad_IndexOf failed, hr %#lx\n", hr);
    ok(size == 0 && found == FALSE, "IVectorView_Gamepad_IndexOf returned size %d, found %d\n", size, found);

    IVectorView_Gamepad_Release(gamepads);

    token.value = 0xdeadbeef;
    hr = IGamepadStatics_add_GamepadAdded(gamepad_statics, &gamepad_event_handler.IEventHandler_Gamepad_iface, &token);
    ok(hr == S_OK, "IGamepadStatics_add_GamepadAdded failed, hr %#lx\n", hr);
    ok(token.value != 0xdeadbeef, "IGamepadStatics_add_GamepadAdded returned token %#I64x\n", token.value);

    hr = IGamepadStatics_remove_GamepadAdded(gamepad_statics, token);
    ok(hr == S_OK, "IGamepadStatics_add_GamepadAdded failed, hr %#lx\n", hr);

    token.value = 0xdeadbeef;
    hr = IGamepadStatics_add_GamepadRemoved(gamepad_statics, &gamepad_event_handler.IEventHandler_Gamepad_iface, &token);
    ok(hr == S_OK, "IGamepadStatics_add_GamepadRemoved failed, hr %#lx\n", hr);
    ok(token.value != 0xdeadbeef, "IGamepadStatics_add_GamepadRemoved returned token %#I64x\n", token.value);

    hr = IGamepadStatics_remove_GamepadRemoved(gamepad_statics, token);
    ok(hr == S_OK, "IGamepadStatics_add_GamepadAdded failed, hr %#lx\n", hr);

    hr = IGamepadStatics_add_GamepadAdded(gamepad_statics, NULL, &token);
    ok(hr == E_INVALIDARG, "IGamepadStatics_add_GamepadAdded failed, hr %#lx\n", hr);

    IGamepadStatics_Release(gamepad_statics);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);

    WindowsDeleteString(str);

    RoUninitialize();
}

struct controller_event_handler
{
    IEventHandler_RawGameController IEventHandler_RawGameController_iface;
    LONG ref;
};

static inline struct controller_event_handler *impl_from_IEventHandler_RawGameController(IEventHandler_RawGameController *iface)
{
    return CONTAINING_RECORD(iface, struct controller_event_handler, IEventHandler_RawGameController_iface);
}

static HRESULT STDMETHODCALLTYPE controller_event_handler_QueryInterface(
        IEventHandler_RawGameController *iface, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IEventHandler_RawGameController))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    trace("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE controller_event_handler_AddRef(
        IEventHandler_RawGameController *iface)
{
    struct controller_event_handler *impl = impl_from_IEventHandler_RawGameController(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE controller_event_handler_Release(
        IEventHandler_RawGameController *iface)
{
    struct controller_event_handler *impl = impl_from_IEventHandler_RawGameController(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE controller_event_handler_Invoke(
        IEventHandler_RawGameController *iface, IInspectable *sender, IRawGameController *args)
{
    trace("iface %p, sender %p, args %p\n", iface, sender, args);
    return S_OK;
}

static const IEventHandler_RawGameControllerVtbl controller_event_handler_vtbl =
{
    controller_event_handler_QueryInterface,
    controller_event_handler_AddRef,
    controller_event_handler_Release,
    /*** IEventHandler<ABI::Windows::Gaming::Input::Gamepad* > methods ***/
    controller_event_handler_Invoke,
};

static void test_RawGameController(void)
{
    static const WCHAR *controller_name = L"Windows.Gaming.Input.RawGameController";

    struct controller_event_handler controller_event_handler;
    EventRegistrationToken token;
    IVectorView_RawGameController *controllers = NULL;
    IActivationFactory *factory = NULL;
    IRawGameControllerStatics *controller_statics = NULL;
    IInspectable *inspectable = NULL, *tmp_inspectable = NULL;
    IAgileObject *agile_object = NULL, *tmp_agile_object = NULL;
    IRawGameController *controller;
    BOOLEAN found;
    HSTRING str;
    HRESULT hr;
    UINT32 size;

    controller_event_handler.IEventHandler_RawGameController_iface.lpVtbl = &controller_event_handler_vtbl;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK || hr == S_FALSE, "RoInitialize failed, hr %#lx\n", hr);

    hr = WindowsCreateString(controller_name, wcslen(controller_name), &str);
    ok(hr == S_OK, "WindowsCreateString failed, hr %#lx\n", hr);

    hr = RoGetActivationFactory(str, &IID_IActivationFactory, (void **)&factory);
    ok(hr == S_OK || broken(hr == REGDB_E_CLASSNOTREG), "RoGetActivationFactory failed, hr %#lx\n", hr);
    if (hr == REGDB_E_CLASSNOTREG)
    {
        win_skip("%s runtimeclass not registered, skipping tests.\n", wine_dbgstr_w(controller_name));
        return;
    }

    hr = IActivationFactory_QueryInterface(factory, &IID_IInspectable, (void **)&inspectable);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IAgileObject, (void **)&agile_object);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);

    hr = IActivationFactory_QueryInterface(factory, &IID_IRawGameControllerStatics, (void **)&controller_statics);
    ok(hr == S_OK, "IActivationFactory_QueryInterface IID_IRawGameControllerStatics failed, hr %#lx\n", hr);

    hr = IRawGameControllerStatics_QueryInterface(controller_statics, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IRawGameControllerStatics_QueryInterface IID_IInspectable failed, hr %#lx\n", hr);
    ok(tmp_inspectable == inspectable, "IRawGameControllerStatics_QueryInterface IID_IInspectable returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IRawGameControllerStatics_QueryInterface(controller_statics, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IRawGameControllerStatics_QueryInterface IID_IAgileObject failed, hr %#lx\n", hr);
    ok(tmp_agile_object == agile_object, "IRawGameControllerStatics_QueryInterface IID_IAgileObject returned %p, expected %p\n", tmp_agile_object, agile_object);
    IAgileObject_Release(tmp_agile_object);

    hr = IRawGameControllerStatics_get_RawGameControllers(controller_statics, &controllers);
    ok(hr == S_OK, "IRawGameControllerStatics_get_RawGameControllers failed, hr %#lx\n", hr);

    hr = IVectorView_RawGameController_QueryInterface(controllers, &IID_IInspectable, (void **)&tmp_inspectable);
    ok(hr == S_OK, "IVectorView_RawGameController_QueryInterface failed, hr %#lx\n", hr);
    ok(tmp_inspectable != inspectable, "IVectorView_RawGameController_QueryInterface returned %p, expected %p\n", tmp_inspectable, inspectable);
    IInspectable_Release(tmp_inspectable);

    hr = IVectorView_RawGameController_QueryInterface(controllers, &IID_IAgileObject, (void **)&tmp_agile_object);
    ok(hr == S_OK, "IVectorView_RawGameController_QueryInterface failed, hr %#lx\n", hr);
    ok(tmp_agile_object != agile_object, "IVectorView_RawGameController_QueryInterface IID_IAgileObject returned agile_object\n");
    IAgileObject_Release(tmp_agile_object);

    size = 0xdeadbeef;
    hr = IVectorView_RawGameController_get_Size(controllers, &size);
    ok(hr == S_OK, "IVectorView_RawGameController_get_Size failed, hr %#lx\n", hr);
    ok(size != 0xdeadbeef, "IVectorView_RawGameController_get_Size returned %u\n", size);

    hr = IVectorView_RawGameController_GetMany(controllers, size, 1, &controller, &size);
    ok(hr == E_BOUNDS, "IVectorView_RawGameController_GetMany failed, hr %#lx\n", hr);
    ok(size == 0, "IVectorView_RawGameController_GetMany returned count %u\n", size);

    controller = (IRawGameController *)0xdeadbeef;
    hr = IVectorView_RawGameController_GetAt(controllers, size, &controller);
    ok(hr == E_BOUNDS, "IVectorView_RawGameController_GetAt failed, hr %#lx\n", hr);
    ok(controller == NULL, "IVectorView_RawGameController_GetAt returned %p\n", controller);

    size = 0xdeadbeef;
    found = TRUE;
    controller = (IRawGameController *)0xdeadbeef;
    hr = IVectorView_RawGameController_IndexOf(controllers, controller, &size, &found);
    ok(hr == S_OK, "IVectorView_RawGameController_IndexOf failed, hr %#lx\n", hr);
    ok(size == 0 && found == FALSE, "IVectorView_RawGameController_IndexOf returned size %d, found %d\n", size, found);

    IVectorView_RawGameController_Release(controllers);

    token.value = 0xdeadbeef;
    hr = IRawGameControllerStatics_add_RawGameControllerAdded(controller_statics, &controller_event_handler.IEventHandler_RawGameController_iface, &token);
    ok(hr == S_OK, "IRawGameControllerStatics_add_RawGameControllerAdded failed, hr %#lx\n", hr);
    ok(token.value != 0xdeadbeef, "IRawGameControllerStatics_add_RawGameControllerAdded returned token %#I64x\n", token.value);

    hr = IRawGameControllerStatics_remove_RawGameControllerAdded(controller_statics, token);
    ok(hr == S_OK, "IRawGameControllerStatics_add_RawGameControllerAdded failed, hr %#lx\n", hr);

    token.value = 0xdeadbeef;
    hr = IRawGameControllerStatics_add_RawGameControllerRemoved(controller_statics, &controller_event_handler.IEventHandler_RawGameController_iface, &token);
    ok(hr == S_OK, "IRawGameControllerStatics_add_RawGameControllerRemoved failed, hr %#lx\n", hr);
    ok(token.value != 0xdeadbeef, "IRawGameControllerStatics_add_RawGameControllerRemoved returned token %#I64x\n", token.value);

    hr = IRawGameControllerStatics_remove_RawGameControllerRemoved(controller_statics, token);
    ok(hr == S_OK, "IRawGameControllerStatics_add_RawGameControllerAdded failed, hr %#lx\n", hr);

    hr = IRawGameControllerStatics_add_RawGameControllerAdded(controller_statics, NULL, &token);
    ok(hr == E_INVALIDARG, "IRawGameControllerStatics_add_RawGameControllerAdded failed, hr %#lx\n", hr);

    IRawGameControllerStatics_Release(controller_statics);

    IAgileObject_Release(agile_object);
    IInspectable_Release(inspectable);
    IActivationFactory_Release(factory);

    WindowsDeleteString(str);

    RoUninitialize();
}

START_TEST(input)
{
    test_Gamepad();
    test_RawGameController();
}
