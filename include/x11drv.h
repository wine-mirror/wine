/*
 * X11 display driver definitions
 */

#ifndef __WINE_X11DRV_H
#define __WINE_X11DRV_H

#include "config.h"

#ifndef X_DISPLAY_MISSING
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif /* !defined(X_DISPLAY_MISSING) */

#include "winbase.h"
#include "gdi.h"
#include "windef.h"

struct tagCLASS;
struct tagDC;
struct tagDeviceCaps;
struct tagWND;
struct tagCREATESTRUCTA;
struct tagWINDOWPOS;
struct tagCURSORICONINFO;

  /* X physical pen */
typedef struct
{
    int          style;
    int          endcap;
    int          linejoin;
    int          pixel;
    int          width;
    char *       dashes;
    int          dash_len;
    int          type;          /* GEOMETRIC || COSMETIC */
} X_PHYSPEN;

  /* X physical brush */
typedef struct
{
    int          style;
    int          fillStyle;
    int          pixel;
    Pixmap       pixmap;
} X_PHYSBRUSH;

  /* X physical font */
typedef UINT	 X_PHYSFONT;

  /* X physical device */
typedef struct
{
    GC            gc;          /* X Window GC */
    Drawable      drawable;
    X_PHYSFONT    font;
    X_PHYSPEN     pen;
    X_PHYSBRUSH   brush;
    int           backgroundPixel;
    int           textPixel;
} X11DRV_PDEVICE;


typedef struct {
    Pixmap	pixmap;
} X11DRV_PHYSBITMAP;

  /* GCs used for B&W and color bitmap operations */
extern GC BITMAP_monoGC, BITMAP_colorGC;

#define BITMAP_GC(bmp) \
  (((bmp)->bitmap.bmBitsPixel == 1) ? BITMAP_monoGC : BITMAP_colorGC)


/* Wine driver X11 functions */

extern BOOL X11DRV_Init(void);
extern BOOL X11DRV_BitBlt( struct tagDC *dcDst, INT xDst, INT yDst,
                             INT width, INT height, struct tagDC *dcSrc,
                             INT xSrc, INT ySrc, DWORD rop );
extern BOOL X11DRV_EnumDeviceFonts( struct tagDC *dc, LPLOGFONT16 plf,
				      DEVICEFONTENUMPROC dfeproc, LPARAM lp );
extern BOOL X11DRV_GetCharWidth( struct tagDC *dc, UINT firstChar,
                                   UINT lastChar, LPINT buffer );
extern BOOL X11DRV_GetTextExtentPoint( struct tagDC *dc, LPCSTR str,
                                         INT count, LPSIZE size );
extern BOOL X11DRV_GetTextMetrics(struct tagDC *dc, TEXTMETRICA *metrics);
extern BOOL X11DRV_PatBlt( struct tagDC *dc, INT left, INT top,
                             INT width, INT height, DWORD rop );
extern VOID   X11DRV_SetDeviceClipping(struct tagDC *dc);
extern BOOL X11DRV_StretchBlt( struct tagDC *dcDst, INT xDst, INT yDst,
                                 INT widthDst, INT heightDst,
                                 struct tagDC *dcSrc, INT xSrc, INT ySrc,
                                 INT widthSrc, INT heightSrc, DWORD rop );
extern BOOL X11DRV_MoveToEx( struct tagDC *dc, INT x, INT y,LPPOINT pt);
extern BOOL X11DRV_LineTo( struct tagDC *dc, INT x, INT y);
extern BOOL X11DRV_Arc( struct tagDC *dc, INT left, INT top, INT right,
			  INT bottom, INT xstart, INT ystart, INT xend,
			  INT yend );
extern BOOL X11DRV_Pie( struct tagDC *dc, INT left, INT top, INT right,
			  INT bottom, INT xstart, INT ystart, INT xend,
			  INT yend );
extern BOOL X11DRV_Chord( struct tagDC *dc, INT left, INT top,
			    INT right, INT bottom, INT xstart,
			    INT ystart, INT xend, INT yend );
extern BOOL X11DRV_Ellipse( struct tagDC *dc, INT left, INT top,
			      INT right, INT bottom );
extern BOOL X11DRV_Rectangle(struct tagDC *dc, INT left, INT top,
			      INT right, INT bottom);
extern BOOL X11DRV_RoundRect( struct tagDC *dc, INT left, INT top,
				INT right, INT bottom, INT ell_width,
				INT ell_height );
