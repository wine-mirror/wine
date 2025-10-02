/*
 * Copyright (C) 2023 Mohamad Al-Jaf
 * Copyright (C) 2025 Vibhav Pant
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

#include "wine/test.h"
#include "winreg.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "devguid.h"
#include "initguid.h"
#include "devpkey.h"
#include "propkey.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "ntddvdeo.h"
#include "devfiltertypes.h"
#include "devquery.h"

static void test_CM_MapCrToWin32Err(void)
{
    unsigned int i;
    DWORD ret;

    static const struct
    {
        CONFIGRET code;
        DWORD win32_error;
    }
    map_codes[] =
    {
        { CR_SUCCESS,                  ERROR_SUCCESS },
        { CR_OUT_OF_MEMORY,            ERROR_NOT_ENOUGH_MEMORY },
        { CR_INVALID_POINTER,          ERROR_INVALID_USER_BUFFER },
        { CR_INVALID_FLAG,             ERROR_INVALID_FLAGS },
        { CR_INVALID_DEVNODE,          ERROR_INVALID_DATA },
        { CR_INVALID_DEVINST,          ERROR_INVALID_DATA },
        { CR_NO_SUCH_DEVNODE,          ERROR_NOT_FOUND },
        { CR_NO_SUCH_DEVINST,          ERROR_NOT_FOUND },
        { CR_ALREADY_SUCH_DEVNODE,     ERROR_ALREADY_EXISTS },
        { CR_ALREADY_SUCH_DEVINST,     ERROR_ALREADY_EXISTS },
        { CR_BUFFER_SMALL,             ERROR_INSUFFICIENT_BUFFER },
        { CR_NO_REGISTRY_HANDLE,       ERROR_INVALID_HANDLE },
        { CR_REGISTRY_ERROR,           ERROR_REGISTRY_CORRUPT },
        { CR_INVALID_DEVICE_ID,        ERROR_INVALID_DATA },
        { CR_NO_SUCH_VALUE,            ERROR_NOT_FOUND },
        { CR_NO_SUCH_REGISTRY_KEY,     ERROR_FILE_NOT_FOUND },
        { CR_INVALID_MACHINENAME,      ERROR_INVALID_DATA },
        { CR_REMOTE_COMM_FAILURE,      ERROR_SERVICE_NOT_ACTIVE },
        { CR_MACHINE_UNAVAILABLE,      ERROR_SERVICE_NOT_ACTIVE },
        { CR_NO_CM_SERVICES,           ERROR_SERVICE_NOT_ACTIVE },
        { CR_ACCESS_DENIED,            ERROR_ACCESS_DENIED },
        { CR_CALL_NOT_IMPLEMENTED,     ERROR_CALL_NOT_IMPLEMENTED },
        { CR_INVALID_PROPERTY,         ERROR_INVALID_DATA },
        { CR_NO_SUCH_DEVICE_INTERFACE, ERROR_NOT_FOUND },
        { CR_INVALID_REFERENCE_STRING, ERROR_INVALID_DATA },
        { CR_DEFAULT,                  0xdeadbeef },
        { CR_INVALID_RES_DES,          0xdeadbeef },
        { CR_INVALID_LOG_CONF,         0xdeadbeef },
        { CR_INVALID_ARBITRATOR,       0xdeadbeef },
        { CR_INVALID_NODELIST,         0xdeadbeef },
        { CR_DEVNODE_HAS_REQS,         0xdeadbeef },
        { CR_DEVINST_HAS_REQS,         0xdeadbeef },
        { CR_INVALID_RESOURCEID,       0xdeadbeef },
        { CR_DLVXD_NOT_FOUND,          0xdeadbeef },
        { CR_NO_MORE_LOG_CONF,         0xdeadbeef },
        { CR_NO_MORE_RES_DES,          0xdeadbeef },
        { CR_INVALID_RANGE_LIST,       0xdeadbeef },
        { CR_INVALID_RANGE,            0xdeadbeef },
        { CR_FAILURE,                  0xdeadbeef },
        { CR_NO_SUCH_LOGICAL_DEV,      0xdeadbeef },
        { CR_CREATE_BLOCKED,           0xdeadbeef },
        { CR_NOT_SYSTEM_VM,            0xdeadbeef },
        { CR_REMOVE_VETOED,            0xdeadbeef },
        { CR_APM_VETOED,               0xdeadbeef },
        { CR_INVALID_LOAD_TYPE,        0xdeadbeef },
        { CR_NO_ARBITRATOR,            0xdeadbeef },
        { CR_INVALID_DATA,             0xdeadbeef },
        { CR_INVALID_API,              0xdeadbeef },
        { CR_DEVLOADER_NOT_READY,      0xdeadbeef },
        { CR_NEED_RESTART,             0xdeadbeef },
        { CR_NO_MORE_HW_PROFILES,      0xdeadbeef },
        { CR_DEVICE_NOT_THERE,         0xdeadbeef },
        { CR_WRONG_TYPE,               0xdeadbeef },
        { CR_INVALID_PRIORITY,         0xdeadbeef },
        { CR_NOT_DISABLEABLE,          0xdeadbeef },
        { CR_FREE_RESOURCES,           0xdeadbeef },
        { CR_QUERY_VETOED,             0xdeadbeef },
        { CR_CANT_SHARE_IRQ,           0xdeadbeef },
        { CR_NO_DEPENDENT,             0xdeadbeef },
        { CR_SAME_RESOURCES,           0xdeadbeef },
        { CR_DEVICE_INTERFACE_ACTIVE,  0xdeadbeef },
        { CR_INVALID_CONFLICT_LIST,    0xdeadbeef },
        { CR_INVALID_INDEX,            0xdeadbeef },
        { CR_INVALID_STRUCTURE_SIZE,   0xdeadbeef },
        { NUM_CR_RESULTS,              0xdeadbeef },
    };

    for ( i = 0; i < ARRAY_SIZE(map_codes); i++ )
    {
        ret = CM_MapCrToWin32Err( map_codes[i].code, 0xdeadbeef );
        ok( ret == map_codes[i].win32_error, "%#lx returned unexpected %ld.\n", map_codes[i].code, ret );
    }
}

HRESULT (WINAPI *pDevCreateObjectQuery)(DEV_OBJECT_TYPE, ULONG, ULONG, const DEVPROPCOMPKEY*, ULONG,
                                        const DEVPROP_FILTER_EXPRESSION*, PDEV_QUERY_RESULT_CALLBACK, void*, HDEVQUERY*);
void (WINAPI *pDevCloseObjectQuery)(HDEVQUERY);
HRESULT (WINAPI *pDevGetObjects)(DEV_OBJECT_TYPE, ULONG, ULONG, const DEVPROPCOMPKEY*, ULONG,
                                 const DEVPROP_FILTER_EXPRESSION*, ULONG*, const DEV_OBJECT**);
void (WINAPI *pDevFreeObjects)(ULONG, const DEV_OBJECT*);
HRESULT (WINAPI *pDevGetObjectProperties)(DEV_OBJECT_TYPE, const WCHAR *, ULONG, ULONG, const DEVPROPCOMPKEY *, ULONG *,
                                          const DEVPROPERTY **);
void (WINAPI *pDevFreeObjectProperties)(ULONG, const DEVPROPERTY *);
const DEVPROPERTY* (WINAPI *pDevFindProperty)(const DEVPROPKEY *, DEVPROPSTORE, PCWSTR, ULONG, const DEVPROPERTY *);

DEFINE_DEVPROPKEY(DEVPROPKEY_GPU_LUID, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2);

static void test_CM_Get_Device_ID_List(void)
{
    struct
    {
        WCHAR id[128];
        DEVINST inst;
    }
    instances[128];
    SP_DEVINFO_DATA device = { sizeof(device) };
    unsigned int i, count, expected_count;
    WCHAR wguid_str[64], id[128], *wbuf, *wp;
    char guid_str[64], id_a[128], *buf, *p;
    DEVINST devinst;
    CONFIGRET ret;
    HDEVINFO set;
    ULONG len;

    StringFromGUID2(&GUID_DEVCLASS_DISPLAY, wguid_str, ARRAY_SIZE(wguid_str));
    wp = wguid_str;
    p = guid_str;
    while ((*p++ = *wp++))
        ;

    ret = CM_Get_Device_ID_List_SizeW(NULL, wguid_str, CM_GETIDLIST_FILTER_CLASS);
    ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
    len = 0xdeadbeef;
    ret = CM_Get_Device_ID_List_SizeW(&len, NULL, CM_GETIDLIST_FILTER_CLASS);
    ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
    ok(!len, "got %#lx.\n", len);
    len = 0xdeadbeef;
    ret = CM_Get_Device_ID_List_SizeW(&len, L"q", CM_GETIDLIST_FILTER_CLASS);
    ok(ret == CR_INVALID_DATA, "got %#lx.\n", ret);
    ok(!len, "got %#lx.\n", len);

    ret = CM_Get_Device_ID_List_SizeA(NULL, guid_str, CM_GETIDLIST_FILTER_CLASS);
    ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
    len = 0xdeadbeef;
    ret = CM_Get_Device_ID_List_SizeA(&len, NULL, CM_GETIDLIST_FILTER_CLASS);
    ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
    ok(!len, "got %#lx.\n", len);
    len = 0xdeadbeef;
    ret = CM_Get_Device_ID_List_SizeA(&len, "q", CM_GETIDLIST_FILTER_CLASS);
    ok(ret == CR_INVALID_DATA, "got %#lx.\n", ret);
    ok(!len, "got %#lx.\n", len);

    len = 0xdeadbeef;
    ret = CM_Get_Device_ID_List_SizeW(&len, NULL, 0);
    ok(!ret, "got %#lx.\n", ret);
    ok(len > 2, "got %#lx.\n", len);

    wbuf = malloc(len * sizeof(*wbuf));
    buf = malloc(len);

    ret = CM_Get_Device_ID_ListW(NULL, wbuf, len, 0);
    ok(!ret, "got %#lx.\n", ret);

    len = 0xdeadbeef;
    ret = CM_Get_Device_ID_List_SizeW(&len, wguid_str, CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT);
    ok(!ret, "got %#lx.\n", ret);
    ok(len > 2, "got %lu.\n", len);
    memset(wbuf, 0xcc, len * sizeof(*wbuf));
    ret = CM_Get_Device_ID_ListW(wguid_str, wbuf, 0, CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT);
    ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
    ok(wbuf[0] == 0xcccc, "got %#x.\n", wbuf[0]);
    memset(wbuf, 0xcc, len * sizeof(*wbuf));
    ret = CM_Get_Device_ID_ListW(wguid_str, wbuf, 1, CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT);
    ok(ret == CR_BUFFER_SMALL, "got %#lx.\n", ret);
    ok(!wbuf[0], "got %#x.\n", wbuf[0]);

    len = 0xdeadbeef;
    ret = CM_Get_Device_ID_List_SizeA(&len, guid_str, CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT);
    ok(!ret, "got %#lx.\n", ret);
    ok(len > 2, "got %lu.\n", len);
    memset(buf, 0x7c, len);
    ret = CM_Get_Device_ID_ListA(guid_str, buf, 0, CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT);
    ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
    ok(buf[0] == 0x7c, "got %#x.\n", buf[0]);
    memset(buf, 0x7c, len);
    ret = CM_Get_Device_ID_ListA(guid_str, buf, 1, CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT);
    ok(ret == CR_BUFFER_SMALL, "got %#lx.\n", ret);
    ok(buf[0] == 0x7c, "got %#x.\n", buf[0]);

    set = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT);
    ok(set != &GUID_DEVCLASS_DISPLAY, "got error %#lx.\n", GetLastError());
    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        ok(i < ARRAY_SIZE(instances), "got %u.\n", i);
        ret = SetupDiGetDeviceInstanceIdW(set, &device, instances[i].id, sizeof(instances[i].id), NULL);
        ok(ret, "got error %#lx.\n", GetLastError());
        instances[i].inst = device.DevInst;
    }
    SetupDiDestroyDeviceInfoList(set);
    expected_count = i;
    ok(expected_count, "got 0.\n");

    wcscpy(id, L"q");
    devinst = 0xdeadbeef;
    ret = CM_Locate_DevNodeW(&devinst, id, 0);
    todo_wine_if(ret == CR_NO_SUCH_DEVNODE) ok(ret == CR_INVALID_DEVICE_ID, "got %#lx.\n", ret);
    ok(!devinst, "got %#lx.\n", devinst);

    wcscpy(id, instances[0].id);
    id[0] = 'Q';
    ret = CM_Locate_DevNodeW(&devinst, id, 0);
    ok(ret == CR_NO_SUCH_DEVNODE, "got %#lx.\n", ret);

    for (i = 0; i < expected_count; ++i)
    {
        DEVPROPTYPE type;
        ULONG size;

        *id = 0;
        ret = CM_Get_Device_IDW(instances[i].inst, id, ARRAY_SIZE(id), 0);
        ok(!ret, "got %#lx.\n", ret);
        ok(!wcscmp(id, instances[i].id), "got %s, expected %s.\n", debugstr_w(id), debugstr_w(instances[i].id));
        size = len;
        ret = CM_Get_DevNode_PropertyW(instances[i].inst, &DEVPROPKEY_GPU_LUID, &type, wbuf, &size, 0);
        ok(!ret || ret == CR_NO_SUCH_VALUE, "got %#lx.\n", ret);
        if (!ret)
            ok(type == DEVPROP_TYPE_UINT64, "got %#lx.\n", type);

        devinst = 0xdeadbeef;
        ret = CM_Locate_DevNodeW(&devinst, instances[i].id, 0);
        ok(!ret, "got %#lx, id %s.\n", ret, debugstr_w(instances[i].id));
        ok(devinst == instances[i].inst, "got %#lx, expected %#lx.\n", devinst, instances[i].inst);
        p = id_a;
        wp = instances[i].id;
        while((*p++ = *wp++))
            ;
        devinst = 0xdeadbeef;
        ret = CM_Locate_DevNodeA(&devinst, id_a, 0);
        ok(!ret, "got %#lx, id %s.\n", ret, debugstr_a(id_a));
        ok(devinst == instances[i].inst, "got %#lx, expected %#lx.\n", devinst, instances[i].inst);
    }

    memset(wbuf, 0xcc, len * sizeof(*wbuf));
    ret = CM_Get_Device_ID_ListW(wguid_str, wbuf, len, CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT);
    ok(!ret, "got %#lx.\n", ret);
    count = 0;
    wp = wbuf;
    while (*wp)
    {
        ++count;
        ok(!wcsnicmp(wp, L"PCI\\", 4) || !wcsnicmp(wp, L"VMBUS\\", 6), "got %s.\n", debugstr_w(wp));
        wp += wcslen(wp) + 1;
    }
    ok(count == expected_count, "got %u, expected %u.\n", count, expected_count);

    memset(buf, 0xcc, len * sizeof(*buf));
    ret = CM_Get_Device_ID_ListA(guid_str, buf, len, CM_GETIDLIST_FILTER_CLASS | CM_GETIDLIST_FILTER_PRESENT);
    ok(!ret, "got %#lx.\n", ret);
    count = 0;
    p = buf;
    while (*p)
    {
        ++count;
        ok(!strnicmp(p, "PCI\\", 4) || !strnicmp(p, "VMBUS\\", 6), "got %s.\n", debugstr_a(p));
        p += strlen(p) + 1;
    }
    ok(count == expected_count, "got %u, expected %u.\n", count, expected_count);

    free(wbuf);
    free(buf);
}

DWORD WINAPI notify_callback( HCMNOTIFICATION notify, void *ctx, CM_NOTIFY_ACTION action,
                              CM_NOTIFY_EVENT_DATA *data, DWORD size )
{
    return ERROR_SUCCESS;
}

static void test_CM_Register_Notification( void )
{
    struct
    {
        CM_NOTIFY_FILTER filter;
        CONFIGRET ret;
    } test_cases[] = {
        {
            { 0, CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES, CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE, 0 },
            CR_INVALID_DATA
        },
        {
            { sizeof( CM_NOTIFY_FILTER ) + 1, 0, CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE, 0,
              .u.DeviceInterface = { GUID_DEVINTERFACE_DISPLAY_ADAPTER } },
            CR_INVALID_DATA
        },
        {
            { sizeof( CM_NOTIFY_FILTER ), CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES,
              CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE, 0, .u.DeviceInterface = { GUID_DEVINTERFACE_DISPLAY_ADAPTER } },
            CR_INVALID_DATA
        },
        {
            { sizeof( CM_NOTIFY_FILTER ), CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES,
              CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE, 0 },
            CR_SUCCESS
        },
        {
            { sizeof( CM_NOTIFY_FILTER ), 0, CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE, 0,
              .u.DeviceInterface = { GUID_DEVINTERFACE_DISPLAY_ADAPTER } },
            CR_SUCCESS
        }
    };
    DWORD (WINAPI *pCM_Register_Notification)(PCM_NOTIFY_FILTER,PVOID,PCM_NOTIFY_CALLBACK,PHCMNOTIFICATION) = NULL;
    DWORD (WINAPI *pCM_Unregister_Notification)(HCMNOTIFICATION) = NULL;
    HMODULE cfgmgr32 = GetModuleHandleW( L"cfgmgr32" );
    DWORD i;
    HCMNOTIFICATION notify = NULL;
    CONFIGRET ret;

    if (cfgmgr32)
    {
        pCM_Register_Notification = (void *)GetProcAddress( cfgmgr32, "CM_Register_Notification" );
        pCM_Unregister_Notification = (void *)GetProcAddress( cfgmgr32, "CM_Unregister_Notification" );
    }

    if (!pCM_Register_Notification)
    {
        win_skip( "CM_Register_Notification not found, skipping tests\n" );
        return;
    }

    ret = pCM_Register_Notification( NULL, NULL, NULL, NULL );
    ok( ret == CR_FAILURE, "Expected 0x13, got %#lx.\n", ret );

    ret = pCM_Register_Notification( NULL, NULL, NULL, &notify );
    ok( ret == CR_INVALID_DATA, "Expected 0x1f, got %#lx.\n", ret );
    ok( !notify, "Expected handle to be NULL, got %p\n", notify );

    for (i = 0; i < ARRAY_SIZE( test_cases ); i++)
    {
        notify = NULL;
        winetest_push_context( "test_cases %lu", i );
        ret = pCM_Register_Notification( &test_cases[i].filter, NULL, notify_callback, &notify );
        ok( test_cases[i].ret == ret, "Expected %#lx, got %#lx\n", test_cases[i].ret, ret );
        if (test_cases[i].ret)
            ok( !notify, "Expected handle to be NULL, got %p\n", notify );
        if (notify)
        {
            ret = pCM_Unregister_Notification( notify );
            ok( !ret, "Expected 0, got %#lx\n", ret );
        }
        winetest_pop_context();
    }
}

static void check_device_path_casing(const WCHAR *original_path)
{
    HKEY current_key, tmp;
    WCHAR *path = wcsdup(original_path);
    WCHAR key_name[MAX_PATH];
    WCHAR separator[] = L"#";
    WCHAR *token, *context = NULL;
    LSTATUS ret;
    DWORD i;

    ret = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Enum", &current_key);
    ok(!ret, "Failed to open enum key: %#lx.\n", ret);

    token = wcstok_s(path + 4, separator, &context);  /* skip \\?\ */
    while (token)
    {
        if (token[0] == L'{' && wcslen(token) == 38) break; /* reached GUID part, done */

        i = 0;
        while (!(ret = RegEnumKeyW(current_key, i++, key_name, ARRAY_SIZE(key_name))))
        {
            if(!wcscmp(token, key_name))
            {
                ret = RegOpenKeyW(current_key, token, &tmp);
                ok(!ret, "Failed to open registry key %s: %#lx.\n", debugstr_w(token), ret);
                RegCloseKey(current_key);
                current_key = tmp;
                break;
            }
        }
        ok(!ret, "Failed to find %s in registry: %#lx.\n", debugstr_w(token), ret);
        if (ret) break;

        token = wcstok_s(NULL, separator, &context);
    }

    RegCloseKey(current_key);
    free(path);
}

