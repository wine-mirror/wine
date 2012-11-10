/*
 *
 * Copyright 2012 Alistair Leslie-Hughes
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
#include <stdio.h>

#include "windows.h"
#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"
#include "dispex.h"

#include "wine/test.h"

#include "initguid.h"
#include "scrrun.h"

static IFileSystem3 *fs3;

static void test_interfaces(void)
{
    static const WCHAR nonexistent_dirW[] = {
        'c', ':', '\\', 'N', 'o', 'n', 'e', 'x', 'i', 's', 't', 'e', 'n', 't', 0};
    static const WCHAR pathW[] = {'p','a','t','h',0};
    static const WCHAR file_kernel32W[] = {
        '\\', 'k', 'e', 'r', 'n', 'e', 'l', '3', '2', '.', 'd', 'l', 'l', 0};
    HRESULT hr;
    IDispatch *disp;
    IDispatchEx *dispex;
    IObjectWithSite *site;
    VARIANT_BOOL b;
    BSTR path;
    WCHAR windows_path[MAX_PATH];
    WCHAR file_path[MAX_PATH];

    IFileSystem3_QueryInterface(fs3, &IID_IDispatch, (void**)&disp);

    GetSystemDirectoryW(windows_path, MAX_PATH);
    lstrcpyW(file_path, windows_path);
    lstrcatW(file_path, file_kernel32W);

    hr = IDispatch_QueryInterface(disp, &IID_IObjectWithSite, (void**)&site);
    ok(hr == E_NOINTERFACE, "got 0x%08x, expected 0x%08x\n", hr, E_NOINTERFACE);

    hr = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
    ok(hr == E_NOINTERFACE, "got 0x%08x, expected 0x%08x\n", hr, E_NOINTERFACE);

    b = VARIANT_TRUE;
    hr = IFileSystem3_FileExists(fs3, NULL, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "got %x\n", b);

    hr = IFileSystem3_FileExists(fs3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x, expected 0x%08x\n", hr, E_POINTER);

    path = SysAllocString(pathW);
    b = VARIANT_TRUE;
    hr = IFileSystem3_FileExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "got %x\n", b);
    SysFreeString(path);

    path = SysAllocString(file_path);
    b = VARIANT_FALSE;
    hr = IFileSystem3_FileExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_TRUE, "got %x\n", b);
    SysFreeString(path);

    path = SysAllocString(windows_path);
    b = VARIANT_TRUE;
    hr = IFileSystem3_FileExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "got %x\n", b);
    SysFreeString(path);

    /* Folder Exists */
    hr = IFileSystem3_FolderExists(fs3, NULL, NULL);
    ok(hr == E_POINTER, "got 0x%08x, expected 0x%08x\n", hr, E_POINTER);

    path = SysAllocString(windows_path);
    hr = IFileSystem3_FolderExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_TRUE, "Folder doesn't exists\n");
    SysFreeString(path);

    path = SysAllocString(nonexistent_dirW);
    hr = IFileSystem3_FolderExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "Folder exists\n");
    SysFreeString(path);

    path = SysAllocString(file_path);
    hr = IFileSystem3_FolderExists(fs3, path, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_FALSE, "Folder exists\n");
    SysFreeString(path);

    IDispatch_Release(disp);
}

static void test_createfolder(void)
{
    HRESULT hr;
    WCHAR pathW[MAX_PATH];
    BSTR path;
    IFolder *folder;

    /* create existing directory */
    GetCurrentDirectoryW(sizeof(pathW)/sizeof(WCHAR), pathW);
    path = SysAllocString(pathW);
    folder = (void*)0xdeabeef;
    hr = IFileSystem3_CreateFolder(fs3, path, &folder);
    ok(hr == CTL_E_FILEALREADYEXISTS, "got 0x%08x\n", hr);
    ok(folder == NULL, "got %p\n", folder);
    SysFreeString(path);
}

static void test_textstream(void)
{
    static WCHAR testfileW[] = {'t','e','s','t','f','i','l','e','.','t','x','t',0};
    ITextStream *stream;
    VARIANT_BOOL b;
    HANDLE file;
    HRESULT hr;
    BSTR name, data;

    file = CreateFileW(testfileW, GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CloseHandle(file);

    name = SysAllocString(testfileW);
    b = VARIANT_FALSE;
    hr = IFileSystem3_FileExists(fs3, name, &b);
    ok(hr == S_OK, "got 0x%08x, expected 0x%08x\n", hr, S_OK);
    ok(b == VARIANT_TRUE, "got %x\n", b);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForReading, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    b = 10;
    hr = ITextStream_get_AtEndOfStream(stream, &b);
todo_wine {
    ok(hr == S_FALSE || broken(hr == S_OK), "got 0x%08x\n", hr);
    ok(b == VARIANT_TRUE, "got 0x%x\n", b);
}
    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForWriting, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    b = 10;
    hr = ITextStream_get_AtEndOfStream(stream, &b);
todo_wine {
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(b == VARIANT_TRUE || broken(b == 10), "got 0x%x\n", b);
}
    b = 10;
    hr = ITextStream_get_AtEndOfLine(stream, &b);
todo_wine {
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(b == VARIANT_FALSE || broken(b == 10), "got 0x%x\n", b);
}
    hr = ITextStream_Read(stream, 1, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_ReadLine(stream, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_ReadAll(stream, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    ITextStream_Release(stream);

    hr = IFileSystem3_OpenTextFile(fs3, name, ForAppending, VARIANT_FALSE, TristateFalse, &stream);
    ok(hr == S_OK, "got 0x%08x\n", hr);
    SysFreeString(name);

    b = 10;
    hr = ITextStream_get_AtEndOfStream(stream, &b);
todo_wine {
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(b == VARIANT_TRUE || broken(b == 10), "got 0x%x\n", b);
}
    b = 10;
    hr = ITextStream_get_AtEndOfLine(stream, &b);
todo_wine {
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);
    ok(b == VARIANT_FALSE || broken(b == 10), "got 0x%x\n", b);
}
    hr = ITextStream_Read(stream, 1, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_ReadLine(stream, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    hr = ITextStream_ReadAll(stream, &data);
    ok(hr == CTL_E_BADFILEMODE, "got 0x%08x\n", hr);

    ITextStream_Release(stream);

    DeleteFileW(testfileW);
}

START_TEST(filesystem)
{
    HRESULT hr;

    CoInitialize(NULL);

    hr = CoCreateInstance(&CLSID_FileSystemObject, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
            &IID_IFileSystem3, (void**)&fs3);
    if(FAILED(hr)) {
        win_skip("Could not create FileSystem object: %08x\n", hr);
        return;
    }

    test_interfaces();
    test_createfolder();
    test_textstream();

    IFileSystem3_Release(fs3);

    CoUninitialize();
}
