/*
 * X11 driver definitions
 */

#ifndef __WINE_X11DRV_H
#define __WINE_X11DRV_H

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#ifdef HAVE_LIBXXSHM
# include <X11/extensions/XShm.h>
#endif /* defined(HAVE_LIBXXSHM) */

#include "windef.h"
#include "winbase.h"
#include "gdi.h"
#include "user.h"
#include "win.h"
#include "thread.h"

#define MAX_PIXELFORMATS 8

struct tagBITMAPOBJ;
struct tagCURSORICONINFO;
struct tagDC;
struct tagDeviceCaps;
struct tagPALETTEOBJ;
struct tagWINDOWPOS;
struct DIDEVICEOBJECTDATA;

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
    int           exposures;   /* count of graphics exposures operations */
    XVisualInfo  *visuals[MAX_PIXELFORMATS];
    int           used_visuals;
    int           current_pf;
} X11DRV_PDEVICE;


  /* GCs used for B&W and color bitmap operations */
extern GC BITMAP_monoGC, BITMAP_colorGC;

#define BITMAP_GC(bmp) \
  (((bmp)->bitmap.bmBitsPixel == 1) ? BITMAP_monoGC : BITMAP_colorGC)

extern unsigned int X11DRV_server_startticks;

/* Wine driver X11 functions */

extern BOOL X11DRV_BitBlt( struct tagDC *dcDst, INT xDst, INT yDst,
                             INT width, INT height, struct tagDC *dcSrc,
                             INT xSrc, INT ySrc, DWORD rop );
extern BOOL X11DRV_EnumDeviceFonts( HDC hdc, LPLOGFONTW plf,
				    DEVICEFONTENUMPROC dfeproc, LPARAM lp );
extern BOOL X11DRV_GetCharWidth( struct tagDC *dc, UINT firstChar,
                                   UINT lastChar, LPINT buffer );
extern BOOL X11DRV_GetDCOrgEx( struct tagDC *dc, LPPOINT lpp );
extern BOOL X11DRV_GetTextExtentPoint( struct tagDC *dc, LPCWSTR str,
                                         INT count, LPSIZE size );
extern BOOL X11DRV_GetTextMetrics(struct tagDC *dc, TEXTMETRICW *metrics);
extern BOOL X11DRV_PatBlt( struct tagDC *dc, INT left, INT top,
                             INT width, INT height, DWORD rop );
extern VOID   X11DRV_SetDeviceClipping(struct tagDC *dc);
extern BOOL X11DRV_StretchBlt( struct tagDC *dcDst, INT xDst, INT yDst,
                                 INT widthDst, INT heightDst,
                                 struct tagDC *dcSrc, INT xSrc, INT ySrc,
                                 INT widthSrc, INT heightSrc, DWORD rop );
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
				 LPCWSTR str, UINT count, const INT *lpDx );
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
extern BOOL X11DRV_GetDeviceGammaRamp( struct tagDC *dc, LPVOID ramp );
extern BOOL X11DRV_SetDeviceGammaRamp( struct tagDC *dc, LPVOID ramp );

/* OpenGL / X11 driver functions */
extern int X11DRV_ChoosePixelFormat(DC *dc, const PIXELFORMATDESCRIPTOR *pppfd) ;
extern int X11DRV_DescribePixelFormat(DC *dc, int iPixelFormat, UINT nBytes, PIXELFORMATDESCRIPTOR *ppfd) ;
extern int X11DRV_GetPixelFormat(DC *dc) ;
extern BOOL X11DRV_SetPixelFormat(DC *dc, int iPixelFormat, const PIXELFORMATDESCRIPTOR *ppfd) ;
extern BOOL X11DRV_SwapBuffers(DC *dc) ;

/* X11 driver internal functions */

