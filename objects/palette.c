/*
 * GDI palette objects
 *
 * Copyright 1993,1994 Alexandre Julliard
 * Copyright 1996 Alex Korobka
 *
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#include "gdi.h"
#include "color.h"
#include "palette.h"
#include "xmalloc.h"
#include "stddebug.h"
/* #define DEBUG_PALETTE */
#include "debug.h"

 /* lookup pixel among static entries of the system palette */
extern int COLOR_LookupSystemPixel(COLORREF);

static UINT32 SystemPaletteUse = SYSPAL_STATIC;  /* currently not considered */

static HPALETTE16 hPrimaryPalette = 0; /* used for WM_PALETTECHANGED */
static HPALETTE16 hLastRealizedPalette = 0; /* UnrealizeObject() needs it */


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
HPALETTE16 CreatePalette16( const LOGPALETTE* palette )
{
    return CreatePalette32( palette );
}


/***********************************************************************
 *           CreatePalette32    (GDI32.53)
 */
HPALETTE32 CreatePalette32( const LOGPALETTE* palette )
{
    PALETTEOBJ * palettePtr;
    HPALETTE32 hpalette;
    int size = sizeof(LOGPALETTE) + (palette->palNumEntries - 1) * sizeof(PALETTEENTRY);

    dprintf_palette(stddeb,"CreatePalette: %i entries, ",
                    palette->palNumEntries);

    hpalette = GDI_AllocObject( size + sizeof(int*) +sizeof(GDIOBJHDR) , PALETTE_MAGIC );
    if (!hpalette) return 0;

    palettePtr = (PALETTEOBJ *) GDI_HEAP_LIN_ADDR( hpalette );
    memcpy( &palettePtr->logpalette, palette, size );
    PALETTE_ValidateFlags(palettePtr->logpalette.palPalEntry, 
			  palettePtr->logpalette.palNumEntries);
    palettePtr->mapping = NULL;

    dprintf_palette(stddeb,"returning %04x\n", hpalette);
    return hpalette;
}


/***********************************************************************
 *           GetPaletteEntries16    (GDI.363)
 */
UINT16 GetPaletteEntries16( HPALETTE16 hpalette, UINT16 start, UINT16 count,
                            LPPALETTEENTRY entries )
{
    return GetPaletteEntries32( hpalette, start, count, entries );
}


/***********************************************************************
 *           GetPaletteEntries32    (GDI32.209)
 */
UINT32 GetPaletteEntries32( HPALETTE32 hpalette, UINT32 start, UINT32 count,
                            LPPALETTEENTRY entries )
{
    PALETTEOBJ * palPtr;
    INT32 numEntries;

    dprintf_palette( stddeb,"GetPaletteEntries: hpal = %04x, %i entries\n",
                     hpalette, count );
        
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;

    numEntries = palPtr->logpalette.palNumEntries;
    if (start >= numEntries) return 0;
    if (start+count > numEntries) count = numEntries - start;
    memcpy( entries, &palPtr->logpalette.palPalEntry[start],
	    count * sizeof(PALETTEENTRY) );
    for( numEntries = 0; numEntries < count ; numEntries++ )
         if (entries[numEntries].peFlags & 0xF0)
             entries[numEntries].peFlags = 0;
    return count;
}


/***********************************************************************
 *           SetPaletteEntries16    (GDI.364)
 */
UINT16 SetPaletteEntries16( HPALETTE16 hpalette, UINT16 start, UINT16 count,
                            LPPALETTEENTRY entries )
{
    return SetPaletteEntries32( hpalette, start, count, entries );
}


/***********************************************************************
 *           SetPaletteEntries32    (GDI32.326)
 */
UINT32 SetPaletteEntries32( HPALETTE32 hpalette, UINT32 start, UINT32 count,
                            LPPALETTEENTRY entries )
{
    PALETTEOBJ * palPtr;
    INT32 numEntries;

    dprintf_palette( stddeb,"SetPaletteEntries: hpal = %04x, %i entries\n",
                     hpalette, count );

    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;

    numEntries = palPtr->logpalette.palNumEntries;
    if (start >= numEntries) return 0;
    if (start+count > numEntries) count = numEntries - start;
    memcpy( &palPtr->logpalette.palPalEntry[start], entries,
	    count * sizeof(PALETTEENTRY) );
    PALETTE_ValidateFlags(palPtr->logpalette.palPalEntry, 
			  palPtr->logpalette.palNumEntries);
    free(palPtr->mapping);
    palPtr->mapping = NULL;

    return count;
}


/***********************************************************************
 *           ResizePalette16   (GDI.368)
 */
BOOL16 ResizePalette16( HPALETTE16 hPal, UINT16 cEntries )
{
    return ResizePalette32( hPal, cEntries );
}


/***********************************************************************
 *           ResizePalette32   (GDI32.289)
 */
