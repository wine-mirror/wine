#ifndef __WINE_SHLWAPI_H
#define __WINE_SHLWAPI_H

#include "objbase.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */


/* Mask returned by GetPathCharType */
#define GCT_INVALID	0x0000
#define GCT_LFNCHAR	0x0001
#define GCT_SHORTCHAR	0x0002
#define GCT_WILD	0x0004
#define GCT_SEPARATOR	0x0008

/*
 * The URL_ defines were determined by trial and error.  If they become
 * documented please check them and add the missing ones including:
 *
 * URL_ESCAPE_PERCENT
 * URL_PLUGGABLE_PROTOCOL
 * URL_DONT_ESCAPE_EXTRA_INFO
 * URL_ESCAPE_SEGMENT_ONLY
 */

#define URL_UNESCAPE_INPLACE         0x00100000
#define URL_DONT_UNESCAPE_EXTRA_INFO 0x02000000
 
#define URL_ESCAPE_SPACES_ONLY       0x04000000

#define URL_UNESCAPE                 0x10000000
#define URL_ESCAPE_UNSAFE            0x20000000
#define URL_DONT_SIMPLIFY            0x40000000

LPSTR  WINAPI PathAddBackslashA(LPSTR path);	
LPWSTR WINAPI PathAddBackslashW(LPWSTR path);	
#define PathAddBackslash WINELIB_NAME_AW(PathAddBackslash)

BOOL WINAPI PathAddExtensionA(LPSTR pszPath, LPCSTR pszExt);
BOOL WINAPI PathAddExtensionW(LPWSTR pszPath, LPCWSTR pszExt);
#define PathAddExtension WINELIB_NAME_AW(PathAddExtension)

BOOL WINAPI PathAppendA(LPSTR lpszPath1,LPCSTR lpszPath2);
BOOL WINAPI PathAppendW(LPWSTR lpszPath1,LPCWSTR lpszPath2);
#define PathAppend WINELIB_NAME_AW(PathAppend)

LPSTR WINAPI PathBuildRootA(LPSTR lpszPath, int drive);
LPWSTR WINAPI PathBuildRootW(LPWSTR lpszPath, int drive);
#define PathBuildRoot WINELIB_NAME_AW(PathBuiltRoot)

BOOL WINAPI PathCanonicalizeA(LPSTR lpszDst, LPCSTR lpszSrc);
BOOL WINAPI PathCanonicalizeW(LPWSTR lpszDst, LPCWSTR lpszSrc);
#define PathCanonicalize WINELIB_NAME_AW(PathCanonicalize)

LPSTR  WINAPI PathCombineA(LPSTR szDest, LPCSTR lpszDir, LPCSTR lpszFile);
LPWSTR WINAPI PathCombineW(LPWSTR szDest, LPCWSTR lpszDir, LPCWSTR lpszFile);
#define PathCombine WINELIB_NAME_AW(PathCombine)

BOOL WINAPI PathFileExistsA(LPCSTR lpszPath);
BOOL WINAPI PathFileExistsW(LPCWSTR lpszPath);
#define PathFileExists WINELIB_NAME_AW(PathFileExists)

LPSTR WINAPI PathFindExtensionA(LPCSTR path);
LPWSTR WINAPI PathFindExtensionW(LPCWSTR path);
#define PathFindExtension WINELIB_NAME_AW(PathFindExtension)

LPSTR WINAPI PathFindFileNameA(LPCSTR pPath);
LPWSTR WINAPI PathFindFileNameW(LPCWSTR pPath);
#define PathFindFileName WINELIB_NAME_AW(PathFindFileName)

LPSTR WINAPI PathFindNextComponentA(LPCSTR pszPath);
LPWSTR WINAPI PathFindNextComponentW(LPCWSTR pszPath);
#define PathFindNextComponent WINELIB_NAME_AW(PathFindNextComponent)

BOOL WINAPI PathFindOnPathA(LPSTR sFile, LPCSTR sOtherDirs);
BOOL WINAPI PathFindOnPathW(LPWSTR sFile, LPCWSTR sOtherDirs);
#define PathFindOnPath WINELIB_NAME_AW(PathFindOnPath)

LPSTR WINAPI PathGetArgsA(LPCSTR lpszPath);
LPWSTR WINAPI PathGetArgsW(LPCWSTR lpszPath);
#define PathGetArgs WINELIB_NAME_AW(PathGetArgs)

