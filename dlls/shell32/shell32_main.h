/*
 * 	internal Shell32 Library definitions
 */

#ifndef __WINE_SHELL_MAIN_H
#define __WINE_SHELL_MAIN_H

/*******************************************
*  global SHELL32.DLL variables
*/
extern HINSTANCE32 shell32_hInstance;
extern UINT32      shell32_DllRefCount;
extern HIMAGELIST ShellSmallIconList;
extern HIMAGELIST ShellBigIconList;

/*******************************************
* pointer to functions dynamically loaded
*/
extern void	(CALLBACK* pDLLInitComctl)(void);
extern INT32	(CALLBACK* pImageList_AddIcon) (HIMAGELIST himl, HICON32 hIcon);
extern INT32	(CALLBACK* pImageList_ReplaceIcon) (HIMAGELIST, INT32, HICON32);
extern HIMAGELIST (CALLBACK * pImageList_Create) (INT32,INT32,UINT32,INT32,INT32);
extern HICON32	(CALLBACK * pImageList_GetIcon) (HIMAGELIST, INT32, UINT32);
extern INT32	(CALLBACK* pImageList_GetImageCount)(HIMAGELIST);

extern LPVOID	(CALLBACK* pCOMCTL32_Alloc) (INT32);  
extern BOOL32	(CALLBACK* pCOMCTL32_Free) (LPVOID);  

extern HDPA	(CALLBACK* pDPA_Create) (INT32);  
extern INT32	(CALLBACK* pDPA_InsertPtr) (const HDPA, INT32, LPVOID); 
extern BOOL32	(CALLBACK* pDPA_Sort) (const HDPA, PFNDPACOMPARE, LPARAM); 
extern LPVOID	(CALLBACK* pDPA_GetPtr) (const HDPA, INT32);   
extern BOOL32	(CALLBACK* pDPA_Destroy) (const HDPA); 
extern INT32	(CALLBACK *pDPA_Search) (const HDPA, LPVOID, INT32, PFNDPACOMPARE, LPARAM, UINT32);

/* Iconcache */
extern BOOL32	WINAPI SIC_Initialize(void);
extern HICON32	WINAPI SIC_GetIcon (LPSTR sSourceFile, DWORD dwSourceIndex, BOOL32 bSmallIcon );

/* Classes Root */
extern BOOL32 WINAPI HCR_MapTypeToValue ( LPSTR szExtension, LPSTR szFileType, DWORD len);
extern BOOL32 WINAPI HCR_GetExecuteCommand ( LPCSTR szClass, LPCSTR szVerb, LPSTR szDest, DWORD len );

HGLOBAL32	WINAPI SHAllocShared(LPVOID psrc, DWORD size, DWORD procID);
LPVOID		WINAPI SHLockShared(HANDLE32 hmem, DWORD procID);
BOOL32		WINAPI SHUnlockShared(HANDLE32 pmem);
HANDLE32	WINAPI SHFreeShared(HANDLE32 hmem, DWORD procID);

/* FIXME should be moved to a header file. IsEqualGUID 
is declared but not exported in compobj.c !!!*/
#define IsEqualGUID(rguid1, rguid2) (!memcmp(rguid1, rguid2, sizeof(GUID)))
#define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
#define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

#endif
