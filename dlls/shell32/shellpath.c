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
#include "tchar.h"

#include "shlobj.h"
#include "shell32_main.h"
#include "windef.h"
#include "options.h"

DEFAULT_DEBUG_CHANNEL(shell)

/* Supported protocols for PathIsURL */
LPSTR SupportedProtocol[] = {"http","https","ftp","gopher","file","mailto",""};

/*************************************************************************
 * PathIsRoot [SHELL32.29]
 */
BOOL WINAPI PathIsRootA(LPCSTR x)
{	TRACE("%s\n",x);
	if (*(x+1)==':' && *(x+2)=='\\')		/* "X:\" */
	  return 1;
	if (*x=='\\')		/* "\" */
	  return 0;
	if (x[0]=='\\' && x[1]=='\\')		/* UNC "\\<xx>\" */
	{ int	foundbackslash = 0;
	  x=x+2;
	  while (*x)
	  { if (*x++=='\\')
	      foundbackslash++;
	  }
	  if (foundbackslash<=1)	/* max 1 \ more ... */
	    return 1;
	}
	return 0;
}
BOOL WINAPI PathIsRootW(LPCWSTR x) 
{	TRACE("%s\n",debugstr_w(x));
	if (*(x+1)==':' && *(x+2)=='\\')		/* "X:\" */
	  return 1;
	if (*x == (WCHAR) '\\')		/* "\" */
	  return 0;
	if (x[0]==(WCHAR)'\\' && x[1]==(WCHAR)'\\')	/* UNC "\\<xx>\" */
	{ int	foundbackslash = 0;
	  x=x+2;
	  while (*x) 
	  { if (*x++==(WCHAR)'\\')
	      foundbackslash++;
	  }
	  if (foundbackslash<=1)	/* max 1 \ more ... */
	    return 1;
	}
	return 0;
}
BOOL WINAPI PathIsRootAW(LPCVOID x) 
{	if (VERSION_OsIsUnicode())
	  return PathIsRootW(x);
	return PathIsRootA(x);

}
/*************************************************************************
 * PathBuildRoot [SHELL32.30]
 */
LPSTR WINAPI PathBuildRootA(LPSTR root,BYTE drive) {
  TRACE("%p %i\n",root, drive);
	strcpy(root,"A:\\");
	root[0]+=drive;
	return root;
}

/*************************************************************************
 * PathFindExtension [SHELL32.31]
 *
 * NOTES
 *     returns pointer to last . in last pathcomponent or at \0.
 */
LPCSTR WINAPI PathFindExtensionA(LPCSTR path) 
{	LPCSTR   lastpoint = NULL;
	TRACE("%p %s\n",path,path);
	while (*path) 
	{ if (*path=='\\'||*path==' ')
	    lastpoint=NULL;
	  if (*path=='.')
	    lastpoint=path;
	  path++;
	}
	return lastpoint?lastpoint:path;
}
LPCWSTR WINAPI PathFindExtensionW(LPCWSTR path) 
{	LPCWSTR   lastpoint = NULL;
	TRACE("(%p %s)\n",path,debugstr_w(path));
	while (*path)
	{ if (*path==(WCHAR)'\\'||*path==(WCHAR)' ')
	    lastpoint=NULL;
	  if (*path==(WCHAR)'.')
	    lastpoint=path;
	  path++;
	}
	return lastpoint?lastpoint:path;
}
LPCVOID WINAPI PathFindExtensionAW(LPCVOID path) 
{	if (VERSION_OsIsUnicode())
	  return PathFindExtensionW(path);
	return PathFindExtensionA(path);

}

/*************************************************************************
 * PathAddBackslash [SHELL32.32]
 * 
 * NOTES
 *     append \ if there is none
 */
LPSTR WINAPI PathAddBackslashA(LPSTR path)
{	int len;
	TRACE("%p->%s\n",path,path);

	len = strlen(path);
	if (len && path[len-1]!='\\') 
	{ path[len]  = '\\';
	  path[len+1]= 0x00;
	  return path+len+1;
	}
	return path+len;
}
LPWSTR WINAPI PathAddBackslashW(LPWSTR path)
{	int len;
	TRACE("%p->%s\n",path,debugstr_w(path));

	len = CRTDLL_wcslen(path);
	if (len && path[len-1]!=(WCHAR)'\\') 
	{ path[len]  = (WCHAR)'\\';
	  path[len+1]= 0x00;
	  return path+len+1;
	}
	return path+len;
}
LPVOID WINAPI PathAddBackslashAW(LPVOID path)
{	if(VERSION_OsIsUnicode())
	  return PathAddBackslashW(path);
	return PathAddBackslashA(path);
}

