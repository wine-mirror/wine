/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002 Mike McCormack for Codeweavers
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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

MSIHANDLEINFO *msihandletable[MSIMAXHANDLES];

MSIHANDLE alloc_msihandle(UINT type, UINT size, msihandledestructor destroy)
{
    MSIHANDLEINFO *info;
    UINT i;

    /* find a slot */
    for(i=0; i<MSIMAXHANDLES; i++)
        if( !msihandletable[i] )
            break;
    if( (i>=MSIMAXHANDLES) || msihandletable[i] )
        return 0;

    size += sizeof (MSIHANDLEINFO);
    info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );
    if( !info )
        return 0;

    info->magic = MSIHANDLE_MAGIC;
    info->type = type;
    info->destructor = destroy;

    msihandletable[i] = info;

    return (MSIHANDLE) (i+1);
}

void *msihandle2msiinfo(MSIHANDLE handle, UINT type)
{
    handle--;
    if( handle<0 )
        return NULL;
    if( handle>=MSIMAXHANDLES )
        return NULL;
    if( !msihandletable[handle] )
        return NULL;
    if( msihandletable[handle]->magic != MSIHANDLE_MAGIC )
        return NULL;
    if( type && (msihandletable[handle]->type != type) )
        return NULL;

    return &msihandletable[handle][1];
}

UINT WINAPI MsiCloseHandle(MSIHANDLE handle)
{
    MSIHANDLEINFO *info = msihandle2msiinfo(handle, 0);

    TRACE("%lx\n",handle);

    if( !info )
        return ERROR_INVALID_HANDLE;

    info--;

    if( info->magic != MSIHANDLE_MAGIC )
    {
        ERR("Invalid handle!\n");
        return ERROR_INVALID_HANDLE;
    }

    if( info->destructor )
        info->destructor( &info[1] );

    HeapFree( GetProcessHeap(), 0, info );
    msihandletable[handle-1] = NULL;

    TRACE("Destroyed\n");

    return 0;
}

UINT WINAPI MsiCloseAllHandles(void)
{
    UINT i;

    TRACE("\n");

    for(i=0; i<MSIMAXHANDLES; i++)
        MsiCloseHandle( i+1 );

    return 0;
}