BOOL32 ResizePalette32( HPALETTE32 hPal, UINT32 cEntries )
{
    PALETTEOBJ * palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hPal, PALETTE_MAGIC );
    UINT32	 cPrevEnt, prevVer;
    int		 prevsize, size = sizeof(LOGPALETTE) + (cEntries - 1) * sizeof(PALETTEENTRY);
    int*	 mapping = NULL;

    dprintf_palette(stddeb,"ResizePalette: hpal = %04x, prev = %i, new = %i\n",
		    hPal, (palPtr)? -1: palPtr->logpalette.palNumEntries, cEntries );
    if( !palPtr ) return FALSE;
    cPrevEnt = palPtr->logpalette.palNumEntries;
    prevVer = palPtr->logpalette.palVersion;
    prevsize = sizeof(LOGPALETTE) + (cPrevEnt - 1) * sizeof(PALETTEENTRY) +
	      				sizeof(int*) + sizeof(GDIOBJHDR);
    size += sizeof(int*) + sizeof(GDIOBJHDR);
    mapping = palPtr->mapping;

    GDI_HEAP_REALLOC( hPal, size );
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
    return TRUE;
}


/***********************************************************************
 *           AnimatePalette16   (GDI.367)
 */
BOOL16 AnimatePalette16( HPALETTE16 hPal, UINT16 StartIndex, UINT16 NumEntries,
                         LPPALETTEENTRY PaletteColors )
{
    return AnimatePalette32( hPal, StartIndex, NumEntries, PaletteColors );
}


/***********************************************************************
 *           AnimatePalette32   (GDI32.6)
 */
BOOL32 AnimatePalette32( HPALETTE32 hPal, UINT32 StartIndex, UINT32 NumEntries,
                         LPPALETTEENTRY PaletteColors )
{
    fprintf(stdnimp,"AnimatePalette: empty stub! \n");
    return TRUE;
}


/***********************************************************************
 *           SetSystemPaletteUse16   (GDI.373)
 */
UINT16 SetSystemPaletteUse16( HDC16 hdc, UINT16 use )
{
    return SetSystemPaletteUse32( hdc, use );
}


/***********************************************************************
 *           SetSystemPaletteUse32   (GDI32.335)
 */
UINT32 SetSystemPaletteUse32( HDC32 hdc, UINT32 use )
{
    UINT32 old = SystemPaletteUse;
    fprintf( stdnimp,"SetSystemPaletteUse(%04x,%04x) // empty stub !!!\n",
             hdc, use );
    SystemPaletteUse = use;
    return old;
}


/***********************************************************************
 *           GetSystemPaletteUse16   (GDI.374)
 */
UINT16 GetSystemPaletteUse16( HDC16 hdc )
{
    return SystemPaletteUse;
}


/***********************************************************************
 *           GetSystemPaletteUse32   (GDI32.223)
 */
UINT32 GetSystemPaletteUse32( HDC32 hdc )
{
    return SystemPaletteUse;
}


/***********************************************************************
 *           GetSystemPaletteEntries16   (GDI.375)
 */
UINT16 GetSystemPaletteEntries16( HDC16 hdc, UINT16 start, UINT16 count,
                                  LPPALETTEENTRY entries )
{
    return GetSystemPaletteEntries32( hdc, start, count, entries );
}


/***********************************************************************
 *           GetSystemPaletteEntries32   (GDI32.222)
 */
UINT32 GetSystemPaletteEntries32( HDC32 hdc, UINT32 start, UINT32 count,
                                  LPPALETTEENTRY entries )
{
    UINT32 i;
    DC *dc;

    dprintf_palette(stddeb,"GetSystemPaletteEntries: hdc = %04x, cound = %i", hdc, count );

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (start >= dc->w.devCaps->sizePalette) return 0;
    if (start+count >= dc->w.devCaps->sizePalette)
	count = dc->w.devCaps->sizePalette - start;
    for (i = 0; i < count; i++)
    {
	*(COLORREF*)(entries + i) = COLOR_GetSystemPaletteEntry( start + i );

        dprintf_palette( stddeb,"\tidx(%02x) -> RGB(%08lx)\n",
                         start + i, *(COLORREF*)(entries + i) );
    }
    return count;
}


/***********************************************************************
 *           GetNearestPaletteIndex16   (GDI.370)
 */
UINT16 GetNearestPaletteIndex16( HPALETTE16 hpalette, COLORREF color )
{
    return GetNearestPaletteIndex32( hpalette, color );
}


/***********************************************************************
 *           GetNearestPaletteIndex32   (GDI32.203)
 */
UINT32 GetNearestPaletteIndex32( HPALETTE32 hpalette, COLORREF color )
{
    PALETTEOBJ*	palObj = (PALETTEOBJ*)GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    UINT32 index  = 0;

    if( palObj )
        index = COLOR_PaletteLookupPixel( palObj->logpalette.palPalEntry, 
				          palObj->logpalette.palNumEntries,
                                          NULL, color, FALSE );

    dprintf_palette(stddeb,"GetNearestPaletteIndex(%04x,%06lx): returning %d\n", 
                    hpalette, color, index );
    return index;
}


