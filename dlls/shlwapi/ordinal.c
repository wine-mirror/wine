/*
 * SHLWAPI ordinal functions
 *
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
 *           2001 Jon Griffiths
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

#define COM_NO_WINDOWS_H
#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>

#include "docobj.h"
#include "shlguid.h"
#include "windef.h"
#include "winnls.h"
#include "winbase.h"
#include "ddeml.h"
#include "shlobj.h"
#include "shellapi.h"
#include "commdlg.h"
#include "wine/unicode.h"
#include "wine/obj_serviceprovider.h"
#include "wine/obj_control.h"
#include "wine/obj_connection.h"
#include "wine/obj_property.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"
#include "wine/debug.h"
#include "shlwapi.h"
#include "ordinal.h"


WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* Get a function pointer from a DLL handle */
#define GET_FUNC(func, module, name, fail) \
  do { \
    if (!func) { \
      if (!SHLWAPI_h##module && !(SHLWAPI_h##module = LoadLibraryA(#module ".dll"))) return fail; \
      if (!(func = (void*)GetProcAddress(SHLWAPI_h##module, name))) return fail; \
    } \
  } while (0)

/* DLL handles for late bound calls */
extern HINSTANCE shlwapi_hInstance;
extern HMODULE SHLWAPI_hshell32;
extern HMODULE SHLWAPI_hwinmm;
extern HMODULE SHLWAPI_hcomdlg32;
extern HMODULE SHLWAPI_hcomctl32;
extern HMODULE SHLWAPI_hmpr;
extern HMODULE SHLWAPI_hmlang;
extern HMODULE SHLWAPI_hversion;

extern DWORD SHLWAPI_ThreadRef_index;

typedef HANDLE HSHARED; /* Shared memory */

/* following is GUID for IObjectWithSite::SetSite  -- see _174           */
static DWORD id1[4] = {0xfc4801a3, 0x11cf2ba9, 0xaa0029a2, 0x52733d00};
/* following is GUID for IPersistMoniker::GetClassID  -- see _174        */
static DWORD id2[4] = {0x79eac9ee, 0x11cebaf9, 0xaa00828c, 0x0ba94b00};

/* Function pointers for GET_FUNC macro; these need to be global because of gcc bug */
static LPITEMIDLIST (WINAPI *pSHBrowseForFolderW)(LPBROWSEINFOW);
static HRESULT (WINAPI *pConvertINetUnicodeToMultiByte)(LPDWORD,DWORD,LPCWSTR,LPINT,LPSTR,LPINT);
static BOOL    (WINAPI *pPlaySoundW)(LPCWSTR, HMODULE, DWORD);
static DWORD   (WINAPI *pSHGetFileInfoW)(LPCWSTR,DWORD,SHFILEINFOW*,UINT,UINT);
static UINT    (WINAPI *pDragQueryFileW)(HDROP, UINT, LPWSTR, UINT);
static BOOL    (WINAPI *pSHGetPathFromIDListW)(LPCITEMIDLIST, LPWSTR);
static BOOL    (WINAPI *pShellExecuteExW)(LPSHELLEXECUTEINFOW);
static HICON   (WINAPI *pSHFileOperationW)(LPSHFILEOPSTRUCTW);
static UINT    (WINAPI *pExtractIconExW)(LPCWSTR, INT,HICON *,HICON *, UINT);
static BOOL    (WINAPI *pSHGetNewLinkInfoW)(LPCWSTR, LPCWSTR, LPCWSTR, BOOL*, UINT);
static HRESULT (WINAPI *pSHDefExtractIconW)(LPCWSTR, int, UINT, HICON*, HICON*, UINT);
static HICON   (WINAPI *pExtractIconW)(HINSTANCE, LPCWSTR, UINT);
static BOOL    (WINAPI *pGetSaveFileNameW)(LPOPENFILENAMEW);
static DWORD   (WINAPI *pWNetRestoreConnectionW)(HWND, LPWSTR);
static DWORD   (WINAPI *pWNetGetLastErrorW)(LPDWORD, LPWSTR, DWORD, LPWSTR, DWORD);
static BOOL    (WINAPI *pPageSetupDlgW)(LPPAGESETUPDLGW);
static BOOL    (WINAPI *pPrintDlgW)(LPPRINTDLGW);
static BOOL    (WINAPI *pGetOpenFileNameW)(LPOPENFILENAMEW);
static DWORD   (WINAPI *pGetFileVersionInfoSizeW)(LPCWSTR,LPDWORD);
static BOOL    (WINAPI *pGetFileVersionInfoW)(LPCWSTR,DWORD,DWORD,LPVOID);
static WORD    (WINAPI *pVerQueryValueW)(LPVOID,LPCWSTR,LPVOID*,UINT*);
static BOOL    (WINAPI *pCOMCTL32_417)(HDC,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
static HRESULT (WINAPI *pDllGetVersion)(DLLVERSIONINFO*);

/*
 NOTES: Most functions exported by ordinal seem to be superflous.
 The reason for these functions to be there is to provide a wrapper
 for unicode functions to provide these functions on systems without
 unicode functions eg. win95/win98. Since we have such functions we just
 call these. If running Wine with native DLL's, some late bound calls may
 fail. However, its better to implement the functions in the forward DLL
 and recommend the builtin rather than reimplementing the calls here!
*/

/*************************************************************************
 * SHLWAPI_DupSharedHandle
 *
 * Internal implemetation of SHLWAPI_11.
 */
static
HSHARED WINAPI SHLWAPI_DupSharedHandle(HSHARED hShared, DWORD dwDstProcId,
                                       DWORD dwSrcProcId, DWORD dwAccess,
                                       DWORD dwOptions)
{
  HANDLE hDst, hSrc;
  DWORD dwMyProcId = GetCurrentProcessId();
  HSHARED hRet = (HSHARED)NULL;

  TRACE("(%p,%ld,%ld,%08lx,%08lx)\n", (PVOID)hShared, dwDstProcId, dwSrcProcId,
        dwAccess, dwOptions);

  /* Get dest process handle */
  if (dwDstProcId == dwMyProcId)
    hDst = GetCurrentProcess();
  else
    hDst = OpenProcess(PROCESS_DUP_HANDLE, 0, dwDstProcId);

  if (hDst)
  {
    /* Get src process handle */
    if (dwSrcProcId == dwMyProcId)
      hSrc = GetCurrentProcess();
    else
      hSrc = OpenProcess(PROCESS_DUP_HANDLE, 0, dwSrcProcId);

    if (hSrc)
    {
      /* Make handle available to dest process */
      if (!DuplicateHandle(hDst, (HANDLE)hShared, hSrc, &hRet,
                           dwAccess, 0, dwOptions | DUPLICATE_SAME_ACCESS))
        hRet = (HSHARED)NULL;

      if (dwSrcProcId != dwMyProcId)
        CloseHandle(hSrc);
    }

    if (dwDstProcId != dwMyProcId)
      CloseHandle(hDst);
  }

  TRACE("Returning handle %p\n", (PVOID)hRet);
  return hRet;
}

/*************************************************************************
 * @  [SHLWAPI.7]
 *
 * Create a block of sharable memory and initialise it with data.
 *
 * PARAMS
 * dwProcId [I] ID of process owning data
 * lpvData  [I] Pointer to data to write
 * dwSize   [I] Size of data
 *
 * RETURNS
 * Success: A shared memory handle
 * Failure: NULL
 *
 * NOTES
 * Ordinals 7-11 provide a set of calls to create shared memory between a
 * group of processes. The shared memory is treated opaquely in that its size
 * is not exposed to clients who map it. This is accomplished by storing
 * the size of the map as the first DWORD of mapped data, and then offsetting
 * the view pointer returned by this size.
 *
 * SHLWAPI_7/SHLWAPI_10 - Create/Destroy the shared memory handle
 * SHLWAPI_8/SHLWAPI_9  - Get/Release a pointer to the shared data
 * SHLWAPI_11           - Helper function; Duplicate cross-process handles
   */
HSHARED WINAPI SHLWAPI_7 (DWORD dwProcId, DWORD dwSize, LPCVOID lpvData)
{
  HANDLE hMap;
  LPVOID pMapped;
  HSHARED hRet = (HSHARED)NULL;

  TRACE("(%ld,%p,%ld)\n", dwProcId, lpvData, dwSize);

  /* Create file mapping of the correct length */
  hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, FILE_MAP_READ, 0,
                            dwSize + sizeof(dwSize), NULL);
  if (!hMap)
    return hRet;

  /* Get a view in our process address space */
  pMapped = MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

  if (pMapped)
  {
    /* Write size of data, followed by the data, to the view */
    *((DWORD*)pMapped) = dwSize;
    if (dwSize)
      memcpy((char *) pMapped + sizeof(dwSize), lpvData, dwSize);

    /* Release view. All further views mapped will be opaque */
    UnmapViewOfFile(pMapped);
    hRet = SHLWAPI_DupSharedHandle((HSHARED)hMap, dwProcId,
                                   GetCurrentProcessId(), FILE_MAP_ALL_ACCESS,
                                   DUPLICATE_SAME_ACCESS);
  }

  CloseHandle(hMap);
  return hRet;
}

/*************************************************************************
 * @ [SHLWAPI.8]
 *
 * Get a pointer to a block of shared memory from a shared memory handle.
 *
 * PARAMS
 * hShared  [I] Shared memory handle
 * dwProcId [I] ID of process owning hShared
 *
 * RETURNS
 * Success: A pointer to the shared memory
 * Failure: NULL
 *
 * NOTES
 * See SHLWAPI_7.
   */
PVOID WINAPI SHLWAPI_8 (HSHARED hShared, DWORD dwProcId)
  {
  HSHARED hDup;
  LPVOID pMapped;

  TRACE("(%p %ld)\n", (PVOID)hShared, dwProcId);

  /* Get handle to shared memory for current process */
  hDup = SHLWAPI_DupSharedHandle(hShared, dwProcId, GetCurrentProcessId(),
                                 FILE_MAP_ALL_ACCESS, 0);
  /* Get View */
  pMapped = MapViewOfFile((HANDLE)hDup, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
  CloseHandle(hDup);

  if (pMapped)
    return (char *) pMapped + sizeof(DWORD); /* Hide size */
  return NULL;
}

/*************************************************************************
 * @ [SHLWAPI.9]
 *
 * Release a pointer to a block of shared memory.
 *
 * PARAMS
 * lpView [I] Shared memory pointer
 *
 * RETURNS
 * Success: TRUE
 * Failure: FALSE
 *
 * NOTES
 * See SHLWAPI_7.
 */
BOOL WINAPI SHLWAPI_9 (LPVOID lpView)
{
  TRACE("(%p)\n", lpView);
  return UnmapViewOfFile((char *) lpView - sizeof(DWORD)); /* Include size */
}

/*************************************************************************
 * @ [SHLWAPI.10]
 *
 * Destroy a block of sharable memory.
 *
 * PARAMS
 * hShared  [I] Shared memory handle
 * dwProcId [I] ID of process owning hShared
 *
 * RETURNS
 * Success: TRUE
 * Failure: FALSE
 *
 * NOTES
 * See SHLWAPI_7.
 */
BOOL WINAPI SHLWAPI_10 (HSHARED hShared, DWORD dwProcId)
{
  HSHARED hClose;

  TRACE("(%p %ld)\n", (PVOID)hShared, dwProcId);

  /* Get a copy of the handle for our process, closing the source handle */
  hClose = SHLWAPI_DupSharedHandle(hShared, dwProcId, GetCurrentProcessId(),
                                   FILE_MAP_ALL_ACCESS,DUPLICATE_CLOSE_SOURCE);
  /* Close local copy */
  return CloseHandle((HANDLE)hClose);
}

/*************************************************************************
 * @   [SHLWAPI.11]
 *
 * Copy a sharable memory handle from one process to another.
 *
 * PARAMS
 * hShared     [I] Shared memory handle to duplicate
 * dwDstProcId [I] ID of the process wanting the duplicated handle
 * dwSrcProcId [I] ID of the process owning hShared
 * dwAccess    [I] Desired DuplicateHandle access
 * dwOptions   [I] Desired DuplicateHandle options
 *
 * RETURNS
 * Success: A handle suitable for use by the dwDstProcId process.
 * Failure: A NULL handle.
 *
 * NOTES
 * See SHLWAPI_7.
 */
HSHARED WINAPI SHLWAPI_11(HSHARED hShared, DWORD dwDstProcId, DWORD dwSrcProcId,
                          DWORD dwAccess, DWORD dwOptions)
{
  HSHARED hRet;

  hRet = SHLWAPI_DupSharedHandle(hShared, dwDstProcId, dwSrcProcId,
                                 dwAccess, dwOptions);
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.13]
 *
 * Create and register a clipboard enumerator for a web browser.
 *
 * PARAMS
 *  lpBC      [I] Binding context
 *  lpUnknown [I] An object exposing the IWebBrowserApp interface
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *  The enumerator is stored as a property of the web browser. If it does not
 *  yet exist, it is created and set before being registered.
 *
 * BUGS
 *  Unimplemented.
 */
HRESULT WINAPI SHLWAPI_13(LPBC lpBC, IUnknown *lpUnknown)
{
	FIXME("(%p,%p) stub\n", lpBC, lpUnknown);
	return 1;
#if 0
	/* pseudo code extracted from relay trace */
	RegOpenKeyA(HKLM, "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Accepted Documents", &newkey);
	ret = 0;
	i = 0;
	size = 0;
	while(!ret) {
	    ret = RegEnumValueA(newkey, i, a1, a2, 0, a3, 0, 0);
	    size += ???;
	    i++;
	}
	b1 = LocalAlloc(0x40, size);
	ret = 0;
	i = 0;
	while(!ret) {
	    ret = RegEnumValueA(newkey, i, a1, a2, 0, a3, a4, a5);
	    RegisterClipBoardFormatA(a4);
	    i++;
	}
	hmod1 = GetModuleHandleA("URLMON.DLL");
	proc = GetProcAddress(hmod1, "CreateFormatEnumerator");
	HeapAlloc(??, 0, 0x14);
	HeapAlloc(??, 0, 0x50);
	LocalAlloc(0x40, 0x78);
	/* FIXME: bad string below */
	lstrlenW(L"{D0FCA420-D3F5-11CF-B211-00AA004AE837}");
	StrCpyW(a6,  L"{D0FCA420-D3F5-11CF-B211-00AA004AE837}");

	GetTickCount();
	IsBadReadPtr(c1 = 0x403fd210,4);
	InterlockedIncrement(c1+4);
	LocalFree(b1);
	RegCloseKey(newkey);
	IsBadReadPtr(c1 = 0x403fd210,4);
	InterlockedIncrement(c1+4);

	HeapAlloc(40350000,00000000,00000014) retval=403fd0a8;
	HeapAlloc(40350000,00000000,00000050) retval=403feb44;
	hmod1 = GetModuleHandleA("URLMON.DLL");
	proc = GetProcAddress(hmod1, "RegisterFormatEnumerator");
	/* 0x1a40c88c is in URLMON.DLL just before above proc
	 * content is L"_EnumFORMATETC_"
	 * label is d1
	 */
        IsBadReadPtr(d1 = 0x1a40c88c,00000002);
	lstrlenW(d1);
	lstrlenW(d1);
	HeapAlloc(40350000,00000000,0000001e) retval=403fed44;
	IsBadReadPtr(d2 = 0x403fd0a8,00000004);
	InterlockedIncrement(d2+4);
	IsBadReadPtr(d2 = 0x403fd0a8,00000004);
	InterlockedDecrement(d2+4);
	IsBadReadPtr(c1,00000004);
	InterlockedDecrement(c1+4);
	IsBadReadPtr(c1,00000004);
	InterlockedDecrement(c1+4);

#endif
}

/*************************************************************************
 *      @	[SHLWAPI.14]
 *
 * Get Explorers "AcceptLanguage" setting.
 *
 * PARAMS
 *  langbuf [O] Destination for language string
 *  buflen  [I] Length of langbuf
 *
 * RETURNS
 *  Success: S_OK.   langbuf is set to the language string found.
 *  Failure: E_FAIL, If any arguments are invalid, error occurred, or Explorer
 *           does not contain the setting.
 */
HRESULT WINAPI SHLWAPI_14 (
	LPSTR langbuf,
	LPDWORD buflen)
{
	CHAR *mystr;
	DWORD mystrlen, mytype;
	HKEY mykey;
	LCID mylcid;

	mystrlen = (*buflen > 6) ? *buflen : 6;
	mystr = (CHAR*)HeapAlloc(GetProcessHeap(),
				 HEAP_ZERO_MEMORY, mystrlen);
	RegOpenKeyA(HKEY_CURRENT_USER,
		    "Software\\Microsoft\\Internet Explorer\\International",
		    &mykey);
	if (RegQueryValueExA(mykey, "AcceptLanguage",
			      0, &mytype, mystr, &mystrlen)) {
	    /* Did not find value */
	    mylcid = GetUserDefaultLCID();
	    /* somehow the mylcid translates into "en-us"
	     *  this is similar to "LOCALE_SABBREVLANGNAME"
	     *  which could be gotten via GetLocaleInfo.
	     *  The only problem is LOCALE_SABBREVLANGUAGE" is
	     *  a 3 char string (first 2 are country code and third is
	     *  letter for "sublanguage", which does not come close to
	     *  "en-us"
	     */
	    lstrcpyA(mystr, "en-us");
	    mystrlen = lstrlenA(mystr);
	}
	else {
	    /* handle returned string */
	    FIXME("missing code\n");
	}
	if (mystrlen > *buflen)
	    lstrcpynA(langbuf, mystr, *buflen);
	else {
	    lstrcpyA(langbuf, mystr);
	    *buflen = lstrlenA(langbuf);
	}
	RegCloseKey(mykey);
	HeapFree(GetProcessHeap(), 0, mystr);
	TRACE("language is %s\n", debugstr_a(langbuf));
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.15]
 *
 * Unicode version of SHLWAPI_14.
 */
HRESULT WINAPI SHLWAPI_15 (
	LPWSTR langbuf,
	LPDWORD buflen)
{
	CHAR *mystr;
	DWORD mystrlen, mytype;
	HKEY mykey;
	LCID mylcid;

	mystrlen = (*buflen > 6) ? *buflen : 6;
	mystr = (CHAR*)HeapAlloc(GetProcessHeap(),
				 HEAP_ZERO_MEMORY, mystrlen);
	RegOpenKeyA(HKEY_CURRENT_USER,
		    "Software\\Microsoft\\Internet Explorer\\International",
		    &mykey);
	if (RegQueryValueExA(mykey, "AcceptLanguage",
			      0, &mytype, mystr, &mystrlen)) {
	    /* Did not find value */
	    mylcid = GetUserDefaultLCID();
	    /* somehow the mylcid translates into "en-us"
	     *  this is similar to "LOCALE_SABBREVLANGNAME"
	     *  which could be gotten via GetLocaleInfo.
	     *  The only problem is LOCALE_SABBREVLANGUAGE" is
	     *  a 3 char string (first 2 are country code and third is
	     *  letter for "sublanguage", which does not come close to
	     *  "en-us"
	     */
	    lstrcpyA(mystr, "en-us");
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
 *      @	[SHLWAPI.25]
 *
 * Seems to be iswalpha
 */
BOOL WINAPI SHLWAPI_25(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_ALPHA) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.26]
 *
 * Seems to be iswupper
 */
BOOL WINAPI SHLWAPI_26(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_UPPER) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.27]
 *
 * Seems to be iswlower
 */
BOOL WINAPI SHLWAPI_27(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_LOWER) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.28]
 *
 * Seems to be iswalnum
 */
BOOL WINAPI SHLWAPI_28(WCHAR wc)
{
    return (get_char_typeW(wc) & (C1_ALPHA|C1_DIGIT)) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.29]
 *
 * Seems to be iswspace
 */
BOOL WINAPI SHLWAPI_29(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_SPACE) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.30]
 *
 * Seems to be iswblank
 */
BOOL WINAPI SHLWAPI_30(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_BLANK) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.31]
 *
 * Seems to be iswpunct
 */
BOOL WINAPI SHLWAPI_31(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_PUNCT) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.32]
 *
 * Seems to be iswcntrl
 */
BOOL WINAPI SHLWAPI_32(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_CNTRL) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.33]
 *
 * Seems to be iswdigit
 */