extern BOOL X11DRV_BITMAP_Init(void);
extern int X11DRV_FONT_Init( int *log_pixels_x, int *log_pixels_y );
extern BOOL X11DRV_OBM_Init(void);
extern HBRUSH X11DRV_BRUSH_SelectObject( DC * dc, HBRUSH hbrush );
extern HFONT X11DRV_FONT_SelectObject( DC * dc, HFONT hfont );
extern HPEN X11DRV_PEN_SelectObject( DC * dc, HPEN hpen );
extern HBITMAP X11DRV_BITMAP_SelectObject( DC * dc, HBITMAP hbitmap );
extern BOOL X11DRV_BITMAP_DeleteObject( HBITMAP hbitmap );

struct tagBITMAPOBJ;
extern XImage *X11DRV_BITMAP_GetXImage( const struct tagBITMAPOBJ *bmp );
extern XImage *X11DRV_DIB_CreateXImage( int width, int height, int depth );
extern HBITMAP X11DRV_BITMAP_CreateBitmapHeaderFromPixmap(Pixmap pixmap);
extern HGLOBAL X11DRV_DIB_CreateDIBFromPixmap(Pixmap pixmap, HDC hdc, BOOL bDeletePixmap);
extern HBITMAP X11DRV_BITMAP_CreateBitmapFromPixmap(Pixmap pixmap, BOOL bDeletePixmap);
extern Pixmap X11DRV_DIB_CreatePixmapFromDIB( HGLOBAL hPackedDIB, HDC hdc );
extern Pixmap X11DRV_BITMAP_CreatePixmapFromBitmap( HBITMAP hBmp, HDC hdc );

extern void X11DRV_SetDrawable( HDC hdc, Drawable drawable, int mode, int org_x, int org_y );
extern void X11DRV_StartGraphicsExposures( HDC hdc );
extern void X11DRV_EndGraphicsExposures( HDC hdc, HRGN hrgn );

extern BOOL X11DRV_SetupGCForPatBlt( struct tagDC *dc, GC gc, BOOL fMapColors );
extern BOOL X11DRV_SetupGCForBrush( struct tagDC *dc );
extern BOOL X11DRV_SetupGCForPen( struct tagDC *dc );
extern BOOL X11DRV_SetupGCForText( struct tagDC *dc );

extern const int X11DRV_XROPfunction[];

extern void _XInitImageFuncPtrs(XImage *);

/* exported dib functions for now */

/* Additional info for DIB section objects */
typedef struct
{
    /* Windows DIB section */
    DIBSECTION  dibSection;

    /* Mapping status */
    int         status, p_status;

    /* Color map info */
    int         nColorMap;
    int        *colorMap;

    /* Cached XImage */
    XImage     *image;

#ifdef HAVE_LIBXXSHM
    /* Shared memory segment info */
    XShmSegmentInfo shminfo;
#endif

    /* Aux buffer access function */
    void (*copy_aux)(void*ctx, int req);
    void *aux_ctx;

    /* GDI access lock */
    CRITICAL_SECTION lock;

} X11DRV_DIBSECTION;

extern int *X11DRV_DIB_BuildColorMap( struct tagDC *dc, WORD coloruse,
				      WORD depth, const BITMAPINFO *info,
				      int *nColors );
extern INT X11DRV_CoerceDIBSection(struct tagDC *dc,INT,BOOL);
extern INT X11DRV_LockDIBSection(struct tagDC *dc,INT,BOOL);
extern void X11DRV_UnlockDIBSection(struct tagDC *dc,BOOL);
extern INT X11DRV_CoerceDIBSection2(HBITMAP bmp,INT,BOOL);
extern INT X11DRV_LockDIBSection2(HBITMAP bmp,INT,BOOL);
extern void X11DRV_UnlockDIBSection2(HBITMAP bmp,BOOL);

extern HBITMAP X11DRV_DIB_CreateDIBSection(struct tagDC *dc, BITMAPINFO *bmi, UINT usage,
					   LPVOID *bits, HANDLE section, DWORD offset, DWORD ovr_pitch);

extern struct tagBITMAP_DRIVER X11DRV_BITMAP_Driver;

