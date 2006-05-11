/*
 * cfgmgr32 implementation
 *
 * Copyright 2004 Aric Stewart for CodeWeavers
 * Copyright 2003 Mike McCormack for CodeWeavers
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
#include "windef.h"
#include "winbase.h"
#include "winnt.h"
#include "ddk/cfgmgr32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cfgmgr32);

/***********************************************************************
 *		DllMain  (CFGMGR32.@)
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( inst );
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		CM_Get_Device_ID_ListA  (CFGMGR32.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_ListA( 
    PCSTR pszFilter, PCHAR Buffer, ULONG BufferLen, ULONG ulFlags )
{
    FIXME("%p %p %ld %ld\n", pszFilter, Buffer, BufferLen, ulFlags );
    memset(Buffer,0,2);
    return CR_SUCCESS;
}

/***********************************************************************
 *		CM_Get_Device_ID_Size  (CFGMGR32.@)
 */
CONFIGRET WINAPI CM_Get_Device_ID_Size( ULONG*  pulLen, LPVOID dnDevInst,
                                        ULONG  ulFlags)
{
    FIXME("%p %p %lu\n",pulLen, dnDevInst, ulFlags);
    *pulLen = 1;
    return CR_SUCCESS;
}

/***********************************************************************
 *		CM_Get_Device_IDA  (CFGMGR32.@)
 */
CONFIGRET WINAPI CM_Get_Device_IDA( LPVOID dnDevInst, LPSTR Buffer, 
                                   ULONG  BufferLen, ULONG  ulFlags)
{
    FIXME("%p, %p, %lu %lu\n",dnDevInst, Buffer, BufferLen, ulFlags);
    Buffer[0] = 0;
    return CR_SUCCESS;
}
