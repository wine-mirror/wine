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
#include "winbase.h"
#include "mmsystem.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mmsys);

#define MAXJOYSTICK	(JOYSTICKID2 + 1)
#define JOY_PERIOD_MIN	(10)	/* min Capture time period */
#define JOY_PERIOD_MAX	(1000)	/* max Capture time period */

static int count_use[MAXJOYSTICK] = {0, 0};
static int joy_nr_open = 0;
static BOOL16 joyCaptured = FALSE;
static HWND16 CaptureWnd[MAXJOYSTICK] = {0, 0};
static int joy_dev[MAXJOYSTICK] = {-1, -1};
static JOYINFO16 joyCapData[MAXJOYSTICK];
static unsigned int joy_threshold[MAXJOYSTICK] = {0, 0};

struct js_status
{
    int buttons;
    int x;
    int y;
};


/**************************************************************************
 *                              joyOpenDriver           [internal]
 */
BOOL16 joyOpenDriver(WORD wID)
{
	char	buf[20];
	int	flags;

	if (wID>=MAXJOYSTICK) return FALSE;
	if (joy_dev[wID] >= 0) return TRUE; /* was already open */
#ifdef HAVE_LINUX_JOYSTICK_H
        sprintf(buf,JOYDEV,wID);
	flags = O_RDONLY;
#ifdef HAVE_LINUX_22_JOYSTICK_API
	flags |= O_NONBLOCK;
#endif
        if ((joy_dev[wID] = open(buf, flags)) >= 0) {
		joy_nr_open++;
		return TRUE;
	} else
		return FALSE;
#else
	return FALSE;
#endif /* HAVE_LINUX_JOYSTICK_H */
}

/**************************************************************************
 *                              joyCloseDriver           [internal]
 */
void joyCloseDriver(WORD wID)
{
	if (joy_dev[wID] >= 0) {
		close(joy_dev[wID]);
		joy_dev[wID] = -1;
		joy_nr_open--;
	}
}

/**************************************************************************
 *                              joyUpdateCaptureData           [internal]
 */
#ifdef HAVE_LINUX_22_JOYSTICK_API
void joyUpdateCaptureData(WORD wID)
{
	struct 		js_event ev;
	unsigned int	ButtonState;
	int		messageType = 0;

	if (joyOpenDriver(wID) == FALSE) return;
	while (read(joy_dev[wID], &ev, sizeof(struct js_event)) > 0) {
       		if (ev.type & JS_EVENT_AXIS) {
			if (ev.number == 0)
				joyCapData[wID].wXpos = ev.value + 32767;
			if (ev.number == 1)
				joyCapData[wID].wYpos = ev.value + 32767;
			if (ev.number <= 1) /* Message for X, Y-Axis */
			   SendMessageA(CaptureWnd[wID], MM_JOY1MOVE + wID,
				joyCapData[wID].wButtons,
				MAKELONG(joyCapData[wID].wXpos,
					joyCapData[wID].wYpos));
			if (ev.number == 2) { /* Message for Z-axis */
				joyCapData[wID].wZpos = ev.value + 32767;
			   SendMessageA(CaptureWnd[wID], MM_JOY1ZMOVE + wID,
				joyCapData[wID].wButtons,
				joyCapData[wID].wZpos);
			}
		} else if (ev.type & JS_EVENT_BUTTON) {
			if (ev.value) {
                        	joyCapData[wID].wButtons |= (1 << ev.number);
				messageType = MM_JOY1BUTTONDOWN;
                	} else {
                        	joyCapData[wID].wButtons &= ~(1 << ev.number);
				messageType = MM_JOY1BUTTONUP;
			}
			/* create button state with changed buttons and 
			   pressed buttons */
			ButtonState = ((1 << (ev.number + 8)) & 0xFF00) |
					(joyCapData[wID].wButtons & 0xFF);
			SendMessageA(CaptureWnd[wID], messageType + wID,
				ButtonState,
				MAKELONG(joyCapData[wID].wXpos,
					joyCapData[wID].wYpos));
		}
	}
	/* EAGAIN is returned when the queue is empty */
	if (errno != EAGAIN) {
		/* FIXME: error should not be ignored */
	}
}
#endif

