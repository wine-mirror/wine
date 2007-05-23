/*
 * Implementation of hyperlinking (hlink.dll)
 *
 * Copyright 2006 Mike McCormack
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

#include <hlink.h>
#include <hlguids.h>

#include "wine/test.h"

static void test_HlinkIsShortcut(void)
{
    int i;
    HRESULT hres;

    static const WCHAR file0[] = {'f','i','l','e',0};
    static const WCHAR file1[] = {'f','i','l','e','.','u','r','l',0};
    static const WCHAR file2[] = {'f','i','l','e','.','l','n','k',0};
    static const WCHAR file3[] = {'f','i','l','e','.','u','R','l',0};
    static const WCHAR file4[] = {'f','i','l','e','u','r','l',0};
    static const WCHAR file5[] = {'c',':','\\','f','i','l','e','.','u','r','l',0};
    static const WCHAR file6[] = {'c',':','\\','f','i','l','e','.','l','n','k',0};
    static const WCHAR file7[] = {'.','u','r','l',0};

    static struct {
        LPCWSTR file;
        HRESULT hres;
    } shortcut_test[] = {
        {file0, S_FALSE},
        {file1, S_OK},
        {file2, S_FALSE},
        {file3, S_OK},
        {file4, S_FALSE},
        {file5, S_OK},
        {file6, S_FALSE},
        {file7, S_OK},
        {NULL,  E_INVALIDARG}
    };

    for(i=0; i<sizeof(shortcut_test)/sizeof(shortcut_test[0]); i++) {
        hres = HlinkIsShortcut(shortcut_test[i].file);
        ok(hres == shortcut_test[i].hres, "[%d] HlinkIsShortcut returned %08x, expected %08x\n",
           i, hres, shortcut_test[i].hres);
    }
}

static void test_reference(void)
{
    HRESULT r;
    IHlink *lnk = NULL;
    IMoniker *mk = NULL;
    const WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',0 };
    const WCHAR url2[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g','/',0 };
    LPWSTR str = NULL;

    r = HlinkCreateFromString(url, NULL, NULL, NULL,
                              0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(r == S_OK, "failed to create link\n");
    if (FAILED(r))
        return;

    r = IHlink_GetMonikerReference(lnk, HLINKGETREF_DEFAULT, NULL, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetMonikerReference(lnk, HLINKGETREF_DEFAULT, &mk, &str);
    ok(r == S_OK, "failed\n");
    ok(mk != NULL, "no moniker\n");
    ok(str == NULL, "string should be null\n");

    r = IMoniker_Release(mk);
    ok( r == 1, "moniker refcount wrong\n");

    r = IHlink_GetStringReference(lnk, -1, &str, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, &str, NULL);
    ok(r == S_OK, "failed\n");
    todo_wine {
    ok(!lstrcmpW(str, url2), "url wrong\n");
    }
    CoTaskMemFree(str);

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, NULL, NULL);
    ok(r == S_OK, "failed\n");

    r = IHlink_GetStringReference(lnk, HLINKGETREF_DEFAULT, NULL, &str);
    ok(r == S_OK, "failed\n");
    ok(str == NULL, "string should be null\n");
}

/* url only */
static const unsigned char expected_hlink_data[] =
{
    0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0xe0,0xc9,0xea,0x79,0xf9,0xba,0xce,0x11,
    0x8c,0x82,0x00,0xaa,0x00,0x4b,0xa9,0x0b,
    0x26,0x00,0x00,0x00,0x68,0x00,0x74,0x00,
    0x74,0x00,0x70,0x00,0x3a,0x00,0x2f,0x00,
    0x2f,0x00,0x77,0x00,0x69,0x00,0x6e,0x00,
    0x65,0x00,0x68,0x00,0x71,0x00,0x2e,0x00,
    0x6f,0x00,0x72,0x00,0x67,0x00,0x2f,0x00,
    0x00,0x00,
};

/* url + friendly name */
static const unsigned char expected_hlink_data2[] =
{
    0x02,0x00,0x00,0x00,0x17,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x57,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x20,0x00,0x48,0x00,
    0x51,0x00,0x00,0x00,0xe0,0xc9,0xea,0x79,
    0xf9,0xba,0xce,0x11,0x8c,0x82,0x00,0xaa,
    0x00,0x4b,0xa9,0x0b,0x26,0x00,0x00,0x00,
    0x68,0x00,0x74,0x00,0x74,0x00,0x70,0x00,
    0x3a,0x00,0x2f,0x00,0x2f,0x00,0x77,0x00,
    0x69,0x00,0x6e,0x00,0x65,0x00,0x68,0x00,
    0x71,0x00,0x2e,0x00,0x6f,0x00,0x72,0x00,
    0x67,0x00,0x2f,0x00,0x00,0x00,
};

/* url + friendly name + location */
static const unsigned char expected_hlink_data3[] =
{
    0x02,0x00,0x00,0x00,0x1f,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x57,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x20,0x00,0x48,0x00,
    0x51,0x00,0x00,0x00,0xe0,0xc9,0xea,0x79,
    0xf9,0xba,0xce,0x11,0x8c,0x82,0x00,0xaa,
    0x00,0x4b,0xa9,0x0b,0x26,0x00,0x00,0x00,
    0x68,0x00,0x74,0x00,0x74,0x00,0x70,0x00,
    0x3a,0x00,0x2f,0x00,0x2f,0x00,0x77,0x00,
    0x69,0x00,0x6e,0x00,0x65,0x00,0x68,0x00,
    0x71,0x00,0x2e,0x00,0x6f,0x00,0x72,0x00,
    0x67,0x00,0x2f,0x00,0x00,0x00,0x07,0x00,
    0x00,0x00,0x5f,0x00,0x62,0x00,0x6c,0x00,
    0x61,0x00,0x6e,0x00,0x6b,0x00,0x00,0x00,
};

