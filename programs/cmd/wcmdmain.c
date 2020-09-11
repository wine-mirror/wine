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
DWORD errorlevel;
WCHAR quals[MAXSTRING], param1[MAXSTRING], param2[MAXSTRING];
BOOL  interactive;
FOR_CONTEXT forloopcontext; /* The 'for' loop context */
BOOL delayedsubst = FALSE; /* The current delayed substitution setting */

int defaultColor = 7;
BOOL echo_mode = TRUE;

WCHAR anykey[100], version_string[100];
const WCHAR newlineW[] = {'\r','\n','\0'};
const WCHAR spaceW[]   = {' ','\0'};
static const WCHAR envPathExt[] = {'P','A','T','H','E','X','T','\0'};
static const WCHAR dfltPathExt[] = {'.','b','a','t',';',
                                    '.','c','o','m',';',
                                    '.','c','m','d',';',
                                    '.','e','x','e','\0'};

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
        output_bufA = heap_xalloc(MAX_WRITECONSOLE_SIZE);
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

  __ms_va_list ap;
  WCHAR* string;
  DWORD len;

  __ms_va_start(ap,format);
  string = NULL;
  len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       format, 0, 0, (LPWSTR)&string, 0, &ap);
  __ms_va_end(ap);
  if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
    WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(format));
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

  __ms_va_list ap;
  WCHAR* string;
  DWORD len;

  __ms_va_start(ap,format);
  string = NULL;
  len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       format, 0, 0, (LPWSTR)&string, 0, &ap);
  __ms_va_end(ap);
  if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
    WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(format));
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
  __ms_va_list ap;
  WCHAR* string;
  DWORD len;

  __ms_va_start(ap,format);
  len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                       format, 0, 0, (LPWSTR)&string, 0, &ap);
  __ms_va_end(ap);
  if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE) {
    WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(format));
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
 * WCMD_Readfile
 *
 *	Read characters in from a console/file, returning result in Unicode
 */
