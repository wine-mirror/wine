/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"

/***********************************************************************
 *           GetFileType              (KERNEL32.222)
 *
 * GetFileType currently only supports stdin, stdout, and stderr, which
 * are considered to be of type FILE_TYPE_CHAR.
 */
DWORD GetFileType(HANDLE hFile)
{
    DWORD file_type;

    if((DWORD)hFile < 3)
    {
        file_type = 2;        /* FILE_TYPE_CHAR */
    }
    else
    {
        file_type = 0;        /* FILE_TYPE_UNKNOWN */
    }

    return file_type;
}

/***********************************************************************
 *           GetStdHandle             (KERNEL32.276)
 */
HANDLE GetStdHandle(DWORD nStdHandle)
{
        switch(nStdHandle)
        {
                case -10/*STD_INPUT_HANDLE*/:return (HANDLE)0;
                case -11/*STD_OUTPUT_HANDLE*/:return (HANDLE)1;
                case -12/*STD_ERROR_HANDLE*/:return (HANDLE)2;
        }
        return (HANDLE)-1;
}

/***********************************************************************
 *              SetFilePointer          (KERNEL32.492)
 *
 * Luckily enough, this function maps almost directly into an lseek
 * call, the exception being the use of 64-bit offsets.
 */
DWORD SetFilePointer(HANDLE hFile, LONG distance, LONG *highword,
                     DWORD method)
{
    int rc;

    if(highword != NULL)
    {
        if(*highword != 0)
        {
            printf("SetFilePointer: 64-bit offsets not yet supported.\n");
            return -1;
        }
    }

    rc = lseek((DWORD)hFile, distance, method);
    if(rc == -1)
        SetLastError(ErrnoToLastError(errno));
    return rc;
}

/***********************************************************************
 *             WriteFile               (KERNEL32.578)
 *
 * WriteFile isn't very useful at this point since only standard
 * handles are permitted, but it lets us see runtime errors at least.
 */
BOOL WriteFile(HANDLE hFile, LPVOID lpBuffer, DWORD numberOfBytesToWrite,
              LPDWORD numberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    int written;

    if((DWORD)hFile < 3)
    {
        written = write((DWORD)hFile, lpBuffer, numberOfBytesToWrite);
        if(numberOfBytesWritten)
            *numberOfBytesWritten = written;
    }

    return 1;
}

