/*
 * GDI OEM bitmap objects
 *
 * Copyright 1994 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef USE_XPM
#include <X11/xpm.h>
#endif
#include "gdi.h"
#include "bitmap.h"
#include "stddebug.h"
/* #define DEBUG_BITMAP */
#include "debug.h"

#define OBM_FIRST  OBM_LFARROWI    /* First OEM bitmap */
#define OBM_LAST   OBM_OLD_CLOSE   /* Last OEM bitmap */

#ifdef USE_XPM


#define NB_COLOR_SYMBOLS  5

  /* This is the list of the symbolic colors. All the colors used */
  /* in the xpm files must be included in this list. If you need  */
  /* to add new colors, add them just before "black", and add the */
  /* color identifier in OBM_Sys_Colors_Symbols below.            */
  /* Warning: black and white must always be the last 2 colors.   */

static XpmColorSymbol OBM_Color_Symbols[NB_COLOR_SYMBOLS+2] =
{
    { "button_face", NULL, 0 },       /* COLOR_BTNFACE */
    { "button_shadow", NULL, 0 },     /* COLOR_BTNSHADOW */
    { "button_highlight", NULL, 0 },  /* COLOR_BTNHIGHLIGHT */
    { "button_text", NULL, 0 },       /* COLOR_BTNTEXT */
    { "window_frame", NULL, 0 },      /* COLOR_WINDOWFRAME */
    { "black", NULL, 0 },
    { "white", NULL, 0 }
};

static const int OBM_Sys_Colors_Symbols[NB_COLOR_SYMBOLS] =
{
    COLOR_BTNFACE,
    COLOR_BTNSHADOW,
    COLOR_BTNHIGHLIGHT,
    COLOR_BTNTEXT,
    COLOR_WINDOWFRAME
};

  /* Don't change this list! */
static XpmColorSymbol OBM_BW_Symbols[2] =
{
    { "white", NULL, 0 },
    { "black", NULL, 1 }
};


  /* Include OEM pixmaps */
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

