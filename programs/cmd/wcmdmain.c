/*
 * CMD - Wine-compatible command line interface.
 *
 * Copyright (C) 1999 - 2001 D A Pickles
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
 * - Cannot handle parameters in quotes
 * - Lots of functionality missing from builtins
 */

#include <time.h>
#include "wcmd.h"
#include "shellapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cmd);

extern const WCHAR inbuilt[][10];
extern struct env_stack *pushd_directories;

BATCH_CONTEXT *context = NULL;
int errorlevel;
WCHAR quals[MAXSTRING], param1[MAXSTRING], param2[MAXSTRING];
BOOL  interactive;
FOR_CONTEXT *forloopcontext; /* The 'for' loop context */
BOOL delayedsubst = FALSE; /* The current delayed substitution setting */

int defaultColor = 7;
BOOL echo_mode = TRUE;

WCHAR anykey[100], version_string[100];

static BOOL opt_c, opt_k, opt_s, unicodeOutput = FALSE;

/* Variables pertaining to paging */
static BOOL paged_mode;
static const WCHAR *pagedMessage = NULL;
static int line_count;
static int max_height;
static int max_width;
static int numChars;

static HANDLE control_c_event;

#define MAX_WRITECONSOLE_SIZE 65535

/*
 * Returns a buffer for reading from/writing to file
 * Never freed
 */
static char *get_file_buffer(void)
{
    static char *output_bufA = NULL;
    if (!output_bufA)
        output_bufA = xalloc(MAX_WRITECONSOLE_SIZE);
    return output_bufA;
}

/*******************************************************************
 * WCMD_output_asis_len - send output to current standard output
 *
 * Output a formatted unicode string. Ideally this will go to the console
 *  and hence required WriteConsoleW to output it, however if file i/o is
 *  redirected, it needs to be WriteFile'd using OEM (not ANSI) format
 */
static void WCMD_output_asis_len(const WCHAR *message, DWORD len, HANDLE device)
{
    DWORD   nOut= 0;
    DWORD   res = 0;

    /* If nothing to write, return (MORE does this sometimes) */
    if (!len) return;

    /* Try to write as unicode assuming it is to a console */
    res = WriteConsoleW(device, message, len, &nOut, NULL);

    /* If writing to console fails, assume it's file
       i/o so convert to OEM codepage and output                  */
    if (!res) {
      BOOL usedDefaultChar = FALSE;
      DWORD convertedChars;
      char *buffer;

      if (!unicodeOutput) {

        if (!(buffer = get_file_buffer()))
            return;

        /* Convert to OEM, then output */
        convertedChars = WideCharToMultiByte(GetConsoleOutputCP(), 0, message,
                            len, buffer, MAX_WRITECONSOLE_SIZE,
                            "?", &usedDefaultChar);
        WriteFile(device, buffer, convertedChars,
                  &nOut, FALSE);
      } else {
        WriteFile(device, message, len*sizeof(WCHAR),
                  &nOut, FALSE);
      }
    }
    return;
}

/*******************************************************************
 * WCMD_output - send output to current standard output device.
 *
 */

void WINAPIV WCMD_output (const WCHAR *format, ...) {

  va_list ap;
  WCHAR* string;
  DWORD len;

  va_start(ap,format);
  string = NULL;
  len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       format, 0, 0, (LPWSTR)&string, 0, &ap);
  va_end(ap);
  if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
    WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(format));
  else
  {
    WCMD_output_asis_len(string, len, GetStdHandle(STD_OUTPUT_HANDLE));
    LocalFree(string);
  }
}

/*******************************************************************
 * WCMD_output_stderr - send output to current standard error device.
 *
 */

void WINAPIV WCMD_output_stderr (const WCHAR *format, ...) {

  va_list ap;
  WCHAR* string;
  DWORD len;

  va_start(ap,format);
  string = NULL;
  len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       format, 0, 0, (LPWSTR)&string, 0, &ap);
  va_end(ap);
  if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
    WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(format));
  else
  {
    WCMD_output_asis_len(string, len, GetStdHandle(STD_ERROR_HANDLE));
    LocalFree(string);
  }
}

/*******************************************************************
 * WCMD_format_string - allocate a buffer and format a string
 *
 */

WCHAR* WINAPIV WCMD_format_string (const WCHAR *format, ...)
{
  va_list ap;
  WCHAR* string;
  DWORD len;

  va_start(ap,format);
  len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       format, 0, 0, (LPWSTR)&string, 0, &ap);
  va_end(ap);
  if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE) {
    WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(format));
    string = (WCHAR*)LocalAlloc(LMEM_FIXED, 2);
    *string = 0;
  }
  return string;
}

void WCMD_enter_paged_mode(const WCHAR *msg)
{
  CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo)) {
    /* Use console window dimensions, not screen buffer dimensions. */
    max_height = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1;
    max_width  = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1;
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

static BOOL has_pending_char_events(HANDLE h)
{
    INPUT_RECORD ir;
    DWORD count;
    BOOL ret = FALSE;

    while (!ret && GetNumberOfConsoleInputEvents(h, &count) && count)
    {
        /* FIXME could be racy if another thread/process gets the input record */
        if (ReadConsoleInputA(h, &ir, 1, &count) && count)
            ret = ir.EventType == KEY_EVENT &&
                ir.Event.KeyEvent.bKeyDown &&
                ir.Event.KeyEvent.uChar.AsciiChar;
    }
    return ret;
}

/***************************************************************************
 * WCMD_wait_for_input
 *
 * Wait for input from a console/file.
 * Used by commands like PAUSE and DIR /P that need to wait for user
 * input before continuing.
 */
RETURN_CODE WCMD_wait_for_input(HANDLE hIn)
{
    RETURN_CODE return_code;
    DWORD oldmode;
    DWORD count;
    WCHAR key;

    return_code = ERROR_INVALID_FUNCTION;
    if (GetConsoleMode(hIn, &oldmode))
    {
        HANDLE h[2] = {hIn, control_c_event};

        SetConsoleMode(hIn, oldmode & ~ENABLE_LINE_INPUT);
        FlushConsoleInputBuffer(hIn);
        while (return_code == ERROR_INVALID_FUNCTION)
        {
            switch (WaitForMultipleObjects(2, h, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0:
                if (has_pending_char_events(hIn))
                    return_code = NO_ERROR;
                /* will make both hIn no long signaled, and also process the pending input record */
                FlushConsoleInputBuffer(hIn);
                break;
            case WAIT_OBJECT_0 + 1:
                return_code = STATUS_CONTROL_C_EXIT;
                break;
            default: break;
            }
        }
        SetConsoleMode(hIn, oldmode);
    }
    else if (WCMD_ReadFile(hIn, &key, 1, &count) && count)
        return_code = NO_ERROR;
    else
        return_code = ERROR_INVALID_FUNCTION;
    return return_code;
}

/***************************************************************************
 * WCMD_ReadFile
 *
 *	Read characters in from a console/file, returning result in Unicode
 */
BOOL WCMD_ReadFile(const HANDLE hIn, WCHAR *intoBuf, const DWORD maxChars, LPDWORD charsRead)
{
    DWORD numRead;
    char *buffer;

    /* Try to read from console as Unicode */
    if (VerifyConsoleIoHandle(hIn) && ReadConsoleW(hIn, intoBuf, maxChars, charsRead, NULL)) return TRUE;

    /* We assume it's a file handle and read then convert from assumed OEM codepage */
    if (!(buffer = get_file_buffer()))
        return FALSE;

    if (!ReadFile(hIn, buffer, maxChars, &numRead, NULL))
        return FALSE;

    *charsRead = MultiByteToWideChar(GetConsoleCP(), 0, buffer, numRead, intoBuf, maxChars);

    return TRUE;
}

/*******************************************************************
 * WCMD_output_asis_handle
 *
 * Send output to specified handle without formatting e.g. when message contains '%'
 */
static RETURN_CODE WCMD_output_asis_handle (DWORD std_handle, const WCHAR *message) {
  RETURN_CODE return_code = NO_ERROR;
  const WCHAR* ptr;
  HANDLE handle = GetStdHandle(std_handle);

  if (paged_mode) {
    do {
      ptr = message;
      while (*ptr && *ptr!='\n' && (numChars < max_width)) {
        numChars++;
        ptr++;
      };
      if (*ptr == '\n') ptr++;
      WCMD_output_asis_len(message, ptr - message, handle);
      numChars = 0;
      if (++line_count >= max_height - 1) {
        line_count = 0;
        WCMD_output_asis_len(pagedMessage, lstrlenW(pagedMessage), handle);
        return_code = WCMD_wait_for_input(GetStdHandle(STD_INPUT_HANDLE));
        WCMD_output_asis_len(L"\r\n", 2, handle);
        if (return_code)
          break;
      }
    } while (((message = ptr) != NULL) && (*ptr));
  } else {
    WCMD_output_asis_len(message, lstrlenW(message), handle);
  }

  return return_code;
}

/*******************************************************************
 * WCMD_output_asis
 *
 * Send output to current standard output device, without formatting
 * e.g. when message contains '%'
 */
RETURN_CODE WCMD_output_asis (const WCHAR *message) {
    return WCMD_output_asis_handle(STD_OUTPUT_HANDLE, message);
}

/*******************************************************************
 * WCMD_output_asis_stderr
 *
 * Send output to current standard error device, without formatting
 * e.g. when message contains '%'
 */
RETURN_CODE WCMD_output_asis_stderr (const WCHAR *message) {
    return WCMD_output_asis_handle(STD_ERROR_HANDLE, message);
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
  status = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			  NULL, error_code, 0, (LPWSTR) &lpMsgBuf, 0, NULL);
  if (!status) {
    WINE_FIXME ("Cannot display message for error %ld, status %ld\n",
			error_code, GetLastError());
    return;
  }

  WCMD_output_asis_len(lpMsgBuf, lstrlenW(lpMsgBuf),
                       GetStdHandle(STD_ERROR_HANDLE));
  LocalFree (lpMsgBuf);
  WCMD_output_asis_len(L"\r\n", lstrlenW(L"\r\n"), GetStdHandle(STD_ERROR_HANDLE));
  return;
}

/******************************************************************************
 * WCMD_show_prompt
 *
 *	Display the prompt on STDout
 *
 */

static void WCMD_show_prompt(void)
{
  int status;
  WCHAR out_string[MAX_PATH], curdir[MAX_PATH], prompt_string[MAX_PATH];
  WCHAR *p, *q;
  DWORD len;

  len = GetEnvironmentVariableW(L"PROMPT", prompt_string, ARRAY_SIZE(prompt_string));
  if ((len == 0) || (len >= ARRAY_SIZE(prompt_string))) {
    lstrcpyW(prompt_string, L"$P$G");
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
      switch (towupper(*p)) {
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
	  GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, q, MAX_PATH - (q - out_string));
	  while (*q) q++;
	  break;
	case 'E':
	  *q++ = '\x1b';
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
          status = GetCurrentDirectoryW(ARRAY_SIZE(curdir), curdir);
	  if (status) {
	    *q++ = curdir[0];
	  }
	  break;
	case 'P':
          status = GetCurrentDirectoryW(ARRAY_SIZE(curdir), curdir);
	  if (status) {
	    lstrcatW (q, curdir);
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
	  GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, NULL, q, MAX_PATH);
	  while (*q) q++;
	  break;
        case 'V':
	  lstrcatW (q, version_string);
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

void *xrealloc(void *ptr, size_t size)
{
    void *ret;

    if (!(ret = realloc(ptr, size)))
    {
        ERR("Out of memory\n");
        ExitProcess(1);
    }

    return ret;
}

/*************************************************************************
 * WCMD_strsubstW
 *    Replaces a portion of a Unicode string with the specified string.
 *    It's up to the caller to ensure there is enough space in the
 *    destination buffer.
 */
WCHAR *WCMD_strsubstW(WCHAR *start, const WCHAR *next, const WCHAR *insert, int len) {

   if (len < 0)
      len=insert ? lstrlenW(insert) : 0;
   if (start+len != next)
       memmove(start+len, next, (lstrlenW(next) + 1) * sizeof(*next));
   if (insert)
       memcpy(start, insert, len * sizeof(*insert));
   return start + len;
}

BOOL WCMD_get_fullpath(const WCHAR* in, SIZE_T outsize, WCHAR* out, WCHAR** start)
{
    DWORD ret = GetFullPathNameW(in, outsize, out, start);
    if (!ret) return FALSE;
    if (ret > outsize)
    {
        WCMD_output_asis_stderr(WCMD_LoadMessage(WCMD_FILENAMETOOLONG));
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************
 * WCMD_skip_leading_spaces
 *
 *  Return a pointer to the first non-whitespace character of string.
 *  Does not modify the input string.
 */
WCHAR *WCMD_skip_leading_spaces(WCHAR *string)
{
    while (*string == L' ' || *string == L'\t') string++;
    return string;
}

static WCHAR *WCMD_strip_for_command_start(WCHAR *string)
{
    while (*string == L' ' || *string == L'\t' || *string == L'@') string++;
    return string;
}

/***************************************************************************
 * WCMD_keyword_ws_found
 *
 *  Checks if the string located at ptr matches a keyword (of length len)
 *  followed by a whitespace character (space or tab)
 */
BOOL WCMD_keyword_ws_found(const WCHAR *keyword, const WCHAR *ptr) {
    const int len = lstrlenW(keyword);
    return (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
                           ptr, len, keyword, len) == CSTR_EQUAL)
            && ((*(ptr + len) == ' ') || (*(ptr + len) == '\t'));
}

/*************************************************************************
 * WCMD_strip_quotes
 *
 *  Remove first and last quote WCHARacters, preserving all other text
 *  Returns the location of the final quote
 */
WCHAR *WCMD_strip_quotes(WCHAR *cmd) {
  WCHAR *src = cmd + 1, *dest = cmd, *lastq = NULL, *lastquote;
  while((*dest=*src) != '\0') {
      if (*src=='\"')
          lastq=dest;
      dest++; src++;
  }
  lastquote = lastq;
  if (lastq) {
      dest=lastq++;
      while ((*dest++=*lastq++) != 0)
          ;
  }
  return lastquote;
}

static inline int read_int_in_range(const WCHAR *from, WCHAR **after, int low, int high)
{
    int val = wcstol(from, after, 10);
    val += (val < 0) ? high : low;
    return val <= low ? low : (val >= high ? high : val);
}

/*************************************************************************
 * WCMD_expand_envvar
 *
 *	Expands environment variables, allowing for WCHARacter substitution
 */
static WCHAR *WCMD_expand_envvar(WCHAR *start)
{
    WCHAR *endOfVar = NULL, *s;
    WCHAR *colonpos = NULL;
    WCHAR thisVar[MAXSTRING];
    WCHAR thisVarContents[MAXSTRING];
    int len;

    WINE_TRACE("Expanding: %s\n", wine_dbgstr_w(start));

    endOfVar = wcschr(start + 1, *start);
    if (!endOfVar)
        /* no corresponding closing char... either skip startchar in batch, or leave untouched otherwise */
        return context ? WCMD_strsubstW(start, start + 1, NULL, 0) : start + 1;

    memcpy(thisVar, start + 1, (endOfVar - start - 1) * sizeof(WCHAR));
    thisVar[endOfVar - start - 1] = L'\0';
    colonpos = wcschr(thisVar, L':');

    /* If there's complex substitution, just need %var% for now
     * to get the expanded data to play with
     */
    if (colonpos) colonpos[0] = L'\0';

    TRACE("Retrieving contents of %s\n", wine_dbgstr_w(thisVar));

    /* env variables (when set) have priority over magic env variables */
    len = GetEnvironmentVariableW(thisVar, thisVarContents, ARRAY_SIZE(thisVarContents));
    if (!len)
    {
        if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT, thisVar, -1, L"ERRORLEVEL", -1) == CSTR_EQUAL)
            len = wsprintfW(thisVarContents, L"%d", errorlevel);
        else if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT, thisVar, -1, L"DATE", -1) == CSTR_EQUAL)
            len = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL,
                                 NULL, thisVarContents, ARRAY_SIZE(thisVarContents));
        else if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT, thisVar, -1, L"TIME", -1) == CSTR_EQUAL)
            len = GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL,
                                 NULL, thisVarContents, ARRAY_SIZE(thisVarContents));
        else if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT, thisVar, -1, L"CD", -1) == CSTR_EQUAL)
            len = GetCurrentDirectoryW(ARRAY_SIZE(thisVarContents), thisVarContents);
        else if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT, thisVar, -1, L"RANDOM", -1) == CSTR_EQUAL)
            len = wsprintfW(thisVarContents, L"%d", rand() % 32768);
    }
    /* Restore complex bit */
    if (colonpos) colonpos[0] = L':';

    /* In a batch program, unknown env vars are replaced with nothing,
     * note syntax %garbage:1,3% results in anything after the ':'
     * except the %
     * From the command line, you just get back what you entered
     */
    if (!len)
    {
        /* Command line - just ignore this */
        if (context == NULL) return endOfVar + 1;

        /* Batch - replace unknown env var with nothing */
        if (colonpos == NULL)
            return WCMD_strsubstW(start, endOfVar + 1, NULL, 0);
        if (colonpos == thisVar)
            return WCMD_strsubstW(start, endOfVar + 1, colonpos, -1);
        return WCMD_strsubstW(start, endOfVar + 1, colonpos + 1, -1);
    }

    /* See if we need to do complex substitution (any ':'s) */
    if (colonpos == NULL)
      return WCMD_strsubstW(start, endOfVar + 1, thisVarContents, -1);

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
    if (colonpos[1] == L'~')
    {
        int   substr_beg, substr_end;
        WCHAR *ptr;

        substr_beg = read_int_in_range(colonpos + 2, &ptr, 0, len);
        if (*ptr == L',')
            substr_end = read_int_in_range(ptr + 1, &ptr, substr_beg, len);
        else
            substr_end = len;
        if (*ptr == L'\0')
            return WCMD_strsubstW(start, endOfVar + 1, &thisVarContents[substr_beg], substr_end - substr_beg);
        /* error, remove enclosing % pair (in place) */
        memmove(start, start + 1, (endOfVar - start - 1) * sizeof(WCHAR));
        return WCMD_strsubstW(endOfVar - 1, endOfVar + 1, NULL, 0);
    /* search and replace manipulation */
    } else {
      WCHAR *equalspos = wcschr(colonpos, L'=');
      WCHAR *replacewith = equalspos+1;
      WCHAR *found       = NULL;
      WCHAR *searchIn;
      WCHAR *searchFor;
      WCHAR *ret;

      if (equalspos == NULL) return start+1;
      s = xstrdupW(endOfVar + 1);

      *equalspos = 0x00;

      /* Since we need to be case insensitive, copy the 2 buffers */
      searchIn  = xstrdupW(thisVarContents);
      CharUpperBuffW(searchIn, lstrlenW(thisVarContents));
      searchFor = xstrdupW(colonpos + 1);
      CharUpperBuffW(searchFor, lstrlenW(colonpos+1));

      /* Handle wildcard case */
      if (*(colonpos+1) == '*') {
        /* Search for string to replace */
        found = wcsstr(searchIn, searchFor+1);

        if (found) {
          /* Do replacement */
          lstrcpyW(start, replacewith);
          lstrcatW(start, thisVarContents + (found-searchIn) + lstrlenW(searchFor+1));
          ret = start + wcslen(start);
          lstrcatW(start, s);
        } else {
          /* Copy as is */
          lstrcpyW(start, thisVarContents);
          ret = start + wcslen(start);
          lstrcatW(start, s);
        }
      } else {
        /* Loop replacing all instances */
        WCHAR *lastFound = searchIn;
        WCHAR *outputposn = start;

        *start = 0x00;
        while ((found = wcsstr(lastFound, searchFor))) {
            lstrcpynW(outputposn,
                    thisVarContents + (lastFound-searchIn),
                    (found - lastFound)+1);
            outputposn  = outputposn + (found - lastFound);
            lstrcatW(outputposn, replacewith);
            outputposn = outputposn + lstrlenW(replacewith);
            lastFound = found + lstrlenW(searchFor);
        }
        lstrcatW(outputposn,
                thisVarContents + (lastFound-searchIn));
        ret = outputposn + wcslen(outputposn);
        lstrcatW(outputposn, s);
      }
      free(s);
      free(searchIn);
      free(searchFor);
      return ret;
    }
}

