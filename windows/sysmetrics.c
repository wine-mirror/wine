/*
 * System metrics functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#include <stdio.h>

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "winbase.h"
#include "winuser.h"
#include "monitor.h"
#include "options.h"
#include "sysmetrics.h"
#include "tweak.h"

static short sysMetrics[SM_CMETRICS+1];

/***********************************************************************
 *           SYSMETRICS_Init
 *
 * Initialisation of the system metrics array.
 *
 * Differences in return values between 3.1 and 95 apps under Win95 (FIXME ?):
 * SM_CXVSCROLL        x+1      x	Fixed May 24, 1999 - Ronald B. Cemer
 * SM_CYHSCROLL        x+1      x	Fixed May 24, 1999 - Ronald B. Cemer
 * SM_CXDLGFRAME       x-1      x	Already fixed
 * SM_CYDLGFRAME       x-1      x	Already fixed
 * SM_CYCAPTION        x+1      x	Fixed May 24, 1999 - Ronald B. Cemer
 * SM_CYMENU           x-1      x	Already fixed
 * SM_CYFULLSCREEN     x-1      x
 * 
 * (collides with TWEAK_WineLook sometimes,
 * so changing anything might be difficult) 
 */
void SYSMETRICS_Init(void)
{
    sysMetrics[SM_CXCURSOR] = 32;
    sysMetrics[SM_CYCURSOR] = 32;
    sysMetrics[SM_CXSCREEN] = MONITOR_GetWidth(&MONITOR_PrimaryMonitor);
    sysMetrics[SM_CYSCREEN] =  MONITOR_GetHeight(&MONITOR_PrimaryMonitor);
    if (TWEAK_WineLook > WIN31_LOOK)
	sysMetrics[SM_CXVSCROLL] =
	    PROFILE_GetWineIniInt("Tweak.Layout", "ScrollBarWidth", 16);
    else
	sysMetrics[SM_CXVSCROLL] =
	    PROFILE_GetWineIniInt("Tweak.Layout", "ScrollBarWidth", 17);
    sysMetrics[SM_CYHSCROLL] = sysMetrics[SM_CXVSCROLL];
    if (TWEAK_WineLook > WIN31_LOOK)
	sysMetrics[SM_CYCAPTION] =
	    PROFILE_GetWineIniInt("Tweak.Layout", "CaptionHeight", 19);
    else
	sysMetrics[SM_CYCAPTION] =
	    PROFILE_GetWineIniInt("Tweak.Layout", "CaptionHeight", 20);
    sysMetrics[SM_CXBORDER] = 1;
    sysMetrics[SM_CYBORDER] = sysMetrics[SM_CXBORDER];
    sysMetrics[SM_CXDLGFRAME] =
	PROFILE_GetWineIniInt("Tweak.Layout", "DialogFrameWidth",
			      (TWEAK_WineLook > WIN31_LOOK) ? 3 : 4);
    sysMetrics[SM_CYDLGFRAME] = sysMetrics[SM_CXDLGFRAME];
    sysMetrics[SM_CYVTHUMB] = sysMetrics[SM_CXVSCROLL] - 1;
    sysMetrics[SM_CXHTHUMB] = sysMetrics[SM_CYVTHUMB];
    sysMetrics[SM_CXICON] = 32;
    sysMetrics[SM_CYICON] = 32;
    if (TWEAK_WineLook > WIN31_LOOK)
	sysMetrics[SM_CYMENU] =
	    PROFILE_GetWineIniInt("Tweak.Layout", "MenuHeight", 19);
    else
	sysMetrics[SM_CYMENU] =
	    PROFILE_GetWineIniInt("Tweak.Layout", "MenuHeight", 18);
    sysMetrics[SM_CXFULLSCREEN] = sysMetrics[SM_CXSCREEN];
    sysMetrics[SM_CYFULLSCREEN] =
	sysMetrics[SM_CYSCREEN] - sysMetrics[SM_CYCAPTION];
    sysMetrics[SM_CYKANJIWINDOW] = 0;
    sysMetrics[SM_MOUSEPRESENT] = 1;
    sysMetrics[SM_CYVSCROLL] = sysMetrics[SM_CYVTHUMB];
    sysMetrics[SM_CXHSCROLL] = sysMetrics[SM_CXHTHUMB];
    sysMetrics[SM_DEBUG] = 0;

    /* FIXME: The following should look for the registry key to see if the
       buttons should be swapped. */
    sysMetrics[SM_SWAPBUTTON] = 0;

    sysMetrics[SM_RESERVED1] = 0;
    sysMetrics[SM_RESERVED2] = 0;
    sysMetrics[SM_RESERVED3] = 0;
    sysMetrics[SM_RESERVED4] = 0;

    /* FIXME: The following two are calculated, but how? */
    sysMetrics[SM_CXMIN] = (TWEAK_WineLook > WIN31_LOOK) ? 112 : 100;
    sysMetrics[SM_CYMIN] = (TWEAK_WineLook > WIN31_LOOK) ? 27 : 28;

    sysMetrics[SM_CXSIZE] = sysMetrics[SM_CYCAPTION] - 2;
    sysMetrics[SM_CYSIZE] = sysMetrics[SM_CXSIZE];
    sysMetrics[SM_CXFRAME] = GetProfileIntA("Windows", "BorderWidth", 4) + 1;
    sysMetrics[SM_CYFRAME] = sysMetrics[SM_CXFRAME];
    sysMetrics[SM_CXMINTRACK] = sysMetrics[SM_CXMIN];
    sysMetrics[SM_CYMINTRACK] = sysMetrics[SM_CYMIN];
    sysMetrics[SM_CXDOUBLECLK] =
	(GetProfileIntA("Windows", "DoubleClickWidth", 4) + 1) & ~1;
    sysMetrics[SM_CYDOUBLECLK] =
	(GetProfileIntA("Windows","DoubleClickHeight", 4) + 1) & ~1;
    sysMetrics[SM_CXICONSPACING] =
	GetProfileIntA("Desktop","IconSpacing", 75);
    sysMetrics[SM_CYICONSPACING] =
	GetProfileIntA("Desktop", "IconVerticalSpacing", 75);
    sysMetrics[SM_MENUDROPALIGNMENT] =
	GetProfileIntA("Windows", "MenuDropAlignment", 0);
    sysMetrics[SM_PENWINDOWS] = 0;
    sysMetrics[SM_DBCSENABLED] = 0;

    /* FIXME: Need to query X for the following */
    sysMetrics[SM_CMOUSEBUTTONS] = 3;

    sysMetrics[SM_SECURE] = 0;
    sysMetrics[SM_CXEDGE] = sysMetrics[SM_CXBORDER] + 1;
    sysMetrics[SM_CYEDGE] = sysMetrics[SM_CXEDGE];
    sysMetrics[SM_CXMINSPACING] = 160;
    sysMetrics[SM_CYMINSPACING] = 24;
    sysMetrics[SM_CXSMICON] = sysMetrics[SM_CYSIZE] - (sysMetrics[SM_CYSIZE] % 2);
    sysMetrics[SM_CYSMICON] = sysMetrics[SM_CXSMICON];
    sysMetrics[SM_CYSMCAPTION] = 16;
    sysMetrics[SM_CXSMSIZE] = 15;
    sysMetrics[SM_CYSMSIZE] = sysMetrics[SM_CXSMSIZE];
    sysMetrics[SM_CXMENUSIZE] = sysMetrics[SM_CYMENU];
    sysMetrics[SM_CYMENUSIZE] = sysMetrics[SM_CXMENUSIZE];

    /* FIXME: What do these mean? */
    sysMetrics[SM_ARRANGE] = 8;
    sysMetrics[SM_CXMINIMIZED] = 160;
    sysMetrics[SM_CYMINIMIZED] = 24;

    /* FIXME: How do I calculate these? */
    sysMetrics[SM_CXMAXTRACK] = 
	sysMetrics[SM_CXSCREEN] + 4 + 2 * sysMetrics[SM_CXFRAME];
    sysMetrics[SM_CYMAXTRACK] =
	sysMetrics[SM_CYSCREEN] + 4 + 2 * sysMetrics[SM_CYFRAME];
    sysMetrics[SM_CXMAXIMIZED] =
	sysMetrics[SM_CXSCREEN] + 2 * sysMetrics[SM_CXFRAME];
    sysMetrics[SM_CYMAXIMIZED] =
	sysMetrics[SM_CYSCREEN] - 45;
    sysMetrics[SM_NETWORK] = 3;

    /* For the following: 0 = ok, 1 = failsafe, 2 = failsafe + network */
    sysMetrics[SM_CLEANBOOT] = 0;

    sysMetrics[SM_CXDRAG] = 2;
    sysMetrics[SM_CYDRAG] = 2;
    sysMetrics[SM_SHOWSOUNDS] = 0;
    sysMetrics[SM_CXMENUCHECK] = 2;
    sysMetrics[SM_CYMENUCHECK] = 2;

    /* FIXME: Should check the type of processor for the following */
    sysMetrics[SM_SLOWMACHINE] = 0;

    /* FIXME: Should perform a check */
    sysMetrics[SM_MIDEASTENABLED] = 0;

    sysMetrics[SM_MOUSEWHEELPRESENT] = 0;
    
    sysMetrics[SM_CXVIRTUALSCREEN] = sysMetrics[SM_CXSCREEN];
    sysMetrics[SM_CYVIRTUALSCREEN] = sysMetrics[SM_CYSCREEN];
    sysMetrics[SM_XVIRTUALSCREEN] = 0;
    sysMetrics[SM_YVIRTUALSCREEN] = 0;
    sysMetrics[SM_CMONITORS] = 1;
    sysMetrics[SM_SAMEDISPLAYFORMAT] = 1;
    sysMetrics[SM_CMETRICS] = SM_CMETRICS;
}


/***********************************************************************
 *           GetSystemMetrics16    (USER.179)
 */
INT16 WINAPI GetSystemMetrics16( INT16 index )
{
    return (INT16)GetSystemMetrics(index);
}


/***********************************************************************
 *           GetSystemMetrics    (USER32.292)
 */
INT WINAPI GetSystemMetrics( INT index )
{
    if ((index < 0) || (index > SM_CMETRICS)) return 0;
    else return sysMetrics[index];    
}
