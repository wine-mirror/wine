/*
 * WCMD - Wine-compatible command line interface - Directory functions.
 *
 * (C) 1999 D A Pickles
 *
 * On entry, global variables quals, param1, param2 contain
 * the qualifiers (uppercased and concatenated) and parameters entered, with
 * environment-variable and batch parameter substitution already done.
 */

/*
 * FIXME:
 * - 32-bit limit on individual file sizes (directories and free space are 64-bit)
 * - DIR /S fails if the starting directory is not the current default.
 */

#include "wcmd.h"

int WCMD_dir_sort (const void *a, const void *b);
void WCMD_list_directory (char *path, int level);
char * WCMD_filesize64 (__int64 n);
char * WCMD_filesize32 (int n);
char * WCMD_strrev (char *buff);


extern char nyi[];
extern char newline[];
extern char version_string[];
extern char anykey[];
extern int echo_mode;
extern char quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];

int file_total, dir_total, line_count, page_mode, recurse;
__int64 byte_total;

/*****************************************************************************
 * WCMD_directory
 *
 * List a file directory.
 * FIXME: /S switch only works for the current directory
 *
 */

void WCMD_directory () {

char path[MAX_PATH], drive[8];
int status;
__int64 free_space;
DWORD spc, bps, fc, capacity;

  line_count = 5;
  page_mode = (strstr(quals, "/P") != NULL);
  recurse = (strstr(quals, "/S") != NULL);
  if (param1[0] == '\0') strcpy (param1, ".");
  GetFullPathName (param1, sizeof(path), path, NULL);
  lstrcpyn (drive, path, 3);
  status = WCMD_volume (0, drive);
  if (!status) {
    return;
  }
  WCMD_list_directory (path, 0);
  lstrcpyn (drive, path, 4);
  GetDiskFreeSpace (drive, &spc, &bps, &fc, &capacity);
  free_space = bps * spc * fc;
  WCMD_output (" %18s bytes free\n\n", WCMD_filesize64 (free_space));
  if (recurse) {
    WCMD_output ("Total files listed:\n%8d files%25s bytes\n%8d directories\n\n",
    	 file_total, WCMD_filesize64 (byte_total), dir_total);
  }
}

/*****************************************************************************
 * WCMD_list_directory
 *
 * List a single file directory. This function (and those below it) can be called
 * recursively when the /S switch is used.
 *
 * FIXME: Assumes individual files are less than 2**32 bytes.
 * FIXME: Entries sorted by name only. Should we support DIRCMD??
 * FIXME: Assumes 24-line display for the /P qualifier.
 * FIXME: Other command qualifiers not supported.
 * FIXME: DIR /S FILENAME fails if at least one matching file is not found in the top level. 
 */

