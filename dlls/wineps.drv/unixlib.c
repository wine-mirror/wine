/*
 * Unix interface for wineps.drv
 *
 * Copyright 1998 Huw D M Davies
 * Copyright 2001 Marcus Meissner
 * Copyright 2023 Piotr Caban for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "psdrv.h"
#include "unixlib.h"
#include "wine/gdi_driver.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/* copied from kernelbase */
static int muldiv(int a, int b, int c)
{
    LONGLONG ret;

    if (!c) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (c < 0)
    {
        a = -a;
        c = -c;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ((a < 0 && b < 0) || (a >= 0 && b >= 0))
        ret = (((LONGLONG)a * b) + (c / 2)) / c;
    else
        ret = (((LONGLONG)a * b) - (c / 2)) / c;

    if (ret > 2147483647 || ret < -2147483647) return -1;
    return ret;
}

static INT CDECL get_device_caps(PHYSDEV dev, INT cap)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);

    TRACE("%p,%d\n", dev->hdc, cap);

    switch(cap)
    {
    case DRIVERVERSION:
        return 0;
    case TECHNOLOGY:
        return DT_RASPRINTER;
    case HORZSIZE:
        return muldiv(pdev->horzSize, 100, pdev->Devmode->dmPublic.dmScale);
    case VERTSIZE:
        return muldiv(pdev->vertSize, 100, pdev->Devmode->dmPublic.dmScale);
    case HORZRES:
        return pdev->horzRes;
    case VERTRES:
        return pdev->vertRes;
    case BITSPIXEL:
        /* Although Windows returns 1 for monochrome printers, we want
           CreateCompatibleBitmap to provide something other than 1 bpp */
        return 32;
    case NUMPENS:
        return 10;
    case NUMFONTS:
        return 39;
    case NUMCOLORS:
        return -1;
    case PDEVICESIZE:
        return sizeof(PSDRV_PDEVICE);
    case TEXTCAPS:
        return TC_CR_ANY | TC_VA_ABLE; /* psdrv 0x59f7 */
    case RASTERCAPS:
        return (RC_BITBLT | RC_BITMAP64 | RC_GDI20_OUTPUT | RC_DIBTODEV |
                RC_STRETCHBLT | RC_STRETCHDIB); /* psdrv 0x6e99 */
    case ASPECTX:
        return pdev->logPixelsX;
    case ASPECTY:
        return pdev->logPixelsY;
    case LOGPIXELSX:
        return muldiv(pdev->logPixelsX, pdev->Devmode->dmPublic.dmScale, 100);
    case LOGPIXELSY:
        return muldiv(pdev->logPixelsY, pdev->Devmode->dmPublic.dmScale, 100);
    case NUMRESERVED:
        return 0;
    case COLORRES:
        return 0;
    case PHYSICALWIDTH:
        return (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) ?
            pdev->PageSize.cy : pdev->PageSize.cx;
    case PHYSICALHEIGHT:
        return (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) ?
            pdev->PageSize.cx : pdev->PageSize.cy;
    case PHYSICALOFFSETX:
        if (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE)
        {
            if (pdev->pi->ppd->LandscapeOrientation == -90)
                return pdev->PageSize.cy - pdev->ImageableArea.top;
            else
                return pdev->ImageableArea.bottom;
        }
        return pdev->ImageableArea.left;

    case PHYSICALOFFSETY:
        if (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE)
        {
            if (pdev->pi->ppd->LandscapeOrientation == -90)
                return pdev->PageSize.cx - pdev->ImageableArea.right;
            else
                return pdev->ImageableArea.left;
        }
        return pdev->PageSize.cy - pdev->ImageableArea.top;

    default:
        dev = GET_NEXT_PHYSDEV(dev, pGetDeviceCaps);
        return dev->funcs->pGetDeviceCaps(dev, cap);
    }
}

static NTSTATUS init_dc(void *arg)
{
    struct init_dc_params *params = arg;

    params->funcs->pGetDeviceCaps = get_device_caps;
    return TRUE;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    init_dc,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count);
