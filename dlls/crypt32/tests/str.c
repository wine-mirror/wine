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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winerror.h>
#include <wincrypt.h>

#include "wine/test.h"

/*#define DUMP_STRINGS*/
#ifdef DUMP_STRINGS
#include "wine/debug.h"
#endif

typedef struct _CertRDNAttrEncoding {
    LPCSTR pszObjId;
    DWORD  dwValueType;
    CERT_RDN_VALUE_BLOB Value;
    LPCSTR str;
} CertRDNAttrEncoding, *PCertRDNAttrEncoding;

typedef struct _CertRDNAttrEncodingW {
    LPCSTR pszObjId;
    DWORD  dwValueType;
    CERT_RDN_VALUE_BLOB Value;
    LPCWSTR str;
} CertRDNAttrEncodingW, *PCertRDNAttrEncodingW;

static const BYTE bin1[] = { 0x55, 0x53 };
static const BYTE bin2[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x73, 0x6f, 0x74,
 0x61 };
static const BYTE bin3[] = { 0x4d, 0x69, 0x6e, 0x6e, 0x65, 0x61, 0x70, 0x6f,
 0x6c, 0x69, 0x73 };
static const BYTE bin4[] = { 0x43, 0x6f, 0x64, 0x65, 0x57, 0x65, 0x61, 0x76,
 0x65, 0x72, 0x73 };
static const BYTE bin5[] = { 0x57, 0x69, 0x6e, 0x65, 0x20, 0x44, 0x65, 0x76,
 0x65, 0x6c, 0x6f, 0x70, 0x6d, 0x65, 0x6e, 0x74 };
static const BYTE bin6[] = { 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x68, 0x6f, 0x73,
 0x74 };
static const BYTE bin7[] = { 0x61, 0x72, 0x69, 0x63, 0x40, 0x63, 0x6f, 0x64,
 0x65, 0x77, 0x65, 0x61, 0x76, 0x65, 0x72, 0x73, 0x2e, 0x63, 0x6f, 0x6d };

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
static WCHAR issuerStrW[] = {
 'U','S',',',' ','M','i','n','n','e','s','o','t','a',',',' ','M','i','n','n',
 'e','a','p','o','l','i','s',',',' ','C','o','d','e','W','e','a','v','e','r',
 's',',',' ','W','i','n','e',' ','D','e','v','e','l','o','p','m','e','n','t',
 ',',' ','l','o','c','a','l','h','o','s','t',',',' ','a','r','i','c','@','c',
 'o','d','e','w','e','a','v','e','r','s','.','c','o','m',0 };
static WCHAR issuerStrSemicolonW[] = {
 'U','S',';',' ','M','i','n','n','e','s','o','t','a',';',' ','M','i','n','n',
 'e','a','p','o','l','i','s',';',' ','C','o','d','e','W','e','a','v','e','r',
 's',';',' ','W','i','n','e',' ','D','e','v','e','l','o','p','m','e','n','t',
 ';',' ','l','o','c','a','l','h','o','s','t',';',' ','a','r','i','c','@','c',
 'o','d','e','w','e','a','v','e','r','s','.','c','o','m',0 };
static WCHAR issuerStrCRLFW[] = {
 'U','S','\r','\n','M','i','n','n','e','s','o','t','a','\r','\n','M','i','n',
 'n','e','a','p','o','l','i','s','\r','\n','C','o','d','e','W','e','a','v','e',
 'r','s','\r','\n','W','i','n','e',' ','D','e','v','e','l','o','p','m','e','n',
 't','\r','\n','l','o','c','a','l','h','o','s','t','\r','\n','a','r','i','c',
 '@','c','o','d','e','w','e','a','v','e','r','s','.','c','o','m',0 };
