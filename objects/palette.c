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

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "gdi.h"
#include "color.h"
#include "palette.h"
#include "debugtools.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(palette)

PALETTE_DRIVER *PALETTE_Driver = NULL;

FARPROC pfnSelectPalette = NULL;
FARPROC pfnRealizePalette = NULL;

static UINT SystemPaletteUse = SYSPAL_STATIC;  /* currently not considered */

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
    HeapFree( GetProcessHeap(), 0, palPtr );

    palObj = (PALETTEOBJ*) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (palObj)
    {
        if (!(palObj->mapping = HeapAlloc( GetProcessHeap(), 0, sizeof(int) * 20 )))
            ERR("Can not create palette mapping -- out of memory!");
        GDI_HEAP_UNLOCK( hpalette );
    }
    	
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
    return CreatePalette( palette );
}


/***********************************************************************
 * CreatePalette32 [GDI32.53]  Creates a logical color palette
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

    hpalette = GDI_AllocObject( size + sizeof(int*) +sizeof(GDIOBJHDR) , PALETTE_MAGIC );
    if (!hpalette) return 0;

    palettePtr = (PALETTEOBJ *) GDI_HEAP_LOCK( hpalette );
    memcpy( &palettePtr->logpalette, palette, size );
    PALETTE_ValidateFlags(palettePtr->logpalette.palPalEntry, 
			  palettePtr->logpalette.palNumEntries);
    palettePtr->mapping = NULL;
    GDI_HEAP_UNLOCK( hpalette );

    TRACE("   returning %04x\n", hpalette);
    return hpalette;
}


/***********************************************************************
 * CreateHalftonePalette16 [GDI.?]  Creates a halftone palette
 *
 * RETURNS
 *    Success: Handle to logical halftone palette
 *    Failure: 0
 */
HPALETTE16 WINAPI CreateHalftonePalette16(
    HDC16 hdc) /* [in] Handle to device context */
{
    return CreateHalftonePalette(hdc);
}

	
/***********************************************************************
 * CreateHalftonePalette32 [GDI32.47]  Creates a halftone palette
 *
 * RETURNS
 *    Success: Handle to logical halftone palette
 *    Failure: 0
 *
 * FIXME: not truly tested
 */
HPALETTE WINAPI CreateHalftonePalette(
    HDC hdc) /* [in] Handle to device context */
{
    int i, r, g, b;
    struct {
	WORD Version;
	WORD NumberOfEntries;
	PALETTEENTRY aEntries[256];
    } Palette = {
	0x300, 256
    };

    GetSystemPaletteEntries(hdc, 0, 256, Palette.aEntries);
    return CreatePalette((LOGPALETTE *)&Palette);

    for (r = 0; r < 6; r++) {
	for (g = 0; g < 6; g++) {
	    for (b = 0; b < 6; b++) {
		i = r + g*6 + b*36 + 10;
		Palette.aEntries[i].peRed = r * 51;
		Palette.aEntries[i].peGreen = g * 51;
		Palette.aEntries[i].peBlue = b * 51;
	    }    
	  }
	}
	
    for (i = 216; i < 246; i++) {
	int v = (i - 216) * 8;
	Palette.aEntries[i].peRed = v;
	Palette.aEntries[i].peGreen = v;
	Palette.aEntries[i].peBlue = v;
	}
	
    return CreatePalette((LOGPALETTE *)&Palette);
}


/***********************************************************************
 *           GetPaletteEntries16    (GDI.363)
 */
UINT16 WINAPI GetPaletteEntries16( HPALETTE16 hpalette, UINT16 start,
                                   UINT16 count, LPPALETTEENTRY entries )
{
    return GetPaletteEntries( hpalette, start, count, entries );
}


/***********************************************************************
 * GetPaletteEntries32 [GDI32.209]  Retrieves palette entries
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
    INT numEntries;

    TRACE("hpal = %04x, count=%i\n", hpalette, count );
        
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;

    numEntries = palPtr->logpalette.palNumEntries;
    if (start+count > numEntries) count = numEntries - start;
    if (entries)
    { 
      if (start >= numEntries) 
      {
	GDI_HEAP_UNLOCK( hpalette );
	return 0;
      }
      memcpy( entries, &palPtr->logpalette.palPalEntry[start],
	      count * sizeof(PALETTEENTRY) );
      for( numEntries = 0; numEntries < count ; numEntries++ )
	   if (entries[numEntries].peFlags & 0xF0)
	       entries[numEntries].peFlags = 0;
      GDI_HEAP_UNLOCK( hpalette );
    }

    return count;
}


/***********************************************************************
 *           SetPaletteEntries16    (GDI.364)
 */