/***********************************************************************
 *           GetNearestColor16   (GDI.154)
 */
COLORREF GetNearestColor16( HDC16 hdc, COLORREF color )
{
    return GetNearestColor32( hdc, color );
}


/***********************************************************************
 *           GetNearestColor32   (GDI32.202)
 */
COLORREF GetNearestColor32( HDC32 hdc, COLORREF color )
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
    }

    dprintf_palette(stddeb,"GetNearestColor(%06lx): returning %06lx\n", 
                    color, nearest );
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
    return GDI_FreeObject( hpalette );
}


/***********************************************************************
 *           GDISelectPalette    (GDI.361)
 */
HPALETTE16 GDISelectPalette( HDC16 hdc, HPALETTE16 hpal, WORD wBkg)
{
    HPALETTE16 prev;
    DC *dc;

    dprintf_palette(stddeb, "GDISelectPalette: %04x %04x\n", hdc, hpal );
    
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    prev = dc->w.hPalette;
    dc->w.hPalette = hpal;
    if (!wBkg) hPrimaryPalette = hpal; 
    return prev;
}


/***********************************************************************
 *           GDIRealizePalette    (GDI.362)
 */
UINT16 GDIRealizePalette( HDC16 hdc )
{
    PALETTEOBJ* palPtr;
    int realized = 0;
    DC*		dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    dprintf_palette(stddeb, "GDIRealizePalette: %04x...", hdc );
    
    if( dc &&  dc->w.hPalette != hLastRealizedPalette )
    {
	if( dc->w.hPalette == STOCK_DEFAULT_PALETTE )
            return RealizeDefaultPalette( hdc );

        palPtr = (PALETTEOBJ *) GDI_GetObjPtr( dc->w.hPalette, PALETTE_MAGIC );
        
	realized = COLOR_SetMapping(palPtr, dc->w.hPalette != hPrimaryPalette 
                                  || dc->w.hPalette == STOCK_DEFAULT_PALETTE );
	hLastRealizedPalette = dc->w.hPalette;
    }
    else dprintf_palette(stddeb, " skipping ");
    
    dprintf_palette(stdnimp, " realized %i colors\n", realized );
    return (UINT16)realized;
}


/***********************************************************************
 *           RealizeDefaultPalette    (GDI.365)
 */
UINT16 RealizeDefaultPalette( HDC16 hdc )
{
    DC          *dc;
    PALETTEOBJ*  palPtr;
    int          i, index, realized = 0;

    dprintf_palette(stddeb,"RealizeDefaultPalette: %04x\n", hdc );

    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }

    if ( dc->w.flags & DC_MEMORY ) return 0;

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
BOOL16 IsDCCurrentPalette(HDC16 hDC)
{
    DC* dc = (DC *)GDI_GetObjPtr( hDC, DC_MAGIC );
    return (dc)?(dc->w.hPalette == hPrimaryPalette):FALSE;
}


/***********************************************************************
 *           SelectPalette16    (USER.282)
 */
HPALETTE16 SelectPalette16( HDC16 hDC, HPALETTE16 hPal,
                            BOOL16 bForceBackground )
{
    return SelectPalette32( hDC, hPal, bForceBackground );
}


/***********************************************************************
 *           SelectPalette32    (GDI32.300)
 */
HPALETTE32 SelectPalette32( HDC32 hDC, HPALETTE32 hPal,
                            BOOL32 bForceBackground )
{
    WORD	wBkgPalette = 1;
    PALETTEOBJ* lpt = (PALETTEOBJ*) GDI_GetObjPtr( hPal, PALETTE_MAGIC );

    dprintf_palette(stddeb,"SelectPalette: dc %04x pal %04x, force=%i ", 
			    hDC, hPal, bForceBackground);
    if( !lpt ) return 0;

    dprintf_palette(stddeb," entries = %d\n", 
			    lpt->logpalette.palNumEntries);

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
UINT16 RealizePalette16( HDC16 hDC )
{
    return RealizePalette32( hDC );
}


/***********************************************************************
 *           RealizePalette32    (GDI32.280)
 */
UINT32 RealizePalette32( HDC32 hDC )
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
 *	    UpdateColors	(GDI.366)
 *
 */
int UpdateColors( HDC16 hDC )
{
    HWND32 hWnd = WindowFromDC32( hDC );

    /* Docs say that we have to remap current drawable pixel by pixel
     * but it would take forever given the speed of XGet/PutPixel.
     */
    if (hWnd && !(COLOR_GetSystemPaletteFlags() & COLOR_VIRTUAL) ) 
	InvalidateRect32( hWnd, NULL, FALSE );
    return 0x666;
}

