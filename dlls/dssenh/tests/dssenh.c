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

struct encrypt_test {
    ALG_ID algid;
    DWORD keyLength;
    const char *plain;
    DWORD plainLen;
    const BYTE *decrypted;
    const BYTE *encrypted;
};

static const char dataToEncrypt1[] = "Great performance with Wine.";
static const char dataToEncrypt2[] = "Wine implements Windows API";
static const char dataToEncrypt3[] = "";

static const BYTE encrypted3DES_1[] = {
0x6c,0x60,0x19,0x41,0x27,0xc1,0x16,0x69, 0x6f,0x96,0x0c,0x2e,0xa4,0x5f,0xf5,0x6a,
0xed,0x4b,0xec,0xd4,0x92,0x0c,0xe2,0x34, 0xe1,0x4a,0xb5,0xe2,0x05,0x43,0xfe,0x17
};
static const BYTE encrypted3DES_2[] = {
0x17,0xeb,0x80,0xde,0xac,0x4d,0x9e,0xd0, 0xa9,0xae,0x74,0xb5,0x86,0x1a,0xea,0xb4,
0x96,0x27,0x5d,0x75,0x4f,0xdd,0x87,0x60, 0xfc,0xaf,0xa1,0x82,0x83,0x09,0xf1,0xca
};
static const BYTE encrypted3DES_3[] = {0xaf, 0x36, 0xc0, 0x3d, 0x78, 0x64, 0xc4, 0x4a};

static const BYTE encrypted3DES112_1[] = {
0xb3,0xf8,0x4b,0x08,0xd6,0x23,0xcb,0xca, 0x43,0x26,0xd9,0x9f,0x6b,0x99,0x09,0xe9,
0x8c,0x4c,0x7d,0xef,0x49,0xda,0x0b,0x44, 0xcc,0x8d,0x06,0x6b,0xed,0xb7,0xf1,0x67
};
static const BYTE encrypted3DES112_2[] = {
0xdc,0xcf,0x93,0x11,0x7a,0xe4,0xcd,0x3f, 0x11,0xd8,0xe0,0x1e,0xe0,0x8d,0x9c,0xba,
0x97,0x5d,0x74,0x4d,0x83,0x03,0x5c,0xf2, 0x01,0xaf,0xed,0x7a,0x87,0x8f,0x88,0x8b
};
static const BYTE encrypted3DES112_3[] = {0x04, 0xb3, 0x9c, 0x59, 0x48, 0xc7, 0x2f, 0xd1};

static const BYTE encryptedDES_1[] = {
0x3d,0xdc,0x54,0xaf,0x66,0x72,0x4e,0xef, 0x9d,0x35,0x02,0xc2,0x1a,0xf4,0x1f,0x01,
0xb1,0xaf,0x13,0xd9,0xbe,0x7b,0xd4,0xf3, 0xf5,0x9d,0x2a,0xd8,0x32,0x90,0xe9,0x0b
};
static const BYTE encryptedDES_2[] = {
0xa8,0x05,0xd7,0xe9,0x61,0xf4,0x6c,0xce, 0x95,0x2b,0x52,0x08,0x25,0x03,0x30,0xac,
0xd7,0xe7,0xd3,0x07,0xb2,0x68,0x63,0x7b, 0xe3,0xab,0x26,0x1e,0x5c,0xec,0x42,0x4f
};
static const BYTE encryptedDES_3[] = {0x35, 0x02, 0xbb, 0x7c, 0x43, 0x5b, 0xf5, 0x59};

