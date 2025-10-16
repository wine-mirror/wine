/*
 * Copyright 2000 Corel Corporation
 * Copyright 2006 CodeWeavers, Aric Stewart
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

#include "sane_i.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* DG_IMAGE/DAT_IMAGEINFO/MSG_GET */
TW_UINT16 SANE_ImageInfoGet (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IMAGEINFO pImageInfo = (pTW_IMAGEINFO) pData;
    int resolution;

    TRACE("DG_IMAGE/DAT_IMAGEINFO/MSG_GET\n");

    if (activeDS.currentState != 6 && activeDS.currentState != 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        if (activeDS.currentState == 6)
        {
            /* return general image description information about the image about to be transferred */
            TRACE("Getting parameters\n");
            if (SANE_CALL( get_params, &activeDS.frame_params ))
            {
                WARN("sane_get_parameters failed\n");
                SANE_CALL( cancel_device, NULL );
                activeDS.twCC = TWCC_OPERATIONERROR;
                return TWRC_FAILURE;
            }
        }

        if (sane_option_get_int("resolution", &resolution) == TWCC_SUCCESS)
            pImageInfo->XResolution.Whole = pImageInfo->YResolution.Whole = resolution;
        else
            pImageInfo->XResolution.Whole = pImageInfo->YResolution.Whole = -1;
        pImageInfo->XResolution.Frac = 0;
        pImageInfo->YResolution.Frac = 0;
        pImageInfo->ImageWidth = activeDS.frame_params.pixels_per_line;
        pImageInfo->ImageLength = activeDS.frame_params.lines;

        TRACE("Bits per Sample %i\n",activeDS.frame_params.depth);
        TRACE("Frame Format %i\n",activeDS.frame_params.format);

        switch (activeDS.frame_params.format)
        {
        case FMT_RGB:
            pImageInfo->BitsPerPixel = activeDS.frame_params.depth * 3;
            pImageInfo->Compression = TWCP_NONE;
            pImageInfo->Planar = TRUE;
            pImageInfo->SamplesPerPixel = 3;
            pImageInfo->BitsPerSample[0] = activeDS.frame_params.depth;
            pImageInfo->BitsPerSample[1] = activeDS.frame_params.depth;
            pImageInfo->BitsPerSample[2] = activeDS.frame_params.depth;
            pImageInfo->PixelType = TWPT_RGB;
            break;
        case FMT_GRAY:
            pImageInfo->BitsPerPixel = activeDS.frame_params.depth;
            pImageInfo->Compression = TWCP_NONE;
            pImageInfo->Planar = TRUE;
            pImageInfo->SamplesPerPixel = 1;
            pImageInfo->BitsPerSample[0] = activeDS.frame_params.depth;
            if (activeDS.frame_params.depth == 1)
                pImageInfo->PixelType = TWPT_BW;
            else
                pImageInfo->PixelType = TWPT_GRAY;
            break;
        case FMT_OTHER:
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_SEQERROR;
            break;
        }
    }

    return twRC;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_GET */
TW_UINT16 SANE_ImageLayoutGet (pTW_IDENTITY pOrigin,
                                TW_MEMREF pData)
{
    TW_IMAGELAYOUT *img = (TW_IMAGELAYOUT *) pData;
    int tlx, tly, brx, bry;
    TW_UINT16 rc;

    TRACE("DG_IMAGE/DAT_IMAGELAYOUT/MSG_GET\n");

    rc = sane_option_get_scan_area( &tlx, &tly, &brx, &bry );
    if (rc != TWCC_SUCCESS)
    {
        activeDS.twCC = rc;
        return TWRC_FAILURE;
    }

    img->Frame.Left   = convert_sane_res_to_twain( tlx );
    img->Frame.Top    = convert_sane_res_to_twain( tly );
    img->Frame.Right  = convert_sane_res_to_twain( brx );
    img->Frame.Bottom = convert_sane_res_to_twain( bry );
    img->DocumentNumber = 1;
    img->PageNumber = 1;
    img->FrameNumber = 1;

    activeDS.twCC = TWCC_SUCCESS;
    return TWRC_SUCCESS;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_GETDEFAULT */
TW_UINT16 SANE_ImageLayoutGetDefault (pTW_IDENTITY pOrigin, 
                                       TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_RESET */
TW_UINT16 SANE_ImageLayoutReset (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_SET */
TW_UINT16 SANE_ImageLayoutSet (pTW_IDENTITY pOrigin,
                                TW_MEMREF pData)
{
    TW_IMAGELAYOUT *img = (TW_IMAGELAYOUT *) pData;
    BOOL changed = FALSE;
    TW_UINT16 twrc;
    int tlx, tly, brx, bry;

    TRACE("DG_IMAGE/DAT_IMAGELAYOUT/MSG_SET\n");
    TRACE("Frame: [Left %x.%x|Top %x.%x|Right %x.%x|Bottom %x.%x]\n",
            img->Frame.Left.Whole, img->Frame.Left.Frac,
            img->Frame.Top.Whole, img->Frame.Top.Frac,
            img->Frame.Right.Whole, img->Frame.Right.Frac,
            img->Frame.Bottom.Whole, img->Frame.Bottom.Frac);

    tlx = convert_twain_res_to_sane( img->Frame.Left );
    tly = convert_twain_res_to_sane( img->Frame.Top );
    brx = convert_twain_res_to_sane( img->Frame.Right );
    bry = convert_twain_res_to_sane( img->Frame.Bottom );

    twrc = sane_option_set_scan_area( tlx, tly, brx, bry, &changed );
    if (twrc != TWRC_SUCCESS)
        return (twrc);

    activeDS.twCC = TWCC_SUCCESS;
    return changed ? TWRC_CHECKSTATUS : TWRC_SUCCESS;
}

/* DG_IMAGE/DAT_IMAGEMEMXFER/MSG_GET */
TW_UINT16 SANE_ImageMemXferGet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IMAGEMEMXFER pImageMemXfer = (pTW_IMAGEMEMXFER) pData;

    TRACE ("DG_IMAGE/DAT_IMAGEMEMXFER/MSG_GET\n");

    if (activeDS.currentState < 6 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        struct read_data_params params;
        LPBYTE buffer;
        int retlen;
        int rows;

        /* Transfer an image from the source to the application */
        if (activeDS.currentState == 6)
        {

            /* trigger scanning dialog */
            activeDS.progressWnd = ScanningDialogBox(NULL,0);

            ScanningDialogBox(activeDS.progressWnd,0);

            if (SANE_CALL( start_device, NULL ))
            {
                activeDS.twCC = TWCC_OPERATIONERROR;
                return TWRC_FAILURE;
            }

            if (get_sane_params( &activeDS.frame_params ))
            {
                WARN("sane_get_parameters failed\n");
                SANE_CALL( cancel_device, NULL );
                activeDS.twCC = TWCC_OPERATIONERROR;
                return TWRC_FAILURE;
            }

            TRACE("Acquiring image %dx%dx%d bits (format=%d last=%d) from sane...\n"
              , activeDS.frame_params.pixels_per_line, activeDS.frame_params.lines,
              activeDS.frame_params.depth, activeDS.frame_params.format,
              activeDS.frame_params.last_frame);

            activeDS.currentState = 7;
        }

        /* access memory buffer */
        if (pImageMemXfer->Memory.Length < activeDS.frame_params.bytes_per_line)
        {
            SANE_CALL( cancel_device, NULL );
            activeDS.twCC = TWCC_BADVALUE;
            return TWRC_FAILURE;
        }

        if (pImageMemXfer->Memory.Flags & TWMF_HANDLE)
        {
            FIXME("Memory Handle, may not be locked correctly\n");
            buffer = LocalLock(pImageMemXfer->Memory.TheMem);
        }
        else
            buffer = pImageMemXfer->Memory.TheMem;
       
        memset(buffer,0,pImageMemXfer->Memory.Length);

        /* must fill full lines */
        rows = pImageMemXfer->Memory.Length / activeDS.frame_params.bytes_per_line;
        params.buffer = buffer;
        params.len = activeDS.frame_params.bytes_per_line * rows;
        params.retlen = &retlen;

        twRC = SANE_CALL( read_data, &params );
        if (twRC == TWCC_SUCCESS)
        {
            pImageMemXfer->Compression = TWCP_NONE;
            pImageMemXfer->BytesPerRow = activeDS.frame_params.bytes_per_line;
            pImageMemXfer->Columns = activeDS.frame_params.pixels_per_line;
            pImageMemXfer->Rows = rows;
            pImageMemXfer->XOffset = 0;
            pImageMemXfer->YOffset = 0;
            pImageMemXfer->BytesWritten = retlen;

            ScanningDialogBox(activeDS.progressWnd, retlen);

            if (retlen < activeDS.frame_params.bytes_per_line * rows)
            {
                ScanningDialogBox(activeDS.progressWnd, -1);
                TRACE("sane_read: %u / %u\n", retlen, activeDS.frame_params.bytes_per_line * rows);
                SANE_CALL( cancel_device, NULL );
                twRC = TWRC_XFERDONE;
            }
            activeDS.twCC = TWRC_SUCCESS;
        }
        else
        {
            ScanningDialogBox(activeDS.progressWnd, -1);
            WARN("sane_read: %u\n", twRC);
            SANE_CALL( cancel_device, NULL );
            activeDS.twCC = TWCC_OPERATIONERROR;
            twRC = TWRC_FAILURE;
        }
    }

    if (pImageMemXfer->Memory.Flags & TWMF_HANDLE)
        LocalUnlock(pImageMemXfer->Memory.TheMem);
    
    return twRC;
}

/* DG_IMAGE/DAT_IMAGENATIVEXFER/MSG_GET */
TW_UINT16 SANE_ImageNativeXferGet (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    TW_HANDLE *pHandle = (TW_HANDLE *) pData;
    HANDLE hDIB;
    BITMAPINFOHEADER *header = NULL;
    int dib_bytes;
    int dib_bytes_per_line;
    BYTE *line, color_buffer;
    RGBQUAD *colors;
    RGBTRIPLE *pixels;
    int color_size = 0;
    int i, j;
    BYTE *p;

    TRACE("DG_IMAGE/DAT_IMAGENATIVEXFER/MSG_GET\n");

    if (activeDS.currentState != 6)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        /* Transfer an image from the source to the application */
        if (SANE_CALL( start_device, NULL ))
        {
            activeDS.twCC = TWCC_OPERATIONERROR;
            return TWRC_FAILURE;
        }

        if (SANE_CALL( get_params, &activeDS.frame_params ))
        {
            WARN("sane_get_parameters failed\n");
            SANE_CALL( cancel_device, NULL );
            activeDS.twCC = TWCC_OPERATIONERROR;
            return TWRC_FAILURE;
        }

        switch (activeDS.frame_params.format)
        {
        case FMT_GRAY:
            if (activeDS.frame_params.depth == 8 || activeDS.frame_params.depth == 1)
                color_size = (1 << activeDS.frame_params.depth) * sizeof(*colors);
            else
            {
                FIXME("For NATIVE, we support only 1 bit monochrome and 8 bit Grayscale, not %d\n", activeDS.frame_params.depth);
                SANE_CALL( cancel_device, NULL );
                activeDS.twCC = TWCC_OPERATIONERROR;
                return TWRC_FAILURE;
            }
            break;
        case FMT_RGB:
            break;
        case FMT_OTHER:
            FIXME("For NATIVE, we support only GRAY and RGB\n");
            SANE_CALL( cancel_device, NULL );
            activeDS.twCC = TWCC_OPERATIONERROR;
            return TWRC_FAILURE;
        }

        TRACE("Acquiring image %dx%dx%d bits (format=%d last=%d bpl=%d) from sane...\n"
              , activeDS.frame_params.pixels_per_line, activeDS.frame_params.lines,
              activeDS.frame_params.depth, activeDS.frame_params.format,
              activeDS.frame_params.last_frame, activeDS.frame_params.bytes_per_line);

        dib_bytes_per_line = ((activeDS.frame_params.bytes_per_line + 3) / 4) * 4;
        dib_bytes = activeDS.frame_params.lines * dib_bytes_per_line;

        hDIB = GlobalAlloc(GMEM_ZEROINIT, dib_bytes + sizeof(*header) + color_size);
        if (hDIB)
           header = GlobalLock(hDIB);

        if (!header)
        {
            SANE_CALL( cancel_device, NULL );
            activeDS.twCC = TWCC_LOWMEMORY;
            if (hDIB)
                GlobalFree(hDIB);
            return TWRC_FAILURE;
        }

        header->biSize = sizeof (*header);
        header->biWidth = activeDS.frame_params.pixels_per_line;
        header->biHeight = activeDS.frame_params.lines;
        header->biPlanes = 1;
        header->biCompression = BI_RGB;
        switch (activeDS.frame_params.format)
        {
        case FMT_RGB:
            header->biBitCount = activeDS.frame_params.depth * 3;
            break;
        case FMT_GRAY:
            header->biBitCount = activeDS.frame_params.depth;
            break;
        case FMT_OTHER:
            break;
        }
        header->biSizeImage = dib_bytes;
        header->biXPelsPerMeter = 0;
        header->biYPelsPerMeter = 0;
        header->biClrUsed = 0;
        header->biClrImportant = 0;

        p = (BYTE *)(header + 1);

        if (color_size > 0)
        {
            colors = (RGBQUAD *) p;
            p += color_size;
            if (activeDS.frame_params.depth == 1)
            {
                /* Sane uses 1 to represent minimum intensity (black) and 0 for maximum (white) */
                colors[0].rgbBlue = colors[0].rgbRed = colors[0].rgbGreen = 255;
                colors[1].rgbBlue = colors[1].rgbRed = colors[1].rgbGreen = 0;
            }
            else
                for (i = 0; i < (color_size / sizeof(*colors)); i++)
                    colors[i].rgbBlue = colors[i].rgbRed = colors[i].rgbGreen = i;
        }


        /* Sane returns data in top down order.  Acrobat does best with
           a bottom up DIB being returned.  */
        line = p + (activeDS.frame_params.lines - 1) * dib_bytes_per_line;
        for (i = activeDS.frame_params.lines - 1; i >= 0; i--)
        {
            int retlen;
            struct read_data_params params = { line, activeDS.frame_params.bytes_per_line, &retlen };

            activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd,
                    ((activeDS.frame_params.lines - 1 - i) * 100)
                            /
                    (activeDS.frame_params.lines - 1));

            twRC = SANE_CALL( read_data, &params );
            if (twRC != TWCC_SUCCESS) break;
            if (retlen < activeDS.frame_params.bytes_per_line) break;
            /* TWAIN: for 24 bit color DIBs, the pixels are stored in BGR order */
            if (activeDS.frame_params.format == FMT_RGB && activeDS.frame_params.depth == 8)
            {
                pixels = (RGBTRIPLE *) line;
                for (j = 0; j < activeDS.frame_params.pixels_per_line; ++j)
                {
                    color_buffer = pixels[j].rgbtRed;
                    pixels[j].rgbtRed = pixels[j].rgbtBlue;
                    pixels[j].rgbtBlue = color_buffer;
                }
            }
            line -= dib_bytes_per_line;
        }
        activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd, -1);

        GlobalUnlock(hDIB);

        if (twRC != TWCC_SUCCESS)
        {
            WARN("sane_read: %u, reading line %d\n", twRC, i);
            SANE_CALL( cancel_device, NULL );
            activeDS.twCC = TWCC_OPERATIONERROR;
            GlobalFree(hDIB);
            return TWRC_FAILURE;
        }

        SANE_CALL( cancel_device, NULL );
        *pHandle = (TW_HANDLE)hDIB;
        twRC = TWRC_XFERDONE;
        activeDS.twCC = TWCC_SUCCESS;
        activeDS.currentState = 7;
    }
    return twRC;
}
