/*
 * msvcrt.dll dll data items
 *
 * Copyright 2000 Jon Griffiths
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

#include "config.h"
#include "wine/port.h"

#include <math.h>
#include "msvcrt.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

int MSVCRT___argc = 0;
unsigned int MSVCRT_basemajor = 0;/* FIXME: */
unsigned int MSVCRT_baseminor = 0;/* FIXME: */
unsigned int MSVCRT_baseversion = 0; /* FIXME: */
unsigned int MSVCRT__commode = 0;
unsigned int MSVCRT__fmode = 0;
unsigned int MSVCRT_osmajor = 0;/* FIXME: */
unsigned int MSVCRT_osminor = 0;/* FIXME: */
unsigned int MSVCRT_osmode = 0;/* FIXME: */
unsigned int MSVCRT__osver = 0;
unsigned int MSVCRT__osplatform = 0;
unsigned int MSVCRT_osversion = 0; /* FIXME: */
unsigned int MSVCRT__winmajor = 0;
unsigned int MSVCRT__winminor = 0;
unsigned int MSVCRT__winver = 0;
unsigned int MSVCRT___setlc_active = 0;
unsigned int MSVCRT___unguarded_readlc_active = 0;
double MSVCRT__HUGE = 0;
char **MSVCRT___argv = NULL;
MSVCRT_wchar_t **MSVCRT___wargv = NULL;
char *MSVCRT__acmdln = NULL;
MSVCRT_wchar_t *MSVCRT__wcmdln = NULL;
char **MSVCRT__environ = NULL;
MSVCRT_wchar_t **MSVCRT__wenviron = NULL;
char **MSVCRT___initenv = NULL;
MSVCRT_wchar_t **MSVCRT___winitenv = NULL;
int MSVCRT_app_type = 0;
char* MSVCRT__pgmptr = NULL;
WCHAR* MSVCRT__wpgmptr = NULL;

/* Get a snapshot of the current environment
 * and construct the __p__environ array
 *
 * The pointer returned from GetEnvironmentStrings may get invalid when
 * some other module cause a reallocation of the env-variable block
 *
 * blk is an array of pointers to environment strings, ending with a NULL
 * and after that the actual copy of the environment strings, ending in a \0
 */
char ** msvcrt_SnapshotOfEnvironmentA(char **blk)
{
  char* environ_strings = GetEnvironmentStringsA();
  int count = 1, len = 1, i = 0; /* keep space for the trailing NULLS */
  char *ptr;

  for (ptr = environ_strings; *ptr; ptr += strlen(ptr) + 1)
  {
    count++;
    len += strlen(ptr) + 1;
  }
  if (blk)
      blk = HeapReAlloc( GetProcessHeap(), 0, blk, count* sizeof(char*) + len );
  else
    blk = HeapAlloc(GetProcessHeap(), 0, count* sizeof(char*) + len );

  if (blk)
    {
      if (count)
	{
	  memcpy(&blk[count],environ_strings,len);
	  for (ptr = (char*) &blk[count]; *ptr; ptr += strlen(ptr) + 1)
	    {
	      blk[i++] = ptr;
	    }
	}
      blk[i] = NULL;
    }
  FreeEnvironmentStringsA(environ_strings);
  return blk;
}

MSVCRT_wchar_t ** msvcrt_SnapshotOfEnvironmentW(MSVCRT_wchar_t **wblk)
{
  MSVCRT_wchar_t* wenviron_strings = GetEnvironmentStringsW();
  int count = 1, len = 1, i = 0; /* keep space for the trailing NULLS */
  MSVCRT_wchar_t *wptr;

  for (wptr = wenviron_strings; *wptr; wptr += strlenW(wptr) + 1)
  {
    count++;
    len += strlenW(wptr) + 1;
  }
  if (wblk)
      wblk = HeapReAlloc( GetProcessHeap(), 0, wblk, count* sizeof(MSVCRT_wchar_t*) + len * sizeof(MSVCRT_wchar_t));
  else
    wblk = HeapAlloc(GetProcessHeap(), 0, count* sizeof(MSVCRT_wchar_t*) + len * sizeof(MSVCRT_wchar_t));
  if (wblk)
    {
      if (count)
	{
	  memcpy(&wblk[count],wenviron_strings,len * sizeof(MSVCRT_wchar_t));
	  for (wptr = (MSVCRT_wchar_t*)&wblk[count]; *wptr; wptr += strlenW(wptr) + 1)
	    {
	      wblk[i++] = wptr;
	    }
	}
      wblk[i] = NULL;
    }
  FreeEnvironmentStringsW(wenviron_strings);
  return wblk;
}

