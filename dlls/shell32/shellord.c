/*
 * The parameters of many functions changes between different OS versions
 * (NT uses Unicode strings, 95 uses ASCII strings)
 *
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
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
#include <string.h>
#include <stdio.h>
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "winnls.h"
#include "heap.h"

#include "wine/obj_base.h"
#include "shellapi.h"
#include "shlguid.h"
#include "shlobj.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "pidl.h"
#include "shlwapi.h"
#include "commdlg.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);
WINE_DECLARE_DEBUG_CHANNEL(pidl);

/*************************************************************************
 * ParseFieldA					[internal]
 *
 * copies a field from a ',' delimited string
 *
 * first field is nField = 1
 */
DWORD WINAPI ParseFieldA(
	LPCSTR src,
	DWORD nField,
	LPSTR dst,
	DWORD len)
{
	WARN("(%s,0x%08lx,%p,%ld) semi-stub.\n",debugstr_a(src),nField,dst,len);

	if (!src || !src[0] || !dst || !len)
	  return 0;

	/* skip n fields delimited by ',' */
	while (nField > 1)
	{
	  if (*src=='\0') return FALSE;
	  if (*(src++)==',') nField--;
	}

	/* copy part till the next ',' to dst */
	while ( *src!='\0' && *src!=',' && (len--)>0 ) *(dst++)=*(src++);

	/* finalize the string */
	*dst=0x0;

	return TRUE;
}

/*************************************************************************
 * ParseFieldW			[internal]
 *
 * copies a field from a ',' delimited string
 *
 * first field is nField = 1
 */
DWORD WINAPI ParseFieldW(LPCWSTR src, DWORD nField, LPWSTR dst, DWORD len)
{
	FIXME("(%s,0x%08lx,%p,%ld) stub\n",
	  debugstr_w(src), nField, dst, len);
	return FALSE;
}

/*************************************************************************
 * ParseField			[SHELL32.58]
 */
DWORD WINAPI ParseFieldAW(LPCVOID src, DWORD nField, LPVOID dst, DWORD len)
{
	if (SHELL_OsIsUnicode())
	  return ParseFieldW(src, nField, dst, len);
	return ParseFieldA(src, nField, dst, len);
}

/*************************************************************************
 * GetFileNameFromBrowse			[SHELL32.63]
 *
 */
BOOL WINAPI GetFileNameFromBrowse(
	HWND hwndOwner,
	LPSTR lpstrFile,
	DWORD nMaxFile,
	LPCSTR lpstrInitialDir,
	LPCSTR lpstrDefExt,
	LPCSTR lpstrFilter,
	LPCSTR lpstrTitle)
{
    HMODULE hmodule;
    FARPROC pGetOpenFileNameA;
    OPENFILENAMEA ofn;
    BOOL ret;

    TRACE("%04x, %s, %ld, %s, %s, %s, %s)\n",
	  hwndOwner, lpstrFile, nMaxFile, lpstrInitialDir, lpstrDefExt,
	  lpstrFilter, lpstrTitle);

    hmodule = LoadLibraryA("comdlg32.dll");
    if(!hmodule) return FALSE;
    pGetOpenFileNameA = GetProcAddress(hmodule, "GetOpenFileNameA");
    if(!pGetOpenFileNameA)
    {
	FreeLibrary(hmodule);
	return FALSE;
    }

    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFilter = lpstrFilter;
    ofn.lpstrFile = lpstrFile;
    ofn.nMaxFile = nMaxFile;
    ofn.lpstrInitialDir = lpstrInitialDir;
    ofn.lpstrTitle = lpstrTitle;
    ofn.lpstrDefExt = lpstrDefExt;
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    ret = pGetOpenFileNameA(&ofn);

    FreeLibrary(hmodule);
    return ret;
}

/*************************************************************************
 * SHGetSetSettings				[SHELL32.68]
 */
VOID WINAPI SHGetSetSettings(DWORD x, DWORD y, DWORD z)
{
	FIXME("0x%08lx 0x%08lx 0x%08lx\n", x, y, z);
}

/*************************************************************************
 * SHGetSettings				[SHELL32.@]
 *
 * NOTES
 *  the registry path are for win98 (tested)
 *  and possibly are the same in nt40
 *
 */
