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
#include "winerror.h"
#include "kernel32.h"
#include "handle32.h"
#include "dos_fs.h"
#include "stddebug.h"
#define DEBUG_WIN32
#include "debug.h"


extern FILE_OBJECT *hstdin;
extern FILE_OBJECT *hstdout;
extern FILE_OBJECT *hstderr;

static void UnixTimeToFileTime(time_t unix_time, FILETIME *filetime);
static int TranslateCreationFlags(DWORD create_flags);
static int TranslateAccessFlags(DWORD access_flags);

/***********************************************************************
 *           OpenFileMappingA             (KERNEL32.397)
 *
 */
WINAPI HANDLE32 OpenFileMapping(DWORD access, BOOL inherit,const char *fname)
{
	return 0;
}
/***********************************************************************
 *           CreateFileMappingA		(KERNEL32.46)
 *
 */
WINAPI HANDLE32 CreateFileMapping(HANDLE32 h,SECURITY_ATTRIBUTES *ats,
  DWORD pot,  DWORD sh,  DWORD hlow,  const char * lpName )
{
    FILE_OBJECT *file_obj;
    FILEMAP_OBJECT *filemap_obj;
    int fd;

    if (sh)
    {
        SetLastError(ErrnoToLastError(errno));
        return INVALID_HANDLE_VALUE;
    }
    fd = open(lpName, O_CREAT, 0666);
    #if 0
    fd = open(DOS_GetUnixFileName(lpName), 0, 0666);
    #endif
    if(fd == -1)
    {
        SetLastError(ErrnoToLastError(errno));
        return INVALID_HANDLE_VALUE;
    }
    file_obj = (FILE_OBJECT *)
	                 CreateKernelObject(sizeof(FILE_OBJECT));
    if(file_obj == NULL)
    {
        SetLastError(ERROR_UNKNOWN);
        return 0;
    }
    filemap_obj = (FILEMAP_OBJECT *)
	                 CreateKernelObject(sizeof(FILEMAP_OBJECT));
    if(filemap_obj == NULL)
    {
	ReleaseKernelObject(file_obj);
        SetLastError(ERROR_UNKNOWN);
        return 0;
    }
    file_obj->common.magic = KERNEL_OBJECT_FILE;
    file_obj->fd = fd;
    file_obj->type = FILE_TYPE_DISK;
    filemap_obj->common.magic = KERNEL_OBJECT_FILEMAP;
    filemap_obj->file_obj = file_obj;
    filemap_obj->prot = TranslateProtectionFlags(pot);
    filemap_obj->size = hlow;
    return (HANDLE32)filemap_obj;;
}

/***********************************************************************
 *           MapViewOfFileEx                  (KERNEL32.386)
 *
 */
WINAPI void *MapViewOfFileEx(HANDLE32 handle, DWORD access, DWORD offhi,
                             DWORD offlo, DWORD size, DWORD st)
{
    if (!size) size = ((FILEMAP_OBJECT *)handle)->size;
    return mmap (st, size, ((FILEMAP_OBJECT *)handle)->prot, 
                 MAP_ANON|MAP_PRIVATE, 
		 ((FILEMAP_OBJECT *)handle)->file_obj->fd,
		 offlo);
}

/***********************************************************************
 *           GetFileInformationByHandle       (KERNEL32.219)
 *
 */
