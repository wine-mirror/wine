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

static int ShowCursCount = 0;
static HCURSOR hActiveCursor;
static HCURSOR hEmptyCursor = 0;
RECT	ClipCursorRect;

static struct { LPSTR name; HCURSOR cursor; } system_cursor[] =
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
HCURSOR LoadCursor(HANDLE instance, LPSTR cursor_name)
{
    XColor	bkcolor;
    XColor	fgcolor;
    HCURSOR 	hCursor;
    HANDLE 	rsc_mem;
    WORD 	*lp;
    CURSORDESCRIP *lpcurdesc;
    CURSORALLOC	  *lpcur;
    HDC 	hdc;
    int i, j, image_size;

    dprintf_resource(stddeb,"LoadCursor: instance = %04x, name = %p\n",
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
	instance = hSysRes;
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

#if 1
    lpcur->xcursor = XCreateFontCursor(display, XC_top_left_arrow);
    GlobalUnlock(hCursor);
    return hCursor;
#endif

    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;
    rsc_mem = RSC_LoadResource(instance, cursor_name, NE_RSCTYPE_GROUP_CURSOR, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
    fprintf(stderr,"LoadCursor / Cursor %p not Found !\n", cursor_name);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    lpcurdesc = (CURSORDESCRIP *)(lp + 3);
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
    lpcur->descriptor = *lpcurdesc;
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
    rsc_mem = RSC_LoadResource(instance, 
    	MAKEINTRESOURCE(lpcurdesc->curDIBOffset), 
    	NE_RSCTYPE_CURSOR, &image_size);
    if (rsc_mem == (HANDLE)NULL) {
    	fprintf(stderr,
		"LoadCursor / Cursor %p Bitmap not Found !\n", cursor_name);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
 	}
	lp++;
    for (j = 0; j < 16; j++)
        dprintf_cursor(stddeb,"%04X ", *(lp + j));
/*
    if (*lp == sizeof(BITMAPINFOHEADER))
	lpcur->hBitmap = ConvertInfoBitmap(hdc, (BITMAPINFO *)lp);
    else
*/
        lpcur->hBitmap = 0;
/*     lp += sizeof(BITMAP); */
    for (i = 0; i < 81; i++) {
	char temp = *((char *)lp + 162 + i);
	*((char *)lp + 162 + i) = *((char *)lp + 324 - i);
	*((char *)lp + 324 - i) = temp;
	}
    lpcur->pixshape = XCreatePixmapFromBitmapData(
    	display, DefaultRootWindow(display), 
        ((char *)lp + 211), 32, 32,
/*
        lpcurdesc->Width / 2, lpcurdesc->Height / 4, 
*/
        WhitePixel(display, DefaultScreen(display)), 
        BlackPixel(display, DefaultScreen(display)), 1);
    lpcur->pixmask = XCreatePixmapFromBitmapData(
    	display, DefaultRootWindow(display), 
        ((char *)lp + 211), 32, 32,
        WhitePixel(display, DefaultScreen(display)), 
        BlackPixel(display, DefaultScreen(display)), 1);
    memset(&bkcolor, 0, sizeof(XColor));
    memset(&fgcolor, 0, sizeof(XColor));
    bkcolor.pixel = WhitePixel(display, DefaultScreen(display)); 
    fgcolor.pixel = BlackPixel(display, DefaultScreen(display));
    dprintf_cursor(stddeb,"LoadCursor / before XCreatePixmapCursor !\n");
    lpcur->xcursor = XCreatePixmapCursor(display,
 	lpcur->pixshape, lpcur->pixmask, 
 	&fgcolor, &bkcolor, lpcur->descriptor.curXHotspot, 
 	lpcur->descriptor.curYHotspot);
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
/*
    hCursor = CreateCursor(instance, lpcur->descriptor.curXHotspot, 
 	lpcur->descriptor.curYHotspot, 32, 32,
	(LPSTR)lp + 211, , (LPSTR)lp + 211);
*/
    XFreePixmap(display, lpcur->pixshape);
    XFreePixmap(display, lpcur->pixmask);
    ReleaseDC(GetDesktopWindow(), hdc); 
    GlobalUnlock(hCursor);
    return hCursor;
}



/**********************************************************************
 *			CreateCursor [USER.406]
 */
HCURSOR CreateCursor(HANDLE instance, short nXhotspot, short nYhotspot, 
	short nWidth, short nHeight, LPSTR lpANDbitPlane, LPSTR lpXORbitPlane)
{
    XColor	bkcolor;
    XColor	fgcolor;
    HCURSOR 	hCursor;
    CURSORALLOC	  *lpcur;
    HDC 	hdc;

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
    lpcur->pixshape = XCreatePixmapFromBitmapData(
    	display, DefaultRootWindow(display), 
        lpXORbitPlane, nWidth, nHeight,
        WhitePixel(display, DefaultScreen(display)), 
        BlackPixel(display, DefaultScreen(display)), 1);
    lpcur->pixmask = XCreatePixmapFromBitmapData(
    	display, DefaultRootWindow(display), 
        lpANDbitPlane, nWidth, nHeight,
        WhitePixel(display, DefaultScreen(display)), 
        BlackPixel(display, DefaultScreen(display)), 1);
    memset(&bkcolor, 0, sizeof(XColor));
    memset(&fgcolor, 0, sizeof(XColor));
    bkcolor.pixel = WhitePixel(display, DefaultScreen(display)); 
    fgcolor.pixel = BlackPixel(display, DefaultScreen(display));
    lpcur->xcursor = XCreatePixmapCursor(display,
 	lpcur->pixshape, lpcur->pixmask, 
 	&fgcolor, &bkcolor, lpcur->descriptor.curXHotspot, 
 	lpcur->descriptor.curYHotspot);
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
    CopyRect(&ClipCursorRect, lpNewClipRect);
}


/**********************************************************************
 *                        GetClipCursor [USER.309]
 */
void GetClipCursor(LPRECT lpRetClipRect)
{
    if (lpRetClipRect != NULL)
	CopyRect(lpRetClipRect, &ClipCursorRect);
}




