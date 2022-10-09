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
#include "shlobj.h"
#include "shellapi.h"
#include "wine/heap.h"
#include "wine/list.h"

/*******************************************
*  global SHELL32.DLL variables
*/
extern HMODULE	huser32 DECLSPEC_HIDDEN;
extern HINSTANCE shell32_hInstance DECLSPEC_HIDDEN;

extern CLSID CLSID_ShellImageDataFactory;

/* Iconcache */
#define INVALID_INDEX -1
void SIC_Destroy(void) DECLSPEC_HIDDEN;
BOOL PidlToSicIndex (IShellFolder * sh, LPCITEMIDLIST pidl, BOOL bBigIcon, UINT uFlags, int * pIndex) DECLSPEC_HIDDEN;
INT SIC_GetIconIndex (LPCWSTR sSourceFile, INT dwSourceIndex, DWORD dwFlags ) DECLSPEC_HIDDEN;
HRESULT SIC_get_location( int list_idx, WCHAR *file, DWORD *size, int *res_idx ) DECLSPEC_HIDDEN;

/* Classes Root */
BOOL HCR_MapTypeToValueW(LPCWSTR szExtension, LPWSTR szFileType, LONG len, BOOL bPrependDot) DECLSPEC_HIDDEN;
BOOL HCR_GetDefaultVerbW( HKEY hkeyClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len ) DECLSPEC_HIDDEN;
BOOL HCR_GetExecuteCommandW( HKEY hkeyClass, LPCWSTR szClass, LPCWSTR szVerb, LPWSTR szDest, DWORD len ) DECLSPEC_HIDDEN;
BOOL HCR_GetDefaultIconW(LPCWSTR szClass, LPWSTR szDest, DWORD len, int* picon_idx) DECLSPEC_HIDDEN;
BOOL HCR_GetClassNameW(REFIID riid, LPWSTR szDest, DWORD len) DECLSPEC_HIDDEN;

/* ANSI versions of above functions, supposed to go away as soon as they are not used anymore */
BOOL HCR_MapTypeToValueA(LPCSTR szExtension, LPSTR szFileType, LONG len, BOOL bPrependDot) DECLSPEC_HIDDEN;
BOOL HCR_GetDefaultIconA(LPCSTR szClass, LPSTR szDest, DWORD len, int* picon_idx) DECLSPEC_HIDDEN;
BOOL HCR_GetClassNameA(REFIID riid, LPSTR szDest, DWORD len) DECLSPEC_HIDDEN;

BOOL HCR_GetFolderAttributes(LPCITEMIDLIST pidlFolder, LPDWORD dwAttributes) DECLSPEC_HIDDEN;

DWORD WINAPI ParseFieldA(LPCSTR src, DWORD nField, LPSTR dst, DWORD len) DECLSPEC_HIDDEN;
DWORD WINAPI ParseFieldW(LPCWSTR src, DWORD nField, LPWSTR dst, DWORD len) DECLSPEC_HIDDEN;

BOOL WINAPI GUIDFromStringW(LPCWSTR, LPGUID) DECLSPEC_HIDDEN;

/****************************************************************************
 * Class constructors
 */
