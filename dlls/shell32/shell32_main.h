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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_SHELL_MAIN_H
#define __WINE_SHELL_MAIN_H

#include "commctrl.h"
#include "docobj.h"
#include "undocshell.h"
#include "wine/obj_shellfolder.h"
#include "wine/obj_dataobject.h"
#include "wine/obj_contextmenu.h"
#include "wine/obj_shellview.h"
#include "wine/obj_shelllink.h"
#include "wine/obj_extracticon.h"
#include "wine/windef16.h"

/*******************************************
*  global SHELL32.DLL variables
*/
extern HMODULE	huser32;
extern HINSTANCE shell32_hInstance;
extern HIMAGELIST	ShellSmallIconList;
extern HIMAGELIST	ShellBigIconList;
extern HDPA		sic_hdpa;

/*******************************************
* pointer to functions dynamically loaded
*/
extern void	(WINAPI *pDLLInitComctl)(LPVOID);

BOOL WINAPI Shell_GetImageList(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);

/* Iconcache */
#define INVALID_INDEX -1
BOOL SIC_Initialize(void);
void SIC_Destroy(void);
BOOL PidlToSicIndex (IShellFolder * sh, LPITEMIDLIST pidl, BOOL bBigIcon, UINT uFlags, UINT * pIndex);
INT SIC_GetIconIndex (LPCSTR sSourceFile, INT dwSourceIndex );

/* Classes Root */
BOOL HCR_MapTypeToValue ( LPCSTR szExtension, LPSTR szFileType, DWORD len, BOOL bPrependDot);
BOOL HCR_GetExecuteCommand ( LPCSTR szClass, LPCSTR szVerb, LPSTR szDest, DWORD len );
BOOL HCR_GetDefaultIcon (LPCSTR szClass, LPSTR szDest, DWORD len, LPDWORD dwNr);
BOOL HCR_GetClassName (REFIID riid, LPSTR szDest, DWORD len);
BOOL HCR_GetFolderAttributes (REFIID riid, LPDWORD szDest);

BOOL  WINAPI AboutDlgProc(HWND,UINT,WPARAM,LPARAM);
DWORD WINAPI ParseFieldA(LPCSTR src,DWORD field,LPSTR dst,DWORD len);

/****************************************************************************
 * Class constructors
 */
LPDATAOBJECT	IDataObject_Constructor(HWND hwndOwner, LPITEMIDLIST myPidl, LPITEMIDLIST * apidl, UINT cidl);
LPENUMFORMATETC	IEnumFORMATETC_Constructor(UINT, const FORMATETC []);

LPCLASSFACTORY	IClassFactory_Constructor(REFCLSID);
IContextMenu *	ISvItemCm_Constructor(LPSHELLFOLDER pSFParent, LPCITEMIDLIST pidl, LPCITEMIDLIST *aPidls, UINT uItemCount);
IContextMenu *	ISvBgCm_Constructor(LPSHELLFOLDER pSFParent);
LPSHELLVIEW	IShellView_Constructor(LPSHELLFOLDER);

HRESULT WINAPI IFSFolder_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IShellLink_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_Desktop_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI ISF_MyComputer_Constructor(IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);
HRESULT WINAPI IDropTargetHelper_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv);

/* kind of enumidlist */
#define EIDL_DESK	0
#define EIDL_MYCOMP	1
#define EIDL_FILE	2

LPENUMIDLIST	IEnumIDList_Constructor(LPCSTR,DWORD,DWORD);

LPEXTRACTICONA	IExtractIconA_Constructor(LPITEMIDLIST);
HRESULT		CreateStreamOnFile (LPCSTR pszFilename, IStream ** ppstm);

/* FIXME: rename the functions when the shell32.dll has it's own exports namespace */
HRESULT WINAPI  SHELL32_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
HRESULT WINAPI  SHELL32_DllCanUnloadNow(void);

/* FIXME: move away */
#define ResultFromShort(i) MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(i))

/* menu merging */
#define MM_ADDSEPARATOR         0x00000001L
#define MM_SUBMENUSHAVEIDS      0x00000002L
HRESULT WINAPI Shell_MergeMenus (HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags);

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
	(((kst) & MK_CONTROL) ?\
	(((kst) & MK_SHIFT) ? DROPEFFECT_LINK : DROPEFFECT_COPY):\
	DROPEFFECT_MOVE)

/* Systray */
BOOL SYSTRAY_Init(void);

