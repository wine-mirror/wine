/*
 * Cursor and icon support
 *
 * Copyright 1995 Alexandre Julliard
 *           1996 Martin Von Loewis
 *           1997 Alex Korobka
 *           1998 Turchanov Sergey
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
 *
 * FIXME: what are we going to do with animation and color (bpp > 1) cursors ?!
 */

#include <string.h>
#include <stdlib.h>

#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "heap.h"
#include "color.h"
#include "bitmap.h"
#include "cursoricon.h"
#include "dc.h"
#include "gdi.h"
#include "global.h"
#include "module.h"
#include "debugtools.h"
#include "task.h"
#include "user.h"
#include "input.h"
#include "display.h"
#include "message.h"
#include "winerror.h"

DECLARE_DEBUG_CHANNEL(cursor)
DECLARE_DEBUG_CHANNEL(icon)

static HCURSOR hActiveCursor = 0;  /* Active cursor */
static INT CURSOR_ShowCount = 0;   /* Cursor display count */
static RECT CURSOR_ClipRect;       /* Cursor clipping rect */


/**********************************************************************
 * ICONCACHE for cursors/icons loaded with LR_SHARED.
 *
 * FIXME: This should not be allocated on the system heap, but on a
 *        subsystem-global heap (i.e. one for all Win16 processes,
 *        and one each for every Win32 process).
 */
typedef struct tagICONCACHE
{
    struct tagICONCACHE *next;

    HMODULE              hModule;
    HRSRC                hRsrc;
    HRSRC                hGroupRsrc;
    HANDLE               handle;

    INT                  count;

} ICONCACHE;

static ICONCACHE *IconAnchor = NULL;
static CRITICAL_SECTION IconCrst;
static DWORD ICON_HOTSPOT = 0x42424242;

/**********************************************************************
 *	    CURSORICON_Init
 */
void CURSORICON_Init( void )
{
    InitializeCriticalSection( &IconCrst );
    MakeCriticalSectionGlobal( &IconCrst );
}

/**********************************************************************
 *	    CURSORICON_FindSharedIcon
 */
static HANDLE CURSORICON_FindSharedIcon( HMODULE hModule, HRSRC hRsrc )
{
    HANDLE handle = 0;
    ICONCACHE *ptr;

    EnterCriticalSection( &IconCrst );

    for ( ptr = IconAnchor; ptr; ptr = ptr->next )
        if ( ptr->hModule == hModule && ptr->hRsrc == hRsrc )
        {
            ptr->count++;
            handle = ptr->handle;
            break;
        }

    LeaveCriticalSection( &IconCrst );

    return handle;
}

/*************************************************************************
 * CURSORICON_FindCache 
 *
 * Given a handle, find the coresponding cache element
 *
 * PARAMS
 *      Handle     [I] handle to an Image 
 *
 * RETURNS
 *     Success: The cache entry
 *     Failure: NULL
 *
 */
static ICONCACHE* CURSORICON_FindCache(HANDLE handle)
{
    ICONCACHE *ptr;
    ICONCACHE *pRet=NULL;
    BOOL IsFound = FALSE;
    int count;

    EnterCriticalSection( &IconCrst );

    for (count = 0, ptr = IconAnchor; ptr != NULL && !IsFound; ptr = ptr->next, count++ )
    {
        if ( handle == ptr->handle )
        {
            IsFound = TRUE;
            pRet = ptr;
        }
    }

    LeaveCriticalSection( &IconCrst );

    return pRet;
}

/**********************************************************************
 *	    CURSORICON_AddSharedIcon
 */
static void CURSORICON_AddSharedIcon( HMODULE hModule, HRSRC hRsrc, HRSRC hGroupRsrc, HANDLE handle )
{
    ICONCACHE *ptr = HeapAlloc( SystemHeap, 0, sizeof(ICONCACHE) );
    if ( !ptr ) return;

    ptr->hModule = hModule;
    ptr->hRsrc   = hRsrc;
    ptr->handle  = handle;
    ptr->hGroupRsrc = hGroupRsrc;
    ptr->count   = 1;

    EnterCriticalSection( &IconCrst );
    ptr->next    = IconAnchor;
    IconAnchor   = ptr;
    LeaveCriticalSection( &IconCrst );
}

/**********************************************************************
 *	    CURSORICON_DelSharedIcon
 */
static INT CURSORICON_DelSharedIcon( HANDLE handle )
{
    INT count = -1;
    ICONCACHE *ptr;

    EnterCriticalSection( &IconCrst );

    for ( ptr = IconAnchor; ptr; ptr = ptr->next )
        if ( ptr->handle == handle )
        {
            if ( ptr->count > 0 ) ptr->count--;
            count = ptr->count;
            break;
        }

    LeaveCriticalSection( &IconCrst );

    return count;
}

/**********************************************************************
 *	    CURSORICON_FreeModuleIcons
 */
void CURSORICON_FreeModuleIcons( HMODULE hModule )
{
    ICONCACHE **ptr = &IconAnchor;

    if ( HIWORD( hModule ) )
        hModule = MapHModuleLS( hModule );
    else
        hModule = GetExePtr( hModule );

    EnterCriticalSection( &IconCrst );

    while ( *ptr )
    {
        if ( (*ptr)->hModule == hModule )
        {
            ICONCACHE *freePtr = *ptr;
            *ptr = freePtr->next;
            
            GlobalFree16( freePtr->handle );
            HeapFree( SystemHeap, 0, freePtr );
            continue;
        }
        ptr = &(*ptr)->next;
    }

    LeaveCriticalSection( &IconCrst );
}

/**********************************************************************
 *	    CURSORICON_FindBestIcon
 *
 * Find the icon closest to the requested size and number of colors.
 */
static CURSORICONDIRENTRY *CURSORICON_FindBestIcon( CURSORICONDIR *dir, int width,
                                              int height, int colors )
{
    int i; 
    CURSORICONDIRENTRY *entry, *bestEntry = NULL;
    UINT iTotalDiff, iXDiff=0, iYDiff=0, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;

    if (dir->idCount < 1)
    {
        WARN_(icon)("Empty directory!\n" );
        return NULL;
    }
    if (dir->idCount == 1) return &dir->idEntries[0];  /* No choice... */

    /* Find Best Fit */
    iTotalDiff = 0xFFFFFFFF;
    iColorDiff = 0xFFFFFFFF;
    for (i = 0, entry = &dir->idEntries[0]; i < dir->idCount; i++,entry++)
        {
	iTempXDiff = abs(width - entry->ResInfo.icon.bWidth);
	iTempYDiff = abs(height - entry->ResInfo.icon.bHeight);

        if(iTotalDiff > (iTempXDiff + iTempYDiff))
        {
            iXDiff = iTempXDiff;
            iYDiff = iTempYDiff;
	    iTotalDiff = iXDiff + iYDiff;
        }
        }

    /* Find Best Colors for Best Fit */
    for (i = 0, entry = &dir->idEntries[0]; i < dir->idCount; i++,entry++)
        {
        if(abs(width - entry->ResInfo.icon.bWidth) == iXDiff &&
            abs(height - entry->ResInfo.icon.bHeight) == iYDiff)
        {
            iTempColorDiff = abs(colors - entry->ResInfo.icon.bColorCount);
            if(iColorDiff > iTempColorDiff)
        {
            bestEntry = entry;
                iColorDiff = iTempColorDiff;
        }
        }
    }

    return bestEntry;
}


/**********************************************************************
 *	    CURSORICON_FindBestCursor
 *
 * Find the cursor closest to the requested size.
 * FIXME: parameter 'color' ignored and entries with more than 1 bpp
 *        ignored too
 */
