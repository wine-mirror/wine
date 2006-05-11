/*
 * crypt32 cert store function tests
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

/* The following aren't defined in wincrypt.h, as they're "reserved" */
#define CERT_CERT_PROP_ID 32
#define CERT_CRL_PROP_ID  33
#define CERT_CTL_PROP_ID  34

struct CertPropIDHeader
{
    DWORD propID;
    DWORD unknown1;
    DWORD cb;
};

static const BYTE emptyCert[] = { 0x30, 0x00 };
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
static const BYTE subjectName[] = { 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06,
 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61,
 0x6e, 0x67, 0x00 };
static const BYTE bigCertHash[] = { 0x6e, 0x30, 0x90, 0x71, 0x5f, 0xd9, 0x23,
 0x56, 0xeb, 0xae, 0x25, 0x40, 0xe6, 0x22, 0xda, 0x19, 0x26, 0x02, 0xa6, 0x08 };
static const BYTE signedBigCert[] = {
 0x30, 0x81, 0x93, 0x30, 0x7a, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06, 0x00, 0x30,
 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a,
 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x22, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06,
 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61,
 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03, 0x01, 0x00, 0xa3,
 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x01, 0x01, 0xff,
 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02, 0x01, 0x01, 0x30, 0x02, 0x06,
 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07,
 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };
static const BYTE serializedCert[] = { 0x20, 0x00, 0x00, 0x00,
 0x01, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00, 0x30, 0x7a, 0x02, 0x01, 0x01,
 0x30, 0x02, 0x06, 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55,
 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67,
 0x00, 0x30, 0x22, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x18, 0x0f, 0x31, 0x36, 0x30, 0x31,
 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x15,
 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75,
 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06,
 0x00, 0x03, 0x01, 0x00, 0xa3, 0x16, 0x30, 0x14, 0x30, 0x12, 0x06, 0x03, 0x55,
 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x08, 0x30, 0x06, 0x01, 0x01, 0xff, 0x02,
 0x01, 0x01 };
static const BYTE signedCRL[] = { 0x30, 0x45, 0x30, 0x2c, 0x30, 0x02, 0x06,
 0x00, 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
 0x0a, 0x4a, 0x75, 0x61, 0x6e, 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x18, 0x0f,
 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x30, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x5a, 0x30, 0x02, 0x06, 0x00, 0x03, 0x11, 0x00, 0x0f, 0x0e, 0x0d, 0x0c,
 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00 };
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
static const BYTE subjectName2[] = { 0x30, 0x15, 0x31, 0x13, 0x30, 0x11, 0x06,
 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x41, 0x6c, 0x65, 0x78, 0x20, 0x4c, 0x61,
 0x6e, 0x67, 0x00 };
static const BYTE bigCert2Hash[] = { 0x4a, 0x7f, 0x32, 0x1f, 0xcf, 0x3b, 0xc0,
 0x87, 0x48, 0x2b, 0xa1, 0x86, 0x54, 0x18, 0xe4, 0x3a, 0x0e, 0x53, 0x7e, 0x2b };
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
static const BYTE serialNum[] = { 1 };

static void testAddCert(void)
{
    HCERTSTORE store;

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    if (store != NULL)
    {
        HCERTSTORE collection;
        PCCERT_CONTEXT context;
        BOOL ret;

        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert2, sizeof(bigCert2), CERT_STORE_ADD_NEW, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        /* This has the same name as bigCert, so finding isn't done by name */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         certWithUsage, sizeof(certWithUsage), CERT_STORE_ADD_NEW, &context);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        ok(context != NULL, "Expected a context\n");
        if (context)
        {
            CRYPT_DATA_BLOB hash = { sizeof(bigCert2Hash),
             (LPBYTE)bigCert2Hash };

            CertDeleteCertificateFromStore(context);
            /* Set the same hash as bigCert2, and try to readd it */
            ret = CertSetCertificateContextProperty(context, CERT_HASH_PROP_ID,
             0, &hash);
            ok(ret, "CertSetCertificateContextProperty failed: %08lx\n",
             GetLastError());
            ret = CertAddCertificateContextToStore(store, context,
             CERT_STORE_ADD_NEW, NULL);
            /* The failure is a bit odd (CRYPT_E_ASN1_BADTAG), so just check
             * that it fails.
             */
            ok(!ret, "Expected failure\n");
            CertFreeCertificateContext(context);
        }
        context = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert2,
         sizeof(bigCert2));
        ok(context != NULL, "Expected a context\n");
        if (context)
        {
            /* Try to readd bigCert2 to the store */
            ret = CertAddCertificateContextToStore(store, context,
             CERT_STORE_ADD_NEW, NULL);
            ok(!ret && GetLastError() == CRYPT_E_EXISTS,
             "Expected CRYPT_E_EXISTS, got %08lx\n", GetLastError());
            CertFreeCertificateContext(context);
        }

        /* FIXME: test whether adding a cert with the same subject name and
         * serial number (but different otherwise) as an existing cert works.
         */
        collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);
        ok(collection != NULL, "CertOpenStore failed: %08lx\n", GetLastError());
        if (collection)
        {
            /* Add store to the collection, but disable updates */
            CertAddStoreToCollection(collection, store, 0, 0);

            context = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert2,
             sizeof(bigCert2));
            ok(context != NULL, "Expected a context\n");
            if (context)
            {
                /* Try to readd bigCert2 to the collection */
                ret = CertAddCertificateContextToStore(collection, context,
                 CERT_STORE_ADD_NEW, NULL);
                ok(!ret && GetLastError() == CRYPT_E_EXISTS,
                 "Expected CRYPT_E_EXISTS, got %08lx\n", GetLastError());
                /* Replacing an existing certificate context is allowed, even
                 * though updates to the collection aren't..
                 */
                ret = CertAddCertificateContextToStore(collection, context,
                 CERT_STORE_ADD_REPLACE_EXISTING, NULL);
                ok(ret, "CertAddCertificateContextToStore failed: %08lx\n",
                 GetLastError());
                /* but adding a new certificate isn't allowed. */
                ret = CertAddCertificateContextToStore(collection, context,
                 CERT_STORE_ADD_ALWAYS, NULL);
                ok(!ret && GetLastError() == E_ACCESSDENIED,
                 "Expected E_ACCESSDENIED, got %08lx\n", GetLastError());
                CertFreeCertificateContext(context);
            }

            CertCloseStore(collection, 0);
        }

        CertCloseStore(store, 0);
    }
}

static void testDupCert(void)
{
    HCERTSTORE store;

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    if (store != NULL)
    {
        PCCERT_CONTEXT context, dupContext;
        BOOL ret;

        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        ok(context != NULL, "Expected a valid cert context\n");
        if (context)
        {
            ok(context->cbCertEncoded == sizeof(bigCert),
             "Expected cert of %d bytes, got %ld\n", sizeof(bigCert),
             context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert, sizeof(bigCert)),
             "Unexpected encoded cert in context\n");
            ok(context->hCertStore == store, "Unexpected store\n");

            dupContext = CertDuplicateCertificateContext(context);
            ok(dupContext != NULL, "Expected valid duplicate\n");
            /* Not only is it a duplicate, it's identical: the address is the
             * same.
             */
            ok(dupContext == context, "Expected identical context addresses\n");
            CertFreeCertificateContext(dupContext);
            CertFreeCertificateContext(context);
        }
        CertCloseStore(store, 0);
    }
}

