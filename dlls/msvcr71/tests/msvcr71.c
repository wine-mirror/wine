/*
 * Copyright 2022 Shaun Ren for CodeWeavers
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
#include <stdlib.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <errno.h>
#include "wine/test.h"

static void test_strcmp(void)
{
    int ret = strcmp( "abc", "abcd" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "", "abc" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "abc", "ab\xa0" );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strcmp( "ab\xb0", "ab\xa0" );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = strcmp( "ab\xc2", "ab\xc2" );
    ok( ret == 0, "wrong ret %d\n", ret );

    ret = strncmp( "abc", "abcd", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "", "abc", 3 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "ab\xa0", 4 );
    ok( ret == -1, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xb0", "ab\xa0", 3 );
    ok( ret == 1, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xb0", "ab\xa0", 2 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "ab\xc2", "ab\xc2", 3 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "abd", 0 );
    ok( ret == 0, "wrong ret %d\n", ret );
    ret = strncmp( "abc", "abc", 12 );
    ok( ret == 0, "wrong ret %d\n", ret );
}

START_TEST(msvcr71)
{
    test_strcmp();
}
