/*
 * Support for system colors
 *
 * Copyright  David W. Metcalfe, 1993
 * Copyright  Alexandre Julliard, 1994
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "gdi.h"
#include "syscolor.h"
#include "stddebug.h"
#include "tweak.h"
/* #define DEBUG_SYSCOLOR */
#include "debug.h"

struct SysColorObjects sysColorObjects;

static const char * const DefSysColors[] =
{
    "Scrollbar", "224 224 224",      /* COLOR_SCROLLBAR           */
    "Background", "192 192 192",     /* COLOR_BACKGROUND          */
    "ActiveTitle", "0 64 128",       /* COLOR_ACTIVECAPTION       */
    "InactiveTitle", "255 255 255",  /* COLOR_INACTIVECAPTION     */
    "Menu", "255 255 255",             /* COLOR_MENU                */
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

static const char * const DefSysColors95[] =
{
    "Scrollbar", "224 224 224",      /* COLOR_SCROLLBAR           */
    "Background", "192 192 192",     /* COLOR_BACKGROUND          */
    "ActiveTitle", "0 64 128",       /* COLOR_ACTIVECAPTION       */
    "InactiveTitle", "255 255 255",  /* COLOR_INACTIVECAPTION     */
    "Menu", "192 192 192",           /* COLOR_MENU                */
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

#define MAKE_SOLID(color) \
       (PALETTEINDEX(GetNearestPaletteIndex32(STOCK_DEFAULT_PALETTE,(color))))

/*************************************************************************
 *             SYSCOLOR_SetColor
 */
static void SYSCOLOR_SetColor( int index, COLORREF color )
{
    SysColors[index] = color;
    switch(index)
    {
    case COLOR_SCROLLBAR:
	DeleteObject32( sysColorObjects.hbrushScrollbar );
	sysColorObjects.hbrushScrollbar = CreateSolidBrush32( color );
	break;
    case COLOR_BACKGROUND:
	break;
    case COLOR_ACTIVECAPTION:
	DeleteObject32( sysColorObjects.hbrushActiveCaption );
	sysColorObjects.hbrushActiveCaption = CreateSolidBrush32( color );
	break;
    case COLOR_INACTIVECAPTION:
	DeleteObject32( sysColorObjects.hbrushInactiveCaption );
	sysColorObjects.hbrushInactiveCaption = CreateSolidBrush32( color );
	break;
    case COLOR_MENU:
	DeleteObject32( sysColorObjects.hbrushMenu );
	sysColorObjects.hbrushMenu = CreateSolidBrush32( MAKE_SOLID(color) );
	break;
    case COLOR_WINDOW:
	DeleteObject32( sysColorObjects.hbrushWindow );
	sysColorObjects.hbrushWindow = CreateSolidBrush32( color );
	break;
    case COLOR_WINDOWFRAME:
	DeleteObject32( sysColorObjects.hpenWindowFrame );
	sysColorObjects.hpenWindowFrame = CreatePen32( PS_SOLID, 1, color );
	break;
    case COLOR_MENUTEXT:
	break;
    case COLOR_WINDOWTEXT:
	DeleteObject32( sysColorObjects.hpenWindowText );
	sysColorObjects.hpenWindowText = CreatePen32( PS_DOT, 1, color );
	break;
    case COLOR_CAPTIONTEXT:
	break;
    case COLOR_ACTIVEBORDER:
	DeleteObject32( sysColorObjects.hbrushActiveBorder );
	sysColorObjects.hbrushActiveBorder = CreateSolidBrush32( color );
	break;
    case COLOR_INACTIVEBORDER:
	DeleteObject32( sysColorObjects.hbrushInactiveBorder );
	sysColorObjects.hbrushInactiveBorder = CreateSolidBrush32( color );
	break;
    case COLOR_APPWORKSPACE:
	break;
    case COLOR_HIGHLIGHT:
	DeleteObject32( sysColorObjects.hbrushHighlight );
	sysColorObjects.hbrushHighlight = CreateSolidBrush32(MAKE_SOLID(color));
	break;
    case COLOR_HIGHLIGHTTEXT:
	break;
    case COLOR_BTNFACE:
	DeleteObject32( sysColorObjects.hbrushBtnFace );
	sysColorObjects.hbrushBtnFace = CreateSolidBrush32( color );
	break;
    case COLOR_BTNSHADOW:
	DeleteObject32( sysColorObjects.hbrushBtnShadow );
	sysColorObjects.hbrushBtnShadow = CreateSolidBrush32( color );
	break;
    case COLOR_GRAYTEXT:
    case COLOR_BTNTEXT:
    case COLOR_INACTIVECAPTIONTEXT:
	break;
    case COLOR_BTNHIGHLIGHT:
	DeleteObject32( sysColorObjects.hbrushBtnHighlight );
	sysColorObjects.hbrushBtnHighlight = CreateSolidBrush32( color );
	break;
    }
}


/*************************************************************************
 *             SYSCOLOR_Init
 */
void SYSCOLOR_Init(void)
{
    int i, r, g, b;
    const char * const *p;
    char buffer[100];

    for (i = 0, p = TWEAK_Win95Look ? DefSysColors95 : DefSysColors;
	 i < NUM_SYS_COLORS; i++, p += 2)
    {
	GetProfileString32A( "colors", p[0], p[1], buffer, 100 );
	if (!sscanf( buffer, " %d %d %d", &r, &g, &b )) r = g = b = 0;
	SYSCOLOR_SetColor( i, RGB(r,g,b) );
    }
}


/*************************************************************************
 *             GetSysColor16   (USER.180)
 */
COLORREF GetSysColor16( INT16 nIndex )
{
    return SysColors[nIndex];
}


/*************************************************************************
 *             GetSysColor32   (USER32.288)
 */
COLORREF GetSysColor32( INT32 nIndex )
{
    return SysColors[nIndex];
}


/*************************************************************************
 *             SetSysColors16   (USER.181)
 */
VOID SetSysColors16( INT16 nChanges, const INT16 *lpSysColor,
                     const COLORREF *lpColorValues )
{
    int i;

    for (i = 0; i < nChanges; i++)
    {
	SYSCOLOR_SetColor( lpSysColor[i], lpColorValues[i] );
    }

    /* Send WM_SYSCOLORCHANGE message to all windows */

    SendMessage32A( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0 );

    /* Repaint affected portions of all visible windows */

    RedrawWindow32( GetDesktopWindow32(), NULL, 0,
                RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/*************************************************************************
 *             SetSysColors32   (USER32.504)
 */
BOOL32 SetSysColors32( INT32 nChanges, const INT32 *lpSysColor,
                       const COLORREF *lpColorValues )
{
    int i;

    for (i = 0; i < nChanges; i++)
    {
	SYSCOLOR_SetColor( lpSysColor[i], lpColorValues[i] );
    }

    /* Send WM_SYSCOLORCHANGE message to all windows */

    SendMessage32A( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0 );

    /* Repaint affected portions of all visible windows */

    RedrawWindow32( GetDesktopWindow32(), NULL, 0,
                RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}
