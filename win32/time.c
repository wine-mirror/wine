/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis and Cameron Heide
 */

#include <string.h>
#include <time.h>
#include <unistd.h>
#include "windows.h"
#include "winerror.h"
#include "kernel32.h"
#include "stddebug.h"
#include "debug.h"

/***********************************************************************
 *              GetTimeZoneInformation  (KERNEL32.302)
 */
DWORD GetTimeZoneInformation(LPTIME_ZONE_INFORMATION tzinfo)
{
    time_t gmt, lt;

    memset(tzinfo, 0, sizeof(TIME_ZONE_INFORMATION));

    gmt = time(NULL);
    lt = mktime(localtime(&gmt));
    tzinfo->Bias = (gmt - lt) / 60;
    tzinfo->StandardBias = 0;
    tzinfo->DaylightBias = -60;

    return TIME_ZONE_ID_UNKNOWN;
}

