/*
 * System metrics functions
 *
 * Copyright 1994 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1994";
*/

#include <X11/Xlib.h>
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
    sysMetrics[SM_CMETRICS] = SM_CMETRICS;
}


/***********************************************************************
 *           GetSystemMetrics    (USER.179)
 */
int GetSystemMetrics( WORD index )
{
    if (index > SM_CMETRICS) return 0;
    else return sysMetrics[index];    
}
