/*
 * GDI palette objects
 *
 * Copyright 1993,1994 Alexandre Julliard
 * Copyright 1996 Alex Korobka
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * NOTES:
 * PALETTEOBJ is documented in the Dr. Dobbs Journal May 1993.
 * Information in the "Undocumented Windows" is incorrect.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "winuser.h"

#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(palette);

typedef BOOL (CDECL *unrealize_function)(HPALETTE);

typedef struct tagPALETTEOBJ
{
    unrealize_function  unrealize;
    WORD                version;    /* palette version */
    WORD                count;      /* count of palette entries */
    PALETTEENTRY       *entries;
} PALETTEOBJ;

static INT PALETTE_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL PALETTE_UnrealizeObject( HGDIOBJ handle );
static BOOL PALETTE_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs palette_funcs =
{
    NULL,                     /* pSelectObject */
    PALETTE_GetObject,        /* pGetObjectA */
    PALETTE_GetObject,        /* pGetObjectW */
    PALETTE_UnrealizeObject,  /* pUnrealizeObject */
    PALETTE_DeleteObject      /* pDeleteObject */
};

/* Pointers to USER implementation of SelectPalette/RealizePalette */
/* they will be patched by USER on startup */
HPALETTE (WINAPI *pfnSelectPalette)(HDC hdc, HPALETTE hpal, WORD bkgnd ) = GDISelectPalette;
UINT (WINAPI *pfnRealizePalette)(HDC hdc) = GDIRealizePalette;

static UINT SystemPaletteUse = SYSPAL_STATIC;  /* currently not considered */

static HPALETTE hPrimaryPalette = 0; /* used for WM_PALETTECHANGED */
static HPALETTE hLastRealizedPalette = 0; /* UnrealizeObject() needs it */


/***********************************************************************
 *           PALETTE_Init
 *
 * Create the system palette.
 */
HPALETTE PALETTE_Init(void)
{
    const RGBQUAD *entries = get_default_color_table( 8 );
    char buffer[FIELD_OFFSET( LOGPALETTE, palPalEntry[20] )];
    LOGPALETTE *palPtr = (LOGPALETTE *)buffer;
    int i;

    /* create default palette (20 system colors) */

    palPtr->palVersion = 0x300;
    palPtr->palNumEntries = 20;
    for (i = 0; i < 20; i++)
    {
        palPtr->palPalEntry[i].peRed   = entries[i < 10 ? i : 236 + i].rgbRed;
        palPtr->palPalEntry[i].peGreen = entries[i < 10 ? i : 236 + i].rgbGreen;
        palPtr->palPalEntry[i].peBlue  = entries[i < 10 ? i : 236 + i].rgbBlue;
        palPtr->palPalEntry[i].peFlags = 0;
    }
    return CreatePalette( palPtr );
}


/***********************************************************************
 * CreatePalette [GDI32.@]
 *
 * Creates a logical color palette.
 *
 * RETURNS
 *    Success: Handle to logical palette
 *    Failure: NULL
 */
