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

DEFAULT_DEBUG_CHANNEL(palette);

PALETTE_DRIVER *PALETTE_Driver = NULL;

/* Pointers to USER implementation of SelectPalette/RealizePalette */
/* they will be patched by USER on startup */
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
            ERR("Can not create palette mapping -- out of memory!\n");
        GDI_ReleaseObj( hpalette );
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
 *           CreatePalette    (GDI.360)
 */
HPALETTE16 WINAPI CreatePalette16( const LOGPALETTE* palette )
{
    return CreatePalette( palette );
}


/***********************************************************************
 * CreatePalette [GDI32.@]  Creates a logical color palette
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
                                        PALETTE_MAGIC, &hpalette ))) return 0;
    memcpy( &palettePtr->logpalette, palette, size );
    PALETTE_ValidateFlags(palettePtr->logpalette.palPalEntry, 
			  palettePtr->logpalette.palNumEntries);
    palettePtr->mapping = NULL;
    GDI_ReleaseObj( hpalette );

    TRACE("   returning %04x\n", hpalette);
    return hpalette;
}


/***********************************************************************
 * CreateHalftonePalette [GDI.529]  Creates a halftone palette
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
 * CreateHalftonePalette [GDI32.@]  Creates a halftone palette
 *
 * RETURNS
 *    Success: Handle to logical halftone palette
 *    Failure: 0
 *
 * FIXME: This simply creates the halftone palette dirived from runing
 *        tests on an windows NT machine. this is assuming a color depth
 *        of greater that 256 color. On a 256 color device the halftone
 *        palette will be differnt and this funtion will be incorrect
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
 *           GetPaletteEntries    (GDI.363)
 */
UINT16 WINAPI GetPaletteEntries16( HPALETTE16 hpalette, UINT16 start,
                                   UINT16 count, LPPALETTEENTRY entries )
{
    return GetPaletteEntries( hpalette, start, count, entries );
}


/***********************************************************************
 * GetPaletteEntries [GDI32.@]  Retrieves palette entries
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

    TRACE("hpal = %04x, count=%i\n", hpalette, count );
        
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
 *           SetPaletteEntries    (GDI.364)
 */
UINT16 WINAPI SetPaletteEntries16( HPALETTE16 hpalette, UINT16 start,
                                   UINT16 count, LPPALETTEENTRY entries )
{
    return SetPaletteEntries( hpalette, start, count, entries );
}


/***********************************************************************
 * SetPaletteEntries [GDI32.@]  Sets color values for range in palette
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
    UINT numEntries;

    TRACE("hpal=%04x,start=%i,count=%i\n",hpalette,start,count );

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
    PALETTE_ValidateFlags(palPtr->logpalette.palPalEntry, 
			  palPtr->logpalette.palNumEntries);
    HeapFree( GetProcessHeap(), 0, palPtr->mapping );
    palPtr->mapping = NULL;
    GDI_ReleaseObj( hpalette );
    return count;
}


/***********************************************************************
 *           ResizePalette   (GDI.368)
 */
BOOL16 WINAPI ResizePalette16( HPALETTE16 hPal, UINT16 cEntries )
{
    return ResizePalette( hPal, cEntries );
}


/***********************************************************************
 * ResizePalette [GDI32.@]  Resizes logical palette
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
    
    if (!(palPtr = GDI_ReallocObject( size, hPal, palPtr ))) return FALSE;

    if( mapping ) 
    {
        int *newMap = (int*) HeapReAlloc(GetProcessHeap(), 0, 
                                    mapping, cEntries * sizeof(int) );
	if(newMap == NULL) 
        {
            ERR("Can not resize mapping -- out of memory!\n");
            GDI_ReleaseObj( hPal );
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
    GDI_ReleaseObj( hPal );
    return TRUE;
}


/***********************************************************************
 *           AnimatePalette   (GDI.367)
 */
void WINAPI AnimatePalette16( HPALETTE16 hPal, UINT16 StartIndex,
                              UINT16 NumEntries, const PALETTEENTRY* PaletteColors)
{
    AnimatePalette( hPal, StartIndex, NumEntries, PaletteColors );
}


/***********************************************************************
 * AnimatePalette [GDI32.@]  Replaces entries in logical palette
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

    if( hPal != GetStockObject(DEFAULT_PALETTE) )
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
            GDI_ReleaseObj( hPal );
	    return TRUE;
	}
	GDI_ReleaseObj( hPal );
    }
    return FALSE;
}


/***********************************************************************
 *           SetSystemPaletteUse   (GDI.373)
 */
