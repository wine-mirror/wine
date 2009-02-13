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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "twain.h"
#include "sane_i.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* DG_IMAGE/DAT_CIECOLOR/MSG_GET */
TW_UINT16 SANE_CIEColorGet (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_EXTIMAGEINFO/MSG_GET */
TW_UINT16 SANE_ExtImageInfoGet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_GRAYRESPONSE/MSG_RESET */
TW_UINT16 SANE_GrayResponseReset (pTW_IDENTITY pOrigin, 
                                   TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_GRAYRESPONSE/MSG_SET */
TW_UINT16 SANE_GrayResponseSet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGEFILEXFER/MSG_GET */
TW_UINT16 SANE_ImageFileXferGet (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGEINFO/MSG_GET */
TW_UINT16 SANE_ImageInfoGet (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
#ifndef SONAME_LIBSANE
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IMAGEINFO pImageInfo = (pTW_IMAGEINFO) pData;
    SANE_Status status;
    SANE_Int resolution;

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
            status = psane_get_parameters (activeDS.deviceHandle, &activeDS.sane_param);
            TRACE("Getting parameters\n");
            if (status != SANE_STATUS_GOOD)
            {
                WARN("psane_get_parameters: %s\n", psane_strstatus (status));
                psane_cancel (activeDS.deviceHandle);
                activeDS.twCC = TWCC_OPERATIONERROR;
                return TWRC_FAILURE;
            }
            activeDS.sane_param_valid = TRUE;
        }

        if (sane_option_get_int(activeDS.deviceHandle, "resolution", &resolution) == SANE_STATUS_GOOD)
            pImageInfo->XResolution.Whole = pImageInfo->YResolution.Whole = resolution;
        else
            pImageInfo->XResolution.Whole = pImageInfo->YResolution.Whole = -1;
        pImageInfo->XResolution.Frac = 0;
        pImageInfo->YResolution.Frac = 0;
        pImageInfo->ImageWidth = activeDS.sane_param.pixels_per_line;
        pImageInfo->ImageLength = activeDS.sane_param.lines;

        TRACE("Bits per Sample %i\n",activeDS.sane_param.depth);
        TRACE("Frame Format %i\n",activeDS.sane_param.format);

        if (activeDS.sane_param.format == SANE_FRAME_RGB )
        {
            pImageInfo->BitsPerPixel = activeDS.sane_param.depth * 3;
            pImageInfo->Compression = TWCP_NONE;
            pImageInfo->Planar = TRUE;
            pImageInfo->SamplesPerPixel = 3;
            pImageInfo->BitsPerSample[0] = activeDS.sane_param.depth;
            pImageInfo->BitsPerSample[1] = activeDS.sane_param.depth;
            pImageInfo->BitsPerSample[2] = activeDS.sane_param.depth;
            pImageInfo->PixelType = TWPT_RGB;
        }
        else if (activeDS.sane_param.format == SANE_FRAME_GRAY)
        {
            pImageInfo->BitsPerPixel = activeDS.sane_param.depth;
            pImageInfo->Compression = TWCP_NONE;
            pImageInfo->Planar = TRUE;
            pImageInfo->SamplesPerPixel = 1;
            pImageInfo->BitsPerSample[0] = activeDS.sane_param.depth;
            pImageInfo->PixelType = TWPT_GRAY;
        }
        else
        {
            ERR("Unhandled source frame type %i\n",activeDS.sane_param.format);
            twRC = TWRC_FAILURE;
            activeDS.twCC = TWCC_SEQERROR;
        }
    }

    return twRC;
#endif
}

/* DG_IMAGE/DAT_IMAGELAYOUT/MSG_GET */
TW_UINT16 SANE_ImageLayoutGet (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
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
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_IMAGEMEMXFER/MSG_GET */
TW_UINT16 SANE_ImageMemXferGet (pTW_IDENTITY pOrigin, 
                                 TW_MEMREF pData)
{
#ifndef SONAME_LIBSANE
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IMAGEMEMXFER pImageMemXfer = (pTW_IMAGEMEMXFER) pData;
    SANE_Status status = SANE_STATUS_GOOD;

    TRACE ("DG_IMAGE/DAT_IMAGEMEMXFER/MSG_GET\n");

    if (activeDS.currentState < 6 || activeDS.currentState > 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        LPBYTE buffer;
        int buff_len = 0;
        int consumed_len = 0;
        LPBYTE ptr;
        int rows;

        /* Transfer an image from the source to the application */
        if (activeDS.currentState == 6)
        {

            /* trigger scanning dialog */
            activeDS.progressWnd = ScanningDialogBox(NULL,0);

            ScanningDialogBox(activeDS.progressWnd,0);

            status = psane_start (activeDS.deviceHandle);
            if (status != SANE_STATUS_GOOD)
            {
                WARN("psane_start: %s\n", psane_strstatus (status));
                psane_cancel (activeDS.deviceHandle);
                activeDS.twCC = TWCC_OPERATIONERROR;
                return TWRC_FAILURE;
            }

            status = psane_get_parameters (activeDS.deviceHandle,
                    &activeDS.sane_param);
            activeDS.sane_param_valid = TRUE;

            if (status != SANE_STATUS_GOOD)
            {
                WARN("psane_get_parameters: %s\n", psane_strstatus (status));
                psane_cancel (activeDS.deviceHandle);
                activeDS.twCC = TWCC_OPERATIONERROR;
                return TWRC_FAILURE;
            }

            TRACE("Acquiring image %dx%dx%d bits (format=%d last=%d) from sane...\n"
              , activeDS.sane_param.pixels_per_line, activeDS.sane_param.lines,
              activeDS.sane_param.depth, activeDS.sane_param.format,
              activeDS.sane_param.last_frame);

            activeDS.currentState = 7;
        }

        /* access memory buffer */
        if (pImageMemXfer->Memory.Length < activeDS.sane_param.bytes_per_line)
        {
            psane_cancel (activeDS.deviceHandle);
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

        ptr = buffer;
        consumed_len = 0;
        rows = pImageMemXfer->Memory.Length / activeDS.sane_param.bytes_per_line;

        /* must fill full lines */
        while (consumed_len < (activeDS.sane_param.bytes_per_line*rows) && 
                status == SANE_STATUS_GOOD)
        {
            status = psane_read (activeDS.deviceHandle, ptr,
                    (activeDS.sane_param.bytes_per_line*rows) - consumed_len ,
                    &buff_len);
            consumed_len += buff_len;
            ptr += buff_len;
        }

        if (status == SANE_STATUS_GOOD || status == SANE_STATUS_EOF)
        {
            pImageMemXfer->Compression = TWCP_NONE;
            pImageMemXfer->BytesPerRow = activeDS.sane_param.bytes_per_line;
            pImageMemXfer->Columns = activeDS.sane_param.pixels_per_line;
            pImageMemXfer->Rows = rows;
            pImageMemXfer->XOffset = 0;
            pImageMemXfer->YOffset = 0;
            pImageMemXfer->BytesWritten = consumed_len;

            ScanningDialogBox(activeDS.progressWnd, consumed_len);

            if (status == SANE_STATUS_EOF)
            {
                ScanningDialogBox(activeDS.progressWnd, -1);
                TRACE("psane_read: %s\n", psane_strstatus (status));
                psane_cancel (activeDS.deviceHandle);
                twRC = TWRC_XFERDONE;
            }
            activeDS.twCC = TWRC_SUCCESS;
        }
        else if (status != SANE_STATUS_EOF)
        {
            ScanningDialogBox(activeDS.progressWnd, -1);
            WARN("psane_read: %s\n", psane_strstatus (status));
            psane_cancel (activeDS.deviceHandle);
            activeDS.twCC = TWCC_OPERATIONERROR;
            twRC = TWRC_FAILURE;
        }
    }

    if (pImageMemXfer->Memory.Flags & TWMF_HANDLE)
        LocalUnlock(pImageMemXfer->Memory.TheMem);
    
    return twRC;
#endif
}

#ifdef SONAME_LIBSANE
static SANE_Status read_one_line(SANE_Handle h, BYTE *line, int len)
{
    int read_len;
    SANE_Status status;

    for (;;)
    {
        read_len = 0;
        status = psane_read (activeDS.deviceHandle, line, len, &read_len);
        if (status != SANE_STATUS_GOOD)
            break;

        if (read_len == len)
            break;

        line += read_len;
        len -= read_len;
    }

    return status;
}
#endif

/* DG_IMAGE/DAT_IMAGENATIVEXFER/MSG_GET */
TW_UINT16 SANE_ImageNativeXferGet (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
#ifndef SONAME_LIBSANE
    return TWRC_FAILURE;
#else
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_UINT32 pHandle = (pTW_UINT32) pData;
    SANE_Status status;
    HANDLE hDIB;
    BITMAPINFOHEADER *header = NULL;
    int dib_bytes;
    int dib_bytes_per_line;
    BYTE *line;
    int i;

    TRACE("DG_IMAGE/DAT_IMAGENATIVEXFER/MSG_GET\n");

    if (activeDS.currentState != 6)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        /* Transfer an image from the source to the application */
        status = psane_start (activeDS.deviceHandle);
        if (status != SANE_STATUS_GOOD)
        {
            WARN("psane_start: %s\n", psane_strstatus (status));
            psane_cancel (activeDS.deviceHandle);
            activeDS.twCC = TWCC_OPERATIONERROR;
            return TWRC_FAILURE;
        }

        status = psane_get_parameters (activeDS.deviceHandle, &activeDS.sane_param);
        activeDS.sane_param_valid = TRUE;
        if (status != SANE_STATUS_GOOD)
        {
            WARN("psane_get_parameters: %s\n", psane_strstatus (status));
            psane_cancel (activeDS.deviceHandle);
            activeDS.twCC = TWCC_OPERATIONERROR;
            return TWRC_FAILURE;
        }

        if (activeDS.sane_param.format != SANE_FRAME_RGB)
        {
            FIXME("For NATIVE, we support only RGB, not %d\n", activeDS.sane_param.format);
            psane_cancel (activeDS.deviceHandle);
            activeDS.twCC = TWCC_OPERATIONERROR;
            return TWRC_FAILURE;
        }

        TRACE("Acquiring image %dx%dx%d bits (format=%d last=%d bpl=%d) from sane...\n"
              , activeDS.sane_param.pixels_per_line, activeDS.sane_param.lines,
              activeDS.sane_param.depth, activeDS.sane_param.format,
              activeDS.sane_param.last_frame, activeDS.sane_param.bytes_per_line);

        dib_bytes_per_line = ((activeDS.sane_param.bytes_per_line + 3) / 4) * 4;
        dib_bytes = activeDS.sane_param.lines * dib_bytes_per_line;

        hDIB = GlobalAlloc(GMEM_ZEROINIT, dib_bytes + sizeof(*header));
        if (hDIB)
           header = GlobalLock(hDIB);

        if (!header)
        {
            psane_cancel (activeDS.deviceHandle);
            activeDS.twCC = TWCC_LOWMEMORY;
            if (hDIB)
                GlobalFree(hDIB);
            return TWRC_FAILURE;
        }

        header->biSize = sizeof (*header);
        header->biWidth = activeDS.sane_param.pixels_per_line;
        header->biHeight = activeDS.sane_param.lines;
        header->biPlanes = 1;
        header->biBitCount = activeDS.sane_param.depth * 3;
        header->biCompression = BI_RGB;
        header->biSizeImage = dib_bytes;
        header->biXPelsPerMeter = 0;
        header->biYPelsPerMeter = 0;
        header->biClrUsed = 0;
        header->biClrImportant = 0;

        /* Sane returns data in top down order.  Acrobat does best with
           a bottom up DIB being returned.  */
        line = (BYTE *)(header + 1) +
                (activeDS.sane_param.lines - 1) * dib_bytes_per_line;
        for (i = activeDS.sane_param.lines - 1; i >= 0; i--)
        {
            activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd,
                    ((activeDS.sane_param.lines - 1 - i) * 100)
                            /
                    (activeDS.sane_param.lines - 1));

            status = read_one_line(activeDS.deviceHandle, line,
                            activeDS.sane_param.bytes_per_line);
            if (status != SANE_STATUS_GOOD)
                break;

            line -= dib_bytes_per_line;
        }
        activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd, -1);

        GlobalUnlock(hDIB);

        if (status != SANE_STATUS_GOOD && status != SANE_STATUS_EOF)
        {
            WARN("psane_read: %s, reading line %d\n", psane_strstatus(status), i);
            psane_cancel (activeDS.deviceHandle);
            activeDS.twCC = TWCC_OPERATIONERROR;
            GlobalFree(hDIB);
            return TWRC_FAILURE;
        }

        psane_cancel (activeDS.deviceHandle);
        *pHandle = (TW_UINT32)hDIB;
        twRC = TWRC_XFERDONE;
        activeDS.twCC = TWCC_SUCCESS;
        activeDS.currentState = 7;
    }
    return twRC;
#endif
}

