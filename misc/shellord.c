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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "windows.h"
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
#include "stddebug.h"
#include "debug.h"
#include "winreg.h"

/*************************************************************************
 *	 		 SHELL32_2   			[SHELL32.2]
 */
DWORD WINAPI SHELL32_2(HWND32 hwnd,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	fprintf(stderr,"SHELL32_2(0x%04x,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
		hwnd,x2,x3,x4,x5,x6
	);
	return 0;
}
/*************************************************************************
 *	 		 SHELL32_16   			[SHELL32.16]
 * find_lastitem_in_itemidlist()
 */
LPSHITEMID WINAPI SHELL32_16(LPITEMIDLIST iil) {
	LPSHITEMID	lastsii,sii;

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
 *	 		 SHELL32_29   			[SHELL32.29]
 * is_rootdir(const char*path)
 */
BOOL32 WINAPI SHELL32_29(LPCSTR x) {
	if (!lstrcmp32A(x+1,":\\"))		/* "X:\" */
		return 1;
	if (!lstrcmp32A(x,"\\"))		/* "\" */
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
 *	 		 SHELL32_30   			[SHELL32.30]
 * get_rootdir(char*path,int drive)
 */
LPSTR WINAPI SHELL32_30(LPSTR root,BYTE drive) {
	strcpy(root,"A:\\");
	root[0]+=drive;
	return root;
}

/*************************************************************************
 *					SHELL32_31      [SHELL32.31]
 * returns pointer to last . in last pathcomponent or at \0.
 */
LPSTR WINAPI SHELL32_31(LPSTR path) {
    LPSTR   lastpoint = NULL;

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
 *				SHELL32_32	[SHELL32.32]
 * append \ if there is none
 */
LPSTR WINAPI SHELL32_32(LPSTR path) {
    int len;

    len = lstrlen32A(path);
    if (len && path[len-1]!='\\') {
    	path[len+0]='\\';
	path[len+1]='\0';
	return path+len+1;
    } else
    	return path+len;
}

/*************************************************************************
 *				SHELL32_33      [SHELL32.33]
 * remove spaces from beginning and end of passed string
 */
LPSTR WINAPI SHELL32_33(LPSTR str) {
    LPSTR x = str;

    while (*x==' ') x++;
    if (x!=str)
	lstrcpy32A(str,x);
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
 *				SHELL32_34	[SHELL32.34]
 * basename(char *fn);
 */
LPSTR WINAPI SHELL32_34(LPSTR fn) {
    LPSTR basefn;

    basefn = fn;
    while (fn[0]) {
    	if (((fn[0]=='\\') || (fn[0]==':')) && fn[1] && fn[1]!='\\')
	    basefn = fn+1;
	fn++;
    }
    return basefn;
}

/*************************************************************************
 *	 		 SHELL32_35   			[SHELL32.35]
 * bool getpath(char *pathname); truncates passed argument to a valid path
 * returns if the string was modified or not.
 * "\foo\xx\foo"-> "\foo\xx"
 * "\"		-> "\"
 * "a:\foo"	-> "a:\"
 */
DWORD WINAPI SHELL32_35(LPSTR fn) {
	LPSTR	x,cutplace;

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
 *				SHELL32_37	[SHELL32.37]
 * concat_paths(char*target,const char*add);
 * concats "target\\add" and writes them to target
 */
LPSTR WINAPI SHELL32_37(LPSTR target,LPSTR x1,LPSTR x2) {
	char	buf[260];

	if (!x2 || !x2[0]) {
		lstrcpy32A(target,x1);
		return target;
	}
	lstrcpy32A(buf,x1);
	SHELL32_32(buf); /* append \ if not there */
	lstrcat32A(buf,x2);
	lstrcpy32A(target,buf);
	return target;
}

/*************************************************************************
 *				SHELL32_36	[SHELL32.36]
 * concat_paths(char*target,const char*add);
 * concats "target\\add" and writes them to target
 */
LPSTR WINAPI SHELL32_36(LPSTR x1,LPSTR x2) {
	while (x2[0]=='\\') x2++;
	return SHELL32_37(x1,x1,x2);
}

/*************************************************************************
 *				SHELL32_39	[SHELL32.39]
 * isUNC(const char*path);
 */
BOOL32 WINAPI SHELL32_39(LPCSTR path) {
	if ((path[0]=='\\') && (path[1]=='\\'))
		return TRUE;
	return FALSE;
}

/*************************************************************************
 *				SHELL32_45	[SHELL32.45]
 * file_exists(char *fn);
 */
BOOL32 WINAPI SHELL32_45(LPSTR fn) {
    if (GetFileAttributes32A(fn)==-1)
    	return FALSE;
    else
    	return TRUE;
}

/*************************************************************************
 *				SHELL32_52	[SHELL32.52]
 * look for next arg in string. handle "quoted" strings
 * returns pointer to argument *AFTER* the space. Or to the \0.
 */
LPSTR WINAPI SHELL32_52(LPSTR cmdline) {
    BOOL32	qflag = FALSE;

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
 *				SHELL32_56      [SHELL32.56]
 * unquote string (remove ")
 */
VOID WINAPI SHELL32_56(LPSTR str) {
    DWORD      len = lstrlen32A(str);

    if (*str!='"') return;
    if (str[len-1]!='"') return;
    str[len-1]='\0';
    lstrcpy32A(str,str+1);
    return;
}

/*************************************************************************
 *	 		 SHELL32_58   			[SHELL32.58]
 */
DWORD WINAPI SHELL32_58(LPCSTR src,DWORD x2,LPSTR target,DWORD pathlen) {
	fprintf(stderr,"SHELL32_58(%s,0x%08lx,%p,%ld),STUB!\n",
		src,x2,target,pathlen
	);
	if (!src)
		return 0;
	return 0;
}

/*************************************************************************
 *	 		 SHELL32_62   			[SHELL32.62]
 */
DWORD WINAPI SHELL32_62(DWORD x,DWORD y,DWORD z,DWORD a) {
	fprintf(stderr,"SHELL32_62(%08lx,%08lx,%08lx,%08lx),stub!\n",x,y,z,a);
	return 0xffffffff;
}

/*************************************************************************
 *                      SHELL32_63                     [SHELL32.63]
 */
DWORD WINAPI SHELL32_63(HWND32 howner, LPSTR targetbuf, DWORD len, DWORD x, LPCSTR suffix, LPCSTR y, LPCSTR cmd) {
    fprintf(stderr,"SHELL32_63(%04x,%p,%ld,%08lx,%s,%s,%s),stub!\n",
	    howner,targetbuf,len,x,suffix,y,cmd
    );
    /* puts up a Open Dialog and requests input into targetbuf */
    /* OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_FILEMUSTEXIST|OFN_unknown */
    lstrcpy32A(targetbuf,"x:\\s3.exe");
    return 1;
}

/*************************************************************************
 *                      SHELL32_68                     [SHELL32.68]
 */
DWORD WINAPI SHELL32_68(DWORD x,DWORD y,DWORD z) {
	fprintf(stderr,"SHELL32_68(0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x,y,z
	);
	return 0;
}
/*************************************************************************
 *			 SHELL32_71   			[SHELL32.71]
 * returns internal shell values in the passed pointers
 */
BOOL32 WINAPI SHELL32_71(LPDWORD x,LPDWORD y) {

	fprintf(stderr,"SHELL32_71(%p,%p),stub!\n",x,y);
	return TRUE;
}

/*************************************************************************
 *			 SHELL32_72   			[SHELL32.72]
 * dunno. something with icons
 */
void WINAPI SHELL32_72(LPSTR x,DWORD y,DWORD z) {
	fprintf(stderr,"SHELL32_72(%s,%08lx,%08lx),stub!\n",x,y,z);
}

/*************************************************************************
 *			 SHELL32_89   			[SHELL32.89]
 */
DWORD WINAPI SHELL32_89(DWORD x1,DWORD x2,DWORD x3) {
	fprintf(stderr,"SHELL32_89(0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x1,x2,x3
	);
	return 0;
}

/*************************************************************************
 *				SHELL32_119	[SHELL32.119]
 * unknown
 */
void WINAPI SHELL32_119(LPVOID x) {
    fprintf(stderr,"SHELL32_119(%p(%s)),stub\n",x,(char *)x);
}

/*************************************************************************
 *				SHELL32_175	[SHELL32.175]
 * unknown
 */
void WINAPI SHELL32_175(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
    fprintf(stdnimp,"SHELL32_175(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub\n",
    	x1,x2,x3,x4
    );
}

/*************************************************************************
 *				SHELL32_181	[SHELL32.181]
 * unknown
 */
void WINAPI SHELL32_181(DWORD x,DWORD y) {
    fprintf(stderr,"SHELL32_181(0x%08lx,0x%08lx)\n",x,y);
}

/*************************************************************************
 *				SHELL32_75	[SHELL32.75]
 * unknown
 */
BOOL32 WINAPI SHELL32_75(LPDWORD x,LPDWORD y) {
    fprintf(stderr,"SHELL32_75(%p,%p),stub\n",x,y);
    return TRUE;
}

/*************************************************************************
 *	 		 SHELL32_77   			[SHELL32.77]
 */
DWORD WINAPI SHELL32_77(DWORD x,DWORD y,DWORD z) {
	fprintf(stderr,"SHELL32_77(%08lx,%08lx,%08lx),stub!\n",x,y,z);
	return 0;
}

/*************************************************************************
 *	 		 SHELL32_79   			[SHELL32.79]
 * create_directory_and_notify(...)
 */
DWORD WINAPI SHELL32_79(LPCSTR dir,LPVOID xvoid) {
	fprintf(stderr,"mkdir %s,%p\n",dir,xvoid);
	if (!CreateDirectory32A(dir,xvoid))
		return FALSE;
	/* SHChangeNotify(8,1,dir,0); */
	return TRUE;
}

static FARPROC32 _find_moduleproc(LPSTR dllname,HMODULE32 *xhmod,LPSTR name) {
	HMODULE32	hmod;
	FARPROC32	dllunload,nameproc;

	if (xhmod) *xhmod = 0;
	if (!lstrcmpi32A(SHELL32_34(dllname),"shell32.dll"))
		return (FARPROC32)SHELL32_DllGetClassObject;

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
	return nameproc;
}

static DWORD _get_instance(REFCLSID clsid,LPSTR dllname,
	LPVOID	unknownouter,REFIID refiid,LPVOID inst
) {
	DWORD	WINAPI	(*dllgetclassob)(REFCLSID,REFIID,LPVOID);
	DWORD		hres;
/*
	LPCLASSFACTORY	classfac;
 */

	dllgetclassob = (DWORD(*)(REFCLSID,REFIID,LPVOID))_find_moduleproc(dllname,NULL,"DllGetClassObject");
	if (!dllgetclassob)
		return 0x80070000|GetLastError();

/* FIXME */
	hres = (*dllgetclassob)(clsid,(REFIID)&IID_IClassFactory,inst);
	if (hres<0)
		return hres;

/*
	hres = (*dllgetclassob)(clsid,(REFIID)&IID_IClassFactory,&classfac);
	if (hres<0)
		return hres;
	classfac->lpvtbl->fnCreateInstance(classfac,unknownouter,refiid,inst);
	classfac->lpvtbl->fnRelease(classfac);
 */
	return 0;
}
/*************************************************************************
 *				SHELL32_102	[SHELL32.102]
 * unknown
 */
LRESULT WINAPI SHELL32_102(
	LPSTR aclsid,CLSID *clsid,LPUNKNOWN unknownouter,REFIID refiid,LPVOID inst
) {
	char	buffer[256],xclsid[48],xiid[48],path[260],tmodel[100];
	HKEY	inprockey;
	DWORD	pathlen,type,tmodellen;
	DWORD	hres;
	
	StringFromCLSID(refiid,xiid);

	if (clsid)
		StringFromCLSID(clsid,xclsid);
	else {
		if (!aclsid)
		    return 0x80040154;
		strcpy(xclsid,aclsid);
	}
	fprintf(stderr,"SHELL32_102(%p,%s,%p,%s,%p)\n",
		aclsid,xclsid,unknownouter,xiid,inst
	);

	sprintf(buffer,"CLSID\\%s\\InProcServer32",xclsid);
	if (RegOpenKeyEx32A(HKEY_CLASSES_ROOT,buffer,0,0x02000000,&inprockey))
		return _get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	pathlen=sizeof(path);
	if (RegQueryValue32A(inprockey,NULL,path,&pathlen)) {
		RegCloseKey(inprockey);
		return _get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	}
	fprintf(stderr,"	-> server dll is %s\n",path);
	tmodellen=sizeof(tmodel);
	type=REG_SZ;
	if (RegQueryValueEx32A(inprockey,"ThreadingModel",NULL,&type,tmodel,&tmodellen)) {
		RegCloseKey(inprockey);
		return _get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	}
	fprintf(stderr,"	-> threading model is %s\n",tmodel);
	hres=_get_instance(clsid,path,unknownouter,refiid,inst);
	if (hres<0)
		hres=_get_instance(clsid,"shell32.dll",unknownouter,refiid,inst);
	RegCloseKey(inprockey);
	return hres;
}


/*************************************************************************
 *			 SHELL32_183   			[SHELL32.183]
 * Format and output errormessage.
 */
void __cdecl SHELL32_183(HMODULE32 hmod,HWND32 hwnd,DWORD id,DWORD x,DWORD type,LPVOID arglist) {
	char	buf[100],buf2[100],*buf3;
	LPVOID	args = &arglist;

	if (!LoadString32A(hmod,x,buf,100))
		strcpy(buf,"Desktop");
	LoadString32A(hmod,id,buf2,100);
	/* FIXME: the varargs handling doesn't. */
	FormatMessage32A(0x500,buf2,0,0,&buf3,256,&args);

	fprintf(stderr,"SHELL32_183(%08lx,%08lx,%08lx(%s),%08lx(%s),%08lx,%p),stub!\n",
		(DWORD)hmod,(DWORD)hwnd,id,buf2,x,buf,type,arglist
	);
	MessageBox32A(hwnd,buf3,buf,id|0x10000);
}


/*************************************************************************
 *			 SHELL32_100   			[SHELL32.100]
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
 */
DWORD WINAPI SHELL32_100(DWORD pol) {
	HKEY	xhkey;

	fprintf(stderr,"SHELL32_100(%08lx),stub!\n",pol);
	if (RegOpenKey32A(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Policies",&xhkey))
		return 0;
	/* FIXME: do nothing for now, just return 0 (== "allowed") */
	RegCloseKey(xhkey);
	return 0;
	
}

/*************************************************************************
 *	 		 SHELL32_152   			[SHELL32.152]
 * itemlist_length
 */
DWORD WINAPI SHELL32_152(LPITEMIDLIST iil) {
	LPSHITEMID	si;
	DWORD		len;

	si = &(iil->mkid);
	len = 2;
	while (si->cb) {
		len	+= si->cb;
		si	 = (LPSHITEMID)(((char*)si)+si->cb);
	}
	return len;
}

/*************************************************************************
 *                      SHELL32_158                    [SHELL32.158]
 */
LPSTR WINAPI SHELL32_158(LPSTR path,DWORD y,DWORD z) {
    fprintf(stderr,"SHELL32_158(%s,%08lx,%08lx)\n",path,y,z);
    path = SHELL32_31(path);
    return *path?(path+1):path;
}

/*************************************************************************
 *	 		 SHELL32_165   			[SHELL32.165]
 * create_path_and_notify(...)
 */
DWORD WINAPI SHELL32_165(DWORD x,LPCSTR path) {
	if (SHELL32_79(path,(LPVOID)x))
		return 0;
	fprintf(stderr,"SHELL32_165(%08lx,%s),stub!\n",x,path);
	return 0;
}

/*************************************************************************
 *	 		 SHELL32_195   			[SHELL32.195]
 * free_ptr() - frees memory using IMalloc
 */
DWORD WINAPI SHELL32_195(LPVOID x) {
	return LocalFree32((HANDLE32)x);
}

/*************************************************************************
 *	 		 SHELL32_196   			[SHELL32.196]
 * void *task_alloc(DWORD len), uses SHMalloc allocator
 */
LPVOID WINAPI SHELL32_196(DWORD len) {
	return (LPVOID)LocalAlloc32(len,LMEM_ZEROINIT); /* FIXME */
}

/*************************************************************************
 *	 		 SHELL32_18   			[SHELL32.18]
 * copy_itemidlist()
 */
LPITEMIDLIST WINAPI SHELL32_18(LPITEMIDLIST iil) {
	DWORD		len;
	LPITEMIDLIST	newiil;

	len = SHELL32_152(iil);
	newiil = (LPITEMIDLIST)SHELL32_196(len);
	memcpy(newiil,iil,len);
	return newiil;
}

/*************************************************************************
 *	 		 SHELL32_25   			[SHELL32.25]
 * merge_itemidlist()
 */
LPITEMIDLIST WINAPI SHELL32_25(LPITEMIDLIST iil1,LPITEMIDLIST iil2) {
	DWORD		len1,len2;
	LPITEMIDLIST	newiil;

	len1 	= SHELL32_152(iil1)-2;
	len2	= SHELL32_152(iil2);
	newiil	= SHELL32_196(len1+len2);
	memcpy(newiil,iil1,len1);
	memcpy(((char*)newiil)+len1,iil2,len2);
	return newiil;
}

/*************************************************************************
 *	 		 SHELL32_155   			[SHELL32.155]
 * free_check_ptr - frees memory (if not NULL) allocated by SHMalloc allocator
 */
DWORD WINAPI SHELL32_155(LPVOID x) {
	if (!x)
		return 0;
	return SHELL32_195(x);
}