UINT16 WINAPI SetPaletteEntries16( HPALETTE16 hpalette, UINT16 start,
                                   UINT16 count, LPPALETTEENTRY entries )
{
    return SetPaletteEntries( hpalette, start, count, entries );
}


/***********************************************************************
 * SetPaletteEntries32 [GDI32.326]  Sets color values for range in palette
 *
 * RETURNS
 *    Success: Number of entries that were set
 *    Failure: 0
 */
UINT WINAPI SetPaletteEntries(
    HPALETTE hpalette,    /* [in] Handle of logical palette */
    UINT start,           /* [in] Index of first entry to set */
    UINT count,           /* [in] Number of entries to set */
    LPPALETTEENTRY entries) /* [in] Address of array of structures */
{
    PALETTEOBJ * palPtr;
    INT numEntries;

    TRACE("hpal=%04x,start=%i,count=%i\n",hpalette,start,count );

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
    return ResizePalette( hPal, cEntries );
}


/***********************************************************************
 * ResizePalette32 [GDI32.289]  Resizes logical palette
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
    int*	 mapping = NULL;

    TRACE("hpal = %04x, prev = %i, new = %i\n",
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
    {
        int *newMap = (int*) HeapReAlloc(GetProcessHeap(), 0, 
                                    mapping, cEntries * sizeof(int) );
	if(newMap == NULL) 
        {
            ERR("Can not resize mapping -- out of memory!");
            GDI_HEAP_UNLOCK( hPal );
            return FALSE;
        }
        palPtr->mapping = newMap;
    }

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
                              UINT16 NumEntries, const PALETTEENTRY* PaletteColors)
{
    AnimatePalette( hPal, StartIndex, NumEntries, PaletteColors );
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
BOOL WINAPI AnimatePalette(
    HPALETTE hPal,              /* [in] Handle to logical palette */
    UINT StartIndex,            /* [in] First entry in palette */
    UINT NumEntries,            /* [in] Count of entries in palette */
    const PALETTEENTRY* PaletteColors) /* [in] Pointer to first replacement */
{
    TRACE("%04x (%i - %i)\n", hPal, StartIndex,StartIndex+NumEntries);

    if( hPal != STOCK_DEFAULT_PALETTE ) 
    {
        PALETTEOBJ* palPtr = (PALETTEOBJ *)GDI_GetObjPtr(hPal, PALETTE_MAGIC);
        if (!palPtr) return FALSE;

	if( (StartIndex + NumEntries) <= palPtr->logpalette.palNumEntries )
	{
	    UINT u;
	    for( u = 0; u < NumEntries; u++ )
		palPtr->logpalette.palPalEntry[u + StartIndex] = PaletteColors[u];
	    PALETTE_Driver->
	      pSetMapping(palPtr, StartIndex, NumEntries,
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
    return SetSystemPaletteUse( hdc, use );
}


/***********************************************************************
 * SetSystemPaletteUse32 [GDI32.335]
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
    FIXME("(%04x,%04x): stub\n", hdc, use );
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
UINT WINAPI GetSystemPaletteUse(
    HDC hdc) /* [in] Handle of device context */
{
    return SystemPaletteUse;
}


/***********************************************************************
 *           GetSystemPaletteEntries16   (GDI.375)
 */
UINT16 WINAPI GetSystemPaletteEntries16( HDC16 hdc, UINT16 start, UINT16 count,
                                         LPPALETTEENTRY entries )
{
    return GetSystemPaletteEntries( hdc, start, count, entries );
}


/***********************************************************************
 * GetSystemPaletteEntries32 [GDI32.222]  Gets range of palette entries
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
    UINT i;
    DC *dc;

    TRACE("hdc=%04x,start=%i,count=%i\n", hdc,start,count);

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (!entries) return dc->w.devCaps->sizePalette;
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

        TRACE("\tidx(%02x) -> RGB(%08lx)\n",
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
    return GetNearestPaletteIndex( hpalette, color );
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
UINT WINAPI GetNearestPaletteIndex(
    HPALETTE hpalette, /* [in] Handle of logical color palette */
    COLORREF color)      /* [in] Color to be matched */
{
    PALETTEOBJ*	palObj = (PALETTEOBJ*)GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    UINT index  = 0;

    if( palObj )
      index = COLOR_PaletteLookupPixel(palObj->logpalette.palPalEntry, 
				       palObj->logpalette.palNumEntries,
				       NULL, color, FALSE );

    TRACE("(%04x,%06lx): returning %d\n", hpalette, color, index );
    GDI_HEAP_UNLOCK( hpalette );
    return index;
}


/***********************************************************************
 *           GetNearestColor16   (GDI.154)
 */
COLORREF WINAPI GetNearestColor16( HDC16 hdc, COLORREF color )
{
    return GetNearestColor( hdc, color );
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
COLORREF WINAPI GetNearestColor(
    HDC hdc,      /* [in] Handle of device context */
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
      if (!palObj) return nearest;

      nearest = COLOR_LookupNearestColor( palObj->logpalette.palPalEntry,
					  palObj->logpalette.palNumEntries, color );
      GDI_HEAP_UNLOCK( dc->w.hPalette );
    }

    TRACE("(%06lx): returning %06lx\n", color, nearest );
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
BOOL PALETTE_UnrealizeObject( HPALETTE16 hpalette, PALETTEOBJ *palette )
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
BOOL PALETTE_DeleteObject( HPALETTE16 hpalette, PALETTEOBJ *palette )
{
    free( palette->mapping );
    if (hLastRealizedPalette == hpalette) hLastRealizedPalette = 0;
    return GDI_FreeObject( hpalette );
}


/***********************************************************************
 *           GDISelectPalette    (GDI.361)
 */
HPALETTE16 WINAPI GDISelectPalette16( HDC16 hdc, HPALETTE16 hpal, WORD wBkg)
{
    HPALETTE16 prev;
    DC *dc;

    TRACE("%04x %04x\n", hdc, hpal );
    
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
UINT16 WINAPI GDIRealizePalette16( HDC16 hdc )
{
    PALETTEOBJ* palPtr;
    int realized = 0;
    DC*		dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    TRACE("%04x...\n", hdc );
    
    if( dc &&  dc->w.hPalette != hLastRealizedPalette )
    {
	if( dc->w.hPalette == STOCK_DEFAULT_PALETTE )
            return RealizeDefaultPalette16( hdc );

        palPtr = (PALETTEOBJ *) GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC );

	if (!palPtr) {
		FIXME("invalid selected palette %04x\n",dc->w.hPalette);
		return 0;
	}
        
        realized = PALETTE_Driver->
	  pSetMapping(palPtr,0,palPtr->logpalette.palNumEntries,
		      (dc->w.hPalette != hPrimaryPalette) ||
		      (dc->w.hPalette == STOCK_DEFAULT_PALETTE));
	GDI_HEAP_UNLOCK( dc->w.hPalette );
	hLastRealizedPalette = dc->w.hPalette;
    }
    else TRACE("  skipping (hLastRealizedPalette = %04x)\n",
			 hLastRealizedPalette);
    GDI_HEAP_UNLOCK( hdc );

    TRACE("   realized %i colors.\n", realized );
    return (UINT16)realized;
}


/***********************************************************************
 *           RealizeDefaultPalette    (GDI.365)
 */
UINT16 WINAPI RealizeDefaultPalette16( HDC16 hdc )
{
    DC          *dc;
    PALETTEOBJ*  palPtr;

    TRACE("%04x\n", hdc );

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
    if (!palPtr) return 0;

    /* lookup is needed to account for SetSystemPaletteUse() stuff */

    return PALETTE_Driver->pUpdateMapping(palPtr);
}

/***********************************************************************
 *           IsDCCurrentPalette   (GDI.412)
 */
BOOL16 WINAPI IsDCCurrentPalette16(HDC16 hDC)
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
    return SelectPalette( hDC, hPal, bForceBackground );
}


/***********************************************************************
 * SelectPalette32 [GDI32.300]  Selects logical palette into DC
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
    WORD	wBkgPalette = 1;
    PALETTEOBJ* lpt = (PALETTEOBJ*) GDI_GetObjPtr( hPal, PALETTE_MAGIC );

    TRACE("dc=%04x,pal=%04x,force=%i\n", hDC, hPal, bForceBackground);
    if( !lpt ) return 0;

    TRACE(" entries = %d\n", lpt->logpalette.palNumEntries);
    GDI_HEAP_UNLOCK( hPal );

    if( hPal != STOCK_DEFAULT_PALETTE )
    {
	HWND hWnd = WindowFromDC( hDC );
	HWND hActive = GetActiveWindow();
	
	/* set primary palette if it's related to current active */

	if((!hWnd || (hActive == hWnd || IsChild16(hActive,hWnd))) &&
            !bForceBackground )
	    wBkgPalette = 0;
    }
    return GDISelectPalette16( hDC, hPal, wBkgPalette);
}


