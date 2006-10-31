/*
 * CMD - Wine-compatible command line interface - built-in functions.
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
 * On entry to each function, global variables quals, param1, param2 contain
 * the qualifiers (uppercased and concatenated) and parameters entered, with
 * environment-variable and batch parameter substitution already done.
 */

/*
 * FIXME:
 * - No support for pipes, shell parameters
 * - Lots of functionality missing from builtins
 * - Messages etc need international support
 */

#define WIN32_LEAN_AND_MEAN

#include "wcmd.h"

void WCMD_execute (char *orig_command, char *parameter, char *substitution);

struct env_stack
{
  struct env_stack *next;
  WCHAR *strings;
};

struct env_stack *saved_environment;

extern HINSTANCE hinst;
extern char *inbuilt[];
extern int echo_mode, verify_mode;
extern char quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
extern BATCH_CONTEXT *context;
extern DWORD errorlevel;



/****************************************************************************
 * WCMD_clear_screen
 *
 * Clear the terminal screen.
 */

void WCMD_clear_screen (void) {

  /* Emulate by filling the screen from the top left to bottom right with
        spaces, then moving the cursor to the top left afterwards */
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

  if (GetConsoleScreenBufferInfo(hStdOut, &consoleInfo))
  {
      COORD topLeft;
      DWORD screenSize;

      screenSize = consoleInfo.dwSize.X * (consoleInfo.dwSize.Y + 1);

      topLeft.X = 0;
      topLeft.Y = 0;
      FillConsoleOutputCharacter(hStdOut, ' ', screenSize, topLeft, &screenSize);
      SetConsoleCursorPosition(hStdOut, topLeft);
  }
}

/****************************************************************************
 * WCMD_change_tty
 *
 * Change the default i/o device (ie redirect STDin/STDout).
 */

void WCMD_change_tty (void) {

  WCMD_output (nyi);

}

/****************************************************************************
 * WCMD_copy
 *
 * Copy a file or wildcarded set.
 * FIXME: No wildcard support
 */

void WCMD_copy (void) {

DWORD count;
WIN32_FIND_DATA fd;
HANDLE hff;
BOOL force, status;
static const char overwrite[] = "Overwrite file (Y/N)?";
char string[8], outpath[MAX_PATH], inpath[MAX_PATH], *infile;

  if (param1[0] == 0x00) {
    WCMD_output ("Argument missing\n");
    return;
  }

  if ((strchr(param1,'*') != NULL) && (strchr(param1,'%') != NULL)) {
    WCMD_output ("Wildcards not yet supported\n");
    return;
  }

  /* If no destination supplied, assume current directory */
  if (param2[0] == 0x00) {
      strcpy(param2, ".");
  }

  GetFullPathName (param2, sizeof(outpath), outpath, NULL);
  hff = FindFirstFile (outpath, &fd);
  if (hff != INVALID_HANDLE_VALUE) {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      GetFullPathName (param1, sizeof(inpath), inpath, &infile);
      strcat (outpath, "\\");
      strcat (outpath, infile);
    }
    FindClose (hff);
  }

  force = (strstr (quals, "/Y") != NULL);
  if (!force) {
    hff = FindFirstFile (outpath, &fd);
    if (hff != INVALID_HANDLE_VALUE) {
      FindClose (hff);
      WCMD_output (overwrite);
      ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
      if (toupper(string[0]) == 'Y') force = TRUE;
    }
    else force = TRUE;
  }
  if (force) {
    status = CopyFile (param1, outpath, FALSE);
    if (!status) WCMD_print_error ();
  }
}

/****************************************************************************
 * WCMD_create_dir
 *
 * Create a directory.
 *
 * this works recursivly. so mkdir dir1\dir2\dir3 will create dir1 and dir2 if
 * they do not already exist.
 */

