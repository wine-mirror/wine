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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
    const BYTE *encoded;
};

static const BYTE bin1[] = {0x02,0x01,0x01};
static const BYTE bin2[] = {0x02,0x01,0x7f};
static const BYTE bin3[] = {0x02,0x02,0x00,0x80};
static const BYTE bin4[] = {0x02,0x02,0x01,0x00};
static const BYTE bin5[] = {0x02,0x01,0x80};
static const BYTE bin6[] = {0x02,0x02,0xff,0x7f};
static const BYTE bin7[] = {0x02,0x04,0xba,0xdd,0xf0,0x0d};

static const struct encodedInt ints[] = {
 { 1,          bin1 },
 { 127,        bin2 },
 { 128,        bin3 },
 { 256,        bin4 },
 { -128,       bin5 },
 { -129,       bin6 },
 { 0xbaddf00d, bin7 },
};

struct encodedBigInt
{
    const BYTE *val;
    const BYTE *encoded;
    const BYTE *decoded;
};

static const BYTE bin8[] = {0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};
static const BYTE bin9[] = {0x02,0x0a,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0xff,0};
static const BYTE bin10[] = {0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};

static const BYTE bin11[] = {0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0xff,0xff,0};
static const BYTE bin12[] = {0x02,0x09,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};
static const BYTE bin13[] = {0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0};

static const struct encodedBigInt bigInts[] = {
 { bin8, bin9, bin10 },
 { bin11, bin12, bin13 },
};

static const BYTE bin14[] = {0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};
static const BYTE bin15[] = {0x02,0x0a,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0xff,0};
static const BYTE bin16[] = {0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0xff,0xff,0xff,0};
static const BYTE bin17[] = {0x02,0x0c,0x00,0xff,0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0};

/* Decoded is the same as original, so don't bother storing a separate copy */
static const struct encodedBigInt bigUInts[] = {
 { bin14, bin15, NULL },
 { bin16, bin17, NULL },
};

