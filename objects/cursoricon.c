/*
 * Cursor and icon support
 *
 * Copyright 1995 Alexandre Julliard
 *           1996 Martin Von Loewis
 *           1997 Alex Korobka
 */

/*
 * Theory:
 *
 * http://www.microsoft.com/win32dev/ui/icons.htm
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
#include "heap.h"
#include "windows.h"
#include "peexe.h"
#include "color.h"
#include "bitmap.h"
#include "callback.h"
#include "cursoricon.h"
#include "sysmetrics.h"
#include "module.h"
#include "win.h"
#include "debug.h"
#include "task.h"
#include "user.h"
#include "keyboard.h"

extern UINT16 COLOR_GetSystemPaletteSize();

Cursor CURSORICON_XCursor = None;    /* Current X cursor */
static HCURSOR32 hActiveCursor = 0;  /* Active cursor */
static INT32 CURSOR_ShowCount = 0;   /* Cursor display count */
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
 *	    CURSORICON_LoadDirEntry16
 *
 * Load the icon/cursor directory for a given resource name and find the
 * best matching entry.
 */
static BOOL32 CURSORICON_LoadDirEntry16( HINSTANCE32 hInstance, SEGPTR name,
                                         INT32 width, INT32 height, INT32 colors, 
					 BOOL32 fCursor, CURSORICONDIRENTRY *dirEntry )
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
 *          CURSORICON_LoadDirEntry32
 *
 * Load the icon/cursor directory for a given resource name and find the
 * best matching entry.
 */
static BOOL32 CURSORICON_LoadDirEntry32( HINSTANCE32 hInstance, LPCWSTR name,
                                         INT32 width, INT32 height, INT32 colors,
                                         BOOL32 fCursor, CURSORICONDIRENTRY *dirEntry )
{
    HANDLE32 hRsrc;
    HANDLE32 hMem;
    CURSORICONDIR *dir;
    CURSORICONDIRENTRY *entry = NULL;

    if (!(hRsrc = FindResource32W( hInstance, name,
                (LPCWSTR)(fCursor ? RT_GROUP_CURSOR : RT_GROUP_ICON) )))
        return FALSE;
    if (!(hMem = LoadResource32( hInstance, hRsrc ))) return FALSE;
    if ((dir = (CURSORICONDIR*)LockResource32( hMem )))
    {
        if (fCursor)
            entry = (CURSORICONDIRENTRY *)CURSORICON_FindBestCursor( dir,
                                                               width, height );
        else
            entry = (CURSORICONDIRENTRY *)CURSORICON_FindBestIcon( dir,
                                                       width, height, colors );
        if (entry) *dirEntry = *entry;
    }
    FreeResource32( hMem );
    return (entry != NULL);
}


/**********************************************************************
 *	    CURSORICON_CreateFromResource
 *
 * Create a cursor or icon from in-memory resource template. 
 *
 * FIXME: Adjust icon size when width and height are nonzero (stretchblt).
 *        Convert to mono when cFlag is LR_MONOCHROME. Do something
 *        with cbSize parameter as well.
 */
