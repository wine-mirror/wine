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

#include <stdio.h>

#include "wine/test.h"
#include "winbase.h"
#include "winerror.h"

static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};
static const WCHAR FOOBARW[] = {'F','O','O','B','A','R',0};
static const WCHAR _foobarW[] = {'_','f','o','o','b','a','r',0};

static void test_add_atom(void)
{
    ATOM atom, w_atom;
    int i;

    SetLastError( 0xdeadbeef );
    atom = GlobalAddAtomA( "foobar" );
    ok( (atom >= 0xc000) && (atom <= 0xffff), "bad atom id %x", atom );
    ok( GetLastError() == 0xdeadbeef, "GlobalAddAtomA set last error" );

    /* Verify that it can be found (or not) appropriately */
    ok( GlobalFindAtomA( "foobar" ) == atom, "could not find atom foobar" );
    ok( GlobalFindAtomA( "FOOBAR" ) == atom, "could not find atom FOOBAR" );
    ok( !GlobalFindAtomA( "_foobar" ), "found _foobar" );

    /* Add the same atom, specifying string as unicode; should
     * find the first one, not add a new one */
    SetLastError( 0xdeadbeef );
    w_atom = GlobalAddAtomW( foobarW );
    ok( w_atom == atom, "Unicode atom does not match ASCII" );
    ok( GetLastError() == 0xdeadbeef, "GlobalAddAtomW set last error" );

    /* Verify that it can be found (or not) appropriately via unicode name */
    ok( GlobalFindAtomW( foobarW ) == atom, "could not find atom foobar" );
    ok( GlobalFindAtomW( FOOBARW ) == atom, "could not find atom FOOBAR" );
    ok( !GlobalFindAtomW( _foobarW ), "found _foobar" );

    /* Test integer atoms
     * (0x0000 .. 0xbfff) should be valid;
     * (0xc000 .. 0xffff) should be invalid */
    todo_wine
    {
        SetLastError( 0xdeadbeef );
        ok( GlobalAddAtomA(0) == 0 && GetLastError() == 0xdeadbeef, "failed to add atom 0" );
        SetLastError( 0xdeadbeef );
        ok( GlobalAddAtomW(0) == 0 && GetLastError() == 0xdeadbeef, "failed to add atom 0" );
    }
    SetLastError( 0xdeadbeef );
    for (i = 1; i <= 0xbfff; i++)
    {
        SetLastError( 0xdeadbeef );
        ok( GlobalAddAtomA((LPCSTR)i) == i && GetLastError() == 0xdeadbeef,
            "failed to add atom %x", i );
        ok( GlobalAddAtomW((LPCWSTR)i) == i && GetLastError() == 0xdeadbeef,
            "failed to add atom %x", i );
    }
    for (i = 0xc000; i <= 0xffff; i++)
    {
        SetLastError( 0xdeadbeef );
        ok( !GlobalAddAtomA((LPCSTR)i) && (GetLastError() == ERROR_INVALID_PARAMETER),
            "succeeded adding %x", i );
        SetLastError( 0xdeadbeef );
        ok( !GlobalAddAtomW((LPCWSTR)i) && (GetLastError() == ERROR_INVALID_PARAMETER),
            "succeeded adding %x", i );
    }
}

