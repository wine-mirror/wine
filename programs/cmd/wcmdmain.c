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

const char * const inbuilt[] = {"ATTRIB", "CALL", "CD", "CHDIR", "CLS", "COPY", "CTTY",
		"DATE", "DEL", "DIR", "ECHO", "ERASE", "FOR", "GOTO",
		"HELP", "IF", "LABEL", "MD", "MKDIR", "MOVE", "PATH", "PAUSE",
		"PROMPT", "REM", "REN", "RENAME", "RD", "RMDIR", "SET", "SHIFT",
                "TIME", "TITLE", "TYPE", "VERIFY", "VER", "VOL",
                "ENDLOCAL", "SETLOCAL", "PUSHD", "POPD", "ASSOC", "COLOR", "FTYPE",
                "EXIT" };

HINSTANCE hinst;
DWORD errorlevel;
int echo_mode = 1, verify_mode = 0, defaultColor = 7;
static int opt_c, opt_k, opt_s;
const char nyi[] = "Not Yet Implemented\n\n";
const char newline[] = "\n";
const char version_string[] = "CMD Version " PACKAGE_VERSION "\n\n";
const char anykey[] = "Press Return key to continue: ";
char quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
BATCH_CONTEXT *context = NULL;
extern struct env_stack *pushd_directories;

static char *WCMD_expand_envvar(char *start);

/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */

int main (int argc, char *argv[])
{
  char string[1024];
  char envvar[4];
  char* cmd=NULL;
  DWORD count;
  HANDLE h;
  int opt_q;
  int opt_t = 0;

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
      } else if (tolower(c)=='t' && (*argv)[2]==':') {
          opt_t=strtoul(&(*argv)[3], NULL, 16);
      } else if (tolower(c)=='x' || tolower(c)=='y') {
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

      if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Command Processor",
                       0, KEY_READ, &key) == ERROR_SUCCESS) {
          char  strvalue[4];

          /* See if DWORD or REG_SZ */
          if (RegQueryValueEx(key, "DefaultColor", NULL, &type,
                     NULL, NULL) == ERROR_SUCCESS) {
              if (type == REG_DWORD) {
                  size = sizeof(DWORD);
                  RegQueryValueEx(key, "DefaultColor", NULL, NULL,
                                  (LPBYTE)&value, &size);
              } else if (type == REG_SZ) {
                  size = sizeof(strvalue);
                  RegQueryValueEx(key, "DefaultColor", NULL, NULL,
                                  (LPBYTE)strvalue, &size);
                  value = strtoul(strvalue, NULL, 10);
              }
          }
      }

      if (value == 0 && RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       "Software\\Microsoft\\Command Processor",
                       0, KEY_READ, &key) == ERROR_SUCCESS) {
          char  strvalue[4];

          /* See if DWORD or REG_SZ */
          if (RegQueryValueEx(key, "DefaultColor", NULL, &type,
                     NULL, NULL) == ERROR_SUCCESS) {
              if (type == REG_DWORD) {
                  size = sizeof(DWORD);
                  RegQueryValueEx(key, "DefaultColor", NULL, NULL,
                                  (LPBYTE)&value, &size);
              } else if (type == REG_SZ) {
                  size = sizeof(strvalue);
                  RegQueryValueEx(key, "DefaultColor", NULL, NULL,
                                  (LPBYTE)strvalue, &size);
                  value = strtoul(strvalue, NULL, 10);
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
    sprintf(envvar, "=%c:", string[0]);
    SetEnvironmentVariable(envvar, string);
  }

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
    WCMD_batch ((char *)"\\autoexec.bat", (char *)"\\autoexec.bat", 0, NULL, INVALID_HANDLE_VALUE);
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
    char *cmd, *p, *s, *t, *redir;
    int status, i;
    DWORD count, creationDisposition;
    HANDLE h;
    char *whichcmd;
    SECURITY_ATTRIBUTES sa;
    char *new_cmd;
    char *first_redir = NULL;
    HANDLE old_stdhandles[3] = {INVALID_HANDLE_VALUE,
                                INVALID_HANDLE_VALUE,
                                INVALID_HANDLE_VALUE};
    DWORD  idx_stdhandles[3] = {STD_INPUT_HANDLE,
                                STD_OUTPUT_HANDLE,
                                STD_ERROR_HANDLE};

    /* Move copy of the command onto the heap so it can be expanded */
    new_cmd = HeapAlloc( GetProcessHeap(), 0, MAXSTRING );
    strcpy(new_cmd, command);

    /* For commands in a context (batch program):                  */
    /*   Expand environment variables in a batch file %{0-9} first */
    /*     including support for any ~ modifiers                   */
    /* Additionally:                                               */
    /*   Expand the DATE, TIME, CD, RANDOM and ERRORLEVEL special  */
    /*     names allowing environment variable overrides           */
    /* NOTE: To support the %PATH:xxx% syntax, also perform        */
    /*   manual expansion of environment variables here            */

    p = new_cmd;
    while ((p = strchr(p, '%'))) {
      i = *(p+1) - '0';

      /* Replace %~ modifications if in batch program */
      if (context && *(p+1) == '~') {
        WCMD_HandleTildaModifiers(&p, NULL);
        p++;

      /* Replace use of %0...%9 if in batch program*/
      } else if (context && (i >= 0) && (i <= 9)) {
        s = strdup (p+2);
        t = WCMD_parameter (context -> command, i + context -> shift_count[i], NULL);
        strcpy (p, t);
        strcat (p, s);
        free (s);

      /* Replace use of %* if in batch program*/
      } else if (context && *(p+1)=='*') {
        char *startOfParms = NULL;
        s = strdup (p+2);
        t = WCMD_parameter (context -> command, 1, &startOfParms);
        if (startOfParms != NULL) strcpy (p, startOfParms);
        else *p = 0x00;
        strcat (p, s);
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
      while ((p = strchr(p, '%'))) {
        s = strchr(p+1, '%');
        if (!s) {
          *p=0x00;
        } else {
          t = strdup(s+1);
          strcpy(p, t);
          free(t);
        }
      }

      /* Show prompt before batch line IF echo is on and in batch program */
      if (echo_mode && (cmd[0] != '@')) {
        WCMD_show_prompt();
        WCMD_output_asis ( cmd);
        WCMD_output_asis ( "\n");
      }
    }

/*
 *	Changing default drive has to be handled as a special case.
 */

    if ((cmd[1] == ':') && IsCharAlpha (cmd[0]) && (strlen(cmd) == 2)) {
      char envvar[5];
      char dir[MAX_PATH];

      /* According to MSDN CreateProcess docs, special env vars record
         the current directory on each drive, in the form =C:
         so see if one specified, and if so go back to it             */
      strcpy(envvar, "=");
      strcat(envvar, cmd);
      if (GetEnvironmentVariable(envvar, dir, MAX_PATH) == 0) {
        sprintf(cmd, "%s\\", cmd);
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

    if ((p = strchr(cmd,'<')) != NULL) {
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
    while (redir != NULL && ((p = strchr(redir,'>')) != NULL)) {
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
        char *param = WCMD_parameter (p, 0, NULL);
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
        WINE_TRACE("Redirect %d to '%s' (%p)\n", handle, param, h);
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
    WINE_TRACE("Command: '%s'\n", cmd);
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
        WCMD_delete (p);
        break;
      case WCMD_DIR:
        WCMD_directory (p);
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
        if (strlen(&whichcmd[count]) > 0)
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
      case WCMD_EXIT:
        WCMD_exit ();
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

void WCMD_run_program (char *command, int called) {

  char  temp[MAX_PATH];
  char  pathtosearch[MAXSTRING];
  char *pathposn;
  char  stemofsearch[MAX_PATH];
  char *lastSlash;
  char  pathext[MAXSTRING];
  BOOL  extensionsupplied = FALSE;
  BOOL  launched = FALSE;
  BOOL  status;
  BOOL  assumeInternal = FALSE;
  DWORD len;


  WCMD_parse (command, quals, param1, param2);	/* Quick way to get the filename */
  if (!(*param1) && !(*param2))
    return;

  /* Calculate the search path and stem to search for */
  if (strpbrk (param1, "/\\:") == NULL) {  /* No explicit path given, search path */
    strcpy(pathtosearch,".;");
    len = GetEnvironmentVariable ("PATH", &pathtosearch[2], sizeof(pathtosearch)-2);
    if ((len == 0) || (len >= sizeof(pathtosearch) - 2)) {
      lstrcpy (pathtosearch, ".");
    }
    if (strchr(param1, '.') != NULL) extensionsupplied = TRUE;
    strcpy(stemofsearch, param1);

  } else {

    /* Convert eg. ..\fred to include a directory by removing file part */
    GetFullPathName(param1, sizeof(pathtosearch), pathtosearch, NULL);
    lastSlash = strrchr(pathtosearch, '\\');
    if (lastSlash) *lastSlash = 0x00;
    if (strchr(lastSlash, '.') != NULL) extensionsupplied = TRUE;
    strcpy(stemofsearch, lastSlash+1);
  }

  /* Now extract PATHEXT */
  len = GetEnvironmentVariable ("PATHEXT", pathext, sizeof(pathext));
  if ((len == 0) || (len >= sizeof(pathext))) {
    lstrcpy (pathext, ".bat;.com;.cmd;.exe");
  }

  /* Loop through the search path, dir by dir */
  pathposn = pathtosearch;
  WINE_TRACE("Searching in '%s' for '%s'\n", pathtosearch, stemofsearch);
  while (!launched && pathposn) {

    char  thisDir[MAX_PATH] = "";
    char *pos               = NULL;
    BOOL  found             = FALSE;

    /* Work on the first directory on the search path */
    pos = strchr(pathposn, ';');
    if (pos) {
      strncpy(thisDir, pathposn, (pos-pathposn));
      thisDir[(pos-pathposn)] = 0x00;
      pathposn = pos+1;

    } else {
      strcpy(thisDir, pathposn);
      pathposn = NULL;
    }

    /* Since you can have eg. ..\.. on the path, need to expand
       to full information                                      */
    strcpy(temp, thisDir);
    GetFullPathName(temp, MAX_PATH, thisDir, NULL);

    /* 1. If extension supplied, see if that file exists */
    strcat(thisDir, "\\");
    strcat(thisDir, stemofsearch);
    pos = &thisDir[strlen(thisDir)]; /* Pos = end of name */

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

      strcat(thisDir,".*");
      h = FindFirstFile(thisDir, &finddata);
      FindClose(h);
      if (h != INVALID_HANDLE_VALUE) {

        char *thisExt = pathext;

        /* 3. Yes - Try each path ext */
        while (thisExt) {
          char *nextExt = strchr(thisExt, ';');

          if (nextExt) {
            strncpy(pos, thisExt, (nextExt-thisExt));
            pos[(nextExt-thisExt)] = 0x00;
            thisExt = nextExt+1;
          } else {
            strcpy(pos, thisExt);
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
        WINE_TRACE("Found as %s\n", thisDir);
    }

    /* Once found, launch it */
    if (found || assumeInternal) {
      STARTUPINFO st;
      PROCESS_INFORMATION pe;
      SHFILEINFO psfi;
      DWORD console;
      HINSTANCE hinst;
      char *ext = strrchr( thisDir, '.' );
      launched = TRUE;

      /* Special case BAT and CMD */
      if (ext && !strcasecmp(ext, ".bat")) {
        WCMD_batch (thisDir, command, called, NULL, INVALID_HANDLE_VALUE);
        return;
      } else if (ext && !strcasecmp(ext, ".cmd")) {
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
  char out_string[MAX_PATH], curdir[MAX_PATH], prompt_string[MAX_PATH];
  char *p, *q;
  DWORD len;

  len = GetEnvironmentVariable ("PROMPT", prompt_string, sizeof(prompt_string));
  if ((len == 0) || (len >= sizeof(prompt_string))) {
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
	case 'S':
	  *q++ = ' ';
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

/*************************************************************************
 * WCMD_expand_envvar
 *
 *	Expands environment variables, allowing for character substitution
 */
static char *WCMD_expand_envvar(char *start) {
    char *endOfVar = NULL, *s;
    char *colonpos = NULL;
    char thisVar[MAXSTRING];
    char thisVarContents[MAXSTRING];
    char savedchar = 0x00;
    int len;

    /* Find the end of the environment variable, and extract name */
    endOfVar = strchr(start+1, '%');
    if (endOfVar == NULL) {
      /* FIXME: Some special conditions here depending opn whether
         in batch, complex or not, and whether env var exists or not! */
      return start+1;
    }
    strncpy(thisVar, start, (endOfVar - start)+1);
    thisVar[(endOfVar - start)+1] = 0x00;
    colonpos = strchr(thisVar+1, ':');

    /* If there's complex substitution, just need %var% for now
       to get the expanded data to play with                    */
    if (colonpos) {
        *colonpos = '%';
        savedchar = *(colonpos+1);
        *(colonpos+1) = 0x00;
    }

    /* Expand to contents, if unchanged, return */
    /* Handle DATE, TIME, ERRORLEVEL and CD replacements allowing */
    /* override if existing env var called that name              */
    if ((CompareString (LOCALE_USER_DEFAULT,
                        NORM_IGNORECASE | SORT_STRINGSORT,
                        thisVar, 12, "%ERRORLEVEL%", -1) == 2) &&
                (GetEnvironmentVariable("ERRORLEVEL", thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {
      sprintf(thisVarContents, "%d", errorlevel);
      len = strlen(thisVarContents);

    } else if ((CompareString (LOCALE_USER_DEFAULT,
                               NORM_IGNORECASE | SORT_STRINGSORT,
                               thisVar, 6, "%DATE%", -1) == 2) &&
                (GetEnvironmentVariable("DATE", thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {

      GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL,
                    NULL, thisVarContents, MAXSTRING);
      len = strlen(thisVarContents);

    } else if ((CompareString (LOCALE_USER_DEFAULT,
                               NORM_IGNORECASE | SORT_STRINGSORT,
                               thisVar, 6, "%TIME%", -1) == 2) &&
                (GetEnvironmentVariable("TIME", thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {
      GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL,
                        NULL, thisVarContents, MAXSTRING);
      len = strlen(thisVarContents);

    } else if ((CompareString (LOCALE_USER_DEFAULT,
                               NORM_IGNORECASE | SORT_STRINGSORT,
                               thisVar, 4, "%CD%", -1) == 2) &&
                (GetEnvironmentVariable("CD", thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {
      GetCurrentDirectory (MAXSTRING, thisVarContents);
      len = strlen(thisVarContents);

    } else if ((CompareString (LOCALE_USER_DEFAULT,
                               NORM_IGNORECASE | SORT_STRINGSORT,
                               thisVar, 8, "%RANDOM%", -1) == 2) &&
                (GetEnvironmentVariable("RANDOM", thisVarContents, 1) == 0) &&
                (GetLastError() == ERROR_ENVVAR_NOT_FOUND)) {
      sprintf(thisVarContents, "%d", rand() % 32768);
      len = strlen(thisVarContents);

    } else {

      len = ExpandEnvironmentStrings(thisVar, thisVarContents,
                                      sizeof(thisVarContents));
    }

    if (len == 0)
      return endOfVar+1;

    /* In a batch program, unknown env vars are replaced with nothing,
         note syntax %garbage:1,3% results in anything after the ':'
         except the %
       From the command line, you just get back what you entered      */
    if (lstrcmpi(thisVar, thisVarContents) == 0) {

      /* Restore the complex part after the compare */
      if (colonpos) {
        *colonpos = ':';
        *(colonpos+1) = savedchar;
      }

      s = strdup (endOfVar + 1);

      /* Command line - just ignore this */
      if (context == NULL) return endOfVar+1;

      /* Batch - replace unknown env var with nothing */
      if (colonpos == NULL) {
        strcpy (start, s);

      } else {
        len = strlen(thisVar);
        thisVar[len-1] = 0x00;
        /* If %:...% supplied, : is retained */
        if (colonpos == thisVar+1) {
          strcpy (start, colonpos);
        } else {
          strcpy (start, colonpos+1);
        }
        strcat (start, s);
      }
      free (s);
      return start;

    }

    /* See if we need to do complex substitution (any ':'s), if not
       then our work here is done                                  */
    if (colonpos == NULL) {
      s = strdup (endOfVar + 1);
      strcpy (start, thisVarContents);
      strcat (start, s);
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
           ~x         (from x chars in)
           ~-x        (from x chars from the end)
           ~x,y       (from x chars in for y characters)
           ~x,-y      (from x chars in until y characters from the end)
     */

    /* ~ is substring manipulation */
    if (savedchar == '~') {

      int   substrposition, substrlength = 0;
      char *commapos = strchr(colonpos+2, ',');
      char *startCopy;

      substrposition = atol(colonpos+2);
      if (commapos) substrlength = atol(commapos+1);

      s = strdup (endOfVar + 1);

      /* Check bounds */
      if (substrposition >= 0) {
        startCopy = &thisVarContents[min(substrposition, len)];
      } else {
        startCopy = &thisVarContents[max(0, len+substrposition-1)];
      }

      if (commapos == NULL) {
        strcpy (start, startCopy); /* Copy the lot */
      } else if (substrlength < 0) {

        int copybytes = (len+substrlength-1)-(startCopy-thisVarContents);
        if (copybytes > len) copybytes = len;
        else if (copybytes < 0) copybytes = 0;
        strncpy (start, startCopy, copybytes); /* Copy the lot */
        start[copybytes] = 0x00;
      } else {
        strncpy (start, startCopy, substrlength); /* Copy the lot */
        start[substrlength] = 0x00;
      }

      strcat (start, s);
      free(s);
      return start;

    /* search and replace manipulation */
    } else {
      char *equalspos = strstr(colonpos, "=");
      char *replacewith = equalspos+1;
      char *found       = NULL;
      char *searchIn;
      char *searchFor;

      s = strdup (endOfVar + 1);
      if (equalspos == NULL) return start+1;

      /* Null terminate both strings */
      thisVar[strlen(thisVar)-1] = 0x00;
      *equalspos = 0x00;

      /* Since we need to be case insensitive, copy the 2 buffers */
      searchIn  = strdup(thisVarContents);
      CharUpperBuff(searchIn, strlen(thisVarContents));
      searchFor = strdup(colonpos+1);
      CharUpperBuff(searchFor, strlen(colonpos+1));


      /* Handle wildcard case */
      if (*(colonpos+1) == '*') {
        /* Search for string to replace */
        found = strstr(searchIn, searchFor+1);

        if (found) {
          /* Do replacement */
          strcpy(start, replacewith);
          strcat(start, thisVarContents + (found-searchIn) + strlen(searchFor+1));
          strcat(start, s);
          free(s);
        } else {
          /* Copy as it */
          strcpy(start, thisVarContents);
          strcat(start, s);
        }

      } else {
        /* Loop replacing all instances */
        char *lastFound = searchIn;
        char *outputposn = start;

        *start = 0x00;
        while ((found = strstr(lastFound, searchFor))) {
            strncpy(outputposn,
                    thisVarContents + (lastFound-searchIn),
                    (found - lastFound));
            outputposn  = outputposn + (found - lastFound);
            *outputposn = 0x00;
            strcat(outputposn, replacewith);
            outputposn = outputposn + strlen(replacewith);
            lastFound = found + strlen(searchFor);
        }
        strcat(outputposn,
                thisVarContents + (lastFound-searchIn));
        strcat(outputposn, s);
      }
      free(searchIn);
      free(searchFor);
      return start;
    }
    return start+1;
}
