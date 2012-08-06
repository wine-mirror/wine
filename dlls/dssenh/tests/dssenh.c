/*
 * Unit tests for dss functions
 *
 * Copyright (c) 2012 Marek Kamil Chmiel
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

#include <string.h>
#include <stdio.h>
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"

static void test_acquire_context(void)
{   /* failure tests common between all four CSP providers */

    HCRYPTPROV hProv = 0;
    BOOL result;

    /* cannot acquire provider with 0 as Prov Type and NULL as CSP name */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, 0, 0);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, 0, CRYPT_VERIFYCONTEXT);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());

    /* flag allows us to delete a keyset, but not of an unknown provider */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, 0, CRYPT_DELETEKEYSET);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());

    /* cannot acquire along with PROV_RSA_SIG, not compatible */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, PROV_RSA_SIG, 0);
    todo_wine
    ok(!result && GetLastError() == NTE_PROV_TYPE_NO_MATCH,
        "Expected NTE_PROV_TYPE_NO_MATCH, got %08x\n", GetLastError());

    /* cannot acquire along with MS_DEF_RSA_SIG_PROV, not compatible */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_RSA_SIG_PROV, PROV_DSS, 0);
    ok(!result && GetLastError() == NTE_KEYSET_NOT_DEF,
        "Expected NTE_KEYSET_NOT_DEF, got %08x\n", GetLastError());

    /* cannot acquire provider with 0 as Prov Type */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, 0, 0);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08x\n", GetLastError());

    /* test base DSS provider (PROV_DSS) */

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, CRYPT_VERIFYCONTEXT);
    if(!result)
    {
        skip("DSS csp is currently not available, skipping tests.\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DSS, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, 0);
    ok(result, "Expected no errors.\n");

    /* test DSS Diffie Hellman provider (PROV_DSS_DH) */

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_DH_PROV, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    /* test DSS Enhanced provider (MS_ENH_DSS_DH_PROV) */

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    if(!result && GetLastError() == NTE_KEYSET_NOT_DEF)
    {
        win_skip("DSSENH and Schannel provider is broken on WinNT4\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    /* test DSS Schannel provider (PROV_DH_SCHANNEL) */

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV, PROV_DH_SCHANNEL, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DH_SCHANNEL, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV, PROV_DH_SCHANNEL, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    /* failure tests, cannot acquire context because the key container already exists */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV, PROV_DH_SCHANNEL, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08x\n", GetLastError());
}

struct keylength_test {
    ALG_ID algid;
    DWORD flags;
    BOOL expectedResult;
    DWORD expectedError;
    BOOL brokenResult;
    DWORD brokenError;
};

static const struct keylength_test baseDSS_keylength[] = {
    /* AT_KEYEXCHANGE is not supported by the base DSS provider */
    {AT_KEYEXCHANGE, 448 << 16, FALSE, NTE_BAD_ALGID, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {AT_KEYEXCHANGE, 512 << 16, FALSE, NTE_BAD_ALGID, TRUE}, /* success on WinNT4 */
    {AT_KEYEXCHANGE, 1024 << 16, FALSE, NTE_BAD_ALGID, TRUE}, /* success on WinNT4 */
    {AT_KEYEXCHANGE, 1088 << 16, FALSE, NTE_BAD_ALGID, FALSE, NTE_BAD_FLAGS},/* WinNT4 and Win2k */
    /* min 512 max 1024 increment by 64 */
    {AT_SIGNATURE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 512 << 16, TRUE},
    {AT_SIGNATURE, 513 << 16, FALSE, NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {AT_SIGNATURE, 768 << 16, TRUE},
    {AT_SIGNATURE, 1024 << 16, TRUE},
    {AT_SIGNATURE, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    /* CALG_DH_EPHEM is not supported by the base DSS provider */
    {CALG_DH_EPHEM, 448 << 16, FALSE, NTE_BAD_ALGID, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DH_EPHEM, 512 << 16, FALSE, NTE_BAD_ALGID, TRUE}, /* success on WinNT4 */
    {CALG_DH_EPHEM, 1024 << 16, FALSE, NTE_BAD_ALGID, TRUE}, /* success on WinNT4 */
    {CALG_DH_EPHEM, 1088 << 16, FALSE, NTE_BAD_ALGID, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    /* CALG_DH_SF is not supported by the base DSS provider */
    {CALG_DH_SF, 448 << 16, FALSE, NTE_BAD_ALGID, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DH_SF, 512 << 16, FALSE, NTE_BAD_ALGID, TRUE}, /* success on WinNT4 */
    {CALG_DH_SF, 1024 << 16, FALSE, NTE_BAD_ALGID, TRUE}, /* success on WinNT4 */
    {CALG_DH_SF, 1088 << 16, FALSE, NTE_BAD_ALGID, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    /* min 512 max 1024, increment by 64 */
    {CALG_DSS_SIGN, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 512 << 16, TRUE},
    {CALG_DSS_SIGN, 513 << 16, FALSE, NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DSS_SIGN, 768 << 16, TRUE},
    {CALG_DSS_SIGN, 1024 << 16, TRUE},
    {CALG_DSS_SIGN, 1088 << 16, FALSE, NTE_BAD_FLAGS}
};

static const struct keylength_test dssDH_keylength[] = {
    /* min 512 max 1024, increment by 64 */
    {AT_KEYEXCHANGE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_KEYEXCHANGE, 512 << 16, TRUE},
    {AT_KEYEXCHANGE, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {AT_KEYEXCHANGE, 768 << 16, TRUE},
    {AT_KEYEXCHANGE, 1024 << 16, TRUE},
    {AT_KEYEXCHANGE, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 512 << 16, TRUE},
    {AT_SIGNATURE, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {AT_SIGNATURE, 768 << 16, TRUE},
    {AT_SIGNATURE, 1024 << 16, TRUE},
    {AT_SIGNATURE, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 512 << 16, TRUE},
    {CALG_DH_EPHEM, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DH_EPHEM, 768 << 16, TRUE},
    {CALG_DH_EPHEM, 1024 << 16, TRUE},
    {CALG_DH_EPHEM, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 512 << 16, TRUE},
    {CALG_DH_SF, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DH_SF, 768 << 16, TRUE},
    {CALG_DH_SF, 1024 << 16, TRUE},
    {CALG_DH_SF, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 512 << 16, TRUE},
    {CALG_DSS_SIGN, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DSS_SIGN, 768 << 16, TRUE},
    {CALG_DSS_SIGN, 1024 << 16, TRUE},
    {CALG_DSS_SIGN, 1088 << 16, FALSE, NTE_BAD_FLAGS}
};

static const struct keylength_test dssENH_keylength[] = {
    /* min 512 max 1024 (AT_KEYEXCHANGE max 4096), increment by 64*/
    {AT_KEYEXCHANGE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_KEYEXCHANGE, 512 << 16, TRUE},
    {AT_KEYEXCHANGE, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {AT_KEYEXCHANGE, 768 << 16, TRUE},
    {AT_KEYEXCHANGE, 1024 << 16, TRUE},
    {AT_KEYEXCHANGE, 1088 << 16, TRUE},
    {AT_KEYEXCHANGE, 2048 << 16, TRUE},
    /* Keylength too large - test bot timeout.
    {AT_KEYEXCHANGE, 3072 << 16, TRUE},
    {AT_KEYEXCHANGE, 4096 << 16, TRUE}, */
    {AT_KEYEXCHANGE, 4160 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 512 << 16, TRUE},
    {AT_SIGNATURE, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {AT_SIGNATURE, 768 << 16, TRUE},
    {AT_SIGNATURE, 1024 << 16, TRUE},
    {AT_SIGNATURE, 1032 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 512 << 16, TRUE},
    {CALG_DH_EPHEM, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DH_EPHEM, 768 << 16, TRUE},
    {CALG_DH_EPHEM, 1024 << 16, TRUE},
    {CALG_DH_EPHEM, 1040 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 512 << 16, TRUE},
    {CALG_DH_SF, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DH_SF, 768 << 16, TRUE},
    {CALG_DH_SF, 1024 << 16, TRUE},
    {CALG_DH_SF, 1032 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 512 << 16, TRUE},
    {CALG_DSS_SIGN, 513 << 16, FALSE,  NTE_FAIL, FALSE, NTE_BAD_FLAGS}, /* WinNT4 and Win2k */
    {CALG_DSS_SIGN, 768 << 16, TRUE},
    {CALG_DSS_SIGN, 1024 << 16, TRUE},
    {CALG_DSS_SIGN, 1088 << 16, FALSE, NTE_BAD_FLAGS}
};

static void test_keylength_array(HCRYPTPROV hProv,const struct keylength_test *tests, int testLen)
{
    HCRYPTKEY key;
    BOOL result;
    int i;

    for (i = 0; i < testLen; i++)
    {
        SetLastError(0xdeadbeef);
        result = CryptGenKey(hProv, tests[i].algid, tests[i].flags, &key);

        /* success */
        if(tests[i].expectedResult)
        {
            ok(result, "Expected a key, got %08x\n", GetLastError());
            result = CryptDestroyKey(key);
            ok(result, "Expected no errors.\n");
        }
        else
        {   /* error but success on older system */
            if(tests[i].brokenResult)
                ok((!result && GetLastError() == tests[i].expectedError) ||
                    broken(result), "Expected a key, got %x.\n", GetLastError());
            else
            {
                /* error */
                if(!tests[i].brokenError)
                    ok(!result && GetLastError() == tests[i].expectedError,
                        "Expected a key, got %x.\n", GetLastError());

                /* error but different error on older system */
                else
                    ok(!result && (GetLastError() == tests[i].expectedError ||
                        broken(GetLastError() == tests[i].brokenError)),
                        "Expected a key, got %x.\n", GetLastError());
            }
        }
    }
}

#define TESTLEN(x) (sizeof(x) / sizeof((x)[0]))

static void test_keylength(void)
{
    HCRYPTPROV hProv = 0;
    BOOL result;

    /* acquire base dss provider */
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, CRYPT_VERIFYCONTEXT);
    if(!result)
    {
        skip("DSSENH is currently not available, skipping key length tests.\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    /* perform keylength tests */
    test_keylength_array(hProv, baseDSS_keylength, TESTLEN(baseDSS_keylength));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of CSP provider.\n");

    /* acquire diffie hellman dss provider */
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    /* perform keylength tests */
    test_keylength_array(hProv, dssDH_keylength, TESTLEN(dssDH_keylength));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of CSP provider.\n");

    /* acquire enhanced dss provider */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    if(!result && GetLastError() == NTE_KEYSET_NOT_DEF)
    {
        win_skip("DSSENH and Schannel provider is broken on WinNT4\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    /* perform keylength tests */
    test_keylength_array(hProv, dssENH_keylength, TESTLEN(dssENH_keylength));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of CSP provider.\n");

    /* acquire schannel dss provider */
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV, PROV_DH_SCHANNEL, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    /* perform keylength tests */
    test_keylength_array(hProv, dssENH_keylength, TESTLEN(dssENH_keylength));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of CSP provider.\n");
}

struct hash_test {
    ALG_ID algid;
    BYTE* input;
    DWORD dataLen;
    BYTE hash[20];
    DWORD hashLen;
};

static const char testHashVal1[] = "I love working with Wine";
static const char testHashVal2[] = "Wine is not an emulater.";
static const char testHashVal3[] = "";

static const struct hash_test hash_data[] = {
    {CALG_MD5, (BYTE *)testHashVal1, sizeof(testHashVal1),
    {0x4f, 0xf4, 0xd0, 0xdf, 0xe8, 0xf6, 0x6b, 0x1b,
        0x87, 0xea, 0xca, 0x3d, 0xe8, 0x3c, 0xdd, 0xae}, 16},
    {CALG_MD5, (BYTE *)testHashVal2, sizeof(testHashVal2),
    {0x80, 0x5c, 0x1c, 0x0e, 0x79, 0x70, 0xd9, 0x38,
        0x04, 0x46, 0x19, 0xbe, 0x38, 0x1f, 0xef, 0xe1}, 16},
    {CALG_MD5, (BYTE *)testHashVal3, sizeof(testHashVal3),
    {0x93, 0xb8, 0x85, 0xad, 0xfe, 0x0d, 0xa0, 0x89,
        0xcd, 0xf6, 0x34, 0x90, 0x4f, 0xd5, 0x9f, 0x71}, 16},
    {CALG_SHA, (BYTE *)testHashVal1, sizeof(testHashVal1),
    {0x2a, 0xd0, 0xc9, 0x42, 0xfb, 0x73, 0x02, 0x48, 0xbb, 0x5f,
        0xc2, 0xa4, 0x78, 0xdd, 0xe4, 0x3b, 0xfc, 0x76, 0xe9, 0xe2}, 20},
    {CALG_SHA, (BYTE *)testHashVal2, sizeof(testHashVal2),
    {0xfd, 0xfc, 0xab, 0x3a, 0xde, 0x33, 0x01, 0x38, 0xfe, 0xbb,
        0xc3, 0x13, 0x84, 0x20, 0x9e, 0x55, 0x94, 0x8d, 0xc6, 0x05}, 20},
    {CALG_SHA, (BYTE *)testHashVal3, sizeof(testHashVal3),
    {0x5b, 0xa9, 0x3c, 0x9d, 0xb0, 0xcf, 0xf9, 0x3f, 0x52, 0xb5,
        0x21, 0xd7, 0x42, 0x0e, 0x43, 0xf6, 0xed, 0xa2, 0x78, 0x4f}, 20}
};

static void test_hash(const struct hash_test *tests, int testLen)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash;
    BYTE hashValue[36];
    BOOL result;
    int i;

    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    if(!result)
    {
        skip("DSSENH is currently not available, skipping hashing tests.\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    for(i = 0; i < testLen; i++)
    {
        BYTE* data = tests[i].input;
        DWORD dataLen = tests[i].dataLen;
        DWORD hashLen;

        /* test algid hash */
        result = CryptCreateHash(hProv, tests[i].algid, 0, 0, &hHash);
        ok(result, "Expected creation of a hash.\n");

        result = CryptHashData(hHash, data, dataLen, 0);
        ok(result, "Expected data to be added to hash.\n");

        dataLen = sizeof(DWORD);
        result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&hashLen, &dataLen, 0);
        ok(result && (hashLen == tests[i].hashLen), "Expected %d hash len, got %d.Error: %x\n",
            tests[i].hashLen, hashLen, GetLastError());

        result = CryptGetHashParam(hHash, HP_HASHVAL, hashValue, &hashLen, 0);
        ok(result, "Expected hash value return.\n");

        ok(!memcmp(hashValue, tests[i].hash, tests[i].hashLen), "Incorrect hash output.\n");

        result = CryptHashData(hHash, data, dataLen, 0);
        ok(!result, "Should not be able to add to hash.\n");

        result = CryptDestroyHash(hHash);
        ok(result, "Expected destruction of hash.\n");
    }
    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the DSS Enhanced provider.\n");
}

START_TEST(dssenh)
{
    test_acquire_context();
    test_keylength();
    test_hash(hash_data, TESTLEN(hash_data));
}
