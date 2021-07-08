/*
 * Path Functions
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
 * Copyright 2004 Juan Lang
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES:
 *
 * Many of these functions are in SHLWAPI.DLL also
 *
 */

#define COBJMACROS

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "shlobj.h"
#include "shtypes.h"
#include "shresdef.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "pidl.h"
#include "wine/unicode.h"
#include "shlwapi.h"
#include "xdg.h"
#include "sddl.h"
#include "knownfolders.h"
#include "initguid.h"
#include "shobjidl.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static const BOOL is_win64 = sizeof(void *) > sizeof(int);

/*
	########## Combining and Constructing paths ##########
*/

/*************************************************************************
 * PathAppend		[SHELL32.36]
 */
BOOL WINAPI PathAppendAW(
	LPVOID lpszPath1,
	LPCVOID lpszPath2)
{
	if (SHELL_OsIsUnicode())
	  return PathAppendW(lpszPath1, lpszPath2);
	return PathAppendA(lpszPath1, lpszPath2);
}

/*************************************************************************
 * PathCombine	 [SHELL32.37]
 */
LPVOID WINAPI PathCombineAW(
	LPVOID szDest,
	LPCVOID lpszDir,
	LPCVOID lpszFile)
{
	if (SHELL_OsIsUnicode())
	  return PathCombineW( szDest, lpszDir, lpszFile );
	return PathCombineA( szDest, lpszDir, lpszFile );
}

/*************************************************************************
 * PathAddBackslash		[SHELL32.32]
 */
LPVOID WINAPI PathAddBackslashAW(LPVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
	  return PathAddBackslashW(lpszPath);
	return PathAddBackslashA(lpszPath);
}

/*************************************************************************
 * PathBuildRoot		[SHELL32.30]
 */
LPVOID WINAPI PathBuildRootAW(LPVOID lpszPath, int drive)
{
	if(SHELL_OsIsUnicode())
	  return PathBuildRootW(lpszPath, drive);
	return PathBuildRootA(lpszPath, drive);
}

/*
	Extracting Component Parts
*/

/*************************************************************************
 * PathFindFileName	[SHELL32.34]
 */
LPVOID WINAPI PathFindFileNameAW(LPCVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
	  return PathFindFileNameW(lpszPath);
	return PathFindFileNameA(lpszPath);
}

/*************************************************************************
 * PathFindExtension		[SHELL32.31]
 */
LPVOID WINAPI PathFindExtensionAW(LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathFindExtensionW(lpszPath);
	return PathFindExtensionA(lpszPath);

}

/*************************************************************************
 * PathGetExtensionA		[internal]
 *
 * NOTES
 *  exported by ordinal
 *  return value points to the first char after the dot
 */
static LPSTR PathGetExtensionA(LPCSTR lpszPath)
{
	TRACE("(%s)\n",lpszPath);

	lpszPath = PathFindExtensionA(lpszPath);
	return (LPSTR)(*lpszPath?(lpszPath+1):lpszPath);
}

/*************************************************************************
 * PathGetExtensionW		[internal]
 */
static LPWSTR PathGetExtensionW(LPCWSTR lpszPath)
{
	TRACE("(%s)\n",debugstr_w(lpszPath));

	lpszPath = PathFindExtensionW(lpszPath);
	return (LPWSTR)(*lpszPath?(lpszPath+1):lpszPath);
}

/*************************************************************************
 * PathGetExtension		[SHELL32.158]
 */
LPVOID WINAPI PathGetExtensionAW(LPCVOID lpszPath,DWORD void1, DWORD void2)
{
	if (SHELL_OsIsUnicode())
	  return PathGetExtensionW(lpszPath);
	return PathGetExtensionA(lpszPath);
}

/*************************************************************************
 * PathGetArgs	[SHELL32.52]
 */
LPVOID WINAPI PathGetArgsAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathGetArgsW(lpszPath);
	return PathGetArgsA(lpszPath);
}

/*************************************************************************
 * PathGetDriveNumber	[SHELL32.57]
 */
int WINAPI PathGetDriveNumberAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathGetDriveNumberW(lpszPath);
	return PathGetDriveNumberA(lpszPath);
}

/*************************************************************************
 * PathRemoveFileSpec [SHELL32.35]
 */
BOOL WINAPI PathRemoveFileSpecAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathRemoveFileSpecW(lpszPath);
	return PathRemoveFileSpecA(lpszPath);
}

/*************************************************************************
 * PathStripPath	[SHELL32.38]
 */
void WINAPI PathStripPathAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
            PathStripPathW(lpszPath);
        else
            PathStripPathA(lpszPath);
}

/*************************************************************************
 * PathStripToRoot	[SHELL32.50]
 */
BOOL WINAPI PathStripToRootAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathStripToRootW(lpszPath);
	return PathStripToRootA(lpszPath);
}

/*************************************************************************
 * PathRemoveArgs	[SHELL32.251]
 */
void WINAPI PathRemoveArgsAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
            PathRemoveArgsW(lpszPath);
        else
            PathRemoveArgsA(lpszPath);
}

/*************************************************************************
 * PathRemoveExtension	[SHELL32.250]
 */
void WINAPI PathRemoveExtensionAW(LPVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
            PathRemoveExtensionW(lpszPath);
        else
            PathRemoveExtensionA(lpszPath);
}


/*
	Path Manipulations
*/

/*************************************************************************
 * PathGetShortPathA [internal]
 */
static void PathGetShortPathA(LPSTR pszPath)
{
	CHAR path[MAX_PATH];

	TRACE("%s\n", pszPath);

	if (GetShortPathNameA(pszPath, path, MAX_PATH))
	{
	  lstrcpyA(pszPath, path);
	}
}

/*************************************************************************
 * PathGetShortPathW [internal]
 */
static void PathGetShortPathW(LPWSTR pszPath)
{
	WCHAR path[MAX_PATH];

	TRACE("%s\n", debugstr_w(pszPath));

	if (GetShortPathNameW(pszPath, path, MAX_PATH))
	{
	  lstrcpyW(pszPath, path);
	}
}

/*************************************************************************
 * PathGetShortPath [SHELL32.92]
 */
VOID WINAPI PathGetShortPathAW(LPVOID pszPath)
{
	if(SHELL_OsIsUnicode())
	  PathGetShortPathW(pszPath);
	PathGetShortPathA(pszPath);
}

/*************************************************************************
 * PathRemoveBlanks [SHELL32.33]
 */
void WINAPI PathRemoveBlanksAW(LPVOID str)
{
	if(SHELL_OsIsUnicode())
            PathRemoveBlanksW(str);
        else
            PathRemoveBlanksA(str);
}

/*************************************************************************
 * PathQuoteSpaces [SHELL32.55]
 */
VOID WINAPI PathQuoteSpacesAW (LPVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
            PathQuoteSpacesW(lpszPath);
        else
            PathQuoteSpacesA(lpszPath);
}

/*************************************************************************
 * PathUnquoteSpaces [SHELL32.56]
 */
VOID WINAPI PathUnquoteSpacesAW(LPVOID str)
{
	if(SHELL_OsIsUnicode())
	  PathUnquoteSpacesW(str);
	else
	  PathUnquoteSpacesA(str);
}

/*************************************************************************
 * PathParseIconLocation	[SHELL32.249]
 */
int WINAPI PathParseIconLocationAW (LPVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
	  return PathParseIconLocationW(lpszPath);
	return PathParseIconLocationA(lpszPath);
}

/*
	########## Path Testing ##########
*/
/*************************************************************************
 * PathIsUNC		[SHELL32.39]
 */
BOOL WINAPI PathIsUNCAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathIsUNCW( lpszPath );
	return PathIsUNCA( lpszPath );
}

/*************************************************************************
 *  PathIsRelative	[SHELL32.40]
 */
BOOL WINAPI PathIsRelativeAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathIsRelativeW( lpszPath );
	return PathIsRelativeA( lpszPath );
}

/*************************************************************************
 * PathIsRoot		[SHELL32.29]
 */
BOOL WINAPI PathIsRootAW(LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathIsRootW(lpszPath);
	return PathIsRootA(lpszPath);
}

/*************************************************************************
 *  PathIsExeA		[internal]
 */
static BOOL PathIsExeA (LPCSTR lpszPath)
{
	LPCSTR lpszExtension = PathGetExtensionA(lpszPath);
        int i;
        static const char * const lpszExtensions[] =
            {"exe", "com", "pif", "cmd", "bat", "scf", "scr", NULL };

	TRACE("path=%s\n",lpszPath);

	for(i=0; lpszExtensions[i]; i++)
	  if (!lstrcmpiA(lpszExtension,lpszExtensions[i])) return TRUE;

	return FALSE;
}

/*************************************************************************
 *  PathIsExeW		[internal]
 */
static BOOL PathIsExeW (LPCWSTR lpszPath)
{
	LPCWSTR lpszExtension = PathGetExtensionW(lpszPath);
        int i;
        static const WCHAR lpszExtensions[][4] =
            {{'e','x','e','\0'}, {'c','o','m','\0'}, {'p','i','f','\0'},
             {'c','m','d','\0'}, {'b','a','t','\0'}, {'s','c','f','\0'},
             {'s','c','r','\0'}, {'\0'} };

	TRACE("path=%s\n",debugstr_w(lpszPath));

	for(i=0; lpszExtensions[i][0]; i++)
	  if (!strcmpiW(lpszExtension,lpszExtensions[i])) return TRUE;

	return FALSE;
}

/*************************************************************************
 *  PathIsExe		[SHELL32.43]
 */
BOOL WINAPI PathIsExeAW (LPCVOID path)
{
	if (SHELL_OsIsUnicode())
	  return PathIsExeW (path);
	return PathIsExeA(path);
}

/*************************************************************************
 * PathIsDirectory	[SHELL32.159]
 */
BOOL WINAPI PathIsDirectoryAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathIsDirectoryW (lpszPath);
	return PathIsDirectoryA (lpszPath);
}

/*************************************************************************
 * PathFileExists	[SHELL32.45]
 */
BOOL WINAPI PathFileExistsAW (LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathFileExistsW (lpszPath);
	return PathFileExistsA (lpszPath);
}

/*************************************************************************
 * PathMatchSpec	[SHELL32.46]
 */
BOOL WINAPI PathMatchSpecAW(LPVOID name, LPVOID mask)
{
	if (SHELL_OsIsUnicode())
	  return PathMatchSpecW( name, mask );
	return PathMatchSpecA( name, mask );
}

/*************************************************************************
 * PathIsSameRoot	[SHELL32.650]
 */
BOOL WINAPI PathIsSameRootAW(LPCVOID lpszPath1, LPCVOID lpszPath2)
{
	if (SHELL_OsIsUnicode())
	  return PathIsSameRootW(lpszPath1, lpszPath2);
	return PathIsSameRootA(lpszPath1, lpszPath2);
}

/*************************************************************************
 * IsLFNDriveA		[SHELL32.41]
 */
BOOL WINAPI IsLFNDriveA(LPCSTR lpszPath)
{
    DWORD	fnlen;

    if (!GetVolumeInformationA(lpszPath, NULL, 0, NULL, &fnlen, NULL, NULL, 0))
	return FALSE;
    return fnlen > 12;
}

/*************************************************************************
 * IsLFNDriveW		[SHELL32.42]
 */
BOOL WINAPI IsLFNDriveW(LPCWSTR lpszPath)
{
    DWORD	fnlen;

    if (!GetVolumeInformationW(lpszPath, NULL, 0, NULL, &fnlen, NULL, NULL, 0))
	return FALSE;
    return fnlen > 12;
}

/*************************************************************************
 * IsLFNDrive		[SHELL32.119]
 */
BOOL WINAPI IsLFNDriveAW(LPCVOID lpszPath)
{
	if (SHELL_OsIsUnicode())
	  return IsLFNDriveW(lpszPath);
	return IsLFNDriveA(lpszPath);
}

/*
	########## Creating Something Unique ##########
*/
/*************************************************************************
 * PathMakeUniqueNameA	[internal]
 */
static BOOL PathMakeUniqueNameA(
	LPSTR lpszBuffer,
	DWORD dwBuffSize,
	LPCSTR lpszShortName,
	LPCSTR lpszLongName,
	LPCSTR lpszPathName)
{
	FIXME("%p %u %s %s %s stub\n",
	 lpszBuffer, dwBuffSize, debugstr_a(lpszShortName),
	 debugstr_a(lpszLongName), debugstr_a(lpszPathName));
	return TRUE;
}

/*************************************************************************
 * PathMakeUniqueNameW	[internal]
 */
static BOOL PathMakeUniqueNameW(
	LPWSTR lpszBuffer,
	DWORD dwBuffSize,
	LPCWSTR lpszShortName,
	LPCWSTR lpszLongName,
	LPCWSTR lpszPathName)
{
	FIXME("%p %u %s %s %s stub\n",
	 lpszBuffer, dwBuffSize, debugstr_w(lpszShortName),
	 debugstr_w(lpszLongName), debugstr_w(lpszPathName));
	return TRUE;
}

/*************************************************************************
 * PathMakeUniqueName	[SHELL32.47]
 */
BOOL WINAPI PathMakeUniqueNameAW(
	LPVOID lpszBuffer,
	DWORD dwBuffSize,
	LPCVOID lpszShortName,
	LPCVOID lpszLongName,
	LPCVOID lpszPathName)
{
	if (SHELL_OsIsUnicode())
	  return PathMakeUniqueNameW(lpszBuffer,dwBuffSize, lpszShortName,lpszLongName,lpszPathName);
	return PathMakeUniqueNameA(lpszBuffer,dwBuffSize, lpszShortName,lpszLongName,lpszPathName);
}

/*************************************************************************
 * PathYetAnotherMakeUniqueName [SHELL32.75]
 */
BOOL WINAPI PathYetAnotherMakeUniqueName(LPWSTR buffer, LPCWSTR path, LPCWSTR shortname, LPCWSTR longname)
{
    WCHAR pathW[MAX_PATH], retW[MAX_PATH];
    const WCHAR *file, *ext;
    int i = 2;

    TRACE("(%p, %s, %s, %s)\n", buffer, debugstr_w(path), debugstr_w(shortname), debugstr_w(longname));

    file = longname ? longname : shortname;
    PathCombineW(pathW, path, file);
    strcpyW(retW, pathW);
    PathRemoveExtensionW(pathW);

    ext = PathFindExtensionW(file);

    /* now try to make it unique */
    while (PathFileExistsW(retW))
    {
        static const WCHAR fmtW[] = {'%','s',' ','(','%','d',')','%','s',0};

        sprintfW(retW, fmtW, pathW, i, ext);
        i++;
    }

    strcpyW(buffer, retW);
    TRACE("ret - %s\n", debugstr_w(buffer));

    return TRUE;
}

/*
	########## cleaning and resolving paths ##########
 */

/*************************************************************************
 * PathFindOnPath	[SHELL32.145]
 */
BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID *sOtherDirs)
{
	if (SHELL_OsIsUnicode())
	  return PathFindOnPathW(sFile, (LPCWSTR *)sOtherDirs);
	return PathFindOnPathA(sFile, (LPCSTR *)sOtherDirs);
}

/*************************************************************************
 * PathCleanupSpec	[SHELL32.171]
 *
 * lpszFile is changed in place.
 */
int WINAPI PathCleanupSpec( LPCWSTR lpszPathW, LPWSTR lpszFileW )
{
    int i = 0;
    DWORD rc = 0;
    int length = 0;

    if (SHELL_OsIsUnicode())
    {
        LPWSTR p = lpszFileW;

        TRACE("Cleanup %s\n",debugstr_w(lpszFileW));

        if (lpszPathW)
            length = strlenW(lpszPathW);

        while (*p)
        {
            int gct = PathGetCharTypeW(*p);
            if (gct == GCT_INVALID || gct == GCT_WILD || gct == GCT_SEPARATOR)
            {
                lpszFileW[i]='-';
                rc |= PCS_REPLACEDCHAR;
            }
            else
                lpszFileW[i]=*p;
            i++;
            p++;
            if (length + i == MAX_PATH)
            {
                rc |= PCS_FATAL | PCS_PATHTOOLONG;
                break;
            }
        }
        lpszFileW[i]=0;
    }
    else
    {
        LPSTR lpszFileA = (LPSTR)lpszFileW;
        LPCSTR lpszPathA = (LPCSTR)lpszPathW;
        LPSTR p = lpszFileA;

        TRACE("Cleanup %s\n",debugstr_a(lpszFileA));

        if (lpszPathA)
            length = strlen(lpszPathA);

        while (*p)
        {
            int gct = PathGetCharTypeA(*p);
            if (gct == GCT_INVALID || gct == GCT_WILD || gct == GCT_SEPARATOR)
            {
                lpszFileA[i]='-';
                rc |= PCS_REPLACEDCHAR;
            }
            else
                lpszFileA[i]=*p;
            i++;
            p++;
            if (length + i == MAX_PATH)
            {
                rc |= PCS_FATAL | PCS_PATHTOOLONG;
                break;
            }
        }
        lpszFileA[i]=0;
    }
    return rc;
}

/*************************************************************************
 * PathQualifyA		[SHELL32]
 */
static BOOL PathQualifyA(LPCSTR pszPath)
{
	FIXME("%s\n",pszPath);
	return FALSE;
}

/*************************************************************************
 * PathQualifyW		[SHELL32]
 */
