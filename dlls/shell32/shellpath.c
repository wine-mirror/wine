/*
 * Path Functions
 *
 * Many of this functions are in SHLWAPI.DLL also
 *
 */
#include <string.h>
#include <ctype.h>
#include "debugtools.h"
#include "winnls.h"
#include "winversion.h"
#include "winreg.h"

#include "shlobj.h"
#include "shell32_main.h"
#include "windef.h"
#include "options.h"
#include "wine/undocshell.h"
#include "wine/unicode.h"
#include "shlwapi.h"

DEFAULT_DEBUG_CHANNEL(shell);

#define isSlash(x) ((x)=='\\' || (x)=='/')
/*
	########## Combining and Constructing paths ##########
*/

/*************************************************************************
 * PathAppendAW		[SHELL32.36]
 */
BOOL WINAPI PathAppendAW(
	LPVOID lpszPath1,
	LPCVOID lpszPath2)
{
	if (VERSION_OsIsUnicode())
	  return PathAppendW(lpszPath1, lpszPath2);
	return PathAppendA(lpszPath1, lpszPath2);
}

/*************************************************************************
 * PathCombineAW	 [SHELL32.37]
 */
LPVOID WINAPI PathCombineAW(
	LPVOID szDest,
	LPCVOID lpszDir,
	LPCVOID lpszFile) 
{
	if (VERSION_OsIsUnicode())
	  return PathCombineW( szDest, lpszDir, lpszFile );
	return PathCombineA( szDest, lpszDir, lpszFile );
}

/*************************************************************************
 * PathAddBackslashAW		[SHELL32.32]
 */
LPVOID WINAPI PathAddBackslashAW(LPVOID lpszPath)
{
	if(VERSION_OsIsUnicode())
	  return PathAddBackslashW(lpszPath);
	return PathAddBackslashA(lpszPath);
}

/*************************************************************************
 * PathBuildRootAW		[SHELL32.30]
 */
LPVOID WINAPI PathBuildRootAW(LPVOID lpszPath, int drive)
{
	if(VERSION_OsIsUnicode())
	  return PathBuildRootW(lpszPath, drive);
	return PathBuildRootA(lpszPath, drive);
}

/*
	Extracting Component Parts
*/

/*************************************************************************
 * PathFindFileNameAW	[SHELL32.34]
 */
LPVOID WINAPI PathFindFileNameAW(LPCVOID lpszPath)
{
	if(VERSION_OsIsUnicode())
	  return PathFindFileNameW(lpszPath);
	return PathFindFileNameA(lpszPath);
}

/*************************************************************************
 * PathFindExtensionAW		[SHELL32.31]
 */
LPVOID WINAPI PathFindExtensionAW(LPCVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathFindExtensionW(lpszPath);
	return PathFindExtensionA(lpszPath);

}

/*************************************************************************
 * PathGetExtensionAW		[SHELL32.158]
 */
LPVOID WINAPI PathGetExtensionAW(LPCVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathGetExtensionW(lpszPath);
	return PathGetExtensionA(lpszPath);
}

/*************************************************************************
 * PathGetArgsAW	[SHELL32.52]
 */
LPVOID WINAPI PathGetArgsAW(LPVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathGetArgsW(lpszPath);
	return PathGetArgsA(lpszPath);
}

/*************************************************************************
 * PathGetDriveNumber	[SHELL32.57]
 */
int WINAPI PathGetDriveNumberAW(LPVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathGetDriveNumberW(lpszPath);
	return PathGetDriveNumberA(lpszPath);
}

/*************************************************************************
 * PathRemoveFileSpec [SHELL32.35]
 */
BOOL WINAPI PathRemoveFileSpecAW(LPVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathRemoveFileSpecW(lpszPath);
	return PathRemoveFileSpecA(lpszPath);
}

/*************************************************************************
 * PathStripPathAW	[SHELL32.38]
 */
void WINAPI PathStripPathAW(LPVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathStripPathW(lpszPath);
	return PathStripPathA(lpszPath);
}

/*************************************************************************
 * PathStripToRootAW	[SHELL32.50]
 */
