/*
 * Schannel tests
 *
 * Copyright 2006 Juan Lang
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

#include <stdarg.h>
#include <windef.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#define SECURITY_WIN32
#include <security.h>
#define SCHANNEL_USE_BLACKLISTS
#include <schannel.h>

#include "wine/test.h"

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
static WCHAR cspNameW[] = { 'W','i','n','e','C','r','y','p','t','T','e',
 'm','p',0 };
static BYTE privKey[] = {
 0x07, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x52, 0x53, 0x41, 0x32, 0x00,
 0x02, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x79, 0x10, 0x1c, 0xd0, 0x6b, 0x10,
 0x18, 0x30, 0x94, 0x61, 0xdc, 0x0e, 0xcb, 0x96, 0x4e, 0x21, 0x3f, 0x79, 0xcd,
 0xa9, 0x17, 0x62, 0xbc, 0xbb, 0x61, 0x4c, 0xe0, 0x75, 0x38, 0x6c, 0xf3, 0xde,
 0x60, 0x86, 0x03, 0x97, 0x65, 0xeb, 0x1e, 0x6b, 0xdb, 0x53, 0x85, 0xad, 0x68,
 0x21, 0xf1, 0x5d, 0xe7, 0x1f, 0xe6, 0x53, 0xb4, 0xbb, 0x59, 0x3e, 0x14, 0x27,
 0xb1, 0x83, 0xa7, 0x3a, 0x54, 0xe2, 0x8f, 0x65, 0x8e, 0x6a, 0x4a, 0xcf, 0x3b,
 0x1f, 0x65, 0xff, 0xfe, 0xf1, 0x31, 0x3a, 0x37, 0x7a, 0x8b, 0xcb, 0xc6, 0xd4,
 0x98, 0x50, 0x36, 0x67, 0xe4, 0xa1, 0xe8, 0x7e, 0x8a, 0xc5, 0x23, 0xf2, 0x77,
 0xf5, 0x37, 0x61, 0x49, 0x72, 0x59, 0xe8, 0x3d, 0xf7, 0x60, 0xb2, 0x77, 0xca,
 0x78, 0x54, 0x6d, 0x65, 0x9e, 0x03, 0x97, 0x1b, 0x61, 0xbd, 0x0c, 0xd8, 0x06,
 0x63, 0xe2, 0xc5, 0x48, 0xef, 0xb3, 0xe2, 0x6e, 0x98, 0x7d, 0xbd, 0x4e, 0x72,
 0x91, 0xdb, 0x31, 0x57, 0xe3, 0x65, 0x3a, 0x49, 0xca, 0xec, 0xd2, 0x02, 0x4e,
 0x22, 0x7e, 0x72, 0x8e, 0xf9, 0x79, 0x84, 0x82, 0xdf, 0x7b, 0x92, 0x2d, 0xaf,
 0xc9, 0xe4, 0x33, 0xef, 0x89, 0x5c, 0x66, 0x99, 0xd8, 0x80, 0x81, 0x47, 0x2b,
 0xb1, 0x66, 0x02, 0x84, 0x59, 0x7b, 0xc3, 0xbe, 0x98, 0x45, 0x4a, 0x3d, 0xdd,
 0xea, 0x2b, 0xdf, 0x4e, 0xb4, 0x24, 0x6b, 0xec, 0xe7, 0xd9, 0x0c, 0x45, 0xb8,
 0xbe, 0xca, 0x69, 0x37, 0x92, 0x4c, 0x38, 0x6b, 0x96, 0x6d, 0xcd, 0x86, 0x67,
 0x5c, 0xea, 0x54, 0x94, 0xa4, 0xca, 0xa4, 0x02, 0xa5, 0x21, 0x4d, 0xae, 0x40,
 0x8f, 0x9d, 0x51, 0x83, 0xf2, 0x3f, 0x33, 0xc1, 0x72, 0xb4, 0x1d, 0x94, 0x6e,
 0x7d, 0xe4, 0x27, 0x3f, 0xea, 0xff, 0xe5, 0x9b, 0xa7, 0x5e, 0x55, 0x8e, 0x0d,
 0x69, 0x1c, 0x7a, 0xff, 0x81, 0x9d, 0x53, 0x52, 0x97, 0x9a, 0x76, 0x79, 0xda,
 0x93, 0x32, 0x16, 0xec, 0x69, 0x51, 0x1a, 0x4e, 0xc3, 0xf1, 0x72, 0x80, 0x78,
 0x5e, 0x66, 0x4a, 0x8d, 0x85, 0x2f, 0x3f, 0xb2, 0xa7 };

static const BYTE selfSignedCert[] = {
 0x30, 0x82, 0x01, 0x1f, 0x30, 0x81, 0xce, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02,
 0x10, 0xeb, 0x0d, 0x57, 0x2a, 0x9c, 0x09, 0xba, 0xa4, 0x4a, 0xb7, 0x25, 0x49,
 0xd9, 0x3e, 0xb5, 0x73, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1d,
 0x05, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03,
 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30,
 0x1e, 0x17, 0x0d, 0x30, 0x36, 0x30, 0x36, 0x32, 0x39, 0x30, 0x35, 0x30, 0x30,
 0x34, 0x36, 0x5a, 0x17, 0x0d, 0x30, 0x37, 0x30, 0x36, 0x32, 0x39, 0x31, 0x31,
 0x30, 0x30, 0x34, 0x36, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03,
 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e,
 0x67, 0x00, 0x30, 0x5c, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x4b, 0x00, 0x30, 0x48, 0x02, 0x41,
 0x00, 0xe2, 0x54, 0x3a, 0xa7, 0x83, 0xb1, 0x27, 0x14, 0x3e, 0x59, 0xbb, 0xb4,
 0x53, 0xe6, 0x1f, 0xe7, 0x5d, 0xf1, 0x21, 0x68, 0xad, 0x85, 0x53, 0xdb, 0x6b,
 0x1e, 0xeb, 0x65, 0x97, 0x03, 0x86, 0x60, 0xde, 0xf3, 0x6c, 0x38, 0x75, 0xe0,
 0x4c, 0x61, 0xbb, 0xbc, 0x62, 0x17, 0xa9, 0xcd, 0x79, 0x3f, 0x21, 0x4e, 0x96,
 0xcb, 0x0e, 0xdc, 0x61, 0x94, 0x30, 0x18, 0x10, 0x6b, 0xd0, 0x1c, 0x10, 0x79,
 0x02, 0x03, 0x01, 0x00, 0x01, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02,
 0x1d, 0x05, 0x00, 0x03, 0x41, 0x00, 0x25, 0x90, 0x53, 0x34, 0xd9, 0x56, 0x41,
 0x5e, 0xdb, 0x7e, 0x01, 0x36, 0xec, 0x27, 0x61, 0x5e, 0xb7, 0x4d, 0x90, 0x66,
 0xa2, 0xe1, 0x9d, 0x58, 0x76, 0xd4, 0x9c, 0xba, 0x2c, 0x84, 0xc6, 0x83, 0x7a,
 0x22, 0x0d, 0x03, 0x69, 0x32, 0x1a, 0x6d, 0xcb, 0x0c, 0x15, 0xb3, 0x6b, 0xc7,
 0x0a, 0x8c, 0xb4, 0x5c, 0x34, 0x78, 0xe0, 0x3c, 0x9c, 0xe9, 0xf3, 0x30, 0x9f,
 0xa8, 0x76, 0x57, 0x92, 0x36 };

static CHAR unisp_name_a[] = UNISP_NAME_A;

static const char *algid_to_str(ALG_ID alg)
{
    static char buf[12];
    switch(alg) {
#define X(x) case x: return #x
        X(CALG_MD2);
        X(CALG_MD4);
        X(CALG_MD5);
        X(CALG_SHA1); /* same as CALG_SHA */
        X(CALG_MAC);
        X(CALG_RSA_SIGN);
        X(CALG_DSS_SIGN);
        X(CALG_NO_SIGN);
        X(CALG_RSA_KEYX);
        X(CALG_DES);
        X(CALG_3DES_112);
        X(CALG_3DES);
        X(CALG_DESX);
        X(CALG_RC2);
        X(CALG_RC4);
        X(CALG_SEAL);
        X(CALG_DH_SF);
        X(CALG_DH_EPHEM);
        X(CALG_AGREEDKEY_ANY);
        X(CALG_KEA_KEYX);
        X(CALG_HUGHES_MD5);
        X(CALG_SKIPJACK);
        X(CALG_TEK);
        X(CALG_CYLINK_MEK);
        X(CALG_SSL3_SHAMD5);
        X(CALG_SSL3_MASTER);
        X(CALG_SCHANNEL_MASTER_HASH);
        X(CALG_SCHANNEL_MAC_KEY);
        X(CALG_SCHANNEL_ENC_KEY);
        X(CALG_PCT1_MASTER);
        X(CALG_SSL2_MASTER);
        X(CALG_TLS1_MASTER);
        X(CALG_RC5);
        X(CALG_HMAC);
        X(CALG_TLS1PRF);
        X(CALG_HASH_REPLACE_OWF);
        X(CALG_AES_128);
        X(CALG_AES_192);
        X(CALG_AES_256);
        X(CALG_AES);
        X(CALG_SHA_256);
        X(CALG_SHA_384);
        X(CALG_SHA_512);
        X(CALG_ECDH);
        X(CALG_ECMQV);
        X(CALG_ECDSA);
#undef X
    }

    sprintf(buf, "%x", alg);
    return buf;
}

static void init_cred(SCHANNEL_CRED *cred)
{
    cred->dwVersion = SCHANNEL_CRED_VERSION;
    cred->cCreds = 0;
    cred->paCred = 0;
    cred->hRootStore = NULL;
    cred->cMappers = 0;
    cred->aphMappers = NULL;
    cred->cSupportedAlgs = 0;
    cred->palgSupportedAlgs = NULL;
    cred->grbitEnabledProtocols = 0;
    cred->dwMinimumCipherStrength = 0;
    cred->dwMaximumCipherStrength = 0;
    cred->dwSessionLifespan = 0;
    cred->dwFlags = 0;
}

static void test_strength(PCredHandle handle)
{
    SecPkgCred_CipherStrengths strength = {-1,-1};
    SECURITY_STATUS st;

    st = QueryCredentialsAttributesA(handle, SECPKG_ATTR_CIPHER_STRENGTHS, &strength);
    ok(st == SEC_E_OK, "QueryCredentialsAttributesA failed: %lu\n", GetLastError());
    ok(strength.dwMinimumCipherStrength, "dwMinimumCipherStrength not changed\n");
    ok(strength.dwMaximumCipherStrength, "dwMaximumCipherStrength not changed\n");
    trace("strength %ld - %ld\n", strength.dwMinimumCipherStrength, strength.dwMaximumCipherStrength);
}

static void test_supported_protocols(CredHandle *handle, unsigned exprots)
{
    SecPkgCred_SupportedProtocols protocols;
    SECURITY_STATUS status;

    status = QueryCredentialsAttributesA(handle, SECPKG_ATTR_SUPPORTED_PROTOCOLS, &protocols);
    ok(status == SEC_E_OK, "QueryCredentialsAttributes failed: %08lx\n", status);

    if(exprots)
        ok(protocols.grbitProtocol == exprots, "protocols.grbitProtocol = %lx, expected %x\n", protocols.grbitProtocol, exprots);

    trace("Supported protocols:\n");

#define X(flag, name) do { if(protocols.grbitProtocol & flag) { trace(name "\n"); protocols.grbitProtocol &= ~flag; } }while(0)
    X(SP_PROT_SSL2_CLIENT, "SSL 2 client");
    X(SP_PROT_SSL3_CLIENT, "SSL 3 client");
    X(SP_PROT_TLS1_0_CLIENT, "TLS 1.0 client");
    X(SP_PROT_TLS1_1_CLIENT, "TLS 1.1 client");
    X(SP_PROT_TLS1_2_CLIENT, "TLS 1.2 client");
    X(SP_PROT_TLS1_3_CLIENT, "TLS 1.3 client");
    X(SP_PROT_DTLS1_0_CLIENT, "DTLS 1.0 client");
    X(SP_PROT_DTLS1_2_CLIENT, "DTLS 1.2 client");
#undef X

    if(protocols.grbitProtocol)
        trace("Unknown flags: %lx\n", protocols.grbitProtocol);
}

static void test_supported_algs(CredHandle *handle)
{
    SecPkgCred_SupportedAlgs algs;
    SECURITY_STATUS status;
    unsigned i;

    status = QueryCredentialsAttributesA(handle, SECPKG_ATTR_SUPPORTED_ALGS, &algs);
    todo_wine ok(status == SEC_E_OK, "QueryCredentialsAttributes failed: %08lx\n", status);
    if(status != SEC_E_OK)
        return;

    trace("Supported algorithms (%ld):\n", algs.cSupportedAlgs);
    for(i=0; i < algs.cSupportedAlgs; i++)
        trace("    %s\n", algid_to_str(algs.palgSupportedAlgs[i]));

    FreeContextBuffer(algs.palgSupportedAlgs);
}

static void test_cread_attrs(void)
{
    SCHANNEL_CRED schannel_cred;
    SECURITY_STATUS status;
    CredHandle cred;

    status = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
            NULL, NULL, NULL, NULL, &cred, NULL);
    ok(status == SEC_E_OK, "AcquireCredentialsHandleA failed: %lx\n", status);

    test_supported_protocols(&cred, 0);
    test_supported_algs(&cred);

    status = QueryCredentialsAttributesA(&cred, SECPKG_ATTR_SUPPORTED_PROTOCOLS, NULL);
    ok(status == SEC_E_INTERNAL_ERROR, "QueryCredentialsAttributes failed: %08lx, expected SEC_E_INTERNAL_ERROR\n", status);

    status = QueryCredentialsAttributesA(&cred, SECPKG_ATTR_SUPPORTED_ALGS, NULL);
    ok(status == SEC_E_INTERNAL_ERROR, "QueryCredentialsAttributes failed: %08lx, expected SEC_E_INTERNAL_ERROR\n", status);

    FreeCredentialsHandle(&cred);

    init_cred(&schannel_cred);
    schannel_cred.grbitEnabledProtocols = SP_PROT_TLS1_CLIENT;
    status = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
            NULL, &schannel_cred, NULL, NULL, &cred, NULL);
    ok(status == SEC_E_OK, "AcquireCredentialsHandleA failed: %lx\n", status);

    test_supported_protocols(&cred, SP_PROT_TLS1_CLIENT);
    test_supported_algs(&cred);

    FreeCredentialsHandle(&cred);
}