static void testFindCert(void)
{
    HCERTSTORE store;

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    if (store)
    {
        PCCERT_CONTEXT context = NULL;
        BOOL ret;
        CERT_INFO certInfo = { 0 };
        CRYPT_HASH_BLOB blob;

        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_NEW, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert2, sizeof(bigCert2), CERT_STORE_ADD_NEW, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        /* This has the same name as bigCert */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         certWithUsage, sizeof(certWithUsage), CERT_STORE_ADD_NEW, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());

        /* Crashes
        context = CertFindCertificateInStore(NULL, 0, 0, 0, NULL, NULL);
         */

        /* Check first cert's there, by issuer */
        certInfo.Subject.pbData = (LPBYTE)subjectName;
        certInfo.Subject.cbData = sizeof(subjectName);
        certInfo.SerialNumber.pbData = (LPBYTE)serialNum;
        certInfo.SerialNumber.cbData = sizeof(serialNum);
        context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
         CERT_FIND_ISSUER_NAME, &certInfo.Subject, NULL);
        ok(context != NULL, "CertFindCertificateInStore failed: %08lx\n",
         GetLastError());
        if (context)
        {
            context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
             CERT_FIND_ISSUER_NAME, &certInfo.Subject, context);
            ok(context != NULL, "Expected more than one cert\n");
            if (context)
            {
                context = CertFindCertificateInStore(store, X509_ASN_ENCODING,
                 0, CERT_FIND_ISSUER_NAME, &certInfo.Subject, context);
                ok(context == NULL, "Expected precisely two certs\n");
            }
        }

        /* Check second cert's there as well, by subject name */
        certInfo.Subject.pbData = (LPBYTE)subjectName2;
        certInfo.Subject.cbData = sizeof(subjectName2);
        context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
         CERT_FIND_SUBJECT_NAME, &certInfo.Subject, NULL);
        ok(context != NULL, "CertFindCertificateInStore failed: %08lx\n",
         GetLastError());
        if (context)
        {
            context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
             CERT_FIND_ISSUER_NAME, &certInfo.Subject, context);
            ok(context == NULL, "Expected one cert only\n");
        }

        /* Strange but true: searching for the subject cert requires you to set
         * the issuer, not the subject
         */
        context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
         CERT_FIND_SUBJECT_CERT, &certInfo.Subject, NULL);
        ok(context == NULL, "Expected no certificate\n");
        certInfo.Subject.pbData = NULL;
        certInfo.Subject.cbData = 0;
        certInfo.Issuer.pbData = (LPBYTE)subjectName2;
        certInfo.Issuer.cbData = sizeof(subjectName2);
        context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
         CERT_FIND_SUBJECT_CERT, &certInfo, NULL);
        ok(context != NULL, "CertFindCertificateInStore failed: %08lx\n",
         GetLastError());
        if (context)
        {
            context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
             CERT_FIND_ISSUER_NAME, &certInfo.Subject, context);
            ok(context == NULL, "Expected one cert only\n");
        }

        /* The nice thing about hashes, they're unique */
        blob.pbData = (LPBYTE)bigCertHash;
        blob.cbData = sizeof(bigCertHash);
        context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
         CERT_FIND_SHA1_HASH, &blob, NULL);
        ok(context != NULL, "CertFindCertificateInStore failed: %08lx\n",
         GetLastError());
        if (context)
        {
            context = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0,
             CERT_FIND_ISSUER_NAME, &certInfo.Subject, context);
            ok(context == NULL, "Expected one cert only\n");
        }

        CertCloseStore(store, 0);
    }
}

