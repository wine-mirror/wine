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
        L"CTTY",
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
        L"EXIT"
};
static const WCHAR externals[][10] = {
        L"ATTRIB",
        L"XCOPY"
};

static HINSTANCE hinst;
struct env_stack *saved_environment;
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
      if (!WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), answer, ARRAY_SIZE(answer), &count))
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

void WCMD_clear_screen (void) {

  /* Emulate by filling the screen from the top left to bottom right with
        spaces, then moving the cursor to the top left afterwards */
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

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
}

/****************************************************************************
 * WCMD_change_tty
 *
 * Change the default i/o device (ie redirect STDin/STDout).
 */

void WCMD_change_tty (void) {

  WCMD_output_stderr (WCMD_LoadMessage(WCMD_NYI));

}

/****************************************************************************
 * WCMD_choice
 *
 */

void WCMD_choice (const WCHAR * args) {
    WCHAR answer[16];
    WCHAR buffer[16];
    WCHAR *ptr = NULL;
    WCHAR *opt_c = NULL;
    WCHAR *my_command = NULL;
    WCHAR opt_default = 0;
    DWORD opt_timeout = 0;
    DWORD count;
    DWORD oldmode;
    BOOL have_console;
    BOOL opt_n = FALSE;
    BOOL opt_s = FALSE;

    have_console = GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &oldmode);
    errorlevel = 0;

    my_command = xstrdupW(WCMD_skip_leading_spaces((WCHAR*)args));

    ptr = WCMD_skip_leading_spaces(my_command);
    while (*ptr == '/') {
        switch (towupper(ptr[1])) {
            case 'C':
                ptr += 2;
                /* the colon is optional */
                if (*ptr == ':')
                    ptr++;

                if (!*ptr || iswspace(*ptr)) {
                    WINE_FIXME("bad parameter %s for /C\n", wine_dbgstr_w(ptr));
                    free(my_command);
                    return;
                }

                /* remember the allowed keys (overwrite previous /C option) */
                opt_c = ptr;
                while (*ptr && (!iswspace(*ptr)))
                    ptr++;

                if (*ptr) {
                    /* terminate allowed chars */
                    *ptr = 0;
                    ptr = WCMD_skip_leading_spaces(&ptr[1]);
                }
                WINE_TRACE("answer-list: %s\n", wine_dbgstr_w(opt_c));
                break;

            case 'N':
                opt_n = TRUE;
                ptr = WCMD_skip_leading_spaces(&ptr[2]);
                break;

            case 'S':
                opt_s = TRUE;
                ptr = WCMD_skip_leading_spaces(&ptr[2]);
                break;

            case 'T':
                ptr = &ptr[2];
                /* the colon is optional */
                if (*ptr == ':')
                    ptr++;

                opt_default = *ptr++;

                if (!opt_default || (*ptr != ',')) {
                    WINE_FIXME("bad option %s for /T\n", opt_default ? wine_dbgstr_w(ptr) : "");
                    free(my_command);
                    return;
                }
                ptr++;

                count = 0;
                while (((answer[count] = *ptr)) && iswdigit(*ptr) && (count < 15)) {
                    count++;
                    ptr++;
                }

                answer[count] = 0;
                opt_timeout = wcstol(answer, NULL, 10);

                ptr = WCMD_skip_leading_spaces(ptr);
                break;

            default:
                WINE_FIXME("bad parameter: %s\n", wine_dbgstr_w(ptr));
                free(my_command);
                return;
        }
    }

    if (opt_timeout)
        WINE_FIXME("timeout not supported: %c,%ld\n", opt_default, opt_timeout);

    if (have_console)
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), 0);

    /* use default keys, when needed: localized versions of "Y"es and "No" */
    if (!opt_c) {
        LoadStringW(hinst, WCMD_YES, buffer, ARRAY_SIZE(buffer));
        LoadStringW(hinst, WCMD_NO, buffer + 1, ARRAY_SIZE(buffer) - 1);
        opt_c = buffer;
        buffer[2] = 0;
    }

    /* print the question, when needed */
    if (*ptr)
        WCMD_output_asis(ptr);

    if (!opt_s) {
        wcsupr(opt_c);
        WINE_TRACE("case insensitive answer-list: %s\n", wine_dbgstr_w(opt_c));
    }

    if (!opt_n) {
        /* print a list of all allowed answers inside brackets */
        WCMD_output_asis(L"[");
        ptr = opt_c;
        answer[1] = 0;
        while ((answer[0] = *ptr++)) {
            WCMD_output_asis(answer);
            if (*ptr)
                WCMD_output_asis(L",");
        }
        WCMD_output_asis(L"]?");
    }

    while (TRUE) {

        /* FIXME: Add support for option /T */
        answer[1] = 0; /* terminate single character string */
        if (!WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), answer, 1, &count))
        {
            free(my_command);
            errorlevel = 0;
            return;
        }

        if (!opt_s)
            answer[0] = towupper(answer[0]);

        ptr = wcschr(opt_c, answer[0]);
        if (ptr) {
            WCMD_output_asis(answer);
            WCMD_output_asis(L"\r\n");
            if (have_console)
                SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), oldmode);

            errorlevel = (ptr - opt_c) + 1;
            WINE_TRACE("answer: %ld\n", errorlevel);
            free(my_command);
            return;
        }
        else
        {
            /* key not allowed: play the bell */
            WINE_TRACE("key not allowed: %s\n", wine_dbgstr_w(answer));
            WCMD_output_asis(L"\a");
        }
    }
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

