/*
 *    WINE
*/
static char Copyright[] = "Copyright  Martin Ayotte, 1993";

/*
#define DEBUG_CURSOR
*/

#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "wine.h"
#include "cursor.h"

static int ShowCursCount = 0;
static HCURSOR hActiveCursor;
static HCURSOR hEmptyCursor = 0;
RECT	ClipCursorRect;
extern HINSTANCE hSysRes;
extern Window winHasCursor;

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
    BITMAP 	BitMap;
    HBITMAP 	hBitMap;
    HDC 	hMemDC;
    HDC 	hdc;
    int i, j, image_size;
#ifdef DEBUG_RESOURCE
    printf("LoadCursor: instance = %04x, name = %08x\n",
	   instance, cursor_name);
#endif    

    if (!instance)
    {
	for (i = 0; i < NB_SYS_CURSORS; i++)
	    if (system_cursor[i].name == cursor_name)
	    {
		hCursor = system_cursor[i].cursor;
		break;
	    }
	if (i == NB_SYS_CURSORS) return 0;
	if (hCursor) return hCursor;
    }
    hCursor = GlobalAlloc(GMEM_MOVEABLE, sizeof(CURSORALLOC) + 1024L); 
    if (hCursor == (HCURSOR)NULL) return 0;
    if (!instance) system_cursor[i].cursor = hCursor;

#ifdef DEBUG_CURSOR
    printf("LoadCursor Alloc hCursor=%X\n", hCursor);