static void testGetSubjectCert(void)
{
    HCERTSTORE store;

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    if (store != NULL)
    {
        PCCERT_CONTEXT context1, context2;
        CERT_INFO info = { 0 };
        BOOL ret;

        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert2, sizeof(bigCert2), CERT_STORE_ADD_NEW, &context1);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        ok(context1 != NULL, "Expected a context\n");
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         certWithUsage, sizeof(certWithUsage), CERT_STORE_ADD_NEW, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());

        context2 = CertGetSubjectCertificateFromStore(store, X509_ASN_ENCODING,
         NULL);
        ok(!context2 && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        context2 = CertGetSubjectCertificateFromStore(store, X509_ASN_ENCODING,
         &info);
        ok(!context2 && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
        info.SerialNumber.cbData = sizeof(serialNum);
        info.SerialNumber.pbData = (LPBYTE)serialNum;
        context2 = CertGetSubjectCertificateFromStore(store, X509_ASN_ENCODING,
         &info);
        ok(!context2 && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
        info.Issuer.cbData = sizeof(subjectName2);
        info.Issuer.pbData = (LPBYTE)subjectName2;
        context2 = CertGetSubjectCertificateFromStore(store, X509_ASN_ENCODING,
         &info);
        ok(context2 != NULL,
         "CertGetSubjectCertificateFromStore failed: %08lx\n", GetLastError());
        /* Not only should this find a context, but it should be the same
         * (same address) as context1.
         */
        ok(context1 == context2, "Expected identical context addresses\n");
        CertFreeCertificateContext(context2);

        CertFreeCertificateContext(context1);
        CertCloseStore(store, 0);
    }
}

/* This expires in 1970 or so */
static const BYTE expiredCert[] = { 0x30, 0x82, 0x01, 0x33, 0x30, 0x81, 0xe2,
 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x10, 0xc4, 0xd7, 0x7f, 0x0e, 0x6f, 0xa6,
 0x8c, 0xaa, 0x47, 0x47, 0x40, 0xe7, 0xb7, 0x0b, 0x4a, 0x7f, 0x30, 0x09, 0x06,
 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1d, 0x05, 0x00, 0x30, 0x1f, 0x31, 0x1d, 0x30,
 0x1b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x14, 0x61, 0x72, 0x69, 0x63, 0x40,
 0x63, 0x6f, 0x64, 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63,
 0x6f, 0x6d, 0x30, 0x1e, 0x17, 0x0d, 0x36, 0x39, 0x30, 0x31, 0x30, 0x31, 0x30,
 0x30, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x17, 0x0d, 0x37, 0x30, 0x30, 0x31, 0x30,
 0x31, 0x30, 0x36, 0x30, 0x30, 0x30, 0x30, 0x5a, 0x30, 0x1f, 0x31, 0x1d, 0x30,
 0x1b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x14, 0x61, 0x72, 0x69, 0x63, 0x40,
 0x63, 0x6f, 0x64, 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63,
 0x6f, 0x6d, 0x30, 0x5c, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7,
 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x4b, 0x00, 0x30, 0x48, 0x02, 0x41,
 0x00, 0xa1, 0xaf, 0x4a, 0xea, 0xa7, 0x83, 0x57, 0xc0, 0x37, 0x33, 0x7e, 0x29,
 0x5e, 0x0d, 0xfc, 0x44, 0x74, 0x3a, 0x1d, 0xc3, 0x1b, 0x1d, 0x96, 0xed, 0x4e,
 0xf4, 0x1b, 0x98, 0xec, 0x69, 0x1b, 0x04, 0xea, 0x25, 0xcf, 0xb3, 0x2a, 0xf5,
 0xd9, 0x22, 0xd9, 0x8d, 0x08, 0x39, 0x81, 0xc6, 0xe0, 0x4f, 0x12, 0x37, 0x2a,
 0x3f, 0x80, 0xa6, 0x6c, 0x67, 0x43, 0x3a, 0xdd, 0x95, 0x0c, 0xbb, 0x2f, 0x6b,
 0x02, 0x03, 0x01, 0x00, 0x01, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02,
 0x1d, 0x05, 0x00, 0x03, 0x41, 0x00, 0x8f, 0xa2, 0x5b, 0xd6, 0xdf, 0x34, 0xd0,
 0xa2, 0xa7, 0x47, 0xf1, 0x13, 0x79, 0xd3, 0xf3, 0x39, 0xbd, 0x4e, 0x2b, 0xa3,
 0xf4, 0x63, 0x37, 0xac, 0x5a, 0x0c, 0x5e, 0x4d, 0x0d, 0x54, 0x87, 0x4f, 0x31,
 0xfb, 0xa0, 0xce, 0x8f, 0x9a, 0x2f, 0x4d, 0x48, 0xc6, 0x84, 0x8d, 0xf5, 0x70,
 0x74, 0x17, 0xa5, 0xf3, 0x66, 0x47, 0x06, 0xd6, 0x64, 0x45, 0xbc, 0x52, 0xef,
 0x49, 0xe5, 0xf9, 0x65, 0xf3 };

/* This expires in 2036 or so */
static const BYTE childOfExpired[] = { 0x30, 0x81, 0xcc, 0x30, 0x78, 0xa0,
 0x03, 0x02, 0x01, 0x02, 0x02, 0x01, 0x01, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86,
 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x1f, 0x31, 0x1d,
 0x30, 0x1b, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x14, 0x61, 0x72, 0x69, 0x63,
 0x40, 0x63, 0x6f, 0x64, 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e,
 0x63, 0x6f, 0x6d, 0x30, 0x1e, 0x17, 0x0d, 0x30, 0x36, 0x30, 0x35, 0x30, 0x35,
 0x31, 0x37, 0x31, 0x32, 0x34, 0x39, 0x5a, 0x17, 0x0d, 0x33, 0x36, 0x30, 0x35,
 0x30, 0x35, 0x31, 0x37, 0x31, 0x32, 0x34, 0x39, 0x5a, 0x30, 0x15, 0x31, 0x13,
 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0a, 0x4a, 0x75, 0x61, 0x6e,
 0x20, 0x4c, 0x61, 0x6e, 0x67, 0x00, 0x30, 0x07, 0x30, 0x02, 0x06, 0x00, 0x03,
 0x01, 0x00, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01,
 0x01, 0x05, 0x05, 0x00, 0x03, 0x41, 0x00, 0x20, 0x3b, 0xdb, 0x4d, 0x67, 0x50,
 0xec, 0x73, 0x9d, 0xf9, 0x85, 0x5d, 0x18, 0xe9, 0xb4, 0x98, 0xe3, 0x31, 0xb7,
 0x03, 0x0b, 0xc0, 0x39, 0x93, 0x56, 0x81, 0x0a, 0xfc, 0x78, 0xa8, 0x29, 0x42,
 0x5f, 0x69, 0xfb, 0xbc, 0x5b, 0xf2, 0xa6, 0x2a, 0xbe, 0x91, 0x2c, 0xfc, 0x89,
 0x69, 0x15, 0x18, 0x58, 0xe5, 0x02, 0x75, 0xf7, 0x2a, 0xb6, 0xa9, 0xfb, 0x47,
 0x6a, 0x6e, 0x0a, 0x9b, 0xe9, 0xdc };

static void testGetIssuerCert(void)
{
    BOOL ret;
    PCCERT_CONTEXT parent, child;
    DWORD flags = 0xffffffff;
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    ok(store != NULL, "CertOpenStore failed: %08lx\n", GetLastError());

    ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
     expiredCert, sizeof(expiredCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
     GetLastError());

    ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
     childOfExpired, sizeof(childOfExpired), CERT_STORE_ADD_ALWAYS, &child);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
     GetLastError());

    /* These crash:
    parent = CertGetIssuerCertificateFromStore(NULL, NULL, NULL, NULL);
    parent = CertGetIssuerCertificateFromStore(store, NULL, NULL, NULL);
     */
    parent = CertGetIssuerCertificateFromStore(NULL, NULL, NULL, &flags);
    ok(!parent && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    parent = CertGetIssuerCertificateFromStore(store, NULL, NULL, &flags);
    ok(!parent && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    parent = CertGetIssuerCertificateFromStore(store, child, NULL, &flags);
    ok(!parent && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Confusing: the caller cannot set either of the
     * CERT_STORE_NO_*_FLAGs, as these are not checks,
     * they're results:
     */
    flags = CERT_STORE_NO_CRL_FLAG | CERT_STORE_NO_ISSUER_FLAG;
    parent = CertGetIssuerCertificateFromStore(store, child, NULL, &flags);
    ok(!parent && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Perform no checks */
    flags = 0;
    parent = CertGetIssuerCertificateFromStore(store, child, NULL, &flags);
    ok(parent != NULL, "CertGetIssuerCertificateFromStore failed: %08lx\n",
     GetLastError());
    if (parent)
        CertFreeCertificateContext(parent);
    /* Check revocation and signature only */
    flags = CERT_STORE_REVOCATION_FLAG | CERT_STORE_SIGNATURE_FLAG;
    parent = CertGetIssuerCertificateFromStore(store, child, NULL, &flags);
    ok(parent != NULL, "CertGetIssuerCertificateFromStore failed: %08lx\n",
     GetLastError());
    /* Confusing: CERT_STORE_REVOCATION_FLAG succeeds when there is no CRL by
     * setting CERT_STORE_NO_CRL_FLAG.
     */
    ok(flags == (CERT_STORE_REVOCATION_FLAG | CERT_STORE_NO_CRL_FLAG),
     "Expected CERT_STORE_REVOCATION_FLAG | CERT_STORE_NO_CRL_FLAG, got %08lx\n",
     flags);
    if (parent)
        CertFreeCertificateContext(parent);
    /* Now check just the time */
    flags = CERT_STORE_TIME_VALIDITY_FLAG;
    parent = CertGetIssuerCertificateFromStore(store, child, NULL, &flags);
    ok(parent != NULL, "CertGetIssuerCertificateFromStore failed: %08lx\n",
     GetLastError());
    /* Oops: the child is not expired, so the time validity check actually
     * succeeds, even though the signing cert is expired.
     */
    ok(!flags, "Expected check to succeed, got %08lx\n", flags);
    if (parent)
        CertFreeCertificateContext(parent);

    CertFreeCertificateContext(child);
    CertCloseStore(store, 0);
}

static void testMemStore(void)
{
    HCERTSTORE store1, store2;
    PCCERT_CONTEXT context;
    BOOL ret;

    /* NULL provider */
    store1 = CertOpenStore(0, 0, 0, 0, NULL);
    ok(!store1 && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());
    /* weird flags */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_DELETE_FLAG, NULL);
    ok(!store1 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
     "Expected ERROR_CALL_NOT_IMPLEMENTED, got %ld\n", GetLastError());

    /* normal */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store1 != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    /* open existing doesn't */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG, NULL);
    ok(store2 != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    ok(store1 != store2, "Expected different stores\n");

    /* add a bogus (empty) cert */
    context = NULL;
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, emptyCert,
     sizeof(emptyCert), CERT_STORE_ADD_ALWAYS, &context);
    /* Windows returns CRYPT_E_ASN1_EOD, but accept CRYPT_E_ASN1_CORRUPT as
     * well (because matching errors is tough in this case)
     */
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_EOD || GetLastError() ==
     CRYPT_E_ASN1_CORRUPT),
     "Expected CRYPT_E_ASN1_EOD or CRYPT_E_ASN1_CORRUPT, got %08lx\n",
     GetLastError());
    /* add a "signed" cert--the signature isn't a real signature, so this adds
     * without any check of the signature's validity
     */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     signedBigCert, sizeof(signedBigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        ok(context->cbCertEncoded == sizeof(signedBigCert),
         "Expected cert of %d bytes, got %ld\n", sizeof(signedBigCert),
         context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, signedBigCert,
         sizeof(signedBigCert)), "Unexpected encoded cert in context\n");
        /* remove it, the rest of the tests will work on an unsigned cert */
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08lx\n",
         GetLastError());
    }
    /* try adding a "signed" CRL as a cert */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     signedCRL, sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, &context);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_BADTAG || GetLastError() ==
     CRYPT_E_ASN1_CORRUPT),
     "Expected CRYPT_E_ASN1_BADTAG or CRYPT_E_ASN1_CORRUPT, got %08lx\n",
     GetLastError());
    /* add a cert to store1 */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        DWORD size;
        BYTE *buf;

        ok(context->cbCertEncoded == sizeof(bigCert),
         "Expected cert of %d bytes, got %ld\n", sizeof(bigCert),
         context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, sizeof(bigCert)),
         "Unexpected encoded cert in context\n");
        ok(context->hCertStore == store1, "Unexpected store\n");

        /* check serializing this element */
        /* These crash
        ret = CertSerializeCertificateStoreElement(NULL, 0, NULL, NULL);
        ret = CertSerializeCertificateStoreElement(context, 0, NULL, NULL);
        ret = CertSerializeCertificateStoreElement(NULL, 0, NULL, &size);
         */
        /* apparently flags are ignored */
        ret = CertSerializeCertificateStoreElement(context, 1, NULL, &size);
        ok(ret, "CertSerializeCertificateStoreElement failed: %08lx\n",
         GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(context, 0, buf, &size);
            ok(size == sizeof(serializedCert), "Expected size %d, got %ld\n",
             sizeof(serializedCert), size);
            ok(!memcmp(serializedCert, buf, size),
             "Unexpected serialized cert\n");
            HeapFree(GetProcessHeap(), 0, buf);
        }

        ret = CertFreeCertificateContext(context);
        ok(ret, "CertFreeCertificateContext failed: %08lx\n", GetLastError());
    }
    /* verify the cert's in store1 */
    context = CertEnumCertificatesInStore(store1, NULL);
    ok(context != NULL, "Expected a valid context\n");
    context = CertEnumCertificatesInStore(store1, context);
    ok(!context && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
    /* verify store2 (the "open existing" mem store) is still empty */
    context = CertEnumCertificatesInStore(store2, NULL);
    ok(!context, "Expected an empty store\n");
    /* delete the cert from store1, and check it's empty */
    context = CertEnumCertificatesInStore(store1, NULL);
    if (context)
    {
        /* Deleting a bitwise copy crashes with an access to an uninitialized
         * pointer, so a cert context has some special data out there in memory
         * someplace
        CERT_CONTEXT copy;
        memcpy(&copy, context, sizeof(copy));
        ret = CertDeleteCertificateFromStore(&copy);
         */
        PCCERT_CONTEXT copy = CertDuplicateCertificateContext(context);

        ok(copy != NULL, "CertDuplicateCertificateContext failed: %08lx\n",
         GetLastError());
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08lx\n",
         GetLastError());
        /* try deleting a copy */
        ret = CertDeleteCertificateFromStore(copy);
        ok(ret, "CertDeleteCertificateFromStore failed: %08lx\n",
         GetLastError());
        /* check that the store is empty */
        context = CertEnumCertificatesInStore(store1, NULL);
        ok(!context, "Expected an empty store\n");
    }

    /* close an empty store */
    ret = CertCloseStore(NULL, 0);
    ok(ret, "CertCloseStore failed: %ld\n", GetLastError());
    ret = CertCloseStore(store1, 0);
    ok(ret, "CertCloseStore failed: %ld\n", GetLastError());
    ret = CertCloseStore(store2, 0);
    ok(ret, "CertCloseStore failed: %ld\n", GetLastError());

    /* This seems nonsensical, but you can open a read-only mem store, only
     * it isn't read-only
     */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_READONLY_FLAG, NULL);
    ok(store1 != NULL, "CertOpenStore failed: %ld\n", GetLastError());
    /* yep, this succeeds */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Expected cert of %d bytes, got %ld\n", sizeof(bigCert),
         context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, sizeof(bigCert)),
         "Unexpected encoded cert in context\n");
        ok(context->hCertStore == store1, "Unexpected store\n");
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08lx\n",
         GetLastError());
    }
    CertCloseStore(store1, 0);
}

