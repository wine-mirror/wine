/*
 * Unit test suite for kernel mode graphics driver
 *
 * Copyright 2019 Zhiyi Zhang
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "ddk/d3dkmthk.h"

#include "wine/test.h"

static const WCHAR display1W[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y','1',0};

static NTSTATUS (WINAPI *pD3DKMTCloseAdapter)(const D3DKMT_CLOSEADAPTER *);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromGdiDisplayName)(D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *);
static NTSTATUS (WINAPI *pD3DKMTOpenAdapterFromHdc)(D3DKMT_OPENADAPTERFROMHDC *);

static void test_D3DKMTOpenAdapterFromGdiDisplayName(void)
{
    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME open_adapter_gdi_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    NTSTATUS status;
    DWORD i;

    lstrcpyW(open_adapter_gdi_desc.DeviceName, display1W);
    if (!pD3DKMTOpenAdapterFromGdiDisplayName
        || pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc) == STATUS_PROCEDURE_NOT_FOUND)
    {
        skip("D3DKMTOpenAdapterFromGdiDisplayName() is unavailable.\n");
        return;
    }

    close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    ok(status == STATUS_SUCCESS, "Got unexpected return code %#x.\n", status);

    /* Invalid parameters */
    status = pD3DKMTOpenAdapterFromGdiDisplayName(NULL);
    ok(status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#x.\n", status);

    memset(&open_adapter_gdi_desc, 0, sizeof(open_adapter_gdi_desc));
    status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
    ok(status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#x.\n", status);

    /* Open adapter */
    for (i = 0; EnumDisplayDevicesW(NULL, i, &display_device, 0); ++i)
    {
        lstrcpyW(open_adapter_gdi_desc.DeviceName, display_device.DeviceName);
        status = pD3DKMTOpenAdapterFromGdiDisplayName(&open_adapter_gdi_desc);
        if (display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
            ok(status == STATUS_SUCCESS, "Got unexpected return code %#x.\n", status);
        else
        {
            ok(status == STATUS_UNSUCCESSFUL, "Got unexpected return code %#x.\n", status);
            continue;
        }

        ok(open_adapter_gdi_desc.hAdapter, "Expect not null.\n");
        if (display_device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            ok(open_adapter_gdi_desc.VidPnSourceId == 0, "Got unexpected value %#x.\n",
               open_adapter_gdi_desc.VidPnSourceId);
        else
            ok(open_adapter_gdi_desc.VidPnSourceId, "Got unexpected value %#x.\n", open_adapter_gdi_desc.VidPnSourceId);

        close_adapter_desc.hAdapter = open_adapter_gdi_desc.hAdapter;
        status = pD3DKMTCloseAdapter(&close_adapter_desc);
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#x.\n", status);
    }
}

static void test_D3DKMTOpenAdapterFromHdc(void)
{
    DISPLAY_DEVICEW display_device = {sizeof(display_device)};
    D3DKMT_OPENADAPTERFROMHDC open_adapter_hdc_desc;
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    INT adapter_count = 0;
    NTSTATUS status;
    HDC hdc;
    DWORD i;

    if (!pD3DKMTOpenAdapterFromHdc || pD3DKMTOpenAdapterFromHdc(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        skip("D3DKMTOpenAdapterFromHdc() is unavailable.\n");
        return;
    }

    /* Invalid parameters */
    status = pD3DKMTOpenAdapterFromHdc(NULL);
    todo_wine ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#x.\n", status);

    memset(&open_adapter_hdc_desc, 0, sizeof(open_adapter_hdc_desc));
    status = pD3DKMTOpenAdapterFromHdc(&open_adapter_hdc_desc);
    todo_wine ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#x.\n", status);

    /* Open adapter */
    for (i = 0; EnumDisplayDevicesW(NULL, i, &display_device, 0); ++i)
    {
        if (!(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        adapter_count++;

        hdc = CreateDCW(0, display_device.DeviceName, 0, NULL);
        open_adapter_hdc_desc.hDc = hdc;
        status = pD3DKMTOpenAdapterFromHdc(&open_adapter_hdc_desc);
        todo_wine ok(status == STATUS_SUCCESS, "Got unexpected return code %#x.\n", status);
        todo_wine ok(open_adapter_hdc_desc.hAdapter, "Expect not null.\n");
        if (display_device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            ok(open_adapter_hdc_desc.VidPnSourceId == 0, "Got unexpected value %#x.\n",
               open_adapter_hdc_desc.VidPnSourceId);
        else
            todo_wine ok(open_adapter_hdc_desc.VidPnSourceId, "Got unexpected value %#x.\n",
                         open_adapter_hdc_desc.VidPnSourceId);
        DeleteDC(hdc);

        if (status == STATUS_SUCCESS)
        {
            close_adapter_desc.hAdapter = open_adapter_hdc_desc.hAdapter;
            status = pD3DKMTCloseAdapter(&close_adapter_desc);
            ok(status == STATUS_SUCCESS, "Got unexpected return code %#x.\n", status);
        }
    }

    /* HDC covering more than two adapters is invalid for D3DKMTOpenAdapterFromHdc */
    hdc = GetDC(0);
    open_adapter_hdc_desc.hDc = hdc;
    status = pD3DKMTOpenAdapterFromHdc(&open_adapter_hdc_desc);
    ReleaseDC(0, hdc);
    todo_wine ok(status == (adapter_count > 1 ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS),
                 "Got unexpected return code %#x.\n", status);
    if (status == STATUS_SUCCESS)
    {
        close_adapter_desc.hAdapter = open_adapter_hdc_desc.hAdapter;
        status = pD3DKMTCloseAdapter(&close_adapter_desc);
        ok(status == STATUS_SUCCESS, "Got unexpected return code %#x.\n", status);
    }
}

static void test_D3DKMTCloseAdapter(void)
{
    D3DKMT_CLOSEADAPTER close_adapter_desc;
    NTSTATUS status;

    if (!pD3DKMTCloseAdapter || pD3DKMTCloseAdapter(NULL) == STATUS_PROCEDURE_NOT_FOUND)
    {
        win_skip("D3DKMTCloseAdapter() is unavailable.\n");
        return;
    }

    /* Invalid parameters */
    status = pD3DKMTCloseAdapter(NULL);
    todo_wine ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#x.\n", status);

    memset(&close_adapter_desc, 0, sizeof(close_adapter_desc));
    status = pD3DKMTCloseAdapter(&close_adapter_desc);
    todo_wine ok(status == STATUS_INVALID_PARAMETER, "Got unexpected return code %#x.\n", status);
}

START_TEST(driver)
{
    HMODULE gdi32 = GetModuleHandleA("gdi32.dll");

    pD3DKMTCloseAdapter = (void *)GetProcAddress(gdi32, "D3DKMTCloseAdapter");
    pD3DKMTOpenAdapterFromGdiDisplayName = (void *)GetProcAddress(gdi32, "D3DKMTOpenAdapterFromGdiDisplayName");
    pD3DKMTOpenAdapterFromHdc = (void *)GetProcAddress(gdi32, "D3DKMTOpenAdapterFromHdc");

    test_D3DKMTOpenAdapterFromGdiDisplayName();
    test_D3DKMTOpenAdapterFromHdc();
    test_D3DKMTCloseAdapter();
}
