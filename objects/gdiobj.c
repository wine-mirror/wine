/*
 * GDI functions
 *
 * Copyright 1993 Alexandre Julliard
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING
#include "x11drv.h"
#else /* !defined(X_DISPLAY_MISSING) */
#include "ttydrv.h"
#endif /* !defined(X_DISPLAY_MISSING */

#include <stdlib.h>

#include "bitmap.h"
#include "brush.h"
#include "dc.h"
#include "font.h"
#include "heap.h"
#include "options.h"
#include "palette.h"
#include "pen.h"
#include "region.h"
#include "debugtools.h"
#include "gdi.h"
#include "tweak.h"
#include "winuser.h"

DEFAULT_DEBUG_CHANNEL(gdi)

/**********************************************************************/

GDI_DRIVER *GDI_Driver = NULL;

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
/* FIXME : this should perhaps be BS_HATCHED, at least for 1 bitperpixel */
    { BS_SOLID, RGB(192,192,192), 0 }  /* logbrush */
};

static BRUSHOBJ GrayBrush =
{
    { 0, BRUSH_MAGIC, 1 },             /* header */
/* FIXME : this should perhaps be BS_HATCHED, at least for 1 bitperpixel */
    { BS_SOLID, RGB(128,128,128), 0 }  /* logbrush */
};

static BRUSHOBJ DkGrayBrush =
{
    { 0, BRUSH_MAGIC, 1 },          /* header */
/* This is BS_HATCHED, for 1 bitperpixel. This makes the spray work in pbrush */
/* NB_HATCH_STYLES is an index into HatchBrushes */
    { BS_HATCHED, RGB(0,0,0), NB_HATCH_STYLES }  /* logbrush */
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
    { 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, OEM_CHARSET,
      0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" }
};
/* Filler to make the location counter dword aligned again.  This is necessary
   since (a) FONTOBJ is packed, (b) gcc places initialised variables in the code
   segment, and (c) Solaris assembler is stupid.  */
static UINT16 align_OEMFixedFont = 1;

static FONTOBJ AnsiFixedFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" }
};
static UINT16 align_AnsiFixedFont = 1;

static FONTOBJ AnsiVarFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "MS Sans Serif" }
};
static UINT16 align_AnsiVarFont = 1;

static FONTOBJ SystemFont =
{
    { 0, FONT_MAGIC, 1 },
    { 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "System" }
};
static UINT16 align_SystemFont = 1;

static FONTOBJ DeviceDefaultFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "" }
};
static UINT16 align_DeviceDefaultFont = 1;

static FONTOBJ SystemFixedFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "" }
};
static UINT16 align_SystemFixedFont = 1;

/* FIXME: Is this correct? */
static FONTOBJ DefaultGuiFont =
{
    { 0, FONT_MAGIC, 1 },   /* header */
    { 0, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
      0, 0, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "MS Sans Serif" }
};
static UINT16 align_DefaultGuiFont = 1;


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
    NULL,            /* DEFAULT_PALETTE created by PALETTE_Init */
    (GDIOBJHDR *) &SystemFixedFont,
    (GDIOBJHDR *) &DefaultGuiFont
};

HBITMAP hPseudoStockBitmap; /* 1x1 bitmap for memory DCs */

/******************************************************************************
 *
 *   void  ReadFontInformation(
 *      char const  *fontName,
 *      FONTOBJ  *font,
 *      int  defHeight,
 *      int  defBold,
 *      int  defItalic,
 *      int  defUnderline,
 *      int  defStrikeOut )
 *
 *   ReadFontInformation() checks the Wine configuration file's Tweak.Fonts
 *   section for entries containing fontName.Height, fontName.Bold, etc.,
 *   where fontName is the name specified in the call (e.g., "System").  It
 *   attempts to be user friendly by accepting 'n', 'N', 'f', 'F', or '0' as
 *   the first character in the boolean attributes (bold, italic, and
 *   underline).
 *****************************************************************************/

