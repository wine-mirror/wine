/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis, Sven Verdoolaege, and Cameron Heide
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include "windows.h"
#include "winbase.h"
#include "winerror.h"
#include "file.h"
#include "heap.h"
#include "handle32.h"
#include "dos_fs.h"
#include "xmalloc.h"
#include "stddebug.h"
#define DEBUG_WIN32
#include "debug.h"


static int TranslateCreationFlags(DWORD create_flags);
static int TranslateAccessFlags(DWORD access_flags);
int TranslateProtectionFlags(DWORD);
#ifndef MAP_ANON
#define MAP_ANON 0
#endif

typedef struct {
    HFILE32		hfile;
    int			prot;
    unsigned long	size;
} FILEMAP_OBJECT;

/***********************************************************************
 *           OpenFileMappingA             (KERNEL32.397)
 * FIXME: stub
 *
 */
HANDLE32 OpenFileMapping(DWORD access, BOOL inherit,const char *fname)
{
	return 0;
}


/***********************************************************************
 *           CreateFileMapping32A   (KERNEL32.46)
 */
HANDLE32 CreateFileMapping32A(HANDLE32 h,LPSECURITY_ATTRIBUTES ats,
  DWORD pot,  DWORD sh,  DWORD hlow,  LPCSTR lpName )
{
    FILEMAP_OBJECT *filemap_obj;

    dprintf_win32(stddeb,"CreateFileMapping32A(%08x,%p,%ld,%ld,%ld,%s)\n",
    	h,ats,pot,sh,hlow,lpName
    );
    if (sh) {
        SetLastError(ErrnoToLastError(errno));
        return INVALID_HANDLE_VALUE32;
    }
    filemap_obj=(FILEMAP_OBJECT *)CreateKernelObject(sizeof(FILEMAP_OBJECT));
    if(filemap_obj == NULL) {
        SetLastError(ERROR_UNKNOWN);
        return 0;
    }
    if (h==INVALID_HANDLE_VALUE32)
    	h=_lcreat32(lpName,1);/*FIXME*/

    filemap_obj->hfile = h;
    filemap_obj->prot = TranslateProtectionFlags(pot);
    filemap_obj->size = hlow;
    return (HANDLE32)filemap_obj;;
}

/***********************************************************************
 *           CreateFileMapping32W   (KERNEL32.47)
 *
 */
HANDLE32 CreateFileMapping32W(HANDLE32 h,LPSECURITY_ATTRIBUTES ats,
  DWORD pot,  DWORD sh,  DWORD hlow,  LPCWSTR lpName)
{
    LPSTR aname = HEAP_strdupWtoA( GetProcessHeap(), 0, lpName );
    HANDLE32 res = CreateFileMapping32A(h,ats,pot,sh,hlow,aname);
    HeapFree( GetProcessHeap(), 0, aname );
    return res;
}


/***********************************************************************
 *           MapViewOfFile                  (KERNEL32.385)
 */
LPVOID MapViewOfFile(HANDLE32 handle, DWORD access, DWORD offhi,
                      DWORD offlo, DWORD size)
{
    return MapViewOfFileEx(handle,access,offhi,offlo,size,0);
}

/***********************************************************************
 *           MapViewOfFileEx                  (KERNEL32.386)
 *
 */
LPVOID MapViewOfFileEx(HANDLE32 handle, DWORD access, DWORD offhi,
                      DWORD offlo, DWORD size, DWORD st)
{
    FILEMAP_OBJECT *fmap = (FILEMAP_OBJECT*)handle;

    if (!size) size = fmap->size;
    if (!size) size = 1;
    return mmap ((caddr_t)st, size, fmap->prot, 
                 MAP_ANON|MAP_PRIVATE, 
		 FILE_GetUnixHandle(fmap->hfile),
		 offlo);
}

/***********************************************************************
 *           UnmapViewOfFile                  (KERNEL32.385)
 */
BOOL32 UnmapViewOfFile(LPVOID address) {
    munmap(address,/*hmm*/1); /* FIXME: size? */
    return TRUE;
}



