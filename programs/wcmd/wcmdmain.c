/*
 * WCMD - Wine-compatible command line interface.
 *
 * Copyright (C) 1999 - 2001 D A Pickles
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * FIXME:
 * - Cannot handle parameters in quotes
 * - Lots of functionality missing from builtins
 */

#include "wcmd.h"

char *inbuilt[] = {"ATTRIB", "CALL", "CD", "CHDIR", "CLS", "COPY", "CTTY",
		"DATE", "DEL", "DIR", "ECHO", "ERASE", "FOR", "GOTO",
		"HELP", "IF", "LABEL", "MD", "MKDIR", "MOVE", "PATH", "PAUSE",
		"PROMPT", "REM", "REN", "RENAME", "RD", "RMDIR", "SET", "SHIFT",
		"TIME", "TITLE", "TYPE", "VERIFY", "VER", "VOL", "EXIT"  };

HINSTANCE hinst;
DWORD errorlevel;
int echo_mode = 1, verify_mode = 0;
char nyi[] = "Not Yet Implemented\n\n";
char newline[] = "\n";
char version_string[] = "WCMD Version 0.17\n\n";
char anykey[] = "Press any key to continue: ";
char quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
BATCH_CONTEXT *context = NULL;

/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */

int main (int argc, char *argv[]) {

char string[1024], args[MAX_PATH], param[MAX_PATH];
int i;
DWORD count;
HANDLE h;

  args[0] = param[0] = '\0';
  if (argc > 1) {
    /* interpreter options must come before the command to execute. 
     * Any options after the command are command options, not interpreter options.
     */
    for (i=1; i<argc && argv[i][0] == '/'; i++)
        strcat (args, argv[i]);
    for (; i<argc; i++) {
        strcat (param, argv[i]);
        strcat (param, " ");
    }
  }

  /* If we do a "wcmd /c command", we don't want to allocate a new
   * console since the command returns immediately. Rather, we use
   * the surrently allocated input and output handles. This allows
   * us to pipe to and read from the command interpreter.
   */
  if (strstr(args, "/c") != NULL) {
    WCMD_process_command (param);
    return 0;
  }

  SetConsoleMode (GetStdHandle(STD_INPUT_HANDLE), ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
  	ENABLE_PROCESSED_INPUT);
  SetConsoleTitle("Wine Command Prompt");

/*
 *	Execute any command-line options.
 */

  if (strstr(args, "/q") != NULL) {
    WCMD_echo ("OFF");
  }

  if (strstr(args, "/k") != NULL) {
    WCMD_process_command (param);
  }

/*
 *	If there is an AUTOEXEC.BAT file, try to execute it.
 */

  GetFullPathName ("\\autoexec.bat", sizeof(string), string, NULL);
  h = CreateFile (string, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h != INVALID_HANDLE_VALUE) {
    CloseHandle (h);
#if 0
    WCMD_batch_command (string);
#endif
  }

/*
 *	Loop forever getting commands and executing them.
 */

  WCMD_version ();
  while (TRUE) {
    WCMD_show_prompt ();
    ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
    if (count > 1) {
      string[count-1] = '\0';		/* ReadFile output is not null-terminated! */
      if (string[count-2] == '\r') string[count-2] = '\0'; /* Under Windoze we get CRLF! */
      if (lstrlen (string) != 0) {
        if (strchr(string,'|') != NULL) {
          WCMD_pipe (string);
        }
	else {
          WCMD_process_command (string);
	}
      }
    }
  }
}


/*****************************************************************************
 * Process one command. If the command is EXIT this routine does not return.
 * We will recurse through here executing batch files.
 */


