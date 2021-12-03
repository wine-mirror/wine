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

#define MAXJOYSTICK (JOYSTICKID2 + 30)
#define JOY_PERIOD_MIN	(10)	/* min Capture time period */
#define JOY_PERIOD_MAX	(1000)	/* max Capture time period */

typedef struct tagWINE_JOYSTICK {
    JOYINFO	ji;
    HWND	hCapture;
    UINT	wTimer;
    DWORD	threshold;
    BOOL	bChanged;
    HDRVR	hDriver;
} WINE_JOYSTICK;

static	WINE_JOYSTICK	JOY_Sticks[MAXJOYSTICK];

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

    if (dwJoyID >= MAXJOYSTICK || winejoystick_missing)
	return FALSE;
    if (JOY_Sticks[dwJoyID].hDriver)
	return TRUE;

    JOY_Sticks[dwJoyID].hDriver = OpenDriverA("winejoystick.drv", 0, dwJoyID);

    if (!JOY_Sticks[dwJoyID].hDriver)
    {
        WARN("OpenDriverA(\"winejoystick.drv\") failed\n");

        /* The default driver is missing, don't attempt to load it again */
        winejoystick_missing = TRUE;
    }

    return (JOY_Sticks[dwJoyID].hDriver != 0);
}

/**************************************************************************
 * 				JOY_Timer		[internal]
 */
static	void	CALLBACK	JOY_Timer(HWND hWnd, UINT wMsg, UINT_PTR wTimer, DWORD dwTime)
{
    int			i;
    WINE_JOYSTICK*	joy;
    MMRESULT		res;
    JOYINFO		ji;
    LONG		pos;
    unsigned 		buttonChange;

    for (i = 0; i < MAXJOYSTICK; i++) {
	joy = &JOY_Sticks[i];

	if (joy->hCapture != hWnd) continue;

	res = joyGetPos(i, &ji);
	if (res != JOYERR_NOERROR) {
	    WARN("joyGetPos failed: %08x\n", res);
	    continue;
	}

	pos = MAKELONG(ji.wXpos, ji.wYpos);

	if (!joy->bChanged ||
	    !compare_uint(joy->ji.wXpos, ji.wXpos, joy->threshold) ||
	    !compare_uint(joy->ji.wYpos, ji.wYpos, joy->threshold)) {
	    SendMessageA(joy->hCapture, MM_JOY1MOVE + i, ji.wButtons, pos);
	    joy->ji.wXpos = ji.wXpos;
	    joy->ji.wYpos = ji.wYpos;
	}
	if (!joy->bChanged ||
	    !compare_uint(joy->ji.wZpos, ji.wZpos, joy->threshold)) {
	    SendMessageA(joy->hCapture, MM_JOY1ZMOVE + i, ji.wButtons, pos);
	    joy->ji.wZpos = ji.wZpos;
	}
	if ((buttonChange = joy->ji.wButtons ^ ji.wButtons) != 0) {
	    if (ji.wButtons & buttonChange)
		SendMessageA(joy->hCapture, MM_JOY1BUTTONDOWN + i,
			     (buttonChange << 8) | (ji.wButtons & buttonChange), pos);
	    if (joy->ji.wButtons & buttonChange)
		SendMessageA(joy->hCapture, MM_JOY1BUTTONUP + i,
			     (buttonChange << 8) | (joy->ji.wButtons & buttonChange), pos);
	    joy->ji.wButtons = ji.wButtons;
	}
    }
}

/**************************************************************************
 *                              joyConfigChanged        [WINMM.@]
 */
MMRESULT WINAPI joyConfigChanged(DWORD flags)
{
    FIXME("(%x) - stub\n", flags);

    if (flags)
	return JOYERR_PARMS;

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyGetNumDevs		[WINMM.@]
 */
UINT WINAPI DECLSPEC_HOTPATCH joyGetNumDevs(void)
{
    UINT	ret = 0;
    int		i;

    for (i = 0; i < MAXJOYSTICK; i++) {
	if (JOY_LoadDriver(i)) {
            ret += SendDriverMessage(JOY_Sticks[i].hDriver, JDD_GETNUMDEVS, 0, 0);
	}
    }
    return ret;
}

/**************************************************************************
 * 				joyGetDevCapsW		[WINMM.@]
 */
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetDevCapsW( UINT_PTR id, JOYCAPSW *caps, UINT size )
{
    TRACE( "id %d, caps %p, size %u.\n", (int)id, caps, size );

    if (!caps) return MMSYSERR_INVALPARAM;
    if (size != sizeof(JOYCAPSW) && size != sizeof(JOYCAPS2W)) return JOYERR_PARMS;

    if (id >= MAXJOYSTICK) return JOYERR_PARMS;
    if (!JOY_LoadDriver( id )) return MMSYSERR_NODRIVER;

    caps->wPeriodMin = JOY_PERIOD_MIN; /* FIXME */
    caps->wPeriodMax = JOY_PERIOD_MAX; /* FIXME (same as MS Joystick Driver) */

    return SendDriverMessage( JOY_Sticks[id].hDriver, JDD_GETDEVCAPS, (LPARAM)caps, size );
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
MMRESULT WINAPI DECLSPEC_HOTPATCH joyGetPosEx(UINT wID, LPJOYINFOEX lpInfo)
{
    TRACE("(%d, %p);\n", wID, lpInfo);

    if (!lpInfo) return MMSYSERR_INVALPARAM;
    if (wID >= MAXJOYSTICK || lpInfo->dwSize < sizeof(JOYINFOEX)) return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID))	return MMSYSERR_NODRIVER;

    lpInfo->dwXpos = 0;
    lpInfo->dwYpos = 0;
    lpInfo->dwZpos = 0;
    lpInfo->dwRpos = 0;
    lpInfo->dwUpos = 0;
    lpInfo->dwVpos = 0;
    lpInfo->dwButtons = 0;
    lpInfo->dwButtonNumber = 0;
    lpInfo->dwPOV = 0;
    lpInfo->dwReserved1 = 0;
    lpInfo->dwReserved2 = 0;

    return SendDriverMessage(JOY_Sticks[wID].hDriver, JDD_GETPOSEX, (LPARAM)lpInfo, 0);
}

