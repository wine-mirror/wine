/*
 * Copyright 2010 Damjan Jovanovic
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
#include "wine/port.h"

#include <stdarg.h>

#ifdef HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H
#define GetCurrentProcess GetCurrentProcess_Mac
#define GetCurrentThread GetCurrentThread_Mac
#define LoadResource LoadResource_Mac
#define AnimatePalette AnimatePalette_Mac
#define EqualRgn EqualRgn_Mac
#define FillRgn FillRgn_Mac
#define FrameRgn FrameRgn_Mac
#define GetPixel GetPixel_Mac
#define InvertRgn InvertRgn_Mac
#define LineTo LineTo_Mac
#define OffsetRgn OffsetRgn_Mac
#define PaintRgn PaintRgn_Mac
#define Polygon Polygon_Mac
#define ResizePalette ResizePalette_Mac
#define SetRectRgn SetRectRgn_Mac
#define EqualRect EqualRect_Mac
#define FillRect FillRect_Mac
#define FrameRect FrameRect_Mac
#define GetCursor GetCursor_Mac
#define InvertRect InvertRect_Mac
#define OffsetRect OffsetRect_Mac
#define PtInRect PtInRect_Mac
#define SetCursor SetCursor_Mac
#define SetRect SetRect_Mac
#define ShowCursor ShowCursor_Mac
#define UnionRect UnionRect_Mac
#include <ApplicationServices/ApplicationServices.h>
#undef GetCurrentProcess
#undef GetCurrentThread
#undef LoadResource
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
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef InvertRect
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef UnionRect
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#if defined(HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H) && \
    MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_4

typedef struct IcnsEncoder {
    struct encoder encoder;
    IStream *stream;
    IconFamilyHandle icns_family;
    struct encoder_frame frame;
    OSType icns_type;
    BYTE* icns_image;
    int lines_written;
} IcnsEncoder;

static inline IcnsEncoder *impl_from_encoder(struct encoder *iface)
{
    return CONTAINING_RECORD(iface, IcnsEncoder, encoder);
}

void CDECL IcnsEncoder_destroy(struct encoder *iface)
{
    IcnsEncoder *This = impl_from_encoder(iface);

    free(This->icns_image);

    if (This->icns_family)
        DisposeHandle((Handle)This->icns_family);

    RtlFreeHeap(GetProcessHeap(), 0, This);
}

HRESULT CDECL IcnsEncoder_get_supported_format(struct encoder *iface,
    GUID *pixel_format, DWORD *bpp, BOOL *indexed)
{
    *pixel_format = GUID_WICPixelFormat32bppBGRA;
    *bpp = 32;
    *indexed = FALSE;
    return S_OK;
}

HRESULT CDECL IcnsEncoder_create_frame(struct encoder *iface, const struct encoder_frame *frame)
{
    IcnsEncoder *This = impl_from_encoder(iface);

    This->frame = *frame;

    switch (frame->width)
    {
        case 16:  This->icns_type = kIconServices16PixelDataARGB;  break;
        case 32:  This->icns_type = kIconServices32PixelDataARGB;  break;
        case 48:  This->icns_type = kIconServices48PixelDataARGB;  break;
        case 128: This->icns_type = kIconServices128PixelDataARGB; break;
        case 256: This->icns_type = kIconServices256PixelDataARGB; break;
        case 512: This->icns_type = kIconServices512PixelDataARGB; break;
        default:
            ERR("cannot generate ICNS icon from %dx%d image\n", frame->width, frame->height);
            return E_INVALIDARG;
    }
    This->icns_image = malloc(frame->width * frame->height * 4);
    if (!This->icns_image)
    {
        WARN("failed to allocate image buffer\n");
        return E_FAIL;
    }
    This->lines_written = 0;

    return S_OK;
}

static HRESULT CDECL IcnsEncoder_write_lines(struct encoder* iface,
    BYTE *data, DWORD line_count, DWORD stride)
{
    IcnsEncoder *This = impl_from_encoder(iface);
    UINT i;

    for (i = 0; i < line_count; i++)
    {
        BYTE *src_row, *dst_row;
        UINT j;
        src_row = data + stride * i;
        dst_row = This->icns_image + (This->lines_written + i)*(This->frame.width*4);
        /* swap bgr -> rgb */
        for (j = 0; j < This->frame.width*4; j += 4)
        {
            dst_row[j] = src_row[j+3];
            dst_row[j+1] = src_row[j+2];
            dst_row[j+2] = src_row[j+1];
            dst_row[j+3] = src_row[j];
        }
    }
    This->lines_written += line_count;

    return S_OK;
}

static HRESULT CDECL IcnsEncoder_commit_frame(struct encoder *iface)
{
    IcnsEncoder *This = impl_from_encoder(iface);
    Handle handle;
    OSErr ret;

    ret = PtrToHand(This->icns_image, &handle, This->frame.width * This->frame.height * 4);
    if (ret != noErr || !handle)
    {
        WARN("PtrToHand failed with error %d\n", ret);
        return E_FAIL;
    }

    ret = SetIconFamilyData(This->icns_family, This->icns_type, handle);
    DisposeHandle(handle);

    if (ret != noErr)
	{
        WARN("SetIconFamilyData failed for image with error %d\n", ret);
        return E_FAIL;
	}

    free(This->icns_image);
    This->icns_image = NULL;

    return S_OK;
}

static HRESULT CDECL IcnsEncoder_initialize(struct encoder *iface, IStream *stream)
{
    IcnsEncoder *This = impl_from_encoder(iface);

    This->icns_family = (IconFamilyHandle)NewHandle(0);
    if (!This->icns_family)
    {
        WARN("error creating icns family\n");
        return E_FAIL;
    }

    This->stream = stream;

    return S_OK;
}

static HRESULT CDECL IcnsEncoder_commit_file(struct encoder *iface)
{
    IcnsEncoder *This = impl_from_encoder(iface);
    size_t buffer_size;
    HRESULT hr = S_OK;
    ULONG byteswritten;

    buffer_size = GetHandleSize((Handle)This->icns_family);
    hr = stream_write(This->stream, *This->icns_family, buffer_size, &byteswritten);
    if (FAILED(hr) || byteswritten != buffer_size)
    {
        WARN("writing file failed, hr = 0x%08X\n", hr);
        return E_FAIL;
    }

    return S_OK;
}

static const struct encoder_funcs IcnsEncoder_vtable = {
    IcnsEncoder_initialize,
    IcnsEncoder_get_supported_format,
    IcnsEncoder_create_frame,
    IcnsEncoder_write_lines,
    IcnsEncoder_commit_frame,
    IcnsEncoder_commit_file,
    IcnsEncoder_destroy
};

HRESULT CDECL icns_encoder_create(struct encoder_info *info, struct encoder **result)
{
    IcnsEncoder *This;

    TRACE("\n");

    This = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(IcnsEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->encoder.vtable = &IcnsEncoder_vtable;
    This->stream = NULL;
    This->icns_family = NULL;
    This->icns_image = NULL;

    *result = &This->encoder;
    info->flags = ENCODER_FLAGS_MULTI_FRAME|ENCODER_FLAGS_ICNS_SIZE;
    info->container_format = GUID_WineContainerFormatIcns;
    info->clsid = CLSID_WICIcnsEncoder;
    info->encoder_options[0] = ENCODER_OPTION_END;

    return S_OK;
}

#else /* !defined(HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H) ||
         MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4 */

HRESULT CDECL icns_encoder_create(struct encoder_info *info, struct encoder **result)
{
    ERR("Trying to save ICNS picture, but ICNS support is not compiled in.\n");
    return E_FAIL;
}

#endif