void WCMD_copy(WCHAR * args) {

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

  /* Assume we were successful! */
  errorlevel = 0;

  /* If no args supplied at all, report an error */
  if (param1[0] == 0x00) {
    WCMD_output_stderr (WCMD_LoadMessage(WCMD_NOARG));
    errorlevel = 1;
    return;
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
        errorlevel = 1;
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
      errorlevel = 1;
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
    errorlevel = 1;
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
    if (!WCMD_get_fullpath(destination->name, ARRAY_SIZE(destname), destname, &filenamepart)) return;
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
    if (!WCMD_get_fullpath(thiscopy->name, ARRAY_SIZE(srcpath), srcpath, &filenamepart)) return;
    WINE_TRACE("Full src name is '%s'\n", wine_dbgstr_w(srcpath));

    /* If parameter is a directory, ensure it ends in \* */
    attributes = GetFileAttributesW(srcpath);
    if (ends_with_backslash( srcpath )) {

      /* We need to know where the filename part starts, so append * and
         recalculate the full resulting path                              */
      lstrcatW(thiscopy->name, L"*");
      if (!WCMD_get_fullpath(thiscopy->name, ARRAY_SIZE(srcpath), srcpath, &filenamepart)) return;
      WINE_TRACE("Directory, so full name is now '%s'\n", wine_dbgstr_w(srcpath));

    } else if ((wcspbrk(srcpath, L"*?") == NULL) &&
               (attributes != INVALID_FILE_ATTRIBUTES) &&
               (attributes & FILE_ATTRIBUTE_DIRECTORY)) {

      /* We need to know where the filename part starts, so append \* and
         recalculate the full resulting path                              */
      lstrcatW(thiscopy->name, L"\\*");
      if (!WCMD_get_fullpath(thiscopy->name, ARRAY_SIZE(srcpath), srcpath, &filenamepart)) return;
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
              /* Silently skip if the destination file is also a source file */
              status = TRUE;
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
              errorlevel = 1;
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
                  errorlevel = 1;
                }
              }
            }
          }
        }
      } while (!srcisdevice && FindNextFileW(hff, &fd) != 0);
      if (!srcisdevice) FindClose (hff);
    } else {
      /* Error if the first file was not found */
      if (!anyconcats || !writtenoneconcat) {
        WCMD_print_error ();
        errorlevel = 1;
      }
    }

    /* Step on to the next supplied source */
    thiscopy = thiscopy -> next;
  }

  /* Append EOF if ascii destination and we were concatenating */
  if (!errorlevel && !destination->binarycopy && anyconcats && writtenoneconcat) {
    if (!WCMD_AppendEOF(destination->name)) {
      WCMD_print_error ();
      errorlevel = 1;
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

  return;
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

void WCMD_create_dir (WCHAR *args) {
    int   argno = 0;
    WCHAR *argN = args;

    if (param1[0] == 0x00) {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
        return;
    }
    /* Loop through all args */
    while (TRUE) {
        WCHAR *thisArg = WCMD_parameter(args, argno++, &argN, FALSE, FALSE);
        if (!argN) break;
        if (!create_full_path(thisArg)) {
            WCMD_print_error ();
            errorlevel = 1;
        }
    }
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
            nextDir = xalloc(sizeof(DIRECTORY_STACK));
            if (allDirs == NULL) allDirs = nextDir;
            if (lastEntry != NULL) lastEntry->next = nextDir;
            lastEntry = nextDir;
            nextDir->next = NULL;
            nextDir->dirName = xstrdupW(subParm);
          }
        } while (FindNextFileW(hff, &fd) != 0);
        FindClose (hff);

        /* Go through each subdir doing the delete */
        while (allDirs != NULL) {
          DIRECTORY_STACK *tempDir;

          tempDir = allDirs->next;
          found |= WCMD_delete_one (allDirs->dirName);

          free(allDirs->dirName);
          free(allDirs);
          allDirs = tempDir;
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

BOOL WCMD_delete (WCHAR *args) {
    int   argno;
    WCHAR *argN;
    BOOL  argsProcessed = FALSE;
    BOOL  foundAny      = FALSE;

    errorlevel = 0;

    for (argno=0; ; argno++) {
        BOOL found;
        WCHAR *thisArg;

        argN = NULL;
        thisArg = WCMD_parameter (args, argno, &argN, FALSE, FALSE);
        if (!argN)
            break;       /* no more parameters */
        if (argN[0] == '/')
            continue;    /* skip options */

        argsProcessed = TRUE;
        found = WCMD_delete_one(thisArg);
        if (!found)
            WCMD_output_stderr(WCMD_LoadMessage(WCMD_FILENOTFOUND), thisArg);
        foundAny |= found;
    }

    /* Handle no valid args */
    if (!argsProcessed)
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));

    return foundAny;
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

void WCMD_echo (const WCHAR *args)
{
  int count;
  const WCHAR *origcommand = args;
  WCHAR *trimmed;

  if (   args[0]==' ' || args[0]=='\t' || args[0]=='.'
      || args[0]==':' || args[0]==';'  || args[0]=='/')
    args++;

  trimmed = WCMD_strtrim(args);
  if (!trimmed) return;

  count = lstrlenW(trimmed);
  if (count == 0 && origcommand[0]!='.' && origcommand[0]!=':'
                 && origcommand[0]!=';' && origcommand[0]!='/') {
    if (echo_mode) WCMD_output(WCMD_LoadMessage(WCMD_ECHOPROMPT), L"ON");
    else WCMD_output (WCMD_LoadMessage(WCMD_ECHOPROMPT), L"OFF");
    free(trimmed);
    return;
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
}

/*****************************************************************************
 * WCMD_part_execute
 *
 * Execute a command, and any && or bracketed follow on to the command. The
 * first command to be executed may not be at the front of the
 * commands->thiscommand string (eg. it may point after a DO or ELSE)
 */
static void WCMD_part_execute(CMD_LIST **cmdList, const WCHAR *firstcmd,
                              BOOL isIF, BOOL executecmds)
{
  CMD_LIST *curPosition = *cmdList;
  int myDepth = (*cmdList)->bracketDepth;

  WINE_TRACE("cmdList(%p), firstCmd(%s), doIt(%d), isIF(%d)\n", cmdList,
                wine_dbgstr_w(firstcmd), executecmds, isIF);

  /* Skip leading whitespace between condition and the command */
  while (firstcmd && *firstcmd && (*firstcmd==' ' || *firstcmd=='\t')) firstcmd++;

  /* Process the first command, if there is one */
  if (executecmds && firstcmd && *firstcmd) {
    WCHAR *command = xstrdupW(firstcmd);
    WCMD_execute (firstcmd, (*cmdList)->redirects, cmdList, FALSE);
    free(command);
  }


  /* If it didn't move the position, step to next command */
  if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;

  /* Process any other parts of the command */
  if (*cmdList) {
    BOOL processThese = executecmds;

    while (*cmdList) {
      /* execute all appropriate commands */
      curPosition = *cmdList;

      WINE_TRACE("Processing cmdList(%p) - delim(%d) bd(%d / %d) processThese(%d)\n",
                 *cmdList,
                 (*cmdList)->prevDelim,
                 (*cmdList)->bracketDepth,
                 myDepth,
                 processThese);

      /* Execute any statements appended to the line */
      /* FIXME: Only if previous call worked for && or failed for || */
      if ((*cmdList)->prevDelim == CMD_ONFAILURE ||
          (*cmdList)->prevDelim == CMD_ONSUCCESS) {
        if (processThese && (*cmdList)->command) {
          WCMD_execute ((*cmdList)->command, (*cmdList)->redirects,
                        cmdList, FALSE);
        }
        if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;

      /* Execute any appended to the statement with (...) */
      } else if ((*cmdList)->bracketDepth > myDepth) {
        if (processThese) {
          *cmdList = WCMD_process_commands(*cmdList, TRUE, FALSE);
        } else {
          WINE_TRACE("Skipping command %p due to stack depth\n", *cmdList);
        }
        if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;

      /* End of the command - does 'ELSE ' follow as the next command? */
      } else {
        if (isIF && WCMD_keyword_ws_found(L"else", (*cmdList)->command)) {
          /* Swap between if and else processing */
          processThese = !executecmds;

          /* Process the ELSE part */
          if (processThese) {
            const int keyw_len = lstrlenW(L"else") + 1;
            WCHAR *cmd = ((*cmdList)->command) + keyw_len;

            /* Skip leading whitespace between condition and the command */
            while (*cmd && (*cmd==' ' || *cmd=='\t')) cmd++;
            if (*cmd) {
              WCMD_execute (cmd, (*cmdList)->redirects, cmdList, FALSE);
            }
          } else {
              /* Loop skipping all commands until we get back to the current
                 depth, including skipping commands and their subsequent
                 pipes (eg cmd | prog)                                       */
              do {
                *cmdList = (*cmdList)->nextcommand;
              } while (*cmdList &&
                      ((*cmdList)->bracketDepth > myDepth ||
                      (*cmdList)->prevDelim));

              /* After the else is complete, we need to now process subsequent commands */
              processThese = TRUE;
          }
          if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;

        /* If we were in an IF statement and we didn't find an else and yet we get back to
           the same bracket depth as the IF, then the IF statement is over. This is required
           to handle nested ifs properly                                                     */
        } else if (isIF && (*cmdList)->bracketDepth == myDepth) {
          if (WCMD_keyword_ws_found(L"do", (*cmdList)->command)) {
              WINE_TRACE("Still inside FOR-loop, not an end of IF statement\n");
              *cmdList = (*cmdList)->nextcommand;
          } else {
              WINE_TRACE("Found end of this nested IF statement, ending this if\n");
              break;
          }
        } else if (!processThese) {
          if (curPosition == *cmdList) *cmdList = (*cmdList)->nextcommand;
          WINE_TRACE("Skipping this command, as in not process mode (next = %p)\n", *cmdList);
        } else {
          WINE_TRACE("Found end of this IF statement (next = %p)\n", *cmdList);
          break;
        }
      }
    }
  }
  return;
}

static BOOL option_equals(WCHAR **haystack, const WCHAR *needle)
{
  int len = wcslen(needle);

  if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
                     *haystack, len, needle, len) == CSTR_EQUAL) {
    *haystack += len;
    return TRUE;
  }

  return FALSE;
}

/*****************************************************************************
 * WCMD_parse_forf_options
 *
 * Parses the for /f 'options', extracting the values and validating the
 * keywords. Note all keywords are optional.
 * Parameters:
 *  options    [I] The unparsed parameter string
 *  eol        [O] Set to the comment character (eol=x)
 *  skip       [O] Set to the number of lines to skip (skip=xx)
 *  delims     [O] Set to the token delimiters (delims=)
 *  tokens     [O] Set to the requested tokens, as provided (tokens=)
 *  usebackq   [O] Set to TRUE if usebackq found
 *
 * Returns TRUE on success, FALSE on syntax error
 *
 */
static BOOL WCMD_parse_forf_options(WCHAR *options, WCHAR *eol, int *skip,
                                    WCHAR *delims, WCHAR *tokens, BOOL *usebackq)
{

  WCHAR *pos = options;
  int    len = lstrlenW(pos);

  /* Initialize to defaults */
  lstrcpyW(delims, L" \t");
  lstrcpyW(tokens, L"1");
  *eol      = 0;
  *skip     = 0;
  *usebackq = FALSE;

  /* Strip (optional) leading and trailing quotes */
  if ((*pos == '"') && (pos[len-1] == '"')) {
    pos[len-1] = 0;
    pos++;
  }

  /* Process each keyword */
  while (pos && *pos) {
    if (*pos == ' ' || *pos == '\t') {
      pos++;

    /* Save End of line character (Ignore line if first token (based on delims) starts with it) */
    } else if (option_equals(&pos, L"eol=")) {
      *eol = *pos++;
      WINE_TRACE("Found eol as %c(%x)\n", *eol, *eol);

    /* Save number of lines to skip (Can be in base 10, hex (0x...) or octal (0xx) */
    } else if (option_equals(&pos, L"skip=")) {
      WCHAR *nextchar = NULL;
      *skip = wcstoul(pos, &nextchar, 0);
      WINE_TRACE("Found skip as %d lines\n", *skip);
      pos = nextchar;

    /* Save if usebackq semantics are in effect */
    } else if (option_equals(&pos, L"usebackq")) {
      *usebackq = TRUE;
      WINE_TRACE("Found usebackq\n");

    /* Save the supplied delims. Slightly odd as space can be a delimiter but only
       if you finish the optionsroot string with delims= otherwise the space is
       just a token delimiter!                                                     */
    } else if (option_equals(&pos, L"delims=")) {
      int i=0;

      while (*pos && *pos != ' ') {
        delims[i++] = *pos;
        pos++;
      }
      if (*pos==' ' && *(pos+1)==0) delims[i++] = *pos;
      delims[i++] = 0; /* Null terminate the delims */
      WINE_TRACE("Found delims as '%s'\n", wine_dbgstr_w(delims));

    /* Save the tokens being requested */
    } else if (option_equals(&pos, L"tokens=")) {
      int i=0;

      while (*pos && *pos != ' ') {
        tokens[i++] = *pos;
        pos++;
      }
      tokens[i++] = 0; /* Null terminate the tokens */
      WINE_TRACE("Found tokens as '%s'\n", wine_dbgstr_w(tokens));

    } else {
      WINE_WARN("Unexpected data in optionsroot: '%s'\n", wine_dbgstr_w(pos));
      return FALSE;
    }
  }
  return TRUE;
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
static void WCMD_add_dirstowalk(DIRECTORY_STACK *dirsToWalk) {
  DIRECTORY_STACK *remainingDirs = dirsToWalk;
  WCHAR fullitem[MAX_PATH];
  WIN32_FIND_DATAW fd;
  HANDLE hff;

  /* Build a generic search and add all directories on the list of directories
     still to walk                                                             */
  lstrcpyW(fullitem, dirsToWalk->dirName);
  lstrcatW(fullitem, L"\\*");
  hff = FindFirstFileW(fullitem, &fd);
  if (hff != INVALID_HANDLE_VALUE) {
    do {
      WINE_TRACE("Looking for subdirectories\n");
      if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
          (lstrcmpW(fd.cFileName, L"..") != 0) && (lstrcmpW(fd.cFileName, L".") != 0))
      {
        /* Allocate memory, add to list */
        DIRECTORY_STACK *toWalk;
        if (wcslen(dirsToWalk->dirName) + 1 + wcslen(fd.cFileName) >= MAX_PATH)
        {
            WINE_TRACE("Skipping too long path %s\\%s\n",
                       debugstr_w(dirsToWalk->dirName), debugstr_w(fd.cFileName));
            continue;
        }
        toWalk = xalloc(sizeof(DIRECTORY_STACK));
        WINE_TRACE("(%p->%p)\n", remainingDirs, remainingDirs->next);
        toWalk->next = remainingDirs->next;
        remainingDirs->next = toWalk;
        remainingDirs = toWalk;
        toWalk->dirName = xalloc(sizeof(WCHAR) * (wcslen(dirsToWalk->dirName) + 2 + wcslen(fd.cFileName)));
        lstrcpyW(toWalk->dirName, dirsToWalk->dirName);
        lstrcatW(toWalk->dirName, L"\\");
        lstrcatW(toWalk->dirName, fd.cFileName);
        WINE_TRACE("Added to stack %s (%p->%p)\n", wine_dbgstr_w(toWalk->dirName),
                   toWalk, toWalk->next);
      }
    } while (FindNextFileW(hff, &fd) != 0);
    WINE_TRACE("Finished adding all subdirectories\n");
    FindClose (hff);
  }
}