BOOL WINAPI SHLWAPI_33(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_DIGIT) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.34]
 *
 * Seems to be iswxdigit
 */
BOOL WINAPI SHLWAPI_34(WCHAR wc)
{
    return (get_char_typeW(wc) & C1_XDIGIT) != 0;
}

/*************************************************************************
 *      @	[SHLWAPI.35]
 *
 */
BOOL WINAPI SHLWAPI_35(LPVOID p1, DWORD dw2, LPVOID p3)
{
    FIXME("(%p, 0x%08lx, %p): stub\n", p1, dw2, p3);
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.36]
 *
 * Insert a bitmap menu item at the bottom of a menu.
 *
 * PARAMS
 *  hMenu [I] Menu to insert into
 *  flags [I] Flags for insertion
 *  id    [I] Menu ID of the item
 *  str   [I] Menu text for the item
 *
 * RETURNS
 *  Success: TRUE,  the item is inserted into the menu
 *  Failure: FALSE, if any parameter is invalid
 */
BOOL WINAPI SHLWAPI_36(HMENU hMenu, UINT flags, UINT id, LPCWSTR str)
{
    TRACE("(%p,0x%08x,0x%08x,%s)\n",hMenu, flags, id, debugstr_w(str));
    return InsertMenuW(hMenu, -1, flags | MF_BITMAP, id, str);
}