typedef void (CDECL *_INITTERMFUN)(void);
typedef int (CDECL *_INITTERM_E_FN)(void);

/***********************************************************************
 *		__p___argc (MSVCRT.@)
 */
int* CDECL __p___argc(void) { return &MSVCRT___argc; }

/***********************************************************************
 *		__p__commode (MSVCRT.@)
 */
unsigned int* CDECL __p__commode(void) { return &MSVCRT__commode; }


/***********************************************************************
 *              __p__pgmptr (MSVCRT.@)
 */
char** CDECL __p__pgmptr(void) { return &MSVCRT__pgmptr; }

/***********************************************************************
 *              __p__wpgmptr (MSVCRT.@)
 */
WCHAR** CDECL __p__wpgmptr(void) { return &MSVCRT__wpgmptr; }

/***********************************************************************
 *              _get_pgmptr (MSVCRT.@)
 */
int CDECL _get_pgmptr(char** p)
{
  if (!MSVCRT_CHECK_PMT(p)) return MSVCRT_EINVAL;

  *p = MSVCRT__pgmptr;
  return 0;
}

/***********************************************************************
 *              _get_wpgmptr (MSVCRT.@)
 */
int CDECL _get_wpgmptr(WCHAR** p)
{
  if (!MSVCRT_CHECK_PMT(p)) return MSVCRT_EINVAL;
  *p = MSVCRT__wpgmptr;
  return 0;
}

/***********************************************************************
 *		__p__fmode (MSVCRT.@)
 */
unsigned int* CDECL __p__fmode(void) { return &MSVCRT__fmode; }

/***********************************************************************
 *		__p__osver (MSVCRT.@)
 */
unsigned int* CDECL __p__osver(void) { return &MSVCRT__osver; }

/***********************************************************************
 *		__p__winmajor (MSVCRT.@)
 */
unsigned int* CDECL __p__winmajor(void) { return &MSVCRT__winmajor; }

/***********************************************************************
 *		__p__winminor (MSVCRT.@)
 */
unsigned int* CDECL __p__winminor(void) { return &MSVCRT__winminor; }

/***********************************************************************
 *		__p__winver (MSVCRT.@)
 */
unsigned int* CDECL __p__winver(void) { return &MSVCRT__winver; }

/*********************************************************************
 *		__p__acmdln (MSVCRT.@)
 */
char** CDECL __p__acmdln(void) { return &MSVCRT__acmdln; }

/*********************************************************************
 *		__p__wcmdln (MSVCRT.@)
 */
MSVCRT_wchar_t** CDECL __p__wcmdln(void) { return &MSVCRT__wcmdln; }

/*********************************************************************
 *		__p___argv (MSVCRT.@)
 */
char*** CDECL __p___argv(void) { return &MSVCRT___argv; }

/*********************************************************************
 *		__p___wargv (MSVCRT.@)
 */
MSVCRT_wchar_t*** CDECL __p___wargv(void) { return &MSVCRT___wargv; }

/*********************************************************************
 *		__p__environ (MSVCRT.@)
 */
char*** CDECL MSVCRT___p__environ(void)
{
  return &MSVCRT__environ;
}

/*********************************************************************
 *		__p__wenviron (MSVCRT.@)
 */
MSVCRT_wchar_t*** CDECL MSVCRT___p__wenviron(void)
{
  return &MSVCRT__wenviron;
}

/*********************************************************************
 *		__p___initenv (MSVCRT.@)
 */
char*** CDECL __p___initenv(void) { return &MSVCRT___initenv; }

