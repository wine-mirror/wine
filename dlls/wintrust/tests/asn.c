/* Unit test suite for wintrust asn functions
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
 *
 */
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wincrypt.h"
#include "wintrust.h"

#include "wine/test.h"

static WCHAR url[] = { 'h','t','t','p',':','/','/','w','i','n','e','h','q','.',
 'o','r','g',0 };
static const WCHAR nihongoURL[] = { 'h','t','t','p',':','/','/',0x226f,
 0x575b, 0 };
static const BYTE emptyURLSPCLink[] = { 0x80,0x00 };
static const BYTE urlSPCLink[] = {
0x80,0x11,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x77,0x69,0x6e,0x65,0x68,0x71,
0x2e,0x6f,0x72,0x67};
static const BYTE fileSPCLink[] = {
0xa2,0x14,0x80,0x12,0x00,0x68,0x00,0x74,0x00,0x74,0x00,0x70,0x00,0x3a,0x00,
0x2f,0x00,0x2f,0x22,0x6f,0x57,0x5b };
static const BYTE emptyMonikerSPCLink[] = {
0xa1,0x14,0x04,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x04,0x00 };
static BYTE data[] = { 0xba, 0xad, 0xf0, 0x0d };
static const BYTE monikerSPCLink[] = {
0xa1,0x18,0x04,0x10,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,
0xea,0xea,0xea,0xea,0xea,0x04,0x04,0xba,0xad,0xf0,0x0d };

static void test_encodeSPCLink(void)
{
    BOOL ret;
    DWORD size = 0;
    LPBYTE buf;
    SPC_LINK link = { 0 };

    SetLastError(0xdeadbeef);
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == E_INVALIDARG,
     "Expected E_INVALIDARG, got %08x\n", GetLastError());
    link.dwLinkChoice = SPC_URL_LINK_CHOICE;
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyURLSPCLink), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptyURLSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
    /* With an invalid char: */
    link.pwszUrl = (LPWSTR)nihongoURL;
    size = 1;
    SetLastError(0xdeadbeef);
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_IA5_STRING,
     "Expected CRYPT_E_INVALID_IA5_STRING, got %08x\n", GetLastError());
    /* Unlike the crypt32 string encoding routines, size is not set to the
     * index of the first invalid character.
     */
    ok(size == 0, "Expected size 0, got %d\n", size);
    link.pwszUrl = url;
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(urlSPCLink), "Unexpected size %d\n", size);
        ok(!memcmp(buf, urlSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
    link.dwLinkChoice = SPC_FILE_LINK_CHOICE;
    link.pwszFile = (LPWSTR)nihongoURL;
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(fileSPCLink), "Unexpected size %d\n", size);
        ok(!memcmp(buf, fileSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
    link.dwLinkChoice = SPC_MONIKER_LINK_CHOICE;
    memset(&link.Moniker, 0, sizeof(link.Moniker));
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(emptyMonikerSPCLink), "Unexpected size %d\n", size);
        ok(!memcmp(buf, emptyMonikerSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
    memset(&link.Moniker.ClassId, 0xea, sizeof(link.Moniker.ClassId));
    link.Moniker.SerializedData.pbData = data;
    link.Moniker.SerializedData.cbData = sizeof(data);
    ret = CryptEncodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT, &link,
     CRYPT_ENCODE_ALLOC_FLAG, NULL, &buf, &size);
    ok(ret, "CryptEncodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        ok(size == sizeof(monikerSPCLink), "Unexpected size %d\n", size);
        ok(!memcmp(buf, monikerSPCLink, size), "Unexpected value\n");
        LocalFree(buf);
    }
}

static const BYTE badMonikerSPCLink[] = {
0xa1,0x19,0x04,0x11,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,0xea,
0xea,0xea,0xea,0xea,0xea,0xea,0x04,0x04,0xba,0xad,0xf0,0x0d };

static void test_decodeSPCLink(void)
{
    BOOL ret;
    LPBYTE buf = NULL;
    DWORD size = 0;
    SPC_LINK *link;

    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     emptyURLSPCLink, sizeof(emptyURLSPCLink), CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_URL_LINK_CHOICE,
         "Expected SPC_URL_LINK_CHOICE, got %d\n", link->dwLinkChoice);
        ok(lstrlenW(link->pwszUrl) == 0, "Expected empty string\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     urlSPCLink, sizeof(urlSPCLink), CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_URL_LINK_CHOICE,
         "Expected SPC_URL_LINK_CHOICE, got %d\n", link->dwLinkChoice);
        ok(!lstrcmpW(link->pwszUrl, url), "Unexpected URL\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     fileSPCLink, sizeof(fileSPCLink), CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_FILE_LINK_CHOICE,
         "Expected SPC_FILE_LINK_CHOICE, got %d\n", link->dwLinkChoice);
        ok(!lstrcmpW(link->pwszFile, nihongoURL), "Unexpected file\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     emptyMonikerSPCLink, sizeof(emptyMonikerSPCLink), CRYPT_DECODE_ALLOC_FLAG,
     NULL, (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        SPC_SERIALIZED_OBJECT emptyMoniker = { { 0 } };

        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_MONIKER_LINK_CHOICE,
         "Expected SPC_MONIKER_LINK_CHOICE, got %d\n", link->dwLinkChoice);
        ok(!memcmp(&link->Moniker.ClassId, &emptyMoniker.ClassId,
         sizeof(emptyMoniker.ClassId)), "Unexpected value\n");
        ok(link->Moniker.SerializedData.cbData == 0,
         "Expected no serialized data\n");
        LocalFree(buf);
    }
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     monikerSPCLink, sizeof(monikerSPCLink), CRYPT_DECODE_ALLOC_FLAG, NULL,
     (BYTE *)&buf, &size);
    ok(ret, "CryptDecodeObjectEx failed: %08x\n", GetLastError());
    if (ret)
    {
        SPC_UUID id;

        link = (SPC_LINK *)buf;
        ok(link->dwLinkChoice == SPC_MONIKER_LINK_CHOICE,
         "Expected SPC_MONIKER_LINK_CHOICE, got %d\n", link->dwLinkChoice);
        memset(&id, 0xea, sizeof(id));
        ok(!memcmp(&link->Moniker.ClassId, &id, sizeof(id)),
         "Unexpected value\n");
        ok(link->Moniker.SerializedData.cbData == sizeof(data),
         "Unexpected data size %d\n", link->Moniker.SerializedData.cbData);
        ok(!memcmp(link->Moniker.SerializedData.pbData, data, sizeof(data)),
         "Unexpected value\n");
        LocalFree(buf);
    }
    SetLastError(0xdeadbeef);
    ret = CryptDecodeObjectEx(X509_ASN_ENCODING, SPC_LINK_STRUCT,
     badMonikerSPCLink, sizeof(badMonikerSPCLink), CRYPT_DECODE_ALLOC_FLAG,
     NULL, (BYTE *)&buf, &size);
    ok(!ret && GetLastError() == CRYPT_E_BAD_ENCODE,
     "Expected CRYPT_E_BAD_ENCODE, got %08x\n", GetLastError());
}

START_TEST(asn)
{
    test_encodeSPCLink();
    test_decodeSPCLink();
}
