/*
 * Support for system colors
 *
 * Copyright  David W. Metcalfe, 1993
 * Copyright  Alexandre Julliard, 1994
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "sysmetrics.h"
#include "winbase.h"
#include "winuser.h"
#include "debugtools.h"
#include "tweak.h"
#include "winreg.h"
#include "winversion.h"

DEFAULT_DEBUG_CHANNEL(syscolor)

static const char * const DefSysColors[] =
{
    "Scrollbar", "224 224 224",      /* COLOR_SCROLLBAR           */
    "Background", "192 192 192",     /* COLOR_BACKGROUND          */
    "ActiveTitle", "0 64 128",       /* COLOR_ACTIVECAPTION       */
    "InactiveTitle", "255 255 255",  /* COLOR_INACTIVECAPTION     */
    "Menu", "255 255 255",           /* COLOR_MENU                */
    "Window", "255 255 255",         /* COLOR_WINDOW              */
    "WindowFrame", "0 0 0",          /* COLOR_WINDOWFRAME         */
    "MenuText", "0 0 0",             /* COLOR_MENUTEXT            */
    "WindowText", "0 0 0",           /* COLOR_WINDOWTEXT          */
    "TitleText", "255 255 255",      /* COLOR_CAPTIONTEXT         */
    "ActiveBorder", "128 128 128",   /* COLOR_ACTIVEBORDER        */
    "InactiveBorder", "255 255 255", /* COLOR_INACTIVEBORDER      */
    "AppWorkspace", "255 255 232",   /* COLOR_APPWORKSPACE        */
    "Hilight", "224 224 224",        /* COLOR_HIGHLIGHT           */
    "HilightText", "0 0 0",          /* COLOR_HIGHLIGHTTEXT       */
    "ButtonFace", "192 192 192",     /* COLOR_BTNFACE             */
    "ButtonShadow", "128 128 128",   /* COLOR_BTNSHADOW           */
    "GrayText", "192 192 192",       /* COLOR_GRAYTEXT            */
    "ButtonText", "0 0 0",           /* COLOR_BTNTEXT             */
    "InactiveTitleText", "0 0 0",    /* COLOR_INACTIVECAPTIONTEXT */
    "ButtonHilight", "255 255 255",  /* COLOR_BTNHIGHLIGHT        */
    "3DDarkShadow", "32 32 32",      /* COLOR_3DDKSHADOW          */
    "3DLight", "192 192 192",        /* COLOR_3DLIGHT             */
    "InfoText", "0 0 0",             /* COLOR_INFOTEXT            */
    "InfoBackground", "255 255 192", /* COLOR_INFOBK              */
    "AlternateButtonFace", "184 180 184",  /* COLOR_ALTERNATEBTNFACE */
    "HotTrackingColor", "0 0 255",         /* COLOR_HOTLIGHT */
    "GradientActiveTitle", "16 132 208",   /* COLOR_GRADIENTACTIVECAPTION */
    "GradientInactiveTitle", "184 180 184" /* COLOR_GRADIENTINACTIVECAPTION */
};

static const char * const DefSysColors95[] =
{
    "Scrollbar", "223 223 223",      /* COLOR_SCROLLBAR           */
    "Background", "192 192 192",     /* COLOR_BACKGROUND          */
    "ActiveTitle", "0 0 128",        /* COLOR_ACTIVECAPTION       */
    "InactiveTitle", "128 128 128",  /* COLOR_INACTIVECAPTION     */
    "Menu", "192 192 192",           /* COLOR_MENU                */
    "Window", "255 255 255",         /* COLOR_WINDOW              */
    "WindowFrame", "0 0 0",          /* COLOR_WINDOWFRAME         */
    "MenuText", "0 0 0",             /* COLOR_MENUTEXT            */
    "WindowText", "0 0 0",           /* COLOR_WINDOWTEXT          */
    "TitleText", "255 255 255",      /* COLOR_CAPTIONTEXT         */
    "ActiveBorder", "192 192 192",   /* COLOR_ACTIVEBORDER        */
    "InactiveBorder", "192 192 192", /* COLOR_INACTIVEBORDER      */
    "AppWorkspace", "128 128 128",   /* COLOR_APPWORKSPACE        */
    "Hilight", "0 0 128",            /* COLOR_HIGHLIGHT           */
    "HilightText", "255 255 255",    /* COLOR_HIGHLIGHTTEXT       */
    "ButtonFace", "192 192 192",     /* COLOR_BTNFACE             */
    "ButtonShadow", "128 128 128",   /* COLOR_BTNSHADOW           */
    "GrayText", "128 128 128",       /* COLOR_GRAYTEXT            */
    "ButtonText", "0 0 0",           /* COLOR_BTNTEXT             */
    "InactiveTitleText", "0 0 0",    /* COLOR_INACTIVECAPTIONTEXT */
    "ButtonHilight", "255 255 255",  /* COLOR_BTNHIGHLIGHT        */
    "3DDarkShadow", "0 0 0",         /* COLOR_3DDKSHADOW          */
    "3DLight", "192 192 192",        /* COLOR_3DLIGHT             */
    "InfoText", "0 0 0",             /* COLOR_INFOTEXT            */
    "InfoBackground", "255 255 192", /* COLOR_INFOBK              */
    "AlternateButtonFace", "184 180 184",  /* COLOR_ALTERNATEBTNFACE */
    "HotTrackingColor", "0 0 255",         /* COLOR_HOTLIGHT */
    "GradientActiveTitle", "16 132 208",   /* COLOR_GRADIENTACTIVECAPTION */
    "GradientInactiveTitle", "184 180 184" /* COLOR_GRADIENTINACTIVECAPTION */
};


#define NUM_SYS_COLORS     (COLOR_GRADIENTINACTIVECAPTION+1)

