/*
 * Copyright 2012 Detlef Riekenberg
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
#include "taskschd.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(schtasks);

static const WCHAR change_optW[] = {'/','c','h','a','n','g','e',0};
static const WCHAR delete_optW[] = {'/','d','e','l','e','t','e',0};
static const WCHAR enable_optW[] = {'/','e','n','a','b','l','e',0};
static const WCHAR f_optW[] = {'/','f',0};
static const WCHAR tn_optW[] = {'/','t','n',0};

static ITaskFolder *get_tasks_root_folder(void)
{
    ITaskService *service;
    ITaskFolder *root;
    VARIANT empty;
    HRESULT hres;

    hres = CoCreateInstance(&CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
                            &IID_ITaskService, (void**)&service);
    if (FAILED(hres))
        return NULL;

    V_VT(&empty) = VT_EMPTY;
    hres = ITaskService_Connect(service, empty, empty, empty, empty);
    if (FAILED(hres)) {
        FIXME("Connect failed: %08x\n", hres);
        return NULL;
    }

    hres = ITaskService_GetFolder(service, NULL, &root);
    ITaskService_Release(service);
    if (FAILED(hres)) {
        FIXME("GetFolder failed: %08x\n", hres);
        return NULL;
    }

    return root;
}

static IRegisteredTask *get_registered_task(const WCHAR *name)
{
    IRegisteredTask *registered_task;
    ITaskFolder *root;
    BSTR str;
    HRESULT hres;

    root = get_tasks_root_folder();
    if (!root)
        return NULL;

    str = SysAllocString(name);
    hres = ITaskFolder_GetTask(root, str, &registered_task);
    SysFreeString(str);
    ITaskFolder_Release(root);
    if (FAILED(hres)) {
        FIXME("GetTask failed: %08x\n", hres);
        return NULL;
    }

    return registered_task;
}

static int change_command(int argc, WCHAR *argv[])
{
    const WCHAR *task_name;
    IRegisteredTask *task;
    HRESULT hres;

    if (!argc) {
        FIXME("Missing /tn option\n");
        return 1;
    }

    if (strcmpiW(argv[0], tn_optW)) {
        FIXME("Unsupported %s option\n", debugstr_w(argv[0]));
        return 0;
    }

    if (argc < 2) {
        FIXME("Missing /tn value\n");
        return 1;
    }

    task_name = argv[1];
    argc -= 2;
    argv += 2;
    if (!argc) {
        FIXME("Missing change options\n");
        return 1;
    }

    if (strcmpiW(argv[0], enable_optW) || argc > 1) {
        FIXME("Unsupported arguments %s\n", debugstr_w(argv[0]));
        return 0;
    }

    task = get_registered_task(task_name);
    if (!task)
        return 1;

    hres = IRegisteredTask_put_Enabled(task, VARIANT_TRUE);
    IRegisteredTask_Release(task);
    if (FAILED(hres)) {
        FIXME("put_Enabled failed: %08x\n", hres);
        return 1;
    }

    return 0;
}

static int delete_command(int argc, WCHAR *argv[])
{
    const WCHAR *task_name = NULL;
    ITaskFolder *root = NULL;
    BSTR str;
    HRESULT hres;

    while (argc) {
        if (!strcmpiW(argv[0], f_optW)) {
            TRACE("force opt\n");
            argc--;
            argv++;
        }else if(!strcmpiW(argv[0], tn_optW)) {
            if (argc < 2) {
                FIXME("Missing /tn value\n");
                return 1;
            }

            if (task_name) {
                FIXME("Duplicated /tn argument\n");
                return 1;
            }

            task_name = argv[1];
            argc -= 2;
            argv += 2;
        }else {
            FIXME("Unsupported argument %s\n", debugstr_w(argv[0]));
            return 1;
        }
    }

    if (!task_name) {
        FIXME("Missing /tn argument\n");
        return 1;
    }

    root = get_tasks_root_folder();
    if (!root)
        return 1;

    str = SysAllocString(task_name);
    hres = ITaskFolder_DeleteTask(root, str, 0);
    SysFreeString(str);
    ITaskFolder_Release(root);
    if (FAILED(hres))
        return 1;

    return 0;
}

int wmain(int argc, WCHAR *argv[])
{
    int i, ret = 0;

    for (i = 0; i < argc; i++)
        TRACE(" %s", wine_dbgstr_w(argv[i]));
    TRACE("\n");

    CoInitialize(NULL);

    if (argc < 2)
        FIXME("Print current tasks state\n");
    else if (!strcmpiW(argv[1], change_optW))
        ret = change_command(argc - 2, argv + 2);
    else if (!strcmpiW(argv[1], delete_optW))
        ret = delete_command(argc - 2, argv + 2);
    else
        FIXME("Unsupported command %s\n", debugstr_w(argv[1]));

    CoUninitialize();
    return ret;
}
