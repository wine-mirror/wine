/*
 * Joystick functions
 *
 * Copyright 1997 Andreas Mohr
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "ldt.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "stddebug.h"
#include "debug.h"

static int count_use[4] = {0, 0, 0, 0};
static int dev_stat;
static int joy_nr_open = 0;
static BOOL16 JoyCaptured = FALSE;
static HWND16 CaptureWnd[2] = {0, 0};
static int joy_dev[2] = {-1, -1};
static JOYINFO JoyCapData[2];
static unsigned int joy_threshold[2] = {0, 0};

struct js_status
{
    int buttons;
    int x;
    int y;
};


/**************************************************************************
 *                              JoyOpenDriver           [internal]
 */
BOOL16 JoyOpenDriver(WORD wID)
{
	char dev_name[] = "/dev/jsx";

	if (joy_dev[wID] >= 0) return TRUE;
        dev_name[strlen(dev_name)-1]=(char) wID+0x30;
        if ((joy_dev[wID] = open(dev_name, O_RDONLY)) >= 0) {
		joy_nr_open++;
		return TRUE;
	}
	else return FALSE;
}

/**************************************************************************
 *                              JoyCloseDriver           [internal]
 */
void JoyCloseDriver(WORD wID)
{
	if (joy_dev[wID] >= 0) {
		close(joy_dev[wID]);
		joy_dev[wID] = -1;
		joy_nr_open--;
	}
}

/**************************************************************************
 *                              JoySendMessages           [internal]
 */
void JoySendMessages(void)
{
	int joy;
        struct js_status js;

	if (joy_nr_open)
	for (joy=0; joy < 4; joy++) 
	if (joy_dev[joy] >= 0) {
		if (count_use[joy] > 250) {
			JoyCloseDriver(joy);
			count_use[joy] = 0;
		}
		count_use[joy]++;
	}
	else return;
        if (JoyCaptured == FALSE) return;
	dprintf_mmsys(stddeb, "JoySendMessages()\n");
        for (joy=0; joy < 4; joy++) {
		if (JoyOpenDriver(joy) == FALSE) continue;
                dev_stat = read(joy_dev[joy], &js, sizeof(js));
                if (dev_stat == sizeof(js)) {
                        js.x = js.x*37;
                        js.y = js.y*37;
                        if ((JoyCapData[joy].wXpos != js.x) || (JoyCapData[joy].wYpos != js.y)) {
                                SendMessage32A(CaptureWnd[joy], MM_JOY1MOVE + joy, js.buttons, MAKELONG(js.x, js.y));
                                JoyCapData[joy].wXpos = js.x;
                                JoyCapData[joy].wYpos = js.y;
                        }
                        if (JoyCapData[joy].wButtons != js.buttons) {
				unsigned int ButtonChanged = (WORD)(JoyCapData[joy].wButtons ^ js.buttons)<<8;
                                if (JoyCapData[joy].wButtons < js.buttons)
                                SendMessage32A(CaptureWnd[joy], MM_JOY1BUTTONDOWN + joy, ButtonChanged, MAKELONG(js.x, js.y));
				else
                                if (JoyCapData[joy].wButtons > js.buttons)
                                SendMessage32A(CaptureWnd[joy], MM_JOY1BUTTONUP
+ joy, ButtonChanged, MAKELONG(js.x, js.y));
                                JoyCapData[joy].wButtons = js.buttons;
                        }
                }
        }
}

/**************************************************************************
 * 				JoyGetNumDevs		[MMSYSTEM.101]
 */
WORD JoyGetNumDevs(void)
{
int joy;
WORD joy_cnt = 0;

    dprintf_mmsys(stddeb, "JoyGetNumDevs: ");
    for (joy=0; joy<4; joy++)
	if (JoyOpenDriver(joy) == TRUE) {		
		JoyCloseDriver(joy);
		joy_cnt++;
	}
    dprintf_mmsys(stddeb, "returning %d\n", joy_cnt);
    if (!joy_cnt) fprintf(stderr, "No joystick found - perhaps get joystick-0.8.0.tar.gz and load it as module or use Linux >= 2.1.45 to be able to use joysticks.\n");
    return joy_cnt;
}

/**************************************************************************
 * 				JoyGetDevCaps		[MMSYSTEM.102]
 */
