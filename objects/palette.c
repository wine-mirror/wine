/*
 * GDI palette objects
 *
 * Copyright 1993,1994 Alexandre Julliard
 * Copyright 1996 Alex Korobka
 *
 * PALETTEOBJ is documented in the Dr. Dobbs Journal May 1993.
 * Information in the "Undocumented Windows" is incorrect.
 */

#include <stdlib.h>
#include <string.h>
#include "ts_xlib.h"

#include "gdi.h"
#include "color.h"
#include "palette.h"
#include "xmalloc.h"
#include "debug.h"

static UINT32 SystemPaletteUse = SYSPAL_STATIC;  /* currently not considered */

static HPALETTE16 hPrimaryPalette = 0; /* used for WM_PALETTECHANGED */
static HPALETTE16 hLastRealizedPalette = 0; /* UnrealizeObject() needs it */


/***********************************************************************
 *           PALETTE_Init
 *
 * Create the system palette.
 */
HPALETTE16 PALETTE_Init(void)
{
    int                 i;
    HPALETTE16          hpalette;
    LOGPALETTE *        palPtr;
    PALETTEOBJ*         palObj;
    const PALETTEENTRY* __sysPalTemplate = COLOR_GetSystemPaletteTemplate();

    /* create default palette (20 system colors) */

    palPtr = HeapAlloc( GetProcessHeap(), 0,
             sizeof(LOGPALETTE) + (NB_RESERVED_COLORS-1)*sizeof(PALETTEENTRY));
    if (!palPtr) return FALSE;

    palPtr->palVersion = 0x300;
    palPtr->palNumEntries = NB_RESERVED_COLORS;
    for( i = 0; i < NB_RESERVED_COLORS; i ++ )
    {
        palPtr->palPalEntry[i].peRed = __sysPalTemplate[i].peRed;
        palPtr->palPalEntry[i].peGreen = __sysPalTemplate[i].peGreen;
        palPtr->palPalEntry[i].peBlue = __sysPalTemplate[i].peBlue;
        palPtr->palPalEntry[i].peFlags = 0;
    }
    hpalette = CreatePalette16( palPtr );

    palObj = (PALETTEOBJ*) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );

    palObj->mapping = xmalloc( sizeof(int) * 20 );

    GDI_HEAP_UNLOCK( hpalette );

    HeapFree( GetProcessHeap(), 0, palPtr );
    return hpalette;
}

/***********************************************************************
 *           PALETTE_ValidateFlags
 */
void PALETTE_ValidateFlags(PALETTEENTRY* lpPalE, int size)
{
    int i = 0;
    for( ; i<size ; i++ )
        lpPalE[i].peFlags = PC_SYS_USED | (lpPalE[i].peFlags & 0x07);
}


/***********************************************************************
 *           CreatePalette16    (GDI.360)
 */
HPALETTE16 WINAPI CreatePalette16( const LOGPALETTE* palette )
{
    return CreatePalette32( palette );
}


/***********************************************************************
 * CreatePalette32 [GDI32.53]  Creates a logical color palette
 *
 * RETURNS
 *    Success: Handle to logical palette
 *    Failure: NULL
 */
HPALETTE32 WINAPI CreatePalette32(
    const LOGPALETTE* palette) /* [in] Pointer to logical color palette */
{
    PALETTEOBJ * palettePtr;
    HPALETTE32 hpalette;
    int size = sizeof(LOGPALETTE) + (palette->palNumEntries - 1) * sizeof(PALETTEENTRY);

    TRACE(palette,"entries=%i\n", palette->palNumEntries);

    hpalette = GDI_AllocObject( size + sizeof(int*) +sizeof(GDIOBJHDR) , PALETTE_MAGIC );
    if (!hpalette) return 0;

    palettePtr = (PALETTEOBJ *) GDI_HEAP_LOCK( hpalette );
    memcpy( &palettePtr->logpalette, palette, size );
    PALETTE_ValidateFlags(palettePtr->logpalette.palPalEntry, 
			  palettePtr->logpalette.palNumEntries);
    palettePtr->mapping = NULL;
    GDI_HEAP_UNLOCK( hpalette );

    TRACE(palette,"   returning %04x\n", hpalette);
    return hpalette;
}


/***********************************************************************
 * CreateHalftonePalette [GDI32.47]  Creates a halftone palette
 *
 * RETURNS
 *    Success: Handle to logical halftone palette
 *    Failure: 0
 */
