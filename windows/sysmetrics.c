/*
 * System metrics functions
 *
 * Copyright 1994 Alexandre Julliard
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winuser.h"
#include "winerror.h"
#include "user.h"

static int sysMetrics[SM_CMETRICS+1];
static HDC display_dc;


/***********************************************************************
 * RegistryTwips2Pixels
 *
 * Convert a a dimension value that was obtained from the registry.  These are
 * quoted as being "twips" values if negative and pixels if positive.
 * See for example
 *   MSDN Library - April 2001 -> Resource Kits ->
 *       Windows 2000 Resource Kit Reference ->
 *       Technical Reference to the Windows 2000 Registry ->
 *       HKEY_CURRENT_USER -> Control Panel -> Desktop -> WindowMetrics
 *
 * This is written as a function to prevent repeated evaluation of the
 * argument.
 */
inline static int RegistryTwips2Pixels(int x)
{
    if (x < 0)
        x = (-x+7)/15;
    return x;
}


/***********************************************************************
 * SYSMETRICS_GetRegistryMetric
 *
 * Get a registry entry from the already open key.  This allows us to open the
 * section once and read several values.
 *
 * Of course this function belongs somewhere more usable but here will do
 * for now.
 */
static int SYSMETRICS_GetRegistryMetric (
        HKEY       hkey,            /* handle to the registry section */
        const char *key,            /* value name in the section */
        int        default_value)   /* default to return */
{
    int value = default_value;
    if (hkey)
    {
        BYTE buffer[128];
        DWORD type, count = sizeof(buffer);
        if(!RegQueryValueExA (hkey, key, 0, &type, buffer, &count))
        {
            if (type != REG_SZ)
            {
                /* Are there any utilities for converting registry entries
                 * between formats?
                 */
                /* FIXME_(reg)("We need reg format converter\n"); */
            }
            else
                value = atoi(buffer);
        }
    }
    return RegistryTwips2Pixels(value);
}


/***********************************************************************
 *           SYSMETRICS_Init
 *
 * Initialisation of the system metrics array.
 *
 * Differences in return values between 3.1 and 95 apps under Win95 (FIXME ?):
 * SM_CXVSCROLL        x+1      x	Fixed May 24, 1999 - Ronald B. Cemer
 * SM_CYHSCROLL        x+1      x	Fixed May 24, 1999 - Ronald B. Cemer
 * SM_CXDLGFRAME       x-1      x	Already fixed
 * SM_CYDLGFRAME       x-1      x	Already fixed
 * SM_CYCAPTION        x+1      x	Fixed May 24, 1999 - Ronald B. Cemer
 * SM_CYMENU           x-1      x	Already fixed
 * SM_CYFULLSCREEN     x-1      x
 * SM_CXFRAME                           Fixed July 6, 2001 - Bill Medland
 *
 * (collides with TWEAK_WineLook sometimes,
 * so changing anything might be difficult)
 *
 * Starting at Win95 there are now a large number or Registry entries in the
 * [WindowMetrics] section that are probably relevant here.
 */
