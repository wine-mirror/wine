/* 
 * Implementation of VERSION.DLL - File Installer routines
 * 
 * Copyright 1996,1997 Marcus Meissner
 * Copyright 1997 David Cuthbert
 */

#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winver.h"
#include "wine/winestring.h"
#include "winerror.h"
#include "heap.h"
#include "ver.h"
#include "lzexpand.h"
#include "xmalloc.h"
#include "debug.h"


/******************************************************************************
 *
 *   void  ver_dstring(
 *      char const * prologue,
 *      char const * teststring,
 *      char const * epilogue )
 *
 *   This function will print via dprintf[_]ver to stddeb the prologue string,
 *   followed by the address of teststring and the string it contains if
 *   teststring is non-null or "(null)" otherwise, and then the epilogue
 *   string followed by a new line.
 *
 *   Revision history
 *      30-May-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *         Original implementation as dprintf[_]ver_string
 *      05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *         Fixed problem that caused bug with tools/make_debug -- renaming
 *         this function should fix the problem.
 *      15-Feb-1998 Dimitrie Paun (dimi@cs.toronto.edu)
 *         Modified it to make it print the message using only one
 *         dprintf[_]ver call.
 *
 *****************************************************************************/

static void  ver_dstring(
    char const * prologue,
    char const * teststring,
    char const * epilogue )
{
    TRACE(ver, "%s %p (\"%s\") %s\n", prologue,
                (void const *) teststring,
                teststring ? teststring : "(null)",
                epilogue);
}

/******************************************************************************
 *
 *   int  testFileExistence(
 *      char const * path,
 *      char const * file )
 *
 *   Tests whether a given path/file combination exists.  If the file does
 *   not exist, the return value is zero.  If it does exist, the return
 *   value is non-zero.
 *
 *   Revision history
 *      30-May-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *         Original implementation
 *
 *****************************************************************************/

static int  testFileExistence(
   char const * path,
   char const * file )
{
    char  filename[1024];
    int  filenamelen;
    OFSTRUCT  fileinfo;
    int  retval;

    fileinfo.cBytes = sizeof(OFSTRUCT);

    strcpy(filename, path);
    filenamelen = strlen(filename);

    /* Add a trailing \ if necessary */
    if(filenamelen) {
	if(filename[filenamelen - 1] != '\\')
	    strcat(filename, "\\");
    }
    else /* specify the current directory */
	strcpy(filename, ".\\");

    /* Create the full pathname */
    strcat(filename, file);

    if(OpenFile(filename, &fileinfo, OF_EXIST) == HFILE_ERROR)
	retval = 0;
    else
	retval = 1;

    return  retval;
}

/******************************************************************************
 *
 *   int  testFileExclusiveExistence(
 *      char const * path,
 *      char const * file )
 *
 *   Tests whether a given path/file combination exists and ensures that no
 *   other programs have handles to the given file.  If the file does not
 *   exist or is open, the return value is zero.  If it does exist, the
 *   return value is non-zero.
 *
 *   Revision history
 *      30-May-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *         Original implementation
 *
 *****************************************************************************/

static int  testFileExclusiveExistence(
   char const * path,
   char const * file )
{
    char  filename[1024];
    int  filenamelen;
    OFSTRUCT  fileinfo;
    int  retval;

    fileinfo.cBytes = sizeof(OFSTRUCT);

    strcpy(filename, path);
    filenamelen = strlen(filename);

    /* Add a trailing \ if necessary */
    if(filenamelen) {
	if(filename[filenamelen - 1] != '\\')
	    strcat(filename, "\\");
    }
    else /* specify the current directory */
	strcpy(filename, ".\\");

    /* Create the full pathname */
    strcat(filename, file);

    if(OpenFile(filename, &fileinfo, OF_EXIST | OF_SHARE_EXCLUSIVE) ==
       HFILE_ERROR)
	retval = 0;
    else
	retval = 1;

    return retval;
}

/*****************************************************************************
 *
 *   VerFindFile() [VER.8]
 *   Determines where to install a file based on whether it locates another
 *   version of the file in the system.  The values VerFindFile returns are
 *   used in a subsequent call to the VerInstallFile function.
 *
 *   Revision history:
 *      30-May-1997   Dave Cuthbert (dacut@ece.cmu.edu)
 *         Reimplementation of VerFindFile from original stub.
 *
 ****************************************************************************/

