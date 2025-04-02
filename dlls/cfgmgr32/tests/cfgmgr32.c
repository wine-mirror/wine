/*
 * Copyright (C) 2023 Mohamad Al-Jaf
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
#include "setupapi.h"
#include "cfgmgr32.h"
#include "ntddvdeo.h"

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

static void test_CM_Get_Device_Interface_List(void)
{
    BYTE iface_detail_buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + 256 * sizeof(WCHAR)];
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_data;
    SP_DEVINFO_DATA device = { sizeof(device) };
    unsigned int count, count2;
    WCHAR *buffer, *p;
    CONFIGRET ret;
    HDEVINFO set;
    ULONG size;
    GUID guid;
    BOOL bret;

    guid = GUID_DEVINTERFACE_DISPLAY_ADAPTER;

    ret = CM_Get_Device_Interface_List_SizeW(&size, &guid, NULL, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    ok(!ret, "got %#lx.\n", ret);

    buffer = malloc(size * sizeof(*buffer));
    ret = CM_Get_Device_Interface_ListW( &guid, NULL, buffer, size, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    ok(!ret, "got %#lx.\n", ret);

    iface_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *)iface_detail_buffer;

    count = 0;
    p = buffer;
    while (*p)
    {
        set = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
        ok(set != INVALID_HANDLE_VALUE, "got %p.\n", set);
        bret = SetupDiOpenDeviceInterfaceW(set, p, 0, &iface);
        ok(bret, "got error %lu.\n", GetLastError());
        memset(iface_detail_buffer, 0xcc, sizeof(iface_detail_buffer));
        iface_data->cbSize = sizeof(*iface_data);
        bret = SetupDiGetDeviceInterfaceDetailW(set, &iface, iface_data, sizeof(iface_detail_buffer), NULL, &device);
        ok(bret, "got error %lu.\n", GetLastError());
        ok(!wcsicmp(iface_data->DevicePath, p), "got %s, expected %s.\n", debugstr_w(p), debugstr_w(iface_data->DevicePath));
        SetupDiDestroyDeviceInfoList(set);
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
}

START_TEST(cfgmgr32)
{
    test_CM_MapCrToWin32Err();
    test_CM_Get_Device_ID_List();
    test_CM_Register_Notification();
    test_CM_Get_Device_Interface_List();
}
