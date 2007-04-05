/*
 * CMD - Wine-compatible command line interface - Directory functions.
 *
 * Copyright (C) 1999 D A Pickles
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

/*
 * NOTES:
 * On entry, global variables quals, param1, param2 contain
 * the qualifiers (uppercased and concatenated) and parameters entered, with
 * environment-variable and batch parameter substitution already done.
 */

#define WIN32_LEAN_AND_MEAN

#include "wcmd.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cmd);

int WCMD_dir_sort (const void *a, const void *b);
void WCMD_list_directory (char *path, int level);
char * WCMD_filesize64 (ULONGLONG free);
char * WCMD_strrev (char *buff);
static void WCMD_getfileowner(char *filename, char *owner, int ownerlen);

extern int echo_mode;
extern char quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
extern DWORD errorlevel;

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

struct directory_stack
{
  struct directory_stack *next;
  char  *name;
};

static int file_total, dir_total, recurse, wide, bare, max_width, lower;
static int shortname, usernames;
static ULONGLONG byte_total;
static DISPLAYTIME dirTime;
static DISPLAYORDER dirOrder;
static BOOL orderReverse, orderGroupDirs, orderGroupDirsReverse, orderByCol;
static BOOL separator;
static ULONG showattrs, attrsbits;

/*****************************************************************************
 * WCMD_directory
 *
 * List a file directory.
 *
 */

void WCMD_directory (void) {

  char path[MAX_PATH], drive[8];
  int status, paged_mode;
  ULARGE_INTEGER avail, total, free;
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  char *p;
  char string[MAXSTRING];

  errorlevel = 0;

  /* Prefill Quals with (uppercased) DIRCMD env var */
  if (GetEnvironmentVariable ("DIRCMD", string, sizeof(string))) {
    p = string;
    while ( (*p = toupper(*p)) ) ++p;
    strcat(string,quals);
    strcpy(quals, string);
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
  attrsbits  = 0;

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

    WINE_TRACE("Processing arg '%c' (in %s)\n", *p, quals);
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
                WINE_TRACE("Processing subparm '%c' (in %s)\n", *p, quals);
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

                WINE_TRACE("Processing subparm '%c' (in %s)\n", *p, quals);
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
     WCMD_enter_paged_mode();
  }

  if (param1[0] == '\0') strcpy (param1, ".");
  status = GetFullPathName (param1, sizeof(path), path, NULL);
  if (!status) {
    WCMD_print_error();
    if (paged_mode) WCMD_leave_paged_mode();
    errorlevel = 1;
    return;
  }
  lstrcpyn (drive, path, 3);

  if (!bare) {
     status = WCMD_volume (0, drive);
     if (!status) {
         if (paged_mode) WCMD_leave_paged_mode();
       errorlevel = 1;
       return;
     }
  }

  WCMD_list_directory (path, 0);
  lstrcpyn (drive, path, 4);
  GetDiskFreeSpaceEx (drive, &avail, &total, &free);

  if (errorlevel==0 && !bare) {
     if (recurse) {
       WCMD_output ("\n     Total files listed:\n%8d files%25s bytes\n",
            file_total, WCMD_filesize64 (byte_total));
       WCMD_output ("%8d directories %18s bytes free\n\n",
            dir_total, WCMD_filesize64 (free.QuadPart));
     } else {
       WCMD_output (" %18s bytes free\n\n", WCMD_filesize64 (free.QuadPart));
     }
  }
  if (paged_mode) WCMD_leave_paged_mode();
}

/*****************************************************************************
 * WCMD_list_directory
 *
 * List a single file directory. This function (and those below it) can be called
 * recursively when the /S switch is used.
 *
 * FIXME: Entries sorted by name only. Should we support DIRCMD??
 * FIXME: Assumes 24-line display for the /P qualifier.
 * FIXME: Other command qualifiers not supported.
 * FIXME: DIR /S FILENAME fails if at least one matching file is not found in the top level.
 */

