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

LPITEMIDLIST WINAPI SHSimpleIDListFromPathA (LPSTR lpszPath);
LPITEMIDLIST WINAPI SHSimpleIDListFromPathW (LPWSTR lpszPath);
LPITEMIDLIST WINAPI SHSimpleIDListFromPathAW (LPVOID lpszPath);

HRESULT WINAPI SHILCreateFromPathA (LPSTR path, LPITEMIDLIST * ppidl, DWORD attributes);
HRESULT WINAPI SHILCreateFromPathW (LPWSTR path, LPITEMIDLIST * ppidl, DWORD attributes);
HRESULT WINAPI SHILCreateFromPathAW (LPVOID path, LPITEMIDLIST * ppidl, DWORD attributes);

LPITEMIDLIST WINAPI ILCreateFromPathA(LPSTR path);
LPITEMIDLIST WINAPI ILCreateFromPathW(LPWSTR path);
LPITEMIDLIST WINAPI ILCreateFromPathAW(LPVOID path);

/*
	string functions
*/
HRESULT WINAPI SHBindToParent(LPCITEMIDLIST pidl, REFIID riid, LPVOID *ppv, LPCITEMIDLIST *ppidlLast);

HRESULT WINAPI StrRetToStrNA (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);
HRESULT WINAPI StrRetToStrNW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);
HRESULT WINAPI StrRetToStrNAW (LPVOID dest, DWORD len, LPSTRRET src, LPITEMIDLIST pidl);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLOBJ_H */
