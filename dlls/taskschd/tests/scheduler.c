/*
 * Copyright 2014 Dmitry Timoshkov
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

#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "objbase.h"
#include "taskschd.h"

#include <wine/test.h>

static void test_Connect(void)
{
    static const WCHAR deadbeefW[] = { 'd','e','a','d','b','e','e','f',0 };
    WCHAR comp_name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len;
    HRESULT hr;
    BSTR bstr;
    VARIANT v_null, v_comp;
    VARIANT_BOOL vbool;
    ITaskService *service;

    hr = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskService, (void **)&service);
    if (hr != S_OK)
    {
        win_skip("CoCreateInstance(CLSID_TaskScheduler) error %#x\n", hr);
        return;
    }

    hr = ITaskService_get_Connected(service, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#x\n", hr);
    ok(vbool == VARIANT_FALSE, "expected VARIANT_FALSE, got %d\n", vbool);

    hr = ITaskService_get_TargetServer(service, NULL);
    ok(hr == E_POINTER, "expected E_POINTER, got %#x\n", hr);

    hr = ITaskService_get_TargetServer(service, &bstr);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED), "expected ERROR_ONLY_IF_CONNECTED, got %#x\n", hr);

    /* Win7 doesn't support UNC \\ prefix, but according to a user
     * comment on MSDN Win8 supports both ways.
     */
    len = sizeof(comp_name)/sizeof(comp_name[0]);
    GetComputerNameW(comp_name, &len);

    V_VT(&v_null) = VT_NULL;

    V_VT(&v_comp) = VT_BSTR;
    V_BSTR(&v_comp) = SysAllocString(comp_name);

    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == S_OK || hr == E_ACCESSDENIED /* not an administrator */, "Connect error %#x\n", hr);

    SysFreeString(V_BSTR(&v_comp));
    V_BSTR(&v_comp) = SysAllocString(deadbeefW);

    hr = ITaskService_Connect(service, v_comp, v_null, v_null, v_null);
    ok(hr == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH), "expected ERROR_BAD_NETPATH, got %#x\n", hr);

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#x\n", hr);
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    hr = ITaskService_Connect(service, v_null, v_null, v_null, v_null);
    ok(hr == S_OK, "Connect error %#x\n", hr);

    vbool = 0xdead;
    hr = ITaskService_get_Connected(service, &vbool);
    ok(hr == S_OK, "get_Connected error %#x\n", hr);
    ok(vbool == VARIANT_TRUE, "expected VARIANT_TRUE, got %d\n", vbool);

    hr = ITaskService_get_TargetServer(service, &bstr);
    ok(hr == S_OK, "get_TargetServer error %#x\n", hr);
    if (hr == S_OK)
    {
        ok(!lstrcmpW(comp_name, bstr), "compname %s != server name %s\n", wine_dbgstr_w(comp_name), wine_dbgstr_w(bstr));
        SysFreeString(bstr);
    }
    SysFreeString(V_BSTR(&v_comp));
}

START_TEST(scheduler)
{
    OleInitialize(NULL);

    test_Connect();

    OleUninitialize();
}
