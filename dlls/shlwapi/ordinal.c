/*
 * SHLWAPI ordinal functions
 *
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
 *           2001 Jon Griffiths
 */

#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winnls.h"
#include "winbase.h"
#include "ddeml.h"
#include "shlobj.h"
#include "shellapi.h"
#include "commdlg.h"
#include "wine/unicode.h"
#include "wine/obj_base.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell);

extern HINSTANCE shlwapi_hInstance;
extern HMODULE SHLWAPI_hshell32;
extern HMODULE SHLWAPI_hwinmm;
extern HMODULE SHLWAPI_hcomdlg32;
extern HMODULE SHLWAPI_hmpr;
extern HMODULE SHLWAPI_hmlang;

typedef struct {
    INT     size;      /* [in]  (always 0x18)                       */
    LPCWSTR ap1;       /* [out] start of area (maybe after spaces)  */
    INT     sizep1;    /* [out] size of first part (until colon)    */
    LPCWSTR ap2;       /* [out] pointer following first colon       */
    INT     sizep2;    /* [out] size of remainder                   */
    INT     fcncde;    /* [out] function match of p1 (0 if unknown) */ 
} UNKNOWN_SHLWAPI_2;

typedef struct {
    INT    protocol_number;
    LPCSTR protocol_name;
} SHL_2_inet_protocol;

/* The following protocols were identified in the native version of
 * SHLWAPI.DLL version 5.50
 */
static const SHL_2_inet_protocol shlwapi_protocols[] = {
  {1, "ftp"},
  {2, "http"},
  {3, "gopher"},
  {4, "mailto"},
  {5, "news"},
  {6, "nntp"},
  {7, "telnet"},
  {8, "wais"},
  {9, "file"},
  {10, "mk"},
  {11, "https"},
  {12, "shell"},
  {13, "snews"},
  {14, "local"},
  {15, "javascript"},
  {16, "vbscript"},
  {17, "about"},
  {18, "res"},
  {0, 0}
};

