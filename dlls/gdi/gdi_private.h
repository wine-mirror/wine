/*
 * GDI definitions
 *
 * Copyright 1993 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_GDI_PRIVATE_H
#define __WINE_GDI_PRIVATE_H

/* Metafile defines */
#define META_EOF 0x0000
/* values of mtType in METAHEADER.  Note however that the disk image of a disk
   based metafile has mtType == 1 */
#define METAFILE_MEMORY 1
#define METAFILE_DISK   2

struct gdi_obj_funcs
{
    HGDIOBJ (*pSelectObject)( HGDIOBJ handle, void *obj, HDC hdc );
    INT     (*pGetObject16)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    INT     (*pGetObjectA)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    INT     (*pGetObjectW)( HGDIOBJ handle, void *obj, INT count, LPVOID buffer );
    BOOL    (*pUnrealizeObject)( HGDIOBJ handle, void *obj );
    BOOL    (*pDeleteObject)( HGDIOBJ handle, void *obj );
};

struct hdc_list
{
    HDC hdc;
    struct hdc_list *next;
};

/* Device functions for the Wine driver interface */

typedef struct tagDC_FUNCS
{
    INT      (*pAbortDoc)(PHYSDEV);
    BOOL     (*pAbortPath)(PHYSDEV);
    BOOL     (*pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    BOOL     (*pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pBeginPath)(PHYSDEV);
    BOOL     (*pBitBlt)(PHYSDEV,INT,INT,INT,INT,PHYSDEV,INT,INT,DWORD);
    INT      (*pChoosePixelFormat)(PHYSDEV,const PIXELFORMATDESCRIPTOR *);
    BOOL     (*pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pCloseFigure)(PHYSDEV);
    BOOL     (*pCreateBitmap)(PHYSDEV,HBITMAP);
    BOOL     (*pCreateDC)(DC *,PHYSDEV *,LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
    HBITMAP  (*pCreateDIBSection)(PHYSDEV,BITMAPINFO *,UINT,LPVOID *,HANDLE,DWORD,DWORD);
    BOOL     (*pDeleteBitmap)(HBITMAP);
    BOOL     (*pDeleteDC)(PHYSDEV);
    BOOL     (*pDeleteObject)(PHYSDEV,HGDIOBJ);
    INT      (*pDescribePixelFormat)(PHYSDEV,INT,UINT,PIXELFORMATDESCRIPTOR *);
    DWORD    (*pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    BOOL     (*pEllipse)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pEndDoc)(PHYSDEV);
    INT      (*pEndPage)(PHYSDEV);
    BOOL     (*pEndPath)(PHYSDEV);
    BOOL     (*pEnumDeviceFonts)(PHYSDEV,LPLOGFONTW,FONTENUMPROCW,LPARAM);
    INT      (*pExcludeClipRect)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);
    INT      (*pExtEscape)(PHYSDEV,INT,INT,LPCVOID,INT,LPVOID);
    BOOL     (*pExtFloodFill)(PHYSDEV,INT,INT,COLORREF,UINT);
    INT      (*pExtSelectClipRgn)(PHYSDEV,HRGN,INT);
    BOOL     (*pExtTextOut)(PHYSDEV,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
    BOOL     (*pFillPath)(PHYSDEV);
    BOOL     (*pFillRgn)(PHYSDEV,HRGN,HBRUSH);
    BOOL     (*pFlattenPath)(PHYSDEV);
    BOOL     (*pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    BOOL     (*pGdiComment)(PHYSDEV,UINT,CONST BYTE*);
    LONG     (*pGetBitmapBits)(HBITMAP,void*,LONG);
    BOOL     (*pGetCharWidth)(PHYSDEV,UINT,UINT,LPINT);
    BOOL     (*pGetDCOrgEx)(PHYSDEV,LPPOINT);
    UINT     (*pGetDIBColorTable)(PHYSDEV,UINT,UINT,RGBQUAD*);
    INT      (*pGetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPVOID,BITMAPINFO*,UINT);
    INT      (*pGetDeviceCaps)(PHYSDEV,INT);
    BOOL     (*pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    COLORREF (*pGetNearestColor)(PHYSDEV,COLORREF);
    COLORREF (*pGetPixel)(PHYSDEV,INT,INT);
    INT      (*pGetPixelFormat)(PHYSDEV);
    UINT     (*pGetSystemPaletteEntries)(PHYSDEV,UINT,UINT,LPPALETTEENTRY);
    BOOL     (*pGetTextExtentPoint)(PHYSDEV,LPCWSTR,INT,LPSIZE);
    BOOL     (*pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    INT      (*pIntersectClipRect)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (*pInvertRgn)(PHYSDEV,HRGN);
    BOOL     (*pLineTo)(PHYSDEV,INT,INT);
    BOOL     (*pModifyWorldTransform)(PHYSDEV,const XFORM*,INT);
    BOOL     (*pMoveTo)(PHYSDEV,INT,INT);
    INT      (*pOffsetClipRgn)(PHYSDEV,INT,INT);
    INT      (*pOffsetViewportOrg)(PHYSDEV,INT,INT);
    INT      (*pOffsetWindowOrg)(PHYSDEV,INT,INT);
    BOOL     (*pPaintRgn)(PHYSDEV,HRGN);
    BOOL     (*pPatBlt)(PHYSDEV,INT,INT,INT,INT,DWORD);
    BOOL     (*pPie)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pPolyBezier)(PHYSDEV,const POINT*,DWORD);
    BOOL     (*pPolyBezierTo)(PHYSDEV,const POINT*,DWORD);
    BOOL     (*pPolyDraw)(PHYSDEV,const POINT*,const BYTE *,DWORD);
    BOOL     (*pPolyPolygon)(PHYSDEV,const POINT*,const INT*,UINT);
    BOOL     (*pPolyPolyline)(PHYSDEV,const POINT*,const DWORD*,DWORD);
    BOOL     (*pPolygon)(PHYSDEV,const POINT*,INT);
    BOOL     (*pPolyline)(PHYSDEV,const POINT*,INT);
    BOOL     (*pPolylineTo)(PHYSDEV,const POINT*,INT);
    UINT     (*pRealizeDefaultPalette)(PHYSDEV);
    UINT     (*pRealizePalette)(PHYSDEV,HPALETTE,BOOL);
    BOOL     (*pRectangle)(PHYSDEV,INT,INT,INT,INT);
    HDC      (*pResetDC)(PHYSDEV,const DEVMODEW*);
    BOOL     (*pRestoreDC)(PHYSDEV,INT);
    BOOL     (*pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    INT      (*pSaveDC)(PHYSDEV);
    INT      (*pScaleViewportExt)(PHYSDEV,INT,INT,INT,INT);
    INT      (*pScaleWindowExt)(PHYSDEV,INT,INT,INT,INT);
    HBITMAP  (*pSelectBitmap)(PHYSDEV,HBITMAP);
    HBRUSH   (*pSelectBrush)(PHYSDEV,HBRUSH);
    BOOL     (*pSelectClipPath)(PHYSDEV,INT);
    HFONT    (*pSelectFont)(PHYSDEV,HFONT);
    HPALETTE (*pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    HPEN     (*pSelectPen)(PHYSDEV,HPEN);
    INT      (*pSetArcDirection)(PHYSDEV,INT);
    LONG     (*pSetBitmapBits)(HBITMAP,const void*,LONG);
    COLORREF (*pSetBkColor)(PHYSDEV,COLORREF);
    INT      (*pSetBkMode)(PHYSDEV,INT);
    COLORREF (*pSetDCBrushColor)(PHYSDEV, COLORREF);
    DWORD    (*pSetDCOrg)(PHYSDEV,INT,INT);
    COLORREF (*pSetDCPenColor)(PHYSDEV, COLORREF);
    UINT     (*pSetDIBColorTable)(PHYSDEV,UINT,UINT,const RGBQUAD*);
    INT      (*pSetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    INT      (*pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,
                                   const BITMAPINFO*,UINT);
    VOID     (*pSetDeviceClipping)(PHYSDEV,HRGN);
    BOOL     (*pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    INT      (*pSetMapMode)(PHYSDEV,INT);
    DWORD    (*pSetMapperFlags)(PHYSDEV,DWORD);
    COLORREF (*pSetPixel)(PHYSDEV,INT,INT,COLORREF);
    BOOL     (*pSetPixelFormat)(PHYSDEV,INT,const PIXELFORMATDESCRIPTOR *);
    INT      (*pSetPolyFillMode)(PHYSDEV,INT);
    INT      (*pSetROP2)(PHYSDEV,INT);
    INT      (*pSetRelAbs)(PHYSDEV,INT);
    INT      (*pSetStretchBltMode)(PHYSDEV,INT);
    UINT     (*pSetTextAlign)(PHYSDEV,UINT);
    INT      (*pSetTextCharacterExtra)(PHYSDEV,INT);
    DWORD    (*pSetTextColor)(PHYSDEV,DWORD);
    INT      (*pSetTextJustification)(PHYSDEV,INT,INT);
    INT      (*pSetViewportExt)(PHYSDEV,INT,INT);
    INT      (*pSetViewportOrg)(PHYSDEV,INT,INT);
    INT      (*pSetWindowExt)(PHYSDEV,INT,INT);
    INT      (*pSetWindowOrg)(PHYSDEV,INT,INT);
    BOOL     (*pSetWorldTransform)(PHYSDEV,const XFORM*);
    INT      (*pStartDoc)(PHYSDEV,const DOCINFOA*);
    INT      (*pStartPage)(PHYSDEV);
    BOOL     (*pStretchBlt)(PHYSDEV,INT,INT,INT,INT,PHYSDEV,INT,INT,INT,INT,DWORD);
    INT      (*pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void *,
                               const BITMAPINFO*,UINT,DWORD);
    BOOL     (*pStrokeAndFillPath)(PHYSDEV);
    BOOL     (*pStrokePath)(PHYSDEV);
    BOOL     (*pSwapBuffers)(PHYSDEV);
    BOOL     (*pWidenPath)(PHYSDEV);
} DC_FUNCTIONS;

  /* DC flags */
#define DC_SAVED      0x0002   /* It is a saved DC */
#define DC_DIRTY      0x0004   /* hVisRgn has to be updated */

/* Certain functions will do no further processing if the driver returns this.
   Used by mfdrv for example. */
#define GDI_NO_MORE_WORK 2

/* bidi.c */

/* Wine_GCPW Flags */
/* Directionality -
 * LOOSE means that the paragraph dir is only set if there is no strong character.
 * FORCE means override the characters in the paragraph.
 */
#define WINE_GCPW_FORCE_LTR 0
#define WINE_GCPW_FORCE_RTL 1
#define WINE_GCPW_LOOSE_LTR 2
#define WINE_GCPW_LOOSE_RTL 3
#define WINE_GCPW_DIR_MASK 3
extern BOOL BIDI_Reorder( LPCWSTR lpString, INT uCount, DWORD dwFlags, DWORD dwWineGCP_Flags,
                          LPWSTR lpOutString, INT uCountOut, UINT *lpOrder );
extern BOOL BidiAvail;

/* clipping.c */
extern void CLIPPING_UpdateGCRegion( DC * dc );

/* dc.c */
extern DC * DC_AllocDC( const DC_FUNCTIONS *funcs, WORD magic );
extern DC * DC_GetDCUpdate( HDC hdc );
extern DC * DC_GetDCPtr( HDC hdc );
extern void DC_InitDC( DC * dc );
extern void DC_UpdateXforms( DC * dc );

/* driver.c */
extern const DC_FUNCTIONS *DRIVER_load_driver( LPCWSTR name );
extern const DC_FUNCTIONS *DRIVER_get_driver( const DC_FUNCTIONS *funcs );
extern void DRIVER_release_driver( const DC_FUNCTIONS *funcs );
extern BOOL DRIVER_GetDriverName( LPCWSTR device, LPWSTR driver, DWORD size );

/* enhmetafile.c */
extern HENHMETAFILE EMF_Create_HENHMETAFILE(ENHMETAHEADER *emh, BOOL on_disk );

/* freetype.c */
extern INT WineEngAddFontResourceEx(LPCWSTR, DWORD, PVOID);
extern GdiFont WineEngCreateFontInstance(DC*, HFONT);
extern BOOL WineEngDestroyFontInstance(HFONT handle);
extern DWORD WineEngEnumFonts(LPLOGFONTW, FONTENUMPROCW, LPARAM);
extern BOOL WineEngGetCharABCWidths(GdiFont font, UINT firstChar,
                                    UINT lastChar, LPABC buffer);
extern BOOL WineEngGetCharWidth(GdiFont, UINT, UINT, LPINT);
extern DWORD WineEngGetFontData(GdiFont, DWORD, DWORD, LPVOID, DWORD);
extern DWORD WineEngGetGlyphIndices(GdiFont font, LPCWSTR lpstr, INT count,
                                    LPWORD pgi, DWORD flags);
extern DWORD WineEngGetGlyphOutline(GdiFont, UINT glyph, UINT format,
                                    LPGLYPHMETRICS, DWORD buflen, LPVOID buf,
                                    const MAT2*);
extern UINT WineEngGetOutlineTextMetrics(GdiFont, UINT, LPOUTLINETEXTMETRICW);
extern UINT WineEngGetTextCharsetInfo(GdiFont font, LPFONTSIGNATURE fs, DWORD flags);
extern BOOL WineEngGetTextExtentPoint(GdiFont, LPCWSTR, INT, LPSIZE);
extern BOOL WineEngGetTextExtentPointI(GdiFont, const WORD *, INT, LPSIZE);
extern INT  WineEngGetTextFace(GdiFont, INT, LPWSTR);
extern BOOL WineEngGetTextMetrics(GdiFont, LPTEXTMETRICW);
extern BOOL WineEngInit(void);
extern BOOL WineEngRemoveFontResourceEx(LPCWSTR, DWORD, PVOID);

/* gdiobj.c */
extern BOOL GDI_Init(void);
extern void *GDI_AllocObject( WORD, WORD, HGDIOBJ *, const struct gdi_obj_funcs *funcs );
extern void *GDI_ReallocObject( WORD, HGDIOBJ, void *obj );
extern BOOL GDI_FreeObject( HGDIOBJ, void *obj );
extern void GDI_CheckNotLock(void);
extern BOOL GDI_hdc_using_object(HGDIOBJ obj, HDC hdc);
extern BOOL GDI_hdc_not_using_object(HGDIOBJ obj, HDC hdc);

/* metafile.c */
extern HMETAFILE MF_Create_HMETAFILE(METAHEADER *mh);
extern HMETAFILE16 MF_Create_HMETAFILE16(METAHEADER *mh);
extern METAHEADER *MF_CreateMetaHeaderDisk(METAHEADER *mr, LPCSTR filename);

/* path.c */

#define PATH_IsPathOpen(path) ((path).state==PATH_Open)
/* Returns TRUE if the specified path is in the open state, i.e. in the
 * state where points will be added to the path, or FALSE otherwise. This
 * function is implemented as a macro for performance reasons.
 */

extern void PATH_InitGdiPath(GdiPath *pPath);
extern void PATH_DestroyGdiPath(GdiPath *pPath);
extern BOOL PATH_AssignGdiPath(GdiPath *pPathDest, const GdiPath *pPathSrc);

extern BOOL PATH_MoveTo(DC *dc);
extern BOOL PATH_LineTo(DC *dc, INT x, INT y);
extern BOOL PATH_Rectangle(DC *dc, INT x1, INT y1, INT x2, INT y2);
extern BOOL PATH_Ellipse(DC *dc, INT x1, INT y1, INT x2, INT y2);
extern BOOL PATH_Arc(DC *dc, INT x1, INT y1, INT x2, INT y2,
                     INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines);
extern BOOL PATH_PolyBezierTo(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyBezier(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolylineTo(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_Polyline(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_Polygon(DC *dc, const POINT *pt, DWORD cbCount);
extern BOOL PATH_PolyPolyline(DC *dc, const POINT *pt, const DWORD *counts, DWORD polylines);
extern BOOL PATH_PolyPolygon(DC *dc, const POINT *pt, const INT *counts, UINT polygons);
extern BOOL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height);
extern BOOL PATH_AddEntry(GdiPath *pPath, const POINT *pPoint, BYTE flags);

/* painting.c */
extern POINT *GDI_Bezier( const POINT *Points, INT count, INT *nPtsOut );

/* palette.c */
extern HPALETTE WINAPI GDISelectPalette( HDC hdc, HPALETTE hpal, WORD wBkg);
extern UINT WINAPI GDIRealizePalette( HDC hdc );

/* region.c */
extern BOOL REGION_FrameRgn( HRGN dest, HRGN src, INT x, INT y );

/* text.c */
extern LPWSTR FONT_mbtowc(HDC, LPCSTR, INT, INT*, UINT*);

#endif /* __WINE_GDI_PRIVATE_H */
