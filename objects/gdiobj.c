/*
 * GDI functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdlib.h>
#include <stdio.h>
#include "user.h"
#include "gdi.h"
#include "prototypes.h"

MDESC *GDI_Heap = NULL;

/* Object types for EnumObjects() */
#define OBJ_PEN             1
#define OBJ_BRUSH           2

#define MAX_OBJ 			1024
HANDLE *lpPenBrushList = NULL;

extern HPALETTE COLOR_Init();  /* color.c */

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
BOOL GDI_Init()
{
    HPALETTE hpalette;
    struct segment_descriptor_s * s;

#ifndef WINELIB
    /* Create GDI heap */

    s = (struct segment_descriptor_s *)GetNextSegment( 0, 0x10000 );
    if (s == NULL) return FALSE;
    HEAP_Init( &GDI_Heap, s->base_addr, GDI_HEAP_SIZE );
#endif
    
      /* Create default palette */

    if (!(hpalette = COLOR_Init())) return FALSE;
    StockObjects[DEFAULT_PALETTE] = (GDIOBJHDR *) GDI_HEAP_ADDR( hpalette );

      /* Create default bitmap */

    if (!BITMAP_Init()) return FALSE;

      /* Initialise regions */

    if (!REGION_Init()) return FALSE;
    
      /* Initialise dithering */

    if (!DITHER_Init()) return FALSE;

    return TRUE;
}


/***********************************************************************
 *           GDI_AppendToPenBrushList
 */
BOOL GDI_AppendToPenBrushList(HANDLE hNewObj)
{
	HANDLE	 	*lphObj;
	int			i = 1;
	if (hNewObj == 0) return FALSE;
	if (lpPenBrushList == NULL) {
		lpPenBrushList = malloc(MAX_OBJ * sizeof(HANDLE));
		lpPenBrushList[0] = 0;
#ifdef DEBUG_GDI
		printf("GDI_AppendToPenBrushList() lpPenBrushList allocated !\n");
#endif
		}
	for (lphObj = lpPenBrushList; i < MAX_OBJ; i++) {
		if (*lphObj == 0) {
			*lphObj = hNewObj;
			*(lphObj + 1) = 0;
#ifdef DEBUG_GDI
			printf("GDI_AppendToPenBrushList(%04X) appended (count=%d)\n", hNewObj, i);
#endif
			return TRUE;
			}
		lphObj++;
		}
	return FALSE;
}


/***********************************************************************
 *           GDI_FindPrevObject
 *
 * Return the GDI object whose hNext field points to obj.
 */
HANDLE GDI_FindPrevObject( HANDLE first, HANDLE obj )
{
    HANDLE handle;
        
    for (handle = first; handle && (handle != obj); )
    {
	GDIOBJHDR * header = (GDIOBJHDR *) GDI_HEAP_ADDR( handle );
	handle = header->hNext;
    }
    return handle;
}


/***********************************************************************
 *           GDI_AllocObject
 */
