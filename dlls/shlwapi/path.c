/*
 * Path Functions
 */

#include <ctype.h>
#include <string.h>

#include "winerror.h"
#include "wine/unicode.h"
#include "wine/undocshell.h"
#include "shlwapi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell);

#define isSlash(x) ((x)=='\\' || (x)=='/')
/*
	########## Combining and Constructing paths ##########
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
BOOL WINAPI PathAppendA(
	LPSTR lpszPath1,
	LPCSTR lpszPath2) 
{
	TRACE("%s %s\n",lpszPath1, lpszPath2);
	while (lpszPath2[0]=='\\') lpszPath2++;
	PathCombineA(lpszPath1,lpszPath1,lpszPath2);
	return TRUE;
}

/*************************************************************************
 * PathAppendW		[SHLWAPI.@]
 */
BOOL WINAPI PathAppendW(
	LPWSTR lpszPath1,
	LPCWSTR lpszPath2) 
{
	TRACE("%s %s\n",debugstr_w(lpszPath1), debugstr_w(lpszPath2));
	while (lpszPath2[0]=='\\') lpszPath2++;
	PathCombineW(lpszPath1,lpszPath1,lpszPath2);
	return TRUE;
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
          strcpyW(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathGetDriveNumberW(lpszFile) != -1)
	{
            strcpyW(szDest,lpszFile);
	}
	else if (lpszFile[0] == (WCHAR)'\\' )
	{
	  strcpyW(sTemp,lpszDir);
	  PathStripToRootW(sTemp);
	  strcatW(sTemp,lpszFile);
	  strcpyW(szDest,sTemp);
	}
	else
	{
	  strcpyW(sTemp,lpszDir);
	  PathAddBackslashW(sTemp);
	  strcatW(sTemp,lpszFile);
	  strcpyW(szDest,sTemp);
	}
	return szDest;
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

	len = strlenW(lpszPath);
	if (len && lpszPath[len-1]!=(WCHAR)'\\') 
	{
	  lpszPath[len]  = (WCHAR)'\\';
	  lpszPath[len+1]= 0x00;
	  return lpszPath+len+1;
	}
	return lpszPath+len;
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

/*
	Extracting Component Parts
*/

/*************************************************************************
 * PathFindFileNameA	[SHLWAPI.@]
 */
LPSTR WINAPI PathFindFileNameA(LPCSTR lpszPath)
{
	LPCSTR lastSlash = lpszPath;

	TRACE("%s\n",lpszPath);
	while (*lpszPath) 
	{
	  if ( isSlash(lpszPath[0]) && lpszPath[1])
	      lastSlash = lpszPath+1;
	  lpszPath = CharNextA(lpszPath);
	}
	return (LPSTR)lastSlash;

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
	  lpszPath = CharNextW(lpszPath);
	}
	return (LPWSTR)wslash;	
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
	  lpszPath = CharNextA(lpszPath);
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
	  lpszPath = CharNextW(lpszPath);
	}
	return (LPWSTR)(lastpoint?lastpoint:lpszPath);
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
	  lpszPath = CharNextA(lpszPath);
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
	  lpszPath = CharNextW(lpszPath);
	}
	return (LPWSTR)lpszPath;
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
	int chr = tolowerW(lpszPath[0]);
	
	TRACE ("%s\n",debugstr_w(lpszPath));

	if (!lpszPath || lpszPath[1]!=':' || chr < 'a' || chr > 'z') return -1;
	return tolowerW(lpszPath[0]) - 'a' ;
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
	LPSTR cutplace = lpszPath;
	BOOL ret = FALSE;
	
	TRACE("%s\n",lpszPath);

	if(lpszPath)
	{
	  while (*lpszPath == '\\') cutplace = ++lpszPath;
	  
	  while (*lpszPath)
	  {
	    if(lpszPath[0] == '\\') cutplace = lpszPath;
	  
	    if(lpszPath[0] == ':')
	    {
	      cutplace = lpszPath + 1;
	      if (lpszPath[1] == '\\') cutplace++;
	      lpszPath++;
	    }
	    lpszPath = CharNextA(lpszPath);
	    if (!lpszPath) break;
	  }
	  
	  ret = (*cutplace!='\0');
	  *cutplace = '\0';
	}
	return ret;
}