/**************************************************************************
 *                              joySendMessages           [internal]
 */
void joySendMessages(void)
{
	int joy;
#ifndef HAVE_LINUX_22_JOYSTICK_API
        struct js_status js;
#endif
	if (joy_nr_open)
        {
            for (joy=0; joy < MAXJOYSTICK; joy++) 
		if (joy_dev[joy] >= 0) {
			if (count_use[joy] > 250) {
				joyCloseDriver(joy);
				count_use[joy] = 0;
			}
			count_use[joy]++;
		} else
			return;
        }
        if (joyCaptured == FALSE) return;

	TRACE(" --\n");

#ifdef HAVE_LINUX_22_JOYSTICK_API
       	for (joy=0; joy < MAXJOYSTICK; joy++)
		joyUpdateCaptureData(joy);
	return;
#else
        for (joy=0; joy < MAXJOYSTICK; joy++) {
		int dev_stat;

		if (joyOpenDriver(joy) == FALSE) continue;
                dev_stat = read(joy_dev[joy], &js, sizeof(js));
                if (dev_stat == sizeof(js)) {
                        js.x = js.x<<8;
                        js.y = js.y<<8;
                        if ((joyCapData[joy].wXpos != js.x) || (joyCapData[joy].wYpos != js.y)) {
                                SendMessageA(CaptureWnd[joy], MM_JOY1MOVE + joy, js.buttons, MAKELONG(js.x, js.y));
                                joyCapData[joy].wXpos = js.x;
                                joyCapData[joy].wYpos = js.y;
                        }
                        if (joyCapData[joy].wButtons != js.buttons) {
				unsigned int ButtonChanged = (WORD)(joyCapData[joy].wButtons ^ js.buttons)<<8;
                                if (joyCapData[joy].wButtons < js.buttons)
                                SendMessageA(CaptureWnd[joy], MM_JOY1BUTTONDOWN + joy, ButtonChanged, MAKELONG(js.x, js.y));
				else
                                if (joyCapData[joy].wButtons > js.buttons)
                                SendMessageA(CaptureWnd[joy], MM_JOY1BUTTONUP
+ joy, ButtonChanged, MAKELONG(js.x, js.y));
                                joyCapData[joy].wButtons = js.buttons;
                        }
                }
        }
#endif
}


/**************************************************************************
 * 				JoyGetNumDevs		[MMSYSTEM.101]
 */
UINT WINAPI joyGetNumDevs(void)
{
	return joyGetNumDevs16();
}

/**************************************************************************
 * 				JoyGetNumDevs		[MMSYSTEM.101]
 */
UINT16 WINAPI joyGetNumDevs16(void)
{
/*    int joy;
    UINT16 joy_cnt = 0;

    for (joy=0; joy<MAXJOYSTICK; joy++)
	if (joyOpenDriver(joy) == TRUE) {		
		joyCloseDriver(joy);
		joy_cnt++;
    }
    TRACE("returning %d\n", joy_cnt);
    if (!joy_cnt) ERR("No joystick found - "
			  "perhaps get joystick-0.8.0.tar.gz and load"
			  "it as module or use Linux >= 2.1.45 to be "
			  "able to use joysticks.\n");
    return joy_cnt;*/

/* simply return the max. nr. of supported joysticks. The rest
   will be done with joyGetPos or joyGetDevCaps. Look at
   MS Joystick Driver */

    return MAXJOYSTICK;
}

/**************************************************************************
 * 				JoyGetDevCaps		[WINMM.27]
 */
