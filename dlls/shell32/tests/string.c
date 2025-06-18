/*
 * Unit tests for shell32 string operations
 *
 * Copyright 2004 Jon Griffiths
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

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "shellapi.h"
#include "shtypes.h"
#include "objbase.h"

#include "wine/test.h"

static HMODULE hShell32;
static BOOL (WINAPI *pStrRetToStrNAW)(LPVOID,DWORD,LPSTRRET,const ITEMIDLIST *);

static WCHAR *CoDupStrW(const char* src)
{
  INT len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
  WCHAR* szTemp = CoTaskMemAlloc(len * sizeof(WCHAR));
  MultiByteToWideChar(CP_ACP, 0, src, -1, szTemp, len);
  return szTemp;
}

static void test_StrRetToStringNA(void)
{
    trace("StrRetToStringNAW is Ascii\n");
    /* FIXME */
}

static void test_StrRetToStringNW(void)
{
    ITEMIDLIST iidl[10];
    WCHAR buff[128];
    STRRET strret;
    BOOL ret;

    trace("StrRetToStringNAW is Unicode\n");

    strret.uType = STRRET_WSTR;
    strret.pOleStr = CoDupStrW("Test");
    memset(buff, 0xff, sizeof(buff));
    ret = pStrRetToStrNAW(buff, ARRAY_SIZE(buff) - 1, &strret, NULL);
    ok(ret == TRUE && !wcscmp(buff, L"Test"),
       "STRRET_WSTR: dup failed, ret=%d\n", ret);

    strret.uType = STRRET_CSTR;
    lstrcpyA(strret.cStr, "Test");
    memset(buff, 0xff, sizeof(buff));
    ret = pStrRetToStrNAW(buff, ARRAY_SIZE(buff), &strret, NULL);
    ok(ret == TRUE && !wcscmp(buff, L"Test"),
       "STRRET_CSTR: dup failed, ret=%d\n", ret);

    strret.uType = STRRET_OFFSET;
    strret.uOffset = 1;
    strcpy((char*)&iidl, " Test");
    memset(buff, 0xff, sizeof(buff));
    ret = pStrRetToStrNAW(buff, ARRAY_SIZE(buff), &strret, iidl);
    ok(ret == TRUE && !wcscmp(buff, L"Test"),
       "STRRET_OFFSET: dup failed, ret=%d\n", ret);

    strret.uType = 3;
    memset(buff, 0xff, sizeof(buff));
    ret = pStrRetToStrNAW(buff, 0, &strret, NULL);
    ok(ret == TRUE && buff[0] == 0xffff,
       "Invalid STRRET type: dup failed, ret=%d\n", ret);

    strret.uType = 3;
    memset(buff, 0xff, sizeof(buff));
    ret = pStrRetToStrNAW(buff, ARRAY_SIZE(buff), &strret, NULL);
    ok(ret == TRUE && buff[0] == 0,
       "Invalid STRRET type: dup failed, ret=%d\n", ret);

    /* The next test crashes on W2K, WinXP and W2K3, so we don't test. */
if (0)
{
    /* Invalid dest - should return FALSE, except NT4 does not, so we don't check. */
    strret.uType = STRRET_WSTR;
    strret.pOleStr = CoDupStrW("Test");
    pStrRetToStrNAW(NULL, ARRAY_SIZE(buff), &strret, NULL);
    trace("NULL dest: ret=%d\n", ret);
}
}

START_TEST(string)
{
    CoInitialize(0);

    hShell32 = GetModuleHandleA("shell32.dll");

    pStrRetToStrNAW = (void*)GetProcAddress(hShell32, (LPSTR)96);
    if (pStrRetToStrNAW)
    {
        if (!(GetVersion() & 0x80000000))
            test_StrRetToStringNW();
        else
            test_StrRetToStringNA();
    }

    CoUninitialize();
}