/*********************************************************************
 *		__p___winitenv (MSVCRT.@)
 */
MSVCRT_wchar_t*** CDECL __p___winitenv(void) { return &MSVCRT___winitenv; }

/*********************************************************************
 *		_get_osplatform (MSVCRT.@)
 */
int CDECL MSVCRT__get_osplatform(int *pValue)
{
    if (!MSVCRT_CHECK_PMT(pValue != NULL)) return MSVCRT_EINVAL;
    *pValue = MSVCRT__osplatform;
    return 0;
}

/* INTERNAL: Create a wide string from an ascii string */
MSVCRT_wchar_t *msvcrt_wstrdupa(const char *str)
{
  const unsigned int len = strlen(str) + 1 ;
  MSVCRT_wchar_t *wstr = MSVCRT_malloc(len* sizeof (MSVCRT_wchar_t));
  if (!wstr)
    return NULL;
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,str,len,wstr,len);
  return wstr;
}

/*********************************************************************
 *		___unguarded_readlc_active_add_func (MSVCRT.@)
 */
unsigned int * CDECL MSVCRT____unguarded_readlc_active_add_func(void)
{
  return &MSVCRT___unguarded_readlc_active;
}

/*********************************************************************
 *		___setlc_active_func (MSVCRT.@)
 */
unsigned int CDECL MSVCRT____setlc_active_func(void)
{
  return MSVCRT___setlc_active;
}

/* INTERNAL: Since we can't rely on Winelib startup code calling w/getmainargs,
 * we initialise data values during DLL loading. When called by a native
 * program we simply return the data we've already initialised. This also means
 * you can call multiple times without leaking
 */
void msvcrt_init_args(void)
{
  OSVERSIONINFOW osvi;

  MSVCRT__acmdln = MSVCRT__strdup( GetCommandLineA() );
  MSVCRT__wcmdln = msvcrt_wstrdupa(MSVCRT__acmdln);
  MSVCRT___argc = __wine_main_argc;
  MSVCRT___argv = __wine_main_argv;
  MSVCRT___wargv = __wine_main_wargv;

  TRACE("got %s, wide = %s argc=%d\n", debugstr_a(MSVCRT__acmdln),
        debugstr_w(MSVCRT__wcmdln),MSVCRT___argc);

  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
  GetVersionExW( &osvi );
  MSVCRT__winver     = (osvi.dwMajorVersion << 8) | osvi.dwMinorVersion;
  MSVCRT__winmajor   = osvi.dwMajorVersion;
  MSVCRT__winminor   = osvi.dwMinorVersion;
  MSVCRT__osver      = osvi.dwBuildNumber;
  MSVCRT__osplatform = osvi.dwPlatformId;
  MSVCRT_osversion   = MSVCRT__winver;
  MSVCRT_osmajor     = MSVCRT__winmajor;
  MSVCRT_osminor     = MSVCRT__winminor;
  MSVCRT_baseversion = MSVCRT__osver;
  MSVCRT_baseminor   = MSVCRT_baseversion & 0xFF;
  MSVCRT_basemajor   = (MSVCRT_baseversion >> 8) & 0xFF;
  TRACE( "winver %08x winmajor %08x winminor %08x osver%08x baseversion %08x basemajor %08x baseminor %08x\n",
          MSVCRT__winver, MSVCRT__winmajor, MSVCRT__winminor, MSVCRT__osver, MSVCRT_baseversion,
          MSVCRT_basemajor, MSVCRT_baseminor);
  TRACE( "osversion %08x osmajor %08x osminor %08x\n", MSVCRT_osversion, MSVCRT_osmajor, MSVCRT_osminor);

  MSVCRT__HUGE = HUGE_VAL;
  MSVCRT___setlc_active = 0;
  MSVCRT___unguarded_readlc_active = 0;
  MSVCRT__fmode = MSVCRT__O_TEXT;

  MSVCRT__environ = msvcrt_SnapshotOfEnvironmentA(NULL);
  MSVCRT___initenv = msvcrt_SnapshotOfEnvironmentA(NULL);
  MSVCRT___winitenv = msvcrt_SnapshotOfEnvironmentW(NULL);

  MSVCRT__pgmptr = HeapAlloc(GetProcessHeap(), 0, MAX_PATH);
  if (MSVCRT__pgmptr)
  {
    if (!GetModuleFileNameA(0, MSVCRT__pgmptr, MAX_PATH))
      MSVCRT__pgmptr[0] = '\0';
    else
      MSVCRT__pgmptr[MAX_PATH - 1] = '\0';
  }

  MSVCRT__wpgmptr = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
  if (MSVCRT__wpgmptr)
  {
    if (!GetModuleFileNameW(0, MSVCRT__wpgmptr, MAX_PATH))
      MSVCRT__wpgmptr[0] = '\0';
    else
      MSVCRT__wpgmptr[MAX_PATH - 1] = '\0';
  }
}


