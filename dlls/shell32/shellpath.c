/*
 * Path Functions
 *
 * Many of this functions are in SHLWAPI.DLL also
 *
 */
#include <string.h>
#include <ctype.h>
#include "windows.h"
#include "debug.h"
#include "winnls.h"
#include "winversion.h"

#include "shlobj.h"
#include "shell32_main.h"

/*************************************************************************
 * PathIsRoot [SHELL32.29]
 */
BOOL32 WINAPI PathIsRoot32A(LPCSTR x)
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
BOOL32 WINAPI PathIsRoot32W(LPCWSTR x) 
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
BOOL32 WINAPI PathIsRoot32AW(LPCVOID x) 
{	if (VERSION_OsIsUnicode())
	  return PathIsRoot32W(x);
	return PathIsRoot32A(x);

}
/*************************************************************************
 * PathBuildRoot [SHELL32.30]
 */
LPSTR WINAPI PathBuildRoot32A(LPSTR root,BYTE drive) {
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
LPCSTR WINAPI PathFindExtension32A(LPCSTR path) 
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
LPCWSTR WINAPI PathFindExtension32W(LPCWSTR path) 
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
LPCVOID WINAPI PathFindExtension32AW(LPCVOID path) 
{	if (VERSION_OsIsUnicode())
	  return PathFindExtension32W(path);
	return PathFindExtension32A(path);

}

/*************************************************************************
 * PathAddBackslash [SHELL32.32]
 * 
 * NOTES
 *     append \ if there is none
 */
LPSTR WINAPI PathAddBackslash32A(LPSTR path)
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
LPWSTR WINAPI PathAddBackslash32W(LPWSTR path)
{	int len;
	TRACE(shell,"%p->%s\n",path,debugstr_w(path));

	len = lstrlen32W(path);
	if (len && path[len-1]!=(WCHAR)'\\') 
	{ path[len]  = (WCHAR)'\\';
	  path[len+1]= 0x00;
	  return path+len+1;
	}
	return path+len;
}
LPVOID WINAPI PathAddBackslash32AW(LPVOID path)
{	if(VERSION_OsIsUnicode())
	  return PathAddBackslash32W(path);
	return PathAddBackslash32A(path);
}

/*************************************************************************
 * PathRemoveBlanks [SHELL32.33]
 * 
 * NOTES
 *     remove spaces from beginning and end of passed string
 */
LPSTR WINAPI PathRemoveBlanks32A(LPSTR str)
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
LPWSTR WINAPI PathRemoveBlanks32W(LPWSTR str)
{	LPWSTR x = str;
	TRACE(shell,"%s\n",debugstr_w(str));
	while (*x==' ') x++;
	if (x!=str)
	  lstrcpy32W(str,x);
	if (!*str)
	  return str;
	x=str+lstrlen32W(str)-1;
	while (*x==' ')
	  x--;
	if (*x==' ')
	  *x='\0';
	return x;
}
LPVOID WINAPI PathRemoveBlanks32AW(LPVOID str)
{	if(VERSION_OsIsUnicode())
	  return PathRemoveBlanks32W(str);
	return PathRemoveBlanks32A(str);
}



/*************************************************************************
 * PathFindFilename [SHELL32.34]
 * 
 * NOTES
 *     basename(char *fn);
 */
LPCSTR WINAPI PathFindFilename32A(LPCSTR aptr)
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
LPCWSTR WINAPI PathFindFilename32W(LPCWSTR wptr)
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
LPCVOID WINAPI PathFindFilename32AW(LPCVOID fn)
{
	if(VERSION_OsIsUnicode())
	  return PathFindFilename32W(fn);
	return PathFindFilename32A(fn);
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
DWORD WINAPI PathRemoveFileSpec32A(LPSTR fn) {
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
LPSTR WINAPI PathAppend32A(LPSTR x1,LPSTR x2) {
  TRACE(shell,"%s %s\n",x1,x2);
  while (x2[0]=='\\') x2++;
  return PathCombine32A(x1,x1,x2);
}

/*************************************************************************
 * PathCombine [SHELL32.37]
 * 
 * NOTES
 *  if lpszFile='.' skip it
 *  szDest can be equal to lpszFile. Thats why we use sTemp
 */
LPSTR WINAPI PathCombine32A(LPSTR szDest, LPCSTR lpszDir, LPCSTR lpszFile) 
{	char sTemp[MAX_PATH];
	TRACE(shell,"%p %p->%s %p->%s\n",szDest, lpszDir, lpszDir, lpszFile, lpszFile);
	
	
	if (!lpszFile || !lpszFile[0] || (lpszFile[0]=='.' && !lpszFile[1]) ) 
	{ strcpy(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathIsRoot32A(lpszFile))
	{ strcpy(szDest,lpszFile);
	}
	else
	{ strcpy(sTemp,lpszDir);
	  PathAddBackslash32A(sTemp);
	  strcat(sTemp,lpszFile);
	  strcpy(szDest,sTemp);
	}
	return szDest;
}
LPWSTR WINAPI PathCombine32W(LPWSTR szDest, LPCWSTR lpszDir, LPCWSTR lpszFile) 
{	WCHAR sTemp[MAX_PATH];
	TRACE(shell,"%p %p->%s %p->%s\n",szDest, lpszDir, debugstr_w(lpszDir),
			 lpszFile, debugstr_w(lpszFile));
	
	
	if (!lpszFile || !lpszFile[0] || (lpszFile[0]==(WCHAR)'.' && !lpszFile[1]) ) 
	{ lstrcpy32W(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathIsRoot32W(lpszFile))
	{ lstrcpy32W(szDest,lpszFile);
	}
	else
	{ lstrcpy32W(sTemp,lpszDir);
	  PathAddBackslash32W(sTemp);
	  lstrcat32W(sTemp,lpszFile);
	  lstrcpy32W(szDest,sTemp);
	}
	return szDest;
}
LPVOID WINAPI PathCombine32AW(LPVOID szDest, LPCVOID lpszDir, LPCVOID lpszFile) 
{	if (VERSION_OsIsUnicode())
	  return PathCombine32W( szDest, lpszDir, lpszFile );
	return PathCombine32A( szDest, lpszDir, lpszFile );
}

/*************************************************************************
 * PathIsUNC [SHELL32.39]
 * 
 * NOTES
 *     PathIsUNC(char*path);
 */
BOOL32 WINAPI PathIsUNC32A(LPCSTR path) 
{	TRACE(shell,"%s\n",path);

	if ((path[0]=='\\') && (path[1]=='\\'))
	  return TRUE;
	return FALSE;
}
BOOL32 WINAPI PathIsUNC32W(LPCWSTR path) 
{	TRACE(shell,"%s\n",debugstr_w(path));

	if ((path[0]=='\\') && (path[1]=='\\'))
	  return TRUE;
	return FALSE;
}
BOOL32 WINAPI PathIsUNC32AW (LPCVOID path)
{	if (VERSION_OsIsUnicode())
	  return PathIsUNC32W( path );
	return PathIsUNC32A( path );  
}
/*************************************************************************
 *  PathIsRelativ [SHELL32.40]
 * 
 */
BOOL32 WINAPI PathIsRelative32A (LPCSTR path)
{	TRACE(shell,"path=%s\n",path);

	if (path && (path[0]!='\\' && path[1]==':'))
	  return TRUE;
	return FALSE;    
}
BOOL32 WINAPI PathIsRelative32W (LPCWSTR path)
{	TRACE(shell,"path=%s\n",debugstr_w(path));

	if (path && (path[0]!='\\' && path[1]==':'))
	  return TRUE;
	return FALSE;    
}
BOOL32 WINAPI PathIsRelative32AW (LPCVOID path)
{	if (VERSION_OsIsUnicode())
	  return PathIsRelative32W( path );
	return PathIsRelative32A( path );  
}
/*************************************************************************
 *  PathIsExe [SHELL32.43]
 * 
 */
BOOL32 WINAPI PathIsExe32A (LPCSTR path)
{	FIXME(shell,"path=%s\n",path);
	return FALSE;
}
BOOL32 WINAPI PathIsExe32W (LPCWSTR path)
{	FIXME(shell,"path=%s\n",debugstr_w(path));
	return FALSE;
}
BOOL32 WINAPI PathIsExe32AW (LPCVOID path)
{	if (VERSION_OsIsUnicode())
	  return PathIsExe32W (path);
	return PathIsExe32A(path);
}

/*************************************************************************
 * PathFileExists [SHELL32.45]
 * 
 * NOTES
 *     file_exists(char *fn);
 */
BOOL32 WINAPI PathFileExists32A(LPSTR fn) {
  TRACE(shell,"%s\n",fn);
   if (GetFileAttributes32A(fn)==-1)
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

BOOL32 WINAPI PathMatchSpec32A(LPCSTR name, LPCSTR mask) 
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
BOOL32 WINAPI PathMatchSpec32W(LPCWSTR name, LPCWSTR mask) 
{	WCHAR stemp[4];
	LPCWSTR _name;
	
	TRACE(shell,"%s %s stub\n",debugstr_w(name),debugstr_w(mask));

	lstrcpyAtoW(stemp,"*.*");	
	if (!lstrcmp32W( mask, stemp )) return 1;

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
BOOL32 WINAPI PathMatchSpec32AW(LPVOID name, LPVOID mask) 
{	if (VERSION_OsIsUnicode())
	  return PathMatchSpec32W( name, mask );
	return PathMatchSpec32A( name, mask );
}
/*************************************************************************
 * PathSetDlgItemPath32AW [SHELL32.48]
 * NOTES
 *  use PathCompactPath to make sure, the path fits into the control
 */

BOOL32 WINAPI PathSetDlgItemPath32A(HWND32 hDlg, int id, LPCSTR pszPath) 
{	TRACE(shell,"%x %x %s\n",hDlg, id, pszPath);
	return SetDlgItemText32A(hDlg, id, pszPath);
}
BOOL32 WINAPI PathSetDlgItemPath32W(HWND32 hDlg, int id, LPCWSTR pszPath) 
{	TRACE(shell,"%x %x %s\n",hDlg, id, debugstr_w(pszPath));
	return SetDlgItemText32W(hDlg, id, pszPath);
}
BOOL32 WINAPI PathSetDlgItemPath32AW(HWND32 hDlg, int id, LPCVOID pszPath) 
{	if (VERSION_OsIsUnicode())
	  return PathSetDlgItemPath32W(hDlg, id, pszPath);
	return PathSetDlgItemPath32A(hDlg, id, pszPath);
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
LPCSTR WINAPI PathGetArgs32A(LPCSTR cmdline) 
{	BOOL32	qflag = FALSE;

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
LPCWSTR WINAPI PathGetArgs32W(LPCWSTR cmdline) 
{	BOOL32	qflag = FALSE;

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
LPCVOID WINAPI PathGetArgs32AW(LPVOID cmdline) 
{	if (VERSION_OsIsUnicode())
	  return PathGetArgs32W(cmdline);
	return PathGetArgs32A(cmdline);
}
/*************************************************************************
 * PathQuoteSpaces [SHELL32.55]
 * 
 * NOTES
 *     basename(char *fn);
 */
LPSTR WINAPI PathQuoteSpaces32A(LPCSTR aptr)
{	FIXME(shell,"%s\n",aptr);
	return 0;

}
LPWSTR WINAPI PathQuoteSpaces32W(LPCWSTR wptr)
{	FIXME(shell,"L%s\n",debugstr_w(wptr));
	return 0;	
}
LPVOID WINAPI PathQuoteSpaces32AW (LPCVOID fn)
{	if(VERSION_OsIsUnicode())
	  return PathQuoteSpaces32W(fn);
	return PathQuoteSpaces32A(fn);
}


/*************************************************************************
 * PathUnquoteSpaces [SHELL32.56]
 * 
 * NOTES
 *     unquote string (remove ")
 */
VOID WINAPI PathUnquoteSpaces32A(LPSTR str) 
{	DWORD      len = lstrlen32A(str);
	TRACE(shell,"%s\n",str);
	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	lstrcpy32A(str,str+1);
	return;
}
VOID WINAPI PathUnquoteSpaces32W(LPWSTR str) 
{	DWORD len = lstrlen32W(str);

	TRACE(shell,"%s\n",debugstr_w(str));

	if (*str!='"')
	  return;
	if (str[len-1]!='"')
	  return;
	str[len-1]='\0';
	lstrcpy32W(str,str+1);
	return;
}
VOID WINAPI PathUnquoteSpaces32AW(LPVOID str) 
{	if(VERSION_OsIsUnicode())
	  PathUnquoteSpaces32W(str);
	PathUnquoteSpaces32A(str);
}


/*************************************************************************
 * PathGetDriveNumber32 [SHELL32.57]
 *
 */
HRESULT WINAPI PathGetDriveNumber32(LPSTR u)
{	FIXME(shell,"%s stub\n",debugstr_a(u));
	return 0;
}

/*************************************************************************
 * PathYetAnotherMakeUniqueName [SHELL32.75]
 * 
 * NOTES
 *     exported by ordinal
 */
BOOL32 WINAPI PathYetAnotherMakeUniqueName32A(LPDWORD x,LPDWORD y) {
    FIXME(shell,"(%p,%p):stub.\n",x,y);
    return TRUE;
}

/*************************************************************************
 * IsLFNDrive [SHELL32.119]
 * 
 * NOTES
 *     exported by ordinal Name
 */
BOOL32 WINAPI IsLFNDrive32A(LPCSTR path) {
    DWORD	fnlen;

    if (!GetVolumeInformation32A(path,NULL,0,NULL,&fnlen,NULL,NULL,0))
	return FALSE;
    return fnlen>12;
}
/*************************************************************************
 * PathFindOnPath [SHELL32.145]
 */
BOOL32 WINAPI PathFindOnPath32A(LPSTR sFile, LPCSTR sOtherDirs)
{	FIXME(shell,"%s %s\n",sFile, sOtherDirs);
	return FALSE;
}
BOOL32 WINAPI PathFindOnPath32W(LPWSTR sFile, LPCWSTR sOtherDirs)
{	FIXME(shell,"%s %s\n",debugstr_w(sFile), debugstr_w(sOtherDirs));
	return FALSE;
}
BOOL32 WINAPI PathFindOnPath32AW(LPVOID sFile, LPCVOID sOtherDirs)
{	if (VERSION_OsIsUnicode())
	  return PathFindOnPath32W(sFile, sOtherDirs);
	return PathFindOnPath32A(sFile, sOtherDirs);
}

/*************************************************************************
 * PathGetExtension [SHELL32.158]
 *
 * NOTES
 *     exported by ordinal
 */
LPCSTR WINAPI PathGetExtension32A(LPCSTR path,DWORD y,DWORD z)
{	TRACE(shell,"(%s,%08lx,%08lx)\n",path,y,z);
	path = PathFindExtension32A(path);
	return *path?(path+1):path;
}
LPCWSTR WINAPI PathGetExtension32W(LPCWSTR path,DWORD y,DWORD z)
{	TRACE(shell,"(L%s,%08lx,%08lx)\n",debugstr_w(path),y,z);
	path = PathFindExtension32W(path);
	return *path?(path+1):path;
}
LPCVOID WINAPI PathGetExtension32AW(LPCVOID path,DWORD y,DWORD z) 
{	if (VERSION_OsIsUnicode())
	  return PathGetExtension32W(path,y,z);
	return PathGetExtension32A(path,y,z);
}

/*************************************************************************
 * SheGetDirW [SHELL32.281]
 *
 */
HRESULT WINAPI SheGetDir32W(LPWSTR u, LPWSTR v)
{	FIXME(shell,"%s %s stub\n",debugstr_w(u),debugstr_w(v) );
	return 0;
}

/*************************************************************************
 * SheChangeDirW [SHELL32.274]
 *
 */
HRESULT WINAPI SheChangeDir32W(LPWSTR u)
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
BOOL32 WINAPI SHGetSpecialFolderPath32A (DWORD x1,LPSTR szPath,DWORD csidl,DWORD x4) 
{	LPITEMIDLIST pidl;

	WARN(shell,"(0x%04lx,%p,csidl=%lu,0x%04lx) semi-stub\n", x1,szPath,csidl,x4);

	SHGetSpecialFolderLocation(0, csidl, &pidl);
	SHGetPathFromIDList32A (pidl, szPath);
	SHFree (pidl);
	return TRUE;
}
BOOL32 WINAPI SHGetSpecialFolderPath32W (DWORD x1,LPWSTR szPath, DWORD csidl,DWORD x4) 
{	LPITEMIDLIST pidl;

	WARN(shell,"(0x%04lx,%p,csidl=%lu,0x%04lx) semi-stub\n", x1,szPath,csidl,x4);

	SHGetSpecialFolderLocation(0, csidl, &pidl);
	SHGetPathFromIDList32W (pidl, szPath);
	SHFree (pidl);
	return TRUE;
}
BOOL32 WINAPI SHGetSpecialFolderPath32 (DWORD x1,LPVOID szPath,DWORD csidl,DWORD x4) 
{	if (VERSION_OsIsUnicode())
	  return SHGetSpecialFolderPath32W ( x1, szPath, csidl, x4);
	return SHGetSpecialFolderPath32A ( x1, szPath, csidl, x4);
}
