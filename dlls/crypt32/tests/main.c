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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

struct OIDToAlgID
{
    LPCSTR oid;
    DWORD algID;
};

static const struct OIDToAlgID oidToAlgID[] = {
 { szOID_RSA_RSA, CALG_RSA_KEYX },
 { szOID_RSA_MD2RSA, CALG_MD2 },
 { szOID_RSA_MD4RSA, CALG_MD4 },
 { szOID_RSA_MD5RSA, CALG_MD5 },
 { szOID_RSA_SHA1RSA, CALG_SHA },
 { szOID_RSA_DH, CALG_DH_SF },
 { szOID_RSA_SMIMEalgESDH, CALG_DH_EPHEM },
 { szOID_RSA_SMIMEalgCMS3DESwrap, CALG_3DES },
 { szOID_RSA_SMIMEalgCMSRC2wrap, CALG_RC2 },
 { szOID_RSA_MD2, CALG_MD2 },
 { szOID_RSA_MD4, CALG_MD4 },
 { szOID_RSA_MD5, CALG_MD5 },
 { szOID_RSA_RC2CBC, CALG_RC2 },
 { szOID_RSA_RC4, CALG_RC4 },
 { szOID_RSA_DES_EDE3_CBC, CALG_3DES },
 { szOID_ANSI_X942_DH, CALG_DH_SF },
 { szOID_X957_DSA, CALG_DSS_SIGN },
 { szOID_X957_SHA1DSA, CALG_SHA },
 { szOID_OIWSEC_md4RSA, CALG_MD4 },
 { szOID_OIWSEC_md5RSA, CALG_MD5 },
 { szOID_OIWSEC_md4RSA2, CALG_MD4 },
 { szOID_OIWSEC_desCBC, CALG_DES },
 { szOID_OIWSEC_dsa, CALG_DSS_SIGN },
 { szOID_OIWSEC_shaDSA, CALG_SHA },
 { szOID_OIWSEC_shaRSA, CALG_SHA },
 { szOID_OIWSEC_sha, CALG_SHA },
 { szOID_OIWSEC_rsaXchg, CALG_RSA_KEYX },
 { szOID_OIWSEC_sha1, CALG_SHA },
 { szOID_OIWSEC_dsaSHA1, CALG_SHA },
 { szOID_OIWSEC_sha1RSASign, CALG_SHA },
 { szOID_OIWDIR_md2RSA, CALG_MD2 },
 { szOID_INFOSEC_mosaicUpdatedSig, CALG_SHA },
 { szOID_INFOSEC_mosaicKMandUpdSig, CALG_DSS_SIGN },
};

static const struct OIDToAlgID algIDToOID[] = {
 { szOID_RSA_RSA, CALG_RSA_KEYX },
 { szOID_RSA_SMIMEalgESDH, CALG_DH_EPHEM },
 { szOID_RSA_MD2, CALG_MD2 },
 { szOID_RSA_MD4, CALG_MD4 },
 { szOID_RSA_MD5, CALG_MD5 },
 { szOID_RSA_RC2CBC, CALG_RC2 },
 { szOID_RSA_RC4, CALG_RC4 },
 { szOID_RSA_DES_EDE3_CBC, CALG_3DES },
 { szOID_ANSI_X942_DH, CALG_DH_SF },
 { szOID_X957_DSA, CALG_DSS_SIGN },
 { szOID_OIWSEC_desCBC, CALG_DES },
 { szOID_OIWSEC_sha1, CALG_SHA },
};

static void testOIDToAlgID(void)
{
    int i;
    DWORD alg;

    /* Test with a bogus one */
    SetLastError(0xdeadbeef);
    alg = CertOIDToAlgId("1.2.3");
    ok(!alg && (GetLastError() == 0xdeadbeef ||
     GetLastError() == ERROR_RESOURCE_NAME_NOT_FOUND),
     "Expected ERROR_RESOURCE_NAME_NOT_FOUND or no error set, got %08lx\n",
     GetLastError());

    for (i = 0; i < sizeof(oidToAlgID) / sizeof(oidToAlgID[0]); i++)
    {
        alg = CertOIDToAlgId(oidToAlgID[i].oid);
        /* Not all Windows installations support all these, so make sure it's
         * at least not the wrong one.
         */
        ok(alg == 0 || alg == oidToAlgID[i].algID,
         "Expected %ld, got %ld\n", oidToAlgID[i].algID, alg);
    }
}