/***********************************************************************
 *           GetStdHandle             (KERNEL32.276)
 * FIXME: We should probably allocate filehandles for stdin/stdout/stderr
 *	  at task creation (with HFILE-handle 0,1,2 respectively) and
 *	  return them here. Or at least look, if we created them already.
 */
HFILE32 GetStdHandle(DWORD nStdHandle)
{
    HFILE32 hfile;
    int	unixfd;

    switch(nStdHandle)
    {
        case STD_INPUT_HANDLE:
	    unixfd = 0;
            break;

        case STD_OUTPUT_HANDLE:
	    unixfd = 1;
            break;

        case STD_ERROR_HANDLE:
	    unixfd = 2;
            break;
        default:
            SetLastError(ERROR_INVALID_HANDLE);
            return HFILE_ERROR32;
    }
    hfile = FILE_DupUnixHandle(unixfd);
    if (hfile == HFILE_ERROR32)
    	return HFILE_ERROR32;
    FILE_SetFileType( hfile, FILE_TYPE_CHAR );
    return hfile;
}


/***********************************************************************
 *              SetFilePointer          (KERNEL32.492)
 *
 * Luckily enough, this function maps almost directly into an lseek
 * call, the exception being the use of 64-bit offsets.
 */
DWORD SetFilePointer(HFILE32 hFile, LONG distance, LONG *highword,
                     DWORD method)
{
    LONG rc;
    if(highword != NULL)
    {
        if(*highword != 0)
        {
            dprintf_file(stddeb, "SetFilePointer: 64-bit offsets not yet supported.\n");
            return -1;
        }
    }

    rc = _llseek32(hFile, distance, method);
    if(rc == -1)
        SetLastError(ErrnoToLastError(errno));
    return rc;
}

/***********************************************************************
 *             WriteFile               (KERNEL32.578)
 */
BOOL32 WriteFile(HFILE32 hFile, LPVOID lpBuffer, DWORD numberOfBytesToWrite,
                 LPDWORD numberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    LONG	res;

    res = _lwrite32(hFile,lpBuffer,numberOfBytesToWrite);
    if (res==-1) {
    	SetLastError(ErrnoToLastError(errno));
    	return FALSE;
    }
    if(numberOfBytesWritten)
        *numberOfBytesWritten = res;
    return TRUE;
}

/***********************************************************************
 *              ReadFile                (KERNEL32.428)
 */
BOOL32 ReadFile(HFILE32 hFile, LPVOID lpBuffer, DWORD numtoread,
                LPDWORD numread, LPOVERLAPPED lpOverlapped)
{
    int actual_read;

    actual_read = _lread32(hFile,lpBuffer,numtoread);
    if(actual_read == -1) {
        SetLastError(ErrnoToLastError(errno));
        return FALSE;
    }
    if(numread)
        *numread = actual_read;

    return TRUE;
}


/*************************************************************************
 *              CreateFile32A              (KERNEL32.45)
 *
 * Doesn't support character devices, pipes, template files, or a
 * lot of the 'attributes' flags yet.
 */