static void testAcquireSecurityContext(void)
{
    BOOL has_schannel = FALSE;
    SecPkgInfoA *package_info;
    ULONG i;
    SECURITY_STATUS st;
    CredHandle cred;
    SecPkgCredentials_NamesA names;
    TimeStamp exp;
    SCHANNEL_CRED schanCred;
    SCH_CREDENTIALS schCred;
    PCCERT_CONTEXT certs[2];
    HCRYPTPROV csp;
    WCHAR ms_def_prov_w[MAX_PATH];
    BOOL ret;
    HCRYPTKEY key;
    CRYPT_KEY_PROV_INFO keyProvInfo;

    if (SUCCEEDED(EnumerateSecurityPackagesA(&i, &package_info)))
    {
        while(i--)
        {
            if (!strcmp(package_info[i].Name, unisp_name_a))
            {
                has_schannel = TRUE;
                break;
            }
        }
        FreeContextBuffer(package_info);
    }
    if (!has_schannel)
    {
        skip("Schannel not available\n");
        return;
    }

    lstrcpyW(ms_def_prov_w, MS_DEF_PROV_W);

    keyProvInfo.pwszContainerName = cspNameW;
    keyProvInfo.pwszProvName = ms_def_prov_w;
    keyProvInfo.dwProvType = PROV_RSA_FULL;
    keyProvInfo.dwFlags = 0;
    keyProvInfo.cProvParam = 0;
    keyProvInfo.rgProvParam = NULL;
    keyProvInfo.dwKeySpec = AT_SIGNATURE;

    certs[0] = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
    certs[1] = CertCreateCertificateContext(X509_ASN_ENCODING, selfSignedCert, sizeof(selfSignedCert));

    SetLastError(0xdeadbeef);
    ret = CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL, CRYPT_DELETEKEYSET);

    st = AcquireCredentialsHandleA(NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_SECPKG_NOT_FOUND, "Expected SEC_E_SECPKG_NOT_FOUND, got %08lx\n", st);

    st = AcquireCredentialsHandleA(NULL, unisp_name_a, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08lx\n", st);

    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_BOTH, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08lx\n", st);

    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND, NULL, NULL, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08lx\n", st);

    if (0) /* crash */
    {
        AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, NULL, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", st);
    if(st == SEC_E_OK)
        FreeCredentialsHandle(&cred);
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND, NULL, NULL, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_NO_CREDENTIALS, "st = %08lx\n", st);
    memset(&cred, 0, sizeof(cred));
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, NULL, NULL, NULL, &cred, &exp);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", st);
    /* expriy is indeterminate in win2k3 */
    trace("expiry: %08lx%08lx\n", exp.HighPart, exp.LowPart);

    st = QueryCredentialsAttributesA(&cred, SECPKG_CRED_ATTR_NAMES, &names);
    ok(st == SEC_E_NO_CREDENTIALS, "expected SEC_E_NO_CREDENTIALS, got %08lx\n", st);

    FreeCredentialsHandle(&cred);

    /* Should fail if no enabled protocols are available */
    init_cred(&schanCred);
    schanCred.grbitEnabledProtocols = SP_PROT_TLS1_X_SERVER;
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND, NULL, &schanCred, NULL, NULL, &cred, &exp);
    ok(st == SEC_E_ALGORITHM_MISMATCH, "st = %08lx\n", st);
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND, NULL, &schanCred, NULL, NULL, &cred, &exp);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", st);
    FreeCredentialsHandle(&cred);

    schanCred.grbitEnabledProtocols = SP_PROT_TLS1_X_CLIENT;
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND, NULL, &schanCred, NULL, NULL, &cred, &exp);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", st);
    FreeCredentialsHandle(&cred);
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND, NULL, &schanCred, NULL, NULL, &cred, &exp);
    ok(st == SEC_E_ALGORITHM_MISMATCH, "st = %08lx\n", st);

    /* Bad version in SCHANNEL_CRED */
    memset(&schanCred, 0, sizeof(schanCred));
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS, "st = %08lx\n", st);
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS, "st = %08lx\n", st);

    /* No cert in SCHANNEL_CRED succeeds for outbound.. */
    schanCred.dwVersion = SCHANNEL_CRED_VERSION;
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", st);
    FreeCredentialsHandle(&cred);
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_OK, "Expected SEC_E_OK, got %08lx\n", st);

    if (0)
    {
        /* Crashes with bad paCred pointer */
        schanCred.cCreds = 1;
        AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, NULL, NULL);
    }

    /* Bogus cert in SCHANNEL_CRED. Windows fails with SEC_E_UNKNOWN_CREDENTIALS. */
    schanCred.cCreds = 1;
    schanCred.paCred = &certs[0];
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS, "st = %08lx\n", st);
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS, "st = %08lx\n", st);

    /* Good cert, but missing private key. Windows fails with SEC_E_NO_CREDENTIALS. */
    schanCred.cCreds = 1;
    schanCred.paCred = &certs[1];
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schanCred, NULL, NULL, &cred, NULL);
    todo_wine ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08lx\n", st);
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
     NULL, &schanCred, NULL, NULL, NULL, NULL);
    todo_wine ok(st == SEC_E_NO_CREDENTIALS, "Expected SEC_E_NO_CREDENTIALS, got %08lx\n", st);

    /* Good cert, with CRYPT_KEY_PROV_INFO set before it's had a key loaded. */
    ret = CertSetCertificateContextProperty(certs[1],
          CERT_KEY_PROV_INFO_PROP_ID, 0, &keyProvInfo);
    schanCred.dwVersion = SCH_CRED_V3;
    ok(ret, "CertSetCertificateContextProperty failed: %08lx\n", GetLastError());
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
        NULL, &schanCred, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS || st == SEC_E_INSUFFICIENT_MEMORY /* win10 */,
       "Expected SEC_E_INSUFFICIENT_MEMORY, got %08lx\n", st);
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
        NULL, &schanCred, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_UNKNOWN_CREDENTIALS || st == SEC_E_INSUFFICIENT_MEMORY /* win10 */,
     "Expected SEC_E_INSUFFICIENT_MEMORY, got %08lx\n", st);

    ret = CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL,
     CRYPT_NEWKEYSET);
    ok(ret, "CryptAcquireContextW failed: %08lx\n", GetLastError());
    ret = 0;

    ret = CryptImportKey(csp, privKey, sizeof(privKey), 0, 0, &key);
    ok(ret, "CryptImportKey failed: %08lx\n", GetLastError());
    if (ret)
    {
        PCCERT_CONTEXT tmp;

        if (0)
        {
            /* Crashes */
            AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
             NULL, &schanCred, NULL, NULL, NULL, NULL);
        }

        /* Good cert with private key, bogus version */
        schanCred.dwVersion = SCH_CRED_V1;
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS, "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08lx\n", st);
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS, "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08lx\n", st);
        schanCred.dwVersion = SCH_CRED_V2;
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS, "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08lx\n", st);
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS, "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08lx\n", st);

        /* Succeeds on V3 or higher */
        schanCred.dwVersion = SCH_CRED_V3;
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        todo_wine ok(st == SEC_E_INSUFFICIENT_MEMORY || broken(st == S_OK) /* <win10 */,
         "AcquireCredentialsHandleA failed: %08lx\n", st);
        if (st == S_OK) FreeCredentialsHandle(&cred);
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        todo_wine ok(st == SEC_E_INSUFFICIENT_MEMORY || broken(st == S_OK) /* <win10 */,
         "AcquireCredentialsHandleA failed: %08lx\n", st);
        if (st == S_OK) FreeCredentialsHandle(&cred);
        schanCred.dwVersion = SCHANNEL_CRED_VERSION;
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", st);
        FreeCredentialsHandle(&cred);
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", st);
        if (st == SEC_E_OK) test_strength(&cred);
        FreeCredentialsHandle(&cred);

        /* How about more than one cert? */
        schanCred.cCreds = 2;
        schanCred.paCred = certs;
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS, "st = %08lx\n", st);
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS, "st = %08lx\n", st);
        tmp = certs[0];
        certs[0] = certs[1];
        certs[1] = tmp;
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS, "st = %08lx\n", st);
        st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_INBOUND,
         NULL, &schanCred, NULL, NULL, &cred, NULL);
        ok(st == SEC_E_UNKNOWN_CREDENTIALS, "Expected SEC_E_UNKNOWN_CREDENTIALS, got %08lx\n", st);
        /* FIXME: what about two valid certs? */

        CryptDestroyKey(key);
    }

    memset(&schCred, 0, sizeof(schCred));
    schCred.dwVersion = SCH_CREDENTIALS_VERSION;
    st = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND,
     NULL, &schCred, NULL, NULL, &cred, NULL);
    ok(st == SEC_E_OK || broken(st == SEC_E_UNKNOWN_CREDENTIALS) /* <= win10v1570 */,
       "AcquireCredentialsHandleA failed: %08lx\n", st);
    FreeCredentialsHandle(&cred);

    CryptReleaseContext(csp, 0);
    CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL, CRYPT_DELETEKEYSET);

    CertFreeCertificateContext(certs[0]);
    CertFreeCertificateContext(certs[1]);
}

static void test_remote_cert(PCCERT_CONTEXT remote_cert)
{
    PCCERT_CONTEXT iter = NULL;
    BOOL incl_remote = FALSE;
    unsigned cert_cnt = 0;

    ok(remote_cert->hCertStore != NULL, "hCertStore == NULL\n");

    while((iter = CertEnumCertificatesInStore(remote_cert->hCertStore, iter))) {
        if(iter == remote_cert)
            incl_remote = TRUE;
        cert_cnt++;
    }

    ok(cert_cnt == 3, "cert_cnt = %u\n", cert_cnt);
    ok(incl_remote, "context does not contain cert itself\n");
}

static const char http_request[] = "GET /tests/clientcert/ HTTP/1.1\r\nHost: test.winehq.org\r\nConnection: close\r\n\r\n";

static void init_buffers(SecBufferDesc *desc, unsigned count, unsigned size)
{
    desc->ulVersion = SECBUFFER_VERSION;
    desc->cBuffers = count;
    desc->pBuffers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count*sizeof(SecBuffer));

    desc->pBuffers[0].cbBuffer = size;
    desc->pBuffers[0].pvBuffer = HeapAlloc(GetProcessHeap(), 0, size);
}

static void init_sec_buffer(SecBuffer *sec_buf, ULONG count, void *buf)
{
    sec_buf->cbBuffer = count;
    sec_buf->pvBuffer = buf;
}

static void reset_buffers(SecBufferDesc *desc)
{
    unsigned i;

    for (i = 0; i < desc->cBuffers; ++i)
    {
        desc->pBuffers[i].BufferType = SECBUFFER_EMPTY;
        if (i > 0)
        {
            desc->pBuffers[i].cbBuffer = 0;
            desc->pBuffers[i].pvBuffer = NULL;
        }
    }
}

static void free_buffers(SecBufferDesc *desc)
{
    HeapFree(GetProcessHeap(), 0, desc->pBuffers[0].pvBuffer);
    HeapFree(GetProcessHeap(), 0, desc->pBuffers);
}

static int receive_data(SOCKET sock, SecBuffer *buf)
{
    unsigned received = 0;

    while (1)
    {
        unsigned char *data = buf->pvBuffer;
        unsigned expected = 0;
        int ret;

        ret = recv(sock, (char *)data+received, buf->cbBuffer-received, 0);
        if (ret == -1)
        {
            skip("recv failed\n");
            return -1;
        }
        else if(ret == 0)
        {
            skip("connection closed\n");
            return -1;
        }
        received += ret;

        while (expected < received)
        {
            unsigned frame_size = 5 + ((data[3]<<8) | data[4]);
            expected += frame_size;
            data += frame_size;
        }

        if (expected == received)
            break;
    }

    buf->cbBuffer = received;

    return received;
}

static void test_context_output_buffer_size(DWORD protocol, DWORD flags, ULONG ctxt_flags_req)
{
    SCHANNEL_CRED cred;
    CredHandle cred_handle;
    CtxtHandle context;
    SECURITY_STATUS status;
    SecBuffer in_buffer = {0, SECBUFFER_EMPTY, NULL};
    SecBufferDesc in_buffers  = {SECBUFFER_VERSION, 1, &in_buffer};
    SecBufferDesc out_buffers;
    unsigned buf_size = 8192;
    void *buf, *buf2;
    ULONG attrs;
    int i;

    init_cred(&cred);
    cred.grbitEnabledProtocols = protocol;
    cred.dwFlags = flags;
    status = AcquireCredentialsHandleA(NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
        &cred, NULL, NULL, &cred_handle, NULL);
    ok( status == SEC_E_OK, "got %08lx\n", status );
    if (status != SEC_E_OK) return;

    init_buffers(&out_buffers, 4, buf_size);
    out_buffers.pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buf = out_buffers.pBuffers[0].pvBuffer;
    buf2 = out_buffers.pBuffers[1].pvBuffer = HeapAlloc(GetProcessHeap(), 0, buf_size);
    for (i = 0; i < 2; i++)
    {
        SecBuffer *buffer = !i ? &out_buffers.pBuffers[0] : &out_buffers.pBuffers[1];

        init_sec_buffer(&out_buffers.pBuffers[0], buf_size, buf);
        if (i) buffer->BufferType = SECBUFFER_ALERT;

        buffer->cbBuffer = 0;
        status = InitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost", ctxt_flags_req,
                0, 0, &in_buffers, 0, &context, &out_buffers, &attrs, NULL);
        ok(status == SEC_E_INSUFFICIENT_MEMORY, "%d: Expected SEC_E_INSUFFICIENT_MEMORY, got %08lx\n", i, status);

        if (i) init_sec_buffer(&out_buffers.pBuffers[0], buf_size, (void *)0xdeadbeef);
        init_sec_buffer(buffer, 0, NULL);
        status = InitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost",
                ctxt_flags_req | ISC_REQ_ALLOCATE_MEMORY, 0, 0, &in_buffers, 0, &context, &out_buffers, &attrs, NULL);
        ok(status == SEC_I_CONTINUE_NEEDED, "%d: Expected SEC_I_CONTINUE_NEEDED, got %08lx\n", i, status);
        ok(out_buffers.pBuffers[0].pvBuffer != (void *)0xdeadbeef, "got %p.\n", out_buffers.pBuffers[0].pvBuffer);
        if (i) FreeContextBuffer(out_buffers.pBuffers[0].pvBuffer);
        FreeContextBuffer(buffer->pvBuffer);
        DeleteSecurityContext(&context);

        if (i) init_sec_buffer(&out_buffers.pBuffers[0], buf_size, buf);
        init_sec_buffer(buffer, buf_size, !i ? buf : buf2);
        status = InitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost", ctxt_flags_req,
                0, 0, &in_buffers, 0, &context, &out_buffers, &attrs, NULL);
        ok(status == SEC_I_CONTINUE_NEEDED, "%d: Expected SEC_I_CONTINUE_NEEDED, got %08lx\n", i, status);
        if (i) ok(!buffer->cbBuffer, "Expected SECBUFFER_ALERT buffer to be empty\n");
        DeleteSecurityContext(&context);
    }

    HeapFree(GetProcessHeap(), 0, buf2);
    free_buffers(&out_buffers);
    FreeCredentialsHandle(&cred_handle);
}

static void test_InitializeSecurityContext(void)
{
    SCHANNEL_CRED cred;
    CredHandle cred_handle;
    CtxtHandle context;
    SECURITY_STATUS status;
    SecBuffer out_buffer = {1000, SECBUFFER_TOKEN, NULL};
    SecBuffer in_buffer = {0, SECBUFFER_EMPTY, NULL};
    SecBufferDesc out_buffers = {SECBUFFER_VERSION, 1, &out_buffer};
    SecBufferDesc in_buffers  = {SECBUFFER_VERSION, 1, &in_buffer};
    ULONG attrs;

    init_cred(&cred);
    cred.grbitEnabledProtocols = SP_PROT_TLS1_CLIENT;
    cred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_MANUAL_CRED_VALIDATION;
    status = AcquireCredentialsHandleA(NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
        &cred, NULL, NULL, &cred_handle, NULL);
    ok(status == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", status);
    if (status != SEC_E_OK) return;

    status = InitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost",
        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM|ISC_REQ_ALLOCATE_MEMORY,
        0, 0, &in_buffers, 0, &context, &out_buffers, &attrs, NULL);
    ok(status == SEC_I_CONTINUE_NEEDED, "Expected SEC_I_CONTINUE_NEEDED, got %08lx\n", status);

    FreeContextBuffer(out_buffer.pvBuffer);
    DeleteSecurityContext(&context);
    FreeCredentialsHandle(&cred_handle);
}

static SOCKET create_ssl_socket( const char *hostname )
{
    struct hostent *host;
    struct sockaddr_in addr;
    SOCKET sock;

    if (!(host = gethostbyname(hostname)))
    {
        skip("Can't resolve \"%s\"\n", hostname);
        return -1;
    }

    addr.sin_family = host->h_addrtype;
    addr.sin_addr = *(struct in_addr *)host->h_addr_list[0];
    addr.sin_port = htons(443);
    if ((sock = socket(host->h_addrtype, SOCK_STREAM, 0)) == -1)
    {
        skip("Can't create socket\n");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        skip("Can't connect to \"%s\"\n", hostname);
        closesocket(sock);
        return -1;
    }

    return sock;
}

