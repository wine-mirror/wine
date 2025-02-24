/*
 * CMD - Wine-compatible command line interface - built-in functions.
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

extern int defaultColor;
extern BOOL echo_mode;
extern BOOL interactive;

struct env_stack *pushd_directories;
const WCHAR inbuilt[][10] = {
        L"CALL",
        L"CD",
        L"CHDIR",
        L"CLS",
        L"COPY",
        L"",
        L"DATE",
        L"DEL",
        L"DIR",
        L"ECHO",
        L"ERASE",
        L"FOR",
        L"GOTO",
        L"HELP",
        L"IF",
        L"LABEL",
        L"MD",
        L"MKDIR",
        L"MOVE",
        L"PATH",
        L"PAUSE",
        L"PROMPT",
        L"REM",
        L"REN",
        L"RENAME",
        L"RD",
        L"RMDIR",
        L"SET",
        L"SHIFT",
        L"START",
        L"TIME",
        L"TITLE",
        L"TYPE",
        L"VERIFY",
        L"VER",
        L"VOL",
        L"ENDLOCAL",
        L"SETLOCAL",
        L"PUSHD",
        L"POPD",
        L"ASSOC",
        L"COLOR",
        L"FTYPE",
        L"MORE",
        L"CHOICE",
        L"MKLINK",
        L"",
        L"EXIT"
};
static const WCHAR externals[][10] = {
        L"ATTRIB",
        L"XCOPY"
};

static HINSTANCE hinst;
static struct env_stack *saved_environment;
static BOOL verify_mode = FALSE;

/* set /a routines work from single character operators, but some of the
   operators are multiple character ones, especially the assignment ones.
   Temporarily represent these using the values below on the operator stack */
#define OP_POSITIVE     'P'
#define OP_NEGATIVE     'N'
#define OP_ASSSIGNMUL   'a'
#define OP_ASSSIGNDIV   'b'
#define OP_ASSSIGNMOD   'c'
#define OP_ASSSIGNADD   'd'
#define OP_ASSSIGNSUB   'e'
#define OP_ASSSIGNAND   'f'
#define OP_ASSSIGNNOT   'g'
#define OP_ASSSIGNOR    'h'
#define OP_ASSSIGNSHL   'i'
#define OP_ASSSIGNSHR   'j'

/* This maintains a stack of operators, holding both the operator precedence
   and the single character representation of the operator in question       */
typedef struct _OPSTACK
{
  int              precedence;
  WCHAR            op;
  struct _OPSTACK *next;
} OPSTACK;

/* This maintains a stack of values, where each value can either be a
   numeric value, or a string representing an environment variable     */
typedef struct _VARSTACK
{
  BOOL              isnum;
  WCHAR            *variable;
  int               value;
  struct _VARSTACK *next;
} VARSTACK;

/* This maintains a mapping between the calculated operator and the
   single character representation for the assignment operators.    */
static struct
{
  WCHAR op;
  WCHAR calculatedop;
} calcassignments[] =
{
  {'*', OP_ASSSIGNMUL},
  {'/', OP_ASSSIGNDIV},
  {'%', OP_ASSSIGNMOD},
  {'+', OP_ASSSIGNADD},
  {'-', OP_ASSSIGNSUB},
  {'&', OP_ASSSIGNAND},
  {'^', OP_ASSSIGNNOT},
  {'|', OP_ASSSIGNOR},
  {'<', OP_ASSSIGNSHL},
  {'>', OP_ASSSIGNSHR},
  {' ',' '}
};

DIRECTORY_STACK *WCMD_dir_stack_create(const WCHAR *dir, const WCHAR *file)
{
    DIRECTORY_STACK *new = xalloc(sizeof(DIRECTORY_STACK));

    new->next = NULL;
    new->fileName = NULL;
    if (!dir && !file)
    {
        DWORD sz = GetCurrentDirectoryW(0, NULL);
        new->dirName = xalloc(sz * sizeof(WCHAR));
        GetCurrentDirectoryW(sz, new->dirName);
    }
    else if (!file)
        new->dirName = xstrdupW(dir);
    else
    {
        new->dirName = xalloc((wcslen(dir) + 1 + wcslen(file) + 1) * sizeof(WCHAR));
        wcscpy(new->dirName, dir);
        wcscat(new->dirName, L"\\");
        wcscat(new->dirName, file);
    }
    return new;
}

DIRECTORY_STACK *WCMD_dir_stack_free(DIRECTORY_STACK *dir)
{
    DIRECTORY_STACK *next;

    if (!dir) return NULL;
    next = dir->next;
    free(dir->dirName);
    free(dir);
    return next;
}

/**************************************************************************
 * WCMD_ask_confirm
 *
 * Issue a message and ask for confirmation, waiting on a valid answer.
 *
 * Returns True if Y (or A) answer is selected
 *         If optionAll contains a pointer, ALL is allowed, and if answered
 *                   set to TRUE
 *
 */
static BOOL WCMD_ask_confirm (const WCHAR *message, BOOL showSureText,
                              BOOL *optionAll) {

    UINT msgid;
    WCHAR confirm[MAXSTRING];
    WCHAR options[MAXSTRING];
    WCHAR Ybuffer[MAXSTRING];
    WCHAR Nbuffer[MAXSTRING];
    WCHAR Abuffer[MAXSTRING];
    WCHAR answer[MAX_PATH] = {'\0'};
    DWORD count = 0;

    /* Load the translated valid answers */
    if (showSureText)
      LoadStringW(hinst, WCMD_CONFIRM, confirm, ARRAY_SIZE(confirm));
    msgid = optionAll ? WCMD_YESNOALL : WCMD_YESNO;
    LoadStringW(hinst, msgid, options, ARRAY_SIZE(options));
    LoadStringW(hinst, WCMD_YES, Ybuffer, ARRAY_SIZE(Ybuffer));
    LoadStringW(hinst, WCMD_NO, Nbuffer, ARRAY_SIZE(Nbuffer));
    LoadStringW(hinst, WCMD_ALL, Abuffer, ARRAY_SIZE(Abuffer));

    /* Loop waiting on a valid answer */
    if (optionAll)
        *optionAll = FALSE;
    while (1)
    {
      WCMD_output_asis (message);
      if (showSureText)
        WCMD_output_asis (confirm);
      WCMD_output_asis (options);
      if (!WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), answer, ARRAY_SIZE(answer), &count) || !count)
          return FALSE;
      answer[0] = towupper(answer[0]);
      if (answer[0] == Ybuffer[0])
        return TRUE;
      if (answer[0] == Nbuffer[0])
        return FALSE;
      if (optionAll && answer[0] == Abuffer[0])
      {
        *optionAll = TRUE;
        return TRUE;
      }
    }
}

/****************************************************************************
 * WCMD_clear_screen
 *
 * Clear the terminal screen.
 */

RETURN_CODE WCMD_clear_screen(void)
{
  /* Emulate by filling the screen from the top left to bottom right with
        spaces, then moving the cursor to the top left afterwards */
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

  if (*quals)
      return errorlevel = ERROR_INVALID_FUNCTION;
  if (GetConsoleScreenBufferInfo(hStdOut, &consoleInfo))
  {
      COORD topLeft;
      DWORD screenSize, written;

      screenSize = consoleInfo.dwSize.X * (consoleInfo.dwSize.Y + 1);

      topLeft.X = 0;
      topLeft.Y = 0;
      FillConsoleOutputCharacterW(hStdOut, ' ', screenSize, topLeft, &written);
      FillConsoleOutputAttribute(hStdOut, consoleInfo.wAttributes, screenSize, topLeft, &written);
      SetConsoleCursorPosition(hStdOut, topLeft);
  }
  return NO_ERROR;
}

/****************************************************************************
 * WCMD_choice
 *
 */

RETURN_CODE WCMD_choice(WCHAR *args)
{
    RETURN_CODE return_code = NO_ERROR;
    WCHAR answer[16];
    WCHAR buffer[16];
    WCHAR *ptr = NULL;
    WCHAR *opt_c = NULL;
    WCHAR *opt_m = NULL;
    WCHAR opt_default = 0;
    DWORD opt_timeout = -1;
    WCHAR *end;
    DWORD oldmode;
    BOOL have_console;
    BOOL opt_n = FALSE;
    BOOL opt_cs = FALSE;
    int argno;

    for (argno = 0; ; argno++)
    {
        WCHAR *arg = WCMD_parameter(args, argno, NULL, FALSE, FALSE);
        if (!*arg) break;

        if (!wcsicmp(arg, L"/N")) opt_n = TRUE;
        else if (!wcsicmp(arg, L"/CS")) opt_cs = TRUE;
        else if (arg[0] == L'/' && wcschr(L"CDTM", towupper(arg[1])))
        {
            WCHAR opt = towupper(arg[1]);
            if (arg[2] == L'\0')
            {
                arg = WCMD_parameter(args, ++argno, NULL, FALSE, FALSE);
                if (!*arg)
                {
                    return_code = ERROR_INVALID_FUNCTION;
                    break;
                }
            }
            else if (arg[2] == L':')
                arg += 3;
            else
            {
                return_code = ERROR_INVALID_FUNCTION;
                break;
            }
            switch (opt)
            {
            case L'C':
                opt_c = wcsdup(arg);
                break;
            case L'M':
                opt_m = wcsdup(arg);
                break;
            case L'D':
                opt_default = *arg;
                break;
            case L'T':
                opt_timeout = wcstol(arg, &end, 10);
                if (end == arg || (*end && !iswspace(*end)))
                    opt_timeout = 10000;
                break;
            }
        }
        else
            return_code = ERROR_INVALID_FUNCTION;
    }

    /* use default keys, when needed: localized versions of "Y"es and "No" */
    if (!opt_c)
    {
        LoadStringW(hinst, WCMD_YES, buffer, ARRAY_SIZE(buffer));
        LoadStringW(hinst, WCMD_NO, buffer + 1, ARRAY_SIZE(buffer) - 1);
        opt_c = buffer;
        buffer[2] = L'\0';
    }
    /* validate various options */
    if (!opt_cs) wcsupr(opt_c);
    /* check that default is in the choices list */
    if (!wcschr(opt_c, opt_cs ? opt_default : towupper(opt_default)))
        return_code = ERROR_INVALID_FUNCTION;
    /* check that there's no duplicates in the choices list */
    for (ptr = opt_c; *ptr; ptr++)
        if (wcschr(ptr + 1, opt_cs ? *ptr : towupper(*ptr)))
            return_code = ERROR_INVALID_FUNCTION;

    TRACE("CHOICE message(%s) choices(%s) timeout(%ld) default(%c)\n",
          debugstr_w(opt_m), debugstr_w(opt_c), opt_timeout, opt_default ? opt_default : '?');
    if (return_code != NO_ERROR ||
        (opt_timeout == -1) != (opt_default == L'\0') ||
        (opt_timeout != -1 && opt_timeout > 9999))
    {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_ARGERR));
        errorlevel = 255;
        if (opt_c != buffer) free(opt_c);
        free(opt_m);
        return ERROR_INVALID_FUNCTION;
    }

    have_console = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &oldmode);
    if (have_console)
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

    /* print the question, when needed */
    if (opt_m)
        WCMD_output_asis(opt_m);

    if (!opt_n)
    {
        /* print a list of all allowed answers inside brackets */
        if (opt_m) WCMD_output_asis(L" ");
        WCMD_output_asis(L"[");
        answer[1] = L'\0';
        for (ptr = opt_c; *ptr; ptr++)
        {
            if (ptr != opt_c)
                WCMD_output_asis(L",");
            answer[0] = *ptr;
            WCMD_output_asis(answer);
        }
        WCMD_output_asis(L"]?");
    }

    while (return_code == NO_ERROR)
    {
        if (opt_timeout == 0)
            answer[0] = opt_default;
        else
        {
            LARGE_INTEGER li, zeroli = {0};
            OVERLAPPED overlapped = {0};
            DWORD count;
            char choice;

            overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
            if (SetFilePointerEx(GetStdHandle(STD_INPUT_HANDLE), zeroli, &li, FILE_CURRENT))
            {
                overlapped.Offset = li.LowPart;
                overlapped.OffsetHigh = li.HighPart;
            }
            if (ReadFile(GetStdHandle(STD_INPUT_HANDLE), &choice, 1, NULL, &overlapped))
            {
                switch (WaitForSingleObject(overlapped.hEvent, opt_timeout == -1 ? INFINITE : opt_timeout * 1000))
                {
                case WAIT_OBJECT_0:
                    answer[0] = choice;
                    break;
                case WAIT_TIMEOUT:
                    answer[0] = opt_default;
                    break;
                default:
                    return_code = ERROR_INVALID_FUNCTION;
                }
            }
            else if (ReadFile(GetStdHandle(STD_INPUT_HANDLE), &choice, 1, &count, NULL))
            {
                if (count == 0)
                {
                    if (opt_timeout != -1)
                        answer[0] = opt_default;
                    else
                        return_code = ERROR_INVALID_FUNCTION;
                }
                else
                    answer[0] = choice;
            }
            else
                return_code = ERROR_INVALID_FUNCTION;
            CloseHandle(overlapped.hEvent);
        }
        if (return_code != NO_ERROR)
        {
            errorlevel = 255;
            break;
        }
        if (!opt_cs)
            answer[0] = towupper(answer[0]);

        answer[1] = L'\0'; /* terminate single character string */
        ptr = wcschr(opt_c, answer[0]);
        if (ptr)
        {
            WCMD_output_asis(answer);
            WCMD_output_asis(L"\r\n");

            return_code = errorlevel = (ptr - opt_c) + 1;
            TRACE("answer: %d\n", return_code);
        }
        else
        {
            /* key not allowed: play the bell */
            TRACE("key not allowed: %s\n", wine_dbgstr_w(answer));
            WCMD_output_asis(L"\a");
        }
    }
    if (have_console)
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), oldmode);

    if (opt_c != buffer) free(opt_c);
    free(opt_m);
    return return_code;
}

/****************************************************************************
 * WCMD_AppendEOF
 *
 * Adds an EOF onto the end of a file
 * Returns TRUE on success
 */
