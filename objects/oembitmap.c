/*
 * GDI OEM bitmap objects
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include "gdi.h"
#include "bitmap.h"
#include "callback.h"
#include "color.h"
#include "cursoricon.h"
#include "stddebug.h"
#include "tweak.h"
#include "debug.h"
#include "xmalloc.h"

  /* Include OEM pixmaps */
#include "bitmaps/obm_cdrom"
#include "bitmaps/obm_harddisk"
#include "bitmaps/obm_drive"
#include "bitmaps/obm_folder2"
#include "bitmaps/obm_folder"
#include "bitmaps/obm_lfarrowi"
#include "bitmaps/obm_rgarrowi"
#include "bitmaps/obm_dnarrowi"
#include "bitmaps/obm_uparrowi"
#include "bitmaps/obm_combo"
#include "bitmaps/obm_mnarrow"
#include "bitmaps/obm_lfarrowd"
#include "bitmaps/obm_rgarrowd"
#include "bitmaps/obm_dnarrowd"
#include "bitmaps/obm_uparrowd"
#include "bitmaps/obm_restored"
#include "bitmaps/obm_restore"
#include "bitmaps/obm_lfarrow"
#include "bitmaps/obm_rgarrow"
#include "bitmaps/obm_dnarrow"
#include "bitmaps/obm_uparrow"
#include "bitmaps/obm_old_restore"
#include "bitmaps/obm_old_zoom"
#include "bitmaps/obm_old_reduce"
#include "bitmaps/obm_btncorners"
#include "bitmaps/obm_checkboxes"
#include "bitmaps/obm_check"
#include "bitmaps/obm_btsize"
#include "bitmaps/obm_old_lfarrow"
#include "bitmaps/obm_old_rgarrow"
#include "bitmaps/obm_old_dnarrow"
#include "bitmaps/obm_old_uparrow"
#include "bitmaps/obm_size"
#include "bitmaps/obm_old_close"
#include "bitmaps/obm_trtype"

#include "bitmaps/obm_zoomd"
#include "bitmaps/obm_reduced"
#include "bitmaps/obm_zoom"
#include "bitmaps/obm_reduce"
#include "bitmaps/obm_close"
#include "bitmaps/obm_zoomd_95"
#include "bitmaps/obm_reduced_95"
#include "bitmaps/obm_zoom_95"
#include "bitmaps/obm_reduce_95"
#include "bitmaps/obm_close_95"
#include "bitmaps/obm_closed_95"


#define OBM_FIRST  OBM_TRTYPE	   /* First OEM bitmap */
#define OBM_LAST   OBM_OLD_CLOSE   /* Last OEM bitmap */

static struct
{
    char** data;   /* Pointer to bitmap data */
    BOOL32 color;  /* Is it a color bitmap?  */
} OBM_Pixmaps_Data[OBM_LAST-OBM_FIRST+1] = {
    { obm_trtype, TRUE },	/* OBM_TRTYPE */    
    { obm_cdrom, TRUE },        /* OBM_CDROM    */
    { obm_harddisk, TRUE },     /* OBM_HARDDISK */
    { obm_drive, TRUE },        /* OBM_DRIVE    */
    { obm_folder2, TRUE },      /* OBM_FOLDER2  */
    { obm_folder, TRUE },       /* OBM_FOLDER   */
    { obm_lfarrowi, TRUE },     /* OBM_LFARROWI */
    { obm_rgarrowi, TRUE },     /* OBM_RGARROWI */
    { obm_dnarrowi, TRUE },     /* OBM_DNARROWI */
    { obm_uparrowi, TRUE },     /* OBM_UPARROWI */
    { obm_combo, FALSE },       /* OBM_COMBO */
    { obm_mnarrow, FALSE },     /* OBM_MNARROW */
    { obm_lfarrowd, TRUE },     /* OBM_LFARROWD */
    { obm_rgarrowd, TRUE },     /* OBM_RGARROWD */
    { obm_dnarrowd, TRUE },     /* OBM_DNARROWD */
    { obm_uparrowd, TRUE },     /* OBM_UPARROWD */
    { obm_restored, TRUE },     /* OBM_RESTORED */
    { obm_zoomd, TRUE },        /* OBM_ZOOMD */
    { obm_reduced, TRUE },      /* OBM_REDUCED */
    { obm_restore, TRUE },      /* OBM_RESTORE */
    { obm_zoom, TRUE },         /* OBM_ZOOM */
    { obm_reduce, TRUE },       /* OBM_REDUCE */
    { obm_lfarrow, TRUE },      /* OBM_LFARROW */
    { obm_rgarrow, TRUE },      /* OBM_RGARROW */
    { obm_dnarrow, TRUE },      /* OBM_DNARROW */
    { obm_uparrow, TRUE },      /* OBM_UPARROW */
    { obm_close, TRUE },        /* OBM_CLOSE */
    { obm_old_restore, FALSE }, /* OBM_OLD_RESTORE */
    { obm_old_zoom, FALSE },    /* OBM_OLD_ZOOM */
    { obm_old_reduce, FALSE },  /* OBM_OLD_REDUCE */
    { obm_btncorners, FALSE },  /* OBM_BTNCORNERS */
    { obm_checkboxes, FALSE },  /* OBM_CHECKBOXES */
    { obm_check, FALSE },       /* OBM_CHECK */
    { obm_btsize, FALSE },      /* OBM_BTSIZE */
    { obm_old_lfarrow, FALSE }, /* OBM_OLD_LFARROW */
    { obm_old_rgarrow, FALSE }, /* OBM_OLD_RGARROW */
    { obm_old_dnarrow, FALSE }, /* OBM_OLD_DNARROW */
    { obm_old_uparrow, FALSE }, /* OBM_OLD_UPARROW */
    { obm_size, FALSE },        /* OBM_SIZE */
    { obm_old_close, FALSE },   /* OBM_OLD_CLOSE */
};


  /* Include OEM icons */