static void  ReadFontInformation(
    char const *fontName,
    FONTOBJ *font,
    int  defHeight,
    int  defBold,
    int  defItalic,
    int  defUnderline,
    int  defStrikeOut )
{
    char  key[256];

    /* In order for the stock fonts to be independent of 
     * mapping mode, the height (& width) must be 0
     */
    sprintf(key, "%s.Height", fontName);
    font->logfont.lfHeight =
	PROFILE_GetWineIniInt("Tweak.Fonts", key, defHeight);

    sprintf(key, "%s.Bold", fontName);
    font->logfont.lfWeight =
	(PROFILE_GetWineIniBool("Tweak.Fonts", key, defBold)) ?
	FW_BOLD : FW_NORMAL;

    sprintf(key, "%s.Italic", fontName);
    font->logfont.lfItalic =
	PROFILE_GetWineIniBool("Tweak.Fonts", key, defItalic);

    sprintf(key, "%s.Underline", fontName);
    font->logfont.lfUnderline =
	PROFILE_GetWineIniBool("Tweak.Fonts", key, defUnderline);

    sprintf(key, "%s.StrikeOut", fontName);
    font->logfont.lfStrikeOut =
	PROFILE_GetWineIniBool("Tweak.Fonts", key, defStrikeOut);

    return;
}

/***********************************************************************
 * Because the stock fonts have their structure initialized with
 * a height of 0 to keep them independent of mapping mode, simply
 * returning the LOGFONT as is will not work correctly.
 * These "FixStockFontSizeXXX()" methods will get the correct
 * size for the fonts.
 */
static void GetFontMetrics(HFONT handle, LPTEXTMETRICA lptm)
{
  HDC         hdc;
  HFONT       hOldFont;

  hdc = CreateDCA("DISPLAY", NULL, NULL, NULL);

  hOldFont = (HFONT)SelectObject(hdc, handle);

  GetTextMetricsA(hdc, lptm);

  SelectObject(hdc, hOldFont);

  DeleteDC(hdc);
}

static inline void FixStockFontSize16(
  HFONT  handle, 
  INT16  count, 
  LPVOID buffer)
{
  TEXTMETRICA tm;
  LOGFONT16*  pLogFont = (LOGFONT16*)buffer;

  /*
   * Was the lfHeight field copied (it's the first field)?
   * If it was and it was null, replace the height.
   */
  if ( (count >= 2*sizeof(INT16)) &&
       (pLogFont->lfHeight == 0) )
  {
    GetFontMetrics(handle, &tm);
    
    pLogFont->lfHeight = tm.tmHeight;
    pLogFont->lfWidth  = tm.tmAveCharWidth;
  }
}

static inline void FixStockFontSizeA(
  HFONT  handle, 
  INT    count, 
  LPVOID buffer)
{
  TEXTMETRICA tm;
  LOGFONTA*  pLogFont = (LOGFONTA*)buffer;

  /*
   * Was the lfHeight field copied (it's the first field)?
   * If it was and it was null, replace the height.
   */
  if ( (count >= 2*sizeof(INT)) &&
       (pLogFont->lfHeight == 0) )
  {
    GetFontMetrics(handle, &tm);

    pLogFont->lfHeight = tm.tmHeight;
    pLogFont->lfWidth  = tm.tmAveCharWidth;
  }
}

/**
 * Since the LOGFONTA and LOGFONTW structures are identical up to the 
 * lfHeight member (the one of interest in this case) we simply define
 * the W version as the A version. 
 */
#define FixStockFontSizeW FixStockFontSizeA



/***********************************************************************
 *           GDI_Init
 *
 * GDI initialization.
 */
