/*
 * Support for system colors
 *
 * Copyright  David W. Metcalfe, 1993
 * Copyright  Alexandre Julliard, 1994
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(syscolor);

static const char * const DefSysColors[] =
{
    "Scrollbar", "192 192 192",      /* COLOR_SCROLLBAR           */
    "Background", "0 128 128",       /* COLOR_BACKGROUND          */
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
    "AppWorkSpace", "128 128 128",   /* COLOR_APPWORKSPACE        */
    "Hilight", "0 0 128",            /* COLOR_HIGHLIGHT           */
    "HilightText", "255 255 255",    /* COLOR_HIGHLIGHTTEXT       */
    "ButtonFace", "192 192 192",     /* COLOR_BTNFACE             */
    "ButtonShadow", "128 128 128",   /* COLOR_BTNSHADOW           */
    "GrayText", "128 128 128",       /* COLOR_GRAYTEXT            */
    "ButtonText", "0 0 0",           /* COLOR_BTNTEXT             */
    "InactiveTitleText", "192 192 192",/* COLOR_INACTIVECAPTIONTEXT */
    "ButtonHilight", "255 255 255",  /* COLOR_BTNHIGHLIGHT        */
    "ButtonDkShadow", "0 0 0",       /* COLOR_3DDKSHADOW          */
    "ButtonLight", "224 224 224",    /* COLOR_3DLIGHT             */
    "InfoText", "0 0 0",             /* COLOR_INFOTEXT            */
    "InfoWindow", "255 255 225",     /* COLOR_INFOBK              */
    "ButtonAlternateFace", "180 180 180",  /* COLOR_ALTERNATEBTNFACE */
    "HotTrackingColor", "0 0 255",         /* COLOR_HOTLIGHT */
    "GradientActiveTitle", "16 132 208",   /* COLOR_GRADIENTACTIVECAPTION */
    "GradientInactiveTitle", "181 181 181",/* COLOR_GRADIENTINACTIVECAPTION */
    "MenuHilight", "0 0 0",          /* COLOR_MENUHILIGHT         */
    "MenuBar", "192 192 192"         /* COLOR_MENUBAR             */
};


#define NUM_SYS_COLORS     (COLOR_MENUBAR+1)

static COLORREF SysColors[NUM_SYS_COLORS];
static HBRUSH SysColorBrushes[NUM_SYS_COLORS];
static HPEN   SysColorPens[NUM_SYS_COLORS];

static const WORD wPattern55AA[] =
    { 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa };

HBRUSH SYSCOLOR_55AABrush = 0;

extern void __wine_make_gdi_object_system( HGDIOBJ handle, BOOL set );

/*************************************************************************
 *             SYSCOLOR_SetColor
 */
static void SYSCOLOR_SetColor( int index, COLORREF color )
{
    if (index < 0 || index >= NUM_SYS_COLORS) return;
    SysColors[index] = color;
    if (SysColorBrushes[index])
    {
	__wine_make_gdi_object_system( SysColorBrushes[index], FALSE);
	DeleteObject( SysColorBrushes[index] );
    }
    SysColorBrushes[index] = CreateSolidBrush( color );
    __wine_make_gdi_object_system( SysColorBrushes[index], TRUE);

    if (SysColorPens[index])
    {
        __wine_make_gdi_object_system( SysColorPens[index], FALSE);
	DeleteObject( SysColorPens[index] );
    }
    SysColorPens[index] = CreatePen( PS_SOLID, 1, color );
    __wine_make_gdi_object_system( SysColorPens[index], TRUE);
}


/*************************************************************************
 *             SYSCOLOR_Init
 */
void SYSCOLOR_Init(void)
{
    int i, r, g, b;
    char buffer[100];
    BOOL bOk = FALSE, bNoReg = FALSE;
    HKEY  hKey;
    HBITMAP h55AABitmap;

    /* first, try to read the values from the registry */
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Control Panel\\Colors", 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0))
      bNoReg = TRUE;
    for (i = 0; i < NUM_SYS_COLORS; i++)
    { bOk = FALSE;

      /* first try, registry */
      if (!bNoReg)
      {
	DWORD dwDataSize = sizeof(buffer);
	if (!(RegQueryValueExA(hKey,DefSysColors[i*2], 0, 0, buffer, &dwDataSize)))
	  if (sscanf( buffer, "%d %d %d", &r, &g, &b ) == 3)
	    bOk = TRUE;
      }

      /* second try, win.ini */
      if (!bOk)
      { GetProfileStringA( "colors", DefSysColors[i*2], DefSysColors[i*2+1], buffer, 100 );
	if (sscanf( buffer, " %d %d %d", &r, &g, &b ) == 3)
	  bOk = TRUE;
      }

      /* last chance, take the default */
      if (!bOk)
      { int iNumColors = sscanf( DefSysColors[i*2+1], " %d %d %d", &r, &g, &b );
	assert (iNumColors==3);
      }

      SYSCOLOR_SetColor( i, RGB(r,g,b) );
    }
    if (!bNoReg)
      RegCloseKey(hKey);

    h55AABitmap = CreateBitmap( 8, 8, 1, 1, wPattern55AA );
    SYSCOLOR_55AABrush = CreatePatternBrush( h55AABitmap );
    __wine_make_gdi_object_system( SYSCOLOR_55AABrush, TRUE );
}


