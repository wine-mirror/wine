/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis, Sven Verdoolaege, and Cameron Heide
 */

#include <errno.h>
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
#include "device.h"
#include "process.h"
#include "heap.h"
#include "debug.h"

DWORD ErrnoToLastError(int errno_num);

static int TranslateCreationFlags(DWORD create_flags);
static int TranslateAccessFlags(DWORD access_flags);

/***********************************************************************
 *             WriteFile               (KERNEL32.578)
 */
BOOL32 WINAPI WriteFile(HANDLE32 hFile, LPCVOID lpBuffer, 
			DWORD numberOfBytesToWrite,
                        LPDWORD numberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
	K32OBJ *ioptr;
	BOOL32 status = FALSE;
	
	TRACE(file, "%d %p %ld\n", hFile, lpBuffer, 
		     numberOfBytesToWrite);
	
	if (!(ioptr = HANDLE_GetObjPtr( PROCESS_Current(), hFile,
                                        K32OBJ_UNKNOWN, 0, NULL ))) 
		return HFILE_ERROR32;
        if (K32OBJ_OPS(ioptr)->write)
            status = K32OBJ_OPS(ioptr)->write(ioptr, lpBuffer, numberOfBytesToWrite, 
                                              numberOfBytesWritten, lpOverlapped);
	K32OBJ_DecCount( ioptr );
	return status;
}

/***********************************************************************
 *              ReadFile                (KERNEL32.428)
 */
BOOL32 WINAPI ReadFile(HANDLE32 hFile, LPVOID lpBuffer, DWORD numberOfBytesToRead,
                       LPDWORD numberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	K32OBJ *ioptr;
	BOOL32 status = FALSE;
	
	TRACE(file, "%d %p %ld\n", hFile, lpBuffer, 
		     numberOfBytesToRead);
	
	if (!(ioptr = HANDLE_GetObjPtr( PROCESS_Current(), hFile,
                                        K32OBJ_UNKNOWN, 0, NULL ))) 
		return HFILE_ERROR32;
        if (K32OBJ_OPS(ioptr)->read)
            status = K32OBJ_OPS(ioptr)->read(ioptr, lpBuffer, numberOfBytesToRead, 
                                             numberOfBytesRead, lpOverlapped);
	K32OBJ_DecCount( ioptr );
	return status;
}


/***********************************************************************
 *              ReadFileEx                (KERNEL32.)
 */
typedef
VOID
(CALLBACK *LPOVERLAPPED_COMPLETION_ROUTINE)(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
    );

BOOL32 WINAPI ReadFileEx(HFILE32 hFile, LPVOID lpBuffer, DWORD numtoread,
			 LPOVERLAPPED lpOverlapped, 
			 LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{

    FIXME(file, "file %d to buf %p num %ld %p func %p stub\n",
	  hFile, lpBuffer, numtoread, lpOverlapped, lpCompletionRoutine);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*************************************************************************
 * CreateFile32A [KERNEL32.45]  Creates or opens a file or other object
 *
 * Creates or opens an object, and returns a handle that can be used to
 * access that object.
 *
 * PARAMS
 *
 * filename	[I] pointer to filename to be accessed
 * access	[I] access mode requested
 * sharing	[I] share mode
 * security	[I] pointer to security attributes
 * creation	[I] ?
 * attributes	[I] ?
 * template	[I] handle to file with attributes to copy
 *
 * RETURNS
 *   Success: Open handle to specified file
 *   Failure: INVALID_HANDLE_VALUE
 *
 * NOTES
 *  Should call SetLastError() on failure.
 *
 * BUGS
 *
 * Doesn't support character devices, pipes, template files, or a
 * lot of the 'attributes' flags yet.
 */
HFILE32 WINAPI CreateFile32A(LPCSTR filename, DWORD access, DWORD sharing,
                             LPSECURITY_ATTRIBUTES security, DWORD creation,
                             DWORD attributes, HANDLE32 template)
{
    int access_flags, create_flags;

    /* Translate the various flags to Unix-style.
     */
    access_flags = TranslateAccessFlags(access);
    create_flags = TranslateCreationFlags(creation);

    if(template)
        FIXME(file, "template handles not supported.\n");

    /* If the name starts with '\\?\' or '\\.\', ignore the first 4 chars.
     */
    if(!strncmp(filename, "\\\\?\\", 4) || !strncmp(filename, "\\\\.\\", 4))
    {
        if (filename[2] == '.')
        {
            FIXME(file,"device name? %s\n",filename);
            /* device? */
            return DEVICE_Open( filename+4, access_flags | create_flags );
        }
        filename += 4;
	if (!strncmp(filename, "UNC", 3))
	{
            FIXME(file, "UNC names not supported.\n");
            SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
            return HFILE_ERROR32;
	}	
    }

    /* If the name still starts with '\\', it's a UNC name.
     */
    if(!strncmp(filename, "\\\\", 2))
    {
        FIXME(file, "UNC names not supported.\n");
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
HFILE32 WINAPI CreateFile32W(LPCWSTR filename, DWORD access, DWORD sharing,
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
BOOL16 WINAPI SetFileAttributes16( LPCSTR lpFileName, DWORD attributes )
{
    return SetFileAttributes32A( lpFileName, attributes );
}


/**************************************************************************
 *              SetFileAttributes32A	(KERNEL32.490)
 */
BOOL32 WINAPI SetFileAttributes32A(LPCSTR lpFileName, DWORD attributes)
{
    struct stat buf;
    DOS_FULL_NAME full_name;

    if (!DOSFS_GetFullName( lpFileName, TRUE, &full_name ))
        return FALSE;

    TRACE(file,"(%s,%lx)\n",lpFileName,attributes);
    if (attributes & FILE_ATTRIBUTE_NORMAL) {
      attributes &= ~FILE_ATTRIBUTE_NORMAL;
      if (attributes)
        FIXME(file,"(%s):%lx illegal combination with FILE_ATTRIBUTE_NORMAL.\n",
	      lpFileName,attributes);
    }
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
        FIXME(file,"(%s):%lx attribute(s) not implemented.\n",
	      lpFileName,attributes);
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
BOOL32 WINAPI SetFileAttributes32W(LPCWSTR lpFileName, DWORD attributes)
{
    LPSTR afn = HEAP_strdupWtoA( GetProcessHeap(), 0, lpFileName );
    BOOL32 res = SetFileAttributes32A( afn, attributes );
    HeapFree( GetProcessHeap(), 0, afn );
    return res;
}


/**************************************************************************
 *              SetFileApisToOEM   (KERNEL32.645)
 */
VOID WINAPI SetFileApisToOEM(void)
{
  /*FIXME(file,"(): stub!\n");*/
}


/**************************************************************************
 *              SetFileApisToANSI   (KERNEL32.644)
 */
VOID WINAPI SetFileApisToANSI(void)
{
    /*FIXME(file,"(): stub\n");*/
}


/******************************************************************************
 * AreFileApisANSI [KERNEL32.105]  Determines if file functions are using ANSI
 *
 * RETURNS
 *    TRUE:  Set of file functions is using ANSI code page
 *    FALSE: Set of file functions is using OEM code page
 */
BOOL32 WINAPI AreFileApisANSI(void)
{
    FIXME(file,"(void): stub\n");
    return TRUE;
}

