/* 
 * Implementation of VER.DLL
 * 
 * Copyright 1996 Marcus Meissner
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "windows.h"
#include "win.h"
#include "winerror.h"
#include "heap.h"
#include "ver.h"
#include "lzexpand.h"
#include "module.h"
#include "neexe.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "winreg.h"

#define LZREAD(what) \
  if (sizeof(*what)!=LZRead32(lzfd,what,sizeof(*what))) return 0;
#define LZTELL(lzfd) LZSeek32(lzfd, 0, SEEK_CUR);

/******************************************************************************
 *
 *   void  ver_dstring(
 *      char const * prologue,
 *      char const * teststring,
 *      char const * epilogue )
 *
 *   This function will print via dprintf_ver to stddeb the prologue string,
 *   followed by the address of teststring and the string it contains if
 *   teststring is non-null or "(null)" otherwise, and then the epilogue
 *   string.
 *
 *   Revision history
 *      30-May-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *         Original implementation as dprintf[_]ver_string
 *      05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *         Fixed problem that caused bug with tools/make_debug -- renaming
 *         this function should fix the problem.
 *
 *****************************************************************************/

static void  ver_dstring(
    char const * prologue,
    char const * teststring,
    char const * epilogue )
{
    dprintf_ver(stddeb, "%s", prologue);
    
    if(teststring)
	dprintf_ver(stddeb, "%p (\"%s\")", (void const *) teststring,
		    teststring);
    else
	dprintf_ver(stddeb, "(null)");

    dprintf_ver(stddeb, "%s", epilogue);

    return;
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

    if(OpenFile32(filename, &fileinfo, OF_EXIST) == HFILE_ERROR32)
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

    if(OpenFile32(filename, &fileinfo, OF_EXIST | OF_SHARE_EXCLUSIVE) ==
       HFILE_ERROR32)
	retval = 0;
    else
	retval = 1;

    return retval;
}


int
read_ne_header(HFILE32 lzfd,LPIMAGE_OS2_HEADER nehd) {
	IMAGE_DOS_HEADER	mzh;

	LZSeek32(lzfd,0,SEEK_SET);
	if (sizeof(mzh)!=LZRead32(lzfd,&mzh,sizeof(mzh)))
		return 0;
	if (mzh.e_magic!=IMAGE_DOS_SIGNATURE)
		return 0;
	LZSeek32(lzfd,mzh.e_lfanew,SEEK_SET);
	LZREAD(nehd);
	if (nehd->ne_magic == IMAGE_OS2_SIGNATURE) {
		LZSeek32(lzfd,mzh.e_lfanew,SEEK_SET);
		return 1;
	}
	fprintf(stderr,"misc/ver.c:read_ne_header:can't handle PE files yet.\n");
	/* must handle PE files too. Later. */
	return 0;
}


