/*
 * msvcrt.dll dll data items
 *
 * Copyright 2000 Jon Griffiths
 */
#include <math.h>
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

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
unsigned int MSVCRT__sys_nerr;
unsigned int MSVCRT___setlc_active;
unsigned int MSVCRT___unguarded_readlc_active;
double MSVCRT__HUGE;
char **MSVCRT___argv;
WCHAR **MSVCRT___wargv;
char *MSVCRT__acmdln;
WCHAR *MSVCRT__wcmdln;
char *MSVCRT__environ;
WCHAR *MSVCRT__wenviron;
char **MSVCRT___initenv;
WCHAR **MSVCRT___winitenv;
int MSVCRT_timezone;
int MSVCRT_app_type;

typedef void (__cdecl *MSVCRT__INITTERMFUN)(void);

/***********************************************************************
 *		__p___argc (MSVCRT.@)
 */
unsigned int *__cdecl MSVCRT___p___argc(void) { return &MSVCRT___argc; }

/***********************************************************************
 *		__p__commode (MSVCRT.@)
 */
unsigned int *__cdecl MSVCRT___p__commode(void) { return &MSVCRT__commode; }

/***********************************************************************
 *		__p__fmode (MSVCRT.@)
 */
unsigned int *__cdecl MSVCRT___p__fmode(void) { return &MSVCRT__fmode; }

/***********************************************************************
 *		__p__osver (MSVCRT.@)
 */
unsigned int *__cdecl MSVCRT___p__osver(void) { return &MSVCRT__osver; }

/***********************************************************************
 *		__p__winmajor (MSVCRT.@)
 */
unsigned int *__cdecl MSVCRT___p__winmajor(void) { return &MSVCRT__winmajor; }

/***********************************************************************
 *		__p__winminor (MSVCRT.@)
 */
unsigned int *__cdecl MSVCRT___p__winminor(void) { return &MSVCRT__winminor; }

/***********************************************************************
 *		__p__winver (MSVCRT.@)
 */
unsigned int *__cdecl MSVCRT___p__winver(void) { return &MSVCRT__winver; }

/*********************************************************************
 *		__p__acmdln (MSVCRT.@)
 */
char **__cdecl MSVCRT___p__acmdln(void) { return &MSVCRT__acmdln; }

/*********************************************************************
 *		__p__wcmdln (MSVCRT.@)
 */
WCHAR **__cdecl MSVCRT___p__wcmdln(void) { return &MSVCRT__wcmdln; }

/*********************************************************************
 *		__p___argv (MSVCRT.@)
 */
char ***__cdecl MSVCRT___p___argv(void) { return &MSVCRT___argv; }

/*********************************************************************
 *		__p___wargv (MSVCRT.@)
 */
WCHAR ***__cdecl MSVCRT___p___wargv(void) { return &MSVCRT___wargv; }

/*********************************************************************
 *		__p__environ (MSVCRT.@)
 */
char **__cdecl MSVCRT___p__environ(void) { return &MSVCRT__environ; }

/*********************************************************************
 *		__p__wenviron (MSVCRT.@)
 */
WCHAR **__cdecl MSVCRT___p__wenviron(void) { return &MSVCRT__wenviron; }

/*********************************************************************
 *		__p___initenv (MSVCRT.@)
 */
char ***__cdecl MSVCRT___p___initenv(void) { return &MSVCRT___initenv; }

/*********************************************************************
 *		__p___winitenv (MSVCRT.@)
 */
WCHAR ***__cdecl MSVCRT___p___winitenv(void) { return &MSVCRT___winitenv; }

/*********************************************************************
 *		__p__timezone (MSVCRT.@)
 */
int *__cdecl MSVCRT___p__timezone(void) { return &MSVCRT_timezone; }

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
 * we initialise data values during DLL loading. The when called by a native
 * program we simply return the data we've already initialised. This also means
 * you can call multiple times without leaking
 */
void MSVCRT_init_args(void)
{
  char *cmdline, **xargv = NULL;
  WCHAR *wcmdline, **wxargv = NULL;
  int xargc,end,last_arg,afterlastspace;
  DWORD version;

  MSVCRT__acmdln = cmdline = MSVCRT__strdup( GetCommandLineA() );
  MSVCRT__wcmdln = wcmdline = wstrdupa(cmdline);
  TRACE("got '%s', wide = '%s'\n", cmdline, debugstr_w(wcmdline));

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

  end = last_arg = xargc = afterlastspace = 0;
  while (1)
  {
    if ((cmdline[end]==' ') || (cmdline[end]=='\0'))
    {
      if (cmdline[end]=='\0')
        last_arg=1;
      else
        cmdline[end]='\0';
      /* alloc xargc + NULL entry */
      xargv=(char**)HeapReAlloc( GetProcessHeap(), 0, xargv,
                                sizeof(char*)*(xargc+1));
      wxargv=(WCHAR**)HeapReAlloc( GetProcessHeap(), 0, wxargv,
                                  sizeof(WCHAR*)*(xargc+1));

      if (strlen(cmdline+afterlastspace))
      {
        xargv[xargc] = MSVCRT__strdup(cmdline+afterlastspace);
        wxargv[xargc] = wstrdupa(xargv[xargc]);
        xargc++;
        if (!last_arg) /* need to seek to the next arg ? */
        {
          end++;
          while (cmdline[end]==' ')
            end++;
        }
        afterlastspace=end;
      }
      else
      {
        xargv[xargc] = NULL;
        wxargv[xargc] = NULL; /* the last entry is NULL */
        break;
      }
    }
    else
      end++;
  }
  MSVCRT___argc = xargc;
  MSVCRT___argv = xargv;
  MSVCRT___wargv = wxargv;

  TRACE("found %d arguments\n",MSVCRT___argc);
  MSVCRT__environ = GetEnvironmentStringsA();
  MSVCRT___initenv = &MSVCRT__environ;
  MSVCRT__wenviron = GetEnvironmentStringsW();
  MSVCRT___winitenv = &MSVCRT__wenviron;
}


/* INTERNAL: free memory used by args */
void MSVCRT_free_args(void)
{
  /* FIXME */
}

/*********************************************************************
 *		__getmainargs (MSVCRT.@)
 */
char** __cdecl MSVCRT___getmainargs(DWORD *argc,char ***argv,char **environ,DWORD flag)
{
  TRACE("(%p,%p,%p,%ld).\n",argc,argv,environ,flag);
  *argc = MSVCRT___argc;
  *argv = MSVCRT___argv;
  *environ = MSVCRT__environ;
  return environ;
}

/*********************************************************************
 *		__wgetmainargs (MSVCRT.@)
 */
WCHAR** __cdecl MSVCRT___wgetmainargs(DWORD *argc,WCHAR ***wargv,WCHAR **wenviron,DWORD flag)
{
  TRACE("(%p,%p,%p,%ld).\n",argc,wargv,wenviron,flag);
  *argc = MSVCRT___argc;
  *wargv = MSVCRT___wargv;
  *wenviron = MSVCRT__wenviron;
  return wenviron;
}

/*********************************************************************
 *		_initterm (MSVCRT.@)
 */
unsigned int __cdecl MSVCRT__initterm(MSVCRT__INITTERMFUN *start,MSVCRT__INITTERMFUN *end)
{
  MSVCRT__INITTERMFUN*current = start;

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
void __cdecl MSVCRT___set_app_type(int app_type)
{
  TRACE("(%d) %s application\n", app_type, app_type == 2 ? "Gui" : "Console");
  MSVCRT_app_type = app_type;
}