#include "bitmaps/oic_sample"
#include "bitmaps/oic_hand"
#include "bitmaps/oic_ques"
#include "bitmaps/oic_bang"
#include "bitmaps/oic_note"
#include "bitmaps/oic_portrait"
#include "bitmaps/oic_landscape"
#include "bitmaps/oic_wineicon"

#define OIC_FIRST  OIC_SAMPLE      /* First OEM icon */
#define OIC_LAST   OIC_WINEICON   /* Last OEM icon */

static char ** const OBM_Icons_Data[OIC_LAST-OIC_FIRST+1] =
{
    oic_sample,    /* OIC_SAMPLE */
    oic_hand,      /* OIC_HAND */
    oic_ques,      /* OIC_QUES */
    oic_bang,      /* OIC_BANG */
    oic_note,      /* OIC_NOTE */
    oic_portrait,  /* OIC_PORTRAIT */
    oic_landscape, /* OIC_LANDSCAPE */
    oic_wineicon   /* OIC_WINEICON */
};


  /* Include OEM cursors */
#include "bitmaps/ocr_normal"
#include "bitmaps/ocr_ibeam"
#include "bitmaps/ocr_wait"
#include "bitmaps/ocr_cross"
#include "bitmaps/ocr_up"
#include "bitmaps/ocr_size"
#include "bitmaps/ocr_icon"
#include "bitmaps/ocr_sizenwse"
#include "bitmaps/ocr_sizenesw"
#include "bitmaps/ocr_sizewe"
#include "bitmaps/ocr_sizens"
#include "bitmaps/ocr_bummer"
#include "bitmaps/ocr_dragobject"
/*#include "bitmaps/ocr_sizeall"*/
/*#include "bitmaps/ocr_icocur"*/

/* Cursor are not all contiguous (go figure...) */
#define OCR_FIRST0 OCR_BUMMER
#define OCR_LAST0  OCR_DRAGOBJECT
#define OCR_BASE0	0

#define OCR_FIRST1 OCR_NORMAL
#define OCR_LAST1  OCR_UP
#define OCR_BASE1	(OCR_BASE0 + OCR_LAST0 - OCR_FIRST0 + 1)

#define OCR_FIRST2 OCR_SIZE
#define OCR_LAST2  OCR_SIZENS 
#define OCR_BASE2	(OCR_BASE1 + OCR_LAST1 - OCR_FIRST1 + 1)

#define NB_CURSORS (OCR_BASE2 + OCR_LAST2 - OCR_FIRST2 + 1)
static char **OBM_Cursors_Data[NB_CURSORS] = 
{
    ocr_bummer,	   /* OCR_BUMMER */
    ocr_dragobject,/* OCR_DRAGOBJECT */ 
    ocr_normal,    /* OCR_NORMAL */
    ocr_ibeam,     /* OCR_IBEAM */
    ocr_wait,      /* OCR_WAIT */
    ocr_cross,     /* OCR_CROSS */
    ocr_up,        /* OCR_UP */
    ocr_size,      /* OCR_SIZE */
    ocr_icon,      /* OCR_ICON */
    ocr_sizenwse,  /* OCR_SIZENWSE */
    ocr_sizenesw,  /* OCR_SIZENESW */
    ocr_sizewe,    /* OCR_SIZEWE */
    ocr_sizens     /* OCR_SIZENS */
#if 0
    ocr_sizeall,   /* OCR_SIZEALL */
    ocr_icocur     /* OCR_ICOCUR */
#endif
};

