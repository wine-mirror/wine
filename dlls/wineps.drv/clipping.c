/*
 *	PostScript clipping functions
 *
 *	Copyright 1999  Luc Tourangau
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

#include "psdrv.h"
#include "wine/debug.h"
#include "winbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/***********************************************************************
 *           PSDRV_AddClip
 */
void PSDRV_AddClip( print_ctx *ctx, HRGN hrgn )
{
    CHAR szArrayName[] = "clippath";
    RECT *rect;
    RGNDATA *data;
    DWORD i, size = GetRegionData(hrgn, 0, NULL);

    if (!size) return;
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return;
    GetRegionData( hrgn, size, data );
    rect = (RECT *)data->Buffer;

    switch (data->rdh.nCount)
    {
    case 0:
        /* set an empty clip path. */
        PSDRV_WriteRectClip(ctx, 0, 0, 0, 0);
        break;
    case 1:
        /* optimize when it is a simple region */
        PSDRV_WriteRectClip(ctx, rect->left, rect->top,
                            rect->right - rect->left, rect->bottom - rect->top);
        break;
    default:
        PSDRV_WriteArrayDef(ctx, szArrayName, data->rdh.nCount * 4);
        for (i = 0; i < data->rdh.nCount; i++, rect++)
        {
            PSDRV_WriteArrayPut(ctx, szArrayName, i * 4, rect->left);
            PSDRV_WriteArrayPut(ctx, szArrayName, i * 4 + 1, rect->top);
            PSDRV_WriteArrayPut(ctx, szArrayName, i * 4 + 2, rect->right - rect->left);
            PSDRV_WriteArrayPut(ctx, szArrayName, i * 4 + 3, rect->bottom - rect->top);
        }
        PSDRV_WriteRectClip2(ctx, szArrayName);
        break;
    }
    HeapFree( GetProcessHeap(), 0, data );
}

/***********************************************************************
 *           PSDRV_SetClip
 *
 * The idea here is that every graphics operation should bracket
 * output in PSDRV_SetClip/ResetClip calls.  The clip path outside
 * these calls will be empty; the reason for this is that it is
 * impossible in PostScript to cleanly make the clip path larger than
 * the current one.  Also Photoshop assumes that despite having set a
 * small clip area in the printer dc that it can still write raw
 * PostScript to the driver and expect this code not to be clipped.
 */
void PSDRV_SetClip( print_ctx *ctx )
{
    HRGN hrgn;

    TRACE("hdc=%p\n", ctx->hdc);

    if(ctx->pathdepth) {
        TRACE("inside a path, so not clipping\n");
        return;
    }

    hrgn = CreateRectRgn(0, 0, 0, 0);
    if (GetRandomRgn(ctx->hdc, hrgn, 3) == 1) /* clip && meta */
    {
        PSDRV_WriteGSave(ctx);
        PSDRV_AddClip( ctx, hrgn );
    }
    DeleteObject(hrgn);
}


/***********************************************************************
 *           PSDRV_ResetClip
 */
void PSDRV_ResetClip( print_ctx *ctx )
{
    HRGN hrgn;

    if (ctx->pathdepth) return;

    hrgn = CreateRectRgn(0, 0, 0, 0);
    if (GetRandomRgn(ctx->hdc, hrgn, 3) == 1) /* clip && meta */
        PSDRV_WriteGRestore(ctx);
    DeleteObject(hrgn);
}