void WCMD_process_command (char *command) {

char cmd[1024];
char *p;
int status, i;
DWORD count;
HANDLE old_stdin = 0, old_stdout = 0, h;
char *whichcmd;


/*
 *	Expand up environment variables.
 */

    status = ExpandEnvironmentStrings (command, cmd, sizeof(cmd));
    if (!status) {
      WCMD_print_error ();
      return;
    }

/*
 *	Changing default drive has to be handled as a special case.
 */

    if ((cmd[1] == ':') && IsCharAlpha (cmd[0]) && (strlen(cmd) == 2)) {
      status = SetCurrentDirectory (cmd);
      if (!status) WCMD_print_error ();
      return;
    }

    /* Dont issue newline WCMD_output (newline);           @JED*/

/*
 *	Redirect stdin and/or stdout if required.
 */

    if ((p = strchr(cmd,'<')) != NULL) {
      h = CreateFile (WCMD_parameter (++p, 0, NULL), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE) {
	WCMD_print_error ();
	return;
      }
      old_stdin = GetStdHandle (STD_INPUT_HANDLE);
      SetStdHandle (STD_INPUT_HANDLE, h);
    }
    if ((p = strchr(cmd,'>')) != NULL) {
      h = CreateFile (WCMD_parameter (++p, 0, NULL), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE) {
	WCMD_print_error ();
	return;
      }
      old_stdout = GetStdHandle (STD_OUTPUT_HANDLE);
      SetStdHandle (STD_OUTPUT_HANDLE, h);
      *--p = '\0';
    }
    if ((p = strchr(cmd,'<')) != NULL) *p = '\0';

/*
 * Strip leading whitespaces, and a '@' if supplied
 */
    whichcmd = WCMD_strtrim_leading_spaces(cmd);
    if (whichcmd[0] == '@') whichcmd++;

/*
 *	Check if the command entered is internal. If it is, pass the rest of the
 *	line down to the command. If not try to run a program.
 */

    count = 0;
    while (IsCharAlphaNumeric(whichcmd[count])) {
      count++;
    }
    for (i=0; i<=WCMD_EXIT; i++) {
      if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
      	  whichcmd, count, inbuilt[i], -1) == 2) break;
    }
    p = WCMD_strtrim_leading_spaces (&whichcmd[count]);
    WCMD_parse (p, quals, param1, param2);
    switch (i) {

      case WCMD_ATTRIB:
        WCMD_setshow_attrib ();
        break;
      case WCMD_CALL:
        WCMD_batch (param1, p, 1);
        break;
      case WCMD_CD:
      case WCMD_CHDIR:
        WCMD_setshow_default ();
        break;
      case WCMD_CLS:
        WCMD_clear_screen ();
        break;
      case WCMD_COPY:
        WCMD_copy ();
        break;
      case WCMD_CTTY:
        WCMD_change_tty ();
        break;
      case WCMD_DATE:
        WCMD_setshow_date ();
	break;
      case WCMD_DEL:
      case WCMD_ERASE:
        WCMD_delete (0);
        break;
      case WCMD_DIR:
        WCMD_directory ();
        break;
      case WCMD_ECHO:
        /* Use the unstripped version of the following data - step over the space */
        /* but only if a parameter follows                                        */
        if (strlen(&whichcmd[count]) > 0)
          WCMD_echo(&whichcmd[count+1]);
        else
          WCMD_echo(&whichcmd[count]);
        break;
      case WCMD_FOR:
        WCMD_for (p);
        break;
      case WCMD_GOTO:
        WCMD_goto ();
        break;
      case WCMD_HELP:
        WCMD_give_help (p);
	break;
      case WCMD_IF:
	WCMD_if (p);
        break;
      case WCMD_LABEL:
        WCMD_volume (1, p);
        break;
      case WCMD_MD:
      case WCMD_MKDIR:
        WCMD_create_dir ();
	break;
      case WCMD_MOVE:
        WCMD_move ();
        break;
      case WCMD_PATH:
        WCMD_setshow_path (p);
        break;
      case WCMD_PAUSE:
        WCMD_pause ();
        break;
      case WCMD_PROMPT:
        WCMD_setshow_prompt ();
        break;
      case WCMD_REM:
        break;
      case WCMD_REN:
      case WCMD_RENAME:
        WCMD_rename ();
	break;
      case WCMD_RD:
      case WCMD_RMDIR:
        WCMD_remove_dir ();
        break;
      case WCMD_SET:
        WCMD_setshow_env (p);
	break;
      case WCMD_SHIFT:
        WCMD_shift ();
        break;
      case WCMD_TIME:
        WCMD_setshow_time ();
        break;
      case WCMD_TITLE:
        if (strlen(&whichcmd[count]) > 0)
          WCMD_title(&whichcmd[count+1]);
        break;
      case WCMD_TYPE:
        WCMD_type ();
	break;
      case WCMD_VER:
        WCMD_version ();
        break;
      case WCMD_VERIFY:
        WCMD_verify (p);
        break;
      case WCMD_VOL:
        WCMD_volume (0, p);
        break;
      case WCMD_EXIT:
        ExitProcess (0);
      default:
        WCMD_run_program (whichcmd);
    };
    if (old_stdin) {
      CloseHandle (GetStdHandle (STD_INPUT_HANDLE));
      SetStdHandle (STD_INPUT_HANDLE, old_stdin);
    }
    if (old_stdout) {
      CloseHandle (GetStdHandle (STD_OUTPUT_HANDLE));
      SetStdHandle (STD_OUTPUT_HANDLE, old_stdout);
    }
  }