/* VerFindFile32A						[VERSION.5] */
DWORD WINAPI VerFindFileA(
    UINT flags,
    LPCSTR lpszFilename,
    LPCSTR lpszWinDir,
    LPCSTR lpszAppDir,
    LPSTR lpszCurDir,
    UINT *lpuCurDirLen,
    LPSTR lpszDestDir,
    UINT *lpuDestDirLen )
{
    DWORD  retval;
    char  curDir[256];
    char  destDir[256];
    unsigned int  curDirSizeReq;
    unsigned int  destDirSizeReq;

    retval = 0;

    /* Print out debugging information */
    TRACE(ver, "called with parameters:\n"
		 "\tflags = %x", flags);
    if(flags & VFFF_ISSHAREDFILE)
	TRACE(ver, " (VFFF_ISSHAREDFILE)\n");
    else
	TRACE(ver, "\n");

    ver_dstring("\tlpszFilename = ", lpszFilename, "");
    ver_dstring("\tlpszWinDir = ", lpszWinDir, "");
    ver_dstring("\tlpszAppDir = ", lpszAppDir, "");

    TRACE(ver, "\tlpszCurDir = %p\n", lpszCurDir);
    if(lpuCurDirLen)
	TRACE(ver, "\tlpuCurDirLen = %p (%u)\n",
		    lpuCurDirLen, *lpuCurDirLen);
    else
	TRACE(ver, "\tlpuCurDirLen = (null)\n");

    TRACE(ver, "\tlpszDestDir = %p\n", lpszDestDir);
    if(lpuDestDirLen)
	TRACE(ver, "\tlpuDestDirLen = %p (%u)\n",
		    lpuDestDirLen, *lpuDestDirLen);

    /* Figure out where the file should go; shared files default to the
       system directory */

    strcpy(curDir, "");
    strcpy(destDir, "");

    if(flags & VFFF_ISSHAREDFILE) {
	GetSystemDirectoryA(destDir, 256);

	/* Were we given a filename?  If so, try to find the file. */
	if(lpszFilename) {
	    if(testFileExistence(destDir, lpszFilename)) {
		strcpy(curDir, destDir);

		if(!testFileExclusiveExistence(destDir, lpszFilename))
		    retval |= VFF_FILEINUSE;
	    }
	    else if(lpszAppDir && testFileExistence(lpszAppDir,
						    lpszFilename)) {
		strcpy(curDir, lpszAppDir);
		retval |= VFF_CURNEDEST;

		if(!testFileExclusiveExistence(lpszAppDir, lpszFilename))
		    retval |= VFF_FILEINUSE;
	    }
	}
    }
    else if(!(flags & VFFF_ISSHAREDFILE)) { /* not a shared file */
	if(lpszAppDir) {
	    char  systemDir[256];
	    GetSystemDirectoryA(systemDir, 256);

	    strcpy(destDir, lpszAppDir);

	    if(lpszFilename) {
		if(testFileExistence(lpszAppDir, lpszFilename)) {
		    strcpy(curDir, lpszAppDir);

		    if(!testFileExclusiveExistence(lpszAppDir, lpszFilename))
			retval |= VFF_FILEINUSE;
		}
		else if(testFileExistence(systemDir, lpszFilename)) {
		    strcpy(curDir, systemDir);
		    retval |= VFF_CURNEDEST;

		    if(!testFileExclusiveExistence(systemDir, lpszFilename))
			retval |= VFF_FILEINUSE;
		}
	    }
	}
    }

    curDirSizeReq = strlen(curDir) + 1;
    destDirSizeReq = strlen(destDir) + 1;



    /* Make sure that the pointers to the size of the buffers are
       valid; if not, do NOTHING with that buffer.  If that pointer
       is valid, then make sure that the buffer pointer is valid, too! */

    if(lpuDestDirLen && lpszDestDir) {
	if(*lpuDestDirLen < destDirSizeReq) {
	    retval |= VFF_BUFFTOOSMALL;
	    if (*lpuDestDirLen) {
	    strncpy(lpszDestDir, destDir, *lpuDestDirLen - 1);
	    lpszDestDir[*lpuDestDirLen - 1] = '\0';
	}
	}
	else
	    strcpy(lpszDestDir, destDir);

	*lpuDestDirLen = destDirSizeReq;
    }
    
    if(lpuCurDirLen && lpszCurDir) {
	if(*lpuCurDirLen < curDirSizeReq) {
	    retval |= VFF_BUFFTOOSMALL;
	    if (*lpuCurDirLen) {
	    strncpy(lpszCurDir, curDir, *lpuCurDirLen - 1);
	    lpszCurDir[*lpuCurDirLen - 1] = '\0';
	    }
	}
	else
	    strcpy(lpszCurDir, curDir);

	*lpuCurDirLen = curDirSizeReq;
    }

    TRACE(ver, "ret = %lu (%s%s%s)\n", retval,
		 (retval & VFF_CURNEDEST) ? "VFF_CURNEDEST " : "",
		 (retval & VFF_FILEINUSE) ? "VFF_FILEINUSE " : "",
		 (retval & VFF_BUFFTOOSMALL) ? "VFF_BUFFTOOSMALL " : "");

    ver_dstring("\t(Exit) lpszCurDir = ", lpszCurDir, "");
    if(lpuCurDirLen)
	TRACE(ver, "\t(Exit) lpuCurDirLen = %p (%u)\n",
		    lpuCurDirLen, *lpuCurDirLen);
    else
	TRACE(ver, "\t(Exit) lpuCurDirLen = (null)\n");

    ver_dstring("\t(Exit) lpszDestDir = ", lpszDestDir, "");
    if(lpuDestDirLen)
	TRACE(ver, "\t(Exit) lpuDestDirLen = %p (%u)\n",
		    lpuDestDirLen, *lpuDestDirLen);

    return retval;
}

