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
#include "ntstatus.h"
#define WIN32_NO_STATUS
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
        "Expected NTE_BAD_PROV_TYPE, got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, 0, CRYPT_VERIFYCONTEXT);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08lx\n", GetLastError());

    /* flag allows us to delete a keyset, but not of an unknown provider */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, 0, CRYPT_DELETEKEYSET);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08lx\n", GetLastError());

    /* cannot acquire along with PROV_RSA_SIG, not compatible */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, PROV_RSA_SIG, 0);
    ok(!result && GetLastError() == NTE_PROV_TYPE_NO_MATCH,
        "Expected NTE_PROV_TYPE_NO_MATCH, got %08lx\n", GetLastError());

    /* cannot acquire along with MS_DEF_RSA_SIG_PROV_A, not compatible */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_RSA_SIG_PROV_A, PROV_DSS, 0);
    ok(!result && GetLastError() == NTE_KEYSET_NOT_DEF,
        "Expected NTE_KEYSET_NOT_DEF, got %08lx\n", GetLastError());

    /* cannot acquire provider with 0 as Prov Type */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, 0, 0);
    ok(!result && GetLastError() == NTE_BAD_PROV_TYPE,
        "Expected NTE_BAD_PROV_TYPE, got %08lx\n", GetLastError());

    /* test base DSS provider (PROV_DSS) */

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DSS, 0);
    if (!result)
    {
        ok(GetLastError() == NTE_BAD_KEYSET, "Expected NTE_BAD_KEYSET, got %08lx\n", GetLastError());
        SetLastError(0xdeadbeef);
        result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DSS, CRYPT_NEWKEYSET);
    }
    ok(result, "CryptAcquireContextA succeeded\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "CryptReleaseContext failed.\n");

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
    ok(result || GetLastError() == NTE_EXISTS, "Expected no errors or NTE_EXISTS\n");

    if (result)
    {
        result = CryptReleaseContext(hProv, 0);
        ok(result, "Expected release of the provider.\n");
    }

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DSS, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, 0);
    ok(result, "Expected no errors.\n");

    /* test DSS Diffie Hellman provider (PROV_DSS_DH) */

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_DH_PROV_A, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    /* test DSS Enhanced provider (MS_ENH_DSS_DH_PROV_A) */

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    /* test DSS Schannel provider (PROV_DH_SCHANNEL) */

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV_A, PROV_DH_SCHANNEL, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(&hProv, NULL, NULL, PROV_DH_SCHANNEL, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV_A, PROV_DH_SCHANNEL, 0);
    ok(result, "Expected no errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");

    /* failure tests, cannot acquire context because the key container already exists */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV_A, PROV_DH_SCHANNEL, CRYPT_NEWKEYSET);
    ok(!result && GetLastError() == NTE_EXISTS,
        "Expected NTE_EXISTS, got %08lx\n", GetLastError());
}

struct keylength_test {
    ALG_ID algid;
    DWORD flags;
    BOOL expectedResult;
    DWORD expectedError;
    DWORD brokenError;
    int todo_result;
    int todo_error;
};

