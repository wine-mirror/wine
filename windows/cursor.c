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
#include <X11/Xutil.h>
#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "neexe.h"
#include "wine.h"
#include "callback.h"
#include "cursor.h"
#include "resource.h"
#include "stddebug.h"
#include "debug.h"
#include "arch.h"
#include "bitmap.h"

static int ShowCursCount = 0;
static HCURSOR hActiveCursor;
static HCURSOR hEmptyCursor = 0;
RECT	ClipCursorRect;

static struct { 
   SEGPTR name; HCURSOR cursor; unsigned int shape;
} system_cursor[] =
{
    { IDC_ARROW,    0, XC_top_left_arrow},
    { IDC_IBEAM,    0, XC_xterm },
    { IDC_WAIT,     0, XC_watch },
    { IDC_CROSS,    0, XC_crosshair },
    { IDC_UPARROW,  0, XC_based_arrow_up },
    { IDC_SIZE,     0, XC_bottom_right_corner },
    { IDC_ICON,     0, XC_icon },
    { IDC_SIZENWSE, 0, XC_fleur },
    { IDC_SIZENESW, 0, XC_fleur },
    { IDC_SIZEWE,   0, XC_sb_h_double_arrow },
    { IDC_SIZENS,   0, XC_sb_v_double_arrow }
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
    char        *lpbits;
    LONG        *lpl,size;
    CURSORALLOC *lpcur;
    CURSORDIR   *lpcurdir;
    CURSORDESCRIP curdesc;
    POINT       hotspot;
    int i;
    dprintf_resource(stddeb,"LoadCursor: instance = %04x, name = %08lx\n",
	   instance, cursor_name);
    if (!instance)
    {
	for (i = 0; i < NB_SYS_CURSORS; i++)
	    if (system_cursor[i].name == cursor_name)
	    {
	        if (!system_cursor[i].cursor) 
	        {
		    hCursor = GlobalAlloc (GMEM_MOVEABLE, sizeof (Cursor));
		    dprintf_cursor(stddeb,"LoadCursor Alloc hCursor=%X\n", hCursor);
		    system_cursor[i].cursor = hCursor;
		    lpcur = (CURSORALLOC *) GlobalLock(hCursor);
		    lpcur->xcursor = XCreateFontCursor(display, system_cursor[i].shape);
		    GlobalUnlock(hCursor);
	        }
	       	return system_cursor[i].cursor;
	    }
	return 0;
    }

    if (!(hRsrc = FindResource( instance, cursor_name, RT_GROUP_CURSOR )))
	return 0;
    rsc_mem = LoadResource(instance, hRsrc );
    if (rsc_mem == (HANDLE)NULL) {
        fprintf(stderr,"LoadCursor / Cursor %08lx not Found !\n", cursor_name);
	return 0;
    }
    lpcurdir = (CURSORDIR *)LockResource(rsc_mem);
    if (lpcurdir == NULL) {
        FreeResource( rsc_mem );
	return 0;
    }
    curdesc = *(CURSORDESCRIP *)(lpcurdir + 1);  /* CONV_CURDESC ? */
#if 0
    dprintf_cursor(stddeb,"LoadCursor / curReserved=%X\n", lpcurdir->cdReserved);
    dprintf_cursor(stddeb,"LoadCursor / curResourceType=%X\n", lpcurdir->cdType);
    dprintf_cursor(stddeb,"LoadCursor / curResourceCount=%X\n", lpcurdir->cdCount);
    dprintf_cursor(stddeb,"LoadCursor / cursor Width=%d\n", 
		(int)curdesc.Width);
    dprintf_cursor(stddeb,"LoadCursor / cursor Height=%d\n", 
		(int)curdesc.Height);
    dprintf_cursor(stddeb,"LoadCursor / cursor curDIBSize=%lX\n", 
		(DWORD)curdesc.curDIBSize);
    dprintf_cursor(stddeb,"LoadCursor / cursor curDIBOffset=%lX\n",
		(DWORD)curdesc.curDIBOffset);
#endif
    FreeResource( rsc_mem );
    if (!(hRsrc = FindResource( instance,
                                MAKEINTRESOURCE(curdesc.curDIBOffset), 
                                RT_CURSOR )))
	return 0;
    rsc_mem = LoadResource(instance, hRsrc );
    if (rsc_mem == (HANDLE)NULL) {
    	fprintf(stderr,
		"LoadCursor / Cursor %08lx Bitmap not Found !\n", cursor_name);
	return 0;
    }
    lpl = (LONG *) LockResource(rsc_mem);
    if (lpl == NULL) {
        FreeResource( rsc_mem );
	return 0;
	}
    hotspot = *(POINT *) lpl++;  /* CONV_POINT ? */
    size = CONV_LONG (*lpl);
#if 0
    if (!(hdc = GetDC(0))) {
       FreeResource( rsc_mem );
       return 0;
    }
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
    ReleaseDC(0,hdc);
#endif
    if (size == sizeof(BITMAPCOREHEADER))
	lpbits = (char *)lpl + sizeof(BITMAPCOREHEADER) + 2*sizeof(RGBTRIPLE);
    else if (size == sizeof(BITMAPINFOHEADER))
	lpbits = (char *)lpl + sizeof(BITMAPINFOHEADER) + 2*sizeof(RGBQUAD);
    else  {
      fprintf(stderr,"Invalid bitmap in cursor resource\n");
      FreeResource(rsc_mem);
      return 0;
    }
    /* The height of the cursor is the height of the XOR-Bitmap
     * plus the height of the AND-Bitmap, so divide it by two.
     */
    size = curdesc.Height/2 * ((curdesc.Width+31)/32 * 4);
#if 0
    dprintf_cursor(stddeb,"Bitmap:\n");
    for(i=0;i<2*size;i++)  {
      dprintf_cursor(stddeb,"%02x ",(unsigned char) lpc[i]);
      if ((i & 7) == 7) dprintf_cursor(stddeb,"\n");
    }
#endif
    hCursor = CreateCursor(instance, hotspot.x, hotspot.y, 
			   curdesc.Width, curdesc.Height / 2,
			   lpbits+size, lpbits);
    FreeResource( rsc_mem );
    return hCursor;
}



