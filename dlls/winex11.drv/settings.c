/*
 * Wine X11drv display settings functions
 *
 * Copyright 2003 Alexander James Pasadyn
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

#include "config.h"
#include <string.h>
#include <stdio.h>
#include "x11drv.h"

#include "windef.h"
#include "wingdi.h"
#include "ddrawi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11settings);

/*
 * The DDHALMODEINFO type is used to hold all the mode information.  
 * This is done because the array of DDHALMODEINFO structures must be 
 * created for use by DirectDraw anyway.
 */
static LPDDHALMODEINFO dd_modes = NULL;
static unsigned int dd_mode_count = 0;
static unsigned int dd_max_modes = 0;
static int dd_mode_default = 0;
static const unsigned int depths[]  = {8, 16, 32};

/* pointers to functions that actually do the hard stuff */
static int (*pGetCurrentMode)(void);
static LONG (*pSetCurrentMode)(int mode);
static const char *handler_name;

/*
 * Set the handlers for resolution changing functions
 * and initialize the master list of modes
 */
LPDDHALMODEINFO X11DRV_Settings_SetHandlers(const char *name,
                                 int (*pNewGCM)(void),
                                 LONG (*pNewSCM)(int),
                                 unsigned int nmodes,
                                 int reserve_depths)
{
    handler_name = name;
    pGetCurrentMode = pNewGCM;
    pSetCurrentMode = pNewSCM;
    TRACE("Resolution settings now handled by: %s\n", name);
    if (reserve_depths)
        /* leave room for other depths */
        dd_max_modes = (3+1)*(nmodes);
    else 
        dd_max_modes = nmodes;

    if (dd_modes) 
    {
        TRACE("Destroying old display modes array\n");
        HeapFree(GetProcessHeap(), 0, dd_modes);
    }
    dd_modes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHALMODEINFO) * dd_max_modes);
    dd_mode_count = 0;
    TRACE("Initialized new display modes array\n");
    return dd_modes;
}

/* Add one mode to the master list */
void X11DRV_Settings_AddOneMode(unsigned int width, unsigned int height, unsigned int bpp, unsigned int freq)
{
    LPDDHALMODEINFO info = &(dd_modes[dd_mode_count]);
    DWORD dwBpp = screen_depth;
    if (dd_mode_count >= dd_max_modes)
    {
        ERR("Maximum modes (%d) exceeded\n", dd_max_modes);
        return;
    }
    if (dwBpp == 24) dwBpp = 32;
    if (bpp == 0) bpp = dwBpp;
    info->dwWidth        = width;
    info->dwHeight       = height;
    info->wRefreshRate   = freq;
    info->lPitch         = 0;
    info->dwBPP          = bpp;
    info->wFlags         = 0;
    info->dwRBitMask     = 0;
    info->dwGBitMask     = 0;
    info->dwBBitMask     = 0;
    info->dwAlphaBitMask = 0;
    TRACE("initialized mode %d: %dx%dx%d @%d Hz (%s)\n", 
          dd_mode_count, width, height, bpp, freq, handler_name);
    dd_mode_count++;
}

/* copy all the current modes using the other color depths */
void X11DRV_Settings_AddDepthModes(void)
{
    int i, j;
    int existing_modes = dd_mode_count;
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    for (j=0; j<3; j++)
    {
        if (depths[j] != dwBpp)
        {
            for (i=0; i < existing_modes; i++)
            {
                X11DRV_Settings_AddOneMode(dd_modes[i].dwWidth, dd_modes[i].dwHeight, 
                                           depths[j], dd_modes[i].wRefreshRate);
            }
        }
    }
}

/* set the default mode */
void X11DRV_Settings_SetDefaultMode(int mode)
{
    dd_mode_default = mode;
}

/* return the number of modes that are initialized */
unsigned int X11DRV_Settings_GetModeCount(void)
{
    return dd_mode_count;
}

/***********************************************************************
 * Default handlers if resolution switching is not enabled
 *
 */
static int X11DRV_nores_GetCurrentMode(void)
{
    return 0;
}
static LONG X11DRV_nores_SetCurrentMode(int mode)
{
    TRACE("Ignoring mode change request\n");
    return DISP_CHANGE_FAILED;
}
/* default handler only gets the current X desktop resolution */
void X11DRV_Settings_Init(void)
{
    X11DRV_Settings_SetHandlers("NoRes", 
                                X11DRV_nores_GetCurrentMode, 
                                X11DRV_nores_SetCurrentMode, 
                                1, 0);
    X11DRV_Settings_AddOneMode(screen_width, screen_height, 0, 60);
}

