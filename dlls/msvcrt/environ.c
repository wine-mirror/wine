/*
 * msvcrt.dll environment functions
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 */
#include "wine/unicode.h"
#include "msvcrt.h"

DEFAULT_DEBUG_CHANNEL(msvcrt);

LPWSTR __cdecl wcsrchr( LPWSTR str, WCHAR ch );

/*********************************************************************
 *		getenv (MSVCRT.@)
 */
char *__cdecl MSVCRT_getenv(const char *name)
{
  LPSTR environ = GetEnvironmentStringsA();
  LPSTR pp,pos = NULL;
  unsigned int length;

  for (pp = environ; (*pp); pp = pp + strlen(pp) +1)
  {
    pos =strchr(pp,'=');
    if (pos)
      length = pos -pp;
    else
      length = strlen(pp);
    if (!strncmp(pp,name,length)) break;
  }
  if ((pp)&& (pos))
  {
     pp = pos+1;
     TRACE("got %s\n",pp);
  }
  FreeEnvironmentStringsA( environ );
  return pp;
}

/*********************************************************************
 *		_wgetenv (MSVCRT.@)
 */
WCHAR *__cdecl MSVCRT__wgetenv(const WCHAR *name)
{
  WCHAR* environ = GetEnvironmentStringsW();
  WCHAR* pp,*pos = NULL;
  unsigned int length;

  for (pp = environ; (*pp); pp = pp + strlenW(pp) + 1)
  {
    pos =wcsrchr(pp,'=');
    if (pos)
      length = pos -pp;
    else
      length = strlenW(pp);
    if (!strncmpW(pp,name,length)) break;
  }
  if ((pp)&& (pos))
  {
     pp = pos+1;
     TRACE("got %s\n",debugstr_w(pp));
  }
  FreeEnvironmentStringsW( environ );
  return pp;
}