static void testAlgIDToOID(void)
{
    int i;
    LPCSTR oid;

    /* Test with a bogus one */
    SetLastError(0xdeadbeef);
    oid = CertAlgIdToOID(ALG_CLASS_SIGNATURE | ALG_TYPE_ANY | 80);
    ok(!oid && GetLastError() == 0xdeadbeef,
     "Didn't expect last error (%08lx) to be set\n", GetLastError());
    for (i = 0; i < sizeof(algIDToOID) / sizeof(algIDToOID[0]); i++)
    {
        oid = CertAlgIdToOID(algIDToOID[i].algID);
        /* Allow failure, not every version of Windows supports every algo */
        if (oid)
            ok(!strcmp(oid, algIDToOID[i].oid), 
             "Expected %s, got %s\n", algIDToOID[i].oid, oid);
    }
}

static void test_findAttribute(void)
{
    PCRYPT_ATTRIBUTE ret;
    BYTE blobbin[] = {0x02,0x01,0x01};
    CRYPT_ATTR_BLOB blobs[] = { { sizeof blobbin, blobbin }, };
    CRYPT_ATTRIBUTE attr = { "1.2.3", sizeof(blobs) / sizeof(blobs[0]), blobs };

    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute(NULL, 0, NULL);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08lx\n",
     GetLastError());
    /* crashes
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute(NULL, 1, NULL);
     */
    /* returns NULL, last error is ERROR_INVALID_PARAMETER */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute(NULL, 1, &attr);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %ld (%08lx)\n", GetLastError(),
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("bogus", 1, &attr);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08lx\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("1.2.4", 1, &attr);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08lx\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindAttribute("1.2.3", 1, &attr);
    ok(ret != NULL, "CertFindAttribute failed: %08lx\n", GetLastError());
}

static void test_findExtension(void)
{
    PCERT_EXTENSION ret;
    BYTE blobbin[] = {0x02,0x01,0x01};
    CERT_EXTENSION ext = { "1.2.3", TRUE, { sizeof blobbin, blobbin } };

    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension(NULL, 0, NULL);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08lx\n",
     GetLastError());
    /* crashes
    SetLastError(0xdeadbeef);
    ret = CertFindExtension(NULL, 1, NULL);
     */
    /* returns NULL, last error is ERROR_INVALID_PARAMETER */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension(NULL, 1, &ext);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %ld (%08lx)\n", GetLastError(),
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("bogus", 1, &ext);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08lx\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("1.2.4", 1, &ext);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08lx\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindExtension("1.2.3", 1, &ext);
    ok(ret != NULL, "CertFindExtension failed: %08lx\n", GetLastError());
}

static void test_findRDNAttr(void)
{
    PCERT_RDN_ATTR ret;
    BYTE bin[] = { 0x16,0x09,'J','u','a','n',' ','L','a','n','g' };
    CERT_RDN_ATTR attrs[] = {
     { "1.2.3", CERT_RDN_IA5_STRING, { sizeof bin, bin } },
    };
    CERT_RDN rdns[] = {
     { sizeof(attrs) / sizeof(attrs[0]), attrs },
    };
    CERT_NAME_INFO nameInfo = { sizeof(rdns) / sizeof(rdns[0]), rdns };

    /* crashes
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr(NULL, NULL);
     */
    /* returns NULL, last error is ERROR_INVALID_PARAMETER */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr(NULL, &nameInfo);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %ld (%08lx)\n", GetLastError(),
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("bogus", &nameInfo);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08lx\n",
     GetLastError());
    /* returns NULL, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("1.2.4", &nameInfo);
    ok(ret == NULL, "Expected failure\n");
    ok(GetLastError() == 0xdeadbeef, "Last error was set to %08lx\n",
     GetLastError());
    /* succeeds, last error not set */
    SetLastError(0xdeadbeef);
    ret = CertFindRDNAttr("1.2.3", &nameInfo);
    ok(ret != NULL, "CertFindRDNAttr failed: %08lx\n", GetLastError());
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
    ok(ret == 1, "Expected 1, got %ld\n", ret);
    memcpy(&info.NotAfter, &fileTime, sizeof(info.NotAfter));
    /* Check with NotAfter equal to comparison time */
    ret = CertVerifyTimeValidity(&fileTime, &info);
    ok(ret == 0, "Expected 0, got %ld\n", ret);
    /* Check with NotBefore after comparison time */
    memcpy(&info.NotBefore, &fileTime, sizeof(info.NotBefore));
    info.NotBefore.dwLowDateTime += 5000;
    ret = CertVerifyTimeValidity(&fileTime, &info);
    ok(ret == -1, "Expected -1, got %ld\n", ret);
}

