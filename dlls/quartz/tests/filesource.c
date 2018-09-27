/*
 * File source filter unit tests
 *
 * Copyright 2016 Sebastian Lackner
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
#include "dshow.h"
#include "wine/test.h"

static void test_file_source_filter(void)
{
    static const WCHAR prefix[] = {'w','i','n',0};
    static const struct
    {
        const char *label;
        const char *data;
        DWORD size;
        const GUID *subtype;
    }
    tests[] =
    {
        {
            "AVI",
            "\x52\x49\x46\x46xxxx\x41\x56\x49\x20",
            12,
            &MEDIASUBTYPE_Avi,
        },
        {
            "MPEG1 System",
            "\x00\x00\x01\xBA\x21\x00\x01\x00\x01\x80\x00\x01\x00\x00\x01\xBB",
            16,
            &MEDIASUBTYPE_MPEG1System,
        },
        {
            "MPEG1 Video",
            "\x00\x00\x01\xB3",
            4,
            &MEDIASUBTYPE_MPEG1Video,
        },
        {
            "MPEG1 Audio",
            "\xFF\xE0",
            2,
            &MEDIASUBTYPE_MPEG1Audio,
        },
        {
            "MPEG2 Program",
            "\x00\x00\x01\xBA\x40",
            5,
            &MEDIASUBTYPE_MPEG2_PROGRAM,
        },
        {
            "WAVE",
            "\x52\x49\x46\x46xxxx\x57\x41\x56\x45",
            12,
            &MEDIASUBTYPE_WAVE,
        },
        {
            "unknown format",
            "Hello World",
            11,
            NULL, /* FIXME: should be &MEDIASUBTYPE_NULL */
        },
    };
    WCHAR path[MAX_PATH], temp[MAX_PATH];
    IFileSourceFilter *filesource;
    IBaseFilter *filter;
    AM_MEDIA_TYPE mt;
    OLECHAR *olepath;
    DWORD written;
    HANDLE file;
    HRESULT hr;
    BOOL ret;
    int i;

    GetTempPathW(MAX_PATH, temp);
    GetTempFileNameW(temp, prefix, 0, path);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        trace("Running test for %s.\n", tests[i].label);

        file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        ok(file != INVALID_HANDLE_VALUE, "Failed to create file, error %u.\n", GetLastError());
        ret = WriteFile(file, tests[i].data, tests[i].size, &written, NULL);
        ok(ret, "Failed to write file, error %u.\n", GetLastError());
        CloseHandle(file);

        CoCreateInstance(&CLSID_AsyncReader, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (void **)&filter);
        IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource);

        olepath = (void *)0xdeadbeef;
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(!olepath, "Got path %s.\n", wine_dbgstr_w(olepath));

        hr = IFileSourceFilter_Load(filesource, NULL, NULL);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);

        hr = IFileSourceFilter_Load(filesource, path, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);

        hr = IFileSourceFilter_GetCurFile(filesource, NULL, &mt);
        ok(hr == E_POINTER, "Got hr %#x.\n", hr);

        olepath = NULL;
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, NULL);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        CoTaskMemFree(olepath);

        olepath = NULL;
        memset(&mt, 0x11, sizeof(mt));
        hr = IFileSourceFilter_GetCurFile(filesource, &olepath, &mt);
        ok(hr == S_OK, "Got hr %#x.\n", hr);
        ok(!lstrcmpW(olepath, path), "Expected path %s, got %s.\n",
                wine_dbgstr_w(path), wine_dbgstr_w(olepath));
        ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Stream), "Got major type %s.\n",
                wine_dbgstr_guid(&mt.majortype));
        if (tests[i].subtype)
            ok(IsEqualGUID(&mt.subtype, tests[i].subtype), "Expected subtype %s, got %s.\n",
                    wine_dbgstr_guid(tests[i].subtype), wine_dbgstr_guid(&mt.subtype));
        CoTaskMemFree(olepath);

        IFileSourceFilter_Release(filesource);
        IBaseFilter_Release(filter);

        ret = DeleteFileW(path);
        ok(ret, "Failed to delete file, error %u\n", GetLastError());
    }
}

START_TEST(filesource)
{
    CoInitialize(NULL);

    test_file_source_filter();

    CoUninitialize();
}