DWORD WINAPI GetFileInformationByHandle(FILE_OBJECT *hFile, 
                                        BY_HANDLE_FILE_INFORMATION *lpfi)
{
  struct stat file_stat;
    int rc;

    if(ValidateKernelObject((HANDLE32)hFile) != 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    if(hFile->common.magic != KERNEL_OBJECT_FILE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    rc = fstat(hFile->fd, &file_stat);
    if(rc == -1)
    {
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
    if(file_stat.st_mode & S_IWRITE == 0)
        lpfi->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

    /* Translate the file times.  Use the last modification time
     * for both the creation time and write time.
     */
    UnixTimeToFileTime(file_stat.st_mtime, &(lpfi->ftCreationTime));
    UnixTimeToFileTime(file_stat.st_mtime, &(lpfi->ftLastWriteTime));
    UnixTimeToFileTime(file_stat.st_atime, &(lpfi->ftLastAccessTime));

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


static void UnixTimeToFileTime(time_t unix_time, FILETIME *filetime)
{
    /* This isn't anywhere close to being correct, but should
     * work for now.
     */
    filetime->dwLowDateTime  = (unix_time & 0x0000FFFF) << 16;
    filetime->dwHighDateTime = (unix_time & 0xFFFF0000) >> 16;
}


/***********************************************************************
 *           GetFileType              (KERNEL32.222)
 *
 * GetFileType currently only supports stdin, stdout, and stderr, which
 * are considered to be of type FILE_TYPE_CHAR.
 */
DWORD GetFileType(FILE_OBJECT *hFile)
{
    if(ValidateKernelObject((HANDLE32)hFile) != 0)
    {
        SetLastError(ERROR_UNKNOWN);
        return FILE_TYPE_UNKNOWN;
    }
    if(hFile->common.magic != KERNEL_OBJECT_FILE)
    {
        SetLastError(ERROR_UNKNOWN);
        return FILE_TYPE_UNKNOWN;
    }

    return hFile->type;
}

/***********************************************************************
 *           GetStdHandle             (KERNEL32.276)
 */
HANDLE32 GetStdHandle(DWORD nStdHandle)
{
    HANDLE32 rc;

    switch(nStdHandle)
    {
        case STD_INPUT_HANDLE:
            rc = (HANDLE32)hstdin;
            break;

        case STD_OUTPUT_HANDLE:
            rc = (HANDLE32)hstdout;
            break;

        case STD_ERROR_HANDLE:
            rc = (HANDLE32)hstderr;
            break;

        default:
            rc = INVALID_HANDLE_VALUE;
            SetLastError(ERROR_INVALID_HANDLE);
            break;
    }

    return rc;
}

/***********************************************************************
 *              SetFilePointer          (KERNEL32.492)
 *
 * Luckily enough, this function maps almost directly into an lseek
 * call, the exception being the use of 64-bit offsets.
 */
DWORD SetFilePointer(FILE_OBJECT *hFile, LONG distance, LONG *highword,
                     DWORD method)
{
    int rc;

    if(ValidateKernelObject((HANDLE32)hFile) != 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return ((DWORD)0xFFFFFFFF);
    }
    if(hFile->common.magic != KERNEL_OBJECT_FILE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return ((DWORD)0xFFFFFFFF);
    }

    if(highword != NULL)
    {
        if(*highword != 0)
        {
            dprintf_win32(stddeb, "SetFilePointer: 64-bit offsets not yet supported.\n");
            return -1;
        }
    }

    rc = lseek(hFile->fd, distance, method);
    if(rc == -1)
        SetLastError(ErrnoToLastError(errno));
    return rc;
}

/***********************************************************************
 *             WriteFile               (KERNEL32.578)
 */
BOOL WriteFile(FILE_OBJECT *hFile, LPVOID lpBuffer, DWORD numberOfBytesToWrite,
               LPDWORD numberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    int written;

    if(ValidateKernelObject((HANDLE32)hFile) != 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    if(hFile->common.magic != KERNEL_OBJECT_FILE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    written = write(hFile->fd, lpBuffer, numberOfBytesToWrite);
    if(numberOfBytesWritten)
        *numberOfBytesWritten = written;

    return 1;
}

/***********************************************************************
 *              ReadFile                (KERNEL32.428)
 */
BOOL ReadFile(FILE_OBJECT *hFile, LPVOID lpBuffer, DWORD numtoread,
              LPDWORD numread, LPOVERLAPPED lpOverlapped)
{
    int actual_read;

    if(ValidateKernelObject((HANDLE32)hFile) != 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
    if(hFile->common.magic != KERNEL_OBJECT_FILE)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    actual_read = read(hFile->fd, lpBuffer, numtoread);
    if(actual_read == -1)
    {
        SetLastError(ErrnoToLastError(errno));
        return 0;
    }
    if(numread)
        *numread = actual_read;

    return 1;
}

/*************************************************************************
 *              CreateFile              (KERNEL32.45)
 *
 * Doesn't support character devices, pipes, template files, or a
 * lot of the 'attributes' flags yet.
 */
HANDLE32 CreateFileA(LPSTR filename, DWORD access, DWORD sharing,
                     LPSECURITY_ATTRIBUTES security, DWORD creation,
                     DWORD attributes, HANDLE32 template)
{
    int access_flags, create_flags;
    int fd;
    FILE_OBJECT *file_obj;
    int type;

    /* Translate the various flags to Unix-style.
     */
    access_flags = TranslateAccessFlags(access);
    create_flags = TranslateCreationFlags(creation);

    if(template)
        dprintf_win32(stddeb, "CreateFile: template handles not supported.\n");

    /* If the name starts with '\\?' or '\\.', ignore the first 3 chars.
     */
    if(!strncmp(filename, "\\\\?", 3) || !strncmp(filename, "\\\\.", 3))
        filename += 3;

    /* If the name still starts with '\\', it's a UNC name.
     */
    if(!strncmp(filename, "\\\\", 2))
    {
        dprintf_win32(stddeb, "CreateFile: UNC names not supported.\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return INVALID_HANDLE_VALUE;
    }

    /* If the name is either CONIN$ or CONOUT$, give them stdin
     * or stdout, respectively.
     */
    if(!strcmp(filename, "CONIN$"))
    {
        type = FILE_TYPE_CHAR;
        fd = 0;
    }
    else if(!strcmp(filename, "CONOUT$"))
    {
        type = FILE_TYPE_CHAR;
        fd = 1;
    }
    else
    {
        type = FILE_TYPE_DISK;

        /* Try to open the file.
         */
        fd = open(DOS_GetUnixFileName(filename),
                  access_flags | create_flags, 0666);
        if(fd == -1)
        {
            SetLastError(ErrnoToLastError(errno));
            return INVALID_HANDLE_VALUE;
        }
    }

    /* We seem to have succeeded, so allocate a kernel object
     * and set it up.
     */
    file_obj = (FILE_OBJECT *)CreateKernelObject(sizeof(FILE_OBJECT));
    if(file_obj == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }
    file_obj->common.magic = KERNEL_OBJECT_FILE;
    file_obj->fd = fd;
    file_obj->type = type;
    file_obj->misc_flags = attributes;
    file_obj->access_flags = access_flags;
    file_obj->create_flags = create_flags;

    return (HANDLE32)file_obj;
}

int CloseFileHandle(FILE_OBJECT *hFile)
{
    /* If it's one of the 3 standard handles, don't really
     * close it.
     */
    if(hFile->fd > 2)
        close(hFile->fd);

    return 1;
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
