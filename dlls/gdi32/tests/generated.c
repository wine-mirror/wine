/* File generated automatically from tools/winapi/tests.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#define WINVER 0x0501
#define _WIN32_IE 0x0501
#define _WIN32_WINNT 0x0501

#define WINE_NOWINSOCK

#include "windows.h"

#include "wine/test.h"

/***********************************************************************
 * Compatibility macros
 */

#define DWORD_PTR UINT_PTR
#define LONG_PTR INT_PTR
#define ULONG_PTR UINT_PTR

/***********************************************************************
 * Windows API extension
 */

#if defined(_MSC_VER) && (_MSC_VER >= 1300) && defined(__cplusplus)
# define _TYPE_ALIGNMENT(type) __alignof(type)
#elif defined(__GNUC__)
# define _TYPE_ALIGNMENT(type) __alignof__(type)
#else
/*
 * FIXME: May not be possible without a compiler extension
 *        (if type is not just a name that is, otherwise the normal
 *         TYPE_ALIGNMENT can be used)
 */
#endif

#if defined(TYPE_ALIGNMENT) && defined(_MSC_VER) && _MSC_VER >= 800 && !defined(__cplusplus)
#pragma warning(disable:4116)
#endif

#if !defined(TYPE_ALIGNMENT) && defined(_TYPE_ALIGNMENT)
# define TYPE_ALIGNMENT _TYPE_ALIGNMENT
#endif

/***********************************************************************
 * Test helper macros
 */

#ifdef FIELD_ALIGNMENT
# define TEST_FIELD_ALIGNMENT(type, field, align) \
   ok(_TYPE_ALIGNMENT(((type*)0)->field) == align, \
       "FIELD_ALIGNMENT(" #type ", " #field ") == %d (expected " #align ")\n", \
           (int)_TYPE_ALIGNMENT(((type*)0)->field))
#else
# define TEST_FIELD_ALIGNMENT(type, field, align) do { } while (0)
#endif

#define TEST_FIELD_OFFSET(type, field, offset) \
    ok(FIELD_OFFSET(type, field) == offset, \
        "FIELD_OFFSET(" #type ", " #field ") == %ld (expected " #offset ")\n", \
             (long int)FIELD_OFFSET(type, field))

#ifdef _TYPE_ALIGNMENT
#define TEST__TYPE_ALIGNMENT(type, align) \
    ok(_TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)_TYPE_ALIGNMENT(type))
