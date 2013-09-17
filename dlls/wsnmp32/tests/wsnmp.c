/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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
#include <windef.h>
#include <winbase.h>
#include <winsnmp.h>

#include "wine/test.h"

static void test_SnmpStartup(void)
{
    SNMPAPI_STATUS status;
    smiUINT32 major, minor, level, translate_mode, retransmit_mode;

    status = SnmpStartup( NULL, NULL, NULL, NULL, NULL );
    ok( status == SNMPAPI_SUCCESS, "got %u\n", status );

    status = SnmpCleanup();
    ok( status == SNMPAPI_SUCCESS, "got %u\n", status );

    major = minor = level = translate_mode = retransmit_mode = 0xdeadbeef;
    status = SnmpStartup( &major, &minor, &level, &translate_mode, &retransmit_mode );
    ok( status == SNMPAPI_SUCCESS, "got %u\n", status );
    trace( "major %u minor %u level %u translate_mode %u retransmit_mode %u\n",
           major, minor, level, translate_mode, retransmit_mode );

    status = SnmpCleanup();
    ok( status == SNMPAPI_SUCCESS, "got %u\n", status );
}

START_TEST (wsnmp)
{
    test_SnmpStartup();
}