static void test_CM_Get_Device_Interface_List(void)
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    SP_DEVINFO_DATA device = { sizeof(device) };
    WCHAR instance_id[256], expected_id[256];
    DEVPROPKEY zero_key = {{0}, 0};
    unsigned int count, count2;
    char *buffera, *pa;
    WCHAR *buffer, *p;
    ULONG size, size2;
    DEVPROPTYPE type;
    CONFIGRET ret;
    HDEVINFO set;
    GUID guid;
    BOOL bret;

    guid = GUID_DEVINTERFACE_DISPLAY_ADAPTER;

    ret = CM_Get_Device_Interface_List_SizeW(&size, &guid, NULL, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    ok(!ret, "got %#lx.\n", ret);

    buffer = malloc(size * sizeof(*buffer));
    ret = CM_Get_Device_Interface_ListW( &guid, NULL, buffer, size, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    ok(!ret, "got %#lx.\n", ret);

    ret = CM_Get_Device_Interface_List_SizeA(&size2, &guid, NULL, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    ok(!ret, "got %#lx.\n", ret);
    ok(size2 == size, "got %lu, %lu.\n", size, size2);
    buffera = malloc(size2 * sizeof(*buffera));
    ret = CM_Get_Device_Interface_ListA(&guid, NULL, buffera, size2, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    ok(!ret, "got %#lx.\n", ret);
    p = malloc(size2 * sizeof(*p));
    memset(p, 0xcc, size2 * sizeof(*p));
    pa = buffera;
    *p = 0;
    while (*pa)
    {
        MultiByteToWideChar(CP_ACP, 0, pa, -1, p + (pa - buffera), size2 - (pa - buffera));
        pa += strlen(pa) + 1;
    }
    p[pa - buffera] = 0;
    ok(!memcmp(p, buffer, size * sizeof(*p)), "results differ, %s, %s.\n", debugstr_wn(p, size), debugstr_wn(buffer, size));
    free(p);
    free(buffera);

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;

    count = 0;
    p = buffer;
    while (*p)
    {
        DEVPROP_BOOLEAN val = DEVPROP_FALSE;

        check_device_path_casing(p);
        set = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
        ok(set != INVALID_HANDLE_VALUE, "got %p.\n", set);
        bret = SetupDiOpenDeviceInterfaceW(set, p, 0, &iface);
        ok(bret, "got error %lu.\n", GetLastError());
        memset(iface_detail_buffer, 0xcc, sizeof(iface_detail_buffer));
        iface_data->cbSize = sizeof(*iface_data);
        bret = SetupDiGetDeviceInterfaceDetailW(set, &iface, iface_data, sizeof(iface_detail_buffer), NULL, &device);
        ok(bret, "got error %lu.\n", GetLastError());
        ok(!wcsicmp(iface_data->DevicePath, p), "got %s, expected %s.\n", debugstr_w(p), debugstr_w(iface_data->DevicePath));
        bret = SetupDiGetDeviceInstanceIdW(set, &device, expected_id, ARRAY_SIZE(expected_id), NULL);
        ok(bret, "got error %lu.\n", GetLastError());
        SetupDiDestroyDeviceInfoList(set);

        size = 0xdeadbeef;
        type = 0xdeadbeef;
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_Device_InstanceId, &type, NULL, &size, 0);
        ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
        ok(type == 0xdeadbeef, "got type %#lx.\n", type);
        ok(size == 0xdeadbeef, "got %#lx.\n", size);

        size = 0;
        type = 0xdeadbeef;
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_Device_InstanceId, &type, NULL, &size, 0);
        ok(ret == CR_BUFFER_SMALL, "got %#lx.\n", ret);
        ok(type == DEVPROP_TYPE_STRING, "got type %#lx.\n", type);
        ok(size && size != 0xdeadbeef, "got %#lx.\n", size);

        ret = CM_Get_Device_Interface_PropertyW(p, NULL, &type, (BYTE *)instance_id, &size, 0);
        ok(ret == CR_FAILURE, "got %#lx.\n", ret);
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_Device_InstanceId, NULL, (BYTE *)instance_id, &size, 0);
        ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
        ret = CM_Get_Device_Interface_PropertyW(NULL, &DEVPKEY_Device_InstanceId, &type, (BYTE *)instance_id, &size, 0);
        ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_Device_InstanceId, &type, (BYTE *)instance_id, NULL, 0);
        ok(ret == CR_INVALID_POINTER, "got %#lx.\n", ret);
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_Device_InstanceId, &type, (BYTE *)instance_id, &size, 1);
        ok(ret == CR_INVALID_FLAG, "got %#lx.\n", ret);

        size = 0;
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_Device_InstanceId, &type, NULL, &size, 0);
        ok(ret == CR_BUFFER_SMALL, "got %#lx.\n", ret);

        --size;
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_Device_InstanceId, &type, (BYTE *)instance_id, &size, 0);
        ok(ret == CR_BUFFER_SMALL, "got %#lx.\n", ret);

        type = 0xdeadbeef;
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_Device_InstanceId, &type, (BYTE *)instance_id, &size, 0);
        ok(!ret, "got %#lx.\n", ret);
        ok(type == DEVPROP_TYPE_STRING, "got type %#lx.\n", type);
        ok(!wcsicmp(instance_id, expected_id), "got %s, expected %s.\n", debugstr_w(instance_id), debugstr_w(expected_id));

        type = 0xdeadbeef;
        size = sizeof(val);
        ret = CM_Get_Device_Interface_PropertyW(p, &DEVPKEY_DeviceInterface_Enabled, &type, (BYTE *)&val, &size, 0);
        ok(!ret, "got %#lx.\n", ret);
        ok(type == DEVPROP_TYPE_BOOLEAN, "got type %#lx.\n", type);
        ok(size == sizeof(val), "got size %lu.\n", size);
        ok(val == DEVPROP_TRUE, "got val %d.\n", val);

        size = 0;
        ret = CM_Get_Device_Interface_PropertyW(p, &zero_key, &type, NULL, &size, 0);
        ok(ret == CR_NO_SUCH_VALUE, "got %#lx.\n", ret);
        p += wcslen(p) + 1;
        ++count;
    }

    free(buffer);

    set = SetupDiGetClassDevsW(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    ok(set != INVALID_HANDLE_VALUE, "got %p.\n", set);
    for (count2 = 0; SetupDiEnumDeviceInterfaces(set, NULL, &guid, count2, &iface); ++count2)
        ;
    SetupDiDestroyDeviceInfoList(set);
    ok(count == count2, "got %u, expected %u.\n", count, count2);

    ret = CM_Get_Device_Interface_PropertyW(L"qqq", &DEVPKEY_Device_InstanceId, &type, (BYTE *)instance_id, &size, 0);
    ok(ret == CR_NO_SUCH_DEVICE_INTERFACE || broken(ret == CR_INVALID_DATA) /* w7 */, "got %#lx.\n", ret);
}

