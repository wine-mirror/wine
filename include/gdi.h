/*
 * GDI definitions
 *
 * Copyright 1993 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_GDI_H
#define __WINE_GDI_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wine/wingdi16.h>
#include <math.h>

  /* GDI objects magic numbers */
#define FIRST_MAGIC           0x4f47
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
#define ENHMETAFILE_MAGIC     0x4f52
#define ENHMETAFILE_DC_MAGIC  0x4f53
#define MEMORY_DC_MAGIC       0x4f54
#define LAST_MAGIC            0x4f54

#define MAGIC_DONTCARE	      0xffff

/* GDI constants for making objects private/system (naming undoc. !) */
#define OBJECT_PRIVATE        0x2000
#define OBJECT_NOSYSTEM       0x8000

#define GDIMAGIC(magic) ((magic) & ~(OBJECT_PRIVATE|OBJECT_NOSYSTEM))

struct gdi_obj_funcs;
struct hdc_list;

typedef struct tagGDIOBJHDR
{
    HANDLE16    hNext;
    WORD        wMagic;
    DWORD       dwCount;
    const struct gdi_obj_funcs *funcs;
    struct hdc_list *hdcs;
} GDIOBJHDR;


/* It should not be necessary to access the contents of the GdiPath
 * structure directly; if you find that the exported functions don't
 * allow you to do what you want, then please place a new exported
 * function that does this job in path.c.
 */
typedef enum tagGdiPathState
{
   PATH_Null,
   PATH_Open,
   PATH_Closed
} GdiPathState;

typedef struct tagGdiPath
{
   GdiPathState state;
   POINT      *pPoints;
   BYTE         *pFlags;
   int          numEntriesUsed, numEntriesAllocated;
   BOOL       newStroke;
} GdiPath;

typedef struct tagGdiFont *GdiFont;

typedef struct { int opaque; } *PHYSDEV;  /* PHYSDEV is an opaque pointer */

typedef struct tagDC
{
    GDIOBJHDR    header;
    HDC          hSelf;            /* Handle to this DC */
    const struct tagDC_FUNCS *funcs; /* DC function table */
    PHYSDEV      physDev;         /* Physical device (driver-specific) */
    INT          saveLevel;
    DWORD        dwHookData;
    FARPROC16    hookProc;         /* the original SEGPTR ... */
    DCHOOKPROC   hookThunk;        /* ... and the thunk to call it */

    INT          wndOrgX;          /* Window origin */
    INT          wndOrgY;
    INT          wndExtX;          /* Window extent */
    INT          wndExtY;
    INT          vportOrgX;        /* Viewport origin */
    INT          vportOrgY;
    INT          vportExtX;        /* Viewport extent */
    INT          vportExtY;

    int           flags;
    HRGN          hClipRgn;     /* Clip region (may be 0) */
    HRGN          hVisRgn;      /* Visible region (must never be 0) */
    HRGN          hGCClipRgn;   /* GC clip region (ClipRgn AND VisRgn) */
    HPEN          hPen;
    HBRUSH        hBrush;
    HFONT         hFont;
    HBITMAP       hBitmap;
    HANDLE        hDevice;
    HPALETTE      hPalette;

    GdiFont       gdiFont;
    GdiPath       path;

    WORD          ROPmode;
    WORD          polyFillMode;
    WORD          stretchBltMode;
    WORD          relAbsMode;
    WORD          backgroundMode;
    COLORREF      backgroundColor;
    COLORREF      textColor;
    COLORREF      dcBrushColor;
    COLORREF      dcPenColor;
    short         brushOrgX;
    short         brushOrgY;

    WORD          textAlign;         /* Text alignment from SetTextAlign() */
    short         charExtra;         /* Spacing from SetTextCharacterExtra() */
    short         breakTotalExtra;   /* Total extra space for justification */
    short         breakCount;        /* Break char. count */
    short         breakExtra;        /* breakTotalExtra / breakCount */
    short         breakRem;          /* breakTotalExtra % breakCount */

    RECT          totalExtent;

    INT           MapMode;
    INT           GraphicsMode;      /* Graphics mode */
    ABORTPROC     pAbortProc;        /* AbortProc for Printing */
    ABORTPROC16   pAbortProc16;
    INT           CursPosX;          /* Current position */
    INT           CursPosY;
    INT           ArcDirection;
    XFORM         xformWorld2Wnd;    /* World-to-window transformation */
    XFORM         xformWorld2Vport;  /* World-to-viewport transformation */
    XFORM         xformVport2World;  /* Inverse of the above transformation */
    BOOL          vport2WorldValid;  /* Is xformVport2World valid? */
} DC;

typedef INT (*DEVICEFONTENUMPROC)(LPENUMLOGFONTEXW,NEWTEXTMETRICEXW *,DWORD,LPARAM);

/* extra stock object: default 1x1 bitmap for memory DCs */
#define DEFAULT_BITMAP (STOCK_LAST+1)

/* Rounds a floating point number to integer. The world-to-viewport
 * transformation process is done in floating point internally. This function
 * is then used to round these coordinates to integer values.
 */
static inline INT WINE_UNUSED GDI_ROUND(FLOAT val)
{
   return (int)floor(val + 0.5);
}

  /* GDI local heap */

extern void *GDI_GetObjPtr( HGDIOBJ, WORD );
extern void GDI_ReleaseObj( HGDIOBJ );

#define WINE_GGO_GRAY16_BITMAP 0x7f

#endif  /* __WINE_GDI_H */