VOID WINAPI SHGetSettings(LPSHELLFLAGSTATE lpsfs, DWORD dwMask)
{
	HKEY	hKey;
	DWORD	dwData;
	DWORD	dwDataSize = sizeof (DWORD);

	TRACE("(%p 0x%08lx)\n",lpsfs,dwMask);

	if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
				 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0))
	  return;

	if ( (SSF_SHOWEXTENSIONS & dwMask) && !RegQueryValueExA(hKey, "HideFileExt", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowExtensions  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_SHOWINFOTIP & dwMask) && !RegQueryValueExA(hKey, "ShowInfoTip", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowInfoTip  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_DONTPRETTYPATH & dwMask) && !RegQueryValueExA(hKey, "DontPrettyPath", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fDontPrettyPath  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_HIDEICONS & dwMask) && !RegQueryValueExA(hKey, "HideIcons", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fHideIcons  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_MAPNETDRVBUTTON & dwMask) && !RegQueryValueExA(hKey, "MapNetDrvBtn", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fMapNetDrvBtn  = ((dwData == 0) ?  0 : 1);

	if ( (SSF_SHOWATTRIBCOL & dwMask) && !RegQueryValueExA(hKey, "ShowAttribCol", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	  lpsfs->fShowAttribCol  = ((dwData == 0) ?  0 : 1);

	if (((SSF_SHOWALLOBJECTS | SSF_SHOWSYSFILES) & dwMask) && !RegQueryValueExA(hKey, "Hidden", 0, 0, (LPBYTE)&dwData, &dwDataSize))
	{ if (dwData == 0)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 0;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 0;
	  }
	  else if (dwData == 1)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 1;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 0;
	  }
	  else if (dwData == 2)
	  { if (SSF_SHOWALLOBJECTS & dwMask)	lpsfs->fShowAllObjects  = 0;
	    if (SSF_SHOWSYSFILES & dwMask)	lpsfs->fShowSysFiles  = 1;
	  }
	}
	RegCloseKey (hKey);

	TRACE("-- 0x%04x\n", *(WORD*)lpsfs);
}

/*************************************************************************
 * SHShellFolderView_Message			[SHELL32.73]
 *
 * PARAMETERS
 *  hwndCabinet defines the explorer cabinet window that contains the
 *              shellview you need to communicate with
 *  uMsg        identifying the SFVM enum to perform
 *  lParam
 *
 * NOTES
 *  Message SFVM_REARRANGE = 1
 *    This message gets sent when a column gets clicked to instruct the
 *    shell view to re-sort the item list. lParam identifies the column
 *    that was clicked.
 */
int WINAPI SHShellFolderView_Message(
	HWND hwndCabinet,
	DWORD dwMessage,
	DWORD dwParam)
{
	FIXME("%04x %08lx %08lx stub\n",hwndCabinet, dwMessage, dwParam);
	return 0;
}

/*************************************************************************
 * RegisterShellHook				[SHELL32.181]
 *
 * PARAMS
 *      hwnd [I]  window handle
 *      y    [I]  flag ????
 *
 * NOTES
 *     exported by ordinal
 */
BOOL WINAPI RegisterShellHook(
	HWND hWnd,
	DWORD dwType)
{
	FIXME("(0x%08x,0x%08lx):stub.\n",hWnd, dwType);
	return TRUE;
}
/*************************************************************************
 * ShellMessageBoxW				[SHELL32.182]
 *
 * Format and output errormessage.
 *
 * idText	resource ID of title or LPSTR
 * idTitle	resource ID of title or LPSTR
 *
 * NOTES
 *     exported by ordinal
 */
int WINAPIV ShellMessageBoxW(
	HINSTANCE hInstance,
	HWND hWnd,
	LPCWSTR lpText,
	LPCWSTR lpCaption,
	UINT uType,
	...)
{
	WCHAR	szText[100],szTitle[100];
	LPCWSTR pszText = szText, pszTitle = szTitle, pszTemp;
	va_list args;
	int	ret;

	va_start(args, uType);
	/* wvsprintfA(buf,fmt, args); */

	TRACE("(%08lx,%08lx,%p,%p,%08x)\n",
	(DWORD)hInstance,(DWORD)hWnd,lpText,lpCaption,uType);

	if (!HIWORD(lpCaption))
	  LoadStringW(hInstance, (DWORD)lpCaption, szTitle, sizeof(szTitle)/sizeof(szTitle[0]));
	else
	  pszTitle = lpCaption;

	if (!HIWORD(lpText))
	  LoadStringW(hInstance, (DWORD)lpText, szText, sizeof(szText)/sizeof(szText[0]));
	else
	  pszText = lpText;

	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
		       pszText, 0, 0, (LPWSTR)&pszTemp, 0, &args);

	va_end(args);

	ret = MessageBoxW(hWnd,pszTemp,pszTitle,uType);
	LocalFree((HLOCAL)pszTemp);
	return ret;
}

/*************************************************************************
 * ShellMessageBoxA				[SHELL32.183]
 */
int WINAPIV ShellMessageBoxA(
	HINSTANCE hInstance,
	HWND hWnd,
	LPCSTR lpText,
	LPCSTR lpCaption,
	UINT uType,
	...)
{
	char	szText[100],szTitle[100];
	LPCSTR  pszText = szText, pszTitle = szTitle, pszTemp;
	va_list args;
	int	ret;

	va_start(args, uType);
	/* wvsprintfA(buf,fmt, args); */

	TRACE("(%08lx,%08lx,%p,%p,%08x)\n",
	(DWORD)hInstance,(DWORD)hWnd,lpText,lpCaption,uType);

	if (!HIWORD(lpCaption))
	  LoadStringA(hInstance, (DWORD)lpCaption, szTitle, sizeof(szTitle));
	else
	  pszTitle = lpCaption;

	if (!HIWORD(lpText))
	  LoadStringA(hInstance, (DWORD)lpText, szText, sizeof(szText));
	else
	  pszText = lpText;

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
		       pszText, 0, 0, (LPSTR)&pszTemp, 0, &args);

	va_end(args);

	ret = MessageBoxA(hWnd,pszTemp,pszTitle,uType);
	LocalFree((HLOCAL)pszTemp);
	return ret;
}

/*************************************************************************
 * SHFree					[SHELL32.195]
 *
 * NOTES
 *     free_ptr() - frees memory using IMalloc
 *     exported by ordinal
 */
#define MEM_DEBUG 0
void WINAPI SHFree(LPVOID x)
{
#if MEM_DEBUG
	WORD len = *(LPWORD)((LPBYTE)x-2);

	if ( *(LPWORD)((LPBYTE)x+len) != 0x7384)
	  ERR("MAGIC2!\n");

	if ( (*(LPWORD)((LPBYTE)x-4)) != 0x8271)
	  ERR("MAGIC1!\n");
	else
	  memset((LPBYTE)x-4, 0xde, len+6);

	TRACE("%p len=%u\n",x, len);

	x = (LPBYTE) x - 4;
#else
	TRACE("%p\n",x);
#endif
	HeapFree(GetProcessHeap(), 0, x);
}

/*************************************************************************
 * SHAlloc					[SHELL32.196]
 *
 * NOTES
 *     void *task_alloc(DWORD len), uses SHMalloc allocator
 *     exported by ordinal
 */
LPVOID WINAPI SHAlloc(DWORD len)
{
	LPBYTE ret;

#if MEM_DEBUG
	ret = (LPVOID) HeapAlloc(GetProcessHeap(),0,len+6);
#else
	ret = (LPVOID) HeapAlloc(GetProcessHeap(),0,len);
#endif

#if MEM_DEBUG
	*(LPWORD)(ret) = 0x8271;
	*(LPWORD)(ret+2) = (WORD)len;
	*(LPWORD)(ret+4+len) = 0x7384;
	ret += 4;
	memset(ret, 0xdf, len);
#endif
	TRACE("%lu bytes at %p\n",len, ret);
	return (LPVOID)ret;
}

/*************************************************************************
 * SHRegisterDragDrop				[SHELL32.86]
 *
 * NOTES
 *     exported by ordinal
 */
HRESULT WINAPI SHRegisterDragDrop(
	HWND hWnd,
	LPDROPTARGET pDropTarget)
{
	FIXME("(0x%08x,%p):stub.\n", hWnd, pDropTarget);
	if (GetShellOle()) return pRegisterDragDrop(hWnd, pDropTarget);
        return 0;
}

/*************************************************************************
 * SHRevokeDragDrop				[SHELL32.87]
 *
 * NOTES
 *     exported by ordinal
 */
HRESULT WINAPI SHRevokeDragDrop(HWND hWnd)
{
    FIXME("(0x%08x):stub.\n",hWnd);
    return 0;
}

/*************************************************************************
 * SHDoDragDrop					[SHELL32.88]
 *
 * NOTES
 *     exported by ordinal
 */