/*************************************************************************
 *      @	[SHLWAPI.74]
 *
 * Get the text from a given dialog item.
 *
 * PARAMS
 *  hWnd     [I] Handle of dialog
 *  nItem    [I] Index of item
 *  lpsDest  [O] Buffer for receiving window text
 *  nDestLen [I] Length of buffer.
 *
 * RETURNS
 *  Success: The length of the returned text.
 *  Failure: 0.
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
 *      @   [SHLWAPI.138]
 *
 * Set the text of a given dialog item.
 *
 * PARAMS
 *  hWnd     [I] Handle of dialog
 *  iItem    [I] Index of item
 *  lpszText [O] Text to set
 *
 * RETURNS
 *  Success: TRUE.  The text of the dialog is set to lpszText.
 *  Failure: FALSE, Otherwise.
 */
BOOL WINAPI SHLWAPI_138(HWND hWnd, INT iItem, LPCWSTR lpszText)
{
    HWND hWndItem = GetDlgItem(hWnd, iItem);
    if (hWndItem)
        return SetWindowTextW(hWndItem, lpszText);
    return FALSE;
}

/*************************************************************************
 *      @	[SHLWAPI.151]
 * Function:  Compare two ASCII strings for "len" bytes.
 * Returns:   *str1-*str2  (case sensitive)
 */
DWORD WINAPI SHLWAPI_151(LPCSTR str1, LPCSTR str2, INT len)
{
    return strncmp( str1, str2, len );
}

/*************************************************************************
 *      @	[SHLWAPI.152]
 *
 * Function:  Compare two WIDE strings for "len" bytes.
 * Returns:   *str1-*str2  (case sensitive)
 */
DWORD WINAPI SHLWAPI_152(LPCWSTR str1, LPCWSTR str2, INT len)
{
    return strncmpW( str1, str2, len );
}

/*************************************************************************
 *      @	[SHLWAPI.153]
 * Function:  Compare two ASCII strings for "len" bytes via caseless compare.
 * Returns:   *str1-*str2  (case insensitive)
 */
DWORD WINAPI SHLWAPI_153(LPCSTR str1, LPCSTR str2, DWORD len)
{
    return strncasecmp( str1, str2, len );
}

/*************************************************************************
 *      @	[SHLWAPI.154]
 *
 * Function:  Compare two WIDE strings for "len" bytes via caseless compare.
 * Returns:   *str1-*str2  (case insensitive)
 */
DWORD WINAPI SHLWAPI_154(LPCWSTR str1, LPCWSTR str2, DWORD len)
{
    return strncmpiW( str1, str2, len );
}

/*************************************************************************
 *      @	[SHLWAPI.155]
 *
 *	Case sensitive string compare (ASCII). Does not SetLastError().
 */
DWORD WINAPI SHLWAPI_155(LPCSTR str1, LPCSTR str2)
{
    return strcmp(str1, str2);
}

/*************************************************************************
 *      @	[SHLWAPI.156]
 *
 *	Case sensitive string compare. Does not SetLastError().
 */
DWORD WINAPI SHLWAPI_156(LPCWSTR str1, LPCWSTR str2)
{
    return strcmpW( str1, str2 );
}

/*************************************************************************
 *      @	[SHLWAPI.157]
 *
 *	Case insensitive string compare (ASCII). Does not SetLastError().
 */
DWORD WINAPI SHLWAPI_157(LPCSTR str1, LPCSTR str2)
{
    return strcasecmp(str1, str2);
}
/*************************************************************************
 *      @	[SHLWAPI.158]
 *
 *	Case insensitive string compare. Does not SetLastError(). ??
 */
DWORD WINAPI SHLWAPI_158 (LPCWSTR str1, LPCWSTR str2)
{
    return strcmpiW( str1, str2 );
}

/*************************************************************************
 *      @	[SHLWAPI.162]
 *
 * Remove a hanging lead byte from the end of a string, if present.
 *
 * PARAMS
 *  lpStr [I] String to check for a hanging lead byte
 *  size  [I] Length of lpszStr
 *
 * RETURNS
 *  Success: The new size of the string. Any hanging lead bytes are removed.
 *  Failure: 0, if any parameters are invalid.
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
 *      @	[SHLWAPI.163]
 *
 * Call IOleCommandTarget::QueryStatus() on an object.
 *
 * PARAMS
 *  lpUnknown     [I] Object supporting the IOleCommandTarget interface
 *  pguidCmdGroup [I] GUID for the command group
 *  cCmds         [I]
 *  prgCmds       [O] Commands
 *  pCmdText      [O] Command text
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_FAIL, if lpUnknown is NULL.
 *           E_NOINTERFACE, if lpUnknown does not support IOleCommandTarget.
 *           Otherwise, an error code from IOleCommandTarget::QueryStatus().
 */
HRESULT WINAPI SHLWAPI_163(IUnknown* lpUnknown, REFGUID pguidCmdGroup,
                           ULONG cCmds, OLECMD *prgCmds, OLECMDTEXT* pCmdText)
{
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p,%ld,%p,%p)\n",lpUnknown, pguidCmdGroup, cCmds, prgCmds, pCmdText);

  if (lpUnknown)
  {
    IOleCommandTarget* lpOle;

    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleCommandTarget,
                                   (void**)&lpOle);

    if (SUCCEEDED(hRet) && lpOle)
    {
      hRet = IOleCommandTarget_QueryStatus(lpOle, pguidCmdGroup, cCmds,
                                           prgCmds, pCmdText);
      IOleCommandTarget_Release(lpOle);
    }
  }
  return hRet;
}

/*************************************************************************
 *      @		[SHLWAPI.164]
 *
 * Call IOleCommandTarget::Exec() on an object.
 *
 * PARAMS
 *  lpUnknown     [I] Object supporting the IOleCommandTarget interface
 *  pguidCmdGroup [I] GUID for the command group
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_FAIL, if lpUnknown is NULL.
 *           E_NOINTERFACE, if lpUnknown does not support IOleCommandTarget.
 *           Otherwise, an error code from IOleCommandTarget::Exec().
 */
HRESULT WINAPI SHLWAPI_164(IUnknown* lpUnknown, REFGUID pguidCmdGroup,
                           DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn,
                           VARIANT* pvaOut)
{
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p,%ld,%ld,%p,%p)\n",lpUnknown, pguidCmdGroup, nCmdID,
        nCmdexecopt, pvaIn, pvaOut);

  if (lpUnknown)
  {
    IOleCommandTarget* lpOle;

    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleCommandTarget,
                                   (void**)&lpOle);
    if (SUCCEEDED(hRet) && lpOle)
    {
      hRet = IOleCommandTarget_Exec(lpOle, pguidCmdGroup, nCmdID,
                                    nCmdexecopt, pvaIn, pvaOut);
      IOleCommandTarget_Release(lpOle);
    }
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.165]
 *
 * Retrieve, modify, and re-set a value from a window.
 *
 * PARAMS
 *  hWnd   [I] Windows to get value from
 *  offset [I] Offset of value
 *  wMask  [I] Mask for uiFlags
 *  wFlags [I] Bits to set in window value
 *
 * RETURNS
 *  The new value as it was set, or 0 if any parameter is invalid.
 *
 * NOTES
 *  Any bits set in uiMask are cleared from the value, then any bits set in
 *  uiFlags are set in the value.
 */
LONG WINAPI SHLWAPI_165(HWND hwnd, INT offset, UINT wMask, UINT wFlags)
{
  LONG ret = GetWindowLongA(hwnd, offset);
  UINT newFlags = (wFlags & wMask) | (ret & ~wFlags);

  if (newFlags != ret)
    ret = SetWindowLongA(hwnd, offset, newFlags);
  return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.167]
 *
 * Change a window's parent.
 *
 * PARAMS
 *  hWnd       [I] Window to change parent of
 *  hWndParent [I] New parent window
 *
 * RETURNS
 *  The old parent of hWnd.
 *
 * NOTES
 *  If hWndParent is NULL (desktop), the window style is changed to WS_POPUP.
 *  If hWndParent is NOT NULL then we set the WS_CHILD style.
 */
HWND WINAPI SHLWAPI_167(HWND hWnd, HWND hWndParent)
{
  TRACE("%p, %p\n", hWnd, hWndParent);

  if(GetParent(hWnd) == hWndParent)
    return 0;

  if(hWndParent)
    SHLWAPI_165(hWnd, GWL_STYLE, WS_CHILD, WS_CHILD);
  else
    SHLWAPI_165(hWnd, GWL_STYLE, WS_POPUP, WS_POPUP);

  return SetParent(hWnd, hWndParent);
}

/*************************************************************************
 *      @       [SHLWAPI.168]
 *
 * Locate and advise a connection point in an IConnectionPointContainer.
 *
 * PARAMS
 *  lpUnkSink   [I] Sink for the connection point advise call
 *  riid        [I] REFIID of connection point to advise
 *  bAdviseOnly [I] TRUE = Advise only, FALSE = Unadvise first
 *  lpUnknown   [I] Object supporting the IConnectionPointContainer interface
 *  lpCookie    [O] Pointer to connection point cookie
 *  lppCP       [O] Destination for the IConnectionPoint found
 *
 * RETURNS
 *  Success: S_OK. If lppCP is non-NULL, it is filled with the IConnectionPoint
 *           that was advised. The caller is responsable for releasing it.
 *  Failure: E_FAIL, if any arguments are invalid.
 *           E_NOINTERFACE, if lpUnknown isn't an IConnectionPointContainer,
 *           Or an HRESULT error code if any call fails.
 */
