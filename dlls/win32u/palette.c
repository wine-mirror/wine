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

#if 0
#pragma makedep unix
#endif

#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(palette);

typedef BOOL (*unrealize_function)(HPALETTE);

typedef struct tagPALETTEOBJ
{
    struct gdi_obj_header obj;
    unrealize_function    unrealize;
    WORD                  version;    /* palette version */
    WORD                  count;      /* count of palette entries */
    PALETTEENTRY         *entries;
} PALETTEOBJ;

static INT PALETTE_GetObject( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL PALETTE_UnrealizeObject( HGDIOBJ handle );
static BOOL PALETTE_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs palette_funcs =
{
    PALETTE_GetObject,        /* pGetObjectW */
    PALETTE_UnrealizeObject,  /* pUnrealizeObject */
    PALETTE_DeleteObject      /* pDeleteObject */
};

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
    return NtGdiCreatePaletteInternal( palPtr, palPtr->palNumEntries );
}


/***********************************************************************
 *           NtGdiCreatePaletteInternal    (win32u.@)
 *
 * Creates a logical color palette.
 */
HPALETTE WINAPI NtGdiCreatePaletteInternal( const LOGPALETTE *palette, UINT count )
{
    PALETTEOBJ * palettePtr;
    HPALETTE hpalette;
    int size;

    if (!palette) return 0;
    TRACE( "entries=%u\n", count );

    if (!(palettePtr = malloc( sizeof(*palettePtr) ))) return 0;
    palettePtr->unrealize = NULL;
    palettePtr->version = palette->palVersion;
    palettePtr->count   = count;
    size = palettePtr->count * sizeof(*palettePtr->entries);
    if (!(palettePtr->entries = malloc( size )))
    {
        free( palettePtr );
        return 0;
    }
    memcpy( palettePtr->entries, palette->palPalEntry, size );
    if (!(hpalette = alloc_gdi_handle( &palettePtr->obj, NTGDI_OBJ_PAL, &palette_funcs )))
    {
        free( palettePtr->entries );
        free( palettePtr );
    }
    TRACE("   returning %p\n", hpalette);
    return hpalette;
}


/***********************************************************************
 *           NtGdiCreateHalftonePalette    (win32u.@)
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
HPALETTE WINAPI NtGdiCreateHalftonePalette( HDC hdc )
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
    return NtGdiCreatePaletteInternal( pal, pal->palNumEntries );
}


UINT get_palette_entries( HPALETTE hpalette, UINT start, UINT count, PALETTEENTRY *entries )
{
    PALETTEOBJ * palPtr;
    UINT numEntries;

    TRACE("hpal = %p, count=%i\n", hpalette, count );

    palPtr = GDI_GetObjPtr( hpalette, NTGDI_OBJ_PAL );
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


static UINT set_palette_entries( HPALETTE hpalette, UINT start, UINT count,
                                 const PALETTEENTRY *entries )
{
    PALETTEOBJ * palPtr;
    UINT numEntries;

    TRACE("hpal=%p,start=%i,count=%i\n",hpalette,start,count );

    if (hpalette == GetStockObject(DEFAULT_PALETTE)) return 0;
    palPtr = GDI_GetObjPtr( hpalette, NTGDI_OBJ_PAL );
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
    NtGdiUnrealizeObject( hpalette );
    return count;
}


/***********************************************************************
 *           NtGdiResizePalette    (win32u.@)
 *
 * Resizes logical palette.
 */
BOOL WINAPI NtGdiResizePalette( HPALETTE hPal, UINT count )
{
    PALETTEOBJ * palPtr = GDI_GetObjPtr( hPal, NTGDI_OBJ_PAL );
    PALETTEENTRY *entries;

    if( !palPtr ) return FALSE;
    TRACE("hpal = %p, prev = %i, new = %i\n", hPal, palPtr->count, count );

    if (!(entries = realloc( palPtr->entries, count * sizeof(*palPtr->entries) )))
    {
        GDI_ReleaseObj( hPal );
        return FALSE;
    }
    if (count > palPtr->count)
        memset( entries + palPtr->count, 0, (count - palPtr->count) * sizeof(*palPtr->entries) );
    palPtr->entries = entries;
    palPtr->count = count;

    GDI_ReleaseObj( hPal );
    PALETTE_UnrealizeObject( hPal );
    return TRUE;
}