static void testCollectionStore(void)
{
    HCERTSTORE store1, store2, collection, collection2;
    PCCERT_CONTEXT context;
    BOOL ret;

    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);

    /* Try adding a cert to any empty collection */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_ACCESSDENIED,
     "Expected E_ACCESSDENIED, got %08lx\n", GetLastError());

    /* Create and add a cert to a memory store */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    /* Add the memory store to the collection, without allowing adding */
    ret = CertAddStoreToCollection(collection, store1, 0, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    /* Verify the cert is in the collection */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        CertFreeCertificateContext(context);
    }
    /* Check that adding to the collection isn't allowed */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
    ok(!ret && GetLastError() == E_ACCESSDENIED,
     "Expected E_ACCESSDENIED, got %08lx\n", GetLastError());

    /* Create a new memory store */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    /* Try adding a store to a non-collection store */
    ret = CertAddStoreToCollection(store1, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Try adding some bogus stores */
    /* This crashes in Windows
    ret = CertAddStoreToCollection(0, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
     */
    /* This "succeeds"... */
    ret = CertAddStoreToCollection(collection, 0,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    /* while this crashes.
    ret = CertAddStoreToCollection(collection, 1,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
     */

    /* Add it to the collection, this time allowing adding */
    ret = CertAddStoreToCollection(collection, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    /* Check that adding to the collection is allowed */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    /* Now check that it was actually added to store2 */
    context = CertEnumCertificatesInStore(store2, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == store2, "Unexpected store\n");
        CertFreeCertificateContext(context);
    }
    /* Check that the collection has both bigCert and bigCert2.  bigCert comes
     * first because store1 was added first.
     */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Expected size %d, got %ld\n", sizeof(bigCert),
         context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Expected size %d, got %ld\n", sizeof(bigCert2),
             context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert2,
             context->cbCertEncoded), "Unexpected cert\n");
            context = CertEnumCertificatesInStore(collection, context);
            ok(!context, "Unexpected cert\n");
        }
    }
    /* close store2, and check that the collection is unmodified */
    CertCloseStore(store2, 0);
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Expected size %d, got %ld\n", sizeof(bigCert),
         context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Expected size %d, got %ld\n", sizeof(bigCert2),
             context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert2,
             context->cbCertEncoded), "Unexpected cert\n");
            context = CertEnumCertificatesInStore(collection, context);
            ok(!context, "Unexpected cert\n");
        }
    }

    /* Adding a collection to a collection is legal */
    collection2 = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddStoreToCollection(collection2, collection,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    /* check the contents of collection2 */
    context = CertEnumCertificatesInStore(collection2, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection2, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Expected size %d, got %ld\n", sizeof(bigCert),
         context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection2, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection2, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Expected size %d, got %ld\n", sizeof(bigCert2),
             context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert2,
             context->cbCertEncoded), "Unexpected cert\n");
            context = CertEnumCertificatesInStore(collection2, context);
            ok(!context, "Unexpected cert\n");
        }
    }

    /* I'd like to test closing the collection in the middle of enumeration,
     * but my tests have been inconsistent.  The first time calling
     * CertEnumCertificatesInStore on a closed collection succeeded, while the
     * second crashed.  So anything appears to be fair game.
     * I'd also like to test removing a store from a collection in the middle
     * of an enumeration, but my tests in Windows have been inconclusive.
     * In one scenario it worked.  In another scenario, about a third of the
     * time this leads to "random" crashes elsewhere in the code.  This
     * probably means this is not allowed.
     */

    CertCloseStore(store1, 0);
    CertCloseStore(collection, 0);
    CertCloseStore(collection2, 0);

    /* Add the same cert to two memory stores, then put them in a collection */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store1 != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store2 != 0, "CertOpenStore failed: %08lx\n", GetLastError());

    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    ret = CertAddEncodedCertificateToStore(store2, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n", GetLastError());
    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(collection != 0, "CertOpenStore failed: %08lx\n", GetLastError());

    ret = CertAddStoreToCollection(collection, store1,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
    ret = CertAddStoreToCollection(collection, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());

    /* Check that the collection has two copies of the same cert */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Expected size %d, got %ld\n", sizeof(bigCert),
         context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert),
             "Expected size %d, got %ld\n", sizeof(bigCert),
             context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
             "Unexpected cert\n");
            context = CertEnumCertificatesInStore(collection, context);
            ok(context == NULL, "Unexpected cert\n");
        }
    }

    /* The following would check whether I can delete an identical cert, rather
     * than one enumerated from the store.  It crashes, so that means I must
     * only call CertDeleteCertificateFromStore with contexts enumerated from
     * the store.
    context = CertCreateCertificateContext(X509_ASN_ENCODING, bigCert,
     sizeof(bigCert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        ret = CertDeleteCertificateFromStore(collection, context);
        printf("ret is %d, GetLastError is %08lx\n", ret, GetLastError());
        CertFreeCertificateContext(context);
    }
     */

    /* Now check deleting from the collection. */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        CertDeleteCertificateFromStore(context);
        /* store1 should now be empty */
        context = CertEnumCertificatesInStore(store1, NULL);
        ok(!context, "Unexpected cert\n");
        /* and there should be one certificate in the collection */
        context = CertEnumCertificatesInStore(collection, NULL);
        ok(context != NULL, "Expected a valid cert\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert),
             "Expected size %d, got %ld\n", sizeof(bigCert),
             context->cbCertEncoded);
            ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
             "Unexpected cert\n");
        }
        context = CertEnumCertificatesInStore(collection, context);
        ok(context == NULL, "Unexpected cert\n");
    }

    /* Finally, test removing stores from the collection.  No return value, so
     * it's a bit funny to test.
     */
    /* This crashes
    CertRemoveStoreFromCollection(NULL, NULL);
     */
    /* This "succeeds," no crash, no last error set */
    SetLastError(0xdeadbeef);
    CertRemoveStoreFromCollection(store2, collection);
    ok(GetLastError() == 0xdeadbeef,
     "Didn't expect an error to be set: %08lx\n", GetLastError());

    /* After removing store2, the collection should be empty */
    SetLastError(0xdeadbeef);
    CertRemoveStoreFromCollection(collection, store2);
    ok(GetLastError() == 0xdeadbeef,
     "Didn't expect an error to be set: %08lx\n", GetLastError());
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(!context, "Unexpected cert\n");

    CertCloseStore(collection, 0);
    CertCloseStore(store2, 0);
    CertCloseStore(store1, 0);
}

