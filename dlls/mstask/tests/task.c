/*
 * Test suite for Task interface
 *
 * Copyright (C) 2008 Google (Roy Shea)
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

#include "corerror.h"
#include "mstask.h"
#include "wine/test.h"

static ITaskScheduler *test_task_scheduler;
static ITask *test_task;
static const WCHAR empty[] = {0};

/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
static char *get_tmp_space(int size)
{
    static char *list[16];
    static long pos;
    char *ret;
    int idx;

    idx = ++pos % (sizeof(list)/sizeof(list[0]));
    if ((ret = realloc(list[idx], size)))
        list[idx] = ret;
    return ret;
}

static const char *dbgstr_w(LPCWSTR str)
{
    char *buf;
    int len;
    if(!str)
        return "(null)";
    len = lstrlenW(str) + 1;
    buf = get_tmp_space(len);
    WideCharToMultiByte(CP_ACP, 0, str, -1, buf, len, NULL, NULL);
    return buf;
}

static BOOL setup_task(void)
{
    HRESULT hres;
    const WCHAR task_name[] = {'T','e','s','t','i','n','g', 0};

    hres = CoCreateInstance(&CLSID_CTaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            &IID_ITaskScheduler, (void **) &test_task_scheduler);
    if(hres != S_OK)
        return FALSE;
    hres = ITaskScheduler_NewWorkItem(test_task_scheduler, task_name, &CLSID_CTask,
            &IID_ITask, (IUnknown**)&test_task);
    if(hres != S_OK)
    {
        ITaskScheduler_Release(test_task_scheduler);
        return FALSE;
    }
    return TRUE;
}

static void cleanup_task(void)
{
    ITask_Release(test_task);
    ITaskScheduler_Release(test_task_scheduler);
}

static LPCWSTR path_resolve_name(LPCWSTR base_name)
{
    static WCHAR buffer[MAX_PATH];
    int len;

    len = SearchPathW(NULL, base_name, NULL, 0, NULL, NULL);
    if (len == 0)
        return base_name;
    else if (len < MAX_PATH)
    {
        SearchPathW(NULL, base_name, NULL, MAX_PATH, buffer, NULL);
        return buffer;
    }
    return NULL;
}

static void test_SetApplicationName_GetApplicationName(void)
{
    BOOL setup;
    HRESULT hres;
    LPWSTR stored_name;
    LPCWSTR full_name;
    const WCHAR non_application_name[] = {'N','o','S','u','c','h',
            'A','p','p','l','i','c','a','t','i','o','n', 0};
    const WCHAR notepad_exe[] = {
            'n','o','t','e','p','a','d','.','e','x','e', 0};
    const WCHAR notepad[] = {'n','o','t','e','p','a','d', 0};

    setup = setup_task();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

    /* Attempt getting before setting application name */
    hres = ITask_GetApplicationName(test_task, &stored_name);
    todo_wine ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        todo_wine ok(!lstrcmpW(stored_name, empty),
                "Got %s, expected empty string\n", dbgstr_w(stored_name));
        CoTaskMemFree(stored_name);
    }

    /* Set application name to a non-existent application and then get
     * the application name that is actually stored */
    hres = ITask_SetApplicationName(test_task, non_application_name);
    todo_wine ok(hres == S_OK, "Failed setting name %s: %08x\n",
            dbgstr_w(non_application_name), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    todo_wine ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(non_application_name);
        todo_wine ok(!lstrcmpW(stored_name, full_name), "Got %s, expected %s\n",
                dbgstr_w(stored_name), dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Set a valid application name with program type extension and then
     * get the stored name */
    hres = ITask_SetApplicationName(test_task, notepad_exe);
    todo_wine ok(hres == S_OK, "Failed setting name %s: %08x\n",
            dbgstr_w(notepad_exe), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    todo_wine ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(notepad_exe);
        todo_wine ok(!lstrcmpW(stored_name, full_name), "Got %s, expected %s\n",
                dbgstr_w(stored_name), dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Set a valid application name without program type extension and
     * then get the stored name */
    hres = ITask_SetApplicationName(test_task, notepad);
    todo_wine ok(hres == S_OK, "Failed setting name %s: %08x\n", dbgstr_w(notepad), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    todo_wine ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(notepad);
        todo_wine ok(!lstrcmpW(stored_name, full_name), "Got %s, expected %s\n",
                dbgstr_w(stored_name), dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* After having a valid application name set, set application name
     * to a non-existant application and then get the name that is
     * actually stored */
    hres = ITask_SetApplicationName(test_task, non_application_name);
    todo_wine ok(hres == S_OK, "Failed setting name %s: %08x\n",
            dbgstr_w(non_application_name), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    todo_wine ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        full_name = path_resolve_name(non_application_name);
        todo_wine ok(!lstrcmpW(stored_name, full_name), "Got %s, expected %s\n",
                dbgstr_w(stored_name), dbgstr_w(full_name));
        CoTaskMemFree(stored_name);
    }

    /* Clear application name */
    hres = ITask_SetApplicationName(test_task, empty);
    todo_wine ok(hres == S_OK, "Failed setting name %s: %08x\n", dbgstr_w(empty), hres);
    hres = ITask_GetApplicationName(test_task, &stored_name);
    todo_wine ok(hres == S_OK, "GetApplicationName failed: %08x\n", hres);
    if (hres == S_OK)
    {
        todo_wine ok(!lstrcmpW(stored_name, empty),
                "Got %s, expected empty string\n", dbgstr_w(stored_name));
        CoTaskMemFree(stored_name);
    }

    cleanup_task();
    return;
}

static void test_CreateTrigger(void)
{
    BOOL setup;
    HRESULT hres;
    WORD trigger_index;
    ITaskTrigger *test_trigger;

    setup = setup_task();
    ok(setup, "Failed to setup test_task\n");
    if (!setup)
    {
        skip("Failed to create task.  Skipping tests.\n");
        return;
    }

    hres = ITask_CreateTrigger(test_task, &trigger_index, &test_trigger);
    todo_wine ok(hres == S_OK, "Failed to create trigger: 0x%08x\n", hres);
    if (hres != S_OK)
    {
        cleanup_task();
        return;
    }

    ITaskTrigger_Release(test_trigger);
    cleanup_task();
    return;
}

START_TEST(task)
{
    CoInitialize(NULL);
    test_SetApplicationName_GetApplicationName();
    test_CreateTrigger();
    CoUninitialize();
}
