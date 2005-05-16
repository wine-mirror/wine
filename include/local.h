/*
 * Local heap declarations
 *
 * Copyright 1995 Alexandre Julliard
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

#ifndef __WINE_LOCAL_H
#define __WINE_LOCAL_H

#include <windef.h>
#include <wine/windef16.h>
#include <wine/winbase16.h>
#include <winreg.h>
#include <winternl.h>

#define CURRENT_DS (((STACK16FRAME*)MapSL((SEGPTR)NtCurrentTeb()->WOW32Reserved))->ds)

  /* These function are equivalent to the Local* API functions, */
  /* excepted that they need DS as the first parameter. This    */
  /* allows managing several heaps from the emulation library.  */
static inline HLOCAL16 LOCAL_Alloc( HANDLE16 ds, UINT16 flags, WORD size )
{
    HANDLE16 oldDS = CURRENT_DS;
    HLOCAL16 ret;

    CURRENT_DS = ds;
    ret = LocalAlloc16 (flags, size);
    CURRENT_DS = oldDS;
    return ret;
}

static inline  HLOCAL16 LOCAL_ReAlloc( HANDLE16 ds, HLOCAL16 handle, WORD size, UINT16 flags )
{
    HANDLE16 oldDS = CURRENT_DS;
    HLOCAL16 ret;

    CURRENT_DS = ds;
    ret = LocalReAlloc16 (handle, size, flags);
    CURRENT_DS = oldDS;
    return ret;
}

static inline HLOCAL16 LOCAL_Free( HANDLE16 ds, HLOCAL16 handle )
{
    HANDLE16 oldDS = CURRENT_DS;
    HLOCAL16 ret;

    CURRENT_DS = ds;
    ret = LocalFree16 (handle);
    CURRENT_DS = oldDS;
    return ret;
}
static inline HLOCAL16 LOCAL_Handle( HANDLE16 ds, WORD addr )
{
    HANDLE16 oldDS = CURRENT_DS;
    HLOCAL16 ret;

    CURRENT_DS = ds;
    ret = LocalHandle16 (addr);
    CURRENT_DS = oldDS;
    return ret;
}

static inline UINT16 LOCAL_Size( HANDLE16 ds, HLOCAL16 handle )
{
    HANDLE16 oldDS = CURRENT_DS;
    UINT16 ret;

    CURRENT_DS = ds;
    ret = LocalSize16 (handle);
    CURRENT_DS = oldDS;
    return ret;
}

static inline UINT16 LOCAL_Flags( HANDLE16 ds, HLOCAL16 handle )
{
    HANDLE16 oldDS = CURRENT_DS;
    UINT16 ret;

    CURRENT_DS = ds;
    ret = LocalFlags16 (handle);
    CURRENT_DS = oldDS;
    return ret;
}


static inline UINT16 LOCAL_HeapSize( HANDLE16 ds )
{
    HANDLE16 oldDS = CURRENT_DS;
    UINT16 ret;

    CURRENT_DS = ds;
    ret = LocalHeapSize16 ();
    CURRENT_DS = oldDS;
    return ret;
}

static inline UINT16 LOCAL_CountFree( HANDLE16 ds )
{
    HANDLE16 oldDS = CURRENT_DS;
    UINT16 ret;

    CURRENT_DS = ds;
    ret = LocalCountFree16 ();
    CURRENT_DS = oldDS;
    return ret;
}

static inline void *LOCAL_Lock( HANDLE16 ds, HLOCAL16 handle )
{
    HANDLE16 oldDS = CURRENT_DS;
    SEGPTR ret;

    CURRENT_DS = ds;
    ret = LocalLock16 (handle);
    CURRENT_DS = oldDS;

    return MapSL(ret);
}

static inline BOOL16 LOCAL_Unlock( HANDLE16 ds, HLOCAL16 handle )
{
    HANDLE16 oldDS = CURRENT_DS;
    BOOL16 ret;

    CURRENT_DS = ds;
    ret = LocalUnlock16 (handle);
    CURRENT_DS = oldDS;
    return ret;
}

static inline WORD LOCAL_Compact( HANDLE16 ds, UINT16 minfree )
{
    HANDLE16 oldDS = CURRENT_DS;
    WORD ret;

    CURRENT_DS = ds;
    ret = LocalCompact16 (minfree);
    CURRENT_DS = oldDS;
    return ret;
}


#endif  /* __WINE_LOCAL_H */