HRESULT WINAPI SHLWAPI_168(IUnknown* lpUnkSink, REFIID riid, BOOL bAdviseOnly,
                           IUnknown* lpUnknown, LPDWORD lpCookie,
                           IConnectionPoint **lppCP)
{
  HRESULT hRet;
  IConnectionPointContainer* lpContainer;
  IConnectionPoint *lpCP;

  if(!lpUnknown || (bAdviseOnly && !lpUnkSink))
    return E_FAIL;

  if(lppCP)
    *lppCP = NULL;

  hRet = IUnknown_QueryInterface(lpUnknown, &IID_IConnectionPointContainer,
                                 (void**)&lpContainer);
  if (SUCCEEDED(hRet))
  {
    hRet = IConnectionPointContainer_FindConnectionPoint(lpContainer, riid, &lpCP);

    if (SUCCEEDED(hRet))
    {
      if(!bAdviseOnly)
        hRet = IConnectionPoint_Unadvise(lpCP, *lpCookie);
      hRet = IConnectionPoint_Advise(lpCP, lpUnkSink, lpCookie);

      if (FAILED(hRet))
        *lpCookie = 0;

      if (lppCP && SUCCEEDED(hRet))
        *lppCP = lpCP; /* Caller keeps the interface */
      else
        IConnectionPoint_Release(lpCP); /* Release it */
    }

    IUnknown_Release(lpContainer);
  }
  return hRet;
}

/*************************************************************************
 *	@	[SHLWAPI.169]
 *
 * Release an interface.
 *
 * PARAMS
 *  lpUnknown [I] Object to release
 *
 * RETURNS
 *  Nothing.
 */
DWORD WINAPI SHLWAPI_169 (IUnknown ** lpUnknown)
{
        IUnknown *temp;

	TRACE("(%p)\n",lpUnknown);
	if(!lpUnknown || !*((LPDWORD)lpUnknown)) return 0;
	temp = *lpUnknown;
	*lpUnknown = NULL;
	TRACE("doing Release\n");
	return IUnknown_Release(temp);
}

/*************************************************************************
 *      @	[SHLWAPI.170]
 *
 * Skip '//' if present in a string.
 *
 * PARAMS
 *  lpszSrc [I] String to check for '//'
 *
 * RETURNS
 *  Success: The next character after the '//' or the string if not present
 *  Failure: NULL, if lpszStr is NULL.
 */
LPCSTR WINAPI SHLWAPI_170(LPCSTR lpszSrc)
{
  if (lpszSrc && lpszSrc[0] == '/' && lpszSrc[1] == '/')
    lpszSrc += 2;
  return lpszSrc;
}

/*************************************************************************
 *      @		[SHLWAPI.171]
 *
 * Check the two interfaces if they come from the same object.
 *
 * PARAMS
 *   lpInt1 [I]: Interface to check against lpInt2.
 *   lpInt2 [I]: Interface to check against lpInt1.
 *
 * RETURNS
 *   TRUE: Interfaces come from the same object.
 *   FALSE: Interfaces come from different objects.
 */
BOOL WINAPI SHLWAPI_171(IUnknown* lpInt1, IUnknown* lpInt2)
{
  LPVOID lpUnknown1, lpUnknown2;

  TRACE("%p %p\n", lpInt1, lpInt2);

  if (!lpInt1 || !lpInt2)
    return FALSE;

  if (lpInt1 == lpInt2)
    return TRUE;

  if (!SUCCEEDED(IUnknown_QueryInterface(lpInt1, &IID_IUnknown,
                                       (LPVOID *)&lpUnknown1)))
    return FALSE;

  if (!SUCCEEDED(IUnknown_QueryInterface(lpInt2, &IID_IUnknown,
                                       (LPVOID *)&lpUnknown2)))
    return FALSE;

  if (lpUnknown1 == lpUnknown2)
    return TRUE;
  
  return FALSE;
}

/*************************************************************************
 *      @	[SHLWAPI.172]
 *
 * Get the window handle of an object.
 *
 * PARAMS
 *  lpUnknown [I] Object to get the window handle of
 *  lphWnd    [O] Destination for window handle
 *
 * RETURNS
 *  Success: S_OK. lphWnd contains the objects window handle.
 *  Failure: An HRESULT error code.
 *
 * NOTES
 *  lpUnknown is expected to support one of the following interfaces:
 *   IOleWindow
 *   IInternetSecurityMgrSite
 *   IShellView
 */
HRESULT WINAPI SHLWAPI_172(IUnknown *lpUnknown, HWND *lphWnd)
{
  /* FIXME: Wine has no header for this object */
  static const GUID IID_IInternetSecurityMgrSite = { 0x79eac9ed,
    0xbaf9, 0x11ce, { 0x8c, 0x82, 0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b }};
  IUnknown *lpOle;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p)\n", lpUnknown, lphWnd);

  if (!lpUnknown)
    return hRet;

  hRet = IUnknown_QueryInterface(lpUnknown, &IID_IOleWindow, (void**)&lpOle);

  if (FAILED(hRet))
  {
    hRet = IUnknown_QueryInterface(lpUnknown,&IID_IShellView, (void**)&lpOle);

    if (FAILED(hRet))
    {
      hRet = IUnknown_QueryInterface(lpUnknown, &IID_IInternetSecurityMgrSite,
                                      (void**)&lpOle);
    }
  }

  if (SUCCEEDED(hRet))
  {
    /* Lazyness here - Since GetWindow() is the first method for the above 3
     * interfaces, we use the same call for them all.
     */
    hRet = IOleWindow_GetWindow((IOleWindow*)lpOle, lphWnd);
    IUnknown_Release(lpOle);
    if (lphWnd)
      TRACE("Returning HWND=%p\n", *lphWnd);
  }

  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.174]
 *
 * Seems to do call either IObjectWithSite::SetSite or
 *   IPersistMoniker::GetClassID.  But since we do not implement either
 *   of those classes in our headers, we will fake it out.
 */
DWORD WINAPI SHLWAPI_174(
	IUnknown *p1,     /* [in]   OLE object                          */
        LPVOID *p2)       /* [out]  ptr to result of either GetClassID
                                    or SetSite call.                    */
{
    DWORD ret, aa;

    if (!p1) return E_FAIL;

    /* see if SetSite interface exists for IObjectWithSite object */
    ret = IUnknown_QueryInterface((IUnknown *)p1, (REFIID)id1, (LPVOID *)&p1);
    TRACE("first IU_QI ret=%08lx, p1=%p\n", ret, p1);
    if (ret) {

	/* see if GetClassId interface exists for IPersistMoniker object */
	ret = IUnknown_QueryInterface((IUnknown *)p1, (REFIID)id2, (LPVOID *)&aa);
	TRACE("second IU_QI ret=%08lx, aa=%08lx\n", ret, aa);
	if (ret) return ret;

	/* fake a GetClassId call */
	ret = IOleWindow_GetWindow((IOleWindow *)aa, (HWND*)p2);
	TRACE("second IU_QI doing 0x0c ret=%08lx, *p2=%08lx\n", ret,
	      *(LPDWORD)p2);
	IUnknown_Release((IUnknown *)aa);
    }
    else {
	/* fake a SetSite call */
	ret = IOleWindow_GetWindow((IOleWindow *)p1, (HWND*)p2);
	TRACE("first IU_QI doing 0x0c ret=%08lx, *p2=%08lx\n", ret,
	      *(LPDWORD)p2);
	IUnknown_Release((IUnknown *)p1);
    }
    return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.175]
 *
 * Call IPersist::GetClassID on an object.
 *
 * PARAMS
 *  lpUnknown [I] Object supporting the IPersist interface
 *  lpClassId [O] Destination for Class Id
 *
 * RETURNS
 *  Success: S_OK. lpClassId contains the Class Id requested.
 *  Failure: E_FAIL, If lpUnknown is NULL,
 *           E_NOINTERFACE If lpUnknown does not support IPersist,
 *           Or an HRESULT error code.
 */
HRESULT WINAPI SHLWAPI_175 (IUnknown *lpUnknown, CLSID* lpClassId)
{
  IPersist* lpPersist;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p)\n", lpUnknown, debugstr_guid(lpClassId));

  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown,&IID_IPersist,(void**)&lpPersist);
    if (SUCCEEDED(hRet))
    {
      IPersist_GetClassID(lpPersist, lpClassId);
      IPersist_Release(lpPersist);
    }
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.176]
 *
 * Retrieve a Service Interface from an object.
 *
 * PARAMS
 *  lpUnknown [I] Object to get an IServiceProvider interface from
 *  sid       [I] Service ID for QueryService call
 *  riid      [I] Function requested for QueryService call
 *  lppOut    [O] Destination for the service interface pointer
 *
 * Function appears to be interface to IServiceProvider::QueryService
 *
 * RETURNS
 *  Success: S_OK. lppOut contains an object providing the requested service
 *  Failure: An HRESULT error code
 *
 * NOTES
 *  lpUnknown is expected to support the IServiceProvider interface.
 */