static const BYTE encryptedRC2_1[] = {
0x9e,0xcb,0xa2,0x27,0xc2,0xec,0x10,0xe0, 0x94,0xb3,0xc3,0x9d,0x7d,0xe2,0x12,0xe4,
0xb0,0xde,0xd9,0x46,0xca,0x1f,0xa6,0xfa, 0xa4,0x79,0x08,0x59,0xa6,0x00,0x62,0x16
};
static const BYTE encryptedRC2_2[] = {
0x29,0x06,0xfd,0xa1,0xe0,0x88,0x89,0xb0, 0x4d,0x7f,0x96,0x9d,0x2c,0x44,0xa1,0xd2,
0xbe,0xc6,0xaf,0x10,0xb8,0x86,0x68,0x1b, 0x1d,0x9f,0x3c,0xc4,0x12,0x02,0xbc,0x73
};
static const BYTE encryptedRC2_3[] = {0x26,0x40,0x73,0xfe,0x13,0xbb,0x32,0xa8};

static const BYTE encryptedRC4_1[] = {
0x5a,0x48,0xeb,0x16,0x96,0x23,0x16,0xb7, 0xbb,0x36,0xe8,0x43,0x88,0x74,0xb1,0x9d,
0x96,0xf0,0x84,0x0f,0x5a,0x56,0xf9,0x62, 0xae,0xb5,0x4a,0xce,0x52
};
static const BYTE encryptedRC4_2[] = {
0x4a,0x53,0xe0,0x12,0xc2,0x6a,0x0b,0xa2, 0xa5,0x35,0xea,0x54,0x8b,0x61,0xac,0xde,
0xa4,0xb9,0x9d,0x02,0x41,0x49,0xaa,0x15, 0x86,0x8b,0x66,0xe0
};
static const BYTE encryptedRC4_3[] = {0x1d};

static const struct encrypt_test encrypt_data[] = {
    {CALG_3DES, 168 << 16, dataToEncrypt1, sizeof(dataToEncrypt1), (BYTE *)dataToEncrypt1,
        encrypted3DES_1},
    {CALG_3DES, 168 << 16, dataToEncrypt2, sizeof(dataToEncrypt2), (BYTE *)dataToEncrypt2,
        encrypted3DES_2},
    {CALG_3DES, 168 << 16, dataToEncrypt3, sizeof(dataToEncrypt3), (BYTE *)dataToEncrypt3,
        encrypted3DES_3},
    {CALG_3DES_112, 112 << 16, dataToEncrypt1, sizeof(dataToEncrypt1), (BYTE *)dataToEncrypt1,
        encrypted3DES112_1},
    {CALG_3DES_112, 112 << 16, dataToEncrypt2, sizeof(dataToEncrypt2), (BYTE *)dataToEncrypt2,
        encrypted3DES112_2},
    {CALG_3DES_112, 112 << 16, dataToEncrypt3, sizeof(dataToEncrypt3), (BYTE *)dataToEncrypt3,
        encrypted3DES112_3},
    {CALG_DES, 56 << 16, dataToEncrypt1, sizeof(dataToEncrypt1), (BYTE *)dataToEncrypt1,
        encryptedDES_1},
    {CALG_DES, 56 << 16, dataToEncrypt2, sizeof(dataToEncrypt2), (BYTE *)dataToEncrypt2,
        encryptedDES_2},
    {CALG_DES, 56 << 16, dataToEncrypt3, sizeof(dataToEncrypt3), (BYTE *)dataToEncrypt3,
        encryptedDES_3},
    /* CALG_RC2 key unexpected results under Win2K when default key length is used, here we use
       minimum length because Win2K's DSSENH provider has a differnt default key length compared
       to the younger operating systems, though there is no default key len issue with CALG_RC4 */
    {CALG_RC2, 40 << 16, dataToEncrypt1, sizeof(dataToEncrypt1), (BYTE *)dataToEncrypt1,
        encryptedRC2_1},
    {CALG_RC2, 40 << 16, dataToEncrypt2, sizeof(dataToEncrypt2), (BYTE *)dataToEncrypt2,
        encryptedRC2_2},
    {CALG_RC2, 40 << 16, dataToEncrypt3, sizeof(dataToEncrypt3), (BYTE *)dataToEncrypt3,
        encryptedRC2_3},
    {CALG_RC4, 40 << 16, dataToEncrypt1, sizeof(dataToEncrypt1), (BYTE *)dataToEncrypt1,
        encryptedRC4_1},
    {CALG_RC4, 40 << 16, dataToEncrypt2, sizeof(dataToEncrypt2), (BYTE *)dataToEncrypt2,
        encryptedRC4_2},
    {CALG_RC4, 40 << 16, dataToEncrypt3, sizeof(dataToEncrypt3), (BYTE *)dataToEncrypt3,
        encryptedRC4_3}
};

