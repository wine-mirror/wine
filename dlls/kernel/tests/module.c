/*
 * Unit tests for module/DLL/library API
 *
 * Copyright (c) 2004 Eric Pouech
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

#include "wine/test.h"
#include "wine/debug.h"
#include <windows.h>

static BOOL cmpStrAW(const char* a, const WCHAR* b, DWORD len)
{
    WCHAR       aw[1024];

    MultiByteToWideChar( CP_ACP, 0, a, len, aw, sizeof(aw) / sizeof(aw[0]) );
    return memcmp(aw, b, len * sizeof(WCHAR)) == 0;
}

static void testGetModuleFileName(const char* name)
{
    HMODULE     hMod;
    char        bufA[MAX_PATH];
    WCHAR       bufW[MAX_PATH];
    DWORD       len1A, len1W, len2A, len2W, l;

    hMod = (name) ? GetModuleHandle(name) : NULL;

    /* first test, with enough space in buffer */
    memset(bufA, '-', sizeof(bufA));
    len1A = GetModuleFileNameA(hMod, bufA, sizeof(bufA));
    ok(len1A > 0, "Getting module filename for handle %p\n", hMod);
    memset(bufW, '-', sizeof(bufW));
    len1W = GetModuleFileNameW(hMod, bufW, sizeof(bufW) / sizeof(WCHAR));
    ok(len1W > 0, "Getting module filename for handle %p\n", hMod);
    ok(len1A == len1W, "Didn't get same A/W lengths (%ld/%ld)\n", len1A, len1W);
    ok(len1A == strlen(bufA), "Unexpected length of GetModuleFilenameA (%ld/%d)\n", len1A, strlen(bufA));
    ok(len1W == lstrlenW(bufW), "Unexpected length of GetModuleFilenameW (%ld/%d)\n", len1W, lstrlenW(bufW));
    ok(cmpStrAW(bufA, bufW, l = min(len1A, len1W)), "Comparing GetModuleFilenameAW results\n");

    /* second test with a buffer too small */
    memset(bufA, '-', sizeof(bufA));
    len2A = GetModuleFileNameA(hMod, bufA, len1A / 2);
    ok(len2A > 0, "Getting module filename for handle %p\n", hMod);
    memset(bufW, '-', sizeof(bufW));
    len2W = GetModuleFileNameW(hMod, bufW, len1W / 2);
    ok(len2W > 0, "Getting module filename for handle %p\n", hMod);
    ok(len2A == len2W, "Didn't get same A/W lengths (%ld/%ld)\n", len2A, len2W);
    l = min(len2A, len2W);
    ok(cmpStrAW(bufA, bufW, l), "Comparing GetModuleFilenameAW results with buffer too small %s / %s\n", wine_dbgstr_an(bufA, l), wine_dbgstr_wn(bufW, l));
    ok(len1A / 2 == len2A, "Correct length in GetModuleFilenameA with buffer too small (%ld/%ld)\n", len1A / 2, len2A);
    ok(len1W / 2 == len2W, "Correct length in GetModuleFilenameW with buffer too small (%ld/%ld)\n", len1W / 2, len2W);
}

static void testGetModuleFileName_Wrong(void)
{
    char        bufA[MAX_PATH];
    WCHAR       bufW[MAX_PATH];

    /* test wrong handle */
    bufW[0] = '*';
    ok(GetModuleFileNameW((void*)0xffffffff, bufW, sizeof(bufW) / sizeof(WCHAR)) == 0, "Unexpected success in module handle\n");
    ok(bufW[0] == '*', "When failing, buffer shouldn't be written to\n");

    bufA[0] = '*';
    ok(GetModuleFileNameA((void*)0xffffffff, bufA, sizeof(bufA)) == 0, "Unexpected success in module handle\n");
    ok(bufA[0] == '*', "When failing, buffer shouldn't be written to\n");
}

START_TEST(module)
{
    testGetModuleFileName(NULL);
    testGetModuleFileName("kernel32.dll");
    testGetModuleFileName_Wrong();
}