HRESULT WINAPI SHDoDragDrop(
	HWND hWnd,
	LPDATAOBJECT lpDataObject,
	LPDROPSOURCE lpDropSource,
	DWORD dwOKEffect,
	LPDWORD pdwEffect)
{
    FIXME("(0x%04x %p %p 0x%08lx %p):stub.\n",
    hWnd, lpDataObject, lpDropSource, dwOKEffect, pdwEffect);
    return 0;
}

/*************************************************************************
 * ArrangeWindows				[SHELL32.184]
 *
 */
WORD WINAPI ArrangeWindows(
	HWND hwndParent,
	DWORD dwReserved,
	LPCRECT lpRect,
	WORD cKids,
	CONST HWND * lpKids)
{
    FIXME("(0x%08x 0x%08lx %p 0x%04x %p):stub.\n",
	   hwndParent, dwReserved, lpRect, cKids, lpKids);
    return 0;
}

/*************************************************************************
 * SignalFileOpen				[SHELL32.103]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI
SignalFileOpen (DWORD dwParam1)
{
    FIXME("(0x%08lx):stub.\n", dwParam1);

    return 0;
}

/*************************************************************************
 * SHADD_get_policy - helper function for SHAddToRecentDocs
 *
 * PARAMETERS
 *   policy    [IN]  policy name (null termed string) to find
 *   type      [OUT] ptr to DWORD to receive type
 *   buffer    [OUT] ptr to area to hold data retrieved
 *   len       [IN/OUT] ptr to DWORD holding size of buffer and getting
 *                      length filled
 *
 * RETURNS
 *   result of the SHQueryValueEx call
 */
static INT SHADD_get_policy(LPSTR policy, LPDWORD type, LPVOID buffer, LPDWORD len)
{
    HKEY Policy_basekey;
    INT ret;

    /* Get the key for the policies location in the registry
     */
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		      "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
		      0, KEY_READ, &Policy_basekey)) {

	if (RegOpenKeyExA(HKEY_CURRENT_USER,
			  "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
			  0, KEY_READ, &Policy_basekey)) {
	    TRACE("No Explorer Policies location exists. Policy wanted=%s\n",
		  policy);
	    *len = 0;
	    return ERROR_FILE_NOT_FOUND;
	}
    }

    /* Retrieve the data if it exists
     */
    ret = SHQueryValueExA(Policy_basekey, policy, 0, type, buffer, len);
    RegCloseKey(Policy_basekey);
    return ret;
}


/*************************************************************************
 * SHADD_compare_mru - helper function for SHAddToRecentDocs
 *
 * PARAMETERS
 *   data1     [IN] data being looked for
 *   data2     [IN] data in MRU
 *   cbdata    [IN] length from FindMRUData call (not used)
 *
 * RETURNS
 *   position within MRU list that data was added.
 */
static INT CALLBACK SHADD_compare_mru(LPCVOID data1, LPCVOID data2, DWORD cbData)
{
    return lstrcmpiA(data1, data2);
}

/*************************************************************************
 * SHADD_create_add_mru_data - helper function for SHAddToRecentDocs
 *
 * PARAMETERS
 *   mruhandle    [IN] handle for created MRU list
 *   doc_name     [IN] null termed pure doc name
 *   new_lnk_name [IN] null termed path and file name for .lnk file
 *   buffer       [IN/OUT] 2048 byte area to consturct MRU data
 *   len          [OUT] ptr to int to receive space used in buffer
 *
 * RETURNS
 *   position within MRU list that data was added.
 */
static INT SHADD_create_add_mru_data(HANDLE mruhandle, LPSTR doc_name, LPSTR new_lnk_name,
                                     LPSTR buffer, INT *len)
{
    LPSTR ptr;
    INT wlen;

    /*FIXME: Document:
     *  RecentDocs MRU data structure seems to be:
     *    +0h   document file name w/ terminating 0h
     *    +nh   short int w/ size of remaining
     *    +n+2h 02h 30h, or 01h 30h, or 00h 30h  -  unknown
     *    +n+4h 10 bytes zeros  -   unknown
     *    +n+eh shortcut file name w/ terminating 0h
     *    +n+e+nh 3 zero bytes  -  unknown
     */

    /* Create the MRU data structure for "RecentDocs"
	 */
    ptr = buffer;
    lstrcpyA(ptr, doc_name);
    ptr += (lstrlenA(buffer) + 1);
    wlen= lstrlenA(new_lnk_name) + 1 + 12;
    *((short int*)ptr) = wlen;
    ptr += 2;   /* step past the length */
    *(ptr++) = 0x30;  /* unknown reason */
    *(ptr++) = 0;     /* unknown, but can be 0x00, 0x01, 0x02 */
    memset(ptr, 0, 10);
    ptr += 10;
    lstrcpyA(ptr, new_lnk_name);
    ptr += (lstrlenA(new_lnk_name) + 1);
    memset(ptr, 0, 3);
    ptr += 3;
    *len = ptr - buffer;

    /* Add the new entry into the MRU list
     */
    return pAddMRUData(mruhandle, (LPCVOID)buffer, *len);
}

/*************************************************************************
 * SHAddToRecentDocs				[SHELL32.@]
 *
 * PARAMETERS
 *   uFlags  [IN] SHARD_PATH or SHARD_PIDL
 *   pv      [IN] string or pidl, NULL clears the list
 *
 * NOTES
 *     exported by name
 *
 * FIXME: ?? MSDN shows this as a VOID
 */
