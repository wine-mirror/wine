/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * joystick functions
 *
 * Copyright 1997 Andreas Mohr
 *
 * nearly all joystick functions can be regarded as obsolete,
 * as Linux (2.1.x) now supports extended joysticks
 * with a completely new joystick driver interface
 * new driver's docu says:
 * "For backward compatibility the old interface is still included,
 * but will be dropped in the future."
 * Thus we should implement the new interface and at most keep the old
 * routines for backward compatibility.
 */

/*
 * Wolfgang Schwotzer
 *
 *    01/2000    added support for new joystick driver
 *
 */

#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef HAVE_LINUX_JOYSTICK_H
#include <linux/joystick.h>
#define JOYDEV "/dev/js%d"
#endif
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmddk.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(joystick);

#ifdef HAVE_LINUX_JOYSTICK_H

#define MAXJOYSTICK	(JOYSTICKID2 + 1)

typedef struct tagWINE_JSTCK {
    int		joyIntf;
    int		in_use;
} WINE_JSTCK;

static	WINE_JSTCK	JSTCK_Data[MAXJOYSTICK];

/**************************************************************************
 * 				JSTCK_drvGet			[internal]	
 */
static	WINE_JSTCK*	JSTCK_drvGet(DWORD dwDevID)
{
    int	p;

    if ((dwDevID - (DWORD)JSTCK_Data) % sizeof(JSTCK_Data[0]) != 0)
	return NULL;
    p = (dwDevID - (DWORD)JSTCK_Data) / sizeof(JSTCK_Data[0]);
    if (p < 0 || p >= MAXJOYSTICK || !((WINE_JSTCK*)dwDevID)->in_use)
	return NULL;

    return (WINE_JSTCK*)dwDevID;
}

/**************************************************************************
 * 				JSTCK_drvOpen			[internal]	
 */
static	DWORD	JSTCK_drvOpen(LPSTR str, DWORD dwIntf)
{
    if (dwIntf >= MAXJOYSTICK || JSTCK_Data[dwIntf].in_use)
	return 0;

    JSTCK_Data[dwIntf].joyIntf = dwIntf;
    JSTCK_Data[dwIntf].in_use = 1;
    return (DWORD)&JSTCK_Data[dwIntf];
}

/**************************************************************************
 * 				JSTCK_drvClose			[internal]	
 */
static	DWORD	JSTCK_drvClose(DWORD dwDevID)
{
    WINE_JSTCK*	jstck = JSTCK_drvGet(dwDevID);

    if (jstck == NULL)
	return 0;
    jstck->in_use = 0;
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
    char	buf[20];
    int		flags;

    sprintf(buf, JOYDEV, jstick->joyIntf);
#ifdef HAVE_LINUX_22_JOYSTICK_API
    flags = O_RDONLY | O_NONBLOCK;
#else
    flags = O_RDONLY;
#endif
    return open(buf, flags);
}

/**************************************************************************
 * 				JoyGetDevCaps		[MMSYSTEM.102]
 */
