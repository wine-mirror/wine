/*
 * GDI palette objects
 *
 * Copyright 1993,1994 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993,1994";
*/
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#if !defined  (MAXINT)
#include <limits.h>
#define MAXINT INT_MAX
#endif

#include <X11/Xlib.h>
#include "color.h"
#include "palette.h"
#include "stddebug.h"
/* #define DEBUG_PALETTE */
#include "debug.h"

static WORD SystemPaletteUse = SYSPAL_STATIC;	/* currently not considered */


/***********************************************************************
 *           PALETTE_GetNearestIndexAndColor
 */
static WORD PALETTE_GetNearestIndexAndColor(HPALETTE hpalette, COLORREF *color)
{
    int i, minDist, dist;
    WORD index = 0;
    BYTE r, g, b;
    PALETTEENTRY * entry;
    PALETTEOBJ * palPtr;
    
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;

    if ((COLOR_WinColormap != DefaultColormapOfScreen(screen)) &&
	(hpalette == STOCK_DEFAULT_PALETTE))
    {
	if ((*color & 0xffffff) == 0) return 0;  /* Entry 0 is black */
	if ((*color & 0xffffff) == 0xffffff)     /* Max entry is white */
	    return palPtr->logpalette.palNumEntries - 1;
    }
    
    r = GetRValue(*color);
    g = GetGValue(*color);
    b = GetBValue(*color);

    entry = palPtr->logpalette.palPalEntry;
    for (i = 0, minDist = MAXINT; minDist !=0 &&
         i < palPtr->logpalette.palNumEntries ; i++)
    {
	if (entry->peFlags != 0xff)
	{
	    dist = (r - entry->peRed) * (r - entry->peRed) +
		   (g - entry->peGreen) * (g - entry->peGreen) +
		   (b - entry->peBlue) * (b - entry->peBlue);	
	    if (dist < minDist)
	    {
		minDist = dist;
		index = i;
	    }
	}
	entry++;
    }
    entry = &palPtr->logpalette.palPalEntry[index];
    *color = RGB( entry->peRed, entry->peGreen, entry->peBlue );
    return index;
}


/***********************************************************************
 *           CreatePalette    (GDI.360)
 */
HPALETTE CreatePalette( LOGPALETTE * palette )
{
    PALETTEOBJ * palettePtr;
    HPALETTE hpalette;
    int size;

    size = sizeof(LOGPALETTE) + (palette->palNumEntries - 1) * sizeof(PALETTEENTRY);
    hpalette = GDI_AllocObject( sizeof(GDIOBJHDR) + size, PALETTE_MAGIC );
    if (!hpalette) return 0;
    palettePtr = (PALETTEOBJ *) GDI_HEAP_LIN_ADDR( hpalette );
    memcpy( &palettePtr->logpalette, palette, size );
    return hpalette;
}


/***********************************************************************
 *           GetPaletteEntries    (GDI.363)
 */
WORD GetPaletteEntries( HPALETTE hpalette, WORD start, WORD count,
		        LPPALETTEENTRY entries )
{
    PALETTEOBJ * palPtr;
    int numEntries;
        
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;
    numEntries = palPtr->logpalette.palNumEntries;
    if (start >= numEntries) return 0;
    if (start+count > numEntries) count = numEntries - start;
    memcpy( entries, &palPtr->logpalette.palPalEntry[start],
	    count * sizeof(PALETTEENTRY) );
    return count;
}


/***********************************************************************
 *           SetPaletteEntries    (GDI.364)
 */
WORD SetPaletteEntries( HPALETTE hpalette, WORD start, WORD count,
		        LPPALETTEENTRY entries )
{
    PALETTEOBJ * palPtr;
    int numEntries;
        
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;
    numEntries = palPtr->logpalette.palNumEntries;
    if (start >= numEntries) return 0;
    if (start+count > numEntries) count = numEntries - start;
    memcpy( &palPtr->logpalette.palPalEntry[start], entries,
	    count * sizeof(PALETTEENTRY) );
    return count;
}