/*****************************************************************************
 * Expand the command. Native expands lines from batch programs as they are
 * read in and not again, except for 'for' variable substitution.
 * eg. As evidence, "echo %1 && shift && echo %1" or "echo %%path%%"
 * atExecute is TRUE when the expansion is occurring as the command is executed
 * rather than at parse time, i.e. delayed expansion and for loops need to be
 * processed
 */
static void handleExpansion(WCHAR *cmd, BOOL atExecute) {

  /* For commands in a context (batch program):                  */
  /*   Expand environment variables in a batch file %{0-9} first */
  /*     including support for any ~ modifiers                   */
  /* Additionally:                                               */
  /*   Expand the DATE, TIME, CD, RANDOM and ERRORLEVEL special  */
  /*     names allowing environment variable overrides           */
  /* NOTE: To support the %PATH:xxx% syntax, also perform        */
  /*   manual expansion of environment variables here            */

  WCHAR *p = cmd;
  WCHAR *t;
  int   i;
  BOOL delayed = atExecute ? delayedsubst : FALSE;
  WCHAR *delayedp = NULL;
  WCHAR  startchar = '%';
  WCHAR *normalp;

  /* Display the FOR variables in effect */
  for (i=0;i<ARRAY_SIZE(forloopcontext->variable);i++) {
    if (forloopcontext->variable[i]) {
      TRACE("FOR variable context: %s = '%s'\n",
            debugstr_for_var((WCHAR)i), wine_dbgstr_w(forloopcontext->variable[i]));
    }
  }

  for (;;)
  {
    /* Find the next environment variable delimiter */
    normalp = wcschr(p, '%');
    if (delayed) delayedp = wcschr(p, '!');
    if (!normalp) p = delayedp;
    else if (!delayedp) p = normalp;
    else p = min(p,delayedp);
    if (!p) break;
    startchar = *p;

    WINE_TRACE("Translate command:%s %d (at: %s)\n",
                   wine_dbgstr_w(cmd), atExecute, wine_dbgstr_w(p));
    i = *(p+1) - '0';

    /* handle consecutive % or ! */
    if ((!atExecute || startchar == L'!') && p[1] == startchar) {
        if (context) WCMD_strsubstW(p, p + 1, NULL, 0);
        if (!context || startchar == L'%') p++;
    /* Replace %~ modifications if in batch program */
    } else if (p[1] == L'~' && p[2] && !iswspace(p[2])) {
      WCMD_HandleTildeModifiers(&p, atExecute);
      p++;

    /* Replace use of %0...%9 if in batch program*/
    } else if (!atExecute && context && (i >= 0) && (i <= 9) && startchar == '%') {
      t = WCMD_parameter(context -> command, i + context -> shift_count[i],
                         NULL, TRUE, TRUE);
      p = WCMD_strsubstW(p, p+2, t, -1);

    /* Replace use of %* if in batch program*/
    } else if (!atExecute && context && *(p+1)=='*' && startchar == '%') {
      WCHAR *startOfParms = NULL;
      WCHAR *thisParm = WCMD_parameter(context -> command, 0, &startOfParms, TRUE, TRUE);
      if (startOfParms != NULL) {
        startOfParms += lstrlenW(thisParm);
        while (*startOfParms==' ' || *startOfParms == '\t') startOfParms++;
        p = WCMD_strsubstW(p, p+2, startOfParms, -1);
      } else
        p = WCMD_strsubstW(p, p+2, NULL, 0);

    } else {
      if (startchar == L'%' && for_var_is_valid(p[1]) && forloopcontext->variable[p[1]]) {
        /* Replace the 2 characters, % and for variable character */
        p = WCMD_strsubstW(p, p + 2, forloopcontext->variable[p[1]], -1);
      } else if (!atExecute || startchar == L'!') {
        BOOL first = p == cmd;
        p = WCMD_expand_envvar(p);
        /* FIXME: maybe this more likely calls for a specific handling of first arg? */
        if (context && startchar == L'!' && first)
        {
            WCHAR *last;
            for (last = p; *last == startchar; last++) {}
            p = WCMD_strsubstW(p, last, NULL, 0);
        }
      /* In a FOR loop, see if this is the variable to replace */
      } else { /* Ignore %'s on second pass of batch program */
        p++;
      }
    }
  }
}


/*******************************************************************
 * WCMD_parse - parse a command into parameters and qualifiers.
 *
 *	On exit, all qualifiers are concatenated into q, the first string
 *	not beginning with "/" is in p1 and the
 *	second in p2. Any subsequent non-qualifier strings are lost.
 *	Parameters in quotes are handled.
 */
