/*
 * CRTDLL drive/directory functions
 * 
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 *
 *
 * Implementation Notes:
 * MT Safe.
 */

#include "crtdll.h"
#include <errno.h>

#include "ntddk.h"
#include <time.h>

DEFAULT_DEBUG_CHANNEL(crtdll);

/* INTERNAL: Translate find_t to PWIN32_FIND_DATAA */
static void __CRTDLL__fttofd(LPWIN32_FIND_DATAA fd, find_t* ft)
{
  DWORD dw;

  /* Tested with crtdll.dll Version 2.50.4170 (NT) from win98 SE:
   * attrib 0x80 (FILE_ATTRIBUTE_NORMAL)is returned as 0.
   */
  if (fd->dwFileAttributes == FILE_ATTRIBUTE_NORMAL)
    ft->attrib = 0;
  else
    ft->attrib = fd->dwFileAttributes;

  RtlTimeToSecondsSince1970( &fd->ftCreationTime, &dw );
  ft->time_create = dw;
  RtlTimeToSecondsSince1970( &fd->ftLastAccessTime, &dw );
  ft->time_access = dw;
  RtlTimeToSecondsSince1970( &fd->ftLastWriteTime, &dw );
  ft->time_write = dw;
  ft->size = fd->nFileSizeLow;
  strcpy(ft->name, fd->cFileName);
}


/*********************************************************************
 *                  _chdir           (CRTDLL.51)
 *
 * Change the current directory.
 *
 * PARAMS
 *   newdir [in] Directory to change to
 *
 * RETURNS
 * Sucess:  0
 * 
 * Failure: -1
 */
INT __cdecl CRTDLL__chdir(LPCSTR newdir)
{
  if (!SetCurrentDirectoryA(newdir))
  {
    __CRTDLL__set_errno(newdir?GetLastError():0);
    return -1;
  }
  return 0;
}


/*********************************************************************
 *                  _chdrive           (CRTDLL.52)
 *
 * Change the current drive.
 *
 * PARAMS
 *   newdrive [in] new drive to change to, A: =1, B: =2, etc
 *
 * RETURNS
 * Sucess:  0
 * 
 * Failure: 1 
 */
BOOL __cdecl CRTDLL__chdrive(INT newdrive)
{
  char buffer[3] = "A:";
  buffer[0] += newdrive - 1;
  if (!SetCurrentDirectoryA( buffer ))
  {
    __CRTDLL__set_errno(GetLastError());
    if (newdrive <= 0)
      CRTDLL_errno = EACCES;
    return -1;
  }
  return 0;
}


/*********************************************************************
 *                  _findclose     (CRTDLL.098)
 * 
 * Free the resources from a search handle created from _findfirst.
 *
 * PARAMS
 *   hand [in]: Search handle to close
 *
 * RETURNS
 * Success:  0
 *
 * Failure:  -1
 */
INT __cdecl CRTDLL__findclose(DWORD hand)
{
  TRACE(":handle %ld\n",hand);
  if (!FindClose((HANDLE)hand))
  {
    __CRTDLL__set_errno(GetLastError());
    return -1;
  }
  return 0;
}


 /*********************************************************************
 *                  _findfirst    (CRTDLL.099)
 *
 * Create and return a search handle for iterating through a file and
 * directory list.
 *
 * PARAMS
 *   fspec [in]  File specification string for search, e.g "C:\*.BAT"
 * 
 *   ft [out]    A pointer to a find_t structure to populate.
 *
 * RETURNS
 * Success:  A handle for the search, suitable for passing to _findnext
 *           or _findclose. Populates the members of ft with the details
 *           of the first matching file.
 *
 * Failure:  -1.
 */
DWORD __cdecl CRTDLL__findfirst(LPCSTR fspec, find_t* ft)
{
  WIN32_FIND_DATAA find_data;
  HANDLE hfind;

  hfind  = FindFirstFileA(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    __CRTDLL__set_errno(GetLastError());
    return -1;
  }
  __CRTDLL__fttofd(&find_data,ft);
  TRACE(":got handle %d\n",hfind);
  return hfind;
}


/*********************************************************************
 *                  _findnext     (CRTDLL.100)
 * 
 * Return the next matching file/directory from a search hadle.
 *
 * PARAMS
 *   hand [in] Search handle from a pervious call to _findfirst
 * 
 *   ft [out]  A pointer to a find_t structure to populate.
 *
 * RETURNS
 * Success:  0. Populates the members of ft with the details
 *           of the first matching file
 *
 * Failure:  -1
 */