static void test_data_encryption(const struct encrypt_test *tests, int testLen)
{   /* Here we test the same encryption ciphers as the RSAENH cryptographic service provider */
    HCRYPTPROV hProv = 0;
    HCRYPTKEY pKey = 0;
    HCRYPTHASH hHash;
    const char dataToHash[] = "I love working with Wine";
    unsigned char pbData[36];
    DWORD dataLen;
    BOOL result;
    int i;

    /* acquire dss enhanced provider */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    if(!result && GetLastError() == NTE_KEYSET_NOT_DEF)
    {
        skip("DSSENH is currently not available, skipping encryption tests.\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    /* testing various encryption algorithms */
    for(i = 0; i < testLen; i++)
    {
        memcpy(pbData, tests[i].plain, tests[i].plainLen);
        dataLen = tests[i].plainLen;

        SetLastError(0xdeadbeef);
        result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
        ok(result, "Expected creation of a MD5 hash for key derivation.\n");

        result = CryptHashData(hHash, (BYTE *)dataToHash, sizeof(dataToHash), 0);
        ok(result, "Expected data to be added to hash for key derivation.\n");

        /* Derive key */
        result = CryptDeriveKey(hProv, tests[i].algid, hHash, tests[i].keyLength, &pKey);
        ok(result, "Expected a derived key.\n");

        result = CryptDestroyHash(hHash);
        ok(result, "Expected destruction of hash after deriving key.\n");

        /* testing CryptEncrypt with ALGID from array */
        result = CryptEncrypt(pKey, 0, TRUE, 0, pbData, &dataLen, 36);
        ok(result, "Expected data encryption.\n");

        /* Verify we have received expected encrypted data */
        ok(!memcmp(pbData, tests[i].encrypted, dataLen), "Incorrect encrypted data.\n");

        result = CryptDecrypt(pKey, 0, TRUE, 0, pbData, &dataLen);
        ok(result, "Expected data decryption.\n");

        /* Verify we have received expected decrypted data */
        ok(!memcmp(pbData, tests[i].decrypted, dataLen), "Incorrect decrypted data.\n");

        result = CryptDestroyKey(pKey);
        ok(result, "Expected no DestroyKey errors.\n");
    }
    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider\n");
}

struct ciphermode_test {
    DWORD cipherMode;
    BOOL expectedResult;
    DWORD expectedError;
    const BYTE *encrypted;
};

static const BYTE encryptedCFB[] = {
0x51,0x15,0x77,0xab,0x62,0x1f,0x7d,0xcb, 0x35,0x1e,0xd8,0xd3,0x2a,0x00,0xf0,0x94,
0x7c,0xa5,0x28,0xda,0xb8,0x81,0x15,0x99, 0xd1,0xd5,0x06,0x18,0xb8,0x85,0xfc,0xc9
};
static const BYTE encryptedCBC[] = {
0x8f,0x7b,0x56,0xeb,0xad,0x4d,0x76,0xc2, 0xd5,0x1d,0xf0,0x60,0x9d,0xde,0x96,0xe8,
0xb7,0x7b,0xeb,0x4b,0xee,0x3f,0xae,0x05, 0xdd,0x3b,0x62,0x47,0x7f,0x6f,0x79,0x6c
};
static const BYTE encryptedECB[] = {
0x8f,0x7b,0x56,0xeb,0xad,0x4d,0x76,0xc2, 0x8b,0xe0,0x4e,0xe4,0x98,0x4f,0xb8,0x3b,
0xf3,0xeb,0x6f,0x0a,0x57,0x91,0xdd,0xc7, 0x64,0x8b,0xb9,0x50,0xe6,0x5e,0x76,0x72
};

static const struct ciphermode_test ciphermode_data[] = {
    {CRYPT_MODE_CFB, TRUE, 0xdeadbeef, encryptedCFB}, /* Testing cipher block chaining */
    {CRYPT_MODE_CBC, TRUE, 0xdeadbeef, encryptedCBC}, /* Testing cipher feedback */
    {CRYPT_MODE_ECB, TRUE, 0xdeadbeef, encryptedECB}, /* Testing electronic codebook */
    {CRYPT_MODE_OFB, FALSE, NTE_BAD_DATA}/* DSSENH does not support Output Feedback cipher mode */
};

static void test_cipher_modes(const struct ciphermode_test *tests, int testLen)
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY pKey = 0;
    HCRYPTHASH hHash;
    const char plainText[] = "Testing block cipher modes.";
    const char dataToHash[] = "GSOC is awesome!";
    unsigned char pbData[36];
    int plainLen = sizeof(plainText), i;
    DWORD mode, dataLen;
    BOOL result;

    /* acquire dss enhanced provider */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    if(!result && GetLastError() == NTE_KEYSET_NOT_DEF)
    {
        skip("DSSENH is currently not available, skipping block cipher mode tests.\n");
        return;
    }
    ok(result, "Expected no errors.\n");

    SetLastError(0xdeadbeef);
    result = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
    ok(result, "Expected creation of a MD5 hash for key derivation.\n");

    result = CryptHashData(hHash, (BYTE *)dataToHash, sizeof(dataToHash), 0);
    ok(result, "Expected data to be added to hash for key derivation.\n");

    /* Derive a CALG_RC2 key, but could be any other encryption cipher */
    result = CryptDeriveKey(hProv, CALG_RC2, hHash, 40 << 16, &pKey);
    ok(result, "Expected a derived key.\n");

    result = CryptDestroyHash(hHash);
    ok(result, "Expected destruction of hash after deriving key.\n");

    /* test block cipher modes */
    for(i = 0; i < testLen; i++)
    {
        SetLastError(0xdeadbeef);
        dataLen = plainLen;
        mode = tests[i].cipherMode;
        memcpy(pbData, plainText, plainLen);

        result = CryptSetKeyParam(pKey, KP_MODE, (BYTE*)&mode, 0);
        if(tests[i].expectedResult)
        {
            ok(result, "Expected setting of KP_MODE, got %x.\n", GetLastError());

            result = CryptEncrypt(pKey, 0, TRUE, 0, pbData, &dataLen, 36);
            ok(result, "Expected data encryption, got %x.\n", GetLastError());

            /* Verify we have the correct encrypted data */
            ok(!memcmp(pbData, tests[i].encrypted, dataLen), "Incorrect encrypted data.\n");

            result = CryptDecrypt(pKey, 0, TRUE, 0, pbData, &dataLen);
            ok(result, "Expected data decryption, got %x.\n", GetLastError());

            /* Verify we have the correct decrypted data */
            ok(!memcmp(pbData, (BYTE *)plainText, dataLen), "Incorrect decrypted data.\n");
        }
        else
        {   /* Expected error */
            ok(!result && GetLastError() == tests[i].expectedError, "Expected %d, got %x.\n",
                tests[i].expectedError, GetLastError());
        }
    }
    result = CryptDestroyKey(pKey);
    ok(result, "Expected no DestroyKey errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");
}


START_TEST(dssenh)
{
    test_acquire_context();
    test_keylength();
    test_hash(hash_data, TESTLEN(hash_data));
    test_data_encryption(encrypt_data, TESTLEN(encrypt_data));
    test_cipher_modes(ciphermode_data, TESTLEN(ciphermode_data));
}