#else
# define TEST__TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#ifdef TYPE_ALIGNMENT
#define TEST_TYPE_ALIGNMENT(type, align) \
    ok(TYPE_ALIGNMENT(type) == align, "TYPE_ALIGNMENT(" #type ") == %d (expected " #align ")\n", (int)TYPE_ALIGNMENT(type))
#else
# define TEST_TYPE_ALIGNMENT(type, align) do { } while (0)
#endif

#define TEST_TYPE_SIZE(type, size) \
    ok(sizeof(type) == size, "sizeof(" #type ") == %d (expected " #size ")\n", ((int) sizeof(type)))

/***********************************************************************
 * Test macros
 */

#define TEST_FIELD(type, field, field_offset, field_size, field_align) \
  TEST_TYPE_SIZE((((type*)0)->field), field_size); \
  TEST_FIELD_ALIGNMENT(type, field, field_align); \
  TEST_FIELD_OFFSET(type, field, field_offset)

#define TEST_TYPE(type, size, align) \
  TEST_TYPE_ALIGNMENT(type, align); \
  TEST_TYPE_SIZE(type, size)

#define TEST_TYPE_POINTER(type, size, align) \
    TEST__TYPE_ALIGNMENT(*(type)0, align); \
    TEST_TYPE_SIZE(*(type)0, size)

#define TEST_TYPE_SIGNED(type) \
    ok((type) -1 < 0, "(" #type ") -1 < 0\n");

#define TEST_TYPE_UNSIGNED(type) \
     ok((type) -1 > 0, "(" #type ") -1 > 0\n");

static void test_pack_ABC(void)
{
    /* ABC (pack 4) */
    TEST_TYPE(ABC, 12, 4);
    TEST_FIELD(ABC, abcA, 0, 4, 4);
    TEST_FIELD(ABC, abcB, 4, 4, 4);
    TEST_FIELD(ABC, abcC, 8, 4, 4);
}

static void test_pack_ABCFLOAT(void)
{
    /* ABCFLOAT (pack 4) */
    TEST_TYPE(ABCFLOAT, 12, 4);
    TEST_FIELD(ABCFLOAT, abcfA, 0, 4, 4);
    TEST_FIELD(ABCFLOAT, abcfB, 4, 4, 4);
    TEST_FIELD(ABCFLOAT, abcfC, 8, 4, 4);
}

static void test_pack_ABORTPROC(void)
{
    /* ABORTPROC */
    TEST_TYPE(ABORTPROC, 4, 4);
}

static void test_pack_BITMAP(void)
{
    /* BITMAP (pack 4) */
    TEST_TYPE(BITMAP, 24, 4);
    TEST_FIELD(BITMAP, bmType, 0, 4, 4);
    TEST_FIELD(BITMAP, bmWidth, 4, 4, 4);
    TEST_FIELD(BITMAP, bmHeight, 8, 4, 4);
    TEST_FIELD(BITMAP, bmWidthBytes, 12, 4, 4);
    TEST_FIELD(BITMAP, bmPlanes, 16, 2, 2);
    TEST_FIELD(BITMAP, bmBitsPixel, 18, 2, 2);
    TEST_FIELD(BITMAP, bmBits, 20, 4, 4);
}

static void test_pack_BITMAPCOREHEADER(void)
{
    /* BITMAPCOREHEADER (pack 4) */
    TEST_TYPE(BITMAPCOREHEADER, 12, 4);
    TEST_FIELD(BITMAPCOREHEADER, bcSize, 0, 4, 4);
    TEST_FIELD(BITMAPCOREHEADER, bcWidth, 4, 2, 2);
    TEST_FIELD(BITMAPCOREHEADER, bcHeight, 6, 2, 2);
    TEST_FIELD(BITMAPCOREHEADER, bcPlanes, 8, 2, 2);
    TEST_FIELD(BITMAPCOREHEADER, bcBitCount, 10, 2, 2);
}

static void test_pack_BITMAPCOREINFO(void)
{
    /* BITMAPCOREINFO (pack 4) */
    TEST_TYPE(BITMAPCOREINFO, 16, 4);
    TEST_FIELD(BITMAPCOREINFO, bmciHeader, 0, 12, 4);
    TEST_FIELD(BITMAPCOREINFO, bmciColors, 12, 3, 1);
}

static void test_pack_BITMAPFILEHEADER(void)
{
    /* BITMAPFILEHEADER (pack 2) */
    TEST_TYPE(BITMAPFILEHEADER, 14, 2);
    TEST_FIELD(BITMAPFILEHEADER, bfType, 0, 2, 2);
    TEST_FIELD(BITMAPFILEHEADER, bfSize, 2, 4, 2);
    TEST_FIELD(BITMAPFILEHEADER, bfReserved1, 6, 2, 2);
    TEST_FIELD(BITMAPFILEHEADER, bfReserved2, 8, 2, 2);
    TEST_FIELD(BITMAPFILEHEADER, bfOffBits, 10, 4, 2);
}

static void test_pack_BITMAPINFO(void)
{
    /* BITMAPINFO (pack 4) */
    TEST_TYPE(BITMAPINFO, 44, 4);
    TEST_FIELD(BITMAPINFO, bmiHeader, 0, 40, 4);
    TEST_FIELD(BITMAPINFO, bmiColors, 40, 4, 1);
}

static void test_pack_BITMAPINFOHEADER(void)
{
    /* BITMAPINFOHEADER (pack 4) */
    TEST_TYPE(BITMAPINFOHEADER, 40, 4);
    TEST_FIELD(BITMAPINFOHEADER, biSize, 0, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, biWidth, 4, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, biHeight, 8, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, biPlanes, 12, 2, 2);
    TEST_FIELD(BITMAPINFOHEADER, biBitCount, 14, 2, 2);
    TEST_FIELD(BITMAPINFOHEADER, biCompression, 16, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, biSizeImage, 20, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, biXPelsPerMeter, 24, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, biYPelsPerMeter, 28, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, biClrUsed, 32, 4, 4);
    TEST_FIELD(BITMAPINFOHEADER, biClrImportant, 36, 4, 4);
}

static void test_pack_BITMAPV4HEADER(void)
{
    /* BITMAPV4HEADER (pack 4) */
    TEST_TYPE(BITMAPV4HEADER, 108, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4Size, 0, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4Width, 4, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4Height, 8, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4Planes, 12, 2, 2);
    TEST_FIELD(BITMAPV4HEADER, bV4BitCount, 14, 2, 2);
    TEST_FIELD(BITMAPV4HEADER, bV4V4Compression, 16, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4SizeImage, 20, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4XPelsPerMeter, 24, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4YPelsPerMeter, 28, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4ClrUsed, 32, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4ClrImportant, 36, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4RedMask, 40, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4GreenMask, 44, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4BlueMask, 48, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4AlphaMask, 52, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4CSType, 56, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4Endpoints, 60, 36, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4GammaRed, 96, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4GammaGreen, 100, 4, 4);
    TEST_FIELD(BITMAPV4HEADER, bV4GammaBlue, 104, 4, 4);
}

static void test_pack_BITMAPV5HEADER(void)
{
    /* BITMAPV5HEADER (pack 4) */
    TEST_TYPE(BITMAPV5HEADER, 124, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5Size, 0, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5Width, 4, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5Height, 8, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5Planes, 12, 2, 2);
    TEST_FIELD(BITMAPV5HEADER, bV5BitCount, 14, 2, 2);
    TEST_FIELD(BITMAPV5HEADER, bV5Compression, 16, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5SizeImage, 20, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5XPelsPerMeter, 24, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5YPelsPerMeter, 28, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5ClrUsed, 32, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5ClrImportant, 36, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5RedMask, 40, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5GreenMask, 44, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5BlueMask, 48, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5AlphaMask, 52, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5CSType, 56, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5Endpoints, 60, 36, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5GammaRed, 96, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5GammaGreen, 100, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5GammaBlue, 104, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5Intent, 108, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5ProfileData, 112, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5ProfileSize, 116, 4, 4);
    TEST_FIELD(BITMAPV5HEADER, bV5Reserved, 120, 4, 4);
}

static void test_pack_BLENDFUNCTION(void)
{
    /* BLENDFUNCTION (pack 4) */
    TEST_TYPE(BLENDFUNCTION, 4, 1);
    TEST_FIELD(BLENDFUNCTION, BlendOp, 0, 1, 1);
    TEST_FIELD(BLENDFUNCTION, BlendFlags, 1, 1, 1);
    TEST_FIELD(BLENDFUNCTION, SourceConstantAlpha, 2, 1, 1);
    TEST_FIELD(BLENDFUNCTION, AlphaFormat, 3, 1, 1);
}

static void test_pack_CHARSETINFO(void)
{
    /* CHARSETINFO (pack 4) */
    TEST_TYPE(CHARSETINFO, 32, 4);
    TEST_FIELD(CHARSETINFO, ciCharset, 0, 4, 4);
    TEST_FIELD(CHARSETINFO, ciACP, 4, 4, 4);
    TEST_FIELD(CHARSETINFO, fs, 8, 24, 4);
}

static void test_pack_CIEXYZ(void)
{
    /* CIEXYZ (pack 4) */
    TEST_TYPE(CIEXYZ, 12, 4);
    TEST_FIELD(CIEXYZ, ciexyzX, 0, 4, 4);
    TEST_FIELD(CIEXYZ, ciexyzY, 4, 4, 4);
    TEST_FIELD(CIEXYZ, ciexyzZ, 8, 4, 4);
}

static void test_pack_CIEXYZTRIPLE(void)
{
    /* CIEXYZTRIPLE (pack 4) */
    TEST_TYPE(CIEXYZTRIPLE, 36, 4);
    TEST_FIELD(CIEXYZTRIPLE, ciexyzRed, 0, 12, 4);
    TEST_FIELD(CIEXYZTRIPLE, ciexyzGreen, 12, 12, 4);
    TEST_FIELD(CIEXYZTRIPLE, ciexyzBlue, 24, 12, 4);
}

static void test_pack_COLOR16(void)
{
    /* COLOR16 */
    TEST_TYPE(COLOR16, 2, 2);
}

static void test_pack_COLORADJUSTMENT(void)
{
    /* COLORADJUSTMENT (pack 4) */
    TEST_TYPE(COLORADJUSTMENT, 24, 2);
    TEST_FIELD(COLORADJUSTMENT, caSize, 0, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caFlags, 2, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caIlluminantIndex, 4, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caRedGamma, 6, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caGreenGamma, 8, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caBlueGamma, 10, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caReferenceBlack, 12, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caReferenceWhite, 14, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caContrast, 16, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caBrightness, 18, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caColorfulness, 20, 2, 2);
    TEST_FIELD(COLORADJUSTMENT, caRedGreenTint, 22, 2, 2);
}

static void test_pack_DEVMODEA(void)
{
    /* DEVMODEA (pack 4) */
    TEST_FIELD(DEVMODEA, dmDeviceName, 0, 32, 1);
    TEST_FIELD(DEVMODEA, dmSpecVersion, 32, 2, 2);
    TEST_FIELD(DEVMODEA, dmDriverVersion, 34, 2, 2);
    TEST_FIELD(DEVMODEA, dmSize, 36, 2, 2);
    TEST_FIELD(DEVMODEA, dmDriverExtra, 38, 2, 2);
    TEST_FIELD(DEVMODEA, dmFields, 40, 4, 4);
}

static void test_pack_DEVMODEW(void)
{
    /* DEVMODEW (pack 4) */
    TEST_FIELD(DEVMODEW, dmDeviceName, 0, 64, 2);
    TEST_FIELD(DEVMODEW, dmSpecVersion, 64, 2, 2);
    TEST_FIELD(DEVMODEW, dmDriverVersion, 66, 2, 2);
    TEST_FIELD(DEVMODEW, dmSize, 68, 2, 2);
    TEST_FIELD(DEVMODEW, dmDriverExtra, 70, 2, 2);
    TEST_FIELD(DEVMODEW, dmFields, 72, 4, 4);
}

static void test_pack_DIBSECTION(void)
{
    /* DIBSECTION (pack 4) */
    TEST_TYPE(DIBSECTION, 84, 4);
    TEST_FIELD(DIBSECTION, dsBm, 0, 24, 4);
    TEST_FIELD(DIBSECTION, dsBmih, 24, 40, 4);
    TEST_FIELD(DIBSECTION, dsBitfields, 64, 12, 4);
    TEST_FIELD(DIBSECTION, dshSection, 76, 4, 4);
    TEST_FIELD(DIBSECTION, dsOffset, 80, 4, 4);
}

static void test_pack_DISPLAY_DEVICEA(void)
{
    /* DISPLAY_DEVICEA (pack 4) */
    TEST_TYPE(DISPLAY_DEVICEA, 424, 4);
    TEST_FIELD(DISPLAY_DEVICEA, cb, 0, 4, 4);
    TEST_FIELD(DISPLAY_DEVICEA, DeviceName, 4, 32, 1);
    TEST_FIELD(DISPLAY_DEVICEA, DeviceString, 36, 128, 1);
    TEST_FIELD(DISPLAY_DEVICEA, StateFlags, 164, 4, 4);
    TEST_FIELD(DISPLAY_DEVICEA, DeviceID, 168, 128, 1);
    TEST_FIELD(DISPLAY_DEVICEA, DeviceKey, 296, 128, 1);
}

static void test_pack_DISPLAY_DEVICEW(void)
{
    /* DISPLAY_DEVICEW (pack 4) */
    TEST_TYPE(DISPLAY_DEVICEW, 840, 4);
    TEST_FIELD(DISPLAY_DEVICEW, cb, 0, 4, 4);
    TEST_FIELD(DISPLAY_DEVICEW, DeviceName, 4, 64, 2);
    TEST_FIELD(DISPLAY_DEVICEW, DeviceString, 68, 256, 2);
    TEST_FIELD(DISPLAY_DEVICEW, StateFlags, 324, 4, 4);
    TEST_FIELD(DISPLAY_DEVICEW, DeviceID, 328, 256, 2);
    TEST_FIELD(DISPLAY_DEVICEW, DeviceKey, 584, 256, 2);
}

static void test_pack_DOCINFOA(void)
{
    /* DOCINFOA (pack 4) */
    TEST_TYPE(DOCINFOA, 20, 4);
    TEST_FIELD(DOCINFOA, cbSize, 0, 4, 4);
    TEST_FIELD(DOCINFOA, lpszDocName, 4, 4, 4);
    TEST_FIELD(DOCINFOA, lpszOutput, 8, 4, 4);
    TEST_FIELD(DOCINFOA, lpszDatatype, 12, 4, 4);
    TEST_FIELD(DOCINFOA, fwType, 16, 4, 4);
}

static void test_pack_DOCINFOW(void)
{
    /* DOCINFOW (pack 4) */
    TEST_TYPE(DOCINFOW, 20, 4);
    TEST_FIELD(DOCINFOW, cbSize, 0, 4, 4);
    TEST_FIELD(DOCINFOW, lpszDocName, 4, 4, 4);
    TEST_FIELD(DOCINFOW, lpszOutput, 8, 4, 4);
    TEST_FIELD(DOCINFOW, lpszDatatype, 12, 4, 4);
    TEST_FIELD(DOCINFOW, fwType, 16, 4, 4);
}

static void test_pack_EMR(void)
{
    /* EMR (pack 4) */
    TEST_TYPE(EMR, 8, 4);
    TEST_FIELD(EMR, iType, 0, 4, 4);
    TEST_FIELD(EMR, nSize, 4, 4, 4);
}

static void test_pack_EMRABORTPATH(void)
{
    /* EMRABORTPATH (pack 4) */
    TEST_TYPE(EMRABORTPATH, 8, 4);
    TEST_FIELD(EMRABORTPATH, emr, 0, 8, 4);
}

static void test_pack_EMRANGLEARC(void)
{
    /* EMRANGLEARC (pack 4) */
    TEST_TYPE(EMRANGLEARC, 28, 4);
    TEST_FIELD(EMRANGLEARC, emr, 0, 8, 4);
    TEST_FIELD(EMRANGLEARC, ptlCenter, 8, 8, 4);
    TEST_FIELD(EMRANGLEARC, nRadius, 16, 4, 4);
    TEST_FIELD(EMRANGLEARC, eStartAngle, 20, 4, 4);
    TEST_FIELD(EMRANGLEARC, eSweepAngle, 24, 4, 4);
}

static void test_pack_EMRARC(void)
{
    /* EMRARC (pack 4) */
    TEST_TYPE(EMRARC, 40, 4);
    TEST_FIELD(EMRARC, emr, 0, 8, 4);
    TEST_FIELD(EMRARC, rclBox, 8, 16, 4);
    TEST_FIELD(EMRARC, ptlStart, 24, 8, 4);
    TEST_FIELD(EMRARC, ptlEnd, 32, 8, 4);
}

static void test_pack_EMRARCTO(void)
{
    /* EMRARCTO (pack 4) */
    TEST_TYPE(EMRARCTO, 40, 4);
    TEST_FIELD(EMRARCTO, emr, 0, 8, 4);
    TEST_FIELD(EMRARCTO, rclBox, 8, 16, 4);
    TEST_FIELD(EMRARCTO, ptlStart, 24, 8, 4);
    TEST_FIELD(EMRARCTO, ptlEnd, 32, 8, 4);
}

static void test_pack_EMRBEGINPATH(void)
{
    /* EMRBEGINPATH (pack 4) */
    TEST_TYPE(EMRBEGINPATH, 8, 4);
    TEST_FIELD(EMRBEGINPATH, emr, 0, 8, 4);
}

static void test_pack_EMRBITBLT(void)
{
    /* EMRBITBLT (pack 4) */
    TEST_TYPE(EMRBITBLT, 100, 4);
    TEST_FIELD(EMRBITBLT, emr, 0, 8, 4);
    TEST_FIELD(EMRBITBLT, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRBITBLT, xDest, 24, 4, 4);
    TEST_FIELD(EMRBITBLT, yDest, 28, 4, 4);
    TEST_FIELD(EMRBITBLT, cxDest, 32, 4, 4);
    TEST_FIELD(EMRBITBLT, cyDest, 36, 4, 4);
    TEST_FIELD(EMRBITBLT, dwRop, 40, 4, 4);
    TEST_FIELD(EMRBITBLT, xSrc, 44, 4, 4);
    TEST_FIELD(EMRBITBLT, ySrc, 48, 4, 4);
    TEST_FIELD(EMRBITBLT, xformSrc, 52, 24, 4);
    TEST_FIELD(EMRBITBLT, crBkColorSrc, 76, 4, 4);
    TEST_FIELD(EMRBITBLT, iUsageSrc, 80, 4, 4);
    TEST_FIELD(EMRBITBLT, offBmiSrc, 84, 4, 4);
    TEST_FIELD(EMRBITBLT, cbBmiSrc, 88, 4, 4);
    TEST_FIELD(EMRBITBLT, offBitsSrc, 92, 4, 4);
    TEST_FIELD(EMRBITBLT, cbBitsSrc, 96, 4, 4);
}

static void test_pack_EMRCHORD(void)
{
    /* EMRCHORD (pack 4) */
    TEST_TYPE(EMRCHORD, 40, 4);
    TEST_FIELD(EMRCHORD, emr, 0, 8, 4);
    TEST_FIELD(EMRCHORD, rclBox, 8, 16, 4);
    TEST_FIELD(EMRCHORD, ptlStart, 24, 8, 4);
    TEST_FIELD(EMRCHORD, ptlEnd, 32, 8, 4);
}

static void test_pack_EMRCLOSEFIGURE(void)
{
    /* EMRCLOSEFIGURE (pack 4) */
    TEST_TYPE(EMRCLOSEFIGURE, 8, 4);
    TEST_FIELD(EMRCLOSEFIGURE, emr, 0, 8, 4);
}

static void test_pack_EMRCREATEBRUSHINDIRECT(void)
{
    /* EMRCREATEBRUSHINDIRECT (pack 4) */
    TEST_TYPE(EMRCREATEBRUSHINDIRECT, 24, 4);
    TEST_FIELD(EMRCREATEBRUSHINDIRECT, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATEBRUSHINDIRECT, ihBrush, 8, 4, 4);
    TEST_FIELD(EMRCREATEBRUSHINDIRECT, lb, 12, 12, 4);
}

static void test_pack_EMRCREATECOLORSPACE(void)
{
    /* EMRCREATECOLORSPACE (pack 4) */
    TEST_TYPE(EMRCREATECOLORSPACE, 340, 4);
    TEST_FIELD(EMRCREATECOLORSPACE, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATECOLORSPACE, ihCS, 8, 4, 4);
    TEST_FIELD(EMRCREATECOLORSPACE, lcs, 12, 328, 4);
}

static void test_pack_EMRCREATECOLORSPACEW(void)
{
    /* EMRCREATECOLORSPACEW (pack 4) */
    TEST_TYPE(EMRCREATECOLORSPACEW, 612, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, ihCS, 8, 4, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, lcs, 12, 588, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, dwFlags, 600, 4, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, cbData, 604, 4, 4);
    TEST_FIELD(EMRCREATECOLORSPACEW, Data, 608, 1, 1);
}

static void test_pack_EMRCREATEDIBPATTERNBRUSHPT(void)
{
    /* EMRCREATEDIBPATTERNBRUSHPT (pack 4) */
    TEST_TYPE(EMRCREATEDIBPATTERNBRUSHPT, 32, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, ihBrush, 8, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, iUsage, 12, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, offBmi, 16, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, cbBmi, 20, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, offBits, 24, 4, 4);
    TEST_FIELD(EMRCREATEDIBPATTERNBRUSHPT, cbBits, 28, 4, 4);
}

static void test_pack_EMRCREATEMONOBRUSH(void)
{
    /* EMRCREATEMONOBRUSH (pack 4) */
    TEST_TYPE(EMRCREATEMONOBRUSH, 32, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, ihBrush, 8, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, iUsage, 12, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, offBmi, 16, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, cbBmi, 20, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, offBits, 24, 4, 4);
    TEST_FIELD(EMRCREATEMONOBRUSH, cbBits, 28, 4, 4);
}

static void test_pack_EMRCREATEPEN(void)
{
    /* EMRCREATEPEN (pack 4) */
    TEST_TYPE(EMRCREATEPEN, 28, 4);
    TEST_FIELD(EMRCREATEPEN, emr, 0, 8, 4);
    TEST_FIELD(EMRCREATEPEN, ihPen, 8, 4, 4);
    TEST_FIELD(EMRCREATEPEN, lopn, 12, 16, 4);
}

static void test_pack_EMRDELETECOLORSPACE(void)
{
    /* EMRDELETECOLORSPACE (pack 4) */
    TEST_TYPE(EMRDELETECOLORSPACE, 12, 4);
    TEST_FIELD(EMRDELETECOLORSPACE, emr, 0, 8, 4);
    TEST_FIELD(EMRDELETECOLORSPACE, ihCS, 8, 4, 4);
}

static void test_pack_EMRDELETEOBJECT(void)
{
    /* EMRDELETEOBJECT (pack 4) */
    TEST_TYPE(EMRDELETEOBJECT, 12, 4);
    TEST_FIELD(EMRDELETEOBJECT, emr, 0, 8, 4);
    TEST_FIELD(EMRDELETEOBJECT, ihObject, 8, 4, 4);
}

static void test_pack_EMRELLIPSE(void)
{
    /* EMRELLIPSE (pack 4) */
    TEST_TYPE(EMRELLIPSE, 24, 4);
    TEST_FIELD(EMRELLIPSE, emr, 0, 8, 4);
    TEST_FIELD(EMRELLIPSE, rclBox, 8, 16, 4);
}

static void test_pack_EMRENDPATH(void)
{
    /* EMRENDPATH (pack 4) */
    TEST_TYPE(EMRENDPATH, 8, 4);
    TEST_FIELD(EMRENDPATH, emr, 0, 8, 4);
}

static void test_pack_EMREOF(void)
{
    /* EMREOF (pack 4) */
    TEST_TYPE(EMREOF, 20, 4);
    TEST_FIELD(EMREOF, emr, 0, 8, 4);
    TEST_FIELD(EMREOF, nPalEntries, 8, 4, 4);
    TEST_FIELD(EMREOF, offPalEntries, 12, 4, 4);
    TEST_FIELD(EMREOF, nSizeLast, 16, 4, 4);
}

static void test_pack_EMREXCLUDECLIPRECT(void)
{
    /* EMREXCLUDECLIPRECT (pack 4) */
    TEST_TYPE(EMREXCLUDECLIPRECT, 24, 4);
    TEST_FIELD(EMREXCLUDECLIPRECT, emr, 0, 8, 4);
    TEST_FIELD(EMREXCLUDECLIPRECT, rclClip, 8, 16, 4);
}

static void test_pack_EMREXTCREATEFONTINDIRECTW(void)
{
    /* EMREXTCREATEFONTINDIRECTW (pack 4) */
    TEST_TYPE(EMREXTCREATEFONTINDIRECTW, 332, 4);
    TEST_FIELD(EMREXTCREATEFONTINDIRECTW, emr, 0, 8, 4);
    TEST_FIELD(EMREXTCREATEFONTINDIRECTW, ihFont, 8, 4, 4);
    TEST_FIELD(EMREXTCREATEFONTINDIRECTW, elfw, 12, 320, 4);
}

static void test_pack_EMREXTCREATEPEN(void)
{
    /* EMREXTCREATEPEN (pack 4) */
    TEST_TYPE(EMREXTCREATEPEN, 56, 4);
    TEST_FIELD(EMREXTCREATEPEN, emr, 0, 8, 4);
    TEST_FIELD(EMREXTCREATEPEN, ihPen, 8, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, offBmi, 12, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, cbBmi, 16, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, offBits, 20, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, cbBits, 24, 4, 4);
    TEST_FIELD(EMREXTCREATEPEN, elp, 28, 28, 4);
}

static void test_pack_EMREXTFLOODFILL(void)
{
    /* EMREXTFLOODFILL (pack 4) */
    TEST_TYPE(EMREXTFLOODFILL, 24, 4);
    TEST_FIELD(EMREXTFLOODFILL, emr, 0, 8, 4);
    TEST_FIELD(EMREXTFLOODFILL, ptlStart, 8, 8, 4);
    TEST_FIELD(EMREXTFLOODFILL, crColor, 16, 4, 4);
    TEST_FIELD(EMREXTFLOODFILL, iMode, 20, 4, 4);
}

static void test_pack_EMREXTSELECTCLIPRGN(void)
{
    /* EMREXTSELECTCLIPRGN (pack 4) */
    TEST_TYPE(EMREXTSELECTCLIPRGN, 20, 4);
    TEST_FIELD(EMREXTSELECTCLIPRGN, emr, 0, 8, 4);
    TEST_FIELD(EMREXTSELECTCLIPRGN, cbRgnData, 8, 4, 4);
    TEST_FIELD(EMREXTSELECTCLIPRGN, iMode, 12, 4, 4);
    TEST_FIELD(EMREXTSELECTCLIPRGN, RgnData, 16, 1, 1);
}

static void test_pack_EMREXTTEXTOUTA(void)
{
    /* EMREXTTEXTOUTA (pack 4) */
    TEST_TYPE(EMREXTTEXTOUTA, 76, 4);
    TEST_FIELD(EMREXTTEXTOUTA, emr, 0, 8, 4);
    TEST_FIELD(EMREXTTEXTOUTA, rclBounds, 8, 16, 4);
    TEST_FIELD(EMREXTTEXTOUTA, iGraphicsMode, 24, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTA, exScale, 28, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTA, eyScale, 32, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTA, emrtext, 36, 40, 4);
}

static void test_pack_EMREXTTEXTOUTW(void)
{
    /* EMREXTTEXTOUTW (pack 4) */
    TEST_TYPE(EMREXTTEXTOUTW, 76, 4);
    TEST_FIELD(EMREXTTEXTOUTW, emr, 0, 8, 4);
    TEST_FIELD(EMREXTTEXTOUTW, rclBounds, 8, 16, 4);
    TEST_FIELD(EMREXTTEXTOUTW, iGraphicsMode, 24, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTW, exScale, 28, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTW, eyScale, 32, 4, 4);
    TEST_FIELD(EMREXTTEXTOUTW, emrtext, 36, 40, 4);
}

static void test_pack_EMRFILLPATH(void)
{
    /* EMRFILLPATH (pack 4) */
    TEST_TYPE(EMRFILLPATH, 24, 4);
    TEST_FIELD(EMRFILLPATH, emr, 0, 8, 4);
    TEST_FIELD(EMRFILLPATH, rclBounds, 8, 16, 4);
}

static void test_pack_EMRFILLRGN(void)
{
    /* EMRFILLRGN (pack 4) */
    TEST_TYPE(EMRFILLRGN, 36, 4);
    TEST_FIELD(EMRFILLRGN, emr, 0, 8, 4);
    TEST_FIELD(EMRFILLRGN, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRFILLRGN, cbRgnData, 24, 4, 4);
    TEST_FIELD(EMRFILLRGN, ihBrush, 28, 4, 4);
    TEST_FIELD(EMRFILLRGN, RgnData, 32, 1, 1);
}

static void test_pack_EMRFLATTENPATH(void)
{
    /* EMRFLATTENPATH (pack 4) */
    TEST_TYPE(EMRFLATTENPATH, 8, 4);
    TEST_FIELD(EMRFLATTENPATH, emr, 0, 8, 4);
}

static void test_pack_EMRFORMAT(void)
{
    /* EMRFORMAT (pack 4) */
    TEST_TYPE(EMRFORMAT, 16, 4);
    TEST_FIELD(EMRFORMAT, dSignature, 0, 4, 4);
    TEST_FIELD(EMRFORMAT, nVersion, 4, 4, 4);
    TEST_FIELD(EMRFORMAT, cbData, 8, 4, 4);
    TEST_FIELD(EMRFORMAT, offData, 12, 4, 4);
}

static void test_pack_EMRFRAMERGN(void)
{
    /* EMRFRAMERGN (pack 4) */
    TEST_TYPE(EMRFRAMERGN, 44, 4);
    TEST_FIELD(EMRFRAMERGN, emr, 0, 8, 4);
    TEST_FIELD(EMRFRAMERGN, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRFRAMERGN, cbRgnData, 24, 4, 4);
    TEST_FIELD(EMRFRAMERGN, ihBrush, 28, 4, 4);
    TEST_FIELD(EMRFRAMERGN, szlStroke, 32, 8, 4);
    TEST_FIELD(EMRFRAMERGN, RgnData, 40, 1, 1);
}

static void test_pack_EMRGDICOMMENT(void)
{
    /* EMRGDICOMMENT (pack 4) */
    TEST_TYPE(EMRGDICOMMENT, 16, 4);
    TEST_FIELD(EMRGDICOMMENT, emr, 0, 8, 4);
    TEST_FIELD(EMRGDICOMMENT, cbData, 8, 4, 4);
    TEST_FIELD(EMRGDICOMMENT, Data, 12, 1, 1);
}

static void test_pack_EMRGLSBOUNDEDRECORD(void)
{
    /* EMRGLSBOUNDEDRECORD (pack 4) */
    TEST_TYPE(EMRGLSBOUNDEDRECORD, 32, 4);
    TEST_FIELD(EMRGLSBOUNDEDRECORD, emr, 0, 8, 4);
    TEST_FIELD(EMRGLSBOUNDEDRECORD, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRGLSBOUNDEDRECORD, cbData, 24, 4, 4);
    TEST_FIELD(EMRGLSBOUNDEDRECORD, Data, 28, 1, 1);
}

static void test_pack_EMRGLSRECORD(void)
{
    /* EMRGLSRECORD (pack 4) */
    TEST_TYPE(EMRGLSRECORD, 16, 4);
    TEST_FIELD(EMRGLSRECORD, emr, 0, 8, 4);
    TEST_FIELD(EMRGLSRECORD, cbData, 8, 4, 4);
    TEST_FIELD(EMRGLSRECORD, Data, 12, 1, 1);
}

static void test_pack_EMRINTERSECTCLIPRECT(void)
{
    /* EMRINTERSECTCLIPRECT (pack 4) */
    TEST_TYPE(EMRINTERSECTCLIPRECT, 24, 4);
    TEST_FIELD(EMRINTERSECTCLIPRECT, emr, 0, 8, 4);
    TEST_FIELD(EMRINTERSECTCLIPRECT, rclClip, 8, 16, 4);
}

static void test_pack_EMRINVERTRGN(void)
{
    /* EMRINVERTRGN (pack 4) */
    TEST_TYPE(EMRINVERTRGN, 32, 4);
    TEST_FIELD(EMRINVERTRGN, emr, 0, 8, 4);
    TEST_FIELD(EMRINVERTRGN, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRINVERTRGN, cbRgnData, 24, 4, 4);
    TEST_FIELD(EMRINVERTRGN, RgnData, 28, 1, 1);
}

static void test_pack_EMRLINETO(void)
{
    /* EMRLINETO (pack 4) */
    TEST_TYPE(EMRLINETO, 16, 4);
    TEST_FIELD(EMRLINETO, emr, 0, 8, 4);
    TEST_FIELD(EMRLINETO, ptl, 8, 8, 4);
}

static void test_pack_EMRMASKBLT(void)
{
    /* EMRMASKBLT (pack 4) */
    TEST_TYPE(EMRMASKBLT, 128, 4);
    TEST_FIELD(EMRMASKBLT, emr, 0, 8, 4);
    TEST_FIELD(EMRMASKBLT, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRMASKBLT, xDest, 24, 4, 4);
    TEST_FIELD(EMRMASKBLT, yDest, 28, 4, 4);
    TEST_FIELD(EMRMASKBLT, cxDest, 32, 4, 4);
    TEST_FIELD(EMRMASKBLT, cyDest, 36, 4, 4);
    TEST_FIELD(EMRMASKBLT, dwRop, 40, 4, 4);
    TEST_FIELD(EMRMASKBLT, xSrc, 44, 4, 4);
    TEST_FIELD(EMRMASKBLT, ySrc, 48, 4, 4);
    TEST_FIELD(EMRMASKBLT, xformSrc, 52, 24, 4);
    TEST_FIELD(EMRMASKBLT, crBkColorSrc, 76, 4, 4);
    TEST_FIELD(EMRMASKBLT, iUsageSrc, 80, 4, 4);
    TEST_FIELD(EMRMASKBLT, offBmiSrc, 84, 4, 4);
    TEST_FIELD(EMRMASKBLT, cbBmiSrc, 88, 4, 4);
    TEST_FIELD(EMRMASKBLT, offBitsSrc, 92, 4, 4);
    TEST_FIELD(EMRMASKBLT, cbBitsSrc, 96, 4, 4);
    TEST_FIELD(EMRMASKBLT, xMask, 100, 4, 4);
    TEST_FIELD(EMRMASKBLT, yMask, 104, 4, 4);
    TEST_FIELD(EMRMASKBLT, iUsageMask, 108, 4, 4);
    TEST_FIELD(EMRMASKBLT, offBmiMask, 112, 4, 4);
    TEST_FIELD(EMRMASKBLT, cbBmiMask, 116, 4, 4);
    TEST_FIELD(EMRMASKBLT, offBitsMask, 120, 4, 4);
    TEST_FIELD(EMRMASKBLT, cbBitsMask, 124, 4, 4);
}

static void test_pack_EMRMODIFYWORLDTRANSFORM(void)
{
    /* EMRMODIFYWORLDTRANSFORM (pack 4) */
    TEST_TYPE(EMRMODIFYWORLDTRANSFORM, 36, 4);
    TEST_FIELD(EMRMODIFYWORLDTRANSFORM, emr, 0, 8, 4);
    TEST_FIELD(EMRMODIFYWORLDTRANSFORM, xform, 8, 24, 4);
    TEST_FIELD(EMRMODIFYWORLDTRANSFORM, iMode, 32, 4, 4);
}

static void test_pack_EMRMOVETOEX(void)
{
    /* EMRMOVETOEX (pack 4) */
    TEST_TYPE(EMRMOVETOEX, 16, 4);
    TEST_FIELD(EMRMOVETOEX, emr, 0, 8, 4);
    TEST_FIELD(EMRMOVETOEX, ptl, 8, 8, 4);
}

static void test_pack_EMROFFSETCLIPRGN(void)
{
    /* EMROFFSETCLIPRGN (pack 4) */
    TEST_TYPE(EMROFFSETCLIPRGN, 16, 4);
    TEST_FIELD(EMROFFSETCLIPRGN, emr, 0, 8, 4);
    TEST_FIELD(EMROFFSETCLIPRGN, ptlOffset, 8, 8, 4);
}

static void test_pack_EMRPAINTRGN(void)
{
    /* EMRPAINTRGN (pack 4) */
    TEST_TYPE(EMRPAINTRGN, 32, 4);
    TEST_FIELD(EMRPAINTRGN, emr, 0, 8, 4);
    TEST_FIELD(EMRPAINTRGN, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPAINTRGN, cbRgnData, 24, 4, 4);
    TEST_FIELD(EMRPAINTRGN, RgnData, 28, 1, 1);
}

static void test_pack_EMRPIE(void)
{
    /* EMRPIE (pack 4) */
    TEST_TYPE(EMRPIE, 40, 4);
    TEST_FIELD(EMRPIE, emr, 0, 8, 4);
    TEST_FIELD(EMRPIE, rclBox, 8, 16, 4);
    TEST_FIELD(EMRPIE, ptlStart, 24, 8, 4);
    TEST_FIELD(EMRPIE, ptlEnd, 32, 8, 4);
}

static void test_pack_EMRPIXELFORMAT(void)
{
    /* EMRPIXELFORMAT (pack 4) */
    TEST_TYPE(EMRPIXELFORMAT, 48, 4);
    TEST_FIELD(EMRPIXELFORMAT, emr, 0, 8, 4);
    TEST_FIELD(EMRPIXELFORMAT, pfd, 8, 40, 4);
}

static void test_pack_EMRPLGBLT(void)
{
    /* EMRPLGBLT (pack 4) */
    TEST_TYPE(EMRPLGBLT, 140, 4);
    TEST_FIELD(EMRPLGBLT, emr, 0, 8, 4);
    TEST_FIELD(EMRPLGBLT, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPLGBLT, aptlDest, 24, 24, 4);
    TEST_FIELD(EMRPLGBLT, xSrc, 48, 4, 4);
    TEST_FIELD(EMRPLGBLT, ySrc, 52, 4, 4);
    TEST_FIELD(EMRPLGBLT, cxSrc, 56, 4, 4);
    TEST_FIELD(EMRPLGBLT, cySrc, 60, 4, 4);
    TEST_FIELD(EMRPLGBLT, xformSrc, 64, 24, 4);
    TEST_FIELD(EMRPLGBLT, crBkColorSrc, 88, 4, 4);
    TEST_FIELD(EMRPLGBLT, iUsageSrc, 92, 4, 4);
    TEST_FIELD(EMRPLGBLT, offBmiSrc, 96, 4, 4);
    TEST_FIELD(EMRPLGBLT, cbBmiSrc, 100, 4, 4);
    TEST_FIELD(EMRPLGBLT, offBitsSrc, 104, 4, 4);
    TEST_FIELD(EMRPLGBLT, cbBitsSrc, 108, 4, 4);
    TEST_FIELD(EMRPLGBLT, xMask, 112, 4, 4);
    TEST_FIELD(EMRPLGBLT, yMask, 116, 4, 4);
    TEST_FIELD(EMRPLGBLT, iUsageMask, 120, 4, 4);
    TEST_FIELD(EMRPLGBLT, offBmiMask, 124, 4, 4);
    TEST_FIELD(EMRPLGBLT, cbBmiMask, 128, 4, 4);
    TEST_FIELD(EMRPLGBLT, offBitsMask, 132, 4, 4);
    TEST_FIELD(EMRPLGBLT, cbBitsMask, 136, 4, 4);
}

static void test_pack_EMRPOLYBEZIER(void)
{
    /* EMRPOLYBEZIER (pack 4) */
    TEST_TYPE(EMRPOLYBEZIER, 36, 4);
    TEST_FIELD(EMRPOLYBEZIER, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYBEZIER, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYBEZIER, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYBEZIER, aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYBEZIER16(void)
{
    /* EMRPOLYBEZIER16 (pack 4) */
    TEST_TYPE(EMRPOLYBEZIER16, 32, 4);
    TEST_FIELD(EMRPOLYBEZIER16, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYBEZIER16, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYBEZIER16, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYBEZIER16, apts, 28, 4, 2);
}

static void test_pack_EMRPOLYBEZIERTO(void)
{
    /* EMRPOLYBEZIERTO (pack 4) */
    TEST_TYPE(EMRPOLYBEZIERTO, 36, 4);
    TEST_FIELD(EMRPOLYBEZIERTO, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYBEZIERTO, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYBEZIERTO, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYBEZIERTO, aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYBEZIERTO16(void)
{
    /* EMRPOLYBEZIERTO16 (pack 4) */
    TEST_TYPE(EMRPOLYBEZIERTO16, 32, 4);
    TEST_FIELD(EMRPOLYBEZIERTO16, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYBEZIERTO16, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYBEZIERTO16, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYBEZIERTO16, apts, 28, 4, 2);
}

static void test_pack_EMRPOLYDRAW(void)
{
    /* EMRPOLYDRAW (pack 4) */
    TEST_TYPE(EMRPOLYDRAW, 40, 4);
    TEST_FIELD(EMRPOLYDRAW, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYDRAW, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYDRAW, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYDRAW, aptl, 28, 8, 4);
    TEST_FIELD(EMRPOLYDRAW, abTypes, 36, 1, 1);
}

static void test_pack_EMRPOLYDRAW16(void)
{
    /* EMRPOLYDRAW16 (pack 4) */
    TEST_TYPE(EMRPOLYDRAW16, 36, 4);
    TEST_FIELD(EMRPOLYDRAW16, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYDRAW16, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYDRAW16, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYDRAW16, apts, 28, 4, 2);
    TEST_FIELD(EMRPOLYDRAW16, abTypes, 32, 1, 1);
}

static void test_pack_EMRPOLYGON(void)
{
    /* EMRPOLYGON (pack 4) */
    TEST_TYPE(EMRPOLYGON, 36, 4);
    TEST_FIELD(EMRPOLYGON, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYGON, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYGON, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYGON, aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYGON16(void)
{
    /* EMRPOLYGON16 (pack 4) */
    TEST_TYPE(EMRPOLYGON16, 32, 4);
    TEST_FIELD(EMRPOLYGON16, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYGON16, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYGON16, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYGON16, apts, 28, 4, 2);
}

static void test_pack_EMRPOLYLINE(void)
{
    /* EMRPOLYLINE (pack 4) */
    TEST_TYPE(EMRPOLYLINE, 36, 4);
    TEST_FIELD(EMRPOLYLINE, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYLINE, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYLINE, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYLINE, aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYLINE16(void)
{
    /* EMRPOLYLINE16 (pack 4) */
    TEST_TYPE(EMRPOLYLINE16, 32, 4);
    TEST_FIELD(EMRPOLYLINE16, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYLINE16, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYLINE16, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYLINE16, apts, 28, 4, 2);
}

static void test_pack_EMRPOLYLINETO(void)
{
    /* EMRPOLYLINETO (pack 4) */
    TEST_TYPE(EMRPOLYLINETO, 36, 4);
    TEST_FIELD(EMRPOLYLINETO, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYLINETO, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYLINETO, cptl, 24, 4, 4);
    TEST_FIELD(EMRPOLYLINETO, aptl, 28, 8, 4);
}

static void test_pack_EMRPOLYLINETO16(void)
{
    /* EMRPOLYLINETO16 (pack 4) */
    TEST_TYPE(EMRPOLYLINETO16, 32, 4);
    TEST_FIELD(EMRPOLYLINETO16, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYLINETO16, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYLINETO16, cpts, 24, 4, 4);
    TEST_FIELD(EMRPOLYLINETO16, apts, 28, 4, 2);
}

static void test_pack_EMRPOLYPOLYGON(void)
{
    /* EMRPOLYPOLYGON (pack 4) */
    TEST_TYPE(EMRPOLYPOLYGON, 44, 4);
    TEST_FIELD(EMRPOLYPOLYGON, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYPOLYGON, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYPOLYGON, nPolys, 24, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON, cptl, 28, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON, aPolyCounts, 32, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON, aptl, 36, 8, 4);
}

static void test_pack_EMRPOLYPOLYGON16(void)
{
    /* EMRPOLYPOLYGON16 (pack 4) */
    TEST_TYPE(EMRPOLYPOLYGON16, 40, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, nPolys, 24, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, cpts, 28, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, aPolyCounts, 32, 4, 4);
    TEST_FIELD(EMRPOLYPOLYGON16, apts, 36, 4, 2);
}

static void test_pack_EMRPOLYPOLYLINE(void)
{
    /* EMRPOLYPOLYLINE (pack 4) */
    TEST_TYPE(EMRPOLYPOLYLINE, 44, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, nPolys, 24, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, cptl, 28, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, aPolyCounts, 32, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE, aptl, 36, 8, 4);
}

static void test_pack_EMRPOLYPOLYLINE16(void)
{
    /* EMRPOLYPOLYLINE16 (pack 4) */
    TEST_TYPE(EMRPOLYPOLYLINE16, 40, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, nPolys, 24, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, cpts, 28, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, aPolyCounts, 32, 4, 4);
    TEST_FIELD(EMRPOLYPOLYLINE16, apts, 36, 4, 2);
}

static void test_pack_EMRPOLYTEXTOUTA(void)
{
    /* EMRPOLYTEXTOUTA (pack 4) */
    TEST_TYPE(EMRPOLYTEXTOUTA, 80, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, iGraphicsMode, 24, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, exScale, 28, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, eyScale, 32, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, cStrings, 36, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTA, aemrtext, 40, 40, 4);
}

static void test_pack_EMRPOLYTEXTOUTW(void)
{
    /* EMRPOLYTEXTOUTW (pack 4) */
    TEST_TYPE(EMRPOLYTEXTOUTW, 80, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, emr, 0, 8, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, iGraphicsMode, 24, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, exScale, 28, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, eyScale, 32, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, cStrings, 36, 4, 4);
    TEST_FIELD(EMRPOLYTEXTOUTW, aemrtext, 40, 40, 4);
}

static void test_pack_EMRREALIZEPALETTE(void)
{
    /* EMRREALIZEPALETTE (pack 4) */
    TEST_TYPE(EMRREALIZEPALETTE, 8, 4);
    TEST_FIELD(EMRREALIZEPALETTE, emr, 0, 8, 4);
}

static void test_pack_EMRRECTANGLE(void)
{
    /* EMRRECTANGLE (pack 4) */
    TEST_TYPE(EMRRECTANGLE, 24, 4);
    TEST_FIELD(EMRRECTANGLE, emr, 0, 8, 4);
    TEST_FIELD(EMRRECTANGLE, rclBox, 8, 16, 4);
}

static void test_pack_EMRRESIZEPALETTE(void)
{
    /* EMRRESIZEPALETTE (pack 4) */
    TEST_TYPE(EMRRESIZEPALETTE, 16, 4);
    TEST_FIELD(EMRRESIZEPALETTE, emr, 0, 8, 4);
    TEST_FIELD(EMRRESIZEPALETTE, ihPal, 8, 4, 4);
    TEST_FIELD(EMRRESIZEPALETTE, cEntries, 12, 4, 4);
}

static void test_pack_EMRRESTOREDC(void)
{
    /* EMRRESTOREDC (pack 4) */
    TEST_TYPE(EMRRESTOREDC, 12, 4);
    TEST_FIELD(EMRRESTOREDC, emr, 0, 8, 4);
    TEST_FIELD(EMRRESTOREDC, iRelative, 8, 4, 4);
}

static void test_pack_EMRROUNDRECT(void)
{
    /* EMRROUNDRECT (pack 4) */
    TEST_TYPE(EMRROUNDRECT, 32, 4);
    TEST_FIELD(EMRROUNDRECT, emr, 0, 8, 4);
    TEST_FIELD(EMRROUNDRECT, rclBox, 8, 16, 4);
    TEST_FIELD(EMRROUNDRECT, szlCorner, 24, 8, 4);
}

static void test_pack_EMRSAVEDC(void)
{
    /* EMRSAVEDC (pack 4) */
    TEST_TYPE(EMRSAVEDC, 8, 4);
    TEST_FIELD(EMRSAVEDC, emr, 0, 8, 4);
}

static void test_pack_EMRSCALEVIEWPORTEXTEX(void)
{
    /* EMRSCALEVIEWPORTEXTEX (pack 4) */
    TEST_TYPE(EMRSCALEVIEWPORTEXTEX, 24, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, emr, 0, 8, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, xNum, 8, 4, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, xDenom, 12, 4, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, yNum, 16, 4, 4);
    TEST_FIELD(EMRSCALEVIEWPORTEXTEX, yDenom, 20, 4, 4);
}

static void test_pack_EMRSCALEWINDOWEXTEX(void)
{
    /* EMRSCALEWINDOWEXTEX (pack 4) */
    TEST_TYPE(EMRSCALEWINDOWEXTEX, 24, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, emr, 0, 8, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, xNum, 8, 4, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, xDenom, 12, 4, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, yNum, 16, 4, 4);
    TEST_FIELD(EMRSCALEWINDOWEXTEX, yDenom, 20, 4, 4);
}

static void test_pack_EMRSELECTCLIPPATH(void)
{
    /* EMRSELECTCLIPPATH (pack 4) */
    TEST_TYPE(EMRSELECTCLIPPATH, 12, 4);
    TEST_FIELD(EMRSELECTCLIPPATH, emr, 0, 8, 4);
    TEST_FIELD(EMRSELECTCLIPPATH, iMode, 8, 4, 4);
}

static void test_pack_EMRSELECTCOLORSPACE(void)
{
    /* EMRSELECTCOLORSPACE (pack 4) */
    TEST_TYPE(EMRSELECTCOLORSPACE, 12, 4);
    TEST_FIELD(EMRSELECTCOLORSPACE, emr, 0, 8, 4);
    TEST_FIELD(EMRSELECTCOLORSPACE, ihCS, 8, 4, 4);
}

static void test_pack_EMRSELECTOBJECT(void)
{
    /* EMRSELECTOBJECT (pack 4) */
    TEST_TYPE(EMRSELECTOBJECT, 12, 4);
    TEST_FIELD(EMRSELECTOBJECT, emr, 0, 8, 4);
    TEST_FIELD(EMRSELECTOBJECT, ihObject, 8, 4, 4);
}

static void test_pack_EMRSELECTPALETTE(void)
{
    /* EMRSELECTPALETTE (pack 4) */
    TEST_TYPE(EMRSELECTPALETTE, 12, 4);
    TEST_FIELD(EMRSELECTPALETTE, emr, 0, 8, 4);
    TEST_FIELD(EMRSELECTPALETTE, ihPal, 8, 4, 4);
}

static void test_pack_EMRSETARCDIRECTION(void)
{
    /* EMRSETARCDIRECTION (pack 4) */
    TEST_TYPE(EMRSETARCDIRECTION, 12, 4);
    TEST_FIELD(EMRSETARCDIRECTION, emr, 0, 8, 4);
    TEST_FIELD(EMRSETARCDIRECTION, iArcDirection, 8, 4, 4);
}

static void test_pack_EMRSETBKCOLOR(void)
{
    /* EMRSETBKCOLOR (pack 4) */
    TEST_TYPE(EMRSETBKCOLOR, 12, 4);
    TEST_FIELD(EMRSETBKCOLOR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETBKCOLOR, crColor, 8, 4, 4);
}

static void test_pack_EMRSETBKMODE(void)
{
    /* EMRSETBKMODE (pack 4) */
    TEST_TYPE(EMRSETBKMODE, 12, 4);
    TEST_FIELD(EMRSETBKMODE, emr, 0, 8, 4);
    TEST_FIELD(EMRSETBKMODE, iMode, 8, 4, 4);
}

static void test_pack_EMRSETBRUSHORGEX(void)
{
    /* EMRSETBRUSHORGEX (pack 4) */
    TEST_TYPE(EMRSETBRUSHORGEX, 16, 4);
    TEST_FIELD(EMRSETBRUSHORGEX, emr, 0, 8, 4);
    TEST_FIELD(EMRSETBRUSHORGEX, ptlOrigin, 8, 8, 4);
}

static void test_pack_EMRSETCOLORADJUSTMENT(void)
{
    /* EMRSETCOLORADJUSTMENT (pack 4) */
    TEST_TYPE(EMRSETCOLORADJUSTMENT, 32, 4);
    TEST_FIELD(EMRSETCOLORADJUSTMENT, emr, 0, 8, 4);
    TEST_FIELD(EMRSETCOLORADJUSTMENT, ColorAdjustment, 8, 24, 2);
}

static void test_pack_EMRSETCOLORSPACE(void)
{
    /* EMRSETCOLORSPACE (pack 4) */
    TEST_TYPE(EMRSETCOLORSPACE, 12, 4);
    TEST_FIELD(EMRSETCOLORSPACE, emr, 0, 8, 4);
    TEST_FIELD(EMRSETCOLORSPACE, ihCS, 8, 4, 4);
}

static void test_pack_EMRSETDIBITSTODEVICE(void)
{
    /* EMRSETDIBITSTODEVICE (pack 4) */
    TEST_TYPE(EMRSETDIBITSTODEVICE, 76, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, emr, 0, 8, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, xDest, 24, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, yDest, 28, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, xSrc, 32, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, ySrc, 36, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, cxSrc, 40, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, cySrc, 44, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, offBmiSrc, 48, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, cbBmiSrc, 52, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, offBitsSrc, 56, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, cbBitsSrc, 60, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, iUsageSrc, 64, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, iStartScan, 68, 4, 4);
    TEST_FIELD(EMRSETDIBITSTODEVICE, cScans, 72, 4, 4);
}

static void test_pack_EMRSETICMMODE(void)
{
    /* EMRSETICMMODE (pack 4) */
    TEST_TYPE(EMRSETICMMODE, 12, 4);
    TEST_FIELD(EMRSETICMMODE, emr, 0, 8, 4);
    TEST_FIELD(EMRSETICMMODE, iMode, 8, 4, 4);
}

static void test_pack_EMRSETLAYOUT(void)
{
    /* EMRSETLAYOUT (pack 4) */
    TEST_TYPE(EMRSETLAYOUT, 12, 4);
    TEST_FIELD(EMRSETLAYOUT, emr, 0, 8, 4);
    TEST_FIELD(EMRSETLAYOUT, iMode, 8, 4, 4);
}

static void test_pack_EMRSETMAPMODE(void)
{
    /* EMRSETMAPMODE (pack 4) */
    TEST_TYPE(EMRSETMAPMODE, 12, 4);
    TEST_FIELD(EMRSETMAPMODE, emr, 0, 8, 4);
    TEST_FIELD(EMRSETMAPMODE, iMode, 8, 4, 4);
}

static void test_pack_EMRSETMAPPERFLAGS(void)
{
    /* EMRSETMAPPERFLAGS (pack 4) */
    TEST_TYPE(EMRSETMAPPERFLAGS, 12, 4);
    TEST_FIELD(EMRSETMAPPERFLAGS, emr, 0, 8, 4);
    TEST_FIELD(EMRSETMAPPERFLAGS, dwFlags, 8, 4, 4);
}

static void test_pack_EMRSETMETARGN(void)
{
    /* EMRSETMETARGN (pack 4) */
    TEST_TYPE(EMRSETMETARGN, 8, 4);
    TEST_FIELD(EMRSETMETARGN, emr, 0, 8, 4);
}

static void test_pack_EMRSETMITERLIMIT(void)
{
    /* EMRSETMITERLIMIT (pack 4) */
    TEST_TYPE(EMRSETMITERLIMIT, 12, 4);
    TEST_FIELD(EMRSETMITERLIMIT, emr, 0, 8, 4);
    TEST_FIELD(EMRSETMITERLIMIT, eMiterLimit, 8, 4, 4);
}

static void test_pack_EMRSETPIXELV(void)
{
    /* EMRSETPIXELV (pack 4) */
    TEST_TYPE(EMRSETPIXELV, 20, 4);
    TEST_FIELD(EMRSETPIXELV, emr, 0, 8, 4);
    TEST_FIELD(EMRSETPIXELV, ptlPixel, 8, 8, 4);
    TEST_FIELD(EMRSETPIXELV, crColor, 16, 4, 4);
}

static void test_pack_EMRSETPOLYFILLMODE(void)
{
    /* EMRSETPOLYFILLMODE (pack 4) */
    TEST_TYPE(EMRSETPOLYFILLMODE, 12, 4);
    TEST_FIELD(EMRSETPOLYFILLMODE, emr, 0, 8, 4);
    TEST_FIELD(EMRSETPOLYFILLMODE, iMode, 8, 4, 4);
}

static void test_pack_EMRSETROP2(void)
{
    /* EMRSETROP2 (pack 4) */
    TEST_TYPE(EMRSETROP2, 12, 4);
    TEST_FIELD(EMRSETROP2, emr, 0, 8, 4);
    TEST_FIELD(EMRSETROP2, iMode, 8, 4, 4);
}

static void test_pack_EMRSETSTRETCHBLTMODE(void)
{
    /* EMRSETSTRETCHBLTMODE (pack 4) */
    TEST_TYPE(EMRSETSTRETCHBLTMODE, 12, 4);
    TEST_FIELD(EMRSETSTRETCHBLTMODE, emr, 0, 8, 4);
    TEST_FIELD(EMRSETSTRETCHBLTMODE, iMode, 8, 4, 4);
}

static void test_pack_EMRSETTEXTALIGN(void)
{
    /* EMRSETTEXTALIGN (pack 4) */
    TEST_TYPE(EMRSETTEXTALIGN, 12, 4);
    TEST_FIELD(EMRSETTEXTALIGN, emr, 0, 8, 4);
    TEST_FIELD(EMRSETTEXTALIGN, iMode, 8, 4, 4);
}

static void test_pack_EMRSETTEXTCOLOR(void)
{
    /* EMRSETTEXTCOLOR (pack 4) */
    TEST_TYPE(EMRSETTEXTCOLOR, 12, 4);
    TEST_FIELD(EMRSETTEXTCOLOR, emr, 0, 8, 4);
    TEST_FIELD(EMRSETTEXTCOLOR, crColor, 8, 4, 4);
}

static void test_pack_EMRSETVIEWPORTEXTEX(void)
{
    /* EMRSETVIEWPORTEXTEX (pack 4) */
    TEST_TYPE(EMRSETVIEWPORTEXTEX, 16, 4);
    TEST_FIELD(EMRSETVIEWPORTEXTEX, emr, 0, 8, 4);
    TEST_FIELD(EMRSETVIEWPORTEXTEX, szlExtent, 8, 8, 4);
}

static void test_pack_EMRSETVIEWPORTORGEX(void)
{
    /* EMRSETVIEWPORTORGEX (pack 4) */
    TEST_TYPE(EMRSETVIEWPORTORGEX, 16, 4);
    TEST_FIELD(EMRSETVIEWPORTORGEX, emr, 0, 8, 4);
    TEST_FIELD(EMRSETVIEWPORTORGEX, ptlOrigin, 8, 8, 4);
}

static void test_pack_EMRSETWINDOWEXTEX(void)
{
    /* EMRSETWINDOWEXTEX (pack 4) */
    TEST_TYPE(EMRSETWINDOWEXTEX, 16, 4);
    TEST_FIELD(EMRSETWINDOWEXTEX, emr, 0, 8, 4);
    TEST_FIELD(EMRSETWINDOWEXTEX, szlExtent, 8, 8, 4);
}

static void test_pack_EMRSETWINDOWORGEX(void)
{
    /* EMRSETWINDOWORGEX (pack 4) */
    TEST_TYPE(EMRSETWINDOWORGEX, 16, 4);
    TEST_FIELD(EMRSETWINDOWORGEX, emr, 0, 8, 4);
    TEST_FIELD(EMRSETWINDOWORGEX, ptlOrigin, 8, 8, 4);
}

static void test_pack_EMRSETWORLDTRANSFORM(void)
{
    /* EMRSETWORLDTRANSFORM (pack 4) */
    TEST_TYPE(EMRSETWORLDTRANSFORM, 32, 4);
    TEST_FIELD(EMRSETWORLDTRANSFORM, emr, 0, 8, 4);
    TEST_FIELD(EMRSETWORLDTRANSFORM, xform, 8, 24, 4);
}

static void test_pack_EMRSTRETCHBLT(void)
{
    /* EMRSTRETCHBLT (pack 4) */
    TEST_TYPE(EMRSTRETCHBLT, 108, 4);
    TEST_FIELD(EMRSTRETCHBLT, emr, 0, 8, 4);
    TEST_FIELD(EMRSTRETCHBLT, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRSTRETCHBLT, xDest, 24, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, yDest, 28, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, cxDest, 32, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, cyDest, 36, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, dwRop, 40, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, xSrc, 44, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, ySrc, 48, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, xformSrc, 52, 24, 4);
    TEST_FIELD(EMRSTRETCHBLT, crBkColorSrc, 76, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, iUsageSrc, 80, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, offBmiSrc, 84, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, cbBmiSrc, 88, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, offBitsSrc, 92, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, cbBitsSrc, 96, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, cxSrc, 100, 4, 4);
    TEST_FIELD(EMRSTRETCHBLT, cySrc, 104, 4, 4);
}

static void test_pack_EMRSTRETCHDIBITS(void)
{
    /* EMRSTRETCHDIBITS (pack 4) */
    TEST_TYPE(EMRSTRETCHDIBITS, 80, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, emr, 0, 8, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, rclBounds, 8, 16, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, xDest, 24, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, yDest, 28, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, xSrc, 32, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, ySrc, 36, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, cxSrc, 40, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, cySrc, 44, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, offBmiSrc, 48, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, cbBmiSrc, 52, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, offBitsSrc, 56, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, cbBitsSrc, 60, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, iUsageSrc, 64, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, dwRop, 68, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, cxDest, 72, 4, 4);
    TEST_FIELD(EMRSTRETCHDIBITS, cyDest, 76, 4, 4);
}

static void test_pack_EMRSTROKEANDFILLPATH(void)
{
    /* EMRSTROKEANDFILLPATH (pack 4) */
    TEST_TYPE(EMRSTROKEANDFILLPATH, 24, 4);
    TEST_FIELD(EMRSTROKEANDFILLPATH, emr, 0, 8, 4);
    TEST_FIELD(EMRSTROKEANDFILLPATH, rclBounds, 8, 16, 4);
}

static void test_pack_EMRSTROKEPATH(void)
{
    /* EMRSTROKEPATH (pack 4) */
    TEST_TYPE(EMRSTROKEPATH, 24, 4);
    TEST_FIELD(EMRSTROKEPATH, emr, 0, 8, 4);
    TEST_FIELD(EMRSTROKEPATH, rclBounds, 8, 16, 4);
}

static void test_pack_EMRTEXT(void)
{
    /* EMRTEXT (pack 4) */
    TEST_TYPE(EMRTEXT, 40, 4);
    TEST_FIELD(EMRTEXT, ptlReference, 0, 8, 4);
    TEST_FIELD(EMRTEXT, nChars, 8, 4, 4);
    TEST_FIELD(EMRTEXT, offString, 12, 4, 4);
    TEST_FIELD(EMRTEXT, fOptions, 16, 4, 4);
    TEST_FIELD(EMRTEXT, rcl, 20, 16, 4);
    TEST_FIELD(EMRTEXT, offDx, 36, 4, 4);
}

static void test_pack_EMRWIDENPATH(void)
{
    /* EMRWIDENPATH (pack 4) */
    TEST_TYPE(EMRWIDENPATH, 8, 4);
    TEST_FIELD(EMRWIDENPATH, emr, 0, 8, 4);
}

static void test_pack_ENHMETAHEADER(void)
{
    /* ENHMETAHEADER (pack 4) */
    TEST_TYPE(ENHMETAHEADER, 108, 4);
    TEST_FIELD(ENHMETAHEADER, iType, 0, 4, 4);
    TEST_FIELD(ENHMETAHEADER, nSize, 4, 4, 4);
    TEST_FIELD(ENHMETAHEADER, rclBounds, 8, 16, 4);
    TEST_FIELD(ENHMETAHEADER, rclFrame, 24, 16, 4);
    TEST_FIELD(ENHMETAHEADER, dSignature, 40, 4, 4);
    TEST_FIELD(ENHMETAHEADER, nVersion, 44, 4, 4);
    TEST_FIELD(ENHMETAHEADER, nBytes, 48, 4, 4);
    TEST_FIELD(ENHMETAHEADER, nRecords, 52, 4, 4);
    TEST_FIELD(ENHMETAHEADER, nHandles, 56, 2, 2);
    TEST_FIELD(ENHMETAHEADER, sReserved, 58, 2, 2);
    TEST_FIELD(ENHMETAHEADER, nDescription, 60, 4, 4);
    TEST_FIELD(ENHMETAHEADER, offDescription, 64, 4, 4);
    TEST_FIELD(ENHMETAHEADER, nPalEntries, 68, 4, 4);
    TEST_FIELD(ENHMETAHEADER, szlDevice, 72, 8, 4);
    TEST_FIELD(ENHMETAHEADER, szlMillimeters, 80, 8, 4);
    TEST_FIELD(ENHMETAHEADER, cbPixelFormat, 88, 4, 4);
    TEST_FIELD(ENHMETAHEADER, offPixelFormat, 92, 4, 4);
    TEST_FIELD(ENHMETAHEADER, bOpenGL, 96, 4, 4);
    TEST_FIELD(ENHMETAHEADER, szlMicrometers, 100, 8, 4);
}

static void test_pack_ENHMETARECORD(void)
{
    /* ENHMETARECORD (pack 4) */
    TEST_TYPE(ENHMETARECORD, 12, 4);
    TEST_FIELD(ENHMETARECORD, iType, 0, 4, 4);
    TEST_FIELD(ENHMETARECORD, nSize, 4, 4, 4);
    TEST_FIELD(ENHMETARECORD, dParm, 8, 4, 4);
}

static void test_pack_ENHMFENUMPROC(void)
{
    /* ENHMFENUMPROC */
    TEST_TYPE(ENHMFENUMPROC, 4, 4);
}

static void test_pack_ENUMLOGFONTA(void)
{
    /* ENUMLOGFONTA (pack 4) */
    TEST_TYPE(ENUMLOGFONTA, 156, 4);
    TEST_FIELD(ENUMLOGFONTA, elfLogFont, 0, 60, 4);
    TEST_FIELD(ENUMLOGFONTA, elfFullName, 60, 64, 1);
    TEST_FIELD(ENUMLOGFONTA, elfStyle, 124, 32, 1);
}

static void test_pack_ENUMLOGFONTEXA(void)
{
    /* ENUMLOGFONTEXA (pack 4) */
    TEST_TYPE(ENUMLOGFONTEXA, 188, 4);
    TEST_FIELD(ENUMLOGFONTEXA, elfLogFont, 0, 60, 4);
    TEST_FIELD(ENUMLOGFONTEXA, elfFullName, 60, 64, 1);
    TEST_FIELD(ENUMLOGFONTEXA, elfStyle, 124, 32, 1);
    TEST_FIELD(ENUMLOGFONTEXA, elfScript, 156, 32, 1);
}

static void test_pack_ENUMLOGFONTEXW(void)
{
    /* ENUMLOGFONTEXW (pack 4) */
    TEST_TYPE(ENUMLOGFONTEXW, 348, 4);
    TEST_FIELD(ENUMLOGFONTEXW, elfLogFont, 0, 92, 4);
    TEST_FIELD(ENUMLOGFONTEXW, elfFullName, 92, 128, 2);
    TEST_FIELD(ENUMLOGFONTEXW, elfStyle, 220, 64, 2);
    TEST_FIELD(ENUMLOGFONTEXW, elfScript, 284, 64, 2);
}

static void test_pack_ENUMLOGFONTW(void)
{
    /* ENUMLOGFONTW (pack 4) */
    TEST_TYPE(ENUMLOGFONTW, 284, 4);
    TEST_FIELD(ENUMLOGFONTW, elfLogFont, 0, 92, 4);
    TEST_FIELD(ENUMLOGFONTW, elfFullName, 92, 128, 2);
    TEST_FIELD(ENUMLOGFONTW, elfStyle, 220, 64, 2);
}

static void test_pack_EXTLOGFONTA(void)
{
    /* EXTLOGFONTA (pack 4) */
    TEST_TYPE(EXTLOGFONTA, 192, 4);
    TEST_FIELD(EXTLOGFONTA, elfLogFont, 0, 60, 4);
    TEST_FIELD(EXTLOGFONTA, elfFullName, 60, 64, 1);
    TEST_FIELD(EXTLOGFONTA, elfStyle, 124, 32, 1);
    TEST_FIELD(EXTLOGFONTA, elfVersion, 156, 4, 4);
    TEST_FIELD(EXTLOGFONTA, elfStyleSize, 160, 4, 4);
    TEST_FIELD(EXTLOGFONTA, elfMatch, 164, 4, 4);
    TEST_FIELD(EXTLOGFONTA, elfReserved, 168, 4, 4);
    TEST_FIELD(EXTLOGFONTA, elfVendorId, 172, 4, 1);
    TEST_FIELD(EXTLOGFONTA, elfCulture, 176, 4, 4);
    TEST_FIELD(EXTLOGFONTA, elfPanose, 180, 10, 1);
}

static void test_pack_EXTLOGFONTW(void)
{
    /* EXTLOGFONTW (pack 4) */
    TEST_TYPE(EXTLOGFONTW, 320, 4);
    TEST_FIELD(EXTLOGFONTW, elfLogFont, 0, 92, 4);
    TEST_FIELD(EXTLOGFONTW, elfFullName, 92, 128, 2);
    TEST_FIELD(EXTLOGFONTW, elfStyle, 220, 64, 2);
    TEST_FIELD(EXTLOGFONTW, elfVersion, 284, 4, 4);
    TEST_FIELD(EXTLOGFONTW, elfStyleSize, 288, 4, 4);
    TEST_FIELD(EXTLOGFONTW, elfMatch, 292, 4, 4);
    TEST_FIELD(EXTLOGFONTW, elfReserved, 296, 4, 4);
    TEST_FIELD(EXTLOGFONTW, elfVendorId, 300, 4, 1);
    TEST_FIELD(EXTLOGFONTW, elfCulture, 304, 4, 4);
    TEST_FIELD(EXTLOGFONTW, elfPanose, 308, 10, 1);
}

static void test_pack_EXTLOGPEN(void)
{
    /* EXTLOGPEN (pack 4) */
    TEST_TYPE(EXTLOGPEN, 28, 4);
    TEST_FIELD(EXTLOGPEN, elpPenStyle, 0, 4, 4);
    TEST_FIELD(EXTLOGPEN, elpWidth, 4, 4, 4);
    TEST_FIELD(EXTLOGPEN, elpBrushStyle, 8, 4, 4);
    TEST_FIELD(EXTLOGPEN, elpColor, 12, 4, 4);
    TEST_FIELD(EXTLOGPEN, elpHatch, 16, 4, 4);
    TEST_FIELD(EXTLOGPEN, elpNumEntries, 20, 4, 4);
    TEST_FIELD(EXTLOGPEN, elpStyleEntry, 24, 4, 4);
}

static void test_pack_FIXED(void)
{
    /* FIXED (pack 4) */
    TEST_TYPE(FIXED, 4, 2);
    TEST_FIELD(FIXED, fract, 0, 2, 2);
    TEST_FIELD(FIXED, value, 2, 2, 2);
}

static void test_pack_FONTENUMPROCA(void)
{
    /* FONTENUMPROCA */
    TEST_TYPE(FONTENUMPROCA, 4, 4);
}

static void test_pack_FONTENUMPROCW(void)
{
    /* FONTENUMPROCW */
    TEST_TYPE(FONTENUMPROCW, 4, 4);
}

static void test_pack_FONTSIGNATURE(void)
{
    /* FONTSIGNATURE (pack 4) */
    TEST_TYPE(FONTSIGNATURE, 24, 4);
    TEST_FIELD(FONTSIGNATURE, fsUsb, 0, 16, 4);
    TEST_FIELD(FONTSIGNATURE, fsCsb, 16, 8, 4);
}

static void test_pack_FXPT16DOT16(void)
{
    /* FXPT16DOT16 */
    TEST_TYPE(FXPT16DOT16, 4, 4);
}

static void test_pack_FXPT2DOT30(void)
{
    /* FXPT2DOT30 */
    TEST_TYPE(FXPT2DOT30, 4, 4);
}

static void test_pack_GCP_RESULTSA(void)
{
    /* GCP_RESULTSA (pack 4) */
    TEST_TYPE(GCP_RESULTSA, 36, 4);
    TEST_FIELD(GCP_RESULTSA, lStructSize, 0, 4, 4);
    TEST_FIELD(GCP_RESULTSA, lpOutString, 4, 4, 4);
    TEST_FIELD(GCP_RESULTSA, lpOrder, 8, 4, 4);
    TEST_FIELD(GCP_RESULTSA, lpDx, 12, 4, 4);
    TEST_FIELD(GCP_RESULTSA, lpCaretPos, 16, 4, 4);
    TEST_FIELD(GCP_RESULTSA, lpClass, 20, 4, 4);
    TEST_FIELD(GCP_RESULTSA, lpGlyphs, 24, 4, 4);
    TEST_FIELD(GCP_RESULTSA, nGlyphs, 28, 4, 4);
    TEST_FIELD(GCP_RESULTSA, nMaxFit, 32, 4, 4);
}

static void test_pack_GCP_RESULTSW(void)
{
    /* GCP_RESULTSW (pack 4) */
    TEST_TYPE(GCP_RESULTSW, 36, 4);
    TEST_FIELD(GCP_RESULTSW, lStructSize, 0, 4, 4);
    TEST_FIELD(GCP_RESULTSW, lpOutString, 4, 4, 4);
    TEST_FIELD(GCP_RESULTSW, lpOrder, 8, 4, 4);
    TEST_FIELD(GCP_RESULTSW, lpDx, 12, 4, 4);
    TEST_FIELD(GCP_RESULTSW, lpCaretPos, 16, 4, 4);
    TEST_FIELD(GCP_RESULTSW, lpClass, 20, 4, 4);
    TEST_FIELD(GCP_RESULTSW, lpGlyphs, 24, 4, 4);
    TEST_FIELD(GCP_RESULTSW, nGlyphs, 28, 4, 4);
    TEST_FIELD(GCP_RESULTSW, nMaxFit, 32, 4, 4);
}

static void test_pack_GLYPHMETRICS(void)
{
    /* GLYPHMETRICS (pack 4) */
    TEST_TYPE(GLYPHMETRICS, 20, 4);
    TEST_FIELD(GLYPHMETRICS, gmBlackBoxX, 0, 4, 4);
    TEST_FIELD(GLYPHMETRICS, gmBlackBoxY, 4, 4, 4);
    TEST_FIELD(GLYPHMETRICS, gmptGlyphOrigin, 8, 8, 4);
    TEST_FIELD(GLYPHMETRICS, gmCellIncX, 16, 2, 2);
    TEST_FIELD(GLYPHMETRICS, gmCellIncY, 18, 2, 2);
}

static void test_pack_GLYPHMETRICSFLOAT(void)
{
    /* GLYPHMETRICSFLOAT (pack 4) */
    TEST_TYPE(GLYPHMETRICSFLOAT, 24, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, gmfBlackBoxX, 0, 4, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, gmfBlackBoxY, 4, 4, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, gmfptGlyphOrigin, 8, 8, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, gmfCellIncX, 16, 4, 4);
    TEST_FIELD(GLYPHMETRICSFLOAT, gmfCellIncY, 20, 4, 4);
}

static void test_pack_GOBJENUMPROC(void)
{
    /* GOBJENUMPROC */
    TEST_TYPE(GOBJENUMPROC, 4, 4);
}

static void test_pack_GRADIENT_RECT(void)
{
    /* GRADIENT_RECT (pack 4) */
    TEST_TYPE(GRADIENT_RECT, 8, 4);
    TEST_FIELD(GRADIENT_RECT, UpperLeft, 0, 4, 4);
    TEST_FIELD(GRADIENT_RECT, LowerRight, 4, 4, 4);
}

static void test_pack_GRADIENT_TRIANGLE(void)
{
    /* GRADIENT_TRIANGLE (pack 4) */
    TEST_TYPE(GRADIENT_TRIANGLE, 12, 4);
    TEST_FIELD(GRADIENT_TRIANGLE, Vertex1, 0, 4, 4);
    TEST_FIELD(GRADIENT_TRIANGLE, Vertex2, 4, 4, 4);
    TEST_FIELD(GRADIENT_TRIANGLE, Vertex3, 8, 4, 4);
}

static void test_pack_HANDLETABLE(void)
{
    /* HANDLETABLE (pack 4) */
    TEST_TYPE(HANDLETABLE, 4, 4);
    TEST_FIELD(HANDLETABLE, objectHandle, 0, 4, 4);
}

static void test_pack_ICMENUMPROCA(void)
{
    /* ICMENUMPROCA */
    TEST_TYPE(ICMENUMPROCA, 4, 4);
}

static void test_pack_ICMENUMPROCW(void)
{
    /* ICMENUMPROCW */
    TEST_TYPE(ICMENUMPROCW, 4, 4);
}

static void test_pack_KERNINGPAIR(void)
{
    /* KERNINGPAIR (pack 4) */
    TEST_TYPE(KERNINGPAIR, 8, 4);
    TEST_FIELD(KERNINGPAIR, wFirst, 0, 2, 2);
    TEST_FIELD(KERNINGPAIR, wSecond, 2, 2, 2);
    TEST_FIELD(KERNINGPAIR, iKernAmount, 4, 4, 4);
}

static void test_pack_LAYERPLANEDESCRIPTOR(void)
{
    /* LAYERPLANEDESCRIPTOR (pack 4) */
    TEST_TYPE(LAYERPLANEDESCRIPTOR, 32, 4);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, nSize, 0, 2, 2);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, nVersion, 2, 2, 2);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, dwFlags, 4, 4, 4);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, iPixelType, 8, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cColorBits, 9, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cRedBits, 10, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cRedShift, 11, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cGreenBits, 12, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cGreenShift, 13, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cBlueBits, 14, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cBlueShift, 15, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cAlphaBits, 16, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cAlphaShift, 17, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cAccumBits, 18, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cAccumRedBits, 19, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cAccumGreenBits, 20, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cAccumBlueBits, 21, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cAccumAlphaBits, 22, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cDepthBits, 23, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cStencilBits, 24, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, cAuxBuffers, 25, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, iLayerPlane, 26, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, bReserved, 27, 1, 1);
    TEST_FIELD(LAYERPLANEDESCRIPTOR, crTransparent, 28, 4, 4);
}

static void test_pack_LCSCSTYPE(void)
{
    /* LCSCSTYPE */
    TEST_TYPE(LCSCSTYPE, 4, 4);
}

static void test_pack_LCSGAMUTMATCH(void)
{
    /* LCSGAMUTMATCH */
    TEST_TYPE(LCSGAMUTMATCH, 4, 4);
}

static void test_pack_LINEDDAPROC(void)
{
    /* LINEDDAPROC */
    TEST_TYPE(LINEDDAPROC, 4, 4);
}

static void test_pack_LOCALESIGNATURE(void)
{
    /* LOCALESIGNATURE (pack 4) */
    TEST_TYPE(LOCALESIGNATURE, 32, 4);
    TEST_FIELD(LOCALESIGNATURE, lsUsb, 0, 16, 4);
    TEST_FIELD(LOCALESIGNATURE, lsCsbDefault, 16, 8, 4);
    TEST_FIELD(LOCALESIGNATURE, lsCsbSupported, 24, 8, 4);
}

static void test_pack_LOGBRUSH(void)
{
    /* LOGBRUSH (pack 4) */
    TEST_TYPE(LOGBRUSH, 12, 4);
    TEST_FIELD(LOGBRUSH, lbStyle, 0, 4, 4);
    TEST_FIELD(LOGBRUSH, lbColor, 4, 4, 4);
    TEST_FIELD(LOGBRUSH, lbHatch, 8, 4, 4);
}

static void test_pack_LOGCOLORSPACEA(void)
{
    /* LOGCOLORSPACEA (pack 4) */
    TEST_TYPE(LOGCOLORSPACEA, 328, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsSignature, 0, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsVersion, 4, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsSize, 8, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsCSType, 12, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsIntent, 16, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsEndpoints, 20, 36, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsGammaRed, 56, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsGammaGreen, 60, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsGammaBlue, 64, 4, 4);
    TEST_FIELD(LOGCOLORSPACEA, lcsFilename, 68, 260, 1);
}

static void test_pack_LOGCOLORSPACEW(void)
{
    /* LOGCOLORSPACEW (pack 4) */
    TEST_TYPE(LOGCOLORSPACEW, 588, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsSignature, 0, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsVersion, 4, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsSize, 8, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsCSType, 12, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsIntent, 16, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsEndpoints, 20, 36, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsGammaRed, 56, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsGammaGreen, 60, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsGammaBlue, 64, 4, 4);
    TEST_FIELD(LOGCOLORSPACEW, lcsFilename, 68, 520, 2);
}

static void test_pack_LOGFONTA(void)
{
    /* LOGFONTA (pack 4) */
    TEST_TYPE(LOGFONTA, 60, 4);
    TEST_FIELD(LOGFONTA, lfHeight, 0, 4, 4);
    TEST_FIELD(LOGFONTA, lfWidth, 4, 4, 4);
    TEST_FIELD(LOGFONTA, lfEscapement, 8, 4, 4);
    TEST_FIELD(LOGFONTA, lfOrientation, 12, 4, 4);
    TEST_FIELD(LOGFONTA, lfWeight, 16, 4, 4);
    TEST_FIELD(LOGFONTA, lfItalic, 20, 1, 1);
    TEST_FIELD(LOGFONTA, lfUnderline, 21, 1, 1);
    TEST_FIELD(LOGFONTA, lfStrikeOut, 22, 1, 1);
    TEST_FIELD(LOGFONTA, lfCharSet, 23, 1, 1);
    TEST_FIELD(LOGFONTA, lfOutPrecision, 24, 1, 1);
    TEST_FIELD(LOGFONTA, lfClipPrecision, 25, 1, 1);
    TEST_FIELD(LOGFONTA, lfQuality, 26, 1, 1);
    TEST_FIELD(LOGFONTA, lfPitchAndFamily, 27, 1, 1);
    TEST_FIELD(LOGFONTA, lfFaceName, 28, 32, 1);
}

static void test_pack_LOGFONTW(void)
{
    /* LOGFONTW (pack 4) */
    TEST_TYPE(LOGFONTW, 92, 4);
    TEST_FIELD(LOGFONTW, lfHeight, 0, 4, 4);
    TEST_FIELD(LOGFONTW, lfWidth, 4, 4, 4);
    TEST_FIELD(LOGFONTW, lfEscapement, 8, 4, 4);
    TEST_FIELD(LOGFONTW, lfOrientation, 12, 4, 4);
    TEST_FIELD(LOGFONTW, lfWeight, 16, 4, 4);
    TEST_FIELD(LOGFONTW, lfItalic, 20, 1, 1);
    TEST_FIELD(LOGFONTW, lfUnderline, 21, 1, 1);
    TEST_FIELD(LOGFONTW, lfStrikeOut, 22, 1, 1);
    TEST_FIELD(LOGFONTW, lfCharSet, 23, 1, 1);
    TEST_FIELD(LOGFONTW, lfOutPrecision, 24, 1, 1);
    TEST_FIELD(LOGFONTW, lfClipPrecision, 25, 1, 1);
    TEST_FIELD(LOGFONTW, lfQuality, 26, 1, 1);
    TEST_FIELD(LOGFONTW, lfPitchAndFamily, 27, 1, 1);
    TEST_FIELD(LOGFONTW, lfFaceName, 28, 64, 2);
}

static void test_pack_LOGPEN(void)
{
    /* LOGPEN (pack 4) */
    TEST_TYPE(LOGPEN, 16, 4);
    TEST_FIELD(LOGPEN, lopnStyle, 0, 4, 4);
    TEST_FIELD(LOGPEN, lopnWidth, 4, 8, 4);
    TEST_FIELD(LOGPEN, lopnColor, 12, 4, 4);
}

static void test_pack_LPABC(void)
{
    /* LPABC */
    TEST_TYPE(LPABC, 4, 4);
    TEST_TYPE_POINTER(LPABC, 12, 4);
}

static void test_pack_LPABCFLOAT(void)
{
    /* LPABCFLOAT */
    TEST_TYPE(LPABCFLOAT, 4, 4);
    TEST_TYPE_POINTER(LPABCFLOAT, 12, 4);
}

static void test_pack_LPBITMAP(void)
{
    /* LPBITMAP */
    TEST_TYPE(LPBITMAP, 4, 4);
    TEST_TYPE_POINTER(LPBITMAP, 24, 4);
}

static void test_pack_LPBITMAPCOREHEADER(void)
{
    /* LPBITMAPCOREHEADER */
    TEST_TYPE(LPBITMAPCOREHEADER, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPCOREHEADER, 12, 4);
}

static void test_pack_LPBITMAPCOREINFO(void)
{
    /* LPBITMAPCOREINFO */
    TEST_TYPE(LPBITMAPCOREINFO, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPCOREINFO, 16, 4);
}

static void test_pack_LPBITMAPFILEHEADER(void)
{
    /* LPBITMAPFILEHEADER */
    TEST_TYPE(LPBITMAPFILEHEADER, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPFILEHEADER, 14, 2);
}

static void test_pack_LPBITMAPINFO(void)
{
    /* LPBITMAPINFO */
    TEST_TYPE(LPBITMAPINFO, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPINFO, 44, 4);
}

static void test_pack_LPBITMAPINFOHEADER(void)
{
    /* LPBITMAPINFOHEADER */
    TEST_TYPE(LPBITMAPINFOHEADER, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPINFOHEADER, 40, 4);
}

static void test_pack_LPBITMAPV5HEADER(void)
{
    /* LPBITMAPV5HEADER */
    TEST_TYPE(LPBITMAPV5HEADER, 4, 4);
    TEST_TYPE_POINTER(LPBITMAPV5HEADER, 124, 4);
}

static void test_pack_LPCHARSETINFO(void)
{
    /* LPCHARSETINFO */
    TEST_TYPE(LPCHARSETINFO, 4, 4);
    TEST_TYPE_POINTER(LPCHARSETINFO, 32, 4);
}

static void test_pack_LPCIEXYZ(void)
{
    /* LPCIEXYZ */
    TEST_TYPE(LPCIEXYZ, 4, 4);
    TEST_TYPE_POINTER(LPCIEXYZ, 12, 4);
}

static void test_pack_LPCIEXYZTRIPLE(void)
{
    /* LPCIEXYZTRIPLE */
    TEST_TYPE(LPCIEXYZTRIPLE, 4, 4);
    TEST_TYPE_POINTER(LPCIEXYZTRIPLE, 36, 4);
}

static void test_pack_LPCOLORADJUSTMENT(void)
{
    /* LPCOLORADJUSTMENT */
    TEST_TYPE(LPCOLORADJUSTMENT, 4, 4);
    TEST_TYPE_POINTER(LPCOLORADJUSTMENT, 24, 2);
}

static void test_pack_LPDEVMODEA(void)
{
    /* LPDEVMODEA */
    TEST_TYPE(LPDEVMODEA, 4, 4);
}

static void test_pack_LPDEVMODEW(void)
{
    /* LPDEVMODEW */
    TEST_TYPE(LPDEVMODEW, 4, 4);
}

static void test_pack_LPDIBSECTION(void)
{
    /* LPDIBSECTION */
    TEST_TYPE(LPDIBSECTION, 4, 4);
    TEST_TYPE_POINTER(LPDIBSECTION, 84, 4);
}

static void test_pack_LPDISPLAY_DEVICEA(void)
{
    /* LPDISPLAY_DEVICEA */
    TEST_TYPE(LPDISPLAY_DEVICEA, 4, 4);
    TEST_TYPE_POINTER(LPDISPLAY_DEVICEA, 424, 4);
}

static void test_pack_LPDISPLAY_DEVICEW(void)
{
    /* LPDISPLAY_DEVICEW */
    TEST_TYPE(LPDISPLAY_DEVICEW, 4, 4);
    TEST_TYPE_POINTER(LPDISPLAY_DEVICEW, 840, 4);
}

static void test_pack_LPDOCINFOA(void)
{
    /* LPDOCINFOA */
    TEST_TYPE(LPDOCINFOA, 4, 4);
    TEST_TYPE_POINTER(LPDOCINFOA, 20, 4);
}

static void test_pack_LPDOCINFOW(void)
{
    /* LPDOCINFOW */
    TEST_TYPE(LPDOCINFOW, 4, 4);
    TEST_TYPE_POINTER(LPDOCINFOW, 20, 4);
}

static void test_pack_LPENHMETAHEADER(void)
{
    /* LPENHMETAHEADER */
    TEST_TYPE(LPENHMETAHEADER, 4, 4);
    TEST_TYPE_POINTER(LPENHMETAHEADER, 108, 4);
}

static void test_pack_LPENHMETARECORD(void)
{
    /* LPENHMETARECORD */
    TEST_TYPE(LPENHMETARECORD, 4, 4);
    TEST_TYPE_POINTER(LPENHMETARECORD, 12, 4);
}

static void test_pack_LPENUMLOGFONTA(void)
{
    /* LPENUMLOGFONTA */
    TEST_TYPE(LPENUMLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(LPENUMLOGFONTA, 156, 4);
}

static void test_pack_LPENUMLOGFONTEXA(void)
{
    /* LPENUMLOGFONTEXA */
    TEST_TYPE(LPENUMLOGFONTEXA, 4, 4);
    TEST_TYPE_POINTER(LPENUMLOGFONTEXA, 188, 4);
}

static void test_pack_LPENUMLOGFONTEXW(void)
{
    /* LPENUMLOGFONTEXW */
    TEST_TYPE(LPENUMLOGFONTEXW, 4, 4);
    TEST_TYPE_POINTER(LPENUMLOGFONTEXW, 348, 4);
}

static void test_pack_LPENUMLOGFONTW(void)
{
    /* LPENUMLOGFONTW */
    TEST_TYPE(LPENUMLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(LPENUMLOGFONTW, 284, 4);
}

static void test_pack_LPEXTLOGFONTA(void)
{
    /* LPEXTLOGFONTA */
    TEST_TYPE(LPEXTLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(LPEXTLOGFONTA, 192, 4);
}

static void test_pack_LPEXTLOGFONTW(void)
{
    /* LPEXTLOGFONTW */
    TEST_TYPE(LPEXTLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(LPEXTLOGFONTW, 320, 4);
}

static void test_pack_LPEXTLOGPEN(void)
{
    /* LPEXTLOGPEN */
    TEST_TYPE(LPEXTLOGPEN, 4, 4);
    TEST_TYPE_POINTER(LPEXTLOGPEN, 28, 4);
}

static void test_pack_LPFONTSIGNATURE(void)
{
    /* LPFONTSIGNATURE */
    TEST_TYPE(LPFONTSIGNATURE, 4, 4);
    TEST_TYPE_POINTER(LPFONTSIGNATURE, 24, 4);
}

static void test_pack_LPGCP_RESULTSA(void)
{
    /* LPGCP_RESULTSA */
    TEST_TYPE(LPGCP_RESULTSA, 4, 4);
    TEST_TYPE_POINTER(LPGCP_RESULTSA, 36, 4);
}

static void test_pack_LPGCP_RESULTSW(void)
{
    /* LPGCP_RESULTSW */
    TEST_TYPE(LPGCP_RESULTSW, 4, 4);
    TEST_TYPE_POINTER(LPGCP_RESULTSW, 36, 4);
}

static void test_pack_LPGLYPHMETRICS(void)
{
    /* LPGLYPHMETRICS */
    TEST_TYPE(LPGLYPHMETRICS, 4, 4);
    TEST_TYPE_POINTER(LPGLYPHMETRICS, 20, 4);
}

static void test_pack_LPGLYPHMETRICSFLOAT(void)
{
    /* LPGLYPHMETRICSFLOAT */
    TEST_TYPE(LPGLYPHMETRICSFLOAT, 4, 4);
    TEST_TYPE_POINTER(LPGLYPHMETRICSFLOAT, 24, 4);
}

static void test_pack_LPGRADIENT_RECT(void)
{
    /* LPGRADIENT_RECT */
    TEST_TYPE(LPGRADIENT_RECT, 4, 4);
    TEST_TYPE_POINTER(LPGRADIENT_RECT, 8, 4);
}

static void test_pack_LPGRADIENT_TRIANGLE(void)
{
    /* LPGRADIENT_TRIANGLE */
    TEST_TYPE(LPGRADIENT_TRIANGLE, 4, 4);
    TEST_TYPE_POINTER(LPGRADIENT_TRIANGLE, 12, 4);
}

static void test_pack_LPHANDLETABLE(void)
{
    /* LPHANDLETABLE */
    TEST_TYPE(LPHANDLETABLE, 4, 4);
    TEST_TYPE_POINTER(LPHANDLETABLE, 4, 4);
}

static void test_pack_LPKERNINGPAIR(void)
{
    /* LPKERNINGPAIR */
    TEST_TYPE(LPKERNINGPAIR, 4, 4);
    TEST_TYPE_POINTER(LPKERNINGPAIR, 8, 4);
}

static void test_pack_LPLAYERPLANEDESCRIPTOR(void)
{
    /* LPLAYERPLANEDESCRIPTOR */
    TEST_TYPE(LPLAYERPLANEDESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(LPLAYERPLANEDESCRIPTOR, 32, 4);
}

static void test_pack_LPLOCALESIGNATURE(void)
{
    /* LPLOCALESIGNATURE */
    TEST_TYPE(LPLOCALESIGNATURE, 4, 4);
    TEST_TYPE_POINTER(LPLOCALESIGNATURE, 32, 4);
}

static void test_pack_LPLOGBRUSH(void)
{
    /* LPLOGBRUSH */
    TEST_TYPE(LPLOGBRUSH, 4, 4);
    TEST_TYPE_POINTER(LPLOGBRUSH, 12, 4);
}

static void test_pack_LPLOGCOLORSPACEA(void)
{
    /* LPLOGCOLORSPACEA */
    TEST_TYPE(LPLOGCOLORSPACEA, 4, 4);
    TEST_TYPE_POINTER(LPLOGCOLORSPACEA, 328, 4);
}

static void test_pack_LPLOGCOLORSPACEW(void)
{
    /* LPLOGCOLORSPACEW */
    TEST_TYPE(LPLOGCOLORSPACEW, 4, 4);
    TEST_TYPE_POINTER(LPLOGCOLORSPACEW, 588, 4);
}

static void test_pack_LPLOGFONTA(void)
{
    /* LPLOGFONTA */
    TEST_TYPE(LPLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(LPLOGFONTA, 60, 4);
}

static void test_pack_LPLOGFONTW(void)
{
    /* LPLOGFONTW */
    TEST_TYPE(LPLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(LPLOGFONTW, 92, 4);
}

static void test_pack_LPLOGPEN(void)
{
    /* LPLOGPEN */
    TEST_TYPE(LPLOGPEN, 4, 4);
    TEST_TYPE_POINTER(LPLOGPEN, 16, 4);
}

static void test_pack_LPMAT2(void)
{
    /* LPMAT2 */
    TEST_TYPE(LPMAT2, 4, 4);
    TEST_TYPE_POINTER(LPMAT2, 16, 2);
}

static void test_pack_LPMETAFILEPICT(void)
{
    /* LPMETAFILEPICT */
    TEST_TYPE(LPMETAFILEPICT, 4, 4);
    TEST_TYPE_POINTER(LPMETAFILEPICT, 16, 4);
}

static void test_pack_LPMETAHEADER(void)
{
    /* LPMETAHEADER */
    TEST_TYPE(LPMETAHEADER, 4, 4);
    TEST_TYPE_POINTER(LPMETAHEADER, 18, 2);
}

static void test_pack_LPMETARECORD(void)
{
    /* LPMETARECORD */
    TEST_TYPE(LPMETARECORD, 4, 4);
    TEST_TYPE_POINTER(LPMETARECORD, 8, 4);
}

static void test_pack_LPNEWTEXTMETRICA(void)
{
    /* LPNEWTEXTMETRICA */
    TEST_TYPE(LPNEWTEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(LPNEWTEXTMETRICA, 72, 4);
}

static void test_pack_LPNEWTEXTMETRICW(void)
{
    /* LPNEWTEXTMETRICW */
    TEST_TYPE(LPNEWTEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(LPNEWTEXTMETRICW, 76, 4);
}

static void test_pack_LPOUTLINETEXTMETRICA(void)
{
    /* LPOUTLINETEXTMETRICA */
    TEST_TYPE(LPOUTLINETEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(LPOUTLINETEXTMETRICA, 212, 4);
}

static void test_pack_LPOUTLINETEXTMETRICW(void)
{
    /* LPOUTLINETEXTMETRICW */
    TEST_TYPE(LPOUTLINETEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(LPOUTLINETEXTMETRICW, 216, 4);
}

static void test_pack_LPPANOSE(void)
{
    /* LPPANOSE */
    TEST_TYPE(LPPANOSE, 4, 4);
    TEST_TYPE_POINTER(LPPANOSE, 10, 1);
}

static void test_pack_LPPELARRAY(void)
{
    /* LPPELARRAY */
    TEST_TYPE(LPPELARRAY, 4, 4);
    TEST_TYPE_POINTER(LPPELARRAY, 20, 4);
}

static void test_pack_LPPIXELFORMATDESCRIPTOR(void)
{
    /* LPPIXELFORMATDESCRIPTOR */
    TEST_TYPE(LPPIXELFORMATDESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(LPPIXELFORMATDESCRIPTOR, 40, 4);
}

static void test_pack_LPPOINTFX(void)
{
    /* LPPOINTFX */
    TEST_TYPE(LPPOINTFX, 4, 4);
    TEST_TYPE_POINTER(LPPOINTFX, 8, 2);
}

static void test_pack_LPPOLYTEXTA(void)
{
    /* LPPOLYTEXTA */
    TEST_TYPE(LPPOLYTEXTA, 4, 4);
    TEST_TYPE_POINTER(LPPOLYTEXTA, 40, 4);
}

static void test_pack_LPPOLYTEXTW(void)
{
    /* LPPOLYTEXTW */
    TEST_TYPE(LPPOLYTEXTW, 4, 4);
    TEST_TYPE_POINTER(LPPOLYTEXTW, 40, 4);
}

static void test_pack_LPRASTERIZER_STATUS(void)
{
    /* LPRASTERIZER_STATUS */
    TEST_TYPE(LPRASTERIZER_STATUS, 4, 4);
    TEST_TYPE_POINTER(LPRASTERIZER_STATUS, 6, 2);
}

static void test_pack_LPRGBQUAD(void)
{
    /* LPRGBQUAD */
    TEST_TYPE(LPRGBQUAD, 4, 4);
    TEST_TYPE_POINTER(LPRGBQUAD, 4, 1);
}

static void test_pack_LPRGNDATA(void)
{
    /* LPRGNDATA */
    TEST_TYPE(LPRGNDATA, 4, 4);
    TEST_TYPE_POINTER(LPRGNDATA, 36, 4);
}

static void test_pack_LPTEXTMETRICA(void)
{
    /* LPTEXTMETRICA */
    TEST_TYPE(LPTEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(LPTEXTMETRICA, 56, 4);
}

static void test_pack_LPTEXTMETRICW(void)
{
    /* LPTEXTMETRICW */
    TEST_TYPE(LPTEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(LPTEXTMETRICW, 60, 4);
}

static void test_pack_LPTRIVERTEX(void)
{
    /* LPTRIVERTEX */
    TEST_TYPE(LPTRIVERTEX, 4, 4);
    TEST_TYPE_POINTER(LPTRIVERTEX, 16, 4);
}

static void test_pack_LPTTPOLYCURVE(void)
{
    /* LPTTPOLYCURVE */
    TEST_TYPE(LPTTPOLYCURVE, 4, 4);
    TEST_TYPE_POINTER(LPTTPOLYCURVE, 12, 2);
}

static void test_pack_LPTTPOLYGONHEADER(void)
{
    /* LPTTPOLYGONHEADER */
    TEST_TYPE(LPTTPOLYGONHEADER, 4, 4);
    TEST_TYPE_POINTER(LPTTPOLYGONHEADER, 16, 4);
}

static void test_pack_LPXFORM(void)
{
    /* LPXFORM */
    TEST_TYPE(LPXFORM, 4, 4);
    TEST_TYPE_POINTER(LPXFORM, 24, 4);
}

static void test_pack_MAT2(void)
{
    /* MAT2 (pack 4) */
    TEST_TYPE(MAT2, 16, 2);
    TEST_FIELD(MAT2, eM11, 0, 4, 2);
    TEST_FIELD(MAT2, eM12, 4, 4, 2);
    TEST_FIELD(MAT2, eM21, 8, 4, 2);
    TEST_FIELD(MAT2, eM22, 12, 4, 2);
}

static void test_pack_METAFILEPICT(void)
{
    /* METAFILEPICT (pack 4) */
    TEST_TYPE(METAFILEPICT, 16, 4);
    TEST_FIELD(METAFILEPICT, mm, 0, 4, 4);
    TEST_FIELD(METAFILEPICT, xExt, 4, 4, 4);
    TEST_FIELD(METAFILEPICT, yExt, 8, 4, 4);
    TEST_FIELD(METAFILEPICT, hMF, 12, 4, 4);
}

static void test_pack_METAHEADER(void)
{
    /* METAHEADER (pack 2) */
    TEST_TYPE(METAHEADER, 18, 2);
    TEST_FIELD(METAHEADER, mtType, 0, 2, 2);
    TEST_FIELD(METAHEADER, mtHeaderSize, 2, 2, 2);
    TEST_FIELD(METAHEADER, mtVersion, 4, 2, 2);
    TEST_FIELD(METAHEADER, mtSize, 6, 4, 2);
    TEST_FIELD(METAHEADER, mtNoObjects, 10, 2, 2);
    TEST_FIELD(METAHEADER, mtMaxRecord, 12, 4, 2);
    TEST_FIELD(METAHEADER, mtNoParameters, 16, 2, 2);
}

static void test_pack_METARECORD(void)
{
    /* METARECORD (pack 4) */
    TEST_TYPE(METARECORD, 8, 4);
    TEST_FIELD(METARECORD, rdSize, 0, 4, 4);
    TEST_FIELD(METARECORD, rdFunction, 4, 2, 2);
    TEST_FIELD(METARECORD, rdParm, 6, 2, 2);
}

static void test_pack_MFENUMPROC(void)
{
    /* MFENUMPROC */
    TEST_TYPE(MFENUMPROC, 4, 4);
}

static void test_pack_NEWTEXTMETRICA(void)
{
    /* NEWTEXTMETRICA (pack 4) */
    TEST_TYPE(NEWTEXTMETRICA, 72, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmHeight, 0, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmAscent, 4, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmDescent, 8, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmInternalLeading, 12, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmExternalLeading, 16, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmAveCharWidth, 20, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmMaxCharWidth, 24, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmWeight, 28, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmOverhang, 32, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmDigitizedAspectX, 36, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmDigitizedAspectY, 40, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, tmFirstChar, 44, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, tmLastChar, 45, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, tmDefaultChar, 46, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, tmBreakChar, 47, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, tmItalic, 48, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, tmUnderlined, 49, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, tmStruckOut, 50, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, tmPitchAndFamily, 51, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, tmCharSet, 52, 1, 1);
    TEST_FIELD(NEWTEXTMETRICA, ntmFlags, 56, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, ntmSizeEM, 60, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, ntmCellHeight, 64, 4, 4);
    TEST_FIELD(NEWTEXTMETRICA, ntmAvgWidth, 68, 4, 4);
}

static void test_pack_NEWTEXTMETRICEXA(void)
{
    /* NEWTEXTMETRICEXA (pack 4) */
    TEST_TYPE(NEWTEXTMETRICEXA, 96, 4);
    TEST_FIELD(NEWTEXTMETRICEXA, ntmTm, 0, 72, 4);
    TEST_FIELD(NEWTEXTMETRICEXA, ntmFontSig, 72, 24, 4);
}

static void test_pack_NEWTEXTMETRICEXW(void)
{
    /* NEWTEXTMETRICEXW (pack 4) */
    TEST_TYPE(NEWTEXTMETRICEXW, 100, 4);
    TEST_FIELD(NEWTEXTMETRICEXW, ntmTm, 0, 76, 4);
    TEST_FIELD(NEWTEXTMETRICEXW, ntmFontSig, 76, 24, 4);
}

static void test_pack_NEWTEXTMETRICW(void)
{
    /* NEWTEXTMETRICW (pack 4) */
    TEST_TYPE(NEWTEXTMETRICW, 76, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmHeight, 0, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmAscent, 4, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmDescent, 8, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmInternalLeading, 12, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmExternalLeading, 16, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmAveCharWidth, 20, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmMaxCharWidth, 24, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmWeight, 28, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmOverhang, 32, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmDigitizedAspectX, 36, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmDigitizedAspectY, 40, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, tmFirstChar, 44, 2, 2);
    TEST_FIELD(NEWTEXTMETRICW, tmLastChar, 46, 2, 2);
    TEST_FIELD(NEWTEXTMETRICW, tmDefaultChar, 48, 2, 2);
    TEST_FIELD(NEWTEXTMETRICW, tmBreakChar, 50, 2, 2);
    TEST_FIELD(NEWTEXTMETRICW, tmItalic, 52, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, tmUnderlined, 53, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, tmStruckOut, 54, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, tmPitchAndFamily, 55, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, tmCharSet, 56, 1, 1);
    TEST_FIELD(NEWTEXTMETRICW, ntmFlags, 60, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, ntmSizeEM, 64, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, ntmCellHeight, 68, 4, 4);
    TEST_FIELD(NEWTEXTMETRICW, ntmAvgWidth, 72, 4, 4);
}

static void test_pack_NPEXTLOGPEN(void)
{
    /* NPEXTLOGPEN */
    TEST_TYPE(NPEXTLOGPEN, 4, 4);
    TEST_TYPE_POINTER(NPEXTLOGPEN, 28, 4);
}

static void test_pack_OLDFONTENUMPROC(void)
{
    /* OLDFONTENUMPROC */
    TEST_TYPE(OLDFONTENUMPROC, 4, 4);
}

static void test_pack_OLDFONTENUMPROCA(void)
{
    /* OLDFONTENUMPROCA */
    TEST_TYPE(OLDFONTENUMPROCA, 4, 4);
}

static void test_pack_OLDFONTENUMPROCW(void)
{
    /* OLDFONTENUMPROCW */
    TEST_TYPE(OLDFONTENUMPROCW, 4, 4);
}

static void test_pack_OUTLINETEXTMETRICA(void)
{
    /* OUTLINETEXTMETRICA (pack 4) */
    TEST_TYPE(OUTLINETEXTMETRICA, 212, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmSize, 0, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmTextMetrics, 4, 56, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmFiller, 60, 1, 1);
    TEST_FIELD(OUTLINETEXTMETRICA, otmPanoseNumber, 61, 10, 1);
    TEST_FIELD(OUTLINETEXTMETRICA, otmfsSelection, 72, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmfsType, 76, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmsCharSlopeRise, 80, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmsCharSlopeRun, 84, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmItalicAngle, 88, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmEMSquare, 92, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmAscent, 96, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmDescent, 100, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmLineGap, 104, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmsCapEmHeight, 108, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmsXHeight, 112, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmrcFontBox, 116, 16, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmMacAscent, 132, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmMacDescent, 136, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmMacLineGap, 140, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmusMinimumPPEM, 144, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmptSubscriptSize, 148, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmptSubscriptOffset, 156, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmptSuperscriptSize, 164, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmptSuperscriptOffset, 172, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmsStrikeoutSize, 180, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmsStrikeoutPosition, 184, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmsUnderscoreSize, 188, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmsUnderscorePosition, 192, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmpFamilyName, 196, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmpFaceName, 200, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmpStyleName, 204, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICA, otmpFullName, 208, 4, 4);
}

static void test_pack_OUTLINETEXTMETRICW(void)
{
    /* OUTLINETEXTMETRICW (pack 4) */
    TEST_TYPE(OUTLINETEXTMETRICW, 216, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmSize, 0, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmTextMetrics, 4, 60, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmFiller, 64, 1, 1);
    TEST_FIELD(OUTLINETEXTMETRICW, otmPanoseNumber, 65, 10, 1);
    TEST_FIELD(OUTLINETEXTMETRICW, otmfsSelection, 76, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmfsType, 80, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmsCharSlopeRise, 84, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmsCharSlopeRun, 88, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmItalicAngle, 92, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmEMSquare, 96, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmAscent, 100, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmDescent, 104, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmLineGap, 108, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmsCapEmHeight, 112, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmsXHeight, 116, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmrcFontBox, 120, 16, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmMacAscent, 136, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmMacDescent, 140, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmMacLineGap, 144, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmusMinimumPPEM, 148, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmptSubscriptSize, 152, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmptSubscriptOffset, 160, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmptSuperscriptSize, 168, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmptSuperscriptOffset, 176, 8, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmsStrikeoutSize, 184, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmsStrikeoutPosition, 188, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmsUnderscoreSize, 192, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmsUnderscorePosition, 196, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmpFamilyName, 200, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmpFaceName, 204, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmpStyleName, 208, 4, 4);
    TEST_FIELD(OUTLINETEXTMETRICW, otmpFullName, 212, 4, 4);
}

static void test_pack_PABC(void)
{
    /* PABC */
    TEST_TYPE(PABC, 4, 4);
    TEST_TYPE_POINTER(PABC, 12, 4);
}

static void test_pack_PABCFLOAT(void)
{
    /* PABCFLOAT */
    TEST_TYPE(PABCFLOAT, 4, 4);
    TEST_TYPE_POINTER(PABCFLOAT, 12, 4);
}

static void test_pack_PANOSE(void)
{
    /* PANOSE (pack 4) */
    TEST_TYPE(PANOSE, 10, 1);
    TEST_FIELD(PANOSE, bFamilyType, 0, 1, 1);
    TEST_FIELD(PANOSE, bSerifStyle, 1, 1, 1);
    TEST_FIELD(PANOSE, bWeight, 2, 1, 1);
    TEST_FIELD(PANOSE, bProportion, 3, 1, 1);
    TEST_FIELD(PANOSE, bContrast, 4, 1, 1);
    TEST_FIELD(PANOSE, bStrokeVariation, 5, 1, 1);
    TEST_FIELD(PANOSE, bArmStyle, 6, 1, 1);
    TEST_FIELD(PANOSE, bLetterform, 7, 1, 1);
    TEST_FIELD(PANOSE, bMidline, 8, 1, 1);
    TEST_FIELD(PANOSE, bXHeight, 9, 1, 1);
}

static void test_pack_PATTERN(void)
{
    /* PATTERN */
    TEST_TYPE(PATTERN, 12, 4);
}

static void test_pack_PBITMAP(void)
{
    /* PBITMAP */
    TEST_TYPE(PBITMAP, 4, 4);
    TEST_TYPE_POINTER(PBITMAP, 24, 4);
}

static void test_pack_PBITMAPCOREHEADER(void)
{
    /* PBITMAPCOREHEADER */
    TEST_TYPE(PBITMAPCOREHEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPCOREHEADER, 12, 4);
}

static void test_pack_PBITMAPCOREINFO(void)
{
    /* PBITMAPCOREINFO */
    TEST_TYPE(PBITMAPCOREINFO, 4, 4);
    TEST_TYPE_POINTER(PBITMAPCOREINFO, 16, 4);
}

static void test_pack_PBITMAPFILEHEADER(void)
{
    /* PBITMAPFILEHEADER */
    TEST_TYPE(PBITMAPFILEHEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPFILEHEADER, 14, 2);
}

static void test_pack_PBITMAPINFO(void)
{
    /* PBITMAPINFO */
    TEST_TYPE(PBITMAPINFO, 4, 4);
    TEST_TYPE_POINTER(PBITMAPINFO, 44, 4);
}

static void test_pack_PBITMAPINFOHEADER(void)
{
    /* PBITMAPINFOHEADER */
    TEST_TYPE(PBITMAPINFOHEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPINFOHEADER, 40, 4);
}

static void test_pack_PBITMAPV4HEADER(void)
{
    /* PBITMAPV4HEADER */
    TEST_TYPE(PBITMAPV4HEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPV4HEADER, 108, 4);
}

static void test_pack_PBITMAPV5HEADER(void)
{
    /* PBITMAPV5HEADER */
    TEST_TYPE(PBITMAPV5HEADER, 4, 4);
    TEST_TYPE_POINTER(PBITMAPV5HEADER, 124, 4);
}

static void test_pack_PBLENDFUNCTION(void)
{
    /* PBLENDFUNCTION */
    TEST_TYPE(PBLENDFUNCTION, 4, 4);
    TEST_TYPE_POINTER(PBLENDFUNCTION, 4, 1);
}

static void test_pack_PCHARSETINFO(void)
{
    /* PCHARSETINFO */
    TEST_TYPE(PCHARSETINFO, 4, 4);
    TEST_TYPE_POINTER(PCHARSETINFO, 32, 4);
}

static void test_pack_PCOLORADJUSTMENT(void)
{
    /* PCOLORADJUSTMENT */
    TEST_TYPE(PCOLORADJUSTMENT, 4, 4);
    TEST_TYPE_POINTER(PCOLORADJUSTMENT, 24, 2);
}

static void test_pack_PDEVMODEA(void)
{
    /* PDEVMODEA */
    TEST_TYPE(PDEVMODEA, 4, 4);
}

static void test_pack_PDEVMODEW(void)
{
    /* PDEVMODEW */
    TEST_TYPE(PDEVMODEW, 4, 4);
}

static void test_pack_PDIBSECTION(void)
{
    /* PDIBSECTION */
    TEST_TYPE(PDIBSECTION, 4, 4);
    TEST_TYPE_POINTER(PDIBSECTION, 84, 4);
}

static void test_pack_PDISPLAY_DEVICEA(void)
{
    /* PDISPLAY_DEVICEA */
    TEST_TYPE(PDISPLAY_DEVICEA, 4, 4);
    TEST_TYPE_POINTER(PDISPLAY_DEVICEA, 424, 4);
}

static void test_pack_PDISPLAY_DEVICEW(void)
{
    /* PDISPLAY_DEVICEW */
    TEST_TYPE(PDISPLAY_DEVICEW, 4, 4);
    TEST_TYPE_POINTER(PDISPLAY_DEVICEW, 840, 4);
}

static void test_pack_PELARRAY(void)
{
    /* PELARRAY (pack 4) */
    TEST_TYPE(PELARRAY, 20, 4);
    TEST_FIELD(PELARRAY, paXCount, 0, 4, 4);
    TEST_FIELD(PELARRAY, paYCount, 4, 4, 4);
    TEST_FIELD(PELARRAY, paXExt, 8, 4, 4);
    TEST_FIELD(PELARRAY, paYExt, 12, 4, 4);
    TEST_FIELD(PELARRAY, paRGBs, 16, 1, 1);
}

static void test_pack_PEMR(void)
{
    /* PEMR */
    TEST_TYPE(PEMR, 4, 4);
    TEST_TYPE_POINTER(PEMR, 8, 4);
}

static void test_pack_PEMRABORTPATH(void)
{
    /* PEMRABORTPATH */
    TEST_TYPE(PEMRABORTPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRABORTPATH, 8, 4);
}

static void test_pack_PEMRANGLEARC(void)
{
    /* PEMRANGLEARC */
    TEST_TYPE(PEMRANGLEARC, 4, 4);
    TEST_TYPE_POINTER(PEMRANGLEARC, 28, 4);
}

static void test_pack_PEMRARC(void)
{
    /* PEMRARC */
    TEST_TYPE(PEMRARC, 4, 4);
    TEST_TYPE_POINTER(PEMRARC, 40, 4);
}

static void test_pack_PEMRARCTO(void)
{
    /* PEMRARCTO */
    TEST_TYPE(PEMRARCTO, 4, 4);
    TEST_TYPE_POINTER(PEMRARCTO, 40, 4);
}

static void test_pack_PEMRBEGINPATH(void)
{
    /* PEMRBEGINPATH */
    TEST_TYPE(PEMRBEGINPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRBEGINPATH, 8, 4);
}

static void test_pack_PEMRBITBLT(void)
{
    /* PEMRBITBLT */
    TEST_TYPE(PEMRBITBLT, 4, 4);
    TEST_TYPE_POINTER(PEMRBITBLT, 100, 4);
}

static void test_pack_PEMRCHORD(void)
{
    /* PEMRCHORD */
    TEST_TYPE(PEMRCHORD, 4, 4);
    TEST_TYPE_POINTER(PEMRCHORD, 40, 4);
}

static void test_pack_PEMRCLOSEFIGURE(void)
{
    /* PEMRCLOSEFIGURE */
    TEST_TYPE(PEMRCLOSEFIGURE, 4, 4);
    TEST_TYPE_POINTER(PEMRCLOSEFIGURE, 8, 4);
}

static void test_pack_PEMRCREATEBRUSHINDIRECT(void)
{
    /* PEMRCREATEBRUSHINDIRECT */
    TEST_TYPE(PEMRCREATEBRUSHINDIRECT, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEBRUSHINDIRECT, 24, 4);
}

static void test_pack_PEMRCREATECOLORSPACE(void)
{
    /* PEMRCREATECOLORSPACE */
    TEST_TYPE(PEMRCREATECOLORSPACE, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATECOLORSPACE, 340, 4);
}

static void test_pack_PEMRCREATECOLORSPACEW(void)
{
    /* PEMRCREATECOLORSPACEW */
    TEST_TYPE(PEMRCREATECOLORSPACEW, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATECOLORSPACEW, 612, 4);
}

static void test_pack_PEMRCREATEDIBPATTERNBRUSHPT(void)
{
    /* PEMRCREATEDIBPATTERNBRUSHPT */
    TEST_TYPE(PEMRCREATEDIBPATTERNBRUSHPT, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEDIBPATTERNBRUSHPT, 32, 4);
}

static void test_pack_PEMRCREATEMONOBRUSH(void)
{
    /* PEMRCREATEMONOBRUSH */
    TEST_TYPE(PEMRCREATEMONOBRUSH, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEMONOBRUSH, 32, 4);
}

static void test_pack_PEMRCREATEPALETTE(void)
{
    /* PEMRCREATEPALETTE */
    TEST_TYPE(PEMRCREATEPALETTE, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEPALETTE, 20, 4);
}

static void test_pack_PEMRCREATEPEN(void)
{
    /* PEMRCREATEPEN */
    TEST_TYPE(PEMRCREATEPEN, 4, 4);
    TEST_TYPE_POINTER(PEMRCREATEPEN, 28, 4);
}

static void test_pack_PEMRDELETECOLORSPACE(void)
{
    /* PEMRDELETECOLORSPACE */
    TEST_TYPE(PEMRDELETECOLORSPACE, 4, 4);
    TEST_TYPE_POINTER(PEMRDELETECOLORSPACE, 12, 4);
}

static void test_pack_PEMRDELETEOBJECT(void)
{
    /* PEMRDELETEOBJECT */
    TEST_TYPE(PEMRDELETEOBJECT, 4, 4);
    TEST_TYPE_POINTER(PEMRDELETEOBJECT, 12, 4);
}

static void test_pack_PEMRELLIPSE(void)
{
    /* PEMRELLIPSE */
    TEST_TYPE(PEMRELLIPSE, 4, 4);
    TEST_TYPE_POINTER(PEMRELLIPSE, 24, 4);
}

static void test_pack_PEMRENDPATH(void)
{
    /* PEMRENDPATH */
    TEST_TYPE(PEMRENDPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRENDPATH, 8, 4);
}

static void test_pack_PEMREOF(void)
{
    /* PEMREOF */
    TEST_TYPE(PEMREOF, 4, 4);
    TEST_TYPE_POINTER(PEMREOF, 20, 4);
}

static void test_pack_PEMREXCLUDECLIPRECT(void)
{
    /* PEMREXCLUDECLIPRECT */
    TEST_TYPE(PEMREXCLUDECLIPRECT, 4, 4);
    TEST_TYPE_POINTER(PEMREXCLUDECLIPRECT, 24, 4);
}

static void test_pack_PEMREXTCREATEFONTINDIRECTW(void)
{
    /* PEMREXTCREATEFONTINDIRECTW */
    TEST_TYPE(PEMREXTCREATEFONTINDIRECTW, 4, 4);
    TEST_TYPE_POINTER(PEMREXTCREATEFONTINDIRECTW, 332, 4);
}

static void test_pack_PEMREXTCREATEPEN(void)
{
    /* PEMREXTCREATEPEN */
    TEST_TYPE(PEMREXTCREATEPEN, 4, 4);
    TEST_TYPE_POINTER(PEMREXTCREATEPEN, 56, 4);
}

static void test_pack_PEMREXTFLOODFILL(void)
{
    /* PEMREXTFLOODFILL */
    TEST_TYPE(PEMREXTFLOODFILL, 4, 4);
    TEST_TYPE_POINTER(PEMREXTFLOODFILL, 24, 4);
}

static void test_pack_PEMREXTSELECTCLIPRGN(void)
{
    /* PEMREXTSELECTCLIPRGN */
    TEST_TYPE(PEMREXTSELECTCLIPRGN, 4, 4);
    TEST_TYPE_POINTER(PEMREXTSELECTCLIPRGN, 20, 4);
}

static void test_pack_PEMREXTTEXTOUTA(void)
{
    /* PEMREXTTEXTOUTA */
    TEST_TYPE(PEMREXTTEXTOUTA, 4, 4);
    TEST_TYPE_POINTER(PEMREXTTEXTOUTA, 76, 4);
}

static void test_pack_PEMREXTTEXTOUTW(void)
{
    /* PEMREXTTEXTOUTW */
    TEST_TYPE(PEMREXTTEXTOUTW, 4, 4);
    TEST_TYPE_POINTER(PEMREXTTEXTOUTW, 76, 4);
}

static void test_pack_PEMRFILLPATH(void)
{
    /* PEMRFILLPATH */
    TEST_TYPE(PEMRFILLPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRFILLPATH, 24, 4);
}

static void test_pack_PEMRFILLRGN(void)
{
    /* PEMRFILLRGN */
    TEST_TYPE(PEMRFILLRGN, 4, 4);
    TEST_TYPE_POINTER(PEMRFILLRGN, 36, 4);
}

static void test_pack_PEMRFLATTENPATH(void)
{
    /* PEMRFLATTENPATH */
    TEST_TYPE(PEMRFLATTENPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRFLATTENPATH, 8, 4);
}

static void test_pack_PEMRFORMAT(void)
{
    /* PEMRFORMAT */
    TEST_TYPE(PEMRFORMAT, 4, 4);
    TEST_TYPE_POINTER(PEMRFORMAT, 16, 4);
}

static void test_pack_PEMRFRAMERGN(void)
{
    /* PEMRFRAMERGN */
    TEST_TYPE(PEMRFRAMERGN, 4, 4);
    TEST_TYPE_POINTER(PEMRFRAMERGN, 44, 4);
}

static void test_pack_PEMRGDICOMMENT(void)
{
    /* PEMRGDICOMMENT */
    TEST_TYPE(PEMRGDICOMMENT, 4, 4);
    TEST_TYPE_POINTER(PEMRGDICOMMENT, 16, 4);
}

static void test_pack_PEMRGLSBOUNDEDRECORD(void)
{
    /* PEMRGLSBOUNDEDRECORD */
    TEST_TYPE(PEMRGLSBOUNDEDRECORD, 4, 4);
    TEST_TYPE_POINTER(PEMRGLSBOUNDEDRECORD, 32, 4);
}

static void test_pack_PEMRGLSRECORD(void)
{
    /* PEMRGLSRECORD */
    TEST_TYPE(PEMRGLSRECORD, 4, 4);
    TEST_TYPE_POINTER(PEMRGLSRECORD, 16, 4);
}

static void test_pack_PEMRINTERSECTCLIPRECT(void)
{
    /* PEMRINTERSECTCLIPRECT */
    TEST_TYPE(PEMRINTERSECTCLIPRECT, 4, 4);
    TEST_TYPE_POINTER(PEMRINTERSECTCLIPRECT, 24, 4);
}

static void test_pack_PEMRINVERTRGN(void)
{
    /* PEMRINVERTRGN */
    TEST_TYPE(PEMRINVERTRGN, 4, 4);
    TEST_TYPE_POINTER(PEMRINVERTRGN, 32, 4);
}

static void test_pack_PEMRLINETO(void)
{
    /* PEMRLINETO */
    TEST_TYPE(PEMRLINETO, 4, 4);
    TEST_TYPE_POINTER(PEMRLINETO, 16, 4);
}

static void test_pack_PEMRMASKBLT(void)
{
    /* PEMRMASKBLT */
    TEST_TYPE(PEMRMASKBLT, 4, 4);
    TEST_TYPE_POINTER(PEMRMASKBLT, 128, 4);
}

static void test_pack_PEMRMODIFYWORLDTRANSFORM(void)
{
    /* PEMRMODIFYWORLDTRANSFORM */
    TEST_TYPE(PEMRMODIFYWORLDTRANSFORM, 4, 4);
    TEST_TYPE_POINTER(PEMRMODIFYWORLDTRANSFORM, 36, 4);
}

static void test_pack_PEMRMOVETOEX(void)
{
    /* PEMRMOVETOEX */
    TEST_TYPE(PEMRMOVETOEX, 4, 4);
    TEST_TYPE_POINTER(PEMRMOVETOEX, 16, 4);
}

static void test_pack_PEMROFFSETCLIPRGN(void)
{
    /* PEMROFFSETCLIPRGN */
    TEST_TYPE(PEMROFFSETCLIPRGN, 4, 4);
    TEST_TYPE_POINTER(PEMROFFSETCLIPRGN, 16, 4);
}

static void test_pack_PEMRPAINTRGN(void)
{
    /* PEMRPAINTRGN */
    TEST_TYPE(PEMRPAINTRGN, 4, 4);
    TEST_TYPE_POINTER(PEMRPAINTRGN, 32, 4);
}

static void test_pack_PEMRPIE(void)
{
    /* PEMRPIE */
    TEST_TYPE(PEMRPIE, 4, 4);
    TEST_TYPE_POINTER(PEMRPIE, 40, 4);
}

static void test_pack_PEMRPIXELFORMAT(void)
{
    /* PEMRPIXELFORMAT */
    TEST_TYPE(PEMRPIXELFORMAT, 4, 4);
    TEST_TYPE_POINTER(PEMRPIXELFORMAT, 48, 4);
}

static void test_pack_PEMRPLGBLT(void)
{
    /* PEMRPLGBLT */
    TEST_TYPE(PEMRPLGBLT, 4, 4);
    TEST_TYPE_POINTER(PEMRPLGBLT, 140, 4);
}

static void test_pack_PEMRPOLYBEZIER(void)
{
    /* PEMRPOLYBEZIER */
    TEST_TYPE(PEMRPOLYBEZIER, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYBEZIER, 36, 4);
}

static void test_pack_PEMRPOLYBEZIER16(void)
{
    /* PEMRPOLYBEZIER16 */
    TEST_TYPE(PEMRPOLYBEZIER16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYBEZIER16, 32, 4);
}

static void test_pack_PEMRPOLYBEZIERTO(void)
{
    /* PEMRPOLYBEZIERTO */
    TEST_TYPE(PEMRPOLYBEZIERTO, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYBEZIERTO, 36, 4);
}

static void test_pack_PEMRPOLYBEZIERTO16(void)
{
    /* PEMRPOLYBEZIERTO16 */
    TEST_TYPE(PEMRPOLYBEZIERTO16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYBEZIERTO16, 32, 4);
}

static void test_pack_PEMRPOLYDRAW(void)
{
    /* PEMRPOLYDRAW */
    TEST_TYPE(PEMRPOLYDRAW, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYDRAW, 40, 4);
}

static void test_pack_PEMRPOLYDRAW16(void)
{
    /* PEMRPOLYDRAW16 */
    TEST_TYPE(PEMRPOLYDRAW16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYDRAW16, 36, 4);
}

static void test_pack_PEMRPOLYGON(void)
{
    /* PEMRPOLYGON */
    TEST_TYPE(PEMRPOLYGON, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYGON, 36, 4);
}

static void test_pack_PEMRPOLYGON16(void)
{
    /* PEMRPOLYGON16 */
    TEST_TYPE(PEMRPOLYGON16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYGON16, 32, 4);
}

static void test_pack_PEMRPOLYLINE(void)
{
    /* PEMRPOLYLINE */
    TEST_TYPE(PEMRPOLYLINE, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYLINE, 36, 4);
}

static void test_pack_PEMRPOLYLINE16(void)
{
    /* PEMRPOLYLINE16 */
    TEST_TYPE(PEMRPOLYLINE16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYLINE16, 32, 4);
}

static void test_pack_PEMRPOLYLINETO(void)
{
    /* PEMRPOLYLINETO */
    TEST_TYPE(PEMRPOLYLINETO, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYLINETO, 36, 4);
}

static void test_pack_PEMRPOLYLINETO16(void)
{
    /* PEMRPOLYLINETO16 */
    TEST_TYPE(PEMRPOLYLINETO16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYLINETO16, 32, 4);
}

static void test_pack_PEMRPOLYPOLYGON(void)
{
    /* PEMRPOLYPOLYGON */
    TEST_TYPE(PEMRPOLYPOLYGON, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYPOLYGON, 44, 4);
}

static void test_pack_PEMRPOLYPOLYGON16(void)
{
    /* PEMRPOLYPOLYGON16 */
    TEST_TYPE(PEMRPOLYPOLYGON16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYPOLYGON16, 40, 4);
}

static void test_pack_PEMRPOLYPOLYLINE(void)
{
    /* PEMRPOLYPOLYLINE */
    TEST_TYPE(PEMRPOLYPOLYLINE, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYPOLYLINE, 44, 4);
}

static void test_pack_PEMRPOLYPOLYLINE16(void)
{
    /* PEMRPOLYPOLYLINE16 */
    TEST_TYPE(PEMRPOLYPOLYLINE16, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYPOLYLINE16, 40, 4);
}

static void test_pack_PEMRPOLYTEXTOUTA(void)
{
    /* PEMRPOLYTEXTOUTA */
    TEST_TYPE(PEMRPOLYTEXTOUTA, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYTEXTOUTA, 80, 4);
}

static void test_pack_PEMRPOLYTEXTOUTW(void)
{
    /* PEMRPOLYTEXTOUTW */
    TEST_TYPE(PEMRPOLYTEXTOUTW, 4, 4);
    TEST_TYPE_POINTER(PEMRPOLYTEXTOUTW, 80, 4);
}

static void test_pack_PEMRREALIZEPALETTE(void)
{
    /* PEMRREALIZEPALETTE */
    TEST_TYPE(PEMRREALIZEPALETTE, 4, 4);
    TEST_TYPE_POINTER(PEMRREALIZEPALETTE, 8, 4);
}

static void test_pack_PEMRRECTANGLE(void)
{
    /* PEMRRECTANGLE */
    TEST_TYPE(PEMRRECTANGLE, 4, 4);
    TEST_TYPE_POINTER(PEMRRECTANGLE, 24, 4);
}

static void test_pack_PEMRRESIZEPALETTE(void)
{
    /* PEMRRESIZEPALETTE */
    TEST_TYPE(PEMRRESIZEPALETTE, 4, 4);
    TEST_TYPE_POINTER(PEMRRESIZEPALETTE, 16, 4);
}

static void test_pack_PEMRRESTOREDC(void)
{
    /* PEMRRESTOREDC */
    TEST_TYPE(PEMRRESTOREDC, 4, 4);
    TEST_TYPE_POINTER(PEMRRESTOREDC, 12, 4);
}

static void test_pack_PEMRROUNDRECT(void)
{
    /* PEMRROUNDRECT */
    TEST_TYPE(PEMRROUNDRECT, 4, 4);
    TEST_TYPE_POINTER(PEMRROUNDRECT, 32, 4);
}

static void test_pack_PEMRSAVEDC(void)
{
    /* PEMRSAVEDC */
    TEST_TYPE(PEMRSAVEDC, 4, 4);
    TEST_TYPE_POINTER(PEMRSAVEDC, 8, 4);
}

static void test_pack_PEMRSCALEVIEWPORTEXTEX(void)
{
    /* PEMRSCALEVIEWPORTEXTEX */
    TEST_TYPE(PEMRSCALEVIEWPORTEXTEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSCALEVIEWPORTEXTEX, 24, 4);
}

static void test_pack_PEMRSCALEWINDOWEXTEX(void)
{
    /* PEMRSCALEWINDOWEXTEX */
    TEST_TYPE(PEMRSCALEWINDOWEXTEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSCALEWINDOWEXTEX, 24, 4);
}

static void test_pack_PEMRSELECTCLIPPATH(void)
{
    /* PEMRSELECTCLIPPATH */
    TEST_TYPE(PEMRSELECTCLIPPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRSELECTCLIPPATH, 12, 4);
}

static void test_pack_PEMRSELECTCOLORSPACE(void)
{
    /* PEMRSELECTCOLORSPACE */
    TEST_TYPE(PEMRSELECTCOLORSPACE, 4, 4);
    TEST_TYPE_POINTER(PEMRSELECTCOLORSPACE, 12, 4);
}

static void test_pack_PEMRSELECTOBJECT(void)
{
    /* PEMRSELECTOBJECT */
    TEST_TYPE(PEMRSELECTOBJECT, 4, 4);
    TEST_TYPE_POINTER(PEMRSELECTOBJECT, 12, 4);
}

static void test_pack_PEMRSELECTPALETTE(void)
{
    /* PEMRSELECTPALETTE */
    TEST_TYPE(PEMRSELECTPALETTE, 4, 4);
    TEST_TYPE_POINTER(PEMRSELECTPALETTE, 12, 4);
}

static void test_pack_PEMRSETARCDIRECTION(void)
{
    /* PEMRSETARCDIRECTION */
    TEST_TYPE(PEMRSETARCDIRECTION, 4, 4);
    TEST_TYPE_POINTER(PEMRSETARCDIRECTION, 12, 4);
}

static void test_pack_PEMRSETBKCOLOR(void)
{
    /* PEMRSETBKCOLOR */
    TEST_TYPE(PEMRSETBKCOLOR, 4, 4);
    TEST_TYPE_POINTER(PEMRSETBKCOLOR, 12, 4);
}

static void test_pack_PEMRSETBKMODE(void)
{
    /* PEMRSETBKMODE */
    TEST_TYPE(PEMRSETBKMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETBKMODE, 12, 4);
}

static void test_pack_PEMRSETBRUSHORGEX(void)
{
    /* PEMRSETBRUSHORGEX */
    TEST_TYPE(PEMRSETBRUSHORGEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETBRUSHORGEX, 16, 4);
}

static void test_pack_PEMRSETCOLORADJUSTMENT(void)
{
    /* PEMRSETCOLORADJUSTMENT */
    TEST_TYPE(PEMRSETCOLORADJUSTMENT, 4, 4);
    TEST_TYPE_POINTER(PEMRSETCOLORADJUSTMENT, 32, 4);
}

static void test_pack_PEMRSETCOLORSPACE(void)
{
    /* PEMRSETCOLORSPACE */
    TEST_TYPE(PEMRSETCOLORSPACE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETCOLORSPACE, 12, 4);
}

static void test_pack_PEMRSETDIBITSTODEVICE(void)
{
    /* PEMRSETDIBITSTODEVICE */
    TEST_TYPE(PEMRSETDIBITSTODEVICE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETDIBITSTODEVICE, 76, 4);
}

static void test_pack_PEMRSETICMMODE(void)
{
    /* PEMRSETICMMODE */
    TEST_TYPE(PEMRSETICMMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETICMMODE, 12, 4);
}

static void test_pack_PEMRSETLAYOUT(void)
{
    /* PEMRSETLAYOUT */
    TEST_TYPE(PEMRSETLAYOUT, 4, 4);
    TEST_TYPE_POINTER(PEMRSETLAYOUT, 12, 4);
}

static void test_pack_PEMRSETMAPMODE(void)
{
    /* PEMRSETMAPMODE */
    TEST_TYPE(PEMRSETMAPMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETMAPMODE, 12, 4);
}

static void test_pack_PEMRSETMAPPERFLAGS(void)
{
    /* PEMRSETMAPPERFLAGS */
    TEST_TYPE(PEMRSETMAPPERFLAGS, 4, 4);
    TEST_TYPE_POINTER(PEMRSETMAPPERFLAGS, 12, 4);
}

static void test_pack_PEMRSETMETARGN(void)
{
    /* PEMRSETMETARGN */
    TEST_TYPE(PEMRSETMETARGN, 4, 4);
    TEST_TYPE_POINTER(PEMRSETMETARGN, 8, 4);
}

static void test_pack_PEMRSETMITERLIMIT(void)
{
    /* PEMRSETMITERLIMIT */
    TEST_TYPE(PEMRSETMITERLIMIT, 4, 4);
    TEST_TYPE_POINTER(PEMRSETMITERLIMIT, 12, 4);
}

static void test_pack_PEMRSETPALETTEENTRIES(void)
{
    /* PEMRSETPALETTEENTRIES */
    TEST_TYPE(PEMRSETPALETTEENTRIES, 4, 4);
    TEST_TYPE_POINTER(PEMRSETPALETTEENTRIES, 24, 4);
}

static void test_pack_PEMRSETPIXELV(void)
{
    /* PEMRSETPIXELV */
    TEST_TYPE(PEMRSETPIXELV, 4, 4);
    TEST_TYPE_POINTER(PEMRSETPIXELV, 20, 4);
}

static void test_pack_PEMRSETPOLYFILLMODE(void)
{
    /* PEMRSETPOLYFILLMODE */
    TEST_TYPE(PEMRSETPOLYFILLMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETPOLYFILLMODE, 12, 4);
}

static void test_pack_PEMRSETROP2(void)
{
    /* PEMRSETROP2 */
    TEST_TYPE(PEMRSETROP2, 4, 4);
    TEST_TYPE_POINTER(PEMRSETROP2, 12, 4);
}

static void test_pack_PEMRSETSTRETCHBLTMODE(void)
{
    /* PEMRSETSTRETCHBLTMODE */
    TEST_TYPE(PEMRSETSTRETCHBLTMODE, 4, 4);
    TEST_TYPE_POINTER(PEMRSETSTRETCHBLTMODE, 12, 4);
}

static void test_pack_PEMRSETTEXTALIGN(void)
{
    /* PEMRSETTEXTALIGN */
    TEST_TYPE(PEMRSETTEXTALIGN, 4, 4);
    TEST_TYPE_POINTER(PEMRSETTEXTALIGN, 12, 4);
}

static void test_pack_PEMRSETTEXTCOLOR(void)
{
    /* PEMRSETTEXTCOLOR */
    TEST_TYPE(PEMRSETTEXTCOLOR, 4, 4);
    TEST_TYPE_POINTER(PEMRSETTEXTCOLOR, 12, 4);
}

static void test_pack_PEMRSETVIEWPORTEXTEX(void)
{
    /* PEMRSETVIEWPORTEXTEX */
    TEST_TYPE(PEMRSETVIEWPORTEXTEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETVIEWPORTEXTEX, 16, 4);
}

static void test_pack_PEMRSETVIEWPORTORGEX(void)
{
    /* PEMRSETVIEWPORTORGEX */
    TEST_TYPE(PEMRSETVIEWPORTORGEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETVIEWPORTORGEX, 16, 4);
}

static void test_pack_PEMRSETWINDOWEXTEX(void)
{
    /* PEMRSETWINDOWEXTEX */
    TEST_TYPE(PEMRSETWINDOWEXTEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETWINDOWEXTEX, 16, 4);
}

static void test_pack_PEMRSETWINDOWORGEX(void)
{
    /* PEMRSETWINDOWORGEX */
    TEST_TYPE(PEMRSETWINDOWORGEX, 4, 4);
    TEST_TYPE_POINTER(PEMRSETWINDOWORGEX, 16, 4);
}

static void test_pack_PEMRSETWORLDTRANSFORM(void)
{
    /* PEMRSETWORLDTRANSFORM */
    TEST_TYPE(PEMRSETWORLDTRANSFORM, 4, 4);
    TEST_TYPE_POINTER(PEMRSETWORLDTRANSFORM, 32, 4);
}

static void test_pack_PEMRSTRETCHBLT(void)
{
    /* PEMRSTRETCHBLT */
    TEST_TYPE(PEMRSTRETCHBLT, 4, 4);
    TEST_TYPE_POINTER(PEMRSTRETCHBLT, 108, 4);
}

static void test_pack_PEMRSTRETCHDIBITS(void)
{
    /* PEMRSTRETCHDIBITS */
    TEST_TYPE(PEMRSTRETCHDIBITS, 4, 4);
    TEST_TYPE_POINTER(PEMRSTRETCHDIBITS, 80, 4);
}

static void test_pack_PEMRSTROKEANDFILLPATH(void)
{
    /* PEMRSTROKEANDFILLPATH */
    TEST_TYPE(PEMRSTROKEANDFILLPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRSTROKEANDFILLPATH, 24, 4);
}

static void test_pack_PEMRSTROKEPATH(void)
{
    /* PEMRSTROKEPATH */
    TEST_TYPE(PEMRSTROKEPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRSTROKEPATH, 24, 4);
}

static void test_pack_PEMRTEXT(void)
{
    /* PEMRTEXT */
    TEST_TYPE(PEMRTEXT, 4, 4);
    TEST_TYPE_POINTER(PEMRTEXT, 40, 4);
}

static void test_pack_PEMRWIDENPATH(void)
{
    /* PEMRWIDENPATH */
    TEST_TYPE(PEMRWIDENPATH, 4, 4);
    TEST_TYPE_POINTER(PEMRWIDENPATH, 8, 4);
}

static void test_pack_PENHMETAHEADER(void)
{
    /* PENHMETAHEADER */
    TEST_TYPE(PENHMETAHEADER, 4, 4);
    TEST_TYPE_POINTER(PENHMETAHEADER, 108, 4);
}

static void test_pack_PEXTLOGFONTA(void)
{
    /* PEXTLOGFONTA */
    TEST_TYPE(PEXTLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(PEXTLOGFONTA, 192, 4);
}

static void test_pack_PEXTLOGFONTW(void)
{
    /* PEXTLOGFONTW */
    TEST_TYPE(PEXTLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(PEXTLOGFONTW, 320, 4);
}

static void test_pack_PEXTLOGPEN(void)
{
    /* PEXTLOGPEN */
    TEST_TYPE(PEXTLOGPEN, 4, 4);
    TEST_TYPE_POINTER(PEXTLOGPEN, 28, 4);
}

static void test_pack_PFONTSIGNATURE(void)
{
    /* PFONTSIGNATURE */
    TEST_TYPE(PFONTSIGNATURE, 4, 4);
    TEST_TYPE_POINTER(PFONTSIGNATURE, 24, 4);
}

static void test_pack_PGLYPHMETRICSFLOAT(void)
{
    /* PGLYPHMETRICSFLOAT */
    TEST_TYPE(PGLYPHMETRICSFLOAT, 4, 4);
    TEST_TYPE_POINTER(PGLYPHMETRICSFLOAT, 24, 4);
}

static void test_pack_PGRADIENT_RECT(void)
{
    /* PGRADIENT_RECT */
    TEST_TYPE(PGRADIENT_RECT, 4, 4);
    TEST_TYPE_POINTER(PGRADIENT_RECT, 8, 4);
}

static void test_pack_PGRADIENT_TRIANGLE(void)
{
    /* PGRADIENT_TRIANGLE */
    TEST_TYPE(PGRADIENT_TRIANGLE, 4, 4);
    TEST_TYPE_POINTER(PGRADIENT_TRIANGLE, 12, 4);
}

static void test_pack_PHANDLETABLE(void)
{
    /* PHANDLETABLE */
    TEST_TYPE(PHANDLETABLE, 4, 4);
    TEST_TYPE_POINTER(PHANDLETABLE, 4, 4);
}

static void test_pack_PIXELFORMATDESCRIPTOR(void)
{
    /* PIXELFORMATDESCRIPTOR (pack 4) */
    TEST_TYPE(PIXELFORMATDESCRIPTOR, 40, 4);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, nSize, 0, 2, 2);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, nVersion, 2, 2, 2);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, dwFlags, 4, 4, 4);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, iPixelType, 8, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cColorBits, 9, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cRedBits, 10, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cRedShift, 11, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cGreenBits, 12, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cGreenShift, 13, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cBlueBits, 14, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cBlueShift, 15, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cAlphaBits, 16, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cAlphaShift, 17, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cAccumBits, 18, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cAccumRedBits, 19, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cAccumGreenBits, 20, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cAccumBlueBits, 21, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cAccumAlphaBits, 22, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cDepthBits, 23, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cStencilBits, 24, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, cAuxBuffers, 25, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, iLayerType, 26, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, bReserved, 27, 1, 1);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, dwLayerMask, 28, 4, 4);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, dwVisibleMask, 32, 4, 4);
    TEST_FIELD(PIXELFORMATDESCRIPTOR, dwDamageMask, 36, 4, 4);
}

static void test_pack_PLAYERPLANEDESCRIPTOR(void)
{
    /* PLAYERPLANEDESCRIPTOR */
    TEST_TYPE(PLAYERPLANEDESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(PLAYERPLANEDESCRIPTOR, 32, 4);
}

static void test_pack_PLOCALESIGNATURE(void)
{
    /* PLOCALESIGNATURE */
    TEST_TYPE(PLOCALESIGNATURE, 4, 4);
    TEST_TYPE_POINTER(PLOCALESIGNATURE, 32, 4);
}

static void test_pack_PLOGBRUSH(void)
{
    /* PLOGBRUSH */
    TEST_TYPE(PLOGBRUSH, 4, 4);
    TEST_TYPE_POINTER(PLOGBRUSH, 12, 4);
}

static void test_pack_PLOGFONTA(void)
{
    /* PLOGFONTA */
    TEST_TYPE(PLOGFONTA, 4, 4);
    TEST_TYPE_POINTER(PLOGFONTA, 60, 4);
}

static void test_pack_PLOGFONTW(void)
{
    /* PLOGFONTW */
    TEST_TYPE(PLOGFONTW, 4, 4);
    TEST_TYPE_POINTER(PLOGFONTW, 92, 4);
}

static void test_pack_PMETAHEADER(void)
{
    /* PMETAHEADER */
    TEST_TYPE(PMETAHEADER, 4, 4);
    TEST_TYPE_POINTER(PMETAHEADER, 18, 2);
}

static void test_pack_PMETARECORD(void)
{
    /* PMETARECORD */
    TEST_TYPE(PMETARECORD, 4, 4);
    TEST_TYPE_POINTER(PMETARECORD, 8, 4);
}

static void test_pack_PNEWTEXTMETRICA(void)
{
    /* PNEWTEXTMETRICA */
    TEST_TYPE(PNEWTEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(PNEWTEXTMETRICA, 72, 4);
}

static void test_pack_PNEWTEXTMETRICW(void)
{
    /* PNEWTEXTMETRICW */
    TEST_TYPE(PNEWTEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(PNEWTEXTMETRICW, 76, 4);
}

static void test_pack_POINTFLOAT(void)
{
    /* POINTFLOAT (pack 4) */
    TEST_TYPE(POINTFLOAT, 8, 4);
    TEST_FIELD(POINTFLOAT, x, 0, 4, 4);
    TEST_FIELD(POINTFLOAT, y, 4, 4, 4);
}

static void test_pack_POINTFX(void)
{
    /* POINTFX (pack 4) */
    TEST_TYPE(POINTFX, 8, 2);
    TEST_FIELD(POINTFX, x, 0, 4, 2);
    TEST_FIELD(POINTFX, y, 4, 4, 2);
}

static void test_pack_POLYTEXTA(void)
{
    /* POLYTEXTA (pack 4) */
    TEST_TYPE(POLYTEXTA, 40, 4);
    TEST_FIELD(POLYTEXTA, x, 0, 4, 4);
    TEST_FIELD(POLYTEXTA, y, 4, 4, 4);
    TEST_FIELD(POLYTEXTA, n, 8, 4, 4);
    TEST_FIELD(POLYTEXTA, lpstr, 12, 4, 4);
    TEST_FIELD(POLYTEXTA, uiFlags, 16, 4, 4);
    TEST_FIELD(POLYTEXTA, rcl, 20, 16, 4);
    TEST_FIELD(POLYTEXTA, pdx, 36, 4, 4);
}

static void test_pack_POLYTEXTW(void)
{
    /* POLYTEXTW (pack 4) */
    TEST_TYPE(POLYTEXTW, 40, 4);
    TEST_FIELD(POLYTEXTW, x, 0, 4, 4);
    TEST_FIELD(POLYTEXTW, y, 4, 4, 4);
    TEST_FIELD(POLYTEXTW, n, 8, 4, 4);
    TEST_FIELD(POLYTEXTW, lpstr, 12, 4, 4);
    TEST_FIELD(POLYTEXTW, uiFlags, 16, 4, 4);
    TEST_FIELD(POLYTEXTW, rcl, 20, 16, 4);
    TEST_FIELD(POLYTEXTW, pdx, 36, 4, 4);
}

static void test_pack_POUTLINETEXTMETRICA(void)
{
    /* POUTLINETEXTMETRICA */
    TEST_TYPE(POUTLINETEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(POUTLINETEXTMETRICA, 212, 4);
}

static void test_pack_POUTLINETEXTMETRICW(void)
{
    /* POUTLINETEXTMETRICW */
    TEST_TYPE(POUTLINETEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(POUTLINETEXTMETRICW, 216, 4);
}

static void test_pack_PPELARRAY(void)
{
    /* PPELARRAY */
    TEST_TYPE(PPELARRAY, 4, 4);
    TEST_TYPE_POINTER(PPELARRAY, 20, 4);
}

static void test_pack_PPIXELFORMATDESCRIPTOR(void)
{
    /* PPIXELFORMATDESCRIPTOR */
    TEST_TYPE(PPIXELFORMATDESCRIPTOR, 4, 4);
    TEST_TYPE_POINTER(PPIXELFORMATDESCRIPTOR, 40, 4);
}

static void test_pack_PPOINTFLOAT(void)
{
    /* PPOINTFLOAT */
    TEST_TYPE(PPOINTFLOAT, 4, 4);
    TEST_TYPE_POINTER(PPOINTFLOAT, 8, 4);
}

static void test_pack_PPOLYTEXTA(void)
{
    /* PPOLYTEXTA */
    TEST_TYPE(PPOLYTEXTA, 4, 4);
    TEST_TYPE_POINTER(PPOLYTEXTA, 40, 4);
}

static void test_pack_PPOLYTEXTW(void)
{
    /* PPOLYTEXTW */
    TEST_TYPE(PPOLYTEXTW, 4, 4);
    TEST_TYPE_POINTER(PPOLYTEXTW, 40, 4);
}

static void test_pack_PRGNDATA(void)
{
    /* PRGNDATA */
    TEST_TYPE(PRGNDATA, 4, 4);
    TEST_TYPE_POINTER(PRGNDATA, 36, 4);
}

static void test_pack_PRGNDATAHEADER(void)
{
    /* PRGNDATAHEADER */
    TEST_TYPE(PRGNDATAHEADER, 4, 4);
    TEST_TYPE_POINTER(PRGNDATAHEADER, 32, 4);
}

static void test_pack_PTEXTMETRICA(void)
{
    /* PTEXTMETRICA */
    TEST_TYPE(PTEXTMETRICA, 4, 4);
    TEST_TYPE_POINTER(PTEXTMETRICA, 56, 4);
}

static void test_pack_PTEXTMETRICW(void)
{
    /* PTEXTMETRICW */
    TEST_TYPE(PTEXTMETRICW, 4, 4);
    TEST_TYPE_POINTER(PTEXTMETRICW, 60, 4);
}

static void test_pack_PTRIVERTEX(void)
{
    /* PTRIVERTEX */
    TEST_TYPE(PTRIVERTEX, 4, 4);
    TEST_TYPE_POINTER(PTRIVERTEX, 16, 4);
}

static void test_pack_PXFORM(void)
{
    /* PXFORM */
    TEST_TYPE(PXFORM, 4, 4);
    TEST_TYPE_POINTER(PXFORM, 24, 4);
}

static void test_pack_RASTERIZER_STATUS(void)
{
    /* RASTERIZER_STATUS (pack 4) */
    TEST_TYPE(RASTERIZER_STATUS, 6, 2);
    TEST_FIELD(RASTERIZER_STATUS, nSize, 0, 2, 2);
    TEST_FIELD(RASTERIZER_STATUS, wFlags, 2, 2, 2);
    TEST_FIELD(RASTERIZER_STATUS, nLanguageID, 4, 2, 2);
}

static void test_pack_RGBQUAD(void)
{
    /* RGBQUAD (pack 4) */
    TEST_TYPE(RGBQUAD, 4, 1);
    TEST_FIELD(RGBQUAD, rgbBlue, 0, 1, 1);
    TEST_FIELD(RGBQUAD, rgbGreen, 1, 1, 1);
    TEST_FIELD(RGBQUAD, rgbRed, 2, 1, 1);
    TEST_FIELD(RGBQUAD, rgbReserved, 3, 1, 1);
}

static void test_pack_RGBTRIPLE(void)
{
    /* RGBTRIPLE (pack 4) */
    TEST_TYPE(RGBTRIPLE, 3, 1);
    TEST_FIELD(RGBTRIPLE, rgbtBlue, 0, 1, 1);
    TEST_FIELD(RGBTRIPLE, rgbtGreen, 1, 1, 1);
    TEST_FIELD(RGBTRIPLE, rgbtRed, 2, 1, 1);
}

static void test_pack_RGNDATA(void)
{
    /* RGNDATA (pack 4) */
    TEST_TYPE(RGNDATA, 36, 4);
    TEST_FIELD(RGNDATA, rdh, 0, 32, 4);
    TEST_FIELD(RGNDATA, Buffer, 32, 1, 1);
}

static void test_pack_RGNDATAHEADER(void)
{
    /* RGNDATAHEADER (pack 4) */
    TEST_TYPE(RGNDATAHEADER, 32, 4);
    TEST_FIELD(RGNDATAHEADER, dwSize, 0, 4, 4);
    TEST_FIELD(RGNDATAHEADER, iType, 4, 4, 4);
    TEST_FIELD(RGNDATAHEADER, nCount, 8, 4, 4);
    TEST_FIELD(RGNDATAHEADER, nRgnSize, 12, 4, 4);
    TEST_FIELD(RGNDATAHEADER, rcBound, 16, 16, 4);
}

static void test_pack_TEXTMETRICA(void)
{
    /* TEXTMETRICA (pack 4) */
    TEST_TYPE(TEXTMETRICA, 56, 4);
    TEST_FIELD(TEXTMETRICA, tmHeight, 0, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmAscent, 4, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmDescent, 8, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmInternalLeading, 12, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmExternalLeading, 16, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmAveCharWidth, 20, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmMaxCharWidth, 24, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmWeight, 28, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmOverhang, 32, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmDigitizedAspectX, 36, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmDigitizedAspectY, 40, 4, 4);
    TEST_FIELD(TEXTMETRICA, tmFirstChar, 44, 1, 1);
    TEST_FIELD(TEXTMETRICA, tmLastChar, 45, 1, 1);
    TEST_FIELD(TEXTMETRICA, tmDefaultChar, 46, 1, 1);
    TEST_FIELD(TEXTMETRICA, tmBreakChar, 47, 1, 1);
    TEST_FIELD(TEXTMETRICA, tmItalic, 48, 1, 1);
    TEST_FIELD(TEXTMETRICA, tmUnderlined, 49, 1, 1);
    TEST_FIELD(TEXTMETRICA, tmStruckOut, 50, 1, 1);
    TEST_FIELD(TEXTMETRICA, tmPitchAndFamily, 51, 1, 1);
    TEST_FIELD(TEXTMETRICA, tmCharSet, 52, 1, 1);
}

static void test_pack_TEXTMETRICW(void)
{
    /* TEXTMETRICW (pack 4) */
    TEST_TYPE(TEXTMETRICW, 60, 4);
    TEST_FIELD(TEXTMETRICW, tmHeight, 0, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmAscent, 4, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmDescent, 8, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmInternalLeading, 12, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmExternalLeading, 16, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmAveCharWidth, 20, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmMaxCharWidth, 24, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmWeight, 28, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmOverhang, 32, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmDigitizedAspectX, 36, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmDigitizedAspectY, 40, 4, 4);
    TEST_FIELD(TEXTMETRICW, tmFirstChar, 44, 2, 2);
    TEST_FIELD(TEXTMETRICW, tmLastChar, 46, 2, 2);
    TEST_FIELD(TEXTMETRICW, tmDefaultChar, 48, 2, 2);
    TEST_FIELD(TEXTMETRICW, tmBreakChar, 50, 2, 2);
    TEST_FIELD(TEXTMETRICW, tmItalic, 52, 1, 1);
    TEST_FIELD(TEXTMETRICW, tmUnderlined, 53, 1, 1);
    TEST_FIELD(TEXTMETRICW, tmStruckOut, 54, 1, 1);
    TEST_FIELD(TEXTMETRICW, tmPitchAndFamily, 55, 1, 1);
    TEST_FIELD(TEXTMETRICW, tmCharSet, 56, 1, 1);
}

static void test_pack_TRIVERTEX(void)
{
    /* TRIVERTEX (pack 4) */
    TEST_TYPE(TRIVERTEX, 16, 4);
    TEST_FIELD(TRIVERTEX, x, 0, 4, 4);
    TEST_FIELD(TRIVERTEX, y, 4, 4, 4);
    TEST_FIELD(TRIVERTEX, Red, 8, 2, 2);
    TEST_FIELD(TRIVERTEX, Green, 10, 2, 2);
    TEST_FIELD(TRIVERTEX, Blue, 12, 2, 2);
    TEST_FIELD(TRIVERTEX, Alpha, 14, 2, 2);
}

static void test_pack_TTPOLYCURVE(void)
{
    /* TTPOLYCURVE (pack 4) */
    TEST_TYPE(TTPOLYCURVE, 12, 2);
    TEST_FIELD(TTPOLYCURVE, wType, 0, 2, 2);
    TEST_FIELD(TTPOLYCURVE, cpfx, 2, 2, 2);
    TEST_FIELD(TTPOLYCURVE, apfx, 4, 8, 2);
}

static void test_pack_TTPOLYGONHEADER(void)
{
    /* TTPOLYGONHEADER (pack 4) */
    TEST_TYPE(TTPOLYGONHEADER, 16, 4);
    TEST_FIELD(TTPOLYGONHEADER, cb, 0, 4, 4);
    TEST_FIELD(TTPOLYGONHEADER, dwType, 4, 4, 4);
    TEST_FIELD(TTPOLYGONHEADER, pfxStart, 8, 8, 2);
}

static void test_pack_XFORM(void)
{
    /* XFORM (pack 4) */
    TEST_TYPE(XFORM, 24, 4);
    TEST_FIELD(XFORM, eM11, 0, 4, 4);
    TEST_FIELD(XFORM, eM12, 4, 4, 4);
    TEST_FIELD(XFORM, eM21, 8, 4, 4);
    TEST_FIELD(XFORM, eM22, 12, 4, 4);
    TEST_FIELD(XFORM, eDx, 16, 4, 4);
    TEST_FIELD(XFORM, eDy, 20, 4, 4);
}

static void test_pack(void)
{
    test_pack_ABC();
    test_pack_ABCFLOAT();
    test_pack_ABORTPROC();
    test_pack_BITMAP();
    test_pack_BITMAPCOREHEADER();
    test_pack_BITMAPCOREINFO();
    test_pack_BITMAPFILEHEADER();
    test_pack_BITMAPINFO();
    test_pack_BITMAPINFOHEADER();
    test_pack_BITMAPV4HEADER();
    test_pack_BITMAPV5HEADER();
    test_pack_BLENDFUNCTION();
    test_pack_CHARSETINFO();
    test_pack_CIEXYZ();
    test_pack_CIEXYZTRIPLE();
    test_pack_COLOR16();
    test_pack_COLORADJUSTMENT();
    test_pack_DEVMODEA();
    test_pack_DEVMODEW();
    test_pack_DIBSECTION();
    test_pack_DISPLAY_DEVICEA();
    test_pack_DISPLAY_DEVICEW();
    test_pack_DOCINFOA();
    test_pack_DOCINFOW();
    test_pack_EMR();
    test_pack_EMRABORTPATH();
    test_pack_EMRANGLEARC();
    test_pack_EMRARC();
    test_pack_EMRARCTO();
    test_pack_EMRBEGINPATH();
    test_pack_EMRBITBLT();
    test_pack_EMRCHORD();
    test_pack_EMRCLOSEFIGURE();
    test_pack_EMRCREATEBRUSHINDIRECT();
    test_pack_EMRCREATECOLORSPACE();
    test_pack_EMRCREATECOLORSPACEW();
    test_pack_EMRCREATEDIBPATTERNBRUSHPT();
    test_pack_EMRCREATEMONOBRUSH();
    test_pack_EMRCREATEPEN();
    test_pack_EMRDELETECOLORSPACE();
    test_pack_EMRDELETEOBJECT();
    test_pack_EMRELLIPSE();
    test_pack_EMRENDPATH();
    test_pack_EMREOF();
    test_pack_EMREXCLUDECLIPRECT();
    test_pack_EMREXTCREATEFONTINDIRECTW();
    test_pack_EMREXTCREATEPEN();
    test_pack_EMREXTFLOODFILL();
    test_pack_EMREXTSELECTCLIPRGN();
    test_pack_EMREXTTEXTOUTA();
    test_pack_EMREXTTEXTOUTW();
    test_pack_EMRFILLPATH();
    test_pack_EMRFILLRGN();
    test_pack_EMRFLATTENPATH();
    test_pack_EMRFORMAT();
    test_pack_EMRFRAMERGN();
    test_pack_EMRGDICOMMENT();
    test_pack_EMRGLSBOUNDEDRECORD();
    test_pack_EMRGLSRECORD();
    test_pack_EMRINTERSECTCLIPRECT();
    test_pack_EMRINVERTRGN();
    test_pack_EMRLINETO();
    test_pack_EMRMASKBLT();
    test_pack_EMRMODIFYWORLDTRANSFORM();
    test_pack_EMRMOVETOEX();
    test_pack_EMROFFSETCLIPRGN();
    test_pack_EMRPAINTRGN();
    test_pack_EMRPIE();
    test_pack_EMRPIXELFORMAT();
    test_pack_EMRPLGBLT();
    test_pack_EMRPOLYBEZIER();
    test_pack_EMRPOLYBEZIER16();
    test_pack_EMRPOLYBEZIERTO();
    test_pack_EMRPOLYBEZIERTO16();
    test_pack_EMRPOLYDRAW();
    test_pack_EMRPOLYDRAW16();
    test_pack_EMRPOLYGON();
    test_pack_EMRPOLYGON16();
    test_pack_EMRPOLYLINE();
    test_pack_EMRPOLYLINE16();
    test_pack_EMRPOLYLINETO();
    test_pack_EMRPOLYLINETO16();
    test_pack_EMRPOLYPOLYGON();
    test_pack_EMRPOLYPOLYGON16();
    test_pack_EMRPOLYPOLYLINE();
    test_pack_EMRPOLYPOLYLINE16();
    test_pack_EMRPOLYTEXTOUTA();
    test_pack_EMRPOLYTEXTOUTW();
    test_pack_EMRREALIZEPALETTE();
    test_pack_EMRRECTANGLE();
    test_pack_EMRRESIZEPALETTE();
    test_pack_EMRRESTOREDC();
    test_pack_EMRROUNDRECT();
    test_pack_EMRSAVEDC();
    test_pack_EMRSCALEVIEWPORTEXTEX();
    test_pack_EMRSCALEWINDOWEXTEX();
    test_pack_EMRSELECTCLIPPATH();
    test_pack_EMRSELECTCOLORSPACE();
    test_pack_EMRSELECTOBJECT();
    test_pack_EMRSELECTPALETTE();
    test_pack_EMRSETARCDIRECTION();
    test_pack_EMRSETBKCOLOR();
    test_pack_EMRSETBKMODE();
    test_pack_EMRSETBRUSHORGEX();
    test_pack_EMRSETCOLORADJUSTMENT();
    test_pack_EMRSETCOLORSPACE();
    test_pack_EMRSETDIBITSTODEVICE();
    test_pack_EMRSETICMMODE();
    test_pack_EMRSETLAYOUT();
    test_pack_EMRSETMAPMODE();
    test_pack_EMRSETMAPPERFLAGS();
    test_pack_EMRSETMETARGN();
    test_pack_EMRSETMITERLIMIT();
    test_pack_EMRSETPIXELV();
    test_pack_EMRSETPOLYFILLMODE();
    test_pack_EMRSETROP2();
    test_pack_EMRSETSTRETCHBLTMODE();
    test_pack_EMRSETTEXTALIGN();
    test_pack_EMRSETTEXTCOLOR();
    test_pack_EMRSETVIEWPORTEXTEX();
    test_pack_EMRSETVIEWPORTORGEX();
    test_pack_EMRSETWINDOWEXTEX();
    test_pack_EMRSETWINDOWORGEX();
    test_pack_EMRSETWORLDTRANSFORM();
    test_pack_EMRSTRETCHBLT();
    test_pack_EMRSTRETCHDIBITS();
    test_pack_EMRSTROKEANDFILLPATH();
    test_pack_EMRSTROKEPATH();
    test_pack_EMRTEXT();
    test_pack_EMRWIDENPATH();
    test_pack_ENHMETAHEADER();
    test_pack_ENHMETARECORD();
    test_pack_ENHMFENUMPROC();
    test_pack_ENUMLOGFONTA();
    test_pack_ENUMLOGFONTEXA();
    test_pack_ENUMLOGFONTEXW();
    test_pack_ENUMLOGFONTW();
    test_pack_EXTLOGFONTA();
    test_pack_EXTLOGFONTW();
    test_pack_EXTLOGPEN();
    test_pack_FIXED();
    test_pack_FONTENUMPROCA();
    test_pack_FONTENUMPROCW();
    test_pack_FONTSIGNATURE();
    test_pack_FXPT16DOT16();
    test_pack_FXPT2DOT30();
    test_pack_GCP_RESULTSA();
    test_pack_GCP_RESULTSW();
    test_pack_GLYPHMETRICS();
    test_pack_GLYPHMETRICSFLOAT();
    test_pack_GOBJENUMPROC();
    test_pack_GRADIENT_RECT();
    test_pack_GRADIENT_TRIANGLE();
    test_pack_HANDLETABLE();
    test_pack_ICMENUMPROCA();
    test_pack_ICMENUMPROCW();
    test_pack_KERNINGPAIR();
    test_pack_LAYERPLANEDESCRIPTOR();
    test_pack_LCSCSTYPE();
    test_pack_LCSGAMUTMATCH();
    test_pack_LINEDDAPROC();
    test_pack_LOCALESIGNATURE();
    test_pack_LOGBRUSH();
    test_pack_LOGCOLORSPACEA();
    test_pack_LOGCOLORSPACEW();
    test_pack_LOGFONTA();
    test_pack_LOGFONTW();
    test_pack_LOGPEN();
    test_pack_LPABC();
    test_pack_LPABCFLOAT();
    test_pack_LPBITMAP();
    test_pack_LPBITMAPCOREHEADER();
    test_pack_LPBITMAPCOREINFO();
    test_pack_LPBITMAPFILEHEADER();
    test_pack_LPBITMAPINFO();
    test_pack_LPBITMAPINFOHEADER();
    test_pack_LPBITMAPV5HEADER();
    test_pack_LPCHARSETINFO();
    test_pack_LPCIEXYZ();
    test_pack_LPCIEXYZTRIPLE();
    test_pack_LPCOLORADJUSTMENT();
    test_pack_LPDEVMODEA();
    test_pack_LPDEVMODEW();
    test_pack_LPDIBSECTION();
    test_pack_LPDISPLAY_DEVICEA();
    test_pack_LPDISPLAY_DEVICEW();
    test_pack_LPDOCINFOA();
    test_pack_LPDOCINFOW();
    test_pack_LPENHMETAHEADER();
    test_pack_LPENHMETARECORD();
    test_pack_LPENUMLOGFONTA();
    test_pack_LPENUMLOGFONTEXA();
    test_pack_LPENUMLOGFONTEXW();
    test_pack_LPENUMLOGFONTW();
    test_pack_LPEXTLOGFONTA();
    test_pack_LPEXTLOGFONTW();
    test_pack_LPEXTLOGPEN();
    test_pack_LPFONTSIGNATURE();
    test_pack_LPGCP_RESULTSA();
    test_pack_LPGCP_RESULTSW();
    test_pack_LPGLYPHMETRICS();
    test_pack_LPGLYPHMETRICSFLOAT();
    test_pack_LPGRADIENT_RECT();
    test_pack_LPGRADIENT_TRIANGLE();
    test_pack_LPHANDLETABLE();
    test_pack_LPKERNINGPAIR();
    test_pack_LPLAYERPLANEDESCRIPTOR();
    test_pack_LPLOCALESIGNATURE();
    test_pack_LPLOGBRUSH();
    test_pack_LPLOGCOLORSPACEA();
    test_pack_LPLOGCOLORSPACEW();
    test_pack_LPLOGFONTA();
    test_pack_LPLOGFONTW();
    test_pack_LPLOGPEN();
    test_pack_LPMAT2();
    test_pack_LPMETAFILEPICT();
    test_pack_LPMETAHEADER();
    test_pack_LPMETARECORD();
    test_pack_LPNEWTEXTMETRICA();
    test_pack_LPNEWTEXTMETRICW();
    test_pack_LPOUTLINETEXTMETRICA();
    test_pack_LPOUTLINETEXTMETRICW();
    test_pack_LPPANOSE();
    test_pack_LPPELARRAY();
    test_pack_LPPIXELFORMATDESCRIPTOR();
    test_pack_LPPOINTFX();
    test_pack_LPPOLYTEXTA();
    test_pack_LPPOLYTEXTW();
    test_pack_LPRASTERIZER_STATUS();
    test_pack_LPRGBQUAD();
    test_pack_LPRGNDATA();
    test_pack_LPTEXTMETRICA();
    test_pack_LPTEXTMETRICW();
    test_pack_LPTRIVERTEX();
    test_pack_LPTTPOLYCURVE();
    test_pack_LPTTPOLYGONHEADER();
    test_pack_LPXFORM();
    test_pack_MAT2();
    test_pack_METAFILEPICT();
    test_pack_METAHEADER();
    test_pack_METARECORD();
    test_pack_MFENUMPROC();
    test_pack_NEWTEXTMETRICA();
    test_pack_NEWTEXTMETRICEXA();
    test_pack_NEWTEXTMETRICEXW();
    test_pack_NEWTEXTMETRICW();
    test_pack_NPEXTLOGPEN();
    test_pack_OLDFONTENUMPROC();
    test_pack_OLDFONTENUMPROCA();
    test_pack_OLDFONTENUMPROCW();
    test_pack_OUTLINETEXTMETRICA();
    test_pack_OUTLINETEXTMETRICW();
    test_pack_PABC();
    test_pack_PABCFLOAT();
    test_pack_PANOSE();
    test_pack_PATTERN();
    test_pack_PBITMAP();
    test_pack_PBITMAPCOREHEADER();
    test_pack_PBITMAPCOREINFO();
    test_pack_PBITMAPFILEHEADER();
    test_pack_PBITMAPINFO();
    test_pack_PBITMAPINFOHEADER();
    test_pack_PBITMAPV4HEADER();
    test_pack_PBITMAPV5HEADER();
    test_pack_PBLENDFUNCTION();
    test_pack_PCHARSETINFO();
    test_pack_PCOLORADJUSTMENT();
    test_pack_PDEVMODEA();
    test_pack_PDEVMODEW();
    test_pack_PDIBSECTION();
    test_pack_PDISPLAY_DEVICEA();
    test_pack_PDISPLAY_DEVICEW();
    test_pack_PELARRAY();
    test_pack_PEMR();
    test_pack_PEMRABORTPATH();
    test_pack_PEMRANGLEARC();
    test_pack_PEMRARC();
    test_pack_PEMRARCTO();
    test_pack_PEMRBEGINPATH();
    test_pack_PEMRBITBLT();
    test_pack_PEMRCHORD();
    test_pack_PEMRCLOSEFIGURE();
    test_pack_PEMRCREATEBRUSHINDIRECT();
    test_pack_PEMRCREATECOLORSPACE();
    test_pack_PEMRCREATECOLORSPACEW();
    test_pack_PEMRCREATEDIBPATTERNBRUSHPT();
    test_pack_PEMRCREATEMONOBRUSH();
    test_pack_PEMRCREATEPALETTE();
    test_pack_PEMRCREATEPEN();
    test_pack_PEMRDELETECOLORSPACE();
    test_pack_PEMRDELETEOBJECT();
    test_pack_PEMRELLIPSE();
    test_pack_PEMRENDPATH();
    test_pack_PEMREOF();
    test_pack_PEMREXCLUDECLIPRECT();
    test_pack_PEMREXTCREATEFONTINDIRECTW();
    test_pack_PEMREXTCREATEPEN();
    test_pack_PEMREXTFLOODFILL();
    test_pack_PEMREXTSELECTCLIPRGN();
    test_pack_PEMREXTTEXTOUTA();
    test_pack_PEMREXTTEXTOUTW();
    test_pack_PEMRFILLPATH();
    test_pack_PEMRFILLRGN();
    test_pack_PEMRFLATTENPATH();
    test_pack_PEMRFORMAT();
    test_pack_PEMRFRAMERGN();
    test_pack_PEMRGDICOMMENT();
    test_pack_PEMRGLSBOUNDEDRECORD();
    test_pack_PEMRGLSRECORD();
    test_pack_PEMRINTERSECTCLIPRECT();
    test_pack_PEMRINVERTRGN();
    test_pack_PEMRLINETO();
    test_pack_PEMRMASKBLT();
    test_pack_PEMRMODIFYWORLDTRANSFORM();
    test_pack_PEMRMOVETOEX();
    test_pack_PEMROFFSETCLIPRGN();
    test_pack_PEMRPAINTRGN();
    test_pack_PEMRPIE();
    test_pack_PEMRPIXELFORMAT();
    test_pack_PEMRPLGBLT();
    test_pack_PEMRPOLYBEZIER();
    test_pack_PEMRPOLYBEZIER16();
    test_pack_PEMRPOLYBEZIERTO();
    test_pack_PEMRPOLYBEZIERTO16();
    test_pack_PEMRPOLYDRAW();
    test_pack_PEMRPOLYDRAW16();
    test_pack_PEMRPOLYGON();
    test_pack_PEMRPOLYGON16();
    test_pack_PEMRPOLYLINE();
    test_pack_PEMRPOLYLINE16();
    test_pack_PEMRPOLYLINETO();
    test_pack_PEMRPOLYLINETO16();
    test_pack_PEMRPOLYPOLYGON();
    test_pack_PEMRPOLYPOLYGON16();
    test_pack_PEMRPOLYPOLYLINE();
    test_pack_PEMRPOLYPOLYLINE16();
    test_pack_PEMRPOLYTEXTOUTA();
    test_pack_PEMRPOLYTEXTOUTW();
    test_pack_PEMRREALIZEPALETTE();
    test_pack_PEMRRECTANGLE();
    test_pack_PEMRRESIZEPALETTE();
    test_pack_PEMRRESTOREDC();
    test_pack_PEMRROUNDRECT();
    test_pack_PEMRSAVEDC();
    test_pack_PEMRSCALEVIEWPORTEXTEX();
    test_pack_PEMRSCALEWINDOWEXTEX();
    test_pack_PEMRSELECTCLIPPATH();
    test_pack_PEMRSELECTCOLORSPACE();
    test_pack_PEMRSELECTOBJECT();
    test_pack_PEMRSELECTPALETTE();
    test_pack_PEMRSETARCDIRECTION();
    test_pack_PEMRSETBKCOLOR();
    test_pack_PEMRSETBKMODE();
    test_pack_PEMRSETBRUSHORGEX();
    test_pack_PEMRSETCOLORADJUSTMENT();
    test_pack_PEMRSETCOLORSPACE();
    test_pack_PEMRSETDIBITSTODEVICE();
    test_pack_PEMRSETICMMODE();
    test_pack_PEMRSETLAYOUT();
    test_pack_PEMRSETMAPMODE();
    test_pack_PEMRSETMAPPERFLAGS();
    test_pack_PEMRSETMETARGN();
    test_pack_PEMRSETMITERLIMIT();
    test_pack_PEMRSETPALETTEENTRIES();
    test_pack_PEMRSETPIXELV();
    test_pack_PEMRSETPOLYFILLMODE();
    test_pack_PEMRSETROP2();
    test_pack_PEMRSETSTRETCHBLTMODE();
    test_pack_PEMRSETTEXTALIGN();
    test_pack_PEMRSETTEXTCOLOR();
    test_pack_PEMRSETVIEWPORTEXTEX();
    test_pack_PEMRSETVIEWPORTORGEX();
    test_pack_PEMRSETWINDOWEXTEX();
    test_pack_PEMRSETWINDOWORGEX();
    test_pack_PEMRSETWORLDTRANSFORM();
    test_pack_PEMRSTRETCHBLT();
    test_pack_PEMRSTRETCHDIBITS();
    test_pack_PEMRSTROKEANDFILLPATH();
    test_pack_PEMRSTROKEPATH();
    test_pack_PEMRTEXT();
    test_pack_PEMRWIDENPATH();
    test_pack_PENHMETAHEADER();
    test_pack_PEXTLOGFONTA();
    test_pack_PEXTLOGFONTW();
    test_pack_PEXTLOGPEN();
    test_pack_PFONTSIGNATURE();
    test_pack_PGLYPHMETRICSFLOAT();
    test_pack_PGRADIENT_RECT();
    test_pack_PGRADIENT_TRIANGLE();
    test_pack_PHANDLETABLE();
    test_pack_PIXELFORMATDESCRIPTOR();
    test_pack_PLAYERPLANEDESCRIPTOR();
    test_pack_PLOCALESIGNATURE();
    test_pack_PLOGBRUSH();
    test_pack_PLOGFONTA();
    test_pack_PLOGFONTW();
    test_pack_PMETAHEADER();
    test_pack_PMETARECORD();
    test_pack_PNEWTEXTMETRICA();
    test_pack_PNEWTEXTMETRICW();
    test_pack_POINTFLOAT();
    test_pack_POINTFX();
    test_pack_POLYTEXTA();
    test_pack_POLYTEXTW();
    test_pack_POUTLINETEXTMETRICA();
    test_pack_POUTLINETEXTMETRICW();
    test_pack_PPELARRAY();
    test_pack_PPIXELFORMATDESCRIPTOR();
    test_pack_PPOINTFLOAT();
    test_pack_PPOLYTEXTA();
    test_pack_PPOLYTEXTW();
    test_pack_PRGNDATA();
    test_pack_PRGNDATAHEADER();
    test_pack_PTEXTMETRICA();
    test_pack_PTEXTMETRICW();
    test_pack_PTRIVERTEX();
    test_pack_PXFORM();
    test_pack_RASTERIZER_STATUS();
    test_pack_RGBQUAD();
    test_pack_RGBTRIPLE();
    test_pack_RGNDATA();
    test_pack_RGNDATAHEADER();
    test_pack_TEXTMETRICA();
    test_pack_TEXTMETRICW();
    test_pack_TRIVERTEX();
    test_pack_TTPOLYCURVE();
    test_pack_TTPOLYGONHEADER();
    test_pack_XFORM();
}

START_TEST(generated)
{
    test_pack();
}