static BOOL WCMD_AppendEOF(WCHAR *filename)
{
    HANDLE h;
    DWORD bytes_written;

    char eof = '\x1a';

    WINE_TRACE("Appending EOF to %s\n", wine_dbgstr_w(filename));
    h = CreateFileW(filename, GENERIC_WRITE, 0, NULL,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (h == INVALID_HANDLE_VALUE) {
      WINE_ERR("Failed to open %s (%ld)\n", wine_dbgstr_w(filename), GetLastError());
      return FALSE;
    } else {
      SetFilePointer (h, 0, NULL, FILE_END);
      if (!WriteFile(h, &eof, 1, &bytes_written, NULL)) {
        WINE_ERR("Failed to append EOF to %s (%ld)\n", wine_dbgstr_w(filename), GetLastError());
        CloseHandle(h);
        return FALSE;
      }
      CloseHandle(h);
    }
    return TRUE;
}

/****************************************************************************
 * WCMD_IsSameFile
 *
 * Checks if the two paths reference to the same file
 */
static BOOL WCMD_IsSameFile(const WCHAR *name1, const WCHAR *name2)
{
  BOOL ret = FALSE;
  HANDLE file1 = INVALID_HANDLE_VALUE, file2 = INVALID_HANDLE_VALUE;
  BY_HANDLE_FILE_INFORMATION info1, info2;

  file1 = CreateFileW(name1, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
  if (file1 == INVALID_HANDLE_VALUE || !GetFileInformationByHandle(file1, &info1))
    goto end;

  file2 = CreateFileW(name2, 0, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
  if (file2 == INVALID_HANDLE_VALUE || !GetFileInformationByHandle(file2, &info2))
    goto end;

  ret = info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber
    && info1.nFileIndexHigh == info2.nFileIndexHigh
    && info1.nFileIndexLow == info2.nFileIndexLow;
end:
  if (file1 != INVALID_HANDLE_VALUE)
    CloseHandle(file1);
  if (file2 != INVALID_HANDLE_VALUE)
    CloseHandle(file2);
  return ret;
}

/****************************************************************************
 * WCMD_ManualCopy
 *
 * Copies from a file
 *    optionally reading only until EOF (ascii copy)
 *    optionally appending onto an existing file (append)
 * Returns TRUE on success
 */
static BOOL WCMD_ManualCopy(WCHAR *srcname, WCHAR *dstname, BOOL ascii, BOOL append)
{
    HANDLE in,out;
    BOOL   ok;
    DWORD  bytesread, byteswritten;

    WINE_TRACE("Manual Copying %s to %s (append?%d)\n",
               wine_dbgstr_w(srcname), wine_dbgstr_w(dstname), append);

    in  = CreateFileW(srcname, GENERIC_READ, 0, NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (in == INVALID_HANDLE_VALUE) {
      WINE_ERR("Failed to open %s (%ld)\n", wine_dbgstr_w(srcname), GetLastError());
      return FALSE;
    }

    /* Open the output file, overwriting if not appending */
    out = CreateFileW(dstname, GENERIC_WRITE, 0, NULL,
                      append?OPEN_EXISTING:CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (out == INVALID_HANDLE_VALUE) {
      WINE_ERR("Failed to open %s (%ld)\n", wine_dbgstr_w(dstname), GetLastError());
      CloseHandle(in);
      return FALSE;
    }

    /* Move to end of destination if we are going to append to it */
    if (append) {
      SetFilePointer(out, 0, NULL, FILE_END);
    }

    /* Loop copying data from source to destination until EOF read */
    do
    {
      char buffer[MAXSTRING];

      ok = ReadFile(in, buffer, MAXSTRING, &bytesread, NULL);
      if (ok) {

        /* Stop at first EOF */
        if (ascii) {
          char *ptr = (char *)memchr((void *)buffer, '\x1a', bytesread);
          if (ptr) bytesread = (ptr - buffer);
        }

        if (bytesread) {
          ok = WriteFile(out, buffer, bytesread, &byteswritten, NULL);
          if (!ok || byteswritten != bytesread) {
            WINE_ERR("Unexpected failure writing to %s, rc=%ld\n",
                     wine_dbgstr_w(dstname), GetLastError());
          }
        }
      } else {
        WINE_ERR("Unexpected failure reading from %s, rc=%ld\n",
                 wine_dbgstr_w(srcname), GetLastError());
      }
    } while (ok && bytesread > 0);

    CloseHandle(out);
    CloseHandle(in);
    return ok;
}

/****************************************************************************
 * WCMD_copy
 *
 * Copy a file or wildcarded set.
 * For ascii/binary type copies, it gets complex:
 *  Syntax on command line is
 *   ... /a | /b   filename  /a /b {[ + filename /a /b]}  [dest /a /b]
 *  Where first /a or /b sets 'mode in operation' until another is found
 *  once another is found, it applies to the file preceding the /a or /b
 *  In addition each filename can contain wildcards
 * To make matters worse, the + may be in the same parameter (i.e. no
 *  whitespace) or with whitespace separating it
 *
 * ASCII mode on read == read and stop at first EOF
 * ASCII mode on write == append EOF to destination
 * Binary == copy as-is
 *
 * Design of this is to build up a list of files which will be copied into a
 * list, then work through the list file by file.
 * If no destination is specified, it defaults to the name of the first file in
 * the list, but the current directory.
 *
 */

RETURN_CODE WCMD_copy(WCHAR * args)
{
  BOOL    opt_d, opt_v, opt_n, opt_z, opt_y, opt_noty;
  WCHAR  *thisparam;
  int     argno = 0;
  WCHAR  *rawarg;
  WIN32_FIND_DATAW fd;
  HANDLE  hff = INVALID_HANDLE_VALUE;
  int     binarymode = -1;            /* -1 means use the default, 1 is binary, 0 ascii */
  BOOL    concatnextfilename = FALSE; /* True if we have just processed a +             */
  BOOL    anyconcats         = FALSE; /* Have we found any + options                    */
  BOOL    appendfirstsource  = FALSE; /* Use first found filename as destination        */
  BOOL    writtenoneconcat   = FALSE; /* Remember when the first concatenated file done */
  BOOL    prompt;                     /* Prompt before overwriting                      */
  WCHAR   destname[MAX_PATH];         /* Used in calculating the destination name       */
  BOOL    destisdirectory = FALSE;    /* Is the destination a directory?                */
  BOOL    status;
  WCHAR   copycmd[4];
  DWORD   len;
  BOOL    dstisdevice = FALSE;

  typedef struct _COPY_FILES
  {
    struct _COPY_FILES *next;
    BOOL                concatenate;
    WCHAR              *name;
    int                 binarycopy;
  } COPY_FILES;
  COPY_FILES *sourcelist    = NULL;
  COPY_FILES *lastcopyentry = NULL;
  COPY_FILES *destination   = NULL;
  COPY_FILES *thiscopy      = NULL;
  COPY_FILES *prevcopy      = NULL;
  RETURN_CODE return_code;

  /* Assume we were successful! */
  return_code = NO_ERROR;

  /* If no args supplied at all, report an error */
  if (param1[0] == 0x00) {
    WCMD_output_stderr (WCMD_LoadMessage(WCMD_NOARG));
    return errorlevel = ERROR_INVALID_FUNCTION;
  }

  opt_d = opt_v = opt_n = opt_z = opt_y = opt_noty = FALSE;

  /* Walk through all args, building up a list of files to process */
  thisparam = WCMD_parameter(args, argno++, &rawarg, TRUE, FALSE);
  while (*(thisparam)) {
    WCHAR *pos1, *pos2;
    BOOL inquotes;

    WINE_TRACE("Working on parameter '%s'\n", wine_dbgstr_w(thisparam));

    /* Handle switches */
    if (*thisparam == '/') {
        while (*thisparam == '/') {
        thisparam++;
        if (towupper(*thisparam) == 'D') {
          opt_d = TRUE;
          if (opt_d) WINE_FIXME("copy /D support not implemented yet\n");
        } else if (towupper(*thisparam) == 'Y') {
          opt_y = TRUE;
        } else if (towupper(*thisparam) == '-' && towupper(*(thisparam+1)) == 'Y') {
          opt_noty = TRUE;
        } else if (towupper(*thisparam) == 'V') {
          opt_v = TRUE;
          if (opt_v) WINE_FIXME("copy /V support not implemented yet\n");
        } else if (towupper(*thisparam) == 'N') {
          opt_n = TRUE;
          if (opt_n) WINE_FIXME("copy /N support not implemented yet\n");
        } else if (towupper(*thisparam) == 'Z') {
          opt_z = TRUE;
          if (opt_z) WINE_FIXME("copy /Z support not implemented yet\n");
        } else if (towupper(*thisparam) == 'A') {
          if (binarymode != 0) {
            binarymode = 0;
            WINE_TRACE("Subsequent files will be handled as ASCII\n");
            if (destination != NULL) {
              WINE_TRACE("file %s will be written as ASCII\n", wine_dbgstr_w(destination->name));
              destination->binarycopy = binarymode;
            } else if (lastcopyentry != NULL) {
              WINE_TRACE("file %s will be read as ASCII\n", wine_dbgstr_w(lastcopyentry->name));
              lastcopyentry->binarycopy = binarymode;
            }
          }
        } else if (towupper(*thisparam) == 'B') {
          if (binarymode != 1) {
            binarymode = 1;
            WINE_TRACE("Subsequent files will be handled as binary\n");
            if (destination != NULL) {
              WINE_TRACE("file %s will be written as binary\n", wine_dbgstr_w(destination->name));
              destination->binarycopy = binarymode;
            } else if (lastcopyentry != NULL) {
              WINE_TRACE("file %s will be read as binary\n", wine_dbgstr_w(lastcopyentry->name));
              lastcopyentry->binarycopy = binarymode;
            }
          }
        } else {
          WINE_FIXME("Unexpected copy switch %s\n", wine_dbgstr_w(thisparam));
        }
        thisparam++;
      }

      /* This parameter was purely switches, get the next one */
      thisparam = WCMD_parameter(args, argno++, &rawarg, TRUE, FALSE);
      continue;
    }

    /* We have found something which is not a switch. If could be anything of the form
         sourcefilename (which could be destination too)
         + (when filename + filename syntex used)
         sourcefilename+sourcefilename
         +sourcefilename
         +/b[tests show windows then ignores to end of parameter]
     */

    if (*thisparam=='+') {
      if (lastcopyentry == NULL) {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
        return_code = ERROR_INVALID_FUNCTION;
        goto exitreturn;
      } else {
        concatnextfilename = TRUE;
        anyconcats         = TRUE;
      }

      /* Move to next thing to process */
      thisparam++;
      if (*thisparam == 0x00)
        thisparam = WCMD_parameter(args, argno++, &rawarg, TRUE, FALSE);
      continue;
    }

    /* We have found something to process - build a COPY_FILE block to store it */
    thiscopy = xalloc(sizeof(COPY_FILES));

    WINE_TRACE("Not a switch, but probably a filename/list %s\n", wine_dbgstr_w(thisparam));
    thiscopy->concatenate = concatnextfilename;
    thiscopy->binarycopy  = binarymode;
    thiscopy->next        = NULL;

    /* Time to work out the name. Allocate at least enough space (deliberately too much to
       leave space to append \* to the end) , then copy in character by character. Strip off
       quotes if we find them.                                                               */
    len = lstrlenW(thisparam) + (sizeof(WCHAR) * 5);  /* 5 spare characters, null + \*.*      */
    thiscopy->name = xalloc(len * sizeof(WCHAR));
    memset(thiscopy->name, 0x00, len);

    pos1 = thisparam;
    pos2 = thiscopy->name;
    inquotes = FALSE;
    while (*pos1 && (inquotes || (*pos1 != '+' && *pos1 != '/'))) {
      if (*pos1 == '"') {
        inquotes = !inquotes;
        pos1++;
      } else *pos2++ = *pos1++;
    }
    *pos2 = 0;
    WINE_TRACE("Calculated file name %s\n", wine_dbgstr_w(thiscopy->name));

    /* This is either the first source, concatenated subsequent source or destination */
    if (sourcelist == NULL) {
      WINE_TRACE("Adding as first source part\n");
      sourcelist = thiscopy;
      lastcopyentry = thiscopy;
    } else if (concatnextfilename) {
      WINE_TRACE("Adding to source file list to be concatenated\n");
      lastcopyentry->next = thiscopy;
      lastcopyentry = thiscopy;
    } else if (destination == NULL) {
      destination = thiscopy;
    } else {
      /* We have processed sources and destinations and still found more to do - invalid */
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
      return_code = ERROR_INVALID_FUNCTION;
      goto exitreturn;
    }
    concatnextfilename    = FALSE;

    /* We either need to process the rest of the parameter or move to the next */
    if (*pos1 == '/' || *pos1 == '+') {
      thisparam = pos1;
      continue;
    } else {
      thisparam = WCMD_parameter(args, argno++, &rawarg, TRUE, FALSE);
    }
  }

  /* Ensure we have at least one source file */
  if (!sourcelist) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
    return_code = ERROR_INVALID_FUNCTION;
    goto exitreturn;
  }

  /* Default whether automatic overwriting is on. If we are interactive then
     we prompt by default, otherwise we overwrite by default
     /-Y has the highest priority, then /Y and finally the COPYCMD env. variable */
  if (opt_noty) prompt = TRUE;
  else if (opt_y) prompt = FALSE;
  else {
    /* By default, we will force the overwrite in batch mode and ask for
     * confirmation in interactive mode. */
    prompt = interactive;
    /* If COPYCMD is set, then we force the overwrite with /Y and ask for
     * confirmation with /-Y. If COPYCMD is neither of those, then we use the
     * default behavior. */
    len = GetEnvironmentVariableW(L"COPYCMD", copycmd, ARRAY_SIZE(copycmd));
    if (len && len < ARRAY_SIZE(copycmd)) {
      if (!lstrcmpiW(copycmd, L"/Y"))
        prompt = FALSE;
      else if (!lstrcmpiW(copycmd, L"/-Y"))
        prompt = TRUE;
    }
  }

  /* Calculate the destination now - if none supplied, it's current dir +
     filename of first file in list*/
  if (destination == NULL) {

    WINE_TRACE("No destination supplied, so need to calculate it\n");

    lstrcpyW(destname, L".");
    lstrcatW(destname, L"\\");

    destination = xalloc(sizeof(COPY_FILES));
    if (destination == NULL) goto exitreturn;
    destination->concatenate = FALSE;           /* Not used for destination */
    destination->binarycopy  = binarymode;
    destination->next        = NULL;            /* Not used for destination */
    destination->name        = NULL;            /* To be filled in          */
    destisdirectory          = TRUE;

  } else {
    WCHAR *filenamepart;
    DWORD  attributes;

    WINE_TRACE("Destination supplied, processing to see if file or directory\n");

    /* Convert to fully qualified path/filename */
    if (!WCMD_get_fullpath(destination->name, ARRAY_SIZE(destname), destname, &filenamepart))
        return errorlevel = ERROR_INVALID_FUNCTION;
    WINE_TRACE("Full dest name is '%s'\n", wine_dbgstr_w(destname));

    /* If parameter is a directory, ensure it ends in \ */
    attributes = GetFileAttributesW(destname);
    if (ends_with_backslash( destname ) ||
        ((attributes != INVALID_FILE_ATTRIBUTES) &&
         (attributes & FILE_ATTRIBUTE_DIRECTORY))) {

      destisdirectory = TRUE;
      if (!ends_with_backslash(destname)) lstrcatW(destname, L"\\");
      WINE_TRACE("Directory, so full name is now '%s'\n", wine_dbgstr_w(destname));
    }
  }

  /* Normally, the destination is the current directory unless we are
     concatenating, in which case it's current directory plus first filename.
     Note that if the
     In addition by default it is a binary copy unless concatenating, when
     the copy defaults to an ascii copy (stop at EOF). We do not know the
     first source part yet (until we search) so flag as needing filling in. */

  if (anyconcats) {
    /* We have found an a+b type syntax, so destination has to be a filename
       and we need to default to ascii copying. If we have been supplied a
       directory as the destination, we need to defer calculating the name   */
    if (destisdirectory) appendfirstsource = TRUE;
    if (destination->binarycopy == -1) destination->binarycopy = 0;

  } else if (!destisdirectory) {
    /* We have been asked to copy to a filename. Default to ascii IF the
       source contains wildcards (true even if only one match)           */
    if (wcspbrk(sourcelist->name, L"*?") != NULL) {
      anyconcats = TRUE;  /* We really are concatenating to a single file */
      if (destination->binarycopy == -1) {
        destination->binarycopy = 0;
      }
    } else {
      if (destination->binarycopy == -1) {
        destination->binarycopy = 1;
      }
    }
  }

  /* Save away the destination name*/
  free(destination->name);
  destination->name = xstrdupW(destname);
  WINE_TRACE("Resolved destination is '%s' (calc later %d)\n",
             wine_dbgstr_w(destname), appendfirstsource);

  /* Remember if the destination is a device */
  if (wcsncmp(destination->name, L"\\\\.\\", lstrlenW(L"\\\\.\\")) == 0) {
    WINE_TRACE("Destination is a device\n");
    dstisdevice = TRUE;
  }

  /* Now we need to walk the set of sources, and process each name we come to.
     If anyconcats is true, we are writing to one file, otherwise we are using
     the source name each time.
     If destination exists, prompt for overwrite the first time (if concatenating
     we ask each time until yes is answered)
     The first source file we come across must exist (when wildcards expanded)
     and if concatenating with overwrite prompts, each source file must exist
     until a yes is answered.                                                    */

  thiscopy = sourcelist;
  prevcopy = NULL;

  return_code = NO_ERROR;
  while (thiscopy != NULL) {

    WCHAR  srcpath[MAX_PATH];
    const  WCHAR *srcname;
    WCHAR *filenamepart;
    DWORD  attributes;
    BOOL   srcisdevice = FALSE;

    /* If it was not explicit, we now know whether we are concatenating or not and
       hence whether to copy as binary or ascii                                    */
    if (thiscopy->binarycopy == -1) thiscopy->binarycopy = !anyconcats;

    /* Convert to fully qualified path/filename in srcpath, file filenamepart pointing
       to where the filename portion begins (used for wildcard expansion).             */
    if (!WCMD_get_fullpath(thiscopy->name, ARRAY_SIZE(srcpath), srcpath, &filenamepart))
        return errorlevel = ERROR_INVALID_FUNCTION;
    WINE_TRACE("Full src name is '%s'\n", wine_dbgstr_w(srcpath));

    /* If parameter is a directory, ensure it ends in \* */
    attributes = GetFileAttributesW(srcpath);
    if (ends_with_backslash( srcpath )) {

      /* We need to know where the filename part starts, so append * and
         recalculate the full resulting path                              */
      lstrcatW(thiscopy->name, L"*");
      if (!WCMD_get_fullpath(thiscopy->name, ARRAY_SIZE(srcpath), srcpath, &filenamepart))
          return errorlevel = ERROR_INVALID_FUNCTION;
      WINE_TRACE("Directory, so full name is now '%s'\n", wine_dbgstr_w(srcpath));

    } else if ((wcspbrk(srcpath, L"*?") == NULL) &&
               (attributes != INVALID_FILE_ATTRIBUTES) &&
               (attributes & FILE_ATTRIBUTE_DIRECTORY)) {

      /* We need to know where the filename part starts, so append \* and
         recalculate the full resulting path                              */
      lstrcatW(thiscopy->name, L"\\*");
      if (!WCMD_get_fullpath(thiscopy->name, ARRAY_SIZE(srcpath), srcpath, &filenamepart))
          return errorlevel = ERROR_INVALID_FUNCTION;
      WINE_TRACE("Directory, so full name is now '%s'\n", wine_dbgstr_w(srcpath));
    }

    WINE_TRACE("Copy source (calculated): path: '%s' (Concats: %d)\n",
                    wine_dbgstr_w(srcpath), anyconcats);

    /* If the source is a device, just use it, otherwise search */
    if (wcsncmp(srcpath, L"\\\\.\\", lstrlenW(L"\\\\.\\")) == 0) {
      WINE_TRACE("Source is a device\n");
      srcisdevice = TRUE;
      srcname  = &srcpath[4]; /* After the \\.\ prefix */
    } else {

      /* Loop through all source files */
      WINE_TRACE("Searching for: '%s'\n", wine_dbgstr_w(srcpath));
      hff = FindFirstFileW(srcpath, &fd);
      if (hff != INVALID_HANDLE_VALUE) {
        srcname = fd.cFileName;
      }
    }

    if (srcisdevice || hff != INVALID_HANDLE_VALUE) {
      do {
        WCHAR outname[MAX_PATH];
        BOOL  overwrite;
        BOOL  appendtofirstfile = FALSE;

        /* Skip . and .., and directories */
        if (!srcisdevice && fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          WINE_TRACE("Skipping directories\n");
        } else {

          /* Build final destination name */
          lstrcpyW(outname, destination->name);
          if (destisdirectory || appendfirstsource) lstrcatW(outname, srcname);

          /* Build source name */
          if (!srcisdevice) lstrcpyW(filenamepart, srcname);

          /* Do we just overwrite (we do if we are writing to a device) */
          overwrite = !prompt;
          if (dstisdevice || (anyconcats && writtenoneconcat)) {
            overwrite = TRUE;
          }

          WINE_TRACE("Copying from : '%s'\n", wine_dbgstr_w(srcpath));
          WINE_TRACE("Copying to : '%s'\n", wine_dbgstr_w(outname));
          WINE_TRACE("Flags: srcbinary(%d), dstbinary(%d), over(%d), prompt(%d)\n",
                     thiscopy->binarycopy, destination->binarycopy, overwrite, prompt);

          if (!writtenoneconcat) {
            appendtofirstfile = anyconcats && WCMD_IsSameFile(srcpath, outname);
          }

          /* Prompt before overwriting */
          if (appendtofirstfile) {
            overwrite = TRUE;
          } else if (!overwrite) {
            DWORD attributes = GetFileAttributesW(outname);
            if (attributes != INVALID_FILE_ATTRIBUTES) {
              WCHAR* question;
              question = WCMD_format_string(WCMD_LoadMessage(WCMD_OVERWRITE), outname);
              overwrite = WCMD_ask_confirm(question, FALSE, NULL);
              LocalFree(question);
            }
            else overwrite = TRUE;
          }

          /* If we needed to save away the first filename, do it */
          if (appendfirstsource && overwrite) {
            free(destination->name);
            destination->name = xstrdupW(outname);
            WINE_TRACE("Final resolved destination name : '%s'\n", wine_dbgstr_w(outname));
            appendfirstsource = FALSE;
            destisdirectory = FALSE;
          }

          /* Do the copy as appropriate */
          if (overwrite) {
            if (anyconcats && WCMD_IsSameFile(srcpath, outname)) {
              /* behavior is as Unix 'touch' (change last-written time only) */
              HANDLE file = CreateFileW(srcpath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
              if (file != INVALID_HANDLE_VALUE)
              {
                FILETIME file_time;
                SYSTEMTIME system_time;

                GetSystemTime(&system_time);
                SystemTimeToFileTime(&system_time, &file_time);
                status = SetFileTime(file, NULL, NULL, &file_time);
                CloseHandle(file);
              }
              else status = FALSE;
            } else if (anyconcats && writtenoneconcat) {
              if (thiscopy->binarycopy) {
                status = WCMD_ManualCopy(srcpath, outname, FALSE, TRUE);
              } else {
                status = WCMD_ManualCopy(srcpath, outname, TRUE, TRUE);
              }
            } else if (!thiscopy->binarycopy) {
              status = WCMD_ManualCopy(srcpath, outname, TRUE, FALSE);
            } else if (srcisdevice) {
              status = WCMD_ManualCopy(srcpath, outname, FALSE, FALSE);
            } else {
              status = CopyFileW(srcpath, outname, FALSE);
            }
            if (!status) {
              WCMD_print_error ();
              return_code = ERROR_INVALID_FUNCTION;
            } else {
              WINE_TRACE("Copied successfully\n");
              if (anyconcats) writtenoneconcat = TRUE;

              /* Append EOF if ascii destination and we are not going to add more onto the end
                 Note: Testing shows windows has an optimization whereas if you have a binary
                 copy of a file to a single destination (ie concatenation) then it does not add
                 the EOF, hence the check on the source copy type below.                       */
              if (!destination->binarycopy && !anyconcats && !thiscopy->binarycopy) {
                if (!WCMD_AppendEOF(outname)) {
                  WCMD_print_error ();
                  return_code = ERROR_INVALID_FUNCTION;
                }
              }
            }
          } else if (prompt)
              return_code = ERROR_INVALID_FUNCTION;
        }
      } while (!srcisdevice && FindNextFileW(hff, &fd) != 0);
      if (!srcisdevice) FindClose (hff);
    } else {
      /* Error if the first file was not found */
      if (!anyconcats || !writtenoneconcat) {
        WCMD_print_error ();
        return_code = ERROR_INVALID_FUNCTION;
      }
    }

    /* Step on to the next supplied source */
    thiscopy = thiscopy -> next;
  }

  /* Append EOF if ascii destination and we were concatenating */
  if (!return_code && !destination->binarycopy && anyconcats && writtenoneconcat) {
    if (!WCMD_AppendEOF(destination->name)) {
      WCMD_print_error ();
      return_code = ERROR_INVALID_FUNCTION;
    }
  }

  /* Exit out of the routine, freeing any remaining allocated memory */
exitreturn:

  thiscopy = sourcelist;
  while (thiscopy != NULL) {
    prevcopy = thiscopy;
    /* Free up this block*/
    thiscopy = thiscopy -> next;
    free(prevcopy->name);
    free(prevcopy);
  }

  /* Free up the destination memory */
  if (destination) {
    free(destination->name);
    free(destination);
  }

  return errorlevel = return_code;
}

/****************************************************************************
 * WCMD_create_dir
 *
 * Create a directory (and, if needed, any intermediate directories).
 *
 * Modifies its argument by replacing slashes temporarily with nulls.
 */

static BOOL create_full_path(WCHAR* path)
{
    WCHAR *p, *start;

    /* don't mess with drive letter portion of path, if any */
    start = path;
    if (path[1] == ':')
        start = path+2;

    /* Strip trailing slashes. */
    for (p = path + lstrlenW(path) - 1; p != start && *p == '\\'; p--)
        *p = 0;

    /* Step through path, creating intermediate directories as needed. */
    /* First component includes drive letter, if any. */
    p = start;
    for (;;) {
        DWORD rv;
        /* Skip to end of component */
        while (*p == '\\') p++;
        while (*p && *p != '\\') p++;
        if (!*p) {
            /* path is now the original full path */
            return CreateDirectoryW(path, NULL);
        }
        /* Truncate path, create intermediate directory, and restore path */
        *p = 0;
        rv = CreateDirectoryW(path, NULL);
        *p = '\\';
        if (!rv && GetLastError() != ERROR_ALREADY_EXISTS)
            return FALSE;
    }
    /* notreached */
    return FALSE;
}

RETURN_CODE WCMD_create_dir(WCHAR *args)
{
    int   argno = 0;
    WCHAR *argN = args;
    RETURN_CODE return_code;

    if (param1[0] == L'\0')
    {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
        return errorlevel = ERROR_INVALID_FUNCTION;
    }
    return_code = NO_ERROR;
    /* Loop through all args */
    for (;;)
    {
        WCHAR *thisArg = WCMD_parameter(args, argno++, &argN, FALSE, FALSE);
        if (!argN) break;
        if (!create_full_path(thisArg))
        {
            WCMD_print_error();
            return_code = ERROR_INVALID_FUNCTION;
        }
    }
    return errorlevel = return_code;
}

/* Parse the /A options given by the user on the commandline
 * into a bitmask of wanted attributes (*wantSet),
 * and a bitmask of unwanted attributes (*wantClear).
 */
static void WCMD_delete_parse_attributes(DWORD *wantSet, DWORD *wantClear) {
    WCHAR *p;

    /* both are strictly 'out' parameters */
    *wantSet=0;
    *wantClear=0;

    /* For each /A argument */
    for (p=wcsstr(quals, L"/A"); p != NULL; p=wcsstr(p, L"/A")) {
        /* Skip /A itself */
        p += 2;

        /* Skip optional : */
        if (*p == ':') p++;

        /* For each of the attribute specifier chars to this /A option */
        for (; *p != 0 && *p != '/'; p++) {
            BOOL negate = FALSE;
            DWORD mask  = 0;

            if (*p == '-') {
                negate=TRUE;
                p++;
            }

            /* Convert the attribute specifier to a bit in one of the masks */
            switch (*p) {
            case 'R': mask = FILE_ATTRIBUTE_READONLY; break;
            case 'H': mask = FILE_ATTRIBUTE_HIDDEN;   break;
            case 'S': mask = FILE_ATTRIBUTE_SYSTEM;   break;
            case 'A': mask = FILE_ATTRIBUTE_ARCHIVE;  break;
            default:
                WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
            }
            if (negate)
                *wantClear |= mask;
            else
                *wantSet |= mask;
        }
    }
}

/* If filename part of parameter is * or *.*,
 * and neither /Q nor /P options were given,
 * prompt the user whether to proceed.
 * Returns FALSE if user says no, TRUE otherwise.
 * *pPrompted is set to TRUE if the user is prompted.
 * (If /P supplied, del will prompt for individual files later.)
 */
static BOOL WCMD_delete_confirm_wildcard(const WCHAR *filename, BOOL *pPrompted) {
    if ((wcsstr(quals, L"/Q") == NULL) && (wcsstr(quals, L"/P") == NULL)) {
        WCHAR drive[10];
        WCHAR dir[MAX_PATH];
        WCHAR fname[MAX_PATH];
        WCHAR ext[MAX_PATH];
        WCHAR fpath[MAX_PATH];

        /* Convert path into actual directory spec */
        if (!WCMD_get_fullpath(filename, ARRAY_SIZE(fpath), fpath, NULL)) return FALSE;
        _wsplitpath(fpath, drive, dir, fname, ext);

        /* Only prompt for * and *.*, not *a, a*, *.a* etc */
        if ((lstrcmpW(fname, L"*") == 0) && (*ext == 0x00 || (lstrcmpW(ext, L".*") == 0))) {

            WCHAR question[MAXSTRING];

            /* Caller uses this to suppress "file not found" warning later */
            *pPrompted = TRUE;

            /* Ask for confirmation */
            wsprintfW(question, L"%s ", fpath);
            return WCMD_ask_confirm(question, TRUE, NULL);
        }
    }
    /* No scary wildcard, or question suppressed, so it's ok to delete the file(s) */
    return TRUE;
}

/* Helper function for WCMD_delete().
 * Deletes a single file, directory, or wildcard.
 * If /S was given, does it recursively.
 * Returns TRUE if a file was deleted.
 */
static BOOL WCMD_delete_one (const WCHAR *thisArg) {
    DWORD wanted_attrs;
    DWORD unwanted_attrs;
    BOOL found = FALSE;
    WCHAR argCopy[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE hff;
    WCHAR fpath[MAX_PATH];
    WCHAR *p;
    BOOL handleParm = TRUE;

    WCMD_delete_parse_attributes(&wanted_attrs, &unwanted_attrs);

    lstrcpyW(argCopy, thisArg);
    WINE_TRACE("del: Processing arg %s (quals:%s)\n",
               wine_dbgstr_w(argCopy), wine_dbgstr_w(quals));

    if (!WCMD_delete_confirm_wildcard(argCopy, &found)) {
        /* Skip this arg if user declines to delete *.* */
        return FALSE;
    }

    /* First, try to delete in the current directory */
    hff = FindFirstFileW(argCopy, &fd);
    if (hff == INVALID_HANDLE_VALUE) {
      handleParm = FALSE;
    } else {
      found = TRUE;
    }

    /* Support del <dirname> by just deleting all files dirname\* */
    if (handleParm
        && (wcschr(argCopy,'*') == NULL)
        && (wcschr(argCopy,'?') == NULL)
        && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      WCHAR modifiedParm[MAX_PATH];

      lstrcpyW(modifiedParm, argCopy);
      lstrcatW(modifiedParm, L"\\*");
      FindClose(hff);
      found = TRUE;
      WCMD_delete_one(modifiedParm);

    } else if (handleParm) {

      /* Build the filename to delete as <supplied directory>\<findfirst filename> */
      lstrcpyW (fpath, argCopy);
      do {
        p = wcsrchr (fpath, '\\');
        if (p != NULL) {
          *++p = '\0';
          lstrcatW (fpath, fd.cFileName);
        }
        else lstrcpyW (fpath, fd.cFileName);
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
          BOOL ok;

          /* Handle attribute matching (/A) */
          ok =  ((fd.dwFileAttributes & wanted_attrs) == wanted_attrs)
             && ((fd.dwFileAttributes & unwanted_attrs) == 0);

          /* /P means prompt for each file */
          if (ok && wcsstr(quals, L"/P") != NULL) {
            WCHAR* question;

            /* Ask for confirmation */
            question = WCMD_format_string(WCMD_LoadMessage(WCMD_DELPROMPT), fpath);
            ok = WCMD_ask_confirm(question, FALSE, NULL);
            LocalFree(question);
          }

          /* Only proceed if ok to */
          if (ok) {

            /* If file is read only, and /A:r or /F supplied, delete it */
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY &&
                ((wanted_attrs & FILE_ATTRIBUTE_READONLY) ||
                wcsstr(quals, L"/F") != NULL)) {
                SetFileAttributesW(fpath, fd.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
            }

            /* Now do the delete */
            if (!DeleteFileW(fpath)) WCMD_print_error ();
          }

        }
      } while (FindNextFileW(hff, &fd) != 0);
      FindClose (hff);
    }

    /* Now recurse into all subdirectories handling the parameter in the same way */
    if (wcsstr(quals, L"/S") != NULL) {

      WCHAR thisDir[MAX_PATH];
      int cPos;

      WCHAR drive[10];
      WCHAR dir[MAX_PATH];
      WCHAR fname[MAX_PATH];
      WCHAR ext[MAX_PATH];

      /* Convert path into actual directory spec */
      if (!WCMD_get_fullpath(argCopy, ARRAY_SIZE(thisDir), thisDir, NULL)) return FALSE;

      _wsplitpath(thisDir, drive, dir, fname, ext);

      lstrcpyW(thisDir, drive);
      lstrcatW(thisDir, dir);
      cPos = lstrlenW(thisDir);

      WINE_TRACE("Searching recursively in '%s'\n", wine_dbgstr_w(thisDir));

      /* Append '*' to the directory */
      thisDir[cPos] = '*';
      thisDir[cPos+1] = 0x00;

      hff = FindFirstFileW(thisDir, &fd);

      /* Remove residual '*' */
      thisDir[cPos] = 0x00;

      if (hff != INVALID_HANDLE_VALUE) {
        DIRECTORY_STACK *allDirs = NULL;
        DIRECTORY_STACK *lastEntry = NULL;

        do {
          if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
              (lstrcmpW(fd.cFileName, L"..") != 0) && (lstrcmpW(fd.cFileName, L".") != 0)) {

            DIRECTORY_STACK *nextDir;
            WCHAR subParm[MAX_PATH];

            if (wcslen(thisDir) + wcslen(fd.cFileName) + 1 + wcslen(fname) + wcslen(ext) >= MAX_PATH)
            {
                WINE_TRACE("Skipping path too long %s%s\\%s%s\n",
                           debugstr_w(thisDir), debugstr_w(fd.cFileName),
                           debugstr_w(fname), debugstr_w(ext));
                continue;
            }
            /* Work out search parameter in sub dir */
            lstrcpyW (subParm, thisDir);
            lstrcatW (subParm, fd.cFileName);
            lstrcatW (subParm, L"\\");
            lstrcatW (subParm, fname);
            lstrcatW (subParm, ext);
            WINE_TRACE("Recursive, Adding to search list '%s'\n", wine_dbgstr_w(subParm));

            /* Allocate memory, add to list */
            nextDir = WCMD_dir_stack_create(subParm, NULL);
            if (allDirs == NULL) allDirs = nextDir;
            if (lastEntry != NULL) lastEntry->next = nextDir;
            lastEntry = nextDir;
          }
        } while (FindNextFileW(hff, &fd) != 0);
        FindClose (hff);

        /* Go through each subdir doing the delete */
        while (allDirs != NULL) {
          found |= WCMD_delete_one (allDirs->dirName);
          allDirs = WCMD_dir_stack_free(allDirs);
        }
      }
    }

    return found;
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

RETURN_CODE WCMD_delete(WCHAR *args)
{
    int   argno;
    WCHAR *argN;
    BOOL  argsProcessed = FALSE;

    errorlevel = NO_ERROR;

    for (argno = 0; ; argno++)
    {
        WCHAR *thisArg;

        argN = NULL;
        thisArg = WCMD_parameter(args, argno, &argN, FALSE, FALSE);
        if (!argN)
            break;       /* no more parameters */
        if (argN[0] == '/')
            continue;    /* skip options */

        argsProcessed = TRUE;
        if (!WCMD_delete_one(thisArg))
        {
            errorlevel = ERROR_INVALID_FUNCTION;
        }
    }

    /* Handle no valid args */
    if (!argsProcessed)
    {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
        errorlevel = ERROR_INVALID_FUNCTION;
    }

    return errorlevel;
}

/*
 * WCMD_strtrim
 *
 * Returns a trimmed version of s with all leading and trailing whitespace removed
 * Pre: s non NULL
 *
 */
static WCHAR *WCMD_strtrim(const WCHAR *s)
{
    DWORD len = lstrlenW(s);
    const WCHAR *start = s;
    WCHAR* result;

    result = xalloc((len + 1) * sizeof(WCHAR));

    while (iswspace(*start)) start++;
    if (*start) {
        const WCHAR *end = s + len - 1;
        while (end > start && iswspace(*end)) end--;
        memcpy(result, start, (end - start + 2) * sizeof(WCHAR));
        result[end - start + 1] = '\0';
    } else {
        result[0] = '\0';
    }

    return result;
}

/****************************************************************************
 * WCMD_echo
 *
 * Echo input to the screen (or not). We don't try to emulate the bugs
 * in DOS (try typing "ECHO ON AGAIN" for an example).
 */

RETURN_CODE WCMD_echo(const WCHAR *args)
{
  int count;
  const WCHAR *origcommand = args;
  WCHAR *trimmed;

  if (   args[0]==' ' || args[0]=='\t' || args[0]=='.'
      || args[0]==':' || args[0]==';'  || args[0]=='/')
    args++;

  trimmed = WCMD_strtrim(args);
  if (!trimmed) return NO_ERROR;

  count = lstrlenW(trimmed);
  if (count == 0 && origcommand[0]!='.' && origcommand[0]!=':'
                 && origcommand[0]!=';' && origcommand[0]!='/') {
    if (echo_mode) WCMD_output(WCMD_LoadMessage(WCMD_ECHOPROMPT), L"ON");
    else WCMD_output (WCMD_LoadMessage(WCMD_ECHOPROMPT), L"OFF");
    free(trimmed);
    return NO_ERROR;
  }

  if (lstrcmpiW(trimmed, L"ON") == 0)
    echo_mode = TRUE;
  else if (lstrcmpiW(trimmed, L"OFF") == 0)
    echo_mode = FALSE;
  else {
    WCMD_output_asis (args);
    WCMD_output_asis(L"\r\n");
  }
  free(trimmed);
  return NO_ERROR;
}

/*****************************************************************************
 * WCMD_add_dirstowalk
 *
 * When recursing through directories (for /r), we need to add to the list of
 * directories still to walk, any subdirectories of the one we are processing.
 *
 * Parameters
 *  options    [I] The remaining list of directories still to process
 *
 * Note this routine inserts the subdirectories found between the entry being
 * processed, and any other directory still to be processed, mimicking what
 * Windows does
 */
void WCMD_add_dirstowalk(DIRECTORY_STACK *dirsToWalk)
{
    DIRECTORY_STACK *remainingDirs = dirsToWalk;
    WCHAR fullitem[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE hff;

    /* Build a generic search and add all directories on the list of directories
       still to walk                                                             */
    lstrcpyW(fullitem, dirsToWalk->dirName);
    lstrcatW(fullitem, L"\\*");
    if ((hff = FindFirstFileW(fullitem, &fd)) == INVALID_HANDLE_VALUE) return;

    do
    {
        TRACE("Looking for subdirectories\n");
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (lstrcmpW(fd.cFileName, L"..") != 0) && (lstrcmpW(fd.cFileName, L".") != 0))
        {
            /* Allocate memory, add to list */
            DIRECTORY_STACK *toWalk;
            if (wcslen(dirsToWalk->dirName) + 1 + wcslen(fd.cFileName) >= MAX_PATH)
            {
                TRACE("Skipping too long path %s\\%s\n",
                      debugstr_w(dirsToWalk->dirName), debugstr_w(fd.cFileName));
                continue;
            }
            toWalk = WCMD_dir_stack_create(dirsToWalk->dirName, fd.cFileName);
            TRACE("(%p->%p)\n", remainingDirs, remainingDirs->next);
            toWalk->next = remainingDirs->next;
            remainingDirs->next = toWalk;
            remainingDirs = toWalk;
            TRACE("Added to stack %s (%p->%p)\n", wine_dbgstr_w(toWalk->dirName),
                  toWalk, toWalk->next);
        }
    } while (FindNextFileW(hff, &fd) != 0);
    TRACE("Finished adding all subdirectories\n");
    FindClose(hff);
}

static int find_in_array(const WCHAR array[][10], size_t sz, const WCHAR *what)
{
    int i;

    for (i = 0; i < sz; i++)
        if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
                           what, -1, array[i], -1) == CSTR_EQUAL)
            return i;
    return -1;
}

/**************************************************************************
 * WCMD_give_help
 *
 *	Simple on-line help. Help text is stored in the resource file.
 */

RETURN_CODE WCMD_give_help(WCHAR *args)
{
    WCHAR *help_on = WCMD_parameter(args, 0, NULL, FALSE, FALSE);

    /* yes, return code / errorlevel look inverted, but native does it this way */
    if (!*help_on)
        WCMD_output_asis(WCMD_LoadMessage(WCMD_ALLHELP));
    else
    {
        int i;
        /* Display help message for builtin commands */
        if ((i = find_in_array(inbuilt, ARRAY_SIZE(inbuilt), help_on)) >= 0)
            WCMD_output_asis(WCMD_LoadMessage(i));
        else if ((i = find_in_array(externals, ARRAY_SIZE(externals), help_on)) >= 0)
        {
            WCHAR cmd[128];
            lstrcpyW(cmd, help_on);
            lstrcatW(cmd, L" /?");
            WCMD_run_builtin_command(WCMD_HELP, cmd);
        }
        else
        {
            WCMD_output(WCMD_LoadMessage(WCMD_NOCMDHELP), help_on);
            return errorlevel = NO_ERROR;
        }
    }
    return errorlevel = ERROR_INVALID_FUNCTION;
}

/****************************************************************************
 * WCMD_goto
 *
 * Batch file jump instruction. Not the most efficient algorithm ;-)
 * Prints error message if the specified label cannot be found - the file pointer is
 * then at EOF, effectively stopping the batch file.
 * FIXME: DOS is supposed to allow labels with spaces - we don't.
 */

RETURN_CODE WCMD_goto(void)
{
    if (context != NULL)
    {
        WCHAR *paramStart = param1;
        HANDLE h;
        BOOL ret;

        if (!param1[0])
        {
            WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
            return ERROR_INVALID_FUNCTION;
        }

        /* Handle special :EOF label */
        if (lstrcmpiW(L":eof", param1) == 0)
        {
            context->skip_rest = TRUE;
            return RETURN_CODE_ABORTED;
        }
        h = CreateFileW(context->batchfileW, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE)
        {
            SetLastError(ERROR_FILE_NOT_FOUND);
            WCMD_print_error();
            return ERROR_INVALID_FUNCTION;
        }

        /* Support goto :label as well as goto label plus remove trailing chars */
        if (*paramStart == ':') paramStart++;
        WCMD_set_label_end(paramStart);
        TRACE("goto label: '%s'\n", wine_dbgstr_w(paramStart));

        ret = WCMD_find_label(h, paramStart, &context->file_position);
        CloseHandle(h);
        if (ret) return RETURN_CODE_ABORTED;
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOTARGET));
        context->skip_rest = TRUE;
    }
    return ERROR_INVALID_FUNCTION;
}

/*****************************************************************************
 * WCMD_pushd
 *
 *	Push a directory onto the stack
 */

RETURN_CODE WCMD_pushd(const WCHAR *args)
{
    struct env_stack *curdir;
    WCHAR *thisdir;
    RETURN_CODE return_code;

    if (!*args)
        return errorlevel = NO_ERROR;

    if (wcschr(args, '/') != NULL) {
      SetLastError(ERROR_INVALID_PARAMETER);
      WCMD_print_error();
      return errorlevel = ERROR_INVALID_FUNCTION;
    }

    curdir  = xalloc(sizeof(struct env_stack));
    thisdir = xalloc(1024 * sizeof(WCHAR));

    /* Change directory using CD code with /D parameter */
    lstrcpyW(quals, L"/D");
    GetCurrentDirectoryW (1024, thisdir);

    return_code = WCMD_setshow_default(args);
    if (return_code != NO_ERROR)
    {
      free(curdir);
      free(thisdir);
      return errorlevel = ERROR_INVALID_FUNCTION;
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
    return errorlevel = return_code;
}


/*****************************************************************************
 * WCMD_popd
 *
 *	Pop a directory from the stack
 */

RETURN_CODE WCMD_popd(void)
{
    struct env_stack *temp = pushd_directories;

    if (!pushd_directories)
        return ERROR_INVALID_FUNCTION;

    /* pop the old environment from the stack, and make it the current dir */
    pushd_directories = temp->next;
    SetCurrentDirectoryW(temp->strings);
    free(temp->strings);
    free(temp);
    return NO_ERROR;
}

/****************************************************************************
 * WCMD_move
 *
 * Move a file, directory tree or wildcarded set of files.
 */

RETURN_CODE WCMD_move(void)
{
  WIN32_FIND_DATAW fd;
  HANDLE           hff;
  WCHAR            input[MAX_PATH];
  WCHAR            output[MAX_PATH];
  WCHAR            drive[10];
  WCHAR            dir[MAX_PATH];
  WCHAR            fname[MAX_PATH];
  WCHAR            ext[MAX_PATH];

  if (param1[0] == 0x00) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
    return errorlevel = ERROR_INVALID_FUNCTION;
  }

  /* If no destination supplied, assume current directory */
  if (param2[0] == 0x00) {
      lstrcpyW(param2, L".");
  }

  /* If 2nd parm is directory, then use original filename */
  /* Convert partial path to full path */
  if (!WCMD_get_fullpath(param1, ARRAY_SIZE(input), input, NULL) ||
      !WCMD_get_fullpath(param2, ARRAY_SIZE(output), output, NULL))
      return errorlevel = ERROR_INVALID_FUNCTION;
  WINE_TRACE("Move from '%s'('%s') to '%s'\n", wine_dbgstr_w(input),
             wine_dbgstr_w(param1), wine_dbgstr_w(output));

  /* Split into components */
  _wsplitpath(input, drive, dir, fname, ext);

  hff = FindFirstFileW(input, &fd);
  if (hff == INVALID_HANDLE_VALUE)
    return errorlevel = ERROR_INVALID_FUNCTION;

  errorlevel = NO_ERROR;
  do {
    WCHAR  dest[MAX_PATH];
    WCHAR  src[MAX_PATH];
    DWORD attribs;
    BOOL ok = TRUE;
    DWORD flags = 0;

    WINE_TRACE("Processing file '%s'\n", wine_dbgstr_w(fd.cFileName));

    /* Build src & dest name */
    lstrcpyW(src, drive);
    lstrcatW(src, dir);

    /* See if dest is an existing directory */
    attribs = GetFileAttributesW(output);
    if (attribs != INVALID_FILE_ATTRIBUTES &&
       (attribs & FILE_ATTRIBUTE_DIRECTORY)) {
      lstrcpyW(dest, output);
      lstrcatW(dest, L"\\");
      lstrcatW(dest, fd.cFileName);
    } else {
      lstrcpyW(dest, output);
    }

    lstrcatW(src, fd.cFileName);

    WINE_TRACE("Source '%s'\n", wine_dbgstr_w(src));
    WINE_TRACE("Dest   '%s'\n", wine_dbgstr_w(dest));

    /* If destination exists, prompt unless /Y supplied */
    if (GetFileAttributesW(dest) != INVALID_FILE_ATTRIBUTES) {
      BOOL force = FALSE;
      WCHAR copycmd[MAXSTRING];
      DWORD len;

      /* Default whether automatic overwriting is on. If we are interactive then
         we prompt by default, otherwise we overwrite by default
         /-Y has the highest priority, then /Y and finally the COPYCMD env. variable */
      if (wcsstr(quals, L"/-Y"))
        force = FALSE;
      else if (wcsstr(quals, L"/Y"))
        force = TRUE;
      else {
        /* By default, we will force the overwrite in batch mode and ask for
         * confirmation in interactive mode. */
        force = !interactive;
        /* If COPYCMD is set, then we force the overwrite with /Y and ask for
         * confirmation with /-Y. If COPYCMD is neither of those, then we use the
         * default behavior. */
        len = GetEnvironmentVariableW(L"COPYCMD", copycmd, ARRAY_SIZE(copycmd));
        if (len && len < ARRAY_SIZE(copycmd)) {
          if (!lstrcmpiW(copycmd, L"/Y"))
            force = TRUE;
          else if (!lstrcmpiW(copycmd, L"/-Y"))
            force = FALSE;
        }
      }

      /* Prompt if overwriting */
      if (!force) {
        WCHAR* question;

        /* Ask for confirmation */
        question = WCMD_format_string(WCMD_LoadMessage(WCMD_OVERWRITE), dest);
        ok = WCMD_ask_confirm(question, FALSE, NULL);
        LocalFree(question);
      }

      if (ok)
        flags |= MOVEFILE_REPLACE_EXISTING;
    }

    if (!ok || !MoveFileExW(src, dest, flags))
    {
      if (!ok) WCMD_print_error();
      errorlevel = ERROR_INVALID_FUNCTION;
    }
  } while (FindNextFileW(hff, &fd) != 0);

  FindClose(hff);
  return errorlevel;
}

/****************************************************************************
 * WCMD_pause
 *
 * Suspend execution of a batch script until a key is typed
 */

RETURN_CODE WCMD_pause(void)
{
  RETURN_CODE return_code = NO_ERROR;
  DWORD oldmode;
  BOOL have_console;
  DWORD count;
  WCHAR key;
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

  have_console = GetConsoleMode(hIn, &oldmode);
  if (have_console)
      SetConsoleMode(hIn, 0);

  WCMD_output_asis(anykey);
  if (!WCMD_ReadFile(hIn, &key, 1, &count) || !count)
      return_code = ERROR_INVALID_FUNCTION;
  if (have_console)
    SetConsoleMode(hIn, oldmode);
  return return_code;
}

/****************************************************************************
 * WCMD_remove_dir
 *
 * Delete a directory.
 */

RETURN_CODE WCMD_remove_dir(WCHAR *args)
{
  int   argno         = 0;
  int   argsProcessed = 0;
  WCHAR *argN         = args;

  /* Loop through all args */
  while (argN) {
    WCHAR *thisArg = WCMD_parameter (args, argno++, &argN, FALSE, FALSE);
    if (argN && argN[0] != '/') {
      WINE_TRACE("rd: Processing arg %s (quals:%s)\n", wine_dbgstr_w(thisArg),
                 wine_dbgstr_w(quals));
      argsProcessed++;

      /* If subdirectory search not supplied, just try to remove
         and report error if it fails (eg if it contains a file) */
      if (wcsstr(quals, L"/S") == NULL) {
        if (!RemoveDirectoryW(thisArg))
        {
            RETURN_CODE return_code = GetLastError();
            WCMD_print_error();
            return return_code;
        }

      /* Otherwise use ShFileOp to recursively remove a directory */
      } else {

        SHFILEOPSTRUCTW lpDir;

        /* Ask first */
        if (wcsstr(quals, L"/Q") == NULL) {
          BOOL  ok;
          WCHAR  question[MAXSTRING];

          /* Ask for confirmation */
          wsprintfW(question, L"%s ", thisArg);
          ok = WCMD_ask_confirm(question, TRUE, NULL);

          /* Abort if answer is 'N' */
          if (!ok) return ERROR_INVALID_FUNCTION;
        }

        /* Do the delete */
        lpDir.hwnd   = NULL;
        lpDir.pTo    = NULL;
        lpDir.pFrom  = thisArg;
        lpDir.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI;
        lpDir.wFunc  = FO_DELETE;

        /* SHFileOperationW needs file list with a double null termination */
        thisArg[lstrlenW(thisArg) + 1] = 0x00;

        if (SHFileOperationW(&lpDir)) WCMD_print_error ();
      }
    }
  }

  /* Handle no valid args */
  if (argsProcessed == 0) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
    return ERROR_INVALID_FUNCTION;
  }
  return NO_ERROR;
}