static const BYTE pfxdata[] =
{
    0x30, 0x82, 0x0b, 0x1d, 0x02, 0x01, 0x03, 0x30, 0x82, 0x0a, 0xe3, 0x06,
    0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01, 0xa0, 0x82,
    0x0a, 0xd4, 0x04, 0x82, 0x0a, 0xd0, 0x30, 0x82, 0x0a, 0xcc, 0x30, 0x82,
    0x05, 0x07, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07,
    0x06, 0xa0, 0x82, 0x04, 0xf8, 0x30, 0x82, 0x04, 0xf4, 0x02, 0x01, 0x00,
    0x30, 0x82, 0x04, 0xed, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d,
    0x01, 0x07, 0x01, 0x30, 0x1c, 0x06, 0x0a, 0x2a, 0x86, 0x48, 0x86, 0xf7,
    0x0d, 0x01, 0x0c, 0x01, 0x06, 0x30, 0x0e, 0x04, 0x08, 0xac, 0x3e, 0x35,
    0xa8, 0xed, 0x0d, 0x50, 0x07, 0x02, 0x02, 0x08, 0x00, 0x80, 0x82, 0x04,
    0xc0, 0x5a, 0x62, 0x55, 0x25, 0xf6, 0x2c, 0xf1, 0x78, 0x6c, 0x63, 0x96,
    0x8a, 0xea, 0x04, 0x64, 0xb3, 0x99, 0x3b, 0x80, 0x50, 0x05, 0x37, 0x55,
    0xa3, 0x5e, 0x9f, 0x35, 0xc3, 0x3c, 0xdc, 0xf6, 0xc4, 0xc1, 0x39, 0xa2,
    0xd7, 0x50, 0xad, 0xf9, 0x29, 0x3c, 0x51, 0xea, 0x15, 0x20, 0x25, 0xd3,
    0x4d, 0x69, 0xdf, 0x10, 0xd8, 0x9d, 0x60, 0x78, 0x8a, 0x70, 0x44, 0x7f,
    0x01, 0x4f, 0x4a, 0xfa, 0xab, 0xfd, 0x46, 0x48, 0x96, 0x2b, 0x69, 0xfc,
    0x11, 0xf8, 0x3f, 0xd3, 0x79, 0x09, 0x75, 0x81, 0x47, 0xdf, 0xce, 0xfe,
    0x07, 0x2f, 0x0a, 0xd8, 0xac, 0x87, 0x14, 0x1f, 0x7b, 0x95, 0x70, 0xee,
    0x7e, 0x52, 0x90, 0x11, 0xd6, 0x69, 0xf4, 0xd5, 0x38, 0x85, 0xc9, 0xc1,
    0x07, 0x01, 0xe8, 0xbb, 0xfb, 0xe2, 0x08, 0xa8, 0xfa, 0xbf, 0xf0, 0x92,
    0x63, 0x1d, 0xbb, 0x2b, 0x45, 0x6f, 0xce, 0x97, 0x01, 0xd7, 0x95, 0xf0,
    0x9c, 0x9a, 0x6b, 0x73, 0x01, 0xbf, 0xf9, 0x3d, 0xc8, 0x2b, 0x86, 0x7a,
    0xd5, 0x65, 0x84, 0xd7, 0xff, 0xb2, 0xf9, 0x20, 0x52, 0x35, 0xc5, 0x60,
    0x33, 0x70, 0x1d, 0x2f, 0x26, 0x09, 0x1c, 0x22, 0x17, 0xd8, 0x08, 0x4e,
    0x69, 0x20, 0xe2, 0x71, 0xe4, 0x07, 0xb1, 0x48, 0x5f, 0x20, 0x08, 0x7a,
    0xbf, 0x65, 0x53, 0x23, 0x07, 0xf9, 0x6c, 0xde, 0x3e, 0x29, 0xbf, 0x6b,
    0xef, 0xbb, 0x6a, 0x5f, 0x79, 0xa1, 0x72, 0xa1, 0x10, 0x24, 0x80, 0xb4,
    0x44, 0xb8, 0xc9, 0xfc, 0xa3, 0x36, 0x7e, 0x23, 0x37, 0x58, 0xc6, 0x1e,
    0xe8, 0x42, 0x4d, 0xb5, 0xf5, 0x58, 0x93, 0x21, 0x38, 0xa2, 0xc4, 0xa9,
    0x01, 0x96, 0xf9, 0x61, 0xac, 0x55, 0xb3, 0x3d, 0xe4, 0x54, 0x8b, 0x6c,
    0xc3, 0x83, 0xff, 0x50, 0x87, 0x94, 0xe8, 0x35, 0x3c, 0x26, 0x0d, 0x20,
    0x8a, 0x25, 0x0e, 0xb6, 0x67, 0x78, 0x29, 0xc7, 0xbf, 0x76, 0x8e, 0x62,
    0x62, 0xc4, 0x50, 0xd6, 0xc5, 0x3c, 0xb4, 0x7a, 0x35, 0xbe, 0x53, 0x52,
    0xc4, 0xe4, 0x10, 0xb3, 0xe0, 0x73, 0xb0, 0xd1, 0xc1, 0x5a, 0x4f, 0x4e,
    0x64, 0x0d, 0x92, 0x51, 0x2d, 0x4d, 0xec, 0xb0, 0xc6, 0x40, 0x1b, 0x03,
    0x89, 0x7f, 0xc2, 0x2c, 0xe3, 0x2c, 0xbd, 0x8c, 0x9c, 0xd9, 0xe0, 0x08,
    0x59, 0xd3, 0xaf, 0x48, 0x56, 0x89, 0x60, 0x85, 0x76, 0xe0, 0xd8, 0x7c,
    0xcf, 0x02, 0x8f, 0xfd, 0xb2, 0x8f, 0x2b, 0x61, 0xcf, 0x28, 0x56, 0x8b,
    0x6b, 0x03, 0x2b, 0x2f, 0x83, 0x31, 0xa0, 0x1c, 0xd1, 0x6c, 0x87, 0x49,
    0xc4, 0x77, 0x55, 0x1f, 0x61, 0x45, 0x58, 0x88, 0x9f, 0x01, 0xc3, 0x63,
    0x62, 0x30, 0x35, 0xdf, 0x61, 0x74, 0x55, 0x63, 0x3f, 0xae, 0x41, 0xc1,
    0xb8, 0xf0, 0x9f, 0xab, 0x25, 0xad, 0x41, 0x5c, 0x1f, 0x00, 0x0d, 0xef,
    0xf0, 0xcf, 0xaf, 0x41, 0x23, 0xca, 0x8c, 0x38, 0xea, 0x5a, 0xe4, 0x8b,
    0xb4, 0x89, 0xd0, 0x76, 0x7f, 0x2b, 0x77, 0x8f, 0xe4, 0x44, 0xd5, 0x37,
    0xac, 0xc2, 0x09, 0x7e, 0x7e, 0x7e, 0x02, 0x5c, 0x27, 0x01, 0xcb, 0x4d,
    0xea, 0xb3, 0x97, 0x36, 0x35, 0xd2, 0x05, 0x3c, 0x4e, 0xb8, 0x04, 0x5c,
    0xb8, 0x95, 0x3f, 0xc6, 0xbf, 0xd4, 0x20, 0x01, 0xfb, 0xed, 0x37, 0x5a,
    0xad, 0x4c, 0x61, 0x93, 0xfe, 0x95, 0x7c, 0x34, 0x11, 0x15, 0x9d, 0x00,
    0x0b, 0x99, 0x69, 0xcb, 0x7e, 0xb9, 0x53, 0x46, 0x57, 0x39, 0x3f, 0x59,
    0x4b, 0x30, 0x8d, 0xfb, 0x84, 0x66, 0x2d, 0x06, 0xc9, 0x88, 0xa6, 0x18,
    0xd7, 0x36, 0xc6, 0xf6, 0xf7, 0x47, 0x85, 0x38, 0xc8, 0x3d, 0x37, 0xea,
    0x57, 0x4c, 0xb0, 0x7c, 0x95, 0x29, 0x84, 0xab, 0xbb, 0x19, 0x86, 0xc2,
    0xc5, 0x99, 0x01, 0x38, 0x6b, 0xf1, 0xd3, 0x1d, 0xa8, 0x02, 0xf9, 0x6f,
    0xaa, 0xf1, 0x57, 0xd0, 0x88, 0x68, 0x62, 0x5f, 0x9f, 0x7a, 0x63, 0xba,
    0x3a, 0xc9, 0x95, 0x11, 0x3c, 0xf9, 0xa1, 0xc1, 0x35, 0xfe, 0xd5, 0x12,
    0x49, 0x88, 0x0d, 0x5c, 0xe2, 0xd1, 0x15, 0x18, 0xfb, 0xd5, 0x7f, 0x19,
    0x3f, 0xaf, 0xa0, 0xcb, 0x31, 0x20, 0x9e, 0x03, 0x93, 0xa4, 0x66, 0xbd,
    0x83, 0xe8, 0x60, 0x34, 0x55, 0x0d, 0x97, 0x10, 0x23, 0x24, 0x7a, 0x45,
    0x36, 0xb4, 0xc4, 0xee, 0x60, 0x6f, 0xd8, 0x46, 0xc5, 0xac, 0x2b, 0xa9,
    0x18, 0x74, 0x83, 0x1e, 0xdf, 0x7c, 0x1a, 0x5a, 0xe8, 0x5f, 0x8b, 0x4f,
    0x9f, 0x40, 0x3e, 0x5e, 0xfb, 0xd3, 0x68, 0xac, 0x34, 0x62, 0x30, 0x23,
    0xb6, 0xbc, 0xdf, 0xbc, 0xc7, 0x25, 0xd2, 0x1b, 0x57, 0x33, 0xfb, 0x78,
    0x22, 0x21, 0x1e, 0x3a, 0xf6, 0x44, 0x18, 0x7e, 0x12, 0x36, 0x47, 0x58,
    0xd0, 0x59, 0x26, 0x98, 0x98, 0x95, 0xf4, 0xd1, 0xaa, 0x45, 0xaa, 0xe7,
    0xd1, 0xe6, 0x2d, 0x78, 0xf0, 0x8b, 0x1c, 0xfd, 0xf8, 0x50, 0x60, 0xa2,
    0x1e, 0x7f, 0xe3, 0x31, 0x77, 0x31, 0x58, 0x99, 0x0f, 0xda, 0x0e, 0xa3,
    0xc6, 0x7a, 0x30, 0x45, 0x55, 0x11, 0x91, 0x77, 0x41, 0x79, 0xd3, 0x56,
    0xb2, 0x07, 0x00, 0x61, 0xab, 0xec, 0x27, 0xc7, 0x9f, 0xfa, 0x89, 0x08,
    0xc2, 0x87, 0xcf, 0xe9, 0xdc, 0x9e, 0x29, 0x22, 0xfb, 0x23, 0x7f, 0x9d,
    0x89, 0xd5, 0x6e, 0x75, 0x20, 0xd8, 0x00, 0x5b, 0xc4, 0x94, 0xbb, 0xc5,
    0xb2, 0xba, 0x77, 0x2b, 0xf6, 0x3c, 0x88, 0xb0, 0x4c, 0x38, 0x46, 0x55,
    0xee, 0x8b, 0x03, 0x15, 0xbc, 0x0a, 0x1d, 0x47, 0x87, 0x44, 0xaf, 0xb1,
    0x2a, 0xa7, 0x4d, 0x08, 0xdf, 0x3b, 0x2d, 0x70, 0xa1, 0x67, 0x31, 0x76,
    0x6e, 0x6f, 0x40, 0x3b, 0x3b, 0xe8, 0xf9, 0xdf, 0x90, 0xa4, 0xce, 0x7f,
    0xb8, 0x2d, 0x69, 0xcb, 0x1c, 0x1e, 0x94, 0xcd, 0xb1, 0xd8, 0x43, 0x22,
    0xb8, 0x4f, 0x98, 0x92, 0x74, 0xb3, 0xde, 0xeb, 0x7a, 0xcb, 0xfa, 0xd0,
    0x36, 0xe4, 0x5d, 0xfa, 0xd3, 0xce, 0xf9, 0xba, 0x3e, 0x0f, 0x6c, 0xc3,
    0x5b, 0xb3, 0x81, 0x84, 0x6e, 0x5d, 0xc1, 0x21, 0x89, 0xec, 0x67, 0x9a,
    0xfd, 0x55, 0x20, 0xb0, 0x71, 0x53, 0xae, 0xf8, 0xa4, 0x8d, 0xd5, 0xe5,
    0x2d, 0x3a, 0xce, 0x89, 0x55, 0x8c, 0x4f, 0x3b, 0x37, 0x95, 0x4e, 0x15,
    0xbe, 0xe7, 0xd1, 0x7a, 0x36, 0x82, 0x45, 0x69, 0x7c, 0x27, 0x4f, 0xb9,
    0x4b, 0x7d, 0xcd, 0x59, 0xc8, 0xf4, 0x8b, 0x0f, 0x4f, 0x75, 0x23, 0xd3,
    0xd0, 0xc7, 0x10, 0x79, 0xc0, 0xf1, 0xac, 0x14, 0xf7, 0x0d, 0xc8, 0x5e,
    0xfc, 0xff, 0x1a, 0x2b, 0x10, 0x88, 0x7e, 0x7e, 0x2f, 0xfa, 0x7b, 0x9f,
    0x47, 0x23, 0x34, 0xfc, 0xf5, 0xde, 0xd9, 0xa3, 0x05, 0x99, 0x2a, 0x96,
    0x83, 0x3d, 0xa4, 0x7f, 0x6a, 0x66, 0x9b, 0xe7, 0xf1, 0x00, 0x4e, 0x9a,
    0xfc, 0x68, 0xd2, 0x74, 0x17, 0xba, 0xc9, 0xc8, 0x20, 0x39, 0xa1, 0xa8,
    0x85, 0xc6, 0x10, 0x2b, 0xab, 0x97, 0x34, 0x2d, 0x49, 0x68, 0x57, 0xb0,
    0x43, 0xee, 0x25, 0xbb, 0x35, 0x1b, 0x03, 0x99, 0xa3, 0x21, 0x68, 0x66,
    0x86, 0x3f, 0xc6, 0xfc, 0x49, 0xf0, 0xba, 0x5f, 0x00, 0xc6, 0xe3, 0x1c,
    0xb2, 0x9f, 0x16, 0x7f, 0xc7, 0x40, 0x4a, 0x9a, 0x39, 0xc1, 0x95, 0x69,
    0xa2, 0x87, 0xba, 0x58, 0xc6, 0xf2, 0xd6, 0x66, 0xa6, 0x4c, 0x6d, 0x29,
    0x9c, 0xa8, 0x6e, 0xa9, 0xd2, 0xe4, 0x54, 0x17, 0x89, 0xe2, 0x43, 0xf0,
    0xe1, 0x8b, 0x57, 0x84, 0x6c, 0x87, 0x63, 0x17, 0xbb, 0xf6, 0x33, 0x1b,
    0xe4, 0x34, 0x6a, 0x80, 0x70, 0x7b, 0x1b, 0xfd, 0xf8, 0x79, 0x28, 0xc8,
    0x3c, 0x8e, 0xa4, 0xd5, 0xb8, 0x96, 0x54, 0xd4, 0xec, 0x72, 0xe5, 0x40,
    0x8f, 0x56, 0xde, 0x82, 0x15, 0x72, 0x4d, 0xd8, 0x0c, 0x07, 0xea, 0xe6,
    0x44, 0xcd, 0x94, 0x73, 0x5c, 0x04, 0xe8, 0x8e, 0xb7, 0xc7, 0xc9, 0x29,
    0xdc, 0x04, 0xef, 0x7c, 0x31, 0x9b, 0x50, 0xbc, 0xea, 0x71, 0x1f, 0x28,
    0x22, 0xb6, 0x04, 0x53, 0x2e, 0x71, 0xc4, 0xf6, 0xbb, 0x88, 0x51, 0xee,
    0x3e, 0x76, 0x65, 0xb4, 0x4b, 0x1b, 0xa3, 0xec, 0x7b, 0xa7, 0x9d, 0x31,
    0x5d, 0xb8, 0x9f, 0xab, 0x6b, 0x54, 0x7d, 0xbd, 0xc1, 0x2c, 0x55, 0xb0,
    0x23, 0x8c, 0x06, 0x60, 0x01, 0x4f, 0x60, 0x85, 0x56, 0x7f, 0xfb, 0x99,
    0x0c, 0xdc, 0x8c, 0x09, 0x37, 0x46, 0x5b, 0x97, 0x5d, 0xe8, 0x31, 0x00,
    0x1b, 0x30, 0x9b, 0x02, 0x92, 0x29, 0xb5, 0x20, 0xce, 0x4b, 0x90, 0xfb,
    0x91, 0x07, 0x5a, 0xd3, 0xf5, 0xa0, 0xe6, 0x8f, 0xf8, 0x73, 0xc5, 0x4b,
    0xbb, 0xad, 0x2a, 0xeb, 0xa8, 0xb7, 0x68, 0x34, 0x36, 0x47, 0xd5, 0x4b,
    0x61, 0x89, 0x53, 0xe6, 0xb6, 0xb1, 0x07, 0xe4, 0x08, 0x2e, 0xed, 0x50,
    0xd4, 0x1e, 0xed, 0x7f, 0xbf, 0x35, 0x68, 0x04, 0x45, 0x72, 0x86, 0x71,
    0x15, 0x55, 0xdf, 0xe6, 0x30, 0xc0, 0x8b, 0x8a, 0xb0, 0x6c, 0xd0, 0x35,
    0x57, 0x8f, 0x04, 0x37, 0xbc, 0xe1, 0xb8, 0xbf, 0x27, 0x37, 0x3d, 0xd0,
    0xc8, 0x46, 0x67, 0x42, 0x51, 0x30, 0x82, 0x05, 0xbd, 0x06, 0x09, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01, 0xa0, 0x82, 0x05, 0xae,
    0x04, 0x82, 0x05, 0xaa, 0x30, 0x82, 0x05, 0xa6, 0x30, 0x82, 0x05, 0xa2,
    0x06, 0x0b, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x0c, 0x0a, 0x01,
    0x02, 0xa0, 0x82, 0x04, 0xee, 0x30, 0x82, 0x04, 0xea, 0x30, 0x1c, 0x06,
    0x0a, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x0c, 0x01, 0x03, 0x30,
    0x0e, 0x04, 0x08, 0x9f, 0xa4, 0x72, 0x2b, 0x6b, 0x0e, 0xcb, 0x9f, 0x02,
    0x02, 0x08, 0x00, 0x04, 0x82, 0x04, 0xc8, 0xe5, 0x35, 0xb9, 0x72, 0x28,
    0x20, 0x28, 0xad, 0xe3, 0x01, 0xd7, 0x0b, 0xe0, 0x4e, 0x36, 0xc3, 0x73,
    0x06, 0xd5, 0xf6, 0x75, 0x1a, 0x78, 0xb2, 0xd8, 0xf6, 0x5a, 0x85, 0x8e,
    0x50, 0xa3, 0x05, 0x49, 0x02, 0x2d, 0xf8, 0xa3, 0x2f, 0xe6, 0x02, 0x7a,
    0xd5, 0x0b, 0x1d, 0xf1, 0xd1, 0xe4, 0x16, 0xaa, 0x70, 0x2e, 0x34, 0xdb,
    0x56, 0xd9, 0x33, 0x94, 0x11, 0xaa, 0x60, 0xd4, 0xfa, 0x5b, 0xd1, 0xb3,
    0x2e, 0x86, 0x6a, 0x5a, 0x69, 0xdf, 0x11, 0x91, 0xb0, 0xca, 0x82, 0xff,
    0x63, 0xad, 0x6a, 0x0b, 0x90, 0xa6, 0xc7, 0x9b, 0xef, 0x9a, 0xf8, 0x96,
    0xec, 0xe4, 0xc4, 0xdf, 0x55, 0x4c, 0x12, 0x07, 0xab, 0x7c, 0x5c, 0x68,
    0x47, 0xf2, 0x92, 0xfb, 0x94, 0xab, 0xc3, 0x64, 0xd3, 0xfe, 0xb2, 0x16,
    0xb4, 0x78, 0x80, 0x52, 0xe9, 0x32, 0x39, 0x3b, 0x8d, 0x12, 0x91, 0x36,
    0xfd, 0xa1, 0x97, 0xc2, 0x0a, 0x4a, 0xf1, 0xb3, 0x8a, 0xe4, 0x01, 0xed,
    0x0a, 0xda, 0x2e, 0xa0, 0x38, 0xa9, 0x47, 0x3d, 0x3a, 0x64, 0x87, 0x06,
    0xc3, 0x83, 0x60, 0xaf, 0x84, 0xdb, 0x87, 0xff, 0x70, 0x61, 0x43, 0x7d,
    0x2d, 0x61, 0x9a, 0xf7, 0x0d, 0xca, 0x0c, 0x0f, 0xbe, 0x43, 0x5b, 0x99,
    0xe1, 0x90, 0x64, 0x1f, 0xa7, 0x1b, 0xa6, 0xa6, 0x5c, 0x13, 0x70, 0xa3,
    0xdb, 0xd7, 0xf0, 0xe8, 0x7a, 0xb0, 0xd1, 0x9b, 0x52, 0xa6, 0x4f, 0xd6,
    0xff, 0x54, 0x4d, 0xa6, 0x15, 0x05, 0x5c, 0xe9, 0x04, 0x6a, 0xc3, 0x49,
    0x12, 0x2f, 0x24, 0x03, 0xc3, 0x80, 0x06, 0xa6, 0x07, 0x8b, 0x96, 0xe7,
    0x39, 0x31, 0x6d, 0xd3, 0x1b, 0xa5, 0x45, 0x58, 0x04, 0xe7, 0x87, 0xdf,
    0x26, 0xfb, 0x1b, 0x9f, 0x92, 0x93, 0x32, 0x12, 0x9a, 0xc9, 0xe6, 0xcb,
    0x88, 0x14, 0x9f, 0x23, 0x0b, 0x52, 0xa2, 0xb8, 0x32, 0x6c, 0xa9, 0x33,
    0xa1, 0x17, 0xe8, 0x4a, 0xd4, 0x5c, 0x7d, 0xb3, 0xa3, 0x64, 0x86, 0x03,
    0x7c, 0x7c, 0x3f, 0x99, 0xdc, 0x21, 0x9f, 0x93, 0xc6, 0xb9, 0x1d, 0xe0,
    0x21, 0x79, 0x78, 0x35, 0xdc, 0x1e, 0x27, 0x3c, 0x73, 0x7f, 0x0f, 0xd6,
    0x4f, 0xde, 0xe9, 0xb4, 0xb7, 0xe3, 0xf5, 0x72, 0xce, 0x42, 0xf3, 0x91,
    0x5b, 0x84, 0xba, 0xbb, 0xae, 0xf0, 0x87, 0x0f, 0x50, 0xa4, 0x5e, 0x80,
    0x23, 0x57, 0x2b, 0xa0, 0xa3, 0xc3, 0x8a, 0x2f, 0xa8, 0x7a, 0x1a, 0x65,
    0x8f, 0x62, 0xf8, 0x3e, 0xe2, 0xcd, 0xbc, 0x63, 0x56, 0x8e, 0x77, 0xf3,
    0xf9, 0x69, 0x10, 0x57, 0xa8, 0xaf, 0x67, 0x2a, 0x9f, 0x7f, 0x7e, 0xeb,
    0x1d, 0x99, 0xa6, 0x67, 0xcd, 0x9e, 0x42, 0x2e, 0x5e, 0x4e, 0x61, 0x24,
    0xfa, 0xca, 0x2a, 0xeb, 0x62, 0x1f, 0xa3, 0x14, 0x0a, 0x06, 0x4b, 0x77,
    0x78, 0x77, 0x9b, 0xf1, 0x03, 0xcc, 0xb5, 0xfe, 0xfb, 0x7a, 0x77, 0xa6,
    0x82, 0x9f, 0xe5, 0xde, 0x9d, 0x0d, 0x4d, 0x37, 0xc6, 0x12, 0x73, 0x6d,
    0xea, 0xbb, 0x48, 0xf0, 0xd2, 0x81, 0xcc, 0x1a, 0x47, 0xfa, 0xa4, 0xd2,
    0xb2, 0x27, 0xa0, 0xfc, 0x30, 0x04, 0xdb, 0x05, 0xd3, 0x0b, 0xbc, 0x4d,
    0x7a, 0x99, 0xef, 0x7f, 0x26, 0x01, 0xd4, 0x07, 0x0b, 0x1e, 0x99, 0x06,
    0x3c, 0xde, 0x3d, 0x1c, 0x21, 0x82, 0x68, 0x46, 0x35, 0x38, 0x61, 0xea,
    0xd4, 0xc2, 0x65, 0x09, 0x39, 0x87, 0xb4, 0xd3, 0x5d, 0x3c, 0xa3, 0x79,
    0xe4, 0x01, 0x4e, 0xbf, 0x18, 0xba, 0x57, 0x3f, 0xdd, 0xea, 0x0a, 0x6b,
    0x99, 0xfb, 0x93, 0xfa, 0xab, 0xee, 0x08, 0xdf, 0x38, 0x23, 0xae, 0x8d,
    0xa8, 0x03, 0x13, 0xfe, 0x83, 0x88, 0xb0, 0xc2, 0xf9, 0x90, 0xa5, 0x1c,
    0x01, 0x6f, 0x71, 0x91, 0x42, 0x35, 0x81, 0x74, 0x71, 0x6c, 0xba, 0x86,
    0x48, 0xfe, 0x96, 0xd2, 0x88, 0x12, 0x36, 0x4e, 0xa6, 0x2f, 0xd1, 0xdb,
    0xfa, 0xbf, 0xdb, 0x84, 0x01, 0xfc, 0x7d, 0x7a, 0xac, 0x20, 0xae, 0xf5,
    0x95, 0xc9, 0xdc, 0x10, 0x5f, 0x4c, 0xae, 0x85, 0x01, 0x8b, 0xfe, 0x77,
    0x13, 0x01, 0xae, 0x39, 0x59, 0x7e, 0xbc, 0xfd, 0xc9, 0x42, 0xe4, 0x13,
    0x07, 0x3f, 0xa9, 0x74, 0xd9, 0xd5, 0xfc, 0xb9, 0x78, 0xbe, 0x97, 0xf5,
    0xe7, 0x36, 0x7f, 0xfa, 0x23, 0x30, 0xeb, 0xab, 0x92, 0xd3, 0xdc, 0x3f,
    0x7f, 0xc0, 0x77, 0x93, 0xf9, 0x88, 0xe3, 0x4e, 0x13, 0x53, 0x6d, 0x71,
    0x87, 0xe9, 0x24, 0x2b, 0xae, 0x26, 0xbf, 0x62, 0x51, 0x04, 0x42, 0xe1,
    0x13, 0x9d, 0xd8, 0x9f, 0x59, 0x87, 0x3f, 0xfc, 0x94, 0xff, 0xcf, 0x88,
    0x88, 0xe6, 0xeb, 0x6e, 0xc1, 0x96, 0x04, 0x27, 0xc8, 0xda, 0xfa, 0xe8,
    0x2e, 0xbb, 0x2c, 0x6e, 0xf4, 0xb4, 0x00, 0x7d, 0x8d, 0x3b, 0xef, 0x8b,
    0x18, 0xa9, 0x5f, 0x32, 0xa9, 0xf2, 0x3a, 0x7e, 0x65, 0x2d, 0x6e, 0x8d,
    0x75, 0x77, 0xf6, 0xa6, 0xd8, 0xf9, 0x6b, 0x51, 0xe6, 0x66, 0x52, 0x59,
    0x39, 0x97, 0x22, 0xda, 0xb2, 0xd6, 0x82, 0x5a, 0x6e, 0x61, 0x60, 0x16,
    0x48, 0x7b, 0xf1, 0xc3, 0x4d, 0x7f, 0x50, 0xfa, 0x4d, 0x58, 0x27, 0x30,
    0xc8, 0x96, 0xe0, 0x41, 0x4f, 0x6b, 0xeb, 0x88, 0xa2, 0x7a, 0xef, 0x8a,
    0x88, 0xc8, 0x50, 0x4b, 0x55, 0x66, 0xee, 0xbf, 0xc4, 0x01, 0x82, 0x4c,
    0xec, 0xde, 0x37, 0x64, 0xd6, 0x1e, 0xcf, 0x3e, 0x2e, 0xfe, 0x84, 0x68,
    0xbf, 0xa3, 0x68, 0x77, 0xa9, 0x03, 0xe4, 0xf8, 0xd7, 0xb2, 0x6e, 0xa3,
    0xc4, 0xc3, 0x36, 0x53, 0xf3, 0xdd, 0x7e, 0x4c, 0xf0, 0xe9, 0xb2, 0x44,
    0xe6, 0x60, 0x3d, 0x00, 0x9a, 0x08, 0xc3, 0x21, 0x17, 0x49, 0xda, 0x49,
    0xfb, 0x4c, 0x8b, 0xe9, 0x10, 0x66, 0xfe, 0xb7, 0xe0, 0xf9, 0xdd, 0xbf,
    0x41, 0xfe, 0x04, 0x9b, 0x7f, 0xe8, 0xd6, 0x2e, 0x4d, 0x0f, 0x7b, 0x10,
    0x73, 0x4c, 0xa1, 0x3e, 0x43, 0xb7, 0xcf, 0x94, 0x97, 0x7e, 0x24, 0xbb,
    0x87, 0xbf, 0x22, 0xb8, 0x3e, 0xeb, 0x9a, 0x3f, 0xe3, 0x86, 0xee, 0x21,
    0xbc, 0xf5, 0x44, 0xeb, 0x60, 0x2e, 0xe7, 0x8f, 0x89, 0xa4, 0x91, 0x61,
    0x28, 0x90, 0x85, 0x68, 0xe0, 0xa9, 0x62, 0x93, 0x86, 0x5a, 0x15, 0xbe,
    0xb2, 0x76, 0x83, 0xf2, 0x0f, 0x00, 0xc7, 0xb6, 0x57, 0xe9, 0x1f, 0x92,
    0x49, 0xfe, 0x50, 0x85, 0xbf, 0x39, 0x3d, 0xe4, 0x8b, 0x72, 0x2d, 0x49,
    0xbe, 0x05, 0x0a, 0x34, 0x56, 0x80, 0xc6, 0x1f, 0x46, 0x59, 0xc9, 0xfe,
    0x40, 0xfb, 0x78, 0x6d, 0x7a, 0xe5, 0x30, 0xe9, 0x81, 0x55, 0x75, 0x05,
    0x63, 0xd2, 0x22, 0xee, 0x2e, 0x6e, 0xb9, 0x18, 0xe5, 0x8a, 0x5a, 0x66,
    0xbd, 0x74, 0x30, 0xe3, 0x8b, 0x76, 0x22, 0x18, 0x1e, 0xef, 0x69, 0xe8,
    0x9d, 0x07, 0xa7, 0x9a, 0x87, 0x6c, 0x04, 0x4b, 0x74, 0x2b, 0xbe, 0x37,
    0x2f, 0x29, 0x9b, 0x60, 0x9d, 0x8b, 0x57, 0x55, 0x34, 0xca, 0x41, 0x25,
    0xae, 0x56, 0x92, 0x34, 0x1b, 0x9e, 0xbd, 0xfe, 0x74, 0xbd, 0x4e, 0x29,
    0xf0, 0x5e, 0x27, 0x94, 0xb0, 0x9e, 0x23, 0x9f, 0x4a, 0x0f, 0xa1, 0xdf,
    0xe7, 0xc4, 0xdb, 0xbe, 0x0f, 0x1a, 0x0b, 0x6c, 0xb0, 0xe1, 0x06, 0x7c,
    0x5a, 0x5b, 0x81, 0x1c, 0xb6, 0x12, 0xec, 0x6f, 0x3b, 0xbb, 0x84, 0x36,
    0xd5, 0x28, 0x16, 0xea, 0x51, 0xa8, 0x99, 0x24, 0x8f, 0xe7, 0xf8, 0xe9,
    0xce, 0xa1, 0x65, 0x96, 0x6f, 0x4e, 0x2f, 0xb7, 0x6f, 0x65, 0x39, 0xad,
    0xfd, 0x2e, 0xa0, 0x37, 0x32, 0x2f, 0xf3, 0x95, 0xa1, 0x3a, 0xa1, 0x9d,
    0x2c, 0x9e, 0xa1, 0x4b, 0x7e, 0xc9, 0x7e, 0x86, 0xaa, 0x16, 0x00, 0x82,
    0x1d, 0x36, 0xbf, 0x98, 0x0a, 0x82, 0x5b, 0xcc, 0xc4, 0x6a, 0xad, 0xa0,
    0x1f, 0x47, 0x98, 0xde, 0x8d, 0x68, 0x38, 0x3f, 0x33, 0xe2, 0x08, 0x3b,
    0x2a, 0x65, 0xd9, 0x2f, 0x53, 0x68, 0xb8, 0x78, 0xd0, 0x1d, 0xbb, 0x2a,
    0x73, 0x19, 0xba, 0x58, 0xea, 0xf1, 0x0a, 0xaa, 0xa6, 0xbe, 0x27, 0xd6,
    0x00, 0x6b, 0x4e, 0x43, 0x8e, 0x5b, 0x19, 0xc1, 0x37, 0x0f, 0xfb, 0x81,
    0x72, 0x10, 0xb6, 0x20, 0x32, 0xcd, 0xa2, 0x7c, 0x90, 0xd4, 0xf5, 0xcf,
    0x1c, 0xcb, 0x14, 0x24, 0x7a, 0x4d, 0xf5, 0xd5, 0xd9, 0xce, 0x6a, 0x64,
    0xc9, 0xd3, 0xa7, 0x36, 0x6f, 0x1d, 0xf1, 0xe9, 0x71, 0x6c, 0x3d, 0x02,
    0xa4, 0x62, 0xb1, 0x82, 0x5c, 0x13, 0x4b, 0x6b, 0x68, 0xe2, 0x31, 0xef,
    0xe4, 0x46, 0xfd, 0xe5, 0xa8, 0x29, 0xe9, 0x1e, 0xad, 0xff, 0x33, 0xdb,
    0x0b, 0xc0, 0x92, 0xb1, 0xef, 0xeb, 0xb3, 0x6f, 0x96, 0x7b, 0xdf, 0xcd,
    0x07, 0x19, 0x86, 0x60, 0x98, 0xcf, 0x95, 0xfe, 0x98, 0xdd, 0x29, 0xa6,
    0x35, 0x7b, 0x46, 0x13, 0x03, 0xa8, 0xd9, 0x7c, 0xb3, 0xdf, 0x9f, 0x14,
    0xb7, 0x34, 0x5a, 0xc4, 0x12, 0x81, 0xc5, 0x98, 0x25, 0x8d, 0x3e, 0xe3,
    0xd8, 0x2d, 0xe4, 0x54, 0xab, 0xb0, 0x13, 0xfd, 0xd1, 0x3f, 0x3b, 0xbf,
    0xa9, 0x45, 0x28, 0x8a, 0x2f, 0x9c, 0x1e, 0x2d, 0xe5, 0xab, 0x13, 0x95,
    0x97, 0xc3, 0x34, 0x37, 0x8d, 0x93, 0x66, 0x31, 0x81, 0xa0, 0x30, 0x23,
    0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x15, 0x31,
    0x16, 0x04, 0x14, 0xa5, 0x23, 0x9b, 0x7e, 0xe6, 0x45, 0x71, 0xbf, 0x48,
    0xc6, 0x27, 0x3c, 0x96, 0x87, 0x63, 0xbd, 0x1f, 0xde, 0x72, 0x12, 0x30,
    0x79, 0x06, 0x09, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x11, 0x01,
    0x31, 0x6c, 0x1e, 0x6a, 0x00, 0x4d, 0x00, 0x69, 0x00, 0x63, 0x00, 0x72,
    0x00, 0x6f, 0x00, 0x73, 0x00, 0x6f, 0x00, 0x66, 0x00, 0x74, 0x00, 0x20,
    0x00, 0x45, 0x00, 0x6e, 0x00, 0x68, 0x00, 0x61, 0x00, 0x6e, 0x00, 0x63,
    0x00, 0x65, 0x00, 0x64, 0x00, 0x20, 0x00, 0x52, 0x00, 0x53, 0x00, 0x41,
    0x00, 0x20, 0x00, 0x61, 0x00, 0x6e, 0x00, 0x64, 0x00, 0x20, 0x00, 0x41,
    0x00, 0x45, 0x00, 0x53, 0x00, 0x20, 0x00, 0x43, 0x00, 0x72, 0x00, 0x79,
    0x00, 0x70, 0x00, 0x74, 0x00, 0x6f, 0x00, 0x67, 0x00, 0x72, 0x00, 0x61,
    0x00, 0x70, 0x00, 0x68, 0x00, 0x69, 0x00, 0x63, 0x00, 0x20, 0x00, 0x50,
    0x00, 0x72, 0x00, 0x6f, 0x00, 0x76, 0x00, 0x69, 0x00, 0x64, 0x00, 0x65,
    0x00, 0x72, 0x30, 0x31, 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
    0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14, 0x93, 0xa8, 0xb2, 0x7e, 0xb7,
    0xab, 0xf1, 0x1c, 0x3c, 0x36, 0x58, 0xdc, 0x67, 0x6d, 0x42, 0xa6, 0xfc,
    0x53, 0x01, 0xe6, 0x04, 0x08, 0x77, 0x57, 0x22, 0xa1, 0x7d, 0xb9, 0xa2,
    0x69, 0x02, 0x02, 0x08, 0x00
};

