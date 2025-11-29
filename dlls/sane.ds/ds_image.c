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
#include "resource.h"

#include <winnls.h>

WINE_DEFAULT_DEBUG_CHANNEL(twain);

/* Sane result codes from sane.h */
typedef enum
{
    SANE_STATUS_GOOD = 0,       /* everything A-OK */
    SANE_STATUS_UNSUPPORTED,    /* operation is not supported */
    SANE_STATUS_CANCELLED,      /* operation was cancelled */
    SANE_STATUS_DEVICE_BUSY,    /* device is busy; try again later */
    SANE_STATUS_INVAL,          /* data is invalid (includes no dev at open) */
    SANE_STATUS_EOF,            /* no more data available (end-of-file) */
    SANE_STATUS_JAMMED,         /* document feeder jammed */
    SANE_STATUS_NO_DOCS,        /* document feeder out of documents */
    SANE_STATUS_COVER_OPEN,     /* scanner cover is open */
    SANE_STATUS_IO_ERROR,       /* error during device I/O */
    SANE_STATUS_NO_MEM,         /* out of memory */
    SANE_STATUS_ACCESS_DENIED   /* access to resource has been denied */
}
SANE_Status;


/* transition from state 6 to state 7.
 *
 * Called by either SANE_ImageMemXferGet, SANE_ImageInfoGet or
 * SANE_ImageNativeXferGet, whatever the application calls first.
 *
 * - start the scan with a call to sane start_devince
 * - open the progress dialog window
 * - call get_sane_params to retrieve parameters of current scan frame
 *
 * @return TWAIN result code, TWRC_SUCCESS on success
 */