static WCHAR subjectStrW[] = {
 '2','.','5','.','4','.','6','=','U','S',',',' ','2','.','5','.','4','.','8',
 '=','M','i','n','n','e','s','o','t','a',',',' ','2','.','5','.','4','.','7',
 '=','M','i','n','n','e','a','p','o','l','i','s',',',' ','2','.','5','.','4',
 '.','1','0','=','C','o','d','e','W','e','a','v','e','r','s',',',' ','2','.',
 '5','.','4','.','1','1','=','W','i','n','e',' ','D','e','v','e','l','o','p',
 'm','e','n','t',',',' ','2','.','5','.','4','.','3','=','l','o','c','a','l',
 'h','o','s','t',',',' ','1','.','2','.','8','4','0','.','1','1','3','5','4',
 '9','.','1','.','9','.','1','=','a','r','i','c','@','c','o','d','e','w','e',
 'a','v','e','r','s','.','c','o','m',0 };
static WCHAR subjectStrSemicolonW[] = {
 '2','.','5','.','4','.','6','=','U','S',';',' ','2','.','5','.','4','.','8',
 '=','M','i','n','n','e','s','o','t','a',';',' ','2','.','5','.','4','.','7',
 '=','M','i','n','n','e','a','p','o','l','i','s',';',' ','2','.','5','.','4',
 '.','1','0','=','C','o','d','e','W','e','a','v','e','r','s',';',' ','2','.',
 '5','.','4','.','1','1','=','W','i','n','e',' ','D','e','v','e','l','o','p',
 'm','e','n','t',';',' ','2','.','5','.','4','.','3','=','l','o','c','a','l',
 'h','o','s','t',';',' ','1','.','2','.','8','4','0','.','1','1','3','5','4',
 '9','.','1','.','9','.','1','=','a','r','i','c','@','c','o','d','e','w','e',
 'a','v','e','r','s','.','c','o','m',0 };
static WCHAR subjectStrCRLFW[] = {
 '2','.','5','.','4','.','6','=','U','S','\r','\n','2','.','5','.','4','.','8',
 '=','M','i','n','n','e','s','o','t','a','\r','\n','2','.','5','.','4','.','7',
 '=','M','i','n','n','e','a','p','o','l','i','s','\r','\n','2','.','5','.','4',
 '.','1','0','=','C','o','d','e','W','e','a','v','e','r','s','\r','\n','2','.',
 '5','.','4','.','1','1','=','W','i','n','e',' ','D','e','v','e','l','o','p',
 'm','e','n','t','\r','\n','2','.','5','.','4','.','3','=','l','o','c','a','l',
 'h','o','s','t','\r','\n','1','.','2','.','8','4','0','.','1','1','3','5','4',
 '9','.','1','.','9','.','1','=','a','r','i','c','@','c','o','d','e','w','e',
 'a','v','e','r','s','.','c','o','m',0 };

typedef BOOL (WINAPI *CryptDecodeObjectFunc)(DWORD, LPCSTR, const BYTE *,
 DWORD, DWORD, void *, DWORD *);
typedef DWORD (WINAPI *CertNameToStrAFunc)(DWORD,LPVOID,DWORD,LPSTR,DWORD);
typedef DWORD (WINAPI *CertNameToStrWFunc)(DWORD,LPVOID,DWORD,LPWSTR,DWORD);
typedef DWORD (WINAPI *CertRDNValueToStrAFunc)(DWORD, PCERT_RDN_VALUE_BLOB,
 LPSTR, DWORD);
typedef DWORD (WINAPI *CertRDNValueToStrWFunc)(DWORD, PCERT_RDN_VALUE_BLOB,
 LPWSTR, DWORD);

HMODULE dll;
static CertNameToStrAFunc pCertNameToStrA;
static CertNameToStrWFunc pCertNameToStrW;
static CryptDecodeObjectFunc pCryptDecodeObject;
static CertRDNValueToStrAFunc pCertRDNValueToStrA;
static CertRDNValueToStrWFunc pCertRDNValueToStrW;

