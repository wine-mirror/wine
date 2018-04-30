/*
 * Unit test suite for mfplat.
 *
 * Copyright 2015 Michael Müller
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"

#include "initguid.h"
#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"
#include "mfreadwrite.h"

#include "wine/test.h"

static HRESULT (WINAPI *pMFCreateSourceResolver)(IMFSourceResolver **resolver);

DEFINE_GUID(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 0xa634a91c, 0x822b, 0x41b9, 0xa4, 0x94, 0x4d, 0xe4, 0x64, 0x36, 0x12, 0xb0);

DEFINE_GUID(MFT_CATEGORY_OTHER, 0x90175d57,0xb7ea,0x4901,0xae,0xb3,0x93,0x3a,0x87,0x47,0x75,0x6f);

DEFINE_GUID(DUMMY_CLSID, 0x12345678,0x1234,0x1234,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19);
DEFINE_GUID(DUMMY_GUID1, 0x12345678,0x1234,0x1234,0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21);
DEFINE_GUID(DUMMY_GUID2, 0x12345678,0x1234,0x1234,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22);

static BOOL check_clsid(CLSID *clsids, UINT32 count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (IsEqualGUID(&clsids[i], &DUMMY_CLSID))
            return TRUE;
    }
    return FALSE;
}

static void test_register(void)
{
    static WCHAR name[] = {'W','i','n','e',' ','t','e','s','t',0};
    MFT_REGISTER_TYPE_INFO input[] =
    {
        { DUMMY_CLSID, DUMMY_GUID1 }
    };
    MFT_REGISTER_TYPE_INFO output[] =
    {
        { DUMMY_CLSID, DUMMY_GUID2 }
    };
    CLSID *clsids;
    UINT32 count;
    HRESULT ret;

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 1, input, 1, output, NULL);
    if (ret == E_ACCESSDENIED)
    {
        win_skip("Not enough permissions to register a filter\n");
        return;
    }
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

if(0)
{
    /* NULL name crashes on windows */
    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, NULL, 0, 1, input, 1, output, NULL);
    ok(ret == E_INVALIDARG, "got %x\n", ret);
}

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 0, NULL, 0, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 1, NULL, 0, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

    ret = MFTRegister(DUMMY_CLSID, MFT_CATEGORY_OTHER, name, 0, 0, NULL, 1, NULL, NULL);
    ok(ret == S_OK, "Failed to register dummy filter: %x\n", ret);

if(0)
{
    /* NULL clsids/count crashes on windows (vista) */
    count = 0;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, NULL, &count);
    ok(ret == E_POINTER, "Failed to enumerate filters: %x\n", ret);
    ok(count == 0, "Expected count > 0\n");

    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, NULL);
    ok(ret == E_POINTER, "Failed to enumerate filters: %x\n", ret);
    ok(count == 0, "Expected count > 0\n");
}

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, NULL, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, input, NULL, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, NULL, output, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, input, output, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(count > 0, "Expected count > 0\n");
    ok(clsids != NULL, "Expected clsids != NULL\n");
    ok(check_clsid(clsids, count), "Filter was not part of enumeration\n");
    CoTaskMemFree(clsids);

    /* exchange input and output */
    count = 0;
    clsids = NULL;
    ret = MFTEnum(MFT_CATEGORY_OTHER, 0, output, input, NULL, &clsids, &count);
    ok(ret == S_OK, "Failed to enumerate filters: %x\n", ret);
    ok(!count, "got %d\n", count);
    ok(clsids == NULL, "Expected clsids == NULL\n");

    ret = MFTUnregister(DUMMY_CLSID);
    ok(ret == S_OK ||
       /* w7pro64 */
       broken(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "got %x\n", ret);

    ret = MFTUnregister(DUMMY_CLSID);
    ok(ret == S_OK || broken(ret == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)), "got %x\n", ret);
}

static void test_source_resolver(void)
{
    IMFSourceResolver *resolver, *resolver2;
    HRESULT hr;

    if (!pMFCreateSourceResolver)
    {
        win_skip("MFCreateSourceResolver() not found\n");
        return;
    }

    hr = pMFCreateSourceResolver(NULL);
    ok(hr == E_POINTER, "got %#x\n", hr);

    hr = pMFCreateSourceResolver(&resolver);
    ok(hr == S_OK, "got %#x\n", hr);

    hr = pMFCreateSourceResolver(&resolver2);
    ok(hr == S_OK, "got %#x\n", hr);
    ok(resolver != resolver2, "Expected new instance\n");

    IMFSourceResolver_Release(resolver);
    IMFSourceResolver_Release(resolver2);
}

static void init_functions(void)
{
    HMODULE mod = GetModuleHandleA("mfplat.dll");

#define X(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return;
    X(MFCreateSourceResolver);
#undef X
}

static void test_MFCreateMediaType(void)
{
    HRESULT hr;
    IMFMediaType *mediatype;

    hr = MFStartup(MAKELONG( MF_API_VERSION, 0xdead ), MFSTARTUP_FULL);
    ok(hr == MF_E_BAD_STARTUP_VERSION, "got 0x%08x\n", hr);

    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    ok(hr == S_OK, "got 0x%08x\n", hr);

if(0)
{
    /* Crash on Windows Vista/7 */
    hr = MFCreateMediaType(NULL);
    ok(hr == E_INVALIDARG, "got 0x%08x\n", hr);
}

    hr = MFCreateMediaType(&mediatype);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFMediaType_SetGUID(mediatype, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);

    IMFMediaType_Release(mediatype);

    MFShutdown();
}

static void test_MFCreateAttributes(void)
{
    IMFAttributes *attributes;
    HRESULT hr;
    UINT32 count;

    hr = MFCreateAttributes( &attributes, 3 );
    ok(hr == S_OK, "got 0x%08x\n", hr);

    count = 88;
    hr = IMFAttributes_GetCount(attributes, &count);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    ok(count == 0, "got %d\n", count);

    hr = IMFAttributes_SetUINT32(attributes, &MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 0);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IMFAttributes_GetCount(attributes, &count);
    todo_wine ok(hr == S_OK, "got 0x%08x\n", hr);
    todo_wine ok(count == 1, "got %d\n", count);

    IMFAttributes_Release(attributes);
}


START_TEST(mfplat)
{
    CoInitialize(NULL);

    init_functions();

    test_register();
    test_source_resolver();
    test_MFCreateMediaType();
    test_MFCreateAttributes();

    CoUninitialize();
}
