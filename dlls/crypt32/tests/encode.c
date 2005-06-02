/*
 * Unit test suite for crypt32.dll's CryptEncodeObjectEx
 *
 * Copyright 2005 Juan Lang
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
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

struct encodedInt
{
    int val;
    BYTE encoded[6];
};

static void test_encodeint(void)
{
    static const struct encodedInt ints[] = {
     { 1,          { 2, 1, 1 } },
     { 127,        { 2, 1, 0x7f } },
     { 128,        { 2, 2, 0x00, 0x80 } },
     { 256,        { 2, 2, 0x01, 0x00 } },
     { -128,       { 2, 1, 0x80 } },
     { -129,       { 2, 2, 0xff, 0x7f } },
     { 0xbaddf00d, { 2, 4, 0xba, 0xdd, 0xf0, 0x0d } },
    };
    DWORD bufSize = 0;
    int i;
    BOOL ret;

    /* CryptEncodeObjectEx with NULL bufSize crashes..
    ret = CryptEncodeObjectEx(3, X509_INTEGER, &ints[0].val, 0, NULL, NULL,
     NULL);
     */
    /* check bogus encoding */
    ret = CryptEncodeObjectEx(0, X509_INTEGER, &ints[0].val, 0, NULL, NULL,
     &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    /* check with NULL integer buffer.  Windows XP incorrectly returns an
     * NTSTATUS.
     */
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_INTEGER, NULL, 0, NULL,
     NULL, &bufSize);
    ok(!ret && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() ==
     STATUS_ACCESS_VIOLATION), "Unexpected error code %ld\n", GetLastError());
    for (i = 0; i < sizeof(ints) / sizeof(ints[0]); i++)
    {
        BYTE *buf = NULL;

        ret = CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
         X509_INTEGER, &ints[i].val, 0, NULL, NULL, &bufSize);
        ok(ret || GetLastError() == ERROR_MORE_DATA,
         "Expected success or ERROR_MORE_DATA, got %ld\n", GetLastError());
        ret = CryptEncodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
         X509_INTEGER, &ints[i].val, CRYPT_ENCODE_ALLOC_FLAG, NULL,
         (BYTE *)&buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %ld\n", GetLastError());
        ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
         buf[0]);
        ok(!memcmp(buf + 1, ints[i].encoded + 1, ints[i].encoded[1] + 1),
         "Encoded value of 0x%08x didn't match expected\n", ints[i].val);
        LocalFree(buf);
    }
}

static void test_registerOIDFunction(void)
{
    static const WCHAR bogusDll[] = { 'b','o','g','u','s','.','d','l','l',0 };
    BOOL ret;

    /* oddly, this succeeds under WinXP; the function name key is merely
     * omitted.  This may be a side effect of the registry code, I don't know.
     * I don't check it because I doubt anyone would depend on it.
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, NULL,
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
     */
    /* On windows XP, GetLastError is incorrectly being set with an HRESULT,
     * HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)
     */
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "foo", NULL, bogusDll,
     NULL);
    ok(!ret && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() ==
     HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)),
     "Expected ERROR_INVALID_PARAMETER: %ld\n", GetLastError());
    /* This has no effect, but "succeeds" on XP */
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "foo",
     "1.2.3.4.5.6.7.8.9.10", NULL, NULL);
    ok(ret, "Expected pseudo-success, got %ld\n", GetLastError());
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(X509_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptRegisterOIDFunction(X509_ASN_ENCODING, "bogus",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(X509_ASN_ENCODING, "bogus",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
    /* This has no effect */
    ret = CryptRegisterOIDFunction(PKCS_7_ASN_ENCODING, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    /* Check with bogus encoding type: */
    ret = CryptRegisterOIDFunction(0, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    /* This is written with value 3 verbatim.  Thus, the encoding type isn't
     * (for now) treated as a mask.
     */
    ret = CryptRegisterOIDFunction(3, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10", bogusDll, NULL);
    ok(ret, "CryptRegisterOIDFunction failed: %ld\n", GetLastError());
    ret = CryptUnregisterOIDFunction(3, "CryptDllEncodeObject",
     "1.2.3.4.5.6.7.8.9.10");
    ok(ret, "CryptUnregisterOIDFunction failed: %ld\n", GetLastError());
}

START_TEST(encode)
{
    test_encodeint();
    test_registerOIDFunction();
}
