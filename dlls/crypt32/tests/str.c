/*
 * Unit test suite for crypt32.dll's Cert*ToStr and CertStrToName functions.
 *
 * Copyright 2006 Juan Lang, Aric Stewart for CodeWeavers
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

typedef struct _CertRDNAttrEncoding {
    LPCSTR pszObjId;
    DWORD  dwValueType;
    CERT_RDN_VALUE_BLOB Value;
    LPCSTR str;
    BOOL todo;
} CertRDNAttrEncoding, *PCertRDNAttrEncoding;

typedef struct _CertRDNAttrEncodingW {
    LPCSTR pszObjId;
    DWORD  dwValueType;
    CERT_RDN_VALUE_BLOB Value;
    LPCWSTR str;
    BOOL todo;
} CertRDNAttrEncodingW, *PCertRDNAttrEncodingW;

static BYTE bin1[] = { 0x55, 0x53 };
static BYTE bin2[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x73, 0x6f, 0x74,
 0x61 };
static BYTE bin3[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x61, 0x70, 0x6f,
 0x6c, 0x69, 0x73 };
static BYTE bin4[] = { 0x43, 0x6f, 0x64, 0x65, 0x57, 0x65, 0x61, 0x76,
 0x65, 0x72, 0x73 };
static BYTE bin5[] = { 0x57, 0x69, 0x6e, 0x65, 0x20, 0x44, 0x65, 0x76,
 0x65, 0x6c, 0x6f, 0x70, 0x6d, 0x65, 0x6e, 0x74 };
static BYTE bin6[] = { 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
 0x74 };
static BYTE bin7[] = { 0x61, 0x72, 0x69, 0x63, 0x40, 0x63, 0x6f, 0x64,
 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63, 0x6f, 0x6d };
static BYTE bin8[] = {
0x65,0x00,0x50,0x00,0x4b,0x00,0x49,0x00,0x20,0x00,0x52,0x00,0x6f,0x00,0x6f,
0x00,0x74,0x00,0x20,0x00,0x43,0x00,0x65,0x00,0x72,0x00,0x74,0x00,0x69,0x00,
0x66,0x00,0x69,0x00,0x63,0x00,0x61,0x00,0x74,0x00,0x69,0x00,0x6f,0x00,0x6e,
0x00,0x20,0x00,0x41,0x00,0x75,0x00,0x74,0x00,0x68,0x00,0x6f,0x00,0x72,0x00,
0x69,0x00,0x74,0x00,0x79,0x00 };
static BYTE bin9[] = { 0x61, 0x62, 0x63, 0x22, 0x64, 0x65, 0x66 };
static BYTE bin10[] = { 0x61, 0x62, 0x63, 0x27, 0x64, 0x65, 0x66 };
static BYTE bin11[] = { 0x61, 0x62, 0x63, 0x2c, 0x20, 0x64, 0x65, 0x66 };
static BYTE bin12[] = { 0x20, 0x61, 0x62, 0x63, 0x20 };
static BYTE bin13[] = { 0x22, 0x64, 0x65, 0x66, 0x22 };
static BYTE bin14[] = { 0x31, 0x3b, 0x33 };

static const BYTE cert[] = 
{0x30,0x82,0x2,0xbb,0x30,0x82,0x2,0x24,0x2,0x9,0x0,0xe3,0x5a,0x10,0xf1,0xfc,
 0x4b,0xf3,0xa2,0x30,0xd,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x1,0x4,0x5,
 0x0,0x30,0x81,0xa1,0x31,0xb,0x30,0x9,0x6,0x3,0x55,0x4,0x6,0x13,0x2,0x55,0x53,
 0x31,0x12,0x30,0x10,0x6,0x3,0x55,0x4,0x8,0x13,0x9,0x4d,0x69,0x6e,0x6e,0x65,
 0x73,0x6f,0x74,0x61,0x31,0x14,0x30,0x12,0x6,0x3,0x55,0x4,0x7,0x13,0xb,0x4d,
 0x69,0x6e,0x6e,0x65,0x61,0x70,0x6f,0x6c,0x69,0x73,0x31,0x14,0x30,0x12,0x6,0x3,
 0x55,0x4,0xa,0x13,0xb,0x43,0x6f,0x64,0x65,0x57,0x65,0x61,0x76,0x65,0x72,0x73,
 0x31,0x19,0x30,0x17,0x6,0x3,0x55,0x4,0xb,0x13,0x10,0x57,0x69,0x6e,0x65,0x20,
 0x44,0x65,0x76,0x65,0x6c,0x6f,0x70,0x6d,0x65,0x6e,0x74,0x31,0x12,0x30,0x10,
 0x6,0x3,0x55,0x4,0x3,0x13,0x9,0x6c,0x6f,0x63,0x61,0x6c,0x68,0x6f,0x73,0x74,
 0x31,0x23,0x30,0x21,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x9,0x1,0x16,
 0x14,0x61,0x72,0x69,0x63,0x40,0x63,0x6f,0x64,0x65,0x77,0x65,0x61,0x76,0x65,
 0x72,0x73,0x2e,0x63,0x6f,0x6d,0x30,0x1e,0x17,0xd,0x30,0x36,0x30,0x31,0x32,
 0x35,0x31,0x33,0x35,0x37,0x32,0x34,0x5a,0x17,0xd,0x30,0x36,0x30,0x32,0x32,
 0x34,0x31,0x33,0x35,0x37,0x32,0x34,0x5a,0x30,0x81,0xa1,0x31,0xb,0x30,0x9,0x6,
 0x3,0x55,0x4,0x6,0x13,0x2,0x55,0x53,0x31,0x12,0x30,0x10,0x6,0x3,0x55,0x4,0x8,
 0x13,0x9,0x4d,0x69,0x6e,0x6e,0x65,0x73,0x6f,0x74,0x61,0x31,0x14,0x30,0x12,0x6,
 0x3,0x55,0x4,0x7,0x13,0xb,0x4d,0x69,0x6e,0x6e,0x65,0x61,0x70,0x6f,0x6c,0x69,
 0x73,0x31,0x14,0x30,0x12,0x6,0x3,0x55,0x4,0xa,0x13,0xb,0x43,0x6f,0x64,0x65,
 0x57,0x65,0x61,0x76,0x65,0x72,0x73,0x31,0x19,0x30,0x17,0x6,0x3,0x55,0x4,0xb,
 0x13,0x10,0x57,0x69,0x6e,0x65,0x20,0x44,0x65,0x76,0x65,0x6c,0x6f,0x70,0x6d,
 0x65,0x6e,0x74,0x31,0x12,0x30,0x10,0x6,0x3,0x55,0x4,0x3,0x13,0x9,0x6c,0x6f,
 0x63,0x61,0x6c,0x68,0x6f,0x73,0x74,0x31,0x23,0x30,0x21,0x6,0x9,0x2a,0x86,0x48,
 0x86,0xf7,0xd,0x1,0x9,0x1,0x16,0x14,0x61,0x72,0x69,0x63,0x40,0x63,0x6f,0x64,
 0x65,0x77,0x65,0x61,0x76,0x65,0x72,0x73,0x2e,0x63,0x6f,0x6d,0x30,0x81,0x9f,
 0x30,0xd,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x1,0x1,0x5,0x0,0x3,0x81,
 0x8d,0x0,0x30,0x81,0x89,0x2,0x81,0x81,0x0,0x9b,0xb5,0x8f,0xaf,0xfb,0x9a,0xaf,
 0xdc,0xa2,0x4d,0xb1,0xc8,0x72,0x44,0xef,0x79,0x7f,0x28,0xb6,0xfe,0x50,0xdc,
 0x8a,0xf7,0x11,0x2f,0x90,0x70,0xed,0xa4,0xa9,0xd,0xbf,0x82,0x3e,0x56,0xd8,
 0x36,0xb6,0x9,0x52,0x83,0xab,0x65,0x95,0x0,0xe2,0xea,0x3c,0x4f,0x85,0xb8,0xc,
 0x41,0x42,0x77,0x5c,0x9d,0x44,0xeb,0xcf,0x7d,0x60,0x64,0x7a,0x6c,0x4c,0xac,
 0x4a,0x9a,0x23,0x25,0x15,0xd7,0x92,0xb4,0x10,0xe7,0x95,0xad,0x4b,0x93,0xda,
 0x6a,0x76,0xe0,0xa5,0xd2,0x13,0x8,0x12,0x30,0x68,0xde,0xb9,0x5b,0x6e,0x2a,
 0x97,0x43,0xaa,0x7b,0x22,0x33,0x34,0xb1,0xca,0x5d,0x19,0xd8,0x42,0x26,0x45,
 0xc6,0xe9,0x1d,0xee,0x7,0xc2,0x27,0x95,0x87,0xd8,0x12,0xec,0x4b,0x16,0x9f,0x2,
 0x3,0x1,0x0,0x1,0x30,0xd,0x6,0x9,0x2a,0x86,0x48,0x86,0xf7,0xd,0x1,0x1,0x4,0x5,
 0x0,0x3,0x81,0x81,0x0,0x96,0xf9,0xf6,0x6a,0x3d,0xd9,0xca,0x6e,0xd5,0x76,0x73,
 0xab,0x75,0xc1,0xcc,0x98,0x44,0xc3,0xa9,0x90,0x68,0x88,0x76,0xb9,0xeb,0xb6,
 0xbe,0x60,0x62,0xb9,0x67,0x1e,0xcc,0xf4,0xe1,0xe7,0x6c,0xc8,0x67,0x3f,0x1d,
 0xf3,0x68,0x86,0x30,0xee,0xaa,0x92,0x61,0x37,0xd7,0x82,0x90,0x28,0xaa,0x7a,
 0x18,0x88,0x60,0x14,0x88,0x75,0xc0,0x4a,0x4e,0x7d,0x48,0xe7,0x3,0xa6,0xfd,
 0xd7,0xce,0x3c,0xe5,0x9b,0xaf,0x2f,0xdc,0xbb,0x7c,0xbd,0x20,0x49,0xd9,0x68,
 0x37,0xeb,0x5d,0xbb,0xe2,0x6d,0x66,0xe3,0x11,0xc1,0xa7,0x88,0x49,0xc6,0x6f,
 0x65,0xd3,0xce,0xae,0x26,0x19,0x3,0x2e,0x4f,0x78,0xa5,0xa,0x97,0x7e,0x4f,0xc4,
 0x91,0x8a,0xf8,0x5,0xef,0x5b,0x3b,0x49,0xbf,0x5f,0x2b};

static char issuerStr[] =
 "US, Minnesota, Minneapolis, CodeWeavers, Wine Development, localhost, aric@codeweavers.com";
static char issuerStrSemicolon[] =
 "US; Minnesota; Minneapolis; CodeWeavers; Wine Development; localhost; aric@codeweavers.com";
static char issuerStrCRLF[] =
 "US\r\nMinnesota\r\nMinneapolis\r\nCodeWeavers\r\nWine Development\r\nlocalhost\r\naric@codeweavers.com";
static char subjectStr[] =
 "2.5.4.6=US, 2.5.4.8=Minnesota, 2.5.4.7=Minneapolis, 2.5.4.10=CodeWeavers, 2.5.4.11=Wine Development, 2.5.4.3=localhost, 1.2.840.113549.1.9.1=aric@codeweavers.com";
static char subjectStrSemicolon[] =
 "2.5.4.6=US; 2.5.4.8=Minnesota; 2.5.4.7=Minneapolis; 2.5.4.10=CodeWeavers; 2.5.4.11=Wine Development; 2.5.4.3=localhost; 1.2.840.113549.1.9.1=aric@codeweavers.com";
static char subjectStrCRLF[] =
 "2.5.4.6=US\r\n2.5.4.8=Minnesota\r\n2.5.4.7=Minneapolis\r\n2.5.4.10=CodeWeavers\r\n2.5.4.11=Wine Development\r\n2.5.4.3=localhost\r\n1.2.840.113549.1.9.1=aric@codeweavers.com";
static char x500SubjectStr[] = "C=US, S=Minnesota, L=Minneapolis, O=CodeWeavers, OU=Wine Development, CN=localhost, E=aric@codeweavers.com";
static char x500SubjectStrSemicolonReverse[] = "E=aric@codeweavers.com; CN=localhost; OU=Wine Development; O=CodeWeavers; L=Minneapolis; S=Minnesota; C=US";

static HMODULE dll;
static DWORD (WINAPI *pCertNameToStrA)(DWORD,LPVOID,DWORD,LPSTR,DWORD);
static DWORD (WINAPI *pCertNameToStrW)(DWORD,LPVOID,DWORD,LPWSTR,DWORD);
static DWORD (WINAPI *pCertRDNValueToStrA)(DWORD, PCERT_RDN_VALUE_BLOB,
 LPSTR, DWORD);
static DWORD (WINAPI *pCertRDNValueToStrW)(DWORD, PCERT_RDN_VALUE_BLOB,
 LPWSTR, DWORD);
static BOOL (WINAPI *pCertStrToNameA)(DWORD dwCertEncodingType,
 LPCSTR pszX500, DWORD dwStrType, void *pvReserved, BYTE *pbEncoded,
 DWORD *pcbEncoded, LPCSTR *ppszError);
static BOOL (WINAPI *pCertStrToNameW)(DWORD dwCertEncodingType,
 LPCWSTR pszX500, DWORD dwStrType, void *pvReserved, BYTE *pbEncoded,
 DWORD *pcbEncoded, LPCWSTR *ppszError);
static DWORD (WINAPI *pCertGetNameStringA)(PCCERT_CONTEXT cert, DWORD type,
 DWORD flags, void *typePara, LPSTR str, DWORD cch);


static void test_CertRDNValueToStrA(void)
{
    CertRDNAttrEncoding attrs[] = {
     { "2.5.4.6", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin1), bin1 }, "US", FALSE },
     { "2.5.4.8", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin2), bin2 }, "Minnesota", FALSE },
     { "2.5.4.7", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin3), bin3 }, "Minneapolis", FALSE },
     { "2.5.4.10", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin4), bin4 }, "CodeWeavers", FALSE },
     { "2.5.4.11", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin5), bin5 }, "Wine Development", FALSE },
     { "2.5.4.3", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin6), bin6 }, "localhost", FALSE },
     { "1.2.840.113549.1.9.1", CERT_RDN_IA5_STRING,
       { sizeof(bin7), bin7 }, "aric@codeweavers.com", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin9), bin9 }, "abc\"def", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin10), bin10 }, "abc'def", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin11), bin11 }, "abc, def", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin12), bin12 }, " abc ", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin13), bin13 }, "\"def\"", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin14), bin14 }, "1;3", FALSE },
    };
    DWORD i, ret;
    char buffer[2000];
    CERT_RDN_VALUE_BLOB blob = { 0, NULL };
    static const char ePKI[] = "ePKI Root Certification Authority";

    if (!pCertRDNValueToStrA) return;

    /* This crashes
    ret = pCertRDNValueToStrA(0, NULL, NULL, 0);
     */
    /* With empty input, it generates the empty string */
    SetLastError(0xdeadbeef);
    ret = pCertRDNValueToStrA(0, &blob, NULL, 0);
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ret = pCertRDNValueToStrA(0, &blob, buffer, sizeof(buffer));
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ok(!buffer[0], "Expected empty string\n");

    for (i = 0; i < ARRAY_SIZE(attrs); i++)
    {
        ret = pCertRDNValueToStrA(attrs[i].dwValueType, &attrs[i].Value,
         buffer, sizeof(buffer));
        todo_wine_if (attrs[i].todo)
        {
            ok(ret == strlen(attrs[i].str) + 1, "Expected length %d, got %ld\n",
             lstrlenA(attrs[i].str) + 1, ret);
            ok(!strcmp(buffer, attrs[i].str), "Expected %s, got %s\n",
             attrs[i].str, buffer);
        }
    }
    blob.pbData = bin8;
    blob.cbData = sizeof(bin8);
    ret = pCertRDNValueToStrA(CERT_RDN_UTF8_STRING, &blob, buffer,
     sizeof(buffer));
    ok(ret == strlen(ePKI) + 1 || broken(ret != strlen(ePKI) + 1),
     "Expected length %d, got %ld\n", lstrlenA(ePKI), ret);
    if (ret == strlen(ePKI) + 1)
        ok(!strcmp(buffer, ePKI), "Expected %s, got %s\n", ePKI, buffer);
}