/****************************************************************************
 * WCMD_rename
 *
 * Rename a file.
 */

RETURN_CODE WCMD_rename(void)
{
  HANDLE           hff;
  WIN32_FIND_DATAW fd;
  WCHAR            input[MAX_PATH];
  WCHAR           *dotDst = NULL;
  WCHAR            drive[10];
  WCHAR            dir[MAX_PATH];
  WCHAR            fname[MAX_PATH];
  WCHAR            ext[MAX_PATH];

  errorlevel = NO_ERROR;

  /* Must be at least two args */
  if (param1[0] == 0x00 || param2[0] == 0x00) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
    return errorlevel = ERROR_INVALID_FUNCTION;
  }

  /* Destination cannot contain a drive letter or directory separator */
  if ((wcschr(param2,':') != NULL) || (wcschr(param2,'\\') != NULL)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      WCMD_print_error();
      return errorlevel = ERROR_INVALID_FUNCTION;
  }

  /* Convert partial path to full path */
  if (!WCMD_get_fullpath(param1, ARRAY_SIZE(input), input, NULL)) return errorlevel = ERROR_INVALID_FUNCTION;
  WINE_TRACE("Rename from '%s'('%s') to '%s'\n", wine_dbgstr_w(input),
             wine_dbgstr_w(param1), wine_dbgstr_w(param2));
  dotDst = wcschr(param2, '.');

  /* Split into components */
  _wsplitpath(input, drive, dir, fname, ext);

  hff = FindFirstFileW(input, &fd);
  if (hff == INVALID_HANDLE_VALUE)
    return errorlevel = ERROR_INVALID_FUNCTION;

 errorlevel = NO_ERROR;
 do {
    WCHAR  dest[MAX_PATH];
    WCHAR  src[MAX_PATH];
    WCHAR *dotSrc = NULL;
    int   dirLen;

    WINE_TRACE("Processing file '%s'\n", wine_dbgstr_w(fd.cFileName));

    /* FIXME: If dest name or extension is *, replace with filename/ext
       part otherwise use supplied name. This supports:
          ren *.fred *.jim
          ren jim.* fred.* etc
       However, windows has a more complex algorithm supporting eg
          ?'s and *'s mid name                                         */
    dotSrc = wcschr(fd.cFileName, '.');

    /* Build src & dest name */
    lstrcpyW(src, drive);
    lstrcatW(src, dir);
    lstrcpyW(dest, src);
    dirLen = lstrlenW(src);
    lstrcatW(src, fd.cFileName);

    /* Build name */
    if (param2[0] == '*') {
      lstrcatW(dest, fd.cFileName);
      if (dotSrc) dest[dirLen + (dotSrc - fd.cFileName)] = 0x00;
    } else {
      lstrcatW(dest, param2);
      if (dotDst) dest[dirLen + (dotDst - param2)] = 0x00;
    }

    /* Build Extension */
    if (dotDst && (*(dotDst+1)=='*')) {
      if (dotSrc) lstrcatW(dest, dotSrc);
    } else if (dotDst) {
      lstrcatW(dest, dotDst);
    }

    WINE_TRACE("Source '%s'\n", wine_dbgstr_w(src));
    WINE_TRACE("Dest   '%s'\n", wine_dbgstr_w(dest));

    if (!MoveFileW(src, dest)) {
      WCMD_print_error ();
      errorlevel = ERROR_INVALID_FUNCTION;
    }
  } while (FindNextFileW(hff, &fd) != 0);

  FindClose(hff);
  return errorlevel;
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
    len += lstrlenW(&env[len]) + 1;
  len++;

  env_copy = xalloc(len * sizeof (WCHAR));
  memcpy(env_copy, env, len*sizeof (WCHAR));

  return env_copy;
}

