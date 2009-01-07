/* File generated automatically from tools/winapi/tests.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "shellapi.h"
#include "winuser.h"
#include "wingdi.h"
#include "shlobj.h"

#include "wine/test.h"

/***********************************************************************
 * Compatibility macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: May not be possible without a compiler extension
 *        (if type is not just a name that is, otherwise the normal
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if defined(TYPE_ALIGNMENT) && defined(_MSC_VER) && _MSC_VER >= 800 && !defined(__cplusplus)
#pragma warning(disable:4116)
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(_TYPE_ALIGNMENT(((type*)0)->field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           (int)_TYPE_ALIGNMENT(((type*)0)->field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             (long int)FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)_TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", ((int) sizeof(type)))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE((((type*)0)->field), field_size); \
  TEST_FIELD_ALIGNMENT(type, field, field_align); \
  TEST_FIELD_OFFSET(type, field, field_offset)

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

#define TEST_TYPE_POINTER(type, size, align) \
    TEST__TYPE_ALIGNMENT(*(type)0, align); \
    TEST_TYPE_SIZE(*(type)0, size)

#define TEST_TYPE_SIGNED(type) \
    ok((type) -1 < 0, "(" #type ") -1 < 0\n");

#define TEST_TYPE_UNSIGNED(type) \
     ok((type) -1 > 0, "(" #type ") -1 > 0\n");

static void test_pack_BLOB(void)
{
    /* BLOB (pack 4) */
    TEST_TYPE(BLOB, 8, 4);
    TEST_FIELD(BLOB, cbSize, 0, 4, 4);
    TEST_FIELD(BLOB, pBlobData, 4, 4, 4);
}

static void test_pack_BSTR(void)
{
    /* BSTR */
    TEST_TYPE(BSTR, 4, 4);
    TEST_TYPE_POINTER(BSTR, 2, 2);
}

static void test_pack_BSTRBLOB(void)
{
    /* BSTRBLOB (pack 4) */
    TEST_TYPE(BSTRBLOB, 8, 4);
    TEST_FIELD(BSTRBLOB, cbSize, 0, 4, 4);
    TEST_FIELD(BSTRBLOB, pData, 4, 4, 4);
}

static void test_pack_BYTE_BLOB(void)
{
    /* BYTE_BLOB (pack 4) */
    TEST_TYPE(BYTE_BLOB, 8, 4);
    TEST_FIELD(BYTE_BLOB, clSize, 0, 4, 4);
    TEST_FIELD(BYTE_BLOB, abData, 4, 1, 1);
}

static void test_pack_BYTE_SIZEDARR(void)
{
    /* BYTE_SIZEDARR (pack 4) */
    TEST_TYPE(BYTE_SIZEDARR, 8, 4);
    TEST_FIELD(BYTE_SIZEDARR, clSize, 0, 4, 4);
    TEST_FIELD(BYTE_SIZEDARR, pData, 4, 4, 4);
}

static void test_pack_CLIPDATA(void)
{
    /* CLIPDATA (pack 4) */
    TEST_TYPE(CLIPDATA, 12, 4);
    TEST_FIELD(CLIPDATA, cbSize, 0, 4, 4);
    TEST_FIELD(CLIPDATA, ulClipFmt, 4, 4, 4);
    TEST_FIELD(CLIPDATA, pClipData, 8, 4, 4);
}

static void test_pack_CLIPFORMAT(void)
{
    /* CLIPFORMAT */
    TEST_TYPE(CLIPFORMAT, 2, 2);
    TEST_TYPE_UNSIGNED(CLIPFORMAT);
}

static void test_pack_COAUTHIDENTITY(void)
{
    /* COAUTHIDENTITY (pack 4) */
    TEST_TYPE(COAUTHIDENTITY, 28, 4);
    TEST_FIELD(COAUTHIDENTITY, User, 0, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, UserLength, 4, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, Domain, 8, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, DomainLength, 12, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, Password, 16, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, PasswordLength, 20, 4, 4);
    TEST_FIELD(COAUTHIDENTITY, Flags, 24, 4, 4);
}

static void test_pack_COAUTHINFO(void)
{
    /* COAUTHINFO (pack 4) */
    TEST_TYPE(COAUTHINFO, 28, 4);
    TEST_FIELD(COAUTHINFO, dwAuthnSvc, 0, 4, 4);
    TEST_FIELD(COAUTHINFO, dwAuthzSvc, 4, 4, 4);
    TEST_FIELD(COAUTHINFO, pwszServerPrincName, 8, 4, 4);
    TEST_FIELD(COAUTHINFO, dwAuthnLevel, 12, 4, 4);
    TEST_FIELD(COAUTHINFO, dwImpersonationLevel, 16, 4, 4);
    TEST_FIELD(COAUTHINFO, pAuthIdentityData, 20, 4, 4);
    TEST_FIELD(COAUTHINFO, dwCapabilities, 24, 4, 4);
}

static void test_pack_DATE(void)
{
    /* DATE */
    TEST_TYPE(DATE, 8, 8);
}

static void test_pack_DOUBLE(void)
{
    /* DOUBLE */
    TEST_TYPE(DOUBLE, 8, 8);
}

static void test_pack_DWORD_SIZEDARR(void)
{
    /* DWORD_SIZEDARR (pack 4) */
    TEST_TYPE(DWORD_SIZEDARR, 8, 4);
    TEST_FIELD(DWORD_SIZEDARR, clSize, 0, 4, 4);
    TEST_FIELD(DWORD_SIZEDARR, pData, 4, 4, 4);
}

static void test_pack_FLAGGED_BYTE_BLOB(void)
{
    /* FLAGGED_BYTE_BLOB (pack 4) */
    TEST_TYPE(FLAGGED_BYTE_BLOB, 12, 4);
    TEST_FIELD(FLAGGED_BYTE_BLOB, fFlags, 0, 4, 4);
    TEST_FIELD(FLAGGED_BYTE_BLOB, clSize, 4, 4, 4);
    TEST_FIELD(FLAGGED_BYTE_BLOB, abData, 8, 1, 1);
}

static void test_pack_FLAGGED_WORD_BLOB(void)
{
    /* FLAGGED_WORD_BLOB (pack 4) */
    TEST_TYPE(FLAGGED_WORD_BLOB, 12, 4);
    TEST_FIELD(FLAGGED_WORD_BLOB, fFlags, 0, 4, 4);
    TEST_FIELD(FLAGGED_WORD_BLOB, clSize, 4, 4, 4);
    TEST_FIELD(FLAGGED_WORD_BLOB, asData, 8, 2, 2);
}