static BOOL PathQualifyW(LPCWSTR pszPath)
{
	FIXME("%s\n",debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 * PathQualify	[SHELL32.49]
 */
BOOL WINAPI PathQualifyAW(LPCVOID pszPath)
{
	if (SHELL_OsIsUnicode())
	  return PathQualifyW(pszPath);
	return PathQualifyA(pszPath);
}

BOOL WINAPI PathFindOnPathExA(LPSTR,LPCSTR *,DWORD);
BOOL WINAPI PathFindOnPathExW(LPWSTR,LPCWSTR *,DWORD);
BOOL WINAPI PathFileExistsDefExtA(LPSTR,DWORD);
BOOL WINAPI PathFileExistsDefExtW(LPWSTR,DWORD);

static BOOL PathResolveA(char *path, const char **dirs, DWORD flags)
{
    BOOL is_file_spec = PathIsFileSpecA(path);
    DWORD dwWhich = flags & PRF_DONTFINDLNK ? 0xf : 0xff;

    TRACE("(%s,%p,0x%08x)\n", debugstr_a(path), dirs, flags);

    if (flags & PRF_VERIFYEXISTS && !PathFileExistsA(path))
    {
        if (PathFindOnPathExA(path, dirs, dwWhich))
            return TRUE;
        if (PathFileExistsDefExtA(path, dwWhich))
            return TRUE;
        if (!is_file_spec) GetFullPathNameA(path, MAX_PATH, path, NULL);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (is_file_spec)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    GetFullPathNameA(path, MAX_PATH, path, NULL);

    return TRUE;
}

static BOOL PathResolveW(WCHAR *path, const WCHAR **dirs, DWORD flags)
{
    BOOL is_file_spec = PathIsFileSpecW(path);
    DWORD dwWhich = flags & PRF_DONTFINDLNK ? 0xf : 0xff;

    TRACE("(%s,%p,0x%08x)\n", debugstr_w(path), dirs, flags);

    if (flags & PRF_VERIFYEXISTS && !PathFileExistsW(path))
    {
        if (PathFindOnPathExW(path, dirs, dwWhich))
            return TRUE;
        if (PathFileExistsDefExtW(path, dwWhich))
            return TRUE;
        if (!is_file_spec) GetFullPathNameW(path, MAX_PATH, path, NULL);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    if (is_file_spec)
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }

    GetFullPathNameW(path, MAX_PATH, path, NULL);

    return TRUE;
}

/*************************************************************************
 * PathResolve [SHELL32.51]
 */
BOOL WINAPI PathResolveAW(void *path, const void **paths, DWORD flags)
{
    if (SHELL_OsIsUnicode())
        return PathResolveW(path, (const WCHAR **)paths, flags);
    else
        return PathResolveA(path, (const char **)paths, flags);
}

/*************************************************************************
*	PathProcessCommandA
*/
static LONG PathProcessCommandA (
	LPCSTR lpszPath,
	LPSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("%s %p 0x%04x 0x%04x stub\n",
	lpszPath, lpszBuff, dwBuffSize, dwFlags);
	if(!lpszPath) return -1;
	if(lpszBuff) strcpy(lpszBuff, lpszPath);
	return strlen(lpszPath);
}

/*************************************************************************
*	PathProcessCommandW
*/
static LONG PathProcessCommandW (
	LPCWSTR lpszPath,
	LPWSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("(%s, %p, 0x%04x, 0x%04x) stub\n",
	debugstr_w(lpszPath), lpszBuff, dwBuffSize, dwFlags);
	if(!lpszPath) return -1;
	if(lpszBuff) strcpyW(lpszBuff, lpszPath);
	return strlenW(lpszPath);
}

/*************************************************************************
*	PathProcessCommand (SHELL32.653)
*/
LONG WINAPI PathProcessCommandAW (
	LPCVOID lpszPath,
	LPVOID lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	if (SHELL_OsIsUnicode())
	  return PathProcessCommandW(lpszPath, lpszBuff, dwBuffSize, dwFlags);
	return PathProcessCommandA(lpszPath, lpszBuff, dwBuffSize, dwFlags);
}

/*
	########## special ##########
*/

/*************************************************************************
 * PathSetDlgItemPath (SHELL32.48)
 */
VOID WINAPI PathSetDlgItemPathAW(HWND hDlg, int id, LPCVOID pszPath)
{
	if (SHELL_OsIsUnicode())
            PathSetDlgItemPathW(hDlg, id, pszPath);
        else
            PathSetDlgItemPathA(hDlg, id, pszPath);
}

static const WCHAR szCategory[] = {'C','a','t','e','g','o','r','y',0};
static const WCHAR szAttributes[] = {'A','t','t','r','i','b','u','t','e','s',0};
static const WCHAR szName[] = {'N','a','m','e',0};
static const WCHAR szParsingName[] = {'P','a','r','s','i','n','g','N','a','m','e',0};
static const WCHAR szRelativePath[] = {'R','e','l','a','t','i','v','e','P','a','t','h',0};
static const WCHAR szParentFolder[] = {'P','a','r','e','n','t','F','o','l','d','e','r',0};

static const WCHAR szCurrentVersion[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\0'};
static const WCHAR AddNewProgramsFolderW[] = {'A','d','d','N','e','w','P','r','o','g','r','a','m','s','F','o','l','d','e','r',0};
static const WCHAR AppUpdatesFolderW[] = {'A','p','p','U','p','d','a','t','e','s','F','o','l','d','e','r',0};
static const WCHAR Administrative_ToolsW[] = {'A','d','m','i','n','i','s','t','r','a','t','i','v','e',' ','T','o','o','l','s','\0'};
static const WCHAR AppDataW[] = {'A','p','p','D','a','t','a','\0'};
static const WCHAR AppData_RoamingW[] = {'A','p','p','D','a','t','a','\\','R','o','a','m','i','n','g','\0'};
static const WCHAR AppData_LocalLowW[] = {'A','p','p','D','a','t','a','\\','L','o','c','a','l','L','o','w','\0'};
static const WCHAR AppData_LocalW[] = {'A','p','p','D','a','t','a','\\','L','o','c','a','l','\0'};
static const WCHAR CacheW[] = {'C','a','c','h','e','\0'};
static const WCHAR CD_BurningW[] = {'C','D',' ','B','u','r','n','i','n','g','\0'};
static const WCHAR ChangeRemoveProgramsFolderW[] = {'C','h','a','n','g','e','R','e','m','o','v','e','P','r','o','g','r','a','m','s','F','o','l','d','e','r',0};
static const WCHAR CommonW[] = {'C','o','m','m','o','n',0};
static const WCHAR Common_Administrative_ToolsW[] = {'C','o','m','m','o','n',' ','A','d','m','i','n','i','s','t','r','a','t','i','v','e',' ','T','o','o','l','s','\0'};
static const WCHAR Common_AppDataW[] = {'C','o','m','m','o','n',' ','A','p','p','D','a','t','a','\0'};
static const WCHAR Common_DesktopW[] = {'C','o','m','m','o','n',' ','D','e','s','k','t','o','p','\0'};
static const WCHAR Common_DocumentsW[] = {'C','o','m','m','o','n',' ','D','o','c','u','m','e','n','t','s','\0'};
static const WCHAR CommonDownloadsW[] = {'C','o','m','m','o','n','D','o','w','n','l','o','a','d','s',0};
static const WCHAR Common_FavoritesW[] = {'C','o','m','m','o','n',' ','F','a','v','o','r','i','t','e','s','\0'};
static const WCHAR CommonFilesDirW[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r','\0'};
static const WCHAR CommonFilesDirX86W[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',' ','(','x','8','6',')','\0'};
static const WCHAR CommonMusicW[] = {'C','o','m','m','o','n','M','u','s','i','c','\0'};
static const WCHAR CommonPicturesW[] = {'C','o','m','m','o','n','P','i','c','t','u','r','e','s','\0'};
static const WCHAR Common_ProgramsW[] = {'C','o','m','m','o','n',' ','P','r','o','g','r','a','m','s','\0'};
static const WCHAR CommonRingtonesW[] = {'C','o','m','m','o','n','R','i','n','g','t','o','n','e','s',0};
static const WCHAR Common_StartUpW[] = {'C','o','m','m','o','n',' ','S','t','a','r','t','U','p','\0'};
static const WCHAR Common_StartupW[] = {'C','o','m','m','o','n',' ','S','t','a','r','t','u','p','\0'};
static const WCHAR Common_Start_MenuW[] = {'C','o','m','m','o','n',' ','S','t','a','r','t',' ','M','e','n','u','\0'};
static const WCHAR Common_TemplatesW[] = {'C','o','m','m','o','n',' ','T','e','m','p','l','a','t','e','s','\0'};
static const WCHAR CommonVideoW[] = {'C','o','m','m','o','n','V','i','d','e','o','\0'};
static const WCHAR ConflictFolderW[] = {'C','o','n','f','l','i','c','t','F','o','l','d','e','r',0};
static const WCHAR ConnectionsFolderW[] = {'C','o','n','n','e','c','t','i','o','n','s','F','o','l','d','e','r',0};
static const WCHAR ContactsW[] = {'C','o','n','t','a','c','t','s','\0'};
static const WCHAR ControlPanelFolderW[] = {'C','o','n','t','r','o','l','P','a','n','e','l','F','o','l','d','e','r',0};
static const WCHAR CookiesW[] = {'C','o','o','k','i','e','s','\0'};
static const WCHAR CSCFolderW[] = {'C','S','C','F','o','l','d','e','r',0};
static const WCHAR Default_GadgetsW[] = {'D','e','f','a','u','l','t',' ','G','a','d','g','e','t','s',0};
static const WCHAR DesktopW[] = {'D','e','s','k','t','o','p','\0'};
static const WCHAR Device_Metadata_StoreW[] = {'D','e','v','i','c','e',' ','M','e','t','a','d','a','t','a',' ','S','t','o','r','e',0};
static const WCHAR DocumentsW[] = {'D','o','c','u','m','e','n','t','s','\0'};
static const WCHAR DocumentsLibraryW[] = {'D','o','c','u','m','e','n','t','s','L','i','b','r','a','r','y','\0'};
static const WCHAR Documents_librarymsW[] = {'D','o','c','u','m','e','n','t','s','.','l','i','b','r','a','r','y','-','m','s',0};
static const WCHAR DownloadsW[] = {'D','o','w','n','l','o','a','d','s','\0'};
static const WCHAR FavoritesW[] = {'F','a','v','o','r','i','t','e','s','\0'};
static const WCHAR FontsW[] = {'F','o','n','t','s','\0'};
static const WCHAR GadgetsW[] = {'G','a','d','g','e','t','s',0};
static const WCHAR GamesW[] = {'G','a','m','e','s',0};
static const WCHAR GameTasksW[] = {'G','a','m','e','T','a','s','k','s',0};
static const WCHAR HistoryW[] = {'H','i','s','t','o','r','y','\0'};
static const WCHAR HomeGroupFolderW[] = {'H','o','m','e','G','r','o','u','p','F','o','l','d','e','r',0};
static const WCHAR ImplicitAppShortcutsW[] = {'I','m','p','l','i','c','i','t','A','p','p','S','h','o','r','t','c','u','t','s',0};
static const WCHAR InternetFolderW[] = {'I','n','t','e','r','n','e','t','F','o','l','d','e','r',0};
static const WCHAR LibrariesW[] = {'L','i','b','r','a','r','i','e','s',0};
static const WCHAR LinksW[] = {'L','i','n','k','s','\0'};
static const WCHAR Local_AppDataW[] = {'L','o','c','a','l',' ','A','p','p','D','a','t','a','\0'};
static const WCHAR LocalAppDataLowW[] = {'L','o','c','a','l','A','p','p','D','a','t','a','L','o','w',0};
static const WCHAR LocalizedResourcesDirW[] = {'L','o','c','a','l','i','z','e','d','R','e','s','o','u','r','c','e','s','D','i','r',0};
static const WCHAR MAPIFolderW[] = {'M','A','P','I','F','o','l','d','e','r',0};
static const WCHAR Microsoft_Internet_Explorer_Quick_LaunchW[] = {'M','i','c','r','o','s','o','f','t','\\','I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r','\\','Q','u','i','c','k',' ','L','a','u','n','c','h',0};
static const WCHAR Microsoft_Windows_Burn_BurnW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','B','u','r','n','\\','B','u','r','n',0};
static const WCHAR Microsoft_Windows_GameExplorerW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','G','a','m','e','E','x','p','l','o','r','e','r','\0'};
static const WCHAR Microsoft_Windows_DeviceMetadataStoreW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','D','e','v','i','c','e','M','e','t','a','d','a','t','a','S','t','o','r','e',0};
static const WCHAR Microsoft_Windows_HistoryW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','H','i','s','t','o','r','y',0};
static const WCHAR Microsoft_Windows_INetCacheW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','I','N','e','t','C','a','c','h','e',0};
static const WCHAR Microsoft_Windows_INetCookiesW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','I','N','e','t','C','o','o','k','i','e','s',0};
static const WCHAR Microsoft_Windows_LibrariesW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','L','i','b','r','a','r','i','e','s','\0'};
static const WCHAR Microsoft_Windows_Network_ShortcutsW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','N','e','t','w','o','r','k',' ','S','h','o','r','t','c','u','t','s',0};
static const WCHAR Microsoft_Windows_Photo_Gallery_Original_ImagesW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ','P','h','o','t','o',' ','G','a','l','l','e','r','y','\\','O','r','i','g','i','n','a','l',' ','I','m','a','g','e','s',0};
static const WCHAR Microsoft_Windows_Printer_ShortcutsW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','P','r','i','n','t','e','r',' ','S','h','o','r','t','c','u','t','s',0};
static const WCHAR Microsoft_Windows_RecentW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','R','e','c','e','n','t','\0'};
static const WCHAR Microsoft_Windows_RingtonesW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','R','i','n','g','t','o','n','e','s','\0'};
static const WCHAR Microsoft_Windows_SendToW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','S','e','n','d','T','o',0};
static const WCHAR Microsoft_Windows_Sidebar_GadgetsW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ','S','i','d','e','b','a','r','\\','G','a','d','g','e','t','s',0};
static const WCHAR Microsoft_Windows_Start_MenuW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','S','t','a','r','t',' ','M','e','n','u',0};
static const WCHAR Microsoft_Windows_TemplatesW[] = {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','T','e','m','p','l','a','t','e','s',0};
static const WCHAR Microsoft_Windows_ThemesW[] =  {'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','T','h','e','m','e','s',0};
static const WCHAR MoviesW[] = {'M','o','v','i','e','s','\0'};
static const WCHAR MusicW[] = {'M','u','s','i','c','\0'};
static const WCHAR MusicLibraryW[] = {'M','u','s','i','c','L','i','b','r','a','r','y',0};
static const WCHAR Music_librarymsW[] = {'M','u','s','i','c','.','l','i','b','r','a','r','y','-','m','s',0};
static const WCHAR My_MusicW[] = {'M','y',' ','M','u','s','i','c','\0'};
static const WCHAR My_PicturesW[] = {'M','y',' ','P','i','c','t','u','r','e','s','\0'};
static const WCHAR My_VideosW[] = {'M','y',' ','V','i','d','e','o','s','\0'};
static const WCHAR My_VideoW[] = {'M','y',' ','V','i','d','e','o','\0'};
static const WCHAR MyComputerFolderW[] = {'M','y','C','o','m','p','u','t','e','r','F','o','l','d','e','r',0};
static const WCHAR NetHoodW[] = {'N','e','t','H','o','o','d','\0'};
static const WCHAR NetworkPlacesFolderW[] = {'N','e','t','w','o','r','k','P','l','a','c','e','s','F','o','l','d','e','r',0};
static const WCHAR OEM_LinksW[] = {'O','E','M',' ','L','i','n','k','s','\0'};
static const WCHAR Original_ImagesW[] = {'O','r','i','g','i','n','a','l',' ','I','m','a','g','e','s',0};
static const WCHAR PersonalW[] = {'P','e','r','s','o','n','a','l','\0'};
static const WCHAR PhotoAlbumsW[] = {'P','h','o','t','o','A','l','b','u','m','s',0};
static const WCHAR PicturesW[] = {'P','i','c','t','u','r','e','s','\0'};
static const WCHAR PicturesLibraryW[] = {'P','i','c','t','u','r','e','s','L','i','b','r','a','r','y',0};
static const WCHAR Pictures_librarymsW[] = {'P','i','c','t','u','r','e','s','.','l','i','b','r','a','r','y','-','m','s',0};
static const WCHAR PlaylistsW[] = {'P','l','a','y','l','i','s','t','s',0};
static const WCHAR PrintersFolderW[] = {'P','r','i','n','t','e','r','s','F','o','l','d','e','r',0};
static const WCHAR PrintHoodW[] = {'P','r','i','n','t','H','o','o','d','\0'};
static const WCHAR ProfileW[] = {'P','r','o','f','i','l','e',0};
static const WCHAR ProgramDataW[] = {'P','r','o','g','r','a','m','D','a','t','a','\0'};
static const WCHAR Program_FilesW[] = {'P','r','o','g','r','a','m',' ','F','i','l','e','s','\0'};
static const WCHAR ProgramFilesW[] = {'P','r','o','g','r','a','m','F','i','l','e','s','\0'};
static const WCHAR ProgramFilesX86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','X','8','6','\0'};
static const WCHAR ProgramFilesX64W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','X','6','4','\0'};
static const WCHAR Program_Files_Common_FilesW[] = {'P','r','o','g','r','a','m',' ','F','i','l','e','s','\\','C','o','m','m','o','n',' ','F','i','l','e','s','\0'};
static const WCHAR Program_Files_x86W[] = {'P','r','o','g','r','a','m',' ','F','i','l','e','s',' ','(','x','8','6',')','\0'};
static const WCHAR Program_Files_x86_Common_FilesW[] = {'P','r','o','g','r','a','m',' ','F','i','l','e','s',' ','(','x','8','6',')','\\','C','o','m','m','o','n',' ','F','i','l','e','s','\0'};
static const WCHAR ProgramFilesCommonW[] = {'P','r','o','g','r','a','m','F','i','l','e','s','C','o','m','m','o','n',0};
static const WCHAR ProgramFilesCommonX86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','C','o','m','m','o','n','X','8','6',0};
static const WCHAR ProgramFilesCommonX64W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','C','o','m','m','o','n','X','6','4',0};
static const WCHAR ProgramFilesDirW[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r','\0'};
static const WCHAR ProgramFilesDirX86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',' ','(','x','8','6',')','\0'};
static const WCHAR ProgramsW[] = {'P','r','o','g','r','a','m','s','\0'};
static const WCHAR PublicW[] = {'P','u','b','l','i','c',0};
static const WCHAR PublicGameTasksW[] = {'P','u','b','l','i','c','G','a','m','e','T','a','s','k','s',0};
static const WCHAR PublicLibrariesW[] = {'P','u','b','l','i','c','L','i','b','r','a','r','i','e','s',0};
static const WCHAR Quick_LaunchW[] = {'Q','u','i','c','k',' ','L','a','u','n','c','h',0};
static const WCHAR RecentW[] = {'R','e','c','e','n','t','\0'};
static const WCHAR RecordedTVLibraryW[] = {'R','e','c','o','r','d','e','d','T','V','L','i','b','r','a','r','y',0};
static const WCHAR RecordedTV_librarymsW[] = {'R','e','c','o','r','d','e','d','T','V','.','l','i','b','r','a','r','y','-','m','s',0};
static const WCHAR RecycleBinFolderW[] = {'R','e','c','y','c','l','e','B','i','n','F','o','l','d','e','r',0};
static const WCHAR ResourceDirW[] = {'R','e','s','o','u','r','c','e','D','i','r','\0'};
static const WCHAR ResourcesW[] = {'R','e','s','o','u','r','c','e','s','\0'};
static const WCHAR RingtonesW[] = {'R','i','n','g','t','o','n','e','s',0};
static const WCHAR SampleMusicW[] = {'S','a','m','p','l','e','M','u','s','i','c',0};
static const WCHAR Sample_MusicW[] = {'S','a','m','p','l','e',' ','M','u','s','i','c',0};
static const WCHAR SamplePicturesW[] = {'S','a','m','p','l','e','P','i','c','t','u','r','e','s',0};
static const WCHAR Sample_PicturesW[] = {'S','a','m','p','l','e',' ','P','i','c','t','u','r','e','s',0};
static const WCHAR SamplePlaylistsW[] = {'S','a','m','p','l','e','P','l','a','y','l','i','s','t','s',0};
static const WCHAR Sample_PlaylistsW[] = {'S','a','m','p','l','e',' ','P','l','a','y','l','i','s','t','s',0};
static const WCHAR Sample_VideosW[] = {'S','a','m','p','l','e',' ','V','i','d','e','o','s',0};
static const WCHAR SampleVideosW[] = {'S','a','m','p','l','e','V','i','d','e','o','s',0};
static const WCHAR Saved_GamesW[] = {'S','a','v','e','d',' ','G','a','m','e','s','\0'};
static const WCHAR SavedGamesW[] = {'S','a','v','e','d','G','a','m','e','s','\0'};
static const WCHAR SearchesW[] = {'S','e','a','r','c','h','e','s','\0'};
static const WCHAR SearchHomeFolderW[] = {'S','e','a','r','c','h','H','o','m','e','F','o','l','d','e','r',0};
static const WCHAR SendToW[] = {'S','e','n','d','T','o','\0'};
static const WCHAR Slide_ShowsW[] = {'S','l','i','d','e',' ','S','h','o','w','s',0};
static const WCHAR StartUpW[] = {'S','t','a','r','t','U','p','\0'};
static const WCHAR StartupW[] = {'S','t','a','r','t','u','p','\0'};
static const WCHAR Start_MenuW[] = {'S','t','a','r','t',' ','M','e','n','u','\0'};
static const WCHAR SyncCenterFolderW[] = {'S','y','n','c','C','e','n','t','e','r','F','o','l','d','e','r',0};
static const WCHAR SyncResultsFolderW[] = {'S','y','n','c','R','e','s','u','l','t','s','F','o','l','d','e','r',0};
static const WCHAR SyncSetupFolderW[] = {'S','y','n','c','S','e','t','u','p','F','o','l','d','e','r',0};
static const WCHAR SystemW[] = {'S','y','s','t','e','m',0};
static const WCHAR SystemX86W[] = {'S','y','s','t','e','m','X','8','6',0};
static const WCHAR TemplatesW[] = {'T','e','m','p','l','a','t','e','s','\0'};
static const WCHAR User_PinnedW[] = {'U','s','e','r',' ','P','i','n','n','e','d',0};
static const WCHAR UsersW[] = {'U','s','e','r','s','\0'};
static const WCHAR UsersFilesFolderW[] = {'U','s','e','r','s','F','i','l','e','s','F','o','l','d','e','r',0};
static const WCHAR UsersLibrariesFolderW[] = {'U','s','e','r','s','L','i','b','r','a','r','i','e','s','F','o','l','d','e','r',0};
static const WCHAR UserProfilesW[] = {'U','s','e','r','P','r','o','f','i','l','e','s',0};
static const WCHAR UserProgramFilesW[] = {'U','s','e','r','P','r','o','g','r','a','m','F','i','l','e','s',0};
static const WCHAR UserProgramFilesCommonW[] = {'U','s','e','r','P','r','o','g','r','a','m','F','i','l','e','s','C','o','m','m','o','n',0};
static const WCHAR UsersPublicW[] = {'u','s','e','r','s','\\','P','u','b','l','i','c','\0'};
static const WCHAR VideosW[] = {'V','i','d','e','o','s','\0'};
static const WCHAR VideosLibraryW[] = {'V','i','d','e','o','s','L','i','b','r','a','r','y',0};
static const WCHAR Videos_librarymsW[] = {'V','i','d','e','o','s','.','l','i','b','r','a','r','y','-','m','s',0};
static const WCHAR WindowsW[] = {'W','i','n','d','o','w','s',0};
static const WCHAR Windows_Sidebar_GadgetsW[] = {'W','i','n','d','o','w','s',' ','S','i','d','e','b','a','r','\\','G','a','d','g','e','t','s',0};
static const WCHAR DefaultW[] = {'.','D','e','f','a','u','l','t','\0'};
static const WCHAR AllUsersProfileW[] = {'%','A','L','L','U','S','E','R','S','P','R','O','F','I','L','E','%','\0'};
static const WCHAR PublicProfileW[] = {'%','P','U','B','L','I','C','%',0};
static const WCHAR UserProfileW[] = {'%','U','S','E','R','P','R','O','F','I','L','E','%','\0'};
static const WCHAR ProgramDataVarW[] = {'%','P','r','o','g','r','a','m','D','a','t','a','%','\0'};
static const WCHAR SystemDriveW[] = {'%','S','y','s','t','e','m','D','r','i','v','e','%','\0'};
static const WCHAR ProfileListW[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ','N','T','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','P','r','o','f','i','l','e','L','i','s','t',0};
static const WCHAR ProfilesDirectoryW[] = {'P','r','o','f','i','l','e','s','D','i','r','e','c','t','o','r','y',0};
static const WCHAR szSHFolders[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','S','h','e','l','l',' ','F','o','l','d','e','r','s','\0'};
static const WCHAR szSHUserFolders[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','U','s','e','r',' ','S','h','e','l','l',' ','F','o','l','d','e','r','s','\0'};
static const WCHAR szDefaultProfileDirW[] = {'u','s','e','r','s',0};
static const WCHAR szKnownFolderDescriptions[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','F','o','l','d','e','r','D','e','s','c','r','i','p','t','i','o','n','s','\0'};
static const WCHAR szKnownFolderRedirections[] = {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','E','x','p','l','o','r','e','r','\\','U','s','e','r',' ','S','h','e','l','l',' ','F','o','l','d','e','r','s',0};

#define CHANGEREMOVEPROGRAMS_PARSING_GUID '{','7','b','8','1','b','e','6','a','-','c','e','2','b','-','4','6','7','6','-','a','2','9','e','-','e','b','9','0','7','a','5','1','2','6','c','5','}'
#define SYNCMANAGER_PARSING_GUID '{','9','C','7','3','F','5','E','5','-','7','A','E','7','-','4','E','3','2','-','A','8','E','8','-','8','D','2','3','B','8','5','2','5','5','B','F','}'
#define SYSTEMFOLDERS_PARSING_GUID '{','2','1','E','C','2','0','2','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','D','-','0','8','0','0','2','B','3','0','3','0','9','D','}'
#define USERFOLDERS_PARSING_GUID '{','5','9','0','3','1','a','4','7','-','3','f','7','2','-','4','4','a','7','-','8','9','c','5','-','5','5','9','5','f','e','6','b','3','0','e','e','}'
#define USERSLIBRARIES_PARSING_GUID '{','0','3','1','E','4','8','2','5','-','7','B','9','4','-','4','d','c','3','-','B','1','3','1','-','E','9','4','6','B','4','4','C','8','D','D','5','}'

static const WCHAR ComputerFolderParsingNameW[] = {':',':','{','2','0','D','0','4','F','E','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','8','-','0','8','0','0','2','B','3','0','3','0','9','D','}',0};
static const WCHAR ControlPanelFolderParsingNameW[] = {':',':','{','2','6','E','E','0','6','6','8','-','A','0','0','A','-','4','4','D','7','-','9','3','7','1','-','B','E','B','0','6','4','C','9','8','6','8','3','}','\\','0',0};
static const WCHAR ControlPanelFolderRelativePathW[] = {':',':','{','2','1','E','C','2','0','2','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','D','-','0','8','0','0','2','B','3','0','3','0','9','D','}',0};
static const WCHAR GamesParsingNameW[] = {':',':','{','E','D','2','2','8','F','D','F','-','9','E','A','8','-','4','8','7','0','-','8','3','b','1','-','9','6','b','0','2','C','F','E','0','D','5','2','}',0};
static const WCHAR HomeGroupParsingNameW[] = {':',':','{','B','4','F','B','3','F','9','8','-','C','1','E','A','-','4','2','8','d','-','A','7','8','A','-','D','1','F','5','6','5','9','C','B','A','9','3','}',0};
static const WCHAR InternetFolderParsingNameW[] = {':',':','{','8','7','1','C','5','3','8','0','-','4','2','A','0','-','1','0','6','9','-','A','2','E','A','-','0','8','0','0','2','B','3','0','3','0','9','D','}',0};
static const WCHAR NetworkFolderParsingNameW[] = {':',':','{','F','0','2','C','1','A','0','D','-','B','E','2','1','-','4','3','5','0','-','8','8','B','0','-','7','3','6','7','F','C','9','6','E','F','3','C','}',0};
static const WCHAR PublicParsingNameW[] = {':',':','{','4','3','3','6','a','5','4','d','-','0','3','8','b','-','4','6','8','5','-','a','b','0','2','-','9','9','b','b','5','2','d','3','f','b','8','b','}',0};
static const WCHAR RecycleBinFolderParsingNameW[] = {':',':','{','6','4','5','F','F','0','4','0','-','5','0','8','1','-','1','0','1','B','-','9','F','0','8','-','0','0','A','A','0','0','2','F','9','5','4','E','}',0};
static const WCHAR SearchHomeParsingNameW[] = {':',':','{','9','3','4','3','8','1','2','e','-','1','c','3','7','-','4','a','4','9','-','a','1','2','e','-','4','b','2','d','8','1','0','d','9','5','6','b','}',0};
static const WCHAR SEARCH_CSCParsingNameW[] = {'s','h','e','l','l',':',':',':','{','B','D','7','A','2','E','7','B','-','2','1','C','B','-','4','1','b','2','-','A','0','8','6','-','B','3','0','9','6','8','0','C','6','B','7','E','}','\\','*',0};
static const WCHAR SEARCH_MAPIParsingNameW[] = {'s','h','e','l','l',':',':',':','{','8','9','D','8','3','5','7','6','-','6','B','D','1','-','4','C','8','6','-','9','4','5','4','-','B','E','B','0','4','E','9','4','C','8','1','9','}','\\','*',0};
static const WCHAR UsersFilesParsingNameW[] = {':',':','{','5','9','0','3','1','a','4','7','-','3','f','7','2','-','4','4','a','7','-','8','9','c','5','-','5','5','9','5','f','e','6','b','3','0','e','e','}',0};
static const WCHAR UsersLibrariesParsingNameW[] = {':',':','{','0','3','1','E','4','8','2','5','-','7','B','9','4','-','4','d','c','3','-','B','1','3','1','-','E','9','4','6','B','4','4','C','8','D','D','5','}',0};
static const WCHAR AddNewProgramsParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':','{','1','5','e','a','e','9','2','e','-','f','1','7','a','-','4','4','3','1','-','9','f','2','8','-','8','0','5','e','4','8','2','d','a','f','d','4','}',0};
static const WCHAR ConnectionsFolderParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':','{','7','0','0','7','A','C','C','7','-','3','2','0','2','-','1','1','D','1','-','A','A','D','2','-','0','0','8','0','5','F','C','1','2','7','0','E','}',0};
static const WCHAR PrintersFolderParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':','{','2','2','2','7','A','2','8','0','-','3','A','E','A','-','1','0','6','9','-','A','2','D','E','-','0','8','0','0','2','B','3','0','3','0','9','D','}',0};
static const WCHAR ChangeRemoveProgramsParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':', CHANGEREMOVEPROGRAMS_PARSING_GUID, 0};
static const WCHAR AppUpdatesParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':', CHANGEREMOVEPROGRAMS_PARSING_GUID, '\\',':',':','{','d','4','5','0','a','8','a','1','-','9','5','6','8','-','4','5','c','7','-','9','c','0','e','-','b','4','f','9','f','b','4','5','3','7','b','d','}',0};
static const WCHAR SyncManagerFolderParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':', SYNCMANAGER_PARSING_GUID, 0};
static const WCHAR ConflictFolderParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':', SYNCMANAGER_PARSING_GUID, '\\',':',':','{','E','4','1','3','D','0','4','0','-','6','7','8','8','-','4','C','2','2','-','9','5','7','E','-','1','7','5','D','1','C','5','1','3','A','3','4','}',',',0};
static const WCHAR SyncResultsFolderParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':', SYNCMANAGER_PARSING_GUID, '\\',':',':','{','B','C','4','8','B','3','2','F','-','5','9','1','0','-','4','7','F','5','-','8','5','7','0','-','5','0','7','4','A','8','A','5','6','3','6','A','}',',',0};
static const WCHAR SyncSetupFolderParsingNameW[] = {':',':', SYSTEMFOLDERS_PARSING_GUID, '\\',':',':', SYNCMANAGER_PARSING_GUID, '\\',':',':','{','F','1','3','9','0','A','9','A','-','A','3','F','4','-','4','E','5','D','-','9','C','5','F','-','9','8','F','3','B','D','8','D','9','3','5','C','}',',',0};
static const WCHAR ContactsParsingNameW[] = {':',':', USERFOLDERS_PARSING_GUID, '\\','{','5','6','7','8','4','8','5','4','-','C','6','C','B','-','4','6','2','B','-','8','1','6','9','-','8','8','E','3','5','0','A','C','B','8','8','2','}',0};
static const WCHAR DocumentsParsingNameW[] = {':',':', USERFOLDERS_PARSING_GUID, '\\','{','F','D','D','3','9','A','D','0','-','2','3','8','F','-','4','6','A','F','-','A','D','B','4','-','6','C','8','5','4','8','0','3','6','9','C','7','}',0};
static const WCHAR LinksParsingNameW[] = {':',':', USERFOLDERS_PARSING_GUID, '\\','{','b','f','b','9','d','5','e','0','-','c','6','a','9','-','4','0','4','c','-','b','2','b','2','-','a','e','6','d','b','6','a','f','4','9','6','8','}',0};
static const WCHAR MusicParsingNameW[] = {':',':', USERFOLDERS_PARSING_GUID, '\\','{','4','B','D','8','D','5','7','1','-','6','D','1','9','-','4','8','D','3','-','B','E','9','7','-','4','2','2','2','2','0','0','8','0','E','4','3','}',0};
static const WCHAR PicturesParsingNameW[] = {':',':', USERFOLDERS_PARSING_GUID, '\\','{','3','3','E','2','8','1','3','0','-','4','E','1','E','-','4','6','7','6','-','8','3','5','A','-','9','8','3','9','5','C','3','B','C','3','B','B','}',0};
static const WCHAR SavedGamesParsingNameW[] = {':',':', USERFOLDERS_PARSING_GUID, '\\','{','4','C','5','C','3','2','F','F','-','B','B','9','D','-','4','3','b','0','-','B','5','B','4','-','2','D','7','2','E','5','4','E','A','A','A','4','}',0};
static const WCHAR SavedSearchesParsingNameW[] = {':',':', USERFOLDERS_PARSING_GUID, '\\','{','7','d','1','d','3','a','0','4','-','d','e','b','b','-','4','1','1','5','-','9','5','c','f','-','2','f','2','9','d','a','2','9','2','0','d','a','}',0};
static const WCHAR VideosParsingNameW[] = {':',':', USERFOLDERS_PARSING_GUID, '\\','{','1','8','9','8','9','B','1','D','-','9','9','B','5','-','4','5','5','B','-','8','4','1','C','-','A','B','7','C','7','4','E','4','D','D','F','C','}',0};
static const WCHAR DocumentsLibraryParsingNameW[] = {':',':', USERSLIBRARIES_PARSING_GUID, '\\','{','7','b','0','d','b','1','7','d','-','9','c','d','2','-','4','a','9','3','-','9','7','3','3','-','4','6','c','c','8','9','0','2','2','e','7','c','}',0};
static const WCHAR MusicLibraryParsingNameW[] = {':',':', USERSLIBRARIES_PARSING_GUID, '\\','{','2','1','1','2','A','B','0','A','-','C','8','6','A','-','4','f','f','e','-','A','3','6','8','-','0','D','E','9','6','E','4','7','0','1','2','E','}',0};
static const WCHAR PicturesLibraryParsingNameW[] = {':',':', USERSLIBRARIES_PARSING_GUID, '\\','{','A','9','9','0','A','E','9','F','-','A','0','3','B','-','4','e','8','0','-','9','4','B','C','-','9','9','1','2','D','7','5','0','4','1','0','4','}',0};
static const WCHAR VideosLibraryParsingNameW[] = {':',':', USERSLIBRARIES_PARSING_GUID, '\\','{','4','9','1','E','9','2','2','F','-','5','6','4','3','-','4','a','f','4','-','A','7','E','B','-','4','E','7','A','1','3','8','D','8','1','7','4','}',0};

typedef enum _CSIDL_Type {
    CSIDL_Type_User,
    CSIDL_Type_AllUsers,
    CSIDL_Type_CurrVer,
    CSIDL_Type_Disallowed,
    CSIDL_Type_NonExistent,
    CSIDL_Type_WindowsPath,
    CSIDL_Type_SystemPath,
    CSIDL_Type_SystemX86Path,
    CSIDL_Type_ProgramData,
} CSIDL_Type;

#define CSIDL_CONTACTS         0x0043
#define CSIDL_DOWNLOADS        0x0047
#define CSIDL_LINKS            0x004d
#define CSIDL_APPDATA_LOCALLOW 0x004e
#define CSIDL_SAVED_GAMES      0x0062
#define CSIDL_SEARCHES         0x0063

typedef struct
{
    IApplicationDestinations IApplicationDestinations_iface;
    LONG ref;
} IApplicationDestinationsImpl;

static inline IApplicationDestinationsImpl *impl_from_IApplicationDestinations( IApplicationDestinations *iface )
{
    return CONTAINING_RECORD(iface, IApplicationDestinationsImpl, IApplicationDestinations_iface);
}

static HRESULT WINAPI ApplicationDestinations_QueryInterface(IApplicationDestinations *iface, REFIID riid,
                                                             LPVOID *ppv)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppv);

    if (ppv == NULL)
        return E_POINTER;

    if (IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IApplicationDestinations, riid))
    {
        *ppv = &This->IApplicationDestinations_iface;
        IUnknown_AddRef((IUnknown*)*ppv);

        TRACE("Returning IApplicationDestinations: %p\n", *ppv);
        return S_OK;
    }

    *ppv = NULL;
    FIXME("(%p)->(%s, %p) interface not supported.\n", This, debugstr_guid(riid), ppv);

    return E_NOINTERFACE;
}

static ULONG WINAPI ApplicationDestinations_AddRef(IApplicationDestinations *iface)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", This, ref);

    return ref;
}

static ULONG WINAPI ApplicationDestinations_Release(IApplicationDestinations *iface)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p), new refcount=%i\n", This, ref);

    if (ref == 0)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI ApplicationDestinations_SetAppID(IApplicationDestinations *iface, const WCHAR *appid)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);

    FIXME("(%p, %s) stub!\n", This, debugstr_w(appid));

    return E_NOTIMPL;
}

