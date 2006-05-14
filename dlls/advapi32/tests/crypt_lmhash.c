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

struct ustring {
    DWORD Length;
    DWORD MaximumLength;
    unsigned char *Buffer;
};

typedef NTSTATUS (WINAPI *fnSystemFunction001)(const LPBYTE, const LPBYTE, LPBYTE);
typedef NTSTATUS (WINAPI *fnSystemFunction002)(const LPBYTE, const LPBYTE, LPBYTE);
typedef NTSTATUS (WINAPI *fnSystemFunction003)(const LPBYTE, LPBYTE);
typedef NTSTATUS (WINAPI *fnSystemFunction004)(const struct ustring *, const struct ustring *, struct ustring *);
typedef VOID (WINAPI *fnSystemFunction006)( PCSTR passwd, PSTR lmhash );
typedef NTSTATUS (WINAPI *fnSystemFunction008)(const LPBYTE, const LPBYTE, LPBYTE);
typedef NTSTATUS (WINAPI *fnSystemFunction032)(struct ustring *, struct ustring *);

fnSystemFunction001 pSystemFunction001;
fnSystemFunction002 pSystemFunction002;
fnSystemFunction003 pSystemFunction003;
fnSystemFunction004 pSystemFunction004;
fnSystemFunction006 pSystemFunction006;
fnSystemFunction008 pSystemFunction008;
fnSystemFunction032 pSystemFunction032;

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