extern COLORREF X11DRV_SetPixel( struct tagDC *dc, INT x, INT y,
				 COLORREF color );
extern COLORREF X11DRV_GetPixel( struct tagDC *dc, INT x, INT y);
extern BOOL X11DRV_PaintRgn( struct tagDC *dc, HRGN hrgn );
extern BOOL X11DRV_Polyline( struct tagDC *dc,const POINT* pt,INT count);
extern BOOL X11DRV_PolyBezier( struct tagDC *dc, const POINT start, const POINT* lppt, DWORD cPoints);
extern BOOL X11DRV_Polygon( struct tagDC *dc, const POINT* pt, INT count );
extern BOOL X11DRV_PolyPolygon( struct tagDC *dc, const POINT* pt, 
				  const INT* counts, UINT polygons);
extern BOOL X11DRV_PolyPolyline( struct tagDC *dc, const POINT* pt, 
				  const DWORD* counts, DWORD polylines);

extern HGDIOBJ X11DRV_SelectObject( struct tagDC *dc, HGDIOBJ handle );

extern COLORREF X11DRV_SetBkColor( struct tagDC *dc, COLORREF color );
extern COLORREF X11DRV_SetTextColor( struct tagDC *dc, COLORREF color );
extern BOOL X11DRV_ExtFloodFill( struct tagDC *dc, INT x, INT y,
				   COLORREF color, UINT fillType );
extern BOOL X11DRV_ExtTextOut( struct tagDC *dc, INT x, INT y,
				 UINT flags, const RECT *lprect,
				 LPCSTR str, UINT count, const INT *lpDx );
extern BOOL X11DRV_CreateBitmap( HBITMAP hbitmap );
extern BOOL X11DRV_DeleteObject( HGDIOBJ handle );
extern LONG X11DRV_BitmapBits( HBITMAP hbitmap, void *bits, LONG count,
			       WORD flags );
extern INT X11DRV_SetDIBitsToDevice( struct tagDC *dc, INT xDest,
				       INT yDest, DWORD cx, DWORD cy,
				       INT xSrc, INT ySrc,
				       UINT startscan, UINT lines,
				       LPCVOID bits, const BITMAPINFO *info,
				       UINT coloruse );
extern INT X11DRV_DeviceBitmapBits( struct tagDC *dc, HBITMAP hbitmap,
				      WORD fGet, UINT startscan,
				      UINT lines, LPSTR bits,
				      LPBITMAPINFO info, UINT coloruse );
extern HANDLE X11DRV_LoadOEMResource( WORD id, WORD type );

/* X11 driver internal functions */

extern BOOL X11DRV_BITMAP_Init(void);
extern BOOL X11DRV_BRUSH_Init(void);
extern BOOL X11DRV_DIB_Init(void);
extern BOOL X11DRV_FONT_Init( struct tagDeviceCaps* );
extern BOOL X11DRV_OBM_Init(void);

struct tagBITMAPOBJ;
extern XImage *X11DRV_BITMAP_GetXImage( const struct tagBITMAPOBJ *bmp );
extern int X11DRV_DIB_GetXImageWidthBytes( int width, int depth );
extern BOOL X11DRV_DIB_Init(void);
extern X11DRV_PHYSBITMAP *X11DRV_AllocBitmap( struct tagBITMAPOBJ *bmp );

extern BOOL X11DRV_SetupGCForPatBlt( struct tagDC *dc, GC gc,
				       BOOL fMapColors );
extern BOOL X11DRV_SetupGCForBrush( struct tagDC *dc );
extern BOOL X11DRV_SetupGCForPen( struct tagDC *dc );
extern BOOL X11DRV_SetupGCForText( struct tagDC *dc );

extern const int X11DRV_XROPfunction[];

/* Xlib critical section */

extern CRITICAL_SECTION X11DRV_CritSection;

extern void _XInitImageFuncPtrs(XImage *);

#define XCREATEIMAGE(image,width,height,bpp) \
{ \
    int width_bytes = X11DRV_DIB_GetXImageWidthBytes( (width), (bpp) ); \
    (image) = TSXCreateImage(display, DefaultVisualOfScreen(X11DRV_GetXScreen()), \
                           (bpp), ZPixmap, 0, xcalloc( (height)*width_bytes ),\
                           (width), (height), 32, width_bytes ); \
}

/* exported dib functions for now */