BOOL WINAPI PathStripToRootAW(LPVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathStripToRootW(lpszPath);
	return PathStripToRootA(lpszPath);
}

/*************************************************************************
 * PathRemoveArgsAW	[SHELL32.251]
 */
void WINAPI PathRemoveArgsAW(LPVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  PathRemoveArgsW(lpszPath);
	PathRemoveArgsA(lpszPath);
}

/*************************************************************************
 * PathRemoveExtensionAW	[SHELL32.250]
 */
void WINAPI PathRemoveExtensionAW(LPVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathRemoveExtensionW(lpszPath);
	return PathRemoveExtensionA(lpszPath);
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
 * PathGetShortPathAW [SHELL32.92]
 */
LPVOID WINAPI PathGetShortPathAW(LPVOID lpszPath)
{
	if(VERSION_OsIsUnicode())
	  return PathGetShortPathW(lpszPath);
	return PathGetShortPathA(lpszPath);
}

/*************************************************************************
 * PathRemoveBlanksAW [SHELL32.33]
 */
void WINAPI PathRemoveBlanksAW(LPVOID str)
{
	if(VERSION_OsIsUnicode())
	  PathRemoveBlanksW(str);
	PathRemoveBlanksA(str);
}

/*************************************************************************
 * PathQuoteSpacesAW [SHELL32.55]
 */
LPVOID WINAPI PathQuoteSpacesAW (LPVOID lpszPath)
{
	if(VERSION_OsIsUnicode())
	  return PathQuoteSpacesW(lpszPath);
	return PathQuoteSpacesA(lpszPath);
}

/*************************************************************************
 * PathUnquoteSpacesAW [SHELL32.56]
 */
VOID WINAPI PathUnquoteSpacesAW(LPVOID str) 
{
	if(VERSION_OsIsUnicode())
	  PathUnquoteSpacesW(str);
	else
	  PathUnquoteSpacesA(str);
}

/*************************************************************************
 * PathParseIconLocationAW	[SHELL32.249]
 */
int WINAPI PathParseIconLocationAW (LPVOID lpszPath)
{
	if(VERSION_OsIsUnicode())
	  return PathParseIconLocationW(lpszPath);
	return PathParseIconLocationA(lpszPath);
}

/*
	########## Path Testing ##########
*/
/*************************************************************************
 * PathIsUNCAW		[SHELL32.39]
 */
BOOL WINAPI PathIsUNCAW (LPCVOID lpszPath)
{
	if (VERSION_OsIsUnicode())
	  return PathIsUNCW( lpszPath );
	return PathIsUNCA( lpszPath );  
}

/*************************************************************************
 *  PathIsRelativeAW	[SHELL32.40]
 */
BOOL WINAPI PathIsRelativeAW (LPCVOID lpszPath)
{
	if (VERSION_OsIsUnicode())
	  return PathIsRelativeW( lpszPath );
	return PathIsRelativeA( lpszPath );  
}

/*************************************************************************
 * PathIsRootAW		[SHELL32.29]
 */
BOOL WINAPI PathIsRootAW(LPCVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathIsRootW(lpszPath);
	return PathIsRootA(lpszPath);
}

/*************************************************************************
 *  PathIsExeAW		[SHELL32.43]
 */
BOOL WINAPI PathIsExeAW (LPCVOID path)
{
	if (VERSION_OsIsUnicode())
	  return PathIsExeW (path);
	return PathIsExeA(path);
}

/*************************************************************************
 * PathIsDirectoryAW	[SHELL32.159]
 */
BOOL WINAPI PathIsDirectoryAW (LPCVOID lpszPath)
{
	if (VERSION_OsIsUnicode())
	  return PathIsDirectoryW (lpszPath);
	return PathIsDirectoryA (lpszPath);
}

/*************************************************************************
 * PathFileExistsAW	[SHELL32.45]
 */ 
BOOL WINAPI PathFileExistsAW (LPCVOID lpszPath)
{
	if (VERSION_OsIsUnicode())
	  return PathFileExistsW (lpszPath);
	return PathFileExistsA (lpszPath);
}

/*************************************************************************
 * PathMatchSpecAW	[SHELL32.46]
 */
BOOL WINAPI PathMatchSpecAW(LPVOID name, LPVOID mask) 
{
	if (VERSION_OsIsUnicode())
	  return PathMatchSpecW( name, mask );
	return PathMatchSpecA( name, mask );
}

/*************************************************************************
 * PathIsSameRootAW	[SHELL32.650]
 */
BOOL WINAPI PathIsSameRootAW(LPCVOID lpszPath1, LPCVOID lpszPath2)
{
	if (VERSION_OsIsUnicode())
	  return PathIsSameRootW(lpszPath1, lpszPath2);
	return PathIsSameRootA(lpszPath1, lpszPath2);
}

/*************************************************************************
 * IsLFNDriveA		[SHELL32.119]
 * 
 * NOTES
 *     exported by ordinal Name
 */
BOOL WINAPI IsLFNDriveA(LPCSTR lpszPath) 
{
    DWORD	fnlen;

    if (!GetVolumeInformationA(lpszPath,NULL,0,NULL,&fnlen,NULL,NULL,0))
	return FALSE;
    return fnlen>12;
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
 * PathMakeUniqueNameAW	[SHELL32.47]
 */
BOOL WINAPI PathMakeUniqueNameAW(
	LPVOID lpszBuffer,
	DWORD dwBuffSize, 
	LPCVOID lpszShortName,
	LPCVOID lpszLongName,
	LPCVOID lpszPathName)
{
	if (VERSION_OsIsUnicode())
	  return PathMakeUniqueNameW(lpszBuffer,dwBuffSize, lpszShortName,lpszLongName,lpszPathName);
	return PathMakeUniqueNameA(lpszBuffer,dwBuffSize, lpszShortName,lpszLongName,lpszPathName);
}

/*************************************************************************
 * PathYetAnotherMakeUniqueNameA [SHELL32.75]
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
 * PathFindOnPathAW	[SHELL32]
 */
BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID sOtherDirs)
{
	if (VERSION_OsIsUnicode())
	  return PathFindOnPathW(sFile, sOtherDirs);
	return PathFindOnPathA(sFile, sOtherDirs);
}

/*************************************************************************
 * PathCleanupSpecAW	[SHELL32]
 */
DWORD WINAPI PathCleanupSpecAW (LPVOID x, LPVOID y)
{
	if (VERSION_OsIsUnicode())
	  return PathCleanupSpecW(x,y);
	return PathCleanupSpecA(x,y);
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
 * PathQualifyAW	[SHELL32]
 */
BOOL WINAPI PathQualifyAW(LPCVOID pszPath) 
{
	if (VERSION_OsIsUnicode())
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
 * PathResolveAW [SHELL32]
 */
BOOL WINAPI PathResolveAW(
	LPVOID lpszPath,
	LPCVOID *alpszPaths, 
	DWORD dwFlags)
{
	if (VERSION_OsIsUnicode())
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
	lstrcpyA(lpszBuff, lpszPath);
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
	lstrcpyW(lpszBuff, lpszPath);
	return 0;
}

/*************************************************************************
*	PathProcessCommandAW
*/
HRESULT WINAPI PathProcessCommandAW (
	LPCVOID lpszPath,
	LPVOID lpszBuff,
	DWORD dwBuffSize,
	DWORD dwFlags)
{
	if (VERSION_OsIsUnicode())
	  return PathProcessCommandW(lpszPath, lpszBuff, dwBuffSize, dwFlags);
	return PathProcessCommandA(lpszPath, lpszBuff, dwBuffSize, dwFlags);
}

/*
	########## special ##########
*/

/*************************************************************************
 * PathSetDlgItemPathAW
 */
BOOL WINAPI PathSetDlgItemPathAW(HWND hDlg, int id, LPCVOID pszPath) 
{	if (VERSION_OsIsUnicode())
	  return PathSetDlgItemPathW(hDlg, id, pszPath);
	return PathSetDlgItemPathA(hDlg, id, pszPath);
}


/*************************************************************************
 * SHGetSpecialFolderPathA [SHELL32.175]
 * 
 * converts csidl to path
 */
 
static char * szSHFolders = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
static char * szSHUserFolders = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
#if 0
static char * szEnvUserProfile = "%USERPROFILE%";
static char * szEnvSystemRoot = "%SYSTEMROOT%";
#endif

BOOL WINAPI SHGetSpecialFolderPathA (
	HWND hwndOwner,
	LPSTR szPath,
	DWORD csidl,
	BOOL bCreate)
{
	CHAR	szValueName[MAX_PATH], szDefaultPath[MAX_PATH];
	HKEY	hRootKey, hKey;
	BOOL	bRelative = TRUE;
	DWORD	dwType, dwDisp, dwPathLen = MAX_PATH;

	TRACE("0x%04x,%p,csidl=%lu,0x%04x\n", hwndOwner,szPath,csidl,bCreate);

	/* build default values */
	switch(csidl)
	{
	  case CSIDL_APPDATA:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy (szValueName, "AppData");
	    strcpy (szDefaultPath, "AppData");
	    break;

	  case CSIDL_COOKIES:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy (szValueName, "Cookies");
	    strcpy(szDefaultPath, "Cookies");
	    break;

	  case CSIDL_DESKTOPDIRECTORY:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Desktop");
	    strcpy(szDefaultPath, "Desktop");
	    break;

	  case CSIDL_COMMON_DESKTOPDIRECTORY:
	    hRootKey = HKEY_LOCAL_MACHINE;
	    strcpy(szValueName, "Common Desktop");
	    strcpy(szDefaultPath, "Desktop");
	    break;

	  case CSIDL_FAVORITES:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Favorites");
	    strcpy(szDefaultPath, "Favorites");
	    break;

	  case CSIDL_FONTS:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Fonts");
	    strcpy(szDefaultPath, "Fonts");
	    break;

	  case CSIDL_HISTORY:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "History");
	    strcpy(szDefaultPath, "History");
	    break;

	  case CSIDL_NETHOOD:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "NetHood");
	    strcpy(szDefaultPath, "NetHood");
	    break;

	  case CSIDL_INTERNET_CACHE:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Cache");
	    strcpy(szDefaultPath, "Temporary Internet Files");
	    break;

	  case CSIDL_PERSONAL:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Personal");
	    strcpy(szDefaultPath, "My Own Files");
	    bRelative = FALSE;
	    break;

	  case CSIDL_PRINTHOOD:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "PrintHood");
	    strcpy(szDefaultPath, "PrintHood");
	    break;

	  case CSIDL_PROGRAMS:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Programs");
	    strcpy(szDefaultPath, "StartMenu\\Programs");
	    break;

	  case CSIDL_COMMON_PROGRAMS:
	    hRootKey = HKEY_LOCAL_MACHINE;
	    strcpy(szValueName, "Common Programs");
	    strcpy(szDefaultPath, "");
	    break;

	  case CSIDL_RECENT:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Recent");
	    strcpy(szDefaultPath, "Recent");
	    break;

	  case CSIDL_SENDTO:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "SendTo");
	    strcpy(szDefaultPath, "SendTo");
	    break;

	  case CSIDL_STARTMENU:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "StartMenu");
	    strcpy(szDefaultPath, "StartMenu");
	    break;

	  case CSIDL_COMMON_STARTMENU:
	    hRootKey = HKEY_LOCAL_MACHINE;
	    strcpy(szValueName, "Common StartMenu");
	    strcpy(szDefaultPath, "StartMenu");
	    break;

	  case CSIDL_STARTUP:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Startup");
	    strcpy(szDefaultPath, "StartMenu\\Programs\\Startup");
	    break;

	  case CSIDL_COMMON_STARTUP:
	    hRootKey = HKEY_LOCAL_MACHINE;
	    strcpy(szValueName, "Common Startup");
	    strcpy(szDefaultPath, "StartMenu\\Programs\\Startup");
	    break;

	  case CSIDL_TEMPLATES:
	    hRootKey = HKEY_CURRENT_USER;
	    strcpy(szValueName, "Templates");
	    strcpy(szDefaultPath, "ShellNew");
	    break;

	  default:
	    ERR("folder unknown or not allowed\n");
	    return FALSE;
	}

	/* user shell folders */
	if (RegCreateKeyExA(hRootKey,szSHUserFolders,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)) return FALSE;

	if (RegQueryValueExA(hKey,szValueName,NULL,&dwType,(LPBYTE)szPath,&dwPathLen))
	{
	  RegCloseKey(hKey);

	  /* shell folders */
	  if (RegCreateKeyExA(hRootKey,szSHFolders,0,NULL,0,KEY_ALL_ACCESS,NULL,&hKey,&dwDisp)) return FALSE;

	  if (RegQueryValueExA(hKey,szValueName,NULL,&dwType,(LPBYTE)szPath,&dwPathLen))
	  {

	    /* value not existing */
	    if (bRelative)
	    {
	      GetWindowsDirectoryA(szPath, MAX_PATH);
	      PathAddBackslashA(szPath);
	      strcat(szPath, szDefaultPath);
	    }
	    else
	    {
	      strcpy(szPath, "C:\\");	/* fixme ??? */
	      strcat(szPath, szDefaultPath);
	    }
	    RegSetValueExA(hKey,szValueName,0,REG_SZ,(LPBYTE)szPath,strlen(szPath)+1);
	  }
	}
	RegCloseKey(hKey);

	/* expand paths like %USERPROFILE% */
	if (dwType == REG_EXPAND_SZ)
	{
	  ExpandEnvironmentStringsA(szPath, szDefaultPath, MAX_PATH);
	  strcpy(szPath, szDefaultPath);
	}

	/* if we don't care about existing directorys we are ready */
	if(csidl & CSIDL_FLAG_DONT_VERIFY) return TRUE;

	if (PathFileExistsA(szPath)) return TRUE;

	/* not existing but we not allowed to create it */
	if (!bCreate) return FALSE;
	
	if (!CreateDirectoryA(szPath,NULL))
	{
	  ERR("Failed to create directory '%s'.\n", szPath);
	  return FALSE;
	}

	MESSAGE("Created not existing system directory '%s'\n", szPath);
	return TRUE;
}

