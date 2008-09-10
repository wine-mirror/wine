/*
 * MimeInternational tests
 *
 * Copyright 2008 Huw Davies
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windows.h"
#include "ole2.h"
#include "ocidl.h"

#include "mimeole.h"

#include <stdio.h>
#include <assert.h>

#include "wine/test.h"

static void test_create(void)
{
    IMimeInternational *internat, *internat2;
    HRESULT hr;
    ULONG ref;

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08x\n", hr);
    hr = MimeOleGetInternat(&internat2);
    ok(hr == S_OK, "ret %08x\n", hr);

    /* test to show that the object is a singleton with
       a reference held by the dll. */
    ok(internat == internat2, "instances differ\n");
    ref = IMimeInternational_Release(internat2);
    ok(ref == 2, "got %d\n", ref);

    IMimeInternational_Release(internat);
}

static void test_charset(void)
{
    IMimeInternational *internat;
    HRESULT hr;
    HCHARSET hcs, hcs_windows_1252, hcs_windows_1251;

    hr = MimeOleGetInternat(&internat);
    ok(hr == S_OK, "ret %08x\n", hr);

    hr = IMimeInternational_FindCharset(internat, "non-existent", &hcs);
    ok(hr == MIME_E_NOT_FOUND, "got %08x\n", hr);

    hr = IMimeInternational_FindCharset(internat, "windows-1252", &hcs_windows_1252);
    ok(hr == S_OK, "got %08x\n", hr);
    hr = IMimeInternational_FindCharset(internat, "windows-1252", &hcs);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(hcs_windows_1252 == hcs, "got different hcharsets for the same name\n");

    hr = IMimeInternational_FindCharset(internat, "windows-1251", &hcs_windows_1251);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(hcs_windows_1252 != hcs_windows_1251, "got the same hcharset for the different names\n");

    IMimeInternational_Release(internat);
}

START_TEST(mimeintl)
{
    OleInitialize(NULL);
    test_create();
    test_charset();
    OleUninitialize();
}