int
find_ne_resource(
	HFILE32 lzfd,LPIMAGE_OS2_HEADER nehd,SEGPTR typeid,SEGPTR resid,
	BYTE **resdata,int *reslen,DWORD *off
) {
	NE_TYPEINFO	ti;
	NE_NAMEINFO	ni;
	int		i;
	WORD		shiftcount;
	DWORD		nehdoffset;

	nehdoffset = LZTELL(lzfd);
	LZSeek32(lzfd,nehd->resource_tab_offset,SEEK_CUR);
	LZREAD(&shiftcount);
	dprintf_ver(stddeb,"shiftcount is %d\n",shiftcount);
	dprintf_ver(stddeb,"reading resource typeinfo dir.\n");

	if (!HIWORD(typeid)) typeid = (SEGPTR)(LOWORD(typeid) | 0x8000);
	if (!HIWORD(resid))  resid  = (SEGPTR)(LOWORD(resid) | 0x8000);
	while (1) {
		int	skipflag;

		LZREAD(&ti);
		if (!ti.type_id)
			return 0;
		dprintf_ver(stddeb,"    ti.typeid =%04x,count=%d\n",ti.type_id,ti.count);
		skipflag=0;
		if (!HIWORD(typeid)) {
			if ((ti.type_id&0x8000)&&(typeid!=ti.type_id))
				skipflag=1;
		} else {
			if (ti.type_id & 0x8000) {
				skipflag=1; 
			} else {
				BYTE	len;
				char	*str;
				DWORD	whereleft;

				whereleft = LZTELL(lzfd);
				LZSeek32(
					lzfd,
					nehdoffset+nehd->resource_tab_offset+ti.type_id,
					SEEK_SET
				);
				LZREAD(&len);
				str=xmalloc(len);
				if (len!=LZRead32(lzfd,str,len))
					return 0;
				dprintf_ver(stddeb,"read %s to compare it with %s\n",
					str,(char*)PTR_SEG_TO_LIN(typeid)
				);
				if (lstrcmpi32A(str,(char*)PTR_SEG_TO_LIN(typeid)))
					skipflag=1;
				free(str);
				LZSeek32(lzfd,whereleft,SEEK_SET);
			}
		}
		if (skipflag) {
			LZSeek32(lzfd,ti.count*sizeof(ni),SEEK_CUR);
			continue;
		}
		for (i=0;i<ti.count;i++) {
			WORD	*rdata;
			int	len;

			LZREAD(&ni);
			dprintf_ver(stddeb,"	ni.id=%4x,offset=%d,length=%d\n",
				ni.id,ni.offset,ni.length
			);
			skipflag=1;
			if (!HIWORD(resid)) {
				if (ni.id == resid)
					skipflag=0;
			} else {
				if (!(ni.id & 0x8000)) {
					BYTE	len;
					char	*str;
					DWORD	whereleft;

					whereleft = LZTELL(lzfd);
					  LZSeek32(
						lzfd,
						nehdoffset+nehd->resource_tab_offset+ni.id,
						SEEK_SET
					);
					LZREAD(&len);
					str=xmalloc(len);
					if (len!=LZRead32(lzfd,str,len))
						return 0;
					dprintf_ver(stddeb,"read %s to compare it with %s\n",
						str,(char*)PTR_SEG_TO_LIN(typeid)
					);
					if (!lstrcmpi32A(str,(char*)PTR_SEG_TO_LIN(typeid)))
						skipflag=0;
					free(str);
					LZSeek32(lzfd,whereleft,SEEK_SET);
				}
			}
			if (skipflag)
				continue;
			LZSeek32(lzfd,((int)ni.offset<<shiftcount),SEEK_SET);
			*off	= (int)ni.offset<<shiftcount;
			len	= ni.length<<shiftcount;
			rdata=(WORD*)xmalloc(len);
			if (len!=LZRead32(lzfd,rdata,len)) {
				free(rdata);
				return 0;
			}
			dprintf_ver(stddeb,"resource found.\n");
			*resdata= (BYTE*)rdata;
			*reslen	= len;
			return 1;
		}
	}
}

/* GetFileResourceSize				[VER.2] */
DWORD
GetFileResourceSize(LPCSTR filename,SEGPTR restype,SEGPTR resid,LPDWORD off) {
	HFILE32			lzfd;
	OFSTRUCT		ofs;
	BYTE			*resdata;
	int			reslen;
	IMAGE_OS2_HEADER	nehd;

	dprintf_ver(stddeb,"GetFileResourceSize(%s,%lx,%lx,%p)\n",
		filename,(LONG)restype,(LONG)resid,off
	);
	lzfd=LZOpenFile32A(filename,&ofs,OF_READ);
	if (lzfd==0)
		return 0;
	if (!read_ne_header(lzfd,&nehd)) {
		LZClose32(lzfd);
		return 0;
	}
	if (!find_ne_resource(lzfd,&nehd,restype,resid,&resdata,&reslen,off)) {
		LZClose32(lzfd);
		return 0;
	}
	free(resdata);
	LZClose32(lzfd);
	return reslen;
}

/* GetFileResource				[VER.3] */
DWORD
GetFileResource(LPCSTR filename,SEGPTR restype,SEGPTR resid,
		DWORD off,DWORD datalen,LPVOID data
) {
	HFILE32			lzfd;
	OFSTRUCT		ofs;
	BYTE			*resdata;
	int			reslen=datalen;
	IMAGE_OS2_HEADER	nehd;
	dprintf_ver(stddeb,"GetFileResource(%s,%lx,%lx,%ld,%ld,%p)\n",
		filename,(LONG)restype,(LONG)resid,off,datalen,data
	);

	lzfd=LZOpenFile32A(filename,&ofs,OF_READ);
	if (lzfd==0)
		return 0;
	if (!off) {
		if (!read_ne_header(lzfd,&nehd)) {
			LZClose32(lzfd);
			return 0;
		}
		if (!find_ne_resource(lzfd,&nehd,restype,resid,&resdata,&reslen,&off)) {
			LZClose32(lzfd);
			return 0;
		}
		free(resdata);
	}
	LZSeek32(lzfd,off,SEEK_SET);
	if (reslen>datalen)
		reslen=datalen;
	LZRead32(lzfd,data,reslen);
	LZClose32(lzfd);
	return reslen;
}