/* Macro to get function pointer for a module*/
#define GET_FUNC(module, name, fail) \
  if (!SHLWAPI_h##module) SHLWAPI_h##module = LoadLibraryA(#module ".dll"); \
  if (!SHLWAPI_h##module) return fail; \
  if (!pfnFunc) pfnFunc = (void*)GetProcAddress(SHLWAPI_h##module, name); \
  if (!pfnFunc) return fail

/*
 NOTES: Most functions exported by ordinal seem to be superflous.
 The reason for these functions to be there is to provide a wraper
 for unicode functions to provide these functions on systems without
 unicode functions eg. win95/win98. Since we have such functions we just
 call these. If running Wine with native DLL's, some late bound calls may
 fail. However, its better to implement the functions in the forward DLL
 and recommend the builtin rather than reimplementing the calls here!
*/

/*************************************************************************
 *      @	[SHLWAPI.1]
 */
DWORD WINAPI SHLWAPI_1 (
	LPSTR lpStr,
	LPVOID x)
{
	FIXME("(%p %s %p %s)\n",lpStr, debugstr_a(lpStr),x, debugstr_a(x));
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.2]
 *
 * Identifies the Internet "protocol" requested in the passed string.
 * Also determines start and length of item after the ':'
 */
DWORD WINAPI SHLWAPI_2 (LPCWSTR x, UNKNOWN_SHLWAPI_2 *y)
{
    DWORD cnt;
    const SHL_2_inet_protocol *inet_pro;
    LPSTR cmpstr;
    INT len;

    if (y->size != 0x18) return E_INVALIDARG;
    /* FIXME: leading white space generates error of 0x80041001 which 
     *        is undefined
     */
    if (*x <= L' ') return 0x80041001;
    cnt = 0;
    y->sizep1 = 0;
    y->ap1 = x;
    while (*x) {
	if (*x == L':') {
	    y->sizep1 = cnt;
	    cnt = -1;
	    y->ap2 = x+1;
	    break;
	}
	x++;
	cnt++;
    }
    if (*x && y->sizep1) {
	y->sizep2 = lstrlenW(y->ap2);
    }

    len = WideCharToMultiByte(0, 0, y->ap1, y->sizep1, 0, 0, 0, 0);
    cmpstr = (LPSTR)HeapAlloc(GetProcessHeap(), 0, len+1);
    WideCharToMultiByte(0, 0, y->ap1, y->sizep1, cmpstr, len+1, 0, 0);
    y->fcncde = 0;
    inet_pro = shlwapi_protocols;
    while (inet_pro->protocol_number) {
	if (!strncasecmp(inet_pro->protocol_name, cmpstr,
		    min(len, lstrlenA(inet_pro->protocol_name)))) {
	    y->fcncde = inet_pro->protocol_number;
	    break;
	}
	inet_pro++;
    }
    HeapFree(GetProcessHeap(), 0, cmpstr);
    return S_OK;
}

/*************************************************************************
 *      SHLWAPI_15	[SHLWAPI.15]
 *
 * Function:
 *    Retrieves IE "AcceptLanguage" value from registry
 *  
 */
HRESULT WINAPI SHLWAPI_15 (
	LPWSTR langbuf,
	LPDWORD buflen)
{
	CHAR *mystr;
	DWORD mystrlen, mytype;
	HKEY mykey;
	LCID mylcid;

	mystrlen = *buflen;
	mystr = (CHAR*)HeapAlloc(GetProcessHeap(), 
				 HEAP_ZERO_MEMORY, mystrlen);
	RegOpenKeyA(HKEY_CURRENT_USER, 
		    "Software\\Microsoft\\Internet Explorer\\International", 
		    &mykey);
	if (RegQueryValueExA(mykey, "AcceptLanguage", 
			      0, &mytype, mystr, &mystrlen)) {
	    mylcid = GetUserDefaultLCID();
	    /* somehow the mylcid translates into "en-us"
	     *  this is similar to "LOCALE_SABBREVLANGNAME"
	     *  which could be gotten via GetLocaleInfo.
	     *  The only problem is LOCALE_SABBREVLANGUAGE" is
	     *  a 3 char string (first 2 are country code and third is
	     *  letter for "sublanguage", which does not come close to
	     *  "en-us"
	     */
	    lstrcpynA(mystr, "en-us", mystrlen);
	    mystrlen = lstrlenA(mystr);
	}
	else {
	    /* handle returned string */
	    FIXME("missing code\n");
	}
	RegCloseKey(mykey);
	*buflen = MultiByteToWideChar(0, 0, mystr, -1, langbuf, (*buflen)-1);
	HeapFree(GetProcessHeap(), 0, mystr);
	TRACE("language is %s\n", debugstr_w(langbuf));
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.16]
 */
HRESULT WINAPI SHLWAPI_16 (
	LPVOID w,
	LPVOID x,
	LPVOID y,
	LPWSTR z)
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	return 0xabba1252;
}

/*************************************************************************
 *      @	[SHLWAPI.23]
 *
 * NOTES
 *	converts a guid to a string
 *	returns strlen(str)
 */
DWORD WINAPI SHLWAPI_23 (
	REFGUID guid,	/* [in]  clsid */
	LPSTR str,	/* [out] buffer */
	INT cmax)	/* [in]  size of buffer */
{
	char xguid[40];

        sprintf( xguid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 guid->Data1, guid->Data2, guid->Data3,
                 guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                 guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
	TRACE("(%s %p 0x%08x)stub\n", xguid, str, cmax);
	if (strlen(xguid)>=cmax) return 0;
	strcpy(str,xguid);
	return strlen(xguid) + 1;
}

/*************************************************************************
 *      @	[SHLWAPI.24]
 *
 * NOTES
 *	converts a guid to a string
 *	returns strlen(str)
 */
DWORD WINAPI SHLWAPI_24 (
	REFGUID guid,	/* [in]  clsid */
	LPWSTR str,	/* [out] buffer */
	INT cmax)	/* [in]  size of buffer */
{
    char xguid[40];

    sprintf( xguid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid->Data1, guid->Data2, guid->Data3,
             guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
             guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    return MultiByteToWideChar( CP_ACP, 0, xguid, -1, str, cmax );
}

/*************************************************************************
 *      SHLWAPI_25	[SHLWAPI.25]
 *
 * Seems to be iswalpha
 */
BOOL WINAPI SHLWAPI_25(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_ALPHA) != 0;
}

/*************************************************************************
 *      SHLWAPI_29	[SHLWAPI.29]
 *
 * Seems to be iswspace
 */
BOOL WINAPI SHLWAPI_29(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_SPACE) != 0;
}

/*************************************************************************
 *      SHLWAPI_30	[SHLWAPI.30]
 *
 * Seems to be iswblank
 */
BOOL WINAPI SHLWAPI_30(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_BLANK) != 0;
}

/*************************************************************************
 *      SHLWAPI_31	[SHLWAPI.31]
 *
 * Seems to be iswpunct
 */
BOOL WINAPI SHLWAPI_31(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_PUNCT) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.32]
 */
BOOL WINAPI SHLWAPI_32(LPCWSTR lpcChar)
{
 if (*lpcChar < (WCHAR)' ')
   return TRUE;

 /* This is probably a shlwapi bug, but we do it the same for compatability */
 if (((DWORD)lpcChar & 0xffff) - 127 <= (WCHAR)' ')
   return TRUE;
 return FALSE;
}

/*************************************************************************
 *      SHLWAPI_33	[SHLWAPI.33]
 *
 * Seems to be iswdigit
 */
BOOL WINAPI SHLWAPI_33(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_DIGIT) != 0;
}

/*************************************************************************
 *      SHLWAPI_35	[SHLWAPI.35]
 *
 */
BOOL WINAPI SHLWAPI_35(LPVOID p1, DWORD dw2, LPVOID p3)
{
    FIXME("(%p, 0x%08lx, %p): stub\n", p1, dw2, p3);
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.40]
 *
 * Get pointer to next Unicode character.
 */
LPCWSTR WINAPI SHLWAPI_40(LPCWSTR str)
{
  return *str ? str + 1 : str;
}

/*************************************************************************
 *      @	[SHLWAPI.74]
 *
 * Get the text from a given dialog item.
 */
