#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "xmalloc.h"
#include "windows.h"
#include "winerror.h"
#include "dos_fs.h"
#include "heap.h"
#include "drive.h"
#include "stddebug.h"
#include "debug.h"



#define PATH_LEN 260

struct FindFileContext32 {
    DIR *	dir;
    char mask[PATH_LEN];
    char path[PATH_LEN];
    char unixpath[PATH_LEN];
};

typedef struct FindFileContext32 FindFileContext32;

const char *DOSFS_Hash(const char *, int, int);

/* TODO/FIXME
 * 1) Check volume information to see if long file names supported
 * and do separate wildcard matching if so. Win95 has extended wildcard
 * matching - It can have wildcards like '*foo*'. These can match both
 * the long file name and the short file name.
 * 2) These file functions may be called from an interrupt
 *    Interrupt 21h Function 714Eh	FindFirstFile
 *    Interrupt 21h Function 714Fh	FindNextFile
 *    Interrupt 21h Function 71A1h	FindClose
 */

static BOOL32 MatchWildCard(LPCSTR file, LPCSTR mask)
{
    int	len;

    len = strlen(file);

    if (strcmp(mask, "*.*") == 0)
      return TRUE;

    while (*file) {
	if (*mask == '*') {
	    if (*(mask+1)) {
		while (*file && (toupper(*file) != *(mask+1))) file++;
		if (!*file)
		    return FALSE;
	    }
	    else
		break;
	}
	else {
	    if (*mask != '?' && *mask != toupper(*file)) {
		return FALSE;
	    }
	    file++;
	}
	mask++;
    }
    return (TRUE);
}

/* Functionality is same as GetFileInformationByHandle, but the structures
 * it fills out are different. This needs to be kept the same as the above
 * mentioned function.
 */


static BOOL32 FINDFILE_GetFileInfo(const char *filename,
				   LPWIN32_FIND_DATA32A finfo)
{
  struct stat file_stat;

  if (stat(filename, &file_stat) == -1) {
    SetLastError(ErrnoToLastError(errno));
    return FALSE;
  }
  finfo->dwFileAttributes = 0;
  if (file_stat.st_mode & S_IFREG)
    finfo->dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
  if (file_stat.st_mode & S_IFDIR)
    finfo->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
  if ((file_stat.st_mode & S_IRUSR) == 0)
    finfo->dwFileAttributes |= FILE_ATTRIBUTE_READONLY;

  /* Translate the file times.  Use the last modification time
   * for both the creation time and write time.
   */
  DOSFS_UnixTimeToFileTime(file_stat.st_mtime, &(finfo->ftCreationTime));
  DOSFS_UnixTimeToFileTime(file_stat.st_mtime, &(finfo->ftLastWriteTime));
  DOSFS_UnixTimeToFileTime(file_stat.st_atime, &(finfo->ftLastAccessTime));

  finfo->nFileSizeLow = file_stat.st_size;

  /* Zero out currently unused fields.
   */
  finfo->nFileSizeHigh = 0;
  finfo->dwReserved0 = 0;
  finfo->dwReserved1 = 0;
  return TRUE;

}


/*************************************************************************
 *              FindNextFile32A             (KERNEL32.126)
 */
BOOL32 FindNextFile32A(HANDLE32 handle, LPWIN32_FIND_DATA32A data)
{
    FindFileContext32 *context;
    struct dirent *dirent;
    char  dosname[14];
    char  fullfilename[PATH_LEN];

    memset(data, 0, sizeof(WIN32_FIND_DATA32A));
    context = (FindFileContext32 *) handle;

    while ((dirent = readdir(context->dir)) != NULL) {
	if (strcmp(dirent->d_name, "..") == 0 ||
	    strcmp(dirent->d_name, ".") == 0)
	    continue;

	lstrcpy32A(dosname, DOSFS_Hash(dirent->d_name, FALSE, FALSE));

	if (MatchWildCard(dirent->d_name, context->mask)) {
	    /* fill in file information */
	    lstrcpy32A(fullfilename, context->unixpath);
	    if (context->unixpath[strlen(context->unixpath)-1] != '/')
	      strcat(fullfilename, "/");
	    strcat(fullfilename, dirent->d_name);
            FINDFILE_GetFileInfo(fullfilename, data);

	    /* long file name */
	    lstrcpy32A(data->cFileName, dirent->d_name);

	    /* file name expressed in 8.3 format */
	    lstrcpy32A(data->cAlternateFileName, dosname);

	    dprintf_file(stddeb, "FindNextFile32A: %s (%s)\n",
			 data->cFileName, data->cAlternateFileName);
	    return (TRUE);
	}
    }
    
    return (FALSE);
}

