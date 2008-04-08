/*
 * Unit tests for Media Detector
 *
 * Copyright (C) 2008 Google (Lei Zhang)
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

#include <assert.h>

#define COBJMACROS

#include "ole2.h"
#include "uuids.h"
#include "wine/test.h"
#include "qedit.h"

static WCHAR test_avi_filename[MAX_PATH];

static BOOL init_tests(void)
{
    static WCHAR temp_path[MAX_PATH];
    static WCHAR prefix[] = {'D','E','S',0};
    static WCHAR avi[] = {'a','v','i',0};
    HRSRC res;
    HGLOBAL data;
    char *mem;
    DWORD size, written;
    HANDLE fh;

    res = FindResourceW(NULL, (LPWSTR) 1, (LPWSTR) 256);
    if (!res)
        return FALSE;

    data = LoadResource(NULL, res);
    if (!data)
        return FALSE;

    mem = LockResource(data);
    if (!mem)
        return FALSE;

    size = SizeofResource(NULL, res);
    if (size == 0)
        return FALSE;

    if (!GetTempPathW(MAX_PATH, temp_path))
        return FALSE;

    /* We might end up relying on the extension here, so .TMP is no good.  */
    if (!GetTempFileNameW(temp_path, prefix, 0, test_avi_filename))
        return FALSE;

    DeleteFileW(test_avi_filename);
    lstrcpyW(test_avi_filename + lstrlenW(test_avi_filename) - 3, avi);

    fh = CreateFileW(test_avi_filename, GENERIC_WRITE, 0, NULL,
                     CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE)
        return FALSE;

    if (!WriteFile(fh, mem, size, &written, NULL) || written != size)
        return FALSE;

    CloseHandle(fh);

    return TRUE;
}

static void test_mediadet(void)
{
    HRESULT hr;
    IMediaDet *pM = NULL;
    BSTR filename = NULL;
    long nstrms = 0;
    long strm;
    AM_MEDIA_TYPE mt;

    hr = CoCreateInstance(&CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMediaDet, (LPVOID*)&pM);
    ok(hr == S_OK, "CoCreateInstance failed with %x\n", hr);
    ok(pM != NULL, "pM is NULL\n");

    filename = NULL;
    hr = IMediaDet_get_Filename(pM, &filename);
    /* Despite what MSDN claims, this returns S_OK.  */
    ok(hr == S_OK, "IMediaDet_get_Filename\n");
    ok(filename == NULL, "IMediaDet_get_Filename\n");

    filename = (BSTR) -1;
    hr = IMediaDet_get_Filename(pM, &filename);
    /* Despite what MSDN claims, this returns S_OK.  */
    ok(hr == S_OK, "IMediaDet_get_Filename\n");
    ok(filename == NULL, "IMediaDet_get_Filename\n");

    filename = SysAllocString(test_avi_filename);
    hr = IMediaDet_put_Filename(pM, filename);
    ok(hr == S_OK, "IMediaDet_put_Filename -> %x\n", hr);
    SysFreeString(filename);

    hr = IMediaDet_get_OutputStreams(pM, &nstrms);
    todo_wine ok(hr == S_OK, "IMediaDet_get_OutputStreams\n");
    todo_wine ok(nstrms == 1, "IMediaDet_get_OutputStreams\n");

    filename = NULL;
    hr = IMediaDet_get_Filename(pM, &filename);
    ok(hr == S_OK, "IMediaDet_get_Filename\n");
    ok(lstrcmpW(filename, test_avi_filename) == 0,
       "IMediaDet_get_Filename\n");
    SysFreeString(filename);

    hr = IMediaDet_get_Filename(pM, NULL);
    ok(hr == E_POINTER, "IMediaDet_get_Filename\n");

    hr = IMediaDet_put_CurrentStream(pM, 0);
    todo_wine ok(hr == S_OK, "IMediaDet_put_CurrentStream\n");

    strm = -1;
    hr = IMediaDet_get_CurrentStream(pM, &strm);
    todo_wine ok(hr == S_OK, "IMediaDet_get_CurrentStream\n");
    todo_wine ok(strm == 0, "IMediaDet_get_CurrentStream\n");

    ZeroMemory(&mt, sizeof mt);
    hr = IMediaDet_get_StreamMediaType(pM, &mt);
    todo_wine ok(hr == S_OK, "IMediaDet_get_StreamMediaType\n");
    todo_wine ok(IsEqualGUID(&mt.majortype, &MEDIATYPE_Video),
                 "IMediaDet_get_StreamMediaType\n");

    hr = IMediaDet_Release(pM);
    ok(hr == 0, "IMediaDet_Release returned: %x\n", hr);

    DeleteFileW(test_avi_filename);
}

START_TEST(mediadet)
{
    if (!init_tests())
    {
        skip("Couldn't initialize tests!\n");
        return;
    }

    CoInitialize(NULL);
    test_mediadet();
    CoUninitialize();
}
