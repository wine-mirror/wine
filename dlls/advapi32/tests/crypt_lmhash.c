/*
 * Unit tests for SystemFunctionXXX (LMHash?)
 *
 * Copyright 2004 Hans Leidekker
 * Copyright 2006 Mike McCormack
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

typedef VOID (WINAPI *fnSystemFunction006)( PCSTR passwd, PSTR lmhash );
typedef DWORD (WINAPI *fnSystemFunction008)(const LPBYTE, const LPBYTE, LPBYTE);

fnSystemFunction006 pSystemFunction006;
fnSystemFunction008 pSystemFunction008;

static void test_SystemFunction006(void)
{
    char lmhash[16 + 1];

    char passwd[] = { 's','e','c','r','e','t', 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned char expect[] = 
        { 0x85, 0xf5, 0x28, 0x9f, 0x09, 0xdc, 0xa7, 0xeb,
          0xaa, 0xd3, 0xb4, 0x35, 0xb5, 0x14, 0x04, 0xee };

    pSystemFunction006( passwd, lmhash );

    ok( !memcmp( lmhash, expect, sizeof(expect) ),
        "lmhash: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
        lmhash[0], lmhash[1], lmhash[2], lmhash[3], lmhash[4], lmhash[5],
        lmhash[6], lmhash[7], lmhash[8], lmhash[9], lmhash[10], lmhash[11],
        lmhash[12], lmhash[13], lmhash[14], lmhash[15] );
}

static void test_SystemFunction008(void)
{
    /* example data from http://davenport.sourceforge.net/ntlm.html */
    unsigned char hash[0x40] = {
        0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 0x12,
        0xc2, 0x26, 0x5b, 0x23, 0x73, 0x4e, 0x0d, 0xac };
    unsigned char challenge[0x40] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    unsigned char expected[0x18] = {
        0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97,
        0x82, 0xa6, 0x67, 0xaf, 0x6d, 0x42, 0x7c, 0x6d,
        0xe6, 0x7c, 0x20, 0xc2, 0xd3, 0xe7, 0x7c, 0x56 };
    unsigned char output[0x18];
    NTSTATUS r;

    r = pSystemFunction008(0,0,0);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    r = pSystemFunction008(challenge,0,0);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    r = pSystemFunction008(challenge, hash, 0);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    /* crashes */
    if (0)
    {
        r = pSystemFunction008(challenge, 0, output);
        ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");
    }

    r = pSystemFunction008(0, 0, output);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    memset(output, 0, sizeof output);
    r = pSystemFunction008(challenge, hash, output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");

    ok( !memcmp(output, expected, sizeof expected), "response wrong\n");
}

START_TEST(crypt_lmhash)
{
    HMODULE module;

    if (!(module = LoadLibrary("advapi32.dll"))) return;

    pSystemFunction006 = (fnSystemFunction006)GetProcAddress( module, "SystemFunction006" );
    if (pSystemFunction006) 
        test_SystemFunction006();

    pSystemFunction008 = (fnSystemFunction008)GetProcAddress( module, "SystemFunction008" );
    if (pSystemFunction008)
        test_SystemFunction008();

    FreeLibrary( module );
}