static HRESULT WINAPI ApplicationDestinations_RemoveDestination(IApplicationDestinations *iface, IUnknown *punk)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);

    FIXME("(%p, %p) stub!\n", This, punk);

    return E_NOTIMPL;
}

static HRESULT WINAPI ApplicationDestinations_RemoveAllDestinations(IApplicationDestinations *iface)
{
    IApplicationDestinationsImpl *This = impl_from_IApplicationDestinations(iface);

    FIXME("(%p) stub!\n", This);

    return E_NOTIMPL;
}

static const IApplicationDestinationsVtbl ApplicationDestinationsVtbl =
{
    ApplicationDestinations_QueryInterface,
    ApplicationDestinations_AddRef,
    ApplicationDestinations_Release,
    ApplicationDestinations_SetAppID,
    ApplicationDestinations_RemoveDestination,
    ApplicationDestinations_RemoveAllDestinations
};

HRESULT WINAPI ApplicationDestinations_Constructor(IUnknown *outer, REFIID riid, LPVOID *ppv)
{
    IApplicationDestinationsImpl *This;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(This = SHAlloc(sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IApplicationDestinations_iface.lpVtbl = &ApplicationDestinationsVtbl;
    This->ref = 0;

    hr = IUnknown_QueryInterface(&This->IApplicationDestinations_iface, riid, ppv);
    if (FAILED(hr))
        SHFree(This);

    return hr;
}

typedef struct
{
    const KNOWNFOLDERID *id;
    CSIDL_Type type;
    LPCWSTR    szValueName;
    LPCWSTR    szDefaultPath; /* fallback string or resource ID */

    /* KNOWNFOLDER_DEFINITION fields */
    KF_CATEGORY category;
    const WCHAR *pszName;
    const WCHAR *pszDescription;
    const KNOWNFOLDERID *fidParent;
    const WCHAR *pszRelativePath;
    const WCHAR *pszParsingName;
    const WCHAR *pszTooltip;
    const WCHAR *pszLocalizedName;
    const WCHAR *pszIcon;
    const WCHAR *pszSecurity;
    DWORD dwAttributes;
    KF_DEFINITION_FLAGS kfdFlags;
    const FOLDERTYPEID *ftidType;
} CSIDL_DATA;

static const CSIDL_DATA CSIDL_Data[] =
{
    { /* 0x00 - CSIDL_DESKTOP */
        &FOLDERID_Desktop,
        CSIDL_Type_User,
        DesktopW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        DesktopW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        DesktopW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x01 - CSIDL_INTERNET */
        &FOLDERID_InternetFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        InternetFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        InternetFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x02 - CSIDL_PROGRAMS */
        &FOLDERID_Programs,
        CSIDL_Type_User,
        ProgramsW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        ProgramsW, /* name */
        NULL, /* description */
        &FOLDERID_StartMenu, /* parent */
        ProgramsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x03 - CSIDL_CONTROLS (.CPL files) */
        &FOLDERID_ControlPanelFolder,
        CSIDL_Type_SystemPath,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        ControlPanelFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        ControlPanelFolderRelativePathW, /* relative path */
        ControlPanelFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x04 - CSIDL_PRINTERS */
        &FOLDERID_PrintersFolder,
        CSIDL_Type_SystemPath,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        PrintersFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        PrintersFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x05 - CSIDL_PERSONAL */
        &FOLDERID_Documents,
        CSIDL_Type_User,
        PersonalW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        PersonalW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        DocumentsW, /* relative path */
        DocumentsParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x06 - CSIDL_FAVORITES */
        &FOLDERID_Favorites,
        CSIDL_Type_User,
        FavoritesW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        FavoritesW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        FavoritesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x07 - CSIDL_STARTUP */
        &FOLDERID_Startup,
        CSIDL_Type_User,
        StartUpW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        StartupW, /* name */
        NULL, /* description */
        &FOLDERID_Programs, /* parent */
        StartUpW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x08 - CSIDL_RECENT */
        &FOLDERID_Recent,
        CSIDL_Type_User,
        RecentW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        RecentW, /* name */
        NULL, /* description */
        &FOLDERID_RoamingAppData, /* parent */
        Microsoft_Windows_RecentW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x09 - CSIDL_SENDTO */
        &FOLDERID_SendTo,
        CSIDL_Type_User,
        SendToW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        SendToW, /* name */
        NULL, /* description */
        &FOLDERID_RoamingAppData, /* parent */
        Microsoft_Windows_SendToW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x0a - CSIDL_BITBUCKET - Recycle Bin */
        &FOLDERID_RecycleBinFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        RecycleBinFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        RecycleBinFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x0b - CSIDL_STARTMENU */
        &FOLDERID_StartMenu,
        CSIDL_Type_User,
        Start_MenuW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        Start_MenuW, /* name */
        NULL, /* description */
        &FOLDERID_RoamingAppData, /* parent */
        Microsoft_Windows_Start_MenuW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x0c - CSIDL_MYDOCUMENTS */
        &GUID_NULL,
        CSIDL_Type_Disallowed, /* matches WinXP--can't get its path */
        NULL,
        NULL
    },
    { /* 0x0d - CSIDL_MYMUSIC */
        &FOLDERID_Music,
        CSIDL_Type_User,
        My_MusicW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        My_MusicW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        MusicW, /* relative path */
        MusicParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x0e - CSIDL_MYVIDEO */
        &FOLDERID_Videos,
        CSIDL_Type_User,
        My_VideosW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        My_VideoW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        VideosW, /* relative path */
        VideosParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x0f - unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
    },
    { /* 0x10 - CSIDL_DESKTOPDIRECTORY */
        &FOLDERID_Desktop,
        CSIDL_Type_User,
        DesktopW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        DesktopW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        DesktopW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x11 - CSIDL_DRIVES */
        &FOLDERID_ComputerFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        MyComputerFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        ComputerFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x12 - CSIDL_NETWORK */
        &FOLDERID_NetworkFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        NetworkPlacesFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NetworkFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x13 - CSIDL_NETHOOD */
        &FOLDERID_NetHood,
        CSIDL_Type_User,
        NetHoodW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        NetHoodW, /* name */
        NULL, /* description */
        &FOLDERID_RoamingAppData, /* parent */
        Microsoft_Windows_Network_ShortcutsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x14 - CSIDL_FONTS */
        &FOLDERID_Fonts,
        CSIDL_Type_WindowsPath,
        FontsW,
        FontsW,

        KF_CATEGORY_FIXED, /* category */
        FontsW, /* name */
        NULL, /* description */
        &FOLDERID_Windows, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &FOLDERID_Windows/* typeid */
    },
    { /* 0x15 - CSIDL_TEMPLATES */
        &FOLDERID_Templates,
        CSIDL_Type_User,
        TemplatesW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        TemplatesW, /* name */
        NULL, /* description */
        &FOLDERID_RoamingAppData, /* parent */
        Microsoft_Windows_TemplatesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x16 - CSIDL_COMMON_STARTMENU */
        &FOLDERID_CommonStartMenu,
        CSIDL_Type_ProgramData,
        Common_Start_MenuW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Common_Start_MenuW, /* name */
        NULL, /* description */
        &FOLDERID_ProgramData, /* parent */
        Microsoft_Windows_Start_MenuW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x17 - CSIDL_COMMON_PROGRAMS */
        &FOLDERID_CommonPrograms,
        CSIDL_Type_ProgramData,
        Common_ProgramsW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Common_ProgramsW, /* name */
        NULL, /* description */
        &FOLDERID_CommonStartMenu, /* parent */
        ProgramsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x18 - CSIDL_COMMON_STARTUP */
        &FOLDERID_CommonStartup,
        CSIDL_Type_ProgramData,
        Common_StartUpW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Common_StartupW, /* name */
        NULL, /* description */
        &FOLDERID_CommonPrograms, /* parent */
        StartUpW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x19 - CSIDL_COMMON_DESKTOPDIRECTORY */
        &FOLDERID_PublicDesktop,
        CSIDL_Type_AllUsers,
        Common_DesktopW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Common_DesktopW, /* name */
        NULL, /* description */
        &FOLDERID_Public, /* parent */
        DesktopW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x1a - CSIDL_APPDATA */
        &FOLDERID_RoamingAppData,
        CSIDL_Type_User,
        AppDataW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        AppDataW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        AppData_RoamingW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x1b - CSIDL_PRINTHOOD */
        &FOLDERID_PrintHood,
        CSIDL_Type_User,
        PrintHoodW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        PrintHoodW, /* name */
        NULL, /* description */
        &FOLDERID_RoamingAppData, /* parent */
        Microsoft_Windows_Printer_ShortcutsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x1c - CSIDL_LOCAL_APPDATA */
        &FOLDERID_LocalAppData,
        CSIDL_Type_User,
        Local_AppDataW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        Local_AppDataW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        AppData_LocalW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_LOCAL_REDIRECT_ONLY | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x1d - CSIDL_ALTSTARTUP */
        &GUID_NULL,
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x1e - CSIDL_COMMON_ALTSTARTUP */
        &GUID_NULL,
        CSIDL_Type_NonExistent,
        NULL,
        NULL
    },
    { /* 0x1f - CSIDL_COMMON_FAVORITES */
        &FOLDERID_Favorites,
        CSIDL_Type_AllUsers,
        Common_FavoritesW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        FavoritesW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        FavoritesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x20 - CSIDL_INTERNET_CACHE */
        &FOLDERID_InternetCache,
        CSIDL_Type_User,
        CacheW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        CacheW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        Microsoft_Windows_INetCacheW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_LOCAL_REDIRECT_ONLY, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x21 - CSIDL_COOKIES */
        &FOLDERID_Cookies,
        CSIDL_Type_User,
        CookiesW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        CookiesW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        Microsoft_Windows_INetCookiesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x22 - CSIDL_HISTORY */
        &FOLDERID_History,
        CSIDL_Type_User,
        HistoryW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        HistoryW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        Microsoft_Windows_HistoryW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_LOCAL_REDIRECT_ONLY, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x23 - CSIDL_COMMON_APPDATA */
        &FOLDERID_ProgramData,
        CSIDL_Type_ProgramData,
        Common_AppDataW,
        NULL,

        KF_CATEGORY_FIXED, /* category */
        Common_AppDataW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x24 - CSIDL_WINDOWS */
        &FOLDERID_Windows,
        CSIDL_Type_WindowsPath,
        NULL,
        NULL,

        KF_CATEGORY_FIXED, /* category */
        WindowsW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x25 - CSIDL_SYSTEM */
        &FOLDERID_System,
        CSIDL_Type_SystemPath,
        NULL,
        NULL,

        KF_CATEGORY_FIXED, /* category */
        SystemW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x26 - CSIDL_PROGRAM_FILES */
        &FOLDERID_ProgramFiles,
        CSIDL_Type_CurrVer,
        ProgramFilesDirW,
        Program_FilesW,

        KF_CATEGORY_FIXED, /* category */
        ProgramFilesW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x27 - CSIDL_MYPICTURES */
        &FOLDERID_Pictures,
        CSIDL_Type_User,
        My_PicturesW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        My_PicturesW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        PicturesW, /* relative path */
        PicturesParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x28 - CSIDL_PROFILE */
        &FOLDERID_Profile,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_FIXED, /* category */
        ProfileW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x29 - CSIDL_SYSTEMX86 */
        &FOLDERID_SystemX86,
        CSIDL_Type_SystemX86Path,
        NULL,
        NULL,

        KF_CATEGORY_FIXED, /* category */
        SystemX86W, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x2a - CSIDL_PROGRAM_FILESX86 */
        &FOLDERID_ProgramFilesX86,
        CSIDL_Type_CurrVer,
        ProgramFilesDirX86W,
        Program_Files_x86W,

        KF_CATEGORY_FIXED, /* category */
        ProgramFilesX86W, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x2b - CSIDL_PROGRAM_FILES_COMMON */
        &FOLDERID_ProgramFilesCommon,
        CSIDL_Type_CurrVer,
        CommonFilesDirW,
        Program_Files_Common_FilesW,

        KF_CATEGORY_FIXED, /* category */
        ProgramFilesCommonW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x2c - CSIDL_PROGRAM_FILES_COMMONX86 */
        &FOLDERID_ProgramFilesCommonX86,
        CSIDL_Type_CurrVer,
        CommonFilesDirX86W,
        Program_Files_x86_Common_FilesW,

        KF_CATEGORY_FIXED, /* category */
        ProgramFilesCommonX86W, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x2d - CSIDL_COMMON_TEMPLATES */
        &FOLDERID_CommonTemplates,
        CSIDL_Type_ProgramData,
        Common_TemplatesW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Common_TemplatesW, /* name */
        NULL, /* description */
        &FOLDERID_ProgramData, /* parent */
        Microsoft_Windows_TemplatesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x2e - CSIDL_COMMON_DOCUMENTS */
        &FOLDERID_PublicDocuments,
        CSIDL_Type_AllUsers,
        Common_DocumentsW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Common_DocumentsW, /* name */
        NULL, /* description */
        &FOLDERID_Public, /* parent */
        DocumentsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x2f - CSIDL_COMMON_ADMINTOOLS */
        &FOLDERID_CommonAdminTools,
        CSIDL_Type_ProgramData,
        Common_Administrative_ToolsW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Common_Administrative_ToolsW, /* name */
        NULL, /* description */
        &FOLDERID_CommonPrograms, /* parent */
        Administrative_ToolsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x30 - CSIDL_ADMINTOOLS */
        &FOLDERID_AdminTools,
        CSIDL_Type_User,
        Administrative_ToolsW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        Administrative_ToolsW, /* name */
        NULL, /* description */
        &FOLDERID_Programs, /* parent */
        Administrative_ToolsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x31 - CSIDL_CONNECTIONS */
        &FOLDERID_ConnectionsFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        ConnectionsFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        Administrative_ToolsW, /* relative path */
        ConnectionsFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x32 - unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x33 - unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x34 - unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x35 - CSIDL_COMMON_MUSIC */
        &FOLDERID_PublicMusic,
        CSIDL_Type_AllUsers,
        CommonMusicW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        CommonMusicW, /* name */
        NULL, /* description */
        &FOLDERID_Public, /* parent */
        MusicW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x36 - CSIDL_COMMON_PICTURES */
        &FOLDERID_PublicPictures,
        CSIDL_Type_AllUsers,
        CommonPicturesW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        CommonPicturesW, /* name */
        NULL, /* description */
        &FOLDERID_Public, /* parent */
        PicturesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x37 - CSIDL_COMMON_VIDEO */
        &FOLDERID_PublicVideos,
        CSIDL_Type_AllUsers,
        CommonVideoW,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        CommonVideoW, /* name */
        NULL, /* description */
        &FOLDERID_Public, /* parent */
        VideosW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x38 - CSIDL_RESOURCES */
        &FOLDERID_ResourceDir,
        CSIDL_Type_WindowsPath,
        NULL,
        ResourcesW,

        KF_CATEGORY_FIXED, /* category */
        ResourceDirW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x39 - CSIDL_RESOURCES_LOCALIZED */
        &FOLDERID_LocalizedResourcesDir,
        CSIDL_Type_NonExistent,
        NULL,
        NULL,

        KF_CATEGORY_FIXED, /* category */
        LocalizedResourcesDirW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x3a - CSIDL_COMMON_OEM_LINKS */
        &FOLDERID_CommonOEMLinks,
        CSIDL_Type_ProgramData,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        OEM_LinksW, /* name */
        NULL, /* description */
        &FOLDERID_ProgramData, /* parent */
        OEM_LinksW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x3b - CSIDL_CDBURN_AREA */
        &FOLDERID_CDBurning,
        CSIDL_Type_User,
        CD_BurningW,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        CD_BurningW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        Microsoft_Windows_Burn_BurnW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_LOCAL_REDIRECT_ONLY, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x3c unassigned */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL
    },
    { /* 0x3d - CSIDL_COMPUTERSNEARME */
        &GUID_NULL,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL
    },
    { /* 0x3e - CSIDL_PROFILES */
        &GUID_NULL,
        CSIDL_Type_Disallowed, /* oddly, this matches WinXP */
        NULL,
        NULL
    },
    { /* 0x3f */
        &FOLDERID_AddNewPrograms,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        AddNewProgramsFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        AddNewProgramsParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x40 */
        &FOLDERID_AppUpdates,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        AppUpdatesFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        AppUpdatesParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x41 */
        &FOLDERID_ChangeRemovePrograms,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        ChangeRemoveProgramsFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        ChangeRemoveProgramsParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x42 */
        &FOLDERID_ConflictFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        ConflictFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        ConflictFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x43 - CSIDL_CONTACTS */
        &FOLDERID_Contacts,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        ContactsW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        ContactsW, /* relative path */
        ContactsParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x44 */
        &FOLDERID_DeviceMetadataStore,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Device_Metadata_StoreW, /* name */
        NULL, /* description */
        &FOLDERID_ProgramData, /* parent */
        Microsoft_Windows_DeviceMetadataStoreW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x45 */
        &GUID_NULL,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,
    },
    { /* 0x46 */
        &FOLDERID_DocumentsLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        DocumentsLibraryW, /* name */
        NULL, /* description */
        &FOLDERID_Libraries, /* parent */
        Documents_librarymsW, /* relative path */
        DocumentsLibraryParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE | KFDF_STREAM, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x47 - CSIDL_DOWNLOADS */
        &FOLDERID_Downloads,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        DownloadsW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        DownloadsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x48 */
        &FOLDERID_Games,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        GamesW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        GamesParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x49 */
        &FOLDERID_GameTasks,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        GameTasksW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        Microsoft_Windows_GameExplorerW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_LOCAL_REDIRECT_ONLY, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x4a */
        &FOLDERID_HomeGroup,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        HomeGroupFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        HomeGroupParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x4b */
        &FOLDERID_ImplicitAppShortcuts,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        ImplicitAppShortcutsW, /* name */
        NULL, /* description */
        &FOLDERID_UserPinned, /* parent */
        ImplicitAppShortcutsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x4c */
        &FOLDERID_Libraries,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        LibrariesW, /* name */
        NULL, /* description */
        &FOLDERID_RoamingAppData, /* parent */
        Microsoft_Windows_LibrariesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x4d - CSIDL_LINKS */
        &FOLDERID_Links,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        LinksW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        LinksW, /* relative path */
        LinksParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x4e - CSIDL_APPDATA_LOCALLOW */
        &FOLDERID_LocalAppDataLow,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        LocalAppDataLowW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        AppData_LocalLowW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, /* attributes */
        KFDF_LOCAL_REDIRECT_ONLY | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x4f */
        &FOLDERID_MusicLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        MusicLibraryW, /* name */
        NULL, /* description */
        &FOLDERID_Libraries, /* parent */
        Music_librarymsW, /* relative path */
        MusicLibraryParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE | KFDF_STREAM, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x50 */
        &FOLDERID_OriginalImages,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        Original_ImagesW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        Microsoft_Windows_Photo_Gallery_Original_ImagesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x51 */
        &FOLDERID_PhotoAlbums,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        PhotoAlbumsW, /* name */
        NULL, /* description */
        &FOLDERID_Pictures, /* parent */
        Slide_ShowsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x52 */
        &FOLDERID_PicturesLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        PicturesLibraryW, /* name */
        NULL, /* description */
        &FOLDERID_Libraries, /* parent */
        Pictures_librarymsW, /* relative path */
        PicturesLibraryParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE | KFDF_STREAM, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x53 */
        &FOLDERID_Playlists,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        PlaylistsW, /* name */
        NULL, /* description */
        &FOLDERID_Music, /* parent */
        PlaylistsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x54 */
        &FOLDERID_ProgramFilesX64,
#ifdef _WIN64
        CSIDL_Type_CurrVer,
        ProgramFilesDirW,
        Program_FilesW,
#else
        CSIDL_Type_NonExistent,
        NULL,
        NULL,
#endif

        KF_CATEGORY_FIXED, /* category */
        ProgramFilesX64W, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x55 */
        &FOLDERID_ProgramFilesCommonX64,
#ifdef _WIN64
        CSIDL_Type_CurrVer,
        ProgramFilesCommonX64W,
        Program_Files_Common_FilesW,
#else
        CSIDL_Type_NonExistent,
        NULL,
        NULL,
#endif

        KF_CATEGORY_FIXED, /* category */
        ProgramFilesCommonX64W, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x56 */
        &FOLDERID_Public,
        CSIDL_Type_AllUsers,
        NULL,
        NULL,

        KF_CATEGORY_FIXED, /* category */
        PublicW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        PublicParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x57 */
        &FOLDERID_PublicDownloads,
        CSIDL_Type_AllUsers,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        CommonDownloadsW, /* name */
        NULL, /* description */
        &FOLDERID_Public, /* parent */
        DownloadsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x58 */
        &FOLDERID_PublicGameTasks,
        CSIDL_Type_ProgramData,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        PublicGameTasksW, /* name */
        NULL, /* description */
        &FOLDERID_ProgramData, /* parent */
        Microsoft_Windows_GameExplorerW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_LOCAL_REDIRECT_ONLY, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x59 */
        &FOLDERID_PublicLibraries,
        CSIDL_Type_AllUsers,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        PublicLibrariesW, /* name */
        NULL, /* description */
        &FOLDERID_Public, /* parent */
        LibrariesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x5a */
        &FOLDERID_PublicRingtones,
        CSIDL_Type_ProgramData,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        CommonRingtonesW, /* name */
        NULL, /* description */
        &FOLDERID_ProgramData, /* parent */
        Microsoft_Windows_RingtonesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x5b */
        &FOLDERID_QuickLaunch,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        Quick_LaunchW, /* name */
        NULL, /* description */
        &FOLDERID_RoamingAppData, /* parent */
        Microsoft_Internet_Explorer_Quick_LaunchW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x5c */
        &FOLDERID_RecordedTVLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        RecordedTVLibraryW, /* name */
        NULL, /* description */
        &FOLDERID_PublicLibraries, /* parent */
        RecordedTV_librarymsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE | KFDF_STREAM, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x5d */
        &FOLDERID_Ringtones,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        RingtonesW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        Microsoft_Windows_RingtonesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x5e */
        &FOLDERID_SampleMusic,
        CSIDL_Type_AllUsers,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        SampleMusicW, /* name */
        NULL, /* description */
        &FOLDERID_PublicMusic, /* parent */
        Sample_MusicW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x5f */
        &FOLDERID_SamplePictures,
        CSIDL_Type_AllUsers,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        SamplePicturesW, /* name */
        NULL, /* description */
        &FOLDERID_PublicPictures, /* parent */
        Sample_PicturesW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x60 */
        &FOLDERID_SamplePlaylists,
        CSIDL_Type_AllUsers,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        SamplePlaylistsW, /* name */
        NULL, /* description */
        &FOLDERID_PublicMusic, /* parent */
        Sample_PlaylistsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x61 */
        &FOLDERID_SampleVideos,
        CSIDL_Type_AllUsers,
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        SampleVideosW, /* name */
        NULL, /* description */
        &FOLDERID_PublicVideos, /* parent */
        Sample_VideosW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x62 - CSIDL_SAVED_GAMES */
        &FOLDERID_SavedGames,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        SavedGamesW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        Saved_GamesW, /* relative path */
        SavedGamesParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_ROAMABLE | KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x63 - CSIDL_SEARCHES */
        &FOLDERID_SavedSearches,
        CSIDL_Type_User,
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        SearchesW, /* name */
        NULL, /* description */
        &FOLDERID_Profile, /* parent */
        SearchesW, /* relative path */
        SavedSearchesParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE | KFDF_PUBLISHEXPANDEDPATH, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x64 */
        &FOLDERID_SEARCH_CSC,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        CSCFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        SEARCH_CSCParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x65 */
        &FOLDERID_SEARCH_MAPI,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        MAPIFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        SEARCH_MAPIParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x66 */
        &FOLDERID_SearchHome,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        SearchHomeFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        SearchHomeParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x67 */
        &FOLDERID_SidebarDefaultParts,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_COMMON, /* category */
        Default_GadgetsW, /* name */
        NULL, /* description */
        &FOLDERID_ProgramFiles, /* parent */
        Windows_Sidebar_GadgetsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x68 */
        &FOLDERID_SidebarParts,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        GadgetsW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        Microsoft_Windows_Sidebar_GadgetsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x69 */
        &FOLDERID_SyncManagerFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        SyncCenterFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        SyncManagerFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x6a */
        &FOLDERID_SyncResultsFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        SyncResultsFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        SyncResultsFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x6b */
        &FOLDERID_SyncSetupFolder,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        SyncSetupFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        SyncSetupFolderParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x6c */
        &FOLDERID_UserPinned,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        User_PinnedW, /* name */
        NULL, /* description */
        &FOLDERID_QuickLaunch, /* parent */
        User_PinnedW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_HIDDEN, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x6d */
        &FOLDERID_UserProfiles,
        CSIDL_Type_CurrVer,
        UsersW,
        UsersW,

        KF_CATEGORY_FIXED, /* category */
        UserProfilesW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        FILE_ATTRIBUTE_READONLY, /* attributes */
        KFDF_PRECREATE, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x6e */
        &FOLDERID_UserProgramFiles,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        UserProgramFilesW, /* name */
        NULL, /* description */
        &FOLDERID_LocalAppData, /* parent */
        ProgramsW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x6f */
        &FOLDERID_UserProgramFilesCommon,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        UserProgramFilesCommonW, /* name */
        NULL, /* description */
        &FOLDERID_UserProgramFiles, /* parent */
        CommonW, /* relative path */
        NULL, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x70 */
        &FOLDERID_UsersFiles,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        UsersFilesFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        UsersFilesParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x71 */
        &FOLDERID_UsersLibraries,
        CSIDL_Type_Disallowed,
        NULL,
        NULL,

        KF_CATEGORY_VIRTUAL, /* category */
        UsersLibrariesFolderW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        NULL, /* relative path */
        UsersLibrariesParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    },
    { /* 0x72 */
        &FOLDERID_VideosLibrary,
        CSIDL_Type_Disallowed, /* FIXME */
        NULL,
        NULL,

        KF_CATEGORY_PERUSER, /* category */
        VideosLibraryW, /* name */
        NULL, /* description */
        &GUID_NULL, /* parent */
        Videos_librarymsW, /* relative path */
        VideosLibraryParsingNameW, /* parsing */
        NULL, /* tooltip */
        NULL, /* localized */
        NULL, /* icon */
        NULL, /* security */
        0, /* attributes */
        0, /* flags */
        &GUID_NULL /* typeid */
    }
};

static int csidl_from_id( const KNOWNFOLDERID *id )
{
    int i;
    for (i = 0; i < ARRAY_SIZE(CSIDL_Data); i++)
        if (IsEqualGUID( CSIDL_Data[i].id, id )) return i;
    return -1;
}

static HRESULT _SHExpandEnvironmentStrings(LPCWSTR szSrc, LPWSTR szDest);

/* Gets the value named value from the registry key
 * rootKey\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders
 * (or from rootKey\userPrefix\... if userPrefix is not NULL) into path, which
 * is assumed to be MAX_PATH WCHARs in length.
 * If it exists, expands the value and writes the expanded value to
 * rootKey\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders
 * Returns successful error code if the value was retrieved from the registry,
 * and a failure otherwise.
 */
static HRESULT _SHGetUserShellFolderPath(HKEY rootKey, LPCWSTR userPrefix,
 LPCWSTR value, LPWSTR path)
{
    HRESULT hr;
    WCHAR shellFolderPath[MAX_PATH], userShellFolderPath[MAX_PATH];
    LPCWSTR pShellFolderPath, pUserShellFolderPath;
    HKEY userShellFolderKey, shellFolderKey;
    DWORD dwType, dwPathLen;

    TRACE("%p,%s,%s,%p\n",rootKey, debugstr_w(userPrefix), debugstr_w(value),
     path);

    if (userPrefix)
    {
        strcpyW(shellFolderPath, userPrefix);
        PathAddBackslashW(shellFolderPath);
        strcatW(shellFolderPath, szSHFolders);
        pShellFolderPath = shellFolderPath;
        strcpyW(userShellFolderPath, userPrefix);
        PathAddBackslashW(userShellFolderPath);
        strcatW(userShellFolderPath, szSHUserFolders);
        pUserShellFolderPath = userShellFolderPath;
    }
    else
    {
        pUserShellFolderPath = szSHUserFolders;
        pShellFolderPath = szSHFolders;
    }

    if (RegCreateKeyW(rootKey, pShellFolderPath, &shellFolderKey))
    {
        TRACE("Failed to create %s\n", debugstr_w(pShellFolderPath));
        return E_FAIL;
    }
    if (RegCreateKeyW(rootKey, pUserShellFolderPath, &userShellFolderKey))
    {
        TRACE("Failed to create %s\n",
         debugstr_w(pUserShellFolderPath));
        RegCloseKey(shellFolderKey);
        return E_FAIL;
    }

    dwPathLen = MAX_PATH * sizeof(WCHAR);
    if (!RegQueryValueExW(userShellFolderKey, value, NULL, &dwType,
     (LPBYTE)path, &dwPathLen) && (dwType == REG_EXPAND_SZ || dwType == REG_SZ))
    {
        LONG ret;

        path[dwPathLen / sizeof(WCHAR)] = '\0';
        if (dwType == REG_EXPAND_SZ && path[0] == '%')
        {
            WCHAR szTemp[MAX_PATH];

            _SHExpandEnvironmentStrings(path, szTemp);
            lstrcpynW(path, szTemp, MAX_PATH);
        }
        ret = RegSetValueExW(shellFolderKey, value, 0, REG_SZ, (LPBYTE)path,
         (strlenW(path) + 1) * sizeof(WCHAR));
        if (ret != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(ret);
        else
            hr = S_OK;
    }
    else
        hr = E_FAIL;
    RegCloseKey(shellFolderKey);
    RegCloseKey(userShellFolderKey);
    TRACE("returning 0x%08x\n", hr);
    return hr;
}

static void append_relative_path(BYTE folder, WCHAR *pszPath)
{
    if (CSIDL_Data[folder].pszRelativePath)
    {
        PathAddBackslashW(pszPath);
        strcatW(pszPath, CSIDL_Data[folder].pszRelativePath);
    }
    else if (CSIDL_Data[folder].szDefaultPath)
    {
        PathAddBackslashW(pszPath);
        strcatW(pszPath, CSIDL_Data[folder].szDefaultPath);
    }
}

/* Gets a 'semi-expanded' default value of the CSIDL with index folder into
 * pszPath, based on the entries in CSIDL_Data.  By semi-expanded, I mean:
 * - Depending on the entry's type, the path may begin with an (unexpanded)
 *   environment variable name.  The caller is responsible for expanding
 *   environment strings if so desired.
 *   The types that are prepended with environment variables are:
 *   CSIDL_Type_User:     %USERPROFILE%
 *   CSIDL_Type_AllUsers: %ALLUSERSPROFILE%
 *   CSIDL_Type_CurrVer:  %SystemDrive%
 *   (Others might make sense too, but as yet are unneeded.)
 */
static HRESULT _SHGetDefaultValue(BYTE folder, LPWSTR pszPath)
{
    HRESULT hr;

    TRACE("0x%02x,%p\n", folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;

    if (!pszPath)
        return E_INVALIDARG;

    if (!is_win64)
    {
        BOOL is_wow64;

        switch (folder)
        {
        case CSIDL_PROGRAM_FILES:
        case CSIDL_PROGRAM_FILESX86:
            IsWow64Process( GetCurrentProcess(), &is_wow64 );
            folder = is_wow64 ? CSIDL_PROGRAM_FILESX86 : CSIDL_PROGRAM_FILES;
            break;
        case CSIDL_PROGRAM_FILES_COMMON:
        case CSIDL_PROGRAM_FILES_COMMONX86:
            IsWow64Process( GetCurrentProcess(), &is_wow64 );
            folder = is_wow64 ? CSIDL_PROGRAM_FILES_COMMONX86 : CSIDL_PROGRAM_FILES_COMMON;
            break;
        }
    }

    if (IsEqualGUID(CSIDL_Data[folder].fidParent, &GUID_NULL))
    {
        /* hit the root, sub in env var */
        switch (CSIDL_Data[folder].type)
        {
            case CSIDL_Type_User:
                strcpyW(pszPath, UserProfileW);
                break;
            case CSIDL_Type_AllUsers:
                strcpyW(pszPath, PublicProfileW);
                break;
            case CSIDL_Type_ProgramData:
                strcpyW(pszPath, ProgramDataVarW);
                break;
            case CSIDL_Type_CurrVer:
                strcpyW(pszPath, SystemDriveW);
                break;
            default:
                ; /* no corresponding env. var, do nothing */
        }
        hr = S_OK;
    }else{
        /* prepend with parent */
        hr = _SHGetDefaultValue(csidl_from_id(CSIDL_Data[folder].fidParent), pszPath);
    }

    if (SUCCEEDED(hr))
        append_relative_path(folder, pszPath);

    TRACE("returning 0x%08x\n", hr);
    return hr;
}

/* Gets the (unexpanded) value of the folder with index folder into pszPath.
 * The folder's type is assumed to be CSIDL_Type_CurrVer.  Its default value
 * can be overridden in the HKLM\\szCurrentVersion key.
 * If dwFlags has SHGFP_TYPE_DEFAULT set or if the value isn't overridden in
 * the registry, uses _SHGetDefaultValue to get the value.
 */
static HRESULT _SHGetCurrentVersionPath(DWORD dwFlags, BYTE folder,
 LPWSTR pszPath)
{
    HRESULT hr;

    TRACE("0x%08x,0x%02x,%p\n", dwFlags, folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_CurrVer)
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
        hr = _SHGetDefaultValue(folder, pszPath);
    else
    {
        HKEY hKey;

        if (RegCreateKeyW(HKEY_LOCAL_MACHINE, szCurrentVersion, &hKey))
            hr = E_FAIL;
        else
        {
            DWORD dwType, dwPathLen = MAX_PATH * sizeof(WCHAR);

            if (RegQueryValueExW(hKey, CSIDL_Data[folder].szValueName, NULL,
             &dwType, (LPBYTE)pszPath, &dwPathLen) ||
             (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
            {
                hr = _SHGetDefaultValue(folder, pszPath);
                dwType = REG_EXPAND_SZ;
                switch (folder)
                {
                case CSIDL_PROGRAM_FILESX86:
                case CSIDL_PROGRAM_FILES_COMMONX86:
                    /* these two should never be set on 32-bit setups */
                    if (!is_win64)
                    {
                        BOOL is_wow64;
                        IsWow64Process( GetCurrentProcess(), &is_wow64 );
                        if (!is_wow64) break;
                    }
                    /* fall through */
                default:
                    RegSetValueExW(hKey, CSIDL_Data[folder].szValueName, 0, dwType,
                                   (LPBYTE)pszPath, (strlenW(pszPath)+1)*sizeof(WCHAR));
                }
            }
            else
            {
                pszPath[dwPathLen / sizeof(WCHAR)] = '\0';
                hr = S_OK;
            }
            RegCloseKey(hKey);
        }
    }
    TRACE("returning 0x%08x (output path is %s)\n", hr, debugstr_w(pszPath));
    return hr;
}

static LPWSTR _GetUserSidStringFromToken(HANDLE Token)
{
    char InfoBuffer[64];
    PTOKEN_USER UserInfo;
    DWORD InfoSize;
    LPWSTR SidStr;

    UserInfo = (PTOKEN_USER) InfoBuffer;
    if (! GetTokenInformation(Token, TokenUser, InfoBuffer, sizeof(InfoBuffer),
                              &InfoSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return NULL;
        UserInfo = heap_alloc(InfoSize);
        if (UserInfo == NULL)
            return NULL;
        if (! GetTokenInformation(Token, TokenUser, UserInfo, InfoSize,
                                  &InfoSize))
        {
            heap_free(UserInfo);
            return NULL;
        }
    }

    if (! ConvertSidToStringSidW(UserInfo->User.Sid, &SidStr))
        SidStr = NULL;

    if (UserInfo != (PTOKEN_USER) InfoBuffer)
        heap_free(UserInfo);

    return SidStr;
}

/* Gets the user's path (unexpanded) for the CSIDL with index folder:
 * If SHGFP_TYPE_DEFAULT is set, calls _SHGetDefaultValue for it.  Otherwise
 * calls _SHGetUserShellFolderPath for it.  Where it looks depends on hToken:
 * - if hToken is -1, looks in HKEY_USERS\.Default
 * - otherwise looks first in HKEY_CURRENT_USER, followed by HKEY_LOCAL_MACHINE
 *   if HKEY_CURRENT_USER doesn't contain any entries.  If both fail, finally
 *   calls _SHGetDefaultValue for it.
 */
static HRESULT _SHGetUserProfilePath(HANDLE hToken, DWORD dwFlags, BYTE folder,
 LPWSTR pszPath)
{
    const WCHAR *szValueName;
    WCHAR buffer[40];
    HRESULT hr;

    TRACE("%p,0x%08x,0x%02x,%p\n", hToken, dwFlags, folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_User)
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
    {
        if (hToken != NULL && hToken != (HANDLE)-1)
        {
            FIXME("unsupported for user other than current or default\n");
            return E_FAIL;
        }
        hr = _SHGetDefaultValue(folder, pszPath);
    }
    else
    {
        LPCWSTR userPrefix = NULL;
        HKEY hRootKey;

        if (hToken == (HANDLE)-1)
        {
            hRootKey = HKEY_USERS;
            userPrefix = DefaultW;
        }
        else if (hToken == NULL)
            hRootKey = HKEY_CURRENT_USER;
        else
        {
            hRootKey = HKEY_USERS;
            userPrefix = _GetUserSidStringFromToken(hToken);
            if (userPrefix == NULL)
            {
                hr = E_FAIL;
                goto error;
            }
        }

        /* For CSIDL_Type_User we also use the GUID if no szValueName is provided */
        szValueName = CSIDL_Data[folder].szValueName;
        if (!szValueName)
        {
            StringFromGUID2( CSIDL_Data[folder].id, buffer, 39 );
            szValueName = &buffer[0];
        }

        hr = _SHGetUserShellFolderPath(hRootKey, userPrefix, szValueName, pszPath);
        if (FAILED(hr) && hRootKey != HKEY_LOCAL_MACHINE)
            hr = _SHGetUserShellFolderPath(HKEY_LOCAL_MACHINE, NULL, szValueName, pszPath);
        if (FAILED(hr))
            hr = _SHGetDefaultValue(folder, pszPath);
        if (userPrefix != NULL && userPrefix != DefaultW)
            LocalFree((HLOCAL) userPrefix);
    }
error:
    TRACE("returning 0x%08x (output path is %s)\n", hr, debugstr_w(pszPath));
    return hr;
}

/* Gets the (unexpanded) path for the CSIDL with index folder.  If dwFlags has
 * SHGFP_TYPE_DEFAULT set, calls _SHGetDefaultValue.  Otherwise calls
 * _SHGetUserShellFolderPath for it, looking only in HKEY_LOCAL_MACHINE.
 * If this fails, falls back to _SHGetDefaultValue.
 */
static HRESULT _SHGetAllUsersProfilePath(DWORD dwFlags, BYTE folder,
 LPWSTR pszPath)
{
    HRESULT hr;

    TRACE("0x%08x,0x%02x,%p\n", dwFlags, folder, pszPath);

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if (CSIDL_Data[folder].type != CSIDL_Type_AllUsers && CSIDL_Data[folder].type != CSIDL_Type_ProgramData)
        return E_INVALIDARG;
    if (!pszPath)
        return E_INVALIDARG;

    if (dwFlags & SHGFP_TYPE_DEFAULT)
        hr = _SHGetDefaultValue(folder, pszPath);
    else
    {
        hr = _SHGetUserShellFolderPath(HKEY_LOCAL_MACHINE, NULL,
         CSIDL_Data[folder].szValueName, pszPath);
        if (FAILED(hr))
            hr = _SHGetDefaultValue(folder, pszPath);
    }
    TRACE("returning 0x%08x (output path is %s)\n", hr, debugstr_w(pszPath));
    return hr;
}

static HRESULT _SHOpenProfilesKey(PHKEY pKey)
{
    LONG lRet;
    DWORD disp;

    lRet = RegCreateKeyExW(HKEY_LOCAL_MACHINE, ProfileListW, 0, NULL, 0,
     KEY_ALL_ACCESS, NULL, pKey, &disp);
    return HRESULT_FROM_WIN32(lRet);
}

/* Reads the value named szValueName from the key profilesKey (assumed to be
 * opened by _SHOpenProfilesKey) into szValue, which is assumed to be MAX_PATH
 * WCHARs in length.  If it doesn't exist, returns szDefault (and saves
 * szDefault to the registry).
 */
static HRESULT _SHGetProfilesValue(HKEY profilesKey, LPCWSTR szValueName,
 LPWSTR szValue, LPCWSTR szDefault)
{
    HRESULT hr;
    DWORD type, dwPathLen = MAX_PATH * sizeof(WCHAR);
    LONG lRet;

    TRACE("%p,%s,%p,%s\n", profilesKey, debugstr_w(szValueName), szValue,
     debugstr_w(szDefault));
    lRet = RegQueryValueExW(profilesKey, szValueName, NULL, &type,
     (LPBYTE)szValue, &dwPathLen);
    if (!lRet && (type == REG_SZ || type == REG_EXPAND_SZ) && dwPathLen
     && *szValue)
    {
        dwPathLen /= sizeof(WCHAR);
        szValue[dwPathLen] = '\0';
        hr = S_OK;
    }
    else
    {
        /* Missing or invalid value, set a default */
        lstrcpynW(szValue, szDefault, MAX_PATH);
        TRACE("Setting missing value %s to %s\n", debugstr_w(szValueName),
                                                  debugstr_w(szValue));
        lRet = RegSetValueExW(profilesKey, szValueName, 0, REG_EXPAND_SZ,
                              (LPBYTE)szValue,
                              (strlenW(szValue) + 1) * sizeof(WCHAR));
        if (lRet)
            hr = HRESULT_FROM_WIN32(lRet);
        else
            hr = S_OK;
    }
    TRACE("returning 0x%08x (output value is %s)\n", hr, debugstr_w(szValue));
    return hr;
}

/* Attempts to expand environment variables from szSrc into szDest, which is
 * assumed to be MAX_PATH characters in length.  Before referring to the
 * environment, handles a few variables directly, because the environment
 * variables may not be set when this is called (as during Wine's installation
 * when default values are being written to the registry).
 * The directly handled environment variables, and their source, are:
 * - ALLUSERSPROFILE, USERPROFILE: reads from the registry
 * - SystemDrive: uses GetSystemDirectoryW and uses the drive portion of its
 *   path
 * If one of the directly handled environment variables is expanded, only
 * expands a single variable, and only in the beginning of szSrc.
 */
static HRESULT _SHExpandEnvironmentStrings(LPCWSTR szSrc, LPWSTR szDest)
{
    HRESULT hr;
    WCHAR szTemp[MAX_PATH], szProfilesPrefix[MAX_PATH] = { 0 };
    HKEY key = NULL;

    TRACE("%s, %p\n", debugstr_w(szSrc), szDest);

    if (!szSrc || !szDest) return E_INVALIDARG;

    /* short-circuit if there's nothing to expand */
    if (szSrc[0] != '%')
    {
        strcpyW(szDest, szSrc);
        hr = S_OK;
        goto end;
    }
    /* Get the profile prefix, we'll probably be needing it */
    hr = _SHOpenProfilesKey(&key);
    if (SUCCEEDED(hr))
    {
        WCHAR def_val[MAX_PATH];

        /* get the system drive */
        GetSystemDirectoryW(def_val, MAX_PATH);
        strcpyW( def_val + 3, szDefaultProfileDirW );

        hr = _SHGetProfilesValue(key, ProfilesDirectoryW, szProfilesPrefix, def_val );
    }

    *szDest = 0;
    strcpyW(szTemp, szSrc);
    while (SUCCEEDED(hr) && szTemp[0] == '%')
    {
        if (!strncmpiW(szTemp, AllUsersProfileW, strlenW(AllUsersProfileW)))
        {
            WCHAR szAllUsers[MAX_PATH], def_val[MAX_PATH];

            GetSystemDirectoryW(def_val, MAX_PATH);
            strcpyW( def_val + 3, UsersPublicW );

            hr = _SHGetProfilesValue(key, PublicW, szAllUsers, def_val);
            PathAppendW(szDest, szAllUsers);
            PathAppendW(szDest, szTemp + strlenW(AllUsersProfileW));
        }
        else if (!strncmpiW(szTemp, PublicProfileW, strlenW(PublicProfileW)))
        {
            WCHAR szAllUsers[MAX_PATH], def_val[MAX_PATH];

            GetSystemDirectoryW(def_val, MAX_PATH);
            strcpyW( def_val + 3, UsersPublicW );

            hr = _SHGetProfilesValue(key, PublicW, szAllUsers, def_val);
            PathAppendW(szDest, szAllUsers);
            PathAppendW(szDest, szTemp + strlenW(PublicProfileW));
        }
        else if (!strncmpiW(szTemp, ProgramDataVarW, strlenW(ProgramDataVarW)))
        {
            WCHAR szProgramData[MAX_PATH], def_val[MAX_PATH];
            HKEY shellFolderKey;
            DWORD dwType, dwPathLen = sizeof(def_val);
            BOOL in_registry = FALSE;

            if (!RegCreateKeyW(HKEY_LOCAL_MACHINE, szSHFolders, &shellFolderKey))
            {
                if (!RegQueryValueExW(shellFolderKey, Common_AppDataW, NULL, &dwType,
                    (LPBYTE)def_val, &dwPathLen) && (dwType == REG_EXPAND_SZ || dwType == REG_SZ))
                    in_registry = TRUE;

                RegCloseKey(shellFolderKey);
            }

            if (!in_registry)
            {
                GetSystemDirectoryW(def_val, MAX_PATH);
                strcpyW( def_val + 3, ProgramDataW );
            }

            hr = _SHGetProfilesValue(key, ProgramDataW, szProgramData, def_val);
            PathAppendW(szDest, szProgramData);
            PathAppendW(szDest, szTemp + strlenW(ProgramDataVarW));
        }
        else if (!strncmpiW(szTemp, UserProfileW, strlenW(UserProfileW)))
        {
            WCHAR userName[MAX_PATH];
            DWORD userLen = MAX_PATH;

            strcpyW(szDest, szProfilesPrefix);
            GetUserNameW(userName, &userLen);
            PathAppendW(szDest, userName);
            PathAppendW(szDest, szTemp + strlenW(UserProfileW));
        }
        else if (!strncmpiW(szTemp, SystemDriveW, strlenW(SystemDriveW)))
        {
            GetSystemDirectoryW(szDest, MAX_PATH);
            strcpyW(szDest + 3, szTemp + strlenW(SystemDriveW) + 1);
        }
        else
        {
            DWORD ret = ExpandEnvironmentStringsW(szTemp, szDest, MAX_PATH);

            if (ret > MAX_PATH)
                hr = E_NOT_SUFFICIENT_BUFFER;
            else if (ret == 0)
                hr = HRESULT_FROM_WIN32(GetLastError());
            else if (!strcmpW( szTemp, szDest )) break;  /* nothing expanded */
        }
        if (SUCCEEDED(hr)) strcpyW(szTemp, szDest);
    }
end:
    if (key)
        RegCloseKey(key);
    TRACE("returning 0x%08x (input was %s, output is %s)\n", hr,
     debugstr_w(szSrc), debugstr_w(szDest));
    return hr;
}

/*************************************************************************
 * _SHGetXDGUserDirs  [Internal]
 *
 * Get XDG directories paths from XDG configuration.
 *
 * PARAMS
 *  xdg_dirs    [I] Array of XDG directories to look for.
 *  num_dirs    [I] Number of elements in xdg_dirs.
 *  xdg_results [O] An array of the XDG directories paths.
 */
static inline void _SHGetXDGUserDirs(const char * const *xdg_dirs, const unsigned int num_dirs, char *** xdg_results) {
    HRESULT hr;

    hr = XDG_UserDirLookup(xdg_dirs, num_dirs, xdg_results);
    if (FAILED(hr)) *xdg_results = NULL;
}

/*************************************************************************
 * _SHFreeXDGUserDirs  [Internal]
 *
 * Free resources allocated by XDG_UserDirLookup().
 *
 * PARAMS
 *  num_dirs    [I] Number of elements in xdg_results.
 *  xdg_results [I] An array of the XDG directories paths.
 */
static inline void _SHFreeXDGUserDirs(const unsigned int num_dirs, char ** xdg_results) {
    UINT i;

    if (xdg_results)
    {
        for (i = 0; i < num_dirs; i++)
            heap_free(xdg_results[i]);
        heap_free(xdg_results);
    }
}

/*************************************************************************
 * _SHAppendToUnixPath  [Internal]
 *
 * Helper function for _SHCreateSymbolicLinks. Appends pwszSubPath (or the
 * corresponding resource, if IS_INTRESOURCE) to the unix base path 'szBasePath'
 * and replaces backslashes with slashes.
 *
 * PARAMS
 *  szBasePath  [IO] The unix base path, which will be appended to (CP_UNXICP).
 *  pwszSubPath [I]  Sub-path or resource id (use MAKEINTRESOURCEW).
 *
 * RETURNS
 *  Success: TRUE,
 *  Failure: FALSE
 */
static inline BOOL _SHAppendToUnixPath(char *szBasePath, LPCWSTR pwszSubPath) {
    WCHAR wszSubPath[MAX_PATH];
    int cLen = strlen(szBasePath);
    char *pBackslash;

    if (IS_INTRESOURCE(pwszSubPath)) {
        if (!LoadStringW(shell32_hInstance, LOWORD(pwszSubPath), wszSubPath, MAX_PATH)) {
            /* Fall back to hard coded defaults. */
            switch (LOWORD(pwszSubPath)) {
                case IDS_PERSONAL:
                    lstrcpyW(wszSubPath, DocumentsW);
                    break;
                case IDS_MYMUSIC:
                    lstrcpyW(wszSubPath, My_MusicW);
                    break;
                case IDS_MYPICTURES:
                    lstrcpyW(wszSubPath, My_PicturesW);
                    break;
                case IDS_MYVIDEOS:
                    lstrcpyW(wszSubPath, My_VideosW);
                    break;
                case IDS_DOWNLOADS:
                    lstrcpyW(wszSubPath, DownloadsW);
                    break;
                case IDS_TEMPLATES:
                    lstrcpyW(wszSubPath, TemplatesW);
                    break;
                default:
                    ERR("LoadString(%d) failed!\n", LOWORD(pwszSubPath));
                    return FALSE;
            }
        }
    } else {
        lstrcpyW(wszSubPath, pwszSubPath);
    }

    if (szBasePath[cLen-1] != '/') szBasePath[cLen++] = '/';

    if (!WideCharToMultiByte(CP_UNIXCP, 0, wszSubPath, -1, szBasePath + cLen,
                             FILENAME_MAX - cLen, NULL, NULL))
    {
        return FALSE;
    }

    pBackslash = szBasePath + cLen;
    while ((pBackslash = strchr(pBackslash, '\\'))) *pBackslash = '/';

    return TRUE;
}

/******************************************************************************
 * _SHGetFolderUnixPath  [Internal]
 *
 * Create a shell folder and get its unix path.
 *
 * PARAMS
 *  nFolder [I] CSIDL identifying the folder.
 */
static inline char * _SHGetFolderUnixPath(const int nFolder)
{
    WCHAR wszTempPath[MAX_PATH];
    HRESULT hr;

    hr = SHGetFolderPathW(NULL, nFolder, NULL,
                          SHGFP_TYPE_CURRENT, wszTempPath);
    if (FAILED(hr)) return NULL;

    return wine_get_unix_file_name(wszTempPath);
}

/******************************************************************************
 * _SHCreateMyDocumentsSubDirs  [Internal]
 *
 * Create real directories for various shell folders under 'My Documents'. For
 * Windows and homeless styles. Fails silently for already existing sub dirs.
 *
 * PARAMS
 *  aidsMyStuff      [I] Array of IDS_* resources to create sub dirs for.
 *  num              [I] Number of elements in aidsMyStuff.
 *  szPersonalTarget [I] Unix path to 'My Documents' directory.
 */
static void _SHCreateMyDocumentsSubDirs(const UINT * aidsMyStuff, const UINT num, const char * szPersonalTarget)
{
    char szMyStuffTarget[FILENAME_MAX];
    UINT i;

    if (aidsMyStuff && szPersonalTarget)
    {
        for (i = 0; i < num; i++)
        {
            strcpy(szMyStuffTarget, szPersonalTarget);
            if (_SHAppendToUnixPath(szMyStuffTarget, MAKEINTRESOURCEW(aidsMyStuff[i])))
                mkdir(szMyStuffTarget, 0777);
        }
    }
}

/******************************************************************************
 * _SHCreateMyDocumentsSymbolicLink  [Internal]
 *
 * Sets up a symbolic link for the 'My Documents' shell folder to point into
 * the users home directory.
 *
 * PARAMS
 *  aidsMyStuff [I] Array of IDS_* resources to create sub dirs for.
 *  aids_num    [I] Number of elements in aidsMyStuff.
 */
static void _SHCreateMyDocumentsSymbolicLink(const UINT * aidsMyStuff, const UINT aids_num)
{
    static const char * const xdg_dirs[] = { "DOCUMENTS" };
    static const unsigned int num = ARRAY_SIZE(xdg_dirs);
    char szPersonalTarget[FILENAME_MAX], *pszPersonal;
    struct stat statFolder;
    const char *pszHome;
    char ** xdg_results;

    /* Get the unix path of 'My Documents'. */
    pszPersonal = _SHGetFolderUnixPath(CSIDL_PERSONAL|CSIDL_FLAG_DONT_VERIFY);
    if (!pszPersonal) return;

    _SHGetXDGUserDirs(xdg_dirs, num, &xdg_results);

    pszHome = getenv("HOME");
    if (pszHome && !stat(pszHome, &statFolder) && S_ISDIR(statFolder.st_mode))
    {
        while (1)
        {
            /* Check if there's already a Wine-specific 'My Documents' folder */
            strcpy(szPersonalTarget, pszHome);
            if (_SHAppendToUnixPath(szPersonalTarget, MAKEINTRESOURCEW(IDS_PERSONAL)) &&
                !stat(szPersonalTarget, &statFolder) && S_ISDIR(statFolder.st_mode))
            {
                /* '$HOME/My Documents' exists. Create subfolders for
                 * 'My Pictures', 'My Videos', 'My Music' etc. or fail silently
                 * if they already exist.
                 */
                _SHCreateMyDocumentsSubDirs(aidsMyStuff, aids_num, szPersonalTarget);
                break;
            }

            /* Try to point to the XDG Documents folder */
            if (xdg_results && xdg_results[0] &&
               !stat(xdg_results[0], &statFolder) &&
               S_ISDIR(statFolder.st_mode))
            {
                strcpy(szPersonalTarget, xdg_results[0]);
                break;
            }

            /* Or the hardcoded / OS X Documents folder */
            strcpy(szPersonalTarget, pszHome);
            if (_SHAppendToUnixPath(szPersonalTarget, DocumentsW) &&
               !stat(szPersonalTarget, &statFolder) &&
               S_ISDIR(statFolder.st_mode))
                break;

            /* As a last resort point to $HOME. */
            strcpy(szPersonalTarget, pszHome);
            break;
        }

        /* Create symbolic link to 'My Documents' or fail silently if a directory
         * or symlink exists. */
        symlink(szPersonalTarget, pszPersonal);
    }
    else
    {
        /* '$HOME' doesn't exist. Create subdirs for 'My Pictures', 'My Videos',
         * 'My Music' etc. in '%USERPROFILE%\My Documents' or fail silently if
         * they already exist. */
        pszHome = NULL;
        strcpy(szPersonalTarget, pszPersonal);
        _SHCreateMyDocumentsSubDirs(aidsMyStuff, aids_num, szPersonalTarget);
    }

    heap_free(pszPersonal);

    _SHFreeXDGUserDirs(num, xdg_results);
}

/******************************************************************************
 * _SHCreateMyStuffSymbolicLink  [Internal]
 *
 * Sets up a symbolic link for one of the 'My Whatever' shell folders to point
 * into the users home directory.
 *
 * PARAMS
 *  nFolder [I] CSIDL identifying the folder.
 */
static void _SHCreateMyStuffSymbolicLink(int nFolder)
{
    static const UINT aidsMyStuff[] = {
        IDS_MYPICTURES, IDS_MYVIDEOS, IDS_MYMUSIC, IDS_DOWNLOADS, IDS_TEMPLATES
    };
    static const WCHAR * const MyOSXStuffW[] = {
        PicturesW, MoviesW, MusicW, DownloadsW, TemplatesW
    };
    static const int acsidlMyStuff[] = {
        CSIDL_MYPICTURES, CSIDL_MYVIDEO, CSIDL_MYMUSIC, CSIDL_DOWNLOADS, CSIDL_TEMPLATES
    };
    static const char * const xdg_dirs[] = {
        "PICTURES", "VIDEOS", "MUSIC", "DOWNLOAD", "TEMPLATES"
    };
    static const unsigned int num = ARRAY_SIZE(xdg_dirs);
    char szPersonalTarget[FILENAME_MAX], *pszPersonal;
    char szMyStuffTarget[FILENAME_MAX], *pszMyStuff;
    struct stat statFolder;
    const char *pszHome;
    char ** xdg_results;
    DWORD folder = nFolder & CSIDL_FOLDER_MASK;
    UINT i;

    for (i = 0; i < ARRAY_SIZE(acsidlMyStuff) && acsidlMyStuff[i] != folder; i++);
    if (i >= ARRAY_SIZE(acsidlMyStuff)) return;

    /* Create all necessary profile sub-dirs up to 'My Documents' and get the unix path. */
    pszPersonal = _SHGetFolderUnixPath(CSIDL_PERSONAL|CSIDL_FLAG_CREATE);
    if (!pszPersonal) return;

    strcpy(szPersonalTarget, pszPersonal);
    if (!stat(pszPersonal, &statFolder) && S_ISLNK(statFolder.st_mode))
    {
        int cLen = readlink(pszPersonal, szPersonalTarget, FILENAME_MAX-1);
        if (cLen >= 0) szPersonalTarget[cLen] = '\0';
    }
    heap_free(pszPersonal);

    _SHGetXDGUserDirs(xdg_dirs, num, &xdg_results);

    pszHome = getenv("HOME");

    while (1)
    {
        /* Get the current 'My Whatever' folder unix path. */
        pszMyStuff = _SHGetFolderUnixPath(acsidlMyStuff[i]|CSIDL_FLAG_DONT_VERIFY);
        if (!pszMyStuff) break;

        while (1)
        {
            /* Check for the Wine-specific '$HOME/My Documents' subfolder */
            strcpy(szMyStuffTarget, szPersonalTarget);
            if (_SHAppendToUnixPath(szMyStuffTarget, MAKEINTRESOURCEW(aidsMyStuff[i])) &&
                !stat(szMyStuffTarget, &statFolder) && S_ISDIR(statFolder.st_mode))
                break;

            /* Try the XDG_XXX_DIR folder */
            if (xdg_results && xdg_results[i])
            {
                strcpy(szMyStuffTarget, xdg_results[i]);
                break;
            }

            /* Or the OS X folder (these are never localized) */
            if (pszHome)
            {
                strcpy(szMyStuffTarget, pszHome);
                if (_SHAppendToUnixPath(szMyStuffTarget, MyOSXStuffW[i]) &&
                    !stat(szMyStuffTarget, &statFolder) &&
                    S_ISDIR(statFolder.st_mode))
                    break;
            }

            /* As a last resort point to the same location as 'My Documents' */
            strcpy(szMyStuffTarget, szPersonalTarget);
            break;
        }
        symlink(szMyStuffTarget, pszMyStuff);
        heap_free(pszMyStuff);
        break;
    }

    _SHFreeXDGUserDirs(num, xdg_results);
}

/******************************************************************************
 * _SHCreateDesktopSymbolicLink  [Internal]
 *
 * Sets up a symbolic link for the 'Desktop' shell folder to point into the
 * users home directory.
 */
static void _SHCreateDesktopSymbolicLink(void)
{
    static const char * const xdg_dirs[] = { "DESKTOP" };
    static const unsigned int num = ARRAY_SIZE(xdg_dirs);
    char *pszPersonal;
    char szDesktopTarget[FILENAME_MAX], *pszDesktop;
    struct stat statFolder;
    const char *pszHome;
    char ** xdg_results;
    char * xdg_desktop_dir;

    /* Create all necessary profile sub-dirs up to 'My Documents' and get the unix path. */
    pszPersonal = _SHGetFolderUnixPath(CSIDL_PERSONAL|CSIDL_FLAG_CREATE);
    if (!pszPersonal) return;

    _SHGetXDGUserDirs(xdg_dirs, num, &xdg_results);

    pszHome = getenv("HOME");

    if (pszHome)
        strcpy(szDesktopTarget, pszHome);
    else
        strcpy(szDesktopTarget, pszPersonal);
    heap_free(pszPersonal);

    xdg_desktop_dir = xdg_results ? xdg_results[0] : NULL;
    if (xdg_desktop_dir ||
        (_SHAppendToUnixPath(szDesktopTarget, DesktopW) &&
        !stat(szDesktopTarget, &statFolder) && S_ISDIR(statFolder.st_mode)))
    {
        pszDesktop = _SHGetFolderUnixPath(CSIDL_DESKTOPDIRECTORY|CSIDL_FLAG_DONT_VERIFY);
        if (pszDesktop)
        {
            if (xdg_desktop_dir)
                symlink(xdg_desktop_dir, pszDesktop);
            else
                symlink(szDesktopTarget, pszDesktop);
            heap_free(pszDesktop);
        }
    }

    _SHFreeXDGUserDirs(num, xdg_results);
}

/******************************************************************************
 * _SHCreateSymbolicLink  [Internal]
 *
 * Sets up a symbolic link for one of the special shell folders to point into
 * the users home directory.
 *
 * PARAMS
 *  nFolder [I] CSIDL identifying the folder.
 */
static void _SHCreateSymbolicLink(int nFolder)
{
    static const UINT aidsMyStuff[] = {
        IDS_MYPICTURES, IDS_MYVIDEOS, IDS_MYMUSIC, IDS_DOWNLOADS, IDS_TEMPLATES
    };
    DWORD folder = nFolder & CSIDL_FOLDER_MASK;

    switch (folder) {
        case CSIDL_PERSONAL:
            _SHCreateMyDocumentsSymbolicLink(aidsMyStuff, ARRAY_SIZE(aidsMyStuff));
            break;
        case CSIDL_MYPICTURES:
        case CSIDL_MYVIDEO:
        case CSIDL_MYMUSIC:
        case CSIDL_DOWNLOADS:
        case CSIDL_TEMPLATES:
            _SHCreateMyStuffSymbolicLink(folder);
            break;
        case CSIDL_DESKTOPDIRECTORY:
            _SHCreateDesktopSymbolicLink();
            break;
    }
}

/******************************************************************************
 * _SHCreateSymbolicLinks  [Internal]
 *
 * Sets up symbol links for various shell folders to point into the user's home
 * directory. We do an educated guess about what the user would probably want:
 * - If there is a 'My Documents' directory in $HOME, the user probably wants
 *   wine's 'My Documents' to point there. Furthermore, we infer that the user
 *   is a Windows lover and has no problem with wine creating subfolders for
 *   'My Pictures', 'My Music', 'My Videos' etc. under '$HOME/My Documents', if
 *   those do not already exist. We put appropriate symbolic links in place for
 *   those, too.
 * - If there is no 'My Documents' directory in $HOME, we let 'My Documents'
 *   point directly to $HOME. We assume the user to be a unix hacker who does not
 *   want wine to create anything anywhere besides the .wine directory. So, if
 *   there already is a 'My Music' directory in $HOME, we symlink the 'My Music'
 *   shell folder to it. But if not, then we check XDG_MUSIC_DIR - "well known"
 *   directory, and try to link to that. If that fails, then we symlink to
 *   $HOME directly. The same holds for 'My Pictures', 'My Videos' etc.
 * - The Desktop shell folder is symlinked to XDG_DESKTOP_DIR. If that does not
 *   exist, then we try '$HOME/Desktop'. If that does not exist, then we leave
 *   it alone.
 * ('My Music',... above in fact means LoadString(IDS_MYMUSIC))
 */
static void _SHCreateSymbolicLinks(void)
{
    static const int acsidlMyStuff[] = {
        CSIDL_MYPICTURES, CSIDL_MYVIDEO, CSIDL_MYMUSIC, CSIDL_DOWNLOADS, CSIDL_TEMPLATES, CSIDL_PERSONAL, CSIDL_DESKTOPDIRECTORY
    };
    UINT i;

    for (i=0; i < ARRAY_SIZE(acsidlMyStuff); i++)
        _SHCreateSymbolicLink(acsidlMyStuff[i]);
}

/******************************************************************************
 * SHGetFolderPathW			[SHELL32.@]
 *
 * Convert nFolder to path.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: standard HRESULT error codes.
 *
 * NOTES
 * Most values can be overridden in either
 * HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\User Shell Folders
 * or in the same location in HKLM.
 * The "Shell Folders" registry key was used in NT4 and earlier systems.
 * Beginning with Windows 2000, the "User Shell Folders" key is used, so
 * changes made to it are made to the former key too.  This synchronization is
 * done on-demand: not until someone requests the value of one of these paths
 * (by calling one of the SHGet functions) is the value synchronized.
 * Furthermore, the HKCU paths take precedence over the HKLM paths.
 */
HRESULT WINAPI SHGetFolderPathW(
	HWND hwndOwner,    /* [I] owner window */
	int nFolder,       /* [I] CSIDL identifying the folder */
	HANDLE hToken,     /* [I] access token */
	DWORD dwFlags,     /* [I] which path to return */
	LPWSTR pszPath)    /* [O] converted path */
{
    HRESULT hr =  SHGetFolderPathAndSubDirW(hwndOwner, nFolder, hToken, dwFlags, NULL, pszPath);
    if(HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    return hr;
}

HRESULT WINAPI SHGetFolderPathAndSubDirA(
	HWND hwndOwner,    /* [I] owner window */
	int nFolder,       /* [I] CSIDL identifying the folder */
	HANDLE hToken,     /* [I] access token */
	DWORD dwFlags,     /* [I] which path to return */
	LPCSTR pszSubPath, /* [I] sub directory of the specified folder */
	LPSTR pszPath)     /* [O] converted path */
{
    int length;
    HRESULT hr = S_OK;
    LPWSTR pszSubPathW = NULL;
    LPWSTR pszPathW = NULL;

    TRACE("%p,%#x,%p,%#x,%s,%p\n", hwndOwner, nFolder, hToken, dwFlags, debugstr_a(pszSubPath), pszPath);

    if(pszPath) {
        pszPathW = heap_alloc(MAX_PATH * sizeof(WCHAR));
        if(!pszPathW) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
    }
    TRACE("%08x,%08x,%s\n",nFolder, dwFlags, debugstr_w(pszSubPathW));

    /* SHGetFolderPathAndSubDirW does not distinguish if pszSubPath isn't
     * set (null), or an empty string.therefore call it without the parameter set
     * if pszSubPath is an empty string
     */
    if (pszSubPath && pszSubPath[0]) {
        length = MultiByteToWideChar(CP_ACP, 0, pszSubPath, -1, NULL, 0);
        pszSubPathW = heap_alloc(length * sizeof(WCHAR));
        if(!pszSubPathW) {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto cleanup;
        }
        MultiByteToWideChar(CP_ACP, 0, pszSubPath, -1, pszSubPathW, length);
    }

    hr = SHGetFolderPathAndSubDirW(hwndOwner, nFolder, hToken, dwFlags, pszSubPathW, pszPathW);

    if (SUCCEEDED(hr) && pszPath)
        WideCharToMultiByte(CP_ACP, 0, pszPathW, -1, pszPath, MAX_PATH, NULL, NULL);

cleanup:
    heap_free(pszPathW);
    heap_free(pszSubPathW);
    return hr;
}

/*************************************************************************
 * SHGetFolderPathAndSubDirW		[SHELL32.@]
 */
HRESULT WINAPI SHGetFolderPathAndSubDirW(
	HWND hwndOwner,    /* [I] owner window */
	int nFolder,       /* [I] CSIDL identifying the folder */
	HANDLE hToken,     /* [I] access token */
	DWORD dwFlags,     /* [I] which path to return */
	LPCWSTR pszSubPath,/* [I] sub directory of the specified folder */
	LPWSTR pszPath)    /* [O] converted path */
{
    HRESULT    hr;
    WCHAR      szBuildPath[MAX_PATH], szTemp[MAX_PATH];
    DWORD      folder = nFolder & CSIDL_FOLDER_MASK;
    CSIDL_Type type;
    int        ret;

    TRACE("%p,%#x,%p,%#x,%s,%p\n", hwndOwner, nFolder, hToken, dwFlags, debugstr_w(pszSubPath), pszPath);

    /* Windows always NULL-terminates the resulting path regardless of success
     * or failure, so do so first
     */
    if (pszPath)
        *pszPath = '\0';

    if (folder >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    if ((SHGFP_TYPE_CURRENT != dwFlags) && (SHGFP_TYPE_DEFAULT != dwFlags))
        return E_INVALIDARG;
    szTemp[0] = 0;
    type = CSIDL_Data[folder].type;
    switch (type)
    {
        case CSIDL_Type_Disallowed:
            hr = E_INVALIDARG;
            break;
        case CSIDL_Type_NonExistent:
            hr = S_FALSE;
            break;
        case CSIDL_Type_WindowsPath:
            GetWindowsDirectoryW(szTemp, MAX_PATH);
            append_relative_path(folder, szTemp);
            hr = S_OK;
            break;
        case CSIDL_Type_SystemPath:
            GetSystemDirectoryW(szTemp, MAX_PATH);
            append_relative_path(folder, szTemp);
            hr = S_OK;
            break;
        case CSIDL_Type_SystemX86Path:
            if (!GetSystemWow64DirectoryW(szTemp, MAX_PATH)) GetSystemDirectoryW(szTemp, MAX_PATH);
            append_relative_path(folder, szTemp);
            hr = S_OK;
            break;
        case CSIDL_Type_CurrVer:
            hr = _SHGetCurrentVersionPath(dwFlags, folder, szTemp);
            break;
        case CSIDL_Type_User:
            hr = _SHGetUserProfilePath(hToken, dwFlags, folder, szTemp);
            break;
        case CSIDL_Type_AllUsers:
        case CSIDL_Type_ProgramData:
            hr = _SHGetAllUsersProfilePath(dwFlags, folder, szTemp);
            break;
        default:
            FIXME("bogus type %d, please fix\n", type);
            hr = E_INVALIDARG;
            break;
    }

    /* Expand environment strings if necessary */
    if (*szTemp == '%')
        hr = _SHExpandEnvironmentStrings(szTemp, szBuildPath);
    else
        strcpyW(szBuildPath, szTemp);

    if (FAILED(hr)) goto end;

    if(pszSubPath) {
        /* make sure the new path does not exceed the buffer length
         * and remember to backslash and terminate it */
        if(MAX_PATH < (lstrlenW(szBuildPath) + lstrlenW(pszSubPath) + 2)) {
            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
            goto end;
        }
        PathAppendW(szBuildPath, pszSubPath);
        PathRemoveBackslashW(szBuildPath);
    }
    /* Copy the path if it's available before we might return */
    if (SUCCEEDED(hr) && pszPath)
        strcpyW(pszPath, szBuildPath);

    /* if we don't care about existing directories we are ready */
    if(nFolder & CSIDL_FLAG_DONT_VERIFY) goto end;

    if (PathFileExistsW(szBuildPath)) goto end;

    /* not existing but we are not allowed to create it.  The return value
     * is verified against shell32 version 6.0.
     */
    if (!(nFolder & CSIDL_FLAG_CREATE))
    {
        hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        goto end;
    }

    /* create symbolic links rather than directories for specific
     * user shell folders */
    _SHCreateSymbolicLink(folder);

    /* create directory/directories */
    ret = SHCreateDirectoryExW(hwndOwner, szBuildPath, NULL);
    if (ret && ret != ERROR_ALREADY_EXISTS)
    {
        ERR("Failed to create directory %s.\n", debugstr_w(szBuildPath));
        hr = E_FAIL;
        goto end;
    }

    TRACE("Created missing system directory %s\n", debugstr_w(szBuildPath));
end:
    TRACE("returning 0x%08x (final path is %s)\n", hr, debugstr_w(szBuildPath));
    return hr;
}

/*************************************************************************
 * SHGetFolderPathA			[SHELL32.@]
 *
 * See SHGetFolderPathW.
 */
HRESULT WINAPI SHGetFolderPathA(
	HWND hwndOwner,
	int nFolder,
	HANDLE hToken,
	DWORD dwFlags,
	LPSTR pszPath)
{
    WCHAR szTemp[MAX_PATH];
    HRESULT hr;

    TRACE("%p,%d,%p,%#x,%p\n", hwndOwner, nFolder, hToken, dwFlags, pszPath);

    if (pszPath)
        *pszPath = '\0';
    hr = SHGetFolderPathW(hwndOwner, nFolder, hToken, dwFlags, szTemp);
    if (SUCCEEDED(hr) && pszPath)
        WideCharToMultiByte(CP_ACP, 0, szTemp, -1, pszPath, MAX_PATH, NULL,
         NULL);

    return hr;
}

/* For each folder in folders, if its value has not been set in the registry,
 * calls _SHGetUserProfilePath or _SHGetAllUsersProfilePath (depending on the
 * folder's type) to get the unexpanded value first.
 * Writes the unexpanded value to User Shell Folders, and queries it with
 * SHGetFolderPathW to force the creation of the directory if it doesn't
 * already exist.  SHGetFolderPathW also returns the expanded value, which
 * this then writes to Shell Folders.
 */
static HRESULT _SHRegisterFolders(HKEY hRootKey, HANDLE hToken,
 LPCWSTR szUserShellFolderPath, LPCWSTR szShellFolderPath, const UINT folders[],
 UINT foldersLen)
{
    const WCHAR *szValueName;
    WCHAR buffer[40];
    UINT i;
    WCHAR path[MAX_PATH];
    HRESULT hr = S_OK;
    HKEY hUserKey = NULL, hKey = NULL;
    DWORD dwType, dwPathLen;
    LONG ret;

    TRACE("%p,%p,%s,%p,%u\n", hRootKey, hToken,
     debugstr_w(szUserShellFolderPath), folders, foldersLen);

    ret = RegCreateKeyW(hRootKey, szUserShellFolderPath, &hUserKey);
    if (ret)
        hr = HRESULT_FROM_WIN32(ret);
    else
    {
        ret = RegCreateKeyW(hRootKey, szShellFolderPath, &hKey);
        if (ret)
            hr = HRESULT_FROM_WIN32(ret);
    }
    for (i = 0; SUCCEEDED(hr) && i < foldersLen; i++)
    {
        dwPathLen = MAX_PATH * sizeof(WCHAR);

        /* For CSIDL_Type_User we also use the GUID if no szValueName is provided */
        szValueName = CSIDL_Data[folders[i]].szValueName;
        if (!szValueName && CSIDL_Data[folders[i]].type == CSIDL_Type_User)
        {
            StringFromGUID2( CSIDL_Data[folders[i]].id, buffer, 39 );
            szValueName = &buffer[0];
        }

        if (RegQueryValueExW(hUserKey, szValueName, NULL,
         &dwType, (LPBYTE)path, &dwPathLen) || (dwType != REG_SZ &&
         dwType != REG_EXPAND_SZ))
        {
            *path = '\0';
            if (CSIDL_Data[folders[i]].type == CSIDL_Type_User)
                _SHGetUserProfilePath(hToken, SHGFP_TYPE_DEFAULT, folders[i],
                 path);
            else if (CSIDL_Data[folders[i]].type == CSIDL_Type_AllUsers ||
                     CSIDL_Data[folders[i]].type == CSIDL_Type_ProgramData)
                _SHGetAllUsersProfilePath(SHGFP_TYPE_DEFAULT, folders[i], path);
            else if (CSIDL_Data[folders[i]].type == CSIDL_Type_WindowsPath)
            {
                GetWindowsDirectoryW(path, MAX_PATH);
                append_relative_path(folders[i], path);
            }
            else
                hr = E_FAIL;
            if (*path)
            {
                ret = RegSetValueExW(hUserKey, szValueName, 0, REG_EXPAND_SZ,
                 (LPBYTE)path, (strlenW(path) + 1) * sizeof(WCHAR));
                if (ret)
                    hr = HRESULT_FROM_WIN32(ret);
                else
                {
                    hr = SHGetFolderPathW(NULL, folders[i] | CSIDL_FLAG_CREATE,
                     hToken, SHGFP_TYPE_DEFAULT, path);
                    ret = RegSetValueExW(hKey, szValueName, 0, REG_SZ,
                     (LPBYTE)path, (strlenW(path) + 1) * sizeof(WCHAR));
                    if (ret)
                        hr = HRESULT_FROM_WIN32(ret);
                }
            }
        }
        else
        {
            /* create the default dir, which may be different from the path
             * stored in the registry. */
            SHGetFolderPathW(NULL, folders[i] | CSIDL_FLAG_CREATE,
             hToken, SHGFP_TYPE_DEFAULT, path);
        }
    }
    if (hUserKey)
        RegCloseKey(hUserKey);
    if (hKey)
        RegCloseKey(hKey);

    TRACE("returning 0x%08x\n", hr);
    return hr;
}

static HRESULT _SHRegisterUserShellFolders(BOOL bDefault)
{
    static const UINT folders[] = {
     CSIDL_PROGRAMS,
     CSIDL_PERSONAL,
     CSIDL_FAVORITES,
     CSIDL_APPDATA,
     CSIDL_STARTUP,
     CSIDL_RECENT,
     CSIDL_SENDTO,
     CSIDL_STARTMENU,
     CSIDL_MYMUSIC,
     CSIDL_MYVIDEO,
     CSIDL_DESKTOPDIRECTORY,
     CSIDL_NETHOOD,
     CSIDL_TEMPLATES,
     CSIDL_PRINTHOOD,
     CSIDL_LOCAL_APPDATA,
     CSIDL_INTERNET_CACHE,
     CSIDL_COOKIES,
     CSIDL_HISTORY,
     CSIDL_MYPICTURES,
     CSIDL_FONTS,
     CSIDL_ADMINTOOLS,
     CSIDL_CONTACTS,
     CSIDL_DOWNLOADS,
     CSIDL_LINKS,
     CSIDL_APPDATA_LOCALLOW,
     CSIDL_SAVED_GAMES,
     CSIDL_SEARCHES
    };
    WCHAR userShellFolderPath[MAX_PATH], shellFolderPath[MAX_PATH];
    LPCWSTR pUserShellFolderPath, pShellFolderPath;
    HRESULT hr = S_OK;
    HKEY hRootKey;
    HANDLE hToken;

    TRACE("%s\n", bDefault ? "TRUE" : "FALSE");
    if (bDefault)
    {
        hToken = (HANDLE)-1;
        hRootKey = HKEY_USERS;
        strcpyW(userShellFolderPath, DefaultW);
        PathAddBackslashW(userShellFolderPath);
        strcatW(userShellFolderPath, szSHUserFolders);
        pUserShellFolderPath = userShellFolderPath;
        strcpyW(shellFolderPath, DefaultW);
        PathAddBackslashW(shellFolderPath);
        strcatW(shellFolderPath, szSHFolders);
        pShellFolderPath = shellFolderPath;
    }
    else
    {
        hToken = NULL;
        hRootKey = HKEY_CURRENT_USER;
        pUserShellFolderPath = szSHUserFolders;
        pShellFolderPath = szSHFolders;
    }

    hr = _SHRegisterFolders(hRootKey, hToken, pUserShellFolderPath,
     pShellFolderPath, folders, ARRAY_SIZE(folders));
    TRACE("returning 0x%08x\n", hr);
    return hr;
}

static HRESULT _SHRegisterCommonShellFolders(void)
{
    static const UINT folders[] = {
     CSIDL_COMMON_STARTMENU,
     CSIDL_COMMON_PROGRAMS,
     CSIDL_COMMON_STARTUP,
     CSIDL_COMMON_DESKTOPDIRECTORY,
     CSIDL_COMMON_FAVORITES,
     CSIDL_COMMON_APPDATA,
     CSIDL_COMMON_TEMPLATES,
     CSIDL_COMMON_DOCUMENTS,
     CSIDL_COMMON_ADMINTOOLS,
     CSIDL_COMMON_MUSIC,
     CSIDL_COMMON_PICTURES,
     CSIDL_COMMON_VIDEO,
    };
    HRESULT hr;

    TRACE("\n");
    hr = _SHRegisterFolders(HKEY_LOCAL_MACHINE, NULL, szSHUserFolders,
     szSHFolders, folders, ARRAY_SIZE(folders));
    TRACE("returning 0x%08x\n", hr);
    return hr;
}

/******************************************************************************
 * create_extra_folders  [Internal]
 *
 * Create some extra folders that don't have a standard CSIDL definition.
 */
static HRESULT create_extra_folders(void)
{
    static const WCHAR environW[] = {'E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR microsoftW[] = {'M','i','c','r','o','s','o','f','t',0};
    static const WCHAR TempW[]    = {'T','e','m','p',0};
    static const WCHAR TEMPW[]    = {'T','E','M','P',0};
    static const WCHAR TMPW[]     = {'T','M','P',0};
    WCHAR path[MAX_PATH+5];
    HRESULT hr;
    HKEY hkey;
    DWORD type, size, ret;

    ret = RegCreateKeyW( HKEY_CURRENT_USER, environW, &hkey );
    if (ret) return HRESULT_FROM_WIN32( ret );

    /* FIXME: should be under AppData, but we don't want spaces in the temp path */
    hr = SHGetFolderPathAndSubDirW( 0, CSIDL_PROFILE | CSIDL_FLAG_CREATE, NULL,
                                    SHGFP_TYPE_DEFAULT, TempW, path );
    if (SUCCEEDED(hr))
    {
        size = sizeof(path);
        if (RegQueryValueExW( hkey, TEMPW, NULL, &type, (LPBYTE)path, &size ))
            RegSetValueExW( hkey, TEMPW, 0, REG_SZ, (LPBYTE)path, (strlenW(path) + 1) * sizeof(WCHAR) );
        size = sizeof(path);
        if (RegQueryValueExW( hkey, TMPW, NULL, &type, (LPBYTE)path, &size ))
            RegSetValueExW( hkey, TMPW, 0, REG_SZ, (LPBYTE)path, (strlenW(path) + 1) * sizeof(WCHAR) );
    }
    RegCloseKey( hkey );

    if (SUCCEEDED(hr))
    {
        hr = SHGetFolderPathAndSubDirW( 0, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL,
                                        SHGFP_TYPE_DEFAULT, microsoftW, path );
    }
    if (SUCCEEDED(hr))
    {
        hr = SHGetFolderPathAndSubDirW(0, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL,
                                       SHGFP_TYPE_DEFAULT, Microsoft_Windows_ThemesW, path);
    }
    return hr;
}


/******************************************************************************
 * set_folder_attributes
 *
 * Set the various folder attributes registry keys.
 */
static HRESULT set_folder_attributes(void)
{
    static const WCHAR clsidW[] = {'C','L','S','I','D','\\',0 };
    static const WCHAR shellfolderW[] = {'\\','S','h','e','l','l','F','o','l','d','e','r', 0 };
    static const WCHAR wfparsingW[] = {'W','a','n','t','s','F','O','R','P','A','R','S','I','N','G',0};
    static const WCHAR wfdisplayW[] = {'W','a','n','t','s','F','O','R','D','I','S','P','L','A','Y',0};
    static const WCHAR hideasdeleteW[] = {'H','i','d','e','A','s','D','e','l','e','t','e','P','e','r','U','s','e','r',0};
    static const WCHAR cfattributesW[] = {'C','a','l','l','F','o','r','A','t','t','r','i','b','u','t','e','s',0};
    static const WCHAR emptyW[] = {0};

    static const struct
    {
        const CLSID *clsid;
        BOOL wfparsing : 1;
        BOOL wfdisplay : 1;
        BOOL hideasdel : 1;
        DWORD attr;
        DWORD call_for_attr;
    } folders[] =
    {
        { &CLSID_UnixFolder, TRUE, FALSE, FALSE },
        { &CLSID_UnixDosFolder, TRUE, FALSE, FALSE,
          SFGAO_FILESYSANCESTOR|SFGAO_FOLDER|SFGAO_HASSUBFOLDER, SFGAO_FILESYSTEM },
        { &CLSID_FolderShortcut, FALSE, FALSE, FALSE,
          SFGAO_FILESYSTEM|SFGAO_FOLDER|SFGAO_LINK,
          SFGAO_HASSUBFOLDER|SFGAO_FILESYSTEM|SFGAO_FOLDER|SFGAO_FILESYSANCESTOR },
        { &CLSID_MyDocuments, TRUE, FALSE, FALSE,
          SFGAO_FILESYSANCESTOR|SFGAO_FOLDER|SFGAO_HASSUBFOLDER, SFGAO_FILESYSTEM },
        { &CLSID_RecycleBin, FALSE, FALSE, FALSE,
          SFGAO_FOLDER|SFGAO_DROPTARGET|SFGAO_HASPROPSHEET },
        { &CLSID_ControlPanel, FALSE, TRUE, TRUE,
          SFGAO_FOLDER|SFGAO_HASSUBFOLDER }
    };

    unsigned int i;
    WCHAR buffer[39 + ARRAY_SIZE(clsidW) + ARRAY_SIZE(shellfolderW)];
    LONG res;
    HKEY hkey;

    for (i = 0; i < ARRAY_SIZE(folders); i++)
    {
        strcpyW( buffer, clsidW );
        StringFromGUID2( folders[i].clsid, buffer + strlenW(buffer), 39 );
        strcatW( buffer, shellfolderW );
        res = RegCreateKeyExW( HKEY_CLASSES_ROOT, buffer, 0, NULL, 0,
                               KEY_READ | KEY_WRITE, NULL, &hkey, NULL);
        if (res) return HRESULT_FROM_WIN32( res );
        if (folders[i].wfparsing)
            res = RegSetValueExW( hkey, wfparsingW, 0, REG_SZ, (const BYTE *)emptyW, sizeof(emptyW) );
        if (folders[i].wfdisplay)
            res = RegSetValueExW( hkey, wfdisplayW, 0, REG_SZ, (const BYTE *)emptyW, sizeof(emptyW) );
        if (folders[i].hideasdel)
            res = RegSetValueExW( hkey, hideasdeleteW, 0, REG_SZ, (const BYTE *)emptyW, sizeof(emptyW) );
        if (folders[i].attr)
            res = RegSetValueExW( hkey, szAttributes, 0, REG_DWORD,
                                  (const BYTE *)&folders[i].attr, sizeof(DWORD));
        if (folders[i].call_for_attr)
            res = RegSetValueExW( hkey, cfattributesW, 0, REG_DWORD,
                                 (const BYTE *)&folders[i].call_for_attr, sizeof(DWORD));
        RegCloseKey( hkey );
    }
    return S_OK;
}

/*************************************************************************
 * SHGetSpecialFolderPathA [SHELL32.@]
 */
BOOL WINAPI SHGetSpecialFolderPathA (
	HWND hwndOwner,
	LPSTR szPath,
	int nFolder,
	BOOL bCreate)
{
    return SHGetFolderPathA(hwndOwner, nFolder + (bCreate ? CSIDL_FLAG_CREATE : 0), NULL, 0,
                            szPath) == S_OK;
}

/*************************************************************************
 * SHGetSpecialFolderPathW
 */
BOOL WINAPI SHGetSpecialFolderPathW (
	HWND hwndOwner,
	LPWSTR szPath,
	int nFolder,
	BOOL bCreate)
{
    return SHGetFolderPathW(hwndOwner, nFolder + (bCreate ? CSIDL_FLAG_CREATE : 0), NULL, 0,
                            szPath) == S_OK;
}

/*************************************************************************
 * SHGetSpecialFolderPath (SHELL32.175)
 */
BOOL WINAPI SHGetSpecialFolderPathAW (
	HWND hwndOwner,
	LPVOID szPath,
	int nFolder,
	BOOL bCreate)

{
	if (SHELL_OsIsUnicode())
	  return SHGetSpecialFolderPathW (hwndOwner, szPath, nFolder, bCreate);
	return SHGetSpecialFolderPathA (hwndOwner, szPath, nFolder, bCreate);
}

/*************************************************************************
 * SHGetFolderLocation [SHELL32.@]
 *
 * Gets the folder locations from the registry and creates a pidl.
 *
 * PARAMS
 *   hwndOwner  [I]
 *   nFolder    [I] CSIDL_xxxxx
 *   hToken     [I] token representing user, or NULL for current user, or -1 for
 *                  default user
 *   dwReserved [I] must be zero
 *   ppidl      [O] PIDL of a special folder
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Standard OLE-defined error result, S_FALSE or E_INVALIDARG
 *
 * NOTES
 *  Creates missing reg keys and directories.
 *  Mostly forwards to SHGetFolderPathW, but a few values of nFolder return
 *  virtual folders that are handled here.
 */
HRESULT WINAPI SHGetFolderLocation(
	HWND hwndOwner,
	int nFolder,
	HANDLE hToken,
	DWORD dwReserved,
	LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_INVALIDARG;

    TRACE("%p 0x%08x %p 0x%08x %p\n",
     hwndOwner, nFolder, hToken, dwReserved, ppidl);
    
    if (!ppidl)
        return E_INVALIDARG;
    if (dwReserved)
        return E_INVALIDARG;

    /* The virtual folders' locations are not user-dependent */
    *ppidl = NULL;
    switch (nFolder & CSIDL_FOLDER_MASK)
    {
        case CSIDL_DESKTOP:
            *ppidl = _ILCreateDesktop();
            break;

        case CSIDL_PERSONAL:
            *ppidl = _ILCreateMyDocuments();
            break;

        case CSIDL_INTERNET:
            *ppidl = _ILCreateIExplore();
            break;

        case CSIDL_CONTROLS:
            *ppidl = _ILCreateControlPanel();
            break;

        case CSIDL_PRINTERS:
            *ppidl = _ILCreatePrinters();
            break;

        case CSIDL_BITBUCKET:
            *ppidl = _ILCreateBitBucket();
            break;

        case CSIDL_DRIVES:
            *ppidl = _ILCreateMyComputer();
            break;

        case CSIDL_NETWORK:
            *ppidl = _ILCreateNetwork();
            break;

        default:
        {
            WCHAR szPath[MAX_PATH];

            hr = SHGetFolderPathW(hwndOwner, nFolder, hToken,
             SHGFP_TYPE_CURRENT, szPath);
            if (SUCCEEDED(hr))
            {
                DWORD attributes=0;

                TRACE("Value=%s\n", debugstr_w(szPath));
                hr = SHILCreateFromPathW(szPath, ppidl, &attributes);
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                /* unlike SHGetFolderPath, SHGetFolderLocation in shell32
                 * version 6.0 returns E_FAIL for nonexistent paths
                 */
                hr = E_FAIL;
            }
        }
    }
    if(*ppidl)
        hr = S_OK;

    TRACE("-- (new pidl %p)\n",*ppidl);
    return hr;
}

/*************************************************************************
 * SHGetSpecialFolderLocation		[SHELL32.@]
 *
 * NOTES
 *   In NT5, SHGetSpecialFolderLocation needs the <winntdir>/Recent
 *   directory.
 */
HRESULT WINAPI SHGetSpecialFolderLocation(
	HWND hwndOwner,
	INT nFolder,
	LPITEMIDLIST * ppidl)
{
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p,0x%x,%p)\n", hwndOwner,nFolder,ppidl);

    if (!ppidl)
        return E_INVALIDARG;

    hr = SHGetFolderLocation(hwndOwner, nFolder, NULL, 0, ppidl);
    return hr;
}

/*************************************************************************
 * SHGetKnownFolderPath           [SHELL32.@]
 */
HRESULT WINAPI SHGetKnownFolderPath(REFKNOWNFOLDERID rfid, DWORD flags, HANDLE token, WCHAR **ret_path)
{
    WCHAR pathW[MAX_PATH], tempW[MAX_PATH];
    HRESULT    hr;
    CSIDL_Type type;
    int        ret;
    int folder = csidl_from_id(rfid), shgfp_flags;

    TRACE("%s, 0x%08x, %p, %p\n", debugstr_guid(rfid), flags, token, ret_path);

    *ret_path = NULL;

    if (folder < 0)
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    if (flags & ~(KF_FLAG_CREATE|KF_FLAG_SIMPLE_IDLIST|KF_FLAG_DONT_UNEXPAND|
        KF_FLAG_DONT_VERIFY|KF_FLAG_NO_ALIAS|KF_FLAG_INIT|KF_FLAG_DEFAULT_PATH))
    {
        FIXME("flags 0x%08x not supported\n", flags);
        return E_INVALIDARG;
    }

    shgfp_flags = flags & KF_FLAG_DEFAULT_PATH ? SHGFP_TYPE_DEFAULT : SHGFP_TYPE_CURRENT;

    type = CSIDL_Data[folder].type;
    switch (type)
    {
        case CSIDL_Type_Disallowed:
            hr = E_INVALIDARG;
            break;
        case CSIDL_Type_NonExistent:
            *tempW = 0;
            hr = S_FALSE;
            break;
        case CSIDL_Type_WindowsPath:
            GetWindowsDirectoryW(tempW, MAX_PATH);
            append_relative_path(folder, tempW);
            hr = S_OK;
            break;
        case CSIDL_Type_SystemPath:
            GetSystemDirectoryW(tempW, MAX_PATH);
            append_relative_path(folder, tempW);
            hr = S_OK;
            break;
        case CSIDL_Type_SystemX86Path:
            if (!GetSystemWow64DirectoryW(tempW, MAX_PATH)) GetSystemDirectoryW(tempW, MAX_PATH);
            append_relative_path(folder, tempW);
            hr = S_OK;
            break;
        case CSIDL_Type_CurrVer:
            hr = _SHGetCurrentVersionPath(shgfp_flags, folder, tempW);
            break;
        case CSIDL_Type_User:
            hr = _SHGetUserProfilePath(token, shgfp_flags, folder, tempW);
            break;
        case CSIDL_Type_AllUsers:
        case CSIDL_Type_ProgramData:
            hr = _SHGetAllUsersProfilePath(shgfp_flags, folder, tempW);
            break;
        default:
            FIXME("bogus type %d, please fix\n", type);
            hr = E_INVALIDARG;
            break;
    }

    if (FAILED(hr))
        goto failed;

    /* Expand environment strings if necessary */
    if (*tempW == '%')
    {
        hr = _SHExpandEnvironmentStrings(tempW, pathW);
        if (FAILED(hr))
            goto failed;
    }
    else
        strcpyW(pathW, tempW);

    /* if we don't care about existing directories we are ready */
    if (flags & KF_FLAG_DONT_VERIFY) goto done;

    if (PathFileExistsW(pathW)) goto done;

    /* Does not exist but we are not allowed to create it. The return value
     * is verified against shell32 version 6.0.
     */
    if (!(flags & KF_FLAG_CREATE))
    {
        hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        goto failed;
    }

    /* create symbolic links rather than directories for specific
     * user shell folders */
    _SHCreateSymbolicLink(folder);

    /* create directory/directories */
    ret = SHCreateDirectoryExW(NULL, pathW, NULL);
    if (ret && ret != ERROR_ALREADY_EXISTS)
    {
        ERR("Failed to create directory %s.\n", debugstr_w(pathW));
        hr = E_FAIL;
        goto failed;
    }

    TRACE("Created missing system directory %s\n", debugstr_w(pathW));

done:
    TRACE("Final path is %s, %#x\n", debugstr_w(pathW), hr);

    *ret_path = CoTaskMemAlloc((strlenW(pathW) + 1) * sizeof(WCHAR));
    if (!*ret_path)
        return E_OUTOFMEMORY;
    strcpyW(*ret_path, pathW);

    return hr;

failed:
    TRACE("Failed to get folder path, %#x.\n", hr);

    return hr;
}

/*************************************************************************
 * SHGetFolderPathEx           [SHELL32.@]
 */
HRESULT WINAPI SHGetFolderPathEx(REFKNOWNFOLDERID rfid, DWORD flags, HANDLE token, LPWSTR path, DWORD len)
{
    HRESULT hr;
    WCHAR *buffer;

    TRACE("%s, 0x%08x, %p, %p, %u\n", debugstr_guid(rfid), flags, token, path, len);

    if (!path || !len) return E_INVALIDARG;

    hr = SHGetKnownFolderPath( rfid, flags, token, &buffer );
    if (SUCCEEDED( hr ))
    {
        if (strlenW( buffer ) + 1 > len)
        {
            CoTaskMemFree( buffer );
            return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }
        strcpyW( path, buffer );
        CoTaskMemFree( buffer );
    }
    return hr;
}

/*
 * Internal function to convert known folder identifier to path of registry key
 * associated with known folder.
 *
 * Parameters:
 *  rfid            [I] pointer to known folder identifier (may be NULL)
 *  lpStringGuid    [I] string with known folder identifier (used when rfid is NULL)
 *  lpPath          [O] place to store string address. String should be
 *                      later freed using HeapFree(GetProcessHeap(),0, ... )
 */
static HRESULT get_known_folder_registry_path(
    REFKNOWNFOLDERID rfid,
    LPWSTR lpStringGuid,
    LPWSTR *lpPath)
{
    static const WCHAR sBackslash[] = {'\\',0};
    HRESULT hr = S_OK;
    int length;
    WCHAR sGuid[50];

    TRACE("(%s, %s, %p)\n", debugstr_guid(rfid), debugstr_w(lpStringGuid), lpPath);

    if(rfid)
        StringFromGUID2(rfid, sGuid, ARRAY_SIZE(sGuid));
    else
        lstrcpyW(sGuid, lpStringGuid);

    length = lstrlenW(szKnownFolderDescriptions)+51;
    *lpPath = heap_alloc(length*sizeof(WCHAR));
    if(!(*lpPath))
        hr = E_OUTOFMEMORY;

    if(SUCCEEDED(hr))
    {
        lstrcpyW(*lpPath, szKnownFolderDescriptions);
        lstrcatW(*lpPath, sBackslash);
        lstrcatW(*lpPath, sGuid);
    }

    return hr;
}

static HRESULT get_known_folder_wstr(const WCHAR *regpath, const WCHAR *value, WCHAR **out)
{
    DWORD size = 0;
    HRESULT hr;

    size = 0;
    hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, regpath, value, RRF_RT_REG_SZ, NULL, NULL, &size));
    if(FAILED(hr))
        return hr;

    *out = CoTaskMemAlloc(size);
    if(!*out)
        return E_OUTOFMEMORY;

    hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, regpath, value, RRF_RT_REG_SZ, NULL, *out, &size));
    if(FAILED(hr)){
        CoTaskMemFree(*out);
        *out = NULL;
    }

    return hr;
}

static HRESULT get_known_folder_dword(const WCHAR *registryPath, const WCHAR *value, DWORD *out)
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType;
    return HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, value, RRF_RT_DWORD, &dwType, out, &dwSize));
}

/*
 * Internal function to get place where folder redirection information are stored.
 *
 * Parameters:
 *  rfid            [I] pointer to known folder identifier (may be NULL)
 *  rootKey         [O] root key where the redirection information are stored
 *                      It can be HKLM for COMMON folders, and HKCU for PERUSER folders.
 *                      However, besides root key, path is always that same, and is stored
 *                      as "szKnownFolderRedirections" constant
 */
static HRESULT get_known_folder_redirection_place(
    REFKNOWNFOLDERID rfid,
    HKEY *rootKey)
{
    HRESULT hr;
    LPWSTR lpRegistryPath = NULL;
    KF_CATEGORY category;

    /* first, get known folder's category */
    hr = get_known_folder_registry_path(rfid, NULL, &lpRegistryPath);

    if(SUCCEEDED(hr))
        hr = get_known_folder_dword(lpRegistryPath, szCategory, &category);

    if(SUCCEEDED(hr))
    {
        if(category == KF_CATEGORY_COMMON)
        {
            *rootKey = HKEY_LOCAL_MACHINE;
            hr = S_OK;
        }
        else if(category == KF_CATEGORY_PERUSER)
        {
            *rootKey = HKEY_CURRENT_USER;
            hr = S_OK;
        }
        else
            hr = E_FAIL;
    }

    heap_free(lpRegistryPath);
    return hr;
}

static HRESULT get_known_folder_path_by_id(REFKNOWNFOLDERID folderId, LPWSTR lpRegistryPath, DWORD dwFlags, LPWSTR *ppszPath);

static HRESULT redirect_known_folder(
    REFKNOWNFOLDERID rfid,
    HWND hwnd,
    KF_REDIRECT_FLAGS flags,
    LPCWSTR pszTargetPath,
    UINT cFolders,
    KNOWNFOLDERID const *pExclusion,
    LPWSTR *ppszError)
{
    HRESULT hr;
    HKEY rootKey = HKEY_LOCAL_MACHINE, hKey;
    WCHAR sGuid[39];
    LPWSTR lpRegistryPath = NULL, lpSrcPath = NULL;
    TRACE("(%s, %p, 0x%08x, %s, %d, %p, %p)\n", debugstr_guid(rfid), hwnd, flags, debugstr_w(pszTargetPath), cFolders, pExclusion, ppszError);

    if (ppszError) *ppszError = NULL;

    hr = get_known_folder_registry_path(rfid, NULL, &lpRegistryPath);

    if(SUCCEEDED(hr))
        hr = get_known_folder_path_by_id(rfid, lpRegistryPath, 0, &lpSrcPath);

    heap_free(lpRegistryPath);

    /* get path to redirection storage */
    if(SUCCEEDED(hr))
        hr = get_known_folder_redirection_place(rfid, &rootKey);

    /* write redirection information */
    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegCreateKeyExW(rootKey, szKnownFolderRedirections, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL));

    if(SUCCEEDED(hr))
    {
        StringFromGUID2(rfid, sGuid, ARRAY_SIZE(sGuid));

        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, sGuid, 0, REG_SZ, (LPBYTE)pszTargetPath, (lstrlenW(pszTargetPath)+1)*sizeof(WCHAR)));

        RegCloseKey(hKey);
    }

    /* make sure destination path exists */
    SHCreateDirectory(NULL, pszTargetPath);

    /* copy content if required */
    if(SUCCEEDED(hr) && (flags & KF_REDIRECT_COPY_CONTENTS) )
    {
        static const WCHAR sWildcard[] = {'\\','*',0};
        WCHAR srcPath[MAX_PATH+1], dstPath[MAX_PATH+1];
        SHFILEOPSTRUCTW fileOp;

        ZeroMemory(srcPath, sizeof(srcPath));
        lstrcpyW(srcPath, lpSrcPath);
        lstrcatW(srcPath, sWildcard);

        ZeroMemory(dstPath, sizeof(dstPath));
        lstrcpyW(dstPath, pszTargetPath);

        ZeroMemory(&fileOp, sizeof(fileOp));

        if(flags & KF_REDIRECT_DEL_SOURCE_CONTENTS)
            fileOp.wFunc = FO_MOVE;
        else
            fileOp.wFunc = FO_COPY;

        fileOp.pFrom = srcPath;
        fileOp.pTo = dstPath;
        fileOp.fFlags = FOF_NO_UI;

        hr = (SHFileOperationW(&fileOp)==0 ? S_OK : E_FAIL);

        if(flags & KF_REDIRECT_DEL_SOURCE_CONTENTS)
        {
            ZeroMemory(srcPath, sizeof(srcPath));
            lstrcpyW(srcPath, lpSrcPath);

            ZeroMemory(&fileOp, sizeof(fileOp));
            fileOp.wFunc = FO_DELETE;
            fileOp.pFrom = srcPath;
            fileOp.fFlags = FOF_NO_UI;

            hr = (SHFileOperationW(&fileOp)==0 ? S_OK : E_FAIL);
        }
    }

    CoTaskMemFree(lpSrcPath);

    return hr;
}


