/*
 * Wine internally cached objects to speedup some things and prevent
 * infinite duplication of trivial code and data.
 *
 * Copyright 1997 Bertho A. Stultiens
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
#include "wingdi.h"
#include "user.h"

static const WORD wPattern55AA[] =
{
    0x5555, 0xaaaa, 0x5555, 0xaaaa,
    0x5555, 0xaaaa, 0x5555, 0xaaaa
};

static HBRUSH  hPattern55AABrush = 0;
static HBITMAP hPattern55AABitmap = 0;


/*********************************************************************
 *	CACHE_GetPattern55AABrush
 */
HBRUSH CACHE_GetPattern55AABrush(void)
{
    if (!hPattern55AABrush)
    {
        hPattern55AABitmap = CreateBitmap( 8, 8, 1, 1, wPattern55AA );
        hPattern55AABrush = CreatePatternBrush( hPattern55AABitmap );
    }
    return hPattern55AABrush;
}