static HGLOBAL16 OBM_Cursors[NB_CURSORS];


  /* All the colors used in the xpm files must be included in this   */
  /* list, to make sure that the loaded bitmaps only use colors from */
  /* the Windows colormap. Note: the PALETTEINDEX() are not really   */
  /* palette indexes, but system colors that will be converted to    */
  /* indexes later on.                                               */

static const struct
{
    char *   name;
    COLORREF color;
} OBM_SymbolicColors[] =
{
    { "black",            RGB(0,0,0) },
    { "white",            RGB(255,255,255) },
    { "red",              RGB(255,0,0) },
    { "green",            RGB(0,255,0) },
    { "blue",             RGB(0,0,255) },
    { "yellow",           RGB(255,255,0) },
    { "cyan",             RGB(0,255,255) },    
    { "dkyellow",         RGB(128,128,0) },
    { "purple",           RGB(128,0,128) },
    { "ltgray",           RGB(192,192,192) },
    { "dkgray",           RGB(128,128,128) },
    { "foldercol",        RGB(0,191,191) },
    { "button_face",      PALETTEINDEX(COLOR_BTNFACE) },
    { "button_shadow",    PALETTEINDEX(COLOR_BTNSHADOW) },
    { "button_highlight", PALETTEINDEX(COLOR_BTNHIGHLIGHT) },
    { "button_edge",      PALETTEINDEX(COLOR_BTNHIGHLIGHT) },
    { "button_text",      PALETTEINDEX(COLOR_BTNTEXT) },
    { "window_frame",     PALETTEINDEX(COLOR_WINDOWFRAME) }
};

#define NB_COLOR_SYMBOLS \
            (sizeof(OBM_SymbolicColors)/sizeof(OBM_SymbolicColors[0]))

  /* These are the symbolic colors for monochrome bitmaps   */
  /* This is needed to make sure that black is always 0 and */
  /* white always 1, as required by Windows.                */

static XpmColorSymbol OBM_BlackAndWhite[2] =
{
    { "black", NULL, 0 },
    { "white", NULL, 0xffffffff }
};

static XpmColorSymbol *OBM_Colors = NULL;


/***********************************************************************
 *           OBM_InitColorSymbols
 */
static BOOL32 OBM_InitColorSymbols()
{
    int i;

    if (OBM_Colors) return TRUE;  /* Already initialised */

    OBM_Colors = (XpmColorSymbol *) malloc( sizeof(XpmColorSymbol) *
                                            NB_COLOR_SYMBOLS );
    if (!OBM_Colors) return FALSE;
    for (i = 0; i < NB_COLOR_SYMBOLS; i++)
    {
        OBM_Colors[i].name  = OBM_SymbolicColors[i].name;
        OBM_Colors[i].value = NULL;
        if (OBM_SymbolicColors[i].color & 0xff000000)  /* PALETTEINDEX */
            OBM_Colors[i].pixel = COLOR_ToPhysical( NULL,
                            GetSysColor32(OBM_SymbolicColors[i].color & 0xff));
        else  /* RGB*/
            OBM_Colors[i].pixel = COLOR_ToPhysical( NULL,
                                                 OBM_SymbolicColors[i].color );
    }
    return TRUE;
}


/***********************************************************************
 *           OBM_MakeBitmap
 *
 * Allocate a GDI bitmap.
 */
static HBITMAP16 OBM_MakeBitmap( WORD width, WORD height,
                                 WORD bpp, Pixmap pixmap )
{
    HBITMAP16 hbitmap;
    BITMAPOBJ * bmpObjPtr;

    if (!pixmap) return 0;

    hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
    if (!hbitmap) return 0;

    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_LIN_ADDR( hbitmap );
    bmpObjPtr->size.cx = width;
    bmpObjPtr->size.cy = height;
    bmpObjPtr->pixmap  = pixmap;
    bmpObjPtr->bitmap.bmType       = 0;
    bmpObjPtr->bitmap.bmWidth      = width;
    bmpObjPtr->bitmap.bmHeight     = height;
    bmpObjPtr->bitmap.bmWidthBytes = BITMAP_WIDTH_BYTES( width, bpp );
    bmpObjPtr->bitmap.bmPlanes     = 1;
    bmpObjPtr->bitmap.bmBitsPixel  = bpp;
    bmpObjPtr->bitmap.bmBits       = NULL;
    return hbitmap;
}


