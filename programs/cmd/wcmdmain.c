/*
 * CMD - Wine-compatible command line interface.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * FIXME:
 * - Cannot handle parameters in quotes
 * - Lots of functionality missing from builtins
 */

#include "config.h"
#include "wcmd.h"

const char * const inbuilt[] = {"ATTRIB", "CALL", "CD", "CHDIR", "CLS", "COPY", "CTTY",
		"DATE", "DEL", "DIR", "ECHO", "ERASE", "FOR", "GOTO",
		"HELP", "IF", "LABEL", "MD", "MKDIR", "MOVE", "PATH", "PAUSE",
		"PROMPT", "REM", "REN", "RENAME", "RD", "RMDIR", "SET", "SHIFT",
                "TIME", "TITLE", "TYPE", "VERIFY", "VER", "VOL", 
                "ENDLOCAL", "SETLOCAL", "EXIT" };

HINSTANCE hinst;
DWORD errorlevel;
int echo_mode = 1, verify_mode = 0;
static int opt_c, opt_k, opt_s;
const char nyi[] = "Not Yet Implemented\n\n";
const char newline[] = "\n";
const char version_string[] = "CMD Version " PACKAGE_VERSION "\n\n";
const char anykey[] = "Press Return key to continue: ";
char quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
BATCH_CONTEXT *context = NULL;
static HANDLE old_stdin = INVALID_HANDLE_VALUE, old_stdout = INVALID_HANDLE_VALUE;

/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */

