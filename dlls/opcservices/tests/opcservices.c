/*
 *    Font related tests
 *
 * Copyright 2012, 2014-2017 Nikolay Sivov for CodeWeavers
 * Copyright 2014 Aric Stewart for CodeWeavers
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

#include "windows.h"
#include "initguid.h"
#include "msopc.h"

#include "wine/test.h"

static IOpcFactory *create_factory(void)
{
    IOpcFactory *factory = NULL;
    CoCreateInstance(&CLSID_OpcFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IOpcFactory, (void **)&factory);
    return factory;
}

static void test_package(void)
{
    static const WCHAR typeW[] = {'t','y','p','e','/','s','u','b','t','y','p','e',0};
    static const WCHAR uriW[] = {'/','u','r','i',0};
    IOpcRelationshipSet *relset, *relset2;
    IOpcPartSet *partset, *partset2;
    IOpcPartUri *part_uri;
    IOpcFactory *factory;
    IOpcPackage *package;
    IOpcPart *part;
    HRESULT hr;
    BOOL ret;

    factory = create_factory();

    hr = IOpcFactory_CreatePackage(factory, &package);
    ok(SUCCEEDED(hr) || broken(hr == E_NOTIMPL) /* Vista */, "Failed to create a package, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        IOpcFactory_Release(factory);
        return;
    }

    hr = IOpcPackage_GetPartSet(package, &partset);
    ok(SUCCEEDED(hr), "Failed to create a part set, hr %#x.\n", hr);

    hr = IOpcPackage_GetPartSet(package, &partset2);
    ok(SUCCEEDED(hr), "Failed to create a part set, hr %#x.\n", hr);
    ok(partset == partset2, "Expected same part set instance.\n");

    /* CreatePart */
    hr = IOpcFactory_CreatePartUri(factory, uriW, &part_uri);
    ok(SUCCEEDED(hr), "Failed to create part uri, hr %#x.\n", hr);

    hr = IOpcPartSet_CreatePart(partset, NULL, typeW, OPC_COMPRESSION_NONE, &part);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    hr = IOpcPartSet_CreatePart(partset, part_uri, typeW, OPC_COMPRESSION_NONE, &part);
    ok(SUCCEEDED(hr), "Failed to create a part, hr %#x.\n", hr);

    hr = IOpcPart_GetRelationshipSet(part, &relset);
    ok(SUCCEEDED(hr), "Failed to get relationship set, hr %#x.\n", hr);

    hr = IOpcPart_GetRelationshipSet(part, &relset2);
    ok(SUCCEEDED(hr), "Failed to get relationship set, hr %#x.\n", hr);
    ok(relset == relset2, "Expected same part set instance.\n");
    IOpcRelationshipSet_Release(relset);
    IOpcRelationshipSet_Release(relset2);

    ret = FALSE;
    hr = IOpcPartSet_PartExists(partset, part_uri, &ret);
todo_wine {
    ok(SUCCEEDED(hr), "Unexpected hr %#x.\n", hr);
    ok(ret, "Expected part to exist.\n");
}
    IOpcPartUri_Release(part_uri);
    IOpcPart_Release(part);

    /* Relationships */
    hr = IOpcPackage_GetRelationshipSet(package, &relset);
    ok(SUCCEEDED(hr), "Failed to get relationship set, hr %#x.\n", hr);

    hr = IOpcPackage_GetRelationshipSet(package, &relset2);
    ok(SUCCEEDED(hr), "Failed to get relationship set, hr %#x.\n", hr);
    ok(relset == relset2, "Expected same part set instance.\n");
    IOpcRelationshipSet_Release(relset);
    IOpcRelationshipSet_Release(relset2);

    IOpcPackage_Release(package);

    IOpcFactory_Release(factory);
}