INT WINAPI SHLWAPI_74(HWND hWnd, INT nItem, LPWSTR lpsDest,INT nDestLen)
{
  HWND hItem = GetDlgItem(hWnd, nItem);

  if (hItem)
    return GetWindowTextW(hItem, lpsDest, nDestLen);
  if (nDestLen)
    *lpsDest = (WCHAR)'\0';
  return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.151]
 * Function:  Compare two ASCII strings for "len" bytes.
 * Returns:   *str1-*str2  (case sensitive)
 */
DWORD WINAPI SHLWAPI_151(LPSTR str1, LPSTR str2, INT len)
{
    return strncmp( str1, str2, len );
}

/*************************************************************************
 *      @	[SHLWAPI.152]
 *
 * Function:  Compare two WIDE strings for "len" bytes.
 * Returns:   *str1-*str2  (case sensitive)
 */
DWORD WINAPI SHLWAPI_152(LPWSTR str1, LPWSTR str2, INT len)
{
    return strncmpW( str1, str2, len );
}

/*************************************************************************
 *      @	[SHLWAPI.153]
 * Function:  Compare two ASCII strings for "len" bytes via caseless compare.
 * Returns:   *str1-*str2  (case insensitive)
 */
DWORD WINAPI SHLWAPI_153(LPSTR str1, LPSTR str2, DWORD len)
{
    return strncasecmp( str1, str2, len );
}

/*************************************************************************
 *      @	[SHLWAPI.154]
 *
 * Function:  Compare two WIDE strings for "len" bytes via caseless compare.
 * Returns:   *str1-*str2  (case insensitive)
 */
DWORD WINAPI SHLWAPI_154(LPWSTR str1, LPWSTR str2, DWORD len)
{
    return strncmpiW( str1, str2, len );
}

/*************************************************************************
 *      @	[SHLWAPI.156]
 *
 *	Case sensitive string compare. Does not SetLastError().
 */
DWORD WINAPI SHLWAPI_156 ( LPWSTR str1, LPWSTR str2)
{
    return strcmpW( str1, str2 );
}

/*************************************************************************
 *      @	[SHLWAPI.158]
 *
 *	Case insensitive string compare. Does not SetLastError(). ??
 */
DWORD WINAPI SHLWAPI_158 ( LPWSTR str1, LPWSTR str2)
{
    return strcmpiW( str1, str2 );
}

/*************************************************************************
 *      @	[SHLWAPI.162]
 *
 * Ensure a multibyte character string doesn't end in a hanging lead byte.
 */
DWORD WINAPI SHLWAPI_162(LPSTR lpStr, DWORD size)
{
  if (lpStr && size)
  {
    LPSTR lastByte = lpStr + size - 1;

    while(lpStr < lastByte)
      lpStr += IsDBCSLeadByte(*lpStr) ? 2 : 1;

    if(lpStr == lastByte && IsDBCSLeadByte(*lpStr))
    {
      *lpStr = '\0';
      size--;
    }
    return size;
  }
  return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.165]
 *
 * SetWindowLongA with mask.
 */
LONG WINAPI SHLWAPI_165(HWND hwnd, INT offset, UINT wFlags, UINT wMask)
{
  LONG ret = GetWindowLongA(hwnd, offset);
  UINT newFlags = (wFlags & wMask) | (ret & ~wFlags);

  if (newFlags != ret)
    ret = SetWindowLongA(hwnd, offset, newFlags);
  return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.169]
 */
DWORD WINAPI SHLWAPI_169 (IUnknown * lpUnknown)
{
	TRACE("(%p)\n",lpUnknown);
#if 0
	if(!lpUnknown || !*((LPDWORD)lpUnknown)) return 0;
	return IUnknown_Release(lpUnknown);
#endif
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.170]
 *
 * Skip URL '//' sequence.
 */
LPCSTR WINAPI SHLWAPI_170(LPCSTR lpszSrc)
{
  if (lpszSrc && lpszSrc[0] == '/' && lpszSrc[1] == '/')
    lpszSrc += 2;
  return lpszSrc;
}

/*************************************************************************
 *      @	[SHLWAPI.181]
 *
 *	Enable or disable a menu item.
 */
UINT WINAPI SHLWAPI_181(HMENU hMenu, UINT wItemID, BOOL bEnable)
{
  return EnableMenuItem(hMenu, wItemID, bEnable ? MF_ENABLED : MF_GRAYED);
}

/*************************************************************************
 *      @	[SHLWAPI.183]
 *
 * Register a window class if it isn't already.
 */
DWORD WINAPI SHLWAPI_183(WNDCLASSA *wndclass)
{
  WNDCLASSA wca;
  if (GetClassInfoA(wndclass->hInstance, wndclass->lpszClassName, &wca))
    return TRUE;
  return (DWORD)RegisterClassA(wndclass);
}

/*************************************************************************
 *      @	[SHLWAPI.193]
 */
DWORD WINAPI SHLWAPI_193 ()
{
	HDC hdc;
	DWORD ret;

	TRACE("()\n");

	hdc = GetDC(0);
	ret = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
	ReleaseDC(0, hdc);
	return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.215]
 *
 * NOTES
 *  check me!
 */