/* VerFindFile32W						[VERSION.6] */
DWORD WINAPI VerFindFileW(
	UINT flags,LPCWSTR filename,LPCWSTR windir,LPCWSTR appdir,
	LPWSTR curdir,UINT *pcurdirlen,LPWSTR destdir,UINT *pdestdirlen )
{
    UINT curdirlen, destdirlen;
    LPSTR wfn,wwd,wad,wdd,wcd;
    DWORD ret;

    wfn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    wwd = HEAP_strdupWtoA( GetProcessHeap(), 0, windir );
    wad = HEAP_strdupWtoA( GetProcessHeap(), 0, appdir );
    wcd = HeapAlloc( GetProcessHeap(), 0, *pcurdirlen );
    wdd = HeapAlloc( GetProcessHeap(), 0, *pdestdirlen );
    ret = VerFindFileA(flags,wfn,wwd,wad,wcd,&curdirlen,wdd,&destdirlen);
    lstrcpynAtoW(curdir,wcd,*pcurdirlen);
    lstrcpynAtoW(destdir,wdd,*pdestdirlen);
    *pcurdirlen = strlen(wcd);
    *pdestdirlen = strlen(wdd);
    HeapFree( GetProcessHeap(), 0, wfn );
    HeapFree( GetProcessHeap(), 0, wwd );
    HeapFree( GetProcessHeap(), 0, wad );
    HeapFree( GetProcessHeap(), 0, wcd );
    HeapFree( GetProcessHeap(), 0, wdd );
    return ret;
}

static LPBYTE
_fetch_versioninfo(LPSTR fn,VS_FIXEDFILEINFO **vffi) {
    DWORD	alloclen;
    LPBYTE	buf;
    DWORD	ret;

    alloclen = 1000;
    buf= xmalloc(alloclen);
    while (1) {
    	ret = GetFileVersionInfoA(fn,0,alloclen,buf);
	if (!ret) {
	    free(buf);
	    return 0;
	}
	if (alloclen<*(WORD*)buf) {
	    free(buf);
	    alloclen = *(WORD*)buf;
	    buf = xmalloc(alloclen);
	} else {
	    *vffi = (VS_FIXEDFILEINFO*)(buf+0x14);
	    if ((*vffi)->dwSignature == 0x004f0049) /* hack to detect unicode */
	    	*vffi = (VS_FIXEDFILEINFO*)(buf+0x28);
	    if ((*vffi)->dwSignature != VS_FFI_SIGNATURE)
	    	WARN(ver,"Bad VS_FIXEDFILEINFO signature 0x%08lx\n",(*vffi)->dwSignature);
	    return buf;
	}
    }
}

static DWORD
_error2vif(DWORD error) {
    switch (error) {
    case ERROR_ACCESS_DENIED:
    	return VIF_ACCESSVIOLATION;
    case ERROR_SHARING_VIOLATION:
    	return VIF_SHARINGVIOLATION;
    default:
    	return 0;
    }
}


