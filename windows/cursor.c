/*
 *    WINE
static char Copyright[] = "Copyright  Martin Ayotte, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "library.h"
#include "neexe.h"
#include "wine.h"
#include "cursor.h"
#include "stddebug.h"
/* #define DEBUG_CURSOR   */
/* #define DEBUG_RESOURCE */
#include "debug.h"
#include "arch.h"

static int ShowCursCount = 0;
static HCURSOR hActiveCursor;
static HCURSOR hEmptyCursor = 0;
RECT	ClipCursorRect;

static struct { SEGPTR name; HCURSOR cursor; } system_cursor[] =
{
    { IDC_ARROW, 0 },
    { IDC_IBEAM, 0 },
    { IDC_WAIT, 0 },
    { IDC_CROSS, 0 },
    { IDC_UPARROW, 0 },
    { IDC_SIZE, 0 },
    { IDC_ICON, 0 },
    { IDC_SIZENWSE, 0 },
    { IDC_SIZENESW, 0 },
    { IDC_SIZEWE, 0 },
    { IDC_SIZENS, 0 }
};

#define NB_SYS_CURSORS  (sizeof(system_cursor)/sizeof(system_cursor[0]))


/**********************************************************************
 *			LoadCursor [USER.173]
 */
HCURSOR LoadCursor(HANDLE instance, SEGPTR cursor_name)
{
    HCURSOR 	hCursor;
    HRSRC       hRsrc;
    HANDLE 	rsc_mem;
    WORD 	*lp;
    LONG        *lpl,size;
    CURSORDESCRIP *lpcurdesc;
    CURSORALLOC	  *lpcur;
    HDC 	hdc;
    int i, image_size;
    unsigned char *cp1,*cp2;
    dprintf_resource(stddeb,"LoadCursor: instance = %04x, name = %08lx\n",
	   instance, cursor_name);
    if (!instance)
    {
	for (i = 0; i < NB_SYS_CURSORS; i++)
	    if (system_cursor[i].name == cursor_name)
	    {
		if (system_cursor[i].cursor) return system_cursor[i].cursor;
                else break;
	    }
	if (i == NB_SYS_CURSORS) return 0;
    }
    hCursor = GlobalAlloc(GMEM_MOVEABLE, sizeof(CURSORALLOC) + 1024L); 
    if (hCursor == (HCURSOR)NULL) return 0;
    if (!instance) system_cursor[i].cursor = hCursor;

    dprintf_cursor(stddeb,"LoadCursor Alloc hCursor=%X\n", hCursor);
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    memset(lpcur, 0, sizeof(CURSORALLOC));
    if (instance == (HANDLE)NULL) {
	switch((LONG)cursor_name) {
	    case IDC_ARROW:
		lpcur->xcursor = XCreateFontCursor(display, XC_top_left_arrow);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_CROSS:
		lpcur->xcursor = XCreateFontCursor(display, XC_crosshair);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_IBEAM:
		lpcur->xcursor = XCreateFontCursor(display, XC_xterm);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_WAIT:
		lpcur->xcursor = XCreateFontCursor(display, XC_watch);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_SIZENS:
		lpcur->xcursor = XCreateFontCursor(display, XC_sb_v_double_arrow);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_SIZEWE:
		lpcur->xcursor = XCreateFontCursor(display, XC_sb_h_double_arrow);
		GlobalUnlock(hCursor);
	    	return hCursor;
            case IDC_SIZENWSE:
            case IDC_SIZENESW:
                lpcur->xcursor = XCreateFontCursor(display, XC_fleur);
                GlobalUnlock(hCursor);
                return hCursor;
	    default:
		break;
	    }
	}

#if 0
    /* this code replaces all bitmap cursors with the default cursor */
    lpcur->xcursor = XCreateFontCursor(display, XC_top_left_arrow);
    GlobalUnlock(hCursor);
    return hCursor;
#endif

    if (!(hdc = GetDC(0))) return 0;
    if (!(hRsrc = FindResource( instance, cursor_name, RT_GROUP_CURSOR )))
    {
	ReleaseDC(0, hdc); 
	return 0;
    }
    rsc_mem = LoadResource(instance, hRsrc );
    if (rsc_mem == (HANDLE)NULL) {
    fprintf(stderr,"LoadCursor / Cursor %08lx not Found !\n", cursor_name);
	ReleaseDC(0, hdc); 
	return 0;
	}
    lp = (WORD *)LockResource(rsc_mem);
    if (lp == NULL) {
        FreeResource( rsc_mem );
	ReleaseDC(0, hdc); 
	return 0;
	}
    lpcurdesc = (CURSORDESCRIP *)(lp + 3);
#if 0
    dprintf_cursor(stddeb,"LoadCursor / image_size=%d\n", image_size);
    dprintf_cursor(stddeb,"LoadCursor / curReserved=%X\n", *lp);
    dprintf_cursor(stddeb,"LoadCursor / curResourceType=%X\n", *(lp + 1));
    dprintf_cursor(stddeb,"LoadCursor / curResourceCount=%X\n", *(lp + 2));
    dprintf_cursor(stddeb,"LoadCursor / cursor Width=%d\n", 
		(int)lpcurdesc->Width);
    dprintf_cursor(stddeb,"LoadCursor / cursor Height=%d\n", 
		(int)lpcurdesc->Height);
    dprintf_cursor(stddeb,"LoadCursor / cursor curXHotspot=%d\n", 
		(int)lpcurdesc->curXHotspot);
    dprintf_cursor(stddeb,"LoadCursor / cursor curYHotspot=%d\n", 
		(int)lpcurdesc->curYHotspot);
    dprintf_cursor(stddeb,"LoadCursor / cursor curDIBSize=%lX\n", 
		(DWORD)lpcurdesc->curDIBSize);
    dprintf_cursor(stddeb,"LoadCursor / cursor curDIBOffset=%lX\n", 
		(DWORD)lpcurdesc->curDIBOffset);
#endif
    lpcur->descriptor = *lpcurdesc;
    FreeResource( rsc_mem );
    if (!(hRsrc = FindResource( instance,
                                MAKEINTRESOURCE(lpcurdesc->curDIBOffset), 
                                RT_CURSOR )))
    {
	ReleaseDC(0, hdc); 
	return 0;
    }
    rsc_mem = LoadResource(instance, hRsrc );
    if (rsc_mem == (HANDLE)NULL) {
    	fprintf(stderr,
		"LoadCursor / Cursor %08lx Bitmap not Found !\n", cursor_name);
	ReleaseDC(0, hdc); 
	return 0;
	}
    lpl = (LONG *)LockResource(rsc_mem);
    lpl++;
    size = CONV_LONG (*lpl);
    if (size == sizeof(BITMAPCOREHEADER)){
	CONV_BITMAPCOREHEADER (lpl);
        ((BITMAPINFOHEADER *)lpl)->biHeight /= 2;
	lpcur->hBitmap = ConvertCoreBitmap( hdc, (BITMAPCOREHEADER *) lpl );
    } else if (size == sizeof(BITMAPINFOHEADER)){
	CONV_BITMAPINFO (lpl);
        ((BITMAPINFOHEADER *)lpl)->biHeight /= 2;
	lpcur->hBitmap = ConvertInfoBitmap( hdc, (BITMAPINFO *) lpl );
    } else  {
      fprintf(stderr,"No bitmap for cursor?\n");
      lpcur->hBitmap = 0;
    }
    lpl = (char *)lpl + size + 8;
  /* This is rather strange! The data is stored *BACKWARDS* and       */
  /* mirrored! But why?? FIXME: the image must be flipped at the Y    */
  /* axis, either here or in CreateCusor(); */
    size = lpcur->descriptor.Height/2 * ((lpcur->descriptor.Width+7)/8);
#if 0
    dprintf_cursor(stddeb,"Before:\n");
    for(i=0;i<2*size;i++)  {
      dprintf_cursor(stddeb,"%02x ",((unsigned char *)lpl)[i]);
      if ((i & 7) == 7) dprintf_cursor(stddeb,"\n");
    }
#endif
    cp1 = (char *)lpl;
    cp2 = cp1+2*size;
    for(i = 0; i < size; i++)  {
      char tmp=*--cp2;
      *cp2 = *cp1;
      *cp1++ = tmp;
    }
#if 0
    dprintf_cursor(stddeb,"After:\n");
    for(i=0;i<2*size;i++)  {
      dprintf_cursor(stddeb,"%02x ",((unsigned char *)lpl)[i]);
      if ((i & 7) == 7) dprintf_cursor(stddeb,"\n");
    }
#endif
    hCursor = CreateCursor(instance, lpcur->descriptor.curXHotspot, 
 	lpcur->descriptor.curYHotspot, lpcur->descriptor.Width,
	lpcur->descriptor.Height/2,
	(LPSTR)lpl, ((LPSTR)lpl)+size);

    FreeResource( rsc_mem );
    GlobalUnlock(hCursor);
    ReleaseDC(0,hdc);
    return hCursor;
}



