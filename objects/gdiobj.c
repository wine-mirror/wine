/*
 * GDI functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include "gdi.h"
#include "color.h"
#include "bitmap.h"
#include "brush.h"
#include "font.h"
#include "palette.h"
#include "pen.h"
#include "region.h"
#include "callback.h"
#include "stddebug.h"
/* #define DEBUG_GDI */
#include "debug.h"
#include "xmalloc.h"

LPSTR GDI_Heap = NULL;
WORD GDI_HeapSel = 0;

/* Object types for EnumObjects() */
#define OBJ_PEN             1
#define OBJ_BRUSH           2

#define MAX_OBJ 			1024
HANDLE *lpPenBrushList = NULL;


/***********************************************************************
 *          GDI stock objects 
 */

static BRUSHOBJ WhiteBrush =
{
    { 0, BRUSH_MAGIC, 1, 0 },          /* header */
    { BS_SOLID, RGB(255,255,255), 0 }  /* logbrush */
};

static BRUSHOBJ LtGrayBrush =
{
    { 0, BRUSH_MAGIC, 1, 0 },          /* header */
    { BS_SOLID, RGB(192,192,192), 0 }  /* logbrush */
};

static BRUSHOBJ GrayBrush =
{
    { 0, BRUSH_MAGIC, 1, 0 },          /* header */
    { BS_SOLID, RGB(128,128,128), 0 }  /* logbrush */
};

static BRUSHOBJ DkGrayBrush =
{
    { 0, BRUSH_MAGIC, 1, 0 },       /* header */
    { BS_SOLID, RGB(64,64,64), 0 }  /* logbrush */
};

static BRUSHOBJ BlackBrush =
{
    { 0, BRUSH_MAGIC, 1, 0 },    /* header */
    { BS_SOLID, RGB(0,0,0), 0 }  /* logbrush */
};

static BRUSHOBJ NullBrush =
{
    { 0, BRUSH_MAGIC, 1, 0 },  /* header */
    { BS_NULL, 0, 0 }          /* logbrush */
};

static PENOBJ WhitePen =
{
    { 0, PEN_MAGIC, 1, 0 },                  /* header */
    { PS_SOLID, { 1, 0 }, RGB(255,255,255) } /* logpen */
};

static PENOBJ BlackPen =
{
    { 0, PEN_MAGIC, 1, 0 },            /* header */
    { PS_SOLID, { 1, 0 }, RGB(0,0,0) } /* logpen */
};

static PENOBJ NullPen =
{
    { 0, PEN_MAGIC, 1, 0 },   /* header */
    { PS_NULL, { 1, 0 }, 0 }  /* logpen */
};