HPALETTE WINAPI CreatePalette(
    const LOGPALETTE* palette) /* [in] Pointer to logical color palette */
{
    PALETTEOBJ * palettePtr;
    HPALETTE hpalette;
    int size;

    if (!palette) return 0;
    TRACE("entries=%i\n", palette->palNumEntries);

    if (!(palettePtr = HeapAlloc( GetProcessHeap(), 0, sizeof(*palettePtr) ))) return 0;
    palettePtr->unrealize = NULL;
    palettePtr->version = palette->palVersion;
    palettePtr->count   = palette->palNumEntries;
    size = palettePtr->count * sizeof(*palettePtr->entries);
    if (!(palettePtr->entries = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        HeapFree( GetProcessHeap(), 0, palettePtr );
        return 0;
    }
    memcpy( palettePtr->entries, palette->palPalEntry, size );
    if (!(hpalette = alloc_gdi_handle( palettePtr, OBJ_PAL, &palette_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, palettePtr->entries );
        HeapFree( GetProcessHeap(), 0, palettePtr );
    }
    TRACE("   returning %p\n", hpalette);
    return hpalette;
}


/***********************************************************************
 * CreateHalftonePalette [GDI32.@]
 *
 * Creates a halftone palette.
 *
 * RETURNS
 *    Success: Handle to logical halftone palette
 *    Failure: 0
 *
 * FIXME: This simply creates the halftone palette derived from running
 *        tests on a windows NT machine. This is assuming a color depth
 *        of greater that 256 color. On a 256 color device the halftone
 *        palette will be different and this function will be incorrect
 */
HPALETTE WINAPI CreateHalftonePalette(
    HDC hdc) /* [in] Handle to device context */
{
    const RGBQUAD *entries = get_default_color_table( 8 );
    char buffer[FIELD_OFFSET( LOGPALETTE, palPalEntry[256] )];
    LOGPALETTE *pal = (LOGPALETTE *)buffer;
    int i;

    pal->palVersion = 0x300;
    pal->palNumEntries = 256;
    for (i = 0; i < 256; i++)
    {
        pal->palPalEntry[i].peRed   = entries[i].rgbRed;
        pal->palPalEntry[i].peGreen = entries[i].rgbGreen;
        pal->palPalEntry[i].peBlue  = entries[i].rgbBlue;
        pal->palPalEntry[i].peFlags = 0;
    }
    return CreatePalette( pal );
}


/***********************************************************************
 * GetPaletteEntries [GDI32.@]
 *
 * Retrieves palette entries.
 *
 * RETURNS
 *    Success: Number of entries from logical palette
 *    Failure: 0
 */
UINT WINAPI GetPaletteEntries(
    HPALETTE hpalette,    /* [in]  Handle of logical palette */
    UINT start,           /* [in]  First entry to receive */
    UINT count,           /* [in]  Number of entries to receive */
    LPPALETTEENTRY entries) /* [out] Address of array receiving entries */
{
    PALETTEOBJ * palPtr;
    UINT numEntries;

    TRACE("hpal = %p, count=%i\n", hpalette, count );

    palPtr = GDI_GetObjPtr( hpalette, OBJ_PAL );
    if (!palPtr) return 0;

    /* NOTE: not documented but test show this to be the case */
    if (count == 0)
    {
        count = palPtr->count;
    }
    else
    {
        numEntries = palPtr->count;
        if (start+count > numEntries) count = numEntries - start;
        if (entries)
        {
            if (start >= numEntries) count = 0;
            else memcpy( entries, &palPtr->entries[start], count * sizeof(PALETTEENTRY) );
        }
    }

    GDI_ReleaseObj( hpalette );
    return count;
}


/***********************************************************************
 * SetPaletteEntries [GDI32.@]
 *
 * Sets color values for range in palette.
 *
 * RETURNS
 *    Success: Number of entries that were set
 *    Failure: 0
 */
UINT WINAPI SetPaletteEntries(
    HPALETTE hpalette,    /* [in] Handle of logical palette */
    UINT start,           /* [in] Index of first entry to set */
    UINT count,           /* [in] Number of entries to set */
    const PALETTEENTRY *entries) /* [in] Address of array of structures */
{
    PALETTEOBJ * palPtr;
    UINT numEntries;

    TRACE("hpal=%p,start=%i,count=%i\n",hpalette,start,count );

    hpalette = get_full_gdi_handle( hpalette );
    if (hpalette == GetStockObject(DEFAULT_PALETTE)) return 0;
    palPtr = GDI_GetObjPtr( hpalette, OBJ_PAL );
    if (!palPtr) return 0;

    numEntries = palPtr->count;
    if (start >= numEntries)
    {
      GDI_ReleaseObj( hpalette );
      return 0;
    }
    if (start+count > numEntries) count = numEntries - start;
    memcpy( &palPtr->entries[start], entries, count * sizeof(PALETTEENTRY) );
    GDI_ReleaseObj( hpalette );
    UnrealizeObject( hpalette );
    return count;
}


/***********************************************************************
 * ResizePalette [GDI32.@]
 *
 * Resizes logical palette.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI ResizePalette(
    HPALETTE hPal, /* [in] Handle of logical palette */
    UINT cEntries) /* [in] Number of entries in logical palette */
{
    PALETTEOBJ * palPtr = GDI_GetObjPtr( hPal, OBJ_PAL );
    PALETTEENTRY *entries;

    if( !palPtr ) return FALSE;
    TRACE("hpal = %p, prev = %i, new = %i\n", hPal, palPtr->count, cEntries );

    if (!(entries = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                 palPtr->entries, cEntries * sizeof(*palPtr->entries) )))
    {
        GDI_ReleaseObj( hPal );
        return FALSE;
    }
    palPtr->entries = entries;
    palPtr->count = cEntries;

    GDI_ReleaseObj( hPal );
    PALETTE_UnrealizeObject( hPal );
    return TRUE;
}