HANDLE GDI_AllocObject( WORD size, WORD magic )
{
    static DWORD count = 0;
    GDIOBJHDR * obj;
    HANDLE handle = GDI_HEAP_ALLOC( GMEM_MOVEABLE, size );
    if (!handle) return 0;
    obj = (GDIOBJHDR *) GDI_HEAP_ADDR( handle );
    if (obj == NULL) {
    	printf("GDI_AllocObject // Error trying to get GDI_HEAD_ADDR !\n");
    	return 0;
    	}
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
    if (handle >= FIRST_STOCK_HANDLE) return FALSE;
    
    object = (GDIOBJHDR *) GDI_HEAP_ADDR( handle );
    if (!object) return FALSE;

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
    else ptr = (GDIOBJHDR *) GDI_HEAP_ADDR( handle );
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

    GDIOBJHDR * header = (GDIOBJHDR *) GDI_HEAP_ADDR( obj );
    if (!header) return FALSE;

#ifdef DEBUG_GDI
    printf( "DeleteObject: %d\n", obj );
#endif

      /* Delete object */

    switch(header->wMagic)
    {
      case PEN_MAGIC:     return GDI_FreeObject( obj );
      case BRUSH_MAGIC:   return BRUSH_DeleteObject( obj, header );
      case FONT_MAGIC:    return GDI_FreeObject( obj );
      case PALETTE_MAGIC: return GDI_FreeObject( obj );
      case BITMAP_MAGIC:  return BMP_DeleteObject( obj, header );
      case REGION_MAGIC:  return REGION_DeleteObject( obj, header );
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
#ifdef DEBUG_GDI
    printf( "GetStockObject: returning %04x\n", FIRST_STOCK_HANDLE + obj );
#endif
    return FIRST_STOCK_HANDLE + obj;
}


/***********************************************************************
 *           GetObject    (GDI.82)
 */
int GetObject( HANDLE handle, int count, LPSTR buffer )
{
    GDIOBJHDR * ptr = NULL;
#ifdef DEBUG_GDI
    printf( "GetObject: %04x %d %08x\n", handle, count, buffer );
#endif
    if (!count) return 0;

    if (handle >= FIRST_STOCK_HANDLE)
    {
	if (handle < FIRST_STOCK_HANDLE + NB_STOCK_OBJECTS)
	    ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    }
    else ptr = (GDIOBJHDR *) GDI_HEAP_ADDR( handle );
    if (!ptr) return 0;
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  return PEN_GetObject( (PENOBJ *)ptr, count, buffer );
      case BRUSH_MAGIC: 
	  return BRUSH_GetObject( (BRUSHOBJ *)ptr, count, buffer );
      case BITMAP_MAGIC: 
	  return BMP_GetObject( (BITMAPOBJ *)ptr, count, buffer );
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
    
#ifdef DEBUG_GDI
    printf( "SelectObject: %d %04x\n", hdc, handle );
#endif
    if (handle >= FIRST_STOCK_HANDLE)
    {
	if (handle < FIRST_STOCK_HANDLE + NB_STOCK_OBJECTS)
	    ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    }
    else ptr = (GDIOBJHDR *) GDI_HEAP_ADDR( handle );
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
#ifdef DEBUG_GDI
    printf( "UnrealizeObject: %04x\n", handle );
#endif
    return TRUE;
}


/***********************************************************************
 *           EnumObjects    (GDI.71)
 */
int EnumObjects(HDC hDC, int nObjType, FARPROC lpEnumFunc, LPSTR lpData)
{
    HANDLE 		handle;
    DC 			*dc;
	HANDLE		*lphObj;
	GDIOBJHDR 	*header;
	WORD  		wMagic;
	LPSTR		lpLog;  	/* Point to a LOGBRUSH or LOGPEN struct */
	HANDLE 		hLog;
	int			i, nRet;
	if (lpEnumFunc == NULL) {
		printf("EnumObjects // Bad EnumProc callback address !\n");
		return 0;
		}
	switch (nObjType) {
		case OBJ_PEN:
			wMagic = PEN_MAGIC;
			printf("EnumObjects(%04X, OBJ_PEN, %08X, %08X);\n", 
									hDC, lpEnumFunc, lpData);
			hLog = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(LOGPEN));
			lpLog = (LPSTR) USER_HEAP_ADDR(hLog);
			if (lpLog == NULL) {
				printf("EnumObjects // Unable to alloc LOGPEN struct !\n");
				return 0;
				}
			break;
		case OBJ_BRUSH:
			wMagic = BRUSH_MAGIC;
			printf("EnumObjects(%04X, OBJ_BRUSH, %08X, %08X);\n", 
									hDC, lpEnumFunc, lpData);
			hLog = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(LOGBRUSH));
			lpLog = (LPSTR) USER_HEAP_ADDR(hLog);
			if (lpLog == NULL) {
				printf("EnumObjects // Unable to alloc LOGBRUSH struct !\n");
				return 0;
				}
			break;
		default:
			printf("EnumObjects(%04X, %04X, %08X, %08X); // Unknown OBJ type !\n", 
						hDC, nObjType, lpEnumFunc, lpData);
			return 0;
		}
	printf("EnumObjects // Stock Objects first !\n");
	for (i = 0; i < NB_STOCK_OBJECTS; i++) {
		header = StockObjects[i];
		if (header->wMagic == wMagic) {
			PEN_GetObject( (PENOBJ *)header, sizeof(LOGPEN), (LPLOGPEN)lpLog);
			BRUSH_GetObject( (BRUSHOBJ *)header, sizeof(LOGBRUSH), (LPLOGBRUSH)lpLog);
			printf("EnumObjects // StockObj lpLog=%08X lpData=%08X\n", lpLog, lpData);
			if (header->wMagic == BRUSH_MAGIC) {
				printf("EnumObjects // StockBrush lbStyle=%04X\n", ((LPLOGBRUSH)lpLog)->lbStyle);
				printf("EnumObjects // StockBrush lbColor=%08X\n", ((LPLOGBRUSH)lpLog)->lbColor);
				printf("EnumObjects // StockBrush lbHatch=%04X\n", ((LPLOGBRUSH)lpLog)->lbHatch);
				}
			if (header->wMagic == PEN_MAGIC) {
				printf("EnumObjects // StockPen lopnStyle=%04X\n", ((LPLOGPEN)lpLog)->lopnStyle);
				printf("EnumObjects // StockPen lopnWidth=%08X\n", ((LPLOGPEN)lpLog)->lopnWidth);
				printf("EnumObjects // StockPen lopnColor=%08X\n", ((LPLOGPEN)lpLog)->lopnColor);
				}
			nRet = 1;
/*
#ifdef WINELIB
			nRet = (*lpEnumFunc)(lpLog, lpData);
#else
			nRet = CallBack16(lpEnumFunc, 4, 2, (int)lpLog,	2, (int)lpData);
#endif
*/
			printf("EnumObjects // after CallBack16 !\n");
			if (nRet == 0) {
				USER_HEAP_FREE(hLog);
				printf("EnumObjects // EnumEnd requested by application !\n");
				return 0;
				}
			}
		}
	if (lpPenBrushList == NULL) return 0;
	printf("EnumObjects // Now DC owned objects %08X !\n", header);
	for (lphObj = lpPenBrushList; *lphObj != 0; ) {
#ifdef DEBUG_GDI
		printf("EnumObjects // *lphObj=%04X\n", *lphObj);
#endif
		header = (GDIOBJHDR *) GDI_HEAP_ADDR(*lphObj++);
		if (header->wMagic == wMagic) {
#ifdef DEBUG_GDI
			printf("EnumObjects // DC_Obj lpLog=%08X lpData=%08X\n", lpLog, lpData);
#endif
			if (header->wMagic == BRUSH_MAGIC) {
				BRUSH_GetObject( (BRUSHOBJ *)header, sizeof(LOGBRUSH), (LPLOGBRUSH)lpLog);
				printf("EnumObjects // DC_Brush lbStyle=%04X\n", ((LPLOGBRUSH)lpLog)->lbStyle);
				printf("EnumObjects // DC_Brush lbColor=%08X\n", ((LPLOGBRUSH)lpLog)->lbColor);
				printf("EnumObjects // DC_Brush lbHatch=%04X\n", ((LPLOGBRUSH)lpLog)->lbHatch);
				}
			if (header->wMagic == PEN_MAGIC) {
				PEN_GetObject( (PENOBJ *)header, sizeof(LOGPEN), (LPLOGPEN)lpLog);
				printf("EnumObjects // DC_Pen lopnStyle=%04X\n", ((LPLOGPEN)lpLog)->lopnStyle);
				printf("EnumObjects // DC_Pen lopnWidth=%08X\n", ((LPLOGPEN)lpLog)->lopnWidth);
				printf("EnumObjects // DC_Pen lopnColor=%08X\n", ((LPLOGPEN)lpLog)->lopnColor);
				}
/*
#ifdef WINELIB
			nRet = (*lpEnumFunc)(lpLog, lpData);
#else
			nRet = CallBack16(lpEnumFunc, 4, 2, (int)lpLog,	2, (int)lpData);
#endif
*/
			nRet = 1;
			printf("EnumObjects // after CallBack16 !\n");
			if (nRet == 0) {
				USER_HEAP_FREE(hLog);
				printf("EnumObjects // EnumEnd requested by application !\n");
				return 0;
				}
			}
		}
	USER_HEAP_FREE(hLog);
	printf("EnumObjects // End of enumeration !\n");
	return 0;
}

/***********************************************************************
 *		IsGDIObject(GDI.462)
 */
BOOL IsGDIObject(HANDLE handle)
{
	GDIOBJHDR *object;

	object = (GDIOBJHDR *) GDI_HEAP_ADDR( handle );
	if (object)
		return TRUE;
	else
		return FALSE;
}
