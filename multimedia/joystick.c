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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include "winuser.h"
#include "winbase.h"
#include "mmsystem.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mmsys)

#define MAXJOYDRIVERS	4

static int count_use[MAXJOYDRIVERS] = {0, 0, 0, 0};
static int dev_stat;
static int joy_nr_open = 0;
static BOOL16 joyCaptured = FALSE;
static HWND16 CaptureWnd[MAXJOYDRIVERS] = {0, 0};
static int joy_dev[MAXJOYDRIVERS] = {-1, -1,-1,-1};
static JOYINFO16 joyCapData[MAXJOYDRIVERS];
static unsigned int joy_threshold[MAXJOYDRIVERS] = {0, 0, 0, 0};

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
	char dev_name[] = "/dev/js%d";
	char	buf[20];

	if (wID>3) return FALSE;
	if (joy_dev[wID] >= 0) return TRUE;
        sprintf(buf,dev_name,wID);
        if ((joy_dev[wID] = open(buf, O_RDONLY)) >= 0) {
		joy_nr_open++;
		return TRUE;
	} else
		return FALSE;
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
 *                              joySendMessages           [internal]
 */
void joySendMessages(void)
{
	int joy;
        struct js_status js;

	if (joy_nr_open)
        {
            for (joy=0; joy < MAXJOYDRIVERS; joy++) 
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

        for (joy=0; joy < MAXJOYDRIVERS; joy++) {
		if (joyOpenDriver(joy) == FALSE) continue;
                dev_stat = read(joy_dev[joy], &js, sizeof(js));
                if (dev_stat == sizeof(js)) {
                        js.x = js.x*37;
                        js.y = js.y*37;
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
    int joy;
    UINT16 joy_cnt = 0;

    for (joy=0; joy<MAXJOYDRIVERS; joy++)
	if (joyOpenDriver(joy) == TRUE) {		
		joyCloseDriver(joy);
		joy_cnt++;
    }
    TRACE("returning %d\n", joy_cnt);
    if (!joy_cnt) ERR("No joystick found - "
			  "perhaps get joystick-0.8.0.tar.gz and load"
			  "it as module or use Linux >= 2.1.45 to be "
			  "able to use joysticks.\n");
    return joy_cnt;
}

/**************************************************************************
 * 				JoyGetDevCaps		[WINMM.27]
 */
MMRESULT WINAPI joyGetDevCapsA(UINT wID, LPJOYCAPSA lpCaps,UINT wSize)
{
	JOYCAPS16	jc16;
	MMRESULT16	ret = joyGetDevCaps16(wID,&jc16,sizeof(jc16));

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
    if (joyOpenDriver(wID) == TRUE) {
        lpCaps->wMid = MM_MICROSOFT;
        lpCaps->wPid = MM_PC_JOYSTICK;
        strcpy(lpCaps->szPname, "WineJoy"); /* joystick product name */
        lpCaps->wXmin = 0; /* FIXME */
        lpCaps->wXmax = 0xffff;
        lpCaps->wYmin = 0;
        lpCaps->wYmax = 0xffff;
        lpCaps->wZmin = 0;
        lpCaps->wZmax = 0xffff;
        lpCaps->wNumButtons = 2;
        lpCaps->wPeriodMin = 0;
        lpCaps->wPeriodMax = 50; /* FIXME end */
	if (wSize == sizeof(JOYCAPS16)) {
		/* complete 95 structure */
		lpCaps->wRmin = 0;
		lpCaps->wRmax = 0xffff;
		lpCaps->wUmin = 0;
		lpCaps->wUmax = 0xffff;
		lpCaps->wVmin = 0;
		lpCaps->wVmax = 0xffff;
		lpCaps->wCaps = 0;
		lpCaps->wMaxAxes = 6;
		lpCaps->wNumAxes = 2;
		lpCaps->wMaxButtons = 3;
		strcpy(lpCaps->szRegKey,"");
		strcpy(lpCaps->szOEMVxD,"");
	}
	joyCloseDriver(wID);
        return JOYERR_NOERROR;
    }
    else
    return MMSYSERR_NODRIVER;
}

/**************************************************************************
 *                              JoyGetPosEx             [WINMM.31]
 */
MMRESULT WINAPI joyGetPosEx(UINT wID, LPJOYINFOEX lpInfo)
{
	/* FIXME: implement it */
	return MMSYSERR_NODRIVER;
}

/**************************************************************************
 * 				JoyGetPos	       	[WINMM.30]
 */
MMRESULT WINAPI joyGetPos(UINT wID, LPJOYINFO lpInfo)
{
	JOYINFO16	ji;
	MMRESULT16	ret = joyGetPos16(wID,&ji);

	lpInfo->wXpos = ji.wXpos;
	lpInfo->wYpos = ji.wYpos;
	lpInfo->wZpos = ji.wZpos;
	lpInfo->wButtons = ji.wButtons;
	return ret;
}

/**************************************************************************
 * 				JoyGetPos	       	[MMSYSTEM.103]
 */
MMRESULT16 WINAPI joyGetPos16(UINT16 wID, LPJOYINFO16 lpInfo)
{
        struct js_status js;

        TRACE("(%04X, %p)\n", wID, lpInfo);
        if (joyOpenDriver(wID) == FALSE) return MMSYSERR_NODRIVER;
	dev_stat = read(joy_dev[wID], &js, sizeof(js));
	if (dev_stat != sizeof(js)) {
		joyCloseDriver(wID);
		return JOYERR_UNPLUGGED; /* FIXME: perhaps wrong, but what should I return else ? */
	}
	count_use[wID] = 0;
	js.x = js.x*37;
	js.y = js.y*37;
	lpInfo->wXpos = js.x;   /* FIXME: perhaps multiply it somehow ? */
	lpInfo->wYpos = js.y;
	lpInfo->wZpos = 0; /* FIXME: Don't know what to do with this value as joystick driver doesn't provide a Z value */
	lpInfo->wButtons = js.buttons;
	TRACE("x: %d, y: %d, buttons: %d\n", js.x, js.y, js.buttons);
	return JOYERR_NOERROR;
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
    if (wID >= MAXJOYDRIVERS) return JOYERR_PARMS;
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

    if (!CaptureWnd[wID]) {
	if (joyOpenDriver(wID) == FALSE) return MMSYSERR_NODRIVER;
	joyCaptured = TRUE;
	CaptureWnd[wID] = hWnd;
	return JOYERR_NOERROR;
    }
    else
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

    if (wID > 3) return JOYERR_PARMS;
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