static void test_pack_HMETAFILEPICT(void)
{
    /* HMETAFILEPICT */
    TEST_TYPE(HMETAFILEPICT, 4, 4);
}

static void test_pack_HYPER_SIZEDARR(void)
{
    /* HYPER_SIZEDARR (pack 4) */
    TEST_TYPE(HYPER_SIZEDARR, 8, 4);
    TEST_FIELD(HYPER_SIZEDARR, clSize, 0, 4, 4);
    TEST_FIELD(HYPER_SIZEDARR, pData, 4, 4, 4);
}

static void test_pack_LPBLOB(void)
{
    /* LPBLOB */
    TEST_TYPE(LPBLOB, 4, 4);
}

static void test_pack_LPBSTR(void)
{
    /* LPBSTR */
    TEST_TYPE(LPBSTR, 4, 4);
    TEST_TYPE_POINTER(LPBSTR, 4, 4);
}

static void test_pack_LPBSTRBLOB(void)
{
    /* LPBSTRBLOB */
    TEST_TYPE(LPBSTRBLOB, 4, 4);
}

static void test_pack_LPCOLESTR(void)
{
    /* LPCOLESTR */
    TEST_TYPE(LPCOLESTR, 4, 4);
    TEST_TYPE_POINTER(LPCOLESTR, 2, 2);
}

static void test_pack_LPCY(void)
{
    /* LPCY */
    TEST_TYPE(LPCY, 4, 4);
}

static void test_pack_LPDECIMAL(void)
{
    /* LPDECIMAL */
    TEST_TYPE(LPDECIMAL, 4, 4);
}

static void test_pack_LPOLESTR(void)
{
    /* LPOLESTR */
    TEST_TYPE(LPOLESTR, 4, 4);
    TEST_TYPE_POINTER(LPOLESTR, 2, 2);
}

static void test_pack_OLECHAR(void)
{
    /* OLECHAR */
    TEST_TYPE(OLECHAR, 2, 2);
}

static void test_pack_PROPID(void)
{
    /* PROPID */
    TEST_TYPE(PROPID, 4, 4);
}

static void test_pack_RemHBITMAP(void)
{
    /* RemHBITMAP (pack 4) */
    TEST_TYPE(RemHBITMAP, 8, 4);
    TEST_FIELD(RemHBITMAP, cbData, 0, 4, 4);
    TEST_FIELD(RemHBITMAP, data, 4, 1, 1);
}

static void test_pack_RemHENHMETAFILE(void)
{
    /* RemHENHMETAFILE (pack 4) */
    TEST_TYPE(RemHENHMETAFILE, 8, 4);
    TEST_FIELD(RemHENHMETAFILE, cbData, 0, 4, 4);
    TEST_FIELD(RemHENHMETAFILE, data, 4, 1, 1);
}

static void test_pack_RemHGLOBAL(void)
{
    /* RemHGLOBAL (pack 4) */
    TEST_TYPE(RemHGLOBAL, 12, 4);
    TEST_FIELD(RemHGLOBAL, fNullHGlobal, 0, 4, 4);
    TEST_FIELD(RemHGLOBAL, cbData, 4, 4, 4);
    TEST_FIELD(RemHGLOBAL, data, 8, 1, 1);
}

static void test_pack_RemHMETAFILEPICT(void)
{
    /* RemHMETAFILEPICT (pack 4) */
    TEST_TYPE(RemHMETAFILEPICT, 20, 4);
    TEST_FIELD(RemHMETAFILEPICT, mm, 0, 4, 4);
    TEST_FIELD(RemHMETAFILEPICT, xExt, 4, 4, 4);
    TEST_FIELD(RemHMETAFILEPICT, yExt, 8, 4, 4);
    TEST_FIELD(RemHMETAFILEPICT, cbData, 12, 4, 4);
    TEST_FIELD(RemHMETAFILEPICT, data, 16, 1, 1);
}

static void test_pack_RemHPALETTE(void)
{
    /* RemHPALETTE (pack 4) */
    TEST_TYPE(RemHPALETTE, 8, 4);
    TEST_FIELD(RemHPALETTE, cbData, 0, 4, 4);
    TEST_FIELD(RemHPALETTE, data, 4, 1, 1);
}

static void test_pack_SCODE(void)
{
    /* SCODE */
    TEST_TYPE(SCODE, 4, 4);
}

static void test_pack_UP_BYTE_BLOB(void)
{
    /* UP_BYTE_BLOB */
    TEST_TYPE(UP_BYTE_BLOB, 4, 4);
    TEST_TYPE_POINTER(UP_BYTE_BLOB, 8, 4);
}

static void test_pack_UP_FLAGGED_BYTE_BLOB(void)
{
    /* UP_FLAGGED_BYTE_BLOB */
    TEST_TYPE(UP_FLAGGED_BYTE_BLOB, 4, 4);
    TEST_TYPE_POINTER(UP_FLAGGED_BYTE_BLOB, 12, 4);
}

static void test_pack_UP_FLAGGED_WORD_BLOB(void)
{
    /* UP_FLAGGED_WORD_BLOB */
    TEST_TYPE(UP_FLAGGED_WORD_BLOB, 4, 4);
    TEST_TYPE_POINTER(UP_FLAGGED_WORD_BLOB, 12, 4);
}

static void test_pack_VARIANT_BOOL(void)
{
    /* VARIANT_BOOL */
    TEST_TYPE(VARIANT_BOOL, 2, 2);
    TEST_TYPE_SIGNED(VARIANT_BOOL);
}

static void test_pack_VARTYPE(void)
{
    /* VARTYPE */
    TEST_TYPE(VARTYPE, 2, 2);
    TEST_TYPE_UNSIGNED(VARTYPE);
}

static void test_pack_WORD_SIZEDARR(void)
{
    /* WORD_SIZEDARR (pack 4) */
    TEST_TYPE(WORD_SIZEDARR, 8, 4);
    TEST_FIELD(WORD_SIZEDARR, clSize, 0, 4, 4);
    TEST_FIELD(WORD_SIZEDARR, pData, 4, 4, 4);
}

static void test_pack_remoteMETAFILEPICT(void)
{
    /* remoteMETAFILEPICT (pack 4) */
    TEST_TYPE(remoteMETAFILEPICT, 16, 4);
    TEST_FIELD(remoteMETAFILEPICT, mm, 0, 4, 4);
    TEST_FIELD(remoteMETAFILEPICT, xExt, 4, 4, 4);
    TEST_FIELD(remoteMETAFILEPICT, yExt, 8, 4, 4);
    TEST_FIELD(remoteMETAFILEPICT, hMF, 12, 4, 4);
}

