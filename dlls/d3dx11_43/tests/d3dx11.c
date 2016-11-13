/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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
#include "initguid.h"
#include "d3d11.h"
#include "d3dx11.h"
#include "wine/test.h"

static void test_D3DX11CreateAsyncMemoryLoader(void)
{
    ID3DX11DataLoader *loader;
    SIZE_T size;
    DWORD data;
    HRESULT hr;
    void *ptr;

    hr = D3DX11CreateAsyncMemoryLoader(NULL, 0, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncMemoryLoader(NULL, 0, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncMemoryLoader(&data, 0, &loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    size = 100;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(!size, "Got unexpected data size.\n");

    /* Load() is no-op. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    data = 0;
    hr = D3DX11CreateAsyncMemoryLoader(&data, sizeof(data), &loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Load() is no-op. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(size == sizeof(data), "Got unexpected data size.\n");

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
}

static void create_testfile(WCHAR *path, const void *data, int data_len)
{
    static const WCHAR test_filename[] = {'a','s','y','n','c','l','o','a','d','e','r','.','d','a','t','a',0};
    DWORD written;
    HANDLE file;
    BOOL ret;

    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, test_filename);

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Test file creation failed, at %s, error %d.\n",
            wine_dbgstr_w(path), GetLastError());

    ret = WriteFile(file, data, data_len, &written, NULL);
    ok(ret, "Write to test file failed.\n");

    CloseHandle(file);
}

static void test_D3DX11CreateAsyncFileLoader(void)
{
    static const char test_data1[] = "test data";
    static const char test_data2[] = "more test data";
    ID3DX11DataLoader *loader;
    WCHAR path[MAX_PATH];
    SIZE_T size;
    HRESULT hr;
    void *ptr;
    BOOL ret;

    hr = D3DX11CreateAsyncFileLoaderA(NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncFileLoaderA(NULL, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncFileLoaderA("nonexistentfilename", &loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    /* Test file sharing using dummy empty file. */
    create_testfile(path, test_data1, sizeof(test_data1));

    hr = D3DX11CreateAsyncFileLoaderW(path, &loader);
    ok(SUCCEEDED(hr), "Failed to create file loader, hr %#x.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "DeleteFile() failed, ret %d, error %d.\n", ret, GetLastError());

    /* File was removed before Load(). */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Load() returned unexpected result, hr %#x.\n", hr);

    /* Create it again. */
    create_testfile(path, test_data1, sizeof(test_data1));
    hr = ID3DX11DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#x.\n", hr);

    /* Already loaded. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#x.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "DeleteFile() failed, ret %d, error %d.\n", ret, GetLastError());

    /* Already loaded, file removed. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Load() returned unexpected result, hr %#x.\n", hr);

    /* Decompress still works. */
    ptr = NULL;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(SUCCEEDED(hr), "Decompress() failed, hr %#x.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data1), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data1))
        ok(!memcmp(ptr, test_data1, size), "Got unexpected file data.\n");

    /* Create it again, with different data. */
    create_testfile(path, test_data2, sizeof(test_data2));

    hr = ID3DX11DataLoader_Load(loader);
    ok(SUCCEEDED(hr), "Load() failed, hr %#x.\n", hr);

    ptr = NULL;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(SUCCEEDED(hr), "Decompress() failed, hr %#x.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data2), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data2))
        ok(!memcmp(ptr, test_data2, size), "Got unexpected file data.\n");

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(SUCCEEDED(hr), "Destroy() failed, hr %#x.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "DeleteFile() failed, ret %d, error %d.\n", ret, GetLastError());
}

static void test_D3DX11CreateAsyncResourceLoader(void)
{
    static const WCHAR resource_name[] = {'n','o','n','a','m','e',0};
    ID3DX11DataLoader *loader;
    HRESULT hr;

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, NULL, &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, "noname", &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, NULL, &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, resource_name, &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);
}

START_TEST(d3dx11)
{
    test_D3DX11CreateAsyncMemoryLoader();
    test_D3DX11CreateAsyncFileLoader();
    test_D3DX11CreateAsyncResourceLoader();
}