/***********************************************************************
 * AnimatePalette [GDI32.@]
 *
 * Replaces entries in logical palette.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * FIXME
 *    Should use existing mapping when animating a primary palette
 */
BOOL WINAPI AnimatePalette(
    HPALETTE hPal,              /* [in] Handle to logical palette */
    UINT StartIndex,            /* [in] First entry in palette */
    UINT NumEntries,            /* [in] Count of entries in palette */
    const PALETTEENTRY* PaletteColors) /* [in] Pointer to first replacement */
{
    TRACE("%p (%i - %i)\n", hPal, StartIndex,StartIndex+NumEntries);

    hPal = get_full_gdi_handle( hPal );
    if( hPal != GetStockObject(DEFAULT_PALETTE) )
    {
        PALETTEOBJ * palPtr;
        UINT pal_entries;
        const PALETTEENTRY *pptr = PaletteColors;

        palPtr = GDI_GetObjPtr( hPal, OBJ_PAL );
        if (!palPtr) return FALSE;

        pal_entries = palPtr->count;
        if (StartIndex >= pal_entries)
        {
          GDI_ReleaseObj( hPal );
          return FALSE;
        }
        if (StartIndex+NumEntries > pal_entries) NumEntries = pal_entries - StartIndex;
        
        for (NumEntries += StartIndex; StartIndex < NumEntries; StartIndex++, pptr++) {
          /* According to MSDN, only animate PC_RESERVED colours */
          if (palPtr->entries[StartIndex].peFlags & PC_RESERVED) {
            TRACE("Animating colour (%d,%d,%d) to (%d,%d,%d)\n",
              palPtr->entries[StartIndex].peRed,
              palPtr->entries[StartIndex].peGreen,
              palPtr->entries[StartIndex].peBlue,
              pptr->peRed, pptr->peGreen, pptr->peBlue);
            palPtr->entries[StartIndex] = *pptr;
          } else {
            TRACE("Not animating entry %d -- not PC_RESERVED\n", StartIndex);
          }
        }
        GDI_ReleaseObj( hPal );
        /* FIXME: check for palette selected in active window */
    }
    return TRUE;
}


/***********************************************************************
 * SetSystemPaletteUse [GDI32.@]
 *
 * Specify whether the system palette contains 2 or 20 static colors.
 *
 * RETURNS
 *    Success: Previous system palette
 *    Failure: SYSPAL_ERROR
 */
UINT WINAPI SetSystemPaletteUse(
    HDC hdc,  /* [in] Handle of device context */
    UINT use) /* [in] Palette-usage flag */
{
    UINT old = SystemPaletteUse;

    /* Device doesn't support colour palettes */
    if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)) {
        return SYSPAL_ERROR;
    }

    switch (use) {
        case SYSPAL_NOSTATIC:
        case SYSPAL_NOSTATIC256:        /* WINVER >= 0x0500 */
        case SYSPAL_STATIC:
            SystemPaletteUse = use;
            return old;
        default:
            return SYSPAL_ERROR;
    }
}