/*************************************************************************
 * PathRemoveFileSpecW	[SHLWAPI.@]
 */
BOOL WINAPI PathRemoveFileSpecW(LPWSTR lpszPath)
{
	LPWSTR cutplace = lpszPath;
	BOOL ret = FALSE;

	TRACE("%s\n",debugstr_w(lpszPath));

	if(lpszPath)
	{
	  while (*lpszPath == '\\') cutplace = ++lpszPath;
	  
	  while (*lpszPath)
	  {
	    if(lpszPath[0] == '\\') cutplace = lpszPath;
	  
	    if(lpszPath[0] == ':')
	    {
	      cutplace = lpszPath + 1;
	      if (lpszPath[1] == '\\') cutplace++;
	      lpszPath++;
	    }
	    lpszPath = CharNextW(lpszPath);
	    if (!lpszPath) break;
	  }
	  
	  ret = (*cutplace!='\0');
	  *cutplace = '\0';
	}
	return ret;
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
	  RtlMoveMemory(lpszPath, lpszFileName, strlen(lpszFileName)+1); 
}

/*************************************************************************
 * PathStripPathW	[SHELLWAPI.@]
 */
void WINAPI PathStripPathW(LPWSTR lpszPath)
{
	LPWSTR lpszFileName = PathFindFileNameW(lpszPath);

	TRACE("%s\n", debugstr_w(lpszPath));
	if(lpszFileName)
	  RtlMoveMemory(lpszPath, lpszFileName, (strlenW(lpszFileName)+1)*sizeof(WCHAR)); 
}

/*************************************************************************
 * PathStripToRootA	[SHLWAPI.@]
 */
BOOL WINAPI PathStripToRootA(LPSTR lpszPath)
{
	TRACE("%s\n", lpszPath);

	if (!lpszPath) return FALSE;
	while(!PathIsRootA(lpszPath))
	  if (!PathRemoveFileSpecA(lpszPath)) return FALSE;
	return TRUE;
}

/*************************************************************************
 * PathStripToRootW	[SHLWAPI.@]
 */
BOOL WINAPI PathStripToRootW(LPWSTR lpszPath)
{
	TRACE("%s\n", debugstr_w(lpszPath));

	if (!lpszPath) return FALSE;
	while(!PathIsRootW(lpszPath))
	  if (!PathRemoveFileSpecW(lpszPath)) return FALSE;
	return TRUE;
}

/*************************************************************************
 * PathRemoveArgsA	[SHLWAPI.@]
 *
 */
void WINAPI PathRemoveArgsA(LPSTR lpszPath)
{
	TRACE("%s\n",lpszPath);
	
	if(lpszPath)
	{
	  LPSTR lpszArgs = PathGetArgsA(lpszPath);
	  if (!*lpszArgs)
	  {
	    LPSTR lpszLastChar = CharPrevA(lpszPath, lpszArgs);
	    if(*lpszLastChar==' ') *lpszLastChar = '\0';
	  }
	}
}

/*************************************************************************
 * PathRemoveArgsW	[SHLWAPI.@]
 */