HPALETTE32 WINAPI CreateHalftonePalette(
    HDC32 hdc) /* [in] Handle to device context */
{
    FIXME(palette,"(%x): stub\n", hdc);
    return (HPALETTE32)NULL;
}


/***********************************************************************
 *           GetPaletteEntries16    (GDI.363)
 */
UINT16 WINAPI GetPaletteEntries16( HPALETTE16 hpalette, UINT16 start,
                                   UINT16 count, LPPALETTEENTRY entries )
{
    return GetPaletteEntries32( hpalette, start, count, entries );
}


/***********************************************************************
 * GetPaletteEntries32 [GDI32.209]  Retrieves palette entries
 *
 * RETURNS
 *    Success: Number of entries from logical palette
 *    Failure: 0
 */
UINT32 WINAPI GetPaletteEntries32(
    HPALETTE32 hpalette,    /* [in]  Handle of logical palette */
    UINT32 start,           /* [in]  First entry to receive */
    UINT32 count,           /* [in]  Number of entries to receive */
    LPPALETTEENTRY entries) /* [out] Address of array receiving entries */
{
    PALETTEOBJ * palPtr;
    INT32 numEntries;

    TRACE(palette,"hpal = %04x, count=%i\n", hpalette, count );
        
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;

    numEntries = palPtr->logpalette.palNumEntries;
    if (start >= numEntries) 
    {
      GDI_HEAP_UNLOCK( hpalette );
      return 0;
    }
    if (start+count > numEntries) count = numEntries - start;
    memcpy( entries, &palPtr->logpalette.palPalEntry[start],
	    count * sizeof(PALETTEENTRY) );
    for( numEntries = 0; numEntries < count ; numEntries++ )
         if (entries[numEntries].peFlags & 0xF0)
             entries[numEntries].peFlags = 0;
    GDI_HEAP_UNLOCK( hpalette );
    return count;
}


/***********************************************************************
 *           SetPaletteEntries16    (GDI.364)
 */
UINT16 WINAPI SetPaletteEntries16( HPALETTE16 hpalette, UINT16 start,
                                   UINT16 count, LPPALETTEENTRY entries )
{
    return SetPaletteEntries32( hpalette, start, count, entries );
}


/***********************************************************************
 * SetPaletteEntries32 [GDI32.326]  Sets color values for range in palette
 *
 * RETURNS
 *    Success: Number of entries that were set
 *    Failure: 0
 */
UINT32 WINAPI SetPaletteEntries32(
    HPALETTE32 hpalette,    /* [in] Handle of logical palette */
    UINT32 start,           /* [in] Index of first entry to set */
    UINT32 count,           /* [in] Number of entries to set */
    LPPALETTEENTRY entries) /* [in] Address of array of structures */
{
    PALETTEOBJ * palPtr;
    INT32 numEntries;

    TRACE(palette,"hpal=%04x,start=%i,count=%i\n",hpalette,start,count );

    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;

    numEntries = palPtr->logpalette.palNumEntries;
    if (start >= numEntries) 
    {
      GDI_HEAP_UNLOCK( hpalette );
      return 0;
    }
    if (start+count > numEntries) count = numEntries - start;
    memcpy( &palPtr->logpalette.palPalEntry[start], entries,
	    count * sizeof(PALETTEENTRY) );
    PALETTE_ValidateFlags(palPtr->logpalette.palPalEntry, 
			  palPtr->logpalette.palNumEntries);
    free(palPtr->mapping);
    palPtr->mapping = NULL;
    GDI_HEAP_UNLOCK( hpalette );
    return count;
}


/***********************************************************************
 *           ResizePalette16   (GDI.368)
 */
BOOL16 WINAPI ResizePalette16( HPALETTE16 hPal, UINT16 cEntries )
{
    return ResizePalette32( hPal, cEntries );
}


