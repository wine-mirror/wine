/*
 * Win32 advapi functions
 *
 * Copyright 1995 Sven Verdoolaege
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif

#include "winbase.h"
#include "windef.h"
#include "winnls.h"
#include "winerror.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);

/******************************************************************************
 * GetUserNameA [ADVAPI32.@]
 *
 * NOTE: lpSize returns the total length of the username, including the
 * terminating null character.
 */
BOOL WINAPI
GetUserNameA( LPSTR lpszName, LPDWORD lpSize )
{
  size_t len;
  char *name;

#ifdef HAVE_GETPWUID
  struct passwd *pwd = getpwuid( getuid() );
  name = pwd ? pwd->pw_name : NULL;
#else
  name = getenv("USER");
#endif
  if (!name) {
    ERR("Username lookup failed: %s\n", strerror(errno));
    return 0;
  }

  /* We need to include the null character when determining the size of the buffer. */
  len = strlen(name) + 1;
  if (len > *lpSize)
  {
    SetLastError(ERROR_MORE_DATA);
    *lpSize = len;
    return 0;
  }

  *lpSize = len;
  strcpy(lpszName, name);
  return 1;
}

/******************************************************************************
 * GetUserNameW [ADVAPI32.@]
 *
 * PARAMS
 *   lpszName []
 *   lpSize   []
 */
BOOL WINAPI
GetUserNameW( LPWSTR lpszName, LPDWORD lpSize )
{
	LPSTR name = (LPSTR)HeapAlloc( GetProcessHeap(), 0, *lpSize );
	DWORD	size = *lpSize;
	BOOL res = GetUserNameA(name,lpSize);

        /* FIXME: should set lpSize in WCHARs */
        if (size && !MultiByteToWideChar( CP_ACP, 0, name, -1, lpszName, size ))
            lpszName[size-1] = 0;
        HeapFree( GetProcessHeap(), 0, name );
	return res;
}

/******************************************************************************
 * GetCurrentHwProfileA [ADVAPI32.@]
 */
BOOL WINAPI GetCurrentHwProfileA(LPHW_PROFILE_INFOA info)
{
	FIXME("Mostly Stub\n");
	info->dwDockInfo = DOCKINFO_DOCKED;
	strcpy(info->szHwProfileGuid,"{12340001-1234-1234-1234-1233456789012}");
	strcpy(info->szHwProfileName,"Wine Profile");
	return 1;
}

/******************************************************************************
 * AbortSystemShutdownA [ADVAPI32.@]
 *
 * PARAMS
 * 	lpMachineName
 */
BOOL WINAPI AbortSystemShutdownA( LPSTR lpMachineName )
{
    TRACE("stub %s (harmless)\n", lpMachineName);
    return TRUE;
}

/******************************************************************************
 * AbortSystemShutdownW [ADVAPI32.@]
 *
 * PARAMS
 * 	lpMachineName
 */
BOOL WINAPI AbortSystemShutdownW( LPCWSTR lpMachineName )
{
    TRACE("stub %s (harmless)\n", debugstr_w(lpMachineName));
    return TRUE;
}