/* This structure holds the arguments for DIB_SetImageBits() */
typedef struct
{
    struct tagDC     *dc;
    LPCVOID           bits;
    XImage           *image;
    int               lines;
    DWORD             infoWidth;
    WORD              depth;
    WORD              infoBpp;
    WORD              compression;
    int              *colorMap;
    int               nColorMap;
    Drawable          drawable;
    GC                gc;
    int               xSrc;
    int               ySrc;
    int               xDest;
    int               yDest;
    int               width;
    int               height;
} DIB_SETIMAGEBITS_DESCR;

extern int X11DRV_DIB_GetImageBits( const DIB_SETIMAGEBITS_DESCR *descr );
extern int X11DRV_DIB_SetImageBits( const DIB_SETIMAGEBITS_DESCR *descr );
extern int *X11DRV_DIB_BuildColorMap( struct tagDC *dc, WORD coloruse,
				      WORD depth, const BITMAPINFO *info,
				      int *nColors );

/* X11 clipboard driver */

extern struct _CLIPBOARD_DRIVER X11DRV_CLIPBOARD_Driver;

extern void X11DRV_CLIPBOARD_EmptyClipboard(void);
extern void X11DRV_CLIPBOARD_SetClipboardData(UINT wFormat);
extern BOOL X11DRV_CLIPBOARD_RequestSelection(void);
extern void X11DRV_CLIPBOARD_ResetOwner(struct tagWND *pWnd, BOOL bFooBar);

void X11DRV_CLIPBOARD_ReadSelection(Window w, Atom prop);
void X11DRV_CLIPBOARD_ReleaseSelection(Window w, HWND hwnd);

/* X11 color driver */

extern Colormap	X11DRV_COLOR_GetColormap(void);

/* X11 desktop driver */

extern struct _DESKTOP_DRIVER X11DRV_DESKTOP_Driver;

typedef struct _X11DRV_DESKTOP_DATA {
} X11DRV_DESKTOP_DATA;

struct tagDESKTOP;

extern Screen *X11DRV_DESKTOP_GetXScreen(struct tagDESKTOP *pDesktop);
extern Window X11DRV_DESKTOP_GetXRootWindow(struct tagDESKTOP *pDesktop);

extern void X11DRV_DESKTOP_Initialize(struct tagDESKTOP *pDesktop);
extern void X11DRV_DESKTOP_Finalize(struct tagDESKTOP *pDesktop);
extern int X11DRV_DESKTOP_GetScreenWidth(struct tagDESKTOP *pDesktop);
extern int X11DRV_DESKTOP_GetScreenHeight(struct tagDESKTOP *pDesktop);
extern int X11DRV_DESKTOP_GetScreenDepth(struct tagDESKTOP *pDesktop);

/* X11 event driver */

extern struct _EVENT_DRIVER X11DRV_EVENT_Driver;

extern BOOL X11DRV_EVENT_Init(void);
extern void X11DRV_EVENT_AddIO(int fd, unsigned flag);
extern void X11DRV_EVENT_DeleteIO(int fd, unsigned flag);
extern BOOL X11DRV_EVENT_WaitNetEvent(BOOL sleep, BOOL peek);
extern void X11DRV_EVENT_Synchronize(void);
extern BOOL X11DRV_EVENT_CheckFocus( void );
extern BOOL X11DRV_EVENT_QueryPointer(DWORD *posX, DWORD *posY, DWORD *state);
extern void X11DRV_EVENT_DummyMotionNotify(void);
extern BOOL X11DRV_EVENT_Pending(void);
extern BOOL16 X11DRV_EVENT_IsUserIdle(void);
extern void X11DRV_EVENT_WakeUp(void);

/* X11 keyboard driver */

extern struct _KEYBOARD_DRIVER X11DRV_KEYBOARD_Driver;

extern void X11DRV_KEYBOARD_Init(void);
extern WORD X11DRV_KEYBOARD_VkKeyScan(CHAR cChar);
extern UINT16 X11DRV_KEYBOARD_MapVirtualKey(UINT16 wCode, UINT16 wMapType);
extern INT16 X11DRV_KEYBOARD_GetKeyNameText(LONG lParam, LPSTR lpBuffer, INT16 nSize);
extern INT16 X11DRV_KEYBOARD_ToAscii(UINT16 virtKey, UINT16 scanCode, LPBYTE lpKeyState, LPVOID lpChar, UINT16 flags);
extern void KEYBOARD_HandleEvent( struct tagWND *pWnd, XKeyEvent *event );
extern void KEYBOARD_UpdateState ( void );