/*****************************************************************************
 * WCMD_setlocal
 *
 *  setlocal pushes the environment onto a stack
 *  Save the environment as unicode so we don't screw anything up.
 */
RETURN_CODE WCMD_setlocal(WCHAR *args)
{
  WCHAR *env;
  struct env_stack *env_copy;
  WCHAR cwd[MAX_PATH];
  BOOL newdelay;
  int argno = 0;
  WCHAR *argN = args;

  /* setlocal does nothing outside of batch programs */
  if (!context)
      return NO_ERROR;
  newdelay = delayedsubst;
  while (argN)
  {
      WCHAR *thisArg = WCMD_parameter (args, argno++, &argN, FALSE, FALSE);
      if (!thisArg || !*thisArg) break;
      if (!wcsicmp(thisArg, L"ENABLEDELAYEDEXPANSION"))
          newdelay = TRUE;
      else if (!wcsicmp(thisArg, L"DISABLEDELAYEDEXPANSION"))
          newdelay = FALSE;
      /* ENABLE/DISABLE EXTENSIONS ignored for now */
      else if (!wcsicmp(thisArg, L"ENABLEEXTENSIONS") || !wcsicmp(thisArg, L"DISABLEEXTENSIONS"))
      {}
      else
          return errorlevel = ERROR_INVALID_FUNCTION;
      TRACE("Setting delayed expansion to %d\n", newdelay);
  }

  env_copy = xalloc( sizeof(struct env_stack));

  env = GetEnvironmentStringsW ();
  env_copy->strings = WCMD_dupenv (env);
  if (env_copy->strings)
  {
    env_copy->context = context;
    env_copy->next = saved_environment;
    env_copy->delayedsubst = delayedsubst;
    delayedsubst = newdelay;
    saved_environment = env_copy;

    /* Save the current drive letter */
    GetCurrentDirectoryW(MAX_PATH, cwd);
    env_copy->u.cwd = cwd[0];
  }
  else
    free(env_copy);

  FreeEnvironmentStringsW (env);
  return errorlevel = NO_ERROR;
}

