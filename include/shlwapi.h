#ifndef __WINE_SHLWAPI_H
#define __WINE_SHLWAPI_H

#include "windef.h"
#include "wine/obj_queryassociations.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

LPSTR WINAPI PathFindFileNameA(LPCSTR pPath);
LPWSTR WINAPI PathFindFileNameW(LPCWSTR pPath);
#define PathFindFileName WINELIB_NAME_AW(PathFindFileName)
LPVOID WINAPI PathFindFileNameAW(LPCVOID path); 

int WINAPI PathGetDriveNumberA(LPCSTR lpszPath);
int WINAPI PathGetDriveNumberW(LPCWSTR lpszPath);
#define PathGetDriveNumber WINELIB_NAME_AW(PathGetDriveNumber)

BOOL WINAPI PathCanonicalizeA(LPSTR lpszDst, LPCSTR lpszSrc);
BOOL WINAPI PathCanonicalizeW(LPWSTR lpszDst, LPCWSTR lpszSrc);
#define PathCanonicalize WINELIB_NAME_AW(PathCanonicalize)

LPSTR WINAPI PathFindNextComponentA(LPCSTR pszPath);
LPWSTR WINAPI PathFindNextComponentW(LPCWSTR pszPath);
#define PathFindNextComponent WINELIB_NAME_AW(PathFindNextComponent)

BOOL WINAPI PathIsURLA(LPCSTR pszPath);
BOOL WINAPI PathIsURLW(LPCWSTR pszPath);
#define PathIsURL WINELIB_NAME_AW(PathIsURL)

BOOL WINAPI PathAddExtensionA(LPSTR pszPath, LPCSTR pszExt);
BOOL WINAPI PathAddExtensionW(LPWSTR pszPath, LPCWSTR pszExt);
#define PathAddExtension WINELIB_NAME_AW(PathAddExtension)

BOOL WINAPI PathStripToRootA(LPSTR pszPath);
BOOL WINAPI PathStripToRootW(LPWSTR pszPath);
#define PathStripToRoot WINELIB_NAME_AW(PathStripToRoot)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLWAPI_H */
