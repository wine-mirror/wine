/*
 * Support for system colors
 *
 * Copyright  David W. Metcalfe, 1993
 * Copyright  Alexandre Julliard, 1994
 *
 */

static char Copyright[] = "Copyright  David W. Metcalfe, 1993";
static char Copyright2[] = "Copyright  Alexandre Julliard, 1994";

#include <stdlib.h>

#include "windows.h"


static char * DefSysColors[] =
{
    "Scrollbar", "224 224 224",      /* COLOR_SCROLLBAR           */
    "Background", "192 192 192",     /* COLOR_BACKGROUND          */
    "ActiveTitle", "0 64 128",       /* COLOR_ACTIVECAPTION       */
    "InactiveTitle", "255 255 255",  /* COLOR_INACTIVECAPTION     */
    "Menu", "0 255 255",             /* COLOR_MENU                */
    "Window", "255 255 255",         /* COLOR_WINDOW              */
    "WindowFrame", "0 0 0",          /* COLOR_WINDOWFRAME         */
    "MenuText", "0 0 0",             /* COLOR_MENUTEXT            */
    "WindowText", "0 0 0",           /* COLOR_WINDOWTEXT          */
    "TitleText", "255 255 255",      /* COLOR_CAPTIONTEXT         */
    "ActiveBorder", "128 128 128",   /* COLOR_ACTIVEBORDER        */
    "InactiveBorder", "255 255 255", /* COLOR_INACTIVEBORDER      */
    "AppWorkspace", "255 255 232",   /* COLOR_APPWORKSPACE        */
    "Hilight", "166 202 240",        /* COLOR_HIGHLIGHT           */
    "HilightText", "0 0 0",          /* COLOR_HIGHLIGHTTEXT       */
    "ButtonFace", "192 192 192",     /* COLOR_BTNFACE             */
    "ButtonShadow", "128 128 128",   /* COLOR_BTNSHADOW           */
    "GrayText", "192 192 192",       /* COLOR_GRAYTEXT            */
    "ButtonText", "0 0 0",           /* COLOR_BTNTEXT             */
    "InactiveTitleText", "0 0 0",    /* COLOR_INACTIVECAPTIONTEXT */
    "ButtonHilight", "255 255 255"   /* COLOR_BTNHIGHLIGHT        */
};

#define NUM_SYS_COLORS     (COLOR_BTNHIGHLIGHT+1)

static COLORREF SysColors[NUM_SYS_COLORS];


/*************************************************************************
 *             SYSCOLOR_Init
 */
void SYSCOLOR_Init()
{
    int i, r, g, b;
    char **p;
    char buffer[100];

    for (i = 0, p = DefSysColors; i < NUM_SYS_COLORS; i++, p += 2)
    {
	GetProfileString( "colors", p[0], p[1], buffer, 100 );
	if (!sscanf( buffer, " %d %d %d", &r, &g, &b )) r = g = b = 0;
	SysColors[i] = RGB( r, g, b );
    }
}


/*************************************************************************
 *             GetSysColor           (USER.180)
 */

COLORREF GetSysColor(short nIndex)
{
#ifdef DEBUG_SYSCOLOR
    printf("System Color %d = %6x\n", nIndex, SysColors[nIndex]);
#endif
    return SysColors[nIndex];
}


/*************************************************************************
 *             SetSysColors          (USER.181)
 */

void SetSysColors(int nChanges, LPINT lpSysColor, COLORREF *lpColorValues)
{
    int i;

    for (i = 0; i < nChanges; i++)
    {
	SysColors[lpSysColor[i]] = lpColorValues[i];
    }

    /* Send WM_SYSCOLORCHANGE message to all windows */

    /* ................ */

    /* Repaint affected portions of all visible windows */

    /* ................ */
}
