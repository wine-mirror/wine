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
#include "crtdll.h"

#include "shlobj.h"
#include "shell32_main.h"
#include "windef.h"
#include "options.h"
#include "wine/undocshell.h"
#include "shlwapi.h"

DEFAULT_DEBUG_CHANNEL(shell)

/*
	Combining and Constructing paths
*/

/*************************************************************************
 * PathAppendA		[SHLWAPI.@]
 * 
 * NOTES
 *  concat path lpszPath2 onto lpszPath1
 *
 * FIXME
 *  the resulting path is also canonicalized
 */
LPSTR WINAPI PathAppendA(
	LPSTR lpszPath1,
	LPCSTR lpszPath2) 
{
	TRACE("%s %s\n",lpszPath1, lpszPath2);
	while (lpszPath2[0]=='\\') lpszPath2++;
	return PathCombineA(lpszPath1,lpszPath1,lpszPath2);
}

/*************************************************************************
 * PathAppendW		[SHLWAPI.@]
 */
LPSTR WINAPI PathAppendW(
	LPWSTR lpszPath1,
	LPCWSTR lpszPath2) 
{
	FIXME("%s %s\n",debugstr_w(lpszPath1), debugstr_w(lpszPath2));
	return NULL;
}

/*************************************************************************
 * PathAppendAW		[SHELL32.36]
 */
LPVOID WINAPI PathAppendAW(
	LPVOID lpszPath1,
	LPCVOID lpszPath2)
{
	if (VERSION_OsIsUnicode())
	  return PathAppendW(lpszPath1, lpszPath2);
	return PathAppendA(lpszPath1, lpszPath2);
}

/*************************************************************************
 * PathCombineA		[SHLWAPI.@]
 * 
 * NOTES
 *  if lpszFile='.' skip it
 *  szDest can be equal to lpszFile. Thats why we use sTemp
 *
 * FIXME
 *  the resulting path is also canonicalized
 */