void WCMD_list_directory (char *search_path, int level) {

  char string[1024], datestring[32], timestring[32];
  char *p;
  char real_path[MAX_PATH];
  WIN32_FIND_DATA *fd;
  FILETIME ft;
  SYSTEMTIME st;
  HANDLE hff;
  int status, dir_count, file_count, entry_count, i, widest, cur_width, tmp_width;
  int numCols, numRows;
  int rows, cols;
  ULARGE_INTEGER byte_count, file_size;

  dir_count = 0;
  file_count = 0;
  entry_count = 0;
  byte_count.QuadPart = 0;
  widest = 0;
  cur_width = 0;

/*
 *  If the path supplied does not include a wildcard, and the endpoint of the
 *  path references a directory, we need to list the *contents* of that
 *  directory not the directory file itself. However, only do this on first
 *  entry and not subsequent recursion
 */

  if ((level == 0) &&
      (strchr(search_path, '*') == NULL) && (strchr(search_path, '%') == NULL)) {
    status = GetFileAttributes (search_path);
    if ((status != INVALID_FILE_ATTRIBUTES) && (status & FILE_ATTRIBUTE_DIRECTORY)) {
      if (search_path[strlen(search_path)-1] == '\\') {
        strcat (search_path, "*");
      }
      else {
        strcat (search_path, "\\*");
      }
    }
  }

  /* Work out the actual current directory name */
  p = strrchr (search_path, '\\');
  memset(real_path, 0x00, sizeof(real_path));
  lstrcpyn (real_path, search_path, (p-search_path+2));

  /* Load all files into an in memory structure */
  fd = HeapAlloc(GetProcessHeap(),0,sizeof(WIN32_FIND_DATA));
  WINE_TRACE("Looking for matches to '%s'\n", search_path);
  hff = FindFirstFile (search_path, fd);
  if (hff == INVALID_HANDLE_VALUE) {
    entry_count = 0;
  } else {
    do {
      /* Skip any which are filtered out by attribute */
      if (((fd+entry_count)->dwFileAttributes & attrsbits) != showattrs) continue;

      entry_count++;

      /* Keep running track of longest filename for wide output */
      if (wide || orderByCol) {
         int tmpLen = strlen((fd+(entry_count-1))->cFileName) + 3;
         if ((fd+(entry_count-1))->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) tmpLen = tmpLen + 2;
         if (tmpLen > widest) widest = tmpLen;
      }

      fd = HeapReAlloc(GetProcessHeap(),0,fd,(entry_count+1)*sizeof(WIN32_FIND_DATA));
      if (fd == NULL) {
        FindClose (hff);
        WCMD_output ("Memory Allocation Error");
        errorlevel = 1;
        return;
      }
    } while (FindNextFile(hff, (fd+entry_count)) != 0);
    FindClose (hff);
  }

  /* Output the results */
  if (!bare) {
     if (level != 0 && (entry_count > 0)) WCMD_output ("\n");
     if ((entry_count > 0) || (!recurse && level == 0)) WCMD_output ("Directory of %s\n\n", real_path);
  }

  /* Handle case where everything is filtered out */
  if (entry_count > 0) {

    /* Sort the list of files */
    qsort (fd, entry_count, sizeof(WIN32_FIND_DATA), WCMD_dir_sort);

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
      char username[24];

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
          char *p = (fd+i)->cFileName;
          while ( (*p = tolower(*p)) ) ++p;
      }

      /* /Q gets file ownership information */
      if (usernames) {
          p = strrchr (search_path, '\\');
          lstrcpyn (string, search_path, (p-search_path+2));
          lstrcat (string, (fd+i)->cFileName);
          WCMD_getfileowner(string, username, sizeof(username));
      }

      if (dirTime == Written) {
        FileTimeToLocalFileTime (&(fd+i)->ftLastWriteTime, &ft);
      } else if (dirTime == Access) {
        FileTimeToLocalFileTime (&(fd+i)->ftLastAccessTime, &ft);
      } else {
        FileTimeToLocalFileTime (&(fd+i)->ftCreationTime, &ft);
      }
      FileTimeToSystemTime (&ft, &st);
      GetDateFormat (0, DATE_SHORTDATE, &st, NULL, datestring,
			sizeof(datestring));
      GetTimeFormat (0, TIME_NOSECONDS, &st,
			NULL, timestring, sizeof(timestring));

      if (wide) {

        tmp_width = cur_width;
        if ((fd+i)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            WCMD_output ("[%s]", (fd+i)->cFileName);
            dir_count++;
            tmp_width = tmp_width + strlen((fd+i)->cFileName) + 2;
        } else {
            WCMD_output ("%s", (fd+i)->cFileName);
            tmp_width = tmp_width + strlen((fd+i)->cFileName) ;
            file_count++;
            file_size.u.LowPart = (fd+i)->nFileSizeLow;
            file_size.u.HighPart = (fd+i)->nFileSizeHigh;
        byte_count.QuadPart += file_size.QuadPart;
        }
        cur_width = cur_width + widest;

        if ((cur_width + widest) > max_width) {
            cur_width = 0;
        } else {
            WCMD_output ("%*.s", (tmp_width - cur_width) ,"");
        }

      } else if ((fd+i)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        dir_count++;

        if (!bare) {
           WCMD_output ("%10s  %8s  <DIR>         ", datestring, timestring);
           if (shortname) WCMD_output ("%-13s", (fd+i)->cAlternateFileName);
           if (usernames) WCMD_output ("%-23s", username);
           WCMD_output("%s",(fd+i)->cFileName);
        } else {
           if (!((strcmp((fd+i)->cFileName, ".") == 0) ||
                 (strcmp((fd+i)->cFileName, "..") == 0))) {
              WCMD_output ("%s%s", recurse?real_path:"", (fd+i)->cFileName);
           } else {
              addNewLine = FALSE;
           }
        }
      }
      else {
        file_count++;
        file_size.u.LowPart = (fd+i)->nFileSizeLow;
        file_size.u.HighPart = (fd+i)->nFileSizeHigh;
        byte_count.QuadPart += file_size.QuadPart;
        if (!bare) {
           WCMD_output ("%10s  %8s    %10s  ", datestring, timestring,
                        WCMD_filesize64(file_size.QuadPart));
           if (shortname) WCMD_output ("%-13s", (fd+i)->cAlternateFileName);
           if (usernames) WCMD_output ("%-23s", username);
           WCMD_output("%s",(fd+i)->cFileName);
        } else {
           WCMD_output ("%s%s", recurse?real_path:"", (fd+i)->cFileName);
        }
      }
     }
     if (addNewLine) WCMD_output ("\n");
     cur_width = 0;
    }

    if (!bare) {
       if (file_count == 1) {
         WCMD_output ("       1 file %25s bytes\n", WCMD_filesize64 (byte_count.QuadPart));
       }
       else {
         WCMD_output ("%8d files %24s bytes\n", file_count, WCMD_filesize64 (byte_count.QuadPart));
       }
    }
    byte_total = byte_total + byte_count.QuadPart;
    file_total = file_total + file_count;
    dir_total = dir_total + dir_count;

    if (!bare && !recurse) {
       if (dir_count == 1) WCMD_output ("%8d directory         ", 1);
       else WCMD_output ("%8d directories", dir_count);
    }
  }
  HeapFree(GetProcessHeap(),0,fd);

  /* When recursing, look in all subdirectories for matches */
  if (recurse) {
    struct directory_stack *dirStack = NULL;
    struct directory_stack *lastEntry = NULL;
    WIN32_FIND_DATA finddata;

    /* Build path to search */
    strcpy(string, search_path);
    p = strrchr (string, '\\');
    strcpy(p+1, "*");

    WINE_TRACE("Recursive, looking for '%s'\n", string);
    hff = FindFirstFile (string, &finddata);
    if (hff != INVALID_HANDLE_VALUE) {
      do {
        if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (strcmp(finddata.cFileName, "..") != 0) &&
            (strcmp(finddata.cFileName, ".") != 0)) {

          struct directory_stack *thisDir;

          /* Work out search parameter in sub dir */
          p = strrchr (search_path, '\\');
          lstrcpyn (string, search_path, (p-search_path+2));
          string[(p-search_path+2)] = 0x00;
          lstrcat (string, finddata.cFileName);
          lstrcat (string, p);
          WINE_TRACE("Recursive, Adding to search list '%s'\n", string);

          /* Allocate memory, add to list */
          thisDir = (struct directory_stack *) HeapAlloc(GetProcessHeap(),0,sizeof(struct directory_stack));
          if (dirStack == NULL) dirStack = thisDir;
          if (lastEntry != NULL) lastEntry->next = thisDir;
          lastEntry = thisDir;
          thisDir->next = NULL;
          thisDir->name = HeapAlloc(GetProcessHeap(),0,(strlen(string)+1));
          strcpy(thisDir->name, string);
        }
      } while (FindNextFile(hff, &finddata) != 0);
      FindClose (hff);

      while (dirStack != NULL) {
        struct directory_stack *thisDir = dirStack;
        dirStack = thisDir->next;

        WCMD_list_directory (thisDir->name, 1);
        HeapFree(GetProcessHeap(),0,thisDir->name);
        HeapFree(GetProcessHeap(),0,thisDir);
      }
    }
  }

  /* Handle case where everything is filtered out */
  if ((file_total + dir_total == 0) && (level == 0)) {
    SetLastError (ERROR_FILE_NOT_FOUND);
    WCMD_print_error ();
    errorlevel = 1;
  }

  return;
}