struct knownfolder
{
    IKnownFolder IKnownFolder_iface;
    LONG refs;
    KNOWNFOLDERID id;
    LPWSTR registryPath;
};

static inline struct knownfolder *impl_from_IKnownFolder( IKnownFolder *iface )
{
    return CONTAINING_RECORD( iface, struct knownfolder, IKnownFolder_iface );
}

static ULONG WINAPI knownfolder_AddRef(
    IKnownFolder *iface )
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    return InterlockedIncrement( &knownfolder->refs );
}

static ULONG WINAPI knownfolder_Release(
    IKnownFolder *iface )
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    LONG refs = InterlockedDecrement( &knownfolder->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", knownfolder);
        heap_free( knownfolder->registryPath );
        heap_free( knownfolder );
    }
    return refs;
}

static HRESULT WINAPI knownfolder_QueryInterface(
    IKnownFolder *iface,
    REFIID riid,
    void **ppv )
{
    struct knownfolder *This = impl_from_IKnownFolder( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppv );

    *ppv = NULL;
    if ( IsEqualGUID( riid, &IID_IKnownFolder ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppv = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IMarshal ) )
    {
        TRACE("IID_IMarshal returning NULL.\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IKnownFolder_AddRef( iface );
    return S_OK;
}

static HRESULT knownfolder_set_id(
    struct knownfolder *knownfolder,
    const KNOWNFOLDERID *kfid)
{
    HKEY hKey;
    HRESULT hr;

    TRACE("%s\n", debugstr_guid(kfid));

    knownfolder->id = *kfid;

    /* check is it registry-registered folder */
    hr = get_known_folder_registry_path(kfid, NULL, &knownfolder->registryPath);
    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegOpenKeyExW(HKEY_LOCAL_MACHINE, knownfolder->registryPath, 0, 0, &hKey));

    if(SUCCEEDED(hr))
    {
        hr = S_OK;
        RegCloseKey(hKey);
    }
    else
    {
        /* This known folder is not registered. To mark it, we set registryPath to NULL */
        heap_free(knownfolder->registryPath);
        knownfolder->registryPath = NULL;
        hr = S_OK;
    }

    return hr;
}

static HRESULT WINAPI knownfolder_GetId(
    IKnownFolder *iface,
    KNOWNFOLDERID *pkfid)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );

    TRACE("%p\n", pkfid);

    *pkfid = knownfolder->id;
    return S_OK;
}