BOOL create_full_path(CHAR* path)
{
    int len;
    CHAR *new_path;
    BOOL ret = TRUE;

    new_path = HeapAlloc(GetProcessHeap(),0,strlen(path)+1);
    strcpy(new_path,path);

    while ((len = strlen(new_path)) && new_path[len - 1] == '\\')
        new_path[len - 1] = 0;

    while (!CreateDirectory(new_path,NULL))
    {
        CHAR *slash;
        DWORD last_error = GetLastError();
        if (last_error == ERROR_ALREADY_EXISTS)
            break;

        if (last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if (!(slash = strrchr(new_path,'\\')) && ! (slash = strrchr(new_path,'/')))
        {
            ret = FALSE;
            break;
        }

        len = slash - new_path;
        new_path[len] = 0;
        if (!create_full_path(new_path))
        {
            ret = FALSE;
            break;
        }
        new_path[len] = '\\';
    }
    HeapFree(GetProcessHeap(),0,new_path);
    return ret;
}

void WCMD_create_dir (void) {

    if (param1[0] == 0x00) {
        WCMD_output ("Argument missing\n");
        return;
    }
    if (!create_full_path(param1)) WCMD_print_error ();
}

/****************************************************************************
 * WCMD_delete
 *
 * Delete a file or wildcarded set.
 *
 */

void WCMD_delete (int recurse) {

WIN32_FIND_DATA fd;
HANDLE hff;
char fpath[MAX_PATH];
char *p;

  if (param1[0] == 0x00) {
    WCMD_output ("Argument missing\n");
    return;
  }
  hff = FindFirstFile (param1, &fd);
  if (hff == INVALID_HANDLE_VALUE) {
    WCMD_output ("%s :File Not Found\n",param1);
    return;
  }
  if ((strchr(param1,'*') == NULL) && (strchr(param1,'?') == NULL)
  	&& (!recurse) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
    strcat (param1, "\\*");
    FindClose(hff);
    WCMD_delete (1);
    return;
  }
  if ((strchr(param1,'*') != NULL) || (strchr(param1,'?') != NULL)) {
    strcpy (fpath, param1);
    do {
      p = strrchr (fpath, '\\');
      if (p != NULL) {
        *++p = '\0';
        strcat (fpath, fd.cFileName);
      }
      else strcpy (fpath, fd.cFileName);
      if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (!DeleteFile (fpath)) WCMD_print_error ();
      }
    } while (FindNextFile(hff, &fd) != 0);
    FindClose (hff);
  }
  else {
    if (!DeleteFile (param1)) WCMD_print_error ();
    FindClose (hff);
  }
}

/****************************************************************************
 * WCMD_echo
 *
 * Echo input to the screen (or not). We don't try to emulate the bugs
 * in DOS (try typing "ECHO ON AGAIN" for an example).
 */

void WCMD_echo (const char *command) {

static const char eon[] = "Echo is ON\n", eoff[] = "Echo is OFF\n";
int count;

  if ((command[0] == '.') && (command[1] == 0)) {
    WCMD_output (newline);
    return;
  }
  if (command[0]==' ')
    command++;
  count = strlen(command);
  if (count == 0) {
    if (echo_mode) WCMD_output (eon);
    else WCMD_output (eoff);
    return;
  }
  if (lstrcmpi(command, "ON") == 0) {
    echo_mode = 1;
    return;
  }
  if (lstrcmpi(command, "OFF") == 0) {
    echo_mode = 0;
    return;
  }
  WCMD_output_asis (command);
  WCMD_output (newline);

}

/**************************************************************************
 * WCMD_for
 *
 * Batch file loop processing.
 * FIXME: We don't exhaustively check syntax. Any command which works in MessDOS
 * will probably work here, but the reverse is not necessarily the case...
 */

void WCMD_for (char *p) {

WIN32_FIND_DATA fd;
HANDLE hff;
char *cmd, *item;
char set[MAX_PATH], param[MAX_PATH];
int i;

  if (lstrcmpi (WCMD_parameter (p, 1, NULL), "in")
	|| lstrcmpi (WCMD_parameter (p, 3, NULL), "do")
	|| (param1[0] != '%')) {
    WCMD_output ("Syntax error\n");
    return;
  }
  lstrcpyn (set, WCMD_parameter (p, 2, NULL), sizeof(set));
  WCMD_parameter (p, 4, &cmd);
  lstrcpy (param, param1);

/*
 *	If the parameter within the set has a wildcard then search for matching files
 *	otherwise do a literal substitution.
 */

  i = 0;
  while (*(item = WCMD_parameter (set, i, NULL))) {
    if (strpbrk (item, "*?")) {
      hff = FindFirstFile (item, &fd);
      if (hff == INVALID_HANDLE_VALUE) {
	return;
      }
      do {
	WCMD_execute (cmd, param, fd.cFileName);
      } while (FindNextFile(hff, &fd) != 0);
      FindClose (hff);
}
    else {
      WCMD_execute (cmd, param, item);
    }
    i++;
  }
}