DWORD WINAPI SHAddToRecentDocs (UINT uFlags,LPCVOID pv)
{

/* FIXME: !!! move CREATEMRULIST and flags to header file !!! */
/*        !!! it is in both here and comctl32undoc.c      !!! */
typedef struct tagCREATEMRULIST
{
    DWORD  cbSize;        /* size of struct */
    DWORD  nMaxItems;     /* max no. of items in list */
    DWORD  dwFlags;       /* see below */
    HKEY   hKey;          /* root reg. key under which list is saved */
    LPCSTR lpszSubKey;    /* reg. subkey */
    PROC   lpfnCompare;   /* item compare proc */
} CREATEMRULIST, *LPCREATEMRULIST;

/* dwFlags */
#define MRUF_STRING_LIST  0 /* list will contain strings */
#define MRUF_BINARY_LIST  1 /* list will contain binary data */
#define MRUF_DELAYED_SAVE 2 /* only save list order to reg. is FreeMRUList */

/* If list is a string list lpfnCompare has the following prototype
 * int CALLBACK MRUCompareString(LPCSTR s1, LPCSTR s2)
 * for binary lists the prototype is
 * int CALLBACK MRUCompareBinary(LPCVOID data1, LPCVOID data2, DWORD cbData)
 * where cbData is the no. of bytes to compare.
 * Need to check what return value means identical - 0?
 */


    UINT olderrormode;
    HKEY HCUbasekey;
    CHAR doc_name[MAX_PATH];
    CHAR link_dir[MAX_PATH];
    CHAR new_lnk_filepath[MAX_PATH];
    CHAR new_lnk_name[MAX_PATH];
    IMalloc *ppM;
    LPITEMIDLIST pidl;
    HWND hwnd = 0;       /* FIXME:  get real window handle */
    INT ret;
    DWORD data[64], datalen, type;

    /*FIXME: Document:
     *  RecentDocs MRU data structure seems to be:
     *    +0h   document file name w/ terminating 0h
     *    +nh   short int w/ size of remaining
     *    +n+2h 02h 30h, or 01h 30h, or 00h 30h  -  unknown
     *    +n+4h 10 bytes zeros  -   unknown
     *    +n+eh shortcut file name w/ terminating 0h
     *    +n+e+nh 3 zero bytes  -  unknown
     */

    /* See if we need to do anything.
     */
    datalen = 64;
    ret=SHADD_get_policy( "NoRecentDocsHistory", &type, &data, &datalen);
    if ((ret > 0) && (ret != ERROR_FILE_NOT_FOUND)) {
	ERR("Error %d getting policy \"NoRecentDocsHistory\"\n", ret);
	return 0;
    }
    if (ret == ERROR_SUCCESS) {
	if (!( (type == REG_DWORD) ||
	       ((type == REG_BINARY) && (datalen == 4)) )) {
	    ERR("Error policy data for \"NoRecentDocsHistory\" not formated correctly, type=%ld, len=%ld\n",
		type, datalen);
	    return 0;
	}

	TRACE("policy value for NoRecentDocsHistory = %08lx\n", data[0]);
	/* now test the actual policy value */
	if ( data[0] != 0)
	    return 0;
    }

    /* Open key to where the necessary info is
     */
    /* FIXME: This should be done during DLL PROCESS_ATTACH (or THREAD_ATTACH)
     *        and the close should be done during the _DETACH. The resulting
     *        key is stored in the DLL global data.
     */
    if (RegCreateKeyExA(HKEY_CURRENT_USER,
			"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
			0, 0, 0, KEY_READ, 0, &HCUbasekey, 0)) {
	ERR("Failed to create 'Software\\Microsoft\\Windows\\CurrentVersion\\Explorer'\n");
	return 0;
    }

    /* Get path to user's "Recent" directory
     */
    if(SUCCEEDED(SHGetMalloc(&ppM))) {
	if (SUCCEEDED(SHGetSpecialFolderLocation(hwnd, CSIDL_RECENT,
						 &pidl))) {
	    SHGetPathFromIDListA(pidl, link_dir);
	    IMalloc_Free(ppM, pidl);
	}
	else {
	    /* serious issues */
	    link_dir[0] = 0;
	    ERR("serious issues 1\n");
	}
	IMalloc_Release(ppM);
    }
    else {
	/* serious issues */
	link_dir[0] = 0;
	ERR("serious issues 2\n");
    }
    TRACE("Users Recent dir %s\n", link_dir);

    /* If no input, then go clear the lists */
    if (!pv) {
	/* clear user's Recent dir
	 */

	/* FIXME: delete all files in "link_dir"
	 *
	 * while( more files ) {
	 *    lstrcpyA(old_lnk_name, link_dir);
	 *    PathAppendA(old_lnk_name, filenam);
	 *    DeleteFileA(old_lnk_name);
	 * }
	 */
	FIXME("should delete all files in %s\\ \n", link_dir);

	/* clear MRU list
	 */
	/* MS Bug ?? v4.72.3612.1700 of shell32 does the delete against
	 *  HKEY_LOCAL_MACHINE version of ...CurrentVersion\Explorer
	 *  and naturally it fails w/ rc=2. It should do it against
	 *  HKEY_CURRENT_USER which is where it is stored, and where
	 *  the MRU routines expect it!!!!
	 */
	RegDeleteKeyA(HCUbasekey, "RecentDocs");
	RegCloseKey(HCUbasekey);
	return 0;
    }

    /* Have data to add, the jobs to be done:
     *   1. Add document to MRU list in registry "HKCU\Software\
     *      Microsoft\Windows\CurrentVersion\Explorer\RecentDocs".
     *   2. Add shortcut to document in the user's Recent directory
     *      (CSIDL_RECENT).
     *   3. Add shortcut to Start menu's Documents submenu.
     */

    /* Get the pure document name from the input
     */
    if (uFlags & SHARD_PIDL) {
	SHGetPathFromIDListA((LPCITEMIDLIST) pv, doc_name);
    }
    else {
	lstrcpyA(doc_name, (LPSTR) pv);
    }
    TRACE("full document name %s\n", doc_name);
    PathStripPathA(doc_name);
    TRACE("stripped document name %s\n", doc_name);


    /* ***  JOB 1: Update registry for ...\Explorer\RecentDocs list  *** */

    {  /* on input needs:
	*      doc_name    -  pure file-spec, no path
	*      link_dir    -  path to the user's Recent directory
	*      HCUbasekey  -  key of ...Windows\CurrentVersion\Explorer" node
	* creates:
	*      new_lnk_name-  pure file-spec, no path for new .lnk file
	*      new_lnk_filepath
	*                  -  path and file name of new .lnk file
	*/
	CREATEMRULIST mymru;
	HANDLE mruhandle;
	INT len, pos, bufused, err;
	INT i;
	DWORD attr;
	CHAR buffer[2048];
	CHAR *ptr;
	CHAR old_lnk_name[MAX_PATH];
	short int slen;

	mymru.cbSize = sizeof(CREATEMRULIST);
	mymru.nMaxItems = 15;
	mymru.dwFlags = MRUF_BINARY_LIST | MRUF_DELAYED_SAVE;
	mymru.hKey = HCUbasekey;
	mymru.lpszSubKey = "RecentDocs";
	mymru.lpfnCompare = &SHADD_compare_mru;
	mruhandle = pCreateMRUListA(&mymru);
	if (!mruhandle) {
	    /* MRU failed */
	    ERR("MRU processing failed, handle zero\n");
	    RegCloseKey(HCUbasekey);
	    return 0;
	}
	len = lstrlenA(doc_name);
	pos = pFindMRUData(mruhandle, doc_name, len, 0);

	/* Now get the MRU entry that will be replaced
	 * and delete the .lnk file for it
	 */
	if ((bufused = pEnumMRUListA(mruhandle, (pos == -1) ? 14 : pos,
				     buffer, 2048)) != -1) {
	    ptr = buffer;
	    ptr += (lstrlenA(buffer) + 1);
	    slen = *((short int*)ptr);
	    ptr += 2;  /* skip the length area */
	    if (bufused >= slen + (ptr-buffer)) {
		/* buffer size looks good */
		ptr += 12; /* get to string */
		len = bufused - (ptr-buffer);  /* get length of buf remaining */
		if ((lstrlenA(ptr) > 0) && (lstrlenA(ptr) <= len-1)) {
		    /* appears to be good string */
		    lstrcpyA(old_lnk_name, link_dir);
		    PathAppendA(old_lnk_name, ptr);
		    if (!DeleteFileA(old_lnk_name)) {
			if ((attr = GetFileAttributesA(old_lnk_name)) == -1) {
			    if ((err = GetLastError()) != ERROR_FILE_NOT_FOUND) {
				ERR("Delete for %s failed, err=%d, attr=%08lx\n",
				    old_lnk_name, err, attr);
			    }
			    else {
				TRACE("old .lnk file %s did not exist\n",
				      old_lnk_name);
			    }
			}
			else {
			    ERR("Delete for %s failed, attr=%08lx\n",
				old_lnk_name, attr);
			}
		    }
		    else {
			TRACE("deleted old .lnk file %s\n", old_lnk_name);
		    }
		}
	    }
	}

	/* Create usable .lnk file name for the "Recent" directory
	 */
	wsprintfA(new_lnk_name, "%s.lnk", doc_name);
	lstrcpyA(new_lnk_filepath, link_dir);
	PathAppendA(new_lnk_filepath, new_lnk_name);
	i = 1;
	olderrormode = SetErrorMode(SEM_FAILCRITICALERRORS);
	while (GetFileAttributesA(new_lnk_filepath) != -1) {
	    i++;
	    wsprintfA(new_lnk_name, "%s (%u).lnk", doc_name, i);
	    lstrcpyA(new_lnk_filepath, link_dir);
	    PathAppendA(new_lnk_filepath, new_lnk_name);
	}
	SetErrorMode(olderrormode);
	TRACE("new shortcut will be %s\n", new_lnk_filepath);

	/* Now add the new MRU entry and data
	 */
	pos = SHADD_create_add_mru_data(mruhandle, doc_name, new_lnk_name,
					buffer, &len);
	pFreeMRUListA(mruhandle);
	TRACE("Updated MRU list, new doc is position %d\n", pos);
    }

    /* ***  JOB 2: Create shortcut in user's "Recent" directory  *** */

    {  /* on input needs:
	*      doc_name    -  pure file-spec, no path
	*      new_lnk_filepath
	*                  -  path and file name of new .lnk file
 	*      uFlags[in]  -  flags on call to SHAddToRecentDocs
	*      pv[in]      -  document path/pidl on call to SHAddToRecentDocs
	*/
	IShellLinkA *psl = NULL;
	IPersistFile *pPf = NULL;
	HRESULT hres;
	CHAR desc[MAX_PATH];
	WCHAR widelink[MAX_PATH];

	CoInitialize(0);

	hres = CoCreateInstance( &CLSID_ShellLink,
				 NULL,
				 CLSCTX_INPROC_SERVER,
				 &IID_IShellLinkA,
				 (LPVOID )&psl);
	if(SUCCEEDED(hres)) {

	    hres = IShellLinkA_QueryInterface(psl, &IID_IPersistFile,
					     (LPVOID *)&pPf);
	    if(FAILED(hres)) {
		/* bombed */
		ERR("failed QueryInterface for IPersistFile %08lx\n", hres);
		goto fail;
	    }

	    /* Set the document path or pidl */
	    if (uFlags & SHARD_PIDL) {
		hres = IShellLinkA_SetIDList(psl, (LPCITEMIDLIST) pv);
	    } else {
		hres = IShellLinkA_SetPath(psl, (LPCSTR) pv);
	    }
	    if(FAILED(hres)) {
		/* bombed */
		ERR("failed Set{IDList|Path} %08lx\n", hres);
		goto fail;
	    }

	    lstrcpyA(desc, "Shortcut to ");
	    lstrcatA(desc, doc_name);
	    hres = IShellLinkA_SetDescription(psl, desc);
	    if(FAILED(hres)) {
		/* bombed */
		ERR("failed SetDescription %08lx\n", hres);
		goto fail;
	    }

	    MultiByteToWideChar(CP_ACP, 0, new_lnk_filepath, -1,
				widelink, MAX_PATH);
	    /* create the short cut */
	    hres = IPersistFile_Save(pPf, widelink, TRUE);
	    if(FAILED(hres)) {
		/* bombed */
		ERR("failed IPersistFile::Save %08lx\n", hres);
		IPersistFile_Release(pPf);
		IShellLinkA_Release(psl);
		goto fail;
	    }
	    hres = IPersistFile_SaveCompleted(pPf, widelink);
	    IPersistFile_Release(pPf);
	    IShellLinkA_Release(psl);
	    TRACE("shortcut %s has been created, result=%08lx\n",
		  new_lnk_filepath, hres);
	}
	else {
	    ERR("CoCreateInstance failed, hres=%08lx\n", hres);
	}
    }

 fail:
    CoUninitialize();

    /* all done */
    RegCloseKey(HCUbasekey);
    return 0;
}