extern INT X11DRV_DIB_SetDIBits(struct tagBITMAPOBJ *bmp, struct tagDC *dc, UINT startscan,
				UINT lines, LPCVOID bits, const BITMAPINFO *info,
				UINT coloruse, HBITMAP hbitmap);
extern INT X11DRV_DIB_GetDIBits(struct tagBITMAPOBJ *bmp, struct tagDC *dc, UINT startscan,
				UINT lines, LPVOID bits, BITMAPINFO *info,
				UINT coloruse, HBITMAP hbitmap);
extern void X11DRV_DIB_DeleteDIBSection(struct tagBITMAPOBJ *bmp);
extern UINT X11DRV_DIB_SetDIBColorTable(struct tagBITMAPOBJ *,struct tagDC*,UINT,UINT,const RGBQUAD *);
extern UINT X11DRV_DIB_GetDIBColorTable(struct tagBITMAPOBJ *,struct tagDC*,UINT,UINT,RGBQUAD *);
extern INT X11DRV_DIB_Coerce(struct tagBITMAPOBJ *,INT,BOOL);
extern INT X11DRV_DIB_Lock(struct tagBITMAPOBJ *,INT,BOOL);
extern void X11DRV_DIB_Unlock(struct tagBITMAPOBJ *,BOOL);
void X11DRV_DIB_CopyDIBSection(DC *dcSrc, DC *dcDst,     
			       DWORD xSrc, DWORD ySrc,   
			       DWORD xDest, DWORD yDest, 
			       DWORD width, DWORD height);
struct _DCICMD;
extern INT X11DRV_DCICommand(INT cbInput, const struct _DCICMD *lpCmd, LPVOID lpOutData);

/**************************************************************************
 * X11 GDI driver
 */

BOOL X11DRV_GDI_Initialize( Display *display );
void X11DRV_GDI_Finalize(void);

extern Display *gdi_display;  /* display to use for all GDI functions */

/* X11 GDI palette driver */

#define X11DRV_PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define X11DRV_PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define X11DRV_PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */
#define X11DRV_PALETTE_WHITESET 0x2000

extern Colormap X11DRV_PALETTE_PaletteXColormap;
extern UINT16 X11DRV_PALETTE_PaletteFlags;

extern int *X11DRV_PALETTE_PaletteToXPixel;
extern int *X11DRV_PALETTE_XPixelToPalette;

extern int X11DRV_PALETTE_mapEGAPixel[16];

extern int X11DRV_PALETTE_Init(void);
extern void X11DRV_PALETTE_Cleanup(void);

extern COLORREF X11DRV_PALETTE_ToLogical(int pixel);
extern int X11DRV_PALETTE_ToPhysical(struct tagDC *dc, COLORREF color);

extern struct tagPALETTE_DRIVER X11DRV_PALETTE_Driver;

extern int X11DRV_PALETTE_SetMapping(struct tagPALETTEOBJ *palPtr, UINT uStart, UINT uNum, BOOL mapOnly);
extern int X11DRV_PALETTE_UpdateMapping(struct tagPALETTEOBJ *palPtr);
extern BOOL X11DRV_PALETTE_IsDark(int pixel);

/**************************************************************************
 * X11 USER driver
 */

struct x11drv_thread_data
{
    Display *display;
    HANDLE   display_fd;
    int      process_event_count;  /* recursion count for event processing */
};

extern struct x11drv_thread_data *x11drv_init_thread_data(void);

inline static struct x11drv_thread_data *x11drv_thread_data(void)
{
    struct x11drv_thread_data *data = NtCurrentTeb()->driver_data;
    if (!data) data = x11drv_init_thread_data();
    return data;
}

inline static Display *thread_display(void) { return x11drv_thread_data()->display; }

extern Visual *visual;
extern Window root_window;
extern unsigned int screen_width;
extern unsigned int screen_height;
extern unsigned int screen_depth;