static void WCMD_parse (const WCHAR *s, WCHAR *q, WCHAR *p1, WCHAR *p2)
{
  int p = 0;

  *q = *p1 = *p2 = '\0';
  while (TRUE) {
    switch (*s) {
      case '/':
        *q++ = *s++;
	while ((*s != '\0') && (*s != ' ') && *s != '/') {
	  *q++ = towupper (*s++);
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
	while ((*s != '\0') && (*s != ' ') && (*s != '\t')
               && (*s != '=')  && (*s != ',') ) {
	  if (p == 0) *p1++ = *s++;
	  else if (p == 1) *p2++ = *s++;
	  else s++;
	}
        /* Skip concurrent parms */
	while ((*s == ' ') || (*s == '\t') || (*s == '=')  || (*s == ',') ) s++;

        if (p == 0) *p1 = '\0';
        if (p == 1) *p2 = '\0';
	p++;
    }
  }
}

void WCMD_expand(const WCHAR *src, WCHAR *dst)
{
    wcscpy(dst, src);
    handleExpansion(dst, FALSE);
    WCMD_parse(dst, quals, param1, param2);
}

/* ============================== */
/*  Data structures for commands  */
/* ============================== */

static void redirection_dispose_list(CMD_REDIRECTION *redir)
{
    while (redir)
    {
        CMD_REDIRECTION *next = redir->next;
        free(redir);
        redir = next;
    }
}

static CMD_REDIRECTION *redirection_create_file(enum CMD_REDIRECTION_KIND kind, unsigned fd, const WCHAR *file)
{
    size_t len = wcslen(file) + 1;
    CMD_REDIRECTION *redir = xalloc(offsetof(CMD_REDIRECTION, file[len]));

    redir->kind = kind;
    redir->fd = fd;
    memcpy(redir->file, file, len * sizeof(WCHAR));
    redir->next = NULL;

    return redir;
}

static CMD_REDIRECTION *redirection_create_clone(unsigned fd, unsigned fd_clone)
{
    CMD_REDIRECTION *redir = xalloc(sizeof(*redir));

    redir->kind = REDIR_WRITE_CLONE;
    redir->fd = fd;
    redir->clone = fd_clone;
    redir->next = NULL;

    return redir;
}

static const char *debugstr_redirection(const CMD_REDIRECTION *redir)
{
    switch (redir->kind)
    {
    case REDIR_READ_FROM:
        return wine_dbg_sprintf("%u< (%ls)", redir->fd, redir->file);
    case REDIR_WRITE_TO:
        return wine_dbg_sprintf("%u> (%ls)", redir->fd, redir->file);
    case REDIR_WRITE_APPEND:
       return wine_dbg_sprintf("%u>> (%ls)", redir->fd, redir->file);
    case REDIR_WRITE_CLONE:
        return wine_dbg_sprintf("%u>&%u", redir->fd, redir->clone);
    default:
        return "-^-";
    }
}

static WCHAR *command_create(const WCHAR *ptr, size_t len)
{
    WCHAR *command = xalloc((len + 1) * sizeof(WCHAR));
    memcpy(command, ptr, len * sizeof(WCHAR));
    command[len] = L'\0';
    return command;
}

static void for_control_dispose(CMD_FOR_CONTROL *for_ctrl)
{
    free((void*)for_ctrl->set);
    switch (for_ctrl->operator)
    {
    case CMD_FOR_FILE_SET:
        free((void*)for_ctrl->delims);
        free((void*)for_ctrl->tokens);
        break;
    case CMD_FOR_FILETREE:
        free((void*)for_ctrl->root_dir);
        break;
    default:
        break;
    }
}

const char *debugstr_for_control(const CMD_FOR_CONTROL *for_ctrl)
{
    static const char* for_ctrl_strings[] = {"tree", "file", "numbers"};
    const char *flags, *options;

    if (for_ctrl->operator >= ARRAY_SIZE(for_ctrl_strings))
    {
        FIXME("Unexpected operator\n");
        return wine_dbg_sprintf("<<%u>>", for_ctrl->operator);
    }

    if (for_ctrl->flags)
        flags = wine_dbg_sprintf("flags=%s%s%s ",
                                 (for_ctrl->flags & CMD_FOR_FLAG_TREE_RECURSE) ? "~recurse" : "",
                                 (for_ctrl->flags & CMD_FOR_FLAG_TREE_INCLUDE_FILES) ? "~+files" : "",
                                 (for_ctrl->flags & CMD_FOR_FLAG_TREE_INCLUDE_DIRECTORIES) ? "~+dirs" : "");
    else
        flags = "";
    switch (for_ctrl->operator)
    {
    case CMD_FOR_FILETREE:
        options = wine_dbg_sprintf("root=(%ls) ", for_ctrl->root_dir);
        break;
    case CMD_FOR_FILE_SET:
        {
            WCHAR eol_buf[4] = {L'\'', for_ctrl->eol, L'\'', L'\0'};
            const WCHAR *eol = for_ctrl->eol ? eol_buf : L"<nul>";
            options = wine_dbg_sprintf("eol=%ls skip=%d use_backq=%c delims=%s tokens=%s ",
                                       eol, for_ctrl->num_lines_to_skip, for_ctrl->use_backq ? 'Y' : 'N',
                                       wine_dbgstr_w(for_ctrl->delims), wine_dbgstr_w(for_ctrl->tokens));
        }
        break;
    default:
        options = "";
        break;
    }
    return wine_dbg_sprintf("[FOR] %s %s%s%s (%ls)",
                            for_ctrl_strings[for_ctrl->operator], flags, options,
                            debugstr_for_var(for_ctrl->variable_index), for_ctrl->set);
}

static void for_control_create(enum for_control_operator for_op, unsigned flags, const WCHAR *options, unsigned varidx, CMD_FOR_CONTROL *for_ctrl)
{
    for_ctrl->operator = for_op;
    for_ctrl->flags = flags;
    for_ctrl->variable_index = varidx;
    for_ctrl->set = NULL;
    switch (for_ctrl->operator)
    {
    case CMD_FOR_FILETREE:
        for_ctrl->root_dir = options && *options ? xstrdupW(options) : NULL;
        break;
    default:
        break;
    }
}

static void for_control_create_fileset(unsigned flags, unsigned varidx, WCHAR eol, int num_lines_to_skip, BOOL use_backq,
                                       const WCHAR *delims, const WCHAR *tokens,
                                       CMD_FOR_CONTROL *for_ctrl)
{
    for_ctrl->operator = CMD_FOR_FILE_SET;
    for_ctrl->flags = flags;
    for_ctrl->variable_index = varidx;
    for_ctrl->set = NULL;

    for_ctrl->eol = eol;
    for_ctrl->use_backq = use_backq;
    for_ctrl->num_lines_to_skip = num_lines_to_skip;
    for_ctrl->delims = delims;
    for_ctrl->tokens = tokens;
}

static void for_control_append_set(CMD_FOR_CONTROL *for_ctrl, const WCHAR *set)
{
    if (for_ctrl->set)
    {
        for_ctrl->set = xrealloc((void*)for_ctrl->set,
                                 (wcslen(for_ctrl->set) + 1 + wcslen(set) + 1) * sizeof(WCHAR));
        wcscat((WCHAR*)for_ctrl->set, L" ");
        wcscat((WCHAR*)for_ctrl->set, set);
    }
    else
        for_ctrl->set = xstrdupW(set);
}

void if_condition_dispose(CMD_IF_CONDITION *cond)
{
    switch (cond->op)
    {
    case CMD_IF_ERRORLEVEL:
    case CMD_IF_EXIST:
    case CMD_IF_DEFINED:
        free((void*)cond->operand);
        break;
    case CMD_IF_BINOP_EQUAL:
    case CMD_IF_BINOP_LSS:
    case CMD_IF_BINOP_LEQ:
    case CMD_IF_BINOP_EQU:
    case CMD_IF_BINOP_NEQ:
    case CMD_IF_BINOP_GEQ:
    case CMD_IF_BINOP_GTR:
        free((void*)cond->left);
        free((void*)cond->right);
        break;
    }
}

static BOOL if_condition_parse(WCHAR *start, WCHAR **end, CMD_IF_CONDITION *cond)
{
    WCHAR *param_start;
    const WCHAR *param_copy;
    int narg = 0;

    if (cond) memset(cond, 0, sizeof(*cond));
    param_copy = WCMD_parameter(start, narg++, &param_start, TRUE, FALSE);
    /* /I is the only option supported */
    if (!wcsicmp(param_copy, L"/I"))
    {
        param_copy = WCMD_parameter(start, narg++, &param_start, TRUE, FALSE);
        if (cond) cond->case_insensitive = 1;
    }
    if (!wcsicmp(param_copy, L"NOT"))
    {
        param_copy = WCMD_parameter(start, narg++, &param_start, TRUE, FALSE);
        if (cond) cond->negated = 1;
    }
    if (!wcsicmp(param_copy, L"errorlevel"))
    {
        param_copy = WCMD_parameter(start, narg++, &param_start, TRUE, FALSE);
        if (cond) cond->op = CMD_IF_ERRORLEVEL;
        if (cond) cond->operand = wcsdup(param_copy);
    }
    else if (!wcsicmp(param_copy, L"exist"))
    {
        param_copy = WCMD_parameter(start, narg++, &param_start, FALSE, FALSE);
        if (cond) cond->op = CMD_IF_EXIST;
        if (cond) cond->operand = wcsdup(param_copy);
    }
    else if (!wcsicmp(param_copy, L"defined"))
    {
        param_copy = WCMD_parameter(start, narg++, &param_start, TRUE, FALSE);
        if (cond) cond->op = CMD_IF_DEFINED;
        if (cond) cond->operand = wcsdup(param_copy);
    }
    else /* comparison operation */
    {
        if (*param_copy == L'\0') return FALSE;
        param_copy = WCMD_parameter(start, narg - 1, &param_start, TRUE, FALSE);
        if (cond) cond->left = wcsdup(param_copy);

        start = WCMD_skip_leading_spaces(param_start + wcslen(param_copy));

        /* Note: '==' can't be returned by WCMD_parameter since '=' is a separator */
        if (start[0] == L'=' && start[1] == L'=')
        {
            start += 2; /* == */
            if (cond) cond->op = CMD_IF_BINOP_EQUAL;
        }
        else
        {
            static struct
            {
                const WCHAR *name;
                enum cond_operator binop;
            }
            allowed_operators[] = {{L"lss", CMD_IF_BINOP_LSS},
                                   {L"leq", CMD_IF_BINOP_LEQ},
                                   {L"equ", CMD_IF_BINOP_EQU},
                                   {L"neq", CMD_IF_BINOP_NEQ},
                                   {L"geq", CMD_IF_BINOP_GEQ},
                                   {L"gtr", CMD_IF_BINOP_GTR},
            };
            int i;

            param_copy = WCMD_parameter(start, 0, &param_start, FALSE, FALSE);
            for (i = 0; i < ARRAY_SIZE(allowed_operators); i++)
                if (!wcsicmp(param_copy, allowed_operators[i].name)) break;
            if (i == ARRAY_SIZE(allowed_operators))
            {
                if (cond) free((void*)cond->left);
                return FALSE;
            }
            if (cond) cond->op = allowed_operators[i].binop;
            start += wcslen(param_copy);
        }

        param_copy = WCMD_parameter(start, 0, &param_start, TRUE, FALSE);
        if (*param_copy == L'\0')
        {
            if (cond) free((void*)cond->left);
            return FALSE;
        }
        if (cond) cond->right = wcsdup(param_copy);

        start = param_start + wcslen(param_copy);
        narg = 0;
    }
    /* check all remaning args are present, and compute pointer to end of condition */
    param_copy = WCMD_parameter(start, narg, end, TRUE, FALSE);
    return cond || *param_copy != L'\0';
}

static const char *debugstr_if_condition(const CMD_IF_CONDITION *cond)
{
    const char *header = wine_dbg_sprintf("{{%s%s", cond->negated ? "not " : "", cond->case_insensitive ? "nocase " : "");

    switch (cond->op)
    {
    case CMD_IF_ERRORLEVEL:   return wine_dbg_sprintf("%serrorlevel %ls}}", header, cond->operand);
    case CMD_IF_EXIST:        return wine_dbg_sprintf("%sexist %ls}}", header, cond->operand);
    case CMD_IF_DEFINED:      return wine_dbg_sprintf("%sdefined %ls}}", header, cond->operand);
    case CMD_IF_BINOP_EQUAL:  return wine_dbg_sprintf("%s%ls == %ls}}", header, cond->left, cond->right);

    case CMD_IF_BINOP_LSS:    return wine_dbg_sprintf("%s%ls LSS %ls}}", header, cond->left, cond->right);
    case CMD_IF_BINOP_LEQ:    return wine_dbg_sprintf("%s%ls LEQ %ls}}", header, cond->left, cond->right);
    case CMD_IF_BINOP_EQU:    return wine_dbg_sprintf("%s%ls EQU %ls}}", header, cond->left, cond->right);
    case CMD_IF_BINOP_NEQ:    return wine_dbg_sprintf("%s%ls NEQ %ls}}", header, cond->left, cond->right);
    case CMD_IF_BINOP_GEQ:    return wine_dbg_sprintf("%s%ls GEQ %ls}}", header, cond->left, cond->right);
    case CMD_IF_BINOP_GTR:    return wine_dbg_sprintf("%s%ls GTR %ls}}", header, cond->left, cond->right);
    default:
        FIXME("Unexpected condition operator %u\n", cond->op);
        return "{{}}";
    }
}

/***************************************************************************
 * node_dispose_tree
 *
 * Frees the storage held for a parsed command line
 * - This is not done in the process_commands, as eventually the current
 *   pointer will be modified within the commands, and hence a single free
 *   routine is simpler
 */
void node_dispose_tree(CMD_NODE *node)
{
    /* Loop through the commands, freeing them one by one */
    if (!node) return;
    switch (node->op)
    {
    case CMD_SINGLE:
        free(node->command);
        break;
    case CMD_CONCAT:
    case CMD_PIPE:
    case CMD_ONFAILURE:
    case CMD_ONSUCCESS:
        node_dispose_tree(node->left);
        node_dispose_tree(node->right);
        break;
    case CMD_IF:
        if_condition_dispose(&node->condition);
        node_dispose_tree(node->then_block);
        node_dispose_tree(node->else_block);
        break;
    case CMD_FOR:
        for_control_dispose(&node->for_ctrl);
        node_dispose_tree(node->do_block);
        break;
    }
    redirection_dispose_list(node->redirects);
    free(node);
}

static CMD_NODE *node_create_single(WCHAR *c)
{
    CMD_NODE *new = xalloc(sizeof(CMD_NODE));

    new->op = CMD_SINGLE;
    new->command = c;
    new->redirects = NULL;

    return new;
}

static CMD_NODE *node_create_binary(CMD_OPERATOR op, CMD_NODE *l, CMD_NODE *r)
{
    CMD_NODE *new = xalloc(sizeof(CMD_NODE));

    new->op = op;
    new->left = l;
    new->right = r;
    new->redirects = NULL;

    return new;
}

static CMD_NODE *node_create_if(CMD_IF_CONDITION *cond, CMD_NODE *then_block, CMD_NODE *else_block)
{
    CMD_NODE *new = xalloc(sizeof(CMD_NODE));

    new->op = CMD_IF;
    new->condition = *cond;
    new->then_block = then_block;
    new->else_block = else_block;
    new->redirects = NULL;

    return new;
}

static CMD_NODE *node_create_for(CMD_FOR_CONTROL *for_ctrl, CMD_NODE *do_block)
{
    CMD_NODE *new = xalloc(sizeof(CMD_NODE));

    new->op = CMD_FOR;
    new->for_ctrl = *for_ctrl;
    new->do_block = do_block;
    new->redirects = NULL;

    return new;
}

static void init_msvcrt_io_block(STARTUPINFOW* st)
{
    STARTUPINFOW st_p;
    /* fetch the parent MSVCRT info block if any, so that the child can use the
     * same handles as its grand-father
     */
    st_p.cb = sizeof(STARTUPINFOW);
    GetStartupInfoW(&st_p);
    st->cbReserved2 = st_p.cbReserved2;
    st->lpReserved2 = st_p.lpReserved2;
    if (st_p.cbReserved2 && st_p.lpReserved2)
    {
        unsigned num = *(unsigned*)st_p.lpReserved2;
        char* flags;
        HANDLE* handles;
        BYTE *ptr;
        size_t sz;

        /* Override the entries for fd 0,1,2 if we happened
         * to change those std handles (this depends on the way cmd sets
         * its new input & output handles)
         */
        sz = max(sizeof(unsigned) + (sizeof(char) + sizeof(HANDLE)) * 3, st_p.cbReserved2);
        ptr = xalloc(sz);
        flags = (char*)(ptr + sizeof(unsigned));
        handles = (HANDLE*)(flags + num * sizeof(char));

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

/* Attempt to open a file at a known path. */
static RETURN_CODE run_full_path(const WCHAR *file, WCHAR *full_cmdline, BOOL called)
{
    const WCHAR *ext = wcsrchr(file, '.');
    STARTUPINFOW si = {.cb = sizeof(si)};
    DWORD console, exit_code;
    WCHAR exe_path[MAX_PATH];
    PROCESS_INFORMATION pi;
    SHFILEINFOW psfi;
    HANDLE handle;
    BOOL ret;

    TRACE("%s\n", debugstr_w(file));

    if (ext && (!wcsicmp(ext, L".bat") || !wcsicmp(ext, L".cmd")))
    {
        RETURN_CODE return_code;
        BOOL oldinteractive = interactive;

        interactive = FALSE;
        return_code = WCMD_call_batch(file, full_cmdline);
        interactive = oldinteractive;
        if (context && !called)
        {
            TRACE("Batch completed, but was not 'called' so skipping outer batch too\n");
            context->skip_rest = TRUE;
        }
        return return_code;
    }

    if ((INT_PTR)FindExecutableW(file, NULL, exe_path) < 32)
        console = 0;
    else
        console = SHGetFileInfoW(exe_path, 0, &psfi, sizeof(psfi), SHGFI_EXETYPE);

    init_msvcrt_io_block(&si);
    ret = CreateProcessW(file, full_cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    free(si.lpReserved2);

    if (ret)
    {
        CloseHandle(pi.hThread);
        handle = pi.hProcess;
    }
    else
    {
        SHELLEXECUTEINFOW sei = {.cbSize = sizeof(sei)};
        WCHAR *args;

        WCMD_parameter(full_cmdline, 1, &args, FALSE, TRUE);
        /* FIXME: when the file extension is not registered,
         * native cmd does popup a dialog box to register an app for this extension.
         * Also, ShellExecuteW returns before the dialog box is closed.
         * Moreover, on Wine, displaying a dialog box without a message loop
         * (in cmd.exe) blocks the dialog.
         * So, until the above bits are solved, don't display any dialog box.
         */
        sei.fMask = SEE_MASK_NO_CONSOLE | SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
        sei.lpFile = file;
        sei.lpParameters = args;
        sei.nShow = SW_SHOWNORMAL;

        if (ShellExecuteExW(&sei) && (INT_PTR)sei.hInstApp >= 32)
        {
            handle = sei.hProcess;
        }
        else
        {
            errorlevel = GetLastError();
            return errorlevel;
        }
    }

    if (!interactive || (console && !HIWORD(console)))
        WaitForSingleObject(handle, INFINITE);
    GetExitCodeProcess(handle, &exit_code);
    errorlevel = (exit_code == STILL_ACTIVE) ? NO_ERROR : exit_code;

    CloseHandle(handle);
    return errorlevel;
}

struct search_command
{
    WCHAR path[MAX_PATH];
    BOOL has_path; /* if input has path part (ie cannot be a builtin command) */
    BOOL has_extension; /* if extension was given to input */
    int cmd_index; /* potential index to builtin command */
};

static BOOL search_in_pathext(WCHAR *path)
{
    static struct
    {
        unsigned short offset; /* into cached_data */
        unsigned short length;
    } *cached_pathext;
    static WCHAR *cached_data;
    static unsigned cached_count;
    WCHAR pathext[MAX_PATH];
    WIN32_FIND_DATAW find;
    WCHAR *pos;
    HANDLE h;
    unsigned efound;
    DWORD len;

    len = GetEnvironmentVariableW(L"PATHEXT", pathext, ARRAY_SIZE(pathext));
    if (len == 0 || len >= ARRAY_SIZE(pathext))
        wcscpy(pathext, L".bat;.com;.cmd;.exe");
    /* erase cache if PATHEXT has changed */
    if (cached_data && wcscmp(cached_data, pathext))
    {
        free(cached_pathext);
        cached_pathext = NULL;
        free(cached_data);
        cached_data = NULL;
        cached_count = 0;
    }
    /* (re)create cache if needed */
    if (!cached_pathext)
    {
        size_t c;
        WCHAR *p, *n;

        cached_data = xstrdupW(pathext);
        for (p = cached_data, c = 1; (p = wcschr(p, L';')) != NULL; c++, p++) {}
        cached_pathext = xalloc(sizeof(cached_pathext[0]) * c);
        cached_count = c;
        for (c = 0, p = cached_data; (n = wcschr(p, L';')) != NULL; c++, p = n + 1)
        {
            cached_pathext[c].offset = p - cached_data;
            cached_pathext[c].length = n - p;
        }
        cached_pathext[c].offset = p - cached_data;
        cached_pathext[c].length = wcslen(p);
    }

    pos = &path[wcslen(path)]; /* Pos = end of name */
    wcscpy(pos, L".*");
    efound = cached_count;
    if ((h = FindFirstFileW(path, &find)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            WCHAR *last;
            size_t basefound_len;
            unsigned i;

            if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!(last = wcsrchr(find.cFileName, L'.'))) continue;
            basefound_len = last - find.cFileName;
            /* skip foo.bar.exe when looking for foo.exe */
            if (pos < path + basefound_len || wcsnicmp(pos - basefound_len, find.cFileName, basefound_len))
                continue;
            for (i = 0; i < efound; i++)
                if (!wcsnicmp(last, cached_data + cached_pathext[i].offset, cached_pathext[i].length) &&
                    !last[cached_pathext[i].length])
                    efound = i;
        } while (FindNextFileW(h, &find));
        CloseHandle(h);
    }
    if (efound == cached_count) return FALSE;

    memcpy(pos, cached_data + cached_pathext[efound].offset, cached_pathext[efound].length * sizeof(WCHAR));
    pos[cached_pathext[efound].length] = L'\0';
    return TRUE;
}

static RETURN_CODE search_command(WCHAR *command, struct search_command *sc, BOOL fast)
{
    WCHAR  temp[MAX_PATH];
    WCHAR  pathtosearch[MAXSTRING];
    WCHAR *pathposn;
    WCHAR  stemofsearch[MAX_PATH];    /* maximum allowed executable name is
                                         MAX_PATH, including null character */
    WCHAR *lastSlash;
    WCHAR *firstParam;
    DWORD  len;
    WCHAR *p;

    /* Quick way to get the filename is to extract the first argument. */
    firstParam = WCMD_parameter(command, 0, NULL, FALSE, TRUE);

    sc->cmd_index = WCMD_EXIT + 1;

    if (!firstParam[0])
    {
        sc->path[0] = L'\0';
        return NO_ERROR;
    }
    for (p = firstParam; *p && IsCharAlphaW(*p); p++) {}
    if (p > firstParam && (!*p || wcschr(L" \t+./(;=:", *p)))
    {
        for (sc->cmd_index = 0; sc->cmd_index <= WCMD_EXIT; sc->cmd_index++)
            if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
                               firstParam, p - firstParam, inbuilt[sc->cmd_index], -1) == CSTR_EQUAL)
                break;
    }
    if (firstParam[1] == L':' && (!firstParam[2] || iswspace(firstParam[2])))
    {
        sc->cmd_index = WCMD_CHGDRIVE;
        fast = TRUE;
    }

    if (fast && sc->cmd_index <= WCMD_EXIT && firstParam[wcslen(inbuilt[sc->cmd_index])] != L'.')
    {
        sc->path[0] = L'\0';
        sc->has_path = sc->has_extension = FALSE;
        return RETURN_CODE_CANT_LAUNCH;
    }

    /* Calculate the search path and stem to search for */
    if (wcspbrk(firstParam, L"/\\:") == NULL)
    {
        /* No explicit path given, search path */
        wcscpy(pathtosearch, L".;");
        len = GetEnvironmentVariableW(L"PATH", &pathtosearch[2], ARRAY_SIZE(pathtosearch)-2);
        if (len == 0 || len >= ARRAY_SIZE(pathtosearch) - 2)
            wcscpy(pathtosearch, L".");
        sc->has_extension = wcschr(firstParam, L'.') != NULL;
        if (wcslen(firstParam) >= MAX_PATH)
        {
            WCMD_output_asis_stderr(WCMD_LoadMessage(WCMD_LINETOOLONG));
            return ERROR_INVALID_FUNCTION;
        }

        wcscpy(stemofsearch, firstParam);
        sc->has_path = FALSE;
    }
    else
    {
        /* Convert eg. ..\fred to include a directory by removing file part */
        if (!WCMD_get_fullpath(firstParam, ARRAY_SIZE(pathtosearch), pathtosearch, NULL))
            return ERROR_INVALID_FUNCTION;
        lastSlash = wcsrchr(pathtosearch, L'\\');
        sc->has_extension = wcschr(lastSlash ? lastSlash + 1 : firstParam, L'.') != NULL;
        wcscpy(stemofsearch, lastSlash ? lastSlash + 1 : firstParam);

        /* Reduce pathtosearch to a path with trailing '\' to support c:\a.bat and
           c:\windows\a.bat syntax                                                 */
        if (lastSlash) *(lastSlash + 1) = L'\0';
        sc->has_path = TRUE;
    }

    /* Loop through the search path, dir by dir */
    pathposn = pathtosearch;
    WINE_TRACE("Searching in '%s' for '%s'\n", wine_dbgstr_w(pathtosearch),
               wine_dbgstr_w(stemofsearch));
    while (pathposn)
    {
        int    length            = 0;
        WCHAR *pos               = NULL;
        BOOL   found             = FALSE;
        BOOL   inside_quotes     = FALSE;

        sc->path[0] = L'\0';

        if (sc->has_path)
        {
            wcscpy(sc->path, pathposn);
            pathposn = NULL;
        }
        else
        {
            /* Work on the next directory on the search path */
            pos = pathposn;
            while ((inside_quotes || *pos != ';') && *pos != 0)
            {
                if (*pos == '"')
                    inside_quotes = !inside_quotes;
                pos++;
            }

            if (*pos)  /* Reached semicolon */
            {
                memcpy(sc->path, pathposn, (pos-pathposn) * sizeof(WCHAR));
                sc->path[(pos-pathposn)] = 0x00;
                pathposn = pos+1;
            }
            else       /* Reached string end */
            {
                wcscpy(sc->path, pathposn);
                pathposn = NULL;
            }

            /* Remove quotes */
            length = wcslen(sc->path);
            if (sc->path[length - 1] == L'"')
                sc->path[length - 1] = 0;

            if (*sc->path != L'"')
                wcscpy(temp, sc->path);
            else
                wcscpy(temp, sc->path + 1);

            /* When temp is an empty string, skip over it. This needs
               to be done before the expansion, because WCMD_get_fullpath
               fails when given an empty string                         */
            if (*temp == L'\0')
                continue;

            /* Since you can have eg. ..\.. on the path, need to expand
               to full information                                      */
            if (!WCMD_get_fullpath(temp, ARRAY_SIZE(sc->path), sc->path, NULL))
                return ERROR_INVALID_FUNCTION;
        }

        if (wcslen(sc->path) + 1 + wcslen(stemofsearch) >= ARRAY_SIZE(sc->path))
            return ERROR_INVALID_FUNCTION;

        /* 1. If extension supplied, see if that file exists */
        wcscat(sc->path, L"\\");
        wcscat(sc->path, stemofsearch);

        if (sc->has_extension)
        {
            DWORD attribs = GetFileAttributesW(sc->path);
            found = attribs != INVALID_FILE_ATTRIBUTES && !(attribs & FILE_ATTRIBUTE_DIRECTORY);
        }
        /* if foo.bat was given but not found, try to match foo.bat.bat (or any valid ext) */
        if (!found) found = search_in_pathext(sc->path);
        if (found) return NO_ERROR;
    }
    return RETURN_CODE_CANT_LAUNCH;
}

static BOOL set_std_redirections(CMD_REDIRECTION *redir)
{
    static DWORD std_index[3] = {STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};
    static SECURITY_ATTRIBUTES sa = {.nLength = sizeof(sa), .lpSecurityDescriptor = NULL, .bInheritHandle = TRUE};
    WCHAR expanded_filename[MAXSTRING];
    HANDLE h;

    for (; redir; redir = redir->next)
    {
        CMD_REDIRECTION *next;

        /* if we have several elements changing same std stream, only use last one */
        for (next = redir->next; next; next = next->next)
            if (redir->fd == next->fd) break;
        if (next) continue;
        switch (redir->kind)
        {
        case REDIR_READ_FROM:
            wcscpy(expanded_filename, redir->file);
            handleExpansion(expanded_filename, TRUE);
            h = CreateFileW(expanded_filename, GENERIC_READ, FILE_SHARE_READ,
                            &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE)
            {
                WARN("Failed to open (%ls)\n", expanded_filename);
                return FALSE;
            }
            TRACE("Open (%ls) => %p\n", expanded_filename, h);
            break;
        case REDIR_WRITE_TO:
        case REDIR_WRITE_APPEND:
            {
                DWORD disposition = redir->kind == REDIR_WRITE_TO ? CREATE_ALWAYS : OPEN_ALWAYS;
                wcscpy(expanded_filename, redir->file);
                handleExpansion(expanded_filename, TRUE);
                h = CreateFileW(expanded_filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE,
                                &sa, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
                if (h == INVALID_HANDLE_VALUE)
                {
                    WARN("Failed to open (%ls)\n", expanded_filename);
                    return FALSE;
                }
                TRACE("Open %u (%ls) => %p\n", redir->fd, expanded_filename, h);
                if (SetFilePointer(h, 0, NULL, FILE_END) == INVALID_SET_FILE_POINTER)
                    WCMD_print_error();
            }
            break;
        case REDIR_WRITE_CLONE:
            if (redir->clone > 2 || redir->clone == redir->fd)
            {
                WARN("Can't duplicate %d from %d\n", redir->fd, redir->clone);
                return FALSE;
            }
            if (!DuplicateHandle(GetCurrentProcess(),
                                 GetStdHandle(std_index[redir->clone]),
                                 GetCurrentProcess(),
                                 &h,
                                 0, TRUE, DUPLICATE_SAME_ACCESS))
            {
                WARN("Duplicating handle failed with gle %ld\n", GetLastError());
                return FALSE;
            }
            break;
        }
        if (redir->fd > 2)
            CloseHandle(h);
        else
            SetStdHandle(std_index[redir->fd], h);
    }
    return TRUE;
}

RETURN_CODE WCMD_run_builtin_command(int cmd_index, WCHAR *cmd)
{
    size_t count = wcslen(inbuilt[cmd_index]);
    WCHAR *parms_start = WCMD_skip_leading_spaces(&cmd[count]);
    RETURN_CODE return_code;

    WCMD_parse(parms_start, quals, param1, param2);
    TRACE("param1: %s, param2: %s\n", wine_dbgstr_w(param1), wine_dbgstr_w(param2));

    if (cmd_index <= WCMD_EXIT && (parms_start[0] == '/') && (parms_start[1] == '?'))
    {
        /* this is a help request for a builtin program */
        cmd_index = WCMD_HELP;
        wcscpy(parms_start, inbuilt[cmd_index]);
    }

    switch (cmd_index)
    {
    case WCMD_CALL:
        return_code = WCMD_call(parms_start);
        break;
    case WCMD_CD:
    case WCMD_CHDIR:
        return_code = WCMD_setshow_default(parms_start);
        break;
    case WCMD_CLS:
        return_code = WCMD_clear_screen();
        break;
    case WCMD_COPY:
        return_code = WCMD_copy(parms_start);
        break;
    case WCMD_DATE:
        return_code = WCMD_setshow_date();
	break;
    case WCMD_DEL:
    case WCMD_ERASE:
        return_code = WCMD_delete(parms_start);
        break;
    case WCMD_DIR:
        return_code = WCMD_directory(parms_start);
        break;
    case WCMD_ECHO:
        return_code = WCMD_echo(&cmd[count]);
        break;
    case WCMD_GOTO:
        return_code = WCMD_goto();
        break;
    case WCMD_HELP:
        return_code = WCMD_give_help(parms_start);
        break;
    case WCMD_LABEL:
        return_code = WCMD_label();
        break;
    case WCMD_MD:
    case WCMD_MKDIR:
        return_code = WCMD_create_dir(parms_start);
        break;
    case WCMD_MOVE:
        return_code = WCMD_move();
        break;
    case WCMD_PATH:
        return_code = WCMD_setshow_path(parms_start);
        break;
    case WCMD_PAUSE:
        return_code = WCMD_pause();
        break;
    case WCMD_PROMPT:
        return_code = WCMD_setshow_prompt();
        break;
    case WCMD_REM:
        return_code = NO_ERROR;
        break;
    case WCMD_REN:
    case WCMD_RENAME:
        return_code = WCMD_rename();
	break;
    case WCMD_RD:
    case WCMD_RMDIR:
        return_code = WCMD_remove_dir(parms_start);
        break;
    case WCMD_SETLOCAL:
        return_code = WCMD_setlocal(parms_start);
        break;
    case WCMD_ENDLOCAL:
        return_code = WCMD_endlocal();
        break;
    case WCMD_SET:
        return_code = WCMD_setshow_env(parms_start);
        break;
    case WCMD_SHIFT:
        return_code = WCMD_shift(parms_start);
        break;
    case WCMD_START:
        return_code = WCMD_start(parms_start);
        break;
    case WCMD_TIME:
        return_code = WCMD_setshow_time();
        break;
    case WCMD_TITLE:
        return_code = WCMD_title(parms_start);
        break;
    case WCMD_TYPE:
        return_code = WCMD_type(parms_start);
        break;
    case WCMD_VER:
        return_code = WCMD_version();
        break;
    case WCMD_VERIFY:
        return_code = WCMD_verify();
        break;
    case WCMD_VOL:
        return_code = WCMD_volume();
        break;
    case WCMD_PUSHD:
        return_code = WCMD_pushd(parms_start);
        break;
    case WCMD_POPD:
        return_code = WCMD_popd();
        break;
    case WCMD_ASSOC:
        return_code = WCMD_assoc(parms_start, TRUE);
        break;
    case WCMD_COLOR:
        return_code = WCMD_color();
        break;
    case WCMD_FTYPE:
        return_code = WCMD_assoc(parms_start, FALSE);
        break;
    case WCMD_MORE:
        return_code = WCMD_more(parms_start);
        break;
    case WCMD_CHOICE:
        return_code = WCMD_choice(parms_start);
        break;
    case WCMD_MKLINK:
        return_code = WCMD_mklink(parms_start);
        break;
    case WCMD_CHGDRIVE:
        return_code = WCMD_change_drive(cmd[0]);
        break;
    case WCMD_EXIT:
        return_code = WCMD_exit();
        break;
    default:
        FIXME("Shouldn't happen %d\n", cmd_index);
    case WCMD_FOR: /* can happen in 'call for...' and should fail */
    case WCMD_IF:
        return_code = RETURN_CODE_CANT_LAUNCH;
        break;
    }
    return return_code;
}

/*****************************************************************************
 * Process one command. If the command is EXIT this routine does not return.
 * We will recurse through here executing batch files.
 * Note: If call is used to a non-existing program, we reparse the line and
 *       try to run it as an internal command. 'retrycall' represents whether
 *       we are attempting this retry.
 */
static RETURN_CODE execute_single_command(const WCHAR *command)
{
    struct search_command sc;
    RETURN_CODE return_code;
    WCHAR *cmd;

    TRACE("command on entry:%s\n", wine_dbgstr_w(command));

    /* Move copy of the command onto the heap so it can be expanded */
    cmd = xalloc(MAXSTRING * sizeof(WCHAR));
    lstrcpyW(cmd, command);
    handleExpansion(cmd, TRUE);

    TRACE("Command: '%s'\n", wine_dbgstr_w(cmd));

    return_code = search_command(cmd, &sc, TRUE);
    if (return_code != NO_ERROR && sc.cmd_index == WCMD_EXIT + 1)
    {
        /* Not found anywhere - give up */
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_NO_COMMAND_FOUND), cmd);

        /* If a command fails to launch, it sets errorlevel 9009 - which
         * does not seem to have any associated constant definition
         */
        errorlevel = RETURN_CODE_CANT_LAUNCH;
        return_code = ERROR_INVALID_FUNCTION;
    }
    else if (sc.cmd_index <= WCMD_EXIT && (return_code != NO_ERROR || (!sc.has_path && !sc.has_extension)))
        return_code = WCMD_run_builtin_command(sc.cmd_index, cmd);
    else
    {
        BOOL prev_echo_mode = echo_mode;
        if (*sc.path)
            return_code = run_full_path(sc.path, cmd, FALSE);
        echo_mode = prev_echo_mode;
    }
    free(cmd);
    return return_code;
}