static CURSORICONDIRENTRY *CURSORICON_FindBestCursor( CURSORICONDIR *dir,
                                                  int width, int height, int color)
{
    int i, maxwidth, maxheight;
    CURSORICONDIRENTRY *entry, *bestEntry = NULL;

    if (dir->idCount < 1)
    {
        WARN_(cursor)("Empty directory!\n" );
        return NULL;
    }
    if (dir->idCount == 1) return &dir->idEntries[0]; /* No choice... */

    /* Double height to account for AND and XOR masks */

    height *= 2;

    /* First find the largest one smaller than or equal to the requested size*/

    maxwidth = maxheight = 0;
    for(i = 0,entry = &dir->idEntries[0]; i < dir->idCount; i++,entry++)
        if ((entry->ResInfo.cursor.wWidth <= width) && (entry->ResInfo.cursor.wHeight <= height) &&
            (entry->ResInfo.cursor.wWidth > maxwidth) && (entry->ResInfo.cursor.wHeight > maxheight) &&
	    (entry->wBitCount == 1))
        {
            bestEntry = entry;
            maxwidth  = entry->ResInfo.cursor.wWidth;
            maxheight = entry->ResInfo.cursor.wHeight;
        }
    if (bestEntry) return bestEntry;

    /* Now find the smallest one larger than the requested size */

    maxwidth = maxheight = 255;
    for(i = 0,entry = &dir->idEntries[0]; i < dir->idCount; i++,entry++)
        if ((entry->ResInfo.cursor.wWidth < maxwidth) && (entry->ResInfo.cursor.wHeight < maxheight) &&
	    (entry->wBitCount == 1))
        {
            bestEntry = entry;
            maxwidth  = entry->ResInfo.cursor.wWidth;
            maxheight = entry->ResInfo.cursor.wHeight;
        }

    return bestEntry;
}

/*********************************************************************
 * The main purpose of this function is to create fake resource directory
 * and fake resource entries. There are several reasons for this:
 *	-	CURSORICONDIR and CURSORICONFILEDIR differ in sizes and their
 *              fields
 *	There are some "bad" cursor files which do not have
 *		bColorCount initialized but instead one must read this info
 *		directly from corresponding DIB sections
 * Note: wResId is index to array of pointer returned in ptrs (origin is 1)
 */
BOOL CURSORICON_SimulateLoadingFromResourceW( LPWSTR filename, BOOL fCursor,
                                                CURSORICONDIR **res, LPBYTE **ptr)
{
    LPBYTE   _free;
    CURSORICONFILEDIR *bits;
    int	     entries, size, i;

    *res = NULL;
    *ptr = NULL;
    if (!(bits = (CURSORICONFILEDIR *)VIRTUAL_MapFileW( filename ))) return FALSE;

    /* FIXME: test for inimated icons
     * hack to load the first icon from the *.ani file
     */
    if ( *(LPDWORD)bits==0x46464952 ) /* "RIFF" */
    { LPBYTE pos = (LPBYTE) bits;
      FIXME_(cursor)("Animated icons not correctly implemented! %p \n", bits);
	
      for (;;)
      { if (*(LPDWORD)pos==0x6e6f6369)		/* "icon" */
        { FIXME_(cursor)("icon entry found! %p\n", bits);
	  pos+=4;
	  if ( !*(LPWORD) pos==0x2fe)		/* iconsize */
	  { goto fail;
	  }
	  bits=(CURSORICONFILEDIR*)(pos+4);
	  FIXME_(cursor)("icon size ok. offset=%p \n", bits);
	  break;
	}
        pos+=2;
        if (pos>=(LPBYTE)bits+766) goto fail;
      }
    }
    if (!(entries = bits->idCount)) goto fail;
    size = sizeof(CURSORICONDIR) + sizeof(CURSORICONDIRENTRY) * (entries - 1);
    _free = (LPBYTE) size;

    for (i=0; i < entries; i++)
      size += bits->idEntries[i].dwDIBSize + (fCursor ? sizeof(POINT16): 0);
    
    if (!(*ptr = HeapAlloc( GetProcessHeap(), 0,
                            entries * sizeof (CURSORICONDIRENTRY*)))) goto fail;
    if (!(*res = HeapAlloc( GetProcessHeap(), 0, size))) goto fail;

    _free = (LPBYTE)(*res) + (int)_free;
    memcpy((*res), bits, 6);
    for (i=0; i<entries; i++)
    {
      ((LPBYTE*)(*ptr))[i] = _free;
      if (fCursor) {
        (*res)->idEntries[i].ResInfo.cursor.wWidth=bits->idEntries[i].bWidth;
        (*res)->idEntries[i].ResInfo.cursor.wHeight=bits->idEntries[i].bHeight;
	((LPPOINT16)_free)->x=bits->idEntries[i].xHotspot;
	((LPPOINT16)_free)->y=bits->idEntries[i].yHotspot;
	_free+=sizeof(POINT16);
      } else {
        (*res)->idEntries[i].ResInfo.icon.bWidth=bits->idEntries[i].bWidth;
        (*res)->idEntries[i].ResInfo.icon.bHeight=bits->idEntries[i].bHeight;
        (*res)->idEntries[i].ResInfo.icon.bColorCount = bits->idEntries[i].bColorCount;
      }
      (*res)->idEntries[i].wPlanes=1;
      (*res)->idEntries[i].wBitCount = ((LPBITMAPINFOHEADER)((LPBYTE)bits +
                                                   bits->idEntries[i].dwDIBOffset))->biBitCount;
      (*res)->idEntries[i].dwBytesInRes = bits->idEntries[i].dwDIBSize;
      (*res)->idEntries[i].wResId=i+1;

      memcpy(_free,(LPBYTE)bits +bits->idEntries[i].dwDIBOffset,
             (*res)->idEntries[i].dwBytesInRes);
      _free += (*res)->idEntries[i].dwBytesInRes;
    }
    UnmapViewOfFile( bits );
    return TRUE;    
fail:
    if (*res) HeapFree( GetProcessHeap(), 0, *res );
    if (*ptr) HeapFree( GetProcessHeap(), 0, *ptr );
    UnmapViewOfFile( bits );
    return FALSE;
}


/**********************************************************************
 *	    CURSORICON_CreateFromResource
 *
 * Create a cursor or icon from in-memory resource template. 
 *
 * FIXME: Convert to mono when cFlag is LR_MONOCHROME. Do something
 *        with cbSize parameter as well.
 */
