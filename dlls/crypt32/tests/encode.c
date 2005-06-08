/*
 * Unit test suite for crypt32.dll's CryptEncodeObjectEx/CryptDecodeObjectEx
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

static const struct encodedInt ints[] = {
 { 1,          { 2, 1, 1 } },
 { 127,        { 2, 1, 0x7f } },
 { 128,        { 2, 2, 0x00, 0x80 } },
 { 256,        { 2, 2, 0x01, 0x00 } },
 { -128,       { 2, 1, 0x80 } },
 { -129,       { 2, 2, 0xff, 0x7f } },
 { 0xbaddf00d, { 2, 4, 0xba, 0xdd, 0xf0, 0x0d } },
};

static void test_encodeint(void)
{
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

static void test_decodeint(void)
{
    static const char bigInt[] = { 2, 5, 0xff, 0xfe, 0xff, 0xfe, 0xff };
    static const char testStr[] = { 16, 4, 't', 'e', 's', 't' };
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    int i;
    BOOL ret;

    /* CryptDecodeObjectEx with NULL bufSize crashes..
    ret = CryptDecodeObjectEx(3, X509_INTEGER, &ints[0].encoded, 
     ints[0].encoded[1] + 2, 0, NULL, NULL, NULL);
     */
    /* check bogus encoding */
    ret = CryptDecodeObjectEx(3, X509_INTEGER, (BYTE *)&ints[0].encoded, 
     ints[0].encoded[1] + 2, 0, NULL, NULL, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    /* check with NULL integer buffer.  Windows XP returns an apparently random
     * error code (0x01c567df).
     */
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_INTEGER, NULL, 0, 0,
     NULL, NULL, &bufSize);
    ok(!ret, "Expected failure, got success\n");
    /* check with a valid, but too large, integer */
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
     X509_INTEGER, bigInt, bigInt[1] + 2, CRYPT_ENCODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_LARGE,
     "Expected CRYPT_E_ASN1_LARGE, got %ld\n", GetLastError());
    /* check with a DER-encoded string */
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
     X509_INTEGER, testStr, testStr[1] + 2, CRYPT_ENCODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_BADTAG,
     "Expected CRYPT_E_ASN1_BADTAG, got %ld\n", GetLastError());
    for (i = 0; i < sizeof(ints) / sizeof(ints[0]); i++)
    {
        /* WinXP succeeds rather than failing with ERROR_MORE_DATA */
        ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_INTEGER,
         (BYTE *)&ints[i].encoded, ints[i].encoded[1] + 2, 0, NULL, NULL,
         &bufSize);
        ok(ret || GetLastError() == ERROR_MORE_DATA,
         "Expected success or ERROR_MORE_DATA, got %ld\n", GetLastError());
        ret = CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
         X509_INTEGER, (BYTE *)&ints[i].encoded, ints[i].encoded[1] + 2,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %ld\n", GetLastError());
        ok(bufSize == sizeof(int), "Expected size %d, got %ld\n", sizeof(int),
         bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (buf)
        {
            ok(!memcmp(buf, &ints[i].val, bufSize), "Expected %d, got %d\n",
             ints[i].val, *(int *)buf);
            LocalFree(buf);
        }
    }
}

struct encodedFiletime
{
    SYSTEMTIME sysTime;
    BYTE *encodedTime;
};

static void testTimeEncoding(LPCSTR encoding,
 const struct encodedFiletime *time)
{
    FILETIME ft = { 0 };
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;

    ret = SystemTimeToFileTime(&time->sysTime, &ft);
    ok(ret, "SystemTimeToFileTime failed: %ld\n", GetLastError());
    /* No test case, but both X509_ASN_ENCODING and PKCS_7_ASN_ENCODING have
     * the same effect for time encodings.
     */
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, encoding, &ft,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    /* years other than 1950-2050 are not allowed for encodings other than
     * X509_CHOICE_OF_TIME.
     */
    if (encoding == X509_CHOICE_OF_TIME ||
     (time->sysTime.wYear >= 1950 && time->sysTime.wYear <= 2050))
    {
        ok(ret, "CryptEncodeObjectEx failed: %ld (0x%08lx)\n", GetLastError(),
         GetLastError());
        ok(buf != NULL, "Expected an allocated buffer\n");
        if (buf)
        {
            ok(buf[0] == time->encodedTime[0],
             "Expected type 0x%02x, got 0x%02x\n", time->encodedTime[0],
             buf[0]);
            ok(buf[1] == time->encodedTime[1], "Expected %d bytes, got %ld\n",
             time->encodedTime[1], bufSize);
            ok(!memcmp(time->encodedTime + 2, buf + 2, time->encodedTime[1]),
             "Got unexpected value for time encoding\n");
            LocalFree(buf);
        }
    }
    else
        ok(!ret && GetLastError() == CRYPT_E_BAD_ENCODE,
         "Expected CRYPT_E_BAD_ENCODE, got 0x%08lx\n", GetLastError());
}