/***********************************************************************
 *           OBM_CreateBitmaps
 *
 * Create the 2 bitmaps from XPM data.
 */
static BOOL32 OBM_CreateBitmaps( char **data, BOOL32 color, HBITMAP16 *hBitmap,
                                 HBITMAP16 *hBitmapMask, POINT32 *hotspot )
{
    Pixmap pixmap, pixmask;
    XpmAttributes *attrs;
    int err;

    attrs = (XpmAttributes *)xmalloc( XpmAttributesSize() );
    attrs->valuemask    = XpmColormap | XpmDepth | XpmColorSymbols |XpmHotspot;
    attrs->colormap     = COLOR_GetColormap();
    attrs->depth        = color ? screenDepth : 1;
    attrs->colorsymbols = (attrs->depth > 1) ? OBM_Colors : OBM_BlackAndWhite;
    attrs->numsymbols   = (attrs->depth > 1) ? NB_COLOR_SYMBOLS : 2;
        
    err = XpmCreatePixmapFromData( display, rootWindow, data,
                                   &pixmap, &pixmask, attrs );

    if (err != XpmSuccess)
    {
        free( attrs );
        return FALSE;
    }
    if (hotspot)
    {
        hotspot->x = attrs->x_hotspot;
        hotspot->y = attrs->y_hotspot;
    }
    *hBitmap = OBM_MakeBitmap( attrs->width, attrs->height,
                               attrs->depth, pixmap );
    if (hBitmapMask) *hBitmapMask = OBM_MakeBitmap(attrs->width, attrs->height,
                                                   1, pixmask );
    free( attrs );
    if (!*hBitmap)
    {
        if (pixmap) XFreePixmap( display, pixmap );
        if (pixmask) XFreePixmap( display, pixmask );
        if (*hBitmap) GDI_FreeObject( *hBitmap );
        if (hBitmapMask && *hBitmapMask) GDI_FreeObject( *hBitmapMask );
        return FALSE;
    }
    else return TRUE;
}


/***********************************************************************
 *           OBM_LoadBitmap
 */
HBITMAP16 OBM_LoadBitmap( WORD id )
{
    HBITMAP16 hbitmap;

    if ((id < OBM_FIRST) || (id > OBM_LAST)) return 0;
    id -= OBM_FIRST;

    if (!OBM_InitColorSymbols()) return 0;
    
    if (!CallTo32_LargeStack( (int(*)())OBM_CreateBitmaps, 5,
                              OBM_Pixmaps_Data[id].data,
                              OBM_Pixmaps_Data[id].color,
                              &hbitmap, NULL, NULL ))
    {
        fprintf( stderr, "Error creating OEM bitmap %d\n", OBM_FIRST+id );
        return 0;
    }
    return hbitmap;
}


/***********************************************************************
 *           OBM_LoadCursorIcon
 */