static void test_pack_userBITMAP(void)
{
    /* userBITMAP (pack 4) */
    TEST_TYPE(userBITMAP, 28, 4);
    TEST_FIELD(userBITMAP, bmType, 0, 4, 4);
    TEST_FIELD(userBITMAP, bmWidth, 4, 4, 4);
    TEST_FIELD(userBITMAP, bmHeight, 8, 4, 4);
    TEST_FIELD(userBITMAP, bmWidthBytes, 12, 4, 4);
    TEST_FIELD(userBITMAP, bmPlanes, 16, 2, 2);
    TEST_FIELD(userBITMAP, bmBitsPixel, 18, 2, 2);
    TEST_FIELD(userBITMAP, cbSize, 20, 4, 4);
    TEST_FIELD(userBITMAP, pBuffer, 24, 1, 1);
}

static void test_pack_userCLIPFORMAT(void)
{
    /* userCLIPFORMAT (pack 4) */
    TEST_FIELD(userCLIPFORMAT, fContext, 0, 4, 4);
}

static void test_pack_userHBITMAP(void)
{
    /* userHBITMAP (pack 4) */
    TEST_FIELD(userHBITMAP, fContext, 0, 4, 4);
}

static void test_pack_userHENHMETAFILE(void)
{
    /* userHENHMETAFILE (pack 4) */
    TEST_FIELD(userHENHMETAFILE, fContext, 0, 4, 4);
}

static void test_pack_userHGLOBAL(void)
{
    /* userHGLOBAL (pack 4) */
    TEST_FIELD(userHGLOBAL, fContext, 0, 4, 4);
}

static void test_pack_userHMETAFILE(void)
{
    /* userHMETAFILE (pack 4) */
    TEST_FIELD(userHMETAFILE, fContext, 0, 4, 4);
}

static void test_pack_userHMETAFILEPICT(void)
{
    /* userHMETAFILEPICT (pack 4) */
    TEST_FIELD(userHMETAFILEPICT, fContext, 0, 4, 4);
}

static void test_pack_userHPALETTE(void)
{
    /* userHPALETTE (pack 4) */
    TEST_FIELD(userHPALETTE, fContext, 0, 4, 4);
}

static void test_pack_wireBSTR(void)
{
    /* wireBSTR */
    TEST_TYPE(wireBSTR, 4, 4);
    TEST_TYPE_POINTER(wireBSTR, 12, 4);
}

static void test_pack_wireCLIPFORMAT(void)
{
    /* wireCLIPFORMAT */
    TEST_TYPE(wireCLIPFORMAT, 4, 4);
}

static void test_pack_wireHBITMAP(void)
{
    /* wireHBITMAP */
    TEST_TYPE(wireHBITMAP, 4, 4);
}

static void test_pack_wireHENHMETAFILE(void)
{
    /* wireHENHMETAFILE */
    TEST_TYPE(wireHENHMETAFILE, 4, 4);
}

static void test_pack_wireHGLOBAL(void)
{
    /* wireHGLOBAL */
    TEST_TYPE(wireHGLOBAL, 4, 4);
}

static void test_pack_wireHMETAFILE(void)
{
    /* wireHMETAFILE */
    TEST_TYPE(wireHMETAFILE, 4, 4);
}

static void test_pack_wireHMETAFILEPICT(void)
{
    /* wireHMETAFILEPICT */
    TEST_TYPE(wireHMETAFILEPICT, 4, 4);
}

static void test_pack_wireHPALETTE(void)
{
    /* wireHPALETTE */
    TEST_TYPE(wireHPALETTE, 4, 4);
}

static void test_pack_CLSID(void)
{
    /* CLSID */
    TEST_TYPE(CLSID, 16, 4);
}

static void test_pack_FMTID(void)
{
    /* FMTID */
    TEST_TYPE(FMTID, 16, 4);
}

static void test_pack_IID(void)
{
    /* IID */
    TEST_TYPE(IID, 16, 4);
}

static void test_pack_APPBARDATA(void)
{
    /* APPBARDATA (pack 1) */
    TEST_TYPE(APPBARDATA, 36, 1);
    TEST_FIELD(APPBARDATA, cbSize, 0, 4, 1);
    TEST_FIELD(APPBARDATA, hWnd, 4, 4, 1);
    TEST_FIELD(APPBARDATA, uCallbackMessage, 8, 4, 1);
    TEST_FIELD(APPBARDATA, uEdge, 12, 4, 1);
    TEST_FIELD(APPBARDATA, rc, 16, 16, 1);
    TEST_FIELD(APPBARDATA, lParam, 32, 4, 1);
}

static void test_pack_DRAGINFOA(void)
{
    /* DRAGINFOA (pack 1) */
    TEST_TYPE(DRAGINFOA, 24, 1);
    TEST_FIELD(DRAGINFOA, uSize, 0, 4, 1);
    TEST_FIELD(DRAGINFOA, pt, 4, 8, 1);
    TEST_FIELD(DRAGINFOA, fNC, 12, 4, 1);
    TEST_FIELD(DRAGINFOA, lpFileList, 16, 4, 1);
    TEST_FIELD(DRAGINFOA, grfKeyState, 20, 4, 1);
}

static void test_pack_DRAGINFOW(void)
{
    /* DRAGINFOW (pack 1) */
    TEST_TYPE(DRAGINFOW, 24, 1);
    TEST_FIELD(DRAGINFOW, uSize, 0, 4, 1);
    TEST_FIELD(DRAGINFOW, pt, 4, 8, 1);
    TEST_FIELD(DRAGINFOW, fNC, 12, 4, 1);
    TEST_FIELD(DRAGINFOW, lpFileList, 16, 4, 1);
    TEST_FIELD(DRAGINFOW, grfKeyState, 20, 4, 1);
}

static void test_pack_FILEOP_FLAGS(void)
{
    /* FILEOP_FLAGS */
    TEST_TYPE(FILEOP_FLAGS, 2, 2);
    TEST_TYPE_UNSIGNED(FILEOP_FLAGS);
}

static void test_pack_LPDRAGINFOA(void)
{
    /* LPDRAGINFOA */
    TEST_TYPE(LPDRAGINFOA, 4, 4);
    TEST_TYPE_POINTER(LPDRAGINFOA, 24, 1);
}

static void test_pack_LPDRAGINFOW(void)
{
    /* LPDRAGINFOW */
    TEST_TYPE(LPDRAGINFOW, 4, 4);
    TEST_TYPE_POINTER(LPDRAGINFOW, 24, 1);
}

