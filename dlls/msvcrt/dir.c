/*
 * msvcrt.dll drive/directory functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 */

#include <time.h>
#include "ntddk.h"
#include "wine/unicode.h"
#include "msvcrt.h"
#include "ms_errno.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

typedef struct MSVCRT_finddata_t
{
  unsigned      attrib;
  time_t        time_create; /* -1 when N/A */
  time_t        time_access; /* -1 when N/A */
  time_t        time_write;
  unsigned long size;        /* FIXME: 64 bit ??*/
  char          name[MAX_PATH];
} MSVCRT_finddata_t;

typedef struct MSVCRT_wfinddata_t
{
  unsigned      attrib;
  time_t        time_create; /* -1 when N/A */
  time_t        time_access; /* -1 when N/A */
  time_t        time_write;
  unsigned long size;        /* FIXME: 64 bit ??*/
  WCHAR          name[MAX_PATH];
} MSVCRT_wfinddata_t;

typedef struct __MSVCRT_diskfree_t {
  unsigned num_clusters;
  unsigned available;
  unsigned cluster_sectors;
  unsigned sector_bytes;
} MSVCRT_diskfree_t;

/* INTERNAL: Translate finddata_t to PWIN32_FIND_DATAA */
static void MSVCRT__fttofd(LPWIN32_FIND_DATAA fd, MSVCRT_finddata_t* ft)
{
  DWORD dw;

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

/* INTERNAL: Translate wfinddata_t to PWIN32_FIND_DATAA */
static void MSVCRT__wfttofd(LPWIN32_FIND_DATAW fd, MSVCRT_wfinddata_t* ft)
{
  DWORD dw;

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
  strcpyW(ft->name, fd->cFileName);
}

char * MSVCRT__strndup(const char *, unsigned int);
LPWSTR __cdecl MSVCRT__wcsdup( LPCWSTR );
LPWSTR __cdecl MSVCRT__wstrndup( LPCWSTR , unsigned int );
char *__cdecl  MSVCRT_getenv(const char *);
WCHAR *__cdecl wcscpy(WCHAR *,const WCHAR *);
WCHAR *__cdecl wcsncpy(WCHAR *,const WCHAR *,unsigned int);
WCHAR *__cdecl wcscat(WCHAR *,const WCHAR *);
WCHAR *__cdecl wcschr(WCHAR *,WCHAR);
WCHAR *__cdecl wcsrchr(WCHAR *,WCHAR);
void __cdecl _splitpath(const char *,char *, char *,char *,char *);

/*********************************************************************
 *		_chdir (MSVCRT.@)
 */
int __cdecl MSVCRT__chdir(const char * newdir)
{
  if (!SetCurrentDirectoryA(newdir))
  {
    MSVCRT__set_errno(newdir?GetLastError():0);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_wchdir (MSVCRT.@)
 */
int __cdecl MSVCRT__wchdir(const WCHAR * newdir)
{
  if (!SetCurrentDirectoryW(newdir))
  {
    MSVCRT__set_errno(newdir?GetLastError():0);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_chdrive (MSVCRT.@)
 */
int __cdecl MSVCRT__chdrive(int newdrive)
{
  char buffer[3] = "A:";
  buffer[0] += newdrive - 1;
  if (!SetCurrentDirectoryA( buffer ))
  {
    MSVCRT__set_errno(GetLastError());
    if (newdrive <= 0)
      SET_THREAD_VAR(errno,MSVCRT_EACCES);
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_findclose (MSVCRT.@)
 */
int __cdecl MSVCRT__findclose(DWORD hand)
{
  TRACE(":handle %ld\n",hand);
  if (!FindClose((HANDLE)hand))
  {
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  return 0;
}

/*********************************************************************
 *		_findfirst (MSVCRT.@)
 */
DWORD __cdecl MSVCRT__findfirst(const char * fspec, MSVCRT_finddata_t* ft)
{
  WIN32_FIND_DATAA find_data;
  HANDLE hfind;

  hfind  = FindFirstFileA(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  MSVCRT__fttofd(&find_data,ft);
  TRACE(":got handle %d\n",hfind);
  return hfind;
}

/*********************************************************************
 *		_wfindfirst (MSVCRT.@)
 */
DWORD __cdecl MSVCRT__wfindfirst(const WCHAR * fspec, MSVCRT_wfinddata_t* ft)
{
  WIN32_FIND_DATAW find_data;
  HANDLE hfind;

  hfind  = FindFirstFileW(fspec, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    MSVCRT__set_errno(GetLastError());
    return -1;
  }
  MSVCRT__wfttofd(&find_data,ft);
  TRACE(":got handle %d\n",hfind);
  return hfind;
}

/*********************************************************************
 *		_findnext (MSVCRT.@)
 */
int __cdecl MSVCRT__findnext(DWORD hand, MSVCRT_finddata_t * ft)
{
  WIN32_FIND_DATAA find_data;

  if (!FindNextFileA(hand, &find_data))
  {
    SET_THREAD_VAR(errno,MSVCRT_ENOENT);
    return -1;
  }

  MSVCRT__fttofd(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_wfindnext (MSVCRT.@)
 */
int __cdecl MSVCRT__wfindnext(DWORD hand, MSVCRT_wfinddata_t * ft)
{
  WIN32_FIND_DATAW find_data;

  if (!FindNextFileW(hand, &find_data))
  {
    SET_THREAD_VAR(errno,MSVCRT_ENOENT);
    return -1;
  }

  MSVCRT__wfttofd(&find_data,ft);
  return 0;
}

/*********************************************************************
 *		_getcwd (MSVCRT.@)
 */
char* __cdecl MSVCRT__getcwd(char * buf, int size)
{
  char dir[_MAX_PATH];
  int dir_len = GetCurrentDirectoryA(MAX_PATH,dir);

  if (dir_len < 1)
    return NULL; /* FIXME: Real return value untested */

  if (!buf)
  {
    if (size < 0)
      return MSVCRT__strdup(dir);
    return MSVCRT__strndup(dir,size);
  }
  if (dir_len >= size)
  {
    SET_THREAD_VAR(errno,MSVCRT_ERANGE);
    return NULL; /* buf too small */
  }
  strcpy(buf,dir);
  return buf;
}

/*********************************************************************
 *		_wgetcwd (MSVCRT.@)
 */
WCHAR* __cdecl MSVCRT__wgetcwd(WCHAR * buf, int size)
{
  WCHAR dir[_MAX_PATH];
  int dir_len = GetCurrentDirectoryW(MAX_PATH,dir);

  if (dir_len < 1)
    return NULL; /* FIXME: Real return value untested */

  if (!buf)
  {
    if (size < 0)
      return MSVCRT__wcsdup(dir);
    return MSVCRT__wstrndup(dir,size);
  }
  if (dir_len >= size)
  {
    SET_THREAD_VAR(errno,MSVCRT_ERANGE);
    return NULL; /* buf too small */
  }
  strcpyW(buf,dir);
  return buf;
}

/*********************************************************************
 *		_getdrive (MSVCRT.@)
 */
int __cdecl MSVCRT__getdrive(void)
{
    char buffer[MAX_PATH];
    if (!GetCurrentDirectoryA( sizeof(buffer), buffer )) return 0;
    if (buffer[1] != ':') return 0;
    return toupper(buffer[0]) - 'A' + 1;
}

/*********************************************************************
 *		_getdcwd (MSVCRT.@)
 */
char* __cdecl MSVCRT__getdcwd(int drive, char * buf, int size)
{
  static char* dummy;

  TRACE(":drive %d(%c), size %d\n",drive, drive + 'A' - 1, size);

  if (!drive || drive == MSVCRT__getdrive())
    return MSVCRT__getcwd(buf,size); /* current */
  else
  {
    char dir[_MAX_PATH];
    char drivespec[4] = {'A', ':', '\\', 0};
    int dir_len;

    drivespec[0] += drive - 1;
    if (GetDriveTypeA(drivespec) < DRIVE_REMOVABLE)
    {
      SET_THREAD_VAR(errno,MSVCRT_EACCES);
      return NULL;
    }

    dir_len = GetFullPathNameA(drivespec,_MAX_PATH,dir,&dummy);
    if (dir_len >= size || dir_len < 1)
    {
      SET_THREAD_VAR(errno,MSVCRT_ERANGE);
      return NULL; /* buf too small */
    }

    TRACE(":returning '%s'\n", dir);
    if (!buf)
      return MSVCRT__strdup(dir); /* allocate */

    strcpy(buf,dir);
  }
  return buf;
}

/*********************************************************************
 *		_wgetdcwd (MSVCRT.@)
 */
WCHAR* __cdecl MSVCRT__wgetdcwd(int drive, WCHAR * buf, int size)
{
  static WCHAR* dummy;

  TRACE(":drive %d(%c), size %d\n",drive, drive + 'A' - 1, size);

  if (!drive || drive == MSVCRT__getdrive())
    return MSVCRT__wgetcwd(buf,size); /* current */
  else
  {
    WCHAR dir[_MAX_PATH];
    WCHAR drivespec[4] = {'A', ':', '\\', 0};
    int dir_len;

    drivespec[0] += drive - 1;
    if (GetDriveTypeW(drivespec) < DRIVE_REMOVABLE)
    {
      SET_THREAD_VAR(errno,MSVCRT_EACCES);
      return NULL;
    }

    dir_len = GetFullPathNameW(drivespec,_MAX_PATH,dir,&dummy);
    if (dir_len >= size || dir_len < 1)
    {
      SET_THREAD_VAR(errno,MSVCRT_ERANGE);
      return NULL; /* buf too small */
    }

    TRACE(":returning '%s'\n", debugstr_w(dir));
    if (!buf)
      return MSVCRT__wcsdup(dir); /* allocate */
    strcpyW(buf,dir);
  }
  return buf;
}

/*********************************************************************
 *		_getdiskfree (MSVCRT.@)
 */
unsigned int __cdecl MSVCRT__getdiskfree(unsigned int disk, MSVCRT_diskfree_t* d)
{
  char drivespec[4] = {'@', ':', '\\', 0};
  DWORD ret[4];
  unsigned int err;

  if (disk > 26)
    return ERROR_INVALID_PARAMETER; /* MSVCRT doesn't set errno here */

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
  MSVCRT__set_errno(err);
  return err;
}

/*********************************************************************
 *		_mkdir (MSVCRT.@)
 */
int __cdecl MSVCRT__mkdir(const char * newdir)
{
  if (CreateDirectoryA(newdir,NULL))
    return 0;
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wmkdir (MSVCRT.@)
 */
int __cdecl MSVCRT__wmkdir(const WCHAR* newdir)
{
  if (CreateDirectoryW(newdir,NULL))
    return 0;
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_rmdir (MSVCRT.@)
 */
int __cdecl MSVCRT__rmdir(const char * dir)
{
  if (RemoveDirectoryA(dir))
    return 0;
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wrmdir (MSVCRT.@)
 */
int __cdecl MSVCRT__wrmdir(const WCHAR * dir)
{
  if (RemoveDirectoryW(dir))
    return 0;
  MSVCRT__set_errno(GetLastError());
  return -1;
}

/*********************************************************************
 *		_wsplitpath (MSVCRT.@)
 */
void __cdecl MSVCRT__wsplitpath(const WCHAR *inpath, WCHAR *drv, WCHAR *dir,
                                WCHAR *fname, WCHAR *ext )
{
  /* Modified PD code from 'snippets' collection. */
  WCHAR ch, *ptr, *p;
  WCHAR pathbuff[MAX_PATH],*path=pathbuff;

  TRACE(":splitting path '%s'\n",debugstr_w(path));
  wcscpy(pathbuff, inpath);

  /* convert slashes to backslashes for searching */
  for (ptr = (WCHAR*)path; *ptr; ++ptr)
    if (*ptr == (WCHAR)L'/')
      *ptr = (WCHAR)L'\\';

  /* look for drive spec */
  if ((ptr = wcschr(path, (WCHAR)L':')) != (WCHAR)L'\0')
  {
    ++ptr;
    if (drv)
    {
      wcsncpy(drv, path, ptr - path);
      drv[ptr - path] = (WCHAR)L'\0';
    }
    path = ptr;
  }
  else if (drv)
    *drv = (WCHAR)L'\0';

  /* find rightmost backslash or leftmost colon */
  if ((ptr = wcsrchr(path, (WCHAR)L'\\')) == NULL)
    ptr = (wcschr(path, (WCHAR)L':'));

  if (!ptr)
  {
    ptr = (WCHAR *)path; /* no path */
    if (dir)
      *dir = (WCHAR)L'\0';
  }
  else
  {
    ++ptr; /* skip the delimiter */
    if (dir)
    {
      ch = *ptr;
      *ptr = (WCHAR)L'\0';
      wcscpy(dir, path);
      *ptr = ch;
    }
  }

  if ((p = wcsrchr(ptr, (WCHAR)L'.')) == NULL)
  {
    if (fname)
      wcscpy(fname, ptr);
    if (ext)
      *ext = (WCHAR)L'\0';
  }
  else
  {
    *p = (WCHAR)L'\0';
    if (fname)
      wcscpy(fname, ptr);
    *p = (WCHAR)L'.';
    if (ext)
      wcscpy(ext, p);
  }

  /* Fix pathological case - Win returns ':' as part of the
   * directory when no drive letter is given.
   */
  if (drv && drv[0] == (WCHAR)L':')
  {
    *drv = (WCHAR)L'\0';
    if (dir)
    {
      pathbuff[0] = (WCHAR)L':';
      pathbuff[1] = (WCHAR)L'\0';
      wcscat(pathbuff,dir);
      wcscpy(dir, pathbuff);
    }
  }
}

/* INTERNAL: Helper for _fullpath. Modified PD code from 'snippets'. */
static void fln_fix(char *path)
{
  int dir_flag = 0, root_flag = 0;
  char *r, *p, *q, *s;

  /* Skip drive */
  if (NULL == (r = strrchr(path, ':')))
    r = path;
  else
    ++r;

  /* Ignore leading slashes */
  while ('\\' == *r)
    if ('\\' == r[1])
      strcpy(r, &r[1]);
    else
    {
      root_flag = 1;
      ++r;
    }

  p = r; /* Change "\\" to "\" */
  while (NULL != (p = strchr(p, '\\')))
    if ('\\' ==  p[1])
      strcpy(p, &p[1]);
    else
      ++p;

  while ('.' == *r) /* Scrunch leading ".\" */
  {
    if ('.' == r[1])
    {
      /* Ignore leading ".." */
      for (p = (r += 2); *p && (*p != '\\'); ++p)
	;
    }
    else
    {
      for (p = r + 1 ;*p && (*p != '\\'); ++p)
	;
    }
    strcpy(r, p + ((*p) ? 1 : 0));
  }

  while ('\\' == path[strlen(path)-1])   /* Strip last '\\' */
  {
    dir_flag = 1;
    path[strlen(path)-1] = '\0';
  }

  s = r;

  /* Look for "\." in path */

  while (NULL != (p = strstr(s, "\\.")))
  {
    if ('.' == p[2])
    {
      /* Execute this section if ".." found */
      q = p - 1;
      while (q > r)           /* Backup one level           */
      {
	if (*q == '\\')
	  break;
	--q;
      }
      if (q > r)
      {
	strcpy(q, p + 3);
	s = q;
      }
      else if ('.' != *q)
      {
	strcpy(q + ((*q == '\\') ? 1 : 0),
	       p + 3 + ((*(p + 3)) ? 1 : 0));
	s = q;
      }
      else  s = ++p;
    }
    else
    {
      /* Execute this section if "." found */
      q = p + 2;
      for ( ;*q && (*q != '\\'); ++q)
	;
      strcpy (p, q);
    }
  }

  if (root_flag)  /* Embedded ".." could have bubbled up to root  */
  {
    for (p = r; *p && ('.' == *p || '\\' == *p); ++p)
      ;
    if (r != p)
      strcpy(r, p);
  }

  if (dir_flag)
    strcat(path, "\\");
}

/*********************************************************************
 *		_fullpath (MSVCRT.@)
 */
char *__cdecl MSVCRT__fullpath(char * absPath, const char* relPath, unsigned int size)
{
  char drive[5],dir[MAX_PATH],file[MAX_PATH],ext[MAX_PATH];
  char res[MAX_PATH];
  size_t len;

  res[0] = '\0';

  if (!relPath || !*relPath)
    return MSVCRT__getcwd(absPath, size);

  if (size < 4)
  {
    SET_THREAD_VAR(errno,MSVCRT_ERANGE);
    return NULL;
  }

  TRACE(":resolving relative path '%s'\n",relPath);

  _splitpath(relPath, drive, dir, file, ext);

  /* Get Directory and drive into 'res' */
  if (!dir[0] || (dir[0] != '/' && dir[0] != '\\'))
  {
    /* Relative or no directory given */
    MSVCRT__getdcwd(drive[0] ? toupper(drive[0]) - 'A' + 1 :  0, res, MAX_PATH);
    strcat(res,"\\");
    if (dir[0])
      strcat(res,dir);
    if (drive[0])
      res[0] = drive[0]; /* If given a drive, preserve the letter case */
  }
  else
  {
    strcpy(res,drive);
    strcat(res,dir);
  }

  strcat(res,"\\");
  strcat(res, file);
  strcat(res, ext);
  fln_fix(res);

  len = strlen(res);
  if (len >= MAX_PATH || len >= (size_t)size)
    return NULL; /* FIXME: errno? */

  if (!absPath)
    return MSVCRT__strdup(res);
  strcpy(absPath,res);
  return absPath;
}

/*********************************************************************
 *		_makepath (MSVCRT.@)
 */
VOID __cdecl MSVCRT__makepath(char * path, const char * drive,
                              const char *directory, const char * filename,
                              const char * extension )
{
    char ch;
    TRACE("MSVCRT__makepath got %s %s %s %s\n", drive, directory,
          filename, extension);

    if ( !path )
        return;

    path[0] = 0;
    if (drive && drive[0])
    {
        path[0] = drive[0];
        path[1] = ':';
        path[2] = 0;
    }
    if (directory && directory[0])
    {
        strcat(path, directory);
        ch = path[strlen(path)-1];
        if (ch != '/' && ch != '\\')
            strcat(path,"\\");
    }
    if (filename && filename[0])
    {
        strcat(path, filename);
        if (extension && extension[0])
        {
            if ( extension[0] != '.' )
                strcat(path,".");
            strcat(path,extension);
        }
    }

    TRACE("MSVCRT__makepath returns %s\n",path);
}


/*********************************************************************
 *		_searchenv (MSVCRT.@)
 */
void __cdecl MSVCRT__searchenv(const char* file, const char* env, char *buf)
{
  char*envVal, *penv;
  char curPath[MAX_PATH];

  *buf = '\0';

  /* Try CWD first */
  if (GetFileAttributesA( file ) != 0xFFFFFFFF)
  {
    GetFullPathNameA( file, MAX_PATH, buf, NULL );
    /* Sigh. This error is *always* set, regardless of success */
    MSVCRT__set_errno(ERROR_FILE_NOT_FOUND);
    return;
  }

  /* Search given environment variable */
  envVal = MSVCRT_getenv(env);
  if (!envVal)
  {
    MSVCRT__set_errno(ERROR_FILE_NOT_FOUND);
    return;
  }

  penv = envVal;
  TRACE(":searching for %s in paths %s\n", file, envVal);

  do
  {
    char *end = penv;

    while(*end && *end != ';') end++; /* Find end of next path */
    if (penv == end || !*penv)
    {
      MSVCRT__set_errno(ERROR_FILE_NOT_FOUND);
      return;
    }
    strncpy(curPath, penv, end - penv);
    if (curPath[end - penv] != '/' || curPath[end - penv] != '\\')
    {
      curPath[end - penv] = '\\';
      curPath[end - penv + 1] = '\0';
    }
    else
      curPath[end - penv] = '\0';

    strcat(curPath, file);
    TRACE("Checking for file %s\n", curPath);
    if (GetFileAttributesA( curPath ) != 0xFFFFFFFF)
    {
      strcpy(buf, curPath);
      MSVCRT__set_errno(ERROR_FILE_NOT_FOUND);
      return; /* Found */
    }
    penv = *end ? end + 1 : end;
  } while(1);
}

