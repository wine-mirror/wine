/******************************************************************************
 *
 *   tweak.c
 *
 *   Windows 95 style interface tweaks.
 *   Copyright (c) 1997 Dave Cuthbert.
 *
 *   FIXME:  This file is, unfortunately, aptly named:  the method of
 *   displaying Win95 style windows is a tweak.  Lots of stuff does not yet
 *   work -- and probably never will unless some of this code is
 *   incorporated into the mainstream Wine code.
 *
 *   DEVELOPERS, PLEASE NOTE:  Before delving into the mainstream code and
 *   altering it, consider how your changes will affect the Win3.1 interface
 *   (which has taken a major effort to create!).  After you make any sort of
 *   non-trivial change, *test* the Wine code running in Win3.1 mode!  The
 *   object here is to make it so that the person who tests the latest version
 *   of Wine without adding the tweaks into wine.conf notices nothing out of
 *   the ordinary.
 *
 *   Revision history
 *        03-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *        05-Aug-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Removed some unused code.
 *        22-Sep-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Removed more unused code.
 *
 *****************************************************************************/

#include <string.h>
#include "winuser.h"
#include "tweak.h"
#include "options.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(tweak)

/* General options */

WINE_LOOK TWEAK_WineLook = WIN31_LOOK;


/******************************************************************************
 *
 *   int  TWEAK_Init()
 *
 *   Does the full initialization of the Win95 tweak subsystem.  Return value
 *   indicates success.  Called by loader/main.c's MAIN_Init().
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *        22-Sep-1998 Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *             Removed unused code and added Win98 option.
 *
 *****************************************************************************/

int TWEAK_Init (void)
{
    char szIniString[80];

    PROFILE_GetWineIniString ("Tweak.Layout", "Win95Look", "TestString",
			      szIniString, 80);
    if (strncmp (szIniString, "TestString", 10)) {
	if (PROFILE_GetWineIniBool ("Tweak.Layout", "Win95Look", 0)) {
	    TWEAK_WineLook = WIN95_LOOK;
	    TRACE (tweak, "Using Win95 look and feel.\n");
	}
	else {
	    TWEAK_WineLook = WIN31_LOOK;
	    TRACE (tweak, "Using Win3.1 look and feel.\n");
	}
	ERR (tweak,
	     "Replace \"Win95Look\" by \"WineLook\" in your \"wine.ini\"!\n");
    }

    PROFILE_GetWineIniString ("Tweak.Layout", "WineLook", "Win31",
			      szIniString, 80);

    if (!strncasecmp (szIniString, "Win31", 5)) {
	TWEAK_WineLook = WIN31_LOOK;
	TRACE (tweak, "Using Win3.1 look and feel.\n");
    }
    else if (!strncasecmp (szIniString, "Win95", 5)) {
	TWEAK_WineLook = WIN95_LOOK;
	TRACE (tweak, "Using Win95 look and feel.\n");
    }
    else if (!strncasecmp (szIniString, "Win98", 5)) {
	TWEAK_WineLook = WIN98_LOOK;
	TRACE (tweak, "Using Win98 look and feel.\n");
    }
    else {
	TWEAK_WineLook = WIN31_LOOK;
	TRACE (tweak, "Using Win3.1 look and feel.\n");
    }

    return 1;
}


/******************************************************************************
 *
 *   int  TWEAK_CheckConfiguration()
 *
 *   Examines wine.conf for old/bad entries and recommends changes to the user.
 *
 *   Revision history
 *        05-Jul-1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *             Original implementation.
 *
 *****************************************************************************/

int  TWEAK_CheckConfiguration()
{
    return 1;
}