static const struct
{
    char** data;   /* Pointer to bitmap data */
    BOOL   color;  /* Is it a color bitmap?  */
} OBM_Pixmaps_Data[OBM_LAST-OBM_FIRST+1] = {
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

#else  /* USE_XPM */

  /* Include OEM bitmaps */
#include "bitmaps/check_boxes"
#include "bitmaps/check_mark"
#include "bitmaps/menu_arrow"

static const struct
{
    WORD width, height;
    char *data;
} OBM_Bitmaps_Data[OBM_LAST-OBM_FIRST+1] =
{
    { 0, 0, NULL },  /* OBM_LFARROWI */
    { 0, 0, NULL },  /* OBM_RGARROWI */
    { 0, 0, NULL },  /* OBM_DNARROWI */
    { 0, 0, NULL },  /* OBM_UPARROWI */
    { 0, 0, NULL },  /* OBM_COMBO */
    { menu_arrow_width, menu_arrow_height, menu_arrow_bits }, /* OBM_MNARROW */
    { 0, 0, NULL },  /* OBM_LFARROWD */
    { 0, 0, NULL },  /* OBM_RGARROWD */
    { 0, 0, NULL },  /* OBM_DNARROWD */
    { 0, 0, NULL },  /* OBM_UPARROWD */
    { 0, 0, NULL },  /* OBM_RESTORED */
    { 0, 0, NULL },  /* OBM_ZOOMD */
    { 0, 0, NULL },  /* OBM_REDUCED */
    { 0, 0, NULL },  /* OBM_RESTORE */
    { 0, 0, NULL },  /* OBM_ZOOM */
    { 0, 0, NULL },  /* OBM_REDUCE */
    { 0, 0, NULL },  /* OBM_LFARROW */
    { 0, 0, NULL },  /* OBM_RGARROW */
    { 0, 0, NULL },  /* OBM_DNARROW */
    { 0, 0, NULL },  /* OBM_UPARROW */
    { 0, 0, NULL },  /* OBM_CLOSE */
    { 0, 0, NULL },  /* OBM_OLD_RESTORE */
    { 0, 0, NULL },  /* OBM_OLD_ZOOM */
    { 0, 0, NULL },  /* OBM_OLD_REDUCE */
    { 0, 0, NULL },  /* OBM_BTNCORNERS */
    { check_boxes_width, check_boxes_height,
          check_boxes_bits },  /* OBM_CHECKBOXES */
    { check_mark_width, check_mark_height, check_mark_bits },  /* OBM_CHECK */
    { 0, 0, NULL },  /* OBM_BTSIZE */
    { 0, 0, NULL },  /* OBM_OLD_LFARROW */
    { 0, 0, NULL },  /* OBM_OLD_RGARROW */
    { 0, 0, NULL },  /* OBM_OLD_DNARROW */
    { 0, 0, NULL },  /* OBM_OLD_UPARROW */
    { 0, 0, NULL },  /* OBM_SIZE */
    { 0, 0, NULL },  /* OBM_OLD_CLOSE */
};

#endif /* USE_XPM */

extern WORD COLOR_ToPhysical( DC *dc, COLORREF color );  /* color.c */

extern Colormap COLOR_WinColormap;


/***********************************************************************
 *           OBM_InitColorSymbols
 */
#ifdef USE_XPM
static void OBM_InitColorSymbols()
{
    int i;
    static int already_done = 0;

    if (already_done) return;

      /* Init the system colors */
    for (i = 0; i < NB_COLOR_SYMBOLS; i++)
    {
        OBM_Color_Symbols[i].pixel = COLOR_ToPhysical( NULL,
                                       GetSysColor(OBM_Sys_Colors_Symbols[i]));
    }
      /* Init black and white */
    OBM_Color_Symbols[i++].pixel = COLOR_ToPhysical( NULL, RGB(0,0,0) );
    OBM_Color_Symbols[i++].pixel = COLOR_ToPhysical( NULL, RGB(255,255,255) );
    already_done = 1;
}
#endif  /* USE_XPM */

/***********************************************************************
 *           OBM_LoadOEMBitmap
 */
HBITMAP OBM_LoadOEMBitmap( WORD id )
{
    BITMAPOBJ * bmpObjPtr;
    HBITMAP hbitmap;
    WORD width, height, bpp;
    Pixmap pixmap;

    if ((id < OBM_FIRST) || (id > OBM_LAST)) return 0;
    id -= OBM_FIRST;

#ifdef USE_XPM
    if (!OBM_Pixmaps_Data[id].data) return 0;
    {
        XpmAttributes attrs;
        int err;

        OBM_InitColorSymbols();
        attrs.valuemask    = XpmColormap | XpmDepth | XpmColorSymbols;
        attrs.colormap     = COLOR_WinColormap;
        if (OBM_Pixmaps_Data[id].color) attrs.depth = bpp = screenDepth;
        else attrs.depth = bpp = 1;
        attrs.colorsymbols = (bpp > 1) ? OBM_Color_Symbols : OBM_BW_Symbols;
        attrs.numsymbols   = (bpp > 1) ? NB_COLOR_SYMBOLS + 2 : 2;
        
        if ((err = XpmCreatePixmapFromData( display, rootWindow,
                                            OBM_Pixmaps_Data[id].data,
                                            &pixmap, NULL,
                                            &attrs )) != XpmSuccess)
        {
            fprintf( stderr, "Error %d creating pixmap %d\n",
                     err, OBM_FIRST+id );
            pixmap = 0;
        }
        else
        {
            width = attrs.width;
            height = attrs.height;
        }
    }
#else
    if (!OBM_Bitmaps_Data[id].data) return 0;
    bpp = 1;
    width  = OBM_Bitmaps_Data[id].width;
    height = OBM_Bitmaps_Data[id].height;
    pixmap = XCreateBitmapFromData( display, rootWindow, 
                                    OBM_Bitmaps_Data[id].data, width, height );
#endif  /* USE_XPM */

    if (!pixmap) return 0;

      /* Create the BITMAPOBJ */
    if (!(hbitmap = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC )))
    {
        XFreePixmap( display, pixmap );
	return 0;
    }
    bmpObjPtr = (BITMAPOBJ *) GDI_HEAP_ADDR( hbitmap );
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


