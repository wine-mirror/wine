/*
 * Miscellaneous crypt32 tests
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
#include <winreg.h>

#include "wine/test.h"

HMODULE hCrypt;

static void test_findAttribute(void)
{
    PCRYPT_ATTRIBUTE ret;
    BYTE blobbin[] = {0x02,0x01,0x01};
    static CHAR oid[] = "1.2.3";
    CRYPT_ATTR_BLOB blobs[] = { { sizeof blobbin, blobbin }, };
    CRYPT_ATTRIBUTE attr = { oid, sizeof(blobs) / sizeof(blobs[0]), blobs };

    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute(NULL, 0, NULL);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    if (0)
    {
        /* crashes */
        SetLastError(0xdeadbeef);
        ret = CertFindAttribute(NULL, 1, NULL);
        /* returns NULL, last error is ERROR_INVALID_PARAMETER
         * crashes on Vista
         */
        SetLastError(0xdeadbeef);
        ret = CertFindAttribute(NULL, 1, &attr);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d (%08x)\n", GetLastError(),
         GetLastError());
    }
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("bogus", 1, &attr);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("1.2.4", 1, &attr);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("1.2.3", 1, &attr);
    ok(ret != NULL, "CertFindAttribute failed: %08x\n", GetLastError());
}

static void test_findExtension(void)
{
    PCERT_EXTENSION ret;
    static CHAR oid[] = "1.2.3";
    BYTE blobbin[] = {0x02,0x01,0x01};
    CERT_EXTENSION ext = { oid, TRUE, { sizeof blobbin, blobbin } };

    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension(NULL, 0, NULL);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    if (0)
    {
        /* crashes */
        SetLastError(0xdeadbeef);
        ret = CertFindExtension(NULL, 1, NULL);
        /* returns NULL, last error is ERROR_INVALID_PARAMETER
         * crashes on Vista
         */
        SetLastError(0xdeadbeef);
        ret = CertFindExtension(NULL, 1, &ext);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d (%08x)\n", GetLastError(),
         GetLastError());
    }
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("bogus", 1, &ext);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("1.2.4", 1, &ext);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("1.2.3", 1, &ext);
    ok(ret != NULL, "CertFindExtension failed: %08x\n", GetLastError());
}

static void test_findRDNAttr(void)
{
    PCERT_RDN_ATTR ret;
    static CHAR oid[] = "1.2.3";
    BYTE bin[] = { 0x16,0x09,'J','u','a','n',' ','L','a','n','g' };
    CERT_RDN_ATTR attrs[] = {
     { oid, CERT_RDN_IA5_STRING, { sizeof bin, bin } },
    };
    CERT_RDN rdns[] = {
     { sizeof(attrs) / sizeof(attrs[0]), attrs },
    };
    CERT_NAME_INFO nameInfo = { sizeof(rdns) / sizeof(rdns[0]), rdns };

    if (0)
    {
        /* crashes */
        SetLastError(0xdeadbeef);
        ret = CertFindRDNAttr(NULL, NULL);
        /* returns NULL, last error is ERROR_INVALID_PARAMETER
         * crashes on Vista
         */
        SetLastError(0xdeadbeef);
        ret = CertFindRDNAttr(NULL, &nameInfo);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d (%08x)\n", GetLastError(),
         GetLastError());
    }
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("bogus", &nameInfo);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("1.2.4", &nameInfo);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08x\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("1.2.3", &nameInfo);
    ok(ret != NULL, "CertFindRDNAttr failed: %08x\n", GetLastError());
}

static void test_verifyTimeValidity(void)
{
    SYSTEMTIME sysTime;
    FILETIME fileTime;
    CERT_INFO info = { 0 };
    LONG ret;

    GetSystemTime(&sysTime);
    SystemTimeToFileTime(&sysTime, &fileTime);
    /* crashes
    ret = CertVerifyTimeValidity(NULL, NULL);
    ret = CertVerifyTimeValidity(&fileTime, NULL);
     */
    /* Check with 0 NotBefore and NotAfter */
    ret = CertVerifyTimeValidity(&fileTime, &info);
    ok(ret == 1, "Expected 1, got %d\n", ret);
    memcpy(&info.NotAfter, &fileTime, sizeof(info.NotAfter));
    /* Check with NotAfter equal to comparison time */
    ret = CertVerifyTimeValidity(&fileTime, &info);
    ok(ret == 0, "Expected 0, got %d\n", ret);
    /* Check with NotBefore after comparison time */
    memcpy(&info.NotBefore, &fileTime, sizeof(info.NotBefore));
    info.NotBefore.dwLowDateTime += 5000;
    ret = CertVerifyTimeValidity(&fileTime, &info);
    ok(ret == -1, "Expected -1, got %d\n", ret);
}