int WINAPI PathGetDriveNumberA(LPCSTR lpszPath);
int WINAPI PathGetDriveNumberW(LPCWSTR lpszPath);
#define PathGetDriveNumber WINELIB_NAME_AW(PathGetDriveNumber)

BOOL WINAPI PathIsDirectoryA(LPCSTR lpszPath);
BOOL WINAPI PathIsDirectoryW(LPCWSTR lpszPath);
#define PathIsDirectory WINELIB_NAME_AW(PathIsDirectory)

BOOL WINAPI PathIsRelativeA(LPCSTR lpszPath);
BOOL WINAPI PathIsRelativeW(LPCWSTR lpszPath);
#define PathIsRelative WINELIB_NAME_AW(PathIsRelative)

BOOL WINAPI PathIsRootA(LPCSTR x);
BOOL WINAPI PathIsRootW(LPCWSTR x);
#define PathIsRoot WINELIB_NAME_AW(PathIsRoot)

BOOL WINAPI PathIsSameRootA(LPCSTR lpszPath1, LPCSTR lpszPath2);
BOOL WINAPI PathIsSameRootW(LPCWSTR lpszPath1, LPCWSTR lpszPath2);
#define PathIsSameRoot WINELIB_NAME_AW(PathIsSameRoot)

BOOL WINAPI PathIsUNCA(LPCSTR lpszPath);
BOOL WINAPI PathIsUNCW(LPCWSTR lpszPath);
#define PathIsUNC WINELIB_NAME_AW(PathIsUNC)

BOOL WINAPI PathIsURLA(LPCSTR pszPath);
BOOL WINAPI PathIsURLW(LPCWSTR pszPath);
#define PathIsURL WINELIB_NAME_AW(PathIsURL)

BOOL WINAPI PathMatchSpecA(LPCSTR lpszPath, LPCSTR lpszSpec);
BOOL WINAPI PathMatchSpecW(LPCWSTR lpszPath, LPCWSTR lpszSpec);
#define PathMatchSpec WINELIB_NAME_AW(PathMatchSpec)

int WINAPI PathParseIconLocationA(LPSTR lpszPath);
int WINAPI PathParseIconLocationW(LPWSTR lpszPath);
#define PathParseIconLocation WINELIB_NAME_AW(PathParseIconLocation)

LPSTR  WINAPI PathQuoteSpacesA(LPSTR path);	
LPWSTR WINAPI PathQuoteSpacesW(LPWSTR path);	
#define PathQuoteSpaces WINELIB_NAME_AW(PathQuoteSpaces)

void WINAPI PathRemoveArgsA(LPSTR lpszPath);
void WINAPI PathRemoveArgsW(LPWSTR lpszPath);
#define PathRemoveArgs WINELIB_NAME_AW(PathRemoveArgs)

LPSTR WINAPI PathRemoveBackslashA(LPSTR lpszPath);
LPWSTR WINAPI PathRemoveBackslashW(LPWSTR lpszPath);
#define PathRemoveBackslash WINELIB_NAME_AW(PathRemoveBackslash)

void WINAPI PathRemoveBlanksA(LPSTR lpszPath);
void WINAPI PathRemoveBlanksW(LPWSTR lpszPath);
#define PathRemoveBlanks WINELIB_NAME_AW(PathRemoveBlanks)

void WINAPI PathRemoveExtensionA(LPSTR lpszPath);
void WINAPI PathRemoveExtensionW(LPWSTR lpszPath);
#define PathRemoveExtension WINELIB_NAME_AW(PathRemoveExtension)

BOOL WINAPI PathRemoveFileSpecA(LPSTR lpszPath);
BOOL WINAPI PathRemoveFileSpecW(LPWSTR lpszPath);
#define PathRemoveFileSpec WINELIB_NAME_AW(PathRemoveFileSpec)

BOOL WINAPI PathSetDlgItemPathA(HWND hDlg, int id, LPCSTR pszPath);
BOOL WINAPI PathSetDlgItemPathW(HWND hDlg, int id, LPCWSTR pszPath);
#define PathSetDlgItemPath WINELIB_NAME_AW(PathSetDlgItemPath)

void WINAPI PathStripPathA(LPSTR lpszPath);
void WINAPI PathStripPathW(LPWSTR lpszPath);
#define PathStripPath WINELIB_NAME_AW(PathStripPath)