static void test_communication(void)
{
    int ret;
    SOCKET sock;
    SECURITY_STATUS status;
    ULONG attrs;
    SCHANNEL_CRED cred;
    CredHandle cred_handle;
    CtxtHandle context, context2;
    SecPkgCredentials_NamesA names;
    SecPkgContext_StreamSizes sizes;
    SecPkgContext_ConnectionInfo conn_info;
    SecPkgContext_KeyInfoA key_info;
    const CERT_CONTEXT *cert;
    CRYPT_DATA_BLOB pfx;
    HCERTSTORE store;
    SecPkgContext_NegotiationInfoA info;
    SecPkgContext_CipherInfo cipher;
    SecBufferDesc buffers[2];
    SecBuffer *buf;
    unsigned buf_size = 8192;
    unsigned char *data;
    unsigned data_size;

    test_context_output_buffer_size(SP_PROT_TLS1_CLIENT, SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_MANUAL_CRED_VALIDATION,
                                    ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM);

    /* Create a socket and connect to test.winehq.org */
    if ((sock = create_ssl_socket( "test.winehq.org" )) == -1) return;

    /* Create client credentials */
    init_cred(&cred);
    cred.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
    cred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_MANUAL_CRED_VALIDATION;

    status = AcquireCredentialsHandleA(NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
        &cred, NULL, NULL, &cred_handle, NULL);
    ok(status == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", status);
    if (status != SEC_E_OK) return;

    /* Initialize the connection */
    init_buffers(&buffers[0], 4, buf_size);
    init_buffers(&buffers[1], 4, buf_size);

    buffers[0].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    status = InitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost",
        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
        0, 0, NULL, 0, &context, &buffers[0], &attrs, NULL);
    ok(status == SEC_I_CONTINUE_NEEDED, "Expected SEC_I_CONTINUE_NEEDED, got %08lx\n", status);

    buffers[1].cBuffers = 1;
    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[0].pBuffers[0].cbBuffer = 1;
    memset(buffers[1].pBuffers[0].pvBuffer, 0xfa, buf_size);
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    todo_wine
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08lx\n", status);
    todo_wine
    ok(buffers[0].pBuffers[0].cbBuffer == 0, "Output buffer size was not set to 0.\n");

    buffers[1].cBuffers = 1;
    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[0].pBuffers[0].cbBuffer = 1;
    memset(buffers[1].pBuffers[0].pvBuffer, 0, buf_size);
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08lx\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == 0, "Output buffer size was not set to 0.\n");

    buffers[0].pBuffers[0].cbBuffer = 0;
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INSUFFICIENT_MEMORY || status == SEC_E_INVALID_TOKEN,
       "Expected SEC_E_INSUFFICIENT_MEMORY or SEC_E_INVALID_TOKEN, got %08lx\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == 0, "Output buffer size was not set to 0.\n");

    status = InitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, NULL, 0, &context, NULL, &attrs, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08lx\n", status);

    buffers[0].pBuffers[0].cbBuffer = buf_size;
    status = InitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, NULL, 0, &context, &buffers[0], &attrs, NULL);
    ok(status == SEC_I_CONTINUE_NEEDED, "Expected SEC_I_CONTINUE_NEEDED, got %08lx\n", status);

    buf = &buffers[0].pBuffers[0];
    send(sock, buf->pvBuffer, buf->cbBuffer, 0);
    buf->cbBuffer = buf_size;

    context2.dwLower = context2.dwUpper = 0xdeadbeef;
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, NULL, 0, &context2, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Got unexpected status %#lx.\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == buf_size, "Output buffer size changed.\n");
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_TOKEN, "Output buffer type changed.\n");
    ok( context2.dwLower == 0xdeadbeef, "Did not expect dwLower to be set on new context\n");
    ok( context2.dwUpper == 0xdeadbeef, "Did not expect dwUpper to be set on new context\n");

    buffers[1].cBuffers = 2;
    buffers[1].pBuffers[0].cbBuffer = 0;
    buffers[1].pBuffers[1].cbBuffer = 0;
    buffers[1].pBuffers[1].BufferType = SECBUFFER_EMPTY;

    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Got unexpected status %#lx.\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == buf_size, "Output buffer size changed.\n");
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_TOKEN, "Output buffer type changed.\n");
    ok(buffers[1].pBuffers[1].cbBuffer == 5 ||
       broken(buffers[1].pBuffers[1].cbBuffer == 0), /* < win10 */ "Wrong buffer size.\n");
    ok(buffers[1].pBuffers[1].BufferType == SECBUFFER_MISSING ||
       broken(buffers[1].pBuffers[1].BufferType == SECBUFFER_EMPTY), /* < win10 */ "Wrong buffer type.\n");

    buf = &buffers[1].pBuffers[0];
    buf->cbBuffer = buf_size;
    ret = receive_data(sock, buf);
    if (ret == -1)
        return;

    buffers[1].pBuffers[0].cbBuffer = 4;
    buffers[1].pBuffers[1].cbBuffer = 0;
    buffers[1].pBuffers[1].BufferType = SECBUFFER_EMPTY;
    buffers[1].pBuffers[1].cbBuffer = 0;
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Got unexpected status %#lx.\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == buf_size, "Output buffer size changed.\n");
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_TOKEN, "Output buffer type changed.\n");
    ok(buffers[1].pBuffers[1].cbBuffer == 1 ||
       broken(buffers[1].pBuffers[1].cbBuffer == 0), /* < win10 */ "Wrong buffer size.\n");
    ok(buffers[1].pBuffers[1].BufferType == SECBUFFER_MISSING ||
       broken(buffers[1].pBuffers[1].BufferType == SECBUFFER_EMPTY), /* < win10 */ "Wrong buffer type.\n");

    context2.dwLower = context2.dwUpper = 0xdeadbeef;
    buffers[1].pBuffers[0].cbBuffer = 5;
    buffers[1].pBuffers[1].cbBuffer = 0;
    buffers[1].pBuffers[1].BufferType = SECBUFFER_EMPTY;
    buffers[1].pBuffers[1].cbBuffer = 0;
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
            0, 0, &buffers[1], 0, &context2, &buffers[0], &attrs, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Got unexpected status %#lx.\n", status);
    ok(buffers[0].pBuffers[0].cbBuffer == buf_size, "Output buffer size changed.\n");
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_TOKEN, "Output buffer type changed.\n");
    ok(buffers[1].pBuffers[1].cbBuffer > 5 ||
       broken(buffers[1].pBuffers[1].cbBuffer == 0), /* < win10 */ "Wrong buffer size\n" );
    ok(buffers[1].pBuffers[1].BufferType == SECBUFFER_MISSING ||
       broken(buffers[1].pBuffers[1].BufferType == SECBUFFER_EMPTY), /* < win10 */ "Wrong buffer type.\n");
    ok( context2.dwLower == 0xdeadbeef, "Did not expect dwLower to be set on new context\n");
    ok( context2.dwUpper == 0xdeadbeef, "Did not expect dwUpper to be set on new context\n");

    buffers[1].cBuffers = 1;
    buffers[1].pBuffers[0].cbBuffer = ret;
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM|ISC_REQ_USE_SUPPLIED_CREDS,
            0, 0, &buffers[1], 0, &context2, &buffers[0], &attrs, NULL);
    buffers[1].pBuffers[0].cbBuffer = buf_size;
    while (status == SEC_I_CONTINUE_NEEDED)
    {
        buf = &buffers[0].pBuffers[0];
        send(sock, buf->pvBuffer, buf->cbBuffer, 0);
        buf->cbBuffer = buf_size;

        ok( context.dwLower == context2.dwLower, "dwLower mismatch, expected %#Ix, got %#Ix\n",
            context.dwLower, context2.dwLower);
        ok( context.dwUpper == context2.dwUpper, "dwUpper mismatch, expected %#Ix, got %#Ix\n",
            context.dwUpper, context2.dwUpper);

        buf = &buffers[1].pBuffers[0];
        ret = receive_data(sock, buf);
        if (ret == -1)
            return;

        context2.dwLower = context2.dwUpper = 0xdeadbeef;
        buf->BufferType = SECBUFFER_TOKEN;

        buffers[1].cBuffers = 2;
        buffers[1].pBuffers[1].BufferType = SECBUFFER_EMPTY;
        buffers[1].pBuffers[1].cbBuffer = 0xdeadbeef;

        status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_USE_SUPPLIED_CREDS,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
        ok(status == SEC_E_INVALID_TOKEN, "Got unexpected status %#lx.\n", status);

        buffers[1].cBuffers = 1;
        status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_USE_SUPPLIED_CREDS,
            0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
        buffers[1].pBuffers[0].cbBuffer = buf_size;
    }

    ok(buffers[0].pBuffers[0].cbBuffer == 0, "Output buffer size was not set to 0.\n");
    ok(status == SEC_E_OK, "InitializeSecurityContext failed: %08lx\n", status);
    if(status != SEC_E_OK) {
        skip("Handshake failed\n");
        return;
    }
    ok(attrs == (ISC_RET_REPLAY_DETECT | ISC_RET_SEQUENCE_DETECT | ISC_RET_CONFIDENTIALITY |
                 ISC_RET_STREAM | ISC_RET_USED_SUPPLIED_CREDS), "got %08lx\n", attrs);

    status = QueryCredentialsAttributesA(&cred_handle, SECPKG_CRED_ATTR_NAMES, &names);
    ok(status == SEC_E_NO_CREDENTIALS, "expected SEC_E_NO_CREDENTIALS, got %08lx\n", status);

    status = QueryContextAttributesA(&context, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&cert);
    ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_REMOTE_CERT_CONTEXT) failed: %08lx\n", status);
    if(status == SEC_E_OK) {
        SecPkgContext_Bindings bindings = {0xdeadbeef, (void*)0xdeadbeef};

        test_remote_cert(cert);

        status = QueryContextAttributesA(&context, SECPKG_ATTR_ENDPOINT_BINDINGS, &bindings);
        ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_ENDPOINT_BINDINGS) failed: %08lx\n", status);
        if (status == SEC_E_OK)
        {
            static const char prefix[] = "tls-server-end-point:";
            const char *p;
            BYTE hash[64];
            DWORD hash_size;

            ok(bindings.BindingsLength == sizeof(*bindings.Bindings) + sizeof(prefix)-1 + 32 /* hash size */,
               "bindings.BindingsLength = %lu\n", bindings.BindingsLength);
            ok(!bindings.Bindings->dwInitiatorAddrType, "dwInitiatorAddrType = %lx\n", bindings.Bindings->dwInitiatorAddrType);
            ok(!bindings.Bindings->cbInitiatorLength, "cbInitiatorLength = %lx\n", bindings.Bindings->cbInitiatorLength);
            ok(!bindings.Bindings->dwInitiatorOffset, "dwInitiatorOffset = %lx\n", bindings.Bindings->dwInitiatorOffset);
            ok(!bindings.Bindings->dwAcceptorAddrType, "dwAcceptorAddrType = %lx\n", bindings.Bindings->dwAcceptorAddrType);
            ok(!bindings.Bindings->cbAcceptorLength, "cbAcceptorLength = %lx\n", bindings.Bindings->cbAcceptorLength);
            ok(!bindings.Bindings->dwAcceptorOffset, "dwAcceptorOffset = %lx\n", bindings.Bindings->dwAcceptorOffset);
            ok(sizeof(*bindings.Bindings) + bindings.Bindings->cbApplicationDataLength == bindings.BindingsLength,
               "cbApplicationDataLength = %lx\n", bindings.Bindings->cbApplicationDataLength);
            ok(bindings.Bindings->dwApplicationDataOffset == sizeof(*bindings.Bindings),
               "dwApplicationDataOffset = %lx\n", bindings.Bindings->dwApplicationDataOffset);
            p = (const char*)(bindings.Bindings+1);
            ok(!memcmp(p, prefix, sizeof(prefix)-1), "missing prefix\n");
            p += sizeof(prefix)-1;

            hash_size = sizeof(hash);
            ret = CryptHashCertificate(0, CALG_SHA_256, 0, cert->pbCertEncoded, cert->cbCertEncoded, hash, &hash_size);
            ok(ret, "got %lu\n", GetLastError());
            ok(hash_size == 32, "hash_size = %lu\n", hash_size);
            ok(!memcmp(hash, p, hash_size), "unexpected hash part\n");
            FreeContextBuffer(bindings.Bindings);
        }

        status = QueryContextAttributesA(&context, SECPKG_ATTR_UNIQUE_BINDINGS, &bindings);
        ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_UNIQUE_BINDINGS) failed: %08lx\n", status);
        if (status == SEC_E_OK)
        {
            const char *p;
            static const char prefix[] = "tls-unique:";

            ok(bindings.BindingsLength > sizeof(*bindings.Bindings) + sizeof(prefix)-1,
               "bindings.BindingsLength = %lu\n", bindings.BindingsLength);
            ok(!bindings.Bindings->dwInitiatorAddrType, "dwInitiatorAddrType = %lx\n", bindings.Bindings->dwInitiatorAddrType);
            ok(!bindings.Bindings->cbInitiatorLength, "cbInitiatorLength = %lx\n", bindings.Bindings->cbInitiatorLength);
            ok(!bindings.Bindings->dwInitiatorOffset, "dwInitiatorOffset = %lx\n", bindings.Bindings->dwInitiatorOffset);
            ok(!bindings.Bindings->dwAcceptorAddrType, "dwAcceptorAddrType = %lx\n", bindings.Bindings->dwAcceptorAddrType);
            ok(!bindings.Bindings->cbAcceptorLength, "cbAcceptorLength = %lx\n", bindings.Bindings->cbAcceptorLength);
            ok(!bindings.Bindings->dwAcceptorOffset, "dwAcceptorOffset = %lx\n", bindings.Bindings->dwAcceptorOffset);
            ok(sizeof(*bindings.Bindings) + bindings.Bindings->cbApplicationDataLength == bindings.BindingsLength,
               "cbApplicationDataLength = %lx\n", bindings.Bindings->cbApplicationDataLength);
            ok(bindings.Bindings->dwApplicationDataOffset == sizeof(*bindings.Bindings),
               "dwApplicationDataOffset = %lx\n", bindings.Bindings->dwApplicationDataOffset);
            p = (const char*)(bindings.Bindings+1);
            ok(!memcmp(p, prefix, sizeof(prefix)-1), "wrong prefix\n");
            FreeContextBuffer(bindings.Bindings);
        }
        CertFreeCertificateContext(cert);
    }

    status = QueryContextAttributesA(&context, SECPKG_ATTR_CONNECTION_INFO, (void*)&conn_info);
    ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_CONNECTION_INFO) failed: %08lx\n", status);
    if(status == SEC_E_OK) {
        ok(conn_info.dwCipherStrength >= 128, "conn_info.dwCipherStrength = %ld\n", conn_info.dwCipherStrength);
        ok(!conn_info.dwHashStrength, "conn_info.dwHashStrength = %ld\n", conn_info.dwHashStrength);
    }

    memset(&cipher, 0, sizeof(cipher));
    cipher.dwVersion = SECPKGCONTEXT_CIPHERINFO_V1;
    status = QueryContextAttributesA(&context, SECPKG_ATTR_CIPHER_INFO, &cipher);
    ok(status == SEC_E_OK, "got %08lx\n", status);
    if (status == SEC_E_OK)
    {
        ok(cipher.dwProtocol == 0x303, "got %lx\n", cipher.dwProtocol);
        todo_wine ok(cipher.dwCipherSuite == 0xc030, "got %lx\n", cipher.dwCipherSuite);
        todo_wine ok(cipher.dwBaseCipherSuite == 0xc030, "got %lx\n", cipher.dwBaseCipherSuite);
        ok(!wcscmp(cipher.szCipherSuite, L"TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384"),
           "got %s\n", wine_dbgstr_w(cipher.szCipherSuite));
        ok(!wcscmp(cipher.szCipher, L"AES"), "got %s\n", wine_dbgstr_w(cipher.szCipher));
        ok(cipher.dwCipherLen == 256, "got %lu\n", cipher.dwCipherLen);
        ok(cipher.dwCipherBlockLen == 16, "got %lu\n", cipher.dwCipherBlockLen);
        ok(!cipher.szHash[0], "got %s\n", wine_dbgstr_w(cipher.szHash));
        ok(!cipher.dwHashLen, "got %lu\n", cipher.dwHashLen);
        ok(!wcscmp(cipher.szExchange, L"ECDH") || !wcscmp(cipher.szExchange, L"ECDH_P256"), /* < win10 */
           "got %s\n", wine_dbgstr_w(cipher.szExchange));
        ok(cipher.dwMinExchangeLen == 0 || cipher.dwMinExchangeLen == 256,  /* < win10 */
           "got %lu\n", cipher.dwMinExchangeLen);
        ok(cipher.dwMaxExchangeLen == 65536 || cipher.dwMaxExchangeLen == 256, /* < win10 */
           "got %lu\n", cipher.dwMaxExchangeLen);
        ok(!wcscmp(cipher.szCertificate, L"RSA"), "got %s\n", wine_dbgstr_w(cipher.szCertificate));
        todo_wine ok(cipher.dwKeyType == 0x1d || cipher.dwKeyType == 0x17, /* < win10 */
                     "got %#lx\n", cipher.dwKeyType);
    }

    status = QueryContextAttributesA(&context, SECPKG_ATTR_KEY_INFO, &key_info);
    ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_KEY_INFO) failed: %08lx\n", status);
    if(status == SEC_E_OK) {
        ok(key_info.SignatureAlgorithm == CALG_RSA_SIGN,
           "key_info.SignatureAlgorithm = %04lx\n", key_info.SignatureAlgorithm);
        ok(!strcmp(key_info.sSignatureAlgorithmName, "RSA"),
           "key_info.sSignatureAlgorithmName = %s\n", key_info.sSignatureAlgorithmName);
        ok(key_info.KeySize >= 128, "key_info.KeySize = %ld\n", key_info.KeySize);
    }

    status = QueryContextAttributesA(&context, SECPKG_ATTR_STREAM_SIZES, &sizes);
    ok(status == SEC_E_OK, "QueryContextAttributesW(SECPKG_ATTR_STREAM_SIZES) failed: %08lx\n", status);

    status = QueryContextAttributesA(&context, SECPKG_ATTR_NEGOTIATION_INFO, &info);
    ok(status == SEC_E_UNSUPPORTED_FUNCTION, "QueryContextAttributesA returned %08lx\n", status);

    reset_buffers(&buffers[0]);

    /* Send a simple request so we get data for testing DecryptMessage */
    buf = &buffers[0].pBuffers[0];
    data = buf->pvBuffer;
    buf->BufferType = SECBUFFER_STREAM_HEADER;
    buf->cbBuffer = sizes.cbHeader;
    ++buf;
    buf->BufferType = SECBUFFER_DATA;
    buf->pvBuffer = data + sizes.cbHeader;
    buf->cbBuffer = sizeof(http_request) - 1;
    memcpy(buf->pvBuffer, http_request, sizeof(http_request) - 1);
    ++buf;
    buf->BufferType = SECBUFFER_STREAM_TRAILER;
    buf->pvBuffer = data + sizes.cbHeader + sizeof(http_request) -1;
    buf->cbBuffer = sizes.cbTrailer;

    status = EncryptMessage(&context, 0, &buffers[0], 0);
    ok(status == SEC_E_OK, "EncryptMessage failed: %08lx\n", status);
    if (status != SEC_E_OK)
        return;

    buf = &buffers[0].pBuffers[0];
    send(sock, buf->pvBuffer,
         buffers[0].pBuffers[0].cbBuffer + buffers[0].pBuffers[1].cbBuffer + buffers[0].pBuffers[2].cbBuffer, 0);

    reset_buffers(&buffers[0]);
    buf->cbBuffer = buf_size;
    data_size = receive_data(sock, buf);

    /* Too few buffers */
    --buffers[0].cBuffers;
    status = DecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08lx\n", status);

    /* No data buffer */
    ++buffers[0].cBuffers;
    status = DecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08lx\n", status);

    /* Two data buffers */
    buffers[0].pBuffers[0].BufferType = SECBUFFER_DATA;
    buffers[0].pBuffers[1].BufferType = SECBUFFER_DATA;
    status = DecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08lx\n", status);

    /* Too few empty buffers */
    buffers[0].pBuffers[1].BufferType = SECBUFFER_EXTRA;
    status = DecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INVALID_TOKEN, "Expected SEC_E_INVALID_TOKEN, got %08lx\n", status);

    /* Incomplete data */
    buffers[0].pBuffers[1].BufferType = SECBUFFER_EMPTY;
    buffers[0].pBuffers[0].cbBuffer = (data[3]<<8) | data[4];
    status = DecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_E_INCOMPLETE_MESSAGE, "Expected SEC_E_INCOMPLETE_MESSAGE, got %08lx\n", status);
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_MISSING, "Expected first buffer to be SECBUFFER_MISSING\n");
    ok(buffers[0].pBuffers[0].cbBuffer == 5, "Expected first buffer to be a five bytes\n");

    /* Renegotiate */
    buffers[0].pBuffers[0].BufferType = SECBUFFER_DATA;
    buffers[0].pBuffers[0].cbBuffer = data_size;
    buffers[0].pBuffers[1].BufferType = SECBUFFER_EMPTY;
    buffers[0].pBuffers[1].cbBuffer = 0;
    buffers[0].pBuffers[2].BufferType = SECBUFFER_EMPTY;
    buffers[0].pBuffers[2].cbBuffer = 0;
    status = DecryptMessage(&context, &buffers[0], 0, NULL);
    ok(status == SEC_I_RENEGOTIATE, "Expected SEC_I_RENEGOTIATE, got %08lx\n", status);
    ok(buffers[0].pBuffers[0].BufferType == SECBUFFER_STREAM_HEADER, "got %lu\n", buffers[0].pBuffers[0].BufferType);
    todo_wine ok(buffers[0].pBuffers[0].cbBuffer == 13, "got %lu\n", buffers[0].pBuffers[0].cbBuffer);
    ok(buffers[0].pBuffers[1].BufferType == SECBUFFER_DATA, "got %lu\n", buffers[0].pBuffers[1].BufferType);
    ok(buffers[0].pBuffers[1].cbBuffer == 0, "got %lu\n", buffers[0].pBuffers[1].cbBuffer);
    ok(buffers[0].pBuffers[2].BufferType == SECBUFFER_STREAM_TRAILER, "got %lu\n", buffers[0].pBuffers[2].BufferType);
    todo_wine ok(buffers[0].pBuffers[2].cbBuffer == 20, "got %lu\n", buffers[0].pBuffers[2].cbBuffer);

    pfx.pbData = (BYTE *)pfxdata;
    pfx.cbData = sizeof(pfxdata);
    store = PFXImportCertStore(&pfx, NULL, CRYPT_EXPORTABLE|CRYPT_USER_KEYSET|PKCS12_NO_PERSIST_KEY);
    ok(store != NULL, "got %lu\n", GetLastError());

    cert = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0, CERT_FIND_ANY, NULL, NULL);
    ok(cert != NULL, "got %lu\n", GetLastError());

    cred.paCred = &cert;
    cred.cCreds = 1;

    FreeCredentialsHandle(&cred_handle);
    status = AcquireCredentialsHandleA(NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
        &cred, NULL, NULL, &cred_handle, NULL);
    ok(status == SEC_E_OK, "AcquireCredentialsHandleA failed: %08lx\n", status);

    init_buffers(&buffers[0], 4, buf_size);
    init_buffers(&buffers[1], 4, buf_size);

    buffers[0].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM,
        0, 0, NULL, 0, &context, &buffers[0], &attrs, NULL);
    todo_wine ok(status == SEC_I_CONTINUE_NEEDED, "Expected SEC_I_CONTINUE_NEEDED, got %08lx\n", status);
    if (status != SEC_I_CONTINUE_NEEDED)
    {
        skip("skipping remaining renegotiate test\n");
        goto done;
    }

    buf = &buffers[0].pBuffers[0];
    send(sock, buf->pvBuffer, buf->cbBuffer, 0);
    buf->cbBuffer = buf_size;

    buf = &buffers[1].pBuffers[0];
    buf->cbBuffer = buf_size;
    ret = receive_data(sock, buf);
    if (ret == -1)
        return;

    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM|ISC_REQ_USE_SUPPLIED_CREDS, 0, 0, &buffers[1], 0, &context2,
        &buffers[0], &attrs, NULL);
    buffers[1].pBuffers[0].cbBuffer = buf_size;
    while (status == SEC_I_CONTINUE_NEEDED)
    {
        buf = &buffers[0].pBuffers[0];
        send(sock, buf->pvBuffer, buf->cbBuffer, 0);
        buf->cbBuffer = buf_size;

        todo_wine ok( context.dwLower == context2.dwLower, "dwLower mismatch, expected %#Ix, got %#Ix\n",
                      context.dwLower, context2.dwLower);
        todo_wine ok( context.dwUpper == context2.dwUpper, "dwUpper mismatch, expected %#Ix, got %#Ix\n",
                      context.dwUpper, context2.dwUpper);

        buf = &buffers[1].pBuffers[0];
        ret = receive_data(sock, buf);
        if (ret == -1)
            return;

        context2.dwLower = context2.dwUpper = 0xdeadbeef;
        buf->BufferType = SECBUFFER_TOKEN;
        status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_USE_SUPPLIED_CREDS, 0, 0, &buffers[1], 0, &context2, &buffers[0], &attrs, NULL);
        buffers[1].pBuffers[0].cbBuffer = buf_size;
    }
    ok (status == SEC_E_ALGORITHM_MISMATCH, "got %08lx\n", status);