/*****************************************************************************
 * WCMD_Execute
 *
 *	Execute a command after substituting variable text for the supplied parameter
 */

void WCMD_execute (char *orig_cmd, char *param, char *subst) {

char *new_cmd, *p, *s, *dup;
int size;

  size = lstrlen (orig_cmd);
  new_cmd = (char *) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, size);
  dup = s = strdup (orig_cmd);

  while ((p = strstr (s, param))) {
    *p = '\0';
    size += lstrlen (subst);
    new_cmd = (char *) LocalReAlloc ((HANDLE)new_cmd, size, 0);
    strcat (new_cmd, s);
    strcat (new_cmd, subst);
    s = p + lstrlen (param);
  }
  strcat (new_cmd, s);
  WCMD_process_command (new_cmd);
  free (dup);
  LocalFree ((HANDLE)new_cmd);
}


/**************************************************************************
 * WCMD_give_help
 *
 *	Simple on-line help. Help text is stored in the resource file.
 */

void WCMD_give_help (char *command) {

int i;
char buffer[2048];

  command = WCMD_strtrim_leading_spaces(command);
  if (lstrlen(command) == 0) {
    LoadString (hinst, 1000, buffer, sizeof(buffer));
    WCMD_output_asis (buffer);
  }
  else {
    for (i=0; i<=WCMD_EXIT; i++) {
      if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
	  param1, -1, inbuilt[i], -1) == 2) {
	LoadString (hinst, i, buffer, sizeof(buffer));
	WCMD_output_asis (buffer);
	return;
      }
    }
    WCMD_output ("No help available for %s\n", param1);
  }
  return;
}

/****************************************************************************
 * WCMD_go_to
 *
 * Batch file jump instruction. Not the most efficient algorithm ;-)
 * Prints error message if the specified label cannot be found - the file pointer is
 * then at EOF, effectively stopping the batch file.
 * FIXME: DOS is supposed to allow labels with spaces - we don't.
 */

void WCMD_goto (void) {

char string[MAX_PATH];

  if (param1[0] == 0x00) {
    WCMD_output ("Argument missing\n");
    return;
  }
  if (context != NULL) {
    SetFilePointer (context -> h, 0, NULL, FILE_BEGIN);
    while (WCMD_fgets (string, sizeof(string), context -> h)) {
      if ((string[0] == ':') && (strcmp (&string[1], param1) == 0)) return;
    }
    WCMD_output ("Target to GOTO not found\n");
  }
  return;
}


/****************************************************************************
 * WCMD_if
 *
 * Batch file conditional.
 * FIXME: Much more syntax checking needed!
 */

void WCMD_if (char *p) {

int negate = 0, test = 0;
char condition[MAX_PATH], *command, *s;

  if (!lstrcmpi (param1, "not")) {
    negate = 1;
    lstrcpy (condition, param2);
}
  else {
    lstrcpy (condition, param1);
  }
  if (!lstrcmpi (condition, "errorlevel")) {
    if (errorlevel >= atoi(WCMD_parameter (p, 1+negate, NULL))) test = 1;
    return;
    WCMD_parameter (p, 2+negate, &command);
  }
  else if (!lstrcmpi (condition, "exist")) {
    if (GetFileAttributesA(WCMD_parameter (p, 1+negate, NULL)) != INVALID_FILE_ATTRIBUTES) {
        test = 1;
    }
    WCMD_parameter (p, 2+negate, &command);
  }
  else if ((s = strstr (p, "=="))) {
    s += 2;
    if (!lstrcmpi (condition, WCMD_parameter (s, 0, NULL))) test = 1;
    WCMD_parameter (s, 1, &command);
  }
  else {
    WCMD_output ("Syntax error\n");
    return;
  }
  if (test != negate) {
    command = strdup (command);
    WCMD_process_command (command);
    free (command);
  }
}

