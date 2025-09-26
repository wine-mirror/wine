/*
 * CMD - Wine-compatible command line interface - Directory functions.
 *
 * Copyright (C) 1999 D A Pickles
 * Copyright (C) 2007 J Edmeades
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN

#include "wcmd.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cmd);

typedef enum _DISPLAYTIME
{
    Creation = 0,
    Access,
    Written
} DISPLAYTIME;

typedef enum _DISPLAYORDER
{
    Unspecified = 0,
    Name,
    Extension,
    Size,
    Date
} DISPLAYORDER;

#define MAX_DATETIME_FORMAT 80

static WCHAR date_format[MAX_DATETIME_FORMAT * 2];
static WCHAR time_format[MAX_DATETIME_FORMAT * 2];
static int file_total, dir_total, max_width;
static ULONGLONG byte_total;
static DISPLAYTIME dirTime;
static DISPLAYORDER dirOrder;
static BOOL orderReverse, orderGroupDirs, orderGroupDirsReverse, orderByCol;
static BOOL paged_mode, recurse, wide, bare, lower, shortname, usernames, separator;
static ULONG showattrs, attrsbits;

/*****************************************************************************
 * WCMD_filesize64
 *
 * Convert a 64-bit number into a WCHARacter string, with commas every three digits.
 * Result is returned in a static string overwritten with each call.
 * FIXME: There must be a better algorithm!
 */
static WCHAR * WCMD_filesize64 (ULONGLONG n) {

  ULONGLONG q;
  unsigned int r, i;
  WCHAR *p;
  static WCHAR buff[32];

  p = buff;
  i = -3;
  do {
    if (separator && ((++i)%3 == 1)) *p++ = ',';
    q = n / 10;
    r = n - (q * 10);
    *p++ = r + '0';
    *p = '\0';
    n = q;
  } while (n != 0);
  wcsrev(buff);
  return buff;
}

/*****************************************************************************
 * WCMD_dir_sort
 *
 * Sort based on the /O options supplied on the command line
 */