/* Replaces entries in logical palette. */
static BOOL animate_palette( HPALETTE hPal, UINT StartIndex, UINT NumEntries,
                             const PALETTEENTRY *PaletteColors )
{
    TRACE("%p (%i - %i)\n", hPal, StartIndex,StartIndex+NumEntries);

    if( hPal != GetStockObject(DEFAULT_PALETTE) )
    {
        PALETTEOBJ * palPtr;
        UINT pal_entries;
        const PALETTEENTRY *pptr = PaletteColors;

        palPtr = GDI_GetObjPtr( hPal, NTGDI_OBJ_PAL );
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
 *           SetSystemPaletteUse    (win32u.@)
 *
 * Specify whether the system palette contains 2 or 20 static colors.
 *
 * RETURNS
 *    Success: Previous system palette
 *    Failure: SYSPAL_ERROR
 */
UINT WINAPI NtGdiSetSystemPaletteUse( HDC hdc, UINT use )
{
    UINT old = SystemPaletteUse;

    /* Device doesn't support colour palettes */
    if (!(NtGdiGetDeviceCaps( hdc, RASTERCAPS ) & RC_PALETTE))
        return SYSPAL_ERROR;

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
 *           NtGdiGetSystemPaletteUse    (win32u.@)
 *
 * Gets state of system palette.
 */
UINT WINAPI NtGdiGetSystemPaletteUse( HDC hdc )
{
    return SystemPaletteUse;
}


static UINT get_system_palette_entries( HDC hdc, UINT start, UINT count, PALETTEENTRY *entries )
{
    UINT ret = 0;
    DC *dc;

    TRACE( "hdc=%p,start=%i,count=%i\n", hdc, start, count );

    if ((dc = get_dc_ptr( hdc )))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pGetSystemPaletteEntries );
        ret = physdev->funcs->pGetSystemPaletteEntries( physdev, start, count, entries );
        release_dc_ptr( dc );
    }
    return ret;
}


/* null driver fallback implementation for GetSystemPaletteEntries */
UINT nulldrv_GetSystemPaletteEntries( PHYSDEV dev, UINT start, UINT count, PALETTEENTRY *entries )
{
    return 0;
}

/***********************************************************************
 *           NtGdiGetNearestPaletteIndex    (win32u.@)
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
UINT WINAPI NtGdiGetNearestPaletteIndex( HPALETTE hpalette, COLORREF color )
{
    PALETTEOBJ* palObj = GDI_GetObjPtr( hpalette, NTGDI_OBJ_PAL );
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
    TRACE("(%p,%s): returning %d\n", hpalette, debugstr_color(color), index );
    return index;
}


/* null driver fallback implementation for GetNearestColor */
COLORREF nulldrv_GetNearestColor( PHYSDEV dev, COLORREF color )
{
    unsigned char spec_type;
    DC *dc = get_nulldrv_dc( dev );

    if (!(NtGdiGetDeviceCaps( dev->hdc, RASTERCAPS ) & RC_PALETTE)) return color;

    spec_type = color >> 24;
    if (spec_type == 1 || spec_type == 2)
    {
        /* we need logical palette for PALETTERGB and PALETTEINDEX colorrefs */
        UINT index;
        PALETTEENTRY entry;
        HPALETTE hpal = dc->hPalette;

        if (!hpal) hpal = GetStockObject( DEFAULT_PALETTE );
        if (spec_type == 2) /* PALETTERGB */
            index = NtGdiGetNearestPaletteIndex( hpal, color );
        else  /* PALETTEINDEX */
            index = LOWORD(color);

        if (!get_palette_entries( hpal, index, 1, &entry ))
        {
            WARN("%s: idx %d is out of bounds, assuming NULL\n", debugstr_color(color), index );
            if (!get_palette_entries( hpal, 0, 1, &entry )) return CLR_INVALID;
        }
        color = RGB( entry.peRed, entry.peGreen, entry.peBlue );
    }
    return color & 0x00ffffff;
}


/***********************************************************************
 *           NtGdiGetNearestColor    (win32u.@)
 *
 * Gets a system color to match.
 *
 * RETURNS
 *    Success: Color from system palette that corresponds to given color
 *    Failure: CLR_INVALID
 */
COLORREF WINAPI NtGdiGetNearestColor( HDC hdc, COLORREF color )
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
    PALETTEOBJ *palette = GDI_GetObjPtr( handle, NTGDI_OBJ_PAL );

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
    PALETTEOBJ *palette = GDI_GetObjPtr( handle, NTGDI_OBJ_PAL );

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
    free( obj->entries );
    free( obj );
    return TRUE;
}


/***********************************************************************
 *           NtUserSelectPalette    (win32u.@)
 */
