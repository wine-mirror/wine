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

    for (i = 0; i < sizeof(oidToAlgID) / sizeof(oidToAlgID[0]); i++)
    {
        DWORD alg = CertOIDToAlgId(oidToAlgID[i].oid);

        ok(alg == oidToAlgID[i].algID,
         "Expected %ld, got %ld\n", oidToAlgID[i].algID, alg);
    }
}

static void testAlgIDToOID(void)
{
    int i;

    for (i = 0; i < sizeof(algIDToOID) / sizeof(algIDToOID[0]); i++)
    {
        LPCSTR oid = CertAlgIdToOID(algIDToOID[i].algID);

        if ((!oid || !algIDToOID[i].oid) && oid != algIDToOID[i].oid &&
         algIDToOID[i].algID)
            printf("Expected %s, got %s\n", algIDToOID[i].oid, oid);
        else if (oid && algIDToOID[i].oid && strcmp(oid, algIDToOID[i].oid))
            printf("Expected %s, got %s\n", algIDToOID[i].oid, oid);
    }
}

static void test_findAttribute(void)
{
    PCRYPT_ATTRIBUTE ret;
    CRYPT_ATTR_BLOB blobs[] = {
     { 3, "\x02\x01\x01" },
    };
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
    CERT_EXTENSION ext = { "1.2.3", TRUE, { 3, "\x02\x01\x01" } };

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
    CERT_RDN_ATTR attrs[] = {
     { "1.2.3", CERT_RDN_IA5_STRING, { 11, "\x16\x09Juan Lang" } },
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

START_TEST(main)
{
    testOIDToAlgID();
    testAlgIDToOID();
    test_findAttribute();
    test_findExtension();
    test_findRDNAttr();
    test_verifyTimeValidity();
}
