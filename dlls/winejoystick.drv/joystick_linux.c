/*
 * joystick functions
 *
 * Copyright 1997 Andreas Mohr
 * Copyright 2000 Wolfgang Schwotzer
 * Copyright 2002 David Hagood
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
 *
 * NOTES:
 *
 * - nearly all joystick functions can be regarded as obsolete,
 *   as Linux (2.1.x) now supports extended joysticks with a completely
 *   new joystick driver interface
 *   New driver's documentation says:
 *     "For backward compatibility the old interface is still included,
 *     but will be dropped in the future."
 *   Thus we should implement the new interface and at most keep the old
 *   routines for backward compatibility.
 * - better support of enhanced joysticks (Linux 2.2 interface is available)
 * - support more joystick drivers (like the XInput extension)
 * - should load joystick DLL as any other driver (instead of hardcoding)
 *   the driver's name, and load it as any low lever driver.
 */

#include "config.h"
#include "wine/port.h"

#ifdef HAVE_LINUX_JOYSTICK_H

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_IOCTL_H
#include <linux/ioctl.h>
#endif
#include <linux/joystick.h>
#ifdef SW_MAX
#undef SW_MAX
#endif
#define JOYDEV_NEW "/dev/input/js%d"
#define JOYDEV_OLD "/dev/js%d"
#include <errno.h>

#include "joystick.h"

#include "wingdi.h"
#include "winnls.h"
#include "wine/debug.h"

#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(joystick);

#define MAXJOYSTICK (JOYSTICKID2 + 30)

typedef struct tagWINE_JSTCK {
    int		joyIntf;
    BOOL        in_use;
    /* Some extra info we need to make this actually work under the
       Linux 2.2 event api.
       First of all, we cannot keep closing and reopening the device file -
       that blows away the state of the stick device, and we lose events. So, we
       need to open the low-level device once, and close it when we are done.

       Secondly, the event API only gives us what's changed. However, Windows apps
       want the whole state every time, so we have to cache the data.
    */

    int         dev; /* Linux level device file descriptor */
    int         x;
    int         y;
    int         z;
    int         r;
    int         u;
    int         v;
    int         pov_x;
    int         pov_y;
    int         buttons;
    char        axesMap[ABS_MAX + 1];
} WINE_JSTCK;

static	WINE_JSTCK	JSTCK_Data[MAXJOYSTICK];

/**************************************************************************
 * 				JSTCK_drvGet			[internal]
 */
static WINE_JSTCK *JSTCK_drvGet(DWORD_PTR dwDevID)
{
    int	p;

    if ((dwDevID - (DWORD_PTR)JSTCK_Data) % sizeof(JSTCK_Data[0]) != 0)
	return NULL;
    p = (dwDevID - (DWORD_PTR)JSTCK_Data) / sizeof(JSTCK_Data[0]);
    if (p < 0 || p >= MAXJOYSTICK || !((WINE_JSTCK*)dwDevID)->in_use)
	return NULL;

    return (WINE_JSTCK*)dwDevID;
}

/**************************************************************************
 * 				driver_open
 */
LRESULT driver_open(LPSTR str, DWORD dwIntf)
{
    if (dwIntf >= MAXJOYSTICK || JSTCK_Data[dwIntf].in_use)
	return 0;

    JSTCK_Data[dwIntf].joyIntf = dwIntf;
    JSTCK_Data[dwIntf].dev = -1;
    JSTCK_Data[dwIntf].in_use = TRUE;
    return (LRESULT)&JSTCK_Data[dwIntf];
}

/**************************************************************************
 * 				driver_close
 */
LRESULT driver_close(DWORD_PTR dwDevID)
{
    WINE_JSTCK*	jstck = JSTCK_drvGet(dwDevID);

    if (jstck == NULL)
	return 0;
    jstck->in_use = FALSE;
    if (jstck->dev > 0)
    {
       close(jstck->dev);
       jstck->dev = 0;
    }
    return 1;
}

struct js_status
{
    int buttons;
    int x;
    int y;
};

/**************************************************************************
 *                              JSTCK_OpenDevice           [internal]
 */
