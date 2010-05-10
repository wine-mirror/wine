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

/** TODO: Work on this!
typedef struct _uri_component_test_str {
    const char* uri;
    DWORD       create_flags;
    HRESULT     create_hres;
    DWORD       property;
    DWORD       property_flags;
    HRESULT     property_hres;
    const char* expected;
} uri_component_test_str;

static const uri_component_test_str URI_STR_PARSE_ONLY_TESTS[] = {
    {"http://www.winehq.org", Uri_CREATE_NO_CANONICALIZE, S_OK, Uri_PROPERTY_SCHEME_NAME, 0, S_OK, "http"},
    {"file://c:/test/test.mp3", Uri_CREATE_NO_CANONICALIZE, S_OK, Uri_PROPERTY_SCHEME_NAME, 0, S_OK, "file"}
};
*/


typedef struct _uri_create_flag_test {
    DWORD   flags;
    HRESULT expected;
} uri_create_flag_test;

static const uri_create_flag_test URI_INVALID_CREATE_FLAGS[] = {
    /* Set of invalid flag combinations to test for. */
    {Uri_CREATE_DECODE_EXTRA_INFO | Uri_CREATE_NO_DECODE_EXTRA_INFO, E_INVALIDARG},
    {Uri_CREATE_CANONICALIZE | Uri_CREATE_NO_CANONICALIZE, E_INVALIDARG},
    {Uri_CREATE_CRACK_UNKNOWN_SCHEMES | Uri_CREATE_NO_CRACK_UNKNOWN_SCHEMES, E_INVALIDARG},
    {Uri_CREATE_PRE_PROCESS_HTML_URI | Uri_CREATE_NO_PRE_PROCESS_HTML_URI, E_INVALIDARG},
    {Uri_CREATE_IE_SETTINGS | Uri_CREATE_NO_IE_SETTINGS, E_INVALIDARG}
};

static void test_create_flags(LPCWSTR uriW, uri_create_flag_test test) {
    HRESULT hr;
    IUri *uri = NULL;

    hr = CreateUri(uriW, test.flags, 0, &uri);
    ok(hr == test.expected, "Error: CreateUri returned 0x%08x, expected 0x%08x", hr, test.expected);

    if(uri) { IUri_Release(uri); }
}

/*
 * Simple tests to make sure the CreateUri function handles invalid flag combinations
 * correctly.
 */
static void test_CreateUri_InvalidFlags(void) {
    static const WCHAR INVALID_URL[] = {'I','N','V','A','L','I','D',0};
    DWORD i;

    for(i = 0; i < sizeof(URI_INVALID_CREATE_FLAGS)/sizeof(URI_INVALID_CREATE_FLAGS[0]); ++i) {
        test_create_flags(INVALID_URL, URI_INVALID_CREATE_FLAGS[i]);
    }
}

START_TEST(uri) {
    todo_wine {
        trace("test CreateUri invalid flags...\n");
        test_CreateUri_InvalidFlags();
    }
}