LPWSTR WINAPI SHLWAPI_215 (
	LPWSTR lpStrSrc,
	LPVOID lpwStrDest,
	int len)
{
	WARN("(%p %p %u)\n",lpStrSrc,lpwStrDest,len);
	return strncpyW(lpwStrDest, lpStrSrc, len);
}

/*************************************************************************
 *      @	[SHLWAPI.218]
 *
 * WideCharToMultiByte with multi language support.
 */
INT WINAPI SHLWAPI_218(UINT CodePage, LPCWSTR lpSrcStr, LPSTR lpDstStr,
                       LPINT lpnMultiCharCount)
{
  static HRESULT (WINAPI *pfnFunc)(LPDWORD,DWORD,LPCWSTR,LPINT,LPSTR,LPINT);
  WCHAR emptyW[] = { '\0' };
  int len , reqLen;
  LPSTR mem;

  if (!lpDstStr || !lpnMultiCharCount)
    return 0;

  if (!lpSrcStr)
    lpSrcStr = emptyW;

  *lpDstStr = '\0';

  len = strlenW(lpSrcStr) + 1;

  switch (CodePage)
  {
  case CP_WINUNICODE:
    CodePage = CP_UTF8; /* Fall through... */
  case 0x0000C350: /* FIXME: CP_ #define */
  case CP_UTF7:
  case CP_UTF8:
    {
      DWORD dwMode = 0;
      INT nWideCharCount = len - 1;

      GET_FUNC(mlang, "ConvertINetUnicodeToMultiByte", 0);
      if (!pfnFunc(&dwMode, CodePage, lpSrcStr, &nWideCharCount, lpDstStr,
                   lpnMultiCharCount))
        return 0;

      if (nWideCharCount < len - 1)
      {
        mem = (LPSTR)HeapAlloc(GetProcessHeap(), 0, *lpnMultiCharCount);
        if (!mem)
          return 0;

        *lpnMultiCharCount = 0;

        if (pfnFunc(&dwMode, CodePage, lpSrcStr, &len, mem, lpnMultiCharCount))
        {
          SHLWAPI_162 (mem, *lpnMultiCharCount);
          lstrcpynA(lpDstStr, mem, *lpnMultiCharCount + 1);
          return *lpnMultiCharCount + 1;
        }
        HeapFree(GetProcessHeap(), 0, mem);
        return *lpnMultiCharCount;
      }
      lpDstStr[*lpnMultiCharCount] = '\0';
      return *lpnMultiCharCount;
    }
    break;
  default:
    break;
  }

  reqLen = WideCharToMultiByte(CodePage, 0, lpSrcStr, len, lpDstStr,
                               *lpnMultiCharCount, NULL, NULL);

  if (!reqLen && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    reqLen = WideCharToMultiByte(CodePage, 0, lpSrcStr, len, NULL, 0, NULL, NULL);
    if (reqLen)
    {
      mem = (LPSTR)HeapAlloc(GetProcessHeap(), 0, reqLen);
      if (mem)
      {
        reqLen = WideCharToMultiByte(CodePage, 0, lpSrcStr, len, mem,
                                     reqLen, NULL, NULL);

        reqLen = SHLWAPI_162(mem, *lpnMultiCharCount);
        reqLen++;

        lstrcpynA(lpDstStr, mem, *lpnMultiCharCount);

        HeapFree(GetProcessHeap(), 0, mem);
      }
    }
  }
  return reqLen;
}

/*************************************************************************
 *      @	[SHLWAPI.217]
 *
 * Hmm, some program used lpnMultiCharCount == 0x3 (and lpSrcStr was "C")
 * --> Crash. Something wrong here.
 */
INT WINAPI SHLWAPI_217(LPCWSTR lpSrcStr, LPSTR lpDstStr, LPINT lpnMultiCharCount)
{
  return SHLWAPI_218(CP_ACP, lpSrcStr, lpDstStr, lpnMultiCharCount);
}

/*************************************************************************
 *      @	[SHLWAPI.219]
 *
 * NOTES
 *  error codes: E_POINTER, E_NOINTERFACE
 */
HRESULT WINAPI SHLWAPI_219 (
	LPVOID w, /* [???] NOTE: returned by LocalAlloc, 0x450 bytes, iface */
	LPVOID x, /* [???] */
	REFIID riid, /* [???] e.g. IWebBrowser2 */
	LPWSTR z) /* [???] NOTE: OUT: path */
{
	FIXME("(%p %s %s %p)stub\n",w,debugstr_a(x),debugstr_guid(riid),z);
	return 0xabba1252;
}

/*************************************************************************
 *      @	[SHLWAPI.222]
 *
 * NOTES
 *  securityattributes missing
 */
