/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis, Sven Verdoolaege, and Cameron Heide
 */

#include <errno.h>
#include <sys/errno.h>
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

