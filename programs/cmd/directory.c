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
    Name = 0,
    Extension,
    Size,
    Date
} DISPLAYORDER;

static int file_total, dir_total, max_width;
static ULONGLONG byte_total;
static DISPLAYTIME dirTime;
static DISPLAYORDER dirOrder;
static BOOL orderReverse, orderGroupDirs, orderGroupDirsReverse, orderByCol;
static BOOL paged_mode, recurse, wide, bare, lower, shortname, usernames, separator;
static ULONG showattrs, attrsbits;

/*****************************************************************************
 * WCMD_strrev
 *
 * Reverse a WCHARacter string in-place (strrev() is not available under unixen :-( ).
 */
static WCHAR * WCMD_strrev (WCHAR *buff) {

  int r, i;
  WCHAR b;

  r = strlenW (buff);
  for (i=0; i<r/2; i++) {
    b = buff[i];
    buff[i] = buff[r-i-1];
    buff[r-i-1] = b;
  }
  return (buff);
}

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
  WCMD_strrev (buff);
  return buff;
}

/*****************************************************************************
 * WCMD_dir_sort
 *
 * Sort based on the /O options supplied on the command line
 */
static int WCMD_dir_sort (const void *a, const void *b)
{
  const WIN32_FIND_DATAW *filea = (const WIN32_FIND_DATAW *)a;
  const WIN32_FIND_DATAW *fileb = (const WIN32_FIND_DATAW *)b;
  int result = 0;

  /* If /OG or /O-G supplied, dirs go at the top or bottom, ignoring the
     requested sort order for the directory components                   */
  if (orderGroupDirs &&
      ((filea->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
       (fileb->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
  {
    BOOL aDir = filea->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    if (aDir) result = -1;
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
      WCMD_splitpath(filea->cFileName, drive, dir, fname, extA);
      WCMD_splitpath(fileb->cFileName, drive, dir, fname, extB);
      result = lstrcmpiW(extA, extB);
  }

  if (orderReverse) result = -result;
  return result;
}

/*****************************************************************************
 * WCMD_getfileowner
 *
 * Reverse a WCHARacter string in-place (strrev() is not available under unixen :-( ).
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

        secBuffer = heap_alloc(sizeNeeded * sizeof(BYTE));

        /* Get the owners security descriptor */
        if(!GetFileSecurityW(filename, OWNER_SECURITY_INFORMATION, secBuffer,
                            sizeNeeded, &sizeNeeded)) {
            heap_free(secBuffer);
            return;
        }

        /* Get the SID from the SD */
        if(!GetSecurityDescriptorOwner(secBuffer, &pSID, &defaulted)) {
            heap_free(secBuffer);
            return;
        }

        /* Convert to a username */
        if (LookupAccountSidW(NULL, pSID, name, &nameLen, domain, &domainLen, &nameuse)) {
            static const WCHAR fmt[]  = {'%','s','%','c','%','s','\0'};
            snprintfW(owner, ownerlen, fmt, domain, '\\', name);
        }
        heap_free(secBuffer);
    }
    return;
}

/*****************************************************************************
 * WCMD_list_directory
 *
 * List a single file directory. This function (and those below it) can be called
 * recursively when the /S switch is used.
 *
 * FIXME: Assumes 24-line display for the /P qualifier.
 */

static DIRECTORY_STACK *WCMD_list_directory (DIRECTORY_STACK *inputparms, int level) {

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

  static const WCHAR fmtDir[]  = {'%','1','!','1','0','s','!',' ',' ','%','2','!','8','s','!',' ',' ',
                                  '<','D','I','R','>',' ',' ',' ',' ',' ',' ',' ',' ',' ','\0'};
  static const WCHAR fmtFile[] = {'%','1','!','1','0','s','!',' ',' ','%','2','!','8','s','!',' ',' ',
                                  ' ',' ','%','3','!','1','0','s','!',' ',' ','\0'};
  static const WCHAR fmt2[]  = {'%','1','!','-','1','3','s','!','\0'};
  static const WCHAR fmt3[]  = {'%','1','!','-','2','3','s','!','\0'};
  static const WCHAR fmt4[]  = {'%','1','\0'};
  static const WCHAR fmt5[]  = {'%','1','%','2','\0'};

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
  fd = heap_alloc(sizeof(WIN32_FIND_DATAW));
  while (parms && strcmpW(inputparms->dirName, parms->dirName) == 0) {
    concurrentDirs++;

    /* Work out the full path + filename */
    strcpyW(real_path, parms->dirName);
    strcatW(real_path, parms->fileName);

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
           int tmpLen = strlenW(fd[entry_count-1].cFileName) + 3;
           if (fd[entry_count-1].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) tmpLen = tmpLen + 2;
           if (tmpLen > widest) widest = tmpLen;
        }

        fd = HeapReAlloc(GetProcessHeap(),0,fd,(entry_count+1)*sizeof(WIN32_FIND_DATAW));
        if (fd == NULL) {
          FindClose (hff);
          WINE_ERR("Out of memory\n");
          errorlevel = 1;
          return parms->next;
        }
      } while (FindNextFileW(hff, &fd[entry_count]) != 0);
      FindClose (hff);
    }

    /* Work out the actual current directory name without a trailing \ */
    strcpyW(real_path, parms->dirName);
    real_path[strlenW(parms->dirName)-1] = 0x00;

    /* Output the results */
    if (!bare) {
       if (level != 0 && (entry_count > 0)) WCMD_output_asis (newlineW);
       if (!recurse || ((entry_count > 0) && done_header==FALSE)) {
           static const WCHAR headerW[] = {'D','i','r','e','c','t','o','r','y',' ','o','f',
                                           ' ','%','1','\n','\n','\0'};
           WCMD_output (headerW, real_path);
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
      numCols = max(1, (int)max_width / widest);
      numRows = entry_count / numCols;
      if (entry_count % numCols) numRows++;
    } else {
      numCols = 1;
      numRows = entry_count;
    }
    WINE_TRACE("cols=%d, rows=%d\n", numCols, numRows);

    for (rows=0; rows<numRows; rows++) {
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
      if (lower) {
          WCHAR *p = fd[i].cFileName;
          while ( (*p = tolower(*p)) ) ++p;
      }

      /* /Q gets file ownership information */
      if (usernames) {
          strcpyW (string, inputparms->dirName);
          strcatW (string, fd[i].cFileName);
          WCMD_getfileowner(string, username, sizeof(username)/sizeof(WCHAR));
      }

      if (dirTime == Written) {
        FileTimeToLocalFileTime (&fd[i].ftLastWriteTime, &ft);
      } else if (dirTime == Access) {
        FileTimeToLocalFileTime (&fd[i].ftLastAccessTime, &ft);
      } else {
        FileTimeToLocalFileTime (&fd[i].ftCreationTime, &ft);
      }
      FileTimeToSystemTime (&ft, &st);
      GetDateFormatW(0, DATE_SHORTDATE, &st, NULL, datestring,
			sizeof(datestring)/sizeof(WCHAR));
      GetTimeFormatW(0, TIME_NOSECONDS, &st,
			NULL, timestring, sizeof(timestring)/sizeof(WCHAR));

      if (wide) {

        tmp_width = cur_width;
        if (fd[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            static const WCHAR fmt[] = {'[','%','1',']','\0'};
            WCMD_output (fmt, fd[i].cFileName);
            dir_count++;
            tmp_width = tmp_width + strlenW(fd[i].cFileName) + 2;
        } else {
            static const WCHAR fmt[] = {'%','1','\0'};
            WCMD_output (fmt, fd[i].cFileName);
            tmp_width = tmp_width + strlenW(fd[i].cFileName) ;
            file_count++;
            file_size.u.LowPart = fd[i].nFileSizeLow;
            file_size.u.HighPart = fd[i].nFileSizeHigh;
        byte_count.QuadPart += file_size.QuadPart;
        }
        cur_width = cur_width + widest;

        if ((cur_width + widest) > max_width) {
            cur_width = 0;
        } else {
            static const WCHAR padfmt[] = {'%','1','!','*','s','!','\0'};
            WCMD_output(padfmt, cur_width - tmp_width, nullW);
        }

      } else if (fd[i].dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        dir_count++;

        if (!bare) {
           WCMD_output (fmtDir, datestring, timestring);
           if (shortname) WCMD_output (fmt2, fd[i].cAlternateFileName);
           if (usernames) WCMD_output (fmt3, username);
           WCMD_output(fmt4,fd[i].cFileName);
        } else {
           if (!((strcmpW(fd[i].cFileName, dotW) == 0) ||
                 (strcmpW(fd[i].cFileName, dotdotW) == 0))) {
              WCMD_output (fmt5, recurse?inputparms->dirName:nullW, fd[i].cFileName);
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
           WCMD_output (fmtFile, datestring, timestring,
                        WCMD_filesize64(file_size.QuadPart));
           if (shortname) WCMD_output (fmt2, fd[i].cAlternateFileName);
           if (usernames) WCMD_output (fmt3, username);
           WCMD_output(fmt4,fd[i].cFileName);
        } else {
           WCMD_output (fmt5, recurse?inputparms->dirName:nullW, fd[i].cFileName);
        }
      }
     }
     if (addNewLine) WCMD_output_asis (newlineW);
     cur_width = 0;
    }

    if (!bare) {
       if (file_count == 1) {
         static const WCHAR fmt[] = {' ',' ',' ',' ',' ',' ',' ','1',' ','f','i','l','e',' ',
                                     '%','1','!','2','5','s','!',' ','b','y','t','e','s','\n','\0'};
         WCMD_output (fmt, WCMD_filesize64 (byte_count.QuadPart));
       }
       else {
         static const WCHAR fmt[] = {'%','1','!','8','d','!',' ','f','i','l','e','s',' ','%','2','!','2','4','s','!',
                                     ' ','b','y','t','e','s','\n','\0'};
         WCMD_output (fmt, file_count, WCMD_filesize64 (byte_count.QuadPart));
       }
    }
    byte_total = byte_total + byte_count.QuadPart;
    file_total = file_total + file_count;
    dir_total = dir_total + dir_count;

    if (!bare && !recurse) {
       if (dir_count == 1) {
           static const WCHAR fmt[] = {'%','1','!','8','d','!',' ','d','i','r','e','c','t','o','r','y',
                                       ' ',' ',' ',' ',' ',' ',' ',' ',' ','\0'};
           WCMD_output (fmt, 1);
       } else {
           static const WCHAR fmt[] = {'%','1','!','8','d','!',' ','d','i','r','e','c','t','o','r','i',
                                       'e','s','\0'};
           WCMD_output (fmt, dir_count);
       }
    }
  }
  heap_free(fd);

  /* When recursing, look in all subdirectories for matches */
  if (recurse) {
    DIRECTORY_STACK *dirStack = NULL;
    DIRECTORY_STACK *lastEntry = NULL;
    WIN32_FIND_DATAW finddata;

    /* Build path to search */
    strcpyW(string, inputparms->dirName);
    strcatW(string, starW);

    WINE_TRACE("Recursive, looking for '%s'\n", wine_dbgstr_w(string));
    hff = FindFirstFileW(string, &finddata);
    if (hff != INVALID_HANDLE_VALUE) {
      do {
        if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (strcmpW(finddata.cFileName, dotdotW) != 0) &&
            (strcmpW(finddata.cFileName, dotW) != 0)) {

          DIRECTORY_STACK *thisDir;
          int              dirsToCopy = concurrentDirs;

          /* Loop creating list of subdirs for all concurrent entries */
          parms = inputparms;
          while (dirsToCopy > 0) {
            dirsToCopy--;

            /* Work out search parameter in sub dir */
            strcpyW (string, inputparms->dirName);
            strcatW (string, finddata.cFileName);
            strcatW (string, slashW);
            WINE_TRACE("Recursive, Adding to search list '%s'\n", wine_dbgstr_w(string));

            /* Allocate memory, add to list */
            thisDir = heap_alloc(sizeof(DIRECTORY_STACK));
            if (dirStack == NULL) dirStack = thisDir;
            if (lastEntry != NULL) lastEntry->next = thisDir;
            lastEntry = thisDir;
            thisDir->next = NULL;
            thisDir->dirName = heap_strdupW(string);
            thisDir->fileName = heap_strdupW(parms->fileName);
            parms = parms->next;
          }
        }
      } while (FindNextFileW(hff, &finddata) != 0);
      FindClose (hff);

      while (dirStack != NULL) {
        DIRECTORY_STACK *thisDir = dirStack;
        dirStack = WCMD_list_directory (thisDir, 1);
        while (thisDir != dirStack) {
          DIRECTORY_STACK *tempDir = thisDir->next;
          heap_free(thisDir->dirName);
          heap_free(thisDir->fileName);
          heap_free(thisDir);
          thisDir = tempDir;
        }
      }
    }
  }

  /* Handle case where everything is filtered out */
  if ((file_total + dir_total == 0) && (level == 0)) {
    SetLastError (ERROR_FILE_NOT_FOUND);
    WCMD_print_error ();
    errorlevel = 1;
  }

  return parms;
}