HGLOBAL16 OBM_LoadCursorIcon( WORD id, BOOL32 fCursor )
{
    HGLOBAL16 handle;
    CURSORICONINFO *pInfo;
    BITMAPOBJ *bmpXor, *bmpAnd;
    HBITMAP16 hXorBits, hAndBits;
    POINT32 hotspot;
    int sizeXor, sizeAnd;

    if (fCursor)
    {
        if ((id >= OCR_FIRST1) && (id <= OCR_LAST1))
             id = OCR_BASE1 + id - OCR_FIRST1;
        else if ((id >= OCR_FIRST2) && (id <= OCR_LAST2))
                  id = OCR_BASE2 + id - OCR_FIRST2;
	     else if ((id >= OCR_FIRST0) && (id <= OCR_LAST0))
		       id = OCR_BASE0 + id - OCR_FIRST0;
        else return 0;
        if (OBM_Cursors[id]) return OBM_Cursors[id];
    }
    else
    {
        if ((id < OIC_FIRST) || (id > OIC_LAST)) return 0;
        id -= OIC_FIRST;
    }

    if (!OBM_InitColorSymbols()) return 0;
    
    if (!CallTo32_LargeStack( (int(*)())OBM_CreateBitmaps, 5,
                           fCursor ? OBM_Cursors_Data[id] : OBM_Icons_Data[id],
                           !fCursor, &hXorBits, &hAndBits, &hotspot ))
    {
        fprintf( stderr, "Error creating OEM cursor/icon %d\n", id );
        return 0;
    }

    bmpXor = (BITMAPOBJ *) GDI_GetObjPtr( hXorBits, BITMAP_MAGIC );
    bmpAnd = (BITMAPOBJ *) GDI_GetObjPtr( hAndBits, BITMAP_MAGIC );
    sizeXor = bmpXor->bitmap.bmHeight * bmpXor->bitmap.bmWidthBytes;
    sizeAnd = bmpXor->bitmap.bmHeight * BITMAP_WIDTH_BYTES( bmpXor->bitmap.bmWidth, 1 );

    if (!(handle = GlobalAlloc16( GMEM_MOVEABLE,
                                  sizeof(CURSORICONINFO) + sizeXor + sizeAnd)))
    {
        DeleteObject32( hXorBits );
        DeleteObject32( hAndBits );
        return 0;
    }

    pInfo = (CURSORICONINFO *)GlobalLock16( handle );
    pInfo->ptHotSpot.x   = hotspot.x;
    pInfo->ptHotSpot.y   = hotspot.y;
    pInfo->nWidth        = bmpXor->bitmap.bmWidth;
    pInfo->nHeight       = bmpXor->bitmap.bmHeight;
    pInfo->nWidthBytes   = bmpXor->bitmap.bmWidthBytes;
    pInfo->bPlanes       = bmpXor->bitmap.bmPlanes;
    pInfo->bBitsPerPixel = bmpXor->bitmap.bmBitsPixel;

    if (hAndBits)
    {
          /* Invert the mask */

        XSetFunction( display, BITMAP_monoGC, GXinvert );
        XFillRectangle( display, bmpAnd->pixmap, BITMAP_monoGC, 0, 0,
                        bmpAnd->bitmap.bmWidth, bmpAnd->bitmap.bmHeight );
        XSetFunction( display, BITMAP_monoGC, GXcopy );

          /* Set the masked pixels to black */

        if (bmpXor->bitmap.bmBitsPixel != 1)
        {
            XSetForeground( display, BITMAP_colorGC,
                            COLOR_ToPhysical( NULL, RGB(0,0,0) ));
            XSetBackground( display, BITMAP_colorGC, 0 );
            XSetFunction( display, BITMAP_colorGC, GXor );
            XCopyPlane(display, bmpAnd->pixmap, bmpXor->pixmap, BITMAP_colorGC,
                       0, 0, bmpXor->bitmap.bmWidth, bmpXor->bitmap.bmHeight,
                       0, 0, 1 );
            XSetFunction( display, BITMAP_colorGC, GXcopy );
        }
    }

    if (hAndBits) GetBitmapBits32( hAndBits, sizeAnd, (char *)(pInfo + 1) );
    else memset( (char *)(pInfo + 1), 0xff, sizeAnd );
    GetBitmapBits32( hXorBits, sizeXor, (char *)(pInfo + 1) + sizeAnd );

    DeleteObject32( hXorBits );
    DeleteObject32( hAndBits );

    if (fCursor) OBM_Cursors[id] = handle;
    return handle;
}


/***********************************************************************
 *           OBM_Init
 *
 * Initializes the OBM_Pixmaps_Data struct
 */
BOOL32 OBM_Init()
{
    if(TWEAK_Win95Look) {
	OBM_Pixmaps_Data[OBM_ZOOMD - OBM_FIRST].data = obm_zoomd_95;
	OBM_Pixmaps_Data[OBM_REDUCED - OBM_FIRST].data = obm_reduced_95;
	OBM_Pixmaps_Data[OBM_ZOOM - OBM_FIRST].data = obm_zoom_95;
	OBM_Pixmaps_Data[OBM_REDUCE - OBM_FIRST].data = obm_reduce_95;
	OBM_Pixmaps_Data[OBM_CLOSE - OBM_FIRST].data = obm_close_95;
    }
    else {
	OBM_Pixmaps_Data[OBM_ZOOMD - OBM_FIRST].data = obm_zoomd;
	OBM_Pixmaps_Data[OBM_REDUCED - OBM_FIRST].data = obm_reduced;
	OBM_Pixmaps_Data[OBM_ZOOM - OBM_FIRST].data = obm_zoom;
	OBM_Pixmaps_Data[OBM_REDUCE - OBM_FIRST].data = obm_reduce;
	OBM_Pixmaps_Data[OBM_CLOSE - OBM_FIRST].data = obm_close;
    }

    return 1;
}
