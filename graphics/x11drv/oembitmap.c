/*
 * X11DRV OEM bitmap objects
 *
 * Copyright 1994, 1995 Alexandre Julliard
 *
 */

#include "config.h"

#include "ts_xlib.h"
#include "ts_xutil.h"

#ifdef HAVE_LIBXXPM
#include "ts_xpm.h"
#else /* defined(HAVE_LIBXXPM) */
typedef unsigned long Pixel;
#endif /* defined(HAVE_LIBXXPM) */

#include <stdlib.h>
#include <string.h>

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winuser16.h"

#include "bitmap.h"
#include "cursoricon.h"
#include "debugtools.h"
#include "gdi.h"
#include "user.h" /* for TWEAK_WineLook (FIXME) */
#include "x11drv.h"

  /* Include OEM pixmaps */
#include "bitmaps/obm_close"
#include "bitmaps/obm_close_95"
#include "bitmaps/obm_closed_95"
#include "bitmaps/obm_reduce"
#include "bitmaps/obm_reduce_95"
#include "bitmaps/obm_reduced"
#include "bitmaps/obm_reduced_95"
#include "bitmaps/obm_restore"
#include "bitmaps/obm_restore_95"
#include "bitmaps/obm_restored"
#include "bitmaps/obm_restored_95"
#include "bitmaps/obm_zoom"
#include "bitmaps/obm_zoom_95"
#include "bitmaps/obm_zoomd"
#include "bitmaps/obm_zoomd_95"

DECLARE_DEBUG_CHANNEL(bitmap);
DECLARE_DEBUG_CHANNEL(cursor);
DECLARE_DEBUG_CHANNEL(x11drv);


#define OBM_FIRST  OBM_CLOSED  /* First OEM bitmap */
#define OBM_LAST   OBM_OLD_CLOSE   /* Last OEM bitmap */

static struct
{
    char** data;   /* Pointer to bitmap data */
    BOOL color;  /* Is it a color bitmap?  */
} OBM_Pixmaps_Data[OBM_LAST-OBM_FIRST+1] = {
    { obm_closed_95,TRUE},      /* OBM_CLOSED */
    { NULL, FALSE },            /* OBM_TRTYPE */
    { NULL, FALSE },            /* unused */
    { NULL, FALSE },            /* OBM_LFARROWI */
    { NULL, FALSE },            /* OBM_RGARROWI */
    { NULL, FALSE },            /* OBM_DNARROWI */
    { NULL, FALSE },            /* OBM_UPARROWI */
    { NULL, FALSE },            /* OBM_COMBO */
    { NULL, FALSE },            /* OBM_MNARROW */
    { NULL, FALSE },            /* OBM_LFARROWD */
    { NULL, FALSE },            /* OBM_RGARROWD */
    { NULL, FALSE },            /* OBM_DNARROWD */
    { NULL, FALSE },            /* OBM_UPARROWD */
    { obm_restored, TRUE },     /* OBM_RESTORED */
    { obm_zoomd, TRUE },        /* OBM_ZOOMD */
    { obm_reduced, TRUE },      /* OBM_REDUCED */
    { obm_restore, TRUE },      /* OBM_RESTORE */
    { obm_zoom, TRUE },         /* OBM_ZOOM */
    { obm_reduce, TRUE },       /* OBM_REDUCE */
    { NULL, FALSE },            /* OBM_LFARROW */
    { NULL, FALSE },            /* OBM_RGARROW */
    { NULL, FALSE },            /* OBM_DNARROW */
    { NULL, FALSE },            /* OBM_UPARROW */
    { obm_close, TRUE },        /* OBM_CLOSE */
    { NULL, FALSE },            /* OBM_OLD_RESTORE */
    { NULL, FALSE },            /* OBM_OLD_ZOOM */
    { NULL, FALSE },            /* OBM_OLD_REDUCE */
    { NULL, FALSE },            /* OBM_BTNCORNERS */
    { NULL, FALSE },            /* OBM_CHECKBOXES */
    { NULL, FALSE },            /* OBM_CHECK */
    { NULL, FALSE },            /* OBM_BTSIZE */
    { NULL, FALSE },            /* OBM_OLD_LFARROW */
    { NULL, FALSE },            /* OBM_OLD_RGARROW */
    { NULL, FALSE },            /* OBM_OLD_DNARROW */
    { NULL, FALSE },            /* OBM_OLD_UPARROW */
    { NULL, FALSE },            /* OBM_SIZE */
    { NULL, FALSE },            /* OBM_OLD_CLOSE */
};


  /* All the colors used in the xpm files must be included in this   */
  /* list, to make sure that the loaded bitmaps only use colors from */
  /* the Windows colormap. Note: the PALETTEINDEX() are not really   */
  /* palette indexes, but system colors that will be converted to    */
  /* indexes later on.                                               */