/* OLE32 */
extern HINSTANCE hShellOle32;

extern HRESULT (WINAPI *pOleInitialize)(LPVOID reserved);
extern void    (WINAPI *pOleUninitialize)(void);
extern HRESULT (WINAPI *pRegisterDragDrop)(HWND hwnd, IDropTarget* pDropTarget);
extern HRESULT (WINAPI *pRevokeDragDrop)(HWND hwnd);
extern HRESULT (WINAPI *pDoDragDrop)(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);
extern void    (WINAPI *pReleaseStgMedium)(STGMEDIUM* pmedium);
extern HRESULT (WINAPI *pOleSetClipboard)(IDataObject* pDataObj);
extern HRESULT (WINAPI *pOleGetClipboard)(IDataObject** ppDataObj);
extern HRESULT (WINAPI *pCoCreateInstance)(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID iid, LPVOID *ppv);

BOOL GetShellOle(void);

HGLOBAL RenderHDROP(LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderSHELLIDLIST (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderSHELLIDLISTOFFSET (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILECONTENTS (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILEDESCRIPTOR (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderFILENAME (LPITEMIDLIST pidlRoot, LPITEMIDLIST * apidl, UINT cidl);
HGLOBAL RenderPREFEREDDROPEFFECT (DWORD dwFlags);

/* Change Notification */
void InitChangeNotifications(void);
void FreeChangeNotifications(void);

/* file operation */
#define ASK_DELETE_FILE		 1
#define ASK_DELETE_FOLDER	 2
#define ASK_DELETE_MULTIPLE_ITEM 3

BOOL SHELL_DeleteDirectoryA(LPCSTR pszDir, BOOL bShowUI);
BOOL SHELL_DeleteFileA(LPCSTR pszFile, BOOL bShowUI);
BOOL SHELL_WarnItemDelete(int nKindOfDialog, LPCSTR szDir);

/* 16-bit functions */
void        WINAPI DragAcceptFiles16(HWND16 hWnd, BOOL16 b);
UINT16      WINAPI DragQueryFile16(HDROP16 hDrop, WORD wFile, LPSTR lpszFile, WORD wLength);
void        WINAPI DragFinish16(HDROP16 h);
BOOL16      WINAPI DragQueryPoint16(HDROP16 hDrop, POINT16 *p);
HINSTANCE16 WINAPI ShellExecute16(HWND16,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT16);
HICON16     WINAPI ExtractIcon16(HINSTANCE16,LPCSTR,UINT16);
HICON16     WINAPI ExtractAssociatedIcon16(HINSTANCE16,LPSTR,LPWORD);
HICON16     WINAPI ExtractIconEx16 ( LPCSTR, INT16, HICON16 *, HICON16 *, UINT16 );
HINSTANCE16 WINAPI FindExecutable16(LPCSTR,LPCSTR,LPSTR);
HGLOBAL16   WINAPI InternalExtractIcon16(HINSTANCE16,LPCSTR,UINT16,WORD);
BOOL16      WINAPI ShellAbout16(HWND16,LPCSTR,LPCSTR,HICON16);
BOOL16      WINAPI AboutDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);

inline static BOOL SHELL_OsIsUnicode(void)
{
    /* if high-bit of version is 0, we are emulating NT */
    return !(GetVersion() & 0x80000000);
}

#define __SHFreeAndNil(ptr) \
	{\
	  SHFree(*ptr); \
	  *ptr = NULL; \
	};
inline static void __SHCloneStrA(char ** target,const char * source)
{
	*target = SHAlloc(strlen(source)+1); \
	strcpy(*target, source); \
}

inline static void __SHCloneStrWtoA(char ** target, const WCHAR * source)
{
	int len = WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL);
	*target = SHAlloc(len);
	WideCharToMultiByte(CP_ACP, 0, source, -1, *target, len, NULL, NULL);
}

/* handle conversions */
#define HICON_16(h32)		(LOWORD(h32))
#define HICON_32(h16)		((HICON)(ULONG_PTR)(h16))
#define HINSTANCE_32(h16)	((HINSTANCE)(ULONG_PTR)(h16))

typedef HINSTANCE (*SHELL_ExecuteA1632)(char *lpCmd, LPSHELLEXECUTEINFOA sei, BOOL shWait);
BOOL WINAPI ShellExecuteExA32 (LPSHELLEXECUTEINFOA sei, SHELL_ExecuteA1632 execfunc);

#endif