/* INTERNAL: free memory used by args */
void msvcrt_free_args(void)
{
  /* FIXME: more things to free */
  HeapFree(GetProcessHeap(), 0, MSVCRT___initenv);
  HeapFree(GetProcessHeap(), 0, MSVCRT___winitenv);
  HeapFree(GetProcessHeap(), 0, MSVCRT__environ);
  HeapFree(GetProcessHeap(), 0, MSVCRT__wenviron);
  HeapFree(GetProcessHeap(), 0, MSVCRT__pgmptr);
  HeapFree(GetProcessHeap(), 0, MSVCRT__wpgmptr);
}

/*********************************************************************
 *		__getmainargs (MSVCRT.@)
 */
void CDECL __getmainargs(int *argc, char** *argv, char** *envp,
                         int expand_wildcards, int *new_mode)
{
  TRACE("(%p,%p,%p,%d,%p).\n", argc, argv, envp, expand_wildcards, new_mode);
  *argc = MSVCRT___argc;
  *argv = MSVCRT___argv;
  *envp = MSVCRT___initenv;
  if (new_mode)
    MSVCRT__set_new_mode( *new_mode );
}

/*********************************************************************
 *		__wgetmainargs (MSVCRT.@)
 */
void CDECL __wgetmainargs(int *argc, MSVCRT_wchar_t** *wargv, MSVCRT_wchar_t** *wenvp,
                          int expand_wildcards, int *new_mode)
{
  TRACE("(%p,%p,%p,%d,%p).\n", argc, wargv, wenvp, expand_wildcards, new_mode);

  /* Initialize the _wenviron array if it's not already created. */
  if (!MSVCRT__wenviron)
    MSVCRT__wenviron = msvcrt_SnapshotOfEnvironmentW(NULL);
  *argc = MSVCRT___argc;
  *wargv = MSVCRT___wargv;
  *wenvp = MSVCRT___winitenv;
  if (new_mode)
    MSVCRT__set_new_mode( *new_mode );
}

/*********************************************************************
 *		_initterm (MSVCRT.@)
 */
void CDECL _initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
  _INITTERMFUN* current = start;

  TRACE("(%p,%p)\n",start,end);
  while (current<end)
  {
    if (*current)
    {
      TRACE("Call init function %p\n",*current);
      (**current)();
      TRACE("returned\n");
    }
    current++;
  }
}

/*********************************************************************
 *  _initterm_e (MSVCRT.@)
 *
 * call an array of application initialization functions and report the return value
 */
int CDECL _initterm_e(_INITTERM_E_FN *table, _INITTERM_E_FN *end)
{
    int res = 0;

    TRACE("(%p, %p)\n", table, end);

    while (!res && table < end) {
        if (*table) {
            TRACE("calling %p\n", **table);
            res = (**table)();
            if (res)
                TRACE("function %p failed: 0x%x\n", *table, res);

        }
        table++;
    }
    return res;
}

/*********************************************************************
 *		__set_app_type (MSVCRT.@)
 */
void CDECL MSVCRT___set_app_type(int app_type)
{
  TRACE("(%d) %s application\n", app_type, app_type == 2 ? "Gui" : "Console");
  MSVCRT_app_type = app_type;
}