int main (int argc, char *argv[])
{
  char string[1024];
  char* cmd=NULL;
  DWORD count;
  HANDLE h;
  int opt_q;

  opt_c=opt_k=opt_q=opt_s=0;
  while (*argv!=NULL)
  {
      char c;
      if ((*argv)[0]!='/' || (*argv)[1]=='\0') {
          argv++;
          continue;
      }

      c=(*argv)[1];
      if (tolower(c)=='c') {
          opt_c=1;
      } else if (tolower(c)=='q') {
          opt_q=1;
      } else if (tolower(c)=='k') {
          opt_k=1;
      } else if (tolower(c)=='s') {
          opt_s=1;
      } else if (tolower(c)=='t' || tolower(c)=='x' || tolower(c)=='y') {
          /* Ignored for compatibility with Windows */
      }

      if ((*argv)[2]==0)
          argv++;
      else /* handle `cmd /cnotepad.exe` and `cmd /x/c ...` */
          *argv+=2;

      if (opt_c || opt_k) /* break out of parsing immediately after c or k */
          break;
  }

  if (opt_q) {
    WCMD_echo("OFF");
  }

  if (opt_c || opt_k) {
      int len,qcount;
      char** arg;
      char* p;

      /* opt_s left unflagged if the command starts with and contains exactly
       * one quoted string (exactly two quote characters). The quoted string
       * must be an executable name that has whitespace and must not have the
       * following characters: &<>()@^| */

      /* Build the command to execute */
      len = 0;
      qcount = 0;
      for (arg = argv; *arg; arg++)
      {
          int has_space,bcount;
          char* a;

          has_space=0;
          bcount=0;
          a=*arg;
          if( !*a ) has_space=1;
          while (*a!='\0') {
              if (*a=='\\') {
                  bcount++;
              } else {
                  if (*a==' ' || *a=='\t') {
                      has_space=1;
                  } else if (*a=='"') {
                      /* doubling of '\' preceding a '"',
                       * plus escaping of said '"'
                       */
                      len+=2*bcount+1;
                      qcount++;
                  }
                  bcount=0;
              }
              a++;
          }
          len+=(a-*arg)+1 /* for the separating space */;
          if (has_space)
          {
              len+=2; /* for the quotes */
              qcount+=2;
          }
      }

      if (qcount!=2)
          opt_s=1;

      /* check argv[0] for a space and invalid characters */
      if (!opt_s) {
          opt_s=1;
          p=*argv;
          while (*p!='\0') {
              if (*p=='&' || *p=='<' || *p=='>' || *p=='(' || *p==')'
                  || *p=='@' || *p=='^' || *p=='|') {
                  opt_s=1;
                  break;
              }
              if (*p==' ')
                  opt_s=0;
              p++;
          }
      }

      cmd = HeapAlloc(GetProcessHeap(), 0, len);
      if (!cmd)
          exit(1);

      p = cmd;
      for (arg = argv; *arg; arg++)
      {
          int has_space,has_quote;
          char* a;

          /* Check for quotes and spaces in this argument */
          has_space=has_quote=0;
          a=*arg;
          if( !*a ) has_space=1;
          while (*a!='\0') {
              if (*a==' ' || *a=='\t') {
                  has_space=1;
                  if (has_quote)
                      break;
              } else if (*a=='"') {
                  has_quote=1;
                  if (has_space)
                      break;
              }
              a++;
          }

          /* Now transfer it to the command line */
          if (has_space)
              *p++='"';
          if (has_quote) {
              int bcount;
              char* a;

              bcount=0;
              a=*arg;
              while (*a!='\0') {
                  if (*a=='\\') {
                      *p++=*a;
                      bcount++;
                  } else {
                      if (*a=='"') {
                          int i;

                          /* Double all the '\\' preceding this '"', plus one */
                          for (i=0;i<=bcount;i++)
                              *p++='\\';
                          *p++='"';
                      } else {
                          *p++=*a;
                      }
                      bcount=0;
                  }
                  a++;
              }
          } else {
              strcpy(p,*arg);
              p+=strlen(*arg);
          }
          if (has_space)
              *p++='"';
          *p++=' ';
      }
      if (p > cmd)
          p--;  /* remove last space */
      *p = '\0';

      /* strip first and last quote characters if opt_s; check for invalid
       * executable is done later */
      if (opt_s && *cmd=='\"')
          WCMD_opt_s_strip_quotes(cmd);
  }

  if (opt_c) {
      /* If we do a "wcmd /c command", we don't want to allocate a new
       * console since the command returns immediately. Rather, we use
       * the currently allocated input and output handles. This allows
       * us to pipe to and read from the command interpreter.
       */
      if (strchr(cmd,'|') != NULL)
          WCMD_pipe(cmd);
      else
          WCMD_process_command(cmd);
      HeapFree(GetProcessHeap(), 0, cmd);
      return errorlevel;
  }

  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_LINE_INPUT |
                 ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
  SetConsoleTitle("Wine Command Prompt");

  if (opt_k) {
      WCMD_process_command(cmd);
      HeapFree(GetProcessHeap(), 0, cmd);
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
      string[count-1] = '\0'; /* ReadFile output is not null-terminated! */
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


void WCMD_process_command (char *command)
{
    char *cmd, *p;
    int status, i, len;
    DWORD count, creationDisposition;
    HANDLE h;
    char *whichcmd;
    SECURITY_ATTRIBUTES sa;

/*
 *	Expand up environment variables.
 */
    len = ExpandEnvironmentStrings (command, NULL, 0);
    cmd = HeapAlloc( GetProcessHeap(), 0, len );
    status = ExpandEnvironmentStrings (command, cmd, len);
    if (!status) {
      WCMD_print_error ();
      HeapFree( GetProcessHeap(), 0, cmd );
      return;
    }

/*
 *	Changing default drive has to be handled as a special case.
 */

    if ((cmd[1] == ':') && IsCharAlpha (cmd[0]) && (strlen(cmd) == 2)) {
      status = SetCurrentDirectory (cmd);
      if (!status) WCMD_print_error ();
      HeapFree( GetProcessHeap(), 0, cmd );
      return;
    }

    /* Don't issue newline WCMD_output (newline);           @JED*/

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
/*
 *	Redirect stdin and/or stdout if required.
 */

    if ((p = strchr(cmd,'<')) != NULL) {
      h = CreateFile (WCMD_parameter (++p, 0, NULL), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE) {
	WCMD_print_error ();
        HeapFree( GetProcessHeap(), 0, cmd );
	return;
      }
      old_stdin = GetStdHandle (STD_INPUT_HANDLE);
      SetStdHandle (STD_INPUT_HANDLE, h);
    }
    if ((p = strchr(cmd,'>')) != NULL) {
      *p++ = '\0';
      if ('>' == *p) {
        creationDisposition = OPEN_ALWAYS;
        p++;
      }
      else {
        creationDisposition = CREATE_ALWAYS;
      }
      h = CreateFile (WCMD_parameter (p, 0, NULL), GENERIC_WRITE, 0, &sa, creationDisposition,
		FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE) {
	WCMD_print_error ();
        HeapFree( GetProcessHeap(), 0, cmd );
	return;
      }
      if (SetFilePointer (h, 0, NULL, FILE_END) ==
          INVALID_SET_FILE_POINTER) {
        WCMD_print_error ();
      }
      old_stdout = GetStdHandle (STD_OUTPUT_HANDLE);
      SetStdHandle (STD_OUTPUT_HANDLE, h);
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
        WCMD_run_program (p, 1);
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
      case WCMD_SETLOCAL:
        WCMD_setlocal(p);
        break;
      case WCMD_ENDLOCAL:
        WCMD_endlocal();
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
        WCMD_exit ();
        break;
      default:
        WCMD_run_program (whichcmd, 0);
    }
    HeapFree( GetProcessHeap(), 0, cmd );
    if (old_stdin != INVALID_HANDLE_VALUE) {
      CloseHandle (GetStdHandle (STD_INPUT_HANDLE));
      SetStdHandle (STD_INPUT_HANDLE, old_stdin);
      old_stdin = INVALID_HANDLE_VALUE;
    }
    if (old_stdout != INVALID_HANDLE_VALUE) {
      CloseHandle (GetStdHandle (STD_OUTPUT_HANDLE));
      SetStdHandle (STD_OUTPUT_HANDLE, old_stdout);
      old_stdout = INVALID_HANDLE_VALUE;
    }
}

static void init_msvcrt_io_block(STARTUPINFO* st)
{
    STARTUPINFO st_p;
    /* fetch the parent MSVCRT info block if any, so that the child can use the
     * same handles as its grand-father
     */
    st_p.cb = sizeof(STARTUPINFO);
    GetStartupInfo(&st_p);
    st->cbReserved2 = st_p.cbReserved2;
    st->lpReserved2 = st_p.lpReserved2;
    if (st_p.cbReserved2 && st_p.lpReserved2 &&
        (old_stdin != INVALID_HANDLE_VALUE || old_stdout != INVALID_HANDLE_VALUE))
    {
        /* Override the entries for fd 0,1,2 if we happened
         * to change those std handles (this depends on the way wcmd sets
         * it's new input & output handles)
         */
        size_t sz = max(sizeof(unsigned) + (sizeof(char) + sizeof(HANDLE)) * 3, st_p.cbReserved2);
        BYTE* ptr = HeapAlloc(GetProcessHeap(), 0, sz);
        if (ptr)
        {
            unsigned num = *(unsigned*)st_p.lpReserved2;
            char* flags = (char*)(ptr + sizeof(unsigned));
            HANDLE* handles = (HANDLE*)(flags + num * sizeof(char));

            memcpy(ptr, st_p.lpReserved2, st_p.cbReserved2);
            st->cbReserved2 = sz;
            st->lpReserved2 = ptr;

#define WX_OPEN 0x01    /* see dlls/msvcrt/file.c */
            if (num <= 0 || (flags[0] & WX_OPEN))
            {
                handles[0] = GetStdHandle(STD_INPUT_HANDLE);
                flags[0] |= WX_OPEN;
            }
            if (num <= 1 || (flags[1] & WX_OPEN))
            {
                handles[1] = GetStdHandle(STD_OUTPUT_HANDLE);
                flags[1] |= WX_OPEN;
            }
            if (num <= 2 || (flags[2] & WX_OPEN))
            {
                handles[2] = GetStdHandle(STD_ERROR_HANDLE);
                flags[2] |= WX_OPEN;
            }
#undef WX_OPEN
        }
    }
}

/******************************************************************************
 * WCMD_run_program
 *
 *	Execute a command line as an external program. If no extension given then
 *	precedence is given to .BAT files. Must allow recursion.
 *	
 *	called is 1 if the program was invoked with a CALL command - removed
 *	from command -. It is only used for batch programs.
 *
 *	FIXME: Case sensitivity in suffixes!
 */

void WCMD_run_program (char *command, int called) {

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
  if (strpbrk (param1, "/\\:") == NULL) {  /* No explicit path given */
    char *ext = strrchr( param1, '.' );
    if (!ext || !strcasecmp( ext, ".bat"))
    {
      if (SearchPath (NULL, param1, ".bat", sizeof(filetorun), filetorun, NULL)) {
        WCMD_batch (filetorun, command, called);
        return;
      }
    }
    if (!ext || !strcasecmp( ext, ".cmd"))
    {
      if (SearchPath (NULL, param1, ".cmd", sizeof(filetorun), filetorun, NULL)) {
        WCMD_batch (filetorun, command, called);
        return;
      }
    }
  }
  else {                                        /* Explicit path given */
    char *ext = strrchr( param1, '.' );
    if (ext && (!strcasecmp( ext, ".bat" ) || !strcasecmp( ext, ".cmd" )))
    {
      WCMD_batch (param1, command, called);
      return;
    }

    if (ext && strpbrk( ext, "/\\:" )) ext = NULL;
    if (!ext)
    {
      strcpy (filetorun, param1);
      strcat (filetorun, ".bat");
      h = CreateFile (filetorun, GENERIC_READ, FILE_SHARE_READ,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (h != INVALID_HANDLE_VALUE) {
        CloseHandle (h);
        WCMD_batch (param1, command, called);
        return;
      }
    }
  }

	/* No batch file found, assume executable */

  hinst = FindExecutable (param1, NULL, filetorun);
  if ((INT_PTR)hinst < 32)
    console = 0;
  else
    console = SHGetFileInfo (filetorun, 0, &psfi, sizeof(psfi), SHGFI_EXETYPE);

  ZeroMemory (&st, sizeof(STARTUPINFO));
  st.cb = sizeof(STARTUPINFO);
  init_msvcrt_io_block(&st);

  status = CreateProcess (NULL, command, NULL, NULL, TRUE, 
                          0, NULL, NULL, &st, &pe);
  if ((opt_c || opt_k) && !opt_s && !status
      && GetLastError()==ERROR_FILE_NOT_FOUND && command[0]=='\"') {
    /* strip first and last quote characters and try again */
    WCMD_opt_s_strip_quotes(command);
    opt_s=1;
    WCMD_run_program(command, called);
    return;
  }
  if (!status) {
    WCMD_print_error ();
    /* If a command fails to launch, it sets errorlevel 9009 - which
       does not seem to have any associated constant definition     */
    errorlevel = 9009;
    return;
  }
  if (!console) errorlevel = 0;
  else
  {
      if (!HIWORD(console)) WaitForSingleObject (pe.hProcess, INFINITE);
      GetExitCodeProcess (pe.hProcess, &errorlevel);
      if (errorlevel == STILL_ACTIVE) errorlevel = 0;
  }
  CloseHandle(pe.hProcess);
  CloseHandle(pe.hThread);
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
  WCMD_output_asis (out_string);
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
  WCMD_output_asis (lpMsgBuf);
  LocalFree ((HLOCAL)lpMsgBuf);
  WCMD_output_asis (newline);
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
      case '\t':
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
	while ((*s != '\0') && (*s != ' ') && (*s != '\t')) {
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

void WCMD_output (const char *format, ...) {

va_list ap;
char string[1024];
int ret;

  va_start(ap,format);
  ret = vsnprintf (string, sizeof( string), format, ap);
  va_end(ap);
  if( ret >= sizeof( string)) {
       WCMD_output_asis("ERR: output truncated in WCMD_output\n" );
       string[sizeof( string) -1] = '\0';
  }
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

void WCMD_output_asis (const char *message) {
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
 * WCMD_opt_s_strip_quotes
 *
 *	Remove first and last quote characters, preserving all other text
 */

void WCMD_opt_s_strip_quotes(char *cmd) {
  char *src = cmd + 1, *dest = cmd, *lastq = NULL;
  while((*dest=*src) != '\0') {
      if (*src=='\"')
          lastq=dest;
      dest++, src++;
  }
  if (lastq) {
      dest=lastq++;
      while ((*dest++=*lastq++) != 0)
          ;
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
  GetTempFileName (temp_path, "CMD", 0, temp_file);
  p = strchr(command, '|');
  *p++ = '\0';
  wsprintf (temp_cmd, "%s > %s", command, temp_file);
  WCMD_process_command (temp_cmd);
  command = p;
  while ((p = strchr(command, '|'))) {
    *p++ = '\0';
    GetTempFileName (temp_path, "CMD", 0, temp_file2);
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
