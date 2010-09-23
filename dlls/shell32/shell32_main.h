/*
 * 	internal Shell32 Library definitions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
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

#ifndef __WINE_SHELL_MAIN_H
#define __WINE_SHELL_MAIN_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "objbase.h"
#include "docobj.h"
#include "undocshell.h"
#include "shlobj.h"
#include "shellapi.h"
#include "wine/unicode.h"

/*******************************************
*  global SHELL32.DLL variables
*/
extern HMODULE	huser32;
extern HINSTANCE shell32_hInstance;
extern HIMAGELIST	ShellSmallIconList;
extern HIMAGELIST	ShellBigIconList;

/* Iconcache */
#define INVALID_INDEX -1
BOOL SIC_Initialize(void);
void SIC_Destroy(void);
BOOL PidlToSicIndex (IShellFolder * sh, LPCITEMIDLIST pidl, BOOL bBigIcon, UINT uFlags, int * pIndex);
INT SIC_GetIconIndex (LPCWSTR sSourceFile, INT dwSourceIndex, DWORD dwFlags );

/* Classes Root */
BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, LONG len, BOOL bPrependDot);
BOOL HCR_GetDefaultVerbW( HKEY hkeyClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len );
BOOL HCR_GetExecuteCommandW( HKEY hkeyClass, LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len );
BOOL HCR_GetDefaultIconW(LPCWSTR szClass, LPWSTR szDest, DWORD len, int* picon_idx);
BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len);

/* ANSI versions of above functions, supposed to go away as soon as they are not used anymore */
BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, LONG len, BOOL bPrependDot);
BOOL HCR_GetDefaultIconA(LPCSTR szClass, LPSTR szDest, DWORD len, int* picon_idx);
BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len);

BOOL HCR_GetFolderAttributes(LPCITEMIDLIST pidlFolder, LPDWORD dwAttributes);

DWORD WINAPI ParseFieldA(LPCSTR src, DWORD nField, LPSTR dst, DWORD len);
DWORD WINAPI ParseFieldW(LPCWSTR src, DWORD nField, LPWSTR dst, DWORD len);

/****************************************************************************
 * Class constructors
 */
LPDATAOBJECT	IDataObject_Constructor(HWND hwndOwner, LPCITEMIDLIST myPidl, LPCITEMIDLIST * apidl, UINT cidl);
LPENUMFORMATETC	IEnumFORMATETC_Constructor(UINT, const FORMATETC []);

LPCLASSFACTORY	IClassFactory_Constructor(REFCLSID);
IContextMenu2 *	ISvItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, const LPCITEMIDLIST *aPidls, UINT uItemCount);
IContextMenu2 *	ISvBgCm_Constructor(LPSHELLFOLDER pSFParent, BOOL bDesktop);
LPSHELLVIEW	IShellView_Constructor(LPSHELLFOLDER);

HRESULT WINAPI IFSFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IShellItem_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IShellLink_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IShellLink_ConstructFromFile(IUnknown * pUnkOuter, REFIID riid, LPCITEMIDLIST pidl, LPVOID * ppv);
HRESULT WINAPI ISF_Desktop_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_MyComputer_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_NetworkPlaces_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI Printers_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IDropTargetHelper_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IFileSystemBindData_Constructor(const WIN32_FIND_DATAW *pfd, LPBC *ppV);
HRESULT WINAPI IControlPanel_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI UnixFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI UnixDosFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI FolderShortcut_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI MyDocuments_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI RecycleBin_Constructor(IUnknown * pUnkOuter, REFIID riif, LPVOID *ppv);
HRESULT WINAPI QueryAssociations_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppOutput);
HRESULT WINAPI ExplorerBrowser_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv);
HRESULT WINAPI KnownFolderManager_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv);
extern HRESULT CPanel_GetIconLocationW(LPCITEMIDLIST, LPWSTR, UINT, int*);
HRESULT WINAPI CPanel_ExtractIconA(LPITEMIDLIST pidl, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);
HRESULT WINAPI CPanel_ExtractIconW(LPITEMIDLIST pidl, LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize);

HRESULT WINAPI IAutoComplete_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

LPEXTRACTICONA	IExtractIconA_Constructor(LPCITEMIDLIST);
LPEXTRACTICONW	IExtractIconW_Constructor(LPCITEMIDLIST);

/* initialisation for FORMATETC */
#define InitFormatEtc(fe, cf, med) \
	{\
	(fe).cfFormat=cf;\
	(fe).dwAspect=DVASPECT_CONTENT;\
	(fe).ptd=NULL;\
	(fe).tymed=med;\
	(fe).lindex=-1;\
	};

#define KeyStateToDropEffect(kst)\
    ((((kst)&(MK_CONTROL|MK_SHIFT))==(MK_CONTROL|MK_SHIFT)) ? DROPEFFECT_LINK :\
    (((kst)&(MK_CONTROL|MK_SHIFT)) ? DROPEFFECT_COPY :\
    DROPEFFECT_MOVE))


HGLOBAL RenderHDROP(LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderSHELLIDLIST (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILENAMEA (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILENAMEW (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);

/* Change Notification */
void InitChangeNotifications(void);
void FreeChangeNotifications(void);

/* file operation */
#define ASK_DELETE_FILE           1
#define ASK_DELETE_FOLDER         2
#define ASK_DELETE_MULTIPLE_ITEM  3
#define ASK_CREATE_FOLDER         4
#define ASK_OVERWRITE_FILE        5
#define ASK_DELETE_SELECTED       6
#define ASK_TRASH_FILE            7
#define ASK_TRASH_FOLDER          8
#define ASK_TRASH_MULTIPLE_ITEM   9
#define ASK_CANT_TRASH_ITEM      10
#define ASK_OVERWRITE_FOLDER     11

BOOL SHELL_ConfirmYesNoW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir);

static inline BOOL SHELL_OsIsUnicode(void)
{
    /* if high-bit of version is 0, we are emulating NT */
    return !(GetVersion() & 0x80000000);
}

#define __SHFreeAndNil(ptr) \
	{\
	  SHFree(*ptr); \
	  *ptr = NULL; \
	};
static inline void __SHCloneStrA(char ** target,const char * source)
{
	*target = SHAlloc(strlen(source)+1);
	strcpy(*target, source);
}

static inline void __SHCloneStrWtoA(char ** target, const WCHAR * source)
{
	int len = WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL);
	*target = SHAlloc(len);
	WideCharToMultiByte(CP_ACP, 0, source, -1, *target, len, NULL, NULL);
}

static inline void __SHCloneStrW(WCHAR ** target, const WCHAR * source)
{
	*target = SHAlloc( (lstrlenW(source)+1) * sizeof(WCHAR) );
	lstrcpyW(*target, source);
}

static inline WCHAR * __SHCloneStrAtoW(WCHAR ** target, const char * source)
{
	int len = MultiByteToWideChar(CP_ACP, 0, source, -1, NULL, 0);
	*target = SHAlloc(len*sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, source, -1, *target, len);
	return *target;
}


extern WCHAR swShell32Name[MAX_PATH];

BOOL UNIXFS_is_rooted_at_desktop(void);
extern const GUID CLSID_UnixFolder;
extern const GUID CLSID_UnixDosFolder;

extern BOOL run_winemenubuilder( const WCHAR *args );

/* Default shell folder value registration */
HRESULT SHELL_RegisterShellFolders(void);

/* Detect Shell Links */
BOOL SHELL_IsShortcut(LPCITEMIDLIST);

#endif
