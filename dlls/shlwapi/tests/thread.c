/* Tests for Thread and SHGlobalCounter functions
 *
 * Copyright 2010 Detlef Riekenberg
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

#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "ole2.h"
#include "shlwapi.h"

#include "wine/test.h"

static HRESULT (WINAPI *pSHGetThreadRef)(IUnknown**);

/* ##### */

static void test_SHGetThreadRef(void)
{
    IUnknown *punk;
    HRESULT hr;

    /* Not present before IE 5 */
    if (!pSHGetThreadRef) {
        win_skip("SHGetThreadRef not found\n");
        return;
    }

    punk = NULL;
    hr = pSHGetThreadRef(&punk);
    ok( (hr == E_NOINTERFACE) && (punk == NULL),
        "got 0x%x and %p (expected E_NOINTERFACE and NULL)\n", hr, punk);

    if (0) {
        /* this crash on Windows */
        hr = pSHGetThreadRef(NULL);
    }
}

START_TEST(thread)
{
    HMODULE hshlwapi = GetModuleHandleA("shlwapi.dll");

    pSHGetThreadRef = (void *) GetProcAddress(hshlwapi, "SHGetThreadRef");

    test_SHGetThreadRef();

}
