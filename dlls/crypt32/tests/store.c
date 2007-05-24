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

static void testMemStore(void)
{
    HCERTSTORE store1, store2;
    PCCERT_CONTEXT context;
    BOOL ret;

    /* NULL provider */
    store1 = CertOpenStore(0, 0, 0, 0, NULL);
    ok(!store1 && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
    /* weird flags */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_DELETE_FLAG, NULL);
    ok(!store1 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
     "Expected ERROR_CALL_NOT_IMPLEMENTED, got %d\n", GetLastError());

    /* normal */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store1 != NULL, "CertOpenStore failed: %d\n", GetLastError());
    /* open existing doesn't */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG, NULL);
    ok(store2 != NULL, "CertOpenStore failed: %d\n", GetLastError());
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
     "Expected CRYPT_E_ASN1_EOD or CRYPT_E_ASN1_CORRUPT, got %08x\n",
     GetLastError());
    /* add a "signed" cert--the signature isn't a real signature, so this adds
     * without any check of the signature's validity
     */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     signedBigCert, sizeof(signedBigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        ok(context->cbCertEncoded == sizeof(signedBigCert),
         "Wrong cert size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, signedBigCert,
         sizeof(signedBigCert)), "Unexpected encoded cert in context\n");
        /* remove it, the rest of the tests will work on an unsigned cert */
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08x\n",
         GetLastError());
    }
    /* try adding a "signed" CRL as a cert */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     signedCRL, sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, &context);
    ok(!ret && (GetLastError() == CRYPT_E_ASN1_BADTAG || GetLastError() ==
     CRYPT_E_ASN1_CORRUPT),
     "Expected CRYPT_E_ASN1_BADTAG or CRYPT_E_ASN1_CORRUPT, got %08x\n",
     GetLastError());
    /* add a cert to store1 */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        DWORD size;
        BYTE *buf;

        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong cert size %d\n", context->cbCertEncoded);
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
        ok(ret, "CertSerializeCertificateStoreElement failed: %08x\n",
         GetLastError());
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (buf)
        {
            ret = CertSerializeCertificateStoreElement(context, 0, buf, &size);
            ok(size == sizeof(serializedCert), "Wrong size %d\n", size);
            ok(!memcmp(serializedCert, buf, size),
             "Unexpected serialized cert\n");
            HeapFree(GetProcessHeap(), 0, buf);
        }

        ret = CertFreeCertificateContext(context);
        ok(ret, "CertFreeCertificateContext failed: %08x\n", GetLastError());
    }
    /* verify the cert's in store1 */
    context = CertEnumCertificatesInStore(store1, NULL);
    ok(context != NULL, "Expected a valid context\n");
    context = CertEnumCertificatesInStore(store1, context);
    ok(!context && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
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

        ok(copy != NULL, "CertDuplicateCertificateContext failed: %08x\n",
         GetLastError());
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08x\n",
         GetLastError());
        /* try deleting a copy */
        ret = CertDeleteCertificateFromStore(copy);
        ok(ret, "CertDeleteCertificateFromStore failed: %08x\n",
         GetLastError());
        /* check that the store is empty */
        context = CertEnumCertificatesInStore(store1, NULL);
        ok(!context, "Expected an empty store\n");
    }

    /* close an empty store */
    ret = CertCloseStore(NULL, 0);
    ok(ret, "CertCloseStore failed: %d\n", GetLastError());
    ret = CertCloseStore(store1, 0);
    ok(ret, "CertCloseStore failed: %d\n", GetLastError());
    ret = CertCloseStore(store2, 0);
    ok(ret, "CertCloseStore failed: %d\n", GetLastError());

    /* This seems nonsensical, but you can open a read-only mem store, only
     * it isn't read-only
     */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_READONLY_FLAG, NULL);
    ok(store1 != NULL, "CertOpenStore failed: %d\n", GetLastError());
    /* yep, this succeeds */
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING, bigCert,
     sizeof(bigCert), CERT_STORE_ADD_ALWAYS, &context);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    ok(context != NULL, "Expected a valid cert context\n");
    if (context)
    {
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong cert size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, sizeof(bigCert)),
         "Unexpected encoded cert in context\n");
        ok(context->hCertStore == store1, "Unexpected store\n");
        ret = CertDeleteCertificateFromStore(context);
        ok(ret, "CertDeleteCertificateFromStore failed: %08x\n",
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
     "Expected E_ACCESSDENIED, got %08x\n", GetLastError());

    /* Create and add a cert to a memory store */
    store1 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    /* Add the memory store to the collection, without allowing adding */
    ret = CertAddStoreToCollection(collection, store1, 0, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
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
     "Expected E_ACCESSDENIED, got %08x\n", GetLastError());

    /* Create a new memory store */
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    /* Try adding a store to a non-collection store */
    ret = CertAddStoreToCollection(store1, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Try adding some bogus stores */
    /* This crashes in Windows
    ret = CertAddStoreToCollection(0, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
     */
    /* This "succeeds"... */
    ret = CertAddStoreToCollection(collection, 0,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    /* while this crashes.
    ret = CertAddStoreToCollection(collection, 1,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
     */

    /* Add it to the collection, this time allowing adding */
    ret = CertAddStoreToCollection(collection, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    /* Check that adding to the collection is allowed */
    ret = CertAddEncodedCertificateToStore(collection, X509_ASN_ENCODING,
     bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
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
         "Wrong size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %d\n", context->cbCertEncoded);
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
         "Wrong size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %d\n", context->cbCertEncoded);
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
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    /* check the contents of collection2 */
    context = CertEnumCertificatesInStore(collection2, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection2, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection2, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection2, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert2),
             "Wrong size %d\n", context->cbCertEncoded);
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
    ok(store1 != 0, "CertOpenStore failed: %08x\n", GetLastError());
    store2 = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store2 != 0, "CertOpenStore failed: %08x\n", GetLastError());

    ret = CertAddEncodedCertificateToStore(store1, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    ret = CertAddEncodedCertificateToStore(store2, X509_ASN_ENCODING,
     bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
    ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n", GetLastError());
    collection = CertOpenStore(CERT_STORE_PROV_COLLECTION, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(collection != 0, "CertOpenStore failed: %08x\n", GetLastError());

    ret = CertAddStoreToCollection(collection, store1,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
    ret = CertAddStoreToCollection(collection, store2,
     CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG, 0);
    ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());

    /* Check that the collection has two copies of the same cert */
    context = CertEnumCertificatesInStore(collection, NULL);
    ok(context != NULL, "Expected a valid context\n");
    if (context)
    {
        ok(context->hCertStore == collection, "Unexpected store\n");
        ok(context->cbCertEncoded == sizeof(bigCert),
         "Wrong size %d\n", context->cbCertEncoded);
        ok(!memcmp(context->pbCertEncoded, bigCert, context->cbCertEncoded),
         "Unexpected cert\n");
        context = CertEnumCertificatesInStore(collection, context);
        ok(context != NULL, "Expected a valid context\n");
        if (context)
        {
            ok(context->hCertStore == collection, "Unexpected store\n");
            ok(context->cbCertEncoded == sizeof(bigCert),
             "Wrong size %d\n", context->cbCertEncoded);
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
    ok(context != NULL, "CertCreateCertificateContext failed: %08x\n",
     GetLastError());
    if (context)
    {
        ret = CertDeleteCertificateFromStore(collection, context);
        printf("ret is %d, GetLastError is %08x\n", ret, GetLastError());
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
             "Wrong size %d\n", context->cbCertEncoded);
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
     "Didn't expect an error to be set: %08x\n", GetLastError());

    /* After removing store2, the collection should be empty */
    SetLastError(0xdeadbeef);
    CertRemoveStoreFromCollection(collection, store2);
    ok(GetLastError() == 0xdeadbeef,
     "Didn't expect an error to be set: %08x\n", GetLastError());
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
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
    ok(!store && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* Opening up any old key works.. */
    key = HKEY_CURRENT_USER;
    store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0, 0, key);
    /* Not sure if this is a bug in DuplicateHandle, marking todo_wine for now
     */
    todo_wine ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
    CertCloseStore(store, 0);

    rc = RegCreateKeyExA(HKEY_CURRENT_USER, tempKey, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, NULL);
    ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
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
        ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
        /* Add a certificate.  It isn't persisted right away, since it's only
         * added to the cache..
         */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert2, sizeof(bigCert2), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
         GetLastError());
        /* so flush the cache to force a commit.. */
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %08x\n", GetLastError());
        /* and check that the expected subkey was written. */
        size = sizeof(hash);
        ret = CryptHashCertificate(0, 0, 0, bigCert2, sizeof(bigCert2),
         hash, &size);
        ok(ret, "CryptHashCertificate failed: %d\n", GetLastError());
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1; i < size;
         i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
        if (subKey)
        {
            LPBYTE buf;

            size = 0;
            RegQueryValueExA(subKey, "Blob", NULL, NULL, NULL, &size);
            buf = HeapAlloc(GetProcessHeap(), 0, size);
            if (buf)
            {
                rc = RegQueryValueExA(subKey, "Blob", NULL, NULL, buf, &size);
                ok(!rc, "RegQueryValueExA failed: %d\n", rc);
                if (!rc)
                {
                    const struct CertPropIDHeader *hdr;

                    /* Both the hash and the cert should be present */
                    hdr = findPropID(buf, size, CERT_CERT_PROP_ID);
                    ok(hdr != NULL, "Expected to find a cert property\n");
                    if (hdr)
                    {
                        ok(hdr->cb == sizeof(bigCert2),
                           "Wrong size %d of cert property\n", hdr->cb);
                        ok(!memcmp((const BYTE *)hdr + sizeof(*hdr), bigCert2,
                         hdr->cb), "Unexpected cert in cert property\n");
                    }
                    hdr = findPropID(buf, size, CERT_HASH_PROP_ID);
                    ok(hdr != NULL, "Expected to find a hash property\n");
                    if (hdr)
                    {
                        ok(hdr->cb == sizeof(hash),
                           "Wrong size %d of hash property\n", hdr->cb);
                        ok(!memcmp((const BYTE *)hdr + sizeof(*hdr), hash,
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
        ok(ret, "CertControlStore failed: %08x\n", GetLastError());

        /* Add a serialized cert with a bogus hash directly to the registry */
        memset(hash, 0, sizeof(hash));
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1;
         i < sizeof(hash); i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
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
            ok(!rc, "RegSetValueExA failed: %d\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08x\n", GetLastError());

            /* Make sure the bogus hash cert gets loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 1, "Expected 1 certificates, got %d\n", certCount);

            RegCloseKey(subKey);
        }

        /* Add another serialized cert directly to the registry, this time
         * under the correct key name (named with the correct hash value).
         */
        size = sizeof(hash);
        ret = CryptHashCertificate(0, 0, 0, bigCert2,
         sizeof(bigCert2), hash, &size);
        ok(ret, "CryptHashCertificate failed: %d\n", GetLastError());
        strcpy(subKeyName, certificates);
        for (i = 0, ptr = subKeyName + sizeof(certificates) - 1;
         i < sizeof(hash); i++, ptr += 2)
            sprintf(ptr, "%02X", hash[i]);
        rc = RegCreateKeyExA(key, subKeyName, 0, NULL, 0, KEY_ALL_ACCESS, NULL,
         &subKey, NULL);
        ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
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
            ok(!rc, "RegSetValueExA failed: %d\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08x\n", GetLastError());

            /* and make sure just one cert still gets loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 1, "Expected 1 certificates, got %d\n", certCount);

            /* Try again with the correct hash... */
            ptr = buf + sizeof(*hdr);
            memcpy(ptr, hash, sizeof(hash));

            rc = RegSetValueExA(subKey, "Blob", 0, REG_BINARY, buf,
             sizeof(buf));
            ok(!rc, "RegSetValueExA failed: %d\n", rc);

            ret = CertControlStore(store, 0, CERT_STORE_CTRL_RESYNC, NULL);
            ok(ret, "CertControlStore failed: %08x\n", GetLastError());

            /* and make sure two certs get loaded. */
            certCount = 0;
            context = NULL;
            do {
                context = CertEnumCertificatesInStore(store, context);
                if (context)
                    certCount++;
            } while (context != NULL);
            ok(certCount == 2, "Expected 2 certificates, got %d\n", certCount);

            RegCloseKey(subKey);
        }
        CertCloseStore(store, 0);
        /* Is delete allowed on a reg store? */
        store = CertOpenStore(CERT_STORE_PROV_REG, 0, 0,
         CERT_STORE_DELETE_FLAG, key);
        ok(store == NULL, "Expected NULL return from CERT_STORE_DELETE_FLAG\n");
        ok(GetLastError() == 0, "CertOpenStore failed: %08x\n",
         GetLastError());

        RegCloseKey(key);
    }
    /* The CertOpenStore with CERT_STORE_DELETE_FLAG above will delete the
     * contents of the key, but not the key itself.
     */
    rc = RegCreateKeyExA(HKEY_CURRENT_USER, tempKey, 0, NULL, 0, KEY_ALL_ACCESS,
     NULL, &key, &disp);
    ok(!rc, "RegCreateKeyExA failed: %d\n", rc);
    ok(disp == REG_OPENED_EXISTING_KEY,
     "Expected REG_OPENED_EXISTING_KEY, got %d\n", disp);
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
         "Expected E_INVALIDARG, got %08x\n", GetLastError());
        CertCloseStore(memStore, 0);
    }
    CertCloseStore(store, 0);

    /* Check opening a bogus store */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, BogusW);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, BogusW);
    ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Now check whether deleting is allowed */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);

    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0, 0, NULL);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyA);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyW);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* The name is expected to be UNICODE, check with an ASCII name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
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
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_SYSTEM_STORE_CURRENT_USER, MyW);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* The name is expected to be UNICODE, first check with an ASCII name */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyA);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* Create the expected key */
    lstrcpyW(keyName, CERT_LOCAL_MACHINE_SYSTEM_STORE_REGPATH);
    lstrcatW(keyName, baskslashW);
    lstrcatW(keyName, MyW);
    rc = RegCreateKeyExW(HKEY_CURRENT_USER, keyName, 0, NULL, 0, KEY_READ,
     NULL, &key, NULL);
    ok(!rc, "RegCreateKeyEx failed: %d\n", rc);
    if (!rc)
        RegCloseKey(key);
    /* Check opening with a UNICODE name, specifying the create new flag */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_CREATE_NEW_FLAG, MyW);
    ok(!store && GetLastError() == ERROR_FILE_EXISTS,
     "Expected ERROR_FILE_EXISTS, got %08x\n", GetLastError());
    /* Now check opening with a UNICODE name, this time opening existing */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, MyW);
    ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        HCERTSTORE memStore = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
         CERT_STORE_CREATE_NEW_FLAG, NULL);

        /* Check that it's a collection store */
        if (memStore)
        {
            BOOL ret = CertAddStoreToCollection(store, memStore, 0, 0);

            /* FIXME: this'll fail on NT4, but what error will it give? */
            ok(ret, "CertAddStoreToCollection failed: %08x\n", GetLastError());
            CertCloseStore(memStore, 0);
        }
        CertCloseStore(store, 0);
    }

    /* Check opening a bogus store */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, BogusW);
    ok(!store && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER, BogusW);
    ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Now check whether deleting is allowed */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);
}

