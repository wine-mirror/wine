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
#include "wine/heap.h"

static WCHAR temp_dir[MAX_PATH];

static BOOL create_file(const WCHAR *filename, const char *data, unsigned int size, WCHAR *out_path)
{
    WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    if (WriteFile(file, data, size, &written, NULL))
    {
        CloseHandle(file);

        if (out_path)
            lstrcpyW(out_path, path);
        return TRUE;
    }

    CloseHandle(file);
    return FALSE;
}

static void delete_file(const WCHAR *filename)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    DeleteFileW(path);
}

static BOOL create_directory(const WCHAR *dir)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, dir);
    return CreateDirectoryW(path, NULL);
}

static void delete_directory(const WCHAR *dir)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, dir);
    RemoveDirectoryW(path);
}

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
    DWORD written;
    HANDLE file;
    BOOL ret;

    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"asyncloader.data");

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

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, L"noname", &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#x.\n", hr);
}

static HRESULT WINAPI test_d3dinclude_open(ID3DInclude *iface, D3D_INCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    static const char include1[] =
        "#define LIGHT float4(0.0f, 0.2f, 0.5f, 1.0f)\n";
    static const char include2[] =
        "#include \"include1.h\"\n"
        "float4 light_color = LIGHT;\n";
    char *buffer;

    trace("filename %s.\n", filename);
    trace("parent_data %p: %s.\n", parent_data, parent_data ? (char *)parent_data : "(null)");

    if (!strcmp(filename, "include1.h"))
    {
        buffer = heap_alloc(strlen(include1));
        CopyMemory(buffer, include1, strlen(include1));
        *bytes = strlen(include1);
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
        ok(!strncmp(include2, parent_data, strlen(include2)),
                "Unexpected parent_data value.\n");
    }
    else if (!strcmp(filename, "include\\include2.h"))
    {
        buffer = heap_alloc(strlen(include2));
        CopyMemory(buffer, include2, strlen(include2));
        *bytes = strlen(include2);
        ok(!parent_data, "Unexpected parent_data value.\n");
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
    }
    else
    {
        ok(0, "Unexpected #include for file %s.\n", filename);
        return E_INVALIDARG;
    }

    *data = buffer;
    return S_OK;
}

static HRESULT WINAPI test_d3dinclude_close(ID3DInclude *iface, const void *data)
{
    heap_free((void *)data);
    return S_OK;
}

static const struct ID3DIncludeVtbl test_d3dinclude_vtbl =
{
    test_d3dinclude_open,
    test_d3dinclude_close
};

struct test_d3dinclude
{
    ID3DInclude ID3DInclude_iface;
};

static void test_D3DX11CompileFromFile(void)
{
    struct test_d3dinclude include = {{&test_d3dinclude_vtbl}};
    WCHAR filename[MAX_PATH], directory[MAX_PATH];
    ID3D10Blob *blob = NULL, *errors = NULL;
    CHAR filename_a[MAX_PATH];
    HRESULT hr, result;
    DWORD len;
    static const char ps_code[] =
        "#include \"include\\include2.h\"\n"
        "\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return light_color;\n"
        "}";
    static const char include1[] =
        "#define LIGHT float4(0.0f, 0.2f, 0.5f, 1.0f)\n";
    static const char include1_wrong[] =
        "#define LIGHT nope\n";
    static const char include2[] =
        "#include \"include1.h\"\n"
        "float4 light_color = LIGHT;\n";

    create_file(L"source.ps", ps_code, strlen(ps_code), filename);
    create_directory(L"include");
    create_file(L"include\\include1.h", include1_wrong, strlen(include1_wrong), NULL);
    create_file(L"include1.h", include1, strlen(include1), NULL);
    create_file(L"include\\include2.h", include2, strlen(include2), NULL);

    hr = D3DX11CompileFromFileW(filename, NULL, &include.ID3DInclude_iface, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    todo_wine ok(hr == S_OK && hr == result, "Got hr %#x, result %#x.\n", hr, result);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    /* Windows always seems to resolve includes from the initial file location
     * instead of using the immediate parent, as it would be the case for
     * standard C preprocessor includes. */
    hr = D3DX11CompileFromFileW(filename, NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    todo_wine ok(hr == S_OK && hr == result, "Got hr %#x, result %#x.\n", hr, result);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, len, NULL, NULL);
    hr = D3DX11CompileFromFileA(filename_a, NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    todo_wine ok(hr == S_OK && hr == result, "Got hr %#x, result %#x.\n", hr, result);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    GetCurrentDirectoryW(MAX_PATH, directory);
    SetCurrentDirectoryW(temp_dir);

    hr = D3DX11CompileFromFileW(L"source.ps", NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    todo_wine ok(hr == S_OK && hr == result, "Got hr %#x, result %#x.\n", hr, result);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    ok(!errors, "Got unexpected errors.\n");
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    SetCurrentDirectoryW(directory);

    delete_file(L"source.ps");
    delete_file(L"include\\include1.h");
    delete_file(L"include1.h");
    delete_file(L"include\\include2.h");
    delete_directory(L"include");
}

START_TEST(d3dx11)
{
    test_D3DX11CreateAsyncMemoryLoader();
    test_D3DX11CreateAsyncFileLoader();
    test_D3DX11CreateAsyncResourceLoader();
    test_D3DX11CompileFromFile();
}
