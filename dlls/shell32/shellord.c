/*
 * 				Shell Ordinal Functions
 *
 * These are completely undocumented. The meaning of the functions changes
 * between different OS versions (NT uses Unicode strings, 95 uses ASCII
 * strings, etc. etc.)
 * 
 * They are just here so that explorer.exe and iexplore.exe can be tested.
 *
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windows.h"
#include "winerror.h"
#include "file.h"
#include "shell.h"
#include "heap.h"
#include "module.h"
#include "neexe.h"
#include "resource.h"
#include "dlgs.h"
#include "win.h"
#include "graphics.h"
#include "cursoricon.h"
#include "interfaces.h"
#include "shlobj.h"
#include "debug.h"
#include "winreg.h"
#include "shell32_main.h"

/*************************************************************************
 * SHChangeNotifyRegister [SHELL32.2]
 * NOTES
 *   Idlist is an array of structures and Count specifies how many items in the array
 *   (usually just one I think).
 */
DWORD WINAPI
SHChangeNotifyRegister(
    HWND32 hwnd,
    LONG events1,
    LONG events2,
    DWORD msg,
    int count,
    IDSTRUCT *idlist)
{	FIXME(shell,"(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08x,%p):stub.\n",
		hwnd,events1,events2,msg,count,idlist);
	return 0;
}
/*************************************************************************
 * SHChangeNotifyDeregister [SHELL32.4]
 */
DWORD WINAPI
SHChangeNotifyDeregister(LONG x1,LONG x2)
{	FIXME(shell,"(0x%08lx,0x%08lx):stub.\n",x1,x2);
	return 0;
}

/*************************************************************************
 * PathIsRoot [SHELL32.29]
 */
BOOL32 WINAPI PathIsRoot(LPCSTR x) {
  TRACE(shell,"%s\n",x);
	if (!strcmp(x+1,":\\"))		/* "X:\" */
		return 1;
	if (!strcmp(x,"\\"))		/* "\" */
		return 1;
	if (x[0]=='\\' && x[1]=='\\') {		/* UNC "\\<xx>\" */
		int	foundbackslash = 0;
		x=x+2;
		while (*x) {
			if (*x++=='\\')
				foundbackslash++;
		}
		if (foundbackslash<=1)	/* max 1 \ more ... */
			return 1;
	}
	return 0;
}

/*************************************************************************
 * PathBuildRoot [SHELL32.30]
 */