static const BYTE serializedStoreWithCert[] = {
 0x00,0x00,0x00,0x00,0x43,0x45,0x52,0x54,0x20,0x00,0x00,0x00,0x01,0x00,0x00,
 0x00,0x7c,0x00,0x00,0x00,0x30,0x7a,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,
 0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
 0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,
 0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,
 0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,
 0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
 0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,
 0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,
 0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00 };
static const BYTE serializedStoreWithCertAndCRL[] = {
 0x00,0x00,0x00,0x00,0x43,0x45,0x52,0x54,0x20,0x00,0x00,0x00,0x01,0x00,0x00,
 0x00,0x7c,0x00,0x00,0x00,0x30,0x7a,0x02,0x01,0x01,0x30,0x02,0x06,0x00,0x30,
 0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
 0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x22,0x18,0x0f,0x31,0x36,0x30,0x31,
 0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x18,0x0f,0x31,0x36,
 0x30,0x31,0x30,0x31,0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x15,
 0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,0x6e,
 0x20,0x4c,0x61,0x6e,0x67,0x00,0x30,0x07,0x30,0x02,0x06,0x00,0x03,0x01,0x00,
 0xa3,0x16,0x30,0x14,0x30,0x12,0x06,0x03,0x55,0x1d,0x13,0x01,0x01,0xff,0x04,
 0x08,0x30,0x06,0x01,0x01,0xff,0x02,0x01,0x01,0x21,0x00,0x00,0x00,0x01,0x00,
 0x00,0x00,0x47,0x00,0x00,0x00,0x30,0x45,0x30,0x2c,0x30,0x02,0x06,0x00,0x30,
 0x15,0x31,0x13,0x30,0x11,0x06,0x03,0x55,0x04,0x03,0x13,0x0a,0x4a,0x75,0x61,
 0x6e,0x20,0x4c,0x61,0x6e,0x67,0x00,0x18,0x0f,0x31,0x36,0x30,0x31,0x30,0x31,
 0x30,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x5a,0x30,0x02,0x06,0x00,0x03,0x11,
 0x00,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,
 0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

static void compareFile(LPCWSTR filename, const BYTE *pb, DWORD cb)
{
    HANDLE h;
    BYTE buf[200];
    BOOL ret;
    DWORD cbRead = 0, totalRead = 0;

    h = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
     FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        ret = ReadFile(h, buf, sizeof(buf), &cbRead, NULL);
        if (ret && cbRead)
        {
            ok(totalRead + cbRead <= cb, "Expected total count %d, see %d\n",
             cb, totalRead + cbRead);
            ok(!memcmp(pb + totalRead, buf, cbRead),
             "Unexpected data in file\n");
            totalRead += cbRead;
        }
    } while (ret && cbRead);
    CloseHandle(h);
}