TW_UINT16 SANE_Start(void)
{
  TW_UINT16 twRC = TWRC_SUCCESS;
  SANE_Status status;

  TRACE("SANE_Start currentState:%d\n", activeDS.currentState);
  if (activeDS.currentState != 6)
  {
      twRC = TWRC_FAILURE;
      activeDS.twCC = TWCC_SEQERROR;
  }
  else
  {
      /* Open progress dialog */
      activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd,0);

      do
      {
          /* Start the scan process in sane */
          status = SANE_CALL( start_device, NULL );

          if (status == SANE_STATUS_GOOD)
          {
              twRC = TWRC_SUCCESS;
          }
          else if (!activeDS.capIndicators && !activeDS.ShowUI)
          {
              twRC = TWRC_FAILURE;
          }
          else
          {
              WCHAR szCaption[256];
              WCHAR szMessage[1024];

              LoadStringW( SANE_instance, IDS_CAPTION, szCaption, ARRAY_SIZE( szCaption ) );
              switch (status)
              {
                case SANE_STATUS_NO_DOCS:
                  LoadStringW( SANE_instance, IDS_NO_DOCS, szMessage, ARRAY_SIZE( szMessage ) );
                  twRC =
                    activeDS.scannedImages!=0 ? TWRC_FAILURE :
                    MessageBoxW(activeDS.progressWnd, szMessage, szCaption, MB_ICONWARNING | MB_RETRYCANCEL)==IDCANCEL ? TWRC_FAILURE :TWRC_CHECKSTATUS;
                  break;
                case SANE_STATUS_JAMMED:
                  LoadStringW( SANE_instance, IDS_JAMMED, szMessage, ARRAY_SIZE( szMessage ) );
                  twRC =
                    MessageBoxW(activeDS.progressWnd, szMessage, szCaption, MB_ICONWARNING | MB_RETRYCANCEL)==IDCANCEL ? TWRC_FAILURE :TWRC_CHECKSTATUS;
                  break;
                case SANE_STATUS_COVER_OPEN:
                  LoadStringW( SANE_instance, IDS_COVER_OPEN, szMessage, ARRAY_SIZE( szMessage ) );
                  twRC =
                    MessageBoxW(activeDS.progressWnd, szMessage, szCaption, MB_ICONWARNING | MB_RETRYCANCEL)==IDCANCEL ? TWRC_FAILURE :TWRC_CHECKSTATUS;
                  break;
                case SANE_STATUS_DEVICE_BUSY:
                  LoadStringW( SANE_instance, IDS_DEVICE_BUSY, szMessage, ARRAY_SIZE( szMessage ) );
                  twRC =
                    MessageBoxW(activeDS.progressWnd, szMessage, szCaption, MB_ICONWARNING | MB_RETRYCANCEL)==IDCANCEL ? TWRC_FAILURE :TWRC_CHECKSTATUS;
                  break;
                default:
                  twRC = TWRC_FAILURE;
              }
          }
      }
      while (twRC == TWRC_CHECKSTATUS);

      /* If starting the scan failed, cancel scan job */
      if (twRC != TWRC_SUCCESS)
      {
          UI_Enable(TRUE);
          activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd, -1);
          activeDS.twCC = TWCC_OPERATIONERROR;
          return TWRC_FAILURE;
      }

      if (get_sane_params( &activeDS.frame_params ))
      {
          WARN("sane_get_parameters failed\n");
          SANE_CALL( cancel_device, NULL );
          UI_Enable(TRUE);
          activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd, -1);
          activeDS.twCC = TWCC_OPERATIONERROR;
          return TWRC_FAILURE;
      }

      if (activeDS.progressWnd)
      {
          WCHAR szLocaleBuffer[4];
          WCHAR szResolution[200];
          WCHAR szFormat[20];
          int sid_format;

          switch (activeDS.frame_params.format)
          {
          case FMT_RGB:  sid_format=IDS_COLOUR; break;
          case FMT_GRAY: sid_format=activeDS.frame_params.depth==1 ? IDS_LINEART : IDS_GRAY; break;
          default:       sid_format=IDS_UNKNOWN;
          }

          LoadStringW( SANE_instance, sid_format, szFormat, ARRAY_SIZE( szFormat ) );

          if (activeDS.XResolution.Whole && activeDS.YResolution.Whole)
          {
              const double xresolution = activeDS.XResolution.Whole + activeDS.XResolution.Frac / 65536.0;
              const double yresolution = activeDS.YResolution.Whole + activeDS.YResolution.Frac / 65536.0;

              GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_IMEASURE, szLocaleBuffer, ARRAY_SIZE(szLocaleBuffer));
              if (szLocaleBuffer[0]=='1')
              {
                  swprintf(szResolution, ARRAY_SIZE(szResolution), L"%d DPI %s %.2f x %.2f \"",
                           activeDS.XResolution.Whole, szFormat,
                           activeDS.frame_params.pixels_per_line / xresolution ,
                           activeDS.frame_params.lines           / yresolution );
              }
              else
              {
                  swprintf(szResolution, ARRAY_SIZE(szResolution), L"%d DPI %s %.1f x %.1f mm",
                           activeDS.XResolution.Whole, szFormat,
                           activeDS.frame_params.pixels_per_line * 25.4 / xresolution ,
                           activeDS.frame_params.lines           * 25.4 / yresolution );
              }
              SetDlgItemTextW(activeDS.progressWnd, IDC_RESOLUTION, szResolution);
          }
      }

      TRACE("Acquiring image %dx%dx%d bits (format=%d last=%d) from sane...\n"
            , activeDS.frame_params.pixels_per_line, activeDS.frame_params.lines,
            activeDS.frame_params.depth, activeDS.frame_params.format,
            activeDS.frame_params.last_frame);

      activeDS.currentState = 7;
      activeDS.YOffset = 0;
  }
  return twRC;
}

/** transition from state 7 or 6 to state 5.
 *
 * Called when an error occurs or when the last frame
 * has been transfered successfully.
 * - call sane cancel_device to finish any open transfer
 * - close the progress dialog box.
 */
void SANE_Cancel(void)
{
  SANE_CALL( cancel_device, NULL );
  UI_Enable(TRUE);
  activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd, -1);
  activeDS.currentState = 5;
}