done:
    DeleteSecurityContext(&context);

    status = QueryContextAttributesW(&context, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&cert);
    ok(status == SEC_E_INVALID_HANDLE, "QueryContextAttributesW(SECPKG_ATTR_REMOTE_CERT_CONTEXT) got %08lx\n", status);

    FreeCredentialsHandle(&cred_handle);

    CertFreeCertificateContext(cert);
    CertCloseStore(store, 0);

    free_buffers(&buffers[0]);
    free_buffers(&buffers[1]);

    closesocket(sock);
}

static void test_application_protocol_negotiation(void)
{
    int ret;
    SOCKET sock;
    SECURITY_STATUS status;
    ULONG attrs;
    SCHANNEL_CRED cred;
    CredHandle cred_handle;
    CtxtHandle context, context2;
    SecPkgContext_ApplicationProtocol protocol;
    SecBufferDesc buffers[3];
    SecBuffer *buf;
    unsigned buf_size = 8192;
    unsigned char *alpn_buffer;
    unsigned int *extension_len;
    unsigned short *list_len;
    int list_start_index, offset = 0;

    if ((sock = create_ssl_socket( "test.winehq.org" )) == -1) return;

    init_cred(&cred);
    cred.grbitEnabledProtocols = SP_PROT_TLS1_2_CLIENT;
    cred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_MANUAL_CRED_VALIDATION;

    status = AcquireCredentialsHandleA(NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
        &cred, NULL, NULL, &cred_handle, NULL);
    ok(status == SEC_E_OK, "got %08lx\n", status);
    if (status != SEC_E_OK) return;

    init_buffers(&buffers[0], 4, buf_size);
    init_buffers(&buffers[1], 4, buf_size);
    init_buffers(&buffers[2], 1, 128);

    alpn_buffer = buffers[2].pBuffers[0].pvBuffer;
    extension_len = (unsigned int *)&alpn_buffer[offset];
    offset += sizeof(*extension_len);
    *(unsigned int *)&alpn_buffer[offset] = SecApplicationProtocolNegotiationExt_ALPN;
    offset += sizeof(unsigned int);
    list_len = (unsigned short *)&alpn_buffer[offset];
    offset += sizeof(*list_len);
    list_start_index = offset;

    alpn_buffer[offset++] = sizeof("http/1.1") - 1;
    memcpy(&alpn_buffer[offset], "http/1.1", sizeof("http/1.1") - 1);
    offset += sizeof("http/1.1") - 1;
    alpn_buffer[offset++] = sizeof("h2") - 1;
    memcpy(&alpn_buffer[offset], "h2", sizeof("h2") - 1);
    offset += sizeof("h2") - 1;

    *list_len = offset - list_start_index;
    *extension_len = *list_len + sizeof(*extension_len) + sizeof(*list_len);

    buffers[2].pBuffers[0].BufferType = SECBUFFER_APPLICATION_PROTOCOLS;
    buffers[2].pBuffers[0].cbBuffer = offset;

    buffers[0].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    status = InitializeSecurityContextA(&cred_handle, NULL, (SEC_CHAR *)"localhost",
        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM, 0, 0, &buffers[2], 0, &context, &buffers[0], &attrs, NULL);
    ok(status == SEC_I_CONTINUE_NEEDED, "got %08lx\n", status);

    buf = &buffers[0].pBuffers[0];
    send(sock, buf->pvBuffer, buf->cbBuffer, 0);
    buf->cbBuffer = buf_size;

    buf = &buffers[1].pBuffers[0];
    buf->cbBuffer = buf_size;
    ret = receive_data(sock, buf);
    if (ret == -1)
        return;

    context2.dwLower = context2.dwUpper = 0xdeadbeef;
    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM|ISC_REQ_USE_SUPPLIED_CREDS, 0, 0, &buffers[1], 0, &context2,
        &buffers[0], &attrs, NULL);
    buffers[1].pBuffers[0].cbBuffer = buf_size;
    while (status == SEC_I_CONTINUE_NEEDED)
    {
        buf = &buffers[0].pBuffers[0];
        send(sock, buf->pvBuffer, buf->cbBuffer, 0);
        buf->cbBuffer = buf_size;

        ok( context.dwLower == context2.dwLower, "dwLower mismatch, expected %#Ix, got %#Ix\n",
            context.dwLower, context2.dwLower);
        ok( context.dwUpper == context2.dwUpper, "dwUpper mismatch, expected %#Ix, got %#Ix\n",
            context.dwUpper, context2.dwUpper);

        buf = &buffers[1].pBuffers[0];
        ret = receive_data(sock, buf);
        if (ret == -1)
            return;

        context2.dwLower = context2.dwUpper = 0xdeadbeef;
        buf->BufferType = SECBUFFER_TOKEN;
        status = InitializeSecurityContextA(&cred_handle, &context, (SEC_CHAR *)"localhost",
            ISC_REQ_USE_SUPPLIED_CREDS, 0, 0, &buffers[1], 0, NULL, &buffers[0], &attrs, NULL);
        buffers[1].pBuffers[0].cbBuffer = buf_size;
    }

    ok (status == SEC_E_OK, "got %08lx\n", status);
    if (status != SEC_E_OK)
    {
        skip("Handshake failed\n");
        return;
    }

    memset(&protocol, 0, sizeof(protocol));
    status = QueryContextAttributesA(&context, SECPKG_ATTR_APPLICATION_PROTOCOL, &protocol);
    ok(status == SEC_E_OK || broken(status == SEC_E_UNSUPPORTED_FUNCTION) /* < win8 */, "got %08lx\n", status);
    if (status == SEC_E_OK)
    {
        ok(protocol.ProtoNegoStatus == SecApplicationProtocolNegotiationStatus_Success, "got %u\n", protocol.ProtoNegoStatus);
        ok(protocol.ProtoNegoExt == SecApplicationProtocolNegotiationExt_ALPN, "got %u\n", protocol.ProtoNegoExt);
        ok(protocol.ProtocolIdSize == 8, "got %u\n", protocol.ProtocolIdSize);
        ok(!memcmp(protocol.ProtocolId, "http/1.1", 8), "wrong protocol id\n");
    }

    DeleteSecurityContext(&context);
    FreeCredentialsHandle(&cred_handle);

    free_buffers(&buffers[0]);
    free_buffers(&buffers[1]);
    free_buffers(&buffers[2]);

    closesocket(sock);
}

