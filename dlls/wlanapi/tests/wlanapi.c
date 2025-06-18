/*
 * Unit test suite for wlanapi functions
 *
 * Copyright 2017 Bruno Jesus
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wlanapi.h>

#include "wine/test.h"

static void test_WlanOpenHandle(void)
{
    HANDLE bad_handle = (HANDLE) 0xdeadcafe, handle = bad_handle, handle2;
    DWORD ret, neg_version = 0xdeadbeef, reserved = 0xdead;
    BOOL is_xp;

    /* invalid version requested */
    ret = WlanOpenHandle(0, NULL, &neg_version, &handle);
    is_xp = ret == ERROR_SUCCESS;
    if (!is_xp) /* the results in XP differ completely from all other versions */
    {
        ok(ret == ERROR_NOT_SUPPORTED, "Expected 50, got %ld\n", ret);
        ok(neg_version == 0xdeadbeef, "neg_version changed\n");
        ok(handle == bad_handle, "handle changed\n");
        ret = WlanOpenHandle(10, NULL, &neg_version, &handle);
        ok(ret == ERROR_NOT_SUPPORTED, "Expected 50, got %ld\n", ret);
        ok(neg_version == 0xdeadbeef, "neg_version changed\n");
        ok(handle == bad_handle, "handle changed\n");

        /* reserved parameter must not be used */
        ret = WlanOpenHandle(1, &reserved, &neg_version, &handle);
        ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ok(neg_version == 0xdeadbeef, "neg_version changed\n");
        ok(handle == bad_handle, "handle changed\n");

        /* invalid parameters */
        ret = WlanOpenHandle(1, NULL, NULL, &handle);
        ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ok(handle == bad_handle, "bad handle\n");
        ret = WlanOpenHandle(1, NULL, &neg_version, NULL);
        ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
        ok(neg_version == 0xdeadbeef, "neg_version changed\n");
    }
    else
    {
        ok(neg_version == 1, "Expected 1, got %ld\n", neg_version);
        ok(handle != bad_handle && handle, "handle changed\n");
        ret = WlanCloseHandle(handle, NULL);
        ok(ret == 0, "Expected 0, got %ld\n", ret);
    }

    /* good tests */
    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    ok(neg_version == 1, "Expected 1, got %ld\n", neg_version);
    ok(handle != bad_handle && handle, "handle changed\n");
    ret = WlanCloseHandle(handle, NULL);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    ret = WlanOpenHandle(2, NULL, &neg_version, &handle);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    if (!is_xp) /* XP does not support client version 2 */
      ok(neg_version == 2, "Expected 2, got %ld\n", neg_version);
    else
      ok(neg_version == 1, "Expected 1, got %ld\n", neg_version);
    ok(handle != bad_handle && handle, "bad handle\n");
    ret = WlanCloseHandle(handle, NULL);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    /* open twice */
    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    ret = WlanOpenHandle(1, NULL, &neg_version, &handle2);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);

    ret = WlanCloseHandle(handle, &reserved);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);

    ret = WlanCloseHandle(handle, NULL);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    ret = WlanCloseHandle(handle2, NULL);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);

    ret = WlanCloseHandle(bad_handle, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "Expected 6, got %ld\n", ret);

    ret = WlanCloseHandle(NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
}

static void test_WlanAllocateFreeMemory(void)
{
    void *ptr;

    SetLastError(0xdeadbeef);
    ptr = WlanAllocateMemory(0);
    ok(ptr == NULL, "Expected NULL, got %p\n", ptr);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", GetLastError());

    ptr = WlanAllocateMemory(1024);
    ok(ptr != NULL, "Expected non-NULL\n");

    WlanFreeMemory(ptr);

    WlanFreeMemory(NULL); /* return is void, proves that won't crash */
}

static void test_WlanEnumInterfaces(void)
{
    HANDLE handle;
    DWORD neg_version, i, ret, reserved = 0xdeadbeef;
    WLAN_INTERFACE_INFO_LIST *bad_list = (WLAN_INTERFACE_INFO_LIST *)0xdeadcafe,
                             *list = bad_list;
    WLAN_INTERFACE_INFO *info;

    ret = WlanOpenHandle(1, NULL, &neg_version, &handle);
    ok(ret == 0, "Expected 0, got %ld\n", ret);

    /* invalid parameters */
    ret = WlanEnumInterfaces(NULL, NULL, &list);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
    ok(list == bad_list, "list changed\n");
    ret = WlanEnumInterfaces(handle, &reserved, &list);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
    ok(list == bad_list, "list changed\n");
    ret = WlanEnumInterfaces(handle, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "Expected 87, got %ld\n", ret);
    ok(list == bad_list, "list changed\n");

    /* good tests */
    list = NULL;
    ret = WlanEnumInterfaces(handle, NULL, &list);
    ok(ret == ERROR_SUCCESS, "Expected 0, got %ld\n", ret);
    ok(list != NULL, "bad interface list\n");
    if (!list || !list->dwNumberOfItems)
    {
        skip("No wireless interfaces\n");
        WlanCloseHandle(handle, NULL);
        WlanFreeMemory(list);
        return;
    }

    trace("Wireless interfaces: %ld\n", list->dwNumberOfItems);
    for (i = 0; i < list->dwNumberOfItems;i ++)
    {
        info = &list->InterfaceInfo[i];
        trace("  Index[%ld] GUID: %s\n", i, wine_dbgstr_guid(&info->InterfaceGuid));
        switch (info->isState)
        {
            case wlan_interface_state_disconnected:
                trace("  Status: Disconnected\n");
                break;
            case wlan_interface_state_connected:
                trace("  Status: Connected\n");
                break;
            default:
                trace("  Status: Other\n");
                break;
        }
        trace("  Description: %s\n", wine_dbgstr_w(info->strInterfaceDescription));
    }

    WlanFreeMemory(list);

    ret = WlanCloseHandle(handle, NULL);
    ok(ret == 0, "Expected 0, got %ld\n", ret);
}

START_TEST(wlanapi)
{
  HANDLE handle;
  DWORD neg_version;

  /* Windows checks the service before validating the client version so this
   * call will always result in error, no need to free the handle. */
  if (WlanOpenHandle(0, NULL, &neg_version, &handle) == ERROR_SERVICE_NOT_ACTIVE)
  {
      win_skip("No wireless service running\n");
      return;
  }

  test_WlanOpenHandle();
  test_WlanAllocateFreeMemory();
  test_WlanEnumInterfaces();
}