HFILE32 CreateFile32A(LPCSTR filename, DWORD access, DWORD sharing,
                      LPSECURITY_ATTRIBUTES security, DWORD creation,
                      DWORD attributes, HANDLE32 template)
{
    int access_flags, create_flags;

    /* Translate the various flags to Unix-style.
     */
    access_flags = TranslateAccessFlags(access);
    create_flags = TranslateCreationFlags(creation);

    if(template)
        dprintf_file(stddeb, "CreateFile: template handles not supported.\n");

    /* If the name starts with '\\?' or '\\.', ignore the first 3 chars.
     */
    if(!strncmp(filename, "\\\\?", 3) || !strncmp(filename, "\\\\.", 3))
        filename += 3;

    /* If the name still starts with '\\', it's a UNC name.
     */
    if(!strncmp(filename, "\\\\", 2))
    {
        dprintf_file(stddeb, "CreateFile: UNC names not supported.\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return HFILE_ERROR32;
    }

    /* If the name is either CONIN$ or CONOUT$, give them stdin
     * or stdout, respectively.
     */
    if(!strcmp(filename, "CONIN$")) return GetStdHandle( STD_INPUT_HANDLE );
    if(!strcmp(filename, "CONOUT$")) return GetStdHandle( STD_OUTPUT_HANDLE );

    return FILE_Open( filename, access_flags | create_flags );
}


/*************************************************************************
 *              CreateFile32W              (KERNEL32.48)
 */
HFILE32 CreateFile32W(LPCWSTR filename, DWORD access, DWORD sharing,
                      LPSECURITY_ATTRIBUTES security, DWORD creation,
                      DWORD attributes, HANDLE32 template)
{
    LPSTR afn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    HFILE32 res = CreateFile32A( afn, access, sharing, security, creation,
                                 attributes, template );
    HeapFree( GetProcessHeap(), 0, afn );
    return res;
}

static int TranslateAccessFlags(DWORD access_flags)
{
    int rc = 0;

    switch(access_flags)
    {
        case GENERIC_READ:
            rc = O_RDONLY;
            break;

        case GENERIC_WRITE:
            rc = O_WRONLY;
            break;

        case (GENERIC_READ | GENERIC_WRITE):
            rc = O_RDWR;
            break;
    }

    return rc;
}

static int TranslateCreationFlags(DWORD create_flags)
{
    int rc = 0;

    switch(create_flags)
    {
        case CREATE_NEW:
            rc = O_CREAT | O_EXCL;
            break;

        case CREATE_ALWAYS:
            rc = O_CREAT | O_TRUNC;
            break;

        case OPEN_EXISTING:
            rc = 0;
            break;

        case OPEN_ALWAYS:
            rc = O_CREAT;
            break;

        case TRUNCATE_EXISTING:
            rc = O_TRUNC;
            break;
    }

    return rc;
}


/**************************************************************************
 *              SetFileAttributes16	(KERNEL.421)
 */
BOOL16 SetFileAttributes16( LPCSTR lpFileName, DWORD attributes )
{
    return SetFileAttributes32A( lpFileName, attributes );
}


/**************************************************************************
 *              SetFileAttributes32A	(KERNEL32.490)
 */
BOOL32 SetFileAttributes32A(LPCSTR lpFileName, DWORD attributes)
{
	struct stat buf;
	LPSTR	fn=(LPSTR)DOSFS_GetUnixFileName(lpFileName,FALSE);

	dprintf_file(stddeb,"SetFileAttributes(%s,%lx)\n",lpFileName,attributes);
	if(stat(fn,&buf)==-1) {
		SetLastError(ErrnoToLastError(errno));
		return FALSE;
	}
	if (attributes & FILE_ATTRIBUTE_READONLY) {
		buf.st_mode &= ~0222; /* octal!, clear write permission bits */
		attributes &= ~FILE_ATTRIBUTE_READONLY;
	}
	attributes &= ~(FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
	if (attributes)
		fprintf(stdnimp,"SetFileAttributesA(%s):%lx attribute(s) not implemented.\n",lpFileName,attributes);
	if (-1==chmod(fn,buf.st_mode)) {
		SetLastError(ErrnoToLastError(errno));
		return FALSE;
	}
	return TRUE;
}

/**************************************************************************
 *              SetFileAttributes32W	(KERNEL32.491)
 */
BOOL32 SetFileAttributes32W(LPCWSTR lpFileName, DWORD attributes)
{
    LPSTR afn = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFileName );
    BOOL32 res = SetFileAttributes32A( afn, attributes );
    HeapFree( GetProcessHeap(), 0, afn );
    return res;
}

/**************************************************************************
 *              MoveFileA		(KERNEL32.387)
 */
BOOL32 MoveFile32A(LPCSTR fn1,LPCSTR fn2)
{
	LPSTR	ufn1;
	LPSTR	ufn2;

	dprintf_file(stddeb,"MoveFileA(%s,%s)\n",fn1,fn2);
	ufn1 = (LPSTR)DOSFS_GetUnixFileName(fn1,FALSE);
	if (!ufn1) {
		SetLastError(ErrnoToLastError(ENOENT));
		return FALSE;
	}
	ufn1 = xstrdup(ufn1);
	ufn2 = (LPSTR)DOSFS_GetUnixFileName(fn2,FALSE);
	if (!ufn2) {
		SetLastError(ErrnoToLastError(ENOENT));
		return FALSE;
	}
	ufn2 = xstrdup(ufn2);
	if (-1==rename(ufn1,ufn2)) {
		SetLastError(ErrnoToLastError(errno));
		free(ufn1);
		free(ufn2);
		return FALSE;
	}
	free(ufn1);
	free(ufn2);
	return TRUE;
}

/**************************************************************************
 *              MoveFileW		(KERNEL32.390)
 */
BOOL32 MoveFile32W(LPCWSTR fn1,LPCWSTR fn2)
{
    LPSTR afn1 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn1 );
    LPSTR afn2 = HEAP_strdupWtoA( GetProcessHeap(), 0, fn2 );
    BOOL32 res = MoveFile32A( afn1, afn2 );
    HeapFree( GetProcessHeap(), 0, afn1 );
    HeapFree( GetProcessHeap(), 0, afn2 );
    return res;
}