static void test_pack_LPSHELLEXECUTEINFOA(void)
{
    /* LPSHELLEXECUTEINFOA */
    TEST_TYPE(LPSHELLEXECUTEINFOA, 4, 4);
}

static void test_pack_LPSHELLEXECUTEINFOW(void)
{
    /* LPSHELLEXECUTEINFOW */
    TEST_TYPE(LPSHELLEXECUTEINFOW, 4, 4);
}

static void test_pack_LPSHFILEOPSTRUCTA(void)
{
    /* LPSHFILEOPSTRUCTA */
    TEST_TYPE(LPSHFILEOPSTRUCTA, 4, 4);
    TEST_TYPE_POINTER(LPSHFILEOPSTRUCTA, 30, 1);
}

static void test_pack_LPSHFILEOPSTRUCTW(void)
{
    /* LPSHFILEOPSTRUCTW */
    TEST_TYPE(LPSHFILEOPSTRUCTW, 4, 4);
    TEST_TYPE_POINTER(LPSHFILEOPSTRUCTW, 30, 1);
}

static void test_pack_LPSHNAMEMAPPINGA(void)
{
    /* LPSHNAMEMAPPINGA */
    TEST_TYPE(LPSHNAMEMAPPINGA, 4, 4);
    TEST_TYPE_POINTER(LPSHNAMEMAPPINGA, 16, 1);
}

static void test_pack_LPSHNAMEMAPPINGW(void)
{
    /* LPSHNAMEMAPPINGW */
    TEST_TYPE(LPSHNAMEMAPPINGW, 4, 4);
    TEST_TYPE_POINTER(LPSHNAMEMAPPINGW, 16, 1);
}

static void test_pack_NOTIFYICONDATAA(void)
{
    /* NOTIFYICONDATAA (pack 1) */
    TEST_FIELD(NOTIFYICONDATAA, cbSize, 0, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, hWnd, 4, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, uID, 8, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, uFlags, 12, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, uCallbackMessage, 16, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, hIcon, 20, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, szTip, 24, 128, 1);
    TEST_FIELD(NOTIFYICONDATAA, dwState, 152, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, dwStateMask, 156, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, szInfo, 160, 256, 1);
}

static void test_pack_NOTIFYICONDATAW(void)
{
    /* NOTIFYICONDATAW (pack 1) */
    TEST_FIELD(NOTIFYICONDATAW, cbSize, 0, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, hWnd, 4, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, uID, 8, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, uFlags, 12, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, uCallbackMessage, 16, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, hIcon, 20, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, szTip, 24, 256, 1);
    TEST_FIELD(NOTIFYICONDATAW, dwState, 280, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, dwStateMask, 284, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, szInfo, 288, 512, 1);
}

static void test_pack_PAPPBARDATA(void)
{
    /* PAPPBARDATA */
    TEST_TYPE(PAPPBARDATA, 4, 4);
    TEST_TYPE_POINTER(PAPPBARDATA, 36, 1);
}

static void test_pack_PNOTIFYICONDATAA(void)
{
    /* PNOTIFYICONDATAA */
    TEST_TYPE(PNOTIFYICONDATAA, 4, 4);
}

static void test_pack_PNOTIFYICONDATAW(void)
{
    /* PNOTIFYICONDATAW */
    TEST_TYPE(PNOTIFYICONDATAW, 4, 4);
}

static void test_pack_PRINTEROP_FLAGS(void)
{
    /* PRINTEROP_FLAGS */
    TEST_TYPE(PRINTEROP_FLAGS, 2, 2);
    TEST_TYPE_UNSIGNED(PRINTEROP_FLAGS);
}

static void test_pack_SHELLEXECUTEINFOA(void)
{
    /* SHELLEXECUTEINFOA (pack 1) */
    TEST_FIELD(SHELLEXECUTEINFOA, cbSize, 0, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, fMask, 4, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, hwnd, 8, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, lpVerb, 12, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, lpFile, 16, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, lpParameters, 20, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, lpDirectory, 24, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, nShow, 28, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, hInstApp, 32, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, lpIDList, 36, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, lpClass, 40, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, hkeyClass, 44, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, dwHotKey, 48, 4, 1);
}

static void test_pack_SHELLEXECUTEINFOW(void)
{
    /* SHELLEXECUTEINFOW (pack 1) */
    TEST_FIELD(SHELLEXECUTEINFOW, cbSize, 0, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, fMask, 4, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, hwnd, 8, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, lpVerb, 12, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, lpFile, 16, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, lpParameters, 20, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, lpDirectory, 24, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, nShow, 28, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, hInstApp, 32, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, lpIDList, 36, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, lpClass, 40, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, hkeyClass, 44, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, dwHotKey, 48, 4, 1);
}

static void test_pack_SHFILEINFOA(void)
{
    /* SHFILEINFOA (pack 1) */
    TEST_TYPE(SHFILEINFOA, 352, 1);
    TEST_FIELD(SHFILEINFOA, hIcon, 0, 4, 1);
    TEST_FIELD(SHFILEINFOA, iIcon, 4, 4, 1);
    TEST_FIELD(SHFILEINFOA, dwAttributes, 8, 4, 1);
    TEST_FIELD(SHFILEINFOA, szDisplayName, 12, 260, 1);
    TEST_FIELD(SHFILEINFOA, szTypeName, 272, 80, 1);
}

static void test_pack_SHFILEINFOW(void)
{
    /* SHFILEINFOW (pack 1) */
    TEST_TYPE(SHFILEINFOW, 692, 1);
    TEST_FIELD(SHFILEINFOW, hIcon, 0, 4, 1);
    TEST_FIELD(SHFILEINFOW, iIcon, 4, 4, 1);
    TEST_FIELD(SHFILEINFOW, dwAttributes, 8, 4, 1);
    TEST_FIELD(SHFILEINFOW, szDisplayName, 12, 520, 1);
    TEST_FIELD(SHFILEINFOW, szTypeName, 532, 160, 1);
}

static void test_pack_SHFILEOPSTRUCTA(void)
{
    /* SHFILEOPSTRUCTA (pack 1) */
    TEST_TYPE(SHFILEOPSTRUCTA, 30, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, hwnd, 0, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, wFunc, 4, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, pFrom, 8, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, pTo, 12, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, fFlags, 16, 2, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, fAnyOperationsAborted, 18, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, hNameMappings, 22, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, lpszProgressTitle, 26, 4, 1);
}