/* DG_IMAGE/DAT_JPEGCOMPRESSION/MSG_GET */
TW_UINT16 SANE_JPEGCompressionGet (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_JPEGCOMPRESSION/MSG_GETDEFAULT */
TW_UINT16 SANE_JPEGCompressionGetDefault (pTW_IDENTITY pOrigin,
                                           
                                           TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_JPEGCOMPRESSION/MSG_RESET */
TW_UINT16 SANE_JPEGCompressionReset (pTW_IDENTITY pOrigin, 
                                      TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_JPEGCOMPRESSION/MSG_SET */
TW_UINT16 SANE_JPEGCompressionSet (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_PALETTE8/MSG_GET */
TW_UINT16 SANE_Palette8Get (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_PALETTE8/MSG_GETDEFAULT */
TW_UINT16 SANE_Palette8GetDefault (pTW_IDENTITY pOrigin, 
                                    TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_PALETTE8/MSG_RESET */
TW_UINT16 SANE_Palette8Reset (pTW_IDENTITY pOrigin, 
                               TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_PALETTE8/MSG_SET */
TW_UINT16 SANE_Palette8Set (pTW_IDENTITY pOrigin, 
                             TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_RGBRESPONSE/MSG_RESET */
TW_UINT16 SANE_RGBResponseReset (pTW_IDENTITY pOrigin, 
                                  TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}

/* DG_IMAGE/DAT_RGBRESPONSE/MSG_SET */
TW_UINT16 SANE_RGBResponseSet (pTW_IDENTITY pOrigin, 
                                TW_MEMREF pData)
{
    FIXME ("stub!\n");

    return TWRC_FAILURE;
}
