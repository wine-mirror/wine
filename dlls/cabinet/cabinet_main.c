/*
 * cabinet.dll main
 *
 * Copyright 2002 Patrik Stridvall
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#define NO_SHLWAPI_REG
#include "shlwapi.h"
#undef NO_SHLWAPI_REG

#include "cabinet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

/***********************************************************************
 * DllGetVersion (CABINET.2)
 *
 * Retrieves version information of the 'CABINET.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Supposedly returns version from IE6SP1RP1
 */
HRESULT WINAPI CABINET_DllGetVersion (DLLVERSIONINFO *pdvi)
{
  WARN("hmmm... not right version number \"5.1.1106.1\"?\n");

  if (pdvi->cbSize != sizeof(DLLVERSIONINFO)) return E_INVALIDARG;

  pdvi->dwMajorVersion = 5;
  pdvi->dwMinorVersion = 1;
  pdvi->dwBuildNumber = 1106;
  pdvi->dwPlatformID = 1;

  return S_OK;
}

/***********************************************************************
 * Extract (CABINET.3)
 *
 * Apparently an undocumented function, presumably to extract a CAB file
 * to somewhere...
 *
 * PARAMS
 *   unknown [IO] unknown pointer
 *   what    [I]  char* describing what to uncompress, I guess.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_OUTOFMEMORY (?)
 */
HRESULT WINAPI Extract(DWORD unknown, LPCSTR what)
{
  LPCSTR whatx;
  LPSTR dir, dirx, lastoption, x;
  BOOL updatelastoption;

  TRACE("(unknown == %0lx, what == %s)\n", unknown, debugstr_a(what));

  dir = LocalAlloc(LPTR, strlen(what)); 
  if (!dir) return E_OUTOFMEMORY;

  /* copy the filename up to the last pathsep to construct the dirname */
  whatx = what;
  dirx = dir;
  lastoption = NULL;
  while (*whatx) {
    if ((*whatx == '\\') || (*whatx == '/')) {
      /* unless all chars between *dirx and lastoption are pathsep's, we
         remember our location in dir as lastoption */
      if (lastoption) {
        updatelastoption = FALSE;
        for (x = lastoption; x < dirx; x++)
	  if ((*dirx != '\\') && (*dirx != '/')) {
	    updatelastoption = TRUE;
	    break;
	  }
        if (updatelastoption) lastoption = dirx;
      } else
        lastoption = dirx;
    }
    *dirx++ = *whatx++;
  }

  if (!lastoption) {
    /* FIXME: I guess use the cwd or something? */
    assert(FALSE);
  } else {
    *lastoption = '\0';
  }

  TRACE("extracting to dir: %s\n", debugstr_a(dir));

  /* FIXME: what to do on failure? */
  if (!process_cabinet(what, dir, FALSE, FALSE))
    return E_OUTOFMEMORY;

  LocalFree(dir);

  return S_OK;
}