static void test_cryptAllocate(void)
{
    LPVOID buf;

    buf = CryptMemAlloc(0);
    ok(buf != NULL, "CryptMemAlloc failed: %08lx\n", GetLastError());
    CryptMemFree(buf);
    buf = CryptMemRealloc(NULL, 0);
    ok(!buf, "Expected NULL\n");
    buf = CryptMemAlloc(0);
    buf = CryptMemRealloc(buf, 1);
    ok(buf != NULL, "CryptMemRealloc failed: %08lx\n", GetLastError());
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
    HMODULE lib = LoadLibraryA("crypt32.dll");

    if (lib)
    {
        DWORD index;
        BOOL ret;

        pI_CryptAllocTls = (I_CryptAllocTlsFunc)GetProcAddress(lib,
         "I_CryptAllocTls");
        pI_CryptDetachTls = (I_CryptDetachTlsFunc)GetProcAddress(lib,
         "I_CryptDetachTls");
        pI_CryptGetTls = (I_CryptGetTlsFunc)GetProcAddress(lib,
         "I_CryptGetTls");
        pI_CryptSetTls = (I_CryptSetTlsFunc)GetProcAddress(lib,
         "I_CryptSetTls");
        pI_CryptFreeTls = (I_CryptFreeTlsFunc)GetProcAddress(lib,
         "I_CryptFreeTls");

        /* One normal pass */
        index = pI_CryptAllocTls();
        ok(index, "I_CryptAllocTls failed: %08lx\n", GetLastError());
        if (index)
        {
            LPVOID ptr;

            ptr = pI_CryptGetTls(index);
            ok(!ptr, "Expected NULL\n");
            ret = pI_CryptSetTls(index, (LPVOID)0xdeadbeef);
            ok(ret, "I_CryptSetTls failed: %08lx\n", GetLastError());
            ptr = pI_CryptGetTls(index);
            ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
            /* This crashes
            ret = pI_CryptFreeTls(index, 1);
             */
            ret = pI_CryptFreeTls(index, 0);
            ok(ret, "I_CryptFreeTls failed: %08lx\n", GetLastError());
            ret = pI_CryptFreeTls(index, 0);
            /* Not sure if this fails because TlsFree should fail, so leave as
             * todo for now.
             */
            todo_wine ok(!ret && GetLastError() ==
             HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER),
             "Expected HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER), got %08lx\n",
             GetLastError());
        }
        /* Similar pass, check I_CryptDetachTls */
        index = pI_CryptAllocTls();
        ok(index, "I_CryptAllocTls failed: %08lx\n", GetLastError());
        if (index)
        {
            LPVOID ptr;

            ptr = pI_CryptGetTls(index);
            ok(!ptr, "Expected NULL\n");
            ret = pI_CryptSetTls(index, (LPVOID)0xdeadbeef);
            ok(ret, "I_CryptSetTls failed: %08lx\n", GetLastError());
            ptr = pI_CryptGetTls(index);
            ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
            ptr = pI_CryptDetachTls(index);
            ok(ptr == (LPVOID)0xdeadbeef, "Expected 0xdeadbeef, got %p\n", ptr);
            ptr = pI_CryptGetTls(index);
            ok(!ptr, "Expected NULL\n");
        }
        FreeLibrary(lib);
    }
}

START_TEST(main)
{
    testOIDToAlgID();
    testAlgIDToOID();
    test_findAttribute();
    test_findExtension();
    test_findRDNAttr();
    test_verifyTimeValidity();
    test_cryptAllocate();
    test_cryptTls();
}
