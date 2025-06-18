/*
 * Tests for special shell folders
 *
 * Copyright 2008 Robert Shearman for CodeWeavers
 * Copyright 2008 Owen Rudge
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
#include <stdio.h>

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "shellapi.h"
#include "shlwapi.h"
#include "shlobj.h"

#include "wine/test.h"

static inline BOOL SHELL_OsIsUnicode(void)
{
    return !(GetVersion() & 0x80000000);
}

/* Tests for My Computer */
static void test_parse_for_my_computer(void)
{
    WCHAR path[] = L"\\\\?\\C:\\";
    IShellFolder *mycomp, *sf;
    WCHAR *drive = path + 4;
    ITEMIDLIST *pidl;
    DWORD eaten;
    HRESULT hr;

    hr = SHGetDesktopFolder(&sf);
    ok(hr == S_OK, "SHGetDesktopFolder failed with error 0x%lx\n", hr);
    if (hr != S_OK) return;
    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidl);
    ok(hr == S_OK, "SHGetSpecialFolderLocation failed with error 0x%lx\n", hr);
    if (hr != S_OK)
    {
        IShellFolder_Release(sf);
        return;
    }
    hr = IShellFolder_BindToObject(sf, pidl, NULL, &IID_IShellFolder, (void**)&mycomp);
    ok(hr == S_OK, "IShellFolder::BindToObject failed with error 0x%lx\n", hr);
    IShellFolder_Release(sf);
    ILFree(pidl);
    if (hr != S_OK) return;

    while (drive[0] <= 'Z' && GetDriveTypeW(drive) == DRIVE_NO_ROOT_DIR) drive[0]++;
    if (drive[0] > 'Z')
    {
        skip("No drive found, skipping My Computer tests...\n");
        goto done;
    }

    pidl = NULL;
    eaten = 0xdeadbeef;
    hr = IShellFolder_ParseDisplayName(mycomp, NULL, NULL, drive, &eaten, &pidl, NULL);
    ok(hr == S_OK, "IShellFolder::ParseDisplayName failed with error 0x%lx\n", hr);
    todo_wine
    ok(eaten == 0xdeadbeef, "eaten should not have been set to %lu\n", eaten);
    ok(pidl != NULL, "pidl is NULL\n");
    ILFree(pidl);

    /* \\?\ prefix is not valid */
    pidl = NULL;
    eaten = 0xdeadbeef;
    hr = IShellFolder_ParseDisplayName(mycomp, NULL, NULL, path, &eaten, &pidl, NULL);
    ok(hr == E_INVALIDARG, "IShellFolder::ParseDisplayName should have failed with E_INVALIDARG, got 0x%08lx\n", hr);
    todo_wine
    ok(eaten == 0xdeadbeef, "eaten should not have been set to %lu\n", eaten);
    ok(pidl == NULL, "pidl is not NULL\n");
    ILFree(pidl);

    /* Try without backslash */
    drive[2] = '\0';
    pidl = NULL;
    eaten = 0xdeadbeef;
    hr = IShellFolder_ParseDisplayName(mycomp, NULL, NULL, drive, &eaten, &pidl, NULL);
    ok(hr == S_OK || broken(hr == E_INVALIDARG) /* WinXP */,
       "IShellFolder::ParseDisplayName failed with error 0x%lx\n", hr);
    todo_wine
    ok(eaten == 0xdeadbeef, "eaten should not have been set to %lu\n", eaten);
    ok(pidl != NULL || broken(!pidl) /* WinXP */, "pidl is NULL\n");
    ILFree(pidl);

done:
    IShellFolder_Release(mycomp);
}

/* Tests for My Network Places */
static void test_parse_for_entire_network(void)
{
    static WCHAR my_network_places_path[] = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}";
    static WCHAR entire_network_path[] = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}\\EntireNetwork";
    IShellFolder *psfDesktop;
    HRESULT hr;
    DWORD eaten = 0xdeadbeef;
    LPITEMIDLIST pidl;
    DWORD attr = ~0;
    DWORD expected_attr;

    hr = SHGetDesktopFolder(&psfDesktop);
    ok(hr == S_OK, "SHGetDesktopFolder failed with error 0x%lx\n", hr);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, my_network_places_path, &eaten, &pidl, &attr);
    ok(hr == S_OK, "IShellFolder_ParseDisplayName failed with error 0x%lx\n", hr);
    todo_wine
    ok(eaten == 0xdeadbeef, "eaten should not have been set to %lu\n", eaten);
    expected_attr = SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET|SFGAO_CANRENAME|SFGAO_CANLINK;
    todo_wine
    ok((attr == expected_attr) || /* Win9x, NT4 */
       (attr == (expected_attr | SFGAO_STREAM)) || /* W2K */
       (attr == (expected_attr | SFGAO_CANDELETE)) || /* XP, W2K3 */
       (attr == (expected_attr | SFGAO_CANDELETE | SFGAO_NONENUMERATED)), /* Vista */
       "Unexpected attributes : %08lx\n", attr);

    ILFree(pidl);

    /* Start clean again */
    eaten = 0xdeadbeef;
    attr = ~0;

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, entire_network_path, &eaten, &pidl, &attr);
    IShellFolder_Release(psfDesktop);
    if (hr == HRESULT_FROM_WIN32(ERROR_BAD_NET_NAME) ||
        hr == HRESULT_FROM_WIN32(ERROR_NO_NET_OR_BAD_PATH) ||
        hr == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER))
    {
        win_skip("'EntireNetwork' is not available on Win9x, NT4 and Vista\n");
        return;
    }
    ok(hr == S_OK, "IShellFolder_ParseDisplayName failed with error 0x%lx\n", hr);
    todo_wine
    ok(eaten == 0xdeadbeef, "eaten should not have been set to %lu\n", eaten);
    expected_attr = SFGAO_HASSUBFOLDER|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_HASPROPSHEET|SFGAO_CANLINK;
    todo_wine
    ok(attr == expected_attr || /* winme, nt4 */
       attr == (expected_attr | SFGAO_STREAM) || /* win2k */
       attr == (expected_attr | SFGAO_STORAGEANCESTOR),  /* others */
       "attr should be 0x%lx, not 0x%lx\n", expected_attr, attr);

    ILFree(pidl);
}