/**************************************************************************
 * WCMD_for_nexttoken
 *
 * Parse the token= line, identifying the next highest number not processed
 * so far. Count how many tokens are referred (including duplicates) and
 * optionally return that, plus optionally indicate if the tokens= line
 * ends in a star.
 *
 * Parameters:
 *  lasttoken    [I]    - Identifies the token index of the last one
 *                           returned so far (-1 used for first loop)
 *  tokenstr     [I]    - The specified tokens= line
 *  firstCmd     [O]    - Optionally indicate how many tokens are listed
 *  doAll        [O]    - Optionally indicate if line ends with *
 *  duplicates   [O]    - Optionally indicate if there is any evidence of
 *                           overlaying tokens in the string
 * Note the caller should keep a running track of duplicates as the tokens
 * are recursively passed. If any have duplicates, then the * token should
 * not be honoured.
 */
static int WCMD_for_nexttoken(int lasttoken, WCHAR *tokenstr,
                              int *totalfound, BOOL *doall,
                              BOOL *duplicates)
{
  WCHAR *pos = tokenstr;
  int    nexttoken = -1;

  if (totalfound) *totalfound = 0;
  if (doall) *doall = FALSE;
  if (duplicates) *duplicates = FALSE;

  WINE_TRACE("Find next token after %d in %s\n", lasttoken,
             wine_dbgstr_w(tokenstr));

  /* Loop through the token string, parsing it. Valid syntax is:
     token=m or x-y with comma delimiter and optionally * to finish*/
  while (*pos) {
    int nextnumber1, nextnumber2 = -1;
    WCHAR *nextchar;

    /* Remember if the next character is a star, it indicates a need to
       show all remaining tokens and should be the last character       */
    if (*pos == '*') {
      if (doall) *doall = TRUE;
      if (totalfound) (*totalfound)++;
      /* If we have not found a next token to return, then indicate
         time to process the star                                   */
      if (nexttoken == -1) {
         if (lasttoken == -1) {
           /* Special case the syntax of tokens=* which just means get whole line */
           nexttoken = 0;
         } else {
           nexttoken = lasttoken;
         }
      }
      break;
    }

    /* Get the next number */
    nextnumber1 = wcstoul(pos, &nextchar, 10);

    /* If it is followed by a minus, it's a range, so get the next one as well */
    if (*nextchar == '-') {
      nextnumber2 = wcstoul(nextchar+1, &nextchar, 10);

      /* We want to return the lowest number that is higher than lasttoken
         but only if range is positive                                     */
      if (nextnumber2 >= nextnumber1 &&
          lasttoken < nextnumber2) {

        int nextvalue;
        if (nexttoken == -1) {
          nextvalue = max(nextnumber1, (lasttoken+1));
        } else {
          nextvalue = min(nexttoken, max(nextnumber1, (lasttoken+1)));
        }

        /* Flag if duplicates identified */
        if (nexttoken == nextvalue && duplicates) *duplicates = TRUE;

        nexttoken = nextvalue;
      }

      /* Update the running total for the whole range */
      if (nextnumber2 >= nextnumber1 && totalfound) {
        *totalfound = *totalfound + 1 + (nextnumber2 - nextnumber1);
      }
      pos = nextchar;

    } else if (pos != nextchar) {
      if (totalfound) (*totalfound)++;

      /* See if the number found is one we have already seen */
      if (nextnumber1 == nexttoken && duplicates) *duplicates = TRUE;

      /* We want to return the lowest number that is higher than lasttoken */
      if (lasttoken < nextnumber1 &&
         ((nexttoken == -1) || (nextnumber1 < nexttoken))) {
        nexttoken = nextnumber1;
      }
      pos = nextchar;

    } else {
      /* Step on to the next character, usually over comma */
      if (*pos) pos++;
    }

  }

  /* Return result */
  if (nexttoken == -1) {
    WINE_TRACE("No next token found, previous was %d\n", lasttoken);
    nexttoken = lasttoken;
  } else if (nexttoken==lasttoken && doall && *doall) {
    WINE_TRACE("Request for all remaining tokens now\n");
  } else {
    WINE_TRACE("Found next token after %d was %d\n", lasttoken, nexttoken);
  }
  if (totalfound) WINE_TRACE("Found total tokens to be %d\n", *totalfound);
  if (duplicates && *duplicates) WINE_TRACE("Duplicate numbers found\n");
  return nexttoken;
}

/**************************************************************************
 * WCMD_parse_line
 *
 * When parsing file or string contents (for /f), once the string to parse
 * has been identified, handle the various options and call the do part
 * if appropriate.
 *
 * Parameters:
 *  cmdStart     [I]    - Identifies the list of commands making up the
 *                           for loop body (especially if brackets in use)
 *  firstCmd     [I]    - The textual start of the command after the DO
 *                           which is within the first item of cmdStart
 *  cmdEnd       [O]    - Identifies where to continue after the DO
 *  variable     [I]    - The variable identified on the for line
 *  buffer       [I]    - The string to parse
 *  doExecuted   [O]    - Set to TRUE if the DO is ever executed once
 *  forf_skip    [I/O]  - How many lines to skip first
 *  forf_eol     [I]    - The 'end of line' (comment) character
 *  forf_delims  [I]    - The delimiters to use when breaking the string apart
 *  forf_tokens  [I]    - The tokens to use when breaking the string apart
 */
static void WCMD_parse_line(CMD_LIST    *cmdStart,
                            const WCHAR *firstCmd,
                            CMD_LIST   **cmdEnd,
                            const WCHAR  variable,
                            WCHAR       *buffer,
                            BOOL        *doExecuted,
                            int         *forf_skip,
                            WCHAR        forf_eol,
                            WCHAR       *forf_delims,
                            WCHAR       *forf_tokens) {

  WCHAR *parm;
  FOR_CONTEXT oldcontext;
  int varidx, varoffset;
  int nexttoken, lasttoken = -1;
  BOOL starfound = FALSE;
  BOOL thisduplicate = FALSE;
  BOOL anyduplicates = FALSE;
  int  totalfound;
  static WCHAR emptyW[] = L"";

  /* Skip lines if requested */
  if (*forf_skip) {
    (*forf_skip)--;
    return;
  }

  /* Save away any existing for variable context (e.g. nested for loops) */
  oldcontext = forloopcontext;

  /* Extract the parameters based on the tokens= value (There will always
     be some value, as if it is not supplied, it defaults to tokens=1).
     Rough logic:
     Count how many tokens are named in the line, identify the lowest
     Empty (set to null terminated string) that number of named variables
     While lasttoken != nextlowest
       %letter = parameter number 'nextlowest'
       letter++ (if >26 or >52 abort)
       Go through token= string finding next lowest number
     If token ends in * set %letter = raw position of token(nextnumber+1)
   */
  lasttoken = -1;
  nexttoken = WCMD_for_nexttoken(lasttoken, forf_tokens, &totalfound,
                                 &starfound, &thisduplicate);
  varidx = FOR_VAR_IDX(variable);

  /* Empty out variables */
  for (varoffset=0;
       varidx >= 0 && varoffset<totalfound && (((varidx%26) + varoffset) < 26);
       varoffset++) {
    forloopcontext.variable[varidx + varoffset] = emptyW;
  }

  /* Loop extracting the tokens
     Note: nexttoken of 0 means there were no tokens requested, to handle
           the special case of tokens=*                                   */
  varoffset = 0;
  WINE_TRACE("Parsing buffer into tokens: '%s'\n", wine_dbgstr_w(buffer));
  while (varidx >= 0 && (nexttoken > 0 && (nexttoken > lasttoken))) {
    anyduplicates |= thisduplicate;

    /* Extract the token number requested and set into the next variable context */
    parm = WCMD_parameter_with_delims(buffer, (nexttoken-1), NULL, TRUE, FALSE, forf_delims);
    WINE_TRACE("Parsed token %d(%d) as parameter %s\n", nexttoken,
               varidx + varoffset, wine_dbgstr_w(parm));
    if (varidx >=0) {
      if (parm) forloopcontext.variable[varidx + varoffset] = xstrdupW(parm);
      varoffset++;
      if (((varidx%26)+varoffset) >= 26) break;
    }

    /* Find the next token */
    lasttoken = nexttoken;
    nexttoken = WCMD_for_nexttoken(lasttoken, forf_tokens, NULL,
                                   &starfound, &thisduplicate);
  }

  /* If all the rest of the tokens were requested, and there is still space in
     the variable range, write them now                                        */
  if (!anyduplicates && starfound && varidx >= 0 && (((varidx%26) + varoffset) < 26)) {
    nexttoken++;
    WCMD_parameter_with_delims(buffer, (nexttoken-1), &parm, FALSE, FALSE, forf_delims);
    WINE_TRACE("Parsed allremaining tokens (%d) as parameter %s\n",
               varidx + varoffset, wine_dbgstr_w(parm));
    if (parm) forloopcontext.variable[varidx + varoffset] = xstrdupW(parm);
  }

  /* Execute the body of the foor loop with these values */
  if (varidx >= 0 && forloopcontext.variable[varidx] && forloopcontext.variable[varidx][0] != forf_eol) {
    CMD_LIST *thisCmdStart = cmdStart;
    *doExecuted = TRUE;
    WCMD_part_execute(&thisCmdStart, firstCmd, FALSE, TRUE);
    *cmdEnd = thisCmdStart;
  }

  /* Free the duplicated strings, and restore the context */
  if (varidx >=0) {
    int i;
    for (i=varidx; i<MAX_FOR_VARIABLES; i++) {
      if ((forloopcontext.variable[i] != oldcontext.variable[i]) &&
          (forloopcontext.variable[i] != emptyW)) {
        free(forloopcontext.variable[i]);
      }
    }
  }

  /* Restore the original for variable contextx */
  forloopcontext = oldcontext;
}