/*****************************************************************************
 * WCMD_dir_trailer
 *
 * Print out the trailer for the supplied drive letter
 */
static void WCMD_dir_trailer(WCHAR drive) {
    ULARGE_INTEGER avail, total, freebytes;
    DWORD status;
    WCHAR driveName[] = {'c',':','\\','\0'};

    driveName[0] = drive;
    status = GetDiskFreeSpaceExW(driveName, &avail, &total, &freebytes);
    WINE_TRACE("Writing trailer for '%s' gave %d(%d)\n", wine_dbgstr_w(driveName),
               status, GetLastError());

    if (errorlevel==0 && !bare) {
      if (recurse) {
        static const WCHAR fmt1[] = {'\n',' ',' ',' ',' ',' ','T','o','t','a','l',' ','f','i','l','e','s',
                                     ' ','l','i','s','t','e','d',':','\n','%','1','!','8','d','!',' ','f','i','l','e',
                                     's','%','2','!','2','5','s','!',' ','b','y','t','e','s','\n','\0'};
        static const WCHAR fmt2[] = {'%','1','!','8','d','!',' ','d','i','r','e','c','t','o','r','i','e','s',' ','%',
                                     '2','!','1','8','s','!',' ','b','y','t','e','s',' ','f','r','e','e','\n','\n',
                                     '\0'};
        WCMD_output (fmt1, file_total, WCMD_filesize64 (byte_total));
        WCMD_output (fmt2, dir_total, WCMD_filesize64 (freebytes.QuadPart));
      } else {
        static const WCHAR fmt[] = {' ','%','1','!','1','8','s','!',' ','b','y','t','e','s',' ','f','r','e','e',
                                    '\n','\n','\0'};
        WCMD_output (fmt, WCMD_filesize64 (freebytes.QuadPart));
      }
    }
}