static void test_cryptAllocate(void)
{
    LPVOID buf;

    buf = CryptMemAlloc(0);
    ok(buf != NULL, "CryptMemAlloc failed: %08x\n", GetLastError());
    CryptMemFree(buf);
    buf = CryptMemRealloc(NULL, 0);
    ok(!buf, "Expected NULL\n");
    buf = CryptMemAlloc(0);
    buf = CryptMemRealloc(buf, 1);
    ok(buf != NULL, "CryptMemRealloc failed: %08x\n", GetLastError());
    CryptMemFree(buf);
}

typedef DWORD  (WINAPI *I_CryptAllocTlsFunc)(void);
typedef LPVOID (WINAPI *I_CryptDetachTlsFunc)(DWORD dwTlsIndex);
typedef LPVOID (WINAPI *I_CryptGetTlsFunc)(DWORD dwTlsIndex);
typedef BOOL   (WINAPI *I_CryptSetTlsFunc)(DWORD dwTlsIndex, LPVOID lpTlsValue);
typedef BOOL   (WINAPI *I_CryptFreeTlsFunc)(DWORD dwTlsIndex, DWORD unknown);

static I_CryptAllocTlsFunc pI_CryptAllocTls;
static I_CryptDetachTlsFunc pI_CryptDetachTls;
static I_CryptGetTlsFunc pI_CryptGetTls;
static I_CryptSetTlsFunc pI_CryptSetTls;
static I_CryptFreeTlsFunc pI_CryptFreeTls;

static void test_cryptTls(void)
{
    DWORD index;
    BOOL ret;

    pI_CryptAllocTls = (I_CryptAllocTlsFunc)GetProcAddress(hCrypt,
     "I_CryptAllocTls");
    pI_CryptDetachTls = (I_CryptDetachTlsFunc)GetProcAddress(hCrypt,
     "I_CryptDetachTls");
    pI_CryptGetTls = (I_CryptGetTlsFunc)GetProcAddress(hCrypt,
     "I_CryptGetTls");
    pI_CryptSetTls = (I_CryptSetTlsFunc)GetProcAddress(hCrypt,
     "I_CryptSetTls");
    pI_CryptFreeTls = (I_CryptFreeTlsFunc)GetProcAddress(hCrypt,
     "I_CryptFreeTls");

    /* One normal pass */
    index = pI_CryptAllocTls();
    ok(index, "I_CryptAllocTls failed: %08x\n", GetLastError());
    if (index)
    {
        LPVOID ptr;

        ptr = pI_CryptGetTls(index);
        ok(!ptr, "Expected NULL\n");
        ret = pI_CryptSetTls(index, (LPVOID)0xdeadbeef);
        ok(ret, "I_CryptSetTls failed: %08x\n", GetLastError());
        ptr = pI_CryptGetTls(index);
        ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
        /* This crashes
        ret = pI_CryptFreeTls(index, 1);
         */
        ret = pI_CryptFreeTls(index, 0);
        ok(ret, "I_CryptFreeTls failed: %08x\n", GetLastError());
        ret = pI_CryptFreeTls(index, 0);
        /* Not sure if this fails because TlsFree should fail, so leave as
         * todo for now.
         */
        todo_wine ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
    }
    /* Similar pass, check I_CryptDetachTls */
    index = pI_CryptAllocTls();
    ok(index, "I_CryptAllocTls failed: %08x\n", GetLastError());
    if (index)
    {
        LPVOID ptr;

        ptr = pI_CryptGetTls(index);
        ok(!ptr, "Expected NULL\n");
        ret = pI_CryptSetTls(index, (LPVOID)0xdeadbeef);
        ok(ret, "I_CryptSetTls failed: %08x\n", GetLastError());
        ptr = pI_CryptGetTls(index);
        ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
        ptr = pI_CryptDetachTls(index);
        ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
        ptr = pI_CryptGetTls(index);
        ok(!ptr, "Expected NULL\n");
    }
}