static void test_pack_SHFILEOPSTRUCTW(void)
{
    /* SHFILEOPSTRUCTW (pack 1) */
    TEST_TYPE(SHFILEOPSTRUCTW, 30, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, hwnd, 0, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, wFunc, 4, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, pFrom, 8, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, pTo, 12, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, fFlags, 16, 2, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, fAnyOperationsAborted, 18, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, hNameMappings, 22, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, lpszProgressTitle, 26, 4, 1);
}

static void test_pack_SHNAMEMAPPINGA(void)
{
    /* SHNAMEMAPPINGA (pack 1) */
    TEST_TYPE(SHNAMEMAPPINGA, 16, 1);
    TEST_FIELD(SHNAMEMAPPINGA, pszOldPath, 0, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGA, pszNewPath, 4, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGA, cchOldPath, 8, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGA, cchNewPath, 12, 4, 1);
}

static void test_pack_SHNAMEMAPPINGW(void)
{
    /* SHNAMEMAPPINGW (pack 1) */
    TEST_TYPE(SHNAMEMAPPINGW, 16, 1);
    TEST_FIELD(SHNAMEMAPPINGW, pszOldPath, 0, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGW, pszNewPath, 4, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGW, cchOldPath, 8, 4, 1);
    TEST_FIELD(SHNAMEMAPPINGW, cchNewPath, 12, 4, 1);
}

static void test_pack_ITEMIDLIST(void)
{
    /* ITEMIDLIST (pack 1) */
    TEST_TYPE(ITEMIDLIST, 3, 1);
    TEST_FIELD(ITEMIDLIST, mkid, 0, 3, 1);
}

static void test_pack_LPCITEMIDLIST(void)
{
    /* LPCITEMIDLIST */
    TEST_TYPE(LPCITEMIDLIST, 4, 4);
    TEST_TYPE_POINTER(LPCITEMIDLIST, 3, 1);
}

static void test_pack_LPCSHITEMID(void)
{
    /* LPCSHITEMID */
    TEST_TYPE(LPCSHITEMID, 4, 4);
    TEST_TYPE_POINTER(LPCSHITEMID, 3, 1);
}

static void test_pack_LPITEMIDLIST(void)
{
    /* LPITEMIDLIST */
    TEST_TYPE(LPITEMIDLIST, 4, 4);
}

static void test_pack_LPSHELLDETAILS(void)
{
    /* LPSHELLDETAILS */
    TEST_TYPE(LPSHELLDETAILS, 4, 4);
}

static void test_pack_LPSHITEMID(void)
{
    /* LPSHITEMID */
    TEST_TYPE(LPSHITEMID, 4, 4);
}

static void test_pack_LPSTRRET(void)
{
    /* LPSTRRET */
    TEST_TYPE(LPSTRRET, 4, 4);
}

static void test_pack_SHELLDETAILS(void)
{
    /* SHELLDETAILS (pack 1) */
    TEST_FIELD(SHELLDETAILS, fmt, 0, 4, 1);
    TEST_FIELD(SHELLDETAILS, cxChar, 4, 4, 1);
}

static void test_pack_SHITEMID(void)
{
    /* SHITEMID (pack 1) */
    TEST_TYPE(SHITEMID, 3, 1);
    TEST_FIELD(SHITEMID, cb, 0, 2, 1);
    TEST_FIELD(SHITEMID, abID, 2, 1, 1);
}

static void test_pack_STRRET(void)
{
    /* STRRET (pack 4) */
    TEST_FIELD(STRRET, uType, 0, 4, 4);
}

static void test_pack_AUTO_SCROLL_DATA(void)
{
    /* AUTO_SCROLL_DATA (pack 1) */
    TEST_TYPE(AUTO_SCROLL_DATA, 48, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, iNextSample, 0, 4, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, dwLastScroll, 4, 4, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, bFull, 8, 4, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, pts, 12, 24, 1);
    TEST_FIELD(AUTO_SCROLL_DATA, dwTimes, 36, 12, 1);
}

static void test_pack_BFFCALLBACK(void)
{
    /* BFFCALLBACK */
    TEST_TYPE(BFFCALLBACK, 4, 4);
}

static void test_pack_BROWSEINFOA(void)
{
    /* BROWSEINFOA (pack 8) */
    TEST_TYPE(BROWSEINFOA, 32, 4);
    TEST_FIELD(BROWSEINFOA, hwndOwner, 0, 4, 4);
    TEST_FIELD(BROWSEINFOA, pidlRoot, 4, 4, 4);
    TEST_FIELD(BROWSEINFOA, pszDisplayName, 8, 4, 4);
    TEST_FIELD(BROWSEINFOA, lpszTitle, 12, 4, 4);
    TEST_FIELD(BROWSEINFOA, ulFlags, 16, 4, 4);
    TEST_FIELD(BROWSEINFOA, lpfn, 20, 4, 4);
    TEST_FIELD(BROWSEINFOA, lParam, 24, 4, 4);
    TEST_FIELD(BROWSEINFOA, iImage, 28, 4, 4);
}

static void test_pack_BROWSEINFOW(void)
{
    /* BROWSEINFOW (pack 8) */
    TEST_TYPE(BROWSEINFOW, 32, 4);
    TEST_FIELD(BROWSEINFOW, hwndOwner, 0, 4, 4);
    TEST_FIELD(BROWSEINFOW, pidlRoot, 4, 4, 4);
    TEST_FIELD(BROWSEINFOW, pszDisplayName, 8, 4, 4);
    TEST_FIELD(BROWSEINFOW, lpszTitle, 12, 4, 4);
    TEST_FIELD(BROWSEINFOW, ulFlags, 16, 4, 4);
    TEST_FIELD(BROWSEINFOW, lpfn, 20, 4, 4);
    TEST_FIELD(BROWSEINFOW, lParam, 24, 4, 4);
    TEST_FIELD(BROWSEINFOW, iImage, 28, 4, 4);
}

static void test_pack_CABINETSTATE(void)
{
    /* CABINETSTATE (pack 1) */
    TEST_TYPE(CABINETSTATE, 12, 1);
    TEST_FIELD(CABINETSTATE, cLength, 0, 2, 1);
    TEST_FIELD(CABINETSTATE, nVersion, 2, 2, 1);
    TEST_FIELD(CABINETSTATE, fMenuEnumFilter, 8, 4, 1);
}

static void test_pack_CIDA(void)
{
    /* CIDA (pack 1) */
    TEST_TYPE(CIDA, 8, 1);
    TEST_FIELD(CIDA, cidl, 0, 4, 1);
    TEST_FIELD(CIDA, aoffset, 4, 4, 1);
}