/*****************************************************************************
 * WCMD_endlocal
 *
 *  endlocal pops the environment off a stack
 *  Note: When searching for '=', search from WCHAR position 1, to handle
 *        special internal environment variables =C:, =D: etc
 */
RETURN_CODE WCMD_endlocal(void)
{
  WCHAR *env, *old, *p;
  struct env_stack *temp;
  int len, n;

  /* setlocal does nothing outside of batch programs */
  if (!context) return NO_ERROR;

  /* setlocal needs a saved environment from within the same context (batch
     program) as it was saved in                                            */
  if (!saved_environment || saved_environment->context != context)
    return ERROR_INVALID_FUNCTION;

  /* pop the old environment from the stack */
  temp = saved_environment;
  saved_environment = temp->next;

  /* delete the current environment, totally */
  env = GetEnvironmentStringsW ();
  old = WCMD_dupenv (env);
  len = 0;
  while (old[len]) {
    n = lstrlenW(&old[len]) + 1;
    p = wcschr(&old[len] + 1, '=');
    if (p)
    {
      *p++ = 0;
      SetEnvironmentVariableW (&old[len], NULL);
    }
    len += n;
  }
  free(old);
  FreeEnvironmentStringsW (env);

  /* restore old environment */
  env = temp->strings;
  len = 0;
  delayedsubst = temp->delayedsubst;
  WINE_TRACE("Delayed expansion now %d\n", delayedsubst);
  while (env[len]) {
    n = lstrlenW(&env[len]) + 1;
    p = wcschr(&env[len] + 1, '=');
    if (p)
    {
      *p++ = 0;
      SetEnvironmentVariableW (&env[len], p);
    }
    len += n;
  }

  /* Restore current drive letter */
  if (IsCharAlphaW(temp->u.cwd)) {
    WCHAR envvar[4];
    WCHAR cwd[MAX_PATH];

    wsprintfW(envvar, L"=%c:", temp->u.cwd);
    if (GetEnvironmentVariableW(envvar, cwd, MAX_PATH)) {
      WINE_TRACE("Resetting cwd to %s\n", wine_dbgstr_w(cwd));
      SetCurrentDirectoryW(cwd);
    }
  }

  free(env);
  free(temp);
  return NO_ERROR;
}

/*****************************************************************************
 * WCMD_setshow_default
 *
 *	Set/Show the current default directory
 */

RETURN_CODE WCMD_setshow_default(const WCHAR *args)
{
  RETURN_CODE return_code;
  BOOL status;
  WCHAR string[1024];
  WCHAR cwd[1024];
  WCHAR *pos;
  WIN32_FIND_DATAW fd;
  HANDLE hff;

  WINE_TRACE("Request change to directory '%s'\n", wine_dbgstr_w(args));

  /* Skip /D and trailing whitespace if on the front of the command line */
  if (lstrlenW(args) >= 2 &&
      CompareStringW(LOCALE_USER_DEFAULT,
                     NORM_IGNORECASE | SORT_STRINGSORT,
                     args, 2, L"/D", -1) == CSTR_EQUAL) {
    args += 2;
    while (*args && (*args==' ' || *args=='\t'))
      args++;
  }

  GetCurrentDirectoryW(ARRAY_SIZE(cwd), cwd);
  return_code = NO_ERROR;

  if (!*args) {
    lstrcatW(cwd, L"\r\n");
    WCMD_output_asis (cwd);
  }
  else {
    /* Remove any double quotes, which may be in the
       middle, eg. cd "C:\Program Files"\Microsoft is ok */
    pos = string;
    while (*args) {
      if (*args != '"') *pos++ = *args;
      args++;
    }
    while (pos > string && (*(pos-1) == ' ' || *(pos-1) == '\t'))
      pos--;
    *pos = 0x00;

    /* Search for appropriate directory */
    WINE_TRACE("Looking for directory '%s'\n", wine_dbgstr_w(string));
    hff = FindFirstFileW(string, &fd);
    if (hff != INVALID_HANDLE_VALUE) {
      do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          WCHAR fpath[MAX_PATH];
          WCHAR drive[10];
          WCHAR dir[MAX_PATH];
          WCHAR fname[MAX_PATH];
          WCHAR ext[MAX_PATH];

          /* Convert path into actual directory spec */
          if (!WCMD_get_fullpath(string, ARRAY_SIZE(fpath), fpath, NULL))
              return errorlevel = ERROR_INVALID_FUNCTION;

          _wsplitpath(fpath, drive, dir, fname, ext);

          /* Rebuild path */
          wsprintfW(string, L"%s%s%s", drive, dir, fd.cFileName);
          break;
        }
      } while (FindNextFileW(hff, &fd) != 0);
      FindClose(hff);
    }

    /* Change to that directory */
    WINE_TRACE("Really changing to directory '%s'\n", wine_dbgstr_w(string));

    status = SetCurrentDirectoryW(string);
    if (!status) {
      WCMD_print_error ();
      return_code = ERROR_INVALID_FUNCTION;
    } else {

      /* Save away the actual new directory, to store as current location */
      GetCurrentDirectoryW(ARRAY_SIZE(string), string);

      /* Restore old directory if drive letter would change, and
           CD x:\directory /D (or pushd c:\directory) not supplied */
      if ((wcsstr(quals, L"/D") == NULL) &&
          (param1[1] == ':') && (towupper(param1[0]) != towupper(cwd[0]))) {
        SetCurrentDirectoryW(cwd);
      }
    }

    /* Set special =C: type environment variable, for drive letter of
       change of directory, even if path was restored due to missing
       /D (allows changing drive letter when not resident on that
       drive                                                          */
    if ((string[1] == ':') && IsCharAlphaW(string[0])) {
      WCHAR env[4];
      lstrcpyW(env, L"=");
      memcpy(env+1, string, 2 * sizeof(WCHAR));
      env[3] = 0x00;
      WINE_TRACE("Setting '%s' to '%s'\n", wine_dbgstr_w(env), wine_dbgstr_w(string));
      SetEnvironmentVariableW(env, string);
    }

  }
  return errorlevel = return_code;
}

/****************************************************************************
 * WCMD_setshow_date
 *
 * Set/Show the system date
 * FIXME: Can't change date yet
 */

RETURN_CODE WCMD_setshow_date(void)
{
  RETURN_CODE return_code = NO_ERROR;
  WCHAR curdate[64], buffer[64];
  DWORD count;

  if (!*param1) {
    if (GetDateFormatW(LOCALE_USER_DEFAULT, 0, NULL, NULL, curdate, ARRAY_SIZE(curdate))) {
      WCMD_output (WCMD_LoadMessage(WCMD_CURRENTDATE), curdate);
      if (wcsstr(quals, L"/T") == NULL) {
        WCMD_output (WCMD_LoadMessage(WCMD_NEWDATE));
        if (WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, ARRAY_SIZE(buffer), &count) &&
            count > 2) {
          WCMD_output_stderr (WCMD_LoadMessage(WCMD_NYI));
        }
      }
    }
    else WCMD_print_error ();
  }
  else {
    return_code = ERROR_INVALID_FUNCTION;
    WCMD_output_stderr (WCMD_LoadMessage(WCMD_NYI));
  }
  return errorlevel = return_code;
}

/****************************************************************************
 * WCMD_compare
 * Note: Native displays 'fred' before 'fred ', so need to only compare up to
 *       the equals sign.
 */
static int __cdecl WCMD_compare( const void *a, const void *b )
{
    int r;
    const WCHAR * const *str_a = a, * const *str_b = b;
    r = CompareStringW( LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
	  *str_a, wcscspn(*str_a, L"="), *str_b, wcscspn(*str_b, L"=") );
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

  if (stub) stublen = lstrlenW(stub);

  /* count the number of strings, and the total length */
  while ( s[len] ) {
    len += lstrlenW(&s[len]) + 1;
    count++;
  }

  /* add the strings to an array */
  str = xalloc(count * sizeof (WCHAR*) );
  str[0] = s;
  for( i=1; i<count; i++ )
    str[i] = str[i-1] + lstrlenW(str[i-1]) + 1;

  /* sort the array */
  qsort( str, count, sizeof (WCHAR*), WCMD_compare );

  /* print it */
  for( i=0; i<count; i++ ) {
    if (!stub || CompareStringW(LOCALE_USER_DEFAULT,
                                NORM_IGNORECASE | SORT_STRINGSORT,
                                str[i], stublen, stub, -1) == CSTR_EQUAL) {
      /* Don't display special internal variables */
      if (str[i][0] != '=') {
        WCMD_output_asis(str[i]);
        WCMD_output_asis(L"\r\n");
        displayedcount++;
      }
    }
  }

  free( str );
  return displayedcount;
}

/****************************************************************************
 * WCMD_getprecedence
 * Return the precedence of a particular operator
 */
static int WCMD_getprecedence(const WCHAR in)
{
  switch (in) {
    case '!':
    case '~':
    case OP_POSITIVE:
    case OP_NEGATIVE:
      return 8;
    case '*':
    case '/':
    case '%':
      return 7;
    case '+':
    case '-':
      return 6;
    case '<':
    case '>':
      return 5;
    case '&':
      return 4;
    case '^':
      return 3;
    case '|':
      return 2;
    case '=':
    case OP_ASSSIGNMUL:
    case OP_ASSSIGNDIV:
    case OP_ASSSIGNMOD:
    case OP_ASSSIGNADD:
    case OP_ASSSIGNSUB:
    case OP_ASSSIGNAND:
    case OP_ASSSIGNNOT:
    case OP_ASSSIGNOR:
    case OP_ASSSIGNSHL:
    case OP_ASSSIGNSHR:
      return 1;
    default:
      return 0;
  }
}