/* GetFileVersionInfoSize			[VER.6] */
DWORD
GetFileVersionInfoSize16(LPCSTR filename,LPDWORD handle) {
	DWORD	len,ret;
	BYTE	buf[72];
	VS_FIXEDFILEINFO *vffi;

	dprintf_ver(stddeb,"GetFileVersionInfoSize16(%s,%p)\n",filename,handle);
	len=GetFileResourceSize(filename,VS_FILE_INFO,VS_VERSION_INFO,handle);
	if (!len)
		return 0;
	ret=GetFileResource(
		filename,VS_FILE_INFO,VS_VERSION_INFO,*handle,sizeof(buf),buf
	);
	if (!ret)
		return 0;

	vffi=(VS_FIXEDFILEINFO*)(buf+0x14);
	if (vffi->dwSignature != VS_FFI_SIGNATURE)
		return 0;
	if (*(WORD*)buf < len)
		len = *(WORD*)buf;
	dprintf_ver(stddeb,"->strucver=%ld.%ld,filever=%ld.%ld,productver=%ld.%ld,flagmask=%lx,flags=%lx,OS=",
		(vffi->dwStrucVersion>>16),vffi->dwStrucVersion&0xFFFF,
		vffi->dwFileVersionMS,vffi->dwFileVersionLS,
		vffi->dwProductVersionMS,vffi->dwProductVersionLS,
		vffi->dwFileFlagsMask,vffi->dwFileFlags
	);
	switch (vffi->dwFileOS&0xFFFF0000) {
	case VOS_DOS:dprintf_ver(stddeb,"DOS,");break;
	case VOS_OS216:dprintf_ver(stddeb,"OS/2-16,");break;
	case VOS_OS232:dprintf_ver(stddeb,"OS/2-32,");break;
	case VOS_NT:dprintf_ver(stddeb,"NT,");break;
	case VOS_UNKNOWN:
	default:
		dprintf_ver(stddeb,"UNKNOWN(%ld),",vffi->dwFileOS&0xFFFF0000);break;
	}
	switch (vffi->dwFileOS & 0xFFFF) {
	case VOS__BASE:dprintf_ver(stddeb,"BASE");break;
	case VOS__WINDOWS16:dprintf_ver(stddeb,"WIN16");break;
	case VOS__WINDOWS32:dprintf_ver(stddeb,"WIN32");break;
	case VOS__PM16:dprintf_ver(stddeb,"PM16");break;
	case VOS__PM32:dprintf_ver(stddeb,"PM32");break;
	default:dprintf_ver(stddeb,"UNKNOWN(%ld)",vffi->dwFileOS&0xFFFF);break;
	}
	switch (vffi->dwFileType) {
	default:
	case VFT_UNKNOWN:
		dprintf_ver(stddeb,"filetype=Unknown(%ld)",vffi->dwFileType);
		break;
	case VFT_APP:dprintf_ver(stddeb,"filetype=APP");break;
	case VFT_DLL:dprintf_ver(stddeb,"filetype=DLL");break;
	case VFT_DRV:
		dprintf_ver(stddeb,"filetype=DRV,");
		switch(vffi->dwFileSubtype) {
		default:
		case VFT2_UNKNOWN:
			dprintf_ver(stddeb,"UNKNOWN(%ld)",vffi->dwFileSubtype);
			break;
		case VFT2_DRV_PRINTER:
			dprintf_ver(stddeb,"PRINTER");
			break;
		case VFT2_DRV_KEYBOARD:
			dprintf_ver(stddeb,"KEYBOARD");
			break;
		case VFT2_DRV_LANGUAGE:
			dprintf_ver(stddeb,"LANGUAGE");
			break;
		case VFT2_DRV_DISPLAY:
			dprintf_ver(stddeb,"DISPLAY");
			break;
		case VFT2_DRV_MOUSE:
			dprintf_ver(stddeb,"MOUSE");
			break;
		case VFT2_DRV_NETWORK:
			dprintf_ver(stddeb,"NETWORK");
			break;
		case VFT2_DRV_SYSTEM:
			dprintf_ver(stddeb,"SYSTEM");
			break;
		case VFT2_DRV_INSTALLABLE:
			dprintf_ver(stddeb,"INSTALLABLE");
			break;
		case VFT2_DRV_SOUND:
			dprintf_ver(stddeb,"SOUND");
			break;
		case VFT2_DRV_COMM:
			dprintf_ver(stddeb,"COMM");
			break;
		case VFT2_DRV_INPUTMETHOD:
			dprintf_ver(stddeb,"INPUTMETHOD");
			break;
		}
		break;
	case VFT_FONT:
		dprintf_ver(stddeb,"filetype=FONT.");
		switch (vffi->dwFileSubtype) {
		default:
			dprintf_ver(stddeb,"UNKNOWN(%ld)",vffi->dwFileSubtype);
			break;
		case VFT2_FONT_RASTER:dprintf_ver(stddeb,"RASTER");break;
		case VFT2_FONT_VECTOR:dprintf_ver(stddeb,"VECTOR");break;
		case VFT2_FONT_TRUETYPE:dprintf_ver(stddeb,"TRUETYPE");break;
		}
		break;
	case VFT_VXD:dprintf_ver(stddeb,"filetype=VXD");break;
	case VFT_STATIC_LIB:dprintf_ver(stddeb,"filetype=STATIC_LIB");break;
	}
	dprintf_ver(stddeb,"filedata=%lx.%lx\n",vffi->dwFileDateMS,vffi->dwFileDateLS);
	return len;
}