static HGLOBAL16 CURSORICON_CreateFromResource( HINSTANCE16 hInstance, HGLOBAL16 hObj, LPBYTE bits,
	 					UINT cbSize, BOOL bIcon, DWORD dwVersion, 
						INT width, INT height, UINT loadflags )
{
    int sizeAnd, sizeXor;
    HBITMAP hAndBits = 0, hXorBits = 0; /* error condition for later */
    BITMAPOBJ *bmpXor, *bmpAnd;
    POINT16 hotspot = { ICON_HOTSPOT, ICON_HOTSPOT };
    BITMAPINFO *bmi;
    HDC hdc;
    BOOL DoStretch;
    INT size;

    TRACE_(cursor)("%08x (%u bytes), ver %08x, %ix%i %s %s\n",
                        (unsigned)bits, cbSize, (unsigned)dwVersion, width, height,
                                  bIcon ? "icon" : "cursor", (loadflags & LR_MONOCHROME) ? "mono" : "" );
    if (dwVersion == 0x00020000)
    {
	FIXME_(cursor)("\t2.xx resources are not supported\n");
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
    size = DIB_BitmapInfoSize( bmi, DIB_RGB_COLORS );

    if (!width) width = bmi->bmiHeader.biWidth;
    if (!height) height = bmi->bmiHeader.biHeight/2;
    DoStretch = (bmi->bmiHeader.biHeight/2 != height) ||
      (bmi->bmiHeader.biWidth != width);

    /* Check bitmap header */

    if ( (bmi->bmiHeader.biSize != sizeof(BITMAPCOREHEADER)) &&
	 (bmi->bmiHeader.biSize != sizeof(BITMAPINFOHEADER)  ||
	  bmi->bmiHeader.biCompression != BI_RGB) )
    {
          WARN_(cursor)("\tinvalid resource bitmap header.\n");
          return 0;
    }

    if( (hdc = GetDC( 0 )) )
    {
	BITMAPINFO* pInfo;

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

    	if ((pInfo = (BITMAPINFO *)HeapAlloc( GetProcessHeap(), 0, 
	  MAX(size, sizeof(BITMAPINFOHEADER) + 2*sizeof(RGBQUAD)))))
	{	
	    memcpy( pInfo, bmi, size );	
	    pInfo->bmiHeader.biHeight /= 2;

	    /* Create the XOR bitmap */

	    if (DoStretch) {
                if(bIcon)
                {
                    hXorBits = CreateCompatibleBitmap(hdc, width, height);
                }
                else
                {
                    hXorBits = CreateBitmap(width, height, 1, 1, NULL);
                }
                if(hXorBits)
                {
		HBITMAP hOld;
		HDC hMem = CreateCompatibleDC(hdc);
		BOOL res;

		if (hMem) {
		  hOld = SelectObject(hMem, hXorBits);
	          res = StretchDIBits(hMem, 0, 0, width, height, 0, 0,
		    bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight/2,
		    (char*)bmi + size, pInfo, DIB_RGB_COLORS, SRCCOPY);
		  SelectObject(hMem, hOld);
		  DeleteDC(hMem);
	        } else res = FALSE;
		if (!res) { DeleteObject(hXorBits); hXorBits = 0; }
	      }
	    } else hXorBits = CreateDIBitmap( hdc, &pInfo->bmiHeader,
		CBM_INIT, (char*)bmi + size, pInfo, DIB_RGB_COLORS );
	    if( hXorBits )
	    {
		char* bits = (char *)bmi + size +
			DIB_GetDIBImageBytes(bmi->bmiHeader.biWidth,
					     bmi->bmiHeader.biHeight,
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

	    if (DoStretch) {
	      if ((hAndBits = CreateBitmap(width, height, 1, 1, NULL))) {
		HBITMAP hOld;
		HDC hMem = CreateCompatibleDC(hdc);
		BOOL res;

		if (hMem) {
		  hOld = SelectObject(hMem, hAndBits);
	          res = StretchDIBits(hMem, 0, 0, width, height, 0, 0,
		    pInfo->bmiHeader.biWidth, pInfo->bmiHeader.biHeight,
		    bits, pInfo, DIB_RGB_COLORS, SRCCOPY);
		  SelectObject(hMem, hOld);
		  DeleteDC(hMem);
	        } else res = FALSE;
		if (!res) { DeleteObject(hAndBits); hAndBits = 0; }
	      }
	    } else hAndBits = CreateDIBitmap( hdc, &pInfo->bmiHeader,
	      CBM_INIT, bits, pInfo, DIB_RGB_COLORS );

		if( !hAndBits ) DeleteObject( hXorBits );
	    }
	    HeapFree( GetProcessHeap(), 0, pInfo ); 
	}
	ReleaseDC( 0, hdc );
    }

    if( !hXorBits || !hAndBits ) 
    {
	WARN_(cursor)("\tunable to create an icon bitmap.\n");
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
	if (hInstance) FarSetOwner16( hObj, GetExePtr(hInstance) );

	info = (CURSORICONINFO *)GlobalLock16( hObj );
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
	GlobalUnlock16( hObj );
    }

    DeleteObject( hXorBits );
    DeleteObject( hAndBits );
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
    return CreateIconFromResourceEx(bits, cbSize, bIcon, dwVersion, 
      width, height, cFlag);
}


/**********************************************************************
 *          CreateIconFromResource          (USER32.76)
 */
HICON WINAPI CreateIconFromResource( LPBYTE bits, UINT cbSize,
                                           BOOL bIcon, DWORD dwVersion)
{
    return CreateIconFromResourceEx( bits, cbSize, bIcon, dwVersion, 0,0,0);
}


/**********************************************************************
 *          CreateIconFromResourceEx32          (USER32.77)
 */
HICON WINAPI CreateIconFromResourceEx( LPBYTE bits, UINT cbSize,
                                           BOOL bIcon, DWORD dwVersion,
                                           INT width, INT height,
                                           UINT cFlag )
{
    TDB* pTask = (TDB*)GlobalLock16( GetCurrentTask() );
    if( pTask )
	return CURSORICON_CreateFromResource( pTask->hInstance, 0, bits, cbSize, bIcon, dwVersion,
					      width, height, cFlag );
    return 0;
}

/**********************************************************************
 *          CURSORICON_Load
 *
 * Load a cursor or icon from resource or file.
 */
HGLOBAL CURSORICON_Load( HINSTANCE hInstance, LPCWSTR name,
                         INT width, INT height, INT colors,
                         BOOL fCursor, UINT loadflags )
{
    HANDLE handle = 0, h = 0;
    HANDLE hRsrc;
    CURSORICONDIR *dir;
    CURSORICONDIRENTRY *dirEntry;
    LPBYTE bits;

    if ( loadflags & LR_LOADFROMFILE )    /* Load from file */
    {
        LPBYTE *ptr;
        if (!CURSORICON_SimulateLoadingFromResourceW((LPWSTR)name, fCursor, &dir, &ptr))
            return 0;
        if (fCursor)
            dirEntry = (CURSORICONDIRENTRY *)CURSORICON_FindBestCursor(dir, width, height, 1);
        else
            dirEntry = (CURSORICONDIRENTRY *)CURSORICON_FindBestIcon(dir, width, height, colors);
        bits = ptr[dirEntry->wResId-1];
        h = CURSORICON_CreateFromResource( 0, 0, bits, dirEntry->dwBytesInRes, 
                                           !fCursor, 0x00030000, width, height, loadflags);
        HeapFree( GetProcessHeap(), 0, dir );
        HeapFree( GetProcessHeap(), 0, ptr );
    }

    else if ( !hInstance )  /* Load OEM cursor/icon */
    {
        WORD resid;
        HDC hdc;

        if ( HIWORD(name) )
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
	hdc = CreateDCA( "DISPLAY", NULL, NULL, NULL );
	if (hdc) {
	    DC *dc = DC_GetDCPtr( hdc );
	    if (dc->funcs->pLoadOEMResource)
	        h = dc->funcs->pLoadOEMResource( resid, fCursor ? OEM_CURSOR : OEM_ICON );
	    GDI_HEAP_UNLOCK( hdc );
	    DeleteDC(  hdc );
	}
    }

    else  /* Load from resource */
    {
        HANDLE hGroupRsrc;
        WORD wResId;
        DWORD dwBytesInRes;

        /* Normalize hInstance (must be uniquely represented for icon cache) */
        
        if ( HIWORD( hInstance ) )
            hInstance = MapHModuleLS( hInstance );
        else
            hInstance = GetExePtr( hInstance );

        /* Get directory resource ID */

        if (!(hRsrc = FindResourceW( hInstance, name,
                          fCursor ? RT_GROUP_CURSORW : RT_GROUP_ICONW )))
            return 0;
	hGroupRsrc = hRsrc;
        /* If shared icon, check whether it was already loaded */

        if (    (loadflags & LR_SHARED) 
             && (h = CURSORICON_FindSharedIcon( hInstance, hRsrc ) ) != 0 )
            return h;

        /* Find the best entry in the directory */
 
        if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
        if (!(dir = (CURSORICONDIR*)LockResource( handle ))) return 0;
        if (fCursor)
            dirEntry = (CURSORICONDIRENTRY *)CURSORICON_FindBestCursor( dir,
                                                              width, height, 1);
        else
            dirEntry = (CURSORICONDIRENTRY *)CURSORICON_FindBestIcon( dir,
                                                       width, height, colors );
        if (!dirEntry) return 0;
        wResId = dirEntry->wResId;
        dwBytesInRes = dirEntry->dwBytesInRes;
        FreeResource( handle );

        /* Load the resource */

        if (!(hRsrc = FindResourceW(hInstance,MAKEINTRESOURCEW(wResId),
                                      fCursor ? RT_CURSORW : RT_ICONW ))) return 0;
        if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
        bits = (LPBYTE)LockResource( handle );
        h = CURSORICON_CreateFromResource( 0, 0, bits, dwBytesInRes, 
                                           !fCursor, 0x00030000, width, height, loadflags);
        FreeResource( handle );

        /* If shared icon, add to icon cache */

        if ( h && (loadflags & LR_SHARED) )
            CURSORICON_AddSharedIcon( hInstance, hRsrc, hGroupRsrc, h );
    }

    return h;
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
    FarSetOwner16( hNew, hInstance );
    ptrNew = (char *)GlobalLock16( hNew );
    memcpy( ptrNew, ptrOld, size );
    GlobalUnlock16( handle );
    GlobalUnlock16( hNew );
    return hNew;
}

/*************************************************************************
 * CURSORICON_ExtCopy 
 *
 * Copies an Image from the Cache if LR_COPYFROMRESOURCE is specified
 *
 * PARAMS
 *      Handle     [I] handle to an Image 
 *      nType      [I] Type of Handle (IMAGE_CURSOR | IMAGE_ICON)
 *      iDesiredCX [I] The Desired width of the Image
 *      iDesiredCY [I] The desired height of the Image
 *      nFlags     [I] The flags from CopyImage
 *
 * RETURNS
 *     Success: The new handle of the Image
 *
 * NOTES
 *     LR_COPYDELETEORG and LR_MONOCHROME are currently not implemented.
 *     LR_MONOCHROME should be implemented by CURSORICON_CreateFromResource.
 *     LR_COPYFROMRESOURCE will only work if the Image is in the Cache.
 *
 *     
 */

HGLOBAL CURSORICON_ExtCopy(HGLOBAL Handle, UINT nType, 
			   INT iDesiredCX, INT iDesiredCY, 
			   UINT nFlags)
{
    HGLOBAL16 hNew=0;

    TRACE_(icon)("Handle %u, uType %u, iDesiredCX %i, iDesiredCY %i, nFlags %u\n", 
        Handle, nType, iDesiredCX, iDesiredCY, nFlags);

    if(Handle == 0)
    {
	return 0;
    }

    /* Best Fit or Monochrome */
    if( (nFlags & LR_COPYFROMRESOURCE
        && (iDesiredCX > 0 || iDesiredCY > 0))
        || nFlags & LR_MONOCHROME) 
    {
        ICONCACHE* pIconCache = CURSORICON_FindCache(Handle);

        /* Not Found in Cache, then do a strait copy
        */
        if(pIconCache == NULL)
        {
            TDB* pTask = (TDB *) GlobalLock16 (GetCurrentTask ());
            hNew = CURSORICON_Copy(pTask->hInstance, Handle);
            if(nFlags & LR_COPYFROMRESOURCE)
            {
                TRACE_(icon)("LR_COPYFROMRESOURCE: Failed to load from cache\n");
            }
        }
        else
        {
            int iTargetCX, iTargetCY;
            LPBYTE pBits;
            HANDLE hMem;
            HRSRC hRsrc;
            DWORD dwBytesInRes;
            WORD wResId;
            CURSORICONDIR *pDir;
            CURSORICONDIRENTRY *pDirEntry;
            BOOL bIsIcon = (nType == IMAGE_ICON);

            /* Completing iDesiredCX CY for Monochrome Bitmaps if needed
            */
            if(((nFlags & LR_MONOCHROME) && !(nFlags & LR_COPYFROMRESOURCE))
                || (iDesiredCX == 0 && iDesiredCY == 0))
            {
                iDesiredCY = GetSystemMetrics(bIsIcon ? 
                    SM_CYICON : SM_CYCURSOR);
                iDesiredCX = GetSystemMetrics(bIsIcon ? 
                    SM_CXICON : SM_CXCURSOR);
            }

            /* Retreive the CURSORICONDIRENTRY 
            */
            if (!(hMem = LoadResource( pIconCache->hModule , 
                            pIconCache->hGroupRsrc))) 
            {
                return 0;
            }
            if (!(pDir = (CURSORICONDIR*)LockResource( hMem ))) 
            {
                return 0;
            }

            /* Find Best Fit 
            */
            if(bIsIcon)
            {
                pDirEntry = (CURSORICONDIRENTRY *)CURSORICON_FindBestIcon(
                                pDir, iDesiredCX, iDesiredCY, 256);
            }
            else
            {
                pDirEntry = (CURSORICONDIRENTRY *)CURSORICON_FindBestCursor( 
                                pDir, iDesiredCX, iDesiredCY, 1);
            }

            wResId = pDirEntry->wResId;
            dwBytesInRes = pDirEntry->dwBytesInRes;
            FreeResource(hMem);

            TRACE_(icon)("ResID %u, BytesInRes %lu, Width %d, Height %d DX %d, DY %d\n", 
                wResId, dwBytesInRes,  pDirEntry->ResInfo.icon.bWidth, 
                pDirEntry->ResInfo.icon.bHeight, iDesiredCX, iDesiredCY);

            /* Get the Best Fit
            */
            if (!(hRsrc = FindResourceW(pIconCache->hModule ,
                MAKEINTRESOURCEW(wResId), bIsIcon ? RT_ICONW : RT_CURSORW))) 
            {
                return 0;
            }
            if (!(hMem = LoadResource( pIconCache->hModule , hRsrc ))) 
            {
                return 0;
            }

            pBits = (LPBYTE)LockResource( hMem );
         
            iTargetCY = GetSystemMetrics(SM_CYICON);
            iTargetCX = GetSystemMetrics(SM_CXICON);

            /* Create a New Icon with the proper dimension
            */
            hNew = CURSORICON_CreateFromResource( 0, 0, pBits, dwBytesInRes, 
                       bIsIcon, 0x00030000, iTargetCX, iTargetCY, nFlags);
            FreeResource(hMem);
        }
    }
    else
    {
        TDB* pTask = (TDB *) GlobalLock16 (GetCurrentTask ());
        hNew = CURSORICON_Copy(pTask->hInstance, Handle);
    }
    return hNew;
}

/***********************************************************************
 *           CURSORICON_IconToCursor
 *
 * Converts bitmap to mono and truncates if icon is too large (should
 * probably do StretchBlt() instead).
 */
HCURSOR16 CURSORICON_IconToCursor(HICON16 hIcon, BOOL bSemiTransparent)
{
 HCURSOR16       hRet = 0;
 CURSORICONINFO *pIcon = NULL;
 HTASK16 	 hTask = GetCurrentTask();
 TDB*  		 pTask = (TDB *)GlobalLock16(hTask);

 if(hIcon && pTask)
    if (!(pIcon = (CURSORICONINFO*)GlobalLock16( hIcon ))) return FALSE;
       if (pIcon->bPlanes * pIcon->bBitsPerPixel == 1)
       {
           hRet = CURSORICON_Copy( pTask->hInstance, hIcon );

 
	   pIcon = GlobalLock16(hRet);

	   pIcon->ptHotSpot.x = pIcon->ptHotSpot.y = 15;
 
	   GlobalUnlock(hRet);
       }
       else
       {
           BYTE  pAndBits[128];
           BYTE  pXorBits[128];
	   int   maxx, maxy, ix, iy, bpp = pIcon->bBitsPerPixel;
           BYTE* psPtr, *pxbPtr = pXorBits;
           unsigned xor_width, and_width, val_base = 0xffffffff >> (32 - bpp);
           BYTE* pbc = NULL;

           CURSORICONINFO cI;

	   TRACE_(icon)("[%04x] %ix%i %ibpp (bogus %ibps)\n", 
		hIcon, pIcon->nWidth, pIcon->nHeight, pIcon->bBitsPerPixel, pIcon->nWidthBytes );

	   xor_width = BITMAP_GetWidthBytes( pIcon->nWidth, bpp );
	   and_width = BITMAP_GetWidthBytes( pIcon->nWidth, 1 );
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
		  if(!PALETTE_Driver->pIsDark(val))
                  {
                    pbc = pxbPtr + ix/8;
                   *pbc |= 0x80 >> (ix%8);
                  }
                }
              }
              psPtr += xor_width;
              pxbPtr += 4;
           }

           hRet = CreateCursorIconIndirect16( pTask->hInstance , &cI, pAndBits, pXorBits);

           if( !hRet ) /* fall back on default drag cursor */
                hRet = CURSORICON_Copy( pTask->hInstance ,
                              CURSORICON_Load(0,MAKEINTRESOURCEW(OCR_DRAGOBJECT),
                                         GetSystemMetrics(SM_CXCURSOR),
					 GetSystemMetrics(SM_CYCURSOR), 1, TRUE, 0) );
       }

 return hRet;
}


