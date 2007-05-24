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
#include "wingdi.h"
#include "wownt32.h"
#include "wine/winuser16.h"
#include "gdi_private.h"
#include "wine/debug.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(palette);

typedef struct tagPALETTEOBJ
{
    GDIOBJHDR           header;
    const DC_FUNCTIONS *funcs;      /* DC function table */
    LOGPALETTE          logpalette; /* _MUST_ be the last field */
} PALETTEOBJ;

static INT PALETTE_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static BOOL PALETTE_UnrealizeObject( HGDIOBJ handle, void *obj );
static BOOL PALETTE_DeleteObject( HGDIOBJ handle, void *obj );

static const struct gdi_obj_funcs palette_funcs =
{
    NULL,                     /* pSelectObject */
    PALETTE_GetObject,        /* pGetObject16 */
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

#define NB_RESERVED_COLORS  20   /* number of fixed colors in system palette */

static const PALETTEENTRY sys_pal_template[NB_RESERVED_COLORS] =
{
    /* first 10 entries in the system palette */
    /* red  green blue  flags */
    { 0x00, 0x00, 0x00, 0 },
    { 0x80, 0x00, 0x00, 0 },
    { 0x00, 0x80, 0x00, 0 },
    { 0x80, 0x80, 0x00, 0 },
    { 0x00, 0x00, 0x80, 0 },
    { 0x80, 0x00, 0x80, 0 },
    { 0x00, 0x80, 0x80, 0 },
    { 0xc0, 0xc0, 0xc0, 0 },
    { 0xc0, 0xdc, 0xc0, 0 },
    { 0xa6, 0xca, 0xf0, 0 },

    /* ... c_min/2 dynamic colorcells */

    /* ... gap (for sparse palettes) */

    /* ... c_min/2 dynamic colorcells */

    { 0xff, 0xfb, 0xf0, 0 },
    { 0xa0, 0xa0, 0xa4, 0 },
    { 0x80, 0x80, 0x80, 0 },
    { 0xff, 0x00, 0x00, 0 },
    { 0x00, 0xff, 0x00, 0 },
    { 0xff, 0xff, 0x00, 0 },
    { 0x00, 0x00, 0xff, 0 },
    { 0xff, 0x00, 0xff, 0 },
    { 0x00, 0xff, 0xff, 0 },
    { 0xff, 0xff, 0xff, 0 }     /* last 10 */
};

/***********************************************************************
 *           PALETTE_Init
 *
 * Create the system palette.
 */
HPALETTE PALETTE_Init(void)
{
    HPALETTE          hpalette;
    LOGPALETTE *        palPtr;

    /* create default palette (20 system colors) */

    palPtr = HeapAlloc( GetProcessHeap(), 0,
             sizeof(LOGPALETTE) + (NB_RESERVED_COLORS-1)*sizeof(PALETTEENTRY));
    if (!palPtr) return FALSE;

    palPtr->palVersion = 0x300;
    palPtr->palNumEntries = NB_RESERVED_COLORS;
    memcpy( palPtr->palPalEntry, sys_pal_template, sizeof(sys_pal_template) );
    hpalette = CreatePalette( palPtr );
    HeapFree( GetProcessHeap(), 0, palPtr );
    return hpalette;
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

    size = sizeof(LOGPALETTE) + (palette->palNumEntries - 1) * sizeof(PALETTEENTRY);

    if (!(palettePtr = GDI_AllocObject( size + sizeof(int*) +sizeof(GDIOBJHDR),
                                        PALETTE_MAGIC, (HGDIOBJ *)&hpalette,
					&palette_funcs ))) return 0;
    memcpy( &palettePtr->logpalette, palette, size );
    palettePtr->funcs = NULL;
    GDI_ReleaseObj( hpalette );

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
    int i;
    struct {
	WORD Version;
	WORD NumberOfEntries;
	PALETTEENTRY aEntries[256];
    } Palette;

    Palette.Version = 0x300;
    Palette.NumberOfEntries = 256;
    GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);

    Palette.NumberOfEntries = 20;

    for (i = 0; i < Palette.NumberOfEntries; i++)
    {
        Palette.aEntries[i].peRed=0xff;
        Palette.aEntries[i].peGreen=0xff;
        Palette.aEntries[i].peBlue=0xff;
        Palette.aEntries[i].peFlags=0x00;
    }

    Palette.aEntries[0].peRed=0x00;
    Palette.aEntries[0].peBlue=0x00;
    Palette.aEntries[0].peGreen=0x00;

    /* the first 6 */
    for (i=1; i <= 6; i++)
    {
        Palette.aEntries[i].peRed=(i%2)?0x80:0;
        Palette.aEntries[i].peGreen=(i==2)?0x80:(i==3)?0x80:(i==6)?0x80:0;
        Palette.aEntries[i].peBlue=(i>3)?0x80:0;
    }

    for (i=7;  i <= 12; i++)
    {
        switch(i)
        {
            case 7:
                Palette.aEntries[i].peRed=0xc0;
                Palette.aEntries[i].peBlue=0xc0;
                Palette.aEntries[i].peGreen=0xc0;
                break;
            case 8:
                Palette.aEntries[i].peRed=0xc0;
                Palette.aEntries[i].peGreen=0xdc;
                Palette.aEntries[i].peBlue=0xc0;
                break;
            case 9:
                Palette.aEntries[i].peRed=0xa6;
                Palette.aEntries[i].peGreen=0xca;
                Palette.aEntries[i].peBlue=0xf0;
                break;
            case 10:
                Palette.aEntries[i].peRed=0xff;
                Palette.aEntries[i].peGreen=0xfb;
                Palette.aEntries[i].peBlue=0xf0;
                break;
            case 11:
                Palette.aEntries[i].peRed=0xa0;
                Palette.aEntries[i].peGreen=0xa0;
                Palette.aEntries[i].peBlue=0xa4;
                break;
            case 12:
                Palette.aEntries[i].peRed=0x80;
                Palette.aEntries[i].peGreen=0x80;
                Palette.aEntries[i].peBlue=0x80;
        }
    }

   for (i=13; i <= 18; i++)
    {
        Palette.aEntries[i].peRed=(i%2)?0xff:0;
        Palette.aEntries[i].peGreen=(i==14)?0xff:(i==15)?0xff:(i==18)?0xff:0;
        Palette.aEntries[i].peBlue=(i>15)?0xff:0x00;
    }

    return CreatePalette((LOGPALETTE *)&Palette);
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

    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;

    /* NOTE: not documented but test show this to be the case */
    if (count == 0)
    {
        int rc = palPtr->logpalette.palNumEntries;
	    GDI_ReleaseObj( hpalette );
        return rc;
    }

    numEntries = palPtr->logpalette.palNumEntries;
    if (start+count > numEntries) count = numEntries - start;
    if (entries)
    {
      if (start >= numEntries)
      {
	GDI_ReleaseObj( hpalette );
	return 0;
      }
      memcpy( entries, &palPtr->logpalette.palPalEntry[start],
	      count * sizeof(PALETTEENTRY) );
      for( numEntries = 0; numEntries < count ; numEntries++ )
	   if (entries[numEntries].peFlags & 0xF0)
	       entries[numEntries].peFlags = 0;
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

    if (hpalette == GetStockObject(DEFAULT_PALETTE)) return 0;
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;

    numEntries = palPtr->logpalette.palNumEntries;
    if (start >= numEntries)
    {
      GDI_ReleaseObj( hpalette );
      return 0;
    }
    if (start+count > numEntries) count = numEntries - start;
    memcpy( &palPtr->logpalette.palPalEntry[start], entries,
	    count * sizeof(PALETTEENTRY) );
    UnrealizeObject( hpalette );
    GDI_ReleaseObj( hpalette );
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
    PALETTEOBJ * palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hPal, PALETTE_MAGIC );
    UINT	 cPrevEnt, prevVer;
    int		 prevsize, size = sizeof(LOGPALETTE) + (cEntries - 1) * sizeof(PALETTEENTRY);

    TRACE("hpal = %p, prev = %i, new = %i\n",
          hPal, palPtr ? palPtr->logpalette.palNumEntries : -1, cEntries );
    if( !palPtr ) return FALSE;
    cPrevEnt = palPtr->logpalette.palNumEntries;
    prevVer = palPtr->logpalette.palVersion;
    prevsize = sizeof(LOGPALETTE) + (cPrevEnt - 1) * sizeof(PALETTEENTRY) +
	      				sizeof(int*) + sizeof(GDIOBJHDR);
    size += sizeof(int*) + sizeof(GDIOBJHDR);

    if (!(palPtr = GDI_ReallocObject( size, hPal, palPtr ))) return FALSE;

    PALETTE_UnrealizeObject( hPal, palPtr );

    if( cEntries > cPrevEnt ) memset( (BYTE*)palPtr + prevsize, 0, size - prevsize );
    palPtr->logpalette.palNumEntries = cEntries;
    palPtr->logpalette.palVersion = prevVer;
    GDI_ReleaseObj( hPal );
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

    if( hPal != GetStockObject(DEFAULT_PALETTE) )
    {
        PALETTEOBJ * palPtr;
        UINT pal_entries;
        const PALETTEENTRY *pptr = PaletteColors;

        palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hPal, PALETTE_MAGIC );
        if (!palPtr) return 0;

        pal_entries = palPtr->logpalette.palNumEntries;
        if (StartIndex >= pal_entries)
        {
          GDI_ReleaseObj( hPal );
          return 0;
        }
        if (StartIndex+NumEntries > pal_entries) NumEntries = pal_entries - StartIndex;
        
        for (NumEntries += StartIndex; StartIndex < NumEntries; StartIndex++, pptr++) {
          /* According to MSDN, only animate PC_RESERVED colours */
          if (palPtr->logpalette.palPalEntry[StartIndex].peFlags & PC_RESERVED) {
            TRACE("Animating colour (%d,%d,%d) to (%d,%d,%d)\n",
              palPtr->logpalette.palPalEntry[StartIndex].peRed,
              palPtr->logpalette.palPalEntry[StartIndex].peGreen,
              palPtr->logpalette.palPalEntry[StartIndex].peBlue,
              pptr->peRed, pptr->peGreen, pptr->peBlue);
            memcpy( &palPtr->logpalette.palPalEntry[StartIndex], pptr,
                    sizeof(PALETTEENTRY) );
          } else {
            TRACE("Not animating entry %d -- not PC_RESERVED\n", StartIndex);
          }
        }
        if (palPtr->funcs && palPtr->funcs->pRealizePalette)
            palPtr->funcs->pRealizePalette( NULL, hPal, hPal == hPrimaryPalette );

        GDI_ReleaseObj( hPal );
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

    if ((dc = DC_GetDCPtr( hdc )))
    {
        if (dc->funcs->pGetSystemPaletteEntries)
            ret = dc->funcs->pGetSystemPaletteEntries( dc->physDev, start, count, entries );
        GDI_ReleaseObj( hdc );
    }
    return ret;
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
    PALETTEOBJ*	palObj = (PALETTEOBJ*)GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    UINT index  = 0;

    if( palObj )
    {
        int i, diff = 0x7fffffff;
        int r,g,b;
        PALETTEENTRY* entry = palObj->logpalette.palPalEntry;

        for( i = 0; i < palObj->logpalette.palNumEntries && diff ; i++, entry++)
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
    unsigned char spec_type;
    COLORREF nearest;
    DC 		*dc;

    if (!(dc = DC_GetDCPtr( hdc ))) return CLR_INVALID;

    if (dc->funcs->pGetNearestColor)
    {
        nearest = dc->funcs->pGetNearestColor( dc->physDev, color );
        GDI_ReleaseObj( hdc );
        return nearest;
    }

    if (!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE))
    {
        GDI_ReleaseObj( hdc );
        return color;
    }

    spec_type = color >> 24;
    if (spec_type == 1 || spec_type == 2)
    {
        /* we need logical palette for PALETTERGB and PALETTEINDEX colorrefs */

        UINT index;
        PALETTEENTRY entry;
        HPALETTE hpal = dc->hPalette ? dc->hPalette : GetStockObject( DEFAULT_PALETTE );

        if (spec_type == 2) /* PALETTERGB */
            index = GetNearestPaletteIndex( hpal, color );
        else  /* PALETTEINDEX */
            index = LOWORD(color);

        if (!GetPaletteEntries( hpal, index, 1, &entry ))
        {
            WARN("RGB(%x) : idx %d is out of bounds, assuming NULL\n", color, index );
            if (!GetPaletteEntries( hpal, 0, 1, &entry ))
            {
                GDI_ReleaseObj( hdc );
                return CLR_INVALID;
            }
        }
        color = RGB( entry.peRed, entry.peGreen, entry.peBlue );
    }
    nearest = color & 0x00ffffff;
    GDI_ReleaseObj( hdc );

    TRACE("(%06x): returning %06x\n", color, nearest );
    return nearest;
}