static void test_CertRDNValueToStrW(void)
{
    static const WCHAR ePKIW[] = L"ePKI Root Certification Authority";
    CertRDNAttrEncodingW attrs[] = {
     { "2.5.4.6", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin1), bin1 }, L"US", FALSE },
     { "2.5.4.8", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin2), bin2 }, L"Minnesota", FALSE },
     { "2.5.4.7", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin3), bin3 }, L"Minneapolis", FALSE },
     { "2.5.4.10", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin4), bin4 }, L"CodeWeavers", FALSE },
     { "2.5.4.11", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin5), bin5 }, L"Wine Development", FALSE },
     { "2.5.4.3", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin6), bin6 }, L"localhost", FALSE },
     { "1.2.840.113549.1.9.1", CERT_RDN_IA5_STRING,
       { sizeof(bin7), bin7 }, L"aric@codeweavers.com", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin9), bin9 }, L"abc\"def", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin10), bin10 }, L"abc'def", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin11), bin11 }, L"abc, def", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin12), bin12 }, L" abc ", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin13), bin13 }, L"\"def\"", FALSE },
     { "0", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin14), bin14 }, L"1;3", FALSE },
    };
    DWORD i, ret;
    WCHAR buffer[2000];
    CERT_RDN_VALUE_BLOB blob = { 0, NULL };

    if (!pCertRDNValueToStrW)
    {
        win_skip("CertRDNValueToStrW is not available\n");
        return;
    }

    /* This crashes
    ret = pCertRDNValueToStrW(0, NULL, NULL, 0);
     */
    /* With empty input, it generates the empty string */
    SetLastError(0xdeadbeef);
    ret = pCertRDNValueToStrW(0, &blob, NULL, 0);
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ret = pCertRDNValueToStrW(0, &blob, buffer, ARRAY_SIZE(buffer));
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ok(!buffer[0], "Expected empty string\n");

    for (i = 0; i < ARRAY_SIZE(attrs); i++)
    {
        ret = pCertRDNValueToStrW(attrs[i].dwValueType, &attrs[i].Value, buffer, ARRAY_SIZE(buffer));
        todo_wine_if (attrs[i].todo)
        {
            ok(ret == lstrlenW(attrs[i].str) + 1,
             "Expected length %d, got %ld\n", lstrlenW(attrs[i].str) + 1, ret);
            ok(!lstrcmpW(buffer, attrs[i].str), "Expected %s, got %s\n",
             wine_dbgstr_w(attrs[i].str), wine_dbgstr_w(buffer));
        }
    }
    blob.pbData = bin8;
    blob.cbData = sizeof(bin8);
    ret = pCertRDNValueToStrW(CERT_RDN_UTF8_STRING, &blob, buffer,
     sizeof(buffer));
    ok(ret == lstrlenW(ePKIW) + 1 || broken(ret != lstrlenW(ePKIW) + 1),
     "Expected length %d, got %ld\n", lstrlenW(ePKIW), ret);
    if (ret == lstrlenW(ePKIW) + 1)
        ok(!lstrcmpW(buffer, ePKIW), "Expected %s, got %s\n",
         wine_dbgstr_w(ePKIW), wine_dbgstr_w(buffer));
}

