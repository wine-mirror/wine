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
 *	 		 ILGetDisplayName			[SHELL32.15]
 * get_path_from_itemlist(itemlist,path); ? not sure...
 */
BOOL32 WINAPI ILGetDisplayName(LPCITEMIDLIST iil,LPSTR path) {
	FIXME(shell,"(%p,%p),stub, return e:!\n",iil,path);
	strcpy(path,"e:\\");
	return TRUE;
}
/*************************************************************************
 * ILFindLastID [SHELL32.16]
 */
LPSHITEMID WINAPI ILFindLastID(LPITEMIDLIST iil) {
	LPSHITEMID	lastsii,sii;

  TRACE(shell,"%p\n",iil);
	if (!iil)
		return NULL;
	sii = &(iil->mkid);
	lastsii = sii;
	while (sii->cb) {
		lastsii = sii;
		sii = (LPSHITEMID)(((char*)sii)+sii->cb);
	}
	return lastsii;
}
/*************************************************************************
 * ILFindLastID [SHELL32.17]
 * NOTES
 *  Creates a new list with the last tiem removed
 */
 LPITEMIDLIST WINAPI ILRemoveLastID(LPCITEMIDLIST);


/*************************************************************************
 * ILClone [SHELL32.18]
 *
 * NOTES
 *    dupicate an idlist
 */
LPITEMIDLIST WINAPI ILClone (LPITEMIDLIST iil) {
	DWORD		len;
	LPITEMIDLIST	newiil;
  TRACE(shell,"%p\n",iil);
	len = ILGetSize(iil);
	newiil = (LPITEMIDLIST)SHAlloc(len);
	if (newiil)
		memcpy(newiil,iil,len);
	return newiil;
}

/*************************************************************************
 * ILCloneFirst [SHELL32.19]
 *
 * NOTES
 *  duplicates the first idlist of a complex pidl
 */
LPITEMIDLIST WINAPI ILCloneFirst(LPCITEMIDLIST pidl);

/*************************************************************************
 * ILCombine [SHELL32.25]
 *
 * NOTES
 *  Concatenates two complex idlists.
 *  The pidl is the first one, pidlsub the next one
 *  Does not destroy the passed in idlists!
 */
