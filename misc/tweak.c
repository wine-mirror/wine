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
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "tweak.h"
#include "options.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(tweak);

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
 *        23-Aug-2000 Andreas Mohr (a.mohr@mailto.de)
 *             Speedup and code cleanup.
 *
 *****************************************************************************/

WINE_LOOK TWEAK_WineLook = WIN31_LOOK;

int TWEAK_Init (void)
{
    static const char *OS = "Win3.1";
    char szIniString[80];

    PROFILE_GetWineIniString ("Tweak.Layout", "WineLook", "Win31", szIniString, 80);

    /* WIN31_LOOK is default */
    if (!strncasecmp (szIniString, "Win95", 5)) {
        TWEAK_WineLook = WIN95_LOOK;
        OS = "Win95";
    }
    else if (!strncasecmp (szIniString, "Win98", 5)) {
        TWEAK_WineLook = WIN98_LOOK;
        OS = "Win98";
    }
    TRACE("Using %s look and feel.\n", OS);
    return 1;
}