static void testFileStore(void)
{
    static const WCHAR szPrefix[] = { 'c','e','r',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    HCERTSTORE store;
    BOOL ret;
    PCCERT_CONTEXT cert;
    HANDLE file;
 
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0, 0, NULL);
    ok(!store && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());

    if (!GetTempFileNameW(szDot, szPrefix, 0, filename))
       return;
 
    DeleteFileW(filename);
    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;

    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0, CERT_STORE_DELETE_FLAG,
     file);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_READONLY_FLAG, file);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());

    /* A "read-only" file store.. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        DWORD size;

        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        /* apparently allows adding certificates.. */
        ok(ret, "CertAddEncodedCertificateToStore failed: %d\n", ret);
        /* but not commits.. */
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08x\n", GetLastError());
        /* It still has certs in memory.. */
        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        CertFreeCertificateContext(cert);
        /* but the file size is still 0. */
        size = GetFileSize(file, NULL);
        ok(size == 0, "Expected size 0, got %d\n", size);
        CertCloseStore(store, 0);
    }

    /* The create new flag is allowed.. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        /* but without the commit enable flag, commits don't happen. */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %d\n", ret);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08x\n", GetLastError());
        CertCloseStore(store, 0);
    }
    /* as is the open existing flag. */
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_STORE_OPEN_EXISTING_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        /* but without the commit enable flag, commits don't happen. */
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %d\n", ret);
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
         "Expected ERROR_CALL_NOT_IMPLEMENTED, got %08x\n", GetLastError());
        CertCloseStore(store, 0);
    }
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        CloseHandle(file);
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
         GetLastError());
        /* with commits enabled, commit is allowed */
        ret = CertControlStore(store, 0, CERT_STORE_CTRL_COMMIT, NULL);
        ok(ret, "CertControlStore failed: %d\n", ret);
        compareFile(filename, serializedStoreWithCert,
         sizeof(serializedStoreWithCert));
        CertCloseStore(store, 0);
    }
    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return;
    store = CertOpenStore(CERT_STORE_PROV_FILE, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, file);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        CloseHandle(file);
        ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING, signedCRL,
         sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());
        CertCloseStore(store, 0);
        compareFile(filename, serializedStoreWithCertAndCRL,
         sizeof(serializedStoreWithCertAndCRL));
    }

    DeleteFileW(filename);
}