LPDATAOBJECT	IDataObject_Constructor(HWND hwndOwner, LPCITEMIDLIST myPidl, LPCITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;
LPENUMFORMATETC	IEnumFORMATETC_Constructor(UINT, const FORMATETC []) DECLSPEC_HIDDEN;

LPCLASSFACTORY	IClassFactory_Constructor(REFCLSID) DECLSPEC_HIDDEN;
HRESULT ItemMenu_Constructor(IShellFolder*, LPCITEMIDLIST, const LPCITEMIDLIST*, UINT, REFIID, void**) DECLSPEC_HIDDEN;
HRESULT BackgroundMenu_Constructor(IShellFolder*, BOOL, REFIID, void**) DECLSPEC_HIDDEN;
LPSHELLVIEW	IShellView_Constructor(LPSHELLFOLDER) DECLSPEC_HIDDEN;

HRESULT WINAPI IFSFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI IShellDispatch_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI IShellItem_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI IShellLink_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI ISF_Desktop_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI ISF_MyComputer_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI ISF_NetworkPlaces_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI Printers_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI IDropTargetHelper_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI IFileSystemBindData_Constructor(const WIN32_FIND_DATAW *pfd, LPBC *ppV) DECLSPEC_HIDDEN;
HRESULT WINAPI IControlPanel_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI UnixFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI UnixDosFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI FolderShortcut_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI MyDocuments_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI RecycleBin_Constructor(IUnknown * pUnkOuter, REFIID riif, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI QueryAssociations_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppOutput) DECLSPEC_HIDDEN;
HRESULT WINAPI ExplorerBrowser_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI KnownFolderManager_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI IFileOperation_Constructor(IUnknown *outer, REFIID riid, void **out) DECLSPEC_HIDDEN;
HRESULT WINAPI ActiveDesktop_Constructor(IUnknown *outer, REFIID riid, void **out) DECLSPEC_HIDDEN;

extern HRESULT CPanel_GetIconLocationW(LPCITEMIDLIST, LPWSTR, UINT, int*) DECLSPEC_HIDDEN;
HRESULT WINAPI CPanel_ExtractIconA(LPITEMIDLIST pidl, LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) DECLSPEC_HIDDEN;
HRESULT WINAPI CPanel_ExtractIconW(LPITEMIDLIST pidl, LPCWSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize) DECLSPEC_HIDDEN;

HRESULT WINAPI IAutoComplete_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI ApplicationAssociationRegistration_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;

HRESULT WINAPI ApplicationDestinations_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT WINAPI ApplicationDocumentLists_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv) DECLSPEC_HIDDEN;

HRESULT IShellLink_ConstructFromFile(IUnknown * pUnkOuter, REFIID riid, LPCITEMIDLIST pidl, IUnknown **ppv) DECLSPEC_HIDDEN;

LPEXTRACTICONA	IExtractIconA_Constructor(LPCITEMIDLIST) DECLSPEC_HIDDEN;
LPEXTRACTICONW	IExtractIconW_Constructor(LPCITEMIDLIST) DECLSPEC_HIDDEN;

HRESULT WINAPI CustomDestinationList_Constructor(IUnknown *outer, REFIID riid, void **obj) DECLSPEC_HIDDEN;

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