/**********************************************************************
 *			CreateCursor [USER.406]
 */
HCURSOR CreateCursor(HANDLE instance, short nXhotspot, short nYhotspot, 
	short nWidth, short nHeight, LPSTR lpANDbitPlane, LPSTR lpXORbitPlane)
{
    HCURSOR 	hCursor;
    CURSORALLOC	  *lpcur;
    HDC 	hdc;
    int         bpllen = (nWidth + 7)/8 * nHeight;
    char        *tmpbpl = malloc(bpllen);
    int         i;
  
    XColor bkcolor,fgcolor;
    Colormap cmap = XDefaultColormap(display,XDefaultScreen(display));
    
    dprintf_resource(stddeb,"CreateCursor: inst=%04x nXhotspot=%d  nYhotspot=%d nWidth=%d nHeight=%d\n",  
       instance, nXhotspot, nYhotspot, nWidth, nHeight);
    dprintf_resource(stddeb,"CreateCursor: inst=%04x lpANDbitPlane=%p lpXORbitPlane=%p\n",
	instance, lpANDbitPlane, lpXORbitPlane);

    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;
    hCursor = GlobalAlloc(GMEM_MOVEABLE, sizeof(CURSORALLOC) + 1024L); 
    if (hCursor == (HCURSOR)NULL) {
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    dprintf_cursor(stddeb,"CreateCursor Alloc hCursor=%X\n", hCursor);
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    memset(lpcur, 0, sizeof(CURSORALLOC));
    lpcur->descriptor.curXHotspot = nXhotspot;
    lpcur->descriptor.curYHotspot = nYhotspot;
    for(i=0; i<bpllen; i++) tmpbpl[i] = ~lpANDbitPlane[i];
    lpcur->pixmask = XCreatePixmapFromBitmapData(
    	display, DefaultRootWindow(display), 
        tmpbpl, nWidth, nHeight, 1, 0, 1);
    for(i=0; i<bpllen; i++) tmpbpl[i] ^= lpXORbitPlane[i];
    lpcur->pixshape = XCreatePixmapFromBitmapData(
    	display, DefaultRootWindow(display),
        tmpbpl, nWidth, nHeight, 1, 0, 1);
    XParseColor(display,cmap,"#000000",&fgcolor);
    XParseColor(display,cmap,"#ffffff",&bkcolor);
    lpcur->xcursor = XCreatePixmapCursor(display,
 	lpcur->pixshape, lpcur->pixmask, 
 	&fgcolor, &bkcolor, lpcur->descriptor.curXHotspot, 
 	lpcur->descriptor.curYHotspot);
    free(tmpbpl);
    XFreePixmap(display, lpcur->pixshape);
    XFreePixmap(display, lpcur->pixmask);
    ReleaseDC(GetDesktopWindow(), hdc); 
    GlobalUnlock(hCursor);
    return hCursor;
}



/**********************************************************************
 *			DestroyCursor [USER.458]
 */
BOOL DestroyCursor(HCURSOR hCursor)
{
    CURSORALLOC	*lpcur;
    if (hCursor == (HCURSOR)NULL) return FALSE;
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    if (lpcur->hBitmap != (HBITMAP)NULL) DeleteObject(lpcur->hBitmap);
    GlobalUnlock(hCursor);
    GlobalFree(hCursor);
    return TRUE;
}


/**********************************************************************
 *         CURSOR_SetCursor
 *
 * Internal helper function for SetCursor() and ShowCursor().
 */
static void CURSOR_SetCursor( HCURSOR hCursor )
{
    CURSORALLOC	*lpcur;

    if (!(lpcur = (CURSORALLOC *)GlobalLock(hCursor))) return;
    if (rootWindow != DefaultRootWindow(display))
    {
        XDefineCursor( display, rootWindow, lpcur->xcursor );
    }
    else
    {
        HWND hwnd = GetWindow( GetDesktopWindow(), GW_CHILD );
        while(hwnd)
        {
            Window win = WIN_GetXWindow( hwnd );
            if (win) XDefineCursor( display, win, lpcur->xcursor );
            hwnd = GetWindow( hwnd, GW_HWNDNEXT );
        }
    }
    GlobalUnlock( hCursor );
}

/**********************************************************************
 *			SetCursor [USER.69]
 */
HCURSOR SetCursor(HCURSOR hCursor)
{
    HCURSOR hOldCursor;

    dprintf_cursor(stddeb,"SetCursor / hCursor=%04X !\n", hCursor);
    hOldCursor = hActiveCursor;
    hActiveCursor = hCursor;
    if ((hCursor != hOldCursor) || (ShowCursCount < 0))
    {
        CURSOR_SetCursor( hCursor );
    }
    ShowCursCount = 0;
    return hOldCursor;
}


/**********************************************************************
 *			GetCursor [USER.247]
 */
HCURSOR GetCursor(void)
{
    return hActiveCursor;
}


/**********************************************************************
 *                        SetCursorPos [USER.70]
 */
void SetCursorPos(short x, short y)
{
    dprintf_cursor(stddeb,"SetCursorPos // x=%d y=%d\n", x, y);
    XWarpPointer( display, None, rootWindow, 0, 0, 0, 0, x, y );
}


/**********************************************************************
 *                        GetCursorPos [USER.17]
 */
void GetCursorPos(LPPOINT lpRetPoint)
{
    Window 	root, child;
    int		rootX, rootY;
    int		childX, childY;
    unsigned int mousebut;

    if (!lpRetPoint) return;
    if (!XQueryPointer( display, rootWindow, &root, &child,
		        &rootX, &rootY, &childX, &childY, &mousebut ))
	lpRetPoint->x = lpRetPoint->y = 0;
    else
    {
	lpRetPoint->x = rootX + desktopX;
	lpRetPoint->y = rootY + desktopY;
    }
    dprintf_cursor(stddeb,
		"GetCursorPos // x=%d y=%d\n", lpRetPoint->x, lpRetPoint->y);
}


/**********************************************************************
 *			ShowCursor [USER.71]
 */
int ShowCursor(BOOL bShow)
{
    dprintf_cursor(stddeb, "ShowCursor(%d), count=%d\n", bShow, ShowCursCount);

    if (bShow)
    {
        if (++ShowCursCount == 0)  /* Time to show it */
            CURSOR_SetCursor( hActiveCursor );
    }
    else  /* Hide it */
    {
        if (--ShowCursCount == -1)  /* Time to hide it */
        {
            if (!hEmptyCursor)
                hEmptyCursor = CreateCursor( 0, 1, 1, 1, 1,
                                             "\xFF\xFF", "\xFF\xFF" );
            CURSOR_SetCursor( hEmptyCursor );
        }
    }
    return 0;
}


/**********************************************************************
 *                        ClipCursor [USER.16]
 */
void ClipCursor(LPRECT lpNewClipRect)
{
    if (!lpNewClipRect) SetRectEmpty( &ClipCursorRect );
    else CopyRect( &ClipCursorRect, lpNewClipRect );
}


/**********************************************************************
 *                        GetClipCursor [USER.309]
 */
void GetClipCursor(LPRECT lpRetClipRect)
{
    if (lpRetClipRect != NULL)
	CopyRect(lpRetClipRect, &ClipCursorRect);
}