/***********************************************************************
 *           ResizePalette          (GDI.368)
 */
BOOL ResizePalette(HPALETTE hPal, UINT cEntries)
{
 fprintf(stdnimp,"ResizePalette: empty stub! \n");
 return FALSE;
}

/***********************************************************************
 *           SetSystemPaletteUse    (GDI.373)
 *	Should this be per DC rather than system wide?
 *  Currently, it does not matter as the use is only set and returned,
 *  but not taken into account
 */
WORD SetSystemPaletteUse( HDC hdc, WORD use)
{
	 WORD old=SystemPaletteUse;
	 printf("SetSystemPaletteUse("NPFMT",%04X) // empty stub !!!\n", hdc, use);
	 SystemPaletteUse=use;
	 return old;
}

/***********************************************************************
 *           GetSystemPaletteUse    (GDI.374)
 */
WORD GetSystemPaletteUse( HDC hdc )
{
	printf("GetSystemPaletteUse("NPFMT") // empty stub !!!\n", hdc);
	return SystemPaletteUse;
}


/***********************************************************************
 *           GetSystemPaletteEntries    (GDI.375)
 */
WORD GetSystemPaletteEntries( HDC hdc, WORD start, WORD count,
			      LPPALETTEENTRY entries )
{
    WORD i;
    DC *dc;
    XColor color;

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    if (start >= dc->w.devCaps->sizePalette) return 0;
    if (start+count >= dc->w.devCaps->sizePalette)
	count = dc->w.devCaps->sizePalette - start;
    for (i = 0; i < count; i++)
    {
	color.pixel = start + i;
	XQueryColor( display, COLOR_WinColormap, &color );
	entries[i].peRed   = color.red >> 8;
	entries[i].peGreen = color.green >> 8;
	entries[i].peBlue  = color.blue >> 8;
	entries[i].peFlags = 0;	
    }
    return count;
}


/***********************************************************************
 *           GetNearestPaletteIndex    (GDI.370)
 */
WORD GetNearestPaletteIndex( HPALETTE hpalette, COLORREF color )
{
    WORD index = PALETTE_GetNearestIndexAndColor( hpalette, &color );
    dprintf_palette(stddeb,"GetNearestPaletteIndex("NPFMT",%06lx): returning %d\n", 
                    hpalette, color, index );
    return index;
}


/***********************************************************************
 *           GetNearestColor    (GDI.154)
 */
COLORREF GetNearestColor( HDC hdc, COLORREF color )
{
    COLORREF nearest = color;
    DC *dc;

    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    PALETTE_GetNearestIndexAndColor( dc->w.hPalette, &nearest );
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
 *           GDISelectPalette    (GDI.361)
 */
HPALETTE GDISelectPalette( HDC hdc, HPALETTE hpal )
{
    HPALETTE prev;
    DC *dc;

    dprintf_palette(stddeb, "GDISelectPalette: "NPFMT" "NPFMT"\n", hdc, hpal );
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    prev = dc->w.hPalette;
    dc->w.hPalette = hpal;
    if (hpal != STOCK_DEFAULT_PALETTE) COLOR_SetMapping( dc, 0, 0, 0 );
    else RealizeDefaultPalette( hdc );  /* Always realize default palette */
    return prev;
}


/***********************************************************************
 *           GDIRealizePalette    (GDI.362)
 */
UINT GDIRealizePalette( HDC hdc )
{
    dprintf_palette(stdnimp, "GDIRealizePalette: "NPFMT"\n", hdc );
    return 0;
}


/***********************************************************************
 *           SelectPalette    (USER.282)
 */
HPALETTE SelectPalette(HDC hDC, HPALETTE hPal, BOOL bForceBackground)
{
    return GDISelectPalette( hDC, hPal );
}


/***********************************************************************
 *           RealizePalette    (USER.283)
 */
UINT RealizePalette(HDC hDC)
{
    return GDIRealizePalette( hDC );
}