HANDLE WINAPI SHLWAPI_222 (LPCLSID guid)
{
	char lpstrName[80];

        sprintf( lpstrName, "shell.{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 guid->Data1, guid->Data2, guid->Data3,
                 guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                 guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
	FIXME("(%s) stub\n", lpstrName);
	return CreateSemaphoreA(NULL,0, 0x7fffffff, lpstrName);
}

/*************************************************************************
 *      @	[SHLWAPI.223]
 *
 * NOTES
 *  get the count of the semaphore
 */
DWORD WINAPI SHLWAPI_223 (HANDLE handle)
{
	DWORD oldCount;

	FIXME("(0x%08x) stub\n",handle);

	ReleaseSemaphore( handle, 1, &oldCount);	/* +1 */
	WaitForSingleObject( handle, 0 );		/* -1 */
	return oldCount;
}

/*************************************************************************
 *      @	[SHLWAPI.237]
 *
 * Unicode version of SHLWAPI_183.
 */
DWORD WINAPI SHLWAPI_237 (WNDCLASSW * lpWndClass)
{
	WNDCLASSW WndClass;

	TRACE("(0x%08x %s)\n",lpWndClass->hInstance, debugstr_w(lpWndClass->lpszClassName));

	if (GetClassInfoW(lpWndClass->hInstance, lpWndClass->lpszClassName, &WndClass))
		return TRUE;
	return RegisterClassW(lpWndClass);
}

/*************************************************************************
 *      @	[SHLWAPI.240]
 *
 *	Calls ASCII or Unicode WindowProc for the given window.
 */
LRESULT CALLBACK SHLWAPI_240(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	if (IsWindowUnicode(hWnd))
		return DefWindowProcW(hWnd, uMessage, wParam, lParam);
	return DefWindowProcA(hWnd, uMessage, wParam, lParam);
}

/*************************************************************************
 *      @	[SHLWAPI.241]
 *
 */
DWORD WINAPI SHLWAPI_241 ()
{
	FIXME("()stub\n");
	return 0xabba1243;
}

/*************************************************************************
 *      @	[SHLWAPI.266]
 */
DWORD WINAPI SHLWAPI_266 (
	LPVOID w,
	LPVOID x,
	LPVOID y,
	LPVOID z)
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	return 0xabba1248;
}

/*************************************************************************
 *      @	[SHLWAPI.267]
 */
HRESULT WINAPI SHLWAPI_267 (
	LPVOID w, /* [???] NOTE: same as 1th parameter of SHLWAPI_219 */
	LPVOID x, /* [???] NOTE: same as 2nd parameter of SHLWAPI_219 */
	LPVOID y,
	LPVOID z)
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	*((LPDWORD)z) = 0xabba1200;
	return 0xabba1254;
}

/*************************************************************************
 *      @	[SHLWAPI.268]
 */
DWORD WINAPI SHLWAPI_268 (
	LPVOID w,
	LPVOID x)
{
	FIXME("(%p %p)\n",w,x);
	return 0xabba1251; /* 0 = failure */
}

/*************************************************************************
 *      @	[SHLWAPI.276]
 *
 */
DWORD WINAPI SHLWAPI_276 ()
{
	FIXME("()stub\n");
	return 0xabba1244;
}

/*************************************************************************
 *      @	[SHLWAPI.278]
 *
 */
HWND WINAPI SHLWAPI_278 (
	LONG wndProc,
	HWND hWndParent,
	DWORD dwExStyle,
	DWORD dwStyle,
	HMENU hMenu,
	LONG z)
{
	WNDCLASSA wndclass;
	HWND hwnd;
	HCURSOR hCursor;
	char * clsname = "WorkerA";

	FIXME("(0x%08lx 0x%08x 0x%08lx 0x%08lx 0x%08x 0x%08lx)stub\n",
	  wndProc,hWndParent,dwExStyle,dwStyle,hMenu,z);

	hCursor = LoadCursorA(0x00000000,IDC_ARROWA);

	if(!GetClassInfoA(shlwapi_hInstance, clsname, &wndclass))
	{
	  RtlZeroMemory(&wndclass, sizeof(WNDCLASSA));
	  wndclass.lpfnWndProc = DefWindowProcW;
	  wndclass.cbWndExtra = 4;
	  wndclass.hInstance = shlwapi_hInstance;
	  wndclass.hCursor = hCursor;
	  wndclass.hbrBackground = COLOR_BTNSHADOW;
	  wndclass.lpszMenuName = NULL;
	  wndclass.lpszClassName = clsname;
	  RegisterClassA (&wndclass);
	}
	hwnd = CreateWindowExA(dwExStyle, clsname, 0,dwStyle,0,0,0,0,hWndParent,
		hMenu,shlwapi_hInstance,0);
	SetWindowLongA(hwnd, 0, z);
	SetWindowLongA(hwnd, GWL_WNDPROC, wndProc);
	return hwnd;
}

/*************************************************************************
 *      @	[SHLWAPI.289]
 *
 * Late bound call to winmm.PlaySoundW
 */
BOOL WINAPI SHLWAPI_289(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
  static BOOL (WINAPI *pfnFunc)(LPCWSTR, HMODULE, DWORD) = NULL;

  GET_FUNC(winmm, "PlaySoundW", FALSE);
  return pfnFunc(pszSound, hmod, fdwSound);
}

/*************************************************************************
 *      @	[SHLWAPI.294]
 */
