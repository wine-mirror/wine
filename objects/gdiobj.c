/*
 * GDI functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include "color.h"
#include "bitmap.h"
#include "brush.h"
#include "dc.h"
#include "font.h"
#include "heap.h"
#include "palette.h"
#include "pen.h"
#include "region.h"
#include "stddebug.h"
#include "debug.h"

WORD GDI_HeapSel = 0;

/* Object types for EnumObjects() */
#define OBJ_PEN             1
#define OBJ_BRUSH           2

/***********************************************************************
 *          GDI stock objects 
 */

static BRUSHOBJ WhiteBrush =
{
    { 0, BRUSH_MAGIC, 1 },             /* header */
    { BS_SOLID, RGB(255,255,255), 0 }  /* logbrush */
};

static BRUSHOBJ LtGrayBrush =
{
    { 0, BRUSH_MAGIC, 1 },             /* header */
    { BS_SOLID, RGB(192,192,192), 0 }  /* logbrush */
};

static BRUSHOBJ GrayBrush =
{
    { 0, BRUSH_MAGIC, 1 },             /* header */
    { BS_SOLID, RGB(128,128,128), 0 }  /* logbrush */
};

static BRUSHOBJ DkGrayBrush =
{
    { 0, BRUSH_MAGIC, 1 },          /* header */
    { BS_SOLID, RGB(64,64,64), 0 }  /* logbrush */
};

static BRUSHOBJ BlackBrush =
{
    { 0, BRUSH_MAGIC, 1 },       /* header */
    { BS_SOLID, RGB(0,0,0), 0 }  /* logbrush */
};

static BRUSHOBJ NullBrush =
{
    { 0, BRUSH_MAGIC, 1 },  /* header */
    { BS_NULL, 0, 0 }       /* logbrush */
};

static PENOBJ WhitePen =
{
    { 0, PEN_MAGIC, 1 },                     /* header */
    { PS_SOLID, { 1, 0 }, RGB(255,255,255) } /* logpen */
};

static PENOBJ BlackPen =
{
    { 0, PEN_MAGIC, 1 },               /* header */
    { PS_SOLID, { 1, 0 }, RGB(0,0,0) } /* logpen */
};

static PENOBJ NullPen =
{
    { 0, PEN_MAGIC, 1 },      /* header */
    { PS_NULL, { 1, 0 }, 0 }  /* logpen */
};

static FONTOBJ OEMFixedFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
      0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" }
};

static FONTOBJ AnsiFixedFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" }
};

static FONTOBJ AnsiVarFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "" }
};

static FONTOBJ SystemFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "" }
};

static FONTOBJ DeviceDefaultFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "" }
};

static FONTOBJ SystemFixedFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
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
 * GDI initialization.
 */
BOOL32 GDI_Init(void)
{
    HPALETTE16 hpalette;
    extern BOOL32 X11DRV_Init(void);
    extern BOOL32 DIB_Init(void);

    /* Initialize drivers */

    if (!X11DRV_Init()) return FALSE;
    if (!DIB_Init()) return FALSE;

      /* Create default palette */

    if (!(hpalette = COLOR_Init())) return FALSE;
    StockObjects[DEFAULT_PALETTE] = (GDIOBJHDR *)GDI_HEAP_LIN_ADDR( hpalette );

    return TRUE;
}


/***********************************************************************
 *           GDI_AllocObject
 */
HGDIOBJ16 GDI_AllocObject( WORD size, WORD magic )
{
    static DWORD count = 0;
    GDIOBJHDR * obj;
    HGDIOBJ16 handle = GDI_HEAP_ALLOC( size );
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
BOOL32 GDI_FreeObject( HGDIOBJ16 handle )
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
GDIOBJHDR * GDI_GetObjPtr( HGDIOBJ16 handle, WORD magic )
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
 *           DeleteObject16    (GDI.69)
 */
BOOL16 DeleteObject16( HGDIOBJ16 obj )
{
    return DeleteObject32( obj );
}


/***********************************************************************
 *           DeleteObject32    (GDI32.70)
 */
BOOL32 DeleteObject32( HGDIOBJ32 obj )
{
      /* Check if object is valid */

    GDIOBJHDR * header = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( obj );
    if (!header || HIWORD(obj)) return FALSE;

    dprintf_gdi(stddeb, "DeleteObject: %04x\n", obj );

      /* Delete object */

    switch(header->wMagic)
    {
      case PEN_MAGIC:     return GDI_FreeObject( obj );
      case BRUSH_MAGIC:   return BRUSH_DeleteObject( obj, (BRUSHOBJ*)header );
      case FONT_MAGIC:    return GDI_FreeObject( obj );
      case PALETTE_MAGIC: return PALETTE_DeleteObject(obj,(PALETTEOBJ*)header);
      case BITMAP_MAGIC:  return BITMAP_DeleteObject( obj, (BITMAPOBJ*)header);
      case REGION_MAGIC:  return REGION_DeleteObject( obj, (RGNOBJ*)header );
    }
    return FALSE;
}


/***********************************************************************
 *           GetStockObject16    (GDI.87)
 */
HGDIOBJ16 GetStockObject16( INT16 obj )
{
    return (HGDIOBJ16)GetStockObject32( obj );
}


/***********************************************************************
 *           GetStockObject32    (GDI32.220)
 */
HGDIOBJ32 GetStockObject32( INT32 obj )
{
    if ((obj < 0) || (obj >= NB_STOCK_OBJECTS)) return 0;
    if (!StockObjects[obj]) return 0;
    dprintf_gdi(stddeb, "GetStockObject: returning %d\n",
                FIRST_STOCK_HANDLE + obj );
    return (HGDIOBJ16)(FIRST_STOCK_HANDLE + obj);
}


/***********************************************************************
 *           GetObject16    (GDI.82)
 */
INT16 GetObject16( HANDLE16 handle, INT16 count, LPVOID buffer )
{
    GDIOBJHDR * ptr = NULL;
    dprintf_gdi(stddeb, "GetObject16: %04x %d %p\n", handle, count, buffer );
    if (!count) return 0;

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else
      ptr = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    if (!ptr) return 0;
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  return PEN_GetObject16( (PENOBJ *)ptr, count, buffer );
      case BRUSH_MAGIC: 
	  return BRUSH_GetObject16( (BRUSHOBJ *)ptr, count, buffer );
      case BITMAP_MAGIC: 
	  return BITMAP_GetObject16( (BITMAPOBJ *)ptr, count, buffer );
      case FONT_MAGIC:
	  return FONT_GetObject16( (FONTOBJ *)ptr, count, buffer );
      case PALETTE_MAGIC:
	  return PALETTE_GetObject( (PALETTEOBJ *)ptr, count, buffer );
    }
    return 0;
}


