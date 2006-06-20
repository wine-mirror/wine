/*
 * crypt32 CRL functions tests
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
static const BYTE bigCert2[] = { 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x41, 0x6c, 0x65, 0x78, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22,
 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30,
 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x41, 0x6c, 0x65, 0x78, 0x20,
 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01,
 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01,
 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01 };
static const BYTE bigCertWithDifferentIssuer[] = { 0x30, 0x7a, 0x02, 0x01,
 0x01, 0x30, 0x02, 0x06, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03,
 0x55, 0x04, 0x03, 0x13, 0x0a, 0x41, 0x6c, 0x65, 0x78, 0x20, 0x4c, 0x61, 0x6e,
 0x67, 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30,
 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30,
 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a,
 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02,
 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03,
 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff,
 0x02, 0x01, 0x01 };
static const BYTE CRL[] = { 0x30, 0x2c, 0x30, 0x02, 0x06, 0x00,
 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a,
 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f, 0x31,
 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x5a };
static const BYTE newerCRL[] = { 0x30, 0x2a, 0x30, 0x02, 0x06, 0x00, 0x30,
 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a,
 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x17, 0x0d, 0x30, 0x36,
 0x30, 0x35, 0x31, 0x36, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a };
static const BYTE signedCRL[] = { 0x30, 0x45, 0x30, 0x2c, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x02, 0x06, 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c,
 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };

static void testCreateCRL(void)
{
    PCCRL_CONTEXT context;

    context = CertCreateCRLContext(0, NULL, 0);
    ok(!context && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    context = CertCreateCRLContext(X509_ASN_ENCODING, NULL, 0);
    ok(!context && GetLastError() == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());
    context = CertCreateCRLContext(X509_ASN_ENCODING, bigCert, sizeof(bigCert));
    ok(!context && GetLastError() == CRYPT_E_ASN1_CORRUPT,
     "Expected CRYPT_E_ASN1_CORRUPT, got %08lx\n", GetLastError());
    context = CertCreateCRLContext(X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL) - 1);
    ok(!context && (GetLastError() == CRYPT_E_ASN1_EOD ||
     GetLastError() == CRYPT_E_ASN1_CORRUPT),
     "Expected CRYPT_E_ASN1_EOD or CRYPT_E_ASN1_CORRUPT, got %08lx\n",
     GetLastError());
    context = CertCreateCRLContext(X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL));
    ok(context != NULL, "CertCreateCRLContext failed: %08lx\n", GetLastError());
    if (context)
        CertFreeCRLContext(context);
    context = CertCreateCRLContext(X509_ASN_ENCODING, CRL, sizeof(CRL));
    ok(context != NULL, "CertCreateCRLContext failed: %08lx\n", GetLastError());
    if (context)
        CertFreeCRLContext(context);
}

static void testAddCRL(void)
{
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    PCCRL_CONTEXT context;
    BOOL ret;

    if (!store) return;

    /* Bad CRL encoding type */
    ret = CertAddEncodedCRLToStore(0, 0, NULL, 0, 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddEncodedCRLToStore(store, 0, NULL, 0, 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddEncodedCRLToStore(0, 0, signedCRL, sizeof(signedCRL), 0, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddEncodedCRLToStore(store, 0, signedCRL, sizeof(signedCRL), 0,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddEncodedCRLToStore(0, 0, signedCRL, sizeof(signedCRL),
     CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddEncodedCRLToStore(store, 0, signedCRL, sizeof(signedCRL),
     CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());

    /* No CRL */
    ret = CertAddEncodedCRLToStore(0, X509_ASN_ENCODING, NULL, 0, 0, NULL);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, NULL, 0, 0, NULL);
    ok(!ret && GetLastError() == CRYPT_E_ASN1_EOD,
     "Expected CRYPT_E_ASN1_EOD, got %08lx\n", GetLastError());

    /* Weird--bad add disposition leads to an access violation in Windows. */
    ret = CertAddEncodedCRLToStore(0, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), 0, NULL);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), 0, NULL);
    ok(!ret && GetLastError() == STATUS_ACCESS_VIOLATION,
     "Expected STATUS_ACCESS_VIOLATION, got %08lx\n", GetLastError());

    /* Weird--can add a CRL to the NULL store (does this have special meaning?)
     */
    context = NULL;
    ret = CertAddEncodedCRLToStore(0, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCRLToStore failed: %08lx\n", GetLastError());
    if (context)
        CertFreeCRLContext(context);

    /* Normal cases: a "signed" CRL is okay.. */
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
    /* and an unsigned one is too. */
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, CRL, sizeof(CRL),
     CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCRLToStore failed: %08lx\n", GetLastError());

    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, newerCRL,
     sizeof(newerCRL), CERT_STORE_ADD_NEW, NULL);
    ok(!ret && GetLastError() == CRYPT_E_EXISTS,
     "Expected CRYPT_E_EXISTS, got %08lx\n", GetLastError());

    /* This should replace (one of) the existing CRL(s). */
    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, newerCRL,
     sizeof(newerCRL), CERT_STORE_ADD_NEWER, NULL);
    ok(ret, "CertAddEncodedCRLToStore failed: %08lx\n", GetLastError());

    CertCloseStore(store, 0);
}

