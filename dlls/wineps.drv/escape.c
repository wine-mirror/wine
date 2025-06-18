/*
 *	PostScript driver Escape function
 *
 *	Copyright 1998  Huw D M Davies
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "winuser.h"
#include "winreg.h"
#include "psdrv.h"
#include "wine/debug.h"
#include "winspool.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

BOOL flush_spool( print_ctx *ctx )
{
    DWORD written;

    if (ctx->job.data_cnt)
    {
        if (!WritePrinter(ctx->job.hprinter, ctx->job.data, ctx->job.data_cnt, &written) ||
                written != ctx->job.data_cnt)
            return FALSE;
    }
    ctx->job.data_cnt = 0;
    return TRUE;
}

DWORD write_spool( print_ctx *ctx, const void *data, DWORD num )
{
    DWORD written;

    if (ctx->job.data_cnt + num > ARRAY_SIZE(ctx->job.data))
    {
        if (!flush_spool(ctx))
            return SP_OUTOFDISK;
    }

    if (ctx->job.data_cnt + num > ARRAY_SIZE(ctx->job.data))
    {
        if (!WritePrinter(ctx->job.hprinter, (LPBYTE) data, num, &written) ||
                written != num)
            return SP_OUTOFDISK;
    }
    else
    {
        memcpy(ctx->job.data + ctx->job.data_cnt, data, num);
        ctx->job.data_cnt += num;
    }
    return num;
}

/**********************************************************************
 *           ExtEscape
 */
INT PSDRV_ExtEscape( print_ctx *ctx, INT nEscape, INT cbInput, LPCVOID in_data,
                     INT cbOutput, LPVOID out_data )
{
    TRACE("%p,%d,%d,%p,%d,%p\n",
          ctx->hdc, nEscape, cbInput, in_data, cbOutput, out_data);

    switch(nEscape)
    {
    case POSTSCRIPT_DATA:
    case PASSTHROUGH:
    case POSTSCRIPT_PASSTHROUGH:
    {
        /* Write directly to spool file, bypassing normal PS driver
         * processing that is done along with writing PostScript code
         * to the spool.
         * We have a WORD before the data counting the size, but
         * cbInput is just this +2.
         * However Photoshop 7 has a bug that sets cbInput to 2 less than the
         * length of the string, rather than 2 more.  So we'll use the WORD at
         * in_data[0] instead.
         */
        passthrough_enter(ctx);
        return write_spool(ctx, ((char*)in_data) + 2, *(const WORD*)in_data);
    }

    case POSTSCRIPT_IGNORE:
    {
        BOOL ret = ctx->job.quiet;
        TRACE("POSTSCRIPT_IGNORE %d\n", *(const short*)in_data);
        ctx->job.quiet = *(const short*)in_data;
        return ret;
    }

    case BEGIN_PATH:
        TRACE("BEGIN_PATH\n");
        if(ctx->pathdepth)
            FIXME("Nested paths not yet handled\n");
        return ++ctx->pathdepth;

    case END_PATH:
    {
        const struct PATH_INFO *info = (const struct PATH_INFO*)in_data;

        TRACE("END_PATH\n");
        if(!ctx->pathdepth) {
            ERR("END_PATH called without a BEGIN_PATH\n");
            return -1;
        }
        TRACE("RenderMode = %d, FillMode = %d, BkMode = %d\n",
                info->RenderMode, info->FillMode, info->BkMode);
        switch(info->RenderMode) {
        case RENDERMODE_NO_DISPLAY:
            PSDRV_WriteClosePath(ctx); /* not sure if this is necessary, but it can't hurt */
            break;
        case RENDERMODE_OPEN:
        case RENDERMODE_CLOSED:
        default:
            FIXME("END_PATH: RenderMode %d, not yet supported\n", info->RenderMode);
            break;
        }
        return --ctx->pathdepth;
    }

    case CLIP_TO_PATH:
    {
        WORD mode = *(const WORD*)in_data;

        switch(mode) {
        case CLIP_SAVE:
            TRACE("CLIP_TO_PATH: CLIP_SAVE\n");
            PSDRV_WriteGSave(ctx);
            return 1;
        case CLIP_RESTORE:
            TRACE("CLIP_TO_PATH: CLIP_RESTORE\n");
            PSDRV_WriteGRestore(ctx);
            return 1;
        case CLIP_INCLUSIVE:
            TRACE("CLIP_TO_PATH: CLIP_INCLUSIVE\n");
            /* FIXME to clip or eoclip ? (see PATH_INFO.FillMode) */
            PSDRV_WriteClip(ctx);
            PSDRV_WriteNewPath(ctx);
            return 1;
        case CLIP_EXCLUSIVE:
            FIXME("CLIP_EXCLUSIVE: not implemented\n");
            return 0;
        default:
            FIXME("Unknown CLIP_TO_PATH mode %d\n", mode);
            return 0;
        }
    }

    default:
        return 0;
    }
}

/************************************************************************
 *           PSDRV_StartPage
 */
INT PSDRV_StartPage( print_ctx *ctx )
{
    TRACE("%p\n", ctx->hdc);

    if(!ctx->job.OutOfPage) {
        FIXME("Already started a page?\n");
	return 1;
    }

    ctx->job.PageNo++;

    if(!PSDRV_WriteNewPage( ctx ))
        return 0;
    ctx->job.OutOfPage = FALSE;
    return 1;
}


/************************************************************************
 *           PSDRV_EndPage
 */
INT PSDRV_EndPage( print_ctx *ctx )
{
    TRACE("%p\n", ctx->hdc);

    if(ctx->job.OutOfPage) {
        FIXME("Already ended a page?\n");
	return 1;
    }

    passthrough_leave(ctx);
    if(!PSDRV_WriteEndPage( ctx ))
        return 0;
    PSDRV_EmptyDownloadList(ctx, FALSE);
    ctx->job.OutOfPage = TRUE;
    return 1;
}
