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

#ifndef _FLATAPI_H
#define _FLATAPI_H

#define WINGDIPAPI __stdcall

#define GDIPCONST const

#ifdef __cplusplus
extern "C" {
#endif

GpStatus WINGDIPAPI GdipClonePen(GpPen*,GpPen**);
GpStatus WINGDIPAPI GdipCreatePen1(ARGB,REAL,GpUnit,GpPen**);
GpStatus WINGDIPAPI GdipDeletePen(GpPen*);
GpStatus WINGDIPAPI GdipGetPenBrushFill(GpPen*,GpBrush**);
GpStatus WINGDIPAPI GdipGetPenColor(GpPen*,ARGB*);
GpStatus WINGDIPAPI GdipGetPenDashArray(GpPen*,REAL*,INT);
GpStatus WINGDIPAPI GdipGetPenDashStyle(GpPen*,GpDashStyle*);
GpStatus WINGDIPAPI GdipSetPenBrushFill(GpPen*,GpBrush*);
GpStatus WINGDIPAPI GdipSetPenColor(GpPen*,ARGB);
GpStatus WINGDIPAPI GdipSetPenCustomEndCap(GpPen*,GpCustomLineCap*);
GpStatus WINGDIPAPI GdipSetPenCustomStartCap(GpPen*,GpCustomLineCap*);
GpStatus WINGDIPAPI GdipSetPenDashArray(GpPen*,GDIPCONST REAL*,INT);
GpStatus WINGDIPAPI GdipSetPenDashStyle(GpPen*,GpDashStyle);
GpStatus WINGDIPAPI GdipSetPenEndCap(GpPen*,GpLineCap);
GpStatus WINGDIPAPI GdipSetPenLineCap197819(GpPen*,GpLineCap,GpLineCap,GpDashCap);
GpStatus WINGDIPAPI GdipSetPenLineJoin(GpPen*,GpLineJoin);
GpStatus WINGDIPAPI GdipSetPenMiterLimit(GpPen*,REAL);
GpStatus WINGDIPAPI GdipSetPenStartCap(GpPen*,GpLineCap);
GpStatus WINGDIPAPI GdipSetPenWidth(GpPen*,REAL);

GpStatus WINGDIPAPI GdipCreateFromHDC(HDC,GpGraphics**);
GpStatus WINGDIPAPI GdipCreateFromHWND(HWND,GpGraphics**);
GpStatus WINGDIPAPI GdipCreateMetafileFromEmf(HENHMETAFILE,BOOL,GpMetafile**);
GpStatus WINGDIPAPI GdipCreateMetafileFromWmf(HMETAFILE,BOOL,
    GDIPCONST WmfPlaceableFileHeader*,GpMetafile**);
GpStatus WINGDIPAPI GdipDeleteGraphics(GpGraphics *);
GpStatus WINGDIPAPI GdipDrawArc(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawBezier(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,
    REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawCurve2(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT,REAL);
GpStatus WINGDIPAPI GdipDrawImagePointsRect(GpGraphics*,GpImage*,
    GDIPCONST GpPointF*,INT,REAL,REAL,REAL,REAL,GpUnit,
    GDIPCONST GpImageAttributes*,DrawImageAbort,VOID*);
GpStatus WINGDIPAPI GdipDrawLine(GpGraphics *graphics,GpPen*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawLineI(GpGraphics*,GpPen*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipDrawLines(GpGraphics*,GpPen*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipDrawPath(GpGraphics*,GpPen*,GpPath*);
GpStatus WINGDIPAPI GdipDrawPie(GpGraphics*,GpPen*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipDrawRectangleI(GpGraphics*,GpPen*,INT,INT,INT,INT);
GpStatus WINGDIPAPI GdipFillPath(GpGraphics*,GpBrush*,GpPath*);
GpStatus WINGDIPAPI GdipFillPie(GpGraphics*,GpBrush*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipFillPolygonI(GpGraphics*,GpBrush*,GDIPCONST GpPoint*,INT,
    GpFillMode);
GpStatus WINGDIPAPI GdipGetCompositingQuality(GpGraphics*,CompositingQuality*);
GpStatus WINGDIPAPI GdipGetInterpolationMode(GpGraphics*,InterpolationMode*);
GpStatus WINGDIPAPI GdipGetPageScale(GpGraphics*,REAL*);
GpStatus WINGDIPAPI GdipGetPageUnit(GpGraphics*,GpUnit*);
GpStatus WINGDIPAPI GdipGetPixelOffsetMode(GpGraphics*,PixelOffsetMode*);
GpStatus WINGDIPAPI GdipGetSmoothingMode(GpGraphics*,SmoothingMode*);
GpStatus WINGDIPAPI GdipGetWorldTransform(GpGraphics*,GpMatrix*);
GpStatus WINGDIPAPI GdipRestoreGraphics(GpGraphics*,GraphicsState);
GpStatus WINGDIPAPI GdipSaveGraphics(GpGraphics*,GraphicsState*);
GpStatus WINGDIPAPI GdipSetCompositingQuality(GpGraphics*,CompositingQuality);
GpStatus WINGDIPAPI GdipSetInterpolationMode(GpGraphics*,InterpolationMode);
GpStatus WINGDIPAPI GdipSetPageScale(GpGraphics*,REAL);
GpStatus WINGDIPAPI GdipSetPageUnit(GpGraphics*,GpUnit);
GpStatus WINGDIPAPI GdipSetPixelOffsetMode(GpGraphics*,PixelOffsetMode);
GpStatus WINGDIPAPI GdipSetSmoothingMode(GpGraphics*,SmoothingMode);
GpStatus WINGDIPAPI GdipSetWorldTransform(GpGraphics*,GpMatrix*);

GpStatus WINGDIPAPI GdipCloneBrush(GpBrush*,GpBrush**);
GpStatus WINGDIPAPI GdipCreatePathGradient(GDIPCONST GpPointF*,INT,GpWrapMode,
    GpPathGradient**);
GpStatus WINGDIPAPI GdipCreatePathGradientFromPath(GDIPCONST GpPath*,
    GpPathGradient**);
GpStatus WINGDIPAPI GdipCreateSolidFill(ARGB,GpSolidFill**);
GpStatus WINGDIPAPI GdipDeleteBrush(GpBrush*);
GpStatus WINGDIPAPI GdipGetBrushType(GpBrush*,GpBrushType*);
GpStatus WINGDIPAPI GdipGetPathGradientCenterPoint(GpPathGradient*,GpPointF*);
GpStatus WINGDIPAPI GdipGetPathGradientFocusScales(GpPathGradient*,REAL*,REAL*);
GpStatus WINGDIPAPI GdipGetPathGradientGammaCorrection(GpPathGradient*,BOOL*);
GpStatus WINGDIPAPI GdipGetPathGradientPointCount(GpPathGradient*,INT*);
GpStatus WINGDIPAPI GdipGetPathGradientSurroundColorsWithCount(GpPathGradient*,
    ARGB*,INT*);
GpStatus WINGDIPAPI GdipGetSolidFillColor(GpSolidFill*,ARGB*);
GpStatus WINGDIPAPI GdipSetPathGradientCenterColor(GpPathGradient*,ARGB);
GpStatus WINGDIPAPI GdipSetPathGradientCenterPoint(GpPathGradient*,GpPointF*);
GpStatus WINGDIPAPI GdipSetPathGradientFocusScales(GpPathGradient*,REAL,REAL);
GpStatus WINGDIPAPI GdipSetPathGradientGammaCorrection(GpPathGradient*,BOOL);
GpStatus WINGDIPAPI GdipSetPathGradientSigmaBlend(GpPathGradient*,REAL,REAL);
GpStatus WINGDIPAPI GdipSetPathGradientSurroundColorsWithCount(GpPathGradient*,
    ARGB*,INT*);
GpStatus WINGDIPAPI GdipSetPathGradientWrapMode(GpPathGradient*,GpWrapMode);
GpStatus WINGDIPAPI GdipSetSolidFillColor(GpSolidFill*,ARGB);

GpStatus WINGDIPAPI GdipAddPathArc(GpPath*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathBeziers(GpPath*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipAddPathEllipse(GpPath*,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipAddPathLine2(GpPath*,GDIPCONST GpPointF*,INT);
GpStatus WINGDIPAPI GdipAddPathPath(GpPath*,GDIPCONST GpPath*,BOOL);
GpStatus WINGDIPAPI GdipClonePath(GpPath*,GpPath**);
GpStatus WINGDIPAPI GdipClosePathFigure(GpPath*);
GpStatus WINGDIPAPI GdipClosePathFigures(GpPath*);
GpStatus WINGDIPAPI GdipCreatePath(GpFillMode,GpPath**);
GpStatus WINGDIPAPI GdipDeletePath(GpPath*);
GpStatus WINGDIPAPI GdipFillPolygon(GpGraphics*,GpBrush*,GDIPCONST GpPointF*,
    INT,GpFillMode);
GpStatus WINGDIPAPI GdipGetPathFillMode(GpPath*,GpFillMode*);
GpStatus WINGDIPAPI GdipGetPathPoints(GpPath*,GpPointF*,INT);
GpStatus WINGDIPAPI GdipGetPathTypes(GpPath*,BYTE*,INT);
GpStatus WINGDIPAPI GdipGetPathWorldBounds(GpPath*,GpRectF*,GDIPCONST GpMatrix*,
    GDIPCONST GpPen*);
GpStatus WINGDIPAPI GdipGetPointCount(GpPath*,INT*);
GpStatus WINGDIPAPI GdipIsOutlineVisiblePathPointI(GpPath*,INT,INT,GpPen*,
    GpGraphics*,BOOL*);
GpStatus WINGDIPAPI GdipResetPath(GpPath*);
GpStatus WINGDIPAPI GdipSetPathFillMode(GpPath*,GpFillMode);
GpStatus WINGDIPAPI GdipStartPathFigure(GpPath*);
GpStatus WINGDIPAPI GdipTransformPath(GpPath*,GpMatrix*);

GpStatus WINGDIPAPI GdipCloneMatrix(GpMatrix*,GpMatrix**);
GpStatus WINGDIPAPI GdipCreateMatrix(GpMatrix**);
GpStatus WINGDIPAPI GdipCreateMatrix2(REAL,REAL,REAL,REAL,REAL,REAL,GpMatrix**);
GpStatus WINGDIPAPI GdipCreateMatrix3(GDIPCONST GpRectF *,GDIPCONST GpPointF*,
    GpMatrix**);
GpStatus WINGDIPAPI GdipDeleteMatrix(GpMatrix*);
GpStatus WINGDIPAPI GdipGetMatrixElements(GDIPCONST GpMatrix*,REAL*);
GpStatus WINGDIPAPI GdipMultiplyMatrix(GpMatrix*,GpMatrix*,GpMatrixOrder);
GpStatus WINGDIPAPI GdipRotateMatrix(GpMatrix*,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipScaleMatrix(GpMatrix*,REAL,REAL,GpMatrixOrder);
GpStatus WINGDIPAPI GdipSetMatrixElements(GpMatrix*,REAL,REAL,REAL,REAL,REAL,REAL);
GpStatus WINGDIPAPI GdipTransformMatrixPoints(GpMatrix*,GpPointF*,INT);
GpStatus WINGDIPAPI GdipTranslateMatrix(GpMatrix*,REAL,REAL,GpMatrixOrder);

GpStatus WINGDIPAPI GdipCreatePathIter(GpPathIterator**,GpPath*);
GpStatus WINGDIPAPI GdipDeletePathIter(GpPathIterator*);
GpStatus WINGDIPAPI GdipPathIterCopyData(GpPathIterator*,INT*,GpPointF*,BYTE*,
    INT,INT);
GpStatus WINGDIPAPI GdipPathIterNextSubpath(GpPathIterator*,INT*,INT*,INT*,BOOL*);
GpStatus WINGDIPAPI GdipPathIterRewind(GpPathIterator*);

GpStatus WINGDIPAPI GdipCloneCustomLineCap(GpCustomLineCap*,GpCustomLineCap**);
GpStatus WINGDIPAPI GdipCreateCustomLineCap(GpPath*,GpPath*,GpLineCap,REAL,
    GpCustomLineCap**);
GpStatus WINGDIPAPI GdipDeleteCustomLineCap(GpCustomLineCap*);

GpStatus WINGDIPAPI GdipBitmapGetPixel(GpBitmap*,INT,INT,ARGB*);
GpStatus WINGDIPAPI GdipCreateBitmapFromScan0(INT,INT,INT,PixelFormat,BYTE*,
    GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromStream(IStream*,GpBitmap**);
GpStatus WINGDIPAPI GdipCreateBitmapFromStreamICM(IStream*,GpBitmap**);
GpStatus WINGDIPAPI GdipDisposeImage(GpImage*);
GpStatus WINGDIPAPI GdipGetImageBounds(GpImage*,GpRectF*,GpUnit*);
GpStatus WINGDIPAPI GdipGetImageHeight(GpImage*,UINT*);
GpStatus WINGDIPAPI GdipGetImageHorizontalResolution(GpImage*,REAL*);
GpStatus WINGDIPAPI GdipGetImagePixelFormat(GpImage*,PixelFormat*);
GpStatus WINGDIPAPI GdipGetImageRawFormat(GpImage*,GUID*);
GpStatus WINGDIPAPI GdipGetImageType(GpImage*,ImageType*);
GpStatus WINGDIPAPI GdipGetImageVerticalResolution(GpImage*,REAL*);
GpStatus WINGDIPAPI GdipGetImageWidth(GpImage*,UINT*);
GpStatus WINGDIPAPI GdipGetMetafileHeaderFromMetafile(GpMetafile*,MetafileHeader*);
GpStatus WINGDIPAPI GdipGetPropertyItemSize(GpImage*,PROPID,UINT*);
GpStatus WINGDIPAPI GdipImageGetFrameCount(GpImage*,GDIPCONST GUID*,UINT*);
GpStatus WINGDIPAPI GdipImageGetFrameDimensionsList(GpImage*,GUID*,UINT);
GpStatus WINGDIPAPI GdipLoadImageFromStream(IStream*,GpImage**);
GpStatus WINGDIPAPI GdipLoadImageFromStreamICM(IStream*,GpImage**);
GpStatus WINGDIPAPI GdipRemovePropertyItem(GpImage*,PROPID);
GpStatus WINGDIPAPI GdipSaveImageToStream(GpImage*,IStream*,
    GDIPCONST CLSID*,GDIPCONST EncoderParameters*);

GpStatus WINGDIPAPI GdipCreateImageAttributes(GpImageAttributes**);
GpStatus WINGDIPAPI GdipDisposeImageAttributes(GpImageAttributes*);
GpStatus WINGDIPAPI GdipSetImageAttributesColorKeys(GpImageAttributes*,
    ColorAdjustType,BOOL,ARGB,ARGB);
GpStatus WINGDIPAPI GdipSetImageAttributesWrapMode(GpImageAttributes*,WrapMode,
    ARGB,BOOL);

#ifdef __cplusplus
}
#endif

#endif
