/*
 * CMD - Wine-compatible command line interface - batch interface.
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

#include "wcmd.h"

extern int echo_mode;
extern WCHAR quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];
extern BATCH_CONTEXT *context;
extern DWORD errorlevel;

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

void WCMD_batch (WCHAR *file, WCHAR *command, int called, WCHAR *startLabel, HANDLE pgmHandle) {

#define WCMD_BATCH_EXT_SIZE 5

  HANDLE h = INVALID_HANDLE_VALUE;
  WCHAR string[MAXSTRING];
  static const WCHAR extension_batch[][WCMD_BATCH_EXT_SIZE] = {{'.','b','a','t','\0'},
                                                               {'.','c','m','d','\0'}};
  static const WCHAR extension_exe[WCMD_BATCH_EXT_SIZE] = {'.','e','x','e','\0'};
  unsigned int  i;
  BATCH_CONTEXT *prev_context;

  if (startLabel == NULL) {
    for(i=0; (i<((sizeof(extension_batch) * sizeof(WCHAR))/WCMD_BATCH_EXT_SIZE)) &&
             (h == INVALID_HANDLE_VALUE); i++) {
      strcpyW (string, file);
      CharLower (string);
      if (strstrW (string, extension_batch[i]) == NULL) strcatW (string, extension_batch[i]);
      h = CreateFile (string, GENERIC_READ, FILE_SHARE_READ,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    if (h == INVALID_HANDLE_VALUE) {
      strcpyW (string, file);
      CharLower (string);
      if (strstrW (string, extension_exe) == NULL) strcatW (string, extension_exe);
      h = CreateFile (string, GENERIC_READ, FILE_SHARE_READ,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      if (h != INVALID_HANDLE_VALUE) {
        WCMD_run_program (command, 0);
      } else {
        SetLastError (ERROR_FILE_NOT_FOUND);
        WCMD_print_error ();
      }
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
  context = (BATCH_CONTEXT *)LocalAlloc (LMEM_FIXED, sizeof (BATCH_CONTEXT));
  context -> h = h;
  context -> command = command;
  memset(context -> shift_count, 0x00, sizeof(context -> shift_count));
  context -> prev_context = prev_context;
  context -> skip_rest = FALSE;

  /* If processing a call :label, 'goto' the label in question */
  if (startLabel) {
    strcpyW(param1, startLabel);
    WCMD_goto(NULL);
  }

/*
 * 	Work through the file line by line. Specific batch commands are processed here,
 * 	the rest are handled by the main command processor.
 */

  while (context -> skip_rest == FALSE) {
      CMD_LIST *toExecute = NULL;         /* Commands left to be executed */
      if (WCMD_ReadAndParseLine(NULL, &toExecute, h) == NULL)
        break;
      WCMD_process_commands(toExecute);
      WCMD_free_commands(toExecute);
      toExecute = NULL;
  }
  CloseHandle (h);

/*
 *	If invoked by a CALL, we return to the context of our caller. Otherwise return
 *	to the caller's caller.
 */

  LocalFree ((HANDLE)context);
  if ((prev_context != NULL) && (!called)) {
    CloseHandle (prev_context -> h);
    context = prev_context -> prev_context;
    LocalFree ((HANDLE)prev_context);
  }
  else {
    context = prev_context;
  }
}

/*******************************************************************
 * WCMD_parameter - extract a parameter from a command line.
 *
 *	Returns the 'n'th space-delimited parameter on the command line (zero-based).
 *	Parameter is in static storage overwritten on the next call.
 *	Parameters in quotes (and brackets) are handled.
 *	Also returns a pointer to the location of the parameter in the command line.
 */