BOOL GDI_Init(void)
{
    BOOL systemIsBold = (TWEAK_WineLook == WIN31_LOOK);

    /* Kill some warnings.  */
    (void)align_OEMFixedFont;
    (void)align_AnsiFixedFont;
    (void)align_AnsiVarFont;
    (void)align_SystemFont;
    (void)align_DeviceDefaultFont;
    (void)align_SystemFixedFont;
    (void)align_DefaultGuiFont;

    /* TWEAK: Initialize font hints */
    ReadFontInformation("OEMFixed", &OEMFixedFont, 0, 0, 0, 0, 0);
    ReadFontInformation("AnsiFixed", &AnsiFixedFont, 0, 0, 0, 0, 0);
    ReadFontInformation("AnsiVar", &AnsiVarFont, 0, 0, 0, 0, 0);
    ReadFontInformation("System", &SystemFont, 0, systemIsBold, 0, 0, 0);
    ReadFontInformation("DeviceDefault", &DeviceDefaultFont, 0, 0, 0, 0, 0);
    ReadFontInformation("SystemFixed", &SystemFixedFont, 0, systemIsBold, 0, 0, 0);
    ReadFontInformation("DefaultGui", &DefaultGuiFont, 0, 0, 0, 0, 0);

    /* Initialize drivers */

#ifndef X_DISPLAY_MISSING
    GDI_Driver = &X11DRV_GDI_Driver;
#else /* !defined(X_DISPLAY_MISSING) */
    GDI_Driver = &TTYDRV_GDI_Driver;
#endif /* !defined(X_DISPLAY_MISSING */

    GDI_Driver->pInitialize();

    /* Create default palette */

    /* DR well *this* palette can't be moveable (?) */
    {
    HPALETTE16 hpalette = PALETTE_Init();
    if( !hpalette )
        return FALSE;
    StockObjects[DEFAULT_PALETTE] = (GDIOBJHDR *)GDI_HEAP_LOCK( hpalette );
    }

    hPseudoStockBitmap = CreateBitmap( 1, 1, 1, 1, NULL ); 
    return TRUE;
}


/***********************************************************************
 *           GDI_AllocObject
 */
HGDIOBJ16 GDI_AllocObject( WORD size, WORD magic )
{
    static DWORD count = 0;
    GDIOBJHDR * obj;
    HGDIOBJ16 handle;
    if ( magic == DC_MAGIC || magic == METAFILE_DC_MAGIC )
      handle = GDI_HEAP_ALLOC( size );
    else 
      handle = GDI_HEAP_ALLOC_MOVEABLE( size );
    if (!handle) return 0;
    obj = (GDIOBJHDR *) GDI_HEAP_LOCK( handle );
    obj->hNext   = 0;
    obj->wMagic  = magic;
    obj->dwCount = ++count;
    GDI_HEAP_UNLOCK( handle );
    return handle;
}


/***********************************************************************
 *           GDI_FreeObject
 */