/***********************************************************************
 * GetSystemPaletteUse [GDI32.@]
 *
 * Gets state of system palette.
 *
 * RETURNS
 *    Current state of system palette
 */
UINT WINAPI GetSystemPaletteUse(
    HDC hdc) /* [in] Handle of device context */
{
    return SystemPaletteUse;
}


/***********************************************************************
 * GetSystemPaletteEntries [GDI32.@]
 *
 * Gets range of palette entries.
 *
 * RETURNS
 *    Success: Number of entries retrieved from palette
 *    Failure: 0
 */
UINT WINAPI GetSystemPaletteEntries(
    HDC hdc,              /* [in]  Handle of device context */
    UINT start,           /* [in]  Index of first entry to be retrieved */
    UINT count,           /* [in]  Number of entries to be retrieved */
    LPPALETTEENTRY entries) /* [out] Array receiving system-palette entries */
{
    UINT ret = 0;
    DC *dc;

    TRACE("hdc=%p,start=%i,count=%i\n", hdc,start,count);

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pGetSystemPaletteEntries );
        ret = physdev->funcs->pGetSystemPaletteEntries( physdev, start, count, entries );
        release_dc_ptr( dc );
    }
    return ret;
}


/* null driver fallback implementation for GetSystemPaletteEntries */
UINT CDECL nulldrv_GetSystemPaletteEntries( PHYSDEV dev, UINT start, UINT count, PALETTEENTRY *entries )
{
    if (entries && start < 256)
    {
        UINT i;
        const RGBQUAD *default_entries;

        if (start + count > 256) count = 256 - start;

        default_entries = get_default_color_table( 8 );
        for (i = 0; i < count; ++i)
        {
            if (start + i < 10 || start + i >= 246)
            {
                entries[i].peRed = default_entries[start + i].rgbRed;
                entries[i].peGreen = default_entries[start + i].rgbGreen;
                entries[i].peBlue = default_entries[start + i].rgbBlue;
            }
            else
            {
                entries[i].peRed = 0;
                entries[i].peGreen = 0;
                entries[i].peBlue = 0;
            }
            entries[i].peFlags = 0;
        }
    }
    return 0;
}

/***********************************************************************
 * GetNearestPaletteIndex [GDI32.@]
 *
 * Gets palette index for color.
 *
 * NOTES
 *    Should index be initialized to CLR_INVALID instead of 0?
 *
 * RETURNS
 *    Success: Index of entry in logical palette
 *    Failure: CLR_INVALID
 */
UINT WINAPI GetNearestPaletteIndex(
    HPALETTE hpalette, /* [in] Handle of logical color palette */
    COLORREF color)      /* [in] Color to be matched */
{
    PALETTEOBJ* palObj = GDI_GetObjPtr( hpalette, OBJ_PAL );
    UINT index  = 0;

    if( palObj )
    {
        int i, diff = 0x7fffffff;
        int r,g,b;
        PALETTEENTRY* entry = palObj->entries;

        for( i = 0; i < palObj->count && diff ; i++, entry++)
        {
            r = entry->peRed - GetRValue(color);
            g = entry->peGreen - GetGValue(color);
            b = entry->peBlue - GetBValue(color);

            r = r*r + g*g + b*b;

            if( r < diff ) { index = i; diff = r; }
        }
        GDI_ReleaseObj( hpalette );
    }
    TRACE("(%p,%06x): returning %d\n", hpalette, color, index );
    return index;
}