/****************************************************************************
 * WCMD_move
 *
 * Move a file, directory tree or wildcarded set of files.
 * FIXME: Needs input and output files to be fully specified.
 */

void WCMD_move (void) {

int status;
char outpath[MAX_PATH], inpath[MAX_PATH], *infile;
WIN32_FIND_DATA fd;
HANDLE hff;

  if (param1[0] == 0x00) {
    WCMD_output ("Argument missing\n");
    return;
  }

  if ((strchr(param1,'*') != NULL) || (strchr(param1,'%') != NULL)) {
    WCMD_output ("Wildcards not yet supported\n");
    return;
  }

  /* If no destination supplied, assume current directory */
  if (param2[0] == 0x00) {
      strcpy(param2, ".");
  }

  /* If 2nd parm is directory, then use original filename */
  GetFullPathName (param2, sizeof(outpath), outpath, NULL);
  hff = FindFirstFile (outpath, &fd);
  if (hff != INVALID_HANDLE_VALUE) {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      GetFullPathName (param1, sizeof(inpath), inpath, &infile);
      strcat (outpath, "\\");
      strcat (outpath, infile);
    }
    FindClose (hff);
  }

  status = MoveFile (param1, outpath);
  if (!status) WCMD_print_error ();
}

/****************************************************************************
 * WCMD_pause
 *
 * Wait for keyboard input.
 */

void WCMD_pause (void) {

DWORD count;
char string[32];

  WCMD_output (anykey);
  ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
}

/****************************************************************************
 * WCMD_remove_dir
 *
 * Delete a directory.
 */

void WCMD_remove_dir (void) {

  if (param1[0] == 0x00) {
    WCMD_output ("Argument missing\n");
    return;
  }
  if (!RemoveDirectory (param1)) WCMD_print_error ();
}

/****************************************************************************
 * WCMD_rename
 *
 * Rename a file.
 * FIXME: Needs input and output files to be fully specified.
 */

void WCMD_rename (void) {

int status;

  if (param1[0] == 0x00 || param2[0] == 0x00) {
    WCMD_output ("Argument missing\n");
    return;
  }
  if ((strchr(param1,'*') != NULL) || (strchr(param1,'%') != NULL)) {
    WCMD_output ("Wildcards not yet supported\n");
    return;
  }
  status = MoveFile (param1, param2);
  if (!status) WCMD_print_error ();
}

/*****************************************************************************
 * WCMD_dupenv
 *
 * Make a copy of the environment.
 */
static WCHAR *WCMD_dupenv( const WCHAR *env )
{
  WCHAR *env_copy;
  int len;

  if( !env )
    return NULL;

  len = 0;
  while ( env[len] )
    len += (lstrlenW(&env[len]) + 1);

  env_copy = LocalAlloc (LMEM_FIXED, (len+1) * sizeof (WCHAR) );
  if (!env_copy)
  {
    WCMD_output ("out of memory\n");
    return env_copy;
  }
  memcpy (env_copy, env, len*sizeof (WCHAR));
  env_copy[len] = 0;

  return env_copy;
}

/*****************************************************************************
 * WCMD_setlocal
 *
 *  setlocal pushes the environment onto a stack
 *  Save the environment as unicode so we don't screw anything up.
 */
void WCMD_setlocal (const char *s) {
  WCHAR *env;
  struct env_stack *env_copy;

  /* DISABLEEXTENSIONS ignored */

  env_copy = LocalAlloc (LMEM_FIXED, sizeof (struct env_stack));
  if( !env_copy )
  {
    WCMD_output ("out of memory\n");
    return;
  }

  env = GetEnvironmentStringsW ();

  env_copy->strings = WCMD_dupenv (env);
  if (env_copy->strings)
  {
    env_copy->next = saved_environment;
    saved_environment = env_copy;
  }
  else
    LocalFree (env_copy);

  FreeEnvironmentStringsW (env);
}

/*****************************************************************************
 * WCMD_strchrW
 */
