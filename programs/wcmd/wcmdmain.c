/*
 * WCMD - Wine-compatible command line interface. 
 *
 * (C) 1999 D A Pickles
 */

/*
 * FIXME:
 * - No support for pipes
 * - 32-bit limit on file sizes in DIR command
 * - Cannot handle parameters in quotes
 * - Lots of functionality missing from builtins
 */

#include "wcmd.h"

char *inbuilt[] = {"ATTRIB", "CALL", "CD", "CHDIR", "CLS", "COPY", "CTTY",
		"DATE", "DEL", "DIR", "ECHO", "ERASE", "FOR", "GOTO",
		"HELP", "IF", "LABEL", "MD", "MKDIR", "MOVE", "PATH", "PAUSE",
		"PROMPT", "REM", "REN", "RENAME", "RD", "RMDIR", "SET", "SHIFT",
		"TIME", "TYPE", "VERIFY", "VER", "VOL", "EXIT"};

int echo_mode = 1, verify_mode = 0;
char nyi[] = "Not Yet Implemented\n\n";
char newline[] = "\n";
char version_string[] = "WCMD Version 0.12\n\n";
char anykey[] = "Press any key to continue: ";
char quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
BATCH_CONTEXT *context = NULL;

/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */


int wine_main (int argc, char *argv[]) {

char string[1024], args[MAX_PATH], param[MAX_PATH];
int status, i;
DWORD count;
HANDLE h;

  args[0] = param[0] = '\0';
  if (argc > 1) {
    for (i=1; i<argc; i++) {
      if (argv[i][0] == '/') {
        strcat (args, argv[i]);
      }
      else {
        strcat (param, argv[i]);
        strcat (param, " ");
      }
    }
  }

/*
 *	Allocate a console and set it up.
 */

  status = FreeConsole ();
  if (!status) WCMD_print_error();
  status = AllocConsole();
  if (!status) WCMD_print_error();
  SetConsoleMode (GetStdHandle(STD_INPUT_HANDLE), ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
  	ENABLE_PROCESSED_INPUT);

/*
 *	Execute any command-line options.
 */

  if (strstr(args, "/q") != NULL) {
    WCMD_echo ("OFF");
  }

  if (strstr(args, "/c") != NULL) {
    WCMD_process_command (param);
    return 0;
  }

  if (strstr(args, "/k") != NULL) {
    WCMD_process_command (param);
  }

/*
 *	If there is an AUTOEXEC.BAT file, try to execute it.
 */

  GetFullPathName ("\\autoexec.bat", sizeof(string), string, NULL);
  h = CreateFile (string, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h != INVALID_HANDLE_VALUE) {
    CloseHandle (h);
#if 0
    WCMD_batch (string, " ");
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
        WCMD_process_command (string);
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
 *	Throw away constructs we don't support yet
 */

    if (strchr(command,'|') != NULL) {
      WCMD_output ("Pipes not yet implemented\n");
      return;
    }

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
      h = CreateFile (WCMD_parameter (++p, 0, NULL), GENERIC_READ, 0, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);
      if (h == INVALID_HANDLE_VALUE) {
	WCMD_print_error ();
	return;
      }
      old_stdin = GetStdHandle (STD_INPUT_HANDLE);
      SetStdHandle (STD_INPUT_HANDLE, h);
    }
    if ((p = strchr(cmd,'>')) != NULL) {
      h = CreateFile (WCMD_parameter (++p, 0, NULL), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, 0);
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
        WCMD_setshow_path ();
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
BOOL status;
HANDLE h;
char filetorun[MAX_PATH];

  WCMD_parse (command, quals, param1, param2);	/* Quick way to get the filename */
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
      h = CreateFile (filetorun, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (h != INVALID_HANDLE_VALUE) {
        CloseHandle (h);
        WCMD_batch (param1, command, 0);
        return;
      }
    }
  }

	/* No batch file found, assume executable */

  ZeroMemory (&st, sizeof(STARTUPINFO));
  st.cb = sizeof(STARTUPINFO);
  status = CreateProcess (NULL, command, NULL, NULL, FALSE,
  		 0, NULL, NULL, &st, &pe);
  if (!status) {
    WCMD_print_error ();
  }
}

/******************************************************************************
 * WCMD_show_prompt
 *
 *	Display the prompt on STDout
 *
 */

void WCMD_show_prompt () {

int status;
char out_string[MAX_PATH], curdir[MAX_PATH], prompt_string[MAX_PATH];
char *p, *q;

  status = GetEnvironmentVariable ("PROMPT", prompt_string, sizeof(prompt_string));
  if ((status == 0) || (status > sizeof(prompt_string))) {
    lstrcpy (prompt_string, "$N$G");
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
 * Print the message for GetLastError - not much use yet as Wine doesn't have
 * the messages available, so we show meaningful messages for the most likely.
 */

void WCMD_print_error () {
LPVOID lpMsgBuf;
DWORD error_code;

  error_code = GetLastError ();
  switch (error_code) {
    case ERROR_FILE_NOT_FOUND:
      WCMD_output ("File Not Found\n");
      break;
    case ERROR_PATH_NOT_FOUND:
      WCMD_output ("Path Not Found\n");
      break;
    default:
      FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
    			NULL, error_code,
			MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf, 0, NULL);
      WCMD_output (lpMsgBuf);
      LocalFree ((HLOCAL)lpMsgBuf);
  }
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
	while ((*s != '\0') && (*s != ' ') && (*s != '/')) {
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
DWORD count;

  va_start(ap,format);
  vsprintf (string, format, ap);
  WriteFile (GetStdHandle(STD_OUTPUT_HANDLE), string, lstrlen(string), &count, NULL);
  va_end(ap);
}

/******************************************************************* 
 * WCMD_output_asis - send output to current standard output device.
 *        without formatting eg. when message contains '%'
 */

void WCMD_output_asis (char *message) {
  DWORD count;
  WriteFile (GetStdHandle(STD_OUTPUT_HANDLE), message, lstrlen(message), &count, NULL);
}



/*	Remove leading spaces from a string. Return a pointer to the first
 *	non-space character. Does not modify the input string
 */

char *WCMD_strtrim_leading_spaces (char *string) {

char *ptr;

  ptr = string;
  while (*ptr == ' ') ptr++;
  return ptr;
}

/*	Remove trailing spaces from a string. This routine modifies the input
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