MMRESULT WINAPI joyGetDevCapsA(UINT wID, LPJOYCAPSA lpCaps,UINT wSize)
{
	JOYCAPS16	jc16;
	MMRESULT16	ret = joyGetDevCaps16(wID,&jc16,sizeof(jc16));

	if (ret != JOYERR_NOERROR) return ret;
	lpCaps->wMid = jc16.wMid;
	lpCaps->wPid = jc16.wPid;
	strcpy(lpCaps->szPname,jc16.szPname);
        lpCaps->wXmin = jc16.wXmin;
        lpCaps->wXmax = jc16.wXmax;
        lpCaps->wYmin = jc16.wYmin;
        lpCaps->wYmax = jc16.wYmax;
        lpCaps->wZmin = jc16.wZmin;
        lpCaps->wZmax = jc16.wZmax;
        lpCaps->wNumButtons = jc16.wNumButtons;
        lpCaps->wPeriodMin = jc16.wPeriodMin;
        lpCaps->wPeriodMax = jc16.wPeriodMax;

        lpCaps->wRmin = jc16.wRmin;
        lpCaps->wRmax = jc16.wRmax;
        lpCaps->wUmin = jc16.wUmin;
        lpCaps->wUmax = jc16.wUmax;
        lpCaps->wVmin = jc16.wVmin;
        lpCaps->wVmax = jc16.wVmax;
        lpCaps->wCaps = jc16.wCaps;
        lpCaps->wMaxAxes = jc16.wMaxAxes;
        lpCaps->wNumAxes = jc16.wNumAxes;
        lpCaps->wMaxButtons = jc16.wMaxButtons;
        strcpy(lpCaps->szRegKey,jc16.szRegKey);
        strcpy(lpCaps->szOEMVxD,jc16.szOEMVxD);
	return ret;
}

/**************************************************************************
 * 				JoyGetDevCaps		[WINMM.28]
 */
MMRESULT WINAPI joyGetDevCapsW(UINT wID, LPJOYCAPSW lpCaps,UINT wSize)
{
	JOYCAPS16	jc16;
	MMRESULT16	ret = joyGetDevCaps16(wID,&jc16,sizeof(jc16));

	if (ret != JOYERR_NOERROR) return ret;
	lpCaps->wMid = jc16.wMid;
	lpCaps->wPid = jc16.wPid;
	lstrcpyAtoW(lpCaps->szPname,jc16.szPname);
        lpCaps->wXmin = jc16.wXmin;
        lpCaps->wXmax = jc16.wXmax;
        lpCaps->wYmin = jc16.wYmin;
        lpCaps->wYmax = jc16.wYmax;
        lpCaps->wZmin = jc16.wZmin;
        lpCaps->wZmax = jc16.wZmax;
        lpCaps->wNumButtons = jc16.wNumButtons;
        lpCaps->wPeriodMin = jc16.wPeriodMin;
        lpCaps->wPeriodMax = jc16.wPeriodMax;

        lpCaps->wRmin = jc16.wRmin;
        lpCaps->wRmax = jc16.wRmax;
        lpCaps->wUmin = jc16.wUmin;
        lpCaps->wUmax = jc16.wUmax;
        lpCaps->wVmin = jc16.wVmin;
        lpCaps->wVmax = jc16.wVmax;
        lpCaps->wCaps = jc16.wCaps;
        lpCaps->wMaxAxes = jc16.wMaxAxes;
        lpCaps->wNumAxes = jc16.wNumAxes;
        lpCaps->wMaxButtons = jc16.wMaxButtons;
        lstrcpyAtoW(lpCaps->szRegKey,jc16.szRegKey);
        lstrcpyAtoW(lpCaps->szOEMVxD,jc16.szOEMVxD);
	return ret;
}
/**************************************************************************
 * 				JoyGetDevCaps		[MMSYSTEM.102]
 */