HRESULT WINAPI SHLWAPI_176(IUnknown* lpUnknown, REFGUID sid, REFIID riid,
                           LPVOID *lppOut)
{
  IServiceProvider* pService = NULL;
  HRESULT hRet;

  if (!lppOut)
    return E_FAIL;

  *lppOut = NULL;

  if (!lpUnknown)
    return E_FAIL;

  /* Get an IServiceProvider interface from the object */
  hRet = IUnknown_QueryInterface(lpUnknown, &IID_IServiceProvider,
                                 (LPVOID*)&pService);

  if (!hRet && pService)
  {
    TRACE("QueryInterface returned (IServiceProvider*)%p\n", pService);

    /* Get a Service interface from the object */
    hRet = IServiceProvider_QueryService(pService, sid, riid, lppOut);

    TRACE("(IServiceProvider*)%p returned (IUnknown*)%p\n", pService, *lppOut);

    /* Release the IServiceProvider interface */
    IUnknown_Release(pService);
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.180]
 *
 * Remove all sub-menus from a menu.
 *
 * PARAMS
 *  hMenu [I] Menu to remove sub-menus from
 *
 * RETURNS
 *  Success: 0.  All sub-menus under hMenu are removed
 *  Failure: -1, if any parameter is invalid
 */
DWORD WINAPI SHLWAPI_180(HMENU hMenu)
{
  int iItemCount = GetMenuItemCount(hMenu) - 1;
  while (iItemCount >= 0)
  {
    HMENU hSubMenu = GetSubMenu(hMenu, iItemCount);
    if (hSubMenu)
      RemoveMenu(hMenu, iItemCount, 0x400);
    iItemCount--;
  }
  return iItemCount;
}

/*************************************************************************
 *      @	[SHLWAPI.181]
 *
 * Enable or disable a menu item.
 *
 * PARAMS
 *  hMenu   [I] Menu holding menu item
 *  uID     [I] ID of menu item to enable/disable
 *  bEnable [I] Whether to enable (TRUE) or disable (FALSE) the item.
 *
 * RETURNS
 *  The return code from EnableMenuItem.
 */
UINT WINAPI SHLWAPI_181(HMENU hMenu, UINT wItemID, BOOL bEnable)
{
  return EnableMenuItem(hMenu, wItemID, bEnable ? MF_ENABLED : MF_GRAYED);
}

/*************************************************************************
 * @	[SHLWAPI.182]
 *
 * Check or uncheck a menu item.
 *
 * PARAMS
 *  hMenu  [I] Menu holding menu item
 *  uID    [I] ID of menu item to check/uncheck
 *  bCheck [I] Whether to check (TRUE) or uncheck (FALSE) the item.
 *
 * RETURNS
 *  The return code from CheckMenuItem.
 */
DWORD WINAPI SHLWAPI_182(HMENU hMenu, UINT uID, BOOL bCheck)
{
  return CheckMenuItem(hMenu, uID, bCheck ? MF_CHECKED : 0);
}

/*************************************************************************
 *      @	[SHLWAPI.183]
 *
 * Register a window class if it isn't already.
 *
 * PARAMS
 *  lpWndClass [I] Window class to register
 *
 * RETURNS
 *  The result of the RegisterClassA call.
 */
DWORD WINAPI SHLWAPI_183(WNDCLASSA *wndclass)
{
  WNDCLASSA wca;
  if (GetClassInfoA(wndclass->hInstance, wndclass->lpszClassName, &wca))
    return TRUE;
  return (DWORD)RegisterClassA(wndclass);
}

/*************************************************************************
 *      @	[SHLWAPI.187]
 *
 * Call IPersistPropertyBag::Load on an object.
 *
 * PARAMS
 *  lpUnknown [I] Object supporting the IPersistPropertyBag interface
 *  lpPropBag [O] Destination for loaded IPropertyBag
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: An HRESULT error code, or E_FAIL if lpUnknown is NULL.
 */
DWORD WINAPI SHLWAPI_187(IUnknown *lpUnknown, IPropertyBag* lpPropBag)
{
  IPersistPropertyBag* lpPPBag;
  HRESULT hRet = E_FAIL;

  TRACE("(%p,%p)\n", lpUnknown, lpPropBag);

  if (lpUnknown)
  {
    hRet = IUnknown_QueryInterface(lpUnknown, &IID_IPersistPropertyBag,
                                   (void**)&lpPPBag);
    if (SUCCEEDED(hRet) && lpPPBag)
    {
      hRet = IPersistPropertyBag_Load(lpPPBag, lpPropBag, NULL);
      IPersistPropertyBag_Release(lpPPBag);
    }
  }
  return hRet;
}

/*************************************************************************
 *      @	[SHLWAPI.189]
 *
 * _IUnknown_OnFocusOCS
 */
DWORD WINAPI SHLWAPI_189(LPVOID x, LPVOID y)
{
	FIXME("%p %p\n", x, y);
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.193]
 *
 * Get the color depth of the primary display.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The color depth of the primary display.
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
 *      @       [SHLWAPI.197]
 *
 * Blank out a region of text by drawing the background only.
 *
 * PARAMS
 *  hDC   [I] Device context to draw in
 *  pRect [I] Area to draw in
 *  cRef  [I] Color to draw in
 *
 * RETURNS
 *  Nothing.
 */
DWORD WINAPI SHLWAPI_197(HDC hDC, LPCRECT pRect, COLORREF cRef)
{
    COLORREF cOldColor = SetBkColor(hDC, cRef);
    ExtTextOutA(hDC, 0, 0, ETO_OPAQUE, pRect, 0, 0, 0);
    SetBkColor(hDC, cOldColor);
    return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.199]
 *
 * Copy interface pointer
 *
 * PARAMS
 *   lppDest   [O] Destination for copy
 *   lpUnknown [I] Source for copy
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI SHLWAPI_199(IUnknown **lppDest, IUnknown *lpUnknown)
{
  TRACE("(%p,%p)\n", lppDest, lpUnknown);

  if (lppDest)
    SHLWAPI_169(lppDest); /* Release existing interface */

  if (lpUnknown)
  {
    /* Copy */
    IUnknown_AddRef(lpUnknown);
    *lppDest = lpUnknown;
  }
}

/*************************************************************************
 *      @	[SHLWAPI.201]
 *
 */
HRESULT WINAPI SHLWAPI_201(IUnknown* lpUnknown, INT iUnk, REFGUID pguidCmdGroup,
                           DWORD nCmdID, DWORD nCmdexecopt, VARIANT* pvaIn,
                           VARIANT* pvaOut)
{
  FIXME("(%p,%d,%p,%ld,%ld,%p,%p) - stub!\n", lpUnknown, iUnk, pguidCmdGroup,
        nCmdID, nCmdexecopt, pvaIn, pvaOut);
  return DRAGDROP_E_NOTREGISTERED;
}

/*************************************************************************
 *      @	[SHLWAPI.202]
 *
 */
HRESULT WINAPI SHLWAPI_202(REFGUID pguidCmdGroup,ULONG cCmds, OLECMD *prgCmds)
{
  FIXME("(%p,%ld,%p) - stub!\n", pguidCmdGroup, cCmds, prgCmds);
  return DRAGDROP_E_NOTREGISTERED;
}

/*************************************************************************
 *      @	[SHLWAPI.203]
 *
 */
VOID WINAPI SHLWAPI_203(LPCSTR lpszStr)
{
  FIXME("(%s) - stub!\n", debugstr_a(lpszStr));
}

/*************************************************************************
 * @	[SHLWAPI.204]
 *
 * Determine if a window is not a child of another window.
 *
 * PARAMS
 * hParent [I] Suspected parent window
 * hChild  [I] Suspected child window
 *
 * RETURNS
 * TRUE:  If hChild is a child window of hParent
 * FALSE: If hChild is not a child window of hParent, or they are equal
 */
BOOL WINAPI SHLWAPI_204(HWND hParent, HWND hChild)
{
  TRACE("(%p,%p)\n", hParent, hChild);

  if (!hParent || !hChild)
    return TRUE;
  else if(hParent == hChild)
    return FALSE;
  return !IsChild(hParent, hChild);
}

/*************************************************************************
 *      @	[SHLWAPI.208]
 *
 * Some sort of memory management process - associated with _210
 */
DWORD WINAPI SHLWAPI_208 (
	DWORD    a,
	DWORD    b,
	LPVOID   c,
	LPVOID   d,
	DWORD    e)
{
    FIXME("(0x%08lx 0x%08lx %p %p 0x%08lx) stub\n",
	  a, b, c, d, e);
    return 1;
}

/*************************************************************************
 *      @	[SHLWAPI.209]
 *
 * Some sort of memory management process - associated with _208
 */
DWORD WINAPI SHLWAPI_209 (
	LPVOID   a)
{
    FIXME("(%p) stub\n",
	  a);
    return 1;
}

/*************************************************************************
 *      @	[SHLWAPI.210]
 *
 * Some sort of memory management process - associated with _208
 */
DWORD WINAPI SHLWAPI_210 (
	LPVOID   a,
	DWORD    b,
	LPVOID   c)
{
    FIXME("(%p 0x%08lx %p) stub\n",
	  a, b, c);
    return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.211]
 */
DWORD WINAPI SHLWAPI_211 (
	LPVOID   a,
	DWORD    b)
{
    FIXME("(%p 0x%08lx) stub\n",
	  a, b);
    return 1;
}

/*************************************************************************
 *      @	[SHLWAPI.215]
 *
 * NOTES
 *  check me!
 */
DWORD WINAPI SHLWAPI_215 (
	LPCSTR lpStrSrc,
	LPWSTR lpwStrDest,
	int len)
{
	INT len_a, ret;

	len_a = lstrlenA(lpStrSrc);
	ret = MultiByteToWideChar(0, 0, lpStrSrc, len_a, lpwStrDest, len);
	TRACE("%s %s %d, ret=%d\n",
	      debugstr_a(lpStrSrc), debugstr_w(lpwStrDest), len, ret);
	return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.218]
 *
 * WideCharToMultiByte with multi language support.
 */
INT WINAPI SHLWAPI_218(UINT CodePage, LPCWSTR lpSrcStr, LPSTR lpDstStr,
                       LPINT lpnMultiCharCount)
{
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

      GET_FUNC(pConvertINetUnicodeToMultiByte, mlang, "ConvertINetUnicodeToMultiByte", 0);
      if (!pConvertINetUnicodeToMultiByte(&dwMode, CodePage, lpSrcStr, &nWideCharCount, lpDstStr,
                                          lpnMultiCharCount))
        return 0;

      if (nWideCharCount < len - 1)
      {
        mem = (LPSTR)HeapAlloc(GetProcessHeap(), 0, *lpnMultiCharCount);
        if (!mem)
          return 0;

        *lpnMultiCharCount = 0;

        if (pConvertINetUnicodeToMultiByte(&dwMode, CodePage, lpSrcStr, &len, mem, lpnMultiCharCount))
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
 *
 * It seems from OE v5 that the third param is the count. (GA 11/2001)
 */
INT WINAPI SHLWAPI_217(LPCWSTR lpSrcStr, LPSTR lpDstStr, INT MultiCharCount)
{
    INT myint = MultiCharCount;

    return SHLWAPI_218(CP_ACP, lpSrcStr, lpDstStr, &myint);
}

/*************************************************************************
 *      @	[SHLWAPI.219]
 *
 * Seems to be "super" QueryInterface. Supplied with a table of interfaces
 * and an array of IIDs and offsets into the table.
 *
 * NOTES
 *  error codes: E_POINTER, E_NOINTERFACE
 */
typedef struct {
    REFIID   refid;
    DWORD    indx;
} IFACE_INDEX_TBL;

HRESULT WINAPI SHLWAPI_219 (
	LPVOID w,           /* [in]   table of interfaces                   */
	IFACE_INDEX_TBL *x, /* [in]   array of REFIIDs and indexes to above */
	REFIID riid,        /* [in]   REFIID to get interface for           */
	LPVOID *ppv)          /* [out]  location to get interface pointer     */
{
	HRESULT ret;
	IUnknown *a_vtbl;
	IFACE_INDEX_TBL *xmove;

	TRACE("(%p %p %s %p)\n", w,x,debugstr_guid(riid),ppv);
	if (ppv) {
	    xmove = x;
	    while (xmove->refid) {
		TRACE("trying (indx %ld) %s\n", xmove->indx, debugstr_guid(xmove->refid));
		if (IsEqualIID(riid, xmove->refid)) {
		    a_vtbl = (IUnknown*)(xmove->indx + (LPBYTE)w);
		    TRACE("matched, returning (%p)\n", a_vtbl);
		    *ppv = (LPVOID)a_vtbl;
		    IUnknown_AddRef(a_vtbl);
		    return S_OK;
		}
		xmove++;
	    }

	    if (IsEqualIID(riid, &IID_IUnknown)) {
		a_vtbl = (IUnknown*)(x->indx + (LPBYTE)w);
		TRACE("returning first for IUnknown (%p)\n", a_vtbl);
		*ppv = (LPVOID)a_vtbl;
		IUnknown_AddRef(a_vtbl);
		return S_OK;
	    }
	    *ppv = 0;
	    ret = E_NOINTERFACE;
	} else
	    ret = E_POINTER;

	TRACE("-- 0x%08lx\n", ret);
	return ret;
}

/*************************************************************************
 *      @	[SHLWAPI.236]
 */
HMODULE WINAPI SHLWAPI_236 (REFIID lpUnknown)
{
    HKEY newkey;
    DWORD type, count;
    CHAR value[MAX_PATH], string[MAX_PATH];

    strcpy(string, "CLSID\\");
    strcat(string, debugstr_guid(lpUnknown));
    strcat(string, "\\InProcServer32");

    count = MAX_PATH;
    RegOpenKeyExA(HKEY_CLASSES_ROOT, string, 0, 1, &newkey);
    RegQueryValueExA(newkey, 0, 0, &type, value, &count);
    RegCloseKey(newkey);
    return LoadLibraryExA(value, 0, 0);
}

/*************************************************************************
 *      @	[SHLWAPI.237]
 *
 * Unicode version of SHLWAPI_183.
 */
DWORD WINAPI SHLWAPI_237 (WNDCLASSW * lpWndClass)
{
	WNDCLASSW WndClass;

	TRACE("(%p %s)\n",lpWndClass->hInstance, debugstr_w(lpWndClass->lpszClassName));

	if (GetClassInfoW(lpWndClass->hInstance, lpWndClass->lpszClassName, &WndClass))
		return TRUE;
	return RegisterClassW(lpWndClass);
}

/*************************************************************************
 *      @	[SHLWAPI.239]
 */
DWORD WINAPI SHLWAPI_239(HINSTANCE hInstance, LPVOID p2, DWORD dw3)
{
    FIXME("(%p %p 0x%08lx) stub\n",
	  hInstance, p2, dw3);
    return 0;
#if 0
    /* pseudo code from relay trace */
    WideCharToMultiByte(0, 0, L"Shell DocObject View", -1, &aa, 0x0207, 0, 0);
    GetClassInfoA(70fe0000,405868ec "Shell DocObject View",40586b14);
    /* above pair repeated for:
           TridentThicketUrlDlClass
	   Shell Embedding
	   CIESplashScreen
	   Inet Notify_Hidden
	   OCHost
    */
#endif
}

/*************************************************************************
 *      @	[SHLWAPI.240]
 *
 * Call The correct (ASCII/Unicode) default window procedure for a window.
 *
 * PARAMS
 *  hWnd     [I] Window to call the default proceedure for
 *  uMessage [I] Message ID
 *  wParam   [I] WPARAM of message
 *  lParam   [I] LPARAM of message
 *
 * RETURNS
 *  The result of calling the window proceedure.
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
	return /* 0xabba1243 */ 0;
}

/* default shell policy registry key */
static WCHAR strRegistryPolicyW[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o',
                                      's','o','f','t','\\','W','i','n','d','o','w','s','\\',
                                      'C','u','r','r','e','n','t','V','e','r','s','i','o','n',
                                      '\\','P','o','l','i','c','i','e','s',0};

/*************************************************************************
 * @                          [SHLWAPI.271]
 *
 * Retrieve a policy value from the registry.
 *
 * PARAMS
 *  lpSubKey   [I]   registry key name
 *  lpSubName  [I]   subname of registry key
 *  lpValue    [I]   value name of registry value
 *
 * RETURNS
 *  the value associated with the registry key or 0 if not found
 */
DWORD WINAPI SHLWAPI_271(LPCWSTR lpSubKey, LPCWSTR lpSubName, LPCWSTR lpValue)
{
	DWORD retval, datsize = 4;
	HKEY hKey;

	if (!lpSubKey)
	  lpSubKey = (LPCWSTR)strRegistryPolicyW;
	
	retval = RegOpenKeyW(HKEY_LOCAL_MACHINE, lpSubKey, &hKey);
    if (retval != ERROR_SUCCESS)
	  retval = RegOpenKeyW(HKEY_CURRENT_USER, lpSubKey, &hKey);
	if (retval != ERROR_SUCCESS)
	  return 0;

	SHGetValueW(hKey, lpSubName, lpValue, NULL, (LPBYTE)&retval, &datsize);
	RegCloseKey(hKey);
	return retval;  
}

/*************************************************************************
 * @                         [SHLWAPI.266]
 *
 * Helper function to retrieve the possibly cached value for a specific policy
 *
 * PARAMS
 *  policy     [I]   The policy to look for
 *  initial    [I]   Main registry key to open, if NULL use default
 *  polTable   [I]   Table of known policies, 0 terminated
 *  polArr     [I]   Cache array of policy values
 *
 * RETURNS
 *  The retrieved policy value or 0 if not successful
 *
 * NOTES
 *  This function is used by the native SHRestricted function to search for the
 *  policy and cache it once retrieved. The current Wine implementation uses a
 *  different POLICYDATA structure and implements a similar algorithme adapted to
 *  that structure.
 */
DWORD WINAPI SHLWAPI_266 (
	DWORD policy,
	LPCWSTR initial, /* [in] partial registry key */
	LPPOLICYDATA polTable,
	LPDWORD polArr)
{
	TRACE("(0x%08lx %s %p %p)\n", policy, debugstr_w(initial), polTable, polArr);

	if (!polTable || !polArr)
	  return 0;

	for (;polTable->policy; polTable++, polArr++)
	{
	  if (policy == polTable->policy)
	  {
	    /* we have a known policy */

	    /* check if this policy has been cached */
		if (*polArr == SHELL_NO_POLICY)
	      *polArr = SHLWAPI_271(initial, polTable->appstr, polTable->keystr);
	    return *polArr;
	  }
	}
	/* we don't know this policy, return 0 */
	TRACE("unknown policy: (%08lx)\n", policy);
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.267]
 *
 * NOTES:
 *   This QueryInterface asks the inner object for a interface. In case
 *   of aggregation this request would be forwarded by the inner to the
 *   outer object. This function asks the inner object directly for the
 *   interface circumventing the forwarding to the outer object.
 */
HRESULT WINAPI SHLWAPI_267 (
	IUnknown * pUnk,   /* [in] outer object */
	IUnknown * pInner, /* [in] inner object */
	IID * riid,
	LPVOID* ppv)
{
	HRESULT hret = E_NOINTERFACE;
	TRACE("(pUnk=%p pInner=%p\n\tIID:  %s %p)\n",pUnk,pInner,debugstr_guid(riid), ppv);

	*ppv = NULL;
	if(pUnk && pInner) {
	    hret = IUnknown_QueryInterface(pInner, riid, (LPVOID*)ppv);
	    if (SUCCEEDED(hret)) IUnknown_Release(pUnk);
	}
	TRACE("-- 0x%08lx\n", hret);
	return hret;
}

/*************************************************************************
 *      @	[SHLWAPI.268]
 *
 * Move a reference from one interface to another.
 *
 * PARAMS
 *   lpDest     [O] Destination to receive the reference
 *   lppUnknown [O] Source to give up the reference to lpDest
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI SHLWAPI_268(IUnknown *lpDest, IUnknown **lppUnknown)
{
  TRACE("(%p,%p)\n", lpDest, lppUnknown);

  if (*lppUnknown)
  {
    /* Copy Reference*/
    IUnknown_AddRef(lpDest);
    SHLWAPI_169(lppUnknown); /* Release existing interface */
  }
}