static HRESULT WINAPI knownfolder_GetCategory(
    IKnownFolder *iface,
    KF_CATEGORY *pCategory)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder(iface);
    HRESULT hr = S_OK;

    TRACE("%p, %p\n", knownfolder, pCategory);

    /* we cannot get a category for a folder which is not registered */
    if(!knownfolder->registryPath)
        hr = E_FAIL;

    if(SUCCEEDED(hr))
        hr = get_known_folder_dword(knownfolder->registryPath, szCategory, pCategory);

    return hr;
}

static HRESULT WINAPI knownfolder_GetShellItem(
    IKnownFolder *iface,
    DWORD flags,
    REFIID riid,
    void **ppv)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder(iface);
    TRACE("(%p, 0x%08x, %s, %p)\n", knownfolder, flags, debugstr_guid(riid), ppv);
    return SHGetKnownFolderItem(&knownfolder->id, flags, NULL, riid, ppv);
}

static HRESULT get_known_folder_path(
    LPWSTR sFolderId,
    LPWSTR registryPath,
    LPWSTR *ppszPath)
{
    static const WCHAR sBackslash[] = {'\\',0};
    HRESULT hr;
    DWORD dwSize, dwType;
    WCHAR path[MAX_PATH] = {0};
    WCHAR parentGuid[39];
    KF_CATEGORY category;
    LPWSTR parentRegistryPath, parentPath;
    HKEY hRedirectionRootKey = NULL;

    TRACE("(%s, %p)\n", debugstr_w(registryPath), ppszPath);
    *ppszPath = NULL;

    /* check if folder has parent */
    dwSize = sizeof(parentGuid);
    hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, szParentFolder, RRF_RT_REG_SZ, &dwType, parentGuid, &dwSize));
    if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) hr = S_FALSE;

    if(hr == S_OK)
    {
        /* get parent's known folder path (recursive) */
        hr = get_known_folder_registry_path(NULL, parentGuid, &parentRegistryPath);
        if(FAILED(hr)) return hr;

        hr = get_known_folder_path(parentGuid, parentRegistryPath, &parentPath);
        if(FAILED(hr)) {
            heap_free(parentRegistryPath);
            return hr;
        }

        lstrcatW(path, parentPath);
        lstrcatW(path, sBackslash);

        heap_free(parentRegistryPath);
        heap_free(parentPath);
    }

    /* check, if folder was redirected */
    if(SUCCEEDED(hr))
        hr = get_known_folder_dword(registryPath, szCategory, &category);

    if(SUCCEEDED(hr))
    {
        if(category == KF_CATEGORY_COMMON)
            hRedirectionRootKey = HKEY_LOCAL_MACHINE;
        else if(category == KF_CATEGORY_PERUSER)
            hRedirectionRootKey = HKEY_CURRENT_USER;

        if(hRedirectionRootKey)
        {
            hr = HRESULT_FROM_WIN32(RegGetValueW(hRedirectionRootKey, szKnownFolderRedirections, sFolderId, RRF_RT_REG_SZ, NULL, NULL, &dwSize));

            if(SUCCEEDED(hr))
            {
                *ppszPath = CoTaskMemAlloc(dwSize+(lstrlenW(path)+1)*sizeof(WCHAR));
                if(!*ppszPath) hr = E_OUTOFMEMORY;
            }

            if(SUCCEEDED(hr))
            {
                lstrcpyW(*ppszPath, path);
                hr = HRESULT_FROM_WIN32(RegGetValueW(hRedirectionRootKey, szKnownFolderRedirections, sFolderId, RRF_RT_REG_SZ, NULL, *ppszPath + lstrlenW(path), &dwSize));
            }
        }

        if(!*ppszPath)
        {
            /* no redirection, use previous way - read the relative path from folder definition */
            hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, szRelativePath, RRF_RT_REG_SZ, &dwType, NULL, &dwSize));

            if(SUCCEEDED(hr))
            {
                *ppszPath = CoTaskMemAlloc(dwSize+(lstrlenW(path)+1)*sizeof(WCHAR));
                if(!*ppszPath) hr = E_OUTOFMEMORY;
            }

            if(SUCCEEDED(hr))
            {
                lstrcpyW(*ppszPath, path);
                hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, registryPath, szRelativePath, RRF_RT_REG_SZ, &dwType, *ppszPath + lstrlenW(path), &dwSize));
            }
        }
    }

    TRACE("returning path: %s\n", debugstr_w(*ppszPath));
    return hr;
}