LPSTR WINAPI PathBuildRoot(LPSTR root,BYTE drive) {
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
LPSTR WINAPI PathFindExtension(LPSTR path) {
  LPSTR   lastpoint = NULL;
  TRACE(shell,"%p %s\n",path,path);
    while (*path) {
	if (*path=='\\'||*path==' ')
	    lastpoint=NULL;
	if (*path=='.')
	    lastpoint=path;
	path++;
    }
    return lastpoint?lastpoint:path;
}

/*************************************************************************
 * PathAddBackslash [SHELL32.32]
 * 
 * NOTES
 *     append \ if there is none
 */
LPSTR WINAPI PathAddBackslash(LPSTR path)
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

/*************************************************************************
 * PathRemoveBlanks [SHELL32.33]
 * 
 * NOTES
 *     remove spaces from beginning and end of passed string
 */
LPSTR WINAPI PathRemoveBlanks(LPSTR str)
{ LPSTR x = str;
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


/*************************************************************************
 * PathFindFilename [SHELL32.34]
 * 
 * NOTES
 *     basename(char *fn);
 */
LPSTR WINAPI PathFindFilename(LPSTR fn)
{	LPSTR basefn;
	TRACE(shell,"%s\n",fn);
    basefn = fn;
    while (fn[0]) 
    { if (((fn[0]=='\\') || (fn[0]==':')) && fn[1] && fn[1]!='\\')
	  basefn = fn+1;
	  fn++;
    }
    return basefn;
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
DWORD WINAPI PathRemoveFileSpec(LPSTR fn) {
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
LPSTR WINAPI PathAppend(LPSTR x1,LPSTR x2) {
  TRACE(shell,"%s %s\n",x1,x2);
  while (x2[0]=='\\') x2++;
  return PathCombine(x1,x1,x2);
}

/*************************************************************************
 * PathCombine [SHELL32.37]
 * 
 * NOTES
 *  if lpszFile='.' skip it
 *  szDest can be equal to lpszFile. Thats why we use sTemp
 */
LPSTR WINAPI PathCombine(LPSTR szDest, LPCSTR lpszDir, LPCSTR lpszFile) 
{	char sTemp[MAX_PATH];
	TRACE(shell,"%p %p->%s %p->%s\n",szDest, lpszDir, lpszDir, lpszFile, lpszFile);

	if (!lpszFile || !lpszFile[0] || (lpszFile[0]=='.' && !lpszFile[1]) ) 
	{ strcpy(szDest,lpszDir);
	  return szDest;
	}

	/*  if lpszFile is a complete path don't care about lpszDir */
	if (PathIsRoot(lpszFile))
	{ strcpy(szDest,lpszFile);
	}
	strcpy(sTemp,lpszDir);
	PathAddBackslash(sTemp);
	strcat(sTemp,lpszFile);
	strcpy(szDest,sTemp);
	return szDest;
}

/*************************************************************************
 * PathIsUNC [SHELL32.39]
 * 
 * NOTES
 *     PathIsUNC(char*path);
 */
BOOL32 WINAPI PathIsUNC(LPCSTR path) {
  TRACE(shell,"%s\n",path);
	if ((path[0]=='\\') && (path[1]=='\\'))
		return TRUE;
	return FALSE;
}
/*************************************************************************
 *  PathIsExe [SHELL32.43]
 * 
 */
BOOL32 WINAPI PathIsExe (LPCSTR path)
{  TRACE(shell,"path=%s\n",path);
    return FALSE;
}

/*************************************************************************
 * PathFileExists [SHELL32.45]
 * 
 * NOTES
 *     file_exists(char *fn);
 */
BOOL32 WINAPI PathFileExists(LPSTR fn) {
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

BOOL32 WINAPI PathMatchSpec(LPSTR x, LPSTR y) 
{	FIXME(shell,"%s %s stub\n",x,y);
	return TRUE;
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
LPSTR WINAPI PathGetArgs(LPSTR cmdline) {
    BOOL32	qflag = FALSE;
  TRACE(shell,"%s\n",cmdline);
    while (*cmdline) {
    	if ((*cmdline==' ') && !qflag)
		return cmdline+1;
	if (*cmdline=='"')
		qflag=!qflag;
	cmdline++;
    }
    return cmdline;
}

/*************************************************************************
 * PathUnquoteSpaces [SHELL32.56]
 * 
 * NOTES
 *     unquote string (remove ")
 */
VOID WINAPI PathUnquoteSpaces(LPSTR str) {
    DWORD      len = lstrlen32A(str);
    TRACE(shell,"%s\n",str);
    if (*str!='"') return;
    if (str[len-1]!='"') return;
    str[len-1]='\0';
    lstrcpy32A(str,str+1);
    return;
}

/*************************************************************************
 * ParseField [SHELL32.58]
 *
 */
DWORD WINAPI ParseField(LPCSTR src,DWORD x2,LPSTR target,DWORD pathlen) {
	FIXME(shell,"(%s,0x%08lx,%p,%ld):stub.\n",
		src,x2,target,pathlen
	);
	if (!src)
		return 0;
	return 0;
}

/*************************************************************************
 * PickIconDlg [SHELL32.62]
 * 
 */
DWORD WINAPI PickIconDlg(DWORD x,DWORD y,DWORD z,DWORD a) {
	FIXME(shell,"(%08lx,%08lx,%08lx,%08lx):stub.\n",x,y,z,a);
	return 0xffffffff;
}

/*************************************************************************
 * GetFileNameFromBrowse [SHELL32.63]
 * 
 */
DWORD WINAPI GetFileNameFromBrowse(HWND32 howner, LPSTR targetbuf, DWORD len, DWORD x, LPCSTR suffix, LPCSTR y, LPCSTR cmd) {
    FIXME(shell,"(%04x,%p,%ld,%08lx,%s,%s,%s):stub.\n",
	    howner,targetbuf,len,x,suffix,y,cmd
    );
    /* puts up a Open Dialog and requests input into targetbuf */
    /* OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST|OFN_unknown */
    lstrcpy32A(targetbuf,"x:\\s3.exe");
    return 1;
}

/*************************************************************************
 * SHGetSettings [SHELL32.68]
 * 
 */
DWORD WINAPI SHGetSettings(DWORD x,DWORD y,DWORD z) {
	FIXME(shell,"(0x%08lx,0x%08lx,0x%08lx):stub.\n",
		x,y,z
	);
	return 0;
}

/*************************************************************************
 * Shell_GetCachedImageIndex [SHELL32.72]
 *
 */
void WINAPI Shell_GetCachedImageIndex(LPSTR x,DWORD y,DWORD z) 
{ FIXME(shell,"(%s,%08lx,%08lx):stub.\n",x,y,z);
}

/*************************************************************************
 * SHShellFolderView_Message [SHELL32.73]
 *
 * PARAMETERS
 *  hwndCabinet defines the explorer cabinet window that contains the 
 *              shellview you need to communicate with
 *  uMsg        identifying the SFVM enum to perform
 *  lParam
 *
 * NOTES
 *  Message SFVM_REARRANGE = 1
 *    This message gets sent when a column gets clicked to instruct the
 *    shell view to re-sort the item list. lParam identifies the column
 *    that was clicked.
 */
int WINAPI SHShellFolderView_Message(HWND32 hwndCabinet,UINT32 uMsg,LPARAM lParam)
{ FIXME(shell,"%04x %08ux %08lx stub\n",hwndCabinet,uMsg,lParam);
  return 0;
}

/*************************************************************************
 * PathYetAnotherMakeUniqueName [SHELL32.75]
 * 
 * NOTES
 *     exported by ordinal
 */
BOOL32 WINAPI PathYetAnotherMakeUniqueName(LPDWORD x,LPDWORD y) {
    FIXME(shell,"(%p,%p):stub.\n",x,y);
    return TRUE;
}

/*************************************************************************
 * SHMapPIDLToSystemImageListIndex [SHELL32.77]
 *
 * PARAMETERS
 * x  pointer to an instance of IShellFolder 
 * 
 * NOTES
 *     exported by ordinal
 *
 */
DWORD WINAPI
SHMapPIDLToSystemImageListIndex(LPSHELLFOLDER sh,DWORD y,DWORD z)
{ FIXME(shell,"(SF=%p,pidl=%08lx,%08lx):stub.\n",sh,y,z);
  return 0;
}

/*************************************************************************
 * OleStrToStrN  [SHELL32.78]
 * 
 * NOTES
 *     exported by ordinal
 * FIXME
 *  wrong implemented OleStr is NOT wide string !!!! (jsch)
 */
BOOL32 WINAPI
OleStrToStrN (LPSTR lpMulti, INT32 nMulti, LPCWSTR lpWide, INT32 nWide) {
    return WideCharToMultiByte (0, 0, lpWide, nWide,
        lpMulti, nMulti, NULL, NULL);
}

/*************************************************************************
 * StrToOleStrN  [SHELL32.79]
 *
 * NOTES
 *     exported by ordinal
 * FIXME
 *  wrong implemented OleStr is NOT wide string !!!! (jsch)
*/
BOOL32 WINAPI
StrToOleStrN (LPWSTR lpWide, INT32 nWide, LPCSTR lpMulti, INT32 nMulti) {
    return MultiByteToWideChar (0, 0, lpMulti, nMulti, lpWide, nWide);
}

/*************************************************************************
 * SHCloneSpecialIDList [SHELL32.89]
 * 
 * PARAMETERS
 *  hwndOwner 	[in] 
 *  nFolder 	[in]	CSIDL_xxxxx ??
 *
 * RETURNS
 *  pidl ??
 * NOTES
 *     exported by ordinal
 */
LPITEMIDLIST WINAPI SHCloneSpecialIDList(HWND32 hwndOwner,DWORD nFolder,DWORD x3)
{	LPITEMIDLIST ppidl;
	WARN(shell,"(hwnd=0x%x,csidl=0x%lx,0x%lx):semi-stub.\n",
					 hwndOwner,nFolder,x3);

	SHGetSpecialFolderLocation(hwndOwner, nFolder, &ppidl);

	return ppidl;
}

/*************************************************************************
 * IsLFNDrive [SHELL32.119]
 * 
 * NOTES
 *     exported by ordinal Name
 */
BOOL32 WINAPI IsLFNDrive(LPCSTR path) {
    DWORD	fnlen;

    if (!GetVolumeInformation32A(path,NULL,0,NULL,&fnlen,NULL,NULL,0))
	return FALSE;
    return fnlen>12;
}

/*************************************************************************
 * SHGetSpecialFolderPath [SHELL32.175]
 * 
 * NOTES
 *     exported by ordinal
 */
void WINAPI SHGetSpecialFolderPath(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
    FIXME(shell,"(0x%04lx,0x%04lx,csidl=0x%04lx,0x%04lx):stub.\n",
      x1,x2,x3,x4
    );
}

/*************************************************************************
 * RegisterShellHook [SHELL32.181]
 *
 * PARAMS
 *      hwnd [I]  window handle
 *      y    [I]  flag ????
 * 
 * NOTES
 *     exported by ordinal
 */
void WINAPI RegisterShellHook32(HWND32 hwnd, DWORD y) {
    FIXME(shell,"(0x%08x,0x%08lx):stub.\n",hwnd,y);
}

/*************************************************************************
 * ShellMessageBoxA [SHELL32.183]
 *
 * Format and output errormessage.
 *
 * NOTES
 *     exported by ordinal
 */
void __cdecl
ShellMessageBoxA(HMODULE32 hmod,HWND32 hwnd,DWORD id,DWORD x,DWORD type,LPVOID arglist) {
	char	buf[100],buf2[100]/*,*buf3*/;
/*	LPVOID	args = &arglist;*/

	if (!LoadString32A(hmod,x,buf,100))
		strcpy(buf,"Desktop");
//	LoadString32A(hmod,id,buf2,100);
	/* FIXME: the varargs handling doesn't. */
//	FormatMessage32A(0x500,buf2,0,0,(LPSTR)&buf3,256,(LPDWORD)&args);

	FIXME(shell,"(%08lx,%08lx,%08lx(%s),%08lx(%s),%08lx,%p):stub.\n",
		(DWORD)hmod,(DWORD)hwnd,id,buf2,x,buf,type,arglist
	);
	/*MessageBox32A(hwnd,buf3,buf,id|0x10000);*/
}

/*************************************************************************
 * SHRestricted [SHELL32.100]
 *
 * walks through policy table, queries <app> key, <type> value, returns 
 * queried (DWORD) value.
 * {0x00001,Explorer,NoRun}
 * {0x00002,Explorer,NoClose}
 * {0x00004,Explorer,NoSaveSettings}
 * {0x00008,Explorer,NoFileMenu}
 * {0x00010,Explorer,NoSetFolders}
 * {0x00020,Explorer,NoSetTaskbar}
 * {0x00040,Explorer,NoDesktop}
 * {0x00080,Explorer,NoFind}
 * {0x00100,Explorer,NoDrives}
 * {0x00200,Explorer,NoDriveAutoRun}
 * {0x00400,Explorer,NoDriveTypeAutoRun}
 * {0x00800,Explorer,NoNetHood}
 * {0x01000,Explorer,NoStartBanner}
 * {0x02000,Explorer,RestrictRun}
 * {0x04000,Explorer,NoPrinterTabs}
 * {0x08000,Explorer,NoDeletePrinter}
 * {0x10000,Explorer,NoAddPrinter}
 * {0x20000,Explorer,NoStartMenuSubFolders}
 * {0x40000,Explorer,MyDocsOnNet}
 * {0x80000,WinOldApp,NoRealMode}
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRestricted (DWORD pol) {
	HKEY	xhkey;

	FIXME(shell,"(%08lx):stub.\n",pol);
	if (RegOpenKey32A(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Policies",&xhkey))
		return 0;
	/* FIXME: do nothing for now, just return 0 (== "allowed") */
	RegCloseKey(xhkey);
	return 0;
}

/*************************************************************************
 * PathGetExtension [SHELL32.158]
 *
 * NOTES
 *     exported by ordinal
 */
LPSTR WINAPI PathGetExtension(LPSTR path,DWORD y,DWORD z)
{ TRACE(shell,"(%s,%08lx,%08lx)\n",path,y,z);
    path = PathFindExtension(path);
    return *path?(path+1):path;
}

/*************************************************************************
 * SHCreateDirectory [SHELL32.165]
 *
 * NOTES
 *  exported by ordinal
 *  not sure about LPSECURITY_ATTRIBUTES
 */
DWORD WINAPI SHCreateDirectory(LPSECURITY_ATTRIBUTES sec,LPCSTR path) {
	TRACE(shell,"(%p,%s):stub.\n",sec,path);
	if (CreateDirectory32A(path,sec))
		return TRUE;
	/* SHChangeNotify(8,1,path,0); */
	return FALSE;
#if 0
	if (SHELL32_79(path,(LPVOID)x))
		return 0;
	FIXME(shell,"(%08lx,%s):stub.\n",x,path);
	return 0;
#endif
}

/*************************************************************************
 * SHFree [SHELL32.195]
 *
 * NOTES
 *     free_ptr() - frees memory using IMalloc
 *     exported by ordinal
 */
DWORD WINAPI SHFree(LPVOID x) {
  TRACE(shell,"%p\n",x);
  /*return LocalFree32((HANDLE32)x);*/ /* crashes */
  return HeapFree(GetProcessHeap(),0,x);
}

/*************************************************************************
 * SHAlloc [SHELL32.196]
 *
 * NOTES
 *     void *task_alloc(DWORD len), uses SHMalloc allocator
 *     exported by ordinal
 */
LPVOID WINAPI SHAlloc(DWORD len) {
 /* void * ret = (LPVOID)LocalAlloc32(len,LMEM_ZEROINIT);*/ /* chrashes */
 void * ret = (LPVOID) HeapAlloc(GetProcessHeap(),0,len);
  TRACE(shell,"%lu bytes at %p\n",len, ret);
  return ret;
}

/*************************************************************************
 * OpenRegStream [SHELL32.85]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI OpenRegStream(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
    FIXME(shell,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx):stub.\n",
    	x1,x2,x3,x4
    );
    return 0;
}

/*************************************************************************
 * SHRegisterDragDrop [SHELL32.86]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRegisterDragDrop(HWND32 hwnd,DWORD x2) {
    FIXME (shell, "(0x%08x,0x%08lx):stub.\n", hwnd, x2);
    return 0;
}

/*************************************************************************
 * SHRevokeDragDrop [SHELL32.87]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHRevokeDragDrop(DWORD x) {
    FIXME(shell,"(0x%08lx):stub.\n",x);
    return 0;
}

/*************************************************************************
 * RunFileDlg [SHELL32.61]
 *
 * NOTES
 *     Original name: RunFileDlg (exported by ordinal)
 */
DWORD WINAPI
RunFileDlg (HWND32 hwndOwner, DWORD dwParam1, DWORD dwParam2,
	    LPSTR lpszTitle, LPSTR lpszPrompt, UINT32 uFlags)
{
    FIXME (shell,"(0x%08x 0x%lx 0x%lx \"%s\" \"%s\" 0x%x):stub.\n",
	   hwndOwner, dwParam1, dwParam2, lpszTitle, lpszPrompt, uFlags);
    return 0;
}

/*************************************************************************
 * ExitWindowsDialog [SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI
ExitWindowsDialog (HWND32 hwndOwner)
{
    FIXME (shell,"(0x%08x):stub.\n", hwndOwner);
    return 0;
}

/*************************************************************************
 * ArrangeWindows [SHELL32.184]
 * 
 */
DWORD WINAPI
ArrangeWindows (DWORD dwParam1, DWORD dwParam2, DWORD dwParam3,
		DWORD dwParam4, DWORD dwParam5)
{
    FIXME (shell,"(0x%lx 0x%lx 0x%lx 0x%lx 0x%lx):stub.\n",
	   dwParam1, dwParam2, dwParam3, dwParam4, dwParam5);
    return 0;
}

/*************************************************************************
 * SHCLSIDFromString [SHELL32.147]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI
SHCLSIDFromString (DWORD dwParam1, DWORD dwParam2)
{
    FIXME (shell,"(0x%lx 0x%lx):stub.\n", dwParam1, dwParam2);
    FIXME (shell,"(\"%s\" \"%s\"):stub.\n", (LPSTR)dwParam1, (LPSTR)dwParam2);

    return 0;
}


/*************************************************************************
 * SignalFileOpen [SHELL32.103]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI
SignalFileOpen (DWORD dwParam1)
{
    FIXME (shell,"(0x%08lx):stub.\n", dwParam1);

    return 0;
}

/*************************************************************************
 * SHAddToRecentDocs [SHELL32.234]
 *
 * PARAMETERS
 *   uFlags  [IN] SHARD_PATH or SHARD_PIDL
 *   pv      [IN] string or pidl, NULL clears the list
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHAddToRecentDocs32 (UINT32 uFlags,LPCVOID pv)   
{ if (SHARD_PIDL==uFlags)
  { FIXME (shell,"(0x%08x,pidl=%p):stub.\n", uFlags,pv);
	}
	else
	{ FIXME (shell,"(0x%08x,%s):stub.\n", uFlags,(char*)pv);
	}
  return 0;
}
/*************************************************************************
 * SHFileOperation32 [SHELL32.242]
 *
 */
DWORD WINAPI SHFileOperation32(DWORD x)
{	FIXME(shell,"0x%08lx stub\n",x);
	return 0;

}

/*************************************************************************
 * SHFileOperation32A [SHELL32.243]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperation32A (LPSHFILEOPSTRUCT32A lpFileOp)   
{ FIXME (shell,"(%p):stub.\n", lpFileOp);
  return 1;
}
/*************************************************************************
 * SHFileOperation32W [SHELL32.244]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperation32W (LPSHFILEOPSTRUCT32W lpFileOp)   
{ FIXME (shell,"(%p):stub.\n", lpFileOp);
  return 1;
}

/*************************************************************************
 * SHChangeNotify [SHELL32.239]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHChangeNotify32 (
    INT32   wEventId,  /* [IN] flags that specifies the event*/
    UINT32  uFlags,   /* [IN] the meaning of dwItem[1|2]*/
		LPCVOID dwItem1,
		LPCVOID dwItem2)
{ FIXME (shell,"(0x%08x,0x%08ux,%p,%p):stub.\n", wEventId,uFlags,dwItem1,dwItem2);
  return 0;
}
/*************************************************************************
 * SHCreateShellFolderViewEx [SHELL32.174]
 *
 * NOTES
 *  see IShellFolder::CreateViewObject
 */
HRESULT WINAPI SHCreateShellFolderViewEx32(
  LPSHELLVIEWDATA psvcbi, /*[in ] shelltemplate struct*/
  LPVOID* ppv)            /*[out] IShellView pointer*/
{ FIXME (shell,"(%p,%p):stub.\n", psvcbi,ppv);
  return 0;
}
/*************************************************************************
 * SHFind_InitMenuPopup [SHELL32.149]
 *
 * NOTES
 *  Registers the menu behind the "Start" button
 *
 * PARAMETERS
 *  hMenu		[in] handel of menu previously created
 *  hWndParent	[in] parent window
 *  w			[in] no pointer
 *  x			[in] no pointer
 */
HRESULT WINAPI SHFind_InitMenuPopup (HMENU32 hMenu, HWND32 hWndParent, DWORD w, DWORD x)
{	FIXME(shell,"hmenu=0x%08x hwnd=0x%08x 0x%08lx 0x%08lx stub\n",
		hMenu,hWndParent,w,x);
	return 0;
}
/*************************************************************************
 * FileMenu_InitMenuPopup [SHELL32.109]
 *
 */
HRESULT WINAPI FileMenu_InitMenuPopup (DWORD hmenu)
{	FIXME(shell,"hmenu=0x%lx stub\n",hmenu);
	return 0;
}
/*************************************************************************
 * FileMenu_Create [SHELL32.114]
 *
 * w retval from LoadBitmapA
 *
 *
 */
HRESULT WINAPI FileMenu_Create (DWORD u, DWORD v, DWORD w, DWORD x, DWORD z)
{ FIXME(shell,"0x%08lx 0x%08lx hbmp=0x%lx 0x%08lx 0x%08lx stub\n",u,v,w,x,z);
  return 0;
}
/*************************************************************************
 * FileMenu_TrackPopupMenuEx [SHELL32.116]
 *
 * PARAMETERS
 *  uFlags		[in]	according to TrackPopupMenuEx
 *  posX		[in]
 *  posY		[in]
 *  hWndParent		[in]
 *  z	could be rect (trace) or TPMPARAMS (TrackPopupMenuEx)
 */
HRESULT WINAPI FileMenu_TrackPopupMenuEx (DWORD t, DWORD uFlags, DWORD posX, DWORD posY, HWND32 hWndParent, DWORD z)
{	FIXME(shell,"0x%lx flags=0x%lx posx=0x%lx posy=0x%lx hwndp=0x%x 0x%lx stub\n",
		t,uFlags,posX,posY, hWndParent,z);
	return 0;
}
/*************************************************************************
 *  SHWinHelp [SHELL32.127]
 *
 */
HRESULT WINAPI SHWinHelp (DWORD v, DWORD w, DWORD x, DWORD z)
{	FIXME(shell,"0x%08lx 0x%08lx 0x%08lx 0x%08lx stub\n",v,w,x,z);
	return 0;
}
/*************************************************************************
 *  SHRunControlPanel [SHELL32.161]
 *
 */
HRESULT WINAPI SHRunControlPanel (DWORD x, DWORD z)
{	FIXME(shell,"0x%08lx 0x%08lx stub\n",x,z);
	return 0;
}
/*************************************************************************
 * ShellExecuteEx [SHELL32.291]
 *
 */
BOOL32 WINAPI ShellExecuteEx32A (LPSHELLEXECUTEINFO32A sei)
{ 	CHAR szTemp[MAX_PATH];

  	FIXME(shell,"(%p): stub\n",sei);

	if (sei->fMask & SEE_MASK_IDLIST)
	{ SHGetPathFromIDList32A (sei->lpIDList,szTemp);
	  TRACE (shell,"-- idlist=%p (%s)\n", sei->lpIDList, szTemp);
	}

	if (sei->fMask & SEE_MASK_CLASSNAME)
	{ TRACE (shell,"-- classname= %s\n", sei->lpClass);
	}
    
	if (sei->lpVerb)
	{ TRACE (shell,"-- action=%s\n", sei->lpVerb);
	}
	
	return 0;
}
/*************************************************************************
 * SHSetInstanceExplorer [SHELL32.176]
 *
 */
HRESULT WINAPI SHSetInstanceExplorer (DWORD u)
{	FIXME(shell,"0x%08lx stub\n",u);
	return 0;
}
/*************************************************************************
 * SHGetInstanceExplorer [SHELL32.256]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI SHGetInstanceExplorer (DWORD u)
{	FIXME(shell,"0x%08lx stub\n",u);
	return 0;
}
/*************************************************************************
 * SHFreeUnusedLibraries [SHELL32.123]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI SHFreeUnusedLibraries (DWORD u)
{	FIXME(shell,"0x%08lx stub\n",u);
	return 0;
}
/*************************************************************************
 * DAD_ShowDragImage [SHELL32.137]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI DAD_ShowDragImage (DWORD u)
{ FIXME(shell,"0x%08lx stub\n",u);
  return 0;
}
/*************************************************************************
 * FileMenu_Destroy [SHELL32.118]
 *
 * NOTES
 *  exported by name
 */
HRESULT WINAPI FileMenu_Destroy (DWORD u)
{ FIXME(shell,"0x%08lx stub\n",u);
  return 0;
}
/*************************************************************************
 * SHGetDataFromIDListA [SHELL32.247]
 *
 */
HRESULT WINAPI SHGetDataFromIDListA(DWORD u, DWORD v, DWORD w, DWORD x, DWORD y)
{	FIXME(shell,"0x%04lx 0x%04lx 0x%04lx 0x%04lx 0x%04lx stub\n",u,v,w,x,y);
	return 0;
}
/*************************************************************************
 * SHRegQueryValueEx32W [NT4.0:SHELL32.511]
 *
 */
HRESULT WINAPI SHRegQueryValueEx32W (DWORD u, LPWSTR v, DWORD w, DWORD x, DWORD y, DWORD z)
{	FIXME(shell,"0x%04lx %s 0x%04lx 0x%04lx 0x%04lx  0x%04lx stub\n",u,debugstr_w(v),w,x,y,z);
	return 0;
}

/*************************************************************************
 * ReadCabinetState [NT 4.0:SHELL32.651]
 *
 */
HRESULT WINAPI ReadCabinetState(DWORD u, DWORD v)
{	FIXME(shell,"0x%04lx 0x%04lx stub\n",u,v);
	return 0;
}
/*************************************************************************
 * WriteCabinetState [NT 4.0:SHELL32.652]
 *
 */
HRESULT WINAPI WriteCabinetState(DWORD u)
{	FIXME(shell,"0x%04lx stub\n",u);
	return 0;
}
/*************************************************************************
 * IsUserAdmin [NT 4.0:SHELL32.680]
 *
 */
HRESULT WINAPI IsUserAdmin()
{	FIXME(shell,"stub\n");
	return TRUE;
}
/*************************************************************************
 * SHFlushClipboard [SHELL32.121]
 *
 */
HRESULT WINAPI SHFlushClipboard(VOID)
{	FIXME(shell,"stub\n");
	return 0;
}