static inline WCHAR *WCMD_strchrW(WCHAR *str, WCHAR ch)
{
   while(*str)
   {
     if(*str == ch)
       return str;
     str++;
   }
   return NULL;
}

/*****************************************************************************
 * WCMD_endlocal
 *
 *  endlocal pops the environment off a stack
 */
void WCMD_endlocal (void) {
  WCHAR *env, *old, *p;
  struct env_stack *temp;
  int len, n;

  if (!saved_environment)
    return;

  /* pop the old environment from the stack */
  temp = saved_environment;
  saved_environment = temp->next;

  /* delete the current environment, totally */
  env = GetEnvironmentStringsW ();
  old = WCMD_dupenv (GetEnvironmentStringsW ());
  len = 0;
  while (old[len]) {
    n = lstrlenW(&old[len]) + 1;
    p = WCMD_strchrW(&old[len], '=');
    if (p)
    {
      *p++ = 0;
      SetEnvironmentVariableW (&old[len], NULL);
    }
    len += n;
  }
  LocalFree (old);
  FreeEnvironmentStringsW (env);
  
  /* restore old environment */
  env = temp->strings;
  len = 0;
  while (env[len]) {
    n = lstrlenW(&env[len]) + 1;
    p = WCMD_strchrW(&env[len], '=');
    if (p)
    {
      *p++ = 0;
      SetEnvironmentVariableW (&env[len], p);
    }
    len += n;
  }
  LocalFree (env);
  LocalFree (temp);
}

/*****************************************************************************
 * WCMD_setshow_attrib
 *
 * Display and optionally sets DOS attributes on a file or directory
 *
 * FIXME: Wine currently uses the Unix stat() function to get file attributes.
 * As a result only the Readonly flag is correctly reported, the Archive bit
 * is always set and the rest are not implemented. We do the Right Thing anyway.
 *
 * FIXME: No SET functionality.
 *
 */

void WCMD_setshow_attrib (void) {

DWORD count;
HANDLE hff;
WIN32_FIND_DATA fd;
char flags[9] = {"        "};

  if (param1[0] == '-') {
    WCMD_output (nyi);
    return;
  }

  if (lstrlen(param1) == 0) {
    GetCurrentDirectory (sizeof(param1), param1);
    strcat (param1, "\\*");
  }

  hff = FindFirstFile (param1, &fd);
  if (hff == INVALID_HANDLE_VALUE) {
    WCMD_output ("%s: File Not Found\n",param1);
  }
  else {
    do {
      if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
	  flags[0] = 'H';
	}
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
	  flags[1] = 'S';
	}
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
	  flags[2] = 'A';
	}
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
	  flags[3] = 'R';
	}
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
	  flags[4] = 'T';
	}
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
	  flags[5] = 'C';
	}
        WCMD_output ("%s   %s\n", flags, fd.cFileName);
	for (count=0; count < 8; count++) flags[count] = ' ';
      }
    } while (FindNextFile(hff, &fd) != 0);
  }
  FindClose (hff);
}

/*****************************************************************************
 * WCMD_setshow_default
 *
 *	Set/Show the current default directory
 */

void WCMD_setshow_default (void) {

BOOL status;
char string[1024];

  if (strlen(param1) == 0) {
    GetCurrentDirectory (sizeof(string), string);
    strcat (string, "\n");
    WCMD_output (string);
  }
  else {
    status = SetCurrentDirectory (param1);
    if (!status) {
      WCMD_print_error ();
      return;
    }
   }
  return;
}

/****************************************************************************
 * WCMD_setshow_date
 *
 * Set/Show the system date
 * FIXME: Can't change date yet
 */

void WCMD_setshow_date (void) {

char curdate[64], buffer[64];
DWORD count;

  if (lstrlen(param1) == 0) {
    if (GetDateFormat (LOCALE_USER_DEFAULT, 0, NULL, NULL,
  		curdate, sizeof(curdate))) {
      WCMD_output ("Current Date is %s\nEnter new date: ", curdate);
      ReadFile (GetStdHandle(STD_INPUT_HANDLE), buffer, sizeof(buffer), &count, NULL);
      if (count > 2) {
        WCMD_output (nyi);
      }
    }
    else WCMD_print_error ();
  }
  else {
    WCMD_output (nyi);
  }
}