static void test_NameToStrConversionA(PCERT_NAME_BLOB pName, DWORD dwStrType,
 LPCSTR expected, BOOL todo)
{
    char buffer[2000] = { 0 };
    DWORD i;

    i = pCertNameToStrA(X509_ASN_ENCODING, pName, dwStrType, NULL, 0);
    todo_wine_if (todo)
        ok(i == strlen(expected) + 1, "Expected %d chars, got %ld\n",
         lstrlenA(expected) + 1, i);
    i = pCertNameToStrA(X509_ASN_ENCODING,pName, dwStrType, buffer,
     sizeof(buffer));
    todo_wine_if (todo)
        ok(i == strlen(expected) + 1, "Expected %d chars, got %ld\n",
         lstrlenA(expected) + 1, i);
    todo_wine_if (todo)
        ok(!strcmp(buffer, expected), "Expected %s, got %s\n", expected,
         buffer);
}

static BYTE encodedSimpleCN[] = {
0x30,0x0c,0x31,0x0a,0x30,0x08,0x06,0x03,0x55,0x04,0x03,0x13,0x01,0x31 };
static BYTE encodedSingleQuotedCN[] = { 0x30,0x0e,0x31,0x0c,0x30,0x0a,
 0x06,0x03,0x55,0x04,0x03,0x13,0x03,0x27,0x31,0x27 };