RETURN_CODE WCMD_call_command(WCHAR *command)
{
  struct search_command sc;
  RETURN_CODE return_code;

  return_code = search_command(command, &sc, FALSE);
  if (return_code == NO_ERROR)
  {
      unsigned old_echo_mode = echo_mode;
      if (!*sc.path) return NO_ERROR;
      return_code = run_full_path(sc.path, command, TRUE);
      if (interactive) echo_mode = old_echo_mode;
      return return_code;
  }

  if (sc.cmd_index <= WCMD_EXIT)
      return errorlevel = WCMD_run_builtin_command(sc.cmd_index, command);

  /* Not found anywhere - give up */
  WCMD_output_stderr(WCMD_LoadMessage(WCMD_NO_COMMAND_FOUND), command);

  /* If a command fails to launch, it sets errorlevel 9009 - which
   * does not seem to have any associated constant definition
   */
  errorlevel = RETURN_CODE_CANT_LAUNCH;
  return ERROR_INVALID_FUNCTION;
}

/*************************************************************************
 * WCMD_LoadMessage
 *    Load a string from the resource file, handling any error
 *    Returns string retrieved from resource file
 */
WCHAR *WCMD_LoadMessage(UINT id) {
    static WCHAR msg[2048];

    if (!LoadStringW(GetModuleHandleW(NULL), id, msg, ARRAY_SIZE(msg))) {
       WINE_FIXME("LoadString failed with %ld\n", GetLastError());
       lstrcpyW(msg, L"Failed!");
    }
    return msg;
}

static WCHAR *find_chr(WCHAR *in, WCHAR *last, const WCHAR *delims)
{
    for (; in < last; in++)
        if (wcschr(delims, *in)) return in;
    return NULL;
}

/***************************************************************************
 * WCMD_IsEndQuote
 *
 *   Checks if the quote pointed to is the end-quote.
 *
 *   Quotes end if:
 *
 *   1) The current parameter ends at EOL or at the beginning
 *      of a redirection or pipe and not in a quote section.
 *
 *   2) If the next character is a space and not in a quote section.
 *
 *   Returns TRUE if this is an end quote, and FALSE if it is not.
 *
 */
static BOOL WCMD_IsEndQuote(const WCHAR *quote, int quoteIndex)
{
    int quoteCount = quoteIndex;
    int i;

    /* If we are not in a quoted section, then we are not an end-quote */
    if(quoteIndex == 0)
    {
        return FALSE;
    }

    /* Check how many quotes are left for this parameter */
    for(i=0;quote[i];i++)
    {
        if(quote[i] == '"')
        {
            quoteCount++;
        }

        /* Quote counting ends at EOL, redirection, space or pipe if current quote is complete */
        else if(((quoteCount % 2) == 0)
            && ((quote[i] == '<') || (quote[i] == '>') || (quote[i] == '|') || (quote[i] == ' ') ||
                (quote[i] == '&')))
        {
            break;
        }
    }

    /* If the quote is part of the last part of a series of quotes-on-quotes, then it must
       be an end-quote */
    if(quoteIndex >= (quoteCount / 2))
    {
        return TRUE;
    }

    /* No cigar */
    return FALSE;
}

static WCHAR *for_fileset_option_split(WCHAR *from, const WCHAR* key)
{
    size_t len = wcslen(key);

    if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
                       from, len, key, len) != CSTR_EQUAL)
        return NULL;
    from += len;
    if (len && key[len - 1] == L'=')
        while (*from && *from != L' ' && *from != L'\t') from++;
    return from;
}

static CMD_FOR_CONTROL *for_control_parse(WCHAR *opts_var)
{
    CMD_FOR_CONTROL *for_ctrl;
    enum for_control_operator for_op;
    WCHAR mode = L' ', option;
    WCHAR options[MAXSTRING];
    WCHAR *arg;
    unsigned flags = 0;
    int arg_index;
    unsigned varidx;

    options[0] = L'\0';
    /* native allows two options only in the /D /R case, a repetition of the option
     * and prints an error otherwise
     */
    for (arg_index = 0; ; arg_index++)
    {
        arg = WCMD_parameter(opts_var, arg_index, NULL, FALSE, FALSE);

        if (!arg || *arg != L'/') break;
        option = towupper(arg[1]);
        if (mode != L' ' && (mode != L'D' || option != 'R') && mode != option)
            break;
        switch (option)
        {
        case L'R':
            if (mode == L'D')
            {
                mode = L'X';
                break;
            }
            /* fall thru */
        case L'D':
        case L'L':
        case L'F':
            mode = option;
            break;
        default:
            /* error unexpected 'arg' at this time */
            WARN("for qualifier '%c' unhandled\n", *arg);
            goto syntax_error;
        }
    }
    switch (mode)
    {
    case L' ':
        for_op = CMD_FOR_FILETREE;
        flags = CMD_FOR_FLAG_TREE_INCLUDE_FILES;
        break;
    case L'D':
        for_op = CMD_FOR_FILETREE;
        flags = CMD_FOR_FLAG_TREE_INCLUDE_DIRECTORIES;
        break;
    case L'X':
        for_op = CMD_FOR_FILETREE;
        flags = CMD_FOR_FLAG_TREE_INCLUDE_DIRECTORIES | CMD_FOR_FLAG_TREE_RECURSE;
        break;
    case L'R':
        for_op = CMD_FOR_FILETREE;
        flags = CMD_FOR_FLAG_TREE_INCLUDE_FILES | /*CMD_FOR_FLAG_TREE_INCLUDE_DIRECTORIES | */CMD_FOR_FLAG_TREE_RECURSE;
        break;
    case L'L':
        for_op = CMD_FOR_NUMBERS;
        break;
    case L'F':
        for_op = CMD_FOR_FILE_SET;
        break;
    default:
        FIXME("Unexpected situation\n");
        return NULL;
    }

    if (mode == L'F' || mode == L'R')
    {
        /* Retrieve next parameter to see if is root/options (raw form required
         * with for /f, or unquoted in for /r)
         */
        arg = WCMD_parameter(opts_var, arg_index, NULL, for_op == CMD_FOR_FILE_SET, FALSE);

        /* Next parm is either qualifier, path/options or variable -
         * only care about it if it is the path/options
         */
        if (arg && *arg != L'/' && *arg != L'%')
        {
            arg_index++;
            wcscpy(options, arg);
        }
    }

    /* Ensure line continues with variable */
    arg = WCMD_parameter(opts_var, arg_index++, NULL, FALSE, FALSE);
    if (!arg || *arg != L'%' || !for_var_is_valid(arg[1]))
        goto syntax_error; /* FIXME native prints the offending token "%<whatever>" was unexpected at this time */
    varidx = arg[1];
    for_ctrl = xalloc(sizeof(*for_ctrl));
    if (for_op == CMD_FOR_FILE_SET)
    {
        size_t len = wcslen(options);
        WCHAR *p = options, *end;
        WCHAR eol = L'\0';
        int num_lines_to_skip = 0;
        BOOL use_backq = FALSE;
        WCHAR *delims = NULL, *tokens = NULL;
        /* strip enclosing double-quotes when present */
        if (len >= 2 && p[0] == L'"' && p[len - 1] == L'"')
        {
            p[len - 1] = L'\0';
            p++;
        }
        for ( ; *(p = WCMD_skip_leading_spaces(p)); p = end)
        {
            /* Save End of line character (Ignore line if first token (based on delims) starts with it) */
            if ((end = for_fileset_option_split(p, L"eol=")))
            {
                /* assuming one char for eol marker */
                if (end != p + 5) goto syntax_error;
                eol = p[4];
            }
            /* Save number of lines to skip (Can be in base 10, hex (0x...) or octal (0xx) */
            else if ((end = for_fileset_option_split(p, L"skip=")))
            {
                WCHAR *nextchar;
                num_lines_to_skip = wcstoul(p + 5, &nextchar, 0);
                if (end != nextchar) goto syntax_error;
            }
            /* Save if usebackq semantics are in effect */
            else if ((end = for_fileset_option_split(p, L"usebackq")))
                use_backq = TRUE;
            /* Save the supplied delims */
            else if ((end = for_fileset_option_split(p, L"delims=")))
            {
                size_t copy_len;

                /* interpret space when last character of whole options string as part of delims= */
                if (end[0] && !end[1]) end++;
                copy_len = end - (p + 7) /* delims= */;
                delims = xalloc((copy_len + 1) * sizeof(WCHAR));
                memcpy(delims, p + 7, copy_len * sizeof(WCHAR));
                delims[copy_len] = L'\0';
            }
            /* Save the tokens being requested */
            else if ((end = for_fileset_option_split(p, L"tokens=")))
            {
                size_t copy_len;

                copy_len = end - (p + 7) /* tokens= */;
                tokens = xalloc((copy_len + 1) * sizeof(WCHAR));
                memcpy(tokens, p + 7, copy_len * sizeof(WCHAR));
                tokens[copy_len] = L'\0';
            }
            else
            {
                WARN("FOR option not found %ls\n", p);
                goto syntax_error;
            }
        }
        for_control_create_fileset(flags, varidx, eol, num_lines_to_skip, use_backq,
                                   delims ? delims : xstrdupW(L" \t"),
                                   tokens ? tokens : xstrdupW(L"1"), for_ctrl);
    }
    else
        for_control_create(for_op, flags, options, varidx, for_ctrl);
    return for_ctrl;
syntax_error:
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
    return NULL;
}

