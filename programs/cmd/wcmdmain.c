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
    if (ReadConsoleW(hIn, intoBuf, maxChars, charsRead, NULL)) return TRUE;

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
static void WCMD_output_asis_handle (DWORD std_handle, const WCHAR *message) {
  DWORD count;
  const WCHAR* ptr;
  WCHAR string[1024];
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
        WCMD_ReadFile(GetStdHandle(STD_INPUT_HANDLE), string, ARRAY_SIZE(string), &count);
      }
    } while (((message = ptr) != NULL) && (*ptr));
  } else {
    WCMD_output_asis_len(message, lstrlenW(message), handle);
  }
}

/*******************************************************************
 * WCMD_output_asis
 *
 * Send output to current standard output device, without formatting
 * e.g. when message contains '%'
 */
void WCMD_output_asis (const WCHAR *message) {
    WCMD_output_asis_handle(STD_OUTPUT_HANDLE, message);
}

/*******************************************************************
 * WCMD_output_asis_stderr
 *
 * Send output to current standard error device, without formatting
 * e.g. when message contains '%'
 */
void WCMD_output_asis_stderr (const WCHAR *message) {
    WCMD_output_asis_handle(STD_ERROR_HANDLE, message);
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

static void WCMD_show_prompt (BOOL newLine) {

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
  if (newLine) {
    *q++ = '\r';
    *q++ = '\n';
  }
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
void WCMD_strsubstW(WCHAR *start, const WCHAR *next, const WCHAR *insert, int len) {

   if (len < 0)
      len=insert ? lstrlenW(insert) : 0;
   if (start+len != next)
       memmove(start+len, next, (lstrlenW(next) + 1) * sizeof(*next));
   if (insert)
       memcpy(start, insert, len * sizeof(*insert));
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
WCHAR *WCMD_skip_leading_spaces (WCHAR *string) {

  WCHAR *ptr;

  ptr = string;
  while (*ptr == ' ' || *ptr == '\t') ptr++;
  return ptr;
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


/*************************************************************************
 * WCMD_is_magic_envvar
 * Return TRUE if s is '%'magicvar'%'
 * and is not masked by a real environment variable.
 */

static inline BOOL WCMD_is_magic_envvar(const WCHAR *s, const WCHAR *magicvar)
{
    int len;

    if (s[0] != '%')
        return FALSE;         /* Didn't begin with % */
    len = lstrlenW(s);
    if (len < 2 || s[len-1] != '%')
        return FALSE;         /* Didn't end with another % */

    if (CompareStringW(LOCALE_USER_DEFAULT,
                       NORM_IGNORECASE | SORT_STRINGSORT,
                       s+1, len-2, magicvar, -1) != CSTR_EQUAL) {
        /* Name doesn't match. */
        return FALSE;
    }

    if (GetEnvironmentVariableW(magicvar, NULL, 0) > 0) {
        /* Masked by real environment variable. */
        return FALSE;
    }

    return TRUE;
}

/*************************************************************************
 * WCMD_expand_envvar
 *
 *	Expands environment variables, allowing for WCHARacter substitution
 */
static WCHAR *WCMD_expand_envvar(WCHAR *start, WCHAR startchar)
{
    WCHAR *endOfVar = NULL, *s;
    WCHAR *colonpos = NULL;
    WCHAR thisVar[MAXSTRING];
    WCHAR thisVarContents[MAXSTRING];
    WCHAR savedchar = 0x00;
    int len;
    WCHAR Delims[] = L"%:"; /* First char gets replaced appropriately */

    WINE_TRACE("Expanding: %s (%c)\n", wine_dbgstr_w(start), startchar);

    /* Find the end of the environment variable, and extract name */
    Delims[0] = startchar;
    endOfVar = wcspbrk(start+1, Delims);

    if (endOfVar == NULL || *endOfVar==' ') {

      /* In batch program, missing terminator for % and no following
         ':' just removes the '%'                                   */
      if (context) {
        WCMD_strsubstW(start, start + 1, NULL, 0);
        return start;
      } else {

        /* In command processing, just ignore it - allows command line
           syntax like: for %i in (a.a) do echo %i                     */
        return start+1;
      }
    }

    /* If ':' found, process remaining up until '%' (or stop at ':' if
       a missing '%' */
    if (*endOfVar==':') {
        WCHAR *endOfVar2 = wcschr(endOfVar+1, startchar);
        if (endOfVar2 != NULL) endOfVar = endOfVar2;
    }

    memcpy(thisVar, start, ((endOfVar - start) + 1) * sizeof(WCHAR));
    thisVar[(endOfVar - start)+1] = 0x00;
    colonpos = wcschr(thisVar+1, ':');

    /* If there's complex substitution, just need %var% for now
       to get the expanded data to play with                    */
    if (colonpos) {
        *colonpos = '%';
        savedchar = *(colonpos+1);
        *(colonpos+1) = 0x00;
    }

    /* By now, we know the variable we want to expand but it may be
       surrounded by '!' if we are in delayed expansion - if so convert
       to % signs.                                                      */
    if (startchar=='!') {
      thisVar[0]                  = '%';
      thisVar[(endOfVar - start)] = '%';
    }
    WINE_TRACE("Retrieving contents of %s\n", wine_dbgstr_w(thisVar));

    /* Expand to contents, if unchanged, return */
    /* Handle DATE, TIME, ERRORLEVEL and CD replacements allowing */
    /* override if existing env var called that name              */
    if (WCMD_is_magic_envvar(thisVar, L"ERRORLEVEL")) {
      len = wsprintfW(thisVarContents, L"%d", errorlevel);
    } else if (WCMD_is_magic_envvar(thisVar, L"DATE")) {
      len = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL,
                           NULL, thisVarContents, ARRAY_SIZE(thisVarContents));
    } else if (WCMD_is_magic_envvar(thisVar, L"TIME")) {
      len = GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL,
                           NULL, thisVarContents, ARRAY_SIZE(thisVarContents));
    } else if (WCMD_is_magic_envvar(thisVar, L"CD")) {
      len = GetCurrentDirectoryW(ARRAY_SIZE(thisVarContents), thisVarContents);
    } else if (WCMD_is_magic_envvar(thisVar, L"RANDOM")) {
      len = wsprintfW(thisVarContents, L"%d", rand() % 32768);
    } else {
      if ((len = ExpandEnvironmentStringsW(thisVar, thisVarContents, ARRAY_SIZE(thisVarContents)))) len--;
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

      /* Command line - just ignore this */
      if (context == NULL) return endOfVar+1;


      /* Batch - replace unknown env var with nothing */
      if (colonpos == NULL) {
        WCMD_strsubstW(start, endOfVar + 1, NULL, 0);
      } else {
        len = lstrlenW(thisVar);
        thisVar[len-1] = 0x00;
        /* If %:...% supplied, : is retained */
        if (colonpos == thisVar+1) {
          WCMD_strsubstW(start, endOfVar + 1, colonpos, -1);
        } else {
          WCMD_strsubstW(start, endOfVar + 1, colonpos + 1, -1);
        }
      }
      return start;

    }

    /* See if we need to do complex substitution (any ':'s), if not
       then our work here is done                                  */
    if (colonpos == NULL) {
      WCMD_strsubstW(start, endOfVar + 1, thisVarContents, -1);
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
      WCHAR *commapos = wcschr(colonpos+2, ',');
      WCHAR *startCopy;

      substrposition = wcstol(colonpos+2, NULL, 10);
      if (commapos) substrlength = wcstol(commapos+1, NULL, 10);

      /* Check bounds */
      if (substrposition >= 0) {
        startCopy = &thisVarContents[min(substrposition, len - 1)];
      } else {
        startCopy = &thisVarContents[max(0, len + substrposition)];
      }

      if (commapos == NULL) {
        /* Copy the lot */
        WCMD_strsubstW(start, endOfVar + 1, startCopy, -1);
      } else if (substrlength < 0) {

        int copybytes = len + substrlength - (startCopy - thisVarContents);
        if (copybytes >= len) copybytes = len - 1;
        else if (copybytes < 0) copybytes = 0;
        WCMD_strsubstW(start, endOfVar + 1, startCopy, copybytes);
      } else {
        substrlength = min(substrlength, len - (startCopy - thisVarContents));
        WCMD_strsubstW(start, endOfVar + 1, startCopy, substrlength);
      }

    /* search and replace manipulation */
    } else {
      WCHAR *equalspos = wcsstr(colonpos, L"=");
      WCHAR *replacewith = equalspos+1;
      WCHAR *found       = NULL;
      WCHAR *searchIn;
      WCHAR *searchFor;

      if (equalspos == NULL) return start+1;
      s = xstrdupW(endOfVar + 1);

      /* Null terminate both strings */
      thisVar[lstrlenW(thisVar)-1] = 0x00;
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
          lstrcatW(start, s);
        } else {
          /* Copy as is */
          lstrcpyW(start, thisVarContents);
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
        lstrcatW(outputposn, s);
      }
      free(s);
      free(searchIn);
      free(searchFor);
    }
    return start;
}

/*****************************************************************************
 * Expand the command. Native expands lines from batch programs as they are
 * read in and not again, except for 'for' variable substitution.
 * eg. As evidence, "echo %1 && shift && echo %1" or "echo %%path%%"
 * atExecute is TRUE when the expansion is occurring as the command is executed
 * rather than at parse time, i.e. delayed expansion and for loops need to be
 * processed
 */
static void handleExpansion(WCHAR *cmd, BOOL atExecute, BOOL delayed) {

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
  WCHAR *delayedp = NULL;
  WCHAR  startchar = '%';
  WCHAR *normalp;

  /* Display the FOR variables in effect */
  for (i=0;i<MAX_FOR_VARIABLES;i++) {
    if (forloopcontext->variable[i]) {
      WINE_TRACE("FOR variable context: %c = '%s'\n",
                 for_var_index_to_char(i),
                 wine_dbgstr_w(forloopcontext->variable[i]));
    }
  }

  /* Find the next environment variable delimiter */
  normalp = wcschr(p, '%');
  if (delayed) delayedp = wcschr(p, '!');
  if (!normalp) p = delayedp;
  else if (!delayedp) p = normalp;
  else p = min(p,delayedp);
  if (p) startchar = *p;

  while (p) {

    WINE_TRACE("Translate command:%s %d (at: %s)\n",
                   wine_dbgstr_w(cmd), atExecute, wine_dbgstr_w(p));
    i = *(p+1) - '0';

    /* Don't touch %% unless it's in Batch */
    if (!atExecute && *(p+1) == startchar) {
      if (context) {
        WCMD_strsubstW(p, p+1, NULL, 0);
      }
      p+=1;

    /* Replace %~ modifications if in batch program */
    } else if (*(p+1) == '~') {
      WCMD_HandleTildeModifiers(&p, atExecute);
      p++;

    /* Replace use of %0...%9 if in batch program*/
    } else if (!atExecute && context && (i >= 0) && (i <= 9) && startchar == '%') {
      t = WCMD_parameter(context -> command, i + context -> shift_count[i],
                         NULL, TRUE, TRUE);
      WCMD_strsubstW(p, p+2, t, -1);

    /* Replace use of %* if in batch program*/
    } else if (!atExecute && context && *(p+1)=='*' && startchar == '%') {
      WCHAR *startOfParms = NULL;
      WCHAR *thisParm = WCMD_parameter(context -> command, 0, &startOfParms, TRUE, TRUE);
      if (startOfParms != NULL) {
        startOfParms += lstrlenW(thisParm);
        while (*startOfParms==' ' || *startOfParms == '\t') startOfParms++;
        WCMD_strsubstW(p, p+2, startOfParms, -1);
      } else
        WCMD_strsubstW(p, p+2, NULL, 0);

    } else {
      int forvaridx = for_var_char_to_index(*(p+1));
      if (startchar == '%' && forvaridx != -1 && forloopcontext->variable[forvaridx]) {
        /* Replace the 2 characters, % and for variable character */
        WCMD_strsubstW(p, p + 2, forloopcontext->variable[forvaridx], -1);
      } else if (!atExecute || startchar == '!') {
        p = WCMD_expand_envvar(p, startchar);

      /* In a FOR loop, see if this is the variable to replace */
      } else { /* Ignore %'s on second pass of batch program */
        p++;
      }
    }

    /* Find the next environment variable delimiter */
    normalp = wcschr(p, '%');
    if (delayed) delayedp = wcschr(p, '!');
    if (!normalp) p = delayedp;
    else if (!delayedp) p = normalp;
    else p = min(p,delayedp);
    if (p) startchar = *p;
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

static void command_dispose(CMD_COMMAND *cmd)
{
    if (cmd)
    {
        free(cmd->command);
        redirection_dispose_list(cmd->redirects);
        free(cmd);
    }
}

void for_control_dispose(CMD_FOR_CONTROL *for_ctrl)
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
    return wine_dbg_sprintf("[FOR] %s %s%s%%%c (%ls)",
                            for_ctrl_strings[for_ctrl->operator], flags, options,
                            for_var_index_to_char(for_ctrl->variable_index), for_ctrl->set);
}

void for_control_create(enum for_control_operator for_op, unsigned flags, const WCHAR *options, int var_idx, CMD_FOR_CONTROL *for_ctrl)
{
    for_ctrl->operator = for_op;
    for_ctrl->flags = flags;
    for_ctrl->variable_index = var_idx;
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

void for_control_create_fileset(unsigned flags, int var_idx, WCHAR eol, int num_lines_to_skip, BOOL use_backq,
                                const WCHAR *delims, const WCHAR *tokens,
                                CMD_FOR_CONTROL *for_ctrl)
{
    for_ctrl->operator = CMD_FOR_FILE_SET;
    for_ctrl->flags = flags;
    for_ctrl->variable_index = var_idx;
    for_ctrl->set = NULL;

    for_ctrl->eol = eol;
    for_ctrl->use_backq = use_backq;
    for_ctrl->num_lines_to_skip = num_lines_to_skip;
    for_ctrl->delims = delims;
    for_ctrl->tokens = tokens;
}

void for_control_append_set(CMD_FOR_CONTROL *for_ctrl, const WCHAR *set)
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

/***************************************************************************
 * node_dispose_tree
 *
 * Frees the storage held for a parsed command line
 * - This is not done in the process_commands, as eventually the current
 *   pointer will be modified within the commands, and hence a single free
 *   routine is simpler
 */
void node_dispose_tree(CMD_NODE *cmds)
{
    /* Loop through the commands, freeing them one by one */
    while (cmds)
    {
        CMD_NODE *thisCmd = cmds;
        cmds = CMD_node_next(cmds);
        if (thisCmd->op == CMD_SINGLE)
            command_dispose(thisCmd->command);
        else
            node_dispose_tree(thisCmd->left);
        free(thisCmd);
    }
}

static CMD_NODE *node_create_single(CMD_COMMAND *c)
{
    CMD_NODE *new = xalloc(sizeof(CMD_NODE));

    new->op = CMD_SINGLE;
    new->command = c;

    return new;
}

static CMD_NODE *node_create_binary(CMD_OPERATOR op, CMD_NODE *l, CMD_NODE *r)
{
    CMD_NODE *new = xalloc(sizeof(CMD_NODE));

    new->op = op;
    new->left = l;
    new->right = r;

    return new;
}

void if_condition_dispose(CMD_IF_CONDITION *cond)
{
    switch (cond->op)
    {
    case CMD_IF_ERRORLEVEL:
        break;
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

BOOL if_condition_create(WCHAR *start, WCHAR **end, CMD_IF_CONDITION *cond)
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
        WCHAR *endptr;
        int level;
        param_copy = WCMD_parameter(start, narg++, &param_start, TRUE, FALSE);
        if (cond) cond->op = CMD_IF_ERRORLEVEL;
        level = wcstol(param_copy, &endptr, 10);
        if (*endptr) return FALSE;
        if (cond) cond->level = level;
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

const char *debugstr_if_condition(const CMD_IF_CONDITION *cond)
{
    const char *header = wine_dbg_sprintf("{{%s%s", cond->negated ? "not " : "", cond->case_insensitive ? "nocase " : "");

    switch (cond->op)
    {
    case CMD_IF_ERRORLEVEL:   return wine_dbg_sprintf("%serrorlevel %d}}", header, cond->level);
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
 *          findexecutable to achieve this which is left untouched.
 *        If an executable has not been found, and we were launched through
 *          a call, we need to check if the command is an internal command,
 *          so go back through wcmd_execute.
 */

void WCMD_run_program (WCHAR *command, BOOL called)
{
  WCHAR  temp[MAX_PATH];
  WCHAR  pathtosearch[MAXSTRING];
  WCHAR *pathposn;
  WCHAR  stemofsearch[MAX_PATH];    /* maximum allowed executable name is
                                       MAX_PATH, including null character */
  WCHAR *lastSlash;
  WCHAR  pathext[MAXSTRING];
  WCHAR *firstParam;
  BOOL  extensionsupplied = FALSE;
  BOOL  explicit_path = FALSE;
  BOOL  status;
  DWORD len;

  /* Quick way to get the filename is to extract the first argument. */
  WINE_TRACE("Running '%s' (%d)\n", wine_dbgstr_w(command), called);
  firstParam = WCMD_parameter(command, 0, NULL, FALSE, TRUE);
  if (!firstParam) return;

  if (!firstParam[0]) {
      errorlevel = 0;
      return;
  }

  /* Calculate the search path and stem to search for */
  if (wcspbrk(firstParam, L"/\\:") == NULL) {  /* No explicit path given, search path */
    lstrcpyW(pathtosearch, L".;");
    len = GetEnvironmentVariableW(L"PATH", &pathtosearch[2], ARRAY_SIZE(pathtosearch)-2);
    if ((len == 0) || (len >= ARRAY_SIZE(pathtosearch) - 2)) {
      lstrcpyW(pathtosearch, L".");
    }
    if (wcschr(firstParam, '.') != NULL) extensionsupplied = TRUE;
    if (lstrlenW(firstParam) >= MAX_PATH)
    {
        WCMD_output_asis_stderr(WCMD_LoadMessage(WCMD_LINETOOLONG));
        return;
    }

    lstrcpyW(stemofsearch, firstParam);

  } else {

    /* Convert eg. ..\fred to include a directory by removing file part */
    if (!WCMD_get_fullpath(firstParam, ARRAY_SIZE(pathtosearch), pathtosearch, NULL)) return;
    lastSlash = wcsrchr(pathtosearch, '\\');
    if (lastSlash && wcschr(lastSlash, '.') != NULL) extensionsupplied = TRUE;
    lstrcpyW(stemofsearch, lastSlash+1);

    /* Reduce pathtosearch to a path with trailing '\' to support c:\a.bat and
       c:\windows\a.bat syntax                                                 */
    if (lastSlash) *(lastSlash + 1) = 0x00;
    explicit_path = TRUE;
  }

  /* Now extract PATHEXT */
  len = GetEnvironmentVariableW(L"PATHEXT", pathext, ARRAY_SIZE(pathext));
  if ((len == 0) || (len >= ARRAY_SIZE(pathext))) {
    lstrcpyW(pathext, L".bat;.com;.cmd;.exe");
  }

  /* Loop through the search path, dir by dir */
  pathposn = pathtosearch;
  WINE_TRACE("Searching in '%s' for '%s'\n", wine_dbgstr_w(pathtosearch),
             wine_dbgstr_w(stemofsearch));
  while (pathposn) {
    WCHAR  thisDir[MAX_PATH] = {'\0'};
    int    length            = 0;
    WCHAR *pos               = NULL;
    BOOL  found             = FALSE;
    BOOL inside_quotes      = FALSE;

    if (explicit_path)
    {
        lstrcpyW(thisDir, pathposn);
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
            memcpy(thisDir, pathposn, (pos-pathposn) * sizeof(WCHAR));
            thisDir[(pos-pathposn)] = 0x00;
            pathposn = pos+1;
        }
        else       /* Reached string end */
        {
            lstrcpyW(thisDir, pathposn);
            pathposn = NULL;
        }

        /* Remove quotes */
        length = lstrlenW(thisDir);
        if (thisDir[length - 1] == '"')
            thisDir[length - 1] = 0;

        if (*thisDir != '"')
            lstrcpyW(temp, thisDir);
        else
            lstrcpyW(temp, thisDir + 1);

        /* When temp is an empty string, skip over it. This needs
           to be done before the expansion, because WCMD_get_fullpath
           fails when given an empty string                         */
        if (*temp == '\0')
            continue;

        /* Since you can have eg. ..\.. on the path, need to expand
           to full information                                      */
        if (!WCMD_get_fullpath(temp, ARRAY_SIZE(thisDir), thisDir, NULL)) return;
    }

    /* 1. If extension supplied, see if that file exists */
    lstrcatW(thisDir, L"\\");
    lstrcatW(thisDir, stemofsearch);
    pos = &thisDir[lstrlenW(thisDir)]; /* Pos = end of name */

    /* 1. If extension supplied, see if that file exists */
    if (extensionsupplied) {
      if (GetFileAttributesW(thisDir) != INVALID_FILE_ATTRIBUTES) {
        found = TRUE;
      }
    }

    /* 2. Any .* matches? */
    if (!found) {
      HANDLE          h;
      WIN32_FIND_DATAW finddata;

      lstrcatW(thisDir, L".*");
      h = FindFirstFileW(thisDir, &finddata);
      FindClose(h);
      if (h != INVALID_HANDLE_VALUE) {

        WCHAR *thisExt = pathext;

        /* 3. Yes - Try each path ext */
        while (thisExt) {
          WCHAR *nextExt = wcschr(thisExt, ';');

          if (nextExt) {
            memcpy(pos, thisExt, (nextExt-thisExt) * sizeof(WCHAR));
            pos[(nextExt-thisExt)] = 0x00;
            thisExt = nextExt+1;
          } else {
            lstrcpyW(pos, thisExt);
            thisExt = NULL;
          }

          if (GetFileAttributesW(thisDir) != INVALID_FILE_ATTRIBUTES) {
            found = TRUE;
            thisExt = NULL;
          }
        }
      }
    }

    /* Once found, launch it */
    if (found) {
      STARTUPINFOW st;
      PROCESS_INFORMATION pe;
      SHFILEINFOW psfi;
      DWORD console;
      HINSTANCE hinst;
      WCHAR *ext = wcsrchr( thisDir, '.' );

      WINE_TRACE("Found as %s\n", wine_dbgstr_w(thisDir));

      /* Special case BAT and CMD */
      if (ext && (!wcsicmp(ext, L".bat") || !wcsicmp(ext, L".cmd"))) {
        BOOL oldinteractive = interactive;
        interactive = FALSE;
        WCMD_batch (thisDir, command, called, NULL, INVALID_HANDLE_VALUE);
        interactive = oldinteractive;
        return;
      } else {
        DWORD exit_code;
        /* thisDir contains the file to be launched, but with what?
           eg. a.exe will require a.exe to be launched, a.html may be iexplore */
        hinst = FindExecutableW (thisDir, NULL, temp);
        if ((INT_PTR)hinst < 32)
          console = 0;
        else
          console = SHGetFileInfoW(temp, 0, &psfi, sizeof(psfi), SHGFI_EXETYPE);

        ZeroMemory (&st, sizeof(STARTUPINFOW));
        st.cb = sizeof(STARTUPINFOW);
        init_msvcrt_io_block(&st);

        /* Launch the process and if a CUI wait on it to complete
           Note: Launching internal wine processes cannot specify a full path to exe */
        status = CreateProcessW(thisDir,
                                command, NULL, NULL, TRUE, 0, NULL, NULL, &st, &pe);
        free(st.lpReserved2);
        if ((opt_c || opt_k) && !opt_s && !status
            && GetLastError()==ERROR_FILE_NOT_FOUND && command[0]=='\"') {
          /* strip first and last quote WCHARacters and try again */
          WCMD_strip_quotes(command);
          opt_s = TRUE;
          WCMD_run_program(command, called);
          return;
        }

        if (!status)
          break;

        /* Always wait when non-interactive (cmd /c or in batch program),
           or for console applications                                    */
        if (!interactive || (console && !HIWORD(console)))
            WaitForSingleObject (pe.hProcess, INFINITE);
        GetExitCodeProcess (pe.hProcess, &exit_code);
        errorlevel = (exit_code == STILL_ACTIVE) ? 0 : exit_code;

        CloseHandle(pe.hProcess);
        CloseHandle(pe.hThread);
        return;
      }
    }
  }

  /* Not found anywhere - were we called? */
  if (called) {
    CMD_NODE *toExecute = NULL;         /* Commands left to be executed */

    /* Parse the command string, without reading any more input */
    WCMD_ReadAndParseLine(command, &toExecute, INVALID_HANDLE_VALUE);
    WCMD_process_commands(toExecute, FALSE, called);
    node_dispose_tree(toExecute);
    toExecute = NULL;
    return;
  }

  /* Not found anywhere - give up */
  WCMD_output_stderr(WCMD_LoadMessage(WCMD_NO_COMMAND_FOUND), command);

  /* If a command fails to launch, it sets errorlevel 9009 - which
     does not seem to have any associated constant definition     */
  errorlevel = 9009;
  return;

}

/* this is obviously wrong... will require more work to be fixed */
static inline unsigned clamp_fd(unsigned fd)
{
    return fd <= 2 ? fd : 1;
}

static BOOL set_std_redirections(CMD_REDIRECTION *redir, WCHAR *in_pipe)
{
    static DWORD std_index[3] = {STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};
    static SECURITY_ATTRIBUTES sa = {.nLength = sizeof(sa), .lpSecurityDescriptor = NULL, .bInheritHandle = TRUE};
    WCHAR expanded_filename[MAXSTRING];
    HANDLE h;

    /* STDIN could come from a preceding pipe, so delete on close if it does */
    if (in_pipe)
    {
        TRACE("Input coming from %s\n", wine_dbgstr_w(in_pipe));
        h = CreateFileW(in_pipe, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
        if (h == INVALID_HANDLE_VALUE) return FALSE;
        SetStdHandle(STD_INPUT_HANDLE, h);
        /* Otherwise STDIN could come from a '<' redirect */
    }
    for (; redir; redir = redir->next)
    {
        switch (redir->kind)
        {
        case REDIR_READ_FROM:
            if (in_pipe) continue; /* give precedence to pipe */
            wcscpy(expanded_filename, redir->file);
            handleExpansion(expanded_filename, context != NULL, delayedsubst);
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
                handleExpansion(expanded_filename, context != NULL, delayedsubst);
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
            if (!DuplicateHandle(GetCurrentProcess(),
                                 GetStdHandle(std_index[clamp_fd(redir->clone)]),
                                 GetCurrentProcess(),
                                 &h,
                                 0, TRUE, DUPLICATE_SAME_ACCESS))
            {
                WARN("Duplicating handle failed with gle %ld\n", GetLastError());
            }
            break;
        }
        SetStdHandle(std_index[clamp_fd(redir->fd)], h);
    }
    return TRUE;
}

/*****************************************************************************
 * Process one command. If the command is EXIT this routine does not return.
 * We will recurse through here executing batch files.
 * Note: If call is used to a non-existing program, we reparse the line and
 *       try to run it as an internal command. 'retrycall' represents whether
 *       we are attempting this retry.
 */
static void execute_single_command(const WCHAR *command, CMD_NODE **cmdList, BOOL retrycall)
{
    WCHAR *cmd, *parms_start;
    int status, cmd_index, count;
    WCHAR *whichcmd;
    WCHAR *new_cmd = NULL;
    BOOL prev_echo_mode;

    TRACE("command on entry:%s (%p)\n", wine_dbgstr_w(command), cmdList);

    /* Move copy of the command onto the heap so it can be expanded */
    new_cmd = xalloc(MAXSTRING * sizeof(WCHAR));
    lstrcpyW(new_cmd, command);
    cmd = new_cmd;

    /* Strip leading whitespaces, and a '@' if supplied */
    whichcmd = WCMD_skip_leading_spaces(cmd);
    TRACE("Command: '%s'\n", wine_dbgstr_w(cmd));
    if (whichcmd[0] == '@') whichcmd++;

    /* Check if the command entered is internal, and identify which one */
    count = 0;
    while (IsCharAlphaNumericW(whichcmd[count])) {
      count++;
    }
    for (cmd_index=0; cmd_index<=WCMD_EXIT; cmd_index++) {
      if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
        whichcmd, count, inbuilt[cmd_index], -1) == CSTR_EQUAL) break;
    }
    parms_start = WCMD_skip_leading_spaces (&whichcmd[count]);

    handleExpansion(new_cmd, (context != NULL), delayedsubst);

/*
 * Changing default drive has to be handled as a special case, anything
 * else if it exists after whitespace is ignored
 */

    if ((cmd[1] == ':') && IsCharAlphaW(cmd[0]) &&
        (!cmd[2] || cmd[2] == ' ' || cmd[2] == '\t')) {
      WCHAR envvar[5];
      WCHAR dir[MAX_PATH];

      /* Ignore potential garbage on the same line */
      cmd[2]=0x00;

      /* According to MSDN CreateProcess docs, special env vars record
         the current directory on each drive, in the form =C:
         so see if one specified, and if so go back to it             */
      lstrcpyW(envvar, L"=");
      lstrcatW(envvar, cmd);
      if (GetEnvironmentVariableW(envvar, dir, MAX_PATH) == 0) {
        wsprintfW(cmd, L"%s\\", cmd);
        WINE_TRACE("No special directory settings, using dir of %s\n", wine_dbgstr_w(cmd));
      }
      WINE_TRACE("Got directory %s as %s\n", wine_dbgstr_w(envvar), wine_dbgstr_w(cmd));
      status = SetCurrentDirectoryW(cmd);
      if (!status) WCMD_print_error ();
      goto cleanup;
    }

    WCMD_parse (parms_start, quals, param1, param2);
    TRACE("param1: %s, param2: %s\n", wine_dbgstr_w(param1), wine_dbgstr_w(param2));

    if (cmd_index <= WCMD_EXIT && (parms_start[0] == '/') && (parms_start[1] == '?')) {
      /* this is a help request for a builtin program */
      cmd_index = WCMD_HELP;
      memcpy(parms_start, whichcmd, count * sizeof(WCHAR));
      parms_start[count] = '\0';

    }

    switch (cmd_index) {

      case WCMD_CALL:
        WCMD_call (parms_start);
        break;
      case WCMD_CD:
      case WCMD_CHDIR:
        WCMD_setshow_default (parms_start);
        break;
      case WCMD_CLS:
        WCMD_clear_screen ();
        break;
      case WCMD_COPY:
        WCMD_copy (parms_start);
        break;
      case WCMD_CTTY:
        WCMD_change_tty ();
        break;
      case WCMD_DATE:
        WCMD_setshow_date ();
	break;
      case WCMD_DEL:
      case WCMD_ERASE:
        WCMD_delete (parms_start);
        break;
      case WCMD_DIR:
        WCMD_directory (parms_start);
        break;
      case WCMD_ECHO:
        WCMD_echo(&whichcmd[count]);
        break;
      case WCMD_GOTO:
        WCMD_goto (cmdList);
        break;
      case WCMD_HELP:
        WCMD_give_help (parms_start);
        break;
      case WCMD_LABEL:
        WCMD_volume (TRUE, parms_start);
        break;
      case WCMD_MD:
      case WCMD_MKDIR:
        WCMD_create_dir (parms_start);
        break;
      case WCMD_MOVE:
        WCMD_move ();
        break;
      case WCMD_PATH:
        WCMD_setshow_path (parms_start);
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
        WCMD_remove_dir (parms_start);
        break;
      case WCMD_SETLOCAL:
        WCMD_setlocal(parms_start);
        break;
      case WCMD_ENDLOCAL:
        WCMD_endlocal();
        break;
      case WCMD_SET:
        WCMD_setshow_env (parms_start);
        break;
      case WCMD_SHIFT:
        WCMD_shift (parms_start);
        break;
      case WCMD_START:
        WCMD_start (parms_start);
        break;
      case WCMD_TIME:
        WCMD_setshow_time ();
        break;
      case WCMD_TITLE:
        if (lstrlenW(&whichcmd[count]) > 0)
          WCMD_title(&whichcmd[count+1]);
        break;
      case WCMD_TYPE:
        WCMD_type (parms_start);
        break;
      case WCMD_VER:
        WCMD_output_asis(L"\r\n");
        WCMD_version ();
        break;
      case WCMD_VERIFY:
        WCMD_verify (parms_start);
        break;
      case WCMD_VOL:
        WCMD_volume (FALSE, parms_start);
        break;
      case WCMD_PUSHD:
        WCMD_pushd(parms_start);
        break;
      case WCMD_POPD:
        WCMD_popd();
        break;
      case WCMD_ASSOC:
        WCMD_assoc(parms_start, TRUE);
        break;
      case WCMD_COLOR:
        WCMD_color();
        break;
      case WCMD_FTYPE:
        WCMD_assoc(parms_start, FALSE);
        break;
      case WCMD_MORE:
        WCMD_more(parms_start);
        break;
      case WCMD_CHOICE:
        WCMD_choice(parms_start);
        break;
      case WCMD_MKLINK:
        WCMD_mklink(parms_start);
        break;
      case WCMD_EXIT:
        WCMD_exit (cmdList);
        break;
      case WCMD_FOR:
      case WCMD_IF:
        /* Very oddly, probably because of all the special parsing required for
           these two commands, neither 'for' nor 'if' is supported when called,
           i.e. 'call if 1==1...' will fail.                                    */
        if (!retrycall) {
          if (cmd_index==WCMD_FOR) WCMD_for (parms_start, cmdList);
          else if (cmd_index==WCMD_IF) WCMD_if (parms_start, cmdList);
          break;
        }
        /* else: drop through */
      default:
        prev_echo_mode = echo_mode;
        WCMD_run_program (whichcmd, FALSE);
        echo_mode = prev_echo_mode;
    }
cleanup:
    free(cmd);
}

/*****************************************************************************
 * Process one command. If the command is EXIT this routine does not return.
 * We will recurse through here executing batch files.
 * Note: If call is used to a non-existing program, we reparse the line and
 *       try to run it as an internal command. 'retrycall' represents whether
 *       we are attempting this retry.
 */
void WCMD_execute(const WCHAR *command, CMD_REDIRECTION *redirects,
                  CMD_NODE **cmdList, BOOL retrycall)
{
    WCHAR *cmd;
    int i, cmd_index, count;
    WCHAR *whichcmd;
    WCHAR *new_cmd = NULL;
    HANDLE old_stdhandles[3] = {GetStdHandle (STD_INPUT_HANDLE),
                                GetStdHandle (STD_OUTPUT_HANDLE),
                                GetStdHandle (STD_ERROR_HANDLE)};
    static DWORD idx_stdhandles[3] = {STD_INPUT_HANDLE, STD_OUTPUT_HANDLE, STD_ERROR_HANDLE};
    CMD_REDIRECTION *piped_redir = redirects;

    TRACE("command on entry:%s (%p)\n", wine_dbgstr_w(command), cmdList);

    /* Move copy of the command onto the heap so it can be expanded */
    new_cmd = xalloc(MAXSTRING * sizeof(WCHAR));
    lstrcpyW(new_cmd, command);
    cmd = new_cmd;

    /* Strip leading whitespaces, and a '@' if supplied */
    whichcmd = WCMD_skip_leading_spaces(cmd);
    TRACE("Command: '%s'\n", wine_dbgstr_w(cmd));
    if (whichcmd[0] == '@') whichcmd++;

    /* Check if the command entered is internal, and identify which one */
    count = 0;
    while (IsCharAlphaNumericW(whichcmd[count])) {
      count++;
    }
    for (cmd_index=0; cmd_index<=WCMD_EXIT; cmd_index++) {
      if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
        whichcmd, count, inbuilt[cmd_index], -1) == CSTR_EQUAL) break;
    }

    /* If the next command is a pipe then we implement pipes by redirecting
       the output from this command to a temp file and input into the
       next command from that temp file.
       Note: Do not do this for a for or if statement as the pipe is for
       the individual statements, not the for or if itself.
       FIXME: Use of named pipes would make more sense here as currently this
       process has to finish before the next one can start but this requires
       a change to not wait for the first app to finish but rather the pipe  */
    /* FIXME this is wrong: we need to discriminate between redirection in individual
     * commands in the blocks, vs redirection of the whole command.
     */
    if (!(cmd_index == WCMD_FOR || cmd_index == WCMD_IF) &&
        cmdList && (*cmdList)->op == CMD_PIPE)
    {
        const WCHAR *to;
        WCHAR temp_path[MAX_PATH];

        /* Remember piping is in action */
        TRACE("Output needs to be piped\n");

        /* Generate a unique temporary filename */
        GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
        GetTempFileNameW(temp_path, L"CMD", 0, CMD_node_get_command(CMD_node_next(*cmdList))->pipeFile);
        TRACE("Using temporary file of %s\n",
              wine_dbgstr_w(CMD_node_get_command(CMD_node_next(*cmdList))->pipeFile));
        /* send stdout to the pipe by appending >filename to redirects */
        to = CMD_node_get_command(CMD_node_next(*cmdList))->pipeFile;
        piped_redir = redirection_create_file(REDIR_WRITE_TO, 1, to);
        piped_redir->next = redirects;
    }

    /*
     *  Redirect stdin, stdout and/or stderr if required.
     *  Note: Do not do this for a for or if statement as the pipe is for
     *  the individual statements, not the for or if itself.
     */
    /* FIXME this is wrong (see above) */
    if (!(cmd_index == WCMD_FOR || cmd_index == WCMD_IF)) {
      WCHAR *in_pipe = NULL;
      if (cmdList && CMD_node_get_command(*cmdList)->pipeFile[0] != 0x00)
        in_pipe = CMD_node_get_command(*cmdList)->pipeFile;
      if (!set_std_redirections(piped_redir, in_pipe)) {
        WCMD_print_error ();
        goto cleanup;
      }
      if (in_pipe)
        /* No need to remember the temporary name any longer once opened */
        in_pipe[0] = 0x00;
    } else {
      TRACE("Not touching redirects for a FOR or IF command\n");
    }
    execute_single_command(command, cmdList, retrycall);
cleanup:
    free(cmd);
    if (piped_redir != redirects) free(piped_redir);

    /* Restore old handles */
    for (i = 0; i < 3; i++)
    {
        if (old_stdhandles[i] != GetStdHandle(idx_stdhandles[i]))
        {
            CloseHandle(GetStdHandle (idx_stdhandles[i]));
            SetStdHandle(idx_stdhandles[i], old_stdhandles[i]);
        }
    }
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
 * WCMD_addCommand
 *
 *   Adds a command to the current command list
 */
static CMD_COMMAND *WCMD_createCommand(WCHAR *command, int *commandLen,
                                       WCHAR *redirs,  int *redirLen,
                                       WCHAR **copyTo, int **copyToLen,
                                       int curDepth)
{
    CMD_COMMAND *thisEntry = NULL;

    /* Allocate storage for command */
    thisEntry = xalloc(sizeof(CMD_COMMAND));

    /* Copy in the command */
    if (command) {
        WCHAR *pos;
        WCHAR *last = redirs + *redirLen;
        CMD_REDIRECTION **insrt;

        thisEntry->command = xalloc((*commandLen + 1) * sizeof(WCHAR));
        memcpy(thisEntry->command, command, *commandLen * sizeof(WCHAR));
        thisEntry->command[*commandLen] = 0x00;

        if (redirs) redirs[*redirLen] = 0;
        /* Create redirects, keeping order (eg "2>foo 1>&2") */
        insrt = &thisEntry->redirects;
        *insrt = NULL;
        for (pos = redirs; pos; insrt = &(*insrt)->next)
        {
            WCHAR *p = find_chr(pos, last, L"<>");
            WCHAR *filename;

            if (!p) break;

            if (*p == L'<')
            {
                filename = WCMD_parameter(p + 1, 0, NULL, FALSE, FALSE);
                handleExpansion(filename, context != NULL, FALSE);
                *insrt = redirection_create_file(REDIR_READ_FROM, 0, filename);
            }
            else
            {
                unsigned fd = 1;
                unsigned op = REDIR_WRITE_TO;

                if (p > redirs && p[-1] >= L'2' && p[-1] <= L'9') fd = p[-1] - L'0';
                if (*++p == L'>') {p++; op = REDIR_WRITE_APPEND;}
                if (*p == L'&' && (p[1] >= L'0' && p[1] <= L'9'))
                {
                    *insrt = redirection_create_clone(fd, p[1] - '0');
                    p++;
                }
                else
                {
                    filename = WCMD_parameter(p, 0, NULL, FALSE, FALSE);
                    handleExpansion(filename, context != NULL, FALSE);
                    *insrt = redirection_create_file(op, fd, filename);
                }
            }
            pos = p + 1;
        }

        /* Reset the lengths */
        *commandLen   = 0;
        *redirLen     = 0;
        *copyToLen    = commandLen;
        *copyTo       = command;

    } else {
        thisEntry->command = NULL;
        thisEntry->redirects = NULL;
    }

    /* Fill in other fields */
    thisEntry->pipeFile[0] = 0x00;
    thisEntry->bracketDepth = curDepth;
    return thisEntry;
}

static void WCMD_appendCommand(CMD_OPERATOR op, CMD_COMMAND *command, CMD_NODE **node)
{
    /* append as left to right operators */
    if (*node)
    {
        CMD_NODE **last = node;
        while ((*last)->op != CMD_SINGLE)
            last = &(*last)->right;

        *last = node_create_binary(op, *last, node_create_single(command));
    }
    else
    {
        *node = node_create_single(command);
    }
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

CMD_FOR_CONTROL *for_control_parse(WCHAR *opts_var)
{
    CMD_FOR_CONTROL *for_ctrl;
    enum for_control_operator for_op;
    WCHAR mode = L' ', option;
    WCHAR options[MAXSTRING];
    WCHAR *arg;
    unsigned flags = 0;
    int arg_index;
    int var_idx;

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
    if (!arg || *arg != L'%' || (var_idx = for_var_char_to_index(arg[1])) == -1)
        goto syntax_error; /* FIXME native prints the offending token "%<whatever>" was unexpected at this time */
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
        for ( ; *p; p = end)
        {
            p = WCMD_skip_leading_spaces(p);
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
        for_control_create_fileset(flags, var_idx, eol, num_lines_to_skip, use_backq,
                                   delims ? delims : xstrdupW(L" \t"),
                                   tokens ? tokens : xstrdupW(L"1"), for_ctrl);
    }
    else
        for_control_create(for_op, flags, options, var_idx, for_ctrl);
    return for_ctrl;
syntax_error:
    WCMD_output_stderr(WCMD_LoadMessage(WCMD_SYNTAXERR));
    return NULL;
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
WCHAR *WCMD_ReadAndParseLine(const WCHAR *optionalcmd, CMD_NODE **output, HANDLE readFrom)
{
    WCHAR    *curPos;
    int       inQuotes = 0;
    WCHAR     curString[MAXSTRING];
    int       curStringLen = 0;
    WCHAR     curRedirs[MAXSTRING];
    int       curRedirsLen = 0;
    WCHAR    *curCopyTo;
    int      *curLen;
    int       curDepth = 0;
    CMD_COMMAND *single_cmd = NULL;
    CMD_OPERATOR cmd_op = CMD_CONCAT;
    static WCHAR    *extraSpace = NULL;  /* Deliberately never freed */
    BOOL      inOneLine = FALSE;
    BOOL      inFor = FALSE;
    BOOL      inIn  = FALSE;
    BOOL      inIf  = FALSE;
    BOOL      inElse= FALSE;
    BOOL      onlyWhiteSpace = FALSE;
    BOOL      lastWasWhiteSpace = FALSE;
    BOOL      lastWasDo   = FALSE;
    BOOL      lastWasIn   = FALSE;
    BOOL      lastWasElse = FALSE;
    BOOL      lastWasRedirect = TRUE;
    BOOL      lastWasCaret = FALSE;
    BOOL      ignoreBracket = FALSE;         /* Some expressions after if (set) require */
                                             /* handling brackets as a normal character */
    int       lineCurDepth;                  /* Bracket depth when line was read in */
    BOOL      resetAtEndOfLine = FALSE;      /* Do we need to reset curdepth at EOL */

    *output = NULL;
    /* Allocate working space for a command read from keyboard, file etc */
    if (!extraSpace)
        extraSpace = xalloc((MAXSTRING + 1) * sizeof(WCHAR));
    if (!extraSpace)
    {
        WINE_ERR("Could not allocate memory for extraSpace\n");
        return NULL;
    }

    *output = NULL;
    /* If initial command read in, use that, otherwise get input from handle */
    if (optionalcmd != NULL) {
        lstrcpyW(extraSpace, optionalcmd);
    } else if (readFrom == INVALID_HANDLE_VALUE) {
        WINE_FIXME("No command nor handle supplied\n");
    } else {
        if (!WCMD_fgets(extraSpace, MAXSTRING, readFrom))
          return NULL;
    }
    curPos = extraSpace;
    TRACE("About to parse line (%ls)\n", extraSpace);

    /* Handle truncated input - issue warning */
    if (lstrlenW(extraSpace) == MAXSTRING -1) {
        WCMD_output_asis_stderr(WCMD_LoadMessage(WCMD_TRUNCATEDLINE));
        WCMD_output_asis_stderr(extraSpace);
        WCMD_output_asis_stderr(L"\r\n");
    }

    /* Replace env vars if in a batch context */
    if (context) handleExpansion(extraSpace, FALSE, FALSE);

    /* Skip preceding whitespace */
    while (*curPos == ' ' || *curPos == '\t') curPos++;

    /* Show prompt before batch line IF echo is on and in batch program */
    if (context && echo_mode && *curPos && (*curPos != '@')) {
      const DWORD len = lstrlenW(L"echo.");
      DWORD curr_size = lstrlenW(curPos);
      DWORD min_len = (curr_size < len ? curr_size : len);
      WCMD_show_prompt(TRUE);
      WCMD_output_asis(curPos);
      /* I don't know why Windows puts a space here but it does */
      /* Except for lines starting with 'echo.', 'echo:' or 'echo/'. Ask MS why */
      if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                         curPos, min_len, L"echo.", len) != CSTR_EQUAL
          && CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                         curPos, min_len, L"echo:", len) != CSTR_EQUAL
          && CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                         curPos, min_len, L"echo/", len) != CSTR_EQUAL)
      {
          WCMD_output_asis(L" ");
      }
      WCMD_output_asis(L"\r\n");
    }

    /* Skip repeated 'no echo' characters */
    while (*curPos == '@') curPos++;

    /* Start with an empty string, copying to the command string */
    curStringLen = 0;
    curRedirsLen = 0;
    curCopyTo    = curString;
    curLen       = &curStringLen;
    lastWasRedirect = FALSE;  /* Required e.g. for spaces between > and filename */
    lineCurDepth = curDepth;  /* What was the curdepth at the beginning of the line */

    /* Parse every character on the line being processed */
    while (*curPos != 0x00) {

      WCHAR thisChar;

      /* Debugging AID:
      WINE_TRACE("Looking at '%c' (len:%d, lws:%d, ows:%d)\n", *curPos, *curLen,
                 lastWasWhiteSpace, onlyWhiteSpace);
      */

      /* Prevent overflow caused by the caret escape char */
      if (*curLen >= MAXSTRING) {
        WINE_ERR("Overflow detected in command\n");
        return NULL;
      }

      /* Certain commands need special handling */
      if (curStringLen == 0 && curCopyTo == curString) {
        /* If command starts with 'rem ' or identifies a label, ignore any &&, ( etc. */
        if (WCMD_keyword_ws_found(L"rem", curPos) || *curPos == ':') {
          inOneLine = TRUE;

        } else if (WCMD_keyword_ws_found(L"for", curPos)) {
          inFor = TRUE;

        /* If command starts with 'if ' or 'else ', handle ('s mid line. We should ensure this
           is only true in the command portion of the IF statement, but this
           should suffice for now.
           To be able to handle ('s in the condition part take as much as evaluate_if_condition
           would take and skip parsing it here. */
        } else if (WCMD_keyword_ws_found(L"if", curPos)) {
          WCHAR *p, *command;

          inIf = TRUE;

          p = WCMD_skip_leading_spaces(curPos + 2); /* "if" */
          if (if_condition_create(p, &command, NULL))
          {
              int if_condition_len = command - curPos;
              WINE_TRACE("p: %s, quals: %s, param1: %s, param2: %s, command: %s, if_condition_len: %d\n",
                         wine_dbgstr_w(p), wine_dbgstr_w(quals), wine_dbgstr_w(param1),
                         wine_dbgstr_w(param2), wine_dbgstr_w(command), if_condition_len);
              memcpy(&curCopyTo[*curLen], curPos, if_condition_len*sizeof(WCHAR));
              (*curLen)+=if_condition_len;
              curPos+=if_condition_len;
          }
          if (WCMD_keyword_ws_found(L"set", curPos))
              ignoreBracket = TRUE;

        } else if (WCMD_keyword_ws_found(L"else", curPos)) {
          const int keyw_len = lstrlenW(L"else") + 1;
          inElse = TRUE;
          lastWasElse = TRUE;
          onlyWhiteSpace = TRUE;
          memcpy(&curCopyTo[*curLen], curPos, keyw_len*sizeof(WCHAR));
          (*curLen)+=keyw_len;
          curPos+=keyw_len;

          /* If we had a single line if XXX which reaches an else (needs odd
             syntax like if 1=1 command && (command) else command we pretended
             to add brackets for the if, so they are now over                  */
          if (resetAtEndOfLine) {
            WINE_TRACE("Resetting curdepth at end of line to %d\n", lineCurDepth);
            resetAtEndOfLine = FALSE;
            curDepth = lineCurDepth;
          }
          continue;

        /* In a for loop, the DO command will follow a close bracket followed by
           whitespace, followed by DO, ie closeBracket inserts a NULL entry, curLen
           is then 0, and all whitespace is skipped                                */
        } else if (inFor && WCMD_keyword_ws_found(L"do", curPos)) {
          const int keyw_len = lstrlenW(L"do") + 1;
          WINE_TRACE("Found 'DO '\n");
          lastWasDo = TRUE;
          onlyWhiteSpace = TRUE;
          memcpy(&curCopyTo[*curLen], curPos, keyw_len*sizeof(WCHAR));
          (*curLen)+=keyw_len;
          curPos+=keyw_len;
          continue;
        }
      } else if (curCopyTo == curString) {

        /* Special handling for the 'FOR' command */
        if (inFor && lastWasWhiteSpace) {
          WINE_TRACE("Found 'FOR ', comparing next parm: '%s'\n", wine_dbgstr_w(curPos));

          if (WCMD_keyword_ws_found(L"in", curPos)) {
            const int keyw_len = lstrlenW(L"in") + 1;
            WINE_TRACE("Found 'IN '\n");
            lastWasIn = TRUE;
            onlyWhiteSpace = TRUE;
            memcpy(&curCopyTo[*curLen], curPos, keyw_len*sizeof(WCHAR));
            (*curLen)+=keyw_len;
            curPos+=keyw_len;
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

      lastWasWhiteSpace = FALSE; /* Will be reset below */
      lastWasCaret = FALSE;

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

                /* Remember just processed whitespace */
                lastWasWhiteSpace = TRUE;

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
                if (thisChar == '>' && *(curPos+1) == '&') {
                    curCopyTo[(*curLen)++] = *(curPos+1);
                    curPos++;
                }
                break;

      case '|': /* Pipe character only if not || */
                if (!inQuotes) {
                  lastWasRedirect = FALSE;

                  /* Add an entry to the command list */
                  if (curStringLen > 0) {

                    /* Add the current command */
                    single_cmd = WCMD_createCommand(curString, &curStringLen,
                                                    curRedirs, &curRedirsLen,
                                                    &curCopyTo, &curLen,
                                                    curDepth);
                    WCMD_appendCommand(cmd_op, single_cmd, output);
                  }

                  if (*(curPos+1) == '|') {
                    curPos++; /* Skip other | */
                    cmd_op = CMD_ONFAILURE;
                  } else {
                    cmd_op = CMD_PIPE;
                  }

                  /* If in an IF or ELSE statement, put subsequent chained
                     commands at a higher depth as if brackets were supplied
                     but remember to reset to the original depth at EOL      */
                  if ((inIf || inElse) && curDepth == lineCurDepth) {
                    curDepth++;
                    resetAtEndOfLine = TRUE;
                  }
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
                WINE_TRACE("Found '(' conditions: curLen(%d), inQ(%d), onlyWS(%d)"
                           ", for(%d, In:%d, Do:%d)"
                           ", if(%d, else:%d, lwe:%d)\n",
                           *curLen, inQuotes,
                           onlyWhiteSpace,
                           inFor, lastWasIn, lastWasDo,
                           inIf, inElse, lastWasElse);
                lastWasRedirect = FALSE;

                /* Ignore open brackets inside the for set */
                if (*curLen == 0 && !inIn) {
                  curDepth++;

                /* If in quotes, ignore brackets */
                } else if (inQuotes) {
                  curCopyTo[(*curLen)++] = *curPos;

                /* In a FOR loop, an unquoted '(' may occur straight after
                      IN or DO
                   In an IF statement just handle it regardless as we don't
                      parse the operands
                   In an ELSE statement, only allow it straight away after
                      the ELSE and whitespace
                 */
                } else if ((inIf && !ignoreBracket) ||
                           (inElse && lastWasElse && onlyWhiteSpace) ||
                           (inFor && (lastWasIn || lastWasDo) && onlyWhiteSpace)) {

                   /* If entering into an 'IN', set inIn */
                  if (inFor && lastWasIn && onlyWhiteSpace) {
                    WINE_TRACE("Inside an IN\n");
                    inIn = TRUE;
                  }

                  /* Add the current command */
                  single_cmd = WCMD_createCommand(curString, &curStringLen,
                                                  curRedirs, &curRedirsLen,
                                                  &curCopyTo, &curLen,
                                                  curDepth);
                  WCMD_appendCommand(cmd_op, single_cmd, output);

                  curDepth++;
                } else {
                  curCopyTo[(*curLen)++] = *curPos;
                }
                break;

      case '^': if (!inQuotes) {
                  /* If we reach the end of the input, we need to wait for more */
                  if (*(curPos+1) == 0x00) {
                    lastWasCaret = TRUE;
                    WINE_TRACE("Caret found at end of line\n");
                    break;
                  }
                  curPos++;
                }
                curCopyTo[(*curLen)++] = *curPos;
                break;

      case '&': if (!inQuotes) {
                  lastWasRedirect = FALSE;

                  /* Add an entry to the command list */
                  if (curStringLen > 0) {

                    /* Add the current command */
                    single_cmd = WCMD_createCommand(curString, &curStringLen,
                                                    curRedirs, &curRedirsLen,
                                                    &curCopyTo, &curLen,
                                                    curDepth);
                    WCMD_appendCommand(cmd_op, single_cmd, output);
                  }

                  if (*(curPos+1) == '&') {
                    curPos++; /* Skip other & */
                    cmd_op = CMD_ONSUCCESS;
                  } else {
                    cmd_op = CMD_CONCAT;
                  }
                  /* If in an IF or ELSE statement, put subsequent chained
                     commands at a higher depth as if brackets were supplied
                     but remember to reset to the original depth at EOL      */
                  if ((inIf || inElse) && curDepth == lineCurDepth) {
                    curDepth++;
                    resetAtEndOfLine = TRUE;
                  }
                } else {
                  curCopyTo[(*curLen)++] = *curPos;
                }
                break;

      case ')': if (!inQuotes && curDepth > 0) {
                  lastWasRedirect = FALSE;

                  /* Add the current command if there is one */
                  if (curStringLen) {

                      /* Add the current command */
                      single_cmd = WCMD_createCommand(curString, &curStringLen,
                                                      curRedirs, &curRedirsLen,
                                                      &curCopyTo, &curLen,
                                                      curDepth);
                      WCMD_appendCommand(cmd_op, single_cmd, output);
                  }

                  /* Add an empty entry to the command list */
                  cmd_op = CMD_CONCAT;
                  single_cmd = WCMD_createCommand(NULL, &curStringLen,
                                                  curRedirs, &curRedirsLen,
                                                  &curCopyTo, &curLen,
                                                  curDepth);
                  WCMD_appendCommand(cmd_op, single_cmd, output);

                  curDepth--;

                  /* Leave inIn if necessary */
                  if (inIn) inIn =  FALSE;
                } else {
                  curCopyTo[(*curLen)++] = *curPos;
                }
                break;
      default:
                lastWasRedirect = FALSE;
                curCopyTo[(*curLen)++] = *curPos;
      }

      curPos++;

      /* At various times we need to know if we have only skipped whitespace,
         so reset this variable and then it will remain true until a non
         whitespace is found                                               */
      if ((thisChar != ' ') && (thisChar != '\t') && (thisChar != '\n'))
        onlyWhiteSpace = FALSE;

      /* Flag end of interest in FOR DO and IN parms once something has been processed */
      if (!lastWasWhiteSpace) {
        lastWasIn = lastWasDo = FALSE;
      }

      /* If we have reached the end, add this command into the list
         Do not add command to list if escape char ^ was last */
      if (*curPos == 0x00 && !lastWasCaret && *curLen > 0) {

          /* Add an entry to the command list */
          single_cmd = WCMD_createCommand(curString, &curStringLen,
                                          curRedirs, &curRedirsLen,
                                          &curCopyTo, &curLen,
                                          curDepth);
          WCMD_appendCommand(cmd_op, single_cmd, output);

          /* If we had a single line if or else, and we pretended to add
             brackets, end them now                                      */
          if (resetAtEndOfLine) {
            WINE_TRACE("Resetting curdepth at end of line to %d\n", lineCurDepth);
            resetAtEndOfLine = FALSE;
            curDepth = lineCurDepth;
          }
        }

      /* If we have reached the end of the string, see if bracketing or
         final caret is outstanding */
      if (*curPos == 0x00 && (curDepth > 0 || lastWasCaret) &&
          readFrom != INVALID_HANDLE_VALUE) {
        WCHAR *extraData;

        WINE_TRACE("Need to read more data as outstanding brackets or carets\n");
        inOneLine = FALSE;
        ignoreBracket = FALSE;
        cmd_op = CMD_CONCAT;
        inQuotes = 0;
        memset(extraSpace, 0x00, (MAXSTRING+1) * sizeof(WCHAR));
        extraData = extraSpace;

        /* Read more, skipping any blank lines */
        do {
          WINE_TRACE("Read more input\n");
          if (!context) WCMD_output_asis( WCMD_LoadMessage(WCMD_MOREPROMPT));
          if (!WCMD_fgets(extraData, MAXSTRING, readFrom))
            break;

          /* Edge case for carets - a completely blank line (i.e. was just
             CRLF) is oddly added as an LF but then more data is received (but
             only once more!) */
          if (lastWasCaret) {
            if (*extraSpace == 0x00) {
              WINE_TRACE("Read nothing, so appending LF char and will try again\n");
              *extraData++ = '\r';
              *extraData = 0x00;
            } else break;
          }

          extraData = WCMD_skip_leading_spaces(extraData);
        } while (*extraData == 0x00);
        curPos = extraSpace;

        /* Skip preceding whitespace */
        while (*curPos == ' ' || *curPos == '\t') curPos++;

        /* Replace env vars if in a batch context */
        if (context) handleExpansion(curPos, FALSE, FALSE);

        /* Continue to echo commands IF echo is on and in batch program */
        if (context && echo_mode && *curPos && *curPos != '@') {
          WCMD_output_asis(extraSpace);
          WCMD_output_asis(L"\r\n");
        }

        /* Skip repeated 'no echo' characters and whitespace */
        while (*curPos == '@' || *curPos == ' ' || *curPos == '\t') curPos++;
      }
    }

    if (curDepth > lineCurDepth) {
        WINE_TRACE("Brackets do not match, error out without executing.\n");
        node_dispose_tree(*output);
        *output = NULL;
        errorlevel = 255;
    }

    return extraSpace;
}

BOOL if_condition_evaluate(CMD_IF_CONDITION *cond, int *test)
{
    int (WINAPI *cmp)(const WCHAR*, const WCHAR*) = cond->case_insensitive ? lstrcmpiW : lstrcmpW;

    TRACE("About to evaluate condition %s\n", debugstr_if_condition(cond));
    *test = 0;
    switch (cond->op)
    {
    case CMD_IF_ERRORLEVEL:
        *test = errorlevel >= cond->level;
        break;
    case CMD_IF_EXIST:
        {
            WIN32_FIND_DATAW fd;
            HANDLE hff;
            size_t len = wcslen(cond->operand);

            if (len)
            {
                /* FindFirstFile does not like a directory path ending in '\' or '/', so append a '.' */
                if (cond->operand[len - 1] == '\\' || cond->operand[len - 1] == '/')
                {
                    WCHAR *new = xrealloc((void*)cond->operand, (wcslen(cond->operand) + 2) * sizeof(WCHAR));
                    wcscat(new, L".");
                    cond->operand = new;
                }
                hff = FindFirstFileW(cond->operand, &fd);
                *test = (hff != INVALID_HANDLE_VALUE);
                if (*test) FindClose(hff);
            }
        }
        break;
    case CMD_IF_DEFINED:
        *test = GetEnvironmentVariableW(cond->operand, NULL, 0) > 0;
        break;
    case CMD_IF_BINOP_EQUAL:
        /* == is a special case, as it always compares strings */
        *test = (*cmp)(cond->left, cond->right) == 0;
        break;
    default:
        {
            int left_int, right_int;
            WCHAR *end_left, *end_right;
            int cmp_val;

            /* Check if we have plain integers (in decimal, octal or hexadecimal notation) */
            left_int = wcstol(cond->left, &end_left, 0);
            right_int = wcstol(cond->right, &end_right, 0);
            if (end_left > cond->left && !*end_left && end_right > cond->right && !*end_right)
                cmp_val = left_int - right_int;
            else
                cmp_val = (*cmp)(cond->left, cond->right);
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

static CMD_NODE *for_loop_fileset_parse_line(CMD_NODE *cmdList, int varidx, WCHAR *buffer,
                                             WCHAR forf_eol, const WCHAR *forf_delims, const WCHAR *forf_tokens)
{
    WCHAR *parm;
    int varoffset;
    int nexttoken, lasttoken = -1;
    BOOL starfound = FALSE;
    BOOL thisduplicate = FALSE;
    BOOL anyduplicates = FALSE;
    int  totalfound;
    static WCHAR emptyW[] = L"";

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

    TRACE("Using var=%lc on %d max\n", for_var_index_to_char(varidx), totalfound);
    /* Empty out variables */
    for (varoffset = 0;
         varoffset < totalfound && for_var_index_in_range(varidx, varoffset);
         varoffset++)
        WCMD_set_for_loop_variable(varidx + varoffset, emptyW);

    /* Loop extracting the tokens
     * Note: nexttoken of 0 means there were no tokens requested, to handle
     * the special case of tokens=*
     */
    varoffset = 0;
    TRACE("Parsing buffer into tokens: '%s'\n", wine_dbgstr_w(buffer));
    while (nexttoken > 0 && (nexttoken > lasttoken))
    {
        anyduplicates |= thisduplicate;

        if (!for_var_index_in_range(varidx, varoffset))
        {
            WARN("Out of range offset\n");
            break;
        }
        /* Extract the token number requested and set into the next variable context */
        parm = WCMD_parameter_with_delims(buffer, (nexttoken-1), NULL, TRUE, FALSE, forf_delims);
        TRACE("Parsed token %d(%d) as parameter %s\n", nexttoken,
              varidx + varoffset, wine_dbgstr_w(parm));
        if (parm)
        {
            WCMD_set_for_loop_variable(varidx + varoffset, parm);
            varoffset++;
        }

        /* Find the next token */
        lasttoken = nexttoken;
        nexttoken = WCMD_for_nexttoken(lasttoken, forf_tokens, NULL,
                                       &starfound, &thisduplicate);
    }
    /* If all the rest of the tokens were requested, and there is still space in
     * the variable range, write them now
     */
    if (!anyduplicates && starfound && for_var_index_in_range(varidx, varoffset))
    {
        nexttoken++;
        WCMD_parameter_with_delims(buffer, (nexttoken-1), &parm, FALSE, FALSE, forf_delims);
        TRACE("Parsed all remaining tokens (%d) as parameter %s\n",
              varidx + varoffset, wine_dbgstr_w(parm));
        if (parm)
            WCMD_set_for_loop_variable(varidx + varoffset, parm);
    }

    /* Execute the body of the for loop with these values */
    if (forloopcontext->variable[varidx] && forloopcontext->variable[varidx][0] != forf_eol)
    {
        /* +3 for "do " */
        WCMD_part_execute(&cmdList, CMD_node_get_command(cmdList)->command + 3, FALSE, TRUE);
    }
    else
    {
        TRACE("Skipping line because of eol\n");
        cmdList = NULL;
    }
    return cmdList;
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
    int varidx;
    if (!old)
    {
        FIXME("Unexpected situation\n");
        return;
    }
    for (varidx = 0; varidx < MAX_FOR_VARIABLES; varidx++)
    {
        if (forloopcontext->variable[varidx] != old->variable[varidx])
            free(forloopcontext->variable[varidx]);
    }
    free(forloopcontext);
    forloopcontext = old;
}

void WCMD_set_for_loop_variable(int var_idx, const WCHAR *value)
{
    if (var_idx < 0 || var_idx >= MAX_FOR_VARIABLES) return;
    if (forloopcontext->previous &&
        forloopcontext->previous->variable[var_idx] != forloopcontext->variable[var_idx])
        free(forloopcontext->variable[var_idx]);
    forloopcontext->variable[var_idx] = xstrdupW(value);
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

static CMD_NODE *for_control_execute_from_FILE(CMD_FOR_CONTROL *for_ctrl, FILE *input, CMD_NODE *cmdList)
{
    WCHAR buffer[MAXSTRING];
    int skip_count = for_ctrl->num_lines_to_skip;
    CMD_NODE *body = NULL;

    /* Read line by line until end of file */
    while (fgetws(buffer, ARRAY_SIZE(buffer), input))
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
        body = for_loop_fileset_parse_line(cmdList, for_ctrl->variable_index, buffer,
                                           for_ctrl->eol, for_ctrl->delims, for_ctrl->tokens);
        buffer[0] = 0;
    }
    return body;
}

static CMD_NODE *for_control_execute_fileset(CMD_FOR_CONTROL *for_ctrl, CMD_NODE *cmdList)
{
    WCHAR set[MAXSTRING];
    WCHAR *args;
    size_t len;
    CMD_NODE *body = NULL;
    FILE *input;
    int i;

    wcscpy(set, for_ctrl->set);
    handleExpansion(set, context != NULL, delayedsubst);

    args = WCMD_skip_leading_spaces(set);
    for (len = wcslen(args); len && (args[len - 1] == L' ' || args[len - 1] == L'\t'); len--)
        args[len - 1] = L'\0';
    if (args[0] == (for_ctrl->use_backq ? L'\'' : L'"') && match_ending_delim(args))
    {
        args++;
        if (!for_ctrl->num_lines_to_skip)
        {
            body = for_loop_fileset_parse_line(cmdList, for_ctrl->variable_index, args,
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
            errorlevel = 1;
            return NULL; /* FOR loop aborts at first failure here */
        }
        body = for_control_execute_from_FILE(for_ctrl, input, cmdList);
        fclose(input);
    }
    else
    {
        for (i = 0; ; i++)
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
                errorlevel = 1;
                return NULL; /* FOR loop aborts at first failure here */
            }
            body = for_control_execute_from_FILE(for_ctrl, input, cmdList);
            fclose(input);
        }
    }

    return body;
}

static CMD_NODE *for_control_execute_set(CMD_FOR_CONTROL *for_ctrl, const WCHAR *from_dir, size_t ref_len, CMD_NODE *cmdList)
{
    CMD_NODE *body = NULL;
    size_t len;
    WCHAR set[MAXSTRING];
    WCHAR buffer[MAX_PATH];
    int i;

    if (from_dir)
    {
        len = wcslen(from_dir) + 1;
        if (len >= ARRAY_SIZE(buffer)) return NULL;
        wcscpy(buffer, from_dir);
        wcscat(buffer, L"\\");
    }
    else
        len = 0;

    wcscpy(set, for_ctrl->set);
    for (i = 0; ; i++)
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

                body = cmdList;
                WCMD_set_for_loop_variable(for_ctrl->variable_index, buffer);
                WCMD_part_execute(&body, CMD_node_get_command(body)->command + 3, FALSE, TRUE);
            } while (FindNextFileW(hff, &fd) != 0);
            FindClose(hff);
        }
        else
        {
            body = cmdList;
            WCMD_set_for_loop_variable(for_ctrl->variable_index, buffer);
            WCMD_part_execute(&body, CMD_node_get_command(body)->command + 3, FALSE, TRUE);
        }
    }
    return body;
}

static CMD_NODE *for_control_execute_walk_files(CMD_FOR_CONTROL *for_ctrl, CMD_NODE *cmdList)
{
    DIRECTORY_STACK *dirs_to_walk;
    size_t ref_len;
    CMD_NODE *body = NULL;

    if (for_ctrl->root_dir)
    {
        dirs_to_walk = WCMD_dir_stack_create(for_ctrl->root_dir, NULL);
    }
    else dirs_to_walk = WCMD_dir_stack_create(NULL, NULL);
    ref_len = wcslen(dirs_to_walk->dirName);

    while (dirs_to_walk)
    {
        TRACE("About to walk %p %ls for %s\n", dirs_to_walk, dirs_to_walk->dirName, debugstr_for_control(for_ctrl));
        if (for_ctrl->flags & CMD_FOR_FLAG_TREE_RECURSE)
            WCMD_add_dirstowalk(dirs_to_walk);

        body = for_control_execute_set(for_ctrl, dirs_to_walk->dirName, ref_len, cmdList);
        /* If we are walking directories, move on to any which remain */
        dirs_to_walk = WCMD_dir_stack_free(dirs_to_walk);
    }
    TRACE("Finished all directories.\n");

    return body;
}

static CMD_NODE *for_control_execute_numbers(CMD_FOR_CONTROL *for_ctrl, CMD_NODE *cmdList)
{
    WCHAR set[MAXSTRING];
    CMD_NODE *body = NULL;
    int numbers[3] = {0, 0, 0}, var;
    int i;

    wcscpy(set, for_ctrl->set);
    handleExpansion(set, context != NULL, delayedsubst);

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
         (numbers[1] < 0) ? var >= numbers[2] : var <= numbers[2];
         var += numbers[1])
    {
        WCHAR tmp[32];

        body = cmdList;
        swprintf(tmp, ARRAY_SIZE(tmp), L"%d", var);
        WCMD_set_for_loop_variable(for_ctrl->variable_index, tmp);
        TRACE("Processing FOR number %s\n", wine_dbgstr_w(tmp));
        WCMD_part_execute(&body, CMD_node_get_command(cmdList)->command + 3, FALSE, TRUE);
    }
    return body;
}

void for_control_execute(CMD_FOR_CONTROL *for_ctrl, CMD_NODE **cmdList)
{
    CMD_NODE *last;
    WCMD_save_for_loop_context(FALSE);

    switch (for_ctrl->operator)
    {
    case CMD_FOR_FILETREE:
        if (for_ctrl->flags & CMD_FOR_FLAG_TREE_RECURSE)
            last = for_control_execute_walk_files(for_ctrl, *cmdList);
        else
            last = for_control_execute_set(for_ctrl, NULL, 0, *cmdList);
        break;
    case CMD_FOR_FILE_SET:
        last = for_control_execute_fileset(for_ctrl, *cmdList);
        break;
    case CMD_FOR_NUMBERS:
        last = for_control_execute_numbers(for_ctrl, *cmdList);
        break;
    default:
        last = NULL;
        break;
    }
    WCMD_restore_for_loop_context();
    *cmdList = last;
}

/***************************************************************************
 * WCMD_process_commands
 *
 * Process all the commands read in so far
 */
CMD_NODE *WCMD_process_commands(CMD_NODE *thisCmd, BOOL oneBracket,
                                BOOL retrycall) {

    int bdepth = -1;

    if (thisCmd && oneBracket) bdepth = CMD_node_get_depth(thisCmd);

    /* Loop through the commands, processing them one by one */
    while (thisCmd) {

      CMD_NODE *origCmd = thisCmd;

      /* If processing one bracket only, and we find the end bracket
         entry (or less), return                                    */
      if (oneBracket && !CMD_node_get_command(thisCmd)->command &&
          bdepth <= CMD_node_get_depth(thisCmd)) {
        WINE_TRACE("Finished bracket @ %p, next command is %p\n",
                   thisCmd, CMD_node_next(thisCmd));
        return CMD_node_next(thisCmd);
      }

      /* Ignore the NULL entries a ')' inserts (Only 'if' cares
         about them and it will be handled in there)
         Also, skip over any batch labels (eg. :fred)          */
      if (CMD_node_get_command(thisCmd)->command && CMD_node_get_command(thisCmd)->command[0] != ':') {
        WINE_TRACE("Executing command: '%s'\n", wine_dbgstr_w(CMD_node_get_command(thisCmd)->command));
        WCMD_execute(CMD_node_get_command(thisCmd)->command, CMD_node_get_command(thisCmd)->redirects, &thisCmd, retrycall);
      }

      /* Step on unless the command itself already stepped on */
      if (thisCmd == origCmd) thisCmd = CMD_node_next(thisCmd);
    }
    return NULL;
}

static BOOL WINAPI my_event_handler(DWORD ctrl)
{
    WCMD_output(L"\n");
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
  BOOL promptNewLine = TRUE;
  BOOL opt_q;
  int opt_t = 0;
  WCHAR comspec[MAX_PATH];
  CMD_NODE *toExecute = NULL;         /* Commands left to be executed */
  RTL_OSVERSIONINFOEXW osv;
  char osver[50];
  STARTUPINFOW startupInfo;
  const WCHAR *arg;

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
      WCMD_ReadAndParseLine(cmd, &toExecute, INVALID_HANDLE_VALUE);
      WCMD_process_commands(toExecute, FALSE, FALSE);
      node_dispose_tree(toExecute);
      toExecute = NULL;

      free(cmd);
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

  if (opt_k) {
      /* Parse the command string, without reading any more input */
      WCMD_ReadAndParseLine(cmd, &toExecute, INVALID_HANDLE_VALUE);
      WCMD_process_commands(toExecute, FALSE, FALSE);
      node_dispose_tree(toExecute);
      toExecute = NULL;
      free(cmd);
  }

/*
 *	Loop forever getting commands and executing them.
 */

  interactive = TRUE;
  if (!opt_k) WCMD_version ();
  while (TRUE) {

    /* Read until EOF (which for std input is never, but if redirect
       in place, may occur                                          */
    if (echo_mode) WCMD_show_prompt(promptNewLine);
    if (!WCMD_ReadAndParseLine(NULL, &toExecute, GetStdHandle(STD_INPUT_HANDLE)))
      break;
    WCMD_process_commands(toExecute, FALSE, FALSE);
    node_dispose_tree(toExecute);
    promptNewLine = !!toExecute;
    toExecute = NULL;
  }
  return 0;
}