static FONTOBJ OEMFixedFont =
{
    { 0, FONT_MAGIC, 1, 0 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
      0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" }
};

static FONTOBJ AnsiFixedFont =
{
    { 0, FONT_MAGIC, 1, 0 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" }
};

static FONTOBJ AnsiVarFont =
{
    { 0, FONT_MAGIC, 1, 0 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "" }
};

static FONTOBJ SystemFont =
{
    { 0, FONT_MAGIC, 1, 0 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "" }
};

static FONTOBJ DeviceDefaultFont =
{
    { 0, FONT_MAGIC, 1, 0 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "" }
};

static FONTOBJ SystemFixedFont =
{
    { 0, FONT_MAGIC, 1, 0 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" }
};


static GDIOBJHDR * StockObjects[NB_STOCK_OBJECTS] =
{
    (GDIOBJHDR *) &WhiteBrush,
    (GDIOBJHDR *) &LtGrayBrush,
    (GDIOBJHDR *) &GrayBrush,
    (GDIOBJHDR *) &DkGrayBrush,
    (GDIOBJHDR *) &BlackBrush,
    (GDIOBJHDR *) &NullBrush,
    (GDIOBJHDR *) &WhitePen,
    (GDIOBJHDR *) &BlackPen,
    (GDIOBJHDR *) &NullPen,
    NULL,
    (GDIOBJHDR *) &OEMFixedFont,
    (GDIOBJHDR *) &AnsiFixedFont,
    (GDIOBJHDR *) &AnsiVarFont,
    (GDIOBJHDR *) &SystemFont,
    (GDIOBJHDR *) &DeviceDefaultFont,
    NULL,            /* DEFAULT_PALETTE created by COLOR_Init */
    (GDIOBJHDR *) &SystemFixedFont
};


/***********************************************************************
 *           GDI_Init
 *
 * GDI initialisation.
 */
BOOL GDI_Init(void)
{
    HPALETTE hpalette;

#ifndef WINELIB
      /* Create GDI heap */

    if (!(GDI_HeapSel = GlobalAlloc(GMEM_FIXED, GDI_HEAP_SIZE))) return FALSE;
    GDI_Heap = GlobalLock( GDI_HeapSel );
    LocalInit( GDI_HeapSel, 0, GDI_HEAP_SIZE-1 );
#endif
    
      /* Create default palette */

    if (!(hpalette = COLOR_Init())) return FALSE;
    StockObjects[DEFAULT_PALETTE] = (GDIOBJHDR *)GDI_HEAP_LIN_ADDR( hpalette );

      /* Create default bitmap */

    if (!BITMAP_Init()) return FALSE;

      /* Initialise brush dithering */

    if (!BRUSH_Init()) return FALSE;

      /* Initialise fonts */

    if (!FONT_Init()) return FALSE;

    return TRUE;
}


/***********************************************************************
 *           GDI_AllocObject
 */
HANDLE GDI_AllocObject( WORD size, WORD magic )
{
    static DWORD count = 0;
    GDIOBJHDR * obj;
    HANDLE handle = GDI_HEAP_ALLOC( size );
    if (!handle) return 0;
    obj = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    obj->hNext   = 0;
    obj->wMagic  = magic;
    obj->dwCount = ++count;
    return handle;
}


/***********************************************************************
 *           GDI_FreeObject
 */
BOOL GDI_FreeObject( HANDLE handle )
{
    GDIOBJHDR * object;

      /* Can't free stock objects */
    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
        return TRUE;
    
    object = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    if (!object) return FALSE;
    object->wMagic = 0;  /* Mark it as invalid */

      /* Free object */
    
    GDI_HEAP_FREE( handle );
    return TRUE;
}

/***********************************************************************
 *           GDI_GetObjPtr
 *
 * Return a pointer to the GDI object associated to the handle.
 * Return NULL if the object has the wrong magic number.
 */
GDIOBJHDR * GDI_GetObjPtr( HANDLE handle, WORD magic )
{
    GDIOBJHDR * ptr = NULL;

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else 
      ptr = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    if (!ptr) return NULL;
    if ((magic != MAGIC_DONTCARE) && (ptr->wMagic != magic)) return NULL;
    return ptr;
}


/***********************************************************************
 *           DeleteObject    (GDI.69)
 */
BOOL DeleteObject( HGDIOBJ obj )
{
      /* Check if object is valid */

    GDIOBJHDR * header = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( obj );
    if (!header) return FALSE;

    dprintf_gdi(stddeb, "DeleteObject: %04x\n", obj );

      /* Delete object */

    switch(header->wMagic)
    {
      case PEN_MAGIC:     return GDI_FreeObject( obj );
      case BRUSH_MAGIC:   return BRUSH_DeleteObject( obj, (BRUSHOBJ*)header );
      case FONT_MAGIC:    return GDI_FreeObject( obj );
      case PALETTE_MAGIC: return GDI_FreeObject( obj );
      case BITMAP_MAGIC:  return BITMAP_DeleteObject( obj, (BITMAPOBJ*)header);
      case REGION_MAGIC:  return REGION_DeleteObject( obj, (RGNOBJ*)header );
    }
    return FALSE;
}


/***********************************************************************
 *           GetStockObject    (GDI.87)
 */
HANDLE GetStockObject( int obj )
{
    if ((obj < 0) || (obj >= NB_STOCK_OBJECTS)) return 0;
    if (!StockObjects[obj]) return 0;
    dprintf_gdi(stddeb, "GetStockObject: returning %d\n",
                FIRST_STOCK_HANDLE + obj );
    return (HANDLE)(FIRST_STOCK_HANDLE + obj);
}


/***********************************************************************
 *           GetObject    (GDI.82)
 */
int GetObject( HANDLE handle, int count, LPSTR buffer )
{
    GDIOBJHDR * ptr = NULL;
    dprintf_gdi(stddeb, "GetObject: %04x %d %p\n", handle, count, buffer );
    if (!count) return 0;

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else
      ptr = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    if (!ptr) return 0;
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  return PEN_GetObject( (PENOBJ *)ptr, count, buffer );
      case BRUSH_MAGIC: 
	  return BRUSH_GetObject( (BRUSHOBJ *)ptr, count, buffer );
      case BITMAP_MAGIC: 
	  return BITMAP_GetObject( (BITMAPOBJ *)ptr, count, buffer );
      case FONT_MAGIC:
	  return FONT_GetObject( (FONTOBJ *)ptr, count, buffer );
      case PALETTE_MAGIC:
	  return PALETTE_GetObject( (PALETTEOBJ *)ptr, count, buffer );
    }
    return 0;
}


/***********************************************************************
 *           SelectObject    (GDI.45)
 */
HANDLE SelectObject( HDC hdc, HANDLE handle )
{
    GDIOBJHDR * ptr = NULL;
    DC * dc;
    
    dprintf_gdi(stddeb, "SelectObject: %04x %04x\n", hdc, handle );
    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else 
      ptr = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    if (!ptr) return 0;
    
    dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) 
    {
	dc = (DC *)GDI_GetObjPtr(hdc, METAFILE_DC_MAGIC);
	if (!dc) return 0;
    }
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  return PEN_SelectObject( dc, handle, (PENOBJ *)ptr );
      case BRUSH_MAGIC:
	  return BRUSH_SelectObject( hdc, dc, handle, (BRUSHOBJ *)ptr );
      case BITMAP_MAGIC:
	  return BITMAP_SelectObject( hdc, dc, handle, (BITMAPOBJ *)ptr );
      case FONT_MAGIC:
	  return FONT_SelectObject( dc, handle, (FONTOBJ *)ptr );	  
      case REGION_MAGIC:
	  return (HANDLE)SelectClipRgn( hdc, handle );
    }
    return 0;
}