static void test_server_protocol_negotiation(void) {
    BOOL ret;
    SECURITY_STATUS status;
    ULONG attrs;
    SCHANNEL_CRED client_cred, server_cred;
    CredHandle client_cred_handle, server_cred_handle;
    CtxtHandle client_context, server_context, client_context2, server_context2;
    SecPkgContext_ApplicationProtocol protocol;
    SecBufferDesc buffers[3];
    PCCERT_CONTEXT cert;
    HCRYPTPROV csp;
    HCRYPTKEY key;
    CRYPT_KEY_PROV_INFO keyProvInfo;
    WCHAR ms_def_prov_w[MAX_PATH];
    unsigned buf_size = 8192;
    unsigned char *alpn_buffer;
    unsigned int *extension_len;
    unsigned short *list_len;
    int list_start_index, offset = 0;

    lstrcpyW(ms_def_prov_w, MS_DEF_PROV_W);
    keyProvInfo.pwszContainerName = cspNameW;
    keyProvInfo.pwszProvName = ms_def_prov_w;
    keyProvInfo.dwProvType = PROV_RSA_FULL;
    keyProvInfo.dwFlags = 0;
    keyProvInfo.cProvParam = 0;
    keyProvInfo.rgProvParam = NULL;
    keyProvInfo.dwKeySpec = AT_SIGNATURE;

    cert = CertCreateCertificateContext(X509_ASN_ENCODING, selfSignedCert, sizeof(selfSignedCert));
    ret = CertSetCertificateContextProperty(cert, CERT_KEY_PROV_INFO_PROP_ID, 0, &keyProvInfo);
    ok(ret, "CertSetCertificateContextProperty failed: %08lx", GetLastError());
    ret = CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL, CRYPT_NEWKEYSET);
    ok(ret, "CryptAcquireContextW failed: %08lx\n", GetLastError());
    ret = CryptImportKey(csp, privKey, sizeof(privKey), 0, 0, &key);
    ok(ret, "CryptImportKey failed: %08lx\n", GetLastError());
    if (!ret) return;

    init_cred(&client_cred);
    init_cred(&server_cred);
    client_cred.grbitEnabledProtocols = SP_PROT_TLS1_CLIENT;
    client_cred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_MANUAL_CRED_VALIDATION;
    server_cred.grbitEnabledProtocols = SP_PROT_TLS1_SERVER;
    server_cred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_MANUAL_CRED_VALIDATION;
    server_cred.cCreds = 1;
    server_cred.paCred = &cert;

    status = AcquireCredentialsHandleA(NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL, &client_cred,
                                       NULL, NULL, &client_cred_handle, NULL);
    ok(status == SEC_E_OK, "got %08lx\n", status);
    if (status != SEC_E_OK) return;
    status = AcquireCredentialsHandleA(NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_INBOUND,  NULL, &server_cred,
                                       NULL, NULL, &server_cred_handle, NULL);
    ok(status == SEC_E_OK, "got %08lx\n", status);
    if (status != SEC_E_OK) return;

    init_buffers(&buffers[0], 4, buf_size);
    init_buffers(&buffers[1], 4, buf_size);
    init_buffers(&buffers[2], 1, 128);

    alpn_buffer = buffers[2].pBuffers[0].pvBuffer;
    extension_len = (unsigned int *)&alpn_buffer[offset];
    offset += sizeof(*extension_len);
    *(unsigned int *)&alpn_buffer[offset] = SecApplicationProtocolNegotiationExt_ALPN;
    offset += sizeof(unsigned int);
    list_len = (unsigned short *)&alpn_buffer[offset];
    offset += sizeof(*list_len);
    list_start_index = offset;

    alpn_buffer[offset++] = sizeof("http/1.1") - 1;
    memcpy(&alpn_buffer[offset], "http/1.1", sizeof("http/1.1") - 1);
    offset += sizeof("http/1.1") - 1;
    alpn_buffer[offset++] = sizeof("h2") - 1;
    memcpy(&alpn_buffer[offset], "h2", sizeof("h2") - 1);
    offset += sizeof("h2") - 1;

    *list_len = offset - list_start_index;
    *extension_len = *list_len + sizeof(*extension_len) + sizeof(*list_len);

    buffers[2].pBuffers[0].BufferType = SECBUFFER_APPLICATION_PROTOCOLS;
    buffers[2].pBuffers[0].cbBuffer = offset;
    buffers[0].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    status = InitializeSecurityContextA(&client_cred_handle, NULL, (SEC_CHAR *)"localhost",
                                        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM, 0, 0, &buffers[2], 0,
                                        &client_context, &buffers[0], &attrs, NULL);
    ok(status == SEC_I_CONTINUE_NEEDED, "got %08lx\n", status);

    buffers[1].pBuffers[0].cbBuffer = buf_size;
    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[0].pBuffers[1] = buffers[2].pBuffers[0];
    status = AcceptSecurityContext(&server_cred_handle, NULL, &buffers[0], ASC_REQ_CONFIDENTIALITY|ASC_REQ_STREAM,
                                   0, &server_context, &buffers[1], &attrs, NULL);
    ok(status == SEC_I_CONTINUE_NEEDED, "got %08lx\n", status);
    memset(&buffers[0].pBuffers[1], 0, sizeof(buffers[0].pBuffers[1]));

    client_context2.dwLower = client_context2.dwUpper = 0xdeadbeef;
    buffers[0].pBuffers[0].cbBuffer = buf_size;
    status = InitializeSecurityContextA(&client_cred_handle, &client_context, (SEC_CHAR *)"localhost",
                                        ISC_REQ_CONFIDENTIALITY|ISC_REQ_STREAM|ISC_REQ_USE_SUPPLIED_CREDS, 0, 0,
                                        &buffers[1], 0, &client_context2, &buffers[0], &attrs, NULL);
    ok(client_context.dwLower == client_context2.dwLower, "dwLower mismatch, expected %#Ix, got %#Ix\n",
       client_context.dwLower, client_context2.dwLower);
    ok(client_context.dwUpper == client_context2.dwUpper, "dwUpper mismatch, expected %#Ix, got %#Ix\n",
       client_context.dwUpper, client_context2.dwUpper);
    ok(status == SEC_I_CONTINUE_NEEDED, "got %08lx\n", status);

    server_context2.dwLower = server_context2.dwUpper = 0xdeadbeef;
    buffers[1].pBuffers[0].cbBuffer = buf_size;
    status = AcceptSecurityContext(&server_cred_handle, &server_context, &buffers[0],
                                   ASC_REQ_CONFIDENTIALITY|ASC_REQ_STREAM, 0, &server_context2, &buffers[1],
                                   &attrs, NULL);
    ok(server_context.dwLower == server_context2.dwLower, "dwLower mismatch, expected %#Ix, got %#Ix\n",
       server_context.dwLower, server_context2.dwLower);
    ok(server_context.dwUpper == server_context2.dwUpper, "dwUpper mismatch, expected %#Ix, got %#Ix\n",
       server_context.dwUpper, server_context2.dwUpper);
    ok(status == SEC_E_OK, "got %08lx\n", status);

    buffers[0].pBuffers[0].cbBuffer = buf_size;
    status = InitializeSecurityContextA(&client_cred_handle, &client_context, (SEC_CHAR *)"localhost",
                                        ISC_REQ_USE_SUPPLIED_CREDS, 0, 0, &buffers[1], 0, NULL, &buffers[0],
                                        &attrs, NULL);
    ok(status == SEC_E_OK, "got %08lx\n", status);

    memset(&protocol, 0, sizeof(protocol));
    status = QueryContextAttributesA(&client_context, SECPKG_ATTR_APPLICATION_PROTOCOL, &protocol);
    ok(status == SEC_E_OK || broken(status == SEC_E_UNSUPPORTED_FUNCTION) /* < win8 */, "got %08lx\n", status);
    if (status == SEC_E_OK)
    {
        ok(protocol.ProtoNegoStatus == SecApplicationProtocolNegotiationStatus_Success, "got %u\n",
           protocol.ProtoNegoStatus);
        ok(protocol.ProtoNegoExt == SecApplicationProtocolNegotiationExt_ALPN, "got %u\n", protocol.ProtoNegoExt);
        ok(protocol.ProtocolIdSize == 8, "got %u\n", protocol.ProtocolIdSize);
        ok(!memcmp(protocol.ProtocolId, "http/1.1", 8), "wrong protocol id\n");
    }

    DeleteSecurityContext(&client_context);
    DeleteSecurityContext(&server_context);
    FreeCredentialsHandle(&client_cred_handle);
    FreeCredentialsHandle(&server_cred_handle);

    free_buffers(&buffers[0]);
    free_buffers(&buffers[1]);
    free_buffers(&buffers[2]);

    CryptDestroyKey(key);
    CryptReleaseContext(csp, 0);
    CryptAcquireContextW(&csp, cspNameW, MS_DEF_PROV_W, PROV_RSA_FULL, CRYPT_DELETEKEYSET);
    CertFreeCertificateContext(cert);
}