/* Looks for the property with ID propID in the buffer buf.  Returns a pointer
 * to its header if found, NULL if not.
 */
static const struct CertPropIDHeader *findPropID(const BYTE *buf, DWORD size,
 DWORD propID)
{
    const struct CertPropIDHeader *ret = NULL;
    BOOL failed = FALSE;

    while (size && !ret && !failed)
    {
        if (size < sizeof(struct CertPropIDHeader))
            failed = TRUE;
        else
        {
            const struct CertPropIDHeader *hdr =
             (const struct CertPropIDHeader *)buf;

            size -= sizeof(struct CertPropIDHeader);
            buf += sizeof(struct CertPropIDHeader);
            if (size < hdr->cb)
                failed = TRUE;
            else if (hdr->propID == propID)
                ret = hdr;
            else
            {
                buf += hdr->cb;
                size -= hdr->cb;
            }
        }
    }
    return ret;
}

typedef DWORD (WINAPI *SHDeleteKeyAFunc)(HKEY, LPCSTR);

static void testRegStore(void)
{
    static const char tempKey[] = "Software\\Wine\\CryptTemp";
    HCERTSTORE store;
    LONG rc;
    HKEY key = NULL;
    DWORD disp;

    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, NULL);
    ok(!store && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
    ok(!store && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    /* Opening up any old key works.. */
    key = HKEY_CURRENT_USER;
    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
    /* Not sure if this is a bug in DuplicateHandle, marking todo_wine for now
     */
    todo_wine ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    CertCloseStore(store, 0);

    rc = RegCreateKeyExA(HKEY_CURRENT_USER, tempKey, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, NULL);
    ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
    if (key)
    {
        BOOL ret;
        BYTE hash[20];
        DWORD size, i;
        static const char certificates[] = "Certificates\\";
        char subKeyName[sizeof(certificates) + 20 * 2 + 1], *ptr;
        HKEY subKey;
        PCCERT_CONTEXT context;

        store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
        ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
        /* Add a certificate.  It isn't persisted right away, since it's only
         * added to the cache..
         */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08lx\n",
         GetLastError());
        /* so flush the cache to force a commit.. */
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %08lx\n", GetLastError());
        /* and check that the expected subkey was written. */
        size = sizeof(hash);
        ret = CryptHashCertificate(0, 0, 0, bigCert2, sizeof(bigCert2),
         hash, &size);
        ok(ret, "CryptHashCertificate failed: %ld\n", GetLastError());
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1; i < size;
         i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
        if (subKey)
        {
            LPBYTE buf;

            size = 0;
            RegQueryValueExA(subKey, "Blob", NULL, NULL, NULL, &size);
            buf = HeapAlloc(GetProcessHeap(), 0, size);
            if (buf)
            {
                rc = RegQueryValueExA(subKey, "Blob", NULL, NULL, buf, &size);
                ok(!rc, "RegQueryValueExA failed: %ld\n", rc);
                if (!rc)
                {
                    const struct CertPropIDHeader *hdr;

                    /* Both the hash and the cert should be present */
                    hdr = findPropID(buf, size, CERT_CERT_PROP_ID);
                    ok(hdr != NULL, "Expected to find a cert property\n");
                    if (hdr)
                    {
                        ok(hdr->cb == sizeof(bigCert2),
                         "Unexpected size %ld of cert property, expected %d\n",
                         hdr->cb, sizeof(bigCert2));
                        ok(!memcmp((BYTE *)hdr + sizeof(*hdr), bigCert2,
                         hdr->cb), "Unexpected cert in cert property\n");
                    }
                    hdr = findPropID(buf, size, CERT_HASH_PROP_ID);
                    ok(hdr != NULL, "Expected to find a hash property\n");
                    if (hdr)
                    {
                        ok(hdr->cb == sizeof(hash),
                         "Unexpected size %ld of hash property, expected %d\n",
                         hdr->cb, sizeof(hash));
                        ok(!memcmp((BYTE *)hdr + sizeof(*hdr), hash,
                         hdr->cb), "Unexpected hash in cert property\n");
                    }
                }
                HeapFree(GetProcessHeap(), 0, buf);
            }
            RegCloseKey(subKey);
        }

        /* Remove the existing context */
        context = CertEnumCertificatesInStore(store, NULL);
        ok(context != NULL, "Expected a cert context\n");
        if (context)
            CertDeleteCertificateFromStore(context);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

        /* Add a serialized cert with a bogus hash directly to the registry */
        memset(hash, 0, sizeof(hash));
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1;
         i < sizeof(hash); i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
        if (subKey)
        {
            BYTE buf[sizeof(struct CertPropIDHeader) * 2 + sizeof(hash) +
             sizeof(bigCert)], *ptr;
            DWORD certCount = 0;
            struct CertPropIDHeader *hdr;

            hdr = (struct CertPropIDHeader *)buf;
            hdr->propID = CERT_HASH_PROP_ID;
            hdr->unknown1 = 1;
            hdr->cb = sizeof(hash);
            ptr = buf + sizeof(*hdr);
            memcpy(ptr, hash, sizeof(hash));
            ptr += sizeof(hash);
            hdr = (struct CertPropIDHeader *)ptr;
            hdr->propID = CERT_CERT_PROP_ID;
            hdr->unknown1 = 1;
            hdr->cb = sizeof(bigCert);
            ptr += sizeof(*hdr);
            memcpy(ptr, bigCert, sizeof(bigCert));

            rc = RegSetValueExA(subKey, "Blob", 0, REG_BINARY, buf,
             sizeof(buf));
            ok(!rc, "RegSetValueExA failed: %ld\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

            /* Make sure the bogus hash cert gets loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 1, "Expected 1 certificates, got %ld\n", certCount);

            RegCloseKey(subKey);
        }

        /* Add another serialized cert directly to the registry, this time
         * under the correct key name (named with the correct hash value).
         */
        size = sizeof(hash);
        ret = CryptHashCertificate(0, 0, 0, bigCert2,
         sizeof(bigCert2), hash, &size);
        ok(ret, "CryptHashCertificate failed: %ld\n", GetLastError());
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1;
         i < sizeof(hash); i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
        if (subKey)
        {
            BYTE buf[sizeof(struct CertPropIDHeader) * 2 + sizeof(hash) +
             sizeof(bigCert2)], *ptr;
            DWORD certCount = 0;
            PCCERT_CONTEXT context;
            struct CertPropIDHeader *hdr;

            /* First try with a bogus hash... */
            hdr = (struct CertPropIDHeader *)buf;
            hdr->propID = CERT_HASH_PROP_ID;
            hdr->unknown1 = 1;
            hdr->cb = sizeof(hash);
            ptr = buf + sizeof(*hdr);
            memset(ptr, 0, sizeof(hash));
            ptr += sizeof(hash);
            hdr = (struct CertPropIDHeader *)ptr;
            hdr->propID = CERT_CERT_PROP_ID;
            hdr->unknown1 = 1;
            hdr->cb = sizeof(bigCert2);
            ptr += sizeof(*hdr);
            memcpy(ptr, bigCert2, sizeof(bigCert2));

            rc = RegSetValueExA(subKey, "Blob", 0, REG_BINARY, buf,
             sizeof(buf));
            ok(!rc, "RegSetValueExA failed: %ld\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

            /* and make sure just one cert still gets loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 1, "Expected 1 certificates, got %ld\n", certCount);

            /* Try again with the correct hash... */
            ptr = buf + sizeof(*hdr);
            memcpy(ptr, hash, sizeof(hash));

            rc = RegSetValueExA(subKey, "Blob", 0, REG_BINARY, buf,
             sizeof(buf));
            ok(!rc, "RegSetValueExA failed: %ld\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08lx\n", GetLastError());

            /* and make sure two certs get loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 2, "Expected 2 certificates, got %ld\n", certCount);

            RegCloseKey(subKey);
        }
        CertCloseStore(store, 0);
        /* Is delete allowed on a reg store? */
        store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0,
         CERT_STORE_DELETE_FLAG, key);
        ok(store == NULL, "Expected NULL return from CERT_STORE_DELETE_FLAG\n");
        ok(GetLastError() == 0, "CertOpenStore failed: %08lx\n",
         GetLastError());

        RegCloseKey(key);
    }
    /* The CertOpenStore with CERT_STORE_DELETE_FLAG above will delete the
     * contents of the key, but not the key itself.
     */
    rc = RegCreateKeyExA(HKEY_CURRENT_USER, tempKey, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, &disp);
    ok(!rc, "RegCreateKeyExA failed: %ld\n", rc);
    ok(disp == REG_OPENED_EXISTING_KEY,
     "Expected REG_OPENED_EXISTING_KEY, got %ld\n", disp);
    if (!rc)
    {
        RegCloseKey(key);
        rc = RegDeleteKeyA(HKEY_CURRENT_USER, tempKey);
        if (rc)
        {
            HMODULE shlwapi = LoadLibraryA("shlwapi");

            /* Use shlwapi's SHDeleteKeyA to _really_ blow away the key,
             * otherwise subsequent tests will fail.
             */
            if (shlwapi)
            {
                SHDeleteKeyAFunc pSHDeleteKeyA =
                 (SHDeleteKeyAFunc)GetProcAddress(shlwapi, "SHDeleteKeyA");

                if (pSHDeleteKeyA)
                    pSHDeleteKeyA(HKEY_CURRENT_USER, tempKey);
                FreeLibrary(shlwapi);
            }
        }
    }
}

static const char MyA[] = { 'M','y',0,0 };
static const WCHAR MyW[] = { 'M','y',0 };
static const WCHAR BogusW[] = { 'B','o','g','u','s',0 };
static const WCHAR BogusPathW[] = { 'S','o','f','t','w','a','r','e','\\',
 'M','i','c','r','o','s','o','f','t','\\','S','y','s','t','e','m','C','e','r',
 't','i','f','i','c','a','t','e','s','\\','B','o','g','u','s',0 };

static void testSystemRegStore(void)
{
    HCERTSTORE store, memStore;

    /* Check with a UNICODE name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyW);
    /* Not all OSes support CERT_STORE_PROV_SYSTEM_REGISTRY, so don't continue
     * testing if they don't.
     */
    if (!store)
        return;

    /* Check that it isn't a collection store */
    memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    if (memStore)
    {
        BOOL ret = CertAddStoreToCollection(store, memStore, 0, 0);

        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        CertCloseStore(memStore, 0);
    }
    CertCloseStore(store, 0);

    /* Check opening a bogus store */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, BogusW);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, BogusW);
    ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Now check whether deleting is allowed */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);

    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0, 0, NULL);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyA);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyW);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* The name is expected to be UNICODE, check with an ASCII name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
}

