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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <math.h>
#include "msvcrt.h"

#include "msvcrt/stdlib.h"
#include "msvcrt/string.h"

#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

unsigned int MSVCRT___argc;
unsigned int MSVCRT_basemajor;/* FIXME: */
unsigned int MSVCRT_baseminor;/* FIXME: */
unsigned int MSVCRT_baseversion; /* FIXME: */
unsigned int MSVCRT__commode;
unsigned int MSVCRT__fmode;
unsigned int MSVCRT_osmajor;/* FIXME: */
unsigned int MSVCRT_osminor;/* FIXME: */
unsigned int MSVCRT_osmode;/* FIXME: */
unsigned int MSVCRT__osver;
unsigned int MSVCRT_osversion; /* FIXME: */
unsigned int MSVCRT__winmajor;
unsigned int MSVCRT__winminor;
unsigned int MSVCRT__winver;
unsigned int MSVCRT__sys_nerr; /* FIXME: not accessible from Winelib apps */
char**       MSVCRT__sys_errlist; /* FIXME: not accessible from Winelib apps */
unsigned int MSVCRT___setlc_active;
unsigned int MSVCRT___unguarded_readlc_active;
double MSVCRT__HUGE;
char **MSVCRT___argv;
WCHAR **MSVCRT___wargv;
char *MSVCRT__acmdln;
WCHAR *MSVCRT__wcmdln;
char **MSVCRT__environ;
WCHAR **MSVCRT__wenviron;
char **MSVCRT___initenv;
WCHAR **MSVCRT___winitenv;
int MSVCRT_timezone;
int MSVCRT_app_type;

static char* environ_strings;
static WCHAR* wenviron_strings;

typedef void (*_INITTERMFUN)(void);

/***********************************************************************
 *		__p___argc (MSVCRT.@)
 */
int* __p___argc(void) { return &MSVCRT___argc; }

/***********************************************************************
 *		__p__commode (MSVCRT.@)
 */
unsigned int* __p__commode(void) { return &MSVCRT__commode; }

/***********************************************************************
 *		__p__fmode (MSVCRT.@)
 */
unsigned int* __p__fmode(void) { return &MSVCRT__fmode; }

/***********************************************************************
 *		__p__osver (MSVCRT.@)
 */
unsigned int* __p__osver(void) { return &MSVCRT__osver; }

/***********************************************************************
 *		__p__winmajor (MSVCRT.@)
 */
unsigned int* __p__winmajor(void) { return &MSVCRT__winmajor; }

/***********************************************************************
 *		__p__winminor (MSVCRT.@)
 */
unsigned int* __p__winminor(void) { return &MSVCRT__winminor; }

/***********************************************************************
 *		__p__winver (MSVCRT.@)
 */
unsigned int* __p__winver(void) { return &MSVCRT__winver; }

/*********************************************************************
 *		__p__acmdln (MSVCRT.@)
 */
char** __p__acmdln(void) { return &MSVCRT__acmdln; }

/*********************************************************************
 *		__p__wcmdln (MSVCRT.@)
 */
WCHAR** __p__wcmdln(void) { return &MSVCRT__wcmdln; }

/*********************************************************************
 *		__p___argv (MSVCRT.@)
 */
char*** __p___argv(void) { return &MSVCRT___argv; }

/*********************************************************************
 *		__p___wargv (MSVCRT.@)
 */
WCHAR*** __p___wargv(void) { return &MSVCRT___wargv; }

/*********************************************************************
 *		__p__environ (MSVCRT.@)
 */
char*** __p__environ(void) { return &MSVCRT__environ; }

/*********************************************************************
 *		__p__wenviron (MSVCRT.@)
 */
WCHAR*** __p__wenviron(void) { return &MSVCRT__wenviron; }

/*********************************************************************
 *		__p___initenv (MSVCRT.@)
 */
char*** __p___initenv(void) { return &MSVCRT___initenv; }

/*********************************************************************
 *		__p___winitenv (MSVCRT.@)
 */
WCHAR*** __p___winitenv(void) { return &MSVCRT___winitenv; }

/*********************************************************************
 *		__p__timezone (MSVCRT.@)
 */
int* __p__timezone(void) { return &MSVCRT_timezone; }

/* INTERNAL: Create a wide string from an ascii string */
static WCHAR *wstrdupa(const char *str)
{
  const size_t len = strlen(str) + 1 ;
  WCHAR *wstr = MSVCRT_malloc(len* sizeof (WCHAR));
  if (!wstr)
    return NULL;
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,str,len*sizeof(char),wstr,len* sizeof (WCHAR));
  return wstr;
}

/* INTERNAL: Since we can't rely on Winelib startup code calling w/getmainargs,
 * we initialise data values during DLL loading. When called by a native
 * program we simply return the data we've already initialised. This also means
 * you can call multiple times without leaking
 */