/***********************************************************************
 *           LoadCursor16    (USER.173)
 */
HCURSOR16 WINAPI LoadCursor16( HINSTANCE16 hInstance, SEGPTR name )
{
    LPCSTR nameStr = HIWORD(name)? PTR_SEG_TO_LIN(name) : (LPCSTR)name;
    return LoadCursorA( hInstance, nameStr );
}


/***********************************************************************
 *           LoadIcon16    (USER.174)
 */
HICON16 WINAPI LoadIcon16( HINSTANCE16 hInstance, SEGPTR name )
{
    LPCSTR nameStr = HIWORD(name)? PTR_SEG_TO_LIN(name) : (LPCSTR)name;
    return LoadIconA( hInstance, nameStr );
}


/***********************************************************************
 *           CreateCursor16    (USER.406)
 */
HCURSOR16 WINAPI CreateCursor16( HINSTANCE16 hInstance,
                                 INT16 xHotSpot, INT16 yHotSpot,
                                 INT16 nWidth, INT16 nHeight,
                                 LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info;

    TRACE_(cursor)("%dx%d spot=%d,%d xor=%p and=%p\n",
                    nWidth, nHeight, xHotSpot, yHotSpot, lpXORbits, lpANDbits);

    info.ptHotSpot.x = xHotSpot;
    info.ptHotSpot.y = yHotSpot;
    info.nWidth = nWidth;
    info.nHeight = nHeight;
    info.nWidthBytes = 0;
    info.bPlanes = 1;
    info.bBitsPerPixel = 1;

    return CreateCursorIconIndirect16( hInstance, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateCursor32    (USER32.67)
 */
HCURSOR WINAPI CreateCursor( HINSTANCE hInstance,
                                 INT xHotSpot, INT yHotSpot,
                                 INT nWidth, INT nHeight,
                                 LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info;

    TRACE_(cursor)("%dx%d spot=%d,%d xor=%p and=%p\n",
                    nWidth, nHeight, xHotSpot, yHotSpot, lpXORbits, lpANDbits);

    info.ptHotSpot.x = xHotSpot;
    info.ptHotSpot.y = yHotSpot;
    info.nWidth = nWidth;
    info.nHeight = nHeight;
    info.nWidthBytes = 0;
    info.bPlanes = 1;
    info.bBitsPerPixel = 1;

    return CreateCursorIconIndirect16( 0, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateIcon16    (USER.407)
 */
HICON16 WINAPI CreateIcon16( HINSTANCE16 hInstance, INT16 nWidth,
                             INT16 nHeight, BYTE bPlanes, BYTE bBitsPixel,
                             LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info;

    TRACE_(icon)("%dx%dx%d, xor=%p, and=%p\n",
                  nWidth, nHeight, bPlanes * bBitsPixel, lpXORbits, lpANDbits);

    info.ptHotSpot.x = ICON_HOTSPOT;
    info.ptHotSpot.y = ICON_HOTSPOT;
    info.nWidth = nWidth;
    info.nHeight = nHeight;
    info.nWidthBytes = 0;
    info.bPlanes = bPlanes;
    info.bBitsPerPixel = bBitsPixel;

    return CreateCursorIconIndirect16( hInstance, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateIcon32    (USER32.75)
 */
HICON WINAPI CreateIcon( HINSTANCE hInstance, INT nWidth,
                             INT nHeight, BYTE bPlanes, BYTE bBitsPixel,
                             LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info;

    TRACE_(icon)("%dx%dx%d, xor=%p, and=%p\n",
                  nWidth, nHeight, bPlanes * bBitsPixel, lpXORbits, lpANDbits);

    info.ptHotSpot.x = ICON_HOTSPOT;
    info.ptHotSpot.y = ICON_HOTSPOT;
    info.nWidth = nWidth;
    info.nHeight = nHeight;
    info.nWidthBytes = 0;
    info.bPlanes = bPlanes;
    info.bBitsPerPixel = bBitsPixel;

    return CreateCursorIconIndirect16( 0, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *           CreateCursorIconIndirect    (USER.408)
 */
HGLOBAL16 WINAPI CreateCursorIconIndirect16( HINSTANCE16 hInstance,
                                           CURSORICONINFO *info,
                                           LPCVOID lpANDbits,
                                           LPCVOID lpXORbits )
{
    HGLOBAL16 handle;
    char *ptr;
    int sizeAnd, sizeXor;

    hInstance = GetExePtr( hInstance );  /* Make it a module handle */
    if (!lpXORbits || !lpANDbits || info->bPlanes != 1) return 0;
    info->nWidthBytes = BITMAP_GetWidthBytes(info->nWidth,info->bBitsPerPixel);
    sizeXor = info->nHeight * info->nWidthBytes;
    sizeAnd = info->nHeight * BITMAP_GetWidthBytes( info->nWidth, 1 );
    if (!(handle = GlobalAlloc16( GMEM_MOVEABLE,
                                  sizeof(CURSORICONINFO) + sizeXor + sizeAnd)))
        return 0;
    if (hInstance) FarSetOwner16( handle, hInstance );
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
    TRACE_(icon)("%04x %04x\n", hInstance, hIcon );
    return CURSORICON_Copy( hInstance, hIcon );
}


/***********************************************************************
 *           CopyIcon32    (USER32.60)
 */
HICON WINAPI CopyIcon( HICON hIcon )
{
  HTASK16 hTask = GetCurrentTask ();
  TDB* pTask = (TDB *) GlobalLock16 (hTask);
    TRACE_(icon)("%04x\n", hIcon );
  return CURSORICON_Copy( pTask->hInstance, hIcon );
}


/***********************************************************************
 *           CopyCursor16    (USER.369)
 */
HCURSOR16 WINAPI CopyCursor16( HINSTANCE16 hInstance, HCURSOR16 hCursor )
{
    TRACE_(cursor)("%04x %04x\n", hInstance, hCursor );
    return CURSORICON_Copy( hInstance, hCursor );
}

/**********************************************************************
 *	    CURSORICON_Destroy   (USER.610)
 *
 * This routine is actually exported from Win95 USER under the name
 * DestroyIcon32 ...  The behaviour implemented here should mimic 
 * the Win95 one exactly, especially the return values, which 
 * depend on the setting of various flags.
 */
WORD WINAPI CURSORICON_Destroy( HGLOBAL16 handle, UINT16 flags )
{
    WORD retv;

    TRACE_(icon)("(%04x, %04x)\n", handle, flags );

    /* Check whether destroying active cursor */

    if ( hActiveCursor == handle )
    {
        ERR_(cursor)("Destroying active cursor!\n" );
        SetCursor( 0 );
    }

    /* Try shared cursor/icon first */

    if ( !(flags & CID_NONSHARED) )
    {
        INT count = CURSORICON_DelSharedIcon( handle );

        if ( count != -1 )
            return (flags & CID_WIN32)? TRUE : (count == 0);

        /* FIXME: OEM cursors/icons should be recognized */
    }

    /* Now assume non-shared cursor/icon */

    retv = GlobalFree16( handle );
    return (flags & CID_RESOURCE)? retv : TRUE;
}

/***********************************************************************
 *           DestroyIcon16    (USER.457)
 */
BOOL16 WINAPI DestroyIcon16( HICON16 hIcon )
{
    return CURSORICON_Destroy( hIcon, 0 );
}

/***********************************************************************
 *           DestroyIcon      (USER32.133)
 */
BOOL WINAPI DestroyIcon( HICON hIcon )
{
    return CURSORICON_Destroy( hIcon, CID_WIN32 );
}

/***********************************************************************
 *           DestroyCursor16  (USER.458)
 */
BOOL16 WINAPI DestroyCursor16( HCURSOR16 hCursor )
{
    return CURSORICON_Destroy( hCursor, 0 );
}

/***********************************************************************
 *           DestroyCursor    (USER32.132)
 */
BOOL WINAPI DestroyCursor( HCURSOR hCursor )
{
    return CURSORICON_Destroy( hCursor, CID_WIN32 );
}


/***********************************************************************
 *           DrawIcon16    (USER.84)
 */
BOOL16 WINAPI DrawIcon16( HDC16 hdc, INT16 x, INT16 y, HICON16 hIcon )
{
    return DrawIcon( hdc, x, y, hIcon );
}


/***********************************************************************
 *           DrawIcon32    (USER32.159)
 */
BOOL WINAPI DrawIcon( HDC hdc, INT x, INT y, HICON hIcon )
{
    CURSORICONINFO *ptr;
    HDC hMemDC;
    HBITMAP hXorBits, hAndBits;
    COLORREF oldFg, oldBg;

    if (!(ptr = (CURSORICONINFO *)GlobalLock16( hIcon ))) return FALSE;
    if (!(hMemDC = CreateCompatibleDC( hdc ))) return FALSE;
    hAndBits = CreateBitmap( ptr->nWidth, ptr->nHeight, 1, 1,
                               (char *)(ptr+1) );
    hXorBits = CreateBitmap( ptr->nWidth, ptr->nHeight, ptr->bPlanes,
                               ptr->bBitsPerPixel, (char *)(ptr + 1)
                        + ptr->nHeight * BITMAP_GetWidthBytes(ptr->nWidth,1) );
    oldFg = SetTextColor( hdc, RGB(0,0,0) );
    oldBg = SetBkColor( hdc, RGB(255,255,255) );

    if (hXorBits && hAndBits)
    {
        HBITMAP hBitTemp = SelectObject( hMemDC, hAndBits );
        BitBlt( hdc, x, y, ptr->nWidth, ptr->nHeight, hMemDC, 0, 0, SRCAND );
        SelectObject( hMemDC, hXorBits );
        BitBlt(hdc, x, y, ptr->nWidth, ptr->nHeight, hMemDC, 0, 0,SRCINVERT);
        SelectObject( hMemDC, hBitTemp );
    }
    DeleteDC( hMemDC );
    if (hXorBits) DeleteObject( hXorBits );
    if (hAndBits) DeleteObject( hAndBits );
    GlobalUnlock16( hIcon );
    SetTextColor( hdc, oldFg );
    SetBkColor( hdc, oldBg );
    return TRUE;
}


/***********************************************************************
 *           DumpIcon    (USER.459)
 */
DWORD WINAPI DumpIcon16( SEGPTR pInfo, WORD *lpLen,
                       SEGPTR *lpXorBits, SEGPTR *lpAndBits )
{
    CURSORICONINFO *info = PTR_SEG_TO_LIN( pInfo );
    int sizeAnd, sizeXor;

    if (!info) return 0;
    sizeXor = info->nHeight * info->nWidthBytes;
    sizeAnd = info->nHeight * BITMAP_GetWidthBytes( info->nWidth, 1 );
    if (lpAndBits) *lpAndBits = pInfo + sizeof(CURSORICONINFO);
    if (lpXorBits) *lpXorBits = pInfo + sizeof(CURSORICONINFO) + sizeAnd;
    if (lpLen) *lpLen = sizeof(CURSORICONINFO) + sizeAnd + sizeXor;
    return MAKELONG( sizeXor, sizeXor );
}


/***********************************************************************
 *           SetCursor16    (USER.69)
 */
HCURSOR16 WINAPI SetCursor16( HCURSOR16 hCursor )
{
    return (HCURSOR16)SetCursor( hCursor );
}


/***********************************************************************
 *           SetCursor32    (USER32.472)
 * RETURNS:
 *	A handle to the previous cursor shape.
 */
HCURSOR WINAPI SetCursor(
	         HCURSOR hCursor /* Handle of cursor to show */
) {
    HCURSOR hOldCursor;

    if (hCursor == hActiveCursor) return hActiveCursor;  /* No change */
    TRACE_(cursor)("%04x\n", hCursor );
    hOldCursor = hActiveCursor;
    hActiveCursor = hCursor;
    /* Change the cursor shape only if it is visible */
    if (CURSOR_ShowCount >= 0)
    {
        DISPLAY_SetCursor( (CURSORICONINFO*)GlobalLock16( hActiveCursor ) );
        GlobalUnlock16( hActiveCursor );
    }
    return hOldCursor;
}


/***********************************************************************
 *           SetCursorPos16    (USER.70)
 */
void WINAPI SetCursorPos16( INT16 x, INT16 y )
{
    SetCursorPos( x, y );
}


/***********************************************************************
 *           SetCursorPos32    (USER32.474)
 */
BOOL WINAPI SetCursorPos( INT x, INT y )
{
    DISPLAY_MoveCursor( x, y );
    return TRUE;
}


/***********************************************************************
 *           ShowCursor16    (USER.71)
 */
INT16 WINAPI ShowCursor16( BOOL16 bShow )
{
    return ShowCursor( bShow );
}


/***********************************************************************
 *           ShowCursor32    (USER32.530)
 */
INT WINAPI ShowCursor( BOOL bShow )
{
    TRACE_(cursor)("%d, count=%d\n",
                    bShow, CURSOR_ShowCount );

    if (bShow)
    {
        if (++CURSOR_ShowCount == 0)  /* Show it */
        {
            DISPLAY_SetCursor((CURSORICONINFO*)GlobalLock16( hActiveCursor ));
            GlobalUnlock16( hActiveCursor );
        }
    }
    else
    {
        if (--CURSOR_ShowCount == -1)  /* Hide it */
            DISPLAY_SetCursor( NULL );
    }
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
 *           GetCursor32    (USER32.227)
 */
HCURSOR WINAPI GetCursor(void)
{
    return hActiveCursor;
}


/***********************************************************************
 *           ClipCursor16    (USER.16)
 */
BOOL16 WINAPI ClipCursor16( const RECT16 *rect )
{
    if (!rect) SetRectEmpty( &CURSOR_ClipRect );
    else CONV_RECT16TO32( rect, &CURSOR_ClipRect );
    return TRUE;
}


/***********************************************************************
 *           ClipCursor32    (USER32.53)
 */
BOOL WINAPI ClipCursor( const RECT *rect )
{
    if (!rect) SetRectEmpty( &CURSOR_ClipRect );
    else CopyRect( &CURSOR_ClipRect, rect );
    return TRUE;
}


/***********************************************************************
 *           GetCursorPos16    (USER.17)
 */
BOOL16 WINAPI GetCursorPos16( POINT16 *pt )
{
    DWORD posX, posY, state;

    if (!pt) return 0;
    if (!EVENT_QueryPointer( &posX, &posY, &state ))
	pt->x = pt->y = 0;
    else
    {
	pt->x = posX;
	pt->y = posY;
        if (state & MK_LBUTTON)
            AsyncMouseButtonsStates[0] = MouseButtonsStates[0] = TRUE;
        else
            MouseButtonsStates[0] = FALSE;
        if (state & MK_MBUTTON)
            AsyncMouseButtonsStates[1] = MouseButtonsStates[1] = TRUE;
        else       
            MouseButtonsStates[1] = FALSE;
        if (state & MK_RBUTTON)
            AsyncMouseButtonsStates[2] = MouseButtonsStates[2] = TRUE;
        else
            MouseButtonsStates[2] = FALSE;
    }
    TRACE_(cursor)("ret=%d,%d\n", pt->x, pt->y );
    return 1;
}


/***********************************************************************
 *           GetCursorPos32    (USER32.229)
 */
BOOL WINAPI GetCursorPos( POINT *pt )
{
    BOOL ret;

    POINT16 pt16;
    ret = GetCursorPos16( &pt16 );
    if (pt) CONV_POINT16TO32( &pt16, pt );
    return ((pt) ? ret : 0);
}


/***********************************************************************
 *           GetClipCursor16    (USER.309)
 */
void WINAPI GetClipCursor16( RECT16 *rect )
{
    if (rect) CONV_RECT32TO16( &CURSOR_ClipRect, rect );
}


/***********************************************************************
 *           GetClipCursor32    (USER32.221)
 */
BOOL WINAPI GetClipCursor( RECT *rect )
{
    if (rect) 
    {
       CopyRect( rect, &CURSOR_ClipRect );
       return TRUE;
    }
    return FALSE;
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
	CURSORICONDIRENTRY* entry;
	HDC hdc;
	UINT palEnts;
	int colors;
	hdc = GetDC(0);
	palEnts = GetSystemPaletteEntries(hdc, 0, 0, NULL);
	if (palEnts == 0)
	    palEnts = 256;
	colors = (cFlag & LR_MONOCHROME) ? 2 : palEnts;

	ReleaseDC(0, hdc);

	if( bIcon )
	    entry = CURSORICON_FindBestIcon( dir, width, height, colors );
	else
	    entry = CURSORICON_FindBestCursor( dir, width, height, 1);

	if( entry ) retVal = entry->wResId;
    }
    else WARN_(cursor)("invalid resource directory\n");
    return retVal;
}

/**********************************************************************
 *          LookupIconIdFromDirectoryEx32       (USER32.380)
 */
INT WINAPI LookupIconIdFromDirectoryEx( LPBYTE dir, BOOL bIcon,
             INT width, INT height, UINT cFlag )
{
    return LookupIconIdFromDirectoryEx16( dir, bIcon, width, height, cFlag );
}

/**********************************************************************
 *          LookupIconIdFromDirectory		(USER.???)
 */
INT16 WINAPI LookupIconIdFromDirectory16( LPBYTE dir, BOOL16 bIcon )
{
    return LookupIconIdFromDirectoryEx16( dir, bIcon, 
	   bIcon ? GetSystemMetrics(SM_CXICON) : GetSystemMetrics(SM_CXCURSOR),
	   bIcon ? GetSystemMetrics(SM_CYICON) : GetSystemMetrics(SM_CYCURSOR), bIcon ? 0 : LR_MONOCHROME );
}

/**********************************************************************
 *          LookupIconIdFromDirectory		(USER32.379)
 */
INT WINAPI LookupIconIdFromDirectory( LPBYTE dir, BOOL bIcon )
{
    return LookupIconIdFromDirectoryEx( dir, bIcon, 
	   bIcon ? GetSystemMetrics(SM_CXICON) : GetSystemMetrics(SM_CXCURSOR),
	   bIcon ? GetSystemMetrics(SM_CYICON) : GetSystemMetrics(SM_CYCURSOR), bIcon ? 0 : LR_MONOCHROME );
}

/**********************************************************************
 *	    GetIconID    (USER.455)
 */
WORD WINAPI GetIconID16( HGLOBAL16 hResource, DWORD resType )
{
    LPBYTE lpDir = (LPBYTE)GlobalLock16(hResource);

    TRACE_(cursor)("hRes=%04x, entries=%i\n",
                    hResource, lpDir ? ((CURSORICONDIR*)lpDir)->idCount : 0);

    switch(resType)
    {
	case RT_CURSOR16:
	     return (WORD)LookupIconIdFromDirectoryEx16( lpDir, FALSE, 
			  GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), LR_MONOCHROME );
	case RT_ICON16:
	     return (WORD)LookupIconIdFromDirectoryEx16( lpDir, TRUE,
			  GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0 );
	default:
	     WARN_(cursor)("invalid res type %ld\n", resType );
    }
    return 0;
}

/**********************************************************************
 *          LoadCursorIconHandler    (USER.336)
 *
 * Supposed to load resources of Windows 2.x applications.
 */
HGLOBAL16 WINAPI LoadCursorIconHandler16( HGLOBAL16 hResource, HMODULE16 hModule, HRSRC16 hRsrc )
{
    FIXME_(cursor)("(%04x,%04x,%04x): old 2.x resources are not supported!\n", 
	  hResource, hModule, hRsrc);
    return (HGLOBAL16)0;
}

/**********************************************************************
 *          LoadDIBIconHandler    (USER.357)
 * 
 * RT_ICON resource loader, installed by USER_SignalProc when module
 * is initialized.
 */
HGLOBAL16 WINAPI LoadDIBIconHandler16( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    /* If hResource is zero we must allocate a new memory block, if it's
     * non-zero but GlobalLock() returns NULL then it was discarded and
     * we have to recommit some memory, otherwise we just need to check 
     * the block size. See LoadProc() in 16-bit SDK for more.
     */

     hMemObj = NE_DefResourceHandler( hMemObj, hModule, hRsrc );
     if( hMemObj )
     {
	 LPBYTE bits = (LPBYTE)GlobalLock16( hMemObj );
	 hMemObj = CURSORICON_CreateFromResource( hModule, hMemObj, bits, 
		   SizeofResource16(hModule, hRsrc), TRUE, 0x00030000, 
		   GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR );
     }
     return hMemObj;
}

/**********************************************************************
 *          LoadDIBCursorHandler    (USER.356)
 *
 * RT_CURSOR resource loader. Same as above.
 */
HGLOBAL16 WINAPI LoadDIBCursorHandler16( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    hMemObj = NE_DefResourceHandler( hMemObj, hModule, hRsrc );
    if( hMemObj )
    {
	LPBYTE bits = (LPBYTE)GlobalLock16( hMemObj );
	hMemObj = CURSORICON_CreateFromResource( hModule, hMemObj, bits,
		  SizeofResource16(hModule, hRsrc), FALSE, 0x00030000,
		  GetSystemMetrics(SM_CXCURSOR), GetSystemMetrics(SM_CYCURSOR), LR_MONOCHROME );
    }
    return hMemObj;
}

/**********************************************************************
 *	    LoadIconHandler    (USER.456)
 */
HICON16 WINAPI LoadIconHandler16( HGLOBAL16 hResource, BOOL16 bNew )
{
    LPBYTE bits = (LPBYTE)LockResource16( hResource );

    TRACE_(cursor)("hRes=%04x\n",hResource);

    return CURSORICON_CreateFromResource( 0, 0, bits, 0, TRUE, 
		      bNew ? 0x00030000 : 0x00020000, 0, 0, LR_DEFAULTCOLOR );
}

/***********************************************************************
 *           LoadCursorW            (USER32.362)
 */
HCURSOR WINAPI LoadCursorW(HINSTANCE hInstance, LPCWSTR name)
{
    return LoadImageW( hInstance, name, IMAGE_CURSOR, 0, 0, 
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
 *           LoadCursorA            (USER32.359)
 */
HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR name)
{
    return LoadImageA( hInstance, name, IMAGE_CURSOR, 0, 0, 
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
*            LoadCursorFromFileW    (USER32.361)
*/
HCURSOR WINAPI LoadCursorFromFileW (LPCWSTR name)
{
    return LoadImageW( 0, name, IMAGE_CURSOR, 0, 0, 
                       LR_LOADFROMFILE | LR_DEFAULTSIZE );
}

/***********************************************************************
*            LoadCursorFromFileA    (USER32.360)
*/
HCURSOR WINAPI LoadCursorFromFileA (LPCSTR name)
{
    return LoadImageA( 0, name, IMAGE_CURSOR, 0, 0, 
                       LR_LOADFROMFILE | LR_DEFAULTSIZE );
}
  
/***********************************************************************
 *           LoadIconW          (USER32.364)
 */
HICON WINAPI LoadIconW(HINSTANCE hInstance, LPCWSTR name)
{
    return LoadImageW( hInstance, name, IMAGE_ICON, 0, 0, 
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
 *           LoadIconA          (USER32.363)
 */
HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR name)
{
    return LoadImageA( hInstance, name, IMAGE_ICON, 0, 0, 
                       LR_SHARED | LR_DEFAULTSIZE );
}

/**********************************************************************
 *          GetIconInfo16       (USER.395)
 */
BOOL16 WINAPI GetIconInfo16(HICON16 hIcon,LPICONINFO16 iconinfo)
{
    ICONINFO	ii32;
    BOOL16	ret = GetIconInfo((HICON)hIcon, &ii32);

    iconinfo->fIcon = ii32.fIcon;
    iconinfo->xHotspot = ii32.xHotspot;
    iconinfo->yHotspot = ii32.yHotspot;
    iconinfo->hbmMask = ii32.hbmMask;
    iconinfo->hbmColor = ii32.hbmColor;
    return ret;
}

/**********************************************************************
 *          GetIconInfo32		(USER32.242)
 */
BOOL WINAPI GetIconInfo(HICON hIcon,LPICONINFO iconinfo) {
    CURSORICONINFO	*ciconinfo;

    ciconinfo = GlobalLock16(hIcon);
    if (!ciconinfo)
	return FALSE;

    if ( (ciconinfo->ptHotSpot.x == ICON_HOTSPOT) &&
	 (ciconinfo->ptHotSpot.y == ICON_HOTSPOT) )
    {
      iconinfo->fIcon    = TRUE;
      iconinfo->xHotspot = 0;
      iconinfo->yHotspot = 0;
    }
    else
    {
      iconinfo->fIcon    = FALSE;
      iconinfo->xHotspot = ciconinfo->ptHotSpot.x;
      iconinfo->yHotspot = ciconinfo->ptHotSpot.y;
    }

    iconinfo->hbmColor = CreateBitmap ( ciconinfo->nWidth, ciconinfo->nHeight,
                                ciconinfo->bPlanes, ciconinfo->bBitsPerPixel,
                                (char *)(ciconinfo + 1)
                                + ciconinfo->nHeight *
                                BITMAP_GetWidthBytes (ciconinfo->nWidth,1) );
    iconinfo->hbmMask = CreateBitmap ( ciconinfo->nWidth, ciconinfo->nHeight,
                                1, 1, (char *)(ciconinfo + 1));

    GlobalUnlock16(hIcon);

    return TRUE;
}

/**********************************************************************
 *          CreateIconIndirect		(USER32.78)
 */
HICON WINAPI CreateIconIndirect(LPICONINFO iconinfo) {
    BITMAPOBJ *bmpXor,*bmpAnd;
    HICON hObj;
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

	/* If we are creating an icon, the hotspot is unused */
	if (iconinfo->fIcon)
	{
	  info->ptHotSpot.x   = ICON_HOTSPOT;
	  info->ptHotSpot.y   = ICON_HOTSPOT;
	}
	else
	{
	  info->ptHotSpot.x   = iconinfo->xHotspot;
	  info->ptHotSpot.y   = iconinfo->yHotspot;
	}

	info->nWidth        = bmpXor->bitmap.bmWidth;
	info->nHeight       = bmpXor->bitmap.bmHeight;
	info->nWidthBytes   = bmpXor->bitmap.bmWidthBytes;
	info->bPlanes       = bmpXor->bitmap.bmPlanes;
	info->bBitsPerPixel = bmpXor->bitmap.bmBitsPixel;

	/* Transfer the bitmap bits to the CURSORICONINFO structure */

	GetBitmapBits( iconinfo->hbmMask ,sizeAnd,(char*)(info + 1) );
	GetBitmapBits( iconinfo->hbmColor,sizeXor,(char*)(info + 1) +sizeAnd);
	GlobalUnlock16( hObj );
    }
    return hObj;
}


/**********************************************************************
 *          
 DrawIconEx16		(USER.394)
 */
BOOL16 WINAPI DrawIconEx16 (HDC16 hdc, INT16 xLeft, INT16 yTop, HICON16 hIcon,
			    INT16 cxWidth, INT16 cyWidth, UINT16 istep,
			    HBRUSH16 hbr, UINT16 flags)
{
    return DrawIconEx(hdc, xLeft, yTop, hIcon, cxWidth, cyWidth,
                        istep, hbr, flags);
}


/******************************************************************************
 * DrawIconEx32 [USER32.160]  Draws an icon or cursor on device context
 *
 * NOTES
 *    Why is this using SM_CXICON instead of SM_CXCURSOR?
 *
 * PARAMS
 *    hdc     [I] Handle to device context
 *    x0      [I] X coordinate of upper left corner
 *    y0      [I] Y coordinate of upper left corner
 *    hIcon   [I] Handle to icon to draw
 *    cxWidth [I] Width of icon
 *    cyWidth [I] Height of icon
 *    istep   [I] Index of frame in animated cursor
 *    hbr     [I] Handle to background brush
 *    flags   [I] Icon-drawing flags
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DrawIconEx( HDC hdc, INT x0, INT y0, HICON hIcon,
                            INT cxWidth, INT cyWidth, UINT istep, 
                            HBRUSH hbr, UINT flags )
{
    CURSORICONINFO *ptr = (CURSORICONINFO *)GlobalLock16 (hIcon);
    HDC hDC_off = 0, hMemDC = CreateCompatibleDC (hdc);
    BOOL result = FALSE, DoOffscreen = FALSE;
    HBITMAP hB_off = 0, hOld = 0;

    if (!ptr) return FALSE;

    if (istep)
        FIXME_(icon)("Ignoring istep=%d\n", istep);
    if (flags & DI_COMPAT)
        FIXME_(icon)("Ignoring flag DI_COMPAT\n");

    /* Calculate the size of the destination image.  */
    if (cxWidth == 0)
    {
      if (flags & DI_DEFAULTSIZE)
	cxWidth = GetSystemMetrics (SM_CXICON);
      else
	cxWidth = ptr->nWidth;
    }
    if (cyWidth == 0)
    {
      if (flags & DI_DEFAULTSIZE)
        cyWidth = GetSystemMetrics (SM_CYICON);
      else
	cyWidth = ptr->nHeight;
    }

    if (!(DoOffscreen = (hbr >= STOCK_WHITE_BRUSH) && (hbr <= 
      STOCK_HOLLOW_BRUSH)))
    {
	GDIOBJHDR *object = (GDIOBJHDR *) GDI_HEAP_LOCK(hbr);
	if (object)
	{
	    UINT16 magic = object->wMagic;
	    GDI_HEAP_UNLOCK(hbr);
	    DoOffscreen = magic == BRUSH_MAGIC;
	}
    }
    if (DoOffscreen) {
      RECT r;

      r.left = 0;
      r.top = 0;
      r.right = cxWidth;
      r.bottom = cxWidth;

      hDC_off = CreateCompatibleDC(hdc);
      hB_off = CreateCompatibleBitmap(hdc, cxWidth, cyWidth);
      if (hDC_off && hB_off) {
	hOld = SelectObject(hDC_off, hB_off);
	FillRect(hDC_off, &r, hbr);
      }
    };

    if (hMemDC && (!DoOffscreen || (hDC_off && hB_off)))
    {
	HBITMAP hXorBits, hAndBits;
	COLORREF  oldFg, oldBg;
	INT     nStretchMode;

	nStretchMode = SetStretchBltMode (hdc, STRETCH_DELETESCANS);

	hXorBits = CreateBitmap ( ptr->nWidth, ptr->nHeight,
				    ptr->bPlanes, ptr->bBitsPerPixel,
				    (char *)(ptr + 1)
				    + ptr->nHeight *
				    BITMAP_GetWidthBytes(ptr->nWidth,1) );
	hAndBits = CreateBitmap ( ptr->nWidth, ptr->nHeight,
				    1, 1, (char *)(ptr+1) );
	oldFg = SetTextColor( hdc, RGB(0,0,0) );
	oldBg = SetBkColor( hdc, RGB(255,255,255) );

	if (hXorBits && hAndBits)
	{
	    HBITMAP hBitTemp = SelectObject( hMemDC, hAndBits );
	    if (flags & DI_MASK)
            {
	      if (DoOffscreen) 
		StretchBlt (hDC_off, 0, 0, cxWidth, cyWidth,
			      hMemDC, 0, 0, ptr->nWidth, ptr->nHeight, SRCAND);
	      else 
	        StretchBlt (hdc, x0, y0, cxWidth, cyWidth,
			      hMemDC, 0, 0, ptr->nWidth, ptr->nHeight, SRCAND);
            }
	    SelectObject( hMemDC, hXorBits );
	    if (flags & DI_IMAGE)
            {
	      if (DoOffscreen) 
		StretchBlt (hDC_off, 0, 0, cxWidth, cyWidth,
			  hMemDC, 0, 0, ptr->nWidth, ptr->nHeight, SRCPAINT);
	      else
		StretchBlt (hdc, x0, y0, cxWidth, cyWidth,
			      hMemDC, 0, 0, ptr->nWidth, ptr->nHeight, SRCPAINT);
            }
	    SelectObject( hMemDC, hBitTemp );
	    result = TRUE;
	}

	SetTextColor( hdc, oldFg );
	SetBkColor( hdc, oldBg );
	if (hXorBits) DeleteObject( hXorBits );
	if (hAndBits) DeleteObject( hAndBits );
	SetStretchBltMode (hdc, nStretchMode);
	if (DoOffscreen) {
	  BitBlt(hdc, x0, y0, cxWidth, cyWidth, hDC_off, 0, 0, SRCCOPY);
	  SelectObject(hDC_off, hOld);
	}
    }
    if (hMemDC) DeleteDC( hMemDC );
    if (hDC_off) DeleteDC(hDC_off);
    if (hB_off) DeleteObject(hB_off);
    GlobalUnlock16( hIcon );
    return result;
}