#endif    
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    memset(lpcur, 0, sizeof(CURSORALLOC));
    if (instance == (HANDLE)NULL) {
	instance = hSysRes;
	switch((LONG)cursor_name) {
	    case IDC_ARROW:
		lpcur->xcursor = XCreateFontCursor(XT_display, XC_top_left_arrow);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_CROSS:
		lpcur->xcursor = XCreateFontCursor(XT_display, XC_crosshair);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_IBEAM:
		lpcur->xcursor = XCreateFontCursor(XT_display, XC_xterm);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_WAIT:
		lpcur->xcursor = XCreateFontCursor(XT_display, XC_watch);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_SIZENS:
		lpcur->xcursor = XCreateFontCursor(XT_display, XC_sb_v_double_arrow);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    case IDC_SIZEWE:
		lpcur->xcursor = XCreateFontCursor(XT_display, XC_sb_h_double_arrow);
		GlobalUnlock(hCursor);
	    	return hCursor;
	    default:
		break;
	    }
	}
    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;
    rsc_mem = RSC_LoadResource(instance, cursor_name, NE_RSCTYPE_GROUP_CURSOR, 
			       &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadCursor / Cursor %08X not Found !\n", cursor_name);
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
#ifdef DEBUG_CURSOR
    printf("LoadCursor / image_size=%d\n", image_size);
    printf("LoadCursor / curReserved=%X\n", *lp);
    printf("LoadCursor / curResourceType=%X\n", *(lp + 1));
    printf("LoadCursor / curResourceCount=%X\n", *(lp + 2));
    printf("LoadCursor / cursor Width=%d\n", (int)lpcurdesc->Width);
    printf("LoadCursor / cursor Height=%d\n", (int)lpcurdesc->Height);
    printf("LoadCursor / cursor curXHotspot=%d\n", (int)lpcurdesc->curXHotspot);
    printf("LoadCursor / cursor curYHotspot=%d\n", (int)lpcurdesc->curYHotspot);
    printf("LoadCursor / cursor curDIBSize=%lX\n", (DWORD)lpcurdesc->curDIBSize);
    printf("LoadCursor / cursor curDIBOffset=%lX\n", (DWORD)lpcurdesc->curDIBOffset);
#endif
    lpcur->descriptor = *lpcurdesc;
    GlobalUnlock(rsc_mem);
    GlobalFree(rsc_mem);
    rsc_mem = RSC_LoadResource(instance, 
    	MAKEINTRESOURCE(lpcurdesc->curDIBOffset), 
    	NE_RSCTYPE_CURSOR, &image_size);
    if (rsc_mem == (HANDLE)NULL) {
	printf("LoadCursor / Cursor %08X Bitmap not Found !\n", cursor_name);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    lp = (WORD *)GlobalLock(rsc_mem);
    if (lp == NULL) {
	GlobalFree(rsc_mem);
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
 	}
    lp += 2;
    for (j = 0; j < 16; j++)
    	printf("%04X ", *(lp + j));
/*
    if (*lp == sizeof(BITMAPINFOHEADER))
	lpcur->hBitmap = ConvertInfoBitmap(hdc, (BITMAPINFO *)lp);
    else
*/
        lpcur->hBitmap = 0;
    lp += sizeof(BITMAP);
    for (i = 0; i < 81; i++) {
	char temp = *((char *)lp + 162 + i);
	*((char *)lp + 162 + i) = *((char *)lp + 324 - i);
	*((char *)lp + 324 - i) = temp;
	}
    lpcur->pixshape = XCreatePixmapFromBitmapData(
    	XT_display, DefaultRootWindow(XT_display), 
        ((char *)lp + 211), 32, 32,
/*
        lpcurdesc->Width / 2, lpcurdesc->Height / 4, 
*/
        WhitePixel(XT_display, DefaultScreen(XT_display)), 
        BlackPixel(XT_display, DefaultScreen(XT_display)), 1);
    lpcur->pixmask = XCreatePixmapFromBitmapData(
    	XT_display, DefaultRootWindow(XT_display), 
        ((char *)lp + 211), 32, 32,
        WhitePixel(XT_display, DefaultScreen(XT_display)), 
        BlackPixel(XT_display, DefaultScreen(XT_display)), 1);
    memset(&bkcolor, 0, sizeof(XColor));
    memset(&fgcolor, 0, sizeof(XColor));
    bkcolor.pixel = WhitePixel(XT_display, DefaultScreen(XT_display)); 
    fgcolor.pixel = BlackPixel(XT_display, DefaultScreen(XT_display));
printf("LoadCursor / before XCreatePixmapCursor !\n");
    lpcur->xcursor = XCreatePixmapCursor(XT_display,
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
    XFreePixmap(XT_display, lpcur->pixshape);
    XFreePixmap(XT_display, lpcur->pixmask);
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
    BITMAP 	BitMap;
    HBITMAP 	hBitMap;
    HDC 	hMemDC;
    HDC 	hdc;
    int i, j;
#ifdef DEBUG_RESOURCE
    printf("CreateCursor: inst=%04x nXhotspot=%d  nYhotspot=%d nWidth=%d nHeight=%d\n",  
       instance, nXhotspot, nYhotspot, nWidth, nHeight);
    printf("CreateCursor: inst=%04x lpANDbitPlane=%08X lpXORbitPlane=%08X\n",
	instance, lpANDbitPlane, lpXORbitPlane);
#endif    
    if (!(hdc = GetDC(GetDesktopWindow()))) return 0;
    hCursor = GlobalAlloc(GMEM_MOVEABLE, sizeof(CURSORALLOC) + 1024L); 
    if (hCursor == (HCURSOR)NULL) {
	ReleaseDC(GetDesktopWindow(), hdc); 
	return 0;
	}
    printf("CreateCursor Alloc hCursor=%X\n", hCursor);
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    memset(lpcur, 0, sizeof(CURSORALLOC));
    lpcur->descriptor.curXHotspot = nXhotspot;
    lpcur->descriptor.curYHotspot = nYhotspot;
    lpcur->pixshape = XCreatePixmapFromBitmapData(
    	XT_display, DefaultRootWindow(XT_display), 
        lpXORbitPlane, nWidth, nHeight,
        WhitePixel(XT_display, DefaultScreen(XT_display)), 
        BlackPixel(XT_display, DefaultScreen(XT_display)), 1);
    lpcur->pixmask = XCreatePixmapFromBitmapData(
    	XT_display, DefaultRootWindow(XT_display), 
        lpANDbitPlane, nWidth, nHeight,
        WhitePixel(XT_display, DefaultScreen(XT_display)), 
        BlackPixel(XT_display, DefaultScreen(XT_display)), 1);
    memset(&bkcolor, 0, sizeof(XColor));
    memset(&fgcolor, 0, sizeof(XColor));
    bkcolor.pixel = WhitePixel(XT_display, DefaultScreen(XT_display)); 
    fgcolor.pixel = BlackPixel(XT_display, DefaultScreen(XT_display));
    lpcur->xcursor = XCreatePixmapCursor(XT_display,
 	lpcur->pixshape, lpcur->pixmask, 
 	&fgcolor, &bkcolor, lpcur->descriptor.curXHotspot, 
 	lpcur->descriptor.curYHotspot);
    XFreePixmap(XT_display, lpcur->pixshape);
    XFreePixmap(XT_display, lpcur->pixmask);
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
 *			SetCursor [USER.69]
 */
HCURSOR SetCursor(HCURSOR hCursor)
{
    HDC		hDC;
    HDC		hMemDC;
    BITMAP	bm;
    CURSORALLOC	*lpcur;
    HCURSOR	hOldCursor;
    Window 	root, child;
    int		rootX, rootY;
    int		childX, childY;
    unsigned int mousebut;
#ifdef DEBUG_CURSOR
    printf("SetCursor / hCursor=%04X !\n", hCursor);
#endif
    if (hCursor == (HCURSOR)NULL) return FALSE;
    lpcur = (CURSORALLOC *)GlobalLock(hCursor);
    hOldCursor = hActiveCursor;
#ifdef DEBUG_CURSOR
    printf("SetCursor / lpcur->xcursor=%08X !\n", &lpcur->xcursor);
    XQueryPointer(XT_display, DefaultRootWindow(XT_display),
	&root, &child, &rootX, &rootY, &childX, &childY, &mousebut);
    printf("SetCursor / winHasCursor=%08X !\n", winHasCursor);
    printf("SetCursor / child=%08X !\n", child);
#endif
    if (hActiveCursor != hCursor) ShowCursCount = 0;
    if ((ShowCursCount >= 0) & (winHasCursor != 0)) {
/*	XUndefineCursor(XT_display, winHasCursor); */
	XDefineCursor(XT_display, winHasCursor, lpcur->xcursor);
	}
    GlobalUnlock(hCursor);
    hActiveCursor = hCursor;
    return hOldCursor;
}


/**********************************************************************
 *                        SetCursorPos [USER.70]
 */
void SetCursorPos(short x, short y)
{
    Window 	root, child;
    int		rootX, rootY;
    int		childX, childY;
    unsigned int mousebut;
#ifdef DEBUG_CURSOR
    printf("SetCursorPos // x=%d y=%d\n", x, y);
#endif
    XQueryPointer(XT_display, DefaultRootWindow(XT_display),
	&root, &child, &rootX, &rootY, &childX, &childY, &mousebut);
    XWarpPointer(XT_display, child, root, 0, 0, 
    DisplayWidth(XT_display, DefaultScreen(XT_display)), 
    DisplayHeight(XT_display, DefaultScreen(XT_display)), 
    (int)x, (int)y);
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
    if (lpRetPoint != NULL) {
	XQueryPointer(XT_display, DefaultRootWindow(XT_display),
	    &root, &child, &rootX, &rootY, &childX, &childY, &mousebut);
#ifdef DEBUG_CURSOR
	printf("GetCursorPos // x=%d y=%d\n", rootX, rootY);
#endif
	lpRetPoint->x = rootX;
	lpRetPoint->y = rootY;
    	}
}


/**********************************************************************
 *			ShowCursor [USER.71]
 */
int ShowCursor(BOOL bShow)
{
    HCURSOR	hCursor;
#ifdef DEBUG_CURSOR
	printf("ShowCursor bShow=%d ShowCount=%d !\n", bShow, ShowCursCount);
#endif
    if (bShow)
	ShowCursCount++;
    else
	ShowCursCount--;
    if (ShowCursCount >= 0) {
/*    	if (hCursor == (HCURSOR)NULL) */
    	    hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
	SetCursor(hCursor);
	}
    else {
/*	XUndefineCursor(XT_display, winHasCursor); */
	if (hEmptyCursor == (HCURSOR)NULL)
	    hEmptyCursor = CreateCursor((HINSTANCE)NULL, 1, 1, 1, 1, 
	    				"\xFF\xFF", "\xFF\xFF");
	hCursor = SetCursor(hEmptyCursor);
	hActiveCursor = hCursor;
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




