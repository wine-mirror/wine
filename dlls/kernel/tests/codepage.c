/*
 * Unit tests for code page to/from unicode translations
 *
 * Copyright (c) 2002 Dmitry Timoshkov
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
#include "winbase.h"
#include "winnls.h"

static void test_negative_source_length(void)
{
    int len;
    char buf[10];
    WCHAR bufW[10];
    static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};

    /* Test, whether any negative source length works as strlen() + 1 */
    SetLastError( 0xdeadbeef );
    len = WideCharToMultiByte(CP_ACP, 0, foobarW, -2002, buf, 10, NULL, NULL);
    ok(len == 7 && !lstrcmpA(buf, "foobar") && GetLastError() == 0xdeadbeef,
       "any negative value should work as strlen() + 1");

    SetLastError( 0xdeadbeef );
    len = MultiByteToWideChar(CP_ACP, 0, "foobar", -2002, bufW, 10);
    ok(len == 7 && !lstrcmpW(bufW, foobarW) && GetLastError() == 0xdeadbeef,
       "any negative value should work as strlen() + 1");
}

START_TEST(codepage)
{
    test_negative_source_length();
}