/* GetFileVersionInfoSize32A			[VERSION.1] */
DWORD
GetFileVersionInfoSize32A(LPCSTR filename,LPDWORD handle) {
	dprintf_ver(stddeb,"GetFileVersionInfoSize32A(%s,%p)\n",filename,handle);
	return GetFileVersionInfoSize16(filename,handle);
}

/* GetFileVersionInfoSize32W			[VERSION.2] */
DWORD GetFileVersionInfoSize32W( LPCWSTR filename, LPDWORD handle )
{
    LPSTR xfn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    DWORD ret = GetFileVersionInfoSize16( xfn, handle );
    HeapFree( GetProcessHeap(), 0, xfn );
    return ret;
}

/* GetFileVersionInfo				[VER.7] */
DWORD 
GetFileVersionInfo16(LPCSTR filename,DWORD handle,DWORD datasize,LPVOID data) {
	dprintf_ver(stddeb,"GetFileVersionInfo16(%s,%ld,%ld,%p)\n->",
		filename,handle,datasize,data
	);
	return GetFileResource(
		filename,VS_FILE_INFO,VS_VERSION_INFO,handle,datasize,data
	);
}

/* GetFileVersionInfoA				[VERSION.0] */
DWORD 
GetFileVersionInfo32A(LPCSTR filename,DWORD handle,DWORD datasize,LPVOID data) {
	return GetFileVersionInfo16(filename,handle,datasize,data);
}

