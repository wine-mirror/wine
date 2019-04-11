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
    ok(hr == S_OK, "Failed to create engine object, hr %#x.\n", hr);

    options = 0xf;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == 0, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->AddEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to add engine options, hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->AddEngineOptions(control, 0x00800000);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->RemoveEngineOptions(control, 0x00800000);
    ok(hr == S_OK, "Failed to remove options, hr %#x.\n", hr);

    hr = control->lpVtbl->AddEngineOptions(control, DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION);
    ok(hr == S_OK, "Failed to add engine options, hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == (DEBUG_ENGOPT_INITIAL_BREAK | DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION),
            "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->RemoveEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to remove options, hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->SetEngineOptions(control, DEBUG_ENGOPT_INITIAL_BREAK);
    ok(hr == S_OK, "Failed to set options, hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#x.\n", options);

    hr = control->lpVtbl->SetEngineOptions(control, 0x00800000);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    hr = control->lpVtbl->SetEngineOptions(control, 0x00800000 | DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION);
    ok(hr == E_INVALIDARG, "Unexpected hr %#x.\n", hr);

    options = 0;
    hr = control->lpVtbl->GetEngineOptions(control, &options);
    ok(hr == S_OK, "Failed to get engine options, hr %#x.\n", hr);
    ok(options == DEBUG_ENGOPT_INITIAL_BREAK, "Unexpected options %#x.\n", options);

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

static const char *event_name = "dbgeng_test_event";

static void test_attach(void)
{
    IDebugEventCallbacks event_callbacks = { &event_callbacks_vtbl };
    PROCESS_INFORMATION info;
    char path_name[MAX_PATH];
    IDebugControl *control;
    IDebugClient *client;
    STARTUPINFOA startup;
    BOOL is_debugged;
    HANDLE event;
    char **argv;
    HRESULT hr;
    BOOL ret;

    hr = DebugCreate(&IID_IDebugClient, (void **)&client);
    ok(hr == S_OK, "Failed to create engine object, hr %#x.\n", hr);

    hr = client->lpVtbl->QueryInterface(client, &IID_IDebugControl, (void **)&control);
    ok(hr == S_OK, "Failed to get interface pointer, hr %#x.\n", hr);

    hr = client->lpVtbl->SetEventCallbacks(client, &event_callbacks);
    ok(hr == S_OK, "Failed to set event callbacks, hr %#x.\n", hr);

    event = CreateEventA(NULL, FALSE, FALSE, event_name);
    ok(event != NULL, "Failed to create event.\n");

    winetest_get_mainargs(&argv);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    sprintf(path_name, "%s dbgeng target", argv[0]);
    ret = CreateProcessA(NULL, path_name, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info),
    ok(ret, "Failed to create target process.\n");

    is_debugged = TRUE;
    CheckRemoteDebuggerPresent(info.hProcess, &is_debugged);
    ok(!is_debugged, "Unexpected mode.\n");

    /* Non-invasive mode. */
    hr = client->lpVtbl->AttachProcess(client, 0, info.dwProcessId, DEBUG_ATTACH_NONINVASIVE);
    ok(hr == S_OK, "Failed to attach to process, hr %#x.\n", hr);

    is_debugged = TRUE;
    ret = CheckRemoteDebuggerPresent(info.hProcess, &is_debugged);
    ok(ret, "Failed to check target status.\n");
    ok(!is_debugged, "Unexpected mode.\n");

    hr = control->lpVtbl->WaitForEvent(control, 0, INFINITE);
todo_wine
    ok(hr == S_OK, "Waiting for event failed, hr %#x.\n", hr);

    is_debugged = TRUE;
    ret = CheckRemoteDebuggerPresent(info.hProcess, &is_debugged);
    ok(ret, "Failed to check target status.\n");
    ok(!is_debugged, "Unexpected mode.\n");

    hr = client->lpVtbl->DetachProcesses(client);
todo_wine
    ok(hr == S_OK, "Failed to detach, hr %#x.\n", hr);

    hr = client->lpVtbl->EndSession(client, DEBUG_END_ACTIVE_DETACH);
todo_wine
    ok(hr == S_OK, "Failed to end session, hr %#x.\n", hr);

    SetEvent(event);

    winetest_wait_child_process(info.hProcess);

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    CloseHandle(event);

    client->lpVtbl->Release(client);
    control->lpVtbl->Release(control);
}

static void target_proc(void)
{
    HANDLE event = OpenEventA(SYNCHRONIZE, FALSE, event_name);

    ok(event != NULL, "Failed to open event handle.\n");

    for (;;)
    {
        if (WaitForSingleObject(event, 100) == WAIT_OBJECT_0)
            break;
    }

    CloseHandle(event);
}

START_TEST(dbgeng)
{
    char **argv;
    int argc;

    argc = winetest_get_mainargs(&argv);

    if (argc >= 3 && !strcmp(argv[2], "target"))
    {
        target_proc();
        return;
    }

    test_engine_options();
    test_attach();
}
