/*
 * Old C RunTime DLL - All functionality is provided by msvcrt.
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

#include <stdarg.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(crtdll);

/* from msvcrt */
extern void CDECL __getmainargs( int *argc, char ***argv, char ***envp,
                                 int expand_wildcards, int *new_mode );

/* The following data items are not exported from msvcrt */
unsigned int CRTDLL__basemajor_dll = 0;
unsigned int CRTDLL__baseminor_dll = 0;
unsigned int CRTDLL__baseversion_dll = 0;
unsigned int CRTDLL__cpumode_dll = 0;
unsigned int CRTDLL__osmajor_dll = 0;
unsigned int CRTDLL__osminor_dll = 0;
unsigned int CRTDLL__osmode_dll = 0;
unsigned int CRTDLL__osversion_dll = 0;

/* dev_t is a short in crtdll but an unsigned int in msvcrt */
typedef short crtdll_dev_t;

struct crtdll_stat
{
  crtdll_dev_t   st_dev;
  _ino_t         st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  crtdll_dev_t   st_rdev;
  _off_t         st_size;
  time_t         st_atime;
  time_t         st_mtime;
  time_t         st_ctime;
};

/* convert struct _stat from crtdll format to msvcrt format */
static void convert_struct_stat( struct crtdll_stat *dst, const struct _stat *src )
{
    dst->st_dev   = src->st_dev;
    dst->st_ino   = src->st_ino;
    dst->st_mode  = src->st_mode;
    dst->st_nlink = src->st_nlink;
    dst->st_uid   = src->st_uid;
    dst->st_gid   = src->st_gid;
    dst->st_rdev  = src->st_rdev;
    dst->st_size  = src->st_size;
    dst->st_atime = src->st_atime;
    dst->st_mtime = src->st_mtime;
    dst->st_ctime = src->st_ctime;
}


/*********************************************************************
 *                  DllMain  (CRTDLL.init)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
  TRACE("(%p,%d,%p)\n",hinstDLL,fdwReason,lpvReserved);

  if (fdwReason == DLL_PROCESS_ATTACH)
  {
    DWORD version = GetVersion();

    DisableThreadLibraryCalls(hinstDLL);

    CRTDLL__basemajor_dll   = (version >> 24) & 0xFF;
    CRTDLL__baseminor_dll   = (version >> 16) & 0xFF;
    CRTDLL__baseversion_dll = (version >> 16);
    CRTDLL__cpumode_dll     = 1; /* FIXME */
    CRTDLL__osmajor_dll     = (version>>8) & 0xFF;
    CRTDLL__osminor_dll     = (version & 0xFF);
    CRTDLL__osmode_dll      = 1; /* FIXME */
    CRTDLL__osversion_dll   = (version & 0xFFFF);
  }
  return TRUE;
}


/*********************************************************************
 *                  __GetMainArgs  (CRTDLL.@)
 */
void CDECL __GetMainArgs( int *argc, char ***argv, char ***envp, int expand_wildcards )
{
    int new_mode = 0;
    __getmainargs( argc, argv, envp, expand_wildcards, &new_mode );
}


/*********************************************************************
 *		_fstat (CRTDLL.@)
 */
int CDECL CRTDLL__fstat(int fd, struct crtdll_stat* buf)
{
    struct _stat st;
    int ret;

    if (!(ret = _fstat( fd, &st ))) convert_struct_stat( buf, &st );
    return ret;
}


/*********************************************************************
 *		_stat (CRTDLL.@)
 */
int CDECL CRTDLL__stat(const char* path, struct crtdll_stat * buf)
{
    struct _stat st;
    int ret;

    if (!(ret = _stat( path, &st ))) convert_struct_stat( buf, &st );
    return ret;
}


/*********************************************************************
 *		_strdec (CRTDLL.@)
 */
char * CDECL _strdec(const char *str1, const char *str2)
{
    return (char *)(str2 - 1);
}


/*********************************************************************
 *		_strinc (CRTDLL.@)
 */
char * CDECL _strinc(const char *str)
{
    return (char *)(str + 1);
}


/*********************************************************************
 *		_strncnt (CRTDLL.@)
 */
size_t CDECL _strncnt(const char *str, size_t maxlen)
{
    size_t len = strlen(str);
    return (len > maxlen) ? maxlen : len;
}


/*********************************************************************
 *		_strnextc (CRTDLL.@)
 */
unsigned int CDECL _strnextc(const char *str)
{
    return (unsigned char)str[0];
}


/*********************************************************************
 *		_strninc (CRTDLL.@)
 */
char * CDECL _strninc(const char *str, size_t len)
{
    return (char *)(str + len);
}


/*********************************************************************
 *		_strspnp (CRTDLL.@)
 */
char * CDECL _strspnp( const char *str1, const char *str2)
{
    str1 += strspn( str1, str2 );
    return *str1 ? (char*)str1 : NULL;
}