BOOL WINAPI SHLWAPI_294(LPSTR str1, LPSTR str2, LPSTR pStr, DWORD some_len,  LPCSTR lpStr2)
{
    /*
     * str1:		"I"	"I"	pushl esp+0x20
     * str2:		"U"	"I"	pushl 0x77c93810
     * (is "I" and "U" "integer" and "unsigned" ??)
     *
     * pStr:		""	""	pushl eax
     * some_len:	0x824	0x104	pushl 0x824
     * lpStr2:		"%l"	"%l"	pushl esp+0xc
     *
     * shlwapi. StrCpyNW(lpStr2, irrelevant_var, 0x104);
     * LocalAlloc(0x00, some_len) -> irrelevant_var
     * LocalAlloc(0x40, irrelevant_len) -> pStr
     * shlwapi.294(str1, str2, pStr, some_len, lpStr2);
     * shlwapi.PathRemoveBlanksW(pStr);
     */
    ERR("('%s', '%s', '%s', %08lx, '%s'): stub!\n", str1, str2, pStr, some_len, lpStr2);
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.313]
 *
 * Late bound call to shell32.SHGetFileInfoW
 */
DWORD WINAPI SHLWAPI_313(LPCWSTR path, DWORD dwFileAttributes,
                         SHFILEINFOW *psfi, UINT sizeofpsfi, UINT flags)
{
  static DWORD (WINAPI *pfnFunc)(LPCWSTR,DWORD,SHFILEINFOW*,UINT,UINT) = NULL;

  GET_FUNC(shell32, "SHGetFileInfoW", 0);
  return pfnFunc(path, dwFileAttributes, psfi, sizeofpsfi, flags);
}

/*************************************************************************
 *      @	[SHLWAPI.318]
 *
 * Late bound call to shell32.DragQueryFileW
 */
UINT WINAPI SHLWAPI_318(HDROP hDrop, UINT lFile, LPWSTR lpszFile, UINT lLength)
{
  static UINT (WINAPI *pfnFunc)(HDROP, UINT, LPWSTR, UINT) = NULL;

  GET_FUNC(shell32, "DragQueryFileW", 0);
  return pfnFunc(hDrop, lFile, lpszFile, lLength);
}

/*************************************************************************
 *      @	[SHLWAPI.333]
 *
 * Late bound call to shell32.SHBrowseForFolderW
 */
LPITEMIDLIST WINAPI SHLWAPI_333(LPBROWSEINFOW lpBi)
{
  static LPITEMIDLIST (WINAPI *pfnFunc)(LPBROWSEINFOW) = NULL;

  GET_FUNC(shell32, "SHBrowseForFolderW", NULL);
  return pfnFunc(lpBi);
}

/*************************************************************************
 *      @	[SHLWAPI.334]
 *
 * Late bound call to shell32.SHGetPathFromIDListW
 */
BOOL WINAPI SHLWAPI_334(LPCITEMIDLIST pidl,LPWSTR pszPath)
{
  static BOOL (WINAPI *pfnFunc)(LPCITEMIDLIST, LPWSTR) = NULL;

  GET_FUNC(shell32, "SHGetPathFromIDListW", 0);
  return pfnFunc(pidl, pszPath);
}

/*************************************************************************
 *      @	[SHLWAPI.335]
 *
 * Late bound call to shell32.ShellExecuteExW
 */
BOOL WINAPI SHLWAPI_335(LPSHELLEXECUTEINFOW lpExecInfo)
{
  static BOOL (WINAPI *pfnFunc)(LPSHELLEXECUTEINFOW) = NULL;

  GET_FUNC(shell32, "ShellExecuteExW", FALSE);
  return pfnFunc(lpExecInfo);
}

/*************************************************************************
 *      @	[SHLWAPI.336]
 *
 * Late bound call to shell32.SHFileOperationW.
 */
DWORD WINAPI SHLWAPI_336(LPSHFILEOPSTRUCTW lpFileOp)
{
  static HICON (WINAPI *pfnFunc)(LPSHFILEOPSTRUCTW) = NULL;

  GET_FUNC(shell32, "SHFileOperationW", 0);
  return pfnFunc(lpFileOp);
}

/*************************************************************************
 *      @	[SHLWAPI.337]
 *
 * Late bound call to shell32.ExtractIconExW.
 */