/******************************************************************************
 * WCMD_run_program
 *
 *	Execute a command line as an external program. If no extension given then
 *	precedence is given to .BAT files. Must allow recursion.
 *
 *	FIXME: Case sensitivity in suffixes!
 */

void WCMD_run_program (char *command) {

STARTUPINFO st;
PROCESS_INFORMATION pe;
SHFILEINFO psfi;
DWORD console;
BOOL status;
HANDLE h;
HINSTANCE hinst;
char filetorun[MAX_PATH];

  WCMD_parse (command, quals, param1, param2);	/* Quick way to get the filename */
  if (!(*param1) && !(*param2))
    return;
  if (strpbrk (param1, "\\:") == NULL) {	/* No explicit path given */
    if ((strchr (param1, '.') == NULL) || (strstr (param1, ".bat") != NULL)) {
      if (SearchPath (NULL, param1, ".bat", sizeof(filetorun), filetorun, NULL)) {
        WCMD_batch (filetorun, command, 0);
        return;
      }
    }
  }
  else {                                        /* Explicit path given */
    if (strstr (param1, ".bat") != NULL) {
      WCMD_batch (param1, command, 0);
      return;
    }
    if (strchr (param1, '.') == NULL) {
      strcpy (filetorun, param1);
      strcat (filetorun, ".bat");
      h = CreateFile (filetorun, GENERIC_READ, FILE_SHARE_READ,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (h != INVALID_HANDLE_VALUE) {
        CloseHandle (h);
        WCMD_batch (param1, command, 0);
        return;
      }
    }
  }

	/* No batch file found, assume executable */

  hinst = FindExecutable (param1, NULL, filetorun);
  if ((int)hinst < 32) {
    WCMD_print_error ();
    return;
  }
  console = SHGetFileInfo (filetorun, 0, &psfi, sizeof(psfi), SHGFI_EXETYPE);
  ZeroMemory (&st, sizeof(STARTUPINFO));
  st.cb = sizeof(STARTUPINFO);
  status = CreateProcess (NULL, command, NULL, NULL, FALSE,
  		 0, NULL, NULL, &st, &pe);
  if (!status) {
    WCMD_print_error ();
    return;
  }
  if (!console) errorlevel = 0;
  else
  {
      if (!HIWORD(console)) WaitForSingleObject (pe.hProcess, INFINITE);
      GetExitCodeProcess (pe.hProcess, &errorlevel);
      if (errorlevel == STILL_ACTIVE) errorlevel = 0;
  }
}

/******************************************************************************
 * WCMD_show_prompt
 *
 *	Display the prompt on STDout
 *
 */

void WCMD_show_prompt (void) {

int status;
char out_string[MAX_PATH], curdir[MAX_PATH], prompt_string[MAX_PATH];
char *p, *q;

  status = GetEnvironmentVariable ("PROMPT", prompt_string, sizeof(prompt_string));
  if ((status == 0) || (status > sizeof(prompt_string))) {
    lstrcpy (prompt_string, "$P$G");
  }
  p = prompt_string;
  q = out_string;
  *q = '\0';
  while (*p != '\0') {
    if (*p != '$') {
      *q++ = *p++;
      *q = '\0';
    }
    else {
      p++;
      switch (toupper(*p)) {
        case '$':
	  *q++ = '$';
	  break;
	case 'B':
	  *q++ = '|';
	  break;
	case 'D':
	  GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, q, MAX_PATH);
	  while (*q) q++;
	  break;
	case 'E':
	  *q++ = '\E';
	  break;
	case 'G':
	  *q++ = '>';
	  break;
	case 'L':
	  *q++ = '<';
	  break;
	case 'N':
          status = GetCurrentDirectory (sizeof(curdir), curdir);
	  if (status) {
	    *q++ = curdir[0];
	  }
	  break;
	case 'P':
          status = GetCurrentDirectory (sizeof(curdir), curdir);
	  if (status) {
	    lstrcat (q, curdir);
	    while (*q) q++;
	  }
	  break;
	case 'Q':
	  *q++ = '=';
	  break;
	case 'T':
	  GetTimeFormat (LOCALE_USER_DEFAULT, 0, NULL, NULL, q, MAX_PATH);
	  while (*q) q++;
	  break;
       case 'V':
           lstrcat (q, version_string);
           while (*q) q++;
         break;
	case '_':
	  *q++ = '\n';
	  break;
      }
      p++;
      *q = '\0';
    }
  }
  WCMD_output (out_string);
}