INT __cdecl CRTDLL__findnext(DWORD hand, find_t * ft)
{
  WIN32_FIND_DATAA find_data;

  if (!FindNextFileA(hand, &find_data))
  {
    CRTDLL_errno = ENOENT;
    return -1;
  }

  __CRTDLL__fttofd(&find_data,ft);
  return 0;
}


/*********************************************************************
 *                  _getcwd           (CRTDLL.120)
 *
 * Get the current directory.
 *
 * PARAMS
 * buf [out]  A buffer to place the current directory name in
 *
 * size [in]  The size of buf.
 *
 * RETURNS
 * Success: buf, or if buf is NULL, an allocated buffer
 *
 * Failure: NULL
 */
CHAR* __cdecl CRTDLL__getcwd(LPSTR buf, INT size)
{
  char dir[_MAX_PATH];
  int dir_len = GetCurrentDirectoryA(MAX_PATH,dir);

  if (dir_len < 1)
    return NULL; /* FIXME: Real return value untested */

  TRACE(":returning '%s'\n", dir);

  if (!buf)
  {
    if (size < 0)
      return CRTDLL__strdup(dir);
    return __CRTDLL__strndup(dir,size);
  }
  if (dir_len >= size)
  {
    CRTDLL_errno = ERANGE;
    return NULL; /* buf too small */
  }
  strcpy(buf,dir);
  return buf;
}


/*********************************************************************
 *                  _getdcwd           (CRTDLL.121)
 *
 * Get the current directory on a drive. A: =1, B: =2, etc.
 * Passing drive 0 means the current drive.
 */
CHAR* __cdecl CRTDLL__getdcwd(INT drive, LPSTR buf, INT size)
{
  static CHAR* dummy;

  TRACE(":drive %d(%c), size %d\n",drive, drive + 'A' - 1, size);

  if (!drive || drive == CRTDLL__getdrive())
    return CRTDLL__getcwd(buf,size); /* current */
  else
  {
    char dir[_MAX_PATH];
    char drivespec[4] = {'A', ':', '\\', 0};
    int dir_len;

    drivespec[0] += drive - 1;
    if (GetDriveTypeA(drivespec) < DRIVE_REMOVABLE)
    {
      CRTDLL_errno = EACCES;
      return NULL;
    }

    dir_len = GetFullPathNameA(drivespec,_MAX_PATH,dir,&dummy);
    if (dir_len >= size || dir_len < 1)
    {
      CRTDLL_errno = ERANGE;
      return NULL; /* buf too small */
    }

    TRACE(":returning '%s'\n", dir);
    if (!buf)
      return CRTDLL__strdup(dir); /* allocate */

    strcpy(buf,dir);
  }
  return buf;
}


/*********************************************************************
 *                  _getdiskfree     (CRTDLL.122)
 *
 * Get free disk space on given drive or the current drive.
 *
 */
UINT __cdecl CRTDLL__getdiskfree(UINT disk, diskfree_t* d)
{
  char drivespec[4] = {'@', ':', '\\', 0};
  DWORD ret[4];
  UINT err;

  if (disk > 26)
    return ERROR_INVALID_PARAMETER; /* CRTDLL doesn't set errno here */

  drivespec[0] += disk; /* make a drive letter */

  if (GetDiskFreeSpaceA(disk==0?NULL:drivespec,ret,ret+1,ret+2,ret+3))
  {
    d->cluster_sectors = (unsigned)ret[0];
    d->sector_bytes = (unsigned)ret[1];
    d->available = (unsigned)ret[2];
    d->num_clusters = (unsigned)ret[3];
    return 0;
  }
  err = GetLastError();
  __CRTDLL__set_errno(err);
  return err;
}


/*********************************************************************
 *                  _getdrive           (CRTDLL.124)
 *
 *  Return current drive, A: =1, B: =2, etc
 */
INT __cdecl CRTDLL__getdrive(VOID)
{
    char buffer[MAX_PATH];
    if (!GetCurrentDirectoryA( sizeof(buffer), buffer )) return 0;
    if (buffer[1] != ':') return 0;
    return toupper(buffer[0]) - 'A' + 1;
}


/*********************************************************************
 *                  _mkdir           (CRTDLL.234)
 *
 * Create a directory.
 */
INT __cdecl CRTDLL__mkdir(LPCSTR newdir)
{
  if (CreateDirectoryA(newdir,NULL))
    return 0;

  __CRTDLL__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *                  _rmdir           (CRTDLL.255)
 *
 * Delete a directory 
 *
 */
INT __cdecl CRTDLL__rmdir(LPSTR dir)
{
  if (RemoveDirectoryA(dir))
    return 0;

  __CRTDLL__set_errno(GetLastError());
  return -1;
}