BOOL WINAPI PathStripToRootA(LPSTR pszPath);
BOOL WINAPI PathStripToRootW(LPWSTR pszPath);
#define PathStripToRoot WINELIB_NAME_AW(PathStripToRoot)

void WINAPI PathUnquoteSpacesA(LPSTR str);
void WINAPI PathUnquoteSpacesW(LPWSTR str);
#define PathUnquoteSpaces WINELIB_NAME_AW(PathUnquoteSpaces)


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

LPSTR WINAPI StrFormatByteSizeA ( DWORD dw, LPSTR pszBuf, UINT cchBuf );
LPWSTR WINAPI StrFormatByteSizeW ( DWORD dw, LPWSTR pszBuf, UINT cchBuf );
#define StrFormatByteSize WINELIB_NAME_AW(StrFormatByteSize)

INT WINAPI wvnsprintfA(LPSTR lpOut, INT cchLimitIn, LPCSTR lpFmt, va_list arglist);
INT WINAPI wvnsprintfW(LPWSTR lpOut, INT cchLimitIn, LPCWSTR lpFmt, va_list arglist);
#define wvnsprintf WINELIB_NAME_AW(wvnsprintf)

INT WINAPIV wnsprintfA(LPSTR lpOut, INT cchLimitIn, LPCSTR lpFmt, ...);
INT WINAPIV wnsprintfW(LPWSTR lpOut, INT cchLimitIn, LPCWSTR lpFmt, ...);
#define wnsprintf WINELIB_NAME_AW(wnsprintf)


struct _STRRET;
struct _ITEMIDLIST;
HRESULT WINAPI StrRetToBufA(struct _STRRET *src, const struct _ITEMIDLIST *pidl, LPSTR dest, DWORD len);
HRESULT WINAPI StrRetToBufW(struct _STRRET *src, const struct _ITEMIDLIST *pidl, LPWSTR dest, DWORD len);
#define StrRetToBuf WINELIB_NAME_AW(StrRetToBuf)

HRESULT WINAPI SHDeleteKeyA(HKEY hKey, LPCSTR lpszSubKey);
HRESULT WINAPI SHDeleteKeyW(HKEY hkey, LPCWSTR pszSubKey);
#define  SHDeleteKey WINELIB_NAME_AW(SHDeleteKey)

DWORD WINAPI SHDeleteEmptyKeyA(HKEY hKey, LPCSTR lpszSubKey);
DWORD WINAPI SHDeleteEmptyKeyW(HKEY hKey, LPCWSTR lpszSubKey);
#define  SHDeleteEmptyKey WINELIB_NAME_AW(SHDeleteEmptyKey)

HRESULT WINAPI UrlCanonicalizeA(LPCSTR pszUrl, LPSTR pszCanonicalized, 
				LPDWORD pcchCanonicalized, DWORD dwFlags);
HRESULT WINAPI UrlCanonicalizeW(LPCWSTR pszUrl, LPWSTR pszCanonicalized, 
				LPDWORD pcchCanonicalized, DWORD dwFlags);
#define UrlCanonicalize WINELIB_NAME_AW(UrlCanoncalize)

HRESULT WINAPI UrlEscapeA(LPCSTR pszUrl, LPSTR pszEscaped, LPDWORD pcchEscaped,
			  DWORD dwFlags);
HRESULT WINAPI UrlEscapeW(LPCWSTR pszUrl, LPWSTR pszEscaped,
			  LPDWORD pcchEscaped, DWORD dwFlags);
#define UrlEscape WINELIB_NAME_AW(UrlEscape)

HRESULT WINAPI UrlUnescapeA(LPCSTR pszUrl, LPSTR pszUnescaped,
			    LPDWORD pcchUnescaped, DWORD dwFlags);
HRESULT WINAPI UrlUnescapeW(LPCWSTR pszUrl, LPWSTR pszUnescaped,
			    LPDWORD pcchUnescaped, DWORD dwFlags);
#define UrlUnescape WINELIB_AW_NAME(UrlUnescape)

typedef struct _DllVersionInfo {
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO;

#define DLLVER_PLATFORM_WINDOWS         0x00000001      // Windows 9x
#define DLLVER_PLATFORM_NT              0x00000002      // Windows NT

typedef HRESULT CALLBACK (*DLLGETVERSIONPROC)(DLLVERSIONINFO *);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_SHLWAPI_H */
