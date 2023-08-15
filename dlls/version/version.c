/*
 * Implementation of VERSION.DLL
 *
 * Copyright 1996,1997 Marcus Meissner
 * Copyright 1997 David Cuthbert
 * Copyright 1999 Ulrich Weigand
 * Copyright 2005 Paul Vriens
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>

#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "winnls.h"
#include "lzexpand.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ver);

static LPBYTE
_fetch_versioninfo(LPSTR fn,VS_FIXEDFILEINFO **vffi) {
    DWORD	alloclen;
    LPBYTE	buf;
    DWORD	ret;

    alloclen = 1000;
    buf=HeapAlloc(GetProcessHeap(), 0, alloclen);
    if(buf == NULL) {
        WARN("Memory exhausted while fetching version info!\n");
        return NULL;
    }
    while (1) {
	ret = GetFileVersionInfoA(fn,0,alloclen,buf);
	if (!ret) {
	    HeapFree(GetProcessHeap(), 0, buf);
	    return NULL;
	}
	if (alloclen<*(WORD*)buf) {
	    alloclen = *(WORD*)buf;
	    HeapFree(GetProcessHeap(), 0, buf);
	    buf = HeapAlloc(GetProcessHeap(), 0, alloclen);
            if(buf == NULL) {
               WARN("Memory exhausted while fetching version info!\n");
               return NULL;
            }
	} else {
	    *vffi = (VS_FIXEDFILEINFO*)(buf+0x14);
	    if ((*vffi)->dwSignature == 0x004f0049) /* hack to detect unicode */
		*vffi = (VS_FIXEDFILEINFO*)(buf+0x28);
	    if ((*vffi)->dwSignature != VS_FFI_SIGNATURE)
                WARN("Bad VS_FIXEDFILEINFO signature 0x%08lx\n",(*vffi)->dwSignature);
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
 * VerInstallFileA [VERSION.@]
 */