/*****************************************************************************
 * WCMD_directory
 *
 * List a file directory.
 *
 */

void WCMD_directory (WCHAR *args)
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
  static const WCHAR dircmdW[] = {'D','I','R','C','M','D','\0'};

  errorlevel = 0;

  /* Prefill quals with (uppercased) DIRCMD env var */
  if (GetEnvironmentVariableW(dircmdW, string, sizeof(string)/sizeof(WCHAR))) {
    p = string;
    while ( (*p = toupper(*p)) ) ++p;
    strcatW(string,quals);
    strcpyW(quals, string);
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
  dirOrder = Name;
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
                SetLastError(ERROR_INVALID_PARAMETER);
                WCMD_print_error();
                errorlevel = 1;
                return;
              }
              break;
    case 'O': p = p + 1;
              if (*p==':') p++;  /* Skip optional : */
              while (*p && *p != '/') {
                WINE_TRACE("Processing subparm '%c' (in %s)\n", *p, wine_dbgstr_w(quals));
                switch (*p) {
                case 'N': dirOrder = Name;       break;
                case 'E': dirOrder = Extension;  break;
                case 'S': dirOrder = Size;       break;
                case 'D': dirOrder = Date;       break;
                case '-': if (*(p+1)=='G') orderGroupDirsReverse=TRUE;
                          else orderReverse = TRUE;
                          break;
                case 'G': orderGroupDirs = TRUE; break;
                default:
                    SetLastError(ERROR_INVALID_PARAMETER);
                    WCMD_print_error();
                    errorlevel = 1;
                    return;
                }
                p++;
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
                    SetLastError(ERROR_INVALID_PARAMETER);
                    WCMD_print_error();
                    errorlevel = 1;
                    return;
                }

                /* Keep running list of bits we care about */
                attrsbits |= mask;

                /* Mask shows what MUST be in the bits we care about */
                if (anegate) showattrs = showattrs & ~mask;
                else showattrs |= mask;

                p++;
              }
              p = p - 1; /* So when step on, move to '/' */
              WINE_TRACE("Result: showattrs %x, bits %x\n", showattrs, attrsbits);
              break;
    default:
              SetLastError(ERROR_INVALID_PARAMETER);
              WCMD_print_error();
              errorlevel = 1;
              return;
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

  argno         = 0;
  argN          = args;
  GetCurrentDirectoryW(MAX_PATH, cwd);
  strcatW(cwd, slashW);

  /* Loop through all args, calculating full effective directory */
  fullParms = NULL;
  prevEntry = NULL;
  while (argN) {
    WCHAR fullname[MAXSTRING];
    WCHAR *thisArg = WCMD_parameter(args, argno++, &argN, FALSE, FALSE);
    if (argN && argN[0] != '/') {

      WINE_TRACE("Found parm '%s'\n", wine_dbgstr_w(thisArg));
      if (thisArg[1] == ':' && thisArg[2] == '\\') {
        strcpyW(fullname, thisArg);
      } else if (thisArg[1] == ':' && thisArg[2] != '\\') {
        WCHAR envvar[4];
        static const WCHAR envFmt[] = {'=','%','c',':','\0'};
        wsprintfW(envvar, envFmt, thisArg[0]);
        if (!GetEnvironmentVariableW(envvar, fullname, MAX_PATH)) {
          static const WCHAR noEnvFmt[] = {'%','c',':','\0'};
          wsprintfW(fullname, noEnvFmt, thisArg[0]);
        }
        strcatW(fullname, slashW);
        strcatW(fullname, &thisArg[2]);
      } else if (thisArg[0] == '\\') {
        memcpy(fullname, cwd, 2 * sizeof(WCHAR));
        strcpyW(fullname+2, thisArg);
      } else {
        strcpyW(fullname, cwd);
        strcatW(fullname, thisArg);
      }
      WINE_TRACE("Using location '%s'\n", wine_dbgstr_w(fullname));

      status = GetFullPathNameW(fullname, sizeof(path)/sizeof(WCHAR), path, NULL);

      /*
       *  If the path supplied does not include a wildcard, and the endpoint of the
       *  path references a directory, we need to list the *contents* of that
       *  directory not the directory file itself.
       */
      if ((strchrW(path, '*') == NULL) && (strchrW(path, '%') == NULL)) {
        status = GetFileAttributesW(path);
        if ((status != INVALID_FILE_ATTRIBUTES) && (status & FILE_ATTRIBUTE_DIRECTORY)) {
          if (path[strlenW(path)-1] == '\\') {
            strcatW (path, starW);
          }
          else {
            static const WCHAR slashStarW[]  = {'\\','*','\0'};
            strcatW (path, slashStarW);
          }
        }
      } else {
        /* Special case wildcard search with no extension (ie parameters ending in '.') as
           GetFullPathName strips off the additional '.'                                  */
        if (fullname[strlenW(fullname)-1] == '.') strcatW(path, dotW);
      }

      WINE_TRACE("Using path '%s'\n", wine_dbgstr_w(path));
      thisEntry = heap_alloc(sizeof(DIRECTORY_STACK));
      if (fullParms == NULL) fullParms = thisEntry;
      if (prevEntry != NULL) prevEntry->next = thisEntry;
      prevEntry = thisEntry;
      thisEntry->next = NULL;

      /* Split into components */
      WCMD_splitpath(path, drive, dir, fname, ext);
      WINE_TRACE("Path Parts: drive: '%s' dir: '%s' name: '%s' ext:'%s'\n",
                 wine_dbgstr_w(drive), wine_dbgstr_w(dir),
                 wine_dbgstr_w(fname), wine_dbgstr_w(ext));

      thisEntry->dirName = heap_alloc(sizeof(WCHAR) * (strlenW(drive)+strlenW(dir)+1));
      strcpyW(thisEntry->dirName, drive);
      strcatW(thisEntry->dirName, dir);

      thisEntry->fileName = heap_alloc(sizeof(WCHAR) * (strlenW(fname)+strlenW(ext)+1));
      strcpyW(thisEntry->fileName, fname);
      strcatW(thisEntry->fileName, ext);

    }
  }

  /* If just 'dir' entered, a '*' parameter is assumed */
  if (fullParms == NULL) {
    WINE_TRACE("Inserting default '*'\n");
    fullParms = heap_alloc(sizeof(DIRECTORY_STACK));
    fullParms->next = NULL;
    fullParms->dirName = heap_strdupW(cwd);
    fullParms->fileName = heap_strdupW(starW);
  }

  lastDrive = '?';
  prevEntry = NULL;
  thisEntry = fullParms;
  trailerReqd = FALSE;

  while (thisEntry != NULL) {

    /* Output disk free (trailer) and volume information (header) if the drive
       letter changes */
    if (lastDrive != toupper(thisEntry->dirName[0])) {

      /* Trailer Information */
      if (lastDrive != '?') {
        trailerReqd = FALSE;
        WCMD_dir_trailer(prevEntry->dirName[0]);
      }

      lastDrive = toupper(thisEntry->dirName[0]);

      if (!bare) {
         WCHAR drive[3];

         WINE_TRACE("Writing volume for '%c:'\n", thisEntry->dirName[0]);
         memcpy(drive, thisEntry->dirName, 2 * sizeof(WCHAR));
         drive[2] = 0x00;
         status = WCMD_volume (0, drive);
         trailerReqd = TRUE;
         if (!status) {
           errorlevel = 1;
           goto exit;
         }
      }
    } else {
      static const WCHAR newLine2[] = {'\n','\n','\0'};
      if (!bare) WCMD_output_asis (newLine2);
    }

    /* Clear any errors from previous invocations, and process it */
    errorlevel = 0;
    prevEntry = thisEntry;
    thisEntry = WCMD_list_directory (thisEntry, 0);
  }

  /* Trailer Information */
  if (trailerReqd) {
    WCMD_dir_trailer(prevEntry->dirName[0]);
  }

exit:
  if (paged_mode) WCMD_leave_paged_mode();

  /* Free storage allocated for parms */
  while (fullParms != NULL) {
    prevEntry = fullParms;
    fullParms = prevEntry->next;
    heap_free(prevEntry->dirName);
    heap_free(prevEntry->fileName);
    heap_free(prevEntry);
  }
}