struct test_property
{
    DEVPROPKEY key;
    DEVPROPTYPE type;
};

DEFINE_DEVPROPKEY(DEVPKEY_dummy, 0xdeadbeef, 0xdead, 0xbeef, 0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, 0xef, 1);

static BOOL dev_property_val_equal( const DEVPROPERTY *p1, const DEVPROPERTY *p2 )
{
    if (!(p1->Type == p2->Type && p1->BufferSize == p2->BufferSize))
        return FALSE;
    switch (p1->Type)
    {
    case DEVPROP_TYPE_STRING:
        return !wcsicmp( (WCHAR *)p1->Buffer, (WCHAR *)p2->Buffer );
    default:
        return !memcmp( p1->Buffer, p2->Buffer, p1->BufferSize );
    }
}

static const char *debugstr_DEVPROP_val( const DEVPROPERTY *prop )
{
    switch (prop->Type)
    {
    case DEVPROP_TYPE_STRING:
        return wine_dbg_sprintf( "{type=%#lx buf=%s buf_len=%lu}", prop->Type, debugstr_w( prop->Buffer ), prop->BufferSize );
    default:
        return wine_dbg_sprintf( "{type=%#lx buf=%p buf_len=%lu}", prop->Type, prop->Buffer, prop->BufferSize );
    }
}

static const char *debugstr_DEVPROPKEY( const DEVPROPKEY *key )
{
    if (!key) return "(null)";
    return wine_dbg_sprintf( "{%s, %04lx}", debugstr_guid( &key->fmtid ), key->pid );
}

static const char *debugstr_DEVPROPCOMPKEY( const DEVPROPCOMPKEY *key )
{
    if (!key) return "(null)";
    return wine_dbg_sprintf( "{%s, %d, %s}", debugstr_DEVPROPKEY( &key->Key ), key->Store,
                             debugstr_w( key->LocaleName ) );
}

