/*
 * Copyright (C) 2007 Google (Evan Stade)
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#define COBJMACROS
#include "objbase.h"
#include "olectl.h"
#include "ole2.h"

#include "gdiplus.h"
#include "gdiplus_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

typedef void ImageItemData;

GpStatus WINGDIPAPI GdipCreateBitmapFromScan0(INT width, INT height, INT stride,
    PixelFormat format, BYTE* scan0, GpBitmap** bitmap)
{
    BITMAPFILEHEADER *bmfh;
    BITMAPINFOHEADER *bmih;
    BYTE *buff;
    INT datalen = stride * height, size;
    IStream *stream;

    TRACE("%d %d %d %d %p %p\n", width, height, stride, format, scan0, bitmap);

    if(!scan0 || !bitmap)
        return InvalidParameter;

    *bitmap = GdipAlloc(sizeof(GpBitmap));
    if(!*bitmap)    return OutOfMemory;

    size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + datalen;
    buff = GdipAlloc(size);
    if(!buff){
        GdipFree(*bitmap);
        return OutOfMemory;
    }

    bmfh = (BITMAPFILEHEADER*) buff;
    bmih = (BITMAPINFOHEADER*) (bmfh + 1);

    bmfh->bfType    = (((WORD)'M') << 8) + (WORD)'B';
    bmfh->bfSize    = size;
    bmfh->bfOffBits = size - datalen;

    bmih->biSize            = sizeof(BITMAPINFOHEADER);
    bmih->biWidth           = width;
    bmih->biHeight          = height;
    /* FIXME: use the rest of the data from format */
    bmih->biBitCount        = format >> 8;
    bmih->biCompression     = BI_RGB;

    memcpy(bmih + 1, scan0, datalen);

    if(CreateStreamOnHGlobal(buff, TRUE, &stream) != S_OK){
        ERR("could not make stream\n");
        GdipFree(*bitmap);
        GdipFree(buff);
        return GenericError;
    }

    if(OleLoadPicture(stream, 0, FALSE, &IID_IPicture,
        (LPVOID*) &((*bitmap)->image.picture)) != S_OK){
        TRACE("Could not load picture\n");
        IStream_Release(stream);
        GdipFree(*bitmap);
        GdipFree(buff);
        return GenericError;
    }

    (*bitmap)->image.type = ImageTypeBitmap;

    return Ok;
}

GpStatus WINGDIPAPI GdipCreateBitmapFromStreamICM(IStream* stream,
    GpBitmap **bitmap)
{
    GpStatus stat;

    stat = GdipLoadImageFromStreamICM(stream, (GpImage**) bitmap);

    if(stat == Ok)
        (*bitmap)->image.type = ImageTypeBitmap;

    return stat;
}

GpStatus WINGDIPAPI GdipDisposeImage(GpImage *image)
{
    if(!image)
        return InvalidParameter;

    IPicture_Release(image->picture);
    GdipFree(image);

    return Ok;
}

GpStatus WINGDIPAPI GdipFindFirstImageItem(GpImage *image, ImageItemData* item)
{
    if(!image || !item)
        return InvalidParameter;

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageBounds(GpImage *image, GpRectF *srcRect,
    GpUnit *srcUnit)
{
    if(!image || !srcRect || !srcUnit)
        return InvalidParameter;
    if(image->type == ImageTypeMetafile){
        memcpy(srcRect, &((GpMetafile*)image)->bounds, sizeof(GpRectF));
        *srcUnit = ((GpMetafile*)image)->unit;
    }
    else{
        FIXME("not implemented for bitmaps\n");
        return NotImplemented;
    }

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageHeight(GpImage *image, UINT *height)
{
    static int calls;

    if(!image || !height)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageHorizontalResolution(GpImage *image, REAL *res)
{
    static int calls;

    if(!image || !res)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageRawFormat(GpImage *image, GUID *format)
{
    static int calls;

    if(!image || !format)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageType(GpImage *image, ImageType *type)
{
    if(!image || !type)
        return InvalidParameter;

    *type = image->type;

    return Ok;
}

GpStatus WINGDIPAPI GdipGetImageVerticalResolution(GpImage *image, REAL *res)
{
    static int calls;

    if(!image || !res)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetImageWidth(GpImage *image, UINT *width)
{
    static int calls;

    if(!image || !width)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetMetafileHeaderFromMetafile(GpMetafile * metafile,
    MetafileHeader * header)
{
    static int calls;

    if(!metafile || !header)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipGetPropertyItemSize(GpImage *image, PROPID pid,
    UINT* size)
{
    static int calls;

    TRACE("%p %x %p\n", image, pid, size);

    if(!size || !image)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

GpStatus WINGDIPAPI GdipImageGetFrameCount(GpImage *image,
    GDIPCONST GUID* dimensionID, UINT* count)
{
    static int calls;

    if(!image || !dimensionID || !count)
        return InvalidParameter;

    if(!(calls++))
        FIXME("not implemented\n");

    return NotImplemented;
}

/* FIXME: no ICM */
GpStatus WINGDIPAPI GdipLoadImageFromStreamICM(IStream* stream, GpImage **image)
{
    if(!stream || !image)
        return InvalidParameter;

    *image = GdipAlloc(sizeof(GpImage));
    if(!*image) return OutOfMemory;

    if(OleLoadPicture(stream, 0, FALSE, &IID_IPicture,
        (LPVOID*) &((*image)->picture)) != S_OK){
        TRACE("Could not load picture\n");
        GdipFree(*image);
        return GenericError;
    }

    /* FIXME: use IPicture_get_Type to get image type */
    (*image)->type = ImageTypeUnknown;

    return Ok;
}

GpStatus WINGDIPAPI GdipSaveImageToStream(GpImage *image, IStream* stream,
    GDIPCONST CLSID* clsid, GDIPCONST EncoderParameters* params)
{
    if(!image || !stream)
        return InvalidParameter;

    /* FIXME: CLSID, EncoderParameters not used */

    IPicture_SaveAsFile(image->picture, stream, FALSE, NULL);

    return Ok;
}