/***********************************************************************
 * ResizePalette32 [GDI32.289]  Resizes logical palette
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI ResizePalette32(
    HPALETTE32 hPal, /* [in] Handle of logical palette */
    UINT32 cEntries) /* [in] Number of entries in logical palette */
{
    PALETTEOBJ * palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hPal, PALETTE_MAGIC );
    UINT32	 cPrevEnt, prevVer;
    int		 prevsize, size = sizeof(LOGPALETTE) + (cEntries - 1) * sizeof(PALETTEENTRY);
    int*	 mapping = NULL;

    TRACE(palette,"hpal = %04x, prev = %i, new = %i\n",
		    hPal, palPtr ? palPtr->logpalette.palNumEntries : -1,
                    cEntries );
    if( !palPtr ) return FALSE;
    cPrevEnt = palPtr->logpalette.palNumEntries;
    prevVer = palPtr->logpalette.palVersion;
    prevsize = sizeof(LOGPALETTE) + (cPrevEnt - 1) * sizeof(PALETTEENTRY) +
	      				sizeof(int*) + sizeof(GDIOBJHDR);
    size += sizeof(int*) + sizeof(GDIOBJHDR);
    mapping = palPtr->mapping;
    
    GDI_HEAP_UNLOCK( hPal );
    
    hPal = GDI_HEAP_REALLOC( hPal, size );
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hPal, PALETTE_MAGIC );
    if( !palPtr ) return FALSE;

    if( mapping )
        palPtr->mapping = (int*) xrealloc( mapping, cEntries * sizeof(int) );
    if( cEntries > cPrevEnt ) 
    {
	if( mapping )
	    memset(palPtr->mapping + cPrevEnt, 0, (cEntries - cPrevEnt)*sizeof(int));
	memset( (BYTE*)palPtr + prevsize, 0, size - prevsize );
        PALETTE_ValidateFlags((PALETTEENTRY*)((BYTE*)palPtr + prevsize), 
						     cEntries - cPrevEnt );
    }
    palPtr->logpalette.palNumEntries = cEntries;
    palPtr->logpalette.palVersion = prevVer;
    GDI_HEAP_UNLOCK( hPal );
    return TRUE;
}


/***********************************************************************
 *           AnimatePalette16   (GDI.367)
 */
void WINAPI AnimatePalette16( HPALETTE16 hPal, UINT16 StartIndex,
                              UINT16 NumEntries, LPPALETTEENTRY PaletteColors)
{
    AnimatePalette32( hPal, StartIndex, NumEntries, PaletteColors );
}


/***********************************************************************
 * AnimatePalette32 [GDI32.6]  Replaces entries in logical palette
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * FIXME
 *    Should use existing mapping when animating a primary palette
 */
BOOL32 WINAPI AnimatePalette32(
    HPALETTE32 hPal,              /* [in] Handle to logical palette */
    UINT32 StartIndex,            /* [in] First entry in palette */
    UINT32 NumEntries,            /* [in] Count of entries in palette */
    LPPALETTEENTRY PaletteColors) /* [in] Pointer to first replacement */
{
    TRACE(palette, "%04x (%i - %i)\n", hPal, StartIndex,StartIndex+NumEntries);

    if( hPal != STOCK_DEFAULT_PALETTE ) 
    {
        PALETTEOBJ* palPtr = (PALETTEOBJ *)GDI_GetObjPtr(hPal, PALETTE_MAGIC);

	if( (StartIndex + NumEntries) <= palPtr->logpalette.palNumEntries )
	{
	    UINT32 u;
	    for( u = 0; u < NumEntries; u++ )
		palPtr->logpalette.palPalEntry[u + StartIndex] = PaletteColors[u];
	    COLOR_SetMapping(palPtr, StartIndex, NumEntries,
                             hPal != hPrimaryPalette );
            GDI_HEAP_UNLOCK( hPal );
	    return TRUE;
	}
    }
    return FALSE;
}


/***********************************************************************
 *           SetSystemPaletteUse16   (GDI.373)
 */
UINT16 WINAPI SetSystemPaletteUse16( HDC16 hdc, UINT16 use )
{
    return SetSystemPaletteUse32( hdc, use );
}


/***********************************************************************
 * SetSystemPaletteUse32 [GDI32.335]
 *
 * RETURNS
 *    Success: Previous system palette
 *    Failure: SYSPAL_ERROR
 */
UINT32 WINAPI SetSystemPaletteUse32(
    HDC32 hdc,  /* [in] Handle of device context */
    UINT32 use) /* [in] Palette-usage flag */
{
    UINT32 old = SystemPaletteUse;
    FIXME(palette,"(%04x,%04x): stub\n", hdc, use );
    SystemPaletteUse = use;
    return old;
}


/***********************************************************************
 *           GetSystemPaletteUse16   (GDI.374)
 */
