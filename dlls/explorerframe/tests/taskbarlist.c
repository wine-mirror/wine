/*
 * Unit tests for ITaskbarList interface
 *
 * Copyright 2021 Zhiyi Zhang for CodeWeavers
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
#include <shobjidl.h>
#include <wine/test.h>

static void test_ITaskbarList(void)
{
    ITaskbarList *taskbarlist;
    ULONG ref_count;
    HRESULT hr;
    HWND hwnd;

    hwnd = CreateWindowA(WC_STATICA, "test", WS_POPUP | WS_VISIBLE, 0, 0, 50, 50, 0, 0, 0, NULL);
    ok(hwnd != NULL, "Failed to create a test window, error %lu.\n", GetLastError());

    hr = CoCreateInstance((const CLSID *)&CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
                          (const IID *)&IID_ITaskbarList, (void **)&taskbarlist);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ref_count = ITaskbarList_AddRef(taskbarlist);
    ok(ref_count == 2, "Got unexpected reference count %lu.\n", ref_count);
    ref_count = ITaskbarList_Release(taskbarlist);
    ok(ref_count == 1, "Got unexpected reference count %lu.\n", ref_count);

    /* Test calling methods before calling ITaskbarList::HrInit() */
    hr = ITaskbarList_AddTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_SetActiveAlt(taskbarlist, hwnd);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_ActivateTab(taskbarlist, hwnd);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_DeleteTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Call ITaskbarList::HrInit() */
    hr = ITaskbarList_HrInit(taskbarlist);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test ITaskbarList::HrInit() */
    /* Call ITaskbarList::HrInit() again */
    hr = ITaskbarList_HrInit(taskbarlist);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test ITaskbarList::AddTab() */
    /* Check invalid parameters */
    hr = ITaskbarList_AddTab(taskbarlist, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_AddTab(taskbarlist, (HWND)0xdeadbeef);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Normal ITaskbarList::AddTab() */
    hr = ITaskbarList_AddTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Repeat ITaskbarList::AddTab() with the same hwnd */
    hr = ITaskbarList_AddTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_DeleteTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test ITaskbarList::SetActiveAlt() */
    /* Check invalid parameters */
    hr = ITaskbarList_SetActiveAlt(taskbarlist, NULL);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_SetActiveAlt(taskbarlist, (HWND)0xdeadbeef);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Normal ITaskbarList::SetActiveAlt() */
    hr = ITaskbarList_AddTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_SetActiveAlt(taskbarlist, hwnd);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Repeat ITaskbarList::SetActiveAlt() with the same hwnd */
    hr = ITaskbarList_SetActiveAlt(taskbarlist, hwnd);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_DeleteTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test ITaskbarList::ActivateTab() */
    /* Check invalid parameters */
    hr = ITaskbarList_ActivateTab(taskbarlist, NULL);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_ActivateTab(taskbarlist, (HWND)0xdeadbeef);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Normal ITaskbarList::ActivateTab() */
    hr = ITaskbarList_AddTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_ActivateTab(taskbarlist, hwnd);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Repeat ITaskbarList::ActivateTab() with the same hwnd */
    hr = ITaskbarList_ActivateTab(taskbarlist, hwnd);
    todo_wine
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_DeleteTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test ITaskbarList::DeleteTab() */
    /* Check invalid parameters */
    hr = ITaskbarList_DeleteTab(taskbarlist, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_DeleteTab(taskbarlist, (HWND)0xdeadbeef);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_DeleteTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Normal ITaskbarList::DeleteTab() */
    hr = ITaskbarList_AddTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = ITaskbarList_DeleteTab(taskbarlist, hwnd);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ref_count = ITaskbarList_Release(taskbarlist);
    ok(ref_count == 0, "Got unexpected reference count %lu.\n", ref_count);
    DestroyWindow(hwnd);
}

START_TEST(taskbarlist)
{
    CoInitialize(NULL);

    test_ITaskbarList();

    CoUninitialize();
}