static	int	JSTCK_OpenDevice(WINE_JSTCK* jstick)
{
    char  buf[20];
    int   flags, fd, found_ix, i;
    static DWORD last_attempt;
    DWORD now;

    if (jstick->dev > 0)
      return jstick->dev;

    now = GetTickCount();
    if (now - last_attempt < 2000)
      return -1;
    last_attempt = now;

#ifdef HAVE_LINUX_22_JOYSTICK_API
    flags = O_RDONLY | O_NONBLOCK;
#else
    flags = O_RDONLY;
#endif

    /* The first joystick may not be at /dev/input/js0, find the correct
     * first or second device. For example the driver for XBOX 360 wireless
     * receiver creates entries starting at 20.
     */
    for (found_ix = i = 0; i < MAXJOYSTICK; i++) {
        sprintf(buf, JOYDEV_NEW, i);
        if ((fd = open(buf, flags)) < 0) {
            sprintf(buf, JOYDEV_OLD, i);
            if ((fd = open(buf, flags)) < 0)
                continue;
        }

        if (found_ix++ == jstick->joyIntf)
        {
            TRACE("Found joystick[%d] at %s\n", jstick->joyIntf, buf);
            jstick->dev = fd;
            last_attempt = 0;
            break;
        }

        close(fd);
    }

#ifdef HAVE_LINUX_22_JOYSTICK_API
    if (jstick->dev > 0)
        ioctl(jstick->dev, JSIOCGAXMAP, jstick->axesMap);
#endif
    return jstick->dev;
}


/**************************************************************************
 * 				JoyGetDevCaps		[MMSYSTEM.102]
 */