MMRESULT16 WINAPI joyGetDevCaps16(UINT16 wID, LPJOYCAPS16 lpCaps, UINT16 wSize)
{
    TRACE("(%04X, %p, %d);\n",
            wID, lpCaps, wSize);
    if (wID >= MAXJOYSTICK) return MMSYSERR_NODRIVER;
#ifdef HAVE_LINUX_22_JOYSTICK_API
    if (joyOpenDriver(wID) == TRUE) {
	char	nrOfAxes;
	char	nrOfButtons;
	char	identString[MAXPNAMELEN];
	int	driverVersion;

	ioctl(joy_dev[wID], JSIOCGAXES, &nrOfAxes);
        ioctl(joy_dev[wID], JSIOCGBUTTONS, &nrOfButtons);
        ioctl(joy_dev[wID], JSIOCGVERSION, &driverVersion);
        ioctl(joy_dev[wID], JSIOCGNAME(sizeof(identString)),
		&identString);
	TRACE("Driver: 0x%06x, Name: %s, #Axes: %d, #Buttons: %d\n",
		driverVersion, identString, nrOfAxes, nrOfButtons);
       	lpCaps->wMid = MM_MICROSOFT;
       	lpCaps->wPid = MM_PC_JOYSTICK;
       	strncpy(lpCaps->szPname, identString, MAXPNAMELEN);
	lpCaps->szPname[MAXPNAMELEN-1]='\0';
       	lpCaps->wXmin = 0;
       	lpCaps->wXmax = 0xFFFF;
       	lpCaps->wYmin = 0;
       	lpCaps->wYmax = 0xFFFF;
       	lpCaps->wZmin = 0;
       	lpCaps->wZmax = nrOfAxes >= 3 ? 0xFFFF : 0;
       	lpCaps->wNumButtons = nrOfButtons;
       	lpCaps->wPeriodMin = JOY_PERIOD_MIN; /* FIXME */
       	lpCaps->wPeriodMax = JOY_PERIOD_MAX; /* FIXME (same as MS Joystick Driver */
	if (wSize == sizeof(JOYCAPS16)) {
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
		strcpy(lpCaps->szRegKey,"");
		strcpy(lpCaps->szOEMVxD,"");
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
	joyCloseDriver(wID);
        return JOYERR_NOERROR;
    } else
    	return JOYERR_PARMS;
#else
    if (joyOpenDriver(wID) == TRUE) {
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
        lpCaps->wPeriodMin = JOY_PERIOD_MIN; /* FIXME */
        lpCaps->wPeriodMax = JOY_PERIOD_MAX; /* FIXME end */
	if (wSize == sizeof(JOYCAPS16)) {
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
	joyCloseDriver(wID);
        return JOYERR_NOERROR;
    } else
    	return JOYERR_PARMS;
#endif
}

/**************************************************************************
 *                              JoyGetPosEx             [WINMM.31]
 */
MMRESULT WINAPI joyGetPosEx(UINT wID, LPJOYINFOEX lpInfo)
{
	return joyGetPosEx16(wID, lpInfo);
}

/**************************************************************************
 *                              JoyGetPosEx             [WINMM.31]
 */
MMRESULT16 WINAPI joyGetPosEx16(UINT16 wID, LPJOYINFOEX lpInfo)
{

    TRACE("(%04X, %p)\n", wID, lpInfo);

    if (wID < MAXJOYSTICK) {
#ifdef HAVE_LINUX_22_JOYSTICK_API
	struct js_event ev;

	joyCloseDriver(wID);
       	if (joyOpenDriver(wID) == FALSE) return JOYERR_PARMS;
	lpInfo->dwSize = sizeof(JOYINFOEX);
	lpInfo->dwXpos = lpInfo->dwYpos =lpInfo->dwZpos = 0;
	lpInfo->dwButtons = lpInfo->dwFlags = 0;
	/* After opening the device it's state can be
	   read with JS_EVENT_INIT flag */
	while ((read(joy_dev[wID], &ev, sizeof(struct js_event))) > 0) {
       		if (ev.type == (JS_EVENT_AXIS | JS_EVENT_INIT)) {
			switch (ev.number) {
			 case 0: lpInfo->dwXpos   = ev.value + 32767;
				 lpInfo->dwFlags |= JOY_RETURNX; break;
			 case 1: lpInfo->dwYpos   = ev.value + 32767;
				 lpInfo->dwFlags |= JOY_RETURNY; break;
			 case 2: lpInfo->dwZpos   = ev.value + 32767;
				 lpInfo->dwFlags |= JOY_RETURNZ; break;
			 case 3: lpInfo->dwRpos   = ev.value + 32767;
				 lpInfo->dwFlags |= JOY_RETURNR; break;
			 case 4: lpInfo->dwUpos   = ev.value + 32767;
				 lpInfo->dwFlags |= JOY_RETURNU; break;
			 case 5: lpInfo->dwVpos   = ev.value + 32767;
				 lpInfo->dwFlags |= JOY_RETURNV; break;
			}
		} else if (ev.type ==
			(JS_EVENT_BUTTON | JS_EVENT_INIT)) {
			if (ev.value)
                       		lpInfo->dwButtons |= (1 << ev.number);
               		else
                       		lpInfo->dwButtons &= ~(1 << ev.number);
			lpInfo->dwFlags |= JOY_RETURNBUTTONS;
		}
	}
	/* EAGAIN is returned when the queue is empty */
	if (errno != EAGAIN) {
		/* FIXME: error should not be ignored */
	}
	joyCloseDriver(wID);
	TRACE("x: %ld, y: %ld, z: %ld, r: %ld, u: %ld, v: %ld, \
		buttons: 0x%04x, flags: 0x%04x\n", 
		lpInfo->dwXpos, lpInfo->dwYpos, lpInfo->dwZpos,
		lpInfo->dwRpos, lpInfo->dwUpos, lpInfo->dwVpos,
		(unsigned int)lpInfo->dwButtons,
		(unsigned int)lpInfo->dwFlags);
	return JOYERR_NOERROR;
#else
        struct js_status js;
       	int    dev_stat;
 
	if (joyOpenDriver(wID) == FALSE) return JOYERR_UNPLUGGED;
	dev_stat = read(joy_dev[wID], &js, sizeof(js));
	if (dev_stat != sizeof(js)) {
		joyCloseDriver(wID);
		return JOYERR_UNPLUGGED; /* FIXME: perhaps wrong, but what should I return else ? */
	}
	count_use[wID] = 0;
	js.x = js.x<<8;
	js.y = js.y<<8;
	lpInfo->dwXpos = js.x;   /* FIXME: perhaps multiply it somehow ? */
	lpInfo->dwYpos = js.y;
	lpInfo->dwZpos = 0;
	lpInfo->dwButtons = js.buttons;
	lpInfo->dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS;
	TRACE("x: %ld, y: %ld, buttons: 0x%04x, flags: 0x%04x\n",
		lpInfo->dwXpos, lpInfo->dwYpos,
		(unsigned int)lpInfo->dwButtons,
		(unsigned int)lpInfo->dwFlags);
	return JOYERR_NOERROR;
#endif
    } else
    	return JOYERR_PARMS;
}

/**************************************************************************
 * 				JoyGetPos	       	[WINMM.30]
 */
MMRESULT WINAPI joyGetPos(UINT wID, LPJOYINFO lpInfo)
{
	JOYINFOEX	ji;
	MMRESULT	ret;

        TRACE("(%d, %p);\n", wID, lpInfo);

	ret = joyGetPosEx16(wID,&ji);
	lpInfo->wXpos = ji.dwXpos;
	lpInfo->wYpos = ji.dwYpos;
	lpInfo->wZpos = ji.dwZpos;
	lpInfo->wButtons = ji.dwButtons;
	return ret;
}

/**************************************************************************
 * 				JoyGetPos16	       	[MMSYSTEM.103]
 */
MMRESULT16 WINAPI joyGetPos16(UINT16 wID, LPJOYINFO16 lpInfo)
{
	JOYINFOEX	ji;
	MMRESULT16	ret;

        TRACE("(%d, %p);\n", wID, lpInfo);

	ret = joyGetPosEx16(wID,&ji);
	lpInfo->wXpos = ji.dwXpos;
	lpInfo->wYpos = ji.dwYpos;
	lpInfo->wZpos = ji.dwZpos;
	lpInfo->wButtons = ji.dwButtons;
	return ret;
}

/**************************************************************************
 * 				JoyGetThreshold		[WINMM.32]
 */
MMRESULT WINAPI joyGetThreshold(UINT wID, LPUINT lpThreshold)
{
	UINT16		thresh;
	MMRESULT16	ret = joyGetThreshold16(wID,&thresh);

	*lpThreshold = thresh;
	return ret;
}

/**************************************************************************
 * 				JoyGetThreshold		[MMSYSTEM.104]
 */
MMRESULT16 WINAPI joyGetThreshold16(UINT16 wID, LPUINT16 lpThreshold)
{
    TRACE("(%04X, %p);\n", wID, lpThreshold);
    if (wID >= MAXJOYSTICK) return MMSYSERR_INVALPARAM;
    *lpThreshold = joy_threshold[wID];
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JoyReleaseCapture	[WINMM.33]
 */
MMRESULT WINAPI joyReleaseCapture(UINT wID)
{
	return joyReleaseCapture16(wID);
}

/**************************************************************************
 * 				JoyReleaseCapture	[MMSYSTEM.105]
 */
MMRESULT16 WINAPI joyReleaseCapture16(UINT16 wID)
{
    TRACE("(%04X);\n", wID);
    if (wID >= MAXJOYSTICK) return MMSYSERR_INVALPARAM;
    joyCaptured = FALSE;
    joyCloseDriver(wID);
    joy_dev[wID] = -1;
    CaptureWnd[wID] = 0;
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JoySetCapture		[MMSYSTEM.106]
 */
MMRESULT WINAPI joySetCapture(HWND hWnd,UINT wID,UINT wPeriod,BOOL bChanged)
{
	return joySetCapture16(hWnd,wID,wPeriod,bChanged);
}

/**************************************************************************
 * 				JoySetCapture		[MMSYSTEM.106]
 */
MMRESULT16 WINAPI joySetCapture16(HWND16 hWnd,UINT16 wID,UINT16 wPeriod,BOOL16 bChanged)
{

    TRACE("(%04X, %04X, %d, %d);\n",
	    hWnd, wID, wPeriod, bChanged);
    if (wID >= MAXJOYSTICK) return JOYERR_PARMS;
    if (hWnd == 0) return JOYERR_PARMS;
    if (wPeriod<JOY_PERIOD_MIN || wPeriod>JOY_PERIOD_MAX) return JOYERR_PARMS;
    if (!CaptureWnd[wID]) {
	if (joyOpenDriver(wID) == FALSE) return JOYERR_PARMS;
	joyCaptured = TRUE;
	CaptureWnd[wID] = hWnd;
	joyCapData[wID].wXpos = 0;
	joyCapData[wID].wYpos = 0;
	joyCapData[wID].wZpos = 0;
	joyCapData[wID].wButtons = 0;
	return JOYERR_NOERROR;
    } else
    return JOYERR_NOCANDO; /* FIXME: what should be returned ? */
}

/**************************************************************************
 * 				JoySetThreshold		[WINMM.35]
 */
MMRESULT WINAPI joySetThreshold(UINT wID, UINT wThreshold)
{
	return joySetThreshold16(wID,wThreshold);
}
/**************************************************************************
 * 				JoySetThreshold		[MMSYSTEM.107]
 */
MMRESULT16 WINAPI joySetThreshold16(UINT16 wID, UINT16 wThreshold)
{
    TRACE("(%04X, %d);\n", wID, wThreshold);

    if (wID >= MAXJOYSTICK) return MMSYSERR_INVALPARAM;
    joy_threshold[wID] = wThreshold;
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JoySetCalibration	[MMSYSTEM.109]
 */
MMRESULT16 WINAPI joySetCalibration16(UINT16 wID)
{
    FIXME("(%04X): stub.\n", wID);
    return JOYERR_NOCANDO;
}