static void testFindCRL(void)
{
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    PCCRL_CONTEXT context;
    PCCERT_CONTEXT cert;
    BOOL ret;

    if (!store) return;

    ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
     sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCRLToStore failed: %08lx\n", GetLastError());

    /* Crashes
    context = CertFindCRLInStore(NULL, 0, 0, 0, NULL, NULL);
     */

    /* Find any context */
    context = CertFindCRLInStore(store, 0, 0, CRL_FIND_ANY, NULL, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);
    /* Bogus flags are ignored */
    context = CertFindCRLInStore(store, 0, 1234, CRL_FIND_ANY, NULL, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);
    /* CRL encoding type is ignored too */
    context = CertFindCRLInStore(store, 1234, 0, CRL_FIND_ANY, NULL, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);

    /* This appears to match any cert */
    context = CertFindCRLInStore(store, 0, 0, CRL_FIND_ISSUED_BY, NULL, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);

    /* Try to match an issuer that isn't in the store */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert2,
     sizeof(bigCert2));
    ok(cert != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    context = CertFindCRLInStore(store, 0, 0, CRL_FIND_ISSUED_BY, cert, NULL);
    ok(context == NULL, "Expected no matching context\n");
    CertFreeCertificateContext(cert);

    /* Match an issuer that is in the store */
    cert = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ok(cert != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    context = CertFindCRLInStore(store, 0, 0, CRL_FIND_ISSUED_BY, cert, NULL);
    ok(context != NULL, "Expected a context\n");
    if (context)
        CertFreeCRLContext(context);
    CertFreeCertificateContext(cert);

    CertCloseStore(store, 0);
}

static void checkCRLHash(const BYTE *data, DWORD dataLen, ALG_ID algID,
 PCCRL_CONTEXT context, DWORD propID)
{
    BYTE hash[20] = { 0 }, hashProperty[20];
    BOOL ret;
    DWORD size;

    memset(hash, 0, sizeof(hash));
    memset(hashProperty, 0, sizeof(hashProperty));
    size = sizeof(hash);
    ret = CryptHashCertificate(0, algID, 0, data, dataLen, hash, &size);
    ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
    ret = CertGetCRLContextProperty(context, propID, hashProperty, &size);
    ok(ret, "CertGetCRLContextProperty failed: %08lx\n", GetLastError());
    ok(!memcmp(hash, hashProperty, size), "Unexpected hash for property %ld\n",
     propID);
}

static void testCRLProperties(void)
{
    PCCRL_CONTEXT context = CertCreateCRLContext(X509_ASN_ENCODING,
     CRL, sizeof(CRL));

    ok(context != NULL, "CertCreateCRLContext failed: %08lx\n", GetLastError());
    if (context)
    {
        DWORD propID, numProps, access, size;
        BOOL ret;
        BYTE hash[20] = { 0 }, hashProperty[20];
        CRYPT_DATA_BLOB blob;

        /* This crashes
        propID = CertEnumCRLContextProperties(NULL, 0);
         */

        propID = 0;
        numProps = 0;
        do {
            propID = CertEnumCRLContextProperties(context, propID);
            if (propID)
                numProps++;
        } while (propID != 0);
        ok(numProps == 0, "Expected 0 properties, got %ld\n", numProps);

        /* Tests with a NULL cert context.  Prop ID 0 fails.. */
        ret = CertSetCRLContextProperty(NULL, 0, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        /* while this just crashes.
        ret = CertSetCRLContextProperty(NULL, CERT_KEY_PROV_HANDLE_PROP_ID, 0,
         NULL);
         */

        ret = CertSetCRLContextProperty(context, 0, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        /* Can't set the cert property directly, this crashes.
        ret = CertSetCRLContextProperty(context, CERT_CRL_PROP_ID, 0, CRL);
         */

        /* These all crash.
        ret = CertGetCRLContextProperty(context, CERT_ACCESS_STATE_PROP_ID, 0,
         NULL);
        ret = CertGetCRLContextProperty(context, CERT_HASH_PROP_ID, NULL, NULL);
        ret = CertGetCRLContextProperty(context, CERT_HASH_PROP_ID, 
         hashProperty, NULL);
         */
        /* A missing prop */
        size = 0;
        ret = CertGetCRLContextProperty(context, CERT_KEY_PROV_INFO_PROP_ID,
         NULL, &size);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
        /* And, an implicit property */
        ret = CertGetCRLContextProperty(context, CERT_ACCESS_STATE_PROP_ID,
         NULL, &size);
        ok(ret, "CertGetCRLContextProperty failed: %08lx\n", GetLastError());
        ret = CertGetCRLContextProperty(context, CERT_ACCESS_STATE_PROP_ID,
         &access, &size);
        ok(ret, "CertGetCRLContextProperty failed: %08lx\n", GetLastError());
        ok(!(access & CERT_ACCESS_STATE_WRITE_PERSIST_FLAG),
         "Didn't expect a persisted crl\n");
        /* Trying to set this "read only" property crashes.
        access |= CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;
        ret = CertSetCRLContextProperty(context, CERT_ACCESS_STATE_PROP_ID, 0,
         &access);
         */

        /* Can I set the hash to an invalid hash? */
        blob.pbData = hash;
        blob.cbData = sizeof(hash);
        ret = CertSetCRLContextProperty(context, CERT_HASH_PROP_ID, 0, &blob);
        ok(ret, "CertSetCRLContextProperty failed: %08lx\n",
         GetLastError());
        size = sizeof(hashProperty);
        ret = CertGetCRLContextProperty(context, CERT_HASH_PROP_ID,
         hashProperty, &size);
        ok(!memcmp(hashProperty, hash, sizeof(hash)), "Unexpected hash\n");
        /* Delete the (bogus) hash, and get the real one */
        ret = CertSetCRLContextProperty(context, CERT_HASH_PROP_ID, 0, NULL);
        ok(ret, "CertSetCRLContextProperty failed: %08lx\n", GetLastError());
        checkCRLHash(CRL, sizeof(CRL), CALG_SHA1, context, CERT_HASH_PROP_ID);

        /* Now that the hash property is set, we should get one property when
         * enumerating.
         */
        propID = 0;
        numProps = 0;
        do {
            propID = CertEnumCRLContextProperties(context, propID);
            if (propID)
                numProps++;
        } while (propID != 0);
        ok(numProps == 1, "Expected 1 properties, got %ld\n", numProps);

        /* Check a few other implicit properties */
        checkCRLHash(CRL, sizeof(CRL), CALG_MD5, context,
         CERT_MD5_HASH_PROP_ID);

        CertFreeCRLContext(context);
    }
}

START_TEST(crl)
{
    testCreateCRL();
    testAddCRL();
    testFindCRL();

    testCRLProperties();
}