/* GetFileVersionInfoW				[VERSION.3] */
DWORD GetFileVersionInfo32W( LPCWSTR filename, DWORD handle, DWORD datasize,
                             LPVOID data)
{
    LPSTR fn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    DWORD ret = GetFileVersionInfo16( fn, handle, datasize, data );
    HeapFree( GetProcessHeap(), 0, fn );
    return ret;
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

DWORD VerFindFile16(
    UINT16 flags,
    LPCSTR lpszFilename,
    LPCSTR lpszWinDir,
    LPCSTR lpszAppDir,
    LPSTR lpszCurDir,
    UINT16 *lpuCurDirLen,
    LPSTR lpszDestDir,
    UINT16 *lpuDestDirLen )
{
    DWORD  retval;
    char  curDir[256];
    char  destDir[256];
    unsigned int  curDirSizeReq;
    unsigned int  destDirSizeReq;

    retval = 0;

    /* Print out debugging information */
    dprintf_ver(stddeb, "VerFindFile() called with parameters:\n"
		"\tflags = %x", flags);
    if(flags & VFFF_ISSHAREDFILE)
	dprintf_ver(stddeb, " (VFFF_ISSHAREDFILE)\n");
    else
	dprintf_ver(stddeb, "\n");

    ver_dstring("\tlpszFilename = ", lpszFilename, "\n");
    ver_dstring("\tlpszWinDir = ", lpszWinDir, "\n");
    ver_dstring("\tlpszAppDir = ", lpszAppDir, "\n");

    dprintf_ver(stddeb, "\tlpszCurDir = %p\n", lpszCurDir);
    if(lpuCurDirLen)
	dprintf_ver(stddeb, "\tlpuCurDirLen = %p (%u)\n",
		    lpuCurDirLen, *lpuCurDirLen);
    else
	dprintf_ver(stddeb, "\tlpuCurDirLen = (null)\n");

    dprintf_ver(stddeb, "\tlpszDestDir = %p\n", lpszDestDir);
    if(lpuDestDirLen)
	dprintf_ver(stddeb, "\tlpuDestDirLen = %p (%u)\n",
		    lpuDestDirLen, *lpuDestDirLen);

    /* Figure out where the file should go; shared files default to the
       system directory */

    strcpy(curDir, "");
    strcpy(destDir, "");

    if(flags & VFFF_ISSHAREDFILE && !getuid()) {
	GetSystemDirectory32A(destDir, 256);

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
	    GetSystemDirectory32A(systemDir, 256);

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
	    strncpy(lpszDestDir, destDir, *lpuDestDirLen - 1);
	    lpszDestDir[*lpuDestDirLen - 1] = '\0';
	}
	else
	    strcpy(lpszDestDir, destDir);

	*lpuDestDirLen = destDirSizeReq;
    }
    
    if(lpuCurDirLen && lpszCurDir) {
	if(*lpuCurDirLen < curDirSizeReq) {
	    retval |= VFF_BUFFTOOSMALL;
	    strncpy(lpszCurDir, curDir, *lpuCurDirLen - 1);
	    lpszCurDir[*lpuCurDirLen - 1] = '\0';
	}
	else
	    strcpy(lpszCurDir, curDir);

	*lpuCurDirLen = curDirSizeReq;
    }

    dprintf_ver(stddeb, "VerFindFile() ret = %lu ",
		retval);

    if(retval) {
	dprintf_ver(stddeb, "( ");

	if(retval & VFF_CURNEDEST)
	    dprintf_ver(stddeb, "VFF_CURNEDEST ");
	if(retval & VFF_FILEINUSE)
	    dprintf_ver(stddeb, "VFF_FILEINUSE ");
	if(retval & VFF_BUFFTOOSMALL)
	    dprintf_ver(stddeb, "VFF_BUFFTOOSMALL ");

	dprintf_ver(stddeb, ")");
    }

    ver_dstring("\n\t(Exit) lpszCurDir = ", lpszCurDir, "\n");
    if(lpuCurDirLen)
	dprintf_ver(stddeb, "\t(Exit) lpuCurDirLen = %p (%u)\n",
		    lpuCurDirLen, *lpuCurDirLen);
    else
	dprintf_ver(stddeb, "\t(Exit) lpuCurDirLen = (null)\n");

    ver_dstring("\t(Exit) lpszDestDir = ", lpszDestDir, "\n");
    if(lpuDestDirLen)
	dprintf_ver(stddeb, "\t(Exit) lpuDestDirLen = %p (%u)\n",
		    lpuDestDirLen, *lpuDestDirLen);

    return retval;
}

/* VerFindFileA						[VERSION.5] */
DWORD
VerFindFile32A(
	UINT32 flags,LPCSTR filename,LPCSTR windir,LPCSTR appdir,
	LPSTR curdir,UINT32 *pcurdirlen,LPSTR destdir,UINT32 *pdestdirlen )
{
    UINT16 curdirlen, destdirlen;
    DWORD ret = VerFindFile16(flags,filename,windir,appdir,
                              curdir,&curdirlen,destdir,&destdirlen);
    *pcurdirlen = curdirlen;
    *pdestdirlen = destdirlen;
    return ret;
}