/*****************************************************************************
 * WCMD_filesize64
 *
 * Convert a 64-bit number into a character string, with commas every three digits.
 * Result is returned in a static string overwritten with each call.
 * FIXME: There must be a better algorithm!
 */

char * WCMD_filesize64 (ULONGLONG n) {

  ULONGLONG q;
  unsigned int r, i;
  char *p;
  static char buff[32];

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
 * WCMD_strrev
 *
 * Reverse a character string in-place (strrev() is not available under unixen :-( ).
 */

char * WCMD_strrev (char *buff) {

  int r, i;
  char b;

  r = lstrlen (buff);
  for (i=0; i<r/2; i++) {
    b = buff[i];
    buff[i] = buff[r-i-1];
    buff[r-i-1] = b;
  }
  return (buff);
}


/*****************************************************************************
 * WCMD_dir_sort
 *
 * Sort based on the /O options supplied on the command line
 */
int WCMD_dir_sort (const void *a, const void *b)
{
  WIN32_FIND_DATA *filea = (WIN32_FIND_DATA *)a;
  WIN32_FIND_DATA *fileb = (WIN32_FIND_DATA *)b;
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
    result = lstrcmpi(filea->cFileName, fileb->cFileName);

  /* Order by Size: */
  } else if (dirOrder == Size) {
    ULONG64 sizea = (((ULONG64)filea->nFileSizeHigh) << 32) + filea->nFileSizeLow;
    ULONG64 sizeb = (((ULONG64)fileb->nFileSizeHigh) << 32) + fileb->nFileSizeLow;
    if( sizea < sizeb ) result = -1;
    else if( sizea == sizeb ) result = 0;
    else result = 1;

  /* Order by Date: (Takes into account which date (/T option) */
  } else if (dirOrder == Date) {

    FILETIME *ft;
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
      char drive[10];
      char dir[MAX_PATH];
      char fname[MAX_PATH];
      char extA[MAX_PATH];
      char extB[MAX_PATH];

      /* Split into components */
      WCMD_splitpath(filea->cFileName, drive, dir, fname, extA);
      WCMD_splitpath(fileb->cFileName, drive, dir, fname, extB);
      result = lstrcmpi(extA, extB);
  }

  if (orderReverse) result = -result;
  return result;
}

/*****************************************************************************
 * WCMD_getfileowner
 *
 * Reverse a character string in-place (strrev() is not available under unixen :-( ).
 */
void WCMD_getfileowner(char *filename, char *owner, int ownerlen) {

    ULONG sizeNeeded = 0;
    DWORD rc;
    char name[MAXSTRING];
    char domain[MAXSTRING];

    /* In case of error, return empty string */
    *owner = 0x00;

    /* Find out how much space we need for the owner security descritpor */
    GetFileSecurity(filename, OWNER_SECURITY_INFORMATION, 0, 0, &sizeNeeded);
    rc = GetLastError();

    if(rc == ERROR_INSUFFICIENT_BUFFER && sizeNeeded > 0) {

        LPBYTE secBuffer;
        PSID pSID = NULL;
        BOOL defaulted = FALSE;
        ULONG nameLen = MAXSTRING;
        ULONG domainLen = MAXSTRING;
        SID_NAME_USE nameuse;

        secBuffer = (LPBYTE) HeapAlloc(GetProcessHeap(),0,sizeNeeded * sizeof(BYTE));
        if(!secBuffer) return;

        /* Get the owners security descriptor */
        if(!GetFileSecurity(filename, OWNER_SECURITY_INFORMATION, secBuffer,
                            sizeNeeded, &sizeNeeded)) {
            HeapFree(GetProcessHeap(),0,secBuffer);
            return;
        }

        /* Get the SID from the SD */
        if(!GetSecurityDescriptorOwner(secBuffer, &pSID, &defaulted)) {
            HeapFree(GetProcessHeap(),0,secBuffer);
            return;
        }

        /* Convert to a username */
        if (LookupAccountSid(NULL, pSID, name, &nameLen, domain, &domainLen, &nameuse)) {
            snprintf(owner, ownerlen, "%s%c%s", domain, '\\', name);
        }
        HeapFree(GetProcessHeap(),0,secBuffer);
    }
    return;
}