static BYTE encodedSpacedCN[] = { 0x30,0x0e,0x31,0x0c,0x30,0x0a,0x06,0x03,
 0x55,0x04,0x03,0x13,0x03,0x20,0x31,0x20 };
static BYTE encodedQuotedCN[] = { 0x30,0x11,0x31,0x0f,0x30,0x0d,0x06,0x03,
 0x55, 0x04,0x03,0x1e,0x06,0x00,0x22,0x00,0x31,0x00,0x22, };
static BYTE encodedMultipleAttrCN[] = { 0x30,0x0e,0x31,0x0c,0x30,0x0a,
 0x06,0x03,0x55,0x04,0x03,0x13,0x03,0x31,0x2b,0x32 };
static BYTE encodedCommaCN[] = {
0x30,0x0e,0x31,0x0c,0x30,0x0a,0x06,0x03,0x55,0x04,0x03,0x13,0x03,0x61,0x2c,
0x62 };
static BYTE encodedEqualCN[] = {
0x30,0x0e,0x31,0x0c,0x30,0x0a,0x06,0x03,0x55,0x04,0x03,0x13,0x03,0x61,0x3d,
0x62 };
static BYTE encodedLessThanCN[] = {
0x30,0x0d,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x03,0x1e,0x02,0x00,0x3c
};
static BYTE encodedGreaterThanCN[] = {
0x30,0x0d,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x03,0x1e,0x02,0x00,0x3e
};
static BYTE encodedHashCN[] = {
0x30,0x0d,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x03,0x1e,0x02,0x00,0x23
};
static BYTE encodedSemiCN[] = {
0x30,0x0d,0x31,0x0b,0x30,0x09,0x06,0x03,0x55,0x04,0x03,0x1e,0x02,0x00,0x3b
};
static BYTE encodedNewlineCN[] = {
0x30,0x11,0x31,0x0f,0x30,0x0d,0x06,0x03,0x55,0x04,0x03,0x1e,0x06,0x00,0x61,
0x00,0x0a,0x00,0x62 };
static BYTE encodedDummyCN[] = {
0x30,0x1F,0x31,0x0E,0x30,0x0C,0x06,0x03,0x55,0x04,0x03,0x13,0x05,0x64,0x75,
0x6D,0x6D,0x79,0x31,0x0D,0x30,0x0B,0x06,0x03,0x55,0x04,0x0C,0x13,0x04,0x74,
0x65,0x73,0x74 };
static BYTE encodedFields[] = {
0x30,0x2F,0x31,0x12,0x30,0x10,0x06,0x03,0x55,0x04,0x03,0x13,0x09,0x57,0x69,
0x6E,0x65,0x20,0x54,0x65,0x73,0x74,0x31,0x0C,0x30,0x0A,0x06,0x03,0x55,0x04,
0x0C,0x13,0x03,0x31,0x32,0x33,0x31,0x0B,0x30,0x09,0x06,0x03,0x55,0x04,0x06,
0x13,0x02,0x42,0x52 };

