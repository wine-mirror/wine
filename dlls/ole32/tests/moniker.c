/*
 * Moniker Tests
 *
 * Copyright 2004 Robert Shearman
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _WIN32_DCOM
#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "comcat.h"

#include "wine/test.h"

#define ok_ole_success(hr, func) ok(hr == S_OK, #func " failed with error 0x%08lx\n", hr)

static void test_MkParseDisplayName()
{
    IBindCtx * pbc = NULL;
    HRESULT hr;
    IMoniker * pmk  = NULL;
    IMoniker * pmk1 = NULL;
    IMoniker * pmk2 = NULL;
    IMoniker * ppmk = NULL;
    IMoniker * spMoniker;
    ULONG eaten;
    int	       monCnt;
    IUnknown * object = NULL;

    IUnknown *lpEM1;

    IEnumMoniker *spEM1  = NULL;
    IEnumMoniker *spEM2  = NULL;
    IEnumMoniker *spEM3  = NULL;

    DWORD pdwReg1=0;
    DWORD grflags=0;
    DWORD pdwReg2=0;
    IRunningObjectTable * pprot=NULL;

    /* CLSID of My Computer */
    static const WCHAR wszDisplayName[] = {'c','l','s','i','d',':',
        '2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8','-','0','8','0','0','2','B','3','0','3','0','9','D',':',0};
    static const WCHAR wszFileName1[] = {'c',':','\\','w','i','n','d','o','w','s','\\','t','e','s','t','1','.','d','o','c',0};
    static const WCHAR wszFileName2[] = {'c',':','\\','w','i','n','d','o','w','s','\\','t','e','s','t','2','.','d','o','c',0};
    WCHAR * szDisplayn;

    hr = CreateBindCtx(0, &pbc);
    ok_ole_success(hr, CreateBindCtx);

    hr = MkParseDisplayName(pbc, wszDisplayName, &eaten, &pmk);
    todo_wine { ok_ole_success(hr, MkParseDisplayName); }

    if (object)
    {
        hr = IMoniker_BindToObject(pmk, pbc, NULL, &IID_IUnknown, (LPVOID*)&object);
        ok_ole_success(hr, IMoniker_BindToObject);

        IUnknown_Release(object);
    }
    IBindCtx_Release(pbc);

    /* Test the EnumMoniker interface */
    hr = CreateBindCtx(0, &pbc);
    ok_ole_success(hr, CreateBindCtx);

    hr = CreateFileMoniker(wszFileName1, &pmk1);
    ok(hr==0, "CreateFileMoniker for file hr=%08lx\n", hr);
    hr = CreateFileMoniker(wszFileName2, &pmk2);
    ok(hr==0, "CreateFileMoniker for file hr=%08lx\n", hr);
    hr = IBindCtx_GetRunningObjectTable(pbc, &pprot);
    ok(hr==0, "IBindCtx_GetRunningObjectTable hr=%08lx\n", hr);

    /* Check EnumMoniker before registering */
    hr = IRunningObjectTable_EnumRunning(pprot, &spEM1);
    ok(hr==0, "IRunningObjectTable_EnumRunning hr=%08lx\n", hr);
    hr = IEnumMoniker_QueryInterface(spEM1, &IID_IUnknown, (void*) &lpEM1);
    /* Register a couple of Monikers and check is ok */
    ok(hr==0, "IEnumMoniker_QueryInterface hr %08lx %p\n", hr, lpEM1);
    hr = MK_E_NOOBJECT;
    monCnt = 0;
    while ((IEnumMoniker_Next(spEM1, 1, &spMoniker, NULL)==S_OK) &&
           (ppmk ==NULL))
    {
        monCnt++;
        szDisplayn=NULL;
        hr=IMoniker_GetDisplayName(spMoniker, pbc, NULL,
                                   (LPOLESTR*) &szDisplayn);
                                          /* szDisplayn needs to be freed by
                                           * IMalloc_Free hence the use of
                                           * CoGetMalloc                      */
        if (SUCCEEDED(hr))
        {
            CoTaskMemFree(szDisplayn);
        }
    }
    ok(monCnt==0, "Number of monikers should be equal to 0 not %i\n", monCnt);      grflags= grflags | ROTFLAGS_REGISTRATIONKEEPSALIVE;
    hr = IRunningObjectTable_Register(pprot, grflags, lpEM1, pmk1, &pdwReg1);
    ok(hr==0, "IRunningObjectTable_Register hr=%08lx %p %08lx %p %p %ld\n",
        hr, pprot, grflags, lpEM1, pmk1, pdwReg1);

    grflags=0;
    grflags= grflags | ROTFLAGS_REGISTRATIONKEEPSALIVE;
    hr = IRunningObjectTable_Register(pprot, grflags, lpEM1, pmk2, &pdwReg2);
    ok(hr==0, "IRunningObjectTable_Register hr=%08lx %p %08lx %p %p %ld\n", hr,
       pprot, grflags, lpEM1, pmk2, pdwReg2);

    hr = IRunningObjectTable_EnumRunning(pprot, &spEM2);
    ok(hr==0, "IRunningObjectTable_EnumRunning hr=%08lx\n", hr);

    monCnt=0;
    while ((IEnumMoniker_Next(spEM2, 1, &spMoniker, NULL)==S_OK) &&
           (ppmk ==NULL))
    {
        WCHAR *szDisplayn=NULL;
        monCnt++;
        hr=IMoniker_GetDisplayName(spMoniker, pbc, NULL,
                                   (LPOLESTR*) &szDisplayn);
                                          /* szDisplayn needs to be freed by
                                           * IMalloc_Free hence the use of
                                           * CoGetMalloc                      */
        if (SUCCEEDED(hr))
        {
            CoTaskMemFree(szDisplayn);
        }
    }
    ok(monCnt==2, "Number of monikers should be equal to 2 not %i\n", monCnt);

    IEnumMoniker_Clone(spEM2, &spEM3);
    monCnt=0;
    while ((IEnumMoniker_Next(spEM3, 1, &spMoniker, NULL)==S_OK) &&
           (ppmk ==NULL))
    {
        WCHAR *szDisplayn=NULL;
        monCnt++;
        hr=IMoniker_GetDisplayName(spMoniker, pbc, NULL,
                                   (LPOLESTR*) &szDisplayn);
                                          /* szDisplayn needs to be freed by
                                           * IMalloc_Free hence the use of
                                           * CoGetMalloc                      */
        if (SUCCEEDED(hr))
        {
            CoTaskMemFree(szDisplayn);
        }
    }
    ok(monCnt==2, "Number of monikers should be equal to 2 not %i\n", monCnt);
    IRunningObjectTable_Revoke(pprot,pdwReg1);
    IRunningObjectTable_Revoke(pprot,pdwReg2);
    IEnumMoniker_Release(spEM1);
    IEnumMoniker_Release(spEM1);
    IEnumMoniker_Release(spEM2);
    IEnumMoniker_Release(spEM3);
    IMoniker_Release(pmk1);
    IMoniker_Release(pmk2);
    IRunningObjectTable_Release(pprot);

    IBindCtx_Release(pbc);
    /* Finished testing EnumMoniker */
}