static void test_DevGetObjectProperties( DEV_OBJECT_TYPE type, const WCHAR *id, const DEVPROPERTY *exp_props, ULONG props_len )
{
    DEVPROPCOMPKEY dummy_propcompkey = { DEVPKEY_dummy, DEVPROP_STORE_SYSTEM, NULL };
    ULONG buf_len, rem_props = props_len, i;
    const DEVPROPERTY *buf;
    DEVPROPCOMPKEY *keys;
    HRESULT hr;

    if (!pDevGetObjectProperties || !pDevFreeObjectProperties || !pDevFindProperty)
    {
        win_skip( "Functions unavailable, skipping test. (%p %p %p)\n", pDevGetObjects, pDevFreeObjects, pDevFindProperty );
        return;
    }

    hr = pDevGetObjectProperties( type, id, DevQueryFlagUpdateResults, 0, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( type, id, DevQueryFlagAsyncClose, 0, NULL, &buf_len, &buf );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( type, id, DevQueryFlagAsyncClose, 0, NULL, &buf_len, &buf );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( type, id, DevQueryFlagNone, 1, NULL, &buf_len, &buf );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( type, id, DevQueryFlagNone, 0, (DEVPROPCOMPKEY *)0xdeadbeef, &buf_len, &buf );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    buf = NULL;
    buf_len = 0;
    hr = pDevGetObjectProperties( type, id, DevQueryFlagAllProperties, 0, NULL, &buf_len, &buf );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( buf_len == props_len, "%lu != %lu\n", buf_len, props_len );
    for (i = 0; i < props_len; i++)
    {
        ULONG j;
        for (j = 0; j < buf_len; j++)
        {
            if (IsEqualDevPropKey( exp_props[i].CompKey.Key, buf[j].CompKey.Key ) && rem_props)
            {
                winetest_push_context( "%s", debugstr_DEVPROPKEY( &exp_props[i].CompKey.Key ) );
                /* ItemNameDisplay for software devices has different values for properties obtained from DevGetObjects
                 * and DevGetObjectProperties. */
                if (!IsEqualDevPropKey(PKEY_ItemNameDisplay, buf[j].CompKey.Key))
                {
                    const DEVPROPERTY *found_prop;

                    ok( dev_property_val_equal( &exp_props[i], &buf[j] ), "%s != %s\n", debugstr_DEVPROP_val( &buf[j] ),
                        debugstr_DEVPROP_val( &exp_props[i] ) );
                    found_prop = pDevFindProperty( &exp_props[i].CompKey.Key, DEVPROP_STORE_SYSTEM, NULL, buf_len, buf );
                    ok( found_prop == &buf[i], "got found_prop %p != %p\n", found_prop, &buf[i] );
                }
                winetest_pop_context();
                rem_props--;
            }
        }
    }
    ok( rem_props == 0, "got rem_props %lu\n", rem_props );
    pDevFreeObjectProperties( buf_len, buf );

    buf = (DEVPROPERTY *)0xdeadbeef;
    buf_len = 0xdeadbeef;
    rem_props = props_len;
    hr = pDevGetObjectProperties( type, id, DevQueryFlagNone, 0, NULL, &buf_len, &buf );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( buf_len == 0, "got buf_len %lu\n", buf_len );
    ok( !buf, "got buf %p\n", buf );

    buf = NULL;
    buf_len = 0;
    keys = calloc( props_len, sizeof( *keys ) );
    for (i = 0; i < props_len; i++)
        keys[i] = exp_props[i].CompKey;
    hr = pDevGetObjectProperties( type, id, DevQueryFlagNone, props_len, keys, &buf_len, &buf );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( buf_len == props_len, "%lu != %lu\n", buf_len, props_len );
    for (i = 0; i < props_len; i++)
    {
        ULONG j;
        for (j = 0; j < buf_len; j++)
        {
            if (IsEqualDevPropKey( exp_props[i].CompKey.Key, buf[j].CompKey.Key ))
            {
                const DEVPROPERTY *found_prop;

                winetest_push_context( "%s", debugstr_DEVPROPKEY( &exp_props[i].CompKey.Key ) );
                rem_props--;
                found_prop = pDevFindProperty( &exp_props[i].CompKey.Key, DEVPROP_STORE_SYSTEM, NULL, buf_len, buf );
                ok( found_prop == &buf[j], "got found_prop %p != %p\n", found_prop, &buf[j] );
                winetest_pop_context();
                break;
            }
        }
    }
    ok( rem_props == 0, "got rem_props %lu\n", rem_props );
    pDevFreeObjectProperties( buf_len, buf );

    buf_len = 0;
    buf = NULL;
    hr = pDevGetObjectProperties( type, id, DevQueryFlagNone, 1, &dummy_propcompkey, &buf_len, &buf );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !!buf, "got buf %p", buf );
    ok( buf_len == 1, "got buf_len %lu\n", buf_len );
    if (buf)
    {
        const DEVPROPERTY *found_prop;

        ok( IsEqualDevPropKey( buf[0].CompKey.Key, DEVPKEY_dummy ), "got propkey {%s, %#lx}\n",
            debugstr_guid( &buf[0].CompKey.Key.fmtid ), buf[0].CompKey.Key.pid );
        ok( buf[0].Type == DEVPROP_TYPE_EMPTY, "got Type %#lx\n", buf[0].Type );
        found_prop = pDevFindProperty( &DEVPKEY_dummy, DEVPROP_STORE_SYSTEM, NULL, buf_len, buf );
        ok( found_prop == &buf[0], "got found_prop %p != %p\n", found_prop, &buf[0] );
        pDevFreeObjectProperties( buf_len, buf );
    }
    free( keys );
}

static void test_dev_object_iface_props( int line, const DEV_OBJECT *obj, const struct test_property *exp_props,
                                         DWORD props_len )
{
    DWORD i, err, rem_props = props_len;
    HDEVINFO set;

    set = SetupDiCreateDeviceInfoListExW( NULL, NULL, NULL, NULL );
    ok_( __FILE__, line )( set != INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed: %lu\n",
                           GetLastError() );
    ok_( __FILE__, line )( obj->cPropertyCount >= props_len, "got cPropertyCount %lu, should be >= %lu\n",
                           obj->cPropertyCount, props_len );
    for (i = 0; i < obj->cPropertyCount && rem_props; i++)
    {
        const DEVPROPERTY *property = &obj->pProperties[i];
        ULONG j;

        for (j = 0; j < props_len; j++)
        {
            if (IsEqualDevPropKey( property->CompKey.Key, exp_props[j].key ))
            {
                SP_INTERFACE_DEVICE_DATA iface_data = {0};
                DEVPROPTYPE type = DEVPROP_TYPE_EMPTY;
                const DEVPROPERTY *found_prop;
                ULONG size = 0;
                CONFIGRET ret;
                BYTE *buf;

                winetest_push_context( "exp_props[%lu]", j );
                rem_props--;
                ok_( __FILE__, line )( property->Type == exp_props[j].type, "got type %#lx\n", property->Type );
                /* Ensure the value matches the value retrieved via SetupDiGetDeviceInterfacePropertyW */
                buf = calloc( property->BufferSize, 1 );
                iface_data.cbSize = sizeof( iface_data );
                ret = SetupDiOpenDeviceInterfaceW( set, obj->pszObjectId, 0, &iface_data );
                err = GetLastError();
                ok_( __FILE__, line )( ret || err == ERROR_NO_SUCH_DEVICE_INTERFACE, "SetupDiOpenDeviceInterfaceW failed: %lu\n", err );
                if (!ret)
                {
                    winetest_pop_context();
                    free( buf );
                    continue;
                }
                ret = SetupDiGetDeviceInterfacePropertyW( set, &iface_data, &property->CompKey.Key, &type, buf,
                                                          property->BufferSize, &size, 0 );
                ok_( __FILE__, line )( ret, "SetupDiGetDeviceInterfacePropertyW failed: %lu\n", GetLastError() );
                SetupDiDeleteDeviceInterfaceData( set, &iface_data );

                ok_( __FILE__, line )( size == property->BufferSize, "got size %lu\n", size );
                ok_( __FILE__, line )( type == property->Type, "got type %#lx\n", type );
                if (size == property->BufferSize)
                {
                    switch (type)
                    {
                    case DEVPROP_TYPE_STRING:
                        ok_( __FILE__, line )( !wcsicmp( (WCHAR *)buf, (WCHAR *)property->Buffer ),
                                               "got instance id %s != %s\n", debugstr_w( (WCHAR *)buf ),
                                               debugstr_w( (WCHAR *)property->Buffer ) );
                        break;
                    default:
                        ok_( __FILE__, line )( !memcmp( buf, property->Buffer, size ),
                                               "got mistmatching property values\n" );
                        break;
                    }
                }

                found_prop = pDevFindProperty( &property->CompKey.Key, DEVPROP_STORE_SYSTEM, NULL,
                                               obj->cPropertyCount, obj->pProperties );
                ok( found_prop == property, "got found_prop %p != %p\n", found_prop, property );
                free( buf );
                winetest_pop_context();
                break;
            }
        }
    }
    ok_( __FILE__, line )( rem_props == 0, "got rem %lu != 0\n", rem_props );
    SetupDiDestroyDeviceInfoList( set );
}

static void filter_add_props( DEVPROP_FILTER_EXPRESSION *filters, ULONG props_len, const DEVPROPERTY *props, BOOL equals )
{
    ULONG i;

    for (i = 0; i < props_len; i++)
    {
        filters[i].Operator = equals ? DEVPROP_OPERATOR_EQUALS : DEVPROP_OPERATOR_NOT_EQUALS;
        filters[i].Property = props[i];
    }
}

