/*
 * Copyright 2021 Hans Leidekker for CodeWeavers
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
#include <windef.h>
#include <winbase.h>
#include <winldap.h>
#include <winber.h>

#include "wine/test.h"

static void test_ber_printf(void)
{
    int ret;
    BerElement *ber;
    struct berval *bv;

    ber = ber_alloc_t( 0 );
    ok( ber == NULL, "ber_alloc_t succeeded\n" );

    ber = ber_alloc_t( LBER_USE_DER );
    ok( ber != NULL, "ber_alloc_t failed\n" );

    ret = ber_printf( ber, (char *)"{i}", -1 );
    todo_wine ok( !ret, "got %d\n", ret );

    ret = ber_flatten( ber, &bv );
    ok( !ret, "got %d\n", ret );
    todo_wine ok( bv->bv_len == 12, "got %ld\n", bv->bv_len );
    if (bv->bv_len == 12) ok( !memcmp( bv->bv_val, "0\x84\x00\x00\x00\x06\x02\x04\xff\xff\xff\xff", 12 ), "got %s\n",
                              wine_dbgstr_an(bv->bv_val, 12) );
    ber_bvfree( bv );
    ber_free( ber, 1 );
}

static void test_ber_scanf(void)
{
    int ret;
    BerElement *ber;
    struct berval bv = { 12, (char *)"0\x84\x00\x00\x00\x06\x02\x04\xff\xff\xff\xff" };
    int i;

    ber = ber_init( &bv );
    ok( ber != NULL, "ber_init failed\n" );

    i = 0;
    ret = ber_scanf( ber, (char *)"{i}", &i );
    ok( !ret, "got %d\n", ret );
    ok( i == -1, "got %d\n", i );
    ber_free( ber, 1 );
}

START_TEST(ber)
{
    test_ber_printf();
    test_ber_scanf();
}
