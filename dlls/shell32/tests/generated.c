/* File generated automatically from tools/winapi/test.dat; do not edit! */
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
 * Compability macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define FIELD_ALIGNMENT(type, field) __alignof(((type*)0)->field)
#elif defined(__GNUC__)
# define FIELD_ALIGNMENT(type, field) __alignof__(((type*)0)->field)
#else
/* FIXME: Not sure if is possible to do without compiler extension */
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: Not sure if is possible to do without compiler extension
 *        (if type is not just a name that is, if so the normal)
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(FIELD_ALIGNMENT(type, field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           FIELD_ALIGNMENT(type, field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", _TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", sizeof(type))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field_type, field_name, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE(field_type, field_size); \
  TEST_FIELD_ALIGNMENT(type, field_name, field_align); \
  TEST_FIELD_OFFSET(type, field_name, field_offset); \

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

static void test_pack_APPBARDATA(void)
{
    /* APPBARDATA (pack 1) */
    TEST_TYPE(APPBARDATA, 36, 1);
    TEST_FIELD(APPBARDATA, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(APPBARDATA, HWND, hWnd, 4, 4, 1);
    TEST_FIELD(APPBARDATA, UINT, uCallbackMessage, 8, 4, 1);
    TEST_FIELD(APPBARDATA, UINT, uEdge, 12, 4, 1);
    TEST_FIELD(APPBARDATA, RECT, rc, 16, 16, 1);
    TEST_FIELD(APPBARDATA, LPARAM, lParam, 32, 4, 1);
}

static void test_pack_DRAGINFOA(void)
{
    /* DRAGINFOA (pack 1) */
    TEST_FIELD(DRAGINFOA, UINT, uSize, 0, 4, 1);
    TEST_FIELD(DRAGINFOA, POINT, pt, 4, 8, 1);
    TEST_FIELD(DRAGINFOA, BOOL, fNC, 12, 4, 1);
}

static void test_pack_DRAGINFOW(void)
{
    /* DRAGINFOW (pack 1) */
    TEST_TYPE(DRAGINFOW, 24, 1);
    TEST_FIELD(DRAGINFOW, UINT, uSize, 0, 4, 1);
    TEST_FIELD(DRAGINFOW, POINT, pt, 4, 8, 1);
    TEST_FIELD(DRAGINFOW, BOOL, fNC, 12, 4, 1);
    TEST_FIELD(DRAGINFOW, LPWSTR, lpFileList, 16, 4, 1);
    TEST_FIELD(DRAGINFOW, DWORD, grfKeyState, 20, 4, 1);
}

static void test_pack_FILEOP_FLAGS(void)
{
    /* FILEOP_FLAGS */
    TEST_TYPE(FILEOP_FLAGS, 2, 2);
}

static void test_pack_LPDRAGINFOA(void)
{
    /* LPDRAGINFOA */
    TEST_TYPE(LPDRAGINFOA, 4, 4);
}

static void test_pack_LPDRAGINFOW(void)
{
    /* LPDRAGINFOW */
    TEST_TYPE(LPDRAGINFOW, 4, 4);
    TEST_TYPE_POINTER(LPDRAGINFOW, 24, 1);
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

static void test_pack_NOTIFYICONDATAA(void)
{
    /* NOTIFYICONDATAA (pack 1) */
    TEST_TYPE(NOTIFYICONDATAA, 88, 1);
    TEST_FIELD(NOTIFYICONDATAA, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, HWND, hWnd, 4, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, UINT, uID, 8, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, UINT, uFlags, 12, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, UINT, uCallbackMessage, 16, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, HICON, hIcon, 20, 4, 1);
    TEST_FIELD(NOTIFYICONDATAA, CHAR[64], szTip, 24, 64, 1);
}

static void test_pack_NOTIFYICONDATAW(void)
{
    /* NOTIFYICONDATAW (pack 1) */
    TEST_TYPE(NOTIFYICONDATAW, 152, 1);
    TEST_FIELD(NOTIFYICONDATAW, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, HWND, hWnd, 4, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, UINT, uID, 8, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, UINT, uFlags, 12, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, UINT, uCallbackMessage, 16, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, HICON, hIcon, 20, 4, 1);
    TEST_FIELD(NOTIFYICONDATAW, WCHAR[64], szTip, 24, 128, 1);
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
    TEST_TYPE_POINTER(PNOTIFYICONDATAA, 88, 1);
}

static void test_pack_PNOTIFYICONDATAW(void)
{
    /* PNOTIFYICONDATAW */
    TEST_TYPE(PNOTIFYICONDATAW, 4, 4);
    TEST_TYPE_POINTER(PNOTIFYICONDATAW, 152, 1);
}

static void test_pack_PRINTEROP_FLAGS(void)
{
    /* PRINTEROP_FLAGS */
    TEST_TYPE(PRINTEROP_FLAGS, 2, 2);
}

static void test_pack_SHFILEINFOA(void)
{
    /* SHFILEINFOA (pack 1) */
    TEST_TYPE(SHFILEINFOA, 352, 1);
    TEST_FIELD(SHFILEINFOA, HICON, hIcon, 0, 4, 1);
    TEST_FIELD(SHFILEINFOA, int, iIcon, 4, 4, 1);
    TEST_FIELD(SHFILEINFOA, DWORD, dwAttributes, 8, 4, 1);
    TEST_FIELD(SHFILEINFOA, CHAR[MAX_PATH], szDisplayName, 12, 260, 1);
    TEST_FIELD(SHFILEINFOA, CHAR[80], szTypeName, 272, 80, 1);
}

static void test_pack_SHFILEINFOW(void)
{
    /* SHFILEINFOW (pack 1) */
    TEST_TYPE(SHFILEINFOW, 692, 1);
    TEST_FIELD(SHFILEINFOW, HICON, hIcon, 0, 4, 1);
    TEST_FIELD(SHFILEINFOW, int, iIcon, 4, 4, 1);
    TEST_FIELD(SHFILEINFOW, DWORD, dwAttributes, 8, 4, 1);
    TEST_FIELD(SHFILEINFOW, WCHAR[MAX_PATH], szDisplayName, 12, 520, 1);
    TEST_FIELD(SHFILEINFOW, WCHAR[80], szTypeName, 532, 160, 1);
}

static void test_pack_SHFILEOPSTRUCTA(void)
{
    /* SHFILEOPSTRUCTA (pack 1) */
    TEST_TYPE(SHFILEOPSTRUCTA, 30, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, HWND, hwnd, 0, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, UINT, wFunc, 4, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, LPCSTR, pFrom, 8, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, LPCSTR, pTo, 12, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, FILEOP_FLAGS, fFlags, 16, 2, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, BOOL, fAnyOperationsAborted, 18, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, LPVOID, hNameMappings, 22, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTA, LPCSTR, lpszProgressTitle, 26, 4, 1);
}

static void test_pack_SHFILEOPSTRUCTW(void)
{
    /* SHFILEOPSTRUCTW (pack 1) */
    TEST_TYPE(SHFILEOPSTRUCTW, 30, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, HWND, hwnd, 0, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, UINT, wFunc, 4, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, LPCWSTR, pFrom, 8, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, LPCWSTR, pTo, 12, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, FILEOP_FLAGS, fFlags, 16, 2, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, BOOL, fAnyOperationsAborted, 18, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, LPVOID, hNameMappings, 22, 4, 1);
    TEST_FIELD(SHFILEOPSTRUCTW, LPCWSTR, lpszProgressTitle, 26, 4, 1);
}

static void test_pack_BFFCALLBACK(void)
{
    /* BFFCALLBACK */
    TEST_TYPE(BFFCALLBACK, 4, 4);
}

static void test_pack_BROWSEINFOA(void)
{
    /* BROWSEINFOA (pack 1) */
    TEST_FIELD(BROWSEINFOA, HWND, hwndOwner, 0, 4, 1);
    TEST_FIELD(BROWSEINFOA, LPCITEMIDLIST, pidlRoot, 4, 4, 1);
}

static void test_pack_BROWSEINFOW(void)
{
    /* BROWSEINFOW (pack 1) */
    TEST_TYPE(BROWSEINFOW, 32, 1);
    TEST_FIELD(BROWSEINFOW, HWND, hwndOwner, 0, 4, 1);
    TEST_FIELD(BROWSEINFOW, LPCITEMIDLIST, pidlRoot, 4, 4, 1);
    TEST_FIELD(BROWSEINFOW, LPWSTR, pszDisplayName, 8, 4, 1);
    TEST_FIELD(BROWSEINFOW, LPCWSTR, lpszTitle, 12, 4, 1);
    TEST_FIELD(BROWSEINFOW, UINT, ulFlags, 16, 4, 1);
    TEST_FIELD(BROWSEINFOW, BFFCALLBACK, lpfn, 20, 4, 1);
    TEST_FIELD(BROWSEINFOW, LPARAM, lParam, 24, 4, 1);
    TEST_FIELD(BROWSEINFOW, INT, iImage, 28, 4, 1);
}

static void test_pack_CIDA(void)
{
    /* CIDA (pack 1) */
    TEST_TYPE(CIDA, 8, 1);
    TEST_FIELD(CIDA, UINT, cidl, 0, 4, 1);
    TEST_FIELD(CIDA, UINT[1], aoffset, 4, 4, 1);
}

static void test_pack_DROPFILES(void)
{
    /* DROPFILES (pack 1) */
    TEST_TYPE(DROPFILES, 20, 1);
    TEST_FIELD(DROPFILES, DWORD, pFiles, 0, 4, 1);
    TEST_FIELD(DROPFILES, POINT, pt, 4, 8, 1);
    TEST_FIELD(DROPFILES, BOOL, fNC, 12, 4, 1);
    TEST_FIELD(DROPFILES, BOOL, fWide, 16, 4, 1);
}

static void test_pack_IShellIcon(void)
{
    /* IShellIcon */
}

static void test_pack_ITEMIDLIST(void)
{
    /* ITEMIDLIST (pack 1) */
    TEST_TYPE(ITEMIDLIST, 3, 1);
    TEST_FIELD(ITEMIDLIST, SHITEMID, mkid, 0, 3, 1);
}

static void test_pack_LPBROWSEINFOA(void)
{
    /* LPBROWSEINFOA */
    TEST_TYPE(LPBROWSEINFOA, 4, 4);
}

static void test_pack_LPBROWSEINFOW(void)
{
    /* LPBROWSEINFOW */
    TEST_TYPE(LPBROWSEINFOW, 4, 4);
    TEST_TYPE_POINTER(LPBROWSEINFOW, 32, 1);
}

static void test_pack_LPCITEMIDLIST(void)
{
    /* LPCITEMIDLIST */
    TEST_TYPE(LPCITEMIDLIST, 4, 4);
    TEST_TYPE_POINTER(LPCITEMIDLIST, 3, 1);
}

static void test_pack_LPIDA(void)
{
    /* LPIDA */
    TEST_TYPE(LPIDA, 4, 4);
    TEST_TYPE_POINTER(LPIDA, 8, 1);
}

static void test_pack_LPITEMIDLIST(void)
{
    /* LPITEMIDLIST */
    TEST_TYPE(LPITEMIDLIST, 4, 4);
    TEST_TYPE_POINTER(LPITEMIDLIST, 3, 1);
}

static void test_pack_LPSHDESCRIPTIONID(void)
{
    /* LPSHDESCRIPTIONID */
    TEST_TYPE(LPSHDESCRIPTIONID, 4, 4);
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

static void test_pack_LPSHITEMID(void)
{
    /* LPSHITEMID */
    TEST_TYPE(LPSHITEMID, 4, 4);
    TEST_TYPE_POINTER(LPSHITEMID, 3, 1);
}

static void test_pack_PBROWSEINFOA(void)
{
    /* PBROWSEINFOA */
    TEST_TYPE(PBROWSEINFOA, 4, 4);
}

static void test_pack_PBROWSEINFOW(void)
{
    /* PBROWSEINFOW */
    TEST_TYPE(PBROWSEINFOW, 4, 4);
    TEST_TYPE_POINTER(PBROWSEINFOW, 32, 1);
}

static void test_pack_SHDESCRIPTIONID(void)
{
    /* SHDESCRIPTIONID (pack 1) */
    TEST_FIELD(SHDESCRIPTIONID, DWORD, dwDescriptionId, 0, 4, 1);
}

static void test_pack_SHELLEXECUTEINFOA(void)
{
    /* SHELLEXECUTEINFOA (pack 1) */
    TEST_FIELD(SHELLEXECUTEINFOA, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, ULONG, fMask, 4, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, HWND, hwnd, 8, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpVerb, 12, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpFile, 16, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpParameters, 20, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpDirectory, 24, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, INT, nShow, 28, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, HINSTANCE, hInstApp, 32, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPVOID, lpIDList, 36, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOA, LPCSTR, lpClass, 40, 4, 1);
}

static void test_pack_SHELLEXECUTEINFOW(void)
{
    /* SHELLEXECUTEINFOW (pack 1) */
    TEST_FIELD(SHELLEXECUTEINFOW, DWORD, cbSize, 0, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, ULONG, fMask, 4, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, HWND, hwnd, 8, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpVerb, 12, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpFile, 16, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpParameters, 20, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpDirectory, 24, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, INT, nShow, 28, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, HINSTANCE, hInstApp, 32, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPVOID, lpIDList, 36, 4, 1);
    TEST_FIELD(SHELLEXECUTEINFOW, LPCWSTR, lpClass, 40, 4, 1);
}

static void test_pack_SHELLVIEWID(void)
{
    /* SHELLVIEWID */
}

static void test_pack_SHITEMID(void)
{
    /* SHITEMID (pack 1) */
    TEST_TYPE(SHITEMID, 3, 1);
    TEST_FIELD(SHITEMID, WORD, cb, 0, 2, 1);
    TEST_FIELD(SHITEMID, BYTE[1], abID, 2, 1, 1);
}

static void test_pack(void)
{
    test_pack_APPBARDATA();
    test_pack_BFFCALLBACK();
    test_pack_BROWSEINFOA();
    test_pack_BROWSEINFOW();
    test_pack_CIDA();
    test_pack_DRAGINFOA();
    test_pack_DRAGINFOW();
    test_pack_DROPFILES();
    test_pack_FILEOP_FLAGS();
    test_pack_IShellIcon();
    test_pack_ITEMIDLIST();
    test_pack_LPBROWSEINFOA();
    test_pack_LPBROWSEINFOW();
    test_pack_LPCITEMIDLIST();
    test_pack_LPDRAGINFOA();
    test_pack_LPDRAGINFOW();
    test_pack_LPIDA();
    test_pack_LPITEMIDLIST();
    test_pack_LPSHDESCRIPTIONID();
    test_pack_LPSHELLEXECUTEINFOA();
    test_pack_LPSHELLEXECUTEINFOW();
    test_pack_LPSHFILEOPSTRUCTA();
    test_pack_LPSHFILEOPSTRUCTW();
    test_pack_LPSHITEMID();
    test_pack_NOTIFYICONDATAA();
    test_pack_NOTIFYICONDATAW();
    test_pack_PAPPBARDATA();
    test_pack_PBROWSEINFOA();
    test_pack_PBROWSEINFOW();
    test_pack_PNOTIFYICONDATAA();
    test_pack_PNOTIFYICONDATAW();
    test_pack_PRINTEROP_FLAGS();
    test_pack_SHDESCRIPTIONID();
    test_pack_SHELLEXECUTEINFOA();
    test_pack_SHELLEXECUTEINFOW();
    test_pack_SHELLVIEWID();
    test_pack_SHFILEINFOA();
    test_pack_SHFILEINFOW();
    test_pack_SHFILEOPSTRUCTA();
    test_pack_SHFILEOPSTRUCTW();
    test_pack_SHITEMID();
}

START_TEST(generated)
{
    test_pack();
}
