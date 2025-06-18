/*
 * X11DRV pen objects
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include "x11drv.h"


static DWORD get_user_dashes( char *res, const DWORD *style, DWORD len )
{
    DWORD i, pos, dashes[MAX_DASHLEN];

    len = min( len, MAX_DASHLEN );
    memcpy( dashes, style, len * sizeof(DWORD) );
    for (i = pos = 0; i < len; i++)
    {
        if (!dashes[i])  /* get rid of 0 entry */
        {
            if (i < len - 1)
            {
                i++;
                if (pos) dashes[pos - 1] += dashes[i];
                else dashes[len - 1] += dashes[i];
            }
            else if (pos)
            {
                dashes[0] += dashes[pos - 1];
                pos--;
            }
        }
        else dashes[pos++] = dashes[i];
    }
    for (i = 0; i < pos; i++) res[i] = (char)min( dashes[i], 255 );
    return pos;
}

/***********************************************************************
 *           SelectPen   (X11DRV.@)
 */
HPEN X11DRV_SelectPen( PHYSDEV dev, HPEN hpen, const struct brush_pattern *pattern )
{
    static const char PEN_dash[]          = { 16,8 };
    static const char PEN_dot[]           = { 4,4 };
    static const char PEN_dashdot[]       = { 12,8,4,8 };
    static const char PEN_dashdotdot[]    = { 12,4,4,4,4,4 };
    static const char PEN_alternate[]     = { 1,1 };
    static const char EXTPEN_dash[]       = { 3,1 };
    static const char EXTPEN_dot[]        = { 1,1 };
    static const char EXTPEN_dashdot[]    = { 3,1,1,1 };
    static const char EXTPEN_dashdotdot[] = { 3,1,1,1,1,1 };
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );
    LOGPEN logpen;
    int i;
    EXTLOGPEN *elp = NULL;

    if (!NtGdiExtGetObjectW( hpen, sizeof(logpen), &logpen ))
    {
        /* must be an extended pen */
        INT size = NtGdiExtGetObjectW( hpen, 0, NULL );

        if (!size) return 0;

        physDev->pen.ext = 1;
        elp = malloc( size );

        NtGdiExtGetObjectW( hpen, size, elp );
        logpen.lopnStyle = elp->elpPenStyle;
        logpen.lopnWidth.x = elp->elpWidth;
        logpen.lopnWidth.y = 0;
        logpen.lopnColor = elp->elpColor;
    }
    else
        physDev->pen.ext = 0;

    physDev->pen.style = logpen.lopnStyle & PS_STYLE_MASK;
    physDev->pen.type = logpen.lopnStyle & PS_TYPE_MASK;
    physDev->pen.endcap = logpen.lopnStyle & PS_ENDCAP_MASK;
    physDev->pen.linejoin = logpen.lopnStyle & PS_JOIN_MASK;

    physDev->pen.width = logpen.lopnWidth.x;
    if ((logpen.lopnStyle & PS_GEOMETRIC) || (physDev->pen.width >= 1))
    {
        physDev->pen.width = X11DRV_XWStoDS( dev->hdc, physDev->pen.width );
        if (physDev->pen.width < 0) physDev->pen.width = -physDev->pen.width;
    }

    if (physDev->pen.width == 1) physDev->pen.width = 0;  /* Faster */
    if (hpen == GetStockObject( DC_PEN ))
        NtGdiGetDCDword( dev->hdc, NtGdiGetDCPenColor, &logpen.lopnColor );
    physDev->pen.pixel = X11DRV_PALETTE_ToPhysical( physDev, logpen.lopnColor );
    switch(logpen.lopnStyle & PS_STYLE_MASK)
    {
      case PS_DASH:
            physDev->pen.dash_len = ARRAY_SIZE( PEN_dash );
            memcpy(physDev->pen.dashes, physDev->pen.ext ? EXTPEN_dash : PEN_dash,
                   physDev->pen.dash_len);
            break;
      case PS_DOT:
            physDev->pen.dash_len = ARRAY_SIZE( PEN_dot );
            memcpy(physDev->pen.dashes, physDev->pen.ext ? EXTPEN_dot : PEN_dot,
                   physDev->pen.dash_len);
            break;
      case PS_DASHDOT:
            physDev->pen.dash_len = ARRAY_SIZE( PEN_dashdot );
            memcpy(physDev->pen.dashes, physDev->pen.ext ? EXTPEN_dashdot : PEN_dashdot,
                   physDev->pen.dash_len);
            break;
      case PS_DASHDOTDOT:
            physDev->pen.dash_len = ARRAY_SIZE( PEN_dashdotdot );
            memcpy(physDev->pen.dashes, physDev->pen.ext ? EXTPEN_dashdotdot : PEN_dashdotdot,
                   physDev->pen.dash_len);
            break;
      case PS_ALTERNATE:
            physDev->pen.dash_len = ARRAY_SIZE( PEN_alternate );
            memcpy(physDev->pen.dashes, PEN_alternate, physDev->pen.dash_len);
            break;
      case PS_USERSTYLE:
            physDev->pen.dash_len = get_user_dashes( physDev->pen.dashes,
                                                     elp->elpStyleEntry, elp->elpNumEntries );
            break;
      default:
        physDev->pen.dash_len = 0;
        break;
    }
    if(physDev->pen.ext && physDev->pen.dash_len && physDev->pen.width &&
       (logpen.lopnStyle & PS_STYLE_MASK) != PS_USERSTYLE &&
       (logpen.lopnStyle & PS_STYLE_MASK) != PS_ALTERNATE)
        for(i = 0; i < physDev->pen.dash_len; i++)
            physDev->pen.dashes[i] = (char)min( physDev->pen.dashes[i] * physDev->pen.width, 255 );

    free( elp );

    return hpen;
}


/***********************************************************************
 *           SetDCPenColor (X11DRV.@)
 */
COLORREF X11DRV_SetDCPenColor( PHYSDEV dev, COLORREF crColor )
{
    X11DRV_PDEVICE *physDev = get_x11drv_dev( dev );

    if (NtGdiGetDCObject( dev->hdc, NTGDI_OBJ_PEN ) == GetStockObject( DC_PEN ))
        physDev->pen.pixel = X11DRV_PALETTE_ToPhysical( physDev, crColor );

    return crColor;
}