/* X11 main driver */

extern Display *display;
extern Screen *X11DRV_GetXScreen(void);
extern Window X11DRV_GetXRootWindow(void);

extern void X11DRV_MAIN_Finalize(void);
extern void X11DRV_MAIN_Initialize(void);
extern void X11DRV_MAIN_ParseOptions(int *argc, char *argv[]);
extern void X11DRV_MAIN_Create(void);
extern void X11DRV_MAIN_SaveSetup(void);
extern void X11DRV_MAIN_RestoreSetup(void);

/* X11 monitor driver */

extern struct tagMONITOR_DRIVER X11DRV_MONITOR_Driver;

typedef struct _X11DRV_MONITOR_DATA {
  Screen  *screen;
  Window   rootWindow;
  int      width;
  int      height;
  int      depth;
} X11DRV_MONITOR_DATA;

struct tagMONITOR;

extern Screen *X11DRV_MONITOR_GetXScreen(struct tagMONITOR *pMonitor);
extern Window X11DRV_MONITOR_GetXRootWindow(struct tagMONITOR *pMonitor);

extern void X11DRV_MONITOR_Initialize(struct tagMONITOR *pMonitor);
extern void X11DRV_MONITOR_Finalize(struct tagMONITOR *pMonitor);
extern int X11DRV_MONITOR_GetWidth(struct tagMONITOR *pMonitor);
extern int X11DRV_MONITOR_GetHeight(struct tagMONITOR *pMonitor);
extern int X11DRV_MONITOR_GetDepth(struct tagMONITOR *pMonitor);

/* X11 mouse driver */

extern struct _MOUSE_DRIVER X11DRV_MOUSE_Driver;

extern void X11DRV_MOUSE_SetCursor(struct tagCURSORICONINFO *lpCursor);
extern void X11DRV_MOUSE_MoveCursor(WORD wAbsX, WORD wAbsY);

/* X11 windows driver */

extern struct _WND_DRIVER X11DRV_WND_Driver;

typedef struct _X11DRV_WND_DATA {
  Window window;
} X11DRV_WND_DATA;

extern Window X11DRV_WND_GetXWindow(struct tagWND *wndPtr);
extern Window X11DRV_WND_FindXWindow(struct tagWND *wndPtr);
extern Screen *X11DRV_WND_GetXScreen(struct tagWND *wndPtr);
extern Window X11DRV_WND_GetXRootWindow(struct tagWND *wndPtr);

extern void X11DRV_WND_Initialize(struct tagWND *wndPtr);
extern void X11DRV_WND_Finalize(struct tagWND *wndPtr);
extern BOOL X11DRV_WND_CreateDesktopWindow(struct tagWND *wndPtr, struct tagCLASS *classPtr, BOOL bUnicode);
extern BOOL X11DRV_WND_CreateWindow(struct tagWND *wndPtr, struct tagCLASS *classPtr, struct tagCREATESTRUCTA *cs, BOOL bUnicode);
extern BOOL X11DRV_WND_DestroyWindow(struct tagWND *pWnd);
extern struct tagWND *X11DRV_WND_SetParent(struct tagWND *wndPtr, struct tagWND *pWndParent);
extern void X11DRV_WND_ForceWindowRaise(struct tagWND *pWnd);
extern void X11DRV_WND_SetWindowPos(struct tagWND *wndPtr, const struct tagWINDOWPOS *winpos, BOOL bSMC_SETXPOS);
extern void X11DRV_WND_SetText(struct tagWND *wndPtr, LPCSTR text);
extern void X11DRV_WND_SetFocus(struct tagWND *wndPtr);
extern void X11DRV_WND_PreSizeMove(struct tagWND *wndPtr);
extern void X11DRV_WND_PostSizeMove(struct tagWND *wndPtr);
extern void X11DRV_WND_ScrollWindow(struct tagWND *wndPtr, struct tagDC *dcPtr, INT dx, INT dy, const RECT *clipRect, BOOL bUpdate);
extern void X11DRV_WND_SetDrawable(struct tagWND *wndPtr, struct tagDC *dc, WORD flags, BOOL bSetClipOrigin);
extern BOOL X11DRV_WND_IsSelfClipping(struct tagWND *wndPtr);

#endif  /* __WINE_X11DRV_H */
