/*
 * GDI OEM bitmap objects
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include <string.h>
#include "ts_xlib.h"
#include "ts_xutil.h"
#include "ts_xpm.h"
#include "gdi.h"
#include "bitmap.h"
#include "callback.h"
#include "color.h"
#include "cursoricon.h"
#include "heap.h"
#include "tweak.h"
#include "debug.h"

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
#include "bitmaps/obm_radiocheck"

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
#include "bitmaps/obm_restore_95"
#include "bitmaps/obm_restored_95"


#define OBM_FIRST  OBM_RADIOCHECK  /* First OEM bitmap */
#define OBM_LAST   OBM_OLD_CLOSE   /* Last OEM bitmap */

static struct
{
    char** data;   /* Pointer to bitmap data */
    BOOL32 color;  /* Is it a color bitmap?  */
} OBM_Pixmaps_Data[OBM_LAST-OBM_FIRST+1] = {
    { obm_radiocheck, FALSE },	/* OBM_RADIOCHECK */
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
#include "bitmaps/oic_hand_95"
#include "bitmaps/oic_ques_95"
#include "bitmaps/oic_bang_95"
#include "bitmaps/oic_note_95"

#define OIC_FIRST  OIC_SAMPLE      /* First OEM icon */
#define OIC_LAST   OIC_WINEICON   /* Last OEM icon */

static char **OBM_Icons_Data[OIC_LAST-OIC_FIRST+1] =
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
#include "bitmaps/ocr_no"
#include "bitmaps/ocr_appstarting"
#include "bitmaps/ocr_help"

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

#define OCR_FIRST3 OCR_NO
#define OCR_LAST3  OCR_NO
#define OCR_BASE3       (OCR_BASE2 + OCR_LAST2 - OCR_FIRST2 + 1)

#define OCR_FIRST4 OCR_APPSTARTING
#define OCR_LAST4  OCR_APPSTARTING
#define OCR_BASE4       (OCR_BASE3 + OCR_LAST3 - OCR_FIRST3 + 1)

#define OCR_FIRST5 OCR_HELP
#define OCR_LAST5  OCR_HELP
#define OCR_BASE5       (OCR_BASE4 + OCR_LAST4 - OCR_FIRST4 + 1)

#define NB_CURSORS (OCR_BASE5 + OCR_LAST5 - OCR_FIRST5 + 1)
static char **OBM_Cursors_Data[NB_CURSORS] = 
{
    ocr_bummer,	     /* OCR_BUMMER */
    ocr_dragobject,  /* OCR_DRAGOBJECT */ 
    ocr_normal,      /* OCR_NORMAL */
    ocr_ibeam,       /* OCR_IBEAM */
    ocr_wait,        /* OCR_WAIT */
    ocr_cross,       /* OCR_CROSS */
    ocr_up,          /* OCR_UP */
    ocr_size,        /* OCR_SIZE */
    ocr_icon,        /* OCR_ICON */
    ocr_sizenwse,    /* OCR_SIZENWSE */
    ocr_sizenesw,    /* OCR_SIZENESW */
    ocr_sizewe,      /* OCR_SIZEWE */
    ocr_sizens,      /* OCR_SIZENS */
#if 0
    ocr_sizeall,     /* OCR_SIZEALL */
    ocr_icocur       /* OCR_ICOCUR */
#endif
    ocr_no,          /* OCR_NO */
    ocr_appstarting, /* OCR_APPSTARTING */
    ocr_help         /* OCR_HELP */
};

static HGLOBAL16 OBM_Cursors[NB_CURSORS];


  /* All the colors used in the xpm files must be included in this   */
  /* list, to make sure that the loaded bitmaps only use colors from */
  /* the Windows colormap. Note: the PALETTEINDEX() are not really   */
  /* palette indexes, but system colors that will be converted to    */
  /* indexes later on.                                               */

#if 0
static const struct
{
    char *   name;
    COLORREF color;
} OBM_SymbolicColors[] =
#endif
static XpmColorSymbol OBM_Colors[] =
{
    { "black",            NULL, (Pixel)RGB(0,0,0) },
    { "white",            NULL, (Pixel)RGB(255,255,255) },
    { "red",              NULL, (Pixel)RGB(255,0,0) },
    { "green",            NULL, (Pixel)RGB(0,255,0) },
    { "blue",             NULL, (Pixel)RGB(0,0,255) },
    { "yellow",           NULL, (Pixel)RGB(255,255,0) },
    { "cyan",             NULL, (Pixel)RGB(0,255,255) },    
    { "dkyellow",         NULL, (Pixel)RGB(128,128,0) },
    { "purple",           NULL, (Pixel)RGB(128,0,128) },
    { "ltgray",           NULL, (Pixel)RGB(192,192,192) },
    { "dkgray",           NULL, (Pixel)RGB(128,128,128) },
    { "foldercol",        NULL, (Pixel)RGB(0,191,191) },
    { "button_face",      NULL, (Pixel)PALETTEINDEX(COLOR_BTNFACE) },
    { "button_shadow",    NULL, (Pixel)PALETTEINDEX(COLOR_BTNSHADOW) },
    { "button_highlight", NULL, (Pixel)PALETTEINDEX(COLOR_BTNHIGHLIGHT) },
    { "button_edge",      NULL, (Pixel)PALETTEINDEX(COLOR_BTNHIGHLIGHT) },
    { "button_text",      NULL, (Pixel)PALETTEINDEX(COLOR_BTNTEXT) },
    { "window_frame",     NULL, (Pixel)PALETTEINDEX(COLOR_WINDOWFRAME) }
};

#define NB_COLOR_SYMBOLS (sizeof(OBM_Colors)/sizeof(OBM_Colors[0]))

  /* These are the symbolic colors for monochrome bitmaps   */
  /* This is needed to make sure that black is always 0 and */
  /* white always 1, as required by Windows.                */

static XpmColorSymbol OBM_BlackAndWhite[2] =
{
    { "black", NULL, 0 },
    { "white", NULL, 0xffffffff }
};

/* This structure holds the arguments for OBM_CreateBitmaps() */
typedef struct
{
    char     **data;      /* In: bitmap data */
    BOOL32     color;     /* In: color or monochrome */
    BOOL32     need_mask; /* In: do we need a mask? */
    HBITMAP16  bitmap;    /* Out: resulting bitmap */
    HBITMAP16  mask;      /* Out: resulting mask (if needed) */
    POINT32    hotspot;   /* Out: bitmap hotspot */
} OBM_BITMAP_DESCR;


/***********************************************************************
 *           OBM_InitColorSymbols
 */
static BOOL32 OBM_InitColorSymbols()
{
    static BOOL32 initialized = FALSE;
    int i;

    if (initialized) return TRUE;  /* Already initialised */
    initialized = TRUE;

    for (i = 0; i < NB_COLOR_SYMBOLS; i++)
    {
        if (OBM_Colors[i].pixel & 0xff000000)  /* PALETTEINDEX */
            OBM_Colors[i].pixel = COLOR_ToPhysical( NULL,
                                    GetSysColor32(OBM_Colors[i].pixel & 0xff));
        else  /* RGB*/
            OBM_Colors[i].pixel = COLOR_ToPhysical( NULL, OBM_Colors[i].pixel);
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

    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_LOCK( hbitmap );
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
    GDI_HEAP_UNLOCK( hbitmap );
    return hbitmap;
}


/***********************************************************************
 *           OBM_CreateBitmaps
 *
 * Create the 2 bitmaps from XPM data.
 *
 * The Xlib critical section must be entered before calling this function.
 */
static BOOL32 OBM_CreateBitmaps( OBM_BITMAP_DESCR *descr )
{
    Pixmap pixmap, pixmask;
    XpmAttributes *attrs;
    int err;

    attrs = (XpmAttributes *)HEAP_xalloc( GetProcessHeap(), 0,
                                          XpmAttributesSize() );
    attrs->valuemask    = XpmColormap | XpmDepth | XpmColorSymbols |XpmHotspot;
    attrs->colormap     = COLOR_GetColormap();
    attrs->depth        = descr->color ? screenDepth : 1;
    attrs->colorsymbols = (attrs->depth > 1) ? OBM_Colors : OBM_BlackAndWhite;
    attrs->numsymbols   = (attrs->depth > 1) ? NB_COLOR_SYMBOLS : 2;
        
    err = XpmCreatePixmapFromData( display, rootWindow, descr->data,
                                   &pixmap, &pixmask, attrs );

    if (err != XpmSuccess)
    {
        HeapFree( GetProcessHeap(), 0, attrs );
        return FALSE;
    }
    descr->hotspot.x = attrs->x_hotspot;
    descr->hotspot.y = attrs->y_hotspot;
    descr->bitmap = OBM_MakeBitmap( attrs->width, attrs->height,
                                    attrs->depth, pixmap );
    if (descr->need_mask)
        descr->mask = OBM_MakeBitmap( attrs->width, attrs->height,
                                      1, pixmask );
    HeapFree( GetProcessHeap(), 0, attrs );
    if (!descr->bitmap)
    {
        if (pixmap) XFreePixmap( display, pixmap );
        if (pixmask) XFreePixmap( display, pixmask );
        if (descr->bitmap) GDI_FreeObject( descr->bitmap );
        if (descr->need_mask && descr->mask) GDI_FreeObject( descr->mask );
        return FALSE;
    }
    else return TRUE;
}


/***********************************************************************
 *           OBM_LoadBitmap
 */
HBITMAP16 OBM_LoadBitmap( WORD id )
{
    OBM_BITMAP_DESCR descr;

    if ((id < OBM_FIRST) || (id > OBM_LAST)) return 0;
    id -= OBM_FIRST;

    if (!OBM_InitColorSymbols()) return 0;

    descr.data      = OBM_Pixmaps_Data[id].data;
    descr.color     = OBM_Pixmaps_Data[id].color;
    descr.need_mask = FALSE;

    EnterCriticalSection( &X11DRV_CritSection );
    if (!CALL_LARGE_STACK( OBM_CreateBitmaps, &descr ))
    {
        LeaveCriticalSection( &X11DRV_CritSection );
        fprintf( stderr, "Error creating OEM bitmap %d\n", OBM_FIRST+id );
        return 0;
    }
    LeaveCriticalSection( &X11DRV_CritSection );
    return descr.bitmap;
}


/***********************************************************************
 *           OBM_LoadCursorIcon
 */
HGLOBAL16 OBM_LoadCursorIcon( WORD id, BOOL32 fCursor )
{
    OBM_BITMAP_DESCR descr;
    HGLOBAL16 handle;
    CURSORICONINFO *pInfo;
    BITMAPOBJ *bmpXor, *bmpAnd;
    int sizeXor, sizeAnd;

    if (fCursor)
    {
        if ((id >= OCR_FIRST1) && (id <= OCR_LAST1))
             id = OCR_BASE1 + id - OCR_FIRST1;
        else if ((id >= OCR_FIRST2) && (id <= OCR_LAST2))
                  id = OCR_BASE2 + id - OCR_FIRST2;
	     else if ((id >= OCR_FIRST0) && (id <= OCR_LAST0))
		       id = OCR_BASE0 + id - OCR_FIRST0;
		  else if ((id >= OCR_FIRST3) && (id <= OCR_LAST3))
			    id = OCR_BASE3 + id - OCR_FIRST3;
		       else if ((id >= OCR_FIRST4) && (id <= OCR_LAST4))
				 id = OCR_BASE4 + id - OCR_FIRST4;
			    else if ((id >= OCR_FIRST5) && (id <= OCR_LAST5))
				      id = OCR_BASE5 + id - OCR_FIRST5;
        else return 0;
        if (OBM_Cursors[id]) return OBM_Cursors[id];
    }
    else
    {
        if ((id < OIC_FIRST) || (id > OIC_LAST)) return 0;
        id -= OIC_FIRST;
    }

    if (!OBM_InitColorSymbols()) return 0;
    
    descr.data      = fCursor ? OBM_Cursors_Data[id] : OBM_Icons_Data[id];
    descr.color     = !fCursor;
    descr.need_mask = TRUE;

    EnterCriticalSection( &X11DRV_CritSection );
    if (!CALL_LARGE_STACK( OBM_CreateBitmaps, &descr ))
    {
        LeaveCriticalSection( &X11DRV_CritSection );
        fprintf( stderr, "Error creating OEM cursor/icon %d\n", id );
        return 0;
    }
    LeaveCriticalSection( &X11DRV_CritSection );

    bmpXor = (BITMAPOBJ *) GDI_GetObjPtr( descr.bitmap, BITMAP_MAGIC );
    bmpAnd = (BITMAPOBJ *) GDI_GetObjPtr( descr.mask, BITMAP_MAGIC );
    sizeXor = bmpXor->bitmap.bmHeight * bmpXor->bitmap.bmWidthBytes;
    sizeAnd = bmpXor->bitmap.bmHeight * BITMAP_WIDTH_BYTES( bmpXor->bitmap.bmWidth, 1 );

    if (!(handle = GlobalAlloc16( GMEM_MOVEABLE,
                                  sizeof(CURSORICONINFO) + sizeXor + sizeAnd)))
    {
        DeleteObject32( descr.bitmap );
        DeleteObject32( descr.mask );
        return 0;
    }

    pInfo = (CURSORICONINFO *)GlobalLock16( handle );
    pInfo->ptHotSpot.x   = descr.hotspot.x;
    pInfo->ptHotSpot.y   = descr.hotspot.y;
    pInfo->nWidth        = bmpXor->bitmap.bmWidth;
    pInfo->nHeight       = bmpXor->bitmap.bmHeight;
    pInfo->nWidthBytes   = bmpXor->bitmap.bmWidthBytes;
    pInfo->bPlanes       = bmpXor->bitmap.bmPlanes;
    pInfo->bBitsPerPixel = bmpXor->bitmap.bmBitsPixel;

    if (descr.mask)
    {
          /* Invert the mask */

        TSXSetFunction( display, BITMAP_monoGC, GXinvert );
        TSXFillRectangle( display, bmpAnd->pixmap, BITMAP_monoGC, 0, 0,
                        bmpAnd->bitmap.bmWidth, bmpAnd->bitmap.bmHeight );
        TSXSetFunction( display, BITMAP_monoGC, GXcopy );

          /* Set the masked pixels to black */

        if (bmpXor->bitmap.bmBitsPixel != 1)
        {
            TSXSetForeground( display, BITMAP_colorGC,
                            COLOR_ToPhysical( NULL, RGB(0,0,0) ));
            TSXSetBackground( display, BITMAP_colorGC, 0 );
            TSXSetFunction( display, BITMAP_colorGC, GXor );
            TSXCopyPlane(display, bmpAnd->pixmap, bmpXor->pixmap, BITMAP_colorGC,
                       0, 0, bmpXor->bitmap.bmWidth, bmpXor->bitmap.bmHeight,
                       0, 0, 1 );
            TSXSetFunction( display, BITMAP_colorGC, GXcopy );
        }
    }

    if (descr.mask) GetBitmapBits32( descr.mask, sizeAnd, (char *)(pInfo + 1));
    else memset( (char *)(pInfo + 1), 0xff, sizeAnd );
    GetBitmapBits32( descr.bitmap, sizeXor, (char *)(pInfo + 1) + sizeAnd );

    DeleteObject32( descr.bitmap );
    DeleteObject32( descr.mask );

    if (fCursor) OBM_Cursors[id] = handle;
    return handle;
}


/***********************************************************************
 *           OBM_Init
 *
 * Initializes the OBM_Pixmaps_Data and OBM_Icons_Data struct
 */
BOOL32 OBM_Init()
{
    if(TWEAK_Win95Look) {
	OBM_Pixmaps_Data[OBM_ZOOMD - OBM_FIRST].data = obm_zoomd_95;
	OBM_Pixmaps_Data[OBM_REDUCED - OBM_FIRST].data = obm_reduced_95;
	OBM_Pixmaps_Data[OBM_ZOOM - OBM_FIRST].data = obm_zoom_95;
	OBM_Pixmaps_Data[OBM_REDUCE - OBM_FIRST].data = obm_reduce_95;
	OBM_Pixmaps_Data[OBM_CLOSE - OBM_FIRST].data = obm_close_95;
        OBM_Pixmaps_Data[OBM_RESTORE - OBM_FIRST].data = obm_restore_95;
        OBM_Pixmaps_Data[OBM_RESTORED - OBM_FIRST].data = obm_restored_95;

        OBM_Icons_Data[OIC_HAND - OIC_FIRST] = oic_hand_95;
        OBM_Icons_Data[OIC_QUES - OIC_FIRST] = oic_ques_95;
        OBM_Icons_Data[OIC_BANG - OIC_FIRST] = oic_bang_95;
        OBM_Icons_Data[OIC_NOTE - OIC_FIRST] = oic_note_95;
    }
    else {
	OBM_Pixmaps_Data[OBM_ZOOMD - OBM_FIRST].data = obm_zoomd;
	OBM_Pixmaps_Data[OBM_REDUCED - OBM_FIRST].data = obm_reduced;
	OBM_Pixmaps_Data[OBM_ZOOM - OBM_FIRST].data = obm_zoom;
	OBM_Pixmaps_Data[OBM_REDUCE - OBM_FIRST].data = obm_reduce;
	OBM_Pixmaps_Data[OBM_CLOSE - OBM_FIRST].data = obm_close;
        OBM_Pixmaps_Data[OBM_RESTORE - OBM_FIRST].data = obm_restore;
        OBM_Pixmaps_Data[OBM_RESTORED - OBM_FIRST].data = obm_restored;

        OBM_Icons_Data[OIC_HAND - OIC_FIRST] = oic_hand;
        OBM_Icons_Data[OIC_QUES - OIC_FIRST] = oic_ques;
        OBM_Icons_Data[OIC_BANG - OIC_FIRST] = oic_bang;
        OBM_Icons_Data[OIC_NOTE - OIC_FIRST] = oic_note;
    }

    return 1;
}