/**************************************************************************
 * WCMD_forf_getinput
 *
 * Return a FILE* which can be used for reading the input lines,
 * either to a specific file (which may be quote delimited as we have to
 * read the parameters in raw mode) or to a command which we need to
 * execute. The command being executed runs in its own shell and stores
 * its data in a temporary file.
 *
 * Parameters:
 *  usebackq     [I]    - Indicates whether usebackq is in effect or not
 *  itemStr      [I]    - The item to be handled, either a filename or
 *                           whole command string to execute
 *  iscmd        [I]    - Identifies whether this is a command or not
 *
 * Returns a file handle which can be used to read the input lines from.
 */
static FILE* WCMD_forf_getinput(BOOL usebackq, WCHAR *itemstr, BOOL iscmd) {
  WCHAR  temp_str[MAX_PATH];
  WCHAR  temp_file[MAX_PATH];
  WCHAR  temp_cmd[MAXSTRING];
  WCHAR *trimmed = NULL;
  FILE*  ret;

  /* Remove leading and trailing character (but there may be trailing whitespace too) */
  if ((iscmd && (itemstr[0] == '`' && usebackq)) ||
      (iscmd && (itemstr[0] == '\'' && !usebackq)) ||
      (!iscmd && (itemstr[0] == '"' && usebackq)))
  {
    trimmed = WCMD_strtrim(itemstr);
    if (trimmed) {
      itemstr = trimmed;
    }
    itemstr[lstrlenW(itemstr)-1] = 0x00;
    itemstr++;
  }

  if (iscmd) {
    /* Get temp filename */
    GetTempPathW(ARRAY_SIZE(temp_str), temp_str);
    GetTempFileNameW(temp_str, L"CMD", 0, temp_file);

    /* Redirect output to the temporary file */
    wsprintfW(temp_str, L">%s", temp_file);
    wsprintfW(temp_cmd, L"CMD.EXE /C %s", itemstr);
    WINE_TRACE("Issuing '%s' with redirs '%s'\n",
               wine_dbgstr_w(temp_cmd), wine_dbgstr_w(temp_str));
    WCMD_execute (temp_cmd, temp_str, NULL, FALSE);

    /* Open the file, read line by line and process */
    ret = _wfopen(temp_file, L"rt,ccs=unicode");
  } else {
    /* Open the file, read line by line and process */
    WINE_TRACE("Reading input to parse from '%s'\n", wine_dbgstr_w(itemstr));
    ret = _wfopen(itemstr, L"rt,ccs=unicode");
  }
  free(trimmed);
  return ret;
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
 */

void WCMD_for (WCHAR *p, CMD_LIST **cmdList) {

  WIN32_FIND_DATAW fd;
  HANDLE hff;
  int i;
  const int in_len = lstrlenW(L"in");
  CMD_LIST *setStart, *thisSet, *cmdStart, *cmdEnd;
  WCHAR variable[4];
  int   varidx = -1;
  WCHAR *oldvariablevalue;
  WCHAR *firstCmd;
  int thisDepth;
  WCHAR optionsRoot[MAX_PATH];
  DIRECTORY_STACK *dirsToWalk = NULL;
  BOOL   expandDirs  = FALSE;
  BOOL   useNumbers  = FALSE;
  BOOL   doFileset   = FALSE;
  BOOL   doRecurse   = FALSE;
  BOOL   doExecuted  = FALSE;  /* Has the 'do' part been executed */
  LONG   numbers[3] = {0,0,0}; /* Defaults to 0 in native */
  int    itemNum;
  CMD_LIST *thisCmdStart;
  int    parameterNo = 0;
  WCHAR  forf_eol = 0;
  int    forf_skip = 0;
  WCHAR  forf_delims[256];
  WCHAR  forf_tokens[MAXSTRING];
  BOOL   forf_usebackq = FALSE;

  /* Handle optional qualifiers (multiple are allowed) */
  WCHAR *thisArg = WCMD_parameter(p, parameterNo++, NULL, FALSE, FALSE);

  optionsRoot[0] = 0;
  while (thisArg && *thisArg == '/') {
      WINE_TRACE("Processing qualifier at %s\n", wine_dbgstr_w(thisArg));
      thisArg++;
      switch (towupper(*thisArg)) {
      case 'D': expandDirs = TRUE; break;
      case 'L': useNumbers = TRUE; break;

      /* Recursive is special case - /R can have an optional path following it                */
      /* filenamesets are another special case - /F can have an optional options following it */
      case 'R':
      case 'F':
          {
              /* When recursing directories, use current directory as the starting point unless
                 subsequently overridden */
              doRecurse = (towupper(*thisArg) == 'R');
              if (doRecurse) GetCurrentDirectoryW(ARRAY_SIZE(optionsRoot), optionsRoot);

              doFileset = (towupper(*thisArg) == 'F');

              /* Retrieve next parameter to see if is root/options (raw form required
                 with for /f, or unquoted in for /r)                                  */
              thisArg = WCMD_parameter(p, parameterNo, NULL, doFileset, FALSE);

              /* Next parm is either qualifier, path/options or variable -
                 only care about it if it is the path/options              */
              if (thisArg && *thisArg != '/' && *thisArg != '%') {
                  parameterNo++;
                  lstrcpyW(optionsRoot, thisArg);
              }
              break;
          }
      default:
          WINE_FIXME("for qualifier '%c' unhandled\n", *thisArg);
      }

      /* Step to next token */
      thisArg = WCMD_parameter(p, parameterNo++, NULL, FALSE, FALSE);
  }

  /* Ensure line continues with variable */
  if (*thisArg != '%') {
      WCMD_output_stderr (WCMD_LoadMessage(WCMD_SYNTAXERR));
      return;
  }

  /* With for /f parse the options if provided */
  if (doFileset) {
    if (!WCMD_parse_forf_options(optionsRoot, &forf_eol, &forf_skip,
                                 forf_delims, forf_tokens, &forf_usebackq))
    {
      WCMD_output_stderr (WCMD_LoadMessage(WCMD_SYNTAXERR));
      return;
    }

  /* Set up the list of directories to recurse if we are going to */
  } else if (doRecurse) {
       /* Allocate memory, add to list */
       dirsToWalk = xalloc(sizeof(DIRECTORY_STACK));
       dirsToWalk->next = NULL;
       dirsToWalk->dirName = xstrdupW(optionsRoot);
       WINE_TRACE("Starting with root directory %s\n", wine_dbgstr_w(dirsToWalk->dirName));
  }

  /* Variable should follow */
  lstrcpyW(variable, thisArg);
  WINE_TRACE("Variable identified as %s\n", wine_dbgstr_w(variable));
  varidx = FOR_VAR_IDX(variable[1]);

  /* Ensure line continues with IN */
  thisArg = WCMD_parameter(p, parameterNo++, NULL, FALSE, FALSE);
  if (!thisArg
       || !(CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
                           thisArg, in_len, L"in", in_len) == CSTR_EQUAL)) {
      WCMD_output_stderr (WCMD_LoadMessage(WCMD_SYNTAXERR));
      return;
  }

  /* Save away where the set of data starts and the variable */
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
  WINE_TRACE("Looking for 'do ' in %p\n", *cmdList);
  if ((*cmdList == NULL) || !WCMD_keyword_ws_found(L"do", (*cmdList)->command)) {
      WCMD_output_stderr (WCMD_LoadMessage(WCMD_SYNTAXERR));
      return;
  }

  cmdEnd   = *cmdList;

  /* Loop repeatedly per-directory we are potentially walking, when in for /r
     mode, or once for the rest of the time.                                  */
  do {

    /* Save away the starting position for the commands (and offset for the
       first one)                                                           */
    cmdStart = *cmdList;
    firstCmd = (*cmdList)->command + 3; /* Skip 'do ' */
    itemNum  = 0;

    /* If we are recursing directories (ie /R), add all sub directories now, then
       prefix the root when searching for the item */
    if (dirsToWalk) WCMD_add_dirstowalk(dirsToWalk);

    thisSet = setStart;
    /* Loop through all set entries */
    while (thisSet &&
           thisSet->command != NULL &&
           thisSet->bracketDepth >= thisDepth) {

      /* Loop through all entries on the same line */
      WCHAR *staticitem;
      WCHAR *itemStart;
      WCHAR buffer[MAXSTRING];

      WINE_TRACE("Processing for set %p\n", thisSet);
      i = 0;
      while (*(staticitem = WCMD_parameter (thisSet->command, i, &itemStart, TRUE, FALSE))) {

        /*
         * If the parameter within the set has a wildcard then search for matching files
         * otherwise do a literal substitution.
         */

        /* Take a copy of the item returned from WCMD_parameter as it is held in a
           static buffer which can be overwritten during parsing of the for body   */
        WCHAR item[MAXSTRING];
        lstrcpyW(item, staticitem);

        thisCmdStart = cmdStart;

        itemNum++;
        WINE_TRACE("Processing for item %d '%s'\n", itemNum, wine_dbgstr_w(item));

        if (!useNumbers && !doFileset) {
            WCHAR fullitem[MAX_PATH];
            int prefixlen = 0;

            /* Now build the item to use / search for in the specified directory,
               as it is fully qualified in the /R case */
            if (dirsToWalk) {
              lstrcpyW(fullitem, dirsToWalk->dirName);
              lstrcatW(fullitem, L"\\");
              lstrcatW(fullitem, item);
            } else {
              WCHAR *prefix = wcsrchr(item, '\\');
              if (prefix) prefixlen = (prefix - item) + 1;
              lstrcpyW(fullitem, item);
            }

            if (wcspbrk(fullitem, L"*?")) {
              hff = FindFirstFileW(fullitem, &fd);
              if (hff != INVALID_HANDLE_VALUE) {
                do {
                  BOOL isDirectory = FALSE;

                  if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) isDirectory = TRUE;

                  /* Handle as files or dirs appropriately, but ignore . and .. */
                  if (isDirectory == expandDirs &&
                      (lstrcmpW(fd.cFileName, L"..") != 0) && (lstrcmpW(fd.cFileName, L".") != 0))
                  {
                      thisCmdStart = cmdStart;
                      WINE_TRACE("Processing FOR filename %s\n", wine_dbgstr_w(fd.cFileName));

                      if (doRecurse) {
                          if (wcslen(dirsToWalk->dirName) + 1 + wcslen(fd.cFileName) >= MAX_PATH)
                          {
                              WINE_TRACE("Skipping too long path %s\\%s\n",
                                         debugstr_w(dirsToWalk->dirName), debugstr_w(fd.cFileName));
                              continue;
                          }
                          lstrcpyW(fullitem, dirsToWalk->dirName);
                          lstrcatW(fullitem, L"\\");
                          lstrcatW(fullitem, fd.cFileName);
                      } else {
                          if (prefixlen) lstrcpynW(fullitem, item, prefixlen + 1);
                          fullitem[prefixlen] = 0x00;
                          lstrcatW(fullitem, fd.cFileName);
                      }
                      doExecuted = TRUE;

                      /* Save away any existing for variable context (e.g. nested for loops)
                         and restore it after executing the body of this for loop           */
                      if (varidx >= 0) {
                        oldvariablevalue = forloopcontext.variable[varidx];
                        forloopcontext.variable[varidx] = fullitem;
                      }
                      WCMD_part_execute (&thisCmdStart, firstCmd, FALSE, TRUE);
                      if (varidx >= 0) forloopcontext.variable[varidx] = oldvariablevalue;

                      cmdEnd = thisCmdStart;
                  }
                } while (FindNextFileW(hff, &fd) != 0);
                FindClose (hff);
              }
            } else {
              doExecuted = TRUE;

              /* Save away any existing for variable context (e.g. nested for loops)
                 and restore it after executing the body of this for loop           */
              if (varidx >= 0) {
                oldvariablevalue = forloopcontext.variable[varidx];
                forloopcontext.variable[varidx] = fullitem;
              }
              WCMD_part_execute (&thisCmdStart, firstCmd, FALSE, TRUE);
              if (varidx >= 0) forloopcontext.variable[varidx] = oldvariablevalue;

              cmdEnd = thisCmdStart;
            }

        } else if (useNumbers) {
            /* Convert the first 3 numbers to signed longs and save */
            if (itemNum <=3) numbers[itemNum-1] = wcstol(item, NULL, 10);
            /* else ignore them! */

        /* Filesets - either a list of files, or a command to run and parse the output */
        } else if (doFileset && ((!forf_usebackq && *itemStart != '"') ||
                                 (forf_usebackq && *itemStart != '\''))) {

            FILE *input;
            WCHAR *itemparm;

            WINE_TRACE("Processing for filespec from item %d '%s'\n", itemNum,
                       wine_dbgstr_w(item));

            /* If backquote or single quote, we need to launch that command
               and parse the results - use a temporary file                 */
            if ((forf_usebackq && *itemStart == '`') ||
                (!forf_usebackq && *itemStart == '\'')) {

              /* Use itemstart because the command is the whole set, not just the first token */
              itemparm = itemStart;
            } else {

              /* Use item because the file to process is just the first item in the set */
              itemparm = item;
            }
            input = WCMD_forf_getinput(forf_usebackq, itemparm, (itemparm==itemStart));

            /* Process the input file */
            if (!input) {
              WCMD_print_error ();
              WCMD_output_stderr(WCMD_LoadMessage(WCMD_READFAIL), item);
              errorlevel = 1;
              return; /* FOR loop aborts at first failure here */

            } else {

              /* Read line by line until end of file */
              while (fgetws(buffer, ARRAY_SIZE(buffer), input)) {
                size_t len = wcslen(buffer);
                /* Either our buffer isn't large enough to fit a full line, or there's a stray
                 * '\0' in the buffer.
                 */
                if (!feof(input) && (len == 0 || (buffer[len - 1] != '\n' && buffer[len - 1] != '\r')))
                    break;
                while (len && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
                    buffer[--len] = L'\0';
                WCMD_parse_line(cmdStart, firstCmd, &cmdEnd, variable[1], buffer, &doExecuted,
                                &forf_skip, forf_eol, forf_delims, forf_tokens);
                buffer[0] = 0;
              }
              fclose (input);
            }

            /* When we have processed the item as a whole command, abort future set processing */
            if (itemparm==itemStart) {
              thisSet = NULL;
              break;
            }

        /* Filesets - A string literal */
        } else if (doFileset && ((!forf_usebackq && *itemStart == '"') ||
                                 (forf_usebackq && *itemStart == '\''))) {

          /* Remove leading and trailing character, ready to parse with delims= delimiters
             Note that the last quote is removed from the set and the string terminates
             there to mimic windows                                                        */
          WCHAR *strend = wcsrchr(itemStart, forf_usebackq?'\'':'"');
          if (strend) {
            *strend = 0x00;
            itemStart++;
          }

          /* Copy the item away from the global buffer used by WCMD_parameter */
          lstrcpyW(buffer, itemStart);
          WCMD_parse_line(cmdStart, firstCmd, &cmdEnd, variable[1], buffer, &doExecuted,
                            &forf_skip, forf_eol, forf_delims, forf_tokens);

          /* Only one string can be supplied in the whole set, abort future set processing */
          thisSet = NULL;
          break;
        }

        WINE_TRACE("Post-command, cmdEnd = %p\n", cmdEnd);
        i++;
      }

      /* Move onto the next set line */
      if (thisSet) thisSet = thisSet->nextcommand;
    }

    /* If /L is provided, now run the for loop */
    if (useNumbers) {
        WCHAR thisNum[20];

        WINE_TRACE("FOR /L provided range from %ld to %ld step %ld\n",
                   numbers[0], numbers[2], numbers[1]);
        for (i=numbers[0];
             (numbers[1]<0)? i>=numbers[2] : i<=numbers[2];
             i=i + numbers[1]) {

            swprintf(thisNum, ARRAY_SIZE(thisNum), L"%d", i);
            WINE_TRACE("Processing FOR number %s\n", wine_dbgstr_w(thisNum));

            thisCmdStart = cmdStart;
            doExecuted = TRUE;

            /* Save away any existing for variable context (e.g. nested for loops)
               and restore it after executing the body of this for loop           */
            if (varidx >= 0) {
              oldvariablevalue = forloopcontext.variable[varidx];
              forloopcontext.variable[varidx] = thisNum;
            }
            WCMD_part_execute (&thisCmdStart, firstCmd, FALSE, TRUE);
            if (varidx >= 0) forloopcontext.variable[varidx] = oldvariablevalue;
        }
        cmdEnd = thisCmdStart;
    }

    /* If we are walking directories, move on to any which remain */
    if (dirsToWalk != NULL) {
      DIRECTORY_STACK *nextDir = dirsToWalk->next;
      free(dirsToWalk->dirName);
      free(dirsToWalk);
      dirsToWalk = nextDir;
      if (dirsToWalk) WINE_TRACE("Moving to next directory to iterate: %s\n",
                                 wine_dbgstr_w(dirsToWalk->dirName));
      else WINE_TRACE("Finished all directories.\n");
    }

  } while (dirsToWalk != NULL);

  /* Now skip over the do part if we did not perform the for loop so far.
     We store in cmdEnd the next command after the do block, but we only
     know this if something was run. If it has not been, we need to calculate
     it.                                                                      */
  if (!doExecuted) {
    thisCmdStart = cmdStart;
    WINE_TRACE("Skipping for loop commands due to no valid iterations\n");
    WCMD_part_execute(&thisCmdStart, firstCmd, FALSE, FALSE);
    cmdEnd = thisCmdStart;
  }

  /* When the loop ends, either something like a GOTO or EXIT /b has terminated
     all processing, OR it should be pointing to the end of && processing OR
     it should be pointing at the NULL end of bracket for the DO. The return
     value needs to be the NEXT command to execute, which it either is, or
     we need to step over the closing bracket                                  */
  *cmdList = cmdEnd;
  if (cmdEnd && cmdEnd->command == NULL) *cmdList = cmdEnd->nextcommand;
}

