/*
 * crypt32 cert functions tests
 *
 * Copyright 2005-2006 Juan Lang
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

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

static BOOL (WINAPI * pCryptVerifyCertificateSignatureEx)
                        (HCRYPTPROV, DWORD, DWORD, void *, DWORD, void *, DWORD, void *);

#define CRYPT_GET_PROC(func)                                       \
    p ## func = (void *)GetProcAddress(hCrypt32, #func);           \
    if(!p ## func)                                                 \
        trace("GetProcAddress(hCrypt32, \"%s\") failed\n", #func); \

static void init_function_pointers(void)
{
    HMODULE hCrypt32;

    pCryptVerifyCertificateSignatureEx = NULL;

    hCrypt32 = GetModuleHandleA("crypt32.dll");
    assert(hCrypt32);

    CRYPT_GET_PROC(CryptVerifyCertificateSignatureEx);
}

static void testCryptHashCert(void)
{
    static const BYTE emptyHash[] = { 0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b,
     0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07,
     0x09 };
    static const BYTE knownHash[] = { 0xae, 0x9d, 0xbf, 0x6d, 0xf5, 0x46, 0xee,
     0x8b, 0xc5, 0x7a, 0x13, 0xba, 0xc2, 0xb1, 0x04, 0xf2, 0xbf, 0x52, 0xa8,
     0xa2 };
    static const BYTE toHash[] = "abcdefghijklmnopqrstuvwxyz0123456789.,;!?:";
    BOOL ret;
    BYTE hash[20];
    DWORD hashLen = sizeof(hash);

    /* NULL buffer and nonzero length crashes
    ret = CryptHashCertificate(0, 0, 0, NULL, size, hash, &hashLen);
       empty hash length also crashes
    ret = CryptHashCertificate(0, 0, 0, buf, size, hash, NULL);
     */
    /* Test empty hash */
    ret = CryptHashCertificate(0, 0, 0, toHash, sizeof(toHash), NULL,
     &hashLen);
    ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
    ok(hashLen == sizeof(hash),
     "Got unexpected size of hash %ld, expected %d\n", hashLen, sizeof(hash));
    /* Test with empty buffer */
    ret = CryptHashCertificate(0, 0, 0, NULL, 0, hash, &hashLen);
    ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
    ok(!memcmp(hash, emptyHash, sizeof(emptyHash)),
     "Unexpected hash of nothing\n");
    /* Test a known value */
    ret = CryptHashCertificate(0, 0, 0, toHash, sizeof(toHash), hash,
     &hashLen);
    ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
    ok(!memcmp(hash, knownHash, sizeof(knownHash)), "Unexpected hash\n");
}

static const WCHAR cspNameW[] = { 'W','i','n','e','C','r','y','p','t','T','e',
 'm','p',0 };

static void verifySig(HCRYPTPROV csp, const BYTE *toSign, size_t toSignLen,
 const BYTE *sig, size_t sigLen)
{
    HCRYPTHASH hash;
    BOOL ret = CryptCreateHash(csp, CALG_SHA1, 0, 0, &hash);

    ok(ret, "CryptCreateHash failed: %08lx\n", GetLastError());
    if (ret)
    {
        BYTE mySig[64];
        DWORD mySigSize = sizeof(mySig);

        ret = CryptHashData(hash, toSign, toSignLen, 0);
        ok(ret, "CryptHashData failed: %08lx\n", GetLastError());
        /* use the A variant so the test can run on Win9x */
        ret = CryptSignHashA(hash, AT_SIGNATURE, NULL, 0, mySig, &mySigSize);
        ok(ret, "CryptSignHash failed: %08lx\n", GetLastError());
        if (ret)
        {
            ok(mySigSize == sigLen, "Expected sig length %d, got %ld\n",
             sigLen, mySigSize);
            ok(!memcmp(mySig, sig, sigLen), "Unexpected signature\n");
        }
        CryptDestroyHash(hash);
    }
}

/* Tests signing the certificate described by toBeSigned with the CSP passed in,
 * using the algorithm with OID sigOID.  The CSP is assumed to be empty, and a
 * keyset named AT_SIGNATURE will be added to it.  The signing key will be
 * stored in *key, and the signature will be stored in sig.  sigLen should be
 * at least 64 bytes.
 */
