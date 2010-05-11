/*
 * UrlMon IUri tests
 *
 * Copyright 2010 Thomas Mullaly
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

/*
 * IUri testing framework goals:
 *  - Test invalid flags
 *  - Test parsing for components when no canonicalization occurs
 *  - Test parsing for components when canonicalization occurs.
 *  - More tests...
 */

#include <wine/test.h>
#include <stdarg.h>
#include <stddef.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "urlmon.h"

static HRESULT (WINAPI *pCreateUri)(LPCWSTR, DWORD, DWORD_PTR, IUri**);

static const WCHAR http_urlW[] = { 'h','t','t','p',':','/','/','w','w','w','.','w','i','n','e','h','q',
        '.','o','r','g','/',0};

typedef struct _uri_create_flag_test {
    DWORD   flags;
    HRESULT expected;
} uri_create_flag_test;

static const uri_create_flag_test invalid_flag_tests[] = {
    /* Set of invalid flag combinations to test for. */
    {Uri_CREATE_DECODE_EXTRA_INFO | Uri_CREATE_NO_DECODE_EXTRA_INFO, E_INVALIDARG},
    {Uri_CREATE_CANONICALIZE | Uri_CREATE_NO_CANONICALIZE, E_INVALIDARG},
    {Uri_CREATE_CRACK_UNKNOWN_SCHEMES | Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES, E_INVALIDARG},
    {Uri_CREATE_PRE_PROCESS_HTML_URI | Uri_CREATE_NO_PRE_PROCESS_HTML_URI, E_INVALIDARG},
    {Uri_CREATE_IE_SETTINGS | Uri_CREATE_NO_IE_SETTINGS, E_INVALIDARG}
};

/*
 * Simple tests to make sure the CreateUri function handles invalid flag combinations
 * correctly.
 */
static void test_CreateUri_InvalidFlags(void) {
    DWORD i;

    for(i = 0; i < sizeof(invalid_flag_tests)/sizeof(invalid_flag_tests[0]); ++i) {
        HRESULT hr;
        IUri *uri = (void*) 0xdeadbeef;

        hr = pCreateUri(http_urlW, invalid_flag_tests[i].flags, 0, &uri);
        todo_wine {
            ok(hr == invalid_flag_tests[i].expected, "Error: CreateUri returned 0x%08x, expected 0x%08x, flags=0x%08x\n",
                    hr, invalid_flag_tests[i].expected, invalid_flag_tests[i].flags);
        }
        todo_wine { ok(uri == NULL, "Error: expected the IUri to be NULL, but it was %p instead\n", uri); }
    }
}

START_TEST(uri) {
    HMODULE hurlmon;

    hurlmon = GetModuleHandle("urlmon.dll");
    pCreateUri = (void*) GetProcAddress(hurlmon, "CreateUri");

    if(!pCreateUri) {
        win_skip("CreateUri is not present, skipping tests.\n");
        return;
    }

    trace("test CreateUri invalid flags...\n");
    test_CreateUri_InvalidFlags();
}