static void checkFileStoreFailure(LPCWSTR filename, DWORD dwEncodingType,
 DWORD dwFlags, DWORD expectedError)
{
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_FILENAME_W,
     dwEncodingType, 0, dwFlags, filename);

    ok(!store && GetLastError() == expectedError,
     "Expected %08x, got %08x\n", expectedError, GetLastError());
}

static BOOL initFileFromData(LPCWSTR filename, const BYTE *pb, DWORD cb)
{
    HANDLE file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    BOOL ret;

    if (file != INVALID_HANDLE_VALUE)
    {
        DWORD written;

        ret = WriteFile(file, pb, cb, &written, NULL);
        CloseHandle(file);
    }
    else
        ret = FALSE;
    return ret;
}
static void testFileNameStore(void)
{
    static const WCHAR szPrefix[] = { 'c','e','r',0 };
    static const WCHAR szDot[] = { '.',0 };
    WCHAR filename[MAX_PATH];
    HCERTSTORE store;
    BOOL ret;

    checkFileStoreFailure(NULL, 0, 0, ERROR_PATH_NOT_FOUND);

    if (!GetTempFileNameW(szDot, szPrefix, 0, filename))
       return;
    DeleteFileW(filename);

    /* The two flags are mutually exclusive */
    checkFileStoreFailure(filename, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_READONLY_FLAG,
     E_INVALIDARG);

    /* In all of the following tests, the encoding type seems to be ignored */
    if (initFileFromData(filename, bigCert, sizeof(bigCert)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        todo_wine ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(!crl, "Expected no CRLs\n");

        CertCloseStore(store, 0);
        DeleteFileW(filename);
    }
    if (initFileFromData(filename, serializedStoreWithCert,
     sizeof(serializedStoreWithCert)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(!crl, "Expected no CRLs\n");

        CertCloseStore(store, 0);
        DeleteFileW(filename);
    }
    if (initFileFromData(filename, serializedStoreWithCertAndCRL,
     sizeof(serializedStoreWithCertAndCRL)))
    {
        PCCERT_CONTEXT cert;
        PCCRL_CONTEXT crl;

        store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
         CERT_STORE_READONLY_FLAG, filename);
        ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());

        cert = CertEnumCertificatesInStore(store, NULL);
        ok(cert != NULL, "CertEnumCertificatesInStore failed: %08x\n",
         GetLastError());
        cert = CertEnumCertificatesInStore(store, cert);
        ok(!cert, "Expected only one cert\n");
        crl = CertEnumCRLsInStore(store, NULL);
        ok(crl != NULL, "CertEnumCRLsInStore failed: %08x\n", GetLastError());
        crl = CertEnumCRLsInStore(store, crl);
        ok(!crl, "Expected only one CRL\n");

        CertCloseStore(store, 0);
        /* Don't delete it this time, the next test uses it */
    }
    /* Now that the file exists, we can open it read-only */
    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_STORE_READONLY_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    CertCloseStore(store, 0);
    DeleteFileW(filename);

    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG | CERT_STORE_CREATE_NEW_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        ret = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
         bigCert, sizeof(bigCert), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCertificateToStore failed: %08x\n",
         GetLastError());
        CertCloseStore(store, 0);
        compareFile(filename, serializedStoreWithCert,
         sizeof(serializedStoreWithCert));
    }
    store = CertOpenStore(CERT_STORE_PROV_FILENAME_W, 0, 0,
     CERT_FILE_STORE_COMMIT_ENABLE_FLAG, filename);
    ok(store != NULL, "CertOpenStore failed: %08x\n", GetLastError());
    if (store)
    {
        ret = CertAddEncodedCRLToStore(store, X509_ASN_ENCODING,
         signedCRL, sizeof(signedCRL), CERT_STORE_ADD_ALWAYS, NULL);
        ok(ret, "CertAddEncodedCRLToStore failed: %08x\n", GetLastError());
        CertCloseStore(store, 0);
        compareFile(filename, serializedStoreWithCertAndCRL,
         sizeof(serializedStoreWithCertAndCRL));
    }
    DeleteFileW(filename);
}

