/*
 * MMSYTEM functions
 *
 * Copyright 1993 Martin Ayotte
 */

#ifndef WINELIB

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "ldt.h"
#include "callback.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "stddebug.h"
#include "debug.h"

/**************************************************************************
 * 				JoyGetNumDevs		[MMSYSTEM.101]
 */
WORD JoyGetNumDevs(void)
{
    fprintf(stdnimp, "EMPTY STUB !!! JoyGetNumDevs();\n");
    return 0;
}

/**************************************************************************
 * 				JoyGetDevCaps		[MMSYSTEM.102]
 */
WORD JoyGetDevCaps(WORD wID, LPJOYCAPS lpCaps, WORD wSize)
{
    fprintf(stdnimp, "EMPTY STUB !!! JoyGetDevCaps(%04X, %p, %d);\n",
	    wID, lpCaps, wSize);
    return MMSYSERR_NODRIVER;
}

/**************************************************************************
 * 				JoyGetPos	       	[MMSYSTEM.103]
 */
WORD JoyGetPos(WORD wID, LPJOYINFO lpInfo)
{
    fprintf(stdnimp, "EMPTY STUB !!! JoyGetPos(%04X, %p);\n", wID, lpInfo);
    return MMSYSERR_NODRIVER;
}

/**************************************************************************
 * 				JoyGetThreshold		[MMSYSTEM.104]
 */
WORD JoyGetThreshold(WORD wID, LPWORD lpThreshold)
{
    fprintf(stdnimp, "EMPTY STUB !!! JoyGetThreshold(%04X, %p);\n", wID, lpThreshold);
    return MMSYSERR_NODRIVER;
}

/**************************************************************************
 * 				JoyReleaseCapture	[MMSYSTEM.105]
 */
WORD JoyReleaseCapture(WORD wID)
{
    fprintf(stdnimp, "EMPTY STUB !!! JoyReleaseCapture(%04X);\n", wID);
    return MMSYSERR_NODRIVER;
}

/**************************************************************************
 * 				JoySetCapture		[MMSYSTEM.106]
 */
WORD JoySetCapture(HWND hWnd, WORD wID, WORD wPeriod, BOOL bChanged)
{
    fprintf(stdnimp, "EMPTY STUB !!! JoySetCapture(%04X, %04X, %d, %d);\n",
	    hWnd, wID, wPeriod, bChanged);
    return MMSYSERR_NODRIVER;
}

/**************************************************************************
 * 				JoySetThreshold		[MMSYSTEM.107]
 */
WORD JoySetThreshold(WORD wID, WORD wThreshold)
{
    fprintf(stdnimp, "EMPTY STUB !!! JoySetThreshold(%04X, %d);\n", wID, wThreshold);
    return MMSYSERR_NODRIVER;
}

/**************************************************************************
 * 				JoySetCalibration	[MMSYSTEM.109]
 */
WORD JoySetCalibration(WORD wID)
{
    fprintf(stdnimp, "EMPTY STUB !!! JoySetCalibration(%04X);\n", wID);
    return MMSYSERR_NODRIVER;
}

#endif