void WINAPI PathRemoveArgsW(LPWSTR lpszPath)
{
	TRACE("%s\n", debugstr_w(lpszPath));

	if(lpszPath)
	{
	  LPWSTR lpszArgs = PathGetArgsW(lpszPath);
	  if (!*lpszArgs)
	  {
	    LPWSTR lpszLastChar = CharPrevW(lpszPath, lpszArgs);
	    if(*lpszLastChar==' ') *lpszLastChar = '\0';
	  }
	}
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
 * PathRemoveBackslashA	[SHLWAPI.@]
 *
 * If the path ends in a backslash it is replaced by a NULL
 * and the address of the NULL is returned
 * Otherwise 
 * the address of the last character is returned.
 *
 * FIXME
 *  "c:\": keep backslash
 */
LPSTR WINAPI PathRemoveBackslashA( LPSTR lpszPath )
{
	int len;
	LPSTR szTemp = NULL;
	
	if(lpszPath)
	{
	  len = strlen(lpszPath);
	  szTemp = CharPrevA(lpszPath, lpszPath+len);
	  if (! PathIsRootA(lpszPath))
	  {
	    if (*szTemp == '\\') *szTemp = '\0';
	  }
	}
	return szTemp;
}

/*************************************************************************
 * PathRemoveBackslashW	[SHLWAPI.@]
 */
LPWSTR WINAPI PathRemoveBackslashW( LPWSTR lpszPath )
{
	int len;
	LPWSTR szTemp = NULL;
	
	if(lpszPath)
	{
	  len = strlenW(lpszPath);
	  szTemp = CharPrevW(lpszPath, lpszPath+len);
	  if (! PathIsRootW(lpszPath))
	  {
	    if (*szTemp == '\\') *szTemp = '\0';
	  }
	}
	return szTemp;
}


/*
	Path Manipulations
*/

/*************************************************************************
 * PathRemoveBlanksA [SHLWAPI.@]
 * 
 * NOTES
 *     remove spaces from beginning and end of passed string
 */
void WINAPI PathRemoveBlanksA(LPSTR str)
{
	LPSTR x = str;

	TRACE("%s\n",str);

	if(str)
	{
	  while (*x==' ') x = CharNextA(x);
	  if (x!=str) strcpy(str,x);
	  x=str+strlen(str)-1;
	  while (*x==' ') x = CharPrevA(str, x);
	  if (*x==' ') *x='\0';
	}
}

/*************************************************************************
 * PathRemoveBlanksW [SHLWAPI.@]
 */
void WINAPI PathRemoveBlanksW(LPWSTR str)
{
	LPWSTR x = str;

	TRACE("%s\n",debugstr_w(str));

	if(str)
	{
	  while (*x==' ') x = CharNextW(x);
	  if (x!=str) lstrcpyW(str,x);
	  x=str+strlenW(str)-1;
	  while (*x==' ') x = CharPrevW(str, x);
	  if (*x==' ') *x='\0';
	}
}

/*************************************************************************
 * PathQuoteSpacesA [SHLWAPI.@]
 * 
 */
LPSTR WINAPI PathQuoteSpacesA(LPSTR lpszPath)
{
	TRACE("%s\n",lpszPath);

	if(StrChrA(lpszPath,' '))
	{
	  int len = strlen(lpszPath);
	  RtlMoveMemory(lpszPath+1, lpszPath, len);
	  *(lpszPath++) = '"';
	  lpszPath += len;
	  *(lpszPath++) = '"';
	  *(lpszPath) = '\0';
	  return --lpszPath;
	}
	return 0;
}

/*************************************************************************
 * PathQuoteSpacesW [SHLWAPI.@]
 */
LPWSTR WINAPI PathQuoteSpacesW(LPWSTR lpszPath)
{
	TRACE("%s\n",debugstr_w(lpszPath));

	if(StrChrW(lpszPath,' '))
	{
	  int len = strlenW(lpszPath);
	  RtlMoveMemory(lpszPath+1, lpszPath, len*sizeof(WCHAR));
	  *(lpszPath++) = '"';
	  lpszPath += len;
	  *(lpszPath++) = '"';
	  *(lpszPath) = '\0';
	  return --lpszPath;
	}
	return 0;
}

/*************************************************************************
 * PathUnquoteSpacesA [SHLWAPI.@]
 * 
 * NOTES
 *     unquote string (remove ")
 */
VOID WINAPI PathUnquoteSpacesA(LPSTR str) 
{
	DWORD len = strlen(str);

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
	DWORD len = strlenW(str);

	TRACE("%s\n",debugstr_w(str));

	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	strcpyW(str,str+1);
	return;
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
	
	PathUnquoteSpacesA(lpszPath);
	return 0;
}

/*************************************************************************
 * PathParseIconLocationW	[SHLWAPI.@]
 */
int WINAPI PathParseIconLocationW(LPWSTR lpszPath)
{
	LPWSTR lpstrComma = strchrW(lpszPath, ',');
	
	FIXME("%s stub\n", debugstr_w(lpszPath));

	if (lpstrComma && lpstrComma[1])
	{
	  lpstrComma[0]='\0';
/*	  return _wtoi(&lpstrComma[1]);	FIXME */
	}
	PathUnquoteSpacesW(lpszPath);
	return 0;
}

/*
	########## cleaning and resolving paths ##########
 */

/*************************************************************************
 * PathFindOnPathA	[SHLWAPI.@]
 */
BOOL WINAPI PathFindOnPathA(LPSTR sFile, LPCSTR sOtherDirs)
{
	FIXME("%s %s\n",sFile, sOtherDirs);
	return FALSE;
}

/*************************************************************************
 * PathFindOnPathW	[SHLWAPI.@]
 */
BOOL WINAPI PathFindOnPathW(LPWSTR sFile, LPCWSTR sOtherDirs)
{
	FIXME("%s %s\n",debugstr_w(sFile), debugstr_w(sOtherDirs));
	return FALSE;
}

/*************************************************************************
 * PathCleanupSpecA	[SHLWAPI.@]
 */
DWORD WINAPI PathCleanupSpecA(LPSTR x, LPSTR y)
{
	FIXME("(%p %s, %p %s) stub\n",x,debugstr_a(x),y,debugstr_a(y));
	return TRUE;
}

/*************************************************************************
 * PathCleanupSpecW	[SHLWAPI.@]
 */
DWORD WINAPI PathCleanupSpecW(LPWSTR x, LPWSTR y)
{
	FIXME("(%p %s, %p %s) stub\n",x,debugstr_w(x),y,debugstr_w(y));
	return TRUE;
}

/*************************************************************************
 *	PathCompactPathExA   [SHLWAPI.@]
 */
BOOL WINAPI PathCompactPathExA(
	LPSTR pszOut,
	LPCSTR pszSrc,
	UINT cchMax,
	DWORD dwFlags)
{
	FIXME("%p %s 0x%08x 0x%08lx\n", pszOut, pszSrc, cchMax, dwFlags);
	return FALSE;
}

/*************************************************************************
 *	PathCompactPathExW   [SHLWAPI.@]
 */
BOOL WINAPI PathCompactPathExW(
	LPWSTR pszOut,
	LPCWSTR pszSrc,
	UINT cchMax,
	DWORD dwFlags)
{
	FIXME("%p %s 0x%08x 0x%08lx\n", pszOut, debugstr_w(pszSrc), cchMax, dwFlags);
	return FALSE;
}

/*
	########## Path Testing ##########
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
	    if (*lpszPath=='\\') foundbackslash++;
	    lpszPath = CharNextA(lpszPath);
	  }
	  if (foundbackslash <= 1)
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
	    if (*lpszPath=='\\') foundbackslash++;
	    lpszPath = CharNextW(lpszPath);
	  }
	  if (foundbackslash <= 1)
	    return TRUE;
	}
	return FALSE;

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
	  if (!strcmpiW(lpszExtension,lpszExtensions[i])) return TRUE;
	  
	return FALSE;
}

/*************************************************************************
 * PathIsDirectoryA	[SHLWAPI.@]
 */
BOOL WINAPI PathIsDirectoryA(LPCSTR lpszPath)
{
	DWORD dwAttr;

	TRACE("%s\n", debugstr_a(lpszPath));

	dwAttr = GetFileAttributesA(lpszPath);
	return  (dwAttr != -1) ? dwAttr & FILE_ATTRIBUTE_DIRECTORY : 0;
}

/*************************************************************************
 * PathIsDirectoryW	[SHLWAPI.@]
 */
BOOL WINAPI PathIsDirectoryW(LPCWSTR lpszPath)
{
	DWORD dwAttr;
	
	TRACE("%s\n", debugstr_w(lpszPath));

	dwAttr = GetFileAttributesW(lpszPath);
	return  (dwAttr != -1) ? dwAttr & FILE_ATTRIBUTE_DIRECTORY : 0;
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
	  name = CharNextA(name);
	  mask = CharNextA(mask);
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
	  if (toupperW(*mask)!=toupperW(*name) && *mask!='?') return 0;
	  name = CharNextW(name);
	  mask = CharNextW(mask);
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
	  while (*mask && *mask!=';') mask = CharNextA(mask);
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
	  while (*mask && *mask!=';') mask = CharNextW(mask);
	  if (*mask==';') 
	  {
	    mask++;
	    while (*mask==' ') mask++;       /* masks may be separated by "; " */
	  }
	}
	return 0;
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
	    pos++; /* fixme: use CharNext*/
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
	if ( toupperW(lpszPath1[0])==toupperW(lpszPath2[0]) &&
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
	    pos++;/* fixme: use CharNext*/
	  }
	  return (lpszPath1[pos] == lpszPath2[pos]);
	}
	return FALSE;
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
	    if(!strncasecmp(lpstrPath, SupportedProtocol[i], iSize))
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
	lpstrRes = strchrW(lpstrPath,':');
	if(!lpstrRes) return FALSE;
	iSize = lpstrRes - lpstrPath;

	while(SupportedProtocol[i])
	{
	  if (iSize == strlenW(SupportedProtocol[i]))
	    if(!strncmpiW(lpstrPath, SupportedProtocol[i], iSize))
	      return TRUE;
	  i++;
	}

	return FALSE;
}  