/****************************************************************************
 * WCMD_pushnumber
 * Push either a number or name (environment variable) onto the supplied
 * stack
 */
static void WCMD_pushnumber(WCHAR *var, int num, VARSTACK **varstack) {
  VARSTACK *thisstack = xalloc(sizeof(VARSTACK));
  thisstack->isnum = (var == NULL);
  if (var) {
    thisstack->variable = var;
    WINE_TRACE("Pushed variable %s\n", wine_dbgstr_w(var));
  } else {
    thisstack->value = num;
    WINE_TRACE("Pushed number %d\n", num);
  }
  thisstack->next = *varstack;
  *varstack = thisstack;
}

/****************************************************************************
 * WCMD_peeknumber
 * Returns the value of the top number or environment variable on the stack
 * and leaves the item on the stack.
 */
static int WCMD_peeknumber(VARSTACK **varstack) {
  int result = 0;
  VARSTACK *thisvar;

  if (varstack) {
    thisvar = *varstack;
    if (!thisvar->isnum) {
      WCHAR tmpstr[MAXSTRING];
      if (GetEnvironmentVariableW(thisvar->variable, tmpstr, MAXSTRING)) {
        result = wcstol(tmpstr,NULL,0);
      }
      WINE_TRACE("Envvar %s converted to %d\n", wine_dbgstr_w(thisvar->variable), result);
    } else {
      result = thisvar->value;
    }
  }
  WINE_TRACE("Peeked number %d\n", result);
  return result;
}

/****************************************************************************
 * WCMD_popnumber
 * Returns the value of the top number or environment variable on the stack
 * and removes the item from the stack.
 */
static int WCMD_popnumber(VARSTACK **varstack) {
  int result = 0;
  VARSTACK *thisvar;

  if (varstack) {
    thisvar = *varstack;
    result = WCMD_peeknumber(varstack);
    if (!thisvar->isnum) free(thisvar->variable);
    *varstack = thisvar->next;
    free(thisvar);
  }
  WINE_TRACE("Popped number %d\n", result);
  return result;
}

/****************************************************************************
 * WCMD_pushoperator
 * Push an operator onto the supplied stack
 */
static void WCMD_pushoperator(WCHAR op, int precedence, OPSTACK **opstack) {
  OPSTACK *thisstack = xalloc(sizeof(OPSTACK));
  thisstack->precedence = precedence;
  thisstack->op = op;
  thisstack->next = *opstack;
  WINE_TRACE("Pushed operator %c\n", op);
  *opstack = thisstack;
}

/****************************************************************************
 * WCMD_popoperator
 * Returns the operator from the top of the stack and removes the item from
 * the stack.
 */
static WCHAR WCMD_popoperator(OPSTACK **opstack) {
  WCHAR result = 0;
  OPSTACK *thisop;

  if (opstack) {
    thisop = *opstack;
    result = thisop->op;
    *opstack = thisop->next;
    free(thisop);
  }
  WINE_TRACE("Popped operator %c\n", result);
  return result;
}

/****************************************************************************
 * WCMD_reduce
 * Actions the top operator on the stack against the first and sometimes
 * second value on the variable stack, and pushes the result
 * Returns non-zero on error.
 */
static int WCMD_reduce(OPSTACK **opstack, VARSTACK **varstack) {
  WCHAR thisop;
  int var1,var2;
  int rc = 0;

  if (!*opstack || !*varstack) {
    WINE_TRACE("No operators for the reduce\n");
    return WCMD_NOOPERATOR;
  }

  /* Remove the top operator */
  thisop = WCMD_popoperator(opstack);
  WINE_TRACE("Reducing the stacks - processing operator %c\n", thisop);

  /* One variable operators */
  var1 = WCMD_popnumber(varstack);
  switch (thisop) {
  case '!': WCMD_pushnumber(NULL, !var1, varstack);
            break;
  case '~': WCMD_pushnumber(NULL, ~var1, varstack);
            break;
  case OP_POSITIVE: WCMD_pushnumber(NULL, var1, varstack);
            break;
  case OP_NEGATIVE: WCMD_pushnumber(NULL, -var1, varstack);
            break;
  }

  /* Two variable operators */
  if (!*varstack) {
    WINE_TRACE("No operands left for the reduce?\n");
    return WCMD_NOOPERAND;
  }
  switch (thisop) {
  case '!':
  case '~':
  case OP_POSITIVE:
  case OP_NEGATIVE:
            break; /* Handled above */
  case '*': var2 = WCMD_popnumber(varstack);
            WCMD_pushnumber(NULL, var2*var1, varstack);
            break;
  case '/': var2 = WCMD_popnumber(varstack);
            if (var1 == 0) return WCMD_DIVIDEBYZERO;
            WCMD_pushnumber(NULL, var2/var1, varstack);
            break;
  case '+': var2 = WCMD_popnumber(varstack);
            WCMD_pushnumber(NULL, var2+var1, varstack);
            break;
  case '-': var2 = WCMD_popnumber(varstack);
            WCMD_pushnumber(NULL, var2-var1, varstack);
            break;
  case '&': var2 = WCMD_popnumber(varstack);
            WCMD_pushnumber(NULL, var2&var1, varstack);
            break;
  case '%': var2 = WCMD_popnumber(varstack);
            if (var1 == 0) return WCMD_DIVIDEBYZERO;
            WCMD_pushnumber(NULL, var2%var1, varstack);
            break;
  case '^': var2 = WCMD_popnumber(varstack);
            WCMD_pushnumber(NULL, var2^var1, varstack);
            break;
  case '<': var2 = WCMD_popnumber(varstack);
            /* Shift left has to be a positive number, 0-31 otherwise 0 is returned,
               which differs from the compiler (for example gcc) so being explicit. */
            if (var1 < 0 || var1 >= (8 * sizeof(INT))) {
              WCMD_pushnumber(NULL, 0, varstack);
            } else {
              WCMD_pushnumber(NULL, var2<<var1, varstack);
            }
            break;
  case '>': var2 = WCMD_popnumber(varstack);
            WCMD_pushnumber(NULL, var2>>var1, varstack);
            break;
  case '|': var2 = WCMD_popnumber(varstack);
            WCMD_pushnumber(NULL, var2|var1, varstack);
            break;

  case OP_ASSSIGNMUL:
  case OP_ASSSIGNDIV:
  case OP_ASSSIGNMOD:
  case OP_ASSSIGNADD:
  case OP_ASSSIGNSUB:
  case OP_ASSSIGNAND:
  case OP_ASSSIGNNOT:
  case OP_ASSSIGNOR:
  case OP_ASSSIGNSHL:
  case OP_ASSSIGNSHR:
        {
          int i = 0;

          /* The left of an equals must be one variable */
          if (!(*varstack) || (*varstack)->isnum) {
            return WCMD_NOOPERAND;
          }

          /* Make the number stack grow by inserting the value of the variable */
          var2 = WCMD_peeknumber(varstack);
          WCMD_pushnumber(NULL, var2, varstack);
          WCMD_pushnumber(NULL, var1, varstack);

          /* Make the operand stack grow by pushing the assign operator plus the
             operator to perform                                                 */
          while (calcassignments[i].op != ' ' &&
                 calcassignments[i].calculatedop != thisop) {
            i++;
          }
          if (calcassignments[i].calculatedop == ' ') {
            WINE_ERR("Unexpected operator %c\n", thisop);
            return WCMD_NOOPERATOR;
          }
          WCMD_pushoperator('=', WCMD_getprecedence('='), opstack);
          WCMD_pushoperator(calcassignments[i].op,
                            WCMD_getprecedence(calcassignments[i].op), opstack);
          break;
        }

  case '=':
        {
          WCHAR  result[MAXSTRING];

          /* Build the result, then push it onto the stack */
          swprintf(result, ARRAY_SIZE(result), L"%d", var1);
          WINE_TRACE("Assigning %s a value %s\n", wine_dbgstr_w((*varstack)->variable),
                     wine_dbgstr_w(result));
          SetEnvironmentVariableW((*varstack)->variable, result);
          var2 = WCMD_popnumber(varstack);
          WCMD_pushnumber(NULL, var1, varstack);
          break;
        }

  default:  WINE_ERR("Unrecognized operator %c\n", thisop);
  }

  return rc;
}


/****************************************************************************
 * WCMD_handleExpression
 * Handles an expression provided to set /a - If it finds brackets, it uses
 * recursion to process the parts in brackets.
 */
static int WCMD_handleExpression(WCHAR **expr, int *ret, int depth)
{
  static const WCHAR mathDelims[] = L" \t()!~-*/%+<>&^|=,";
  int       rc = 0;
  WCHAR    *pos;
  BOOL      lastwasnumber = FALSE;  /* FALSE makes a minus at the start of the expression easier to handle */
  OPSTACK  *opstackhead = NULL;
  VARSTACK *varstackhead = NULL;
  WCHAR     foundhalf = 0;

  /* Initialize */
  WINE_TRACE("Handling expression '%s'\n", wine_dbgstr_w(*expr));
  pos = *expr;

  /* Iterate through until whole expression is processed */
  while (pos && *pos) {
    BOOL treatasnumber;

    /* Skip whitespace to get to the next character to process*/
    while (*pos && (*pos==' ' || *pos=='\t')) pos++;
    if (!*pos) goto exprreturn;

    /* If we have found anything other than an operator then it's a number/variable */
    if (wcschr(mathDelims, *pos) == NULL) {
      WCHAR *parmstart, *parm, *dupparm;
      WCHAR *nextpos;

      /* Cannot have an expression with var/number twice, without an operator
         in-between, nor or number following a half constructed << or >> operator */
      if (lastwasnumber || foundhalf) {
        rc = WCMD_NOOPERATOR;
        goto exprerrorreturn;
      }
      lastwasnumber = TRUE;

      if (iswdigit(*pos)) {
        /* For a number - just push it onto the stack */
        int num = wcstoul(pos, &nextpos, 0);
        WCMD_pushnumber(NULL, num, &varstackhead);
        pos = nextpos;

        /* Verify the number was validly formed */
        if (*nextpos && (wcschr(mathDelims, *nextpos) == NULL)) {
          rc = WCMD_BADHEXOCT;
          goto exprerrorreturn;
        }
      } else {

        /* For a variable - just push it onto the stack */
        parm = WCMD_parameter_with_delims(pos, 0, &parmstart, FALSE, FALSE, mathDelims);
        dupparm = xstrdupW(parm);
        WCMD_pushnumber(dupparm, 0, &varstackhead);
        pos = parmstart + lstrlenW(dupparm);
      }
      continue;
    }

    /* We have found an operator. Some operators are one character, some two, and the minus
       and plus signs need special processing as they can be either operators or just influence
       the parameter which follows them                                                         */
    if (foundhalf && (*pos != foundhalf)) {
      /* Badly constructed operator pair */
      rc = WCMD_NOOPERATOR;
      goto exprerrorreturn;
    }

    treatasnumber = FALSE; /* We are processing an operand */
    switch (*pos) {

    /* > and < are special as they are double character operators (and spaces can be between them!)
       If we see these for the first time, set a flag, and second time around we continue.
       Note these double character operators are stored as just one of the characters on the stack */
    case '>':
    case '<': if (!foundhalf) {
                foundhalf = *pos;
                pos++;
                break;
              }
              /* We have found the rest, so clear up the knowledge of the half completed part and
                 drop through to normal operator processing                                       */
              foundhalf = 0;
              /* drop through */

    case '=': if (*pos=='=') {
                /* = is special cased as if the last was an operator then we may have e.g. += or
                   *= etc which we need to handle by replacing the operator that is on the stack
                   with a calculated assignment equivalent                                       */
                if (!lastwasnumber && opstackhead) {
                  int i = 0;
                  while (calcassignments[i].op != ' ' && calcassignments[i].op != opstackhead->op) {
                    i++;
                  }
                  if (calcassignments[i].op == ' ') {
                    rc = WCMD_NOOPERAND;
                    goto exprerrorreturn;
                  } else {
                    /* Remove the operator on the stack, it will be replaced with a ?= equivalent
                       when the general operator handling happens further down.                   */
                    *pos = calcassignments[i].calculatedop;
                    WCMD_popoperator(&opstackhead);
                  }
                }
              }
              /* Drop though */

    /* + and - are slightly special as they can be a numeric prefix, if they follow an operator
       so if they do, convert the +/- (arithmetic) to +/- (numeric prefix for positive/negative) */
    case '+': if (!lastwasnumber && *pos=='+') *pos = OP_POSITIVE;
              /* drop through */
    case '-': if (!lastwasnumber && *pos=='-') *pos = OP_NEGATIVE;
              /* drop through */

    /* Normal operators - push onto stack unless precedence means we have to calculate it now */
    case '!': /* drop through */
    case '~': /* drop through */
    case '/': /* drop through */
    case '%': /* drop through */
    case '&': /* drop through */
    case '^': /* drop through */
    case '*': /* drop through */
    case '|':
               /* General code for handling most of the operators - look at the
                  precedence of the top item on the stack, and see if we need to
                  action the stack before we push something else onto it.        */
               {
                 int precedence = WCMD_getprecedence(*pos);
                 WINE_TRACE("Found operator %c precedence %d (head is %d)\n", *pos,
                            precedence, !opstackhead?-1:opstackhead->precedence);

                 /* In general, for things with the same precedence, reduce immediately
                    except for assignments and unary operators which do not             */
                 while (!rc && opstackhead &&
                        ((opstackhead->precedence > precedence) ||
                         ((opstackhead->precedence == precedence) &&
                            (precedence != 1) && (precedence != 8)))) {
                   rc = WCMD_reduce(&opstackhead, &varstackhead);
                 }
                 if (rc) goto exprerrorreturn;
                 WCMD_pushoperator(*pos, precedence, &opstackhead);
                 pos++;
                 break;
               }

    /* comma means start a new expression, ie calculate what we have */
    case ',':
               {
                 int prevresult = -1;
                 WINE_TRACE("Found expression delimiter - reducing existing stacks\n");
                 while (!rc && opstackhead) {
                   rc = WCMD_reduce(&opstackhead, &varstackhead);
                 }
                 if (rc) goto exprerrorreturn;
                 /* If we have anything other than one number left, error
                    otherwise throw the number away                      */
                 if (!varstackhead || varstackhead->next) {
                   rc = WCMD_NOOPERATOR;
                   goto exprerrorreturn;
                 }
                 prevresult = WCMD_popnumber(&varstackhead);
                 WINE_TRACE("Expression resolved to %d\n", prevresult);
                 free(varstackhead);
                 varstackhead = NULL;
                 pos++;
                 break;
               }

    /* Open bracket - use iteration to parse the inner expression, then continue */
    case '(' : {
                 int exprresult = 0;
                 pos++;
                 rc = WCMD_handleExpression(&pos, &exprresult, depth+1);
                 if (rc) goto exprerrorreturn;
                 WCMD_pushnumber(NULL, exprresult, &varstackhead);
                 break;
               }

    /* Close bracket - we have finished this depth, calculate and return */
    case ')' : {
                 pos++;
                 treatasnumber = TRUE; /* Things in brackets result in a number */
                 if (depth == 0) {
                   rc = WCMD_BADPAREN;
                   goto exprerrorreturn;
                 }
                 goto exprreturn;
               }

    default:
        WINE_ERR("Unrecognized operator %c\n", *pos);
        pos++;
    }
    lastwasnumber = treatasnumber;
  }

exprreturn:
  *expr = pos;

  /* We need to reduce until we have a single number (or variable) on the
     stack and set the return value to that                               */
  while (!rc && opstackhead) {
    rc = WCMD_reduce(&opstackhead, &varstackhead);
  }
  if (rc) goto exprerrorreturn;

  /* If we have anything other than one number left, error
      otherwise throw the number away                      */
  if (!varstackhead || varstackhead->next) {
    rc = WCMD_NOOPERATOR;
    goto exprerrorreturn;
  }

  /* Now get the number (and convert if it's just a variable name) */
  *ret = WCMD_popnumber(&varstackhead);

exprerrorreturn:
  /* Free all remaining memory */
  while (opstackhead) WCMD_popoperator(&opstackhead);
  while (varstackhead) WCMD_popnumber(&varstackhead);

  WINE_TRACE("Returning result %d, rc %d\n", *ret, rc);
  return rc;
}

/****************************************************************************
 * WCMD_setshow_env
 *
 * Set/Show the environment variables
 */