LRESULT driver_joyGetDevCaps(DWORD_PTR dwDevID, LPJOYCAPSW lpCaps, DWORD dwSize)
{
    WINE_JSTCK*	jstck;
#ifdef HAVE_LINUX_22_JOYSTICK_API
    int		dev;
    char	nrOfAxes;
    char	nrOfButtons;
    char	identString[MAXPNAMELEN];
    int		i;
    int		driverVersion;
#else
static const WCHAR ini[] = {'W','i','n','e',' ','J','o','y','s','t','i','c','k',' ','D','r','i','v','e','r',0};
#endif

    if ((jstck = JSTCK_drvGet(dwDevID)) == NULL)
	return MMSYSERR_NODRIVER;

#ifdef HAVE_LINUX_22_JOYSTICK_API
    if ((dev = JSTCK_OpenDevice(jstck)) < 0) return JOYERR_PARMS;
    ioctl(dev, JSIOCGAXES, &nrOfAxes);
    ioctl(dev, JSIOCGBUTTONS, &nrOfButtons);
    ioctl(dev, JSIOCGVERSION, &driverVersion);
    ioctl(dev, JSIOCGNAME(sizeof(identString)), identString);
    TRACE("Driver: 0x%06x, Name: %s, #Axes: %d, #Buttons: %d\n",
	  driverVersion, identString, nrOfAxes, nrOfButtons);
    lpCaps->wMid = MM_MICROSOFT;
    lpCaps->wPid = MM_PC_JOYSTICK;
    MultiByteToWideChar(CP_UNIXCP, 0, identString, -1, lpCaps->szPname, MAXPNAMELEN);
    lpCaps->szPname[MAXPNAMELEN-1] = '\0';
    lpCaps->wXmin = 0;
    lpCaps->wXmax = 0xFFFF;
    lpCaps->wYmin = 0;
    lpCaps->wYmax = 0xFFFF;
    lpCaps->wZmin = 0;
    lpCaps->wZmax = (nrOfAxes >= 3) ? 0xFFFF : 0;
#ifdef BODGE_THE_HAT
    /* Half-Life won't allow you to map an axis event to things like
       "next weapon" and "use". Linux reports the hat on my stick as
       axis U and V. So, IFF BODGE_THE_HAT is defined, lie through our
       teeth and say we have 32 buttons, and we will map the axes to
       the high buttons. Really, perhaps this should be a registry entry,
       or even a parameter to the Linux joystick driver (which would completely
       remove the need for this.)
    */
    lpCaps->wNumButtons = 32;
#else
    lpCaps->wNumButtons = nrOfButtons;
#endif
    if (dwSize == sizeof(JOYCAPSW)) {
	/* complete 95 structure */
	lpCaps->wRmin = 0;
	lpCaps->wRmax = 0xFFFF;
	lpCaps->wUmin = 0;
	lpCaps->wUmax = 0xFFFF;
	lpCaps->wVmin = 0;
	lpCaps->wVmax = 0xFFFF;
	lpCaps->wMaxAxes = 6; /* same as MS Joystick Driver */
	lpCaps->wNumAxes = 0; /* nr of axes in use */
	lpCaps->wMaxButtons = 32; /* same as MS Joystick Driver */
	lpCaps->szRegKey[0] = 0;
	lpCaps->szOEMVxD[0] = 0;
	lpCaps->wCaps = 0;
        for (i = 0; i < nrOfAxes; i++) {
	    switch (jstck->axesMap[i]) {
	    case 0: /* X */
	    case 1: /* Y */
	    case 8: /* Wheel */
	    case 9: /* Gas */
		lpCaps->wNumAxes++;
		break;
	    case 2: /* Z */
	    case 6: /* Throttle */
	    case 10: /* Brake */
		lpCaps->wNumAxes++;
		lpCaps->wCaps |= JOYCAPS_HASZ;
		break;
	    case 5: /* Rz */
	    case 7: /* Rudder */
		lpCaps->wNumAxes++;
		lpCaps->wCaps |= JOYCAPS_HASR;
		break;
	    case 3: /* Rx */
		lpCaps->wNumAxes++;
		lpCaps->wCaps |= JOYCAPS_HASU;
		break;
	    case 4: /* Ry */
		lpCaps->wNumAxes++;
		lpCaps->wCaps |= JOYCAPS_HASV;
		break;
	    case 16: /* Hat 0 X */
	    case 17: /* Hat 0 Y */
		lpCaps->wCaps |= JOYCAPS_HASPOV | JOYCAPS_POV4DIR;
		/* TODO: JOYCAPS_POVCTS handling */
		break;
	    default:
		WARN("Unknown axis %hhu(%u). Skipped.\n", jstck->axesMap[i], i);
	    }
	}
    }
#else
    lpCaps->wMid = MM_MICROSOFT;
    lpCaps->wPid = MM_PC_JOYSTICK;
    strcpyW(lpCaps->szPname, ini); /* joystick product name */
    lpCaps->wXmin = 0;
    lpCaps->wXmax = 0xFFFF;
    lpCaps->wYmin = 0;
    lpCaps->wYmax = 0xFFFF;
    lpCaps->wZmin = 0;
    lpCaps->wZmax = 0;
    lpCaps->wNumButtons = 2;
    if (dwSize == sizeof(JOYCAPSW)) {
	/* complete 95 structure */
	lpCaps->wRmin = 0;
	lpCaps->wRmax = 0;
	lpCaps->wUmin = 0;
	lpCaps->wUmax = 0;
	lpCaps->wVmin = 0;
	lpCaps->wVmax = 0;
	lpCaps->wCaps = 0;
	lpCaps->wMaxAxes = 2;
	lpCaps->wNumAxes = 2;
	lpCaps->wMaxButtons = 4;
	lpCaps->szRegKey[0] = 0;
	lpCaps->szOEMVxD[0] = 0;
    }
#endif

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				driver_joyGetPos
 */
LRESULT driver_joyGetPosEx(DWORD_PTR dwDevID, LPJOYINFOEX lpInfo)
{
    WINE_JSTCK*		jstck;
    int			dev;
#ifdef HAVE_LINUX_22_JOYSTICK_API
    struct js_event 	ev;
#else
    struct js_status 	js;
    int    		dev_stat;
#endif

    if ((jstck = JSTCK_drvGet(dwDevID)) == NULL)
	return MMSYSERR_NODRIVER;

    if ((dev = JSTCK_OpenDevice(jstck)) < 0) return JOYERR_PARMS;

#ifdef HAVE_LINUX_22_JOYSTICK_API
    while ((read(dev, &ev, sizeof(struct js_event))) > 0) {
	if (ev.type == (JS_EVENT_AXIS)) {
	    switch (jstck->axesMap[ev.number]) {
	    case 0: /* X */
	    case 8: /* Wheel */
		jstck->x = ev.value;
		break;
	    case 1: /* Y */
	    case 9: /* Gas */
		jstck->y = ev.value;
		break;
	    case 2: /* Z */
	    case 6: /* Throttle */
	    case 10: /* Brake */
		jstck->z = ev.value;
		break;
	    case 5: /* Rz */
	    case 7: /* Rudder */
		jstck->r = ev.value;
		break;
	    case 3: /* Rx */
		jstck->u = ev.value;
		break;
	    case 4: /* Ry */
		jstck->v = ev.value;
		break;
	    case 16: /* Hat 0 X */
		jstck->pov_x = ev.value;
		break;
	    case 17: /* Hat 0 Y */
		jstck->pov_y = ev.value;
		break;
	    default:
		FIXME("Unknown joystick event '%d'\n", ev.number);
	    }
	} else if (ev.type == (JS_EVENT_BUTTON)) {
	    if (ev.value) {
		    jstck->buttons |= (1 << ev.number);
		    /* FIXME: what to do for this field when
		     * multiple buttons are depressed ?
		     */
		    if (lpInfo->dwFlags & JOY_RETURNBUTTONS)
                       lpInfo->dwButtonNumber = ev.number + 1;
		}
             else
               jstck->buttons  &= ~(1 << ev.number);
	}
    }
    /* EAGAIN is returned when the queue is empty */
    if (errno != EAGAIN) {
	/* FIXME: error should not be ignored */
	ERR("Error while reading joystick state (%s)\n", strerror(errno));
    }
    /* Now, copy the cached values into Window's structure... */
    if (lpInfo->dwFlags & JOY_RETURNBUTTONS)
       lpInfo->dwButtons = jstck->buttons;
    if (lpInfo->dwFlags & JOY_RETURNX)
       lpInfo->dwXpos   = jstck->x + 32767;
    if (lpInfo->dwFlags & JOY_RETURNY)
       lpInfo->dwYpos   = jstck->y + 32767;
    if (lpInfo->dwFlags & JOY_RETURNZ)
       lpInfo->dwZpos   = jstck->z + 32767;
    if (lpInfo->dwFlags & JOY_RETURNR)
       lpInfo->dwRpos   = jstck->r + 32767;
# ifdef BODGE_THE_HAT
    else if (lpInfo->dwFlags & JOY_RETURNBUTTONS)
    {
         if (jstck->r > 0)
            lpInfo->dwButtons |= 1<<7;
         else if (jstck->r < 0)
            lpInfo->dwButtons |= 1<<8;
    }
# endif
    if (lpInfo->dwFlags & JOY_RETURNU)
	lpInfo->dwUpos   = jstck->u + 32767;
# ifdef BODGE_THE_HAT
    else if (lpInfo->dwFlags & JOY_RETURNBUTTONS)
    {
       if (jstck->u > 0)
          lpInfo->dwButtons |= 1<<9;
        else if (jstck->u < 0)
          lpInfo->dwButtons |= 1<<10;
    }
# endif
    if (lpInfo->dwFlags & JOY_RETURNV)
       lpInfo->dwVpos   = jstck->v + 32767;
    if (lpInfo->dwFlags & JOY_RETURNPOV) {
	if (jstck->pov_y > 0) {
	    if (jstck->pov_x < 0)
		lpInfo->dwPOV = 22500; /* SW */
	    else if (jstck->pov_x > 0)
		lpInfo->dwPOV = 13500; /* SE */
	    else
		lpInfo->dwPOV = 18000; /* S, JOY_POVBACKWARD */
	} else if (jstck->pov_y < 0) {
	    if (jstck->pov_x < 0)
		lpInfo->dwPOV = 31500; /* NW */
	    else if (jstck->pov_x > 0)
		lpInfo->dwPOV = 4500; /* NE */
	    else
		lpInfo->dwPOV = 0; /* N, JOY_POVFORWARD */
	} else if (jstck->pov_x < 0)
	    lpInfo->dwPOV = 27000; /* W, JOY_POVLEFT */
	else if (jstck->pov_x > 0)
	    lpInfo->dwPOV = 9000; /* E, JOY_POVRIGHT */
	else
	    lpInfo->dwPOV = JOY_POVCENTERED; /* Center */
    }

#else
    dev_stat = read(dev, &js, sizeof(js));
    if (dev_stat != sizeof(js)) {
	return JOYERR_UNPLUGGED; /* FIXME: perhaps wrong, but what should I return else ? */
    }
    js.x = js.x<<8;
    js.y = js.y<<8;
    if (lpInfo->dwFlags & JOY_RETURNX)
	lpInfo->dwXpos = js.x;   /* FIXME: perhaps multiply it somehow ? */
    if (lpInfo->dwFlags & JOY_RETURNY)
	lpInfo->dwYpos = js.y;
    if (lpInfo->dwFlags & JOY_RETURNBUTTONS)
	lpInfo->dwButtons = js.buttons;
#endif

    TRACE("x: %d, y: %d, z: %d, r: %d, u: %d, v: %d, buttons: 0x%04x, flags: 0x%04x (fd %d)\n",
	  lpInfo->dwXpos, lpInfo->dwYpos, lpInfo->dwZpos,
	  lpInfo->dwRpos, lpInfo->dwUpos, lpInfo->dwVpos,
	  lpInfo->dwButtons, lpInfo->dwFlags, dev
         );

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				driver_joyGetPos
 */
LRESULT driver_joyGetPos(DWORD_PTR dwDevID, LPJOYINFO lpInfo)
{
    JOYINFOEX	ji;
    LONG	ret;

    memset(&ji, 0, sizeof(ji));

    ji.dwSize = sizeof(ji);
    ji.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNBUTTONS;
    ret = driver_joyGetPosEx(dwDevID, &ji);
    if (ret == JOYERR_NOERROR)	{
	lpInfo->wXpos    = ji.dwXpos;
	lpInfo->wYpos    = ji.dwYpos;
	lpInfo->wZpos    = ji.dwZpos;
	lpInfo->wButtons = ji.dwButtons;
    }

    return ret;
}

#endif /* HAVE_LINUX_JOYSTICK_H */
