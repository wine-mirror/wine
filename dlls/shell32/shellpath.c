/*
 * Path Functions
 *
 * Copyright 1998, 1999, 2000 Juergen Schmied
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES:
 *
 * Many of these functions are in SHLWAPI.DLL also
 *
 */

#include "config.h"
#include "wine/port.h"

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
#include "shell32_main.h"
#include "undocshell.h"
#include "wine/unicode.h"
#include "shlwapi.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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
LPSTR WINAPI PathGetShortPathA(LPSTR lpszPath)
{
	FIXME("%s stub\n", lpszPath);
	return NULL;
}

/*************************************************************************
 * PathGetShortPathW [internal]
 */
LPWSTR WINAPI PathGetShortPathW(LPWSTR lpszPath)
{
	FIXME("%s stub\n", debugstr_w(lpszPath));
	return NULL;
}

/*************************************************************************
 * PathGetShortPath [SHELL32.92]
 */
LPVOID WINAPI PathGetShortPathAW(LPVOID lpszPath)
{
	if(SHELL_OsIsUnicode())
	  return PathGetShortPathW(lpszPath);
	return PathGetShortPathA(lpszPath);
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
	  if (!strcasecmp(lpszExtension,lpszExtensions[i])) return TRUE;

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
BOOL WINAPI PathMakeUniqueNameA(
	LPSTR lpszBuffer,
	DWORD dwBuffSize,
	LPCSTR lpszShortName,
	LPCSTR lpszLongName,
	LPCSTR lpszPathName)
{
	FIXME("%p %lu %s %s %s stub\n",
	 lpszBuffer, dwBuffSize, debugstr_a(lpszShortName),
	 debugstr_a(lpszLongName), debugstr_a(lpszPathName));
	return TRUE;
}

/*************************************************************************
 * PathMakeUniqueNameW	[internal]
 */
BOOL WINAPI PathMakeUniqueNameW(
	LPWSTR lpszBuffer,
	DWORD dwBuffSize,
	LPCWSTR lpszShortName,
	LPCWSTR lpszLongName,
	LPCWSTR lpszPathName)
{
	FIXME("%p %lu %s %s %s stub\n",
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
 *
 * NOTES
 *     exported by ordinal
 */
BOOL WINAPI PathYetAnotherMakeUniqueNameA(
	LPSTR lpszBuffer,
	LPCSTR lpszPathName,
	LPCSTR lpszShortName,
	LPCSTR lpszLongName)
{
    FIXME("(%p,%p, %p ,%p):stub.\n",
     lpszBuffer, lpszPathName, lpszShortName, lpszLongName);
    return TRUE;
}


/*
	########## cleaning and resolving paths ##########
 */

/*************************************************************************
 * PathFindOnPath	[SHELL32.145]
 */
BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID sOtherDirs)
{
	if (SHELL_OsIsUnicode())
	  return PathFindOnPathW(sFile, (LPCWSTR *)sOtherDirs);
	return PathFindOnPathA(sFile, (LPCSTR *)sOtherDirs);
}

/*************************************************************************
 * PathCleanupSpec	[SHELL32.171]
 */
DWORD WINAPI PathCleanupSpecAW (LPCVOID x, LPVOID y)
{
    FIXME("(%p, %p) stub\n",x,y);
    return TRUE;
}

/*************************************************************************
 * PathQualifyA		[SHELL32]
 */
BOOL WINAPI PathQualifyA(LPCSTR pszPath)
{
	FIXME("%s\n",pszPath);
	return 0;
}

/*************************************************************************
 * PathQualifyW		[SHELL32]
 */
BOOL WINAPI PathQualifyW(LPCWSTR pszPath)
{
	FIXME("%s\n",debugstr_w(pszPath));
	return 0;
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

/*************************************************************************
 * PathResolveA [SHELL32.51]
 */
BOOL WINAPI PathResolveA(
	LPSTR lpszPath,
	LPCSTR *alpszPaths,
	DWORD dwFlags)
{
	FIXME("(%s,%p,0x%08lx),stub!\n",
	  lpszPath, *alpszPaths, dwFlags);
	return 0;
}

/*************************************************************************
 * PathResolveW [SHELL32]
 */
BOOL WINAPI PathResolveW(
	LPWSTR lpszPath,
	LPCWSTR *alpszPaths,
	DWORD dwFlags)
{
	FIXME("(%s,%p,0x%08lx),stub!\n",
	  debugstr_w(lpszPath), debugstr_w(*alpszPaths), dwFlags);
	return 0;
}

/*************************************************************************
 * PathResolve [SHELL32.51]
 */
BOOL WINAPI PathResolveAW(
	LPVOID lpszPath,
	LPCVOID *alpszPaths,
	DWORD dwFlags)
{
	if (SHELL_OsIsUnicode())
	  return PathResolveW(lpszPath, (LPCWSTR*)alpszPaths, dwFlags);
	return PathResolveA(lpszPath, (LPCSTR*)alpszPaths, dwFlags);
}

/*************************************************************************
*	PathProcessCommandA	[SHELL32.653]
*/
HRESULT WINAPI PathProcessCommandA (
	LPCSTR lpszPath,
	LPSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("%s %p 0x%04lx 0x%04lx stub\n",
	lpszPath, lpszBuff, dwBuffSize, dwFlags);
	strcpy(lpszBuff, lpszPath);
	return 0;
}

/*************************************************************************
*	PathProcessCommandW
*/
HRESULT WINAPI PathProcessCommandW (
	LPCWSTR lpszPath,
	LPWSTR lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	FIXME("(%s, %p, 0x%04lx, 0x%04lx) stub\n",
	debugstr_w(lpszPath), lpszBuff, dwBuffSize, dwFlags);
	strcpyW(lpszBuff, lpszPath);
	return 0;
}

/*************************************************************************
*	PathProcessCommand (SHELL32.653)
*/
HRESULT WINAPI PathProcessCommandAW (
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


/*************************************************************************
 * SHGetSpecialFolderPathA [SHELL32.@]
 *
 * converts csidl to path
 */

static const char * const szSHFolders = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
static const char * const szSHUserFolders = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
static const char * const szSetup = "Software\\Microsoft\\Windows\\CurrentVersion\\Setup";
static const char * const szCurrentVersion = "Software\\Microsoft\\Windows\\CurrentVersion";
#if 0
static const char * const szEnvUserProfile = "%USERPROFILE%";
static const char * const szEnvSystemRoot = "%SYSTEMROOT%";
#endif

typedef struct
{
    DWORD dwFlags;
    HKEY hRootKey;
    LPCSTR szValueName;
    LPCSTR szDefaultPath; /* fallback string; sub dir of windows directory */
} CSIDL_DATA;

#define CSIDL_MYFLAG_SHFOLDER	1
#define CSIDL_MYFLAG_SETUP	2
#define CSIDL_MYFLAG_CURRVER	4
#define CSIDL_MYFLAG_RELATIVE	8

#define HKLM HKEY_LOCAL_MACHINE
#define HKCU HKEY_CURRENT_USER
static const CSIDL_DATA CSIDL_Data[] =
{
    { /* CSIDL_DESKTOP */
	9, HKCU,
	"Desktop",
	"Desktop"
    },
    { /* CSIDL_INTERNET */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_PROGRAMS */
	9, HKCU,
	"Programs",
	"Start Menu\\Programs"
    },
    { /* CSIDL_CONTROLS (.CPL files) */
	10, HKLM,
	"SysDir",
	"SYSTEM"
    },
    { /* CSIDL_PRINTERS */
	10, HKLM,
	"SysDir",
	"SYSTEM"
    },
    { /* CSIDL_PERSONAL */
	1, HKCU,
	"Personal",
	"My Documents"
    },
    { /* CSIDL_FAVORITES */
	9, HKCU,
	"Favorites",
	"Favorites"
    },
    { /* CSIDL_STARTUP */
	9, HKCU,
	"StartUp",
	"Start Menu\\Programs\\StartUp"
    },
    { /* CSIDL_RECENT */
	9, HKCU,
	"Recent",
	"Recent"
    },
    { /* CSIDL_SENDTO */
	9, HKCU,
	"SendTo",
	"SendTo"
    },
    { /* CSIDL_BITBUCKET (is this c:\recycled ?) */
	0, (HKEY)1, /* FIXME */
	NULL,
	"recycled"
    },
    { /* CSIDL_STARTMENU */
	9, HKCU,
	"Start Menu",
	"Start Menu"
    },
    { /* CSIDL_MYDOCUMENTS */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_MYMUSIC */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_MYVIDEO */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL,
    },
    { /* unassigned */
	0, 0,
	NULL,
	NULL,
    },
    { /* CSIDL_DESKTOPDIRECTORY */
	9, HKCU,
	"Desktop",
	"Desktop"
    },
    { /* CSIDL_DRIVES */
	0, (HKEY)1, /* FIXME */
	NULL,
	"My Computer"
    },
    { /* CSIDL_NETWORK */
	0, (HKEY)1, /* FIXME */
	NULL,
	"Network Neighborhood"
    },
    { /* CSIDL_NETHOOD */
	9, HKCU,
	"NetHood",
	"NetHood"
    },
    { /* CSIDL_FONTS */
	9, HKCU,
	"Fonts",
	"Fonts"
    },
    { /* CSIDL_TEMPLATES */
	9, HKCU,
	"Templates",
	"ShellNew"
    },
    { /* CSIDL_COMMON_STARTMENU */
	9, HKLM,
	"Common Start Menu",
	"Start Menu"
    },
    { /* CSIDL_COMMON_PROGRAMS */
	9, HKLM,
	"Common Programs",
	""
    },
    { /* CSIDL_COMMON_STARTUP */
	9, HKLM,
	"Common StartUp",
	"All Users\\Start Menu\\Programs\\StartUp"
    },
    { /* CSIDL_COMMON_DESKTOPDIRECTORY */
	9, HKLM,
	"Common Desktop",
	"Desktop"
    },
    { /* CSIDL_APPDATA */
	9, HKCU,
	"AppData",
	"Application Data"
    },
    { /* CSIDL_PRINTHOOD */
	9, HKCU,
	"PrintHood",
	"PrintHood"
    },
    { /* CSIDL_LOCAL_APPDATA (win2k only/undocumented) */
	1, HKCU,
	"Local AppData",
	"Local Settings\\Application Data",
    },
    { /* CSIDL_ALTSTARTUP */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_COMMON_ALTSTARTUP */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_COMMON_FAVORITES */
	9, HKCU,
	"Favorites",
	"Favorites"
    },
    { /* CSIDL_INTERNET_CACHE */
	9, HKCU,
	"Cache",
	"Temporary Internet Files"
    },
    { /* CSIDL_COOKIES */
	9, HKCU,
	"Cookies",
	"Cookies"
    },
    { /* CSIDL_HISTORY */
	9, HKCU,
	"History",
	"History"
    },
    { /* CSIDL_COMMON_APPDATA */
	9, HKLM,
	"Common AppData",
	"All Users\\Application Data"
    },
    { /* CSIDL_WINDOWS */
	2, HKLM,
	"WinDir",
	"Windows"
    },
    { /* CSIDL_SYSTEM */
	10, HKLM,
	"SysDir",
	"SYSTEM"
    },
    { /* CSIDL_PROGRAM_FILES */
	4, HKLM,
	"ProgramFilesDir",
	"Program Files"
    },
    { /* CSIDL_MYPICTURES */
	1, HKCU,
	"My Pictures",
	"My Documents\\My Pictures"
    },
    { /* CSIDL_PROFILE */
	10, HKLM,
	"WinDir", /* correct ? */
	""
    },
    { /* CSIDL_SYSTEMX86 */
	10, HKLM,
 	"SysDir",
	"SYSTEM"
    },
    { /* CSIDL_PROGRAM_FILESX86 */
	4, HKLM,
	"ProgramFilesDir",
	"Program Files"
    },
    { /* CSIDL_PROGRAM_FILES_COMMON */
	4, HKLM,
	"CommonFilesDir",
	"Program Files\\Common Files" /* ? */
    },
    { /* CSIDL_PROGRAM_FILES_COMMONX86 */
	4, HKLM,
	"CommonFilesDir",
	"Program Files\\Common Files" /* ? */
    },
    { /* CSIDL_COMMON_TEMPLATES */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_COMMON_DOCUMENTS */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_COMMON_ADMINTOOLS */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL
    },
    { /* CSIDL_ADMINTOOLS */
	9, HKCU,
	"Administrative Tools",
	"Start Menu\\Programs\\Administrative Tools"
    },
    { /* CSIDL_CONNECTIONS */
	0, (HKEY)1, /* FIXME */
	NULL,
	NULL
    },
    { /* unassigned 32*/
	0, 0,
	NULL,
	NULL,
    },
    { /* unassigned 33*/
	0, 0,
	NULL,
	NULL,
    },
    { /* unassigned 34*/
	0, 0,
	NULL,
	NULL,
    },
    { /* CSIDL_COMMON_MUSIC */
	0, 0, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_COMMON_PICTURES */
	0, 0, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_COMMON_VIDEO */
	0, 0, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_RESOURCES */
	0, 0, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_RESOURCES_LOCALIZED */
	0, 0, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_COMMON_OEM_LINKS */
	0, 0, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_CDBURN_AREA */
	0, 0, /* FIXME */
	NULL,
	NULL,
    },
    { /* unassigned 3C */
	0, 0,
	NULL,
	NULL,
    },
    { /* CSIDL_COMPUTERSNEARME */
	0, 0, /* FIXME */
	NULL,
	NULL,
    },
    { /* CSIDL_PROFILES */
	0, 0, /* FIXME */
	NULL,
	NULL,
    }
};
#undef HKCU
#undef HKLM

/**********************************************************************/

BOOL WINAPI SHGetSpecialFolderPathA (
	HWND hwndOwner,
	LPSTR szPath,
	int csidl,
	BOOL bCreate)
{
	CHAR	szValueName[MAX_PATH], szDefaultPath[MAX_PATH], szBuildPath[MAX_PATH];
	HKEY	hRootKey, hKey;
	DWORD	dwFlags;
	DWORD	dwType, dwDisp, dwPathLen = MAX_PATH;
	DWORD	folder = csidl & CSIDL_FOLDER_MASK;
	CHAR	*p;

	TRACE("%p,%p,csidl=%d,0x%04x\n", hwndOwner,szPath,csidl,bCreate);

	if ((folder >= sizeof(CSIDL_Data) / sizeof(CSIDL_Data[0])) ||
	    (CSIDL_Data[folder].hRootKey == 0))
	{
	    ERR("folder 0x%04lx unknown or not allowed\n", folder);
	    return FALSE;
	}
	if (CSIDL_Data[folder].hRootKey == (HKEY)1)
	{
	    FIXME("folder 0x%04lx unknown, please add.\n", folder);
	    return FALSE;
	}

	dwFlags = CSIDL_Data[folder].dwFlags;
	hRootKey = CSIDL_Data[folder].hRootKey;
	strcpy(szValueName, CSIDL_Data[folder].szValueName);
	strcpy(szDefaultPath, CSIDL_Data[folder].szDefaultPath);

	if (dwFlags & CSIDL_MYFLAG_SHFOLDER)
	{
	  /*   user shell folders */
	  if   (RegCreateKeyExA(hRootKey,szSHUserFolders,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)) return FALSE;

	  if   (RegQueryValueExA(hKey,szValueName,NULL,&dwType,(LPBYTE)szPath,&dwPathLen))
	  {
	    RegCloseKey(hKey);

	    /* shell folders */
	    if (RegCreateKeyExA(hRootKey,szSHFolders,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)) return FALSE;

	    if (RegQueryValueExA(hKey,szValueName,NULL,&dwType,(LPBYTE)szPath,&dwPathLen))
	    {

	      /* value not existing */
	      if (dwFlags & CSIDL_MYFLAG_RELATIVE)
	      {
	        GetWindowsDirectoryA(szPath, MAX_PATH);
	        PathAddBackslashA(szPath);
	        strcat(szPath, szDefaultPath);
	      }
	      else
	      {
	        strcpy(szPath, "C:\\");	/* FIXME ??? */
	        strcat(szPath, szDefaultPath);
	      }
	      RegSetValueExA(hKey,szValueName,0,REG_SZ,(LPBYTE)szPath,strlen(szPath)+1);
	    }
	  }
	  RegCloseKey(hKey);
        }
	else
	{
	  LPCSTR pRegPath;

	  if (dwFlags & CSIDL_MYFLAG_SETUP)
	    pRegPath = szSetup;
	  else
	  if (dwFlags & CSIDL_MYFLAG_CURRVER)
	    pRegPath = szCurrentVersion;
	  else
	  {
	    ERR("folder settings broken, please correct !\n");
	    return FALSE;
	  }

	  if   (RegCreateKeyExA(hRootKey,pRegPath,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)) return FALSE;

	  if   (RegQueryValueExA(hKey,szValueName,NULL,&dwType,(LPBYTE)szPath,&dwPathLen))
	  {
	    /* value not existing */
	    if (dwFlags & CSIDL_MYFLAG_RELATIVE)
	    {
	      GetWindowsDirectoryA(szPath, MAX_PATH);
	      PathAddBackslashA(szPath);
	      strcat(szPath, szDefaultPath);
	    }
	    else
	    {
	      strcpy(szPath, "C:\\");	/* FIXME ??? */
	      strcat(szPath, szDefaultPath);
	    }
	    RegSetValueExA(hKey,szValueName,0,REG_SZ,(LPBYTE)szPath,strlen(szPath)+1);
	  }
	  RegCloseKey(hKey);
	}

	/* expand paths like %USERPROFILE% */
	if (dwType == REG_EXPAND_SZ)
	{
	  ExpandEnvironmentStringsA(szPath, szDefaultPath, MAX_PATH);
	  strcpy(szPath, szDefaultPath);
	}

	/* if we don't care about existing directories we are ready */
	if(csidl & CSIDL_FLAG_DONT_VERIFY) return TRUE;

	if (PathFileExistsA(szPath)) return TRUE;

	/* not existing but we are not allowed to create it */
	if (!bCreate) return FALSE;

	/* create directory/directories */
	strcpy(szBuildPath, szPath);
	p = strchr(szBuildPath, '\\');
	while (p)
	{
	    *p = 0;
	    if (!PathFileExistsA(szBuildPath))
	    {
		if (!CreateDirectoryA(szBuildPath,NULL))
		{
		    ERR("Failed to create directory '%s'.\n", szPath);
		    return FALSE;
		}
	    }
	    *p = '\\';
	    p = strchr(p+1, '\\');
	}
	/* last component must be created too. */
	if (!PathFileExistsA(szBuildPath))
	{
	    if (!CreateDirectoryA(szBuildPath,NULL))
	    {
		ERR("Failed to create directory '%s'.\n", szPath);
		return FALSE;
	    }
	}

	TRACE("Created missing system directory '%s'\n", szPath);
	return TRUE;
}

/*************************************************************************
 * SHGetSpecialFolderPathW
 */
BOOL WINAPI SHGetSpecialFolderPathW (
	HWND hwndOwner,
	LPWSTR szPath,
	int csidl,
	BOOL bCreate)
{
	char szTemp[MAX_PATH];

	if (SHGetSpecialFolderPathA(hwndOwner, szTemp, csidl, bCreate))
	{
            if (!MultiByteToWideChar( CP_ACP, 0, szTemp, -1, szPath, MAX_PATH ))
                szPath[MAX_PATH-1] = 0;
        }

	TRACE("%p,%p,csidl=%d,0x%04x\n", hwndOwner,szPath,csidl,bCreate);

	return TRUE;
}

/*************************************************************************
 * SHGetSpecialFolderPath (SHELL32.175)
 */
BOOL WINAPI SHGetSpecialFolderPathAW (
	HWND hwndOwner,
	LPVOID szPath,
	int csidl,
	BOOL bCreate)

{
	if (SHELL_OsIsUnicode())
	  return SHGetSpecialFolderPathW (hwndOwner, szPath, csidl, bCreate);
	return SHGetSpecialFolderPathA (hwndOwner, szPath, csidl, bCreate);
}

/*************************************************************************
 * SHGetFolderPathA			[SHELL32.@]
 */
HRESULT WINAPI SHGetFolderPathA(
	HWND hwndOwner,
	int nFolder,
	HANDLE hToken,	/* [in] FIXME: get paths for specific user */
	DWORD dwFlags,	/* [in] FIXME: SHGFP_TYPE_CURRENT|SHGFP_TYPE_DEFAULT */
	LPSTR pszPath)
{
	return (SHGetSpecialFolderPathA(
		hwndOwner,
		pszPath,
		CSIDL_FOLDER_MASK & nFolder,
		CSIDL_FLAG_CREATE & nFolder )) ? S_OK : E_FAIL;
}

/*************************************************************************
 * SHGetFolderPathW			[SHELL32.@]
 */
HRESULT WINAPI SHGetFolderPathW(
	HWND hwndOwner,
	int nFolder,
	HANDLE hToken,
	DWORD dwFlags,
	LPWSTR pszPath)
{
	return (SHGetSpecialFolderPathW(
		hwndOwner,
		pszPath,
		CSIDL_FOLDER_MASK & nFolder,
		CSIDL_FLAG_CREATE & nFolder )) ? S_OK : E_FAIL;
}
