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
#include "xmalloc.h"
#include "stddebug.h"
#define DEBUG_WIN32
#include "debug.h"


static int TranslateCreationFlags(DWORD create_flags);
static int TranslateAccessFlags(DWORD access_flags);

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
    DOS_FULL_NAME full_name;

    if (!DOSFS_GetFullName( lpFileName, TRUE, &full_name ))
        return FALSE;

    dprintf_file(stddeb,"SetFileAttributes(%s,%lx)\n",lpFileName,attributes);
    if(stat(full_name.long_name,&buf)==-1)
    {
        SetLastError(ErrnoToLastError(errno));
        return FALSE;
    }
    if (attributes & FILE_ATTRIBUTE_READONLY)
    {
        buf.st_mode &= ~0222; /* octal!, clear write permission bits */
        attributes &= ~FILE_ATTRIBUTE_READONLY;
    }
    attributes &= ~(FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM);
    if (attributes)
        fprintf(stdnimp,"SetFileAttributesA(%s):%lx attribute(s) not implemented.\n",lpFileName,attributes);
    if (-1==chmod(full_name.long_name,buf.st_mode))
    {
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