UINT16 WINAPI GetSystemPaletteUse16( HDC16 hdc )
{
    return SystemPaletteUse;
}


/***********************************************************************
 * GetSystemPaletteUse32 [GDI32.223]  Gets state of system palette
 *
 * RETURNS
 *    Current state of system palette
 */
UINT32 WINAPI GetSystemPaletteUse32(
    HDC32 hdc) /* [in] Handle of device context */
{
    return SystemPaletteUse;
}


/***********************************************************************
 *           GetSystemPaletteEntries16   (GDI.375)
 */
UINT16 WINAPI GetSystemPaletteEntries16( HDC16 hdc, UINT16 start, UINT16 count,
                                         LPPALETTEENTRY entries )
{
    return GetSystemPaletteEntries32( hdc, start, count, entries );
}


/***********************************************************************
 * GetSystemPaletteEntries32 [GDI32.222]  Gets range of palette entries
 *
 * RETURNS
 *    Success: Number of entries retrieved from palette
 *    Failure: 0
 */
UINT32 WINAPI GetSystemPaletteEntries32(
    HDC32 hdc,              /* [in]  Handle of device context */
    UINT32 start,           /* [in]  Index of first entry to be retrieved */
    UINT32 count,           /* [in]  Number of entries to be retrieved */
    LPPALETTEENTRY entries) /* [out] Array receiving system-palette entries */
{
    UINT32 i;
    DC *dc;

    TRACE(palette, "hdc=%04x,start=%i,count=%i\n", hdc,start,count);

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (start >= dc->w.devCaps->sizePalette)
      {
	GDI_HEAP_UNLOCK( hdc );
	return 0;
      }
    if (start+count >= dc->w.devCaps->sizePalette)
	count = dc->w.devCaps->sizePalette - start;
    for (i = 0; i < count; i++)
    {
	*(COLORREF*)(entries + i) = COLOR_GetSystemPaletteEntry( start + i );

        TRACE(palette,"\tidx(%02x) -> RGB(%08lx)\n",
                         start + i, *(COLORREF*)(entries + i) );
    }
    GDI_HEAP_UNLOCK( hdc );
    return count;
}


/***********************************************************************
 *           GetNearestPaletteIndex16   (GDI.370)
 */
UINT16 WINAPI GetNearestPaletteIndex16( HPALETTE16 hpalette, COLORREF color )
{
    return GetNearestPaletteIndex32( hpalette, color );
}


/***********************************************************************
 * GetNearestPaletteIndex32 [GDI32.203]  Gets palette index for color
 *
 * NOTES
 *    Should index be initialized to CLR_INVALID instead of 0?
 *
 * RETURNS
 *    Success: Index of entry in logical palette
 *    Failure: CLR_INVALID
 */
UINT32 WINAPI GetNearestPaletteIndex32(
    HPALETTE32 hpalette, /* [in] Handle of logical color palette */
    COLORREF color)      /* [in] Color to be matched */
{
    PALETTEOBJ*	palObj = (PALETTEOBJ*)GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    UINT32 index  = 0;

    if( palObj )
        index = COLOR_PaletteLookupPixel( palObj->logpalette.palPalEntry, 
				          palObj->logpalette.palNumEntries,
                                          NULL, color, FALSE );

    TRACE(palette,"(%04x,%06lx): returning %d\n", hpalette, color, index );
    GDI_HEAP_UNLOCK( hpalette );
    return index;
}


/***********************************************************************
 *           GetNearestColor16   (GDI.154)
 */
COLORREF WINAPI GetNearestColor16( HDC16 hdc, COLORREF color )
{
    return GetNearestColor32( hdc, color );
}


/***********************************************************************
 * GetNearestColor32 [GDI32.202]  Gets a system color to match
 *
 * NOTES
 *    Should this return CLR_INVALID instead of FadeCafe?
 *
 * RETURNS
 *    Success: Color from system palette that corresponds to given color
 *    Failure: CLR_INVALID
 */
COLORREF WINAPI GetNearestColor32(
    HDC32 hdc,      /* [in] Handle of device context */
    COLORREF color) /* [in] Color to be matched */
{
    COLORREF 	 nearest = 0xFADECAFE;
    DC 		*dc;
    PALETTEOBJ  *palObj;

    if ( (dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC )) )
    {
      palObj = (PALETTEOBJ*) 
	        GDI_GetObjPtr( (dc->w.hPalette)? dc->w.hPalette
				 	       : STOCK_DEFAULT_PALETTE, PALETTE_MAGIC );

      nearest = COLOR_LookupNearestColor( palObj->logpalette.palPalEntry,
					  palObj->logpalette.palNumEntries, color );
      GDI_HEAP_UNLOCK( dc->w.hPalette );
    }

    TRACE(palette,"(%06lx): returning %06lx\n", color, nearest );
    GDI_HEAP_UNLOCK( hdc );    
    return nearest;
}