/* used to store additional information dedicated a given token */
union token_parameter
{
    WCHAR *command;
    CMD_REDIRECTION *redirection;
    void *none;
};

struct node_builder
{
    unsigned num;
    unsigned allocated;
    struct token
    {
        enum builder_token
        {
            TKN_EOF, TKN_EOL, TKN_REDIRECTION, TKN_FOR, TKN_IN, TKN_DO, TKN_IF, TKN_ELSE,
            TKN_OPENPAR, TKN_CLOSEPAR, TKN_AMP, TKN_BARBAR, TKN_AMPAMP, TKN_BAR, TKN_COMMAND,
        } token;
        union token_parameter parameter;
    } *stack;
    unsigned pos;
    unsigned opened_parenthesis;
};

static const char* debugstr_token(enum builder_token tkn, union token_parameter tkn_pmt)
{
    static const char *tokens[] = {"EOF", "EOL", "REDIR", "FOR", "IN", "DO", "IF", "ELSE",
                                   "(", ")", "&", "||", "&&", "|", "CMD"};

    if (tkn >= ARRAY_SIZE(tokens)) return "<<<>>>";
    switch (tkn)
    {
    case TKN_COMMAND:     return wine_dbg_sprintf("%s {{%s}}", tokens[tkn], debugstr_w(tkn_pmt.command));
    case TKN_REDIRECTION: return wine_dbg_sprintf("%s {{%s}}", tokens[tkn], debugstr_redirection(tkn_pmt.redirection));
    default:              return wine_dbg_sprintf("%s", tokens[tkn]);
    }
}

static unsigned token_get_precedence(enum builder_token tkn)
{
    switch (tkn)
    {
    case TKN_EOL: return 5;
    case TKN_BAR: return 4;
    case TKN_AMPAMP: return 3;
    case TKN_BARBAR: return 2;
    case TKN_AMP: return 1;
    default: return 0;
    }
}

static void node_builder_init(struct node_builder *builder)
{
    memset(builder, 0, sizeof(*builder));
}

static void node_builder_dispose(struct node_builder *builder)
{
    free(builder->stack);
}

static void node_builder_push_token_parameter(struct node_builder *builder, enum builder_token tkn, union token_parameter pmt)
{
    if (builder->allocated <= builder->num)
    {
        unsigned sz = builder->allocated ? 2 * builder->allocated : 64;
        builder->stack = xrealloc(builder->stack, sz * sizeof(builder->stack[0]));
        builder->allocated = sz;
    }
    builder->stack[builder->num].token = tkn;
    builder->stack[builder->num].parameter = pmt;

    if (tkn == TKN_OPENPAR)
        builder->opened_parenthesis++;
    if (tkn == TKN_CLOSEPAR)
        builder->opened_parenthesis--;
    builder->num++;
}

static void node_builder_push_token(struct node_builder *builder, enum builder_token tkn)
{
    union token_parameter pmt = {.none = NULL};
    node_builder_push_token_parameter(builder, tkn, pmt);
}

static enum builder_token node_builder_peek_next_token(struct node_builder *builder, union token_parameter *pmt)
{
    enum builder_token tkn;

    if (builder->pos >= builder->num)
    {
        tkn = TKN_EOF;
        if (pmt) pmt->none = NULL;
    }
    else
    {
        tkn = builder->stack[builder->pos].token;
        if (pmt)
            *pmt = builder->stack[builder->pos].parameter;
    }
    return tkn;
}

static void node_builder_consume(struct node_builder *builder)
{
    builder->stack[builder->pos].parameter.none = NULL;
    builder->pos++;
}

static BOOL node_builder_expect_token(struct node_builder *builder, enum builder_token tkn)
{
    if (builder->pos >= builder->num || builder->stack[builder->pos].token != tkn)
        return FALSE;
    node_builder_consume(builder);
    return TRUE;
}

static enum builder_token node_builder_top(const struct node_builder *builder, unsigned d)
{
    return builder->num > d ? builder->stack[builder->num - (d + 1)].token : TKN_EOF;
}

static void redirection_list_append(CMD_REDIRECTION **redir, CMD_REDIRECTION *last)
{
    if (last)
    {
        for ( ; *redir; redir = &(*redir)->next) {}
        *redir = last;
    }
}

static BOOL node_builder_parse(struct node_builder *builder, unsigned precedence, CMD_NODE **result)
{
    CMD_REDIRECTION *redir = NULL;
    unsigned bogus_line;
    CMD_NODE *left = NULL, *right;
    CMD_FOR_CONTROL *for_ctrl = NULL;
    union token_parameter pmt;
    enum builder_token tkn;
    BOOL done;

#define ERROR_IF(x) if (x) {bogus_line = __LINE__; goto error_handling;}
    do
    {
        tkn = node_builder_peek_next_token(builder, &pmt);
        done = FALSE;

        TRACE("\t%u/%u) %s\n", builder->pos, builder->num, debugstr_token(tkn, pmt));
        switch (tkn)
        {
        case TKN_EOF:
            /* always an error to read past end of tokens */
            ERROR_IF(TRUE);
            break;
        case TKN_EOL:
            done = TRUE;
            break;
        case TKN_OPENPAR:
            ERROR_IF(left);
            node_builder_consume(builder);
            /* empty lines are allowed here */
            while ((tkn = node_builder_peek_next_token(builder, &pmt)) == TKN_EOL)
                node_builder_consume(builder);
            ERROR_IF(!node_builder_parse(builder, 0, &left));
            /* temp before using precedence in chaining */
            while ((tkn = node_builder_peek_next_token(builder, &pmt)) != TKN_CLOSEPAR)
            {
                ERROR_IF(tkn != TKN_EOL);
                node_builder_consume(builder);
                /* FIXME potential empty here?? */
                ERROR_IF(!node_builder_parse(builder, 0, &right));
                left = node_create_binary(CMD_CONCAT, left, right);
            }
            node_builder_consume(builder);
            /* if we had redirection before '(', add them up front */
            if (redir)
            {
                redirection_list_append(&redir, left->redirects);
                left->redirects = redir;
                redir = NULL;
            }
            /* just in case we're handling: "(if ...) > a"... to not trigger errors in TKN_REDIRECTION */
            while (node_builder_peek_next_token(builder, &pmt) == TKN_REDIRECTION)
            {
                redirection_list_append(&left->redirects, pmt.redirection);
                node_builder_consume(builder);
            }
            break;
            /* shouldn't appear here... error handling ? */
        case TKN_IN:
            /* following tokens act as a delimiter for inner context; return to upper */
        case TKN_CLOSEPAR:
        case TKN_ELSE:
        case TKN_DO:
            done = TRUE;
            break;
        case TKN_AMP:
            ERROR_IF(!left);
            if (!(done = token_get_precedence(tkn) <= precedence))
            {
                node_builder_consume(builder);
                if (node_builder_peek_next_token(builder, &pmt) == TKN_CLOSEPAR)
                {
                    done = TRUE;
                    break;
                }
                ERROR_IF(!node_builder_parse(builder, token_get_precedence(tkn), &right));
                left = node_create_binary(CMD_CONCAT, left, right);
            }
            break;
        case TKN_AMPAMP:
            ERROR_IF(!left);
            if (!(done = token_get_precedence(tkn) <= precedence))
            {
                node_builder_consume(builder);
                ERROR_IF(!node_builder_parse(builder, token_get_precedence(tkn), &right));
                left = node_create_binary(CMD_ONSUCCESS, left, right);
            }
            break;
        case TKN_BAR:
            ERROR_IF(!left);
            if (!(done = token_get_precedence(tkn) <= precedence))
            {
                node_builder_consume(builder);
                ERROR_IF(!node_builder_parse(builder, token_get_precedence(tkn), &right));
                left = node_create_binary(CMD_PIPE, left, right);
            }
            break;
        case TKN_BARBAR:
            ERROR_IF(!left);
            if (!(done = token_get_precedence(tkn) <= precedence))
            {
                node_builder_consume(builder);
                ERROR_IF(!node_builder_parse(builder, token_get_precedence(tkn), &right));
                left = node_create_binary(CMD_ONFAILURE, left, right);
            }
            break;
        case TKN_COMMAND:
            ERROR_IF(left);
            left = node_create_single(pmt.command);
            node_builder_consume(builder);
            left->redirects = redir;
            redir = NULL;
            break;
        case TKN_IF:
            ERROR_IF(left);
            ERROR_IF(redir);
            {
                WCHAR *end;
                CMD_IF_CONDITION cond;
                CMD_NODE *then_block;
                CMD_NODE *else_block;

                node_builder_consume(builder);
                tkn = node_builder_peek_next_token(builder, &pmt);
                ERROR_IF(tkn != TKN_COMMAND);
                if (!wcscmp(pmt.command, L"/?"))
                {
                    node_builder_consume(builder);
                    free(pmt.command);
                    left = node_create_single(command_create(L"help if", 7));
                    break;
                }
                ERROR_IF(!if_condition_parse(pmt.command, &end, &cond));
                free(pmt.command);
                node_builder_consume(builder);
                if (!node_builder_parse(builder, 0, &then_block))
                {
                    if_condition_dispose(&cond);
                    ERROR_IF(TRUE);
                }
                tkn = node_builder_peek_next_token(builder, NULL);
                if (tkn == TKN_ELSE)
                {
                    node_builder_consume(builder);
                    if (!node_builder_parse(builder, 0, &else_block))
                    {
                        if_condition_dispose(&cond);
                        node_dispose_tree(then_block);
                        ERROR_IF(TRUE);
                    }
                }
                else
                    else_block = NULL;
                left = node_create_if(&cond, then_block, else_block);
            }
            break;
        case TKN_FOR:
            ERROR_IF(left);
            ERROR_IF(redir);
            {
                CMD_NODE *do_block;

                node_builder_consume(builder);
                tkn = node_builder_peek_next_token(builder, &pmt);
                ERROR_IF(tkn != TKN_COMMAND);
                if (!wcscmp(pmt.command, L"/?"))
                {
                    node_builder_consume(builder);
                    free(pmt.command);
                    left = node_create_single(command_create(L"help for", 8));
                    break;
                }
                node_builder_consume(builder);
                for_ctrl = for_control_parse(pmt.command);
                free(pmt.command);
                ERROR_IF(for_ctrl == NULL);
                ERROR_IF(!node_builder_expect_token(builder, TKN_IN));
                ERROR_IF(!node_builder_expect_token(builder, TKN_OPENPAR));
                do
                {
                    tkn = node_builder_peek_next_token(builder, &pmt);
                    switch (tkn)
                    {
                    case TKN_COMMAND:
                        for_control_append_set(for_ctrl, pmt.command);
                        free(pmt.command);
                        break;
                    case TKN_EOL:
                    case TKN_CLOSEPAR:
                        break;
                    default:
                        ERROR_IF(TRUE);
                    }
                    node_builder_consume(builder);
                } while (tkn != TKN_CLOSEPAR);
                ERROR_IF(!node_builder_expect_token(builder, TKN_DO));
                ERROR_IF(!node_builder_parse(builder, 0, &do_block));
                left = node_create_for(for_ctrl, do_block);
                for_ctrl = NULL;
            }
            break;
        case TKN_REDIRECTION:
            ERROR_IF(left && (left->op == CMD_IF || left->op == CMD_FOR));
            redirection_list_append(left ? &left->redirects : &redir, pmt.redirection);
            node_builder_consume(builder);
            break;
        }
    } while (!done);
#undef ERROR_IF
    *result = left;
    return TRUE;
error_handling:
    TRACE("Parser failed at line %u:token %s\n", bogus_line, debugstr_token(tkn, pmt));
    node_dispose_tree(left);
    redirection_dispose_list(redir);
    if (for_ctrl) for_control_dispose(for_ctrl);

    return FALSE;
}

static BOOL node_builder_generate(struct node_builder *builder, CMD_NODE **node)
{
    union token_parameter tkn_pmt;
    enum builder_token tkn;

    if (builder->opened_parenthesis)
    {
        TRACE("Brackets do not match, error out without executing.\n");
        WCMD_output_stderr(WCMD_LoadMessage(WCMD_BADPAREN));
    }
    else
    {
        if (!builder->num) /* line without tokens */
        {
            *node = NULL;
            return TRUE;
        }
        if (node_builder_parse(builder, 0, node) &&
            builder->pos + 1 >= builder->num) /* consumed all tokens? */
            return TRUE;
        /* print error on first unused token */
        if (builder->pos < builder->num)
        {
            WCHAR buffer[MAXSTRING];
            const WCHAR *tknstr;

            tkn = node_builder_peek_next_token(builder, &tkn_pmt);
            switch (tkn)
            {
            case TKN_COMMAND:
                tknstr = tkn_pmt.command;
                break;
            case TKN_EOF:
                tknstr = WCMD_LoadMessage(WCMD_ENDOFFILE);
                break;
            case TKN_EOL:
                tknstr = WCMD_LoadMessage(WCMD_ENDOFLINE);
                break;
            case TKN_REDIRECTION:
                MultiByteToWideChar(CP_ACP, 0, debugstr_redirection(tkn_pmt.redirection), -1, buffer, ARRAY_SIZE(buffer));
                tknstr = buffer;
                break;
            case TKN_AMP:
            case TKN_AMPAMP:
            case TKN_BAR:
            case TKN_BARBAR:
            case TKN_FOR:
            case TKN_IN:
            case TKN_DO:
            case TKN_IF:
            case TKN_ELSE:
            case TKN_OPENPAR:
            case TKN_CLOSEPAR:
                MultiByteToWideChar(CP_ACP, 0, debugstr_token(tkn, tkn_pmt), -1, buffer, ARRAY_SIZE(buffer));
                tknstr = buffer;
                break;
            default:
                FIXME("Unexpected situation\n");
                tknstr = L"";
                break;
            }
            WCMD_output_stderr(WCMD_LoadMessage(WCMD_BADTOKEN), tknstr);
        }
    }
    /* free remaining tokens */
    for (;;)
    {
        tkn = node_builder_peek_next_token(builder, &tkn_pmt);
        if (tkn == TKN_EOF) break;
        if (tkn == TKN_COMMAND) free(tkn_pmt.command);
        if (tkn == TKN_REDIRECTION) redirection_dispose_list(tkn_pmt.redirection);
        node_builder_consume(builder);
    }

    *node = NULL;
    return FALSE;
}

static void lexer_push_command(struct node_builder *builder,
                               WCHAR *command, int *commandLen,
                               WCHAR *redirs,  int *redirLen,
                               WCHAR **copyTo, int **copyToLen)
{
    union token_parameter tkn_pmt;

    /* push first all redirections */
    if (*redirLen)
    {
        WCHAR *pos;
        WCHAR *last = redirs + *redirLen;

        redirs[*redirLen] = 0;

        /* Create redirects, keeping order (eg "2>foo 1>&2") */
        for (pos = redirs; pos; )
        {
            WCHAR *p = find_chr(pos, last, L"<>");
            WCHAR *filename;

            if (!p) break;

            if (*p == L'<')
            {
                unsigned fd = 0;

                if (p > redirs && p[-1] >= L'0' && p[-1] <= L'9') fd = p[-1] - L'0';
                p++;
                if (*p == L'&' && (p[1] >= L'0' && p[1] <= L'9'))
                {
                    tkn_pmt.redirection = redirection_create_clone(fd, p[1] - L'0');
                    p++;
                }
                else
                {
                    filename = WCMD_parameter(p + 1, 0, NULL, FALSE, FALSE);
                    tkn_pmt.redirection = redirection_create_file(REDIR_READ_FROM, 0, filename);
                }
            }
            else
            {
                unsigned fd = 1;
                unsigned op = REDIR_WRITE_TO;

                if (p > redirs && p[-1] >= L'2' && p[-1] <= L'9') fd = p[-1] - L'0';
                if (*++p == L'>') {p++; op = REDIR_WRITE_APPEND;}
                if (*p == L'&' && (p[1] >= L'0' && p[1] <= L'9'))
                {
                    tkn_pmt.redirection = redirection_create_clone(fd, p[1] - '0');
                    p++;
                }
                else
                {
                    filename = WCMD_parameter(p, 0, NULL, FALSE, FALSE);
                    tkn_pmt.redirection = redirection_create_file(op, fd, filename);
                }
            }
            pos = p + 1;
            node_builder_push_token_parameter(builder, TKN_REDIRECTION, tkn_pmt);
        }
    }
    if (*commandLen)
    {
        tkn_pmt.command = command_create(command, *commandLen);
        node_builder_push_token_parameter(builder, TKN_COMMAND, tkn_pmt);
    }
    /* Reset the lengths */
    *commandLen   = 0;
    *redirLen     = 0;
    *copyToLen    = commandLen;
    *copyTo       = command;
}