/*************************************************************************
 *	PathIsContentTypeA   [SHLWAPI.@]
 */
BOOL WINAPI PathIsContentTypeA(LPCSTR pszPath, LPCSTR pszContentType)
{
	FIXME("%s %s\n", pszPath, pszContentType);
	return FALSE;
}

/*************************************************************************
 *	PathIsContentTypeW   [SHLWAPI.@]
 */
BOOL WINAPI PathIsContentTypeW(LPCWSTR pszPath, LPCWSTR pszContentType)
{
	FIXME("%s %s\n", debugstr_w(pszPath), debugstr_w(pszContentType));
	return FALSE;
}

/*************************************************************************
 *	PathIsFileSpecA   [SHLWAPI.@]
 */
BOOL WINAPI PathIsFileSpecA(LPCSTR pszPath)
{
	FIXME("%s\n", pszPath);
	return FALSE;
}

/*************************************************************************
 *	PathIsFileSpecW   [SHLWAPI.@]
 */
BOOL WINAPI PathIsFileSpecW(LPCWSTR pszPath)
{
	FIXME("%s\n", debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 *	PathIsPrefixA   [SHLWAPI.@]
 */
BOOL WINAPI PathIsPrefixA(LPCSTR pszPrefix, LPCSTR pszPath)
{
	FIXME("%s %s\n", pszPrefix, pszPath);
	return FALSE;
}

/*************************************************************************
 *	PathIsPrefixW   [SHLWAPI.@]
 */
BOOL WINAPI PathIsPrefixW(LPCWSTR pszPrefix, LPCWSTR pszPath)
{
	FIXME("%s %s\n", debugstr_w(pszPrefix), debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 *	PathIsSystemFolderA   [SHLWAPI.@]
 */
BOOL WINAPI PathIsSystemFolderA(LPCSTR pszPath, DWORD dwAttrb)
{
	FIXME("%s 0x%08lx\n", pszPath, dwAttrb);
	return FALSE;
}

/*************************************************************************
 *	PathIsSystemFolderW   [SHLWAPI.@]
 */
BOOL WINAPI PathIsSystemFolderW(LPCWSTR pszPath, DWORD dwAttrb)
{
	FIXME("%s 0x%08lx\n", debugstr_w(pszPath), dwAttrb);
	return FALSE;
}

/*************************************************************************
 *	PathIsUNCServerA   [SHLWAPI.@]
 */
BOOL WINAPI PathIsUNCServerA(
	LPCSTR pszPath)
{
	FIXME("%s\n", pszPath);
	return FALSE;
}

/*************************************************************************
 *	PathIsUNCServerW   [SHLWAPI.@]
 */
BOOL WINAPI PathIsUNCServerW(
	LPCWSTR pszPath)
{
	FIXME("%s\n", debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 *	PathIsUNCServerShareA   [SHLWAPI.@]
 */
BOOL WINAPI PathIsUNCServerShareA(
	LPCSTR pszPath)
{
	FIXME("%s\n", pszPath);
	return FALSE;
}

/*************************************************************************
 *	PathIsUNCServerShareW   [SHLWAPI.@]
 */
BOOL WINAPI PathIsUNCServerShareW(
	LPCWSTR pszPath)
{
	FIXME("%s\n", debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 * PathCanonicalizeA   [SHLWAPI.@]
 *
 *  FIXME
 *   returnvalue, use CharNext
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
 * PathCanonicalizeW   [SHLWAPI.@]
 *
 *  FIXME
 *   returnvalue, use CharNext
 */
BOOL WINAPI PathCanonicalizeW(LPWSTR pszBuf, LPCWSTR pszPath)
{
	int OffsetMin = 0, OffsetSrc = 0, OffsetDst = 0, LenSrc = strlenW(pszPath);
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
 * PathFindNextComponentA   [SHLWAPI.@]
 *
 * NOTES
 * special cases:
 *	""              null
 *	aa              "" (pointer to traling NULL)
 *	aa\             "" (pointer to traling NULL)
 *	aa\\            "" (pointer to traling NULL)
 *	aa\\bb          bb
 *	aa\\\bb         \bb
 *	c:\aa\          "aa\"
 *	\\aa            aa
 *	\\aa\b          aa\b
*/
LPSTR WINAPI PathFindNextComponentA(LPCSTR pszPath)
{
	LPSTR pos;

	TRACE("%s\n", pszPath);

	if(!pszPath || !*pszPath) return NULL;
	if(!(pos = StrChrA(pszPath, '\\')))
	  return (LPSTR) pszPath + strlen(pszPath);
	pos++;
	if(pos[0] == '\\') pos++;
	return pos;
}

/*************************************************************************
 * PathFindNextComponentW   [SHLWAPI.@]
 */
LPWSTR WINAPI PathFindNextComponentW(LPCWSTR pszPath)
{
	LPWSTR pos;

	TRACE("%s\n", debugstr_w(pszPath));
	
	if(!pszPath || !*pszPath) return NULL;
	if (!(pos = StrChrW(pszPath, '\\')))
	  return (LPWSTR) pszPath + strlenW(pszPath);
	pos++;
	if(pos[0] == '\\') pos++;
	return pos;
}

/*************************************************************************
 * PathAddExtensionA   [SHLWAPI.@]
 *
 * NOTES
 *  it adds never a dot
 */
 
BOOL WINAPI PathAddExtensionA(
	LPSTR  pszPath,
	LPCSTR pszExtension)
{
	if (*pszPath)
	{
	  if (*(PathFindExtensionA(pszPath))) return FALSE;

	  if (!pszExtension || *pszExtension=='\0')
	    strcat(pszPath, "exe");
	  else
	    strcat(pszPath, pszExtension);
	}

	return TRUE;
}

/*************************************************************************
 *	PathAddExtensionW   [SHLWAPI.@]
 */
BOOL WINAPI PathAddExtensionW(
	LPWSTR  pszPath,
	LPCWSTR pszExtension)
{
	static const WCHAR ext[] = { 'e','x','e',0 };

	if (*pszPath)
	{
	  if (*(PathFindExtensionW(pszPath))) return FALSE;

	  if (!pszExtension || *pszExtension=='\0')
	    lstrcatW(pszPath, ext);
	  else
	    lstrcatW(pszPath, pszExtension);
	}
	return TRUE;

}

/*************************************************************************
 *	PathMakePrettyA   [SHLWAPI.@]
 */
BOOL WINAPI PathMakePrettyA(
	LPSTR lpPath)
{
	FIXME("%s\n", lpPath);
	return TRUE;
}

/*************************************************************************
 *	PathMakePrettyW   [SHLWAPI.@]
 */
BOOL WINAPI PathMakePrettyW(
	LPWSTR lpPath)
{
	FIXME("%s\n", debugstr_w(lpPath));
	return TRUE;

}

/*************************************************************************
 *	PathCommonPrefixA   [SHLWAPI.@]
 */
int WINAPI PathCommonPrefixA(
	LPCSTR pszFile1,
	LPCSTR pszFile2,
	LPSTR achPath)
{
	FIXME("%s %s %p\n", pszFile1, pszFile2, achPath);
	return 0;
}

/*************************************************************************
 *	PathCommonPrefixW   [SHLWAPI.@]
 */
int WINAPI PathCommonPrefixW(
	LPCWSTR pszFile1,
	LPCWSTR pszFile2,
	LPWSTR achPath)
{
	FIXME("%s %s %p\n", debugstr_w(pszFile1), debugstr_w(pszFile2),achPath );
	return 0;
}

/*************************************************************************
 *	PathCompactPathA   [SHLWAPI.@]
 */
BOOL WINAPI PathCompactPathA(HDC hDC, LPSTR pszPath, UINT dx)
{
	FIXME("0x%08x %s 0x%08x\n", hDC, pszPath, dx);
	return FALSE;
}

/*************************************************************************
 *	PathCompactPathW   [SHLWAPI.@]
 */
BOOL WINAPI PathCompactPathW(HDC hDC, LPWSTR pszPath, UINT dx)
{
	FIXME("0x%08x %s 0x%08x\n", hDC, debugstr_w(pszPath), dx);
	return FALSE;
}

/*************************************************************************
 *	PathGetCharTypeA   [SHLWAPI.@]
 */
UINT WINAPI PathGetCharTypeA(UCHAR ch)
{
	FIXME("%c\n", ch);
	return 0;
}

/*************************************************************************
 *	PathGetCharTypeW   [SHLWAPI.@]
 */
UINT WINAPI PathGetCharTypeW(WCHAR ch)
{
	FIXME("%c\n", ch);
	return 0;
}

/*************************************************************************
 *	PathMakeSystemFolderA   [SHLWAPI.@]
 */
BOOL WINAPI PathMakeSystemFolderA(LPCSTR pszPath)
{
	FIXME("%s\n", pszPath);
	return FALSE;
}

/*************************************************************************
 *	PathMakeSystemFolderW   [SHLWAPI.@]
 */
BOOL WINAPI PathMakeSystemFolderW(LPCWSTR pszPath)
{
	FIXME("%s\n", debugstr_w(pszPath));
	return FALSE;
}

/*************************************************************************
 *	PathRenameExtensionA   [SHLWAPI.@]
 */
BOOL WINAPI PathRenameExtensionA(LPSTR pszPath, LPCSTR pszExt)
{
	FIXME("%s %s\n", pszPath, pszExt);
	return FALSE;
}

/*************************************************************************
 *	PathRenameExtensionW   [SHLWAPI.@]
 */
BOOL WINAPI PathRenameExtensionW(LPWSTR pszPath, LPCWSTR pszExt)
{
	FIXME("%s %s\n", debugstr_w(pszPath), debugstr_w(pszExt));
	return FALSE;
}

/*************************************************************************
 *	PathSearchAndQualifyA   [SHLWAPI.@]
 */
BOOL WINAPI PathSearchAndQualifyA(
	LPCSTR pszPath,
	LPSTR pszBuf,
	UINT cchBuf)
{
	FIXME("%s %s 0x%08x\n", pszPath, pszBuf, cchBuf);
	return FALSE;
}

/*************************************************************************
 *	PathSearchAndQualifyW   [SHLWAPI.@]
 */
BOOL WINAPI PathSearchAndQualifyW(
	LPCWSTR pszPath,
	LPWSTR pszBuf,
	UINT cchBuf)
{
	FIXME("%s %s 0x%08x\n", debugstr_w(pszPath), debugstr_w(pszBuf), cchBuf);
	return FALSE;
}

/*************************************************************************
 *	PathSkipRootA   [SHLWAPI.@]
 */
LPSTR WINAPI PathSkipRootA(LPCSTR pszPath)
{
	FIXME("%s\n", pszPath);
	return (LPSTR)pszPath;
}

/*************************************************************************
 *	PathSkipRootW   [SHLWAPI.@]
 */
LPWSTR WINAPI PathSkipRootW(LPCWSTR pszPath)
{
	FIXME("%s\n", debugstr_w(pszPath));
	return (LPWSTR)pszPath;
}

/*************************************************************************
 *	PathCreateFromUrlA   [SHLWAPI.@]
 */
HRESULT WINAPI PathCreateFromUrlA(
	LPCSTR pszUrl,
	LPSTR pszPath,
	LPDWORD pcchPath,
	DWORD dwFlags)
{
	FIXME("%s %p %p 0x%08lx\n",
	  pszUrl, pszPath, pcchPath, dwFlags);
	return S_OK;
}

/*************************************************************************
 *	PathCreateFromUrlW   [SHLWAPI.@]
 */
HRESULT WINAPI PathCreateFromUrlW(
	LPCWSTR pszUrl,
	LPWSTR pszPath,
	LPDWORD pcchPath,
	DWORD dwFlags)
{
	FIXME("%s %p %p 0x%08lx\n",
	  debugstr_w(pszUrl), pszPath, pcchPath, dwFlags);
	return S_OK;
}

/*************************************************************************
 *	PathRelativePathToA   [SHLWAPI.@]
 */
BOOL WINAPI PathRelativePathToA(
	LPSTR pszPath,
	LPCSTR pszFrom,
	DWORD dwAttrFrom,
	LPCSTR pszTo,
	DWORD dwAttrTo)
{
	FIXME("%s %s 0x%08lx %s 0x%08lx\n",
	  pszPath, pszFrom, dwAttrFrom, pszTo, dwAttrTo);
	return FALSE;
}

/*************************************************************************
 *	PathRelativePathToW   [SHLWAPI.@]
 */
BOOL WINAPI PathRelativePathToW(
	LPWSTR pszPath,
	LPCWSTR pszFrom,
	DWORD dwAttrFrom,
	LPCWSTR pszTo,
	DWORD dwAttrTo)
{
	FIXME("%s %s 0x%08lx %s 0x%08lx\n",
	  debugstr_w(pszPath), debugstr_w(pszFrom), dwAttrFrom, debugstr_w(pszTo), dwAttrTo);
	return FALSE;
}

/*************************************************************************
 *	PathUnmakeSystemFolderA   [SHLWAPI.@]
 */
BOOL WINAPI PathUnmakeSystemFolderA(LPCSTR pszPath)
{
	FIXME("%s\n", pszPath);
	return FALSE;
}

/*************************************************************************
 *	PathUnmakeSystemFolderW   [SHLWAPI.@]
 */
BOOL WINAPI PathUnmakeSystemFolderW(LPCWSTR pszPath)
{
	FIXME("%s\n", debugstr_w(pszPath));
	return FALSE;
}

/*
	########## special ##########
*/

/*************************************************************************
 * PathSetDlgItemPathA   [SHLWAPI.@]
 *
 * NOTES
 *  use PathCompactPath to make sure, the path fits into the control
 */
BOOL WINAPI PathSetDlgItemPathA(HWND hDlg, int id, LPCSTR pszPath) 
{	TRACE("%x %x %s\n",hDlg, id, pszPath);
	return SetDlgItemTextA(hDlg, id, pszPath);
}

/*************************************************************************
 * PathSetDlgItemPathW   [SHLWAPI.@]
 */
BOOL WINAPI PathSetDlgItemPathW(HWND hDlg, int id, LPCWSTR pszPath) 
{	TRACE("%x %x %s\n",hDlg, id, debugstr_w(pszPath));
	return SetDlgItemTextW(hDlg, id, pszPath);
}
