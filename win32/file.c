/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis, Sven Verdoolaege, and Cameron Heide
 */

#include <errno.h>
#include <stdio.h>
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
#include "handle32.h"
#include "string32.h"
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
    HFILE hfile;
    FILEMAP_OBJECT *filemap_obj;

    dprintf_win32(stddeb,"CreateFileMapping32A(%08x,%p,%ld,%ld,%ld,%s)\n",
    	h,ats,pot,sh,hlow,lpName
    );
    if (sh) {
        SetLastError(ErrnoToLastError(errno));
        return INVALID_HANDLE_VALUE;
    }
    filemap_obj=(FILEMAP_OBJECT *)CreateKernelObject(sizeof(FILEMAP_OBJECT));
    if(filemap_obj == NULL) {
        SetLastError(ERROR_UNKNOWN);
        return 0;
    }
    if (h==INVALID_HANDLE_VALUE)
    	h=_lcreat(lpName,1);/*FIXME*/

    filemap_obj->common.magic = KERNEL_OBJECT_FILEMAP;
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
    HANDLE32	res;
    LPSTR	aname = STRING32_DupUniToAnsi(lpName);

    res = CreateFileMapping32A(h,ats,pot,sh,hlow,aname);
    free(aname);
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
 *           GetFileInformationByHandle       (KERNEL32.219)
 */
DWORD GetFileInformationByHandle(HFILE hFile,BY_HANDLE_FILE_INFORMATION *lpfi)
{
    struct stat file_stat;
    int	rc;
    int	unixfd = FILE_GetUnixHandle(hFile);

    if (unixfd==-1)
    	return 0;
    rc = fstat(unixfd,&file_stat);
    if(rc == -1) {
        SetLastError(ErrnoToLastError(errno));
        return 0;
    }

    /* Translate the file attributes.
     */
    lpfi->dwFileAttributes = 0;
    if(file_stat.st_mode & S_IFREG)
        lpfi->dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
    if(file_stat.st_mode & S_IFDIR)
        lpfi->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    if((file_stat.st_mode & S_IWRITE) == 0)
        lpfi->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

    /* Translate the file times.  Use the last modification time
     * for both the creation time and write time.
     */
    DOSFS_UnixTimeToFileTime(file_stat.st_mtime, &(lpfi->ftCreationTime));
    DOSFS_UnixTimeToFileTime(file_stat.st_mtime, &(lpfi->ftLastWriteTime));
    DOSFS_UnixTimeToFileTime(file_stat.st_atime, &(lpfi->ftLastAccessTime));

    lpfi->nFileSizeLow = file_stat.st_size;
    lpfi->nNumberOfLinks = file_stat.st_nlink;
    lpfi->nFileIndexLow = file_stat.st_ino;

    /* Zero out currently unused fields.
     */
    lpfi->dwVolumeSerialNumber = 0;
    lpfi->nFileSizeHigh = 0;
    lpfi->nFileIndexHigh = 0;

    return 1;
}

/***********************************************************************
 *           GetFileSize             (KERNEL32.220)
 */
DWORD GetFileSize(HFILE hf,LPDWORD filesizehigh) {
    BY_HANDLE_FILE_INFORMATION	fi;
	DWORD	res = GetFileInformationByHandle(hf,&fi);

	if (!res) 
		return 0;	/* FIXME: correct ? */
	if (filesizehigh) 
		*filesizehigh = fi.nFileSizeHigh;
	return fi.nFileSizeLow;
}

/***********************************************************************
 *           GetStdHandle             (KERNEL32.276)
 * FIXME: We should probably allocate filehandles for stdin/stdout/stderr
 *	  at task creation (with HFILE-handle 0,1,2 respectively) and
 *	  return them here. Or at least look, if we created them already.
 */
HFILE GetStdHandle(DWORD nStdHandle)
{
    HFILE hfile;
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
            return HFILE_ERROR;
    }
    hfile = FILE_DupUnixHandle(unixfd);
    if (hfile == HFILE_ERROR)
    	return HFILE_ERROR;
    FILE_SetFileType( hfile, FILE_TYPE_CHAR );
    return hfile;
}


