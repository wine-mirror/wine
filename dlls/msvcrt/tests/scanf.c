/*
 * Unit test suite for *scanf functions.
 *
 * Copyright 2002 Uwe Bonnes
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

#include <stdio.h>

#include "wine/test.h"

static void test_sscanf( void )
{
    char buffer[100];
    char format[20];
    int result, ret;

    /* check EOF */
    strcpy(buffer,"");
    ret = sscanf(buffer, "%d", &result);
    ok( ret == EOF,"sscanf returns %x instead of %x", ret, EOF );
		
    /* check %x */
    strcpy(buffer,"0x519");
    ok( sscanf(buffer, "%x", &result) == 1, "sscanf failed"  );
    ok( result == 0x519,"sscanf reads %x instead of %x", result, 0x519 );

    strcpy(buffer,"0x51a");
    ok( sscanf(buffer, "%x", &result) == 1, "sscanf failed" );
    ok( result == 0x51a ,"sscanf reads %x instead of %x", result, 0x51a );

    strcpy(buffer,"0x51g");
    ok( sscanf(buffer, "%x", &result) == 1, "sscanf failed" );
    ok( result == 0x51, "sscanf reads %x instead of %x", result, 0x51 );

    /* check % followed by any char */
    strcpy(buffer,"\"%12@");
    strcpy(format,"%\"%%%d%@");  /* work around gcc format check */
    ok( sscanf(buffer, format, &result) == 1, "sscanf failed" );
    ok( result == 12, "sscanf reads %x instead of %x", result, 12 );
}


START_TEST(scanf)
{
    test_sscanf();
}