static void test_CertNameToStrA(void)
{
    PCCERT_CONTEXT context;
    CERT_NAME_BLOB blob;

    if (!pCertNameToStrA)
    {
        win_skip("CertNameToStrA is not available\n");
        return;
    }

    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert,
     sizeof(cert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        DWORD ret;

        /* This crashes
        ret = pCertNameToStrA(0, NULL, 0, NULL, 0);
         */
        /* Test with a bogus encoding type */
        SetLastError(0xdeadbeef);
        ret = pCertNameToStrA(0, &context->pCertInfo->Issuer, 0, NULL, 0);
        ok(ret == 1 && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected retval 1 and ERROR_FILE_NOT_FOUND, got %ld - %08lx\n",
         ret, GetLastError());
        SetLastError(0xdeadbeef);
        ret = pCertNameToStrA(X509_ASN_ENCODING, &context->pCertInfo->Issuer,
         0, NULL, 0);
        ok(ret && GetLastError() == ERROR_SUCCESS,
         "Expected positive return and ERROR_SUCCESS, got %ld - %08lx\n",
         ret, GetLastError());

        test_NameToStrConversionA(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR, issuerStr, FALSE);
        test_NameToStrConversionA(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         issuerStrSemicolon, FALSE);
        test_NameToStrConversionA(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         issuerStrCRLF, FALSE);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR, subjectStr, FALSE);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         subjectStrSemicolon, FALSE);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         subjectStrCRLF, FALSE);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_X500_NAME_STR, x500SubjectStr, FALSE);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG | CERT_NAME_STR_REVERSE_FLAG,
         x500SubjectStrSemicolonReverse, FALSE);

        CertFreeCertificateContext(context);
    }
    blob.pbData = encodedSimpleCN;
    blob.cbData = sizeof(encodedSimpleCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=1", FALSE);
    blob.pbData = encodedSingleQuotedCN;
    blob.cbData = sizeof(encodedSingleQuotedCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN='1'", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "'1'", FALSE);
    blob.pbData = encodedSpacedCN;
    blob.cbData = sizeof(encodedSpacedCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\" 1 \"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\" 1 \"", FALSE);
    blob.pbData = encodedQuotedCN;
    blob.cbData = sizeof(encodedQuotedCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"\"\"1\"\"\"",
     FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"\"\"1\"\"\"",
     FALSE);
    blob.pbData = encodedMultipleAttrCN;
    blob.cbData = sizeof(encodedMultipleAttrCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"1+2\"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"1+2\"", FALSE);
    blob.pbData = encodedCommaCN;
    blob.cbData = sizeof(encodedCommaCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"a,b\"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"a,b\"", FALSE);
    blob.pbData = encodedEqualCN;
    blob.cbData = sizeof(encodedEqualCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"a=b\"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"a=b\"", FALSE);
    blob.pbData = encodedLessThanCN;
    blob.cbData = sizeof(encodedLessThanCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"<\"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"<\"", FALSE);
    blob.pbData = encodedGreaterThanCN;
    blob.cbData = sizeof(encodedGreaterThanCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\">\"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\">\"", FALSE);
    blob.pbData = encodedHashCN;
    blob.cbData = sizeof(encodedHashCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"#\"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"#\"", FALSE);
    blob.pbData = encodedSemiCN;
    blob.cbData = sizeof(encodedSemiCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\";\"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\";\"", FALSE);
    blob.pbData = encodedNewlineCN;
    blob.cbData = sizeof(encodedNewlineCN);
    test_NameToStrConversionA(&blob, CERT_X500_NAME_STR, "CN=\"a\nb\"", FALSE);
    test_NameToStrConversionA(&blob, CERT_SIMPLE_NAME_STR, "\"a\nb\"", FALSE);
}

static void test_NameToStrConversionW(PCERT_NAME_BLOB pName, DWORD dwStrType,
 LPCWSTR expected, BOOL todo)
{
    WCHAR buffer[2000] = { 0 };
    DWORD i;

    i = pCertNameToStrW(X509_ASN_ENCODING,pName, dwStrType, NULL, 0);
    todo_wine_if (todo)
        ok(i == lstrlenW(expected) + 1, "Expected %d chars, got %ld\n",
         lstrlenW(expected) + 1, i);
    i = pCertNameToStrW(X509_ASN_ENCODING,pName, dwStrType, buffer, ARRAY_SIZE(buffer));
    todo_wine_if (todo)
        ok(i == lstrlenW(expected) + 1, "Expected %d chars, got %ld\n",
         lstrlenW(expected) + 1, i);
    todo_wine_if (todo)
        ok(!lstrcmpW(buffer, expected), "Expected %s, got %s\n",
         wine_dbgstr_w(expected), wine_dbgstr_w(buffer));
}

static void test_CertNameToStrW(void)
{
    PCCERT_CONTEXT context;
    CERT_NAME_BLOB blob;

    if (!pCertNameToStrW)
    {
        win_skip("CertNameToStrW is not available\n");
        return;
    }

    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert,
     sizeof(cert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        DWORD ret;

        /* This crashes
        ret = pCertNameToStrW(0, NULL, 0, NULL, 0);
         */
        /* Test with a bogus encoding type */
        SetLastError(0xdeadbeef);
        ret = pCertNameToStrW(0, &context->pCertInfo->Issuer, 0, NULL, 0);
        ok(ret == 1 && GetLastError() == ERROR_FILE_NOT_FOUND,
         "Expected retval 1 and ERROR_FILE_NOT_FOUND, got %ld - %08lx\n",
         ret, GetLastError());
        SetLastError(0xdeadbeef);
        ret = pCertNameToStrW(X509_ASN_ENCODING, &context->pCertInfo->Issuer,
         0, NULL, 0);
        ok(ret && GetLastError() == ERROR_SUCCESS,
         "Expected positive return and ERROR_SUCCESS, got %ld - %08lx\n",
         ret, GetLastError());

        test_NameToStrConversionW(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR,
         L"US, Minnesota, Minneapolis, CodeWeavers, Wine Development, localhost, aric@codeweavers.com", FALSE);
        test_NameToStrConversionW(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         L"US; Minnesota; Minneapolis; CodeWeavers; Wine Development; localhost; aric@codeweavers.com", FALSE);
        test_NameToStrConversionW(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         L"US\r\nMinnesota\r\nMinneapolis\r\nCodeWeavers\r\nWine Development\r\nlocalhost\r\naric@codeweavers.com",
         FALSE);
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR,
         L"2.5.4.6=US, 2.5.4.8=Minnesota, 2.5.4.7=Minneapolis, 2.5.4.10=CodeWeavers, 2.5.4.11=Wine Development,"
          " 2.5.4.3=localhost, 1.2.840.113549.1.9.1=aric@codeweavers.com", FALSE);
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         L"2.5.4.6=US; 2.5.4.8=Minnesota; 2.5.4.7=Minneapolis; 2.5.4.10=CodeWeavers; 2.5.4.11=Wine Development;"
          " 2.5.4.3=localhost; 1.2.840.113549.1.9.1=aric@codeweavers.com", FALSE);
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         L"2.5.4.6=US\r\n2.5.4.8=Minnesota\r\n2.5.4.7=Minneapolis\r\n2.5.4.10=CodeWeavers\r\n2.5.4.11=Wine "
          "Development\r\n2.5.4.3=localhost\r\n1.2.840.113549.1.9.1=aric@codeweavers.com", FALSE);
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_X500_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG | CERT_NAME_STR_REVERSE_FLAG,
         L"E=aric@codeweavers.com; CN=localhost; OU=Wine Development; O=CodeWeavers; L=Minneapolis; S=Minnesota; "
          "C=US", FALSE);

        CertFreeCertificateContext(context);
    }
    blob.pbData = encodedSimpleCN;
    blob.cbData = sizeof(encodedSimpleCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=1", FALSE);
    blob.pbData = encodedSingleQuotedCN;
    blob.cbData = sizeof(encodedSingleQuotedCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN='1'",
     FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR,
     L"'1'", FALSE);
    blob.pbData = encodedSpacedCN;
    blob.cbData = sizeof(encodedSpacedCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\" 1 \"", FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\" 1 \"",
     FALSE);
    blob.pbData = encodedQuotedCN;
    blob.cbData = sizeof(encodedQuotedCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"\"\"1\"\"\"",
     FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"\"\"1\"\"\"",
     FALSE);
    blob.pbData = encodedMultipleAttrCN;
    blob.cbData = sizeof(encodedMultipleAttrCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"1+2\"",
     FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR,
     L"\"1+2\"", FALSE);
    blob.pbData = encodedCommaCN;
    blob.cbData = sizeof(encodedCommaCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"a,b\"", FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"a,b\"",
     FALSE);
    blob.pbData = encodedEqualCN;
    blob.cbData = sizeof(encodedEqualCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"a=b\"", FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"a=b\"",
     FALSE);
    blob.pbData = encodedLessThanCN;
    blob.cbData = sizeof(encodedLessThanCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"<\"", FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"<\"",
     FALSE);
    blob.pbData = encodedGreaterThanCN;
    blob.cbData = sizeof(encodedGreaterThanCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\">\"",
     FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR,
     L"\">\"", FALSE);
    blob.pbData = encodedHashCN;
    blob.cbData = sizeof(encodedHashCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"#\"", FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"#\"",
     FALSE);
    blob.pbData = encodedSemiCN;
    blob.cbData = sizeof(encodedSemiCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\";\"", FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\";\"",
     FALSE);
    blob.pbData = encodedNewlineCN;
    blob.cbData = sizeof(encodedNewlineCN);
    test_NameToStrConversionW(&blob, CERT_X500_NAME_STR, L"CN=\"a\nb\"", FALSE);
    test_NameToStrConversionW(&blob, CERT_SIMPLE_NAME_STR, L"\"a\nb\"",
     FALSE);
}