BOOL GDI_FreeObject( HGDIOBJ16 handle )
{
    GDIOBJHDR * object;

      /* Can't free stock objects */
    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
        return TRUE;
    
    object = (GDIOBJHDR *) GDI_HEAP_LOCK( handle );
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
 * Movable GDI objects are locked in memory: it is up to the caller to unlock
 * it after the caller is done with the pointer.
 */
GDIOBJHDR * GDI_GetObjPtr( HGDIOBJ16 handle, WORD magic )
{
    GDIOBJHDR * ptr = NULL;

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else 
      ptr = (GDIOBJHDR *) GDI_HEAP_LOCK( handle );
    if (!ptr) return NULL;
    if ((magic != MAGIC_DONTCARE) && (ptr->wMagic != magic)) 
    {
      GDI_HEAP_UNLOCK( handle );
      return NULL;
    }
    return ptr;
}


/***********************************************************************
 *           DeleteObject16    (GDI.69)
 */
BOOL16 WINAPI DeleteObject16( HGDIOBJ16 obj )
{
    return DeleteObject( obj );
}


/***********************************************************************
 *           DeleteObject32    (GDI32.70)
 */
BOOL WINAPI DeleteObject( HGDIOBJ obj )
{
      /* Check if object is valid */

    GDIOBJHDR * header;
    if (HIWORD(obj)) return FALSE;
    if ((obj >= FIRST_STOCK_HANDLE) && (obj <= LAST_STOCK_HANDLE))
        return TRUE;
    if (obj == hPseudoStockBitmap) return TRUE;
    if (!(header = (GDIOBJHDR *) GDI_HEAP_LOCK( obj ))) return FALSE;

    TRACE("%04x\n", obj );

      /* Delete object */

    switch(header->wMagic)
    {
      case PEN_MAGIC:     return GDI_FreeObject( obj );
      case BRUSH_MAGIC:   return BRUSH_DeleteObject( obj, (BRUSHOBJ*)header );
      case FONT_MAGIC:    return GDI_FreeObject( obj );
      case PALETTE_MAGIC: return PALETTE_DeleteObject(obj,(PALETTEOBJ*)header);
      case BITMAP_MAGIC:  return BITMAP_DeleteObject( obj, (BITMAPOBJ*)header);
      case REGION_MAGIC:  return REGION_DeleteObject( obj, (RGNOBJ*)header );
      case DC_MAGIC:      return DeleteDC(obj);
      case 0 :
        WARN("Already deleted\n");
        break;
      default:
        WARN("Unknown magic number (%d)\n",header->wMagic);
    }
    return FALSE;
}

/***********************************************************************
 *           GetStockObject16    (GDI.87)
 */
HGDIOBJ16 WINAPI GetStockObject16( INT16 obj )
{
    return (HGDIOBJ16)GetStockObject( obj );
}


/***********************************************************************
 *           GetStockObject32    (GDI32.220)
 */
HGDIOBJ WINAPI GetStockObject( INT obj )
{
    if ((obj < 0) || (obj >= NB_STOCK_OBJECTS)) return 0;
    if (!StockObjects[obj]) return 0;
    TRACE("returning %d\n",
                FIRST_STOCK_HANDLE + obj );
    return (HGDIOBJ16)(FIRST_STOCK_HANDLE + obj);
}


/***********************************************************************
 *           GetObject16    (GDI.82)
 */
INT16 WINAPI GetObject16( HANDLE16 handle, INT16 count, LPVOID buffer )
{
    GDIOBJHDR * ptr = NULL;
    INT16 result = 0;
    TRACE("%04x %d %p\n", handle, count, buffer );
    if (!count) return 0;

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else
      ptr = (GDIOBJHDR *) GDI_HEAP_LOCK( handle );
    if (!ptr) return 0;
    
    switch(ptr->wMagic)
      {
      case PEN_MAGIC:
	result = PEN_GetObject16( (PENOBJ *)ptr, count, buffer );
	break;
      case BRUSH_MAGIC: 
	result = BRUSH_GetObject16( (BRUSHOBJ *)ptr, count, buffer );
	break;
      case BITMAP_MAGIC: 
	result = BITMAP_GetObject16( (BITMAPOBJ *)ptr, count, buffer );
	break;
      case FONT_MAGIC:
	result = FONT_GetObject16( (FONTOBJ *)ptr, count, buffer );
	
	/*
	 * Fix the LOGFONT structure for the stock fonts
	 */
	if ( (handle >= FIRST_STOCK_HANDLE) && 
	     (handle <= LAST_STOCK_HANDLE) )
	  FixStockFontSize16(handle, count, buffer);	
	break;
      case PALETTE_MAGIC:
	result = PALETTE_GetObject( (PALETTEOBJ *)ptr, count, buffer );
	break;
      }
    GDI_HEAP_UNLOCK( handle );
    return result;
}


/***********************************************************************
 *           GetObject32A    (GDI32.204)
 */
INT WINAPI GetObjectA( HANDLE handle, INT count, LPVOID buffer )
{
    GDIOBJHDR * ptr = NULL;
    INT result = 0;
    TRACE("%08x %d %p\n", handle, count, buffer );
    if (!count) return 0;

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else
      ptr = (GDIOBJHDR *) GDI_HEAP_LOCK( handle );
    if (!ptr) return 0;
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  result = PEN_GetObject( (PENOBJ *)ptr, count, buffer );
	  break;
      case BRUSH_MAGIC: 
	  result = BRUSH_GetObject( (BRUSHOBJ *)ptr, count, buffer );
	  break;
      case BITMAP_MAGIC: 
	  result = BITMAP_GetObject( (BITMAPOBJ *)ptr, count, buffer );
	  break;
      case FONT_MAGIC:
	  result = FONT_GetObjectA( (FONTOBJ *)ptr, count, buffer );
	  
	  /*
	   * Fix the LOGFONT structure for the stock fonts
	   */
	  if ( (handle >= FIRST_STOCK_HANDLE) && 
	       (handle <= LAST_STOCK_HANDLE) )
	    FixStockFontSizeA(handle, count, buffer);
	  break;
      case PALETTE_MAGIC:
	  result = PALETTE_GetObject( (PALETTEOBJ *)ptr, count, buffer );
	  break;
      default:
          FIXME("Magic %04x not implemented\n",
                   ptr->wMagic );
          break;
    }
    GDI_HEAP_UNLOCK( handle );
    return result;
}
/***********************************************************************
 *           GetObject32W    (GDI32.206)
 */