BOOL WCMD_ReadFile(const HANDLE hIn, WCHAR *intoBuf, const DWORD maxChars, LPDWORD charsRead)
{
    DWORD numRead;
    char *buffer;

    if (WCMD_is_console_handle(hIn))
        /* Try to read from console as Unicode */
        return ReadConsoleW(hIn, intoBuf, maxChars, charsRead, NULL);

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
    WINE_FIXME ("Cannot display message for error %d, status %d\n",
			error_code, GetLastError());
    return;
  }

  WCMD_output_asis_len(lpMsgBuf, lstrlenW(lpMsgBuf),
                       GetStdHandle(STD_ERROR_HANDLE));
  LocalFree (lpMsgBuf);
  WCMD_output_asis_len (newlineW, lstrlenW(newlineW),
                        GetStdHandle(STD_ERROR_HANDLE));
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
  static const WCHAR envPrompt[] = {'P','R','O','M','P','T','\0'};

  len = GetEnvironmentVariableW(envPrompt, prompt_string, ARRAY_SIZE(prompt_string));
  if ((len == 0) || (len >= ARRAY_SIZE(prompt_string))) {
    static const WCHAR dfltPrompt[] = {'$','P','$','G','\0'};
    lstrcpyW (prompt_string, dfltPrompt);
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

void *heap_xalloc(size_t size)
{
    void *ret;

    ret = heap_alloc(size);
    if(!ret) {
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
BOOL WCMD_keyword_ws_found(const WCHAR *keyword, int len, const WCHAR *ptr) {
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

    static const WCHAR ErrorLvl[]  = {'E','R','R','O','R','L','E','V','E','L','\0'};
    static const WCHAR Date[]      = {'D','A','T','E','\0'};
    static const WCHAR Time[]      = {'T','I','M','E','\0'};
    static const WCHAR Cd[]        = {'C','D','\0'};
    static const WCHAR Random[]    = {'R','A','N','D','O','M','\0'};
    WCHAR Delims[]    = {'%',':','\0'}; /* First char gets replaced appropriately */

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
        *colonpos = startchar;
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
    if (WCMD_is_magic_envvar(thisVar, ErrorLvl)) {
      static const WCHAR fmt[] = {'%','d','\0'};
      wsprintfW(thisVarContents, fmt, errorlevel);
      len = lstrlenW(thisVarContents);
    } else if (WCMD_is_magic_envvar(thisVar, Date)) {
      GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL,
                    NULL, thisVarContents, MAXSTRING);
      len = lstrlenW(thisVarContents);
    } else if (WCMD_is_magic_envvar(thisVar, Time)) {
      GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL,
                        NULL, thisVarContents, MAXSTRING);
      len = lstrlenW(thisVarContents);
    } else if (WCMD_is_magic_envvar(thisVar, Cd)) {
      GetCurrentDirectoryW(MAXSTRING, thisVarContents);
      len = lstrlenW(thisVarContents);
    } else if (WCMD_is_magic_envvar(thisVar, Random)) {
      static const WCHAR fmt[] = {'%','d','\0'};
      wsprintfW(thisVarContents, fmt, rand() % 32768);
      len = lstrlenW(thisVarContents);
    } else {

      len = ExpandEnvironmentStringsW(thisVar, thisVarContents, ARRAY_SIZE(thisVarContents));
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
        startCopy = &thisVarContents[min(substrposition, len)];
      } else {
        startCopy = &thisVarContents[max(0, len+substrposition-1)];
      }

      if (commapos == NULL) {
        /* Copy the lot */
        WCMD_strsubstW(start, endOfVar + 1, startCopy, -1);
      } else if (substrlength < 0) {

        int copybytes = (len+substrlength-1)-(startCopy-thisVarContents);
        if (copybytes > len) copybytes = len;
        else if (copybytes < 0) copybytes = 0;
        WCMD_strsubstW(start, endOfVar + 1, startCopy, copybytes);
      } else {
        substrlength = min(substrlength, len - (startCopy- thisVarContents + 1));
        WCMD_strsubstW(start, endOfVar + 1, startCopy, substrlength);
      }

    /* search and replace manipulation */
    } else {
      WCHAR *equalspos = wcsstr(colonpos, equalW);
      WCHAR *replacewith = equalspos+1;
      WCHAR *found       = NULL;
      WCHAR *searchIn;
      WCHAR *searchFor;

      if (equalspos == NULL) return start+1;
      s = heap_strdupW(endOfVar + 1);

      /* Null terminate both strings */
      thisVar[lstrlenW(thisVar)-1] = 0x00;
      *equalspos = 0x00;

      /* Since we need to be case insensitive, copy the 2 buffers */
      searchIn  = heap_strdupW(thisVarContents);
      CharUpperBuffW(searchIn, lstrlenW(thisVarContents));
      searchFor = heap_strdupW(colonpos+1);
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
      heap_free(s);
      heap_free(searchIn);
      heap_free(searchFor);
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
  for (i=0;i<52;i++) {
    if (forloopcontext.variable[i]) {
      WINE_TRACE("FOR variable context: %c = '%s'\n",
                 i<26?i+'a':(i-26)+'A',
                 wine_dbgstr_w(forloopcontext.variable[i]));
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
      int forvaridx = FOR_VAR_IDX(*(p+1));
      if (startchar == '%' && forvaridx != -1 && forloopcontext.variable[forvaridx]) {
        /* Replace the 2 characters, % and for variable character */
        WCMD_strsubstW(p, p + 2, forloopcontext.variable[forvaridx], -1);
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
        ptr = heap_xalloc(sz);
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
  static const WCHAR envPath[] = {'P','A','T','H','\0'};
  static const WCHAR delims[] = {'/','\\',':','\0'};

  /* Quick way to get the filename is to extract the first argument. */
  WINE_TRACE("Running '%s' (%d)\n", wine_dbgstr_w(command), called);
  firstParam = WCMD_parameter(command, 0, NULL, FALSE, TRUE);
  if (!firstParam) return;

  /* Calculate the search path and stem to search for */
  if (wcspbrk (firstParam, delims) == NULL) {  /* No explicit path given, search path */
    static const WCHAR curDir[] = {'.',';','\0'};
    lstrcpyW(pathtosearch, curDir);
    len = GetEnvironmentVariableW(envPath, &pathtosearch[2], ARRAY_SIZE(pathtosearch)-2);
    if ((len == 0) || (len >= ARRAY_SIZE(pathtosearch) - 2)) {
      static const WCHAR curDir[] = {'.','\0'};
      lstrcpyW (pathtosearch, curDir);
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
    GetFullPathNameW(firstParam, ARRAY_SIZE(pathtosearch), pathtosearch, NULL);
    lastSlash = wcsrchr(pathtosearch, '\\');
    if (lastSlash && wcschr(lastSlash, '.') != NULL) extensionsupplied = TRUE;
    lstrcpyW(stemofsearch, lastSlash+1);

    /* Reduce pathtosearch to a path with trailing '\' to support c:\a.bat and
       c:\windows\a.bat syntax                                                 */
    if (lastSlash) *(lastSlash + 1) = 0x00;
    explicit_path = TRUE;
  }

  /* Now extract PATHEXT */
  len = GetEnvironmentVariableW(envPathExt, pathext, ARRAY_SIZE(pathext));
  if ((len == 0) || (len >= ARRAY_SIZE(pathext))) {
    lstrcpyW (pathext, dfltPathExt);
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

        /* Since you can have eg. ..\.. on the path, need to expand
           to full information                                      */
        GetFullPathNameW(temp, MAX_PATH, thisDir, NULL);
    }

    /* 1. If extension supplied, see if that file exists */
    lstrcatW(thisDir, slashW);
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
      static const WCHAR allFiles[] = {'.','*','\0'};

      lstrcatW(thisDir,allFiles);
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
      static const WCHAR batExt[] = {'.','b','a','t','\0'};
      static const WCHAR cmdExt[] = {'.','c','m','d','\0'};

      WINE_TRACE("Found as %s\n", wine_dbgstr_w(thisDir));

      /* Special case BAT and CMD */
      if (ext && (!wcsicmp(ext, batExt) || !wcsicmp(ext, cmdExt))) {
        BOOL oldinteractive = interactive;
        interactive = FALSE;
        WCMD_batch (thisDir, command, called, NULL, INVALID_HANDLE_VALUE);
        interactive = oldinteractive;
        return;
      } else {

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
        heap_free(st.lpReserved2);
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
        GetExitCodeProcess (pe.hProcess, &errorlevel);
        if (errorlevel == STILL_ACTIVE) errorlevel = 0;

        CloseHandle(pe.hProcess);
        CloseHandle(pe.hThread);
        return;
      }
    }
  }

  /* Not found anywhere - were we called? */
  if (called) {
    CMD_LIST *toExecute = NULL;         /* Commands left to be executed */

    /* Parse the command string, without reading any more input */
    WCMD_ReadAndParseLine(command, &toExecute, INVALID_HANDLE_VALUE);
    WCMD_process_commands(toExecute, FALSE, called);
    WCMD_free_commands(toExecute);
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

/*****************************************************************************
 * Process one command. If the command is EXIT this routine does not return.
 * We will recurse through here executing batch files.
 * Note: If call is used to a non-existing program, we reparse the line and
 *       try to run it as an internal command. 'retrycall' represents whether
 *       we are attempting this retry.
 */
void WCMD_execute (const WCHAR *command, const WCHAR *redirects,
                   CMD_LIST **cmdList, BOOL retrycall)
{
    WCHAR *cmd, *parms_start, *redir;
    WCHAR *pos;
    int status, i, cmd_index;
    DWORD count, creationDisposition;
    HANDLE h;
    WCHAR *whichcmd;
    SECURITY_ATTRIBUTES sa;
    WCHAR *new_cmd = NULL;
    WCHAR *new_redir = NULL;
    HANDLE old_stdhandles[3] = {GetStdHandle (STD_INPUT_HANDLE),
                                GetStdHandle (STD_OUTPUT_HANDLE),
                                GetStdHandle (STD_ERROR_HANDLE)};
    DWORD  idx_stdhandles[3] = {STD_INPUT_HANDLE,
                                STD_OUTPUT_HANDLE,
                                STD_ERROR_HANDLE};
    BOOL prev_echo_mode, piped = FALSE;

    WINE_TRACE("command on entry:%s (%p)\n",
               wine_dbgstr_w(command), cmdList);

    /* Move copy of the command onto the heap so it can be expanded */
    new_cmd = heap_xalloc(MAXSTRING * sizeof(WCHAR));
    lstrcpyW(new_cmd, command);
    cmd = new_cmd;

    /* Move copy of the redirects onto the heap so it can be expanded */
    new_redir = heap_xalloc(MAXSTRING * sizeof(WCHAR));
    redir = new_redir;

    /* Strip leading whitespaces, and a '@' if supplied */
    whichcmd = WCMD_skip_leading_spaces(cmd);
    WINE_TRACE("Command: '%s'\n", wine_dbgstr_w(cmd));
    if (whichcmd[0] == '@') whichcmd++;

    /* Check if the command entered is internal, and identify which one */
    count = 0;
    while (IsCharAlphaNumericW(whichcmd[count])) {
      count++;
    }
    for (i=0; i<=WCMD_EXIT; i++) {
      if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_STRINGSORT,
        whichcmd, count, inbuilt[i], -1) == CSTR_EQUAL) break;
    }
    cmd_index = i;
    parms_start = WCMD_skip_leading_spaces (&whichcmd[count]);

    /* If the next command is a pipe then we implement pipes by redirecting
       the output from this command to a temp file and input into the
       next command from that temp file.
       Note: Do not do this for a for or if statement as the pipe is for
       the individual statements, not the for or if itself.
       FIXME: Use of named pipes would make more sense here as currently this
       process has to finish before the next one can start but this requires
       a change to not wait for the first app to finish but rather the pipe  */
    if (!(cmd_index == WCMD_FOR || cmd_index == WCMD_IF) &&
        cmdList && (*cmdList)->nextcommand &&
        (*cmdList)->nextcommand->prevDelim == CMD_PIPE) {

        WCHAR temp_path[MAX_PATH];
        static const WCHAR cmdW[]     = {'C','M','D','\0'};

        /* Remember piping is in action */
        WINE_TRACE("Output needs to be piped\n");
        piped = TRUE;

        /* Generate a unique temporary filename */
        GetTempPathW(ARRAY_SIZE(temp_path), temp_path);
        GetTempFileNameW(temp_path, cmdW, 0, (*cmdList)->nextcommand->pipeFile);
        WINE_TRACE("Using temporary file of %s\n",
                   wine_dbgstr_w((*cmdList)->nextcommand->pipeFile));
    }

    /* If piped output, send stdout to the pipe by appending >filename to redirects */
    if (piped) {
        static const WCHAR redirOut[] = {'%','s',' ','>',' ','%','s','\0'};
        wsprintfW (new_redir, redirOut, redirects, (*cmdList)->nextcommand->pipeFile);
        WINE_TRACE("Redirects now %s\n", wine_dbgstr_w(new_redir));
    } else {
        lstrcpyW(new_redir, redirects);
    }

    /* Expand variables in command line mode only (batch mode will
       be expanded as the line is read in, except for 'for' loops) */
    handleExpansion(new_cmd, (context != NULL), delayedsubst);
    handleExpansion(new_redir, (context != NULL), delayedsubst);

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
      lstrcpyW(envvar, equalW);
      lstrcatW(envvar, cmd);
      if (GetEnvironmentVariableW(envvar, dir, MAX_PATH) == 0) {
        static const WCHAR fmt[] = {'%','s','\\','\0'};
        wsprintfW(cmd, fmt, cmd);
        WINE_TRACE("No special directory settings, using dir of %s\n", wine_dbgstr_w(cmd));
      }
      WINE_TRACE("Got directory %s as %s\n", wine_dbgstr_w(envvar), wine_dbgstr_w(cmd));
      status = SetCurrentDirectoryW(cmd);
      if (!status) WCMD_print_error ();
      heap_free(cmd );
      heap_free(new_redir);
      return;
    }

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    /*
     *  Redirect stdin, stdout and/or stderr if required.
     *  Note: Do not do this for a for or if statement as the pipe is for
     *  the individual statements, not the for or if itself.
     */
    if (!(cmd_index == WCMD_FOR || cmd_index == WCMD_IF)) {
      /* STDIN could come from a preceding pipe, so delete on close if it does */
      if (cmdList && (*cmdList)->pipeFile[0] != 0x00) {
          WINE_TRACE("Input coming from %s\n", wine_dbgstr_w((*cmdList)->pipeFile));
          h = CreateFileW((*cmdList)->pipeFile, GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
          if (h == INVALID_HANDLE_VALUE) {
            WCMD_print_error ();
            heap_free(cmd);
            heap_free(new_redir);
            return;
          }
          SetStdHandle (STD_INPUT_HANDLE, h);

          /* No need to remember the temporary name any longer once opened */
          (*cmdList)->pipeFile[0] = 0x00;

      /* Otherwise STDIN could come from a '<' redirect */
      } else if ((pos = wcschr(new_redir,'<')) != NULL) {
        h = CreateFileW(WCMD_parameter(++pos, 0, NULL, FALSE, FALSE), GENERIC_READ, FILE_SHARE_READ,
                        &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
          WCMD_print_error ();
          heap_free(cmd);
          heap_free(new_redir);
          return;
        }
        SetStdHandle (STD_INPUT_HANDLE, h);
      }

      /* Scan the whole command looking for > and 2> */
      while (redir != NULL && ((pos = wcschr(redir,'>')) != NULL)) {
        int handle = 0;

        if (pos > redir && (*(pos-1)=='2'))
          handle = 2;
        else
          handle = 1;

        pos++;
        if ('>' == *pos) {
          creationDisposition = OPEN_ALWAYS;
          pos++;
        }
        else {
          creationDisposition = CREATE_ALWAYS;
        }

        /* Add support for 2>&1 */
        redir = pos;
        if (*pos == '&') {
          int idx = *(pos+1) - '0';

          if (DuplicateHandle(GetCurrentProcess(),
                          GetStdHandle(idx_stdhandles[idx]),
                          GetCurrentProcess(),
                          &h,
                          0, TRUE, DUPLICATE_SAME_ACCESS) == 0) {
            WINE_FIXME("Duplicating handle failed with gle %d\n", GetLastError());
          }
          WINE_TRACE("Redirect %d (%p) to %d (%p)\n", handle, GetStdHandle(idx_stdhandles[idx]), idx, h);

        } else {
          WCHAR *param = WCMD_parameter(pos, 0, NULL, FALSE, FALSE);
          h = CreateFileW(param, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE,
                          &sa, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
          if (h == INVALID_HANDLE_VALUE) {
            WCMD_print_error ();
            heap_free(cmd);
            heap_free(new_redir);
            return;
          }
          if (SetFilePointer (h, 0, NULL, FILE_END) ==
                INVALID_SET_FILE_POINTER) {
            WCMD_print_error ();
          }
          WINE_TRACE("Redirect %d to '%s' (%p)\n", handle, wine_dbgstr_w(param), h);
        }

        SetStdHandle (idx_stdhandles[handle], h);
      }
    } else {
      WINE_TRACE("Not touching redirects for a FOR or IF command\n");
    }
    WCMD_parse (parms_start, quals, param1, param2);
    WINE_TRACE("param1: %s, param2: %s\n", wine_dbgstr_w(param1), wine_dbgstr_w(param2));

    if (i <= WCMD_EXIT && (parms_start[0] == '/') && (parms_start[1] == '?')) {
      /* this is a help request for a builtin program */
      i = WCMD_HELP;
      memcpy(parms_start, whichcmd, count * sizeof(WCHAR));
      parms_start[count] = '\0';

    }

    switch (i) {

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
        WCMD_output_asis(newlineW);
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
          if (i==WCMD_FOR) WCMD_for (parms_start, cmdList);
          else if (i==WCMD_IF) WCMD_if (parms_start, cmdList);
          break;
        }
        /* else: drop through */
      default:
        prev_echo_mode = echo_mode;
        WCMD_run_program (whichcmd, FALSE);
        echo_mode = prev_echo_mode;
    }
    heap_free(cmd);
    heap_free(new_redir);

    /* Restore old handles */
    for (i=0; i<3; i++) {
      if (old_stdhandles[i] != GetStdHandle(idx_stdhandles[i])) {
        CloseHandle (GetStdHandle (idx_stdhandles[i]));
        SetStdHandle (idx_stdhandles[i], old_stdhandles[i]);
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
    static const WCHAR failedMsg[]  = {'F','a','i','l','e','d','!','\0'};

    if (!LoadStringW(GetModuleHandleW(NULL), id, msg, ARRAY_SIZE(msg))) {
       WINE_FIXME("LoadString failed with %d\n", GetLastError());
       lstrcpyW(msg, failedMsg);
    }
    return msg;
}

/***************************************************************************
 * WCMD_DumpCommands
 *
 *	Dumps out the parsed command line to ensure syntax is correct
 */
static void WCMD_DumpCommands(CMD_LIST *commands) {
    CMD_LIST *thisCmd = commands;

    WINE_TRACE("Parsed line:\n");
    while (thisCmd != NULL) {
      WINE_TRACE("%p %d %2.2d %p %s Redir:%s\n",
               thisCmd,
               thisCmd->prevDelim,
               thisCmd->bracketDepth,
               thisCmd->nextcommand,
               wine_dbgstr_w(thisCmd->command),
               wine_dbgstr_w(thisCmd->redirects));
      thisCmd = thisCmd->nextcommand;
    }
}

/***************************************************************************
 * WCMD_addCommand
 *
 *   Adds a command to the current command list
 */
static void WCMD_addCommand(WCHAR *command, int *commandLen,
                     WCHAR *redirs,  int *redirLen,
                     WCHAR **copyTo, int **copyToLen,
                     CMD_DELIMITERS prevDelim, int curDepth,
                     CMD_LIST **lastEntry, CMD_LIST **output) {

    CMD_LIST *thisEntry = NULL;

    /* Allocate storage for command */
    thisEntry = heap_xalloc(sizeof(CMD_LIST));

    /* Copy in the command */
    if (command) {
        thisEntry->command = heap_xalloc((*commandLen+1) * sizeof(WCHAR));
        memcpy(thisEntry->command, command, *commandLen * sizeof(WCHAR));
        thisEntry->command[*commandLen] = 0x00;

        /* Copy in the redirects */
        thisEntry->redirects = heap_xalloc((*redirLen+1) * sizeof(WCHAR));
        memcpy(thisEntry->redirects, redirs, *redirLen * sizeof(WCHAR));
        thisEntry->redirects[*redirLen] = 0x00;
        thisEntry->pipeFile[0] = 0x00;

        /* Reset the lengths */
        *commandLen   = 0;
        *redirLen     = 0;
        *copyToLen    = commandLen;
        *copyTo       = command;

    } else {
        thisEntry->command = NULL;
        thisEntry->redirects = NULL;
        thisEntry->pipeFile[0] = 0x00;
    }

    /* Fill in other fields */
    thisEntry->nextcommand = NULL;
    thisEntry->prevDelim = prevDelim;
    thisEntry->bracketDepth = curDepth;
    if (*lastEntry) {
        (*lastEntry)->nextcommand = thisEntry;
    } else {
        *output = thisEntry;
    }
    *lastEntry = thisEntry;
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
WCHAR *WCMD_ReadAndParseLine(const WCHAR *optionalcmd, CMD_LIST **output, HANDLE readFrom)
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
    CMD_LIST *lastEntry = NULL;
    CMD_DELIMITERS prevDelim = CMD_NONE;
    static WCHAR    *extraSpace = NULL;  /* Deliberately never freed */
    static const WCHAR remCmd[] = {'r','e','m'};
    static const WCHAR forCmd[] = {'f','o','r'};
    static const WCHAR ifCmd[]  = {'i','f'};
    static const WCHAR ifElse[] = {'e','l','s','e'};
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
    int       lineCurDepth;                  /* Bracket depth when line was read in */
    BOOL      resetAtEndOfLine = FALSE;      /* Do we need to reset curdepth at EOL */

    /* Allocate working space for a command read from keyboard, file etc */
    if (!extraSpace)
        extraSpace = heap_xalloc((MAXSTRING+1) * sizeof(WCHAR));
    if (!extraSpace)
    {
        WINE_ERR("Could not allocate memory for extraSpace\n");
        return NULL;
    }

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

    /* Handle truncated input - issue warning */
    if (lstrlenW(extraSpace) == MAXSTRING -1) {
        WCMD_output_asis_stderr(WCMD_LoadMessage(WCMD_TRUNCATEDLINE));
        WCMD_output_asis_stderr(extraSpace);
        WCMD_output_asis_stderr(newlineW);
    }

    /* Replace env vars if in a batch context */
    if (context) handleExpansion(extraSpace, FALSE, FALSE);

    /* Skip preceding whitespace */
    while (*curPos == ' ' || *curPos == '\t') curPos++;

    /* Show prompt before batch line IF echo is on and in batch program */
    if (context && echo_mode && *curPos && (*curPos != '@')) {
      static const WCHAR echoDot[] = {'e','c','h','o','.'};
      static const WCHAR echoCol[] = {'e','c','h','o',':'};
      static const WCHAR echoSlash[] = {'e','c','h','o','/'};
      const DWORD len = ARRAY_SIZE(echoDot);
      DWORD curr_size = lstrlenW(curPos);
      DWORD min_len = (curr_size < len ? curr_size : len);
      WCMD_show_prompt(TRUE);
      WCMD_output_asis(curPos);
      /* I don't know why Windows puts a space here but it does */
      /* Except for lines starting with 'echo.', 'echo:' or 'echo/'. Ask MS why */
      if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                         curPos, min_len, echoDot, len) != CSTR_EQUAL
          && CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                         curPos, min_len, echoCol, len) != CSTR_EQUAL
          && CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                         curPos, min_len, echoSlash, len) != CSTR_EQUAL)
      {
          WCMD_output_asis(spaceW);
      }
      WCMD_output_asis(newlineW);
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
        static const WCHAR forDO[] = {'d','o'};

        /* If command starts with 'rem ' or identifies a label, ignore any &&, ( etc. */
        if (WCMD_keyword_ws_found(remCmd, ARRAY_SIZE(remCmd), curPos) || *curPos == ':') {
          inOneLine = TRUE;

        } else if (WCMD_keyword_ws_found(forCmd, ARRAY_SIZE(forCmd), curPos)) {
          inFor = TRUE;

        /* If command starts with 'if ' or 'else ', handle ('s mid line. We should ensure this
           is only true in the command portion of the IF statement, but this
           should suffice for now.
           To be able to handle ('s in the condition part take as much as evaluate_if_condition
           would take and skip parsing it here. */
        } else if (WCMD_keyword_ws_found(ifCmd, ARRAY_SIZE(ifCmd), curPos)) {
          int negate; /* Negate condition */
          int test;   /* Condition evaluation result */
          WCHAR *p, *command;

          inIf = TRUE;

          p = curPos+(ARRAY_SIZE(ifCmd));
          while (*p == ' ' || *p == '\t')
            p++;
          WCMD_parse (p, quals, param1, param2);

          /* Function evaluate_if_condition relies on the global variables quals, param1 and param2
             set in a call to WCMD_parse before */
          if (evaluate_if_condition(p, &command, &test, &negate) != -1)
          {
              int if_condition_len = command - curPos;
              WINE_TRACE("p: %s, quals: %s, param1: %s, param2: %s, command: %s, if_condition_len: %d\n",
                         wine_dbgstr_w(p), wine_dbgstr_w(quals), wine_dbgstr_w(param1),
                         wine_dbgstr_w(param2), wine_dbgstr_w(command), if_condition_len);
              memcpy(&curCopyTo[*curLen], curPos, if_condition_len*sizeof(WCHAR));
              (*curLen)+=if_condition_len;
              curPos+=if_condition_len;
          }

        } else if (WCMD_keyword_ws_found(ifElse, ARRAY_SIZE(ifElse), curPos)) {
          const int keyw_len = ARRAY_SIZE(ifElse) + 1;
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
        } else if (inFor && WCMD_keyword_ws_found(forDO, ARRAY_SIZE(forDO), curPos)) {
          const int keyw_len = ARRAY_SIZE(forDO) + 1;
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
          static const WCHAR forIN[] = {'i','n'};

          WINE_TRACE("Found 'FOR ', comparing next parm: '%s'\n", wine_dbgstr_w(curPos));

          if (WCMD_keyword_ws_found(forIN, ARRAY_SIZE(forIN), curPos)) {
            const int keyw_len = ARRAY_SIZE(forIN) + 1;
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
                if (curStringLen > 2
                        && (*(curPos-1)>='1') && (*(curPos-1)<='9')
                        && ((*(curPos-2)==' ') || (*(curPos-2)=='\t'))) {
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
                    WCMD_addCommand(curString, &curStringLen,
                          curRedirs, &curRedirsLen,
                          &curCopyTo, &curLen,
                          prevDelim, curDepth,
                          &lastEntry, output);

                  }

                  if (*(curPos+1) == '|') {
                    curPos++; /* Skip other | */
                    prevDelim = CMD_ONFAILURE;
                  } else {
                    prevDelim = CMD_PIPE;
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
                } else if (inIf ||
                           (inElse && lastWasElse && onlyWhiteSpace) ||
                           (inFor && (lastWasIn || lastWasDo) && onlyWhiteSpace)) {

                   /* If entering into an 'IN', set inIn */
                  if (inFor && lastWasIn && onlyWhiteSpace) {
                    WINE_TRACE("Inside an IN\n");
                    inIn = TRUE;
                  }

                  /* Add the current command */
                  WCMD_addCommand(curString, &curStringLen,
                                  curRedirs, &curRedirsLen,
                                  &curCopyTo, &curLen,
                                  prevDelim, curDepth,
                                  &lastEntry, output);

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
                    WCMD_addCommand(curString, &curStringLen,
                          curRedirs, &curRedirsLen,
                          &curCopyTo, &curLen,
                          prevDelim, curDepth,
                          &lastEntry, output);

                  }

                  if (*(curPos+1) == '&') {
                    curPos++; /* Skip other & */
                    prevDelim = CMD_ONSUCCESS;
                  } else {
                    prevDelim = CMD_NONE;
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
                      WCMD_addCommand(curString, &curStringLen,
                            curRedirs, &curRedirsLen,
                            &curCopyTo, &curLen,
                            prevDelim, curDepth,
                            &lastEntry, output);
                  }

                  /* Add an empty entry to the command list */
                  prevDelim = CMD_NONE;
                  WCMD_addCommand(NULL, &curStringLen,
                        curRedirs, &curRedirsLen,
                        &curCopyTo, &curLen,
                        prevDelim, curDepth,
                        &lastEntry, output);
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
          WCMD_addCommand(curString, &curStringLen,
                curRedirs, &curRedirsLen,
                &curCopyTo, &curLen,
                prevDelim, curDepth,
                &lastEntry, output);

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
        prevDelim = CMD_NONE;
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

        } while (*extraData == 0x00);
        curPos = extraSpace;

        /* Skip preceding whitespace */
        while (*curPos == ' ' || *curPos == '\t') curPos++;

        /* Replace env vars if in a batch context */
        if (context) handleExpansion(curPos, FALSE, FALSE);

        /* Continue to echo commands IF echo is on and in batch program */
        if (context && echo_mode && *curPos && *curPos != '@') {
          WCMD_output_asis(extraSpace);
          WCMD_output_asis(newlineW);
        }

        /* Skip repeated 'no echo' characters and whitespace */
        while (*curPos == '@' || *curPos == ' ' || *curPos == '\t') curPos++;
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
                                BOOL retrycall) {

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
        WCMD_execute (thisCmd->command, thisCmd->redirects, &thisCmd, retrycall);
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
      heap_free(thisCmd->command);
      heap_free(thisCmd->redirects);
      heap_free(thisCmd);
    }
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
  static const WCHAR offW[] = {'O','F','F','\0'};
  static const WCHAR promptW[] = {'P','R','O','M','P','T','\0'};
  static const WCHAR defaultpromptW[] = {'$','P','$','G','\0'};
  static const WCHAR comspecW[] = {'C','O','M','S','P','E','C',0};
  static const WCHAR cmdW[] = {'\\','c','m','d','.','e','x','e',0};
  WCHAR comspec[MAX_PATH];
  CMD_LIST *toExecute = NULL;         /* Commands left to be executed */
  RTL_OSVERSIONINFOEXW osv;
  char osver[50];
  STARTUPINFOW startupInfo;
  const WCHAR *arg;

  if (!GetEnvironmentVariableW(comspecW, comspec, ARRAY_SIZE(comspec)))
  {
      GetSystemDirectoryW(comspec, ARRAY_SIZE(comspec) - ARRAY_SIZE(cmdW));
      lstrcatW(comspec, cmdW);
      SetEnvironmentVariableW(comspecW, comspec);
  }

  srand(time(NULL));

  /* Get the windows version being emulated */
  osv.dwOSVersionInfoSize = sizeof(osv);
  RtlGetVersion(&osv);

  /* Pre initialize some messages */
  lstrcpyW(anykey, WCMD_LoadMessage(WCMD_ANYKEY));
  sprintf(osver, "%d.%d.%d", osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);
  cmd = WCMD_format_string(WCMD_LoadMessage(WCMD_VERSION), osver);
  lstrcpyW(version_string, cmd);
  LocalFree(cmd);
  cmd = NULL;

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
    WCMD_echo(offW);
  }

  /* Until we start to read from the keyboard, stay as non-interactive */
  interactive = FALSE;

  SetEnvironmentVariableW(promptW, defaultpromptW);

  if (opt_c || opt_k) {
      int     len;
      WCHAR   *q1 = NULL,*q2 = NULL,*p;

      /* Take a copy */
      cmd = heap_strdupW(arg);

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
        len = GetEnvironmentVariableW(envPathExt, pathext, ARRAY_SIZE(pathext));
        if ((len == 0) || (len >= ARRAY_SIZE(pathext))) {
          lstrcpyW (pathext, dfltPathExt);
        }

        /* If the supplied parameter has any directory information, look there */
        WINE_TRACE("First parameter is '%s'\n", wine_dbgstr_w(thisArg));
        if (wcschr(thisArg, '\\') != NULL) {

          GetFullPathNameW(thisArg, ARRAY_SIZE(string), string, NULL);
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

  /* Save cwd into appropriate env var (Must be before the /c processing */
  GetCurrentDirectoryW(ARRAY_SIZE(string), string);
  if (IsCharAlphaW(string[0]) && string[1] == ':') {
    static const WCHAR fmt[] = {'=','%','c',':','\0'};
    wsprintfW(envvar, fmt, string[0]);
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
      WCMD_free_commands(toExecute);
      toExecute = NULL;

      heap_free(cmd);
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
      static const WCHAR regKeyW[] = {'S','o','f','t','w','a','r','e','\\',
                                      'M','i','c','r','o','s','o','f','t','\\',
                                      'C','o','m','m','a','n','d',' ','P','r','o','c','e','s','s','o','r','\0'};
      static const WCHAR dfltColorW[] = {'D','e','f','a','u','l','t','C','o','l','o','r','\0'};

      if (RegOpenKeyExW(HKEY_CURRENT_USER, regKeyW,
                       0, KEY_READ, &key) == ERROR_SUCCESS) {
          WCHAR  strvalue[4];

          /* See if DWORD or REG_SZ */
          if (RegQueryValueExW(key, dfltColorW, NULL, &type,
                     NULL, NULL) == ERROR_SUCCESS) {
              if (type == REG_DWORD) {
                  size = sizeof(DWORD);
                  RegQueryValueExW(key, dfltColorW, NULL, NULL,
                                  (LPBYTE)&value, &size);
              } else if (type == REG_SZ) {
                  size = ARRAY_SIZE(strvalue);
                  RegQueryValueExW(key, dfltColorW, NULL, NULL,
                                  (LPBYTE)strvalue, &size);
                  value = wcstoul(strvalue, NULL, 10);
              }
          }
          RegCloseKey(key);
      }

      if (value == 0 && RegOpenKeyExW(HKEY_LOCAL_MACHINE, regKeyW,
                       0, KEY_READ, &key) == ERROR_SUCCESS) {
          WCHAR  strvalue[4];

          /* See if DWORD or REG_SZ */
          if (RegQueryValueExW(key, dfltColorW, NULL, &type,
                     NULL, NULL) == ERROR_SUCCESS) {
              if (type == REG_DWORD) {
                  size = sizeof(DWORD);
                  RegQueryValueExW(key, dfltColorW, NULL, NULL,
                                  (LPBYTE)&value, &size);
              } else if (type == REG_SZ) {
                  size = ARRAY_SIZE(strvalue);
                  RegQueryValueExW(key, dfltColorW, NULL, NULL,
                                  (LPBYTE)strvalue, &size);
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
      WCMD_free_commands(toExecute);
      toExecute = NULL;
      heap_free(cmd);
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
    WCMD_free_commands(toExecute);
    promptNewLine = !!toExecute;
    toExecute = NULL;
  }
  return 0;
}