/******************************************************************************
 * VerInstallFile32A [VERSION.7]
 */
DWORD WINAPI VerInstallFileA(
	UINT flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
 	LPCSTR destdir,LPCSTR curdir,LPSTR tmpfile,UINT *tmpfilelen )
{
    LPCSTR pdest;
    char	destfn[260],tmpfn[260],srcfn[260];
    HFILE	hfsrc,hfdst;
    DWORD	attr,ret,xret,tmplast;
    LPBYTE	buf1,buf2;
    OFSTRUCT	ofs;

    TRACE(ver,"(%x,%s,%s,%s,%s,%s,%p,%d)\n",
	    flags,srcfilename,destfilename,srcdir,destdir,curdir,tmpfile,*tmpfilelen
    );
    xret = 0;
    sprintf(srcfn,"%s\\%s",srcdir,srcfilename);
    if (!destdir || !*destdir) pdest = srcdir;
    else pdest = destdir;
    sprintf(destfn,"%s\\%s",pdest,destfilename);
    hfsrc=LZOpenFileA(srcfn,&ofs,OF_READ);
    if (hfsrc==HFILE_ERROR)
    	return VIF_CANNOTREADSRC;
    sprintf(tmpfn,"%s\\%s",pdest,destfilename);
    tmplast=strlen(pdest)+1;
    attr = GetFileAttributesA(tmpfn);
    if (attr!=-1) {
	if (attr & FILE_ATTRIBUTE_READONLY) {
	    LZClose(hfsrc);
	    return VIF_WRITEPROT;
	}
	/* FIXME: check if file currently in use and return VIF_FILEINUSE */
    }
    attr = -1;
    if (flags & VIFF_FORCEINSTALL) {
    	if (tmpfile[0]) {
	    sprintf(tmpfn,"%s\\%s",pdest,tmpfile);
	    tmplast = strlen(pdest)+1;
	    attr = GetFileAttributesA(tmpfn);
	    /* if it exists, it has been copied by the call before.
	     * we jump over the copy part... 
	     */
	}
    }
    if (attr == -1) {
    	char	*s;

	GetTempFileNameA(pdest,"ver",0,tmpfn); /* should not fail ... */
	s=strrchr(tmpfn,'\\');
	if (s)
	    tmplast = s-tmpfn;
	else
	    tmplast = 0;
	hfdst = OpenFile(tmpfn,&ofs,OF_CREATE);
	if (hfdst == HFILE_ERROR) {
	    LZClose(hfsrc);
	    return VIF_CANNOTCREATE; /* | translated dos error */
	}
	ret = LZCopy(hfsrc,hfdst);
	_lclose(hfdst);
	if (((long) ret) < 0) {
	    /* translate LZ errors into VIF_xxx */
	    switch (ret) {
	    case LZERROR_BADINHANDLE:
	    case LZERROR_READ:
	    case LZERROR_BADVALUE:
	    case LZERROR_UNKNOWNALG:
		ret = VIF_CANNOTREADSRC;
		break;
	    case LZERROR_BADOUTHANDLE:
	    case LZERROR_WRITE:
		ret = VIF_OUTOFMEMORY; /* FIXME: correct? */
		break;
	    case LZERROR_GLOBALLOC:
	    case LZERROR_GLOBLOCK:
		ret = VIF_OUTOFSPACE;
		break;
	    default: /* unknown error, should not happen */
		ret = 0;
		break;
	    }
	    if (ret) {
		LZClose(hfsrc);
		return ret;
	    }
	}
    }
    xret = 0;
    if (!(flags & VIFF_FORCEINSTALL)) {
	VS_FIXEDFILEINFO *destvffi,*tmpvffi;
    	buf1 = _fetch_versioninfo(destfn,&destvffi);
	if (buf1) {
	    buf2 = _fetch_versioninfo(tmpfn,&tmpvffi);
	    if (buf2) {
	    	char	*tbuf1,*tbuf2;
		UINT	len1,len2;

		len1=len2=40;

		/* compare file versions */
		if ((destvffi->dwFileVersionMS > tmpvffi->dwFileVersionMS)||
		    ((destvffi->dwFileVersionMS==tmpvffi->dwFileVersionMS)&&
		     (destvffi->dwFileVersionLS > tmpvffi->dwFileVersionLS)
		    )
		)
		    xret |= VIF_MISMATCH|VIF_SRCOLD;
		/* compare filetypes and filesubtypes */
		if ((destvffi->dwFileType!=tmpvffi->dwFileType) ||
		    (destvffi->dwFileSubtype!=tmpvffi->dwFileSubtype)
		)
		    xret |= VIF_MISMATCH|VIF_DIFFTYPE;
		if (VerQueryValueA(buf1,"\\VarFileInfo\\Translation",(LPVOID*)&tbuf1,&len1) &&
		    VerQueryValueA(buf2,"\\VarFileInfo\\Translation",(LPVOID*)&tbuf2,&len2)
		) {
		    /* irgendwas mit tbuf1 und tbuf2 machen 
		     * generiert DIFFLANG|MISMATCH
		     */
		}
		free(buf2);
	    } else
		xret=VIF_MISMATCH|VIF_SRCOLD;
	    free(buf1);
	}
    }
    if (xret) {
	if (*tmpfilelen<strlen(tmpfn+tmplast)) {
	    xret|=VIF_BUFFTOOSMALL;
	    DeleteFileA(tmpfn);
	} else {
	    strcpy(tmpfile,tmpfn+tmplast);
	    *tmpfilelen = strlen(tmpfn+tmplast)+1;
	    xret|=VIF_TEMPFILE;
	}
    } else {
    	if (-1!=GetFileAttributesA(destfn))
	    if (!DeleteFileA(destfn)) {
		xret|=_error2vif(GetLastError())|VIF_CANNOTDELETE;
		DeleteFileA(tmpfn);
		LZClose(hfsrc);
		return xret;
	    }
	if ((!(flags & VIFF_DONTDELETEOLD))	&& 
	    curdir				&& 
	    *curdir				&&
	    lstrcmpiA(curdir,pdest)
	) {
	    char curfn[260];

	    sprintf(curfn,"%s\\%s",curdir,destfilename);
	    if (-1!=GetFileAttributesA(curfn)) {
		/* FIXME: check if in use ... if it is, VIF_CANNOTDELETECUR */
		if (!DeleteFileA(curfn))
	    	    xret|=_error2vif(GetLastError())|VIF_CANNOTDELETECUR;
	    }
	}
	if (!MoveFileA(tmpfn,destfn)) {
	    xret|=_error2vif(GetLastError())|VIF_CANNOTRENAME;
	    DeleteFileA(tmpfn);
	}
    }
    LZClose(hfsrc);
    return xret;
}