static void testTimeDecoding(LPCSTR encoding,
 const struct encodedFiletime *time)
{
    FILETIME ft1 = { 0 }, ft2 = { 0 };
    DWORD size = sizeof(ft2);
    BOOL ret;

    ret = SystemTimeToFileTime(&time->sysTime, &ft1);
    ok(ret, "SystemTimeToFileTime failed: %ld\n", GetLastError());
    /* No test case, but both X509_ASN_ENCODING and PKCS_7_ASN_ENCODING have
     * the same effect for time encodings.
     */
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, encoding, time->encodedTime,
     time->encodedTime[1] + 2, 0, NULL, &ft2, &size);
    /* years other than 1950-2050 are not allowed for encodings other than
     * X509_CHOICE_OF_TIME.
     */
    if (encoding == X509_CHOICE_OF_TIME ||
     (time->sysTime.wYear >= 1950 && time->sysTime.wYear <= 2050))
    {
        ok(ret, "CryptDecodeObjectEx failed: %ld (0x%08lx)\n", GetLastError(),
         GetLastError());
        ok(!memcmp(&ft1, &ft2, sizeof(ft1)),
         "Got unexpected value for time decoding\n");
    }
    else
        ok(!ret && GetLastError() == CRYPT_E_ASN1_BADTAG,
         "Expected CRYPT_E_ASN1_BADTAG, got 0x%08lx\n", GetLastError());
}

static const struct encodedFiletime times[] = {
 { { 2005, 6, 1, 6, 16, 10, 0, 0 }, "\x17" "\x0d" "050606161000Z" },
 { { 1945, 6, 1, 6, 16, 10, 0, 0 }, "\x18" "\x0f" "19450606161000Z" },
 { { 2145, 6, 1, 6, 16, 10, 0, 0 }, "\x18" "\x0f" "21450606161000Z" },
};

static void test_encodeFiletime(void)
{
    DWORD i;

    for (i = 0; i < sizeof(times) / sizeof(times[0]); i++)
    {
        testTimeEncoding(X509_CHOICE_OF_TIME, &times[i]);
        testTimeEncoding(PKCS_UTC_TIME, &times[i]);
        testTimeEncoding(szOID_RSA_signingTime, &times[i]);
    }
}

static void test_decodeFiletime(void)
{
    static const struct encodedFiletime otherTimes[] = {
     { { 1945, 6, 1, 6, 16, 10, 0, 0 },   "\x18" "\x13" "19450606161000.000Z" },
     { { 1945, 6, 1, 6, 16, 10, 0, 999 }, "\x18" "\x13" "19450606161000.999Z" },
     { { 1945, 6, 1, 6, 17, 10, 0, 0 },   "\x18" "\x13" "19450606161000+0100" },
     { { 1945, 6, 1, 6, 15, 10, 0, 0 },   "\x18" "\x13" "19450606161000-0100" },
     { { 1945, 6, 1, 6, 14, 55, 0, 0 },   "\x18" "\x13" "19450606161000-0115" },
     { { 2145, 6, 1, 6, 16,  0, 0, 0 },   "\x18" "\x0a" "2145060616" },
     { { 2045, 6, 1, 6, 16, 10, 0, 0 },   "\x17" "\x0a" "4506061610" },
    };
    /* An oddball case that succeeds in Windows, but doesn't seem correct
     { { 2145, 6, 1, 2, 11, 31, 0, 0 },   "\x18" "\x13" "21450606161000-9999" },
     */
    static const char *bogusTimes[] = {
     /* oddly, this succeeds on Windows, with year 2765
     "\x18" "\x0f" "21r50606161000Z",
      */
     "\x17" "\x08" "45060616",
     "\x18" "\x0f" "aaaaaaaaaaaaaaZ",
     "\x18" "\x04" "2145",
     "\x18" "\x08" "21450606",
    };
    DWORD i;

    for (i = 0; i < sizeof(times) / sizeof(times[0]); i++)
    {
        testTimeDecoding(X509_CHOICE_OF_TIME, &times[i]);
        testTimeDecoding(PKCS_UTC_TIME, &times[i]);
        testTimeDecoding(szOID_RSA_signingTime, &times[i]);
    }
    for (i = 0; i < sizeof(otherTimes) / sizeof(otherTimes[0]); i++)
    {
        testTimeDecoding(X509_CHOICE_OF_TIME, &otherTimes[i]);
        testTimeDecoding(PKCS_UTC_TIME, &otherTimes[i]);
        testTimeDecoding(szOID_RSA_signingTime, &otherTimes[i]);
    }
    for (i = 0; i < sizeof(bogusTimes) / sizeof(bogusTimes[0]); i++)
    {
        FILETIME ft;
        SYSTEMTIME sysTime;
        DWORD size = sizeof(ft);
        BOOL ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_CHOICE_OF_TIME,
         bogusTimes[i], bogusTimes[i][1] + 2, 0, NULL, &ft, &size);

        if (ret)
        {
            ret = FileTimeToSystemTime(&ft, &sysTime);
            printf("%02d %02d %04d %02d:%02d.%02d\n", sysTime.wMonth,
             sysTime.wDay, sysTime.wYear, sysTime.wHour, sysTime.wMinute,
             sysTime.wSecond);
        }
        ok(!ret && GetLastError() == CRYPT_E_ASN1_CORRUPT,
         "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
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
    test_decodeint();
    test_encodeFiletime();
    test_decodeFiletime();
    test_registerOIDFunction();
}