/*************************************************************************
 * SHGetSpecialFolderPathW
 */
BOOL WINAPI SHGetSpecialFolderPathW (
	HWND hwndOwner,
	LPWSTR szPath,
	DWORD csidl,
	BOOL bCreate)
{
	char szTemp[MAX_PATH];
	
	if (SHGetSpecialFolderPathA(hwndOwner, szTemp, csidl, bCreate))
	{
	  lstrcpynAtoW(szPath, szTemp, MAX_PATH);
	}

	TRACE("0x%04x,%p,csidl=%lu,0x%04x\n", hwndOwner,szPath,csidl,bCreate);

	return TRUE;
}

/*************************************************************************
 * SHGetSpecialFolderPathAW
 */
BOOL WINAPI SHGetSpecialFolderPathAW (
	HWND hwndOwner,
	LPVOID szPath,
	DWORD csidl,
	BOOL bCreate)

{
	if (VERSION_OsIsUnicode())
	  return SHGetSpecialFolderPathW (hwndOwner, szPath, csidl, bCreate);
	return SHGetSpecialFolderPathA (hwndOwner, szPath, csidl, bCreate);
}

/*************************************************************************
 * SHGetFolderPathA			[SHFOLDER.@]
 */
HRESULT WINAPI SHGetFolderPathA(
	HWND hwndOwner,
	int nFolder,
	HANDLE hToken,	/* FIXME: get paths for specific user */
	DWORD dwFlags,	/* FIXME: SHGFP_TYPE_CURRENT|SHGFP_TYPE_DEFAULT */
	LPSTR pszPath)
{
	return (SHGetSpecialFolderPathA(
		hwndOwner,
		pszPath,
		CSIDL_FOLDER_MASK & nFolder,
		CSIDL_FLAG_CREATE & nFolder )) ? S_OK : E_FAIL;
}

/*************************************************************************
 * SHGetFolderPathW			[SHFOLDER.@]
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
