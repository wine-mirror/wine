#ifndef __WINE_UNDOCSHELL_H
#define __WINE_UNDOCSHELL_H

#include "windef.h"
#include "shell.h"
#include "wine/obj_shellfolder.h"	/* strret */

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/****************************************************************************
 *	IDList Functions
 */
LPITEMIDLIST WINAPI ILClone (LPCITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILCloneFirst(LPCITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILCombine(LPCITEMIDLIST iil1,LPCITEMIDLIST iil2);

DWORD WINAPI ILGetSize(LPITEMIDLIST pidl);

LPITEMIDLIST WINAPI ILGetNext(LPITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILFindLastID(LPITEMIDLIST pidl);
BOOL WINAPI ILRemoveLastID(LPCITEMIDLIST pidl);
LPITEMIDLIST WINAPI ILFindChild(LPCITEMIDLIST pidl1,LPCITEMIDLIST pidl2);

BOOL WINAPI ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

BOOL WINAPI ILGetDisplayName(LPCITEMIDLIST pidl,LPSTR path);

DWORD WINAPI ILFree(LPITEMIDLIST pidl);

LPITEMIDLIST WINAPI SHSimpleIDListFromPathA (LPCSTR lpszPath);
LPITEMIDLIST WINAPI SHSimpleIDListFromPathW (LPCWSTR lpszPath);
LPITEMIDLIST WINAPI SHSimpleIDListFromPathAW (LPCVOID lpszPath);

HRESULT WINAPI SHILCreateFromPathA (LPCSTR path, LPITEMIDLIST * ppidl, DWORD *attributes);
HRESULT WINAPI SHILCreateFromPathW (LPCWSTR path, LPITEMIDLIST * ppidl, DWORD *attributes);
HRESULT WINAPI SHILCreateFromPathAW (LPCVOID path, LPITEMIDLIST * ppidl, DWORD *attributes);

LPITEMIDLIST WINAPI ILCreateFromPathA(LPCSTR path);
LPITEMIDLIST WINAPI ILCreateFromPathW(LPCWSTR path);
LPITEMIDLIST WINAPI ILCreateFromPathAW(LPCVOID path);

/*
	string functions
*/
HRESULT WINAPI SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv, LPCITEMIDLIST *ppidlLast);

HRESULT WINAPI StrRetToStrNA (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);
HRESULT WINAPI StrRetToStrNW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);
HRESULT WINAPI StrRetToStrNAW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);

HRESULT WINAPI StrRetToBufA (LPSTRRET src, LPITEMIDLIST pidl, LPSTR dest, DWORD len);
HRESULT WINAPI StrRetToBufW (LPSTRRET src, LPITEMIDLIST pidl, LPWSTR dest, DWORD len);



/****************************************************************************
* SHChangeNotifyRegister API
*/
#define SHCNF_ACCEPT_INTERRUPTS		0x0001
#define SHCNF_ACCEPT_NON_INTERRUPTS	0x0002
#define SHCNF_NO_PROXY			0x8001

typedef struct
{
        LPCITEMIDLIST pidlPath;
        BOOL bWatchSubtree;
} NOTIFYREGISTER, *LPNOTIFYREGISTER;

typedef const LPNOTIFYREGISTER LPCNOTIFYREGISTER;

typedef struct
{
	USHORT	cb;
	DWORD	dwItem1;
	DWORD	dwItem2;
} DWORDITEMID;

HANDLE WINAPI SHChangeNotifyRegister(
    HWND hwnd,
    LONG dwFlags,
    LONG wEventMask,
    DWORD uMsg,
    int cItems,
    LPCNOTIFYREGISTER lpItems);

BOOL WINAPI SHChangeNotifyDeregister( HANDLE hNotify);



#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLOBJ_H */
