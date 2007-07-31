/*
 * GDI pen objects
 *
 * Copyright 1993 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

  /* GDI logical pen object */
typedef struct
{
    GDIOBJHDR header;
    EXTLOGPEN logpen;
} PENOBJ;


static HGDIOBJ PEN_SelectObject( HGDIOBJ handle, void *obj, HDC hdc );
static INT PEN_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
static INT PEN_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );

static const struct gdi_obj_funcs pen_funcs =
{
    PEN_SelectObject,  /* pSelectObject */
    PEN_GetObject16,   /* pGetObject16 */
    PEN_GetObject,     /* pGetObjectA */
    PEN_GetObject,     /* pGetObjectW */
    NULL,              /* pUnrealizeObject */
    GDI_FreeObject     /* pDeleteObject */
};


/***********************************************************************
 *           CreatePen    (GDI32.@)
 */
HPEN WINAPI CreatePen( INT style, INT width, COLORREF color )
{
    LOGPEN logpen;

    TRACE("%d %d %06x\n", style, width, color );

    logpen.lopnStyle = style;
    logpen.lopnWidth.x = width;
    logpen.lopnWidth.y = 0;
    logpen.lopnColor = color;

    return CreatePenIndirect( &logpen );
}


/***********************************************************************
 *           CreatePenIndirect    (GDI32.@)
 */
HPEN WINAPI CreatePenIndirect( const LOGPEN * pen )
{
    PENOBJ * penPtr;
    HPEN hpen;

    if (!(penPtr = GDI_AllocObject( sizeof(PENOBJ), PEN_MAGIC, (HGDIOBJ *)&hpen,
				    &pen_funcs ))) return 0;
    if (pen->lopnStyle == PS_USERSTYLE || pen->lopnStyle == PS_ALTERNATE)
        penPtr->logpen.elpPenStyle = PS_SOLID;
    else
        penPtr->logpen.elpPenStyle = pen->lopnStyle;
    if (pen->lopnStyle == PS_NULL)
    {
        penPtr->logpen.elpWidth = 1;
        penPtr->logpen.elpColor = RGB(0, 0, 0);
    }
    else
    {
        penPtr->logpen.elpWidth = abs(pen->lopnWidth.x);
        penPtr->logpen.elpColor = pen->lopnColor;
    }
    penPtr->logpen.elpBrushStyle = BS_SOLID;
    penPtr->logpen.elpHatch = 0;
    penPtr->logpen.elpNumEntries = 0;
    penPtr->logpen.elpStyleEntry[0] = 0;

    GDI_ReleaseObj( hpen );
    return hpen;
}

/***********************************************************************
 *           ExtCreatePen    (GDI32.@)
 *
 * FIXME: PS_USERSTYLE not handled
 */