static	LONG	JSTCK_GetDevCaps(DWORD dwDevID, LPJOYCAPSA lpCaps, DWORD dwSize)
{
    WINE_JSTCK*	jstck;
#ifdef HAVE_LINUX_22_JOYSTICK_API
    int		dev;
    char	nrOfAxes;
    char	nrOfButtons;
    char	identString[MAXPNAMELEN];
    int		driverVersion;
#endif

    if ((jstck = JSTCK_drvGet(dwDevID)) == NULL)
	return MMSYSERR_NODRIVER;

#ifdef HAVE_LINUX_22_JOYSTICK_API
    
    if ((dev = JSTCK_OpenDevice(jstck)) < 0) return JOYERR_PARMS;
    ioctl(dev, JSIOCGAXES, &nrOfAxes);
    ioctl(dev, JSIOCGBUTTONS, &nrOfButtons);
    ioctl(dev, JSIOCGVERSION, &driverVersion);
    ioctl(dev, JSIOCGNAME(sizeof(identString)), &identString);
    TRACE("Driver: 0x%06x, Name: %s, #Axes: %d, #Buttons: %d\n",
	  driverVersion, identString, nrOfAxes, nrOfButtons);
    lpCaps->wMid = MM_MICROSOFT;
    lpCaps->wPid = MM_PC_JOYSTICK;
    strncpy(lpCaps->szPname, identString, MAXPNAMELEN);
    lpCaps->szPname[MAXPNAMELEN-1] = '\0';
    lpCaps->wXmin = 0;
    lpCaps->wXmax = 0xFFFF;
    lpCaps->wYmin = 0;
    lpCaps->wYmax = 0xFFFF;
    lpCaps->wZmin = 0;
    lpCaps->wZmax = (nrOfAxes >= 3) ? 0xFFFF : 0;
    lpCaps->wNumButtons = nrOfButtons;
    if (dwSize == sizeof(JOYCAPSA)) {
	/* since we supose ntOfAxes <= 6 in the following code, do it explicitely */
	if (nrOfAxes > 6) nrOfAxes = 6;
	/* complete 95 structure */
	lpCaps->wRmin = 0;
	lpCaps->wRmax = nrOfAxes >= 4 ? 0xFFFF : 0;
	lpCaps->wUmin = 0;
	lpCaps->wUmax = nrOfAxes >= 5 ? 0xFFFF : 0;
	lpCaps->wVmin = 0;
	lpCaps->wVmax = nrOfAxes >= 6 ? 0xFFFF : 0;
	lpCaps->wMaxAxes = 6; /* same as MS Joystick Driver */
	lpCaps->wNumAxes = nrOfAxes; /* nr of axes in use */
	lpCaps->wMaxButtons = 32; /* same as MS Joystick Driver */
	strcpy(lpCaps->szRegKey, "");
	strcpy(lpCaps->szOEMVxD, "");
	lpCaps->wCaps = 0;
	switch(nrOfAxes) {
	case 6: lpCaps->wCaps |= JOYCAPS_HASV;
	case 5: lpCaps->wCaps |= JOYCAPS_HASU;
	case 4: lpCaps->wCaps |= JOYCAPS_HASR;
	case 3: lpCaps->wCaps |= JOYCAPS_HASZ;
	    /* FIXME: don't know how to detect for
	       JOYCAPS_HASPOV, JOYCAPS_POV4DIR, JOYCAPS_POVCTS */
	}
    }
    close(dev);

#else
    lpCaps->wMid = MM_MICROSOFT;
    lpCaps->wPid = MM_PC_JOYSTICK;
    strcpy(lpCaps->szPname, "WineJoy"); /* joystick product name */
    lpCaps->wXmin = 0;
    lpCaps->wXmax = 0xFFFF;
    lpCaps->wYmin = 0;
    lpCaps->wYmax = 0xFFFF;
    lpCaps->wZmin = 0;
    lpCaps->wZmax = 0;
    lpCaps->wNumButtons = 2;
    if (dwSize == sizeof(JOYCAPSA)) {
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
	strcpy(lpCaps->szRegKey,"");
	strcpy(lpCaps->szOEMVxD,"");
    }
#endif

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JSTCK_GetPos			[internal]
 */
static LONG	JSTCK_GetPosEx(DWORD dwDevID, LPJOYINFOEX lpInfo)
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
    /* After opening the device it's state can be
       read with JS_EVENT_INIT flag */
    while ((read(dev, &ev, sizeof(struct js_event))) > 0) {
	if (ev.type == (JS_EVENT_AXIS | JS_EVENT_INIT)) {
	    switch (ev.number) {
	    case 0:
		if (lpInfo->dwFlags & JOY_RETURNX)
		    lpInfo->dwXpos   = ev.value + 32767;
		break;
	    case 1: 
		if (lpInfo->dwFlags & JOY_RETURNY)
		    lpInfo->dwYpos   = ev.value + 32767;
		break;
	    case 2:
		if (lpInfo->dwFlags & JOY_RETURNZ)
		    lpInfo->dwZpos   = ev.value + 32767;
		break;
	    case 3: 
		if (lpInfo->dwFlags & JOY_RETURNR)
		    lpInfo->dwRpos   = ev.value + 32767;
	    case 4: 
		if (lpInfo->dwFlags & JOY_RETURNU)
		    lpInfo->dwUpos   = ev.value + 32767;
	    case 5: 
		if (lpInfo->dwFlags & JOY_RETURNV)
		    lpInfo->dwVpos   = ev.value + 32767;
		break;
	    default: 
		FIXME("Unknown joystick event '%d'\n", ev.number);
	    }
	} else if (ev.type == (JS_EVENT_BUTTON | JS_EVENT_INIT)) {
	    if (lpInfo->dwFlags & JOY_RETURNBUTTONS) {
		if (ev.value) {
		    lpInfo->dwButtons |= (1 << ev.number);
		    /* FIXME: what to do for this field when 
		     * multiple buttons are depressed ?
		     */
		    lpInfo->dwButtonNumber = ev.number + 1;
		}
	    }
	}
    }
    /* EAGAIN is returned when the queue is empty */
    if (errno != EAGAIN) {
	/* FIXME: error should not be ignored */
	ERR("Error while reading joystick state (%d)\n", errno);
    }
#else
    dev_stat = read(dev, &js, sizeof(js));
    if (dev_stat != sizeof(js)) {
	close(dev);
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

    close(dev);

    TRACE("x: %ld, y: %ld, z: %ld, r: %ld, u: %ld, v: %ld, buttons: 0x%04x, flags: 0x%04x\n", 
	  lpInfo->dwXpos, lpInfo->dwYpos, lpInfo->dwZpos,
	  lpInfo->dwRpos, lpInfo->dwUpos, lpInfo->dwVpos,
	  (unsigned int)lpInfo->dwButtons,
	  (unsigned int)lpInfo->dwFlags);

    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JSTCK_GetPos			[internal]
 */
static LONG	JSTCK_GetPos(DWORD dwDevID, LPJOYINFO lpInfo)
{
    JOYINFOEX	ji;
    LONG	ret;

    memset(&ji, 0, sizeof(ji));

    ji.dwSize = sizeof(ji);
    ji.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNBUTTONS;
    ret = JSTCK_GetPosEx(dwDevID, &ji);
    if (ret == JOYERR_NOERROR)	{
	lpInfo->wXpos    = ji.dwXpos;
	lpInfo->wYpos    = ji.dwYpos;
	lpInfo->wZpos    = ji.dwZpos;
	lpInfo->wButtons = ji.dwButtons;
    }

    return ret;
}

/**************************************************************************
 * 				JSTCK_DriverProc		[internal]
 */
LONG CALLBACK	JSTCK_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
				 DWORD dwParam1, DWORD dwParam2)
{
    /* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
    /* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */
    
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return JSTCK_drvOpen((LPSTR)dwParam1, dwParam2);
    case DRV_CLOSE:		return JSTCK_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "JoyStick MultiMedia Driver !", "JoyStick Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case JDD_GETNUMDEVS:	return 1;
    case JDD_GETDEVCAPS:	return JSTCK_GetDevCaps(dwDevID, (LPJOYCAPSA)dwParam1, dwParam2);
    case JDD_GETPOS:		return JSTCK_GetPos(dwDevID, (LPJOYINFO)dwParam1);
    case JDD_SETCALIBRATION:	
    case JDD_CONFIGCHANGED:	return JOYERR_NOCANDO;
    case JDD_GETPOSEX:		return JSTCK_GetPosEx(dwDevID, (LPJOYINFOEX)dwParam1);
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}

#else

/**************************************************************************
 * 				JSTCK_DriverProc		[internal]
 */
LONG CALLBACK	JSTCK_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
				 DWORD dwParam1, DWORD dwParam2)
{
    /* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
    /* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */
    
    switch(wMsg) {
    case DRV_LOAD:		
    case DRV_FREE:		
    case DRV_OPEN:		
    case DRV_CLOSE:		
    case DRV_ENABLE:		
    case DRV_DISABLE:		
    case DRV_QUERYCONFIGURE:	return 0;
    case DRV_CONFIGURE:		MessageBoxA(0, "JoyStick MultiMedia Driver !", "JoyStick Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}

#endif