extern Atom wmProtocols;
extern Atom wmDeleteWindow;
extern Atom wmTakeFocus;
extern Atom dndProtocol;
extern Atom dndSelection;
extern Atom wmChangeState;
extern Atom kwmDockWindow;
extern Atom _kde_net_wm_system_tray_window_for;

/* X11 clipboard driver */

extern void X11DRV_CLIPBOARD_FreeResources( Atom property );
extern BOOL X11DRV_CLIPBOARD_RegisterPixmapResource( Atom property, Pixmap pixmap );
extern BOOL X11DRV_CLIPBOARD_IsNativeProperty(Atom prop);
extern UINT X11DRV_CLIPBOARD_MapPropertyToFormat(char *itemFmtName);
extern Atom X11DRV_CLIPBOARD_MapFormatToProperty(UINT id);
extern void X11DRV_CLIPBOARD_ReleaseSelection(Atom selType, Window w, HWND hwnd);
extern BOOL X11DRV_IsSelectionOwner(void);
extern BOOL X11DRV_GetClipboardData(UINT wFormat);

/* X11 event driver */

extern WORD X11DRV_EVENT_XStateToKeyState( int state ) ;

typedef enum {
  X11DRV_INPUT_RELATIVE,
  X11DRV_INPUT_ABSOLUTE
} INPUT_TYPE;
extern INPUT_TYPE X11DRV_EVENT_SetInputMethod(INPUT_TYPE type);

#ifdef HAVE_LIBXXF86DGA2
void X11DRV_EVENT_SetDGAStatus(HWND hwnd, int event_base) ;
#endif

/* X11 mouse driver */

extern void X11DRV_InitMouse(LPMOUSE_EVENT_PROC);
extern void X11DRV_SetCursor(struct tagCURSORICONINFO *lpCursor);
extern void X11DRV_MoveCursor(WORD wAbsX, WORD wAbsY);
extern void X11DRV_SendEvent( DWORD mouseStatus, DWORD posX, DWORD posY,
                              WORD keyState, DWORD data, DWORD time, HWND hWnd );

/* x11drv private window data */
struct x11drv_win_data
{
    Window  whole_window;   /* X window for the complete window */
    Window  client_window;  /* X window for the client area */
    Window  icon_window;    /* X window for the icon */
    RECT    whole_rect;     /* X window rectangle for the whole window relative to parent */
    RECT    client_rect;    /* client area relative to whole window */
    HBITMAP hWMIconBitmap;
    HBITMAP hWMIconMask;
};

typedef struct x11drv_win_data X11DRV_WND_DATA;

extern Window X11DRV_get_client_window( HWND hwnd );
extern Window X11DRV_get_whole_window( HWND hwnd );

inline static Window get_client_window( WND *wnd )
{
    struct x11drv_win_data *data = wnd->pDriverData;
    return data->client_window;
}

inline static Window get_whole_window( WND *wnd )
{
    struct x11drv_win_data *data = wnd->pDriverData;
    return data->whole_window;
}

extern void X11DRV_SetFocus( HWND hwnd );
extern Cursor X11DRV_GetCursor( Display *display, struct tagCURSORICONINFO *ptr );

extern void X11DRV_expect_error( unsigned char request, unsigned char error, XID id );
extern int X11DRV_check_error(void);
extern void X11DRV_register_window( Display *display, HWND hwnd, struct x11drv_win_data *data );
extern void X11DRV_set_iconic_state( WND *win );
extern void X11DRV_window_to_X_rect( WND *win, RECT *rect );
extern void X11DRV_X_to_window_rect( WND *win, RECT *rect );
extern void X11DRV_create_desktop_thread(void);
extern Window X11DRV_create_desktop( XVisualInfo *desktop_vi, const char *geometry );
extern int X11DRV_sync_whole_window_position( Display *display, WND *win, int zorder );
extern int X11DRV_sync_client_window_position( Display *display, WND *win );

#endif  /* __WINE_X11DRV_H */
