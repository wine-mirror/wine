/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include "wine/test.h"

#include "winbase.h"
#include "ntuser.h"


static void test_NtUserCloseWindowStation(void)
{
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = NtUserCloseWindowStation( 0 );
    ok( !ret && GetLastError() == ERROR_INVALID_HANDLE,
        "NtUserCloseWindowStation returned %x %u\n", ret, GetLastError() );
}


START_TEST(win32u)
{
    /* native win32u.dll fails if user32 is not loaded, so make sure it's fully initialized */
    GetDesktopWindow();

    test_NtUserCloseWindowStation();
}