static HRESULT get_known_folder_path_by_id(
    REFKNOWNFOLDERID folderId,
    LPWSTR lpRegistryPath,
    DWORD dwFlags,
    LPWSTR *ppszPath)
{
    HRESULT hr = E_FAIL;
    WCHAR sGuid[39];
    DWORD dwAttributes;

    TRACE("(%s, %s, 0x%08x, %p)\n", debugstr_guid(folderId), debugstr_w(lpRegistryPath), dwFlags, ppszPath);

    /* if this is registry-registered known folder, get path from registry */
    if(lpRegistryPath)
    {
        StringFromGUID2(folderId, sGuid, ARRAY_SIZE(sGuid));

        hr = get_known_folder_path(sGuid, lpRegistryPath, ppszPath);
    }
    /* in other case, use older way */

    if(FAILED(hr))
        hr = SHGetKnownFolderPath( folderId, dwFlags, NULL, ppszPath );

    if (FAILED(hr)) return hr;

    /* check if known folder really exists */
    dwAttributes = GetFileAttributesW(*ppszPath);
    if(dwAttributes == INVALID_FILE_ATTRIBUTES || !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
        TRACE("directory %s not found\n", debugstr_w(*ppszPath));
        CoTaskMemFree(*ppszPath);
        *ppszPath = NULL;
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hr;
}

static HRESULT WINAPI knownfolder_GetPath(
    IKnownFolder *iface,
    DWORD dwFlags,
    LPWSTR *ppszPath)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    TRACE("(%p, 0x%08x, %p)\n", knownfolder, dwFlags, ppszPath);

    return get_known_folder_path_by_id(&knownfolder->id, knownfolder->registryPath, dwFlags, ppszPath);
}

