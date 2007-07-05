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
#include <shellapi.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cmd);

static void WCMD_part_execute(CMD_LIST **commands, WCHAR *firstcmd, WCHAR *variable,
                               WCHAR *value, BOOL isIF, BOOL conditionTRUE);

struct env_stack *saved_environment;
struct env_stack *pushd_directories;

extern HINSTANCE hinst;
extern WCHAR inbuilt[][10];
extern int echo_mode, verify_mode, defaultColor;
extern WCHAR quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
extern BATCH_CONTEXT *context;
extern DWORD errorlevel;

static const WCHAR dotW[]    = {'.','\0'};
static const WCHAR dotdotW[] = {'.','.','\0'};
static const WCHAR slashW[]  = {'\\','\0'};
static const WCHAR starW[]   = {'*','\0'};
static const WCHAR equalW[]  = {'=','\0'};
static const WCHAR fslashW[] = {'/','\0'};
static const WCHAR onW[]  = {'O','N','\0'};
static const WCHAR offW[] = {'O','F','F','\0'};
static const WCHAR parmY[] = {'/','Y','\0'};
static const WCHAR parmNoY[] = {'/','-','Y','\0'};
static const WCHAR nullW[] = {'\0'};

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

  WCMD_output (WCMD_LoadMessage(WCMD_NYI));

}

/****************************************************************************
 * WCMD_copy
 *
 * Copy a file or wildcarded set.
 * FIXME: No wildcard support
 */