/***********************************************************************
 *           RealizePalette16    (USER.283)
 */
UINT16 WINAPI RealizePalette16( HDC16 hDC )
{
    return RealizePalette( hDC );
}


/***********************************************************************
 * RealizePalette32 [GDI32.280]  Maps palette entries to system palette
 *
 * RETURNS
 *    Success: Number of entries in logical palette
 *    Failure: GDI_ERROR
 */
UINT WINAPI RealizePalette(
    HDC hDC) /* [in] Handle of device context */
{
    DC *dc;
    UINT realized;

    if (!(dc = (DC *) GDI_GetObjPtr( hDC, DC_MAGIC ))) return 0;
    
    realized = GDIRealizePalette16( hDC );

    /* do not send anything if no colors were changed */

    if( IsDCCurrentPalette16( hDC ) && realized &&
	dc->w.devCaps->sizePalette )
    {
	/* Send palette change notification */

	HWND hWnd;
 	if( (hWnd = WindowFromDC( hDC )) )
            SendMessage16( HWND_BROADCAST, WM_PALETTECHANGED, hWnd, 0L);
    }

    GDI_HEAP_UNLOCK( hDC );
    return realized;
}


/**********************************************************************
 *            UpdateColors16   (GDI.366)
 */
INT16 WINAPI UpdateColors16( HDC16 hDC )
{
    DC *dc;
    HWND hWnd;

    if (!(dc = (DC *) GDI_GetObjPtr( hDC, DC_MAGIC ))) return 0;

    hWnd = WindowFromDC( hDC );

    /* Docs say that we have to remap current drawable pixel by pixel
     * but it would take forever given the speed of XGet/PutPixel.
     */
    if (hWnd && dc->w.devCaps->sizePalette ) 
	InvalidateRect( hWnd, NULL, FALSE );

    GDI_HEAP_UNLOCK( hDC );

    return 0x666;
}