INT WINAPI GetObjectW( HANDLE handle, INT count, LPVOID buffer )
{
    GDIOBJHDR * ptr = NULL;
    INT result = 0;
    TRACE("%08x %d %p\n", handle, count, buffer );
    if (!count) return 0;

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else
      ptr = (GDIOBJHDR *) GDI_HEAP_LOCK( handle );
    if (!ptr) return 0;
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  result = PEN_GetObject( (PENOBJ *)ptr, count, buffer );
	  break;
      case BRUSH_MAGIC: 
	  result = BRUSH_GetObject( (BRUSHOBJ *)ptr, count, buffer );
	  break;
      case BITMAP_MAGIC: 
	  result = BITMAP_GetObject( (BITMAPOBJ *)ptr, count, buffer );
	  break;
      case FONT_MAGIC:
	  result = FONT_GetObjectW( (FONTOBJ *)ptr, count, buffer );

	  /*
	   * Fix the LOGFONT structure for the stock fonts
	   */
	  if ( (handle >= FIRST_STOCK_HANDLE) && 
	       (handle <= LAST_STOCK_HANDLE) )
	    FixStockFontSizeW(handle, count, buffer);
	  break;
      case PALETTE_MAGIC:
	  result = PALETTE_GetObject( (PALETTEOBJ *)ptr, count, buffer );
	  break;
      default:
          FIXME("Magic %04x not implemented\n",
                   ptr->wMagic );
          break;
    }
    GDI_HEAP_UNLOCK( handle );
    return result;
}

/***********************************************************************
 *           GetObjectType    (GDI32.205)
 */
DWORD WINAPI GetObjectType( HANDLE handle )
{
    GDIOBJHDR * ptr = NULL;
    INT result = 0;
    TRACE("%08x\n", handle );

    if ((handle >= FIRST_STOCK_HANDLE) && (handle <= LAST_STOCK_HANDLE))
      ptr = StockObjects[handle - FIRST_STOCK_HANDLE];
    else
      ptr = (GDIOBJHDR *) GDI_HEAP_LOCK( handle );
    if (!ptr) return 0;
    
    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
	  result = OBJ_PEN;
	  break;
      case BRUSH_MAGIC: 
	  result = OBJ_BRUSH;
	  break;
      case BITMAP_MAGIC: 
	  result = OBJ_BITMAP;
	  break;
      case FONT_MAGIC:
	  result = OBJ_FONT;
	  break;
      case PALETTE_MAGIC:
	  result = OBJ_PAL;
	  break;
      case REGION_MAGIC:
	  result = OBJ_REGION;
	  break;
      case DC_MAGIC:
	  result = OBJ_DC;
	  break;
      case META_DC_MAGIC:
	  result = OBJ_METADC;
	  break;
      case METAFILE_MAGIC:
	  result = OBJ_METAFILE;
	  break;
      case METAFILE_DC_MAGIC:
	  result = OBJ_METADC;
	  break;
      case ENHMETAFILE_MAGIC:
	  result = OBJ_ENHMETAFILE;
	  break;
      case ENHMETAFILE_DC_MAGIC:
	  result = OBJ_ENHMETADC;
	  break;
      default:
	  FIXME("Magic %04x not implemented\n",
			   ptr->wMagic );
	  break;
    }
    GDI_HEAP_UNLOCK( handle );
    return result;
}

