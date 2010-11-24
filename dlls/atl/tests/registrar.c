/*
 * ATL test program
 *
 * Copyright 2010 Damjan Jovanovic
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
#include <stdio.h>

#define COBJMACROS

#include <wine/test.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wingdi.h>
#include <winnls.h>
#include <winerror.h>
#include <winnt.h>
#include <wtypes.h>
#include <olectl.h>
#include <ocidl.h>
#include <initguid.h>
#include <atliface.h>

static const char textA[] =
"HKCR \n"
"{ \n"
"    ForceRemove eebf73c4-50fd-478f-bbcf-db212221227a \n"
"    { \n"
"        val 'string' = s 'string' \n"
"        val 'dword_quoted_dec' = d '1' \n"
"        val 'dword_unquoted_dec' = d 1 \n"
"        val 'dword_quoted_hex' = d '0xA' \n"
"        val 'dword_unquoted_hex' = d 0xA \n"
"    } \n"
"}";

static void test_registrar(void)
{
    IRegistrar *registrar = NULL;
    HRESULT hr;
    INT count;
    WCHAR *textW = NULL;

    hr = CoCreateInstance(&CLSID_Registrar, NULL, CLSCTX_INPROC_SERVER, &IID_IRegistrar, (void**)&registrar);
    if (FAILED(hr))
    {
        skip("creating IRegistrar failed, hr = 0x%08X\n", hr);
        return;
    }

    count = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
    textW = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR));
    if (textW)
    {
        DWORD dword;
        DWORD size;
        LONG lret;
        HKEY key;

        MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, count);
        hr = IRegistrar_StringRegister(registrar, textW);
        ok(SUCCEEDED(hr), "IRegistar_StringRegister failed, hr = 0x%08X\n", hr);

        lret = RegOpenKeyA(HKEY_CLASSES_ROOT, "eebf73c4-50fd-478f-bbcf-db212221227a", &key);
        ok(lret == ERROR_SUCCESS, "error %d opening registry key\n", lret);

        size = sizeof(dword);
        lret = RegQueryValueExA(key, "dword_unquoted_hex", NULL, NULL, (BYTE*)&dword, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %d\n", lret);
        ok(dword != 0xA, "unquoted hex is not supposed to be preserved\n");

        size = sizeof(dword);
        lret = RegQueryValueExA(key, "dword_quoted_hex", NULL, NULL, (BYTE*)&dword, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %d\n", lret);
        ok(dword != 0xA, "quoted hex is not supposed to be preserved\n");

        size = sizeof(dword);
        lret = RegQueryValueExA(key, "dword_unquoted_dec", NULL, NULL, (BYTE*)&dword, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %d\n", lret);
        ok(dword == 1, "unquoted dec is not supposed to be %d\n", dword);

        size = sizeof(dword);
        lret = RegQueryValueExA(key, "dword_quoted_dec", NULL, NULL, (BYTE*)&dword, &size);
        ok(lret == ERROR_SUCCESS, "RegQueryValueExA failed, error %d\n", lret);
        ok(dword == 1, "quoted dec is not supposed to be %d\n", dword);

        hr = IRegistrar_StringUnregister(registrar, textW);
        ok(SUCCEEDED(hr), "IRegistar_StringUnregister failed, hr = 0x%08X\n", hr);
        RegCloseKey(key);
    }
    else
        skip("allocating memory failed\n");

    IRegistrar_Release(registrar);
}

START_TEST(registrar)
{
    CoInitialize(NULL);

    test_registrar();

    CoUninitialize();
}