static const BYTE expected_moniker_data[] =
{
	0x4d,0x45,0x4f,0x57,0x04,0x00,0x00,0x00,
	0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
	0x1a,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
	0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
	0x00,0x00,0x00,0x00,0x14,0x00,0x00,0x00,
	0x05,0xe0,0x02,0x00,0x00,0x00,0x00,0x00,
	0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46,
	0x00,0x00,0x00,0x00,
};

static const LARGE_INTEGER llZero;

static void test_class_moniker()
{
    IStream * stream;
    IMoniker * moniker;
    HRESULT hr;
    HGLOBAL hglobal;
    LPBYTE moniker_data;
    DWORD moniker_size;
    DWORD i;
    BOOL same = TRUE;

    hr = CreateClassMoniker(&CLSID_StdComponentCategoriesMgr, &moniker);
    todo_wine { ok_ole_success(hr, CreateClassMoniker); }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);

    hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    todo_wine { ok_ole_success(hr, CoMarshalInterface); }

    hr = GetHGlobalFromStream(stream, &hglobal);
    ok_ole_success(hr, GetHGlobalFromStream);

    moniker_size = GlobalSize(hglobal);

    moniker_data = GlobalLock(hglobal);

    /* first check we have the right amount of data */
    todo_wine {
    ok(moniker_size == sizeof(expected_moniker_data),
        "Size of marshaled data differs (expected %d, actual %ld)\n",
        sizeof(expected_moniker_data), moniker_size);
    }

    /* then do a byte-by-byte comparison */
    for (i = 0; i < min(moniker_size, sizeof(expected_moniker_data)); i++)
    {
        if (expected_moniker_data[i] != moniker_data[i])
        {
            same = FALSE;
            break;
        }
    }

    ok(same, "Marshaled data differs\n");
    if (!same)
    {
        trace("Dumping marshaled moniker data:\n");
        for (i = 0; i < moniker_size; i++)
        {
            trace("0x%02x,", moniker_data[i]);
            if (i % 8 == 7) trace("\n");
            if (i % 8 == 0) trace("     ");
        }
    }

    GlobalUnlock(hglobal);

    IStream_Seek(stream, llZero, STREAM_SEEK_SET, NULL);
    hr = CoReleaseMarshalData(stream);
    todo_wine { ok_ole_success(hr, CoReleaseMarshalData); }

    IStream_Release(stream);
    if (moniker) IMoniker_Release(moniker);
}

START_TEST(moniker)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_MkParseDisplayName();
    test_class_moniker();
    /* FIXME: test moniker creation funcs and parsing other moniker formats */

    CoUninitialize();
}