/***********************************************************************
 *           GetCurrentObject    	(GDI32.166)
 */
HANDLE WINAPI GetCurrentObject(HDC hdc,UINT type)
{
    DC * dc = DC_GetDCPtr( hdc );

    if (!dc) 
    	return 0;
    switch (type) {
    case OBJ_PEN:	return dc->w.hPen;
    case OBJ_BRUSH:	return dc->w.hBrush;
    case OBJ_PAL:	return dc->w.hPalette;
    case OBJ_FONT:	return dc->w.hFont;
    case OBJ_BITMAP:	return dc->w.hBitmap;
    default:
    	/* the SDK only mentions those above */
    	WARN("(%08x,%d): unknown type.\n",hdc,type);
	return 0;
    }
}


/***********************************************************************
 *           SelectObject16    (GDI.45)
 */
HGDIOBJ16 WINAPI SelectObject16( HDC16 hdc, HGDIOBJ16 handle )
{
    return (HGDIOBJ16)SelectObject( hdc, handle );
}


/***********************************************************************
 *           SelectObject32    (GDI32.299)
 */
HGDIOBJ WINAPI SelectObject( HDC hdc, HGDIOBJ handle )
{
    DC * dc = DC_GetDCPtr( hdc );
    if (!dc || !dc->funcs->pSelectObject) return 0;
    TRACE("hdc=%04x %04x\n", hdc, handle );
    return dc->funcs->pSelectObject( dc, handle );
}


/***********************************************************************
 *           UnrealizeObject16    (GDI.150)
 */
BOOL16 WINAPI UnrealizeObject16( HGDIOBJ16 obj )
{
    return UnrealizeObject( obj );
}


/***********************************************************************
 *           UnrealizeObject    (GDI32.358)
 */
BOOL WINAPI UnrealizeObject( HGDIOBJ obj )
{
    BOOL result = TRUE;
  /* Check if object is valid */

    GDIOBJHDR * header = (GDIOBJHDR *) GDI_HEAP_LOCK( obj );
    if (!header) return FALSE;

    TRACE("%04x\n", obj );

      /* Unrealize object */

    switch(header->wMagic)
    {
    case PALETTE_MAGIC: 
        result = PALETTE_UnrealizeObject( obj, (PALETTEOBJ *)header );
	break;

    case BRUSH_MAGIC:
        /* Windows resets the brush origin. We don't need to. */
        break;
    }
    GDI_HEAP_UNLOCK( obj );
    return result;
}


/***********************************************************************
 *           EnumObjects16    (GDI.71)
 */
INT16 WINAPI EnumObjects16( HDC16 hdc, INT16 nObjType,
                            GOBJENUMPROC16 lpEnumFunc, LPARAM lParam )
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

    TRACE("%04x %d %08lx %08lx\n",
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
            TRACE("solid pen %08lx, ret=%d\n",
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
            TRACE("solid brush %08lx, ret=%d\n",
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
            TRACE("hatched brush %d, ret=%d\n",
                         i, retval);
            if (!retval) break;
        }
        SEGPTR_FREE(brush);
        break;

    default:
        WARN("(%d): Invalid type\n", nObjType );
        break;
    }
    return retval;
}