RETURN_CODE WCMD_setshow_env(WCHAR *s)
{
  RETURN_CODE return_code = NO_ERROR;
  WCHAR *p;
  BOOL status;
  WCHAR string[MAXSTRING];

  if (!*s) {
    WCHAR *env = GetEnvironmentStringsW();
    WCMD_setshow_sortenv( env, NULL );
    FreeEnvironmentStringsW(env);
  }

  /* See if /P supplied, and if so echo the prompt, and read in a reply */
  else if (CompareStringW(LOCALE_USER_DEFAULT,
                          NORM_IGNORECASE | SORT_STRINGSORT,
                          s, 2, L"/P", -1) == CSTR_EQUAL) {
    DWORD count;

    s += 2;
    while (*s && (*s==' ' || *s=='\t')) s++;
    /* set /P "var=value"jim ignores anything after the last quote */
    if (*s=='\"') {
      WCHAR *lastquote;
      lastquote = WCMD_strip_quotes(s);
      if (lastquote) *lastquote = 0x00;
      WINE_TRACE("set: Stripped command line '%s'\n", wine_dbgstr_w(s));
    }

    /* If no parameter, or no '=' sign, return an error */
    if (!(*s) || ((p = wcschr(s, '=')) == NULL )) {
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
      return_code = ERROR_INVALID_FUNCTION;
    }
    else
    {
      /* Output the prompt */
      *p++ = '\0';
      if (*p) {
        if (*p == L'"') {
          WCHAR* last = wcsrchr(p+1, L'"');
          p++;
          if (last) *last = L'\0';
        }
        WCMD_output_asis(p);
      }

      /* Read the reply */
      if (WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), string, ARRAY_SIZE(string), &count) && count > 1) {
        string[count-1] = '\0'; /* ReadFile output is not null-terminated! */
        if (string[count-2] == '\r') string[count-2] = '\0'; /* Under Windoze we get CRLF! */
        TRACE("set /p: Setting var '%s' to '%s'\n", wine_dbgstr_w(s),
              wine_dbgstr_w(string));
        if (*string) SetEnvironmentVariableW(s, string);
      }
    }

  /* See if /A supplied, and if so calculate the results of all the expressions */
  } else if (CompareStringW(LOCALE_USER_DEFAULT,
                            NORM_IGNORECASE | SORT_STRINGSORT,
                            s, 2, L"/A", -1) == CSTR_EQUAL) {
    /* /A supplied, so evaluate expressions and set variables appropriately */
    /* Syntax is set /a var=1,var2=var+4 etc, and it echos back the result  */
    /* of the final computation                                             */
    int result = 0;
    int rc = 0;
    WCHAR *thisexpr;
    WCHAR *src,*dst;

    /* Remove all quotes before doing any calculations */
    thisexpr = xalloc((wcslen(s + 2) + 1) * sizeof(WCHAR));
    src = s+2;
    dst = thisexpr;
    while (*src) {
      if (*src != '"') *dst++ = *src;
      src++;
    }
    *dst = 0;

    /* Now calculate the results of the expression */
    src = thisexpr;
    rc = WCMD_handleExpression(&src, &result, 0);
    free(thisexpr);

    /* If parsing failed, issue the error message */
    if (rc > 0) {
      WCMD_output_stderr(WCMD_LoadMessage(rc));
      return_code = ERROR_INVALID_FUNCTION;
    }
    /* If we have no context (interactive or cmd.exe /c) print the final result */
    else if (!context) {
      swprintf(string, ARRAY_SIZE(string), L"%d", result);
      WCMD_output_asis(string);
    }

  } else {
    DWORD gle;

    /* set "var=value"jim ignores anything after the last quote */
    if (*s=='\"') {
      WCHAR *lastquote;
      lastquote = WCMD_strip_quotes(s);
      if (lastquote) *lastquote = 0x00;
      WINE_TRACE("set: Stripped command line '%s'\n", wine_dbgstr_w(s));
    }

    p = wcschr (s, '=');
    if (p == NULL) {
      WCHAR *env = GetEnvironmentStringsW();
      if (WCMD_setshow_sortenv( env, s ) == 0) {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_MISSINGENV), s);
        return_code = ERROR_INVALID_FUNCTION;
      }
      FreeEnvironmentStringsW(env);
    }
    else
    {
      *p++ = '\0';

      if (!*p) p = NULL;
      TRACE("set: Setting var '%s' to '%s'\n", wine_dbgstr_w(s),
            wine_dbgstr_w(p));
      status = SetEnvironmentVariableW(s, p);
      gle = GetLastError();
      if ((!status) & (gle == ERROR_ENVVAR_NOT_FOUND)) {
          return_code = ERROR_INVALID_FUNCTION;
      } else if (!status) WCMD_print_error();
    }
  }
  return WCMD_is_in_context(L".bat") && return_code == NO_ERROR ?
      return_code : (errorlevel = return_code);
}

/****************************************************************************
 * WCMD_setshow_path
 *
 * Set/Show the path environment variable
 */

RETURN_CODE WCMD_setshow_path(const WCHAR *args)
{
  WCHAR string[1024];

  if (!*param1 && !*param2) {
    if (!GetEnvironmentVariableW(L"PATH", string, ARRAY_SIZE(string)))
      wcscpy(string, L"(null)");
    WCMD_output_asis(L"PATH=");
    WCMD_output_asis(string);
    WCMD_output_asis(L"\r\n");
  }
  else {
    if (*args == '=') args++; /* Skip leading '=' */
    if (args[0] == L';' && *WCMD_skip_leading_spaces((WCHAR *)(args + 1)) == L'\0') args = NULL;
    if (!SetEnvironmentVariableW(L"PATH", args))
    {
        WCMD_print_error();
        return errorlevel = ERROR_INVALID_FUNCTION;
    }
  }
  return WCMD_is_in_context(L".bat") ? NO_ERROR : (errorlevel = NO_ERROR);
}

/****************************************************************************
 * WCMD_setshow_prompt
 *
 * Set or show the command prompt.
 */

RETURN_CODE WCMD_setshow_prompt(void)
{

  WCHAR *s;

  if (!*param1) {
    SetEnvironmentVariableW(L"PROMPT", NULL);
  }
  else {
    s = param1;
    while ((*s == '=') || (*s == ' ') || (*s == '\t')) s++;
    if (!*s) {
      SetEnvironmentVariableW(L"PROMPT", NULL);
    }
    else SetEnvironmentVariableW(L"PROMPT", s);
  }
  return WCMD_is_in_context(L".bat") ? NO_ERROR : (errorlevel = NO_ERROR);
}

/****************************************************************************
 * WCMD_setshow_time
 *
 * Set/Show the system time
 * FIXME: Can't change time yet
 */

RETURN_CODE WCMD_setshow_time(void)
{
  RETURN_CODE return_code = NO_ERROR;
  WCHAR curtime[64], buffer[64];
  DWORD count;
  SYSTEMTIME st;

  if (!*param1) {
    GetLocalTime(&st);
    if (GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &st, NULL, curtime, ARRAY_SIZE(curtime))) {
      WCMD_output (WCMD_LoadMessage(WCMD_CURRENTTIME), curtime);
      if (wcsstr(quals, L"/T") == NULL) {
        WCMD_output (WCMD_LoadMessage(WCMD_NEWTIME));
        if (WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, ARRAY_SIZE(buffer), &count) &&
            count > 2) {
          WCMD_output_stderr (WCMD_LoadMessage(WCMD_NYI));
        }
      }
    }
    else WCMD_print_error ();
  }
  else {
    return_code = ERROR_INVALID_FUNCTION;
    WCMD_output_stderr (WCMD_LoadMessage(WCMD_NYI));
  }
  return errorlevel = return_code;
}

/****************************************************************************
 * WCMD_shift
 *
 * Shift batch parameters.
 * Optional /n says where to start shifting (n=0-8)
 */

RETURN_CODE WCMD_shift(const WCHAR *args)
{
  int start;

  if (context != NULL) {
    WCHAR *pos = wcschr(args, '/');
    int   i;

    if (pos == NULL) {
      start = 0;
    } else if (*(pos+1)>='0' && *(pos+1)<='8') {
      start = (*(pos+1) - '0');
    } else {
      SetLastError(ERROR_INVALID_PARAMETER);
      WCMD_print_error();
      return errorlevel = ERROR_INVALID_FUNCTION;
    }

    WINE_TRACE("Shifting variables, starting at %d\n", start);
    for (i=start;i<=8;i++) {
      context -> shift_count[i] = context -> shift_count[i+1] + 1;
    }
    context -> shift_count[9] = context -> shift_count[9] + 1;
  }
  return NO_ERROR;
}

/****************************************************************************
 * WCMD_start
 */
RETURN_CODE WCMD_start(WCHAR *args)
{
    RETURN_CODE return_code = NO_ERROR;
    int argno;
    int have_title;
    WCHAR file[MAX_PATH];
    WCHAR *cmdline, *cmdline_params;
    STARTUPINFOW st;
    PROCESS_INFORMATION pi;

    GetSystemDirectoryW( file, MAX_PATH );
    lstrcatW(file, L"\\start.exe");
    cmdline = xalloc( (wcslen(file) + wcslen(args) + 8) * sizeof(WCHAR) );
    lstrcpyW( cmdline, file );
    lstrcatW(cmdline, L" ");
    cmdline_params = cmdline + lstrlenW(cmdline);

    /* The start built-in has some special command-line parsing properties
     * which will be outlined here.
     *
     * both '\t' and ' ' are argument separators
     * '/' has a special double role as both separator and switch prefix, e.g.
     *
     * > start /low/i
     * or
     * > start "title"/i
     *
     * are valid ways to pass multiple options to start. In the latter case
     * '/i' is not a part of the title but parsed as a switch.
     *
     * However, '=', ';' and ',' are not separators:
     * > start "deus"=ex,machina
     *
     * will in fact open a console titled 'deus=ex,machina'
     *
     * The title argument parsing code is only interested in quotes themselves,
     * it does not respect escaping of any kind and all quotes are dropped
     * from the resulting title, therefore:
     *
     * > start "\"" hello"/low
     *
     * actually opens a console titled '\ hello' with low priorities.
     *
     * To not break compatibility with wine programs relying on
     * wine's separate 'start.exe', this program's peculiar console
     * title parsing is actually implemented in 'cmd.exe' which is the
     * application native Windows programs will use to invoke 'start'.
     *
     * WCMD_parameter_with_delims will take care of everything for us.
     */
    /* FIXME: using an external start.exe has several caveats:
     * - cannot discriminate syntax error in arguments from child's return code
     * - need to access start.exe's child to get its running state
     *   (not start.exe itself)
     */
    have_title = FALSE;
    for (argno=0; ; argno++) {
        WCHAR *thisArg, *argN;

        argN = NULL;
        thisArg = WCMD_parameter_with_delims(args, argno, &argN, FALSE, FALSE, L" \t/");

        /* No more parameters */
        if (!argN)
            break;

        /* Found the title */
        if (argN[0] == '"') {
            TRACE("detected console title: %s\n", wine_dbgstr_w(thisArg));
            have_title = TRUE;

            /* Copy all of the cmdline processed */
            memcpy(cmdline_params, args, sizeof(WCHAR) * (argN - args));
            cmdline_params[argN - args] = '\0';

            /* Add quoted title */
            lstrcatW(cmdline_params, L"\"\\\"");
            lstrcatW(cmdline_params, thisArg);
            lstrcatW(cmdline_params, L"\\\"\"");

            /* Concatenate remaining command-line */
            thisArg = WCMD_parameter_with_delims(args, argno, &argN, TRUE, FALSE, L" \t/");
            lstrcatW(cmdline_params, argN + lstrlenW(thisArg));

            break;
        }

        /* Skipping a regular argument? */
        else if (argN != args && argN[-1] == '/') {
            continue;

        /* Not an argument nor the title, start of program arguments,
         * stop looking for title.
         */
        } else
            break;
    }

    /* build command-line if not built yet */
    if (!have_title) {
        lstrcatW( cmdline, args );
    }

    memset( &st, 0, sizeof(STARTUPINFOW) );
    st.cb = sizeof(STARTUPINFOW);

    if (CreateProcessW( file, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &st, &pi ))
    {
        DWORD exit_code;
        WaitForSingleObject( pi.hProcess, INFINITE );
        GetExitCodeProcess( pi.hProcess, &exit_code );
        errorlevel = (exit_code == STILL_ACTIVE) ? NO_ERROR : exit_code;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        WCMD_print_error ();
        return_code = errorlevel = ERROR_INVALID_FUNCTION;
    }
    free(cmdline);
    return return_code;
}

/****************************************************************************
 * WCMD_title
 *
 * Set the console title
 */
RETURN_CODE WCMD_title(const WCHAR *args)
{
    SetConsoleTitleW(args);
    return NO_ERROR;
}

/****************************************************************************
 * WCMD_type
 *
 * Copy a file to standard output.
 */

RETURN_CODE WCMD_type(WCHAR *args)
{
  RETURN_CODE return_code;
  int   argno         = 0;
  WCHAR *argN          = args;
  BOOL  writeHeaders  = FALSE;

  if (param1[0] == 0x00) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
    return errorlevel = ERROR_INVALID_FUNCTION;
  }

  if (param2[0] != 0x00) writeHeaders = TRUE;

  /* Loop through all args */
  return_code = NO_ERROR;
  while (argN) {
    WCHAR *thisArg = WCMD_parameter (args, argno++, &argN, FALSE, FALSE);

    HANDLE h;
    WCHAR buffer[512];
    DWORD count;

    if (!argN) break;

    WINE_TRACE("type: Processing arg '%s'\n", wine_dbgstr_w(thisArg));
    h = CreateFileW(thisArg, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
      WCMD_print_error ();
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_READFAIL), thisArg);
      return errorlevel = ERROR_INVALID_FUNCTION;
    } else {
      if (writeHeaders) {
        WCMD_output_stderr(L"\n%1\n\n\n", thisArg);
      }
      while (WCMD_ReadFile(h, buffer, ARRAY_SIZE(buffer) - 1, &count)) {
        if (count == 0) break;	/* ReadFile reports success on EOF! */
        buffer[count] = 0;
        WCMD_output_asis (buffer);
      }
      CloseHandle (h);
    }
  }
  return errorlevel = return_code;
}

/****************************************************************************
 * WCMD_more
 *
 * Output either a file or stdin to screen in pages
 */

RETURN_CODE WCMD_more(WCHAR *args)
{
  int   argno         = 0;
  WCHAR *argN         = args;
  WCHAR  moreStr[100];
  WCHAR  moreStrPage[100];
  WCHAR  buffer[512];
  DWORD count;
  RETURN_CODE return_code = NO_ERROR;

  /* Prefix the NLS more with '-- ', then load the text */
  lstrcpyW(moreStr, L"-- ");
  LoadStringW(hinst, WCMD_MORESTR, &moreStr[3], ARRAY_SIZE(moreStr)-3);

  if (param1[0] == 0x00) {

    /* Wine implements pipes via temporary files, and hence stdin is
       effectively reading from the file. This means the prompts for
       more are satisfied by the next line from the input (file). To
       avoid this, ensure stdin is to the console                    */
    HANDLE hstdin  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hConIn = CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, 0);
    WINE_TRACE("No parms - working probably in pipe mode\n");
    SetStdHandle(STD_INPUT_HANDLE, hConIn);

    /* Warning: No easy way of ending the stream (ctrl+z on windows) so
       once you get in this bit unless due to a pipe, it's going to end badly...  */
    wsprintfW(moreStrPage, L"%s --\n", moreStr);

    WCMD_enter_paged_mode(moreStrPage);
    while (WCMD_ReadFile(hstdin, buffer, ARRAY_SIZE(buffer)-1, &count)) {
      if (count == 0) break;	/* ReadFile reports success on EOF! */
      buffer[count] = 0;
      WCMD_output_asis (buffer);
    }
    WCMD_leave_paged_mode();

    /* Restore stdin to what it was */
    SetStdHandle(STD_INPUT_HANDLE, hstdin);
    CloseHandle(hConIn);
    WCMD_output_asis (L"\r\n");
  } else {
    BOOL needsPause = FALSE;

    /* Loop through all args */
    WINE_TRACE("Parms supplied - working through each file\n");
    WCMD_enter_paged_mode(moreStrPage);

    while (argN) {
      WCHAR *thisArg = WCMD_parameter (args, argno++, &argN, FALSE, FALSE);
      HANDLE h;

      if (!argN) break;

      if (needsPause) {

        /* Wait */
        wsprintfW(moreStrPage, L"%s (%2.2d%%) --\n", moreStr, 100);
        WCMD_leave_paged_mode();
        WCMD_output_asis(moreStrPage);
        WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), buffer, ARRAY_SIZE(buffer), &count);
        WCMD_enter_paged_mode(moreStrPage);
      }


      WINE_TRACE("more: Processing arg '%s'\n", wine_dbgstr_w(thisArg));
      h = CreateFileW(thisArg, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE) {
        WCMD_print_error ();
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_READFAIL), thisArg);
      } else {
        ULONG64 curPos  = 0;
        ULONG64 fileLen = 0;
        WIN32_FILE_ATTRIBUTE_DATA   fileInfo;

        /* Get the file size */
        GetFileAttributesExW(thisArg, GetFileExInfoStandard, (void*)&fileInfo);
        fileLen = (((ULONG64)fileInfo.nFileSizeHigh) << 32) + fileInfo.nFileSizeLow;

        needsPause = TRUE;
        while (WCMD_ReadFile(h, buffer, ARRAY_SIZE(buffer)-1, &count)) {
          if (count == 0) break;	/* ReadFile reports success on EOF! */
          buffer[count] = 0;
          curPos += count;

          /* Update % count (would be used in WCMD_output_asis as prompt) */
          wsprintfW(moreStrPage, L"%s (%2.2d%%) --\n", moreStr, (int) min(99, (curPos * 100)/fileLen));

          WCMD_output_asis (buffer);
        }
        CloseHandle (h);
      }
    }

    WCMD_leave_paged_mode();
  }
  return errorlevel = return_code;
}

