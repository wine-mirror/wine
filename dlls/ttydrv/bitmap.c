/*
 * TTY bitmap driver
 *
 * Copyright 1999 Patrik Stridvall
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

#include "config.h"

#include <string.h>

#include "gdi.h"
#include "ttydrv.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ttydrv);


/***********************************************************************
 *           GetBitmapBits   (TTYDRV.@)
 */
LONG TTYDRV_GetBitmapBits(HBITMAP hbitmap, void *bits, LONG count)
{
    FIXME("(%p, %p, %ld): stub\n", hbitmap, bits, count);
    memset(bits, 0, count);
    return count;
}

/***********************************************************************
 *           SetBitmapBits   (TTYDRV.@)
 */
LONG TTYDRV_SetBitmapBits(HBITMAP hbitmap, const void *bits, LONG count)
{
    FIXME("(%p, %p, %ld): stub\n", hbitmap, bits, count);
    return count;
}

/***********************************************************************
 *		TTYDRV_BITMAP_CreateDIBSection
 */
HBITMAP TTYDRV_BITMAP_CreateDIBSection(
  TTYDRV_PDEVICE *physDev, BITMAPINFO *bmi, UINT usage,
  LPVOID *bits, HANDLE section, DWORD offset)
{
  FIXME("(%p, %p, %u, %p, %p, %ld): stub\n",
	physDev->hdc, bmi, usage, bits, section, offset);

  return (HBITMAP) NULL;
}

/***********************************************************************
 *		TTYDRV_DC_SetDIBitsToDevice
 */
INT TTYDRV_DC_SetDIBitsToDevice(TTYDRV_PDEVICE *physDev, INT xDest, INT yDest, DWORD cx,
				DWORD cy, INT xSrc, INT ySrc,
				UINT startscan, UINT lines, LPCVOID bits,
				const BITMAPINFO *info, UINT coloruse)
{
  FIXME("(%p, %d, %d, %ld, %ld, %d, %d, %u, %u, %p, %p, %u): stub\n",
	physDev->hdc, xDest, yDest, cx, cy, xSrc, ySrc, startscan, lines, bits, info, coloruse);

  return 0;
}