/*************************************************************************
 *              FindNextFile32W             (KERNEL32.127)
 */
BOOL32 FindNextFile32W(HANDLE32 handle, LPWIN32_FIND_DATA32W data)
{
    WIN32_FIND_DATA32A	adata;
    BOOL32	res;

    adata.dwFileAttributes	= data->dwFileAttributes;
    adata.ftCreationTime	= data->ftCreationTime;
    adata.ftLastAccessTime	= data->ftLastAccessTime;
    adata.ftLastWriteTime	= data->ftLastWriteTime;
    adata.nFileSizeHigh		= data->nFileSizeHigh;
    adata.nFileSizeLow		= data->nFileSizeLow;
    adata.dwReserved0		= data->dwReserved0;
    adata.dwReserved1		= data->dwReserved1;
    lstrcpyWtoA(adata.cFileName,data->cFileName);
    lstrcpyWtoA(adata.cAlternateFileName,data->cAlternateFileName);
    res=FindNextFile32A(handle,&adata);
    if (res) {
	    data->dwFileAttributes 	= adata.dwFileAttributes;
	    data->ftCreationTime	= adata.ftCreationTime;
	    data->ftLastAccessTime	= adata.ftLastAccessTime;
	    data->ftLastWriteTime	= adata.ftLastWriteTime;
	    data->nFileSizeHigh		= adata.nFileSizeHigh;
	    data->nFileSizeLow		= adata.nFileSizeLow;
	    data->dwReserved0		= adata.dwReserved0; 
	    data->dwReserved1		= adata.dwReserved1;
	    lstrcpyAtoW(data->cFileName,adata.cFileName);
	    lstrcpyAtoW(data->cAlternateFileName,adata.cAlternateFileName);
    }
    return res;
}

/*************************************************************************
 *              FindFirstFile32A             (KERNEL32.123)
 */

HANDLE32 FindFirstFile32A(LPCSTR lpfilename_in, 
			  LPWIN32_FIND_DATA32A lpFindFileData)
{
    const char *unixpath;
    char *slash, *p;
    FindFileContext32 *context;
    char	lpfilename[PATH_LEN];
    INT32	len;

    context = HeapAlloc(SystemHeap, 0, sizeof(FindFileContext32));
    if (!context)
	return (INVALID_HANDLE_VALUE);

    /* These type of cases
     * A;\*.*
     * A;stuff\*.*
     * *.*
     * \stuff\*.*
     */
    lstrcpy32A(lpfilename, lpfilename_in);
    if (lpfilename[1] != ':' &&
	lpfilename[0] != '\\') {
      /* drive and root path are not set */
      len = GetCurrentDirectory32A(PATH_LEN, lpfilename);
      if (lpfilename[len-1] != '\\')
	strcat(lpfilename, "\\");
      strcat(lpfilename, lpfilename_in);
    }
    else if (lpfilename[1] != ':') {
      /* drive not set, but path is rooted */
      memmove(&lpfilename[2], lpfilename, strlen(lpfilename));
      lpfilename[0] = DRIVE_GetCurrentDrive();
      lpfilename[1] = ':';
    }
    else if (lpfilename[1] == ':' &&
	     lpfilename[2] != '\\') {
      /* drive is set, but not root path */
      lstrcpy32A(lpfilename, DRIVE_GetDosCwd(lpfilename[0]));
      strcat(lpfilename, lpfilename_in);
    }

    dprintf_file(stddeb, "FindFirstFile32A: %s -> %s .\n",
		 lpfilename_in, lpfilename);

    slash = strrchr(lpfilename, '\\');

    if (slash) {
	lstrcpyn32A(context->path, lpfilename, slash - lpfilename + 1);
	context->path[slash - lpfilename + 1] = '\0';
	unixpath = DOSFS_GetUnixFileName(context->path, FALSE);
	if (!unixpath) {
	    /* FIXME: SetLastError(??) */
	    HeapFree(SystemHeap, 0, context);
	    return INVALID_HANDLE_VALUE;
	}
	lstrcpy32A(context->mask, slash+1);
    }
    else {
      /* shouldn't really get here now */
      context->path[0] = '\0';
      unixpath = ".";
      lstrcpy32A(context->mask, lpfilename);
    }

    lstrcpy32A(context->unixpath, unixpath);
    context->dir = opendir(unixpath);
    if (!context->dir) {
	/* FIXME: SetLastError(??) */
	HeapFree(SystemHeap, 0, context);
	return INVALID_HANDLE_VALUE;
    }

    /* uppercase mask in place */
    for (p = context->mask ; *p; p++)
	*p = toupper(*p);

    if (!FindNextFile32A((HANDLE32) context, lpFindFileData))
	return (INVALID_HANDLE_VALUE);
    return ((HANDLE32) context);
}