LPITEMIDLIST WINAPI ILCombine(LPITEMIDLIST iil1,LPITEMIDLIST iil2) {
	DWORD		len1,len2;
	LPITEMIDLIST	newiil;
	TRACE(shell,"%p %p\n",iil1,iil2);
	len1 	= ILGetSize(iil1)-2;
	len2	= ILGetSize(iil2);
	newiil	= SHAlloc(len1+len2);
	memcpy(newiil,iil1,len1);
	memcpy(((char*)newiil)+len1,iil2,len2);
	return newiil;
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
{ int len;
  TRACE(shell,"%p->%s\n",path,path);
  len = strlen(path);
  if (len && path[len-1]!='\\') 
	{ path[len+0]='\\';
    path[len+1]='\0';
    return path+len+1;
  }
	else
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
LPSTR WINAPI PathFindFilename(LPSTR fn) {
    LPSTR basefn;
  TRACE(shell,"%s\n",fn);
    basefn = fn;
    while (fn[0]) {
    	if (((fn[0]=='\\') || (fn[0]==':')) && fn[1] && fn[1]!='\\')
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
 * PathCombine [SHELL32.37]
 * 
 * NOTES
 *     concat_paths(char*target,const char*add);
 *     concats "target\\add" and writes them to target
 */
LPSTR WINAPI PathCombine(LPSTR target,LPSTR x1,LPSTR x2) {
	char	buf[260];
  TRACE(shell,"%s %s\n",x1,x2);
	if (!x2 || !x2[0]) {
		lstrcpy32A(target,x1);
		return target;
	}
	lstrcpy32A(buf,x1);
	PathAddBackslash(buf); /* append \ if not there */
	lstrcat32A(buf,x2);
	lstrcpy32A(target,buf);
	return target;
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
 * PathIsUNC [SHELL32.39]
 * 
 * NOTES
 *     isUNC(const char*path);
 */
BOOL32 WINAPI PathIsUNC(LPCSTR path) {
  TRACE(shell,"%s\n",path);
	if ((path[0]=='\\') && (path[1]=='\\'))
		return TRUE;
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
 * Shell_GetImageList [SHELL32.71]
 * 
 * NOTES
 *     returns internal shell values in the passed pointers
 */
BOOL32 WINAPI Shell_GetImageList(LPDWORD x,LPDWORD y) {

	FIXME(shell,"(%p,%p):stub.\n",x,y);
	return TRUE;
}

/*************************************************************************
 * Shell_GetCachedImageIndex [SHELL32.72]
 *
 */
void WINAPI Shell_GetCachedImageIndex(LPSTR x,DWORD y,DWORD z) {
	FIXME(shell,"(%s,%08lx,%08lx):stub.\n",x,y,z);
}
/*************************************************************************
 * SHShellFolderView_Message [SHELL32.73]
 * NOTES
 *  Message SFVM_REARRANGE = 1
 *    This message gets sent when a column gets clicked to instruct the
 *    shell view to re-sort the item list. lParam identifies the column
 *    that was clicked.
 */
int WINAPI SHShellFolderView_Message(
    HWND32 hwndCabinet, /* This hwnd defines the explorer cabinet window that contains the 
                         shellview you need to communicate with*/
		UINT32 uMsg,        /* A parameter identifying the SFVM enum to perform */
		LPARAM lParam);
/*************************************************************************
 * SHCloneSpecialIDList [SHELL32.89]
 * 
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHCloneSpecialIDList(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(shell,"(0x%08lx,0x%08lx,0x%08lx):stub.\n",
		x1,x2,x3
	);
	return 0;
}

/*************************************************************************
 * IsLFNDrive [SHELL32.119]
 * 
 * NOTES
 *     exported by ordinal Name
 */
void WINAPI IsLFNDrive(LPVOID x) {
    FIXME(shell,"(%p(%s)):stub.\n",x,(char *)x);
}

/*************************************************************************
 * SHGetSpecialFolderPath [SHELL32.175]
 * 
 * NOTES
 *     exported by ordinal
 */
void WINAPI SHGetSpecialFolderPath(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
    FIXME(shell,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx):stub.\n",
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
 * NOTES
 *     exported by ordinal
 *
 */
DWORD WINAPI
SHMapPIDLToSystemImageListIndex(
    DWORD x,	/* pointer to an instance of IShellFolder */
		DWORD y,
		DWORD z)
{	FIXME(shell,"(%08lx,%08lx,%08lx):stub.\n",x,y,z);
	return 0;
}

/*************************************************************************
 * OleStrToStrN	[SHELL32.78]
 * 
 * NOTES
 *     exported by ordinal
 */
BOOL32 WINAPI
OleStrToStrN (LPSTR lpMulti, INT32 nMulti, LPCWSTR lpWide, INT32 nWide) {
    return WideCharToMultiByte (0, 0, lpWide, nWide,
				lpMulti, nMulti, NULL, NULL);
}

/*************************************************************************
 * StrToOleStrN	[SHELL32.79]
 * 
 * NOTES
 *     exported by ordinal
 */
BOOL32 WINAPI
StrToOleStrN (LPWSTR lpWide, INT32 nWide, LPCSTR lpMulti, INT32 nMulti) {
    return MultiByteToWideChar (0, 0, lpMulti, nMulti, lpWide, nWide);
}

/*************************************************************************
 *
 */
typedef DWORD (* WINAPI GetClassPtr)(REFCLSID,REFIID,LPVOID);

static GetClassPtr SH_find_moduleproc(LPSTR dllname,HMODULE32 *xhmod,
                                      LPSTR name)
{
	HMODULE32	hmod;
	FARPROC32	dllunload,nameproc;

	if (xhmod) *xhmod = 0;
	if (!strcasecmp(PathFindFilename(dllname),"shell32.dll"))
		return (GetClassPtr)SHELL32_DllGetClassObject;

	hmod = LoadLibraryEx32A(dllname,0,LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!hmod)
		return NULL;
	dllunload = GetProcAddress32(hmod,"DllCanUnloadNow");
	if (!dllunload)
		if (xhmod) *xhmod = hmod;
	nameproc = GetProcAddress32(hmod,name);
	if (!nameproc) {
		FreeLibrary32(hmod);
		return NULL;
	}
	/* register unloadable dll with unloadproc ... */
	return (GetClassPtr)nameproc;
}
/*************************************************************************
 *
 */
static DWORD SH_get_instance(
    REFCLSID clsid,
		LPSTR dllname,
	  LPVOID unknownouter,
		REFIID refiid,
		LPVOID inst) 
{ GetClassPtr     dllgetclassob;
	DWORD		hres;
	LPCLASSFACTORY	classfac;

  char	xclsid[50],xrefiid[50];
  WINE_StringFromCLSID((LPCLSID)clsid,xclsid);
  WINE_StringFromCLSID((LPCLSID)refiid,xrefiid);
  TRACE(shell,"\n\tCLSID:%s,%s,%p,\n\tIID:%s,%p\n",
	   xclsid, dllname,unknownouter,xrefiid,inst);
	
	dllgetclassob = SH_find_moduleproc(dllname,NULL,"DllGetClassObject");
	if (!dllgetclassob)
		return 0x80070000|GetLastError();

/* FIXME */
/*
	hres = (*dllgetclassob)(clsid,(REFIID)&IID_IClassFactory,inst);
	if (hres<0)
		return hres;

 */
	hres = (*dllgetclassob)(clsid,(REFIID)&IID_IClassFactory,&classfac);
	if (hres<0 || (hres>=0x80000000))
		return hres;
	if (!classfac) {
		FIXME(shell,"no classfactory, but hres is 0x%ld!\n",hres);
		return E_FAIL;
	}
	classfac->lpvtbl->fnCreateInstance(classfac,unknownouter,refiid,inst);
	classfac->lpvtbl->fnRelease(classfac);
	return 0;
}

/*************************************************************************
 * SHCoCreateInstance [SHELL32.102]
 * 
 * NOTES
 *     exported by ordinal
 */
LRESULT WINAPI SHCoCreateInstance(
	LPSTR aclsid,CLSID *clsid,LPUNKNOWN unknownouter,REFIID refiid,LPVOID inst
) {
	char	buffer[256],xclsid[48],xiid[48],path[260],tmodel[100];
	HKEY	inprockey;
	DWORD	pathlen,type,tmodellen;
	DWORD	hres;
	
	WINE_StringFromCLSID(refiid,xiid);

	if (clsid)
		WINE_StringFromCLSID(clsid,xclsid);
	else {
		if (!aclsid)
		    return 0x80040154;
		strcpy(xclsid,aclsid);
	}
	TRACE(shell,"(%p,\n\tSID:\t%s,%p,\n\tIID:\t%s,%p)\n",aclsid,xclsid,unknownouter,xiid,inst);

	sprintf(buffer,"CLSID\\%s\\InProcServer32",xclsid);
	if (RegOpenKeyEx32A(HKEY_CLASSES_ROOT,buffer,0,0x02000000,&inprockey))
		return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	pathlen=sizeof(path);
	if (RegQueryValue32A(inprockey,NULL,path,&pathlen)) {
		RegCloseKey(inprockey);
		return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	}
	TRACE(shell, "Server dll is %s\n",path);
	tmodellen=sizeof(tmodel);
	type=REG_SZ;
	if (RegQueryValueEx32A(inprockey,"ThreadingModel",NULL,&type,tmodel,&tmodellen)) {
		RegCloseKey(inprockey);
		return SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	}
	TRACE(shell, "Threading model is %s\n",tmodel);
	hres=SH_get_instance(clsid,path,unknownouter,refiid,inst);
	if (hres<0)
		hres=SH_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	RegCloseKey(inprockey);
	return hres;
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
	char	buf[100],buf2[100],*buf3;
	LPVOID	args = &arglist;

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
 * ILGetSize [SHELL32.152]
 *
 * NOTES
 *  exported by ordinal
 *  Gets the byte size of an idlist including zero terminator
 */
DWORD WINAPI ILGetSize(LPITEMIDLIST iil) {
	LPSHITEMID	si;
	DWORD		len;
	TRACE(shell,"%p\n",iil);
	if (!iil)
		return 0;
	si = &(iil->mkid);
	len = 2;
	while (si->cb) {
		len	+= si->cb;
		si	 = (LPSHITEMID)(((char*)si)+si->cb);
	}
	return len;
}
/*************************************************************************
 * ILAppend [SHELL32.154]
 *
 * NOTES
 *  Adds the single item to the idlist indicated by pidl.
 *  if bEnd is 0, adds the item to the front of the list,
 *  otherwise adds the item to the end.
 *  Destroys the passed in idlist!
 */
LPITEMIDLIST WINAPI ILAppend(LPITEMIDLIST pidl,LPCITEMIDLIST item,BOOL32 bEnd);

/*************************************************************************
 * PathGetExtension [SHELL32.158]
 *
 * NOTES
 *     exported by ordinal
 */
LPSTR WINAPI PathGetExtension(LPSTR path,DWORD y,DWORD z) {
    TRACE(shell,"(%s,%08lx,%08lx)\n",path,y,z);
    path = PathFindExtension(path);
    return *path?(path+1):path;
}

/*************************************************************************
 * SHCreateDirectory [SHELL32.165]
 *
 * NOTES
 *     exported by ordinal
 */
DWORD WINAPI SHCreateDirectory(DWORD x,LPCSTR path) {
	TRACE(shell,"(%08lx,%s):stub.\n",x,path);
	if (CreateDirectory32A(path,x))
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
	return LocalFree32((HANDLE32)x);
}

/*************************************************************************
 * SHAlloc [SHELL32.196]
 *
 * NOTES
 *     void *task_alloc(DWORD len), uses SHMalloc allocator
 *     exported by ordinal
 */
LPVOID WINAPI SHAlloc(DWORD len) {
  TRACE(shell,"%lu\n",len);
	return (LPVOID)LocalAlloc32(len,LMEM_ZEROINIT); /* FIXME */
}


/*************************************************************************
 * ILFree [SHELL32.155]
 *
 * NOTES
 *     free_check_ptr - frees memory (if not NULL)
 *     allocated by SHMalloc allocator
 *     exported by ordinal
 */
DWORD WINAPI ILFree(LPVOID x) {
	FIXME (shell,"(0x%08lx):stub.\n", (DWORD)x);
	if (!x)
		return 0;
	return SHFree(x);
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
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHAddToRecentDocs32 (
    UINT32 uFlags,  /* [IN] SHARD_PATH or SHARD_PIDL */
		LPCVOID pv)     /* [IN] string or pidl, NULL clears the list */
{ if (SHARD_PIDL==uFlags)
  { FIXME (shell,"(0x%08x,pidl=%p):stub.\n", uFlags,pv);
	}
	else
	{ FIXME (shell,"(0x%08x,%s):stub.\n", uFlags,(char*)pv);
	}
  return 0;
}

/*************************************************************************
 * SHFileOperation [SHELL32.242]
 *
 * NOTES
 *     exported by name
 */
DWORD WINAPI SHFileOperation32 (
    LPSHFILEOPSTRUCT32 lpFileOp)   
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
 */
HRESULT WINAPI SHCreateShellFolderViewEx32(
  LPSHELLVIEWDATA psvcbi, /*[in ] shelltemplate struct*/
  LPVOID* ppv)            /*[out] IShellView pointer*/
{ FIXME (shell,"(%p,%p):stub.\n", psvcbi,ppv);
  return 0;
}