/****************************************************************************
 * WCMD_compare
 */
static int WCMD_compare( const void *a, const void *b )
{
    int r;
    const char * const *str_a = a, * const *str_b = b;
    r = CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
	  *str_a, -1, *str_b, -1 );
    if( r == CSTR_LESS_THAN ) return -1;
    if( r == CSTR_GREATER_THAN ) return 1;
    return 0;
}

/****************************************************************************
 * WCMD_setshow_sortenv
 *
 * sort variables into order for display
 */
static void WCMD_setshow_sortenv(const char *s)
{
  UINT count=0, len=0, i;
  const char **str;

  /* count the number of strings, and the total length */
  while ( s[len] ) {
    len += (lstrlen(&s[len]) + 1);
    count++;
  }

  /* add the strings to an array */
  str = LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, count * sizeof (char*) );
  if( !str )
    return;
  str[0] = s;
  for( i=1; i<count; i++ )
    str[i] = str[i-1] + lstrlen(str[i-1]) + 1;

  /* sort the array */
  qsort( str, count, sizeof (char*), WCMD_compare );

  /* print it */
  for( i=0; i<count; i++ ) {
      WCMD_output_asis(str[i]);
      WCMD_output_asis("\n");
  }

  LocalFree( str );
}

/****************************************************************************
 * WCMD_setshow_env
 *
 * Set/Show the environment variables
 */

void WCMD_setshow_env (char *s) {

LPVOID env;
char *p;
int status;
char buffer[1048];

  if (strlen(param1) == 0) {
    env = GetEnvironmentStrings ();
    WCMD_setshow_sortenv( env );
  }
  else {
    p = strchr (s, '=');
    if (p == NULL) {

      /* FIXME: Emulate Win98 for now, ie "SET C" looks ONLY for an
         environment variable C, whereas on NT it shows ALL variables
         starting with C.
       */
      status = GetEnvironmentVariable(s, buffer, sizeof(buffer));
      if (status) {
        WCMD_output_asis( s);
        WCMD_output_asis( "=");
        WCMD_output_asis( buffer);
        WCMD_output_asis( "\n");
      } else {
        WCMD_output ("Environment variable %s not defined\n", s);
      }
      return;
    }
    *p++ = '\0';

    if (strlen(p) == 0) p = NULL;
    status = SetEnvironmentVariable (s, p);
    if ((!status) & (GetLastError() != ERROR_ENVVAR_NOT_FOUND)) WCMD_print_error();
  }
  /* WCMD_output (newline);   @JED*/
}

/****************************************************************************
 * WCMD_setshow_path
 *
 * Set/Show the path environment variable
 */

void WCMD_setshow_path (char *command) {

char string[1024];
DWORD status;

  if (strlen(param1) == 0) {
    status = GetEnvironmentVariable ("PATH", string, sizeof(string));
    if (status != 0) {
      WCMD_output_asis ( "PATH=");
      WCMD_output_asis ( string);
      WCMD_output_asis ( "\n");
    }
    else {
      WCMD_output ("PATH not found\n");
    }
  }
  else {
    status = SetEnvironmentVariable ("PATH", command);
    if (!status) WCMD_print_error();
  }
}

/****************************************************************************
 * WCMD_setshow_prompt
 *
 * Set or show the command prompt.
 */

void WCMD_setshow_prompt (void) {

char *s;

  if (strlen(param1) == 0) {
    SetEnvironmentVariable ("PROMPT", NULL);
  }
  else {
    s = param1;
    while ((*s == '=') || (*s == ' ')) s++;
    if (strlen(s) == 0) {
      SetEnvironmentVariable ("PROMPT", NULL);
    }
    else SetEnvironmentVariable ("PROMPT", s);
  }
}

/****************************************************************************
 * WCMD_setshow_time
 *
 * Set/Show the system time
 * FIXME: Can't change time yet
 */