UINT16 WINAPI SetSystemPaletteUse16( HDC16 hdc, UINT16 use )
{
    return SetSystemPaletteUse( hdc, use );
}


/***********************************************************************
 * SetSystemPaletteUse [GDI32.@]
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
 *           GetSystemPaletteUse   (GDI.374)
 */
UINT16 WINAPI GetSystemPaletteUse16( HDC16 hdc )
{
    return SystemPaletteUse;
}


/***********************************************************************
 * GetSystemPaletteUse [GDI32.@]  Gets state of system palette
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
 *           GetSystemPaletteEntries   (GDI.375)
 */
UINT16 WINAPI GetSystemPaletteEntries16( HDC16 hdc, UINT16 start, UINT16 count,
                                         LPPALETTEENTRY entries )
{
    return GetSystemPaletteEntries( hdc, start, count, entries );
}


/***********************************************************************
 * GetSystemPaletteEntries [GDI32.@]  Gets range of palette entries
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
    INT sizePalette = GetDeviceCaps( hdc, SIZEPALETTE );

    TRACE("hdc=%04x,start=%i,count=%i\n", hdc,start,count);

    if (!entries) return sizePalette;
    if (start >= sizePalette) return 0;
    if (start+count >= sizePalette) count = sizePalette - start;

    for (i = 0; i < count; i++)
    {
	*(COLORREF*)(entries + i) = COLOR_GetSystemPaletteEntry( start + i );

        TRACE("\tidx(%02x) -> RGB(%08lx)\n",
                         start + i, *(COLORREF*)(entries + i) );
    }
    return count;
}


/***********************************************************************
 *           GetNearestPaletteIndex   (GDI.370)
 */
UINT16 WINAPI GetNearestPaletteIndex16( HPALETTE16 hpalette, COLORREF color )
{
    return GetNearestPaletteIndex( hpalette, color );
}


/***********************************************************************
 * GetNearestPaletteIndex [GDI32.@]  Gets palette index for color
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
      index = COLOR_PaletteLookupPixel(palObj->logpalette.palPalEntry, 
				       palObj->logpalette.palNumEntries,
				       NULL, color, FALSE );

      GDI_ReleaseObj( hpalette );
    }
    TRACE("(%04x,%06lx): returning %d\n", hpalette, color, index );
    return index;
}


/***********************************************************************
 *           GetNearestColor   (GDI.154)
 */
COLORREF WINAPI GetNearestColor16( HDC16 hdc, COLORREF color )
{
    return GetNearestColor( hdc, color );
}


/***********************************************************************
 * GetNearestColor [GDI32.@]  Gets a system color to match
 *
 * RETURNS
 *    Success: Color from system palette that corresponds to given color
 *    Failure: CLR_INVALID
 */
COLORREF WINAPI GetNearestColor(
    HDC hdc,      /* [in] Handle of device context */
    COLORREF color) /* [in] Color to be matched */
{
    COLORREF 	 nearest = CLR_INVALID;
    DC 		*dc;
    PALETTEOBJ  *palObj;

    if(!(GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)) {
        return color;
    }
    if ( (dc = DC_GetDCPtr( hdc )) )
    {
        HPALETTE hpal = (dc->hPalette)? dc->hPalette : GetStockObject( DEFAULT_PALETTE );
        palObj = GDI_GetObjPtr( hpal, PALETTE_MAGIC );
        if (!palObj) {
            GDI_ReleaseObj( hdc );
            return nearest;
        }

        nearest = COLOR_LookupNearestColor( palObj->logpalette.palPalEntry,
                                            palObj->logpalette.palNumEntries, color );
        GDI_ReleaseObj( hpal );
        GDI_ReleaseObj( hdc );
    }

    TRACE("(%06lx): returning %06lx\n", color, nearest );
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
        HeapFree( GetProcessHeap(), 0, palette->mapping );
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
    HeapFree( GetProcessHeap(), 0, palette->mapping );
    if (hLastRealizedPalette == hpalette) hLastRealizedPalette = 0;
    return GDI_FreeObject( hpalette, palette );
}


/***********************************************************************
 *           GDISelectPalette    (GDI.361)
 */