/*************************************************************************
 * SHCreateShellFolderViewEx			[SHELL32.174]
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderViewEx(
	LPCSHELLFOLDERVIEWINFO psvcbi, /* [in] shelltemplate struct */
	LPSHELLVIEW* ppv)              /* [out] IShellView pointer */
{
	IShellView * psf;
	HRESULT hRes;

	TRACE("sf=%p pidl=%p cb=%p mode=0x%08x parm=0x%08lx\n",
	  psvcbi->pshf, psvcbi->pidlFolder, psvcbi->lpfnCallback,
	  psvcbi->uViewMode, psvcbi->dwUser);

	psf = IShellView_Constructor(psvcbi->pshf);

	if (!psf)
	  return E_OUTOFMEMORY;

	IShellView_AddRef(psf);
	hRes = IShellView_QueryInterface(psf, &IID_IShellView, (LPVOID *)ppv);
	IShellView_Release(psf);

	return hRes;
}
/*************************************************************************
 *  SHWinHelp					[SHELL32.127]
 *
 */
HRESULT WINAPI SHWinHelp (DWORD v, DWORD w, DWORD x, DWORD z)
{	FIXME("0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",v,w,x,z);
	return 0;
}
/*************************************************************************
 *  SHRunControlPanel [SHELL32.161]
 *
 */