static WCHAR *fetch_next_line(BOOL feed, BOOL first_line, WCHAR* buffer)
{
    /* display prompt */
    if (interactive && !context)
    {
        /* native does is this way... not symmetrical wrt. echo_mode */
        if (!first_line)
            WCMD_output_asis(WCMD_LoadMessage(WCMD_MOREPROMPT));
        else if (echo_mode)
            WCMD_show_prompt();
    }

    if (feed)
    {
        BOOL ret;
        if (context)
        {
            LARGE_INTEGER zeroli = {.QuadPart = 0};
            HANDLE h = CreateFileW(context->batchfileW, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h == INVALID_HANDLE_VALUE)
            {
                SetLastError(ERROR_FILE_NOT_FOUND);
                WCMD_print_error();
                ret = FALSE;
            }
            else
            {
                ret = SetFilePointerEx(h, context->file_position, NULL, FILE_BEGIN) &&
                    !!WCMD_fgets(buffer, MAXSTRING, h) &&
                    SetFilePointerEx(h, zeroli, &context->file_position, FILE_CURRENT);
                CloseHandle(h);
            }
        }
        else
            ret = !!WCMD_fgets(buffer, MAXSTRING, GetStdHandle(STD_INPUT_HANDLE));
        if (!ret)
        {
            buffer[0] = L'\0';
            return NULL;
        }
    }
    /* Handle truncated input - issue warning */
    if (wcslen(buffer) == MAXSTRING - 1)
    {
        WCMD_output_asis_stderr(WCMD_LoadMessage(WCMD_TRUNCATEDLINE));
        WCMD_output_asis_stderr(buffer);
        WCMD_output_asis_stderr(L"\r\n");
    }
    /* Replace env vars if in a batch context */
    handleExpansion(buffer, FALSE);

    buffer = WCMD_skip_leading_spaces(buffer);
    /* Show prompt before batch line IF echo is on and in batch program */
    if (context && echo_mode && *buffer && *buffer != '@')
    {
        if (first_line)
        {
            const size_t len = wcslen(L"echo.");
            size_t curr_size = wcslen(buffer);
            size_t min_len = curr_size < len ? curr_size : len;
            WCMD_output_asis(L"\r\n");
            WCMD_show_prompt();
            WCMD_output_asis(buffer);
            /* I don't know why Windows puts a space here but it does */
            /* Except for lines starting with 'echo.', 'echo:' or 'echo/'. Ask MS why */
            if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                               buffer, min_len, L"echo.", len) != CSTR_EQUAL
                && CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                  buffer, min_len, L"echo:", len) != CSTR_EQUAL
                && CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                  buffer, min_len, L"echo/", len) != CSTR_EQUAL)
            {
                WCMD_output_asis(L" ");
            }
        }
        else
            WCMD_output_asis(buffer);

        WCMD_output_asis(L"\r\n");
    }

    return buffer;
}

static BOOL lexer_can_accept_do(const struct node_builder *builder)
{
    unsigned d = 0;

    if (node_builder_top(builder, d++) != TKN_CLOSEPAR) return FALSE;
    while (node_builder_top(builder, d) == TKN_COMMAND || node_builder_top(builder, d) == TKN_EOL) d++;
    if (node_builder_top(builder, d++) != TKN_OPENPAR) return FALSE;
    return node_builder_top(builder, d) == TKN_IN;
}

static BOOL lexer_white_space_only(const WCHAR *string, int len)
{
    int i;

    for (i = 0; i < len; i++)
        if (!iswspace(string[i])) return FALSE;
    return TRUE;
}

/***************************************************************************
 * WCMD_ReadAndParseLine
 *
 *   Either uses supplied input or
 *     Reads a file from the handle, and then...
 *   Parse the text buffer, splitting into separate commands
 *     - unquoted && strings split 2 commands but the 2nd is flagged as
 *            following an &&
 *     - ( as the first character just ups the bracket depth
 *     - unquoted ) when bracket depth > 0 terminates a bracket and
 *            adds a CMD_LIST structure with null command
 *     - Anything else gets put into the command string (including
 *            redirects)
 */
enum read_parse_line WCMD_ReadAndParseLine(const WCHAR *optionalcmd, CMD_NODE **output)
{
    WCHAR    *curPos;
    int       inQuotes = 0;
    WCHAR     curString[MAXSTRING];
    int       curStringLen = 0;
    WCHAR     curRedirs[MAXSTRING];
    int       curRedirsLen = 0;
    WCHAR    *curCopyTo;
    int      *curLen;
    static WCHAR    *extraSpace = NULL;  /* Deliberately never freed */
    BOOL      inOneLine = FALSE;
    BOOL      lastWasRedirect = TRUE;
    BOOL      acceptCommand = TRUE;
    struct node_builder builder;
    BOOL      ret;

    *output = NULL;
    /* Allocate working space for a command read from keyboard, file etc */
    if (!extraSpace)
        extraSpace = xalloc((MAXSTRING + 1) * sizeof(WCHAR));

    /* If initial command read in, use that, otherwise get input from handle */
    if (optionalcmd)
        wcscpy(extraSpace, optionalcmd);
    if (!(curPos = fetch_next_line(optionalcmd == NULL, TRUE, extraSpace)))
        return RPL_EOF;

    TRACE("About to parse line (%ls)\n", extraSpace);

    node_builder_init(&builder);

    /* Start with an empty string, copying to the command string */
    curStringLen = 0;
    curRedirsLen = 0;
    curCopyTo    = curString;
    curLen       = &curStringLen;
    lastWasRedirect = FALSE;  /* Required e.g. for spaces between > and filename */

    curPos = WCMD_strip_for_command_start(curPos);
    /* Parse every character on the line being processed */
    while (*curPos != 0x00) {

      WCHAR thisChar;

      /* Debugging AID:
      WINE_TRACE("Looking at '%c' (len:%d)\n", *curPos, *curLen);
      */

      /* Prevent overflow caused by the caret escape char */
      if (*curLen >= MAXSTRING) {
        WINE_ERR("Overflow detected in command\n");
        return RPL_SYNTAXERROR;
      }

      /* Certain commands need special handling */
      if (curStringLen == 0 && curCopyTo == curString) {
        if (acceptCommand)
          curPos = WCMD_strip_for_command_start(curPos);
        /* If command starts with 'rem ' or identifies a label, ignore any &&, ( etc. */
        if (WCMD_keyword_ws_found(L"rem", curPos) || *curPos == ':') {
          inOneLine = TRUE;

        } else if (WCMD_keyword_ws_found(L"for", curPos)) {
          node_builder_push_token(&builder, TKN_FOR);

          curPos = WCMD_skip_leading_spaces(curPos + 3); /* "for */
        /* If command starts with 'if ' or 'else ', handle ('s mid line. We should ensure this
           is only true in the command portion of the IF statement, but this
           should suffice for now.
           To be able to handle ('s in the condition part take as much as evaluate_if_condition
           would take and skip parsing it here. */
          acceptCommand = FALSE;
        } else if (acceptCommand && WCMD_keyword_ws_found(L"if", curPos)) {
          WCHAR *command;

          node_builder_push_token(&builder, TKN_IF);

          curPos = WCMD_skip_leading_spaces(curPos + 2); /* "if" */
          if (if_condition_parse(curPos, &command, NULL))
          {
              int if_condition_len = command - curPos;
              TRACE("p: %s, command: %s, if_condition_len: %d\n",
                    wine_dbgstr_w(curPos), wine_dbgstr_w(command), if_condition_len);
              memcpy(&curCopyTo[*curLen], curPos, if_condition_len * sizeof(WCHAR));
              (*curLen) += if_condition_len;
              curPos += if_condition_len;

              /* FIXME we do parsing twice of condition (once here, second time in node_builder_parse) */
              lexer_push_command(&builder, curString, &curStringLen,
                                 curRedirs, &curRedirsLen,
                                 &curCopyTo, &curLen);

          }
          acceptCommand = TRUE;
          continue;
        } else if (WCMD_keyword_ws_found(L"else", curPos)) {
          acceptCommand = TRUE;
          node_builder_push_token(&builder, TKN_ELSE);

          curPos = WCMD_skip_leading_spaces(curPos + 4 /* else */);
          continue;

        /* In a for loop, the DO command will follow a close bracket followed by
           whitespace, followed by DO, ie closeBracket inserts a NULL entry, curLen
           is then 0, and all whitespace is skipped                                */
        } else if (lexer_can_accept_do(&builder) && WCMD_keyword_ws_found(L"do", curPos)) {

          WINE_TRACE("Found 'DO '\n");
          acceptCommand = TRUE;

          node_builder_push_token(&builder, TKN_DO);
          curPos = WCMD_skip_leading_spaces(curPos + 2 /* do */);
          continue;
        }
      } else if (curCopyTo == curString) {

        /* Special handling for the 'FOR' command */
          if (node_builder_top(&builder, 0) == TKN_FOR) {
          WINE_TRACE("Found 'FOR ', comparing next parm: '%s'\n", wine_dbgstr_w(curPos));

          if (WCMD_keyword_ws_found(L"in", curPos)) {
            WINE_TRACE("Found 'IN '\n");

            lexer_push_command(&builder, curString, &curStringLen,
                               curRedirs, &curRedirsLen,
                               &curCopyTo, &curLen);
            node_builder_push_token(&builder, TKN_IN);
            curPos = WCMD_skip_leading_spaces(curPos + 2 /* in */);
            continue;
          }
        }
      }

      /* Nothing 'ends' a one line statement (e.g. REM or :labels mean
         the &&, quotes and redirection etc are ineffective, so just force
         the use of the default processing by skipping character specific
         matching below)                                                   */
      if (!inOneLine) thisChar = *curPos;
      else            thisChar = 'X';  /* Character with no special processing */

      switch (thisChar) {

      case '=': /* drop through - ignore token delimiters at the start of a command */
      case ',': /* drop through - ignore token delimiters at the start of a command */
      case '\t':/* drop through - ignore token delimiters at the start of a command */
      case ' ':
                /* If a redirect in place, it ends here */
                if (!inQuotes && !lastWasRedirect) {

                  /* If finishing off a redirect, add a whitespace delimiter */
                  if (curCopyTo == curRedirs) {
                      curCopyTo[(*curLen)++] = ' ';
                  }
                  curCopyTo = curString;
                  curLen = &curStringLen;
                }
                if (*curLen > 0) {
                  curCopyTo[(*curLen)++] = *curPos;
                }
                break;

      case '>': /* drop through - handle redirect chars the same */
      case '<':
                /* Make a redirect start here */
                if (!inQuotes) {
                  curCopyTo = curRedirs;
                  curLen = &curRedirsLen;
                  lastWasRedirect = TRUE;
                }

                /* See if 1>, 2> etc, in which case we have some patching up
                   to do (provided there's a preceding whitespace, and enough
                   chars read so far) */
                if (curPos[-1] >= '1' && curPos[-1] <= '9'
                        && (curStringLen == 1 ||
                            curPos[-2] == ' ' || curPos[-2] == '\t')) {
                    curStringLen--;
                    curString[curStringLen] = 0x00;
                    curCopyTo[(*curLen)++] = *(curPos-1);
                }

                curCopyTo[(*curLen)++] = *curPos;

                /* If a redirect is immediately followed by '&' (ie. 2>&1) then
                    do not process that ampersand as an AND operator */
                if ((thisChar == '>' || thisChar == '<') && *(curPos+1) == '&') {
                    curCopyTo[(*curLen)++] = *(curPos+1);
                    curPos++;
                }
                break;

      case '|': /* Pipe character only if not || */
                if (!inQuotes) {
                  lastWasRedirect = FALSE;

                  lexer_push_command(&builder, curString, &curStringLen,
                                     curRedirs, &curRedirsLen,
                                     &curCopyTo, &curLen);

                  if (*(curPos+1) == '|') {
                    curPos++; /* Skip other | */
                    node_builder_push_token(&builder, TKN_BARBAR);
                  } else {
                    node_builder_push_token(&builder, TKN_BAR);
                  }
                  acceptCommand = TRUE;
                } else {
                  curCopyTo[(*curLen)++] = *curPos;
                }
                break;

      case '"': if (WCMD_IsEndQuote(curPos, inQuotes)) {
                    inQuotes--;
                } else {
                    inQuotes++; /* Quotes within quotes are fun! */
                }
                curCopyTo[(*curLen)++] = *curPos;
                lastWasRedirect = FALSE;
                break;

      case '(': /* If a '(' is the first non whitespace in a command portion
                   ie start of line or just after &&, then we read until an
                   unquoted ) is found                                       */
                lastWasRedirect = FALSE;

                if (inQuotes) {
                  curCopyTo[(*curLen)++] = *curPos;

                /* In a FOR loop, an unquoted '(' may occur straight after
                      IN or DO
                   In an IF statement just handle it regardless as we don't
                      parse the operands
                   In an ELSE statement, only allow it straight away after
                      the ELSE and whitespace
                 */
                } else if ((acceptCommand || node_builder_top(&builder, 0) == TKN_IN) &&
                           lexer_white_space_only(curString, curStringLen)) {
                  node_builder_push_token(&builder, TKN_OPENPAR);
                  acceptCommand = TRUE;
                } else {
                  curCopyTo[(*curLen)++] = *curPos;
                }
                break;

      case '^': if (!inQuotes) {
                  /* If we reach the end of the input, we need to wait for more */
                  if (curPos[1] == L'\0') {
                    TRACE("Caret found at end of line\n");
                    extraSpace[0] = L'^';
                    if (optionalcmd) break;
                    if (!fetch_next_line(TRUE, FALSE, extraSpace + 1))
                        break;
                    if (!extraSpace[1]) /* empty line */
                    {
                        extraSpace[1] = L'\r';
                        if (!fetch_next_line(TRUE, FALSE, extraSpace + 2))
                            break;
                    }
                    curPos = extraSpace;
                    break;
                  }
                  curPos++;
                }
                curCopyTo[(*curLen)++] = *curPos;
                break;

      case '&': if (!inQuotes) {
                  lastWasRedirect = FALSE;

                  /* Add an entry to the command list */
                  lexer_push_command(&builder, curString, &curStringLen,
                                     curRedirs, &curRedirsLen,
                                     &curCopyTo, &curLen);

                  if (*(curPos+1) == '&') {
                    curPos++; /* Skip other & */
                    node_builder_push_token(&builder, TKN_AMPAMP);
                  } else {
                    node_builder_push_token(&builder, TKN_AMP);
                  }
                  acceptCommand = TRUE;
                } else {
                  curCopyTo[(*curLen)++] = *curPos;
                }
                break;

      case ')': if (!inQuotes && builder.opened_parenthesis > 0) {
                  lastWasRedirect = FALSE;

                  /* Add the current command if there is one */
                  lexer_push_command(&builder, curString, &curStringLen,
                                     curRedirs, &curRedirsLen,
                                     &curCopyTo, &curLen);
                  node_builder_push_token(&builder, TKN_CLOSEPAR);
                  acceptCommand = FALSE;
                } else {
                  curCopyTo[(*curLen)++] = *curPos;
                }
                break;
      default:
                lastWasRedirect = FALSE;
                curCopyTo[(*curLen)++] = *curPos;
      }

      curPos++;

      /* If we have reached the end, add this command into the list
         Do not add command to list if escape char ^ was last */
      if (*curPos == L'\0') {
          /* Add an entry to the command list */
          lexer_push_command(&builder, curString, &curStringLen,
                             curRedirs, &curRedirsLen,
                             &curCopyTo, &curLen);
          node_builder_push_token(&builder, TKN_EOL);

          /* If we have reached the end of the string, see if bracketing is outstanding */
          if (builder.opened_parenthesis > 0 && optionalcmd == NULL) {
              TRACE("Need to read more data as outstanding brackets or carets\n");
              inOneLine = FALSE;
              inQuotes = 0;
              acceptCommand = TRUE;

              /* fetch next non empty line */
              do {
                  curPos = fetch_next_line(TRUE, FALSE, extraSpace);
              } while (curPos && *curPos == L'\0');
              curPos = curPos ? WCMD_strip_for_command_start(curPos) : extraSpace;
          }
      }
    }

    ret = node_builder_generate(&builder, output);
    node_builder_dispose(&builder);

    return ret ? RPL_SUCCESS : RPL_SYNTAXERROR;
}

static BOOL if_condition_evaluate(CMD_IF_CONDITION *cond, int *test)
{
    WCHAR expanded_left[MAXSTRING];
    WCHAR expanded_right[MAXSTRING];
    int (WINAPI *cmp)(const WCHAR*, const WCHAR*) = cond->case_insensitive ? lstrcmpiW : lstrcmpW;

    TRACE("About to evaluate condition %s\n", debugstr_if_condition(cond));
    *test = 0;
    switch (cond->op)
    {
    case CMD_IF_ERRORLEVEL:
        {
            WCHAR *endptr;
            int level;

            wcscpy(expanded_left, cond->operand);
            handleExpansion(expanded_left, TRUE);
            level = wcstol(expanded_left, &endptr, 10);
            if (*endptr) return FALSE;
            *test = errorlevel >= level;
        }
        break;
    case CMD_IF_EXIST:
        {
            size_t len;
            WIN32_FIND_DATAW fd;
            HANDLE hff;

            wcscpy(expanded_left, cond->operand);
            handleExpansion(expanded_left, TRUE);
            if ((len = wcslen(expanded_left)))
            {
                if (!wcspbrk(expanded_left, L"*?"))
                    *test = GetFileAttributesW(expanded_left) != INVALID_FILE_ATTRIBUTES;
                else
                {
                    hff = FindFirstFileW(expanded_left, &fd);
                    *test = (hff != INVALID_HANDLE_VALUE);
                    if (*test) FindClose(hff);
                }
            }
        }
        break;
    case CMD_IF_DEFINED:
        wcscpy(expanded_left, cond->operand);
        handleExpansion(expanded_left, TRUE);
        *test = GetEnvironmentVariableW(expanded_left, NULL, 0) > 0;
        break;
    case CMD_IF_BINOP_EQUAL:
        wcscpy(expanded_left, cond->left);
        handleExpansion(expanded_left, TRUE);
        wcscpy(expanded_right, cond->right);
        handleExpansion(expanded_right, TRUE);

        /* == is a special case, as it always compares strings */
        *test = (*cmp)(expanded_left, expanded_right) == 0;
        break;
    default:
        {
            int left_int, right_int;
            WCHAR *end_left, *end_right;
            int cmp_val;

            wcscpy(expanded_left, cond->left);
            handleExpansion(expanded_left, TRUE);
            wcscpy(expanded_right, cond->right);
            handleExpansion(expanded_right, TRUE);

            /* Check if we have plain integers (in decimal, octal or hexadecimal notation) */
            left_int = wcstol(expanded_left, &end_left, 0);
            right_int = wcstol(expanded_right, &end_right, 0);
            if (end_left > expanded_left && !*end_left && end_right > expanded_right && !*end_right)
                cmp_val = left_int - right_int;
            else
                cmp_val = (*cmp)(expanded_left, expanded_right);
            switch (cond->op)
            {
            case CMD_IF_BINOP_LSS: *test = cmp_val <  0; break;
            case CMD_IF_BINOP_LEQ: *test = cmp_val <= 0; break;
            case CMD_IF_BINOP_EQU: *test = cmp_val == 0; break;
            case CMD_IF_BINOP_NEQ: *test = cmp_val != 0; break;
            case CMD_IF_BINOP_GEQ: *test = cmp_val >= 0; break;
            case CMD_IF_BINOP_GTR: *test = cmp_val >  0; break;
            default:
                FIXME("Unexpected comparison operator %u\n", cond->op);
                return FALSE;
            }
        }
        break;
    }
    if (cond->negated) *test ^= 1;
    return TRUE;
}

