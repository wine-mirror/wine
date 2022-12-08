/*
 * CMD - Wine-compatible command line interface - batch interface.
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

#include "wcmd.h"
#include "wine/debug.h"

extern struct env_stack *saved_environment;

WINE_DEFAULT_DEBUG_CHANNEL(cmd);

/****************************************************************************
 * WCMD_batch
 *
 * Open and execute a batch file.
 * On entry *command includes the complete command line beginning with the name
 * of the batch file (if a CALL command was entered the CALL has been removed).
 * *file is the name of the file, which might not exist and may not have the
 * .BAT suffix on. Called is 1 for a CALL, 0 otherwise.
 *
 * We need to handle recursion correctly, since one batch program might call another.
 * So parameters for this batch file are held in a BATCH_CONTEXT structure.
 *
 * To support call within the same batch program, another input parameter is
 * a label to goto once opened.
 */

void WCMD_batch (WCHAR *file, WCHAR *command, BOOL called, WCHAR *startLabel, HANDLE pgmHandle)
{
  HANDLE h = INVALID_HANDLE_VALUE;
  BATCH_CONTEXT *prev_context;

  if (startLabel == NULL) {
    h = CreateFileW (file, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
      SetLastError (ERROR_FILE_NOT_FOUND);
      WCMD_print_error ();
      return;
    }
  } else {
    DuplicateHandle(GetCurrentProcess(), pgmHandle,
                    GetCurrentProcess(), &h,
                    0, FALSE, DUPLICATE_SAME_ACCESS);
  }

/*
 *	Create a context structure for this batch file.
 */

  prev_context = context;
  context = LocalAlloc (LMEM_FIXED, sizeof (BATCH_CONTEXT));
  context -> h = h;
  context->batchfileW = xstrdupW(file);
  context -> command = command;
  memset(context -> shift_count, 0x00, sizeof(context -> shift_count));
  context -> prev_context = prev_context;
  context -> skip_rest = FALSE;

  /* If processing a call :label, 'goto' the label in question */
  if (startLabel) {
    lstrcpyW(param1, startLabel);
    WCMD_goto(NULL);
  }

/*
 * 	Work through the file line by line. Specific batch commands are processed here,
 * 	the rest are handled by the main command processor.
 */

  while (context -> skip_rest == FALSE) {
      CMD_LIST *toExecute = NULL;         /* Commands left to be executed */
      if (!WCMD_ReadAndParseLine(NULL, &toExecute, h))
        break;
      /* Note: although this batch program itself may be called, we are not retrying
         the command as a result of a call failing to find a program, hence the
         retryCall parameter below is FALSE                                           */
      WCMD_process_commands(toExecute, FALSE, FALSE);
      WCMD_free_commands(toExecute);
      toExecute = NULL;
  }
  CloseHandle (h);

/*
 *  If there are outstanding setlocal's to the current context, unwind them.
 */
  while (saved_environment && saved_environment->batchhandle == context->h) {
      WCMD_endlocal();
  }

/*
 *	If invoked by a CALL, we return to the context of our caller. Otherwise return
 *	to the caller's caller.
 */

  free(context->batchfileW);
  LocalFree(context);
  if ((prev_context != NULL) && (!called)) {
    WINE_TRACE("Batch completed, but was not 'called' so skipping outer batch too\n");
    prev_context -> skip_rest = TRUE;
    context = prev_context;
  }
  context = prev_context;
}

/*******************************************************************
 * WCMD_parameter_with_delims
 *
 * Extracts a delimited parameter from an input string, providing
 * the delimiters characters to use
 *
 * PARAMS
 *  s     [I] input string, non NULL
 *  n     [I] # of the parameter to return, counted from 0
 *  start [O] Optional. Pointer to the first char of param n in s
 *  raw   [I] TRUE to return the parameter in raw format (quotes maintained)
 *            FALSE to return the parameter with quotes stripped (including internal ones)
 *  wholecmdline [I] TRUE to indicate this routine is being used to parse the
 *                   command line, and special logic for arg0->1 transition
 *                   needs to be applied.
 *  delims[I] The delimiter characters to use
 *
 * RETURNS
 *  Success: The nth delimited parameter found in s
 *           if start != NULL, *start points to the start of the param (quotes maintained)
 *  Failure: An empty string if the param is not found.
 *           *start == NULL
 *
 * NOTES
 *  Return value is stored in static storage (i.e. overwritten after each call).
 *  By default, the parameter is returned with quotes removed, ready for use with
 *  other API calls, e.g. c:\"a b"\c is returned as c:\a b\c. However, some commands
 *  need to preserve the exact syntax (echo, for, etc) hence the raw option.
 */