HRESULT WINAPI SHRunControlPanel (DWORD x, DWORD z)
{	FIXME("0x%08lx 0x%08lx stub\n",x,z);
	return 0;
}
/*************************************************************************
 * ShellExecuteEx				[SHELL32.291]
 *
 */
BOOL WINAPI ShellExecuteExAW (LPVOID sei)
{	if (SHELL_OsIsUnicode())
	  return ShellExecuteExW (sei);
	return ShellExecuteExA (sei);
}
/*************************************************************************
 * ShellExecuteExA				[SHELL32.292]
 *
 * placeholder in the commandline:
 *	%1 file
 *	%2 printer
 *	%3 driver
 *	%4 port
 *	%I adress of a global item ID (explorer switch /idlist)
 *	%L ??? path/url/current file ???
 *	%S ???
 *	%* all following parameters (see batfile)
 */
BOOL WINAPI ShellExecuteExA (LPSHELLEXECUTEINFOA sei)
{ 	CHAR szApplicationName[MAX_PATH],szCommandline[MAX_PATH],szPidl[20];
	LPSTR pos;
	int gap, len;
	STARTUPINFOA  startup;
	PROCESS_INFORMATION info;

	WARN("mask=0x%08lx hwnd=0x%04x verb=%s file=%s parm=%s dir=%s show=0x%08x class=%s incomplete\n",
	     sei->fMask, sei->hwnd, debugstr_a(sei->lpVerb),
	     debugstr_a(sei->lpFile), debugstr_a(sei->lpParameters),
	     debugstr_a(sei->lpDirectory), sei->nShow,
	     (sei->fMask & SEE_MASK_CLASSNAME) ? debugstr_a(sei->lpClass) : "not used");

	ZeroMemory(szApplicationName,MAX_PATH);
	if (sei->lpFile)
	  strcpy(szApplicationName, sei->lpFile);

	ZeroMemory(szCommandline,MAX_PATH);
	if (sei->lpParameters)
	  strcpy(szCommandline, sei->lpParameters);

	if (sei->fMask & (SEE_MASK_CLASSKEY | SEE_MASK_INVOKEIDLIST | SEE_MASK_ICON | SEE_MASK_HOTKEY |
			  SEE_MASK_CONNECTNETDRV | SEE_MASK_FLAG_DDEWAIT |
			  SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE |
			  SEE_MASK_NO_CONSOLE | SEE_MASK_ASYNCOK | SEE_MASK_HMONITOR ))
	{
	  FIXME("flags ignored: 0x%08lx\n", sei->fMask);
	}

	/* launch a document by fileclass like 'Wordpad.Document.1' */
	if (sei->fMask & SEE_MASK_CLASSNAME)
	{
	  /* FIXME: szCommandline should not be of a fixed size. Plus MAX_PATH is way too short! */
	  /* the commandline contains 'c:\Path\wordpad.exe "%1"' */
	  HCR_GetExecuteCommand(sei->lpClass, (sei->lpVerb) ? sei->lpVerb : "open", szCommandline, sizeof(szCommandline));
	  /* FIXME: get the extension of lpFile, check if it fits to the lpClass */
	  TRACE("SEE_MASK_CLASSNAME->'%s'\n", szCommandline);
	}

	/* process the IDList */
	if ( (sei->fMask & SEE_MASK_INVOKEIDLIST) == SEE_MASK_INVOKEIDLIST) /*0x0c*/
	{
	  SHGetPathFromIDListA (sei->lpIDList,szApplicationName);
	  TRACE("-- idlist=%p (%s)\n", sei->lpIDList, szApplicationName);
	}
	else
	{
	  if (sei->fMask & SEE_MASK_IDLIST )
	  {
	    pos = strstr(szCommandline, "%I");
	    if (pos)
	    {
	      LPVOID pv;
	      HGLOBAL hmem = SHAllocShared ( sei->lpIDList, ILGetSize(sei->lpIDList), 0);
	      pv = SHLockShared(hmem,0);
	      sprintf(szPidl,":%p",pv );
	      SHUnlockShared(pv);

	      gap = strlen(szPidl);
	      len = strlen(pos)-2;
	      memmove(pos+gap,pos+2,len);
	      memcpy(pos,szPidl,gap);

	    }
	  }
	}

	TRACE("execute:'%s','%s'\n",szApplicationName, szCommandline);

	if (szCommandline[0]) {
	  strcat(szApplicationName, " ");
	  strcat(szApplicationName, szCommandline);
	}

	ZeroMemory(&startup,sizeof(STARTUPINFOA));
	startup.cb = sizeof(STARTUPINFOA);

	if (! CreateProcessA(NULL, szApplicationName,
			 NULL, NULL, FALSE, 0,
			 NULL, sei->lpDirectory,
			 &startup, &info))
	{
        BOOL failed = TRUE;

        if ((!sei->lpVerb)||(strcasecmp(sei->lpVerb,"open")))
        {
            LPSTR   ext = PathFindExtensionA(szApplicationName);
            CHAR    key[1023];
            CHAR    buffer[1023];
            CHAR    cmdline[1023];
            DWORD   size;

            sprintf(key,"Software\\Classes\\%s",ext);
            size = 1023;
            if (!RegQueryValueA(HKEY_LOCAL_MACHINE,key,buffer,&size))
            {
                sprintf(key,"Software\\Classes\\%s\\shell\\%s\\command", buffer,
                    (sei->lpVerb)?sei->lpVerb:"open");
                size = 1023;
                if (!RegQueryValueA(HKEY_LOCAL_MACHINE,key,buffer,&size))
                {
                    sprintf(cmdline,"%s \"%s\"",buffer,szApplicationName);
                    if (CreateProcessA(NULL,cmdline,  NULL, NULL, FALSE, 0,
                                   NULL, sei->lpDirectory, &startup, &info))
                        failed = FALSE;
                }
            }
        }

        if (failed)
        {
            sei->hInstApp = GetLastError();
            return FALSE;
        }
	}

        sei->hInstApp = 33;

	if(sei->fMask & SEE_MASK_NOCLOSEPROCESS)
	  sei->hProcess = info.hProcess;
        else
          CloseHandle( info.hProcess );
        CloseHandle( info.hThread );
	return TRUE;
}
/*************************************************************************
 * ShellExecuteExW				[SHELL32.293]
 *
 */