static void init_dtls_output_buffer(SecBufferDesc *buffer)
{
    buffer->pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buffer->pBuffers[0].cbBuffer = 1420;
    buffer->pBuffers[1].BufferType = SECBUFFER_ALERT;
    buffer->pBuffers[1].cbBuffer = 1024;
    if (!buffer->pBuffers[1].pvBuffer)
        buffer->pBuffers[1].pvBuffer = HeapAlloc( GetProcessHeap(), 0, 1024 );
}

static void test_dtls(void)
{
    SECURITY_STATUS status;
    TimeStamp exp;
    SCHANNEL_CRED cred;
    CredHandle cred_handle, cred_handle2;
    CtxtHandle ctx_handle, ctx_handle2;
    SecBufferDesc buffers[3];
    ULONG flags_req, flags_ret, attr, prev_buf_len;
    char *buf, *buf2;

    init_cred( &cred );
    cred.grbitEnabledProtocols = SP_PROT_DTLS_CLIENT | SP_PROT_DTLS1_2_CLIENT;
    cred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS;

    status = AcquireCredentialsHandleA( NULL, unisp_name_a, SECPKG_CRED_OUTBOUND, NULL, &cred, NULL, NULL,
                                        &cred_handle, &exp );
    if (status == SEC_E_ALGORITHM_MISMATCH)
    {
        win_skip( "no DTLS support\n" );
        return;
    }
    ok( status == SEC_E_OK, "got %08lx\n", status );

    /* Should fail if both DTLS and TLS protocols are requested */
    cred.grbitEnabledProtocols |= SP_PROT_TLS1_CLIENT;
    status = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND, NULL, &cred, NULL, NULL,
                                       &cred_handle2, &exp);
    ok(status == SEC_E_ALGORITHM_MISMATCH, "status = %08lx\n", status);

    cred.grbitEnabledProtocols = SP_PROT_DTLS1_X_CLIENT | SP_PROT_TLS1_SERVER;
    status = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND, NULL, &cred, NULL, NULL,
                                       &cred_handle2, &exp);
    ok(status == SEC_E_ALGORITHM_MISMATCH, "status = got %08lx\n", status);

    cred.grbitEnabledProtocols = SP_PROT_DTLS1_X_CLIENT | SP_PROT_SSL3_SERVER;
    status = AcquireCredentialsHandleA(NULL, unisp_name_a, SECPKG_CRED_OUTBOUND, NULL, &cred, NULL, NULL,
                                       &cred_handle2, &exp);
    ok(status == SEC_E_ALGORITHM_MISMATCH, "status = got %08lx\n", status);

    flags_req = ISC_REQ_MANUAL_CRED_VALIDATION | ISC_REQ_EXTENDED_ERROR | ISC_REQ_DATAGRAM |
                ISC_REQ_USE_SUPPLIED_CREDS | ISC_REQ_CONFIDENTIALITY | ISC_REQ_SEQUENCE_DETECT |
                ISC_REQ_REPLAY_DETECT;
    test_context_output_buffer_size(SP_PROT_DTLS_CLIENT | SP_PROT_DTLS1_2_CLIENT, SCH_CRED_NO_DEFAULT_CREDS,
                                    flags_req);

    init_buffers( &buffers[0], 1, 128 );
    buffers[0].pBuffers[0].BufferType = SECBUFFER_DTLS_MTU;
    *(WORD *)(buffers[0].pBuffers[0].pvBuffer) = 1024;
    buffers[0].pBuffers[0].cbBuffer = 2;

    init_buffers( &buffers[1], 2, 2048 );
    init_dtls_output_buffer(&buffers[1]);

    attr = 0;
    exp.LowPart = exp.HighPart = 0xdeadbeef;
    status = InitializeSecurityContextA( &cred_handle, NULL, (SEC_CHAR *)"winetest", flags_req, 0, 16, &buffers[0], 0,
                                         &ctx_handle, &buffers[1], &attr, &exp );
    ok( status == SEC_I_CONTINUE_NEEDED, "got %08lx\n", status );

    flags_ret = ISC_RET_MANUAL_CRED_VALIDATION | ISC_RET_STREAM | ISC_RET_EXTENDED_ERROR |
                ISC_RET_DATAGRAM | ISC_RET_USED_SUPPLIED_CREDS | ISC_RET_CONFIDENTIALITY |
                ISC_RET_SEQUENCE_DETECT | ISC_RET_REPLAY_DETECT;
    ok( attr == flags_ret, "got %08lx\n", attr );
    ok( !exp.LowPart, "got %08lx\n", exp.LowPart );
    ok( !exp.HighPart, "got %08lx\n", exp.HighPart );
    ok( buffers[1].pBuffers[1].BufferType == SECBUFFER_ALERT, "Expected buffertype SECBUFFER_ALERT, got %#lx\n",
        buffers[1].pBuffers[1].BufferType);
    ok( !buffers[1].pBuffers[1].cbBuffer, "Expected SECBUFFER_ALERT buffer to be empty, got %#lx\n",
        buffers[1].pBuffers[1].cbBuffer);
    prev_buf_len = buffers[1].pBuffers[0].cbBuffer;
    buf = HeapAlloc( GetProcessHeap(), 0, prev_buf_len );
    memcpy( buf, buffers[1].pBuffers[0].pvBuffer, prev_buf_len );
    ok( buf[10] == 0, "Expected initial packet to have sequence number value of 0, got %d\n", buf[10]);

    /* If we don't set the SECBUFFER_ALERT cbBuffer value we will get SEC_E_INSUFFICIENT_MEMORY. */
    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[1].pBuffers[0].cbBuffer = 1420;

    attr = 0;
    exp.LowPart = exp.HighPart = 0xdeadbeef;
    ctx_handle2.dwLower = ctx_handle2.dwUpper = 0xdeadbeef;
    status = InitializeSecurityContextA( &cred_handle, &ctx_handle, (SEC_CHAR *)"winetest", flags_req, 0, 16, NULL, 0,
                                         &ctx_handle2, &buffers[1], &attr, &exp );
    ok( status == SEC_E_INSUFFICIENT_MEMORY, "got %08lx\n", status );

    flags_ret = ISC_RET_CONFIDENTIALITY | ISC_RET_SEQUENCE_DETECT | ISC_RET_REPLAY_DETECT;
    todo_wine ok( attr == flags_ret, "got %08lx\n", attr );
    ok( !exp.LowPart, "got %08lx\n", exp.LowPart );
    ok( !exp.HighPart, "got %08lx\n", exp.HighPart );
    ok( ctx_handle2.dwLower == 0xdeadbeef, "Did not expect dwLower to be set on new context\n");
    ok( ctx_handle2.dwUpper == 0xdeadbeef, "Did not expect dwUpper to be set on new context\n");

    /* No new input buffer value, just repeats the same behavior as before. */
    init_dtls_output_buffer(&buffers[1]);

    attr = 0;
    exp.LowPart = exp.HighPart = 0xdeadbeef;
    ctx_handle2.dwLower = ctx_handle2.dwUpper = 0xdeadbeef;
    status = InitializeSecurityContextA( &cred_handle, &ctx_handle, (SEC_CHAR *)"winetest", flags_req, 0, 16, NULL, 0,
                                         &ctx_handle2, &buffers[1], &attr, &exp );
    ok( status == SEC_I_CONTINUE_NEEDED, "got %08lx\n", status );

    flags_ret = ISC_RET_MANUAL_CRED_VALIDATION | ISC_RET_STREAM | ISC_RET_EXTENDED_ERROR |
                ISC_RET_DATAGRAM | ISC_RET_USED_SUPPLIED_CREDS | ISC_RET_CONFIDENTIALITY |
                ISC_RET_SEQUENCE_DETECT | ISC_RET_REPLAY_DETECT;
    ok( attr == flags_ret, "got %08lx\n", attr );
    todo_wine ok( exp.LowPart, "got %08lx\n", exp.LowPart );
    todo_wine ok( exp.HighPart, "got %08lx\n", exp.HighPart );
    ok( buffers[1].pBuffers[1].BufferType == SECBUFFER_ALERT, "Expected buffertype SECBUFFER_ALERT, got %#lx\n",
        buffers[1].pBuffers[1].BufferType);
    ok( !buffers[1].pBuffers[1].cbBuffer, "Expected SECBUFFER_ALERT buffer to be empty, got %#lx\n",
        buffers[1].pBuffers[1].cbBuffer);
    ok( ctx_handle.dwLower == ctx_handle2.dwLower, "dwLower mismatch, expected %#Ix, got %#Ix\n",
        ctx_handle.dwLower, ctx_handle2.dwLower);
    ok( ctx_handle.dwUpper == ctx_handle2.dwUpper, "dwUpper mismatch, expected %#Ix, got %#Ix\n",
        ctx_handle.dwUpper, ctx_handle2.dwUpper);

    /* With no new input buffer, output buffer length should match prior call. */
    ok(buffers[1].pBuffers[0].cbBuffer == prev_buf_len, "Output buffer size mismatch, expected %#lx, got %#lx\n",
            prev_buf_len, buffers[1].pBuffers[0].cbBuffer);

    /* The retransmission packet and the original packet should only differ in their sequence number value. */
    buf2 = (char *)buffers[1].pBuffers[0].pvBuffer;
    ok( buf2[10] == 1, "Expected retransmitted packet to have sequence number value of 1, got %d\n", buf2[10]);
    ok( !memcmp(buf2, buf, 9), "Lower portion mismatch between retransmitted packet and original packet\n");
    ok( !memcmp(buf2 + 11, buf + 11, prev_buf_len - 11),
        "Upper portion mismatch between retransmitted packet and original packet\n");

    free_buffers( &buffers[0] );
    HeapFree(GetProcessHeap(), 0, buf);
    HeapFree(GetProcessHeap(), 0, buffers[1].pBuffers[1].pvBuffer);
    free_buffers( &buffers[1] );
    DeleteSecurityContext( &ctx_handle );
    FreeCredentialsHandle( &cred_handle );
}