WCHAR *WCMD_parameter (WCHAR *s, int n, WCHAR **where) {

  int i = 0;
  static WCHAR param[MAX_PATH];
  WCHAR *p;

  if (where != NULL) *where = NULL;
  p = param;
  while (TRUE) {
    switch (*s) {
      case ' ':
	s++;
	break;
      case '"':
        if (where != NULL && i==n) *where = s;
	s++;
	while ((*s != '\0') && (*s != '"')) {
	  *p++ = *s++;
	}
        if (i == n) {
          *p = '\0';
          return param;
        }
	if (*s == '"') s++;
          param[0] = '\0';
          i++;
        p = param;
	break;
      case '(':
        if (where != NULL && i==n) *where = s;
	s++;
	while ((*s != '\0') && (*s != ')')) {
	  *p++ = *s++;
	}
        if (i == n) {
          *p = '\0';
          return param;
        }
	if (*s == ')') s++;
          param[0] = '\0';
          i++;
        p = param;
	break;
      case '\0':
        return param;
      default:
        /* Only return where if it is for the right parameter */
        if (where != NULL && i==n) *where = s;
	while ((*s != '\0') && (*s != ' ')) {
	  *p++ = *s++;
	}
        if (i == n) {
          *p = '\0';
          return param;
        }
          param[0] = '\0';
          i++;
        p = param;
    }
  }
}

/****************************************************************************
 * WCMD_fgets
 *
 * Get one line from a batch file. We can't use the native f* functions because
 * of the filename syntax differences between DOS and Unix. Also need to lose
 * the LF (or CRLF) from the line.
 */

WCHAR *WCMD_fgets (WCHAR *s, int noChars, HANDLE h) {

  DWORD bytes;
  BOOL status;
  WCHAR *p;

  p = s;
  do {
    status = WCMD_ReadFile (h, s, 1, &bytes, NULL);
    if ((status == 0) || ((bytes == 0) && (s == p))) return NULL;
    if (*s == '\n') bytes = 0;
    else if (*s != '\r') {
      s++;
      noChars--;
    }
    *s = '\0';
  } while ((bytes == 1) && (noChars > 1));
  return p;
}

/* WCMD_splitpath - copied from winefile as no obvious way to use it otherwise */
void WCMD_splitpath(const WCHAR* path, WCHAR* drv, WCHAR* dir, WCHAR* name, WCHAR* ext)
{
        const WCHAR* end; /* end of processed string */
	const WCHAR* p;	 /* search pointer */
	const WCHAR* s;	 /* copy pointer */

	/* extract drive name */
	if (path[0] && path[1]==':') {
		if (drv) {
			*drv++ = *path++;
			*drv++ = *path++;
			*drv = '\0';
		}
	} else if (drv)
		*drv = '\0';

	/* search for end of string or stream separator */
	for(end=path; *end && *end!=':'; )
		end++;

	/* search for begin of file extension */
	for(p=end; p>path && *--p!='\\' && *p!='/'; )
		if (*p == '.') {
			end = p;
			break;
		}

	if (ext)
		for(s=end; (*ext=*s++); )
			ext++;

	/* search for end of directory name */
	for(p=end; p>path; )
		if (*--p=='\\' || *p=='/') {
			p++;
			break;
		}

	if (name) {
		for(s=p; s<end; )
			*name++ = *s++;

		*name = '\0';
	}

	if (dir) {
		for(s=path; s<p; )
			*dir++ = *s++;

		*dir = '\0';
	}
}