void msvcrt_init_args(void)
{
  char *ptr;
  WCHAR *wptr;
  int count;
  DWORD version;

  MSVCRT__acmdln = _strdup( GetCommandLineA() );
  MSVCRT__wcmdln = wstrdupa(MSVCRT__acmdln);
  MSVCRT___argc = __wine_main_argc;
  MSVCRT___argv = __wine_main_argv;
  MSVCRT___wargv = __wine_main_wargv;

  TRACE("got '%s', wide = %s argc=%d\n", MSVCRT__acmdln,
        debugstr_w(MSVCRT__wcmdln),MSVCRT___argc);

  version = GetVersion();
  MSVCRT__osver       = version >> 16;
  MSVCRT__winminor    = version & 0xFF;
  MSVCRT__winmajor    = (version>>8) & 0xFF;
  MSVCRT_baseversion = version >> 16;
  MSVCRT__winver     = ((version >> 8) & 0xFF) + ((version & 0xFF) << 8);
  MSVCRT_baseminor   = (version >> 16) & 0xFF;
  MSVCRT_basemajor   = (version >> 24) & 0xFF;
  MSVCRT_osversion   = version & 0xFFFF;
  MSVCRT_osminor     = version & 0xFF;
  MSVCRT_osmajor     = (version>>8) & 0xFF;
  MSVCRT__sys_nerr   = 43;
  MSVCRT__HUGE = HUGE_VAL;
  MSVCRT___setlc_active = 0;
  MSVCRT___unguarded_readlc_active = 0;
  MSVCRT_timezone = 0;

  /* FIXME: set app type for Winelib apps */

  environ_strings = GetEnvironmentStringsA();
  count = 1; /* for NULL sentinel */
  for (ptr = environ_strings; *ptr; ptr += strlen(ptr) + 1)
  {
    count++;
  }
  MSVCRT__environ = HeapAlloc(GetProcessHeap(), 0, count * sizeof(char*));
  if (MSVCRT__environ)
  {
    count = 0;
    for (ptr = environ_strings; *ptr; ptr += strlen(ptr) + 1)
    {
      MSVCRT__environ[count++] = ptr;
    }
    MSVCRT__environ[count] = NULL;
  }

  MSVCRT___initenv = MSVCRT__environ;

  wenviron_strings = GetEnvironmentStringsW();
  count = 1; /* for NULL sentinel */
  for (wptr = wenviron_strings; *wptr; wptr += lstrlenW(wptr) + 1)
  {
    count++;
  }
  MSVCRT__wenviron = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WCHAR*));
  if (MSVCRT__wenviron)
  {
    count = 0;
    for (wptr = wenviron_strings; *wptr; wptr += lstrlenW(wptr) + 1)
    {
      MSVCRT__wenviron[count++] = wptr;
    }
    MSVCRT__wenviron[count] = NULL;
  }
  MSVCRT___winitenv = MSVCRT__wenviron;
}


/* INTERNAL: free memory used by args */
void msvcrt_free_args(void)
{
  /* FIXME: more things to free */
  FreeEnvironmentStringsA(environ_strings);
  FreeEnvironmentStringsW(wenviron_strings);
}

/*********************************************************************
 *		__getmainargs (MSVCRT.@)
 */
void __getmainargs(int *argc, char** *argv, char** *envp,
                                  int expand_wildcards, int *new_mode)
{
  TRACE("(%p,%p,%p,%d,%p).\n", argc, argv, envp, expand_wildcards, new_mode);
  *argc = MSVCRT___argc;
  *argv = MSVCRT___argv;
  *envp = MSVCRT__environ;
  if (new_mode)
    MSVCRT__set_new_mode( *new_mode );
}

/*********************************************************************
 *		__wgetmainargs (MSVCRT.@)
 */
void __wgetmainargs(int *argc, WCHAR** *wargv, WCHAR** *wenvp,
                                   int expand_wildcards, int *new_mode)
{
  TRACE("(%p,%p,%p,%d,%p).\n", argc, wargv, wenvp, expand_wildcards, new_mode);
  *argc = MSVCRT___argc;
  *wargv = MSVCRT___wargv;
  *wenvp = MSVCRT__wenviron;
  if (new_mode)
    MSVCRT__set_new_mode( *new_mode );
}

/*********************************************************************
 *		_initterm (MSVCRT.@)
 */
unsigned int _initterm(_INITTERMFUN *start,_INITTERMFUN *end)
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
  return 0;
}

/*********************************************************************
 *		__set_app_type (MSVCRT.@)
 */
void MSVCRT___set_app_type(int app_type)
{
  TRACE("(%d) %s application\n", app_type, app_type == 2 ? "Gui" : "Console");
  MSVCRT_app_type = app_type;
}