static void test_connection_shutdown(void)
{
    static const BYTE message[] = {0x15, 0x03, 0x01, 0x00, 0x02, 0x01, 0x00};
    static const BYTE message2[] = {0x15, 0x03, 0x01, 0x00, 0x02, 0x02, 0x2a};
    CtxtHandle context, context2;
    SecBufferDesc buffers[2];
    SECURITY_STATUS status;
    CredHandle cred_handle;
    SCHANNEL_CRED cred;
    SecBuffer *buf;
    SCHANNEL_ALERT_TOKEN alert;
    ULONG attrs;
    void *tmp;

    init_cred(&cred);
    cred.grbitEnabledProtocols = SP_PROT_TLS1_CLIENT;
    cred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION;

    status = AcquireCredentialsHandleA( NULL, (SEC_CHAR *)UNISP_NAME_A, SECPKG_CRED_OUTBOUND, NULL,
                                        &cred, NULL, NULL, &cred_handle, NULL );
    ok( status == SEC_E_OK, "got %08lx\n", status );

    init_buffers( &buffers[0], 2, 24 );
    init_buffers( &buffers[1], 1, 1000 );
    buffers[0].cBuffers = 1;
    buffers[0].pBuffers[0].BufferType = SECBUFFER_EMPTY;
    buffers[1].cBuffers = 1;
    buffers[1].pBuffers[0].BufferType = SECBUFFER_TOKEN;
    buffers[1].pBuffers[0].cbBuffer = 1000;
    tmp = buffers[1].pBuffers[0].pvBuffer;
    buffers[1].pBuffers[0].pvBuffer = NULL;
    status = InitializeSecurityContextA( &cred_handle, NULL, (SEC_CHAR *)"localhost",
                                         ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM,
                                         0, 0, &buffers[0], 0, &context, &buffers[1], &attrs, NULL );
    ok( status == SEC_I_CONTINUE_NEEDED, "Expected SEC_I_CONTINUE_NEEDED, got %08lx\n", status );
    ok( !!buffers[1].pBuffers[0].pvBuffer, "Got NULL buffer.\n" );
    FreeContextBuffer( buffers[1].pBuffers[0].pvBuffer );
    buffers[1].pBuffers[0].pvBuffer = tmp;

    buf = &buffers[0].pBuffers[0];
    buffers[0].cBuffers = 2;
    buf->cbBuffer = sizeof(DWORD);
    *(DWORD *)buf->pvBuffer = SCHANNEL_SHUTDOWN;
    buf->BufferType = SECBUFFER_TOKEN;
    buffers[0].pBuffers[1] = buffers[0].pBuffers[0];

    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_INVALID_TOKEN, "got %08lx.\n", status );

    buffers[0].pBuffers[1].cbBuffer = 0;
    buffers[0].pBuffers[1].BufferType = SECBUFFER_EMPTY;
    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_INVALID_TOKEN, "got %08lx.\n", status );

    *(DWORD *)buf->pvBuffer = SCHANNEL_RENEGOTIATE;
    buffers[0].cBuffers = 1;
    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_UNSUPPORTED_FUNCTION, "got %08lx.\n", status );

    status = ApplyControlToken(NULL, buffers);
    ok( status == SEC_E_INVALID_HANDLE, "got %08lx.\n", status );

    status = ApplyControlToken( &context, NULL );
    ok( status == SEC_E_INTERNAL_ERROR, "got %08lx.\n", status );

    *(DWORD *)buf->pvBuffer = SCHANNEL_SHUTDOWN;

    buf->BufferType = SECBUFFER_ALERT;
    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_INVALID_TOKEN, "got %08lx.\n", status );
    buf->BufferType = SECBUFFER_DATA;
    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_INVALID_TOKEN, "got %08lx.\n", status );

    buf->BufferType = SECBUFFER_TOKEN;

    buf->cbBuffer = 2;
    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_UNSUPPORTED_FUNCTION, "got %08lx.\n", status );

    buf->cbBuffer = sizeof(DWORD) + 1;
    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_OK, "got %08lx.\n", status );

    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_OK, "got %08lx.\n", status );

    buf->cbBuffer = 1000;
    buf->BufferType = SECBUFFER_TOKEN;
    context2.dwLower = context2.dwUpper = 0xdeadbeef;
    status = InitializeSecurityContextA( &cred_handle, &context, NULL, 0, 0, 0, &buffers[1], 0,
                                         &context2, &buffers[0], &attrs, NULL );
    ok( status == SEC_E_OK, "got %08lx.\n", status );
    ok( context.dwLower == context2.dwLower, "dwLower mismatch, expected %#Ix, got %#Ix\n",
                                             context.dwLower, context2.dwLower );
    ok( context.dwUpper == context2.dwUpper, "dwUpper mismatch, expected %#Ix, got %#Ix\n",
                                             context.dwUpper, context2.dwUpper );
    ok( buf->cbBuffer == sizeof(message), "got cbBuffer %#lx.\n", buf->cbBuffer );
    ok( !memcmp( buf->pvBuffer, message, sizeof(message) ), "message data mismatch.\n" );

    buf->BufferType = SECBUFFER_TOKEN;
    buf->cbBuffer = 1000;
    context2.dwLower = context2.dwUpper = 0xdeadbeef;
    status = InitializeSecurityContextA( &cred_handle, &context, NULL, 0, 0, 0, NULL, 0,
                                         &context2, &buffers[1], &attrs, NULL );
    ok( status == SEC_E_INCOMPLETE_MESSAGE, "got %08lx.\n", status );
    ok( buf->cbBuffer == 1000, "got cbBuffer %#lx.\n", buf->cbBuffer );
    ok( context2.dwLower == 0xdeadbeef, "dwLower mismatch, got %#Ix\n", context2.dwLower );
    ok( context2.dwUpper == 0xdeadbeef, "dwUpper mismatch, got %#Ix\n", context2.dwUpper );

    buf->cbBuffer = sizeof(DWORD);
    *(DWORD *)buf->pvBuffer = SCHANNEL_SHUTDOWN;
    buf->BufferType = SECBUFFER_TOKEN;
    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_OK, "got %08lx.\n", status );

    buffers[1].pBuffers[0].cbBuffer = 1000;
    status = InitializeSecurityContextA( &cred_handle, &context, NULL, 0, 0, 0,
                                         NULL, 0, NULL, &buffers[1], &attrs, NULL );
    ok( status == SEC_E_OK, "got %08lx.\n", status );
    ok( buffers[1].pBuffers[0].cbBuffer == sizeof(message), "got cbBuffer %#lx.\n", buffers[1].pBuffers[0].cbBuffer );
    ok( !memcmp( buffers[1].pBuffers[0].pvBuffer, message, sizeof(message) ), "message data mismatch.\n" );

    alert.dwTokenType = SCHANNEL_ALERT;
    alert.dwAlertType = TLS1_ALERT_FATAL;
    alert.dwAlertNumber = TLS1_ALERT_BAD_CERTIFICATE;
    memcpy(buf->pvBuffer, &alert, sizeof(alert));
    buf->cbBuffer = sizeof(alert);
    status = ApplyControlToken( &context, buffers );
    ok( status == SEC_E_OK, "got %08lx.\n", status );

    buffers[1].pBuffers[0].cbBuffer = 1000;
    status = InitializeSecurityContextA( &cred_handle, &context, NULL, 0, 0, 0,
                                         NULL, 0, NULL, &buffers[1], &attrs, NULL );
    ok( status == SEC_E_OK, "got %08lx.\n", status );
    ok( buffers[1].pBuffers[0].cbBuffer == sizeof(message2), "got cbBuffer %#lx.\n", buffers[1].pBuffers[0].cbBuffer );
    ok( !memcmp( buffers[1].pBuffers[0].pvBuffer, message2, sizeof(message2) ), "message data mismatch.\n" );

    free_buffers( &buffers[0] );
    free_buffers( &buffers[1] );
    DeleteSecurityContext( &context );
    FreeCredentialsHandle( &cred_handle );
}

START_TEST(schannel)
{
    WSADATA wsa_data;

    WSAStartup(0x0202, &wsa_data);

    test_cread_attrs();
    testAcquireSecurityContext();
    test_InitializeSecurityContext();
    test_communication();
    test_application_protocol_negotiation();
    test_server_protocol_negotiation();
    test_dtls();
    test_connection_shutdown();
}