/*************************************************************************
 *      @	[SHLWAPI.276]
 *
 * Determine if the browser is integrated into the shell, and set a registry
 * key accordingly.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  1, If the browser is not integrated.
 *  2, If the browser is integrated.
 *
 * NOTES
 *  The key HKLM\Software\Microsoft\Internet Explorer\IntegratedBrowser is
 *  either set to TRUE, or removed depending on whether the browser is deemed
 *  to be integrated.
 */
DWORD WINAPI SHLWAPI_276()
{
  static LPCSTR szIntegratedBrowser = "IntegratedBrowser";
  static DWORD dwState = 0;
  HKEY hKey;
  DWORD dwRet, dwData, dwSize;

  if (dwState)
    return dwState;

  /* If shell32 exports DllGetVersion(), the browser is integrated */
  GET_FUNC(pDllGetVersion, shell32, "DllGetVersion", 1);
  dwState = pDllGetVersion ? 2 : 1;

  /* Set or delete the key accordinly */
  dwRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                        "Software\\Microsoft\\Internet Explorer", 0,
                         KEY_ALL_ACCESS, &hKey);
  if (!dwRet)
  {
    dwRet = RegQueryValueExA(hKey, szIntegratedBrowser, 0, 0,
                             (LPBYTE)&dwData, &dwSize);

    if (!dwRet && dwState == 1)
    {
      /* Value exists but browser is not integrated */
      RegDeleteValueA(hKey, szIntegratedBrowser);
    }
    else if (dwRet && dwState == 2)
    {
      /* Browser is integrated but value does not exist */
      dwData = TRUE;
      RegSetValueExA(hKey, szIntegratedBrowser, 0, REG_DWORD,
                     (LPBYTE)&dwData, sizeof(dwData));
    }
    RegCloseKey(hKey);
  }
  return dwState;
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

	FIXME("(0x%08lx %p 0x%08lx 0x%08lx %p 0x%08lx) partial stub\n",
	  wndProc,hWndParent,dwExStyle,dwStyle,hMenu,z);

	hCursor = LoadCursorA(0x00000000,IDC_ARROWA);

	if(!GetClassInfoA(shlwapi_hInstance, clsname, &wndclass))
	{
	  RtlZeroMemory(&wndclass, sizeof(WNDCLASSA));
	  wndclass.lpfnWndProc = DefWindowProcW;
	  wndclass.cbWndExtra = 4;
	  wndclass.hInstance = shlwapi_hInstance;
	  wndclass.hCursor = hCursor;
	  wndclass.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
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
 *      @	[SHLWAPI.281]
 *
 * _SHPackDispParamsV
 */
HRESULT WINAPI SHLWAPI_281(LPVOID w, LPVOID x, LPVOID y, LPVOID z)
{
	FIXME("%p %p %p %p\n",w,x,y,z);
	return E_FAIL;
}

/*************************************************************************
 *      @       [SHLWAPI.282]
 *
 * This function seems to be a forward to SHLWAPI.281 (whatever THAT
 * function does...).
 */
HRESULT WINAPI SHLWAPI_282(LPVOID w, LPVOID x, LPVOID y, LPVOID z)
{
  FIXME("%p %p %p %p\n", w, x, y, z);
  return E_FAIL;
}

/*************************************************************************
 *      @	[SHLWAPI.284]
 *
 * _IConnectionPoint_SimpleInvoke
 */
DWORD WINAPI SHLWAPI_284 (
	LPVOID x,
	LPVOID y,
	LPVOID z)
{
        TRACE("(%p %p %p) stub\n",x,y,z);
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.287]
 *
 * _IUnknown_CPContainerOnChanged
 */
HRESULT WINAPI SHLWAPI_287(LPVOID x, LPVOID y)
{
	FIXME("%p %p\n", x,y);
	return E_FAIL;
}

/*************************************************************************
 *      @	[SHLWAPI.289]
 *
 * Late bound call to winmm.PlaySoundW
 */
BOOL WINAPI SHLWAPI_289(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound)
{
  GET_FUNC(pPlaySoundW, winmm, "PlaySoundW", FALSE);
  return pPlaySoundW(pszSound, hmod, fdwSound);
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
    FIXME("('%s', '%s', '%s', %08lx, '%s'): stub!\n", str1, str2, pStr, some_len, lpStr2);
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.295]
 *
 * Called by ICQ2000b install via SHDOCVW:
 * str1: "InternetShortcut"
 * x: some unknown pointer
 * str2: "http://free.aol.com/tryaolfree/index.adp?139269"
 * str3: "C:\\WINDOWS\\Desktop.new2\\Free AOL & Unlimited Internet.url"
 *
 * In short: this one maybe creates a desktop link :-)
 */
