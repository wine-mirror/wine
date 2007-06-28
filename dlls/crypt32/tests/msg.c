/*
 * Unit test suite for crypt32.dll's CryptMsg functions
 *
 * Copyright 2007 Juan Lang
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

#include "wine/test.h"

static void test_msg_open_to_encode(void)
{
    HCRYPTMSG msg;

    /* Crash
    msg = CryptMsgOpenToEncode(PKCS_7_ASN_ENCODING, 0, CMSG_ENVELOPED, NULL,
     NULL, NULL);
    msg = CryptMsgOpenToEncode(PKCS_7_ASN_ENCODING, 0, CMSG_HASHED, NULL, NULL,
     NULL);
    msg = CryptMsgOpenToEncode(PKCS_7_ASN_ENCODING, 0, CMSG_SIGNED, NULL, NULL,
     NULL);
     */

    /* Bad encodings */
    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToEncode(0, 0, 0, NULL, NULL, NULL);
    ok(!msg && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %x\n", GetLastError());
    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToEncode(X509_ASN_ENCODING, 0, 0, NULL, NULL, NULL);
    ok(!msg && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %x\n", GetLastError());

    /* Bad message types */
    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToEncode(PKCS_7_ASN_ENCODING, 0, 0, NULL, NULL, NULL);
    ok(!msg && GetLastError() == CRYPT_E_INVALID_MSG_TYPE,
     "Expected CRYPT_E_INVALID_MSG_TYPE, got %x\n", GetLastError());
    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToEncode(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, 0,
     NULL, NULL, NULL);
    ok(!msg && GetLastError() == CRYPT_E_INVALID_MSG_TYPE,
     "Expected CRYPT_E_INVALID_MSG_TYPE, got %x\n", GetLastError());
    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToEncode(PKCS_7_ASN_ENCODING, 0,
     CMSG_SIGNED_AND_ENVELOPED, NULL, NULL, NULL);
    ok(!msg && GetLastError() == CRYPT_E_INVALID_MSG_TYPE,
     "Expected CRYPT_E_INVALID_MSG_TYPE, got %x\n", GetLastError());
    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToEncode(PKCS_7_ASN_ENCODING, 0, CMSG_ENCRYPTED, NULL,
     NULL, NULL);
    ok(!msg && GetLastError() == CRYPT_E_INVALID_MSG_TYPE,
     "Expected CRYPT_E_INVALID_MSG_TYPE, got %x\n", GetLastError());
}

static void test_msg_open_to_decode(void)
{
    HCRYPTMSG msg;
    CMSG_STREAM_INFO streamInfo = { 0 };

    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToDecode(0, 0, 0, 0, NULL, NULL);
    ok(!msg && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %x\n", GetLastError());

    /* Bad encodings */
    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToDecode(X509_ASN_ENCODING, 0, 0, 0, NULL, NULL);
    ok(!msg && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %x\n", GetLastError());
    SetLastError(0xdeadbeef);
    msg = CryptMsgOpenToDecode(X509_ASN_ENCODING, 0, CMSG_DATA, 0, NULL, NULL);
    ok(!msg && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %x\n", GetLastError());

    /* The message type can be explicit... */
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_DATA, 0, NULL,
     NULL);
    todo_wine {
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    CryptMsgClose(msg);
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_ENVELOPED, 0, NULL,
     NULL);
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    CryptMsgClose(msg);
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_HASHED, 0, NULL,
     NULL);
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    CryptMsgClose(msg);
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_SIGNED, 0, NULL,
     NULL);
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    CryptMsgClose(msg);
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0,
     CMSG_SIGNED_AND_ENVELOPED, 0, NULL, NULL);
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    CryptMsgClose(msg);
    /* or implicit.. */
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, 0, 0, NULL, NULL);
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    CryptMsgClose(msg);
    /* or even invalid. */
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_ENCRYPTED, 0, NULL,
     NULL);
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    CryptMsgClose(msg);
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, 1000, 0, NULL, NULL);
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    }
    CryptMsgClose(msg);

    /* And even though the stream info parameter "must be set to NULL" for
     * CMSG_HASHED, it's still accepted.
     */
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_HASHED, 0, NULL,
     &streamInfo);
    todo_wine
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    CryptMsgClose(msg);
}