/* VerFindFileW						[VERSION.6] */
DWORD
VerFindFile32W(
	UINT32 flags,LPCWSTR filename,LPCWSTR windir,LPCWSTR appdir,
	LPWSTR curdir,UINT32 *pcurdirlen,LPWSTR destdir,UINT32 *pdestdirlen )
{
    UINT16 curdirlen, destdirlen;
    LPSTR wfn,wwd,wad,wdd,wcd;
    DWORD ret;

    wfn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    wwd = HEAP_strdupWtoA( GetProcessHeap(), 0, windir );
    wad = HEAP_strdupWtoA( GetProcessHeap(), 0, appdir );
    wcd = HeapAlloc( GetProcessHeap(), 0, *pcurdirlen );
    wdd = HeapAlloc( GetProcessHeap(), 0, *pdestdirlen );
    ret = VerFindFile16(flags,wfn,wwd,wad,wcd,&curdirlen,wdd,&destdirlen);
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

/* VerInstallFile					[VER.9] */
DWORD
VerInstallFile16(
	UINT16 flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
 	LPCSTR destdir,LPCSTR curdir,LPSTR tmpfile,UINT16 *tmpfilelen )
{
    UINT32	filelen;
    DWORD ret= VerInstallFile32A(flags,srcfilename,destfilename,srcdir,
                                 destdir,curdir,tmpfile,&filelen);

    *tmpfilelen = filelen;
    return ret;
}

/* VerInstallFileA				[VERSION.7] */
static LPBYTE
_fetch_versioninfo(LPSTR fn) {
    DWORD	alloclen;
    LPBYTE	buf;
    DWORD	ret;

    alloclen = 1000;
    buf= xmalloc(alloclen);
    while (1) {
    	ret = GetFileVersionInfo32A(fn,0,alloclen,buf);
	if (!ret) {
	    free(buf);
	    return 0;
	}
	if (alloclen<*(WORD*)buf) {
	    free(buf);
	    alloclen = *(WORD*)buf;
	    buf = xmalloc(alloclen);
	} else
	    return buf;
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

DWORD
VerInstallFile32A(
	UINT32 flags,LPCSTR srcfilename,LPCSTR destfilename,LPCSTR srcdir,
 	LPCSTR destdir,LPCSTR curdir,LPSTR tmpfile,UINT32 *tmpfilelen )
{
    char	destfn[260],tmpfn[260],srcfn[260];
    HFILE32	hfsrc,hfdst;
    DWORD	attr,ret,xret,tmplast;
    LPBYTE	buf1,buf2;
    OFSTRUCT	ofs;

    fprintf(stddeb,"VerInstallFile(%x,%s,%s,%s,%s,%s,%p,%d)\n",
	    flags,srcfilename,destfilename,srcdir,destdir,curdir,tmpfile,*tmpfilelen
    );
    xret = 0;
    sprintf(srcfn,"%s\\%s",srcdir,srcfilename);
    sprintf(destfn,"%s\\%s",destdir,destfilename);
    hfsrc=LZOpenFile32A(srcfn,&ofs,OF_READ);
    if (hfsrc==HFILE_ERROR32)
    	return VIF_CANNOTREADSRC;
    sprintf(tmpfn,"%s\\%s",destdir,destfilename);
    tmplast=strlen(destdir)+1;
    attr = GetFileAttributes32A(tmpfn);
    if (attr!=-1) {
	if (attr & FILE_ATTRIBUTE_READONLY) {
	    LZClose32(hfsrc);
	    return VIF_WRITEPROT;
	}
	/* FIXME: check if file currently in use and return VIF_FILEINUSE */
    }
    attr = -1;
    if (flags & VIFF_FORCEINSTALL) {
    	if (tmpfile[0]) {
	    sprintf(tmpfn,"%s\\%s",destdir,tmpfile);
	    tmplast = strlen(destdir)+1;
	    attr = GetFileAttributes32A(tmpfn);
	    /* if it exists, it has been copied by the call before.
	     * we jump over the copy part... 
	     */
	}
    }
    if (attr == -1) {
    	char	*s;

	GetTempFileName32A(destdir,"ver",0,tmpfn); /* should not fail ... */
	s=strrchr(tmpfn,'\\');
	if (s)
	    tmplast = s-tmpfn;
	else
	    tmplast = 0;
	hfdst = OpenFile32(tmpfn,&ofs,OF_CREATE);
	if (hfdst == HFILE_ERROR32) {
	    LZClose32(hfsrc);
	    return VIF_CANNOTCREATE; /* | translated dos error */
	}
	ret = LZCopy32(hfsrc,hfdst);
	_lclose32(hfdst);
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
		LZClose32(hfsrc);
		return ret;
	    }
	}
    }
    xret = 0;
    if (!(flags & VIFF_FORCEINSTALL)) {
    	buf1 = _fetch_versioninfo(destfn);
	if (buf1) {
	    buf2 = _fetch_versioninfo(tmpfn);
	    if (buf2) {
	    	char	*tbuf1,*tbuf2;
		VS_FIXEDFILEINFO *destvffi,*tmpvffi;
		UINT32	len1,len2;

		destvffi= (VS_FIXEDFILEINFO*)(buf1+0x14);
		tmpvffi = (VS_FIXEDFILEINFO*)(buf2+0x14);
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
		if (VerQueryValue32A(buf1,"\\VarFileInfo\\Translation",(LPVOID*)&tbuf1,&len1) &&
		    VerQueryValue32A(buf2,"\\VarFileInfo\\Translation",(LPVOID*)&tbuf2,&len2)
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
	    xret|=VIF_BUFTOSMALL;
	    DeleteFile32A(tmpfn);
	} else {
	    strcpy(tmpfile,tmpfn+tmplast);
	    *tmpfilelen = strlen(tmpfn+tmplast)+1;
	    xret|=VIF_TEMPFILE;
	}
    } else {
    	if (-1!=GetFileAttributes32A(destfn))
	    if (!DeleteFile32A(destfn)) {
		xret|=_error2vif(GetLastError())|VIF_CANNOTDELETE;
		DeleteFile32A(tmpfn);
		LZClose32(hfsrc);
		return xret;
	    }
	if ((!(flags & VIFF_DONTDELETEOLD))	&& 
	    curdir				&& 
	    *curdir				&&
	    lstrcmpi32A(curdir,destdir)
	) {
	    char curfn[260];

	    sprintf(curfn,"%s\\%s",curdir,destfilename);
	    if (-1!=GetFileAttributes32A(curfn)) {
		/* FIXME: check if in use ... if it is, VIF_CANNOTDELETECUR */
		if (!DeleteFile32A(curfn))
	    	    xret|=_error2vif(GetLastError())|VIF_CANNOTDELETECUR;
	    }
	}
	if (!MoveFile32A(tmpfn,destfn)) {
	    xret|=_error2vif(GetLastError())|VIF_CANNOTRENAME;
	    DeleteFile32A(tmpfn);
	}
    }
    LZClose32(hfsrc);
    return xret;
}

/* VerInstallFileW				[VERSION.8] */
DWORD
VerInstallFile32W(
	UINT32 flags,LPCWSTR srcfilename,LPCWSTR destfilename,LPCWSTR srcdir,
	LPCWSTR destdir,LPCWSTR curdir,LPWSTR tmpfile,UINT32 *tmpfilelen )
{
    LPSTR wsrcf,wsrcd,wdestf,wdestd,wtmpf,wcurd;
    DWORD ret;

    wsrcf  = HEAP_strdupWtoA( GetProcessHeap(), 0, srcfilename );
    wsrcd  = HEAP_strdupWtoA( GetProcessHeap(), 0, srcdir );
    wdestf = HEAP_strdupWtoA( GetProcessHeap(), 0, destfilename );
    wdestd = HEAP_strdupWtoA( GetProcessHeap(), 0, destdir );
    wtmpf  = HEAP_strdupWtoA( GetProcessHeap(), 0, tmpfile );
    wcurd  = HEAP_strdupWtoA( GetProcessHeap(), 0, curdir );
    ret = VerInstallFile32A(flags,wsrcf,wdestf,wsrcd,wdestd,wcurd,wtmpf,tmpfilelen);
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


/* FIXME: UNICODE? */
struct db {
	WORD	nextoff;
	WORD	datalen;
/* in memory structure... */
	char	name[1]; 	/* padded to dword alignment */
/* .... 
	char	data[datalen];     padded to dword alignemnt
	BYTE	subdirdata[];      until nextoff
 */
};

static BYTE*
_find_data(BYTE *block,LPCSTR str) {
	char	*nextslash;
	int	substrlen;
	struct	db	*db;

	while (*str && *str=='\\')
		str++;
	if (NULL!=(nextslash=strchr(str,'\\')))
		substrlen=nextslash-str;
	else
		substrlen=strlen(str);
	if (nextslash!=NULL) {
		while (*nextslash && *nextslash=='\\')
			nextslash++;
		if (!*nextslash)
			nextslash=NULL;
	}


	while (1) {
		db=(struct db*)block;
		dprintf_ver(stddeb,"db=%p,db->nextoff=%d,db->datalen=%d,db->name=%s,db->data=%s\n",
			db,db->nextoff,db->datalen,db->name,(char*)((char*)db+4+((strlen(db->name)+4)&~3))
		);
		if (!db->nextoff)
			return NULL;

		dprintf_ver(stddeb,"comparing with %s\n",db->name);
		if (!strncmp(db->name,str,substrlen)) {
			if (nextslash)
				return _find_data(
					block+4+((strlen(db->name)+4)&~3)+((db->datalen+3)&~3)
					,nextslash
				);
			else
				return block;
		}
		block=block+((db->nextoff+3)&~3);
	}
}

/* VerQueryValue 			[VER.11] */
/* take care, 'buffer' is NOT a SEGPTR, it just points to one */
DWORD
VerQueryValue16(SEGPTR segblock,LPCSTR subblock,SEGPTR *buffer,UINT16 *buflen)
{
	BYTE	*block=PTR_SEG_TO_LIN(segblock),*b;
	struct	db	*db;
	char	*s;

	dprintf_ver(stddeb,"VerQueryValue16(%p,%s,%p,%d)\n",
		block,subblock,buffer,*buflen
	);
	s=(char*)xmalloc(strlen("VS_VERSION_INFO\\")+strlen(subblock)+1);
	strcpy(s,"VS_VERSION_INFO\\");strcat(s,subblock);
	b=_find_data(block,s);
	if (b==NULL) {
		*buflen=0;
		return 0;
	}
	db=(struct db*)b;
	*buflen	= db->datalen;
	/* let b point to data area */
	b	= b+4+((strlen(db->name)+4)&~3);
	/* now look up what the resp. SEGPTR would be ... */
	*buffer	= (b-block)+segblock;
	dprintf_ver(stddeb,"	-> %s=%s\n",subblock,b);
	return 1;
}

DWORD
VerQueryValue32A(LPVOID vblock,LPCSTR subblock,LPVOID *vbuffer,UINT32 *buflen)
{
	BYTE	*b,*block=(LPBYTE)vblock,**buffer=(LPBYTE*)vbuffer;
	struct	db	*db;
	char	*s;

	dprintf_ver(stddeb,"VerQueryValue32A(%p,%s,%p,%d)\n",
		block,subblock,buffer,*buflen
	);
	s=(char*)xmalloc(strlen("VS_VERSION_INFO\\")+strlen(subblock)+1);
	strcpy(s,"VS_VERSION_INFO\\");strcat(s,subblock);
	b=_find_data(block,s);
	if (b==NULL) {
		*buflen=0;
		return 0;
	}
	db=(struct db*)b;
	*buflen	= db->datalen;
	/* let b point to data area */
	b	= b+4+((strlen(db->name)+4)&~3);
	*buffer	= b;
	dprintf_ver(stddeb,"	-> %s=%s\n",subblock,b);
	return 1;
}

DWORD
VerQueryValue32W(LPVOID vblock,LPCWSTR subblock,LPVOID *vbuffer,UINT32 *buflen)
{
	/* FIXME: hmm, we not only need to convert subblock, but also 
	 *        the content...or?
	 * And what about UNICODE version info?
	 * And the NAMES of the values?
	 */
	BYTE		*b,**buffer=(LPBYTE*)vbuffer,*block=(LPBYTE)vblock;
	struct	db	*db;
	char		*s,*sb;

	sb = HEAP_strdupWtoA( GetProcessHeap(), 0, subblock );
	s=(char*)xmalloc(strlen("VS_VERSION_INFO\\")+strlen(sb)+1);
	strcpy(s,"VS_VERSION_INFO\\");strcat(s,sb);
	b=_find_data(block,s);
	if (b==NULL) {
		*buflen=0;
		HeapFree( GetProcessHeap(), 0, sb );
		return 0;
	}
	db=(struct db*)b;
	*buflen	= db->datalen;
	/* let b point to data area */
	b	= b+4+((strlen(db->name)+4)&~3);
	*buffer	= b;
	dprintf_ver(stddeb,"	-> %s=%s\n",sb,b);
        HeapFree( GetProcessHeap(), 0, sb );
	return 1;
}
/* 20 GETFILEVERSIONINFORAW */
