/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * joystick functions
 *
 * Copyright 1997 Andreas Mohr
 *	     2000 Wolfgang Schwotzer
 *                Eric Pouech
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "mmddk.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winmm);

static CRITICAL_SECTION joystick_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &joystick_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": joystick_cs") }
};
static CRITICAL_SECTION joystick_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

#define JOY_PERIOD_MIN	(10)	/* min Capture time period */
#define JOY_PERIOD_MAX	(1000)	/* max Capture time period */

typedef struct tagWINE_JOYSTICK {
    JOYINFO info;
    HWND capture;
    UINT timer;
    DWORD threshold;
    BOOL changed;
    HDRVR driver;
} WINE_JOYSTICK;

static WINE_JOYSTICK joysticks[16];

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;
    return diff <= max_diff;
}

/**************************************************************************
 * 				JOY_LoadDriver		[internal]
 */
static	BOOL JOY_LoadDriver(DWORD dwJoyID)
{
    static BOOL winejoystick_missing = FALSE;

    if (dwJoyID >= ARRAY_SIZE(joysticks) || winejoystick_missing) return FALSE;
    if (joysticks[dwJoyID].driver) return TRUE;

    joysticks[dwJoyID].driver = OpenDriverA( "winejoystick.drv", 0, dwJoyID );

    if (!joysticks[dwJoyID].driver)
    {
        WARN("OpenDriverA(\"winejoystick.drv\") failed\n");

        /* The default driver is missing, don't attempt to load it again */
        winejoystick_missing = TRUE;
    }

    return (joysticks[dwJoyID].driver != 0);
}

static void CALLBACK joystick_timer( HWND hwnd, UINT msg, UINT_PTR timer, DWORD time )
{
    MMRESULT res;
    JOYINFO info;
    WORD change;
    LONG pos;
    int i;

    EnterCriticalSection( &joystick_cs );

    for (i = 0; i < ARRAY_SIZE(joysticks); i++)
    {
        if (joysticks[i].capture != hwnd) continue;
        if ((res = joyGetPos( i, &info )))
        {
            WARN( "joyGetPos failed: %08x\n", res );
            continue;
        }

        pos = MAKELONG( info.wXpos, info.wYpos );

        if (!joysticks[i].changed ||
            !compare_uint( joysticks[i].info.wXpos, info.wXpos, joysticks[i].threshold ) ||
            !compare_uint( joysticks[i].info.wYpos, info.wYpos, joysticks[i].threshold ))
        {
            SendMessageA( hwnd, MM_JOY1MOVE + i, info.wButtons, pos );
            joysticks[i].info.wXpos = info.wXpos;
            joysticks[i].info.wYpos = info.wYpos;
        }
        if (!joysticks[i].changed ||
            !compare_uint( joysticks[i].info.wZpos, info.wZpos, joysticks[i].threshold ))
        {
            SendMessageA( hwnd, MM_JOY1ZMOVE + i, info.wButtons, pos );
            joysticks[i].info.wZpos = info.wZpos;
        }
        if ((change = joysticks[i].info.wButtons ^ info.wButtons) != 0)
        {
            if (info.wButtons & change)
                SendMessageA( hwnd, MM_JOY1BUTTONDOWN + i, (change << 8) | (info.wButtons & change), pos );
            if (joysticks[i].info.wButtons & change)
                SendMessageA( hwnd, MM_JOY1BUTTONUP + i, (change << 8) | (joysticks[i].info.wButtons & change), pos );
            joysticks[i].info.wButtons = info.wButtons;
        }
    }

    LeaveCriticalSection( &joystick_cs );
}

/**************************************************************************
 *                              joyConfigChanged        [WINMM.@]
 */