static void test_DevGetObjects( void )
{
    struct {
        DEV_OBJECT_TYPE object_type;
        struct test_property exp_props[3];
        ULONG props_len;
    } test_cases[] = {
        {
            DevObjectTypeDeviceInterface,
            {
                { DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_TYPE_GUID },
                { DEVPKEY_DeviceInterface_Enabled, DEVPROP_TYPE_BOOLEAN },
                { DEVPKEY_Device_InstanceId, DEVPROP_TYPE_STRING }
            },
            3,
        },
        {
            DevObjectTypeDeviceInterfaceDisplay,
            {
                { DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_TYPE_GUID },
                { DEVPKEY_DeviceInterface_Enabled, DEVPROP_TYPE_BOOLEAN },
                { DEVPKEY_Device_InstanceId, DEVPROP_TYPE_STRING }
            },
            3,
        },
    };
    static const DEVPROP_OPERATOR invalid_ops[] = {
        /* Logical operators, need to paired with their CLOSE counterpart. */
        DEVPROP_OPERATOR_AND_OPEN,
        DEVPROP_OPERATOR_AND_CLOSE,
        DEVPROP_OPERATOR_OR_OPEN,
        DEVPROP_OPERATOR_OR_CLOSE,
        DEVPROP_OPERATOR_NOT_OPEN,
        DEVPROP_OPERATOR_NOT_CLOSE,
        /* Mask value, cannot be used by itself in a filter. */
        DEVPROP_OPERATOR_MASK_LOGICAL,
        /* Non-existent operators */
        0xdeadbeef,
    };
    static const DEVPROP_OPERATOR boolean_open_ops[] = {
        DEVPROP_OPERATOR_AND_OPEN,
        DEVPROP_OPERATOR_OR_OPEN,
        DEVPROP_OPERATOR_NOT_OPEN,
    };
    /* The difference between CLOSE and OPEN operators is always 0x100000, this is just here for clariy's sake. */
    static const DEVPROP_OPERATOR boolean_close_ops[] = {
        DEVPROP_OPERATOR_AND_CLOSE,
        DEVPROP_OPERATOR_OR_CLOSE,
        DEVPROP_OPERATOR_NOT_CLOSE,
    };
    CHAR bool_val_extra[] = { DEVPROP_TRUE, 0xde, 0xad, 0xbe, 0xef };
    DEVPROP_BOOLEAN bool_val = DEVPROP_TRUE;
    const DEVPROP_FILTER_EXPRESSION valid_filter = {
        DEVPROP_OPERATOR_EQUALS,
        {
            { DEVPKEY_DeviceInterface_Enabled, DEVPROP_STORE_SYSTEM, NULL },
            DEVPROP_TYPE_BOOLEAN,
            sizeof( bool_val ),
            &bool_val,
        }
    };
    DEVPROP_FILTER_EXPRESSION filter_instance_id_exists = {
        DEVPROP_OPERATOR_NOT_EQUALS, { { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_EMPTY, 0, NULL }
    };
    DEVPROP_FILTER_EXPRESSION filter_instance_id_not_exists = {
        DEVPROP_OPERATOR_EQUALS, { { DEVPKEY_Device_InstanceId, DEVPROP_STORE_SYSTEM, NULL }, DEVPROP_TYPE_EMPTY, 0, NULL }
    };
    /* Test cases for filter expressions containing boolean operators. All of these filters are logically equivalent
     * and should return identical objects */
    struct {
        DEVPROP_FILTER_EXPRESSION expr[12];
        SIZE_T size;
    } logical_op_test_cases[] = {
        {
            /* NOT (System.Devices.InstanceId := []) */
            { { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE } },
            3
        },
        {
            /* NOT (NOT (System.Device.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN }, { DEVPROP_OPERATOR_NOT_OPEN },
              filter_instance_id_exists,
              { DEVPROP_OPERATOR_NOT_CLOSE }, { DEVPROP_OPERATOR_NOT_CLOSE } },
              5
        },
        {
            /* NOT ((System.Device.InstanceId := []) AND (System.Device.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_not_exists, filter_instance_id_exists,{ DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE } },
              6
        },
        {
            /* NOT ((NOT (System.Device.InstanceId :- [])) AND (System.Device.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              filter_instance_id_exists,
              { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE } },
              8
        },
        {
            /* ((System.Device.InstanceId :- []) OR (System.Device.InstanceId := [])) */
            { { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_exists, filter_instance_id_not_exists, { DEVPROP_OPERATOR_OR_CLOSE } },
            4
        },
        {
            /* ((System.Devices.InstanceId :- [] AND System.Devices.InstanceId :- []) OR System.Device.InstanceId := []) */
            { { DEVPROP_OPERATOR_OR_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_exists, filter_instance_id_exists, { DEVPROP_OPERATOR_AND_CLOSE },
              filter_instance_id_not_exists,
              { DEVPROP_OPERATOR_OR_CLOSE } },
            7
        },
        {
            /* ((System.Devices.InstanceId :- [] AND System.Devices.InstanceId :- []) AND System.Device.InstanceId :- []) */
            { { DEVPROP_OPERATOR_AND_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_exists, filter_instance_id_exists, { DEVPROP_OPERATOR_AND_CLOSE },
              filter_instance_id_exists,
              { DEVPROP_OPERATOR_AND_CLOSE } },
            7
        },
        {
            /* (System.Device.InstanceId :- [] AND (System.Devices.InstanceId :- [] AND System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_AND_OPEN },
            filter_instance_id_exists,
            { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_exists, filter_instance_id_exists, { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_AND_CLOSE } },
            7
        },
        {
            /* ((System.Devices.InstanceId := [] OR System.Devices.InstanceId := []) OR System.Device.InstanceId :- []) */
            { { DEVPROP_OPERATOR_OR_OPEN },
              { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists, filter_instance_id_not_exists, { DEVPROP_OPERATOR_OR_CLOSE },
              filter_instance_id_exists,
              { DEVPROP_OPERATOR_OR_CLOSE } },
            7
        },
        {
            /* (System.Device.InstanceId := [] OR (System.Devices.InstanceId := [] OR System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_OR_OPEN },
            filter_instance_id_not_exists,
            { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists, filter_instance_id_exists, { DEVPROP_OPERATOR_OR_CLOSE },
              { DEVPROP_OPERATOR_OR_CLOSE } },
            7
        },
        /* Logical operators with implicit AND. */
        {
            /* (NOT (System.Device.InstanceId := [])) System.Device.InstanceId :- [] */
            { { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              filter_instance_id_exists },
            4
        },
        {
            /* (NOT (System.Device.InstanceId := [])) (NOT (System.Device.InstanceId := [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE } },
            6
        },
        {
            /* (NOT (System.Device.InstanceId :- [] AND System.Device.InstanceId := [])) System.Device.InstanceId :- [] */
            { { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_exists, filter_instance_id_not_exists, { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE },
              filter_instance_id_exists },
            7
        },
        {
            /* (NOT (System.Device.InstanceId := [] OR System.Device.InstanceId := [])) System.Device.InstanceId :- [] */
            { { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists, filter_instance_id_not_exists, { DEVPROP_OPERATOR_OR_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE },
              filter_instance_id_exists },
            7
        },
        {
            /* (NOT (System.Device.InstanceId := [] OR System.Device.InstanceId := [])) (System.Device.InstanceId := [] OR System.Device.InstanceId :- []) */
            { { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists, filter_instance_id_not_exists, { DEVPROP_OPERATOR_OR_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists, filter_instance_id_exists, { DEVPROP_OPERATOR_OR_CLOSE } },
            10
        },
        {
            /* (NOT (System.Device.InstanceId := [] OR System.Device.InstanceId := [])) (NOT (System.Device.InstanceId :- [] AND System.Device.InstanceId := [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists, filter_instance_id_not_exists, { DEVPROP_OPERATOR_OR_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_exists, filter_instance_id_not_exists, { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE } },
            12
        },
        {
            /* ((System.Device.InstanceId := [] AND System.Device.InstanceId := []) OR (Syste.Device.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_OR_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_not_exists,
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_CLOSE },
            },
            11
        }
    };
    /* Filters that return empty results */
    struct {
        DEVPROP_FILTER_EXPRESSION expr[14];
        SIZE_T size;
    } logical_op_empty_test_cases[] = {
        {
            /* (NOT (System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE } },
            3,
        },
        {
            /* (NOT (System.Devices.InstanceId :- [])) System.Devices.InstanceId := [] */
            { { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              filter_instance_id_not_exists },
            4,
        },
        {
            /* (NOT (System.Devices.InstanceId :- [])) (NOT (System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE } },
            6,
        },
        {
            /* (NOT (System.Devices.InstanceId :- [])) (NOT (System.Devices.InstanceId :- [] AND System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_exists, filter_instance_id_exists, { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE } },
            9,
        },
        {
            /* NOT (NOT (System.Devices.InstanceId := [] AND System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_NOT_OPEN }, { DEVPROP_OPERATOR_NOT_OPEN },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_not_exists, filter_instance_id_exists, { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_NOT_CLOSE }, { DEVPROP_OPERATOR_NOT_CLOSE } },
            8,
        },
        {
            /* (System.Devices.InstanceId := [] OR System.Devices.InstanceId := []) */
            { { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists, filter_instance_id_not_exists, { DEVPROP_OPERATOR_OR_CLOSE } },
            4,
        },
        {
            /* (NOT System.Devices.InstanceId :- []) OR (System.Devices.InstanceId := [] OR (NOT System.Devices.InstanceId :- []) OR (NOT System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_OR_OPEN },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists,
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_CLOSE } },
            14
        },
        {
            /* (NOT System.Devices.InstanceId :- []) OR (NOT System.Devices.InstanceId :- []) OR (System.Devices.InstanceId := [] OR (NOT System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_OR_OPEN },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists,
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_CLOSE },
              { DEVPROP_OPERATOR_OR_CLOSE } },
            14
        },
        {
            /* (NOT System.Devices.InstanceId :- []) OR (NOT System.Devices.InstanceId :- []) OR (System.Devices.InstanceId :- [] AND (NOT System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_OR_OPEN },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_exists,
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_OR_CLOSE } },
            14
        },
        {
            /* (NOT System.Devices.InstanceId := []) AND (NOT System.Devices.InstanceId := []) AND (System.Devices.InstanceId :- [] AND (NOT System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_AND_OPEN },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_AND_OPEN }, filter_instance_id_exists,
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_AND_CLOSE },
              { DEVPROP_OPERATOR_AND_CLOSE } },
            14
        },
        {
            /* (NOT System.Devices.InstanceId := []) AND (NOT System.Devices.InstanceId := []) AND (System.Devices.InstanceId := [] OR (NOT System.Devices.InstanceId :- [])) */
            { { DEVPROP_OPERATOR_AND_OPEN },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_not_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_OPEN }, filter_instance_id_not_exists,
              { DEVPROP_OPERATOR_NOT_OPEN }, filter_instance_id_exists, { DEVPROP_OPERATOR_NOT_CLOSE },
              { DEVPROP_OPERATOR_OR_CLOSE },
              { DEVPROP_OPERATOR_AND_CLOSE } },
            14
        }
    };
    DEVPROPCOMPKEY prop_iface_class = { DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_STORE_SYSTEM, NULL };
    DEVPROP_FILTER_EXPRESSION filters[4];
    const DEV_OBJECT *objects = NULL;
    DEVPROPCOMPKEY prop_key = {0};
    DEVPROPCOMPKEY props[2];
    HRESULT hr;
    ULONG i, len = 0;

    if (!pDevGetObjects || !pDevFreeObjects || !pDevFindProperty)
    {
        win_skip("Functions unavailable, skipping test. (%p %p %p)\n", pDevGetObjects, pDevFreeObjects, pDevFindProperty);
        return;
    }

    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 1, NULL, 0, NULL, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, NULL, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, (void *)0xdeadbeef, 0, NULL, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 0, (void *)0xdeadbeef, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagUpdateResults, 0, NULL, 0, (void *)0xdeadbeef, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAsyncClose, 0, NULL, 0, (void *)0xdeadbeef, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjects( DevObjectTypeDeviceInterface, 0xdeadbeef, 0, NULL, 0, (void *)0xdeadbeef, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    prop_key.Key = test_cases[0].exp_props[0].key;
    prop_key.Store = DEVPROP_STORE_SYSTEM;
    prop_key.LocaleName = NULL;
    /* DevQueryFlagAllProperties is mutually exlusive with requesting specific properties. */
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 1, &prop_key, 0, NULL, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    hr = pDevGetObjects( DevObjectTypeUnknown, DevQueryFlagNone, 0, NULL, 0, NULL, &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len == 0, "got len %lu\n", len );
    ok( !objects, "got objects %p\n", objects );

    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    hr = pDevGetObjects( 0xdeadbeef, DevQueryFlagNone, 0, NULL, 0, NULL, &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len == 0, "got len %lu\n", len );
    ok( !objects, "got objects %p\n", objects );

    /* Filter expressions */
    memset( filters, 0, sizeof( filters ) );
    filters[0] = valid_filter;

    /* Invalid buffer value */
    filters[0].Property.Buffer = NULL;
    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 1, &filters[0], &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
    /* Filters are validated before len and objects are modified. */
    ok( len == 0xdeadbeef, "got len %lu\n", len );
    ok( objects == (DEV_OBJECT *)0xdeadbeef, "got objects %p\n", objects );

    /* Mismatching BufferSize */
    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    filters[0].Property.Buffer = &bool_val;
    filters[0].Property.BufferSize = 0;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 1, &filters[0], &len, &objects );
    /* BufferSize is not validated in Windows 10 and before, but no objects are returned. */
    ok( hr == E_INVALIDARG || broken( hr == S_OK ), "got hr %#lx\n", hr );
    ok( len == 0xdeadbeef || broken( !len ), "got len %lu\n", len );
    ok( objects == (DEV_OBJECT *)0xdeadbeef || broken( !objects ), "got objects %p\n", objects );

    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    filters[0].Property.Buffer = bool_val_extra;
    filters[0].Property.BufferSize = sizeof( bool_val_extra );
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 1, &filters[0], &len, &objects );
    /* The extra bytes are ignored in Windows 10 and before. */
    ok( hr == E_INVALIDARG || broken( hr == S_OK ), "got hr %#lx\n", hr );
    ok( len == 0xdeadbeef || broken( len ), "got len %lu\n", len );
    ok( objects == (DEV_OBJECT *)0xdeadbeef || broken( !!objects ), "got objects %p\n", objects );
    if (SUCCEEDED( hr )) pDevFreeObjects( len, objects );

    for (i = 0; i < ARRAY_SIZE( invalid_ops ); i++)
    {
        winetest_push_context( "invalid_ops[%lu]", i );
        filters[0].Operator = invalid_ops[i];

        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 1, filters, &len, &objects );
        ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 2, filters, &len, &objects );
        ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

        winetest_pop_context();
    }

    memset( &filters[0], 0, sizeof( *filters ) );
    /* MSDN says this is not a valid operator. However, using this does not fail, but the returned object list is empty. */
    filters[0].Operator = DEVPROP_OPERATOR_NONE;
    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 1, &filters[0], &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !len, "got len %lu\n", len );
    ok( !objects, "got objects %p\n", objects );

    filters[1] = valid_filter;
    /* DEVPROP_OPERATOR_NONE preceeding the next filter expression has the same result. */
    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 2, filters, &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( !len, "got len %lu\n", len );
    ok( !objects, "got objects %p\n", objects );

    /* However, filter expressions are still validated. */
    filters[1].Property.Buffer = NULL;
    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 2, filters, &len, &objects );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
    ok( len == 0xdeadbeef, "got len %lu\n", len );
    ok( objects == (DEV_OBJECT *)0xdeadbeef, "got objects %p\n", objects );

    filters[0] = valid_filter;
    /* DEVPROP_OPERATOR_EXISTS ignores the property type. */
    len = 0;
    objects = NULL;
    filters[0].Operator = DEVPROP_OPERATOR_EXISTS;
    filters[0].Property.Type = DEVPROP_TYPE_GUID;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 1, &filters[0], &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len, "got len %lu\n", len );
    pDevFreeObjects( len, objects );

    /* Don't fetch any properties, but still use them in the filter. */
    filters[0] = valid_filter;
    len = 0;
    objects = NULL;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, &filters[0], &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len, "got len %lu\n", len );
    for (i = 0; i < len; i++)
    {
        /* No properties should have been fetched. */
        winetest_push_context( "object %s", debugstr_w( objects[i].pszObjectId ) );
        ok( !objects[i].cPropertyCount, "got cPropertyCount %lu\n", objects[i].cPropertyCount );
        ok( !objects[i].pProperties, "got pProperties %p\n", objects[i].pProperties );
        winetest_pop_context();
    }
    pDevFreeObjects( len, objects );

    /* Request and filter different properties, make sure we *only* get the properties we requested. */
    len = 0;
    objects = NULL;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 1, &prop_iface_class, 1, &filters[0], &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len, "got len %lu\n", len );
    for (i = 0; i < len; i++)
    {
        const DEVPROPERTY *prop;

        winetest_push_context( "object %s", debugstr_w( objects[i].pszObjectId ) );
        ok( objects[i].cPropertyCount == 1, "got cPropertyCount %lu\n", objects[i].cPropertyCount );
        prop = pDevFindProperty( &prop_iface_class.Key, prop_iface_class.Store, prop_iface_class.LocaleName,
                                 objects[i].cPropertyCount, objects[i].pProperties );
        ok (!!prop, "got prop %p\n", prop );
        winetest_pop_context();
    }
    pDevFreeObjects( len, objects );

    /* DevGetObjects will not de-duplicate properties. */
    len = 0;
    objects = NULL;
    props[0] = prop_iface_class;
    props[1] = prop_iface_class;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 2, props, 0, NULL, &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len, "got len %lu\n", len );
    for (i = 0; i < len; i++)
    {
        const DEVPROPERTY *props = objects[i].pProperties;

        winetest_push_context( "object %s", debugstr_w( objects[i].pszObjectId ) );
        ok( objects[i].cPropertyCount == 2, "got cPropertyCount %lu\n", objects[i].cPropertyCount );
        ok( IsEqualDevPropCompKey( props[0].CompKey, prop_iface_class ), "got props[0].CompKey %s\n",
            debugstr_DEVPROPCOMPKEY( &props[0].CompKey ) );
        ok( IsEqualDevPropCompKey( props[1].CompKey, prop_iface_class ), "got props[1].CompKey %s\n",
            debugstr_DEVPROPCOMPKEY( &props[1].CompKey ) );
        winetest_pop_context();
    }
    pDevFreeObjects( len, objects );

    /* AND/OR with a single expression */
    memset( filters, 0, sizeof( filters ) );
    filters[0].Operator = DEVPROP_OPERATOR_AND_OPEN;
    filters[1] = valid_filter;
    filters[2].Operator = DEVPROP_OPERATOR_AND_CLOSE;
    len = 0;
    objects = NULL;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 3, filters, &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len, "got len %lu\n", len );
    for (i = 0; i < len; i++)
    {
        const DEVPROPERTY *prop;

        prop = pDevFindProperty( &valid_filter.Property.CompKey.Key, valid_filter.Property.CompKey.Store,
                                 valid_filter.Property.CompKey.LocaleName, objects[i].cPropertyCount,
                                 objects[i].pProperties );
        ok( !!prop, "got prop %p\n", prop );
    }
    pDevFreeObjects( len, objects );

    filters[0].Operator = DEVPROP_OPERATOR_OR_OPEN;
    filters[2].Operator = DEVPROP_OPERATOR_OR_CLOSE;
    len = 0;
    objects = NULL;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties, 0, NULL, 3, filters, &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len, "got len %lu\n", len );
    for (i = 0; i < len; i++)
    {
        const DEVPROPERTY *prop;

        prop = pDevFindProperty( &valid_filter.Property.CompKey.Key, valid_filter.Property.CompKey.Store,
                                 valid_filter.Property.CompKey.LocaleName, objects[i].cPropertyCount,
                                 objects[i].pProperties );
        ok( !!prop, "got prop %p\n", prop );
    }
    pDevFreeObjects( len, objects );

    memset(filters, 0, sizeof( filters ) );
    filters[0] = valid_filter;
    filters[0].Operator = DEVPROP_OPERATOR_NOT_EXISTS;
    /* All device interfaces have this property, so this should not return any objects. */
    len = 0xdeadbeef;
    objects = (DEV_OBJECT *)0xdeadbeef;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, &filters[0], &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len == 0, "got len %lu\n", len );
    ok( !objects, "got objects %p\n", objects );

    /* Empty expressions */
    memset( filters, 0, sizeof( filters ) );
    for (i = 0; i < ARRAY_SIZE( boolean_open_ops ); i++)
    {
        DEVPROP_OPERATOR open = boolean_open_ops[i], close = boolean_close_ops[i];

        winetest_push_context( "open=%#x, close=%#x", open, close );

        filters[0].Operator = open;
        filters[1].Operator = close;
        len = 0xdeadbeef;
        objects = (DEV_OBJECT *)0xdeadbeef;
        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 2, filters, &len, &objects );
        ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
        ok( len == 0xdeadbeef, "got len %lu\n", len );
        ok( objects == (DEV_OBJECT *)0xdeadbeef, "got objects %p\n", objects );

        /* Empty nested expressions */
        filters[0].Operator = filters[1].Operator = open;
        filters[2].Operator = filters[3].Operator = close;
        len = 0xdeadbeef;
        objects = (DEV_OBJECT *)0xdeadbeef;
        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 4, filters, &len, &objects );
        ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
        ok( len == 0xdeadbeef, "got len %lu\n", len );
        ok( objects == (DEV_OBJECT *)0xdeadbeef, "got objects %p\n", objects );

        winetest_pop_context();
    }

    memset( filters, 0, sizeof( filters ) );
    filters[1] = valid_filter;
    /* Improperly paired expressions */
    for (i = 0; i < ARRAY_SIZE( boolean_open_ops ); i++)
    {
        DEVPROP_OPERATOR open = boolean_open_ops[i];
        ULONG j;

        for (j = 0; j < ARRAY_SIZE( boolean_close_ops ) && i != j; j++)
        {
            DEVPROP_OPERATOR close = boolean_close_ops[j];

            winetest_push_context( "open=%#x, close=%#x", open, close );

            filters[0].Operator = open;
            filters[2].Operator = close;
            len = 0xdeadbeef;
            objects = (DEV_OBJECT *)0xdeadbeef;
            hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 3, filters, &len, &objects );
            ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
            ok( len == 0xdeadbeef, "got len %lu\n", len );
            ok( objects == (DEV_OBJECT *)0xdeadbeef, "got objects %p\n", objects );

            winetest_pop_context();
        }
    }

    memset( filters, 0, sizeof( filters ) );
    for (i = DEVPROP_OPERATOR_GREATER_THAN; i <= DEVPROP_OPERATOR_LESS_THAN_EQUALS; i++)
    {
        GUID guid = {0};

        winetest_push_context( "op=%#08lx", i );

        /* Use the less/greater than operators with GUIDs will never result in a match. */
        filters[0].Operator = i;
        filters[0].Property.CompKey.Key = DEVPKEY_DeviceInterface_ClassGuid;
        filters[0].Property.Buffer = &guid;
        filters[0].Property.BufferSize = sizeof( guid );
        filters[0].Property.Type = DEVPROP_TYPE_GUID;
        len = 0xdeadbeef;
        objects = (DEV_OBJECT *)0xdeadbeef;
        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, filters, &len, &objects );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( !len, "got len %lu\n", len );
        ok( !objects, "got objects %p\n", objects );
        if (objects) pDevFreeObjects( len, objects );

        /* Consequently, using the DEVPROP_OPERATOR_MODIFIER_NOT modifier will always match. */
        filters[0].Operator |= DEVPROP_OPERATOR_MODIFIER_NOT;
        len = 0;
        objects = NULL;
        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, filters, &len, &objects );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( len > 0, "got len %lu\n", len );
        ok( !!objects, "got objects %p\n", objects );
        pDevFreeObjects( len, objects );

        /* Make sure we get the same results with the max GUID value as well. */
        memset( &guid, 0xff, sizeof( guid ) );
        filters[0].Operator = i;
        len = 0xdeadbeef;
        objects = (DEV_OBJECT *)0xdeadbeef;
        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, filters, &len, &objects );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( !len, "got len %lu\n", len );
        ok( !objects, "got objects %p\n", objects );
        if (objects) pDevFreeObjects( len, objects );

        filters[0].Operator |= DEVPROP_OPERATOR_MODIFIER_NOT;
        len = 0;
        objects = NULL;
        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, filters, &len, &objects );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( len > 0, "got len %lu\n", len );
        ok( !!objects, "got objects %p\n", objects );
        pDevFreeObjects( len, objects );

        winetest_pop_context();
    }

    memset( filters, 0, sizeof( filters ) );
    filters[0] = valid_filter;
    filters[0].Operator = DEVPROP_OPERATOR_NOT_EQUALS;
    bool_val = FALSE;
    len = 0;
    objects = NULL;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, filters, &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len > 0, "got len %lu\n", len );
    ok( !!objects, "got objects %p\n", objects );
    pDevFreeObjects( len, objects );
    bool_val = TRUE;

    /* Get the number of objects that the filters in logical_op_test_cases should return. */
    filters[0] = filter_instance_id_exists;
    len = 0;
    objects = NULL;
    hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, 1, filters, &len, &objects );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ok( len > 0, "got len %lu\n", len );
    ok( !!objects, "got objects %p\n", objects );
    pDevFreeObjects( len, objects );

    for (i = 0; i < ARRAY_SIZE( logical_op_test_cases ); i++ )
    {
        ULONG len2 = 0;

        winetest_push_context( "logical_op_test_cases[%lu]", i );

        objects = NULL;
        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL, logical_op_test_cases[i].size,
                             logical_op_test_cases[i].expr, &len2, &objects );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( len2 == len, "got len2 %lu != %lu\n", len2, len );
        ok( !!objects, "got objects %p\n", objects );
        pDevFreeObjects( len2, objects );

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE ( logical_op_empty_test_cases ); i++ )
    {
        winetest_push_context( "logical_op_empty_test_cases[%lu]", i );

        len = 0xdeadbeef;
        objects = (DEV_OBJECT *)0xdeadbeef;
        hr = pDevGetObjects( DevObjectTypeDeviceInterface, DevQueryFlagNone, 0, NULL,
                             logical_op_empty_test_cases[i].size, logical_op_empty_test_cases[i].expr, &len, &objects );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( !len, "got len %lu\n", len );
        ok( !objects, "got objects %p\n", objects );

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE( test_cases ); i++)
    {
        const DEV_OBJECT *objects = NULL;
        ULONG j, len = 0;

        /* Get all objects of this type, with all properties. */
        objects = NULL;
        len = 0;
        winetest_push_context( "test_cases[%lu]", i );
        hr = pDevGetObjects( test_cases[i].object_type, DevQueryFlagAllProperties, 0, NULL, 0, NULL, &len, &objects );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        for (j = 0; j < len; j++)
        {
            DEVPROP_FILTER_EXPRESSION *filters;
            const DEV_OBJECT *obj = &objects[j], *objects2;
            ULONG k, len2 = 0;
            BOOL found = FALSE;

            winetest_push_context( "device %s", debugstr_w( obj->pszObjectId ) );
            ok( obj->ObjectType == test_cases[i].object_type, "got ObjectType %d\n", obj->ObjectType );
            test_dev_object_iface_props( __LINE__, obj, test_cases[i].exp_props, test_cases[i].props_len );
            winetest_push_context( "%d", __LINE__ );
            test_DevGetObjectProperties( obj->ObjectType, obj->pszObjectId, obj->pProperties, obj->cPropertyCount );
            winetest_pop_context();

            /* Create a filter for all properties of this object. */
            filters = calloc( obj->cPropertyCount, sizeof( *filters ) );
            /* If there are no logical operators present, then logical AND is used. */
            filter_add_props( filters, obj->cPropertyCount, obj->pProperties, TRUE );
            hr = pDevGetObjects( test_cases[i].object_type, DevQueryFlagAllProperties, 0, NULL, obj->cPropertyCount,
                                filters, &len2, &objects2 );
            ok( hr == S_OK, "got hr %#lx\n", hr );
            /* For device interface objects, DEVPKEY_Device_Instance and DEVPKEY_DeviceInterface_ClassGuid are a
             * unique pair, so there should only be one object. */
            if (test_cases[i].object_type == DevObjectTypeDeviceInterface
                || test_cases[i].object_type == DevObjectTypeDeviceInterfaceDisplay)
                ok( len2 == 1, "got len2 %lu\n", len2 );
            else
                ok( len2, "got len2 %lu\n", len2 );
            for (k = 0; k < len2; k++)
            {
                if (!wcsicmp( objects2[k].pszObjectId, obj->pszObjectId ))
                {
                    found = TRUE;
                    break;
                }
            }
            ok( found, "failed to get object using query filters\n" );
            pDevFreeObjects( len2, objects2 );
            free( filters );
            winetest_pop_context();
        }
        pDevFreeObjects( len, objects );

        /* Get all objects of this type, but only with a single requested property. */
        for (j = 0; j < test_cases[i].props_len; j++)
        {
            const struct test_property *prop = &test_cases[i].exp_props[j];
            ULONG k;

            winetest_push_context( "exp_props[%lu]", j );
            objects = NULL;
            len = 0;
            prop_key.Key = prop->key;
            prop_key.LocaleName = NULL;
            prop_key.Store = DEVPROP_STORE_SYSTEM;
            hr = pDevGetObjects( test_cases[i].object_type, 0, 1, &prop_key, 0, NULL, &len, &objects );
            ok( hr == S_OK, "got hr %#lx\n", hr );
            ok( len, "got buf_len %lu\n", len );
            ok( !!objects, "got objects %p\n", objects );
            for (k = 0; k < len; k++)
            {
                const DEV_OBJECT *obj = &objects[k];
                const DEVPROPERTY *found_prop;

                winetest_push_context( "objects[%lu]", k );
                ok( obj->cPropertyCount == 1, "got cPropertyCount %lu != 1\n", obj->cPropertyCount );
                ok( !!obj->pProperties, "got pProperties %p\n", obj->pProperties );
                if (obj->pProperties)
                {
                    ok( IsEqualDevPropKey( obj->pProperties[0].CompKey.Key, prop->key ), "got property {%s, %#lx} != {%s, %#lx}\n",
                        debugstr_guid( &obj->pProperties[0].CompKey.Key.fmtid ), obj->pProperties[0].CompKey.Key.pid,
                        debugstr_guid( &prop->key.fmtid ), prop->key.pid );
                    found_prop = pDevFindProperty( &prop->key, DEVPROP_STORE_SYSTEM, NULL, obj->cPropertyCount, obj->pProperties );
                    ok( found_prop == &obj->pProperties[0], "got found_prop %p != %p\n", found_prop, &obj->pProperties[0] );
                }
                /* Search for a property not in obj->pProperties, we should get NULL, as we haven't requested this
                 * property in the DevGetObjects call. */
                found_prop = pDevFindProperty( &DEVPKEY_dummy, DEVPROP_STORE_SYSTEM, NULL, obj->cPropertyCount, obj->pProperties );
                ok( !found_prop, "got found_prop %p\n", found_prop );

                winetest_pop_context();
            }
            pDevFreeObjects( len, objects );
            winetest_pop_context();
        }

        /* Get all objects of this type, but with a non existent property. The returned objects will still have this
         * property, albeit with Type set to DEVPROP_TYPE_EMPTY. */
        len = 0;
        objects = NULL;
        prop_key.Key = DEVPKEY_dummy;
        hr = pDevGetObjects( test_cases[i].object_type, 0, 1, &prop_key, 0, NULL, &len, &objects );
        ok( hr == S_OK, "got hr %#lx\n", hr );
        ok( len, "got len %lu\n", len );
        ok( !!objects, "got objects %p\n", objects );
        for (j = 0; j < len; j++)
        {
            const DEV_OBJECT *obj = &objects[j];

            winetest_push_context( "objects[%lu]", j );
            ok( obj->cPropertyCount == 1, "got cPropertyCount %lu != 1\n", obj->cPropertyCount );
            ok( !!obj->pProperties, "got pProperties %p\n", obj->pProperties );
            if (obj->pProperties)
            {
                const DEVPROPERTY *found_prop;

                ok( IsEqualDevPropKey( obj->pProperties[0].CompKey.Key, DEVPKEY_dummy ), "got property %s != %s\n",
                    debugstr_DEVPROPKEY( &obj->pProperties[0].CompKey.Key ), debugstr_DEVPROPKEY( &DEVPKEY_dummy ) );
                ok( obj->pProperties[0].Type == DEVPROP_TYPE_EMPTY, "got Type %#lx != %#x", obj->pProperties[0].Type,
                    DEVPROP_TYPE_EMPTY );
                found_prop = pDevFindProperty( &DEVPKEY_dummy, DEVPROP_STORE_SYSTEM, NULL, obj->cPropertyCount, obj->pProperties );
                ok( found_prop == &obj->pProperties[0], "got found_prop %p != %p\n", found_prop, &obj->pProperties[0] );
            }
            winetest_pop_context();
        }
        winetest_pop_context();
        pDevFreeObjects( len, objects );
    }
}

