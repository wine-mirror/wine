/*
 * KERNEL32 thunks and other undocumented stuff
 *
 * Copyright 1997-1998 Marcus Meissner
 * Copyright 1998      Ulrich Weigand
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

#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win32);

/***********************************************************************
 *          BeginUpdateResourceA                 (KERNEL32.@)
 */
HANDLE WINAPI BeginUpdateResourceA( LPCSTR pFileName, BOOL bDeleteExistingResources )
{
  FIXME("(%s,%d): stub\n",debugstr_a(pFileName),bDeleteExistingResources);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *          BeginUpdateResourceW                 (KERNEL32.@)
 */
HANDLE WINAPI BeginUpdateResourceW( LPCWSTR pFileName, BOOL bDeleteExistingResources )
{

  FIXME("(%s,%d): stub\n",debugstr_w(pFileName),bDeleteExistingResources);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *          EndUpdateResourceA                 (KERNEL32.@)
 */
BOOL WINAPI EndUpdateResourceA( HANDLE hUpdate, BOOL fDiscard )
{

  FIXME("(%p,%d): stub\n",hUpdate, fDiscard);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *          EndUpdateResourceW                 (KERNEL32.@)
 */
BOOL WINAPI EndUpdateResourceW( HANDLE hUpdate, BOOL fDiscard )
{
  FIXME("(%p,%d): stub\n",hUpdate, fDiscard);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           UpdateResourceA                 (KERNEL32.@)
 */
BOOL WINAPI UpdateResourceA(
  HANDLE  hUpdate,
  LPCSTR  lpType,
  LPCSTR  lpName,
  WORD    wLanguage,
  LPVOID  lpData,
  DWORD   cbData) {

  FIXME(": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           UpdateResourceW                 (KERNEL32.@)
 */
BOOL WINAPI UpdateResourceW(
  HANDLE  hUpdate,
  LPCWSTR lpType,
  LPCWSTR lpName,
  WORD    wLanguage,
  LPVOID  lpData,
  DWORD   cbData) {

  FIXME(": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