static HRESULT WINAPI knownfolder_SetPath(
    IKnownFolder *iface,
    DWORD dwFlags,
    LPCWSTR pszPath)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    HRESULT hr = S_OK;

    TRACE("(%p, 0x%08x, %s)\n", knownfolder, dwFlags, debugstr_w(pszPath));

    /* check if the known folder is registered */
    if(!knownfolder->registryPath)
        hr = E_FAIL;

    if(SUCCEEDED(hr))
        hr = redirect_known_folder(&knownfolder->id, NULL, 0, pszPath, 0, NULL, NULL);

    return hr;
}

static HRESULT WINAPI knownfolder_GetIDList(
    IKnownFolder *iface,
    DWORD flags,
    PIDLIST_ABSOLUTE *ppidl)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    TRACE("(%p, 0x%08x, %p)\n", knownfolder, flags, ppidl);
    return SHGetKnownFolderIDList(&knownfolder->id, flags, NULL, ppidl);
}

static HRESULT WINAPI knownfolder_GetFolderType(
    IKnownFolder *iface,
    FOLDERTYPEID *pftid)
{
    FIXME("%p\n", pftid);
    return E_NOTIMPL;
}

static HRESULT WINAPI knownfolder_GetRedirectionCapabilities(
    IKnownFolder *iface,
    KF_REDIRECTION_CAPABILITIES *pCapabilities)
{
    FIXME("%p stub\n", pCapabilities);
    if(!pCapabilities) return E_INVALIDARG;
    *pCapabilities = KF_REDIRECTION_CAPABILITIES_DENY_ALL;
    return S_OK;
}

