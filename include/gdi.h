/*
 * GDI definitions
 *
 * Copyright 1993 Alexandre Julliard
 */

#ifndef __WINE_GDI_H
#define __WINE_GDI_H

#include "windows.h"
#include "ldt.h"
#include "local.h"
#include "x11drv.h"
#include "path.h"
#include <math.h>

  /* GDI objects magic numbers */
#define PEN_MAGIC             0x4f47
#define BRUSH_MAGIC           0x4f48
#define FONT_MAGIC            0x4f49
#define PALETTE_MAGIC         0x4f4a
#define BITMAP_MAGIC          0x4f4b
#define REGION_MAGIC          0x4f4c
#define DC_MAGIC              0x4f4d
#define DISABLED_DC_MAGIC     0x4f4e
#define META_DC_MAGIC         0x4f4f
#define METAFILE_MAGIC        0x4f50
#define METAFILE_DC_MAGIC     0x4f51
#define MAGIC_DONTCARE	      0xffff

typedef struct tagGDIOBJHDR
{
    HANDLE16    hNext;
    WORD        wMagic;
    DWORD       dwCount;
} GDIOBJHDR;


#define OBJ_PEN             1 
#define OBJ_BRUSH           2 
#define OBJ_DC              3 
#define OBJ_METADC          4 
#define OBJ_PAL             5 
#define OBJ_FONT            6 
#define OBJ_BITMAP          7 
#define OBJ_REGION          8 
#define OBJ_METAFILE        9 
#define OBJ_MEMDC           10 
#define OBJ_EXTPEN          11 
#define OBJ_ENHMETADC       12 
#define OBJ_ENHMETAFILE     13 


typedef struct tagDeviceCaps
{
    WORD   version;       /*   0: driver version */
    WORD   technology;    /*   2: device technology */
    WORD   horzSize;      /*   4: width of display in mm */
    WORD   vertSize;      /*   6: height of display in mm */
    WORD   horzRes;       /*   8: width of display in pixels */
    WORD   vertRes;       /*  10: width of display in pixels */
    WORD   bitsPixel;     /*  12: bits per pixel */
    WORD   planes;        /*  14: color planes */
    WORD   numBrushes;    /*  16: device-specific brushes */
    WORD   numPens;       /*  18: device-specific pens */
    WORD   numMarkers;    /*  20: device-specific markers */
    WORD   numFonts;      /*  22: device-specific fonts */
    WORD   numColors;     /*  24: size of color table */
    WORD   pdeviceSize;   /*  26: size of PDEVICE structure */
    WORD   curveCaps;     /*  28: curve capabilities */
    WORD   lineCaps;      /*  30: line capabilities */
    WORD   polygonalCaps; /*  32: polygon capabilities */
    WORD   textCaps;      /*  34: text capabilities */
    WORD   clipCaps;      /*  36: clipping capabilities */
    WORD   rasterCaps;    /*  38: raster capabilities */
    WORD   aspectX;       /*  40: relative width of device pixel */
    WORD   aspectY;       /*  42: relative height of device pixel */
    WORD   aspectXY;      /*  44: relative diagonal width of device pixel */
    WORD   pad1[21];      /*  46-86: reserved */
    WORD   logPixelsX;    /*  88: pixels / logical X inch */
    WORD   logPixelsY;    /*  90: pixels / logical Y inch */
    WORD   pad2[6];       /*  92-102: reserved */
    WORD   sizePalette;   /* 104: entries in system palette */
    WORD   numReserved;   /* 106: reserved entries */
    WORD   colorRes;      /* 108: color resolution */    
} DeviceCaps;


  /* Device independent DC information */