BOOL WINAPI ShellExecuteExW (LPSHELLEXECUTEINFOW sei)
{	SHELLEXECUTEINFOA seiA;
	DWORD ret;

	TRACE("%p\n", sei);

	memcpy(&seiA, sei, sizeof(SHELLEXECUTEINFOA));

        if (sei->lpVerb)
	  seiA.lpVerb = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpVerb);

        if (sei->lpFile)
	  seiA.lpFile = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpFile);

        if (sei->lpParameters)
	  seiA.lpParameters = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpParameters);

	if (sei->lpDirectory)
	  seiA.lpDirectory = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpDirectory);

        if ((sei->fMask & SEE_MASK_CLASSNAME) && sei->lpClass)
	  seiA.lpClass = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpClass);
	else
	  seiA.lpClass = NULL;

	ret = ShellExecuteExA(&seiA);

        if (seiA.lpVerb)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpVerb );
	if (seiA.lpFile)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpFile );
	if (seiA.lpParameters)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpParameters );
	if (seiA.lpDirectory)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpDirectory );
	if (seiA.lpClass)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpClass );

 	return ret;
}

static LPUNKNOWN SHELL32_IExplorerInterface=0;
/*************************************************************************
 * SHSetInstanceExplorer			[SHELL32.176]
 *
 * NOTES
 *  Sets the interface
 */
HRESULT WINAPI SHSetInstanceExplorer (LPUNKNOWN lpUnknown)
{	TRACE("%p\n", lpUnknown);
	SHELL32_IExplorerInterface = lpUnknown;
	return (HRESULT) lpUnknown;
}
/*************************************************************************
 * SHGetInstanceExplorer			[SHELL32.@]
 *
 * NOTES
 *  gets the interface pointer of the explorer and a reference
 */
HRESULT WINAPI SHGetInstanceExplorer (LPUNKNOWN * lpUnknown)
{	TRACE("%p\n", lpUnknown);

	*lpUnknown = SHELL32_IExplorerInterface;

	if (!SHELL32_IExplorerInterface)
	  return E_FAIL;

	IUnknown_AddRef(SHELL32_IExplorerInterface);
	return NOERROR;
}
/*************************************************************************
 * SHFreeUnusedLibraries			[SHELL32.123]
 *
 * NOTES
 *  exported by name
 */
void WINAPI SHFreeUnusedLibraries (void)
{
	FIXME("stub\n");
}
/*************************************************************************
 * DAD_SetDragImage				[SHELL32.136]
 *
 * NOTES
 *  exported by name
 */
BOOL WINAPI DAD_SetDragImage(
	HIMAGELIST himlTrack,
	LPPOINT lppt)
{
	FIXME("%p %p stub\n",himlTrack, lppt);
  return 0;
}
/*************************************************************************
 * DAD_ShowDragImage				[SHELL32.137]
 *
 * NOTES
 *  exported by name
 */
BOOL WINAPI DAD_ShowDragImage(BOOL bShow)
{
	FIXME("0x%08x stub\n",bShow);
	return 0;
}
/*************************************************************************
 * ReadCabinetState				[SHELL32.651] NT 4.0
 *
 */
HRESULT WINAPI ReadCabinetState(DWORD u, DWORD v)
{	FIXME("0x%04lx 0x%04lx stub\n",u,v);
	return 0;
}
/*************************************************************************
 * WriteCabinetState				[SHELL32.652] NT 4.0
 *
 */
