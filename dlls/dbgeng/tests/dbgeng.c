/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"

#include "initguid.h"
#include "dbgeng.h"

#include "wine/test.h"

static void test_engine_options(void)
{
    IDebugControl *control;
    ULONG options;
    HRESULT hr;

    hr = DebugCreate(&IID_IDebugControl, (void **)&control);
    ok(hr == S_OK, "Failed to create engine object, hr %#lx.\n", hr);

    options = 0xf;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#lx.\n", hr);
    ok(options == 0, "Unexpected options %#lx.\n", options);

    hr = control->lpVtbl->AddEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to add engine options, hr %#lx.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#lx.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#lx.\n", options);

    hr = control->lpVtbl->AddEngineOptions(control, 0x01000000);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#lx.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#lx.\n", options);

    hr = control->lpVtbl->RemoveEngineOptions(control, 0x01000000);
    ok(hr == S_OK, "Failed to remove options, hr %#lx.\n", hr);

    hr = control->lpVtbl->AddEngineOptions(control, DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION);
    ok(hr == S_OK, "Failed to add engine options, hr %#lx.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#lx.\n", hr);
    ok(options == (DEBUG_ENGOPT_INITIAL_BREAK | DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION),
            "Unexpected options %#lx.\n", options);

    hr = control->lpVtbl->RemoveEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to remove options, hr %#lx.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#lx.\n", hr);
    ok(options == DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION, "Unexpected options %#lx.\n", options);

    hr = control->lpVtbl->SetEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to set options, hr %#lx.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#lx.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#lx.\n", options);

    hr = control->lpVtbl->SetEngineOptions(control, 0x01000000);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = control->lpVtbl->SetEngineOptions(control, 0x01000000 | DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#lx.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#lx.\n", options);

    control->lpVtbl->Release(control);
}

static HRESULT WINAPI event_callbacks_QueryInterface(IDebugEventCallbacks *iface, REFIID riid, void **out)
{
    if (IsEqualIID(riid, &IID_IDebugEventCallbacks) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *out = iface;
        iface->lpVtbl->AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI event_callbacks_AddRef(IDebugEventCallbacks *iface)
{
    return 2;
}

static ULONG WINAPI event_callbacks_Release(IDebugEventCallbacks *iface)
{
    return 1;
}

static HRESULT WINAPI event_callbacks_GetInterestMask(IDebugEventCallbacks *iface, ULONG *mask)
{
    *mask = ~0u;
    return S_OK;
}

static HRESULT WINAPI event_callbacks_Breakpoint(IDebugEventCallbacks *iface, PDEBUG_BREAKPOINT breakpoint)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_Exception(IDebugEventCallbacks *iface, EXCEPTION_RECORD64 *exception,
        ULONG first_chance)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_CreateThread(IDebugEventCallbacks *iface, ULONG64 handle, ULONG64 data_offset,
        ULONG64 start_offset)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_ExitThread(IDebugEventCallbacks *iface, ULONG exit_code)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_CreateProcess(IDebugEventCallbacks *iface, ULONG64 image_handle, ULONG64 handle,
        ULONG64 base_offset, ULONG module_size, const char *module_name, const char *image_name, ULONG checksum,
        ULONG timedatestamp, ULONG64 initial_thread_handle, ULONG64 thread_data_offset, ULONG64 start_offset)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_ExitProcess(IDebugEventCallbacks *iface, ULONG exit_code)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_LoadModule(IDebugEventCallbacks *iface, ULONG64 image_handle,
        ULONG64 base_offset, ULONG module_size, const char *module_name, const char *image_name, ULONG checksum,
        ULONG timedatestamp)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_UnloadModule(IDebugEventCallbacks *iface, const char *image_basename,
        ULONG64 base_offset)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_SystemError(IDebugEventCallbacks *iface, ULONG error, ULONG level)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_SessionStatus(IDebugEventCallbacks *iface, ULONG status)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_ChangeDebuggeeState(IDebugEventCallbacks *iface, ULONG flags, ULONG64 argument)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_ChangeEngineState(IDebugEventCallbacks *iface, ULONG flags, ULONG64 argument)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI event_callbacks_ChangeSymbolState(IDebugEventCallbacks *iface, ULONG flags, ULONG64 argument)

{
    return E_NOTIMPL;
}

static const IDebugEventCallbacksVtbl event_callbacks_vtbl =
{
    event_callbacks_QueryInterface,
    event_callbacks_AddRef,
    event_callbacks_Release,
    event_callbacks_GetInterestMask,
    event_callbacks_Breakpoint,
    event_callbacks_Exception,
    event_callbacks_CreateThread,
    event_callbacks_ExitThread,
    event_callbacks_CreateProcess,
    event_callbacks_ExitProcess,
    event_callbacks_LoadModule,
    event_callbacks_UnloadModule,
    event_callbacks_SystemError,
    event_callbacks_SessionStatus,
    event_callbacks_ChangeDebuggeeState,
    event_callbacks_ChangeEngineState,
    event_callbacks_ChangeSymbolState,
};

static BOOL create_target_process(const char *event_name, PROCESS_INFORMATION *info)
{
    static const char *event_target_ready_name = "dbgeng_test_target_ready_event";
    char path_name[MAX_PATH];
    STARTUPINFOA startup;
    HANDLE ready_event;
    char **argv;
    BOOL ret;

    ready_event = CreateEventA(NULL, FALSE, FALSE, event_target_ready_name);
    ok(ready_event != NULL, "Failed to create event.\n");

    winetest_get_mainargs(&argv);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    sprintf(path_name, "%s dbgeng target %s %s", argv[0], event_name, event_target_ready_name);
    ret = CreateProcessA(NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, info);
    if (ret)
    {
        WaitForSingleObject(ready_event, INFINITE);
    }

    CloseHandle(ready_event);

    return ret;
}

static void test_attach(void)
{
    static const char *event_name = "dbgeng_test_event";
    IDebugEventCallbacks event_callbacks = { &event_callbacks_vtbl };
    PROCESS_INFORMATION info;
    IDebugControl *control;
    IDebugClient *client;
    BOOL is_debugged;
    HANDLE event;
    HRESULT hr;
    BOOL ret;

    hr = DebugCreate(&IID_IDebugClient, (void **)&client);
    ok(hr == S_OK, "Failed to create engine object, hr %#lx.\n", hr);

    hr = client->lpVtbl->QueryInterface(client, &IID_IDebugControl, (void **)&control);
    ok(hr == S_OK, "Failed to get interface pointer, hr %#lx.\n", hr);

    hr = client->lpVtbl->SetEventCallbacks(client, &event_callbacks);
    ok(hr == S_OK, "Failed to set event callbacks, hr %#lx.\n", hr);

    event = CreateEventA(NULL, FALSE, FALSE, event_name);
    ok(event != NULL, "Failed to create event.\n");

    ret = create_target_process(event_name, &info);
    ok(ret, "Failed to create target process.\n");

    is_debugged = TRUE;
    CheckRemoteDebuggerPresent(info.hProcess, &is_debugged);
    ok(!is_debugged, "Unexpected mode.\n");

    /* Non-invasive mode. */
    hr = client->lpVtbl->AttachProcess(client, 0, info.dwProcessId, DEBUG_ATTACH_NONINVASIVE);
    ok(hr == S_OK, "Failed to attach to process, hr %#lx.\n", hr);

    is_debugged = TRUE;
    ret = CheckRemoteDebuggerPresent(info.hProcess, &is_debugged);
    ok(ret, "Failed to check target status.\n");
    ok(!is_debugged, "Unexpected mode.\n");

    hr = control->lpVtbl->WaitForEvent(control, 0, INFINITE);
    ok(hr == S_OK, "Waiting for event failed, hr %#lx.\n", hr);

    is_debugged = TRUE;
    ret = CheckRemoteDebuggerPresent(info.hProcess, &is_debugged);
    ok(ret, "Failed to check target status.\n");
    ok(!is_debugged, "Unexpected mode.\n");

    hr = client->lpVtbl->DetachProcesses(client);
    ok(hr == S_OK, "Failed to detach, hr %#lx.\n", hr);

    hr = client->lpVtbl->EndSession(client, DEBUG_END_ACTIVE_DETACH);
    todo_wine
    ok(hr == S_OK, "Failed to end session, hr %#lx.\n", hr);

    SetEvent(event);

    wait_child_process(info.hProcess);

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    CloseHandle(event);

    client->lpVtbl->Release(client);
    control->lpVtbl->Release(control);
}

static void test_module_information(void)
{
    static const char *event_name = "dbgeng_test_event";
    ULONG loaded, unloaded, index, length;
    DEBUG_MODULE_PARAMETERS params[2];
    IDebugDataSpaces *dataspaces;
    PROCESS_INFORMATION info;
    IDebugSymbols2 *symbols;
    IDebugControl *control;
    ULONG64 bases[2], base;
    char buffer[MAX_PATH];
    IDebugClient *client;
    HANDLE event;
    HRESULT hr;
    BOOL ret;

    hr = DebugCreate(&IID_IDebugClient, (void **)&client);
    ok(hr == S_OK, "Failed to create engine object, hr %#lx.\n", hr);

    hr = client->lpVtbl->QueryInterface(client, &IID_IDebugControl, (void **)&control);
    ok(hr == S_OK, "Failed to get interface pointer, hr %#lx.\n", hr);

    hr = client->lpVtbl->QueryInterface(client, &IID_IDebugSymbols2, (void **)&symbols);
    ok(hr == S_OK, "Failed to get interface pointer, hr %#lx.\n", hr);

    hr = client->lpVtbl->QueryInterface(client, &IID_IDebugDataSpaces, (void **)&dataspaces);
    ok(hr == S_OK, "Failed to get interface pointer, hr %#lx.\n", hr);

    hr = control->lpVtbl->IsPointer64Bit(control);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    event = CreateEventA(NULL, FALSE, FALSE, event_name);
    ok(event != NULL, "Failed to create event.\n");

    ret = create_target_process(event_name, &info);
    ok(ret, "Failed to create target process.\n");

    hr = control->lpVtbl->SetEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to set engine options, hr %#lx.\n", hr);

    hr = client->lpVtbl->AttachProcess(client, 0, info.dwProcessId, DEBUG_ATTACH_NONINVASIVE);
    ok(hr == S_OK, "Failed to attach to process, hr %#lx.\n", hr);

    hr = control->lpVtbl->IsPointer64Bit(control);
    ok(hr == E_UNEXPECTED, "Unexpected hr %#lx.\n", hr);

    hr = control->lpVtbl->WaitForEvent(control, 0, INFINITE);
    ok(hr == S_OK, "Waiting for event failed, hr %#lx.\n", hr);

    hr = control->lpVtbl->IsPointer64Bit(control);
    ok(SUCCEEDED(hr), "Failed to get pointer length, hr %#lx.\n", hr);

    /* Number of modules. */
    hr = symbols->lpVtbl->GetNumberModules(symbols, &loaded, &unloaded);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(loaded > 0, "Unexpected module count %lu.\n", loaded);

    /* Module base. */
    hr = symbols->lpVtbl->GetModuleByIndex(symbols, loaded, &base);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    base = 0;
    hr = symbols->lpVtbl->GetModuleByIndex(symbols, 0, &base);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!base, "Unexpected module base.\n");

    hr = symbols->lpVtbl->GetModuleByOffset(symbols, 0, 0, &index, &base);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    hr = symbols->lpVtbl->GetModuleByOffset(symbols, base, 0, &index, &base);
    ok(hr == S_OK, "Failed to get module, hr %#lx.\n", hr);

    hr = symbols->lpVtbl->GetModuleByOffset(symbols, base, 0, NULL, NULL);
    ok(hr == S_OK, "Failed to get module, hr %#lx.\n", hr);

    hr = symbols->lpVtbl->GetModuleByOffset(symbols, base + 1, 0, NULL, NULL);
    ok(hr == S_OK, "Failed to get module, hr %#lx.\n", hr);

    hr = symbols->lpVtbl->GetModuleByOffset(symbols, base, loaded, NULL, NULL);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    /* Parameters. */
    base = 0;
    hr = symbols->lpVtbl->GetModuleByIndex(symbols, 0, &base);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!base, "Unexpected module base.\n");

    hr = symbols->lpVtbl->GetModuleParameters(symbols, 1, NULL, 0, params);
    ok(hr == S_OK, "Failed to get module parameters, hr %#lx.\n", hr);
    ok(params[0].Base == base, "Unexpected module base.\n");

    hr = symbols->lpVtbl->GetModuleParameters(symbols, 1, &base, 100, params);
    ok(hr == S_OK, "Failed to get module parameters, hr %#lx.\n", hr);
    ok(params[0].Base == base, "Unexpected module base.\n");

    bases[0] = base + 1;
    bases[1] = base;
    hr = symbols->lpVtbl->GetModuleParameters(symbols, 2, bases, 0, params);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* XP */, "Failed to get module parameters, hr %#lx.\n", hr);
    ok(params[0].Base == DEBUG_INVALID_OFFSET, "Unexpected module base.\n");
    ok(params[0].Size == 0, "Unexpected module size.\n");
    ok(params[1].Base == base, "Unexpected module base.\n");
    ok(params[1].Size != 0, "Unexpected module size.\n");

    hr = symbols->lpVtbl->GetModuleParameters(symbols, 1, bases, 0, params);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* XP */, "Failed to get module parameters, hr %#lx.\n", hr);

    hr = symbols->lpVtbl->GetModuleParameters(symbols, 1, bases, loaded, params);
    ok(hr == S_OK || broken(hr == E_NOINTERFACE) /* XP */, "Failed to get module parameters, hr %#lx.\n", hr);

    hr = symbols->lpVtbl->GetModuleParameters(symbols, 1, NULL, loaded, params);
    ok(FAILED(hr), "Unexpected hr %#lx.\n", hr);

    /* Image name. */
    hr = symbols->lpVtbl->GetModuleNameString(symbols, DEBUG_MODNAME_IMAGE, 0, 0, buffer, sizeof(buffer), &length);
    ok(hr == S_OK, "Failed to get image name, hr %#lx.\n", hr);
    ok(strlen(buffer) + 1 == length, "Unexpected length.\n");

    hr = symbols->lpVtbl->GetModuleNameString(symbols, DEBUG_MODNAME_IMAGE, 0, 0, NULL, sizeof(buffer), &length);
    ok(hr == S_OK, "Failed to get image name, hr %#lx.\n", hr);
    ok(length > 0, "Unexpected length.\n");

    hr = symbols->lpVtbl->GetModuleNameString(symbols, DEBUG_MODNAME_IMAGE, DEBUG_ANY_ID, base, buffer, sizeof(buffer),
            &length);
    ok(hr == S_OK, "Failed to get image name, hr %#lx.\n", hr);
    ok(strlen(buffer) + 1 == length, "Unexpected length.\n");

    hr = symbols->lpVtbl->GetModuleNameString(symbols, DEBUG_MODNAME_IMAGE, 0, 0, buffer, length - 1, &length);
    ok(hr == S_FALSE, "Failed to get image name, hr %#lx.\n", hr);
    ok(strlen(buffer) + 2 == length, "Unexpected length %lu.\n", length);

    hr = symbols->lpVtbl->GetModuleNameString(symbols, DEBUG_MODNAME_IMAGE, 0, 0, NULL, length - 1, NULL);
    ok(hr == S_FALSE, "Failed to get image name, hr %#lx.\n", hr);

    /* Read memory. */
    base = 0;
    hr = symbols->lpVtbl->GetModuleByIndex(symbols, 0, &base);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!base, "Unexpected module base.\n");

    hr = dataspaces->lpVtbl->ReadVirtual(dataspaces, base, buffer, sizeof(buffer), &length);
    ok(hr == S_OK, "Failed to read process memory, hr %#lx.\n", hr);
    ok(length == sizeof(buffer), "Unexpected length %lu.\n", length);
    ok(buffer[0] == 'M' && buffer[1] == 'Z', "Unexpected contents.\n");

    memset(buffer, 0, sizeof(buffer));
    hr = dataspaces->lpVtbl->ReadVirtual(dataspaces, base, buffer, sizeof(buffer), NULL);
    ok(hr == S_OK, "Failed to read process memory, hr %#lx.\n", hr);
    ok(buffer[0] == 'M' && buffer[1] == 'Z', "Unexpected contents.\n");

    hr = client->lpVtbl->DetachProcesses(client);
    ok(hr == S_OK, "Failed to detach, hr %#lx.\n", hr);

    SetEvent(event);
    wait_child_process(info.hProcess);

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
    CloseHandle(event);

    client->lpVtbl->Release(client);
    control->lpVtbl->Release(control);
    symbols->lpVtbl->Release(symbols);
    dataspaces->lpVtbl->Release(dataspaces);
}

static void target_proc(const char *event_name, const char *event_ready_name)
{
    HANDLE terminate_event, ready_event;

    terminate_event = OpenEventA(SYNCHRONIZE, FALSE, event_name);
    ok(terminate_event != NULL, "Failed to open event handle.\n");

    ready_event = OpenEventA(EVENT_MODIFY_STATE, FALSE, event_ready_name);
    ok(ready_event != NULL, "Failed to open event handle.\n");

    SetEvent(ready_event);

    for (;;)
    {
        if (WaitForSingleObject(terminate_event, 100) == WAIT_OBJECT_0)
            break;
    }

    CloseHandle(terminate_event);
    CloseHandle(ready_event);
}

START_TEST(dbgeng)
{
    char **argv;
    int argc;

    argc = winetest_get_mainargs(&argv);

    if (argc > 4 && !strcmp(argv[2], "target"))
    {
        target_proc(argv[3], argv[4]);
        return;
    }

    test_engine_options();
    test_attach();
    test_module_information();
}