/**************************************************************************
 * 				joyGetPos	       	[WINMM.@]
 */
MMRESULT WINAPI joyGetPos(UINT wID, LPJOYINFO lpInfo)
{
    TRACE("(%d, %p);\n", wID, lpInfo);

    if (!lpInfo) return MMSYSERR_INVALPARAM;
    if (wID >= MAXJOYSTICK)	return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID))	return MMSYSERR_NODRIVER;

    lpInfo->wXpos = 0;
    lpInfo->wYpos = 0;
    lpInfo->wZpos = 0;
    lpInfo->wButtons = 0;

    return SendDriverMessage(JOY_Sticks[wID].hDriver, JDD_GETPOS, (LPARAM)lpInfo, 0);
}

/**************************************************************************
 * 				joyGetThreshold		[WINMM.@]
 */
MMRESULT WINAPI joyGetThreshold(UINT wID, LPUINT lpThreshold)
{
    TRACE("(%04X, %p);\n", wID, lpThreshold);

    if (wID >= MAXJOYSTICK)	return JOYERR_PARMS;

    *lpThreshold = JOY_Sticks[wID].threshold;
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joyReleaseCapture	[WINMM.@]
 */
MMRESULT WINAPI joyReleaseCapture(UINT wID)
{
    TRACE("(%04X);\n", wID);

    if (wID >= MAXJOYSTICK)		return JOYERR_PARMS;
    if (!JOY_LoadDriver(wID))		return MMSYSERR_NODRIVER;
    if (JOY_Sticks[wID].hCapture)
    {
        KillTimer(JOY_Sticks[wID].hCapture, JOY_Sticks[wID].wTimer);
        JOY_Sticks[wID].hCapture = 0;
        JOY_Sticks[wID].wTimer = 0;
    }
    else
        TRACE("Joystick is not captured, ignoring request.\n");

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joySetCapture		[WINMM.@]
 */
MMRESULT WINAPI joySetCapture(HWND hWnd, UINT wID, UINT wPeriod, BOOL bChanged)
{
    TRACE("(%p, %04X, %d, %d);\n",  hWnd, wID, wPeriod, bChanged);

    if (wID >= MAXJOYSTICK || hWnd == 0) return JOYERR_PARMS;
    if (wPeriod<JOY_PERIOD_MIN) wPeriod = JOY_PERIOD_MIN;
    else if(wPeriod>JOY_PERIOD_MAX) wPeriod = JOY_PERIOD_MAX;
    if (!JOY_LoadDriver(wID)) return MMSYSERR_NODRIVER;

    if (JOY_Sticks[wID].hCapture || !IsWindow(hWnd))
	return JOYERR_NOCANDO; /* FIXME: what should be returned ? */

    if (joyGetPos(wID, &JOY_Sticks[wID].ji) != JOYERR_NOERROR)
	return JOYERR_UNPLUGGED;

    if ((JOY_Sticks[wID].wTimer = SetTimer(hWnd, 0, wPeriod, JOY_Timer)) == 0)
	return JOYERR_NOCANDO;

    JOY_Sticks[wID].hCapture = hWnd;
    JOY_Sticks[wID].bChanged = bChanged;

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				joySetThreshold		[WINMM.@]
 */
MMRESULT WINAPI joySetThreshold(UINT wID, UINT wThreshold)
{
    TRACE("(%04X, %d);\n", wID, wThreshold);

    if (wID >= MAXJOYSTICK || wThreshold > 65535) return MMSYSERR_INVALPARAM;

    JOY_Sticks[wID].threshold = wThreshold;

    return JOYERR_NOERROR;
}