/**************************************************************************
 * WCMD_give_help
 *
 *	Simple on-line help. Help text is stored in the resource file.
 */

void WCMD_give_help (const WCHAR *args)
{
  size_t i;

  args = WCMD_skip_leading_spaces((WCHAR*) args);
  if (!*args) {
    WCMD_output_asis (WCMD_LoadMessage(WCMD_ALLHELP));
  }
  else {
    /* Display help message for builtin commands */
    for (i=0; i<=WCMD_EXIT; i++) {
      if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
	  args, -1, inbuilt[i], -1) == CSTR_EQUAL) {
	WCMD_output_asis (WCMD_LoadMessage(i));
	return;
      }
    }
    /* Launch the command with the /? option for external commands shipped with cmd.exe */
    for (i = 0; i <= ARRAY_SIZE(externals); i++) {
      if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
	  args, -1, externals[i], -1) == CSTR_EQUAL) {
        WCHAR cmd[128];
        lstrcpyW(cmd, args);
        lstrcatW(cmd, L" /?");
        WCMD_run_program(cmd, FALSE);
        return;
      }
    }
    WCMD_output (WCMD_LoadMessage(WCMD_NOCMDHELP), args);
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
  WCHAR *labelend = NULL;
  const WCHAR labelEndsW[] = L"><|& :\t";

  /* Do not process any more parts of a processed multipart or multilines command */
  if (cmdList) *cmdList = NULL;

  if (context != NULL) {
    WCHAR *paramStart = param1, *str;

    if (param1[0] == 0x00) {
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
      return;
    }

    /* Handle special :EOF label */
    if (lstrcmpiW(L":eof", param1) == 0) {
      context -> skip_rest = TRUE;
      return;
    }

    /* Support goto :label as well as goto label plus remove trailing chars */
    if (*paramStart == ':') paramStart++;
    labelend = wcspbrk(paramStart, labelEndsW);
    if (labelend) *labelend = 0x00;
    WINE_TRACE("goto label: '%s'\n", wine_dbgstr_w(paramStart));

    /* Loop through potentially twice - once from current file position
       through to the end, and second time from start to current file
       position                                                         */
    if (*paramStart) {
        int loop;
        LARGE_INTEGER startli;
        for (loop=0; loop<2; loop++) {
            if (loop==0) {
              /* On first loop, save the file size */
              startli.QuadPart = 0;
              startli.u.LowPart = SetFilePointer(context -> h, startli.u.LowPart,
                                                 &startli.u.HighPart, FILE_CURRENT);
            } else {
              /* On second loop, start at the beginning of the file */
              WINE_TRACE("Label not found, trying from beginning of file\n");
              if (loop==1) SetFilePointer (context -> h, 0, NULL, FILE_BEGIN);
            }

            while (WCMD_fgets (string, ARRAY_SIZE(string), context -> h)) {
              str = string;

              /* Ignore leading whitespace or no-echo character */
              while (*str=='@' || iswspace (*str)) str++;

              /* If the first real character is a : then this is a label */
              if (*str == ':') {
                str++;

                /* Skip spaces between : and label */
                while (iswspace (*str)) str++;
                WINE_TRACE("str before brk %s\n", wine_dbgstr_w(str));

                /* Label ends at whitespace or redirection characters */
                labelend = wcspbrk(str, labelEndsW);
                if (labelend) *labelend = 0x00;
                WINE_TRACE("comparing found label %s\n", wine_dbgstr_w(str));

                if (lstrcmpiW (str, paramStart) == 0) return;
              }

              /* See if we have gone beyond the end point if second time through */
              if (loop==1) {
                LARGE_INTEGER curli;
                curli.QuadPart = 0;
                curli.u.LowPart = SetFilePointer(context -> h, curli.u.LowPart,
                                                &curli.u.HighPart, FILE_CURRENT);
                if (curli.QuadPart > startli.QuadPart) {
                  WINE_TRACE("Reached wrap point, label not found\n");
                  break;
                }
              }
            }
        }
    }

    WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOTARGET));
    context -> skip_rest = TRUE;
  }
  return;
}

