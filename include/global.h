/*
 * Global heap declarations
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

#ifndef __WINE_GLOBAL_H
#define __WINE_GLOBAL_H

#include <windef.h>
#include <wine/windef16.h>
#include <wine/library.h>

/* memory/global.c */
extern HGLOBAL16 GLOBAL_CreateBlock( UINT16 flags, const void *ptr, DWORD size,
                                     HGLOBAL16 hOwner, unsigned char selflags );
extern BOOL16 GLOBAL_FreeBlock( HGLOBAL16 handle );
extern BOOL16 GLOBAL_MoveBlock( HGLOBAL16 handle, const void *ptr, DWORD size );
extern HGLOBAL16 GLOBAL_Alloc( WORD flags, DWORD size, HGLOBAL16 hOwner, unsigned char selflags );

/* memory/virtual.c */
typedef BOOL (*HANDLERPROC)(LPVOID, LPCVOID);
extern BOOL VIRTUAL_SetFaultHandler(LPCVOID addr, HANDLERPROC proc, LPVOID arg);
extern DWORD VIRTUAL_HandleFault(LPCVOID addr);

#endif  /* __WINE_GLOBAL_H */
