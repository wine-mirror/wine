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
    StockObjects[DEFAULT_PALETTE] = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( hpalette );

      /* Create default bitmap */

    if (!BITMAP_Init()) return FALSE;

      /* Initialise brush dithering */

    if (!BRUSH_Init()) return FALSE;

      /* Initialise fonts */

    if (!FONT_Init()) return FALSE;

    return TRUE;
}


/***********************************************************************
 *           GDI_AppendToPenBrushList
 */
BOOL GDI_AppendToPenBrushList(HANDLE hNewObj)
{
	HANDLE	 	*lphObj;
	int	       	i = 1;
	if (hNewObj == 0) return FALSE;
	if (lpPenBrushList == NULL) {
		lpPenBrushList = malloc(MAX_OBJ * sizeof(HANDLE));
		lpPenBrushList[0] = 0;
		dprintf_gdi(stddeb,"GDI_AppendToPenBrushList() lpPenBrushList allocated !\n");
	}
	for (lphObj = lpPenBrushList; i < MAX_OBJ; i++) {
		if (*lphObj == 0) {
			*lphObj = hNewObj;
			*(lphObj + 1) = 0;
			dprintf_gdi(stddeb,"GDI_AppendToPenBrushList(%04X) appended (count=%d)\n", hNewObj, i);
			return TRUE;
		}
		lphObj++;
	}
	return FALSE;
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
    if (magic == PEN_MAGIC || magic == BRUSH_MAGIC) {
      GDI_AppendToPenBrushList(handle);
    }
    return handle;
}


/***********************************************************************
 *           GDI_FreeObject
 */
BOOL GDI_FreeObject( HANDLE handle )
{
    GDIOBJHDR * object;

      /* Can't free stock objects */
    if (handle >= FIRST_STOCK_HANDLE) return TRUE;
    
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

    if (handle >= FIRST_STOCK_HANDLE)
    {
	if (handle < FIRST_STOCK_HANDLE + NB_STOCK_OBJECTS)
	    ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    }
    else ptr = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
    if (!ptr) return NULL;
    if (ptr->wMagic != magic) return NULL;
    return ptr;
}


/***********************************************************************
 *           DeleteObject    (GDI.69)
 */
BOOL DeleteObject( HANDLE obj )
{
      /* Check if object is valid */

    GDIOBJHDR * header = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( obj );
    if (!header) return FALSE;

    dprintf_gdi(stddeb, "DeleteObject: %d\n", obj );

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
    dprintf_gdi(stddeb, "GetStockObject: returning %04x\n", 
		FIRST_STOCK_HANDLE + obj );
    return FIRST_STOCK_HANDLE + obj;
}


/***********************************************************************
 *           GetObject    (GDI.82)
 */
