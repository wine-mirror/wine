/*	ddraw/d3d structure version conversion
 *
 * Copyright 2000 TransGaming Technologies Inc.
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"

#include "ddraw_private.h"

void DDRAW_Convert_DDSCAPS_1_To_2(const DDSCAPS* pIn, DDSCAPS2* pOut)
{
    /* 2 adds three additional caps fields to the end. Both versions
     * are unversioned. */
    pOut->dwCaps = pIn->dwCaps;
    pOut->dwCaps2 = 0;
    pOut->dwCaps3 = 0;
    pOut->dwCaps4 = 0;
}

void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER2* pIn,
					     DDDEVICEIDENTIFIER* pOut)
{
    /* 2 adds a dwWHQLLevel field to the end. Both structures are
     * unversioned. */
    memcpy(pOut, pIn, sizeof(*pOut));
}
