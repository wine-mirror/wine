/*
 * Unit test for bcrypt functions
 *
 * Copyright 2014 Bruno Jesus
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
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <bcrypt.h>
#include <ncrypt.h>

#include "wine/test.h"

static NTSTATUS (WINAPI *pBCryptHash)(BCRYPT_ALG_HANDLE, UCHAR *, ULONG, UCHAR *, ULONG, UCHAR *, ULONG);

static void test_BCryptGenRandom(void)
{
    NTSTATUS ret;
    UCHAR buffer[256];

    ret = BCryptGenRandom(NULL, NULL, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %#lx\n", ret);
    ret = BCryptGenRandom(NULL, buffer, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %#lx\n", ret);
    ret = BCryptGenRandom(NULL, buffer, sizeof(buffer), 0);
    ok(ret == STATUS_INVALID_HANDLE, "Expected STATUS_INVALID_HANDLE, got %#lx\n", ret);
    ret = BCryptGenRandom(NULL, buffer, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got %#lx\n", ret);
    ret = BCryptGenRandom(NULL, buffer, sizeof(buffer),
          BCRYPT_USE_SYSTEM_PREFERRED_RNG|BCRYPT_RNG_USE_ENTROPY_IN_BUFFER);
    ok(ret == STATUS_SUCCESS, "Expected success, got %#lx\n", ret);
    ret = BCryptGenRandom(NULL, NULL, sizeof(buffer), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", ret);

    /* Zero sized buffer should work too */
    ret = BCryptGenRandom(NULL, buffer, 0, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got %#lx\n", ret);

    /* Test random number generation - It's impossible for a sane RNG to return 8 zeros */
    memset(buffer, 0, 16);
    ret = BCryptGenRandom(NULL, buffer, 8, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    ok(ret == STATUS_SUCCESS, "Expected success, got %#lx\n", ret);
    ok(memcmp(buffer, buffer + 8, 8), "Expected a random number, got 0\n");
}

static void test_BCryptGetFipsAlgorithmMode(void)
{
    HKEY hkey = NULL;
    BOOLEAN expected;
    BOOLEAN enabled;
    DWORD value, count[2] = {sizeof(value), sizeof(value)};
    NTSTATUS ret;

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE,
                    L"System\\CurrentControlSet\\Control\\Lsa\\FIPSAlgorithmPolicy", &hkey) == ERROR_SUCCESS &&
        RegQueryValueExW(hkey, L"Enabled", NULL, NULL, (void *)&value, &count[0]) == ERROR_SUCCESS)
    {
        expected = !!value;
    }
      else if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Lsa", &hkey) == ERROR_SUCCESS &&
               RegQueryValueExW(hkey, L"FIPSAlgorithmPolicy", NULL, NULL, (void *)&value, &count[0]) == ERROR_SUCCESS)
    {
        expected = !!value;
    }
    else
    {
        expected = FALSE;
        todo_wine
        ok(0, "Neither XP or Vista key is present\n");
    }
    RegCloseKey(hkey);

    ret = BCryptGetFipsAlgorithmMode(&enabled);
    ok(ret == STATUS_SUCCESS, "Expected STATUS_SUCCESS, got %#lx\n", ret);
    ok(enabled == expected, "expected result %d, got %d\n", expected, enabled);

    ret = BCryptGetFipsAlgorithmMode(NULL);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", ret);
}

static void format_hash(const UCHAR *bytes, ULONG size, char *buf)
{
    ULONG i;
    buf[0] = '\0';
    for (i = 0; i < size; i++)
    {
        sprintf(buf + i * 2, "%02x", bytes[i]);
    }
    return;
}

#define test_object_length(a) _test_object_length(__LINE__,a)
static void _test_object_length(unsigned line, void *handle)
{
    NTSTATUS status;
    ULONG len, size;

    len = size = 0xdeadbeef;
    status = BCryptGetProperty(NULL, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_HANDLE, "BCryptGetProperty failed: %#lx\n", status);

    len = size = 0xdeadbeef;
    status = BCryptGetProperty(handle, NULL, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_PARAMETER, "BCryptGetProperty failed: %#lx\n", status);

    len = size = 0xdeadbeef;
    status = BCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), NULL, 0);
    ok_(__FILE__,line)(status == STATUS_INVALID_PARAMETER, "BCryptGetProperty failed: %#lx\n", status);

    len = size = 0xdeadbeef;
    status = BCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, NULL, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %#lx\n", status);
    ok_(__FILE__,line)(size == sizeof(len), "got %lu\n", size);

    len = size = 0xdeadbeef;
    status = BCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, 0, &size, 0);
    ok_(__FILE__,line)(status == STATUS_BUFFER_TOO_SMALL, "BCryptGetProperty failed: %#lx\n", status);
    ok_(__FILE__,line)(len == 0xdeadbeef, "got %lu\n", len);
    ok_(__FILE__,line)(size == sizeof(len), "got %lu\n", size);

    len = size = 0xdeadbeef;
    status = BCryptGetProperty(handle, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %#lx\n", status);
    ok_(__FILE__,line)(len != 0xdeadbeef, "len not set\n");
    ok_(__FILE__,line)(size == sizeof(len), "got %lu\n", size);
}

