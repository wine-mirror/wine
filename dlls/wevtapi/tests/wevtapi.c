/*
 * Copyright 2021 Dmitry Timoshkov
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

#include "windef.h"
#include "winbase.h"
#include "winevt.h"
#include "wine/test.h"

static void test_EvtGetChannelConfigProperty(void)
{
    HKEY key;
    EVT_HANDLE config;
    DWORD size, ret;
    struct
    {
        EVT_VARIANT var;
        WCHAR buf[MAX_PATH];
    } path;

    ret = RegCreateKeyW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\EventLog\\Winetest", &key);
    if (ret == ERROR_ACCESS_DENIED)
    {
        skip("Not enough privileges to modify HKLM\n");
        return;
    }
    ok(ret == ERROR_SUCCESS, "RegCreateKey error %lu\n", ret);

    config = EvtOpenChannelConfig(NULL, L"Winetest", 0);
    ok(config != NULL, "EvtOpenChannelConfig error %lu\n", GetLastError());

    ret = EvtGetChannelConfigProperty(config, EvtChannelLoggingConfigLogFilePath, 0, 0, NULL, &size);
    ok(!ret, "EvtGetChannelConfigProperty should fail\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %lu\n", GetLastError());
    ok(size < sizeof(path), "got %lu\n", size);

    memset(&path, 0, sizeof(path));
    path.var.Count = 0xdeadbeef;
    path.var.Type = 0xdeadbeef;
    ret = EvtGetChannelConfigProperty(config, EvtChannelLoggingConfigLogFilePath, 0, size, (EVT_VARIANT *)&path, &size);
    ok(ret, "EvtGetChannelConfigProperty error %lu\n", GetLastError());
    ok(path.var.Count == 0xdeadbeef, "got %lu\n", path.var.Count);
    ok(path.var.Type == EvtVarTypeString, "got %lu\n", path.var.Type);
    ok(size == sizeof(EVT_VARIANT) + (wcslen(path.var.StringVal) + 1) * sizeof(WCHAR), "got %lu\n", size);
    ok(path.var.StringVal == path.buf, "path.var.StringVal = %p, path.buf = %p\n", path.var.StringVal, path.buf);

    EvtClose(config);

    RegDeleteKeyW(key, L"");
    RegCloseKey(key);
}

START_TEST(wevtapi)
{
    test_EvtGetChannelConfigProperty();
}
