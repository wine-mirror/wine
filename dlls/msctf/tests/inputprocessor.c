/*
 * Unit tests for ITfInputProcessor
 *
 * Copyright 2009 Aric Stewart, CodeWeavers
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

#include <stdio.h>

#define COBJMACROS
#include "wine/test.h"
#include "winuser.h"
#include "shlwapi.h"
#include "shlguid.h"
#include "comcat.h"
#include "initguid.h"
#include "msctf.h"

static ITfInputProcessorProfiles* g_ipp;
static LANGID gLangid;

DEFINE_GUID(CLSID_FakeService, 0xEDE1A7AD,0x66DE,0x47E0,0xB6,0x20,0x3E,0x92,0xF8,0x24,0x6B,0xF3);
DEFINE_GUID(CLSID_TF_InputProcessorProfiles, 0x33c53a50,0xf456,0x4884,0xb0,0x49,0x85,0xfd,0x64,0x3e,0xcf,0xed);

static HRESULT initialize(void)
{
    HRESULT hr;
    CoInitialize(NULL);
    hr = CoCreateInstance (&CLSID_TF_InputProcessorProfiles, NULL,
          CLSCTX_INPROC_SERVER, &IID_ITfInputProcessorProfiles, (void**)&g_ipp);
    return hr;
}

static void cleanup(void)
{
    if (g_ipp)
        ITfInputProcessorProfiles_Release(g_ipp);
    CoUninitialize();
}

static void test_Register(void)
{
    HRESULT hr;

    static const WCHAR szDesc[] = {'F','a','k','e',' ','W','i','n','e',' ','S','e','r','v','i','c','e',0};
    static const WCHAR szFile[] = {'F','a','k','e',' ','W','i','n','e',' ','S','e','r','v','i','c','e',' ','F','i','l','e',0};

    hr = ITfInputProcessorProfiles_Register(g_ipp, &CLSID_FakeService);
    ok(SUCCEEDED(hr),"Unable to register text service(%x)\n",hr);
    hr = ITfInputProcessorProfiles_AddLanguageProfile(g_ipp, &CLSID_FakeService, gLangid, &CLSID_FakeService, szDesc, sizeof(szDesc)/sizeof(WCHAR), szFile, sizeof(szFile)/sizeof(WCHAR), 1);
    ok(SUCCEEDED(hr),"Unable to add Language Profile (%x)\n",hr);
}

static void test_Unregister(void)
{
    HRESULT hr;
    hr = ITfInputProcessorProfiles_Unregister(g_ipp, &CLSID_FakeService);
    todo_wine ok(SUCCEEDED(hr),"Unable to unregister text service(%x)\n",hr);
}

START_TEST(inputprocessor)
{
    if (SUCCEEDED(initialize()))
    {
        gLangid = GetUserDefaultLCID();
        test_Register();
        test_Unregister();
    }
    else
        skip("Unable to create InputProcessor\n");
    cleanup();
}