HPALETTE16 WINAPI GDISelectPalette16( HDC16 hdc, HPALETTE16 hpal, WORD wBkg)
{
    HPALETTE16 prev;
    DC *dc;

    TRACE("%04x %04x\n", hdc, hpal );

    if (GetObjectType(hpal) != OBJ_PAL)
    {
      WARN("invalid selected palette %04x\n",hpal);
      return 0;
    }
    if (!(dc = DC_GetDCPtr( hdc ))) return 0;
    prev = dc->hPalette;
    dc->hPalette = hpal;
    GDI_ReleaseObj( hdc );
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
    DC* dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;

    TRACE("%04x...\n", hdc );
    
    if(dc->hPalette != hLastRealizedPalette )
    {
	if( dc->hPalette == GetStockObject( DEFAULT_PALETTE )) {
            realized = RealizeDefaultPalette16( hdc );
	    GDI_ReleaseObj( hdc );
	    return (UINT16)realized;
	}


        palPtr = (PALETTEOBJ *) GDI_GetObjPtr( dc->hPalette, PALETTE_MAGIC );

	if (!palPtr) {
	    GDI_ReleaseObj( hdc );
            FIXME("invalid selected palette %04x\n",dc->hPalette);
            return 0;
	}
        
        realized = PALETTE_Driver->
	  pSetMapping(palPtr,0,palPtr->logpalette.palNumEntries,
		      (dc->hPalette != hPrimaryPalette) ||
		      (dc->hPalette == GetStockObject( DEFAULT_PALETTE )));
	hLastRealizedPalette = dc->hPalette;
	GDI_ReleaseObj( dc->hPalette );
    }
    else TRACE("  skipping (hLastRealizedPalette = %04x)\n",
			 hLastRealizedPalette);
    GDI_ReleaseObj( hdc );

    TRACE("   realized %i colors.\n", realized );
    return (UINT16)realized;
}


/***********************************************************************
 *           RealizeDefaultPalette    (GDI.365)
 */
UINT16 WINAPI RealizeDefaultPalette16( HDC16 hdc )
{
    UINT16 ret = 0;
    DC          *dc;
    PALETTEOBJ*  palPtr;

    TRACE("%04x\n", hdc );

    if (!(dc = DC_GetDCPtr( hdc ))) return 0;

    if (!(dc->flags & DC_MEMORY))
    {
        palPtr = (PALETTEOBJ*)GDI_GetObjPtr( GetStockObject(DEFAULT_PALETTE), PALETTE_MAGIC );
        if (palPtr)
        {
            /* lookup is needed to account for SetSystemPaletteUse() stuff */
            ret = PALETTE_Driver->pUpdateMapping(palPtr);
            GDI_ReleaseObj( GetStockObject(DEFAULT_PALETTE) );
        }
    }
    GDI_ReleaseObj( hdc );
    return ret;
}

/***********************************************************************
 *           IsDCCurrentPalette   (GDI.412)
 */
BOOL16 WINAPI IsDCCurrentPalette16(HDC16 hDC)
{
    DC *dc = DC_GetDCPtr( hDC );
    if (dc) 
    {
      BOOL bRet = dc->hPalette == hPrimaryPalette;
      GDI_ReleaseObj( hDC );
      return bRet;
    }
    return FALSE;
}


/***********************************************************************
 * SelectPalette [GDI32.@]  Selects logical palette into DC
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
 * RealizePalette [GDI32.@]  Maps palette entries to system palette
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
 * UpdateColors [GDI32.@]  Remaps current colors to logical palette
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
                RedrawWindow_funcptr pRedrawWindow = GetProcAddress( mod, "RedrawWindow" );
                if (pRedrawWindow) pRedrawWindow( hWnd, NULL, 0, RDW_INVALIDATE );
            }
        }
    }
    return 0x666;
}


/**********************************************************************
 *            UpdateColors   (GDI.366)
 */
INT16 WINAPI UpdateColors16( HDC16 hDC )
{
    UpdateColors( hDC );
    return TRUE;
}


/*********************************************************************
 *           SetMagicColors   (GDI.606)
 */
VOID WINAPI SetMagicColors16(HDC16 hDC, COLORREF color, UINT16 index)
{
    FIXME("(hDC %04x, color %04x, index %04x): stub\n", hDC, (int)color, index);

}

/**********************************************************************
 * GetICMProfileA [GDI32.@]
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

/*********************************************************************/

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
    strcpy(lpszFilename, WINEICM);
    return TRUE;
}