/*************************************************************************
 * PathRemoveBlanks [SHELL32.33]
 * 
 * NOTES
 *     remove spaces from beginning and end of passed string
 */
LPSTR WINAPI PathRemoveBlanksA(LPSTR str)
{	LPSTR x = str;
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
LPWSTR WINAPI PathRemoveBlanksW(LPWSTR str)
{	LPWSTR x = str;
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
LPVOID WINAPI PathRemoveBlanksAW(LPVOID str)
{	if(VERSION_OsIsUnicode())
	  return PathRemoveBlanksW(str);
	return PathRemoveBlanksA(str);
}



/*************************************************************************
 * PathFindFilename [SHELL32.34]
 * 
 * NOTES
 *     basename(char *fn);
 */
LPCSTR WINAPI PathFindFilenameA(LPCSTR aptr)
{	LPCSTR aslash;
	aslash = aptr;

	TRACE("%s\n",aslash);
	while (aptr[0]) 
	{ if (((aptr[0]=='\\') || (aptr[0]==':')) && aptr[1] && aptr[1]!='\\')
	      aslash = aptr+1;
	  aptr++;
	}
	return aslash;

}
LPCWSTR WINAPI PathFindFilenameW(LPCWSTR wptr)
{	LPCWSTR wslash;
	wslash = wptr;

	TRACE("%s\n",debugstr_w(wslash));
	while (wptr[0]) 
	{ if (((wptr[0]=='\\') || (wptr[0]==':')) && wptr[1] && wptr[1]!='\\')
	    wslash = wptr+1;
	  wptr++;
	}
	return wslash;	
}
LPCVOID WINAPI PathFindFilenameAW(LPCVOID fn)
{
	if(VERSION_OsIsUnicode())
	  return PathFindFilenameW(fn);
	return PathFindFilenameA(fn);
}

/*************************************************************************
 * PathRemoveFileSpec [SHELL32.35]
 * 
 * NOTES
 *     bool getpath(char *pathname); truncates passed argument to a valid path
 *     returns if the string was modified or not.
 *     "\foo\xx\foo"-> "\foo\xx"
 *     "\" -> "\"
 *     "a:\foo"	-> "a:\"
 */
DWORD WINAPI PathRemoveFileSpecA(LPSTR fn) {
	LPSTR	x,cutplace;
  TRACE("%s\n",fn);
	if (!fn[0])
		return 0;
	x=fn;
	cutplace = fn;
	while (*x) {
		if (*x=='\\') {
			cutplace=x++;
			continue;
		}
		if (*x==':') {
			x++;
			if (*x=='\\')
				cutplace=++x;
			continue; /* already x++ed */
		}
		x++;
	}
	if (!*cutplace)
		return 0;
	if (cutplace==fn) {
		if (fn[0]=='\\') {
			if (!fn[1])
				return 0;
			fn[0]='\0';
			return 1;
		}
	}
	*cutplace='\0';
	return 1;
}

/*************************************************************************
 * PathAppend [SHELL32.36]
 * 
 * NOTES
 *     concat_paths(char*target,const char*add);
 *     concats "target\\add" and writes them to target
 */
LPSTR WINAPI PathAppendA(LPSTR x1,LPSTR x2) {
  TRACE("%s %s\n",x1,x2);
  while (x2[0]=='\\') x2++;
  return PathCombineA(x1,x1,x2);
}

/*************************************************************************
 * PathCombine [SHELL32.37]
 * 
 * NOTES
 *  if lpszFile='.' skip it
 *  szDest can be equal to lpszFile. Thats why we use sTemp
 */
LPSTR WINAPI PathCombineA(LPSTR szDest, LPCSTR lpszDir, LPCSTR lpszFile) 
{	char sTemp[MAX_PATH];
	TRACE("%p %p->%s %p->%s\n",szDest, lpszDir, lpszDir, lpszFile, lpszFile);
	
	
	if (!lpszFile || !lpszFile[0] || (lpszFile[0]=='.' && !lpszFile[1]) ) 
	{ strcpy(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathIsRootA(lpszFile))
	{ strcpy(szDest,lpszFile);
	}
	else
	{ strcpy(sTemp,lpszDir);
	  PathAddBackslashA(sTemp);
	  strcat(sTemp,lpszFile);
	  strcpy(szDest,sTemp);
	}
	return szDest;
}
LPWSTR WINAPI PathCombineW(LPWSTR szDest, LPCWSTR lpszDir, LPCWSTR lpszFile) 
{	WCHAR sTemp[MAX_PATH];
	TRACE("%p %p->%s %p->%s\n",szDest, lpszDir, debugstr_w(lpszDir),
			 lpszFile, debugstr_w(lpszFile));
	
	
	if (!lpszFile || !lpszFile[0] || (lpszFile[0]==(WCHAR)'.' && !lpszFile[1]) ) 
	{ CRTDLL_wcscpy(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathIsRootW(lpszFile))
	{ CRTDLL_wcscpy(szDest,lpszFile);
	}
	else
	{ CRTDLL_wcscpy(sTemp,lpszDir);
	  PathAddBackslashW(sTemp);
	  CRTDLL_wcscat(sTemp,lpszFile);
	  CRTDLL_wcscpy(szDest,sTemp);
	}
	return szDest;
}
LPVOID WINAPI PathCombineAW(LPVOID szDest, LPCVOID lpszDir, LPCVOID lpszFile) 
{	if (VERSION_OsIsUnicode())
	  return PathCombineW( szDest, lpszDir, lpszFile );
	return PathCombineA( szDest, lpszDir, lpszFile );
}

/*************************************************************************
 * PathIsUNC [SHELL32.39]
 * 
 * NOTES
 *     PathIsUNC(char*path);
 */
BOOL WINAPI PathIsUNCA(LPCSTR path) 
{	TRACE("%s\n",path);

	if ((path[0]=='\\') && (path[1]=='\\'))
	  return TRUE;
	return FALSE;
}
BOOL WINAPI PathIsUNCW(LPCWSTR path) 
{	TRACE("%s\n",debugstr_w(path));

	if ((path[0]=='\\') && (path[1]=='\\'))
	  return TRUE;
	return FALSE;
}
BOOL WINAPI PathIsUNCAW (LPCVOID path)
{	if (VERSION_OsIsUnicode())
	  return PathIsUNCW( path );
	return PathIsUNCA( path );  
}
/*************************************************************************
 *  PathIsRelativ [SHELL32.40]
 * 
 */
BOOL WINAPI PathIsRelativeA (LPCSTR path)
{	TRACE("path=%s\n",path);

	if (path && (path[0]!='\\' && path[1]==':'))
	  return TRUE;
	return FALSE;    
}
BOOL WINAPI PathIsRelativeW (LPCWSTR path)
{	TRACE("path=%s\n",debugstr_w(path));

	if (path && (path[0]!='\\' && path[1]==':'))
	  return TRUE;
	return FALSE;    
}
BOOL WINAPI PathIsRelativeAW (LPCVOID path)
{	if (VERSION_OsIsUnicode())
	  return PathIsRelativeW( path );
	return PathIsRelativeA( path );  
}
/*************************************************************************
 *  PathIsExe [SHELL32.43]
 * 
 */
BOOL WINAPI PathIsExeA (LPCSTR path)
{	FIXME("path=%s\n",path);
	return FALSE;
}
BOOL WINAPI PathIsExeW (LPCWSTR path)
{	FIXME("path=%s\n",debugstr_w(path));
	return FALSE;
}
BOOL WINAPI PathIsExeAW (LPCVOID path)
{	if (VERSION_OsIsUnicode())
	  return PathIsExeW (path);
	return PathIsExeA(path);
}

/*************************************************************************
 * PathFileExists [SHELL32.45]
 * 
 * NOTES
 *     file_exists(char *fn);
 */
BOOL WINAPI PathFileExistsA(LPSTR fn) {
  TRACE("%s\n",fn);
   if (GetFileAttributesA(fn)==-1)
    	return FALSE;
    else
    	return TRUE;
}
/*************************************************************************
 * PathMatchSingleMask
 * 
 * NOTES
 *     internal (used by PathMatchSpec)
 */
static BOOL PathMatchSingleMaskA(LPCSTR name, LPCSTR mask)
{
  while (*name && *mask && *mask!=';') {
    if (*mask=='*') {
      do {
	if (PathMatchSingleMaskA(name,mask+1)) return 1;  /* try substrings */
      } while (*name++);
      return 0;
    }
    if (toupper(*mask)!=toupper(*name) && *mask!='?') return 0;
    name++;
    mask++;
  }
  if (!*name) {
    while (*mask=='*') mask++;
    if (!*mask || *mask==';') return 1;
  }
  return 0;
}
static BOOL PathMatchSingleMaskW(LPCWSTR name, LPCWSTR mask)
{
  while (*name && *mask && *mask!=';') {
    if (*mask=='*') {
      do {
	if (PathMatchSingleMaskW(name,mask+1)) return 1;  /* try substrings */
      } while (*name++);
      return 0;
    }
    if (towupper(*mask)!=towupper(*name) && *mask!='?') return 0;
    name++;
    mask++;
  }
  if (!*name) {
    while (*mask=='*') mask++;
    if (!*mask || *mask==';') return 1;
  }
  return 0;
}
/*************************************************************************
 * PathMatchSpec [SHELL32.46]
 * 
 * NOTES
 *     used from COMDLG32
 */
BOOL WINAPI PathMatchSpecA(LPCSTR name, LPCSTR mask) 
{
  TRACE("%s %s\n",name,mask);

  if (!lstrcmpA( mask, "*.*" )) return 1;   /* we don't require a period */

  while (*mask) {
    if (PathMatchSingleMaskA(name,mask)) return 1;    /* helper function */
    while (*mask && *mask!=';') mask++;
    if (*mask==';') {
      mask++;
      while (*mask==' ') mask++;      /*  masks may be separated by "; " */
	  }
	}
  return 0;
}
BOOL WINAPI PathMatchSpecW(LPCWSTR name, LPCWSTR mask) 
{
  WCHAR stemp[4];
  TRACE("%s %s\n",debugstr_w(name),debugstr_w(mask));

	lstrcpyAtoW(stemp,"*.*");	
  if (!lstrcmpW( mask, stemp )) return 1;   /* we don't require a period */

  while (*mask) {
    if (PathMatchSingleMaskW(name,mask)) return 1;    /* helper function */
    while (*mask && *mask!=';') mask++;
    if (*mask==';') {
      mask++;
      while (*mask==' ') mask++;       /* masks may be separated by "; " */
	  }
	}
  return 0;
}
BOOL WINAPI PathMatchSpecAW(LPVOID name, LPVOID mask) 
{	if (VERSION_OsIsUnicode())
	  return PathMatchSpecW( name, mask );
	return PathMatchSpecA( name, mask );
}
/*************************************************************************
 * PathSetDlgItemPathAW [SHELL32.48]
 * NOTES
 *  use PathCompactPath to make sure, the path fits into the control
 */

BOOL WINAPI PathSetDlgItemPathA(HWND hDlg, int id, LPCSTR pszPath) 
{	TRACE("%x %x %s\n",hDlg, id, pszPath);
	return SetDlgItemTextA(hDlg, id, pszPath);
}
BOOL WINAPI PathSetDlgItemPathW(HWND hDlg, int id, LPCWSTR pszPath) 
{	TRACE("%x %x %s\n",hDlg, id, debugstr_w(pszPath));
	return SetDlgItemTextW(hDlg, id, pszPath);
}
BOOL WINAPI PathSetDlgItemPathAW(HWND hDlg, int id, LPCVOID pszPath) 
{	if (VERSION_OsIsUnicode())
	  return PathSetDlgItemPathW(hDlg, id, pszPath);
	return PathSetDlgItemPathA(hDlg, id, pszPath);
}

/*************************************************************************
 * PathQualifyAW [SHELL32.49]
 */

BOOL WINAPI PathQualifyA(LPCSTR pszPath) 
{	FIXME("%s\n",pszPath);
	return 0;
}
BOOL WINAPI PathQualifyW(LPCWSTR pszPath) 
{	FIXME("%s\n",debugstr_w(pszPath));
	return 0;
}
BOOL WINAPI PathQualifyAW(LPCVOID pszPath) 
{	if (VERSION_OsIsUnicode())
	  return PathQualifyW(pszPath);
	return PathQualifyA(pszPath);
}

/*************************************************************************
 * PathResolve [SHELL32.51]
 */
DWORD WINAPI PathResolve(LPCSTR s,DWORD x2,DWORD x3) {
	FIXME("(%s,0x%08lx,0x%08lx),stub!\n",s,x2,x3);
	return 0;
}

/*************************************************************************
 * PathGetArgs [SHELL32.52]
 *
 * NOTES
 *     look for next arg in string. handle "quoted" strings
 *     returns pointer to argument *AFTER* the space. Or to the \0.
 */
LPCSTR WINAPI PathGetArgsA(LPCSTR cmdline) 
{	BOOL	qflag = FALSE;

	TRACE("%s\n",cmdline);

	while (*cmdline) 
	{ if ((*cmdline==' ') && !qflag)
	    return cmdline+1;
	  if (*cmdline=='"')
	    qflag=!qflag;
	  cmdline++;
	}
	return cmdline;

}
LPCWSTR WINAPI PathGetArgsW(LPCWSTR cmdline) 
{	BOOL	qflag = FALSE;

	TRACE("%s\n",debugstr_w(cmdline));

	while (*cmdline) 
	{ if ((*cmdline==' ') && !qflag)
	    return cmdline+1;
	  if (*cmdline=='"')
	    qflag=!qflag;
	  cmdline++;
	}
	return cmdline;
}
LPCVOID WINAPI PathGetArgsAW(LPVOID cmdline) 
{	if (VERSION_OsIsUnicode())
	  return PathGetArgsW(cmdline);
	return PathGetArgsA(cmdline);
}
/*************************************************************************
 * PathQuoteSpaces [SHELL32.55]
 * 
 * NOTES
 *     basename(char *fn);
 */
LPSTR WINAPI PathQuoteSpacesA(LPCSTR aptr)
{	FIXME("%s\n",aptr);
	return 0;

}
LPWSTR WINAPI PathQuoteSpacesW(LPCWSTR wptr)
{	FIXME("%s\n",debugstr_w(wptr));
	return 0;	
}
LPVOID WINAPI PathQuoteSpacesAW (LPCVOID fn)
{	if(VERSION_OsIsUnicode())
	  return PathQuoteSpacesW(fn);
	return PathQuoteSpacesA(fn);
}


/*************************************************************************
 * PathUnquoteSpaces [SHELL32.56]
 * 
 * NOTES
 *     unquote string (remove ")
 */
VOID WINAPI PathUnquoteSpacesA(LPSTR str) 
{	DWORD      len = lstrlenA(str);
	TRACE("%s\n",str);
	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	lstrcpyA(str,str+1);
	return;
}
VOID WINAPI PathUnquoteSpacesW(LPWSTR str) 
{	DWORD len = CRTDLL_wcslen(str);

	TRACE("%s\n",debugstr_w(str));

	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	CRTDLL_wcscpy(str,str+1);
	return;
}
VOID WINAPI PathUnquoteSpacesAW(LPVOID str) 
{
	if(VERSION_OsIsUnicode())
	  PathUnquoteSpacesW(str);
	else
	  PathUnquoteSpacesA(str);
}


/*************************************************************************
 * PathGetDriveNumber32 [SHELL32.57]
 *
 */
HRESULT WINAPI PathGetDriveNumber(LPSTR u)
{	FIXME("%s stub\n",debugstr_a(u));
	return 0;
}

/*************************************************************************
 * PathYetAnotherMakeUniqueName [SHELL32.75]
 * 
 * NOTES
 *     exported by ordinal
 */
BOOL WINAPI PathYetAnotherMakeUniqueNameA(LPDWORD x,LPDWORD y) {
    FIXME("(%p,%p):stub.\n",x,y);
    return TRUE;
}

/*************************************************************************
 * IsLFNDrive [SHELL32.119]
 * 
 * NOTES
 *     exported by ordinal Name
 */
BOOL WINAPI IsLFNDriveA(LPCSTR path) {
    DWORD	fnlen;

    if (!GetVolumeInformationA(path,NULL,0,NULL,&fnlen,NULL,NULL,0))
	return FALSE;
    return fnlen>12;
}
/*************************************************************************
 * PathFindOnPath [SHELL32.145]
 */
BOOL WINAPI PathFindOnPathA(LPSTR sFile, LPCSTR sOtherDirs)
{	FIXME("%s %s\n",sFile, sOtherDirs);
	return FALSE;
}
BOOL WINAPI PathFindOnPathW(LPWSTR sFile, LPCWSTR sOtherDirs)
{	FIXME("%s %s\n",debugstr_w(sFile), debugstr_w(sOtherDirs));
	return FALSE;
}
BOOL WINAPI PathFindOnPathAW(LPVOID sFile, LPCVOID sOtherDirs)
{	if (VERSION_OsIsUnicode())
	  return PathFindOnPathW(sFile, sOtherDirs);
	return PathFindOnPathA(sFile, sOtherDirs);
}

/*************************************************************************
 * PathGetExtension [SHELL32.158]
 *
 * NOTES
 *     exported by ordinal
 */
LPCSTR WINAPI PathGetExtensionA(LPCSTR path,DWORD y,DWORD z)
{	TRACE("(%s,%08lx,%08lx)\n",path,y,z);
	path = PathFindExtensionA(path);
	return *path?(path+1):path;
}
LPCWSTR WINAPI PathGetExtensionW(LPCWSTR path,DWORD y,DWORD z)
{	TRACE("(%s, %08lx, %08lx)\n",debugstr_w(path),y,z);
	path = PathFindExtensionW(path);
	return *path?(path+1):path;
}
LPCVOID WINAPI PathGetExtensionAW(LPCVOID path,DWORD y,DWORD z) 
{	if (VERSION_OsIsUnicode())
	  return PathGetExtensionW(path,y,z);
	return PathGetExtensionA(path,y,z);
}

/*************************************************************************
 * PathCleanupSpec				[SHELL32.171]
 *
 */
DWORD WINAPI PathCleanupSpecA(LPSTR x, LPSTR y)
{
	FIXME("(%p %s, %p %s) stub\n",x,debugstr_a(x),y,debugstr_a(y));
	return TRUE;
}

DWORD WINAPI PathCleanupSpecW(LPWSTR x, LPWSTR y)
{
	FIXME("(%p %s, %p %s) stub\n",x,debugstr_w(x),y,debugstr_w(y));
	return TRUE;
}

DWORD WINAPI PathCleanupSpecAW (LPVOID x, LPVOID y)
{
	if (VERSION_OsIsUnicode())
	  return PathCleanupSpecW(x,y);
	return PathCleanupSpecA(x,y);
}

/*************************************************************************
 * SheGetDirW [SHELL32.281]
 *
 */
HRESULT WINAPI SheGetDirW(LPWSTR u, LPWSTR v)
{	FIXME("%p %p stub\n",u,v);
	return 0;
}

/*************************************************************************
 * SheChangeDirW [SHELL32.274]
 *
 */
HRESULT WINAPI SheChangeDirW(LPWSTR u)
{	FIXME("(%s),stub\n",debugstr_w(u));
	return 0;
}

/*************************************************************************
*	PathProcessCommand	[SHELL32.653]
*/
HRESULT WINAPI PathProcessCommandA (LPSTR lpCommand, LPSTR v, DWORD w, DWORD x)
{
	FIXME("%p(%s) %p 0x%04lx 0x%04lx stub\n",
	lpCommand, lpCommand, v, w,x );
	lstrcpyA(v, lpCommand);
	return 0;
}

HRESULT WINAPI PathProcessCommandW (LPWSTR lpCommand, LPSTR v, DWORD w, DWORD x)
{
	FIXME("(%p %s, %p, 0x%04lx, 0x%04lx) stub\n",
	lpCommand, debugstr_w(lpCommand), v, w,x );
	return 0;
}

HRESULT WINAPI PathProcessCommandAW (LPVOID lpCommand, LPSTR v, DWORD w, DWORD x)
{
	if (VERSION_OsIsUnicode())
	  return PathProcessCommandW(lpCommand, v, w, x);
	return PathProcessCommandA(lpCommand, v, w, x);
}

/*************************************************************************
 * SHGetSpecialFolderPath [SHELL32.175]
 * 
 * converts csidl to path
 * 
 */
 
static char * szSHFolders = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders";

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

	if (RegCreateKeyExA(hRootKey,szSHFolders,0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hKey,&dwDisp))
	{
	  return FALSE;
	}

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
	    strcpy(szPath, szDefaultPath);
	  }
	  if (bCreate)
	  {
	    CreateDirectoryA(szPath,NULL);
	  }
	  RegSetValueExA(hKey,szValueName,0,REG_SZ,(LPBYTE)szPath,strlen(szPath)+1);
	}
	RegCloseKey(hKey);

	return TRUE;
}
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

/* PathRemoveBackslash
 *
 * If the path ends in a backslash it is replaced by a NULL
 * and the address of the NULL is returned
 * Otherwise 
 * the address of the last character is returned.
 *
 */

LPSTR WINAPI PathRemoveBackslashA( LPSTR lpPath )
{
	LPSTR p = lpPath;
	
	while (*lpPath) p = lpPath++;
	if ( *p == (CHAR)'\\') *p = (CHAR)'\0';
	return p;
}

LPWSTR WINAPI PathRemoveBackslashW( LPWSTR lpPath )
{
	LPWSTR p = lpPath;
	
	while (*lpPath); p = lpPath++;
	if ( *p == (WCHAR)'\\') *p = (WCHAR)'\0';
	return p;
}

/*
   shlwapi functions that have found their way in because most of
   shlwapi is unimplemented and doesn't have a home.  

   FIXME: move to a more appropriate file( when one exists )
*/
   
 /* SHGetValue: Gets a value from the registry */


BOOL WINAPI PathIsURLA(LPCSTR lpstrPath)
{
  LPSTR lpstrRes;
  char lpstrFileType[10] = "";
  int iSize;
  int i = 0;
  /* sanity check */
  if(!lpstrPath)
    return FALSE;

  /* get protocol        */
  /* protocol://location */
  if(!(lpstrRes = strchr(lpstrPath,':')))
  {
    return FALSE;
  }
  iSize = lpstrRes - lpstrPath;
  if(iSize > sizeof(lpstrFileType))
    return FALSE;

  strncpy(lpstrFileType,lpstrPath,iSize);

  while(strlen(SupportedProtocol[i]))
  {
    if(!_stricmp(lpstrFileType,SupportedProtocol[i++]))
      return TRUE;
  }

  return FALSE;
}  

DWORD WINAPI SHGetValueA(
    HKEY     hkey,
    LPCSTR   pSubKey,
    LPCSTR   pValue,
    LPDWORD  pwType,
    LPVOID   pvData,
    LPDWORD  pbData
    )
{
    FIXME("(%p),stub!\n", pSubKey);

	return ERROR_SUCCESS;  /* return success */
}

DWORD WINAPI SHGetValueW(
    HKEY     hkey,
    LPCWSTR  pSubKey,
    LPCWSTR  pValue,
    LPDWORD  pwType,
    LPVOID   pvData,
    LPDWORD  pbData
    )
{
    FIXME("(%p),stub!\n", pSubKey);

	return ERROR_SUCCESS;  /* return success */
}

/* gets a user-specific registry value. */

LONG WINAPI SHRegGetUSValueA(
    LPCSTR   pSubKey,
    LPCSTR   pValue,
    LPDWORD  pwType,
    LPVOID   pvData,
    LPDWORD  pbData,
    BOOL     fIgnoreHKCU,
    LPVOID   pDefaultData,
    DWORD    wDefaultDataSize
    )
{
    FIXME("(%p),stub!\n", pSubKey);

	return ERROR_SUCCESS;  /* return success */
}

LONG WINAPI SHRegGetUSValueW(
    LPCWSTR  pSubKey,
    LPCWSTR  pValue,
    LPDWORD  pwType,
    LPVOID   pvData,
    LPDWORD  pbData,
    BOOL     flagIgnoreHKCU,
    LPVOID   pDefaultData,
    DWORD    wDefaultDataSize
    )
{
    FIXME("(%p),stub!\n", pSubKey);

	return ERROR_SUCCESS;  /* return success */
}
  
  