/*************************************************************************
 *		GetSysColor (USER32.@)
 */
COLORREF WINAPI GetSysColor( INT nIndex )
{
    if (nIndex >= 0 && nIndex < NUM_SYS_COLORS)
	return SysColors[nIndex];
    else
	return 0;
}


/*************************************************************************
 *		SetSysColors (USER32.@)
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

    SendMessageTimeoutW( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0,
                         SMTO_ABORTIFHUNG, 2000, NULL );

    /* Repaint affected portions of all visible windows */

    RedrawWindow( GetDesktopWindow(), NULL, 0,
                RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
    return TRUE;
}

/*************************************************************************
 *		SetSysColorsTemp (USER32.@)
 *
 * UNDOCUMENTED !!
 *
 * Called by W98SE desk.cpl Control Panel Applet:
 * handle = SetSysColorsTemp(ptr, ptr, nCount);     ("set" call)
 * result = SetSysColorsTemp(NULL, NULL, handle);   ("restore" call)
 *
 * pPens is an array of COLORREF values, which seems to be used
 * to indicate the color values to create new pens with.
 *
 * pBrushes is an array of solid brush handles (returned by a previous
 * CreateSolidBrush), which seems to contain the brush handles to set
 * for the system colors.
 *
 * n seems to be used for
 *   a) indicating the number of entries to operate on (length of pPens,
 *      pBrushes)
 *   b) passing the handle that points to the previously used color settings.
 *      I couldn't figure out in hell what kind of handle this is on
 *      Windows. I just use a heap handle instead. Shouldn't matter anyway.
 *
 * RETURNS
 *     heap handle of our own copy of the current syscolors in case of
 *                 "set" call, i.e. pPens, pBrushes != NULL.
 *     TRUE (unconditionally !) in case of "restore" call,
 *          i.e. pPens, pBrushes == NULL.
 *     FALSE in case of either pPens != NULL and pBrushes == NULL
 *          or pPens == NULL and pBrushes != NULL.
 *
 * I'm not sure whether this implementation is 100% correct. [AM]
 */
DWORD WINAPI SetSysColorsTemp( const COLORREF *pPens, const HBRUSH *pBrushes, DWORD n)
{
	int i;

	if (pPens && pBrushes) /* "set" call */
	{
	    /* allocate our structure to remember old colors */
	    LPVOID pOldCol = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD)+n*sizeof(HPEN)+n*sizeof(HBRUSH));
	    LPVOID p = pOldCol;
           *(DWORD *)p = n; p = (char*)p + sizeof(DWORD);
           memcpy(p, SysColorPens, n*sizeof(HPEN)); p = (char*)p + n*sizeof(HPEN);
           memcpy(p, SysColorBrushes, n*sizeof(HBRUSH)); p = (char*)p + n*sizeof(HBRUSH);

	    for (i=0; i < n; i++)
	    {
		SysColorPens[i] = CreatePen( PS_SOLID, 1, pPens[i] );
		SysColorBrushes[i] = pBrushes[i];
	    }

	    return (DWORD)pOldCol;
	}
	if ((!pPens) && (!pBrushes)) /* "restore" call */
	{
	    LPVOID pOldCol = (LPVOID)n;
	    LPVOID p = pOldCol;
	    DWORD nCount = *(DWORD *)p;
           p = (char*)p + sizeof(DWORD);

	    for (i=0; i < nCount; i++)
	    {
		DeleteObject(SysColorPens[i]);
               SysColorPens[i] = *(HPEN *)p; p = (char*)p + sizeof(HPEN);
	    }
	    for (i=0; i < nCount; i++)
	    {
               SysColorBrushes[i] = *(HBRUSH *)p; p = (char*)p + sizeof(HBRUSH);
	    }
	    /* get rid of storage structure */
	    HeapFree(GetProcessHeap(), 0, pOldCol);

	    return TRUE;
	}
	return FALSE;
}

/***********************************************************************
 *		GetSysColorBrush (USER32.@)
 */
HBRUSH WINAPI GetSysColorBrush( INT index )
{
    if (0 <= index && index < NUM_SYS_COLORS)
        return SysColorBrushes[index];
    WARN("Unknown index(%d)\n", index );
    return GetStockObject(LTGRAY_BRUSH);
}


/***********************************************************************
 *		SYSCOLOR_GetPen
 */
HPEN SYSCOLOR_GetPen( INT index )
{
    /* We can assert here, because this function is internal to Wine */
    assert (0 <= index && index < NUM_SYS_COLORS);
    return SysColorPens[index];
}