static void test_pack_CSFV(void)
{
    /* CSFV (pack 1) */
    TEST_FIELD(CSFV, cbSize, 0, 4, 1);
    TEST_FIELD(CSFV, pshf, 4, 4, 1);
    TEST_FIELD(CSFV, psvOuter, 8, 4, 1);
    TEST_FIELD(CSFV, pidl, 12, 4, 1);
    TEST_FIELD(CSFV, lEvents, 16, 4, 1);
    TEST_FIELD(CSFV, pfnCallback, 20, 4, 1);
}

static void test_pack_DROPFILES(void)
{
    /* DROPFILES (pack 1) */
    TEST_TYPE(DROPFILES, 20, 1);
    TEST_FIELD(DROPFILES, pFiles, 0, 4, 1);
    TEST_FIELD(DROPFILES, pt, 4, 8, 1);
    TEST_FIELD(DROPFILES, fNC, 12, 4, 1);
    TEST_FIELD(DROPFILES, fWide, 16, 4, 1);
}

static void test_pack_FILEDESCRIPTORA(void)
{
    /* FILEDESCRIPTORA (pack 1) */
    TEST_TYPE(FILEDESCRIPTORA, 332, 1);
    TEST_FIELD(FILEDESCRIPTORA, dwFlags, 0, 4, 1);
    TEST_FIELD(FILEDESCRIPTORA, clsid, 4, 16, 1);
    TEST_FIELD(FILEDESCRIPTORA, sizel, 20, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, pointl, 28, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, dwFileAttributes, 36, 4, 1);
    TEST_FIELD(FILEDESCRIPTORA, ftCreationTime, 40, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, ftLastAccessTime, 48, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, ftLastWriteTime, 56, 8, 1);
    TEST_FIELD(FILEDESCRIPTORA, nFileSizeHigh, 64, 4, 1);
    TEST_FIELD(FILEDESCRIPTORA, nFileSizeLow, 68, 4, 1);
    TEST_FIELD(FILEDESCRIPTORA, cFileName, 72, 260, 1);
}

static void test_pack_FILEDESCRIPTORW(void)
{
    /* FILEDESCRIPTORW (pack 1) */
    TEST_TYPE(FILEDESCRIPTORW, 592, 1);
    TEST_FIELD(FILEDESCRIPTORW, dwFlags, 0, 4, 1);
    TEST_FIELD(FILEDESCRIPTORW, clsid, 4, 16, 1);
    TEST_FIELD(FILEDESCRIPTORW, sizel, 20, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, pointl, 28, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, dwFileAttributes, 36, 4, 1);
    TEST_FIELD(FILEDESCRIPTORW, ftCreationTime, 40, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, ftLastAccessTime, 48, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, ftLastWriteTime, 56, 8, 1);
    TEST_FIELD(FILEDESCRIPTORW, nFileSizeHigh, 64, 4, 1);
    TEST_FIELD(FILEDESCRIPTORW, nFileSizeLow, 68, 4, 1);
    TEST_FIELD(FILEDESCRIPTORW, cFileName, 72, 520, 1);
}

static void test_pack_FILEGROUPDESCRIPTORA(void)
{
    /* FILEGROUPDESCRIPTORA (pack 1) */
    TEST_TYPE(FILEGROUPDESCRIPTORA, 336, 1);
    TEST_FIELD(FILEGROUPDESCRIPTORA, cItems, 0, 4, 1);
    TEST_FIELD(FILEGROUPDESCRIPTORA, fgd, 4, 332, 1);
}

static void test_pack_FILEGROUPDESCRIPTORW(void)
{
    /* FILEGROUPDESCRIPTORW (pack 1) */
    TEST_TYPE(FILEGROUPDESCRIPTORW, 596, 1);
    TEST_FIELD(FILEGROUPDESCRIPTORW, cItems, 0, 4, 1);
    TEST_FIELD(FILEGROUPDESCRIPTORW, fgd, 4, 592, 1);
}

static void test_pack_LPBROWSEINFOA(void)
{
    /* LPBROWSEINFOA */
    TEST_TYPE(LPBROWSEINFOA, 4, 4);
    TEST_TYPE_POINTER(LPBROWSEINFOA, 32, 4);
}

static void test_pack_LPBROWSEINFOW(void)
{
    /* LPBROWSEINFOW */
    TEST_TYPE(LPBROWSEINFOW, 4, 4);
    TEST_TYPE_POINTER(LPBROWSEINFOW, 32, 4);
}

static void test_pack_LPCABINETSTATE(void)
{
    /* LPCABINETSTATE */
    TEST_TYPE(LPCABINETSTATE, 4, 4);
    TEST_TYPE_POINTER(LPCABINETSTATE, 12, 1);
}

static void test_pack_LPCSFV(void)
{
    /* LPCSFV */
    TEST_TYPE(LPCSFV, 4, 4);
}

static void test_pack_LPDROPFILES(void)
{
    /* LPDROPFILES */
    TEST_TYPE(LPDROPFILES, 4, 4);
    TEST_TYPE_POINTER(LPDROPFILES, 20, 1);
}

static void test_pack_LPFILEDESCRIPTORA(void)
{
    /* LPFILEDESCRIPTORA */
    TEST_TYPE(LPFILEDESCRIPTORA, 4, 4);
    TEST_TYPE_POINTER(LPFILEDESCRIPTORA, 332, 1);
}

static void test_pack_LPFILEDESCRIPTORW(void)
{
    /* LPFILEDESCRIPTORW */
    TEST_TYPE(LPFILEDESCRIPTORW, 4, 4);
    TEST_TYPE_POINTER(LPFILEDESCRIPTORW, 592, 1);
}

static void test_pack_LPFILEGROUPDESCRIPTORA(void)
{
    /* LPFILEGROUPDESCRIPTORA */
    TEST_TYPE(LPFILEGROUPDESCRIPTORA, 4, 4);
    TEST_TYPE_POINTER(LPFILEGROUPDESCRIPTORA, 336, 1);
}

static void test_pack_LPFILEGROUPDESCRIPTORW(void)
{
    /* LPFILEGROUPDESCRIPTORW */
    TEST_TYPE(LPFILEGROUPDESCRIPTORW, 4, 4);
    TEST_TYPE_POINTER(LPFILEGROUPDESCRIPTORW, 596, 1);
}

static void test_pack_LPFNVIEWCALLBACK(void)
{
    /* LPFNVIEWCALLBACK */
    TEST_TYPE(LPFNVIEWCALLBACK, 4, 4);
}

static void test_pack_LPIDA(void)
{
    /* LPIDA */
    TEST_TYPE(LPIDA, 4, 4);
    TEST_TYPE_POINTER(LPIDA, 8, 1);
}