struct for_loop_variables
{
    unsigned char table[32];
    unsigned last, num_duplicates;
    unsigned has_star;
};

static void for_loop_variables_init(struct for_loop_variables *flv)
{
    flv->last = flv->num_duplicates = 0;
    flv->has_star = FALSE;
}

static BOOL for_loop_variables_push(struct for_loop_variables *flv, unsigned char o)
{
    unsigned i;
    for (i = 0; i < flv->last; i++)
         if (flv->table[i] == o)
         {
             flv->num_duplicates++;
             return TRUE;
         }
    if (flv->last >= ARRAY_SIZE(flv->table)) return FALSE;
    flv->table[flv->last] = o;
    flv->last++;
    return TRUE;
}

static int my_flv_compare(const void *a1, const void *a2)
{
    return *(const char*)a1 - *(const char*)a2;
}

static unsigned for_loop_variables_max(const struct for_loop_variables *flv)
{
    return flv->last == 0 ? -1 : flv->table[flv->last - 1];
}

static BOOL for_loop_fill_variables(const WCHAR *forf_tokens, struct for_loop_variables *flv)
{
    const WCHAR *pos = forf_tokens;

    /* Loop through the token string, parsing it.
     * Valid syntax is: token=m or x-y with comma delimiter and optionally * to finish
     */
    while (*pos)
    {
        unsigned num;
        WCHAR *nextchar;

        if (*pos == L'*')
        {
            if (pos[1] != L'\0') return FALSE;
            qsort(flv->table, flv->last, sizeof(flv->table[0]), my_flv_compare);
            if (flv->num_duplicates)
            {
                flv->num_duplicates++;
                return TRUE;
            }
            flv->has_star = TRUE;
            return for_loop_variables_push(flv, for_loop_variables_max(flv) + 1);
        }

        /* Get the next number */
        num = wcstoul(pos, &nextchar, 10);
        if (!num || num >= 32) return FALSE;
        num--;
        /* range x-y */
        if (*nextchar == L'-')
        {
            unsigned int end = wcstoul(nextchar + 1, &nextchar, 10);
            if (!end || end >= 32) return FALSE;
            end--;
            while (num <= end)
                if (!for_loop_variables_push(flv, num++)) return FALSE;
        }
        else if (!for_loop_variables_push(flv, num)) return FALSE;

        pos = nextchar;
        if (*pos == L',') pos++;
    }
    if (flv->last)
        qsort(flv->table, flv->last, sizeof(flv->table[0]), my_flv_compare);
    else
        for_loop_variables_push(flv, 0);
    return TRUE;
}

static RETURN_CODE for_loop_fileset_parse_line(CMD_NODE *node, unsigned varidx, WCHAR *buffer,
                                               WCHAR forf_eol, const WCHAR *forf_delims, const WCHAR *forf_tokens)
{
    RETURN_CODE return_code = NO_ERROR;
    struct for_loop_variables flv;
    WCHAR *parm;
    unsigned i;

    for_loop_variables_init(&flv);
    if (!for_loop_fill_variables(forf_tokens, &flv))
    {
        TRACE("Error while parsing tokens=%ls\n", forf_tokens);
        return ERROR_INVALID_FUNCTION;
    }

    TRACE("Using var=%s on %u max%s\n", debugstr_for_var(varidx), flv.last, flv.has_star ? " with star" : "");
    /* Empty out variables */
    for (i = 0; i < flv.last + flv.num_duplicates; i++)
        WCMD_set_for_loop_variable(varidx + i, L"");

    for (i = 0; i < flv.last; i++)
    {
        if (flv.has_star && i + 1 == flv.last)
        {
            WCMD_parameter_with_delims(buffer, flv.table[i], &parm, FALSE, FALSE, forf_delims);
            TRACE("Parsed all remaining tokens %d(%s) as parameter %s\n",
                  flv.table[i], debugstr_for_var(varidx + i), wine_dbgstr_w(parm));
            if (parm)
                WCMD_set_for_loop_variable(varidx + i, parm);
            break;
        }
        /* Extract the token number requested and set into the next variable context */
        parm = WCMD_parameter_with_delims(buffer, flv.table[i], NULL, TRUE, FALSE, forf_delims);
        TRACE("Parsed token %d(%s) as parameter %s\n",
              flv.table[i], debugstr_for_var(varidx + i), wine_dbgstr_w(parm));
        if (parm)
            WCMD_set_for_loop_variable(varidx + i, parm);
    }

    /* Execute the body of the for loop with these values */
    if (forloopcontext->variable[varidx] && forloopcontext->variable[varidx][0] != forf_eol)
    {
        return_code = node_execute(node);
    }
    else
    {
        TRACE("Skipping line because of eol\n");
    }
    return return_code;
}

void WCMD_save_for_loop_context(BOOL reset)
{
    FOR_CONTEXT *new = xalloc(sizeof(*new));
    if (reset)
        memset(new, 0, sizeof(*new));
    else /* clone existing */
        *new = *forloopcontext;
    new->previous = forloopcontext;
    forloopcontext = new;
}

void WCMD_restore_for_loop_context(void)
{
    FOR_CONTEXT *old = forloopcontext->previous;
    unsigned varidx;
    if (!old)
    {
        FIXME("Unexpected situation\n");
        return;
    }
    for (varidx = 0; varidx < ARRAY_SIZE(forloopcontext->variable); varidx++)
    {
        if (forloopcontext->variable[varidx] != old->variable[varidx])
            free(forloopcontext->variable[varidx]);
    }
    free(forloopcontext);
    forloopcontext = old;
}

void WCMD_set_for_loop_variable(unsigned varidx, const WCHAR *value)
{
    if (!for_var_is_valid(varidx)) return;
    if (forloopcontext->previous &&
        forloopcontext->previous->variable[varidx] != forloopcontext->variable[varidx])
        free(forloopcontext->variable[varidx]);
    forloopcontext->variable[varidx] = xstrdupW(value);
}

static BOOL match_ending_delim(WCHAR *string)
{
    WCHAR *to = string + wcslen(string);

    /* strip trailing delim */
    if (to > string) to--;
    if (to > string && *to == string[0])
    {
        *to = L'\0';
        return TRUE;
    }
    WARN("Can't find ending delimiter (%ls)\n", string);
    return FALSE;
}