/* null driver fallback implementation for GetNearestColor */
COLORREF CDECL nulldrv_GetNearestColor( PHYSDEV dev, COLORREF color )
{
    unsigned char spec_type;
    DC *dc = get_nulldrv_dc( dev );

    if (!(GetDeviceCaps( dev->hdc, RASTERCAPS ) & RC_PALETTE)) return color;

    spec_type = color >> 24;
    if (spec_type == 1 || spec_type == 2)
    {
        /* we need logical palette for PALETTERGB and PALETTEINDEX colorrefs */
        UINT index;
        PALETTEENTRY entry;
        HPALETTE hpal = dc->hPalette;

        if (!hpal) hpal = GetStockObject( DEFAULT_PALETTE );
        if (spec_type == 2) /* PALETTERGB */
            index = GetNearestPaletteIndex( hpal, color );
        else  /* PALETTEINDEX */
            index = LOWORD(color);

        if (!GetPaletteEntries( hpal, index, 1, &entry ))
        {
            WARN("RGB(%x) : idx %d is out of bounds, assuming NULL\n", color, index );
            if (!GetPaletteEntries( hpal, 0, 1, &entry )) return CLR_INVALID;
        }
        color = RGB( entry.peRed, entry.peGreen, entry.peBlue );
    }
    return color & 0x00ffffff;
}


/***********************************************************************
 * GetNearestColor [GDI32.@]
 *
 * Gets a system color to match.
 *
 * RETURNS
 *    Success: Color from system palette that corresponds to given color
 *    Failure: CLR_INVALID
 */
COLORREF WINAPI GetNearestColor(
    HDC hdc,      /* [in] Handle of device context */
    COLORREF color) /* [in] Color to be matched */
{
    COLORREF nearest = CLR_INVALID;
    DC *dc;

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pGetNearestColor );
        nearest = physdev->funcs->pGetNearestColor( physdev, color );
        release_dc_ptr( dc );
    }
    return nearest;
}


/***********************************************************************
 *           PALETTE_GetObject
 */
static INT PALETTE_GetObject( HGDIOBJ handle, INT count, LPVOID buffer )
{
    PALETTEOBJ *palette = GDI_GetObjPtr( handle, OBJ_PAL );

    if (!palette) return 0;

    if (buffer)
    {
        if (count > sizeof(WORD)) count = sizeof(WORD);
        memcpy( buffer, &palette->count, count );
    }
    else count = sizeof(WORD);
    GDI_ReleaseObj( handle );
    return count;
}


/***********************************************************************
 *           PALETTE_UnrealizeObject
 */
static BOOL PALETTE_UnrealizeObject( HGDIOBJ handle )
{
    PALETTEOBJ *palette = GDI_GetObjPtr( handle, OBJ_PAL );

    if (palette)
    {
        unrealize_function unrealize = palette->unrealize;
        palette->unrealize = NULL;
        GDI_ReleaseObj( handle );
        if (unrealize) unrealize( handle );
    }

    if (InterlockedCompareExchangePointer( (void **)&hLastRealizedPalette, 0, handle ) == handle)
        TRACE("unrealizing palette %p\n", handle);

    return TRUE;
}


/***********************************************************************
 *           PALETTE_DeleteObject
 */
static BOOL PALETTE_DeleteObject( HGDIOBJ handle )
{
    PALETTEOBJ *obj;

    PALETTE_UnrealizeObject( handle );
    if (!(obj = free_gdi_handle( handle ))) return FALSE;
    HeapFree( GetProcessHeap(), 0, obj->entries );
    HeapFree( GetProcessHeap(), 0, obj );
    return TRUE;
}


/***********************************************************************
 *           GDISelectPalette    (Not a Windows API)
 */
HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg)
{
    HPALETTE ret = 0;
    DC *dc;

    TRACE("%p %p\n", hdc, hpal );

    hpal = get_full_gdi_handle( hpal );
    if (GetObjectType(hpal) != OBJ_PAL)
    {
      WARN("invalid selected palette %p\n",hpal);
      return 0;
    }
    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pSelectPalette );
        ret = dc->hPalette;
        if (physdev->funcs->pSelectPalette( physdev, hpal, FALSE ))
        {
            dc->hPalette = hpal;
            if (!wBkg) hPrimaryPalette = hpal;
        }
        else ret = 0;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           GDIRealizePalette    (Not a Windows API)
 */
