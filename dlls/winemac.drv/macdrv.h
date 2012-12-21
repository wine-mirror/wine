/*
 * MACDRV windowing driver
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 1999 Patrik Stridvall
 * Copyright 2011, 2012 Ken Thomases for CodeWeavers Inc.
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

#ifndef __WINE_MACDRV_H
#define __WINE_MACDRV_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include "macdrv_cocoa.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "wine/gdi_driver.h"


static inline CGRect cgrect_from_rect(RECT rect)
{
    if (rect.left >= rect.right || rect.top >= rect.bottom)
        return CGRectNull;
    return CGRectMake(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}

static inline RECT rect_from_cgrect(CGRect cgrect)
{
    static const RECT empty;

    if (!CGRectIsNull(cgrect))
    {
        RECT rect = { CGRectGetMinX(cgrect), CGRectGetMinY(cgrect),
                      CGRectGetMaxX(cgrect), CGRectGetMaxY(cgrect) };
        return rect;
    }

    return empty;
}

static inline const char *wine_dbgstr_cgrect(CGRect cgrect)
{
    return wine_dbg_sprintf("(%g,%g)-(%g,%g)", CGRectGetMinX(cgrect), CGRectGetMinY(cgrect),
                            CGRectGetMaxX(cgrect), CGRectGetMaxY(cgrect));
}


#endif  /* __WINE_MACDRV_H */