struct StrToNameA
{
    LPCSTR x500;
    DWORD encodedSize;
    const BYTE *encoded;
};

static const struct StrToNameA namesA[] = {
 { "CN=1", sizeof(encodedSimpleCN), encodedSimpleCN },
 { "CN=\"1\"", sizeof(encodedSimpleCN), encodedSimpleCN },
 { "CN = \"1\"", sizeof(encodedSimpleCN), encodedSimpleCN },
 { "CN='1'", sizeof(encodedSingleQuotedCN), encodedSingleQuotedCN },
 { "CN=\" 1 \"", sizeof(encodedSpacedCN), encodedSpacedCN },
 { "CN=\"\"\"1\"\"\"", sizeof(encodedQuotedCN), encodedQuotedCN },
 { "CN=\"1+2\"", sizeof(encodedMultipleAttrCN), encodedMultipleAttrCN },
 { "CN=\"a,b\"", sizeof(encodedCommaCN), encodedCommaCN },
 { "CN=\"a=b\"", sizeof(encodedEqualCN), encodedEqualCN },
 { "CN=\"<\"", sizeof(encodedLessThanCN), encodedLessThanCN },
 { "CN=\">\"", sizeof(encodedGreaterThanCN), encodedGreaterThanCN },
 { "CN=\"#\"", sizeof(encodedHashCN), encodedHashCN },
 { "CN=\";\"", sizeof(encodedSemiCN), encodedSemiCN },
 { "CN=dummy,T=test", sizeof(encodedDummyCN), encodedDummyCN },
 { " CN =   Wine Test,T = 123, C = BR", sizeof(encodedFields), encodedFields },
};