UINT WINAPI GDIRealizePalette( HDC hdc )
{
    UINT realized = 0;
    DC* dc = get_dc_ptr( hdc );

    if (!dc) return 0;

    TRACE("%p...\n", hdc );

    if( dc->hPalette == GetStockObject( DEFAULT_PALETTE ))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pRealizeDefaultPalette );
        realized = physdev->funcs->pRealizeDefaultPalette( physdev );
    }
    else if (InterlockedExchangePointer( (void **)&hLastRealizedPalette, dc->hPalette ) != dc->hPalette)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pRealizePalette );
        PALETTEOBJ *palPtr = GDI_GetObjPtr( dc->hPalette, OBJ_PAL );
        if (palPtr)
        {
            realized = physdev->funcs->pRealizePalette( physdev, dc->hPalette,
                                                        (dc->hPalette == hPrimaryPalette) );
            palPtr->unrealize = physdev->funcs->pUnrealizePalette;
            GDI_ReleaseObj( dc->hPalette );
        }
    }
    else TRACE("  skipping (hLastRealizedPalette = %p)\n", hLastRealizedPalette);

    release_dc_ptr( dc );
    TRACE("   realized %i colors.\n", realized );
    return realized;
}


/***********************************************************************
 * SelectPalette [GDI32.@]
 *
 * Selects logical palette into DC.
 *
 * RETURNS
 *    Success: Previous logical palette
 *    Failure: NULL
 */
HPALETTE WINAPI SelectPalette(
    HDC hDC,               /* [in] Handle of device context */
    HPALETTE hPal,         /* [in] Handle of logical color palette */
    BOOL bForceBackground) /* [in] Foreground/background mode */
{
    return pfnSelectPalette( hDC, hPal, bForceBackground );
}


/***********************************************************************
 * RealizePalette [GDI32.@]
 *
 * Maps palette entries to system palette.
 *
 * RETURNS
 *    Success: Number of entries in logical palette
 *    Failure: GDI_ERROR
 */
UINT WINAPI RealizePalette(
    HDC hDC) /* [in] Handle of device context */
{
    return pfnRealizePalette( hDC );
}


typedef HWND (WINAPI *WindowFromDC_funcptr)( HDC );
typedef BOOL (WINAPI *RedrawWindow_funcptr)( HWND, const RECT *, HRGN, UINT );

/**********************************************************************
 * UpdateColors [GDI32.@]
 *
 * Remaps current colors to logical palette.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI UpdateColors(
    HDC hDC) /* [in] Handle of device context */
{
    HMODULE mod;
    int size = GetDeviceCaps( hDC, SIZEPALETTE );

    if (!size) return FALSE;

    mod = GetModuleHandleA("user32.dll");
    if (mod)
    {
        WindowFromDC_funcptr pWindowFromDC = (WindowFromDC_funcptr)GetProcAddress(mod,"WindowFromDC");
        if (pWindowFromDC)
        {
            HWND hWnd = pWindowFromDC( hDC );

            /* Docs say that we have to remap current drawable pixel by pixel
             * but it would take forever given the speed of XGet/PutPixel.
             */
            if (hWnd && size)
            {
                RedrawWindow_funcptr pRedrawWindow = (void *)GetProcAddress( mod, "RedrawWindow" );
                if (pRedrawWindow) pRedrawWindow( hWnd, NULL, 0, RDW_INVALIDATE );
            }
        }
    }
    return TRUE;
}

/*********************************************************************
 *           SetMagicColors   (GDI32.@)
 */
BOOL WINAPI SetMagicColors(HDC hdc, ULONG u1, ULONG u2)
{
    FIXME("(%p 0x%08x 0x%08x): stub\n", hdc, u1, u2);
    return TRUE;
}