void SYSMETRICS_Init(void)
{
    HKEY hkey; /* key to the window metrics area of the registry */
    INT dummy;

    display_dc = CreateICA( "DISPLAY", NULL, NULL, NULL );
    assert( display_dc );

    if (RegOpenKeyExA (HKEY_CURRENT_USER, "Control Panel\\desktop\\WindowMetrics",
                       0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS) hkey = 0;

    if (TWEAK_WineLook > WIN31_LOOK)
    {
        sysMetrics[SM_CXVSCROLL] = SYSMETRICS_GetRegistryMetric( hkey, "ScrollWidth", 16 );
        sysMetrics[SM_CYHSCROLL] = sysMetrics[SM_CXVSCROLL];

        /* The Win 2000 resource kit SAYS that this is governed by the ScrollHeight
         * but on my computer that controls the CYV/CXH values. */
        sysMetrics[SM_CYCAPTION] = SYSMETRICS_GetRegistryMetric(hkey, "CaptionHeight", 18)
                                     + 1; /* for the separator? */

        sysMetrics[SM_CYMENU] = SYSMETRICS_GetRegistryMetric (hkey, "MenuHeight", 18) + 1;


        sysMetrics[SM_CXDLGFRAME] = 3;
        sysMetrics[SM_CYDLGFRAME] = sysMetrics[SM_CXDLGFRAME];

        /* force setting of SM_CXFRAME/SM_CYFRAME */
        SystemParametersInfoA( SPI_GETBORDER, 0, &dummy, 0 );
    }
    else
    {
        sysMetrics[SM_CXVSCROLL]  = 17;
        sysMetrics[SM_CYHSCROLL]  = sysMetrics[SM_CXVSCROLL];
        sysMetrics[SM_CYCAPTION]  = 20;
        sysMetrics[SM_CYMENU]     = 18;
        sysMetrics[SM_CXDLGFRAME] = 4;
        sysMetrics[SM_CYDLGFRAME] = sysMetrics[SM_CXDLGFRAME];
        sysMetrics[SM_CXFRAME]    = GetProfileIntA("Windows", "BorderWidth", 4) + 1;
        sysMetrics[SM_CYFRAME]    = sysMetrics[SM_CXFRAME];
    }

    sysMetrics[SM_CXCURSOR] = 32;
    sysMetrics[SM_CYCURSOR] = 32;
    sysMetrics[SM_CXBORDER] = 1;
    sysMetrics[SM_CYBORDER] = sysMetrics[SM_CXBORDER];
    sysMetrics[SM_CYVTHUMB] = sysMetrics[SM_CXVSCROLL];
    sysMetrics[SM_CXHTHUMB] = sysMetrics[SM_CYVTHUMB];
    sysMetrics[SM_CXICON] = 32;
    sysMetrics[SM_CYICON] = 32;
    sysMetrics[SM_CYKANJIWINDOW] = 0;
    sysMetrics[SM_MOUSEPRESENT] = 1;
    sysMetrics[SM_CYVSCROLL] = SYSMETRICS_GetRegistryMetric (hkey, "ScrollHeight", sysMetrics[SM_CXVSCROLL]);
    sysMetrics[SM_CXHSCROLL] = SYSMETRICS_GetRegistryMetric (hkey, "ScrollHeight", sysMetrics[SM_CYHSCROLL]);
    sysMetrics[SM_DEBUG] = 0;

    sysMetrics[SM_SWAPBUTTON] = 0;
    sysMetrics[SM_SWAPBUTTON] = SYSPARAMS_GetMouseButtonSwap();

    sysMetrics[SM_RESERVED1] = 0;
    sysMetrics[SM_RESERVED2] = 0;
    sysMetrics[SM_RESERVED3] = 0;
    sysMetrics[SM_RESERVED4] = 0;

    /* FIXME: The following two are calculated, but how? */
    sysMetrics[SM_CXMIN] = (TWEAK_WineLook > WIN31_LOOK) ? 112 : 100;
    sysMetrics[SM_CYMIN] = (TWEAK_WineLook > WIN31_LOOK) ? 27 : 28;

    sysMetrics[SM_CXSIZE] = sysMetrics[SM_CYCAPTION] - 1;
    sysMetrics[SM_CYSIZE] = sysMetrics[SM_CXSIZE];
    sysMetrics[SM_CXMINTRACK] = sysMetrics[SM_CXMIN];
    sysMetrics[SM_CYMINTRACK] = sysMetrics[SM_CYMIN];

    sysMetrics[SM_CXDOUBLECLK] = 4;
    sysMetrics[SM_CYDOUBLECLK] = 4;
    SYSPARAMS_GetDoubleClickSize( &sysMetrics[SM_CXDOUBLECLK], &sysMetrics[SM_CYDOUBLECLK] );

    sysMetrics[SM_CXICONSPACING] = 75;
    SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0,
                           &sysMetrics[SM_CXICONSPACING], 0 );
    sysMetrics[SM_CYICONSPACING] = 75;
    SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0,
                           &sysMetrics[SM_CYICONSPACING], 0 );

    SystemParametersInfoA( SPI_GETMENUDROPALIGNMENT, 0,
                           &sysMetrics[SM_MENUDROPALIGNMENT], 0 );
    sysMetrics[SM_PENWINDOWS] = 0;
    sysMetrics[SM_DBCSENABLED] = 0;

    /* FIXME: Need to query X for the following */
    sysMetrics[SM_CMOUSEBUTTONS] = 3;

    sysMetrics[SM_SECURE] = 0;
    sysMetrics[SM_CXEDGE] = sysMetrics[SM_CXBORDER] + 1;
    sysMetrics[SM_CYEDGE] = sysMetrics[SM_CXEDGE];
    sysMetrics[SM_CXMINSPACING] = 160;
    sysMetrics[SM_CYMINSPACING] = 24;
    sysMetrics[SM_CXSMICON] = sysMetrics[SM_CYSIZE] - (sysMetrics[SM_CYSIZE] % 2);
    sysMetrics[SM_CYSMICON] = sysMetrics[SM_CXSMICON];
    sysMetrics[SM_CYSMCAPTION] = 16;
    sysMetrics[SM_CXSMSIZE] = 13;
    sysMetrics[SM_CYSMSIZE] = 15;
    sysMetrics[SM_CXMENUSIZE] = sysMetrics[SM_CYMENU] - 1;
    sysMetrics[SM_CYMENUSIZE] = sysMetrics[SM_CXMENUSIZE];

    /* FIXME: What do these mean? */
    sysMetrics[SM_ARRANGE] = ARW_HIDE;
    sysMetrics[SM_CXMINIMIZED] = 160;
    sysMetrics[SM_CYMINIMIZED] = 24;

    /* FIXME: How do I calculate these? */
    sysMetrics[SM_NETWORK] = 3;

    /* For the following: 0 = ok, 1 = failsafe, 2 = failsafe + network */
    sysMetrics[SM_CLEANBOOT] = 0;

    sysMetrics[SM_CXDRAG] = 2;
    sysMetrics[SM_CYDRAG] = 2;
    sysMetrics[SM_CXMENUCHECK] = 13;
    sysMetrics[SM_CYMENUCHECK] = 13;

    /* FIXME: Should check the type of processor for the following */
    sysMetrics[SM_SLOWMACHINE] = 0;

    /* FIXME: Should perform a check */
    sysMetrics[SM_MIDEASTENABLED] = 0;

    sysMetrics[SM_MOUSEWHEELPRESENT] = 1;

    sysMetrics[SM_XVIRTUALSCREEN] = 0;
    sysMetrics[SM_YVIRTUALSCREEN] = 0;
    sysMetrics[SM_CMONITORS] = 1;
    sysMetrics[SM_SAMEDISPLAYFORMAT] = 1;
    sysMetrics[SM_CMETRICS] = SM_CMETRICS;

    SystemParametersInfoA( SPI_GETSHOWSOUNDS, 0, &sysMetrics[SM_SHOWSOUNDS], 0 );

    if (hkey) RegCloseKey (hkey);
}