HPEN WINAPI ExtCreatePen( DWORD style, DWORD width,
                              const LOGBRUSH * brush, DWORD style_count,
                              const DWORD *style_bits )
{
    PENOBJ * penPtr;
    HPEN hpen;

    if ((style & PS_STYLE_MASK) == PS_USERSTYLE)
    {
        if(((INT)style_count) <= 0)
            return 0;

        if ((style_count > 16) || !style_bits)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        if ((style & PS_TYPE_MASK) == PS_COSMETIC)
        {
            /* FIXME: PS_USERSTYLE workaround */
            FIXME("PS_COSMETIC | PS_USERSTYLE not handled\n");
            style = (style & ~PS_STYLE_MASK) | PS_SOLID;
        }
        else
        {
            UINT i;
            BOOL has_neg = FALSE, all_zero = TRUE;

            for(i = 0; (i < style_count) && !has_neg; i++)
            {
                has_neg = has_neg || (((INT)(style_bits[i])) < 0);
                all_zero = all_zero && (style_bits[i] == 0);
            }

            if(all_zero || has_neg)
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return 0;
            }
        }
    }
    else
    {
        if (style_count || style_bits)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    if ((style & PS_STYLE_MASK) == PS_NULL)
        return CreatePen( PS_NULL, 0, brush->lbColor );

    if ((style & PS_TYPE_MASK) == PS_GEOMETRIC)
    {
        /* PS_ALTERNATE is applicable only for cosmetic pens */
        if ((style & PS_STYLE_MASK) == PS_ALTERNATE)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        if (brush->lbHatch && ((brush->lbStyle == BS_SOLID) || (brush->lbStyle == BS_HOLLOW)))
        {
            static int fixme_hatches_shown;
            if (!fixme_hatches_shown++) FIXME("Hatches not implemented\n");
        }
    }
    else
    {
        /* PS_INSIDEFRAME is applicable only for gemetric pens */
        if ((style & PS_STYLE_MASK) == PS_INSIDEFRAME || width != 1)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    if (!(penPtr = GDI_AllocObject( sizeof(PENOBJ) +
                                    style_count * sizeof(DWORD) - sizeof(penPtr->logpen.elpStyleEntry),
                                    EXT_PEN_MAGIC, (HGDIOBJ *)&hpen,
				    &pen_funcs ))) return 0;

    penPtr->logpen.elpPenStyle = style;
    penPtr->logpen.elpWidth = abs(width);
    penPtr->logpen.elpBrushStyle = brush->lbStyle;
    penPtr->logpen.elpColor = brush->lbColor;
    penPtr->logpen.elpHatch = brush->lbHatch;
    penPtr->logpen.elpNumEntries = style_count;
    memcpy(penPtr->logpen.elpStyleEntry, style_bits, style_count * sizeof(DWORD));
    
    GDI_ReleaseObj( hpen );

    return hpen;
}


/***********************************************************************
 *           PEN_SelectObject
 */
static HGDIOBJ PEN_SelectObject( HGDIOBJ handle, void *obj, HDC hdc )
{
    HGDIOBJ ret;
    DC *dc = DC_GetDCPtr( hdc );

    if (!dc) return 0;
    ret = dc->hPen;
    if (dc->funcs->pSelectPen) handle = dc->funcs->pSelectPen( dc->physDev, handle );
    if (handle) dc->hPen = handle;
    else ret = 0;
    GDI_ReleaseObj( hdc );
    return ret;
}


/***********************************************************************
 *           PEN_GetObject16
 */
static INT PEN_GetObject16( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    PENOBJ *pen = obj;
    LOGPEN16 *logpen;

    if (!buffer) return sizeof(LOGPEN16);

    if (count < sizeof(LOGPEN16)) return 0;

    logpen = buffer;
    logpen->lopnStyle = pen->logpen.elpPenStyle;
    logpen->lopnColor = pen->logpen.elpColor;
    logpen->lopnWidth.x = pen->logpen.elpWidth;
    logpen->lopnWidth.y = 0;

    return sizeof(LOGPEN16);
}


/***********************************************************************
 *           PEN_GetObject
 */
static INT PEN_GetObject( HGDIOBJ handle, void *obj, INT count, LPVOID buffer )
{
    PENOBJ *pen = obj;

    switch (GDIMAGIC(pen->header.wMagic))
    {
    case PEN_MAGIC:
    {
        LOGPEN *lp;

        if (!buffer) return sizeof(LOGPEN);

        if (count < sizeof(LOGPEN)) return 0;

        if ((pen->logpen.elpPenStyle & PS_STYLE_MASK) == PS_NULL &&
            count == sizeof(EXTLOGPEN))
        {
            EXTLOGPEN *elp = buffer;
            memcpy(elp, &pen->logpen, sizeof(EXTLOGPEN));
            elp->elpWidth = 0;
            return sizeof(EXTLOGPEN);
        }

        lp = buffer;
        lp->lopnStyle = pen->logpen.elpPenStyle;
        lp->lopnColor = pen->logpen.elpColor;
        lp->lopnWidth.x = pen->logpen.elpWidth;
        lp->lopnWidth.y = 0;
        return sizeof(LOGPEN);
    }

    case EXT_PEN_MAGIC:
    {
        INT size = sizeof(EXTLOGPEN) + pen->logpen.elpNumEntries * sizeof(DWORD) - sizeof(pen->logpen.elpStyleEntry);

        if (!buffer) return size;

        if (count < size) return 0;
        memcpy(buffer, &pen->logpen, size);
        return size;
    }

    default:
        break;
    }
    assert(0);
    return 0;
}