/***********************************************************************
 *           EnumObjects32    (GDI32.89)
 */
INT WINAPI EnumObjects( HDC hdc, INT nObjType,
                            GOBJENUMPROC lpEnumFunc, LPARAM lParam )
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
    
    INT i, retval = 0;
    LOGPEN pen;
    LOGBRUSH brush;

    TRACE("%04x %d %08lx %08lx\n",
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
            TRACE("solid pen %08lx, ret=%d\n",
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
            TRACE("solid brush %08lx, ret=%d\n",
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
            TRACE("hatched brush %d, ret=%d\n",
                         i, retval);
            if (!retval) break;
        }
        break;

    default:
        /* FIXME: implement Win32 types */
        WARN("(%d): Invalid type\n", nObjType );
        break;
    }
    return retval;
}


/***********************************************************************
 *           IsGDIObject    (GDI.462)
 * 
 * returns type of object if valid (W95 system programming secrets p. 264-5)
 */
BOOL16 WINAPI IsGDIObject16( HGDIOBJ16 handle )
{
    UINT16 magic = 0;

    if (handle >= FIRST_STOCK_HANDLE ) 
    {
        switch (handle)
        {
        case STOCK_WHITE_BRUSH:
        case STOCK_LTGRAY_BRUSH:
        case STOCK_GRAY_BRUSH:
        case STOCK_DKGRAY_BRUSH:
        case STOCK_BLACK_BRUSH:
        case STOCK_HOLLOW_BRUSH:
            magic = BRUSH_MAGIC;
            break;

        case STOCK_WHITE_PEN:
        case STOCK_BLACK_PEN:
        case STOCK_NULL_PEN :
            magic = PEN_MAGIC;
            break;

        case STOCK_OEM_FIXED_FONT:
        case STOCK_ANSI_FIXED_FONT:
        case STOCK_ANSI_VAR_FONT:
        case STOCK_SYSTEM_FONT:
        case STOCK_DEVICE_DEFAULT_FONT:
        case STOCK_SYSTEM_FIXED_FONT:
        case STOCK_DEFAULT_GUI_FONT:
            magic = FONT_MAGIC;
            break;

        case STOCK_DEFAULT_PALETTE:
            magic = PALETTE_MAGIC;
            break;
        }
    }
    else
    {
	GDIOBJHDR *object = (GDIOBJHDR *) GDI_HEAP_LOCK( handle );
	if (object)
	{
	    magic = object->wMagic;
	    GDI_HEAP_UNLOCK( handle );
	}
    }

    if (magic >= PEN_MAGIC && magic <= METAFILE_DC_MAGIC)
        return magic - PEN_MAGIC + 1;
    else
        return FALSE;
}


/***********************************************************************
 *           SetObjectOwner16    (GDI.461)
 */
void WINAPI SetObjectOwner16( HGDIOBJ16 handle, HANDLE16 owner )
{
    /* Nothing to do */
}


/***********************************************************************
 *           SetObjectOwner32    (GDI32.386)
 */
void WINAPI SetObjectOwner( HGDIOBJ handle, HANDLE owner )
{
    /* Nothing to do */
}

/***********************************************************************
 *           MakeObjectPrivate    (GDI.463)
 */
void WINAPI MakeObjectPrivate16( HGDIOBJ16 handle, BOOL16 private )
{
    /* FIXME */
}


/***********************************************************************
 *           GdiFlush    (GDI32.128)
 */
BOOL WINAPI GdiFlush(void)
{
    return TRUE;  /* FIXME */
}


/***********************************************************************
 *           GdiGetBatchLimit    (GDI32.129)
 */
DWORD WINAPI GdiGetBatchLimit(void)
{
    return 1;  /* FIXME */
}


/***********************************************************************
 *           GdiSetBatchLimit    (GDI32.139)
 */
DWORD WINAPI GdiSetBatchLimit( DWORD limit )
{
    return 1; /* FIXME */
}