static COLORREF SysColors[NUM_SYS_COLORS];
static HBRUSH SysColorBrushes[NUM_SYS_COLORS];
static HPEN   SysColorPens[NUM_SYS_COLORS];

#define MAKE_SOLID(color) \
       (PALETTEINDEX(GetNearestPaletteIndex(STOCK_DEFAULT_PALETTE,(color))))

/*************************************************************************
 *             SYSCOLOR_SetColor
 */
static void SYSCOLOR_SetColor( int index, COLORREF color )
{
    if (index < 0 || index >= NUM_SYS_COLORS) return;
    SysColors[index] = color;
    if (SysColorBrushes[index]) DeleteObject( SysColorBrushes[index] );
    SysColorBrushes[index] = CreateSolidBrush( color );
    if (SysColorPens[index]) DeleteObject( SysColorPens[index] ); 
    SysColorPens[index] = CreatePen( PS_SOLID, 1, color );
}


/*************************************************************************
 *             SYSCOLOR_Init
 */
void SYSCOLOR_Init(void)
{
    int i, r, g, b;
    const char * const *p;
    char buffer[100];
    BOOL bOk = FALSE, bNoReg = FALSE;
    HKEY  hKey;

    p = (TWEAK_WineLook == WIN31_LOOK) ? DefSysColors : DefSysColors95;

    /* first, try to read the values from the registry */
    if (VERSION_GetVersion() != WIN31)
    {
      if (RegCreateKeyExA(HKEY_CURRENT_USER, "Control Panel\\Colors", 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0))
        bNoReg = TRUE;
	
    }	
      for (i = 0; i < NUM_SYS_COLORS; i++)
      { bOk = FALSE;

	/* first try, registry */
	if (!bNoReg)
        {
          DWORD dwDataSize = sizeof(buffer);
          if (!(RegQueryValueExA(hKey,(LPSTR)p[i*2], 0, 0, buffer, &dwDataSize)))
            if (sscanf( buffer, "%d %d %d", &r, &g, &b ) == 3) 
              bOk = TRUE;
        }

	/* second try, win.ini */
	if (!bOk)
	{ GetProfileStringA( "colors", p[i*2], p[i*2+1], buffer, 100 );
	  if (sscanf( buffer, " %d %d %d", &r, &g, &b ) == 3)
	    bOk = TRUE;
	}
	
	/* last chance, take the default */
	if (!bOk)
	{ int iNumColors = sscanf( p[i*2+1], " %d %d %d", &r, &g, &b );
	  assert (iNumColors==3);
	}
	
	SYSCOLOR_SetColor( i, RGB(r,g,b) );
      }
      if (!bNoReg)
        RegCloseKey(hKey);

}


/*************************************************************************
 *             GetSysColor16   (USER.180)
 */
COLORREF WINAPI GetSysColor16( INT16 nIndex )
{
    return GetSysColor (nIndex);
}


/*************************************************************************
 *             GetSysColor32   (USER32.289)
 */
COLORREF WINAPI GetSysColor( INT nIndex )
{
    if (nIndex >= 0 && nIndex < NUM_SYS_COLORS)
	return SysColors[nIndex];
    else
	return 0;
}


/*************************************************************************
 *             SetSysColors16   (USER.181)
 */
VOID WINAPI SetSysColors16( INT16 nChanges, const INT16 *lpSysColor,
			    const COLORREF *lpColorValues )
{
    int i;

    for (i = 0; i < nChanges; i++)
    {
	SYSCOLOR_SetColor( lpSysColor[i], lpColorValues[i] );
    }

    /* Send WM_SYSCOLORCHANGE message to all windows */

    SendMessageA( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0 );

    /* Repaint affected portions of all visible windows */

    RedrawWindow( GetDesktopWindow(), NULL, 0,
                RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/*************************************************************************
 *             SetSysColors32   (USER32.505)
 */
BOOL WINAPI SetSysColors( INT nChanges, const INT *lpSysColor,
                              const COLORREF *lpColorValues )
{
    int i;

    for (i = 0; i < nChanges; i++)
    {
	SYSCOLOR_SetColor( lpSysColor[i], lpColorValues[i] );
    }

    /* Send WM_SYSCOLORCHANGE message to all windows */

    SendMessageA( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0 );

    /* Repaint affected portions of all visible windows */

    RedrawWindow( GetDesktopWindow(), NULL, 0,
                RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}


/***********************************************************************
 *           GetSysColorBrush16    (USER.281)
 */
HBRUSH16 WINAPI GetSysColorBrush16( INT16 index )
{
    return (HBRUSH16)GetSysColorBrush(index);
}


/***********************************************************************
 *           GetSysColorBrush32    (USER32.290)
 */
HBRUSH WINAPI GetSysColorBrush( INT index )
{
    if (0 <= index && index < NUM_SYS_COLORS)
        return SysColorBrushes[index];
    WARN("Unknown index(%d)\n", index );
    return GetStockObject(LTGRAY_BRUSH);
}


/***********************************************************************
 *           GetSysColorPen16    (Not a Windows API)
 */
HPEN16 WINAPI GetSysColorPen16( INT16 index )
{
    return (HPEN16)GetSysColorPen(index);
}


/***********************************************************************
 *           GetSysColorPen32    (Not a Windows API)
 *
 * This function is new to the Wine lib -- it does not exist in 
 * Windows. However, it is a natural complement for GetSysColorBrush
 * in the Win32 API and is needed quite a bit inside Wine.
 */
HPEN WINAPI GetSysColorPen( INT index )
{
    /* We can assert here, because this function is internal to Wine */
    assert (0 <= index && index < NUM_SYS_COLORS);
    return SysColorPens[index];

}