/* Tests for Control Panel */
static void test_parse_for_control_panel(void)
{
    /* path of My Computer\Control Panel */
    static WCHAR control_panel_path[] =
        L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}";
    IShellFolder *psfDesktop;
    HRESULT hr;
    DWORD eaten = 0xdeadbeef;
    LPITEMIDLIST pidl;
    DWORD attr = ~0;

    hr = SHGetDesktopFolder(&psfDesktop);
    ok(hr == S_OK, "SHGetDesktopFolder failed with error 0x%lx\n", hr);

    hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, control_panel_path, &eaten, &pidl, &attr);
    ok(hr == S_OK, "IShellFolder_ParseDisplayName failed with error 0x%lx\n", hr);
    todo_wine ok(eaten == 0xdeadbeef, "eaten should not have been set to %lu\n", eaten);
    todo_wine
    ok((attr == (SFGAO_CANLINK | SFGAO_FOLDER)) || /* Win9x, NT4 */
       (attr == (SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_STREAM)) || /* W2K */
       (attr == (SFGAO_CANLINK | SFGAO_FOLDER | SFGAO_HASSUBFOLDER)) || /* W2K, XP, W2K3 */
       (attr == (SFGAO_CANLINK | SFGAO_NONENUMERATED)) || /* Vista */
       (attr == SFGAO_CANLINK), /* Vista, W2K8 */
       "Unexpected attributes : %08lx\n", attr);

    ILFree(pidl);
    IShellFolder_Release(psfDesktop);
}

static void test_printers_folder(void)
{
    IShellFolder2 *folder;
    IPersistFolder2 *pf;
    SHELLDETAILS details;
    SHCOLSTATEF state;
    LPITEMIDLIST pidl1, pidl2;
    HRESULT hr;
    INT i;

    CoInitialize( NULL );

    hr = CoCreateInstance(&CLSID_Printers, NULL, CLSCTX_INPROC_SERVER, &IID_IShellFolder2, (void**)&folder);
    if (hr != S_OK)
    {
        win_skip("Failed to created IShellFolder2 for Printers folder\n");
        CoUninitialize();
        return;
    }

if (0)
{
    /* crashes on XP */
    IShellFolder2_GetDetailsOf(folder, NULL, 0, NULL);
    IShellFolder2_GetDefaultColumnState(folder, 0, NULL);
    IPersistFolder2_GetCurFolder(pf, NULL);
}

    /* 5 columns defined */
    hr = IShellFolder2_GetDetailsOf(folder, NULL, 6, &details);
    ok(hr == E_NOTIMPL, "got 0x%08lx\n", hr);

    hr = IShellFolder2_GetDefaultColumnState(folder, 6, &state);
    ok(broken(hr == E_NOTIMPL) || hr == E_INVALIDARG /* Win7 */, "got 0x%08lx\n", hr);

    details.str.pOleStr = NULL;
    hr = IShellFolder2_GetDetailsOf(folder, NULL, 0, &details);
    ok(hr == S_OK || broken(hr == E_NOTIMPL) /* W2K */, "got 0x%08lx\n", hr);
    if (SHELL_OsIsUnicode()) SHFree(details.str.pOleStr);

    /* test every column if method is implemented */
    if (hr == S_OK)
    {
        ok(details.str.uType == STRRET_WSTR, "got %d\n", details.str.uType);

        for(i = 0; i < 6; i++)
        {
            hr = IShellFolder2_GetDetailsOf(folder, NULL, i, &details);
            ok(hr == S_OK, "got 0x%08lx\n", hr);

            /* all columns are left-aligned */
            ok(details.fmt == LVCFMT_LEFT, "got 0x%x\n", details.fmt);
            /* can't be on w9x at this point, IShellFolder2 unsupported there,
               check present for running Wine with w9x setup */
            if (SHELL_OsIsUnicode()) SHFree(details.str.pOleStr);

            hr = IShellFolder2_GetDefaultColumnState(folder, i, &state);
            ok(hr == S_OK, "got 0x%08lx\n", hr);
            /* all columns are string except document count */
            if (i == 1)
                ok(state == (SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT), "got 0x%lx\n", state);
            else
                ok(state == (SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT), "got 0x%lx\n", state);
        }
    }

    /* default pidl */
    hr = IShellFolder2_QueryInterface(folder, &IID_IPersistFolder2, (void**)&pf);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    /* not initialized */
    pidl1 = (void*)0xdeadbeef;
    hr = IPersistFolder2_GetCurFolder(pf, &pidl1);
    ok(hr == S_FALSE, "got 0x%08lx\n", hr);
    ok(pidl1 == NULL, "got %p\n", pidl1);

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_PRINTERS, &pidl2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IPersistFolder2_Initialize(pf, pidl2);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    hr = IPersistFolder2_GetCurFolder(pf, &pidl1);
    ok(hr == S_OK, "got 0x%08lx\n", hr);

    ok(ILIsEqual(pidl1, pidl2), "expected same PIDL\n");
    IPersistFolder2_Release(pf);

    ILFree(pidl1);
    ILFree(pidl2);
    IShellFolder2_Release(folder);

    CoUninitialize();
}