void WCMD_setshow_time (void) {

char curtime[64], buffer[64];
DWORD count;
SYSTEMTIME st;

  if (strlen(param1) == 0) {
    GetLocalTime(&st);
    if (GetTimeFormat (LOCALE_USER_DEFAULT, 0, &st, NULL,
  		curtime, sizeof(curtime))) {
      WCMD_output ("Current Time is %s\nEnter new time: ", curtime);
      ReadFile (GetStdHandle(STD_INPUT_HANDLE), buffer, sizeof(buffer), &count, NULL);
      if (count > 2) {
        WCMD_output (nyi);
      }
    }
    else WCMD_print_error ();
  }
  else {
    WCMD_output (nyi);
  }
}

/****************************************************************************
 * WCMD_shift
 *
 * Shift batch parameters.
 */

void WCMD_shift (void) {

  if (context != NULL) context -> shift_count++;

}

/****************************************************************************
 * WCMD_title
 *
 * Set the console title
 */
void WCMD_title (char *command) {
  SetConsoleTitle(command);
}

/****************************************************************************
 * WCMD_type
 *
 * Copy a file to standard output.
 */

void WCMD_type (void) {

HANDLE h;
char buffer[512];
DWORD count;

  if (param1[0] == 0x00) {
    WCMD_output ("Argument missing\n");
    return;
  }
  h = CreateFile (param1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
  		FILE_ATTRIBUTE_NORMAL, NULL);
  if (h == INVALID_HANDLE_VALUE) {
    WCMD_print_error ();
    return;
  }
  while (ReadFile (h, buffer, sizeof(buffer), &count, NULL)) {
    if (count == 0) break;	/* ReadFile reports success on EOF! */
    buffer[count] = 0;
    WCMD_output_asis (buffer);
  }
  CloseHandle (h);
}

/****************************************************************************
 * WCMD_verify
 *
 * Display verify flag.
 * FIXME: We don't actually do anything with the verify flag other than toggle
 * it...
 */

void WCMD_verify (char *command) {

static const char von[] = "Verify is ON\n", voff[] = "Verify is OFF\n";
int count;

  count = strlen(command);
  if (count == 0) {
    if (verify_mode) WCMD_output (von);
    else WCMD_output (voff);
    return;
  }
  if (lstrcmpi(command, "ON") == 0) {
    verify_mode = 1;
    return;
  }
  else if (lstrcmpi(command, "OFF") == 0) {
    verify_mode = 0;
    return;
  }
  else WCMD_output ("Verify must be ON or OFF\n");
}

/****************************************************************************
 * WCMD_version
 *
 * Display version info.
 */

void WCMD_version (void) {

  WCMD_output (version_string);

}

/****************************************************************************
 * WCMD_volume
 *
 * Display volume info and/or set volume label. Returns 0 if error.
 */

int WCMD_volume (int mode, char *path) {

DWORD count, serial;
char string[MAX_PATH], label[MAX_PATH], curdir[MAX_PATH];
BOOL status;

  if (lstrlen(path) == 0) {
    status = GetCurrentDirectory (sizeof(curdir), curdir);
    if (!status) {
      WCMD_print_error ();
      return 0;
    }
    status = GetVolumeInformation (NULL, label, sizeof(label), &serial, NULL,
    	NULL, NULL, 0);
  }
  else {
    if ((path[1] != ':') || (lstrlen(path) != 2)) {
      WCMD_output_asis("Syntax Error\n\n");
      return 0;
    }
    wsprintf (curdir, "%s\\", path);
    status = GetVolumeInformation (curdir, label, sizeof(label), &serial, NULL,
    	NULL, NULL, 0);
  }
  if (!status) {
    WCMD_print_error ();
    return 0;
  }
  WCMD_output ("Volume in drive %c is %s\nVolume Serial Number is %04x-%04x\n\n",
    	curdir[0], label, HIWORD(serial), LOWORD(serial));
  if (mode) {
    WCMD_output ("Volume label (11 characters, ENTER for none)?");
    ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
    if (count > 1) {
      string[count-1] = '\0';		/* ReadFile output is not null-terminated! */
      if (string[count-2] == '\r') string[count-2] = '\0'; /* Under Windoze we get CRLF! */
    }
    if (lstrlen(path) != 0) {
      if (!SetVolumeLabel (curdir, string)) WCMD_print_error ();
    }
    else {
      if (!SetVolumeLabel (NULL, string)) WCMD_print_error ();
    }
  }
  return 1;
}