static void testSystemStore(void)
{
    static const WCHAR baskslashW[] = { '\\',0 };
    HCERTSTORE store;
    WCHAR keyName[MAX_PATH];
    HKEY key;
    LONG rc;

    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, 0, NULL);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyW);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    /* The name is expected to be UNICODE, first check with an ASCII name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    /* Create the expected key */
    lstrcpyW(keyName, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH);
    lstrcatW(keyName, baskslashW);
    lstrcatW(keyName, MyW);
    rc = RegCreateKeyExW(HKEY_CURRENT_USER, keyName, 0, NULL, 0, KEY_READ,
     NULL, &key, NULL);
    ok(!rc, "RegCreateKeyEx failed: %ld\n", rc);
    if (!rc)
        RegCloseKey(key);
    /* Check opening with a UNICODE name, specifying the create new flag */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_CREATE_NEW_FLAG, MyW);
    ok(!store && GetLastError() == ERROR_FILE_EXISTS,
     "Expected ERROR_FILE_EXISTS, got %08lx\n", GetLastError());
    /* Now check opening with a UNICODE name, this time opening existing */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyW);
    ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
    {
        HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        /* Check that it's a collection store */
        if (memStore)
        {
            BOOL ret = CertAddStoreToCollection(store, memStore, 0, 0);

            /* FIXME: this'll fail on NT4, but what error will it give? */
            ok(ret, "CertAddStoreToCollection failed: %08lx\n", GetLastError());
            CertCloseStore(memStore, 0);
        }
        CertCloseStore(store, 0);
    }

    /* Check opening a bogus store */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, BogusW);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, BogusW);
    ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Now check whether deleting is allowed */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);
}