static RETURN_CODE for_control_execute_from_FILE(CMD_FOR_CONTROL *for_ctrl, FILE *input, CMD_NODE *node)
{
    WCHAR buffer[MAXSTRING];
    int skip_count = for_ctrl->num_lines_to_skip;
    RETURN_CODE return_code = NO_ERROR;

    /* Read line by line until end of file */
    while (return_code != RETURN_CODE_ABORTED && fgetws(buffer, ARRAY_SIZE(buffer), input))
    {
        size_t len;

        if (skip_count)
        {
            TRACE("skipping %d\n", skip_count);
            skip_count--;
            continue;
        }
        len = wcslen(buffer);
        /* Either our buffer isn't large enough to fit a full line, or there's a stray
         * '\0' in the buffer.
         */
        if (!feof(input) && (len == 0 || (buffer[len - 1] != '\n' && buffer[len - 1] != '\r')))
            break;
        while (len && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
            buffer[--len] = L'\0';
        return_code = for_loop_fileset_parse_line(node, for_ctrl->variable_index, buffer,
                                                  for_ctrl->eol, for_ctrl->delims, for_ctrl->tokens);
        buffer[0] = 0;
    }
    return return_code;
}

static RETURN_CODE for_control_execute_fileset(CMD_FOR_CONTROL *for_ctrl, CMD_NODE *node)
{
    RETURN_CODE return_code = NO_ERROR;
    WCHAR set[MAXSTRING];
    WCHAR *args;
    size_t len;
    FILE *input;
    int i;

    wcscpy(set, for_ctrl->set);
    handleExpansion(set, TRUE);

    args = WCMD_skip_leading_spaces(set);
    for (len = wcslen(args); len && (args[len - 1] == L' ' || args[len - 1] == L'\t'); len--)
        args[len - 1] = L'\0';
    if (args[0] == (for_ctrl->use_backq ? L'\'' : L'"') && match_ending_delim(args))
    {
        args++;
        if (!for_ctrl->num_lines_to_skip)
        {
            return_code = for_loop_fileset_parse_line(node, for_ctrl->variable_index, args,
                                                      for_ctrl->eol, for_ctrl->delims, for_ctrl->tokens);
        }
    }
    else if (args[0] == (for_ctrl->use_backq ? L'`' : L'\'') && match_ending_delim(args))
    {
        WCHAR  temp_cmd[MAX_PATH];

        args++;
        wsprintfW(temp_cmd, L"CMD.EXE /C %s", args);
        TRACE("Reading output of '%s'\n", wine_dbgstr_w(temp_cmd));
        input = _wpopen(temp_cmd, L"rt,ccs=unicode");
        if (!input)
        {
            WCMD_print_error();
            WCMD_output_stderr(WCMD_LoadMessage(WCMD_READFAIL), args);
            return errorlevel = ERROR_INVALID_FUNCTION; /* FOR loop aborts at first failure here */
        }
        return_code = for_control_execute_from_FILE(for_ctrl, input, node);
        pclose(input);
    }
    else
    {
        for (i = 0; return_code != RETURN_CODE_ABORTED; i++)
        {
            WCHAR *element = WCMD_parameter(args, i, NULL, TRUE, FALSE);
            if (!element || !*element) break;
            if (element[0] == L'"' && match_ending_delim(element)) element++;
            /* Open the file, read line by line and process */
            TRACE("Reading input to parse from '%s'\n", wine_dbgstr_w(element));
            input = _wfopen(element, L"rt,ccs=unicode");
            if (!input)
            {
                WCMD_print_error();
                WCMD_output_stderr(WCMD_LoadMessage(WCMD_READFAIL), element);
                return errorlevel = ERROR_INVALID_FUNCTION; /* FOR loop aborts at first failure here */
            }
            return_code = for_control_execute_from_FILE(for_ctrl, input, node);
            fclose(input);
        }
    }

    return return_code;
}

static RETURN_CODE for_control_execute_set(CMD_FOR_CONTROL *for_ctrl, const WCHAR *from_dir, size_t ref_len, CMD_NODE *node)
{
    RETURN_CODE return_code = NO_ERROR;
    size_t len;
    WCHAR set[MAXSTRING];
    WCHAR buffer[MAX_PATH];
    int i;

    if (from_dir)
    {
        len = wcslen(from_dir) + 1;
        wcscpy(buffer, from_dir);
        wcscat(buffer, L"\\");
    }
    else
        len = 0;

    wcscpy(set, for_ctrl->set);
    handleExpansion(set, TRUE);
    for (i = 0; return_code != RETURN_CODE_ABORTED; i++)
    {
        WCHAR *element = WCMD_parameter(set, i, NULL, TRUE, FALSE);
        if (!element || !*element) break;
        if (len + wcslen(element) + 1 >= ARRAY_SIZE(buffer)) continue;

        wcscpy(&buffer[len], element);

        TRACE("Doing set element %ls\n", buffer);

        if (wcspbrk(element, L"?*"))
        {
            WIN32_FIND_DATAW fd;
            HANDLE hff = FindFirstFileW(buffer, &fd);
            size_t insert_pos = (wcsrchr(buffer, L'\\') ? wcsrchr(buffer, L'\\') + 1 - buffer : 0);

            if (hff == INVALID_HANDLE_VALUE)
            {
                TRACE("Couldn't FindFirstFile on %ls\n", buffer);
                continue;
            }
            do
            {
                TRACE("Considering %ls\n", fd.cFileName);
                if (!lstrcmpW(fd.cFileName, L"..") || !lstrcmpW(fd.cFileName, L".")) continue;
                if (!(for_ctrl->flags & CMD_FOR_FLAG_TREE_INCLUDE_FILES) &&
                    !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    continue;
                if (!(for_ctrl->flags & CMD_FOR_FLAG_TREE_INCLUDE_DIRECTORIES) &&
                    (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    continue;

                if (insert_pos + wcslen(fd.cFileName) + 1 >= ARRAY_SIZE(buffer)) continue;
                wcscpy(&buffer[insert_pos], fd.cFileName);
                WCMD_set_for_loop_variable(for_ctrl->variable_index, buffer);
                return_code = node_execute(node);
            } while (return_code != RETURN_CODE_ABORTED && FindNextFileW(hff, &fd) != 0);
            FindClose(hff);
        }
        else
        {
            WCMD_set_for_loop_variable(for_ctrl->variable_index, buffer);
            return_code = node_execute(node);
        }
    }
    return return_code;
}

static RETURN_CODE for_control_execute_walk_files(CMD_FOR_CONTROL *for_ctrl, CMD_NODE *node)
{
    DIRECTORY_STACK *dirs_to_walk;
    size_t ref_len;
    RETURN_CODE return_code = NO_ERROR;

    if (for_ctrl->root_dir)
    {
        WCHAR buffer[MAXSTRING];

        wcscpy(buffer, for_ctrl->root_dir);
        handleExpansion(buffer, TRUE);
        dirs_to_walk = WCMD_dir_stack_create(buffer, NULL);
    }
    else dirs_to_walk = WCMD_dir_stack_create(NULL, NULL);
    ref_len = wcslen(dirs_to_walk->dirName);

    while (return_code != RETURN_CODE_ABORTED && dirs_to_walk)
    {
        TRACE("About to walk %p %ls for %s\n", dirs_to_walk, dirs_to_walk->dirName, debugstr_for_control(for_ctrl));
        if (for_ctrl->flags & CMD_FOR_FLAG_TREE_RECURSE)
            WCMD_add_dirstowalk(dirs_to_walk);

        return_code = for_control_execute_set(for_ctrl, dirs_to_walk->dirName, ref_len, node);
        /* If we are walking directories, move on to any which remain */
        dirs_to_walk = WCMD_dir_stack_free(dirs_to_walk);
    }
    TRACE("Finished all directories.\n");

    return return_code;
}

static RETURN_CODE for_control_execute_numbers(CMD_FOR_CONTROL *for_ctrl, CMD_NODE *node)
{
    RETURN_CODE return_code = NO_ERROR;
    WCHAR set[MAXSTRING];
    int numbers[3] = {0, 0, 0}, var;
    int i;

    wcscpy(set, for_ctrl->set);
    handleExpansion(set, TRUE);

    /* Note: native doesn't check the actual number of parameters, and set
     * them by default to 0.
     * so (-10 1) is interpreted as (-10 1 0)
     * and (10) loops for ever !!!
     */
    for (i = 0; i < ARRAY_SIZE(numbers); i++)
    {
        WCHAR *element = WCMD_parameter(set, i, NULL, FALSE, FALSE);
        if (!element || !*element) break;
        /* native doesn't no error handling */
        numbers[i] = wcstol(element, NULL, 0);
    }

    for (var = numbers[0];
         return_code != RETURN_CODE_ABORTED && ((numbers[1] < 0) ? var >= numbers[2] : var <= numbers[2]);
         var += numbers[1])
    {
        WCHAR tmp[32];

        swprintf(tmp, ARRAY_SIZE(tmp), L"%d", var);
        WCMD_set_for_loop_variable(for_ctrl->variable_index, tmp);
        TRACE("Processing FOR number %s\n", wine_dbgstr_w(tmp));
        return_code = node_execute(node);
    }
    return return_code;
}

static RETURN_CODE for_control_execute(CMD_FOR_CONTROL *for_ctrl, CMD_NODE *node)
{
    RETURN_CODE return_code;

    if (!for_ctrl->set) return NO_ERROR;

    WCMD_save_for_loop_context(FALSE);

    switch (for_ctrl->operator)
    {
    case CMD_FOR_FILETREE:
        if (for_ctrl->flags & CMD_FOR_FLAG_TREE_RECURSE)
            return_code = for_control_execute_walk_files(for_ctrl, node);
        else
            return_code = for_control_execute_set(for_ctrl, NULL, 0, node);
        break;
    case CMD_FOR_FILE_SET:
        return_code = for_control_execute_fileset(for_ctrl, node);
        break;
    case CMD_FOR_NUMBERS:
        return_code = for_control_execute_numbers(for_ctrl, node);
        break;
    default:
        return_code = NO_ERROR;
        break;
    }
    WCMD_restore_for_loop_context();
    return return_code;
}

RETURN_CODE node_execute(CMD_NODE *node)
{
    HANDLE old_stdhandles[3] = {GetStdHandle (STD_INPUT_HANDLE),
                                GetStdHandle (STD_OUTPUT_HANDLE),
                                GetStdHandle (STD_ERROR_HANDLE)};
    static DWORD idx_stdhandles[3] = {STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};

    RETURN_CODE return_code;
    int i, test;

    if (!node) return NO_ERROR;
    if (!set_std_redirections(node->redirects))
    {
        WCMD_print_error();
        return_code = ERROR_INVALID_FUNCTION;
    }
    else switch (node->op)
    {
    case CMD_SINGLE:
        if (node->command[0] != ':')
            return_code = execute_single_command(node->command);
        else return_code = NO_ERROR;
        break;
    case CMD_CONCAT:
        return_code = node_execute(node->left);
        if (return_code != RETURN_CODE_ABORTED)
            return_code = node_execute(node->right);
        break;
    case CMD_ONSUCCESS:
        return_code = node_execute(node->left);
        if (return_code == NO_ERROR)
            return_code = node_execute(node->right);
        break;
    case CMD_ONFAILURE:
        return_code = node_execute(node->left);
        if (return_code != NO_ERROR && return_code != RETURN_CODE_ABORTED)
        {
            /* that's needed for commands (POPD, RMDIR) that don't set errorlevel in case of failure. */
            errorlevel = return_code;
            return_code = node_execute(node->right);
        }
        break;
    case CMD_PIPE:
        {
            static SECURITY_ATTRIBUTES sa = {.nLength = sizeof(sa), .lpSecurityDescriptor = NULL, .bInheritHandle = TRUE};
            WCHAR temp_path[MAX_PATH];
            WCHAR filename[MAX_PATH];
            CMD_REDIRECTION *output;
            HANDLE saved_output;
            BATCH_CONTEXT *saved_context = context;

            /* pipe LHS & RHS are run outside of any batch context */
            context = NULL;
            /* FIXME: a real pipe instead of writing to an intermediate file would be
             * better.
             * But waiting for completion of commands will require more work.
             */
            /* FIXME check precedence (eg foo > a | more)
             * with following code, | has higher precedence than > a
             * (which is likely wrong IIRC, and not what previous code was doing)
             */
            /* Generate a unique temporary filename */
            GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
            GetTempFileNameW(temp_path, L"CMD", 0, filename);
            TRACE("Using temporary file of %ls\n", filename);

            saved_output = GetStdHandle(STD_OUTPUT_HANDLE);
            /* set output for left hand side command */
            output = redirection_create_file(REDIR_WRITE_TO, 1, filename);
            if (set_std_redirections(output))
            {
                RETURN_CODE return_code_left = node_execute(node->left);
                CloseHandle(GetStdHandle(STD_OUTPUT_HANDLE));
                SetStdHandle(STD_OUTPUT_HANDLE, saved_output);

                if (errorlevel == RETURN_CODE_CANT_LAUNCH && saved_context)
                    ExitProcess(255);
                return_code = ERROR_INVALID_FUNCTION;
                if (return_code_left != RETURN_CODE_ABORTED && errorlevel != RETURN_CODE_CANT_LAUNCH)
                {
                    HANDLE h = CreateFileW(filename, GENERIC_READ,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL, NULL);
                    if (h != INVALID_HANDLE_VALUE)
                    {
                        SetStdHandle(STD_INPUT_HANDLE, h);
                        return_code = node_execute(node->right);
                        if (errorlevel == RETURN_CODE_CANT_LAUNCH && saved_context)
                            ExitProcess(255);
                    }
                }
                DeleteFileW(filename);
                errorlevel = return_code;
            }
            else return_code = ERROR_INVALID_FUNCTION;
            redirection_dispose_list(output);
            context = saved_context;
        }
        break;
    case CMD_IF:
        if (if_condition_evaluate(&node->condition, &test))
            return_code = node_execute(test ? node->then_block : node->else_block);
        else
            return_code = ERROR_INVALID_FUNCTION;
        break;
    case CMD_FOR:
        return_code = for_control_execute(&node->for_ctrl, node->do_block);
        break;
    default:
        FIXME("Unexpected operator %u\n", node->op);
        return_code = ERROR_INVALID_FUNCTION;
    }
    /* Restore old handles */
    for (i = 0; i < 3; i++)
    {
        if (old_stdhandles[i] != GetStdHandle(idx_stdhandles[i]))
        {
            CloseHandle(GetStdHandle(idx_stdhandles[i]));
            SetStdHandle(idx_stdhandles[i], old_stdhandles[i]);
        }
    }
    return return_code;
}


RETURN_CODE WCMD_ctrlc_status(void)
{
    return (WAIT_OBJECT_0 == WaitForSingleObject(control_c_event, 0)) ? STATUS_CONTROL_C_EXIT : NO_ERROR;
}

static BOOL WINAPI my_event_handler(DWORD ctrl)
{
    WCMD_output(L"\n");
    if (ctrl == CTRL_C_EVENT)
    {
        SetEvent(control_c_event);
    }
    return ctrl == CTRL_C_EVENT;
}


/*****************************************************************************
 * Main entry point. This is a console application so we have a main() not a
 * winmain().
 */

int __cdecl wmain (int argc, WCHAR *argvW[])
{
  WCHAR  *cmdLine = NULL;
  WCHAR  *cmd     = NULL;
  WCHAR string[1024];
  WCHAR envvar[4];
  BOOL opt_q;
  int opt_t = 0;
  WCHAR comspec[MAX_PATH];
  CMD_NODE *toExecute = NULL;         /* Commands left to be executed */
  RTL_OSVERSIONINFOEXW osv;
  char osver[50];
  STARTUPINFOW startupInfo;
  const WCHAR *arg;
  enum read_parse_line rpl_status;

  if (!GetEnvironmentVariableW(L"COMSPEC", comspec, ARRAY_SIZE(comspec)))
  {
      GetSystemDirectoryW(comspec, ARRAY_SIZE(comspec) - ARRAY_SIZE(L"\\cmd.exe"));
      lstrcatW(comspec, L"\\cmd.exe");
      SetEnvironmentVariableW(L"COMSPEC", comspec);
  }

  srand(time(NULL));

  /* Get the windows version being emulated */
  osv.dwOSVersionInfoSize = sizeof(osv);
  RtlGetVersion(&osv);

  /* Pre initialize some messages */
  lstrcpyW(anykey, WCMD_LoadMessage(WCMD_ANYKEY));
  sprintf(osver, "%ld.%ld.%ld", osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);
  cmd = WCMD_format_string(WCMD_LoadMessage(WCMD_VERSION), osver);
  lstrcpyW(version_string, cmd);
  LocalFree(cmd);
  cmd = NULL;

  /* init for loop context */
  forloopcontext = NULL;
  WCMD_save_for_loop_context(TRUE);

  /* Initialize the event here because the command loop at the bottom will
   * reset it unconditionally even if the Control-C handler is not installed.
   */
  control_c_event = CreateEventW(NULL, TRUE, FALSE, NULL);

  /* Can't use argc/argv as it will have stripped quotes from parameters
   * meaning cmd.exe /C echo "quoted string" is impossible
   */
  cmdLine = GetCommandLineW();
  WINE_TRACE("Full commandline '%s'\n", wine_dbgstr_w(cmdLine));

  while (*cmdLine && *cmdLine != '/') ++cmdLine;

  opt_c = opt_k = opt_q = opt_s = FALSE;

  for (arg = cmdLine; *arg; ++arg)
  {
        if (arg[0] != '/')
            continue;

        switch (towlower(arg[1]))
        {
        case 'a':
            unicodeOutput = FALSE;
            break;
        case 'c':
            opt_c = TRUE;
            break;
        case 'k':
            opt_k = TRUE;
            break;
        case 'q':
            opt_q = TRUE;
            break;
        case 's':
            opt_s = TRUE;
            break;
        case 't':
            if (arg[2] == ':')
                opt_t = wcstoul(&arg[3], NULL, 16);
            break;
        case 'u':
            unicodeOutput = TRUE;
            break;
        case 'v':
            if (arg[2] == ':')
                delayedsubst = wcsnicmp(&arg[3], L"OFF", 3);
            break;
        }

        if (opt_c || opt_k)
        {
            arg += 2;
            break;
        }
  }

  while (*arg && wcschr(L" \t,=;", *arg)) arg++;

  if (opt_q) {
    WCMD_echo(L"OFF");
  }

  /* Until we start to read from the keyboard, stay as non-interactive */
  interactive = FALSE;

  SetEnvironmentVariableW(L"PROMPT", L"$P$G");

  if (opt_c || opt_k) {
      int     len;
      WCHAR   *q1 = NULL,*q2 = NULL,*p;

      /* Take a copy */
      cmd = xstrdupW(arg);

      /* opt_s left unflagged if the command starts with and contains exactly
       * one quoted string (exactly two quote characters). The quoted string
       * must be an executable name that has whitespace and must not have the
       * following characters: &<>()@^| */

      if (!opt_s) {
        /* 1. Confirm there is at least one quote */
        q1 = wcschr(arg, '"');
        if (!q1) opt_s=1;
      }

      if (!opt_s) {
          /* 2. Confirm there is a second quote */
          q2 = wcschr(q1+1, '"');
          if (!q2) opt_s=1;
      }

      if (!opt_s) {
          /* 3. Ensure there are no more quotes */
          if (wcschr(q2+1, '"')) opt_s=1;
      }

      /* check first parameter for a space and invalid characters. There must not be any
       * invalid characters, but there must be one or more whitespace                    */
      if (!opt_s) {
          opt_s = TRUE;
          p=q1;
          while (p!=q2) {
              if (*p=='&' || *p=='<' || *p=='>' || *p=='(' || *p==')'
                  || *p=='@' || *p=='^' || *p=='|') {
                  opt_s = TRUE;
                  break;
              }
              if (*p==' ' || *p=='\t')
                  opt_s = FALSE;
              p++;
          }
      }

      WINE_TRACE("/c command line: '%s'\n", wine_dbgstr_w(cmd));

      /* Finally, we only stay in new mode IF the first parameter is quoted and
         is a valid executable, i.e. must exist, otherwise drop back to old mode  */
      if (!opt_s) {
        WCHAR *thisArg = WCMD_parameter(cmd, 0, NULL, FALSE, TRUE);
        WCHAR  pathext[MAXSTRING];
        BOOL found = FALSE;

        /* Now extract PATHEXT */
        len = GetEnvironmentVariableW(L"PATHEXT", pathext, ARRAY_SIZE(pathext));
        if ((len == 0) || (len >= ARRAY_SIZE(pathext))) {
          lstrcpyW(pathext, L".bat;.com;.cmd;.exe");
        }

        /* If the supplied parameter has any directory information, look there */
        WINE_TRACE("First parameter is '%s'\n", wine_dbgstr_w(thisArg));
        if (wcschr(thisArg, '\\') != NULL) {

          if (!WCMD_get_fullpath(thisArg, ARRAY_SIZE(string), string, NULL)) return FALSE;
          WINE_TRACE("Full path name '%s'\n", wine_dbgstr_w(string));
          p = string + lstrlenW(string);

          /* Does file exist with this name? */
          if (GetFileAttributesW(string) != INVALID_FILE_ATTRIBUTES) {
            WINE_TRACE("Found file as '%s'\n", wine_dbgstr_w(string));
            found = TRUE;
          } else {
            WCHAR *thisExt = pathext;

            /* No - try with each of the PATHEXT extensions */
            while (!found && thisExt) {
              WCHAR *nextExt = wcschr(thisExt, ';');

              if (nextExt) {
                memcpy(p, thisExt, (nextExt-thisExt) * sizeof(WCHAR));
                p[(nextExt-thisExt)] = 0x00;
                thisExt = nextExt+1;
              } else {
                lstrcpyW(p, thisExt);
                thisExt = NULL;
              }

              /* Does file exist with this extension appended? */
              if (GetFileAttributesW(string) != INVALID_FILE_ATTRIBUTES) {
                WINE_TRACE("Found file as '%s'\n", wine_dbgstr_w(string));
                found = TRUE;
              }
            }
          }

        /* Otherwise we now need to look in the path to see if we can find it */
        } else {
          /* Does file exist with this name? */
          if (SearchPathW(NULL, thisArg, NULL, ARRAY_SIZE(string), string, NULL) != 0)  {
            WINE_TRACE("Found on path as '%s'\n", wine_dbgstr_w(string));
            found = TRUE;
          } else {
            WCHAR *thisExt = pathext;

            /* No - try with each of the PATHEXT extensions */
            while (!found && thisExt) {
              WCHAR *nextExt = wcschr(thisExt, ';');

              if (nextExt) {
                *nextExt = 0;
                nextExt = nextExt+1;
              } else {
                nextExt = NULL;
              }

              /* Does file exist with this extension? */
              if (SearchPathW(NULL, thisArg, thisExt, ARRAY_SIZE(string), string, NULL) != 0)  {
                WINE_TRACE("Found on path as '%s' with extension '%s'\n", wine_dbgstr_w(string),
                           wine_dbgstr_w(thisExt));
                found = TRUE;
              }
              thisExt = nextExt;
            }
          }
        }

        /* If not found, drop back to old behaviour */
        if (!found) {
          WINE_TRACE("Binary not found, dropping back to old behaviour\n");
          opt_s = TRUE;
        }

      }

      /* strip first and last quote characters if opt_s; check for invalid
       * executable is done later */
      if (opt_s && *cmd=='\"')
          WCMD_strip_quotes(cmd);
  }
  else
  {
      SetConsoleCtrlHandler(my_event_handler, TRUE);
  }

  /* Save cwd into appropriate env var (Must be before the /c processing */
  GetCurrentDirectoryW(ARRAY_SIZE(string), string);
  if (IsCharAlphaW(string[0]) && string[1] == ':') {
    wsprintfW(envvar, L"=%c:", string[0]);
    SetEnvironmentVariableW(envvar, string);
    WINE_TRACE("Set %s to %s\n", wine_dbgstr_w(envvar), wine_dbgstr_w(string));
  }

  if (opt_c) {
      /* If we do a "cmd /c command", we don't want to allocate a new
       * console since the command returns immediately. Rather, we use
       * the currently allocated input and output handles. This allows
       * us to pipe to and read from the command interpreter.
       */

      /* Parse the command string, without reading any more input */
      rpl_status = WCMD_ReadAndParseLine(cmd, &toExecute);
      if (rpl_status == RPL_SUCCESS && toExecute)
      {
          node_execute(toExecute);
          node_dispose_tree(toExecute);
      }
      else if (rpl_status == RPL_SYNTAXERROR)
          errorlevel = RETURN_CODE_SYNTAX_ERROR;

      return errorlevel;
  }

  GetStartupInfoW(&startupInfo);
  if (startupInfo.lpTitle != NULL)
      SetConsoleTitleW(startupInfo.lpTitle);
  else
      SetConsoleTitleW(WCMD_LoadMessage(WCMD_CONSTITLE));

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
      static const WCHAR regKeyW[] = L"Software\\Microsoft\\Command Processor";

      if (RegOpenKeyExW(HKEY_CURRENT_USER, regKeyW,
                       0, KEY_READ, &key) == ERROR_SUCCESS) {
          WCHAR  strvalue[4];

          /* See if DWORD or REG_SZ */
          if (RegQueryValueExW(key, L"DefaultColor", NULL, &type, NULL, NULL) == ERROR_SUCCESS) {
              if (type == REG_DWORD) {
                  size = sizeof(DWORD);
                  RegQueryValueExW(key, L"DefaultColor", NULL, NULL, (BYTE *)&value, &size);
              } else if (type == REG_SZ) {
                  size = sizeof(strvalue);
                  RegQueryValueExW(key, L"DefaultColor", NULL, NULL, (BYTE *)strvalue, &size);
                  value = wcstoul(strvalue, NULL, 10);
              }
          }
          RegCloseKey(key);
      }

      if (value == 0 && RegOpenKeyExW(HKEY_LOCAL_MACHINE, regKeyW,
                       0, KEY_READ, &key) == ERROR_SUCCESS) {
          WCHAR  strvalue[4];

          /* See if DWORD or REG_SZ */
          if (RegQueryValueExW(key, L"DefaultColor", NULL, &type,
                     NULL, NULL) == ERROR_SUCCESS) {
              if (type == REG_DWORD) {
                  size = sizeof(DWORD);
                  RegQueryValueExW(key, L"DefaultColor", NULL, NULL, (BYTE *)&value, &size);
              } else if (type == REG_SZ) {
                  size = sizeof(strvalue);
                  RegQueryValueExW(key, L"DefaultColor", NULL, NULL, (BYTE *)strvalue, &size);
                  value = wcstoul(strvalue, NULL, 10);
              }
          }
          RegCloseKey(key);
      }

      /* If one found, set the screen to that colour */
      if (!(((value & 0xF0) >> 4) == (value & 0x0F))) {
          defaultColor = value & 0xFF;
          param1[0] = 0x00;
          WCMD_color();
      }

  }

  if (opt_k)
  {
      rpl_status = WCMD_ReadAndParseLine(cmd, &toExecute);
      /* Parse the command string, without reading any more input */
      if (rpl_status == RPL_SUCCESS && toExecute)
      {
          node_execute(toExecute);
          node_dispose_tree(toExecute);
      }
      else if (rpl_status == RPL_SYNTAXERROR)
          errorlevel = RETURN_CODE_SYNTAX_ERROR;
      free(cmd);
  }

/*
 *	Loop forever getting commands and executing them.
 */

  interactive = TRUE;
  if (!opt_k) WCMD_output_asis(version_string);
  if (echo_mode) WCMD_output_asis(L"\r\n");
  /* Read until EOF (which for std input is never, but if redirect in place, may occur */
  while ((rpl_status = WCMD_ReadAndParseLine(NULL, &toExecute)) != RPL_EOF)
  {
      if (rpl_status == RPL_SUCCESS && toExecute)
      {
          ResetEvent(control_c_event);
          node_execute(toExecute);
          node_dispose_tree(toExecute);
          if (echo_mode) WCMD_output_asis(L"\r\n");
      }
  }
  CloseHandle(control_c_event);
  return 0;
}
