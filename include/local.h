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

  /* These function are equivalent to the Local* API functions, */
  /* excepted that they need DS as the first parameter. This    */
  /* allows managing several heaps from the emulation library.  */

extern HLOCAL16 LOCAL_Alloc( HANDLE16 ds, UINT16 flags, WORD size );
extern HLOCAL16 LOCAL_ReAlloc( HANDLE16 ds, HLOCAL16 handle,
                               WORD size, UINT16 flags );
extern HLOCAL16 LOCAL_Free( HANDLE16 ds, HLOCAL16 handle );
extern HLOCAL16 LOCAL_Handle( HANDLE16 ds, WORD addr );
extern UINT16 LOCAL_Size( HANDLE16 ds, HLOCAL16 handle );
extern UINT16 LOCAL_Flags( HANDLE16 ds, HLOCAL16 handle );
extern UINT16 LOCAL_HeapSize( HANDLE16 ds );
extern UINT16 LOCAL_CountFree( HANDLE16 ds );
extern void *LOCAL_Lock( HANDLE16 ds, HLOCAL16 handle );
extern BOOL16 LOCAL_Unlock( HANDLE16 ds, HLOCAL16 handle );
extern WORD LOCAL_Compact( HANDLE16 ds, UINT16 minfree, UINT16 flags );

#endif  /* __WINE_LOCAL_H */