static void testCertOpenSystemStore(void)
{
    HCERTSTORE store;

    store = CertOpenSystemStoreW(0, NULL);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* This succeeds, and on WinXP at least, the Bogus key is created under
     * HKCU (but not under HKLM, even when run as an administrator.)
     */
    store = CertOpenSystemStoreW(0, BogusW);
    ok(store != 0, "CertOpenSystemStore failed: %08lx\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Delete it so other tests succeed next time around */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);
}

static void checkHash(const BYTE *data, DWORD dataLen, ALG_ID algID,
 PCCERT_CONTEXT context, DWORD propID)
{
    BYTE hash[20] = { 0 }, hashProperty[20];
    BOOL ret;
    DWORD size;

    memset(hash, 0, sizeof(hash));
    memset(hashProperty, 0, sizeof(hashProperty));
    size = sizeof(hash);
    ret = CryptHashCertificate(0, algID, 0, data, dataLen, hash, &size);
    ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
    ret = CertGetCertificateContextProperty(context, propID, hashProperty,
     &size);
    ok(ret, "CertGetCertificateContextProperty failed: %08lx\n",
     GetLastError());
    ok(!memcmp(hash, hashProperty, size), "Unexpected hash for property %ld\n",
     propID);
}

static void testCertProperties(void)
{
    PCCERT_CONTEXT context = CertCreateCertificateContext(X509_ASN_ENCODING,
     bigCert, sizeof(bigCert));

    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        DWORD propID, numProps, access, size;
        BOOL ret;
        BYTE hash[20] = { 0 }, hashProperty[20];
        CRYPT_DATA_BLOB blob;

        /* This crashes
        propID = CertEnumCertificateContextProperties(NULL, 0);
         */

        propID = 0;
        numProps = 0;
        do {
            propID = CertEnumCertificateContextProperties(context, propID);
            if (propID)
                numProps++;
        } while (propID != 0);
        ok(numProps == 0, "Expected 0 properties, got %ld\n", numProps);

        /* Tests with a NULL cert context.  Prop ID 0 fails.. */
        ret = CertSetCertificateContextProperty(NULL, 0, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        /* while this just crashes.
        ret = CertSetCertificateContextProperty(NULL,
         CERT_KEY_PROV_HANDLE_PROP_ID, 0, NULL);
         */

        ret = CertSetCertificateContextProperty(context, 0, 0, NULL);
        ok(!ret && GetLastError() == E_INVALIDARG,
         "Expected E_INVALIDARG, got %08lx\n", GetLastError());
        /* Can't set the cert property directly, this crashes.
        ret = CertSetCertificateContextProperty(context,
         CERT_CERT_PROP_ID, 0, bigCert2);
         */

        /* These all crash.
        ret = CertGetCertificateContextProperty(context,
         CERT_ACCESS_STATE_PROP_ID, 0, NULL);
        ret = CertGetCertificateContextProperty(context, CERT_HASH_PROP_ID, 
         NULL, NULL);
        ret = CertGetCertificateContextProperty(context, CERT_HASH_PROP_ID, 
         hashProperty, NULL);
         */
        /* A missing prop */
        size = 0;
        ret = CertGetCertificateContextProperty(context,
         CERT_KEY_PROV_INFO_PROP_ID, NULL, &size);
        ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
         "Expected CRYPT_E_NOT_FOUND, got %08lx\n", GetLastError());
        /* And, an implicit property */
        size = sizeof(access);
        ret = CertGetCertificateContextProperty(context,
         CERT_ACCESS_STATE_PROP_ID, &access, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08lx\n",
         GetLastError());
        ok(!(access & CERT_ACCESS_STATE_WRITE_PERSIST_FLAG),
         "Didn't expect a persisted cert\n");
        /* Trying to set this "read only" property crashes.
        access |= CERT_ACCESS_STATE_WRITE_PERSIST_FLAG;
        ret = CertSetCertificateContextProperty(context,
         CERT_ACCESS_STATE_PROP_ID, 0, &access);
         */

        /* Can I set the hash to an invalid hash? */
        blob.pbData = hash;
        blob.cbData = sizeof(hash);
        ret = CertSetCertificateContextProperty(context, CERT_HASH_PROP_ID, 0,
         &blob);
        ok(ret, "CertSetCertificateContextProperty failed: %08lx\n",
         GetLastError());
        size = sizeof(hashProperty);
        ret = CertGetCertificateContextProperty(context, CERT_HASH_PROP_ID,
         hashProperty, &size);
        ok(!memcmp(hashProperty, hash, sizeof(hash)), "Unexpected hash\n");
        /* Delete the (bogus) hash, and get the real one */
        ret = CertSetCertificateContextProperty(context, CERT_HASH_PROP_ID, 0,
         NULL);
        ok(ret, "CertSetCertificateContextProperty failed: %08lx\n",
         GetLastError());
        checkHash(bigCert, sizeof(bigCert), CALG_SHA1, context,
         CERT_HASH_PROP_ID);

        /* Now that the hash property is set, we should get one property when
         * enumerating.
         */
        propID = 0;
        numProps = 0;
        do {
            propID = CertEnumCertificateContextProperties(context, propID);
            if (propID)
                numProps++;
        } while (propID != 0);
        ok(numProps == 1, "Expected 1 properties, got %ld\n", numProps);

        /* Check a few other implicit properties */
        checkHash(bigCert, sizeof(bigCert), CALG_MD5, context,
         CERT_MD5_HASH_PROP_ID);
        checkHash(
         context->pCertInfo->Subject.pbData,
         context->pCertInfo->Subject.cbData,
         CALG_MD5, context, CERT_SUBJECT_NAME_MD5_HASH_PROP_ID);
        checkHash(
         context->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
         context->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
         CALG_MD5, context, CERT_SUBJECT_PUBLIC_KEY_MD5_HASH_PROP_ID);

        /* Odd: this doesn't fail on other certificates, so there must be
         * something weird about this cert that causes it to fail.
         */
        size = 0;
        ret = CertGetCertificateContextProperty(context,
         CERT_KEY_IDENTIFIER_PROP_ID, NULL, &size);
        todo_wine ok(!ret && GetLastError() == ERROR_INVALID_DATA,
         "Expected ERROR_INVALID_DATA, got %08lx\n", GetLastError());

        CertFreeCertificateContext(context);
    }
}