static void test_desktop_folder(void)
{
    IShellFolder *psf;
    HRESULT hr;

    hr = SHGetDesktopFolder(&psf);
    ok(hr == S_OK, "Got %lx\n", hr);

    hr = IShellFolder_QueryInterface(psf, &IID_IShellFolder, NULL);
    ok(hr == E_POINTER, "Got %lx\n", hr);

    IShellFolder_Release(psf);
}

static void test_desktop_displaynameof(void)
{
    static WCHAR MyComputer[]  = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
    static WCHAR MyDocuments[] = L"::{450D8FBA-AD25-11D0-98A8-0800361B1103}";
    static WCHAR RecycleBin[]  = L"::{645FF040-5081-101B-9F08-00AA002F954E}";
    static WCHAR ControlPanel[]= L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}";
    static WCHAR *folders[] = { MyComputer, MyDocuments, RecycleBin, ControlPanel };
    IShellFolder *desktop;
    ITEMIDLIST *pidl;
    STRRET strret;
    DWORD eaten;
    HRESULT hr;
    UINT i;

    hr = SHGetDesktopFolder(&desktop);
    ok(hr == S_OK, "SHGetDesktopFolder failed with error 0x%08lx\n", hr);
    if (FAILED(hr)) return;

    for (i = 0; i < ARRAY_SIZE(folders); i++)
    {
        WCHAR name1[MAX_PATH], name2[MAX_PATH];

        hr = IShellFolder_ParseDisplayName(desktop, NULL, NULL, folders[i], &eaten, &pidl, NULL);
        ok(hr == S_OK, "IShellFolder::ParseDisplayName failed with error 0x%08lx\n", hr);
        if (FAILED(hr)) continue;

        hr = IShellFolder_GetDisplayNameOf(desktop, pidl, SHGDN_INFOLDER, &strret);
        ok(hr == S_OK, "IShellFolder::GetDisplayNameOf failed with error 0x%08lx\n", hr);
        hr = StrRetToBufW(&strret, pidl, name1, ARRAY_SIZE(name1));
        ok(hr == S_OK, "StrRetToBuf failed with error 0x%08lx\n", hr);

        hr = IShellFolder_GetDisplayNameOf(desktop, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, &strret);
        ok(hr == S_OK, "IShellFolder::GetDisplayNameOf failed with error 0x%08lx\n", hr);
        hr = StrRetToBufW(&strret, pidl, name2, ARRAY_SIZE(name2));
        ok(hr == S_OK, "StrRetToBuf failed with error 0x%08lx\n", hr);

        ok(!lstrcmpW(name1, name2), "the display names are not equal: %s vs %s\n", wine_dbgstr_w(name1), wine_dbgstr_w(name2));
        ok(name1[0] != ':' || name1[1] != ':', "display name is a GUID: %s\n", wine_dbgstr_w(name1));

        hr = IShellFolder_GetDisplayNameOf(desktop, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, &strret);
        ok(hr == S_OK, "IShellFolder::GetDisplayNameOf failed with error 0x%08lx\n", hr);
        hr = StrRetToBufW(&strret, pidl, name1, ARRAY_SIZE(name1));
        ok(hr == S_OK, "StrRetToBuf failed with error 0x%08lx\n", hr);

        ok(lstrcmpW(name1, name2), "the display names are equal: %s\n", wine_dbgstr_w(name1));
        ok(name1[0] == ':' && name1[1] == ':', "display name is not a GUID: %s\n", wine_dbgstr_w(name1));

        ILFree(pidl);
    }
    IShellFolder_Release(desktop);
}

START_TEST(shfldr_special)
{
    test_parse_for_my_computer();
    test_parse_for_entire_network();
    test_parse_for_control_panel();
    test_printers_folder();
    test_desktop_folder();
    test_desktop_displaynameof();
}