BOOL WINAPI SHLWAPI_295(LPWSTR str1, LPVOID x, LPWSTR str2, LPWSTR str3)
{
    FIXME("('%s', %p, '%s', '%s'), stub.\n", debugstr_w(str1), x, debugstr_w(str2), debugstr_w(str3));
    return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.299]
 *
 * Late bound call to comctl32.417
 */
BOOL WINAPI SHLWAPI_299(HDC hdc, INT x, INT y, UINT flags, const RECT *lprect,
                         LPCWSTR str, UINT count, const INT *lpDx)
{
    GET_FUNC(pCOMCTL32_417, comctl32, (LPCSTR)417, FALSE);
    return pCOMCTL32_417(hdc, x, y, flags, lprect, str, count, lpDx);
}

/*************************************************************************
 *      @	[SHLWAPI.313]
 *
 * Late bound call to shell32.SHGetFileInfoW
 */
DWORD WINAPI SHLWAPI_313(LPCWSTR path, DWORD dwFileAttributes,
                         SHFILEINFOW *psfi, UINT sizeofpsfi, UINT flags)
{
  GET_FUNC(pSHGetFileInfoW, shell32, "SHGetFileInfoW", 0);
  return pSHGetFileInfoW(path, dwFileAttributes, psfi, sizeofpsfi, flags);
}

/*************************************************************************
 *      @	[SHLWAPI.318]
 *
 * Late bound call to shell32.DragQueryFileW
 */
UINT WINAPI SHLWAPI_318(HDROP hDrop, UINT lFile, LPWSTR lpszFile, UINT lLength)
{
  GET_FUNC(pDragQueryFileW, shell32, "DragQueryFileW", 0);
  return pDragQueryFileW(hDrop, lFile, lpszFile, lLength);
}

/*************************************************************************
 *      @	[SHLWAPI.333]
 *
 * Late bound call to shell32.SHBrowseForFolderW
 */
LPITEMIDLIST WINAPI SHLWAPI_333(LPBROWSEINFOW lpBi)
{
  GET_FUNC(pSHBrowseForFolderW, shell32, "SHBrowseForFolderW", NULL);
  return pSHBrowseForFolderW(lpBi);
}

/*************************************************************************
 *      @	[SHLWAPI.334]
 *
 * Late bound call to shell32.SHGetPathFromIDListW
 */
BOOL WINAPI SHLWAPI_334(LPCITEMIDLIST pidl,LPWSTR pszPath)
{
  GET_FUNC(pSHGetPathFromIDListW, shell32, "SHGetPathFromIDListW", 0);
  return pSHGetPathFromIDListW(pidl, pszPath);
}

/*************************************************************************
 *      @	[SHLWAPI.335]
 *
 * Late bound call to shell32.ShellExecuteExW
 */
BOOL WINAPI SHLWAPI_335(LPSHELLEXECUTEINFOW lpExecInfo)
{
  GET_FUNC(pShellExecuteExW, shell32, "ShellExecuteExW", FALSE);
  return pShellExecuteExW(lpExecInfo);
}

/*************************************************************************
 *      @	[SHLWAPI.336]
 *
 * Late bound call to shell32.SHFileOperationW.
 */
HICON WINAPI SHLWAPI_336(LPSHFILEOPSTRUCTW lpFileOp)
{
  GET_FUNC(pSHFileOperationW, shell32, "SHFileOperationW", 0);
  return pSHFileOperationW(lpFileOp);
}

/*************************************************************************
 *      @	[SHLWAPI.337]
 *
 * Late bound call to shell32.ExtractIconExW.
 */