static void test_SystemFunction001(void)
{
    unsigned char key[8] = { 0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 0 };
    unsigned char data[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    unsigned char expected[8] = { 0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97 };
    unsigned char output[16];
    NTSTATUS r;

    r = pSystemFunction001(0,0,0);
    ok( r == STATUS_UNSUCCESSFUL, "wrong error code\n");

    memset(output, 0, sizeof output);

    r = pSystemFunction001(data,key,output);
    ok( r == STATUS_SUCCESS, "wrong error code\n");

    ok(!memcmp(output, expected, sizeof expected), "response wrong\n");
}

static void test_SystemFunction002(void)
{
    /* reverse of SystemFunction001 */
    unsigned char key[8] = { 0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24, 0 };
    unsigned char expected[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef };
    unsigned char data[8] = { 0xc3, 0x37, 0xcd, 0x5c, 0xbd, 0x44, 0xfc, 0x97 };
    unsigned char output[8];
    int r;

    memset(output, 0, sizeof output);
    r = pSystemFunction002(data, key, output);
    ok(r == STATUS_SUCCESS, "function failed\n");
    ok(!memcmp(output, expected, sizeof expected), "response wrong\n");
}

static void test_SystemFunction032(void)
{
    struct ustring key, data;
    unsigned char szKey[] = { 'f','o','o',0 };
    unsigned char szData[8] = { 'b','a','r',0 };
    unsigned char expected[] = {0x28, 0xb9, 0xf8, 0xe1};
    int r;

    /* crashes:    pSystemFunction032(NULL,NULL); */

    key.Buffer = szKey;
    key.Length = sizeof szKey;
    key.MaximumLength = key.Length;

    data.Buffer = szData;
    data.Length = 4;
    data.MaximumLength = 8;

    r = pSystemFunction032(&data, &key);
    ok(r == STATUS_SUCCESS, "function failed\n");

    ok(!memcmp(expected, data.Buffer, data.Length), "wrong result\n");
}

static void test_SystemFunction003(void)
{
    unsigned char output[8], data[8];
    unsigned char key[7] = { 0xff, 0x37, 0x50, 0xbc, 0xc2, 0xb2, 0x24 };
    unsigned char exp1[8] = { 0x9d, 0x21, 0xc8, 0x86, 0x6c, 0x21, 0xcf, 0x43 };
    char exp2[] = "KGS!@#$%";
    int r;

    r = pSystemFunction003(NULL, NULL);
    ok(r == STATUS_UNSUCCESSFUL, "function failed\n");

    r = pSystemFunction003(key, NULL);
    ok(r == STATUS_UNSUCCESSFUL, "function failed\n");

    memset(data, 0, sizeof data);
    r = pSystemFunction003(key, data);
    ok(r == STATUS_SUCCESS, "function failed\n");
    ok( !memcmp(exp1, data, sizeof data), "decrypted message wrong\n");

    memset(output, 0, sizeof output);
    r = pSystemFunction002(data, key, output);

    ok( !memcmp(exp2, output, sizeof output), "decrypted message wrong\n");
}

static void test_SystemFunction004(void)
{
    unsigned char inbuf[0x100], keybuf[0x100], resbuf[0x100];
    unsigned char output[8];
    int r;
    struct ustring in, key, out;

    /* crash 
    r = pSystemFunction004(NULL, NULL, NULL);
    ok(r == STATUS_UNSUCCESSFUL, "function failed\n");
    */

    memset(inbuf, 0, sizeof inbuf);
    memset(keybuf, 0, sizeof keybuf);
    memset(resbuf, 0, sizeof resbuf);

    in.Buffer = NULL;
    in.Length = in.MaximumLength = 0;

    key.Buffer = NULL;
    key.Length = key.MaximumLength = 0;

    out.Buffer = NULL;
    out.Length = out.MaximumLength = 0;

    r = pSystemFunction004(&in, &key, &out);
    ok(r == STATUS_INVALID_PARAMETER_2, "function failed\n");

    key.Buffer = keybuf;
    key.Length = 0x100;
    key.MaximumLength = 0x100;

    r = pSystemFunction004(&in, &key, (struct ustring *)&out);
    ok(r == STATUS_BUFFER_TOO_SMALL, "function failed\n");

    in.Buffer = inbuf;
    in.Length = 0x0c;
    in.MaximumLength = 0;

    /* add two identical blocks... */
    inbuf[0] = 1;
    inbuf[1] = 2;
    inbuf[2] = 3;
    inbuf[3] = 4;

    inbuf[8] = 1;
    inbuf[9] = 2;
    inbuf[10] = 3;
    inbuf[11] = 4;

    /* check that the Length field is really obeyed */
    keybuf[6] = 1;

    key.Buffer = keybuf;
    key.Length = 6;
    key.MaximumLength = 0;

    keybuf[1] = 0x33;

    out.Buffer = resbuf;
    out.Length = 0;
    out.MaximumLength = 0x40;
    r = pSystemFunction004(&in, &key, &out);
    ok(r == STATUS_SUCCESS, "function failed\n");

    keybuf[6] = 0;

    memset(output, 0, sizeof output);
    r = pSystemFunction002(out.Buffer, key.Buffer, output);

    ok(((unsigned int*)output)[0] == in.Length, "crypted length wrong\n");
    ok(((unsigned int*)output)[1] == 1, "crypted value wrong\n");

    memset(output, 0, sizeof output);
    r = pSystemFunction002(out.Buffer+8, key.Buffer, output);
    ok(!memcmp(output, inbuf, sizeof output), "crypted data wrong\n");

    memset(output, 0, sizeof output);
    r = pSystemFunction002(out.Buffer+16, key.Buffer, output);
    ok(!memcmp(output, inbuf, sizeof output), "crypted data wrong\n");
}

START_TEST(crypt_lmhash)
{
    HMODULE module;

    if (!(module = LoadLibrary("advapi32.dll"))) return;

    pSystemFunction001 = (fnSystemFunction001)GetProcAddress( module, "SystemFunction001" );
    if (pSystemFunction001)
        test_SystemFunction001();

    pSystemFunction002 = (fnSystemFunction002)GetProcAddress( module, "SystemFunction002" );
    if (pSystemFunction002)
        test_SystemFunction002();

    pSystemFunction003 = (fnSystemFunction003)GetProcAddress( module, "SystemFunction003" );
    if (pSystemFunction003)
        test_SystemFunction003();

    pSystemFunction004 = (fnSystemFunction004)GetProcAddress( module, "SystemFunction004" );
    if (pSystemFunction004)
        test_SystemFunction004();

    pSystemFunction006 = (fnSystemFunction006)GetProcAddress( module, "SystemFunction006" );
    if (pSystemFunction006) 
        test_SystemFunction006();

    pSystemFunction008 = (fnSystemFunction008)GetProcAddress( module, "SystemFunction008" );
    if (pSystemFunction008)
        test_SystemFunction008();

    pSystemFunction032 = (fnSystemFunction032)GetProcAddress( module, "SystemFunction032" );
    if (pSystemFunction032)
        test_SystemFunction032();

    FreeLibrary( module );
}
