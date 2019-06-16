/*
 * QuickTime Toolkit decoder utils
 *
 * Copyright 2011 Aric Stewart, CodeWeavers
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

#define ULONG CoreFoundation_ULONG
#define HRESULT CoreFoundation_HRESULT

#define LoadResource __carbon_LoadResource
#define CompareString __carbon_CompareString
#define GetCurrentThread __carbon_GetCurrentThread
#define GetCurrentProcess __carbon_GetCurrentProcess
#define AnimatePalette __carbon_AnimatePalette
#define EqualRgn __carbon_EqualRgn
#define FillRgn __carbon_FillRgn
#define FrameRgn __carbon_FrameRgn
#define GetPixel __carbon_GetPixel
#define InvertRgn __carbon_InvertRgn
#define LineTo __carbon_LineTo
#define OffsetRgn __carbon_OffsetRgn
#define PaintRgn __carbon_PaintRgn
#define Polygon __carbon_Polygon
#define ResizePalette __carbon_ResizePalette
#define SetRectRgn __carbon_SetRectRgn

#define CheckMenuItem __carbon_CheckMenuItem
#define DeleteMenu __carbon_DeleteMenu
#define DrawMenuBar __carbon_DrawMenuBar
#define EnableMenuItem __carbon_EnableMenuItem
#define EqualRect __carbon_EqualRect
#define FillRect __carbon_FillRect
#define FrameRect __carbon_FrameRect
#define GetCursor __carbon_GetCursor
#define GetMenu __carbon_GetMenu
#define InvertRect __carbon_InvertRect
#define IsWindowVisible __carbon_IsWindowVisible
#define MoveWindow __carbon_MoveWindow
#define OffsetRect __carbon_OffsetRect
#define PtInRect __carbon_PtInRect
#define SetCursor __carbon_SetCursor
#define SetRect __carbon_SetRect
#define ShowCursor __carbon_ShowCursor
#define ShowWindow __carbon_ShowWindow
#define UnionRect __carbon_UnionRect

#include <CoreVideo/CVPixelBuffer.h>

#undef LoadResource
#undef CompareString
#undef GetCurrentThread
#undef _CDECL
#undef GetCurrentProcess
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#undef CheckMenuItem
#undef DeleteMenu
#undef DrawMenuBar
#undef EnableMenuItem
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef GetMenu
#undef InvertRect
#undef IsWindowVisible
#undef MoveWindow
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef ShowWindow
#undef UnionRect

#undef ULONG
#undef HRESULT
#undef STDMETHODCALLTYPE

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "winuser.h"
#include "dshow.h"

#include "wine/debug.h"
#include "qtprivate.h"

WINE_DEFAULT_DEBUG_CHANNEL(qtdecoder);

typedef struct {
      UInt8 a; /* Alpha Channel */
      UInt8 r; /* red component */
      UInt8 g; /* green component */
      UInt8 b; /* blue component */
} ARGBPixelRecord, *ARGBPixelPtr, **ARGBPixelHdl;

HRESULT AccessPixelBufferPixels( CVPixelBufferRef pixelBuffer, LPBYTE pbDstStream)
{
    LPBYTE pPixels = NULL;
    size_t bytesPerRow = 0, height = 0, width = 0;
    OSType actualType;
    int i;

    actualType = CVPixelBufferGetPixelFormatType(pixelBuffer);
    if (k32ARGBPixelFormat != actualType)
    {
        ERR("Pixel Buffer is not desired Type\n");
        return E_FAIL;
    }
    CVPixelBufferLockBaseAddress(pixelBuffer,0);
    pPixels = (LPBYTE)CVPixelBufferGetBaseAddress(pixelBuffer);
    bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    height = CVPixelBufferGetHeight(pixelBuffer);
    width = CVPixelBufferGetWidth(pixelBuffer);

    for (i = 1; i <= height; i++)
    {
        int j;
        LPBYTE out = pbDstStream + ((height - i) * width * 3);

        for (j = 0; j < width; j++)
        {
            *((DWORD*)out) = (((ARGBPixelPtr)pPixels)[j].r) << 16
                          | (((ARGBPixelPtr)pPixels)[j].g) << 8
                          | (((ARGBPixelPtr)pPixels)[j].b);
            out+=3;
        }
        pPixels += bytesPerRow;
    }
    CVPixelBufferUnlockBaseAddress(pixelBuffer,0);
    return S_OK;
}
