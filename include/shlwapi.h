#ifndef __WINE_SHLWAPI_H
#define __WINE_SHLWAPI_H

#include "objbase.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

BOOL WINAPI PathAppendA(LPSTR lpszPath1,LPCSTR lpszPath2);
BOOL WINAPI PathAppendW(LPWSTR lpszPath1,LPCWSTR lpszPath2);

LPSTR WINAPI PathBuildRootA(LPSTR lpszPath, int drive);
LPWSTR WINAPI PathBuildRootW(LPWSTR lpszPath, int drive);

LPSTR WINAPI PathGetArgsA(LPCSTR lpszPath);
LPWSTR WINAPI PathGetArgsW(LPCWSTR lpszPath);

BOOL WINAPI PathRemoveFileSpecA(LPSTR lpszPath);
BOOL WINAPI PathRemoveFileSpecW(LPWSTR lpszPath);

void WINAPI PathStripPathA(LPSTR lpszPath);
void WINAPI PathStripPathW(LPWSTR lpszPath);

void WINAPI PathRemoveArgsA(LPSTR lpszPath);
void WINAPI PathRemoveArgsW(LPWSTR lpszPath);

void WINAPI PathRemoveExtensionA(LPSTR lpszPath);
void WINAPI PathRemoveExtensionW(LPWSTR lpszPath);

void WINAPI PathUnquoteSpacesA(LPSTR str);
void WINAPI PathUnquoteSpacesW(LPWSTR str);

int WINAPI PathParseIconLocationA(LPSTR lpszPath);
int WINAPI PathParseIconLocationW(LPWSTR lpszPath);

BOOL WINAPI PathIsDirectoryA(LPCSTR lpszPath);
BOOL WINAPI PathIsDirectoryW(LPCWSTR lpszPath);

BOOL WINAPI PathFileExistsA(LPCSTR lpszPath);
BOOL WINAPI PathFileExistsW(LPCWSTR lpszPath);

BOOL WINAPI PathIsSameRootA(LPCSTR lpszPath1, LPCSTR lpszPath2);
BOOL WINAPI PathIsSameRootW(LPCWSTR lpszPath1, LPCWSTR lpszPath2);

BOOL WINAPI PathSetDlgItemPathA(HWND hDlg, int id, LPCSTR pszPath);
BOOL WINAPI PathSetDlgItemPathW(HWND hDlg, int id, LPCWSTR pszPath);

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

INT WINAPI StrCSpnA(LPCSTR lpStr, LPCSTR lpSet);
INT WINAPI StrCSpnW(LPCWSTR lpStr, LPCWSTR lpSet);
#define StrCSpn WINELIB_NAME_AW(StrCSpn)

INT WINAPI StrCSpnIA(LPCSTR lpStr, LPCSTR lpSet);
INT WINAPI StrCSpnIW(LPCWSTR lpStr, LPCWSTR lpSet);
#define StrCSpnI WINELIB_NAME_AW(StrCSpnI)

#define StrCatA lstrcatA
LPWSTR WINAPI StrCatW(LPWSTR front, LPCWSTR back);
#define StrCat WINELIB_NAME_AW(StrCat)

LPSTR WINAPI StrCatBuffA(LPSTR front, LPCSTR back, INT size);
LPWSTR WINAPI StrCatBuffW(LPWSTR front, LPCWSTR back, INT size);
#define StrCatBuff WINELIB_NAME_AW(StrCatBuff)

LPSTR WINAPI StrChrA(LPCSTR lpStart, WORD wMatch);
LPWSTR WINAPI StrChrW(LPCWSTR lpStart, WCHAR wMatch); 
#define StrChr WINELIB_NAME_AW(StrChr)

LPSTR WINAPI StrChrIA(LPCSTR lpStart, WORD wMatch);
LPWSTR WINAPI StrChrIW(LPCWSTR lpStart, WCHAR wMatch); 
#define StrChrI WINELIB_NAME_AW(StrChrI)

INT WINAPI StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, INT nChar);
INT WINAPI StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, INT nChar);
#define StrCmpN WINELIB_NAME_AW(StrCmpN)

INT WINAPI StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, INT nChar);
INT WINAPI StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, INT nChar);
#define StrCmpNI WINELIB_NAME_AW(StrCmpNI)

LPSTR WINAPI StrDupA(LPCSTR lpSrc);
LPWSTR WINAPI StrDupW(LPCWSTR lpSrc);
#define StrDup WINELIB_NAME_AW(StrDup)

struct _STRRET;
struct _ITEMIDLIST;
HRESULT WINAPI StrRetToBufA(struct _STRRET *src, const struct _ITEMIDLIST *pidl, LPSTR dest, DWORD len);
HRESULT WINAPI StrRetToBufW(struct _STRRET *src, const struct _ITEMIDLIST *pidl, LPWSTR dest, DWORD len);
#define StrRetToBuf WINELIB_NAME_AW(StrRetToBuf)

void WINAPI PathRemoveBlanksA(LPSTR lpszPath);
void WINAPI PathRemoveBlanksW(LPWSTR lpszPath);
#define  PathRemoveBlanks WINELIB_NAME_AW(PathRemoveBlanks)
void WINAPI PathRemoveBlanksAW(LPVOID lpszPath);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLWAPI_H */
