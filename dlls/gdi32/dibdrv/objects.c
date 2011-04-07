/*
 * DIB driver GDI objects.
 *
 * Copyright 2011 Huw Davies
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

#include "gdi_private.h"
#include "dibdrv.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dib);

/*
 *
 * Decompose the 16 ROP2s into an expression of the form
 *
 * D = (D & A) ^ X
 *
 * Where A and X depend only on P (and so can be precomputed).
 *
 *                                       A    X
 *
 * R2_BLACK         0                    0    0
 * R2_NOTMERGEPEN   ~(D | P)            ~P   ~P
 * R2_MASKNOTPEN    ~P & D              ~P    0
 * R2_NOTCOPYPEN    ~P                   0   ~P
 * R2_MASKPENNOT    P & ~D               P    P
 * R2_NOT           ~D                   1    1
 * R2_XORPEN        P ^ D                1    P
 * R2_NOTMASKPEN    ~(P & D)             P    1
 * R2_MASKPEN       P & D                P    0
 * R2_NOTXORPEN     ~(P ^ D)             1   ~P
 * R2_NOP           D                    1    0
 * R2_MERGENOTPEN   ~P | D               P   ~P
 * R2_COPYPEN       P                    0    P
 * R2_MERGEPENNOT   P | ~D              ~P    1
 * R2_MERGEPEN      P | D               ~P    P
 * R2_WHITE         1                    0    1
 *
 */

/* A = (P & A1) | (~P & A2) */
#define ZERO {0, 0}
#define ONE {0xffffffff, 0xffffffff}
#define P {0xffffffff, 0}
#define NOT_P {0, 0xffffffff}

static const DWORD rop2_and_array[16][2] =
{
    ZERO, NOT_P, NOT_P, ZERO,
    P,    ONE,   ONE,   P,
    P,    ONE,   ONE,   P,
    ZERO, NOT_P, NOT_P, ZERO
};

/* X = (P & X1) | (~P & X2) */
static const DWORD rop2_xor_array[16][2] =
{
    ZERO, NOT_P, ZERO, NOT_P,
    P,    ONE,   P,    ONE,
    ZERO, NOT_P, ZERO, NOT_P,
    P,    ONE,   P,    ONE
};

#undef NOT_P
#undef P
#undef ONE
#undef ZERO

void calc_and_xor_masks(INT rop, DWORD color, DWORD *and, DWORD *xor)
{
    /* NB The ROP2 codes start at one and the arrays are zero-based */
    *and = (color & rop2_and_array[rop-1][0]) | ((~color) & rop2_and_array[rop-1][1]);
    *xor = (color & rop2_xor_array[rop-1][0]) | ((~color) & rop2_xor_array[rop-1][1]);
}

/***********************************************************************
 *           dibdrv_SelectPen
 */
HPEN CDECL dibdrv_SelectPen( PHYSDEV dev, HPEN hpen )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSelectPen );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);
    LOGPEN logpen;

    TRACE("(%p, %p)\n", dev, hpen);

    if (!GetObjectW( hpen, sizeof(logpen), &logpen ))
    {
        /* must be an extended pen */
        EXTLOGPEN *elp;
        INT size = GetObjectW( hpen, 0, NULL );

        if (!size) return 0;

        elp = HeapAlloc( GetProcessHeap(), 0, size );

        GetObjectW( hpen, size, elp );
        /* FIXME: add support for user style pens */
        logpen.lopnStyle = elp->elpPenStyle;
        logpen.lopnWidth.x = elp->elpWidth;
        logpen.lopnWidth.y = 0;
        logpen.lopnColor = elp->elpColor;

        HeapFree( GetProcessHeap(), 0, elp );
    }

    if (hpen == GetStockObject( DC_PEN ))
        logpen.lopnColor = GetDCPenColor( dev->hdc );

    pdev->pen_color = pdev->dib.funcs->colorref_to_pixel(&pdev->dib, logpen.lopnColor);
    calc_and_xor_masks(GetROP2(dev->hdc), pdev->pen_color, &pdev->pen_and, &pdev->pen_xor);

    pdev->defer |= DEFER_PEN;

    switch(logpen.lopnStyle & PS_STYLE_MASK)
    {
    case PS_SOLID:
        if(logpen.lopnStyle & PS_GEOMETRIC) break;
        if(logpen.lopnWidth.x > 1) break;
        pdev->defer &= ~DEFER_PEN;
        break;
    default:
        break;
    }

    return next->funcs->pSelectPen( next, hpen );
}

/***********************************************************************
 *           dibdrv_SetDCPenColor
 */
COLORREF CDECL dibdrv_SetDCPenColor( PHYSDEV dev, COLORREF color )
{
    PHYSDEV next = GET_NEXT_PHYSDEV( dev, pSetDCPenColor );
    dibdrv_physdev *pdev = get_dibdrv_pdev(dev);

    if (GetCurrentObject(dev->hdc, OBJ_PEN) == GetStockObject( DC_PEN ))
    {
        pdev->pen_color = pdev->dib.funcs->colorref_to_pixel(&pdev->dib, color);
        calc_and_xor_masks(GetROP2(dev->hdc), pdev->pen_color, &pdev->pen_and, &pdev->pen_xor);
    }

    return next->funcs->pSetDCPenColor( next, color );
}