/***********************************************************************
 *		EnumDisplaySettingsEx  (X11DRV.@)
 *
 */
BOOL X11DRV_EnumDisplaySettingsEx( LPCWSTR name, DWORD n, LPDEVMODEW devmode, DWORD flags)
{
    DWORD dwBpp = screen_depth;
    if (dwBpp == 24) dwBpp = 32;
    devmode->dmDisplayFlags = 0;
    devmode->dmDisplayFrequency = 0;
    devmode->dmSize = sizeof(DEVMODEW);
    if (n == ENUM_CURRENT_SETTINGS)
    {
        TRACE("mode %d (current) -- getting current mode (%s)\n", n, handler_name);
        n = pGetCurrentMode();
    }
    if (n == ENUM_REGISTRY_SETTINGS)
    {
        TRACE("mode %d (registry) -- getting default mode (%s)\n", n, handler_name);
        n = dd_mode_default;
    }
    if (n < dd_mode_count)
    {
        devmode->dmPelsWidth = dd_modes[n].dwWidth;
        devmode->dmPelsHeight = dd_modes[n].dwHeight;
        devmode->dmBitsPerPel = dd_modes[n].dwBPP;
        devmode->dmDisplayFrequency = dd_modes[n].wRefreshRate;
        devmode->dmFields = (DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL);
        if (devmode->dmDisplayFrequency)
        {
            devmode->dmFields |= DM_DISPLAYFREQUENCY;
            TRACE("mode %d -- %dx%dx%dbpp @%d Hz (%s)\n", n,
                  devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel,
                  devmode->dmDisplayFrequency, handler_name);
        }
        else
        {
            TRACE("mode %d -- %dx%dx%dbpp (%s)\n", n,
                  devmode->dmPelsWidth, devmode->dmPelsHeight, devmode->dmBitsPerPel, 
                  handler_name);
        }
        return TRUE;
    }
    TRACE("mode %d -- not present (%s)\n", n, handler_name);
    return FALSE;
}

#define _X_FIELD(prefix, bits) if ((fields) & prefix##_##bits) {p+=sprintf(p, "%s%s", first ? "" : ",", #bits); first=FALSE;}
static const char * _CDS_flags(DWORD fields)
{
    BOOL first = TRUE;
    char buf[128];
    char *p = buf;
    _X_FIELD(CDS,UPDATEREGISTRY);_X_FIELD(CDS,TEST);_X_FIELD(CDS,FULLSCREEN);
    _X_FIELD(CDS,GLOBAL);_X_FIELD(CDS,SET_PRIMARY);_X_FIELD(CDS,RESET);
    _X_FIELD(CDS,SETRECT);_X_FIELD(CDS,NORESET);
    *p = 0;
    return wine_dbg_sprintf("%s", buf);
}
static const char * _DM_fields(DWORD fields)
{
    BOOL first = TRUE;
    char buf[128];
    char *p = buf;
    _X_FIELD(DM,BITSPERPEL);_X_FIELD(DM,PELSWIDTH);_X_FIELD(DM,PELSHEIGHT);
    _X_FIELD(DM,DISPLAYFLAGS);_X_FIELD(DM,DISPLAYFREQUENCY);_X_FIELD(DM,POSITION);
    *p = 0;
    return wine_dbg_sprintf("%s", buf);
}
#undef _X_FIELD

/***********************************************************************
 *		ChangeDisplaySettingsEx  (X11DRV.@)
 *
 */
