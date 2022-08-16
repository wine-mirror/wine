/*
 * Copyright 2022 Piotr Caban
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

#include <windows.h>
#include <prsht.h>
#include <ddk/compstui.h>

#include "wine/test.h"

typedef struct {
    WORD            cbSize;
    WORD            Flags;
    LPWSTR          pTitle;
    HWND            hWndParent;
    HINSTANCE       hInst;
    union {
        HICON       hIcon;
        ULONG_PTR   IconID;
    };
} PROPSHEETUI_INFO_HEADERW;

static INT_PTR CALLBACK prop_page_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_INITDIALOG)
    {
        HWND dlg = GetParent(hwnd);

        PostMessageW(dlg, PSM_PRESSBUTTON, PSBTN_OK, 0);
    }
    return FALSE;
}

#define ADD_PAGES 0x1

static LONG WINAPI propsheetui_callback(PROPSHEETUI_INFO *info, LPARAM lparam)
{
    PROPSHEETUI_INFO_HEADERW *info_header;

    ok(info->cbSize == sizeof(*info), "info->cbSize = %hd\n", info->cbSize);

    switch(info->Reason)
    {
    case PROPSHEETUI_REASON_BEFORE_INIT:
        ok(!info->Version, "info->Version = %hd\n", info->Version);
        ok(!info->Flags, "info->Flags = %hd\n", info->Flags);
        ok(!info->hComPropSheet, "info->hComPropSheet = %p\n", info->hComPropSheet);
        ok(!info->pfnComPropSheet, "info->pfnComPropSheet = %p\n", info->pfnComPropSheet);
        ok(!info->Result, "info->Result = %Ix\n", info->Result);
        break;
    case PROPSHEETUI_REASON_INIT:
        ok(info->Version == PROPSHEETUI_INFO_VERSION, "info->Version = %hd\n", info->Version);
        ok(info->Flags == PSUIINFO_UNICODE, "info->Flags = %hd\n", info->Flags);
        ok(info->hComPropSheet != NULL, "info->hComPropSheet = NULL\n");
        ok(info->pfnComPropSheet != NULL, "info->pfnComPropSheet = NULL\n");
        ok(!info->UserData, "info->UserData = %Ix\n", info->UserData);
        ok(!info->Result, "info->Result = %Ix\n", info->Result);
        break;
    case PROPSHEETUI_REASON_GET_INFO_HEADER:
        ok(info->Version == PROPSHEETUI_INFO_VERSION, "info->Version = %hd\n", info->Version);
        ok(info->Flags == PSUIINFO_UNICODE, "info->Flags = %hd\n", info->Flags);
        ok(info->hComPropSheet != NULL, "info->hComPropSheet = NULL\n");
        ok(info->pfnComPropSheet != NULL, "info->pfnComPropSheet = NULL\n");
        ok(!info->UserData, "info->UserData = %Ix\n", info->UserData);
        ok(!info->Result, "info->Result = %Ix\n", info->Result);

        info_header = (PROPSHEETUI_INFO_HEADERW*)lparam;
        ok(info_header->cbSize == sizeof(*info_header), "info_header->cbSize = %hd\n", info_header->cbSize);
        ok(!info_header->Flags, "info_header->Flags = %hx\n", info_header->Flags);
        ok(!info_header->pTitle, "info_header->pTitle = %p\n", info_header->pTitle);
        ok(info_header->hWndParent == (HWND)0x4321, "info_header->hWndParent = %p\n", info_header->hWndParent);
        ok(!info_header->hInst, "info_header->hInst = %p\n", info_header->hInst);

        info_header->hWndParent = 0;

        if (info->lParamInit & ADD_PAGES)
        {
            HPROPSHEETPAGE hpsp;
            PROPSHEETPAGEW psp;
            LONG_PTR ret;

            memset(&psp, 0, sizeof(psp));
            psp.dwSize = sizeof(psp);
            U(psp).pszTemplate = L"prop_page1";
            psp.pfnDlgProc = prop_page_proc;
            hpsp = CreatePropertySheetPageW(&psp);
            ok(hpsp != NULL, "hpsp = %p\n", hpsp);
            ret = info->pfnComPropSheet(info->hComPropSheet, CPSFUNC_ADD_HPROPSHEETPAGE, (LPARAM)hpsp, 0);
            ok(ret, "ret = 0\n");
        }
        break;
    case PROPSHEETUI_REASON_DESTROY:
        if (info->lParamInit & ADD_PAGES)
            ok(lparam, "lparam = 0\n");
        else
            ok(!lparam, "lparam = %Ix\n", lparam);

        ok(info->Version == PROPSHEETUI_INFO_VERSION, "info->Version = %hd\n", info->Version);
        ok(info->Flags == PSUIINFO_UNICODE, "info->Flags = %hd\n", info->Flags);
        ok(info->hComPropSheet != NULL, "info->hComPropSheet = NULL\n");
        ok(info->pfnComPropSheet != NULL, "info->pfnComPropSheet = NULL\n");
        ok(!info->UserData, "info->UserData = %Ix\n", info->UserData);
        ok(!info->Result, "info->Result = %Ix\n", info->Result);
        break;
    default:
        ok(0, "unhandled info->Reason = %hd\n", info->Reason);
        break;
    }

    return 1;
}

static void test_CommonPropertySheetUI(void)
{
    DWORD res;
    LONG ret;

    SetLastError(0xdeadbeef);
    res = 0xdeadbeef;
    ret = CommonPropertySheetUIW(0, NULL, 0, &res);
    ok(ret == ERR_CPSUI_GETLASTERROR, "ret = %ld\n", ret);
    ok(res == 0xdeadbeef, "res = %lx\n", res);
    ok(!GetLastError(), "CommonPropertySheetUIW error %ld\n", GetLastError());

    res = 0xdeadbeef;
    ret = CommonPropertySheetUIW((HWND)0x4321, propsheetui_callback, 0, &res);
    ok(ret == ERR_CPSUI_NO_PROPSHEETPAGE, "CommonPropertySheetUIW returned %ld\n", ret);
    ok(!res, "res = %lx\n", res);

    res = 0xdeadbeef;
    ret = CommonPropertySheetUIW((HWND)0x4321, propsheetui_callback, ADD_PAGES, &res);
    ok(ret == CPSUI_OK, "CommonPropertySheetUIW returned %ld\n", ret);
    ok(!res, "res = %lx\n", res);
}

START_TEST(compstui)
{
    test_CommonPropertySheetUI();
}