/* relative url */
static const unsigned char expected_hlink_data4[] =
{
    0x02,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
    0x00,0x00,0x0b,0x00,0x00,0x00,0x69,0x6e,
    0x64,0x65,0x78,0x2e,0x68,0x74,0x6d,0x6c,
    0x00,0xff,0xff,0xad,0xde,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,
};

/* url + target frame name */
static const unsigned char expected_hlink_data5[] =
{
    0x02,0x00,0x00,0x00,0x83,0x00,0x00,0x00,
    0x07,0x00,0x00,0x00,0x74,0x00,0x67,0x00,
    0x74,0x00,0x66,0x00,0x72,0x00,0x6d,0x00,
    0x00,0x00,0xe0,0xc9,0xea,0x79,0xf9,0xba,
    0xce,0x11,0x8c,0x82,0x00,0xaa,0x00,0x4b,
    0xa9,0x0b,0x26,0x00,0x00,0x00,0x68,0x00,
    0x74,0x00,0x74,0x00,0x70,0x00,0x3a,0x00,
    0x2f,0x00,0x2f,0x00,0x77,0x00,0x69,0x00,
    0x6e,0x00,0x65,0x00,0x68,0x00,0x71,0x00,
    0x2e,0x00,0x6f,0x00,0x72,0x00,0x67,0x00,
    0x2f,0x00,0x00,0x00,
};

/* filename */
static const unsigned char expected_hlink_data6[] =
{
     0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
     0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
     0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
     0x00,0x00,0x04,0x00,0x00,0x00,0x63,0x3a,
     0x5c,0x00,0xff,0xff,0xad,0xde,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
     0x00,0x00,0x0c,0x00,0x00,0x00,0x06,0x00,
     0x00,0x00,0x03,0x00,0x63,0x00,0x3a,0x00,
     0x5c,0x00,
};

static void test_persist_save_data(const char *testname, IHlink *lnk,
                                   const unsigned char *expected_data,
                                   unsigned int expected_data_size)
{
    HRESULT hr;
    IStream *stream;
    IPersistStream *ps;
    HGLOBAL hglobal;
    DWORD data_size;
    const unsigned char *data;
    DWORD i;
    BOOL same;

    hr = IHlink_QueryInterface(lnk, &IID_IPersistStream, (void **)&ps);
    ok(hr == S_OK, "IHlink_QueryInterface failed with error 0x%08x\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed with error 0x%08x\n", hr);

    hr = IPersistStream_Save(ps, stream, TRUE);
    ok(hr == S_OK, "IPersistStream_Save failed with error 0x%08x\n", hr);

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok(hr == S_OK, "GetHGlobalFromStream failed with error 0x%08x\n", hr);

    data_size = GlobalSize(hglobal);

    data = GlobalLock(hglobal);

    /* first check we have the right amount of data */
    ok(data_size == expected_data_size,
       "%s: Size of saved data differs (expected %d, actual %d)\n",
       testname, expected_data_size, data_size);

    same = TRUE;
    /* then do a byte-by-byte comparison */
    for (i = 0; i < min(data_size, expected_data_size); i++)
    {
        if ((expected_data[i] != data[i]) &&
            (((expected_data != expected_hlink_data2) &&
              (expected_data != expected_hlink_data3)) ||
             ((i < 52 || i >= 56) && (i < 80 || i >= 84))))
        {
            same = FALSE;
            break;
        }
    }

    ok(same, "%s: Saved data differs\n", testname);
    if (!same)
    {
        for (i = 0; i < data_size; i++)
        {
            if (i % 8 == 0) printf("    ");
            printf("0x%02x,", data[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n");
    }

    GlobalUnlock(hglobal);

    IStream_Release(stream);
    IPersistStream_Release(ps);
}

static void test_persist(void)
{
    static const WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',0 };
    static const WCHAR rel_url[] = { 'i','n','d','e','x','.','h','t','m','l',0 };
    static const WCHAR filename[] = { 'c',':','\\',0 };
    static const WCHAR friendly_name[] = { 'W','i','n','e',' ','H','Q',0 };
    static const WCHAR location[] = { '_','b','l','a','n','k',0 };
    static const WCHAR target_frame_name[] = { 't','g','t','f','r','m',0 };
    HRESULT hr;
    IHlink *lnk;

    hr = HlinkCreateFromString(url, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("url only", lnk, expected_hlink_data, sizeof(expected_hlink_data));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, NULL, friendly_name, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("url + friendly name", lnk, expected_hlink_data2, sizeof(expected_hlink_data2));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, location, friendly_name, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("url + friendly_name + location", lnk, expected_hlink_data3, sizeof(expected_hlink_data3));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(rel_url, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("relative url", lnk, expected_hlink_data4, sizeof(expected_hlink_data4));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(url, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    hr = IHlink_SetTargetFrameName(lnk, target_frame_name);
    ok(hr == S_OK, "IHlink_SetTargetFrameName failed with error 0x%08x\n", hr);
    test_persist_save_data("url + target frame name", lnk, expected_hlink_data5, sizeof(expected_hlink_data5));
    IHlink_Release(lnk);

    hr = HlinkCreateFromString(filename, NULL, NULL, NULL,
                               0, NULL, &IID_IHlink, (LPVOID*) &lnk);
    ok(hr == S_OK, "IHlinCreateFromString failed with error 0x%08x\n", hr);
    test_persist_save_data("filename", lnk, expected_hlink_data6, sizeof(expected_hlink_data6));
    IHlink_Release(lnk);
}

START_TEST(hlink)
{
    CoInitialize(NULL);

    test_HlinkIsShortcut();
    test_reference();
    test_persist();

    CoUninitialize();
}
