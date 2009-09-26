/*
 *    General still image implementation
 *
 * Copyright 2009 Damjan Jovanovic
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
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#define COBJMACROS
#include <initguid.h>
#include <sti.h>
#include <guiddef.h>
#include <devguid.h>
#include <stdio.h>

#include "wine/test.h"

static HMODULE sti_dll;
static HRESULT (WINAPI *pStiCreateInstance)(HINSTANCE,DWORD,PSTIW*,LPUNKNOWN);
static HRESULT (WINAPI *pStiCreateInstanceA)(HINSTANCE,DWORD,PSTIA*,LPUNKNOWN);
static HRESULT (WINAPI *pStiCreateInstanceW)(HINSTANCE,DWORD,PSTIW*,LPUNKNOWN);

static BOOL init_function_pointers(void)
{
    sti_dll = LoadLibrary("sti.dll");
    if (sti_dll)
    {
        pStiCreateInstance = (void*)
            GetProcAddress(sti_dll, "StiCreateInstance");
        pStiCreateInstanceA = (void*)
            GetProcAddress(sti_dll, "StiCreateInstanceA");
        pStiCreateInstanceW = (void*)
            GetProcAddress(sti_dll, "StiCreateInstanceW");
        return TRUE;
    }
    return FALSE;
}

void test_version_flag_versus_aw(void)
{
    HRESULT hr;

    /* Who wins, the STI_VERSION_FLAG_UNICODE or the A/W function? And what about the neutral StiCreateInstance function? */

    if (pStiCreateInstance)
    {
        PSTIW pStiW;
        hr = pStiCreateInstance(GetModuleHandle(NULL), STI_VERSION_REAL, &pStiW, NULL);
        if (SUCCEEDED(hr))
        {
            IUnknown *pUnknown;
            hr = IUnknown_QueryInterface((IUnknown*)pStiW, &IID_IStillImageW, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                ok(pUnknown == (IUnknown*)pStiW, "created interface was not IID_IStillImageW\n");
                IUnknown_Release(pUnknown);
            }
            IUnknown_Release((IUnknown*)pStiW);
        }
        else
            todo_wine ok(0, "could not create StillImageA, hr = 0x%X\n", hr);
        hr = pStiCreateInstance(GetModuleHandle(NULL), STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE, &pStiW, NULL);
        if (SUCCEEDED(hr))
        {
            IUnknown *pUnknown;
            hr = IUnknown_QueryInterface((IUnknown*)pStiW, &IID_IStillImageW, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                ok(pUnknown == (IUnknown*)pStiW, "created interface was not IID_IStillImageW\n");
                IUnknown_Release(pUnknown);
            }
            IUnknown_Release((IUnknown*)pStiW);
        }
        else
            todo_wine ok(0, "could not create StillImageW, hr = 0x%X\n", hr);
    }
    else
        skip("No StiCreateInstance function\n");

    if (pStiCreateInstanceA)
    {
        PSTIA pStiA;
        hr = pStiCreateInstanceA(GetModuleHandle(NULL), STI_VERSION_REAL | STI_VERSION_FLAG_UNICODE, &pStiA, NULL);
        if (SUCCEEDED(hr))
        {
            IUnknown *pUnknown;
            hr = IUnknown_QueryInterface((IUnknown*)pStiA, &IID_IStillImageA, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                ok(pUnknown == (IUnknown*)pStiA, "created interface was not IID_IStillImageA\n");
                IUnknown_Release(pUnknown);
            }
            IUnknown_Release((IUnknown*)pStiA);
        }
        else
            todo_wine ok(0, "could not create StillImageA, hr = 0x%X\n", hr);
    }
    else
        skip("No StiCreateInstanceA function\n");

    if (pStiCreateInstanceW)
    {
        PSTIW pStiW;
        hr = pStiCreateInstanceW(GetModuleHandle(NULL), STI_VERSION_REAL, &pStiW, NULL);
        if (SUCCEEDED(hr))
        {
            IUnknown *pUnknown;
            hr = IUnknown_QueryInterface((IUnknown*)pStiW, &IID_IStillImageW, (void**)&pUnknown);
            if (SUCCEEDED(hr))
            {
                ok(pUnknown == (IUnknown*)pStiW, "created interface was not IID_IStillImageW\n");
                IUnknown_Release((IUnknown*)pUnknown);
            }
            IUnknown_Release((IUnknown*)pStiW);
        }
        else
            todo_wine ok(0, "could not create StillImageW, hr = 0x%X\n", hr);
    }
    else
        skip("No StiCreateInstanceW function\n");
}

START_TEST(sti)
{
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        if (init_function_pointers())
        {
            test_version_flag_versus_aw();
            FreeLibrary(sti_dll);
        }
        else
            skip("could not load sti.dll\n");
        CoUninitialize();
    }
    else
        skip("CoInitialize failed\n");
}
