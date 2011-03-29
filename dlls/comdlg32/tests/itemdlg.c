/*
 * Unit tests for the Common Item Dialog
 *
 * Copyright 2010,2011 David Hedberg
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
 *
 */

#define COBJMACROS

#include "shlobj.h"
#include "wine/test.h"

static HRESULT (WINAPI *pSHCreateShellItem)(LPCITEMIDLIST,IShellFolder*,LPCITEMIDLIST,IShellItem**);
static HRESULT (WINAPI *pSHGetIDListFromObject)(IUnknown*, PIDLIST_ABSOLUTE*);

static void init_function_pointers(void)
{
    HMODULE hmod = GetModuleHandleA("shell32.dll");

#define MAKEFUNC(f) (p##f = (void*)GetProcAddress(hmod, #f))
    MAKEFUNC(SHCreateShellItem);
    MAKEFUNC(SHGetIDListFromObject);
#undef MAKEFUNC
}

static BOOL test_instantiation(void)
{
    IFileDialog *pfd;
    IFileOpenDialog *pfod;
    IFileSaveDialog *pfsd;
    IServiceProvider *psp;
    IUnknown *punk;
    HRESULT hr;

    /* Instantiate FileOpenDialog */
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileOpenDialog, (void**)&pfod);
    if(FAILED(hr))
    {
        skip("Could not instantiate the FileOpenDialog.\n");
        return FALSE;
    }
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialog, (void**)&pfd);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileDialog_Release(pfd);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialogCustomize, (void**)&pfd);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileDialog_Release(pfd);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileSaveDialog, (void**)&pfsd);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileOpenDialog_Release(pfsd);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IServiceProvider, (void**)&psp);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        IExplorerBrowser *peb;
        IShellBrowser *psb;

        hr = IServiceProvider_QueryService(psp, &SID_STopLevelBrowser, &IID_IExplorerBrowser, (void**)&peb);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) IExplorerBrowser_Release(peb);
        hr = IServiceProvider_QueryService(psp, &SID_STopLevelBrowser, &IID_IShellBrowser, (void**)&psb);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) IShellBrowser_Release(psb);
        hr = IServiceProvider_QueryService(psp, &SID_STopLevelBrowser, &IID_ICommDlgBrowser, (void**)&punk);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) IUnknown_Release(punk);
        hr = IServiceProvider_QueryService(psp, &SID_STopLevelBrowser, &IID_ICommDlgBrowser, (void**)&punk);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        if(SUCCEEDED(hr)) IUnknown_Release(punk);

        IServiceProvider_Release(psp);
    }

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialogEvents, (void**)&pfd);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileDialogEvents_Release(pfd);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IExplorerBrowser, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IExplorerBrowserEvents, (void**)&punk);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IShellBrowser, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    IFileOpenDialog_Release(pfod);

    /* Instantiate FileSaveDialog */
    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileSaveDialog, (void**)&pfsd);
    if(FAILED(hr))
    {
        skip("Could not instantiate the FileSaveDialog.\n");
        return FALSE;
    }
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialog, (void**)&pfd);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileDialog_Release(pfd);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialogCustomize, (void**)&pfd);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileDialogCustomize_Release(pfd);

    hr = IFileOpenDialog_QueryInterface(pfsd, &IID_IFileOpenDialog, (void**)&pfod);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileOpenDialog_Release(pfod);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialogEvents, (void**)&pfd);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IFileDialogEvents_Release(pfd);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IExplorerBrowser, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IExplorerBrowserEvents, (void**)&punk);
    todo_wine ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IShellBrowser, (void**)&punk);
    ok(hr == E_NOINTERFACE, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr)) IUnknown_Release(punk);

    IFileSaveDialog_Release(pfsd);
    return TRUE;
}