static const struct keylength_test baseDSS_keylength[] = {
    /* AT_KEYEXCHANGE is not supported by the base DSS provider */
    {AT_KEYEXCHANGE, 448 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {AT_KEYEXCHANGE, 512 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {AT_KEYEXCHANGE, 1024 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {AT_KEYEXCHANGE, 1088 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    /* min 512 max 1024 increment by 64 */
    {AT_SIGNATURE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 512 << 16, TRUE},
    {AT_SIGNATURE, 513 << 16, FALSE, STATUS_INVALID_PARAMETER, NTE_FAIL},
    {AT_SIGNATURE, 768 << 16, TRUE},
    {AT_SIGNATURE, 1024 << 16, TRUE},
    {AT_SIGNATURE, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    /* CALG_DH_EPHEM is not supported by the base DSS provider */
    {CALG_DH_EPHEM, 448 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {CALG_DH_EPHEM, 512 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {CALG_DH_EPHEM, 1024 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {CALG_DH_EPHEM, 1088 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    /* CALG_DH_SF is not supported by the base DSS provider */
    {CALG_DH_SF, 448 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {CALG_DH_SF, 512 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {CALG_DH_SF, 1024 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    {CALG_DH_SF, 1088 << 16, FALSE, NTE_BAD_ALGID, 0, 0, 1},
    /* min 512 max 1024, increment by 64 */
    {CALG_DSS_SIGN, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 512 << 16, TRUE},
    {CALG_DSS_SIGN, 513 << 16, FALSE, STATUS_INVALID_PARAMETER, NTE_FAIL},
    {CALG_DSS_SIGN, 768 << 16, TRUE},
    {CALG_DSS_SIGN, 1024 << 16, TRUE},
    {CALG_DSS_SIGN, 1088 << 16, FALSE, NTE_BAD_FLAGS}
};

static const struct keylength_test dssDH_keylength[] = {
    /* min 512 max 1024, increment by 64 */
    {AT_KEYEXCHANGE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_KEYEXCHANGE, 512 << 16, TRUE, 0, 0, 1},
    {AT_KEYEXCHANGE, 513 << 16, FALSE, NTE_BAD_FLAGS, 0, 0, 1},
    {AT_KEYEXCHANGE, 768 << 16, TRUE, 0, 0, 1},
    {AT_KEYEXCHANGE, 1024 << 16, TRUE, 0, 0, 1},
    {AT_KEYEXCHANGE, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 512 << 16, TRUE},
    {AT_SIGNATURE, 513 << 16, FALSE, STATUS_INVALID_PARAMETER, NTE_FAIL},
    {AT_SIGNATURE, 768 << 16, TRUE},
    {AT_SIGNATURE, 1024 << 16, TRUE},
    {AT_SIGNATURE, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 512 << 16, TRUE, 0, 0, 1},
    {CALG_DH_EPHEM, 513 << 16, FALSE, NTE_BAD_FLAGS, 0, 0, 1},
    {CALG_DH_EPHEM, 768 << 16, TRUE, 0, 0, 1},
    {CALG_DH_EPHEM, 1024 << 16, TRUE, 0, 0, 1},
    {CALG_DH_EPHEM, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 512 << 16, TRUE, 0, 0, 1},
    {CALG_DH_SF, 513 << 16, FALSE, NTE_BAD_FLAGS, 0, 0, 1},
    {CALG_DH_SF, 768 << 16, TRUE, 0, 0, 1},
    {CALG_DH_SF, 1024 << 16, TRUE, 0, 0, 1},
    {CALG_DH_SF, 1088 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 512 << 16, TRUE},
    {CALG_DSS_SIGN, 513 << 16, FALSE, STATUS_INVALID_PARAMETER, NTE_FAIL},
    {CALG_DSS_SIGN, 768 << 16, TRUE},
    {CALG_DSS_SIGN, 1024 << 16, TRUE},
    {CALG_DSS_SIGN, 1088 << 16, FALSE, NTE_BAD_FLAGS}
};

static const struct keylength_test dssENH_keylength[] = {
    /* min 512 max 1024 (AT_KEYEXCHANGE, CALG_DH_EPHEM, CALG_DH_SF max 4096), increment by 64*/
    {AT_KEYEXCHANGE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_KEYEXCHANGE, 512 << 16, TRUE, 0, 0, 1},
    {AT_KEYEXCHANGE, 513 << 16, FALSE, NTE_BAD_FLAGS, 0, 0, 1},
    {AT_KEYEXCHANGE, 768 << 16, TRUE, 0, 0, 1},
    {AT_KEYEXCHANGE, 1024 << 16, TRUE, 0, 0, 1},
    {AT_KEYEXCHANGE, 1088 << 16, TRUE, 0, 0, 1},
    {AT_KEYEXCHANGE, 2048 << 16, TRUE, 0, 0, 1},
    /* Keylength too large - test bot timeout.
    {AT_KEYEXCHANGE, 3072 << 16, TRUE},
    {AT_KEYEXCHANGE, 4096 << 16, TRUE}, */
    {AT_KEYEXCHANGE, 4160 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {AT_SIGNATURE, 512 << 16, TRUE},
    {AT_SIGNATURE, 513 << 16, FALSE, STATUS_INVALID_PARAMETER, NTE_FAIL},
    {AT_SIGNATURE, 768 << 16, TRUE},
    {AT_SIGNATURE, 1024 << 16, TRUE},
    {AT_SIGNATURE, 1032 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 512 << 16, TRUE, 0, 0, 1},
    {CALG_DH_EPHEM, 513 << 16, FALSE, NTE_BAD_FLAGS, 0, 0, 1},
    {CALG_DH_EPHEM, 768 << 16, TRUE, 0, 0, 1},
    {CALG_DH_EPHEM, 1024 << 16, TRUE, 0, 0, 1},
    {CALG_DH_EPHEM, 1040 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_EPHEM, 1088 << 16, TRUE, 0, 0, 1},
    {CALG_DH_EPHEM, 4160 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 512 << 16, TRUE, 0, 0, 1},
    {CALG_DH_SF, 513 << 16, FALSE, NTE_BAD_FLAGS, 0, 0, 1},
    {CALG_DH_SF, 768 << 16, TRUE, 0, 0, 1},
    {CALG_DH_SF, 1024 << 16, TRUE, 0, 0, 1},
    {CALG_DH_SF, 1032 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DH_SF, 1088 << 16, TRUE, 0, 0, 1},
    {CALG_DH_SF, 4160 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 448 << 16, FALSE, NTE_BAD_FLAGS},
    {CALG_DSS_SIGN, 512 << 16, TRUE},
    {CALG_DSS_SIGN, 513 << 16, FALSE, STATUS_INVALID_PARAMETER, NTE_FAIL},
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
        if (tests[i].expectedResult)
        {
            todo_wine_if (tests[i].todo_result) ok(result, "%d: got %08lx\n", i, GetLastError());
            if (result)
            {
                result = CryptDestroyKey(key);
                ok(result, "%d: got %08lx\n", i, GetLastError());
            }
        }
        else
        {
            todo_wine_if (tests[i].todo_result) ok(!result, "%d: got %lx\n", i, GetLastError());
            todo_wine_if (tests[i].todo_error)
                ok(GetLastError() == tests[i].expectedError ||
                   broken(GetLastError() == tests[i].brokenError), "%d: got %08lx\n", i, GetLastError());
        }
    }
}

static void test_keylength(void)
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY key;
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

    result = CryptGenKey(hProv, AT_SIGNATURE, 0, &key);
    ok(result, "Expected no errors.\n");

    result = CryptDestroyKey(key);
    ok(result, "Expected no errors.\n");

    /* perform keylength tests */
    test_keylength_array(hProv, baseDSS_keylength, ARRAY_SIZE(baseDSS_keylength));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of CSP provider.\n");

    /* acquire diffie hellman dss provider */
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    /* perform keylength tests */
    test_keylength_array(hProv, dssDH_keylength, ARRAY_SIZE(dssDH_keylength));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of CSP provider.\n");

    /* acquire enhanced dss provider */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(
        &hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    /* perform keylength tests */
    test_keylength_array(hProv, dssENH_keylength, ARRAY_SIZE(dssENH_keylength));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of CSP provider.\n");

    /* acquire schannel dss provider */
    result = CryptAcquireContextA(
        &hProv, NULL, MS_DEF_DH_SCHANNEL_PROV_A, PROV_DH_SCHANNEL, CRYPT_VERIFYCONTEXT);
    ok(result, "Expected no errors.\n");

    /* perform keylength tests */
    test_keylength_array(hProv, dssENH_keylength, ARRAY_SIZE(dssENH_keylength));

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
        &hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
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
        ok(result && (hashLen == tests[i].hashLen), "Expected %ld hash len, got %ld.Error: %lx\n",
            tests[i].hashLen, hashLen, GetLastError());

        dataLen = 0xdeadbeef;
        result = CryptGetHashParam(hHash, HP_HASHVAL, 0, &dataLen, 0);
        ok(result, "Expected hash value return.\n");
        ok(dataLen == hashLen, "Expected hash length to match.\n");

        hashLen = 0xdeadbeef;
        result = CryptGetHashParam(hHash, HP_HASHVAL, hashValue, &hashLen, 0);
        ok(result, "Expected hash value return.\n");

        ok(dataLen == hashLen, "Expected hash length to match.\n");
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
       minimum length because Win2K's DSSENH provider has a different default key length compared
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
        &hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    if (!result)
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
        if (!result)
        {
            skip("skipping encryption tests\n");
            return;
        }
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
        ok(!memcmp(pbData, tests[i].decrypted, dataLen) ||
           broken(tests[i].algid == CALG_RC4), "Incorrect decrypted data.\n");

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
0x7c,0xa5,0x28,0xda,0xb8,0x81,0x15,0x99, 0xd1,0xd5,0x06,0x1d,0xd3,0x46,0x7e,0xca
};
static const BYTE encryptedCBC[] = {
0x8f,0x7b,0x56,0xeb,0xad,0x4d,0x76,0xc2, 0xd5,0x1d,0xf0,0x60,0x9d,0xde,0x96,0xe8,
0xb7,0x7b,0xeb,0x4b,0xee,0x3f,0xae,0x05, 0x20,0xf5,0xe0,0x75,0xa0,0x1d,0xf9,0x39
};
static const BYTE encryptedECB[] = {
0x8f,0x7b,0x56,0xeb,0xad,0x4d,0x76,0xc2, 0x8b,0xe0,0x4e,0xe4,0x98,0x4f,0xb8,0x3b,
0xf3,0xeb,0x6f,0x0a,0x57,0x91,0xdd,0xc7, 0x34,0x5d,0x4c,0xa3,0x7e,0x97,0xbf,0xee
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
        &hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_VERIFYCONTEXT);
    if (!result)
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
    if (!result)
    {
        skip("skipping cipher mode tests\n");
        return;
    }
    ok(result, "Expected a derived key.\n");

    result = CryptDestroyHash(hHash);
    ok(result, "Expected destruction of hash after deriving key.\n");

    /* the default algorithm is CBC, test that without setting a mode */
    mode = 0xdeadbeef;
    dataLen = sizeof(mode);
    result = CryptGetKeyParam(pKey, KP_MODE, (BYTE*)&mode, &dataLen, 0);
    ok(result, "Expected getting of KP_MODE, got %lx.\n", GetLastError());
    ok(mode == CRYPT_MODE_CBC, "Default mode should be CBC\n");

    memcpy(pbData, plainText, plainLen);
    dataLen = plainLen;
    result = CryptEncrypt(pKey, 0, TRUE, 0, pbData, &dataLen, 36);
    ok(result, "Expected data encryption, got %lx.\n", GetLastError());

    /* Verify we have the correct encrypted data */
    ok(!memcmp(pbData, tests[1].encrypted, dataLen), "Incorrect encrypted data.\n");

    result = CryptDecrypt(pKey, 0, TRUE, 0, pbData, &dataLen);
    ok(result, "Expected data decryption, got %lx.\n", GetLastError());

    /* Verify we have the correct decrypted data */
    ok(!memcmp(pbData, (BYTE *)plainText, dataLen), "Incorrect decrypted data.\n");

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
            ok(result, "Expected setting of KP_MODE, got %lx.\n", GetLastError());

            result = CryptEncrypt(pKey, 0, TRUE, 0, pbData, &dataLen, 36);
            ok(result, "Expected data encryption, got %lx.\n", GetLastError());

            /* Verify we have the correct encrypted data */
            ok(!memcmp(pbData, tests[i].encrypted, dataLen), "Incorrect encrypted data.\n");

            result = CryptDecrypt(pKey, 0, TRUE, 0, pbData, &dataLen);
            ok(result, "Expected data decryption, got %lx.\n", GetLastError());

            /* Verify we have the correct decrypted data */
            ok(!memcmp(pbData, (BYTE *)plainText, dataLen), "Incorrect decrypted data.\n");
        }
        else
        {   /* Expected error */
            ok(!result && GetLastError() == tests[i].expectedError, "Expected %ld, got %lx.\n",
                tests[i].expectedError, GetLastError());
        }
    }
    result = CryptDestroyKey(pKey);
    ok(result, "Expected no DestroyKey errors.\n");

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Expected release of the provider.\n");
}

struct signature_test {
    const BYTE *privateKey;
    DWORD keyLen;
    BYTE* signData;
    DWORD dataLen;
};

static const char dataToSign1[] = "Put your hands up for Cryptography :)";
static const char dataToSign2[] = "With DSSENH implemented, applications requiring it will now work.";
static const char dataToSign3[] = "";

static const BYTE AT_SIGNATURE_PrivateKey[] = {
0x07,0x02,0x00,0x00,0x00,0x22,0x00,0x00, 0x44,0x53,0x53,0x32,0x00,0x04,0x00,0x00,
0x01,0xd1,0xfc,0x7a,0x70,0x53,0xb2,0x48, 0x70,0x23,0x19,0x1f,0x3c,0xe1,0x26,0x14,
0x7e,0x9f,0x0f,0x7f,0x33,0x5e,0x2b,0xf7, 0xca,0x01,0x74,0x8c,0xb4,0xfd,0xf6,0x44,
0x95,0x35,0x56,0xaa,0x4d,0x62,0x48,0xe2, 0xd1,0xa2,0x7e,0x6e,0xeb,0xd6,0xcc,0x7c,
0xe8,0xfd,0x21,0x9a,0xa2,0xfd,0x7a,0x9d, 0x1a,0x38,0x69,0x87,0x39,0x5a,0x91,0xc0,
0x52,0x2b,0x9f,0x2a,0x54,0x78,0x37,0x82, 0x9a,0x70,0x57,0xab,0xec,0x93,0x8e,0xac,
0x73,0x04,0xe8,0x53,0x72,0x72,0x32,0xc6, 0xcb,0xef,0x47,0x98,0x3c,0x56,0x49,0x62,
0xcb,0xbb,0xe7,0x34,0x84,0xa6,0x72,0x3a, 0xbe,0x26,0x46,0x86,0xca,0xcb,0x35,0x62,
0x4f,0x19,0x18,0x0b,0xb0,0x78,0xae,0xd5, 0x42,0xdf,0x26,0xdb,0x85,0x63,0x77,0x85,
0x01,0x3b,0x32,0xbe,0x5c,0xf8,0x05,0xc8, 0xde,0x17,0x7f,0xb9,0x03,0x82,0xfa,0xf1,
0x9e,0x32,0x73,0xfa,0x8d,0xea,0xa3,0x30, 0x48,0xe2,0xdf,0x5a,0xcb,0x83,0x3d,0xff,
0x56,0xe9,0xc0,0x94,0xf8,0x6d,0xb3,0xaf, 0x4a,0x97,0xb9,0x43,0x0e,0xd4,0x28,0x98,
0x57,0x2e,0x3a,0xca,0xde,0x6f,0x45,0x0d, 0xfb,0x58,0xec,0x78,0x34,0x2e,0x46,0x4d,
0xfe,0x98,0x02,0xbb,0xef,0x07,0x1a,0x13, 0xb6,0xc2,0x2c,0x06,0xd9,0x0c,0xc4,0xb0,
0x4c,0x3a,0xfc,0x01,0x63,0xb5,0x5a,0x5d, 0x2d,0x9c,0x47,0x04,0x67,0x51,0xf2,0x52,
0xf5,0x82,0x36,0xeb,0x6e,0x66,0x58,0x4c, 0x10,0x2c,0x29,0x72,0x4a,0x6f,0x6b,0x6c,
0xe0,0x93,0x31,0x42,0xf6,0xda,0xfa,0x5b, 0x22,0x43,0x9b,0x1a,0x98,0x71,0xe7,0x41,
0x74,0xe9,0x12,0xa4,0x1f,0x27,0x0a,0x63, 0x94,0x49,0xd7,0xad,0xa5,0xc4,0x5c,0xc3,
0xc9,0x70,0xb3,0x7b,0x16,0xb6,0x1d,0xd4, 0x09,0xc4,0x9a,0x46,0x2d,0x0e,0x75,0x07,
0x31,0x7b,0xed,0x45,0xcd,0x99,0x84,0x14, 0xf1,0x01,0x00,0x00,0x93,0xd5,0xa3,0xe4,
0x34,0x05,0xeb,0x98,0x3b,0x5f,0x2f,0x11, 0xa4,0xa5,0xc4,0xff,0xfb,0x22,0x7c,0x54
};

static const BYTE DSS_SIGN_PrivateKey[] = {
0x07,0x02,0x00,0x00,0x00,0x22,0x00,0x00, 0x44,0x53,0x53,0x32,0x00,0x04,0x00,0x00,
0xf7,0x9e,0x89,0xa2,0xcd,0x0b,0x61,0xe0, 0xa3,0xe5,0x86,0x6b,0x04,0x98,0x80,0x9c,
0x36,0xc2,0x76,0x4e,0x22,0xd5,0x21,0xaa, 0x03,0x59,0xf4,0x95,0xb2,0x11,0x1f,0xa0,
0xc5,0xfc,0xbe,0x5d,0x1f,0x2e,0xf4,0x36, 0x40,0x48,0x81,0x51,0xb4,0x25,0x86,0xe0,
0x98,0xc8,0x4d,0xa0,0x08,0x99,0xa1,0x00, 0x45,0x1b,0x75,0x6b,0x0d,0x3e,0x7d,0x13,
0xd7,0x23,0x32,0x08,0xf4,0xeb,0x27,0x9e, 0xe9,0x05,0x5d,0xac,0xc8,0xd7,0x62,0x13,
0x43,0x2a,0x69,0x65,0xdc,0xe6,0x52,0xf9, 0x6a,0xe8,0x07,0xcf,0x3e,0xf8,0xc9,0x1d,
0x8e,0xdf,0x4e,0x9a,0xd1,0x48,0xf2,0xda, 0x9e,0xfa,0x92,0x5f,0x6d,0x57,0xf2,0xa4,
0x5f,0x60,0xce,0x92,0x7a,0x80,0x39,0x21, 0x9d,0x4d,0x3a,0x60,0x76,0x4c,0x2f,0xc0,
0xd3,0xf4,0x14,0x03,0x03,0x05,0xa9,0x0c, 0x57,0x72,0x4f,0x60,0x3c,0xe9,0x09,0x54,
0x0c,0x2a,0x56,0xda,0x30,0xb6,0x2e,0x6a, 0x96,0x7f,0x4a,0x8f,0x83,0x0a,0xb9,0x5c,
0xff,0x84,0xfa,0x0e,0x85,0x81,0x46,0xe9, 0x1c,0xbb,0x78,0x1d,0x78,0x25,0x00,0x8c,
0x78,0x56,0x68,0xe4,0x06,0x37,0xcc,0xc7, 0x22,0x27,0xee,0x0e,0xf8,0xca,0xfc,0x72,
0x0e,0xd6,0xe6,0x90,0x30,0x66,0x22,0xe2, 0xa2,0xbf,0x2e,0x35,0xbc,0xe7,0xd6,0x24,
0x6a,0x3d,0x06,0xe8,0xe2,0xbe,0x96,0xcc, 0x9a,0x08,0x06,0xb5,0x44,0x83,0xb0,0x7b,
0x70,0x7b,0x2d,0xc3,0x46,0x9a,0xc5,0x6b, 0xd9,0xde,0x9a,0x24,0xc9,0xea,0xf5,0x28,
0x69,0x8a,0x17,0xca,0xdf,0xc4,0x0e,0xa3, 0x08,0x22,0x99,0xd2,0x27,0xdc,0x9b,0x08,
0x54,0x4a,0xf9,0xb1,0x74,0x3a,0x9d,0xd9, 0xc2,0x82,0x21,0xf5,0x97,0x04,0x90,0x37,
0xda,0xd9,0xdc,0x19,0xad,0x83,0xcd,0x35, 0xb0,0x4e,0x06,0x68,0xd1,0x69,0x7e,0x73,
0x93,0xbe,0xa5,0x05,0xb3,0xcc,0xd2,0x51, 0x3c,0x00,0x00,0x00,0x16,0xe1,0xac,0x17,
0xdc,0x68,0xae,0x03,0xad,0xf7,0xb9,0xca, 0x0d,0xca,0x27,0xef,0x76,0xda,0xe5,0xcb
};

static const struct signature_test dssSign_data[] = {
    {AT_SIGNATURE_PrivateKey, sizeof(AT_SIGNATURE_PrivateKey), (BYTE *)dataToSign1, sizeof(dataToSign1)},
    {AT_SIGNATURE_PrivateKey, sizeof(AT_SIGNATURE_PrivateKey), (BYTE *)dataToSign2, sizeof(dataToSign2)},
    {AT_SIGNATURE_PrivateKey, sizeof(AT_SIGNATURE_PrivateKey), (BYTE *)dataToSign3, sizeof(dataToSign3)},
    {DSS_SIGN_PrivateKey, sizeof(DSS_SIGN_PrivateKey), (BYTE *)dataToSign1, sizeof(dataToSign1)},
    {DSS_SIGN_PrivateKey, sizeof(DSS_SIGN_PrivateKey), (BYTE *)dataToSign2, sizeof(dataToSign2)},
    {DSS_SIGN_PrivateKey, sizeof(DSS_SIGN_PrivateKey), (BYTE *)dataToSign3, sizeof(dataToSign3)}
};

static void test_signhash_array(HCRYPTPROV hProv, const struct signature_test *tests, int testLen)
{
    HCRYPTHASH hHash1, hHash2;
    HCRYPTKEY privKey = 0, pubKey = 0;
    BYTE pubKeyBuffer[512];
    BYTE signValue1[40], signValue2[40];
    BYTE hashValue1[40], hashValue2[40];
    DWORD hashLen1, hashLen2, pubKeyLen;
    DWORD dataLen1, dataLen2;
    BOOL result;
    int i;

    for (i = 0; i < testLen; i++)
    {
        DWORD signLen1 = tests[i].dataLen;
        DWORD signLen2 = tests[i].dataLen;

        /* Get a private key of array specified ALG_ID */
        result = CryptImportKey(hProv, tests[i].privateKey, tests[i].keyLen, 0, 0, &privKey);
        ok(result, "Failed to imported key, got %lx\n", GetLastError());

        /* Create hash object and add data for signature 1 */
        result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash1);
        ok(result, "Failed to create a hash, got %lx\n", GetLastError());

        result = CryptHashData(hHash1, tests[i].signData, signLen1, 0);
        ok(result, "Failed to add data to hash, got %lx\n", GetLastError());

        /* Create hash object and add data for signature 2 */
        result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash2);
        ok(result, "Failed to create a hash, got %lx\n", GetLastError());

        result = CryptHashData(hHash2, tests[i].signData, signLen2, 0);
        ok(result, "Failed to add data to hash, got %lx\n", GetLastError());

        /* Acquire hash length and hash value */
        dataLen1 = sizeof(DWORD);
        result = CryptGetHashParam(hHash1, HP_HASHSIZE, (BYTE *)&hashLen1, &dataLen1, 0);
        ok(result, "Failed to get hash length, got %lx\n", GetLastError());

        result = CryptGetHashParam(hHash1, HP_HASHVAL, hashValue1, &hashLen1, 0);
        ok(result, "Failed to return hash value.\n");

        dataLen2 = sizeof(DWORD);
        result = CryptGetHashParam(hHash2, HP_HASHSIZE, (BYTE *)&hashLen2, &dataLen2, 0);
        ok(result, "Failed to get hash length, got %lx\n", GetLastError());

        result = CryptGetHashParam(hHash2, HP_HASHVAL, hashValue2, &hashLen2, 0);
        ok(result, "Failed to return hash value.\n");

        /* Compare hashes to ensure they are the same */
        ok(hashLen1 ==  hashLen2, "Hash lengths were not the same.\n");
        ok(!memcmp(hashValue1, hashValue2, hashLen2), "Hashes were not identical.\n");

        /* Sign hash 1 */
        signLen1 = 0;
        result = CryptSignHashA(hHash1, AT_SIGNATURE, NULL, 0, NULL, &signLen1);
        ok(result, "Failed to get signature length, got %lx\n", GetLastError());
        ok(signLen1 == 40, "Expected a 40-byte signature, got %ld\n", signLen1);

        result = CryptSignHashA(hHash1, AT_SIGNATURE, NULL, 0, signValue1, &signLen1);
        ok(result, "Failed to sign hash, got %lx\n", GetLastError());

        /* Sign hash 2 */
        signLen2 = 0;
        result = CryptSignHashA(hHash2, AT_SIGNATURE, NULL, 0, NULL, &signLen2);
        ok(result, "Failed to get signature length, got %lx\n", GetLastError());
        ok(signLen2 == 40, "Expected a 40-byte signature, got %ld\n", signLen2);

        result = CryptSignHashA(hHash2, AT_SIGNATURE, NULL, 0, signValue2, &signLen2);
        ok(result, "Failed to sign hash2, got %lx\n", GetLastError());

        /* Compare signatures to ensure they are both different, because every DSS signature
           should be different even if the input hash data is identical */
        ok(memcmp(signValue1, signValue2, signLen2), "Expected two different signatures from "
            "the same hash input.\n");

        result = CryptExportKey(privKey, 0, PUBLICKEYBLOB, 0, NULL, &pubKeyLen);
        ok(result, "Failed to acquire public key length, got %lx\n", GetLastError());

        /* Export the public key */
        result = CryptExportKey(privKey, 0, PUBLICKEYBLOB, 0, pubKeyBuffer, &pubKeyLen);
        ok(result, "Failed to export public key, got %lx\n", GetLastError());

        result = CryptDestroyHash(hHash1);
        ok(result, "Failed to destroy hash1, got %lx\n", GetLastError());
        result = CryptDestroyHash(hHash2);
        ok(result, "Failed to destroy hash2, got %lx\n", GetLastError());

        /* Destroy the private key */
        result = CryptDestroyKey(privKey);
        ok(result, "Failed to destroy private key, got %lx\n", GetLastError());

        /* Import the public key we obtained earlier */
        result = CryptImportKey(hProv, pubKeyBuffer, pubKeyLen, 0, 0, &pubKey);
        ok(result, "Failed to import public key, got %lx\n", GetLastError());

        result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash1);
        ok(result, "Failed to create hash, got %lx\n", GetLastError());

        /* Hash the data to compare with the signed hash */
        result = CryptHashData(hHash1, tests[i].signData, tests[i].dataLen, 0);
        ok(result, "Failed to add data to hash1, got %lx\n", GetLastError());

        /* Verify signed hash 1 */
        result = CryptVerifySignatureA(hHash1, signValue1, sizeof(signValue1), pubKey, NULL, 0);
        if (!result)
        {
            skip("skipping sign tests\n");
            return;
        }
        ok(result, "Failed to verify signature, got %lx\n", GetLastError());

        result = CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash2);
        ok(result, "Failed to create hash, got %lx\n", GetLastError());

        /* Hash the data to compare with the signed hash */
        result = CryptHashData(hHash2, tests[i].signData, tests[i].dataLen, 0);
        ok(result, "Failed to add data to hash2, got %lx\n", GetLastError());

        /* Verify signed hash 2 */
        result = CryptVerifySignatureA(hHash2, signValue2, sizeof(signValue2), pubKey, NULL, 0);
        ok(result, "Failed to verify signature, got %lx\n", GetLastError());

        result = CryptDestroyHash(hHash1);
        ok(result, "Failed to destroy hash1, got %lx\n", GetLastError());
        result = CryptDestroyHash(hHash2);
        ok(result, "Failed to destroy hash2, got %lx\n", GetLastError());

        /* Destroy the public key */
        result = CryptDestroyKey(pubKey);
        ok(result, "Failed to destroy public key, got %lx\n", GetLastError());
    }
}

static void test_verify_signature(void)
{
    HCRYPTPROV hProv = 0;
    BOOL result;

    /* acquire base dss provider */
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, 0);
    if(!result)
    {
        skip("DSSENH is currently not available, skipping signature verification tests.\n");
        return;
    }
    ok(result, "Failed to acquire CSP.\n");

    test_signhash_array(hProv, dssSign_data, ARRAY_SIZE(dssSign_data));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Failed to release CSP provider.\n");

    /* acquire diffie hellman dss provider */
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_DH_PROV_A, PROV_DSS_DH, 0);
    ok(result, "Failed to acquire CSP.\n");

    test_signhash_array(hProv, dssSign_data, ARRAY_SIZE(dssSign_data));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Failed to release CSP provider.\n");

    /* acquire enhanced dss provider */
    SetLastError(0xdeadbeef);
    result = CryptAcquireContextA(&hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, 0);
    ok(result, "Failed to acquire CSP.\n");

    test_signhash_array(hProv, dssSign_data, ARRAY_SIZE(dssSign_data));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Failed to release CSP provider.\n");

    /* acquire schannel dss provider */
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DH_SCHANNEL_PROV_A, PROV_DH_SCHANNEL, 0);
    ok(result, "Failed to acquire CSP.\n");

    test_signhash_array(hProv, dssSign_data, ARRAY_SIZE(dssSign_data));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Failed to release CSP provider.\n");
}

struct keyExchange_test {
    ALG_ID algid;
    const BYTE *primeNum;
    int primeLen;
    const BYTE *generatorNum;
    int generatorLen;
};

/* AT_KEYEXCHANGE interprets to CALG_DH_SF by the DSSENH cryptographic service provider */
static const BYTE primeAT_KEYEXCHANGE[] = {
0x65,0x28,0x24,0xd8,0xbe,0x3f,0x95,0x93, 0x3c,0x4d,0x1b,0x51,0xe1,0x89,0x9a,0x90,
0x5e,0xa0,0x6c,0x25,0xf0,0xb5,0x93,0x98, 0xba,0x76,0x9d,0x54,0x78,0xf6,0xdc,0x04,
0xe1,0xd0,0x87,0x8f,0xa0,0xe4,0x2f,0xb4, 0x09,0x78,0x24,0xbf,0xc8,0x7f,0x59,0xf4,
0x07,0xee,0x07,0x20,0x1b,0x2d,0x54,0x2a, 0xb5,0xb4,0x8f,0x8c,0x33,0x73,0xec,0xaf
};

static const BYTE generatorAT_KEYEXCHANGE[] = {
0xdc,0x62,0x20,0xe7,0x36,0xa2,0xa6,0xef, 0x77,0x91,0xa8,0xa3,0x6d,0x60,0x70,0x0d,
0x1d,0x79,0xb1,0xbe,0xa8,0x87,0x69,0x39, 0x29,0xaa,0x54,0x27,0x05,0xe6,0x6f,0xa5,
0xde,0x82,0x00,0x5d,0x87,0x1f,0x84,0xf7, 0x40,0xec,0x6f,0x15,0x64,0x02,0x0d,0xb8,
0x50,0x48,0x94,0x66,0xb2,0x7d,0xbd,0xf2, 0x66,0xf8,0x40,0x62,0x94,0xbf,0x24,0x3b
};

static const BYTE primeCALG_DH_EPHEM[] = {
0x17,0x99,0xa9,0x79,0x31,0xb9,0x05,0xdd, 0x7f,0xf0,0x02,0x11,0x4d,0x0d,0xc3,0x81,
0x8b,0x41,0x50,0x41,0x5f,0x07,0xe6,0x8d, 0x02,0xf9,0xaa,0x86,0x2a,0x07,0x07,0xea,
0x0a,0x75,0xed,0x96,0xa7,0x85,0xee,0xac, 0xb1,0x71,0xbd,0x57,0x48,0xbd,0x41,0x0b,
0xde,0x34,0xe2,0xba,0x5b,0x55,0x64,0x77, 0x84,0xfa,0x96,0xd1,0xaf,0x79,0x49,0x9d
};

static const BYTE generatorCALG_DH_EPHEM[] = {
0xc7,0x64,0x56,0xde,0xf7,0xb4,0xd3,0xd8, 0xa2,0xd4,0x12,0x2d,0x54,0x5c,0x54,0xc8,
0x04,0x11,0x88,0x14,0x6c,0x9f,0x88,0x41, 0x82,0x93,0x32,0xb1,0x82,0x84,0x5b,0x07,
0x55,0x13,0x82,0x7a,0x64,0x7b,0x12,0x09, 0xe2,0xa0,0x28,0x51,0xf4,0x7a,0xd9,0x26,
0x86,0x95,0x5f,0xc0,0x9a,0x25,0xc2,0x7e, 0x91,0x14,0xdd,0x3c,0x4e,0x86,0x4f,0x6f
};

static const BYTE primeCALG_DH_SF[] = {
0x85,0xb8,0xa5,0x4a,0xcf,0x2b,0x7c,0x61, 0xb2,0x06,0x93,0x8a,0x87,0x37,0x58,0xb0,
0x8d,0xc7,0x2a,0xa7,0x7f,0x0d,0x74,0xf9, 0x3a,0x7e,0xbc,0xab,0x3a,0x54,0xe4,0x11,
0x69,0x6f,0xcd,0xea,0xad,0x32,0x44,0x4f, 0xee,0x54,0x69,0x8c,0x9c,0x3b,0x87,0x27,
0x36,0x70,0x06,0xf3,0x4e,0xde,0x7f,0x9d, 0xa6,0xf2,0xad,0x43,0x90,0xdd,0xb5,0x9b
};

static const BYTE generatorCALG_DH_SF[] = {
0xea,0xdc,0xe0,0xbb,0x60,0x26,0xc6,0xb3, 0x93,0x6f,0x61,0xe6,0x7e,0xe2,0xee,0xd6,
0xdb,0x3d,0xca,0xa8,0x31,0x46,0x8f,0x5d, 0xb4,0xaa,0x83,0xd3,0x52,0x10,0xcd,0xfb,
0xfd,0xfc,0x14,0x89,0x0c,0xde,0xcf,0x54, 0x1d,0x05,0x8c,0xbe,0x4a,0xe4,0x37,0xb4,
0xc0,0x15,0x75,0xc5,0xa2,0xfc,0x99,0xfc, 0xad,0x63,0xcb,0x7c,0xb8,0x20,0xdc,0x2b
};

static const struct keyExchange_test baseDSSkey_data[] = {
    /* Cannot exchange keys using the base DSS provider, except on WinNT4 */
    {AT_KEYEXCHANGE, primeAT_KEYEXCHANGE, sizeof(primeAT_KEYEXCHANGE), generatorAT_KEYEXCHANGE,
        sizeof(generatorAT_KEYEXCHANGE)},
    {CALG_DH_EPHEM, primeCALG_DH_EPHEM, sizeof(primeCALG_DH_EPHEM), generatorCALG_DH_EPHEM,
        sizeof(generatorCALG_DH_EPHEM)},
    {CALG_DH_SF, primeCALG_DH_SF, sizeof(generatorCALG_DH_SF), generatorCALG_DH_SF,
        sizeof(generatorCALG_DH_SF)}
};

static const struct keyExchange_test dssDHkey_data[] = {
    {AT_KEYEXCHANGE, primeAT_KEYEXCHANGE, sizeof(primeAT_KEYEXCHANGE), generatorAT_KEYEXCHANGE,
        sizeof(generatorAT_KEYEXCHANGE)},
    {CALG_DH_EPHEM, primeCALG_DH_EPHEM, sizeof(primeCALG_DH_EPHEM), generatorCALG_DH_EPHEM,
        sizeof(generatorCALG_DH_EPHEM)},
    {CALG_DH_SF, primeCALG_DH_SF, sizeof(generatorCALG_DH_SF), generatorCALG_DH_SF,
        sizeof(generatorCALG_DH_SF)}
};

static void test_keyExchange_baseDSS(HCRYPTPROV hProv, const struct keyExchange_test *tests, int testLen)
{
    HCRYPTKEY privKey1 = 0, privKey2 = 0;
    HCRYPTKEY sessionKey1 = 0, sessionKey2 = 0;
    const char plainText[] = "Testing shared key.";
    unsigned char pbData1[36];
    unsigned char pbData2[36];
    BYTE pubKeyBuffer1[512], pubKeyBuffer2[512];
    DWORD pubKeyLen1, pubKeyLen2, dataLen;
    DATA_BLOB Prime, Generator;
    int plainLen = sizeof(plainText), i;
    ALG_ID algid;
    BOOL result;

    for(i = 0; i < testLen; i++)
    {
        SetLastError(0xdeadbeef);

        /* Create the data blobs and the prime and generator */
        Prime.cbData = tests[i].primeLen;
        Prime.pbData = (BYTE *)tests[i].primeNum;
        Generator.cbData = tests[i].generatorLen;
        Generator.pbData = (BYTE *)tests[i].generatorNum;

        /* Generate key exchange keys for user1 and user2 */
        result = CryptGenKey(hProv, tests[i].algid, 512 << 16 | CRYPT_PREGEN, &privKey1);
        if (!result)
        {
            skip("skipping key exchange tests\n");
            return;
        }
        ok(!result && GetLastError() == NTE_BAD_ALGID,
           "Expected NTE_BAD_ALGID, got %lx\n", GetLastError());

        result = CryptGenKey(hProv, tests[i].algid, 512 << 16 | CRYPT_PREGEN, &privKey2);
        ok(!result && GetLastError() == NTE_BAD_ALGID,
           "Expected NTE_BAD_ALGID, got %lx\n", GetLastError());

        /* Set the prime and generator values, which are agreed upon */
        result = CryptSetKeyParam(privKey1, KP_P, (BYTE *)&Prime, 0);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptSetKeyParam(privKey2, KP_P, (BYTE *)&Prime, 0);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptSetKeyParam(privKey1, KP_G, (BYTE *)&Generator, 0);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptSetKeyParam(privKey2, KP_G, (BYTE *)&Generator, 0);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        /* Generate the secret value for user1 and user2 */
        result = CryptSetKeyParam(privKey1, KP_X, NULL, 0);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptSetKeyParam(privKey2, KP_X, NULL, 0);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        /* Acquire required size for the public keys */
        result = CryptExportKey(privKey1, 0, PUBLICKEYBLOB, 0, NULL, &pubKeyLen1);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptExportKey(privKey2, 0, PUBLICKEYBLOB, 0, NULL, &pubKeyLen2);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        /* Export public key which will be calculated into the shared key */
        result = CryptExportKey(privKey1, 0, PUBLICKEYBLOB, 0, pubKeyBuffer1, &pubKeyLen1);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptExportKey(privKey2, 0, PUBLICKEYBLOB, 0, pubKeyBuffer2, &pubKeyLen2);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        /* Import the public key and convert it into a shared key */
        result = CryptImportKey(hProv, pubKeyBuffer2, pubKeyLen2, privKey1, 0, &sessionKey1);
        ok((!result && GetLastError() == ERROR_INVALID_PARAMETER) ||
            broken(!result && GetLastError() == NTE_BAD_DATA) || /* Vista.64 */
            broken(!result && GetLastError() == NTE_BAD_TYPE) || /* Win2K-W2K8, Win7.64 */
            broken(!result && GetLastError() == NTE_BAD_ALGID),  /* W7SP164 (32 bit dssenh) */
            "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptImportKey(hProv, pubKeyBuffer1, pubKeyLen1, privKey2, 0, &sessionKey2);
        ok((!result && GetLastError() == ERROR_INVALID_PARAMETER) ||
            broken(!result && GetLastError() == NTE_BAD_DATA) || /* Win 7 */
            broken(!result && GetLastError() == NTE_BAD_TYPE) || /* Win2K-W2K8, Win7.64 */
            broken(!result && GetLastError() == NTE_BAD_ALGID),  /* W7SP164 (32 bit dssenh) */
            "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        /* Set the shared key parameters to matching type */
        algid = CALG_RC4;
        result = CryptSetKeyParam(sessionKey1, KP_ALGID, (BYTE *)&algid, 0);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        algid = CALG_RC4;
        result = CryptSetKeyParam(sessionKey2, KP_ALGID, (BYTE *)&algid, 0);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        /* Encrypt some data and verify we are getting the same output */
        memcpy(pbData1, plainText, plainLen);
        dataLen = plainLen;

        result = CryptEncrypt(sessionKey1, 0, TRUE, 0, pbData1, &dataLen, 36);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptDecrypt(sessionKey2, 0, TRUE, 0, pbData1, &dataLen);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        ok(!memcmp(pbData1, (BYTE *)plainText, sizeof(plainText)), "Incorrect decrypted data.\n");

        memcpy(pbData2, plainText, plainLen);
        dataLen = plainLen;

        result = CryptEncrypt(sessionKey2, 0, TRUE, 0, pbData2, &dataLen, 36);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptDecrypt(sessionKey1, 0, TRUE, 0, pbData2, &dataLen);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        ok(!memcmp(pbData1, pbData2, dataLen), "Decrypted data is not identical.\n");

        /* Destroy all user keys */
        result = CryptDestroyKey(sessionKey1);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptDestroyKey(sessionKey2);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptDestroyKey(privKey1);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());

        result = CryptDestroyKey(privKey2);
        ok(!result && GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %lx\n", GetLastError());
    }
}

static void test_keyExchange_dssDH(HCRYPTPROV hProv, const struct keyExchange_test *tests, int testLen)
{
    HCRYPTKEY privKey1 = 0, privKey2 = 0;
    HCRYPTKEY sessionKey1 = 0, sessionKey2 = 0;
    const char plainText[] = "Testing shared key.";
    unsigned char pbData1[36];
    unsigned char pbData2[36];
    BYTE pubKeyBuffer1[512], pubKeyBuffer2[512];
    DWORD pubKeyLen1, pubKeyLen2, dataLen;
    DATA_BLOB Prime, Generator;
    int plainLen = sizeof(plainText), i;
    ALG_ID algid;
    BOOL result;

    for(i = 0; i < testLen; i++)
    {
        SetLastError(0xdeadbeef);

        /* Create the data blobs and the prime and generator */
        Prime.cbData = tests[i].primeLen;
        Prime.pbData = (BYTE *)tests[i].primeNum;
        Generator.cbData = tests[i].generatorLen;
        Generator.pbData = (BYTE *)tests[i].generatorNum;

        /* Generate key exchange keys for user1 and user2 */
        result = CryptGenKey(hProv, tests[i].algid, 512 << 16 | CRYPT_PREGEN, &privKey1);
        if (!result)
        {
            skip("skipping key exchange tests\n");
            return;
        }
        ok(result, "Failed to generate a key for user1, got %lx\n", GetLastError());

        result = CryptGenKey(hProv, tests[i].algid, 512 << 16 | CRYPT_PREGEN, &privKey2);
        ok(result, "Failed to generate a key for user2, got %lx\n", GetLastError());

        /* Set the prime and generator values, which are agreed upon */
        result = CryptSetKeyParam(privKey1, KP_P, (BYTE *)&Prime, 0);
        ok(result, "Failed to set prime for user 1's key, got %lx\n", GetLastError());

        result = CryptSetKeyParam(privKey2, KP_P, (BYTE *)&Prime, 0);
        ok(result, "Failed to set prime for user 2's key, got %lx\n", GetLastError());

        result = CryptSetKeyParam(privKey1, KP_G, (BYTE *)&Generator, 0);
        ok(result, "Failed to set generator for user 1's key, got %lx\n", GetLastError());

        result = CryptSetKeyParam(privKey2, KP_G, (BYTE *)&Generator, 0);
        ok(result, "Failed to set generator for user 2's key, got %lx\n", GetLastError());

        /* Generate the secret value for user1 and user2 */
        result = CryptSetKeyParam(privKey1, KP_X, NULL, 0);
        ok(result, "Failed to set secret value for user 1, got %lx\n", GetLastError());

        result = CryptSetKeyParam(privKey2, KP_X, NULL, 0);
        ok(result, "Failed to set secret value for user 2, got %lx\n", GetLastError());

        /* Acquire required size for the public keys */
        result = CryptExportKey(privKey1, 0, PUBLICKEYBLOB, 0, NULL, &pubKeyLen1);
        ok(result, "Failed to acquire public key length for user 1, got %lx\n", GetLastError());

        result = CryptExportKey(privKey2, 0, PUBLICKEYBLOB, 0, NULL, &pubKeyLen2);
        ok(result, "Failed to acquire public key length for user 2, got %lx\n", GetLastError());

        /* Export public key which will be calculated into the shared key */
        result = CryptExportKey(privKey1, 0, PUBLICKEYBLOB, 0, pubKeyBuffer1, &pubKeyLen1);
        ok(result, "Failed to export public key for user 1, got %lx\n", GetLastError());

        result = CryptExportKey(privKey2, 0, PUBLICKEYBLOB, 0, pubKeyBuffer2, &pubKeyLen2);
        ok(result, "Failed to export public key for user 2, got %lx\n", GetLastError());

        /* Import the public key and convert it into a shared key */
        result = CryptImportKey(hProv, pubKeyBuffer2, pubKeyLen2, privKey1, 0, &sessionKey1);
        ok(result, "Failed to import key for user 1, got %lx\n", GetLastError());

        result = CryptImportKey(hProv, pubKeyBuffer1, pubKeyLen1, privKey2, 0, &sessionKey2);
        ok(result, "Failed to import key for user 2, got %lx\n", GetLastError());

        /* Set the shared key parameters to matching cipher type */
        algid = CALG_3DES;
        result = CryptSetKeyParam(sessionKey1, KP_ALGID, (BYTE *)&algid, 0);
        ok(result, "Failed to set session key for user 1, got %lx\n", GetLastError());

        algid = CALG_3DES;
        result = CryptSetKeyParam(sessionKey2, KP_ALGID, (BYTE *)&algid, 0);
        ok(result, "Failed to set session key for user 2, got %lx\n", GetLastError());

        /* Encrypt some data and verify we are getting the correct output */
        memcpy(pbData1, plainText, plainLen);
        dataLen = plainLen;

        result = CryptEncrypt(sessionKey1, 0, TRUE, 0, pbData1, &dataLen, 36);
        ok(result, "Failed to encrypt data, got %lx.\n", GetLastError());

        result = CryptDecrypt(sessionKey2, 0, TRUE, 0, pbData1, &dataLen);
        ok(result, "Failed to decrypt data, got %lx.\n", GetLastError());

        ok(!memcmp(pbData1, (BYTE *)plainText, sizeof(plainText)), "Incorrect decrypted data.\n");

        memcpy(pbData2, plainText, plainLen);
        dataLen = plainLen;

        result = CryptEncrypt(sessionKey2, 0, TRUE, 0, pbData2, &dataLen, 36);
        ok(result, "Failed to encrypt data, got %lx.\n", GetLastError());

        result = CryptDecrypt(sessionKey1, 0, TRUE, 0, pbData2, &dataLen);
        ok(result, "Failed to decrypt data, got %lx.\n", GetLastError());

        ok(!memcmp(pbData1, pbData2, dataLen), "Decrypted data is not identical.\n");

        /* Destroy all user keys */
        result = CryptDestroyKey(sessionKey1);
        ok(result, "Failed to destroy session key 1, got %lx\n", GetLastError());

        result = CryptDestroyKey(sessionKey2);
        ok(result, "Failed to destroy session key 2, got %lx\n", GetLastError());

        result = CryptDestroyKey(privKey1);
        ok(result, "Failed to destroy key private key 1, got %lx\n", GetLastError());

        result = CryptDestroyKey(privKey2);
        ok(result, "Failed to destroy key private key 2, got %lx\n", GetLastError());
    }
}

static void test_key_exchange(void)
{
    HCRYPTPROV hProv = 0;
    BOOL result;

    /* acquire base dss provider */
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, CRYPT_VERIFYCONTEXT);
    if(!result)
    {
        skip("DSSENH is currently not available, skipping shared key tests.\n");
        return;
    }
    ok(result, "Failed to acquire CSP.\n");

    test_keyExchange_baseDSS(hProv, baseDSSkey_data, ARRAY_SIZE(baseDSSkey_data));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Failed to release CSP provider.\n");

    /* acquire diffie hellman dss provider */
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DSS_DH_PROV_A, PROV_DSS_DH,
        CRYPT_VERIFYCONTEXT);
    ok(result, "Failed to acquire CSP.\n");

    test_keyExchange_dssDH(hProv,  dssDHkey_data, ARRAY_SIZE(dssDHkey_data));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Failed to release CSP provider.\n");

    /* acquire enhanced dss provider */
    result = CryptAcquireContextA(&hProv, NULL, MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH,
        CRYPT_VERIFYCONTEXT);
    ok(result, "Failed to acquire CSP.\n");

    test_keyExchange_dssDH(hProv, dssDHkey_data, ARRAY_SIZE(dssDHkey_data));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Failed to release CSP provider.\n");

    /* acquire schannel dss provider */
    result = CryptAcquireContextA(&hProv, NULL, MS_DEF_DH_SCHANNEL_PROV_A, PROV_DH_SCHANNEL,
        CRYPT_VERIFYCONTEXT);
    ok(result, "Failed to acquire CSP.\n");

    test_keyExchange_dssDH(hProv, dssDHkey_data, ARRAY_SIZE(dssDHkey_data));

    result = CryptReleaseContext(hProv, 0);
    ok(result, "Failed to release CSP provider.\n");
}

static void test_duplicate_hash(void)
{
    static const char expected[] =
        {0xb9,0x7b,0xed,0xd4,0x7b,0xd8,0xa0,0xcd,0x6c,0xba,0xce,0xe9,0xb1,0x36,0xbb,0x00,0x27,0xe3,0x95,0x21};
    HCRYPTPROV hprov;
    HCRYPTHASH hhash, hhash2;
    BYTE buf[20];
    DWORD len;
    BOOL result;

    result = CryptAcquireContextA(&hprov, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, CRYPT_VERIFYCONTEXT);
    ok(result, "got %08lx\n", GetLastError());

    result = CryptCreateHash(hprov, CALG_SHA, 0, 0, &hhash);
    ok(result, "got %08lx\n", GetLastError());

    result = CryptHashData(hhash, (const BYTE *)"winetest", sizeof("winetest"), 0);
    ok(result, "got %08lx\n", GetLastError());

    len = sizeof(buf);
    result = CryptGetHashParam(hhash, HP_HASHVAL, buf, &len, 0);
    ok(result, "got %08lx\n", GetLastError());
    ok(!memcmp(buf, expected, sizeof(expected)), "wrong data\n");

    SetLastError(0xdeadbeef);
    result = CryptHashData(hhash, (const BYTE *)"winetest", sizeof("winetest"), 0);
    ok(!result, "success\n");
    ok(GetLastError() == NTE_BAD_HASH_STATE, "got %08lx\n", GetLastError());

    result = CryptDuplicateHash(hhash, NULL, 0, &hhash2);
    ok(result, "got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptHashData(hhash2, (const BYTE *)"winetest", sizeof("winetest"), 0);
    ok(!result, "success\n");
    ok(GetLastError() == NTE_BAD_HASH_STATE, "got %08lx\n", GetLastError());

    len = sizeof(buf);
    result = CryptGetHashParam(hhash2, HP_HASHVAL, buf, &len, 0);
    ok(result, "got %08lx\n", GetLastError());
    ok(!memcmp(buf, expected, sizeof(expected)), "wrong data\n");

    result = CryptDestroyHash(hhash2);
    ok(result, "got %08lx\n", GetLastError());

    result = CryptDestroyHash(hhash);
    ok(result, "got %08lx\n", GetLastError());

    result = CryptReleaseContext(hprov, 0);
    ok(result, "got %08lx\n", GetLastError());
}

static void test_userkey(void)
{
    HCRYPTPROV hprov;
    HCRYPTKEY hkey;
    BOOL result;

    CryptAcquireContextA(&hprov, "winetest", MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_DELETEKEYSET);
    result = CryptAcquireContextA(&hprov, "winetest", MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_NEWKEYSET);
    ok(result, "got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptGetUserKey(hprov, AT_KEYEXCHANGE, &hkey);
    ok(!result, "success\n");
    ok(GetLastError() == NTE_NO_KEY, "got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptGetUserKey(hprov, AT_SIGNATURE, &hkey);
    ok(!result, "success\n");
    ok(GetLastError() == NTE_NO_KEY, "got %08lx\n", GetLastError());

    result = CryptGenKey(hprov, AT_SIGNATURE, 1024 << 16, &hkey);
    ok(result, "got %08lx\n", GetLastError());
    result = CryptDestroyKey(hkey);
    ok(result, "got %08lx\n", GetLastError());

    result = CryptGetUserKey(hprov, AT_SIGNATURE, &hkey);
    ok(result, "got %08lx\n", GetLastError());
    result = CryptDestroyKey(hkey);
    ok(result, "got %08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    result = CryptGetUserKey(hprov, AT_KEYEXCHANGE, &hkey);
    ok(!result, "success\n");
    ok(GetLastError() == NTE_NO_KEY, "got %08lx\n", GetLastError());

    result = CryptReleaseContext(hprov, 0);
    ok(result, "got %08lx\n", GetLastError());

    hprov = 0xdeadbeef;
    result = CryptAcquireContextA(&hprov, "winetest", MS_ENH_DSS_DH_PROV_A, PROV_DSS_DH, CRYPT_DELETEKEYSET);
    ok(result, "got %08lx\n", GetLastError());
    ok(!hprov, "got %08lx\n", (DWORD)hprov);
}

static void test_duplicate_key(void)
{
    HCRYPTPROV hprov;
    HCRYPTKEY hkey, hkey2;
    DWORD len;
    BOOL result;
    BYTE buf[512];

    result = CryptAcquireContextA(&hprov, NULL, MS_DEF_DSS_PROV_A, PROV_DSS, CRYPT_VERIFYCONTEXT);
    ok(result, "got %08lx\n", GetLastError());

    result = CryptImportKey(hprov, DSS_SIGN_PrivateKey, sizeof(DSS_SIGN_PrivateKey), 0, CRYPT_EXPORTABLE, &hkey);
    ok(result, "got %08lx\n", GetLastError());

    result = CryptDuplicateKey(hkey, NULL, 0, &hkey2);
    ok(result, "got %08lx\n", GetLastError());

    len = sizeof(buf);
    result = CryptExportKey(hkey2, 0, PRIVATEKEYBLOB, 0, buf, &len);
    ok(result, "got %08lx\n", GetLastError());
    ok(len == sizeof(DSS_SIGN_PrivateKey), "got %lu\n", len);
    ok(!memcmp(buf, DSS_SIGN_PrivateKey, sizeof(DSS_SIGN_PrivateKey)), "wrong data\n");

    result = CryptDestroyKey(hkey2);
    ok(result, "got %08lx\n", GetLastError());

    result = CryptDestroyKey(hkey);
    ok(result, "got %08lx\n", GetLastError());
}

START_TEST(dssenh)
{
    test_acquire_context();
    test_keylength();
    test_hash(hash_data, ARRAY_SIZE(hash_data));
    test_data_encryption(encrypt_data, ARRAY_SIZE(encrypt_data));
    test_cipher_modes(ciphermode_data, ARRAY_SIZE(ciphermode_data));
    test_verify_signature();
    test_key_exchange();
    test_duplicate_hash();
    test_userkey();
    test_duplicate_key();
}
