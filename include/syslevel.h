/*
 * Win32 'syslevel' routines
 *
 * Copyright 1998 Ulrich Weigand
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

#ifndef __WINE_SYSLEVEL_H
#define __WINE_SYSLEVEL_H

#include <windef.h>
#include <winbase.h>

extern WORD SYSLEVEL_Win16CurrentTeb;
extern WORD SYSLEVEL_EmergencyTeb;

VOID SYSLEVEL_CheckNotLevel( INT level );

#endif  /* __WINE_SYSLEVEL_H */