/*****************************************************************************
 * WCMD_pushd
 *
 *	Push a directory onto the stack
 */

void WCMD_pushd (const WCHAR *args)
{
    struct env_stack *curdir;
    WCHAR *thisdir;

    if (wcschr(args, '/') != NULL) {
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
    lstrcpyW(quals, L"/D");
    GetCurrentDirectoryW (1024, thisdir);
    errorlevel = 0;
    WCMD_setshow_default(args);
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

/*******************************************************************
 * evaluate_if_comparison
 *
 * Evaluates an "if" comparison operation
 *
 * PARAMS
 *  leftOperand     [I] left operand, non NULL
 *  operator        [I] "if" binary comparison operator, non NULL
 *  rightOperand    [I] right operand, non NULL
 *  caseInsensitive [I] 0 for case sensitive comparison, anything else for insensitive
 *
 * RETURNS
 *  Success:  1 if operator applied to the operands evaluates to TRUE
 *            0 if operator applied to the operands evaluates to FALSE
 *  Failure: -1 if operator is not recognized
 */
static int evaluate_if_comparison(const WCHAR *leftOperand, const WCHAR *operator,
                                  const WCHAR *rightOperand, int caseInsensitive)
{
    WCHAR *endptr_leftOp, *endptr_rightOp;
    long int leftOperand_int, rightOperand_int;
    BOOL int_operands;

    /* == is a special case, as it always compares strings */
    if (!lstrcmpiW(operator, L"=="))
        return caseInsensitive ? lstrcmpiW(leftOperand, rightOperand) == 0
                               : lstrcmpW (leftOperand, rightOperand) == 0;

    /* Check if we have plain integers (in decimal, octal or hexadecimal notation) */
    leftOperand_int = wcstol(leftOperand, &endptr_leftOp, 0);
    rightOperand_int = wcstol(rightOperand, &endptr_rightOp, 0);
    int_operands = (!*endptr_leftOp) && (!*endptr_rightOp);

    /* Perform actual (integer or string) comparison */
    if (!lstrcmpiW(operator, L"lss")) {
        if (int_operands)
            return leftOperand_int < rightOperand_int;
        else
            return caseInsensitive ? lstrcmpiW(leftOperand, rightOperand) < 0
                                   : lstrcmpW (leftOperand, rightOperand) < 0;
    }

    if (!lstrcmpiW(operator, L"leq")) {
        if (int_operands)
            return leftOperand_int <= rightOperand_int;
        else
            return caseInsensitive ? lstrcmpiW(leftOperand, rightOperand) <= 0
                                   : lstrcmpW (leftOperand, rightOperand) <= 0;
    }

    if (!lstrcmpiW(operator, L"equ")) {
        if (int_operands)
            return leftOperand_int == rightOperand_int;
        else
            return caseInsensitive ? lstrcmpiW(leftOperand, rightOperand) == 0
                                   : lstrcmpW (leftOperand, rightOperand) == 0;
    }

    if (!lstrcmpiW(operator, L"neq")) {
        if (int_operands)
            return leftOperand_int != rightOperand_int;
        else
            return caseInsensitive ? lstrcmpiW(leftOperand, rightOperand) != 0
                                   : lstrcmpW (leftOperand, rightOperand) != 0;
    }

    if (!lstrcmpiW(operator, L"geq")) {
        if (int_operands)
            return leftOperand_int >= rightOperand_int;
        else
            return caseInsensitive ? lstrcmpiW(leftOperand, rightOperand) >= 0
                                   : lstrcmpW (leftOperand, rightOperand) >= 0;
    }

    if (!lstrcmpiW(operator, L"gtr")) {
        if (int_operands)
            return leftOperand_int > rightOperand_int;
        else
            return caseInsensitive ? lstrcmpiW(leftOperand, rightOperand) > 0
                                   : lstrcmpW (leftOperand, rightOperand) > 0;
    }

    return -1;
}

int evaluate_if_condition(WCHAR *p, WCHAR **command, int *test, int *negate)
{
  WCHAR condition[MAX_PATH];
  int caseInsensitive = (wcsstr(quals, L"/I") != NULL);

  *negate = !lstrcmpiW(param1,L"not");
  lstrcpyW(condition, (*negate ? param2 : param1));
  WINE_TRACE("Condition: %s\n", wine_dbgstr_w(condition));

  if (!lstrcmpiW(condition, L"errorlevel")) {
    WCHAR *param = WCMD_parameter(p, 1+(*negate), NULL, FALSE, FALSE);
    WCHAR *endptr;
    long int param_int = wcstol(param, &endptr, 10);
    if (*endptr) goto syntax_err;
    *test = ((long int)errorlevel >= param_int);
    WCMD_parameter(p, 2+(*negate), command, FALSE, FALSE);
  }
  else if (!lstrcmpiW(condition, L"exist")) {
    WIN32_FIND_DATAW fd;
    HANDLE hff;
    WCHAR *param = WCMD_parameter(p, 1+(*negate), NULL, FALSE, FALSE);
    int    len = lstrlenW(param);

    if (!len) {
        *test = FALSE;
    } else {
        /* FindFirstFile does not like a directory path ending in '\' or '/', so append a '.' */
        if (param[len-1] == '\\' || param[len-1] == '/') wcscat(param, L".");

        hff = FindFirstFileW(param, &fd);
        *test = (hff != INVALID_HANDLE_VALUE);
        if (*test) FindClose(hff);
    }

    WCMD_parameter(p, 2+(*negate), command, FALSE, FALSE);
  }
  else if (!lstrcmpiW(condition, L"defined")) {
    *test = (GetEnvironmentVariableW(WCMD_parameter(p, 1+(*negate), NULL, FALSE, FALSE),
                                    NULL, 0) > 0);
    WCMD_parameter(p, 2+(*negate), command, FALSE, FALSE);
  }
  else { /* comparison operation */
    WCHAR leftOperand[MAXSTRING], rightOperand[MAXSTRING], operator[MAXSTRING];
    WCHAR *paramStart;

    lstrcpyW(leftOperand, WCMD_parameter(p, (*negate)+caseInsensitive, &paramStart, TRUE, FALSE));
    if (!*leftOperand)
      goto syntax_err;

    /* Note: '==' can't be returned by WCMD_parameter since '=' is a separator */
    p = paramStart + lstrlenW(leftOperand);
    while (*p == ' ' || *p == '\t')
      p++;

    if (!wcsncmp(p, L"==", lstrlenW(L"==")))
      lstrcpyW(operator, L"==");
    else {
      lstrcpyW(operator, WCMD_parameter(p, 0, &paramStart, FALSE, FALSE));
      if (!*operator) goto syntax_err;
    }
    p += lstrlenW(operator);

    lstrcpyW(rightOperand, WCMD_parameter(p, 0, &paramStart, TRUE, FALSE));
    if (!*rightOperand)
      goto syntax_err;

    *test = evaluate_if_comparison(leftOperand, operator, rightOperand, caseInsensitive);
    if (*test == -1)
      goto syntax_err;

    p = paramStart + lstrlenW(rightOperand);
    WCMD_parameter(p, 0, command, FALSE, FALSE);
  }

  return 1;

syntax_err:
  return -1;
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
void WCMD_if (WCHAR *p, CMD_LIST **cmdList)
{
  int negate; /* Negate condition */
  int test;   /* Condition evaluation result */
  WCHAR *command;

  /* Function evaluate_if_condition relies on the global variables quals, param1 and param2
     set in a call to WCMD_parse before */
  if (evaluate_if_condition(p, &command, &test, &negate) == -1)
      goto syntax_err;

  WINE_TRACE("p: %s, quals: %s, param1: %s, param2: %s, command: %s\n",
             wine_dbgstr_w(p), wine_dbgstr_w(quals), wine_dbgstr_w(param1),
             wine_dbgstr_w(param2), wine_dbgstr_w(command));

  /* Process rest of IF statement which is on the same line
     Note: This may process all or some of the cmdList (eg a GOTO) */
  WCMD_part_execute(cmdList, command, TRUE, (test != negate));
  return;

syntax_err:
  WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
}

/****************************************************************************
 * WCMD_move
 *
 * Move a file, directory tree or wildcarded set of files.
 */

void WCMD_move (void)
{
  BOOL             status;
  WIN32_FIND_DATAW fd;
  HANDLE          hff;
  WCHAR            input[MAX_PATH];
  WCHAR            output[MAX_PATH];
  WCHAR            drive[10];
  WCHAR            dir[MAX_PATH];
  WCHAR            fname[MAX_PATH];
  WCHAR            ext[MAX_PATH];

  if (param1[0] == 0x00) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
    return;
  }

  /* If no destination supplied, assume current directory */
  if (param2[0] == 0x00) {
      lstrcpyW(param2, L".");
  }

  /* If 2nd parm is directory, then use original filename */
  /* Convert partial path to full path */
  if (!WCMD_get_fullpath(param1, ARRAY_SIZE(input), input, NULL) ||
      !WCMD_get_fullpath(param2, ARRAY_SIZE(output), output, NULL)) return;
  WINE_TRACE("Move from '%s'('%s') to '%s'\n", wine_dbgstr_w(input),
             wine_dbgstr_w(param1), wine_dbgstr_w(output));

  /* Split into components */
  _wsplitpath(input, drive, dir, fname, ext);

  hff = FindFirstFileW(input, &fd);
  if (hff == INVALID_HANDLE_VALUE)
    return;

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

    if (ok) {
      status = MoveFileExW(src, dest, flags);
    } else {
      status = TRUE;
    }

    if (!status) {
      WCMD_print_error ();
      errorlevel = 1;
    }
  } while (FindNextFileW(hff, &fd) != 0);

  FindClose(hff);
}

/****************************************************************************
 * WCMD_pause
 *
 * Suspend execution of a batch script until a key is typed
 */

void WCMD_pause (void)
{
  DWORD oldmode;
  BOOL have_console;
  DWORD count;
  WCHAR key;
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

  have_console = GetConsoleMode(hIn, &oldmode);
  if (have_console)
      SetConsoleMode(hIn, 0);

  WCMD_output_asis(anykey);
  WCMD_ReadFile(hIn, &key, 1, &count);
  if (have_console)
    SetConsoleMode(hIn, oldmode);
}

/****************************************************************************
 * WCMD_remove_dir
 *
 * Delete a directory.
 */

void WCMD_remove_dir (WCHAR *args) {

  int   argno         = 0;
  int   argsProcessed = 0;
  WCHAR *argN          = args;

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
        if (!RemoveDirectoryW(thisArg)) WCMD_print_error ();

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
          if (!ok) return;
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
    return;
  }

}