VOID SetFileApisToOEM()
{
    fprintf(stdnimp,"SetFileApisToOEM(),stub!\n");
}

VOID SetFileApisToANSI()
{
    fprintf(stdnimp,"SetFileApisToANSI(),stub!\n");
}

BOOL32 AreFileApisANSI()
{
    fprintf(stdnimp,"AreFileApisANSI(),stub!\n");
    return TRUE;
}


BOOL32 CopyFile32A(LPCSTR sourcefn,LPCSTR destfn,BOOL32 failifexists)
{
	OFSTRUCT	of;
	HFILE32		hf1,hf2;
	char		buffer[2048];
	int		lastread,curlen;

	fprintf(stddeb,"CopyFile: %s -> %s\n",sourcefn,destfn);
	hf1 = OpenFile32(sourcefn,&of,OF_READ);
	if (hf1==HFILE_ERROR32) return TRUE;
	if (failifexists)
        {
		hf2 = OpenFile32(sourcefn,&of,OF_WRITE);
		if (hf2 != HFILE_ERROR32) return FALSE;
		_lclose32(hf2);
	}
	hf2 = OpenFile32(sourcefn,&of,OF_WRITE);
	if (hf2 == HFILE_ERROR32) return FALSE;
	curlen = 0;
	while ((lastread=_lread32(hf1,buffer,sizeof(buffer)))>0) {
		curlen=0;
		while (curlen<lastread) {
			int res = _lwrite32(hf2,buffer+curlen,lastread-curlen);
			if (res<=0) break;
			curlen+=res;
		}
	}
	_lclose32(hf1);
	_lclose32(hf2);
	return curlen > 0;
}

BOOL32
LockFile(
	HFILE32 hFile,DWORD dwFileOffsetLow,DWORD dwFileOffsetHigh,
	DWORD nNumberOfBytesToLockLow,DWORD nNumberOfBytesToLockHigh )
{
	fprintf(stdnimp,"LockFile(%d,0x%08lx%08lx,0x%08lx%08lx),stub!\n",
		hFile,dwFileOffsetHigh,dwFileOffsetLow,
		nNumberOfBytesToLockHigh,nNumberOfBytesToLockLow
	);
	return TRUE;
}

BOOL32
UnlockFile(
	HFILE32 hFile,DWORD dwFileOffsetLow,DWORD dwFileOffsetHigh,
	DWORD nNumberOfBytesToUnlockLow,DWORD nNumberOfBytesToUnlockHigh )
{
	fprintf(stdnimp,"UnlockFile(%d,0x%08lx%08lx,0x%08lx%08lx),stub!\n",
		hFile,dwFileOffsetHigh,dwFileOffsetLow,
		nNumberOfBytesToUnlockHigh,nNumberOfBytesToUnlockLow
	);
	return TRUE;
}