static void test_CertRDNValueToStrA(void)
{
    CertRDNAttrEncoding attrs[] = {
     { "2.5.4.6", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin1), (PBYTE)bin1 }, "US" },
     { "2.5.4.8", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin2), (PBYTE)bin2 }, "Minnesota" },
     { "2.5.4.7", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin3), (PBYTE)bin3 }, "Minneapolis" },
     { "2.5.4.10", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin4), (PBYTE)bin4 }, "CodeWeavers" },
     { "2.5.4.11", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin5), (PBYTE)bin5 }, "Wine Development" },
     { "2.5.4.3", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin6), (PBYTE)bin6 }, "localhost" },
     { "1.2.840.113549.1.9.1", CERT_RDN_IA5_STRING,
       { sizeof(bin7), (PBYTE)bin7 }, "aric@codeweavers.com" },
    };
    DWORD i, ret;
    char buffer[2000];
    CERT_RDN_VALUE_BLOB blob = { 0, NULL };

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

    for (i = 0; i < sizeof(attrs) / sizeof(attrs[0]); i++)
    {
        ret = pCertRDNValueToStrA(attrs[i].dwValueType, &attrs[i].Value,
         buffer, sizeof(buffer));
        ok(ret == strlen(attrs[i].str) + 1, "Expected length %d, got %ld\n",
         strlen(attrs[i].str) + 1, ret);
        ok(!strcmp(buffer, attrs[i].str), "Expected %s, got %s\n", attrs[i].str,
         buffer);
    }
}

static void test_CertRDNValueToStrW(void)
{
    static const WCHAR usW[] = { 'U','S',0 };
    static const WCHAR minnesotaW[] = { 'M','i','n','n','e','s','o','t','a',0 };
    static const WCHAR minneapolisW[] = { 'M','i','n','n','e','a','p','o','l',
     'i','s',0 };
    static const WCHAR codeweaversW[] = { 'C','o','d','e','W','e','a','v','e',
     'r','s',0 };
    static const WCHAR wineDevW[] = { 'W','i','n','e',' ','D','e','v','e','l',
     'o','p','m','e','n','t',0 };
    static const WCHAR localhostW[] = { 'l','o','c','a','l','h','o','s','t',0 };
    static const WCHAR aricW[] = { 'a','r','i','c','@','c','o','d','e','w','e',
     'a','v','e','r','s','.','c','o','m',0 };
    CertRDNAttrEncodingW attrs[] = {
     { "2.5.4.6", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin1), (PBYTE)bin1 }, usW },
     { "2.5.4.8", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin2), (PBYTE)bin2 }, minnesotaW },
     { "2.5.4.7", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin3), (PBYTE)bin3 }, minneapolisW },
     { "2.5.4.10", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin4), (PBYTE)bin4 }, codeweaversW },
     { "2.5.4.11", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin5), (PBYTE)bin5 }, wineDevW },
     { "2.5.4.3", CERT_RDN_PRINTABLE_STRING,
       { sizeof(bin6), (PBYTE)bin6 }, localhostW },
     { "1.2.840.113549.1.9.1", CERT_RDN_IA5_STRING,
       { sizeof(bin7), (PBYTE)bin7 }, aricW },
    };
    DWORD i, ret;
    WCHAR buffer[2000];
    CERT_RDN_VALUE_BLOB blob = { 0, NULL };

    if (!pCertRDNValueToStrW) return;

    /* This crashes
    ret = pCertRDNValueToStrW(0, NULL, NULL, 0);
     */
    /* With empty input, it generates the empty string */
    SetLastError(0xdeadbeef);
    ret = pCertRDNValueToStrW(0, &blob, NULL, 0);
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ret = pCertRDNValueToStrW(0, &blob, buffer,
     sizeof(buffer) / sizeof(buffer[0]));
    ok(ret == 1 && GetLastError() == 0xdeadbeef, "Expected empty string\n");
    ok(!buffer[0], "Expected empty string\n");

    for (i = 0; i < sizeof(attrs) / sizeof(attrs[0]); i++)
    {
        ret = pCertRDNValueToStrW(attrs[i].dwValueType, &attrs[i].Value,
         buffer, sizeof(buffer) / sizeof(buffer[0]));
        ok(ret == lstrlenW(attrs[i].str) + 1, "Expected length %d, got %ld\n",
         lstrlenW(attrs[i].str) + 1, ret);
        ok(!lstrcmpW(buffer, attrs[i].str), "Unexpected value\n");
#ifdef DUMP_STRINGS
        trace("Expected %s, got %s\n",
         wine_dbgstr_w(attrs[i].str), wine_dbgstr_w(buffer));
#endif
    }
}