static void test_basics(void)
{
    IFileOpenDialog *pfod;
    IFileSaveDialog *pfsd;
    IFileDialog2 *pfd2;
    FILEOPENDIALOGOPTIONS fdoptions;
    IShellFolder *psfdesktop;
    IShellItem *psi, *psidesktop, *psi_original;
    IShellItemArray *psia;
    IPropertyStore *pps;
    LPITEMIDLIST pidl;
    WCHAR *filename;
    UINT filetype;
    HRESULT hr;
    const WCHAR txt[] = {'t','x','t', 0};
    const WCHAR null[] = {0};
    const WCHAR fname1[] = {'f','n','a','m','e','1', 0};
    const WCHAR fspec1[] = {'*','.','t','x','t',0};
    const WCHAR fname2[] = {'f','n','a','m','e','2', 0};
    const WCHAR fspec2[] = {'*','.','e','x','e',0};
    COMDLG_FILTERSPEC filterspec[2] = {{fname1, fspec1}, {fname2, fspec2}};

    /* This should work on every platform with IFileDialog */
    SHGetDesktopFolder(&psfdesktop);
    hr = pSHGetIDListFromObject((IUnknown*)psfdesktop, &pidl);
    if(SUCCEEDED(hr))
    {
        hr = pSHCreateShellItem(NULL, NULL, pidl, &psidesktop);
        ILFree(pidl);
    }
    IShellFolder_Release(psfdesktop);
    if(FAILED(hr))
    {
        skip("Failed to get ShellItem from DesktopFolder, skipping tests.\n");
        return;
    }


    /* Instantiate FileOpenDialog and FileSaveDialog */
    hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileOpenDialog, (void**)&pfod);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = CoCreateInstance(&CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFileSaveDialog, (void**)&pfsd);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    /* ClearClientData */
    todo_wine
    {
    hr = IFileOpenDialog_ClearClientData(pfod);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_ClearClientData(pfsd);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    }

    /* GetOptions */
    hr = IFileOpenDialog_GetOptions(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_GetOptions(pfsd, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    /* Check default options */
    hr = IFileOpenDialog_GetOptions(pfod, &fdoptions);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(fdoptions == (FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_NOCHANGEDIR),
       "Unexpected default options: 0x%08x\n", fdoptions);
    hr = IFileSaveDialog_GetOptions(pfsd, &fdoptions);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(fdoptions == (FOS_OVERWRITEPROMPT | FOS_NOREADONLYRETURN | FOS_PATHMUSTEXIST | FOS_NOCHANGEDIR),
       "Unexpected default options: 0x%08x\n", fdoptions);

    /* GetResult */
    hr = IFileOpenDialog_GetResult(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_GetResult(pfsd, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    psi = (void*)0xdeadbeef;
    hr = IFileOpenDialog_GetResult(pfod, &psi);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    ok(psi == (void*)0xdeadbeef, "got %p.\n", psi);
    psi = (void*)0xdeadbeef;
    hr = IFileSaveDialog_GetResult(pfsd, &psi);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    ok(psi == (void*)0xdeadbeef, "got %p.\n", psi);

    /* GetCurrentSelection */
    if(0) {
        /* Crashes on Vista/W2K8. Tests below passes on Windows 7 */
        hr = IFileOpenDialog_GetCurrentSelection(pfod, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
        hr = IFileSaveDialog_GetCurrentSelection(pfsd, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
        hr = IFileOpenDialog_GetCurrentSelection(pfod, &psi);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        hr = IFileSaveDialog_GetCurrentSelection(pfsd, &psi);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    }

    /* GetFileName */
    todo_wine
    {
    hr = IFileOpenDialog_GetFileName(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    filename = (void*)0xdeadbeef;
    hr = IFileOpenDialog_GetFileName(pfod, &filename);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    ok(filename == NULL, "got %p\n", filename);
    hr = IFileSaveDialog_GetFileName(pfsd, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    filename = (void*)0xdeadbeef;
    hr = IFileSaveDialog_GetFileName(pfsd, &filename);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    ok(filename == NULL, "got %p\n", filename);
    }

    /* GetFileTypeIndex */
    hr = IFileOpenDialog_GetFileTypeIndex(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    filetype = 0x12345;
    hr = IFileOpenDialog_GetFileTypeIndex(pfod, &filetype);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(filetype == 0, "got %d.\n", filetype);
    hr = IFileSaveDialog_GetFileTypeIndex(pfsd, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    filetype = 0x12345;
    hr = IFileSaveDialog_GetFileTypeIndex(pfsd, &filetype);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    ok(filetype == 0, "got %d.\n", filetype);

    /* SetFileTypes / SetFileTypeIndex */
    hr = IFileOpenDialog_SetFileTypes(pfod, 0, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 0, filterspec);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileOpenDialog_SetFileTypeIndex(pfod, -1);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypeIndex(pfod, 0);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypeIndex(pfod, 1);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 1, filterspec);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 0, filterspec);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypeIndex(pfod, 0);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypeIndex(pfod, 100);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 1, filterspec);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetFileTypes(pfod, 1, &filterspec[1]);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);

    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, -1);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, 0);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, 1);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypes(pfsd, 1, filterspec);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, 0);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypeIndex(pfsd, 100);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypes(pfsd, 1, filterspec);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFileTypes(pfsd, 1, &filterspec[1]);
    ok(hr == E_UNEXPECTED, "got 0x%08x.\n", hr);

    /* SetFilter */
    todo_wine
    {
    hr = IFileOpenDialog_SetFilter(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFilter(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    }

    /* SetFolder */
    hr = IFileOpenDialog_SetFolder(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetFolder(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    /* SetDefaultExtension */
    todo_wine
    {
    hr = IFileOpenDialog_SetDefaultExtension(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetDefaultExtension(pfod, txt);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileOpenDialog_SetDefaultExtension(pfod, null);
    ok(hr == S_OK, "got 0x%08x.\n", hr);

    hr = IFileSaveDialog_SetDefaultExtension(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetDefaultExtension(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    hr = IFileSaveDialog_SetDefaultExtension(pfsd, null);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    }

    /* SetDefaultFolder */
    hr = IFileOpenDialog_SetDefaultFolder(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetDefaultFolder(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IFileOpenDialog_SetDefaultFolder(pfod, psidesktop);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetDefaultFolder(pfsd, psidesktop);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    if(0)
    {
        /* Crashes under Windows 7 */
        IFileOpenDialog_SetDefaultFolder(pfod, (void*)0x1234);
        IFileSaveDialog_SetDefaultFolder(pfsd, (void*)0x1234);
    }

    /* GetFolder / SetFolder */
    hr = IFileOpenDialog_GetFolder(pfod, NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x.\n", hr);

    hr = IFileOpenDialog_GetFolder(pfod, &psi_original);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IFileOpenDialog_SetFolder(pfod, psidesktop);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        hr = IFileOpenDialog_SetFolder(pfod, psi_original);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        IShellItem_Release(psi_original);
    }

    hr = IFileSaveDialog_GetFolder(pfsd, &psi_original);
    ok(hr == S_OK, "got 0x%08x.\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IFileSaveDialog_SetFolder(pfsd, psidesktop);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        hr = IFileSaveDialog_SetFolder(pfsd, psi_original);
        ok(hr == S_OK, "got 0x%08x.\n", hr);
        IShellItem_Release(psi_original);
    }

    /* AddPlace */
    if(0)
    {
        /* Crashes under Windows 7 */
        IFileOpenDialog_AddPlace(pfod, NULL, 0);
        IFileSaveDialog_AddPlace(pfsd, NULL, 0);
    }

    todo_wine
    {
    hr = IFileOpenDialog_AddPlace(pfod, psidesktop, FDAP_TOP + 1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_AddPlace(pfod, psidesktop, FDAP_BOTTOM);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_AddPlace(pfod, psidesktop, FDAP_TOP);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IFileSaveDialog_AddPlace(pfsd, psidesktop, FDAP_TOP + 1);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_AddPlace(pfsd, psidesktop, FDAP_BOTTOM);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_AddPlace(pfsd, psidesktop, FDAP_TOP);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetFileName */
    todo_wine
    {
    hr = IFileOpenDialog_SetFileName(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetFileName(pfod, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetFileName(pfod, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IFileSaveDialog_SetFileName(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetFileName(pfsd, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetFileName(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetFileNameLabel */
    todo_wine
    {
    hr = IFileOpenDialog_SetFileNameLabel(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetFileNameLabel(pfod, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetFileNameLabel(pfod, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IFileSaveDialog_SetFileNameLabel(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetFileNameLabel(pfsd, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetFileNameLabel(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* Close */
    todo_wine
    {
    hr = IFileOpenDialog_Close(pfod, S_FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_Close(pfsd, S_FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetOkButtonLabel */
    todo_wine
    {
    hr = IFileOpenDialog_SetOkButtonLabel(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetOkButtonLabel(pfod, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetOkButtonLabel(pfod, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetOkButtonLabel(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetOkButtonLabel(pfsd, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetOkButtonLabel(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetTitle */
    todo_wine
    {
    hr = IFileOpenDialog_SetTitle(pfod, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetTitle(pfod, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileOpenDialog_SetTitle(pfod, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetTitle(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetTitle(pfsd, null);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetTitle(pfsd, txt);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /** IFileOpenDialog specific **/

    /* GetResults */
    if(0)
    {
        /* Crashes under Windows 7 */
        IFileOpenDialog_GetResults(pfod, NULL);
    }
    psia = (void*)0xdeadbeef;
    hr = IFileOpenDialog_GetResults(pfod, &psia);
    ok(hr == E_FAIL, "got 0x%08x.\n", hr);
    ok(psia == NULL, "got %p.\n", psia);

    /* GetSelectedItems */
    if(0)
    {
        /* Crashes under W2K8 */
        hr = IFileOpenDialog_GetSelectedItems(pfod, NULL);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        psia = (void*)0xdeadbeef;
        hr = IFileOpenDialog_GetSelectedItems(pfod, &psia);
        ok(hr == E_FAIL, "got 0x%08x.\n", hr);
        ok(psia == (void*)0xdeadbeef, "got %p.\n", psia);
    }

    /** IFileSaveDialog specific **/

    /* ApplyProperties */
    if(0)
    {
        /* Crashes under windows 7 */
        IFileSaveDialog_ApplyProperties(pfsd, NULL, NULL, NULL, NULL);
        IFileSaveDialog_ApplyProperties(pfsd, psidesktop, NULL, NULL, NULL);
    }

    /* GetProperties */
    hr = IFileSaveDialog_GetProperties(pfsd, NULL);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);
    pps = (void*)0xdeadbeef;
    hr = IFileSaveDialog_GetProperties(pfsd, &pps);
    todo_wine ok(hr == E_UNEXPECTED, "got 0x%08x\n", hr);
    ok(pps == (void*)0xdeadbeef, "got %p\n", pps);

    /* SetProperties */
    if(0)
    {
        /* Crashes under W2K8 */
        hr = IFileSaveDialog_SetProperties(pfsd, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetCollectedProperties */
    todo_wine
    {
    hr = IFileSaveDialog_SetCollectedProperties(pfsd, NULL, TRUE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetCollectedProperties(pfsd, NULL, FALSE);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    }

    /* SetSaveAsItem */
    todo_wine
    {
    hr = IFileSaveDialog_SetSaveAsItem(pfsd, NULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    hr = IFileSaveDialog_SetSaveAsItem(pfsd, psidesktop);
    ok(hr == MK_E_NOOBJECT, "got 0x%08x\n", hr);
    }

    /** IFileDialog2 **/

    hr = IFileOpenDialog_QueryInterface(pfod, &IID_IFileDialog2, (void**)&pfd2);
    ok((hr == S_OK) || broken(hr == E_NOINTERFACE), "got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        /* SetCancelButtonLabel */
        todo_wine
        {
        hr = IFileDialog2_SetOkButtonLabel(pfd2, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetOkButtonLabel(pfd2, null);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetOkButtonLabel(pfd2, txt);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        }

        /* SetNavigationRoot */
        todo_wine
        {
        hr = IFileDialog2_SetNavigationRoot(pfd2, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetNavigationRoot(pfd2, psidesktop);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        }

        IFileDialog2_Release(pfd2);
    }

    hr = IFileSaveDialog_QueryInterface(pfsd, &IID_IFileDialog2, (void**)&pfd2);
    ok((hr == S_OK) || broken(hr == E_NOINTERFACE), "got 0x%08x\n", hr);
    if(SUCCEEDED(hr))
    {
        /* SetCancelButtonLabel */
        todo_wine
        {
        hr = IFileDialog2_SetOkButtonLabel(pfd2, NULL);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetOkButtonLabel(pfd2, null);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetOkButtonLabel(pfd2, txt);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        }

        /* SetNavigationRoot */
        todo_wine
        {
        hr = IFileDialog2_SetNavigationRoot(pfd2, NULL);
        ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
        hr = IFileDialog2_SetNavigationRoot(pfd2, psidesktop);
        ok(hr == S_OK, "got 0x%08x\n", hr);
        }

        IFileDialog2_Release(pfd2);
    }

    /* Cleanup */
    IShellItem_Release(psidesktop);
    IFileOpenDialog_Release(pfod);
    IFileSaveDialog_Release(pfsd);
}

START_TEST(itemdlg)
{
    OleInitialize(NULL);
    init_function_pointers();

    if(test_instantiation())
    {
        test_basics();
    }
    else
        skip("Skipping all Item Dialog tests.\n");

    OleUninitialize();
}