#ifndef HAVE_LIBXXPM
typedef struct {
    char  *name;
    char  *value;
    Pixel  pixel;
} XpmColorSymbol;
#endif /* !defined(HAVE_LIBXXPM) */

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

#ifdef HAVE_LIBXXPM
static XpmColorSymbol OBM_BlackAndWhite[2] =
{
    { "black", NULL, 0 },
    { "white", NULL, 0xffffffff }
};
#endif /* defined(HAVE_LIBXXPM) */

extern const DC_FUNCTIONS *X11DRV_DC_Funcs;  /* hack */


/***********************************************************************
 *           OBM_InitColorSymbols
 */
static BOOL OBM_InitColorSymbols()
{
    static BOOL initialized = FALSE;
    int i;

    if (initialized) return TRUE;  /* Already initialised */
    initialized = TRUE;

    for (i = 0; i < NB_COLOR_SYMBOLS; i++)
    {
        if (OBM_Colors[i].pixel & 0xff000000)  /* PALETTEINDEX */
            OBM_Colors[i].pixel = X11DRV_PALETTE_ToPhysical( NULL,
                                    GetSysColor(OBM_Colors[i].pixel & 0xff));
        else  /* RGB*/
            OBM_Colors[i].pixel = X11DRV_PALETTE_ToPhysical( NULL, OBM_Colors[i].pixel);
    }
    return TRUE;
}


/***********************************************************************
 *           OBM_MakeBitmap
 *
 * Allocate a GDI bitmap.
 */
#ifdef HAVE_LIBXXPM
static HBITMAP16 OBM_MakeBitmap( WORD width, WORD height,
                                 WORD bpp, Pixmap pixmap )
{
    HBITMAP hbitmap;
    BITMAPOBJ * bmpObjPtr;

    if (!pixmap) return 0;

    if (!(bmpObjPtr = GDI_AllocObject( sizeof(BITMAPOBJ), BITMAP_MAGIC, &hbitmap ))) return 0;
    bmpObjPtr->size.cx = width;
    bmpObjPtr->size.cy = height;
    bmpObjPtr->bitmap.bmType       = 0;
    bmpObjPtr->bitmap.bmWidth      = width;
    bmpObjPtr->bitmap.bmHeight     = height;
    bmpObjPtr->bitmap.bmWidthBytes = BITMAP_GetWidthBytes( width, bpp );
    bmpObjPtr->bitmap.bmPlanes     = 1;
    bmpObjPtr->bitmap.bmBitsPixel  = bpp;
    bmpObjPtr->bitmap.bmBits       = NULL;
    bmpObjPtr->dib                 = NULL;

    bmpObjPtr->funcs = X11DRV_DC_Funcs;
    bmpObjPtr->physBitmap = (void *)pixmap;

    GDI_ReleaseObj( hbitmap );
    return hbitmap;
}
#endif /* defined(HAVE_LIBXXPM) */


/***********************************************************************
 *           OBM_CreateBitmaps
 *
 * Create the 2 bitmaps from XPM data.
 */