struct query_callback_data
{
    int line;
    DEV_OBJECT_TYPE exp_type;
    const struct test_property *exp_props;
    DWORD props_len;

    HANDLE enum_completed;
    HANDLE closed;
};

static void WINAPI query_result_callback( HDEVQUERY query, void *user_data, const DEV_QUERY_RESULT_ACTION_DATA *action_data )
{
    struct query_callback_data *data = user_data;

    ok( !!data, "got null user_data\n" );
    if (!data) return;

    switch (action_data->Action)
    {
    case DevQueryResultStateChange:
    {
        DEV_QUERY_STATE state = action_data->Data.State;
        ok( state == DevQueryStateEnumCompleted || state == DevQueryStateClosed,
            "got unexpected Data.State value: %d\n", state );
        switch (state)
        {
        case DevQueryStateEnumCompleted:
            SetEvent( data->enum_completed );
            break;
        case DevQueryStateClosed:
            SetEvent( data->closed );
        default:
            break;
        }
        break;
    }
    case DevQueryResultAdd:
    {
        const DEV_OBJECT *obj = &action_data->Data.DeviceObject;
        winetest_push_context( "device %s", debugstr_w( obj->pszObjectId ) );
        ok_( __FILE__, data->line )( obj->ObjectType == data->exp_type, "got DeviceObject.ObjectType %d != %d",
                                     obj->ObjectType, data->exp_type );
        test_dev_object_iface_props( data->line, &action_data->Data.DeviceObject, data->exp_props, data->props_len );
        winetest_pop_context();
        break;
    }
    default:
        ok( action_data->Action == DevQueryResultUpdate || action_data->Action == DevQueryResultRemove,
            "got unexpected Action %d\n", action_data->Action );
        break;
    }
}

