/* CryptXML Tests
 *
 * Copyright (C) 2025 Mohamad Al-Jaf
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

#include "windef.h"
#include "winbase.h"
#include "cryptxml.h"

#include "wine/test.h"

#define check_status( handle, expected_size, expected_error, expected_info, todo ) check_status_( __LINE__, handle, expected_size, expected_error, expected_info, todo )
static void check_status_( unsigned int line, HCRYPTXML handle, ULONG expected_size, DWORD expected_error, DWORD expected_info, BOOL todo )
{
    CRYPT_XML_STATUS status;
    HRESULT hr;

    status.cbSize = 0xdeadbeef;
    status.dwErrorStatus = 0xdeadbeef;
    status.dwInfoStatus = 0xdeadbeef;

    hr = CryptXmlGetStatus( handle, &status );
    ok_(__FILE__, line)( hr == S_OK, "got CryptXmlGetStatus hr %#lx\n", hr );
    ok_(__FILE__, line)( status.cbSize == expected_size, "got status.cbSize %lu\n", status.cbSize );
    todo_wine_if( todo ) ok_(__FILE__, line)( status.dwErrorStatus == expected_error, "got status.dwErrorStatus %ld\n", status.dwErrorStatus );
    ok_(__FILE__, line)( status.dwInfoStatus == expected_info, "got status.dwInfoStatus %ld\n", status.dwInfoStatus );
}

static void test_verify_signature(void)
{
    static const BYTE signature[] =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<Envelope xmlns=\"urn:envelope\">"
    "<ds:Signature xmlns:ds=\"http://www.w3.org/2000/09/xmldsig#\">"
    "<ds:SignedInfo>"
    "<ds:CanonicalizationMethod Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315\"/>"
    "<ds:SignatureMethod Algorithm=\"http://www.w3.org/2001/04/xmldsig-more#rsa-sha256\"/>"
    "<ds:Reference URI=\"\">"
    "<ds:Transforms>"
    "<ds:Transform Algorithm=\"http://www.w3.org/2000/09/xmldsig#enveloped-signature\"/>"
    "<ds:Transform Algorithm=\"http://www.w3.org/TR/2001/REC-xml-c14n-20010315\"/>"
    "</ds:Transforms>"
    "<ds:DigestMethod Algorithm=\"http://www.w3.org/2001/04/xmlenc#sha256\"/>"
    "<ds:DigestValue>/juoQ4bDxElf1M+KJauO20euW+QAvvPP0nDCruCQooM=</ds:DigestValue>"
    "</ds:Reference>"
    "</ds:SignedInfo>"
    "<ds:SignatureValue>N0e0G9nH8CKLZPoUii3W2/lZWnu/eFl1mX5kVMLb1VCBdkcDlbIldw44HbHSY9WQruLjsm7YNyR4RjSYJSwk6RqcbaPt5LfzBJCgh3K+Hd/2TLlnN/xRpHzpnP86+11owwz6i475dRARDGRBUBWFcDkXXjdmWQ1nXbz1Pu4fIdBmpUtkPWnKtYitcq+YANnJez55PxShaZMQUZGnBipMJDajfyXTQr106TBrGic39+UJ+wfni6AE2GSeALsliiop9loBa0yekUcMSIUM8fjhBYRqfdg8vls26pacyEpBjDYUool0YA76o2iGHOXchFzndlm5KbQQyekea4ohmtVq2w==</ds:SignatureValue>"
    "<ds:KeyInfo>"
    "<ds:KeyValue>"
    "<ds:RSAKeyValue>"
    "<ds:Modulus>smBoZK3x0L164lUyh0iM8tTfHbR6Mk9yuoHryzryfe1bplF/Vj+mKCKPVXyCPrUvvHMZsDvdoE50EbILeSA3CGDSghDscKMJnWJZv19qNBZSzCi7UlE3PfmivL6NX8VwYuixHR7YbUJDq/gGnMW687gC8+b4JWPEgiBB71tw8OFluMa5hlIMuAfYMXYjr2QqTKuu4rgwRo3uei83HF3np2mglqFwRCVr6vADX+tRsA5YR6cDtVYqMOahF6YOhHbfYl9x2r+aFv82oBo+SussqdE2lthqowrWLTjHMy3htQoVhEJ0jiJULr/Da5isXDWzXENXH6F5jGIb7v86ZsgMQQ==</ds:Modulus>"
    "<ds:Exponent>AQAB</ds:Exponent>"
    "</ds:RSAKeyValue>"
    "</ds:KeyValue>"
    "</ds:KeyInfo>"
    "</ds:Signature>"
    "</Envelope>";
    const CRYPT_XML_SIGNATURE *sig;
    const CRYPT_XML_DOC_CTXT *ctx;
    HCRYPTXML handle = NULL;
    CRYPT_XML_STATUS status;
    CRYPT_XML_BLOB blob;
    HRESULT hr;

    hr = CryptXmlOpenToDecode( NULL, 0, NULL, 0, NULL, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    handle = (HCRYPTXML)0xdeadbeef;
    hr = CryptXmlOpenToDecode( NULL, 0, NULL, 0, NULL, &handle );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    ok( handle == (HCRYPTXML)0xdeadbeef, "got handle %p.\n", handle );

    blob.dwCharset = CRYPT_XML_CHARSET_UTF8;
    blob.cbData = strlen((const char *)signature) - 1;
    blob.pbData = (BYTE *)signature;
    hr = CryptXmlOpenToDecode( NULL, 0, NULL, 0, &blob, &handle );
    todo_wine
    ok( hr == WS_E_INVALID_FORMAT, "got hr %#lx.\n", hr );
    todo_wine
    ok( handle == NULL, "got handle %p.\n", handle );

    hr = CryptXmlClose( NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    blob.dwCharset = CRYPT_XML_CHARSET_UTF8;
    blob.cbData = strlen( (const char *)signature );
    blob.pbData = (BYTE *)signature;
    hr = CryptXmlOpenToDecode( NULL, 0, NULL, 0, &blob, &handle );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    hr = CryptXmlGetStatus( handle, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlGetStatus( NULL, &status );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    ctx = (CRYPT_XML_DOC_CTXT *)0xdeadbeef;
    hr = CryptXmlGetDocContext( NULL, &ctx );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    ok( ctx == (CRYPT_XML_DOC_CTXT *)0xdeadbeef, "got ctx %p\n", ctx );
    hr = CryptXmlGetDocContext( handle, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );

    hr = CryptXmlGetDocContext( handle, &ctx );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( ctx->cSignature == 1, "got signature count %lu\n", ctx->cSignature );
    ok( ctx->rgpSignature != NULL, "got NULL rgpSignature\n" );

    sig = (CRYPT_XML_SIGNATURE *)0xdeadbeef;
    hr = CryptXmlGetSignature( NULL, &sig );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    ok( sig == (CRYPT_XML_SIGNATURE *)0xdeadbeef, "got sig %p\n", sig );
    hr = CryptXmlGetSignature( handle, NULL );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlGetSignature( handle, &sig );
    ok( hr == CRYPT_XML_E_HANDLE, "got hr %#lx.\n", hr );
    ok( sig == NULL, "got sig %p\n", sig );

    hr = CryptXmlGetSignature( ctx->rgpSignature[0]->hSignature, &sig );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
    ok( sig != NULL, "failed to get signature\n" );
    ok( sig->cbSize == sizeof( CRYPT_XML_SIGNATURE ), "got sig->cbSize %lu\n", sig->cbSize );

    check_status( handle, sizeof( CRYPT_XML_STATUS ), CRYPT_XML_STATUS_ERROR_NOT_RESOLVED, 0, FALSE );

    hr = CryptXmlVerifySignature( NULL, NULL, 0 );
    ok( hr == E_INVALIDARG, "got hr %#lx.\n", hr );
    hr = CryptXmlVerifySignature( handle, NULL, 0 );
    ok( hr == CRYPT_XML_E_HANDLE, "got hr %#lx.\n", hr );
    hr = CryptXmlVerifySignature( handle, sig->pKeyInfo->hVerifyKey, 0 );
    ok( hr == CRYPT_XML_E_HANDLE, "got hr %#lx.\n", hr );

    hr = CryptXmlVerifySignature( sig->hSignature, sig->pKeyInfo->hVerifyKey, 0 );
    ok( hr == S_OK, "got hr %#lx.\n", hr );

    check_status( handle,          sizeof( CRYPT_XML_STATUS ), CRYPT_XML_STATUS_ERROR_DIGEST_INVALID, CRYPT_XML_STATUS_SIGNATURE_VALID, TRUE );
    check_status( sig->hSignature, sizeof( CRYPT_XML_STATUS ), CRYPT_XML_STATUS_ERROR_DIGEST_INVALID, CRYPT_XML_STATUS_SIGNATURE_VALID, TRUE );

    ctx = (CRYPT_XML_DOC_CTXT *)0xdeadbeef;
    hr = CryptXmlGetDocContext( sig->hSignature, &ctx );
    ok( hr == CRYPT_XML_E_HANDLE, "got hr %#lx.\n", hr );
    ok( ctx == NULL, "got ctx %p\n", ctx );

    hr = CryptXmlClose( sig->hSignature );
    ok( hr == S_FALSE, "got hr %#lx.\n", hr );
    hr = CryptXmlClose( handle );
    ok( hr == S_OK, "got hr %#lx.\n", hr );
}

START_TEST(cryptxml)
{
    test_verify_signature();
}
