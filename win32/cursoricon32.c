/*
 * Cursor and icon support
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Martin von Loewis
 */

/*
 * Theory:
 *
 * Cursors and icons are stored in a global heap block, with the
 * following layout:
 *
 * CURSORICONINFO info;
 * BYTE[]         ANDbits;
 * BYTE[]         XORbits;
 *
 * The bits structures are in the format of a device-dependent bitmap.
 *
 * This layout is very sub-optimal, as the bitmap bits are stored in
 * the X client instead of in the server like other bitmaps; however,
 * some programs (notably Paint Brush) expect to be able to manipulate
 * the bits directly :-(
 */

#include <string.h>
#include <stdlib.h>
#include "windows.h"
#include "bitmap.h"
#include "callback.h"
#include "cursoricon.h"
#include "sysmetrics.h"
#include "win.h"
#include "struct32.h"
#include "string32.h"
#include "resource32.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "task.h"

/* This dictionary could might eventually become a macro for better reuse */
struct MAP_DWORD_DWORD{
	DWORD key;
	DWORD value;
};

struct MAP_DWORD_DWORD *CURSORICON_map;
int CURSORICON_count;

BOOL CURSORICON_lookup(DWORD key,DWORD *value)
{
	int i;
	for(i=0;i<CURSORICON_count;i++)
	{
		if(key==CURSORICON_map[i].key)
		{
			*value=CURSORICON_map[i].value;
			return TRUE;
		}
	}
	return FALSE;
}

void CURSORICON_insert(DWORD key,DWORD value)
{
	if(!CURSORICON_count)
	{
		CURSORICON_count=1;
		CURSORICON_map=malloc(sizeof(struct MAP_DWORD_DWORD));
	}else{
		CURSORICON_count++;
		CURSORICON_map=realloc(CURSORICON_map,
			sizeof(struct MAP_DWORD_DWORD)*CURSORICON_count);
	}
	CURSORICON_map[CURSORICON_count-1].key=key;
	CURSORICON_map[CURSORICON_count-1].value=value;
}

/**********************************************************************
 *	    CURSORICON32_FindBestIcon
 *
 * Find the icon closest to the requested size and number of colors.
 */
static ICONDIRENTRY32 *CURSORICON32_FindBestIcon( CURSORICONDIR32 *dir, 
	int width, int height, int colors )
{
    int i, maxcolors, maxwidth, maxheight;
    ICONDIRENTRY32 *entry, *bestEntry = NULL;

    if (dir->idCount < 1)
    {
        fprintf( stderr, "Icon: empty directory!\n" );
        return NULL;
    }
    if (dir->idCount == 1) return &dir->idEntries[0].icon;  /* No choice... */

    /* First find the exact size with less colors */

    maxcolors = 0;
    for (i = 0, entry = &dir->idEntries[0].icon; i < dir->idCount; i++,entry++)
        if ((entry->bWidth == width) && (entry->bHeight == height) &&
            (entry->bColorCount <= colors) && (entry->bColorCount > maxcolors))
        {
            bestEntry = entry;
            maxcolors = entry->bColorCount;
        }
    if (bestEntry) return bestEntry;

    /* First find the exact size with more colors */

    maxcolors = 255;
    for (i = 0, entry = &dir->idEntries[0].icon; i < dir->idCount; i++,entry++)
        if ((entry->bWidth == width) && (entry->bHeight == height) &&
            (entry->bColorCount > colors) && (entry->bColorCount <= maxcolors))
        {
            bestEntry = entry;
            maxcolors = entry->bColorCount;
        }
    if (bestEntry) return bestEntry;

    /* Now find a smaller one with less colors */

    maxcolors = maxwidth = maxheight = 0;
    for (i = 0, entry = &dir->idEntries[0].icon; i < dir->idCount; i++,entry++)
        if ((entry->bWidth <= width) && (entry->bHeight <= height) &&
            (entry->bWidth >= maxwidth) && (entry->bHeight >= maxheight) &&
            (entry->bColorCount <= colors) && (entry->bColorCount > maxcolors))
        {
            bestEntry = entry;
            maxwidth  = entry->bWidth;
            maxheight = entry->bHeight;
            maxcolors = entry->bColorCount;
        }
    if (bestEntry) return bestEntry;

    /* Now find a smaller one with more colors */

    maxcolors = 255;
    maxwidth = maxheight = 0;
    for (i = 0, entry = &dir->idEntries[0].icon; i < dir->idCount; i++,entry++)
        if ((entry->bWidth <= width) && (entry->bHeight <= height) &&
            (entry->bWidth >= maxwidth) && (entry->bHeight >= maxheight) &&
            (entry->bColorCount > colors) && (entry->bColorCount <= maxcolors))
        {
            bestEntry = entry;
            maxwidth  = entry->bWidth;
            maxheight = entry->bHeight;
            maxcolors = entry->bColorCount;
        }
    if (bestEntry) return bestEntry;

    /* Now find a larger one with less colors */

    maxcolors = 0;
    maxwidth = maxheight = 255;
    for (i = 0, entry = &dir->idEntries[0].icon; i < dir->idCount; i++,entry++)
        if ((entry->bWidth <= maxwidth) && (entry->bHeight <= maxheight) &&
            (entry->bColorCount <= colors) && (entry->bColorCount > maxcolors))
        {
            bestEntry = entry;
            maxwidth  = entry->bWidth;
            maxheight = entry->bHeight;
            maxcolors = entry->bColorCount;
        }
    if (bestEntry) return bestEntry;

    /* Now find a larger one with more colors */

    maxcolors = maxwidth = maxheight = 255;
    for (i = 0, entry = &dir->idEntries[0].icon; i < dir->idCount; i++,entry++)
        if ((entry->bWidth <= maxwidth) && (entry->bHeight <= maxheight) &&
            (entry->bColorCount > colors) && (entry->bColorCount <= maxcolors))
        {
            bestEntry = entry;
            maxwidth  = entry->bWidth;
            maxheight = entry->bHeight;
            maxcolors = entry->bColorCount;
        }

    return bestEntry;
}


