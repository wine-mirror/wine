/*
 * Unit tests for advpack.dll install functions
 *
 * Copyright (C) 2006 James Hawkins
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

#include <stdio.h>
#include <windows.h>
#include <advpub.h>
#include <urlmon.h>
#include "wine/test.h"

/* function pointers */
static HRESULT (WINAPI *pRunSetupCommand)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, HANDLE*, DWORD, LPVOID);

static BOOL init_function_pointers()
{
    HMODULE hAdvPack = LoadLibraryA("advpack.dll");
    if (!hAdvPack)
        return FALSE;

    pRunSetupCommand = (void *)GetProcAddress(hAdvPack, "RunSetupCommand");

    if (!pRunSetupCommand)
        return FALSE;

    return TRUE;
}

static void test_RunSetupCommand()
{
    HRESULT hr;
    HANDLE hexe;

    /* try an invalid cmd name */
    hr = pRunSetupCommand(NULL, NULL, "Install", "Dir", "Title", NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try an invalid directory */
    hr = pRunSetupCommand(NULL, "winver.exe", "Install", NULL, "Title", NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "Expected E_INVALIDARG, got %ld\n", hr);

    /* try to run a non-existent exe */
    hexe = NULL;
    hr = pRunSetupCommand(NULL, "idontexist.exe", "Install", "c:\\windows\\system32", "Title", &hexe, 0, NULL);
    todo_wine
    {
        ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
           "Expected HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), got %ld\n", hr);
    }
    ok(hexe == NULL, "Expcted hexe to be NULL\n");
    ok(!TerminateProcess(hexe, 0), "Expected TerminateProcess to fail\n");

    /* try a bad directory */
    hexe = NULL;
    hr = pRunSetupCommand(NULL, "winver.exe", "Install", "windows\\system32", "Title", &hexe, 0, NULL);
    todo_wine
    {
        ok(hr == HRESULT_FROM_WIN32(ERROR_DIRECTORY),
           "Expected HRESULT_FROM_WIN32(ERROR_DIRECTORY), got %ld\n", hr);
    }
    ok(hexe == NULL, "Expcted hexe to be NULL\n");
    ok(!TerminateProcess(hexe, 0), "Expected TerminateProcess to fail\n");

    /* try to run an exe with the RSC_FLAG_INF flag */
    hexe = NULL;
    hr = pRunSetupCommand(NULL, "winver.exe", "Install", "c:\\windows\\system32", "Title", &hexe, RSC_FLAG_INF, NULL);
    todo_wine
    {
        ok(hr == SPAPI_E_WRONG_INF_STYLE, "Expected SPAPI_E_WRONG_INF_STYLE, got %ld\n", hr);
    }
    ok(hexe == NULL, "Expected hexe to be NULL\n");
    ok(!TerminateProcess(hexe, 0), "Expected TerminateProcess to fail\n");

    /* run winver.exe */
    hexe = NULL;
    hr = pRunSetupCommand(NULL, "winver.exe", "Install", "c:\\windows\\system32", "Title", &hexe, 0, NULL);
    todo_wine
    {
        ok(hr == S_ASYNCHRONOUS, "Expected S_ASYNCHRONOUS, got %ld\n", hr);
        ok(hexe != NULL, "Expected hexe to be non-NULL\n");
        ok(TerminateProcess(hexe, 0), "Expected TerminateProcess to succeed\n");
    }
}

START_TEST(install)
{
    if (!init_function_pointers())
        return;

    test_RunSetupCommand();
}