LPSTR WINAPI PathCombineA(
	LPSTR szDest,
	LPCSTR lpszDir,
	LPCSTR lpszFile) 
{
	char sTemp[MAX_PATH];
	TRACE("%p %p->%s %p->%s\n",szDest, lpszDir, lpszDir, lpszFile, lpszFile);
	
	
	if (!lpszFile || !lpszFile[0] || (lpszFile[0]=='.' && !lpszFile[1]) ) 
	{
	  strcpy(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathGetDriveNumberA(lpszFile) != -1)
	{
	  strcpy(szDest,lpszFile);
	}
	else if (lpszFile[0] == '\\' )
	{
	  strcpy(sTemp,lpszDir);
	  PathStripToRootA(sTemp);
	  strcat(sTemp,lpszFile);
	  strcpy(szDest,sTemp);
	}
	else
	{
	  strcpy(sTemp,lpszDir);
	  PathAddBackslashA(sTemp);
	  strcat(sTemp,lpszFile);
	  strcpy(szDest,sTemp);
	}
	return szDest;
}

/*************************************************************************
 * PathCombineW		 [SHLWAPI.@]
 */
LPWSTR WINAPI PathCombineW(
	LPWSTR szDest,
	LPCWSTR lpszDir,
	LPCWSTR lpszFile) 
{
	WCHAR sTemp[MAX_PATH];
	TRACE("%p %p->%s %p->%s\n",szDest, lpszDir, debugstr_w(lpszDir),
			 lpszFile, debugstr_w(lpszFile));
	
	
	if (!lpszFile || !lpszFile[0] || (lpszFile[0]==(WCHAR)'.' && !lpszFile[1]) ) 
	{
	  CRTDLL_wcscpy(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathGetDriveNumberW(lpszFile) != -1)
	{
	  CRTDLL_wcscpy(szDest,lpszFile);
	}
	else if (lpszFile[0] == (WCHAR)'\\' )
	{
	  CRTDLL_wcscpy(sTemp,lpszDir);
	  PathStripToRootW(sTemp);
	  CRTDLL_wcscat(sTemp,lpszFile);
	  CRTDLL_wcscpy(szDest,sTemp);
	}
	else
	{
	  CRTDLL_wcscpy(sTemp,lpszDir);
	  PathAddBackslashW(sTemp);
	  CRTDLL_wcscat(sTemp,lpszFile);
	  CRTDLL_wcscpy(szDest,sTemp);
	}
	return szDest;
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
 * PathAddBackslashA	[SHLWAPI.@]
 *
 * NOTES
 *     append \ if there is none
 */
LPSTR WINAPI PathAddBackslashA(LPSTR lpszPath)
{
	int len;
	TRACE("%p->%s\n",lpszPath,lpszPath);

	len = strlen(lpszPath);
	if (len && lpszPath[len-1]!='\\') 
	{
	  lpszPath[len]  = '\\';
	  lpszPath[len+1]= 0x00;
	  return lpszPath+len+1;
	}
	return lpszPath+len;
}

/*************************************************************************
 * PathAddBackslashW	[SHLWAPI.@]
 */
LPWSTR WINAPI PathAddBackslashW(LPWSTR lpszPath)
{
	int len;
	TRACE("%p->%s\n",lpszPath,debugstr_w(lpszPath));

	len = CRTDLL_wcslen(lpszPath);
	if (len && lpszPath[len-1]!=(WCHAR)'\\') 
	{
	  lpszPath[len]  = (WCHAR)'\\';
	  lpszPath[len+1]= 0x00;
	  return lpszPath+len+1;
	}
	return lpszPath+len;
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
 * PathBuildRootA		[SHLWAPI.@]
 */
LPSTR WINAPI PathBuildRootA(LPSTR lpszPath, int drive) 
{
	TRACE("%p %i\n",lpszPath, drive);

	strcpy(lpszPath,"A:\\");
	lpszPath[0]+=drive;
	return lpszPath;
}

/*************************************************************************
 * PathBuildRootW		[SHLWAPI.@]
 */
LPWSTR WINAPI PathBuildRootW(LPWSTR lpszPath, int drive) 
{
	TRACE("%p %i\n",debugstr_w(lpszPath), drive);

	lstrcpyAtoW(lpszPath,"A:\\");
	lpszPath[0]+=drive;
	return lpszPath;
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
 * PathFindFileNameA	[SHLWAPI.@]
 */
LPSTR WINAPI PathFindFileNameA(LPCSTR lpszPath)
{
	LPCSTR aslash;
	aslash = lpszPath;

	TRACE("%s\n",aslash);
	while (lpszPath[0]) 
	{
	  if (((lpszPath[0]=='\\') || (lpszPath[0]==':')) && lpszPath[1] && lpszPath[1]!='\\')
	      aslash = lpszPath+1;
	  lpszPath++;
	}
	return (LPSTR)aslash;

}

/*************************************************************************
 * PathFindFileNameW	[SHLWAPI.@]
 */
LPWSTR WINAPI PathFindFileNameW(LPCWSTR lpszPath)
{
	LPCWSTR wslash;
	wslash = lpszPath;

	TRACE("%s\n",debugstr_w(wslash));
	while (lpszPath[0]) 
	{
	  if (((lpszPath[0]=='\\') || (lpszPath[0]==':')) && lpszPath[1] && lpszPath[1]!='\\')
	    wslash = lpszPath+1;
	  lpszPath++;
	}
	return (LPWSTR)wslash;	
}

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
 * PathFindExtensionA	[SHLWAPI.@]
 *
 * NOTES
 *     returns pointer to last . in last lpszPath component or at \0.
 */

LPSTR WINAPI PathFindExtensionA(LPCSTR lpszPath) 
{
	LPCSTR   lastpoint = NULL;

	TRACE("%p %s\n",lpszPath,lpszPath);

	while (*lpszPath) 
	{
	  if (*lpszPath=='\\'||*lpszPath==' ')
	    lastpoint=NULL;
	  if (*lpszPath=='.')
	    lastpoint=lpszPath;
	  lpszPath++;
	}
	return (LPSTR)(lastpoint?lastpoint:lpszPath);
}

/*************************************************************************
 * PathFindExtensionW	[SHLWAPI.@]
 */
LPWSTR WINAPI PathFindExtensionW(LPCWSTR lpszPath) 
{
	LPCWSTR   lastpoint = NULL;

	TRACE("(%p %s)\n",lpszPath,debugstr_w(lpszPath));

	while (*lpszPath)
	{
	  if (*lpszPath==(WCHAR)'\\'||*lpszPath==(WCHAR)' ')
	    lastpoint=NULL;
	  if (*lpszPath==(WCHAR)'.')
	    lastpoint=lpszPath;
	  lpszPath++;
	}
	return (LPWSTR)(lastpoint?lastpoint:lpszPath);
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
 * PathGetExtensionA		[internal]
 *
 * NOTES
 *  exported by ordinal
 *  return value points to the first char after the dot
 */
LPSTR WINAPI PathGetExtensionA(LPCSTR lpszPath)
{
	TRACE("(%s)\n",lpszPath);

	lpszPath = PathFindExtensionA(lpszPath);
	return (LPSTR)(*lpszPath?(lpszPath+1):lpszPath);
}

/*************************************************************************
 * PathGetExtensionW		[internal]
 */
LPWSTR WINAPI PathGetExtensionW(LPCWSTR lpszPath)
{
	TRACE("(%s)\n",debugstr_w(lpszPath));

	lpszPath = PathFindExtensionW(lpszPath);
	return (LPWSTR)(*lpszPath?(lpszPath+1):lpszPath);
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
 * PathGetArgsA		[SHLWAPI.@]
 *
 * NOTES
 *     look for next arg in string. handle "quoted" strings
 *     returns pointer to argument *AFTER* the space. Or to the \0.
 *
 * FIXME
 *     quoting by '\'
 */
LPSTR WINAPI PathGetArgsA(LPCSTR lpszPath) 
{
	BOOL	qflag = FALSE;

	TRACE("%s\n",lpszPath);

	while (*lpszPath) 
	{
	  if ((*lpszPath==' ') && !qflag)
	    return (LPSTR)lpszPath+1;
	  if (*lpszPath=='"')
	    qflag=!qflag;
	  lpszPath++;
	}
	return (LPSTR)lpszPath;

}

/*************************************************************************
 * PathGetArgsW		[SHLWAPI.@]
 */
LPWSTR WINAPI PathGetArgsW(LPCWSTR lpszPath) 
{
	BOOL	qflag = FALSE;

	TRACE("%s\n",debugstr_w(lpszPath));

	while (*lpszPath) 
	{
	  if ((*lpszPath==' ') && !qflag)
	    return (LPWSTR)lpszPath+1;
	  if (*lpszPath=='"')
	    qflag=!qflag;
	  lpszPath++;
	}
	return (LPWSTR)lpszPath;
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
 * PathGetDriveNumberA	[SHLWAPI.@]
 */
int WINAPI PathGetDriveNumberA(LPCSTR lpszPath)
{
	int chr = tolower(lpszPath[0]);
	
	TRACE ("%s\n",debugstr_a(lpszPath));

	if (!lpszPath || lpszPath[1]!=':' || chr < 'a' || chr > 'z') return -1;
	return tolower(lpszPath[0]) - 'a' ;
}

/*************************************************************************
 * PathGetDriveNumberW	[SHLWAPI.@]
 */
int WINAPI PathGetDriveNumberW(LPCWSTR lpszPath)
{
	int chr = CRTDLL_towlower(lpszPath[0]);
	
	TRACE ("%s\n",debugstr_w(lpszPath));

	if (!lpszPath || lpszPath[1]!=':' || chr < 'a' || chr > 'z') return -1;
	return tolower(lpszPath[0]) - 'a' ;
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
 * PathRemoveFileSpecA	[SHLWAPI.@]
 * 
 * NOTES
 *     truncates passed argument to a valid path
 *     returns if the string was modified or not.
 *     "\foo\xx\foo"-> "\foo\xx"
 *     "\" -> "\"
 *     "a:\foo"	-> "a:\"
 */
BOOL WINAPI PathRemoveFileSpecA(LPSTR lpszPath)
{
	LPSTR cutplace;

	TRACE("%s\n",lpszPath);

	if (!lpszPath[0]) return 0;

	cutplace = PathFindFileNameA(lpszPath);
	if (cutplace)
	{
	  *cutplace='\0';
	  if (PathIsRootA(lpszPath))
	  {
	    PathAddBackslashA(lpszPath);
	  }
	  else
	  {
	    PathRemoveBackslashA(lpszPath);
	  }
	  return TRUE;
	}
	return FALSE;
}

/*************************************************************************
 * PathRemoveFileSpecW	[SHLWAPI.@]
 */
BOOL WINAPI PathRemoveFileSpecW(LPWSTR lpszPath)
{
	LPWSTR cutplace;

	TRACE("%s\n",debugstr_w(lpszPath));

	if (!lpszPath[0]) return 0;
	cutplace = PathFindFileNameW(lpszPath);
	if (cutplace)
	{
	  *cutplace='\0';
	  if (PathIsRootW(lpszPath))
	  {
	    PathAddBackslashW(lpszPath);
	  }
	  else
	  {
	    PathRemoveBackslashW(lpszPath);
	  }
	  return TRUE;
	}
	return FALSE;
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
 * PathStripPathA	[SHELLWAPI.@]
 * 
 * NOTES
 *  removes the path from the beginning of a filename
 */
void WINAPI PathStripPathA(LPSTR lpszPath)
{
	LPSTR lpszFileName = PathFindFileNameA(lpszPath);

	TRACE("%s\n", lpszPath);

	if(lpszFileName)
	  RtlMoveMemory(lpszPath, lpszFileName, strlen(lpszFileName)); 
}

/*************************************************************************
 * PathStripPathW	[SHELLWAPI.@]
 */
void WINAPI PathStripPathW(LPWSTR lpszPath)
{
	LPWSTR lpszFileName = PathFindFileNameW(lpszPath);

	TRACE("%s\n", debugstr_w(lpszPath));
	if(lpszFileName)
	  RtlMoveMemory(lpszPath, lpszFileName, lstrlenW(lpszFileName)*sizeof(WCHAR)); 
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
 * PathStripToRootA	[SHLWAPI.@]
 */
BOOL WINAPI PathStripToRootA(LPSTR lpszPath)
{
	TRACE("%s\n", lpszPath);

	/* X:\ */
	if (lpszPath[1]==':' && lpszPath[2]=='\\')
	{
	  lpszPath[3]='\0';
	  return TRUE;
	}

	/* "\" */
	if (lpszPath[0]=='\\')
	{
	  lpszPath[1]='\0';
	  return TRUE;
	}

	/* UNC "\\<computer>\<share>" */
	if (lpszPath[0]=='\\' && lpszPath[1]=='\\')		
	{
	  int foundbackslash = 0;
	  lpszPath += 2;
	  while (*lpszPath)
	  {
	    if (*lpszPath=='\\') foundbackslash++;
	    if (foundbackslash==2)
	    {
	      *lpszPath = '\0';
	      return TRUE;
	    }
	    lpszPath++;
	  }
	}

	return FALSE;
}

/*************************************************************************
 * PathStripToRootW	[SHLWAPI.@]
 */
BOOL WINAPI PathStripToRootW(LPWSTR lpszPath)
{
	TRACE("%s\n", debugstr_w(lpszPath));

	/* X:\ */
	if (lpszPath[1]==':' && lpszPath[2]=='\\')
	{
	  lpszPath[3]='\0';
	  return TRUE;
	}

	/* "\" */
	if (lpszPath[0]=='\\')
	{
	  lpszPath[1]='\0';
	  return TRUE;
	}

	/* UNC "\\<computer>\<share>" */
	if (lpszPath[0]=='\\' && lpszPath[1]=='\\')		
	{
	  int foundbackslash = 0;
	  lpszPath += 2;
	  while (*lpszPath)
	  {
	    if (*lpszPath=='\\') foundbackslash++;
	    if (foundbackslash==2)
	    {
	      *lpszPath = '\0';
	      return TRUE;
	    }
	    lpszPath++;
	  }
	}

	return FALSE;
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
 * PathRemoveArgsA	[SHLWAPI.@]
 */
void WINAPI PathRemoveArgsA(LPSTR lpszPath)
{
	LPSTR lpszArgs = PathGetArgsA(lpszPath);

	TRACE("%s\n", lpszPath);

	if (lpszArgs) *(--lpszArgs)='\0';
}

/*************************************************************************
 * PathRemoveArgsW	[SHLWAPI.@]
 */
void WINAPI PathRemoveArgsW(LPWSTR lpszPath)
{
	LPWSTR lpszArgs = PathGetArgsW(lpszPath);

	TRACE("%s\n", debugstr_w(lpszPath));

	if (lpszArgs) *(--lpszArgs)='\0';
}

/*************************************************************************
 * PathRemoveArgsAW	[SHELL32.251]
 */
void WINAPI PathRemoveArgsAW(LPVOID lpszPath) 
{
	if (VERSION_OsIsUnicode())
	  return PathRemoveArgsW(lpszPath);
	return PathRemoveArgsA(lpszPath);
}

/*************************************************************************
 * PathRemoveExtensionA		[SHLWAPI.@]
 */
void WINAPI PathRemoveExtensionA(LPSTR lpszPath)
{
	LPSTR lpszExtension = PathFindExtensionA(lpszPath);

	TRACE("%s\n", lpszPath);

	if (lpszExtension) *lpszExtension='\0';
}

/*************************************************************************
 * PathRemoveExtensionW		[SHLWAPI.@]
 */
void WINAPI PathRemoveExtensionW(LPWSTR lpszPath)
{
	LPWSTR lpszExtension = PathFindExtensionW(lpszPath);

	TRACE("%s\n", debugstr_w(lpszPath));

	if (lpszExtension) *lpszExtension='\0';
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

/*************************************************************************
 * PathRemoveBackslashA	[SHLWAPI.@]
 *
 * If the path ends in a backslash it is replaced by a NULL
 * and the address of the NULL is returned
 * Otherwise 
 * the address of the last character is returned.
 *
 */
LPSTR WINAPI PathRemoveBackslashA( LPSTR lpszPath )
{
	LPSTR p = lpszPath;
	
	while (*lpszPath) p = lpszPath++;
	if ( *p == (CHAR)'\\') *p = (CHAR)'\0';
	return p;
}

/*************************************************************************
 * PathRemoveBackslashW	[SHLWAPI.@]
 */
LPWSTR WINAPI PathRemoveBackslashW( LPWSTR lpszPath )
{
	LPWSTR p = lpszPath;
	
	while (*lpszPath) p = lpszPath++;
	if ( *p == (WCHAR)'\\') *p = (WCHAR)'\0';
	return p;
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
 * PathRemoveBlanksA [SHLWAPI.@]
 * 
 * NOTES
 *     remove spaces from beginning and end of passed string
 */
LPSTR WINAPI PathRemoveBlanksA(LPSTR str)
{
	LPSTR x = str;

	TRACE("%s\n",str);

	while (*x==' ') x++;
	if (x!=str)
	  strcpy(str,x);
	if (!*str)
	  return str;
	x=str+strlen(str)-1;
	while (*x==' ')
	  x--;
	if (*x==' ')
	  *x='\0';
	return x;
}

/*************************************************************************
 * PathRemoveBlanksW [SHLWAPI.@]
 */
LPWSTR WINAPI PathRemoveBlanksW(LPWSTR str)
{
	LPWSTR x = str;

	TRACE("%s\n",debugstr_w(str));

	while (*x==' ') x++;
	if (x!=str)
	  CRTDLL_wcscpy(str,x);
	if (!*str)
	  return str;
	x=str+CRTDLL_wcslen(str)-1;
	while (*x==' ')
	  x--;
	if (*x==' ')
	  *x='\0';
	return x;
}

/*************************************************************************
 * PathRemoveBlanksAW [SHELL32.33]
 */
LPVOID WINAPI PathRemoveBlanksAW(LPVOID str)
{
	if(VERSION_OsIsUnicode())
	  return PathRemoveBlanksW(str);
	return PathRemoveBlanksA(str);
}

/*************************************************************************
 * PathQuoteSpacesA [SHLWAPI.@]
 * 
 * NOTES
 */
LPSTR WINAPI PathQuoteSpacesA(LPCSTR lpszPath)
{
	FIXME("%s\n",lpszPath);
	return 0;
}

/*************************************************************************
 * PathQuoteSpacesW [SHLWAPI.@]
 */
LPWSTR WINAPI PathQuoteSpacesW(LPCWSTR lpszPath)
{
	FIXME("%s\n",debugstr_w(lpszPath));
	return 0;	
}

/*************************************************************************
 * PathQuoteSpacesAW [SHELL32.55]
 */
LPVOID WINAPI PathQuoteSpacesAW (LPCVOID lpszPath)
{
	if(VERSION_OsIsUnicode())
	  return PathQuoteSpacesW(lpszPath);
	return PathQuoteSpacesA(lpszPath);
}

/*************************************************************************
 * PathUnquoteSpacesA [SHLWAPI.@]
 * 
 * NOTES
 *     unquote string (remove ")
 */
VOID WINAPI PathUnquoteSpacesA(LPSTR str) 
{
	DWORD len = lstrlenA(str);

	TRACE("%s\n",str);

	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	lstrcpyA(str,str+1);
	return;
}

/*************************************************************************
 * PathUnquoteSpacesW [SHLWAPI.@]
 */
VOID WINAPI PathUnquoteSpacesW(LPWSTR str) 
{
	DWORD len = CRTDLL_wcslen(str);

	TRACE("%s\n",debugstr_w(str));

	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	CRTDLL_wcscpy(str,str+1);
	return;
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
 * PathParseIconLocationA	[SHLWAPI.@]
 */
int WINAPI PathParseIconLocationA(LPSTR lpszPath)
{
	LPSTR lpstrComma = strchr(lpszPath, ',');
	
	FIXME("%s stub\n", debugstr_a(lpszPath));

	if (lpstrComma && lpstrComma[1])
	{
	  lpstrComma[0]='\0';
/*	  return atoi(&lpstrComma[1]);	FIXME */
	}
	return 0;
}

/*************************************************************************
 * PathParseIconLocationW	[SHLWAPI.@]
 */
int WINAPI PathParseIconLocationW(LPWSTR lpszPath)
{
	LPWSTR lpstrComma = CRTDLL_wcschr(lpszPath, ',');
	
	FIXME("%s stub\n", debugstr_w(lpszPath));

	if (lpstrComma && lpstrComma[1])
	{
	  lpstrComma[0]='\0';
/*	  return _wtoi(&lpstrComma[1]);	FIXME */
	}
	return 0;
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
	Path Testing
*/
/*************************************************************************
 * PathIsUNCA		[SHLWAPI.@]
 * 
 * NOTES
 *     PathIsUNC(char*path);
 */
BOOL WINAPI PathIsUNCA(LPCSTR lpszPath) 
{
	TRACE("%s\n",lpszPath);

	return (lpszPath && (lpszPath[0]=='\\') && (lpszPath[1]=='\\'));
}

/*************************************************************************
 * PathIsUNCW		[SHLWAPI.@]
 */
BOOL WINAPI PathIsUNCW(LPCWSTR lpszPath) 
{
	TRACE("%s\n",debugstr_w(lpszPath));

	return (lpszPath && (lpszPath[0]=='\\') && (lpszPath[1]=='\\'));
}

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
 *  PathIsRelativeA	[SHLWAPI.@]
 */
BOOL WINAPI PathIsRelativeA (LPCSTR lpszPath)
{
	TRACE("lpszPath=%s\n",lpszPath);

	return (lpszPath && (lpszPath[0]!='\\' && lpszPath[1]!=':'));
}

/*************************************************************************
 *  PathIsRelativeW	[SHLWAPI.@]
 */
BOOL WINAPI PathIsRelativeW (LPCWSTR lpszPath)
{
	TRACE("lpszPath=%s\n",debugstr_w(lpszPath));

	return (lpszPath && (lpszPath[0]!='\\' && lpszPath[1]!=':'));
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
 * PathIsRootA		[SHLWAPI.@]
 *
 * notes
 *  TRUE if the path points to a root directory
 */
BOOL WINAPI PathIsRootA(LPCSTR lpszPath)
{
	TRACE("%s\n",lpszPath);

	/* X:\ */
	if (lpszPath[1]==':' && lpszPath[2]=='\\' && lpszPath[3]=='\0')
	  return TRUE;

	/* "\" */
	if (lpszPath[0]=='\\' && lpszPath[1]=='\0')
	  return TRUE;

	/* UNC "\\<computer>\<share>" */
	if (lpszPath[0]=='\\' && lpszPath[1]=='\\')		
	{
	  int foundbackslash = 0;
	  lpszPath += 2;
	  while (*lpszPath)
	  {
	    if (*(lpszPath++)=='\\') foundbackslash++;
	  }
	  if (foundbackslash==1)
	    return TRUE;
	}
	return FALSE;
}

/*************************************************************************
 * PathIsRootW		[SHLWAPI.@]
 */
BOOL WINAPI PathIsRootW(LPCWSTR lpszPath) 
{
	TRACE("%s\n",debugstr_w(lpszPath));

	/* X:\ */
	if (lpszPath[1]==':' && lpszPath[2]=='\\' && lpszPath[3]=='\0')
	  return TRUE;

	/* "\" */
	if (lpszPath[0]=='\\' && lpszPath[1]=='\0')
	  return TRUE;

	/* UNC "\\<computer>\<share>" */
	if (lpszPath[0]=='\\' && lpszPath[1]=='\\')		
	{
	  int foundbackslash = 0;
	  lpszPath += 2;
	  while (*lpszPath)
	  {
	    if (*(lpszPath++)=='\\') foundbackslash++;
	  }
	  if (foundbackslash==1)
	    return TRUE;
	}
	return FALSE;

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
 *  PathIsExeA		[internal]
 */
BOOL WINAPI PathIsExeA (LPCSTR lpszPath)
{
	LPCSTR lpszExtension = PathGetExtensionA(lpszPath);
	int i = 0;
	static char * lpszExtensions[6] = {"exe", "com", "pid", "cmd", "bat", NULL };
	
	TRACE("path=%s\n",lpszPath);

	for(i=0; lpszExtensions[i]; i++)
	  if (!strcasecmp(lpszExtension,lpszExtensions[i])) return TRUE;
	  
	return FALSE;
}

/*************************************************************************
 *  PathIsExeW		[internal]
 */
BOOL WINAPI PathIsExeW (LPCWSTR lpszPath)
{
	LPCWSTR lpszExtension = PathGetExtensionW(lpszPath);
	int i = 0;
	static WCHAR lpszExtensions[6][4] =
	  {{'e','x','e','\0'}, {'c','o','m','\0'}, {'p','i','d','\0'},
	   {'c','m','d','\0'}, {'b','a','t','\0'}, {'\0'} };
	
	TRACE("path=%s\n",debugstr_w(lpszPath));

	for(i=0; lpszExtensions[i]; i++)
	  if (!CRTDLL__wcsicmp(lpszExtension,lpszExtensions[i])) return TRUE;
	  
	return FALSE;
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
 * PathIsDirectoryA	[SHLWAPI.@]
 */
BOOL WINAPI PathIsDirectoryA(LPCSTR lpszPath)
{
	HANDLE hFile;
	WIN32_FIND_DATAA stffile;
	
	TRACE("%s\n", debugstr_a(lpszPath));

	hFile = FindFirstFileA(lpszPath, &stffile);

	if ( hFile != INVALID_HANDLE_VALUE )
	{
	  FindClose (hFile);
          return (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	}

	return FALSE;	
}

/*************************************************************************
 * PathIsDirectoryW	[SHLWAPI.@]
 */
BOOL WINAPI PathIsDirectoryW(LPCWSTR lpszPath)
{
	HANDLE hFile;
	WIN32_FIND_DATAW stffile;
	
	TRACE("%s\n", debugstr_w(lpszPath));

	hFile = FindFirstFileW(lpszPath, &stffile);

	if ( hFile != INVALID_HANDLE_VALUE )
	{
	  FindClose (hFile);
          return (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	}

	return FALSE;	
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
 * PathFileExistsA	[SHLWAPI.@]
 * 
 * NOTES
 *     file_exists(char *fn);
 */
BOOL WINAPI PathFileExistsA(LPCSTR lpszPath) 
{
	TRACE("%s\n",lpszPath);
	return  (GetFileAttributesA(lpszPath)!=-1);
}

/*************************************************************************
 * PathFileExistsW	[SHLWAPI.@]
 */
BOOL WINAPI PathFileExistsW(LPCWSTR lpszPath) 
{
	TRACE("%s\n",debugstr_w(lpszPath));
	return  (GetFileAttributesW(lpszPath)!=-1);
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
 * PathMatchSingleMaskA	[internal]
 * 
 * NOTES
 *     internal (used by PathMatchSpec)
 */
static BOOL PathMatchSingleMaskA(LPCSTR name, LPCSTR mask)
{
	while (*name && *mask && *mask!=';') 
	{
	  if (*mask=='*') 
	  {
	    do 
	    {
	      if (PathMatchSingleMaskA(name,mask+1)) return 1;  /* try substrings */
	    } while (*name++);
	    return 0;
	  }
	  if (toupper(*mask)!=toupper(*name) && *mask!='?') return 0;
	  name++;
	  mask++;
	}
	if (!*name) 
	{
	  while (*mask=='*') mask++;
	  if (!*mask || *mask==';') return 1;
	}
	return 0;
}

/*************************************************************************
 * PathMatchSingleMaskW	[internal]
 */
static BOOL PathMatchSingleMaskW(LPCWSTR name, LPCWSTR mask)
{
	while (*name && *mask && *mask!=';') 
	{
	  if (*mask=='*') 
	  {
	    do 
	    {
	      if (PathMatchSingleMaskW(name,mask+1)) return 1;  /* try substrings */
	    } while (*name++);
	    return 0;
	  }
	  if (CRTDLL_towupper(*mask)!=CRTDLL_towupper(*name) && *mask!='?') return 0;
	  name++;
	  mask++;
	}
	if (!*name) 
	{
	  while (*mask=='*') mask++;
	  if (!*mask || *mask==';') return 1;
	}
	return 0;
}
/*************************************************************************
 * PathMatchSpecA	[SHLWAPI.@]
 * 
 * NOTES
 *     used from COMDLG32
 */
BOOL WINAPI PathMatchSpecA(LPCSTR name, LPCSTR mask) 
{
	TRACE("%s %s\n",name,mask);

	if (!lstrcmpA( mask, "*.*" )) return 1;   /* we don't require a period */

	while (*mask) 
	{
	  if (PathMatchSingleMaskA(name,mask)) return 1;    /* helper function */
	  while (*mask && *mask!=';') mask++;
	  if (*mask==';') 
	  {
	    mask++;
	    while (*mask==' ') mask++;      /*  masks may be separated by "; " */
	  }
	}
	return 0;
}

/*************************************************************************
 * PathMatchSpecW	[SHLWAPI.@]
 */
BOOL WINAPI PathMatchSpecW(LPCWSTR name, LPCWSTR mask) 
{
	WCHAR stemp[4];
	TRACE("%s %s\n",debugstr_w(name),debugstr_w(mask));

	lstrcpyAtoW(stemp,"*.*");	
	if (!lstrcmpW( mask, stemp )) return 1;   /* we don't require a period */

	while (*mask) 
	{
	  if (PathMatchSingleMaskW(name,mask)) return 1;    /* helper function */
	  while (*mask && *mask!=';') mask++;
	  if (*mask==';') 
	  {
	    mask++;
	    while (*mask==' ') mask++;       /* masks may be separated by "; " */
	  }
	}
	return 0;
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
 * PathIsSameRootA	[SHLWAPI.@]
 *
 * FIXME
 *  what to do with "\path" ??
 */
BOOL WINAPI PathIsSameRootA(LPCSTR lpszPath1, LPCSTR lpszPath2)
{
	TRACE("%s %s\n", lpszPath1, lpszPath2);
	
	if (PathIsRelativeA(lpszPath1) || PathIsRelativeA(lpszPath2)) return FALSE;

	/* usual path */
	if ( toupper(lpszPath1[0])==toupper(lpszPath2[0]) &&
	     lpszPath1[1]==':' && lpszPath2[1]==':' &&
	     lpszPath1[2]=='\\' && lpszPath2[2]=='\\')
	  return TRUE;

	/* UNC */
	if (lpszPath1[0]=='\\' && lpszPath2[0]=='\\' &&
	    lpszPath1[1]=='\\' && lpszPath2[1]=='\\')
	{
	  int pos=2, bsfound=0;
	  while (lpszPath1[pos] && lpszPath2[pos] &&
	        (lpszPath1[pos] == lpszPath2[pos]))
	  {
	    if (lpszPath1[pos]=='\\') bsfound++;
	    if (bsfound == 2) return TRUE;
	    pos++;
	  }
	  return (lpszPath1[pos] == lpszPath2[pos]);
	}
	return FALSE;
}

/*************************************************************************
 * PathIsSameRootW	[SHLWAPI.@]
 */
BOOL WINAPI PathIsSameRootW(LPCWSTR lpszPath1, LPCWSTR lpszPath2)
{
	TRACE("%s %s\n", debugstr_w(lpszPath1), debugstr_w(lpszPath2));
	
	if (PathIsRelativeW(lpszPath1) || PathIsRelativeW(lpszPath2)) return FALSE;

	/* usual path */
	if ( CRTDLL_towupper(lpszPath1[0])==CRTDLL_towupper(lpszPath2[0]) &&
	     lpszPath1[1]==':' && lpszPath2[1]==':' &&
	     lpszPath1[2]=='\\' && lpszPath2[2]=='\\')
	  return TRUE;

	/* UNC */
	if (lpszPath1[0]=='\\' && lpszPath2[0]=='\\' &&
	    lpszPath1[1]=='\\' && lpszPath2[1]=='\\')
	{
	  int pos=2, bsfound=0;
	  while (lpszPath1[pos] && lpszPath2[pos] &&
	        (lpszPath1[pos] == lpszPath2[pos]))
	  {
	    if (lpszPath1[pos]=='\\') bsfound++;
	    if (bsfound == 2) return TRUE;
	    pos++;
	  }
	  return (lpszPath1[pos] == lpszPath2[pos]);
	}
	return FALSE;
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
 * PathIsURLA
 */
BOOL WINAPI PathIsURLA(LPCSTR lpstrPath)
{
	LPSTR lpstrRes;
	int iSize, i=0;
	static LPSTR SupportedProtocol[] = 
	  {"http","https","ftp","gopher","file","mailto",NULL};

	if(!lpstrPath) return FALSE;

	/* get protocol        */
	lpstrRes = strchr(lpstrPath,':');
	if(!lpstrRes) return FALSE;
	iSize = lpstrRes - lpstrPath;

	while(SupportedProtocol[i])
	{
	  if (iSize == strlen(SupportedProtocol[i]))
	    if(!strncasecmp(lpstrPath, SupportedProtocol[i], iSize));
	      return TRUE;
	  i++;
	}

	return FALSE;
}  

/*************************************************************************
 * PathIsURLW
 */
BOOL WINAPI PathIsURLW(LPCWSTR lpstrPath)
{
	LPWSTR lpstrRes;
	int iSize, i=0;
	static WCHAR SupportedProtocol[7][7] = 
	  {{'h','t','t','p','\0'},{'h','t','t','p','s','\0'},{'f','t','p','\0'},
	  {'g','o','p','h','e','r','\0'},{'f','i','l','e','\0'},
	  {'m','a','i','l','t','o','\0'},{0}};

	if(!lpstrPath) return FALSE;

	/* get protocol        */
	lpstrRes = CRTDLL_wcschr(lpstrPath,':');
	if(!lpstrRes) return FALSE;
	iSize = lpstrRes - lpstrPath;

	while(SupportedProtocol[i])
	{
	  if (iSize == CRTDLL_wcslen(SupportedProtocol[i]))
	    if(!CRTDLL__wcsnicmp(lpstrPath, SupportedProtocol[i], iSize));
	      return TRUE;
	  i++;
	}

	return FALSE;
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
	Creating Something Unique
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
	cleaning and resolving paths 
 */

/*************************************************************************
 * PathFindOnPathA	[SHELL32.145]
 */
BOOL WINAPI PathFindOnPathA(LPSTR sFile, LPCSTR sOtherDirs)
{
	FIXME("%s %s\n",sFile, sOtherDirs);
	return FALSE;
}

/*************************************************************************
 * PathFindOnPathW	[SHELL32]
 */
BOOL WINAPI PathFindOnPathW(LPWSTR sFile, LPCWSTR sOtherDirs)
{
	FIXME("%s %s\n",debugstr_w(sFile), debugstr_w(sOtherDirs));
	return FALSE;
}

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
 * PathCleanupSpecA	[SHELL32.171]
 */
DWORD WINAPI PathCleanupSpecA(LPSTR x, LPSTR y)
{
	FIXME("(%p %s, %p %s) stub\n",x,debugstr_a(x),y,debugstr_a(y));
	return TRUE;
}

/*************************************************************************
 * PathCleanupSpecA	[SHELL32]
 */
DWORD WINAPI PathCleanupSpecW(LPWSTR x, LPWSTR y)
{
	FIXME("(%p %s, %p %s) stub\n",x,debugstr_w(x),y,debugstr_w(y));
	return TRUE;
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
	special
*/

/*************************************************************************
 * PathSetDlgItemPathA
 *
 * NOTES
 *  use PathCompactPath to make sure, the path fits into the control
 */
BOOL WINAPI PathSetDlgItemPathA(HWND hDlg, int id, LPCSTR pszPath) 
{	TRACE("%x %x %s\n",hDlg, id, pszPath);
	return SetDlgItemTextA(hDlg, id, pszPath);
}

/*************************************************************************
 * PathSetDlgItemPathW
 */
BOOL WINAPI PathSetDlgItemPathW(HWND hDlg, int id, LPCWSTR pszPath) 
{	TRACE("%x %x %s\n",hDlg, id, debugstr_w(pszPath));
	return SetDlgItemTextW(hDlg, id, pszPath);
}

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
 * 
 */
 
static char * szSHFolders = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";
static char * szSHUserFolders = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";

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

	if (bCreate && CreateDirectoryA(szPath,NULL))
	{
	    MESSAGE("Created not existing system directory '%s'\n", szPath);
	}

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
 * PathCanonicalizeA
 *
 *  FIXME
 *   returnvalue
 */
 
BOOL WINAPI PathCanonicalizeA(LPSTR pszBuf, LPCSTR pszPath)
{
	int OffsetMin = 0, OffsetSrc = 0, OffsetDst = 0, LenSrc = strlen(pszPath);
	BOOL bModifyed = FALSE;

	TRACE("%p %s\n", pszBuf, pszPath);
	
	pszBuf[OffsetDst]='\0';

	/* keep the root of the path */
	if( LenSrc && (pszPath[OffsetSrc]=='\\'))
	{
	  pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; OffsetMin++; LenSrc--;
	}
	else if ( (LenSrc >= 2) && (pszPath[OffsetSrc+1] == ':'))
	{
	  pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; OffsetMin++; LenSrc--;
	  pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; OffsetMin++; LenSrc--;
	  if (LenSrc && (pszPath[OffsetSrc] == '\\'))
	  {
	    pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; OffsetMin++; LenSrc--;
	    if (LenSrc == 1 && pszPath[OffsetSrc]=='.')
	    {
	      /* C:\. */
	      OffsetSrc++; LenSrc--; bModifyed = TRUE;
	    } 
	    else if (LenSrc == 2 && pszPath[OffsetSrc]=='.' && pszPath[OffsetSrc+1]=='.')
	    {
	      /* C:\.. */
	      OffsetSrc+=2; LenSrc-=2; bModifyed = TRUE;
	    } 
          }
	}
	
	/* ".\" at the beginning of the path */
	if (LenSrc >= 2 && pszPath[OffsetSrc]=='.' && pszPath[OffsetSrc+1]=='\\')
	{
	  OffsetSrc+=2; LenSrc-=2; bModifyed = TRUE;
	} 
	
	while ( LenSrc )
	{
	  if((LenSrc>=3) && (pszPath[OffsetSrc]=='\\') && (pszPath[OffsetSrc+1]=='.') && (pszPath[OffsetSrc+2]=='.'))
	  {
	    /* "\.." found, go one deeper */
	    while((OffsetDst > OffsetMin) && (pszBuf[OffsetDst]!='\\')) OffsetDst--;
	    OffsetSrc += 3; LenSrc -= 3; bModifyed = TRUE;
	    if(OffsetDst == OffsetMin && pszPath[OffsetSrc]=='\\') OffsetSrc++;
	    pszBuf[OffsetDst] = '\0';			/* important for \..\.. */
	  }
	  else if(LenSrc>=2 && pszPath[OffsetSrc]=='\\' && pszPath[OffsetSrc+1]=='.' )
	  {
	    /* "\." found, skip it */
	    OffsetSrc += 2; LenSrc-=2; bModifyed = TRUE;
	  }
	  else
	  {
	    pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; LenSrc--;
	  }
	}
	pszBuf[OffsetDst] = '\0';
	TRACE("-- %s %u\n", pszBuf, bModifyed);
	return bModifyed;
}


/*************************************************************************
 * PathCanonicalizeW
 *
 *  FIXME
 *   returnvalue
 */
BOOL WINAPI PathCanonicalizeW(LPWSTR pszBuf, LPCWSTR pszPath)
{
	int OffsetMin = 0, OffsetSrc = 0, OffsetDst = 0, LenSrc = lstrlenW(pszPath);
	BOOL bModifyed = FALSE;

	TRACE("%p %s\n", pszBuf, debugstr_w(pszPath));
	
	pszBuf[OffsetDst]='\0';

	/* keep the root of the path */
	if( LenSrc && (pszPath[OffsetSrc]=='\\'))
	{
	  pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; OffsetMin++; LenSrc--;
	}
	else if ( (LenSrc >= 2) && (pszPath[OffsetSrc+1] == ':'))
	{
	  pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; OffsetMin++; LenSrc--;
	  pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; OffsetMin++; LenSrc--;
	  if (LenSrc && (pszPath[OffsetSrc] == '\\'))
	  {
	    pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; OffsetMin++; LenSrc--;
	    if (LenSrc == 1 && pszPath[OffsetSrc]=='.')
	    {
	      /* C:\. */
	      OffsetSrc++; LenSrc--; bModifyed = TRUE;
	    } 
	    else if (LenSrc == 2 && pszPath[OffsetSrc]=='.' && pszPath[OffsetSrc+1]=='.')
	    {
	      /* C:\.. */
	      OffsetSrc+=2; LenSrc-=2; bModifyed = TRUE;
	    } 
          }
	}
	
	/* ".\" at the beginning of the path */
	if (LenSrc >= 2 && pszPath[OffsetSrc]=='.' && pszPath[OffsetSrc+1]=='\\')
	{
	  OffsetSrc+=2; LenSrc-=2; bModifyed = TRUE;
	} 
	
	while ( LenSrc )
	{
	  if((LenSrc>=3) && (pszPath[OffsetSrc]=='\\') && (pszPath[OffsetSrc+1]=='.') && (pszPath[OffsetSrc+2]=='.'))
	  {
	    /* "\.." found, go one deeper */
	    while((OffsetDst > OffsetMin) && (pszBuf[OffsetDst]!='\\')) OffsetDst--;
	    OffsetSrc += 3; LenSrc -= 3; bModifyed = TRUE;
	    if(OffsetDst == OffsetMin && pszPath[OffsetSrc]=='\\') OffsetSrc++;
	    pszBuf[OffsetDst] = '\0';			/* important for \..\.. */
	  }
	  else if(LenSrc>=2 && pszPath[OffsetSrc]=='\\' && pszPath[OffsetSrc+1]=='.' )
	  {
	    /* "\." found, skip it */
	    OffsetSrc += 2; LenSrc-=2; bModifyed = TRUE;
	  }
	  else
	  {
	    pszBuf[OffsetDst++] = pszPath[OffsetSrc++]; LenSrc--;
	  }
	}
	pszBuf[OffsetDst] = '\0';
	TRACE("-- %s %u\n", debugstr_w(pszBuf), bModifyed);
	return bModifyed;
}

/*************************************************************************
 * PathFindNextComponentA
 *
 * Windows returns a pointer NULL (BO 000605)
 */
LPSTR WINAPI PathFindNextComponentA(LPCSTR pszPath)
{
	while( *pszPath )
	{
	  if(*pszPath++=='\\')
	    return pszPath;
	}
	return pszPath;
}

/*************************************************************************
 * PathFindNextComponentW
 */
LPWSTR WINAPI PathFindNextComponentW(LPCWSTR pszPath)
{
	while( *pszPath )
	{
	  if(*pszPath++=='\\')
	    return pszPath;
	}
	return pszPath;
}

/*************************************************************************
 * PathAddExtensionA
 */
 
static void _PathAddDotA(LPSTR lpszPath)
{
	int len = strlen(lpszPath);
	if (len && lpszPath[len-1]!='.')
	{
	  lpszPath[len]  = '.';
	  lpszPath[len+1]= '\0';
	}
}

BOOL WINAPI PathAddExtensionA(
	LPSTR  pszPath,
	LPCSTR pszExtension)
{
	if (*pszPath)
	{
	  LPSTR pszExt = PathFindFileNameA(pszPath);	/* last path component */
	  pszExt = PathFindExtensionA(pszExt);	    
	  if (*pszExt != '\0') return FALSE;		/* already with extension */
	  _PathAddDotA(pszPath);
	}

	if (!pszExtension || *pszExtension=='\0')
	  strcat(pszPath, "exe");
	else
	  strcat(pszPath, pszExtension);
	return TRUE;
}

/*************************************************************************
 *	PathAddExtensionW
 */
static void _PathAddDotW(LPWSTR lpszPath)
{
	int len = lstrlenW(lpszPath);
	if (len && lpszPath[len-1]!='.')
	{
	  lpszPath[len]  = '.';
	  lpszPath[len+1]= '\0';
	}
}

/*************************************************************************
 *	PathAddExtensionW
 */
BOOL WINAPI PathAddExtensionW(
	LPWSTR  pszPath,
	LPCWSTR pszExtension)
{
	static const WCHAR ext[] = { 'e','x','e',0 };

	if (*pszPath)
	{
	  LPWSTR pszExt = PathFindFileNameW(pszPath);	/* last path component */
	  pszExt = PathFindExtensionW(pszExt);	    
	  if (*pszExt != '\0') return FALSE;		/* already with extension */
	  _PathAddDotW(pszPath);
	}

	if (!pszExtension || *pszExtension=='\0')
	  lstrcatW(pszPath, ext);
	else
	  lstrcatW(pszPath, pszExtension);
	return TRUE;

}

/*************************************************************************
 *	PathIsUNCServerA
 */
BOOL WINAPI PathIsUNCServerA(
	LPCSTR pszPath)
{
	FIXME("%s\n", pszPath);
	return FALSE;
}

/*************************************************************************
 *	PathIsUNCServerW
 */
BOOL WINAPI PathIsUNCServerW(
	LPCWSTR pszPath)
{
	FIXME("%s\n", debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 *	PathIsUNCServerShareA
 */
BOOL WINAPI PathIsUNCServerShareA(
	LPCSTR pszPath)
{
	FIXME("%s\n", pszPath);
	return FALSE;
}

/*************************************************************************
 *	PathIsUNCServerShareW
 */
BOOL WINAPI PathIsUNCServerShareW(
	LPCWSTR pszPath)
{
	FIXME("%s\n", debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 *	PathMakePrettyA
 */
BOOL WINAPI PathMakePrettyA(
	LPSTR lpPath)
{
	FIXME("%s\n", lpPath);
	return TRUE;
}

/*************************************************************************
 *	PathMakePrettyW
 */
BOOL WINAPI PathMakePrettyW(
	LPWSTR lpPath)
{
	FIXME("%s\n", debugstr_w(lpPath));
	return TRUE;
}