HRESULT WINAPI WriteCabinetState(DWORD u)
{	FIXME("0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * FileIconInit 				[SHELL32.660]
 *
 */
BOOL WINAPI FileIconInit(BOOL bFullInit)
{	FIXME("(%s)\n", bFullInit ? "true" : "false");
	return 0;
}
/*************************************************************************
 * IsUserAdmin					[SHELL32.680] NT 4.0
 *
 */
HRESULT WINAPI IsUserAdmin(void)
{	FIXME("stub\n");
	return TRUE;
}

/*************************************************************************
 * SHAllocShared				[SHELL32.520]
 *
 * NOTES
 *  parameter1 is return value from HeapAlloc
 *  parameter2 is equal to the size allocated with HeapAlloc
 *  parameter3 is return value from GetCurrentProcessId
 *
 *  the return value is posted as lParam with 0x402 (WM_USER+2) to somewhere
 *  WM_USER+2 could be the undocumented CWM_SETPATH
 *  the allocated memory contains a pidl
 */
HGLOBAL WINAPI SHAllocShared(LPVOID psrc, DWORD size, DWORD procID)
{	HGLOBAL hmem;
	LPVOID pmem;

	TRACE("ptr=%p size=0x%04lx procID=0x%04lx\n",psrc,size,procID);
	hmem = GlobalAlloc(GMEM_FIXED, size);
	if (!hmem)
	  return 0;

	pmem =  GlobalLock (hmem);

	if (! pmem)
	  return 0;

	memcpy (pmem, psrc, size);
	GlobalUnlock(hmem);
	return hmem;
}
/*************************************************************************
 * SHLockShared					[SHELL32.521]
 *
 * NOTES
 *  parameter1 is return value from SHAllocShared
 *  parameter2 is return value from GetCurrentProcessId
 *  the receiver of (WM_USER+2) tries to lock the HANDLE (?)
 *  the return value seems to be a memory address
 */
LPVOID WINAPI SHLockShared(HANDLE hmem, DWORD procID)
{	TRACE("handle=0x%04x procID=0x%04lx\n",hmem,procID);
	return GlobalLock(hmem);
}
/*************************************************************************
 * SHUnlockShared				[SHELL32.522]
 *
 * NOTES
 *  parameter1 is return value from SHLockShared
 */
BOOL WINAPI SHUnlockShared(LPVOID pv)
{
	TRACE("%p\n",pv);
	return GlobalUnlock((HANDLE)pv);
}
/*************************************************************************
 * SHFreeShared					[SHELL32.523]
 *
 * NOTES
 *  parameter1 is return value from SHAllocShared
 *  parameter2 is return value from GetCurrentProcessId
 */
BOOL WINAPI SHFreeShared(
	HANDLE hMem,
	DWORD pid)
{
	TRACE("handle=0x%04x 0x%04lx\n",hMem,pid);
	return GlobalFree(hMem);
}

/*************************************************************************
 * SetAppStartingCursor				[SHELL32.99]
 */
HRESULT WINAPI SetAppStartingCursor(HWND u, DWORD v)
{	FIXME("hwnd=0x%04x 0x%04lx stub\n",u,v );
	return 0;
}
/*************************************************************************
 * SHLoadOLE					[SHELL32.151]
 *
 */
HRESULT WINAPI SHLoadOLE(DWORD u)
{	FIXME("0x%04lx stub\n",u);
	return S_OK;
}
/*************************************************************************
 * DriveType					[SHELL32.64]
 *
 */
HRESULT WINAPI DriveType(DWORD u)
{	FIXME("0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * SHAbortInvokeCommand				[SHELL32.198]
 *
 */
HRESULT WINAPI SHAbortInvokeCommand(void)
{	FIXME("stub\n");
	return 1;
}
/*************************************************************************
 * SHOutOfMemoryMessageBox			[SHELL32.126]
 *
 */
int WINAPI SHOutOfMemoryMessageBox(
	HWND hwndOwner,
	LPCSTR lpCaption,
	UINT uType)
{
	FIXME("0x%04x %s 0x%08x stub\n",hwndOwner, lpCaption, uType);
	return 0;
}
/*************************************************************************
 * SHFlushClipboard				[SHELL32.121]
 *
 */
HRESULT WINAPI SHFlushClipboard(void)
{	FIXME("stub\n");
	return 1;
}

/*************************************************************************
 * SHWaitForFileToOpen				[SHELL32.97]
 *
 */
BOOL WINAPI SHWaitForFileToOpen(
	LPCITEMIDLIST pidl,
	DWORD dwFlags,
	DWORD dwTimeout)
{
	FIXME("%p 0x%08lx 0x%08lx stub\n", pidl, dwFlags, dwTimeout);
	return 0;
}

/************************************************************************
 *	@				[SHELL32.654]
 *
 * NOTES: first parameter seems to be a pointer (same as passed to WriteCabinetState)
 * second one could be a size (0x0c). The size is the same as the structure saved to
 * HCU\Software\Microsoft\Windows\CurrentVersion\Explorer\CabinetState
 * I'm (js) guessing: this one is just ReadCabinetState ;-)
 */
HRESULT WINAPI shell32_654 (DWORD x, DWORD y)
{	FIXME("0x%08lx 0x%08lx stub\n",x,y);
	return 0;
}

/************************************************************************
 *	RLBuildListOfPaths			[SHELL32.146]
 *
 * NOTES
 *   builds a DPA
 */
DWORD WINAPI RLBuildListOfPaths (void)
{	FIXME("stub\n");
	return 0;
}
/************************************************************************
 *	SHValidateUNC				[SHELL32.173]
 *
 */
HRESULT WINAPI SHValidateUNC (DWORD x, DWORD y, DWORD z)
{
	FIXME("0x%08lx 0x%08lx 0x%08lx stub\n",x,y,z);
	return 0;
}

/************************************************************************
 *	DoEnvironmentSubstA			[SHELL32.@]
 *
 */
HRESULT WINAPI DoEnvironmentSubstA(LPSTR x, LPSTR y)
{
	FIXME("(%s, %s) stub\n", debugstr_a(x), debugstr_a(y));
	return 0;
}

/************************************************************************
 *	DoEnvironmentSubstW			[SHELL32.@]
 *
 */
HRESULT WINAPI DoEnvironmentSubstW(LPWSTR x, LPWSTR y)
{
	FIXME("(%s, %s): stub\n", debugstr_w(x), debugstr_w(y));
	return 0;
}

/************************************************************************
 *	DoEnvironmentSubst			[SHELL32.53]
 *
 */
HRESULT WINAPI DoEnvironmentSubstAW(LPVOID x, LPVOID y)
{
	if (SHELL_OsIsUnicode())
	  return DoEnvironmentSubstW(x, y);
	return DoEnvironmentSubstA(x, y);
}

/*************************************************************************
 *      @                             [SHELL32.243]
 *
 * Win98+ by-ordinal routine.  In Win98 this routine returns zero and
 * does nothing else.  Possibly this does something in NT or SHELL32 5.0?
 *
 */

BOOL WINAPI shell32_243(DWORD a, DWORD b)
{
  return FALSE;
}

/*************************************************************************
 *      @	[SHELL32.714]
 */
DWORD WINAPI SHELL32_714(LPVOID x)
{
 	FIXME("(%s)stub\n", debugstr_w(x));
	return 0;
}

/*************************************************************************
 *      SHAddFromPropSheetExtArray	[SHELL32.167]
 */
DWORD WINAPI SHAddFromPropSheetExtArray(DWORD a, DWORD b, DWORD c)
{
 	FIXME("(%08lx,%08lx,%08lx)stub\n", a, b, c);
	return 0;
}

/*************************************************************************
 *      SHCreatePropSheetExtArray	[SHELL32.168]
 */
DWORD WINAPI SHCreatePropSheetExtArray(DWORD a, LPCSTR b, DWORD c)
{
 	FIXME("(%08lx,%s,%08lx)stub\n", a, debugstr_a(b), c);
	return 0;
}

/*************************************************************************
 *      SHReplaceFromPropSheetExtArray	[SHELL32.170]
 */
DWORD WINAPI SHReplaceFromPropSheetExtArray(DWORD a, DWORD b, DWORD c, DWORD d)
{
 	FIXME("(%08lx,%08lx,%08lx,%08lx)stub\n", a, b, c, d);
	return 0;
}

/*************************************************************************
 *      SHDestroyPropSheetExtArray	[SHELL32.169]
 */
DWORD WINAPI SHDestroyPropSheetExtArray(DWORD a)
{
 	FIXME("(%08lx)stub\n", a);
	return 0;
}

/*************************************************************************
 *      CIDLData_CreateFromIDArray	[SHELL32.83]
 *
 *  Create IDataObject from PIDLs??
 */
HRESULT WINAPI CIDLData_CreateFromIDArray(
	LPCITEMIDLIST pidlFolder,
	DWORD cpidlFiles,
	LPCITEMIDLIST *lppidlFiles,
	LPDATAOBJECT *ppdataObject)
{
    INT i;
    HWND hwnd = 0;   /*FIXME: who should be hwnd of owner? set to desktop */

    TRACE("(%p, %ld, %p, %p)\n", pidlFolder, cpidlFiles, lppidlFiles, ppdataObject);
    if (TRACE_ON(pidl))
    {
	pdump (pidlFolder);
	for (i=0; i<cpidlFiles; i++) pdump (lppidlFiles[i]);
    }
    *ppdataObject = IDataObject_Constructor( hwnd, pidlFolder,
					     lppidlFiles, cpidlFiles);
    if (*ppdataObject) return S_OK;
    return E_OUTOFMEMORY;
}
