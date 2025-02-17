/*
 * Unit tests for misc shdocvw functions
 *
 * Copyright 2008 Detlef Riekenberg
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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wininet.h"
#include "winnls.h"

#include "wine/test.h"

/* ################ */

static HMODULE hshdocvw;
static HRESULT (WINAPI *pURLSubRegQueryA)(LPCSTR, LPCSTR, DWORD, LPVOID, DWORD, DWORD);

static const char appdata[] = "AppData";
static const char common_appdata[] = "Common AppData";
static const char default_page_url[] = "Default_Page_URL";
static const char does_not_exist[] = "does_not_exist";
static const char regpath_iemain[] = "Software\\Microsoft\\Internet Explorer\\Main";
static const char regpath_shellfolders[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
static const char start_page[] = "Start Page";

static void init_functions(void)
{
    hshdocvw = LoadLibraryA("shdocvw.dll");
    pURLSubRegQueryA = (void *) GetProcAddress(hshdocvw, (LPSTR) 151);
}

/* ################ */

static void test_URLSubRegQueryA(void)
{
    CHAR buffer[INTERNET_MAX_URL_LENGTH];
    HRESULT hr;
    DWORD used;
    DWORD len;

    if (!pURLSubRegQueryA) {
        skip("URLSubRegQueryA not found\n");
        return;
    }

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    /* called by inetcpl.cpl */
    hr = pURLSubRegQueryA(regpath_iemain, default_page_url, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    ok(hr == E_FAIL || hr == S_OK, "got 0x%lx (expected E_FAIL or S_OK)\n", hr);

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    /* called by inetcpl.cpl */
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    len = lstrlenA(buffer);
    /* respect privacy: do not dump the url */
    ok(hr == S_OK, "got 0x%lx and %ld (expected S_OK)\n", hr, len);

    /* test buffer length: just large enough */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, len+1, -1);
    used = lstrlenA(buffer);
    /* respect privacy: do not dump the url */
    ok((hr == S_OK) && (used == len),
        "got 0x%lx and %ld (expected S_OK and %ld)\n", hr, used, len);

    /* no space for terminating 0: result is truncated */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, len, -1);
    used = lstrlenA(buffer);
    ok((hr == S_OK) && (used == len - 1),
        "got 0x%lx and %ld (expected S_OK and %ld)\n", hr, used, len - 1);

    /* no space for the complete result: truncate another char */
    if (len > 1) {
        memset(buffer, '#', sizeof(buffer)-1);
        buffer[sizeof(buffer)-1] = '\0';
        hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, len-1, -1);
        used = lstrlenA(buffer);
        ok((hr == S_OK) && (used == (len - 2)),
            "got 0x%lx and %ld (expected S_OK and %ld)\n", hr, used, len - 2);
    }

    /* only space for the terminating 0: function still succeeded */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, 1, -1);
    used = lstrlenA(buffer);
    ok((hr == S_OK) && !used,
        "got 0x%lx and %ld (expected S_OK and 0)\n", hr, used);

    /* size of buffer is 0, but the function still succeed.
       buffer[0] is cleared in IE 5.01 and IE 5.5 (Buffer Overflow) */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, buffer, 0, -1);
    used = lstrlenA(buffer);
    ok( (hr == S_OK) &&
        ((used == INTERNET_MAX_URL_LENGTH - 1) || broken(used == 0)) ,
        "got 0x%lx and %ld (expected S_OK and INTERNET_MAX_URL_LENGTH - 1)\n",
        hr, used);

    /* still succeed without a buffer for the result */
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, NULL, 0, -1);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* still succeed, when a length is given without a buffer */
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_SZ, NULL, INTERNET_MAX_URL_LENGTH, -1);
    ok(hr == S_OK, "got 0x%lx (expected S_OK)\n", hr);

    /* this value does not exist */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, does_not_exist, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    /* random bytes are copied to the buffer */
    ok((hr == E_FAIL), "got 0x%lx (expected E_FAIL)\n", hr);

    /* the third parameter is ignored. Is it really a type? (data is REG_SZ) */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_iemain, start_page, REG_DWORD, buffer, INTERNET_MAX_URL_LENGTH, -1);
    used = lstrlenA(buffer);
    ok((hr == S_OK) && (used == len),
        "got 0x%lx and %ld (expected S_OK and %ld)\n", hr, used, len);

    /* the function works for HKCU and HKLM */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_shellfolders, appdata, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    used = lstrlenA(buffer);
    ok(hr == S_OK, "got 0x%lx and %ld (expected S_OK)\n", hr, used);

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    hr = pURLSubRegQueryA(regpath_shellfolders, common_appdata, REG_SZ, buffer, INTERNET_MAX_URL_LENGTH, -1);
    used = lstrlenA(buffer);
    ok(hr == S_OK, "got 0x%lx and %ld (expected S_OK)\n", hr, used);

    /* todo: what does the last parameter mean? */
}

START_TEST(shdocvw)
{
    init_functions();
    test_URLSubRegQueryA();
    FreeLibrary(hshdocvw);
}