static void test_msg_get_param(void)
{
    BOOL ret;
    HCRYPTMSG msg;
    DWORD size, i, value;
    CMSG_SIGNED_ENCODE_INFO signInfo = { sizeof(signInfo), 0 };
    CMSG_SIGNER_ENCODE_INFO signer = { sizeof(signer), 0 };

    /* Crash
    ret = CryptMsgGetParam(NULL, 0, 0, NULL, NULL);
    ret = CryptMsgGetParam(NULL, 0, 0, NULL, &size);
    ret = CryptMsgGetParam(msg, 0, 0, NULL, NULL);
     */

    /* Decoded messages */
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, 0, 0, NULL, NULL);
    todo_wine
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    /* For decoded messages, the type is always available */
    size = 0;
    ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, NULL, &size);
    todo_wine {
    ok(ret, "CryptMsgGetParam failed: %x\n", GetLastError());
    size = sizeof(value);
    ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, (LPBYTE)&value, &size);
    ok(ret, "CryptMsgGetParam failed: %x\n", GetLastError());
    /* For this (empty) message, the type isn't set */
    ok(value == 0, "Expected type 0, got %d\n", value);
    }
    CryptMsgClose(msg);

    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_DATA, 0, NULL,
     NULL);
    todo_wine
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    /* For explicitly typed messages, the type is known. */
    size = sizeof(value);
    ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, (LPBYTE)&value, &size);
    todo_wine {
    ok(ret, "CryptMsgGetParam failed: %x\n", GetLastError());
    ok(value == CMSG_DATA, "Expected CMSG_DATA, got %d\n", value);
    }
    for (i = CMSG_CONTENT_PARAM; i <= CMSG_CMS_SIGNER_INFO_PARAM; i++)
    {
        size = 0;
        ret = CryptMsgGetParam(msg, i, 0, NULL, &size);
        ok(!ret, "Parameter %d: expected failure\n", i);
    }
    CryptMsgClose(msg);

    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_ENVELOPED, 0, NULL,
     NULL);
    todo_wine {
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    size = sizeof(value);
    ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, (LPBYTE)&value, &size);
    ok(ret, "CryptMsgGetParam failed: %x\n", GetLastError());
    ok(value == CMSG_ENVELOPED, "Expected CMSG_ENVELOPED, got %d\n", value);
    }
    for (i = CMSG_CONTENT_PARAM; i <= CMSG_CMS_SIGNER_INFO_PARAM; i++)
    {
        size = 0;
        ret = CryptMsgGetParam(msg, i, 0, NULL, &size);
        ok(!ret, "Parameter %d: expected failure\n", i);
    }
    CryptMsgClose(msg);

    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_HASHED, 0, NULL,
     NULL);
    todo_wine {
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    size = sizeof(value);
    ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, (LPBYTE)&value, &size);
    ok(ret, "CryptMsgGetParam failed: %x\n", GetLastError());
    ok(value == CMSG_HASHED, "Expected CMSG_HASHED, got %d\n", value);
    }
    for (i = CMSG_CONTENT_PARAM; i <= CMSG_CMS_SIGNER_INFO_PARAM; i++)
    {
        size = 0;
        ret = CryptMsgGetParam(msg, i, 0, NULL, &size);
        ok(!ret, "Parameter %d: expected failure\n", i);
    }
    CryptMsgClose(msg);

    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_SIGNED, 0, NULL,
     NULL);
    todo_wine {
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    size = sizeof(value);
    ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, (LPBYTE)&value, &size);
    ok(ret, "CryptMsgGetParam failed: %x\n", GetLastError());
    ok(value == CMSG_SIGNED, "Expected CMSG_SIGNED, got %d\n", value);
    }
    for (i = CMSG_CONTENT_PARAM; i <= CMSG_CMS_SIGNER_INFO_PARAM; i++)
    {
        size = 0;
        ret = CryptMsgGetParam(msg, i, 0, NULL, &size);
        ok(!ret, "Parameter %d: expected failure\n", i);
    }
    CryptMsgClose(msg);

    /* Explicitly typed messages get their types set, even if they're invalid */
    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, CMSG_ENCRYPTED, 0, NULL,
     NULL);
    todo_wine {
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    size = sizeof(value);
    ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, (LPBYTE)&value, &size);
    ok(ret, "CryptMsgGetParam failed: %x\n", GetLastError());
    ok(value == CMSG_ENCRYPTED, "Expected CMSG_ENCRYPTED, got %d\n", value);
    }
    CryptMsgClose(msg);

    msg = CryptMsgOpenToDecode(PKCS_7_ASN_ENCODING, 0, 1000, 0, NULL, NULL);
    todo_wine {
    ok(msg != NULL, "CryptMsgOpenToDecode failed: %x\n", GetLastError());
    size = sizeof(value);
    ret = CryptMsgGetParam(msg, CMSG_TYPE_PARAM, 0, (LPBYTE)&value, &size);
    ok(ret, "CryptMsgGetParam failed: %x\n", GetLastError());
    ok(value == 1000, "Expected 1000, got %d\n", value);
    }
    CryptMsgClose(msg);
}

static void test_msg_close(void)
{
    BOOL ret;
    HCRYPTMSG msg;

    /* NULL succeeds.. */
    ret = CryptMsgClose(NULL);
    ok(ret, "CryptMsgClose failed: %x\n", GetLastError());
    /* but an arbitrary pointer crashes. */
    if (0)
        ret = CryptMsgClose((HCRYPTMSG)1);
    msg = CryptMsgOpenToEncode(PKCS_7_ASN_ENCODING, 0, CMSG_DATA, NULL, NULL,
     NULL);
    todo_wine
    ok(msg != NULL, "CryptMsgOpenToEncode failed: %x\n", GetLastError());
    ret = CryptMsgClose(msg);
    ok(ret, "CryptMsgClose failed: %x\n", GetLastError());
}

START_TEST(msg)
{
    /* Basic parameter checking tests */
    test_msg_open_to_encode();
    test_msg_open_to_decode();
    test_msg_get_param();
    test_msg_close();
}