typedef struct
{
    int           flags;
    const DeviceCaps *devCaps;

    HRGN16        hClipRgn;     /* Clip region (may be 0) */
    HRGN16        hVisRgn;      /* Visible region (must never be 0) */
    HRGN16        hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
    HPEN16        hPen;
    HBRUSH16      hBrush;
    HFONT16       hFont;
    HBITMAP16     hBitmap;
    HBITMAP16     hFirstBitmap; /* Bitmap selected at creation of the DC */
    HANDLE16      hDevice;
    HPALETTE16    hPalette;
    
    GdiPath       path;

    WORD          ROPmode;
    WORD          polyFillMode;
    WORD          stretchBltMode;
    WORD          relAbsMode;
    WORD          backgroundMode;
    COLORREF      backgroundColor;
    COLORREF      textColor;
    int           backgroundPixel;
    int           textPixel;
    short         brushOrgX;
    short         brushOrgY;

    WORD          textAlign;         /* Text alignment from SetTextAlign() */
    short         charExtra;         /* Spacing from SetTextCharacterExtra() */
    short         breakTotalExtra;   /* Total extra space for justification */
    short         breakCount;        /* Break char. count */
    short         breakExtra;        /* breakTotalExtra / breakCount */
    short         breakRem;          /* breakTotalExtra % breakCount */

    BYTE          bitsPerPixel;

    INT32         MapMode;
    INT32         GraphicsMode;      /* Graphics mode */
    INT32         DCOrgX;            /* DC origin */
    INT32         DCOrgY;
    FARPROC16     lpfnPrint;         /* AbortProc for Printing */
    INT32         CursPosX;          /* Current position */
    INT32         CursPosY;
    INT32         ArcDirection;
    XFORM         xformWorld2Wnd;    /* World-to-window transformation */
    XFORM         xformWorld2Vport;  /* World-to-viewport transformation */
    XFORM         xformVport2World;  /* Inverse of the above transformation */
    BOOL32        vport2WorldValid;  /* Is xformVport2World valid? */
} WIN_DC_INFO;

typedef X11DRV_PDEVICE X_DC_INFO;  /* Temporary */

typedef struct tagDC
{
    GDIOBJHDR      header;
    HDC32          hSelf;            /* Handle to this DC */
    const struct tagDC_FUNCS *funcs; /* DC function table */
    void          *physDev;          /* Physical device (driver-specific) */
    INT32          saveLevel;
    DWORD          dwHookData;
    FARPROC16      hookProc;

    INT32          wndOrgX;          /* Window origin */
    INT32          wndOrgY;
    INT32          wndExtX;          /* Window extent */
    INT32          wndExtY;
    INT32          vportOrgX;        /* Viewport origin */
    INT32          vportOrgY;
    INT32          vportExtX;        /* Viewport extent */
    INT32          vportExtY;

    WIN_DC_INFO w;
    union
    {
	X_DC_INFO x;
	/* other devices (e.g. printer) */
    } u;
} DC;