void WCMD_list_directory (char *search_path, int level) {

char string[1024], datestring[32], timestring[32];
char mem_err[] = "Memory Allocation Error";
char *p;
DWORD count;
WIN32_FIND_DATA *fd;
FILETIME ft;
SYSTEMTIME st;
HANDLE hff;
int status, dir_count, file_count, entry_count, i;
__int64 byte_count;

  dir_count = 0;
  file_count = 0;
  entry_count = 0;
  byte_count = 0;

/*
 *  If the path supplied does not include a wildcard, and the endpoint of the
 *  path references a directory, we need to list the *contents* of that
 *  directory not the directory file itself.
 */

  if ((strchr(search_path, '*') == NULL) && (strchr(search_path, '%') == NULL)) {
    status = GetFileAttributes (search_path);
    if ((status != -1) && (status & FILE_ATTRIBUTE_DIRECTORY)) {
      if (search_path[strlen(search_path)-1] == '\\') {
        strcat (search_path, "*");
      }
      else {
        strcat (search_path, "\\*");
      }
    }
  }

  fd = malloc (sizeof(WIN32_FIND_DATA));
  hff = FindFirstFile (search_path, fd);
  if (hff == INVALID_HANDLE_VALUE) {
    WCMD_output ("File Not Found\n");
    free (fd);
    return;
  }
  do {
    entry_count++;
    fd = realloc (fd, (entry_count+1)*sizeof(WIN32_FIND_DATA));
    if (fd == NULL) {
      FindClose (hff);
      WCMD_output (mem_err);
       return;
    }
  } while (FindNextFile(hff, (fd+entry_count)) != 0);
  FindClose (hff);
  qsort (fd, entry_count, sizeof(WIN32_FIND_DATA), WCMD_dir_sort);
  if (level != 0) WCMD_output ("\n\n");
  WCMD_output ("Directory of %s\n\n", search_path);
  if (page_mode) {
    line_count += 2;
    if (line_count > 23) {
      line_count = 0;
      WCMD_output (anykey);
      ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
    }
  }
  for (i=0; i<entry_count; i++) {
    FileTimeToLocalFileTime (&(fd+i)->ftLastWriteTime, &ft);
    FileTimeToSystemTime (&ft, &st);
    GetDateFormat (0, DATE_SHORTDATE, &st, NULL, datestring,
      		sizeof(datestring));
    GetTimeFormat (0, TIME_NOSECONDS, &st,
      		NULL, timestring, sizeof(timestring));
    if ((fd+i)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      dir_count++;
      WCMD_output ("%8s  %8s   <DIR>        %s\n",
      	  datestring, timestring, (fd+i)->cFileName);
    }
    else {
      file_count++;
      byte_count += (fd+i)->nFileSizeLow;
      WCMD_output ("%8s  %8s    %10s  %s\n",
     	  datestring, timestring,
	  WCMD_filesize32((fd+i)->nFileSizeLow), (fd+i)->cFileName);
    }
    if (page_mode) {
      if (++line_count > 23) {
        line_count = 0;
        WCMD_output (anykey);
        ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
      }
    }
  }
  if (file_count == 1) {
    WCMD_output ("       1 file %25s bytes\n", WCMD_filesize64 (byte_count));
  }
  else {
    WCMD_output ("%8d files %24s bytes\n", file_count, WCMD_filesize64 (byte_count));
  }
  if (page_mode) {
    if (++line_count > 23) {
      line_count = 0;
      WCMD_output (anykey);
      ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
    }
  }
  byte_total = byte_total + byte_count;
  file_total = file_total + file_count;
  dir_total = dir_total + dir_count;
  if (dir_count == 1) WCMD_output ("1 directory         ");
  else WCMD_output ("%8d directories", dir_count);
  if (page_mode) {
    if (++line_count > 23) {
      line_count = 0;
      WCMD_output (anykey);
      ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
    }
  }
  for (i=0; i<entry_count; i++) {
    if ((recurse) &&
          ((fd+i)->cFileName[0] != '.') &&
      	  ((fd+i)->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
#if 0
      GetFullPathName ((fd+i)->cFileName, sizeof(string), string, NULL);
#endif
      p = strrchr (search_path, '\\');
      lstrcpyn (string, search_path, (p-search_path+2));
      lstrcat (string, (fd+i)->cFileName);
      lstrcat (string, p);
      WCMD_list_directory (string, 1);
    }
  }
  free (fd);
  return;
}

/*****************************************************************************
 * WCMD_filesize64
 *
 * Convert a 64-bit number into a character string, with commas every three digits.
 * Result is returned in a static string overwritten with each call.
 * FIXME: There must be a better algorithm!
 */

char * WCMD_filesize64 (__int64 n) {

__int64 q;
int r, i;
char *p;
static char buff[32];

  p = buff;
  i = -3;
  do {
    if ((++i)%3 == 1) *p++ = ',';
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
 * WCMD_filesize32
 *
 * Convert a 32-bit number into a character string, with commas every three digits.
 * Result is returned in a static string overwritten with each call.
 * FIXME: There must be a better algorithm!
 */

char * WCMD_filesize32 (int n) {

int r, i;
char *p, *q;
static char buff1[16], buff2[16];

  wsprintf (buff1, "%i", n);
  r = lstrlen (buff1);
  WCMD_strrev (buff1);
  p = buff1;
  q = buff2;
  for (i=0; i<r; i++) {
  if ((i-2)%3 == 1) *q++ = ',';
  *q++ = *p++;
  }
  *q = '\0';
  WCMD_strrev (buff2);
  return buff2;
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


int WCMD_dir_sort (const void *a, const void *b) {

  return (lstrcmpi(((WIN32_FIND_DATA *)a)->cFileName,
  	((WIN32_FIND_DATA *)b)->cFileName));
}