static void test_pack_LPQCMINFO(void)
{
    /* LPQCMINFO */
    TEST_TYPE(LPQCMINFO, 4, 4);
    TEST_TYPE_POINTER(LPQCMINFO, 20, 4);
}

static void test_pack_LPSHChangeDWORDAsIDList(void)
{
    /* LPSHChangeDWORDAsIDList */
    TEST_TYPE(LPSHChangeDWORDAsIDList, 4, 4);
    TEST_TYPE_POINTER(LPSHChangeDWORDAsIDList, 12, 1);
}

static void test_pack_LPSHChangeProductKeyAsIDList(void)
{
    /* LPSHChangeProductKeyAsIDList */
    TEST_TYPE(LPSHChangeProductKeyAsIDList, 4, 4);
    TEST_TYPE_POINTER(LPSHChangeProductKeyAsIDList, 82, 1);
}

static void test_pack_LPSHDESCRIPTIONID(void)
{
    /* LPSHDESCRIPTIONID */
    TEST_TYPE(LPSHDESCRIPTIONID, 4, 4);
    TEST_TYPE_POINTER(LPSHDESCRIPTIONID, 20, 4);
}

static void test_pack_LPSHELLFLAGSTATE(void)
{
    /* LPSHELLFLAGSTATE */
    TEST_TYPE(LPSHELLFLAGSTATE, 4, 4);
    TEST_TYPE_POINTER(LPSHELLFLAGSTATE, 4, 1);
}

static void test_pack_LPSHELLSTATE(void)
{
    /* LPSHELLSTATE */
    TEST_TYPE(LPSHELLSTATE, 4, 4);
    TEST_TYPE_POINTER(LPSHELLSTATE, 32, 1);
}

static void test_pack_LPTBINFO(void)
{
    /* LPTBINFO */
    TEST_TYPE(LPTBINFO, 4, 4);
    TEST_TYPE_POINTER(LPTBINFO, 8, 4);
}

static void test_pack_PBROWSEINFOA(void)
{
    /* PBROWSEINFOA */
    TEST_TYPE(PBROWSEINFOA, 4, 4);
    TEST_TYPE_POINTER(PBROWSEINFOA, 32, 4);
}

static void test_pack_PBROWSEINFOW(void)
{
    /* PBROWSEINFOW */
    TEST_TYPE(PBROWSEINFOW, 4, 4);
    TEST_TYPE_POINTER(PBROWSEINFOW, 32, 4);
}

static void test_pack_QCMINFO(void)
{
    /* QCMINFO (pack 8) */
    TEST_TYPE(QCMINFO, 20, 4);
    TEST_FIELD(QCMINFO, hmenu, 0, 4, 4);
    TEST_FIELD(QCMINFO, indexMenu, 4, 4, 4);
    TEST_FIELD(QCMINFO, idCmdFirst, 8, 4, 4);
    TEST_FIELD(QCMINFO, idCmdLast, 12, 4, 4);
    TEST_FIELD(QCMINFO, pIdMap, 16, 4, 4);
}

static void test_pack_QCMINFO_IDMAP(void)
{
    /* QCMINFO_IDMAP (pack 8) */
    TEST_TYPE(QCMINFO_IDMAP, 12, 4);
    TEST_FIELD(QCMINFO_IDMAP, nMaxIds, 0, 4, 4);
    TEST_FIELD(QCMINFO_IDMAP, pIdList, 4, 8, 4);
}

static void test_pack_QCMINFO_IDMAP_PLACEMENT(void)
{
    /* QCMINFO_IDMAP_PLACEMENT (pack 8) */
    TEST_TYPE(QCMINFO_IDMAP_PLACEMENT, 8, 4);
    TEST_FIELD(QCMINFO_IDMAP_PLACEMENT, id, 0, 4, 4);
    TEST_FIELD(QCMINFO_IDMAP_PLACEMENT, fFlags, 4, 4, 4);
}

static void test_pack_SHChangeDWORDAsIDList(void)
{
    /* SHChangeDWORDAsIDList (pack 1) */
    TEST_TYPE(SHChangeDWORDAsIDList, 12, 1);
    TEST_FIELD(SHChangeDWORDAsIDList, cb, 0, 2, 1);
    TEST_FIELD(SHChangeDWORDAsIDList, dwItem1, 2, 4, 1);
    TEST_FIELD(SHChangeDWORDAsIDList, dwItem2, 6, 4, 1);
    TEST_FIELD(SHChangeDWORDAsIDList, cbZero, 10, 2, 1);
}

static void test_pack_SHChangeNotifyEntry(void)
{
    /* SHChangeNotifyEntry (pack 1) */
    TEST_TYPE(SHChangeNotifyEntry, 8, 1);
    TEST_FIELD(SHChangeNotifyEntry, pidl, 0, 4, 1);
    TEST_FIELD(SHChangeNotifyEntry, fRecursive, 4, 4, 1);
}

static void test_pack_SHChangeProductKeyAsIDList(void)
{
    /* SHChangeProductKeyAsIDList (pack 1) */
    TEST_TYPE(SHChangeProductKeyAsIDList, 82, 1);
    TEST_FIELD(SHChangeProductKeyAsIDList, cb, 0, 2, 1);
    TEST_FIELD(SHChangeProductKeyAsIDList, wszProductKey, 2, 78, 1);
    TEST_FIELD(SHChangeProductKeyAsIDList, cbZero, 80, 2, 1);
}

static void test_pack_SHDESCRIPTIONID(void)
{
    /* SHDESCRIPTIONID (pack 8) */
    TEST_TYPE(SHDESCRIPTIONID, 20, 4);
    TEST_FIELD(SHDESCRIPTIONID, dwDescriptionId, 0, 4, 4);
    TEST_FIELD(SHDESCRIPTIONID, clsid, 4, 16, 4);
}

static void test_pack_SHELLFLAGSTATE(void)
{
    /* SHELLFLAGSTATE (pack 1) */
    TEST_TYPE(SHELLFLAGSTATE, 4, 1);
}

static void test_pack_SHELLSTATE(void)
{
    /* SHELLSTATE (pack 1) */
    TEST_TYPE(SHELLSTATE, 32, 1);
    TEST_FIELD(SHELLSTATE, dwWin95Unused, 4, 4, 1);
    TEST_FIELD(SHELLSTATE, uWin95Unused, 8, 4, 1);
    TEST_FIELD(SHELLSTATE, lParamSort, 12, 4, 1);
    TEST_FIELD(SHELLSTATE, iSortDirection, 16, 4, 1);
    TEST_FIELD(SHELLSTATE, version, 20, 4, 1);
    TEST_FIELD(SHELLSTATE, uNotUsed, 24, 4, 1);
}