UINT WINAPI SHLWAPI_337(LPCWSTR lpszFile, INT nIconIndex, HICON *phiconLarge,
                         HICON *phiconSmall, UINT nIcons)
{
  GET_FUNC(pExtractIconExW, shell32, "ExtractIconExW", 0);
  return pExtractIconExW(lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

/*************************************************************************
 *      @	[SHLWAPI.342]
 *
 */
LONG WINAPI SHInterlockedCompareExchange( PLONG dest, LONG xchg, LONG compare)
{
        return InterlockedCompareExchange(dest, xchg, compare);
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
 *      @	[SHLWAPI.350]
 *
 * seems to be late bound call to GetFileVersionInfoSizeW
 */
DWORD WINAPI SHLWAPI_350 (
	LPWSTR x,
	LPVOID y)
{
        DWORD ret;

	GET_FUNC(pGetFileVersionInfoSizeW, version, "GetFileVersionInfoSizeW", 0);
	ret = pGetFileVersionInfoSizeW(x, y);
	return 0x208 + ret;
}

/*************************************************************************
 *      @	[SHLWAPI.351]
 *
 * seems to be late bound call to GetFileVersionInfoW
 */
BOOL  WINAPI SHLWAPI_351 (
	LPWSTR w,   /* [in] path to dll */
	DWORD  x,   /* [in] parm 2 to GetFileVersionInfoA */
	DWORD  y,   /* [in] return value from .350 - assume length */
	LPVOID z)   /* [in/out] buffer (+0x208 sent to GetFileVersionInfoA) */
{
    GET_FUNC(pGetFileVersionInfoW, version, "GetFileVersionInfoW", 0);
    return pGetFileVersionInfoW(w, x, y-0x208, (char*)z+0x208);
}

/*************************************************************************
 *      @	[SHLWAPI.352]
 *
 * seems to be late bound call to VerQueryValueW
 */
WORD WINAPI SHLWAPI_352 (
	LPVOID w,   /* [in] buffer from _351 */
	LPWSTR x,   /* [in]   value to retrieve -
                              converted and passed to VerQueryValueA as #2 */
	LPVOID y,   /* [out]  ver buffer - passed to VerQueryValueA as #3 */
	UINT*  z)   /* [in]   ver length - passed to VerQueryValueA as #4 */
{
    GET_FUNC(pVerQueryValueW, version, "VerQueryValueW", 0);
    return pVerQueryValueW((char*)w+0x208, x, y, z);
}

/*************************************************************************
 *      @	[SHLWAPI.357]
 *
 * Late bound call to shell32.SHGetNewLinkInfoW
 */
BOOL WINAPI SHLWAPI_357(LPCWSTR pszLinkTo, LPCWSTR pszDir, LPWSTR pszName,
                        BOOL *pfMustCopy, UINT uFlags)
{
  GET_FUNC(pSHGetNewLinkInfoW, shell32, "SHGetNewLinkInfoW", FALSE);
  return pSHGetNewLinkInfoW(pszLinkTo, pszDir, pszName, pfMustCopy, uFlags);
}

/*************************************************************************
 *      @	[SHLWAPI.358]
 *
 * Late bound call to shell32.SHDefExtractIconW
 */
UINT WINAPI SHLWAPI_358(LPCWSTR pszIconFile, int iIndex, UINT uFlags, HICON* phiconLarge,
                         HICON* phiconSmall, UINT nIconSize)
{
  GET_FUNC(pSHDefExtractIconW, shell32, "SHDefExtractIconW", 0);
  return pSHDefExtractIconW(pszIconFile, iIndex, uFlags, phiconLarge, phiconSmall, nIconSize);
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
  GET_FUNC(pExtractIconW, shell32, "ExtractIconW", NULL);
  return pExtractIconW(hInstance, lpszExeFileName, nIconIndex);
}

/*************************************************************************
 *      @	[SHLWAPI.376]
 */
LANGID WINAPI SHLWAPI_376 ()
{
    FIXME("() stub\n");
    /* FIXME: This should be a forward in the .spec file to the win2k function
     * kernel32.GetUserDefaultUILanguage, however that function isn't there yet.
     */
    return GetUserDefaultLangID();
}

/*************************************************************************
 *      @	[SHLWAPI.377]
 *
 * FIXME: Native appears to do DPA_Create and a DPA_InsertPtr for
 *        each call here.
 * FIXME: Native shows calls to:
 *  SHRegGetUSValue for "Software\Microsoft\Internet Explorer\International"
 *                      CheckVersion
 *  RegOpenKeyExA for "HKLM\Software\Microsoft\Internet Explorer"
 *  RegQueryValueExA for "LPKInstalled"
 *  RegCloseKey
 *  RegOpenKeyExA for "HKCU\Software\Microsoft\Internet Explorer\International"
 *  RegQueryValueExA for "ResourceLocale"
 *  RegCloseKey
 *  RegOpenKeyExA for "HKLM\Software\Microsoft\Active Setup\Installed Components\{guid}"
 *  RegQueryValueExA for "Locale"
 *  RegCloseKey
 *  and then tests the Locale ("en" for me).
 *     code below
 *  after the code then a DPA_Create (first time) and DPA_InsertPtr are done.
 */
DWORD WINAPI SHLWAPI_377 (LPCSTR new_mod, HMODULE inst_hwnd, LPVOID z)
{
    CHAR mod_path[2*MAX_PATH];
    LPSTR ptr;

    GetModuleFileNameA(inst_hwnd, mod_path, 2*MAX_PATH);
    ptr = strrchr(mod_path, '\\');
    if (ptr) {
	strcpy(ptr+1, new_mod);
	TRACE("loading %s\n", debugstr_a(mod_path));
	return (DWORD)LoadLibraryA(mod_path);
    }
    return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.378]
 *
 *  This is Unicode version of .377
 */
DWORD WINAPI SHLWAPI_378 (
	LPCWSTR   new_mod,          /* [in] new module name        */
	HMODULE   inst_hwnd,        /* [in] calling module handle  */
	LPVOID z)                   /* [???] 4 */
{
    WCHAR mod_path[2*MAX_PATH];
    LPWSTR ptr;

    GetModuleFileNameW(inst_hwnd, mod_path, 2*MAX_PATH);
    ptr = strrchrW(mod_path, '\\');
    if (ptr) {
	strcpyW(ptr+1, new_mod);
	TRACE("loading %s\n", debugstr_w(mod_path));
	return (DWORD)LoadLibraryW(mod_path);
    }
    return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.389]
 *
 * Late bound call to comdlg32.GetSaveFileNameW
 */
BOOL WINAPI SHLWAPI_389(LPOPENFILENAMEW ofn)
{
  GET_FUNC(pGetSaveFileNameW, comdlg32, "GetSaveFileNameW", FALSE);
  return pGetSaveFileNameW(ofn);
}

/*************************************************************************
 *      @	[SHLWAPI.390]
 *
 * Late bound call to mpr.WNetRestoreConnectionW
 */
DWORD WINAPI SHLWAPI_390(HWND hwndOwner, LPWSTR lpszDevice)
{
  GET_FUNC(pWNetRestoreConnectionW, mpr, "WNetRestoreConnectionW", 0);
  return pWNetRestoreConnectionW(hwndOwner, lpszDevice);
}

/*************************************************************************
 *      @	[SHLWAPI.391]
 *
 * Late bound call to mpr.WNetGetLastErrorW
 */
DWORD WINAPI SHLWAPI_391(LPDWORD lpError, LPWSTR lpErrorBuf, DWORD nErrorBufSize,
                         LPWSTR lpNameBuf, DWORD nNameBufSize)
{
  GET_FUNC(pWNetGetLastErrorW, mpr, "WNetGetLastErrorW", 0);
  return pWNetGetLastErrorW(lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize);
}

/*************************************************************************
 *      @	[SHLWAPI.401]
 *
 * Late bound call to comdlg32.PageSetupDlgW
 */
BOOL WINAPI SHLWAPI_401(LPPAGESETUPDLGW pagedlg)
{
  GET_FUNC(pPageSetupDlgW, comdlg32, "PageSetupDlgW", FALSE);
  return pPageSetupDlgW(pagedlg);
}

/*************************************************************************
 *      @	[SHLWAPI.402]
 *
 * Late bound call to comdlg32.PrintDlgW
 */
BOOL WINAPI SHLWAPI_402(LPPRINTDLGW printdlg)
{
  GET_FUNC(pPrintDlgW, comdlg32, "PrintDlgW", FALSE);
  return pPrintDlgW(printdlg);
}

/*************************************************************************
 *      @	[SHLWAPI.403]
 *
 * Late bound call to comdlg32.GetOpenFileNameW
 */
BOOL WINAPI SHLWAPI_403(LPOPENFILENAMEW ofn)
{
  GET_FUNC(pGetOpenFileNameW, comdlg32, "GetOpenFileNameW", FALSE);
  return pGetOpenFileNameW(ofn);
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
 *      @       [SHLWAPI.406]
 */
DWORD WINAPI SHLWAPI_406(LPVOID u, LPVOID v, LPVOID w, LPVOID x, LPVOID y, LPVOID z)
{
  FIXME("%p %p %p %p %p %p\n", u, v, w, x, y, z);
  return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.413]
 *
 * Function unknown seems to always to return 0
 * x can be 0x3.
 */
DWORD WINAPI SHLWAPI_413 (DWORD x)
{
	FIXME("(0x%08lx)stub\n", x);
	return 0;
}

/*************************************************************************
 *      @	[SHLWAPI.418]
 *
 * Function seems to do FreeLibrary plus other things.
 *
 * FIXME native shows the following calls:
 *   RtlEnterCriticalSection
 *   LocalFree
 *   GetProcAddress(Comctl32??, 150L)
 *   DPA_DeletePtr
 *   RtlLeaveCriticalSection
 *  followed by the FreeLibrary.
 *  The above code may be related to .377 above.
 */
BOOL  WINAPI SHLWAPI_418 (HMODULE x)
{
	FIXME("(0x%08lx) partial stub\n", (LONG)x);
	return FreeLibrary(x);
}

/*************************************************************************
 *      @	[SHLWAPI.430]
 */
DWORD WINAPI SHLWAPI_430 (HINSTANCE hModule, HANDLE heap)
{
	FIXME("(0x%08lx 0x%08lx) stub\n", (DWORD)hModule, (DWORD)heap);
	return E_FAIL;   /* This is what is used if shlwapi not loaded */
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
 *      @	[SHLWAPI.269]
 *
 * Convert an ASCII string CLSID into a CLSID.
 */
BOOL WINAPI SHLWAPI_269(LPCSTR idstr, CLSID *id)
{
	WCHAR wClsid[40];
	MultiByteToWideChar(CP_ACP, 0, idstr, -1, wClsid, sizeof(wClsid)/sizeof(WCHAR));
	return SUCCEEDED(SHLWAPI_436(wClsid, id));
}

/*************************************************************************
 *      @	[SHLWAPI.270]
 *
 * Convert an Unicode string CLSID into a CLSID.
 */
BOOL WINAPI SHLWAPI_270(LPCWSTR idstr, CLSID *id)
{
	return SUCCEEDED(SHLWAPI_436(idstr, id));
}

/*************************************************************************
 *      @	[SHLWAPI.436]
 *
 * Convert an Unicode string CLSID into a CLSID.
 *
 * PARAMS
 *  idstr      [I]   string containing a CLSID in text form
 *  id         [O]   CLSID extracted from the string
 *
 * RETURNS
 *  S_OK on success or E_INVALIDARG on failure
 *
 * NOTES
 *  This is really CLSIDFromString which is exported by ole32.dll,
 *  however the native shlwapi.dll does *not* import ole32. Nor does
 *  ole32.dll import this ordinal from shlwapi. Therefore we must conclude
 *  that MS duplicated the code for CLSIDFromString, and yes they did, only
 *  it returns an E_INVALIDARG error code on failure.
 *  This is a duplicate (with changes for UNICODE) of CLSIDFromString16
 *  in dlls/ole32/compobj.c
 */
HRESULT WINAPI SHLWAPI_436(LPCWSTR idstr, CLSID *id)
{
	LPCWSTR s = idstr;
	BYTE *p;
	INT i;
	WCHAR table[256];

	if (!s) {
	  memset(id, 0, sizeof(CLSID));
	  return S_OK;
	}
	else {  /* validate the CLSID string */

	  if (strlenW(s) != 38)
	    return E_INVALIDARG;

	  if ((s[0]!=L'{') || (s[9]!=L'-') || (s[14]!=L'-') || (s[19]!=L'-') || (s[24]!=L'-') || (s[37]!=L'}'))
	    return E_INVALIDARG;

	  for (i=1; i<37; i++)
	  {
	    if ((i == 9)||(i == 14)||(i == 19)||(i == 24))
	      continue;
	    if (!(((s[i] >= L'0') && (s[i] <= L'9'))  ||
	        ((s[i] >= L'a') && (s[i] <= L'f'))  ||
	        ((s[i] >= L'A') && (s[i] <= L'F')))
	       )
	      return E_INVALIDARG;
	  }
	}

    TRACE("%s -> %p\n", debugstr_w(s), id);

  /* quick lookup table */
    memset(table, 0, 256*sizeof(WCHAR));

    for (i = 0; i < 10; i++) {
	table['0' + i] = i;
    }
    for (i = 0; i < 6; i++) {
	table['A' + i] = i+10;
	table['a' + i] = i+10;
    }

    /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

    p = (BYTE *) id;

    s++;	/* skip leading brace  */
    for (i = 0; i < 4; i++) {
	p[3 - i] = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }
    p += 4;
    s++;	/* skip - */

    for (i = 0; i < 2; i++) {
	p[1-i] = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }
    p += 2;
    s++;	/* skip - */

    for (i = 0; i < 2; i++) {
	p[1-i] = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }
    p += 2;
    s++;	/* skip - */

    /* these are just sequential bytes */
    for (i = 0; i < 2; i++) {
	*p++ = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }
    s++;	/* skip - */

    for (i = 0; i < 6; i++) {
	*p++ = table[*s]<<4 | table[*(s+1)];
	s += 2;
    }

    return S_OK;
}

/*************************************************************************
 *      @	[SHLWAPI.437]
 *
 * Determine if the OS supports a given feature.
 *
 * PARAMS
 *  dwFeature [I] Feature requested (undocumented)
 *
 * RETURNS
 *  TRUE  If the feature is available.
 *  FALSE If the feature is not available.
 */
DWORD WINAPI SHLWAPI_437 (DWORD feature)
{
  FIXME("(0x%08lx) stub\n", feature);
  return FALSE;
}

/*************************************************************************
 *      ColorRGBToHLS	[SHLWAPI.445]
 *
 * Convert from RGB COLORREF into the HLS color space.
 *
 * NOTES
 * Input HLS values are constrained to the range (0..240).
 */
VOID WINAPI ColorRGBToHLS(COLORREF drRGB, LPWORD pwHue,
			  LPWORD wLuminance, LPWORD pwSaturation)
{
    FIXME("stub\n");
    return;
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
DWORD WINAPI SHGetInverseCMAP (LPDWORD* x, DWORD why)
{
    if (why == 4) {
	FIXME(" - returning bogus address for SHGetInverseCMAP\n");
	*x = (LPDWORD)0xabba1249;
	return 0;
    }
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
 * SHSkipJunction	[SHLWAPI.@]
 *
 * Determine if a bind context can be bound to an object
 *
 * PARAMS
 *  pbc    [I] Bind context to check
 *  pclsid [I] CLSID of object to be bound to
 *
 * RETURNS
 *  TRUE: If it is safe to bind
 *  FALSE: If pbc is invalid or binding would not be safe
 *
 */
BOOL WINAPI SHSkipJunction(IBindCtx *pbc, const CLSID *pclsid)
{
  static WCHAR szSkipBinding[] = { 'S','k','i','p',' ',
    'B','i','n','d','i','n','g',' ','C','L','S','I','D','\0' };
  BOOL bRet = FALSE;

  if (pbc)
  {
    IUnknown* lpUnk;

    if (SUCCEEDED(IBindCtx_GetObjectParam(pbc, szSkipBinding, &lpUnk)))
    {
      CLSID clsid;

      if (SUCCEEDED(SHLWAPI_175(lpUnk, &clsid)) &&
          IsEqualGUID(pclsid, &clsid))
        bRet = TRUE;

      IUnknown_Release(lpUnk);
    }
  }
  return bRet;
}
