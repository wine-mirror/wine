/*
 * Big endian structure conversion routines
 *
 * Copyright Miguel de Icaza, 1994
 */

#include "arch.h"
#include "windows.h"

void ARCH_ConvBitmapInfo (BITMAPINFOHEADER *image)
{
    image->biSize = CONV_LONG (image->biSize);
    image->biWidth = CONV_LONG (image->biWidth);
    image->biHeight = CONV_LONG (image->biHeight);
    image->biPlanes = CONV_SHORT (image->biPlanes);
    image->biBitCount = CONV_SHORT (image->biBitCount);
    image->biCompression = CONV_LONG (image->biCompression);
    image->biSizeImage = CONV_LONG (image->biSizeImage);
    image->biXPelsPerMeter = CONV_LONG (image->biXPelsPerMeter);
    image->biYPelsPerMeter = CONV_LONG (image->biYPelsPerMeter);
    image->biClrUsed = CONV_LONG (image->biClrUsed);
    image->biClrImportant = CONV_LONG (image->biClrImportant);
}

void ARCH_ConvCoreHeader (BITMAPCOREHEADER *image)
{
    image->bcSize = CONV_LONG (image->bcSize);
    image->bcWidth = CONV_SHORT (image->bcWidth);
    image->bcHeight = CONV_SHORT (image->bcHeight);
    image->bcPlanes = CONV_SHORT (image->bcPlanes);
    image->bcBitCount = CONV_SHORT (image->bcBitCount);
}