/* VerInstallFile32W				[VERSION.8] */
DWORD WINAPI VerInstallFileW(
	UINT flags,LPCWSTR srcfilename,LPCWSTR destfilename,LPCWSTR srcdir,
	LPCWSTR destdir,LPCWSTR curdir,LPWSTR tmpfile,UINT *tmpfilelen )
{
    LPSTR wsrcf,wsrcd,wdestf,wdestd,wtmpf,wcurd;
    DWORD ret;

    wsrcf  = HEAP_strdupWtoA( GetProcessHeap(), 0, srcfilename );
    wsrcd  = HEAP_strdupWtoA( GetProcessHeap(), 0, srcdir );
    wdestf = HEAP_strdupWtoA( GetProcessHeap(), 0, destfilename );
    wdestd = HEAP_strdupWtoA( GetProcessHeap(), 0, destdir );
    wtmpf  = HEAP_strdupWtoA( GetProcessHeap(), 0, tmpfile );
    wcurd  = HEAP_strdupWtoA( GetProcessHeap(), 0, curdir );
    ret = VerInstallFileA(flags,wsrcf,wdestf,wsrcd,wdestd,wcurd,wtmpf,tmpfilelen);
    if (!ret)
    	lstrcpynAtoW(tmpfile,wtmpf,*tmpfilelen);
    HeapFree( GetProcessHeap(), 0, wsrcf );
    HeapFree( GetProcessHeap(), 0, wsrcd );
    HeapFree( GetProcessHeap(), 0, wdestf );
    HeapFree( GetProcessHeap(), 0, wdestd );
    HeapFree( GetProcessHeap(), 0, wtmpf );
    if (wcurd) 
    	HeapFree( GetProcessHeap(), 0, wcurd );
    return ret;
}