/***********************************************************************
 *              SetFilePointer          (KERNEL32.492)
 *
 * Luckily enough, this function maps almost directly into an lseek
 * call, the exception being the use of 64-bit offsets.
 */
DWORD SetFilePointer(HFILE hFile, LONG distance, LONG *highword,
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

    rc = _llseek(hFile, distance, method);
    if(rc == -1)
        SetLastError(ErrnoToLastError(errno));
    return rc;
}

/***********************************************************************
 *             WriteFile               (KERNEL32.578)
 */
BOOL32 WriteFile(HFILE hFile, LPVOID lpBuffer, DWORD numberOfBytesToWrite,
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
BOOL32 ReadFile(HFILE hFile, LPVOID lpBuffer, DWORD numtoread,
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
HFILE CreateFile32A(LPCSTR filename, DWORD access, DWORD sharing,
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
        return HFILE_ERROR;
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
HFILE CreateFile32W(LPCWSTR filename, DWORD access, DWORD sharing,
                    LPSECURITY_ATTRIBUTES security, DWORD creation,
                    DWORD attributes, HANDLE32 template)
{
    HFILE 	res;
    LPSTR	afn = STRING32_DupUniToAnsi(filename);

    res = CreateFile32A(afn,access,sharing,security,creation,attributes,template);
    free(afn);
    return res;
}

/*************************************************************************
 *              SetHandleCount32   (KERNEL32.494)
 */
UINT32 SetHandleCount32( UINT32 cHandles )
{
    return SetHandleCount16(cHandles);
}


int CloseFileHandle(HFILE hFile)
{
    if (!_lclose(hFile))
	return TRUE;
    return FALSE;
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
 *              GetFileAttributes32A	(KERNEL32.217)
 */
DWORD GetFileAttributes32A(LPCSTR lpFileName)
{
	struct stat buf;
	DWORD	res=0;
	char	*fn;

	dprintf_file(stddeb,"GetFileAttributesA(%s)\n",lpFileName);
	fn=(LPSTR)DOSFS_GetUnixFileName(lpFileName,FALSE);
	/* fn points to a static buffer, don't free it */
	if(stat(fn,&buf)==-1) {
		SetLastError(ErrnoToLastError(errno));
		return 0xFFFFFFFF;
	}
	if(buf.st_mode & S_IFREG)
		res |= FILE_ATTRIBUTE_NORMAL;
	if(buf.st_mode & S_IFDIR)
		res |= FILE_ATTRIBUTE_DIRECTORY;
	if((buf.st_mode & S_IWRITE) == 0)
		res |= FILE_ATTRIBUTE_READONLY;
	return res;
}

/**************************************************************************
 *              GetFileAttributes32W	(KERNEL32.218)
 */
DWORD GetFileAttributes32W(LPCWSTR lpFileName)
{
	LPSTR	afn = STRING32_DupUniToAnsi(lpFileName);
	DWORD	res;

	res = GetFileAttributes32A(afn);
	free(afn);
	return res;
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
	LPSTR afn = STRING32_DupUniToAnsi(lpFileName);
	BOOL32	res;

	res = SetFileAttributes32A(afn,attributes);
	free(afn);
	return res;
}

/**************************************************************************
 *              SetEndOfFile		(KERNEL32.483)
 */
BOOL32 SetEndOfFile(HFILE hFile)
{
	int	unixfd = FILE_GetUnixHandle(hFile);

	dprintf_file(stddeb,"SetEndOfFile(%lx)\n",(LONG)hFile);
	if (!unixfd)
		return 0;
	if (-1==ftruncate(unixfd,lseek(unixfd,0,SEEK_CUR))) {
            SetLastError(ErrnoToLastError(errno));
	    return FALSE;
	}
	return TRUE;
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
	LPSTR	afn1 = STRING32_DupUniToAnsi(fn1);
	LPSTR	afn2 = STRING32_DupUniToAnsi(fn2);
	BOOL32	res;

	res = MoveFile32A(afn1,afn2);
	free(afn1);
	free(afn2);
	return res;
}
