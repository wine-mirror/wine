/*
 * 	internal Shell32 Library definitions
 */

#ifndef __WINE_SHELL_MAIN_H
#define __WINE_SHELL_MAIN_H

#include "imagelist.h"
#include "commctrl.h"
#include "shell.h"
#include "docobj.h"

#include "wine/obj_shellfolder.h"
#include "wine/obj_dataobject.h"
#include "wine/obj_contextmenu.h"
#include "wine/obj_shellview.h"
#include "wine/obj_shelllink.h"
#include "wine/obj_extracticon.h"

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
extern void	(WINAPI* pDLLInitComctl)(LPVOID);
extern INT	(WINAPI* pImageList_AddIcon) (HIMAGELIST himl, HICON hIcon);
extern INT	(WINAPI* pImageList_ReplaceIcon) (HIMAGELIST, INT, HICON);
extern HIMAGELIST (WINAPI* pImageList_Create) (INT,INT,UINT,INT,INT);
extern BOOL	(WINAPI* pImageList_Draw) (HIMAGELIST himl, int i, HDC hdcDest, int x, int y, UINT fStyle);
extern HICON	(WINAPI* pImageList_GetIcon) (HIMAGELIST, INT, UINT);
extern INT	(WINAPI* pImageList_GetImageCount)(HIMAGELIST);
extern COLORREF (WINAPI *pImageList_SetBkColor)(HIMAGELIST, COLORREF);

extern LPVOID	(WINAPI* pCOMCTL32_Alloc) (INT);  
extern BOOL	(WINAPI* pCOMCTL32_Free) (LPVOID);  

extern HDPA	(WINAPI* pDPA_Create) (INT);  
extern INT	(WINAPI* pDPA_InsertPtr) (const HDPA, INT, LPVOID); 
extern BOOL	(WINAPI* pDPA_Sort) (const HDPA, PFNDPACOMPARE, LPARAM); 
extern LPVOID	(WINAPI* pDPA_GetPtr) (const HDPA, INT);   
extern BOOL	(WINAPI* pDPA_Destroy) (const HDPA); 
extern INT	(WINAPI* pDPA_Search) (const HDPA, LPVOID, INT, PFNDPACOMPARE, LPARAM, UINT);
extern LPVOID	(WINAPI* pDPA_DeletePtr) (const HDPA hdpa, INT i);
#define pDPA_GetPtrCount(hdpa)  (*(INT*)(hdpa))   

extern HICON (WINAPI *pLookupIconIdFromDirectoryEx)(LPBYTE dir, BOOL bIcon, INT width, INT height, UINT cFlag);
extern HICON (WINAPI *pCreateIconFromResourceEx)(LPBYTE bits,UINT cbSize, BOOL bIcon, DWORD dwVersion, INT width, INT height,UINT cFlag);

/* ole2 */
/*
extern HRESULT (WINAPI* pOleInitialize)(LPVOID reserved);
extern void (WINAPI* pOleUninitialize)(void);
extern HRESULT (WINAPI* pDoDragDrop)(IDataObject* pDataObject, IDropSource * pDropSource, DWORD dwOKEffect, DWORD * pdwEffect);
extern HRESULT (WINAPI* pRegisterDragDrop)(HWND hwnd, IDropTarget* pDropTarget);
extern HRESULT (WINAPI* pRevokeDragDrop)(HWND hwnd);
*/
BOOL WINAPI Shell_GetImageList(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);

HRESULT WINAPI StrRetToStrNA (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);
HRESULT WINAPI StrRetToStrNW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);
HRESULT WINAPI StrRetToStrN (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);

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

DWORD 	WINAPI ParseFieldA(LPCSTR src,DWORD field,LPSTR dst,DWORD len);

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

/* fixme: rename the functions when the shell32.dll has it's own exports namespace */
HRESULT WINAPI  SHELL32_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
HRESULT WINAPI  SHELL32_DllCanUnloadNow(void);

/* fixme: move away */
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

HRESULT (WINAPI* pOleInitialize)(LPVOID reserved);
void    (WINAPI* pOleUninitialize)(void);
HRESULT (WINAPI* pRegisterDragDrop)(HWND hwnd, IDropTarget* pDropTarget);
HRESULT (WINAPI* pRevokeDragDrop)(HWND hwnd);
HRESULT (WINAPI* pDoDragDrop)(LPDATAOBJECT,LPDROPSOURCE,DWORD,DWORD*); 
void 	(WINAPI* pReleaseStgMedium)(STGMEDIUM* pmedium);
HRESULT (WINAPI* pOleSetClipboard)(IDataObject* pDataObj);
HRESULT (WINAPI* pOleGetClipboard)(IDataObject** ppDataObj);

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
BOOL SHELL_DeleteDirectoryA(LPCSTR pszDir, BOOL bShowUI);
#endif
