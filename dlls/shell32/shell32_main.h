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
extern LONG	  shell32_ObjCount;
extern HIMAGELIST	ShellSmallIconList;
extern HIMAGELIST	ShellBigIconList;
extern HDPA		sic_hdpa;

/*******************************************
* pointer to functions dynamically loaded
*/
extern void	(WINAPI *pDLLInitComctl)(LPVOID);

extern LPVOID	(WINAPI *pCOMCTL32_Alloc) (INT);
extern BOOL	(WINAPI *pCOMCTL32_Free) (LPVOID);

extern HDPA	(WINAPI *pDPA_Create) (INT);
extern INT	(WINAPI *pDPA_InsertPtr) (const HDPA, INT, LPVOID);
extern BOOL	(WINAPI *pDPA_Sort) (const HDPA, PFNDPACOMPARE, LPARAM);
extern LPVOID	(WINAPI *pDPA_GetPtr) (const HDPA, INT);
extern BOOL	(WINAPI *pDPA_Destroy) (const HDPA);
extern INT	(WINAPI *pDPA_Search) (const HDPA, LPVOID, INT, PFNDPACOMPARE, LPARAM, UINT);
extern LPVOID	(WINAPI *pDPA_DeletePtr) (const HDPA hdpa, INT i);
extern HANDLE   (WINAPI *pCreateMRUListA) (LPVOID lpcml);
extern DWORD    (WINAPI *pFreeMRUListA) (HANDLE hMRUList);
extern INT      (WINAPI *pAddMRUData) (HANDLE hList, LPCVOID lpData, DWORD cbData);
extern INT      (WINAPI *pFindMRUData) (HANDLE hList, LPCVOID lpData, DWORD cbData, LPINT lpRegNum);
extern INT      (WINAPI *pEnumMRUListA) (HANDLE hList, INT nItemPos, LPVOID lpBuffer, DWORD nBufferSize);
#define pDPA_GetPtrCount(hdpa)  (*(INT*)(hdpa))

/* ole2 */
/*
extern HRESULT (WINAPI *pOleInitialize)(LPVOID reserved);
extern void (WINAPI *pOleUninitialize)(void);
extern HRESULT (WINAPI *pDoDragDrop)(IDataObject* pDataObject, IDropSource * pDropSource, DWORD dwOKEffect, DWORD * pdwEffect);
extern HRESULT (WINAPI *pRegisterDragDrop)(HWND hwnd, IDropTarget* pDropTarget);
extern HRESULT (WINAPI *pRevokeDragDrop)(HWND hwnd);
*/
BOOL WINAPI Shell_GetImageList(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);

/* Iconcache */
#define INVALID_INDEX -1
BOOL SIC_Initialize(void);
void SIC_Destroy(void);
BOOL PidlToSicIndex (IShellFolder * sh, LPITEMIDLIST pidl, BOOL bBigIcon, UINT uFlags, UINT * pIndex);

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
LPSHELLLINK	IShellLink_Constructor(BOOL);

IShellFolder * ISF_Desktop_Constructor(void);

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

/* Clipboard */
void InitShellOle(void);
void FreeShellOle(void);
BOOL GetShellOle(void);

HRESULT (WINAPI *pOleInitialize)(LPVOID reserved);
void    (WINAPI *pOleUninitialize)(void);
HRESULT (WINAPI *pRegisterDragDrop)(HWND hwnd, IDropTarget* pDropTarget);
HRESULT (WINAPI *pRevokeDragDrop)(HWND hwnd);
HRESULT (WINAPI *pDoDragDrop)(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*);
void 	(WINAPI *pReleaseStgMedium)(STGMEDIUM* pmedium);
HRESULT (WINAPI *pOleSetClipboard)(IDataObject* pDataObj);
HRESULT (WINAPI *pOleGetClipboard)(IDataObject** ppDataObj);

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

#endif