HPALETTE WINAPI NtUserSelectPalette( HDC hdc, HPALETTE hpal, WORD bkg )
{
    BOOL is_primary = FALSE;
    HPALETTE ret = 0;
    DC *dc;

    TRACE("%p %p\n", hdc, hpal );

    if (!bkg && hpal != GetStockObject( DEFAULT_PALETTE ))
    {
        HWND hwnd = NtUserWindowFromDC( hdc );
        if (hwnd)
        {
            /* set primary palette if it's related to current active */
            HWND foreground = NtUserGetForegroundWindow();
            is_primary = foreground == hwnd || is_child( foreground, hwnd );
        }
    }

    if (get_gdi_object_type(hpal) != NTGDI_OBJ_PAL)
    {
      WARN("invalid selected palette %p\n",hpal);
      return 0;
    }
    if ((dc = get_dc_ptr( hdc )))
    {
        ret = dc->hPalette;
        dc->hPalette = hpal;
        if (is_primary) hPrimaryPalette = hpal;
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           NtUserRealizePalette    (win32u.@)
 */
UINT WINAPI NtUserRealizePalette( HDC hdc )
{
    BOOL is_primary = FALSE;
    UINT realized = 0;
    DC* dc = get_dc_ptr( hdc );

    TRACE( "%p\n", hdc );
    if (!dc) return 0;

    /* FIXME: move primary palette handling from user32 */

    if( dc->hPalette == GetStockObject( DEFAULT_PALETTE ))
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pRealizeDefaultPalette );
        realized = physdev->funcs->pRealizeDefaultPalette( physdev );
    }
    else if (InterlockedExchangePointer( (void **)&hLastRealizedPalette, dc->hPalette ) != dc->hPalette)
    {
        PHYSDEV physdev = GET_DC_PHYSDEV( dc, pRealizePalette );
        PALETTEOBJ *palPtr = GDI_GetObjPtr( dc->hPalette, NTGDI_OBJ_PAL );
        if (palPtr)
        {
            realized = physdev->funcs->pRealizePalette( physdev, dc->hPalette,
                                                        (dc->hPalette == hPrimaryPalette) );
            palPtr->unrealize = physdev->funcs->pUnrealizePalette;
            GDI_ReleaseObj( dc->hPalette );
            is_primary = dc->hPalette == hPrimaryPalette;
        }
    }
    else TRACE("  skipping (hLastRealizedPalette = %p)\n", hLastRealizedPalette);

    release_dc_ptr( dc );
    TRACE("   realized %i colors.\n", realized );

    /* do not send anything if no colors were changed */
    if (realized && is_primary)
    {
        /* send palette change notification */
        HWND hwnd = NtUserWindowFromDC( hdc );
        if (hwnd) send_message_timeout( HWND_BROADCAST, WM_PALETTECHANGED, HandleToUlong(hwnd), 0,
                                        SMTO_ABORTIFHUNG, 2000, FALSE );
    }
    return realized;
}


/**********************************************************************
 *           NtGdiUpdateColors    (win32u.@)
 *
 * Remaps current colors to logical palette.
 */
BOOL WINAPI NtGdiUpdateColors( HDC hDC )
{
    int size = NtGdiGetDeviceCaps( hDC, SIZEPALETTE );
    HWND hwnd;

    if (!size) return FALSE;

    hwnd = NtUserWindowFromDC( hDC );

    /* Docs say that we have to remap current drawable pixel by pixel
     * but it would take forever given the speed of XGet/PutPixel.
     */
    if (hwnd && size) NtUserRedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE );
    return TRUE;
}

/*********************************************************************
 *           NtGdiSetMagicColors   (win32u.@)
 */
BOOL WINAPI NtGdiSetMagicColors( HDC hdc, DWORD magic, ULONG index )
{
    FIXME( "(%p 0x%08x 0x%08x): stub\n", hdc, magic, index );
    return TRUE;
}

/*********************************************************************
 *           NtGdiDoPalette   (win32u.@)
 */
LONG WINAPI NtGdiDoPalette( HGDIOBJ handle, WORD start, WORD count, void *entries,
                            DWORD func, BOOL inbound )
{
    switch (func)
    {
    case NtGdiAnimatePalette:
        return animate_palette( handle, start, count, entries );
    case NtGdiSetPaletteEntries:
        return set_palette_entries( handle, start, count, entries );
    case NtGdiGetPaletteEntries:
        return get_palette_entries( handle, start, count, entries );
    case NtGdiGetSystemPaletteEntries:
        return get_system_palette_entries( handle, start, count, entries );
    case NtGdiSetDIBColorTable:
        return set_dib_dc_color_table( handle, start, count, entries );
    case NtGdiGetDIBColorTable:
        return get_dib_dc_color_table( handle, start, count, entries );
    default:
        WARN( "invalid func %u\n", func );
        return 0;
    }
}
