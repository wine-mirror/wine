/*	ddraw/d3d structure version conversion
 *
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include <string.h>

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

void DDRAW_Convert_DDDEVICEIDENTIFIER_2_To_1(const DDDEVICEIDENTIFIER* pIn,
					     DDDEVICEIDENTIFIER2* pOut)
{
    /* 2 adds a dwWHQLLevel field to the end. Both structures are
     * unversioned. */
    memcpy(pOut, pIn, sizeof(*pOut));
}
