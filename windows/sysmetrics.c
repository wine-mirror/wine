/*
 * System metrics functions
 *
 * Copyright 1994 Alexandre Julliard
 *
 */

#include <stdio.h>
#include "gdi.h"
#include "sysmetrics.h"

short sysMetrics[SM_CMETRICS+1];

/***********************************************************************
 *           SYSMETRICS_Init
 *
 * Initialisation of the system metrics array.
 */
void SYSMETRICS_Init(void)
{
    sysMetrics[SM_CXSCREEN] = screenWidth;
    sysMetrics[SM_CYSCREEN] = screenHeight;
    sysMetrics[SM_CXVSCROLL] = SYSMETRICS_CXVSCROLL;
    sysMetrics[SM_CYHSCROLL] = SYSMETRICS_CYHSCROLL;
    sysMetrics[SM_CYCAPTION] = SYSMETRICS_CYCAPTION;
    sysMetrics[SM_CXBORDER] = SYSMETRICS_CXBORDER;
    sysMetrics[SM_CYBORDER] = SYSMETRICS_CYBORDER;
    sysMetrics[SM_CXDLGFRAME] = SYSMETRICS_CXDLGFRAME;
    sysMetrics[SM_CYDLGFRAME] = SYSMETRICS_CYDLGFRAME;
    sysMetrics[SM_CYVTHUMB] = SYSMETRICS_CYVTHUMB;
    sysMetrics[SM_CXHTHUMB] = SYSMETRICS_CXHTHUMB;
    sysMetrics[SM_CXICON] = SYSMETRICS_CXICON;
    sysMetrics[SM_CYICON] = SYSMETRICS_CYICON;
    sysMetrics[SM_CXCURSOR] = SYSMETRICS_CXCURSOR;
    sysMetrics[SM_CYCURSOR] = SYSMETRICS_CYCURSOR;
    sysMetrics[SM_CYMENU] = SYSMETRICS_CYMENU;
    sysMetrics[SM_CXFULLSCREEN] = sysMetrics[SM_CXSCREEN];
    sysMetrics[SM_CYFULLSCREEN] = sysMetrics[SM_CYSCREEN] - sysMetrics[SM_CYCAPTION];
    sysMetrics[SM_CYKANJIWINDOW] = 0;
    sysMetrics[SM_MOUSEPRESENT] = 1;
    sysMetrics[SM_CYVSCROLL] = SYSMETRICS_CYVSCROLL;
    sysMetrics[SM_CXHSCROLL] = SYSMETRICS_CXHSCROLL;
    sysMetrics[SM_DEBUG] = 0;
    sysMetrics[SM_SWAPBUTTON] = 0;
    sysMetrics[SM_RESERVED1] = 0;
    sysMetrics[SM_RESERVED2] = 0;
    sysMetrics[SM_RESERVED3] = 0;
    sysMetrics[SM_RESERVED4] = 0;
    sysMetrics[SM_CXMIN] = SYSMETRICS_CXMIN;
    sysMetrics[SM_CYMIN] = SYSMETRICS_CYMIN;
    sysMetrics[SM_CXSIZE] = SYSMETRICS_CXSIZE;
    sysMetrics[SM_CYSIZE] = SYSMETRICS_CYSIZE;
    sysMetrics[SM_CXFRAME] = GetProfileInt32A( "windows", "BorderWidth", 4 );
    sysMetrics[SM_CYFRAME] = sysMetrics[SM_CXFRAME];
    sysMetrics[SM_CXMINTRACK] = SYSMETRICS_CXMINTRACK;
    sysMetrics[SM_CYMINTRACK] = SYSMETRICS_CYMINTRACK;
    sysMetrics[SM_CXDOUBLECLK] = (GetProfileInt32A( "windows","DoubleClickWidth", 4) + 1) & ~1;
    sysMetrics[SM_CYDOUBLECLK] = (GetProfileInt32A( "windows","DoubleClickHeight", 4) + 1) & ~1;
    sysMetrics[SM_CXICONSPACING] = GetProfileInt32A( "desktop","IconSpacing", 75);
    sysMetrics[SM_CYICONSPACING] = GetProfileInt32A( "desktop","IconVerticalSpacing", 72);
    sysMetrics[SM_MENUDROPALIGNMENT] = GetProfileInt32A( "windows","MenuDropAlignment", 0 );
    sysMetrics[SM_PENWINDOWS] = 0;
    sysMetrics[SM_DBCSENABLED] = 0;
    /* Win32 additions */
    sysMetrics[SM_CMOUSEBUTTONS] = 3; /* FIXME: query X on that one */
    sysMetrics[SM_SECURE] = 0;
    sysMetrics[SM_CXEDGE] = SYSMETRICS_CXBORDER;
    sysMetrics[SM_CYEDGE] = SYSMETRICS_CYBORDER;
    sysMetrics[SM_CXMINSPACING] = SYSMETRICS_CYBORDER;

/*
SM_CXEDGE               45
SM_CYEDGE               46
SM_CXMINSPACING         47
SM_CYMINSPACING         48
SM_CXSMICON             49
SM_CYSMICON             50
SM_CYSMCAPTION          51
SM_CXSMSIZE             52
SM_CYSMSIZE             53
SM_CXMENUSIZE           54
SM_CYMENUSIZE           55
SM_ARRANGE              56
SM_CXMINIMIZED          57
SM_CYMINIMIZED          58
SM_CXMAXTRACK           59
SM_CYMAXTRACK           60
SM_CXMAXIMIZED          61
SM_CYMAXIMIZED          62
 */
    sysMetrics[SM_NETWORK] = 1;
    sysMetrics[SM_CLEANBOOT] = 0; /* 0 - ok, 1 - failsafe, 2 - failsafe & net */
 /*
SM_CXDRAG               68
SM_CYDRAG               69
  */
    sysMetrics[SM_SHOWSOUNDS] = 1;
 /*
SM_CXMENUCHECK          71
SM_CYMENUCHECK          72
  */
    sysMetrics[SM_SLOWMACHINE] = 0; /* FIXME: perhaps decide on type of proc */
    sysMetrics[SM_MIDEASTENABLED] = 0; /* FIXME: 1 if enabled */
    sysMetrics[SM_CMETRICS] = SM_CMETRICS;

}


/***********************************************************************
 *           GetSystemMetrics16    (USER.179)
 */
INT16 GetSystemMetrics16( INT16 index )
{
    if ((index < 0) || (index > SM_CMETRICS)) return 0;
    else return sysMetrics[index];    
}


/***********************************************************************
 *           GetSystemMetrics32    (USER32.291)
 */
INT32 GetSystemMetrics32( INT32 index )
{
    if ((index < 0) || (index > SM_CMETRICS)) return 0;
    else return sysMetrics[index];    
}
