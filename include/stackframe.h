/*
 * 16-bit and 32-bit mode stack frame layout
 *
 * Copyright 1995, 1998 Alexandre Julliard
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

#ifndef __WINE_STACKFRAME_H
#define __WINE_STACKFRAME_H

#include <string.h>
#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#include <winreg.h>
#include <winternl.h>
#include <thread.h>
#include <wine/winbase16.h>

#define CURRENT_STACK16      ((STACK16FRAME*)MapSL((SEGPTR)NtCurrentTeb()->WOW32Reserved))
#define CURRENT_DS           (CURRENT_STACK16->ds)

/* Push bytes on the 16-bit stack of a thread;
 * return a segptr to the first pushed byte
 */
static inline SEGPTR stack16_push( int size )
{
    STACK16FRAME *frame = CURRENT_STACK16;
    memmove( (char*)frame - size, frame, sizeof(*frame) );
    NtCurrentTeb()->WOW32Reserved = (char *)NtCurrentTeb()->WOW32Reserved - size;
    return (SEGPTR)((char *)NtCurrentTeb()->WOW32Reserved + sizeof(*frame));
}

/* Pop bytes from the 16-bit stack of a thread */
static inline void stack16_pop( int size )
{
    STACK16FRAME *frame = CURRENT_STACK16;
    memmove( (char*)frame + size, frame, sizeof(*frame) );
    NtCurrentTeb()->WOW32Reserved = (char *)NtCurrentTeb()->WOW32Reserved + size;
}

#endif /* __WINE_STACKFRAME_H */