#define call_DevCreateObjectQuery( a, b, c, d, e, f, g, h, i ) \
    call_DevCreateObjectQuery_(__LINE__, (a), (b), (c), (d), (e), (f), (g), (h), (i))

static HRESULT call_DevCreateObjectQuery_( int line, DEV_OBJECT_TYPE type, ULONG flags, ULONG props_len,
                                           const DEVPROPCOMPKEY *props, ULONG filters_len,
                                           const DEVPROP_FILTER_EXPRESSION *filters, PDEV_QUERY_RESULT_CALLBACK callback,
                                           struct query_callback_data *data, HDEVQUERY *devquery )
{
    data->line = line;
    return pDevCreateObjectQuery( type, flags, props_len, props, filters_len, filters, callback, data, devquery );
}

static void test_DevCreateObjectQuery( void )
{
    struct test_property iface_props[3] = {
        { DEVPKEY_DeviceInterface_ClassGuid, DEVPROP_TYPE_GUID },
        { DEVPKEY_DeviceInterface_Enabled, DEVPROP_TYPE_BOOLEAN },
        { DEVPKEY_Device_InstanceId, DEVPROP_TYPE_STRING }
    };
    struct query_callback_data data = {0};
    HDEVQUERY query = NULL;
    HRESULT hr;
    DWORD ret;

    if (!pDevCreateObjectQuery || !pDevCloseObjectQuery)
    {
        win_skip("Functions unavailable, skipping test. (%p %p)\n", pDevCreateObjectQuery, pDevCloseObjectQuery);
        return;
    }

    hr = pDevCreateObjectQuery( DevObjectTypeDeviceInterface, 0, 0, NULL, 0, NULL, NULL, NULL, &query );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
    ok( !query, "got query %p\n", query );

    hr = pDevCreateObjectQuery( DevObjectTypeDeviceInterface, 0xdeadbeef, 0, NULL, 0, NULL, query_result_callback,
                               NULL, &query );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
    ok( !query, "got query %p\n", query );

    data.enum_completed = CreateEventW( NULL, FALSE, FALSE, NULL );
    data.closed = CreateEventW( NULL, FALSE, FALSE, NULL );

    hr = call_DevCreateObjectQuery( DevObjectTypeUnknown, 0, 0, NULL, 0, NULL, &query_result_callback, &data, &query );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ret = WaitForSingleObject( data.enum_completed, 1000 );
    ok( !ret, "got ret %lu\n", ret );
    pDevCloseObjectQuery( query );

    hr = call_DevCreateObjectQuery( 0xdeadbeef, 0, 0, NULL, 0, NULL, &query_result_callback, &data, &query );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ret = WaitForSingleObject( data.enum_completed, 1000 );
    ok( !ret, "got ret %lu\n", ret );
    pDevCloseObjectQuery( query );

    hr = call_DevCreateObjectQuery( DevObjectTypeUnknown, DevQueryFlagAsyncClose, 0, NULL, 0, NULL, &query_result_callback,
                                    &data, &query );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ret = WaitForSingleObject( data.enum_completed, 1000 );
    ok( !ret, "got ret %lu\n", ret );
    pDevCloseObjectQuery( query );
    ret = WaitForSingleObject( data.closed, 1000 );
    ok( !ret, "got ret %lu\n", ret );

    data.exp_props = iface_props;
    data.props_len = ARRAY_SIZE( iface_props );

    data.exp_type = DevObjectTypeDeviceInterface;
    hr = call_DevCreateObjectQuery( DevObjectTypeDeviceInterface, DevQueryFlagAllProperties | DevQueryFlagAsyncClose, 0,
                                    NULL, 0, NULL, &query_result_callback, &data, &query );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ret = WaitForSingleObject( data.enum_completed, 5000 );
    ok( !ret, "got ret %lu\n", ret );
    pDevCloseObjectQuery( query );
    ret = WaitForSingleObject( data.closed, 1000 );
    ok( !ret, "got ret %lu\n", ret );

    data.exp_type = DevObjectTypeDeviceInterfaceDisplay;
    hr = call_DevCreateObjectQuery( DevObjectTypeDeviceInterfaceDisplay, DevQueryFlagAllProperties | DevQueryFlagAsyncClose,
                                    0, NULL, 0, NULL, &query_result_callback, &data, &query );
    ok( hr == S_OK, "got hr %#lx\n", hr );
    ret = WaitForSingleObject( data.enum_completed, 5000 );
    ok( !ret, "got ret %lu\n", ret );
    pDevCloseObjectQuery( query );
    ret = WaitForSingleObject( data.closed, 1000 );
    ok( !ret, "got ret %lu\n", ret );

    CloseHandle( data.enum_completed );
    CloseHandle( data.closed );
}