WCHAR *WCMD_parameter_with_delims (WCHAR *s, int n, WCHAR **start,
                                   BOOL raw, BOOL wholecmdline, const WCHAR *delims)
{
    int curParamNb = 0;
    static WCHAR param[MAXSTRING];
    WCHAR *p = s, *begin;

    if (start != NULL) *start = NULL;
    param[0] = '\0';

    while (TRUE) {

        /* Absorb repeated word delimiters until we get to the next token (or the end!) */
        while (*p && (wcschr(delims, *p) != NULL))
            p++;
        if (*p == '\0') return param;

        /* If we have reached the token number we want, remember the beginning of it */
        if (start != NULL && curParamNb == n) *start = p;

        /* Return the whole word up to the next delimiter, handling quotes in the middle
           of it, e.g. a"\b c\"d is a single parameter.                                  */
        begin = p;

        /* Loop character by character, but just need to special case quotes */
        while (*p) {
            /* Once we have found a delimiter, break */
            if (wcschr(delims, *p) != NULL) break;

            /* Very odd special case - Seems as if a ( acts as a delimiter which is
               not swallowed but is effective only when it comes between the program
               name and the parameters. Need to avoid this triggering when used
               to walk parameters generally.                                         */
            if (wholecmdline && curParamNb == 0 && *p=='(') break;

            /* If we find a quote, copy until we get the end quote */
            if (*p == '"') {
                p++;
                while (*p && *p != '"') p++;
            }

            /* Now skip the character / quote */
            if (*p) p++;
        }

        if (curParamNb == n) {
            /* Return the parameter in static storage either as-is (raw) or
               suitable for use with other win32 api calls (quotes stripped) */
            if (raw) {
                memcpy(param, begin, (p - begin) * sizeof(WCHAR));
                param[p-begin] = '\0';
            } else {
                int i=0;
                while (begin < p) {
                  if (*begin != '"') param[i++] = *begin;
                  begin++;
                }
                param[i] = '\0';
            }
            return param;
        }
        curParamNb++;
    }
}

/*******************************************************************
 * WCMD_parameter
 *
 * Extracts a delimited parameter from an input string, using a
 * default set of delimiter characters. For parameters, see the main
 * function above.
 */
WCHAR *WCMD_parameter (WCHAR *s, int n, WCHAR **start, BOOL raw,
                       BOOL wholecmdline)
{
  return WCMD_parameter_with_delims (s, n, start, raw, wholecmdline, L" \t,=;");
}

/****************************************************************************
 * WCMD_fgets
 *
 * Gets one line from a file/console and puts it into buffer buf
 * Pre:  buf has size noChars
 *       1 <= noChars <= MAXSTRING
 * Post: buf is filled with at most noChars-1 characters, and gets nul-terminated
         buf does not include EOL terminator
 * Returns:
 *       buf on success
 *       NULL on error or EOF
 */

WCHAR *WCMD_fgets(WCHAR *buf, DWORD noChars, HANDLE h)
{
  DWORD charsRead;
  BOOL status;
  DWORD i;

  /* We can't use the native f* functions because of the filename syntax differences
     between DOS and Unix. Also need to lose the LF (or CRLF) from the line. */

  if (!ReadConsoleW(h, buf, noChars, &charsRead, NULL)) {
      LARGE_INTEGER filepos;
      char *bufA;
      UINT cp;
      const char *p;

      cp = GetOEMCP();
      bufA = xalloc(noChars);

      /* Save current file position */
      filepos.QuadPart = 0;
      SetFilePointerEx(h, filepos, &filepos, FILE_CURRENT);

      status = ReadFile(h, bufA, noChars, &charsRead, NULL);
      if (!status || charsRead == 0) {
          free(bufA);
          return NULL;
      }

      /* Find first EOL */
      for (p = bufA; p < (bufA + charsRead); p = CharNextExA(cp, p, 0)) {
          if (*p == '\n' || *p == '\r')
              break;
      }

      /* Sets file pointer to the start of the next line, if any */
      filepos.QuadPart += p - bufA + 1 + (*p == '\r' ? 1 : 0);
      SetFilePointerEx(h, filepos, NULL, FILE_BEGIN);

      i = MultiByteToWideChar(cp, 0, bufA, p - bufA, buf, noChars);
      free(bufA);
  }
  else {
      if (!charsRead) return NULL;

      /* Find first EOL */
      for (i = 0; i < charsRead; i++) {
          if (buf[i] == '\n' || buf[i] == '\r')
              break;
      }
  }

  /* Truncate at EOL (or end of buffer) */
  if (i == noChars)
    i--;

  buf[i] = '\0';

  return buf;
}