static void testAddSerialized(void)
{
    BOOL ret;
    HCERTSTORE store;
    BYTE buf[sizeof(struct CertPropIDHeader) * 2 + 20 + sizeof(bigCert)] =
     { 0 };
    BYTE hash[20];
    struct CertPropIDHeader *hdr;
    PCCERT_CONTEXT context;

    ret = CertAddSerializedElementToStore(0, NULL, 0, 0, 0, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_END_OF_MEDIA,
     "Expected ERROR_END_OF_MEDIA, got %08lx\n", GetLastError());

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != 0, "CertOpenStore failed: %08lx\n", GetLastError());

    ret = CertAddSerializedElementToStore(store, NULL, 0, 0, 0, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_END_OF_MEDIA,
     "Expected ERROR_END_OF_MEDIA, got %08lx\n", GetLastError());

    /* Test with an empty property */
    hdr = (struct CertPropIDHeader *)buf;
    hdr->propID = CERT_CERT_PROP_ID;
    hdr->unknown1 = 1;
    hdr->cb = 0;
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Test with a bad size in property header */
    hdr->cb = sizeof(bigCert) - 1;
    memcpy(buf + sizeof(struct CertPropIDHeader), bigCert, sizeof(bigCert));
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 0, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, 0, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Kosher size in property header, but no context type */
    hdr->cb = sizeof(bigCert);
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 0, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, 0, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* With a bad context type */
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 
     CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 
     CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08lx\n", GetLastError());
    /* Bad unknown field, good type */
    hdr->unknown1 = 2;
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08lx\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08lx\n", GetLastError());
    /* Most everything okay, but bad add disposition */
    hdr->unknown1 = 1;
    /* This crashes
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
     * as does this
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
     */
    /* Everything okay, but buffer's too big */
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf),
     CERT_STORE_ADD_NEW, 0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(ret, "CertAddSerializedElementToStore failed: %08lx\n", GetLastError());
    /* Everything okay, check it's not re-added */
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == CRYPT_E_EXISTS,
     "Expected CRYPT_E_EXISTS, got %08lx\n", GetLastError());

    context = CertEnumCertificatesInStore(store, NULL);
    ok(context != NULL, "Expected a cert\n");
    if (context)
        CertDeleteCertificateFromStore(context);

    /* Try adding with a bogus hash.  Oddly enough, it succeeds, and the hash,
     * when queried, is the real hash rather than the bogus hash.
     */
    hdr = (struct CertPropIDHeader *)(buf + sizeof(struct CertPropIDHeader) +
     sizeof(bigCert));
    hdr->propID = CERT_HASH_PROP_ID;
    hdr->unknown1 = 1;
    hdr->cb = sizeof(hash);
    memset(hash, 0xc, sizeof(hash));
    memcpy((LPBYTE)hdr + sizeof(struct CertPropIDHeader), hash, sizeof(hash));
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf),
     CERT_STORE_ADD_NEW, 0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL,
     (const void **)&context);
    ok(ret, "CertAddSerializedElementToStore failed: %08lx\n", GetLastError());
    if (context)
    {
        BYTE hashVal[20], realHash[20];
        DWORD size = sizeof(hashVal);

        ret = CryptHashCertificate(0, 0, 0, bigCert, sizeof(bigCert),
         realHash, &size);
        ok(ret, "CryptHashCertificate failed: %08lx\n", GetLastError());
        ret = CertGetCertificateContextProperty(context, CERT_HASH_PROP_ID,
         hashVal, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08lx\n",
         GetLastError());
        ok(!memcmp(hashVal, realHash, size), "Unexpected hash\n");
        CertFreeCertificateContext(context);
    }

    CertCloseStore(store, 0);
}

START_TEST(store)
{
    testAddCert();
    testDupCert();
    testFindCert();
    testGetSubjectCert();
    testGetIssuerCert();

    /* various combinations of CertOpenStore */
    testMemStore();
    testCollectionStore();
    testRegStore();
    testSystemRegStore();
    testSystemStore();

    testCertOpenSystemStore();

    testCertProperties();
    testAddSerialized();
}