static void test_DevGetObjectProperties_invalid( void )
{
    HRESULT hr;

    if (!pDevGetObjectProperties)
    {
        win_skip( "Functions unavailable, skipping test. (%p)\n", pDevGetObjectProperties );
        return;
    }

    hr = pDevGetObjectProperties( DevObjectTypeUnknown, NULL, 0, 0, NULL, NULL, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ), "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeUnknown, L"", 0, 0, NULL, NULL, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ), "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeUnknown, NULL, DevQueryFlagAsyncClose, 0, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeDeviceInterface, L"foobar", DevQueryFlagUpdateResults, 0, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeDeviceInterface, L"foobar", 0xdeadbeef, 0, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeUnknown, NULL, 0, 1, NULL, NULL, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ), "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeUnknown, NULL, 0, 0, (DEVPROPCOMPKEY *)0xdeadbeef, NULL, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ), "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeDeviceInterface, L"foobar", 0, 0, NULL, NULL, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ), "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeDeviceInterfaceDisplay, L"foobar", 0, 0, NULL, NULL, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ), "got hr %#lx\n", hr );

    hr = pDevGetObjectProperties( DevObjectTypeDeviceInterface, NULL, 0, 0, NULL, NULL, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx\n", hr );
}

static void test_DevFindProperty_invalid( void )
{
    const DEVPROPERTY *prop;

    if (!pDevFindProperty)
    {
        win_skip( "Functions unavailable, skipping test. (%p)\n", pDevFindProperty );
        return;
    }

    prop = pDevFindProperty( &DEVPKEY_dummy, DEVPROP_STORE_SYSTEM, NULL, 0, NULL );
    ok( !prop, "got prop %p\n", prop );

    prop = pDevFindProperty( &DEVPKEY_dummy, DEVPROP_STORE_SYSTEM, NULL, 0, (DEVPROPERTY *)0xdeadbeef );
    ok( !prop, "got prop %p\n", prop );
}

START_TEST(cfgmgr32)
{
    HMODULE mod = GetModuleHandleA("cfgmgr32.dll");
    pDevCreateObjectQuery = (void *)GetProcAddress(mod, "DevCreateObjectQuery");
    pDevCloseObjectQuery = (void *)GetProcAddress(mod, "DevCloseObjectQuery");
    pDevGetObjects = (void *)GetProcAddress(mod, "DevGetObjects");
    pDevFreeObjects = (void *)GetProcAddress(mod, "DevFreeObjects");
    pDevGetObjectProperties = (void *)GetProcAddress(mod, "DevGetObjectProperties");
    pDevFreeObjectProperties = (void *)GetProcAddress(mod, "DevFreeObjectProperties");
    pDevFindProperty = (void *)GetProcAddress(mod, "DevFindProperty");

    test_CM_MapCrToWin32Err();
    test_CM_Get_Device_ID_List();
    test_CM_Register_Notification();
    test_CM_Get_Device_Interface_List();
    test_DevGetObjects();
    test_DevCreateObjectQuery();
    test_DevGetObjectProperties_invalid();
    test_DevFindProperty_invalid();
}
