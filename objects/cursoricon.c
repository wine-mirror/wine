/*
 * Cursor and icon support
 *
 * Copyright 1995 Alexandre Julliard
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
#include "color.h"
#include "bitmap.h"
#include "callback.h"
#include "cursoricon.h"
#include "sysmetrics.h"
#include "win.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "task.h"

extern UINT16 COLOR_GetSystemPaletteSize();

Cursor CURSORICON_XCursor = None;  /* Current X cursor */
static HCURSOR16 hActiveCursor = 0;  /* Active cursor */
static int CURSOR_ShowCount = 0;   /* Cursor display count */
static RECT32 CURSOR_ClipRect;       /* Cursor clipping rect */

/**********************************************************************
 *	    CURSORICON_FindBestIcon
 *
 * Find the icon closest to the requested size and number of colors.
 */
static ICONDIRENTRY *CURSORICON_FindBestIcon( CURSORICONDIR *dir, int width,
                                              int height, int colors )
{
    int i, maxcolors, maxwidth, maxheight;
    ICONDIRENTRY *entry, *bestEntry = NULL;

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
 *	    CURSORICON_FindBestCursor
 *
 * Find the cursor closest to the requested size.
 */
static CURSORDIRENTRY *CURSORICON_FindBestCursor( CURSORICONDIR *dir,
                                                  int width, int height )
{
    int i, maxwidth, maxheight;
    CURSORDIRENTRY *entry, *bestEntry = NULL;

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
 *	    CURSORICON_LoadDirEntry
 *
 * Load the icon/cursor directory for a given resource name and find the
 * best matching entry.
 */
static BOOL CURSORICON_LoadDirEntry(HINSTANCE32 hInstance, SEGPTR name,
                                    int width, int height, int colors,
                                    BOOL fCursor, CURSORICONDIRENTRY *dirEntry)
{
    HRSRC16 hRsrc;
    HGLOBAL16 hMem;
    CURSORICONDIR *dir;
    CURSORICONDIRENTRY *entry = NULL;

    if (!(hRsrc = FindResource16( hInstance, name,
                                fCursor ? RT_GROUP_CURSOR : RT_GROUP_ICON )))
        return FALSE;
    if (!(hMem = LoadResource16( hInstance, hRsrc ))) return FALSE;
    if ((dir = (CURSORICONDIR *)LockResource16( hMem )))
    {
        if (fCursor)
            entry = (CURSORICONDIRENTRY *)CURSORICON_FindBestCursor( dir,
                                                               width, height );
        else
            entry = (CURSORICONDIRENTRY *)CURSORICON_FindBestIcon( dir,
                                                       width, height, colors );
        if (entry) *dirEntry = *entry;
    }
    FreeResource16( hMem );
    return (entry != NULL);
}


/**********************************************************************
 *	    CURSORICON_LoadHandler 
 *
 * Create a cursor or icon from a resource.
 */
HGLOBAL16 CURSORICON_LoadHandler( HGLOBAL16 handle, HINSTANCE16 hInstance,
                                  BOOL fCursor )
{
    HBITMAP32 hAndBits, hXorBits;
    HDC32 hdc;
    int size, sizeAnd, sizeXor;
    POINT16 hotspot = { 0 ,0 };
    BITMAPOBJ *bmpXor, *bmpAnd;
    BITMAPINFO *bmi, *pInfo;
    CURSORICONINFO *info;
    char *bits;

    if (fCursor)  /* If cursor, get the hotspot */
    {
        POINT16 *pt = (POINT16 *)LockResource16( handle );
        hotspot = *pt;
        bmi = (BITMAPINFO *)(pt + 1);
    }
    else bmi = (BITMAPINFO *)LockResource16( handle );

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
        fprintf( stderr, "CURSORICON_Load: Unknown bitmap length %ld!\n",
                 pInfo->bmiHeader.biSize );
        free( pInfo );
        return 0;
    }

    /* Create the XOR bitmap */

    if (!(hdc = GetDC32( 0 )))
    {
        free( pInfo );
        return 0;
    }

    hXorBits = CreateDIBitmap32( hdc, &pInfo->bmiHeader, CBM_INIT,
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

    hAndBits = CreateDIBitmap32( hdc, &pInfo->bmiHeader, CBM_INIT,
                                 bits, pInfo, DIB_RGB_COLORS );
    ReleaseDC32( 0, hdc );

    /* Now create the CURSORICONINFO structure */

    bmpXor = (BITMAPOBJ *) GDI_GetObjPtr( hXorBits, BITMAP_MAGIC );
    bmpAnd = (BITMAPOBJ *) GDI_GetObjPtr( hAndBits, BITMAP_MAGIC );
    sizeXor = bmpXor->bitmap.bmHeight * bmpXor->bitmap.bmWidthBytes;
    sizeAnd = bmpAnd->bitmap.bmHeight * bmpAnd->bitmap.bmWidthBytes;

    if (!(handle = GlobalAlloc16( GMEM_MOVEABLE,
                                  sizeof(CURSORICONINFO) + sizeXor + sizeAnd)))
    {
        DeleteObject32( hXorBits );
        DeleteObject32( hAndBits );
        return 0;
    }

    /* Make it owned by the module */
    if (hInstance) FarSetOwner( handle, GetExePtr(hInstance) );

    info = (CURSORICONINFO *)GlobalLock16( handle );
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
    DeleteObject32( hXorBits );
    DeleteObject32( hAndBits );
    GlobalUnlock16( handle );
    return handle;
}

/**********************************************************************
 *	    CURSORICON_Load
 *
 * Load a cursor or icon.
 */
static HGLOBAL16 CURSORICON_Load( HINSTANCE16 hInstance, SEGPTR name,
                                  int width, int height, int colors,
                                  BOOL fCursor )
{
    HGLOBAL16 handle, hRet;
    HRSRC16 hRsrc;
    CURSORICONDIRENTRY dirEntry;

    if (!hInstance)  /* OEM cursor/icon */
    {
        if (HIWORD(name))  /* Check for '#xxx' name */
        {
            char *ptr = PTR_SEG_TO_LIN( name );
            if (ptr[0] != '#') return 0;
            if (!(name = (SEGPTR)atoi( ptr + 1 ))) return 0;
        }
        return OBM_LoadCursorIcon( LOWORD(name), fCursor );
    }

    /* Find the best entry in the directory */

    if (!CURSORICON_LoadDirEntry( hInstance, name, width, height,
                                  colors, fCursor, &dirEntry )) return 0;

    /* Load the resource */

    if (!(hRsrc = FindResource16( hInstance,
                                MAKEINTRESOURCE( dirEntry.icon.wResId ),
                                fCursor ? RT_CURSOR : RT_ICON ))) return 0;
    if (!(handle = LoadResource16( hInstance, hRsrc ))) return 0;

    hRet = CURSORICON_LoadHandler( handle, hInstance, fCursor );
    FreeResource16(handle);
    return hRet;
}


/***********************************************************************
 *           CURSORICON_Copy
 *
 * Make a copy of a cursor or icon.
 */
static HGLOBAL16 CURSORICON_Copy( HINSTANCE16 hInstance, HGLOBAL16 handle )
{
    char *ptrOld, *ptrNew;
    int size;
    HGLOBAL16 hNew;

    if (!(ptrOld = (char *)GlobalLock16( handle ))) return 0;
    if (!(hInstance = GetExePtr( hInstance ))) return 0;
    size = GlobalSize16( handle );
    hNew = GlobalAlloc16( GMEM_MOVEABLE, size );
    FarSetOwner( hNew, hInstance );
    ptrNew = (char *)GlobalLock16( hNew );
    memcpy( ptrNew, ptrOld, size );
    GlobalUnlock16( handle );
    GlobalUnlock16( hNew );
    return hNew;
}

/***********************************************************************
 *           CURSORICON_IconToCursor
 *
 * Converts bitmap to mono and truncates if icon is too large
 */
HCURSOR16 CURSORICON_IconToCursor(HICON16 hIcon, BOOL32 bSemiTransparent)
{
 HCURSOR16       hRet = 0;
 CURSORICONINFO *ptr = NULL;
 HTASK16 	 hTask = GetCurrentTask();
 TDB*  		 pTask = (TDB *)GlobalLock16(hTask);

 if(hIcon)
    if (!(ptr = (CURSORICONINFO*)GlobalLock16( hIcon ))) return FALSE;
       if (ptr->bPlanes * ptr->bBitsPerPixel == 1)
           hRet = CURSORICON_Copy( pTask->hInstance, hIcon );
       else
          {
           BYTE  pAndBits[128];
           BYTE  pXorBits[128];
	   int   x, y, ix, iy, shift; 
	   int   bpp = (ptr->bBitsPerPixel>=24)?32:ptr->bBitsPerPixel; /* this sucks */
           BYTE* psPtr = (BYTE *)(ptr + 1) +
                            ptr->nHeight * BITMAP_WIDTH_BYTES(ptr->nWidth,1);
           BYTE* pxbPtr = pXorBits;
           unsigned *psc = NULL, val = 0;
           unsigned val_base = 0xffffffff >> (32 - bpp);
           BYTE* pbc = NULL;

           COLORREF       col;
           CURSORICONINFO cI;

           if(!pTask) return 0;

           memset(pXorBits, 0, 128);
           cI.bBitsPerPixel = 1; cI.bPlanes = 1;
           cI.ptHotSpot.x = cI.ptHotSpot.y = 15;
           cI.nWidth = 32; cI.nHeight = 32;
           cI.nWidthBytes = 4;	/* 1bpp */

           x = (ptr->nWidth > 32) ? 32 : ptr->nWidth;
           y = (ptr->nHeight > 32) ? 32 : ptr->nHeight;

           for( iy = 0; iy < y; iy++ )
           {
              val = BITMAP_WIDTH_BYTES( ptr->nWidth, 1 );
              memcpy( pAndBits + iy * 4,
                     (BYTE *)(ptr + 1) + iy * val, (val>4) ? 4 : val);
              shift = iy % 2;

              for( ix = 0; ix < x; ix++ )
              {
                if( bSemiTransparent && ((ix+shift)%2) )
                {
                    pbc = pAndBits + iy * 4 + ix/8;
                   *pbc |= 0x80 >> (ix%8);
                }
                else
                {
                  psc = (unsigned*)(psPtr + (ix * bpp)/8);
                  val = ((*psc) >> (ix * bpp)%8) & val_base;
                  col = COLOR_ToLogical(val);
                  if( GetRValue(col) > 0xa0 ||
                      GetGValue(col) > 0x80 ||
                      GetBValue(col) > 0xa0 )
                  {
                    pbc = pxbPtr + ix/8;
                   *pbc |= 0x80 >> (ix%8);
                  }
                }
              }
              psPtr += ptr->nWidthBytes;
              pxbPtr += 4;
           }
           hRet = CreateCursorIconIndirect( pTask->hInstance , &cI, pAndBits, pXorBits);

           if( !hRet ) /* fall back on default drag cursor */
                hRet = CURSORICON_Copy( pTask->hInstance ,
                              CURSORICON_Load(0,MAKEINTRESOURCE(OCR_DRAGOBJECT),
                                         SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR, 1, TRUE) );
          }

 return hRet;
}

/***********************************************************************
 *           LoadCursor    (USER.173)
 */
HCURSOR16 LoadCursor16( HINSTANCE16 hInstance, SEGPTR name )
{
    if (HIWORD(name))
        dprintf_cursor( stddeb, "LoadCursor16: %04x '%s'\n",
                        hInstance, (char *)PTR_SEG_TO_LIN( name ) );
    else
        dprintf_cursor( stddeb, "LoadCursor16: %04x %04x\n",
                        hInstance, LOWORD(name) );

    return CURSORICON_Load( hInstance, name,
                            SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR, 1, TRUE);
}


/***********************************************************************
 *           LoadIcon    (USER.174)
 */
HICON16 LoadIcon16(HINSTANCE16 hInstance,SEGPTR name)
{
    if (HIWORD(name))
        dprintf_icon( stddeb, "LoadIcon: %04x '%s'\n",
                      hInstance, (char *)PTR_SEG_TO_LIN( name ) );
    else
        dprintf_icon( stddeb, "LoadIcon: %04x %04x\n",
                      hInstance, LOWORD(name) );

    return CURSORICON_Load( hInstance, name,
                            SYSMETRICS_CXICON, SYSMETRICS_CYICON,
                            MIN( 16, COLOR_GetSystemPaletteSize() ), FALSE );
}


/***********************************************************************
 *           CreateCursor    (USER.406)
 */
HCURSOR16 CreateCursor( HINSTANCE16 hInstance, INT xHotSpot, INT yHotSpot,
                        INT nWidth, INT nHeight,
                        const BYTE *lpANDbits, const BYTE *lpXORbits )
{
    CURSORICONINFO info = { { xHotSpot, yHotSpot }, nWidth, nHeight, 0, 1, 1 };

    dprintf_cursor( stddeb, "CreateCursor: %dx%d spot=%d,%d xor=%p and=%p\n",
                    nWidth, nHeight, xHotSpot, yHotSpot, lpXORbits, lpANDbits);
    return CreateCursorIconIndirect( hInstance, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateIcon    (USER.407)
 */
HICON16 CreateIcon( HINSTANCE16 hInstance, INT nWidth, INT nHeight, BYTE bPlanes,
                  BYTE bBitsPixel, const BYTE* lpANDbits, const BYTE* lpXORbits)
{
    CURSORICONINFO info = { { 0, 0 }, nWidth, nHeight, 0, bPlanes, bBitsPixel };

    dprintf_icon( stddeb, "CreateIcon: %dx%dx%d, xor=%p, and=%p\n",
                  nWidth, nHeight, bPlanes * bBitsPixel, lpXORbits, lpANDbits);
    return CreateCursorIconIndirect( hInstance, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateCursorIconIndirect    (USER.408)
 */
HGLOBAL16 CreateCursorIconIndirect( HINSTANCE16 hInstance,
                                    CURSORICONINFO *info,
                                    const BYTE *lpANDbits,
                                    const BYTE *lpXORbits )
{
    HGLOBAL16 handle;
    char *ptr;
    int sizeAnd, sizeXor;

    hInstance = GetExePtr( hInstance );  /* Make it a module handle */
    if (!hInstance || !lpXORbits || !lpANDbits || info->bPlanes != 1) return 0;
    info->nWidthBytes = BITMAP_WIDTH_BYTES(info->nWidth,info->bBitsPerPixel);
    sizeXor = info->nHeight * info->nWidthBytes;
    sizeAnd = info->nHeight * BITMAP_WIDTH_BYTES( info->nWidth, 1 );
    if (!(handle = DirectResAlloc(hInstance, 0x10,
                                  sizeof(CURSORICONINFO) + sizeXor + sizeAnd)))
        return 0;
    ptr = (char *)GlobalLock16( handle );
    memcpy( ptr, info, sizeof(*info) );
    memcpy( ptr + sizeof(CURSORICONINFO), lpANDbits, sizeAnd );
    memcpy( ptr + sizeof(CURSORICONINFO) + sizeAnd, lpXORbits, sizeXor );
    GlobalUnlock16( handle );
    return handle;
}


/***********************************************************************
 *           CopyIcon16    (USER.368)
 */
HICON16 CopyIcon16( HINSTANCE16 hInstance, HICON16 hIcon )
{
    dprintf_icon( stddeb, "CopyIcon16: %04x %04x\n", hInstance, hIcon );
    return CURSORICON_Copy( hInstance, hIcon );
}


/***********************************************************************
 *           CopyIcon32    (USER32.59)
 */
HICON32 CopyIcon32( HICON32 hIcon )
{
    dprintf_icon( stddeb, "CopyIcon32: %04x\n", hIcon );
    return CURSORICON_Copy( 0, hIcon );
}


/***********************************************************************
 *           CopyCursor16    (USER.369)
 */
HCURSOR16 CopyCursor16( HINSTANCE16 hInstance, HCURSOR16 hCursor )
{
    dprintf_cursor( stddeb, "CopyCursor16: %04x %04x\n", hInstance, hCursor );
    return CURSORICON_Copy( hInstance, hCursor );
}


/***********************************************************************
 *           DestroyIcon    (USER.457)
 */
BOOL DestroyIcon( HICON16 hIcon )
{
    dprintf_icon( stddeb, "DestroyIcon: %04x\n", hIcon );
    /* FIXME: should check for OEM icon here */
    return (GlobalFree16( hIcon ) != 0);
}


/***********************************************************************
 *           DestroyCursor    (USER.458)
 */
BOOL DestroyCursor( HCURSOR16 hCursor )
{
    dprintf_cursor( stddeb, "DestroyCursor: %04x\n", hCursor );
    /* FIXME: should check for OEM cursor here */
    return (GlobalFree16( hCursor ) != 0);
}


/***********************************************************************
 *           DrawIcon    (USER.84)
 */
BOOL DrawIcon( HDC16 hdc, INT x, INT y, HICON16 hIcon )
{
    CURSORICONINFO *ptr;
    HDC32 hMemDC;
    HBITMAP16 hXorBits, hAndBits;
    COLORREF oldFg, oldBg;

    if (!(ptr = (CURSORICONINFO *)GlobalLock16( hIcon ))) return FALSE;
    if (!(hMemDC = CreateCompatibleDC32( hdc ))) return FALSE;
    hAndBits = CreateBitmap( ptr->nWidth, ptr->nHeight, 1, 1, (char *)(ptr+1));
    hXorBits = CreateBitmap( ptr->nWidth, ptr->nHeight, ptr->bPlanes,
                             ptr->bBitsPerPixel, (char *)(ptr + 1)
                         + ptr->nHeight * BITMAP_WIDTH_BYTES(ptr->nWidth,1) );
    oldFg = SetTextColor( hdc, RGB(0,0,0) );
    oldBg = SetBkColor( hdc, RGB(255,255,255) );

    if (hXorBits && hAndBits)
    {
        HBITMAP32 hBitTemp = SelectObject32( hMemDC, hAndBits );
        BitBlt32( hdc, x, y, ptr->nWidth, ptr->nHeight, hMemDC, 0, 0, SRCAND );
        SelectObject32( hMemDC, hXorBits );
        BitBlt32(hdc, x, y, ptr->nWidth, ptr->nHeight, hMemDC, 0, 0,SRCINVERT);
        SelectObject32( hMemDC, hBitTemp );
    }
    DeleteDC32( hMemDC );
    if (hXorBits) DeleteObject32( hXorBits );
    if (hAndBits) DeleteObject32( hAndBits );
    GlobalUnlock16( hIcon );
    SetTextColor( hdc, oldFg );
    SetBkColor( hdc, oldBg );
    return TRUE;
}


/***********************************************************************
 *           DumpIcon    (USER.459)
 */
DWORD DumpIcon( SEGPTR pInfo, WORD *lpLen,
                SEGPTR *lpXorBits, SEGPTR *lpAndBits )
{
    CURSORICONINFO *info = PTR_SEG_TO_LIN( pInfo );
    int sizeAnd, sizeXor;

    if (!info) return 0;
    sizeXor = info->nHeight * info->nWidthBytes;
    sizeAnd = info->nHeight * BITMAP_WIDTH_BYTES( info->nWidth, 1 );
    if (lpAndBits) *lpAndBits = pInfo + sizeof(CURSORICONINFO);
    if (lpXorBits) *lpXorBits = pInfo + sizeof(CURSORICONINFO) + sizeAnd;
    if (lpLen) *lpLen = sizeof(CURSORICONINFO) + sizeAnd + sizeXor;
    return MAKELONG( sizeXor, sizeXor );
}


/***********************************************************************
 *           CURSORICON_SetCursor
 *
 * Change the X cursor. Helper function for SetCursor() and ShowCursor().
 */
static BOOL CURSORICON_SetCursor( HCURSOR16 hCursor )
{
    Pixmap pixmapBits, pixmapMask, pixmapAll;
    XColor fg, bg;
    Cursor cursor = None;

    if (!hCursor)  /* Create an empty cursor */
    {
        static const char data[] = { 0 };

        bg.red = bg.green = bg.blue = 0x0000;
        pixmapBits = XCreateBitmapFromData( display, rootWindow, data, 1, 1 );
        if (pixmapBits)
        {
            cursor = XCreatePixmapCursor( display, pixmapBits, pixmapBits,
                                          &bg, &bg, 0, 0 );
            XFreePixmap( display, pixmapBits );
        }
    }
    else  /* Create the X cursor from the bits */
    {
        CURSORICONINFO *ptr;
        XImage *image;

        if (!(ptr = (CURSORICONINFO*)GlobalLock16( hCursor ))) return FALSE;
        if (ptr->bPlanes * ptr->bBitsPerPixel != 1)
        {
            fprintf( stderr, "Cursor %04x has more than 1 bpp!\n", hCursor );
            return FALSE;
        }

        /* Create a pixmap and transfer all the bits to it */

        pixmapAll = XCreatePixmap( display, rootWindow,
                                   ptr->nWidth, ptr->nHeight * 2, 1 );
        image = XCreateImage( display, DefaultVisualOfScreen(screen),
                              1, ZPixmap, 0, (char *)(ptr + 1), ptr->nWidth,
                              ptr->nHeight * 2, 16, ptr->nWidthBytes);
        if (image)
        {
            extern void _XInitImageFuncPtrs( XImage* );
            image->byte_order = MSBFirst;
            image->bitmap_bit_order = MSBFirst;
            image->bitmap_unit = 16;
            _XInitImageFuncPtrs(image);
            if (pixmapAll)
                CallTo32_LargeStack( XPutImage, 10,
                                     display, pixmapAll, BITMAP_monoGC, image,
                                     0, 0, 0, 0, ptr->nWidth, ptr->nHeight*2 );
            image->data = NULL;
            XDestroyImage( image );
        }

        /* Now create the 2 pixmaps for bits and mask */

        pixmapBits = XCreatePixmap( display, rootWindow,
                                    ptr->nWidth, ptr->nHeight, 1 );
        pixmapMask = XCreatePixmap( display, rootWindow,
                                    ptr->nWidth, ptr->nHeight, 1 );

        /* Make sure everything went OK so far */

        if (pixmapBits && pixmapMask && pixmapAll)
        {
            /* We have to do some magic here, as cursors are not fully
             * compatible between Windows and X11. Under X11, there
             * are only 3 possible color cursor: black, white and
             * masked. So we map the 4th Windows color (invert the
             * bits on the screen) to black. This require some boolean
             * arithmetic:
             *
             *         Windows          |          X11
             * Xor    And      Result   |   Bits     Mask     Result
             *  0      0     black      |    0        1     background
             *  0      1     no change  |    X        0     no change
             *  1      0     white      |    1        1     foreground
             *  1      1     inverted   |    0        1     background
             *
             * which gives:
             *  Bits = 'Xor' and not 'And'
             *  Mask = 'Xor' or not 'And'
             *
             * FIXME: apparently some servers do support 'inverted' color.
             * I don't know if it's correct per the X spec, but maybe
             * we ought to take advantage of it.  -- AJ
             */
            XCopyArea( display, pixmapAll, pixmapBits, BITMAP_monoGC,
                       0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
            XCopyArea( display, pixmapAll, pixmapMask, BITMAP_monoGC,
                       0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, BITMAP_monoGC, GXandReverse );
            XCopyArea( display, pixmapAll, pixmapBits, BITMAP_monoGC,
                       0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, BITMAP_monoGC, GXorReverse );
            XCopyArea( display, pixmapAll, pixmapMask, BITMAP_monoGC,
                       0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
            XSetFunction( display, BITMAP_monoGC, GXcopy );
            fg.red = fg.green = fg.blue = 0xffff;
            bg.red = bg.green = bg.blue = 0x0000;
            cursor = XCreatePixmapCursor( display, pixmapBits, pixmapMask,
                                &fg, &bg, ptr->ptHotSpot.x, ptr->ptHotSpot.y );
        }

        /* Now free everything */

        if (pixmapAll) XFreePixmap( display, pixmapAll );
        if (pixmapBits) XFreePixmap( display, pixmapBits );
        if (pixmapMask) XFreePixmap( display, pixmapMask );
        GlobalUnlock16( hCursor );
    }

    if (cursor == None) return FALSE;
    if (CURSORICON_XCursor != None) XFreeCursor( display, CURSORICON_XCursor );
    CURSORICON_XCursor = cursor;

    if (rootWindow != DefaultRootWindow(display))
    {
        /* Set the cursor on the desktop window */
        XDefineCursor( display, rootWindow, cursor );
    }
    else
    {
        /* Set the same cursor for all top-level windows */
        HWND hwnd = GetWindow32( GetDesktopWindow32(), GW_CHILD );
        while(hwnd)
        {
            Window win = WIN_GetXWindow( hwnd );
            if (win) XDefineCursor( display, win, cursor );
            hwnd = GetWindow32( hwnd, GW_HWNDNEXT );
        }
    }
    return TRUE;
}


/***********************************************************************
 *           SetCursor    (USER.69)
 */
HCURSOR16 SetCursor( HCURSOR16 hCursor )
{
    HCURSOR16 hOldCursor;

    if (hCursor == hActiveCursor) return hActiveCursor;  /* No change */
    dprintf_cursor( stddeb, "SetCursor: %04x\n", hCursor );
    hOldCursor = hActiveCursor;
    hActiveCursor = hCursor;
    /* Change the cursor shape only if it is visible */
    if (CURSOR_ShowCount >= 0) CURSORICON_SetCursor( hActiveCursor );
    return hOldCursor;
}


/***********************************************************************
 *           SetCursorPos    (USER.70)
 */
void SetCursorPos( short x, short y )
{
    dprintf_cursor( stddeb, "SetCursorPos: x=%d y=%d\n", x, y );
    XWarpPointer( display, rootWindow, rootWindow, 0, 0, 0, 0, x, y );
}


/***********************************************************************
 *           ShowCursor    (USER.71)
 */
int ShowCursor( BOOL bShow )
{
    dprintf_cursor( stddeb, "ShowCursor: %d, count=%d\n",
                    bShow, CURSOR_ShowCount );

    if (bShow)
    {
        if (++CURSOR_ShowCount == 0)
            CURSORICON_SetCursor( hActiveCursor );  /* Show it */
    }
    else
    {
        if (--CURSOR_ShowCount == -1)
            CURSORICON_SetCursor( 0 );  /* Hide it */
    }
    return CURSOR_ShowCount;
}


/***********************************************************************
 *           GetCursor    (USER.247)
 */
HCURSOR16 GetCursor(void)
{
    return hActiveCursor;
}


/***********************************************************************
 *           ClipCursor16    (USER.16)
 */
BOOL16 ClipCursor16( const RECT16 *rect )
{
    if (!rect) SetRectEmpty32( &CURSOR_ClipRect );
    else CONV_RECT16TO32( rect, &CURSOR_ClipRect );
    return TRUE;
}


/***********************************************************************
 *           ClipCursor32    (USER32.52)
 */
BOOL32 ClipCursor32( const RECT32 *rect )
{
    if (!rect) SetRectEmpty32( &CURSOR_ClipRect );
    else CopyRect32( &CURSOR_ClipRect, rect );
    return TRUE;
}


/***********************************************************************
 *           GetCursorPos16    (USER.17)
 */
void GetCursorPos16( POINT16 *pt )
{
    Window root, child;
    int rootX, rootY, childX, childY;
    unsigned int mousebut;

    if (!pt) return;
    if (!XQueryPointer( display, rootWindow, &root, &child,
		        &rootX, &rootY, &childX, &childY, &mousebut ))
	pt->x = pt->y = 0;
    else
    {
	pt->x = childX;
	pt->y = childY;
    }
    dprintf_cursor(stddeb, "GetCursorPos: ret=%d,%d\n", pt->x, pt->y );
}


/***********************************************************************
 *           GetCursorPos32    (USER32.228)
 */
void GetCursorPos32( POINT32 *pt )
{
    POINT16 pt16;
    GetCursorPos16( &pt16 );
    if (pt) CONV_POINT16TO32( &pt16, pt );
}


/***********************************************************************
 *           GetClipCursor16    (USER.309)
 */
void GetClipCursor16( RECT16 *rect )
{
    if (rect) CONV_RECT32TO16( &CURSOR_ClipRect, rect );
}


/***********************************************************************
 *           GetClipCursor32    (USER32.220)
 */
void GetClipCursor32( RECT32 *rect )
{
    if (rect) CopyRect32( rect, &CURSOR_ClipRect );
}


/**********************************************************************
 *	    GetIconID    (USER.455)
 */
WORD GetIconID( HGLOBAL16 hResource, DWORD resType )
{
    CURSORICONDIR *lpDir = (CURSORICONDIR *)GlobalLock16(hResource);
/* LockResource16(hResource); */

    if (!lpDir || lpDir->idReserved ||
        ((lpDir->idType != 1) && (lpDir->idType != 2)))
    {
        dprintf_cursor(stddeb,"GetIconID: invalid resource directory\n");
        return 0;
    }

    dprintf_cursor( stddeb, "GetIconID: hRes=%04x, entries=%i\n",
                    hResource, lpDir->idCount );

    switch(resType)
    {
    case 1:  /* cursor */
        {
            CURSORDIRENTRY *entry = CURSORICON_FindBestCursor( lpDir,
                                    SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR );
            return entry ? entry->wResId : 0;
        }
    case 3:  /* icon */
        {
            ICONDIRENTRY * entry =  CURSORICON_FindBestIcon( lpDir,
                                    SYSMETRICS_CXICON, SYSMETRICS_CYICON,
                                    MIN( 16, COLOR_GetSystemPaletteSize() ) );
            return entry ? entry->wResId : 0;
        }
    }
    fprintf( stderr, "GetIconID: invalid res type %ld\n", resType );
    return 0;
}


/**********************************************************************
 *	    LoadIconHandler    (USER.456)
 */
HICON16 LoadIconHandler( HGLOBAL16 hResource, BOOL bNew )
{
    dprintf_cursor(stddeb,"LoadIconHandler: hRes=%04x\n",hResource);

    if( !bNew )
      {
	fprintf(stdnimp,"LoadIconHandler: 2.xx resources are not supported\n");
        return 0;
      }
    return CURSORICON_LoadHandler( hResource, 0, FALSE);
}