typedef BOOL (WINAPI *I_CryptReadTrustedPublisherDWORDValueFromRegistryFunc)
 (LPCWSTR, DWORD *);

static void test_readTrustedPublisherDWORD(void)
{
    I_CryptReadTrustedPublisherDWORDValueFromRegistryFunc pReadDWORD;

    pReadDWORD = 
     (I_CryptReadTrustedPublisherDWORDValueFromRegistryFunc)GetProcAddress(
     hCrypt, "I_CryptReadTrustedPublisherDWORDValueFromRegistry");
    if (pReadDWORD)
    {
        static const WCHAR safer[] = { 
         'S','o','f','t','w','a','r','e','\\',
         'P','o','l','i','c','i','e','s','\\',
         'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m',
         'C','e','r','t','i','f','i','c','a','t','e','s','\\',
         'T','r','u','s','t','e','d','P','u','b','l','i','s','h','e','r',
         '\\','S','a','f','e','r',0 };
        static const WCHAR authenticodeFlags[] = { 'A','u','t','h','e','n',
         't','i','c','o','d','e','F','l','a','g','s',0 };
        BOOL ret, exists = FALSE;
        DWORD size, readFlags = 0, returnedFlags;
        HKEY key;
        LONG rc;

        rc = RegOpenKeyW(HKEY_LOCAL_MACHINE, safer, &key);
        if (rc == ERROR_SUCCESS)
        {
            size = sizeof(readFlags);
            rc = RegQueryValueExW(key, authenticodeFlags, NULL, NULL,
             (LPBYTE)&readFlags, &size);
            if (rc == ERROR_SUCCESS)
                exists = TRUE;
        }
        returnedFlags = 0xdeadbeef;
        ret = pReadDWORD(authenticodeFlags, &returnedFlags);
        ok(ret == exists, "Unexpected return value\n");
        ok(readFlags == returnedFlags,
         "Expected flags %08x, got %08x\n", readFlags, returnedFlags);
    }
}

typedef HCRYPTPROV (WINAPI *I_CryptGetDefaultCryptProvFunc)(DWORD w);

static void test_getDefaultCryptProv(void)
{
    I_CryptGetDefaultCryptProvFunc pI_CryptGetDefaultCryptProv;
    HCRYPTPROV prov;

    pI_CryptGetDefaultCryptProv = (I_CryptGetDefaultCryptProvFunc)
     GetProcAddress(hCrypt, "I_CryptGetDefaultCryptProv");
    if (!pI_CryptGetDefaultCryptProv) return;

    prov = pI_CryptGetDefaultCryptProv(0xdeadbeef);
    ok(prov == 0 && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    prov = pI_CryptGetDefaultCryptProv(PROV_RSA_FULL);
    ok(prov == 0 && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    prov = pI_CryptGetDefaultCryptProv(1);
    ok(prov == 0 && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    prov = pI_CryptGetDefaultCryptProv(0);
    ok(prov != 0, "I_CryptGetDefaultCryptProv failed: %08x\n", GetLastError());
    CryptReleaseContext(prov, 0);
}

typedef int (WINAPI *I_CryptInstallOssGlobal)(DWORD,DWORD,DWORD);

static void test_CryptInstallOssGlobal(void)
{
    int ret,i;
    I_CryptInstallOssGlobal pI_CryptInstallOssGlobal;

    pI_CryptInstallOssGlobal= (I_CryptInstallOssGlobal)GetProcAddress(hCrypt,"I_CryptInstallOssGlobal");
    /* passing in some random values to I_CryptInstallOssGlobal, it always returns 9 the first time, then 10, 11 etc.*/
    for(i=0;i<30;i++)
    {
      ret =  pI_CryptInstallOssGlobal(rand(),rand(),rand());
      ok((9+i) == ret,"Expected %d, got %d\n",(9+i),ret);
    }
}

START_TEST(main)
{
    hCrypt = GetModuleHandleA("crypt32.dll");

    test_findAttribute();
    test_findExtension();
    test_findRDNAttr();
    test_verifyTimeValidity();
    test_cryptAllocate();
    test_cryptTls();
    test_readTrustedPublisherDWORD();
    test_getDefaultCryptProv();
    test_CryptInstallOssGlobal();
}