/***********************************************************************
 *           PALETTE_GetObject
 */
static INT PALETTE_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    PALETTEOBJ *palette = obj;

    if( !buffer )
        return sizeof(WORD);

    if (count > sizeof(WORD)) count = sizeof(WORD);
    memcpy( buffer, &palette->logpalette.palNumEntries, count );
    return count;
}


/***********************************************************************
 *           PALETTE_UnrealizeObject
 */
static BOOL PALETTE_UnrealizeObject( HGDIOBJ handle, void *obj )
{
    PALETTEOBJ *palette = obj;

    if (palette->funcs)
    {
        if (palette->funcs->pUnrealizePalette)
            palette->funcs->pUnrealizePalette( handle );
        palette->funcs = NULL;
    }

    if (hLastRealizedPalette == handle)
    {
        TRACE("unrealizing palette %p\n", handle);
        hLastRealizedPalette = 0;
    }
    return TRUE;
}


/***********************************************************************
 *           PALETTE_DeleteObject
 */
static BOOL PALETTE_DeleteObject( HGDIOBJ handle, void *obj )
{
    PALETTE_UnrealizeObject( handle, obj );
    return GDI_FreeObject( handle, obj );
}


/***********************************************************************
 *           GDISelectPalette    (Not a Windows API)
 */
HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg)
{
    HPALETTE ret;
    DC *dc;

    TRACE("%p %p\n", hdc, hpal );

    if (GetObjectType(hpal) != OBJ_PAL)
    {
      WARN("invalid selected palette %p\n",hpal);
      return 0;
    }
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    ret = dc->hPalette;
    if (dc->funcs->pSelectPalette) hpal = dc->funcs->pSelectPalette( dc->physDev, hpal, FALSE );
    if (hpal)
    {
        dc->hPalette = hpal;
        if (!wBkg) hPrimaryPalette = hpal;
    }
    else ret = 0;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           GDIRealizePalette    (Not a Windows API)
 */
UINT WINAPI GDIRealizePalette( HDC hdc )
{
    UINT realized = 0;
    DC* dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;

    TRACE("%p...\n", hdc );

    if( dc->hPalette == GetStockObject( DEFAULT_PALETTE ))
    {
        if (dc->funcs->pRealizeDefaultPalette)
            realized = dc->funcs->pRealizeDefaultPalette( dc->physDev );
    }
    else if(dc->hPalette != hLastRealizedPalette )
    {
        if (dc->funcs->pRealizePalette)
        {
            PALETTEOBJ *palPtr = GDI_GetObjPtr( dc->hPalette, PALETTE_MAGIC );
            if (palPtr)
            {
                realized = dc->funcs->pRealizePalette( dc->physDev, dc->hPalette,
                                                       (dc->hPalette == hPrimaryPalette) );
                palPtr->funcs = dc->funcs;
                GDI_ReleaseObj( dc->hPalette );
            }
        }
        hLastRealizedPalette = dc->hPalette;
    }
    else TRACE("  skipping (hLastRealizedPalette = %p)\n", hLastRealizedPalette);

    GDI_ReleaseObj( hdc );
    TRACE("   realized %i colors.\n", realized );
    return realized;
}


/***********************************************************************
 *           RealizeDefaultPalette    (GDI.365)
 */
UINT16 WINAPI RealizeDefaultPalette16( HDC16 hdc )
{
    UINT16 ret = 0;
    DC          *dc;

    TRACE("%04x\n", hdc );

    if (!(dc = DC_GetDCPtr( HDC_32(hdc) ))) return 0;

    if (dc->funcs->pRealizeDefaultPalette) ret = dc->funcs->pRealizeDefaultPalette( dc->physDev );
    GDI_ReleaseObj( HDC_32(hdc) );
    return ret;
}

/***********************************************************************
 *           IsDCCurrentPalette   (GDI.412)
 */
BOOL16 WINAPI IsDCCurrentPalette16(HDC16 hDC)
{
    DC *dc = DC_GetDCPtr( HDC_32(hDC) );
    if (dc)
    {
      BOOL bRet = dc->hPalette == hPrimaryPalette;
      GDI_ReleaseObj( HDC_32(hDC) );
      return bRet;
    }
    return FALSE;
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

    if (!size) return 0;

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
    return 0x666;
}


/*********************************************************************
 *           SetMagicColors   (GDI.606)
 */
VOID WINAPI SetMagicColors16(HDC16 hDC, COLORREF color, UINT16 index)
{
    FIXME("(hDC %04x, color %04x, index %04x): stub\n", hDC, (int)color, index);

}

/*********************************************************************
 *           SetMagicColors   (GDI.@)
 */
BOOL WINAPI SetMagicColors(HDC hdc, ULONG u1, ULONG u2)
{
    FIXME("(%p 0x%08x 0x%08x): stub\n", hdc, u1, u2);
    return TRUE;
}

/**********************************************************************
 * GetICMProfileA [GDI32.@]
 *
 * Returns the filename of the specified device context's color
 * management profile, even if color management is not enabled
 * for that DC.
 *
 * RETURNS
 *    TRUE if name copied successfully OR lpszFilename is NULL
 *    FALSE if the buffer length pointed to by lpcbName is too small
 *
 * NOTE
 *    The buffer length pointed to by lpcbName is ALWAYS updated to
 *    the length required regardless of other actions this function
 *    may take.
 *
 * FIXME
 *    How does Windows assign these?  Some registry key?
 */


/*********************************************************************/

BOOL WINAPI GetICMProfileA(HDC hDC, LPDWORD lpcbName, LPSTR lpszFilename)
{
    DWORD callerLen;
    static const char icm[] = "winefake.icm";

    FIXME("(%p, %p, %p): partial stub\n", hDC, lpcbName, lpszFilename);

    callerLen = *lpcbName;

    /* all 3 behaviors require the required buffer size to be set */
    *lpcbName = sizeof(icm);

    /* behavior 1: if lpszFilename is NULL, return size of string and no error */
    if (!lpszFilename) return TRUE;

    /* behavior 2: if buffer size too small, return size of string and error */
    if (callerLen < sizeof(icm))
    {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }

    /* behavior 3: if buffer size OK and pointer not NULL, copy and return size */
    memcpy(lpszFilename, icm, sizeof(icm));
    return TRUE;
}

/**********************************************************************
 * GetICMProfileW [GDI32.@]
 **/
BOOL WINAPI GetICMProfileW(HDC hDC, LPDWORD lpcbName, LPWSTR lpszFilename)
{
    DWORD callerLen;
    static const WCHAR icm[] = { 'w','i','n','e','f','a','k','e','.','i','c','m', 0 };

    FIXME("(%p, %p, %p): partial stub\n", hDC, lpcbName, lpszFilename);

    callerLen = *lpcbName;

    /* all 3 behaviors require the required buffer size to be set */
    *lpcbName = sizeof(icm) / sizeof(WCHAR);

    /* behavior 1: if lpszFilename is NULL, return size of string and no error */
    if (!lpszFilename) return TRUE;

    /* behavior 2: if buffer size too small, return size of string and error */
    if (callerLen < sizeof(icm)/sizeof(WCHAR))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    /* behavior 3: if buffer size OK and pointer not NULL, copy and return size */
    memcpy(lpszFilename, icm, sizeof(icm));
    return TRUE;
}

/**********************************************************************
 * GetLogColorSpaceA [GDI32.@]
 *
 */
BOOL WINAPI GetLogColorSpaceA(HCOLORSPACE hColorSpace, LPLOGCOLORSPACEA lpBuffer, DWORD nSize)
{
    FIXME("%p %p 0x%08x: stub!\n", hColorSpace, lpBuffer, nSize);
    return FALSE;
}

/**********************************************************************
 * GetLogColorSpaceW [GDI32.@]
 *
 */
BOOL WINAPI GetLogColorSpaceW(HCOLORSPACE hColorSpace, LPLOGCOLORSPACEW lpBuffer, DWORD nSize)
{
    FIXME("%p %p 0x%08x: stub!\n", hColorSpace, lpBuffer, nSize);
    return FALSE;
}

/**********************************************************************
 * SetICMProfileA [GDI32.@]
 *
 */
BOOL WINAPI SetICMProfileA(HDC hDC, LPSTR lpszFilename)
{
    FIXME("hDC %p filename %s: stub!\n", hDC, debugstr_a(lpszFilename));
    return TRUE; /* success */
}

/**********************************************************************
 * SetICMProfileA [GDI32.@]
 *
 */
BOOL WINAPI SetICMProfileW(HDC hDC, LPWSTR lpszFilename)
{
    FIXME("hDC %p filename %s: stub!\n", hDC, debugstr_w(lpszFilename));
    return TRUE; /* success */
}

/**********************************************************************
 * UpdateICMRegKeyA [GDI32.@]
 *
 */
BOOL WINAPI UpdateICMRegKeyA(DWORD dwReserved, LPSTR lpszCMID, LPSTR lpszFileName, UINT nCommand)
{
    FIXME("(0x%08x, %s, %s, 0x%08x): stub!\n", dwReserved, debugstr_a(lpszCMID),
          debugstr_a(lpszFileName), nCommand);
    return TRUE; /* success */
}

/**********************************************************************
 * UpdateICMRegKeyW [GDI32.@]
 *
 */
BOOL WINAPI UpdateICMRegKeyW(DWORD dwReserved, LPWSTR lpszCMID, LPWSTR lpszFileName, UINT nCommand)
{
    FIXME("(0x%08x, %s, %s, 0x%08x): stub!\n", dwReserved, debugstr_w(lpszCMID),
          debugstr_w(lpszFileName), nCommand);
    return TRUE; /* success */
}