static HGLOBAL16 CURSORICON_CreateFromResource( HINSTANCE32 hInstance, HGLOBAL16 hObj, LPBYTE bits,
	 					UINT32 cbSize, BOOL32 bIcon, DWORD dwVersion, 
						INT32 width, INT32 height, UINT32 cFlag )
{
    int sizeAnd, sizeXor;
    HBITMAP32 hAndBits = 0, hXorBits = 0; /* error condition for later */
    BITMAPOBJ *bmpXor, *bmpAnd;
    POINT16 hotspot = { 0 ,0 };
    BITMAPINFO *bmi;
    HDC32 hdc;

    TRACE(cursor,"%08x (%u bytes), ver %08x, %ix%i %s %s\n",
                        (unsigned)bits, cbSize, (unsigned)dwVersion, width, height,
                                  bIcon ? "icon" : "cursor", cFlag ? "mono" : "" );
    if (dwVersion == 0x00020000)
    {
	fprintf(stdnimp,"\t2.xx resources are not supported\n");
	return 0;
    }

    if (bIcon)
	bmi = (BITMAPINFO *)bits;
    else /* get the hotspot */
    {
        POINT16 *pt = (POINT16 *)bits;
        hotspot = *pt;
        bmi = (BITMAPINFO *)(pt + 1);
    }

    /* Check bitmap header */

    if ( (bmi->bmiHeader.biSize != sizeof(BITMAPCOREHEADER)) &&
	 (bmi->bmiHeader.biSize != sizeof(BITMAPINFOHEADER)  ||
	  bmi->bmiHeader.biCompression != BI_RGB) )
    {
          fprintf(stderr,"\tinvalid resource bitmap header.\n");
          return 0;
    }

    if( (hdc = GetDC32( 0 )) )
    {
	BITMAPINFO* pInfo;
        INT32 size = DIB_BitmapInfoSize( bmi, DIB_RGB_COLORS );

        /* Make sure we have room for the monochrome bitmap later on.
	 * Note that BITMAPINFOINFO and BITMAPCOREHEADER are the same
	 * up to and including the biBitCount. In-memory icon resource 
	 * format is as follows:
	 *
	 *   BITMAPINFOHEADER   icHeader  // DIB header
	 *   RGBQUAD         icColors[]   // Color table
  	 *   BYTE            icXOR[]      // DIB bits for XOR mask
	 *   BYTE            icAND[]      // DIB bits for AND mask
	 */

    	if( (pInfo = (BITMAPINFO *)HeapAlloc( GetProcessHeap(), 0, 
			  MAX(size, sizeof(BITMAPINFOHEADER) + 2*sizeof(RGBQUAD)))) )
	{	
	    memcpy( pInfo, bmi, size );	
	    pInfo->bmiHeader.biHeight /= 2;

	    /* Create the XOR bitmap */

	    hXorBits = CreateDIBitmap32( hdc, &pInfo->bmiHeader, CBM_INIT,
                                    (char*)bmi + size, pInfo, DIB_RGB_COLORS );
	    if( hXorBits )
	    {
		char* bits = (char *)bmi + size + bmi->bmiHeader.biHeight *
				DIB_GetDIBWidthBytes(bmi->bmiHeader.biWidth,
						     bmi->bmiHeader.biBitCount) / 2;

		pInfo->bmiHeader.biBitCount = 1;
	        if (pInfo->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
	        {
	            RGBQUAD *rgb = pInfo->bmiColors;

	            pInfo->bmiHeader.biClrUsed = pInfo->bmiHeader.biClrImportant = 2;
	            rgb[0].rgbBlue = rgb[0].rgbGreen = rgb[0].rgbRed = 0x00;
	            rgb[1].rgbBlue = rgb[1].rgbGreen = rgb[1].rgbRed = 0xff;
	            rgb[0].rgbReserved = rgb[1].rgbReserved = 0;
	        }
	        else
	        {
	            RGBTRIPLE *rgb = (RGBTRIPLE *)(((BITMAPCOREHEADER *)pInfo) + 1);

	            rgb[0].rgbtBlue = rgb[0].rgbtGreen = rgb[0].rgbtRed = 0x00;
	            rgb[1].rgbtBlue = rgb[1].rgbtGreen = rgb[1].rgbtRed = 0xff;
	        }

	        /* Create the AND bitmap */

	        hAndBits = CreateDIBitmap32( hdc, &pInfo->bmiHeader, CBM_INIT,
	                                     bits, pInfo, DIB_RGB_COLORS );
		if( !hAndBits ) DeleteObject32( hXorBits );
	    }
	    HeapFree( GetProcessHeap(), 0, pInfo ); 
	}
	ReleaseDC32( 0, hdc );
    }

    if( !hXorBits || !hAndBits ) 
    {
	fprintf(stderr,"\tunable to create an icon bitmap.\n");
	return 0;
    }

    /* Now create the CURSORICONINFO structure */

    bmpXor = (BITMAPOBJ *) GDI_GetObjPtr( hXorBits, BITMAP_MAGIC );
    bmpAnd = (BITMAPOBJ *) GDI_GetObjPtr( hAndBits, BITMAP_MAGIC );
    sizeXor = bmpXor->bitmap.bmHeight * bmpXor->bitmap.bmWidthBytes;
    sizeAnd = bmpAnd->bitmap.bmHeight * bmpAnd->bitmap.bmWidthBytes;

    if (hObj) hObj = GlobalReAlloc16( hObj, 
		     sizeof(CURSORICONINFO) + sizeXor + sizeAnd, GMEM_MOVEABLE );
    if (!hObj) hObj = GlobalAlloc16( GMEM_MOVEABLE, 
		     sizeof(CURSORICONINFO) + sizeXor + sizeAnd );
    if (hObj)
    {
	CURSORICONINFO *info;

	/* Make it owned by the module */
	if (hInstance) FarSetOwner( hObj, MODULE_HANDLEtoHMODULE16(hInstance));

	info = (CURSORICONINFO *)GlobalLock16( hObj );
	info->ptHotSpot.x   = hotspot.x;
	info->ptHotSpot.y   = hotspot.y;
	info->nWidth        = bmpXor->bitmap.bmWidth;
	info->nHeight       = bmpXor->bitmap.bmHeight;
	info->nWidthBytes   = bmpXor->bitmap.bmWidthBytes;
	info->bPlanes       = bmpXor->bitmap.bmPlanes;
	info->bBitsPerPixel = bmpXor->bitmap.bmBitsPixel;

	/* Transfer the bitmap bits to the CURSORICONINFO structure */

	GetBitmapBits32( hAndBits, sizeAnd, (char *)(info + 1) );
	GetBitmapBits32( hXorBits, sizeXor, (char *)(info + 1) + sizeAnd );
	GlobalUnlock16( hObj );
    }

    DeleteObject32( hXorBits );
    DeleteObject32( hAndBits );
    return hObj;
}


/**********************************************************************
 *          CreateIconFromResourceEx16          (USER.450)
 *
 * FIXME: not sure about exact parameter types
 */
HICON16 WINAPI CreateIconFromResourceEx16( LPBYTE bits, UINT16 cbSize, BOOL16 bIcon,
                                    DWORD dwVersion, INT16 width, INT16 height, UINT16 cFlag )
{
    TDB* pTask = (TDB*)GlobalLock16( GetCurrentTask() );
    if( pTask )
	return CURSORICON_CreateFromResource( pTask->hInstance, 0, bits, cbSize, bIcon, dwVersion,
					      width, height, cFlag );
    return 0;
}


/**********************************************************************
 *          CreateIconFromResource          (USER32.???)
 * FIXME:
 *  	bon@elektron.ikp.physik.tu-darmstadt.de 971130: Test with weditres
 *	showed only blank layout. Couldn't determine if this is a problem
 *	with CreateIconFromResource32 or the application. The application
 *	windows behaves strange (no redraw) before CreateIconFromResource32
 */
HICON32 WINAPI CreateIconFromResource32( LPBYTE bits, UINT32 cbSize,
                                           BOOL32 bIcon, DWORD dwVersion)
{
    HICON32 ret;
    ret = CreateIconFromResourceEx16( bits, cbSize, bIcon, dwVersion, 0,0,0);
    FIXME(icon,"probably only a stub\n");
    TRACE(icon, "%s at %p size %d winver %d return 0x%04x\n",
                 (bIcon)?"Icon":"Cursor",bits,cbSize,bIcon,ret);
    return ret;
}


/**********************************************************************
 *          CreateIconFromResourceEx32          (USER32.76)
 */
HICON32 WINAPI CreateIconFromResourceEx32( LPBYTE bits, UINT32 cbSize,
                                           BOOL32 bIcon, DWORD dwVersion,
                                           INT32 width, INT32 height,
                                           UINT32 cFlag )
{
    return CreateIconFromResourceEx16( bits, cbSize, bIcon, dwVersion, width, height, cFlag );
}


/**********************************************************************
 *	    CURSORICON_Load16
 *
 * Load a cursor or icon from a 16-bit resource.
 */
static HGLOBAL16 CURSORICON_Load16( HINSTANCE16 hInstance, SEGPTR name,
                                    INT32 width, INT32 height, INT32 colors,
                                    BOOL32 fCursor )
{
    HGLOBAL16 handle;
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

    if ( !CURSORICON_LoadDirEntry16( hInstance, name, width, height,
                                    colors, fCursor, &dirEntry ) )  return 0;
    /* Load the resource */

    if ( (hRsrc = FindResource16( hInstance,
                                MAKEINTRESOURCE( dirEntry.icon.wResId ),
                                fCursor ? RT_CURSOR : RT_ICON )) )
    {
	/* 16-bit icon or cursor resources are processed
	 * transparently by the LoadResource16() via custom
	 * resource handlers set by SetResourceHandler().
	 */

	if ( (handle = LoadResource16( hInstance, hRsrc )) )
	    return handle;
    }
    return 0;
}

/**********************************************************************
 *          CURSORICON_Load32
 *
 * Load a cursor or icon from a 32-bit resource.
 */
static HGLOBAL32 CURSORICON_Load32( HINSTANCE32 hInstance, LPCWSTR name,
                                    int width, int height, int colors,
                                    BOOL32 fCursor )
{
    HANDLE32 handle;
    HANDLE32 hRsrc;
    CURSORICONDIRENTRY dirEntry;

    if(!hInstance)  /* OEM cursor/icon */
    {
	WORD resid;
	if(HIWORD(name))
	{
            LPSTR ansi = HEAP_strdupWtoA(GetProcessHeap(),0,name);
            if( ansi[0]=='#')        /*Check for '#xxx' name */
            {
                resid = atoi(ansi+1);
                HeapFree( GetProcessHeap(), 0, ansi );
            }
            else
            {
                HeapFree( GetProcessHeap(), 0, ansi );
                return 0;
            }
        }
        else resid = LOWORD(name);
        return OBM_LoadCursorIcon(resid, fCursor);
    }

    /* Find the best entry in the directory */
 
    if ( !CURSORICON_LoadDirEntry32( hInstance, name, width, height,
				    colors, fCursor, &dirEntry ) )  return 0;
    /* Load the resource */

    if ( (hRsrc = FindResource32W( hInstance,
                      (LPWSTR) (DWORD) dirEntry.icon.wResId,
                      (LPWSTR) (fCursor ? RT_CURSOR : RT_ICON ))) )
    {
	HANDLE32 h = 0;
	if ( (handle = LoadResource32( hInstance, hRsrc )) )
	{
	    /* Hack to keep LoadCursor/Icon32() from spawning multiple
	     * copies of the same object.
	     */
#define pRsrcEntry ((LPIMAGE_RESOURCE_DATA_ENTRY)hRsrc)
	    if( !pRsrcEntry->ResourceHandle ) 
	    {
		LPBYTE bits = (LPBYTE)LockResource32( handle );
		h = CURSORICON_CreateFromResource( hInstance, 0, bits, dirEntry.icon.dwBytesInRes, 
					!fCursor, 0x00030000, width, height, LR_DEFAULTCOLOR );
		pRsrcEntry->ResourceHandle = h;
	    }
	    else h = pRsrcEntry->ResourceHandle;
#undef  pRsrcEntry
	}
	return h;
    }
    return 0;
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
 * Converts bitmap to mono and truncates if icon is too large (should
 * probably do StretchBlt() instead).
 */
HCURSOR16 CURSORICON_IconToCursor(HICON16 hIcon, BOOL32 bSemiTransparent)
{
 HCURSOR16       hRet = 0;
 CURSORICONINFO *pIcon = NULL;
 HTASK16 	 hTask = GetCurrentTask();
 TDB*  		 pTask = (TDB *)GlobalLock16(hTask);

 if(hIcon && pTask)
    if (!(pIcon = (CURSORICONINFO*)GlobalLock16( hIcon ))) return FALSE;
       if (pIcon->bPlanes * pIcon->bBitsPerPixel == 1)
           hRet = CURSORICON_Copy( pTask->hInstance, hIcon );
       else
       {
           BYTE  pAndBits[128];
           BYTE  pXorBits[128];
	   int   maxx, maxy, ix, iy, bpp = pIcon->bBitsPerPixel;
           BYTE* psPtr, *pxbPtr = pXorBits;
           unsigned xor_width, and_width, val_base = 0xffffffff >> (32 - bpp);
           BYTE* pbc = NULL;

           COLORREF       col;
           CURSORICONINFO cI;

	   TRACE(icon, "[%04x] %ix%i %ibpp (bogus %ibps)\n", 
		hIcon, pIcon->nWidth, pIcon->nHeight, pIcon->bBitsPerPixel, pIcon->nWidthBytes );

	   xor_width = BITMAP_GetBitsWidth( pIcon->nWidth, bpp );
	   and_width =  BITMAP_GetBitsWidth( pIcon->nWidth, 1 );
	   psPtr = (BYTE *)(pIcon + 1) + pIcon->nHeight * and_width;

           memset(pXorBits, 0, 128);
           cI.bBitsPerPixel = 1; cI.bPlanes = 1;
           cI.ptHotSpot.x = cI.ptHotSpot.y = 15;
           cI.nWidth = 32; cI.nHeight = 32;
           cI.nWidthBytes = 4;	/* 32x1bpp */

           maxx = (pIcon->nWidth > 32) ? 32 : pIcon->nWidth;
           maxy = (pIcon->nHeight > 32) ? 32 : pIcon->nHeight;

           for( iy = 0; iy < maxy; iy++ )
           {
	      unsigned shift = iy % 2; 

              memcpy( pAndBits + iy * 4, (BYTE *)(pIcon + 1) + iy * and_width, 
					 (and_width > 4) ? 4 : and_width );
              for( ix = 0; ix < maxx; ix++ )
              {
                if( bSemiTransparent && ((ix+shift)%2) )
                {
		    /* set AND bit, XOR bit stays 0 */

                    pbc = pAndBits + iy * 4 + ix/8;
                   *pbc |= 0x80 >> (ix%8);
                }
                else
                {
		    /* keep AND bit, set XOR bit */

		  unsigned *psc = (unsigned*)(psPtr + (ix * bpp)/8);
                  unsigned  val = ((*psc) >> (ix * bpp)%8) & val_base;
                  col = COLOR_ToLogical(val);
		  if( (GetRValue(col) + GetGValue(col) + GetBValue(col)) > 0x180 )
                  {
                    pbc = pxbPtr + ix/8;
                   *pbc |= 0x80 >> (ix%8);
                  }
                }
              }
              psPtr += xor_width;
              pxbPtr += 4;
           }

           hRet = CreateCursorIconIndirect( pTask->hInstance , &cI, pAndBits, pXorBits);

           if( !hRet ) /* fall back on default drag cursor */
                hRet = CURSORICON_Copy( pTask->hInstance ,
                              CURSORICON_Load16(0,MAKEINTRESOURCE(OCR_DRAGOBJECT),
                                         SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR, 1, TRUE) );
       }

 return hRet;
}


/***********************************************************************
 *           LoadCursor16    (USER.173)
 */
HCURSOR16 WINAPI LoadCursor16( HINSTANCE16 hInstance, SEGPTR name )
{
    if (HIWORD(name))
        TRACE(cursor, "%04x '%s'\n",
                        hInstance, (char *)PTR_SEG_TO_LIN( name ) );
    else
        TRACE(cursor, "%04x %04x\n",
                        hInstance, LOWORD(name) );

    return CURSORICON_Load16( hInstance, name,
                              SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR, 1, TRUE);
}


/***********************************************************************
 *           LoadIcon16    (USER.174)
 */
HICON16 WINAPI LoadIcon16( HINSTANCE16 hInstance, SEGPTR name )
{
    if (HIWORD(name))
        TRACE(icon, "%04x '%s'\n",
                      hInstance, (char *)PTR_SEG_TO_LIN( name ) );
    else
        TRACE(icon, "%04x %04x\n",
                      hInstance, LOWORD(name) );

    return CURSORICON_Load16( hInstance, name,
                              SYSMETRICS_CXICON, SYSMETRICS_CYICON,
                              MIN( 16, COLOR_GetSystemPaletteSize() ), FALSE );
}


/***********************************************************************
 *           CreateCursor16    (USER.406)
 */
HCURSOR16 WINAPI CreateCursor16( HINSTANCE16 hInstance,
                                 INT16 xHotSpot, INT16 yHotSpot,
                                 INT16 nWidth, INT16 nHeight,
                                 LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info = { { xHotSpot, yHotSpot }, nWidth, nHeight, 0, 1, 1 };

    TRACE(cursor, "%dx%d spot=%d,%d xor=%p and=%p\n",
                    nWidth, nHeight, xHotSpot, yHotSpot, lpXORbits, lpANDbits);
    return CreateCursorIconIndirect( hInstance, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateCursor32    (USER32.66)
 */
HCURSOR32 WINAPI CreateCursor32( HINSTANCE32 hInstance,
                                 INT32 xHotSpot, INT32 yHotSpot,
                                 INT32 nWidth, INT32 nHeight,
                                 LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info = { { xHotSpot, yHotSpot }, nWidth, nHeight, 0, 1, 1 };

    TRACE(cursor, "%dx%d spot=%d,%d xor=%p and=%p\n",
                    nWidth, nHeight, xHotSpot, yHotSpot, lpXORbits, lpANDbits);
    return CreateCursorIconIndirect( MODULE_HANDLEtoHMODULE16( hInstance ),
                                     &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateIcon16    (USER.407)
 */
HICON16 WINAPI CreateIcon16( HINSTANCE16 hInstance, INT16 nWidth,
                             INT16 nHeight, BYTE bPlanes, BYTE bBitsPixel,
                             LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info = { { 0, 0 }, nWidth, nHeight, 0, bPlanes, bBitsPixel};

    TRACE(icon, "%dx%dx%d, xor=%p, and=%p\n",
                  nWidth, nHeight, bPlanes * bBitsPixel, lpXORbits, lpANDbits);
    return CreateCursorIconIndirect( hInstance, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateIcon32    (USER32.74)
 */
HICON32 WINAPI CreateIcon32( HINSTANCE32 hInstance, INT32 nWidth,
                             INT32 nHeight, BYTE bPlanes, BYTE bBitsPixel,
                             LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info = { { 0, 0 }, nWidth, nHeight, 0, bPlanes, bBitsPixel};

    TRACE(icon, "%dx%dx%d, xor=%p, and=%p\n",
                  nWidth, nHeight, bPlanes * bBitsPixel, lpXORbits, lpANDbits);
    return CreateCursorIconIndirect( MODULE_HANDLEtoHMODULE16( hInstance ),
                                     &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateCursorIconIndirect    (USER.408)
 */
HGLOBAL16 WINAPI CreateCursorIconIndirect( HINSTANCE16 hInstance,
                                           CURSORICONINFO *info,
                                           LPCVOID lpANDbits,
                                           LPCVOID lpXORbits )
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
HICON16 WINAPI CopyIcon16( HINSTANCE16 hInstance, HICON16 hIcon )
{
    TRACE(icon, "%04x %04x\n", hInstance, hIcon );
    return CURSORICON_Copy( hInstance, hIcon );
}


/***********************************************************************
 *           CopyIcon32    (USER32.59)
 */
HICON32 WINAPI CopyIcon32( HICON32 hIcon )
{
  HTASK16 hTask = GetCurrentTask ();
  TDB* pTask = (TDB *) GlobalLock16 (hTask);
    TRACE(icon, "%04x\n", hIcon );
  return CURSORICON_Copy( pTask->hInstance, hIcon );
}


/***********************************************************************
 *           CopyCursor16    (USER.369)
 */
HCURSOR16 WINAPI CopyCursor16( HINSTANCE16 hInstance, HCURSOR16 hCursor )
{
    TRACE(cursor, "%04x %04x\n", hInstance, hCursor );
    return CURSORICON_Copy( hInstance, hCursor );
}


/***********************************************************************
 *           DestroyIcon16    (USER.457)
 */
BOOL16 WINAPI DestroyIcon16( HICON16 hIcon )
{
    return DestroyIcon32( hIcon );
}


/***********************************************************************
 *           DestroyIcon32    (USER32.132)
 */
BOOL32 WINAPI DestroyIcon32( HICON32 hIcon )
{
    TRACE(icon, "%04x\n", hIcon );
    /* FIXME: should check for OEM icon here */
    return (FreeResource16( hIcon ) == 0);
}


/***********************************************************************
 *           DestroyCursor16    (USER.458)
 */
BOOL16 WINAPI DestroyCursor16( HCURSOR16 hCursor )
{
    return DestroyCursor32( hCursor );
}


/***********************************************************************
 *           DestroyCursor32    (USER32.131)
 */
BOOL32 WINAPI DestroyCursor32( HCURSOR32 hCursor )
{
    TRACE(cursor, "%04x\n", hCursor );
    /* FIXME: should check for OEM cursor here */
    return (FreeResource16( hCursor ) == 0);
}


/***********************************************************************
 *           DrawIcon16    (USER.84)
 */
BOOL16 WINAPI DrawIcon16( HDC16 hdc, INT16 x, INT16 y, HICON16 hIcon )
{
    return DrawIcon32( hdc, x, y, hIcon );
}


/***********************************************************************
 *           DrawIcon32    (USER32.158)
 */
BOOL32 WINAPI DrawIcon32( HDC32 hdc, INT32 x, INT32 y, HICON32 hIcon )
{
    CURSORICONINFO *ptr;
    HDC32 hMemDC;
    HBITMAP32 hXorBits, hAndBits;
    COLORREF oldFg, oldBg;

    if (!(ptr = (CURSORICONINFO *)GlobalLock16( hIcon ))) return FALSE;
    if (!(hMemDC = CreateCompatibleDC32( hdc ))) return FALSE;
    hAndBits = CreateBitmap32( ptr->nWidth, ptr->nHeight, 1, 1,
                               (char *)(ptr+1) );
    hXorBits = CreateBitmap32( ptr->nWidth, ptr->nHeight, ptr->bPlanes,
                               ptr->bBitsPerPixel, (char *)(ptr + 1)
                         + ptr->nHeight * BITMAP_WIDTH_BYTES(ptr->nWidth,1) );
    oldFg = SetTextColor32( hdc, RGB(0,0,0) );
    oldBg = SetBkColor32( hdc, RGB(255,255,255) );

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
    SetTextColor32( hdc, oldFg );
    SetBkColor32( hdc, oldBg );
    return TRUE;
}


/***********************************************************************
 *           DumpIcon    (USER.459)
 */
DWORD WINAPI DumpIcon( SEGPTR pInfo, WORD *lpLen,
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
 * The Xlib critical section must be entered before calling this function.
 */
static BOOL32 CURSORICON_SetCursor( HCURSOR16 hCursor )
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

	/* NOTE: Following hack works, but only because XFree depth
	 * 	 1 images really use 1 bit/pixel (and so the same layout
	 *	 as the Windows cursor data). Perhaps use a more generic
	 *	 algorithm here.
	 */
        pixmapAll = XCreatePixmap( display, rootWindow,
                                   ptr->nWidth, ptr->nHeight * 2, 1 );
        image = XCreateImage( display, DefaultVisualOfScreen(screen),
                              1, ZPixmap, 0, (char *)(ptr + 1), ptr->nWidth,
                              ptr->nHeight * 2, 16, ptr->nWidthBytes);
        if (image)
        {
            image->byte_order = MSBFirst;
            image->bitmap_bit_order = MSBFirst;
            image->bitmap_unit = 16;
            _XInitImageFuncPtrs(image);
            if (pixmapAll)
                XPutImage( display, pixmapAll, BITMAP_monoGC, image,
                           0, 0, 0, 0, ptr->nWidth, ptr->nHeight * 2 );
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
        HWND32 hwnd = GetWindow32( GetDesktopWindow32(), GW_CHILD );
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
 *           SetCursor16    (USER.69)
 */
HCURSOR16 WINAPI SetCursor16( HCURSOR16 hCursor )
{
    return (HCURSOR16)SetCursor32( hCursor );
}


/***********************************************************************
 *           SetCursor32    (USER32.471)
 * RETURNS:
 *	A handle to the previous cursor shape.
 */
HCURSOR32 WINAPI SetCursor32(
	         HCURSOR32 hCursor /* Handle of cursor to show */
) {
    HCURSOR32 hOldCursor;

    if (hCursor == hActiveCursor) return hActiveCursor;  /* No change */
    TRACE(cursor, "%04x\n", hCursor );
    hOldCursor = hActiveCursor;
    hActiveCursor = hCursor;
    /* Change the cursor shape only if it is visible */
    if (CURSOR_ShowCount >= 0)
    {
        EnterCriticalSection( &X11DRV_CritSection );
        CALL_LARGE_STACK( CURSORICON_SetCursor, hActiveCursor );
        LeaveCriticalSection( &X11DRV_CritSection );
    }
    return hOldCursor;
}


/***********************************************************************
 *           SetCursorPos16    (USER.70)
 */
void WINAPI SetCursorPos16( INT16 x, INT16 y )
{
    SetCursorPos32( x, y );
}


/***********************************************************************
 *           SetCursorPos32    (USER32.473)
 */
BOOL32 WINAPI SetCursorPos32( INT32 x, INT32 y )
{
    TRACE(cursor, "x=%d y=%d\n", x, y );
    TSXWarpPointer( display, rootWindow, rootWindow, 0, 0, 0, 0, x, y );
    return TRUE;
}


/***********************************************************************
 *           ShowCursor16    (USER.71)
 */
INT16 WINAPI ShowCursor16( BOOL16 bShow )
{
    return ShowCursor32( bShow );
}


/***********************************************************************
 *           ShowCursor32    (USER32.529)
 */
INT32 WINAPI ShowCursor32( BOOL32 bShow )
{
    TRACE(cursor, "%d, count=%d\n",
                    bShow, CURSOR_ShowCount );

    EnterCriticalSection( &X11DRV_CritSection );
    if (bShow)
    {
        if (++CURSOR_ShowCount == 0)  /* Show it */
            CALL_LARGE_STACK( CURSORICON_SetCursor, hActiveCursor );
    }
    else
    {
        if (--CURSOR_ShowCount == -1)  /* Hide it */
            CALL_LARGE_STACK( CURSORICON_SetCursor, 0 );
    }
    LeaveCriticalSection( &X11DRV_CritSection );
    return CURSOR_ShowCount;
}


/***********************************************************************
 *           GetCursor16    (USER.247)
 */
HCURSOR16 WINAPI GetCursor16(void)
{
    return hActiveCursor;
}


/***********************************************************************
 *           GetCursor32    (USER32.226)
 */
HCURSOR32 WINAPI GetCursor32(void)
{
    return hActiveCursor;
}


/***********************************************************************
 *           ClipCursor16    (USER.16)
 */
BOOL16 WINAPI ClipCursor16( const RECT16 *rect )
{
    if (!rect) SetRectEmpty32( &CURSOR_ClipRect );
    else CONV_RECT16TO32( rect, &CURSOR_ClipRect );
    return TRUE;
}


/***********************************************************************
 *           ClipCursor32    (USER32.52)
 */
BOOL32 WINAPI ClipCursor32( const RECT32 *rect )
{
    if (!rect) SetRectEmpty32( &CURSOR_ClipRect );
    else CopyRect32( &CURSOR_ClipRect, rect );
    return TRUE;
}


/***********************************************************************
 *           GetCursorPos16    (USER.17)
 */
void WINAPI GetCursorPos16( POINT16 *pt )
{
    Window root, child;
    int rootX, rootY, childX, childY;
    unsigned int mousebut;

    if (!pt) return;
    if (!TSXQueryPointer( display, rootWindow, &root, &child,
		        &rootX, &rootY, &childX, &childY, &mousebut ))
	pt->x = pt->y = 0;
    else
    {
	pt->x = childX;
	pt->y = childY;
        if (mousebut & Button1Mask)
            AsyncMouseButtonsStates[0] = MouseButtonsStates[0] = TRUE;
        else
            MouseButtonsStates[0] = FALSE;
        if (mousebut & Button2Mask)
            AsyncMouseButtonsStates[1] = MouseButtonsStates[1] = TRUE;
        else       
            MouseButtonsStates[1] = FALSE;
        if (mousebut & Button3Mask)
            AsyncMouseButtonsStates[2] = MouseButtonsStates[2] = TRUE;
        else
            MouseButtonsStates[2] = FALSE;
    }
    TRACE(cursor, "ret=%d,%d\n", pt->x, pt->y );
}


/***********************************************************************
 *           GetCursorPos32    (USER32.228)
 */
void WINAPI GetCursorPos32( POINT32 *pt )
{
    POINT16 pt16;
    GetCursorPos16( &pt16 );
    if (pt) CONV_POINT16TO32( &pt16, pt );
}


/***********************************************************************
 *           GetClipCursor16    (USER.309)
 */
void WINAPI GetClipCursor16( RECT16 *rect )
{
    if (rect) CONV_RECT32TO16( &CURSOR_ClipRect, rect );
}


/***********************************************************************
 *           GetClipCursor32    (USER32.220)
 */
void WINAPI GetClipCursor32( RECT32 *rect )
{
    if (rect) CopyRect32( rect, &CURSOR_ClipRect );
}

/**********************************************************************
 *          LookupIconIdFromDirectoryEx16	(USER.364)
 *
 * FIXME: exact parameter sizes
 */
INT16 WINAPI LookupIconIdFromDirectoryEx16( LPBYTE xdir, BOOL16 bIcon,
	     INT16 width, INT16 height, UINT16 cFlag )
{
    CURSORICONDIR	*dir = (CURSORICONDIR*)xdir;
    UINT16 retVal = 0;
    if( dir && !dir->idReserved && (dir->idType & 3) )
    {
	int colors = (cFlag == LR_MONOCHROME) ? 2 : COLOR_GetSystemPaletteSize();
	if( bIcon )
	{
	    ICONDIRENTRY* entry;
	    entry = CURSORICON_FindBestIcon( dir, width, height, colors );
	    if( entry ) retVal = entry->wResId;
	}
	else
	{
	    CURSORDIRENTRY* entry;
	    entry = CURSORICON_FindBestCursor( dir, width, height );
	    if( entry ) retVal = entry->wResId;
	}
    }
    else WARN(cursor, "invalid resource directory\n");
    return retVal;
}

/**********************************************************************
 *          LookupIconIdFromDirectoryEx32       (USER32.379)
 */
INT32 WINAPI LookupIconIdFromDirectoryEx32( LPBYTE dir, BOOL32 bIcon,
             INT32 width, INT32 height, UINT32 cFlag )
{
    return LookupIconIdFromDirectoryEx16( dir, bIcon, width, height, cFlag );
}

/**********************************************************************
 *          LookupIconIdFromDirectory		(USER.???)
 */
INT16 WINAPI LookupIconIdFromDirectory16( LPBYTE dir, BOOL16 bIcon )
{
    return LookupIconIdFromDirectoryEx16( dir, bIcon, 
	   bIcon ? SYSMETRICS_CXICON : SYSMETRICS_CXCURSOR,
	   bIcon ? SYSMETRICS_CYICON : SYSMETRICS_CYCURSOR, bIcon ? 0 : LR_MONOCHROME );
}

/**********************************************************************
 *          LookupIconIdFromDirectory		(USER32.378)
 */
INT32 WINAPI LookupIconIdFromDirectory32( LPBYTE dir, BOOL32 bIcon )
{
    return LookupIconIdFromDirectoryEx32( dir, bIcon, 
	   bIcon ? SYSMETRICS_CXICON : SYSMETRICS_CXCURSOR,
	   bIcon ? SYSMETRICS_CYICON : SYSMETRICS_CYCURSOR, bIcon ? 0 : LR_MONOCHROME );
}

/**********************************************************************
 *	    GetIconID    (USER.455)
 */
WORD WINAPI GetIconID( HGLOBAL16 hResource, DWORD resType )
{
    LPBYTE lpDir = (LPBYTE)GlobalLock16(hResource);

    TRACE(cursor, "hRes=%04x, entries=%i\n",
                    hResource, lpDir ? ((CURSORICONDIR*)lpDir)->idCount : 0);

    switch(resType)
    {
	case RT_CURSOR:
	     return (WORD)LookupIconIdFromDirectoryEx16( lpDir, FALSE, 
			  SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR, LR_MONOCHROME );
	case RT_ICON:
	     return (WORD)LookupIconIdFromDirectoryEx16( lpDir, TRUE,
			  SYSMETRICS_CXICON, SYSMETRICS_CYICON, 0 );
	default:
	     fprintf( stderr, "GetIconID: invalid res type %ld\n", resType );
    }
    return 0;
}

/**********************************************************************
 *          LoadCursorIconHandler    (USER.336)
 *
 * Supposed to load resources of Windows 2.x applications.
 */
HGLOBAL16 WINAPI LoadCursorIconHandler( HGLOBAL16 hResource, HMODULE16 hModule, HRSRC16 hRsrc )
{
    fprintf(stderr,"hModule[%04x]: old 2.x resources are not supported!\n", hModule);
    return (HGLOBAL16)0;
}

/**********************************************************************
 *          LoadDIBIconHandler    (USER.357)
 * 
 * RT_ICON resource loader, installed by USER_SignalProc when module
 * is initialized.
 */
HGLOBAL16 WINAPI LoadDIBIconHandler( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    /* If hResource is zero we must allocate a new memory block, if it's
     * non-zero but GlobalLock() returns NULL then it was discarded and
     * we have to recommit some memory, otherwise we just need to check 
     * the block size. See LoadProc() in 16-bit SDK for more.
     */

     hMemObj = USER_CallDefaultRsrcHandler( hMemObj, hModule, hRsrc );
     if( hMemObj )
     {
	 LPBYTE bits = (LPBYTE)GlobalLock16( hMemObj );
	 hMemObj = CURSORICON_CreateFromResource( hModule, hMemObj, bits, 
		   SizeofResource16(hModule, hRsrc), TRUE, 0x00030000, 
		   SYSMETRICS_CXICON, SYSMETRICS_CYICON, LR_DEFAULTCOLOR );
     }
     return hMemObj;
}

/**********************************************************************
 *          LoadDIBCursorHandler    (USER.356)
 *
 * RT_CURSOR resource loader. Same as above.
 */
HGLOBAL16 WINAPI LoadDIBCursorHandler( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    hMemObj = USER_CallDefaultRsrcHandler( hMemObj, hModule, hRsrc );
    if( hMemObj )
    {
	LPBYTE bits = (LPBYTE)GlobalLock16( hMemObj );
	hMemObj = CURSORICON_CreateFromResource( hModule, hMemObj, bits,
		  SizeofResource16(hModule, hRsrc), FALSE, 0x00030000,
		  SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR, LR_MONOCHROME );
    }
    return hMemObj;
}

/**********************************************************************
 *	    LoadIconHandler    (USER.456)
 */
HICON16 WINAPI LoadIconHandler( HGLOBAL16 hResource, BOOL16 bNew )
{
    LPBYTE bits = (LPBYTE)LockResource16( hResource );

    TRACE(cursor,"hRes=%04x\n",hResource);

    return CURSORICON_CreateFromResource( 0, 0, bits, 0, TRUE, 
		      bNew ? 0x00030000 : 0x00020000, 0, 0, LR_DEFAULTCOLOR );
}

/***********************************************************************
 *           LoadCursorW                (USER32.361)
 */
HCURSOR32 WINAPI LoadCursor32W(HINSTANCE32 hInstance, LPCWSTR name)
{
    return CURSORICON_Load32( hInstance, name,
                              SYSMETRICS_CXCURSOR, SYSMETRICS_CYCURSOR, 1, TRUE);
}

/***********************************************************************
 *           LoadCursorA                (USER32.358)
 */
HCURSOR32 WINAPI LoadCursor32A(HINSTANCE32 hInstance, LPCSTR name)
{
        HCURSOR32 res=0;
        if(!HIWORD(name))
                return LoadCursor32W(hInstance,(LPCWSTR)name);
        else
        {
            LPWSTR uni = HEAP_strdupAtoW( GetProcessHeap(), 0, name );
            res = LoadCursor32W(hInstance, uni);
            HeapFree( GetProcessHeap(), 0, uni);
        }
        return res;
}

/***********************************************************************
 *           LoadIconW          (USER32.363)
 */
HICON32 WINAPI LoadIcon32W(HINSTANCE32 hInstance, LPCWSTR name)
{
    return CURSORICON_Load32( hInstance, name,
                              SYSMETRICS_CXICON, SYSMETRICS_CYICON,
                              MIN( 16, COLOR_GetSystemPaletteSize() ), FALSE );
}

/***********************************************************************
 *           LoadIconA          (USER32.362)
 */
HICON32 WINAPI LoadIcon32A(HINSTANCE32 hInstance, LPCSTR name)
{
    HICON32 res=0;

    if( !HIWORD(name) )
	return LoadIcon32W(hInstance, (LPCWSTR)name);
    else
    {
	LPWSTR uni = HEAP_strdupAtoW( GetProcessHeap(), 0, name );
	res = LoadIcon32W( hInstance, uni );
	HeapFree( GetProcessHeap(), 0, uni );
    }
    return res;
}

/**********************************************************************
 *          GetIconInfo		(USER32.241)
 */
BOOL32 WINAPI GetIconInfo(HICON32 hIcon,LPICONINFO iconinfo) {
    CURSORICONINFO	*ciconinfo;

    ciconinfo = GlobalLock16(hIcon);
    if (!ciconinfo)
	return FALSE;
    iconinfo->xHotspot = ciconinfo->ptHotSpot.x;
    iconinfo->yHotspot = ciconinfo->ptHotSpot.y;
    iconinfo->fIcon    = TRUE; /* hmm */
    /* FIXME ... add both bitmaps */
    return TRUE;
}

/**********************************************************************
 *          CreateIconIndirect		(USER32.78)
 */
HICON32 WINAPI CreateIconIndirect(LPICONINFO iconinfo) {
    BITMAPOBJ *bmpXor,*bmpAnd;
    HICON32 hObj;
    int	sizeXor,sizeAnd;

    bmpXor = (BITMAPOBJ *) GDI_GetObjPtr( iconinfo->hbmColor, BITMAP_MAGIC );
    bmpAnd = (BITMAPOBJ *) GDI_GetObjPtr( iconinfo->hbmMask, BITMAP_MAGIC );

    sizeXor = bmpXor->bitmap.bmHeight * bmpXor->bitmap.bmWidthBytes;
    sizeAnd = bmpAnd->bitmap.bmHeight * bmpAnd->bitmap.bmWidthBytes;

    hObj = GlobalAlloc16( GMEM_MOVEABLE, 
		     sizeof(CURSORICONINFO) + sizeXor + sizeAnd );
    if (hObj)
    {
	CURSORICONINFO *info;

	info = (CURSORICONINFO *)GlobalLock16( hObj );
	info->ptHotSpot.x   = iconinfo->xHotspot;
	info->ptHotSpot.y   = iconinfo->yHotspot;
	info->nWidth        = bmpXor->bitmap.bmWidth;
	info->nHeight       = bmpXor->bitmap.bmHeight;
	info->nWidthBytes   = bmpXor->bitmap.bmWidthBytes;
	info->bPlanes       = bmpXor->bitmap.bmPlanes;
	info->bBitsPerPixel = bmpXor->bitmap.bmBitsPixel;

	/* Transfer the bitmap bits to the CURSORICONINFO structure */

	GetBitmapBits32( iconinfo->hbmMask ,sizeAnd,(char*)(info + 1) );
	GetBitmapBits32( iconinfo->hbmColor,sizeXor,(char*)(info + 1) +sizeAnd);
	GlobalUnlock16( hObj );
    }
    return hObj;
}

/**********************************************************************
 *          DrawIconEx16		(USER.394)
 */

BOOL16 WINAPI DrawIconEx16 (HDC16 hdc, INT16 xLeft, INT16 yTop, HICON16 hIcon,
			    INT16 cxWidth, INT16 cyWidth, UINT16 istep,
			    HBRUSH16 hbr, UINT16 flags)
{
  return DrawIconEx32 (hdc, xLeft, yTop, hIcon, cxWidth, cyWidth,
		       istep, hbr, flags);
}

/**********************************************************************
 *          DrawIconEx32		(USER32.160)
 */

BOOL32 WINAPI DrawIconEx32 (HDC32 hdc, INT32 x0, INT32 y0, HICON32 hIcon,
			    INT32 cxWidth, INT32 cyWidth, UINT32 istep,
			    HBRUSH32 hbr, UINT32 flags)
{
    CURSORICONINFO *ptr = (CURSORICONINFO *)GlobalLock16 (hIcon);
    HDC32 hMemDC = CreateCompatibleDC32 (hdc);
    BOOL32 result = FALSE;

    FIXME(icon, "part stub.\n");

    if (hMemDC && ptr)
    {
	HBITMAP32 hXorBits, hAndBits;
	COLORREF oldFg, oldBg;

	/* Calculate the size of the destination image.  */
	if (cxWidth == 0)
	    if (flags & DI_DEFAULTSIZE)
		cxWidth = GetSystemMetrics32 (SM_CXICON);
	    else
		cxWidth = ptr->nWidth;
	if (cyWidth == 0)
	    if (flags & DI_DEFAULTSIZE)
		cyWidth = GetSystemMetrics32 (SM_CYICON);
	    else
		cyWidth = ptr->nHeight;

	hXorBits = CreateBitmap32 ( ptr->nWidth, ptr->nHeight,
				    ptr->bPlanes, ptr->bBitsPerPixel,
				    (char *)(ptr + 1)
				    + ptr->nHeight *
				    BITMAP_WIDTH_BYTES(ptr->nWidth,1) );
	hAndBits = CreateBitmap32 ( cxWidth, cyWidth,
				    1, 1, (char *)(ptr+1) );
	oldFg = SetTextColor32( hdc, RGB(0,0,0) );
	oldBg = SetBkColor32( hdc, RGB(255,255,255) );

	if (hXorBits && hAndBits)
	{
	    HBITMAP32 hBitTemp = SelectObject32( hMemDC, hAndBits );
	    if (flags & DI_MASK)
		BitBlt32 (hdc, x0, y0, cxWidth, cyWidth,
			  hMemDC, 0, 0, SRCAND);
	    SelectObject32( hMemDC, hXorBits );
	    if (flags & DI_IMAGE)
		BitBlt32 (hdc, x0, y0, cxWidth, cyWidth,
			  hMemDC, 0, 0, SRCPAINT);
	    SelectObject32( hMemDC, hBitTemp );
	    result = TRUE;
	}

	SetTextColor32( hdc, oldFg );
	SetBkColor32( hdc, oldBg );
	if (hXorBits) DeleteObject32( hXorBits );
	if (hAndBits) DeleteObject32( hAndBits );
    }
    if (hMemDC) DeleteDC32( hMemDC );
    GlobalUnlock16( hIcon );
    return result;
}