/***********************************************************************
 *           PALETTE_GetObject
 */
int PALETTE_GetObject( PALETTEOBJ * palette, int count, LPSTR buffer )
{
    if (count > sizeof(WORD)) count = sizeof(WORD);
    memcpy( buffer, &palette->logpalette.palNumEntries, count );
    return count;
}


/***********************************************************************
 *           PALETTE_UnrealizeObject
 */
BOOL32 PALETTE_UnrealizeObject( HPALETTE16 hpalette, PALETTEOBJ *palette )
{
    if (palette->mapping)
    {
        free( palette->mapping );
        palette->mapping = NULL;
    }
    if (hLastRealizedPalette == hpalette) hLastRealizedPalette = 0;
    return TRUE;
}


/***********************************************************************
 *           PALETTE_DeleteObject
 */
BOOL32 PALETTE_DeleteObject( HPALETTE16 hpalette, PALETTEOBJ *palette )
{
    free( palette->mapping );
    if (hLastRealizedPalette == hpalette) hLastRealizedPalette = 0;
    return GDI_FreeObject( hpalette );
}


/***********************************************************************
 *           GDISelectPalette    (GDI.361)
 */
HPALETTE16 WINAPI GDISelectPalette( HDC16 hdc, HPALETTE16 hpal, WORD wBkg)
{
    HPALETTE16 prev;
    DC *dc;

    TRACE(palette, "%04x %04x\n", hdc, hpal );
    
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    prev = dc->w.hPalette;
    dc->w.hPalette = hpal;
    GDI_HEAP_UNLOCK( hdc );
    if (!wBkg) hPrimaryPalette = hpal; 
    return prev;
}


/***********************************************************************
 *           GDIRealizePalette    (GDI.362)
 */
UINT16 WINAPI GDIRealizePalette( HDC16 hdc )
{
    PALETTEOBJ* palPtr;
    int realized = 0;
    DC*		dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    TRACE(palette, "%04x...\n", hdc );
    
    if( dc &&  dc->w.hPalette != hLastRealizedPalette )
    {
	if( dc->w.hPalette == STOCK_DEFAULT_PALETTE )
            return RealizeDefaultPalette( hdc );

        palPtr = (PALETTEOBJ *) GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC );

	if (!palPtr) {
		FIXME(palette,"invalid selected palette %04x\n",dc->w.hPalette);
		return 0;
	}
        
        realized = COLOR_SetMapping(palPtr,0,palPtr->logpalette.palNumEntries,
                                    (dc->w.hPalette != hPrimaryPalette) ||
                                    (dc->w.hPalette == STOCK_DEFAULT_PALETTE));
	GDI_HEAP_UNLOCK( dc->w.hPalette );
	hLastRealizedPalette = dc->w.hPalette;
    }
    else TRACE(palette, "  skipping (hLastRealizedPalette = %04x)\n",
			 hLastRealizedPalette);
    GDI_HEAP_UNLOCK( hdc );

    TRACE(palette, "   realized %i colors.\n", realized );
    return (UINT16)realized;
}


/***********************************************************************
 *           RealizeDefaultPalette    (GDI.365)
 */
UINT16 WINAPI RealizeDefaultPalette( HDC16 hdc )
{
    DC          *dc;
    PALETTEOBJ*  palPtr;
    int          i, index, realized = 0;

    TRACE(palette,"%04x\n", hdc );

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    if ( dc->w.flags & DC_MEMORY ) 
      {
	GDI_HEAP_UNLOCK( hdc );
	return 0;
      }

    hPrimaryPalette = STOCK_DEFAULT_PALETTE;
    hLastRealizedPalette = STOCK_DEFAULT_PALETTE;

    palPtr = (PALETTEOBJ*)GDI_GetObjPtr(STOCK_DEFAULT_PALETTE, PALETTE_MAGIC );

    /* lookup is needed to account for SetSystemPaletteUse() stuff */

    for( i = 0; i < 20; i++ )
       {
         index = COLOR_LookupSystemPixel(*(COLORREF*)(palPtr->logpalette.palPalEntry + i));

         /* mapping is allocated in COLOR_InitPalette() */

         if( index != palPtr->mapping[i] ) { palPtr->mapping[i]=index; realized++; }
       }
    return realized;
}