/***********************************************************************
 *           UnrealizeObject    (GDI.150)
 */
BOOL UnrealizeObject( HANDLE handle )
{
    dprintf_gdi(stdnimp, "UnrealizeObject: %04x\n", handle );
    return TRUE;
}


/***********************************************************************
 *           EnumObjects    (GDI.71)
 */
INT EnumObjects( HDC hdc, INT nObjType, GOBJENUMPROC lpEnumFunc, LPARAM lParam )
{
    /* Solid colors to enumerate */
    static const COLORREF solid_colors[] =
    { RGB(0x00,0x00,0x00), RGB(0xff,0xff,0xff),
      RGB(0xff,0x00,0x00), RGB(0x00,0xff,0x00),
      RGB(0x00,0x00,0xff), RGB(0xff,0xff,0x00),
      RGB(0xff,0x00,0xff), RGB(0x00,0xff,0xff),
      RGB(0x80,0x00,0x00), RGB(0x00,0x80,0x00),
      RGB(0x80,0x80,0x00), RGB(0x00,0x00,0x80),
      RGB(0x80,0x00,0x80), RGB(0x00,0x80,0x80),
      RGB(0x80,0x80,0x80), RGB(0xc0,0xc0,0xc0)
    };
    
    int i, retval = 0;

    dprintf_gdi( stddeb, "EnumObjects: %04x %d %08lx %08lx\n",
                 hdc, nObjType, (DWORD)lpEnumFunc, lParam );
    switch(nObjType)
    {
    case OBJ_PEN:
        /* Enumerate solid pens */
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
        {
            LOGPEN pen = { PS_SOLID, { 1, 0 }, solid_colors[i] };
            retval = CallEnumObjectsProc( lpEnumFunc, MAKE_SEGPTR(&pen),
                                          lParam );
            dprintf_gdi( stddeb, "EnumObject: solid pen %08lx, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }
        break;

    case OBJ_BRUSH:
        /* Enumerate solid brushes */
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
        {
            LOGBRUSH brush = { BS_SOLID, solid_colors[i], 0 };
            retval = CallEnumObjectsProc( lpEnumFunc, MAKE_SEGPTR(&brush),
                                          lParam );
            dprintf_gdi( stddeb, "EnumObject: solid brush %08lx, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }
        if (!retval) break;

        /* Now enumerate hatched brushes */
        for (i = HS_HORIZONTAL; i <= HS_DIAGCROSS; i++)
        {
            LOGBRUSH brush = { BS_HATCHED, RGB(0,0,0), i };
            retval = CallEnumObjectsProc( lpEnumFunc, MAKE_SEGPTR(&brush),
                                          lParam );
            dprintf_gdi( stddeb, "EnumObject: hatched brush %d, ret=%d\n",
                         i, retval);
            if (!retval) break;
        }
        break;

    default:
        fprintf( stderr, "EnumObjects: invalid type %d\n", nObjType );
        break;
    }
    return retval;
}


/***********************************************************************
 *           IsGDIObject    (GDI.462)
 */
BOOL IsGDIObject(HANDLE handle)
{
    GDIOBJHDR *object;

    object = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    /* FIXME: should check magic here */
    return (object != NULL);
}