/***********************************************************************
 *           CreateCursorIconIndirect           (USER.408)
 *
 * Returns handle to either an icon or a cursor. Used by CreateCursor
 * and CreateIcon in  Windoze, but will use same in this version.
 */
HANDLE CreateCursorIconIndirect(HANDLE hInstance, LPCURSORICONINFO lpInfo,
                                LPSTR lpANDBits, /* bitmap data */
                                LPSTR lpXORBits /* masking data */)
{
        return CreateIcon(hInstance,
                lpInfo->nWidth, lpInfo->nHeight,
                lpInfo->byPlanes, lpInfo->byBitsPix,
                lpANDBits, lpXORBits);
}


/**********************************************************************
 *			CreateCursor [USER.406]
 */
HCURSOR CreateCursor(HANDLE instance, short nXhotspot, short nYhotspot, 
	short nWidth, short nHeight, LPSTR lpANDbitPlane, LPSTR lpXORbitPlane)
{
    HCURSOR	hCursor;
    CURSORALLOC *lpcur;
    int		bpl = (nWidth + 31)/32 * 4;
    char	*tmpbits = malloc(bpl * nHeight);
    char	*src, *dst;
    int		i;
    XImage	*image;
    Pixmap	pixshape, pixmask;
    extern void _XInitImageFuncPtrs( XImage* );
  
    XColor bkcolor,fgcolor;
    Colormap cmap = XDefaultColormap(display,XDefaultScreen(display));
    
    dprintf_resource(stddeb, "CreateCursor: inst=%04x nXhotspot=%d  nYhotspot=%d nWidth=%d nHeight=%d\n",  
       instance, nXhotspot, nYhotspot, nWidth, nHeight);
    dprintf_resource(stddeb,"CreateCursor: inst=%04x lpANDbitPlane=%p lpXORbitPlane=%p\n",
	instance, lpANDbitPlane, lpXORbitPlane);

    image = XCreateImage( display, DefaultVisualOfScreen(screen),
                          1, ZPixmap, 0, tmpbits,
                          nWidth, nHeight, 32, bpl );
    if (!image) {
	free (tmpbits);
	return 0;
    }
    image->byte_order = MSBFirst;
    image->bitmap_bit_order = MSBFirst;
    image->bitmap_unit = 16;
    _XInitImageFuncPtrs(image);

    hCursor = GlobalAlloc(GMEM_MOVEABLE, sizeof(Cursor)); 
    if (hCursor == (HCURSOR)NULL) {
	XDestroyImage(image);
	return 0;
    }
    dprintf_cursor(stddeb,"CreateCursor Alloc hCursor=%X\n", hCursor);
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);

    for(src=lpANDbitPlane, dst=tmpbits+(nHeight-1)*bpl; 
	dst>=tmpbits; src+=bpl, dst-=bpl)
        for(i=0; i<bpl; i++) 
            dst[i] = ~src[i];
    pixmask = XCreatePixmap(display, DefaultRootWindow(display), 
			    nWidth, nHeight, 1);
    CallTo32_LargeStack(XPutImage, 10,
			display, pixmask, BITMAP_monoGC, image, 
			0, 0, 0, 0, nWidth, nHeight );

    for(src=lpXORbitPlane, dst=tmpbits+(nHeight-1)*bpl; 
	dst>=tmpbits; src+=bpl, dst-=bpl)
        for(i=0; i<bpl; i++) 
            dst[i] = ~src[i];
    pixshape = XCreatePixmap(display, DefaultRootWindow(display), 
			     nWidth, nHeight, 1);
    CallTo32_LargeStack(XPutImage, 10,
			display, pixshape, BITMAP_monoGC, image, 
			0, 0, 0, 0, nWidth, nHeight );
    XParseColor(display,cmap,"#000000",&fgcolor);
    XParseColor(display,cmap,"#ffffff",&bkcolor);
    lpcur->xcursor = XCreatePixmapCursor(display, pixshape, pixmask,
			                 &fgcolor, &bkcolor, nXhotspot, nYhotspot);
    XFreePixmap(display, pixshape);
    XFreePixmap(display, pixmask);
    GlobalUnlock(hCursor);
    XDestroyImage(image);
    return hCursor;
}



/**********************************************************************
 *			DestroyCursor [USER.458]
 */
BOOL DestroyCursor(HCURSOR hCursor)
{
    int i;
    CURSORALLOC	*lpcur;
    
    if (hCursor == 0) return FALSE;
    for (i = 0; i < NB_SYS_CURSORS; i++) {
	if (system_cursor[i].cursor == hCursor) return TRUE;
    }
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    XFreeCursor (display, lpcur->xcursor);
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
					    "\xFF\xFF\xFF\xFF", "\xFF\xFF\xFF\xFF" );
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