/***********************************************************************
 *           GetObject32A    (GDI32.204)
 */
INT32 GetObject32A( HANDLE32 handle, INT32 count, LPVOID buffer )
{
    GDIOBJHDR * ptr = NULL;
    dprintf_gdi(stddeb, "GetObject32A: %08x %d %p\n", handle, count, buffer );
    if (!count) return 0;

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else
      ptr = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    if (!ptr) return 0;
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  return PEN_GetObject32( (PENOBJ *)ptr, count, buffer );
      case BRUSH_MAGIC: 
	  return BRUSH_GetObject32( (BRUSHOBJ *)ptr, count, buffer );
      case BITMAP_MAGIC: 
	  return BITMAP_GetObject32( (BITMAPOBJ *)ptr, count, buffer );
      case FONT_MAGIC:
	  return FONT_GetObject32A( (FONTOBJ *)ptr, count, buffer );
      case PALETTE_MAGIC:
          fprintf( stderr, "GetObject32: magic %04x not implemented\n",
                   ptr->wMagic );
          break;
    }
    return 0;
}


/***********************************************************************
 *           GetObject32W    (GDI32.206)
 */
INT32 GetObject32W( HANDLE32 handle, INT32 count, LPVOID buffer )
{
    return GetObject32A( handle, count, buffer );
}


/***********************************************************************
 *           SelectObject16    (GDI.45)
 */
HGDIOBJ16 SelectObject16( HDC16 hdc, HGDIOBJ16 handle )
{
    return (HGDIOBJ16)SelectObject32( hdc, handle );
}


/***********************************************************************
 *           SelectObject32    (GDI32.299)
 */
HGDIOBJ32 SelectObject32( HDC32 hdc, HGDIOBJ32 handle )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pSelectObject) return 0;
    dprintf_gdi(stddeb, "SelectObject: hdc=%04x %04x\n", hdc, handle );
    return dc->funcs->pSelectObject( dc, handle );
}


/***********************************************************************
 *           UnrealizeObject16    (GDI.150)
 */
BOOL16 UnrealizeObject16( HGDIOBJ16 obj )
{
    return UnrealizeObject32( obj );
}


/***********************************************************************
 *           UnrealizeObject    (GDI32.358)
 */
BOOL32 UnrealizeObject32( HGDIOBJ32 obj )
{
      /* Check if object is valid */

    GDIOBJHDR * header = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( obj );
    if (!header) return FALSE;

    dprintf_gdi( stddeb, "UnrealizeObject: %04x\n", obj );

      /* Unrealize object */

    switch(header->wMagic)
    {
    case PALETTE_MAGIC: 
        return PALETTE_UnrealizeObject( obj, (PALETTEOBJ *)header );

    case BRUSH_MAGIC:
        /* Windows resets the brush origin. We don't need to. */
        break;
    }
    return TRUE;
}


/***********************************************************************
 *           EnumObjects16    (GDI.71)
 */
