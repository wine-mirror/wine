/*
 * DPLAYX.DLL LibMain
 *
 * Copyright 1999,2000 - Peter Hunnisett
 *
 * contact <hunnise@nortelnetworks.com>
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
#include <stdarg.h>

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"
#include "dplayx_global.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/* This is a globally exported variable at ordinal 6 of DPLAYX.DLL */
DWORD gdwDPlaySPRefCount = 0; /* FIXME: Should it be initialized here? */


BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{

  TRACE( "(%p,0x%08lx,%p)\n", hinstDLL, fdwReason, lpvReserved );

  switch ( fdwReason )
  {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        /* First instance perform construction of global processor data */
        return DPLAYX_ConstructData();

    case DLL_PROCESS_DETACH:
        /* Last instance performs destruction of global processor data */
        return DPLAYX_DestructData();

    default:
      break;

  }

  return TRUE;
}

/***********************************************************************
 *              DllCanUnloadNow (DPLAYX.10)
 */
HRESULT WINAPI DPLAYX_DllCanUnloadNow(void)
{
  HRESULT hr = ( gdwDPlaySPRefCount > 0 ) ? S_FALSE : S_OK;

  /* FIXME: Should I be putting a check in for class factory objects
   *        as well
   */

  TRACE( ": returning 0x%08lx\n", hr );

  return hr;
}