static void test_CertStrToNameA(void)
{
    BOOL ret;
    DWORD size, i;
    BYTE buf[100];

    if (!pCertStrToNameA)
    {
        win_skip("CertStrToNameA is not available\n");
        return;
    }

    /* Crash
    ret = pCertStrToNameA(0, NULL, 0, NULL, NULL, NULL, NULL);
     */
    ret = pCertStrToNameA(0, NULL, 0, NULL, NULL, &size, NULL);
    ok(!ret, "Expected failure\n");
    ret = pCertStrToNameA(0, "bogus", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(0, "foo=1", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(0, "CN=1", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=1", 0, NULL, NULL, &size, NULL);
    ok(ret, "CertStrToNameA failed: %08lx\n", GetLastError());
    size = sizeof(buf);
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=\"\"1\"\"", 0, NULL, buf, &size,
     NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=1+2", 0, NULL, buf,
     &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=1+2", CERT_NAME_STR_NO_PLUS_FLAG, NULL, buf,
                          &size, NULL);
    ok(ret && GetLastError() == ERROR_SUCCESS,
                 "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=1,2", CERT_NAME_STR_NO_QUOTING_FLAG, NULL, buf,
                          &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
                 "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=\"1,2;3,4\"", CERT_NAME_STR_NO_QUOTING_FLAG, NULL, buf,
                          &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
                 "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=abc", 0, NULL, buf,
                          &size, NULL);
    ok(ret && GetLastError() == ERROR_SUCCESS,
                 "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=abc", CERT_NAME_STR_NO_QUOTING_FLAG, NULL, buf,
                          &size, NULL);
    ok(ret && GetLastError() == ERROR_SUCCESS,
                 "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=\"abc\"", 0, NULL, buf,
                          &size, NULL);
    ok(ret && GetLastError() == ERROR_SUCCESS,
                 "Expected ERROR_SUCCESS, got %08lx\n", GetLastError());
    ret = pCertStrToNameA(X509_ASN_ENCODING, "CN=\"abc\"", CERT_NAME_STR_NO_QUOTING_FLAG, NULL, buf,
                          &size, NULL);
    ok(!ret && GetLastError() == ERROR_MORE_DATA,
                 "Expected ERROR_MORE_DATA, got %08lx\n", GetLastError());
    for (i = 0; i < ARRAY_SIZE(namesA); i++)
    {
        size = sizeof(buf);
        ret = pCertStrToNameA(X509_ASN_ENCODING, namesA[i].x500, 0, NULL, buf,
         &size, NULL);
        ok(ret, "CertStrToNameA failed on string %s: %08lx\n", namesA[i].x500,
         GetLastError());
        ok(size == namesA[i].encodedSize,
         "Expected size %ld, got %ld\n", namesA[i].encodedSize, size);
        if (ret)
            ok(!memcmp(buf, namesA[i].encoded, namesA[i].encodedSize),
             "Unexpected value for string %s\n", namesA[i].x500);
    }
}

struct StrToNameW
{
    LPCWSTR x500;
    DWORD encodedSize;
    const BYTE *encoded;
};

static const BYTE encodedJapaneseCN[] = { 0x30,0x0f,0x31,0x0d,0x30,0x0b,0x06,
 0x03,0x55,0x04,0x03,0x1e,0x04,0x22,0x6f,0x57,0x5b };

static const struct StrToNameW namesW[] = {
 { L"CN=1", sizeof(encodedSimpleCN), encodedSimpleCN },
 { L"CN=\"1\"", sizeof(encodedSimpleCN), encodedSimpleCN },
 { L"CN = \"1\"", sizeof(encodedSimpleCN), encodedSimpleCN },
 { L"CN='1'", sizeof(encodedSingleQuotedCN), encodedSingleQuotedCN },
 { L"CN=\" 1 \"", sizeof(encodedSpacedCN), encodedSpacedCN },
 { L"CN=\"\"\"1\"\"\"", sizeof(encodedQuotedCN), encodedQuotedCN },
 { L"CN=\"1+2\"", sizeof(encodedMultipleAttrCN), encodedMultipleAttrCN },
 { L"CN=\x226f\x575b", sizeof(encodedJapaneseCN), encodedJapaneseCN },
 { L"CN=\"a,b\"", sizeof(encodedCommaCN), encodedCommaCN },
 { L"CN=\"a=b\"", sizeof(encodedEqualCN), encodedEqualCN },
 { L"CN=\"<\"", sizeof(encodedLessThanCN), encodedLessThanCN },
 { L"CN=\">\"", sizeof(encodedGreaterThanCN), encodedGreaterThanCN },
 { L"CN=\"#\"", sizeof(encodedHashCN), encodedHashCN },
 { L"CN=\";\"", sizeof(encodedSemiCN), encodedSemiCN },
 { L"CN=dummy,T=test", sizeof(encodedDummyCN), encodedDummyCN },
 { L" CN =   Wine Test,T = 123, C = BR", sizeof(encodedFields), encodedFields },
};

static void test_CertStrToNameW(void)
{
    BOOL ret;
    DWORD size, i;
    LPCWSTR errorPtr;
    BYTE buf[100];

    if (!pCertStrToNameW)
    {
        win_skip("CertStrToNameW is not available\n");
        return;
    }

    /* Crash
    ret = pCertStrToNameW(0, NULL, 0, NULL, NULL, NULL, NULL);
     */
    ret = pCertStrToNameW(0, NULL, 0, NULL, NULL, &size, NULL);
    ok(!ret, "Expected failure\n");
    ret = pCertStrToNameW(0, L"bogus", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameW(0, L"foo=1", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameW(0, L"CN=1", 0, NULL, NULL, &size, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08lx\n", GetLastError());
    ret = pCertStrToNameW(X509_ASN_ENCODING, L"CN=1", 0, NULL, NULL, &size,
     NULL);
    ok(ret, "CertStrToNameW failed: %08lx\n", GetLastError());
    size = sizeof(buf);
    ret = pCertStrToNameW(X509_ASN_ENCODING, L"CN=\"\"1\"\"", 0, NULL, buf,
     &size, NULL);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ret = pCertStrToNameW(X509_ASN_ENCODING, L"CN=\"\"1\"\"", 0, NULL, buf,
     &size, &errorPtr);
    ok(!ret && GetLastError() == CRYPT_E_INVALID_X500_STRING,
     "Expected CRYPT_E_INVALID_X500_STRING, got %08lx\n", GetLastError());
    ok(errorPtr && *errorPtr == '1', "Expected first error character was 1\n");
    for (i = 0; i < ARRAY_SIZE(namesW); i++)
    {
        size = sizeof(buf);
        ret = pCertStrToNameW(X509_ASN_ENCODING, namesW[i].x500, 0, NULL, buf,
         &size, NULL);
        ok(ret, "Index %ld: CertStrToNameW failed: %08lx\n", i, GetLastError());
        ok(size == namesW[i].encodedSize,
         "Index %ld: expected size %ld, got %ld\n", i, namesW[i].encodedSize,
         size);
        if (ret)
            ok(!memcmp(buf, namesW[i].encoded, size),
             "Index %ld: unexpected value for string %s\n", i, wine_dbgstr_w(namesW[i].x500));
    }
}

static void test_CertGetNameStringA(void)
{
    PCCERT_CONTEXT context;

    if (!pCertGetNameStringA)
    {
        win_skip("CertGetNameStringA is not available\n");
        return;
    }

    context = CertCreateCertificateContext(X509_ASN_ENCODING, cert,
     sizeof(cert));
    ok(context != NULL, "CertCreateCertificateContext failed: %08lx\n",
     GetLastError());
    if (context)
    {
        static const char aric[] = "aric@codeweavers.com";
        static const char localhost[] = "localhost";
        DWORD len, type;
        LPSTR str;

        /* Bad string types/types missing from the cert */
        len = pCertGetNameStringA(NULL, 0, 0, NULL, NULL, 0);
        ok(len == 1, "expected 1, got %ld\n", len);
        len = pCertGetNameStringA(context, 0, 0, NULL, NULL, 0);
        ok(len == 1, "expected 1, got %ld\n", len);
        len = pCertGetNameStringA(context, CERT_NAME_URL_TYPE, 0, NULL, NULL,
         0);
        ok(len == 1, "expected 1, got %ld\n", len);

        len = pCertGetNameStringA(context, CERT_NAME_EMAIL_TYPE, 0, NULL, NULL,
         0);
        ok(len == strlen(aric) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_EMAIL_TYPE, 0, NULL,
             str, len);
            ok(!strcmp(str, aric), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }

        len = pCertGetNameStringA(context, CERT_NAME_RDN_TYPE, 0, NULL, NULL,
         0);
        ok(len == strlen(issuerStr) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_RDN_TYPE, 0, NULL,
             str, len);
            ok(!strcmp(str, issuerStr), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }
        type = 0;
        len = pCertGetNameStringA(context, CERT_NAME_RDN_TYPE, 0, &type, NULL,
         0);
        ok(len == strlen(issuerStr) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_RDN_TYPE, 0, &type,
             str, len);
            ok(!strcmp(str, issuerStr), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }
        type = CERT_OID_NAME_STR;
        len = pCertGetNameStringA(context, CERT_NAME_RDN_TYPE, 0, &type, NULL,
         0);
        ok(len == strlen(subjectStr) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_RDN_TYPE, 0, &type,
             str, len);
            ok(!strcmp(str, subjectStr), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }

        len = pCertGetNameStringA(context, CERT_NAME_ATTR_TYPE, 0, NULL, NULL,
         0);
        ok(len == strlen(aric) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_ATTR_TYPE, 0, NULL,
             str, len);
            ok(!strcmp(str, aric), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }
        len = pCertGetNameStringA(context, CERT_NAME_ATTR_TYPE, 0,
         (void *)szOID_RSA_emailAddr, NULL, 0);
        ok(len == strlen(aric) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_ATTR_TYPE, 0,
             (void *)szOID_RSA_emailAddr, str, len);
            ok(!strcmp(str, aric), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }
        len = pCertGetNameStringA(context, CERT_NAME_ATTR_TYPE, 0,
         (void *)szOID_COMMON_NAME, NULL, 0);
        ok(len == strlen(localhost) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_ATTR_TYPE, 0,
             (void *)szOID_COMMON_NAME, str, len);
            ok(!strcmp(str, localhost), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }

        len = pCertGetNameStringA(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,
         NULL, NULL, 0);
        ok(len == strlen(localhost) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_SIMPLE_DISPLAY_TYPE,
             0, NULL, str, len);
            ok(!strcmp(str, localhost), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }

        len = pCertGetNameStringA(context, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0,
         NULL, NULL, 0);
        ok(len == strlen(localhost) + 1, "unexpected length %ld\n", len);
        str = HeapAlloc(GetProcessHeap(), 0, len);
        if (str)
        {
            len = pCertGetNameStringA(context, CERT_NAME_FRIENDLY_DISPLAY_TYPE,
             0, NULL, str, len);
            ok(!strcmp(str, localhost), "unexpected value %s\n", str);
            HeapFree(GetProcessHeap(), 0, str);
        }

        len = pCertGetNameStringA(context, CERT_NAME_DNS_TYPE, 0, NULL, NULL,
         0);
        ok(len == strlen(localhost) + 1 || broken(len == 1) /* NT4 */,
         "unexpected length %ld\n", len);
        if (len > 1)
        {
            str = HeapAlloc(GetProcessHeap(), 0, len);
            if (str)
            {
                len = pCertGetNameStringA(context, CERT_NAME_DNS_TYPE, 0, NULL,
                 str, len);
                ok(!strcmp(str, localhost), "unexpected value %s\n", str);
                HeapFree(GetProcessHeap(), 0, str);
            }
        }

        CertFreeCertificateContext(context);
    }
}

START_TEST(str)
{
    dll = GetModuleHandleA("Crypt32.dll");

    pCertNameToStrA = (void*)GetProcAddress(dll,"CertNameToStrA");
    pCertNameToStrW = (void*)GetProcAddress(dll,"CertNameToStrW");
    pCertRDNValueToStrA = (void*)GetProcAddress(dll, "CertRDNValueToStrA");
    pCertRDNValueToStrW = (void*)GetProcAddress(dll, "CertRDNValueToStrW");
    pCertStrToNameA = (void*)GetProcAddress(dll,"CertStrToNameA");
    pCertStrToNameW = (void*)GetProcAddress(dll,"CertStrToNameW");
    pCertGetNameStringA = (void*)GetProcAddress(dll, "CertGetNameStringA");

    test_CertRDNValueToStrA();
    test_CertRDNValueToStrW();
    test_CertNameToStrA();
    test_CertNameToStrW();
    test_CertStrToNameA();
    test_CertStrToNameW();
    test_CertGetNameStringA();
}