static int __cdecl WCMD_dir_sort (const void *a, const void *b)
{
  const WIN32_FIND_DATAW *filea = (const WIN32_FIND_DATAW *)a;
  const WIN32_FIND_DATAW *fileb = (const WIN32_FIND_DATAW *)b;
  BOOL aDir = filea->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
  BOOL bDir = fileb->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
  int result = 0;

  if (orderGroupDirs && dirOrder == Unspecified) {
    /* Special case: If ordering groups and not sorting by other criteria, "." and ".." always go first. */
    if (aDir && !lstrcmpW(filea->cFileName, L".")) {
      result = -1;
    } else if (bDir && !lstrcmpW(fileb->cFileName, L".")) {
      result = 1;
    } else if (aDir && !lstrcmpW(filea->cFileName, L"..")) {
      result = -1;
    } else if (bDir && !lstrcmpW(fileb->cFileName, L"..")) {
      result = 1;
    }

    if (result) {
      if (orderGroupDirsReverse) result = -result;
      return result;
    }
  }

  /* If /OG or /O-G supplied, dirs go at the top or bottom, also sorted
     if requested sort order is by name.                                */
  if (orderGroupDirs && (aDir || bDir)) {
    if (aDir && bDir && dirOrder == Name) {
      result = lstrcmpiW(filea->cFileName, fileb->cFileName);
      if (orderReverse) result = -result;
    } else if (aDir) {
      result = -1;
    }
    else result = 1;
    if (orderGroupDirsReverse) result = -result;
    return result;

  /* Order by Name: */
  } else if (dirOrder == Name) {
    result = lstrcmpiW(filea->cFileName, fileb->cFileName);

  /* Order by Size: */
  } else if (dirOrder == Size) {
    ULONG64 sizea = (((ULONG64)filea->nFileSizeHigh) << 32) + filea->nFileSizeLow;
    ULONG64 sizeb = (((ULONG64)fileb->nFileSizeHigh) << 32) + fileb->nFileSizeLow;
    if( sizea < sizeb ) result = -1;
    else if( sizea == sizeb ) result = 0;
    else result = 1;

  /* Order by Date: (Takes into account which date (/T option) */
  } else if (dirOrder == Date) {

    const FILETIME *ft;
    ULONG64 timea, timeb;

    if (dirTime == Written) {
      ft = &filea->ftLastWriteTime;
      timea = (((ULONG64)ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
      ft = &fileb->ftLastWriteTime;
      timeb = (((ULONG64)ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
    } else if (dirTime == Access) {
      ft = &filea->ftLastAccessTime;
      timea = (((ULONG64)ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
      ft = &fileb->ftLastAccessTime;
      timeb = (((ULONG64)ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
    } else {
      ft = &filea->ftCreationTime;
      timea = (((ULONG64)ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
      ft = &fileb->ftCreationTime;
      timeb = (((ULONG64)ft->dwHighDateTime) << 32) + ft->dwLowDateTime;
    }
    if( timea < timeb ) result = -1;
    else if( timea == timeb ) result = 0;
    else result = 1;

  /* Order by Extension: (Takes into account which date (/T option) */
  } else if (dirOrder == Extension) {
      WCHAR drive[10];
      WCHAR dir[MAX_PATH];
      WCHAR fname[MAX_PATH];
      WCHAR extA[MAX_PATH];
      WCHAR extB[MAX_PATH];

      /* Split into components */
      _wsplitpath(filea->cFileName, drive, dir, fname, extA);
      _wsplitpath(fileb->cFileName, drive, dir, fname, extB);
      result = lstrcmpiW(extA, extB);
  }

  if (orderReverse) result = -result;
  return result;
}

/*****************************************************************************
 * WCMD_getfileowner
 */
static void WCMD_getfileowner(WCHAR *filename, WCHAR *owner, int ownerlen) {

    ULONG sizeNeeded = 0;
    DWORD rc;
    WCHAR name[MAXSTRING];
    WCHAR domain[MAXSTRING];

    /* In case of error, return empty string */
    *owner = 0x00;

    /* Find out how much space we need for the owner security descriptor */
    GetFileSecurityW(filename, OWNER_SECURITY_INFORMATION, 0, 0, &sizeNeeded);
    rc = GetLastError();

    if(rc == ERROR_INSUFFICIENT_BUFFER && sizeNeeded > 0) {

        LPBYTE secBuffer;
        PSID pSID = NULL;
        BOOL defaulted = FALSE;
        ULONG nameLen = MAXSTRING;
        ULONG domainLen = MAXSTRING;
        SID_NAME_USE nameuse;

        secBuffer = xalloc(sizeNeeded * sizeof(BYTE));

        /* Get the owners security descriptor */
        if(!GetFileSecurityW(filename, OWNER_SECURITY_INFORMATION, secBuffer,
                            sizeNeeded, &sizeNeeded)) {
            free(secBuffer);
            return;
        }

        /* Get the SID from the SD */
        if(!GetSecurityDescriptorOwner(secBuffer, &pSID, &defaulted)) {
            free(secBuffer);
            return;
        }

        /* Convert to a username */
        if (LookupAccountSidW(NULL, pSID, name, &nameLen, domain, &domainLen, &nameuse)) {
            swprintf(owner, ownerlen, L"%s%c%s", domain, '\\', name);
        }
        free(secBuffer);
    }
    return;
}

/*****************************************************************************
 * WCMD_list_directory
 *
 * List a single file directory. This function (and those below it) can be called
 * recursively when the /S switch is used.
 */

static RETURN_CODE WCMD_list_directory (DIRECTORY_STACK *inputparms, int level, DIRECTORY_STACK **outputparms) {

  WCHAR string[1024], datestring[32], timestring[32];
  WCHAR real_path[MAX_PATH];
  WIN32_FIND_DATAW *fd;
  FILETIME ft;
  SYSTEMTIME st;
  HANDLE hff;
  int dir_count, file_count, entry_count, i, widest, cur_width, tmp_width;
  int numCols, numRows;
  int rows, cols;
  ULARGE_INTEGER byte_count, file_size;
  DIRECTORY_STACK *parms;
  int concurrentDirs = 0;
  BOOL done_header = FALSE;
  RETURN_CODE return_code = NO_ERROR;

  dir_count = 0;
  file_count = 0;
  entry_count = 0;
  byte_count.QuadPart = 0;
  widest = 0;
  cur_width = 0;

  /* Loop merging all the files from consecutive parms which relate to the
     same directory. Note issuing a directory header with no contents
     mirrors what windows does                                            */
  parms = inputparms;
  fd = xalloc(sizeof(WIN32_FIND_DATAW));
  while (parms && lstrcmpW(inputparms->dirName, parms->dirName) == 0) {
    concurrentDirs++;

    /* Work out the full path + filename */
    lstrcpyW(real_path, parms->dirName);
    lstrcatW(real_path, parms->fileName);

    /* Load all files into an in memory structure */
    WINE_TRACE("Looking for matches to '%s'\n", wine_dbgstr_w(real_path));
    hff = FindFirstFileW(real_path, &fd[entry_count]);
    if (hff != INVALID_HANDLE_VALUE) {
      do {
        /* Skip any which are filtered out by attribute */
        if ((fd[entry_count].dwFileAttributes & attrsbits) != showattrs) continue;

        entry_count++;

        /* Keep running track of longest filename for wide output */
        if (wide || orderByCol) {
           int tmpLen = lstrlenW(fd[entry_count-1].cFileName) + 3;
           if (fd[entry_count-1].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) tmpLen = tmpLen + 2;
           if (tmpLen > widest) widest = tmpLen;
        }

        fd = xrealloc(fd, (entry_count + 1) * sizeof(WIN32_FIND_DATAW));
      } while (FindNextFileW(hff, &fd[entry_count]) != 0);
      FindClose (hff);
    }

    /* Work out the actual current directory name without a trailing \ */
    lstrcpyW(real_path, parms->dirName);
    real_path[lstrlenW(parms->dirName)-1] = 0x00;

    /* Output the results */
    if (!bare) {
       if (level != 0 && (entry_count > 0)) return_code = WCMD_output_asis(L"\r\n");
       if (!recurse || ((entry_count > 0) && done_header==FALSE)) {
           WCMD_output (L"Directory of %1\n\n", real_path);
           done_header = TRUE;
       }
    }

    /* Move to next parm */
    parms = parms->next;
  }

  /* Handle case where everything is filtered out */
  if (entry_count > 0) {

    /* Sort the list of files */
    qsort (fd, entry_count, sizeof(WIN32_FIND_DATAW), WCMD_dir_sort);

    /* Work out the number of columns */
    WINE_TRACE("%d entries, maxwidth=%d, widest=%d\n", entry_count, max_width, widest);
    if (wide || orderByCol) {
      numCols = max(1, max_width / widest);
      numRows = entry_count / numCols;
      if (entry_count % numCols) numRows++;
    } else {
      numCols = 1;
      numRows = entry_count;
    }
    WINE_TRACE("cols=%d, rows=%d\n", numCols, numRows);

    for (rows=0; rows<numRows && return_code == NO_ERROR; rows++) {
     BOOL addNewLine = TRUE;
     for (cols=0; cols<numCols; cols++) {
      WCHAR username[24];

      /* Work out the index of the entry being pointed to */
      if (orderByCol) {
        i = (cols * numRows) + rows;
        if (i >= entry_count) continue;
      } else {
        i = (rows * numCols) + cols;
        if (i >= entry_count) continue;
      }

      /* /L convers all names to lower case */
      if (lower) wcslwr( fd[i].cFileName );

      /* /Q gets file ownership information */
      if (usernames) {
          lstrcpyW (string, inputparms->dirName);
          lstrcatW (string, fd[i].cFileName);
          WCMD_getfileowner(string, username, ARRAY_SIZE(username));
      }

      if (dirTime == Written) {
        FileTimeToLocalFileTime (&fd[i].ftLastWriteTime, &ft);
      } else if (dirTime == Access) {
        FileTimeToLocalFileTime (&fd[i].ftLastAccessTime, &ft);
      } else {
        FileTimeToLocalFileTime (&fd[i].ftCreationTime, &ft);
      }
      FileTimeToSystemTime (&ft, &st);
      GetDateFormatW(LOCALE_USER_DEFAULT, 0, &st, date_format,
                     datestring, ARRAY_SIZE(datestring));
      GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, time_format,
                     timestring, ARRAY_SIZE(timestring));

      if (wide) {

        tmp_width = cur_width;
        if (fd[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            WCMD_output (L"[%1]", fd[i].cFileName);
            dir_count++;
            tmp_width = tmp_width + lstrlenW(fd[i].cFileName) + 2;
        } else {
            WCMD_output (L"%1", fd[i].cFileName);
            tmp_width = tmp_width + lstrlenW(fd[i].cFileName) ;
            file_count++;
            file_size.u.LowPart = fd[i].nFileSizeLow;
            file_size.u.HighPart = fd[i].nFileSizeHigh;
        byte_count.QuadPart += file_size.QuadPart;
        }
        cur_width = cur_width + widest;

        if ((cur_width + widest) > max_width) {
            cur_width = 0;
        } else {
            WCMD_output(L"%1!*s!", cur_width - tmp_width, L"");
        }

      } else if (fd[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        dir_count++;

        if (!bare) {
           WCMD_output (L"%1  %2    <DIR>          ", datestring, timestring);
           if (shortname) WCMD_output(L"%1!-13s!", fd[i].cAlternateFileName);
           if (usernames) WCMD_output(L"%1!-23s!", username);
           WCMD_output(L"%1",fd[i].cFileName);
        } else {
           if (!((lstrcmpW(fd[i].cFileName, L".") == 0) ||
                 (lstrcmpW(fd[i].cFileName, L"..") == 0))) {
              WCMD_output(L"%1%2", recurse ? inputparms->dirName : L"", fd[i].cFileName);
           } else {
              addNewLine = FALSE;
           }
        }
      }
      else {
        file_count++;
        file_size.u.LowPart = fd[i].nFileSizeLow;
        file_size.u.HighPart = fd[i].nFileSizeHigh;
        byte_count.QuadPart += file_size.QuadPart;
        if (!bare) {
           WCMD_output (L"%1  %2    %3!14s! ", datestring, timestring,
                        WCMD_filesize64(file_size.QuadPart));
           if (shortname) WCMD_output(L"%1!-13s!", fd[i].cAlternateFileName);
           if (usernames) WCMD_output(L"%1!-23s!", username);
           WCMD_output(L"%1",fd[i].cFileName);
        } else {
           WCMD_output(L"%1%2", recurse ? inputparms->dirName : L"", fd[i].cFileName);
        }
      }
     }
     if (addNewLine) return_code = WCMD_output_asis(L"\r\n");
     cur_width = 0;

     /* Allow command to be aborted if user presses Ctrl-C.
      * Don't overwrite any existing error code.
      */
     if (return_code == NO_ERROR) {
        return_code = WCMD_ctrlc_status();
     }
    }

    if (!bare && return_code == NO_ERROR) {
       if (file_count == 1) {
         WCMD_output (L"       1 file %1!25s! bytes\n", WCMD_filesize64 (byte_count.QuadPart));
       }
       else {
         WCMD_output (L"%1!8d! files %2!24s! bytes\n", file_count, WCMD_filesize64 (byte_count.QuadPart));
       }
    }
    byte_total = byte_total + byte_count.QuadPart;
    file_total = file_total + file_count;
    dir_total = dir_total + dir_count;

    if (!bare && !recurse && return_code == NO_ERROR) {
       if (dir_count == 1) {
           WCMD_output (L"%1!8d! directory         ", 1);
       } else {
           WCMD_output (L"%1!8d! directories", dir_count);
       }
    }
  }
  free(fd);

  /* When recursing, look in all subdirectories for matches */
  if (recurse && return_code == NO_ERROR) {
    DIRECTORY_STACK *dirStack = NULL;
    DIRECTORY_STACK *lastEntry = NULL;
    WIN32_FIND_DATAW finddata;

    /* Build path to search */
    lstrcpyW(string, inputparms->dirName);
    lstrcatW(string, L"*");

    WINE_TRACE("Recursive, looking for '%s'\n", wine_dbgstr_w(string));
    hff = FindFirstFileW(string, &finddata);
    if (hff != INVALID_HANDLE_VALUE) {
      do {
        if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (lstrcmpW(finddata.cFileName, L"..") != 0) &&
            (lstrcmpW(finddata.cFileName, L".") != 0)) {

          DIRECTORY_STACK *thisDir;
          int              dirsToCopy = concurrentDirs;

          /* Loop creating list of subdirs for all concurrent entries */
          parms = inputparms;
          while (dirsToCopy > 0) {
            dirsToCopy--;

            /* Work out search parameter in sub dir */
            lstrcpyW (string, inputparms->dirName);
            lstrcatW (string, finddata.cFileName);
            lstrcatW(string, L"\\");
            WINE_TRACE("Recursive, Adding to search list '%s'\n", wine_dbgstr_w(string));

            /* Allocate memory, add to list */
            thisDir = xalloc(sizeof(DIRECTORY_STACK));
            if (dirStack == NULL) dirStack = thisDir;
            if (lastEntry != NULL) lastEntry->next = thisDir;
            lastEntry = thisDir;
            thisDir->next = NULL;
            thisDir->dirName = xstrdupW(string);
            thisDir->fileName = xstrdupW(parms->fileName);
            parms = parms->next;
          }
        }
      } while (FindNextFileW(hff, &finddata) != 0);
      FindClose (hff);

      while (dirStack != NULL && return_code == NO_ERROR) {
        DIRECTORY_STACK *thisDir = dirStack;
        return_code = WCMD_list_directory (thisDir, 1, &dirStack);
        if (return_code != NO_ERROR) dirStack = NULL;
        while (thisDir != dirStack) {
          DIRECTORY_STACK *tempDir = thisDir->next;
          free(thisDir->dirName);
          free(thisDir->fileName);
          free(thisDir);
          thisDir = tempDir;
        }
      }
    }
  }

  /* Handle case where everything is filtered out */
  if ((file_total + dir_total == 0) && (level == 0)) {
    return_code = ERROR_FILE_NOT_FOUND;
    SetLastError (return_code);
    WCMD_print_error ();
  }

  *outputparms = parms;
  return return_code;
}

/*****************************************************************************
 * WCMD_dir_trailer
 *
 * Print out the trailer for the supplied path
 */
static void WCMD_dir_trailer(const WCHAR *path) {
    ULARGE_INTEGER freebytes;
    BOOL status;

    status = GetDiskFreeSpaceExW(path, NULL, NULL, &freebytes);
    WINE_TRACE("Writing trailer for '%s' gave %d(%ld)\n", wine_dbgstr_w(path),
               status, GetLastError());

    if (!bare) {
      if (recurse) {
        WCMD_output (L"\n     Total files listed:\n%1!8d! files%2!25s! bytes\n", file_total, WCMD_filesize64 (byte_total));
        WCMD_output (L"%1!8d! directories %2!18s! bytes free\n\n", dir_total, WCMD_filesize64 (freebytes.QuadPart));
      } else {
        WCMD_output (L" %1!18s! bytes free\n\n", WCMD_filesize64 (freebytes.QuadPart));
      }
    }
}

/* Get the length of a date/time formatting pattern */
/* copied from dlls/kernelbase/locale.c */
static int get_pattern_len( const WCHAR *pattern, const WCHAR *accept )
{
    int i;

    if (*pattern == '\'')
    {
        for (i = 1; pattern[i]; i++)
        {
            if (pattern[i] != '\'') continue;
            if (pattern[++i] != '\'') return i;
        }
        return i;
    }
    if (!wcschr( accept, *pattern )) return 1;
    for (i = 1; pattern[i]; i++) if (pattern[i] != pattern[0]) break;
    return i;
}

/* Initialize date format to use abbreviated one with leading zeros */
static void init_date_format(void)
{
    WCHAR sys_format[MAX_DATETIME_FORMAT];
    int src_pat_len, dst_pat_len;
    const WCHAR *src;
    WCHAR *dst = date_format;

    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SSHORTDATE, sys_format, ARRAY_SIZE(sys_format));

    for (src = sys_format; *src; src += src_pat_len, dst += dst_pat_len) {
        src_pat_len = dst_pat_len = get_pattern_len(src, L"yMd");

        switch (*src)
        {
        case '\'':
            wmemcpy(dst, src, src_pat_len);
            break;

        case 'd':
        case 'M':
            if (src_pat_len == 4) /* full name */
                dst_pat_len--; /* -> use abbreviated one */
            /* fallthrough */
        case 'y':
            if (src_pat_len == 1) /* without leading zeros */
                dst_pat_len++; /* -> with leading zeros */
            wmemset(dst, *src, dst_pat_len);
            break;

        default:
            *dst = *src;
            break;
        }
    }
    *dst = '\0';

    TRACE("date format: %s\n", wine_dbgstr_w(date_format));
}

/* Initialize time format to use leading zeros */
static void init_time_format(void)
{
    WCHAR sys_format[MAX_DATETIME_FORMAT];
    int src_pat_len, dst_pat_len;
    const WCHAR *src;
    WCHAR *dst = time_format;

    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, sys_format, ARRAY_SIZE(sys_format));

    for (src = sys_format; *src; src += src_pat_len, dst += dst_pat_len) {
        src_pat_len = dst_pat_len = get_pattern_len(src, L"Hhmst");

        switch (*src)
        {
        case '\'':
            wmemcpy(dst, src, src_pat_len);
            break;

        case 'H':
        case 'h':
        case 'm':
        case 's':
            if (src_pat_len == 1) /* without leading zeros */
                dst_pat_len++; /* -> with leading zeros */
            /* fallthrough */
        case 't':
            wmemset(dst, *src, dst_pat_len);
            break;

        default:
            *dst = *src;
            break;
        }
    }
    *dst = '\0';

    /* seconds portion will be dropped by TIME_NOSECONDS */
    TRACE("time format: %s\n", wine_dbgstr_w(time_format));
}

/*****************************************************************************
 * WCMD_directory
 *
 * List a file directory.
 *
 */

RETURN_CODE WCMD_directory(WCHAR *args)
{
  WCHAR path[MAX_PATH], cwd[MAX_PATH];
  DWORD status;
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  WCHAR *p;
  WCHAR string[MAXSTRING];
  int   argno         = 0;
  WCHAR *argN          = args;
  WCHAR  lastDrive;
  BOOL  trailerReqd = FALSE;
  DIRECTORY_STACK *fullParms = NULL;
  DIRECTORY_STACK *prevEntry = NULL;
  DIRECTORY_STACK *thisEntry = NULL;
  WCHAR drive[10];
  WCHAR dir[MAX_PATH];
  WCHAR fname[MAX_PATH];
  WCHAR ext[MAX_PATH];
  unsigned num_empty = 0, num_with_data = 0;
  RETURN_CODE return_code = NO_ERROR;

  /* Prefill quals with (uppercased) DIRCMD env var */
  if (GetEnvironmentVariableW(L"DIRCMD", string, ARRAY_SIZE(string))) {
    wcsupr( string );
    lstrcatW(string,quals);
    lstrcpyW(quals, string);
  }

  byte_total = 0;
  file_total = dir_total = 0;

  /* Initialize all flags to their defaults as if no DIRCMD or quals */
  paged_mode = FALSE;
  recurse    = FALSE;
  wide       = FALSE;
  bare       = FALSE;
  lower      = FALSE;
  shortname  = FALSE;
  usernames  = FALSE;
  orderByCol = FALSE;
  separator  = TRUE;
  dirTime = Written;
  dirOrder = Unspecified;
  orderReverse = FALSE;
  orderGroupDirs = FALSE;
  orderGroupDirsReverse = FALSE;
  showattrs  = 0;
  attrsbits  = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;

  /* Handle args - Loop through so right most is the effective one */
  /* Note: /- appears to be a negate rather than an off, eg. dir
           /-W is wide, or dir /w /-w /-w is also wide             */
  p = quals;
  while (*p && (*p=='/' || *p==' ')) {
    BOOL negate = FALSE;
    if (*p++==' ') continue;  /* Skip / and blanks introduced through DIRCMD */

    if (*p=='-') {
      negate = TRUE;
      p++;
    }

    WINE_TRACE("Processing arg '%c' (in %s)\n", *p, wine_dbgstr_w(quals));
    switch (*p) {
    case 'P': if (negate) paged_mode = !paged_mode;
              else paged_mode = TRUE;
              break;
    case 'S': if (negate) recurse = !recurse;
              else recurse = TRUE;
              break;
    case 'W': if (negate) wide = !wide;
              else wide = TRUE;
              break;
    case 'B': if (negate) bare = !bare;
              else bare = TRUE;
              break;
    case 'L': if (negate) lower = !lower;
              else lower = TRUE;
              break;
    case 'X': if (negate) shortname = !shortname;
              else shortname = TRUE;
              break;
    case 'Q': if (negate) usernames = !usernames;
              else usernames = TRUE;
              break;
    case 'D': if (negate) orderByCol = !orderByCol;
              else orderByCol = TRUE;
              break;
    case 'C': if (negate) separator = !separator;
              else separator = TRUE;
              break;
    case 'T': p = p + 1;
              if (*p==':') p++;  /* Skip optional : */

              if (*p == 'A') dirTime = Access;
              else if (*p == 'C') dirTime = Creation;
              else if (*p == 'W') dirTime = Written;

              /* Support /T and /T: with no parms, default to written */
              else if (*p == 0x00 || *p == '/') {
                dirTime = Written;
                p = p - 1; /* So when step on, move to '/' */
              } else {
                SetLastError(return_code = ERROR_INVALID_PARAMETER);
                WCMD_print_error();
                goto exit;
              }
              break;
    case 'O': /* Reset order state for each occurrence of /O, i.e. if DIRCMD contains /O and user
                 also specified /O on the command line. */
              dirOrder = Unspecified;
              orderGroupDirs = FALSE;
              orderReverse = FALSE;
              orderGroupDirsReverse = FALSE;
              p = p + 1;
              if (*p==':') p++;  /* Skip optional : */
              while (*p && *p != '/') {
                WINE_TRACE("Processing subparm '%c' (in %s)\n", *p, wine_dbgstr_w(quals));
                /* Options N,E,S,D are mutually-exclusive, first encountered takes precedence. */
                switch (*p) {
                case 'N': if (dirOrder == Unspecified) dirOrder = Name;       break;
                case 'E': if (dirOrder == Unspecified) dirOrder = Extension;  break;
                case 'S': if (dirOrder == Unspecified) dirOrder = Size;       break;
                case 'D': if (dirOrder == Unspecified) dirOrder = Date;       break;
                case '-': if (dirOrder == Unspecified) {
                            if (*(p+1)=='G') orderGroupDirsReverse=TRUE;
                            else if (*(p+1)=='N'||*(p+1)=='E'||*(p+1)=='S'||*(p+1)=='D') orderReverse = TRUE;
                          }
                          break;
                case 'G': orderGroupDirs = TRUE; break;
                default:
                    SetLastError(return_code = ERROR_INVALID_PARAMETER);
                    WCMD_print_error();
                    goto exit;
                }
                p++;
              }
              /* Handle default case of /O specified by itself, with no specific options.
                 This is equivalent to /O:GN. */
              if (dirOrder == Unspecified && !orderGroupDirs) {
                orderGroupDirs = TRUE;
                orderGroupDirsReverse = FALSE;
                dirOrder = Name;
              }
              p = p - 1; /* So when step on, move to '/' */
              break;
    case 'A': p = p + 1;
              showattrs = 0;
              attrsbits = 0;
              if (*p==':') p++;  /* Skip optional : */
              while (*p && *p != '/') {
                BOOL anegate = FALSE;
                ULONG mask;

                /* Note /A: - options are 'offs' not toggles */
                if (*p=='-') {
                  anegate = TRUE;
                  p++;
                }

                WINE_TRACE("Processing subparm '%c' (in %s)\n", *p, wine_dbgstr_w(quals));
                switch (*p) {
                case 'D': mask = FILE_ATTRIBUTE_DIRECTORY; break;
                case 'H': mask = FILE_ATTRIBUTE_HIDDEN;    break;
                case 'S': mask = FILE_ATTRIBUTE_SYSTEM;    break;
                case 'R': mask = FILE_ATTRIBUTE_READONLY;  break;
                case 'A': mask = FILE_ATTRIBUTE_ARCHIVE;   break;
                default:
                    SetLastError(return_code = ERROR_INVALID_PARAMETER);
                    WCMD_print_error();
                    goto exit;
                }

                /* Keep running list of bits we care about */
                attrsbits |= mask;

                /* Mask shows what MUST be in the bits we care about */
                if (anegate) showattrs = showattrs & ~mask;
                else showattrs |= mask;

                p++;
              }
              p = p - 1; /* So when step on, move to '/' */
              WINE_TRACE("Result: showattrs %lx, bits %lx\n", showattrs, attrsbits);
              break;
    default:
              SetLastError(return_code = ERROR_INVALID_PARAMETER);
              WCMD_print_error();
              goto exit;
    }
    p = p + 1;
  }

  /* Handle conflicting args and initialization */
  if (bare || shortname) wide = FALSE;
  if (bare) shortname = FALSE;
  if (wide) usernames = FALSE;
  if (orderByCol) wide = TRUE;

  if (wide) {
      if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo))
          max_width = consoleInfo.dwSize.X;
      else
          max_width = 80;
  }
  if (paged_mode) {
     WCMD_enter_paged_mode(NULL);
  }

  init_date_format();
  init_time_format();

  argno         = 0;
  argN          = args;
  GetCurrentDirectoryW(MAX_PATH, cwd);
  lstrcatW(cwd, L"\\");

  /* Loop through all args, calculating full effective directory */
  fullParms = NULL;
  prevEntry = NULL;
  while (argN) {
    WCHAR fullname[MAXSTRING];
    WCHAR *thisArg = WCMD_parameter(args, argno++, &argN, FALSE, FALSE);
    if (argN && argN[0] != '/') {

      WINE_TRACE("Found parm '%s'\n", wine_dbgstr_w(thisArg));
      if (thisArg[1] == ':' && thisArg[2] == '\\') {
        lstrcpyW(fullname, thisArg);
      } else if (thisArg[1] == ':' && thisArg[2] != '\\') {
        WCHAR envvar[4];
        wsprintfW(envvar, L"=%c:", thisArg[0]);
        if (!GetEnvironmentVariableW(envvar, fullname, MAX_PATH)) {
          wsprintfW(fullname, L"%c:", thisArg[0]);
        }
        lstrcatW(fullname, L"\\");
        lstrcatW(fullname, &thisArg[2]);
      } else if (thisArg[0] == '\\') {
        memcpy(fullname, cwd, 2 * sizeof(WCHAR));
        lstrcpyW(fullname+2, thisArg);
      } else {
        lstrcpyW(fullname, cwd);
        lstrcatW(fullname, thisArg);
      }
      WINE_TRACE("Using location '%s'\n", wine_dbgstr_w(fullname));

      if (!WCMD_get_fullpath(fullname, ARRAY_SIZE(path), path, NULL)) continue;

      /*
       *  If the path supplied does not include a wildcard, and the endpoint of the
       *  path references a directory, we need to list the *contents* of that
       *  directory not the directory file itself.
       */
      if ((wcschr(path, '*') == NULL) && (wcschr(path, '%') == NULL)) {
        status = GetFileAttributesW(path);
        if ((status != INVALID_FILE_ATTRIBUTES) && (status & FILE_ATTRIBUTE_DIRECTORY)) {
          if (!ends_with_backslash(path)) lstrcatW(path, L"\\");
          lstrcatW(path, L"*");
        }
      } else {
        /* Special case wildcard search with no extension (ie parameters ending in '.') as
           GetFullPathName strips off the additional '.'                                  */
        if (fullname[lstrlenW(fullname)-1] == '.') lstrcatW(path, L".");
      }

      WINE_TRACE("Using path '%s'\n", wine_dbgstr_w(path));
      thisEntry = xalloc(sizeof(DIRECTORY_STACK));
      if (fullParms == NULL) fullParms = thisEntry;
      if (prevEntry != NULL) prevEntry->next = thisEntry;
      prevEntry = thisEntry;
      thisEntry->next = NULL;

      /* Split into components */
      _wsplitpath(path, drive, dir, fname, ext);
      WINE_TRACE("Path Parts: drive: '%s' dir: '%s' name: '%s' ext:'%s'\n",
                 wine_dbgstr_w(drive), wine_dbgstr_w(dir),
                 wine_dbgstr_w(fname), wine_dbgstr_w(ext));

      thisEntry->dirName = xalloc(sizeof(WCHAR) * (wcslen(drive) + wcslen(dir) + 1));
      lstrcpyW(thisEntry->dirName, drive);
      lstrcatW(thisEntry->dirName, dir);

      thisEntry->fileName = xalloc(sizeof(WCHAR) * (wcslen(fname) + wcslen(ext) + 1));
      lstrcpyW(thisEntry->fileName, fname);
      lstrcatW(thisEntry->fileName, ext);

    }
  }

  /* If just 'dir' entered, a '*' parameter is assumed */
  if (fullParms == NULL) {
    WINE_TRACE("Inserting default '*'\n");
    fullParms = xalloc(sizeof(DIRECTORY_STACK));
    fullParms->next = NULL;
    fullParms->dirName = xstrdupW(cwd);
    fullParms->fileName = xstrdupW(L"*");
  }

  lastDrive = '?';
  prevEntry = NULL;
  thisEntry = fullParms;
  trailerReqd = FALSE;

  while (thisEntry != NULL && return_code == NO_ERROR) {

    /* Output disk free (trailer) and volume information (header) if the drive
       letter changes */
    if (lastDrive != towupper(thisEntry->dirName[0])) {

      /* Trailer Information */
      if (lastDrive != '?') {
        trailerReqd = FALSE;
        if (return_code == NO_ERROR)
            WCMD_dir_trailer(prevEntry->dirName);
        byte_total = file_total = dir_total = 0;
      }

      lastDrive = towupper(thisEntry->dirName[0]);

      if (!bare) {
         WCHAR drive[4];
         WINE_TRACE("Writing volume for '%c:'\n", thisEntry->dirName[0]);
         drive[0] = thisEntry->dirName[0];
         drive[1] = thisEntry->dirName[1];
         drive[2] = L'\\';
         drive[3] = L'\0';
         trailerReqd = TRUE;
         if (!WCMD_print_volume_information(drive)) {
           return_code = ERROR_INVALID_PARAMETER;
           goto exit;
         }
      }
    } else {
      if (!bare) WCMD_output_asis (L"\n\n");
    }

    prevEntry = thisEntry;
    return_code = WCMD_list_directory (thisEntry, 0, &thisEntry);
    if (return_code == ERROR_FILE_NOT_FOUND)
      num_empty++;
    else
      num_with_data++;
  }

  /* Trailer Information */
  if (trailerReqd && return_code == NO_ERROR) {
    WCMD_dir_trailer(prevEntry->dirName);
  }

exit:
  if (return_code == STATUS_CONTROL_C_EXIT)
      errorlevel = ERROR_INVALID_FUNCTION;
  else if (return_code != NO_ERROR || (num_empty && !num_with_data))
      return_code = errorlevel = ERROR_INVALID_FUNCTION;
  else
      errorlevel = NO_ERROR;

  if (paged_mode) WCMD_leave_paged_mode();

  /* Free storage allocated for parms */
  while (fullParms != NULL) {
    prevEntry = fullParms;
    fullParms = prevEntry->next;
    free(prevEntry->dirName);
    free(prevEntry->fileName);
    free(prevEntry);
  }

  return return_code;
}