static void test_NameToStrConversionA(PCERT_NAME_BLOB pName, DWORD dwStrType,
 LPCSTR expected)
{
    char buffer[2000] = { 0 };
    DWORD i;

    i = pCertNameToStrA(X509_ASN_ENCODING,pName, dwStrType, buffer,
     sizeof(buffer));
    ok(i == strlen(expected) + 1, "Expected %d chars, got %ld\n",
     strlen(expected) + 1, i);
    ok(!strcmp(buffer, expected), "Expected %s, got %s\n", expected, buffer);
}

static void test_CertNameToStrA(void)
{
    PCCERT_CONTEXT context;

    if (!pCertNameToStrA) return;

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
         CERT_SIMPLE_NAME_STR, issuerStr);
        test_NameToStrConversionA(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         issuerStrSemicolon);
        test_NameToStrConversionA(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         issuerStrCRLF);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR, subjectStr);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         subjectStrSemicolon);
        test_NameToStrConversionA(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         subjectStrCRLF);

        CertFreeCertificateContext(context);
    }
}

static void test_NameToStrConversionW(PCERT_NAME_BLOB pName, DWORD dwStrType,
 LPCWSTR expected)
{
    WCHAR buffer[2000] = { 0 };
    DWORD i;

    i = pCertNameToStrW(X509_ASN_ENCODING,pName, dwStrType, buffer,
     sizeof(buffer) / sizeof(buffer[0]));
    ok(i == lstrlenW(expected) + 1, "Expected %d chars, got %ld\n",
     lstrlenW(expected) + 1, i);
    ok(!lstrcmpW(buffer, expected), "Unexpected value\n");
#ifdef DUMP_STRINGS
    trace("Expected %s, got %s\n",
     wine_dbgstr_w(expected), wine_dbgstr_w(buffer));
#endif
}

static void test_CertNameToStrW(void)
{
    PCCERT_CONTEXT context;

    if (!pCertNameToStrW) return;

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
         CERT_SIMPLE_NAME_STR, issuerStrW);
        test_NameToStrConversionW(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         issuerStrSemicolonW);
        test_NameToStrConversionW(&context->pCertInfo->Issuer,
         CERT_SIMPLE_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         issuerStrCRLFW);
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR, subjectStrW);
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_SEMICOLON_FLAG,
         subjectStrSemicolonW);
        test_NameToStrConversionW(&context->pCertInfo->Subject,
         CERT_OID_NAME_STR | CERT_NAME_STR_CRLF_FLAG,
         subjectStrCRLFW);

        CertFreeCertificateContext(context);
    }
}

START_TEST(str)
{
    dll = LoadLibrary("Crypt32.dll");

    pCertNameToStrA = (CertNameToStrAFunc)GetProcAddress(dll,"CertNameToStrA");
    pCertNameToStrW = (CertNameToStrWFunc)GetProcAddress(dll,"CertNameToStrW");
    pCertRDNValueToStrA = (CertRDNValueToStrAFunc)GetProcAddress(dll,
     "CertRDNValueToStrA");
    pCertRDNValueToStrW = (CertRDNValueToStrWFunc)GetProcAddress(dll,
     "CertRDNValueToStrW");
    pCryptDecodeObject = (CryptDecodeObjectFunc)GetProcAddress(dll,
     "CryptDecodeObject");

    test_CertRDNValueToStrA();
    test_CertRDNValueToStrW();
    test_CertNameToStrA();
    test_CertNameToStrW();

    FreeLibrary(dll);
}