/****************************************************************************
 * WCMD_HandleTildeModifiers
 *
 * Handle the ~ modifiers when expanding %0-9 or (%a-z/A-Z in for command)
 *    %~xxxxxV  (V=0-9 or A-Z, a-z)
 * Where xxxx is any combination of:
 *    ~ - Removes quotes
 *    f - Fully qualified path (assumes current dir if not drive\dir)
 *    d - drive letter
 *    p - path
 *    n - filename
 *    x - file extension
 *    s - path with shortnames
 *    a - attributes
 *    t - date/time
 *    z - size
 *    $ENVVAR: - Searches ENVVAR for (contents of V) and expands to fully
 *                   qualified path
 *
 *  To work out the length of the modifier:
 *
 *  Note: In the case of %0-9 knowing the end of the modifier is easy,
 *    but in a for loop, the for end WCHARacter may also be a modifier
 *    eg. for %a in (c:\a.a) do echo XXX
 *             where XXX = %~a    (just ~)
 *                         %~aa   (~ and attributes)
 *                         %~aaxa (~, attributes and extension)
 *                   BUT   %~aax  (~ and attributes followed by 'x')
 *
 *  Hence search forwards until find an invalid modifier, and then
 *  backwards until find for variable or 0-9
 */
void WCMD_HandleTildeModifiers(WCHAR **start, BOOL atExecute)
{

#define NUMMODIFIERS 11
  static const WCHAR validmodifiers[NUMMODIFIERS] = {
        '~', 'f', 'd', 'p', 'n', 'x', 's', 'a', 't', 'z', '$'
  };

  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  WCHAR  outputparam[MAXSTRING];
  WCHAR  finaloutput[MAXSTRING];
  WCHAR  fullfilename[MAX_PATH];
  WCHAR  thisoutput[MAX_PATH];
  WCHAR  *filepart       = NULL;
  WCHAR  *pos            = *start+1;
  WCHAR  *firstModifier  = pos;
  WCHAR  *lastModifier   = NULL;
  int   modifierLen     = 0;
  BOOL  finished        = FALSE;
  int   i               = 0;
  BOOL  exists          = TRUE;
  BOOL  skipFileParsing = FALSE;
  BOOL  doneModifier    = FALSE;

  /* Search forwards until find invalid character modifier */
  while (!finished) {

    /* Work on the previous character */
    if (lastModifier != NULL) {

      for (i=0; i<NUMMODIFIERS; i++) {
        if (validmodifiers[i] == *lastModifier) {

          /* Special case '$' to skip until : found */
          if (*lastModifier == '$') {
            while (*pos != ':' && *pos) pos++;
            if (*pos == 0x00) return; /* Invalid syntax */
            pos++;                    /* Skip ':'       */
          }
          break;
        }
      }

      if (i==NUMMODIFIERS) {
        finished = TRUE;
      }
    }

    /* Save this one away */
    if (!finished) {
      lastModifier = pos;
      pos++;
    }
  }

  while (lastModifier > firstModifier) {
    WINE_TRACE("Looking backwards for parameter id: %s\n",
               wine_dbgstr_w(lastModifier));

    if (!atExecute && context && (*lastModifier >= '0' && *lastModifier <= '9')) {
      /* Its a valid parameter identifier - OK */
      break;

    } else {
      int foridx = FOR_VAR_IDX(*lastModifier);
      /* Its a valid parameter identifier - OK */
      if ((foridx >= 0) && (forloopcontext.variable[foridx] != NULL)) break;

      /* Its not a valid parameter identifier - step backwards */
      lastModifier--;
    }
  }
  if (lastModifier == firstModifier) return; /* Invalid syntax */

  /* So now, firstModifier points to beginning of modifiers, lastModifier
     points to the variable just after the modifiers. Process modifiers
     in a specific order, remembering there could be duplicates           */
  modifierLen = lastModifier - firstModifier;
  finaloutput[0] = 0x00;

  /* Extract the parameter to play with
     Special case param 0 - With %~0 you get the batch label which was called
     whereas if you start applying other modifiers to it, you get the filename
     the batch label is in                                                     */
  if (*lastModifier == '0' && modifierLen > 1) {
    lstrcpyW(outputparam, context->batchfileW);
  } else if ((*lastModifier >= '0' && *lastModifier <= '9')) {
    lstrcpyW(outputparam,
            WCMD_parameter (context -> command,
                            *lastModifier-'0' + context -> shift_count[*lastModifier-'0'],
                            NULL, FALSE, TRUE));
  } else {
    int foridx = FOR_VAR_IDX(*lastModifier);
    lstrcpyW(outputparam, forloopcontext.variable[foridx]);
  }

  /* 1. Handle '~' : Strip surrounding quotes */
  if (outputparam[0]=='"' &&
      wmemchr(firstModifier, '~', modifierLen) != NULL) {
    int len = lstrlenW(outputparam);
    if (outputparam[len-1] == '"') {
        outputparam[len-1]=0x00;
        len = len - 1;
    }
    memmove(outputparam, &outputparam[1], (len * sizeof(WCHAR))-1);
  }

  /* 2. Handle the special case of a $ */
  if (wmemchr(firstModifier, '$', modifierLen) != NULL) {
    /* Special Case: Search envar specified in $[envvar] for outputparam
       Note both $ and : are guaranteed otherwise check above would fail */
    WCHAR *begin = wcschr(firstModifier, '$') + 1;
    WCHAR *end   = wcschr(firstModifier, ':');
    WCHAR env[MAX_PATH];
    DWORD size;

    /* Extract the env var */
    memcpy(env, begin, (end-begin) * sizeof(WCHAR));
    env[(end-begin)] = 0x00;

    size = GetEnvironmentVariableW(env, NULL, 0);
    if (size > 0) {
      WCHAR *fullpath = malloc(size * sizeof(WCHAR));
      if (!fullpath || (GetEnvironmentVariableW(env, fullpath, size) == 0) ||
          (SearchPathW(fullpath, outputparam, NULL, MAX_PATH, outputparam, NULL) == 0))
          size = 0;
      free(fullpath);
    }

    if (!size) {
      /* If env var not found, return empty string */
      finaloutput[0] = 0x00;
      outputparam[0] = 0x00;
      skipFileParsing = TRUE;
    }
  }

  /* After this, we need full information on the file,
    which is valid not to exist.  */
  if (!skipFileParsing) {
    if (!WCMD_get_fullpath(outputparam, MAX_PATH, fullfilename, &filepart)) {
      exists = FALSE;
      fullfilename[0] = 0x00;
    } else {
      exists = GetFileAttributesExW(fullfilename, GetFileExInfoStandard,
                                    &fileInfo);
    }

    /* 2. Handle 'a' : Output attributes (File doesn't have to exist) */
    if (wmemchr(firstModifier, 'a', modifierLen) != NULL) {

      doneModifier = TRUE;

      if (exists) {
        lstrcpyW(thisoutput, L"---------");
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          thisoutput[0]='d';
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
          thisoutput[1]='r';
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
          thisoutput[2]='a';
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
          thisoutput[3]='h';
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
          thisoutput[4]='s';
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
          thisoutput[5]='c';
        /* FIXME: What are 6 and 7? */
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
          thisoutput[8]='l';
        lstrcatW(finaloutput, thisoutput);
      }
    }

    /* 3. Handle 't' : Date+time (File doesn't have to exist) */
    if (wmemchr(firstModifier, 't', modifierLen) != NULL) {

      SYSTEMTIME systime;
      int datelen;

      doneModifier = TRUE;

      if (exists) {
        if (finaloutput[0] != 0x00) lstrcatW(finaloutput, L" ");

        /* Format the time */
        FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &systime);
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,
                          NULL, thisoutput, MAX_PATH);
        lstrcatW(thisoutput, L" ");
        datelen = lstrlenW(thisoutput);
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &systime,
                          NULL, (thisoutput+datelen), MAX_PATH-datelen);
        lstrcatW(finaloutput, thisoutput);
      }
    }

    /* 4. Handle 'z' : File length (File doesn't have to exist) */
    if (wmemchr(firstModifier, 'z', modifierLen) != NULL) {
      /* FIXME: Output full 64 bit size (sprintf does not support I64 here) */
      ULONG/*64*/ fullsize = /*(fileInfo.nFileSizeHigh << 32) +*/
                                  fileInfo.nFileSizeLow;

      doneModifier = TRUE;
      if (exists) {
        if (finaloutput[0] != 0x00) lstrcatW(finaloutput, L" ");
        wsprintfW(thisoutput, L"%u", fullsize);
        lstrcatW(finaloutput, thisoutput);
      }
    }

    /* 4. Handle 's' : Use short paths (File doesn't have to exist) */
    if (wmemchr(firstModifier, 's', modifierLen) != NULL) {
      if (finaloutput[0] != 0x00) lstrcatW(finaloutput, L" ");

      /* Convert fullfilename's path to a short path - Save filename away as
         only path is valid, name may not exist which causes GetShortPathName
         to fail if it is provided                                            */
      if (filepart) {
        lstrcpyW(thisoutput, filepart);
        *filepart = 0x00;
        GetShortPathNameW(fullfilename, fullfilename, ARRAY_SIZE(fullfilename));
        lstrcatW(fullfilename, thisoutput);
      }
    }

    /* 5. Handle 'f' : Fully qualified path (File doesn't have to exist) */
    /*      Note this overrides d,p,n,x                                 */
    if (wmemchr(firstModifier, 'f', modifierLen) != NULL) {
      doneModifier = TRUE;
      if (finaloutput[0] != 0x00) lstrcatW(finaloutput, L" ");
      lstrcatW(finaloutput, fullfilename);
    } else {

      WCHAR drive[10];
      WCHAR dir[MAX_PATH];
      WCHAR fname[MAX_PATH];
      WCHAR ext[MAX_PATH];
      BOOL doneFileModifier = FALSE;
      BOOL addSpace = (finaloutput[0] != 0x00);

      /* Split into components */
      _wsplitpath(fullfilename, drive, dir, fname, ext);

      /* 5. Handle 'd' : Drive Letter */
      if (wmemchr(firstModifier, 'd', modifierLen) != NULL) {
        if (addSpace) {
          lstrcatW(finaloutput, L" ");
          addSpace = FALSE;
        }

        lstrcatW(finaloutput, drive);
        doneModifier = TRUE;
        doneFileModifier = TRUE;
      }

      /* 6. Handle 'p' : Path */
      if (wmemchr(firstModifier, 'p', modifierLen) != NULL) {
        if (addSpace) {
          lstrcatW(finaloutput, L" ");
          addSpace = FALSE;
        }

        lstrcatW(finaloutput, dir);
        doneModifier = TRUE;
        doneFileModifier = TRUE;
      }

      /* 7. Handle 'n' : Name */
      if (wmemchr(firstModifier, 'n', modifierLen) != NULL) {
        if (addSpace) {
          lstrcatW(finaloutput, L" ");
          addSpace = FALSE;
        }

        lstrcatW(finaloutput, fname);
        doneModifier = TRUE;
        doneFileModifier = TRUE;
      }

      /* 8. Handle 'x' : Ext */
      if (wmemchr(firstModifier, 'x', modifierLen) != NULL) {
        if (addSpace) {
          lstrcatW(finaloutput, L" ");
          addSpace = FALSE;
        }

        lstrcatW(finaloutput, ext);
        doneModifier = TRUE;
        doneFileModifier = TRUE;
      }

      /* If 's' but no other parameter, dump the whole thing */
      if (!doneFileModifier &&
          wmemchr(firstModifier, 's', modifierLen) != NULL) {
        doneModifier = TRUE;
        if (finaloutput[0] != 0x00) lstrcatW(finaloutput, L" ");
        lstrcatW(finaloutput, fullfilename);
      }
    }
  }

  /* If No other modifier processed,  just add in parameter */
  if (!doneModifier) lstrcpyW(finaloutput, outputparam);

  /* Finish by inserting the replacement into the string */
  WCMD_strsubstW(*start, lastModifier+1, finaloutput, -1);
}

