/*
 * Unit test suite for Microsoft Color Matching System User Interface
 *
 * Copyright (C) 2024 Mohamad Al-Jaf
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
 *
 */

#include "wine/test.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

static void test_SetupColorMatchingW(void)
{
    COLORMATCHSETUPW cms;
    BOOL ret;

    memset( &cms, 0, sizeof( cms ) );
    SetLastError( 0xdeadbeef );
    ret = SetupColorMatchingW( &cms );
    ok( ret == FALSE, "got ret %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() returned %ld\n", GetLastError() );
}

START_TEST(icmui)
{
    test_SetupColorMatchingW();
}