static HRESULT WINAPI knownfolder_GetFolderDefinition(
    IKnownFolder *iface,
    KNOWNFOLDER_DEFINITION *pKFD)
{
    struct knownfolder *knownfolder = impl_from_IKnownFolder( iface );
    HRESULT hr;
    DWORD dwSize;
    WCHAR parentGuid[39];
    TRACE("(%p, %p)\n", knownfolder, pKFD);

    if(!pKFD) return E_INVALIDARG;

    ZeroMemory(pKFD, sizeof(*pKFD));

    /* required fields */
    hr = get_known_folder_dword(knownfolder->registryPath, szCategory, &pKFD->category);
    if(FAILED(hr))
        return hr;

    hr = get_known_folder_wstr(knownfolder->registryPath, szName, &pKFD->pszName);
    if(FAILED(hr))
        return hr;

    /* optional fields */
    dwSize = sizeof(parentGuid);
    hr = HRESULT_FROM_WIN32(RegGetValueW(HKEY_LOCAL_MACHINE, knownfolder->registryPath, szParentFolder,
                RRF_RT_REG_SZ, NULL, parentGuid, &dwSize));
    if(SUCCEEDED(hr))
    {
        hr = IIDFromString(parentGuid, &pKFD->fidParent);
        if(FAILED(hr))
            return hr;
    }

    get_known_folder_dword(knownfolder->registryPath, szAttributes, &pKFD->dwAttributes);

    get_known_folder_wstr(knownfolder->registryPath, szRelativePath, &pKFD->pszRelativePath);

    get_known_folder_wstr(knownfolder->registryPath, szParsingName, &pKFD->pszParsingName);

    return S_OK;
}

static const struct IKnownFolderVtbl knownfolder_vtbl =
{
    knownfolder_QueryInterface,
    knownfolder_AddRef,
    knownfolder_Release,
    knownfolder_GetId,
    knownfolder_GetCategory,
    knownfolder_GetShellItem,
    knownfolder_GetPath,
    knownfolder_SetPath,
    knownfolder_GetIDList,
    knownfolder_GetFolderType,
    knownfolder_GetRedirectionCapabilities,
    knownfolder_GetFolderDefinition
};

static HRESULT knownfolder_create( struct knownfolder **knownfolder )
{
    struct knownfolder *kf;

    kf = heap_alloc( sizeof(*kf) );
    if (!kf) return E_OUTOFMEMORY;

    kf->IKnownFolder_iface.lpVtbl = &knownfolder_vtbl;
    kf->refs = 1;
    memset( &kf->id, 0, sizeof(kf->id) );
    kf->registryPath = NULL;

    *knownfolder = kf;

    TRACE("returning iface %p\n", &kf->IKnownFolder_iface);
    return S_OK;
}

struct foldermanager
{
    IKnownFolderManager IKnownFolderManager_iface;
    LONG refs;
    UINT num_ids;
    KNOWNFOLDERID *ids;
};

static inline struct foldermanager *impl_from_IKnownFolderManager( IKnownFolderManager *iface )
{
    return CONTAINING_RECORD( iface, struct foldermanager, IKnownFolderManager_iface );
}

static ULONG WINAPI foldermanager_AddRef(
    IKnownFolderManager *iface )
{
    struct foldermanager *foldermanager = impl_from_IKnownFolderManager( iface );
    return InterlockedIncrement( &foldermanager->refs );
}

static ULONG WINAPI foldermanager_Release(
    IKnownFolderManager *iface )
{
    struct foldermanager *foldermanager = impl_from_IKnownFolderManager( iface );
    LONG refs = InterlockedDecrement( &foldermanager->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", foldermanager);
        heap_free( foldermanager->ids );
        heap_free( foldermanager );
    }
    return refs;
}

static HRESULT WINAPI foldermanager_QueryInterface(
    IKnownFolderManager *iface,
    REFIID riid,
    void **ppv )
{
    struct foldermanager *This = impl_from_IKnownFolderManager( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppv );

    *ppv = NULL;
    if ( IsEqualGUID( riid, &IID_IKnownFolderManager ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppv = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IMarshal ) )
    {
        TRACE("IID_IMarshal returning NULL.\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IKnownFolderManager_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI foldermanager_FolderIdFromCsidl(
    IKnownFolderManager *iface,
    int nCsidl,
    KNOWNFOLDERID *pfid)
{
    TRACE("%d, %p\n", nCsidl, pfid);

    if (nCsidl >= ARRAY_SIZE(CSIDL_Data))
        return E_INVALIDARG;
    *pfid = *CSIDL_Data[nCsidl].id;
    return S_OK;
}

static HRESULT WINAPI foldermanager_FolderIdToCsidl(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid,
    int *pnCsidl)
{
    int csidl;

    TRACE("%s, %p\n", debugstr_guid(rfid), pnCsidl);

    csidl = csidl_from_id( rfid );
    if (csidl == -1) return E_INVALIDARG;
    *pnCsidl = csidl;
    return S_OK;
}

static HRESULT WINAPI foldermanager_GetFolderIds(
    IKnownFolderManager *iface,
    KNOWNFOLDERID **ppKFId,
    UINT *pCount)
{
    struct foldermanager *fm = impl_from_IKnownFolderManager( iface );

    TRACE("%p, %p\n", ppKFId, pCount);

    *ppKFId = CoTaskMemAlloc(fm->num_ids * sizeof(KNOWNFOLDERID));
    memcpy(*ppKFId, fm->ids, fm->num_ids * sizeof(KNOWNFOLDERID));
    *pCount = fm->num_ids;
    return S_OK;
}

static BOOL is_knownfolder( struct foldermanager *fm, const KNOWNFOLDERID *id )
{
    UINT i;
    HRESULT hr;
    LPWSTR registryPath = NULL;
    HKEY hKey;

    /* TODO: move all entries from "CSIDL_Data" static array to registry known folder descriptions */
    for (i = 0; i < fm->num_ids; i++)
        if (IsEqualGUID( &fm->ids[i], id )) return TRUE;

    hr = get_known_folder_registry_path(id, NULL, &registryPath);
    if(SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegOpenKeyExW(HKEY_LOCAL_MACHINE, registryPath, 0, 0, &hKey));
        heap_free(registryPath);
    }

    if(SUCCEEDED(hr))
    {
        hr = S_OK;
        RegCloseKey(hKey);
    }

    return hr == S_OK;
}

static HRESULT WINAPI foldermanager_GetFolder(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid,
    IKnownFolder **ppkf)
{
    struct foldermanager *fm = impl_from_IKnownFolderManager( iface );
    struct knownfolder *kf;
    HRESULT hr;

    TRACE("%s, %p\n", debugstr_guid(rfid), ppkf);

    if (!is_knownfolder( fm, rfid ))
    {
        WARN("unknown folder\n");
        return E_INVALIDARG;
    }
    hr = knownfolder_create( &kf );
    if (SUCCEEDED( hr ))
    {
        hr = knownfolder_set_id( kf, rfid );
        *ppkf = &kf->IKnownFolder_iface;
    }
    else
        *ppkf = NULL;

    return hr;
}

static HRESULT WINAPI foldermanager_GetFolderByName(
    IKnownFolderManager *iface,
    LPCWSTR pszCanonicalName,
    IKnownFolder **ppkf)
{
    struct foldermanager *fm = impl_from_IKnownFolderManager( iface );
    struct knownfolder *kf;
    BOOL found = FALSE;
    HRESULT hr;
    UINT i;

    TRACE( "%s, %p\n", debugstr_w(pszCanonicalName), ppkf );

    for (i = 0; i < fm->num_ids; i++)
    {
        WCHAR *path, *name;
        hr = get_known_folder_registry_path( &fm->ids[i], NULL, &path );
        if (FAILED( hr )) return hr;

        hr = get_known_folder_wstr( path, szName, &name );
        heap_free( path );
        if (FAILED( hr )) return hr;

        found = !strcmpiW( pszCanonicalName, name );
        CoTaskMemFree( name );
        if (found) break;
    }

    if (found)
    {
        hr = knownfolder_create( &kf );
        if (FAILED( hr )) return hr;

        hr = knownfolder_set_id( kf, &fm->ids[i] );
        if (FAILED( hr ))
        {
            IKnownFolder_Release( &kf->IKnownFolder_iface );
            return hr;
        }
        *ppkf = &kf->IKnownFolder_iface;
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
        *ppkf = NULL;
    }

    return hr;
}

static HRESULT register_folder(const KNOWNFOLDERID *rfid, const KNOWNFOLDER_DEFINITION *pKFD)
{
    HRESULT hr;
    HKEY hKey = NULL;
    DWORD dwDisp;
    LPWSTR registryPath = NULL;

    hr = get_known_folder_registry_path(rfid, NULL, &registryPath);
    TRACE("registry path: %s\n", debugstr_w(registryPath));

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegCreateKeyExW(HKEY_LOCAL_MACHINE, registryPath, 0, NULL, 0, KEY_WRITE, 0, &hKey, &dwDisp));

    if(SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, szCategory, 0, REG_DWORD, (LPBYTE)&pKFD->category, sizeof(pKFD->category)));

        if(SUCCEEDED(hr) && pKFD->dwAttributes != 0)
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, szAttributes, 0, REG_DWORD, (LPBYTE)&pKFD->dwAttributes, sizeof(pKFD->dwAttributes)));

        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, szName, 0, REG_SZ, (LPBYTE)pKFD->pszName, (lstrlenW(pKFD->pszName)+1)*sizeof(WCHAR) ));

        if(SUCCEEDED(hr) && pKFD->pszParsingName)
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, szParsingName, 0, REG_SZ, (LPBYTE)pKFD->pszParsingName, (lstrlenW(pKFD->pszParsingName)+1)*sizeof(WCHAR) ));

        if(SUCCEEDED(hr) && !IsEqualGUID(&pKFD->fidParent, &GUID_NULL))
        {
            WCHAR sParentGuid[39];
            StringFromGUID2(&pKFD->fidParent, sParentGuid, ARRAY_SIZE(sParentGuid));

            /* this known folder has parent folder */
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, szParentFolder, 0, REG_SZ, (LPBYTE)sParentGuid, sizeof(sParentGuid)));
        }

        if(SUCCEEDED(hr) && pKFD->category != KF_CATEGORY_VIRTUAL && pKFD->pszRelativePath)
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, szRelativePath, 0, REG_SZ, (LPBYTE)pKFD->pszRelativePath, (lstrlenW(pKFD->pszRelativePath)+1)*sizeof(WCHAR) ));

        RegCloseKey(hKey);

        if(FAILED(hr))
            SHDeleteKeyW(HKEY_LOCAL_MACHINE, registryPath);
    }

    heap_free(registryPath);
    return hr;
}

static HRESULT WINAPI foldermanager_RegisterFolder(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid,
    KNOWNFOLDER_DEFINITION const *pKFD)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(rfid), pKFD);
    return register_folder(rfid, pKFD);
}

static HRESULT WINAPI foldermanager_UnregisterFolder(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid)
{
    HRESULT hr;
    LPWSTR registryPath = NULL;
    TRACE("(%p, %s)\n", iface, debugstr_guid(rfid));

    hr = get_known_folder_registry_path(rfid, NULL, &registryPath);

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(SHDeleteKeyW(HKEY_LOCAL_MACHINE, registryPath));

    heap_free(registryPath);
    return hr;
}

static HRESULT WINAPI foldermanager_FindFolderFromPath(
    IKnownFolderManager *iface,
    LPCWSTR pszPath,
    FFFP_MODE mode,
    IKnownFolder **ppkf)
{
    FIXME("%s, 0x%08x, %p\n", debugstr_w(pszPath), mode, ppkf);
    return E_NOTIMPL;
}

static HRESULT WINAPI foldermanager_FindFolderFromIDList(
    IKnownFolderManager *iface,
    PCIDLIST_ABSOLUTE pidl,
    IKnownFolder **ppkf)
{
    FIXME("%p, %p\n", pidl, ppkf);
    return E_NOTIMPL;
}

static HRESULT WINAPI foldermanager_Redirect(
    IKnownFolderManager *iface,
    REFKNOWNFOLDERID rfid,
    HWND hwnd,
    KF_REDIRECT_FLAGS flags,
    LPCWSTR pszTargetPath,
    UINT cFolders,
    KNOWNFOLDERID const *pExclusion,
    LPWSTR *ppszError)
{
    return redirect_known_folder(rfid, hwnd, flags, pszTargetPath, cFolders, pExclusion, ppszError);
}

static const struct IKnownFolderManagerVtbl foldermanager_vtbl =
{
    foldermanager_QueryInterface,
    foldermanager_AddRef,
    foldermanager_Release,
    foldermanager_FolderIdFromCsidl,
    foldermanager_FolderIdToCsidl,
    foldermanager_GetFolderIds,
    foldermanager_GetFolder,
    foldermanager_GetFolderByName,
    foldermanager_RegisterFolder,
    foldermanager_UnregisterFolder,
    foldermanager_FindFolderFromPath,
    foldermanager_FindFolderFromIDList,
    foldermanager_Redirect
};

static HRESULT foldermanager_create( void **ppv )
{
    UINT i, j;
    struct foldermanager *fm;

    fm = heap_alloc( sizeof(*fm) );
    if (!fm) return E_OUTOFMEMORY;

    fm->IKnownFolderManager_iface.lpVtbl = &foldermanager_vtbl;
    fm->refs = 1;
    fm->num_ids = 0;

    for (i = 0; i < ARRAY_SIZE(CSIDL_Data); i++)
    {
        if (!IsEqualGUID( CSIDL_Data[i].id, &GUID_NULL )) fm->num_ids++;
    }
    fm->ids = heap_alloc( fm->num_ids * sizeof(KNOWNFOLDERID) );
    if (!fm->ids)
    {
        heap_free( fm );
        return E_OUTOFMEMORY;
    }
    for (i = j = 0; i < ARRAY_SIZE(CSIDL_Data); i++)
    {
        if (!IsEqualGUID( CSIDL_Data[i].id, &GUID_NULL ))
        {
            fm->ids[j] = *CSIDL_Data[i].id;
            j++;
        }
    }
    TRACE("found %u known folders\n", fm->num_ids);
    *ppv = &fm->IKnownFolderManager_iface;

    TRACE("returning iface %p\n", *ppv);
    return S_OK;
}

HRESULT WINAPI KnownFolderManager_Constructor( IUnknown *punk, REFIID riid, void **ppv )
{
    TRACE("%p, %s, %p\n", punk, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    if (punk)
        return CLASS_E_NOAGGREGATION;

    return foldermanager_create( ppv );
}

HRESULT WINAPI SHGetKnownFolderIDList(REFKNOWNFOLDERID rfid, DWORD flags, HANDLE token, PIDLIST_ABSOLUTE *pidl)
{
    TRACE("%s, 0x%08x, %p, %p\n", debugstr_guid(rfid), flags, token, pidl);

    if (!pidl)
        return E_INVALIDARG;

    if (flags)
        FIXME("unsupported flags: 0x%08x\n", flags);

    if (token)
        FIXME("user token is not used.\n");

    *pidl = NULL;
    if (IsEqualIID(rfid, &FOLDERID_Desktop))
        *pidl = _ILCreateDesktop();
    else if (IsEqualIID(rfid, &FOLDERID_RecycleBinFolder))
        *pidl = _ILCreateBitBucket();
    else if (IsEqualIID(rfid, &FOLDERID_ComputerFolder))
        *pidl = _ILCreateMyComputer();
    else if (IsEqualIID(rfid, &FOLDERID_PrintersFolder))
        *pidl = _ILCreatePrinters();
    else if (IsEqualIID(rfid, &FOLDERID_ControlPanelFolder))
        *pidl = _ILCreateControlPanel();
    else if (IsEqualIID(rfid, &FOLDERID_NetworkFolder))
        *pidl = _ILCreateNetwork();
    else if (IsEqualIID(rfid, &FOLDERID_Documents))
        *pidl = _ILCreateMyDocuments();
    else
    {
        DWORD attributes = 0;
        WCHAR *pathW;
        HRESULT hr;

        hr = SHGetKnownFolderPath(rfid, flags, token, &pathW);
        if (FAILED(hr))
            return hr;

        hr = SHILCreateFromPathW(pathW, pidl, &attributes);
        CoTaskMemFree(pathW);
        return hr;
    }

    return *pidl ? S_OK : E_FAIL;
}

HRESULT WINAPI SHGetKnownFolderItem(REFKNOWNFOLDERID rfid, KNOWN_FOLDER_FLAG flags, HANDLE hToken,
    REFIID riid, void **ppv)
{
    PIDLIST_ABSOLUTE pidl;
    HRESULT hr;

    TRACE("%s, 0x%08x, %p, %s, %p\n", debugstr_guid(rfid), flags, hToken, debugstr_guid(riid), ppv);

    hr = SHGetKnownFolderIDList(rfid, flags, hToken, &pidl);
    if (FAILED(hr))
    {
        *ppv = NULL;
        return hr;
    }

    hr = SHCreateItemFromIDList(pidl, riid, ppv);
    CoTaskMemFree(pidl);
    return hr;
}

static void register_system_knownfolders(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(CSIDL_Data); ++i)
    {
        const CSIDL_DATA *folder = &CSIDL_Data[i];
        if(folder->pszName){
            KNOWNFOLDER_DEFINITION kfd;

            /* register_folder won't modify kfd, so cast away const instead of
             * reallocating */
            kfd.category = folder->category;
            kfd.pszName = (WCHAR*)folder->pszName;
            kfd.pszDescription = (WCHAR*)folder->pszDescription;
            memcpy(&kfd.fidParent, folder->fidParent, sizeof(KNOWNFOLDERID));
            kfd.pszRelativePath = (WCHAR*)folder->pszRelativePath;
            kfd.pszParsingName = (WCHAR*)folder->pszParsingName;
            kfd.pszTooltip = (WCHAR*)folder->pszTooltip;
            kfd.pszLocalizedName = (WCHAR*)folder->pszLocalizedName;
            kfd.pszIcon = (WCHAR*)folder->pszIcon;
            kfd.pszSecurity = (WCHAR*)folder->pszSecurity;
            kfd.dwAttributes = folder->dwAttributes;
            kfd.kfdFlags = folder->kfdFlags;
            memcpy(&kfd.ftidType, folder->ftidType, sizeof(FOLDERTYPEID));

            register_folder(folder->id, &kfd);
        }
    }
}

HRESULT SHELL_RegisterShellFolders(void)
{
    HRESULT hr;

    /* Set up '$HOME' targeted symlinks for 'My Documents', 'My Pictures',
     * 'My Videos', 'My Music', 'Desktop' etc. in advance, so that the
     * _SHRegister*ShellFolders() functions will find everything nice and clean
     * and thus will not attempt to create them in the profile directory. */
    _SHCreateSymbolicLinks();

    hr = _SHRegisterUserShellFolders(TRUE);
    if (SUCCEEDED(hr))
        hr = _SHRegisterUserShellFolders(FALSE);
    if (SUCCEEDED(hr))
        hr = _SHRegisterCommonShellFolders();
    if (SUCCEEDED(hr))
        hr = create_extra_folders();
    if (SUCCEEDED(hr))
        hr = set_folder_attributes();
    if (SUCCEEDED(hr))
        register_system_knownfolders();
    return hr;
}