/* Device functions for the Wine driver interface */
typedef struct tagDC_FUNCS
{
    BOOL32     (*pArc)(DC*,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
    BOOL32     (*pBitBlt)(DC*,INT32,INT32,INT32,INT32,DC*,INT32,INT32,DWORD);
    BOOL32     (*pChord)(DC*,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
    BOOL32     (*pCreateDC)(DC*,LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
    BOOL32     (*pDeleteDC)(DC*);
    BOOL32     (*pDeleteObject)(HGDIOBJ16);
    BOOL32     (*pEllipse)(DC*,INT32,INT32,INT32,INT32);
    BOOL32     (*pEnumDeviceFonts)(DC*,LPLOGFONT16,DEVICEFONTENUMPROC,LPARAM);
    INT32      (*pEscape)(DC*,INT32,INT32,SEGPTR,SEGPTR);
    INT32      (*pExcludeClipRect)(DC*,INT32,INT32,INT32,INT32);
    INT32      (*pExcludeVisRect)(DC*,INT32,INT32,INT32,INT32);
    BOOL32     (*pExtFloodFill)(DC*,INT32,INT32,COLORREF,UINT32);
    BOOL32     (*pExtTextOut)(DC*,INT32,INT32,UINT32,const RECT32*,LPCSTR,UINT32,const INT32*);
    BOOL32     (*pGetCharWidth)(DC*,UINT32,UINT32,LPINT32);
    COLORREF   (*pGetPixel)(DC*,INT32,INT32);
    BOOL32     (*pGetTextExtentPoint)(DC*,LPCSTR,INT32,LPSIZE32);
    BOOL32     (*pGetTextMetrics)(DC*,TEXTMETRIC32A*);
    INT32      (*pIntersectClipRect)(DC*,INT32,INT32,INT32,INT32);
    INT32      (*pIntersectVisRect)(DC*,INT32,INT32,INT32,INT32);
    BOOL32     (*pLineTo)(DC*,INT32,INT32);
    BOOL32     (*pMoveToEx)(DC*,INT32,INT32,LPPOINT32);
    INT32      (*pOffsetClipRgn)(DC*,INT32,INT32);
    BOOL32     (*pOffsetViewportOrg)(DC*,INT32,INT32);
    BOOL32     (*pOffsetWindowOrg)(DC*,INT32,INT32);
    BOOL32     (*pPaintRgn)(DC*,HRGN32);
    BOOL32     (*pPatBlt)(DC*,INT32,INT32,INT32,INT32,DWORD);
    BOOL32     (*pPie)(DC*,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
    BOOL32     (*pPolyPolygon)(DC*,LPPOINT32,LPINT32,UINT32);
    BOOL32     (*pPolyPolyline)(DC*,LPPOINT32,LPINT32,UINT32);
    BOOL32     (*pPolygon)(DC*,LPPOINT32,INT32);
    BOOL32     (*pPolyline)(DC*,LPPOINT32,INT32);
    BOOL32     (*pPolyBezier)(DC*,POINT32, LPPOINT32,DWORD);
    UINT32     (*pRealizePalette)(DC*);
    BOOL32     (*pRectangle)(DC*,INT32,INT32,INT32,INT32);
    BOOL32     (*pRestoreDC)(DC*,INT32);
    BOOL32     (*pRoundRect)(DC*,INT32,INT32,INT32,INT32,INT32,INT32);
    INT32      (*pSaveDC)(DC*);
    BOOL32     (*pScaleViewportExt)(DC*,INT32,INT32,INT32,INT32);
    BOOL32     (*pScaleWindowExt)(DC*,INT32,INT32,INT32,INT32);
    INT32      (*pSelectClipRgn)(DC*,HRGN32);
    HANDLE32   (*pSelectObject)(DC*,HANDLE32);
    HPALETTE32 (*pSelectPalette)(DC*,HPALETTE32,BOOL32);
    COLORREF   (*pSetBkColor)(DC*,COLORREF);
    WORD       (*pSetBkMode)(DC*,WORD);
    VOID       (*pSetDeviceClipping)(DC*);
    INT32      (*pSetDIBitsToDevice)(DC*,INT32,INT32,DWORD,DWORD,INT32,INT32,UINT32,UINT32,LPCVOID,const BITMAPINFO*,UINT32);
    INT32      (*pSetMapMode)(DC*,INT32);
    DWORD      (*pSetMapperFlags)(DC*,DWORD);
    COLORREF   (*pSetPixel)(DC*,INT32,INT32,COLORREF);
    WORD       (*pSetPolyFillMode)(DC*,WORD);
    WORD       (*pSetROP2)(DC*,WORD);
    WORD       (*pSetRelAbs)(DC*,WORD);
    WORD       (*pSetStretchBltMode)(DC*,WORD);
    WORD       (*pSetTextAlign)(DC*,WORD);
    INT32      (*pSetTextCharacterExtra)(DC*,INT32);
    DWORD      (*pSetTextColor)(DC*,DWORD);
    INT32      (*pSetTextJustification)(DC*,INT32,INT32);
    BOOL32     (*pSetViewportExt)(DC*,INT32,INT32);
    BOOL32     (*pSetViewportOrg)(DC*,INT32,INT32);
    BOOL32     (*pSetWindowExt)(DC*,INT32,INT32);
    BOOL32     (*pSetWindowOrg)(DC*,INT32,INT32);
    BOOL32     (*pStretchBlt)(DC*,INT32,INT32,INT32,INT32,DC*,INT32,INT32,INT32,INT32,DWORD);
    INT32      (*pStretchDIBits)(DC*,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,LPSTR,LPBITMAPINFO,WORD,DWORD);
} DC_FUNCTIONS;

  /* DC hook codes */
#define DCHC_INVALIDVISRGN      0x0001
#define DCHC_DELETEDC           0x0002

#define DCHF_INVALIDATEVISRGN   0x0001
#define DCHF_VALIDATEVISRGN     0x0002

  /* DC flags */
#define DC_MEMORY     0x0001   /* It is a memory DC */
#define DC_SAVED      0x0002   /* It is a saved DC */
#define DC_DIRTY      0x0004   /* hVisRgn has to be updated */
#define DC_THUNKHOOK  0x0008   /* DC hook is in the 16-bit code */ 

  /* Last 32 bytes are reserved for stock object handles */
#define GDI_HEAP_SIZE               0xffe0

  /* First handle possible for stock objects (must be >= GDI_HEAP_SIZE) */
#define FIRST_STOCK_HANDLE          GDI_HEAP_SIZE

  /* Stock objects handles */

#define NB_STOCK_OBJECTS          (DEFAULT_GUI_FONT + 1)

#define STOCK_WHITE_BRUSH       ((HBRUSH16)(FIRST_STOCK_HANDLE+WHITE_BRUSH))
#define STOCK_LTGRAY_BRUSH      ((HBRUSH16)(FIRST_STOCK_HANDLE+LTGRAY_BRUSH))
#define STOCK_GRAY_BRUSH        ((HBRUSH16)(FIRST_STOCK_HANDLE+GRAY_BRUSH))
#define STOCK_DKGRAY_BRUSH      ((HBRUSH16)(FIRST_STOCK_HANDLE+DKGRAY_BRUSH))
#define STOCK_BLACK_BRUSH       ((HBRUSH16)(FIRST_STOCK_HANDLE+BLACK_BRUSH))
#define STOCK_NULL_BRUSH        ((HBRUSH16)(FIRST_STOCK_HANDLE+NULL_BRUSH))
#define STOCK_HOLLOW_BRUSH      ((HBRUSH16)(FIRST_STOCK_HANDLE+HOLLOW_BRUSH))
#define STOCK_WHITE_PEN	        ((HPEN16)(FIRST_STOCK_HANDLE+WHITE_PEN))
#define STOCK_BLACK_PEN	        ((HPEN16)(FIRST_STOCK_HANDLE+BLACK_PEN))
#define STOCK_NULL_PEN	        ((HPEN16)(FIRST_STOCK_HANDLE+NULL_PEN))
#define STOCK_OEM_FIXED_FONT    ((HFONT16)(FIRST_STOCK_HANDLE+OEM_FIXED_FONT))
#define STOCK_ANSI_FIXED_FONT   ((HFONT16)(FIRST_STOCK_HANDLE+ANSI_FIXED_FONT))
#define STOCK_ANSI_VAR_FONT     ((HFONT16)(FIRST_STOCK_HANDLE+ANSI_VAR_FONT))
#define STOCK_SYSTEM_FONT       ((HFONT16)(FIRST_STOCK_HANDLE+SYSTEM_FONT))
#define STOCK_DEVICE_DEFAULT_FONT ((HFONT16)(FIRST_STOCK_HANDLE+DEVICE_DEFAULT_FONT))
#define STOCK_DEFAULT_PALETTE   ((HPALETTE16)(FIRST_STOCK_HANDLE+DEFAULT_PALETTE))
#define STOCK_SYSTEM_FIXED_FONT ((HFONT16)(FIRST_STOCK_HANDLE+SYSTEM_FIXED_FONT))
#define STOCK_DEFAULT_GUI_FONT  ((HFONT16)(FIRST_STOCK_HANDLE+DEFAULT_GUI_FONT))

#define FIRST_STOCK_FONT        STOCK_OEM_FIXED_FONT
#define LAST_STOCK_FONT         STOCK_DEFAULT_GUI_FONT

#define LAST_STOCK_HANDLE       ((DWORD)STOCK_DEFAULT_GUI_FONT)

  /* Device <-> logical coords conversion */

/* A floating point version of the POINT structure */
typedef struct tagFLOAT_POINT
{
   FLOAT x, y;
} FLOAT_POINT;

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static __inline__ INT32 GDI_ROUND(FLOAT val)
{
   return (int)floor(val + 0.5);
}

/* Performs a viewport-to-world transformation on the specified point (which
 * is in floating point format). Returns TRUE if successful, else FALSE.
 */
static __inline__ BOOL32 INTERNAL_DPTOLP_FLOAT(DC *dc, FLOAT_POINT *point)
{
    FLOAT x, y;
    
    /* Check that the viewport-to-world transformation is valid */
    if (!dc->w.vport2WorldValid)
        return FALSE;

    /* Perform the transformation */
    x = point->x;
    y = point->y;
    point->x = x * dc->w.xformVport2World.eM11 +
               y * dc->w.xformVport2World.eM21 +
	       dc->w.xformVport2World.eDx;
    point->y = x * dc->w.xformVport2World.eM12 +
               y * dc->w.xformVport2World.eM22 +
	       dc->w.xformVport2World.eDy;

    return TRUE;
}

/* Performs a viewport-to-world transformation on the specified point (which
 * is in integer format). Returns TRUE if successful, else FALSE.
 */
static __inline__ BOOL32 INTERNAL_DPTOLP(DC *dc, LPPOINT32 point)
{
    FLOAT_POINT floatPoint;
    
    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)point->x;
    floatPoint.y=(FLOAT)point->y;
    if (!INTERNAL_DPTOLP_FLOAT(dc, &floatPoint))
        return FALSE;
    
    /* Round to integers */
    point->x = GDI_ROUND(floatPoint.x);
    point->y = GDI_ROUND(floatPoint.y);

    return TRUE;
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in floating point format).
 */
static __inline__ void INTERNAL_LPTODP_FLOAT(DC *dc, FLOAT_POINT *point)
{
    FLOAT x, y;
    
    /* Perform the transformation */
    x = point->x;
    y = point->y;
    point->x = x * dc->w.xformWorld2Vport.eM11 +
               y * dc->w.xformWorld2Vport.eM21 +
	       dc->w.xformWorld2Vport.eDx;
    point->y = x * dc->w.xformWorld2Vport.eM12 +
               y * dc->w.xformWorld2Vport.eM22 +
	       dc->w.xformWorld2Vport.eDy;
}

/* Performs a world-to-viewport transformation on the specified point (which
 * is in integer format).
 */
static __inline__ void INTERNAL_LPTODP(DC *dc, LPPOINT32 point)
{
    FLOAT_POINT floatPoint;
    
    /* Perform operation with floating point */
    floatPoint.x=(FLOAT)point->x;
    floatPoint.y=(FLOAT)point->y;
    INTERNAL_LPTODP_FLOAT(dc, &floatPoint);
    
    /* Round to integers */
    point->x = GDI_ROUND(floatPoint.x);
    point->y = GDI_ROUND(floatPoint.y);
}

#define XDPTOLP(dc,x) \
    (((x)-(dc)->vportOrgX) * (dc)->wndExtX / (dc)->vportExtX+(dc)->wndOrgX)
#define YDPTOLP(dc,y) \
    (((y)-(dc)->vportOrgY) * (dc)->wndExtY / (dc)->vportExtY+(dc)->wndOrgY)
#define XLPTODP(dc,x) \
    (((x)-(dc)->wndOrgX) * (dc)->vportExtX / (dc)->wndExtX+(dc)->vportOrgX)
#define YLPTODP(dc,y) \
    (((y)-(dc)->wndOrgY) * (dc)->vportExtY / (dc)->wndExtY+(dc)->vportOrgY)

  /* Device <-> logical size conversion */

#define XDSTOLS(dc,x) \
    ((x) * (dc)->wndExtX / (dc)->vportExtX)
#define YDSTOLS(dc,y) \
    ((y) * (dc)->wndExtY / (dc)->vportExtY)
#define XLSTODS(dc,x) \
    ((x) * (dc)->vportExtX / (dc)->wndExtX)
#define YLSTODS(dc,y) \
    ((y) * (dc)->vportExtY / (dc)->wndExtY)

  /* GDI local heap */

extern WORD GDI_HeapSel;

#define GDI_HEAP_ALLOC(size) \
            LOCAL_Alloc( GDI_HeapSel, LMEM_FIXED, (size) )
#define GDI_HEAP_ALLOC_MOVEABLE(size) \
            LOCAL_Alloc( GDI_HeapSel, LMEM_MOVEABLE, (size) )
#define GDI_HEAP_REALLOC(handle,size) \
            LOCAL_ReAlloc( GDI_HeapSel, (handle), (size), LMEM_FIXED )
#define GDI_HEAP_FREE(handle) \
            LOCAL_Free( GDI_HeapSel, (handle) )

#define GDI_HEAP_LOCK(handle) \
            LOCAL_Lock( GDI_HeapSel, (handle) )
#define GDI_HEAP_LOCK_SEGPTR(handle) \
            LOCAL_LockSegptr( GDI_HeapSel, (handle) )
#define GDI_HEAP_UNLOCK(handle) \
    ((((HGDIOBJ16)(handle) >= FIRST_STOCK_HANDLE) && \
      ((HGDIOBJ16)(handle)<=LAST_STOCK_HANDLE)) ? \
      0 : LOCAL_Unlock( GDI_HeapSel, (handle) ))

extern BOOL32 GDI_Init(void);
extern HGDIOBJ16 GDI_AllocObject( WORD, WORD );
extern BOOL32 GDI_FreeObject( HGDIOBJ16 );
extern GDIOBJHDR * GDI_GetObjPtr( HGDIOBJ16, WORD );

extern BOOL32 DRIVER_RegisterDriver( LPCSTR name, const DC_FUNCTIONS *funcs );
extern const DC_FUNCTIONS *DRIVER_FindDriver( LPCSTR name );
extern BOOL32 DRIVER_UnregisterDriver( LPCSTR name );

extern BOOL32 DIB_Init(void);

extern Display * display;
extern Screen * screen;
extern Window rootWindow;
extern int screenWidth, screenHeight, screenDepth;

#endif  /* __WINE_GDI_H */