static void test_pack_TBINFO(void)
{
    /* TBINFO (pack 8) */
    TEST_TYPE(TBINFO, 8, 4);
    TEST_FIELD(TBINFO, cbuttons, 0, 4, 4);
    TEST_FIELD(TBINFO, uFlags, 4, 4, 4);
}

static void test_pack(void)
{
    test_pack_APPBARDATA();
    test_pack_AUTO_SCROLL_DATA();
    test_pack_BFFCALLBACK();
    test_pack_BLOB();
    test_pack_BROWSEINFOA();
    test_pack_BROWSEINFOW();
    test_pack_BSTR();
    test_pack_BSTRBLOB();
    test_pack_BYTE_BLOB();
    test_pack_BYTE_SIZEDARR();
    test_pack_CABINETSTATE();
    test_pack_CIDA();
    test_pack_CLIPDATA();
    test_pack_CLIPFORMAT();
    test_pack_CLSID();
    test_pack_COAUTHIDENTITY();
    test_pack_COAUTHINFO();
    test_pack_CSFV();
    test_pack_DATE();
    test_pack_DOUBLE();
    test_pack_DRAGINFOA();
    test_pack_DRAGINFOW();
    test_pack_DROPFILES();
    test_pack_DWORD_SIZEDARR();
    test_pack_FILEDESCRIPTORA();
    test_pack_FILEDESCRIPTORW();
    test_pack_FILEGROUPDESCRIPTORA();
    test_pack_FILEGROUPDESCRIPTORW();
    test_pack_FILEOP_FLAGS();
    test_pack_FLAGGED_BYTE_BLOB();
    test_pack_FLAGGED_WORD_BLOB();
    test_pack_FMTID();
    test_pack_HMETAFILEPICT();
    test_pack_HYPER_SIZEDARR();
    test_pack_IID();
    test_pack_ITEMIDLIST();
    test_pack_LPBLOB();
    test_pack_LPBROWSEINFOA();
    test_pack_LPBROWSEINFOW();
    test_pack_LPBSTR();
    test_pack_LPBSTRBLOB();
    test_pack_LPCABINETSTATE();
    test_pack_LPCITEMIDLIST();
    test_pack_LPCOLESTR();
    test_pack_LPCSFV();
    test_pack_LPCSHITEMID();
    test_pack_LPCY();
    test_pack_LPDECIMAL();
    test_pack_LPDRAGINFOA();
    test_pack_LPDRAGINFOW();
    test_pack_LPDROPFILES();
    test_pack_LPFILEDESCRIPTORA();
    test_pack_LPFILEDESCRIPTORW();
    test_pack_LPFILEGROUPDESCRIPTORA();
    test_pack_LPFILEGROUPDESCRIPTORW();
    test_pack_LPFNVIEWCALLBACK();
    test_pack_LPIDA();
    test_pack_LPITEMIDLIST();
    test_pack_LPOLESTR();
    test_pack_LPQCMINFO();
    test_pack_LPSHChangeDWORDAsIDList();
    test_pack_LPSHChangeProductKeyAsIDList();
    test_pack_LPSHDESCRIPTIONID();
    test_pack_LPSHELLDETAILS();
    test_pack_LPSHELLEXECUTEINFOA();
    test_pack_LPSHELLEXECUTEINFOW();
    test_pack_LPSHELLFLAGSTATE();
    test_pack_LPSHELLSTATE();
    test_pack_LPSHFILEOPSTRUCTA();
    test_pack_LPSHFILEOPSTRUCTW();
    test_pack_LPSHITEMID();
    test_pack_LPSHNAMEMAPPINGA();
    test_pack_LPSHNAMEMAPPINGW();
    test_pack_LPSTRRET();
    test_pack_LPTBINFO();
    test_pack_NOTIFYICONDATAA();
    test_pack_NOTIFYICONDATAW();
    test_pack_OLECHAR();
    test_pack_PAPPBARDATA();
    test_pack_PBROWSEINFOA();
    test_pack_PBROWSEINFOW();
    test_pack_PNOTIFYICONDATAA();
    test_pack_PNOTIFYICONDATAW();
    test_pack_PRINTEROP_FLAGS();
    test_pack_PROPID();
    test_pack_QCMINFO();
    test_pack_QCMINFO_IDMAP();
    test_pack_QCMINFO_IDMAP_PLACEMENT();
    test_pack_RemHBITMAP();
    test_pack_RemHENHMETAFILE();
    test_pack_RemHGLOBAL();
    test_pack_RemHMETAFILEPICT();
    test_pack_RemHPALETTE();
    test_pack_SCODE();
    test_pack_SHChangeDWORDAsIDList();
    test_pack_SHChangeNotifyEntry();
    test_pack_SHChangeProductKeyAsIDList();
    test_pack_SHDESCRIPTIONID();
    test_pack_SHELLDETAILS();
    test_pack_SHELLEXECUTEINFOA();
    test_pack_SHELLEXECUTEINFOW();
    test_pack_SHELLFLAGSTATE();
    test_pack_SHELLSTATE();
    test_pack_SHFILEINFOA();
    test_pack_SHFILEINFOW();
    test_pack_SHFILEOPSTRUCTA();
    test_pack_SHFILEOPSTRUCTW();
    test_pack_SHITEMID();
    test_pack_SHNAMEMAPPINGA();
    test_pack_SHNAMEMAPPINGW();
    test_pack_STRRET();
    test_pack_TBINFO();
    test_pack_UP_BYTE_BLOB();
    test_pack_UP_FLAGGED_BYTE_BLOB();
    test_pack_UP_FLAGGED_WORD_BLOB();
    test_pack_VARIANT_BOOL();
    test_pack_VARTYPE();
    test_pack_WORD_SIZEDARR();
    test_pack_remoteMETAFILEPICT();
    test_pack_userBITMAP();
    test_pack_userCLIPFORMAT();
    test_pack_userHBITMAP();
    test_pack_userHENHMETAFILE();
    test_pack_userHGLOBAL();
    test_pack_userHMETAFILE();
    test_pack_userHMETAFILEPICT();
    test_pack_userHPALETTE();
    test_pack_wireBSTR();
    test_pack_wireCLIPFORMAT();
    test_pack_wireHBITMAP();
    test_pack_wireHENHMETAFILE();
    test_pack_wireHGLOBAL();
    test_pack_wireHMETAFILE();
    test_pack_wireHMETAFILEPICT();
    test_pack_wireHPALETTE();
}

START_TEST(generated)
{
    test_pack();
}
