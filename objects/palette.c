/*
 * GDI palette objects
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#ifdef linux
#include <values.h>
#endif
#ifdef __NetBSD__
#include <limits.h>
#define MAXINT INT_MAX
#endif
#include <X11/Xlib.h>

#include "gdi.h"

extern Display * XT_display;
extern Screen * XT_screen;


#define NB_RESERVED_COLORS  17
static char * ReservedColors[NB_RESERVED_COLORS] =
{
    "black",
    "gray25",
    "gray50",
    "gray75",
    "white",
    "red1",
    "red4",
    "green1",
    "green4",
    "blue1",
    "blue4",
    "cyan1",
    "cyan4",
    "magenta1",
    "magenta4",
    "yellow1",
    "yellow4"
};

GDIOBJHDR * PALETTE_systemPalette;

static int SysColorPixels[NB_RESERVED_COLORS];


/***********************************************************************
 *           PALETTE_Init
 */
BOOL PALETTE_Init()
{
    int i, size, pixel;
    XColor serverColor, exactColor;
    HPALETTE hpalette;
    LOGPALETTE * palPtr;

    size = DefaultVisual(XT_display,DefaultScreen(XT_display))->map_entries;
    palPtr = malloc( sizeof(LOGPALETTE) + (size-1)*sizeof(PALETTEENTRY) );
    if (!palPtr) return FALSE;
    palPtr->palVersion = 0x300;
    palPtr->palNumEntries = size;
    memset( palPtr->palPalEntry, 0xff, size*sizeof(PALETTEENTRY) );
    
    for (i = 0; i < NB_RESERVED_COLORS; i++)
    {
	if (XAllocNamedColor( XT_display,
			      DefaultColormapOfScreen( XT_screen ),
			      ReservedColors[i], 
			      &serverColor, &exactColor ))
	{
	    pixel = serverColor.pixel;
	    palPtr->palPalEntry[pixel].peRed   = serverColor.red >> 8;
	    palPtr->palPalEntry[pixel].peGreen = serverColor.green >> 8;
	    palPtr->palPalEntry[pixel].peBlue  = serverColor.blue >> 8;
	    palPtr->palPalEntry[pixel].peFlags = 0;
	}
    }
    hpalette = CreatePalette( palPtr );
    PALETTE_systemPalette = (GDIOBJHDR *) GDI_HEAP_ADDR( hpalette );
    free( palPtr );
    return TRUE;
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
    palettePtr = (PALETTEOBJ *) GDI_HEAP_ADDR( hpalette );
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
 *           GetNearestPaletteIndex    (GDI.370)
 */
WORD GetNearestPaletteIndex( HPALETTE hpalette, COLORREF color )
{
    int i, minDist, dist;
    WORD index = 0;
    BYTE r, g, b;
    PALETTEENTRY * entry;
    PALETTEOBJ * palPtr;
    
    palPtr = (PALETTEOBJ *) GDI_GetObjPtr( hpalette, PALETTE_MAGIC );
    if (!palPtr) return 0;
    
    r = GetRValue(color);
    g = GetGValue(color);
    b = GetBValue(color);
    entry = palPtr->logpalette.palPalEntry;
    for (i = 0, minDist = MAXINT; i < palPtr->logpalette.palNumEntries; i++)
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
#ifdef DEBUG_GDI
    printf( "GetNearestPaletteIndex(%x,%06x) : returning %d\n", 
	     hpalette, color, index );
#endif
    return index;
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