/*******************************************************************
 * WCMD_call - processes a batch call statement
 *
 *	If there is a leading ':', calls within this batch program
 *	otherwise launches another program.
 */
void WCMD_call (WCHAR *command) {

  /* Run other program if no leading ':' */
  if (*command != ':') {
    WCMD_run_program(command, TRUE);
    /* If the thing we try to run does not exist, call returns 1 */
    if (errorlevel) errorlevel=1;
  } else {

    WCHAR gotoLabel[MAX_PATH];

    lstrcpyW(gotoLabel, param1);

    if (context) {

      LARGE_INTEGER li;
      FOR_CONTEXT oldcontext;

      /* Save the for variable context, then start with an empty context
         as for loop variables do not survive a call                    */
      oldcontext = forloopcontext;
      memset(&forloopcontext, 0, sizeof(forloopcontext));

      /* Save the current file position, call the same file,
         restore position                                    */
      li.QuadPart = 0;
      li.u.LowPart = SetFilePointer(context -> h, li.u.LowPart,
                     &li.u.HighPart, FILE_CURRENT);
      WCMD_batch (context->batchfileW, command, TRUE, gotoLabel, context->h);
      SetFilePointer(context -> h, li.u.LowPart,
                     &li.u.HighPart, FILE_BEGIN);

      /* Restore the for loop context */
      forloopcontext = oldcontext;
    } else {
      WCMD_output_asis_stderr(WCMD_LoadMessage(WCMD_CALLINSCRIPT));
    }
  }
}
