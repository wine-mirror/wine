/*
 * 	internal Shell32 Library definitions
 */

#ifndef __WINE_SHELL_MAIN_H
#define __WINE_SHELL_MAIN_H

/*******************************************
*  global SHELL32.DLL variables
*/
extern HINSTANCE32 shell32_hInstance;
extern INT32	  shell32_ObjCount;
extern HIMAGELIST ShellSmallIconList;
extern HIMAGELIST ShellBigIconList;

/*******************************************
* pointer to functions dynamically loaded
*/
extern void	(WINAPI* pDLLInitComctl)(LPVOID);
extern INT32	(WINAPI* pImageList_AddIcon) (HIMAGELIST himl, HICON32 hIcon);
extern INT32	(WINAPI* pImageList_ReplaceIcon) (HIMAGELIST, INT32, HICON32);
extern HIMAGELIST (WINAPI* pImageList_Create) (INT32,INT32,UINT32,INT32,INT32);
extern HICON32	(WINAPI* pImageList_GetIcon) (HIMAGELIST, INT32, UINT32);
extern INT32	(WINAPI* pImageList_GetImageCount)(HIMAGELIST);

extern LPVOID	(WINAPI* pCOMCTL32_Alloc) (INT32);  
extern BOOL32	(WINAPI* pCOMCTL32_Free) (LPVOID);  

extern HDPA	(WINAPI* pDPA_Create) (INT32);  
extern INT32	(WINAPI* pDPA_InsertPtr) (const HDPA, INT32, LPVOID); 
extern BOOL32	(WINAPI* pDPA_Sort) (const HDPA, PFNDPACOMPARE, LPARAM); 
extern LPVOID	(WINAPI* pDPA_GetPtr) (const HDPA, INT32);   
extern BOOL32	(WINAPI* pDPA_Destroy) (const HDPA); 
extern INT32	(WINAPI* pDPA_Search) (const HDPA, LPVOID, INT32, PFNDPACOMPARE, LPARAM, UINT32);

extern HICON32 (WINAPI *pLookupIconIdFromDirectoryEx32)(LPBYTE dir, BOOL32 bIcon, INT32 width, INT32 height, UINT32 cFlag);
extern HICON32 (WINAPI *pCreateIconFromResourceEx32)(LPBYTE bits,UINT32 cbSize, BOOL32 bIcon, DWORD dwVersion, INT32 width, INT32 height,UINT32 cFlag);

/* undocumented WINAPI functions not globaly exported */
LPITEMIDLIST WINAPI ILClone (LPCITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILGetNext(LPITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILCombine(LPCITEMIDLIST iil1,LPCITEMIDLIST iil2);
LPITEMIDLIST WINAPI ILFindLastID(LPITEMIDLIST pidl);
DWORD WINAPI ILGetSize(LPITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILCreateFromPath(LPVOID path);

DWORD WINAPI Shell_GetImageList(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);
HRESULT WINAPI StrRetToStrN (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);

/* Iconcache */
#define INVALID_INDEX -1
BOOL32 SIC_Initialize(void);
/*INT32 SIC_GetIconIndex (LPCSTR sSourceFile, INT32 dwSourceIndex );*/

/* Classes Root */
BOOL32 HCR_MapTypeToValue ( LPCSTR szExtension, LPSTR szFileType, DWORD len);
BOOL32 HCR_GetExecuteCommand ( LPCSTR szClass, LPCSTR szVerb, LPSTR szDest, DWORD len );
BOOL32 HCR_GetDefaultIcon (LPCSTR szClass, LPSTR szDest, DWORD len, LPDWORD dwNr);

DWORD 		WINAPI ParseField32A(LPCSTR src,DWORD field,LPSTR dst,DWORD len);

HGLOBAL32	WINAPI SHAllocShared(LPVOID psrc, DWORD size, DWORD procID);
LPVOID		WINAPI SHLockShared(HANDLE32 hmem, DWORD procID);
BOOL32		WINAPI SHUnlockShared(HANDLE32 pmem);
HANDLE32	WINAPI SHFreeShared(HANDLE32 hmem, DWORD procID);

/****************************************************************************
 * Class constructors
 */
#ifdef __WINE__
extern LPDATAOBJECT	IDataObject_Constructor(HWND32 hwndOwner, LPSHELLFOLDER psf, LPITEMIDLIST * apidl, UINT32 cidl);
extern LPENUMFORMATETC	IEnumFORMATETC_Constructor(UINT32, const FORMATETC32 []);

extern LPCLASSFACTORY IShellLink_CF_Constructor(void);
extern LPCLASSFACTORY IShellLinkW_CF_Constructor(void);

extern LPCLASSFACTORY	IClassFactory_Constructor(void);
extern LPCONTEXTMENU	IContextMenu_Constructor(LPSHELLFOLDER, LPCITEMIDLIST *, UINT32);
extern LPSHELLFOLDER	IShellFolder_Constructor(LPSHELLFOLDER,LPITEMIDLIST);
extern LPSHELLVIEW	IShellView_Constructor(LPSHELLFOLDER, LPCITEMIDLIST);
extern LPSHELLLINK	IShellLink_Constructor(void);
extern LPSHELLLINKW	IShellLinkW_Constructor(void);
extern LPENUMIDLIST	IEnumIDList_Constructor(LPCSTR,DWORD);
extern LPEXTRACTICON	IExtractIcon_Constructor(LPITEMIDLIST);
#endif


/* FIXME should be moved to a header file. IsEqualGUID 
is declared but not exported in compobj.c !!!*/
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#endif