/**********************************************************************
 *	    CURSORICON32_FindBestCursor
 *
 * Find the cursor closest to the requested size.
 */
static CURSORDIRENTRY32 *CURSORICON32_FindBestCursor( CURSORICONDIR32 *dir,
                                                  int width, int height )
{
    int i, maxwidth, maxheight;
    CURSORDIRENTRY32 *entry, *bestEntry = NULL;

    if (dir->idCount < 1)
    {
        fprintf( stderr, "Cursor: empty directory!\n" );
        return NULL;
    }
    if (dir->idCount == 1) return &dir->idEntries[0].cursor; /* No choice... */

    /* First find the largest one smaller than or equal to the requested size*/

    maxwidth = maxheight = 0;
    for(i = 0,entry = &dir->idEntries[0].cursor; i < dir->idCount; i++,entry++)
        if ((entry->wWidth <= width) && (entry->wHeight <= height) &&
            (entry->wWidth > maxwidth) && (entry->wHeight > maxheight))
        {
            bestEntry = entry;
            maxwidth  = entry->wWidth;
            maxheight = entry->wHeight;
        }
    if (bestEntry) return bestEntry;

    /* Now find the smallest one larger than the requested size */

    maxwidth = maxheight = 255;
    for(i = 0,entry = &dir->idEntries[0].cursor; i < dir->idCount; i++,entry++)
        if ((entry->wWidth < maxwidth) && (entry->wHeight < maxheight))
        {
            bestEntry = entry;
            maxwidth  = entry->wWidth;
            maxheight = entry->wHeight;
        }

    return bestEntry;
}


/**********************************************************************
 *	    CURSORICON32_LoadDirEntry
 *
 * Load the icon/cursor directory for a given resource name and find the
 * best matching entry.
 */
static BOOL CURSORICON32_LoadDirEntry(HANDLE hInstance, LPCWSTR name,
                                    int width, int height, int colors,
                                    BOOL fCursor, CURSORICONDIRENTRY32 *dirEntry)
{
    HANDLE32 hRsrc;
    HANDLE32 hMem;
    CURSORICONDIR32 *dir;
    CURSORICONDIRENTRY32 *entry = NULL;

    if (!(hRsrc = FindResource32( hInstance, name,
                (LPCWSTR)(fCursor ? RT_GROUP_CURSOR : RT_GROUP_ICON) )))
        return FALSE;
    if (!(hMem = LoadResource32( hInstance, hRsrc ))) return FALSE;
    if ((dir = (CURSORICONDIR32 *)LockResource32( hMem )))
    {
        if (fCursor)
            entry = (CURSORICONDIRENTRY32 *)CURSORICON32_FindBestCursor( dir,
                                                               width, height );
        else
            entry = (CURSORICONDIRENTRY32 *)CURSORICON32_FindBestIcon( dir,
                                                       width, height, colors );
        if (entry) *dirEntry = *entry;
    }
    FreeResource32( hMem );
    return (entry != NULL);
}