static void test_encodeInt(DWORD dwEncoding)
{
    DWORD bufSize = 0;
    int i;
    BOOL ret;
    CRYPT_INTEGER_BLOB blob;
    BYTE *buf = NULL;

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
    ret = CryptEncodeObjectEx(dwEncoding, X509_INTEGER, NULL, 0, NULL, NULL,
     &bufSize);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    for (i = 0; i < sizeof(ints) / sizeof(ints[0]); i++)
    {
        /* encode as normal integer */
        ret = CryptEncodeObjectEx(dwEncoding, X509_INTEGER, &ints[i].val, 0,
         NULL, NULL, &bufSize);
        ok(ret, "Expected success, got %ld\n", GetLastError());
        ret = CryptEncodeObjectEx(dwEncoding, X509_INTEGER, &ints[i].val,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %ld\n", GetLastError());
        if (buf)
        {
            ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
             buf[0]);
            ok(buf[1] == ints[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], ints[i].encoded[1]);
            ok(!memcmp(buf + 1, ints[i].encoded + 1, ints[i].encoded[1] + 1),
             "Encoded value of 0x%08x didn't match expected\n", ints[i].val);
            LocalFree(buf);
        }
        /* encode as multibyte integer */
        blob.cbData = sizeof(ints[i].val);
        blob.pbData = (BYTE *)&ints[i].val;
        ret = CryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, &blob,
         0, NULL, NULL, &bufSize);
        ok(ret, "Expected success, got %ld\n", GetLastError());
        ret = CryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %ld\n", GetLastError());
        if (buf)
        {
            ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
             buf[0]);
            ok(buf[1] == ints[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], ints[i].encoded[1]);
            ok(!memcmp(buf + 1, ints[i].encoded + 1, ints[i].encoded[1] + 1),
             "Encoded value of 0x%08x didn't match expected\n", ints[i].val);
            LocalFree(buf);
        }
    }
    /* encode a couple bigger ints, just to show it's little-endian and leading
     * sign bytes are dropped
     */
    for (i = 0; i < sizeof(bigInts) / sizeof(bigInts[0]); i++)
    {
        blob.cbData = strlen((const char*)bigInts[i].val);
        blob.pbData = (BYTE *)bigInts[i].val;
        ret = CryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, &blob,
         0, NULL, NULL, &bufSize);
        ok(ret, "Expected success, got %ld\n", GetLastError());
        ret = CryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %ld\n", GetLastError());
        if (buf)
        {
            ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
             buf[0]);
            ok(buf[1] == bigInts[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], bigInts[i].encoded[1]);
            ok(!memcmp(buf + 1, bigInts[i].encoded + 1,
             bigInts[i].encoded[1] + 1),
             "Encoded value didn't match expected\n");
            LocalFree(buf);
        }
    }
    /* and, encode some uints */
    for (i = 0; i < sizeof(bigUInts) / sizeof(bigUInts[0]); i++)
    {
        blob.cbData = strlen((const char*)bigUInts[i].val);
        blob.pbData = (BYTE*)bigUInts[i].val;
        ret = CryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_UINT, &blob,
         0, NULL, NULL, &bufSize);
        ok(ret, "Expected success, got %ld\n", GetLastError());
        ret = CryptEncodeObjectEx(dwEncoding, X509_MULTI_BYTE_UINT, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %ld\n", GetLastError());
        if (buf)
        {
            ok(buf[0] == 2, "Got unexpected type %d for integer (expected 2)\n",
             buf[0]);
            ok(buf[1] == bigUInts[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], bigUInts[i].encoded[1]);
            ok(!memcmp(buf + 1, bigUInts[i].encoded + 1,
             bigUInts[i].encoded[1] + 1),
             "Encoded value didn't match expected\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeInt(DWORD dwEncoding)
{
    static const BYTE bigInt[] = { 2, 5, 0xff, 0xfe, 0xff, 0xfe, 0xff };
    static const BYTE testStr[] = { 0x16, 4, 't', 'e', 's', 't' };
    static const BYTE longForm[] = { 2, 0x81, 0x01, 0x01 };
    static const BYTE bigBogus[] = { 0x02, 0x84, 0x01, 0xff, 0xff, 0xf9 };
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
    /* check with NULL integer buffer */
    ret = CryptDecodeObjectEx(dwEncoding, X509_INTEGER, NULL, 0, 0, NULL, NULL,
     &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());
    /* check with a valid, but too large, integer */
    ret = CryptDecodeObjectEx(dwEncoding, X509_INTEGER, bigInt, bigInt[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_LARGE,
     "Expected CRYPT_E_ASN1_LARGE, got %ld\n", GetLastError());
    /* check with a DER-encoded string */
    ret = CryptDecodeObjectEx(dwEncoding, X509_INTEGER, testStr, testStr[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_BADTAG,
     "Expected CRYPT_E_ASN1_BADTAG, got %ld\n", GetLastError());
    for (i = 0; i < sizeof(ints) / sizeof(ints[0]); i++)
    {
        /* When the output buffer is NULL, this always succeeds */
        SetLastError(0xdeadbeef);
        ret = CryptDecodeObjectEx(dwEncoding, X509_INTEGER,
         (BYTE *)ints[i].encoded, ints[i].encoded[1] + 2, 0, NULL, NULL,
         &bufSize);
        ok(ret && GetLastError() == NOERROR,
         "Expected success and NOERROR, got %ld\n", GetLastError());
        ret = CryptDecodeObjectEx(dwEncoding, X509_INTEGER,
         (BYTE *)ints[i].encoded, ints[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %ld\n", GetLastError());
        ok(bufSize == sizeof(int), "Wrong size %ld\n", bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (buf)
        {
            ok(!memcmp(buf, &ints[i].val, bufSize), "Expected %d, got %d\n",
             ints[i].val, *(int *)buf);
            LocalFree(buf);
        }
    }
    for (i = 0; i < sizeof(bigInts) / sizeof(bigInts[0]); i++)
    {
        ret = CryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER,
         (BYTE *)bigInts[i].encoded, bigInts[i].encoded[1] + 2, 0, NULL, NULL,
         &bufSize);
        ok(ret && GetLastError() == NOERROR,
         "Expected success and NOERROR, got %ld\n", GetLastError());
        ret = CryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER,
         (BYTE *)bigInts[i].encoded, bigInts[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %ld\n", GetLastError());
        ok(bufSize >= sizeof(CRYPT_INTEGER_BLOB), "Wrong size %ld\n", bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (buf)
        {
            CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)buf;

            ok(blob->cbData == strlen((const char*)bigInts[i].decoded),
             "Expected len %d, got %ld\n", lstrlenA((const char*)bigInts[i].decoded),
             blob->cbData);
            ok(!memcmp(blob->pbData, bigInts[i].decoded, blob->cbData),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
    for (i = 0; i < sizeof(bigUInts) / sizeof(bigUInts[0]); i++)
    {
        ret = CryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_UINT,
         (BYTE *)bigUInts[i].encoded, bigUInts[i].encoded[1] + 2, 0, NULL, NULL,
         &bufSize);
        ok(ret && GetLastError() == NOERROR,
         "Expected success and NOERROR, got %ld\n", GetLastError());
        ret = CryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_UINT,
         (BYTE *)bigUInts[i].encoded, bigUInts[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %ld\n", GetLastError());
        ok(bufSize >= sizeof(CRYPT_INTEGER_BLOB), "Wrong size %ld\n", bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (buf)
        {
            CRYPT_INTEGER_BLOB *blob = (CRYPT_INTEGER_BLOB *)buf;

            ok(blob->cbData == strlen((const char*)bigUInts[i].val),
             "Expected len %d, got %ld\n", lstrlenA((const char*)bigUInts[i].val),
             blob->cbData);
            ok(!memcmp(blob->pbData, bigUInts[i].val, blob->cbData),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
    /* Decode the value 1 with long-form length */
    ret = CryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, longForm,
     sizeof(longForm), CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(*(int *)buf == 1, "Expected 1, got %d\n", *(int *)buf);
        LocalFree(buf);
    }
    /* Try to decode some bogus large items */
    /* The buffer size is smaller than the encoded length, so this should fail
     * with CRYPT_E_ASN1_EOD if it's being decoded.
     * Under XP it fails with CRYPT_E_ASN1_LARGE, which means there's a limit
     * on the size decoded, but in ME it fails with CRYPT_E_ASN1_EOD or crashes.
     * So this test unfortunately isn't useful.
    ret = CryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, tooBig,
     0x7fffffff, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_LARGE,
     "Expected CRYPT_E_ASN1_LARGE, got %08lx\n", GetLastError());
     */
    /* This will try to decode the buffer and overflow it, check that it's
     * caught.
     */
    ret = CryptDecodeObjectEx(dwEncoding, X509_MULTI_BYTE_INTEGER, bigBogus,
     0x01ffffff, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
}

static const BYTE bin18[] = {0x0a,0x01,0x01};
static const BYTE bin19[] = {0x0a,0x05,0x00,0xff,0xff,0xff,0x80};

/* These are always encoded unsigned, and aren't constrained to be any
 * particular value
 */
static const struct encodedInt enums[] = {
 { 1,    bin18 },
 { -128, bin19 },
};

/* X509_CRL_REASON_CODE is also an enumerated type, but it's #defined to
 * X509_ENUMERATED.
 */
static const LPCSTR enumeratedTypes[] = { X509_ENUMERATED,
 szOID_CRL_REASON_CODE };

static void test_encodeEnumerated(DWORD dwEncoding)
{
    DWORD i, j;

    for (i = 0; i < sizeof(enumeratedTypes) / sizeof(enumeratedTypes[0]); i++)
    {
        for (j = 0; j < sizeof(enums) / sizeof(enums[0]); j++)
        {
            BOOL ret;
            BYTE *buf = NULL;
            DWORD bufSize = 0;

            ret = CryptEncodeObjectEx(dwEncoding, enumeratedTypes[i],
             &enums[j].val, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
             &bufSize);
            ok(ret, "CryptEncodeObjectEx failed: %ld\n", GetLastError());
            if (buf)
            {
                ok(buf[0] == 0xa,
                 "Got unexpected type %d for enumerated (expected 0xa)\n",
                 buf[0]);
                ok(buf[1] == enums[j].encoded[1],
                 "Got length %d, expected %d\n", buf[1], enums[j].encoded[1]);
                ok(!memcmp(buf + 1, enums[j].encoded + 1,
                 enums[j].encoded[1] + 1),
                 "Encoded value of 0x%08x didn't match expected\n",
                 enums[j].val);
                LocalFree(buf);
            }
        }
    }
}

static void test_decodeEnumerated(DWORD dwEncoding)
{
    DWORD i, j;

    for (i = 0; i < sizeof(enumeratedTypes) / sizeof(enumeratedTypes[0]); i++)
    {
        for (j = 0; j < sizeof(enums) / sizeof(enums[0]); j++)
        {
            BOOL ret;
            DWORD bufSize = sizeof(int);
            int val;

            ret = CryptDecodeObjectEx(dwEncoding, enumeratedTypes[i],
             enums[j].encoded, enums[j].encoded[1] + 2, 0, NULL,
             (BYTE *)&val, &bufSize);
            ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
            ok(bufSize == sizeof(int),
             "Got unexpected size %ld for enumerated\n", bufSize);
            ok(val == enums[j].val, "Unexpected value %d, expected %d\n",
             val, enums[j].val);
        }
    }
}

struct encodedFiletime
{
    SYSTEMTIME sysTime;
    const BYTE *encodedTime;
};

static void testTimeEncoding(DWORD dwEncoding, LPCSTR structType,
 const struct encodedFiletime *time)
{
    FILETIME ft = { 0 };
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;

    ret = SystemTimeToFileTime(&time->sysTime, &ft);
    ok(ret, "SystemTimeToFileTime failed: %ld\n", GetLastError());
    ret = CryptEncodeObjectEx(dwEncoding, structType, &ft,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    /* years other than 1950-2050 are not allowed for encodings other than
     * X509_CHOICE_OF_TIME.
     */
    if (structType == X509_CHOICE_OF_TIME ||
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

static void testTimeDecoding(DWORD dwEncoding, LPCSTR structType,
 const struct encodedFiletime *time)
{
    FILETIME ft1 = { 0 }, ft2 = { 0 };
    DWORD size = sizeof(ft2);
    BOOL ret;

    ret = SystemTimeToFileTime(&time->sysTime, &ft1);
    ok(ret, "SystemTimeToFileTime failed: %ld\n", GetLastError());
    ret = CryptDecodeObjectEx(dwEncoding, structType, time->encodedTime,
     time->encodedTime[1] + 2, 0, NULL, &ft2, &size);
    /* years other than 1950-2050 are not allowed for encodings other than
     * X509_CHOICE_OF_TIME.
     */
    if (structType == X509_CHOICE_OF_TIME ||
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

static const BYTE bin20[] = {
    0x17,0x0d,'0','5','0','6','0','6','1','6','1','0','0','0','Z'};
static const BYTE bin21[] = {
    0x18,0x0f,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','Z'};
static const BYTE bin22[] = {
    0x18,0x0f,'2','1','4','5','0','6','0','6','1','6','1','0','0','0','Z'};

static const struct encodedFiletime times[] = {
 { { 2005, 6, 1, 6, 16, 10, 0, 0 }, bin20 },
 { { 1945, 6, 1, 6, 16, 10, 0, 0 }, bin21 },
 { { 2145, 6, 1, 6, 16, 10, 0, 0 }, bin22 },
};

static void test_encodeFiletime(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(times) / sizeof(times[0]); i++)
    {
        testTimeEncoding(dwEncoding, X509_CHOICE_OF_TIME, &times[i]);
        testTimeEncoding(dwEncoding, PKCS_UTC_TIME, &times[i]);
        testTimeEncoding(dwEncoding, szOID_RSA_signingTime, &times[i]);
    }
}

static const BYTE bin23[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','.','0','0','0','Z'};
static const BYTE bin24[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','.','9','9','9','Z'};
static const BYTE bin25[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','+','0','1','0','0'};
static const BYTE bin26[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','-','0','1','0','0'};
static const BYTE bin27[] = {
    0x18,0x13,'1','9','4','5','0','6','0','6','1','6','1','0','0','0','-','0','1','1','5'};
static const BYTE bin28[] = {
    0x18,0x0a,'2','1','4','5','0','6','0','6','1','6'};
static const BYTE bin29[] = {
    0x17,0x0a,'4','5','0','6','0','6','1','6','1','0'};
static const BYTE bin30[] = {
    0x17,0x0b,'4','5','0','6','0','6','1','6','1','0','Z'};
static const BYTE bin31[] = {
    0x17,0x0d,'4','5','0','6','0','6','1','6','1','0','+','0','1'};
static const BYTE bin32[] = {
    0x17,0x0d,'4','5','0','6','0','6','1','6','1','0','-','0','1'};
static const BYTE bin33[] = {
    0x17,0x0f,'4','5','0','6','0','6','1','6','1','0','+','0','1','0','0'};
static const BYTE bin34[] = {
    0x17,0x0f,'4','5','0','6','0','6','1','6','1','0','-','0','1','0','0'};
static const BYTE bin35[] = {
    0x17,0x08, '4','5','0','6','0','6','1','6'};
static const BYTE bin36[] = {
    0x18,0x0f, 'a','a','a','a','a','a','a','a','a','a','a','a','a','a','Z'};
static const BYTE bin37[] = {
    0x18,0x04, '2','1','4','5'};
static const BYTE bin38[] = {
    0x18,0x08, '2','1','4','5','0','6','0','6'};

static void test_decodeFiletime(DWORD dwEncoding)
{
    static const struct encodedFiletime otherTimes[] = {
     { { 1945, 6, 1, 6, 16, 10, 0, 0 },   bin23 },
     { { 1945, 6, 1, 6, 16, 10, 0, 999 }, bin24 },
     { { 1945, 6, 1, 6, 17, 10, 0, 0 },   bin25 },
     { { 1945, 6, 1, 6, 15, 10, 0, 0 },   bin26 },
     { { 1945, 6, 1, 6, 14, 55, 0, 0 },   bin27 },
     { { 2145, 6, 1, 6, 16,  0, 0, 0 },   bin28 },
     { { 2045, 6, 1, 6, 16, 10, 0, 0 },   bin29 },
     { { 2045, 6, 1, 6, 16, 10, 0, 0 },   bin30 },
     { { 2045, 6, 1, 6, 17, 10, 0, 0 },   bin31 },
     { { 2045, 6, 1, 6, 15, 10, 0, 0 },   bin32 },
     { { 2045, 6, 1, 6, 17, 10, 0, 0 },   bin33 },
     { { 2045, 6, 1, 6, 15, 10, 0, 0 },   bin34 },
    };
    /* An oddball case that succeeds in Windows, but doesn't seem correct
     { { 2145, 6, 1, 2, 11, 31, 0, 0 },   "\x18" "\x13" "21450606161000-9999" },
     */
    static const unsigned char *bogusTimes[] = {
     /* oddly, this succeeds on Windows, with year 2765
     "\x18" "\x0f" "21r50606161000Z",
      */
     bin35,
     bin36,
     bin37,
     bin38,
    };
    DWORD i, size;
    FILETIME ft1 = { 0 }, ft2 = { 0 };
    BOOL ret;

    /* Check bogus length with non-NULL buffer */
    ret = SystemTimeToFileTime(&times[0].sysTime, &ft1);
    ok(ret, "SystemTimeToFileTime failed: %ld\n", GetLastError());
    size = 1;
    ret = CryptDecodeObjectEx(dwEncoding, X509_CHOICE_OF_TIME,
     times[0].encodedTime, times[0].encodedTime[1] + 2, 0, NULL, &ft2, &size);
    ok(!ret && GetLastError() == ERROR_MORE_DATA,
     "Expected ERROR_MORE_DATA, got %ld\n", GetLastError());
    /* Normal tests */
    for (i = 0; i < sizeof(times) / sizeof(times[0]); i++)
    {
        testTimeDecoding(dwEncoding, X509_CHOICE_OF_TIME, &times[i]);
        testTimeDecoding(dwEncoding, PKCS_UTC_TIME, &times[i]);
        testTimeDecoding(dwEncoding, szOID_RSA_signingTime, &times[i]);
    }
    for (i = 0; i < sizeof(otherTimes) / sizeof(otherTimes[0]); i++)
    {
        testTimeDecoding(dwEncoding, X509_CHOICE_OF_TIME, &otherTimes[i]);
        testTimeDecoding(dwEncoding, PKCS_UTC_TIME, &otherTimes[i]);
        testTimeDecoding(dwEncoding, szOID_RSA_signingTime, &otherTimes[i]);
    }
    for (i = 0; i < sizeof(bogusTimes) / sizeof(bogusTimes[0]); i++)
    {
        size = sizeof(ft1);
        ret = CryptDecodeObjectEx(dwEncoding, X509_CHOICE_OF_TIME,
         bogusTimes[i], bogusTimes[i][1] + 2, 0, NULL, &ft1, &size);
        ok(!ret && GetLastError() == CRYPT_E_ASN1_CORRUPT,
         "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
    }
}

static const char commonName[] = "Juan Lang";
static const char surName[] = "Lang";

static const BYTE emptySequence[] = { 0x30, 0 };
static const BYTE emptyRDNs[] = { 0x30, 0x02, 0x31, 0 };
static const BYTE twoRDNs[] = {
    0x30,0x23,0x31,0x21,0x30,0x0c,0x06,0x03,0x55,0x04,0x04,
    0x13,0x05,0x4c,0x61,0x6e,0x67,0x00,0x30,0x11,0x06,0x03,0x55,0x04,0x03,
    0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0};
static const BYTE encodedTwoRDNs[] = {
0x30,0x2e,0x31,0x2c,0x30,0x2a,0x06,0x03,0x55,0x04,0x03,0x30,0x23,0x31,0x21,
0x30,0x0c,0x06,0x03,0x55,0x04,0x04,0x13,0x05,0x4c,0x61,0x6e,0x67,0x00,0x30,
0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,
0x6e,0x67,0x00,
};

static const BYTE us[] = { 0x55, 0x53 };
static const BYTE minnesota[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x73, 0x6f,
 0x74, 0x61 };
static const BYTE minneapolis[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x61, 0x70,
 0x6f, 0x6c, 0x69, 0x73 };
static const BYTE codeweavers[] = { 0x43, 0x6f, 0x64, 0x65, 0x57, 0x65, 0x61,
 0x76, 0x65, 0x72, 0x73 };
static const BYTE wine[] = { 0x57, 0x69, 0x6e, 0x65, 0x20, 0x44, 0x65, 0x76,
 0x65, 0x6c, 0x6f, 0x70, 0x6d, 0x65, 0x6e, 0x74 };
static const BYTE localhostAttr[] = { 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f,
 0x73, 0x74 };
static const BYTE aric[] = { 0x61, 0x72, 0x69, 0x63, 0x40, 0x63, 0x6f, 0x64,
 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63, 0x6f, 0x6d };

#define _blob_of(arr) { sizeof(arr), (LPBYTE)arr }
const CERT_RDN_ATTR rdnAttrs[] = {
 { "2.5.4.6", CERT_RDN_PRINTABLE_STRING,        _blob_of(us) },
 { "2.5.4.8", CERT_RDN_PRINTABLE_STRING,        _blob_of(minnesota) },
 { "2.5.4.7", CERT_RDN_PRINTABLE_STRING,        _blob_of(minneapolis) },
 { "2.5.4.10", CERT_RDN_PRINTABLE_STRING,       _blob_of(codeweavers) },
 { "2.5.4.11", CERT_RDN_PRINTABLE_STRING,       _blob_of(wine) },
 { "2.5.4.3", CERT_RDN_PRINTABLE_STRING,        _blob_of(localhostAttr) },
 { "1.2.840.113549.1.9.1", CERT_RDN_IA5_STRING, _blob_of(aric) },
};
const CERT_RDN_ATTR decodedRdnAttrs[] = {
 { "2.5.4.6", CERT_RDN_PRINTABLE_STRING,        _blob_of(us) },
 { "2.5.4.3", CERT_RDN_PRINTABLE_STRING,        _blob_of(localhostAttr) },
 { "2.5.4.8", CERT_RDN_PRINTABLE_STRING,        _blob_of(minnesota) },
 { "2.5.4.7", CERT_RDN_PRINTABLE_STRING,        _blob_of(minneapolis) },
 { "2.5.4.10", CERT_RDN_PRINTABLE_STRING,       _blob_of(codeweavers) },
 { "2.5.4.11", CERT_RDN_PRINTABLE_STRING,       _blob_of(wine) },
 { "1.2.840.113549.1.9.1", CERT_RDN_IA5_STRING, _blob_of(aric) },
};
#undef _blob_of

static const BYTE encodedRDNAttrs[] = {
0x30,0x81,0x96,0x31,0x81,0x93,0x30,0x09,0x06,0x03,0x55,0x04,0x06,0x13,0x02,0x55,
0x53,0x30,0x10,0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x6c,0x6f,0x63,0x61,0x6c,0x68,
0x6f,0x73,0x74,0x30,0x10,0x06,0x03,0x55,0x04,0x08,0x13,0x09,0x4d,0x69,0x6e,0x6e,
0x65,0x73,0x6f,0x74,0x61,0x30,0x12,0x06,0x03,0x55,0x04,0x07,0x13,0x0b,0x4d,0x69,
0x6e,0x6e,0x65,0x61,0x70,0x6f,0x6c,0x69,0x73,0x30,0x12,0x06,0x03,0x55,0x04,0x0a,
0x13,0x0b,0x43,0x6f,0x64,0x65,0x57,0x65,0x61,0x76,0x65,0x72,0x73,0x30,0x17,0x06,
0x03,0x55,0x04,0x0b,0x13,0x10,0x57,0x69,0x6e,0x65,0x20,0x44,0x65,0x76,0x65,0x6c,
0x6f,0x70,0x6d,0x65,0x6e,0x74,0x30,0x21,0x06,0x09,0x2a,0x86,0x48,0x86,0xf7,0x0d,
0x01,0x09,0x01,0x16,0x14,0x61,0x72,0x69,0x63,0x40,0x63,0x6f,0x64,0x65,0x77,0x65,
0x61,0x76,0x65,0x72,0x73,0x2e,0x63,0x6f,0x6d
};

static void test_encodeName(DWORD dwEncoding)
{
    CERT_RDN_ATTR attrs[2];
    CERT_RDN rdn;
    CERT_NAME_INFO info;
    static CHAR oid_common_name[] = szOID_COMMON_NAME,
                oid_sur_name[]    = szOID_SUR_NAME;
    BYTE *buf = NULL;
    DWORD size = 0;
    BOOL ret;

    /* Test with NULL pvStructInfo */
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, NULL,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    /* Test with empty CERT_NAME_INFO */
    info.cRDN = 0;
    info.rgRDN = NULL;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(!memcmp(buf, emptySequence, sizeof(emptySequence)),
         "Got unexpected encoding for empty name\n");
        LocalFree(buf);
    }
    /* Test with bogus CERT_RDN */
    info.cRDN = 1;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    /* Test with empty CERT_RDN */
    rdn.cRDNAttr = 0;
    rdn.rgRDNAttr = NULL;
    info.cRDN = 1;
    info.rgRDN = &rdn;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(!memcmp(buf, emptyRDNs, sizeof(emptyRDNs)),
         "Got unexpected encoding for empty RDN array\n");
        LocalFree(buf);
    }
    /* Test with bogus attr array */
    rdn.cRDNAttr = 1;
    rdn.rgRDNAttr = NULL;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    /* oddly, a bogus OID is accepted by Windows XP; not testing.
    attrs[0].pszObjId = "bogus";
    attrs[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[0].Value.cbData = sizeof(commonName);
    attrs[0].Value.pbData = (BYTE *)commonName;
    rdn.cRDNAttr = 1;
    rdn.rgRDNAttr = attrs;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret, "Expected failure, got success\n");
     */
    /* Check with two CERT_RDN_ATTRs.  Note DER encoding forces the order of
     * the encoded attributes to be swapped.
     */
    attrs[0].pszObjId = oid_common_name;
    attrs[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[0].Value.cbData = sizeof(commonName);
    attrs[0].Value.pbData = (BYTE *)commonName;
    attrs[1].pszObjId = oid_sur_name;
    attrs[1].dwValueType = CERT_RDN_PRINTABLE_STRING;
    attrs[1].Value.cbData = sizeof(surName);
    attrs[1].Value.pbData = (BYTE *)surName;
    rdn.cRDNAttr = 2;
    rdn.rgRDNAttr = attrs;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(!memcmp(buf, twoRDNs, sizeof(twoRDNs)),
         "Got unexpected encoding for two RDN array\n");
        LocalFree(buf);
    }
    /* A name can be "encoded" with previously encoded RDN attrs. */
    attrs[0].dwValueType = CERT_RDN_ENCODED_BLOB;
    attrs[0].Value.pbData = (LPBYTE)twoRDNs;
    attrs[0].Value.cbData = sizeof(twoRDNs);
    rdn.cRDNAttr = 1;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(encodedTwoRDNs), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, encodedTwoRDNs, size),
         "Unexpected value for re-endoded two RDN array\n");
        LocalFree(buf);
    }
    /* CERT_RDN_ANY_TYPE is too vague for X509_NAMEs, check the return */
    rdn.cRDNAttr = 1;
    attrs[0].dwValueType = CERT_RDN_ANY_TYPE;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Test a more complex name */
    rdn.cRDNAttr = sizeof(rdnAttrs) / sizeof(rdnAttrs[0]);
    rdn.rgRDNAttr = (PCERT_RDN_ATTR)rdnAttrs;
    info.cRDN = 1;
    info.rgRDN = &rdn;
    buf = NULL;
    size = 0;
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(encodedRDNAttrs), "Wrong size %ld\n", size);
        ok(!memcmp(buf, encodedRDNAttrs, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void compareNameValues(const CERT_NAME_VALUE *expected,
 const CERT_NAME_VALUE *got)
{
    ok(got->dwValueType == expected->dwValueType,
     "Expected string type %ld, got %ld\n", expected->dwValueType,
     got->dwValueType);
    ok(got->Value.cbData == expected->Value.cbData,
     "String type %ld: unexpected data size, got %ld, expected %ld\n",
     expected->dwValueType, got->Value.cbData, expected->Value.cbData);
    if (got->Value.cbData && got->Value.pbData)
        ok(!memcmp(got->Value.pbData, expected->Value.pbData,
         min(got->Value.cbData, expected->Value.cbData)),
         "String type %ld: unexpected value\n", expected->dwValueType);
}

static void compareRDNAttrs(const CERT_RDN_ATTR *expected,
 const CERT_RDN_ATTR *got)
{
    if (expected->pszObjId && strlen(expected->pszObjId))
    {
        ok(got->pszObjId != NULL, "Expected OID %s, got NULL\n",
         expected->pszObjId);
        if (got->pszObjId)
        {
            ok(!strcmp(got->pszObjId, expected->pszObjId),
             "Got unexpected OID %s, expected %s\n", got->pszObjId,
             expected->pszObjId);
        }
    }
    compareNameValues((const CERT_NAME_VALUE *)&expected->dwValueType,
     (const CERT_NAME_VALUE *)&got->dwValueType);
}

static void compareRDNs(const CERT_RDN *expected, const CERT_RDN *got)
{
    ok(got->cRDNAttr == expected->cRDNAttr,
     "Expected %ld RDN attrs, got %ld\n", expected->cRDNAttr, got->cRDNAttr);
    if (got->cRDNAttr)
    {
        DWORD i;

        for (i = 0; i < got->cRDNAttr; i++)
            compareRDNAttrs(&expected->rgRDNAttr[i], &got->rgRDNAttr[i]);
    }
}

static void compareNames(const CERT_NAME_INFO *expected,
 const CERT_NAME_INFO *got)
{
    ok(got->cRDN == expected->cRDN, "Expected %ld RDNs, got %ld\n",
     expected->cRDN, got->cRDN);
    if (got->cRDN)
    {
        DWORD i;

        for (i = 0; i < got->cRDN; i++)
            compareRDNs(&expected->rgRDN[i], &got->rgRDN[i]);
    }
}

static void test_decodeName(DWORD dwEncoding)
{
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;
    CERT_RDN rdn;
    CERT_NAME_INFO info = { 1, &rdn };

    /* test empty name */
    bufSize = 0;
    ret = CryptDecodeObjectEx(dwEncoding, X509_NAME, emptySequence,
     emptySequence[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    /* Interestingly, in Windows, if cRDN is 0, rgRGN may not be NULL.  My
     * decoder works the same way, so only test the count.
     */
    if (buf)
    {
        ok(bufSize == sizeof(CERT_NAME_INFO), "Wrong bufSize %ld\n", bufSize);
        ok(((CERT_NAME_INFO *)buf)->cRDN == 0,
         "Expected 0 RDNs in empty info, got %ld\n",
         ((CERT_NAME_INFO *)buf)->cRDN);
        LocalFree(buf);
    }
    /* test empty RDN */
    bufSize = 0;
    ret = CryptDecodeObjectEx(dwEncoding, X509_NAME, emptyRDNs,
     emptyRDNs[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_NAME_INFO *info = (CERT_NAME_INFO *)buf;

        ok(bufSize == sizeof(CERT_NAME_INFO) + sizeof(CERT_RDN) &&
         info->cRDN == 1 && info->rgRDN && info->rgRDN[0].cRDNAttr == 0,
         "Got unexpected value for empty RDN\n");
        LocalFree(buf);
    }
    /* test two RDN attrs */
    bufSize = 0;
    ret = CryptDecodeObjectEx(dwEncoding, X509_NAME, twoRDNs,
     twoRDNs[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
     (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_RDN_ATTR attrs[] = {
         { szOID_SUR_NAME, CERT_RDN_PRINTABLE_STRING, { sizeof(surName),
          (BYTE *)surName } },
         { szOID_COMMON_NAME, CERT_RDN_PRINTABLE_STRING, { sizeof(commonName),
          (BYTE *)commonName } },
        };

        rdn.cRDNAttr = sizeof(attrs) / sizeof(attrs[0]);
        rdn.rgRDNAttr = attrs;
        compareNames(&info, (CERT_NAME_INFO *)buf);
        LocalFree(buf);
    }
    /* And, a slightly more complicated name */
    buf = NULL;
    bufSize = 0;
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, X509_NAME, encodedRDNAttrs,
     sizeof(encodedRDNAttrs), CRYPT_DECODE_ALLOC_FLAG, NULL, &buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        rdn.cRDNAttr = sizeof(decodedRdnAttrs) / sizeof(decodedRdnAttrs[0]);
        rdn.rgRDNAttr = (PCERT_RDN_ATTR)decodedRdnAttrs;
        compareNames(&info, (CERT_NAME_INFO *)buf);
        LocalFree(buf);
    }
}

struct EncodedNameValue
{
    CERT_NAME_VALUE value;
    const BYTE *encoded;
    DWORD encodedSize;
};

static const char bogusIA5[] = "\x80";
static const char bogusPrintable[] = "~";
static const char bogusNumeric[] = "A";
static WCHAR commonNameW[] = { 'J','u','a','n',' ','L','a','n','g',0 };
static const BYTE bin42[] = { 0x16,0x02,0x80,0x00 };
static const BYTE bin43[] = { 0x13,0x02,0x7e,0x00 };
static const BYTE bin44[] = { 0x12,0x02,0x41,0x00 };
static BYTE octetCommonNameValue[] = {
 0x04,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE numericCommonNameValue[] = {
 0x12,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE printableCommonNameValue[] = {
 0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE t61CommonNameValue[] = {
 0x14,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE videotexCommonNameValue[] = {
 0x15,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE ia5CommonNameValue[] = {
 0x16,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE graphicCommonNameValue[] = {
 0x19,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE visibleCommonNameValue[] = {
 0x1a,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE generalCommonNameValue[] = {
 0x1b,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };
static BYTE bmpCommonNameValue[] = {
 0x1e,0x14,0x00,0x4a,0x00,0x75,0x00,0x61,0x00,0x6e,0x00,0x20,0x00,0x4c,0x00,
 0x61,0x00,0x6e,0x00,0x67,0x00,0x00 };
static BYTE utf8CommonNameValue[] = {
 0x0c,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00 };

static struct EncodedNameValue nameValues[] = {
 { { CERT_RDN_OCTET_STRING, { sizeof(commonName), (BYTE *)commonName } },
     octetCommonNameValue, sizeof(octetCommonNameValue) },
 { { CERT_RDN_NUMERIC_STRING, { sizeof(commonName), (BYTE *)commonName } },
     numericCommonNameValue, sizeof(numericCommonNameValue) },
 { { CERT_RDN_PRINTABLE_STRING, { sizeof(commonName), (BYTE *)commonName } },
     printableCommonNameValue, sizeof(printableCommonNameValue) },
 { { CERT_RDN_T61_STRING, { sizeof(commonName), (BYTE *)commonName } },
     t61CommonNameValue, sizeof(t61CommonNameValue) },
 { { CERT_RDN_VIDEOTEX_STRING, { sizeof(commonName), (BYTE *)commonName } },
     videotexCommonNameValue, sizeof(videotexCommonNameValue) },
 { { CERT_RDN_IA5_STRING, { sizeof(commonName), (BYTE *)commonName } },
     ia5CommonNameValue, sizeof(ia5CommonNameValue) },
 { { CERT_RDN_GRAPHIC_STRING, { sizeof(commonName), (BYTE *)commonName } },
     graphicCommonNameValue, sizeof(graphicCommonNameValue) },
 { { CERT_RDN_VISIBLE_STRING, { sizeof(commonName), (BYTE *)commonName } },
     visibleCommonNameValue, sizeof(visibleCommonNameValue) },
 { { CERT_RDN_GENERAL_STRING, { sizeof(commonName), (BYTE *)commonName } },
     generalCommonNameValue, sizeof(generalCommonNameValue) },
 { { CERT_RDN_BMP_STRING, { sizeof(commonNameW), (BYTE *)commonNameW } },
     bmpCommonNameValue, sizeof(bmpCommonNameValue) },
 { { CERT_RDN_UTF8_STRING, { sizeof(commonNameW), (BYTE *)commonNameW } },
     utf8CommonNameValue, sizeof(utf8CommonNameValue) },
 /* The following tests succeed under Windows, but really should fail,
  * they contain characters that are illegal for the encoding.  I'm
  * including them to justify my lazy encoding.
  */
 { { CERT_RDN_IA5_STRING, { sizeof(bogusIA5), (BYTE *)bogusIA5 } }, bin42,
     sizeof(bin42) },
 { { CERT_RDN_PRINTABLE_STRING, { sizeof(bogusPrintable),
     (BYTE *)bogusPrintable } }, bin43, sizeof(bin43) },
 { { CERT_RDN_NUMERIC_STRING, { sizeof(bogusNumeric), (BYTE *)bogusNumeric } },
     bin44, sizeof(bin44) },
};

static void test_encodeNameValue(DWORD dwEncoding)
{
    BYTE *buf = NULL;
    DWORD size = 0, i;
    BOOL ret;
    CERT_NAME_VALUE value = { 0, { 0, NULL } };

    value.dwValueType = 14;
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_CHOICE,
     "Expected CRYPT_E_ASN1_CHOICE, got %08lx\n", GetLastError());
    value.dwValueType = CERT_RDN_ENCODED_BLOB;
    value.Value.pbData = printableCommonNameValue;
    value.Value.cbData = sizeof(printableCommonNameValue);
    ret = CryptEncodeObjectEx(dwEncoding, X509_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(printableCommonNameValue), "Unexpected size %ld\n",
         size);
        ok(!memcmp(buf, printableCommonNameValue, size),
         "Unexpected encoding\n");
        LocalFree(buf);
    }
    for (i = 0; i < sizeof(nameValues) / sizeof(nameValues[0]); i++)
    {
        ret = CryptEncodeObjectEx(dwEncoding, X509_NAME_VALUE,
         &nameValues[i].value, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
         &size);
        ok(ret, "Type %ld: CryptEncodeObjectEx failed: %08lx\n",
         nameValues[i].value.dwValueType, GetLastError());
        if (buf)
        {
            ok(size == nameValues[i].encodedSize,
             "Expected size %ld, got %ld\n", nameValues[i].encodedSize, size);
            ok(!memcmp(buf, nameValues[i].encoded, size),
             "Got unexpected encoding\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeNameValue(DWORD dwEncoding)
{
    int i;
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;

    for (i = 0; i < sizeof(nameValues) / sizeof(nameValues[0]); i++)
    {
        ret = CryptDecodeObjectEx(dwEncoding, X509_NAME_VALUE,
         nameValues[i].encoded, nameValues[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG | CRYPT_DECODE_SHARE_OID_STRING_FLAG, NULL,
         (BYTE *)&buf, &bufSize);
        ok(ret, "Value type %ld: CryptDecodeObjectEx failed: %08lx\n",
         nameValues[i].value.dwValueType, GetLastError());
        if (buf)
        {
            compareNameValues(&nameValues[i].value,
             (const CERT_NAME_VALUE *)buf);
            LocalFree(buf);
        }
    }
}

static const BYTE emptyURL[] = { 0x30, 0x02, 0x86, 0x00 };
static const WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e',
 'h','q','.','o','r','g',0 };
static const BYTE encodedURL[] = { 0x30, 0x13, 0x86, 0x11, 0x68, 0x74,
 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x69, 0x6e, 0x65, 0x68, 0x71, 0x2e,
 0x6f, 0x72, 0x67 };
static const WCHAR nihongoURL[] = { 'h','t','t','p',':','/','/',0x226f,
 0x575b, 0 };
static const WCHAR dnsName[] = { 'w','i','n','e','h','q','.','o','r','g',0 };
static const BYTE encodedDnsName[] = { 0x30, 0x0c, 0x82, 0x0a, 0x77, 0x69,
 0x6e, 0x65, 0x68, 0x71, 0x2e, 0x6f, 0x72, 0x67 };
static const BYTE localhost[] = { 127, 0, 0, 1 };
static const BYTE encodedIPAddr[] = { 0x30, 0x06, 0x87, 0x04, 0x7f, 0x00, 0x00,
 0x01 };

static void test_encodeAltName(DWORD dwEncoding)
{
    CERT_ALT_NAME_INFO info = { 0 };
    CERT_ALT_NAME_ENTRY entry = { 0 };
    BYTE *buf = NULL;
    DWORD size = 0;
    BOOL ret;

    /* Test with empty info */
    ret = CryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(emptySequence), "Wrong size %ld\n", size);
        ok(!memcmp(buf, emptySequence, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Test with an empty entry */
    info.cAltEntry = 1;
    info.rgAltEntry = &entry;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Test with an empty pointer */
    entry.dwAltNameChoice = CERT_ALT_NAME_URL;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(emptyURL), "Wrong size %ld\n", size);
        ok(!memcmp(buf, emptyURL, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Test with a real URL */
    U(entry).pwszURL = (LPWSTR)url;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(encodedURL), "Wrong size %ld\n", size);
        ok(!memcmp(buf, encodedURL, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Now with the URL containing an invalid IA5 char */
    U(entry).pwszURL = (LPWSTR)nihongoURL;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_IA5_STRING,
     "Expected CRYPT_E_INVALID_IA5_STRING, got %08lx\n", GetLastError());
    /* The first invalid character is at index 7 */
    ok(GET_CERT_ALT_NAME_VALUE_ERR_INDEX(size) == 7,
     "Expected invalid char at index 7, got %ld\n",
     GET_CERT_ALT_NAME_VALUE_ERR_INDEX(size));
    /* Now with the URL missing a scheme */
    U(entry).pwszURL = (LPWSTR)dnsName;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        /* This succeeds, but it shouldn't, so don't worry about conforming */
        LocalFree(buf);
    }
    /* Now with a DNS name */
    entry.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(encodedDnsName), "Wrong size %ld\n", size);
        ok(!memcmp(buf, encodedDnsName, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Test with an IP address */
    entry.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
    U(entry).IPAddress.cbData = sizeof(localhost);
    U(entry).IPAddress.pbData = (LPBYTE)localhost;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(encodedIPAddr), "Wrong size %ld\n", size);
        ok(!memcmp(buf, encodedIPAddr, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeAltName(DWORD dwEncoding)
{
    static const BYTE unimplementedType[] = { 0x30, 0x06, 0x85, 0x04, 0x7f,
     0x00, 0x00, 0x01 };
    static const BYTE bogusType[] = { 0x30, 0x06, 0x89, 0x04, 0x7f, 0x00, 0x00,
     0x01 };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;
    CERT_ALT_NAME_INFO *info;

    /* Test some bogus ones first */
    ret = CryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     unimplementedType, sizeof(unimplementedType), CRYPT_DECODE_ALLOC_FLAG,
     NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_BADTAG,
     "Expected CRYPT_E_ASN1_BADTAG, got %08lx\n", GetLastError());
    ret = CryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME,
     bogusType, sizeof(bogusType), CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_CORRUPT,
     "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
    /* Now expected cases */
    ret = CryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, emptySequence,
     emptySequence[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 0, "Expected 0 entries, got %ld\n",
         info->cAltEntry);
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, emptyURL,
     emptyURL[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %ld\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %ld\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(U(info->rgAltEntry[0]).pwszURL == NULL || !*U(info->rgAltEntry[0]).pwszURL,
         "Expected empty URL\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, encodedURL,
     encodedURL[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %ld\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %ld\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(!lstrcmpW(U(info->rgAltEntry[0]).pwszURL, url), "Unexpected URL\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, encodedDnsName,
     encodedDnsName[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %ld\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_DNS_NAME,
         "Expected CERT_ALT_NAME_DNS_NAME, got %ld\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(!lstrcmpW(U(info->rgAltEntry[0]).pwszDNSName, dnsName),
         "Unexpected DNS name\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_ALTERNATE_NAME, encodedIPAddr,
     encodedIPAddr[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        info = (CERT_ALT_NAME_INFO *)buf;

        ok(info->cAltEntry == 1, "Expected 1 entries, got %ld\n",
         info->cAltEntry);
        ok(info->rgAltEntry[0].dwAltNameChoice == CERT_ALT_NAME_IP_ADDRESS,
         "Expected CERT_ALT_NAME_IP_ADDRESS, got %ld\n",
         info->rgAltEntry[0].dwAltNameChoice);
        ok(U(info->rgAltEntry[0]).IPAddress.cbData == sizeof(localhost),
         "Unexpected IP address length %ld\n",
          U(info->rgAltEntry[0]).IPAddress.cbData);
        ok(!memcmp(U(info->rgAltEntry[0]).IPAddress.pbData, localhost,
         sizeof(localhost)), "Unexpected IP address value\n");
        LocalFree(buf);
    }
}

struct UnicodeExpectedError
{
    DWORD   valueType;
    LPCWSTR str;
    DWORD   errorIndex;
    DWORD   error;
};

static const WCHAR oneW[] = { '1',0 };
static const WCHAR aW[] = { 'a',0 };
static const WCHAR quoteW[] = { '"', 0 };

static struct UnicodeExpectedError unicodeErrors[] = {
 { CERT_RDN_ANY_TYPE,         oneW,       0, CRYPT_E_NOT_CHAR_STRING },
 { CERT_RDN_ENCODED_BLOB,     oneW,       0, CRYPT_E_NOT_CHAR_STRING },
 { CERT_RDN_OCTET_STRING,     oneW,       0, CRYPT_E_NOT_CHAR_STRING },
 { 14,                        oneW,       0, CRYPT_E_ASN1_CHOICE },
 { CERT_RDN_NUMERIC_STRING,   aW,         0, CRYPT_E_INVALID_NUMERIC_STRING },
 { CERT_RDN_PRINTABLE_STRING, quoteW,     0, CRYPT_E_INVALID_PRINTABLE_STRING },
 { CERT_RDN_IA5_STRING,       nihongoURL, 7, CRYPT_E_INVALID_IA5_STRING },
};

struct UnicodeExpectedResult
{
    DWORD           valueType;
    LPCWSTR         str;
    CRYPT_DATA_BLOB encoded;
};

static BYTE oneNumeric[] = { 0x12, 0x01, 0x31 };
static BYTE onePrintable[] = { 0x13, 0x01, 0x31 };
static BYTE oneTeletex[] = { 0x14, 0x01, 0x31 };
static BYTE oneVideotex[] = { 0x15, 0x01, 0x31 };
static BYTE oneIA5[] = { 0x16, 0x01, 0x31 };
static BYTE oneGraphic[] = { 0x19, 0x01, 0x31 };
static BYTE oneVisible[] = { 0x1a, 0x01, 0x31 };
static BYTE oneUniversal[] = { 0x1c, 0x04, 0x00, 0x00, 0x00, 0x31 };
static BYTE oneGeneral[] = { 0x1b, 0x01, 0x31 };
static BYTE oneBMP[] = { 0x1e, 0x02, 0x00, 0x31 };
static BYTE oneUTF8[] = { 0x0c, 0x01, 0x31 };
static BYTE nihongoT61[] = { 0x14,0x09,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x6f,
 0x5b };
static BYTE nihongoGeneral[] = { 0x1b,0x09,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,
 0x6f,0x5b };
static BYTE nihongoBMP[] = { 0x1e,0x12,0x00,0x68,0x00,0x74,0x00,0x74,0x00,0x70,
 0x00,0x3a,0x00,0x2f,0x00,0x2f,0x22,0x6f,0x57,0x5b };
static BYTE nihongoUTF8[] = { 0x0c,0x0d,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,
 0xe2,0x89,0xaf,0xe5,0x9d,0x9b };

static struct UnicodeExpectedResult unicodeResults[] = {
 { CERT_RDN_NUMERIC_STRING,   oneW, { sizeof(oneNumeric), oneNumeric } },
 { CERT_RDN_PRINTABLE_STRING, oneW, { sizeof(onePrintable), onePrintable } },
 { CERT_RDN_TELETEX_STRING,   oneW, { sizeof(oneTeletex), oneTeletex } },
 { CERT_RDN_VIDEOTEX_STRING,  oneW, { sizeof(oneVideotex), oneVideotex } },
 { CERT_RDN_IA5_STRING,       oneW, { sizeof(oneIA5), oneIA5 } },
 { CERT_RDN_GRAPHIC_STRING,   oneW, { sizeof(oneGraphic), oneGraphic } },
 { CERT_RDN_VISIBLE_STRING,   oneW, { sizeof(oneVisible), oneVisible } },
 { CERT_RDN_UNIVERSAL_STRING, oneW, { sizeof(oneUniversal), oneUniversal } },
 { CERT_RDN_GENERAL_STRING,   oneW, { sizeof(oneGeneral), oneGeneral } },
 { CERT_RDN_BMP_STRING,       oneW, { sizeof(oneBMP), oneBMP } },
 { CERT_RDN_UTF8_STRING,      oneW, { sizeof(oneUTF8), oneUTF8 } },
 { CERT_RDN_BMP_STRING,     nihongoURL, { sizeof(nihongoBMP), nihongoBMP } },
 { CERT_RDN_UTF8_STRING,    nihongoURL, { sizeof(nihongoUTF8), nihongoUTF8 } },
};

static struct UnicodeExpectedResult unicodeWeirdness[] = {
 { CERT_RDN_TELETEX_STRING, nihongoURL, { sizeof(nihongoT61), nihongoT61 } },
 { CERT_RDN_GENERAL_STRING, nihongoURL, { sizeof(nihongoGeneral), nihongoGeneral } },
};

static void test_encodeUnicodeNameValue(DWORD dwEncoding)
{
    BYTE *buf = NULL;
    DWORD size = 0, i;
    BOOL ret;
    CERT_NAME_VALUE value;

    ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, NULL,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    /* Have to have a string of some sort */
    value.dwValueType = 0; /* aka CERT_RDN_ANY_TYPE */
    value.Value.pbData = NULL;
    value.Value.cbData = 0;
    ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08lx\n", GetLastError());
    value.dwValueType = CERT_RDN_ENCODED_BLOB;
    ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08lx\n", GetLastError());
    value.dwValueType = CERT_RDN_ANY_TYPE;
    value.Value.pbData = (LPBYTE)oneW;
    ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08lx\n", GetLastError());
    value.Value.cbData = sizeof(oneW);
    ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08lx\n", GetLastError());
    /* An encoded string with specified length isn't good enough either */
    value.dwValueType = CERT_RDN_ENCODED_BLOB;
    value.Value.pbData = oneUniversal;
    value.Value.cbData = sizeof(oneUniversal);
    ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_CHAR_STRING,
     "Expected CRYPT_E_NOT_CHAR_STRING, got %08lx\n", GetLastError());
    /* More failure checking */
    value.Value.cbData = 0;
    for (i = 0; i < sizeof(unicodeErrors) / sizeof(unicodeErrors[0]); i++)
    {
        value.Value.pbData = (LPBYTE)unicodeErrors[i].str;
        value.dwValueType = unicodeErrors[i].valueType;
        ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
        ok(!ret && GetLastError() == unicodeErrors[i].error,
         "Value type %ld: expected %08lx, got %08lx\n", value.dwValueType,
         unicodeErrors[i].error, GetLastError());
        ok(size == unicodeErrors[i].errorIndex,
         "Expected error index %ld, got %ld\n", unicodeErrors[i].errorIndex,
         size);
    }
    /* cbData can be zero if the string is NULL-terminated */
    value.Value.cbData = 0;
    for (i = 0; i < sizeof(unicodeResults) / sizeof(unicodeResults[0]); i++)
    {
        value.Value.pbData = (LPBYTE)unicodeResults[i].str;
        value.dwValueType = unicodeResults[i].valueType;
        ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
        ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            ok(size == unicodeResults[i].encoded.cbData,
             "Value type %ld: expected size %ld, got %ld\n",
             value.dwValueType, unicodeResults[i].encoded.cbData, size);
            ok(!memcmp(unicodeResults[i].encoded.pbData, buf, size),
             "Value type %ld: unexpected value\n", value.dwValueType);
            LocalFree(buf);
        }
    }
    /* These "encode," but they do so by truncating each unicode character
     * rather than properly encoding it.  Kept separate from the proper results,
     * because the encoded forms won't decode to their original strings.
     */
    for (i = 0; i < sizeof(unicodeWeirdness) / sizeof(unicodeWeirdness[0]); i++)
    {
        value.Value.pbData = (LPBYTE)unicodeWeirdness[i].str;
        value.dwValueType = unicodeWeirdness[i].valueType;
        ret = CryptEncodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE, &value,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
        ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            ok(size == unicodeWeirdness[i].encoded.cbData,
             "Value type %ld: expected size %ld, got %ld\n",
             value.dwValueType, unicodeWeirdness[i].encoded.cbData, size);
            ok(!memcmp(unicodeWeirdness[i].encoded.pbData, buf, size),
             "Value type %ld: unexpected value\n", value.dwValueType);
            LocalFree(buf);
        }
    }
}

static void test_decodeUnicodeNameValue(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(unicodeResults) / sizeof(unicodeResults[0]); i++)
    {
        BYTE *buf = NULL;
        BOOL ret;
        DWORD size = 0;

        ret = CryptDecodeObjectEx(dwEncoding, X509_UNICODE_NAME_VALUE,
         unicodeResults[i].encoded.pbData, unicodeResults[i].encoded.cbData,
         CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
        ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
        if (ret && buf)
        {
            PCERT_NAME_VALUE value = (PCERT_NAME_VALUE)buf;

            ok(value->dwValueType == unicodeResults[i].valueType,
             "Expected value type %ld, got %ld\n", unicodeResults[i].valueType,
             value->dwValueType);
            ok(!lstrcmpW((LPWSTR)value->Value.pbData, unicodeResults[i].str),
             "Unexpected decoded value for index %ld (value type %ld)\n", i,
             unicodeResults[i].valueType);
            LocalFree(buf);
        }
    }
}

struct encodedOctets
{
    const BYTE *val;
    const BYTE *encoded;
};

static const unsigned char bin46[] = { 'h','i',0 };
static const unsigned char bin47[] = { 0x04,0x02,'h','i',0 };
static const unsigned char bin48[] = {
     's','o','m','e','l','o','n','g',0xff,'s','t','r','i','n','g',0 };
static const unsigned char bin49[] = {
     0x04,0x0f,'s','o','m','e','l','o','n','g',0xff,'s','t','r','i','n','g',0 };
static const unsigned char bin50[] = { 0 };
static const unsigned char bin51[] = { 0x04,0x00,0 };

static const struct encodedOctets octets[] = {
    { bin46, bin47 },
    { bin48, bin49 },
    { bin50, bin51 },
};

static void test_encodeOctets(DWORD dwEncoding)
{
    CRYPT_DATA_BLOB blob;
    DWORD i;

    for (i = 0; i < sizeof(octets) / sizeof(octets[0]); i++)
    {
        BYTE *buf = NULL;
        BOOL ret;
        DWORD bufSize = 0;

        blob.cbData = strlen((const char*)octets[i].val);
        blob.pbData = (BYTE*)octets[i].val;
        ret = CryptEncodeObjectEx(dwEncoding, X509_OCTET_STRING, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %ld\n", GetLastError());
        if (buf)
        {
            ok(buf[0] == 4,
             "Got unexpected type %d for octet string (expected 4)\n", buf[0]);
            ok(buf[1] == octets[i].encoded[1], "Got length %d, expected %d\n",
             buf[1], octets[i].encoded[1]);
            ok(!memcmp(buf + 1, octets[i].encoded + 1,
             octets[i].encoded[1] + 1), "Got unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeOctets(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(octets) / sizeof(octets[0]); i++)
    {
        BYTE *buf = NULL;
        BOOL ret;
        DWORD bufSize = 0;

        ret = CryptDecodeObjectEx(dwEncoding, X509_OCTET_STRING,
         (BYTE *)octets[i].encoded, octets[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
        ok(bufSize >= sizeof(CRYPT_DATA_BLOB) + octets[i].encoded[1],
         "Expected size >= %d, got %ld\n",
           (int)sizeof(CRYPT_DATA_BLOB) + octets[i].encoded[1], bufSize);
        ok(buf != NULL, "Expected allocated buffer\n");
        if (buf)
        {
            CRYPT_DATA_BLOB *blob = (CRYPT_DATA_BLOB *)buf;

            if (blob->cbData)
                ok(!memcmp(blob->pbData, octets[i].val, blob->cbData),
                 "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static const BYTE bytesToEncode[] = { 0xff, 0xff };

struct encodedBits
{
    DWORD cUnusedBits;
    const BYTE *encoded;
    DWORD cbDecoded;
    const BYTE *decoded;
};

static const unsigned char bin52[] = { 0x03,0x03,0x00,0xff,0xff };
static const unsigned char bin53[] = { 0xff,0xff };
static const unsigned char bin54[] = { 0x03,0x03,0x01,0xff,0xfe };
static const unsigned char bin55[] = { 0xff,0xfe };
static const unsigned char bin56[] = { 0x03,0x02,0x01,0xfe };
static const unsigned char bin57[] = { 0xfe };
static const unsigned char bin58[] = { 0x03,0x01,0x00 };

static const struct encodedBits bits[] = {
    /* normal test cases */
    { 0, bin52, 2, bin53 },
    { 1, bin54, 2, bin55 },
    /* strange test case, showing cUnusedBits >= 8 is allowed */
    { 9, bin56, 1, bin57 },
    /* even stranger test case, showing cUnusedBits > cbData * 8 is allowed */
    { 17, bin58, 0, NULL },
};

static void test_encodeBits(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(bits) / sizeof(bits[0]); i++)
    {
        CRYPT_BIT_BLOB blob;
        BOOL ret;
        BYTE *buf = NULL;
        DWORD bufSize = 0;

        blob.cbData = sizeof(bytesToEncode);
        blob.pbData = (BYTE *)bytesToEncode;
        blob.cUnusedBits = bits[i].cUnusedBits;
        ret = CryptEncodeObjectEx(dwEncoding, X509_BITS, &blob,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            ok(bufSize == bits[i].encoded[1] + 2,
             "Got unexpected size %ld, expected %d\n", bufSize,
             bits[i].encoded[1] + 2);
            ok(!memcmp(buf, bits[i].encoded, bits[i].encoded[1] + 2),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeBits(DWORD dwEncoding)
{
    static const BYTE ber[] = "\x03\x02\x01\xff";
    static const BYTE berDecoded = 0xfe;
    DWORD i;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    /* normal cases */
    for (i = 0; i < sizeof(bits) / sizeof(bits[0]); i++)
    {
        ret = CryptDecodeObjectEx(dwEncoding, X509_BITS, bits[i].encoded,
         bits[i].encoded[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
         &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            CRYPT_BIT_BLOB *blob;

            ok(bufSize >= sizeof(CRYPT_BIT_BLOB) + bits[i].cbDecoded,
             "Got unexpected size %ld, expected >= %ld\n", bufSize,
             sizeof(CRYPT_BIT_BLOB) + bits[i].cbDecoded);
            blob = (CRYPT_BIT_BLOB *)buf;
            ok(blob->cbData == bits[i].cbDecoded,
             "Got unexpected length %ld, expected %ld\n", blob->cbData,
             bits[i].cbDecoded);
            if (blob->cbData && bits[i].cbDecoded)
                ok(!memcmp(blob->pbData, bits[i].decoded, bits[i].cbDecoded),
                 "Unexpected value\n");
            LocalFree(buf);
        }
    }
    /* special case: check that something that's valid in BER but not in DER
     * decodes successfully
     */
    ret = CryptDecodeObjectEx(dwEncoding, X509_BITS, ber, ber[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRYPT_BIT_BLOB *blob;

        ok(bufSize >= sizeof(CRYPT_BIT_BLOB) + sizeof(berDecoded),
           "Got unexpected size %ld\n", bufSize);
        blob = (CRYPT_BIT_BLOB *)buf;
        ok(blob->cbData == sizeof(berDecoded),
           "Got unexpected length %ld\n", blob->cbData);
        if (blob->cbData)
            ok(*blob->pbData == berDecoded, "Unexpected value\n");
        LocalFree(buf);
    }
}

struct Constraints2
{
    CERT_BASIC_CONSTRAINTS2_INFO info;
    const BYTE *encoded;
};

static const unsigned char bin59[] = { 0x30,0x00 };
static const unsigned char bin60[] = { 0x30,0x03,0x01,0x01,0xff };
static const unsigned char bin61[] = { 0x30,0x03,0x02,0x01,0x00 };
static const unsigned char bin62[] = { 0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const struct Constraints2 constraints2[] = {
 /* empty constraints */
 { { FALSE, FALSE, 0}, bin59 },
 /* can be a CA */
 { { TRUE,  FALSE, 0}, bin60 },
 /* has path length constraints set (MSDN implies fCA needs to be TRUE as well,
  * but that's not the case
  */
 { { FALSE, TRUE,  0}, bin61 },
 /* can be a CA and has path length constraints set */
 { { TRUE,  TRUE,  1}, bin62 },
};

static const BYTE emptyConstraint[] = { 0x30, 0x03, 0x03, 0x01, 0x00 };
static const BYTE encodedDomainName[] = { 0x30, 0x2b, 0x31, 0x29, 0x30, 0x11,
 0x06, 0x0a, 0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19, 0x16,
 0x03, 0x6f, 0x72, 0x67, 0x30, 0x14, 0x06, 0x0a, 0x09, 0x92, 0x26, 0x89, 0x93,
 0xf2, 0x2c, 0x64, 0x01, 0x19, 0x16, 0x06, 0x77, 0x69, 0x6e, 0x65, 0x68, 0x71 };
static const BYTE constraintWithDomainName[] = { 0x30, 0x32, 0x03, 0x01, 0x00,
 0x30, 0x2d, 0x30, 0x2b, 0x31, 0x29, 0x30, 0x11, 0x06, 0x0a, 0x09, 0x92, 0x26,
 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19, 0x16, 0x03, 0x6f, 0x72, 0x67, 0x30,
 0x14, 0x06, 0x0a, 0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19,
 0x16, 0x06, 0x77, 0x69, 0x6e, 0x65, 0x68, 0x71 };

static void test_encodeBasicConstraints(DWORD dwEncoding)
{
    DWORD i, bufSize = 0;
    CERT_BASIC_CONSTRAINTS_INFO info;
    CERT_NAME_BLOB nameBlob = { sizeof(encodedDomainName),
     (LPBYTE)encodedDomainName };
    BOOL ret;
    BYTE *buf = NULL;

    /* First test with the simpler info2 */
    for (i = 0; i < sizeof(constraints2) / sizeof(constraints2[0]); i++)
    {
        ret = CryptEncodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
         &constraints2[i].info, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
         &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            ok(bufSize == constraints2[i].encoded[1] + 2,
             "Expected %d bytes, got %ld\n", constraints2[i].encoded[1] + 2,
             bufSize);
            ok(!memcmp(buf, constraints2[i].encoded,
             constraints2[i].encoded[1] + 2), "Unexpected value\n");
            LocalFree(buf);
        }
    }
    /* Now test with more complex basic constraints */
    info.SubjectType.cbData = 0;
    info.fPathLenConstraint = FALSE;
    info.cSubtreesConstraint = 0;
    ret = CryptEncodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(bufSize == sizeof(emptyConstraint), "Wrong size %ld\n", bufSize);
        ok(!memcmp(buf, emptyConstraint, sizeof(emptyConstraint)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    /* None of the certs I examined had any subtree constraint, but I test one
     * anyway just in case.
     */
    info.cSubtreesConstraint = 1;
    info.rgSubtreesConstraint = &nameBlob;
    ret = CryptEncodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(bufSize == sizeof(constraintWithDomainName), "Wrong size %ld\n", bufSize);
        ok(!memcmp(buf, constraintWithDomainName,
         sizeof(constraintWithDomainName)), "Unexpected value\n");
        LocalFree(buf);
    }
    /* FIXME: test encoding with subject type. */
}

static const unsigned char bin63[] = { 0x30,0x06,0x01,0x01,0x01,0x02,0x01,0x01 };
static const unsigned char encodedCommonName[] = {
    0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,'J','u','a','n',' ','L','a','n','g',0};

static void test_decodeBasicConstraints(DWORD dwEncoding)
{
    static const BYTE inverted[] = { 0x30, 0x06, 0x02, 0x01, 0x01, 0x01, 0x01,
     0xff };
    static const struct Constraints2 badBool = { { TRUE, TRUE, 1 }, bin63 };
    DWORD i;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    /* First test with simpler info2 */
    for (i = 0; i < sizeof(constraints2) / sizeof(constraints2[0]); i++)
    {
        ret = CryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
         constraints2[i].encoded, constraints2[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed for item %ld: %08lx\n", i,
         GetLastError());
        if (buf)
        {
            CERT_BASIC_CONSTRAINTS2_INFO *info =
             (CERT_BASIC_CONSTRAINTS2_INFO *)buf;

            ok(!memcmp(info, &constraints2[i].info, sizeof(*info)),
             "Unexpected value for item %ld\n", i);
            LocalFree(buf);
        }
    }
    /* Check with the order of encoded elements inverted */
    buf = (PBYTE)1;
    ret = CryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
     inverted, inverted[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_CORRUPT,
     "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
    ok(!buf, "Expected buf to be set to NULL\n");
    /* Check with a non-DER bool */
    ret = CryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
     badBool.encoded, badBool.encoded[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_BASIC_CONSTRAINTS2_INFO *info =
         (CERT_BASIC_CONSTRAINTS2_INFO *)buf;

        ok(!memcmp(info, &badBool.info, sizeof(*info)), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Check with a non-basic constraints value */
    ret = CryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS2,
     (LPBYTE)encodedCommonName, encodedCommonName[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_CORRUPT,
     "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
    /* Now check with the more complex CERT_BASIC_CONSTRAINTS_INFO */
    ret = CryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS,
     emptyConstraint, sizeof(emptyConstraint), CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_BASIC_CONSTRAINTS_INFO *info = (CERT_BASIC_CONSTRAINTS_INFO *)buf;

        ok(info->SubjectType.cbData == 0, "Expected no subject type\n");
        ok(!info->fPathLenConstraint, "Expected no path length constraint\n");
        ok(info->cSubtreesConstraint == 0, "Expected no subtree constraints\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_BASIC_CONSTRAINTS,
     constraintWithDomainName, sizeof(constraintWithDomainName),
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_BASIC_CONSTRAINTS_INFO *info = (CERT_BASIC_CONSTRAINTS_INFO *)buf;

        ok(info->SubjectType.cbData == 0, "Expected no subject type\n");
        ok(!info->fPathLenConstraint, "Expected no path length constraint\n");
        ok(info->cSubtreesConstraint == 1, "Expected a subtree constraint\n");
        if (info->cSubtreesConstraint && info->rgSubtreesConstraint)
        {
            ok(info->rgSubtreesConstraint[0].cbData ==
             sizeof(encodedDomainName), "Wrong size %ld\n",
             info->rgSubtreesConstraint[0].cbData);
            ok(!memcmp(info->rgSubtreesConstraint[0].pbData, encodedDomainName,
             sizeof(encodedDomainName)), "Unexpected value\n");
        }
        LocalFree(buf);
    }
}

/* These are terrible public keys of course, I'm just testing encoding */
static const BYTE modulus1[] = { 0,0,0,1,1,1,1,1 };
static const BYTE modulus2[] = { 1,1,1,1,1,0,0,0 };
static const BYTE modulus3[] = { 0x80,1,1,1,1,0,0,0 };
static const BYTE modulus4[] = { 1,1,1,1,1,0,0,0x80 };
static const BYTE mod1_encoded[] = { 0x30,0x0f,0x02,0x08,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x02,0x03,0x01,0x00,0x01 };
static const BYTE mod2_encoded[] = { 0x30,0x0c,0x02,0x05,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x01,0x00,0x01 };
static const BYTE mod3_encoded[] = { 0x30,0x0c,0x02,0x05,0x01,0x01,0x01,0x01,0x80,0x02,0x03,0x01,0x00,0x01 };
static const BYTE mod4_encoded[] = { 0x30,0x10,0x02,0x09,0x00,0x80,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x02,0x03,0x01,0x00,0x01 };

struct EncodedRSAPubKey
{
    const BYTE *modulus;
    size_t modulusLen;
    const BYTE *encoded;
    size_t decodedModulusLen;
};

struct EncodedRSAPubKey rsaPubKeys[] = {
    { modulus1, sizeof(modulus1), mod1_encoded, sizeof(modulus1) },
    { modulus2, sizeof(modulus2), mod2_encoded, 5 },
    { modulus3, sizeof(modulus3), mod3_encoded, 5 },
    { modulus4, sizeof(modulus4), mod4_encoded, 8 },
};

static void test_encodeRsaPublicKey(DWORD dwEncoding)
{
    BYTE toEncode[sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) + sizeof(modulus1)];
    BLOBHEADER *hdr = (BLOBHEADER *)toEncode;
    RSAPUBKEY *rsaPubKey = (RSAPUBKEY *)(toEncode + sizeof(BLOBHEADER));
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0, i;

    /* Try with a bogus blob type */
    hdr->bType = 2;
    hdr->bVersion = CUR_BLOB_VERSION;
    hdr->reserved = 0;
    hdr->aiKeyAlg = CALG_RSA_KEYX;
    rsaPubKey->magic = 0x31415352;
    rsaPubKey->bitlen = sizeof(modulus1) * 8;
    rsaPubKey->pubexp = 65537;
    memcpy(toEncode + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY), modulus1,
     sizeof(modulus1));

    ret = CryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Now with a bogus reserved field */
    hdr->bType = PUBLICKEYBLOB;
    hdr->reserved = 1;
    ret = CryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    if (buf)
    {
        ok(bufSize == rsaPubKeys[0].encoded[1] + 2,
         "Expected size %d, got %ld\n", rsaPubKeys[0].encoded[1] + 2, bufSize);
        ok(!memcmp(buf, rsaPubKeys[0].encoded, bufSize), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Now with a bogus blob version */
    hdr->reserved = 0;
    hdr->bVersion = 0;
    ret = CryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    if (buf)
    {
        ok(bufSize == rsaPubKeys[0].encoded[1] + 2,
         "Expected size %d, got %ld\n", rsaPubKeys[0].encoded[1] + 2, bufSize);
        ok(!memcmp(buf, rsaPubKeys[0].encoded, bufSize), "Unexpected value\n");
        LocalFree(buf);
    }
    /* And with a bogus alg ID */
    hdr->bVersion = CUR_BLOB_VERSION;
    hdr->aiKeyAlg = CALG_DES;
    ret = CryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    if (buf)
    {
        ok(bufSize == rsaPubKeys[0].encoded[1] + 2,
         "Expected size %d, got %ld\n", rsaPubKeys[0].encoded[1] + 2, bufSize);
        ok(!memcmp(buf, rsaPubKeys[0].encoded, bufSize), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Check a couple of RSA-related OIDs */
    hdr->aiKeyAlg = CALG_RSA_KEYX;
    ret = CryptEncodeObjectEx(dwEncoding, szOID_RSA_RSA,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    ret = CryptEncodeObjectEx(dwEncoding, szOID_RSA_SHA1RSA,
     toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    /* Finally, all valid */
    hdr->aiKeyAlg = CALG_RSA_KEYX;
    for (i = 0; i < sizeof(rsaPubKeys) / sizeof(rsaPubKeys[0]); i++)
    {
        memcpy(toEncode + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY),
         rsaPubKeys[i].modulus, rsaPubKeys[i].modulusLen);
        ret = CryptEncodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
         toEncode, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            ok(bufSize == rsaPubKeys[i].encoded[1] + 2,
             "Expected size %d, got %ld\n", rsaPubKeys[i].encoded[1] + 2,
             bufSize);
            ok(!memcmp(buf, rsaPubKeys[i].encoded, bufSize),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeRsaPublicKey(DWORD dwEncoding)
{
    DWORD i;
    LPBYTE buf = NULL;
    DWORD bufSize = 0;
    BOOL ret;

    /* Try with a bad length */
    ret = CryptDecodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
     rsaPubKeys[0].encoded, rsaPubKeys[0].encoded[1],
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", CRYPT_E_ASN1_EOD);
    /* Try with a couple of RSA-related OIDs */
    ret = CryptDecodeObjectEx(dwEncoding, szOID_RSA_RSA,
     rsaPubKeys[0].encoded, rsaPubKeys[0].encoded[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    ret = CryptDecodeObjectEx(dwEncoding, szOID_RSA_SHA1RSA,
     rsaPubKeys[0].encoded, rsaPubKeys[0].encoded[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    /* Now try success cases */
    for (i = 0; i < sizeof(rsaPubKeys) / sizeof(rsaPubKeys[0]); i++)
    {
        bufSize = 0;
        ret = CryptDecodeObjectEx(dwEncoding, RSA_CSP_PUBLICKEYBLOB,
         rsaPubKeys[i].encoded, rsaPubKeys[i].encoded[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            BLOBHEADER *hdr = (BLOBHEADER *)buf;
            RSAPUBKEY *rsaPubKey = (RSAPUBKEY *)(buf + sizeof(BLOBHEADER));

            ok(bufSize >= sizeof(BLOBHEADER) + sizeof(RSAPUBKEY) +
             rsaPubKeys[i].decodedModulusLen,
             "Wrong size %ld\n", bufSize);
            ok(hdr->bType == PUBLICKEYBLOB,
             "Expected type PUBLICKEYBLOB (%d), got %d\n", PUBLICKEYBLOB,
             hdr->bType);
            ok(hdr->bVersion == CUR_BLOB_VERSION,
             "Expected version CUR_BLOB_VERSION (%d), got %d\n",
             CUR_BLOB_VERSION, hdr->bVersion);
            ok(hdr->reserved == 0, "Expected reserved 0, got %d\n",
             hdr->reserved);
            ok(hdr->aiKeyAlg == CALG_RSA_KEYX,
             "Expected CALG_RSA_KEYX, got %08x\n", hdr->aiKeyAlg);
            ok(rsaPubKey->magic == 0x31415352,
             "Expected magic RSA1, got %08lx\n", rsaPubKey->magic);
            ok(rsaPubKey->bitlen == rsaPubKeys[i].decodedModulusLen * 8,
             "Wrong bit len %ld\n", rsaPubKey->bitlen);
            ok(rsaPubKey->pubexp == 65537, "Expected pubexp 65537, got %ld\n",
             rsaPubKey->pubexp);
            ok(!memcmp(buf + sizeof(BLOBHEADER) + sizeof(RSAPUBKEY),
             rsaPubKeys[i].modulus, rsaPubKeys[i].decodedModulusLen),
             "Unexpected modulus\n");
            LocalFree(buf);
        }
    }
}

static const BYTE intSequence[] = { 0x30, 0x1b, 0x02, 0x01, 0x01, 0x02, 0x01,
 0x7f, 0x02, 0x02, 0x00, 0x80, 0x02, 0x02, 0x01, 0x00, 0x02, 0x01, 0x80, 0x02,
 0x02, 0xff, 0x7f, 0x02, 0x04, 0xba, 0xdd, 0xf0, 0x0d };

static const BYTE mixedSequence[] = { 0x30, 0x27, 0x17, 0x0d, 0x30, 0x35, 0x30,
 0x36, 0x30, 0x36, 0x31, 0x36, 0x31, 0x30, 0x30, 0x30, 0x5a, 0x02, 0x01, 0x7f,
 0x02, 0x02, 0x00, 0x80, 0x02, 0x02, 0x01, 0x00, 0x02, 0x01, 0x80, 0x02, 0x02,
 0xff, 0x7f, 0x02, 0x04, 0xba, 0xdd, 0xf0, 0x0d };

static void test_encodeSequenceOfAny(DWORD dwEncoding)
{
    CRYPT_DER_BLOB blobs[sizeof(ints) / sizeof(ints[0])];
    CRYPT_SEQUENCE_OF_ANY seq;
    DWORD i;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    /* Encode a homogenous sequence */
    for (i = 0; i < sizeof(ints) / sizeof(ints[0]); i++)
    {
        blobs[i].cbData = ints[i].encoded[1] + 2;
        blobs[i].pbData = (BYTE *)ints[i].encoded;
    }
    seq.cValue = sizeof(ints) / sizeof(ints[0]);
    seq.rgValue = blobs;

    ret = CryptEncodeObjectEx(dwEncoding, X509_SEQUENCE_OF_ANY, &seq,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(bufSize == sizeof(intSequence), "Wrong size %ld\n", bufSize);
        ok(!memcmp(buf, intSequence, intSequence[1] + 2), "Unexpected value\n");
        LocalFree(buf);
    }
    /* Change the type of the first element in the sequence, and give it
     * another go
     */
    blobs[0].cbData = times[0].encodedTime[1] + 2;
    blobs[0].pbData = (BYTE *)times[0].encodedTime;
    ret = CryptEncodeObjectEx(dwEncoding, X509_SEQUENCE_OF_ANY, &seq,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(bufSize == sizeof(mixedSequence), "Wrong size %ld\n", bufSize);
        ok(!memcmp(buf, mixedSequence, mixedSequence[1] + 2),
         "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeSequenceOfAny(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    ret = CryptDecodeObjectEx(dwEncoding, X509_SEQUENCE_OF_ANY, intSequence,
     intSequence[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRYPT_SEQUENCE_OF_ANY *seq = (CRYPT_SEQUENCE_OF_ANY *)buf;
        DWORD i;

        ok(seq->cValue == sizeof(ints) / sizeof(ints[0]),
         "Wrong elements %ld\n", seq->cValue);
        for (i = 0; i < min(seq->cValue, sizeof(ints) / sizeof(ints[0])); i++)
        {
            ok(seq->rgValue[i].cbData == ints[i].encoded[1] + 2,
             "Expected %d bytes, got %ld\n", ints[i].encoded[1] + 2,
             seq->rgValue[i].cbData);
            ok(!memcmp(seq->rgValue[i].pbData, ints[i].encoded,
             ints[i].encoded[1] + 2), "Unexpected value\n");
        }
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_SEQUENCE_OF_ANY, mixedSequence,
     mixedSequence[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
     &bufSize);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRYPT_SEQUENCE_OF_ANY *seq = (CRYPT_SEQUENCE_OF_ANY *)buf;

        ok(seq->cValue == sizeof(ints) / sizeof(ints[0]),
         "Wrong elements %ld\n", seq->cValue);
        /* Just check the first element since it's all that changed */
        ok(seq->rgValue[0].cbData == times[0].encodedTime[1] + 2,
         "Expected %d bytes, got %ld\n", times[0].encodedTime[1] + 2,
         seq->rgValue[0].cbData);
        ok(!memcmp(seq->rgValue[0].pbData, times[0].encodedTime,
         times[0].encodedTime[1] + 2), "Unexpected value\n");
        LocalFree(buf);
    }
}

struct encodedExtensions
{
    CERT_EXTENSIONS exts;
    const BYTE *encoded;
};

static BYTE crit_ext_data[] = { 0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static BYTE noncrit_ext_data[] = { 0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };

static CERT_EXTENSION criticalExt =
 { szOID_BASIC_CONSTRAINTS2, TRUE, { 8, crit_ext_data } };
static CERT_EXTENSION nonCriticalExt =
 { szOID_BASIC_CONSTRAINTS2, FALSE, { 8, noncrit_ext_data } };

static const BYTE ext0[] = { 0x30,0x00 };
static const BYTE ext1[] = { 0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,
                             0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE ext2[] = { 0x30,0x11,0x30,0x0f,0x06,0x03,0x55,0x1d,0x13,0x04,
                             0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };

static const struct encodedExtensions exts[] = {
 { { 0, NULL }, ext0 },
 { { 1, &criticalExt }, ext1 },
 { { 1, &nonCriticalExt }, ext2 },
};

static void test_encodeExtensions(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(exts) / sizeof(exts[i]); i++)
    {
        BOOL ret;
        BYTE *buf = NULL;
        DWORD bufSize = 0;

        ret = CryptEncodeObjectEx(dwEncoding, X509_EXTENSIONS, &exts[i].exts,
         CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            ok(bufSize == exts[i].encoded[1] + 2,
             "Expected %d bytes, got %ld\n", exts[i].encoded[1] + 2, bufSize);
            ok(!memcmp(buf, exts[i].encoded, exts[i].encoded[1] + 2),
             "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void test_decodeExtensions(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(exts) / sizeof(exts[i]); i++)
    {
        BOOL ret;
        BYTE *buf = NULL;
        DWORD bufSize = 0;

        ret = CryptDecodeObjectEx(dwEncoding, X509_EXTENSIONS,
         exts[i].encoded, exts[i].encoded[1] + 2, CRYPT_DECODE_ALLOC_FLAG,
         NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            CERT_EXTENSIONS *ext = (CERT_EXTENSIONS *)buf;
            DWORD j;

            ok(ext->cExtension == exts[i].exts.cExtension,
             "Expected %ld extensions, see %ld\n", exts[i].exts.cExtension,
             ext->cExtension);
            for (j = 0; j < min(ext->cExtension, exts[i].exts.cExtension); j++)
            {
                ok(!strcmp(ext->rgExtension[j].pszObjId,
                 exts[i].exts.rgExtension[j].pszObjId),
                 "Expected OID %s, got %s\n",
                 exts[i].exts.rgExtension[j].pszObjId,
                 ext->rgExtension[j].pszObjId);
                ok(!memcmp(ext->rgExtension[j].Value.pbData,
                 exts[i].exts.rgExtension[j].Value.pbData,
                 exts[i].exts.rgExtension[j].Value.cbData),
                 "Unexpected value\n");
            }
            LocalFree(buf);
        }
    }
}

/* MS encodes public key info with a NULL if the algorithm identifier's
 * parameters are empty.  However, when encoding an algorithm in a CERT_INFO,
 * it encodes them by omitting the algorithm parameters.  This latter approach
 * seems more correct, so accept either form.
 */
struct encodedPublicKey
{
    CERT_PUBLIC_KEY_INFO info;
    const BYTE *encoded;
    const BYTE *encodedNoNull;
    CERT_PUBLIC_KEY_INFO decoded;
};

static const BYTE aKey[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd,
 0xe, 0xf };
static const BYTE params[] = { 0x02, 0x01, 0x01 };

static const unsigned char bin64[] = {
    0x30,0x0b,0x30,0x06,0x06,0x02,0x2a,0x03,0x05,0x00,0x03,0x01,0x00};
static const unsigned char bin65[] = {
    0x30,0x09,0x30,0x04,0x06,0x02,0x2a,0x03,0x03,0x01,0x00};
static const unsigned char bin66[] = {
    0x30,0x0f,0x30,0x0a,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x05,0x00,0x03,0x01,0x00};
static const unsigned char bin67[] = {
    0x30,0x0d,0x30,0x08,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x03,0x01,0x00};
static const unsigned char bin68[] = {
    0x30,0x1f,0x30,0x0a,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x05,0x00,0x03,0x11,0x00,0x00,0x01,
    0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
static const unsigned char bin69[] = {
    0x30,0x1d,0x30,0x08,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x03,0x11,0x00,0x00,0x01,
    0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
static const unsigned char bin70[] = {
    0x30,0x20,0x30,0x0b,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x01,0x01,
    0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f};
static const unsigned char bin71[] = {
    0x30,0x20,0x30,0x0b,0x06,0x06,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x01,0x01,
    0x03,0x11,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,
    0x0f};
static unsigned char bin72[] = { 0x05,0x00};

static const struct encodedPublicKey pubKeys[] = {
 /* with a bogus OID */
 { { { "1.2.3", { 0, NULL } }, { 0, NULL, 0 } },
  bin64, bin65,
  { { "1.2.3", { 2, bin72 } }, { 0, NULL, 0 } } },
 /* some normal keys */
 { { { szOID_RSA, { 0, NULL } }, { 0, NULL, 0} },
  bin66, bin67,
  { { szOID_RSA, { 2, bin72 } }, { 0, NULL, 0 } } },
 { { { szOID_RSA, { 0, NULL } }, { sizeof(aKey), (BYTE *)aKey, 0} },
  bin68, bin69,
  { { szOID_RSA, { 2, bin72 } }, { sizeof(aKey), (BYTE *)aKey, 0} } },
 /* with add'l parameters--note they must be DER-encoded */
 { { { szOID_RSA, { sizeof(params), (BYTE *)params } }, { sizeof(aKey),
  (BYTE *)aKey, 0 } },
  bin70, bin71,
  { { szOID_RSA, { sizeof(params), (BYTE *)params } }, { sizeof(aKey),
  (BYTE *)aKey, 0 } } },
};

static void test_encodePublicKeyInfo(DWORD dwEncoding)
{
    DWORD i;

    for (i = 0; i < sizeof(pubKeys) / sizeof(pubKeys[0]); i++)
    {
        BOOL ret;
        BYTE *buf = NULL;
        DWORD bufSize = 0;

        ret = CryptEncodeObjectEx(dwEncoding, X509_PUBLIC_KEY_INFO,
         &pubKeys[i].info, CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf,
         &bufSize);
        ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            ok(bufSize == pubKeys[i].encoded[1] + 2 ||
             bufSize == pubKeys[i].encodedNoNull[1] + 2,
             "Expected %d or %d bytes, got %ld\n", pubKeys[i].encoded[1] + 2,
             pubKeys[i].encodedNoNull[1] + 2, bufSize);
            if (bufSize == pubKeys[i].encoded[1] + 2)
                ok(!memcmp(buf, pubKeys[i].encoded, pubKeys[i].encoded[1] + 2),
                 "Unexpected value\n");
            else if (bufSize == pubKeys[i].encodedNoNull[1] + 2)
                ok(!memcmp(buf, pubKeys[i].encodedNoNull,
                 pubKeys[i].encodedNoNull[1] + 2), "Unexpected value\n");
            LocalFree(buf);
        }
    }
}

static void comparePublicKeyInfo(const CERT_PUBLIC_KEY_INFO *expected,
 const CERT_PUBLIC_KEY_INFO *got)
{
    ok(!strcmp(expected->Algorithm.pszObjId, got->Algorithm.pszObjId),
     "Expected OID %s, got %s\n", expected->Algorithm.pszObjId,
     got->Algorithm.pszObjId);
    ok(expected->Algorithm.Parameters.cbData ==
     got->Algorithm.Parameters.cbData,
     "Expected parameters of %ld bytes, got %ld\n",
     expected->Algorithm.Parameters.cbData, got->Algorithm.Parameters.cbData);
    if (expected->Algorithm.Parameters.cbData)
        ok(!memcmp(expected->Algorithm.Parameters.pbData,
         got->Algorithm.Parameters.pbData, got->Algorithm.Parameters.cbData),
         "Unexpected algorithm parameters\n");
    ok(expected->PublicKey.cbData == got->PublicKey.cbData,
     "Expected public key of %ld bytes, got %ld\n",
     expected->PublicKey.cbData, got->PublicKey.cbData);
    if (expected->PublicKey.cbData)
        ok(!memcmp(expected->PublicKey.pbData, got->PublicKey.pbData,
         got->PublicKey.cbData), "Unexpected public key value\n");
}

static void test_decodePublicKeyInfo(DWORD dwEncoding)
{
    static const BYTE bogusPubKeyInfo[] = { 0x30, 0x22, 0x30, 0x0d, 0x06, 0x06,
     0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03,
     0x11, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
     0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    DWORD i;
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    for (i = 0; i < sizeof(pubKeys) / sizeof(pubKeys[0]); i++)
    {
        /* The NULL form decodes to the decoded member */
        ret = CryptDecodeObjectEx(dwEncoding, X509_PUBLIC_KEY_INFO,
         pubKeys[i].encoded, pubKeys[i].encoded[1] + 2, CRYPT_DECODE_ALLOC_FLAG,
         NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            comparePublicKeyInfo(&pubKeys[i].decoded,
             (CERT_PUBLIC_KEY_INFO *)buf);
            LocalFree(buf);
        }
        /* The non-NULL form decodes to the original */
        ret = CryptDecodeObjectEx(dwEncoding, X509_PUBLIC_KEY_INFO,
         pubKeys[i].encodedNoNull, pubKeys[i].encodedNoNull[1] + 2,
         CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
        ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
        if (buf)
        {
            comparePublicKeyInfo(&pubKeys[i].info, (CERT_PUBLIC_KEY_INFO *)buf);
            LocalFree(buf);
        }
    }
    /* Test with bogus (not valid DER) parameters */
    ret = CryptDecodeObjectEx(dwEncoding, X509_PUBLIC_KEY_INFO,
     bogusPubKeyInfo, bogusPubKeyInfo[1] + 2, CRYPT_DECODE_ALLOC_FLAG,
     NULL, (BYTE *)&buf, &bufSize);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_CORRUPT,
     "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
}

static const BYTE v1Cert[] = { 0x30, 0x33, 0x02, 0x00, 0x30, 0x02, 0x06, 0x00,
 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x07, 0x30,
 0x02, 0x06, 0x00, 0x03, 0x01, 0x00 };
static const BYTE v2Cert[] = { 0x30, 0x38, 0xa0, 0x03, 0x02, 0x01, 0x01, 0x02,
 0x00, 0x30, 0x02, 0x06, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00 };
static const BYTE v3Cert[] = { 0x30, 0x38, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02,
 0x00, 0x30, 0x02, 0x06, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00 };
static const BYTE v1CertWithConstraints[] = { 0x30, 0x4b, 0x02, 0x00, 0x30,
 0x02, 0x06, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a,
 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14,
 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30,
 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };
static const BYTE v1CertWithSerial[] = { 0x30, 0x4c, 0x02, 0x01, 0x01, 0x30,
 0x02, 0x06, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a,
 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14,
 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30,
 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };
static const BYTE bigCert[] = { 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22,
 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30,
 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20,
 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01,
 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01,
 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };

static const BYTE serialNum[] = { 0x01 };

static void test_encodeCertToBeSigned(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CERT_INFO info = { 0 };

    /* Test with NULL pvStructInfo */
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, NULL,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    /* Test with a V1 cert */
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == v1Cert[1] + 2, "Expected size %d, got %ld\n",
         v1Cert[1] + 2, size);
        ok(!memcmp(buf, v1Cert, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v2 cert */
    info.dwVersion = CERT_V2;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v2Cert), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v2Cert, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v3 cert */
    info.dwVersion = CERT_V3;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v3Cert), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v3Cert, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* see if a V1 cert can have basic constraints set (RFC3280 says no, but
     * API doesn't prevent it)
     */
    info.dwVersion = CERT_V1;
    info.cExtension = 1;
    info.rgExtension = &criticalExt;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v1CertWithConstraints), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v1CertWithConstraints, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* test v1 cert with a serial number */
    info.SerialNumber.cbData = sizeof(serialNum);
    info.SerialNumber.pbData = (BYTE *)serialNum;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(v1CertWithSerial), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v1CertWithSerial, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v1 cert with an issuer name, a subject name, and a serial number */
    info.Issuer.cbData = sizeof(encodedCommonName);
    info.Issuer.pbData = (BYTE *)encodedCommonName;
    info.Subject.cbData = sizeof(encodedCommonName);
    info.Subject.pbData = (BYTE *)encodedCommonName;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(bigCert), "Wrong size %ld\n", size);
        ok(!memcmp(buf, bigCert, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* for now, I let more interesting tests be done for each subcomponent,
     * rather than retesting them all here.
     */
}

static void test_decodeCertToBeSigned(DWORD dwEncoding)
{
    static const BYTE *corruptCerts[] = { v1Cert, v2Cert, v3Cert,
     v1CertWithConstraints, v1CertWithSerial };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0, i;

    /* Test with NULL pbEncoded */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, NULL, 0,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, NULL, 1,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    /* The following certs all fail with CRYPT_E_ASN1_CORRUPT, because at a
     * minimum a cert must have a non-zero serial number, an issuer, and a
     * subject.
     */
    for (i = 0; i < sizeof(corruptCerts) / sizeof(corruptCerts[0]); i++)
    {
        ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED,
         corruptCerts[i], corruptCerts[i][1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
         (BYTE *)&buf, &size);
        ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT),
         "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
    }
    /* Now check with serial number, subject and issuer specified */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, bigCert,
     sizeof(bigCert), CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_INFO *info = (CERT_INFO *)buf;

        ok(size >= sizeof(CERT_INFO), "Wrong size %ld\n", size);
        ok(info->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %ld\n", info->SerialNumber.cbData);
        ok(*info->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *info->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong size %ld\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        ok(info->Subject.cbData == sizeof(encodedCommonName),
         "Wrong size %ld\n", info->Subject.cbData);
        ok(!memcmp(info->Subject.pbData, encodedCommonName,
         info->Subject.cbData), "Unexpected subject\n");
        LocalFree(buf);
    }
}

static const BYTE hash[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd,
 0xe, 0xf };

static const BYTE signedBigCert[] = {
 0x30, 0x81, 0x93, 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06, 0x00, 0x30,
 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a,
 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06,
 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61,
 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3,
 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07,
 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

static void test_encodeCert(DWORD dwEncoding)
{
    /* Note the SignatureAlgorithm must match that in the encoded cert.  Note
     * also that bigCert is a NULL-terminated string, so don't count its
     * last byte (otherwise the signed cert won't decode.)
     */
    CERT_SIGNED_CONTENT_INFO info = { { sizeof(bigCert), (BYTE *)bigCert },
     { NULL, { 0, NULL } }, { sizeof(hash), (BYTE *)hash, 0 } };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD bufSize = 0;

    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &bufSize);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(bufSize == sizeof(signedBigCert), "Wrong size %ld\n", bufSize);
        ok(!memcmp(buf, signedBigCert, bufSize), "Unexpected cert\n");
        LocalFree(buf);
    }
}

static void test_decodeCert(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;

    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT, signedBigCert,
     sizeof(signedBigCert), CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_SIGNED_CONTENT_INFO *info = (CERT_SIGNED_CONTENT_INFO *)buf;

        ok(info->ToBeSigned.cbData == sizeof(bigCert),
         "Wrong cert size %ld\n", info->ToBeSigned.cbData);
        ok(!memcmp(info->ToBeSigned.pbData, bigCert, info->ToBeSigned.cbData),
         "Unexpected cert\n");
        ok(info->Signature.cbData == sizeof(hash),
         "Wrong signature size %ld\n", info->Signature.cbData);
        ok(!memcmp(info->Signature.pbData, hash, info->Signature.cbData),
         "Unexpected signature\n");
        LocalFree(buf);
    }
    /* A signed cert decodes as a CERT_INFO too */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_TO_BE_SIGNED, signedBigCert,
     sizeof(signedBigCert), CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_INFO *info = (CERT_INFO *)buf;

        ok(size >= sizeof(CERT_INFO), "Wrong size %ld\n", size);
        ok(info->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %ld\n", info->SerialNumber.cbData);
        ok(*info->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *info->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong size %ld\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        ok(info->Subject.cbData == sizeof(encodedCommonName),
         "Wrong size %ld\n", info->Subject.cbData);
        ok(!memcmp(info->Subject.pbData, encodedCommonName,
         info->Subject.cbData), "Unexpected subject\n");
        LocalFree(buf);
    }
}

static const BYTE emptyDistPoint[] = { 0x30, 0x02, 0x30, 0x00 };
static const BYTE distPointWithUrl[] = { 0x30, 0x19, 0x30, 0x17, 0xa0, 0x15,
 0xa0, 0x13, 0x86, 0x11, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x69,
 0x6e, 0x65, 0x68, 0x71, 0x2e, 0x6f, 0x72, 0x67 };
static const BYTE distPointWithReason[] = { 0x30, 0x06, 0x30, 0x04, 0x81, 0x02,
 0x00, 0x03 };
static const BYTE distPointWithIssuer[] = { 0x30, 0x17, 0x30, 0x15, 0xa2, 0x13,
 0x86, 0x11, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x69, 0x6e, 0x65,
 0x68, 0x71, 0x2e, 0x6f, 0x72, 0x67 };
static const BYTE distPointWithUrlAndIssuer[] = { 0x30, 0x2e, 0x30, 0x2c, 0xa0,
 0x15, 0xa0, 0x13, 0x86, 0x11, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77,
 0x69, 0x6e, 0x65, 0x68, 0x71, 0x2e, 0x6f, 0x72, 0x67, 0xa2, 0x13, 0x86, 0x11,
 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x69, 0x6e, 0x65, 0x68, 0x71,
 0x2e, 0x6f, 0x72, 0x67 };
static const BYTE crlReason = CRL_REASON_KEY_COMPROMISE |
 CRL_REASON_AFFILIATION_CHANGED;

static void test_encodeCRLDistPoints(DWORD dwEncoding)
{
    CRL_DIST_POINTS_INFO info = { 0 };
    CRL_DIST_POINT point = { { 0 } };
    CERT_ALT_NAME_ENTRY entry = { 0 };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;

    /* Test with an empty info */
    ret = CryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Test with one empty dist point */
    info.cDistPoint = 1;
    info.rgDistPoint = &point;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(emptyDistPoint), "Wrong size %ld\n", size);
        ok(!memcmp(buf, emptyDistPoint, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* A dist point with an invalid name */
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
    entry.dwAltNameChoice = CERT_ALT_NAME_URL;
    U(entry).pwszURL = (LPWSTR)nihongoURL;
    U(point.DistPointName).FullName.cAltEntry = 1;
    U(point.DistPointName).FullName.rgAltEntry = &entry;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_IA5_STRING,
     "Expected CRYPT_E_INVALID_IA5_STRING, got %08lx\n", GetLastError());
    /* The first invalid character is at index 7 */
    ok(GET_CERT_ALT_NAME_VALUE_ERR_INDEX(size) == 7,
     "Expected invalid char at index 7, got %ld\n",
     GET_CERT_ALT_NAME_VALUE_ERR_INDEX(size));
    /* A dist point with (just) a valid name */
    U(entry).pwszURL = (LPWSTR)url;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(distPointWithUrl), "Wrong size %ld\n", size);
        ok(!memcmp(buf, distPointWithUrl, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* A dist point with (just) reason flags */
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_NO_NAME;
    point.ReasonFlags.cbData = sizeof(crlReason);
    point.ReasonFlags.pbData = (LPBYTE)&crlReason;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(distPointWithReason), "Wrong size %ld\n", size);
        ok(!memcmp(buf, distPointWithReason, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* A dist point with just an issuer */
    point.ReasonFlags.cbData = 0;
    point.CRLIssuer.cAltEntry = 1;
    point.CRLIssuer.rgAltEntry = &entry;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(distPointWithIssuer), "Wrong size %ld\n", size);
        ok(!memcmp(buf, distPointWithIssuer, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* A dist point with both a name and an issuer */
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(distPointWithUrlAndIssuer),
         "Wrong size %ld\n", size);
        ok(!memcmp(buf, distPointWithUrlAndIssuer, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeCRLDistPoints(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    PCRL_DIST_POINTS_INFO info;
    PCRL_DIST_POINT point;
    PCERT_ALT_NAME_ENTRY entry;

    ret = CryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     emptyDistPoint, emptyDistPoint[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    if (ret)
    {
        info = (PCRL_DIST_POINTS_INFO)buf;
        ok(size >= sizeof(CRL_DIST_POINTS_INFO) + sizeof(CRL_DIST_POINT),
         "Wrong size %ld\n", size);
        ok(info->cDistPoint == 1, "Expected 1 dist points, got %ld\n",
         info->cDistPoint);
        point = info->rgDistPoint;
        ok(point->DistPointName.dwDistPointNameChoice == CRL_DIST_POINT_NO_NAME,
         "Expected CRL_DIST_POINT_NO_NAME, got %ld\n",
         point->DistPointName.dwDistPointNameChoice);
        ok(point->ReasonFlags.cbData == 0, "Expected no reason\n");
        ok(point->CRLIssuer.cAltEntry == 0, "Expected no issuer\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     distPointWithUrl, distPointWithUrl[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    if (ret)
    {
        info = (PCRL_DIST_POINTS_INFO)buf;
        ok(size >= sizeof(CRL_DIST_POINTS_INFO) + sizeof(CRL_DIST_POINT),
         "Wrong size %ld\n", size);
        ok(info->cDistPoint == 1, "Expected 1 dist points, got %ld\n",
         info->cDistPoint);
        point = info->rgDistPoint;
        ok(point->DistPointName.dwDistPointNameChoice ==
         CRL_DIST_POINT_FULL_NAME,
         "Expected CRL_DIST_POINT_FULL_NAME, got %ld\n",
         point->DistPointName.dwDistPointNameChoice);
        ok(U(point->DistPointName).FullName.cAltEntry == 1,
         "Expected 1 name entry, got %ld\n",
         U(point->DistPointName).FullName.cAltEntry);
        entry = U(point->DistPointName).FullName.rgAltEntry;
        ok(entry->dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %ld\n", entry->dwAltNameChoice);
        ok(!lstrcmpW(U(*entry).pwszURL, url), "Unexpected name\n");
        ok(point->ReasonFlags.cbData == 0, "Expected no reason\n");
        ok(point->CRLIssuer.cAltEntry == 0, "Expected no issuer\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     distPointWithReason, distPointWithReason[1] + 2, CRYPT_DECODE_ALLOC_FLAG,
     NULL, (BYTE *)&buf, &size);
    if (ret)
    {
        info = (PCRL_DIST_POINTS_INFO)buf;
        ok(size >= sizeof(CRL_DIST_POINTS_INFO) + sizeof(CRL_DIST_POINT),
         "Wrong size %ld\n", size);
        ok(info->cDistPoint == 1, "Expected 1 dist points, got %ld\n",
         info->cDistPoint);
        point = info->rgDistPoint;
        ok(point->DistPointName.dwDistPointNameChoice ==
         CRL_DIST_POINT_NO_NAME,
         "Expected CRL_DIST_POINT_NO_NAME, got %ld\n",
         point->DistPointName.dwDistPointNameChoice);
        ok(point->ReasonFlags.cbData == sizeof(crlReason),
         "Expected reason length\n");
        ok(!memcmp(point->ReasonFlags.pbData, &crlReason, sizeof(crlReason)),
         "Unexpected reason\n");
        ok(point->CRLIssuer.cAltEntry == 0, "Expected no issuer\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_CRL_DIST_POINTS,
     distPointWithUrlAndIssuer, distPointWithUrlAndIssuer[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (ret)
    {
        info = (PCRL_DIST_POINTS_INFO)buf;
        ok(size >= sizeof(CRL_DIST_POINTS_INFO) + sizeof(CRL_DIST_POINT),
         "Wrong size %ld\n", size);
        ok(info->cDistPoint == 1, "Expected 1 dist points, got %ld\n",
         info->cDistPoint);
        point = info->rgDistPoint;
        ok(point->DistPointName.dwDistPointNameChoice ==
         CRL_DIST_POINT_FULL_NAME,
         "Expected CRL_DIST_POINT_FULL_NAME, got %ld\n",
         point->DistPointName.dwDistPointNameChoice);
        ok(U(point->DistPointName).FullName.cAltEntry == 1,
         "Expected 1 name entry, got %ld\n",
         U(point->DistPointName).FullName.cAltEntry);
        entry = U(point->DistPointName).FullName.rgAltEntry;
        ok(entry->dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %ld\n", entry->dwAltNameChoice);
        ok(!lstrcmpW(U(*entry).pwszURL, url), "Unexpected name\n");
        ok(point->ReasonFlags.cbData == 0, "Expected no reason\n");
        ok(point->CRLIssuer.cAltEntry == 1,
         "Expected 1 issuer entry, got %ld\n", point->CRLIssuer.cAltEntry);
        entry = point->CRLIssuer.rgAltEntry;
        ok(entry->dwAltNameChoice == CERT_ALT_NAME_URL,
         "Expected CERT_ALT_NAME_URL, got %ld\n", entry->dwAltNameChoice);
        ok(!lstrcmpW(U(*entry).pwszURL, url), "Unexpected name\n");
        LocalFree(buf);
    }
}

static const BYTE badFlagsIDP[] = { 0x30,0x06,0x81,0x01,0xff,0x82,0x01,0xff };
static const BYTE emptyNameIDP[] = { 0x30,0x04,0xa0,0x02,0xa0,0x00 };
static const BYTE urlIDP[] = { 0x30,0x17,0xa0,0x15,0xa0,0x13,0x86,0x11,0x68,
 0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,0x2e,0x6f,0x72,
 0x67 };

static void test_encodeCRLIssuingDistPoint(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CRL_ISSUING_DIST_POINT point = { { 0 } };
    CERT_ALT_NAME_ENTRY entry;

    ret = CryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, NULL,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    ret = CryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(emptySequence), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, emptySequence, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* nonsensical flags */
    point.fOnlyContainsUserCerts = TRUE;
    point.fOnlyContainsCACerts = TRUE;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(badFlagsIDP), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, badFlagsIDP, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* unimplemented name type */
    point.fOnlyContainsCACerts = point.fOnlyContainsUserCerts = FALSE;
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_ISSUER_RDN_NAME;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* empty name */
    point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
    U(point.DistPointName).FullName.cAltEntry = 0;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(emptyNameIDP), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, emptyNameIDP, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* name with URL entry */
    entry.dwAltNameChoice = CERT_ALT_NAME_URL;
    U(entry).pwszURL = (LPWSTR)url;
    U(point.DistPointName).FullName.cAltEntry = 1;
    U(point.DistPointName).FullName.rgAltEntry = &entry;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT, &point,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(urlIDP), "Unexpected size %ld\n", size);
        ok(!memcmp(buf, urlIDP, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static void compareAltNameEntry(const CERT_ALT_NAME_ENTRY *expected,
 const CERT_ALT_NAME_ENTRY *got)
{
    ok(expected->dwAltNameChoice == got->dwAltNameChoice,
     "Expected name choice %ld, got %ld\n", expected->dwAltNameChoice,
     got->dwAltNameChoice);
    if (expected->dwAltNameChoice == got->dwAltNameChoice)
    {
        switch (got->dwAltNameChoice)
        {
        case CERT_ALT_NAME_RFC822_NAME:
        case CERT_ALT_NAME_DNS_NAME:
        case CERT_ALT_NAME_EDI_PARTY_NAME:
        case CERT_ALT_NAME_URL:
        case CERT_ALT_NAME_REGISTERED_ID:
            ok((!U(*expected).pwszURL && !U(*got).pwszURL) ||
               !lstrcmpW(U(*expected).pwszURL, U(*got).pwszURL), "Unexpected name\n");
            break;
        case CERT_ALT_NAME_X400_ADDRESS:
        case CERT_ALT_NAME_DIRECTORY_NAME:
        case CERT_ALT_NAME_IP_ADDRESS:
            ok(U(*got).IPAddress.cbData == U(*expected).IPAddress.cbData,
               "Unexpected IP address length %ld\n", U(*got).IPAddress.cbData);
            ok(!memcmp(U(*got).IPAddress.pbData, U(*got).IPAddress.pbData,
                       U(*got).IPAddress.cbData), "Unexpected value\n");
            break;
        }
    }
}

static void compareAltNameInfo(const CERT_ALT_NAME_INFO *expected,
 const CERT_ALT_NAME_INFO *got)
{
    DWORD i;

    ok(expected->cAltEntry == got->cAltEntry, "Expected %ld entries, got %ld\n",
     expected->cAltEntry, got->cAltEntry);
    for (i = 0; i < min(expected->cAltEntry, got->cAltEntry); i++)
        compareAltNameEntry(&expected->rgAltEntry[i], &got->rgAltEntry[i]);
}

static void compareDistPointName(const CRL_DIST_POINT_NAME *expected,
 const CRL_DIST_POINT_NAME *got)
{
    ok(got->dwDistPointNameChoice == expected->dwDistPointNameChoice,
     "Unexpected name choice %ld\n", got->dwDistPointNameChoice);
    if (got->dwDistPointNameChoice == CRL_DIST_POINT_FULL_NAME)
        compareAltNameInfo(&(U(*expected).FullName), &(U(*got).FullName));
}

static void compareCRLIssuingDistPoints(const CRL_ISSUING_DIST_POINT *expected,
 const CRL_ISSUING_DIST_POINT *got)
{
    compareDistPointName(&expected->DistPointName, &got->DistPointName);
    ok(got->fOnlyContainsUserCerts == expected->fOnlyContainsUserCerts,
     "Unexpected fOnlyContainsUserCerts\n");
    ok(got->fOnlyContainsCACerts == expected->fOnlyContainsCACerts,
     "Unexpected fOnlyContainsCACerts\n");
    ok(got->OnlySomeReasonFlags.cbData == expected->OnlySomeReasonFlags.cbData,
     "Unexpected reason flags\n");
    ok(got->fIndirectCRL == expected->fIndirectCRL,
     "Unexpected fIndirectCRL\n");
}

static void test_decodeCRLIssuingDistPoint(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CRL_ISSUING_DIST_POINT point = { { 0 } };

    ret = CryptDecodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT,
     emptySequence, emptySequence[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        compareCRLIssuingDistPoints(&point, (PCRL_ISSUING_DIST_POINT)buf);
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT,
     badFlagsIDP, badFlagsIDP[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        point.fOnlyContainsUserCerts = point.fOnlyContainsCACerts = TRUE;
        compareCRLIssuingDistPoints(&point, (PCRL_ISSUING_DIST_POINT)buf);
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT,
     emptyNameIDP, emptyNameIDP[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        point.fOnlyContainsCACerts = point.fOnlyContainsUserCerts = FALSE;
        point.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;
        U(point.DistPointName).FullName.cAltEntry = 0;
        compareCRLIssuingDistPoints(&point, (PCRL_ISSUING_DIST_POINT)buf);
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_ISSUING_DIST_POINT,
     urlIDP, urlIDP[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (ret)
    {
        CERT_ALT_NAME_ENTRY entry;

        entry.dwAltNameChoice = CERT_ALT_NAME_URL;
        U(entry).pwszURL = (LPWSTR)url;
        U(point.DistPointName).FullName.cAltEntry = 1;
        U(point.DistPointName).FullName.rgAltEntry = &entry;
        compareCRLIssuingDistPoints(&point, (PCRL_ISSUING_DIST_POINT)buf);
        LocalFree(buf);
    }
}

static const BYTE v1CRL[] = { 0x30, 0x15, 0x30, 0x02, 0x06, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a };
static const BYTE v2CRL[] = { 0x30, 0x18, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE v1CRLWithIssuer[] = { 0x30, 0x2c, 0x30, 0x02, 0x06, 0x00,
 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a,
 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f, 0x31,
 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x5a };
static const BYTE v1CRLWithIssuerAndEmptyEntry[] = { 0x30, 0x43, 0x30, 0x02,
 0x06, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03,
 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18,
 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x5a, 0x30, 0x15, 0x30, 0x13, 0x02, 0x00, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE v1CRLWithIssuerAndEntry[] = { 0x30, 0x44, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x16, 0x30, 0x14, 0x02, 0x01, 0x01, 0x18, 0x0f, 0x31, 0x36,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE v1CRLWithEntryExt[] = { 0x30,0x5a,0x30,0x02,0x06,0x00,0x30,
 0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
 0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
 0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x2c,0x30,0x2a,0x02,0x01,
 0x01,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,
 0x30,0x30,0x5a,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,
 0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE v1CRLWithExt[] = { 0x30,0x5c,0x30,0x02,0x06,0x00,0x30,0x15,
 0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
 0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,
 0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x16,0x30,0x14,0x02,0x01,0x01,
 0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,
 0x30,0x5a,0xa0,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,
 0xff,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE v2CRLWithExt[] = { 0x30,0x5c,0x02,0x01,0x01,0x30,0x02,0x06,
 0x00,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,
 0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,
 0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x16,0x30,0x14,
 0x02,0x01,0x01,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,
 0x30,0x30,0x30,0x30,0x5a,0xa0,0x13,0x30,0x11,0x30,0x0f,0x06,0x03,0x55,0x1d,
 0x13,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };
static const BYTE v2CRLWithIssuingDistPoint[] = { 0x30,0x5c,0x02,0x01,0x01,
 0x30,0x02,0x06,0x00,0x30,0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,
 0x13,0x0a,0x4a,0x75,0x61,0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,
 0x36,0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,
 0x16,0x30,0x14,0x02,0x01,0x01,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,0x30,
 0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0xa0,0x13,0x30,0x11,0x30,0x0f,0x06,
 0x03,0x55,0x1d,0x13,0x04,0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01 };

static void test_encodeCRLToBeSigned(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    static CHAR oid_issuing_dist_point[] = szOID_ISSUING_DIST_POINT;
    DWORD size = 0;
    CRL_INFO info = { 0 };
    CRL_ENTRY entry = { { 0 }, { 0 }, 0, 0 };
    CERT_EXTENSION ext;

    /* Test with a V1 CRL */
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v1CRL), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v1CRL, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test v2 CRL */
    info.dwVersion = CRL_V2;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == v2CRL[1] + 2, "Expected size %d, got %ld\n",
         v2CRL[1] + 2, size);
        ok(!memcmp(buf, v2CRL, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* v1 CRL with a name */
    info.dwVersion = CRL_V1;
    info.Issuer.cbData = sizeof(encodedCommonName);
    info.Issuer.pbData = (BYTE *)encodedCommonName;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v1CRLWithIssuer), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v1CRLWithIssuer, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* v1 CRL with a name and a NULL entry pointer */
    info.cCRLEntry = 1;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    /* now set an empty entry */
    info.rgCRLEntry = &entry;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(v1CRLWithIssuerAndEmptyEntry),
         "Wrong size %ld\n", size);
        ok(!memcmp(buf, v1CRLWithIssuerAndEmptyEntry, size),
         "Got unexpected value\n");
        LocalFree(buf);
    }
    /* an entry with a serial number */
    entry.SerialNumber.cbData = sizeof(serialNum);
    entry.SerialNumber.pbData = (BYTE *)serialNum;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    if (buf)
    {
        ok(size == sizeof(v1CRLWithIssuerAndEntry),
         "Wrong size %ld\n", size);
        ok(!memcmp(buf, v1CRLWithIssuerAndEntry, size),
         "Got unexpected value\n");
        LocalFree(buf);
    }
    /* an entry with an extension */
    entry.cExtension = 1;
    entry.rgExtension = &criticalExt;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v1CRLWithEntryExt), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v1CRLWithEntryExt, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* a CRL with an extension */
    entry.cExtension = 0;
    info.cExtension = 1;
    info.rgExtension = &criticalExt;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v1CRLWithExt), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v1CRLWithExt, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* a v2 CRL with an extension, this time non-critical */
    info.dwVersion = CRL_V2;
    info.rgExtension = &nonCriticalExt;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v2CRLWithExt), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v2CRLWithExt, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* a v2 CRL with an issuing dist point extension */
    ext.pszObjId = oid_issuing_dist_point;
    ext.fCritical = TRUE;
    ext.Value.cbData = sizeof(urlIDP);
    ext.Value.pbData = (LPBYTE)urlIDP;
    entry.rgExtension = &ext;
    ret = CryptEncodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(v2CRLWithIssuingDistPoint), "Wrong size %ld\n", size);
        ok(!memcmp(buf, v2CRLWithIssuingDistPoint, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static const BYTE verisignCRL[] = { 0x30, 0x82, 0x01, 0xb1, 0x30, 0x82, 0x01,
 0x1a, 0x02, 0x01, 0x01, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x02, 0x05, 0x00, 0x30, 0x61, 0x31, 0x11, 0x30, 0x0f, 0x06,
 0x03, 0x55, 0x04, 0x07, 0x13, 0x08, 0x49, 0x6e, 0x74, 0x65, 0x72, 0x6e, 0x65,
 0x74, 0x31, 0x17, 0x30, 0x15, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0e, 0x56,
 0x65, 0x72, 0x69, 0x53, 0x69, 0x67, 0x6e, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e,
 0x31, 0x33, 0x30, 0x31, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x2a, 0x56, 0x65,
 0x72, 0x69, 0x53, 0x69, 0x67, 0x6e, 0x20, 0x43, 0x6f, 0x6d, 0x6d, 0x65, 0x72,
 0x63, 0x69, 0x61, 0x6c, 0x20, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65,
 0x20, 0x50, 0x75, 0x62, 0x6c, 0x69, 0x73, 0x68, 0x65, 0x72, 0x73, 0x20, 0x43,
 0x41, 0x17, 0x0d, 0x30, 0x31, 0x30, 0x33, 0x32, 0x34, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x30, 0x34, 0x30, 0x31, 0x30, 0x37, 0x32, 0x33,
 0x35, 0x39, 0x35, 0x39, 0x5a, 0x30, 0x69, 0x30, 0x21, 0x02, 0x10, 0x1b, 0x51,
 0x90, 0xf7, 0x37, 0x24, 0x39, 0x9c, 0x92, 0x54, 0xcd, 0x42, 0x46, 0x37, 0x99,
 0x6a, 0x17, 0x0d, 0x30, 0x31, 0x30, 0x31, 0x33, 0x30, 0x30, 0x30, 0x30, 0x31,
 0x32, 0x34, 0x5a, 0x30, 0x21, 0x02, 0x10, 0x75, 0x0e, 0x40, 0xff, 0x97, 0xf0,
 0x47, 0xed, 0xf5, 0x56, 0xc7, 0x08, 0x4e, 0xb1, 0xab, 0xfd, 0x17, 0x0d, 0x30,
 0x31, 0x30, 0x31, 0x33, 0x31, 0x30, 0x30, 0x30, 0x30, 0x34, 0x39, 0x5a, 0x30,
 0x21, 0x02, 0x10, 0x77, 0xe6, 0x5a, 0x43, 0x59, 0x93, 0x5d, 0x5f, 0x7a, 0x75,
 0x80, 0x1a, 0xcd, 0xad, 0xc2, 0x22, 0x17, 0x0d, 0x30, 0x30, 0x30, 0x38, 0x33,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x35, 0x36, 0x5a, 0xa0, 0x1a, 0x30, 0x18, 0x30,
 0x09, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0b, 0x06,
 0x03, 0x55, 0x1d, 0x0f, 0x04, 0x04, 0x03, 0x02, 0x05, 0xa0, 0x30, 0x0d, 0x06,
 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x02, 0x05, 0x00, 0x03,
 0x81, 0x81, 0x00, 0x18, 0x2c, 0xe8, 0xfc, 0x16, 0x6d, 0x91, 0x4a, 0x3d, 0x88,
 0x54, 0x48, 0x5d, 0xb8, 0x11, 0xbf, 0x64, 0xbb, 0xf9, 0xda, 0x59, 0x19, 0xdd,
 0x0e, 0x65, 0xab, 0xc0, 0x0c, 0xfa, 0x67, 0x7e, 0x21, 0x1e, 0x83, 0x0e, 0xcf,
 0x9b, 0x89, 0x8a, 0xcf, 0x0c, 0x4b, 0xc1, 0x39, 0x9d, 0xe7, 0x6a, 0xac, 0x46,
 0x74, 0x6a, 0x91, 0x62, 0x22, 0x0d, 0xc4, 0x08, 0xbd, 0xf5, 0x0a, 0x90, 0x7f,
 0x06, 0x21, 0x3d, 0x7e, 0xa7, 0xaa, 0x5e, 0xcd, 0x22, 0x15, 0xe6, 0x0c, 0x75,
 0x8e, 0x6e, 0xad, 0xf1, 0x84, 0xe4, 0x22, 0xb4, 0x30, 0x6f, 0xfb, 0x64, 0x8f,
 0xd7, 0x80, 0x43, 0xf5, 0x19, 0x18, 0x66, 0x1d, 0x72, 0xa3, 0xe3, 0x94, 0x82,
 0x28, 0x52, 0xa0, 0x06, 0x4e, 0xb1, 0xc8, 0x92, 0x0c, 0x97, 0xbe, 0x15, 0x07,
 0xab, 0x7a, 0xc9, 0xea, 0x08, 0x67, 0x43, 0x4d, 0x51, 0x63, 0x3b, 0x9c, 0x9c,
 0xcd };

static void test_decodeCRLToBeSigned(DWORD dwEncoding)
{
    static const BYTE *corruptCRLs[] = { v1CRL, v2CRL };
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0, i;

    for (i = 0; i < sizeof(corruptCRLs) / sizeof(corruptCRLs[0]); i++)
    {
        ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
         corruptCRLs[i], corruptCRLs[i][1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
         (BYTE *)&buf, &size);
        ok(!ret && (GetLastError() == CRYPT_E_ASN1_CORRUPT),
         "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
    }
    /* at a minimum, a CRL must contain an issuer: */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v1CRLWithIssuer, v1CRLWithIssuer[1] + 2, CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRL_INFO *info = (CRL_INFO *)buf;

        ok(size >= sizeof(CRL_INFO), "Wrong size %ld\n", size);
        ok(info->cCRLEntry == 0, "Expected 0 CRL entries, got %ld\n",
         info->cCRLEntry);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong issuer size %ld\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        LocalFree(buf);
    }
    /* check decoding with an empty CRL entry */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v1CRLWithIssuerAndEmptyEntry, v1CRLWithIssuerAndEmptyEntry[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    todo_wine ok(!ret && GetLastError() == CRYPT_E_ASN1_CORRUPT,
     "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
    /* with a real CRL entry */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v1CRLWithIssuerAndEntry, v1CRLWithIssuerAndEntry[1] + 2,
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRL_INFO *info = (CRL_INFO *)buf;
        CRL_ENTRY *entry;

        ok(size >= sizeof(CRL_INFO), "Wrong size %ld\n", size);
        ok(info->cCRLEntry == 1, "Expected 1 CRL entries, got %ld\n",
         info->cCRLEntry);
        ok(info->rgCRLEntry != NULL, "Expected a valid CRL entry array\n");
        entry = info->rgCRLEntry;
        ok(entry->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %ld\n",
         entry->SerialNumber.cbData);
        ok(*entry->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *entry->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong issuer size %ld\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
    }
    /* a real CRL from verisign that has extensions */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     verisignCRL, sizeof(verisignCRL), CRYPT_DECODE_ALLOC_FLAG,
     NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRL_INFO *info = (CRL_INFO *)buf;
        CRL_ENTRY *entry;

        ok(size >= sizeof(CRL_INFO), "Wrong size %ld\n", size);
        ok(info->cCRLEntry == 3, "Expected 3 CRL entries, got %ld\n",
         info->cCRLEntry);
        ok(info->rgCRLEntry != NULL, "Expected a valid CRL entry array\n");
        entry = info->rgCRLEntry;
        ok(info->cExtension == 2, "Expected 2 extensions, got %ld\n",
         info->cExtension);
    }
    /* and finally, with an extension */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v1CRLWithExt, sizeof(v1CRLWithExt), CRYPT_DECODE_ALLOC_FLAG,
     NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRL_INFO *info = (CRL_INFO *)buf;
        CRL_ENTRY *entry;

        ok(size >= sizeof(CRL_INFO), "Wrong size %ld\n", size);
        ok(info->cCRLEntry == 1, "Expected 1 CRL entries, got %ld\n",
         info->cCRLEntry);
        ok(info->rgCRLEntry != NULL, "Expected a valid CRL entry array\n");
        entry = info->rgCRLEntry;
        ok(entry->SerialNumber.cbData == 1,
         "Expected serial number size 1, got %ld\n",
         entry->SerialNumber.cbData);
        ok(*entry->SerialNumber.pbData == *serialNum,
         "Expected serial number %d, got %d\n", *serialNum,
         *entry->SerialNumber.pbData);
        ok(info->Issuer.cbData == sizeof(encodedCommonName),
         "Wrong issuer size %ld\n", info->Issuer.cbData);
        ok(!memcmp(info->Issuer.pbData, encodedCommonName, info->Issuer.cbData),
         "Unexpected issuer\n");
        ok(info->cExtension == 1, "Expected 1 extensions, got %ld\n",
         info->cExtension);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v2CRLWithExt, sizeof(v2CRLWithExt), CRYPT_DECODE_ALLOC_FLAG,
     NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRL_INFO *info = (CRL_INFO *)buf;

        ok(info->cExtension == 1, "Expected 1 extensions, got %ld\n",
         info->cExtension);
        LocalFree(buf);
    }
    /* And again, with an issuing dist point */
    ret = CryptDecodeObjectEx(dwEncoding, X509_CERT_CRL_TO_BE_SIGNED,
     v2CRLWithIssuingDistPoint, sizeof(v2CRLWithIssuingDistPoint),
     CRYPT_DECODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CRL_INFO *info = (CRL_INFO *)buf;

        ok(info->cExtension == 1, "Expected 1 extensions, got %ld\n",
         info->cExtension);
        LocalFree(buf);
    }
}

static const LPCSTR keyUsages[] = { szOID_PKIX_KP_CODE_SIGNING,
 szOID_PKIX_KP_CLIENT_AUTH, szOID_RSA_RSA };
static const BYTE encodedUsage[] = {
 0x30, 0x1f, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x03,
 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x02, 0x06, 0x09,
 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 };

static void test_encodeEnhancedKeyUsage(DWORD dwEncoding)
{
    BOOL ret;
    BYTE *buf = NULL;
    DWORD size = 0;
    CERT_ENHKEY_USAGE usage;

    /* Test with empty usage */
    usage.cUsageIdentifier = 0;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE, &usage,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(emptySequence), "Wrong size %ld\n", size);
        ok(!memcmp(buf, emptySequence, size), "Got unexpected value\n");
        LocalFree(buf);
    }
    /* Test with a few usages */
    usage.cUsageIdentifier = sizeof(keyUsages) / sizeof(keyUsages[0]);
    usage.rgpszUsageIdentifier = (LPSTR *)keyUsages;
    ret = CryptEncodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE, &usage,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        ok(size == sizeof(encodedUsage), "Wrong size %ld\n", size);
        ok(!memcmp(buf, encodedUsage, size), "Got unexpected value\n");
        LocalFree(buf);
    }
}

static void test_decodeEnhancedKeyUsage(DWORD dwEncoding)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;

    ret = CryptDecodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE,
     emptySequence, sizeof(emptySequence), CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_ENHKEY_USAGE *usage = (CERT_ENHKEY_USAGE *)buf;

        ok(size >= sizeof(CERT_ENHKEY_USAGE),
         "Wrong size %ld\n", size);
        ok(usage->cUsageIdentifier == 0, "Expected 0 CRL entries, got %ld\n",
         usage->cUsageIdentifier);
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(dwEncoding, X509_ENHANCED_KEY_USAGE,
     encodedUsage, sizeof(encodedUsage), CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08lx\n", GetLastError());
    if (buf)
    {
        CERT_ENHKEY_USAGE *usage = (CERT_ENHKEY_USAGE *)buf;
        DWORD i;

        ok(size >= sizeof(CERT_ENHKEY_USAGE),
         "Wrong size %ld\n", size);
        ok(usage->cUsageIdentifier == sizeof(keyUsages) / sizeof(keyUsages[0]),
         "Wrong CRL entries count %ld\n", usage->cUsageIdentifier);
        for (i = 0; i < usage->cUsageIdentifier; i++)
            ok(!strcmp(usage->rgpszUsageIdentifier[i], keyUsages[i]),
             "Expected OID %s, got %s\n", keyUsages[i],
             usage->rgpszUsageIdentifier[i]);
        LocalFree(buf);
    }
}

/* Free *pInfo with HeapFree */
static void testExportPublicKey(HCRYPTPROV csp, PCERT_PUBLIC_KEY_INFO *pInfo)
{
    BOOL ret;
    DWORD size = 0;
    HCRYPTKEY key;

    /* This crashes
    ret = CryptExportPublicKeyInfoEx(0, 0, 0, NULL, 0, NULL, NULL, NULL);
     */
    ret = CryptExportPublicKeyInfoEx(0, 0, 0, NULL, 0, NULL, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    ret = CryptExportPublicKeyInfoEx(0, AT_SIGNATURE, 0, NULL, 0, NULL, NULL,
     &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    ret = CryptExportPublicKeyInfoEx(0, 0, X509_ASN_ENCODING, NULL, 0, NULL,
     NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    ret = CryptExportPublicKeyInfoEx(0, AT_SIGNATURE, X509_ASN_ENCODING, NULL,
     0, NULL, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    /* Test with no key */
    ret = CryptExportPublicKeyInfoEx(csp, AT_SIGNATURE, X509_ASN_ENCODING, NULL,
     0, NULL, NULL, &size);
    ok(!ret && GetLastError() == NTE_NO_KEY, "Expected NTE_NO_KEY, got %08lx\n",
     GetLastError());
    ret = CryptGenKey(csp, AT_SIGNATURE, 0, &key);
    ok(ret, "CryptGenKey failed: %08lx\n", GetLastError());
    if (ret)
    {
        ret = CryptExportPublicKeyInfoEx(csp, AT_SIGNATURE, X509_ASN_ENCODING,
         NULL, 0, NULL, NULL, &size);
        ok(ret, "CryptExportPublicKeyInfoEx failed: %08lx\n", GetLastError());
        *pInfo = HeapAlloc(GetProcessHeap(), 0, size);
        if (*pInfo)
        {
            ret = CryptExportPublicKeyInfoEx(csp, AT_SIGNATURE,
             X509_ASN_ENCODING, NULL, 0, NULL, *pInfo, &size);
            ok(ret, "CryptExportPublicKeyInfoEx failed: %08lx\n",
             GetLastError());
            if (ret)
            {
                /* By default (we passed NULL as the OID) the OID is
                 * szOID_RSA_RSA.
                 */
                ok(!strcmp((*pInfo)->Algorithm.pszObjId, szOID_RSA_RSA),
                 "Expected %s, got %s\n", szOID_RSA_RSA,
                 (*pInfo)->Algorithm.pszObjId);
            }
        }
    }
}

static const BYTE expiredCert[] = { 0x30, 0x82, 0x01, 0x33, 0x30, 0x81, 0xe2,
 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0xc4, 0xd7, 0x7f, 0x0e, 0x6f, 0xa6,
 0x8c, 0xaa, 0x47, 0x47, 0x40, 0xe7, 0xb7, 0x0b, 0x4a, 0x7f, 0x30, 0x09, 0x06,
 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1d, 0x05, 0x00, 0x30, 0x1f, 0x31, 0x1d, 0x30,
 0x1b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x14, 0x61, 0x72, 0x69, 0x63, 0x40,
 0x63, 0x6f, 0x64, 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63,
 0x6f, 0x6d, 0x30, 0x1e, 0x17, 0x0d, 0x36, 0x39, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x37, 0x30, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x36, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x1f, 0x31, 0x1d, 0x30,
 0x1b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x14, 0x61, 0x72, 0x69, 0x63, 0x40,
 0x63, 0x6f, 0x64, 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63,
 0x6f, 0x6d, 0x30, 0x5c, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x4b, 0x00, 0x30, 0x48, 0x02, 0x41,
 0x00, 0xa1, 0xaf, 0x4a, 0xea, 0xa7, 0x83, 0x57, 0xc0, 0x37, 0x33, 0x7e, 0x29,
 0x5e, 0x0d, 0xfc, 0x44, 0x74, 0x3a, 0x1d, 0xc3, 0x1b, 0x1d, 0x96, 0xed, 0x4e,
 0xf4, 0x1b, 0x98, 0xec, 0x69, 0x1b, 0x04, 0xea, 0x25, 0xcf, 0xb3, 0x2a, 0xf5,
 0xd9, 0x22, 0xd9, 0x8d, 0x08, 0x39, 0x81, 0xc6, 0xe0, 0x4f, 0x12, 0x37, 0x2a,
 0x3f, 0x80, 0xa6, 0x6c, 0x67, 0x43, 0x3a, 0xdd, 0x95, 0x0c, 0xbb, 0x2f, 0x6b,
 0x02, 0x03, 0x01, 0x00, 0x01, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02,
 0x1d, 0x05, 0x00, 0x03, 0x41, 0x00, 0x8f, 0xa2, 0x5b, 0xd6, 0xdf, 0x34, 0xd0,
 0xa2, 0xa7, 0x47, 0xf1, 0x13, 0x79, 0xd3, 0xf3, 0x39, 0xbd, 0x4e, 0x2b, 0xa3,
 0xf4, 0x63, 0x37, 0xac, 0x5a, 0x0c, 0x5e, 0x4d, 0x0d, 0x54, 0x87, 0x4f, 0x31,
 0xfb, 0xa0, 0xce, 0x8f, 0x9a, 0x2f, 0x4d, 0x48, 0xc6, 0x84, 0x8d, 0xf5, 0x70,
 0x74, 0x17, 0xa5, 0xf3, 0x66, 0x47, 0x06, 0xd6, 0x64, 0x45, 0xbc, 0x52, 0xef,
 0x49, 0xe5, 0xf9, 0x65, 0xf3 };

static void testImportPublicKey(HCRYPTPROV csp, PCERT_PUBLIC_KEY_INFO info)
{
    BOOL ret;
    HCRYPTKEY key;
    PCCERT_CONTEXT context;

    /* These crash
    ret = CryptImportPublicKeyInfoEx(0, 0, NULL, 0, 0, NULL, NULL);
    ret = CryptImportPublicKeyInfoEx(0, 0, NULL, 0, 0, NULL, &key);
    ret = CryptImportPublicKeyInfoEx(0, 0, info, 0, 0, NULL, NULL);
    ret = CryptImportPublicKeyInfoEx(csp, X509_ASN_ENCODING, info, 0, 0, NULL,
     NULL);
     */
    ret = CryptImportPublicKeyInfoEx(0, 0, info, 0, 0, NULL, &key);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    ret = CryptImportPublicKeyInfoEx(csp, 0, info, 0, 0, NULL, &key);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    ret = CryptImportPublicKeyInfoEx(0, X509_ASN_ENCODING, info, 0, 0, NULL,
     &key);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    ret = CryptImportPublicKeyInfoEx(csp, X509_ASN_ENCODING, info, 0, 0, NULL,
     &key);
    ok(ret, "CryptImportPublicKeyInfoEx failed: %08lx\n", GetLastError());
    CryptDestroyKey(key);

    /* Test importing a public key from a certificate context */
    context = CertCreateCertificateContext(X509_ASN_ENCODING, expiredCert,
     sizeof(expiredCert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        ok(!strcmp(szOID_RSA_RSA,
         context->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId),
         "Expected %s, got %s\n", szOID_RSA_RSA,
         context->pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId);
        ret = CryptImportPublicKeyInfoEx(csp, X509_ASN_ENCODING,
         &context->pCertInfo->SubjectPublicKeyInfo, 0, 0, NULL, &key);
        ok(ret, "CryptImportPublicKeyInfoEx failed: %08lx\n", GetLastError());
        CryptDestroyKey(key);
        CertFreeCertificateContext(context);
    }
}

static const char cspName[] = "WineCryptTemp";

static void testPortPublicKeyInfo(void)
{
    HCRYPTPROV csp;
    BOOL ret;
    PCERT_PUBLIC_KEY_INFO info = NULL;

    /* Just in case a previous run failed, delete this thing */
    CryptAcquireContextA(&csp, cspName, MS_DEF_PROV, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    ret = CryptAcquireContextA(&csp, cspName, MS_DEF_PROV, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);

    testExportPublicKey(csp, &info);
    testImportPublicKey(csp, info);

    HeapFree(GetProcessHeap(), 0, info);
    CryptReleaseContext(csp, 0);
    ret = CryptAcquireContextA(&csp, cspName, MS_DEF_PROV, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
}

START_TEST(encode)
{
    static const DWORD encodings[] = { X509_ASN_ENCODING, PKCS_7_ASN_ENCODING,
     X509_ASN_ENCODING | PKCS_7_ASN_ENCODING };
    DWORD i;

    for (i = 0; i < sizeof(encodings) / sizeof(encodings[0]); i++)
    {
        test_encodeInt(encodings[i]);
        test_decodeInt(encodings[i]);
        test_encodeEnumerated(encodings[i]);
        test_decodeEnumerated(encodings[i]);
        test_encodeFiletime(encodings[i]);
        test_decodeFiletime(encodings[i]);
        test_encodeName(encodings[i]);
        test_decodeName(encodings[i]);
        test_encodeNameValue(encodings[i]);
        test_decodeNameValue(encodings[i]);
        test_encodeUnicodeNameValue(encodings[i]);
        test_decodeUnicodeNameValue(encodings[i]);
        test_encodeAltName(encodings[i]);
        test_decodeAltName(encodings[i]);
        test_encodeOctets(encodings[i]);
        test_decodeOctets(encodings[i]);
        test_encodeBits(encodings[i]);
        test_decodeBits(encodings[i]);
        test_encodeBasicConstraints(encodings[i]);
        test_decodeBasicConstraints(encodings[i]);
        test_encodeRsaPublicKey(encodings[i]);
        test_decodeRsaPublicKey(encodings[i]);
        test_encodeSequenceOfAny(encodings[i]);
        test_decodeSequenceOfAny(encodings[i]);
        test_encodeExtensions(encodings[i]);
        test_decodeExtensions(encodings[i]);
        test_encodePublicKeyInfo(encodings[i]);
        test_decodePublicKeyInfo(encodings[i]);
        test_encodeCertToBeSigned(encodings[i]);
        test_decodeCertToBeSigned(encodings[i]);
        test_encodeCert(encodings[i]);
        test_decodeCert(encodings[i]);
        test_encodeCRLDistPoints(encodings[i]);
        test_decodeCRLDistPoints(encodings[i]);
        test_encodeCRLIssuingDistPoint(encodings[i]);
        test_decodeCRLIssuingDistPoint(encodings[i]);
        test_encodeCRLToBeSigned(encodings[i]);
        test_decodeCRLToBeSigned(encodings[i]);
        test_encodeEnhancedKeyUsage(encodings[i]);
        test_decodeEnhancedKeyUsage(encodings[i]);
    }
    testPortPublicKeyInfo();
}
