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
extern HINSTANCE shell32_hInstance;
extern INT	  shell32_ObjCount;
extern HIMAGELIST ShellSmallIconList;
extern HIMAGELIST ShellBigIconList;

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

/* undocumented WINAPI functions not globaly exported */
LPITEMIDLIST WINAPI ILClone (LPCITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILGetNext(LPITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILCombine(LPCITEMIDLIST iil1,LPCITEMIDLIST iil2);
LPITEMIDLIST WINAPI ILFindLastID(LPITEMIDLIST pidl);
DWORD WINAPI ILGetSize(LPITEMIDLIST pidl);
BOOL WINAPI ILGetDisplayName(LPCITEMIDLIST pidl,LPSTR path);
DWORD WINAPI ILFree(LPITEMIDLIST pidl);

HRESULT WINAPI SHILCreateFromPathA (LPSTR path, LPITEMIDLIST * ppidl, DWORD attributes);
HRESULT WINAPI SHILCreateFromPathW (LPWSTR path, LPITEMIDLIST * ppidl, DWORD attributes);
HRESULT WINAPI SHILCreateFromPathAW (LPVOID path, LPITEMIDLIST * ppidl, DWORD attributes);

LPITEMIDLIST WINAPI ILCreateFromPathA(LPSTR path);
LPITEMIDLIST WINAPI ILCreateFromPathW(LPWSTR path);
LPITEMIDLIST WINAPI ILCreateFromPathAW(LPVOID path);

BOOL WINAPI Shell_GetImageList(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);
HRESULT WINAPI StrRetToStrN (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);

/* Iconcache */
#define INVALID_INDEX -1
BOOL SIC_Initialize(void);
void SIC_Destroy(void);
BOOL PidlToSicIndex (IShellFolder * sh, LPITEMIDLIST pidl, BOOL bBigIcon, UINT * pIndex);

/* Classes Root */
BOOL HCR_MapTypeToValue ( LPCSTR szExtension, LPSTR szFileType, DWORD len);
BOOL HCR_GetExecuteCommand ( LPCSTR szClass, LPCSTR szVerb, LPSTR szDest, DWORD len );
BOOL HCR_GetDefaultIcon (LPCSTR szClass, LPSTR szDest, DWORD len, LPDWORD dwNr);

DWORD 		WINAPI ParseFieldA(LPCSTR src,DWORD field,LPSTR dst,DWORD len);

HGLOBAL	WINAPI SHAllocShared(LPVOID psrc, DWORD size, DWORD procID);
LPVOID		WINAPI SHLockShared(HANDLE hmem, DWORD procID);
BOOL		WINAPI SHUnlockShared(HANDLE pmem);
HANDLE	WINAPI SHFreeShared(HANDLE hmem, DWORD procID);

/****************************************************************************
 * Class constructors
 */
extern LPDATAOBJECT	IDataObject_Constructor(HWND hwndOwner, LPSHELLFOLDER psf, LPITEMIDLIST * apidl, UINT cidl);
extern LPENUMFORMATETC	IEnumFORMATETC_Constructor(UINT, const FORMATETC []);

extern LPCLASSFACTORY IShellLink_CF_Constructor(void);
extern LPCLASSFACTORY IShellLinkW_CF_Constructor(void);

extern LPCLASSFACTORY	IClassFactory_Constructor(void);
extern LPCONTEXTMENU	IContextMenu_Constructor(LPSHELLFOLDER, LPCITEMIDLIST *, UINT);
extern LPSHELLVIEW	IShellView_Constructor(LPSHELLFOLDER, LPCITEMIDLIST);
extern LPSHELLLINK	IShellLink_Constructor(void);
extern LPSHELLLINKW	IShellLinkW_Constructor(void);
extern LPENUMIDLIST	IEnumIDList_Constructor(LPCSTR,DWORD);
extern LPEXTRACTICONA	IExtractIconA_Constructor(LPITEMIDLIST);

/* fixme: rename the functions when the shell32.dll has it's own exports namespace */
HRESULT WINAPI  SHELL32_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv);
HRESULT WINAPI  SHELL32_DllCanUnloadNow(void);

/* elements of this structure are accessed directly from within shell32 */
typedef struct 
{
	ICOM_VTABLE(IShellFolder)*	lpvtbl;
	DWORD				ref;
	ICOM_VTABLE(IPersistFolder)*	lpvtblPersistFolder;

	LPSTR				sMyPath;
	LPITEMIDLIST			pMyPidl;
	LPITEMIDLIST			mpidl;
} IGenericSFImpl;
extern LPSHELLFOLDER	IShellFolder_Constructor(IGenericSFImpl*,LPITEMIDLIST);

#endif