/****************************************************************************
 * WCMD_HandleTildaModifiers
 *
 * Handle the ~ modifiers when expanding %0-9 or (%a-z in for command)
 *    %~xxxxxV  (V=0-9 or A-Z)
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
void WCMD_HandleTildaModifiers(WCHAR **start, WCHAR *forVariable) {

#define NUMMODIFIERS 11
  static const WCHAR validmodifiers[NUMMODIFIERS] = {
        '~', 'f', 'd', 'p', 'n', 'x', 's', 'a', 't', 'z', '$'
  };
  static const WCHAR space[] = {' ', '\0'};

  WIN32_FILE_ATTRIBUTE_DATA fileInfo;
  WCHAR  outputparam[MAX_PATH];
  WCHAR  finaloutput[MAX_PATH];
  WCHAR  fullfilename[MAX_PATH];
  WCHAR  thisoutput[MAX_PATH];
  WCHAR  *pos            = *start+1;
  WCHAR  *firstModifier  = pos;
  WCHAR  *lastModifier   = NULL;
  int   modifierLen     = 0;
  BOOL  finished        = FALSE;
  int   i               = 0;
  BOOL  exists          = TRUE;
  BOOL  skipFileParsing = FALSE;
  BOOL  doneModifier    = FALSE;

  /* Search forwards until find invalid WCHARacter modifier */
  while (!finished) {

    /* Work on the previous WCHARacter */
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

  /* Now make sure the position we stopped at is a valid parameter */
  if (!(*lastModifier >= '0' || *lastModifier <= '9') &&
      (forVariable != NULL) &&
      (toupperW(*lastModifier) != toupperW(*forVariable)))  {

    /* Its not... Step backwards until it matches or we get to the start */
    while (toupperW(*lastModifier) != toupperW(*forVariable) &&
          lastModifier > firstModifier) {
      lastModifier--;
    }
    if (lastModifier == firstModifier) return; /* Invalid syntax */
  }

  /* Extract the parameter to play with */
  if ((*lastModifier >= '0' && *lastModifier <= '9')) {
    strcpyW(outputparam, WCMD_parameter (context -> command,
                 *lastModifier-'0' + context -> shift_count[*lastModifier-'0'], NULL));
  } else {
    /* FIXME: Retrieve 'for' variable %c\n", *lastModifier); */
    /* Need to get 'for' loop variable into outputparam      */
    return;
  }

  /* So now, firstModifier points to beginning of modifiers, lastModifier
     points to the variable just after the modifiers. Process modifiers
     in a specific order, remembering there could be duplicates           */
  modifierLen = lastModifier - firstModifier;
  finaloutput[0] = 0x00;

  /* Useful for debugging purposes: */
  /*printf("Modifier string '%*.*s' and variable is %c\n Param starts as '%s'\n",
             (modifierLen), (modifierLen), firstModifier, *lastModifier,
             outputparam);*/

  /* 1. Handle '~' : Strip surrounding quotes */
  if (outputparam[0]=='"' &&
      memchrW(firstModifier, '~', modifierLen) != NULL) {
    int len = strlenW(outputparam);
    if (outputparam[len-1] == '"') {
        outputparam[len-1]=0x00;
        len = len - 1;
    }
    memmove(outputparam, &outputparam[1], (len * sizeof(WCHAR))-1);
  }

  /* 2. Handle the special case of a $ */
  if (memchrW(firstModifier, '$', modifierLen) != NULL) {
    /* Special Case: Search envar specified in $[envvar] for outputparam
       Note both $ and : are guaranteed otherwise check above would fail */
    WCHAR *start = strchrW(firstModifier, '$') + 1;
    WCHAR *end   = strchrW(firstModifier, ':');
    WCHAR env[MAX_PATH];
    WCHAR fullpath[MAX_PATH];

    /* Extract the env var */
    memcpy(env, start, (end-start) * sizeof(WCHAR));
    env[(end-start)] = 0x00;

    /* If env var not found, return emptry string */
    if ((GetEnvironmentVariable(env, fullpath, MAX_PATH) == 0) ||
        (SearchPath(fullpath, outputparam, NULL,
                    MAX_PATH, outputparam, NULL) == 0)) {
      finaloutput[0] = 0x00;
      outputparam[0] = 0x00;
      skipFileParsing = TRUE;
    }
  }

  /* After this, we need full information on the file,
    which is valid not to exist.  */
  if (!skipFileParsing) {
    if (GetFullPathName(outputparam, MAX_PATH, fullfilename, NULL) == 0)
      return;

    exists = GetFileAttributesExW(fullfilename, GetFileExInfoStandard,
                                  &fileInfo);

    /* 2. Handle 'a' : Output attributes */
    if (exists &&
        memchrW(firstModifier, 'a', modifierLen) != NULL) {

      WCHAR defaults[] = {'-','-','-','-','-','-','-','-','-','\0'};
      doneModifier = TRUE;
      strcpyW(thisoutput, defaults);
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
      strcatW(finaloutput, thisoutput);
    }

    /* 3. Handle 't' : Date+time */
    if (exists &&
        memchrW(firstModifier, 't', modifierLen) != NULL) {

      SYSTEMTIME systime;
      int datelen;

      doneModifier = TRUE;
      if (finaloutput[0] != 0x00) strcatW(finaloutput, space);

      /* Format the time */
      FileTimeToSystemTime(&fileInfo.ftLastWriteTime, &systime);
      GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime,
                        NULL, thisoutput, MAX_PATH);
      strcatW(thisoutput, space);
      datelen = strlenW(thisoutput);
      GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &systime,
                        NULL, (thisoutput+datelen), MAX_PATH-datelen);
      strcatW(finaloutput, thisoutput);
    }

    /* 4. Handle 'z' : File length */
    if (exists &&
        memchrW(firstModifier, 'z', modifierLen) != NULL) {
      /* FIXME: Output full 64 bit size (sprintf does not support I64 here) */
      ULONG/*64*/ fullsize = /*(fileInfo.nFileSizeHigh << 32) +*/
                                  fileInfo.nFileSizeLow;
      static const WCHAR fmt[] = {'%','u','\0'};

      doneModifier = TRUE;
      if (finaloutput[0] != 0x00) strcatW(finaloutput, space);
      wsprintf(thisoutput, fmt, fullsize);
      strcatW(finaloutput, thisoutput);
    }

    /* 4. Handle 's' : Use short paths (File doesn't have to exist) */
    if (memchrW(firstModifier, 's', modifierLen) != NULL) {
      if (finaloutput[0] != 0x00) strcatW(finaloutput, space);
      /* Don't flag as doneModifier - %~s on its own is processed later */
      GetShortPathName(outputparam, outputparam, sizeof(outputparam));
    }

    /* 5. Handle 'f' : Fully qualified path (File doesn't have to exist) */
    /*      Note this overrides d,p,n,x                                 */
    if (memchrW(firstModifier, 'f', modifierLen) != NULL) {
      doneModifier = TRUE;
      if (finaloutput[0] != 0x00) strcatW(finaloutput, space);
      strcatW(finaloutput, fullfilename);
    } else {

      WCHAR drive[10];
      WCHAR dir[MAX_PATH];
      WCHAR fname[MAX_PATH];
      WCHAR ext[MAX_PATH];
      BOOL doneFileModifier = FALSE;

      if (finaloutput[0] != 0x00) strcatW(finaloutput, space);

      /* Split into components */
      WCMD_splitpath(fullfilename, drive, dir, fname, ext);

      /* 5. Handle 'd' : Drive Letter */
      if (memchrW(firstModifier, 'd', modifierLen) != NULL) {
        strcatW(finaloutput, drive);
        doneModifier = TRUE;
        doneFileModifier = TRUE;
      }

      /* 6. Handle 'p' : Path */
      if (memchrW(firstModifier, 'p', modifierLen) != NULL) {
        strcatW(finaloutput, dir);
        doneModifier = TRUE;
        doneFileModifier = TRUE;
      }

      /* 7. Handle 'n' : Name */
      if (memchrW(firstModifier, 'n', modifierLen) != NULL) {
        strcatW(finaloutput, fname);
        doneModifier = TRUE;
        doneFileModifier = TRUE;
      }

      /* 8. Handle 'x' : Ext */
      if (memchrW(firstModifier, 'x', modifierLen) != NULL) {
        strcatW(finaloutput, ext);
        doneModifier = TRUE;
        doneFileModifier = TRUE;
      }

      /* If 's' but no other parameter, dump the whole thing */
      if (!doneFileModifier &&
          memchrW(firstModifier, 's', modifierLen) != NULL) {
        doneModifier = TRUE;
        if (finaloutput[0] != 0x00) strcatW(finaloutput, space);
        strcatW(finaloutput, outputparam);
      }
    }
  }

  /* If No other modifier processed,  just add in parameter */
  if (!doneModifier) strcpyW(finaloutput, outputparam);

  /* Finish by inserting the replacement into the string */
  pos = WCMD_strdupW(lastModifier+1);
  strcpyW(*start, finaloutput);
  strcatW(*start, pos);
  free(pos);
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
    WCMD_run_program(command, 1);
  } else {

    WCHAR gotoLabel[MAX_PATH];

    strcpyW(gotoLabel, param1);

    if (context) {

      LARGE_INTEGER li;

      /* Save the current file position, call the same file,
         restore position                                    */
      li.QuadPart = 0;
      li.u.LowPart = SetFilePointer(context -> h, li.u.LowPart,
                     &li.u.HighPart, FILE_CURRENT);

      WCMD_batch (param1, command, 1, gotoLabel, context->h);

      SetFilePointer(context -> h, li.u.LowPart,
                     &li.u.HighPart, FILE_BEGIN);
    } else {
      WCMD_output_asis( WCMD_LoadMessage(WCMD_CALLINSCRIPT));
    }
  }
}
