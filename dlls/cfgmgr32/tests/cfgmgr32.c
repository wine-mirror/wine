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

static void test_CM_Get_Device_ID_List(void)
{
    WCHAR wguid_str[64], *wbuf, *wp;
    unsigned int count;
    CONFIGRET ret;
    ULONG len;

    StringFromGUID2(&GUID_DEVCLASS_DISPLAY, wguid_str, ARRAY_SIZE(wguid_str));
    wp = wguid_str;

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

    len = 0xdeadbeef;
    ret = CM_Get_Device_ID_List_SizeW(&len, NULL, 0);
    ok(!ret, "got %#lx.\n", ret);
    ok(len > 2, "got %#lx.\n", len);

    wbuf = malloc(len * sizeof(*wbuf));

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
    ok(count, "got 0.\n");

    free(wbuf);
}

START_TEST(cfgmgr32)
{
    test_CM_MapCrToWin32Err();
    test_CM_Get_Device_ID_List();
}