MMRESULT WINAPI joyConfigChanged(DWORD flags)
{
    FIXME( "flags %#x stub!\n", flags );
    if (flags) return JOYERR_PARMS;
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyGetNumDevs		[WINMM.@]
 */
UINT WINAPI DECLSPEC_HOTPATCH joyGetNumDevs(void)
{
    return ARRAY_SIZE(joysticks);
}

/**************************************************************************
 * 				joyGetDevCapsW		[WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetDevCapsW( UINT_PTR id, JOYCAPSW *caps, UINT size )
{
    TRACE( "id %d, caps %p, size %u.\n", (int)id, caps, size );

    if (!caps) return MMSYSERR_INVALPARAM;
    if (size != sizeof(JOYCAPSW) && size != sizeof(JOYCAPS2W)) return JOYERR_PARMS;

    memset( caps, 0, size );
    wcscpy( caps->szRegKey, L"DINPUT.DLL" );
    if (id == ~(UINT_PTR)0) return JOYERR_NOERROR;

    if (id >= ARRAY_SIZE(joysticks)) return JOYERR_PARMS;
    if (!JOY_LoadDriver( id )) return MMSYSERR_NODRIVER;

    caps->wPeriodMin = JOY_PERIOD_MIN; /* FIXME */
    caps->wPeriodMax = JOY_PERIOD_MAX; /* FIXME (same as MS Joystick Driver) */

    return SendDriverMessage( joysticks[id].driver, JDD_GETDEVCAPS, (LPARAM)caps, size );
}

/**************************************************************************
 * 				joyGetDevCapsA		[WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetDevCapsA( UINT_PTR id, JOYCAPSA *caps, UINT size )
{
    UINT size_w = sizeof(JOYCAPS2W);
    JOYCAPS2W caps_w;
    MMRESULT res;

    TRACE( "id %d, caps %p, size %u.\n", (int)id, caps, size );

    if (!caps) return MMSYSERR_INVALPARAM;
    if (size != sizeof(JOYCAPSA) && size != sizeof(JOYCAPS2A)) return JOYERR_PARMS;

    if (size == sizeof(JOYCAPSA)) size_w = sizeof(JOYCAPSW);
    res = joyGetDevCapsW( id, (JOYCAPSW *)&caps_w, size_w );
    if (res) return res;

    caps->wMid = caps_w.wMid;
    caps->wPid = caps_w.wPid;
    WideCharToMultiByte( CP_ACP, 0, caps_w.szPname, -1, caps->szPname,
                         sizeof(caps->szPname), NULL, NULL );
    caps->wXmin = caps_w.wXmin;
    caps->wXmax = caps_w.wXmax;
    caps->wYmin = caps_w.wYmin;
    caps->wYmax = caps_w.wYmax;
    caps->wZmin = caps_w.wZmin;
    caps->wZmax = caps_w.wZmax;
    caps->wNumButtons = caps_w.wNumButtons;
    caps->wPeriodMin = caps_w.wPeriodMin;
    caps->wPeriodMax = caps_w.wPeriodMax;
    caps->wRmin = caps_w.wRmin;
    caps->wRmax = caps_w.wRmax;
    caps->wUmin = caps_w.wUmin;
    caps->wUmax = caps_w.wUmax;
    caps->wVmin = caps_w.wVmin;
    caps->wVmax = caps_w.wVmax;
    caps->wCaps = caps_w.wCaps;
    caps->wMaxAxes = caps_w.wMaxAxes;
    caps->wNumAxes = caps_w.wNumAxes;
    caps->wMaxButtons = caps_w.wMaxButtons;
    WideCharToMultiByte( CP_ACP, 0, caps_w.szRegKey, -1, caps->szRegKey,
                         sizeof(caps->szRegKey), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, caps_w.szOEMVxD, -1, caps->szOEMVxD,
                         sizeof(caps->szOEMVxD), NULL, NULL );

    if (size == sizeof(JOYCAPS2A))
    {
        JOYCAPS2A *caps2_a = (JOYCAPS2A *)caps;
        caps2_a->ManufacturerGuid = caps_w.ManufacturerGuid;
        caps2_a->ProductGuid = caps_w.ProductGuid;
        caps2_a->NameGuid = caps_w.NameGuid;
    }

    return JOYERR_NOERROR;
}

/**************************************************************************
 *                              joyGetPosEx             [WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetPosEx( UINT id, JOYINFOEX *info )
{
    TRACE( "id %u, info %p.\n", id, info );

    if (!info) return MMSYSERR_INVALPARAM;
    if (id >= ARRAY_SIZE(joysticks) || info->dwSize < sizeof(JOYINFOEX)) return JOYERR_PARMS;
    if (!JOY_LoadDriver( id )) return MMSYSERR_NODRIVER;

    info->dwXpos = 0;
    info->dwYpos = 0;
    info->dwZpos = 0;
    info->dwRpos = 0;
    info->dwUpos = 0;
    info->dwVpos = 0;
    info->dwButtons = 0;
    info->dwButtonNumber = 0;
    info->dwPOV = 0;
    info->dwReserved1 = 0;
    info->dwReserved2 = 0;

    return SendDriverMessage( joysticks[id].driver, JDD_GETPOSEX, (LPARAM)info, 0 );
}

/**************************************************************************
 * 				joyGetPos	       	[WINMM.@]
 */