#define test_stream_stat(stream, size) test_stream_stat_(__LINE__, stream, size)
static void test_stream_stat_(unsigned int line, IStream *stream, ULONG size)
{
    STATSTG statstg;
    HRESULT hr;

    hr = IStream_Stat(stream, NULL, 0);
    ok_(__FILE__, line)(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    memset(&statstg, 0xff, sizeof(statstg));
    hr = IStream_Stat(stream, &statstg, 0);
    ok_(__FILE__, line)(SUCCEEDED(hr), "Failed to get stat info, hr %#x.\n", hr);

    ok_(__FILE__, line)(statstg.pwcsName == NULL, "Unexpected name %s.\n", wine_dbgstr_w(statstg.pwcsName));
    ok_(__FILE__, line)(statstg.type == STGTY_STREAM, "Unexpected type.\n");
    ok_(__FILE__, line)(statstg.cbSize.QuadPart == size, "Unexpected size %u, expected %u.\n",
            statstg.cbSize.LowPart, size);
    ok_(__FILE__, line)(statstg.grfMode == STGM_READ, "Unexpected mode.\n");
    ok_(__FILE__, line)(statstg.grfLocksSupported == 0, "Unexpected lock mode.\n");
    ok_(__FILE__, line)(statstg.grfStateBits == 0, "Unexpected state bits.\n");
}

static void test_file_stream(void)
{
    static const WCHAR filereadW[] = {'o','p','c','f','i','l','e','r','e','a','d','.','e','x','t',0};
    WCHAR temppathW[MAX_PATH], pathW[MAX_PATH];
    IOpcFactory *factory;
    LARGE_INTEGER move;
    IStream *stream;
    char buff[64];
    HRESULT hr;
    ULONG size;

    factory = create_factory();

    hr = IOpcFactory_CreateStreamOnFile(factory, NULL, OPC_STREAM_IO_READ, NULL, 0, &stream);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    GetTempPathW(ARRAY_SIZE(temppathW), temppathW);
    lstrcpyW(pathW, temppathW);
    lstrcatW(pathW, filereadW);
    DeleteFileW(pathW);

    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_READ, NULL, 0, NULL);
    ok(hr == E_POINTER, "Unexpected hr %#x.\n", hr);

    /* File does not exist */
    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_READ, NULL, 0, &stream);
    ok(FAILED(hr), "Unexpected hr %#x.\n", hr);

    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_WRITE, NULL, 0, &stream);
    ok(SUCCEEDED(hr), "Failed to create a write stream, hr %#x.\n", hr);

    test_stream_stat(stream, 0);

    size = lstrlenW(pathW) * sizeof(WCHAR);
    hr = IStream_Write(stream, pathW, size, NULL);
    ok(hr == S_OK, "Stream write failed, hr %#x.\n", hr);

    test_stream_stat(stream, size);
    IStream_Release(stream);

    /* Invalid I/O mode */
    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, 10, NULL, 0, &stream);
    ok(hr == E_INVALIDARG, "Failed to create a write stream, hr %#x.\n", hr);

    /* Write to read-only stream. */
    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_READ, NULL, 0, &stream);
    ok(SUCCEEDED(hr), "Failed to create a read stream, hr %#x.\n", hr);

    test_stream_stat(stream, size);
    hr = IStream_Write(stream, pathW, size, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), "Stream write failed, hr %#x.\n", hr);
    IStream_Release(stream);

    /* Read from write-only stream. */
    hr = IOpcFactory_CreateStreamOnFile(factory, pathW, OPC_STREAM_IO_WRITE, NULL, 0, &stream);
    ok(SUCCEEDED(hr), "Failed to create a read stream, hr %#x.\n", hr);

    test_stream_stat(stream, 0);
    hr = IStream_Write(stream, pathW, size, NULL);
    ok(hr == S_OK, "Stream write failed, hr %#x.\n", hr);
    test_stream_stat(stream, size);

    move.QuadPart = 0;
    hr = IStream_Seek(stream, move, STREAM_SEEK_SET, NULL);
    ok(SUCCEEDED(hr), "Seek failed, hr %#x.\n", hr);

    hr = IStream_Read(stream, buff, sizeof(buff), NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED), "Stream read failed, hr %#x.\n", hr);

    IStream_Release(stream);

    IOpcFactory_Release(factory);
    DeleteFileW(pathW);
}

START_TEST(opcservices)
{
    IOpcFactory *factory;
    HRESULT hr;

    hr = CoInitialize(NULL);
    ok(SUCCEEDED(hr), "Failed to initialize COM, hr %#x.\n", hr);

    if (!(factory = create_factory())) {
        win_skip("Failed to create IOpcFactory factory.\n");
        CoUninitialize();
        return;
    }

    test_package();
    test_file_stream();

    IOpcFactory_Release(factory);

    CoUninitialize();
}
