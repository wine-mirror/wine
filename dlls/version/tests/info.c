/*
 * Copyright (C) 2004 Stefan Leichter
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

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winver.h"

static void test_info_size(void)
{   DWORD hdl, retval;

    SetLastError(-1L);
    retval = GetFileVersionInfoSizeA( NULL, NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    ok( (ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) ||
	(ERROR_INVALID_PARAMETER == GetLastError()),
	"Last error wrong! ERROR_RESOURCE_DATA_NOT_FOUND/ERROR_INVALID_PARAMETER "
	"(98) expected, got 0x%08lx\n", GetLastError());

    hdl = 0x55555555;
    SetLastError(-1L);
    retval = GetFileVersionInfoSizeA( NULL, &hdl);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    ok( (ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) ||
	(ERROR_INVALID_PARAMETER == GetLastError()),
	"Last error wrong! ERROR_RESOURCE_DATA_NOT_FOUND/ERROR_INVALID_PARAMETER "
	"(98) expected, got 0x%08lx\n", GetLastError());
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08lx\n", hdl);

    SetLastError(-1L);
    retval = GetFileVersionInfoSizeA( "", NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    ok( (ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) ||
	(ERROR_BAD_PATHNAME == GetLastError()),
	"Last error wrong! ERROR_RESOURCE_DATA_NOT_FOUND/ERROR_BAD_PATHNAME "
	"(98) expected, got 0x%08lx\n", GetLastError());

    hdl = 0x55555555;
    SetLastError(-1L);
    retval = GetFileVersionInfoSizeA( "", &hdl);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    ok( (ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()) ||
	(ERROR_BAD_PATHNAME == GetLastError()),
	"Last error wrong! ERROR_RESOURCE_DATA_NOT_FOUND/ERROR_BAD_PATHNAME "
	"(98) expected, got 0x%08lx\n", GetLastError());
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08lx\n", hdl);

    SetLastError(-1L);
    retval = GetFileVersionInfoSizeA( "kernel32.dll", NULL);
    ok( retval,
	"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08lx\n",
	retval);
    ok( NO_ERROR == GetLastError(),
	"Last error wrong! NO_ERROR expected, got 0x%08lx\n",
	GetLastError());

    hdl = 0x55555555;
    SetLastError(-1L);
    retval = GetFileVersionInfoSizeA( "kernel32.dll", &hdl);
    ok( retval,
	"GetFileVersionInfoSizeA result wrong! <> 0L expected, got 0x%08lx\n",
	retval);
    ok( NO_ERROR == GetLastError(),
	"Last error wrong! NO_ERROR expected, got 0x%08lx\n",
	GetLastError());
    ok( hdl == 0L,
	"Handle wrong! 0L expected, got 0x%08lx\n", hdl);

    SetLastError(-1L);
    retval = GetFileVersionInfoSizeA( "notexist.dll", NULL);
    ok( !retval,
	"GetFileVersionInfoSizeA result wrong! 0L expected, got 0x%08lx\n",
	retval);
    ok( (ERROR_FILE_NOT_FOUND == GetLastError()) || 
	(ERROR_RESOURCE_DATA_NOT_FOUND == GetLastError()),
	"Last error wrong! ERROR_FILE_NOT_FOUND/ERROR_RESOURCE_DATA_NOT_FOUND "
	"(XP) expected, got 0x%08lx\n", GetLastError());
}

START_TEST(info)
{
    test_info_size();
}