/****************************************************************************
 * WCMD_rename
 *
 * Rename a file.
 */

void WCMD_rename (void)
{
  BOOL             status;
  HANDLE          hff;
  WIN32_FIND_DATAW fd;
  WCHAR            input[MAX_PATH];
  WCHAR           *dotDst = NULL;
  WCHAR            drive[10];
  WCHAR            dir[MAX_PATH];
  WCHAR            fname[MAX_PATH];
  WCHAR            ext[MAX_PATH];

  errorlevel = 0;

  /* Must be at least two args */
  if (param1[0] == 0x00 || param2[0] == 0x00) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
    errorlevel = 1;
    return;
  }

  /* Destination cannot contain a drive letter or directory separator */
  if ((wcschr(param2,':') != NULL) || (wcschr(param2,'\\') != NULL)) {
      SetLastError(ERROR_INVALID_PARAMETER);
      WCMD_print_error();
      errorlevel = 1;
      return;
  }

  /* Convert partial path to full path */
  if (!WCMD_get_fullpath(param1, ARRAY_SIZE(input), input, NULL)) return;
  WINE_TRACE("Rename from '%s'('%s') to '%s'\n", wine_dbgstr_w(input),
             wine_dbgstr_w(param1), wine_dbgstr_w(param2));
  dotDst = wcschr(param2, '.');

  /* Split into components */
  _wsplitpath(input, drive, dir, fname, ext);

  hff = FindFirstFileW(input, &fd);
  if (hff == INVALID_HANDLE_VALUE)
    return;

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

    status = MoveFileW(src, dest);

    if (!status) {
      WCMD_print_error ();
      errorlevel = 1;
    }
  } while (FindNextFileW(hff, &fd) != 0);

  FindClose(hff);
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
  BOOL newdelay;

  /* setlocal does nothing outside of batch programs */
  if (!context) return;

  /* DISABLEEXTENSIONS ignored */

  /* ENABLEDELAYEDEXPANSION / DISABLEDELAYEDEXPANSION could be parm1 or parm2
     (if both ENABLEEXTENSIONS and ENABLEDELAYEDEXPANSION supplied for example) */
  if (!wcsicmp(param1, L"ENABLEDELAYEDEXPANSION") || !wcsicmp(param2, L"ENABLEDELAYEDEXPANSION")) {
    newdelay = TRUE;
  } else if (!wcsicmp(param1, L"DISABLEDELAYEDEXPANSION") || !wcsicmp(param2, L"DISABLEDELAYEDEXPANSION")) {
    newdelay = FALSE;
  } else {
    newdelay = delayedsubst;
  }
  WINE_TRACE("Setting delayed expansion to %d\n", newdelay);

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
    env_copy->batchhandle = context->h;
    env_copy->next = saved_environment;
    env_copy->delayedsubst = delayedsubst;
    delayedsubst = newdelay;
    saved_environment = env_copy;

    /* Save the current drive letter */
    GetCurrentDirectoryW(MAX_PATH, cwd);
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

  /* setlocal does nothing outside of batch programs */
  if (!context) return;

  /* setlocal needs a saved environment from within the same context (batch
     program) as it was saved in                                            */
  if (!saved_environment || saved_environment->batchhandle != context->h)
    return;

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
  LocalFree (old);
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

  LocalFree (env);
  LocalFree (temp);
}

/*****************************************************************************
 * WCMD_setshow_default
 *
 *	Set/Show the current default directory
 */

void WCMD_setshow_default (const WCHAR *args) {

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
          if (!WCMD_get_fullpath(string, ARRAY_SIZE(fpath), fpath, NULL)) return;
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
      errorlevel = 1;
      WCMD_print_error ();
      return;
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
    WCMD_output_stderr (WCMD_LoadMessage(WCMD_NYI));
  }
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
    len += (lstrlenW(&s[len]) + 1);
    count++;
  }

  /* add the strings to an array */
  str = LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, count * sizeof (WCHAR*) );
  if( !str )
    return 0;
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

  LocalFree( str );
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

void WCMD_setshow_env (WCHAR *s) {

  LPVOID env;
  WCHAR *p;
  BOOL status;
  WCHAR string[MAXSTRING];

  if (param1[0] == 0x00 && quals[0] == 0x00) {
    env = GetEnvironmentStringsW();
    WCMD_setshow_sortenv( env, NULL );
    return;
  }

  /* See if /P supplied, and if so echo the prompt, and read in a reply */
  if (CompareStringW(LOCALE_USER_DEFAULT,
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
    if (!(*s) || ((p = wcschr (s, '=')) == NULL )) {
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
      return;
    }

    /* Output the prompt */
    *p++ = '\0';
    if (*p) WCMD_output_asis(p);

    /* Read the reply */
    if (WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), string, ARRAY_SIZE(string), &count) && count > 1) {
      string[count-1] = '\0'; /* ReadFile output is not null-terminated! */
      if (string[count-2] == '\r') string[count-2] = '\0'; /* Under Windoze we get CRLF! */
      WINE_TRACE("set /p: Setting var '%s' to '%s'\n", wine_dbgstr_w(s),
                 wine_dbgstr_w(string));
      SetEnvironmentVariableW(s, string);
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
      return;
    }

    /* If we have no context (interactive or cmd.exe /c) print the final result */
    if (!context) {
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
      env = GetEnvironmentStringsW();
      if (WCMD_setshow_sortenv( env, s ) == 0) {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_MISSINGENV), s);
        errorlevel = 1;
      }
      return;
    }
    *p++ = '\0';

    if (!*p) p = NULL;
    WINE_TRACE("set: Setting var '%s' to '%s'\n", wine_dbgstr_w(s),
               wine_dbgstr_w(p));
    status = SetEnvironmentVariableW(s, p);
    gle = GetLastError();
    if ((!status) & (gle == ERROR_ENVVAR_NOT_FOUND)) {
      errorlevel = 1;
    } else if (!status) WCMD_print_error();
    else if (!interactive) errorlevel = 0;
  }
}

/****************************************************************************
 * WCMD_setshow_path
 *
 * Set/Show the path environment variable
 */

void WCMD_setshow_path (const WCHAR *args) {

  WCHAR string[1024];
  DWORD status;

  if (!*param1 && !*param2) {
    status = GetEnvironmentVariableW(L"PATH", string, ARRAY_SIZE(string));
    if (status != 0) {
      WCMD_output_asis(L"PATH=");
      WCMD_output_asis ( string);
      WCMD_output_asis(L"\r\n");
    }
    else {
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOPATH));
    }
  }
  else {
    if (*args == '=') args++; /* Skip leading '=' */
    status = SetEnvironmentVariableW(L"PATH", args);
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
    WCMD_output_stderr (WCMD_LoadMessage(WCMD_NYI));
  }
}