/***********************************************************************
 *           IsDCCurrentPalette   (GDI.412)
 */
BOOL16 WINAPI IsDCCurrentPalette(HDC16 hDC)
{
    DC* dc = (DC *)GDI_GetObjPtr( hDC, DC_MAGIC );
    if (dc) 
    {
      GDI_HEAP_UNLOCK( hDC );
      return dc->w.hPalette == hPrimaryPalette;
    }
    return FALSE;
}


/***********************************************************************
 *           SelectPalette16    (USER.282)
 */
HPALETTE16 WINAPI SelectPalette16( HDC16 hDC, HPALETTE16 hPal,
                                   BOOL16 bForceBackground )
{
    return SelectPalette32( hDC, hPal, bForceBackground );
}


/***********************************************************************
 * SelectPalette32 [GDI32.300]  Selects logical palette into DC
 *
 * RETURNS
 *    Success: Previous logical palette
 *    Failure: NULL
 */
HPALETTE32 WINAPI SelectPalette32(
    HDC32 hDC,               /* [in] Handle of device context */
    HPALETTE32 hPal,         /* [in] Handle of logical color palette */
    BOOL32 bForceBackground) /* [in] Foreground/background mode */
{
    WORD	wBkgPalette = 1;
    PALETTEOBJ* lpt = (PALETTEOBJ*) GDI_GetObjPtr( hPal, PALETTE_MAGIC );

    TRACE(palette,"dc=%04x,pal=%04x,force=%i\n", hDC, hPal, bForceBackground);
    if( !lpt ) return 0;

    TRACE(palette," entries = %d\n", lpt->logpalette.palNumEntries);
    GDI_HEAP_UNLOCK( hPal );

    if( hPal != STOCK_DEFAULT_PALETTE )
    {
	HWND32 hWnd = WindowFromDC32( hDC );
	HWND32 hActive = GetActiveWindow32();
	
	/* set primary palette if it's related to current active */

	if((!hWnd || (hActive == hWnd || IsChild16(hActive,hWnd))) &&
            !bForceBackground )
	    wBkgPalette = 0;
    }
    return GDISelectPalette( hDC, hPal, wBkgPalette);
}


/***********************************************************************
 *           RealizePalette16    (USER.283)
 */
UINT16 WINAPI RealizePalette16( HDC16 hDC )
{
    return RealizePalette32( hDC );
}


/***********************************************************************
 * RealizePalette32 [GDI32.280]  Maps palette entries to system palette
 *
 * RETURNS
 *    Success: Number of entries in logical palette
 *    Failure: GDI_ERROR
 */
UINT32 WINAPI RealizePalette32(
    HDC32 hDC) /* [in] Handle of device context */
{
    UINT32 realized = GDIRealizePalette( hDC );

    /* do not send anything if no colors were changed */

    if( IsDCCurrentPalette( hDC ) && realized && 
        !(COLOR_GetSystemPaletteFlags() & COLOR_VIRTUAL) )
    {
	/* Send palette change notification */

	HWND32 hWnd;
 	if( (hWnd = WindowFromDC32( hDC )) )
            SendMessage16( HWND_BROADCAST, WM_PALETTECHANGED, hWnd, 0L);
    }
    return realized;
}


/**********************************************************************
 *            UpdateColors16   (GDI.366)
 */
INT16 WINAPI UpdateColors16( HDC16 hDC )
{
    HWND32 hWnd = WindowFromDC32( hDC );

    /* Docs say that we have to remap current drawable pixel by pixel
     * but it would take forever given the speed of XGet/PutPixel.
     */
    if (hWnd && !(COLOR_GetSystemPaletteFlags() & COLOR_VIRTUAL) ) 
	InvalidateRect32( hWnd, NULL, FALSE );
    return 0x666;
}


/**********************************************************************
 * UpdateColors32 [GDI32.359]  Remaps current colors to logical palette
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL32 WINAPI UpdateColors32(
    HDC32 hDC) /* [in] Handle of device context */
{
    UpdateColors16( hDC );
    return TRUE;
}