/**********************************************************************
 * UpdateColors32 [GDI32.359]  Remaps current colors to logical palette
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI UpdateColors(
    HDC hDC) /* [in] Handle of device context */
{
    UpdateColors16( hDC );
    return TRUE;
}


/*********************************************************************
 *           SetMagicColors16   (GDI.606)
 */
VOID WINAPI SetMagicColors16(HDC16 hDC, COLORREF color, UINT16 index)
{
    FIXME("(hDC %04x, color %04x, index %04x): stub\n", hDC, (int)color, index);

}

/**********************************************************************
 * GetICMProfileA [GDI32.316]
 *
 * Returns the filename of the specified device context's color
 * management profile, even if color management is not enabled
 * for that DC.
 *
 * RETURNS
 *    TRUE if name copied succesfully OR lpszFilename is NULL
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

#define WINEICM "winefake.icm"  /* easy-to-identify fake filename */

BOOL WINAPI GetICMProfileA(HDC hDC, LPDWORD lpcbName, LPSTR lpszFilename)
{
    DWORD callerLen;

    FIXME("(%04x, %p, %p): partial stub\n", hDC, lpcbName, lpszFilename);

    callerLen = *lpcbName;

    /* all 3 behaviors require the required buffer size to be set */
    *lpcbName = strlen(WINEICM);

    /* behavior 1: if lpszFilename is NULL, return size of string and no error */
    if ((DWORD)lpszFilename == (DWORD)0x00000000)
 	return TRUE;
    
    /* behavior 2: if buffer size too small, return size of string and error */
    if (callerLen < strlen(WINEICM)) 
    {
	SetLastError(ERROR_INSUFFICIENT_BUFFER);
	return FALSE;
    }

    /* behavior 3: if buffer size OK and pointer not NULL, copy and return size */
    lstrcpyA(lpszFilename, WINEICM);
    return TRUE;
}
