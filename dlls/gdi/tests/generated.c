/* File generated automatically from tools/winapi/test.dat; do not edit! */
/* This file can be copied, modified and distributed without restriction. */

/*
 * Unit tests for data structure packing
 */

#include <stdio.h>

#include "wine/test.h"
#include "wingdi.h"

void test_pack(void)
{
    /* ABC */
    ok(FIELD_OFFSET(ABC, abcA) == 0,
       "FIELD_OFFSET(ABC, abcA) == %ld (expected 0)",
       FIELD_OFFSET(ABC, abcA)); /* INT */
    ok(FIELD_OFFSET(ABC, abcB) == 4,
       "FIELD_OFFSET(ABC, abcB) == %ld (expected 4)",
       FIELD_OFFSET(ABC, abcB)); /* UINT */
    ok(FIELD_OFFSET(ABC, abcC) == 8,
       "FIELD_OFFSET(ABC, abcC) == %ld (expected 8)",
       FIELD_OFFSET(ABC, abcC)); /* INT */
    ok(sizeof(ABC) == 12, "sizeof(ABC) == %d (expected 12)", sizeof(ABC));

    /* ABCFLOAT */
    ok(FIELD_OFFSET(ABCFLOAT, abcfA) == 0,
       "FIELD_OFFSET(ABCFLOAT, abcfA) == %ld (expected 0)",
       FIELD_OFFSET(ABCFLOAT, abcfA)); /* FLOAT */
    ok(FIELD_OFFSET(ABCFLOAT, abcfB) == 4,
       "FIELD_OFFSET(ABCFLOAT, abcfB) == %ld (expected 4)",
       FIELD_OFFSET(ABCFLOAT, abcfB)); /* FLOAT */
    ok(FIELD_OFFSET(ABCFLOAT, abcfC) == 8,
       "FIELD_OFFSET(ABCFLOAT, abcfC) == %ld (expected 8)",
       FIELD_OFFSET(ABCFLOAT, abcfC)); /* FLOAT */
    ok(sizeof(ABCFLOAT) == 12, "sizeof(ABCFLOAT) == %d (expected 12)", sizeof(ABCFLOAT));

    /* BITMAP */
    ok(FIELD_OFFSET(BITMAP, bmType) == 0,
       "FIELD_OFFSET(BITMAP, bmType) == %ld (expected 0)",
       FIELD_OFFSET(BITMAP, bmType)); /* INT */
    ok(FIELD_OFFSET(BITMAP, bmWidth) == 4,
       "FIELD_OFFSET(BITMAP, bmWidth) == %ld (expected 4)",
       FIELD_OFFSET(BITMAP, bmWidth)); /* INT */
    ok(FIELD_OFFSET(BITMAP, bmHeight) == 8,
       "FIELD_OFFSET(BITMAP, bmHeight) == %ld (expected 8)",
       FIELD_OFFSET(BITMAP, bmHeight)); /* INT */
    ok(FIELD_OFFSET(BITMAP, bmWidthBytes) == 12,
       "FIELD_OFFSET(BITMAP, bmWidthBytes) == %ld (expected 12)",
       FIELD_OFFSET(BITMAP, bmWidthBytes)); /* INT */
    ok(FIELD_OFFSET(BITMAP, bmPlanes) == 16,
       "FIELD_OFFSET(BITMAP, bmPlanes) == %ld (expected 16)",
       FIELD_OFFSET(BITMAP, bmPlanes)); /* WORD */
    ok(FIELD_OFFSET(BITMAP, bmBitsPixel) == 18,
       "FIELD_OFFSET(BITMAP, bmBitsPixel) == %ld (expected 18)",
       FIELD_OFFSET(BITMAP, bmBitsPixel)); /* WORD */
    ok(FIELD_OFFSET(BITMAP, bmBits) == 20,
       "FIELD_OFFSET(BITMAP, bmBits) == %ld (expected 20)",
       FIELD_OFFSET(BITMAP, bmBits)); /* LPVOID */
    ok(sizeof(BITMAP) == 24, "sizeof(BITMAP) == %d (expected 24)", sizeof(BITMAP));

    /* BITMAPCOREHEADER */
    ok(FIELD_OFFSET(BITMAPCOREHEADER, bcSize) == 0,
       "FIELD_OFFSET(BITMAPCOREHEADER, bcSize) == %ld (expected 0)",
       FIELD_OFFSET(BITMAPCOREHEADER, bcSize)); /* DWORD */
    ok(FIELD_OFFSET(BITMAPCOREHEADER, bcWidth) == 4,
       "FIELD_OFFSET(BITMAPCOREHEADER, bcWidth) == %ld (expected 4)",
       FIELD_OFFSET(BITMAPCOREHEADER, bcWidth)); /* WORD */
    ok(FIELD_OFFSET(BITMAPCOREHEADER, bcHeight) == 6,
       "FIELD_OFFSET(BITMAPCOREHEADER, bcHeight) == %ld (expected 6)",
       FIELD_OFFSET(BITMAPCOREHEADER, bcHeight)); /* WORD */
    ok(FIELD_OFFSET(BITMAPCOREHEADER, bcPlanes) == 8,
       "FIELD_OFFSET(BITMAPCOREHEADER, bcPlanes) == %ld (expected 8)",
       FIELD_OFFSET(BITMAPCOREHEADER, bcPlanes)); /* WORD */
    ok(FIELD_OFFSET(BITMAPCOREHEADER, bcBitCount) == 10,
       "FIELD_OFFSET(BITMAPCOREHEADER, bcBitCount) == %ld (expected 10)",
       FIELD_OFFSET(BITMAPCOREHEADER, bcBitCount)); /* WORD */
    ok(sizeof(BITMAPCOREHEADER) == 12, "sizeof(BITMAPCOREHEADER) == %d (expected 12)", sizeof(BITMAPCOREHEADER));

    /* BITMAPFILEHEADER */
    ok(FIELD_OFFSET(BITMAPFILEHEADER, bfType) == 0,
       "FIELD_OFFSET(BITMAPFILEHEADER, bfType) == %ld (expected 0)",
       FIELD_OFFSET(BITMAPFILEHEADER, bfType)); /* WORD */
    ok(FIELD_OFFSET(BITMAPFILEHEADER, bfSize) == 2,
       "FIELD_OFFSET(BITMAPFILEHEADER, bfSize) == %ld (expected 2)",
       FIELD_OFFSET(BITMAPFILEHEADER, bfSize)); /* DWORD */
    ok(FIELD_OFFSET(BITMAPFILEHEADER, bfReserved1) == 6,
       "FIELD_OFFSET(BITMAPFILEHEADER, bfReserved1) == %ld (expected 6)",
       FIELD_OFFSET(BITMAPFILEHEADER, bfReserved1)); /* WORD */
    ok(FIELD_OFFSET(BITMAPFILEHEADER, bfReserved2) == 8,
       "FIELD_OFFSET(BITMAPFILEHEADER, bfReserved2) == %ld (expected 8)",
       FIELD_OFFSET(BITMAPFILEHEADER, bfReserved2)); /* WORD */
    ok(FIELD_OFFSET(BITMAPFILEHEADER, bfOffBits) == 10,
       "FIELD_OFFSET(BITMAPFILEHEADER, bfOffBits) == %ld (expected 10)",
       FIELD_OFFSET(BITMAPFILEHEADER, bfOffBits)); /* DWORD */
    ok(sizeof(BITMAPFILEHEADER) == 14, "sizeof(BITMAPFILEHEADER) == %d (expected 14)", sizeof(BITMAPFILEHEADER));

    /* BITMAPINFOHEADER */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biSize) == 0,
       "FIELD_OFFSET(BITMAPINFOHEADER, biSize) == %ld (expected 0)",
       FIELD_OFFSET(BITMAPINFOHEADER, biSize)); /* DWORD */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biWidth) == 4,
       "FIELD_OFFSET(BITMAPINFOHEADER, biWidth) == %ld (expected 4)",
       FIELD_OFFSET(BITMAPINFOHEADER, biWidth)); /* LONG */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biHeight) == 8,
       "FIELD_OFFSET(BITMAPINFOHEADER, biHeight) == %ld (expected 8)",
       FIELD_OFFSET(BITMAPINFOHEADER, biHeight)); /* LONG */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biPlanes) == 12,
       "FIELD_OFFSET(BITMAPINFOHEADER, biPlanes) == %ld (expected 12)",
       FIELD_OFFSET(BITMAPINFOHEADER, biPlanes)); /* WORD */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biBitCount) == 14,
       "FIELD_OFFSET(BITMAPINFOHEADER, biBitCount) == %ld (expected 14)",
       FIELD_OFFSET(BITMAPINFOHEADER, biBitCount)); /* WORD */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biCompression) == 16,
       "FIELD_OFFSET(BITMAPINFOHEADER, biCompression) == %ld (expected 16)",
       FIELD_OFFSET(BITMAPINFOHEADER, biCompression)); /* DWORD */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biSizeImage) == 20,
       "FIELD_OFFSET(BITMAPINFOHEADER, biSizeImage) == %ld (expected 20)",
       FIELD_OFFSET(BITMAPINFOHEADER, biSizeImage)); /* DWORD */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biXPelsPerMeter) == 24,
       "FIELD_OFFSET(BITMAPINFOHEADER, biXPelsPerMeter) == %ld (expected 24)",
       FIELD_OFFSET(BITMAPINFOHEADER, biXPelsPerMeter)); /* LONG */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biYPelsPerMeter) == 28,
       "FIELD_OFFSET(BITMAPINFOHEADER, biYPelsPerMeter) == %ld (expected 28)",
       FIELD_OFFSET(BITMAPINFOHEADER, biYPelsPerMeter)); /* LONG */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biClrUsed) == 32,
       "FIELD_OFFSET(BITMAPINFOHEADER, biClrUsed) == %ld (expected 32)",
       FIELD_OFFSET(BITMAPINFOHEADER, biClrUsed)); /* DWORD */
    ok(FIELD_OFFSET(BITMAPINFOHEADER, biClrImportant) == 36,
       "FIELD_OFFSET(BITMAPINFOHEADER, biClrImportant) == %ld (expected 36)",
       FIELD_OFFSET(BITMAPINFOHEADER, biClrImportant)); /* DWORD */
    ok(sizeof(BITMAPINFOHEADER) == 40, "sizeof(BITMAPINFOHEADER) == %d (expected 40)", sizeof(BITMAPINFOHEADER));

    /* BLENDFUNCTION */
    ok(FIELD_OFFSET(BLENDFUNCTION, BlendOp) == 0,
       "FIELD_OFFSET(BLENDFUNCTION, BlendOp) == %ld (expected 0)",
       FIELD_OFFSET(BLENDFUNCTION, BlendOp)); /* BYTE */
    ok(FIELD_OFFSET(BLENDFUNCTION, BlendFlags) == 1,
       "FIELD_OFFSET(BLENDFUNCTION, BlendFlags) == %ld (expected 1)",
       FIELD_OFFSET(BLENDFUNCTION, BlendFlags)); /* BYTE */
    ok(FIELD_OFFSET(BLENDFUNCTION, SourceConstantAlpha) == 2,
       "FIELD_OFFSET(BLENDFUNCTION, SourceConstantAlpha) == %ld (expected 2)",
       FIELD_OFFSET(BLENDFUNCTION, SourceConstantAlpha)); /* BYTE */
    ok(FIELD_OFFSET(BLENDFUNCTION, AlphaFormat) == 3,
       "FIELD_OFFSET(BLENDFUNCTION, AlphaFormat) == %ld (expected 3)",
       FIELD_OFFSET(BLENDFUNCTION, AlphaFormat)); /* BYTE */
    ok(sizeof(BLENDFUNCTION) == 4, "sizeof(BLENDFUNCTION) == %d (expected 4)", sizeof(BLENDFUNCTION));

    /* CIEXYZ */
    ok(FIELD_OFFSET(CIEXYZ, ciexyzX) == 0,
       "FIELD_OFFSET(CIEXYZ, ciexyzX) == %ld (expected 0)",
       FIELD_OFFSET(CIEXYZ, ciexyzX)); /* FXPT2DOT30 */
    ok(FIELD_OFFSET(CIEXYZ, ciexyzY) == 4,
       "FIELD_OFFSET(CIEXYZ, ciexyzY) == %ld (expected 4)",
       FIELD_OFFSET(CIEXYZ, ciexyzY)); /* FXPT2DOT30 */
    ok(FIELD_OFFSET(CIEXYZ, ciexyzZ) == 8,
       "FIELD_OFFSET(CIEXYZ, ciexyzZ) == %ld (expected 8)",
       FIELD_OFFSET(CIEXYZ, ciexyzZ)); /* FXPT2DOT30 */
    ok(sizeof(CIEXYZ) == 12, "sizeof(CIEXYZ) == %d (expected 12)", sizeof(CIEXYZ));

    /* DISPLAY_DEVICEA */
    ok(FIELD_OFFSET(DISPLAY_DEVICEA, cb) == 0,
       "FIELD_OFFSET(DISPLAY_DEVICEA, cb) == %ld (expected 0)",
       FIELD_OFFSET(DISPLAY_DEVICEA, cb)); /* DWORD */
    ok(FIELD_OFFSET(DISPLAY_DEVICEA, DeviceName) == 4,
       "FIELD_OFFSET(DISPLAY_DEVICEA, DeviceName) == %ld (expected 4)",
       FIELD_OFFSET(DISPLAY_DEVICEA, DeviceName)); /* CHAR[32] */
    ok(FIELD_OFFSET(DISPLAY_DEVICEA, DeviceString) == 36,
       "FIELD_OFFSET(DISPLAY_DEVICEA, DeviceString) == %ld (expected 36)",
       FIELD_OFFSET(DISPLAY_DEVICEA, DeviceString)); /* CHAR[128] */
    ok(FIELD_OFFSET(DISPLAY_DEVICEA, StateFlags) == 164,
       "FIELD_OFFSET(DISPLAY_DEVICEA, StateFlags) == %ld (expected 164)",
       FIELD_OFFSET(DISPLAY_DEVICEA, StateFlags)); /* DWORD */
    ok(FIELD_OFFSET(DISPLAY_DEVICEA, DeviceID) == 168,
       "FIELD_OFFSET(DISPLAY_DEVICEA, DeviceID) == %ld (expected 168)",
       FIELD_OFFSET(DISPLAY_DEVICEA, DeviceID)); /* CHAR[128] */
    ok(FIELD_OFFSET(DISPLAY_DEVICEA, DeviceKey) == 296,
       "FIELD_OFFSET(DISPLAY_DEVICEA, DeviceKey) == %ld (expected 296)",
       FIELD_OFFSET(DISPLAY_DEVICEA, DeviceKey)); /* CHAR[128] */
    ok(sizeof(DISPLAY_DEVICEA) == 424, "sizeof(DISPLAY_DEVICEA) == %d (expected 424)", sizeof(DISPLAY_DEVICEA));

    /* DISPLAY_DEVICEW */
    ok(FIELD_OFFSET(DISPLAY_DEVICEW, cb) == 0,
       "FIELD_OFFSET(DISPLAY_DEVICEW, cb) == %ld (expected 0)",
       FIELD_OFFSET(DISPLAY_DEVICEW, cb)); /* DWORD */
    ok(FIELD_OFFSET(DISPLAY_DEVICEW, DeviceName) == 4,
       "FIELD_OFFSET(DISPLAY_DEVICEW, DeviceName) == %ld (expected 4)",
       FIELD_OFFSET(DISPLAY_DEVICEW, DeviceName)); /* WCHAR[32] */
    ok(FIELD_OFFSET(DISPLAY_DEVICEW, DeviceString) == 68,
       "FIELD_OFFSET(DISPLAY_DEVICEW, DeviceString) == %ld (expected 68)",
       FIELD_OFFSET(DISPLAY_DEVICEW, DeviceString)); /* WCHAR[128] */
    ok(FIELD_OFFSET(DISPLAY_DEVICEW, StateFlags) == 324,
       "FIELD_OFFSET(DISPLAY_DEVICEW, StateFlags) == %ld (expected 324)",
       FIELD_OFFSET(DISPLAY_DEVICEW, StateFlags)); /* DWORD */
    ok(FIELD_OFFSET(DISPLAY_DEVICEW, DeviceID) == 328,
       "FIELD_OFFSET(DISPLAY_DEVICEW, DeviceID) == %ld (expected 328)",
       FIELD_OFFSET(DISPLAY_DEVICEW, DeviceID)); /* WCHAR[128] */
    ok(FIELD_OFFSET(DISPLAY_DEVICEW, DeviceKey) == 584,
       "FIELD_OFFSET(DISPLAY_DEVICEW, DeviceKey) == %ld (expected 584)",
       FIELD_OFFSET(DISPLAY_DEVICEW, DeviceKey)); /* WCHAR[128] */
    ok(sizeof(DISPLAY_DEVICEW) == 840, "sizeof(DISPLAY_DEVICEW) == %d (expected 840)", sizeof(DISPLAY_DEVICEW));

    /* DOCINFOA */
    ok(FIELD_OFFSET(DOCINFOA, cbSize) == 0,
       "FIELD_OFFSET(DOCINFOA, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(DOCINFOA, cbSize)); /* INT */
    ok(FIELD_OFFSET(DOCINFOA, lpszDocName) == 4,
       "FIELD_OFFSET(DOCINFOA, lpszDocName) == %ld (expected 4)",
       FIELD_OFFSET(DOCINFOA, lpszDocName)); /* LPCSTR */
    ok(FIELD_OFFSET(DOCINFOA, lpszOutput) == 8,
       "FIELD_OFFSET(DOCINFOA, lpszOutput) == %ld (expected 8)",
       FIELD_OFFSET(DOCINFOA, lpszOutput)); /* LPCSTR */
    ok(FIELD_OFFSET(DOCINFOA, lpszDatatype) == 12,
       "FIELD_OFFSET(DOCINFOA, lpszDatatype) == %ld (expected 12)",
       FIELD_OFFSET(DOCINFOA, lpszDatatype)); /* LPCSTR */
    ok(FIELD_OFFSET(DOCINFOA, fwType) == 16,
       "FIELD_OFFSET(DOCINFOA, fwType) == %ld (expected 16)",
       FIELD_OFFSET(DOCINFOA, fwType)); /* DWORD */
    ok(sizeof(DOCINFOA) == 20, "sizeof(DOCINFOA) == %d (expected 20)", sizeof(DOCINFOA));

    /* DOCINFOW */
    ok(FIELD_OFFSET(DOCINFOW, cbSize) == 0,
       "FIELD_OFFSET(DOCINFOW, cbSize) == %ld (expected 0)",
       FIELD_OFFSET(DOCINFOW, cbSize)); /* INT */
    ok(FIELD_OFFSET(DOCINFOW, lpszDocName) == 4,
       "FIELD_OFFSET(DOCINFOW, lpszDocName) == %ld (expected 4)",
       FIELD_OFFSET(DOCINFOW, lpszDocName)); /* LPCWSTR */
    ok(FIELD_OFFSET(DOCINFOW, lpszOutput) == 8,
       "FIELD_OFFSET(DOCINFOW, lpszOutput) == %ld (expected 8)",
       FIELD_OFFSET(DOCINFOW, lpszOutput)); /* LPCWSTR */
    ok(FIELD_OFFSET(DOCINFOW, lpszDatatype) == 12,
       "FIELD_OFFSET(DOCINFOW, lpszDatatype) == %ld (expected 12)",
       FIELD_OFFSET(DOCINFOW, lpszDatatype)); /* LPCWSTR */
    ok(FIELD_OFFSET(DOCINFOW, fwType) == 16,
       "FIELD_OFFSET(DOCINFOW, fwType) == %ld (expected 16)",
       FIELD_OFFSET(DOCINFOW, fwType)); /* DWORD */
    ok(sizeof(DOCINFOW) == 20, "sizeof(DOCINFOW) == %d (expected 20)", sizeof(DOCINFOW));

    /* EMR */
    ok(FIELD_OFFSET(EMR, iType) == 0,
       "FIELD_OFFSET(EMR, iType) == %ld (expected 0)",
       FIELD_OFFSET(EMR, iType)); /* DWORD */
    ok(FIELD_OFFSET(EMR, nSize) == 4,
       "FIELD_OFFSET(EMR, nSize) == %ld (expected 4)",
       FIELD_OFFSET(EMR, nSize)); /* DWORD */
    ok(sizeof(EMR) == 8, "sizeof(EMR) == %d (expected 8)", sizeof(EMR));

    /* EMRABORTPATH */
    ok(FIELD_OFFSET(EMRABORTPATH, emr) == 0,
       "FIELD_OFFSET(EMRABORTPATH, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRABORTPATH, emr)); /* EMR */
    ok(sizeof(EMRABORTPATH) == 8, "sizeof(EMRABORTPATH) == %d (expected 8)", sizeof(EMRABORTPATH));

    /* EMRANGLEARC */
    ok(FIELD_OFFSET(EMRANGLEARC, emr) == 0,
       "FIELD_OFFSET(EMRANGLEARC, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRANGLEARC, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRANGLEARC, ptlCenter) == 8,
       "FIELD_OFFSET(EMRANGLEARC, ptlCenter) == %ld (expected 8)",
       FIELD_OFFSET(EMRANGLEARC, ptlCenter)); /* POINTL */
    ok(FIELD_OFFSET(EMRANGLEARC, nRadius) == 16,
       "FIELD_OFFSET(EMRANGLEARC, nRadius) == %ld (expected 16)",
       FIELD_OFFSET(EMRANGLEARC, nRadius)); /* DWORD */
    ok(FIELD_OFFSET(EMRANGLEARC, eStartAngle) == 20,
       "FIELD_OFFSET(EMRANGLEARC, eStartAngle) == %ld (expected 20)",
       FIELD_OFFSET(EMRANGLEARC, eStartAngle)); /* FLOAT */
    ok(FIELD_OFFSET(EMRANGLEARC, eSweepAngle) == 24,
       "FIELD_OFFSET(EMRANGLEARC, eSweepAngle) == %ld (expected 24)",
       FIELD_OFFSET(EMRANGLEARC, eSweepAngle)); /* FLOAT */
    ok(sizeof(EMRANGLEARC) == 28, "sizeof(EMRANGLEARC) == %d (expected 28)", sizeof(EMRANGLEARC));

    /* EMRARC */
    ok(FIELD_OFFSET(EMRARC, emr) == 0,
       "FIELD_OFFSET(EMRARC, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRARC, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRARC, rclBox) == 8,
       "FIELD_OFFSET(EMRARC, rclBox) == %ld (expected 8)",
       FIELD_OFFSET(EMRARC, rclBox)); /* RECTL */
    ok(FIELD_OFFSET(EMRARC, ptlStart) == 24,
       "FIELD_OFFSET(EMRARC, ptlStart) == %ld (expected 24)",
       FIELD_OFFSET(EMRARC, ptlStart)); /* POINTL */
    ok(FIELD_OFFSET(EMRARC, ptlEnd) == 32,
       "FIELD_OFFSET(EMRARC, ptlEnd) == %ld (expected 32)",
       FIELD_OFFSET(EMRARC, ptlEnd)); /* POINTL */
    ok(sizeof(EMRARC) == 40, "sizeof(EMRARC) == %d (expected 40)", sizeof(EMRARC));

    /* EMRBITBLT */
    ok(FIELD_OFFSET(EMRBITBLT, emr) == 0,
       "FIELD_OFFSET(EMRBITBLT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRBITBLT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRBITBLT, rclBounds) == 8,
       "FIELD_OFFSET(EMRBITBLT, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRBITBLT, rclBounds)); /* RECTL */
    ok(FIELD_OFFSET(EMRBITBLT, xDest) == 24,
       "FIELD_OFFSET(EMRBITBLT, xDest) == %ld (expected 24)",
       FIELD_OFFSET(EMRBITBLT, xDest)); /* LONG */
    ok(FIELD_OFFSET(EMRBITBLT, yDest) == 28,
       "FIELD_OFFSET(EMRBITBLT, yDest) == %ld (expected 28)",
       FIELD_OFFSET(EMRBITBLT, yDest)); /* LONG */
    ok(FIELD_OFFSET(EMRBITBLT, cxDest) == 32,
       "FIELD_OFFSET(EMRBITBLT, cxDest) == %ld (expected 32)",
       FIELD_OFFSET(EMRBITBLT, cxDest)); /* LONG */
    ok(FIELD_OFFSET(EMRBITBLT, cyDest) == 36,
       "FIELD_OFFSET(EMRBITBLT, cyDest) == %ld (expected 36)",
       FIELD_OFFSET(EMRBITBLT, cyDest)); /* LONG */
    ok(FIELD_OFFSET(EMRBITBLT, dwRop) == 40,
       "FIELD_OFFSET(EMRBITBLT, dwRop) == %ld (expected 40)",
       FIELD_OFFSET(EMRBITBLT, dwRop)); /* DWORD */
    ok(FIELD_OFFSET(EMRBITBLT, xSrc) == 44,
       "FIELD_OFFSET(EMRBITBLT, xSrc) == %ld (expected 44)",
       FIELD_OFFSET(EMRBITBLT, xSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRBITBLT, ySrc) == 48,
       "FIELD_OFFSET(EMRBITBLT, ySrc) == %ld (expected 48)",
       FIELD_OFFSET(EMRBITBLT, ySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRBITBLT, xformSrc) == 52,
       "FIELD_OFFSET(EMRBITBLT, xformSrc) == %ld (expected 52)",
       FIELD_OFFSET(EMRBITBLT, xformSrc)); /* XFORM */
    ok(FIELD_OFFSET(EMRBITBLT, crBkColorSrc) == 76,
       "FIELD_OFFSET(EMRBITBLT, crBkColorSrc) == %ld (expected 76)",
       FIELD_OFFSET(EMRBITBLT, crBkColorSrc)); /* COLORREF */
    ok(FIELD_OFFSET(EMRBITBLT, iUsageSrc) == 80,
       "FIELD_OFFSET(EMRBITBLT, iUsageSrc) == %ld (expected 80)",
       FIELD_OFFSET(EMRBITBLT, iUsageSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRBITBLT, offBmiSrc) == 84,
       "FIELD_OFFSET(EMRBITBLT, offBmiSrc) == %ld (expected 84)",
       FIELD_OFFSET(EMRBITBLT, offBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRBITBLT, cbBmiSrc) == 88,
       "FIELD_OFFSET(EMRBITBLT, cbBmiSrc) == %ld (expected 88)",
       FIELD_OFFSET(EMRBITBLT, cbBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRBITBLT, offBitsSrc) == 92,
       "FIELD_OFFSET(EMRBITBLT, offBitsSrc) == %ld (expected 92)",
       FIELD_OFFSET(EMRBITBLT, offBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRBITBLT, cbBitsSrc) == 96,
       "FIELD_OFFSET(EMRBITBLT, cbBitsSrc) == %ld (expected 96)",
       FIELD_OFFSET(EMRBITBLT, cbBitsSrc)); /* DWORD */
    ok(sizeof(EMRBITBLT) == 100, "sizeof(EMRBITBLT) == %d (expected 100)", sizeof(EMRBITBLT));

    /* EMRCREATEDIBPATTERNBRUSHPT */
    ok(FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, emr) == 0,
       "FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, ihBrush) == 8,
       "FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, ihBrush) == %ld (expected 8)",
       FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, ihBrush)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, iUsage) == 12,
       "FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, iUsage) == %ld (expected 12)",
       FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, iUsage)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, offBmi) == 16,
       "FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, offBmi) == %ld (expected 16)",
       FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, offBmi)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, cbBmi) == 20,
       "FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, cbBmi) == %ld (expected 20)",
       FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, cbBmi)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, offBits) == 24,
       "FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, offBits) == %ld (expected 24)",
       FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, offBits)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, cbBits) == 28,
       "FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, cbBits) == %ld (expected 28)",
       FIELD_OFFSET(EMRCREATEDIBPATTERNBRUSHPT, cbBits)); /* DWORD */
    ok(sizeof(EMRCREATEDIBPATTERNBRUSHPT) == 32, "sizeof(EMRCREATEDIBPATTERNBRUSHPT) == %d (expected 32)", sizeof(EMRCREATEDIBPATTERNBRUSHPT));

    /* EMRCREATEMONOBRUSH */
    ok(FIELD_OFFSET(EMRCREATEMONOBRUSH, emr) == 0,
       "FIELD_OFFSET(EMRCREATEMONOBRUSH, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRCREATEMONOBRUSH, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRCREATEMONOBRUSH, ihBrush) == 8,
       "FIELD_OFFSET(EMRCREATEMONOBRUSH, ihBrush) == %ld (expected 8)",
       FIELD_OFFSET(EMRCREATEMONOBRUSH, ihBrush)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEMONOBRUSH, iUsage) == 12,
       "FIELD_OFFSET(EMRCREATEMONOBRUSH, iUsage) == %ld (expected 12)",
       FIELD_OFFSET(EMRCREATEMONOBRUSH, iUsage)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEMONOBRUSH, offBmi) == 16,
       "FIELD_OFFSET(EMRCREATEMONOBRUSH, offBmi) == %ld (expected 16)",
       FIELD_OFFSET(EMRCREATEMONOBRUSH, offBmi)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEMONOBRUSH, cbBmi) == 20,
       "FIELD_OFFSET(EMRCREATEMONOBRUSH, cbBmi) == %ld (expected 20)",
       FIELD_OFFSET(EMRCREATEMONOBRUSH, cbBmi)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEMONOBRUSH, offBits) == 24,
       "FIELD_OFFSET(EMRCREATEMONOBRUSH, offBits) == %ld (expected 24)",
       FIELD_OFFSET(EMRCREATEMONOBRUSH, offBits)); /* DWORD */
    ok(FIELD_OFFSET(EMRCREATEMONOBRUSH, cbBits) == 28,
       "FIELD_OFFSET(EMRCREATEMONOBRUSH, cbBits) == %ld (expected 28)",
       FIELD_OFFSET(EMRCREATEMONOBRUSH, cbBits)); /* DWORD */
    ok(sizeof(EMRCREATEMONOBRUSH) == 32, "sizeof(EMRCREATEMONOBRUSH) == %d (expected 32)", sizeof(EMRCREATEMONOBRUSH));

    /* EMRDELETECOLORSPACE */
    ok(FIELD_OFFSET(EMRDELETECOLORSPACE, emr) == 0,
       "FIELD_OFFSET(EMRDELETECOLORSPACE, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRDELETECOLORSPACE, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRDELETECOLORSPACE, ihCS) == 8,
       "FIELD_OFFSET(EMRDELETECOLORSPACE, ihCS) == %ld (expected 8)",
       FIELD_OFFSET(EMRDELETECOLORSPACE, ihCS)); /* DWORD */
    ok(sizeof(EMRDELETECOLORSPACE) == 12, "sizeof(EMRDELETECOLORSPACE) == %d (expected 12)", sizeof(EMRDELETECOLORSPACE));

    /* EMRDELETEOBJECT */
    ok(FIELD_OFFSET(EMRDELETEOBJECT, emr) == 0,
       "FIELD_OFFSET(EMRDELETEOBJECT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRDELETEOBJECT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRDELETEOBJECT, ihObject) == 8,
       "FIELD_OFFSET(EMRDELETEOBJECT, ihObject) == %ld (expected 8)",
       FIELD_OFFSET(EMRDELETEOBJECT, ihObject)); /* DWORD */
    ok(sizeof(EMRDELETEOBJECT) == 12, "sizeof(EMRDELETEOBJECT) == %d (expected 12)", sizeof(EMRDELETEOBJECT));

    /* EMRELLIPSE */
    ok(FIELD_OFFSET(EMRELLIPSE, emr) == 0,
       "FIELD_OFFSET(EMRELLIPSE, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRELLIPSE, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRELLIPSE, rclBox) == 8,
       "FIELD_OFFSET(EMRELLIPSE, rclBox) == %ld (expected 8)",
       FIELD_OFFSET(EMRELLIPSE, rclBox)); /* RECTL */
    ok(sizeof(EMRELLIPSE) == 24, "sizeof(EMRELLIPSE) == %d (expected 24)", sizeof(EMRELLIPSE));

    /* EMREOF */
    ok(FIELD_OFFSET(EMREOF, emr) == 0,
       "FIELD_OFFSET(EMREOF, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMREOF, emr)); /* EMR */
    ok(FIELD_OFFSET(EMREOF, nPalEntries) == 8,
       "FIELD_OFFSET(EMREOF, nPalEntries) == %ld (expected 8)",
       FIELD_OFFSET(EMREOF, nPalEntries)); /* DWORD */
    ok(FIELD_OFFSET(EMREOF, offPalEntries) == 12,
       "FIELD_OFFSET(EMREOF, offPalEntries) == %ld (expected 12)",
       FIELD_OFFSET(EMREOF, offPalEntries)); /* DWORD */
    ok(FIELD_OFFSET(EMREOF, nSizeLast) == 16,
       "FIELD_OFFSET(EMREOF, nSizeLast) == %ld (expected 16)",
       FIELD_OFFSET(EMREOF, nSizeLast)); /* DWORD */
    ok(sizeof(EMREOF) == 20, "sizeof(EMREOF) == %d (expected 20)", sizeof(EMREOF));

    /* EMREXCLUDECLIPRECT */
    ok(FIELD_OFFSET(EMREXCLUDECLIPRECT, emr) == 0,
       "FIELD_OFFSET(EMREXCLUDECLIPRECT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMREXCLUDECLIPRECT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMREXCLUDECLIPRECT, rclClip) == 8,
       "FIELD_OFFSET(EMREXCLUDECLIPRECT, rclClip) == %ld (expected 8)",
       FIELD_OFFSET(EMREXCLUDECLIPRECT, rclClip)); /* RECTL */
    ok(sizeof(EMREXCLUDECLIPRECT) == 24, "sizeof(EMREXCLUDECLIPRECT) == %d (expected 24)", sizeof(EMREXCLUDECLIPRECT));

    /* EMREXTFLOODFILL */
    ok(FIELD_OFFSET(EMREXTFLOODFILL, emr) == 0,
       "FIELD_OFFSET(EMREXTFLOODFILL, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMREXTFLOODFILL, emr)); /* EMR */
    ok(FIELD_OFFSET(EMREXTFLOODFILL, ptlStart) == 8,
       "FIELD_OFFSET(EMREXTFLOODFILL, ptlStart) == %ld (expected 8)",
       FIELD_OFFSET(EMREXTFLOODFILL, ptlStart)); /* POINTL */
    ok(FIELD_OFFSET(EMREXTFLOODFILL, crColor) == 16,
       "FIELD_OFFSET(EMREXTFLOODFILL, crColor) == %ld (expected 16)",
       FIELD_OFFSET(EMREXTFLOODFILL, crColor)); /* COLORREF */
    ok(FIELD_OFFSET(EMREXTFLOODFILL, iMode) == 20,
       "FIELD_OFFSET(EMREXTFLOODFILL, iMode) == %ld (expected 20)",
       FIELD_OFFSET(EMREXTFLOODFILL, iMode)); /* DWORD */
    ok(sizeof(EMREXTFLOODFILL) == 24, "sizeof(EMREXTFLOODFILL) == %d (expected 24)", sizeof(EMREXTFLOODFILL));

    /* EMRFILLPATH */
    ok(FIELD_OFFSET(EMRFILLPATH, emr) == 0,
       "FIELD_OFFSET(EMRFILLPATH, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRFILLPATH, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRFILLPATH, rclBounds) == 8,
       "FIELD_OFFSET(EMRFILLPATH, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRFILLPATH, rclBounds)); /* RECTL */
    ok(sizeof(EMRFILLPATH) == 24, "sizeof(EMRFILLPATH) == %d (expected 24)", sizeof(EMRFILLPATH));

    /* EMRFORMAT */
    ok(FIELD_OFFSET(EMRFORMAT, signature) == 0,
       "FIELD_OFFSET(EMRFORMAT, signature) == %ld (expected 0)",
       FIELD_OFFSET(EMRFORMAT, signature)); /* DWORD */
    ok(FIELD_OFFSET(EMRFORMAT, nVersion) == 4,
       "FIELD_OFFSET(EMRFORMAT, nVersion) == %ld (expected 4)",
       FIELD_OFFSET(EMRFORMAT, nVersion)); /* DWORD */
    ok(FIELD_OFFSET(EMRFORMAT, cbData) == 8,
       "FIELD_OFFSET(EMRFORMAT, cbData) == %ld (expected 8)",
       FIELD_OFFSET(EMRFORMAT, cbData)); /* DWORD */
    ok(FIELD_OFFSET(EMRFORMAT, offData) == 12,
       "FIELD_OFFSET(EMRFORMAT, offData) == %ld (expected 12)",
       FIELD_OFFSET(EMRFORMAT, offData)); /* DWORD */
    ok(sizeof(EMRFORMAT) == 16, "sizeof(EMRFORMAT) == %d (expected 16)", sizeof(EMRFORMAT));

    /* EMRLINETO */
    ok(FIELD_OFFSET(EMRLINETO, emr) == 0,
       "FIELD_OFFSET(EMRLINETO, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRLINETO, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRLINETO, ptl) == 8,
       "FIELD_OFFSET(EMRLINETO, ptl) == %ld (expected 8)",
       FIELD_OFFSET(EMRLINETO, ptl)); /* POINTL */
    ok(sizeof(EMRLINETO) == 16, "sizeof(EMRLINETO) == %d (expected 16)", sizeof(EMRLINETO));

    /* EMRMASKBLT */
    ok(FIELD_OFFSET(EMRMASKBLT, emr) == 0,
       "FIELD_OFFSET(EMRMASKBLT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRMASKBLT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRMASKBLT, rclBounds) == 8,
       "FIELD_OFFSET(EMRMASKBLT, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRMASKBLT, rclBounds)); /* RECTL */
    ok(FIELD_OFFSET(EMRMASKBLT, xDest) == 24,
       "FIELD_OFFSET(EMRMASKBLT, xDest) == %ld (expected 24)",
       FIELD_OFFSET(EMRMASKBLT, xDest)); /* LONG */
    ok(FIELD_OFFSET(EMRMASKBLT, yDest) == 28,
       "FIELD_OFFSET(EMRMASKBLT, yDest) == %ld (expected 28)",
       FIELD_OFFSET(EMRMASKBLT, yDest)); /* LONG */
    ok(FIELD_OFFSET(EMRMASKBLT, cxDest) == 32,
       "FIELD_OFFSET(EMRMASKBLT, cxDest) == %ld (expected 32)",
       FIELD_OFFSET(EMRMASKBLT, cxDest)); /* LONG */
    ok(FIELD_OFFSET(EMRMASKBLT, cyDest) == 36,
       "FIELD_OFFSET(EMRMASKBLT, cyDest) == %ld (expected 36)",
       FIELD_OFFSET(EMRMASKBLT, cyDest)); /* LONG */
    ok(FIELD_OFFSET(EMRMASKBLT, dwRop) == 40,
       "FIELD_OFFSET(EMRMASKBLT, dwRop) == %ld (expected 40)",
       FIELD_OFFSET(EMRMASKBLT, dwRop)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, xSrc) == 44,
       "FIELD_OFFSET(EMRMASKBLT, xSrc) == %ld (expected 44)",
       FIELD_OFFSET(EMRMASKBLT, xSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRMASKBLT, ySrc) == 48,
       "FIELD_OFFSET(EMRMASKBLT, ySrc) == %ld (expected 48)",
       FIELD_OFFSET(EMRMASKBLT, ySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRMASKBLT, xformSrc) == 52,
       "FIELD_OFFSET(EMRMASKBLT, xformSrc) == %ld (expected 52)",
       FIELD_OFFSET(EMRMASKBLT, xformSrc)); /* XFORM */
    ok(FIELD_OFFSET(EMRMASKBLT, crBkColorSrc) == 76,
       "FIELD_OFFSET(EMRMASKBLT, crBkColorSrc) == %ld (expected 76)",
       FIELD_OFFSET(EMRMASKBLT, crBkColorSrc)); /* COLORREF */
    ok(FIELD_OFFSET(EMRMASKBLT, iUsageSrc) == 80,
       "FIELD_OFFSET(EMRMASKBLT, iUsageSrc) == %ld (expected 80)",
       FIELD_OFFSET(EMRMASKBLT, iUsageSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, offBmiSrc) == 84,
       "FIELD_OFFSET(EMRMASKBLT, offBmiSrc) == %ld (expected 84)",
       FIELD_OFFSET(EMRMASKBLT, offBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, cbBmiSrc) == 88,
       "FIELD_OFFSET(EMRMASKBLT, cbBmiSrc) == %ld (expected 88)",
       FIELD_OFFSET(EMRMASKBLT, cbBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, offBitsSrc) == 92,
       "FIELD_OFFSET(EMRMASKBLT, offBitsSrc) == %ld (expected 92)",
       FIELD_OFFSET(EMRMASKBLT, offBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, cbBitsSrc) == 96,
       "FIELD_OFFSET(EMRMASKBLT, cbBitsSrc) == %ld (expected 96)",
       FIELD_OFFSET(EMRMASKBLT, cbBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, xMask) == 100,
       "FIELD_OFFSET(EMRMASKBLT, xMask) == %ld (expected 100)",
       FIELD_OFFSET(EMRMASKBLT, xMask)); /* LONG */
    ok(FIELD_OFFSET(EMRMASKBLT, yMask) == 104,
       "FIELD_OFFSET(EMRMASKBLT, yMask) == %ld (expected 104)",
       FIELD_OFFSET(EMRMASKBLT, yMask)); /* LONG */
    ok(FIELD_OFFSET(EMRMASKBLT, iUsageMask) == 108,
       "FIELD_OFFSET(EMRMASKBLT, iUsageMask) == %ld (expected 108)",
       FIELD_OFFSET(EMRMASKBLT, iUsageMask)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, offBmiMask) == 112,
       "FIELD_OFFSET(EMRMASKBLT, offBmiMask) == %ld (expected 112)",
       FIELD_OFFSET(EMRMASKBLT, offBmiMask)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, cbBmiMask) == 116,
       "FIELD_OFFSET(EMRMASKBLT, cbBmiMask) == %ld (expected 116)",
       FIELD_OFFSET(EMRMASKBLT, cbBmiMask)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, offBitsMask) == 120,
       "FIELD_OFFSET(EMRMASKBLT, offBitsMask) == %ld (expected 120)",
       FIELD_OFFSET(EMRMASKBLT, offBitsMask)); /* DWORD */
    ok(FIELD_OFFSET(EMRMASKBLT, cbBitsMask) == 124,
       "FIELD_OFFSET(EMRMASKBLT, cbBitsMask) == %ld (expected 124)",
       FIELD_OFFSET(EMRMASKBLT, cbBitsMask)); /* DWORD */
    ok(sizeof(EMRMASKBLT) == 128, "sizeof(EMRMASKBLT) == %d (expected 128)", sizeof(EMRMASKBLT));

    /* EMRMODIFYWORLDTRANSFORM */
    ok(FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, emr) == 0,
       "FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, xform) == 8,
       "FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, xform) == %ld (expected 8)",
       FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, xform)); /* XFORM */
    ok(FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, iMode) == 32,
       "FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, iMode) == %ld (expected 32)",
       FIELD_OFFSET(EMRMODIFYWORLDTRANSFORM, iMode)); /* DWORD */
    ok(sizeof(EMRMODIFYWORLDTRANSFORM) == 36, "sizeof(EMRMODIFYWORLDTRANSFORM) == %d (expected 36)", sizeof(EMRMODIFYWORLDTRANSFORM));

    /* EMROFFSETCLIPRGN */
    ok(FIELD_OFFSET(EMROFFSETCLIPRGN, emr) == 0,
       "FIELD_OFFSET(EMROFFSETCLIPRGN, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMROFFSETCLIPRGN, emr)); /* EMR */
    ok(FIELD_OFFSET(EMROFFSETCLIPRGN, ptlOffset) == 8,
       "FIELD_OFFSET(EMROFFSETCLIPRGN, ptlOffset) == %ld (expected 8)",
       FIELD_OFFSET(EMROFFSETCLIPRGN, ptlOffset)); /* POINTL */
    ok(sizeof(EMROFFSETCLIPRGN) == 16, "sizeof(EMROFFSETCLIPRGN) == %d (expected 16)", sizeof(EMROFFSETCLIPRGN));

    /* EMRPLGBLT */
    ok(FIELD_OFFSET(EMRPLGBLT, emr) == 0,
       "FIELD_OFFSET(EMRPLGBLT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRPLGBLT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRPLGBLT, rclBounds) == 8,
       "FIELD_OFFSET(EMRPLGBLT, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRPLGBLT, rclBounds)); /* RECTL */
    ok(FIELD_OFFSET(EMRPLGBLT, aptlDst) == 24,
       "FIELD_OFFSET(EMRPLGBLT, aptlDst) == %ld (expected 24)",
       FIELD_OFFSET(EMRPLGBLT, aptlDst)); /* POINTL[3] */
    ok(FIELD_OFFSET(EMRPLGBLT, xSrc) == 48,
       "FIELD_OFFSET(EMRPLGBLT, xSrc) == %ld (expected 48)",
       FIELD_OFFSET(EMRPLGBLT, xSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRPLGBLT, ySrc) == 52,
       "FIELD_OFFSET(EMRPLGBLT, ySrc) == %ld (expected 52)",
       FIELD_OFFSET(EMRPLGBLT, ySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRPLGBLT, cxSrc) == 56,
       "FIELD_OFFSET(EMRPLGBLT, cxSrc) == %ld (expected 56)",
       FIELD_OFFSET(EMRPLGBLT, cxSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRPLGBLT, cySrc) == 60,
       "FIELD_OFFSET(EMRPLGBLT, cySrc) == %ld (expected 60)",
       FIELD_OFFSET(EMRPLGBLT, cySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRPLGBLT, xformSrc) == 64,
       "FIELD_OFFSET(EMRPLGBLT, xformSrc) == %ld (expected 64)",
       FIELD_OFFSET(EMRPLGBLT, xformSrc)); /* XFORM */
    ok(FIELD_OFFSET(EMRPLGBLT, crBkColorSrc) == 88,
       "FIELD_OFFSET(EMRPLGBLT, crBkColorSrc) == %ld (expected 88)",
       FIELD_OFFSET(EMRPLGBLT, crBkColorSrc)); /* COLORREF */
    ok(FIELD_OFFSET(EMRPLGBLT, iUsageSrc) == 92,
       "FIELD_OFFSET(EMRPLGBLT, iUsageSrc) == %ld (expected 92)",
       FIELD_OFFSET(EMRPLGBLT, iUsageSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, offBmiSrc) == 96,
       "FIELD_OFFSET(EMRPLGBLT, offBmiSrc) == %ld (expected 96)",
       FIELD_OFFSET(EMRPLGBLT, offBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, cbBmiSrc) == 100,
       "FIELD_OFFSET(EMRPLGBLT, cbBmiSrc) == %ld (expected 100)",
       FIELD_OFFSET(EMRPLGBLT, cbBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, offBitsSrc) == 104,
       "FIELD_OFFSET(EMRPLGBLT, offBitsSrc) == %ld (expected 104)",
       FIELD_OFFSET(EMRPLGBLT, offBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, cbBitsSrc) == 108,
       "FIELD_OFFSET(EMRPLGBLT, cbBitsSrc) == %ld (expected 108)",
       FIELD_OFFSET(EMRPLGBLT, cbBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, xMask) == 112,
       "FIELD_OFFSET(EMRPLGBLT, xMask) == %ld (expected 112)",
       FIELD_OFFSET(EMRPLGBLT, xMask)); /* LONG */
    ok(FIELD_OFFSET(EMRPLGBLT, yMask) == 116,
       "FIELD_OFFSET(EMRPLGBLT, yMask) == %ld (expected 116)",
       FIELD_OFFSET(EMRPLGBLT, yMask)); /* LONG */
    ok(FIELD_OFFSET(EMRPLGBLT, iUsageMask) == 120,
       "FIELD_OFFSET(EMRPLGBLT, iUsageMask) == %ld (expected 120)",
       FIELD_OFFSET(EMRPLGBLT, iUsageMask)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, offBmiMask) == 124,
       "FIELD_OFFSET(EMRPLGBLT, offBmiMask) == %ld (expected 124)",
       FIELD_OFFSET(EMRPLGBLT, offBmiMask)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, cbBmiMask) == 128,
       "FIELD_OFFSET(EMRPLGBLT, cbBmiMask) == %ld (expected 128)",
       FIELD_OFFSET(EMRPLGBLT, cbBmiMask)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, offBitsMask) == 132,
       "FIELD_OFFSET(EMRPLGBLT, offBitsMask) == %ld (expected 132)",
       FIELD_OFFSET(EMRPLGBLT, offBitsMask)); /* DWORD */
    ok(FIELD_OFFSET(EMRPLGBLT, cbBitsMask) == 136,
       "FIELD_OFFSET(EMRPLGBLT, cbBitsMask) == %ld (expected 136)",
       FIELD_OFFSET(EMRPLGBLT, cbBitsMask)); /* DWORD */
    ok(sizeof(EMRPLGBLT) == 140, "sizeof(EMRPLGBLT) == %d (expected 140)", sizeof(EMRPLGBLT));

    /* EMRPOLYLINE */
    ok(FIELD_OFFSET(EMRPOLYLINE, emr) == 0,
       "FIELD_OFFSET(EMRPOLYLINE, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRPOLYLINE, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRPOLYLINE, rclBounds) == 8,
       "FIELD_OFFSET(EMRPOLYLINE, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRPOLYLINE, rclBounds)); /* RECTL */
    ok(FIELD_OFFSET(EMRPOLYLINE, cptl) == 24,
       "FIELD_OFFSET(EMRPOLYLINE, cptl) == %ld (expected 24)",
       FIELD_OFFSET(EMRPOLYLINE, cptl)); /* DWORD */
    ok(FIELD_OFFSET(EMRPOLYLINE, aptl) == 28,
       "FIELD_OFFSET(EMRPOLYLINE, aptl) == %ld (expected 28)",
       FIELD_OFFSET(EMRPOLYLINE, aptl)); /* POINTL[1] */
    ok(sizeof(EMRPOLYLINE) == 36, "sizeof(EMRPOLYLINE) == %d (expected 36)", sizeof(EMRPOLYLINE));

    /* EMRPOLYPOLYLINE */
    ok(FIELD_OFFSET(EMRPOLYPOLYLINE, emr) == 0,
       "FIELD_OFFSET(EMRPOLYPOLYLINE, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRPOLYPOLYLINE, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRPOLYPOLYLINE, rclBounds) == 8,
       "FIELD_OFFSET(EMRPOLYPOLYLINE, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRPOLYPOLYLINE, rclBounds)); /* RECTL */
    ok(FIELD_OFFSET(EMRPOLYPOLYLINE, nPolys) == 24,
       "FIELD_OFFSET(EMRPOLYPOLYLINE, nPolys) == %ld (expected 24)",
       FIELD_OFFSET(EMRPOLYPOLYLINE, nPolys)); /* DWORD */
    ok(FIELD_OFFSET(EMRPOLYPOLYLINE, cptl) == 28,
       "FIELD_OFFSET(EMRPOLYPOLYLINE, cptl) == %ld (expected 28)",
       FIELD_OFFSET(EMRPOLYPOLYLINE, cptl)); /* DWORD */
    ok(FIELD_OFFSET(EMRPOLYPOLYLINE, aPolyCounts) == 32,
       "FIELD_OFFSET(EMRPOLYPOLYLINE, aPolyCounts) == %ld (expected 32)",
       FIELD_OFFSET(EMRPOLYPOLYLINE, aPolyCounts)); /* DWORD[1] */
    ok(FIELD_OFFSET(EMRPOLYPOLYLINE, aptl) == 36,
       "FIELD_OFFSET(EMRPOLYPOLYLINE, aptl) == %ld (expected 36)",
       FIELD_OFFSET(EMRPOLYPOLYLINE, aptl)); /* POINTL[1] */
    ok(sizeof(EMRPOLYPOLYLINE) == 44, "sizeof(EMRPOLYPOLYLINE) == %d (expected 44)", sizeof(EMRPOLYPOLYLINE));

    /* EMRRESIZEPALETTE */
    ok(FIELD_OFFSET(EMRRESIZEPALETTE, emr) == 0,
       "FIELD_OFFSET(EMRRESIZEPALETTE, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRRESIZEPALETTE, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRRESIZEPALETTE, ihPal) == 8,
       "FIELD_OFFSET(EMRRESIZEPALETTE, ihPal) == %ld (expected 8)",
       FIELD_OFFSET(EMRRESIZEPALETTE, ihPal)); /* DWORD */
    ok(FIELD_OFFSET(EMRRESIZEPALETTE, cEntries) == 12,
       "FIELD_OFFSET(EMRRESIZEPALETTE, cEntries) == %ld (expected 12)",
       FIELD_OFFSET(EMRRESIZEPALETTE, cEntries)); /* DWORD */
    ok(sizeof(EMRRESIZEPALETTE) == 16, "sizeof(EMRRESIZEPALETTE) == %d (expected 16)", sizeof(EMRRESIZEPALETTE));

    /* EMRRESTOREDC */
    ok(FIELD_OFFSET(EMRRESTOREDC, emr) == 0,
       "FIELD_OFFSET(EMRRESTOREDC, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRRESTOREDC, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRRESTOREDC, iRelative) == 8,
       "FIELD_OFFSET(EMRRESTOREDC, iRelative) == %ld (expected 8)",
       FIELD_OFFSET(EMRRESTOREDC, iRelative)); /* LONG */
    ok(sizeof(EMRRESTOREDC) == 12, "sizeof(EMRRESTOREDC) == %d (expected 12)", sizeof(EMRRESTOREDC));

    /* EMRROUNDRECT */
    ok(FIELD_OFFSET(EMRROUNDRECT, emr) == 0,
       "FIELD_OFFSET(EMRROUNDRECT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRROUNDRECT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRROUNDRECT, rclBox) == 8,
       "FIELD_OFFSET(EMRROUNDRECT, rclBox) == %ld (expected 8)",
       FIELD_OFFSET(EMRROUNDRECT, rclBox)); /* RECTL */
    ok(FIELD_OFFSET(EMRROUNDRECT, szlCorner) == 24,
       "FIELD_OFFSET(EMRROUNDRECT, szlCorner) == %ld (expected 24)",
       FIELD_OFFSET(EMRROUNDRECT, szlCorner)); /* SIZEL */
    ok(sizeof(EMRROUNDRECT) == 32, "sizeof(EMRROUNDRECT) == %d (expected 32)", sizeof(EMRROUNDRECT));

    /* EMRSCALEVIEWPORTEXTEX */
    ok(FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, emr) == 0,
       "FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, xNum) == 8,
       "FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, xNum) == %ld (expected 8)",
       FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, xNum)); /* LONG */
    ok(FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, xDenom) == 12,
       "FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, xDenom) == %ld (expected 12)",
       FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, xDenom)); /* LONG */
    ok(FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, yNum) == 16,
       "FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, yNum) == %ld (expected 16)",
       FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, yNum)); /* LONG */
    ok(FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, yDenom) == 20,
       "FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, yDenom) == %ld (expected 20)",
       FIELD_OFFSET(EMRSCALEVIEWPORTEXTEX, yDenom)); /* LONG */
    ok(sizeof(EMRSCALEVIEWPORTEXTEX) == 24, "sizeof(EMRSCALEVIEWPORTEXTEX) == %d (expected 24)", sizeof(EMRSCALEVIEWPORTEXTEX));

    /* EMRSELECTCLIPPATH */
    ok(FIELD_OFFSET(EMRSELECTCLIPPATH, emr) == 0,
       "FIELD_OFFSET(EMRSELECTCLIPPATH, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSELECTCLIPPATH, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSELECTCLIPPATH, iMode) == 8,
       "FIELD_OFFSET(EMRSELECTCLIPPATH, iMode) == %ld (expected 8)",
       FIELD_OFFSET(EMRSELECTCLIPPATH, iMode)); /* DWORD */
    ok(sizeof(EMRSELECTCLIPPATH) == 12, "sizeof(EMRSELECTCLIPPATH) == %d (expected 12)", sizeof(EMRSELECTCLIPPATH));

    /* EMRSELECTPALETTE */
    ok(FIELD_OFFSET(EMRSELECTPALETTE, emr) == 0,
       "FIELD_OFFSET(EMRSELECTPALETTE, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSELECTPALETTE, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSELECTPALETTE, ihPal) == 8,
       "FIELD_OFFSET(EMRSELECTPALETTE, ihPal) == %ld (expected 8)",
       FIELD_OFFSET(EMRSELECTPALETTE, ihPal)); /* DWORD */
    ok(sizeof(EMRSELECTPALETTE) == 12, "sizeof(EMRSELECTPALETTE) == %d (expected 12)", sizeof(EMRSELECTPALETTE));

    /* EMRSETARCDIRECTION */
    ok(FIELD_OFFSET(EMRSETARCDIRECTION, emr) == 0,
       "FIELD_OFFSET(EMRSETARCDIRECTION, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETARCDIRECTION, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETARCDIRECTION, iArcDirection) == 8,
       "FIELD_OFFSET(EMRSETARCDIRECTION, iArcDirection) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETARCDIRECTION, iArcDirection)); /* DWORD */
    ok(sizeof(EMRSETARCDIRECTION) == 12, "sizeof(EMRSETARCDIRECTION) == %d (expected 12)", sizeof(EMRSETARCDIRECTION));

    /* EMRSETBKCOLOR */
    ok(FIELD_OFFSET(EMRSETBKCOLOR, emr) == 0,
       "FIELD_OFFSET(EMRSETBKCOLOR, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETBKCOLOR, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETBKCOLOR, crColor) == 8,
       "FIELD_OFFSET(EMRSETBKCOLOR, crColor) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETBKCOLOR, crColor)); /* COLORREF */
    ok(sizeof(EMRSETBKCOLOR) == 12, "sizeof(EMRSETBKCOLOR) == %d (expected 12)", sizeof(EMRSETBKCOLOR));

    /* EMRSETBRUSHORGEX */
    ok(FIELD_OFFSET(EMRSETBRUSHORGEX, emr) == 0,
       "FIELD_OFFSET(EMRSETBRUSHORGEX, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETBRUSHORGEX, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETBRUSHORGEX, ptlOrigin) == 8,
       "FIELD_OFFSET(EMRSETBRUSHORGEX, ptlOrigin) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETBRUSHORGEX, ptlOrigin)); /* POINTL */
    ok(sizeof(EMRSETBRUSHORGEX) == 16, "sizeof(EMRSETBRUSHORGEX) == %d (expected 16)", sizeof(EMRSETBRUSHORGEX));

    /* EMRSETDIBITSTODEIVCE */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, emr) == 0,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, rclBounds) == 8,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, rclBounds)); /* RECTL */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, xDest) == 24,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, xDest) == %ld (expected 24)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, xDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, yDest) == 28,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, yDest) == %ld (expected 28)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, yDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, xSrc) == 32,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, xSrc) == %ld (expected 32)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, xSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, ySrc) == 36,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, ySrc) == %ld (expected 36)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, ySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cxSrc) == 40,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cxSrc) == %ld (expected 40)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cxSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cySrc) == 44,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cySrc) == %ld (expected 44)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, offBmiSrc) == 48,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, offBmiSrc) == %ld (expected 48)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, offBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cbBmiSrc) == 52,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cbBmiSrc) == %ld (expected 52)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cbBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, offBitsSrc) == 56,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, offBitsSrc) == %ld (expected 56)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, offBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cbBitsSrc) == 60,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cbBitsSrc) == %ld (expected 60)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cbBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, iUsageSrc) == 64,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, iUsageSrc) == %ld (expected 64)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, iUsageSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, iStartScan) == 68,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, iStartScan) == %ld (expected 68)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, iStartScan)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cScans) == 72,
       "FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cScans) == %ld (expected 72)",
       FIELD_OFFSET(EMRSETDIBITSTODEIVCE, cScans)); /* DWORD */
    ok(sizeof(EMRSETDIBITSTODEIVCE) == 76, "sizeof(EMRSETDIBITSTODEIVCE) == %d (expected 76)", sizeof(EMRSETDIBITSTODEIVCE));

    /* EMRSETMAPPERFLAGS */
    ok(FIELD_OFFSET(EMRSETMAPPERFLAGS, emr) == 0,
       "FIELD_OFFSET(EMRSETMAPPERFLAGS, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETMAPPERFLAGS, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETMAPPERFLAGS, dwFlags) == 8,
       "FIELD_OFFSET(EMRSETMAPPERFLAGS, dwFlags) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETMAPPERFLAGS, dwFlags)); /* DWORD */
    ok(sizeof(EMRSETMAPPERFLAGS) == 12, "sizeof(EMRSETMAPPERFLAGS) == %d (expected 12)", sizeof(EMRSETMAPPERFLAGS));

    /* EMRSETMITERLIMIT */
    ok(FIELD_OFFSET(EMRSETMITERLIMIT, emr) == 0,
       "FIELD_OFFSET(EMRSETMITERLIMIT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETMITERLIMIT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETMITERLIMIT, eMiterLimit) == 8,
       "FIELD_OFFSET(EMRSETMITERLIMIT, eMiterLimit) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETMITERLIMIT, eMiterLimit)); /* FLOAT */
    ok(sizeof(EMRSETMITERLIMIT) == 12, "sizeof(EMRSETMITERLIMIT) == %d (expected 12)", sizeof(EMRSETMITERLIMIT));

    /* EMRSETPALETTEENTRIES */
    ok(FIELD_OFFSET(EMRSETPALETTEENTRIES, emr) == 0,
       "FIELD_OFFSET(EMRSETPALETTEENTRIES, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETPALETTEENTRIES, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETPALETTEENTRIES, ihPal) == 8,
       "FIELD_OFFSET(EMRSETPALETTEENTRIES, ihPal) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETPALETTEENTRIES, ihPal)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETPALETTEENTRIES, iStart) == 12,
       "FIELD_OFFSET(EMRSETPALETTEENTRIES, iStart) == %ld (expected 12)",
       FIELD_OFFSET(EMRSETPALETTEENTRIES, iStart)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETPALETTEENTRIES, cEntries) == 16,
       "FIELD_OFFSET(EMRSETPALETTEENTRIES, cEntries) == %ld (expected 16)",
       FIELD_OFFSET(EMRSETPALETTEENTRIES, cEntries)); /* DWORD */
    ok(FIELD_OFFSET(EMRSETPALETTEENTRIES, aPalEntries) == 20,
       "FIELD_OFFSET(EMRSETPALETTEENTRIES, aPalEntries) == %ld (expected 20)",
       FIELD_OFFSET(EMRSETPALETTEENTRIES, aPalEntries)); /* PALETTEENTRY[1] */
    ok(sizeof(EMRSETPALETTEENTRIES) == 24, "sizeof(EMRSETPALETTEENTRIES) == %d (expected 24)", sizeof(EMRSETPALETTEENTRIES));

    /* EMRSETPIXELV */
    ok(FIELD_OFFSET(EMRSETPIXELV, emr) == 0,
       "FIELD_OFFSET(EMRSETPIXELV, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETPIXELV, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETPIXELV, ptlPixel) == 8,
       "FIELD_OFFSET(EMRSETPIXELV, ptlPixel) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETPIXELV, ptlPixel)); /* POINTL */
    ok(FIELD_OFFSET(EMRSETPIXELV, crColor) == 16,
       "FIELD_OFFSET(EMRSETPIXELV, crColor) == %ld (expected 16)",
       FIELD_OFFSET(EMRSETPIXELV, crColor)); /* COLORREF */
    ok(sizeof(EMRSETPIXELV) == 20, "sizeof(EMRSETPIXELV) == %d (expected 20)", sizeof(EMRSETPIXELV));

    /* EMRSETTEXTJUSTIFICATION */
    ok(FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, emr) == 0,
       "FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, nBreakExtra) == 8,
       "FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, nBreakExtra) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, nBreakExtra)); /* INT */
    ok(FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, nBreakCount) == 12,
       "FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, nBreakCount) == %ld (expected 12)",
       FIELD_OFFSET(EMRSETTEXTJUSTIFICATION, nBreakCount)); /* INT */
    ok(sizeof(EMRSETTEXTJUSTIFICATION) == 16, "sizeof(EMRSETTEXTJUSTIFICATION) == %d (expected 16)", sizeof(EMRSETTEXTJUSTIFICATION));

    /* EMRSETVIEWPORTEXTEX */
    ok(FIELD_OFFSET(EMRSETVIEWPORTEXTEX, emr) == 0,
       "FIELD_OFFSET(EMRSETVIEWPORTEXTEX, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETVIEWPORTEXTEX, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETVIEWPORTEXTEX, szlExtent) == 8,
       "FIELD_OFFSET(EMRSETVIEWPORTEXTEX, szlExtent) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETVIEWPORTEXTEX, szlExtent)); /* SIZEL */
    ok(sizeof(EMRSETVIEWPORTEXTEX) == 16, "sizeof(EMRSETVIEWPORTEXTEX) == %d (expected 16)", sizeof(EMRSETVIEWPORTEXTEX));

    /* EMRSETWORLDTRANSFORM */
    ok(FIELD_OFFSET(EMRSETWORLDTRANSFORM, emr) == 0,
       "FIELD_OFFSET(EMRSETWORLDTRANSFORM, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSETWORLDTRANSFORM, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSETWORLDTRANSFORM, xform) == 8,
       "FIELD_OFFSET(EMRSETWORLDTRANSFORM, xform) == %ld (expected 8)",
       FIELD_OFFSET(EMRSETWORLDTRANSFORM, xform)); /* XFORM */
    ok(sizeof(EMRSETWORLDTRANSFORM) == 32, "sizeof(EMRSETWORLDTRANSFORM) == %d (expected 32)", sizeof(EMRSETWORLDTRANSFORM));

    /* EMRSTRETCHBLT */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, emr) == 0,
       "FIELD_OFFSET(EMRSTRETCHBLT, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSTRETCHBLT, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, rclBounds) == 8,
       "FIELD_OFFSET(EMRSTRETCHBLT, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRSTRETCHBLT, rclBounds)); /* RECTL */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, xDest) == 24,
       "FIELD_OFFSET(EMRSTRETCHBLT, xDest) == %ld (expected 24)",
       FIELD_OFFSET(EMRSTRETCHBLT, xDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, yDest) == 28,
       "FIELD_OFFSET(EMRSTRETCHBLT, yDest) == %ld (expected 28)",
       FIELD_OFFSET(EMRSTRETCHBLT, yDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, cxDest) == 32,
       "FIELD_OFFSET(EMRSTRETCHBLT, cxDest) == %ld (expected 32)",
       FIELD_OFFSET(EMRSTRETCHBLT, cxDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, cyDest) == 36,
       "FIELD_OFFSET(EMRSTRETCHBLT, cyDest) == %ld (expected 36)",
       FIELD_OFFSET(EMRSTRETCHBLT, cyDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, dwRop) == 40,
       "FIELD_OFFSET(EMRSTRETCHBLT, dwRop) == %ld (expected 40)",
       FIELD_OFFSET(EMRSTRETCHBLT, dwRop)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, xSrc) == 44,
       "FIELD_OFFSET(EMRSTRETCHBLT, xSrc) == %ld (expected 44)",
       FIELD_OFFSET(EMRSTRETCHBLT, xSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, ySrc) == 48,
       "FIELD_OFFSET(EMRSTRETCHBLT, ySrc) == %ld (expected 48)",
       FIELD_OFFSET(EMRSTRETCHBLT, ySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, xformSrc) == 52,
       "FIELD_OFFSET(EMRSTRETCHBLT, xformSrc) == %ld (expected 52)",
       FIELD_OFFSET(EMRSTRETCHBLT, xformSrc)); /* XFORM */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, crBkColorSrc) == 76,
       "FIELD_OFFSET(EMRSTRETCHBLT, crBkColorSrc) == %ld (expected 76)",
       FIELD_OFFSET(EMRSTRETCHBLT, crBkColorSrc)); /* COLORREF */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, iUsageSrc) == 80,
       "FIELD_OFFSET(EMRSTRETCHBLT, iUsageSrc) == %ld (expected 80)",
       FIELD_OFFSET(EMRSTRETCHBLT, iUsageSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, offBmiSrc) == 84,
       "FIELD_OFFSET(EMRSTRETCHBLT, offBmiSrc) == %ld (expected 84)",
       FIELD_OFFSET(EMRSTRETCHBLT, offBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, cbBmiSrc) == 88,
       "FIELD_OFFSET(EMRSTRETCHBLT, cbBmiSrc) == %ld (expected 88)",
       FIELD_OFFSET(EMRSTRETCHBLT, cbBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, offBitsSrc) == 92,
       "FIELD_OFFSET(EMRSTRETCHBLT, offBitsSrc) == %ld (expected 92)",
       FIELD_OFFSET(EMRSTRETCHBLT, offBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, cbBitsSrc) == 96,
       "FIELD_OFFSET(EMRSTRETCHBLT, cbBitsSrc) == %ld (expected 96)",
       FIELD_OFFSET(EMRSTRETCHBLT, cbBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, cxSrc) == 100,
       "FIELD_OFFSET(EMRSTRETCHBLT, cxSrc) == %ld (expected 100)",
       FIELD_OFFSET(EMRSTRETCHBLT, cxSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHBLT, cySrc) == 104,
       "FIELD_OFFSET(EMRSTRETCHBLT, cySrc) == %ld (expected 104)",
       FIELD_OFFSET(EMRSTRETCHBLT, cySrc)); /* LONG */
    ok(sizeof(EMRSTRETCHBLT) == 108, "sizeof(EMRSTRETCHBLT) == %d (expected 108)", sizeof(EMRSTRETCHBLT));

    /* EMRSTRETCHDIBITS */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, emr) == 0,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, emr) == %ld (expected 0)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, emr)); /* EMR */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, rclBounds) == 8,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, rclBounds) == %ld (expected 8)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, rclBounds)); /* RECTL */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, xDest) == 24,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, xDest) == %ld (expected 24)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, xDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, yDest) == 28,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, yDest) == %ld (expected 28)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, yDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, xSrc) == 32,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, xSrc) == %ld (expected 32)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, xSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, ySrc) == 36,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, ySrc) == %ld (expected 36)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, ySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, cxSrc) == 40,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, cxSrc) == %ld (expected 40)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, cxSrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, cySrc) == 44,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, cySrc) == %ld (expected 44)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, cySrc)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, offBmiSrc) == 48,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, offBmiSrc) == %ld (expected 48)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, offBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, cbBmiSrc) == 52,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, cbBmiSrc) == %ld (expected 52)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, cbBmiSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, offBitsSrc) == 56,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, offBitsSrc) == %ld (expected 56)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, offBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, cbBitsSrc) == 60,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, cbBitsSrc) == %ld (expected 60)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, cbBitsSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, iUsageSrc) == 64,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, iUsageSrc) == %ld (expected 64)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, iUsageSrc)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, dwRop) == 68,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, dwRop) == %ld (expected 68)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, dwRop)); /* DWORD */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, cxDest) == 72,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, cxDest) == %ld (expected 72)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, cxDest)); /* LONG */
    ok(FIELD_OFFSET(EMRSTRETCHDIBITS, cyDest) == 76,
       "FIELD_OFFSET(EMRSTRETCHDIBITS, cyDest) == %ld (expected 76)",
       FIELD_OFFSET(EMRSTRETCHDIBITS, cyDest)); /* LONG */
    ok(sizeof(EMRSTRETCHDIBITS) == 80, "sizeof(EMRSTRETCHDIBITS) == %d (expected 80)", sizeof(EMRSTRETCHDIBITS));

    /* EMRTEXT */
    ok(FIELD_OFFSET(EMRTEXT, ptlReference) == 0,
       "FIELD_OFFSET(EMRTEXT, ptlReference) == %ld (expected 0)",
       FIELD_OFFSET(EMRTEXT, ptlReference)); /* POINTL */
    ok(FIELD_OFFSET(EMRTEXT, nChars) == 8,
       "FIELD_OFFSET(EMRTEXT, nChars) == %ld (expected 8)",
       FIELD_OFFSET(EMRTEXT, nChars)); /* DWORD */
    ok(FIELD_OFFSET(EMRTEXT, offString) == 12,
       "FIELD_OFFSET(EMRTEXT, offString) == %ld (expected 12)",
       FIELD_OFFSET(EMRTEXT, offString)); /* DWORD */
    ok(FIELD_OFFSET(EMRTEXT, fOptions) == 16,
       "FIELD_OFFSET(EMRTEXT, fOptions) == %ld (expected 16)",
       FIELD_OFFSET(EMRTEXT, fOptions)); /* DWORD */
    ok(FIELD_OFFSET(EMRTEXT, rcl) == 20,
       "FIELD_OFFSET(EMRTEXT, rcl) == %ld (expected 20)",
       FIELD_OFFSET(EMRTEXT, rcl)); /* RECTL */
    ok(FIELD_OFFSET(EMRTEXT, offDx) == 36,
       "FIELD_OFFSET(EMRTEXT, offDx) == %ld (expected 36)",
       FIELD_OFFSET(EMRTEXT, offDx)); /* DWORD */
    ok(sizeof(EMRTEXT) == 40, "sizeof(EMRTEXT) == %d (expected 40)", sizeof(EMRTEXT));

    /* ENHMETARECORD */
    ok(FIELD_OFFSET(ENHMETARECORD, iType) == 0,
       "FIELD_OFFSET(ENHMETARECORD, iType) == %ld (expected 0)",
       FIELD_OFFSET(ENHMETARECORD, iType)); /* DWORD */
    ok(FIELD_OFFSET(ENHMETARECORD, nSize) == 4,
       "FIELD_OFFSET(ENHMETARECORD, nSize) == %ld (expected 4)",
       FIELD_OFFSET(ENHMETARECORD, nSize)); /* DWORD */
    ok(FIELD_OFFSET(ENHMETARECORD, dParm) == 8,
       "FIELD_OFFSET(ENHMETARECORD, dParm) == %ld (expected 8)",
       FIELD_OFFSET(ENHMETARECORD, dParm)); /* DWORD[1] */
    ok(sizeof(ENHMETARECORD) == 12, "sizeof(ENHMETARECORD) == %d (expected 12)", sizeof(ENHMETARECORD));

    /* EXTLOGPEN */
    ok(FIELD_OFFSET(EXTLOGPEN, elpPenStyle) == 0,
       "FIELD_OFFSET(EXTLOGPEN, elpPenStyle) == %ld (expected 0)",
       FIELD_OFFSET(EXTLOGPEN, elpPenStyle)); /* DWORD */
    ok(FIELD_OFFSET(EXTLOGPEN, elpWidth) == 4,
       "FIELD_OFFSET(EXTLOGPEN, elpWidth) == %ld (expected 4)",
       FIELD_OFFSET(EXTLOGPEN, elpWidth)); /* DWORD */
    ok(FIELD_OFFSET(EXTLOGPEN, elpBrushStyle) == 8,
       "FIELD_OFFSET(EXTLOGPEN, elpBrushStyle) == %ld (expected 8)",
       FIELD_OFFSET(EXTLOGPEN, elpBrushStyle)); /* UINT */
    ok(FIELD_OFFSET(EXTLOGPEN, elpColor) == 12,
       "FIELD_OFFSET(EXTLOGPEN, elpColor) == %ld (expected 12)",
       FIELD_OFFSET(EXTLOGPEN, elpColor)); /* COLORREF */
    ok(FIELD_OFFSET(EXTLOGPEN, elpHatch) == 16,
       "FIELD_OFFSET(EXTLOGPEN, elpHatch) == %ld (expected 16)",
       FIELD_OFFSET(EXTLOGPEN, elpHatch)); /* LONG */
    ok(FIELD_OFFSET(EXTLOGPEN, elpNumEntries) == 20,
       "FIELD_OFFSET(EXTLOGPEN, elpNumEntries) == %ld (expected 20)",
       FIELD_OFFSET(EXTLOGPEN, elpNumEntries)); /* DWORD */
    ok(FIELD_OFFSET(EXTLOGPEN, elpStyleEntry) == 24,
       "FIELD_OFFSET(EXTLOGPEN, elpStyleEntry) == %ld (expected 24)",
       FIELD_OFFSET(EXTLOGPEN, elpStyleEntry)); /* DWORD[1] */
    ok(sizeof(EXTLOGPEN) == 28, "sizeof(EXTLOGPEN) == %d (expected 28)", sizeof(EXTLOGPEN));

    /* FONTSIGNATURE */
    ok(FIELD_OFFSET(FONTSIGNATURE, fsUsb) == 0,
       "FIELD_OFFSET(FONTSIGNATURE, fsUsb) == %ld (expected 0)",
       FIELD_OFFSET(FONTSIGNATURE, fsUsb)); /* DWORD[4] */
    ok(FIELD_OFFSET(FONTSIGNATURE, fsCsb) == 16,
       "FIELD_OFFSET(FONTSIGNATURE, fsCsb) == %ld (expected 16)",
       FIELD_OFFSET(FONTSIGNATURE, fsCsb)); /* DWORD[2] */
    ok(sizeof(FONTSIGNATURE) == 24, "sizeof(FONTSIGNATURE) == %d (expected 24)", sizeof(FONTSIGNATURE));

    /* GCP_RESULTSA */
    ok(FIELD_OFFSET(GCP_RESULTSA, lStructSize) == 0,
       "FIELD_OFFSET(GCP_RESULTSA, lStructSize) == %ld (expected 0)",
       FIELD_OFFSET(GCP_RESULTSA, lStructSize)); /* DWORD */
    ok(FIELD_OFFSET(GCP_RESULTSA, lpOutString) == 4,
       "FIELD_OFFSET(GCP_RESULTSA, lpOutString) == %ld (expected 4)",
       FIELD_OFFSET(GCP_RESULTSA, lpOutString)); /* LPSTR */
    ok(FIELD_OFFSET(GCP_RESULTSA, lpOrder) == 8,
       "FIELD_OFFSET(GCP_RESULTSA, lpOrder) == %ld (expected 8)",
       FIELD_OFFSET(GCP_RESULTSA, lpOrder)); /* UINT * */
    ok(FIELD_OFFSET(GCP_RESULTSA, lpDx) == 12,
       "FIELD_OFFSET(GCP_RESULTSA, lpDx) == %ld (expected 12)",
       FIELD_OFFSET(GCP_RESULTSA, lpDx)); /* INT * */
    ok(FIELD_OFFSET(GCP_RESULTSA, lpCaretPos) == 16,
       "FIELD_OFFSET(GCP_RESULTSA, lpCaretPos) == %ld (expected 16)",
       FIELD_OFFSET(GCP_RESULTSA, lpCaretPos)); /* INT * */
    ok(FIELD_OFFSET(GCP_RESULTSA, lpClass) == 20,
       "FIELD_OFFSET(GCP_RESULTSA, lpClass) == %ld (expected 20)",
       FIELD_OFFSET(GCP_RESULTSA, lpClass)); /* LPSTR */
    ok(FIELD_OFFSET(GCP_RESULTSA, lpGlyphs) == 24,
       "FIELD_OFFSET(GCP_RESULTSA, lpGlyphs) == %ld (expected 24)",
       FIELD_OFFSET(GCP_RESULTSA, lpGlyphs)); /* LPWSTR */
    ok(FIELD_OFFSET(GCP_RESULTSA, nGlyphs) == 28,
       "FIELD_OFFSET(GCP_RESULTSA, nGlyphs) == %ld (expected 28)",
       FIELD_OFFSET(GCP_RESULTSA, nGlyphs)); /* UINT */
    ok(FIELD_OFFSET(GCP_RESULTSA, nMaxFit) == 32,
       "FIELD_OFFSET(GCP_RESULTSA, nMaxFit) == %ld (expected 32)",
       FIELD_OFFSET(GCP_RESULTSA, nMaxFit)); /* UINT */
    ok(sizeof(GCP_RESULTSA) == 36, "sizeof(GCP_RESULTSA) == %d (expected 36)", sizeof(GCP_RESULTSA));

    /* GCP_RESULTSW */
    ok(FIELD_OFFSET(GCP_RESULTSW, lStructSize) == 0,
       "FIELD_OFFSET(GCP_RESULTSW, lStructSize) == %ld (expected 0)",
       FIELD_OFFSET(GCP_RESULTSW, lStructSize)); /* DWORD */
    ok(FIELD_OFFSET(GCP_RESULTSW, lpOutString) == 4,
       "FIELD_OFFSET(GCP_RESULTSW, lpOutString) == %ld (expected 4)",
       FIELD_OFFSET(GCP_RESULTSW, lpOutString)); /* LPWSTR */
    ok(FIELD_OFFSET(GCP_RESULTSW, lpOrder) == 8,
       "FIELD_OFFSET(GCP_RESULTSW, lpOrder) == %ld (expected 8)",
       FIELD_OFFSET(GCP_RESULTSW, lpOrder)); /* UINT * */
    ok(FIELD_OFFSET(GCP_RESULTSW, lpDx) == 12,
       "FIELD_OFFSET(GCP_RESULTSW, lpDx) == %ld (expected 12)",
       FIELD_OFFSET(GCP_RESULTSW, lpDx)); /* INT * */
    ok(FIELD_OFFSET(GCP_RESULTSW, lpCaretPos) == 16,
       "FIELD_OFFSET(GCP_RESULTSW, lpCaretPos) == %ld (expected 16)",
       FIELD_OFFSET(GCP_RESULTSW, lpCaretPos)); /* INT * */
    ok(FIELD_OFFSET(GCP_RESULTSW, lpClass) == 20,
       "FIELD_OFFSET(GCP_RESULTSW, lpClass) == %ld (expected 20)",
       FIELD_OFFSET(GCP_RESULTSW, lpClass)); /* LPSTR */
    ok(FIELD_OFFSET(GCP_RESULTSW, lpGlyphs) == 24,
       "FIELD_OFFSET(GCP_RESULTSW, lpGlyphs) == %ld (expected 24)",
       FIELD_OFFSET(GCP_RESULTSW, lpGlyphs)); /* LPWSTR */
    ok(FIELD_OFFSET(GCP_RESULTSW, nGlyphs) == 28,
       "FIELD_OFFSET(GCP_RESULTSW, nGlyphs) == %ld (expected 28)",
       FIELD_OFFSET(GCP_RESULTSW, nGlyphs)); /* UINT */
    ok(FIELD_OFFSET(GCP_RESULTSW, nMaxFit) == 32,
       "FIELD_OFFSET(GCP_RESULTSW, nMaxFit) == %ld (expected 32)",
       FIELD_OFFSET(GCP_RESULTSW, nMaxFit)); /* UINT */
    ok(sizeof(GCP_RESULTSW) == 36, "sizeof(GCP_RESULTSW) == %d (expected 36)", sizeof(GCP_RESULTSW));

    /* GRADIENT_RECT */
    ok(FIELD_OFFSET(GRADIENT_RECT, UpperLeft) == 0,
       "FIELD_OFFSET(GRADIENT_RECT, UpperLeft) == %ld (expected 0)",
       FIELD_OFFSET(GRADIENT_RECT, UpperLeft)); /* ULONG */
    ok(FIELD_OFFSET(GRADIENT_RECT, LowerRight) == 4,
       "FIELD_OFFSET(GRADIENT_RECT, LowerRight) == %ld (expected 4)",
       FIELD_OFFSET(GRADIENT_RECT, LowerRight)); /* ULONG */
    ok(sizeof(GRADIENT_RECT) == 8, "sizeof(GRADIENT_RECT) == %d (expected 8)", sizeof(GRADIENT_RECT));

    /* GRADIENT_TRIANGLE */
    ok(FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex1) == 0,
       "FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex1) == %ld (expected 0)",
       FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex1)); /* ULONG */
    ok(FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex2) == 4,
       "FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex2) == %ld (expected 4)",
       FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex2)); /* ULONG */
    ok(FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex3) == 8,
       "FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex3) == %ld (expected 8)",
       FIELD_OFFSET(GRADIENT_TRIANGLE, Vertex3)); /* ULONG */
    ok(sizeof(GRADIENT_TRIANGLE) == 12, "sizeof(GRADIENT_TRIANGLE) == %d (expected 12)", sizeof(GRADIENT_TRIANGLE));

    /* HANDLETABLE */
    ok(FIELD_OFFSET(HANDLETABLE, objectHandle) == 0,
       "FIELD_OFFSET(HANDLETABLE, objectHandle) == %ld (expected 0)",
       FIELD_OFFSET(HANDLETABLE, objectHandle)); /* HGDIOBJ[1] */
    ok(sizeof(HANDLETABLE) == 4, "sizeof(HANDLETABLE) == %d (expected 4)", sizeof(HANDLETABLE));

    /* KERNINGPAIR */
    ok(FIELD_OFFSET(KERNINGPAIR, wFirst) == 0,
       "FIELD_OFFSET(KERNINGPAIR, wFirst) == %ld (expected 0)",
       FIELD_OFFSET(KERNINGPAIR, wFirst)); /* WORD */
    ok(FIELD_OFFSET(KERNINGPAIR, wSecond) == 2,
       "FIELD_OFFSET(KERNINGPAIR, wSecond) == %ld (expected 2)",
       FIELD_OFFSET(KERNINGPAIR, wSecond)); /* WORD */
    ok(FIELD_OFFSET(KERNINGPAIR, iKernAmount) == 4,
       "FIELD_OFFSET(KERNINGPAIR, iKernAmount) == %ld (expected 4)",
       FIELD_OFFSET(KERNINGPAIR, iKernAmount)); /* INT */
    ok(sizeof(KERNINGPAIR) == 8, "sizeof(KERNINGPAIR) == %d (expected 8)", sizeof(KERNINGPAIR));

    /* LOCALESIGNATURE */
    ok(FIELD_OFFSET(LOCALESIGNATURE, lsUsb) == 0,
       "FIELD_OFFSET(LOCALESIGNATURE, lsUsb) == %ld (expected 0)",
       FIELD_OFFSET(LOCALESIGNATURE, lsUsb)); /* DWORD[4] */
    ok(FIELD_OFFSET(LOCALESIGNATURE, lsCsbDefault) == 16,
       "FIELD_OFFSET(LOCALESIGNATURE, lsCsbDefault) == %ld (expected 16)",
       FIELD_OFFSET(LOCALESIGNATURE, lsCsbDefault)); /* DWORD[2] */
    ok(FIELD_OFFSET(LOCALESIGNATURE, lsCsbSupported) == 24,
       "FIELD_OFFSET(LOCALESIGNATURE, lsCsbSupported) == %ld (expected 24)",
       FIELD_OFFSET(LOCALESIGNATURE, lsCsbSupported)); /* DWORD[2] */
    ok(sizeof(LOCALESIGNATURE) == 32, "sizeof(LOCALESIGNATURE) == %d (expected 32)", sizeof(LOCALESIGNATURE));

    /* LOGBRUSH */
    ok(FIELD_OFFSET(LOGBRUSH, lbStyle) == 0,
       "FIELD_OFFSET(LOGBRUSH, lbStyle) == %ld (expected 0)",
       FIELD_OFFSET(LOGBRUSH, lbStyle)); /* UINT */
    ok(FIELD_OFFSET(LOGBRUSH, lbColor) == 4,
       "FIELD_OFFSET(LOGBRUSH, lbColor) == %ld (expected 4)",
       FIELD_OFFSET(LOGBRUSH, lbColor)); /* COLORREF */
    ok(FIELD_OFFSET(LOGBRUSH, lbHatch) == 8,
       "FIELD_OFFSET(LOGBRUSH, lbHatch) == %ld (expected 8)",
       FIELD_OFFSET(LOGBRUSH, lbHatch)); /* INT */
    ok(sizeof(LOGBRUSH) == 12, "sizeof(LOGBRUSH) == %d (expected 12)", sizeof(LOGBRUSH));

    /* LOGPALETTE */
    ok(FIELD_OFFSET(LOGPALETTE, palVersion) == 0,
       "FIELD_OFFSET(LOGPALETTE, palVersion) == %ld (expected 0)",
       FIELD_OFFSET(LOGPALETTE, palVersion)); /* WORD */
    ok(FIELD_OFFSET(LOGPALETTE, palNumEntries) == 2,
       "FIELD_OFFSET(LOGPALETTE, palNumEntries) == %ld (expected 2)",
       FIELD_OFFSET(LOGPALETTE, palNumEntries)); /* WORD */
    ok(FIELD_OFFSET(LOGPALETTE, palPalEntry) == 4,
       "FIELD_OFFSET(LOGPALETTE, palPalEntry) == %ld (expected 4)",
       FIELD_OFFSET(LOGPALETTE, palPalEntry)); /* PALETTEENTRY[1] */
    ok(sizeof(LOGPALETTE) == 8, "sizeof(LOGPALETTE) == %d (expected 8)", sizeof(LOGPALETTE));

    /* LOGPEN */
    ok(FIELD_OFFSET(LOGPEN, lopnStyle) == 0,
       "FIELD_OFFSET(LOGPEN, lopnStyle) == %ld (expected 0)",
       FIELD_OFFSET(LOGPEN, lopnStyle)); /* UINT */
    ok(FIELD_OFFSET(LOGPEN, lopnWidth) == 4,
       "FIELD_OFFSET(LOGPEN, lopnWidth) == %ld (expected 4)",
       FIELD_OFFSET(LOGPEN, lopnWidth)); /* POINT */
    ok(FIELD_OFFSET(LOGPEN, lopnColor) == 12,
       "FIELD_OFFSET(LOGPEN, lopnColor) == %ld (expected 12)",
       FIELD_OFFSET(LOGPEN, lopnColor)); /* COLORREF */
    ok(sizeof(LOGPEN) == 16, "sizeof(LOGPEN) == %d (expected 16)", sizeof(LOGPEN));

    /* MAT2 */
    ok(FIELD_OFFSET(MAT2, eM11) == 0,
       "FIELD_OFFSET(MAT2, eM11) == %ld (expected 0)",
       FIELD_OFFSET(MAT2, eM11)); /* FIXED */
    ok(FIELD_OFFSET(MAT2, eM12) == 4,
       "FIELD_OFFSET(MAT2, eM12) == %ld (expected 4)",
       FIELD_OFFSET(MAT2, eM12)); /* FIXED */
    ok(FIELD_OFFSET(MAT2, eM21) == 8,
       "FIELD_OFFSET(MAT2, eM21) == %ld (expected 8)",
       FIELD_OFFSET(MAT2, eM21)); /* FIXED */
    ok(FIELD_OFFSET(MAT2, eM22) == 12,
       "FIELD_OFFSET(MAT2, eM22) == %ld (expected 12)",
       FIELD_OFFSET(MAT2, eM22)); /* FIXED */
    ok(sizeof(MAT2) == 16, "sizeof(MAT2) == %d (expected 16)", sizeof(MAT2));

    /* METAFILEPICT */
    ok(FIELD_OFFSET(METAFILEPICT, mm) == 0,
       "FIELD_OFFSET(METAFILEPICT, mm) == %ld (expected 0)",
       FIELD_OFFSET(METAFILEPICT, mm)); /* LONG */
    ok(FIELD_OFFSET(METAFILEPICT, xExt) == 4,
       "FIELD_OFFSET(METAFILEPICT, xExt) == %ld (expected 4)",
       FIELD_OFFSET(METAFILEPICT, xExt)); /* LONG */
    ok(FIELD_OFFSET(METAFILEPICT, yExt) == 8,
       "FIELD_OFFSET(METAFILEPICT, yExt) == %ld (expected 8)",
       FIELD_OFFSET(METAFILEPICT, yExt)); /* LONG */
    ok(FIELD_OFFSET(METAFILEPICT, hMF) == 12,
       "FIELD_OFFSET(METAFILEPICT, hMF) == %ld (expected 12)",
       FIELD_OFFSET(METAFILEPICT, hMF)); /* HMETAFILE */
    ok(sizeof(METAFILEPICT) == 16, "sizeof(METAFILEPICT) == %d (expected 16)", sizeof(METAFILEPICT));

    /* METAHEADER */
    ok(FIELD_OFFSET(METAHEADER, mtType) == 0,
       "FIELD_OFFSET(METAHEADER, mtType) == %ld (expected 0)",
       FIELD_OFFSET(METAHEADER, mtType)); /* WORD */
    ok(FIELD_OFFSET(METAHEADER, mtHeaderSize) == 2,
       "FIELD_OFFSET(METAHEADER, mtHeaderSize) == %ld (expected 2)",
       FIELD_OFFSET(METAHEADER, mtHeaderSize)); /* WORD */
    ok(FIELD_OFFSET(METAHEADER, mtVersion) == 4,
       "FIELD_OFFSET(METAHEADER, mtVersion) == %ld (expected 4)",
       FIELD_OFFSET(METAHEADER, mtVersion)); /* WORD */
    ok(FIELD_OFFSET(METAHEADER, mtSize) == 6,
       "FIELD_OFFSET(METAHEADER, mtSize) == %ld (expected 6)",
       FIELD_OFFSET(METAHEADER, mtSize)); /* DWORD */
    ok(FIELD_OFFSET(METAHEADER, mtNoObjects) == 10,
       "FIELD_OFFSET(METAHEADER, mtNoObjects) == %ld (expected 10)",
       FIELD_OFFSET(METAHEADER, mtNoObjects)); /* WORD */
    ok(FIELD_OFFSET(METAHEADER, mtMaxRecord) == 12,
       "FIELD_OFFSET(METAHEADER, mtMaxRecord) == %ld (expected 12)",
       FIELD_OFFSET(METAHEADER, mtMaxRecord)); /* DWORD */
    ok(FIELD_OFFSET(METAHEADER, mtNoParameters) == 16,
       "FIELD_OFFSET(METAHEADER, mtNoParameters) == %ld (expected 16)",
       FIELD_OFFSET(METAHEADER, mtNoParameters)); /* WORD */
    ok(sizeof(METAHEADER) == 18, "sizeof(METAHEADER) == %d (expected 18)", sizeof(METAHEADER));

    /* METARECORD */
    ok(FIELD_OFFSET(METARECORD, rdSize) == 0,
       "FIELD_OFFSET(METARECORD, rdSize) == %ld (expected 0)",
       FIELD_OFFSET(METARECORD, rdSize)); /* DWORD */
    ok(FIELD_OFFSET(METARECORD, rdFunction) == 4,
       "FIELD_OFFSET(METARECORD, rdFunction) == %ld (expected 4)",
       FIELD_OFFSET(METARECORD, rdFunction)); /* WORD */
    ok(FIELD_OFFSET(METARECORD, rdParm) == 6,
       "FIELD_OFFSET(METARECORD, rdParm) == %ld (expected 6)",
       FIELD_OFFSET(METARECORD, rdParm)); /* WORD[1] */
    ok(sizeof(METARECORD) == 8, "sizeof(METARECORD) == %d (expected 8)", sizeof(METARECORD));

    /* PALETTEENTRY */
    ok(FIELD_OFFSET(PALETTEENTRY, peRed) == 0,
       "FIELD_OFFSET(PALETTEENTRY, peRed) == %ld (expected 0)",
       FIELD_OFFSET(PALETTEENTRY, peRed)); /* BYTE */
    ok(sizeof(PALETTEENTRY) == 4, "sizeof(PALETTEENTRY) == %d (expected 4)", sizeof(PALETTEENTRY));

    /* PELARRAY */
    ok(FIELD_OFFSET(PELARRAY, paXCount) == 0,
       "FIELD_OFFSET(PELARRAY, paXCount) == %ld (expected 0)",
       FIELD_OFFSET(PELARRAY, paXCount)); /* LONG */
    ok(FIELD_OFFSET(PELARRAY, paYCount) == 4,
       "FIELD_OFFSET(PELARRAY, paYCount) == %ld (expected 4)",
       FIELD_OFFSET(PELARRAY, paYCount)); /* LONG */
    ok(FIELD_OFFSET(PELARRAY, paXExt) == 8,
       "FIELD_OFFSET(PELARRAY, paXExt) == %ld (expected 8)",
       FIELD_OFFSET(PELARRAY, paXExt)); /* LONG */
    ok(FIELD_OFFSET(PELARRAY, paYExt) == 12,
       "FIELD_OFFSET(PELARRAY, paYExt) == %ld (expected 12)",
       FIELD_OFFSET(PELARRAY, paYExt)); /* LONG */
    ok(FIELD_OFFSET(PELARRAY, paRGBs) == 16,
       "FIELD_OFFSET(PELARRAY, paRGBs) == %ld (expected 16)",
       FIELD_OFFSET(PELARRAY, paRGBs)); /* BYTE */
    ok(sizeof(PELARRAY) == 20, "sizeof(PELARRAY) == %d (expected 20)", sizeof(PELARRAY));

    /* PIXELFORMATDESCRIPTOR */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, nSize) == 0,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, nSize) == %ld (expected 0)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, nSize)); /* WORD */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, nVersion) == 2,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, nVersion) == %ld (expected 2)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, nVersion)); /* WORD */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwFlags) == 4,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwFlags) == %ld (expected 4)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwFlags)); /* DWORD */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, iPixelType) == 8,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, iPixelType) == %ld (expected 8)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, iPixelType)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cColorBits) == 9,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cColorBits) == %ld (expected 9)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cColorBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cRedBits) == 10,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cRedBits) == %ld (expected 10)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cRedBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cRedShift) == 11,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cRedShift) == %ld (expected 11)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cRedShift)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cGreenBits) == 12,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cGreenBits) == %ld (expected 12)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cGreenBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cGreenShift) == 13,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cGreenShift) == %ld (expected 13)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cGreenShift)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cBlueBits) == 14,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cBlueBits) == %ld (expected 14)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cBlueBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cBlueShift) == 15,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cBlueShift) == %ld (expected 15)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cBlueShift)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAlphaBits) == 16,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAlphaBits) == %ld (expected 16)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAlphaBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAlphaShift) == 17,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAlphaShift) == %ld (expected 17)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAlphaShift)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumBits) == 18,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumBits) == %ld (expected 18)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumRedBits) == 19,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumRedBits) == %ld (expected 19)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumRedBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumGreenBits) == 20,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumGreenBits) == %ld (expected 20)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumGreenBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumBlueBits) == 21,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumBlueBits) == %ld (expected 21)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumBlueBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumAlphaBits) == 22,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumAlphaBits) == %ld (expected 22)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAccumAlphaBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cDepthBits) == 23,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cDepthBits) == %ld (expected 23)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cDepthBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cStencilBits) == 24,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cStencilBits) == %ld (expected 24)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cStencilBits)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAuxBuffers) == 25,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAuxBuffers) == %ld (expected 25)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, cAuxBuffers)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, iLayerType) == 26,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, iLayerType) == %ld (expected 26)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, iLayerType)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, bReserved) == 27,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, bReserved) == %ld (expected 27)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, bReserved)); /* BYTE */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwLayerMask) == 28,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwLayerMask) == %ld (expected 28)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwLayerMask)); /* DWORD */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwVisibleMask) == 32,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwVisibleMask) == %ld (expected 32)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwVisibleMask)); /* DWORD */
    ok(FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwDamageMask) == 36,
       "FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwDamageMask) == %ld (expected 36)",
       FIELD_OFFSET(PIXELFORMATDESCRIPTOR, dwDamageMask)); /* DWORD */
    ok(sizeof(PIXELFORMATDESCRIPTOR) == 40, "sizeof(PIXELFORMATDESCRIPTOR) == %d (expected 40)", sizeof(PIXELFORMATDESCRIPTOR));

    /* POINTFX */
    ok(FIELD_OFFSET(POINTFX, x) == 0,
       "FIELD_OFFSET(POINTFX, x) == %ld (expected 0)",
       FIELD_OFFSET(POINTFX, x)); /* FIXED */
    ok(FIELD_OFFSET(POINTFX, y) == 4,
       "FIELD_OFFSET(POINTFX, y) == %ld (expected 4)",
       FIELD_OFFSET(POINTFX, y)); /* FIXED */
    ok(sizeof(POINTFX) == 8, "sizeof(POINTFX) == %d (expected 8)", sizeof(POINTFX));

    /* RGBQUAD */
    ok(FIELD_OFFSET(RGBQUAD, rgbBlue) == 0,
       "FIELD_OFFSET(RGBQUAD, rgbBlue) == %ld (expected 0)",
       FIELD_OFFSET(RGBQUAD, rgbBlue)); /* BYTE */
    ok(FIELD_OFFSET(RGBQUAD, rgbGreen) == 1,
       "FIELD_OFFSET(RGBQUAD, rgbGreen) == %ld (expected 1)",
       FIELD_OFFSET(RGBQUAD, rgbGreen)); /* BYTE */
    ok(FIELD_OFFSET(RGBQUAD, rgbRed) == 2,
       "FIELD_OFFSET(RGBQUAD, rgbRed) == %ld (expected 2)",
       FIELD_OFFSET(RGBQUAD, rgbRed)); /* BYTE */
    ok(FIELD_OFFSET(RGBQUAD, rgbReserved) == 3,
       "FIELD_OFFSET(RGBQUAD, rgbReserved) == %ld (expected 3)",
       FIELD_OFFSET(RGBQUAD, rgbReserved)); /* BYTE */
    ok(sizeof(RGBQUAD) == 4, "sizeof(RGBQUAD) == %d (expected 4)", sizeof(RGBQUAD));

    /* TEXTMETRICA */
    ok(FIELD_OFFSET(TEXTMETRICA, tmHeight) == 0,
       "FIELD_OFFSET(TEXTMETRICA, tmHeight) == %ld (expected 0)",
       FIELD_OFFSET(TEXTMETRICA, tmHeight)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmAscent) == 4,
       "FIELD_OFFSET(TEXTMETRICA, tmAscent) == %ld (expected 4)",
       FIELD_OFFSET(TEXTMETRICA, tmAscent)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmDescent) == 8,
       "FIELD_OFFSET(TEXTMETRICA, tmDescent) == %ld (expected 8)",
       FIELD_OFFSET(TEXTMETRICA, tmDescent)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmInternalLeading) == 12,
       "FIELD_OFFSET(TEXTMETRICA, tmInternalLeading) == %ld (expected 12)",
       FIELD_OFFSET(TEXTMETRICA, tmInternalLeading)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmExternalLeading) == 16,
       "FIELD_OFFSET(TEXTMETRICA, tmExternalLeading) == %ld (expected 16)",
       FIELD_OFFSET(TEXTMETRICA, tmExternalLeading)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmAveCharWidth) == 20,
       "FIELD_OFFSET(TEXTMETRICA, tmAveCharWidth) == %ld (expected 20)",
       FIELD_OFFSET(TEXTMETRICA, tmAveCharWidth)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmMaxCharWidth) == 24,
       "FIELD_OFFSET(TEXTMETRICA, tmMaxCharWidth) == %ld (expected 24)",
       FIELD_OFFSET(TEXTMETRICA, tmMaxCharWidth)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmWeight) == 28,
       "FIELD_OFFSET(TEXTMETRICA, tmWeight) == %ld (expected 28)",
       FIELD_OFFSET(TEXTMETRICA, tmWeight)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmOverhang) == 32,
       "FIELD_OFFSET(TEXTMETRICA, tmOverhang) == %ld (expected 32)",
       FIELD_OFFSET(TEXTMETRICA, tmOverhang)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmDigitizedAspectX) == 36,
       "FIELD_OFFSET(TEXTMETRICA, tmDigitizedAspectX) == %ld (expected 36)",
       FIELD_OFFSET(TEXTMETRICA, tmDigitizedAspectX)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmDigitizedAspectY) == 40,
       "FIELD_OFFSET(TEXTMETRICA, tmDigitizedAspectY) == %ld (expected 40)",
       FIELD_OFFSET(TEXTMETRICA, tmDigitizedAspectY)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICA, tmFirstChar) == 44,
       "FIELD_OFFSET(TEXTMETRICA, tmFirstChar) == %ld (expected 44)",
       FIELD_OFFSET(TEXTMETRICA, tmFirstChar)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICA, tmLastChar) == 45,
       "FIELD_OFFSET(TEXTMETRICA, tmLastChar) == %ld (expected 45)",
       FIELD_OFFSET(TEXTMETRICA, tmLastChar)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICA, tmDefaultChar) == 46,
       "FIELD_OFFSET(TEXTMETRICA, tmDefaultChar) == %ld (expected 46)",
       FIELD_OFFSET(TEXTMETRICA, tmDefaultChar)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICA, tmBreakChar) == 47,
       "FIELD_OFFSET(TEXTMETRICA, tmBreakChar) == %ld (expected 47)",
       FIELD_OFFSET(TEXTMETRICA, tmBreakChar)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICA, tmItalic) == 48,
       "FIELD_OFFSET(TEXTMETRICA, tmItalic) == %ld (expected 48)",
       FIELD_OFFSET(TEXTMETRICA, tmItalic)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICA, tmUnderlined) == 49,
       "FIELD_OFFSET(TEXTMETRICA, tmUnderlined) == %ld (expected 49)",
       FIELD_OFFSET(TEXTMETRICA, tmUnderlined)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICA, tmStruckOut) == 50,
       "FIELD_OFFSET(TEXTMETRICA, tmStruckOut) == %ld (expected 50)",
       FIELD_OFFSET(TEXTMETRICA, tmStruckOut)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICA, tmPitchAndFamily) == 51,
       "FIELD_OFFSET(TEXTMETRICA, tmPitchAndFamily) == %ld (expected 51)",
       FIELD_OFFSET(TEXTMETRICA, tmPitchAndFamily)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICA, tmCharSet) == 52,
       "FIELD_OFFSET(TEXTMETRICA, tmCharSet) == %ld (expected 52)",
       FIELD_OFFSET(TEXTMETRICA, tmCharSet)); /* BYTE */
    ok(sizeof(TEXTMETRICA) == 56, "sizeof(TEXTMETRICA) == %d (expected 56)", sizeof(TEXTMETRICA));

    /* TEXTMETRICW */
    ok(FIELD_OFFSET(TEXTMETRICW, tmHeight) == 0,
       "FIELD_OFFSET(TEXTMETRICW, tmHeight) == %ld (expected 0)",
       FIELD_OFFSET(TEXTMETRICW, tmHeight)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmAscent) == 4,
       "FIELD_OFFSET(TEXTMETRICW, tmAscent) == %ld (expected 4)",
       FIELD_OFFSET(TEXTMETRICW, tmAscent)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmDescent) == 8,
       "FIELD_OFFSET(TEXTMETRICW, tmDescent) == %ld (expected 8)",
       FIELD_OFFSET(TEXTMETRICW, tmDescent)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmInternalLeading) == 12,
       "FIELD_OFFSET(TEXTMETRICW, tmInternalLeading) == %ld (expected 12)",
       FIELD_OFFSET(TEXTMETRICW, tmInternalLeading)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmExternalLeading) == 16,
       "FIELD_OFFSET(TEXTMETRICW, tmExternalLeading) == %ld (expected 16)",
       FIELD_OFFSET(TEXTMETRICW, tmExternalLeading)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmAveCharWidth) == 20,
       "FIELD_OFFSET(TEXTMETRICW, tmAveCharWidth) == %ld (expected 20)",
       FIELD_OFFSET(TEXTMETRICW, tmAveCharWidth)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmMaxCharWidth) == 24,
       "FIELD_OFFSET(TEXTMETRICW, tmMaxCharWidth) == %ld (expected 24)",
       FIELD_OFFSET(TEXTMETRICW, tmMaxCharWidth)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmWeight) == 28,
       "FIELD_OFFSET(TEXTMETRICW, tmWeight) == %ld (expected 28)",
       FIELD_OFFSET(TEXTMETRICW, tmWeight)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmOverhang) == 32,
       "FIELD_OFFSET(TEXTMETRICW, tmOverhang) == %ld (expected 32)",
       FIELD_OFFSET(TEXTMETRICW, tmOverhang)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmDigitizedAspectX) == 36,
       "FIELD_OFFSET(TEXTMETRICW, tmDigitizedAspectX) == %ld (expected 36)",
       FIELD_OFFSET(TEXTMETRICW, tmDigitizedAspectX)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmDigitizedAspectY) == 40,
       "FIELD_OFFSET(TEXTMETRICW, tmDigitizedAspectY) == %ld (expected 40)",
       FIELD_OFFSET(TEXTMETRICW, tmDigitizedAspectY)); /* LONG */
    ok(FIELD_OFFSET(TEXTMETRICW, tmFirstChar) == 44,
       "FIELD_OFFSET(TEXTMETRICW, tmFirstChar) == %ld (expected 44)",
       FIELD_OFFSET(TEXTMETRICW, tmFirstChar)); /* WCHAR */
    ok(FIELD_OFFSET(TEXTMETRICW, tmLastChar) == 46,
       "FIELD_OFFSET(TEXTMETRICW, tmLastChar) == %ld (expected 46)",
       FIELD_OFFSET(TEXTMETRICW, tmLastChar)); /* WCHAR */
    ok(FIELD_OFFSET(TEXTMETRICW, tmDefaultChar) == 48,
       "FIELD_OFFSET(TEXTMETRICW, tmDefaultChar) == %ld (expected 48)",
       FIELD_OFFSET(TEXTMETRICW, tmDefaultChar)); /* WCHAR */
    ok(FIELD_OFFSET(TEXTMETRICW, tmBreakChar) == 50,
       "FIELD_OFFSET(TEXTMETRICW, tmBreakChar) == %ld (expected 50)",
       FIELD_OFFSET(TEXTMETRICW, tmBreakChar)); /* WCHAR */
    ok(FIELD_OFFSET(TEXTMETRICW, tmItalic) == 52,
       "FIELD_OFFSET(TEXTMETRICW, tmItalic) == %ld (expected 52)",
       FIELD_OFFSET(TEXTMETRICW, tmItalic)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICW, tmUnderlined) == 53,
       "FIELD_OFFSET(TEXTMETRICW, tmUnderlined) == %ld (expected 53)",
       FIELD_OFFSET(TEXTMETRICW, tmUnderlined)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICW, tmStruckOut) == 54,
       "FIELD_OFFSET(TEXTMETRICW, tmStruckOut) == %ld (expected 54)",
       FIELD_OFFSET(TEXTMETRICW, tmStruckOut)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICW, tmPitchAndFamily) == 55,
       "FIELD_OFFSET(TEXTMETRICW, tmPitchAndFamily) == %ld (expected 55)",
       FIELD_OFFSET(TEXTMETRICW, tmPitchAndFamily)); /* BYTE */
    ok(FIELD_OFFSET(TEXTMETRICW, tmCharSet) == 56,
       "FIELD_OFFSET(TEXTMETRICW, tmCharSet) == %ld (expected 56)",
       FIELD_OFFSET(TEXTMETRICW, tmCharSet)); /* BYTE */
    ok(sizeof(TEXTMETRICW) == 60, "sizeof(TEXTMETRICW) == %d (expected 60)", sizeof(TEXTMETRICW));

    /* XFORM */
    ok(FIELD_OFFSET(XFORM, eM11) == 0,
       "FIELD_OFFSET(XFORM, eM11) == %ld (expected 0)",
       FIELD_OFFSET(XFORM, eM11)); /* FLOAT */
    ok(FIELD_OFFSET(XFORM, eM12) == 4,
       "FIELD_OFFSET(XFORM, eM12) == %ld (expected 4)",
       FIELD_OFFSET(XFORM, eM12)); /* FLOAT */
    ok(FIELD_OFFSET(XFORM, eM21) == 8,
       "FIELD_OFFSET(XFORM, eM21) == %ld (expected 8)",
       FIELD_OFFSET(XFORM, eM21)); /* FLOAT */
    ok(FIELD_OFFSET(XFORM, eM22) == 12,
       "FIELD_OFFSET(XFORM, eM22) == %ld (expected 12)",
       FIELD_OFFSET(XFORM, eM22)); /* FLOAT */
    ok(FIELD_OFFSET(XFORM, eDx) == 16,
       "FIELD_OFFSET(XFORM, eDx) == %ld (expected 16)",
       FIELD_OFFSET(XFORM, eDx)); /* FLOAT */
    ok(FIELD_OFFSET(XFORM, eDy) == 20,
       "FIELD_OFFSET(XFORM, eDy) == %ld (expected 20)",
       FIELD_OFFSET(XFORM, eDy)); /* FLOAT */
    ok(sizeof(XFORM) == 24, "sizeof(XFORM) == %d (expected 24)", sizeof(XFORM));

}

START_TEST(generated)
{
    test_pack();
}