/****************************************************************************
 * WCMD_print_error
 *
 * Print the message for GetLastError
 */

void WCMD_print_error (void) {
LPVOID lpMsgBuf;
DWORD error_code;
int status;

  error_code = GetLastError ();
  status = FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    			NULL, error_code, 0, (LPTSTR) &lpMsgBuf, 0, NULL);
  if (!status) {
    WCMD_output ("FIXME: Cannot display message for error %d, status %d\n",
			error_code, GetLastError());
    return;
  }
  WCMD_output (lpMsgBuf);
  LocalFree ((HLOCAL)lpMsgBuf);
  WCMD_output (newline);
  return;
}

/*******************************************************************
 * WCMD_parse - parse a command into parameters and qualifiers.
 *
 *	On exit, all qualifiers are concatenated into q, the first string
 *	not beginning with "/" is in p1 and the
 *	second in p2. Any subsequent non-qualifier strings are lost.
 *	Parameters in quotes are handled.
 */

void WCMD_parse (char *s, char *q, char *p1, char *p2) {

int p = 0;

  *q = *p1 = *p2 = '\0';
  while (TRUE) {
    switch (*s) {
      case '/':
        *q++ = *s++;
	while ((*s != '\0') && (*s != ' ') && *s != '/') {
	  *q++ = toupper (*s++);
	}
        *q = '\0';
	break;
      case ' ':
	s++;
	break;
      case '"':
	s++;
	while ((*s != '\0') && (*s != '"')) {
	  if (p == 0) *p1++ = *s++;
	  else if (p == 1) *p2++ = *s++;
	  else s++;
	}
        if (p == 0) *p1 = '\0';
        if (p == 1) *p2 = '\0';
        p++;
	if (*s == '"') s++;
	break;
      case '\0':
        return;
      default:
	while ((*s != '\0') && (*s != ' ')) {
	  if (p == 0) *p1++ = *s++;
	  else if (p == 1) *p2++ = *s++;
	  else s++;
	}
        if (p == 0) *p1 = '\0';
        if (p == 1) *p2 = '\0';
	p++;
    }
  }
}