static void testCertOpenSystemStore(void)
{
    HCERTSTORE store;

    store = CertOpenSystemStoreW(0, NULL);
    ok(!store && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* This succeeds, and on WinXP at least, the Bogus key is created under
     * HKCU (but not under HKLM, even when run as an administrator.)
     */
    store = CertOpenSystemStoreW(0, BogusW);
    ok(store != 0, "CertOpenSystemStore failed: %08x\n", GetLastError());
    if (store)
        CertCloseStore(store, 0);
    /* Delete it so other tests succeed next time around */
    store = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0,
     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_DELETE_FLAG, BogusW);
    RegDeleteKeyW(HKEY_CURRENT_USER, BogusPathW);
}

struct EnumSystemStoreInfo
{
    BOOL  goOn;
    DWORD storeCount;
};

static BOOL CALLBACK enumSystemStoreCB(const void *systemStore, DWORD dwFlags,
 PCERT_SYSTEM_STORE_INFO pStoreInfo, void *pvReserved, void *pvArg)
{
    struct EnumSystemStoreInfo *info = (struct EnumSystemStoreInfo *)pvArg;

    info->storeCount++;
    return info->goOn;
}

static void testCertEnumSystemStore(void)
{
    BOOL ret;
    struct EnumSystemStoreInfo info = { FALSE, 0 };

    SetLastError(0xdeadbeef);
    ret = CertEnumSystemStore(0, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* Crashes
    ret = CertEnumSystemStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, NULL, NULL,
     NULL);
     */

    SetLastError(0xdeadbeef);
    ret = CertEnumSystemStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, NULL, &info,
     enumSystemStoreCB);
    /* Callback returning FALSE stops enumeration */
    ok(!ret, "Expected CertEnumSystemStore to stop\n");
    ok(info.storeCount == 0 || info.storeCount == 1,
     "Expected 0 or 1 stores\n");

    info.goOn = TRUE;
    info.storeCount = 0;
    ret = CertEnumSystemStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, NULL, &info,
     enumSystemStoreCB);
    ok(ret, "CertEnumSystemStore failed: %08x\n", GetLastError());
    /* There should always be at least My, Root, and CA stores */
    ok(info.storeCount == 0 || info.storeCount >= 3,
     "Expected at least 3 stores\n");
}