/*************************************************************************
 *              FindFirstFile32W             (KERNEL32.124)
 */
HANDLE32 FindFirstFile32W(LPCWSTR filename,LPWIN32_FIND_DATA32W data)
{
    WIN32_FIND_DATA32A	adata;
    LPSTR afn = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    HANDLE32 res = FindFirstFile32A(afn,&adata);
    if (res)
    {
	data->dwFileAttributes 	= adata.dwFileAttributes;
	data->ftCreationTime	= adata.ftCreationTime;
	data->ftLastAccessTime	= adata.ftLastAccessTime;
	data->ftLastWriteTime	= adata.ftLastWriteTime;
	data->nFileSizeHigh	= adata.nFileSizeHigh;
	data->nFileSizeLow	= adata.nFileSizeLow;
	data->dwReserved0	= adata.dwReserved0; 
	data->dwReserved1	= adata.dwReserved1;
	lstrcpyAtoW(data->cFileName,adata.cFileName);
	lstrcpyAtoW(data->cAlternateFileName,adata.cAlternateFileName);
    }
    HeapFree( GetProcessHeap(), 0, afn );
    return res;
}

/*************************************************************************
 *              FindClose32             (KERNEL32.119)
 */
BOOL32 FindClose32(HANDLE32 handle)
{
    FindFileContext32 *context;

    /* Windows95 ignores an invalid handle. */
    if (handle == INVALID_HANDLE_VALUE)
    {
	SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    context = (FindFileContext32 *) handle;
    if (context->dir)
	closedir(context->dir);
    HeapFree(SystemHeap, 0, context);
    return (TRUE);
}

/* 16 bit versions of find functions */
/*************************************************************************
 *              FindFirstFile16             (KERNEL.413)
 */

HANDLE16
FindFirstFile16(LPCSTR lpFileName, LPVOID lpdata)
{
  WIN32_FIND_DATA32A	*findData = (WIN32_FIND_DATA32A *) lpdata;
  HANDLE32 h32;
  HGLOBAL16 h16;
  HANDLE32 *ptr;

  /* get a handle to the real pointer */


  h32 = FindFirstFile32A(lpFileName, findData);
  if (h32 > 0) {
    h16 = GlobalAlloc16(0, sizeof(h32));
    ptr = GlobalLock16(h16);
    *ptr = h32;
    return (h16);
  }
  else
    return ((HANDLE16) h32);
}

/*************************************************************************
 *              FindNextFile16             (KERNEL.414)
 */

BOOL16
FindNextFile16(HANDLE16 handle, LPVOID lpdata)
{
  WIN32_FIND_DATA32A	*findData = (WIN32_FIND_DATA32A *) lpdata;
  HANDLE32 *lph32;
  
  lph32 = GlobalLock16(handle);
  if (FindNextFile32A(*lph32, findData)) {
    return TRUE;
  }
  else
    return FALSE;
}

/*************************************************************************
 *              FindClose16             (KERNEL.415)
 */

BOOL16
FindClose16(HANDLE16 handle)
{
  HANDLE32 *lph32;
  BOOL16 ret;

  if (handle == (HANDLE16) INVALID_HANDLE_VALUE) {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  lph32 = GlobalLock16(handle);
  ret = FindClose32(*lph32);
  GlobalFree16(handle);
  return (ret);
}