LONG X11DRV_ChangeDisplaySettingsEx( LPCWSTR devname, LPDEVMODEW devmode,
                                     HWND hwnd, DWORD flags, LPVOID lpvoid )
{
    DWORD i, dwBpp = 0;
    DEVMODEW dm;
    BOOL def_mode = TRUE;

    TRACE("(%s,%p,%p,0x%08x,%p)\n",debugstr_w(devname),devmode,hwnd,flags,lpvoid);
    TRACE("flags=%s\n",_CDS_flags(flags));
    if (devmode)
    {
        TRACE("DM_fields=%s\n",_DM_fields(devmode->dmFields));
        TRACE("width=%d height=%d bpp=%d freq=%d (%s)\n",
              devmode->dmPelsWidth,devmode->dmPelsHeight,
              devmode->dmBitsPerPel,devmode->dmDisplayFrequency, handler_name);

        dwBpp = (devmode->dmBitsPerPel == 24) ? 32 : devmode->dmBitsPerPel;
        if (devmode->dmFields & DM_BITSPERPEL) def_mode &= !dwBpp;
        if (devmode->dmFields & DM_PELSWIDTH)  def_mode &= !devmode->dmPelsWidth;
        if (devmode->dmFields & DM_PELSHEIGHT) def_mode &= !devmode->dmPelsHeight;
        if (devmode->dmFields & DM_DISPLAYFREQUENCY) def_mode &= !devmode->dmDisplayFrequency;
    }

    if (def_mode)
    {
        TRACE("Return to original display mode (%s)\n", handler_name);
        if (!X11DRV_EnumDisplaySettingsEx(devname, dd_mode_default, &dm, 0))
        {
            ERR("Default mode not found!\n");
            return DISP_CHANGE_BADMODE;
        }
        devmode = &dm;
    }
    dwBpp = !dwBpp ? dd_modes[dd_mode_default].dwBPP : dwBpp;

    if ((devmode->dmFields & (DM_PELSWIDTH | DM_PELSHEIGHT)) != (DM_PELSWIDTH | DM_PELSHEIGHT))
        return DISP_CHANGE_BADMODE;

    for (i = 0; i < dd_mode_count; i++)
    {
        if (devmode->dmFields & DM_BITSPERPEL)
        {
            if (dwBpp != dd_modes[i].dwBPP)
                continue;
        }
        if (devmode->dmFields & DM_PELSWIDTH)
        {
            if (devmode->dmPelsWidth != dd_modes[i].dwWidth)
                continue;
        }
        if (devmode->dmFields & DM_PELSHEIGHT)
        {
            if (devmode->dmPelsHeight != dd_modes[i].dwHeight)
                continue;
        }
        if ((devmode->dmFields & DM_DISPLAYFREQUENCY) && (dd_modes[i].wRefreshRate != 0) &&
            devmode->dmDisplayFrequency != 0)
        {
            if (devmode->dmDisplayFrequency != dd_modes[i].wRefreshRate)
                continue;
        }
        /* we have a valid mode */
        TRACE("Requested display settings match mode %d (%s)\n", i, handler_name);
        if (!(flags & CDS_TEST))
            return pSetCurrentMode(i);
        return DISP_CHANGE_SUCCESSFUL;
    }

    /* no valid modes found */
    ERR("No matching mode found(%dx%dx%d)! (%s)\n",
        devmode->dmPelsWidth, devmode->dmPelsHeight,
        devmode->dmBitsPerPel, handler_name);
    return DISP_CHANGE_BADMODE;
}




/***********************************************************************
 * DirectDraw HAL interface
 *
 */
static DWORD PASCAL X11DRV_Settings_SetMode(LPDDHAL_SETMODEDATA data)
{
    TRACE("Mode %d requested by DDHAL (%s)\n", data->dwModeIndex, handler_name);
    switch (pSetCurrentMode(data->dwModeIndex))
    {
    case DISP_CHANGE_SUCCESSFUL:
        X11DRV_DDHAL_SwitchMode(data->dwModeIndex, NULL, NULL);
        data->ddRVal = DD_OK;
        break;
    case DISP_CHANGE_BADMODE:
        data->ddRVal = DDERR_WRONGMODE;
        break;
    default:
        data->ddRVal = DDERR_UNSUPPORTEDMODE;
    }
    return DDHAL_DRIVER_HANDLED;
}
int X11DRV_Settings_CreateDriver(LPDDHALINFO info)
{
    if (!dd_mode_count) return 0; /* no settings defined */

    TRACE("Setting up display settings for DDRAW (%s)\n", handler_name);
    info->dwNumModes = dd_mode_count;
    info->lpModeInfo = dd_modes;
    X11DRV_DDHAL_SwitchMode(pGetCurrentMode(), NULL, NULL);
    info->lpDDCallbacks->SetMode = X11DRV_Settings_SetMode;
    return TRUE;
}