static void testStoreProperty(void)
{
    HCERTSTORE store;
    BOOL ret;
    DWORD propID, size = 0, state;
    CRYPT_DATA_BLOB blob;

    /* Crash
    ret = CertGetStoreProperty(NULL, 0, NULL, NULL);
    ret = CertGetStoreProperty(NULL, 0, NULL, &size);
    ret = CertGetStoreProperty(store, 0, NULL, NULL);
     */

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    /* Check a missing prop ID */
    SetLastError(0xdeadbeef);
    ret = CertGetStoreProperty(store, 0, NULL, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    /* Contrary to MSDN, CERT_ACCESS_STATE_PROP_ID is supported for stores.. */
    size = sizeof(state);
    ret = CertGetStoreProperty(store, CERT_ACCESS_STATE_PROP_ID, &state, &size);
    ok(ret, "CertGetStoreProperty failed for CERT_ACCESS_STATE_PROP_ID: %08x\n",
     GetLastError());
    ok(!state, "Expected a non-persisted store\n");
    /* and CERT_STORE_LOCALIZED_NAME_PROP_ID isn't supported by default. */
    size = 0;
    ret = CertGetStoreProperty(store, CERT_STORE_LOCALIZED_NAME_PROP_ID, NULL,
     &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    /* Delete an arbitrary property on a store */
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, NULL);
    ok(ret, "CertSetStoreProperty failed: %08x\n", GetLastError());
    /* Set an arbitrary property on a store */
    blob.pbData = (LPBYTE)&state;
    blob.cbData = sizeof(state);
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, &blob);
    ok(ret, "CertSetStoreProperty failed: %08x\n", GetLastError());
    /* Get an arbitrary property that's been set */
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, NULL, &size);
    ok(ret, "CertGetStoreProperty failed: %08x\n", GetLastError());
    ok(size == sizeof(state), "Unexpected data size %d\n", size);
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, &propID, &size);
    ok(ret, "CertGetStoreProperty failed: %08x\n", GetLastError());
    ok(propID == state, "CertGetStoreProperty got the wrong value\n");
    /* Delete it again */
    ret = CertSetStoreProperty(store, CERT_FIRST_USER_PROP_ID, 0, NULL);
    ok(ret, "CertSetStoreProperty failed: %08x\n", GetLastError());
    /* And check that it's missing */
    SetLastError(0xdeadbeef);
    ret = CertGetStoreProperty(store, CERT_FIRST_USER_PROP_ID, NULL, &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    CertCloseStore(store, 0);

    /* Recheck on the My store.. */
    store = CertOpenSystemStoreW(0, MyW);
    size = sizeof(state);
    ret = CertGetStoreProperty(store, CERT_ACCESS_STATE_PROP_ID, &state, &size);
    ok(ret, "CertGetStoreProperty failed for CERT_ACCESS_STATE_PROP_ID: %08x\n",
     GetLastError());
    ok(state, "Expected a persisted store\n");
    SetLastError(0xdeadbeef);
    size = 0;
    ret = CertGetStoreProperty(store, CERT_STORE_LOCALIZED_NAME_PROP_ID, NULL,
     &size);
    ok(!ret && GetLastError() == CRYPT_E_NOT_FOUND,
     "Expected CRYPT_E_NOT_FOUND, got %08x\n", GetLastError());
    CertCloseStore(store, 0);
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
     "Expected ERROR_END_OF_MEDIA, got %08x\n", GetLastError());

    store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0,
     CERT_STORE_CREATE_NEW_FLAG, NULL);
    ok(store != 0, "CertOpenStore failed: %08x\n", GetLastError());

    ret = CertAddSerializedElementToStore(store, NULL, 0, 0, 0, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_END_OF_MEDIA,
     "Expected ERROR_END_OF_MEDIA, got %08x\n", GetLastError());

    /* Test with an empty property */
    hdr = (struct CertPropIDHeader *)buf;
    hdr->propID = CERT_CERT_PROP_ID;
    hdr->unknown1 = 1;
    hdr->cb = 0;
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Test with a bad size in property header */
    hdr->cb = sizeof(bigCert) - 1;
    memcpy(buf + sizeof(struct CertPropIDHeader), bigCert, sizeof(bigCert));
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 0, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, 0, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Kosher size in property header, but no context type */
    hdr->cb = sizeof(bigCert);
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 0,
     NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 0, NULL,
     NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, 0, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* With a bad context type */
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 
     CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 
     CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CRL_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    /* Bad unknown field, good type */
    hdr->unknown1 = 2;
    ret = CertAddSerializedElementToStore(store, buf, sizeof(buf), 0, 0, 
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), 0, 0, 
     CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08x\n", GetLastError());
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND got %08x\n", GetLastError());
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
    ok(ret, "CertAddSerializedElementToStore failed: %08x\n", GetLastError());
    /* Everything okay, check it's not re-added */
    ret = CertAddSerializedElementToStore(store, buf,
     sizeof(struct CertPropIDHeader) + sizeof(bigCert), CERT_STORE_ADD_NEW,
     0, CERT_STORE_CERTIFICATE_CONTEXT_FLAG, NULL, NULL);
    ok(!ret && GetLastError() == CRYPT_E_EXISTS,
     "Expected CRYPT_E_EXISTS, got %08x\n", GetLastError());

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
    ok(ret, "CertAddSerializedElementToStore failed: %08x\n", GetLastError());
    if (context)
    {
        BYTE hashVal[20], realHash[20];
        DWORD size = sizeof(hashVal);

        ret = CryptHashCertificate(0, 0, 0, bigCert, sizeof(bigCert),
         realHash, &size);
        ok(ret, "CryptHashCertificate failed: %08x\n", GetLastError());
        ret = CertGetCertificateContextProperty(context, CERT_HASH_PROP_ID,
         hashVal, &size);
        ok(ret, "CertGetCertificateContextProperty failed: %08x\n",
         GetLastError());
        ok(!memcmp(hashVal, realHash, size), "Unexpected hash\n");
        CertFreeCertificateContext(context);
    }

    CertCloseStore(store, 0);
}

START_TEST(store)
{
    /* various combinations of CertOpenStore */
    testMemStore();
    testCollectionStore();
    testRegStore();
    testSystemRegStore();
    testSystemStore();
    testFileStore();
    testFileNameStore();

    testCertOpenSystemStore();
    testCertEnumSystemStore();
    testStoreProperty();

    testAddSerialized();
}