static void test_get_atom_name(void)
{
    char buf[10];
    WCHAR bufW[10];
    int i, len;
    static const char resultW[] = {'f','o','o','b','a','r',0,'.','.','.'};

    ATOM atom = GlobalAddAtomA( "foobar" );

    /* Get the name of the atom we added above */
    memset( buf, '.', sizeof(buf) );
    len = GlobalGetAtomNameA( atom, buf, 10 );
    ok( len == strlen("foobar"), "bad length %d", len );
    ok( !memcmp( buf, "foobar\0...", 10 ), "bad buffer contents" );
    ok( !GlobalGetAtomNameA( atom, buf, 3 ), "GlobalGetAtomNameA succeeded on short buffer" );

    /* Repeat, unicode-style */
    for (i = 0; i < 10; i++) bufW[i] = '.';
    len = GlobalGetAtomNameW( atom, bufW, 10 );
    ok( len == lstrlenW(foobarW), "bad length %d", len );
    todo_wine
    {
        ok( !memcmp( bufW, resultW, 10*sizeof(WCHAR) ), "bad buffer contents" );
    }
    ok( !GlobalGetAtomNameW( atom, bufW, 3 ), "GlobalGetAtomNameW succeeded on short buffer" );

    /* Check error code returns */
    SetLastError( 0xdeadbeef );
    ok( !GlobalGetAtomNameA( atom, buf,  0 ) && GetLastError() == ERROR_MORE_DATA, "succeded" );
    SetLastError( 0xdeadbeef );
    ok( !GlobalGetAtomNameA( atom, buf, -1 ) && GetLastError() == ERROR_MORE_DATA, "succeded" );
    SetLastError( 0xdeadbeef );
    ok( !GlobalGetAtomNameW( atom, bufW,  0 ) && GetLastError() == ERROR_MORE_DATA, "succeded" );
    SetLastError( 0xdeadbeef );
    ok( !GlobalGetAtomNameW( atom, bufW, -1 ) && GetLastError() == ERROR_MORE_DATA, "succeded" );

    /* Test integer atoms */
    for (i = 0; i <= 0xbfff; i++)
    {
        memset( buf, 'a', 10 );
        len = GlobalGetAtomNameA( i, buf, 10 );
        if (i)
        {
            char res[20];
            ok( (len > 1) && (len < 7), "bad length %d", len );
            sprintf( res, "#%d", i );
            memset( res + strlen(res) + 1, 'a', 10 );
            ok( !memcmp( res, buf, 10 ), "bad buffer contents %s", buf );
        }
        else todo_wine
        {
            char res[20];
            ok( (len > 1) && (len < 7), "bad length %d", len );
            sprintf( res, "#%d", i );
            memset( res + strlen(res) + 1, 'a', 10 );
            ok( !memcmp( res, buf, 10 ), "bad buffer contents %s", buf );
        }
    }
}

static void test_delete_atom(void)
{
    ATOM atom;
    int i;

    /* First make sure it doesn't exist */
    atom = GlobalAddAtomA( "foobar" );
    while (!GlobalDeleteAtom( atom ));

    /* Now add it a number of times */
    for (i = 0; i < 10; i++) atom = GlobalAddAtomA( "foobar" );

    /* Make sure it's here */
    ok( GlobalFindAtomA( "foobar" ), "foobar not found" );
    ok( GlobalFindAtomW( foobarW ), "foobarW not found" );

    /* That many deletions should succeed */
    for (i = 0; i < 10; i++)
        ok( !GlobalDeleteAtom( atom ), "delete atom failed" );

    /* It should be gone now */
    ok( !GlobalFindAtomA( "foobar" ), "still found it" );
    ok( !GlobalFindAtomW( foobarW ), "still found it" );

    /* So this one should fail */
    SetLastError( 0xdeadbeef );
    ok( GlobalDeleteAtom( atom ) == atom && GetLastError() == ERROR_INVALID_HANDLE,
        "delete failed" );
}

static void test_error_handling(void)
{
    char buffer[260];
    WCHAR bufferW[260];
    ATOM atom;
    int i;

    memset( buffer, 'a', 255 );
    buffer[255] = 0;
    ok( atom = GlobalAddAtomA( buffer ), "add failed" );
    ok( !GlobalDeleteAtom( atom ), "delete failed" );
    buffer[255] = 'a';
    buffer[256] = 0;
    SetLastError( 0xdeadbeef );
    ok( !GlobalAddAtomA(buffer) && GetLastError() == ERROR_INVALID_PARAMETER, "add succeded" );
    SetLastError( 0xdeadbeef );
    ok( !GlobalFindAtomA(buffer) && GetLastError() == ERROR_INVALID_PARAMETER, "find succeded" );

    for (i = 0; i < 255; i++) bufferW[i] = 'b';
    bufferW[255] = 0;
    ok( atom = GlobalAddAtomW( bufferW ), "add failed" );
    ok( !GlobalDeleteAtom( atom ), "delete failed" );
    bufferW[255] = 'b';
    bufferW[256] = 0;
    SetLastError( 0xdeadbeef );
    ok( !GlobalAddAtomW(bufferW) && GetLastError() == ERROR_INVALID_PARAMETER, "add succeded" );
    SetLastError( 0xdeadbeef );
    ok( !GlobalFindAtomW(bufferW) && GetLastError() == ERROR_INVALID_PARAMETER, "find succeded" );
}

START_TEST(atom)
{
    test_add_atom();
    test_get_atom_name();
    test_delete_atom();
    test_error_handling();
}
