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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cmd);

const WCHAR inbuilt[][10] = {
        {'A','T','T','R','I','B','\0'},
        {'C','A','L','L','\0'},
        {'C','D','\0'},
        {'C','H','D','I','R','\0'},
        {'C','L','S','\0'},
        {'C','O','P','Y','\0'},
        {'C','T','T','Y','\0'},
        {'D','A','T','E','\0'},
        {'D','E','L','\0'},
        {'D','I','R','\0'},
        {'E','C','H','O','\0'},
        {'E','R','A','S','E','\0'},
        {'F','O','R','\0'},
        {'G','O','T','O','\0'},
        {'H','E','L','P','\0'},
        {'I','F','\0'},
        {'L','A','B','E','L','\0'},
        {'M','D','\0'},
        {'M','K','D','I','R','\0'},
        {'M','O','V','E','\0'},
        {'P','A','T','H','\0'},
        {'P','A','U','S','E','\0'},
        {'P','R','O','M','P','T','\0'},
        {'R','E','M','\0'},
        {'R','E','N','\0'},
        {'R','E','N','A','M','E','\0'},
        {'R','D','\0'},
        {'R','M','D','I','R','\0'},
        {'S','E','T','\0'},
        {'S','H','I','F','T','\0'},
        {'T','I','M','E','\0'},
        {'T','I','T','L','E','\0'},
        {'T','Y','P','E','\0'},
        {'V','E','R','I','F','Y','\0'},
        {'V','E','R','\0'},
        {'V','O','L','\0'},
        {'E','N','D','L','O','C','A','L','\0'},
        {'S','E','T','L','O','C','A','L','\0'},
        {'P','U','S','H','D','\0'},
        {'P','O','P','D','\0'},
        {'A','S','S','O','C','\0'},
        {'C','O','L','O','R','\0'},
        {'F','T','Y','P','E','\0'},
        {'M','O','R','E','\0'},
        {'E','X','I','T','\0'}
};

HINSTANCE hinst;
DWORD errorlevel;
int echo_mode = 1, verify_mode = 0, defaultColor = 7;
static int opt_c, opt_k, opt_s;
const WCHAR newline[] = {'\n','\0'};
static const WCHAR equalsW[] = {'=','\0'};
static const WCHAR closeBW[] = {')','\0'};
WCHAR anykey[100];
WCHAR version_string[100];
WCHAR quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
BATCH_CONTEXT *context = NULL;
extern struct env_stack *pushd_directories;
static const WCHAR *pagedMessage = NULL;
static char  *output_bufA = NULL;
#define MAX_WRITECONSOLE_SIZE 65535
BOOL unicodePipes = FALSE;

static WCHAR *WCMD_expand_envvar(WCHAR *start);

/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */

int wmain (int argc, WCHAR *argvW[])
{
  int     args;
  WCHAR  *cmd   = NULL;
  WCHAR string[1024];
  WCHAR envvar[4];
  HANDLE h;
  int opt_q;
  int opt_t = 0;
  static const WCHAR autoexec[] = {'\\','a','u','t','o','e','x','e','c','.',
                                   'b','a','t','\0'};
  char ansiVersion[100];
  CMD_LIST *toExecute = NULL;         /* Commands left to be executed */

  /* Pre initialize some messages */
  strcpy(ansiVersion, PACKAGE_VERSION);
  MultiByteToWideChar(CP_ACP, 0, ansiVersion, -1, string, 1024);
  wsprintf(version_string, WCMD_LoadMessage(WCMD_VERSION), string);
  strcpyW(anykey, WCMD_LoadMessage(WCMD_ANYKEY));

  args  = argc;
  opt_c=opt_k=opt_q=opt_s=0;
  while (args > 0)
  {
      WCHAR c;
      WINE_TRACE("Command line parm: '%s'\n", wine_dbgstr_w(*argvW));
      if ((*argvW)[0]!='/' || (*argvW)[1]=='\0') {
          argvW++;
          args--;
          continue;
      }

      c=(*argvW)[1];
      if (tolowerW(c)=='c') {
          opt_c=1;
      } else if (tolowerW(c)=='q') {
          opt_q=1;
      } else if (tolowerW(c)=='k') {
          opt_k=1;
      } else if (tolowerW(c)=='s') {
          opt_s=1;
      } else if (tolowerW(c)=='a') {
          unicodePipes=FALSE;
      } else if (tolowerW(c)=='u') {
          unicodePipes=TRUE;
      } else if (tolowerW(c)=='t' && (*argvW)[2]==':') {
          opt_t=strtoulW(&(*argvW)[3], NULL, 16);
      } else if (tolowerW(c)=='x' || tolowerW(c)=='y') {
          /* Ignored for compatibility with Windows */
      }

      if ((*argvW)[2]==0) {
          argvW++;
          args--;
      }
      else /* handle `cmd /cnotepad.exe` and `cmd /x/c ...` */
      {
          *argvW+=2;
      }

      if (opt_c || opt_k) /* break out of parsing immediately after c or k */
          break;
  }

  if (opt_q) {
    const WCHAR eoff[] = {'O','F','F','\0'};
    WCMD_echo(eoff);
  }

  if (opt_c || opt_k) {
      int     len,qcount;
      WCHAR** arg;
      int     argsLeft;
      WCHAR*  p;

      /* opt_s left unflagged if the command starts with and contains exactly
       * one quoted string (exactly two quote characters). The quoted string
       * must be an executable name that has whitespace and must not have the
       * following characters: &<>()@^| */

      /* Build the command to execute */
      len = 0;
      qcount = 0;
      argsLeft = args;
      for (arg = argvW; argsLeft>0; arg++,argsLeft--)
      {
          int has_space,bcount;
          WCHAR* a;

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
          len+=(a-*arg) + 1; /* for the separating space */
          if (has_space)
          {
              len+=2; /* for the quotes */
              qcount+=2;
          }
      }

      if (qcount!=2)
          opt_s=1;

      /* check argvW[0] for a space and invalid characters */
      if (!opt_s) {
          opt_s=1;
          p=*argvW;
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

      cmd = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
      if (!cmd)
          exit(1);

      p = cmd;
      argsLeft = args;
      for (arg = argvW; argsLeft>0; arg++,argsLeft--)
      {
          int has_space,has_quote;
          WCHAR* a;

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
              WCHAR* a;

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
              strcpyW(p,*arg);
              p+=strlenW(*arg);
          }
          if (has_space)
              *p++='"';
          *p++=' ';
      }
      if (p > cmd)
          p--;  /* remove last space */
      *p = '\0';

      WINE_TRACE("/c command line: '%s'\n", wine_dbgstr_w(cmd));

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

      /* Parse the command string, without reading any more input */
      WCMD_ReadAndParseLine(cmd, &toExecute, INVALID_HANDLE_VALUE);
      WCMD_process_commands(toExecute, FALSE, NULL, NULL);
      WCMD_free_commands(toExecute);
      toExecute = NULL;

      HeapFree(GetProcessHeap(), 0, cmd);
      return errorlevel;
  }

  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_LINE_INPUT |
                 ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
  SetConsoleTitle(WCMD_LoadMessage(WCMD_CONSTITLE));

  /* Note: cmd.exe /c dir does not get a new color, /k dir does */
  if (opt_t) {
      if (!(((opt_t & 0xF0) >> 4) == (opt_t & 0x0F))) {
          defaultColor = opt_t & 0xFF;
          param1[0] = 0x00;
          WCMD_color();
      }
  } else {
      /* Check HKCU\Software\Microsoft\Command Processor
         Then  HKLM\Software\Microsoft\Command Processor
           for defaultcolour value
           Note  Can be supplied as DWORD or REG_SZ
           Note2 When supplied as REG_SZ it's in decimal!!! */
      HKEY key;
      DWORD type;
      DWORD value=0, size=4;
      static const WCHAR regKeyW[] = {'S','o','f','t','w','a','r','e','\\',
                                      'M','i','c','r','o','s','o','f','t','\\',
                                      'C','o','m','m','a','n','d',' ','P','r','o','c','e','s','s','o','r','\0'};
      static const WCHAR dfltColorW[] = {'D','e','f','a','u','l','t','C','o','l','o','r','\0'};

      if (RegOpenKeyEx(HKEY_CURRENT_USER, regKeyW,
                       0, KEY_READ, &key) == ERROR_SUCCESS) {
          WCHAR  strvalue[4];

          /* See if DWORD or REG_SZ */
          if (RegQueryValueEx(key, dfltColorW, NULL, &type,
                     NULL, NULL) == ERROR_SUCCESS) {
              if (type == REG_DWORD) {
                  size = sizeof(DWORD);
                  RegQueryValueEx(key, dfltColorW, NULL, NULL,
                                  (LPBYTE)&value, &size);
              } else if (type == REG_SZ) {
                  size = sizeof(strvalue)/sizeof(WCHAR);
                  RegQueryValueEx(key, dfltColorW, NULL, NULL,
                                  (LPBYTE)strvalue, &size);
                  value = strtoulW(strvalue, NULL, 10);
              }
          }
      }

      if (value == 0 && RegOpenKeyEx(HKEY_LOCAL_MACHINE, regKeyW,
                       0, KEY_READ, &key) == ERROR_SUCCESS) {
          WCHAR  strvalue[4];

          /* See if DWORD or REG_SZ */
          if (RegQueryValueEx(key, dfltColorW, NULL, &type,
                     NULL, NULL) == ERROR_SUCCESS) {
              if (type == REG_DWORD) {
                  size = sizeof(DWORD);
                  RegQueryValueEx(key, dfltColorW, NULL, NULL,
                                  (LPBYTE)&value, &size);
              } else if (type == REG_SZ) {
                  size = sizeof(strvalue)/sizeof(WCHAR);
                  RegQueryValueEx(key, dfltColorW, NULL, NULL,
                                  (LPBYTE)strvalue, &size);
                  value = strtoulW(strvalue, NULL, 10);
              }
          }
      }

      /* If one found, set the screen to that colour */
      if (!(((value & 0xF0) >> 4) == (value & 0x0F))) {
          defaultColor = value & 0xFF;
          param1[0] = 0x00;
          WCMD_color();
      }

  }

  /* Save cwd into appropriate env var */
  GetCurrentDirectory(1024, string);
  if (IsCharAlpha(string[0]) && string[1] == ':') {
    static const WCHAR fmt[] = {'=','%','c',':','\0'};
    wsprintf(envvar, fmt, string[0]);
    SetEnvironmentVariable(envvar, string);
  }

  if (opt_k) {
      /* Parse the command string, without reading any more input */
      WCMD_ReadAndParseLine(cmd, &toExecute, INVALID_HANDLE_VALUE);
      WCMD_process_commands(toExecute, FALSE, NULL, NULL);
      WCMD_free_commands(toExecute);
      toExecute = NULL;
      HeapFree(GetProcessHeap(), 0, cmd);
  }

/*
 *	If there is an AUTOEXEC.BAT file, try to execute it.
 */

  GetFullPathName (autoexec, sizeof(string)/sizeof(WCHAR), string, NULL);
  h = CreateFile (string, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (h != INVALID_HANDLE_VALUE) {
    CloseHandle (h);
#if 0
    WCMD_batch (autoexec, autoexec, 0, NULL, INVALID_HANDLE_VALUE);
#endif
  }

/*
 *	Loop forever getting commands and executing them.
 */

  WCMD_version ();
  while (TRUE) {

    /* Read until EOF (which for std input is never, but if redirect
       in place, may occur                                          */
    WCMD_show_prompt ();
    if (WCMD_ReadAndParseLine(NULL, &toExecute,
                              GetStdHandle(STD_INPUT_HANDLE)) == NULL)
      break;
    WCMD_process_commands(toExecute, FALSE, NULL, NULL);
    WCMD_free_commands(toExecute);
    toExecute = NULL;
  }
  return 0;
}


/*****************************************************************************
 * Process one command. If the command is EXIT this routine does not return.
 * We will recurse through here executing batch files.
 */


void WCMD_process_command (WCHAR *command, CMD_LIST **cmdList)
{
    WCHAR *cmd, *p, *s, *t, *redir;
    int status, i;
    DWORD count, creationDisposition;
    HANDLE h;
    WCHAR *whichcmd;
    SECURITY_ATTRIBUTES sa;
    WCHAR *new_cmd;
    WCHAR *first_redir = NULL;
    HANDLE old_stdhandles[3] = {INVALID_HANDLE_VALUE,
                                INVALID_HANDLE_VALUE,
                                INVALID_HANDLE_VALUE};
    DWORD  idx_stdhandles[3] = {STD_INPUT_HANDLE,
                                STD_OUTPUT_HANDLE,
                                STD_ERROR_HANDLE};

    /* Move copy of the command onto the heap so it can be expanded */
    new_cmd = HeapAlloc( GetProcessHeap(), 0, MAXSTRING * sizeof(WCHAR));
    strcpyW(new_cmd, command);

    /* For commands in a context (batch program):                  */
    /*   Expand environment variables in a batch file %{0-9} first */
    /*     including support for any ~ modifiers                   */
    /* Additionally:                                               */
    /*   Expand the DATE, TIME, CD, RANDOM and ERRORLEVEL special  */
    /*     names allowing environment variable overrides           */
    /* NOTE: To support the %PATH:xxx% syntax, also perform        */
    /*   manual expansion of environment variables here            */

    p = new_cmd;
    while ((p = strchrW(p, '%'))) {
      i = *(p+1) - '0';

      /* Don't touch %% */
      if (*(p+1) == '%') {
        p+=2;

      /* Replace %~ modifications if in batch program */
      } else if (context && *(p+1) == '~') {
        WCMD_HandleTildaModifiers(&p, NULL);
        p++;

      /* Replace use of %0...%9 if in batch program*/
      } else if (context && (i >= 0) && (i <= 9)) {
        s = WCMD_strdupW(p+2);
        t = WCMD_parameter (context -> command, i + context -> shift_count[i], NULL);
        strcpyW (p, t);
        strcatW (p, s);
        free (s);

      /* Replace use of %* if in batch program*/
      } else if (context && *(p+1)=='*') {
        WCHAR *startOfParms = NULL;
        s = WCMD_strdupW(p+2);
        t = WCMD_parameter (context -> command, 1, &startOfParms);
        if (startOfParms != NULL) strcpyW (p, startOfParms);
        else *p = 0x00;
        strcatW (p, s);
        free (s);

      } else {
        p = WCMD_expand_envvar(p);
      }
    }
    cmd = new_cmd;

    /* In a batch program, unknown variables are replace by nothing */
    /* so remove any remaining %var%                                */
    if (context) {
      p = cmd;
      while ((p = strchrW(p, '%'))) {
        if (*(p+1) == '%') {
          p+=2;
        } else {
          s = strchrW(p+1, '%');
          if (!s) {
            *p=0x00;
          } else {
            t = WCMD_strdupW(s+1);
            strcpyW(p, t);
            free(t);
          }
        }
      }

      /* Show prompt before batch line IF echo is on and in batch program */
      if (echo_mode && (cmd[0] != '@')) {
        WCMD_show_prompt();
        WCMD_output_asis ( cmd);
        WCMD_output_asis ( newline);
      }
    }

/*
 *	Changing default drive has to be handled as a special case.
 */

    if ((cmd[1] == ':') && IsCharAlpha (cmd[0]) && (strlenW(cmd) == 2)) {
      WCHAR envvar[5];
      WCHAR dir[MAX_PATH];

      /* According to MSDN CreateProcess docs, special env vars record
         the current directory on each drive, in the form =C:
         so see if one specified, and if so go back to it             */
      strcpyW(envvar, equalsW);
      strcatW(envvar, cmd);
      if (GetEnvironmentVariable(envvar, dir, MAX_PATH) == 0) {
        static const WCHAR fmt[] = {'%','s','\\','\0'};
        wsprintf(cmd, fmt, cmd);
      }
      status = SetCurrentDirectory (cmd);
      if (!status) WCMD_print_error ();
      HeapFree( GetProcessHeap(), 0, cmd );
      return;
    }

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

/*
 *	Redirect stdin, stdout and/or stderr if required.
 */

    if ((p = strchrW(cmd,'<')) != NULL) {
      if (first_redir == NULL) first_redir = p;
      h = CreateFile (WCMD_parameter (++p, 0, NULL), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE) {
	WCMD_print_error ();
        HeapFree( GetProcessHeap(), 0, cmd );
	return;
      }
      old_stdhandles[0] = GetStdHandle (STD_INPUT_HANDLE);
      SetStdHandle (STD_INPUT_HANDLE, h);
    }

    /* Scan the whole command looking for > and 2> */
    redir = cmd;
    while (redir != NULL && ((p = strchrW(redir,'>')) != NULL)) {
      int handle = 0;

      if (*(p-1)!='2') {
        if (first_redir == NULL) first_redir = p;
        handle = 1;
      } else {
        if (first_redir == NULL) first_redir = (p-1);
        handle = 2;
      }

      p++;
      if ('>' == *p) {
        creationDisposition = OPEN_ALWAYS;
        p++;
      }
      else {
        creationDisposition = CREATE_ALWAYS;
      }

      /* Add support for 2>&1 */
      redir = p;
      if (*p == '&') {
        int idx = *(p+1) - '0';

        if (DuplicateHandle(GetCurrentProcess(),
                        GetStdHandle(idx_stdhandles[idx]),
                        GetCurrentProcess(),
                        &h,
                        0, TRUE, DUPLICATE_SAME_ACCESS) == 0) {
          WINE_FIXME("Duplicating handle failed with gle %d\n", GetLastError());
        }
        WINE_TRACE("Redirect %d (%p) to %d (%p)\n", handle, GetStdHandle(idx_stdhandles[idx]), idx, h);

      } else {
        WCHAR *param = WCMD_parameter (p, 0, NULL);
        h = CreateFile (param, GENERIC_WRITE, 0, &sa, creationDisposition,
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
        WINE_TRACE("Redirect %d to '%s' (%p)\n", handle, wine_dbgstr_w(param), h);
      }

      old_stdhandles[handle] = GetStdHandle (idx_stdhandles[handle]);
      SetStdHandle (idx_stdhandles[handle], h);
    }

    /* Terminate the command string at <, or first 2> or > */
    if (first_redir != NULL) *first_redir = '\0';

/*
 * Strip leading whitespaces, and a '@' if supplied
 */
    whichcmd = WCMD_strtrim_leading_spaces(cmd);
    WINE_TRACE("Command: '%s'\n", wine_dbgstr_w(cmd));
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
        WCMD_call (p);
        break;
      case WCMD_CD:
      case WCMD_CHDIR:
        WCMD_setshow_default (p);
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
        WCMD_delete (p, TRUE);
        break;
      case WCMD_DIR:
        WCMD_directory (p);
        break;
      case WCMD_ECHO:
        WCMD_echo(&whichcmd[count]);
        break;
      case WCMD_FOR:
        WCMD_for (p, cmdList);
        break;
      case WCMD_GOTO:
        WCMD_goto (cmdList);
        break;
      case WCMD_HELP:
        WCMD_give_help (p);
	break;
      case WCMD_IF:
	WCMD_if (p, cmdList);
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
        WCMD_remove_dir (p);
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
        WCMD_shift (p);
        break;
      case WCMD_TIME:
        WCMD_setshow_time ();
        break;
      case WCMD_TITLE:
        if (strlenW(&whichcmd[count]) > 0)
          WCMD_title(&whichcmd[count+1]);
        break;
      case WCMD_TYPE:
        WCMD_type (p);
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
      case WCMD_PUSHD:
        WCMD_pushd(p);
        break;
      case WCMD_POPD:
        WCMD_popd();
        break;
      case WCMD_ASSOC:
        WCMD_assoc(p, TRUE);
        break;
      case WCMD_COLOR:
        WCMD_color();
        break;
      case WCMD_FTYPE:
        WCMD_assoc(p, FALSE);
        break;
      case WCMD_MORE:
        WCMD_more(p);
        break;
      case WCMD_EXIT:
        WCMD_exit (cmdList);
        break;
      default:
        WCMD_run_program (whichcmd, 0);
    }
    HeapFree( GetProcessHeap(), 0, cmd );

    /* Restore old handles */
    for (i=0; i<3; i++) {
      if (old_stdhandles[i] != INVALID_HANDLE_VALUE) {
        CloseHandle (GetStdHandle (idx_stdhandles[i]));
        SetStdHandle (idx_stdhandles[i], old_stdhandles[i]);
      }
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
    if (st_p.cbReserved2 && st_p.lpReserved2)
    {
        /* Override the entries for fd 0,1,2 if we happened
         * to change those std handles (this depends on the way wcmd sets
         * it's new input & output handles)
         */
        size_t sz = max(sizeof(unsigned) + (sizeof(WCHAR) + sizeof(HANDLE)) * 3, st_p.cbReserved2);
        BYTE* ptr = HeapAlloc(GetProcessHeap(), 0, sz);
        if (ptr)
        {
            unsigned num = *(unsigned*)st_p.lpReserved2;
            WCHAR* flags = (WCHAR*)(ptr + sizeof(unsigned));
            HANDLE* handles = (HANDLE*)(flags + num * sizeof(WCHAR));

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
 *	Execute a command line as an external program. Must allow recursion.
 *
 *      Precedence:
 *        Manual testing under windows shows PATHEXT plays a key part in this,
 *      and the search algorithm and precedence appears to be as follows.
 *
 *      Search locations:
 *        If directory supplied on command, just use that directory
 *        If extension supplied on command, look for that explicit name first
 *        Otherwise, search in each directory on the path
 *      Precedence:
 *        If extension supplied on command, look for that explicit name first
 *        Then look for supplied name .* (even if extension supplied, so
 *          'garbage.exe' will match 'garbage.exe.cmd')
 *        If any found, cycle through PATHEXT looking for name.exe one by one
 *      Launching
 *        Once a match has been found, it is launched - Code currently uses
 *          findexecutable to acheive this which is left untouched.
 */

void WCMD_run_program (WCHAR *command, int called) {

  WCHAR  temp[MAX_PATH];
  WCHAR  pathtosearch[MAXSTRING];
  WCHAR *pathposn;
  WCHAR  stemofsearch[MAX_PATH];
  WCHAR *lastSlash;
  WCHAR  pathext[MAXSTRING];
  BOOL  extensionsupplied = FALSE;
  BOOL  launched = FALSE;
  BOOL  status;
  BOOL  assumeInternal = FALSE;
  DWORD len;
  static const WCHAR envPath[] = {'P','A','T','H','\0'};
  static const WCHAR envPathExt[] = {'P','A','T','H','E','X','T','\0'};
  static const WCHAR delims[] = {'/','\\',':','\0'};

  WCMD_parse (command, quals, param1, param2);	/* Quick way to get the filename */
  if (!(*param1) && !(*param2))
    return;

  /* Calculate the search path and stem to search for */
  if (strpbrkW (param1, delims) == NULL) {  /* No explicit path given, search path */
    static const WCHAR curDir[] = {'.',';','\0'};
    strcpyW(pathtosearch, curDir);
    len = GetEnvironmentVariable (envPath, &pathtosearch[2], (sizeof(pathtosearch)/sizeof(WCHAR))-2);
    if ((len == 0) || (len >= (sizeof(pathtosearch)/sizeof(WCHAR)) - 2)) {
      static const WCHAR curDir[] = {'.','\0'};
      strcpyW (pathtosearch, curDir);
    }
    if (strchrW(param1, '.') != NULL) extensionsupplied = TRUE;
    strcpyW(stemofsearch, param1);

  } else {

    /* Convert eg. ..\fred to include a directory by removing file part */
    GetFullPathName(param1, sizeof(pathtosearch)/sizeof(WCHAR), pathtosearch, NULL);
    lastSlash = strrchrW(pathtosearch, '\\');
    if (lastSlash && strchrW(lastSlash, '.') != NULL) extensionsupplied = TRUE;
    if (lastSlash) *lastSlash = 0x00;
    strcpyW(stemofsearch, lastSlash+1);
  }

  /* Now extract PATHEXT */
  len = GetEnvironmentVariable (envPathExt, pathext, sizeof(pathext)/sizeof(WCHAR));
  if ((len == 0) || (len >= (sizeof(pathext)/sizeof(WCHAR)))) {
    static const WCHAR dfltPathExt[] = {'.','b','a','t',';',
                                        '.','c','o','m',';',
                                        '.','c','m','d',';',
                                        '.','e','x','e','\0'};
    strcpyW (pathext, dfltPathExt);
  }

  /* Loop through the search path, dir by dir */
  pathposn = pathtosearch;
  WINE_TRACE("Searching in '%s' for '%s'\n", wine_dbgstr_w(pathtosearch),
             wine_dbgstr_w(stemofsearch));
  while (!launched && pathposn) {

    WCHAR  thisDir[MAX_PATH] = {'\0'};
    WCHAR *pos               = NULL;
    BOOL  found             = FALSE;
    const WCHAR slashW[] = {'\\','\0'};

    /* Work on the first directory on the search path */
    pos = strchrW(pathposn, ';');
    if (pos) {
      memcpy(thisDir, pathposn, (pos-pathposn) * sizeof(WCHAR));
      thisDir[(pos-pathposn)] = 0x00;
      pathposn = pos+1;

    } else {
      strcpyW(thisDir, pathposn);
      pathposn = NULL;
    }

    /* Since you can have eg. ..\.. on the path, need to expand
       to full information                                      */
    strcpyW(temp, thisDir);
    GetFullPathName(temp, MAX_PATH, thisDir, NULL);

    /* 1. If extension supplied, see if that file exists */
    strcatW(thisDir, slashW);
    strcatW(thisDir, stemofsearch);
    pos = &thisDir[strlenW(thisDir)]; /* Pos = end of name */

    /* 1. If extension supplied, see if that file exists */
    if (extensionsupplied) {
      if (GetFileAttributes(thisDir) != INVALID_FILE_ATTRIBUTES) {
        found = TRUE;
      }
    }

    /* 2. Any .* matches? */
    if (!found) {
      HANDLE          h;
      WIN32_FIND_DATA finddata;
      static const WCHAR allFiles[] = {'.','*','\0'};

      strcatW(thisDir,allFiles);
      h = FindFirstFile(thisDir, &finddata);
      FindClose(h);
      if (h != INVALID_HANDLE_VALUE) {

        WCHAR *thisExt = pathext;

        /* 3. Yes - Try each path ext */
        while (thisExt) {
          WCHAR *nextExt = strchrW(thisExt, ';');

          if (nextExt) {
            memcpy(pos, thisExt, (nextExt-thisExt) * sizeof(WCHAR));
            pos[(nextExt-thisExt)] = 0x00;
            thisExt = nextExt+1;
          } else {
            strcpyW(pos, thisExt);
            thisExt = NULL;
          }

          if (GetFileAttributes(thisDir) != INVALID_FILE_ATTRIBUTES) {
            found = TRUE;
            thisExt = NULL;
          }
        }
      }
    }

   /* Internal programs won't be picked up by this search, so even
      though not found, try one last createprocess and wait for it
      to complete.
      Note: Ideally we could tell between a console app (wait) and a
      windows app, but the API's for it fail in this case           */
    if (!found && pathposn == NULL) {
        WINE_TRACE("ASSUMING INTERNAL\n");
        assumeInternal = TRUE;
    } else {
        WINE_TRACE("Found as %s\n", wine_dbgstr_w(thisDir));
    }

    /* Once found, launch it */
    if (found || assumeInternal) {
      STARTUPINFO st;
      PROCESS_INFORMATION pe;
      SHFILEINFO psfi;
      DWORD console;
      HINSTANCE hinst;
      WCHAR *ext = strrchrW( thisDir, '.' );
      static const WCHAR batExt[] = {'.','b','a','t','\0'};
      static const WCHAR cmdExt[] = {'.','c','m','d','\0'};

      launched = TRUE;

      /* Special case BAT and CMD */
      if (ext && !strcmpiW(ext, batExt)) {
        WCMD_batch (thisDir, command, called, NULL, INVALID_HANDLE_VALUE);
        return;
      } else if (ext && !strcmpiW(ext, cmdExt)) {
        WCMD_batch (thisDir, command, called, NULL, INVALID_HANDLE_VALUE);
        return;
      } else {

        /* thisDir contains the file to be launched, but with what?
           eg. a.exe will require a.exe to be launched, a.html may be iexplore */
        hinst = FindExecutable (thisDir, NULL, temp);
        if ((INT_PTR)hinst < 32)
          console = 0;
        else
          console = SHGetFileInfo (temp, 0, &psfi, sizeof(psfi), SHGFI_EXETYPE);

        ZeroMemory (&st, sizeof(STARTUPINFO));
        st.cb = sizeof(STARTUPINFO);
        init_msvcrt_io_block(&st);

        /* Launch the process and if a CUI wait on it to complete
           Note: Launching internal wine processes cannot specify a full path to exe */
        status = CreateProcess (assumeInternal?NULL : thisDir,
                                command, NULL, NULL, TRUE, 0, NULL, NULL, &st, &pe);
        if ((opt_c || opt_k) && !opt_s && !status
            && GetLastError()==ERROR_FILE_NOT_FOUND && command[0]=='\"') {
          /* strip first and last quote WCHARacters and try again */
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
        if (!assumeInternal && !console) errorlevel = 0;
        else
        {
            if (assumeInternal || !HIWORD(console)) WaitForSingleObject (pe.hProcess, INFINITE);
            GetExitCodeProcess (pe.hProcess, &errorlevel);
            if (errorlevel == STILL_ACTIVE) errorlevel = 0;
        }
        CloseHandle(pe.hProcess);
        CloseHandle(pe.hThread);
        return;
      }
    }
  }

  /* Not found anywhere - give up */
  SetLastError(ERROR_FILE_NOT_FOUND);
  WCMD_print_error ();

  /* If a command fails to launch, it sets errorlevel 9009 - which
     does not seem to have any associated constant definition     */
  errorlevel = 9009;
  return;

}

/******************************************************************************
 * WCMD_show_prompt
 *
 *	Display the prompt on STDout
 *
 */

void WCMD_show_prompt (void) {

  int status;
  WCHAR out_string[MAX_PATH], curdir[MAX_PATH], prompt_string[MAX_PATH];
  WCHAR *p, *q;
  DWORD len;
  static const WCHAR envPrompt[] = {'P','R','O','M','P','T','\0'};

  len = GetEnvironmentVariable (envPrompt, prompt_string,
                                sizeof(prompt_string)/sizeof(WCHAR));
  if ((len == 0) || (len >= (sizeof(prompt_string)/sizeof(WCHAR)))) {
    const WCHAR dfltPrompt[] = {'$','P','$','G','\0'};
    strcpyW (prompt_string, dfltPrompt);
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
	case 'A':
	  *q++ = '&';
	  break;
	case 'B':
	  *q++ = '|';
	  break;
	case 'C':
	  *q++ = '(';
	  break;
	case 'D':
	  GetDateFormat (LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, q, MAX_PATH);
	  while (*q) q++;
	  break;
	case 'E':
	  *q++ = '\E';
	  break;
	case 'F':
	  *q++ = ')';
	  break;
	case 'G':
	  *q++ = '>';
	  break;
	case 'H':
	  *q++ = '\b';
	  break;
	case 'L':
	  *q++ = '<';
	  break;
	case 'N':
          status = GetCurrentDirectory (sizeof(curdir)/sizeof(WCHAR), curdir);
	  if (status) {
	    *q++ = curdir[0];
	  }
	  break;
	case 'P':
          status = GetCurrentDirectory (sizeof(curdir)/sizeof(WCHAR), curdir);
	  if (status) {
	    strcatW (q, curdir);
	    while (*q) q++;
	  }
	  break;
	case 'Q':
	  *q++ = '=';
	  break;
	case 'S':
	  *q++ = ' ';
	  break;
	case 'T':
	  GetTimeFormat (LOCALE_USER_DEFAULT, 0, NULL, NULL, q, MAX_PATH);
	  while (*q) q++;
	  break;
        case 'V':
	  strcatW (q, version_string);
	  while (*q) q++;
          break;
	case '_':
	  *q++ = '\n';
	  break;
	case '+':
	  if (pushd_directories) {
	    memset(q, '+', pushd_directories->u.stackdepth);
	    q = q + pushd_directories->u.stackdepth;
	  }
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
    WINE_FIXME ("Cannot display message for error %d, status %d\n",
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

void WCMD_parse (WCHAR *s, WCHAR *q, WCHAR *p1, WCHAR *p2) {

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
 * WCMD_output_asis_len - send output to current standard output
 *
 * Output a formatted unicode string. Ideally this will go to the console
 *  and hence required WriteConsoleW to output it, however if file i/o is
 *  redirected, it needs to be WriteFile'd using OEM (not ANSI) format
 */
static void WCMD_output_asis_len(const WCHAR *message, int len) {

    DWORD   nOut= 0;
    DWORD   res = 0;

    /* If nothing to write, return (MORE does this sometimes) */
    if (!len) return;

    /* Try to write as unicode assuming it is to a console */
    res = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                        message, len, &nOut, NULL);

    /* If writing to console fails, assume its file
       i/o so convert to OEM codepage and output                  */
    if (!res) {
      BOOL usedDefaultChar = FALSE;
      DWORD convertedChars;

      if (!unicodePipes) {
        /*
         * Allocate buffer to use when writing to file. (Not freed, as one off)
         */
        if (!output_bufA) output_bufA = HeapAlloc(GetProcessHeap(), 0,
                                                  MAX_WRITECONSOLE_SIZE);
        if (!output_bufA) {
          WINE_FIXME("Out of memory - could not allocate ansi 64K buffer\n");
          return;
        }

        /* Convert to OEM, then output */
        convertedChars = WideCharToMultiByte(GetConsoleOutputCP(), 0, message,
                            len, output_bufA, MAX_WRITECONSOLE_SIZE,
                            "?", &usedDefaultChar);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), output_bufA, convertedChars,
                  &nOut, FALSE);
      } else {
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), message, len*sizeof(WCHAR),
                  &nOut, FALSE);
      }
    }
    return;
}

/*******************************************************************
 * WCMD_output - send output to current standard output device.
 *
 */

void WCMD_output (const WCHAR *format, ...) {

  va_list ap;
  WCHAR string[1024];
  int ret;

  va_start(ap,format);
  ret = wvsprintf (string, format, ap);
  if( ret >= (sizeof(string)/sizeof(WCHAR))) {
       WINE_ERR("Output truncated in WCMD_output\n" );
       ret = (sizeof(string)/sizeof(WCHAR)) - 1;
       string[ret] = '\0';
  }
  va_end(ap);
  WCMD_output_asis_len(string, ret);
}


static int line_count;
static int max_height;
static int max_width;
static BOOL paged_mode;
static int numChars;

void WCMD_enter_paged_mode(const WCHAR *msg)
{
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo)) {
    max_height = consoleInfo.dwSize.Y;
    max_width  = consoleInfo.dwSize.X;
  } else {
    max_height = 25;
    max_width  = 80;
  }
  paged_mode = TRUE;
  line_count = 0;
  numChars   = 0;
  pagedMessage = (msg==NULL)? anykey : msg;
}

void WCMD_leave_paged_mode(void)
{
  paged_mode = FALSE;
  pagedMessage = NULL;
}

/*******************************************************************
 * WCMD_output_asis - send output to current standard output device.
 *        without formatting eg. when message contains '%'
 */

void WCMD_output_asis (const WCHAR *message) {
  DWORD count;
  const WCHAR* ptr;
  WCHAR string[1024];

  if (paged_mode) {
    do {
      ptr = message;
      while (*ptr && *ptr!='\n' && (numChars < max_width)) {
        numChars++;
        ptr++;
      };
      if (*ptr == '\n') ptr++;
      WCMD_output_asis_len(message, (ptr) ? ptr - message : strlenW(message));
      if (ptr) {
        numChars = 0;
        if (++line_count >= max_height - 1) {
          line_count = 0;
          WCMD_output_asis_len(pagedMessage, strlenW(pagedMessage));
          WCMD_ReadFile (GetStdHandle(STD_INPUT_HANDLE), string,
                         sizeof(string)/sizeof(WCHAR), &count, NULL);
        }
      }
    } while (((message = ptr) != NULL) && (*ptr));
  } else {
    WCMD_output_asis_len(message, lstrlen(message));
  }
}


/***************************************************************************
 * WCMD_strtrim_leading_spaces
 *
 *	Remove leading spaces from a string. Return a pointer to the first
 *	non-space character. Does not modify the input string
 */

WCHAR *WCMD_strtrim_leading_spaces (WCHAR *string) {

  WCHAR *ptr;

  ptr = string;
  while (*ptr == ' ') ptr++;
  return ptr;
}

/*************************************************************************
 * WCMD_strtrim_trailing_spaces
 *
 *	Remove trailing spaces from a string. This routine modifies the input
 *	string by placing a null after the last non-space WCHARacter
 */

void WCMD_strtrim_trailing_spaces (WCHAR *string) {

  WCHAR *ptr;

  ptr = string + strlenW (string) - 1;
  while ((*ptr == ' ') && (ptr >= string)) {
    *ptr = '\0';
    ptr--;
  }
}

/*************************************************************************
 * WCMD_opt_s_strip_quotes
 *
 *	Remove first and last quote WCHARacters, preserving all other text
 */

void WCMD_opt_s_strip_quotes(WCHAR *cmd) {
  WCHAR *src = cmd + 1, *dest = cmd, *lastq = NULL;
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

void WCMD_pipe (CMD_LIST **cmdEntry, WCHAR *var, WCHAR *val) {

  WCHAR *p;
  WCHAR *command = (*cmdEntry)->command;
  WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH], temp_file2[MAX_PATH], temp_cmd[1024];
  static const WCHAR redirOut[] = {'%','s',' ','>',' ','%','s','\0'};
  static const WCHAR redirIn[]  = {'%','s',' ','<',' ','%','s','\0'};
  static const WCHAR redirBoth[]= {'%','s',' ','<',' ','%','s',' ','>','%','s','\0'};
  static const WCHAR cmdW[]     = {'C','M','D','\0'};


  GetTempPath (sizeof(temp_path)/sizeof(WCHAR), temp_path);
  GetTempFileName (temp_path, cmdW, 0, temp_file);
  p = strchrW(command, '|');
  *p++ = '\0';
  wsprintf (temp_cmd, redirOut, command, temp_file);
  WCMD_execute (temp_cmd, var, val, cmdEntry);
  command = p;
  while ((p = strchrW(command, '|'))) {
    *p++ = '\0';
    GetTempFileName (temp_path, cmdW, 0, temp_file2);
    wsprintf (temp_cmd, redirBoth, command, temp_file, temp_file2);
    WCMD_execute (temp_cmd, var, val, cmdEntry);
    DeleteFile (temp_file);
    strcpyW (temp_file, temp_file2);
    command = p;
  }
  wsprintf (temp_cmd, redirIn, command, temp_file);
  WCMD_execute (temp_cmd, var, val, cmdEntry);
  DeleteFile (temp_file);
}

/*************************************************************************
 * WCMD_expand_envvar
 *
 *	Expands environment variables, allowing for WCHARacter substitution
 */
static WCHAR *WCMD_expand_envvar(WCHAR *start) {
    WCHAR *endOfVar = NULL, *s;
    WCHAR *colonpos = NULL;
    WCHAR thisVar[MAXSTRING];
    WCHAR thisVarContents[MAXSTRING];
    WCHAR savedchar = 0x00;
    int len;

    static const WCHAR ErrorLvl[]  = {'E','R','R','O','R','L','E','V','E','L','\0'};
    static const WCHAR ErrorLvlP[] = {'%','E','R','R','O','R','L','E','V','E','L','%','\0'};
    static const WCHAR Date[]      = {'D','A','T','E','\0'};
    static const WCHAR DateP[]     = {'%','D','A','T','E','%','\0'};
    static const WCHAR Time[]      = {'T','I','M','E','\0'};
    static const WCHAR TimeP[]     = {'%','T','I','M','E','%','\0'};
    static const WCHAR Cd[]        = {'C','D','\0'};
    static const WCHAR CdP[]       = {'%','C','D','%','\0'};
    static const WCHAR Random[]    = {'R','A','N','D','O','M','\0'};
    static const WCHAR RandomP[]   = {'%','R','A','N','D','O','M','%','\0'};

    /* Find the end of the environment variable, and extract name */
    endOfVar = strchrW(start+1, '%');
    if (endOfVar == NULL) {
      /* In batch program, missing terminator for % and no following
         ':' just removes the '%'                                   */
      s = WCMD_strdupW(start + 1);
      strcpyW (start, s);
      free(s);

      /* FIXME: Some other special conditions here depending on whether
         in batch, complex or not, and whether env var exists or not! */
      return start;
    }
    memcpy(thisVar, start, ((endOfVar - start) + 1) * sizeof(WCHAR));
    thisVar[(endOfVar - start)+1] = 0x00;
    colonpos = strchrW(thisVar+1, ':');

    /* If there's complex substitution, just need %var% for now
       to get the expanded data to play with                    */
    if (colonpos) {
        *colonpos = '%';
        savedchar = *(colonpos+1);
        *(colonpos+1) = 0x00;
    }

    WINE_TRACE("Retrieving contents of %s\n", wine_dbgstr_w(thisVar));

    /* Expand to contents, if unchanged, return */
    /* Handle DATE, TIME, ERRORLEVEL and CD replacements allowing */
    /* override if existing env var called that name              */
    if ((CompareString (LOCALE_USER_DEFAULT,
                        NORM_IGNORECASE | SORT_STRINGSORT,
                        thisVar, 12, ErrorLvlP, -1) == 2) &&
                (GetEnvironmentVariable(ErrorLvl, thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {
      static const WCHAR fmt[] = {'%','d','\0'};
      wsprintf(thisVarContents, fmt, errorlevel);
      len = strlenW(thisVarContents);

    } else if ((CompareString (LOCALE_USER_DEFAULT,
                               NORM_IGNORECASE | SORT_STRINGSORT,
                               thisVar, 6, DateP, -1) == 2) &&
                (GetEnvironmentVariable(Date, thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {

      GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL,
                    NULL, thisVarContents, MAXSTRING);
      len = strlenW(thisVarContents);

    } else if ((CompareString (LOCALE_USER_DEFAULT,
                               NORM_IGNORECASE | SORT_STRINGSORT,
                               thisVar, 6, TimeP, -1) == 2) &&
                (GetEnvironmentVariable(Time, thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {
      GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL,
                        NULL, thisVarContents, MAXSTRING);
      len = strlenW(thisVarContents);

    } else if ((CompareString (LOCALE_USER_DEFAULT,
                               NORM_IGNORECASE | SORT_STRINGSORT,
                               thisVar, 4, CdP, -1) == 2) &&
                (GetEnvironmentVariable(Cd, thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {
      GetCurrentDirectory (MAXSTRING, thisVarContents);
      len = strlenW(thisVarContents);

    } else if ((CompareString (LOCALE_USER_DEFAULT,
                               NORM_IGNORECASE | SORT_STRINGSORT,
                               thisVar, 8, RandomP, -1) == 2) &&
                (GetEnvironmentVariable(Random, thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {
      static const WCHAR fmt[] = {'%','d','\0'};
      wsprintf(thisVarContents, fmt, rand() % 32768);
      len = strlenW(thisVarContents);

    } else {

      len = ExpandEnvironmentStrings(thisVar, thisVarContents,
                               sizeof(thisVarContents)/sizeof(WCHAR));
    }

    if (len == 0)
      return endOfVar+1;

    /* In a batch program, unknown env vars are replaced with nothing,
         note syntax %garbage:1,3% results in anything after the ':'
         except the %
       From the command line, you just get back what you entered      */
    if (lstrcmpiW(thisVar, thisVarContents) == 0) {

      /* Restore the complex part after the compare */
      if (colonpos) {
        *colonpos = ':';
        *(colonpos+1) = savedchar;
      }

      s = WCMD_strdupW(endOfVar + 1);

      /* Command line - just ignore this */
      if (context == NULL) return endOfVar+1;

      /* Batch - replace unknown env var with nothing */
      if (colonpos == NULL) {
        strcpyW (start, s);

      } else {
        len = strlenW(thisVar);
        thisVar[len-1] = 0x00;
        /* If %:...% supplied, : is retained */
        if (colonpos == thisVar+1) {
          strcpyW (start, colonpos);
        } else {
          strcpyW (start, colonpos+1);
        }
        strcatW (start, s);
      }
      free (s);
      return start;

    }

    /* See if we need to do complex substitution (any ':'s), if not
       then our work here is done                                  */
    if (colonpos == NULL) {
      s = WCMD_strdupW(endOfVar + 1);
      strcpyW (start, thisVarContents);
      strcatW (start, s);
      free(s);
      return start;
    }

    /* Restore complex bit */
    *colonpos = ':';
    *(colonpos+1) = savedchar;

    /*
        Handle complex substitutions:
           xxx=yyy    (replace xxx with yyy)
           *xxx=yyy   (replace up to and including xxx with yyy)
           ~x         (from x WCHARs in)
           ~-x        (from x WCHARs from the end)
           ~x,y       (from x WCHARs in for y WCHARacters)
           ~x,-y      (from x WCHARs in until y WCHARacters from the end)
     */

    /* ~ is substring manipulation */
    if (savedchar == '~') {

      int   substrposition, substrlength = 0;
      WCHAR *commapos = strchrW(colonpos+2, ',');
      WCHAR *startCopy;

      substrposition = atolW(colonpos+2);
      if (commapos) substrlength = atolW(commapos+1);

      s = WCMD_strdupW(endOfVar + 1);

      /* Check bounds */
      if (substrposition >= 0) {
        startCopy = &thisVarContents[min(substrposition, len)];
      } else {
        startCopy = &thisVarContents[max(0, len+substrposition-1)];
      }

      if (commapos == NULL) {
        strcpyW (start, startCopy); /* Copy the lot */
      } else if (substrlength < 0) {

        int copybytes = (len+substrlength-1)-(startCopy-thisVarContents);
        if (copybytes > len) copybytes = len;
        else if (copybytes < 0) copybytes = 0;
        memcpy (start, startCopy, copybytes * sizeof(WCHAR)); /* Copy the lot */
        start[copybytes] = 0x00;
      } else {
        memcpy (start, startCopy, substrlength * sizeof(WCHAR)); /* Copy the lot */
        start[substrlength] = 0x00;
      }

      strcatW (start, s);
      free(s);
      return start;

    /* search and replace manipulation */
    } else {
      WCHAR *equalspos = strstrW(colonpos, equalsW);
      WCHAR *replacewith = equalspos+1;
      WCHAR *found       = NULL;
      WCHAR *searchIn;
      WCHAR *searchFor;

      s = WCMD_strdupW(endOfVar + 1);
      if (equalspos == NULL) return start+1;

      /* Null terminate both strings */
      thisVar[strlenW(thisVar)-1] = 0x00;
      *equalspos = 0x00;

      /* Since we need to be case insensitive, copy the 2 buffers */
      searchIn  = WCMD_strdupW(thisVarContents);
      CharUpperBuff(searchIn, strlenW(thisVarContents));
      searchFor = WCMD_strdupW(colonpos+1);
      CharUpperBuff(searchFor, strlenW(colonpos+1));


      /* Handle wildcard case */
      if (*(colonpos+1) == '*') {
        /* Search for string to replace */
        found = strstrW(searchIn, searchFor+1);

        if (found) {
          /* Do replacement */
          strcpyW(start, replacewith);
          strcatW(start, thisVarContents + (found-searchIn) + strlenW(searchFor+1));
          strcatW(start, s);
          free(s);
        } else {
          /* Copy as it */
          strcpyW(start, thisVarContents);
          strcatW(start, s);
        }

      } else {
        /* Loop replacing all instances */
        WCHAR *lastFound = searchIn;
        WCHAR *outputposn = start;

        *start = 0x00;
        while ((found = strstrW(lastFound, searchFor))) {
            lstrcpynW(outputposn,
                    thisVarContents + (lastFound-searchIn),
                    (found - lastFound)+1);
            outputposn  = outputposn + (found - lastFound);
            strcatW(outputposn, replacewith);
            outputposn = outputposn + strlenW(replacewith);
            lastFound = found + strlenW(searchFor);
        }
        strcatW(outputposn,
                thisVarContents + (lastFound-searchIn));
        strcatW(outputposn, s);
      }
      free(searchIn);
      free(searchFor);
      return start;
    }
    return start+1;
}

/*************************************************************************
 * WCMD_LoadMessage
 *    Load a string from the resource file, handling any error
 *    Returns string retrieved from resource file
 */
WCHAR *WCMD_LoadMessage(UINT id) {
    static WCHAR msg[2048];
    static const WCHAR failedMsg[]  = {'F','a','i','l','e','d','!','\0'};

    if (!LoadString(GetModuleHandle(NULL), id, msg, sizeof(msg)/sizeof(WCHAR))) {
       WINE_FIXME("LoadString failed with %d\n", GetLastError());
       strcpyW(msg, failedMsg);
    }
    return msg;
}

/*************************************************************************
 * WCMD_strdupW
 *    A wide version of strdup as its missing from unicode.h
 */
WCHAR *WCMD_strdupW(WCHAR *input) {
   int len=strlenW(input)+1;
   /* Note: Use malloc not HeapAlloc to emulate strdup */
   WCHAR *result = malloc(len * sizeof(WCHAR));
   memcpy(result, input, len * sizeof(WCHAR));
   return result;
}

/***************************************************************************
 * WCMD_Readfile
 *
 *	Read characters in from a console/file, returning result in Unicode
 *      with signature identical to ReadFile
 */
BOOL WCMD_ReadFile(const HANDLE hIn, WCHAR *intoBuf, const DWORD maxChars,
                          LPDWORD charsRead, const LPOVERLAPPED unused) {

    BOOL   res;

    /* Try to read from console as Unicode */
    res = ReadConsoleW(hIn, intoBuf, maxChars, charsRead, NULL);

    /* If reading from console has failed we assume its file
       i/o so read in and convert from OEM codepage               */
    if (!res) {

        DWORD numRead;
        /*
         * Allocate buffer to use when reading from file. Not freed
         */
        if (!output_bufA) output_bufA = HeapAlloc(GetProcessHeap(), 0,
                                                  MAX_WRITECONSOLE_SIZE);
        if (!output_bufA) {
          WINE_FIXME("Out of memory - could not allocate ansi 64K buffer\n");
          return 0;
        }

        /* Read from file (assume OEM codepage) */
        res = ReadFile(hIn, output_bufA, maxChars, &numRead, unused);

        /* Convert from OEM */
        *charsRead = MultiByteToWideChar(GetConsoleCP(), 0, output_bufA, numRead,
                         intoBuf, maxChars);

    }
    return res;
}

/***************************************************************************
 * WCMD_DumpCommands
 *
 *	Domps out the parsed command line to ensure syntax is correct
 */
void WCMD_DumpCommands(CMD_LIST *commands) {
    WCHAR buffer[MAXSTRING];
    CMD_LIST *thisCmd = commands;
    const WCHAR fmt[] = {'%','p',' ','%','c',' ','%','2','.','2','d',' ',
                         '%','p',' ','%','s','\0'};

    WINE_TRACE("Parsed line:\n");
    while (thisCmd != NULL) {
      sprintfW(buffer, fmt,
               thisCmd,
               thisCmd->isAmphersand?'Y':'N',
               thisCmd->bracketDepth,
               thisCmd->nextcommand,
               thisCmd->command);
      WINE_TRACE("%s\n", wine_dbgstr_w(buffer));
      thisCmd = thisCmd->nextcommand;
    }
}

/***************************************************************************
 * WCMD_ReadAndParseLine
 *
 *   Either uses supplied input or
 *     Reads a file from the handle, and then...
 *   Parse the text buffer, spliting into separate commands
 *     - unquoted && strings split 2 commands but the 2nd is flagged as
 *            following an &&
 *     - ( as the first character just ups the bracket depth
 *     - unquoted ) when bracket depth > 0 terminates a bracket and
 *            adds a CMD_LIST structure with null command
 *     - Anything else gets put into the command string (including
 *            redirects)
 */
WCHAR *WCMD_ReadAndParseLine(WCHAR *optionalcmd, CMD_LIST **output, HANDLE readFrom) {

    WCHAR    *curPos;
    BOOL      inQuotes = FALSE;
    WCHAR     curString[MAXSTRING];
    int       curLen   = 0;
    int       curDepth = 0;
    CMD_LIST *thisEntry = NULL;
    CMD_LIST *lastEntry = NULL;
    BOOL      isAmphersand = FALSE;
    static WCHAR    *extraSpace = NULL;  /* Deliberately never freed */
    const WCHAR remCmd[] = {'r','e','m',' ','\0'};
    const WCHAR forCmd[] = {'f','o','r',' ','\0'};
    const WCHAR ifCmd[]  = {'i','f',' ','\0'};
    const WCHAR ifElse[] = {'e','l','s','e',' ','\0'};
    BOOL      inRem = FALSE;
    BOOL      inFor = FALSE;
    BOOL      inIn  = FALSE;
    BOOL      inIf  = FALSE;
    BOOL      inElse= FALSE;
    BOOL      onlyWhiteSpace = FALSE;
    BOOL      lastWasWhiteSpace = FALSE;
    BOOL      lastWasDo   = FALSE;
    BOOL      lastWasIn   = FALSE;
    BOOL      lastWasElse = FALSE;

    /* Allocate working space for a command read from keyboard, file etc */
    if (!extraSpace)
      extraSpace = HeapAlloc(GetProcessHeap(), 0, (MAXSTRING+1) * sizeof(WCHAR));

    /* If initial command read in, use that, otherwise get input from handle */
    if (optionalcmd != NULL) {
        strcpyW(extraSpace, optionalcmd);
    } else if (readFrom == INVALID_HANDLE_VALUE) {
        WINE_FIXME("No command nor handle supplied\n");
    } else {
        if (WCMD_fgets(extraSpace, MAXSTRING, readFrom) == NULL) return NULL;
    }
    curPos = extraSpace;

    /* Handle truncated input - issue warning */
    if (strlenW(extraSpace) == MAXSTRING -1) {
        WCMD_output_asis(WCMD_LoadMessage(WCMD_TRUNCATEDLINE));
        WCMD_output_asis(extraSpace);
        WCMD_output_asis(newline);
    }

    /* Start with an empty string */
    curLen = 0;

    /* Parse every character on the line being processed */
    while (*curPos != 0x00) {

      WCHAR thisChar;

      /* Debugging AID:
      WINE_TRACE("Looking at '%c' (len:%d, lws:%d, ows:%d)\n", *curPos, curLen,
                 lastWasWhiteSpace, onlyWhiteSpace);
      */

     /* Certain commands need special handling */
      if (curLen == 0) {
        const WCHAR forDO[]  = {'d','o',' ','\0'};

        /* If command starts with 'rem', ignore any &&, ( etc */
        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
          curPos, 4, remCmd, -1) == 2) {
          inRem = TRUE;

        /* If command starts with 'for', handle ('s mid line after IN or DO */
        } else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
          curPos, 4, forCmd, -1) == 2) {
          inFor = TRUE;

        /* If command starts with 'if' or 'else', handle ('s mid line. We should ensure this
           is only true in the command portion of the IF statement, but this
           should suffice for now
            FIXME: Silly syntax like "if 1(==1( (
                                        echo they equal
                                      )" will be parsed wrong */
        } else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
          curPos, 3, ifCmd, -1) == 2) {
          inIf = TRUE;

        } else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
          curPos, 5, ifElse, -1) == 2) {
          inElse = TRUE;
          lastWasElse = TRUE;
          onlyWhiteSpace = TRUE;
          memcpy(&curString[curLen], curPos, 5*sizeof(WCHAR));
          curLen+=5;
          curPos+=5;
          continue;

        /* In a for loop, the DO command will follow a close bracket followed by
           whitespace, followed by DO, ie closeBracket inserts a NULL entry, curLen
           is then 0, and all whitespace is skipped                                */
        } else if (inFor &&
                   (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
                    curPos, 3, forDO, -1) == 2)) {
          WINE_TRACE("Found DO\n");
          lastWasDo = TRUE;
          onlyWhiteSpace = TRUE;
          memcpy(&curString[curLen], curPos, 3*sizeof(WCHAR));
          curLen+=3;
          curPos+=3;
          continue;
        }
      } else {

        /* Special handling for the 'FOR' command */
        if (inFor && lastWasWhiteSpace) {
          const WCHAR forIN[] = {'i','n',' ','\0'};

          WINE_TRACE("Found 'FOR', comparing next parm: '%s'\n", wine_dbgstr_w(curPos));

          if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
              curPos, 3, forIN, -1) == 2) {
            WINE_TRACE("Found IN\n");
            lastWasIn = TRUE;
            onlyWhiteSpace = TRUE;
            memcpy(&curString[curLen], curPos, 3*sizeof(WCHAR));
            curLen+=3;
            curPos+=3;
            continue;
          }
        }
      }

      /* Nothing 'ends' a REM statement and &&, quotes etc are ineffective,
         so just use the default processing ie skip character specific
         matching below                                                    */
      if (!inRem) thisChar = *curPos;
      else        thisChar = 'X';  /* Character with no special processing */

      lastWasWhiteSpace = FALSE; /* Will be reset below */

      switch (thisChar) {

      case '\t':/* drop through - ignore whitespace at the start of a command */
      case ' ': if (curLen > 0)
                  curString[curLen++] = *curPos;

                /* Remember just processed whitespace */
                lastWasWhiteSpace = TRUE;

                break;

      case '"': inQuotes = !inQuotes;
                curString[curLen++] = *curPos;
                break;

      case '(': /* If a '(' is the first non whitespace in a command portion
                   ie start of line or just after &&, then we read until an
                   unquoted ) is found                                       */
                WINE_TRACE("Found '(' conditions: curLen(%d), inQ(%d), onlyWS(%d)"
                           ", for(%d, In:%d, Do:%d)"
                           ", if(%d, else:%d, lwe:%d)\n",
                           curLen, inQuotes,
                           onlyWhiteSpace,
                           inFor, lastWasIn, lastWasDo,
                           inIf, inElse, lastWasElse);

                /* Ignore open brackets inside the for set */
                if (curLen == 0 && !inIn) {
                    WINE_TRACE("@@@4\n");
                  curDepth++;

                /* If in quotes, ignore brackets */
                } else if (inQuotes) {
                    WINE_TRACE("@@@3\n");
                  curString[curLen++] = *curPos;

                /* In a FOR loop, an unquoted '(' may occur straight after
                      IN or DO
                   In an IF statement just handle it regardless as we don't
                      parse the operands
                   In an ELSE statement, only allow it straight away after
                      the ELSE and whitespace
                 */
                } else if (inIf ||
                           (inElse && lastWasElse && onlyWhiteSpace) ||
                           (inFor && (lastWasIn || lastWasDo) && onlyWhiteSpace)) {

                  WINE_TRACE("@@@2\n");
                   /* If entering into an 'IN', set inIn */
                  if (inFor && lastWasIn && onlyWhiteSpace) {
                    WINE_TRACE("Inside an IN\n");
                    inIn = TRUE;
                  }

                  /* Add the current command */
                  thisEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(CMD_LIST));
                  thisEntry->command = HeapAlloc(GetProcessHeap(), 0,
                                                 (curLen+1) * sizeof(WCHAR));
                  memcpy(thisEntry->command, curString, curLen * sizeof(WCHAR));
                  thisEntry->command[curLen] = 0x00;
                  curLen = 0;
                  thisEntry->nextcommand = NULL;
                  thisEntry->isAmphersand = isAmphersand;
                  thisEntry->bracketDepth = curDepth;
                  if (lastEntry) {
                    lastEntry->nextcommand = thisEntry;
                  } else {
                    *output = thisEntry;
                  }
                  lastEntry = thisEntry;

                  curDepth++;
                } else {
                  WINE_TRACE("@@@1\n");
                  curString[curLen++] = *curPos;
                }
                break;

      case '&': if (!inQuotes && *(curPos+1) == '&') {
                  curPos++; /* Skip other & */

                  /* Add an entry to the command list */
                  if (curLen > 0) {
                    thisEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(CMD_LIST));
                    thisEntry->command = HeapAlloc(GetProcessHeap(), 0,
                                                   (curLen+1) * sizeof(WCHAR));
                    memcpy(thisEntry->command, curString, curLen * sizeof(WCHAR));
                    thisEntry->command[curLen] = 0x00;
                    curLen = 0;
                    thisEntry->nextcommand = NULL;
                    thisEntry->isAmphersand = isAmphersand;
                    thisEntry->bracketDepth = curDepth;
                    if (lastEntry) {
                      lastEntry->nextcommand = thisEntry;
                    } else {
                      *output = thisEntry;
                    }
                    lastEntry = thisEntry;
                  }
                  isAmphersand = TRUE;
                } else {
                  curString[curLen++] = *curPos;
                }
                break;

      case ')': if (!inQuotes && curDepth > 0) {

                  /* Add the current command if there is one */
                  if (curLen) {
                    thisEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(CMD_LIST));
                    thisEntry->command = HeapAlloc(GetProcessHeap(), 0,
                                                   (curLen+1) * sizeof(WCHAR));
                    memcpy(thisEntry->command, curString, curLen * sizeof(WCHAR));
                    thisEntry->command[curLen] = 0x00;
                    curLen = 0;
                    thisEntry->nextcommand = NULL;
                    thisEntry->isAmphersand = isAmphersand;
                    thisEntry->bracketDepth = curDepth;
                    if (lastEntry) {
                      lastEntry->nextcommand = thisEntry;
                    } else {
                      *output = thisEntry;
                    }
                    lastEntry = thisEntry;
                  }

                  /* Add an empty entry to the command list */
                  thisEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(CMD_LIST));
                  thisEntry->command = NULL;
                  thisEntry->nextcommand = NULL;
                  thisEntry->isAmphersand = FALSE;
                  thisEntry->bracketDepth = curDepth;
                  curDepth--;
                  if (lastEntry) {
                    lastEntry->nextcommand = thisEntry;
                  } else {
                    *output = thisEntry;
                  }
                  lastEntry = thisEntry;

                  /* Leave inIn if necessary */
                  if (inIn) inIn =  FALSE;
                } else {
                  curString[curLen++] = *curPos;
                }
                break;
      default:
                curString[curLen++] = *curPos;
      }

      curPos++;

      /* At various times we need to know if we have only skipped whitespace,
         so reset this variable and then it will remain true until a non
         whitespace is found                                               */
      if ((thisChar != ' ') && (thisChar != '\n')) onlyWhiteSpace = FALSE;

      /* Flag end of interest in FOR DO and IN parms once something has been processed */
      if (!lastWasWhiteSpace) {
        lastWasIn = lastWasDo = FALSE;
      }

      /* If we have reached the end, add this command into the list */
      if (*curPos == 0x00 && curLen > 0) {

          /* Add an entry to the command list */
          thisEntry = HeapAlloc(GetProcessHeap(), 0, sizeof(CMD_LIST));
          thisEntry->command = HeapAlloc(GetProcessHeap(), 0,
                                         (curLen+1) * sizeof(WCHAR));
          memcpy(thisEntry->command, curString, curLen * sizeof(WCHAR));
          thisEntry->command[curLen] = 0x00;
          curLen = 0;
          thisEntry->nextcommand = NULL;
          thisEntry->isAmphersand = isAmphersand;
          thisEntry->bracketDepth = curDepth;
          if (lastEntry) {
            lastEntry->nextcommand = thisEntry;
          } else {
            *output = thisEntry;
          }
          lastEntry = thisEntry;
      }

      /* If we have reached the end of the string, see if bracketing outstanding */
      if (*curPos == 0x00 && curDepth > 0 && readFrom != INVALID_HANDLE_VALUE) {
        inRem = FALSE;
        isAmphersand = FALSE;
        inQuotes = FALSE;
        memset(extraSpace, 0x00, (MAXSTRING+1) * sizeof(WCHAR));

        /* Read more, skipping any blank lines */
        while (*extraSpace == 0x00) {
          if (!context) WCMD_output_asis( WCMD_LoadMessage(WCMD_MOREPROMPT));
          if (WCMD_fgets(extraSpace, MAXSTRING, readFrom) == NULL) break;
        }
        curPos = extraSpace;
      }
    }

    /* Dump out the parsed output */
    WCMD_DumpCommands(*output);

    return extraSpace;
}

/***************************************************************************
 * WCMD_process_commands
 *
 * Process all the commands read in so far
 */
CMD_LIST *WCMD_process_commands(CMD_LIST *thisCmd, BOOL oneBracket,
                                WCHAR *var, WCHAR *val) {

    int bdepth = -1;

    if (thisCmd && oneBracket) bdepth = thisCmd->bracketDepth;

    /* Loop through the commands, processing them one by one */
    while (thisCmd) {

      CMD_LIST *origCmd = thisCmd;

      /* If processing one bracket only, and we find the end bracket
         entry (or less), return                                    */
      if (oneBracket && !thisCmd->command &&
          bdepth <= thisCmd->bracketDepth) {
        WINE_TRACE("Finished bracket @ %p, next command is %p\n",
                   thisCmd, thisCmd->nextcommand);
        return thisCmd->nextcommand;
      }

      /* Ignore the NULL entries a ')' inserts (Only 'if' cares
         about them and it will be handled in there)
         Also, skip over any batch labels (eg. :fred)          */
      if (thisCmd->command && thisCmd->command[0] != ':') {

        WINE_TRACE("Executing command: '%s'\n", wine_dbgstr_w(thisCmd->command));

        if (strchrW(thisCmd->command,'|') != NULL) {
          WCMD_pipe (&thisCmd, var, val);
        } else {
          WCMD_execute (thisCmd->command, var, val, &thisCmd);
        }
      }

      /* Step on unless the command itself already stepped on */
      if (thisCmd == origCmd) thisCmd = thisCmd->nextcommand;
    }
    return NULL;
}

/***************************************************************************
 * WCMD_free_commands
 *
 * Frees the storage held for a parsed command line
 * - This is not done in the process_commands, as eventually the current
 *   pointer will be modified within the commands, and hence a single free
 *   routine is simpler
 */
void WCMD_free_commands(CMD_LIST *cmds) {

    /* Loop through the commands, freeing them one by one */
    while (cmds) {
      CMD_LIST *thisCmd = cmds;
      cmds = cmds->nextcommand;
      HeapFree(GetProcessHeap(), 0, thisCmd->command);
      HeapFree(GetProcessHeap(), 0, thisCmd);
    }
}
