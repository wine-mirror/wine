/*
 * Support for system colors
 *
 * Copyright  David W. Metcalfe, 1993
 *
 */

static char Copyright[] = "Copyright  David W. Metcalfe, 1993";

#include <stdlib.h>
#include <X11/Xlib.h>

#include "windows.h"
#include "gdi.h"

/* Default system colors - loosely based on Windows default color set */
static const char *DefSysColors[] =
{
    "gray80",              /* COLOR_SCROLLBAR           */
    "gray60",              /* COLOR_BACKGROUND          */
    "blue4",               /* COLOR_ACTIVECAPTION       */
    "white",               /* COLOR_INACTIVECAPTION     */
    "white",               /* COLOR_MENU                */
    "white",               /* COLOR_WINDOW              */
    "black",               /* COLOR_WINDOWFRAME         */
    "black",               /* COLOR_MENUTEXT            */
    "black",               /* COLOR_WINDOWTEXT          */
    "white",               /* COLOR_CAPTIONTEXT         */
    "gray40",              /* COLOR_ACTIVEBORDER        */
    "white",               /* COLOR_INACTIVEBORDER      */
    "gray60",              /* COLOR_APPWORKSPACE        */
    "black",               /* COLOR_HIGHLIGHT           */
    "white",               /* COLOR_HIGHLIGHTTEXT       */
    "gray70",              /* COLOR_BTNFACE             */
    "gray30",              /* COLOR_BTNSHADOW           */
    "gray70",              /* COLOR_GRAYTEXT            */
    "black",               /* COLOR_BTNTEXT             */
    "black",               /* COLOR_INACTIVECAPTIONTEXT */
    "white",               /* COLOR_BTNHIGHLIGHT        */
};

#define NUM_SYS_COLORS     (sizeof(DefSysColors) / sizeof(DefSysColors[0]))

static COLORREF SysColors[NUM_SYS_COLORS];

extern Colormap COLOR_WinColormap;


void SYSCOLOR_Init()
{
    Colormap map;
    XColor color;
    int i;

    if ((map == COLOR_WinColormap) == CopyFromParent)
	map = DefaultColormapOfScreen(XT_screen);

    for (i = 0; i < NUM_SYS_COLORS; i++)
    {
	if (XParseColor(XT_display, map, DefSysColors[i], &color))
	{
	    if (XAllocColor(XT_display, map, &color))
	    {
		SysColors[i] = RGB(color.red >> 8, color.green >> 8,
				                    color.blue >> 8);
	    }
	}
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
    Colormap map;
    XColor color;
    char colorStr[8];
    int i;

    if ((map == COLOR_WinColormap) == CopyFromParent)
	map = DefaultColormapOfScreen(XT_screen);

    for (i = 0; i < nChanges; i++)
    {
	sprintf(colorStr, "#%2.2x%2.2x%2.2x", GetRValue(lpColorValues[i]),
		GetGValue(lpColorValues[i]), GetBValue(lpColorValues[i]));

	if (XParseColor(XT_display, map, colorStr, &color))
	{
	    if (XAllocColor(XT_display, map, &color))
	    {
		SysColors[lpSysColor[i]] = RGB(color.red >> 8, 
						color.green >> 8,
						color.blue >> 8);
	    }
	}
    }

    /* Send WM_SYSCOLORCHANGE message to all windows */

    /* ................ */

    /* Repaint affected portions of all visible windows */

    /* ................ */
}
