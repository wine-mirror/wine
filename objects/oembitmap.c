/*
 * GDI OEM bitmap objects
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *
 */

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include "gdi.h"
#include "bitmap.h"
#include "color.h"
#include "icon.h"
#include "stddebug.h"
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
#include "bitmaps/obm_zoomd"
#include "bitmaps/obm_reduced"
#include "bitmaps/obm_restore"
#include "bitmaps/obm_zoom"
#include "bitmaps/obm_reduce"
#include "bitmaps/obm_lfarrow"
#include "bitmaps/obm_rgarrow"
#include "bitmaps/obm_dnarrow"
#include "bitmaps/obm_uparrow"
#include "bitmaps/obm_close"
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

#define OBM_FIRST  OBM_CDROM      /* First OEM bitmap */
#define OBM_LAST   OBM_OLD_CLOSE   /* Last OEM bitmap */

static const struct
{
    char** data;   /* Pointer to bitmap data */
    BOOL   color;  /* Is it a color bitmap?  */
} OBM_Pixmaps_Data[OBM_LAST-OBM_FIRST+1] = {
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
      /* Black & white must always be the first 2 colors */
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
    { "button_text",      PALETTEINDEX(COLOR_BTNTEXT) },
    { "window_frame",     PALETTEINDEX(COLOR_WINDOWFRAME) }
};

#define NB_COLOR_SYMBOLS \
            (sizeof(OBM_SymbolicColors)/sizeof(OBM_SymbolicColors[0]))

static XpmColorSymbol *OBM_Colors = NULL;


/***********************************************************************
 *           OBM_InitColorSymbols
 */
static BOOL OBM_InitColorSymbols()
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
                              GetSysColor(OBM_SymbolicColors[i].color & 0xff));
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
static HBITMAP OBM_MakeBitmap( WORD width, WORD height,
                               WORD bpp, Pixmap pixmap )
{
    HBITMAP hbitmap;
    BITMAPOBJ * bmpObjPtr;

    if (!pixmap) return 0;

    hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC );
    if (!hbitmap) return 0;

    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_LIN_ADDR( hbitmap );
    bmpObjPtr->size.cx = 0;
    bmpObjPtr->size.cy = 0;
    bmpObjPtr->pixmap  = pixmap;
    bmpObjPtr->bitmap.bmType       = 0;
    bmpObjPtr->bitmap.bmWidth      = width;
    bmpObjPtr->bitmap.bmHeight     = height;
    bmpObjPtr->bitmap.bmWidthBytes = (width + 15) / 16 * 2;
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
static BOOL OBM_CreateBitmaps( char **data, BOOL color, BOOL mask,
                               HBITMAP *hBitmap, HBITMAP *hBitmapMask )
{
    Pixmap pixmap, pixmask;
    XpmAttributes attrs;
    int err;

    attrs.valuemask    = XpmColormap | XpmDepth | XpmColorSymbols;
    attrs.colormap     = COLOR_WinColormap;
    attrs.depth        = color ? screenDepth : 1;
    attrs.colorsymbols = OBM_Colors;
    attrs.numsymbols   = (attrs.depth > 1) ? NB_COLOR_SYMBOLS : 2;
        
    err = XpmCreatePixmapFromData( display, rootWindow, data,
                                   &pixmap, &pixmask, &attrs );

    if (err != XpmSuccess) return FALSE;
    *hBitmap = OBM_MakeBitmap( attrs.width, attrs.height,
                               attrs.depth, pixmap );
    if (mask) *hBitmapMask = OBM_MakeBitmap( attrs.width, attrs.height,
                                             1, pixmask );
    if (!*hBitmap)
    {
        if (pixmap) XFreePixmap( display, pixmap );
        if (pixmask) XFreePixmap( display, pixmask );
        if (*hBitmap) GDI_FreeObject( *hBitmap );
        if (*hBitmapMask) GDI_FreeObject( *hBitmapMask );
        return FALSE;
    }
    else return TRUE;
}


/***********************************************************************
 *           OBM_LoadBitmap
 */
HBITMAP OBM_LoadBitmap( WORD id )
{
    HBITMAP hbitmap, hbitmask;

    if ((id < OBM_FIRST) || (id > OBM_LAST)) return 0;
    id -= OBM_FIRST;

    if (!OBM_InitColorSymbols()) return 0;
    
    if (!OBM_CreateBitmaps( OBM_Pixmaps_Data[id].data,
                            OBM_Pixmaps_Data[id].color,
                            FALSE, &hbitmap, &hbitmask ))
    {
        fprintf( stderr, "Error creating OEM bitmap %d\n", OBM_FIRST+id );
        return 0;
    }
    return hbitmap;
}


/***********************************************************************
 *           OBM_LoadIcon
 */
HICON OBM_LoadIcon( WORD id )
{
    HICON hicon;
    ICONALLOC *pIcon;
    BITMAPOBJ *bmp;

    if ((id < OIC_FIRST) || (id > OIC_LAST)) return 0;
    id -= OIC_FIRST;

    if (!OBM_InitColorSymbols()) return 0;
    
    if (!(hicon = GlobalAlloc( GMEM_MOVEABLE, sizeof(ICONALLOC) ))) return 0;
    pIcon = (ICONALLOC *)GlobalLock( hicon );

    if (!OBM_CreateBitmaps( OBM_Icons_Data[id], TRUE, TRUE,
                            &pIcon->hBitmap, &pIcon->hBitMask ))
    {
        fprintf( stderr, "Error creating OEM icon %d\n", OIC_FIRST+id );
        GlobalFree( hicon );
        return 0;
    }

    bmp = (BITMAPOBJ *) GDI_GetObjPtr( pIcon->hBitmap, BITMAP_MAGIC );
    pIcon->descriptor.Width = bmp->bitmap.bmWidth;
    pIcon->descriptor.Height = bmp->bitmap.bmHeight;
    pIcon->descriptor.ColorCount = bmp->bitmap.bmBitsPixel;

    if (pIcon->hBitMask)
    {
        BITMAPOBJ *bmpMask;

          /* Invert the mask */
        bmpMask = (BITMAPOBJ *) GDI_GetObjPtr( pIcon->hBitMask, BITMAP_MAGIC );
        XSetFunction( display, BITMAP_monoGC, GXinvert );
        XFillRectangle( display, bmpMask->pixmap, BITMAP_monoGC, 0, 0,
                        bmpMask->bitmap.bmWidth, bmpMask->bitmap.bmHeight );

          /* Set the masked pixels to black */
        if (bmp->bitmap.bmBitsPixel != 1)
        {
            XSetForeground( display, BITMAP_colorGC,
                            COLOR_ToPhysical( NULL, RGB(0,0,0) ));
            XSetBackground( display, BITMAP_colorGC, 0 );
            XSetFunction( display, BITMAP_colorGC, GXor );
            XCopyPlane( display, bmpMask->pixmap, bmp->pixmap, BITMAP_colorGC,
                        0, 0, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
                        0, 0, 1 );
            XSetFunction( display, BITMAP_colorGC, GXcopy );
        }
        XSetFunction( display, BITMAP_monoGC, GXcopy );
    }

    return hicon;
}