int GetObject( HANDLE handle, int count, LPSTR buffer )
{
    GDIOBJHDR * ptr = NULL;
    dprintf_gdi(stddeb, "GetObject: %04x %d %p\n", handle, count, buffer );
    if (!count) return 0;

    if (handle >= FIRST_STOCK_HANDLE)
    {
	if (handle < FIRST_STOCK_HANDLE + NB_STOCK_OBJECTS)
	    ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    }
    else ptr = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
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
    
    dprintf_gdi(stddeb, "SelectObject: %d %04x\n", hdc, handle );
    if (handle >= FIRST_STOCK_HANDLE)
    {
	if (handle < FIRST_STOCK_HANDLE + NB_STOCK_OBJECTS)
	    ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    }
    else ptr = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
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
	  return SelectClipRgn( hdc, handle );
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
int EnumObjects(HDC hDC, int nObjType, FARPROC lpEnumFunc, LPSTR lpData)
{
  /*    HANDLE 		handle;
   DC 			*dc;*/
  HANDLE       	*lphObj;
  GDIOBJHDR 	*header;
  WORD         	wMagic;
  LPSTR		lpLog;  	/* Point to a LOGBRUSH or LOGPEN struct */
  HANDLE       	hLog;
  int	       	i, nRet = 0;
  
  if (lpEnumFunc == NULL) {
    fprintf(stderr,"EnumObjects // Bad EnumProc callback address !\n");
    return 0;
  }
  switch (nObjType) {
   case OBJ_PEN:
    wMagic = PEN_MAGIC;
    dprintf_gdi(stddeb,"EnumObjects(%04X, OBJ_PEN, %08lx, %p);\n",
		hDC, (LONG)lpEnumFunc, lpData);
    hLog = GDI_HEAP_ALLOC( sizeof(LOGPEN) );
    lpLog = (LPSTR) GDI_HEAP_LIN_ADDR(hLog);
    if (lpLog == NULL) {
      fprintf(stderr,"EnumObjects // Unable to alloc LOGPEN struct !\n");
      return 0;
    }
    break;
   case OBJ_BRUSH:
    wMagic = BRUSH_MAGIC;
    dprintf_gdi(stddeb,"EnumObjects(%04X, OBJ_BRUSH, %08lx, %p);\n",
		hDC, (LONG)lpEnumFunc, lpData);
    hLog = GDI_HEAP_ALLOC( sizeof(LOGBRUSH) );
    lpLog = (LPSTR) GDI_HEAP_LIN_ADDR(hLog);
    if (lpLog == NULL) {
      fprintf(stderr,"EnumObjects // Unable to alloc LOGBRUSH struct !\n");
      return 0;
    }
    break;
   default:
    fprintf(stderr,"EnumObjects(%04X, %04X, %08lx, %p); // Unknown OBJ type !\n", 
	    hDC, nObjType, (LONG)lpEnumFunc, lpData);
    return 0;
  }
#ifdef notdef  /* FIXME: stock object ptr won't work in callback */
  dprintf_gdi(stddeb,"EnumObjects // Stock Objects first !\n");
  for (i = 0; i < NB_STOCK_OBJECTS; i++) {
    header = StockObjects[i];
    if (header->wMagic == wMagic) {
      PEN_GetObject( (PENOBJ *)header, sizeof(LOGPEN), lpLog);
      BRUSH_GetObject( (BRUSHOBJ *)header, sizeof(LOGBRUSH),lpLog);
      dprintf_gdi(stddeb,"EnumObjects // StockObj lpLog=%p lpData=%p\n", lpLog, lpData);
      if (header->wMagic == BRUSH_MAGIC) {
	dprintf_gdi(stddeb,"EnumObjects // StockBrush lbStyle=%04X\n", ((LPLOGBRUSH)lpLog)->lbStyle);
	dprintf_gdi(stddeb,"EnumObjects // StockBrush lbColor=%08lX\n", ((LPLOGBRUSH)lpLog)->lbColor);
	dprintf_gdi(stddeb,"EnumObjects // StockBrush lbHatch=%04X\n", ((LPLOGBRUSH)lpLog)->lbHatch);
      }
      if (header->wMagic == PEN_MAGIC) {
	dprintf_gdi(stddeb,"EnumObjects // StockPen lopnStyle=%04X\n", ((LPLOGPEN)lpLog)->lopnStyle);
	dprintf_gdi(stddeb,"EnumObjects // StockPen lopnWidth=%d\n", ((LPLOGPEN)lpLog)->lopnWidth.x);
	dprintf_gdi(stddeb,"EnumObjects // StockPen lopnColor=%08lX\n", ((LPLOGPEN)lpLog)->lopnColor);
      }
      nRet = CallEnumObjectsProc( lpEnumFunc,
				 GDI_HEAP_SEG_ADDR(hLog),
				 (int)lpData );
      dprintf_gdi(stddeb,"EnumObjects // after Callback!\n");
      if (nRet == 0) {
	GDI_HEAP_FREE(hLog);
	dprintf_gdi(stddeb,"EnumObjects // EnumEnd requested by application !\n");
	return 0;
      }
    }
  }
  dprintf_gdi(stddeb,"EnumObjects // Now DC owned objects %p !\n", header);
#endif  /* notdef */
  
  if (lpPenBrushList == NULL) return 0;
  for (lphObj = lpPenBrushList; *lphObj != 0; ) {
    dprintf_gdi(stddeb,"EnumObjects // *lphObj=%04X\n", *lphObj);
    header = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR(*lphObj++);
    if (header->wMagic == wMagic) {
      dprintf_gdi(stddeb,"EnumObjects // DC_Obj lpLog=%p lpData=%p\n", lpLog, lpData);
      if (header->wMagic == BRUSH_MAGIC) {
	BRUSH_GetObject( (BRUSHOBJ *)header, sizeof(LOGBRUSH), lpLog);
	dprintf_gdi(stddeb,"EnumObjects // DC_Brush lbStyle=%04X\n", ((LPLOGBRUSH)lpLog)->lbStyle);
	dprintf_gdi(stddeb,"EnumObjects // DC_Brush lbColor=%08lX\n", ((LPLOGBRUSH)lpLog)->lbColor);
	dprintf_gdi(stddeb,"EnumObjects // DC_Brush lbHatch=%04X\n", ((LPLOGBRUSH)lpLog)->lbHatch);
      }
      if (header->wMagic == PEN_MAGIC) {
	PEN_GetObject( (PENOBJ *)header, sizeof(LOGPEN), lpLog);
	dprintf_gdi(stddeb,"EnumObjects // DC_Pen lopnStyle=%04X\n", ((LPLOGPEN)lpLog)->lopnStyle);
	dprintf_gdi(stddeb,"EnumObjects // DC_Pen lopnWidth=%d\n", ((LPLOGPEN)lpLog)->lopnWidth.x);
	dprintf_gdi(stddeb,"EnumObjects // DC_Pen lopnColor=%08lX\n", ((LPLOGPEN)lpLog)->lopnColor);
      }
      nRet = CallEnumObjectsProc(lpEnumFunc, GDI_HEAP_SEG_ADDR(hLog),
				 (LONG)lpData);
      if (nRet == 0)
        break;
    }
  }
  GDI_HEAP_FREE(hLog);
  dprintf_gdi(stddeb,"EnumObjects // End of enumeration !\n");
  return nRet;
}

/***********************************************************************
 *		IsGDIObject(GDI.462)
 */
BOOL IsGDIObject(HANDLE handle)
{
	GDIOBJHDR *object;

	object = (GDIOBJHDR *) GDI_HEAP_LIN_ADDR( handle );
	if (object)
		return TRUE;
	else
		return FALSE;
}
