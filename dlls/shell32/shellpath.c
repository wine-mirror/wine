/*
 * Path Functions
 *
 * Many of this functions are in SHLWAPI.DLL also
 *
 */
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "winnls.h"
#include "winversion.h"

#include "shlobj.h"
#include "shell32_main.h"

DEFAULT_DEBUG_CHANNEL(shell)

/*************************************************************************
 * PathIsRoot [SHELL32.29]
 */
BOOL WINAPI PathIsRootA(LPCSTR x)
{	TRACE(shell,"%s\n",x);
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
{	TRACE(shell,"%s\n",debugstr_w(x));
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
  TRACE(shell,"%p %i\n",root, drive);
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
	TRACE(shell,"%p %s\n",path,path);
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
	TRACE(shell,"%p L%s\n",path,debugstr_w(path));
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
	TRACE(shell,"%p->%s\n",path,path);

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
	TRACE(shell,"%p->%s\n",path,debugstr_w(path));

	len = lstrlenW(path);
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
	TRACE(shell,"%s\n",str);
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
	TRACE(shell,"%s\n",debugstr_w(str));
	while (*x==' ') x++;
	if (x!=str)
	  lstrcpyW(str,x);
	if (!*str)
	  return str;
	x=str+lstrlenW(str)-1;
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

	TRACE(shell,"%s\n",aslash);
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

	TRACE(shell,"L%s\n",debugstr_w(wslash));
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
  TRACE(shell,"%s\n",fn);
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
  TRACE(shell,"%s %s\n",x1,x2);
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
	TRACE(shell,"%p %p->%s %p->%s\n",szDest, lpszDir, lpszDir, lpszFile, lpszFile);
	
	
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
	TRACE(shell,"%p %p->%s %p->%s\n",szDest, lpszDir, debugstr_w(lpszDir),
			 lpszFile, debugstr_w(lpszFile));
	
	
	if (!lpszFile || !lpszFile[0] || (lpszFile[0]==(WCHAR)'.' && !lpszFile[1]) ) 
	{ lstrcpyW(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathIsRootW(lpszFile))
	{ lstrcpyW(szDest,lpszFile);
	}
	else
	{ lstrcpyW(sTemp,lpszDir);
	  PathAddBackslashW(sTemp);
	  lstrcatW(sTemp,lpszFile);
	  lstrcpyW(szDest,sTemp);
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
{	TRACE(shell,"%s\n",path);

	if ((path[0]=='\\') && (path[1]=='\\'))
	  return TRUE;
	return FALSE;
}
BOOL WINAPI PathIsUNCW(LPCWSTR path) 
{	TRACE(shell,"%s\n",debugstr_w(path));

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
{	TRACE(shell,"path=%s\n",path);

	if (path && (path[0]!='\\' && path[1]==':'))
	  return TRUE;
	return FALSE;    
}
BOOL WINAPI PathIsRelativeW (LPCWSTR path)
{	TRACE(shell,"path=%s\n",debugstr_w(path));

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
{	FIXME(shell,"path=%s\n",path);
	return FALSE;
}
BOOL WINAPI PathIsExeW (LPCWSTR path)
{	FIXME(shell,"path=%s\n",debugstr_w(path));
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
  TRACE(shell,"%s\n",fn);
   if (GetFileAttributesA(fn)==-1)
    	return FALSE;
    else
    	return TRUE;
}
/*************************************************************************
 * PathMatchSpec [SHELL32.46]
 * 
 * NOTES
 *     used from COMDLG32
 */

BOOL WINAPI PathMatchSpecA(LPCSTR name, LPCSTR mask) 
{	LPCSTR _name;

	TRACE(shell,"%s %s stub\n",name,mask);

	_name = name;
	while (*_name && *mask)
	{ if (*mask ==';')
	  { mask++;
	    _name = name;
	  }
	  else if (*mask == '*')
	  { mask++;
	    while (*mask == '*') mask++;		/* Skip consecutive '*' */
	    if (!*mask || *mask==';') return TRUE;	/* '*' matches everything */
	    while (*_name && (toupper(*_name) != toupper(*mask))) _name++;
	    if (!*_name)
	    { while ( *mask && *mask != ';') mask++;
	      _name = name;
	    }
	  }
	  else if ( (*mask == '?') || (toupper(*mask) == toupper(*_name)) )
	  { mask++;
	    _name++;
	  }
	  else
	  { while ( *mask && *mask != ';') mask++;
	  }
	}
	return (!*_name && (!*mask || *mask==';'));
}
BOOL WINAPI PathMatchSpecW(LPCWSTR name, LPCWSTR mask) 
{	WCHAR stemp[4];
	LPCWSTR _name;
	
	TRACE(shell,"%s %s stub\n",debugstr_w(name),debugstr_w(mask));

	lstrcpyAtoW(stemp,"*.*");	
	if (!lstrcmpW( mask, stemp )) return 1;

	_name = name;
	while (*_name && *mask)
	{ if (*mask ==';')
	  { mask++;
	    _name = name;
	  }
	  else if (*mask == '*')
	  { mask++;
	    while (*mask == '*') mask++;		/* Skip consecutive '*' */
	    if (!*mask || *mask==';') return TRUE;	/* '*' matches everything */
	    while (*_name && (towupper(*_name) != towupper(*mask))) _name++;
	    if (!*_name)
	    { while ( *mask && *mask != ';') mask++;
	      _name = name;
	    }
	  }
	  else if ( (*mask == '?') || (towupper(*mask) == towupper(*_name)) )
	  { mask++;
	    _name++;
	  }
	  else
	  { while ( *mask && *mask != ';') mask++;
	  }
	}
	return (!*_name && (!*mask || *mask==';'));
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
{	TRACE(shell,"%x %x %s\n",hDlg, id, pszPath);
	return SetDlgItemTextA(hDlg, id, pszPath);
}
BOOL WINAPI PathSetDlgItemPathW(HWND hDlg, int id, LPCWSTR pszPath) 
{	TRACE(shell,"%x %x %s\n",hDlg, id, debugstr_w(pszPath));
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
{	TRACE(shell,"%s\n",pszPath);
	return 0;
}
BOOL WINAPI PathQualifyW(LPCWSTR pszPath) 
{	TRACE(shell,"%s\n",debugstr_w(pszPath));
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
	FIXME(shell,"(%s,0x%08lx,0x%08lx),stub!\n",s,x2,x3);
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

	TRACE(shell,"%s\n",cmdline);

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

	TRACE(shell,"%sL\n",debugstr_w(cmdline));

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
{	FIXME(shell,"%s\n",aptr);
	return 0;

}
LPWSTR WINAPI PathQuoteSpacesW(LPCWSTR wptr)
{	FIXME(shell,"L%s\n",debugstr_w(wptr));
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
	TRACE(shell,"%s\n",str);
	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	lstrcpyA(str,str+1);
	return;
}
VOID WINAPI PathUnquoteSpacesW(LPWSTR str) 
{	DWORD len = lstrlenW(str);

	TRACE(shell,"%s\n",debugstr_w(str));

	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	lstrcpyW(str,str+1);
	return;
}
VOID WINAPI PathUnquoteSpacesAW(LPVOID str) 
{	if(VERSION_OsIsUnicode())
	  PathUnquoteSpacesW(str);
	PathUnquoteSpacesA(str);
}


/*************************************************************************
 * PathGetDriveNumber32 [SHELL32.57]
 *
 */
HRESULT WINAPI PathGetDriveNumber(LPSTR u)
{	FIXME(shell,"%s stub\n",debugstr_a(u));
	return 0;
}

/*************************************************************************
 * PathYetAnotherMakeUniqueName [SHELL32.75]
 * 
 * NOTES
 *     exported by ordinal
 */
BOOL WINAPI PathYetAnotherMakeUniqueNameA(LPDWORD x,LPDWORD y) {
    FIXME(shell,"(%p,%p):stub.\n",x,y);
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
{	FIXME(shell,"%s %s\n",sFile, sOtherDirs);
	return FALSE;
}
BOOL WINAPI PathFindOnPathW(LPWSTR sFile, LPCWSTR sOtherDirs)
{	FIXME(shell,"%s %s\n",debugstr_w(sFile), debugstr_w(sOtherDirs));
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
{	TRACE(shell,"(%s,%08lx,%08lx)\n",path,y,z);
	path = PathFindExtensionA(path);
	return *path?(path+1):path;
}
LPCWSTR WINAPI PathGetExtensionW(LPCWSTR path,DWORD y,DWORD z)
{	TRACE(shell,"(L%s,%08lx,%08lx)\n",debugstr_w(path),y,z);
	path = PathFindExtensionW(path);
	return *path?(path+1):path;
}
LPCVOID WINAPI PathGetExtensionAW(LPCVOID path,DWORD y,DWORD z) 
{	if (VERSION_OsIsUnicode())
	  return PathGetExtensionW(path,y,z);
	return PathGetExtensionA(path,y,z);
}

/*************************************************************************
 * SheGetDirW [SHELL32.281]
 *
 */
HRESULT WINAPI SheGetDirW(LPWSTR u, LPWSTR v)
{	FIXME(shell,"%p %p stub\n",u,v);
	return 0;
}

/*************************************************************************
 * SheChangeDirW [SHELL32.274]
 *
 */
HRESULT WINAPI SheChangeDirW(LPWSTR u)
{	FIXME(shell,"(%s),stub\n",debugstr_w(u));
	return 0;
}

/*************************************************************************
*	PathProcessCommand	[SHELL32.653]
*/
HRESULT WINAPI PathProcessCommand (DWORD u, DWORD v, DWORD w, DWORD x)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w,x);
	return 0;
}

/*************************************************************************
 * SHGetSpecialFolderPath [SHELL32.175]
 * 
 * converts csidl to path
 * 
 */
BOOL WINAPI SHGetSpecialFolderPathA (DWORD x1,LPSTR szPath,DWORD csidl,DWORD x4) 
{	LPITEMIDLIST pidl;

	WARN(shell,"(0x%04lx,%p,csidl=%lu,0x%04lx) semi-stub\n", x1,szPath,csidl,x4);

	SHGetSpecialFolderLocation(0, csidl, &pidl);
	SHGetPathFromIDListA (pidl, szPath);
	SHFree (pidl);
	return TRUE;
}
BOOL WINAPI SHGetSpecialFolderPathW (DWORD x1,LPWSTR szPath, DWORD csidl,DWORD x4) 
{	LPITEMIDLIST pidl;

	WARN(shell,"(0x%04lx,%p,csidl=%lu,0x%04lx) semi-stub\n", x1,szPath,csidl,x4);

	SHGetSpecialFolderLocation(0, csidl, &pidl);
	SHGetPathFromIDListW (pidl, szPath);
	SHFree (pidl);
	return TRUE;
}
BOOL WINAPI SHGetSpecialFolderPath (DWORD x1,LPVOID szPath,DWORD csidl,DWORD x4) 
{	if (VERSION_OsIsUnicode())
	  return SHGetSpecialFolderPathW ( x1, szPath, csidl, x4);
	return SHGetSpecialFolderPathA ( x1, szPath, csidl, x4);
}
