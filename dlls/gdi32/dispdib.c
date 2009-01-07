/*
 * DISPDIB.dll
 *
 * Copyright 1998 Ove Kåven (with some help from Marcus Meissner)
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

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "dispdib.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);


/*********************************************************************
 *	DisplayDib	(DISPDIB.1)
 *
 *  Disables GDI and takes over the VGA screen to show DIBs in full screen.
 *
 * FLAGS
 *
 *  DISPLAYDIB_NOPALETTE: don't change palette
 *  DISPLAYDIB_NOCENTER: don't center bitmap
 *  DISPLAYDIB_NOWAIT: don't wait (for keypress) before returning
 *  DISPLAYDIB_BEGIN: start of multiple calls (does not restore the screen)
 *  DISPLAYDIB_END: end of multiple calls (restores the screen)
 *  DISPLAYDIB_MODE_DEFAULT: default display mode
 *  DISPLAYDIB_MODE_320x200x8: Standard VGA 320x200 256 colors
 *  DISPLAYDIB_MODE_320x240x8: Tweaked VGA 320x240 256 colors
 *
 * RETURNS
 *
 *  DISPLAYDIB_NOERROR: success
 *  DISPLAYDIB_NOTSUPPORTED: function not supported
 *  DISPLAYDIB_INVALIDDIB: null or invalid DIB header
 *  DISPLAYDIB_INVALIDFORMAT: invalid DIB format
 *  DISPLAYDIB_INVALIDTASK: not called from current task
 *
 * BUGS
 *
 *  Waiting for keypresses is not implemented.
 */
WORD WINAPI DisplayDib(
		LPBITMAPINFO lpbi, /* [in] DIB header with resolution and palette */
		LPSTR lpBits,      /* [in] Bitmap bits to show */
		WORD wFlags        /* [in] */
	)
{
    FIXME( "broken, should be rewritten using ddraw\n" );
    return DISPLAYDIB_NOERROR;
}
