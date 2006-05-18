/*
 * BIOS interrupt 19h handler
 *
 * Copyright 1998 Carl van Schaik
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdlib.h>
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);


/**********************************************************************
 *          DOSVM_Int19Handler
 *
 * Handler for int 19h (Reboot).
 */
void WINAPI DOSVM_Int19Handler( CONTEXT86 *context )
{
    TRACE( "Attempted Reboot\n" );
    ExitProcess(0);
}