static BOOL OBM_CreateBitmaps( char **data, BOOL color,
                               HBITMAP16 *bitmap, HBITMAP16 *mask, POINT *hotspot )
{
#ifdef HAVE_LIBXXPM
    XpmAttributes *attrs;
    Pixmap pixmap, pixmask;
    int err;

    attrs = (XpmAttributes *)HeapAlloc( GetProcessHeap(), 0, XpmAttributesSize() );
    if (attrs == NULL) return FALSE;
    attrs->valuemask    = XpmColormap | XpmDepth | XpmColorSymbols | XpmHotspot;
    attrs->colormap     = X11DRV_PALETTE_PaletteXColormap;
    attrs->depth        = color ? screen_depth : 1;
    attrs->colorsymbols = (attrs->depth > 1) ? OBM_Colors : OBM_BlackAndWhite;
    attrs->numsymbols   = (attrs->depth > 1) ? NB_COLOR_SYMBOLS : 2;

    err = TSXpmCreatePixmapFromData( gdi_display, root_window, data, &pixmap, &pixmask, attrs );
    if (err != XpmSuccess)
    {
        HeapFree( GetProcessHeap(), 0, attrs );
        return FALSE;
    }

    if (hotspot)
    {
        hotspot->x = attrs->x_hotspot;
        hotspot->y = attrs->y_hotspot;
    }

    if (bitmap)
        *bitmap = OBM_MakeBitmap( attrs->width, attrs->height,
                                  attrs->depth, pixmap );
        
    if (mask)
        *mask = OBM_MakeBitmap( attrs->width, attrs->height,
                                1, pixmask );

    HeapFree( GetProcessHeap(), 0, attrs );

    if (pixmap && (!bitmap || !*bitmap)) TSXFreePixmap( gdi_display, pixmap );
    if (pixmask && (!mask || !*mask)) TSXFreePixmap( gdi_display, pixmask );

    if (bitmap && !*bitmap)
    {
        if (mask && *mask) DeleteObject( *mask );
        return FALSE;
    }
    return TRUE;
#else /* defined(HAVE_LIBXXPM) */
    FIXME_(x11drv)(
        "Xpm support not in the binary, "
	"please install the Xpm and Xpm-devel packages and recompile wine\n"
    );
    return FALSE;
#endif /* defined(HAVE_LIBXXPM) */
}


/***********************************************************************
 *           OBM_LoadBitmap
 */
static HBITMAP16 OBM_LoadBitmap( WORD id )
{
    HBITMAP16 bitmap;

    if ((id < OBM_FIRST) || (id > OBM_LAST)) return 0;
    id -= OBM_FIRST;
    if (!OBM_Pixmaps_Data[id].data) return 0;

    if (!OBM_InitColorSymbols()) return 0;

    if (!OBM_CreateBitmaps( OBM_Pixmaps_Data[id].data, OBM_Pixmaps_Data[id].color,
                            &bitmap, NULL, NULL ))
    {
        WARN_(bitmap)("Error creating OEM bitmap %d\n", OBM_FIRST+id );
        return 0;
    }
    return bitmap;
}


/***********************************************************************
 *           LoadOEMResource (X11DRV.@)
 *
 */
HANDLE X11DRV_LoadOEMResource(WORD resid, WORD type)
{
    switch(type) {
    case OEM_BITMAP:
        return OBM_LoadBitmap(resid);

    default:
        ERR_(x11drv)("Unknown type\n");
    }
    return 0;
}


/***********************************************************************
 *           X11DRV_OBM_Init
 *
 * Initializes the OBM_Pixmaps_Data and OBM_Icons_Data struct
 */
BOOL X11DRV_OBM_Init(void)
{
    if (TWEAK_WineLook == WIN31_LOOK) {
	OBM_Pixmaps_Data[OBM_ZOOMD - OBM_FIRST].data = obm_zoomd;
	OBM_Pixmaps_Data[OBM_REDUCED - OBM_FIRST].data = obm_reduced;
	OBM_Pixmaps_Data[OBM_ZOOM - OBM_FIRST].data = obm_zoom;
	OBM_Pixmaps_Data[OBM_REDUCE - OBM_FIRST].data = obm_reduce;
	OBM_Pixmaps_Data[OBM_CLOSE - OBM_FIRST].data = obm_close;
        OBM_Pixmaps_Data[OBM_RESTORE - OBM_FIRST].data = obm_restore;
        OBM_Pixmaps_Data[OBM_RESTORED - OBM_FIRST].data = obm_restored;
    }
    else {
	OBM_Pixmaps_Data[OBM_ZOOMD - OBM_FIRST].data = obm_zoomd_95;
	OBM_Pixmaps_Data[OBM_REDUCED - OBM_FIRST].data = obm_reduced_95;
	OBM_Pixmaps_Data[OBM_ZOOM - OBM_FIRST].data = obm_zoom_95;
	OBM_Pixmaps_Data[OBM_REDUCE - OBM_FIRST].data = obm_reduce_95;
	OBM_Pixmaps_Data[OBM_CLOSE - OBM_FIRST].data = obm_close_95;
        OBM_Pixmaps_Data[OBM_RESTORE - OBM_FIRST].data = obm_restore_95;
        OBM_Pixmaps_Data[OBM_RESTORED - OBM_FIRST].data = obm_restored_95;
    }

    return 1;
}

