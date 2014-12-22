/*
 * Copyright 2014 Sebastian Lackner
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
#include "winerror.h"

#include "slpublic.h"
#include "slerror.h"

#include <wine/test.h>

static void test_SLGetWindowsInformationDWORD(void)
{
    static const WCHAR NonexistentLicenseValueW[] = {'N','o','n','e','x','i','s','t','e','n','t','-',
                                                     'L','i','c','e','n','s','e','-','V','a','l','u','e',0};
    static const WCHAR KernelMUILanguageAllowedW[] = {'K','e','r','n','e','l','-','M','U','I','-',
                                                      'L','a','n','g','u','a','g','e','-','A','l','l','o','w','e','d',0};
    static const WCHAR KernelMUINumberAllowedW[] = {'K','e','r','n','e','l','-','M','U','I','-',
                                                    'N','u','m','b','e','r','-','A','l','l','o','w','e','d',0};
    static const WCHAR emptyW[] = {0};
    DWORD value;
    HRESULT res;

    res = SLGetWindowsInformationDWORD(NonexistentLicenseValueW, NULL);
    ok(res == E_INVALIDARG, "expected E_INVALIDARG, got %08x\n", res);

    res = SLGetWindowsInformationDWORD(NULL, &value);
    ok(res == E_INVALIDARG, "expected E_INVALIDARG, got %08x\n", res);

    value = 0xdeadbeef;
    res = SLGetWindowsInformationDWORD(NonexistentLicenseValueW, &value);
    ok(res == SL_E_VALUE_NOT_FOUND, "expected SL_E_VALUE_NOT_FOUND, got %08x\n", res);
    ok(value == 0xdeadbeef, "expected value = 0xdeadbeef, got %u\n", value);

    value = 0xdeadbeef;
    res = SLGetWindowsInformationDWORD(emptyW, &value);
    ok(res == SL_E_RIGHT_NOT_GRANTED || broken(res == 0xd000000d) /* Win 8 */,
       "expected SL_E_RIGHT_NOT_GRANTED, got %08x\n", res);
    ok(value == 0xdeadbeef, "expected value = 0xdeadbeef, got %u\n", value);

    value = 0xdeadbeef;
    res = SLGetWindowsInformationDWORD(KernelMUILanguageAllowedW, &value);
    ok(res == SL_E_DATATYPE_MISMATCHED, "expected SL_E_DATATYPE_MISMATCHED, got %08x\n", res);
    ok(value == 0xdeadbeef, "expected value = 0xdeadbeef, got %u\n", value);

    value = 0xdeadbeef;
    res = SLGetWindowsInformationDWORD(KernelMUINumberAllowedW, &value);
    ok(res == S_OK, "expected S_OK, got %u\n", res);
    ok(value != 0xdeadbeef, "expected value != 0xdeadbeef\n");
}


START_TEST(slc)
{
    test_SLGetWindowsInformationDWORD();
}