INT16 EnumObjects16( HDC16 hdc, INT16 nObjType, GOBJENUMPROC16 lpEnumFunc,
                     LPARAM lParam )
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
    
    INT16 i, retval = 0;
    LOGPEN16 *pen;
    LOGBRUSH16 *brush = NULL;

    dprintf_gdi( stddeb, "EnumObjects16: %04x %d %08lx %08lx\n",
                 hdc, nObjType, (DWORD)lpEnumFunc, lParam );
    switch(nObjType)
    {
    case OBJ_PEN:
        /* Enumerate solid pens */
        if (!(pen = SEGPTR_NEW(LOGPEN16))) break;
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
        {
            pen->lopnStyle   = PS_SOLID;
            pen->lopnWidth.x = 1;
            pen->lopnWidth.y = 0;
            pen->lopnColor   = solid_colors[i];
            retval = lpEnumFunc( SEGPTR_GET(pen), lParam );
            dprintf_gdi( stddeb, "EnumObjects16: solid pen %08lx, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }
        SEGPTR_FREE(pen);
        break;

    case OBJ_BRUSH:
        /* Enumerate solid brushes */
        if (!(brush = SEGPTR_NEW(LOGBRUSH16))) break;
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
        {
            brush->lbStyle = BS_SOLID;
            brush->lbColor = solid_colors[i];
            brush->lbHatch = 0;
            retval = lpEnumFunc( SEGPTR_GET(brush), lParam );
            dprintf_gdi( stddeb, "EnumObjects16: solid brush %08lx, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }

        /* Now enumerate hatched brushes */
        if (retval) for (i = HS_HORIZONTAL; i <= HS_DIAGCROSS; i++)
        {
            brush->lbStyle = BS_HATCHED;
            brush->lbColor = RGB(0,0,0);
            brush->lbHatch = i;
            retval = lpEnumFunc( SEGPTR_GET(brush), lParam );
            dprintf_gdi( stddeb, "EnumObjects16: hatched brush %d, ret=%d\n",
                         i, retval);
            if (!retval) break;
        }
        SEGPTR_FREE(brush);
        break;

    default:
        fprintf( stderr, "EnumObjects16: invalid type %d\n", nObjType );
        break;
    }
    return retval;
}


/***********************************************************************
 *           EnumObjects32    (GDI32.89)
 */
INT32 EnumObjects32( HDC32 hdc, INT32 nObjType, GOBJENUMPROC32 lpEnumFunc,
                     LPARAM lParam )
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
    
    INT32 i, retval = 0;
    LOGPEN32 pen;
    LOGBRUSH32 brush;

    dprintf_gdi( stddeb, "EnumObjects32: %04x %d %08lx %08lx\n",
                 hdc, nObjType, (DWORD)lpEnumFunc, lParam );
    switch(nObjType)
    {
    case OBJ_PEN:
        /* Enumerate solid pens */
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
        {
            pen.lopnStyle   = PS_SOLID;
            pen.lopnWidth.x = 1;
            pen.lopnWidth.y = 0;
            pen.lopnColor   = solid_colors[i];
            retval = lpEnumFunc( &pen, lParam );
            dprintf_gdi( stddeb, "EnumObjects32: solid pen %08lx, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }
        break;

    case OBJ_BRUSH:
        /* Enumerate solid brushes */
        for (i = 0; i < sizeof(solid_colors)/sizeof(solid_colors[0]); i++)
        {
            brush.lbStyle = BS_SOLID;
            brush.lbColor = solid_colors[i];
            brush.lbHatch = 0;
            retval = lpEnumFunc( &brush, lParam );
            dprintf_gdi( stddeb, "EnumObjects32: solid brush %08lx, ret=%d\n",
                         solid_colors[i], retval);
            if (!retval) break;
        }

        /* Now enumerate hatched brushes */
        if (retval) for (i = HS_HORIZONTAL; i <= HS_DIAGCROSS; i++)
        {
            brush.lbStyle = BS_HATCHED;
            brush.lbColor = RGB(0,0,0);
            brush.lbHatch = i;
            retval = lpEnumFunc( &brush, lParam );
            dprintf_gdi( stddeb, "EnumObjects32: hatched brush %d, ret=%d\n",
                         i, retval);
            if (!retval) break;
        }
        break;

    default:
        /* FIXME: implement Win32 types */
        fprintf( stderr, "EnumObjects32: invalid type %d\n", nObjType );
        break;
    }
    return retval;
}


/***********************************************************************
 *           IsGDIObject    (GDI.462)
 */
BOOL16 IsGDIObject( HGDIOBJ16 handle )
{
    GDIOBJHDR *object = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    if (object)
      return (object->wMagic>=PEN_MAGIC && object->wMagic<= METAFILE_DC_MAGIC);
    return FALSE;
}


/***********************************************************************
 *           MulDiv16   (GDI.128)
 */
INT16 MulDiv16( INT16 foo, INT16 bar, INT16 baz )
{
    INT32 ret;
    if (!baz) return -32768;
    ret = (foo * bar) / baz;
    if ((ret > 32767) || (ret < -32767)) return -32768;
    return ret;
}


/***********************************************************************
 *           MulDiv32   (KERNEL32.391)
 */
INT32 MulDiv32( INT32 foo, INT32 bar, INT32 baz )
{
#ifdef __GNUC__
    long long ret;
    if (!baz) return -1;
    ret = ((long long)foo * bar) / baz;
    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
#else
    if (!baz) return -1;
    return (foo * bar) / baz;
#endif
}
