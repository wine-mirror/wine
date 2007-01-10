/*
 * Copyright 2007 Bill Medland
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

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "odbcinst.h"

static void test_SQLInstallerError(void)
{
    RETCODE sql_ret;

    /* MSDN states that the error number should be between 1 and 8.  Passing 0 is an error */
    sql_ret = SQLInstallerError(0, NULL, NULL, 0, NULL);
    ok(sql_ret == SQL_ERROR, "SQLInstallerError(0...) failed with %d instead of SQL_ERROR\n", sql_ret);
    /* However numbers greater than 8 do not return SQL_ERROR.
     * I am currenly unsure as to whether it should return SQL_NO_DATA or "the same as for error 8";
     * I have never been able to generate 8 errors to test it
     */
    sql_ret = SQLInstallerError(65535, NULL, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA, "SQLInstallerError(>8...) failed with %d instead of SQL_NO_DATA\n", sql_ret);

    /* Force an error to work with.  This should generate ODBC_ERROR_INVALID_BUFF_LEN */
    ok(!SQLGetInstalledDrivers(0, 0, 0), "Failed to force an error for testing\n");
    sql_ret = SQLInstallerError(2, NULL, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA, "Too many errors when forcing an error for testing\n");

    /* Null pointers are acceptable in all obvious places */
    sql_ret = SQLInstallerError(1, NULL, NULL, 0, NULL);
    ok(sql_ret == SQL_SUCCESS_WITH_INFO, "SQLInstallerError(null addresses) failed with %d instead of SQL_SUCCESS_WITH_INFO\n", sql_ret);
}

START_TEST(misc)
{
    test_SQLInstallerError();
}