#define test_hash_length(a,b) _test_hash_length(__LINE__,a,b)
static void _test_hash_length(unsigned line, void *handle, ULONG exlen)
{
    ULONG len = 0xdeadbeef, size = 0xdeadbeef;
    NTSTATUS status;

    status = BCryptGetProperty(handle, BCRYPT_HASH_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %#lx\n", status);
    ok_(__FILE__,line)(size == sizeof(len), "got %lu\n", size);
    ok_(__FILE__,line)(len == exlen, "len = %lu, expected %lu\n", len, exlen);
}

#define test_alg_name(a,b) _test_alg_name(__LINE__,a,b)
static void _test_alg_name(unsigned line, void *handle, const WCHAR *exname)
{
    ULONG size = 0xdeadbeef;
    UCHAR buf[256];
    const WCHAR *name = (const WCHAR*)buf;
    NTSTATUS status;

    status = BCryptGetProperty(handle, BCRYPT_ALGORITHM_NAME, buf, sizeof(buf), &size, 0);
    ok_(__FILE__,line)(status == STATUS_SUCCESS, "BCryptGetProperty failed: %#lx\n", status);
    ok_(__FILE__,line)(size == (lstrlenW(exname) + 1) * sizeof(WCHAR), "got %lu\n", size);
    ok_(__FILE__,line)(!lstrcmpW(name, exname), "alg name = %s, expected %s\n", wine_dbgstr_w(name),
                       wine_dbgstr_w(exname));
}

struct hash_test
{
    const WCHAR *alg;
    unsigned hash_size;
    const char *hash;
    const char *hash2;
    const char *hmac_hash;
    const char *hmac_hash2;
};

static void test_hash(const struct hash_test *test)
{
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR buf[512], buf_hmac[1024], hash_buf[128], hmac_hash[128];
    char str[512];
    NTSTATUS ret;
    ULONG len;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, test->alg, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_object_length(alg);
    test_hash_length(alg, test->hash_size);
    test_alg_name(alg, test->alg);

    hash = NULL;
    len = sizeof(buf);
    ret = BCryptCreateHash(alg, &hash, buf, len, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    test_hash_length(hash, test->hash_size);
    test_alg_name(hash, test->alg);

    memset(hash_buf, 0, sizeof(hash_buf));
    ret = BCryptFinishHash(hash, hash_buf, test->hash_size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    format_hash( hash_buf, test->hash_size, str );
    ok(!strcmp(str, test->hash), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    hash = NULL;
    len = sizeof(buf);
    ret = BCryptCreateHash(alg, &hash, buf, len, NULL, 0, BCRYPT_HASH_REUSABLE_FLAG);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER) /* < win8 */, "got %#lx\n", ret);
    if (ret == STATUS_SUCCESS)
    {
        ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

        memset(hash_buf, 0, sizeof(hash_buf));
        ret = BCryptFinishHash(hash, hash_buf, test->hash_size, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        format_hash( hash_buf, test->hash_size, str );
        ok(!strcmp(str, test->hash), "got %s\n", str);

        /* reuse it */
        ret = BCryptHashData(hash, (UCHAR *)"tset", sizeof("tset"), 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

        memset(hash_buf, 0, sizeof(hash_buf));
        ret = BCryptFinishHash(hash, hash_buf, test->hash_size, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        format_hash( hash_buf, test->hash_size, str );
        ok(!strcmp(str, test->hash2), "got %s\n", str);

        ret = BCryptDestroyHash(hash);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    }

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, test->alg, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(alg != NULL, "alg not set\n");

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = BCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(hash != NULL, "hash not set\n");

    ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    test_hash_length(hash, test->hash_size);
    test_alg_name(hash, test->alg);

    memset(hmac_hash, 0, sizeof(hmac_hash));
    ret = BCryptFinishHash(hash, hmac_hash, test->hash_size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    format_hash( hmac_hash, test->hash_size, str );
    ok(!strcmp(str, test->hmac_hash), "got %s\n", str);

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    hash = NULL;
    len = sizeof(buf_hmac);
    ret = BCryptCreateHash(alg, &hash, buf_hmac, len, (UCHAR *)"key", sizeof("key"), BCRYPT_HASH_REUSABLE_FLAG);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER) /* < win8 */, "got %#lx\n", ret);
    if (ret == STATUS_SUCCESS)
    {
        ret = BCryptHashData(hash, (UCHAR *)"test", sizeof("test"), 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

        memset(hmac_hash, 0, sizeof(hmac_hash));
        ret = BCryptFinishHash(hash, hmac_hash, test->hash_size, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        format_hash( hmac_hash, test->hash_size, str );
        ok(!strcmp(str, test->hmac_hash), "got %s\n", str);

        /* reuse it */
        ret = BCryptHashData(hash, (UCHAR *)"tset", sizeof("tset"), 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

        memset(hmac_hash, 0, sizeof(hmac_hash));
        ret = BCryptFinishHash(hash, hmac_hash, test->hash_size, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        format_hash( hmac_hash, test->hash_size, str );
        ok(!strcmp(str, test->hmac_hash2), "got %s\n", str);

        ret = BCryptDestroyHash(hash);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    }

    ret = BCryptDestroyHash(hash);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    ret = BCryptDestroyHash(NULL);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

static void test_hashes(void)
{
    static const struct hash_test tests[] =
    {
        { L"SHA1", 20,
        "961fa64958818f767707072755d7018dcd278e94",
        "9314f62ff64197143c91fc86de37e9ae776a3fb8",
        "2472cf65d0e090618d769d3e46f0d9446cf212da",
        "b2d2ba8cfd714d474cf0d9622cc5d15e1f53d53f",
        },
        { L"SHA256", 32,
        "ceb73749c899693706ede1e30c9929b3fd5dd926163831c2fb8bd41e6efb1126",
        "ea0938c118a7b15954f41b85195f2b42aec3a9429c63f593cfa65c137ffaa986",
        "34c1aa473a4468a91d06e7cdbc75bc4f93b830ccfc2a47ffd74e8e6ed29e4c72",
        "55feb7052060bd99e33f36eb0982c7f4856eb6a84fbefe19a1afd9faafc3af6f",
        },
        { L"SHA384", 48,
        "62b21e90c9022b101671ba1f808f8631a8149f0f12904055839a35c1ca78ae53"
        "63eed1e743a692d70e0504b0cfd12ef9",
        "724db7c0bbc51ef1ac3fc793083fc54c0e5c423faec9b11378c01c236b19aaaf"
        "a45177ad055feaf003968cc40ece44c7",
        "4b3e6d6ff2da121790ab7e7b9247583e3a7eed2db5bd4dabc680303b1608f37d"
        "fdc836d96a704c03283bc05b4f6c5eb8",
        "03e1818e5c165a0e54619e513acb06c393e1a6cb0ddbb4036b5f29617b334642"
        "e6e0be8b214d8508595b17a8c4b4e7db",
        },
        { L"SHA512", 64,
        "d55ced17163bf5386f2cd9ff21d6fd7fe576a915065c24744d09cfae4ec84ee1"
        "ef6ef11bfbc5acce3639bab725b50a1fe2c204f8c820d6d7db0df0ecbc49c5ca",
        "7752d707b54d2b00e7d1c09120d189475b0fd2e31ebb988cf0a01fc8492ddc0b"
        "3ca9c9ca61d9d7d1fb65ca7665e87f043c1d5bc9f786f8345e951c2d91ac594f",
        "415fb6b10018ca03b38a1b1399c42ac0be5e8aceddb9a73103f5e543bf2d888f"
        "2eecf91373941f9315dd730a77937fa92444450fbece86f409d9cb5ec48c6513",
        "1487bcecba46ae677622fa499e4cb2f0fdf92f6f3427cba76382d537a06e49c3"
        "3e70a2fc1fc730092bf21128c3704cc6387f6dfbf7e2f9f315bbb894505a1205",
        },
        { L"MD2", 16,
        "1bb33606ba908912a84221109d29cd7e",
        "b9a6ad9323b17e2d0cd389dddd6ef78a",
        "7f05b0638d77f4a27f3a9c4d353cd648",
        "05980873e6bfdd05dd7b30078de7e42a",
        },
        { L"MD4", 16,
        "74b5db93c0b41e36ca7074338fc0b637",
        "a14a9ff2059a8c28f47b01e6bc48a1bf",
        "bc2e8ac4d8248ed21b8d26227a30ea3a",
        "b609db0eb4b8669db74f2c20099701e4",
        },
        { L"MD5", 16,
        "e2a3e68d23ce348b8f68b3079de3d4c9",
        "bcdd7ca574342aa9db0e212348eacb16",
        "7bda029b93fa8d817fcc9e13d6bdf092",
        "dd636ab8e9592c5088e57c37d44c5bb3",
        }
    };
    unsigned i;

    for(i = 0; i < ARRAY_SIZE(tests); i++)
        test_hash(tests+i);
}

static void test_BcryptHash(void)
{
    static const char expected[] =
        "e2a3e68d23ce348b8f68b3079de3d4c9";
    static const char expected_hmac[] =
        "7bda029b93fa8d817fcc9e13d6bdf092";
    BCRYPT_ALG_HANDLE alg;
    UCHAR md5[16], md5_hmac[16];
    char str[65];
    NTSTATUS ret;

    if (!pBCryptHash) /* < Win10 */
    {
        win_skip("BCryptHash is not available\n");
        return;
    }

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_hash_length(alg, 16);
    test_alg_name(alg, L"MD5");

    memset(md5, 0, sizeof(md5));
    ret = pBCryptHash(alg, NULL, 0, (UCHAR *)"test", sizeof("test"), md5, sizeof(md5));
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    format_hash( md5, sizeof(md5), str );
    ok(!strcmp(str, expected), "got %s\n", str);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    alg = NULL;
    memset(md5_hmac, 0, sizeof(md5_hmac));
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_MD5_ALGORITHM, MS_PRIMITIVE_PROVIDER, BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(alg != NULL, "alg not set\n");

    ret = pBCryptHash(alg, (UCHAR *)"key", sizeof("key"), (UCHAR *)"test", sizeof("test"), md5_hmac, sizeof(md5_hmac));
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    format_hash( md5_hmac, sizeof(md5_hmac), str );
    ok(!strcmp(str, expected_hmac), "got %s\n", str);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

/* test vectors from RFC 6070 */
static UCHAR password[] = "password";
static UCHAR salt[] = "salt";
static UCHAR long_password[] = "passwordPASSWORDpassword";
static UCHAR long_salt[] = "saltSALTsaltSALTsaltSALTsaltSALTsalt";
static UCHAR password_NUL[] = "pass\0word";
static UCHAR salt_NUL[] = "sa\0lt";

static UCHAR dk1[] = "0c60c80f961f0e71f3a9b524af6012062fe037a6";
static UCHAR dk2[] = "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957";
static UCHAR dk3[] = "4b007901b765489abead49d926f721d065a429c1";
static UCHAR dk4[] = "364dd6bc200ec7d197f1b85f4a61769010717124";
static UCHAR dk5[] = "3d2eec4fe41c849b80c8d83662c0e44a8b291a964cf2f07038";
static UCHAR dk6[] = "56fa6aa75548099dcc37d7f03425e0c3";

static const struct
{
    ULONG        pwd_len;
    ULONG        salt_len;
    ULONGLONG    iterations;
    ULONG        dk_len;
    UCHAR       *pwd;
    UCHAR       *salt;
    const UCHAR *dk;
} rfc6070[] =
{
    {  8,  4,        1, 20, password,      salt,      dk1 },
    {  8,  4,        2, 20, password,      salt,      dk2 },
    {  8,  4,     4096, 20, password,      salt,      dk3 },
    {  8,  4,  1000000, 20, password,      salt,      dk4 },
    { 24, 36,     4096, 25, long_password, long_salt, dk5 },
    {  9,  5,     4096, 16, password_NUL,  salt_NUL,  dk6 }
};

static void test_BcryptDeriveKeyPBKDF2(void)
{
    BCRYPT_ALG_HANDLE alg;
    UCHAR buf[25];
    char str[51];
    NTSTATUS ret;
    ULONG i;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, MS_PRIMITIVE_PROVIDER,
                                       BCRYPT_ALG_HANDLE_HMAC_FLAG);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(alg != NULL, "alg not set\n");

    test_hash_length(alg, 20);
    test_alg_name(alg, L"SHA1");

    ret = BCryptDeriveKeyPBKDF2(alg, rfc6070[0].pwd, rfc6070[0].pwd_len, rfc6070[0].salt, rfc6070[0].salt_len,
                                 0, buf, rfc6070[0].dk_len, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    for (i = 0; i < ARRAY_SIZE(rfc6070); i++)
    {
        memset(buf, 0, sizeof(buf));
        ret = BCryptDeriveKeyPBKDF2(alg, rfc6070[i].pwd, rfc6070[i].pwd_len, rfc6070[i].salt, rfc6070[i].salt_len,
                                     rfc6070[i].iterations, buf, rfc6070[i].dk_len, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        format_hash(buf, rfc6070[i].dk_len, str);
        ok(!memcmp(str, rfc6070[i].dk, rfc6070[i].dk_len), "got %s\n", str);
    }

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

static void test_rng(void)
{
    BCRYPT_ALG_HANDLE alg;
    ULONG size, len;
    UCHAR buf[16];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_RNG_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %#lx\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %#lx\n", ret);

    test_alg_name(alg, L"RNG");

    memset(buf, 0, 16);
    ret = BCryptGenRandom(alg, buf, 8, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(memcmp(buf, buf + 8, 8), "got zeroes\n");

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

static void test_aes(void)
{
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    BCRYPT_ALG_HANDLE alg;
    ULONG size, len;
    UCHAR mode[64];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(len, "expected non-zero len\n");
    ok(size == sizeof(len), "got %lu\n", size);

    len = size = 0;
    ret = BCryptGetProperty(alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(len == 16, "got %lu\n", len);
    ok(size == sizeof(len), "got %lu\n", size);

    size = 0;
    ret = BCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 64, "got %lu\n", size);

    size = 0;
    ret = BCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode) - 1, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 64, "got %lu\n", size);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = BCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %lu\n", size);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = BCryptGetProperty(alg, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(key_lengths), "got %lu\n", size);
    ok(key_lengths.dwMinLength == 128, "Expected 128, got %lu\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 256, "Expected 256, got %lu\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 64, "Expected 64, got %lu\n", key_lengths.dwIncrement);

    memcpy(mode, BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM));
    ret = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, mode, 0, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = BCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_GCM), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %lu\n", size);

    test_alg_name(alg, L"AES");

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

static void test_3des(void)
{
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    BCRYPT_ALG_HANDLE alg;
    ULONG size, len;
    UCHAR mode[64];
    NTSTATUS ret;

    alg = NULL;
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_3DES_ALGORITHM, MS_PRIMITIVE_PROVIDER, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(alg != NULL, "alg not set\n");

    len = size = 0;
    ret = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(len, "expected non-zero len\n");
    ok(size == sizeof(len), "got %lu\n", size);

    len = size = 0;
    ret = BCryptGetProperty(alg, BCRYPT_BLOCK_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(len == 8, "got %lu\n", len);
    ok(size == sizeof(len), "got %lu\n", size);

    size = 0;
    ret = BCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 64, "got %lu\n", size);

    size = 0;
    ret = BCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode) - 1, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 64, "got %lu\n", size);

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = BCryptGetProperty(alg, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %lu\n", size);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = BCryptGetProperty(alg, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(key_lengths), "got %lu\n", size);
    ok(key_lengths.dwMinLength == 192, "Expected 192, got %lu\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 192, "Expected 192, got %lu\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 0, "Expected 0, got %lu\n", key_lengths.dwIncrement);

    memcpy(mode, BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM));
    ret = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, mode, 0, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %#lx\n", ret);

    test_alg_name(alg, L"3DES");

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

static void test_BCryptGenerateSymmetricKey(void)
{
    static UCHAR secret[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR iv[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR data[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR expected[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79};
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key, key2;
    UCHAR *buf, ciphertext[16], plaintext[16], ivbuf[16], mode[64];
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    ULONG size, len, i;
    NTSTATUS ret;
    DWORD keylen;

    ret = BCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    len = size = 0xdeadbeef;
    ret = BCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    key = (void *)0xdeadbeef;
    ret = BCryptGenerateSymmetricKey(NULL, &key, NULL, 0, secret, sizeof(secret), 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);
    ok(key == (void *)0xdeadbeef, "got %p\n", key);

    key = NULL;
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);

    key = (BCRYPT_KEY_HANDLE)0xdeadbeef;
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, 1, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);
    ok(key == (HANDLE)0xdeadbeef, "got unexpected key %p.\n", key);

    key = (BCRYPT_KEY_HANDLE)0xdeadbeef;
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret) + 1, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);
    ok(key == (HANDLE)0xdeadbeef, "got unexpected key %p.\n", key);

    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(key != NULL, "key not set\n");

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CBC,
                            sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    todo_wine
    {
    keylen = 512;
    ret = BCryptSetProperty(aes, BCRYPT_KEY_LENGTH, (UCHAR *)&keylen, sizeof(keylen), 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %#lx\n", ret);
    }

    size = 0;
    memset(mode, 0, sizeof(mode));
    ret = BCryptGetProperty(key, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
    ok(size == 64, "got %lu\n", size);

    ret = BCryptSetProperty(key, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_ECB, 0, 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_NOT_SUPPORTED) /* < Win 8 */, "got %#lx\n", ret);
    if (ret == STATUS_SUCCESS)
    {
        size = 0;
        memset(mode, 0, sizeof(mode));
        ret = BCryptGetProperty(key, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_ECB), "got %s\n", wine_dbgstr_w((const WCHAR *)mode));
        ok(size == 64, "got %lu\n", size);
    }

    ret = BCryptSetProperty(key, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CBC,
                             sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_NOT_SUPPORTED) /* < Win 8 */, "got %#lx\n", ret);

    size = 0xdeadbeef;
    ret = BCryptEncrypt(key, NULL, 0, NULL, NULL, 0, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!size, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptEncrypt(key, data, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected[i]);

    key2 = (void *)0xdeadbeef;
    ret = BCryptDuplicateKey(NULL, &key2, NULL, 0, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);
    ok(key2 == (void *)0xdeadbeef, "got %p\n", key2);

    if (0) /* crashes on some Windows versions */
    {
        ret = BCryptDuplicateKey(key, NULL, NULL, 0, 0);
        ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);
    }

    key2 = (void *)0xdeadbeef;
    ret = BCryptDuplicateKey(key, &key2, NULL, 0, 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER), "got %#lx\n", ret);

    if (ret == STATUS_SUCCESS)
    {
        size = 0;
        memcpy(ivbuf, iv, sizeof(iv));
        memset(ciphertext, 0, sizeof(ciphertext));
        ret = BCryptEncrypt(key2, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        ok(size == 16, "got %lu\n", size);
        ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
        for (i = 0; i < 16; i++)
            ok(ciphertext[i] == expected[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected[i]);

        ret = BCryptDestroyKey(key2);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    }

    size = 0xdeadbeef;
    ret = BCryptDecrypt(key, NULL, 0, NULL, NULL, 0, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!size, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext, 16, NULL, ivbuf, 16, plaintext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    ok(!memcmp(plaintext, data, sizeof(data)), "wrong data\n");

    memset(mode, 0, sizeof(mode));
    ret = BCryptGetProperty(key, BCRYPT_CHAINING_MODE, mode, sizeof(mode), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!lstrcmpW((const WCHAR *)mode, BCRYPT_CHAIN_MODE_CBC), "wrong mode\n");

    len = 0;
    size = 0;
    ret = BCryptGetProperty(key, BCRYPT_BLOCK_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(len == 16, "got %lu\n", len);
    ok(size == sizeof(len), "got %lu\n", size);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = BCryptGetProperty(aes, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(key_lengths), "got %lu\n", size);
    ok(key_lengths.dwMinLength == 128, "Expected 128, got %lu\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 256, "Expected 256, got %lu\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 64, "Expected 64, got %lu\n", key_lengths.dwIncrement);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = BCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

#define RACE_TEST_COUNT 200
static LONG encrypt_race_start_barrier;

static DWORD WINAPI encrypt_race_thread(void *parameter)
{
    static UCHAR nonce[] =
        {0x11,0x20,0x30,0x40,0x50,0x60,0x10,0x20,0x30,0x40,0x50,0x60};
    static UCHAR auth_data[] =
        {0x61,0x50,0x40,0x30,0x20,0x10,0x60,0x50,0x40,0x30,0x20,0x10};
    static UCHAR data2[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
    static UCHAR expected4[] =
        {0xb2,0x27,0x19,0x09,0xc7,0x89,0xdc,0x52,0x24,0x83,0x3a,0x55,0x34,0x76,0x2c,0xbf,
         0x15,0xa1,0xcb,0x40,0x78,0x11,0xba,0xbc,0xa4,0x76,0x69,0x7c,0x75,0x4f,0x11,0xba};
    static UCHAR expected_tag3[] =
        {0xef,0xee,0x75,0x99,0xb8,0x12,0xe9,0xf0,0xb4,0xcc,0x65,0x11,0x67,0x60,0x2d,0xe6};

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    BCRYPT_KEY_HANDLE key = parameter;
    UCHAR ciphertext[48], tag[16];
    unsigned int i, test;
    NTSTATUS ret;
    ULONG size;

    memset(&auth_info, 0, sizeof(auth_info));
    auth_info.cbSize = sizeof(auth_info);
    auth_info.dwInfoVersion = 1;
    auth_info.pbNonce = nonce;
    auth_info.cbNonce = sizeof(nonce);
    auth_info.pbTag = tag;
    auth_info.cbTag = sizeof(tag);
    auth_info.pbAuthData = auth_data;
    auth_info.cbAuthData = sizeof(auth_data);

    InterlockedIncrement(&encrypt_race_start_barrier);
    while (InterlockedCompareExchange(&encrypt_race_start_barrier, 3, 2) != 2)
        ;

    for (test = 0; test < RACE_TEST_COUNT; ++test)
    {
        size = 0;
        memset(ciphertext, 0xff, sizeof(ciphertext));
        memset(tag, 0xff, sizeof(tag));
        ret = BCryptEncrypt(key, data2, 32, &auth_info, NULL, 0, ciphertext, 32, &size, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        ok(size == 32, "got %lu\n", size);
        ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
        ok(!memcmp(tag, expected_tag3, sizeof(expected_tag3)), "wrong tag\n");
        for (i = 0; i < 32; i++)
            ok(ciphertext[i] == expected4[i], "%u: %02x != %02x\n", i, ciphertext[i], expected4[i]);
        for (i = 0; i < 16; i++)
            ok(tag[i] == expected_tag3[i], "%u: %02x != %02x\n", i, tag[i], expected_tag3[i]);
    }

    return 0;
}

static void test_BCryptEncrypt(void)
{
    static UCHAR nonce[] =
        {0x10,0x20,0x30,0x40,0x50,0x60,0x10,0x20,0x30,0x40,0x50,0x60};
    static UCHAR auth_data[] =
        {0x60,0x50,0x40,0x30,0x20,0x10,0x60,0x50,0x40,0x30,0x20,0x10};
    static UCHAR secret[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR secret256[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,0x00};
    static UCHAR iv[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR data[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10};
    static UCHAR data2[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
    static UCHAR expected[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79};
    static UCHAR expected2[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0x28,0x73,0x3d,0xef,0x84,0x8f,0xb0,0xa6,0x5d,0x1a,0x51,0xb7,0xec,0x8f,0xea,0xe9};
    static UCHAR expected3[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0xb1,0xa2,0x92,0x73,0xbe,0x2c,0x42,0x07,0xa5,0xac,0xe3,0x93,0x39,0x8c,0xb6,0xfb,
         0x87,0x5d,0xea,0xa3,0x7e,0x0f,0xde,0xfa,0xd9,0xec,0x6c,0x4e,0x3c,0x76,0x86,0xe4};
    static UCHAR expected4[] =
        {0xe1,0x82,0xc3,0xc0,0x24,0xfb,0x86,0x85,0xf3,0xf1,0x2b,0x7d,0x09,0xb4,0x73,0x67,
         0x86,0x64,0xc3,0xfe,0xa3,0x07,0x61,0xf8,0x16,0xc9,0x78,0x7f,0xe7,0xb1,0xc4,0x94};
    static UCHAR expected5[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a};
    static UCHAR expected6[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a,
         0x84,0x07,0x66,0xb7,0x49,0xc0,0x9b,0x49,0x74,0x28,0x8c,0x10,0xb9,0xc2,0x09,0x70};
    static UCHAR expected7[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a,
         0x95,0x4f,0x64,0xf2,0xe4,0xe8,0x6e,0x9e,0xee,0x82,0xd2,0x02,0x16,0x68,0x48,0x99,
         0x95,0x4f,0x64,0xf2,0xe4,0xe8,0x6e,0x9e,0xee,0x82,0xd2,0x02,0x16,0x68,0x48,0x99};
    static UCHAR expected8[] =
        {0xb5,0x8a,0x10,0x64,0xd8,0xac,0xa9,0x9b,0xd9,0xb0,0x40,0x5b,0x85,0x45,0xf5,0xbb};
    static UCHAR expected9[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a};
    static UCHAR expected10[] =
        {0x66,0xb8,0xbd,0xe5,0x90,0x6c,0xec,0xdf,0xfa,0x8a,0xb2,0xfd,0x92,0x84,0xeb,0xf0,
         0x95,0xc4,0xdf,0xa7,0x7a,0x62,0xe4,0xab,0xd4,0x0e,0x94,0x4e,0xd7,0x6e,0xa1,0x47,
         0x29,0x4b,0x37,0xfe,0x28,0x6d,0x5f,0x69,0x46,0x30,0x73,0xc0,0xaa,0x42,0xe4,0x46};
    static UCHAR expected_tag[] =
        {0x89,0xb3,0x92,0x00,0x39,0x20,0x09,0xb4,0x6a,0xd6,0xaf,0xca,0x4b,0x5b,0xfd,0xd0};
    static UCHAR expected_tag2[] =
        {0x9a,0x92,0x32,0x2c,0x61,0x2a,0xae,0xef,0x66,0x2a,0xfb,0x55,0xe9,0x48,0xdf,0xbd};
    static UCHAR expected_tag3[] =
        {0x17,0x9d,0xc0,0x7a,0xf0,0xcf,0xaa,0xd5,0x1c,0x11,0xc4,0x4b,0xd6,0xa3,0x3e,0x77};
    static UCHAR expected_tag4[] =
        {0x4c,0x42,0x83,0x9e,0x8d,0x40,0xf1,0x19,0xd6,0x2b,0x1c,0x66,0x03,0x2b,0x39,0x63};

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    UCHAR *buf, ciphertext[48], ivbuf[16], tag[16];
    BCRYPT_AUTH_TAG_LENGTHS_STRUCT tag_length;
    ULONG size, len, i, test;
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key;
    HANDLE hthread;
    NTSTATUS ret;

    ret = BCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    /******************
     * AES - CBC mode *
     ******************/

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = BCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    key = NULL;
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(key != NULL, "key not set\n");

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptEncrypt(key, data, 16, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected, sizeof(expected)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected[i]);

    /* NULL initialization vector */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data, 16, NULL, NULL, 0, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    todo_wine ok(!memcmp(ciphertext, expected8, sizeof(expected8)), "wrong data\n");

    /* all zero initialization vector */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    memset(ivbuf, 0, sizeof(ivbuf));
    ret = BCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected9, sizeof(expected9)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected9[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected9[i]);

    /* input size is not a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptEncrypt(key, data, 17, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %#lx\n", ret);
    ok(size == 17, "got %lu\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptEncrypt(key, data, 17, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data, 17, NULL, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected2, sizeof(expected2)), "wrong data\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected2[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected2[i]);

    /* input size is a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected3, sizeof(expected3)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected3[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected3[i]);

    /* output size too small */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data, 17, NULL, ivbuf, 16, ciphertext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /* 256 bit key */
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);

    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret256, sizeof(secret256), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    /* Key generations succeeds if the key size exceeds maximum and uses maximum key length
     * from secret. */
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret256, sizeof(secret256) + 1, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data2, 32, NULL, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected10, sizeof(expected10)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected10[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected10[i]);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /******************
     * AES - GCM mode *
     ******************/

    size = 0;
    ret = BCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, NULL, 0, &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %#lx\n", ret);

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    size = 0;
    ret = BCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(tag_length), "got %lu\n", size);

    size = 0;
    memset(&tag_length, 0, sizeof(tag_length));
    ret = BCryptGetProperty(aes, BCRYPT_AUTH_TAG_LENGTH, (UCHAR*)&tag_length, sizeof(tag_length), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(tag_length), "got %lu\n", size);
    ok(tag_length.dwMinLength == 12, "Expected 12, got %lu\n", tag_length.dwMinLength);
    ok(tag_length.dwMaxLength == 16, "Expected 16, got %lu\n", tag_length.dwMaxLength);
    ok(tag_length.dwIncrement == 1, "Expected 1, got %lu\n", tag_length.dwIncrement);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = BCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    key = NULL;
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(key != NULL, "key not set\n");

    ret = BCryptGetProperty(key, BCRYPT_AUTH_TAG_LENGTH, (UCHAR*)&tag_length, sizeof(tag_length), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %#lx\n", ret);

    memset(&auth_info, 0, sizeof(auth_info));
    auth_info.cbSize = sizeof(auth_info);
    auth_info.dwInfoVersion = 1;
    auth_info.pbNonce = nonce;
    auth_info.cbNonce = sizeof(nonce);
    auth_info.pbTag = tag;
    auth_info.cbTag = sizeof(tag);

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = BCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag, sizeof(expected_tag)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag[i], "%lu: %02x != %02x\n", i, tag[i], expected_tag[i]);

    /* NULL initialization vector */
    size = 0;
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = BCryptEncrypt(key, data2, 32, &auth_info, NULL, 0, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag, sizeof(expected_tag)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag[i], "%lu: %02x != %02x\n", i, tag[i], expected_tag[i]);

    /* all zero initialization vector */
    size = 0;
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    memset(ivbuf, 0, sizeof(ivbuf));
    ret = BCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag, sizeof(expected_tag)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag[i], "%lu: %02x != %02x\n", i, tag[i], expected_tag[i]);

    /* input size is not multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = BCryptEncrypt(key, data2, 24, &auth_info, ivbuf, 16, ciphertext, 24, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 24, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected4, 24), "wrong data\n");
    ok(!memcmp(tag, expected_tag2, sizeof(expected_tag2)), "wrong tag\n");
    for (i = 0; i < 24; i++)
        ok(ciphertext[i] == expected4[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag2[i], "%lu: %02x != %02x\n", i, tag[i], expected_tag2[i]);

    /* test with auth data */
    auth_info.pbAuthData = auth_data;
    auth_info.cbAuthData = sizeof(auth_data);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0xff, sizeof(ciphertext));
    memset(tag, 0xff, sizeof(tag));
    ret = BCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
    ok(!memcmp(tag, expected_tag3, sizeof(expected_tag3)), "wrong tag\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected4[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected4[i]);
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag3[i], "%lu: %02x != %02x\n", i, tag[i], expected_tag3[i]);

    memset(tag, 0xff, sizeof(tag));
    ret = BCryptEncrypt(key, data2, 0, &auth_info, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!size, "got %lu\n", size);
    for (i = 0; i < 16; i++)
        ok(tag[i] == 0xff, "%lu: %02x != %02x\n", i, tag[i], 0xff);

    memset(tag, 0xff, sizeof(tag));
    ret = BCryptEncrypt(key, NULL, 0, &auth_info, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(!size, "got %lu\n", size);
    ok(!memcmp(tag, expected_tag4, sizeof(expected_tag4)), "wrong tag\n");
    for (i = 0; i < 16; i++)
        ok(tag[i] == expected_tag4[i], "%lu: %02x != %02x\n", i, tag[i], expected_tag4[i]);

    /* test with padding */
    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    todo_wine ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);

    memcpy(ivbuf, iv, sizeof(iv));
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data2, 32, &auth_info, ivbuf, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    /* race test */

    encrypt_race_start_barrier = 0;
    hthread = CreateThread(NULL, 0, encrypt_race_thread, key, 0, NULL);

    while (InterlockedCompareExchange(&encrypt_race_start_barrier, 2, 1) != 1)
        ;

    for (test = 0; test < RACE_TEST_COUNT; ++test)
    {
        size = 0;
        memset(ciphertext, 0xff, sizeof(ciphertext));
        memset(tag, 0xff, sizeof(tag));
        ret = BCryptEncrypt(key, data2, 32, &auth_info, NULL, 0, ciphertext, 32, &size, 0);
        ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
        ok(size == 32, "got %lu\n", size);
        ok(!memcmp(ciphertext, expected4, sizeof(expected4)), "wrong data\n");
        ok(!memcmp(tag, expected_tag3, sizeof(expected_tag2)), "wrong tag\n");
        for (i = 0; i < 32; i++)
            ok(ciphertext[i] == expected4[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected4[i]);
        for (i = 0; i < 16; i++)
            ok(tag[i] == expected_tag3[i], "%lu: %02x != %02x\n", i, tag[i], expected_tag3[i]);
    }

    WaitForSingleObject(hthread, INFINITE);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /******************
     * AES - ECB mode *
     ******************/

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = BCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    /* initialization vector is not allowed */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptEncrypt(key, data, 16, NULL, ivbuf, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);

    /* input size is a multiple of block size */
    size = 0;
    ret = BCryptEncrypt(key, data, 16, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data, 16, NULL, NULL, 16, ciphertext, 16, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected5, sizeof(expected5)), "wrong data\n");
    for (i = 0; i < 16; i++)
        ok(ciphertext[i] == expected5[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected5[i]);

    /* input size is not a multiple of block size */
    size = 0;
    ret = BCryptEncrypt(key, data, 17, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %#lx\n", ret);
    ok(size == 17, "got %lu\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    ret = BCryptEncrypt(key, data, 17, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data, 17, NULL, NULL, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected6, sizeof(expected6)), "wrong data\n");
    for (i = 0; i < 32; i++)
        ok(ciphertext[i] == expected6[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected6[i]);

    /* input size is a multiple of block size, block padding set */
    size = 0;
    ret = BCryptEncrypt(key, data2, 32, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data2, 32, NULL, NULL, 16, ciphertext, 48, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);
    ok(!memcmp(ciphertext, expected7, sizeof(expected7)), "wrong data\n");
    for (i = 0; i < 48; i++)
        ok(ciphertext[i] == expected7[i], "%lu: %02x != %02x\n", i, ciphertext[i], expected7[i]);

    /* output size too small */
    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data, 17, NULL, NULL, 16, ciphertext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memset(ciphertext, 0, sizeof(ciphertext));
    ret = BCryptEncrypt(key, data2, 32, NULL, NULL, 16, ciphertext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = BCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

static void test_BCryptDecrypt(void)
{
    static UCHAR nonce[] =
        {0x10,0x20,0x30,0x40,0x50,0x60,0x10,0x20,0x30,0x40,0x50,0x60};
    static UCHAR auth_data[] =
        {0x60,0x50,0x40,0x30,0x20,0x10,0x60,0x50,0x40,0x30,0x20,0x10};
    static UCHAR secret[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR iv[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR expected[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    static UCHAR expected2[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10};
    static UCHAR expected3[] =
        {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
         0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};
    static UCHAR ciphertext[32] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0x28,0x73,0x3d,0xef,0x84,0x8f,0xb0,0xa6,0x5d,0x1a,0x51,0xb7,0xec,0x8f,0xea,0xe9};
    static UCHAR ciphertext2[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0x28,0x73,0x3d,0xef,0x84,0x8f,0xb0,0xa6,0x5d,0x1a,0x51,0xb7,0xec,0x8f,0xea,0xe9};
    static UCHAR ciphertext3[] =
        {0xc6,0xa1,0x3b,0x37,0x87,0x8f,0x5b,0x82,0x6f,0x4f,0x81,0x62,0xa1,0xc8,0xd8,0x79,
         0xb1,0xa2,0x92,0x73,0xbe,0x2c,0x42,0x07,0xa5,0xac,0xe3,0x93,0x39,0x8c,0xb6,0xfb,
         0x87,0x5d,0xea,0xa3,0x7e,0x0f,0xde,0xfa,0xd9,0xec,0x6c,0x4e,0x3c,0x76,0x86,0xe4};
    static UCHAR ciphertext4[] =
        {0xe1,0x82,0xc3,0xc0,0x24,0xfb,0x86,0x85,0xf3,0xf1,0x2b,0x7d,0x09,0xb4,0x73,0x67,
         0x86,0x64,0xc3,0xfe,0xa3,0x07,0x61,0xf8,0x16,0xc9,0x78,0x7f,0xe7,0xb1,0xc4,0x94};
    static UCHAR ciphertext5[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a,
         0x84,0x07,0x66,0xb7,0x49,0xc0,0x9b,0x49,0x74,0x28,0x8c,0x10,0xb9,0xc2,0x09,0x70};
    static UCHAR ciphertext6[] =
        {0x0a,0x94,0x0b,0xb5,0x41,0x6e,0xf0,0x45,0xf1,0xc3,0x94,0x58,0xc6,0x53,0xea,0x5a,
         0x95,0x4f,0x64,0xf2,0xe4,0xe8,0x6e,0x9e,0xee,0x82,0xd2,0x02,0x16,0x68,0x48,0x99,
         0x95,0x4f,0x64,0xf2,0xe4,0xe8,0x6e,0x9e,0xee,0x82,0xd2,0x02,0x16,0x68,0x48,0x99};
    static UCHAR tag[] =
        {0x89,0xb3,0x92,0x00,0x39,0x20,0x09,0xb4,0x6a,0xd6,0xaf,0xca,0x4b,0x5b,0xfd,0xd0};
    static UCHAR tag2[] =
        {0x17,0x9d,0xc0,0x7a,0xf0,0xcf,0xaa,0xd5,0x1c,0x11,0xc4,0x4b,0xd6,0xa3,0x3e,0x77};
    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth_info;
    BCRYPT_KEY_LENGTHS_STRUCT key_lengths;
    BCRYPT_AUTH_TAG_LENGTHS_STRUCT tag_lengths;
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key;
    UCHAR *buf, plaintext[48], ivbuf[16];
    ULONG size, len;
    NTSTATUS ret;

    ret = BCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    size = 0;
    memset(&key_lengths, 0, sizeof(key_lengths));
    ret = BCryptGetProperty(aes, BCRYPT_KEY_LENGTHS, (UCHAR*)&key_lengths, sizeof(key_lengths), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(key_lengths), "got %lu\n", size);
    ok(key_lengths.dwMinLength == 128, "Expected 128, got %lu\n", key_lengths.dwMinLength);
    ok(key_lengths.dwMaxLength == 256, "Expected 256, got %lu\n", key_lengths.dwMaxLength);
    ok(key_lengths.dwIncrement == 64, "Expected 64, got %lu\n", key_lengths.dwIncrement);

    /******************
     * AES - CBC mode *
     ******************/

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = BCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    key = NULL;
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(key != NULL, "key not set\n");

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(plaintext, expected, sizeof(expected)), "wrong data\n");

    /* test with padding smaller than block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 17, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 17, "got %lu\n", size);
    ok(!memcmp(plaintext, expected2, sizeof(expected2)), "wrong data\n");

    /* test with padding of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, plaintext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* output size too small */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext, 32, NULL, ivbuf, 16, plaintext, 31, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 15, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext2, 32, NULL, ivbuf, 16, plaintext, 16, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 17, "got %lu\n", size);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext3, 48, NULL, ivbuf, 16, plaintext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    /* input size is not a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext, 17, NULL, ivbuf, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %#lx\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %lu\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext, 17, NULL, ivbuf, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %#lx\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %lu\n", size);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /******************
     * AES - GCM mode *
     ******************/

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    key = NULL;
    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(key != NULL, "key not set\n");

    ret = BCryptGetProperty(key, BCRYPT_AUTH_TAG_LENGTH, (UCHAR*)&tag_lengths, sizeof(tag_lengths), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %#lx\n", ret);

    memset(&auth_info, 0, sizeof(auth_info));
    auth_info.cbSize = sizeof(auth_info);
    auth_info.dwInfoVersion = 1;
    auth_info.pbNonce = nonce;
    auth_info.cbNonce = sizeof(nonce);
    auth_info.pbTag = tag;
    auth_info.cbTag = sizeof(tag);

    /* input size is a multiple of block size */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* test with auth data */
    auth_info.pbAuthData = auth_data;
    auth_info.cbAuthData = sizeof(auth_data);
    auth_info.pbTag = tag2;
    auth_info.cbTag = sizeof(tag2);

    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* test with wrong tag */
    memcpy(ivbuf, iv, sizeof(iv));
    auth_info.pbTag = iv; /* wrong tag */
    ret = BCryptDecrypt(key, ciphertext4, 32, &auth_info, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_AUTH_TAG_MISMATCH, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    /******************
     * AES - ECB mode *
     ******************/

    ret = BCryptSetProperty(aes, BCRYPT_CHAINING_MODE, (UCHAR*)BCRYPT_CHAIN_MODE_ECB, sizeof(BCRYPT_CHAIN_MODE_ECB), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    len = 0xdeadbeef;
    size = sizeof(len);
    ret = BCryptGetProperty(aes, BCRYPT_OBJECT_LENGTH, (UCHAR *)&len, sizeof(len), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = BCryptGenerateSymmetricKey(aes, &key, buf, len, secret, sizeof(secret), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    /* initialization vector is not allowed */
    size = 0;
    memcpy(ivbuf, iv, sizeof(iv));
    ret = BCryptDecrypt(key, ciphertext5, 32, NULL, ivbuf, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    /* input size is a multiple of block size */
    size = 0;
    ret = BCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 32, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(plaintext, expected, sizeof(expected)), "wrong data\n");

    /* test with padding smaller than block size */
    size = 0;
    ret = BCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 17, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 17, "got %lu\n", size);
    ok(!memcmp(plaintext, expected2, sizeof(expected2)), "wrong data\n");

    /* test with padding of block size */
    size = 0;
    ret = BCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    size = 0;
    memset(plaintext, 0, sizeof(plaintext));
    ret = BCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, plaintext, 32, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);
    ok(!memcmp(plaintext, expected3, sizeof(expected3)), "wrong data\n");

    /* output size too small */
    size = 0;
    ret = BCryptDecrypt(key, ciphertext4, 32, NULL, NULL, 16, plaintext, 31, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    ret = BCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 15, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 32, "got %lu\n", size);

    size = 0;
    ret = BCryptDecrypt(key, ciphertext5, 32, NULL, NULL, 16, plaintext, 16, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 17, "got %lu\n", size);

    size = 0;
    ret = BCryptDecrypt(key, ciphertext6, 48, NULL, NULL, 16, plaintext, 31, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == 48, "got %lu\n", size);

    /* input size is not a multiple of block size */
    size = 0;
    ret = BCryptDecrypt(key, ciphertext4, 17, NULL, NULL, 16, NULL, 0, &size, 0);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %#lx\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %lu\n", size);

    /* input size is not a multiple of block size, block padding set */
    size = 0;
    ret = BCryptDecrypt(key, ciphertext4, 17, NULL, NULL, 16, NULL, 0, &size, BCRYPT_BLOCK_PADDING);
    ok(ret == STATUS_INVALID_BUFFER_SIZE, "got %#lx\n", ret);
    ok(size == 17 || broken(size == 0 /* Win < 7 */), "got %lu\n", size);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = BCryptDestroyKey(NULL);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(NULL, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);
}

static void test_key_import_export(void)
{
    UCHAR buffer1[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 16];
    UCHAR buffer2[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + 16], *buf;
    BCRYPT_KEY_DATA_BLOB_HEADER *key_data1 = (void*)buffer1;
    BCRYPT_ALG_HANDLE aes;
    BCRYPT_KEY_HANDLE key;
    NTSTATUS ret;
    ULONG size;

    ret = BCryptOpenAlgorithmProvider(&aes, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    key_data1->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
    key_data1->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    key_data1->cbKeyData = 16;
    memset(&key_data1[1], 0x11, 16);

    key = NULL;
    ret = BCryptImportKey(aes, NULL, BCRYPT_KEY_DATA_BLOB, &key, NULL, 0, buffer1, sizeof(buffer1), 0);
    ok(ret == STATUS_SUCCESS || broken(ret == STATUS_INVALID_PARAMETER) /* vista */, "got %#lx\n", ret);
    if (ret == STATUS_INVALID_PARAMETER)
    {
        win_skip("broken BCryptImportKey\n");
        return;
    }
    ok(key != NULL, "key not set\n");

    size = 0;
    ret = BCryptExportKey(key, NULL, BCRYPT_KEY_DATA_BLOB, buffer2, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size == sizeof(buffer2), "got %lu\n", size);

    size = 0;
    memset(buffer2, 0xff, sizeof(buffer2));
    ret = BCryptExportKey(key, NULL, BCRYPT_KEY_DATA_BLOB, buffer2, sizeof(buffer2), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(buffer2), "Got %lu\n", size);
    ok(!memcmp(buffer1, buffer2, sizeof(buffer1)), "Expected exported key to match imported key\n");

    /* opaque blob */
    size = 0;
    ret = BCryptExportKey(key, NULL, BCRYPT_OPAQUE_KEY_BLOB, buffer2, 0, &size, 0);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);
    ok(size > 0, "got zero\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    ret = BCryptExportKey(key, NULL, BCRYPT_OPAQUE_KEY_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    key = NULL;
    ret = BCryptImportKey(aes, NULL, BCRYPT_OPAQUE_KEY_BLOB, &key, NULL, 0, buf, size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(key != NULL, "key not set\n");
    HeapFree(GetProcessHeap(), 0, buf);

    ret = BCryptDestroyKey(key);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(aes, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
}

static BYTE eccPrivkey[] =
{
    /* X */
    0x26, 0xff, 0x0e, 0xf9, 0x71, 0x93, 0xf8, 0xed, 0x59, 0xfa, 0x24, 0xec, 0x18, 0x13, 0xfe, 0xf5,
    0x0b, 0x4a, 0xb1, 0x27, 0xb7, 0xab, 0x3e, 0x4f, 0xc5, 0x5a, 0x91, 0xa3, 0x6e, 0x21, 0x61, 0x65,
    /* Y */
    0x62, 0x7b, 0x8b, 0x30, 0x7a, 0x63, 0x4c, 0x1a, 0xf4, 0x54, 0x54, 0xbb, 0x75, 0x59, 0x68, 0x36,
    0xfe, 0x49, 0x95, 0x75, 0x9e, 0x20, 0x3e, 0x69, 0x58, 0xb9, 0x7a, 0x84, 0x03, 0x45, 0x5c, 0x10,
    /* d */
    0xb9, 0xcd, 0xbe, 0xd4, 0x75, 0x5d, 0x05, 0xe5, 0x83, 0x0c, 0xd3, 0x37, 0x34, 0x15, 0xe3, 0x2c,
    0xe5, 0x85, 0x15, 0xa9, 0xee, 0xba, 0x94, 0x03, 0x03, 0x0b, 0x86, 0xea, 0x85, 0x40, 0xbd, 0x35,
};
static BYTE eccPubkey[] =
{
    /* X */
    0x3b, 0x3c, 0x34, 0xc8, 0x3f, 0x15, 0xea, 0x02, 0x68, 0x46, 0x69, 0xdf, 0x0c, 0xa6, 0xee, 0x7a,
    0xd9, 0x82, 0x08, 0x9b, 0x37, 0x53, 0x42, 0xf3, 0x13, 0x63, 0xda, 0x65, 0x79, 0xe8, 0x04, 0x9e,
    /* Y */
    0x8c, 0x77, 0xc4, 0x33, 0x77, 0xd9, 0x5a, 0x7f, 0x60, 0x7b, 0x98, 0xce, 0xf3, 0x96, 0x56, 0xd6,
    0xb5, 0x8d, 0x87, 0x7a, 0x00, 0x2b, 0xf3, 0x70, 0xb3, 0x90, 0x73, 0xa0, 0x56, 0x06, 0x3b, 0x22,
};
static BYTE certHash[] =
{
    0x28, 0x19, 0x0f, 0x15, 0x6d, 0x75, 0xcc, 0xcf, 0x62, 0xf1, 0x5e, 0xe6, 0x8a, 0xc3, 0xf0, 0x5d,
    0x89, 0x28, 0x2d, 0x48, 0xd8, 0x73, 0x7c, 0x05, 0x05, 0x8e, 0xbc, 0xce, 0x28, 0xb7, 0xba, 0xc9,
};
static BYTE certSignature[] =
{
    /* r */
    0xd7, 0x29, 0xce, 0x5a, 0xef, 0x74, 0x85, 0xd1, 0x18, 0x5f, 0x6e, 0xf1, 0xba, 0x53, 0xd4, 0xcd,
    0xdd, 0xe0, 0x5d, 0xf1, 0x5e, 0x48, 0x51, 0xea, 0x63, 0xc0, 0xe8, 0xe2, 0xf6, 0xfa, 0x4c, 0xaf,
    /* s */
    0xe3, 0x94, 0x15, 0x3b, 0x6c, 0x71, 0x6e, 0x44, 0x22, 0xcb, 0xa0, 0x88, 0xcd, 0x0a, 0x5a, 0x50,
    0x29, 0x7c, 0x5c, 0xd6, 0x6c, 0xd2, 0xe0, 0x7f, 0xcd, 0x02, 0x92, 0x21, 0x4c, 0x2c, 0x92, 0xee,
};

static void test_ECDSA(void)
{
    BYTE buffer[sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPrivkey)];
    BCRYPT_ECCKEY_BLOB *ecckey = (void *)buffer;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    NTSTATUS status;
    ULONG size;

    status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0);
    if (status)
    {
        skip("Failed to open ECDSA provider: %#lx, skipping test\n", status);
        return;
    }

    ecckey->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    memcpy(ecckey + 1, eccPubkey, sizeof(eccPubkey));

    ecckey->cbKey = 2;
    size = sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPubkey);
    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", status);

    ecckey->dwMagic = BCRYPT_ECDH_PUBLIC_P256_MAGIC;
    ecckey->cbKey = 32;
    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", status);

    ecckey->dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    ecckey->cbKey = 32;
    status = BCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &key, buffer, size, 0);
    ok(!status, "BCryptImportKeyPair failed: %#lx\n", status);
    BCryptDestroyKey(key);

    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &key, buffer, size, 0);
    ok(!status, "BCryptImportKeyPair failed: %#lx\n", status);

    memset(buffer, 0xcc, sizeof(buffer));
    status = BCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(!status, "Got unexpected status %#lx\n", status);
    ok(ecckey->dwMagic == BCRYPT_ECDSA_PUBLIC_P256_MAGIC, "Got unexpected magic %#lx.\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %lu\n", ecckey->cbKey);
    ok(!memcmp(ecckey + 1, eccPubkey, sizeof(eccPubkey)), "Got unexpected key data.\n");

    status = BCryptExportKey(key, NULL, BCRYPT_ECCPRIVATE_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Got unexpected status %#lx\n", status);

    status = BCryptVerifySignature(key, NULL, certHash, sizeof(certHash) - 1, certSignature, sizeof(certSignature), 0);
    ok(status == STATUS_INVALID_SIGNATURE, "Expected STATUS_INVALID_SIGNATURE, got %#lx\n", status);

    status = BCryptVerifySignature(key, NULL, certHash, sizeof(certHash), certSignature, sizeof(certSignature), 0);
    ok(!status, "BCryptVerifySignature failed: %#lx\n", status);
    BCryptDestroyKey(key);

    ecckey->dwMagic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
    memcpy(ecckey + 1, eccPrivkey, sizeof(eccPrivkey));

    ecckey->cbKey = 2;
    size = sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPrivkey);
    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", status);

    ecckey->dwMagic = BCRYPT_ECDH_PRIVATE_P256_MAGIC;
    ecckey->cbKey = 32;
    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &key, buffer, size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", status);

    ecckey->dwMagic = BCRYPT_ECDSA_PRIVATE_P256_MAGIC;
    ecckey->cbKey = 32;
    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &key, buffer, size, 0);
    ok(!status, "BCryptImportKeyPair failed: %#lx\n", status);

    memset( buffer, 0xcc, sizeof(buffer) );
    status = BCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, buffer, sizeof(buffer), &size, 0);
    ok(!status, "Got unexpected status %#lx\n", status);
    ok(ecckey->dwMagic == BCRYPT_ECDSA_PUBLIC_P256_MAGIC, "got %#lx\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %lu\n", ecckey->cbKey);
    ok(!memcmp(ecckey + 1, eccPrivkey, sizeof(eccPubkey)), "Got unexpected key data.\n");

    size = sizeof(BCRYPT_ECCKEY_BLOB) + sizeof(eccPrivkey);
    memset( buffer, 0, sizeof(buffer) );
    status = BCryptExportKey(key, NULL, BCRYPT_ECCPRIVATE_BLOB, buffer, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ecckey = (BCRYPT_ECCKEY_BLOB *)buffer;
    ok(ecckey->dwMagic == BCRYPT_ECDSA_PRIVATE_P256_MAGIC, "got %#lx\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %lu\n", ecckey->cbKey);
    ok(size == sizeof(*ecckey) + ecckey->cbKey * 3, "got %lu\n", size);

    BCryptDestroyKey(key);
    BCryptCloseAlgorithmProvider(alg, 0);
}

static UCHAR rsaPublicBlob[] =
{
    0x52, 0x53, 0x41, 0x31, 0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xad, 0x41, 0x09, 0xa2, 0x56,
    0x3a, 0x7b, 0x75, 0x4b, 0x72, 0x9b, 0x28, 0x72, 0x3b, 0xae, 0x9f, 0xd8, 0xa8, 0x25, 0x4a, 0x4c,
    0x19, 0xf5, 0xa6, 0xd0, 0x05, 0x1c, 0x59, 0x8f, 0xe3, 0xf3, 0x2d, 0x29, 0x47, 0xf8, 0x80, 0x25,
    0x25, 0x21, 0x58, 0xc2, 0xac, 0xa1, 0x9e, 0x93, 0x8e, 0x82, 0x6d, 0xd7, 0xf3, 0xe7, 0x8f, 0x0b,
    0xc0, 0x41, 0x85, 0x29, 0x3c, 0xf1, 0x0b, 0x2c, 0x5d, 0x49, 0xed, 0xb4, 0x30, 0x6e, 0x02, 0x15,
    0x4b, 0x9a, 0x08, 0x0d, 0xe1, 0x6f, 0xa8, 0xd3, 0x12, 0xab, 0x66, 0x48, 0x4d, 0xd9, 0x28, 0x03,
    0x6c, 0x9d, 0x44, 0x7a, 0xed, 0xc9, 0x43, 0x4f, 0x9d, 0x4e, 0x3c, 0x7d, 0x0e, 0xff, 0x07, 0x87,
    0xeb, 0xca, 0xca, 0x65, 0x6d, 0xbe, 0xc5, 0x31, 0x8b, 0xcc, 0x7e, 0x0a, 0x71, 0x4a, 0x4d, 0x9d,
    0x3d, 0xfd, 0x7a, 0x56, 0x32, 0x8a, 0x6c, 0x6d, 0x9d, 0x2a, 0xd9, 0x8e, 0x68, 0x89, 0x63, 0xc6,
    0x4f, 0x24, 0xd1, 0x2a, 0x72, 0x69, 0x08, 0x77, 0xa0, 0x7f, 0xfe, 0xc6, 0x33, 0x8d, 0xb4, 0x7d,
    0x73, 0x91, 0x13, 0x9c, 0x47, 0x53, 0x6a, 0x13, 0xdf, 0x19, 0xc7, 0xed, 0x48, 0x81, 0xed, 0xd8,
    0x1f, 0x11, 0x11, 0xbb, 0x41, 0x15, 0x5b, 0xa4, 0xf5, 0xc9, 0x2b, 0x48, 0x5e, 0xd8, 0x4b, 0x52,
    0x1f, 0xf7, 0x87, 0xf2, 0x68, 0x25, 0x28, 0x79, 0xee, 0x39, 0x41, 0xc9, 0x0e, 0xc8, 0xf9, 0xf2,
    0xd8, 0x24, 0x09, 0xb4, 0xd4, 0xb7, 0x90, 0xba, 0x26, 0xe8, 0x1d, 0xb4, 0xd7, 0x09, 0x00, 0xc4,
    0xa0, 0xb6, 0x14, 0xe8, 0x4c, 0x29, 0x60, 0x54, 0x2e, 0x01, 0xde, 0x54, 0x66, 0x40, 0x22, 0x50,
    0x27, 0xf1, 0xe7, 0x62, 0xa9, 0x00, 0x5a, 0x61, 0x2e, 0xfa, 0xfe, 0x16, 0xd8, 0xe0, 0xe7, 0x66,
    0x17, 0xda, 0xb8, 0x0c, 0xa6, 0x04, 0x8d, 0xf8, 0x21, 0x68, 0x39
};

static UCHAR rsaHash[] =
{
    0x96, 0x1f, 0xa6, 0x49, 0x58, 0x81, 0x8f, 0x76, 0x77, 0x07, 0x07, 0x27, 0x55, 0xd7, 0x01, 0x8d,
    0xcd, 0x27, 0x8e, 0x94
};

static UCHAR rsaSignature[] =
{
    0xa8, 0x3a, 0x9d, 0xaf, 0x92, 0x94, 0xa4, 0x4d, 0x34, 0xba, 0x41, 0x0c, 0xc1, 0x23, 0x91, 0xc7,
    0x91, 0xa8, 0xf8, 0xfc, 0x94, 0x87, 0x4d, 0x05, 0x85, 0x63, 0xe8, 0x7d, 0xea, 0x7f, 0x6b, 0x8d,
    0xbb, 0x9a, 0xd4, 0x46, 0xa6, 0xc0, 0xd6, 0xdc, 0x91, 0xba, 0xd3, 0x1a, 0xbf, 0xf4, 0x52, 0xa0,
    0xc7, 0x15, 0x87, 0xe9, 0x1e, 0x60, 0x49, 0x9c, 0xee, 0x5a, 0x9c, 0x6c, 0xbd, 0x7a, 0x3e, 0xc3,
    0x48, 0xb3, 0xee, 0xca, 0x68, 0x40, 0x9b, 0xa1, 0x4c, 0x6e, 0x20, 0xd6, 0xca, 0x6c, 0x72, 0xaf,
    0x2b, 0x6b, 0x62, 0x7c, 0x78, 0x06, 0x94, 0x4c, 0x02, 0xf3, 0x8d, 0x49, 0xe0, 0x11, 0xc4, 0x9b,
    0x62, 0x5b, 0xc2, 0xfd, 0x68, 0xf4, 0x07, 0x15, 0x71, 0x11, 0x4c, 0x35, 0x97, 0x5d, 0xc0, 0xe6,
    0x22, 0xc9, 0x8a, 0x7b, 0x96, 0xc9, 0xc3, 0xe4, 0x2b, 0x1e, 0x88, 0x17, 0x4f, 0x98, 0x9b, 0xf3,
    0x42, 0x23, 0x0c, 0xa0, 0xfa, 0x19, 0x03, 0x2a, 0xf7, 0x13, 0x2d, 0x27, 0xac, 0x9f, 0xaf, 0x2d,
    0xa3, 0xce, 0xf7, 0x63, 0xbb, 0x39, 0x9f, 0x72, 0x80, 0xdd, 0x6c, 0x73, 0x00, 0x85, 0x70, 0xf2,
    0xed, 0x50, 0xed, 0xa0, 0x74, 0x42, 0xd7, 0x22, 0x46, 0x24, 0xee, 0x67, 0xdf, 0xb5, 0x45, 0xe8,
    0x49, 0xf4, 0x9c, 0xe4, 0x00, 0x83, 0xf2, 0x27, 0x8e, 0xa2, 0xb1, 0xc3, 0xc2, 0x01, 0xd7, 0x59,
    0x2e, 0x4d, 0xac, 0x49, 0xa2, 0xc1, 0x8d, 0x88, 0x4b, 0xfe, 0x28, 0xe5, 0xac, 0xa6, 0x85, 0xc4,
    0x1f, 0xf8, 0xc5, 0xc5, 0x14, 0x4e, 0xa3, 0xcb, 0x17, 0xb7, 0x64, 0xb3, 0xc2, 0x12, 0xf8, 0xf8,
    0x36, 0x99, 0x1c, 0x91, 0x9b, 0xbd, 0xed, 0x55, 0x0f, 0xfd, 0x49, 0x85, 0xbb, 0x32, 0xad, 0x78,
    0xc1, 0x74, 0xe6, 0x7c, 0x18, 0x0f, 0x2b, 0x3b, 0xaa, 0xd1, 0x9d, 0x40, 0x71, 0x1d, 0x19, 0x53
};

static UCHAR rsaPrivateBlob[] =
{
    0x52, 0x53, 0x41, 0x32, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xa6, 0x8b, 0x46, 0x26, 0xb5,
    0xa9, 0x69, 0x83, 0x94, 0x66, 0xa7, 0xf3, 0x33, 0x95, 0x74, 0xe9, 0xeb, 0xc8, 0xcd, 0xd7, 0x81,
    0x9e, 0x45, 0x66, 0xb2, 0x48, 0x8b, 0x1f, 0xfe, 0xb3, 0x62, 0xc4, 0x0d, 0xa2, 0xf9, 0xf3, 0xe2,
    0xa6, 0x86, 0xd1, 0x1e, 0x8a, 0xbb, 0x1d, 0xa5, 0xc5, 0xe8, 0xa7, 0x50, 0x37, 0xfd, 0x69, 0x1f,
    0x6f, 0x99, 0x99, 0xca, 0x39, 0x13, 0xea, 0x5b, 0x6b, 0xe3, 0x91, 0xc0, 0xd2, 0x2c, 0x0b, 0x21,
    0xb1, 0xac, 0xa9, 0xe8, 0xa0, 0x6d, 0xa4, 0x1f, 0x1b, 0x34, 0xcb, 0x88, 0x7f, 0x2e, 0xeb, 0x7d,
    0x91, 0x38, 0x48, 0xce, 0x05, 0x73, 0x05, 0xdd, 0x22, 0x94, 0xc3, 0xdd, 0x1c, 0xfd, 0xc5, 0x41,
    0x2e, 0x94, 0xf9, 0xed, 0xe5, 0x92, 0x5f, 0x3f, 0x06, 0xf8, 0x49, 0x60, 0xb8, 0x92, 0x52, 0x6a,
    0x56, 0x6e, 0xd7, 0x04, 0x1a, 0xb5, 0xb5, 0x1c, 0x31, 0xd1, 0x1b,
};

static UCHAR rsaFullPrivateBlob[] =
{
    0x52, 0x53, 0x41, 0x33, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xa6, 0x8b, 0x46, 0x26, 0xb5,
    0xa9, 0x69, 0x83, 0x94, 0x66, 0xa7, 0xf3, 0x33, 0x95, 0x74, 0xe9, 0xeb, 0xc8, 0xcd, 0xd7, 0x81,
    0x9e, 0x45, 0x66, 0xb2, 0x48, 0x8b, 0x1f, 0xfe, 0xb3, 0x62, 0xc4, 0x0d, 0xa2, 0xf9, 0xf3, 0xe2,
    0xa6, 0x86, 0xd1, 0x1e, 0x8a, 0xbb, 0x1d, 0xa5, 0xc5, 0xe8, 0xa7, 0x50, 0x37, 0xfd, 0x69, 0x1f,
    0x6f, 0x99, 0x99, 0xca, 0x39, 0x13, 0xea, 0x5b, 0x6b, 0xe3, 0x91, 0xc0, 0xd2, 0x2c, 0x0b, 0x21,
    0xb1, 0xac, 0xa9, 0xe8, 0xa0, 0x6d, 0xa4, 0x1f, 0x1b, 0x34, 0xcb, 0x88, 0x7f, 0x2e, 0xeb, 0x7d,
    0x91, 0x38, 0x48, 0xce, 0x05, 0x73, 0x05, 0xdd, 0x22, 0x94, 0xc3, 0xdd, 0x1c, 0xfd, 0xc5, 0x41,
    0x2e, 0x94, 0xf9, 0xed, 0xe5, 0x92, 0x5f, 0x3f, 0x06, 0xf8, 0x49, 0x60, 0xb8, 0x92, 0x52, 0x6a,
    0x56, 0x6e, 0xd7, 0x04, 0x1a, 0xb5, 0xb5, 0x1c, 0x31, 0xd1, 0x1b, 0xa3, 0xf3, 0xd1, 0x69, 0x61,
    0xab, 0xfe, 0xc1, 0xb6, 0x40, 0x7b, 0x19, 0xbb, 0x2d, 0x59, 0xf5, 0xda, 0x49, 0x32, 0x6f, 0x20,
    0x24, 0xd3, 0xb3, 0xec, 0x21, 0xec, 0x0c, 0xc7, 0x5b, 0xf9, 0x1b, 0xba, 0x6e, 0xe9, 0x61, 0xda,
    0x55, 0xc6, 0x72, 0xfd, 0x2d, 0x66, 0x3f, 0x3c, 0xcb, 0x49, 0xa9, 0xc5, 0x0d, 0x9b, 0x02, 0x36,
    0x7a, 0xee, 0x36, 0x09, 0x55, 0xe4, 0x03, 0xf2, 0xe3, 0xe6, 0x25, 0x14, 0x89, 0x7f, 0x2b, 0xfb,
    0x27, 0x0e, 0x8d, 0x37, 0x84, 0xfd, 0xad, 0x10, 0x79, 0x43, 0x4e, 0x38, 0x4a, 0xd4, 0x5e, 0xfa,
    0xda, 0x9f, 0x88, 0x21, 0x7c, 0xb4, 0x98, 0xb6, 0x6e, 0x1c, 0x24, 0x09, 0xe5, 0xe7, 0x22, 0x6f,
    0xd3, 0x84, 0xc0, 0xdc, 0x36, 0x09, 0xaf, 0x4b, 0x96, 0x8b, 0x5f, 0x47, 0xb3, 0x24, 0x80, 0xb5,
    0x64, 0x69, 0xad, 0x83, 0xd5, 0x09, 0xe7, 0xb9, 0xe4, 0x81, 0x6f, 0x1a, 0xe2, 0x6d, 0xf1, 0x5e,
    0x2b, 0xb3, 0x7a, 0xd0, 0x77, 0xef, 0x82, 0xcd, 0x55, 0x2e, 0xd5, 0xb1, 0xa7, 0x72, 0xec, 0x02,
    0x9d, 0xe2, 0xcc, 0x5a, 0xf1, 0x68, 0x30, 0xe5, 0xbc, 0x8d, 0xad,
};


static UCHAR rsaPublicBlobWithInvalidPublicExpSize[] =
{
    0x52, 0x53, 0x41, 0x31, 0x00, 0x04, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x01, 0xc7, 0x8f, 0xac, 0x2a, 0xce, 0xbf, 0xc9, 0x6c, 0x7b,
    0x85, 0x74, 0x71, 0xbb, 0xff, 0xbb, 0x9b, 0x20, 0x03, 0x79, 0x17, 0x34,
    0xe7, 0x26, 0x91, 0x5c, 0x1f, 0x1b, 0x03, 0x3d, 0x46, 0xdf, 0xb6, 0xf2,
    0x10, 0x55, 0xf0, 0x39, 0x55, 0x0a, 0xe3, 0x9c, 0x0c, 0x63, 0xc2, 0x14,
    0x03, 0x94, 0x51, 0x0d, 0xb4, 0x22, 0x09, 0xf2, 0x5c, 0xb2, 0xd1, 0xc3,
    0xac, 0x6f, 0xa8, 0xc4, 0xac, 0xb8, 0xbc, 0x59, 0xe7, 0xed, 0x77, 0x6e,
    0xb1, 0x80, 0x58, 0x7d, 0xb2, 0x94, 0x46, 0xe5, 0x00, 0xe2, 0xb7, 0x33,
    0x48, 0x7a, 0xd3, 0x78, 0xe9, 0x26, 0x01, 0xc7, 0x00, 0x7b, 0x41, 0x6d,
    0x94, 0x3a, 0xe1, 0x50, 0x2b, 0x9f, 0x6b, 0x1c, 0x08, 0xa3, 0xfc, 0x0a,
    0x44, 0x81, 0x09, 0x41, 0x80, 0x23, 0x7b, 0xf6, 0x3f, 0xaf, 0x91, 0xa1,
    0x87, 0x75, 0x33, 0x15, 0xb8, 0xde, 0x32, 0x30, 0xb4, 0x5e, 0xfd
};

static void test_RSA(void)
{
    static UCHAR hash[] =
        {0x7e,0xe3,0x74,0xe7,0xc5,0x0b,0x6b,0x70,0xdb,0xab,0x32,0x6d,0x1d,0x51,0xd6,0x74,0x79,0x8e,0x5b,0x4b};
    BCRYPT_PKCS1_PADDING_INFO pad;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    BCRYPT_RSAKEY_BLOB *rsablob;
    UCHAR sig[64];
    ULONG len, size, size2, schemes;
    NTSTATUS ret;
    BYTE *buf;
    DWORD keylen;

    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_ALGORITHM, NULL, 0);
    if (ret)
    {
        win_skip("Failed to open RSA provider: %#lx, skipping test\n", ret);
        return;
    }

    schemes = size = 0;
    ret = BCryptGetProperty(alg, L"PaddingSchemes", (UCHAR *)&schemes, sizeof(schemes), &size, 0);
    ok(!ret, "got %#lx\n", ret);
    ok(schemes, "schemes not set\n");
    ok(size == sizeof(schemes), "got %lu\n", size);

    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &key, rsaPublicBlob, sizeof(rsaPublicBlob), 0);
    ok(!ret, "BCryptImportKeyPair failed: %#lx\n", ret);
    BCryptDestroyKey(key);

    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsaPublicBlob, sizeof(rsaPublicBlob), 0);
    ok(!ret, "BCryptImportKeyPair failed: %#lx\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = BCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(!ret, "BCryptVerifySignature failed: %#lx\n", ret);

    ret = BCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = BCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", ret);

    ret = BCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", ret);

    pad.pszAlgId = BCRYPT_AES_ALGORITHM;
    ret = BCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_NOT_SUPPORTED, "Expected STATUS_NOT_SUPPORTED, got %#lx\n", ret);

    pad.pszAlgId = NULL;
    ret = BCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_SIGNATURE, "Expected STATUS_INVALID_SIGNATURE, got %#lx\n", ret);

    ret = BCryptDestroyKey(key);
    ok(!ret, "BCryptDestroyKey failed: %#lx\n", ret);

    /* sign/verify with export/import round-trip */
    ret = BCryptGenerateKeyPair(alg, &key, 1024, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    keylen = 512;
    ret = BCryptSetProperty(key, BCRYPT_KEY_LENGTH, (UCHAR *)&keylen, 2, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);
    ret = BCryptSetProperty(key, BCRYPT_KEY_LENGTH, (UCHAR *)&keylen, sizeof(keylen), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptSetProperty(key, BCRYPT_KEY_LENGTH, (UCHAR *)&keylen, sizeof(keylen), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    memset(sig, 0, sizeof(sig));
    ret = BCryptSignHash(key, &pad, hash, sizeof(hash), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got %#lx\n", ret);

    /* export private key */
    size = 0;
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAPRIVATE_MAGIC, "got %#lx\n", rsablob->Magic);
    ok(rsablob->BitLength == 512, "got %lu\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %lu\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 64, "got %lu\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 32, "got %lu\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 32, "got %lu\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus + rsablob->cbPrime1 + rsablob->cbPrime2;
    ok(size == size2, "got %lu expected %lu\n", size2, size);
    HeapFree(GetProcessHeap(), 0, buf);

    size = 0;
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAFULLPRIVATE_MAGIC, "got %#lx\n", rsablob->Magic);
    ok(rsablob->BitLength == 512, "got %lu\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %lu\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 64, "got %lu\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 32, "got %lu\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 32, "got %lu\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus * 2 + rsablob->cbPrime1 * 3 + rsablob->cbPrime2 * 2;
    ok(size == size2, "got %lu expected %lu\n", size2, size);
    HeapFree(GetProcessHeap(), 0, buf);

    /* import/export public key */
    size = 0;
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPUBLIC_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPUBLIC_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAPUBLIC_MAGIC, "got %#lx\n", rsablob->Magic);
    ok(rsablob->BitLength == 512, "got %lu\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %lu\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 64, "got %lu\n", rsablob->cbModulus);
    ok(!rsablob->cbPrime1, "got %lu\n", rsablob->cbPrime1);
    ok(!rsablob->cbPrime2, "got %lu\n", rsablob->cbPrime2);
    ok(size == sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus, "got %lu\n", size);
    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsaPublicBlobWithInvalidPublicExpSize,
                              sizeof(rsaPublicBlobWithInvalidPublicExpSize), 0);
    ok(ret == NTE_BAD_DATA, "got %#lx\n", ret);

    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, buf, size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = BCryptVerifySignature(key, &pad, hash, sizeof(hash), sig, len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got %#lx\n", ret);
    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    /* import/export private key */
    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_RSAPRIVATE_BLOB, &key, rsaPrivateBlob, sizeof(rsaPrivateBlob), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    size = 0;
    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(rsaPrivateBlob));
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, buf, sizeof(rsaPrivateBlob), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(rsaPrivateBlob), "got %lu\n", size);
    ok(!memcmp(buf, rsaPrivateBlob, size), "wrong data\n");
    HeapFree(GetProcessHeap(), 0, buf);
    BCryptDestroyKey(key);

    /* import/export full private key */
    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, &key, rsaFullPrivateBlob, sizeof(rsaFullPrivateBlob), 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    size = 0;
    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(rsaFullPrivateBlob));
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, buf, sizeof(rsaFullPrivateBlob), &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(rsaFullPrivateBlob), "got %lu\n", size);
    ok(!memcmp(buf, rsaFullPrivateBlob, size), "wrong data\n");
    HeapFree(GetProcessHeap(), 0, buf);
    BCryptDestroyKey(key);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got %#lx\n", ret);
}

static void test_RSA_SIGN(void)
{
    BCRYPT_PKCS1_PADDING_INFO pad;
    BCRYPT_ALG_HANDLE alg = NULL;
    BCRYPT_KEY_HANDLE key = NULL;
    BCRYPT_RSAKEY_BLOB *rsablob;
    NTSTATUS ret;
    ULONG size, size2;
    BYTE *buf, buf2[sizeof(BCRYPT_RSAKEY_BLOB) + sizeof(rsaPublicBlob)];

    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_SIGN_ALGORITHM, NULL, 0);
    if (ret)
    {
        win_skip("Failed to open RSA_SIGN provider: %#lx, skipping test\n", ret);
        return;
    }

    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsaPublicBlob, sizeof(rsaPublicBlob), 0);
    ok(!ret, "BCryptImportKeyPair failed: %#lx\n", ret);

    memset(buf2, 0xcc, sizeof(buf2));
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPUBLIC_BLOB, buf2, sizeof(buf2), &size, 0);
    ok(!ret, "got %#lx\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf2;
    ok(rsablob->Magic == BCRYPT_RSAPUBLIC_MAGIC, "got %#lx\n", rsablob->Magic);
    ok(rsablob->BitLength == 2048, "got %lu\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %lu\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 256, "got %lu\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 0, "got %lu\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 0, "got %lu\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus + rsablob->cbPrime1 + rsablob->cbPrime2;
    ok(size == size2, "got %lu expected %lu\n", size2, size);

    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, buf2, sizeof(buf2), &size, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = BCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(!ret, "BCryptVerifySignature failed: %#lx\n", ret);

    ret = BCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", ret);

    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = BCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", ret);

    ret = BCryptVerifySignature(key, NULL, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), 0);
    ok(ret == STATUS_INVALID_PARAMETER, "Expected STATUS_INVALID_PARAMETER, got %#lx\n", ret);

    pad.pszAlgId = BCRYPT_AES_ALGORITHM;
    ret = BCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_NOT_SUPPORTED, "Expected STATUS_NOT_SUPPORTED, got %#lx\n", ret);

    pad.pszAlgId = NULL;
    ret = BCryptVerifySignature(key, &pad, rsaHash, sizeof(rsaHash), rsaSignature, sizeof(rsaSignature), BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_SIGNATURE, "Expected STATUS_INVALID_SIGNATURE, got %#lx\n", ret);

    ret = BCryptDestroyKey(key);
    ok(!ret, "BCryptDestroyKey failed: %#lx\n", ret);

    /* export private key */
    ret = BCryptGenerateKeyPair(alg, &key, 512, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    size = 0;
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAPRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAPRIVATE_MAGIC, "got %#lx\n", rsablob->Magic);
    ok(rsablob->BitLength == 512, "got %lu\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %lu\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 64, "got %lu\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 32, "got %lu\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 32, "got %lu\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus + rsablob->cbPrime1 + rsablob->cbPrime2;
    ok(size == size2, "got %lu expected %lu\n", size2, size);
    HeapFree(GetProcessHeap(), 0, buf);

    size = 0;
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    ret = BCryptExportKey(key, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    rsablob = (BCRYPT_RSAKEY_BLOB *)buf;
    ok(rsablob->Magic == BCRYPT_RSAFULLPRIVATE_MAGIC, "got %#lx\n", rsablob->Magic);
    ok(rsablob->BitLength == 512, "got %lu\n", rsablob->BitLength);
    ok(rsablob->cbPublicExp == 3, "got %lu\n", rsablob->cbPublicExp);
    ok(rsablob->cbModulus == 64, "got %lu\n", rsablob->cbModulus);
    ok(rsablob->cbPrime1 == 32, "got %lu\n", rsablob->cbPrime1);
    ok(rsablob->cbPrime2 == 32, "got %lu\n", rsablob->cbPrime2);
    size2 = sizeof(*rsablob) + rsablob->cbPublicExp + rsablob->cbModulus * 2 + rsablob->cbPrime1 * 3 + rsablob->cbPrime2 * 2;
    ok(size == size2, "got %lu expected %lu\n", size2, size);
    HeapFree(GetProcessHeap(), 0, buf);
    BCryptDestroyKey(key);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "BCryptCloseAlgorithmProvider failed: %#lx\n", ret);
}

static BYTE eccprivkey[] =
{
    0x45, 0x43, 0x4b, 0x32, 0x20, 0x00, 0x00, 0x00,
    0xfb, 0xbd, 0x3d, 0x20, 0x1b, 0x6d, 0x66, 0xb3, 0x7c, 0x9f, 0x89, 0xf3, 0xe4, 0x41, 0x16, 0xa5,
    0x68, 0x52, 0x77, 0xac, 0xab, 0x55, 0xb2, 0x6c, 0xb0, 0x23, 0x55, 0xcb, 0x96, 0x14, 0xfd, 0x0b,
    0x1c, 0xef, 0xdf, 0x07, 0x6d, 0x31, 0xaf, 0x39, 0xce, 0x8c, 0x8f, 0x9d, 0x75, 0xd0, 0x7b, 0xea,
    0x81, 0xdc, 0x40, 0x21, 0x1f, 0x58, 0x22, 0x5f, 0x72, 0x55, 0xfc, 0x58, 0x8a, 0xeb, 0x88, 0x5d,
    0x02, 0x09, 0x90, 0xd2, 0xe3, 0x36, 0xac, 0xfe, 0x83, 0x13, 0x6c, 0x88, 0x1a, 0xab, 0x9b, 0xdd,
    0xaa, 0x8a, 0xee, 0x69, 0x9a, 0x6a, 0x62, 0x86, 0x6a, 0x13, 0x69, 0x88, 0xb7, 0xd5, 0xa3, 0xcd
};

static BYTE ecdh_pubkey[] =
{
    0x45, 0x43, 0x4b, 0x31, 0x20, 0x00, 0x00, 0x00,
    0x07, 0x61, 0x9d, 0x49, 0x63, 0x6b, 0x96, 0x94, 0xd1, 0x8f, 0xd1, 0x48, 0xcc, 0xcf, 0x72, 0x4d,
    0xff, 0x43, 0xf4, 0x97, 0x0f, 0xa3, 0x8a, 0x72, 0xe9, 0xe0, 0xba, 0x87, 0x6d, 0xc3, 0x62, 0x15,
    0xae, 0x65, 0xdd, 0x31, 0x51, 0xfc, 0x3b, 0xc9, 0x59, 0xa1, 0x0a, 0x92, 0x17, 0x2b, 0x64, 0x55,
    0x03, 0x3e, 0x62, 0x1d, 0xac, 0x3e, 0x37, 0x40, 0x6a, 0x4c, 0xb6, 0x21, 0x3f, 0x73, 0x5c, 0xf5
};

/* little endian */
static BYTE ecdh_secret[] =
{
    0x48, 0xb0, 0x11, 0xdb, 0x69, 0x4e, 0xb4, 0xf4, 0xf5, 0x3e, 0xe1, 0x9b, 0xca, 0x00, 0x04, 0xc8,
    0x9b, 0x69, 0xaf, 0xd1, 0xaf, 0x1f, 0xc2, 0xd7, 0x83, 0x0a, 0xb7, 0xf8, 0x4f, 0x24, 0x32, 0x8e,
};

BCryptBuffer hash_param_buffers[] =
{
{
    sizeof(BCRYPT_SHA1_ALGORITHM),
    KDF_HASH_ALGORITHM,
    (void *)BCRYPT_SHA1_ALGORITHM,
}
};

BCryptBufferDesc hash_params =
{
    BCRYPTBUFFER_VERSION,
    ARRAY_SIZE(hash_param_buffers),
    hash_param_buffers,
};

static BYTE hashed_secret[] =
{
    0x1b, 0xe7, 0xbf, 0x0f, 0x65, 0x1e, 0xd0, 0x07, 0xf9, 0xf4, 0x77, 0x48, 0x48, 0x39, 0xd0, 0xf8,
    0xf3, 0xce, 0xfc, 0x89
};

static void test_ECDH(void)
{
    BYTE *buf;
    BCRYPT_ECCKEY_BLOB *ecckey;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key, privkey, pubkey;
    BCRYPT_SECRET_HANDLE secret;
    NTSTATUS status;
    ULONG size;

    status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
    if (status)
    {
        skip("Failed to open BCRYPT_ECDH_P256_ALGORITHM provider %#lx\n", status);
        return;
    }

    key = NULL;
    status = BCryptGenerateKeyPair(alg, &key, 256, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(key != NULL, "key not set\n");

    status = BCryptFinalizeKeyPair(key, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    size = 0;
    status = BCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    status = BCryptExportKey(key, NULL, BCRYPT_ECCPUBLIC_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ecckey = (BCRYPT_ECCKEY_BLOB *)buf;
    ok(ecckey->dwMagic == BCRYPT_ECDH_PUBLIC_P256_MAGIC, "got %#lx\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %lu\n", ecckey->cbKey);
    ok(size == sizeof(*ecckey) + ecckey->cbKey * 2, "got %lu\n", size);

    status = BCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &pubkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    BCryptDestroyKey(pubkey);

    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &pubkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    HeapFree(GetProcessHeap(), 0, buf);

    size = 0;
    status = BCryptExportKey(key, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    status = BCryptExportKey(key, NULL, BCRYPT_ECCPRIVATE_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ecckey = (BCRYPT_ECCKEY_BLOB *)buf;
    ok(ecckey->dwMagic == BCRYPT_ECDH_PRIVATE_P256_MAGIC, "got %#lx\n", ecckey->dwMagic);
    ok(ecckey->cbKey == 32, "got %lu\n", ecckey->cbKey);
    ok(size == sizeof(*ecckey) + ecckey->cbKey * 3, "got %lu\n", size);

    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &privkey, buf, size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    HeapFree(GetProcessHeap(), 0, buf);
    BCryptDestroyKey(pubkey);
    BCryptDestroyKey(privkey);
    BCryptDestroyKey(key);

    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPRIVATE_BLOB, &privkey, eccprivkey, sizeof(eccprivkey), 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    size = 0;
    status = BCryptExportKey(privkey, NULL, BCRYPT_ECCPRIVATE_BLOB, NULL, 0, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), 0, size);
    status = BCryptExportKey(privkey, NULL, BCRYPT_ECCPRIVATE_BLOB, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(size == sizeof(eccprivkey), "got %lu\n", size);
    ok(!memcmp(buf, eccprivkey, size), "wrong data\n");
    HeapFree(GetProcessHeap(), 0, buf);

    status = BCryptImportKeyPair(alg, NULL, BCRYPT_ECCPUBLIC_BLOB, &pubkey, ecdh_pubkey, sizeof(ecdh_pubkey), 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    status = BCryptSecretAgreement(privkey, pubkey, &secret, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    if (status != STATUS_SUCCESS)
    {
        goto derive_end;
    }

    /* verify result on windows 10 */
    status = BCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, NULL, 0, &size, 0);

    if (status == STATUS_NOT_SUPPORTED)
    {
        win_skip("BCRYPT_KDF_RAW_SECRET not supported\n");
        goto raw_secret_end;
    }

    todo_wine ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    if (status != STATUS_SUCCESS)
    {
        goto raw_secret_end;
    }

    ok(size == 32, "size of secret key incorrect, got %lu, expected 32\n", size);
    buf = HeapAlloc(GetProcessHeap(), 0, size);
    status = BCryptDeriveKey(secret, BCRYPT_KDF_RAW_SECRET, NULL, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(!(memcmp(ecdh_secret, buf, size)), "wrong data\n");
    HeapFree(GetProcessHeap(), 0, buf);

    raw_secret_end:

    status = BCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, NULL, 0, &size, 0);
    todo_wine ok (status == STATUS_SUCCESS, "got %#lx\n", status);

    if (status != STATUS_SUCCESS)
    {
        goto derive_end;
    }

    ok (size == 20, "got %lu\n", size);
    buf = HeapAlloc(GetProcessHeap(), 0, size);
    status = BCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, buf, size, &size, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(!(memcmp(hashed_secret, buf, size)), "wrong data\n");
    HeapFree(GetProcessHeap(), 0, buf);

    /* ulVersion is not verified */
    hash_params.ulVersion = 0xdeadbeef;
    status = BCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, NULL, 0, &size, 0);
    ok (status == STATUS_SUCCESS, "got %#lx\n", status);

    hash_params.ulVersion = BCRYPTBUFFER_VERSION;
    hash_param_buffers[0].pvBuffer = (void*) L"INVALID";
    hash_param_buffers[0].cbBuffer = sizeof(L"INVALID");

    status = BCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, NULL, 0, &size, 0);
    ok (status == STATUS_NOT_SUPPORTED || broken (status == STATUS_NOT_FOUND) /* < win8 */, "got %#lx\n", status);

    hash_param_buffers[0].pvBuffer = (void*) BCRYPT_RNG_ALGORITHM;
    hash_param_buffers[0].cbBuffer = sizeof(BCRYPT_RNG_ALGORITHM);
    status = BCryptDeriveKey(secret, BCRYPT_KDF_HASH, &hash_params, NULL, 0, &size, 0);
    ok (status == STATUS_NOT_SUPPORTED, "got %#lx\n", status);

    derive_end:

    BCryptDestroySecret(secret);
    BCryptDestroyKey(pubkey);
    BCryptDestroyKey(privkey);
    BCryptCloseAlgorithmProvider(alg, 0);
}

static void test_BCryptEnumContextFunctions(void)
{
    CRYPT_CONTEXT_FUNCTIONS *buffer;
    NTSTATUS status;
    ULONG buflen;

    buffer = NULL;
    status = BCryptEnumContextFunctions( CRYPT_LOCAL, L"SSL", NCRYPT_SCHANNEL_INTERFACE, &buflen, &buffer );
    todo_wine ok( status == STATUS_SUCCESS, "got %#lx\n", status);
    if (status == STATUS_SUCCESS) BCryptFreeBuffer( buffer );
}

static BYTE rsapublic[] =
{
    0x52, 0x53, 0x41, 0x31, 0x00, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0xd5, 0xfe, 0xf6, 0x7a, 0x9a, 0xa1, 0x2d, 0xcf, 0x98,
    0x60, 0xca, 0x38, 0x60, 0x0b, 0x74, 0x4c, 0x7e, 0xa1, 0x42, 0x64, 0xad, 0x05, 0xa5, 0x29, 0x25, 0xcb, 0xd5,
    0x9c, 0xaf, 0x6f, 0x63, 0x85, 0x6d, 0x5b, 0x59, 0xe5, 0x17, 0x8f, 0xf9, 0x18, 0x90, 0xa7, 0x63, 0xae, 0xe0,
    0x3a, 0x62, 0xf7, 0x98, 0x57, 0xe9, 0x91, 0xda, 0xfb, 0xd9, 0x36, 0x45, 0xe4, 0x9e, 0x75, 0xf6, 0x73, 0xc4,
    0x99, 0x23, 0x21, 0x1b, 0x3d, 0xe1, 0xe0, 0xa6, 0xa0, 0x4a, 0x50, 0x2a, 0xcb, 0x2a, 0x50, 0xf0, 0x8b, 0x70,
    0x9c, 0xe4, 0x1a, 0x14, 0x3b, 0xbe, 0x35, 0xa5, 0x5a, 0x91, 0xa3, 0xa1, 0x82, 0xea, 0x84, 0x4d, 0xe8, 0x62,
    0x3b, 0x11, 0xec, 0x61, 0x09, 0x6c, 0xfe, 0xb2, 0xcc, 0x4b, 0xa8, 0xff, 0xaf, 0x73, 0x72, 0x05, 0x4e, 0x7e,
    0xe5, 0x73, 0xdf, 0x24, 0xcf, 0x7f, 0x5d, 0xaf, 0x8a, 0xf0, 0xd8, 0xcb, 0x08, 0x1e, 0xf2, 0x36, 0x70, 0x8d,
    0x1b, 0x9e, 0xc8, 0x98, 0x60, 0x54, 0xeb, 0x45, 0x34, 0x21, 0x43, 0x4d, 0x42, 0x0a, 0x3a, 0x2d, 0x0f, 0x0e,
    0xd6, 0x0d, 0xe4, 0x2e, 0x8c, 0x31, 0x87, 0xa8, 0x09, 0x89, 0x61, 0x16, 0xca, 0x5b, 0xbe, 0x76, 0x69, 0xbb,
    0xfd, 0x91, 0x63, 0xd2, 0x66, 0x57, 0x08, 0xef, 0xe2, 0x40, 0x67, 0xd7, 0x7f, 0x50, 0x15, 0x42, 0x33, 0x97,
    0x54, 0x73, 0x47, 0xe7, 0x9c, 0x14, 0xa8, 0xb0, 0x3d, 0xc9, 0x23, 0xb0, 0x27, 0x3b, 0xe7, 0xdd, 0x5f, 0xd1,
    0x4f, 0x31, 0x10, 0x7d, 0xdd, 0x69, 0x8e, 0xde, 0xa3, 0xe8, 0x92, 0x00, 0xfa, 0xa5, 0xa4, 0x40, 0x51, 0x23,
    0x82, 0x84, 0xc7, 0xce, 0x19, 0x61, 0x26, 0xf1, 0xae, 0xf3, 0x90, 0x93, 0x98, 0x56, 0x23, 0x9a, 0xd1, 0xbd,
    0xf2, 0xdf, 0xfd, 0x13, 0x9c, 0x30, 0x07, 0xf9, 0x5a, 0x2e, 0x00, 0xc6, 0x1f
};

static void test_BCryptSignHash(void)
{
    static UCHAR hash[] =
        {0x7e,0xe3,0x74,0xe7,0xc5,0x0b,0x6b,0x70,0xdb,0xab,0x32,0x6d,0x1d,0x51,0xd6,0x74,0x79,0x8e,0x5b,0x4b};
    static UCHAR hash_sha256[] =
        {0x25,0x2f,0x10,0xc8,0x36,0x10,0xeb,0xca,0x1a,0x05,0x9c,0x0b,0xae,0x82,0x55,0xeb,0xa2,0xf9,0x5b,0xe4,
         0xd1,0xd7,0xbC,0xfA,0x89,0xd7,0x24,0x8a,0x82,0xd9,0xf1,0x11};
    BCRYPT_PKCS1_PADDING_INFO pad;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    UCHAR sig[256];
    NTSTATUS ret;
    ULONG len;

    /* RSA */
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_RSA_ALGORITHM, NULL, 0);
    if (ret)
    {
        win_skip("failed to open RSA provider: %#lx\n", ret);
        return;
    }

    /* public key */
    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_RSAPUBLIC_BLOB, &key, rsapublic, sizeof(rsapublic), 0);
    ok(!ret, "got %#lx\n", ret);

    len = 0;
    pad.pszAlgId = BCRYPT_SHA1_ALGORITHM;
    ret = BCryptSignHash(key, &pad, NULL, 0, NULL, 0, &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got %#lx\n", ret);
    ok(len == 256, "got %lu\n", len);

    /* test len return when only output is NULL, as described in BCryptSignHash doc */
    ret = BCryptSignHash(key, &pad, hash, sizeof(hash), NULL, 0, &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got %#lx\n", ret);
    ok(len == 256, "got %lu\n", len);

    len = 0;
    ret = BCryptSignHash(key, &pad, hash, sizeof(hash), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_INVALID_PARAMETER || broken(ret == STATUS_INTERNAL_ERROR) /* < win7 */, "got %#lx\n", ret);
    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptGenerateKeyPair(alg, &key, 512, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    len = 0;
    memset(sig, 0, sizeof(sig));

    /* inference of padding info on RSA not supported */
    ret = BCryptSignHash(key, NULL, hash, sizeof(hash), sig, sizeof(sig), &len, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    ret = BCryptSignHash(key, &pad, hash, sizeof(hash), sig, 0, &len, BCRYPT_PAD_PKCS1);
    ok(ret == STATUS_BUFFER_TOO_SMALL, "got %#lx\n", ret);

    ret = BCryptSignHash(key, &pad, hash, sizeof(hash), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got %#lx\n", ret);
    ok(len == 64, "got %lu\n", len);

    ret = BCryptVerifySignature(key, &pad, hash, sizeof(hash), sig, len, BCRYPT_PAD_PKCS1);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got %#lx\n", ret);

    /* ECDSA */
    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, NULL, 0);
    if (ret)
    {
        win_skip("failed to open ECDSA provider: %#lx\n", ret);
        return;
    }

    ret = BCryptGenerateKeyPair(alg, &key, 256, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    memset(sig, 0, sizeof(sig));
    len = 0;

    /* automatically detects padding info */
    ret = BCryptSignHash(key, NULL, hash, sizeof(hash), sig, sizeof(sig), &len, 0);
    ok (!ret, "got %#lx\n", ret);
    ok (len == 64, "got %lu\n", len);

    ret = BCryptVerifySignature(key, NULL, hash, sizeof(hash), sig, len, 0);
    ok(!ret, "got %#lx\n", ret);

    /* mismatch info (SHA-1 != SHA-256) */
    ret  = BCryptSignHash(key, &pad, hash_sha256, sizeof(hash_sha256), sig, sizeof(sig), &len, BCRYPT_PAD_PKCS1);
    ok (ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got %#lx\n", ret);
}

static void test_BCryptEnumAlgorithms(void)
{
    BCRYPT_ALGORITHM_IDENTIFIER *list;
    NTSTATUS ret;
    ULONG count;

    ret = BCryptEnumAlgorithms(0, NULL, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    ret = BCryptEnumAlgorithms(0, &count, NULL, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    ret = BCryptEnumAlgorithms(0, NULL, &list, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    ret = BCryptEnumAlgorithms(~0u, &count, &list, 0);
    ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    count = 0;
    list = NULL;
    ret = BCryptEnumAlgorithms(0, &count, &list, 0);
    ok(!ret, "got %#lx\n", ret);
    ok(list != NULL, "NULL list\n");
    ok(count, "got %lu\n", count);
    BCryptFreeBuffer( list );
}

static void test_aes_vector(void)
{
    static const UCHAR secret[] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10};
    static const UCHAR expect[] = {0xb0,0xcb,0xf5,0x80,0xd4,0xe3,0x55,0x23,0x6e,0x19,0x5b,0xdb,0xfe,0xe0,0x6c,0xd3};
    static const UCHAR expect2[] = {0x06,0x0c,0x81,0xab,0xd4,0x28,0x80,0x42,0xce,0x30,0x56,0x17,0x15,0x00,0x9e,0xc1};
    static const UCHAR expect3[] = {0x3e,0x99,0xbf,0x02,0xf5,0xd3,0xb8,0x81,0x91,0x4d,0x93,0xea,0xd4,0x92,0x93,0x46};
    static UCHAR iv[16], input[] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p'};
    UCHAR output[16];
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    UCHAR data[sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + sizeof(secret)];
    BCRYPT_KEY_DATA_BLOB_HEADER *blob = (BCRYPT_KEY_DATA_BLOB_HEADER *)data;
    ULONG size;
    NTSTATUS ret;

    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_AES_ALGORITHM, NULL, 0);
    ok(!ret, "got %#lx\n", ret);

    size = sizeof(BCRYPT_CHAIN_MODE_CBC);
    ret = BCryptSetProperty(alg, BCRYPT_CHAINING_MODE, (UCHAR *)BCRYPT_CHAIN_MODE_CBC, size, 0);
    ok(!ret, "got %#lx\n", ret);

    blob->dwMagic   = BCRYPT_KEY_DATA_BLOB_MAGIC;
    blob->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
    blob->cbKeyData = sizeof(secret);
    memcpy(data + sizeof(*blob), secret, sizeof(secret));
    size = sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + sizeof(secret);
    ret = BCryptImportKey(alg, NULL, BCRYPT_KEY_DATA_BLOB, &key, NULL, 0, data, size, 0);
    ok(!ret || broken(ret == STATUS_INVALID_PARAMETER) /* vista */, "got %#lx\n", ret);
    if (ret == STATUS_INVALID_PARAMETER)
    {
        win_skip("broken BCryptImportKey\n");
        BCryptCloseAlgorithmProvider(alg, 0);
        return;
    }

    /* zero initialization vector */
    size = 0;
    memset(output, 0, sizeof(output));
    ret = BCryptEncrypt(key, input, sizeof(input), NULL, iv, sizeof(iv), output, sizeof(output), &size, 0);
    ok(!ret, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    ok(!memcmp(output, expect, sizeof(expect)), "wrong cipher text\n");

    /* same initialization vector */
    size = 0;
    memset(output, 0, sizeof(output));
    ret = BCryptEncrypt(key, input, sizeof(input), NULL, iv, sizeof(iv), output, sizeof(output), &size, 0);
    ok(!ret, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    ok(!memcmp(output, expect2, sizeof(expect2)), "wrong cipher text\n");

    /* different initialization vector */
    iv[0] = 0x1;
    size = 0;
    memset(output, 0, sizeof(output));
    ret = BCryptEncrypt(key, input, sizeof(input), NULL, iv, sizeof(iv), output, sizeof(output), &size, 0);
    ok(!ret, "got %#lx\n", ret);
    ok(size == 16, "got %lu\n", size);
    todo_wine ok(!memcmp(output, expect3, sizeof(expect3)), "wrong cipher text\n");

    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got %#lx\n", ret);
}

static void test_BcryptDeriveKeyCapi(void)
{
    static const UCHAR expect[] =
        {0xda,0x39,0xa3,0xee,0x5e,0x6b,0x4b,0x0d,0x32,0x55,0xbf,0xef,0x95,0x60,0x18,0x90,0xaf,0xd8,0x07,0x09};
    static const UCHAR expect2[] =
        {0x9b,0x03,0x17,0x41,0xf4,0x75,0x11,0xac,0xff,0x22,0xee,0x40,0xbb,0xe8,0xf9,0x74,0x17,0x26,0xb6,0xf2,
         0xf8,0xc7,0x88,0x02,0x9a,0xdc,0x0d,0xd7,0x83,0x58,0xea,0x65,0x2e,0x8b,0x85,0xc6,0xdb,0xb7,0xed,0x1c};
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_HASH_HANDLE hash;
    UCHAR key[40];
    NTSTATUS ret;

    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, NULL, 0);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret || broken(ret == STATUS_INVALID_PARAMETER) /* win2k8 */, "got %#lx\n", ret);
    if (ret == STATUS_INVALID_PARAMETER)
    {
        win_skip( "broken BCryptCreateHash\n" );
        return;
    }

    ret = BCryptDeriveKeyCapi(NULL, NULL, NULL, 0, 0);
    ok(ret == STATUS_INVALID_PARAMETER || ret == STATUS_INVALID_HANDLE /* win7 */, "got %#lx\n", ret);

    ret = BCryptDeriveKeyCapi(hash, NULL, NULL, 0, 0);
    ok(ret == STATUS_INVALID_PARAMETER || !ret /* win7 */, "got %#lx\n", ret);

    ret = BCryptDestroyHash(hash);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptDeriveKeyCapi(hash, NULL, key, 0, 0);
    ok(ret == STATUS_INVALID_PARAMETER || !ret /* win7 */, "got %#lx\n", ret);

    ret = BCryptDestroyHash(hash);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got %#lx\n", ret);

    memset(key, 0, sizeof(key));
    ret = BCryptDeriveKeyCapi(hash, NULL, key, 41, 0);
    ok(ret == STATUS_INVALID_PARAMETER || !ret /* win7 */, "got %#lx\n", ret);
    if (!ret)
        ok(!memcmp(key, expect, sizeof(expect) - 1), "wrong key data\n");

    ret = BCryptDestroyHash(hash);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got %#lx\n", ret);

    memset(key, 0, sizeof(key));
    ret = BCryptDeriveKeyCapi(hash, NULL, key, 20, 0);
    ok(!ret, "got %#lx\n", ret);
    ok(!memcmp(key, expect, sizeof(expect) - 1), "wrong key data\n");

    ret = BCryptDeriveKeyCapi(hash, NULL, key, 20, 0);
    todo_wine ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = BCryptHashData(hash, NULL, 0, 0);
    todo_wine ok(ret == STATUS_INVALID_HANDLE, "got %#lx\n", ret);

    ret = BCryptDestroyHash(hash);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCreateHash(alg, &hash, NULL, 0, NULL, 0, 0);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptHashData(hash, (UCHAR *)"test", 4, 0);
    ok(!ret, "got %#lx\n", ret);

    /* padding */
    memset(key, 0, sizeof(key));
    ret = BCryptDeriveKeyCapi(hash, NULL, key, 40, 0);
    ok(!ret, "got %#lx\n", ret);
    ok(!memcmp(key, expect2, sizeof(expect2) - 1), "wrong key data\n");

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got %#lx\n", ret);
}

static UCHAR dsaHash[] =
{
    0x7e,0xe3,0x74,0xe7,0xc5,0x0b,0x6b,0x70,0xdb,0xab,0x32,0x6d,0x1d,0x51,0xd6,0x74,0x79,0x8e,0x5b,0x4b
};

static UCHAR dsaSignature[] =
{
    0x5f,0x95,0x1f,0x08,0x19,0x44,0xa5,0xab,0x28,0x11,0x51,0x68,0x82,0x9b,0xe4,0xc3,0x04,0x1b,0xc9,0xdc,
    0x41,0x2a,0x89,0xd4,0x4a,0x8b,0x86,0xaf,0x98,0x2c,0x59,0x0b,0xd2,0x88,0xf6,0xe8,0x29,0x13,0x84,0x49
};

static UCHAR dsaPublicBlob[] =
{
    0x44,0x53,0x50,0x42,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x8f,0xd2,0x92,0xbb,0x92,0xb9,0x00,0xc5,0xed,
    0x52,0xcc,0x48,0x4a,0x44,0x1d,0xd3,0x74,0xfb,0x75,0xd1,0x7e,0xb6,0x24,0x9b,0x5d,0x57,0x0a,0x8a,0xc4,
    0x5d,0xab,0x9c,0x26,0x86,0xc6,0x25,0x16,0x20,0xf9,0xa9,0x71,0xbc,0x1d,0x30,0xc4,0xef,0x8c,0xc4,0xdf,
    0x1a,0xaf,0x96,0xdf,0x90,0xd8,0x85,0x9d,0xf9,0x2c,0x86,0x8c,0x91,0x39,0x6c,0x6d,0x11,0x4e,0x53,0x63,
    0x2a,0x2b,0x26,0xa7,0xf9,0x76,0x74,0x51,0xbf,0x08,0x87,0x6f,0xe0,0x71,0x91,0x24,0x8a,0xc2,0x84,0x2d,
    0x84,0x9c,0x5f,0x94,0xaa,0x38,0x53,0x77,0x84,0xba,0xbc,0xff,0x49,0x3a,0x08,0x0f,0x38,0xb5,0x91,0x5c,
    0x06,0x15,0xa4,0x27,0xf4,0xa5,0x59,0xaa,0x1c,0x41,0xa3,0xa0,0xbb,0xf7,0x32,0x86,0xfb,0x94,0x41,0xff,
    0xcd,0xed,0x69,0xeb,0xc6,0x5e,0xb6,0xa8,0x15,0x82,0x3b,0x60,0x1e,0x91,0x55,0xd5,0x2c,0xa5,0x74,0x5a,
    0x65,0x8f,0xc6,0x56,0xc4,0x3f,0x4e,0xe3,0x3a,0x71,0xb2,0x63,0x66,0xa4,0x0d,0x0d,0xf9,0xdd,0x1e,0x48,
    0x81,0xe9,0xbf,0x8f,0xbb,0x85,0x47,0x81,0x68,0x11,0xb5,0x91,0x6b,0xc4,0x05,0xef,0xa3,0xc7,0xbf,0x26,
    0x53,0x4f,0xc4,0x10,0xfd,0xfa,0xed,0x61,0x64,0xd6,0x2e,0xad,0x04,0x3e,0x82,0xed,0xb2,0x22,0x76,0xd0,
    0x44,0xad,0xc1,0x4c,0xde,0x33,0xa3,0x61,0x55,0xec,0x24,0xe5,0x79,0x45,0xcf,0x94,0x39,0x92,0x9f,0xd8,
    0x24,0xce,0x85,0xb9
};

static UCHAR dssKey[] =
{
    0x07,0x02,0x00,0x00,0x00,0x22,0x00,0x00,0x44,0x53,0x53,0x32,0x00,0x04,0x00,0x00,0x01,0xd1,0xfc,0x7a,
    0x70,0x53,0xb2,0x48,0x70,0x23,0x19,0x1f,0x3c,0xe1,0x26,0x14,0x7e,0x9f,0x0f,0x7f,0x33,0x5e,0x2b,0xf7,
    0xca,0x01,0x74,0x8c,0xb4,0xfd,0xf6,0x44,0x95,0x35,0x56,0xaa,0x4d,0x62,0x48,0xe2,0xd1,0xa2,0x7e,0x6e,
    0xeb,0xd6,0xcc,0x7c,0xe8,0xfd,0x21,0x9a,0xa2,0xfd,0x7a,0x9d,0x1a,0x38,0x69,0x87,0x39,0x5a,0x91,0xc0,
    0x52,0x2b,0x9f,0x2a,0x54,0x78,0x37,0x82,0x9a,0x70,0x57,0xab,0xec,0x93,0x8e,0xac,0x73,0x04,0xe8,0x53,
    0x72,0x72,0x32,0xc6,0xcb,0xef,0x47,0x98,0x3c,0x56,0x49,0x62,0xcb,0xbb,0xe7,0x34,0x84,0xa6,0x72,0x3a,
    0xbe,0x26,0x46,0x86,0xca,0xcb,0x35,0x62,0x4f,0x19,0x18,0x0b,0xb0,0x78,0xae,0xd5,0x42,0xdf,0x26,0xdb,
    0x85,0x63,0x77,0x85,0x01,0x3b,0x32,0xbe,0x5c,0xf8,0x05,0xc8,0xde,0x17,0x7f,0xb9,0x03,0x82,0xfa,0xf1,
    0x9e,0x32,0x73,0xfa,0x8d,0xea,0xa3,0x30,0x48,0xe2,0xdf,0x5a,0xcb,0x83,0x3d,0xff,0x56,0xe9,0xc0,0x94,
    0xf8,0x6d,0xb3,0xaf,0x4a,0x97,0xb9,0x43,0x0e,0xd4,0x28,0x98,0x57,0x2e,0x3a,0xca,0xde,0x6f,0x45,0x0d,
    0xfb,0x58,0xec,0x78,0x34,0x2e,0x46,0x4d,0xfe,0x98,0x02,0xbb,0xef,0x07,0x1a,0x13,0xb6,0xc2,0x2c,0x06,
    0xd9,0x0c,0xc4,0xb0,0x4c,0x3a,0xfc,0x01,0x63,0xb5,0x5a,0x5d,0x2d,0x9c,0x47,0x04,0x67,0x51,0xf2,0x52,
    0xf5,0x82,0x36,0xeb,0x6e,0x66,0x58,0x4c,0x10,0x2c,0x29,0x72,0x4a,0x6f,0x6b,0x6c,0xe0,0x93,0x31,0x42,
    0xf6,0xda,0xfa,0x5b,0x22,0x43,0x9b,0x1a,0x98,0x71,0xe7,0x41,0x74,0xe9,0x12,0xa4,0x1f,0x27,0x0a,0x63,
    0x94,0x49,0xd7,0xad,0xa5,0xc4,0x5c,0xc3,0xc9,0x70,0xb3,0x7b,0x16,0xb6,0x1d,0xd4,0x09,0xc4,0x9a,0x46,
    0x2d,0x0e,0x75,0x07,0x31,0x7b,0xed,0x45,0xcd,0x99,0x84,0x14,0xf1,0x01,0x00,0x00,0x93,0xd5,0xa3,0xe4,
    0x34,0x05,0xeb,0x98,0x3b,0x5f,0x2f,0x11,0xa4,0xa5,0xc4,0xff,0xfb,0x22,0x7c,0x54
};

static void test_DSA(void)
{
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    BCRYPT_DSA_KEY_BLOB *dsablob;
    UCHAR sig[40], schemes;
    ULONG len, size;
    NTSTATUS ret;
    BYTE *buf, buf2[sizeof(BCRYPT_DSA_KEY_BLOB) + sizeof(dsaPublicBlob)];

    ret = BCryptOpenAlgorithmProvider(&alg, BCRYPT_DSA_ALGORITHM, NULL, 0);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptGetProperty(alg, L"PaddingSchemes", (UCHAR *)&schemes, sizeof(schemes), &size, 0);
    ok(ret == STATUS_NOT_SUPPORTED, "got %#lx\n", ret);

    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_PUBLIC_KEY_BLOB, &key, dsaPublicBlob, sizeof(dsaPublicBlob), 0);
    ok(!ret, "got %#lx\n", ret);
    BCryptDestroyKey(key);

    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_DSA_PUBLIC_BLOB, &key, dsaPublicBlob, sizeof(dsaPublicBlob), 0);
    ok(!ret, "got %#lx\n", ret);

    memset(buf2, 0xcc, sizeof(buf2));
    ret = BCryptExportKey(key, NULL, BCRYPT_DSA_PUBLIC_BLOB, buf2, sizeof(buf2), &size, 0);
    ok(!ret, "got %#lx\n", ret);
    dsablob = (BCRYPT_DSA_KEY_BLOB *)buf2;
    ok(dsablob->dwMagic == BCRYPT_DSA_PUBLIC_MAGIC, "got %#lx\n", dsablob->dwMagic);
    ok(dsablob->cbKey == 64, "got %lu\n", dsablob->cbKey);
    ok(size == sizeof(*dsablob) + dsablob->cbKey * 3, "got %lu\n", size);

    ret = BCryptExportKey(key, NULL, BCRYPT_DSA_PRIVATE_BLOB, buf2, sizeof(buf2), &size, 0);
    todo_wine ok(ret == STATUS_INVALID_PARAMETER, "got %#lx\n", ret);

    ret = BCryptVerifySignature(key, NULL, dsaHash, sizeof(dsaHash), dsaSignature, sizeof(dsaSignature), 0);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    /* sign/verify with export/import round-trip */
    ret = BCryptGenerateKeyPair(alg, &key, 512, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    ret = BCryptFinalizeKeyPair(key, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);

    len = 0;
    memset(sig, 0, sizeof(sig));
    ret = BCryptSignHash(key, NULL, dsaHash, sizeof(dsaHash), sig, sizeof(sig), &len, 0);
    ok(!ret, "got %#lx\n", ret);
    ok(len == 40, "got %lu\n", len);

    size = 0;
    ret = BCryptExportKey(key, NULL, BCRYPT_DSA_PUBLIC_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    ret = BCryptExportKey(key, NULL, BCRYPT_DSA_PUBLIC_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    dsablob = (BCRYPT_DSA_KEY_BLOB *)buf;
    ok(dsablob->dwMagic == BCRYPT_DSA_PUBLIC_MAGIC, "got %#lx\n", dsablob->dwMagic);
    ok(dsablob->cbKey == 64, "got %lu\n", dsablob->cbKey);
    ok(size == sizeof(*dsablob) + dsablob->cbKey * 3, "got %lu\n", size);

    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptImportKeyPair(alg, NULL, BCRYPT_DSA_PUBLIC_BLOB, &key, buf, size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    HeapFree(GetProcessHeap(), 0, buf);

    ret = BCryptVerifySignature(key, NULL, dsaHash, sizeof(dsaHash), sig, len, 0);
    ok(!ret, "got %#lx\n", ret);
    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptImportKeyPair(alg, NULL, LEGACY_DSA_V2_PRIVATE_BLOB, &key, dssKey, sizeof(dssKey), 0);
    ok(!ret, "got %#lx\n", ret);

    size = 0;
    ret = BCryptExportKey(key, NULL, LEGACY_DSA_V2_PRIVATE_BLOB, NULL, 0, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size, "size not set\n");

    buf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    ret = BCryptExportKey(key, NULL, LEGACY_DSA_V2_PRIVATE_BLOB, buf, size, &size, 0);
    ok(ret == STATUS_SUCCESS, "got %#lx\n", ret);
    ok(size == sizeof(dssKey), "got %lu expected %Iu\n", size, sizeof(dssKey));
    ok(!memcmp(dssKey, buf, size), "wrong data\n");
    HeapFree(GetProcessHeap(), 0, buf);

    ret = BCryptDestroyKey(key);
    ok(!ret, "got %#lx\n", ret);

    ret = BCryptCloseAlgorithmProvider(alg, 0);
    ok(!ret, "got %#lx\n", ret);
}

static void test_SecretAgreement(void)
{
    BCRYPT_SECRET_HANDLE secret;
    BCRYPT_ALG_HANDLE alg;
    BCRYPT_KEY_HANDLE key;
    NTSTATUS status;
    ULONG size;

    status = BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDH_P256_ALGORITHM, NULL, 0);
    if (status)
    {
        skip("Failed to open BCRYPT_ECDH_P256_ALGORITHM provider %#lx\n", status);
        return;
    }

    key = NULL;
    status = BCryptGenerateKeyPair(alg, &key, 256, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    ok(key != NULL, "key not set\n");

    status = BCryptFinalizeKeyPair(key, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    status = BCryptSecretAgreement(NULL, key, &secret, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %#lx\n", status);

    status = BCryptSecretAgreement(key, NULL, &secret, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %#lx\n", status);

    status = BCryptSecretAgreement(key, key, NULL, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);

    status = BCryptSecretAgreement(key, key, &secret, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    status = BCryptDeriveKey(NULL, L"HASH", NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %#lx\n", status);

    status = BCryptDeriveKey(key, L"HASH", NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_HANDLE, "got %#lx\n", status);

    status = BCryptDeriveKey(secret, NULL, NULL, NULL, 0, &size, 0);
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);

    status = BCryptDeriveKey(secret, L"HASH", NULL, NULL, 0, &size, 0);
    todo_wine
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    status = BCryptDestroyHash(secret);
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);

    status = BCryptDestroyKey(secret);
    ok(status == STATUS_INVALID_HANDLE, "got %#lx\n", status);

    status = BCryptDestroySecret(NULL);
    ok(status == STATUS_INVALID_HANDLE, "got %#lx\n", status);

    status = BCryptDestroySecret(alg);
    ok(status == STATUS_INVALID_HANDLE, "got %#lx\n", status);

    status = BCryptDestroySecret(secret);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    status = BCryptDestroyKey(key);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    status = BCryptCloseAlgorithmProvider(alg, 0);
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
}

START_TEST(bcrypt)
{
    HMODULE module;

    module = LoadLibraryA("bcrypt.dll");
    if (!module)
    {
        win_skip("bcrypt.dll not found\n");
        return;
    }
    pBCryptHash = (void *)GetProcAddress(module, "BCryptHash");

    test_BCryptGenRandom();
    test_BCryptGetFipsAlgorithmMode();
    test_hashes();
    test_BcryptHash();
    test_BcryptDeriveKeyPBKDF2();
    test_rng();
    test_3des();
    test_aes();
    test_BCryptGenerateSymmetricKey();
    test_BCryptEncrypt();
    test_BCryptDecrypt();
    test_key_import_export();
    test_ECDSA();
    test_RSA();
    test_RSA_SIGN();
    test_ECDH();
    test_BCryptEnumContextFunctions();
    test_BCryptSignHash();
    test_BCryptEnumAlgorithms();
    test_aes_vector();
    test_BcryptDeriveKeyCapi();
    test_DSA();
    test_SecretAgreement();

    FreeLibrary(module);
}
