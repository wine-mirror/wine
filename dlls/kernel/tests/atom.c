/*
 * Unit tests for atom functions
 *
 * Copyright (c) 2002 Alexandre Julliard
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
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};
static const WCHAR FOOBARW[] = {'F','O','O','B','A','R',0};
static const WCHAR _foobarW[] = {'_','f','o','o','b','a','r',0};

static BOOL unicode_OS;

static void test_add_atom(void)
{
    ATOM atom, w_atom;
    int i;

    SetLastError( 0xdeadbeef );
    atom = GlobalAddAtomA( "foobar" );
    ok( atom >= 0xc000, "bad atom id %x", atom );
    ok( GetLastError() == 0xdeadbeef, "GlobalAddAtomA set last error" );

    /* Verify that it can be found (or not) appropriately */
    ok( GlobalFindAtomA( "foobar" ) == atom, "could not find atom foobar" );
    ok( GlobalFindAtomA( "FOOBAR" ) == atom, "could not find atom FOOBAR" );
    ok( !GlobalFindAtomA( "_foobar" ), "found _foobar" );

    /* Add the same atom, specifying string as unicode; should
     * find the first one, not add a new one */
    SetLastError( 0xdeadbeef );
    w_atom = GlobalAddAtomW( foobarW );
    if (w_atom && GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        unicode_OS = TRUE;
    else
        trace("WARNING: Unicode atom APIs are not supported on this platform\n");

    if (unicode_OS)
    {
        ok( w_atom == atom, "Unicode atom does not match ASCII" );
        ok( GetLastError() == 0xdeadbeef, "GlobalAddAtomW set last error" );
    }

    /* Verify that it can be found (or not) appropriately via unicode name */
    if (unicode_OS)
    {
        ok( GlobalFindAtomW( foobarW ) == atom, "could not find atom foobar" );
        ok( GlobalFindAtomW( FOOBARW ) == atom, "could not find atom FOOBAR" );
        ok( !GlobalFindAtomW( _foobarW ), "found _foobar" );
    }

    /* Test integer atoms
     * (0x0001 .. 0xbfff) should be valid;
     * (0xc000 .. 0xffff) should be invalid */

    SetLastError( 0xdeadbeef );
    ok( GlobalAddAtomA(0) == 0 && GetLastError() == 0xdeadbeef, "succeeded to add atom 0" );
    if (unicode_OS)
    {
        SetLastError( 0xdeadbeef );
        ok( GlobalAddAtomW(0) == 0 && GetLastError() == 0xdeadbeef, "succeeded to add atom 0" );
    }

    SetLastError( 0xdeadbeef );
    for (i = 1; i <= 0xbfff; i++)
    {
        SetLastError( 0xdeadbeef );
        ok( GlobalAddAtomA((LPCSTR)i) == i && GetLastError() == 0xdeadbeef,
            "failed to add atom %x", i );
        if (unicode_OS)
        {
            SetLastError( 0xdeadbeef );
            ok( GlobalAddAtomW((LPCWSTR)i) == i && GetLastError() == 0xdeadbeef,
                "failed to add atom %x", i );
        }
    }

    for (i = 0xc000; i <= 0xffff; i++)
    {
        ok( !GlobalAddAtomA((LPCSTR)i), "succeeded adding %x", i );
        if (unicode_OS)
            ok( !GlobalAddAtomW((LPCWSTR)i), "succeeded adding %x", i );
    }
}

static void test_get_atom_name(void)
{
    char buf[10];
    WCHAR bufW[10];
    int i;
    UINT len;
    static const WCHAR resultW[] = {'f','o','o','b','a','r',0,'.','.','.'};

    ATOM atom = GlobalAddAtomA( "foobar" );

    /* Get the name of the atom we added above */
    memset( buf, '.', sizeof(buf) );
    len = GlobalGetAtomNameA( atom, buf, 10 );
    ok( len == strlen("foobar"), "bad length %d", len );
    ok( !memcmp( buf, "foobar\0...", 10 ), "bad buffer contents" );

    /* Repeat, unicode-style */
    if (unicode_OS)
    {
        for (i = 0; i < 10; i++) bufW[i] = '.';
        SetLastError( 0xdeadbeef );
        len = GlobalGetAtomNameW( atom, bufW, 10 );
        ok( len && GetLastError() == 0xdeadbeef, "GlobalGetAtomNameW failed" );
        ok( len == lstrlenW(foobarW), "bad length %d", len );
        ok( !memcmp( bufW, resultW, 10*sizeof(WCHAR) ), "bad buffer contents" );
    }

    /* Check error code returns */
    memset(buf, '.', 10);
    ok( !GlobalGetAtomNameA( atom, buf,  0 ), "succeeded" );
    ok( !memcmp( buf, "..........", 10 ), "should not touch buffer" );

    if (unicode_OS)
    {
        static const WCHAR sampleW[10] = {'.','.','.','.','.','.','.','.','.','.'};

        for (i = 0; i < 10; i++) bufW[i] = '.';
        ok( !GlobalGetAtomNameW( atom, bufW, 0 ), "succeeded" );
        ok( !memcmp( bufW, sampleW, 10 * sizeof(WCHAR) ), "should not touch buffer" );
    }

    /* Test integer atoms */
    for (i = 0; i <= 0xbfff; i++)
    {
        memset( buf, 'a', 10 );
        len = GlobalGetAtomNameA( (ATOM)i, buf, 10 );
        if (i)
        {
            char res[20];
            ok( (len > 1) && (len < 7), "bad length %d", len );
            sprintf( res, "#%d", i );
            memset( res + strlen(res) + 1, 'a', 10 );
            ok( !memcmp( res, buf, 10 ), "bad buffer contents %s", buf );
        }
        else
            ok( !len, "bad length %d", len );
    }
}

static void test_error_handling(void)
{
    char buffer[260];
    WCHAR bufferW[260];
    int i;

    memset( buffer, 'a', 256 );
    buffer[256] = 0;
    ok( !GlobalAddAtomA(buffer), "add succeeded" );
    ok( !GlobalFindAtomA(buffer), "find succeeded" );

    if (unicode_OS)
    {
        for (i = 0; i < 256; i++) bufferW[i] = 'b';
        bufferW[256] = 0;
        ok( !GlobalAddAtomW(bufferW), "add succeeded" );
        ok( !GlobalFindAtomW(bufferW), "find succeeded" );
    }
}

START_TEST(atom)
{
    test_add_atom();
    test_get_atom_name();
    test_error_handling();
}
