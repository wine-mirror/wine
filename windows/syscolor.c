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
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "winuser.h"
#include "wownt32.h"
#include "winreg.h"
#include "local.h"
#include "user.h"
#include "gdi.h" /* sic */
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(syscolor);

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
    "GradientInactiveTitle", "181 181 181" /* COLOR_GRADIENTINACTIVECAPTION */
};

static const char * const DefSysColors95[] =
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
    "AppWorkspace", "128 128 128",   /* COLOR_APPWORKSPACE        */
    "Hilight", "0 0 128",            /* COLOR_HIGHLIGHT           */
    "HilightText", "255 255 255",    /* COLOR_HIGHLIGHTTEXT       */
    "ButtonFace", "192 192 192",     /* COLOR_BTNFACE             */
    "ButtonShadow", "128 128 128",   /* COLOR_BTNSHADOW           */
    "GrayText", "128 128 128",       /* COLOR_GRAYTEXT            */
    "ButtonText", "0 0 0",           /* COLOR_BTNTEXT             */
    "InactiveTitleText", "192 192 192",/* COLOR_INACTIVECAPTIONTEXT */
    "ButtonHilight", "255 255 255",  /* COLOR_BTNHIGHLIGHT        */
    "3DDarkShadow", "0 0 0",         /* COLOR_3DDKSHADOW          */
    "3DLight", "224 224 224",        /* COLOR_3DLIGHT             */
    "InfoText", "0 0 0",             /* COLOR_INFOTEXT            */
    "InfoBackground", "255 255 225", /* COLOR_INFOBK              */
    "AlternateButtonFace", "180 180 180",  /* COLOR_ALTERNATEBTNFACE */
    "HotTrackingColor", "0 0 255",         /* COLOR_HOTLIGHT */
    "GradientActiveTitle", "16 132 208",   /* COLOR_GRADIENTACTIVECAPTION */
    "GradientInactiveTitle", "181 181 181" /* COLOR_GRADIENTINACTIVECAPTION */
};


#define NUM_SYS_COLORS     (COLOR_GRADIENTINACTIVECAPTION+1)

static COLORREF SysColors[NUM_SYS_COLORS];
static HBRUSH SysColorBrushes[NUM_SYS_COLORS];
static HPEN   SysColorPens[NUM_SYS_COLORS];


/*************************************************************************
 * SYSCOLOR_MakeObjectSystem
 *
 * OK, now for a very ugly hack.
 * USER somehow has to tell GDI that its system brushes and pens are
 * non-deletable.
 * We don't want to export a function from GDI doing this for us,
 * so we just do that ourselves by "wildly flipping some bits in memory".
 * For a description of the GDI object magics and their flags,
 * see "Undocumented Windows" (wrong about the OBJECT_NOSYSTEM flag, though).
 */
static void SYSCOLOR_MakeObjectSystem( HGDIOBJ16 handle, BOOL set)
{
    static WORD heap_sel = 0;
    LPWORD ptr;

    if (!heap_sel) heap_sel = LoadLibrary16( "gdi" );
    if (heap_sel >= 32)
    {
        ptr = (LPWORD)LOCAL_Lock(heap_sel, handle);

        /* touch the "system" bit of the wMagic field of a GDIOBJHDR */
        if (set)
            *(ptr+1) &= ~OBJECT_NOSYSTEM;
        else
            *(ptr+1) |= OBJECT_NOSYSTEM;
        LOCAL_Unlock( heap_sel, handle );
    }
}

/*************************************************************************
 *             SYSCOLOR_SetColor
 */
static void SYSCOLOR_SetColor( int index, COLORREF color )
{
    if (index < 0 || index >= NUM_SYS_COLORS) return;
    SysColors[index] = color;
    if (SysColorBrushes[index])
    {
	SYSCOLOR_MakeObjectSystem( HBRUSH_16(SysColorBrushes[index]), FALSE);
	DeleteObject( SysColorBrushes[index] );
    }
    SysColorBrushes[index] = CreateSolidBrush( color );
    SYSCOLOR_MakeObjectSystem( HBRUSH_16(SysColorBrushes[index]), TRUE);

    if (SysColorPens[index])
    {
        SYSCOLOR_MakeObjectSystem( HPEN_16(SysColorPens[index]), FALSE);
	DeleteObject( SysColorPens[index] );
    }
    SysColorPens[index] = CreatePen( PS_SOLID, 1, color );
    SYSCOLOR_MakeObjectSystem( HPEN_16(SysColorPens[index]), TRUE);
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
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Control Panel\\Colors", 0, 0, 0, KEY_ALL_ACCESS, 0, &hKey, 0))
      bNoReg = TRUE;
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
 *		GetSysColor (USER.180)
 */
COLORREF WINAPI GetSysColor16( INT16 nIndex )
{
    return GetSysColor (nIndex);
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
 *		SetSysColors (USER.181)
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

    SendMessageTimeoutW( HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0,
                         SMTO_ABORTIFHUNG, 2000, NULL );

    /* Repaint affected portions of all visible windows */

    RedrawWindow( GetDesktopWindow(), NULL, 0,
                RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
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