static void testSignCert(HCRYPTPROV csp, const CRYPT_DATA_BLOB *toBeSigned,
 LPCSTR sigOID, HCRYPTKEY *key, BYTE *sig, DWORD *sigLen)
{
    BOOL ret;
    DWORD size = 0;
    CRYPT_ALGORITHM_IDENTIFIER algoID = { NULL, { 0, NULL } };

    /* These all crash
    ret = CryptSignCertificate(0, 0, 0, NULL, 0, NULL, NULL, NULL, NULL);
    ret = CryptSignCertificate(0, 0, 0, NULL, 0, NULL, NULL, NULL, &size);
    ret = CryptSignCertificate(0, 0, 0, toBeSigned->pbData, toBeSigned->cbData,
     NULL, NULL, NULL, &size);
     */
    ret = CryptSignCertificate(0, 0, 0, toBeSigned->pbData, toBeSigned->cbData,
     &algoID, NULL, NULL, &size);
    ok(!ret && GetLastError() == NTE_BAD_ALGID, 
     "Expected NTE_BAD_ALGID, got %08lx\n", GetLastError());
    algoID.pszObjId = (LPSTR)sigOID;
    ret = CryptSignCertificate(0, 0, 0, toBeSigned->pbData, toBeSigned->cbData,
     &algoID, NULL, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    ret = CryptSignCertificate(0, AT_SIGNATURE, 0, toBeSigned->pbData,
     toBeSigned->cbData, &algoID, NULL, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());

    /* No keys exist in the new CSP yet.. */
    ret = CryptSignCertificate(csp, AT_SIGNATURE, 0, toBeSigned->pbData,
     toBeSigned->cbData, &algoID, NULL, NULL, &size);
    ok(!ret && (GetLastError() == NTE_BAD_KEYSET || GetLastError() ==
     NTE_NO_KEY), "Expected NTE_BAD_KEYSET or NTE_NO_KEY, got %08lx\n",
     GetLastError());
    ret = CryptGenKey(csp, AT_SIGNATURE, 0, key);
    ok(ret, "CryptGenKey failed: %08lx\n", GetLastError());
    if (ret)
    {
        ret = CryptSignCertificate(csp, AT_SIGNATURE, 0, toBeSigned->pbData,
         toBeSigned->cbData, &algoID, NULL, NULL, &size);
        ok(ret, "CryptSignCertificate failed: %08lx\n", GetLastError());
        ok(size <= *sigLen, "Expected size <= %ld, got %ld\n", *sigLen, size);
        if (ret)
        {
            ret = CryptSignCertificate(csp, AT_SIGNATURE, 0, toBeSigned->pbData,
             toBeSigned->cbData, &algoID, NULL, sig, &size);
            ok(ret, "CryptSignCertificate failed: %08lx\n", GetLastError());
            if (ret)
            {
                *sigLen = size;
                verifySig(csp, toBeSigned->pbData, toBeSigned->cbData, sig,
                 size);
            }
        }
    }
}

static void testVerifyCertSig(HCRYPTPROV csp, const CRYPT_DATA_BLOB *toBeSigned,
 LPCSTR sigOID, const BYTE *sig, DWORD sigLen)
{
    CERT_SIGNED_CONTENT_INFO info;
    LPBYTE cert = NULL;
    DWORD size = 0;
    BOOL ret;

    if(pCryptVerifyCertificateSignatureEx) {
        ret = pCryptVerifyCertificateSignatureEx(0, 0, 0, NULL, 0, NULL, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        ret = pCryptVerifyCertificateSignatureEx(csp, 0, 0, NULL, 0, NULL, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        ret = pCryptVerifyCertificateSignatureEx(csp, X509_ASN_ENCODING, 0, NULL, 0,
         NULL, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        /* This crashes
        ret = pCryptVerifyCertificateSignatureEx(csp, X509_ASN_ENCODING,
         CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, NULL, 0, NULL, 0, NULL);
         */
    }
    info.ToBeSigned.cbData = toBeSigned->cbData;
    info.ToBeSigned.pbData = toBeSigned->pbData;
    info.SignatureAlgorithm.pszObjId = (LPSTR)sigOID;
    info.SignatureAlgorithm.Parameters.cbData = 0;
    info.Signature.cbData = sigLen;
    info.Signature.pbData = (BYTE *)sig;
    info.Signature.cUnusedBits = 0;
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, X509_CERT, &info,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, (BYTE *)&cert, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08lx\n", GetLastError());
    if (cert)
    {
        CRYPT_DATA_BLOB certBlob = { 0, NULL };
        PCERT_PUBLIC_KEY_INFO pubKeyInfo = NULL;

        if(pCryptVerifyCertificateSignatureEx) {
            ret = pCryptVerifyCertificateSignatureEx(csp, X509_ASN_ENCODING,
             CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, &certBlob, 0, NULL, 0, NULL);
            ok(!ret && GetLastError() == CRYPT_E_ASN1_EOD,
             "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());
            certBlob.cbData = 1;
            certBlob.pbData = (void *)0xdeadbeef;
            ret = pCryptVerifyCertificateSignatureEx(csp, X509_ASN_ENCODING,
             CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, &certBlob, 0, NULL, 0, NULL);
            ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
             "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
            certBlob.cbData = size;
            certBlob.pbData = cert;
            ret = pCryptVerifyCertificateSignatureEx(csp, X509_ASN_ENCODING,
             CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, &certBlob, 0, NULL, 0, NULL);
            ok(!ret && GetLastError() == E_INVALIDARG,
             "Expected E_INVALIDARG, got %08lx\n", GetLastError());
            ret = pCryptVerifyCertificateSignatureEx(csp, X509_ASN_ENCODING,
             CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, &certBlob,
             CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL, NULL, 0, NULL);
            ok(!ret && GetLastError() == E_INVALIDARG,
             "Expected E_INVALIDARG, got %08lx\n", GetLastError());
            /* This crashes
            ret = pCryptVerifyCertificateSignatureEx(csp, X509_ASN_ENCODING,
             CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, &certBlob,
             CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY, NULL, 0, NULL);
             */
        }
        CryptExportPublicKeyInfoEx(csp, AT_SIGNATURE, X509_ASN_ENCODING,
         (LPSTR)sigOID, 0, NULL, NULL, &size);
        pubKeyInfo = HeapAlloc(GetProcessHeap(), 0, size);
        if (pubKeyInfo)
        {
            ret = CryptExportPublicKeyInfoEx(csp, AT_SIGNATURE,
             X509_ASN_ENCODING, (LPSTR)sigOID, 0, NULL, pubKeyInfo, &size);
            ok(ret, "CryptExportKey failed: %08lx\n", GetLastError());
            if (ret && pCryptVerifyCertificateSignatureEx)
            {
                ret = pCryptVerifyCertificateSignatureEx(csp, X509_ASN_ENCODING,
                 CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB, &certBlob,
                 CRYPT_VERIFY_CERT_SIGN_ISSUER_PUBKEY, pubKeyInfo, 0, NULL);
                ok(ret, "CryptVerifyCertificateSignatureEx failed: %08lx\n",
                 GetLastError());
            }
            HeapFree(GetProcessHeap(), 0, pubKeyInfo);
        }
        LocalFree(cert);
    }
}

static const BYTE emptyCert[] = { 0x30, 0x00 };

static void testCertSigs(void)
{
    HCRYPTPROV csp;
    CRYPT_DATA_BLOB toBeSigned = { sizeof(emptyCert), (LPBYTE)emptyCert };
    BOOL ret;
    HCRYPTKEY key;
    BYTE sig[64];
    DWORD sigSize = sizeof(sig);

    /* Just in case a previous run failed, delete this thing */
    CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    ret = CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(ret, "CryptAcquireContext failed: %08lx\n", GetLastError());

    testSignCert(csp, &toBeSigned, szOID_RSA_SHA1RSA, &key, sig, &sigSize);
    testVerifyCertSig(csp, &toBeSigned, szOID_RSA_SHA1RSA, sig, sigSize);

    CryptDestroyKey(key);
    CryptReleaseContext(csp, 0);
    ret = CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
}

static const BYTE subjectName[] = { 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06,
 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61,
 0x6e, 0x67, 0x00 };

static void testCreateSelfSignCert(void)
{
    PCCERT_CONTEXT context;
    CERT_NAME_BLOB name = { sizeof(subjectName), (LPBYTE)subjectName };
    HCRYPTPROV csp;
    BOOL ret;
    HCRYPTKEY key;

    /* This crashes:
    context = CertCreateSelfSignCertificate(0, NULL, 0, NULL, NULL, NULL, NULL,
     NULL);
     * Calling this with no first parameter creates a new key container, which
     * lasts beyond the test, so I don't test that.  Nb: the generated key
     * name is a GUID.
    context = CertCreateSelfSignCertificate(0, &name, 0, NULL, NULL, NULL, NULL,
     NULL);
     */

    /* Acquire a CSP */
    CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
    ret = CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(ret, "CryptAcquireContext failed: %08lx\n", GetLastError());

    context = CertCreateSelfSignCertificate(csp, &name, 0, NULL, NULL, NULL,
     NULL, NULL);
    ok(!context && GetLastError() == NTE_NO_KEY,
     "Expected NTE_NO_KEY, got %08lx\n", GetLastError());
    ret = CryptGenKey(csp, AT_SIGNATURE, 0, &key);
    ok(ret, "CryptGenKey failed: %08lx\n", GetLastError());
    if (ret)
    {
        context = CertCreateSelfSignCertificate(csp, &name, 0, NULL, NULL, NULL,
         NULL, NULL);
        ok(context != NULL, "CertCreateSelfSignCertificate failed: %08lx\n",
         GetLastError());
        if (context)
        {
            DWORD size = 0;
            PCRYPT_KEY_PROV_INFO info;

            /* The context must have a key provider info property */
            ret = CertGetCertificateContextProperty(context,
             CERT_KEY_PROV_INFO_PROP_ID, NULL, &size);
            ok(ret && size, "Expected non-zero key provider info\n");
            if (size)
            {
                info = HeapAlloc(GetProcessHeap(), 0, size);
                if (info)
                {
                    ret = CertGetCertificateContextProperty(context,
                     CERT_KEY_PROV_INFO_PROP_ID, info, &size);
                    ok(ret, "CertGetCertificateContextProperty failed: %08lx\n",
                     GetLastError());
                    if (ret)
                    {
                        /* Sanity-check the key provider */
                        ok(!lstrcmpW(info->pwszContainerName, cspNameW),
                         "Unexpected key container\n");
                        ok(!lstrcmpW(info->pwszProvName, MS_DEF_PROV_W),
                         "Unexpected provider\n");
                        ok(info->dwKeySpec == AT_SIGNATURE,
                         "Expected AT_SIGNATURE, got %ld\n", info->dwKeySpec);
                    }
                    HeapFree(GetProcessHeap(), 0, info);
                }
            }

            CertFreeCertificateContext(context);
        }

        CryptDestroyKey(key);
    }

    CryptReleaseContext(csp, 0);
    ret = CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_DELETEKEYSET);
}

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

static const LPCSTR keyUsages[] = { szOID_PKIX_KP_CODE_SIGNING,
 szOID_PKIX_KP_CLIENT_AUTH, szOID_RSA_RSA };

static const BYTE certWithUsage[] = { 0x30, 0x81, 0x93, 0x02, 0x01, 0x01, 0x30,
 0x02, 0x06, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04,
 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00,
 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31,
 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61,
 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00,
 0x03, 0x01, 0x00, 0xa3, 0x2f, 0x30, 0x2d, 0x30, 0x2b, 0x06, 0x03, 0x55, 0x1d,
 0x25, 0x01, 0x01, 0xff, 0x04, 0x21, 0x30, 0x1f, 0x06, 0x08, 0x2b, 0x06, 0x01,
 0x05, 0x05, 0x07, 0x03, 0x03, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07,
 0x03, 0x02, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 };

static void testKeyUsage(void)
{
    BOOL ret;
    PCCERT_CONTEXT context;
    DWORD size;

    /* Test base cases */
    ret = CertGetEnhancedKeyUsage(NULL, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    size = 1;
    ret = CertGetEnhancedKeyUsage(NULL, 0, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    size = 0;
    ret = CertGetEnhancedKeyUsage(NULL, 0, NULL, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
    /* These crash
    ret = CertSetEnhancedKeyUsage(NULL, NULL);
    usage.cUsageIdentifier = 0;
    ret = CertSetEnhancedKeyUsage(NULL, &usage);
     */
    /* Test with a cert with no enhanced key usage extension */
    context = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        static const char oid[] = "1.2.3.4";
        BYTE buf[sizeof(CERT_ENHKEY_USAGE) + 2 * (sizeof(LPSTR) + sizeof(oid))];
        PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE)buf;

        ret = CertGetEnhancedKeyUsage(context, 0, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08lx\n", GetLastError());
        size = 1;
        ret = CertGetEnhancedKeyUsage(context, 0, NULL, &size);
        if (ret)
        {
            /* Windows 2000, ME, or later: even though it succeeded, we expect
             * CRYPT_E_NOT_FOUND, which indicates there is no enhanced key
             * usage set for this cert (which implies it's valid for all uses.)
             */
            ok(GetLastError() == CRYPT_E_NOT_FOUND,
             "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
            ok(size == sizeof(CERT_ENHKEY_USAGE), "Expected size %d, got %ld\n",
             sizeof(CERT_ENHKEY_USAGE), size);
            ret = CertGetEnhancedKeyUsage(context, 0, pUsage, &size);
            ok(ret, "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
            ok(pUsage->cUsageIdentifier == 0, "Expected 0 usages, got %ld\n",
             pUsage->cUsageIdentifier);
        }
        else
        {
            /* Windows NT, 95, or 98: it fails, and the last error is
             * CRYPT_E_NOT_FOUND.
             */
            ok(GetLastError() == CRYPT_E_NOT_FOUND,
             "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
        }
        /* I can add a usage identifier when no key usage has been set */
        ret = CertAddEnhancedKeyUsageIdentifier(context, oid);
        ok(ret, "CertAddEnhancedKeyUsageIdentifier failed: %08lx\n",
         GetLastError());
        size = sizeof(buf);
        ret = CertGetEnhancedKeyUsage(context,
         CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, pUsage, &size);
        ok(ret && GetLastError() == 0,
         "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        ok(pUsage->cUsageIdentifier == 1, "Expected 1 usage, got %ld\n",
         pUsage->cUsageIdentifier);
        if (pUsage->cUsageIdentifier)
            ok(!strcmp(pUsage->rgpszUsageIdentifier[0], oid),
             "Expected %s, got %s\n", oid, pUsage->rgpszUsageIdentifier[0]);
        /* Now set an empty key usage */
        pUsage->cUsageIdentifier = 0;
        ret = CertSetEnhancedKeyUsage(context, pUsage);
        ok(ret, "CertSetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        /* Shouldn't find it in the cert */
        size = sizeof(buf);
        ret = CertGetEnhancedKeyUsage(context,
         CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, pUsage, &size);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
        /* Should find it as an extended property */
        ret = CertGetEnhancedKeyUsage(context,
         CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, pUsage, &size);
        ok(ret && GetLastError() == 0,
         "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        ok(pUsage->cUsageIdentifier == 0, "Expected 0 usages, got %ld\n",
         pUsage->cUsageIdentifier);
        /* Should find it as either */
        ret = CertGetEnhancedKeyUsage(context, 0, pUsage, &size);
        ok(ret && GetLastError() == 0,
         "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        ok(pUsage->cUsageIdentifier == 0, "Expected 0 usages, got %ld\n",
         pUsage->cUsageIdentifier);
        /* Add a usage identifier */
        ret = CertAddEnhancedKeyUsageIdentifier(context, oid);
        ok(ret, "CertAddEnhancedKeyUsageIdentifier failed: %08lx\n",
         GetLastError());
        size = sizeof(buf);
        ret = CertGetEnhancedKeyUsage(context, 0, pUsage, &size);
        ok(ret && GetLastError() == 0,
         "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        ok(pUsage->cUsageIdentifier == 1, "Expected 1 identifier, got %ld\n",
         pUsage->cUsageIdentifier);
        if (pUsage->cUsageIdentifier)
            ok(!strcmp(pUsage->rgpszUsageIdentifier[0], oid),
             "Expected %s, got %s\n", oid, pUsage->rgpszUsageIdentifier[0]);
        /* Yep, I can re-add the same usage identifier */
        ret = CertAddEnhancedKeyUsageIdentifier(context, oid);
        ok(ret, "CertAddEnhancedKeyUsageIdentifier failed: %08lx\n",
         GetLastError());
        size = sizeof(buf);
        ret = CertGetEnhancedKeyUsage(context, 0, pUsage, &size);
        ok(ret && GetLastError() == 0,
         "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        ok(pUsage->cUsageIdentifier == 2, "Expected 2 identifiers, got %ld\n",
         pUsage->cUsageIdentifier);
        if (pUsage->cUsageIdentifier)
            ok(!strcmp(pUsage->rgpszUsageIdentifier[0], oid),
             "Expected %s, got %s\n", oid, pUsage->rgpszUsageIdentifier[0]);
        if (pUsage->cUsageIdentifier >= 2)
            ok(!strcmp(pUsage->rgpszUsageIdentifier[1], oid),
             "Expected %s, got %s\n", oid, pUsage->rgpszUsageIdentifier[1]);
        /* Now set a NULL extended property--this deletes the property. */
        ret = CertSetEnhancedKeyUsage(context, NULL);
        ok(ret, "CertSetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        SetLastError(0xbaadcafe);
        size = sizeof(buf);
        ret = CertGetEnhancedKeyUsage(context, 0, pUsage, &size);
        ok(GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());

        CertFreeCertificateContext(context);
    }
    /* Now test with a cert with an enhanced key usage extension */
    context = CertCreateCertificateContext(X509_ASN_ENCODING, certWithUsage,
     sizeof(certWithUsage));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        LPBYTE buf = NULL;
        DWORD bufSize = 0, i;

        /* The size may depend on what flags are used to query it, so I
         * realloc the buffer for each test.
         */
        ret = CertGetEnhancedKeyUsage(context,
         CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, NULL, &bufSize);
        ok(ret, "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, bufSize);
        if (buf)
        {
            PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE)buf;

            /* Should find it in the cert */
            size = bufSize;
            ret = CertGetEnhancedKeyUsage(context,
             CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, pUsage, &size);
            ok(ret && GetLastError() == 0,
             "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
            ok(pUsage->cUsageIdentifier == 3, "Expected 3 usages, got %ld\n",
             pUsage->cUsageIdentifier);
            for (i = 0; i < pUsage->cUsageIdentifier; i++)
                ok(!strcmp(pUsage->rgpszUsageIdentifier[i], keyUsages[i]),
                 "Expected %s, got %s\n", keyUsages[i],
                 pUsage->rgpszUsageIdentifier[i]);
            HeapFree(GetProcessHeap(), 0, buf);
        }
        ret = CertGetEnhancedKeyUsage(context, 0, NULL, &bufSize);
        ok(ret, "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, bufSize);
        if (buf)
        {
            PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE)buf;

            /* Should find it as either */
            size = bufSize;
            ret = CertGetEnhancedKeyUsage(context, 0, pUsage, &size);
            /* In Windows, GetLastError returns CRYPT_E_NOT_FOUND not found
             * here, even though the return is successful and the usage id
             * count is positive.  I don't enforce that here.
             */
            ok(ret,
             "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
            ok(pUsage->cUsageIdentifier == 3, "Expected 3 usages, got %ld\n",
             pUsage->cUsageIdentifier);
            for (i = 0; i < pUsage->cUsageIdentifier; i++)
                ok(!strcmp(pUsage->rgpszUsageIdentifier[i], keyUsages[i]),
                 "Expected %s, got %s\n", keyUsages[i],
                 pUsage->rgpszUsageIdentifier[i]);
            HeapFree(GetProcessHeap(), 0, buf);
        }
        /* Shouldn't find it as an extended property */
        ret = CertGetEnhancedKeyUsage(context,
         CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, NULL, &size);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
        /* Adding a usage identifier overrides the cert's usage!? */
        ret = CertAddEnhancedKeyUsageIdentifier(context, szOID_RSA_RSA);
        ok(ret, "CertAddEnhancedKeyUsageIdentifier failed: %08lx\n",
         GetLastError());
        ret = CertGetEnhancedKeyUsage(context, 0, NULL, &bufSize);
        ok(ret, "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, bufSize);
        if (buf)
        {
            PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE)buf;

            /* Should find it as either */
            size = bufSize;
            ret = CertGetEnhancedKeyUsage(context, 0, pUsage, &size);
            ok(ret,
             "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
            ok(pUsage->cUsageIdentifier == 1, "Expected 1 usage, got %ld\n",
             pUsage->cUsageIdentifier);
            ok(!strcmp(pUsage->rgpszUsageIdentifier[0], szOID_RSA_RSA),
             "Expected %s, got %s\n", szOID_RSA_RSA,
             pUsage->rgpszUsageIdentifier[0]);
            HeapFree(GetProcessHeap(), 0, buf);
        }
        /* But querying the cert directly returns its usage */
        ret = CertGetEnhancedKeyUsage(context,
         CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, NULL, &bufSize);
        ok(ret, "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, bufSize);
        if (buf)
        {
            PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE)buf;

            size = bufSize;
            ret = CertGetEnhancedKeyUsage(context,
             CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, pUsage, &size);
            ok(ret,
             "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
            ok(pUsage->cUsageIdentifier == 3, "Expected 3 usages, got %ld\n",
             pUsage->cUsageIdentifier);
            for (i = 0; i < pUsage->cUsageIdentifier; i++)
                ok(!strcmp(pUsage->rgpszUsageIdentifier[i], keyUsages[i]),
                 "Expected %s, got %s\n", keyUsages[i],
                 pUsage->rgpszUsageIdentifier[i]);
            HeapFree(GetProcessHeap(), 0, buf);
        }
        /* And removing the only usage identifier in the extended property
         * results in the cert's key usage being found.
         */
        ret = CertRemoveEnhancedKeyUsageIdentifier(context, szOID_RSA_RSA);
        ok(ret, "CertRemoveEnhancedKeyUsage failed: %08lx\n", GetLastError());
        ret = CertGetEnhancedKeyUsage(context, 0, NULL, &bufSize);
        ok(ret, "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, bufSize);
        if (buf)
        {
            PCERT_ENHKEY_USAGE pUsage = (PCERT_ENHKEY_USAGE)buf;

            /* Should find it as either */
            size = bufSize;
            ret = CertGetEnhancedKeyUsage(context, 0, pUsage, &size);
            ok(ret,
             "CertGetEnhancedKeyUsage failed: %08lx\n", GetLastError());
            ok(pUsage->cUsageIdentifier == 3, "Expected 3 usages, got %ld\n",
             pUsage->cUsageIdentifier);
            for (i = 0; i < pUsage->cUsageIdentifier; i++)
                ok(!strcmp(pUsage->rgpszUsageIdentifier[i], keyUsages[i]),
                 "Expected %s, got %s\n", keyUsages[i],
                 pUsage->rgpszUsageIdentifier[i]);
            HeapFree(GetProcessHeap(), 0, buf);
        }

        CertFreeCertificateContext(context);
    }
}

static void testCompareCertName(void)
{
    static const BYTE bogus[] = { 1, 2, 3, 4 };
    static const BYTE bogusPrime[] = { 0, 1, 2, 3, 4 };
    static const BYTE emptyPrime[] = { 0x30, 0x00, 0x01 };
    BOOL ret;
    CERT_NAME_BLOB blob1, blob2;

    /* crashes
    ret = CertCompareCertificateName(0, NULL, NULL);
     */
    /* An empty name checks against itself.. */
    blob1.pbData = (LPBYTE)emptyCert;
    blob1.cbData = sizeof(emptyCert);
    ret = CertCompareCertificateName(0, &blob1, &blob1);
    ok(ret, "CertCompareCertificateName failed: %08lx\n", GetLastError());
    /* It doesn't have to be a valid encoded name.. */
    blob1.pbData = (LPBYTE)bogus;
    blob1.cbData = sizeof(bogus);
    ret = CertCompareCertificateName(0, &blob1, &blob1);
    ok(ret, "CertCompareCertificateName failed: %08lx\n", GetLastError());
    /* Leading zeroes matter.. */
    blob2.pbData = (LPBYTE)bogusPrime;
    blob2.cbData = sizeof(bogusPrime);
    ret = CertCompareCertificateName(0, &blob1, &blob2);
    ok(!ret, "Expected failure\n");
    /* As do trailing extra bytes. */
    blob2.pbData = (LPBYTE)emptyPrime;
    blob2.cbData = sizeof(emptyPrime);
    ret = CertCompareCertificateName(0, &blob1, &blob2);
    ok(!ret, "Expected failure\n");
}

static const BYTE int1[] = { 0x88, 0xff, 0xff, 0xff };
static const BYTE int2[] = { 0x88, 0xff };
static const BYTE int3[] = { 0x23, 0xff };
static const BYTE int4[] = { 0x7f, 0x00 };
static const BYTE int5[] = { 0x7f };
static const BYTE int6[] = { 0x80, 0x00, 0x00, 0x00 };
static const BYTE int7[] = { 0x80, 0x00 };

struct IntBlobTest
{
    CRYPT_INTEGER_BLOB blob1;
    CRYPT_INTEGER_BLOB blob2;
    BOOL areEqual;
} intBlobs[] = {
 { { sizeof(int1), (LPBYTE)int1 }, { sizeof(int2), (LPBYTE)int2 }, TRUE },
 { { sizeof(int3), (LPBYTE)int3 }, { sizeof(int3), (LPBYTE)int3 }, TRUE },
 { { sizeof(int4), (LPBYTE)int4 }, { sizeof(int5), (LPBYTE)int5 }, TRUE },
 { { sizeof(int6), (LPBYTE)int6 }, { sizeof(int7), (LPBYTE)int7 }, TRUE },
 { { sizeof(int1), (LPBYTE)int1 }, { sizeof(int7), (LPBYTE)int7 }, FALSE },
};

static void testCompareIntegerBlob(void)
{
    DWORD i;
    BOOL ret;

    for (i = 0; i < sizeof(intBlobs) / sizeof(intBlobs[0]); i++)
    {
        ret = CertCompareIntegerBlob(&intBlobs[i].blob1, &intBlobs[i].blob2);
        ok(ret == intBlobs[i].areEqual,
         "%ld: expected blobs %s compare\n", i, intBlobs[i].areEqual ?
         "to" : "not to");
    }
}

static void testComparePublicKeyInfo(void)
{
    BOOL ret;
    CERT_PUBLIC_KEY_INFO info1 = { { 0 } }, info2 = { { 0 } };
    static CHAR oid_rsa_rsa[]     = szOID_RSA_RSA;
    static CHAR oid_rsa_sha1rsa[] = szOID_RSA_SHA1RSA;
    static CHAR oid_x957_dsa[]    = szOID_X957_DSA;
    static const BYTE bits1[] = { 1, 0 };
    static const BYTE bits2[] = { 0 };
    static const BYTE bits3[] = { 1 };

    /* crashes
    ret = CertComparePublicKeyInfo(0, NULL, NULL);
     */
    /* Empty public keys compare */
    ret = CertComparePublicKeyInfo(0, &info1, &info2);
    ok(ret, "CertComparePublicKeyInfo failed: %08lx\n", GetLastError());
    /* Different OIDs appear to compare */
    info1.Algorithm.pszObjId = oid_rsa_rsa;
    info2.Algorithm.pszObjId = oid_rsa_sha1rsa;
    ret = CertComparePublicKeyInfo(0, &info1, &info2);
    ok(ret, "CertComparePublicKeyInfo failed: %08lx\n", GetLastError());
    info2.Algorithm.pszObjId = oid_x957_dsa;
    ret = CertComparePublicKeyInfo(0, &info1, &info2);
    ok(ret, "CertComparePublicKeyInfo failed: %08lx\n", GetLastError());
    info1.PublicKey.cbData = sizeof(bits1);
    info1.PublicKey.pbData = (LPBYTE)bits1;
    info1.PublicKey.cUnusedBits = 0;
    info2.PublicKey.cbData = sizeof(bits1);
    info2.PublicKey.pbData = (LPBYTE)bits1;
    info2.PublicKey.cUnusedBits = 0;
    ret = CertComparePublicKeyInfo(0, &info1, &info2);
    ok(ret, "CertComparePublicKeyInfo failed: %08lx\n", GetLastError());
    /* Even though they compare in their used bits, these do not compare */
    info1.PublicKey.cbData = sizeof(bits2);
    info1.PublicKey.pbData = (LPBYTE)bits2;
    info1.PublicKey.cUnusedBits = 0;
    info2.PublicKey.cbData = sizeof(bits3);
    info2.PublicKey.pbData = (LPBYTE)bits3;
    info2.PublicKey.cUnusedBits = 1;
    ret = CertComparePublicKeyInfo(0, &info1, &info2);
    /* Simple (non-comparing) case */
    ok(!ret, "Expected keys not to compare\n");
    info2.PublicKey.cbData = sizeof(bits1);
    info2.PublicKey.pbData = (LPBYTE)bits1;
    info2.PublicKey.cUnusedBits = 0;
    ret = CertComparePublicKeyInfo(0, &info1, &info2);
    ok(!ret, "Expected keys not to compare\n");
}

static const BYTE subjectName2[] = { 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06,
 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x41, 0x6c, 0x65, 0x78, 0x20, 0x4c, 0x61,
 0x6e, 0x67, 0x00 };

static const BYTE serialNum[] = { 1 };

void testCompareCert(void)
{
    CERT_INFO info1 = { 0 }, info2 = { 0 };
    BOOL ret;

    /* Crashes
    ret = CertCompareCertificate(X509_ASN_ENCODING, NULL, NULL);
     */

    /* Certs with the same issuer and serial number are equal, even if they
     * differ in other respects (like subject).
     */
    info1.SerialNumber.pbData = (LPBYTE)serialNum;
    info1.SerialNumber.cbData = sizeof(serialNum);
    info1.Issuer.pbData = (LPBYTE)subjectName;
    info1.Issuer.cbData = sizeof(subjectName);
    info1.Subject.pbData = (LPBYTE)subjectName2;
    info1.Subject.cbData = sizeof(subjectName2);
    info2.SerialNumber.pbData = (LPBYTE)serialNum;
    info2.SerialNumber.cbData = sizeof(serialNum);
    info2.Issuer.pbData = (LPBYTE)subjectName;
    info2.Issuer.cbData = sizeof(subjectName);
    info2.Subject.pbData = (LPBYTE)subjectName;
    info2.Subject.cbData = sizeof(subjectName);
    ret = CertCompareCertificate(X509_ASN_ENCODING, &info1, &info2);
    ok(ret, "Expected certs to be equal\n");

    info2.Issuer.pbData = (LPBYTE)subjectName2;
    info2.Issuer.cbData = sizeof(subjectName2);
    ret = CertCompareCertificate(X509_ASN_ENCODING, &info1, &info2);
    ok(!ret, "Expected certs not to be equal\n");
}

static const BYTE bigCertWithDifferentSubject[] = { 0x30, 0x7a, 0x02, 0x01, 0x02,
 0x30, 0x02, 0x06, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55,
 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67,
 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15,
 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x41, 0x6c,
 0x65, 0x78, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06,
 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55,
 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02,
 0x01, 0x01 };

static void testVerifySubjectCert(void)
{
    BOOL ret;
    DWORD flags;
    PCCERT_CONTEXT context1, context2;

    /* Crashes
    ret = CertVerifySubjectCertificateContext(NULL, NULL, NULL);
     */
    flags = 0;
    ret = CertVerifySubjectCertificateContext(NULL, NULL, &flags);
    ok(ret, "CertVerifySubjectCertificateContext failed; %08lx\n",
     GetLastError());
    flags = CERT_STORE_NO_CRL_FLAG;
    ret = CertVerifySubjectCertificateContext(NULL, NULL, &flags);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());

    flags = 0;
    context1 = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ret = CertVerifySubjectCertificateContext(NULL, context1, &flags);
    ok(ret, "CertVerifySubjectCertificateContext failed; %08lx\n",
     GetLastError());
    ret = CertVerifySubjectCertificateContext(context1, NULL, &flags);
    ok(ret, "CertVerifySubjectCertificateContext failed; %08lx\n",
     GetLastError());
    ret = CertVerifySubjectCertificateContext(context1, context1, &flags);
    ok(ret, "CertVerifySubjectCertificateContext failed; %08lx\n",
     GetLastError());

    context2 = CertCreateCertificateContext(X509_ASN_ENCODING,
     bigCertWithDifferentSubject, sizeof(bigCertWithDifferentSubject));
    SetLastError(0xdeadbeef);
    ret = CertVerifySubjectCertificateContext(context1, context2, &flags);
    ok(ret, "CertVerifySubjectCertificateContext failed; %08lx\n",
     GetLastError());
    flags = CERT_STORE_REVOCATION_FLAG;
    ret = CertVerifySubjectCertificateContext(context1, context2, &flags);
    ok(ret, "CertVerifySubjectCertificateContext failed; %08lx\n",
     GetLastError());
    ok(flags == (CERT_STORE_REVOCATION_FLAG | CERT_STORE_NO_CRL_FLAG),
     "Expected CERT_STORE_REVOCATION_FLAG | CERT_STORE_NO_CRL_FLAG, got %08lx\n",
     flags);
    flags = CERT_STORE_SIGNATURE_FLAG;
    ret = CertVerifySubjectCertificateContext(context1, context2, &flags);
    ok(ret, "CertVerifySubjectCertificateContext failed; %08lx\n",
     GetLastError());
    ok(flags == CERT_STORE_SIGNATURE_FLAG,
     "Expected CERT_STORE_SIGNATURE_FLAG, got %08lx\n", flags);
    CertFreeCertificateContext(context2);

    CertFreeCertificateContext(context1);
}

START_TEST(cert)
{
    init_function_pointers();
    testCryptHashCert();
    testCertSigs();
    testCreateSelfSignCert();
    testKeyUsage();
    testCompareCertName();
    testCompareIntegerBlob();
    testComparePublicKeyInfo();
    testCompareCert();
    testVerifySubjectCert();
}