/**********************************************************************
 *	    CURSORICON32_LoadHandler 
 *
 * Create a cursor or icon from a resource.
 */
static HANDLE CURSORICON32_LoadHandler( HANDLE32 handle, HINSTANCE hInstance,
                                      BOOL fCursor )
{
    HANDLE hAndBits, hXorBits, hRes;
    HDC hdc;
    int size, sizeAnd, sizeXor;
    POINT hotspot = { 0 ,0 };
    BITMAPOBJ *bmpXor, *bmpAnd;
    BITMAPINFO *bmi, *pInfo;
    CURSORICONINFO *info;
    char *bits;

	hRes=0;
    if (fCursor)  /* If cursor, get the hotspot */
    {
        POINT *pt = (POINT *)LockResource32( handle );
        hotspot = *pt;
        bmi = (BITMAPINFO *)(pt + 1);
    }
    else bmi = (BITMAPINFO *)LockResource32( handle );

    /* Create a copy of the bitmap header */

    size = DIB_BitmapInfoSize( bmi, DIB_RGB_COLORS );
    /* Make sure we have room for the monochrome bitmap later on */
    size = MAX( size, sizeof(BITMAPINFOHEADER) + 2*sizeof(RGBQUAD) );
    pInfo = (BITMAPINFO *)xmalloc( size );
    memcpy( pInfo, bmi, size );

    if (pInfo->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
    {
        if (pInfo->bmiHeader.biCompression != BI_RGB)
        {
            fprintf(stderr,"Unknown size for compressed icon bitmap.\n");
            free( pInfo );
            return 0;
        }
        pInfo->bmiHeader.biHeight /= 2;
    }
    else if (pInfo->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)pInfo;
        core->bcHeight /= 2;
    }
    else
    {
        fprintf( stderr, "CURSORICON32_Load: Unknown bitmap length %ld!\n",
                 pInfo->bmiHeader.biSize );
        free( pInfo );
        return 0;
    }

    /* Create the XOR bitmap */

    if (!(hdc = GetDC( 0 )))
    {
        free( pInfo );
        return 0;
    }

    hXorBits = CreateDIBitmap( hdc, &pInfo->bmiHeader, CBM_INIT,
                               (char*)bmi + size, pInfo, DIB_RGB_COLORS );

    /* Fix the bitmap header to load the monochrome mask */

    if (pInfo->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
    {
        BITMAPINFOHEADER *bih = &pInfo->bmiHeader;
        RGBQUAD *rgb = pInfo->bmiColors;
        bits = (char *)bmi + size +
            DIB_GetImageWidthBytes(bih->biWidth,bih->biBitCount)*bih->biHeight;
        bih->biBitCount = 1;
        bih->biClrUsed = bih->biClrImportant = 2;
        rgb[0].rgbBlue = rgb[0].rgbGreen = rgb[0].rgbRed = 0x00;
        rgb[1].rgbBlue = rgb[1].rgbGreen = rgb[1].rgbRed = 0xff;
        rgb[0].rgbReserved = rgb[1].rgbReserved = 0;
    }
    else
    {
        BITMAPCOREHEADER *bch = (BITMAPCOREHEADER *)pInfo;
        RGBTRIPLE *rgb = (RGBTRIPLE *)(bch + 1);
        bits = (char *)bmi + size +
            DIB_GetImageWidthBytes(bch->bcWidth,bch->bcBitCount)*bch->bcHeight;
        bch->bcBitCount = 1;
        rgb[0].rgbtBlue = rgb[0].rgbtGreen = rgb[0].rgbtRed = 0x00;
        rgb[1].rgbtBlue = rgb[1].rgbtGreen = rgb[1].rgbtRed = 0xff;
    }

    /* Create the AND bitmap */

    hAndBits = CreateDIBitmap( hdc, &pInfo->bmiHeader, CBM_INIT,
                               bits, pInfo, DIB_RGB_COLORS );
    ReleaseDC( 0, hdc );

    /* Now create the CURSORICONINFO structure */

    bmpXor = (BITMAPOBJ *) GDI_GetObjPtr( hXorBits, BITMAP_MAGIC );
    bmpAnd = (BITMAPOBJ *) GDI_GetObjPtr( hAndBits, BITMAP_MAGIC );
    sizeXor = bmpXor->bitmap.bmHeight * bmpXor->bitmap.bmWidthBytes;
    sizeAnd = bmpAnd->bitmap.bmHeight * bmpAnd->bitmap.bmWidthBytes;

    if (!(hRes = GlobalAlloc( GMEM_MOVEABLE,
                                sizeof(CURSORICONINFO) + sizeXor + sizeAnd)))
    {
        DeleteObject( hXorBits );
        DeleteObject( hAndBits );
        return 0;
    }

    /* Make it owned by the module */
    if (hInstance) FarSetOwner( hRes, (WORD)(DWORD)GetExePtr(hInstance) );

    info = (CURSORICONINFO *)GlobalLock( hRes );
    info->ptHotSpot.x   = hotspot.x;
    info->ptHotSpot.y   = hotspot.y;
    info->nWidth        = bmpXor->bitmap.bmWidth;
    info->nHeight       = bmpXor->bitmap.bmHeight;
    info->nWidthBytes   = bmpXor->bitmap.bmWidthBytes;
    info->bPlanes       = bmpXor->bitmap.bmPlanes;
    info->bBitsPerPixel = bmpXor->bitmap.bmBitsPixel;

    /* Transfer the bitmap bits to the CURSORICONINFO structure */

    GetBitmapBits( hAndBits, sizeAnd, (char *)(info + 1) );
    GetBitmapBits( hXorBits, sizeXor, (char *)(info + 1) + sizeAnd );
    DeleteObject( hXorBits );
    DeleteObject( hAndBits );
    GlobalUnlock( hRes );
    return hRes;
}

/**********************************************************************
 *	    CURSORICON32_Load
 *
 * Load a cursor or icon.
 */
static HANDLE CURSORICON32_Load( HANDLE hInstance, LPCWSTR name, int width,
                               int height, int colors, BOOL fCursor )
{
    HANDLE32 handle;
	HANDLE hRet;
    HANDLE32  hRsrc;
    CURSORICONDIRENTRY32 dirEntry;

	if(!hInstance)	/* OEM cursor/icon */
	{
		WORD resid;
		if(HIWORD(name))
		{
			LPSTR ansi;
			ansi=STRING32_DupUniToAnsi(name);
			if(ansi[0]=='#')	/*Check for '#xxx' name */
			{
				resid=atoi(ansi+1);
				free(ansi);
			}else{
				free(ansi);
				return 0;
			}
		}
		else
			resid=(WORD)(int)name;
		return OBM_LoadCursorIcon(resid, fCursor);
	}

    /* Find the best entry in the directory */

    if (!CURSORICON32_LoadDirEntry( hInstance, name, width, height,
                                  colors, fCursor, &dirEntry )) return 0;

    /* Load the resource */

    if (!(hRsrc = FindResource32( hInstance,
                      (LPWSTR) (DWORD) dirEntry.icon.wResId,
                      (LPWSTR) (fCursor ? RT_CURSOR : RT_ICON )))) return 0;
    if (!(handle = LoadResource32( hInstance, hRsrc ))) return 0;

	/* Use the resource handle as key to detect multiple loading */
	if(CURSORICON_lookup(handle,&hRet))
		return hRet;

    hRet = CURSORICON32_LoadHandler( handle, hInstance, fCursor );
    /* Obsolete - FreeResource32(handle);*/
	CURSORICON_insert(handle,hRet);
    return hRet;
}


/***********************************************************************
 *           LoadCursor
 */
HCURSOR WIN32_LoadCursorW( HANDLE hInstance, LPCWSTR name )
{
    return CURSORICON32_Load( hInstance, name,
                            SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR, 1, TRUE);
}

HCURSOR WIN32_LoadCursorA(HANDLE hInstance, LPCSTR name)
{
	HCURSOR res=0;
	if(!HIWORD(name))
		return WIN32_LoadCursorW(hInstance, name);
	else {
		LPWSTR uni = STRING32_DupAnsiToUni(name);
		res = WIN32_LoadCursorW(hInstance, uni);
		free(uni);
	}
	return res;
}


/***********************************************************************
 *           LoadIcon
 */
HICON WIN32_LoadIconW( HANDLE hInstance, LPCWSTR name )
{
    return CURSORICON32_Load( hInstance, name,
                            SYSMETRICS_CXICON, SYSMETRICS_CYICON,
                            MIN( 16, 1 << screenDepth ), FALSE );
}

HICON WIN32_LoadIconA( HANDLE hInstance, LPCSTR name)
{
	HICON res=0;
	if(!HIWORD(name))
		return WIN32_LoadIconW(hInstance, name);
	else {
		LPWSTR uni = STRING32_DupAnsiToUni(name);
		res = WIN32_LoadIconW(hInstance, uni);
		free(uni);
	}
	return res;
}
	