/***********************************************************************
 *           GdiSeeGdiDo   (GDI.452)
 */
DWORD WINAPI GdiSeeGdiDo16( WORD wReqType, WORD wParam1, WORD wParam2,
                          WORD wParam3 )
{
    switch (wReqType)
    {
    case 0x0001:  /* LocalAlloc */
        return LOCAL_Alloc( GDI_HeapSel, wParam1, wParam3 );
    case 0x0002:  /* LocalFree */
        return LOCAL_Free( GDI_HeapSel, wParam1 );
    case 0x0003:  /* LocalCompact */
        return LOCAL_Compact( GDI_HeapSel, wParam3, 0 );
    case 0x0103:  /* LocalHeap */
        return GDI_HeapSel;
    default:
        WARN("(wReqType=%04x): Unknown\n", wReqType);
        return (DWORD)-1;
    }
}

/***********************************************************************
 *           GdiSignalProc     (GDI.610)
 */
WORD WINAPI GdiSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                           DWORD dwFlags, HMODULE16 hModule )
{
    return 0;
}

/***********************************************************************
 *           FinalGdiInit16     (GDI.405)
 */
void WINAPI FinalGdiInit16( HANDLE16 unknown )
{
}

/***********************************************************************
 *           GdiFreeResources   (GDI.609)
 */
WORD WINAPI GdiFreeResources16( DWORD reserve )
{
   return (WORD)( (int)LOCAL_CountFree( GDI_HeapSel ) * 100 /
                  (int)LOCAL_HeapSize( GDI_HeapSel ) );
}

/***********************************************************************
 *           MulDiv16   (GDI.128)
 */
INT16 WINAPI MulDiv16( INT16 foo, INT16 bar, INT16 baz )
{
    INT ret;
    if (!baz) return -32768;
    ret = (foo * bar) / baz;
    if ((ret > 32767) || (ret < -32767)) return -32768;
    return ret;
}


/***********************************************************************
 *           MulDiv32   (KERNEL32.391)
 * RETURNS
 *	Result of multiplication and division
 *	-1: Overflow occurred or Divisor was 0
 */
INT WINAPI MulDiv(
	     INT nMultiplicand, 
	     INT nMultiplier,
	     INT nDivisor
) {
#if SIZEOF_LONG_LONG >= 8
    long long ret;

    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
	 ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      ret = (((long long)nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
    else
      ret = (((long long)nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
#else
    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
	 ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      return ((nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
 
    return ((nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;
    
#endif
}
/*******************************************************************
 *      GetColorAdjustment [GDI32.164]
 *
 *
 */
BOOL WINAPI GetColorAdjustment(HDC hdc, LPCOLORADJUSTMENT lpca)
{
        FIXME("GetColorAdjustment, stub\n");
        return 0;
}

/*******************************************************************
 *      GetMiterLimit [GDI32.201]
 *
 *
 */
BOOL WINAPI GetMiterLimit(HDC hdc, PFLOAT peLimit)
{
        FIXME("GetMiterLimit, stub\n");
        return 0;
}

/*******************************************************************
 *      SetMiterLimit [GDI32.325]
 *
 *
 */
BOOL WINAPI SetMiterLimit(HDC hdc, FLOAT eNewLimit, PFLOAT peOldLimit)
{
        FIXME("SetMiterLimit, stub\n");
        return 0;
}

/*******************************************************************
 *      GdiComment [GDI32.109]
 *
 *
 */
BOOL WINAPI GdiComment(HDC hdc, UINT cbSize, const BYTE *lpData)
{
        FIXME("GdiComment, stub\n");
        return 0;
}
/*******************************************************************
 *      SetColorAdjustment [GDI32.309]
 *
 *
 */
BOOL WINAPI SetColorAdjustment(HDC hdc, const COLORADJUSTMENT* lpca)
{
        FIXME("SetColorAdjustment, stub\n");
        return 0;
}
 
