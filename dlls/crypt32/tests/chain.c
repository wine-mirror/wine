/*
 * crypt32 certificate chain functions tests
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
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

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

static void testCreateCertChainEngine(void)
{
    BOOL ret;
    CERT_CHAIN_ENGINE_CONFIG config = { 0 };
    HCERTCHAINENGINE engine;
    HCERTSTORE store;

    /* Crash
    ret = CertCreateCertificateChainEngine(NULL, NULL);
    ret = CertCreateCertificateChainEngine(NULL, &engine);
     */
    ret = CertCreateCertificateChainEngine(&config, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertCreateCertificateChainEngine(&config, &engine);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Crashes
    config.cbSize = sizeof(config);
    ret = CertCreateCertificateChainEngine(&config, NULL);
     */
    config.cbSize = sizeof(config);
    ret = CertCreateCertificateChainEngine(&config, &engine);
    ok(ret, "CertCreateCertificateChainEngine failed: %08x\n", GetLastError());
    CertFreeCertificateChainEngine(engine);
    config.dwFlags = 0xff000000;
    ret = CertCreateCertificateChainEngine(&config, &engine);
    ok(ret, "CertCreateCertificateChainEngine failed: %08x\n", GetLastError());
    CertFreeCertificateChainEngine(engine);

    /* Creating a cert with no root certs at all is allowed.. */
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    config.hRestrictedRoot = store;
    ret = CertCreateCertificateChainEngine(&config, &engine);
    ok(ret, "CertCreateCertificateChainEngine failed: %08x\n", GetLastError());
    CertFreeCertificateChainEngine(engine);

    /* but creating one with a restricted root with a cert that isn't a member
     * of the Root store isn't allowed.
     */
    CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING, selfSignedCert,
     sizeof(selfSignedCert), CERT_STORE_ADD_ALWAYS, NULL);
    ret = CertCreateCertificateChainEngine(&config, &engine);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());

    CertCloseStore(store, 0);
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

static void testGetCertChain(void)
{
    BOOL ret;
    CERT_CHAIN_ENGINE_CONFIG config = { 0 };
    HCERTCHAINENGINE engine;
    PCCERT_CONTEXT cert;
    CERT_CHAIN_PARA para = { 0 };
    PCCERT_CHAIN_CONTEXT chain;
    HCERTSTORE store;

    /* Basic parameter checks */
    ret = CertGetCertificateChain(NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL);
    todo_wine
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertGetCertificateChain(NULL, NULL, NULL, NULL, NULL, 0, NULL,
     &chain);
    todo_wine
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Crash
    ret = CertGetCertificateChain(NULL, NULL, NULL, NULL, &para, 0, NULL, NULL);
    ret = CertGetCertificateChain(NULL, NULL, NULL, NULL, &para, 0, NULL,
     &chain);
     */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    todo_wine
    ret = CertGetCertificateChain(NULL, cert, NULL, NULL, NULL, 0, NULL, NULL);
    todo_wine
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Crash
    ret = CertGetCertificateChain(NULL, cert, NULL, NULL, &para, 0, NULL, NULL);
     */

    /* Tests with an invalid cert (one whose signature is bad) */
    ret = CertGetCertificateChain(NULL, cert, NULL, NULL, &para, 0, NULL,
     &chain);
    ok(!ret, "Expected failure\n");
    para.cbSize = sizeof(para);
    ret = CertGetCertificateChain(NULL, cert, NULL, NULL, &para, 0, NULL,
     &chain);
    ok(!ret, "Expected failure\n");
    CertFreeCertificateContext(cert);

    /* Tests with a valid cert that doesn't trace back to a trusted root */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, selfSignedCert,
     sizeof(selfSignedCert));
    ret = CertGetCertificateChain(NULL, cert, NULL, NULL, NULL, 0, NULL, NULL);
    todo_wine
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertGetCertificateChain(NULL, cert, NULL, NULL, &para, 0, NULL,
     &chain);
    todo_wine
    ok(ret, "CertGetCertificateChain failed: %08x\n", GetLastError());
    todo_wine
    ok(chain != NULL, "Expected a chain\n");
    if (chain)
    {
        ok(chain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_UNTRUSTED_ROOT,
         "Expected CERT_TRUST_IS_UNTRUSTED_ROOT, got %08x\n",
         chain->TrustStatus.dwErrorStatus);
        ok(chain->TrustStatus.dwInfoStatus == CERT_TRUST_HAS_PREFERRED_ISSUER,
         "Expected CERT_TRUST_HAS_PREFERRED_ISSUER, got %08x\n",
         chain->TrustStatus.dwInfoStatus);
        ok(chain->cChain == 1, "Expected 1 chain, got %d\n", chain->cChain);
        ok(chain->rgpChain[0]->cElement == 1,
         "Expected one chain element, got %d\n", chain->rgpChain[0]->cElement);
        ok(chain->rgpChain[0]->pTrustListInfo == NULL,
         "Expected no trust list\n");
        CertFreeCertificateChain(chain);
    }

    /* A self-signed cert isn't affected by having no chain to a trusted root */
    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    config.cbSize = sizeof(config);
    config.hRestrictedRoot = store;
    ret = CertCreateCertificateChainEngine(&config, &engine);
    ok(ret, "CertCreateCertificateChainEngine failed: %08x\n", GetLastError());
    ret = CertGetCertificateChain(engine, cert, NULL, NULL, &para, 0, NULL,
     &chain);
    todo_wine
    ok(chain != NULL, "Expected a chain\n");
    if (chain)
    {
        ok(chain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_UNTRUSTED_ROOT,
         "Expected CERT_TRUST_IS_UNTRUSTED_ROOT, got %08x\n",
         chain->TrustStatus.dwErrorStatus);
        ok(chain->TrustStatus.dwInfoStatus == CERT_TRUST_HAS_PREFERRED_ISSUER,
         "Expected CERT_TRUST_HAS_PREFERRED_ISSUER, got %08x\n",
         chain->TrustStatus.dwInfoStatus);
        ok(chain->cChain == 1, "Expected 1 chain, got %d\n", chain->cChain);
        ok(chain->rgpChain[0]->cElement == 1,
         "Expected one chain element, got %d\n", chain->rgpChain[0]->cElement);
        ok(chain->rgpChain[0]->pTrustListInfo == NULL,
         "Expected no trust list\n");
        CertFreeCertificateChain(chain);
    }
    CertFreeCertificateChainEngine(engine);
    CertCloseStore(store, 0);
    CertFreeCertificateContext(cert);
}

START_TEST(chain)
{
    testCreateCertChainEngine();
    testGetCertChain();
}