HGLOBAL RenderHDROP(LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;
HGLOBAL RenderSHELLIDLIST (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;
HGLOBAL RenderFILENAMEA (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;
HGLOBAL RenderFILENAMEW (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl) DECLSPEC_HIDDEN;

/* Change Notification */
void InitChangeNotifications(void) DECLSPEC_HIDDEN;
void FreeChangeNotifications(void) DECLSPEC_HIDDEN;

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

BOOL SHELL_ConfirmYesNoW(HWND hWnd, int nKindOfDialog, LPCWSTR szDir) DECLSPEC_HIDDEN;

static inline BOOL SHELL_OsIsUnicode(void)
{
    /* if high-bit of version is 0, we are emulating NT */
    return !(GetVersion() & 0x80000000);
}

static inline WCHAR * __SHCloneStrAtoW(WCHAR ** target, const char * source)
{
	int len = MultiByteToWideChar(CP_ACP, 0, source, -1, NULL, 0);
	*target = SHAlloc(len*sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, source, -1, *target, len);
	return *target;
}


extern WCHAR swShell32Name[MAX_PATH] DECLSPEC_HIDDEN;

extern const GUID CLSID_UnixFolder DECLSPEC_HIDDEN;
extern const GUID CLSID_UnixDosFolder DECLSPEC_HIDDEN;

extern BOOL run_winemenubuilder( const WCHAR *args ) DECLSPEC_HIDDEN;

/* Default shell folder value registration */
HRESULT SHELL_RegisterShellFolders(void) DECLSPEC_HIDDEN;

/* Detect Shell Links */
BOOL SHELL_IsShortcut(LPCITEMIDLIST) DECLSPEC_HIDDEN;


/* IEnumIDList stuff */
struct pidl_enum_entry
{
    struct list entry;
    LPITEMIDLIST pidl;
};

typedef struct
{
    IEnumIDList  IEnumIDList_iface;
    LONG         ref;
    struct list  pidls;
    struct list *current;
} IEnumIDListImpl;

/* Creates an IEnumIDList; add LPITEMIDLISTs to it with AddToEnumList. */
IEnumIDListImpl *IEnumIDList_Constructor(void) DECLSPEC_HIDDEN;
BOOL AddToEnumList(IEnumIDListImpl *list, LPITEMIDLIST pidl) DECLSPEC_HIDDEN;

/* Enumerates the folders and/or files (depending on dwFlags) in lpszPath and
 * adds them to the already-created list.
 */
BOOL CreateFolderEnumList(IEnumIDListImpl *list, LPCWSTR lpszPath, DWORD dwFlags) DECLSPEC_HIDDEN;

enum tid_t {
    NULL_tid,
    IShellDispatch6_tid,
    IShellFolderViewDual3_tid,
    Folder3_tid,
    FolderItem2_tid,
    FolderItems3_tid,
    FolderItemVerb_tid,
    FolderItemVerbs_tid,
    IShellLinkDual2_tid,
    LAST_tid
};

HRESULT get_typeinfo(enum tid_t, ITypeInfo**) DECLSPEC_HIDDEN;
void release_typelib(void) DECLSPEC_HIDDEN;
void release_desktop_folder(void) DECLSPEC_HIDDEN;

static inline WCHAR *strdupW(const WCHAR *src)
{
    WCHAR *dest;
    if (!src) return NULL;
    dest = heap_alloc((lstrlenW(src) + 1) * sizeof(*dest));
    if (dest)
        lstrcpyW(dest, src);
    return dest;
}

static inline WCHAR *strndupW(const WCHAR *src, DWORD len)
{
    WCHAR *dest;
    if (!src) return NULL;
    dest = heap_alloc((len + 1) * sizeof(*dest));
    if (dest)
    {
        memcpy(dest, src, len * sizeof(WCHAR));
        dest[len] = '\0';
    }
    return dest;
}

static inline WCHAR *strdupAtoW(const char *str)
{
    WCHAR *ret;
    DWORD len;

    if (!str) return NULL;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    ret = heap_alloc(len * sizeof(WCHAR));
    if (ret)
        MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);

    return ret;
}

/* explorer ("cabinet") window messages */
#define CWM_SETPATH             (WM_USER + 2)
#define CWM_WANTIDLE            (WM_USER + 3)
#define CWM_GETSETCURRENTINFO   (WM_USER + 4)
#define CWM_SELECTITEM          (WM_USER + 5)
#define CWM_SELECTITEMSTR       (WM_USER + 6)
#define CWM_GETISHELLBROWSER    (WM_USER + 7)
#define CWM_TESTPATH            (WM_USER + 9)
#define CWM_STATECHANGE         (WM_USER + 10)
#define CWM_GETPATH             (WM_USER + 12)

/* CWM_TESTPATH types */
#define CWTP_ISEQUAL  0
#define CWTP_ISCHILD  1

/* CWM_TESTPATH structure */
typedef struct
{
    DWORD dwType;
    ITEMIDLIST idl;
} CWTESTPATHSTRUCT;

BOOL WINAPI StrRetToStrNA(char *, DWORD, STRRET *, const ITEMIDLIST *);
BOOL WINAPI StrRetToStrNW(WCHAR *, DWORD, STRRET *, const ITEMIDLIST *);

#endif