/****************************************************************************
 * WCMD_verify
 *
 * Display verify flag.
 * FIXME: We don't actually do anything with the verify flag other than toggle
 * it...
 */

RETURN_CODE WCMD_verify(void)
{
    RETURN_CODE return_code = NO_ERROR;

    if (!param1[0])
        WCMD_output(WCMD_LoadMessage(WCMD_VERIFYPROMPT), verify_mode ? L"ON" : L"OFF");
    else if (lstrcmpiW(param1, L"ON") == 0)
        verify_mode = TRUE;
    else if (lstrcmpiW(param1, L"OFF") == 0)
        verify_mode = FALSE;
    else
    {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_VERIFYERR));
        return_code = ERROR_INVALID_FUNCTION;
    }
    return errorlevel = return_code;
}

/****************************************************************************
 * WCMD_version
 *
 * Display version info.
 */

RETURN_CODE WCMD_version(void)
{
    RETURN_CODE return_code;

    WCMD_output_asis(L"\r\n");
    if (*quals)
    {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
        return_code = ERROR_INVALID_FUNCTION;
    }
    else
    {
        WCMD_output_asis(version_string);
        return_code = NO_ERROR;
    }
    return errorlevel = return_code;
}

BOOL WCMD_print_volume_information(const WCHAR *path)
{
    WCHAR label[MAX_PATH];
    DWORD serial;

    if (!GetVolumeInformationW(path, label, ARRAY_SIZE(label), &serial, NULL, NULL, NULL, 0))
        return FALSE;
    if (label[0])
        WCMD_output(WCMD_LoadMessage(WCMD_VOLUMELABEL), path[0], label);
    else
        WCMD_output(WCMD_LoadMessage(WCMD_VOLUMENOLABEL), path[0]);

    WCMD_output(WCMD_LoadMessage(WCMD_VOLUMESERIALNO), HIWORD(serial), LOWORD(serial));
    return TRUE;
}

/****************************************************************************
 * WCMD_label
 *
 * Set volume label
 */

RETURN_CODE WCMD_label(void)
{
  DWORD count;
  WCHAR string[MAX_PATH], curdir[MAX_PATH];

  /* FIXME incomplete implementation:
   * - no support for /MP qualifier,
   * - no support for passing label as parameter
   */
  if (*quals)
      return errorlevel = ERROR_INVALID_FUNCTION;
  if (!*param1) {
    if (!GetCurrentDirectoryW(ARRAY_SIZE(curdir), curdir)) {
      WCMD_print_error();
      return errorlevel = ERROR_INVALID_FUNCTION;
    }
  }
  else if (param1[1] == ':' && !param1[2]) {
    curdir[0] = param1[0];
    curdir[1] = param1[1];
  } else {
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
      return errorlevel = ERROR_INVALID_FUNCTION;
  }
  curdir[2] = L'\\';
  curdir[3] = L'\0';
  if (!WCMD_print_volume_information(curdir)) {
    WCMD_print_error();
    return errorlevel = ERROR_INVALID_FUNCTION;
  }

  if (WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), string, ARRAY_SIZE(string), &count) &&
      count > 1) {
    string[count-1] = '\0';		/* ReadFile output is not null-terminatrred! */
    if (string[count-2] == '\r') string[count-2] = '\0'; /* Under Windoze we get CRLF! */
  }
  else return errorlevel = ERROR_INVALID_FUNCTION;
  if (*param1) {
    if (!SetVolumeLabelW(curdir, string))
    {
        errorlevel = GetLastError();
        WCMD_print_error();
        return errorlevel;
    }
  }
  return errorlevel = NO_ERROR;
}

RETURN_CODE WCMD_volume(void)
{
    WCHAR curdir[MAX_PATH];
    RETURN_CODE return_code = NO_ERROR;

    if (*quals)
        return errorlevel = ERROR_INVALID_FUNCTION;
    if (!*param1)
    {
        if (!GetCurrentDirectoryW(ARRAY_SIZE(curdir), curdir))
            return errorlevel = ERROR_INVALID_FUNCTION;
    }
    else if (param1[1] == L':' && !param1[2])
    {
        memcpy(curdir, param1, 2 * sizeof(WCHAR));
    }
    else
        return errorlevel = ERROR_INVALID_FUNCTION;
    curdir[2] = L'\\';
    curdir[3] = L'\0';
    if (!WCMD_print_volume_information(curdir))
    {
        return_code = GetLastError();
        WCMD_print_error();
    }
    return errorlevel = return_code;
}

/**************************************************************************
 * WCMD_exit
 *
 * Exit either the process, or just this batch program
 *
 */

RETURN_CODE WCMD_exit(void)
{
    int rc = wcstol(param1, NULL, 10); /* Note: wcstol of empty parameter is 0 */

    if (context && lstrcmpiW(quals, L"/B") == 0)
    {
        errorlevel = rc;
        context -> skip_rest = TRUE;
        return RETURN_CODE_ABORTED;
    }
    ExitProcess(rc);
}


/*****************************************************************************
 * WCMD_assoc
 *
 *	Lists or sets file associations  (assoc = TRUE)
 *      Lists or sets file types         (assoc = FALSE)
 */
RETURN_CODE WCMD_assoc(const WCHAR *args, BOOL assoc)
{
    RETURN_CODE return_code;
    HKEY        key;
    DWORD       accessOptions = KEY_READ;
    WCHAR      *newValue;
    LONG        rc = ERROR_SUCCESS;
    WCHAR       keyValue[MAXSTRING];
    DWORD       valueLen;
    HKEY        readKey;

    /* See if parameter includes '=' */
    return_code = NO_ERROR;
    newValue = wcschr(args, '=');
    if (newValue) accessOptions |= KEY_WRITE;

    /* Open a key to HKEY_CLASSES_ROOT for enumerating */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"", 0, accessOptions, &key) != ERROR_SUCCESS) {
      WINE_FIXME("Unexpected failure opening HKCR key: %ld\n", GetLastError());
      return ERROR_INVALID_FUNCTION;
    }

    /* If no parameters then list all associations */
    if (*args == 0x00) {
      int index = 0;

      /* Enumerate all the keys */
      while (rc != ERROR_NO_MORE_ITEMS) {
        WCHAR  keyName[MAXSTRING];
        DWORD nameLen;

        /* Find the next value */
        nameLen = MAXSTRING;
        rc = RegEnumKeyExW(key, index++, keyName, &nameLen, NULL, NULL, NULL, NULL);

        if (rc == ERROR_SUCCESS) {

          /* Only interested in extension ones if assoc, or others
             if not assoc                                          */
          if ((keyName[0] == '.' && assoc) ||
              (!(keyName[0] == '.') && (!assoc)))
          {
            WCHAR subkey[MAXSTRING];
            lstrcpyW(subkey, keyName);
            if (!assoc) lstrcatW(subkey, L"\\Shell\\Open\\Command");

            if (RegOpenKeyExW(key, subkey, 0, accessOptions, &readKey) == ERROR_SUCCESS) {

              valueLen = sizeof(keyValue);
              rc = RegQueryValueExW(readKey, NULL, NULL, NULL, (LPBYTE)keyValue, &valueLen);
              WCMD_output_asis(keyName);
              WCMD_output_asis(L"=");
              /* If no default value found, leave line empty after '=' */
              if (rc == ERROR_SUCCESS) {
                WCMD_output_asis(keyValue);
              }
              WCMD_output_asis(L"\r\n");
              RegCloseKey(readKey);
            }
          }
        }
      }

    } else {

      /* Parameter supplied - if no '=' on command line, it's a query */
      if (newValue == NULL) {
        WCHAR *space;
        WCHAR subkey[MAXSTRING];

        /* Query terminates the parameter at the first space */
        lstrcpyW(keyValue, args);
        space = wcschr(keyValue, ' ');
        if (space) *space=0x00;

        /* Set up key name */
        lstrcpyW(subkey, keyValue);
        if (!assoc) lstrcatW(subkey, L"\\Shell\\Open\\Command");

        valueLen = sizeof(keyValue);
        if (RegOpenKeyExW(key, subkey, 0, accessOptions, &readKey) == ERROR_SUCCESS &&
            RegQueryValueExW(readKey, NULL, NULL, NULL, (LPBYTE)keyValue, &valueLen) == ERROR_SUCCESS) {
          WCMD_output_asis(args);
          WCMD_output_asis(L"=");
          WCMD_output_asis(keyValue);
          WCMD_output_asis(L"\r\n");
          RegCloseKey(readKey);
          return_code = NO_ERROR;
        } else {
          WCHAR  msgbuffer[MAXSTRING];

          /* Load the translated 'File association not found' */
          if (assoc) {
            LoadStringW(hinst, WCMD_NOASSOC, msgbuffer, ARRAY_SIZE(msgbuffer));
          } else {
            LoadStringW(hinst, WCMD_NOFTYPE, msgbuffer, ARRAY_SIZE(msgbuffer));
          }
          WCMD_output_stderr(msgbuffer, keyValue);
          return_code = assoc ? ERROR_INVALID_FUNCTION : ERROR_FILE_NOT_FOUND;
        }

      /* Not a query - it's a set or clear of a value */
      } else {

        WCHAR subkey[MAXSTRING];

        /* Get pointer to new value */
        *newValue = 0x00;
        newValue++;

        /* Set up key name */
        lstrcpyW(subkey, args);
        if (!assoc) lstrcatW(subkey, L"\\Shell\\Open\\Command");

        if (*newValue == 0x00) {

          if (assoc)
            rc = RegDeleteKeyW(key, args);
          else {
            rc = RegCreateKeyExW(key, subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                                accessOptions, NULL, &readKey, NULL);
            if (rc == ERROR_SUCCESS) {
              rc = RegDeleteValueW(readKey, NULL);
              RegCloseKey(readKey);
            }
          }
          if (rc == ERROR_SUCCESS) {
            WINE_TRACE("HKCR Key '%s' deleted\n", wine_dbgstr_w(args));

          } else if (rc != ERROR_FILE_NOT_FOUND) {
            WCMD_print_error();
            return_code = ERROR_FILE_NOT_FOUND;

          } else {
            WCHAR  msgbuffer[MAXSTRING];

            /* Load the translated 'File association not found' */
            if (assoc) {
              LoadStringW(hinst, WCMD_NOASSOC, msgbuffer, ARRAY_SIZE(msgbuffer));
            } else {
              LoadStringW(hinst, WCMD_NOFTYPE, msgbuffer, ARRAY_SIZE(msgbuffer));
            }
            WCMD_output_stderr(msgbuffer, args);
            return_code = ERROR_FILE_NOT_FOUND;
          }

        /* It really is a set value = contents */
        } else {
          rc = RegCreateKeyExW(key, subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
                              accessOptions, NULL, &readKey, NULL);
          if (rc == ERROR_SUCCESS) {
            rc = RegSetValueExW(readKey, NULL, 0, REG_SZ,
                                (LPBYTE)newValue,
                                sizeof(WCHAR) * (lstrlenW(newValue) + 1));
            RegCloseKey(readKey);
          }

          if (rc != ERROR_SUCCESS) {
            WCMD_print_error();
            return_code = ERROR_FILE_NOT_FOUND;
          } else {
            WCMD_output_asis(args);
            WCMD_output_asis(L"=");
            WCMD_output_asis(newValue);
            WCMD_output_asis(L"\r\n");
          }
        }
      }
    }

    /* Clean up */
    RegCloseKey(key);
    return WCMD_is_in_context(L".bat") && return_code == NO_ERROR ?
        return_code : (errorlevel = return_code);
}

/****************************************************************************
 * WCMD_color
 *
 * Colors the terminal screen.
 */

RETURN_CODE WCMD_color(void)
{
  RETURN_CODE return_code = ERROR_INVALID_FUNCTION;
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

  if (param1[0] != 0x00 && lstrlenW(param1) > 2)
  {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_ARGERR));
  }
  else if (GetConsoleScreenBufferInfo(hStdOut, &consoleInfo))
  {
      COORD topLeft = {0, 0};
      DWORD screenSize;
      DWORD color;

      screenSize = consoleInfo.dwSize.X * (consoleInfo.dwSize.Y + 1);

      /* Convert the color hex digits */
      if (param1[0] == 0x00) {
        color = defaultColor;
      } else {
        color = wcstoul(param1, NULL, 16);
      }

      /* Fail if fg == bg color */
      if (((color & 0xF0) >> 4) != (color & 0x0F))
      {
          FillConsoleOutputAttribute(hStdOut, color, screenSize, topLeft, &screenSize);
          SetConsoleTextAttribute(hStdOut, color);
          return_code = NO_ERROR;
      }
  }
  return errorlevel = return_code;
}

/****************************************************************************
 * WCMD_mklink
 */

RETURN_CODE WCMD_mklink(WCHAR *args)
{
    int   argno = 0;
    WCHAR *argN = args;
    BOOL isdir = FALSE;
    BOOL junction = FALSE;
    BOOL hard = FALSE;
    BOOL ret = FALSE;
    WCHAR file1[MAX_PATH];
    WCHAR file2[MAX_PATH];

    file1[0] = 0;

    while (argN) {
        WCHAR *thisArg = WCMD_parameter (args, argno++, &argN, FALSE, FALSE);

        if (!argN) break;

        WINE_TRACE("mklink: Processing arg '%s'\n", wine_dbgstr_w(thisArg));

        if (lstrcmpiW(thisArg, L"/D") == 0)
            isdir = TRUE;
        else if (lstrcmpiW(thisArg, L"/H") == 0)
            hard = TRUE;
        else if (lstrcmpiW(thisArg, L"/J") == 0)
            junction = TRUE;
        else if (*thisArg == L'/')
        {
            return errorlevel = ERROR_INVALID_FUNCTION;
        }
        else
        {
            if(!file1[0])
                lstrcpyW(file1, thisArg);
            else
                lstrcpyW(file2, thisArg);
        }
    }

    if (*file1 && *file2)
    {
        if (hard)
            ret = CreateHardLinkW(file1, file2, NULL);
        else if(!junction)
            ret = CreateSymbolicLinkW(file1, file2, isdir);
        else
            TRACE("Junction links currently not supported.\n");
    }

    if (ret) return errorlevel = NO_ERROR;

    WCMD_output_stderr(WCMD_LoadMessage(WCMD_READFAIL), file1);
    return errorlevel = ERROR_INVALID_FUNCTION;
}

RETURN_CODE WCMD_change_drive(WCHAR drive)
{
    WCHAR envvar[4];
    WCHAR dir[MAX_PATH];

    /* According to MSDN CreateProcess docs, special env vars record
     * the current directory on each drive, in the form =C:
     * so see if one specified, and if so go back to it
     */
    envvar[0] = L'=';
    envvar[1] = drive;
    envvar[2] = L':';
    envvar[3] = L'\0';

    if (GetEnvironmentVariableW(envvar, dir, ARRAY_SIZE(dir)) == 0)
        wcscpy(dir, envvar + 1);
    WINE_TRACE("Got directory for %lc: as %s\n", drive, wine_dbgstr_w(dir));
    if (!SetCurrentDirectoryW(dir))
    {
        WCMD_print_error();
        return errorlevel = ERROR_INVALID_FUNCTION;
    }
    return NO_ERROR;
}