void WCMD_copy (void) {

  WIN32_FIND_DATA fd;
  HANDLE hff;
  BOOL force, status;
  WCHAR outpath[MAX_PATH], inpath[MAX_PATH], *infile, copycmd[3];
  DWORD len;
  static const WCHAR copyCmdW[] = {'C','O','P','Y','C','M','D','\0'};

  if (param1[0] == 0x00) {
    WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
    return;
  }

  if ((strchrW(param1,'*') != NULL) && (strchrW(param1,'%') != NULL)) {
    WCMD_output (WCMD_LoadMessage(WCMD_NYI));
    return;
  }

  /* If no destination supplied, assume current directory */
  if (param2[0] == 0x00) {
      strcpyW(param2, dotW);
  }

  GetFullPathName (param2, sizeof(outpath)/sizeof(WCHAR), outpath, NULL);
  if (outpath[strlenW(outpath) - 1] == '\\')
      outpath[strlenW(outpath) - 1] = '\0';
  hff = FindFirstFile (outpath, &fd);
  if (hff != INVALID_HANDLE_VALUE) {
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      GetFullPathName (param1, sizeof(inpath)/sizeof(WCHAR), inpath, &infile);
      strcatW (outpath, slashW);
      strcatW (outpath, infile);
    }
    FindClose (hff);
  }

  /* /-Y has the highest priority, then /Y and finally the COPYCMD env. variable */
  if (strstrW (quals, parmNoY))
    force = FALSE;
  else if (strstrW (quals, parmY))
    force = TRUE;
  else {
    len = GetEnvironmentVariable (copyCmdW, copycmd, sizeof(copycmd)/sizeof(WCHAR));
    force = (len && len < (sizeof(copycmd)/sizeof(WCHAR)) && ! lstrcmpiW (copycmd, parmY));
  }

  if (!force) {
    hff = FindFirstFile (outpath, &fd);
    if (hff != INVALID_HANDLE_VALUE) {
      WCHAR buffer[MAXSTRING];

      FindClose (hff);

      wsprintf(buffer, WCMD_LoadMessage(WCMD_OVERWRITE), outpath);
      force = WCMD_ask_confirm(buffer, FALSE, NULL);
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

static BOOL create_full_path(WCHAR* path)
{
    int len;
    WCHAR *new_path;
    BOOL ret = TRUE;

    new_path = HeapAlloc(GetProcessHeap(),0,(strlenW(path) * sizeof(WCHAR))+1);
    strcpyW(new_path,path);

    while ((len = strlenW(new_path)) && new_path[len - 1] == '\\')
        new_path[len - 1] = 0;

    while (!CreateDirectory(new_path,NULL))
    {
        WCHAR *slash;
        DWORD last_error = GetLastError();
        if (last_error == ERROR_ALREADY_EXISTS)
            break;

        if (last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if (!(slash = strrchrW(new_path,'\\')) && ! (slash = strrchrW(new_path,'/')))
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
        WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
        return;
    }
    if (!create_full_path(param1)) WCMD_print_error ();
}

/****************************************************************************
 * WCMD_delete
 *
 * Delete a file or wildcarded set.
 *
 * Note on /A:
 *  - Testing shows /A is repeatable, eg. /a-r /ar matches all files
 *  - Each set is a pattern, eg /ahr /as-r means
 *         readonly+hidden OR nonreadonly system files
 *  - The '-' applies to a single field, ie /a:-hr means read only
 *         non-hidden files
 */

BOOL WCMD_delete (WCHAR *command, BOOL expectDir) {

    int   argno         = 0;
    int   argsProcessed = 0;
    WCHAR *argN          = command;
    BOOL  foundAny      = FALSE;
    static const WCHAR parmA[] = {'/','A','\0'};
    static const WCHAR parmQ[] = {'/','Q','\0'};
    static const WCHAR parmP[] = {'/','P','\0'};
    static const WCHAR parmS[] = {'/','S','\0'};
    static const WCHAR parmF[] = {'/','F','\0'};

    /* If not recursing, clear error flag */
    if (expectDir) errorlevel = 0;

    /* Loop through all args */
    while (argN) {
      WCHAR *thisArg = WCMD_parameter (command, argno++, &argN);
      WCHAR argCopy[MAX_PATH];

      if (argN && argN[0] != '/') {

        WIN32_FIND_DATA fd;
        HANDLE hff;
        WCHAR fpath[MAX_PATH];
        WCHAR *p;
        BOOL handleParm = TRUE;
        BOOL found = FALSE;
        static const WCHAR anyExt[]= {'.','*','\0'};

        strcpyW(argCopy, thisArg);
        WINE_TRACE("del: Processing arg %s (quals:%s)\n",
                   wine_dbgstr_w(argCopy), wine_dbgstr_w(quals));
        argsProcessed++;

        /* If filename part of parameter is * or *.*, prompt unless
           /Q supplied.                                            */
        if ((strstrW (quals, parmQ) == NULL) && (strstrW (quals, parmP) == NULL)) {

          WCHAR drive[10];
          WCHAR dir[MAX_PATH];
          WCHAR fname[MAX_PATH];
          WCHAR ext[MAX_PATH];

          /* Convert path into actual directory spec */
          GetFullPathName (argCopy, sizeof(fpath)/sizeof(WCHAR), fpath, NULL);
          WCMD_splitpath(fpath, drive, dir, fname, ext);

          /* Only prompt for * and *.*, not *a, a*, *.a* etc */
          if ((strcmpW(fname, starW) == 0) &&
              (*ext == 0x00 || (strcmpW(ext, anyExt) == 0))) {
            BOOL  ok;
            WCHAR  question[MAXSTRING];
            static const WCHAR fmt[] = {'%','s',' ','\0'};

            /* Note: Flag as found, to avoid file not found message */
            found = TRUE;

            /* Ask for confirmation */
            wsprintf(question, fmt, fpath);
            ok = WCMD_ask_confirm(question, TRUE, NULL);

            /* Abort if answer is 'N' */
            if (!ok) continue;
          }
        }

        /* First, try to delete in the current directory */
        hff = FindFirstFile (argCopy, &fd);
        if (hff == INVALID_HANDLE_VALUE) {
          handleParm = FALSE;
        } else {
          found = TRUE;
        }

        /* Support del <dirname> by just deleting all files dirname\* */
        if (handleParm && (strchrW(argCopy,'*') == NULL) && (strchrW(argCopy,'?') == NULL)
		&& (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
          WCHAR modifiedParm[MAX_PATH];
          static const WCHAR slashStar[] = {'\\','*','\0'};

          strcpyW(modifiedParm, argCopy);
          strcatW(modifiedParm, slashStar);
          FindClose(hff);
          found = TRUE;
          WCMD_delete(modifiedParm, FALSE);

        } else if (handleParm) {

          /* Build the filename to delete as <supplied directory>\<findfirst filename> */
          strcpyW (fpath, argCopy);
          do {
            p = strrchrW (fpath, '\\');
            if (p != NULL) {
              *++p = '\0';
              strcatW (fpath, fd.cFileName);
            }
            else strcpyW (fpath, fd.cFileName);
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
              BOOL  ok = TRUE;
              WCHAR *nextA = strstrW (quals, parmA);

              /* Handle attribute matching (/A) */
              if (nextA != NULL) {
                ok = FALSE;
                while (nextA != NULL && !ok) {

                  WCHAR *thisA = (nextA+2);
                  BOOL  stillOK = TRUE;

                  /* Skip optional : */
                  if (*thisA == ':') thisA++;

                  /* Parse each of the /A[:]xxx in turn */
                  while (*thisA && *thisA != '/') {
                    BOOL negate    = FALSE;
                    BOOL attribute = FALSE;

                    /* Match negation of attribute first */
                    if (*thisA == '-') {
                      negate=TRUE;
                      thisA++;
                    }

                    /* Match attribute */
                    switch (*thisA) {
                    case 'R': attribute = (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY);
                              break;
                    case 'H': attribute = (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
                              break;
                    case 'S': attribute = (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM);
                              break;
                    case 'A': attribute = (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE);
                              break;
                    default:
                        WCMD_output (WCMD_LoadMessage(WCMD_SYNTAXERR));
                    }

                    /* Now check result, keeping a running boolean about whether it
                       matches all parsed attribues so far                         */
                    if (attribute && !negate) {
                        stillOK = stillOK;
                    } else if (!attribute && negate) {
                        stillOK = stillOK;
                    } else {
                        stillOK = FALSE;
                    }
                    thisA++;
                  }

                  /* Save the running total as the final result */
                  ok = stillOK;

                  /* Step on to next /A set */
                  nextA = strstrW (nextA+1, parmA);
                }
              }

              /* /P means prompt for each file */
              if (ok && strstrW (quals, parmP) != NULL) {
                WCHAR  question[MAXSTRING];

                /* Ask for confirmation */
                wsprintf(question, WCMD_LoadMessage(WCMD_DELPROMPT), fpath);
                ok = WCMD_ask_confirm(question, FALSE, NULL);
              }

              /* Only proceed if ok to */
              if (ok) {

                /* If file is read only, and /F supplied, delete it */
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY &&
                    strstrW (quals, parmF) != NULL) {
                    SetFileAttributes(fpath, fd.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
                }

                /* Now do the delete */
                if (!DeleteFile (fpath)) WCMD_print_error ();
              }

            }
          } while (FindNextFile(hff, &fd) != 0);
          FindClose (hff);
        }

        /* Now recurse into all subdirectories handling the parameter in the same way */
        if (strstrW (quals, parmS) != NULL) {

          WCHAR thisDir[MAX_PATH];
          int cPos;

          WCHAR drive[10];
          WCHAR dir[MAX_PATH];
          WCHAR fname[MAX_PATH];
          WCHAR ext[MAX_PATH];

          /* Convert path into actual directory spec */
          GetFullPathName (argCopy, sizeof(thisDir)/sizeof(WCHAR), thisDir, NULL);
          WCMD_splitpath(thisDir, drive, dir, fname, ext);

          strcpyW(thisDir, drive);
          strcatW(thisDir, dir);
          cPos = strlenW(thisDir);

          WINE_TRACE("Searching recursively in '%s'\n", wine_dbgstr_w(thisDir));

          /* Append '*' to the directory */
          thisDir[cPos] = '*';
          thisDir[cPos+1] = 0x00;

          hff = FindFirstFile (thisDir, &fd);

          /* Remove residual '*' */
          thisDir[cPos] = 0x00;

          if (hff != INVALID_HANDLE_VALUE) {
            DIRECTORY_STACK *allDirs = NULL;
            DIRECTORY_STACK *lastEntry = NULL;

            do {
              if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                  (strcmpW(fd.cFileName, dotdotW) != 0) &&
                  (strcmpW(fd.cFileName, dotW) != 0)) {

                DIRECTORY_STACK *nextDir;
                WCHAR subParm[MAX_PATH];

                /* Work out search parameter in sub dir */
                strcpyW (subParm, thisDir);
                strcatW (subParm, fd.cFileName);
                strcatW (subParm, slashW);
                strcatW (subParm, fname);
                strcatW (subParm, ext);
                WINE_TRACE("Recursive, Adding to search list '%s'\n", wine_dbgstr_w(subParm));

                /* Allocate memory, add to list */
                nextDir = HeapAlloc(GetProcessHeap(),0,sizeof(DIRECTORY_STACK));
                if (allDirs == NULL) allDirs = nextDir;
                if (lastEntry != NULL) lastEntry->next = nextDir;
                lastEntry = nextDir;
                nextDir->next = NULL;
                nextDir->dirName = HeapAlloc(GetProcessHeap(),0,
                                             (strlenW(subParm)+1) * sizeof(WCHAR));
                strcpyW(nextDir->dirName, subParm);
              }
            } while (FindNextFile(hff, &fd) != 0);
            FindClose (hff);

            /* Go through each subdir doing the delete */
            while (allDirs != NULL) {
              DIRECTORY_STACK *tempDir;

              tempDir = allDirs->next;
              found |= WCMD_delete (allDirs->dirName, FALSE);

              HeapFree(GetProcessHeap(),0,allDirs->dirName);
              HeapFree(GetProcessHeap(),0,allDirs);
              allDirs = tempDir;
            }
          }
        }
        /* Keep running total to see if any found, and if not recursing
           issue error message                                         */
        if (expectDir) {
          if (!found) {
            errorlevel = 1;
            WCMD_output (WCMD_LoadMessage(WCMD_FILENOTFOUND), argCopy);
          }
        }
        foundAny |= found;
      }
    }

    /* Handle no valid args */
    if (argsProcessed == 0) {
      WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
    }

    return foundAny;
}

/****************************************************************************
 * WCMD_echo
 *
 * Echo input to the screen (or not). We don't try to emulate the bugs
 * in DOS (try typing "ECHO ON AGAIN" for an example).
 */

void WCMD_echo (const WCHAR *command) {

  int count;

  if ((command[0] == '.') && (command[1] == 0)) {
    WCMD_output (newline);
    return;
  }
  if (command[0]==' ')
    command++;
  count = strlenW(command);
  if (count == 0) {
    if (echo_mode) WCMD_output (WCMD_LoadMessage(WCMD_ECHOPROMPT), onW);
    else WCMD_output (WCMD_LoadMessage(WCMD_ECHOPROMPT), offW);
    return;
  }
  if (lstrcmpiW(command, onW) == 0) {
    echo_mode = 1;
    return;
  }
  if (lstrcmpiW(command, offW) == 0) {
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
 *
 * On entry: cmdList       contains the syntax up to the set
 *           next cmdList and all in that bracket contain the set data
 *           next cmdlist  contains the DO cmd
 *           following that is either brackets or && entries (as per if)
 *
 * FIXME: We don't exhaustively check syntax. Any command which works in MessDOS
 * will probably work here, but the reverse is not necessarily the case...
 */

void WCMD_for (WCHAR *p, CMD_LIST **cmdList) {

  WIN32_FIND_DATA fd;
  HANDLE hff;
  int i;
  const WCHAR inW[] = {'i', 'n', '\0'};
  const WCHAR doW[] = {'d', 'o', ' ','\0'};
  CMD_LIST *setStart, *thisSet, *cmdStart, *cmdEnd;
  WCHAR variable[4];
  WCHAR *firstCmd;
  int thisDepth;
  BOOL isDirs = FALSE;

  /* Check:
     the first line includes the % variable name as first parm
     we have been provided with more parts to the command
     and there is at least some set data
     and IN as the one after that                                   */
  if (lstrcmpiW (WCMD_parameter (p, 1, NULL), inW)
      || (*cmdList) == NULL
      || (*cmdList)->nextcommand == NULL
      || (param1[0] != '%')
      || (strlenW(param1) > 3)) {
    WCMD_output (WCMD_LoadMessage(WCMD_SYNTAXERR));
    return;
  }

  /* Save away where the set of data starts and the variable */
  strcpyW(variable, param1);
  thisDepth = (*cmdList)->bracketDepth;
  *cmdList = (*cmdList)->nextcommand;
  setStart = (*cmdList);

  /* Skip until the close bracket */
  WINE_TRACE("Searching %p as the set\n", *cmdList);
  while (*cmdList &&
         (*cmdList)->command != NULL &&
         (*cmdList)->bracketDepth > thisDepth) {
    WINE_TRACE("Skipping %p which is part of the set\n", *cmdList);
    *cmdList = (*cmdList)->nextcommand;
  }

  /* Skip the close bracket, if there is one */
  if (*cmdList) *cmdList = (*cmdList)->nextcommand;

  /* Syntax error if missing close bracket, or nothing following it
     and once we have the complete set, we expect a DO              */
  WINE_TRACE("Looking for 'do' in %p\n", *cmdList);
  if ((*cmdList == NULL) ||
      (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
                            (*cmdList)->command, 3, doW, -1) != 2)) {
      WCMD_output (WCMD_LoadMessage(WCMD_SYNTAXERR));
      return;
  }

  /* Save away the starting position for the commands (and offset for the
     first one                                                           */
  cmdStart = *cmdList;
  cmdEnd   = *cmdList;
  firstCmd = (*cmdList)->command + 3; /* Skip 'do ' */

  thisSet = setStart;
  /* Loop through all set entries */
  while (thisSet &&
         thisSet->command != NULL &&
         thisSet->bracketDepth >= thisDepth) {

    /* Loop through all entries on the same line */
    WCHAR *item;

    WINE_TRACE("Processing for set %p\n", thisSet);
    i = 0;
    while (*(item = WCMD_parameter (thisSet->command, i, NULL))) {

      /*
       * If the parameter within the set has a wildcard then search for matching files
       * otherwise do a literal substitution.
       */
      static const WCHAR wildcards[] = {'*','?','\0'};
      CMD_LIST *thisCmdStart = cmdStart;

      WINE_TRACE("Processing for item '%s'\n", wine_dbgstr_w(item));
      if (strpbrkW (item, wildcards)) {
        hff = FindFirstFile (item, &fd);
        if (hff != INVALID_HANDLE_VALUE) {
          do {
            BOOL isDirectory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
            if ((isDirs && isDirectory) ||
                (!isDirs && !isDirectory))
            {
              thisCmdStart = cmdStart;
              WINE_TRACE("Processing FOR filename %s\n", wine_dbgstr_w(fd.cFileName));
              WCMD_part_execute (&thisCmdStart, firstCmd, variable,
                                           fd.cFileName, FALSE, TRUE);
            }

          } while (FindNextFile(hff, &fd) != 0);
          FindClose (hff);
        }
      } else {
        WCMD_part_execute(&thisCmdStart, firstCmd, variable, item, FALSE, TRUE);
      }

      WINE_TRACE("Post-command, cmdEnd = %p\n", cmdEnd);
      cmdEnd = thisCmdStart;
      i++;
    }

    /* Move onto the next set line */
    thisSet = thisSet->nextcommand;
  }

  /* When the loop ends, either something like a GOTO or EXIT /b has terminated
     all processing, OR it should be pointing to the end of && processing OR
     it should be pointing at the NULL end of bracket for the DO. The return
     value needs to be the NEXT command to execute, which it either is, or
     we need to step over the closing bracket                                  */
  *cmdList = cmdEnd;
  if (cmdEnd && cmdEnd->command == NULL) *cmdList = cmdEnd->nextcommand;
}


/*****************************************************************************
 * WCMD_part_execute
 *
 * Execute a command, and any && or bracketed follow on to the command. The
 * first command to be executed may not be at the front of the
 * commands->thiscommand string (eg. it may point after a DO or ELSE
 * Returns TRUE if something like exit or goto has aborted all processing
 */
void WCMD_part_execute(CMD_LIST **cmdList, WCHAR *firstcmd, WCHAR *variable,
                       WCHAR *value, BOOL isIF, BOOL conditionTRUE) {

  CMD_LIST *curPosition = *cmdList;
  int myDepth = (*cmdList)->bracketDepth;

  WINE_TRACE("cmdList(%p), firstCmd(%p), with '%s'='%s', doIt(%d)\n",
             cmdList, wine_dbgstr_w(firstcmd),
             wine_dbgstr_w(variable), wine_dbgstr_w(value),
             conditionTRUE);

  /* Skip leading whitespace between condition and the command */
  while (firstcmd && *firstcmd && (*firstcmd==' ' || *firstcmd=='\t')) firstcmd++;

  /* Process the first command, if there is one */
  if (conditionTRUE && firstcmd && *firstcmd) {
    WCHAR *command = WCMD_strdupW(firstcmd);
    WCMD_execute (firstcmd, variable, value, cmdList);
    free (command);
  }


  /* If it didn't move the position, step to next command */
  if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;

  /* Process any other parts of the command */
  if (*cmdList) {
    BOOL processThese = TRUE;

    if (isIF) processThese = conditionTRUE;

    while (*cmdList) {
      const WCHAR ifElse[] = {'e','l','s','e',' ','\0'};

      /* execute all appropriate commands */
      curPosition = *cmdList;

      WINE_TRACE("Processing cmdList(%p) - &(%d) bd(%d / %d)\n",
                 *cmdList,
                 (*cmdList)->isAmphersand,
                 (*cmdList)->bracketDepth, myDepth);

      /* Execute any appended to the statement with &&'s */
      if ((*cmdList)->isAmphersand) {
        if (processThese) {
          WCMD_execute ((*cmdList)->command, variable, value, cmdList);
        }
        if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;

      /* Execute any appended to the statement with (...) */
      } else if ((*cmdList)->bracketDepth > myDepth) {
        if (processThese) {
          *cmdList = WCMD_process_commands(*cmdList, TRUE, variable, value);
          WINE_TRACE("Back from processing commands, (next = %p)\n", *cmdList);
        }
        if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;

      /* End of the command - does 'ELSE ' follow as the next command? */
      } else {
        if (isIF && CompareString (LOCALE_USER_DEFAULT,
                                   NORM_IGNORECASE | SORT_STRINGSORT,
                           (*cmdList)->command, 5, ifElse, -1) == 2) {

          /* Swap between if and else processing */
          processThese = !processThese;

          /* Process the ELSE part */
          if (processThese) {
            WCHAR *cmd = ((*cmdList)->command) + strlenW(ifElse);

            /* Skip leading whitespace between condition and the command */
            while (*cmd && (*cmd==' ' || *cmd=='\t')) cmd++;
            if (*cmd) {
              WCMD_execute (cmd, variable, value, cmdList);
            }
          }
          if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;
        } else {
          WINE_TRACE("Found end of this IF statement (next = %p)\n", *cmdList);
          break;
        }
      }
    }
  }
  return;
}

/*****************************************************************************
 * WCMD_Execute
 *
 *	Execute a command after substituting variable text for the supplied parameter
 */

void WCMD_execute (WCHAR *orig_cmd, WCHAR *param, WCHAR *subst, CMD_LIST **cmdList) {

  WCHAR *new_cmd, *p, *s, *dup;
  int size;

  if (param) {
    size = (strlenW (orig_cmd) + 1) * sizeof(WCHAR);
    new_cmd = (WCHAR *) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, size);
    dup = s = WCMD_strdupW(orig_cmd);

    while ((p = strstrW (s, param))) {
      *p = '\0';
      size += strlenW (subst) * sizeof(WCHAR);
      new_cmd = (WCHAR *) LocalReAlloc ((HANDLE)new_cmd, size, 0);
      strcatW (new_cmd, s);
      strcatW (new_cmd, subst);
      s = p + strlenW (param);
    }
    strcatW (new_cmd, s);
    WCMD_process_command (new_cmd, cmdList);
    free (dup);
    LocalFree ((HANDLE)new_cmd);
  } else {
    WCMD_process_command (orig_cmd, cmdList);
  }
}


/**************************************************************************
 * WCMD_give_help
 *
 *	Simple on-line help. Help text is stored in the resource file.
 */

void WCMD_give_help (WCHAR *command) {

  int i;

  command = WCMD_strtrim_leading_spaces(command);
  if (strlenW(command) == 0) {
    WCMD_output_asis (WCMD_LoadMessage(WCMD_ALLHELP));
  }
  else {
    for (i=0; i<=WCMD_EXIT; i++) {
      if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
	  param1, -1, inbuilt[i], -1) == 2) {
	WCMD_output_asis (WCMD_LoadMessage(i));
	return;
      }
    }
    WCMD_output (WCMD_LoadMessage(WCMD_NOCMDHELP), param1);
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

void WCMD_goto (CMD_LIST **cmdList) {

  WCHAR string[MAX_PATH];

  /* Do not process any more parts of a processed multipart or multilines command */
  *cmdList = NULL;

  if (param1[0] == 0x00) {
    WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
    return;
  }
  if (context != NULL) {
    WCHAR *paramStart = param1;
    static const WCHAR eofW[] = {':','e','o','f','\0'};

    /* Handle special :EOF label */
    if (lstrcmpiW (eofW, param1) == 0) {
      context -> skip_rest = TRUE;
      return;
    }

    /* Support goto :label as well as goto label */
    if (*paramStart == ':') paramStart++;

    SetFilePointer (context -> h, 0, NULL, FILE_BEGIN);
    while (WCMD_fgets (string, sizeof(string)/sizeof(WCHAR), context -> h)) {
      if ((string[0] == ':') && (lstrcmpiW (&string[1], paramStart) == 0)) return;
    }
    WCMD_output (WCMD_LoadMessage(WCMD_NOTARGET));
  }
  return;
}

/*****************************************************************************
 * WCMD_pushd
 *
 *	Push a directory onto the stack
 */

void WCMD_pushd (WCHAR *command) {
    struct env_stack *curdir;
    WCHAR *thisdir;
    static const WCHAR parmD[] = {'/','D','\0'};

    if (strchrW(command, '/') != NULL) {
      SetLastError(ERROR_INVALID_PARAMETER);
      WCMD_print_error();
      return;
    }

    curdir  = LocalAlloc (LMEM_FIXED, sizeof (struct env_stack));
    thisdir = LocalAlloc (LMEM_FIXED, 1024 * sizeof(WCHAR));
    if( !curdir || !thisdir ) {
      LocalFree(curdir);
      LocalFree(thisdir);
      WINE_ERR ("out of memory\n");
      return;
    }

    /* Change directory using CD code with /D parameter */
    strcpyW(quals, parmD);
    GetCurrentDirectoryW (1024, thisdir);
    errorlevel = 0;
    WCMD_setshow_default(command);
    if (errorlevel) {
      LocalFree(curdir);
      LocalFree(thisdir);
      return;
    } else {
      curdir -> next    = pushd_directories;
      curdir -> strings = thisdir;
      if (pushd_directories == NULL) {
        curdir -> u.stackdepth = 1;
      } else {
        curdir -> u.stackdepth = pushd_directories -> u.stackdepth + 1;
      }
      pushd_directories = curdir;
    }
}


/*****************************************************************************
 * WCMD_popd
 *
 *	Pop a directory from the stack
 */

void WCMD_popd (void) {
    struct env_stack *temp = pushd_directories;

    if (!pushd_directories)
      return;

    /* pop the old environment from the stack, and make it the current dir */
    pushd_directories = temp->next;
    SetCurrentDirectoryW(temp->strings);
    LocalFree (temp->strings);
    LocalFree (temp);
}

/****************************************************************************
 * WCMD_if
 *
 * Batch file conditional.
 *
 * On entry, cmdlist will point to command containing the IF, and optionally
 *   the first command to execute (if brackets not found)
 *   If &&'s were found, this may be followed by a record flagged as isAmpersand
 *   If ('s were found, execute all within that bracket
 *   Command may optionally be followed by an ELSE - need to skip instructions
 *   in the else using the same logic
 *
 * FIXME: Much more syntax checking needed!
 */

void WCMD_if (WCHAR *p, CMD_LIST **cmdList) {

  int negate = 0, test = 0;
  WCHAR condition[MAX_PATH], *command, *s;
  static const WCHAR notW[]    = {'n','o','t','\0'};
  static const WCHAR errlvlW[] = {'e','r','r','o','r','l','e','v','e','l','\0'};
  static const WCHAR existW[]  = {'e','x','i','s','t','\0'};
  static const WCHAR defdW[]   = {'d','e','f','i','n','e','d','\0'};
  static const WCHAR eqeqW[]   = {'=','=','\0'};

  if (!lstrcmpiW (param1, notW)) {
    negate = 1;
    strcpyW (condition, param2);
  }
  else {
    strcpyW (condition, param1);
  }
  if (!lstrcmpiW (condition, errlvlW)) {
    if (errorlevel >= atoiW(WCMD_parameter (p, 1+negate, NULL))) test = 1;
    WCMD_parameter (p, 2+negate, &command);
  }
  else if (!lstrcmpiW (condition, existW)) {
    if (GetFileAttributes(WCMD_parameter (p, 1+negate, NULL)) != INVALID_FILE_ATTRIBUTES) {
        test = 1;
    }
    WCMD_parameter (p, 2+negate, &command);
  }
  else if (!lstrcmpiW (condition, defdW)) {
    if (GetEnvironmentVariable(WCMD_parameter (p, 1+negate, NULL), NULL, 0) > 0) {
        test = 1;
    }
    WCMD_parameter (p, 2+negate, &command);
  }
  else if ((s = strstrW (p, eqeqW))) {
    s += 2;
    if (!lstrcmpiW (condition, WCMD_parameter (s, 0, NULL))) test = 1;
    WCMD_parameter (s, 1, &command);
  }
  else {
    WCMD_output (WCMD_LoadMessage(WCMD_SYNTAXERR));
    return;
  }

  /* Process rest of IF statement which is on the same line
     Note: This may process all or some of the cmdList (eg a GOTO) */
  WCMD_part_execute(cmdList, command, NULL, NULL, TRUE, (test != negate));
}

/****************************************************************************
 * WCMD_move
 *
 * Move a file, directory tree or wildcarded set of files.
 */

void WCMD_move (void) {

  int             status;
  WIN32_FIND_DATA fd;
  HANDLE          hff;
  WCHAR            input[MAX_PATH];
  WCHAR            output[MAX_PATH];
  WCHAR            drive[10];
  WCHAR            dir[MAX_PATH];
  WCHAR            fname[MAX_PATH];
  WCHAR            ext[MAX_PATH];

  if (param1[0] == 0x00) {
    WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
    return;
  }

  /* If no destination supplied, assume current directory */
  if (param2[0] == 0x00) {
      strcpyW(param2, dotW);
  }

  /* If 2nd parm is directory, then use original filename */
  /* Convert partial path to full path */
  GetFullPathName (param1, sizeof(input)/sizeof(WCHAR), input, NULL);
  GetFullPathName (param2, sizeof(output)/sizeof(WCHAR), output, NULL);
  WINE_TRACE("Move from '%s'('%s') to '%s'\n", wine_dbgstr_w(input),
             wine_dbgstr_w(param1), wine_dbgstr_w(output));

  /* Split into components */
  WCMD_splitpath(input, drive, dir, fname, ext);

  hff = FindFirstFile (input, &fd);
  while (hff != INVALID_HANDLE_VALUE) {
    WCHAR  dest[MAX_PATH];
    WCHAR  src[MAX_PATH];
    DWORD attribs;

    WINE_TRACE("Processing file '%s'\n", wine_dbgstr_w(fd.cFileName));

    /* Build src & dest name */
    strcpyW(src, drive);
    strcatW(src, dir);

    /* See if dest is an existing directory */
    attribs = GetFileAttributes(output);
    if (attribs != INVALID_FILE_ATTRIBUTES &&
       (attribs & FILE_ATTRIBUTE_DIRECTORY)) {
      strcpyW(dest, output);
      strcatW(dest, slashW);
      strcatW(dest, fd.cFileName);
    } else {
      strcpyW(dest, output);
    }

    strcatW(src, fd.cFileName);

    WINE_TRACE("Source '%s'\n", wine_dbgstr_w(src));
    WINE_TRACE("Dest   '%s'\n", wine_dbgstr_w(dest));

    /* Check if file is read only, otherwise move it */
    attribs = GetFileAttributes(src);
    if ((attribs != INVALID_FILE_ATTRIBUTES) &&
        (attribs & FILE_ATTRIBUTE_READONLY)) {
      SetLastError(ERROR_ACCESS_DENIED);
      status = 0;
    } else {
      BOOL ok = TRUE;

      /* If destination exists, prompt unless /Y supplied */
      if (GetFileAttributes(dest) != INVALID_FILE_ATTRIBUTES) {
        BOOL force = FALSE;
        WCHAR copycmd[MAXSTRING];
        int len;

        /* /-Y has the highest priority, then /Y and finally the COPYCMD env. variable */
        if (strstrW (quals, parmNoY))
          force = FALSE;
        else if (strstrW (quals, parmY))
          force = TRUE;
        else {
          const WCHAR copyCmdW[] = {'C','O','P','Y','C','M','D','\0'};
          len = GetEnvironmentVariable (copyCmdW, copycmd, sizeof(copycmd)/sizeof(WCHAR));
          force = (len && len < (sizeof(copycmd)/sizeof(WCHAR))
                       && ! lstrcmpiW (copycmd, parmY));
        }

        /* Prompt if overwriting */
        if (!force) {
          WCHAR  question[MAXSTRING];
          WCHAR  yesChar[10];

          strcpyW(yesChar, WCMD_LoadMessage(WCMD_YES));

          /* Ask for confirmation */
          wsprintf(question, WCMD_LoadMessage(WCMD_OVERWRITE), dest);
          ok = WCMD_ask_confirm(question, FALSE, NULL);

          /* So delete the destination prior to the move */
          if (ok) {
            if (!DeleteFile (dest)) {
              WCMD_print_error ();
              errorlevel = 1;
              ok = FALSE;
            }
          }
        }
      }

      if (ok) {
        status = MoveFile (src, dest);
      } else {
        status = 1; /* Anything other than 0 to prevent error msg below */
      }
    }

    if (!status) {
      WCMD_print_error ();
      errorlevel = 1;
    }

    /* Step on to next match */
    if (FindNextFile(hff, &fd) == 0) {
      FindClose(hff);
      hff = INVALID_HANDLE_VALUE;
      break;
    }
  }
}

/****************************************************************************
 * WCMD_pause
 *
 * Wait for keyboard input.
 */

void WCMD_pause (void) {

  DWORD count;
  WCHAR string[32];

  WCMD_output (anykey);
  WCMD_ReadFile (GetStdHandle(STD_INPUT_HANDLE), string,
                 sizeof(string)/sizeof(WCHAR), &count, NULL);
}

/****************************************************************************
 * WCMD_remove_dir
 *
 * Delete a directory.
 */

void WCMD_remove_dir (WCHAR *command) {

  int   argno         = 0;
  int   argsProcessed = 0;
  WCHAR *argN          = command;
  static const WCHAR parmS[] = {'/','S','\0'};
  static const WCHAR parmQ[] = {'/','Q','\0'};

  /* Loop through all args */
  while (argN) {
    WCHAR *thisArg = WCMD_parameter (command, argno++, &argN);
    if (argN && argN[0] != '/') {
      WINE_TRACE("rd: Processing arg %s (quals:%s)\n", wine_dbgstr_w(thisArg),
                 wine_dbgstr_w(quals));
      argsProcessed++;

      /* If subdirectory search not supplied, just try to remove
         and report error if it fails (eg if it contains a file) */
      if (strstrW (quals, parmS) == NULL) {
        if (!RemoveDirectory (thisArg)) WCMD_print_error ();

      /* Otherwise use ShFileOp to recursively remove a directory */
      } else {

        SHFILEOPSTRUCT lpDir;

        /* Ask first */
        if (strstrW (quals, parmQ) == NULL) {
          BOOL  ok;
          WCHAR  question[MAXSTRING];
          static const WCHAR fmt[] = {'%','s',' ','\0'};

          /* Ask for confirmation */
          wsprintf(question, fmt, thisArg);
          ok = WCMD_ask_confirm(question, TRUE, NULL);

          /* Abort if answer is 'N' */
          if (!ok) return;
        }

        /* Do the delete */
        lpDir.hwnd   = NULL;
        lpDir.pTo    = NULL;
        lpDir.pFrom  = thisArg;
        lpDir.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;
        lpDir.wFunc  = FO_DELETE;
        if (SHFileOperation(&lpDir)) WCMD_print_error ();
      }
    }
  }

  /* Handle no valid args */
  if (argsProcessed == 0) {
    WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
    return;
  }

}

/****************************************************************************
 * WCMD_rename
 *
 * Rename a file.
 */

void WCMD_rename (void) {

  int             status;
  HANDLE          hff;
  WIN32_FIND_DATA fd;
  WCHAR            input[MAX_PATH];
  WCHAR           *dotDst = NULL;
  WCHAR            drive[10];
  WCHAR            dir[MAX_PATH];
  WCHAR            fname[MAX_PATH];
  WCHAR            ext[MAX_PATH];
  DWORD           attribs;

  errorlevel = 0;

  /* Must be at least two args */
  if (param1[0] == 0x00 || param2[0] == 0x00) {
    WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
    errorlevel = 1;
    return;
  }

  /* Destination cannot contain a drive letter or directory separator */
  if ((strchrW(param1,':') != NULL) || (strchrW(param1,'\\') != NULL)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      WCMD_print_error();
      errorlevel = 1;
      return;
  }

  /* Convert partial path to full path */
  GetFullPathName (param1, sizeof(input)/sizeof(WCHAR), input, NULL);
  WINE_TRACE("Rename from '%s'('%s') to '%s'\n", wine_dbgstr_w(input),
             wine_dbgstr_w(param1), wine_dbgstr_w(param2));
  dotDst = strchrW(param2, '.');

  /* Split into components */
  WCMD_splitpath(input, drive, dir, fname, ext);

  hff = FindFirstFile (input, &fd);
  while (hff != INVALID_HANDLE_VALUE) {
    WCHAR  dest[MAX_PATH];
    WCHAR  src[MAX_PATH];
    WCHAR *dotSrc = NULL;
    int   dirLen;

    WINE_TRACE("Processing file '%s'\n", wine_dbgstr_w(fd.cFileName));

    /* FIXME: If dest name or extension is *, replace with filename/ext
       part otherwise use supplied name. This supports:
          ren *.fred *.jim
          ren jim.* fred.* etc
       However, windows has a more complex algorithum supporting eg
          ?'s and *'s mid name                                         */
    dotSrc = strchrW(fd.cFileName, '.');

    /* Build src & dest name */
    strcpyW(src, drive);
    strcatW(src, dir);
    strcpyW(dest, src);
    dirLen = strlenW(src);
    strcatW(src, fd.cFileName);

    /* Build name */
    if (param2[0] == '*') {
      strcatW(dest, fd.cFileName);
      if (dotSrc) dest[dirLen + (dotSrc - fd.cFileName)] = 0x00;
    } else {
      strcatW(dest, param2);
      if (dotDst) dest[dirLen + (dotDst - param2)] = 0x00;
    }

    /* Build Extension */
    if (dotDst && (*(dotDst+1)=='*')) {
      if (dotSrc) strcatW(dest, dotSrc);
    } else if (dotDst) {
      if (dotDst) strcatW(dest, dotDst);
    }

    WINE_TRACE("Source '%s'\n", wine_dbgstr_w(src));
    WINE_TRACE("Dest   '%s'\n", wine_dbgstr_w(dest));

    /* Check if file is read only, otherwise move it */
    attribs = GetFileAttributes(src);
    if ((attribs != INVALID_FILE_ATTRIBUTES) &&
        (attribs & FILE_ATTRIBUTE_READONLY)) {
      SetLastError(ERROR_ACCESS_DENIED);
      status = 0;
    } else {
      status = MoveFile (src, dest);
    }

    if (!status) {
      WCMD_print_error ();
      errorlevel = 1;
    }

    /* Step on to next match */
    if (FindNextFile(hff, &fd) == 0) {
      FindClose(hff);
      hff = INVALID_HANDLE_VALUE;
      break;
    }
  }
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
    len += (strlenW(&env[len]) + 1);

  env_copy = LocalAlloc (LMEM_FIXED, (len+1) * sizeof (WCHAR) );
  if (!env_copy)
  {
    WINE_ERR("out of memory\n");
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
void WCMD_setlocal (const WCHAR *s) {
  WCHAR *env;
  struct env_stack *env_copy;
  WCHAR cwd[MAX_PATH];

  /* DISABLEEXTENSIONS ignored */

  env_copy = LocalAlloc (LMEM_FIXED, sizeof (struct env_stack));
  if( !env_copy )
  {
    WINE_ERR ("out of memory\n");
    return;
  }

  env = GetEnvironmentStringsW ();

  env_copy->strings = WCMD_dupenv (env);
  if (env_copy->strings)
  {
    env_copy->next = saved_environment;
    saved_environment = env_copy;

    /* Save the current drive letter */
    GetCurrentDirectory (MAX_PATH, cwd);
    env_copy->u.cwd = cwd[0];
  }
  else
    LocalFree (env_copy);

  FreeEnvironmentStringsW (env);

}

/*****************************************************************************
 * WCMD_endlocal
 *
 *  endlocal pops the environment off a stack
 *  Note: When searching for '=', search from WCHAR position 1, to handle
 *        special internal environment variables =C:, =D: etc
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
    n = strlenW(&old[len]) + 1;
    p = strchrW(&old[len] + 1, '=');
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
    n = strlenW(&env[len]) + 1;
    p = strchrW(&env[len] + 1, '=');
    if (p)
    {
      *p++ = 0;
      SetEnvironmentVariableW (&env[len], p);
    }
    len += n;
  }

  /* Restore current drive letter */
  if (IsCharAlpha(temp->u.cwd)) {
    WCHAR envvar[4];
    WCHAR cwd[MAX_PATH];
    static const WCHAR fmt[] = {'=','%','c',':','\0'};

    wsprintf(envvar, fmt, temp->u.cwd);
    if (GetEnvironmentVariable(envvar, cwd, MAX_PATH)) {
      WINE_TRACE("Resetting cwd to %s\n", wine_dbgstr_w(cwd));
      SetCurrentDirectory(cwd);
    }
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
  WCHAR flags[9] = {' ',' ',' ',' ',' ',' ',' ',' ','\0'};

  if (param1[0] == '-') {
    WCMD_output (WCMD_LoadMessage(WCMD_NYI));
    return;
  }

  if (strlenW(param1) == 0) {
    static const WCHAR slashStarW[]  = {'\\','*','\0'};

    GetCurrentDirectory (sizeof(param1)/sizeof(WCHAR), param1);
    strcatW (param1, slashStarW);
  }

  hff = FindFirstFile (param1, &fd);
  if (hff == INVALID_HANDLE_VALUE) {
    WCMD_output (WCMD_LoadMessage(WCMD_FILENOTFOUND), param1);
  }
  else {
    do {
      if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        static const WCHAR fmt[] = {'%','s',' ',' ',' ','%','s','\n','\0'};
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
        WCMD_output (fmt, flags, fd.cFileName);
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

void WCMD_setshow_default (WCHAR *command) {

  BOOL status;
  WCHAR string[1024];
  WCHAR cwd[1024];
  WCHAR *pos;
  WIN32_FIND_DATA fd;
  HANDLE hff;
  static const WCHAR parmD[] = {'/','D','\0'};

  WINE_TRACE("Request change to directory '%s'\n", wine_dbgstr_w(command));

  /* Skip /D and trailing whitespace if on the front of the command line */
  if (CompareString (LOCALE_USER_DEFAULT,
                     NORM_IGNORECASE | SORT_STRINGSORT,
                     command, 2, parmD, -1) == 2) {
    command += 2;
    while (*command && *command==' ') command++;
  }

  GetCurrentDirectory (sizeof(cwd)/sizeof(WCHAR), cwd);
  if (strlenW(command) == 0) {
    strcatW (cwd, newline);
    WCMD_output (cwd);
  }
  else {
    /* Remove any double quotes, which may be in the
       middle, eg. cd "C:\Program Files"\Microsoft is ok */
    pos = string;
    while (*command) {
      if (*command != '"') *pos++ = *command;
      command++;
    }
    *pos = 0x00;

    /* Search for approprate directory */
    WINE_TRACE("Looking for directory '%s'\n", wine_dbgstr_w(string));
    hff = FindFirstFile (string, &fd);
    while (hff != INVALID_HANDLE_VALUE) {
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        WCHAR fpath[MAX_PATH];
        WCHAR drive[10];
        WCHAR dir[MAX_PATH];
        WCHAR fname[MAX_PATH];
        WCHAR ext[MAX_PATH];
        static const WCHAR fmt[] = {'%','s','%','s','%','s','\0'};

        /* Convert path into actual directory spec */
        GetFullPathName (string, sizeof(fpath)/sizeof(WCHAR), fpath, NULL);
        WCMD_splitpath(fpath, drive, dir, fname, ext);

        /* Rebuild path */
        wsprintf(string, fmt, drive, dir, fd.cFileName);

        FindClose(hff);
        hff = INVALID_HANDLE_VALUE;
        break;
      }

      /* Step on to next match */
      if (FindNextFile(hff, &fd) == 0) {
        FindClose(hff);
        hff = INVALID_HANDLE_VALUE;
        break;
      }
    }

    /* Change to that directory */
    WINE_TRACE("Really changing to directory '%s'\n", wine_dbgstr_w(string));

    status = SetCurrentDirectory (string);
    if (!status) {
      errorlevel = 1;
      WCMD_print_error ();
      return;
    } else {

      /* Restore old directory if drive letter would change, and
           CD x:\directory /D (or pushd c:\directory) not supplied */
      if ((strstrW(quals, parmD) == NULL) &&
          (param1[1] == ':') && (toupper(param1[0]) != toupper(cwd[0]))) {
        SetCurrentDirectory(cwd);
      }
    }

    /* Set special =C: type environment variable, for drive letter of
       change of directory, even if path was restored due to missing
       /D (allows changing drive letter when not resident on that
       drive                                                          */
    if ((string[1] == ':') && IsCharAlpha (string[0])) {
      WCHAR env[4];
      strcpyW(env, equalW);
      memcpy(env+1, string, 2 * sizeof(WCHAR));
      env[3] = 0x00;
      WINE_TRACE("Setting '%s' to '%s'\n", wine_dbgstr_w(env), wine_dbgstr_w(string));
      SetEnvironmentVariable(env, string);
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

  WCHAR curdate[64], buffer[64];
  DWORD count;
  static const WCHAR parmT[] = {'/','T','\0'};

  if (strlenW(param1) == 0) {
    if (GetDateFormat (LOCALE_USER_DEFAULT, 0, NULL, NULL,
		curdate, sizeof(curdate)/sizeof(WCHAR))) {
      WCMD_output (WCMD_LoadMessage(WCMD_CURRENTDATE), curdate);
      if (strstrW (quals, parmT) == NULL) {
        WCMD_output (WCMD_LoadMessage(WCMD_NEWDATE));
        WCMD_ReadFile (GetStdHandle(STD_INPUT_HANDLE),
                       buffer, sizeof(buffer)/sizeof(WCHAR), &count, NULL);
        if (count > 2) {
          WCMD_output (WCMD_LoadMessage(WCMD_NYI));
        }
      }
    }
    else WCMD_print_error ();
  }
  else {
    WCMD_output (WCMD_LoadMessage(WCMD_NYI));
  }
}

/****************************************************************************
 * WCMD_compare
 */
static int WCMD_compare( const void *a, const void *b )
{
    int r;
    const WCHAR * const *str_a = a, * const *str_b = b;
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
 * Optionally only display those who start with a stub
 * returns the count displayed
 */
static int WCMD_setshow_sortenv(const WCHAR *s, const WCHAR *stub)
{
  UINT count=0, len=0, i, displayedcount=0, stublen=0;
  const WCHAR **str;

  if (stub) stublen = strlenW(stub);

  /* count the number of strings, and the total length */
  while ( s[len] ) {
    len += (strlenW(&s[len]) + 1);
    count++;
  }

  /* add the strings to an array */
  str = LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, count * sizeof (WCHAR*) );
  if( !str )
    return 0;
  str[0] = s;
  for( i=1; i<count; i++ )
    str[i] = str[i-1] + strlenW(str[i-1]) + 1;

  /* sort the array */
  qsort( str, count, sizeof (WCHAR*), WCMD_compare );

  /* print it */
  for( i=0; i<count; i++ ) {
    if (!stub || CompareString (LOCALE_USER_DEFAULT,
                                NORM_IGNORECASE | SORT_STRINGSORT,
                                str[i], stublen, stub, -1) == 2) {
      /* Don't display special internal variables */
      if (str[i][0] != '=') {
        WCMD_output_asis(str[i]);
        WCMD_output_asis(newline);
        displayedcount++;
      }
    }
  }

  LocalFree( str );
  return displayedcount;
}

/****************************************************************************
 * WCMD_setshow_env
 *
 * Set/Show the environment variables
 */

void WCMD_setshow_env (WCHAR *s) {

  LPVOID env;
  WCHAR *p;
  int status;
  static const WCHAR parmP[] = {'/','P','\0'};

  errorlevel = 0;
  if (param1[0] == 0x00 && quals[0] == 0x00) {
    env = GetEnvironmentStrings ();
    WCMD_setshow_sortenv( env, NULL );
    return;
  }

  /* See if /P supplied, and if so echo the prompt, and read in a reply */
  if (CompareString (LOCALE_USER_DEFAULT,
                     NORM_IGNORECASE | SORT_STRINGSORT,
                     s, 2, parmP, -1) == 2) {
    WCHAR string[MAXSTRING];
    DWORD count;

    s += 2;
    while (*s && *s==' ') s++;

    /* If no parameter, or no '=' sign, return an error */
    if (!(*s) || ((p = strchrW (s, '=')) == NULL )) {
      WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
      return;
    }

    /* Output the prompt */
    *p++ = '\0';
    if (strlenW(p) != 0) WCMD_output(p);

    /* Read the reply */
    WCMD_ReadFile (GetStdHandle(STD_INPUT_HANDLE), string,
                   sizeof(string)/sizeof(WCHAR), &count, NULL);
    if (count > 1) {
      string[count-1] = '\0'; /* ReadFile output is not null-terminated! */
      if (string[count-2] == '\r') string[count-2] = '\0'; /* Under Windoze we get CRLF! */
      WINE_TRACE("set /p: Setting var '%s' to '%s'\n", wine_dbgstr_w(s),
                 wine_dbgstr_w(string));
      status = SetEnvironmentVariable (s, string);
    }

  } else {
    DWORD gle;
    p = strchrW (s, '=');
    if (p == NULL) {
      env = GetEnvironmentStrings ();
      if (WCMD_setshow_sortenv( env, s ) == 0) {
        WCMD_output (WCMD_LoadMessage(WCMD_MISSINGENV), s);
        errorlevel = 1;
      }
      return;
    }
    *p++ = '\0';

    if (strlenW(p) == 0) p = NULL;
    status = SetEnvironmentVariable (s, p);
    gle = GetLastError();
    if ((!status) & (gle == ERROR_ENVVAR_NOT_FOUND)) {
      errorlevel = 1;
    } else if ((!status)) WCMD_print_error();
  }
}

/****************************************************************************
 * WCMD_setshow_path
 *
 * Set/Show the path environment variable
 */

void WCMD_setshow_path (WCHAR *command) {

  WCHAR string[1024];
  DWORD status;
  static const WCHAR pathW[] = {'P','A','T','H','\0'};
  static const WCHAR pathEqW[] = {'P','A','T','H','=','\0'};

  if (strlenW(param1) == 0) {
    status = GetEnvironmentVariable (pathW, string, sizeof(string)/sizeof(WCHAR));
    if (status != 0) {
      WCMD_output_asis ( pathEqW);
      WCMD_output_asis ( string);
      WCMD_output_asis ( newline);
    }
    else {
      WCMD_output (WCMD_LoadMessage(WCMD_NOPATH));
    }
  }
  else {
    if (*command == '=') command++; /* Skip leading '=' */
    status = SetEnvironmentVariable (pathW, command);
    if (!status) WCMD_print_error();
  }
}

/****************************************************************************
 * WCMD_setshow_prompt
 *
 * Set or show the command prompt.
 */

void WCMD_setshow_prompt (void) {

  WCHAR *s;
  static const WCHAR promptW[] = {'P','R','O','M','P','T','\0'};

  if (strlenW(param1) == 0) {
    SetEnvironmentVariable (promptW, NULL);
  }
  else {
    s = param1;
    while ((*s == '=') || (*s == ' ')) s++;
    if (strlenW(s) == 0) {
      SetEnvironmentVariable (promptW, NULL);
    }
    else SetEnvironmentVariable (promptW, s);
  }
}

/****************************************************************************
 * WCMD_setshow_time
 *
 * Set/Show the system time
 * FIXME: Can't change time yet
 */

void WCMD_setshow_time (void) {

  WCHAR curtime[64], buffer[64];
  DWORD count;
  SYSTEMTIME st;
  static const WCHAR parmT[] = {'/','T','\0'};

  if (strlenW(param1) == 0) {
    GetLocalTime(&st);
    if (GetTimeFormat (LOCALE_USER_DEFAULT, 0, &st, NULL,
		curtime, sizeof(curtime)/sizeof(WCHAR))) {
      WCMD_output (WCMD_LoadMessage(WCMD_CURRENTDATE), curtime);
      if (strstrW (quals, parmT) == NULL) {
        WCMD_output (WCMD_LoadMessage(WCMD_NEWTIME));
        WCMD_ReadFile (GetStdHandle(STD_INPUT_HANDLE), buffer,
                       sizeof(buffer)/sizeof(WCHAR), &count, NULL);
        if (count > 2) {
          WCMD_output (WCMD_LoadMessage(WCMD_NYI));
        }
      }
    }
    else WCMD_print_error ();
  }
  else {
    WCMD_output (WCMD_LoadMessage(WCMD_NYI));
  }
}

/****************************************************************************
 * WCMD_shift
 *
 * Shift batch parameters.
 * Optional /n says where to start shifting (n=0-8)
 */

void WCMD_shift (WCHAR *command) {
  int start;

  if (context != NULL) {
    WCHAR *pos = strchrW(command, '/');
    int   i;

    if (pos == NULL) {
      start = 0;
    } else if (*(pos+1)>='0' && *(pos+1)<='8') {
      start = (*(pos+1) - '0');
    } else {
      SetLastError(ERROR_INVALID_PARAMETER);
      WCMD_print_error();
      return;
    }

    WINE_TRACE("Shifting variables, starting at %d\n", start);
    for (i=start;i<=8;i++) {
      context -> shift_count[i] = context -> shift_count[i+1] + 1;
    }
    context -> shift_count[9] = context -> shift_count[9] + 1;
  }

}

/****************************************************************************
 * WCMD_title
 *
 * Set the console title
 */
void WCMD_title (WCHAR *command) {
  SetConsoleTitle(command);
}

/****************************************************************************
 * WCMD_type
 *
 * Copy a file to standard output.
 */

void WCMD_type (WCHAR *command) {

  int   argno         = 0;
  WCHAR *argN          = command;
  BOOL  writeHeaders  = FALSE;

  if (param1[0] == 0x00) {
    WCMD_output (WCMD_LoadMessage(WCMD_NOARG));
    return;
  }

  if (param2[0] != 0x00) writeHeaders = TRUE;

  /* Loop through all args */
  errorlevel = 0;
  while (argN) {
    WCHAR *thisArg = WCMD_parameter (command, argno++, &argN);

    HANDLE h;
    WCHAR buffer[512];
    DWORD count;

    if (!argN) break;

    WINE_TRACE("type: Processing arg '%s'\n", wine_dbgstr_w(thisArg));
    h = CreateFile (thisArg, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
      WCMD_print_error ();
      WCMD_output (WCMD_LoadMessage(WCMD_READFAIL), thisArg);
      errorlevel = 1;
    } else {
      if (writeHeaders) {
        static const WCHAR fmt[] = {'\n','%','s','\n','\n','\0'};
        WCMD_output(fmt, thisArg);
      }
      while (WCMD_ReadFile (h, buffer, sizeof(buffer)/sizeof(WCHAR), &count, NULL)) {
        if (count == 0) break;	/* ReadFile reports success on EOF! */
        buffer[count] = 0;
        WCMD_output_asis (buffer);
      }
      CloseHandle (h);
    }
  }
}

/****************************************************************************
 * WCMD_more
 *
 * Output either a file or stdin to screen in pages
 */

void WCMD_more (WCHAR *command) {

  int   argno         = 0;
  WCHAR *argN          = command;
  BOOL  useinput      = FALSE;
  WCHAR  moreStr[100];
  WCHAR  moreStrPage[100];
  WCHAR  buffer[512];
  DWORD count;
  static const WCHAR moreStart[] = {'-','-',' ','\0'};
  static const WCHAR moreFmt[]   = {'%','s',' ','-','-','\n','\0'};
  static const WCHAR moreFmt2[]  = {'%','s',' ','(','%','2','.','2','d','%','%',
                                    ')',' ','-','-','\n','\0'};
  static const WCHAR conInW[]    = {'C','O','N','I','N','$','\0'};

  /* Prefix the NLS more with '-- ', then load the text */
  errorlevel = 0;
  strcpyW(moreStr, moreStart);
  LoadString (hinst, WCMD_MORESTR, &moreStr[3],
              (sizeof(moreStr)/sizeof(WCHAR))-3);

  if (param1[0] == 0x00) {

    /* Wine implements pipes via temporary files, and hence stdin is
       effectively reading from the file. This means the prompts for
       more are satistied by the next line from the input (file). To
       avoid this, ensure stdin is to the console                    */
    HANDLE hstdin  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hConIn = CreateFile(conInW, GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, 0);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);

    /* Warning: No easy way of ending the stream (ctrl+z on windows) so
       once you get in this bit unless due to a pipe, its going to end badly...  */
    useinput = TRUE;
    wsprintf(moreStrPage, moreFmt, moreStr);

    WCMD_enter_paged_mode(moreStrPage);
    while (WCMD_ReadFile (hstdin, buffer, (sizeof(buffer)/sizeof(WCHAR))-1, &count, NULL)) {
      if (count == 0) break;	/* ReadFile reports success on EOF! */
      buffer[count] = 0;
      WCMD_output_asis (buffer);
    }
    WCMD_leave_paged_mode();

    /* Restore stdin to what it was */
    SetStdHandle(STD_INPUT_HANDLE, hstdin);
    CloseHandle(hConIn);

    return;
  } else {
    BOOL needsPause = FALSE;

    /* Loop through all args */
    WCMD_enter_paged_mode(moreStrPage);

    while (argN) {
      WCHAR *thisArg = WCMD_parameter (command, argno++, &argN);
      HANDLE h;

      if (!argN) break;

      if (needsPause) {

        /* Wait */
        wsprintf(moreStrPage, moreFmt2, moreStr, 100);
        WCMD_leave_paged_mode();
        WCMD_output_asis(moreStrPage);
        WCMD_ReadFile (GetStdHandle(STD_INPUT_HANDLE), buffer,
                       sizeof(buffer)/sizeof(WCHAR), &count, NULL);
        WCMD_enter_paged_mode(moreStrPage);
      }


      WINE_TRACE("more: Processing arg '%s'\n", wine_dbgstr_w(thisArg));
      h = CreateFile (thisArg, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE) {
        WCMD_print_error ();
        WCMD_output (WCMD_LoadMessage(WCMD_READFAIL), thisArg);
        errorlevel = 1;
      } else {
        ULONG64 curPos  = 0;
        ULONG64 fileLen = 0;
        WIN32_FILE_ATTRIBUTE_DATA   fileInfo;

        /* Get the file size */
        GetFileAttributesEx(thisArg, GetFileExInfoStandard, (void*)&fileInfo);
        fileLen = (((ULONG64)fileInfo.nFileSizeHigh) << 32) + fileInfo.nFileSizeLow;

        needsPause = TRUE;
        while (WCMD_ReadFile (h, buffer, (sizeof(buffer)/sizeof(WCHAR))-1, &count, NULL)) {
          if (count == 0) break;	/* ReadFile reports success on EOF! */
          buffer[count] = 0;
          curPos += count;

          /* Update % count (would be used in WCMD_output_asis as prompt) */
          wsprintf(moreStrPage, moreFmt2, moreStr, (int) min(99, (curPos * 100)/fileLen));

          WCMD_output_asis (buffer);
        }
        CloseHandle (h);
      }
    }

    WCMD_leave_paged_mode();
  }
}

/****************************************************************************
 * WCMD_verify
 *
 * Display verify flag.
 * FIXME: We don't actually do anything with the verify flag other than toggle
 * it...
 */

void WCMD_verify (WCHAR *command) {

  int count;

  count = strlenW(command);
  if (count == 0) {
    if (verify_mode) WCMD_output (WCMD_LoadMessage(WCMD_VERIFYPROMPT), onW);
    else WCMD_output (WCMD_LoadMessage(WCMD_VERIFYPROMPT), offW);
    return;
  }
  if (lstrcmpiW(command, onW) == 0) {
    verify_mode = 1;
    return;
  }
  else if (lstrcmpiW(command, offW) == 0) {
    verify_mode = 0;
    return;
  }
  else WCMD_output (WCMD_LoadMessage(WCMD_VERIFYERR));
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

int WCMD_volume (int mode, WCHAR *path) {

  DWORD count, serial;
  WCHAR string[MAX_PATH], label[MAX_PATH], curdir[MAX_PATH];
  BOOL status;

  if (strlenW(path) == 0) {
    status = GetCurrentDirectory (sizeof(curdir)/sizeof(WCHAR), curdir);
    if (!status) {
      WCMD_print_error ();
      return 0;
    }
    status = GetVolumeInformation (NULL, label, sizeof(label)/sizeof(WCHAR),
                                   &serial, NULL, NULL, NULL, 0);
  }
  else {
    static const WCHAR fmt[] = {'%','s','\\','\0'};
    if ((path[1] != ':') || (strlenW(path) != 2)) {
      WCMD_output (WCMD_LoadMessage(WCMD_SYNTAXERR));
      return 0;
    }
    wsprintf (curdir, fmt, path);
    status = GetVolumeInformation (curdir, label, sizeof(label)/sizeof(WCHAR),
                                   &serial, NULL,
    	NULL, NULL, 0);
  }
  if (!status) {
    WCMD_print_error ();
    return 0;
  }
  WCMD_output (WCMD_LoadMessage(WCMD_VOLUMEDETAIL),
    	curdir[0], label, HIWORD(serial), LOWORD(serial));
  if (mode) {
    WCMD_output (WCMD_LoadMessage(WCMD_VOLUMEPROMPT));
    WCMD_ReadFile (GetStdHandle(STD_INPUT_HANDLE), string,
                   sizeof(string)/sizeof(WCHAR), &count, NULL);
    if (count > 1) {
      string[count-1] = '\0';		/* ReadFile output is not null-terminated! */
      if (string[count-2] == '\r') string[count-2] = '\0'; /* Under Windoze we get CRLF! */
    }
    if (strlenW(path) != 0) {
      if (!SetVolumeLabel (curdir, string)) WCMD_print_error ();
    }
    else {
      if (!SetVolumeLabel (NULL, string)) WCMD_print_error ();
    }
  }
  return 1;
}

/**************************************************************************
 * WCMD_exit
 *
 * Exit either the process, or just this batch program
 *
 */

void WCMD_exit (CMD_LIST **cmdList) {

    static const WCHAR parmB[] = {'/','B','\0'};
    int rc = atoiW(param1); /* Note: atoi of empty parameter is 0 */

    if (context && lstrcmpiW(quals, parmB) == 0) {
        errorlevel = rc;
        context -> skip_rest = TRUE;
        *cmdList = NULL;
    } else {
        ExitProcess(rc);
    }
}

/**************************************************************************
 * WCMD_ask_confirm
 *
 * Issue a message and ask 'Are you sure (Y/N)', waiting on a valid
 * answer.
 *
 * Returns True if Y (or A) answer is selected
 *         If optionAll contains a pointer, ALL is allowed, and if answered
 *                   set to TRUE
 *
 */
BOOL WCMD_ask_confirm (WCHAR *message, BOOL showSureText, BOOL *optionAll) {

    WCHAR  msgbuffer[MAXSTRING];
    WCHAR  Ybuffer[MAXSTRING];
    WCHAR  Nbuffer[MAXSTRING];
    WCHAR  Abuffer[MAXSTRING];
    WCHAR  answer[MAX_PATH] = {'\0'};
    DWORD count = 0;

    /* Load the translated 'Are you sure', plus valid answers */
    LoadString (hinst, WCMD_CONFIRM, msgbuffer, sizeof(msgbuffer)/sizeof(WCHAR));
    LoadString (hinst, WCMD_YES, Ybuffer, sizeof(Ybuffer)/sizeof(WCHAR));
    LoadString (hinst, WCMD_NO,  Nbuffer, sizeof(Nbuffer)/sizeof(WCHAR));
    LoadString (hinst, WCMD_ALL, Abuffer, sizeof(Abuffer)/sizeof(WCHAR));

    /* Loop waiting on a Y or N */
    while (answer[0] != Ybuffer[0] && answer[0] != Nbuffer[0]) {
      static const WCHAR startBkt[] = {' ','(','\0'};
      static const WCHAR endBkt[]   = {')','?','\0'};

      WCMD_output_asis (message);
      if (showSureText) {
        WCMD_output_asis (msgbuffer);
      }
      WCMD_output_asis (startBkt);
      WCMD_output_asis (Ybuffer);
      WCMD_output_asis (fslashW);
      WCMD_output_asis (Nbuffer);
      if (optionAll) {
          WCMD_output_asis (fslashW);
          WCMD_output_asis (Abuffer);
      }
      WCMD_output_asis (endBkt);
      WCMD_ReadFile (GetStdHandle(STD_INPUT_HANDLE), answer,
                     sizeof(answer)/sizeof(WCHAR), &count, NULL);
      answer[0] = toupper(answer[0]);
    }

    /* Return the answer */
    return ((answer[0] == Ybuffer[0]) ||
            (optionAll && (answer[0] == Abuffer[0])));
}

/*****************************************************************************
 * WCMD_assoc
 *
 *	Lists or sets file associations  (assoc = TRUE)
 *      Lists or sets file types         (assoc = FALSE)
 */
void WCMD_assoc (WCHAR *command, BOOL assoc) {

    HKEY    key;
    DWORD   accessOptions = KEY_READ;
    WCHAR   *newValue;
    LONG    rc = ERROR_SUCCESS;
    WCHAR    keyValue[MAXSTRING];
    DWORD   valueLen = MAXSTRING;
    HKEY    readKey;
    static const WCHAR shOpCmdW[] = {'\\','S','h','e','l','l','\\',
                                     'O','p','e','n','\\','C','o','m','m','a','n','d','\0'};

    /* See if parameter includes '=' */
    errorlevel = 0;
    newValue = strchrW(command, '=');
    if (newValue) accessOptions |= KEY_WRITE;

    /* Open a key to HKEY_CLASSES_ROOT for enumerating */
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, nullW, 0,
                     accessOptions, &key) != ERROR_SUCCESS) {
      WINE_FIXME("Unexpected failure opening HKCR key: %d\n", GetLastError());
      return;
    }

    /* If no parameters then list all associations */
    if (*command == 0x00) {
      int index = 0;

      /* Enumerate all the keys */
      while (rc != ERROR_NO_MORE_ITEMS) {
        WCHAR  keyName[MAXSTRING];
        DWORD nameLen;

        /* Find the next value */
        nameLen = MAXSTRING;
        rc = RegEnumKeyEx(key, index++,
                          keyName, &nameLen,
                          NULL, NULL, NULL, NULL);

        if (rc == ERROR_SUCCESS) {

          /* Only interested in extension ones if assoc, or others
             if not assoc                                          */
          if ((keyName[0] == '.' && assoc) ||
              (!(keyName[0] == '.') && (!assoc)))
          {
            WCHAR subkey[MAXSTRING];
            strcpyW(subkey, keyName);
            if (!assoc) strcatW(subkey, shOpCmdW);

            if (RegOpenKeyEx(key, subkey, 0,
                             accessOptions, &readKey) == ERROR_SUCCESS) {

              valueLen = sizeof(keyValue)/sizeof(WCHAR);
              rc = RegQueryValueEx(readKey, NULL, NULL, NULL,
                                   (LPBYTE)keyValue, &valueLen);
              WCMD_output_asis(keyName);
              WCMD_output_asis(equalW);
              /* If no default value found, leave line empty after '=' */
              if (rc == ERROR_SUCCESS) {
                WCMD_output_asis(keyValue);
              }
              WCMD_output_asis(newline);
            }
          }
        }
      }
      RegCloseKey(readKey);

    } else {

      /* Parameter supplied - if no '=' on command line, its a query */
      if (newValue == NULL) {
        WCHAR *space;
        WCHAR subkey[MAXSTRING];

        /* Query terminates the parameter at the first space */
        strcpyW(keyValue, command);
        space = strchrW(keyValue, ' ');
        if (space) *space=0x00;

        /* Set up key name */
        strcpyW(subkey, keyValue);
        if (!assoc) strcatW(subkey, shOpCmdW);

        if (RegOpenKeyEx(key, subkey, 0,
                         accessOptions, &readKey) == ERROR_SUCCESS) {

          rc = RegQueryValueEx(readKey, NULL, NULL, NULL,
                               (LPBYTE)keyValue, &valueLen);
          WCMD_output_asis(command);
          WCMD_output_asis(equalW);
          /* If no default value found, leave line empty after '=' */
          if (rc == ERROR_SUCCESS) WCMD_output_asis(keyValue);
          WCMD_output_asis(newline);
          RegCloseKey(readKey);

        } else {
          WCHAR  msgbuffer[MAXSTRING];
          WCHAR  outbuffer[MAXSTRING];

          /* Load the translated 'File association not found' */
          if (assoc) {
            LoadString (hinst, WCMD_NOASSOC, msgbuffer, sizeof(msgbuffer)/sizeof(WCHAR));
          } else {
            LoadString (hinst, WCMD_NOFTYPE, msgbuffer, sizeof(msgbuffer)/sizeof(WCHAR));
          }
          wsprintf(outbuffer, msgbuffer, keyValue);
          WCMD_output_asis(outbuffer);
          errorlevel = 2;
        }

      /* Not a query - its a set or clear of a value */
      } else {

        WCHAR subkey[MAXSTRING];

        /* Get pointer to new value */
        *newValue = 0x00;
        newValue++;

        /* Set up key name */
        strcpyW(subkey, command);
        if (!assoc) strcatW(subkey, shOpCmdW);

        /* If nothing after '=' then clear value - only valid for ASSOC */
        if (*newValue == 0x00) {

          if (assoc) rc = RegDeleteKey(key, command);
          if (assoc && rc == ERROR_SUCCESS) {
            WINE_TRACE("HKCR Key '%s' deleted\n", wine_dbgstr_w(command));

          } else if (assoc && rc != ERROR_FILE_NOT_FOUND) {
            WCMD_print_error();
            errorlevel = 2;

          } else {
            WCHAR  msgbuffer[MAXSTRING];
            WCHAR  outbuffer[MAXSTRING];

            /* Load the translated 'File association not found' */
            if (assoc) {
              LoadString (hinst, WCMD_NOASSOC, msgbuffer,
                          sizeof(msgbuffer)/sizeof(WCHAR));
            } else {
              LoadString (hinst, WCMD_NOFTYPE, msgbuffer,
                          sizeof(msgbuffer)/sizeof(WCHAR));
            }
            wsprintf(outbuffer, msgbuffer, keyValue);
            WCMD_output_asis(outbuffer);
            errorlevel = 2;
          }

        /* It really is a set value = contents */
        } else {
          rc = RegCreateKeyEx(key, subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                              accessOptions, NULL, &readKey, NULL);
          if (rc == ERROR_SUCCESS) {
            rc = RegSetValueEx(readKey, NULL, 0, REG_SZ,
                                 (LPBYTE)newValue, strlenW(newValue));
            RegCloseKey(readKey);
          }

          if (rc != ERROR_SUCCESS) {
            WCMD_print_error();
            errorlevel = 2;
          } else {
            WCMD_output_asis(command);
            WCMD_output_asis(equalW);
            WCMD_output_asis(newValue);
            WCMD_output_asis(newline);
          }
        }
      }
    }

    /* Clean up */
    RegCloseKey(key);
}

/****************************************************************************
 * WCMD_color
 *
 * Clear the terminal screen.
 */

void WCMD_color (void) {

  /* Emulate by filling the screen from the top left to bottom right with
        spaces, then moving the cursor to the top left afterwards */
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

  if (param1[0] != 0x00 && strlenW(param1) > 2) {
    WCMD_output (WCMD_LoadMessage(WCMD_ARGERR));
    return;
  }

  if (GetConsoleScreenBufferInfo(hStdOut, &consoleInfo))
  {
      COORD topLeft;
      DWORD screenSize;
      DWORD color = 0;

      screenSize = consoleInfo.dwSize.X * (consoleInfo.dwSize.Y + 1);

      topLeft.X = 0;
      topLeft.Y = 0;

      /* Convert the color hex digits */
      if (param1[0] == 0x00) {
        color = defaultColor;
      } else {
        color = strtoulW(param1, NULL, 16);
      }

      /* Fail if fg == bg color */
      if (((color & 0xF0) >> 4) == (color & 0x0F)) {
        errorlevel = 1;
        return;
      }

      /* Set the current screen contents and ensure all future writes
         remain this color                                             */
      FillConsoleOutputAttribute(hStdOut, color, screenSize, topLeft, &screenSize);
      SetConsoleTextAttribute(hStdOut, color);
  }
}