/****************************************************************************
 * WCMD_shift
 *
 * Shift batch parameters.
 * Optional /n says where to start shifting (n=0-8)
 */

void WCMD_shift (const WCHAR *args) {
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
 * WCMD_start
 */
void WCMD_start(WCHAR *args)
{
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
        WaitForSingleObject( pi.hProcess, INFINITE );
        GetExitCodeProcess( pi.hProcess, &errorlevel );
        if (errorlevel == STILL_ACTIVE) errorlevel = 0;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        SetLastError(ERROR_FILE_NOT_FOUND);
        WCMD_print_error ();
        errorlevel = 9009;
    }
    free(cmdline);
}

/****************************************************************************
 * WCMD_title
 *
 * Set the console title
 */
void WCMD_title (const WCHAR *args) {
  SetConsoleTitleW(args);
}

/****************************************************************************
 * WCMD_type
 *
 * Copy a file to standard output.
 */

void WCMD_type (WCHAR *args) {

  int   argno         = 0;
  WCHAR *argN          = args;
  BOOL  writeHeaders  = FALSE;

  if (param1[0] == 0x00) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
    return;
  }

  if (param2[0] != 0x00) writeHeaders = TRUE;

  /* Loop through all args */
  errorlevel = 0;
  while (argN) {
    WCHAR *thisArg = WCMD_parameter (args, argno++, &argN, FALSE, FALSE);

    HANDLE h;
    WCHAR buffer[512];
    DWORD count;

    if (!argN) break;

    WINE_TRACE("type: Processing arg '%s'\n", wine_dbgstr_w(thisArg));
    h = CreateFileW(thisArg, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
      WCMD_print_error ();
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_READFAIL), thisArg);
      errorlevel = 1;
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
}

/****************************************************************************
 * WCMD_more
 *
 * Output either a file or stdin to screen in pages
 */

void WCMD_more (WCHAR *args) {

  int   argno         = 0;
  WCHAR *argN         = args;
  WCHAR  moreStr[100];
  WCHAR  moreStrPage[100];
  WCHAR  buffer[512];
  DWORD count;

  /* Prefix the NLS more with '-- ', then load the text */
  errorlevel = 0;
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

    return;
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
        errorlevel = 1;
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
}

/****************************************************************************
 * WCMD_verify
 *
 * Display verify flag.
 * FIXME: We don't actually do anything with the verify flag other than toggle
 * it...
 */

void WCMD_verify (const WCHAR *args) {

  int count;

  count = lstrlenW(args);
  if (count == 0) {
    if (verify_mode) WCMD_output(WCMD_LoadMessage(WCMD_VERIFYPROMPT), L"ON");
    else WCMD_output (WCMD_LoadMessage(WCMD_VERIFYPROMPT), L"OFF");
    return;
  }
  if (lstrcmpiW(args, L"ON") == 0) {
    verify_mode = TRUE;
    return;
  }
  else if (lstrcmpiW(args, L"OFF") == 0) {
    verify_mode = FALSE;
    return;
  }
  else WCMD_output_stderr(WCMD_LoadMessage(WCMD_VERIFYERR));
}

/****************************************************************************
 * WCMD_version
 *
 * Display version info.
 */

void WCMD_version (void) {

  WCMD_output_asis (version_string);

}

/****************************************************************************
 * WCMD_volume
 *
 * Display volume information (set_label = FALSE)
 * Additionally set volume label (set_label = TRUE)
 * Returns 1 on success, 0 otherwise
 */

int WCMD_volume(BOOL set_label, const WCHAR *path)
{
  DWORD count, serial;
  WCHAR string[MAX_PATH], label[MAX_PATH], curdir[MAX_PATH];
  BOOL status;

  if (!*path) {
    status = GetCurrentDirectoryW(ARRAY_SIZE(curdir), curdir);
    if (!status) {
      WCMD_print_error ();
      return 0;
    }
    status = GetVolumeInformationW(NULL, label, ARRAY_SIZE(label), &serial, NULL, NULL, NULL, 0);
  }
  else {
    if ((path[1] != ':') || (lstrlenW(path) != 2)) {
      WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
      return 0;
    }
    wsprintfW (curdir, L"%s\\", path);
    status = GetVolumeInformationW(curdir, label, ARRAY_SIZE(label), &serial, NULL, NULL, NULL, 0);
  }
  if (!status) {
    WCMD_print_error ();
    return 0;
  }
  if (label[0] != '\0') {
    WCMD_output (WCMD_LoadMessage(WCMD_VOLUMELABEL),
      	curdir[0], label);
  }
  else {
    WCMD_output (WCMD_LoadMessage(WCMD_VOLUMENOLABEL),
      	curdir[0]);
  }
  WCMD_output (WCMD_LoadMessage(WCMD_VOLUMESERIALNO),
    	HIWORD(serial), LOWORD(serial));
  if (set_label) {
    WCMD_output (WCMD_LoadMessage(WCMD_VOLUMEPROMPT));
    if (WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), string, ARRAY_SIZE(string), &count) &&
        count > 1) {
      string[count-1] = '\0';		/* ReadFile output is not null-terminated! */
      if (string[count-2] == '\r') string[count-2] = '\0'; /* Under Windoze we get CRLF! */
    }
    if (*path) {
      if (!SetVolumeLabelW(curdir, string)) WCMD_print_error ();
    }
    else {
      if (!SetVolumeLabelW(NULL, string)) WCMD_print_error ();
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
    int rc = wcstol(param1, NULL, 10); /* Note: wcstol of empty parameter is 0 */

    if (context && lstrcmpiW(quals, L"/B") == 0) {
        errorlevel = rc;
        context -> skip_rest = TRUE;
        *cmdList = NULL;
    } else {
        ExitProcess(rc);
    }
}


/*****************************************************************************
 * WCMD_assoc
 *
 *	Lists or sets file associations  (assoc = TRUE)
 *      Lists or sets file types         (assoc = FALSE)
 */
void WCMD_assoc (const WCHAR *args, BOOL assoc) {

    HKEY    key;
    DWORD   accessOptions = KEY_READ;
    WCHAR   *newValue;
    LONG    rc = ERROR_SUCCESS;
    WCHAR    keyValue[MAXSTRING];
    DWORD   valueLen;
    HKEY    readKey;

    /* See if parameter includes '=' */
    errorlevel = 0;
    newValue = wcschr(args, '=');
    if (newValue) accessOptions |= KEY_WRITE;

    /* Open a key to HKEY_CLASSES_ROOT for enumerating */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"", 0, accessOptions, &key) != ERROR_SUCCESS) {
      WINE_FIXME("Unexpected failure opening HKCR key: %ld\n", GetLastError());
      return;
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

        if (RegOpenKeyExW(key, subkey, 0, accessOptions, &readKey) == ERROR_SUCCESS) {

          valueLen = sizeof(keyValue);
          rc = RegQueryValueExW(readKey, NULL, NULL, NULL, (LPBYTE)keyValue, &valueLen);
          WCMD_output_asis(args);
          WCMD_output_asis(L"=");
          /* If no default value found, leave line empty after '=' */
          if (rc == ERROR_SUCCESS) WCMD_output_asis(keyValue);
          WCMD_output_asis(L"\r\n");
          RegCloseKey(readKey);

        } else {
          WCHAR  msgbuffer[MAXSTRING];

          /* Load the translated 'File association not found' */
          if (assoc) {
            LoadStringW(hinst, WCMD_NOASSOC, msgbuffer, ARRAY_SIZE(msgbuffer));
          } else {
            LoadStringW(hinst, WCMD_NOFTYPE, msgbuffer, ARRAY_SIZE(msgbuffer));
          }
          WCMD_output_stderr(msgbuffer, keyValue);
          errorlevel = 2;
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

        /* If nothing after '=' then clear value - only valid for ASSOC */
        if (*newValue == 0x00) {

          if (assoc) rc = RegDeleteKeyW(key, args);
          if (assoc && rc == ERROR_SUCCESS) {
            WINE_TRACE("HKCR Key '%s' deleted\n", wine_dbgstr_w(args));

          } else if (assoc && rc != ERROR_FILE_NOT_FOUND) {
            WCMD_print_error();
            errorlevel = 2;

          } else {
            WCHAR  msgbuffer[MAXSTRING];

            /* Load the translated 'File association not found' */
            if (assoc) {
              LoadStringW(hinst, WCMD_NOASSOC, msgbuffer, ARRAY_SIZE(msgbuffer));
            } else {
              LoadStringW(hinst, WCMD_NOFTYPE, msgbuffer, ARRAY_SIZE(msgbuffer));
            }
            WCMD_output_stderr(msgbuffer, args);
            errorlevel = 2;
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
            errorlevel = 2;
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
}

/****************************************************************************
 * WCMD_color
 *
 * Colors the terminal screen.
 */

void WCMD_color (void) {

  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
  HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

  if (param1[0] != 0x00 && lstrlenW(param1) > 2) {
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_ARGERR));
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
        color = wcstoul(param1, NULL, 16);
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

/****************************************************************************
 * WCMD_mklink
 */

void WCMD_mklink(WCHAR *args)
{
    int   argno = 0;
    WCHAR *argN = args;
    BOOL isdir = FALSE;
    BOOL junction = FALSE;
    BOOL hard = FALSE;
    BOOL ret = FALSE;
    WCHAR file1[MAX_PATH];
    WCHAR file2[MAX_PATH];

    if (param1[0] == 0x00 || param2[0] == 0x00) {
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_NOARG));
        return;
    }

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
        else {
            if(!file1[0])
                lstrcpyW(file1, thisArg);
            else
                lstrcpyW(file2, thisArg);
        }
    }

    if(hard)
        ret = CreateHardLinkW(file1, file2, NULL);
    else if(!junction)
        ret = CreateSymbolicLinkW(file1, file2, isdir);
    else
        WINE_TRACE("Juction links currently not supported.\n");

    if(!ret)
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_READFAIL), file1);
}