/* DG_IMAGE/DAT_IMAGEINFO/MSG_GET */
TW_UINT16 SANE_ImageInfoGet (pTW_IDENTITY pOrigin, 
                              TW_MEMREF pData)
{
    TW_UINT16 twRC = TWRC_SUCCESS;
    pTW_IMAGEINFO pImageInfo = (pTW_IMAGEINFO) pData;

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
            /* Transition from state 6 to state 7 by starting the scan */
            twRC = SANE_Start();
            if (twRC != TWRC_SUCCESS)
            {
                return twRC;
            }
        }

        pImageInfo->XResolution = activeDS.XResolution;
        pImageInfo->YResolution = activeDS.YResolution;
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
            /* Transition from state 6 to state 7 by starting the scan */
            twRC = SANE_Start();
            if (twRC != TWRC_SUCCESS)
            {
                return twRC;
            }
        }

        /* access memory buffer */
        if (pImageMemXfer->Memory.Length < activeDS.frame_params.bytes_per_line)
        {
            SANE_Cancel();
            activeDS.twCC = TWCC_BADVALUE;
            return TWRC_FAILURE;
        }

        if (pImageMemXfer->Memory.Flags & TWMF_HANDLE)
            buffer = GlobalLock(pImageMemXfer->Memory.TheMem);
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
            pImageMemXfer->YOffset = activeDS.YOffset;
            pImageMemXfer->BytesWritten = retlen;
            activeDS.YOffset += rows;

            ScanningDialogBox(activeDS.progressWnd, MulDiv(activeDS.YOffset, 100, activeDS.frame_params.lines));

            if (retlen < activeDS.frame_params.bytes_per_line * rows)
            {
                TRACE("sane_read: %u / %u\n", retlen, activeDS.frame_params.bytes_per_line * rows);
                twRC = TWRC_XFERDONE;
            }
            activeDS.twCC = TWRC_SUCCESS;
        }
        else
        {
            WARN("sane_read: %u\n", twRC);
            SANE_Cancel();
            activeDS.twCC = TWCC_OPERATIONERROR;
            twRC = TWRC_FAILURE;
        }
    }

    if (pImageMemXfer->Memory.Flags & TWMF_HANDLE)
        GlobalUnlock(pImageMemXfer->Memory.TheMem);

    if (activeDS.userCancelled)
    {
        SANE_Cancel();
        activeDS.twCC = TWCC_OPERATIONERROR;
        twRC = TWRC_FAILURE;
    }
    
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
    int y, eof = 0;
    BYTE *p;
    DWORD tmp, *ptop, *pbot;

    TRACE("DG_IMAGE/DAT_IMAGENATIVEXFER/MSG_GET\n");

    if (activeDS.currentState == 6)
    {
        /* Transition from state 6 to state 7 by starting the scan */
        twRC = SANE_Start();
        if (twRC != TWRC_SUCCESS)
        {
            return twRC;
        }
    }

    if (activeDS.currentState != 7)
    {
        twRC = TWRC_FAILURE;
        activeDS.twCC = TWCC_SEQERROR;
    }
    else
    {
        switch (activeDS.frame_params.format)
        {
        case FMT_GRAY:
            if (activeDS.frame_params.depth == 8 || activeDS.frame_params.depth == 1)
                color_size = (1 << activeDS.frame_params.depth) * sizeof(*colors);
            else
            {
                FIXME("For NATIVE, we support only 1 bit monochrome and 8 bit Grayscale, not %d\n", activeDS.frame_params.depth);
                SANE_Cancel();
                activeDS.twCC = TWCC_OPERATIONERROR;
                activeDS.currentState = 6;
                return TWRC_FAILURE;
            }
            break;
        case FMT_RGB:
            if (activeDS.frame_params.depth != 8)
            {
                FIXME("For NATIVE, we support only 8 bit per color channel, not %d\n", activeDS.frame_params.depth);
                SANE_Cancel();
                activeDS.twCC = TWCC_OPERATIONERROR;
                activeDS.currentState = 6;
                return TWRC_FAILURE;
            }
            break;
        case FMT_OTHER:
            FIXME("For NATIVE, we support only GRAY and RGB\n");
            SANE_Cancel();
            activeDS.twCC = TWCC_OPERATIONERROR;
            activeDS.currentState = 6;
            return TWRC_FAILURE;
        }

        dib_bytes_per_line = ((activeDS.frame_params.bytes_per_line + 3) / 4) * 4;
        y = activeDS.feederEnabled ? activeDS.frame_params.lines * 3/2 : activeDS.frame_params.lines+1;
        dib_bytes = y * dib_bytes_per_line;

        hDIB = GlobalAlloc(GMEM_MOVEABLE, dib_bytes + sizeof(*header) + color_size);
        if (hDIB)
           header = GlobalLock(hDIB);

        if (!header)
        {
            SANE_Cancel();
            activeDS.twCC = TWCC_LOWMEMORY;
            activeDS.currentState = 6;
            if (hDIB)
                GlobalFree(hDIB);
            return TWRC_FAILURE;
        }

        memset(header, 0, sizeof(BITMAPINFOHEADER)+color_size);
        header->biSize = sizeof (*header);
        header->biWidth = activeDS.frame_params.pixels_per_line;
        header->biHeight = -y;
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

        y=0;
        do
        {
            int retlen;
            struct read_data_params params = { NULL, activeDS.frame_params.bytes_per_line, &retlen };

            if (y >= -header->biHeight)
            {
                TRACE("Data source transfers more lines than expected. Extend DIB to %ld lines\n", (-header->biHeight)+1000);
                GlobalUnlock(hDIB);

                dib_bytes += 1000 * dib_bytes_per_line;

                if (!GlobalReAlloc(hDIB, dib_bytes + sizeof(*header) + color_size, GMEM_MOVEABLE))
                {
                    SANE_Cancel();
                    activeDS.twCC = TWCC_LOWMEMORY;
                    activeDS.currentState = 6;
                    GlobalFree(hDIB);
                    return TWRC_FAILURE;
                }

                header = (BITMAPINFOHEADER *) GlobalLock(hDIB);
                header->biHeight -= 1000;

                p = ((BYTE *) header) + header->biSize + color_size;
            }

            activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd,
                                                     MulDiv(y, 100, activeDS.frame_params.lines));

            params.buffer =
              line = p + y * dib_bytes_per_line;

            if (activeDS.frame_params.bytes_per_line & 3)
            {
                /* Set padding-bytes at the end of the line buffer to 0 */
                memset(line+dib_bytes_per_line-4, 0, 4);
            }

            twRC = SANE_CALL( read_data, &params );
            if (twRC != TWCC_SUCCESS) break;
            if (retlen < activeDS.frame_params.bytes_per_line)
            {   /* EOF reached */
                eof = 1;
            }
            else
            {
                y++;
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
            }
        }
        while (!eof && !activeDS.userCancelled);

        if (twRC != TWCC_SUCCESS || y==0 || activeDS.userCancelled)
        {
            WARN("sane_read: %u, reading line %d\n", twRC, y);
            SANE_Cancel();
            activeDS.twCC = TWCC_OPERATIONERROR;
            GlobalUnlock(hDIB);
            GlobalFree(hDIB);
            return TWRC_FAILURE;
        }

        /* Sane returns data in top down order.  Acrobat does best with
           a bottom up DIB being returned. Flip image */
        header->biHeight = y;
        for (y = 0; y<header->biHeight / 2; y++)
        {
            ptop = (DWORD *) (p + dib_bytes_per_line *                   y    );
            pbot = (DWORD *) (p + dib_bytes_per_line * (header->biHeight-y-1) );

            for (i=0; i<dib_bytes_per_line/4; i++)
            {
                tmp = *ptop;
                *ptop = *pbot;
                *pbot = tmp;
                ptop++;
                pbot++;
            }
        }

        /* Shrink the memory block to the size actually transfered by
         * the sane data source. */
        header->biSizeImage =
          dib_bytes = header->biHeight * dib_bytes_per_line;

        GlobalUnlock(hDIB);

        if (!GlobalReAlloc(hDIB, dib_bytes + sizeof(*header) + color_size, GMEM_MOVEABLE))
        {
            SANE_Cancel();
            activeDS.twCC = TWCC_LOWMEMORY;
            activeDS.currentState = 6;
            GlobalUnlock(hDIB);
            GlobalFree(hDIB);
            return TWRC_FAILURE;
        }

        activeDS.progressWnd = ScanningDialogBox(activeDS.progressWnd, -1);

        *pHandle = (TW_HANDLE)hDIB;
        twRC = TWRC_XFERDONE;
        activeDS.twCC = TWCC_SUCCESS;
    }
    return twRC;
}