MMRESULT WINAPI joyGetPos( UINT id, JOYINFO *info )
{
    JOYINFOEX infoex =
    {
        .dwSize = sizeof(JOYINFOEX),
        .dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNBUTTONS,
    };
    MMRESULT res;

    TRACE( "id %u, info %p.\n", id, info );

    if (!info) return MMSYSERR_INVALPARAM;
    if ((res = joyGetPosEx( id, &infoex ))) return res;

    info->wXpos = infoex.dwXpos;
    info->wYpos = infoex.dwYpos;
    info->wZpos = infoex.dwZpos;
    info->wButtons = infoex.dwButtons;

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyGetThreshold		[WINMM.@]
 */
MMRESULT WINAPI joyGetThreshold( UINT id, UINT *threshold )
{
    TRACE( "id %u, threshold %p.\n", id, threshold );

    if (id >= ARRAY_SIZE(joysticks)) return JOYERR_PARMS;

    EnterCriticalSection( &joystick_cs );
    *threshold = joysticks[id].threshold;
    LeaveCriticalSection( &joystick_cs );

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyReleaseCapture	[WINMM.@]
 */
MMRESULT WINAPI joyReleaseCapture( UINT id )
{
    TRACE( "id %u.\n", id );

    if (id >= ARRAY_SIZE(joysticks)) return JOYERR_PARMS;
    if (!JOY_LoadDriver( id )) return MMSYSERR_NODRIVER;

    EnterCriticalSection( &joystick_cs );

    if (!joysticks[id].capture)
        TRACE("Joystick is not captured, ignoring request.\n");
    else
    {
        KillTimer( joysticks[id].capture, joysticks[id].timer );
        joysticks[id].capture = 0;
        joysticks[id].timer = 0;
    }

    LeaveCriticalSection( &joystick_cs );

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joySetCapture		[WINMM.@]
 */
MMRESULT WINAPI joySetCapture( HWND hwnd, UINT id, UINT period, BOOL changed )
{
    MMRESULT res = JOYERR_NOERROR;

    TRACE( "hwnd %p, id %u, period %u, changed %u.\n", hwnd, id, period, changed );

    if (id >= ARRAY_SIZE(joysticks) || hwnd == 0) return JOYERR_PARMS;
    if (period < JOY_PERIOD_MIN) period = JOY_PERIOD_MIN;
    else if (period > JOY_PERIOD_MAX) period = JOY_PERIOD_MAX;
    if (!JOY_LoadDriver( id )) return MMSYSERR_NODRIVER;

    EnterCriticalSection( &joystick_cs );

    if (joysticks[id].capture || !IsWindow( hwnd ))
        res = JOYERR_NOCANDO; /* FIXME: what should be returned ? */
    else if (joyGetPos( id, &joysticks[id].info ) != JOYERR_NOERROR)
        res = JOYERR_UNPLUGGED;
    else if ((joysticks[id].timer = SetTimer( hwnd, 0, period, joystick_timer )) == 0)
        res = JOYERR_NOCANDO;
    else
    {
        joysticks[id].capture = hwnd;
        joysticks[id].changed = changed;
    }

    LeaveCriticalSection( &joystick_cs );
    return res;
}

/**************************************************************************
 * 				joySetThreshold		[WINMM.@]
 */
MMRESULT WINAPI joySetThreshold( UINT id, UINT threshold )
{
    TRACE( "id %u, threshold %u.\n", id, threshold );

    if (id >= ARRAY_SIZE(joysticks) || threshold > 65535) return MMSYSERR_INVALPARAM;

    EnterCriticalSection( &joystick_cs );
    joysticks[id].threshold = threshold;
    LeaveCriticalSection( &joystick_cs );

    return JOYERR_NOERROR;
}