/*******************************************************************
 * WCMD_output - send output to current standard output device.
 *
 */

void WCMD_output (char *format, ...) {

va_list ap;
char string[1024];

  va_start(ap,format);
  vsprintf (string, format, ap);
  va_end(ap);
  WCMD_output_asis(string);
}


static int line_count;
static int max_height;
static BOOL paged_mode;

void WCMD_enter_paged_mode(void)
{
CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo))
    max_height = consoleInfo.dwSize.Y;
  else
    max_height = 25;
  paged_mode = TRUE;
  line_count = 5; /* keep 5 lines from previous output */
}

void WCMD_leave_paged_mode(void)
{
  paged_mode = FALSE;
}

/*******************************************************************
 * WCMD_output_asis - send output to current standard output device.
 *        without formatting eg. when message contains '%'
 */

void WCMD_output_asis (char *message) {
  DWORD count;
  char* ptr;
  char string[1024];

  if (paged_mode) {
    do {
      if ((ptr = strchr(message, '\n')) != NULL) ptr++;
      WriteFile (GetStdHandle(STD_OUTPUT_HANDLE), message, 
                 (ptr) ? ptr - message : lstrlen(message), &count, NULL);
      if (ptr) {
        if (++line_count >= max_height - 1) {
          line_count = 0;
          WCMD_output_asis (anykey);
          ReadFile (GetStdHandle(STD_INPUT_HANDLE), string, sizeof(string), &count, NULL);
        }
      }
    } while ((message = ptr) != NULL);
  } else {
      WriteFile (GetStdHandle(STD_OUTPUT_HANDLE), message, lstrlen(message), &count, NULL);
  }
}


/***************************************************************************
 * WCMD_strtrim_leading_spaces
 *
 *	Remove leading spaces from a string. Return a pointer to the first
 *	non-space character. Does not modify the input string
 */

char *WCMD_strtrim_leading_spaces (char *string) {

char *ptr;

  ptr = string;
  while (*ptr == ' ') ptr++;
  return ptr;
}

/*************************************************************************
 * WCMD_strtrim_trailing_spaces
 *
 *	Remove trailing spaces from a string. This routine modifies the input
 *	string by placing a null after the last non-space character
 */

void WCMD_strtrim_trailing_spaces (char *string) {

char *ptr;

  ptr = string + lstrlen (string) - 1;
  while ((*ptr == ' ') && (ptr >= string)) {
    *ptr = '\0';
    ptr--;
  }
}

/*************************************************************************
 * WCMD_pipe
 *
 *	Handle pipes within a command - the DOS way using temporary files.
 */

void WCMD_pipe (char *command) {

char *p;
char temp_path[MAX_PATH], temp_file[MAX_PATH], temp_file2[MAX_PATH], temp_cmd[1024];

  GetTempPath (sizeof(temp_path), temp_path);
  GetTempFileName (temp_path, "WCMD", 0, temp_file);
  p = strchr(command, '|');
  *p++ = '\0';
  wsprintf (temp_cmd, "%s > %s", command, temp_file);
  WCMD_process_command (temp_cmd);
  command = p;
  while ((p = strchr(command, '|'))) {
    *p++ = '\0';
    GetTempFileName (temp_path, "WCMD", 0, temp_file2);
    wsprintf (temp_cmd, "%s < %s > %s", command, temp_file, temp_file2);
    WCMD_process_command (temp_cmd);
    DeleteFile (temp_file);
    lstrcpy (temp_file, temp_file2);
    command = p;
  }
  wsprintf (temp_cmd, "%s < %s", command, temp_file);
  WCMD_process_command (temp_cmd);
  DeleteFile (temp_file);
}