HICON WINAPI SHLWAPI_337(LPCWSTR lpszFile, INT nIconIndex, HICON *phiconLarge,
                         HICON *phiconSmall, UINT nIcons)
{
  static HICON (WINAPI *pfnFunc)(LPCWSTR, INT,HICON *,HICON *, UINT) = NULL;

  GET_FUNC(shell32, "ExtractIconExW", (HICON)0);
  return pfnFunc(lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

/*************************************************************************
 *      @	[SHLWAPI.342]
 *
 */
DWORD WINAPI SHLWAPI_342 (
	LPVOID w,
	LPVOID x,
	LPVOID y,
	LPVOID z)
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	return 0xabba1249;
}

/*************************************************************************
 *      @	[SHLWAPI.346]
 */
DWORD WINAPI SHLWAPI_346 (
	LPCWSTR src,
	LPWSTR dest,
	int len)
{
	FIXME("(%s %p 0x%08x)stub\n",debugstr_w(src),dest,len);
	lstrcpynW(dest, src, len);
	return lstrlenW(dest)+1;
}

/*************************************************************************
 *      @	[SHLWAPI.357]
 *
 * Late bound call to shell32.SHGetNewLinkInfoW
 */
BOOL WINAPI SHLWAPI_357(LPCWSTR pszLinkTo, LPCWSTR pszDir, LPWSTR pszName,
                        BOOL *pfMustCopy, UINT uFlags)
{
  static BOOL (WINAPI *pfnFunc)(LPCWSTR, LPCWSTR, LPCWSTR, BOOL*, UINT) = NULL;

  GET_FUNC(shell32, "SHGetNewLinkInfoW", FALSE);
  return pfnFunc(pszLinkTo, pszDir, pszName, pfMustCopy, uFlags);
}

/*************************************************************************
 *      @	[SHLWAPI.358]
 *
 * Late bound call to shell32.SHDefExtractIconW
 */
DWORD WINAPI SHLWAPI_358(LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4,
                         LPVOID arg5, LPVOID arg6)
{
  /* FIXME: Correct args */
  static DWORD (WINAPI *pfnFunc)(LPVOID, LPVOID, LPVOID, LPVOID, LPVOID, LPVOID) = NULL;

  GET_FUNC(shell32, "SHDefExtractIconW", 0);
  return pfnFunc(arg1, arg2, arg3, arg4, arg5, arg6);
}

/*************************************************************************
 *      @	[SHLWAPI.364]
 *
 * Wrapper for lstrcpynA with src and dst swapped.
 */
DWORD WINAPI SHLWAPI_364(LPCSTR src, LPSTR dst, INT n)
{
  lstrcpynA(dst, src, n);
  return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.370]
 *
 * Late bound call to shell32.ExtractIconW
 */
HICON WINAPI SHLWAPI_370(HINSTANCE hInstance, LPCWSTR lpszExeFileName,
                         UINT nIconIndex)
{
  static HICON (WINAPI *pfnFunc)(HINSTANCE, LPCWSTR, UINT) = NULL;

  GET_FUNC(shell32, "ExtractIconW", (HICON)0);
  return pfnFunc(hInstance, lpszExeFileName, nIconIndex);
}

/*************************************************************************
 *      @	[SHLWAPI.376]
 */
DWORD WINAPI SHLWAPI_376 (LONG x)
{
	FIXME("(0x%08lx)stub\n", x );
  /* FIXME: This should be a forward in the .spec file to the win2k function
   * kernel32.GetUserDefaultUILanguage, however that function isn't there yet.
   */
  return 0xabba1245;
}

/*************************************************************************
 *      @	[SHLWAPI.377]
 */
DWORD WINAPI SHLWAPI_377 (LPVOID x, LPVOID y, LPVOID z)
{
	FIXME("(%p %p %p)stub\n", x,y,z);
	return 0xabba1246;
}

/*************************************************************************
 *      @	[SHLWAPI.378]
 */
DWORD WINAPI SHLWAPI_378 (
	LPSTR x,
	LPVOID y, /* [???] 0x50000000 */
	LPVOID z) /* [???] 4 */
{
	FIXME("(%s %p %p)stub\n", x,y,z);
	return LoadLibraryA(x);
}

/*************************************************************************
 *      @	[SHLWAPI.389]
 *
 * Late bound call to comdlg32.GetSaveFileNameW
 */
BOOL WINAPI SHLWAPI_389(LPOPENFILENAMEW ofn)
{
  static BOOL (WINAPI *pfnFunc)(LPOPENFILENAMEW) = NULL;

  GET_FUNC(comdlg32, "GetSaveFileNameW", FALSE);
  return pfnFunc(ofn);
}

/*************************************************************************
 *      @	[SHLWAPI.390]
 *
 * Late bound call to mpr.WNetRestoreConnectionW
 */
DWORD WINAPI SHLWAPI_390(LPVOID arg1, LPVOID arg2)
{
  /* FIXME: Correct args */
  static DWORD (WINAPI *pfnFunc)(LPVOID, LPVOID) = NULL;

  GET_FUNC(mpr, "WNetRestoreConnectionW", 0);
  return pfnFunc(arg1, arg2);
}

/*************************************************************************
 *      @	[SHLWAPI.391]
 *
 * Late bound call to mpr.WNetGetLastErrorW
 */
DWORD WINAPI SHLWAPI_391(LPVOID arg1, LPVOID arg2, LPVOID arg3, LPVOID arg4,
                         LPVOID arg5)
{
  /* FIXME: Correct args */
  static DWORD (WINAPI *pfnFunc)(LPVOID, LPVOID, LPVOID, LPVOID, LPVOID) = NULL;

  GET_FUNC(mpr, "WNetGetLastErrorW", 0);
  return pfnFunc(arg1, arg2, arg3, arg4, arg5);
}

/*************************************************************************
 *      @	[SHLWAPI.401]
 *
 * Late bound call to comdlg32.PageSetupDlgW
 */
BOOL WINAPI SHLWAPI_401(LPPAGESETUPDLGW pagedlg)
{
  static BOOL (WINAPI *pfnFunc)(LPPAGESETUPDLGW) = NULL;

  GET_FUNC(comdlg32, "PageSetupDlgW", FALSE);
  return pfnFunc(pagedlg);
}

/*************************************************************************
 *      @	[SHLWAPI.402]
 *
 * Late bound call to comdlg32.PrintDlgW
 */
BOOL WINAPI SHLWAPI_402(LPPRINTDLGW printdlg)
{
  static BOOL (WINAPI *pfnFunc)(LPPRINTDLGW) = NULL;

  GET_FUNC(comdlg32, "PrintDlgW", FALSE);
  return pfnFunc(printdlg);
}

/*************************************************************************
 *      @	[SHLWAPI.403]
 *
 * Late bound call to comdlg32.GetOpenFileNameW
 */
BOOL WINAPI SHLWAPI_403(LPOPENFILENAMEW ofn)
{
  static BOOL (WINAPI *pfnFunc)(LPOPENFILENAMEW) = NULL;

  GET_FUNC(comdlg32, "GetOpenFileNameW", FALSE);
  return pfnFunc(ofn);
}

/* INTERNAL: Map from HLS color space to RGB */
static WORD ConvertHue(int wHue, WORD wMid1, WORD wMid2)
{
  wHue = wHue > 240 ? wHue - 240 : wHue < 0 ? wHue + 240 : wHue;

  if (wHue > 160)
    return wMid1;
  else if (wHue > 120)
    wHue = 160 - wHue;
  else if (wHue > 40)
    return wMid2;

  return ((wHue * (wMid2 - wMid1) + 20) / 40) + wMid1;
}

/* Convert to RGB and scale into RGB range (0..255) */
#define GET_RGB(h) (ConvertHue(h, wMid1, wMid2) * 255 + 120) / 240

/*************************************************************************
 *      ColorHLSToRGB	[SHLWAPI.404]
 *
 * Convert from HLS color space into an RGB COLORREF.
 *
 * NOTES
 * Input HLS values are constrained to the range (0..240).
 */
COLORREF WINAPI ColorHLSToRGB(WORD wHue, WORD wLuminosity, WORD wSaturation)
{
  WORD wRed;

  if (wSaturation)
  {
    WORD wGreen, wBlue, wMid1, wMid2;

    if (wLuminosity > 120)
      wMid2 = wSaturation + wLuminosity - (wSaturation * wLuminosity + 120) / 240;
    else
      wMid2 = ((wSaturation + 240) * wLuminosity + 120) / 240;

    wMid1 = wLuminosity * 2 - wMid2;

    wRed   = GET_RGB(wHue + 80);
    wGreen = GET_RGB(wHue);
    wBlue  = GET_RGB(wHue - 80);

    return RGB(wRed, wGreen, wBlue);
  }

  wRed = wLuminosity * 255 / 240;
  return RGB(wRed, wRed, wRed);
}

/*************************************************************************
 *      @	[SHLWAPI.431]
 */
DWORD WINAPI SHLWAPI_431 (DWORD x)
{
	FIXME("(0x%08lx)stub\n", x);
	return 0xabba1247;
}

/*************************************************************************
 *      @	[SHLWAPI.437]
 *
 * NOTES
 *  In the real shlwapi, One time initialisation calls GetVersionEx and reads
 *  the registry to determine what O/S & Service Pack level is running, and
 *  therefore which functions are available. Currently we always run as NT,
 *  since this means that we don't need extra code to emulate Unicode calls,
 *  they are forwarded directly to the appropriate API call instead.
 *  Since the flags for whether to call or emulate Unicode are internal to
 *  the dll, this function does not need a full implementation.
 */
DWORD WINAPI SHLWAPI_437 (DWORD functionToCall)
{
	FIXME("(0x%08lx)stub\n", functionToCall);
	return 0xabba1247;
}

/*************************************************************************
 *      SHCreateShellPalette	[SHLWAPI.@]
 */
HPALETTE WINAPI SHCreateShellPalette(HDC hdc)
{
	FIXME("stub\n");
	return CreateHalftonePalette(hdc);
}

/*************************************************************************
 *	SHGetInverseCMAP (SHLWAPI.@)
 */
DWORD WINAPI SHGetInverseCMAP (LPVOID x, DWORD why)
{
	FIXME("(%p, %#lx)stub\n", x, why);
	return 0;
}

/*************************************************************************
 *      SHIsLowMemoryMachine	[SHLWAPI.@]
 */
DWORD WINAPI SHIsLowMemoryMachine (DWORD x)
{
	FIXME("0x%08lx\n", x);
	return 0;
}

/*************************************************************************
 *      GetMenuPosFromID	[SHLWAPI.@]
 */
INT WINAPI GetMenuPosFromID(HMENU hMenu, UINT wID)
{
 MENUITEMINFOA mi;
 INT nCount = GetMenuItemCount(hMenu), nIter = 0;

 while (nIter < nCount)
 {
   mi.wID = 0;
   if (!GetMenuItemInfoA(hMenu, nIter, TRUE, &mi) && mi.wID == wID)
     return nIter;
   nIter++;
 }
 return -1;
}

/*************************************************************************
 *      _SHGetInstanceExplorer@4	[SHLWAPI.@]
 *
 * Late bound call to shell32.SHGetInstanceExplorer.
 */
HRESULT WINAPI _SHGetInstanceExplorer (LPUNKNOWN *lpUnknown)
{
  static HRESULT (WINAPI *pfnFunc)(LPUNKNOWN *) = NULL;

  GET_FUNC(shell32, "SHGetInstanceExplorer", E_FAIL);
  return pfnFunc(lpUnknown);
}