DWORD WINAPI VerInstallFileA(
	DWORD flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
	LPCSTR destdir,LPCSTR curdir,LPSTR tmpfile,PUINT tmpfilelen )
{
    LPCSTR pdest;
    char	destfn[260],tmpfn[260],srcfn[260];
    HFILE	hfsrc,hfdst;
    DWORD	attr,xret,tmplast;
    LONG	ret;
    LPBYTE	buf1,buf2;
    OFSTRUCT	ofs;

    TRACE("(%lx,%s,%s,%s,%s,%s,%p,%d)\n",
          flags,debugstr_a(srcfilename),debugstr_a(destfilename),
          debugstr_a(srcdir),debugstr_a(destdir),debugstr_a(curdir),
          tmpfile,*tmpfilelen);
    xret = 0;
    if (!srcdir || !srcfilename) return VIF_CANNOTREADSRC;
    sprintf(srcfn,"%s\\%s",srcdir,srcfilename);
    if (!destdir || !*destdir) pdest = srcdir;
    else pdest = destdir;
    sprintf(destfn,"%s\\%s",pdest,destfilename);
    hfsrc=LZOpenFileA(srcfn,&ofs,OF_READ);
    if (hfsrc < 0)
	return VIF_CANNOTREADSRC;
    sprintf(tmpfn,"%s\\%s",pdest,destfilename);
    tmplast=strlen(pdest)+1;
    attr = GetFileAttributesA(tmpfn);
    if (attr != INVALID_FILE_ATTRIBUTES) {
	if (attr & FILE_ATTRIBUTE_READONLY) {
	    LZClose(hfsrc);
	    return VIF_WRITEPROT;
	}
	/* FIXME: check if file currently in use and return VIF_FILEINUSE */
    }
    attr = INVALID_FILE_ATTRIBUTES;
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
    if (attr == INVALID_FILE_ATTRIBUTES) {
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
	if (ret < 0) {
	    /* translate LZ errors into VIF_xxx */
	    switch (ret) {
	    case LZERROR_BADINHANDLE:
	    case LZERROR_READ:
	    case LZERROR_BADVALUE:
	    case LZERROR_UNKNOWNALG:
		xret = VIF_CANNOTREADSRC;
		break;
	    case LZERROR_BADOUTHANDLE:
	    case LZERROR_WRITE:
		xret = VIF_OUTOFSPACE;
		break;
	    case LZERROR_GLOBALLOC:
	    case LZERROR_GLOBLOCK:
		xret = VIF_OUTOFMEMORY;
		break;
	    default: /* unknown error, should not happen */
		FIXME("Unknown LZCopy error %ld, ignoring.\n", ret);
		xret = 0;
		break;
	    }
	    if (xret) {
		LZClose(hfsrc);
		return xret;
	    }
	}
    }
    if (!(flags & VIFF_FORCEINSTALL)) {
	VS_FIXEDFILEINFO *destvffi,*tmpvffi;
	buf1 = _fetch_versioninfo(destfn,&destvffi);
	if (buf1) {
	    buf2 = _fetch_versioninfo(tmpfn,&tmpvffi);
	    if (buf2) {
		char	*tbuf1,*tbuf2;
		static const CHAR trans_array[] = "\\VarFileInfo\\Translation";
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
		if (VerQueryValueA(buf1,trans_array,(LPVOID*)&tbuf1,&len1) &&
		    VerQueryValueA(buf2,trans_array,(LPVOID*)&tbuf2,&len2)
		) {
                    /* Do something with tbuf1 and tbuf2
		     * generates DIFFLANG|MISMATCH
		     */
		}
		HeapFree(GetProcessHeap(), 0, buf2);
	    } else
		xret=VIF_MISMATCH|VIF_SRCOLD;
	    HeapFree(GetProcessHeap(), 0, buf1);
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
	if (INVALID_FILE_ATTRIBUTES!=GetFileAttributesA(destfn))
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
	    if (INVALID_FILE_ATTRIBUTES != GetFileAttributesA(curfn)) {
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


/******************************************************************************
 * VerInstallFileW				[VERSION.@]
 */
DWORD WINAPI VerInstallFileW(
	DWORD flags,LPCWSTR srcfilename,LPCWSTR destfilename,LPCWSTR srcdir,
	LPCWSTR destdir,LPCWSTR curdir,LPWSTR tmpfile,PUINT tmpfilelen )
{
    LPSTR wsrcf = NULL, wsrcd = NULL, wdestf = NULL, wdestd = NULL, wtmpf = NULL, wcurd = NULL;
    DWORD ret = 0;
    UINT len;

    if (srcfilename)
    {
        len = WideCharToMultiByte( CP_ACP, 0, srcfilename, -1, NULL, 0, NULL, NULL );
        if ((wsrcf = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, srcfilename, -1, wsrcf, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (srcdir && !ret)
    {
        len = WideCharToMultiByte( CP_ACP, 0, srcdir, -1, NULL, 0, NULL, NULL );
        if ((wsrcd = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, srcdir, -1, wsrcd, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (destfilename && !ret)
    {
        len = WideCharToMultiByte( CP_ACP, 0, destfilename, -1, NULL, 0, NULL, NULL );
        if ((wdestf = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, destfilename, -1, wdestf, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (destdir && !ret)
    {
        len = WideCharToMultiByte( CP_ACP, 0, destdir, -1, NULL, 0, NULL, NULL );
        if ((wdestd = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, destdir, -1, wdestd, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (curdir && !ret)
    {
        len = WideCharToMultiByte( CP_ACP, 0, curdir, -1, NULL, 0, NULL, NULL );
        if ((wcurd = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, curdir, -1, wcurd, len, NULL, NULL );
        else
            ret = VIF_OUTOFMEMORY;
    }
    if (!ret)
    {
        len = *tmpfilelen * sizeof(WCHAR);
        wtmpf = HeapAlloc( GetProcessHeap(), 0, len );
        if (!wtmpf)
            ret = VIF_OUTOFMEMORY;
    }
    if (!ret)
        ret = VerInstallFileA(flags,wsrcf,wdestf,wsrcd,wdestd,wcurd,wtmpf,&len);
    if (!ret)
        *tmpfilelen = MultiByteToWideChar( CP_ACP, 0, wtmpf, -1, tmpfile, *tmpfilelen );
    else if (ret & VIF_BUFFTOOSMALL)
        *tmpfilelen = len;  /* FIXME: not correct */

    HeapFree( GetProcessHeap(), 0, wsrcf );
    HeapFree( GetProcessHeap(), 0, wsrcd );
    HeapFree( GetProcessHeap(), 0, wdestf );
    HeapFree( GetProcessHeap(), 0, wdestd );
    HeapFree( GetProcessHeap(), 0, wtmpf );
    HeapFree( GetProcessHeap(), 0, wcurd );
    return ret;
}
