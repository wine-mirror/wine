/*
 * System metrics functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#include <stdio.h>
#include "gdi.h"
#include "options.h"
#include "tweak.h"
#include "sysmetrics.h"

short sysMetrics[SM_CMETRICS+1];

/***********************************************************************
 *           SYSMETRICS_Init
 *
 * Initialisation of the system metrics array.
 */
void SYSMETRICS_Init(void)
{
    sysMetrics[SM_CXCURSOR] = 32;
    sysMetrics[SM_CYCURSOR] = 32;
    sysMetrics[SM_CXSCREEN] = screenWidth;
    sysMetrics[SM_CYSCREEN] = screenHeight;
    sysMetrics[SM_CXVSCROLL] =
	PROFILE_GetWineIniInt("Tweak.Layout", "ScrollBarWidth", 16) + 1;
    sysMetrics[SM_CYHSCROLL] = sysMetrics[SM_CXVSCROLL];
    sysMetrics[SM_CYCAPTION] = 2 +
	PROFILE_GetWineIniInt("Tweak.Layout", "CaptionHeight", 18);
    sysMetrics[SM_CXBORDER] = 1;
    sysMetrics[SM_CYBORDER] = sysMetrics[SM_CXBORDER];
    sysMetrics[SM_CXDLGFRAME] =
	PROFILE_GetWineIniInt("Tweak.Layout", "DialogFrameWidth",
			      TWEAK_Win95Look ? 3 : 4);
    sysMetrics[SM_CYDLGFRAME] = sysMetrics[SM_CXDLGFRAME];
    sysMetrics[SM_CYVTHUMB] = sysMetrics[SM_CXVSCROLL] - 1;
    sysMetrics[SM_CXHTHUMB] = sysMetrics[SM_CYVTHUMB];
    sysMetrics[SM_CXICON] = 32;
    sysMetrics[SM_CYICON] = 32;
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
    sysMetrics[SM_CXMIN] = TWEAK_Win95Look ? 112 : 100;
    sysMetrics[SM_CYMIN] = TWEAK_Win95Look ? 27 : 28;

    sysMetrics[SM_CXSIZE] = sysMetrics[SM_CYCAPTION] - 2;
    sysMetrics[SM_CYSIZE] = sysMetrics[SM_CXSIZE];
    sysMetrics[SM_CXFRAME] = GetProfileInt32A("Windows", "BorderWidth", 4);
    sysMetrics[SM_CYFRAME] = sysMetrics[SM_CXFRAME];
    sysMetrics[SM_CXMINTRACK] = sysMetrics[SM_CXMIN];
    sysMetrics[SM_CYMINTRACK] = sysMetrics[SM_CYMIN];
    sysMetrics[SM_CXDOUBLECLK] =
	(GetProfileInt32A("Windows", "DoubleClickWidth", 4) + 1) & ~1;
    sysMetrics[SM_CYDOUBLECLK] =
	(GetProfileInt32A("Windows","DoubleClickHeight", 4) + 1) & ~1;
    sysMetrics[SM_CXICONSPACING] =
	GetProfileInt32A("Desktop","IconSpacing", 75);
    sysMetrics[SM_CYICONSPACING] =
	GetProfileInt32A("Desktop", "IconVerticalSpacing", 75);
    sysMetrics[SM_MENUDROPALIGNMENT] =
	GetProfileInt32A("Windows", "MenuDropAlignment", 0);
    sysMetrics[SM_PENWINDOWS] = 0;
    sysMetrics[SM_DBCSENABLED] = 0;

    /* FIXME: Need to query X for the following */
    sysMetrics[SM_CMOUSEBUTTONS] = 3;

    sysMetrics[SM_SECURE] = 0;
    sysMetrics[SM_CXEDGE] = sysMetrics[SM_CXBORDER] + 1;
    sysMetrics[SM_CYEDGE] = sysMetrics[SM_CXEDGE];
    sysMetrics[SM_CXMINSPACING] = 160;
    sysMetrics[SM_CYMINSPACING] = 24;
    sysMetrics[SM_CXSMICON] =
	sysMetrics[SM_CYSIZE] - (sysMetrics[SM_CYSIZE] % 2) - 2;
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
    sysMetrics[SM_CMETRICS] = SM_CMETRICS;
}


/***********************************************************************
 *           GetSystemMetrics16    (USER.179)
 */
INT16 WINAPI GetSystemMetrics16( INT16 index )
{
    if ((index < 0) || (index > SM_CMETRICS)) return 0;
    else return sysMetrics[index];    
}


/***********************************************************************
 *           GetSystemMetrics32    (USER32.292)
 */
INT32 WINAPI GetSystemMetrics32( INT32 index )
{
    if ((index < 0) || (index > SM_CMETRICS)) return 0;
    else return sysMetrics[index];    
}