/***********************************************************************
 *		SYSMETRICS_Set
 *
 *  Sets system metrics.
 */
INT SYSMETRICS_Set( INT index, INT value )
{
    if ((index < 0) || (index > SM_CMETRICS)) return 0;
    else
    {
        INT prev = sysMetrics[index];
        sysMetrics[index] = value;
        return prev;
    }
}


/***********************************************************************
 *		GetSystemMetrics (USER.179)
 */
INT16 WINAPI GetSystemMetrics16( INT16 index )
{
    return (INT16)GetSystemMetrics(index);
}


/***********************************************************************
 *		GetSystemMetrics (USER32.@)
 */
INT WINAPI GetSystemMetrics( INT index )
{
    /* some metrics are dynamic */
    switch (index)
    {
    case SM_CXSCREEN:
    case SM_CXFULLSCREEN:
    case SM_CXVIRTUALSCREEN:
        return GetDeviceCaps( display_dc, HORZRES );
    case SM_CYSCREEN:
    case SM_CYVIRTUALSCREEN:
        return GetDeviceCaps( display_dc, VERTRES );
    case SM_CYFULLSCREEN:
        return GetDeviceCaps( display_dc, VERTRES ) - sysMetrics[SM_CYCAPTION];

    /* FIXME: How do I calculate these? */
    case SM_CXMAXTRACK:
        return GetDeviceCaps( display_dc, HORZRES ) + 4 + 2 * sysMetrics[SM_CXFRAME];
    case SM_CYMAXTRACK:
        return GetDeviceCaps( display_dc, VERTRES ) + 4 + 2 * sysMetrics[SM_CYFRAME];
    case SM_CXMAXIMIZED:
        return GetDeviceCaps( display_dc, HORZRES ) + 2 * sysMetrics[SM_CXFRAME];
    case SM_CYMAXIMIZED:
        return GetDeviceCaps( display_dc, VERTRES ) + 2 * sysMetrics[SM_CYFRAME];

    default:
        if ((index < 0) || (index > SM_CMETRICS)) return 0;
        return sysMetrics[index];
    }
}