WORD JoyGetDevCaps(WORD wID, LPJOYCAPS lpCaps, WORD wSize)
{
    dprintf_mmsys(stderr, "JoyGetDevCaps(%04X, %p, %d);\n",
            wID, lpCaps, wSize);
    if (wSize != sizeof(*lpCaps)) return JOYERR_PARMS; /* FIXME: should we really return this error value ? */
    if (JoyOpenDriver(wID) == TRUE) {
        lpCaps->wMid = MM_MICROSOFT;
        lpCaps->wPid = MM_PC_JOYSTICK;
        strcpy(lpCaps->szPname, "WineJoy\0"); /* joystick product name */
        lpCaps->wXmin = 0; /* FIXME */
        lpCaps->wXmax = 0xffff;
        lpCaps->wYmin = 0;
        lpCaps->wYmax = 0xffff;
        lpCaps->wZmin = 0;
        lpCaps->wZmax = 0xffff;
        lpCaps->wNumButtons = 2;
        lpCaps->wPeriodMin = 0;
        lpCaps->wPeriodMax = 50; /* FIXME end */

	JoyCloseDriver(wID);
        return JOYERR_NOERROR;
    }
    else
    return MMSYSERR_NODRIVER;
}

/**************************************************************************
 * 				JoyGetPos	       	[MMSYSTEM.103]
 */
WORD JoyGetPos(WORD wID, LPJOYINFO lpInfo)
{
        struct js_status js;

        dprintf_mmsys(stderr, "JoyGetPos(%04X, %p):", wID, lpInfo);
        if (JoyOpenDriver(wID) == FALSE) return MMSYSERR_NODRIVER;
	dev_stat = read(joy_dev[wID], &js, sizeof(js));
	if (dev_stat != sizeof(js)) {
		JoyCloseDriver(wID);
		return JOYERR_UNPLUGGED; /* FIXME: perhaps wrong, but what should I return else ? */
	}
	count_use[wID] = 0;
	js.x = js.x*37;
	js.y = js.y*37;
	lpInfo->wXpos = js.x;   /* FIXME: perhaps multiply it somehow ? */
	lpInfo->wYpos = js.y;
	lpInfo->wZpos = 0; /* FIXME: Don't know what to do with this value as joystick driver doesn't provide a Z value */
	lpInfo->wButtons = js.buttons;
	dprintf_mmsys(stderr, "x: %d, y: %d, buttons: %d\n", js.x, js.y, js.buttons);
	return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JoyGetThreshold		[MMSYSTEM.104]
 */
WORD JoyGetThreshold(WORD wID, LPWORD lpThreshold)
{
    dprintf_mmsys(stderr, "JoyGetThreshold(%04X, %p);\n", wID, lpThreshold);
    if (wID > 3) return JOYERR_PARMS;
    *lpThreshold = joy_threshold[wID];
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JoyReleaseCapture	[MMSYSTEM.105]
 */
WORD JoyReleaseCapture(WORD wID)
{
    dprintf_mmsys(stderr, "JoyReleaseCapture(%04X);\n", wID);
    JoyCaptured = FALSE;
    JoyCloseDriver(wID);
    joy_dev[wID] = -1;
    CaptureWnd[wID] = 0;
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JoySetCapture		[MMSYSTEM.106]
 */
WORD JoySetCapture(HWND16 hWnd, WORD wID, WORD wPeriod, BOOL16 bChanged)
{

    dprintf_mmsys(stderr, "JoySetCapture(%04X, %04X, %d, %d);\n",
	    hWnd, wID, wPeriod, bChanged);

    if (!CaptureWnd[wID]) {
	if (JoyOpenDriver(wID) == FALSE) return MMSYSERR_NODRIVER;
	JoyCaptured = TRUE;
	CaptureWnd[wID] = hWnd;
	return JOYERR_NOERROR;
    }
    else
    return JOYERR_NOCANDO; /* FIXME: what should be returned ? */
}

/**************************************************************************
 * 				JoySetThreshold		[MMSYSTEM.107]
 */
WORD JoySetThreshold(WORD wID, WORD wThreshold)
{
    dprintf_mmsys(stderr, "JoySetThreshold(%04X, %d);\n", wID, wThreshold);

    if (wID > 3) return JOYERR_PARMS;
    joy_threshold[wID] = wThreshold;
    return JOYERR_NOERROR;
}

/**************************************************************************
 * 				JoySetCalibration	[MMSYSTEM.109]
 */
WORD JoySetCalibration(WORD wID)
{
    fprintf(stderr, "EMPTY STUB !!! JoySetCalibration(%04X);\n", wID);
    return JOYERR_NOCANDO;
}
