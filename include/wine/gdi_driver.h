/*
 * Definitions for Wine GDI drivers
 *
 * Copyright 2011 Alexandre Julliard
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

#ifndef __WINE_WINE_GDI_DRIVER_H
#define __WINE_WINE_GDI_DRIVER_H

struct gdi_dc_funcs;

typedef struct gdi_physdev
{
    const struct gdi_dc_funcs *funcs;
    struct gdi_physdev        *next;
    HDC                        hdc;
} *PHYSDEV;

struct bitblt_coords
{
    int  log_x;     /* original position and size, in logical coords */
    int  log_y;
    int  log_width;
    int  log_height;
    int  x;         /* mapped position and size, in device coords */
    int  y;
    int  width;
    int  height;
    RECT visrect;   /* rectangle clipped to the visible part, in device coords */
    DWORD layout;   /* DC layout */
};

struct gdi_dc_funcs
{
    INT      (CDECL *pAbortDoc)(PHYSDEV);
    BOOL     (CDECL *pAbortPath)(PHYSDEV);
    BOOL     (CDECL *pAlphaBlend)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,BLENDFUNCTION);
    BOOL     (CDECL *pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    BOOL     (CDECL *pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pBeginPath)(PHYSDEV);
    INT      (CDECL *pChoosePixelFormat)(PHYSDEV,const PIXELFORMATDESCRIPTOR *);
    BOOL     (CDECL *pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pCloseFigure)(PHYSDEV);
    BOOL     (CDECL *pCreateBitmap)(PHYSDEV,HBITMAP,LPVOID);
    BOOL     (CDECL *pCreateDC)(HDC,PHYSDEV *,LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
    HBITMAP  (CDECL *pCreateDIBSection)(PHYSDEV,HBITMAP,const BITMAPINFO *,UINT);
    BOOL     (CDECL *pDeleteBitmap)(HBITMAP);
    BOOL     (CDECL *pDeleteDC)(PHYSDEV);
    BOOL     (CDECL *pDeleteObject)(PHYSDEV,HGDIOBJ);
    INT      (CDECL *pDescribePixelFormat)(PHYSDEV,INT,UINT,PIXELFORMATDESCRIPTOR *);
    DWORD    (CDECL *pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    BOOL     (CDECL *pEllipse)(PHYSDEV,INT,INT,INT,INT);
    INT      (CDECL *pEndDoc)(PHYSDEV);
    INT      (CDECL *pEndPage)(PHYSDEV);
    BOOL     (CDECL *pEndPath)(PHYSDEV);
    BOOL     (CDECL *pEnumDeviceFonts)(PHYSDEV,LPLOGFONTW,FONTENUMPROCW,LPARAM);
    INT      (CDECL *pEnumICMProfiles)(PHYSDEV,ICMENUMPROCW,LPARAM);
    INT      (CDECL *pExcludeClipRect)(PHYSDEV,INT,INT,INT,INT);
    INT      (CDECL *pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);
    INT      (CDECL *pExtEscape)(PHYSDEV,INT,INT,LPCVOID,INT,LPVOID);
    BOOL     (CDECL *pExtFloodFill)(PHYSDEV,INT,INT,COLORREF,UINT);
    INT      (CDECL *pExtSelectClipRgn)(PHYSDEV,HRGN,INT);
    BOOL     (CDECL *pExtTextOut)(PHYSDEV,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
    BOOL     (CDECL *pFillPath)(PHYSDEV);
    BOOL     (CDECL *pFillRgn)(PHYSDEV,HRGN,HBRUSH);
    BOOL     (CDECL *pFlattenPath)(PHYSDEV);
    BOOL     (CDECL *pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    BOOL     (CDECL *pGdiComment)(PHYSDEV,UINT,CONST BYTE*);
    LONG     (CDECL *pGetBitmapBits)(HBITMAP,void*,LONG);
    BOOL     (CDECL *pGetCharWidth)(PHYSDEV,UINT,UINT,LPINT);
    INT      (CDECL *pGetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPVOID,BITMAPINFO*,UINT);
    INT      (CDECL *pGetDeviceCaps)(PHYSDEV,INT);
    BOOL     (CDECL *pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    BOOL     (CDECL *pGetICMProfile)(PHYSDEV,LPDWORD,LPWSTR);
    COLORREF (CDECL *pGetNearestColor)(PHYSDEV,COLORREF);
    COLORREF (CDECL *pGetPixel)(PHYSDEV,INT,INT);
    INT      (CDECL *pGetPixelFormat)(PHYSDEV);
    UINT     (CDECL *pGetSystemPaletteEntries)(PHYSDEV,UINT,UINT,LPPALETTEENTRY);
    BOOL     (CDECL *pGetTextExtentExPoint)(PHYSDEV,LPCWSTR,INT,INT,LPINT,LPINT,LPSIZE);
    BOOL     (CDECL *pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    INT      (CDECL *pIntersectClipRect)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (CDECL *pInvertRgn)(PHYSDEV,HRGN);
    BOOL     (CDECL *pLineTo)(PHYSDEV,INT,INT);
    BOOL     (CDECL *pModifyWorldTransform)(PHYSDEV,const XFORM*,DWORD);
    BOOL     (CDECL *pMoveTo)(PHYSDEV,INT,INT);
    INT      (CDECL *pOffsetClipRgn)(PHYSDEV,INT,INT);
    BOOL     (CDECL *pOffsetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (CDECL *pOffsetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (CDECL *pPaintRgn)(PHYSDEV,HRGN);
    BOOL     (CDECL *pPatBlt)(PHYSDEV,struct bitblt_coords*,DWORD);
    BOOL     (CDECL *pPie)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (CDECL *pPolyBezier)(PHYSDEV,const POINT*,DWORD);
    BOOL     (CDECL *pPolyBezierTo)(PHYSDEV,const POINT*,DWORD);
    BOOL     (CDECL *pPolyDraw)(PHYSDEV,const POINT*,const BYTE *,DWORD);
    BOOL     (CDECL *pPolyPolygon)(PHYSDEV,const POINT*,const INT*,UINT);
    BOOL     (CDECL *pPolyPolyline)(PHYSDEV,const POINT*,const DWORD*,DWORD);
    BOOL     (CDECL *pPolygon)(PHYSDEV,const POINT*,INT);
    BOOL     (CDECL *pPolyline)(PHYSDEV,const POINT*,INT);
    BOOL     (CDECL *pPolylineTo)(PHYSDEV,const POINT*,INT);
    UINT     (CDECL *pRealizeDefaultPalette)(PHYSDEV);
    UINT     (CDECL *pRealizePalette)(PHYSDEV,HPALETTE,BOOL);
    BOOL     (CDECL *pRectangle)(PHYSDEV,INT,INT,INT,INT);
    HDC      (CDECL *pResetDC)(PHYSDEV,const DEVMODEW*);
    BOOL     (CDECL *pRestoreDC)(PHYSDEV,INT);
    BOOL     (CDECL *pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    INT      (CDECL *pSaveDC)(PHYSDEV);
    BOOL     (CDECL *pScaleViewportExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    BOOL     (CDECL *pScaleWindowExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    HBITMAP  (CDECL *pSelectBitmap)(PHYSDEV,HBITMAP);
    HBRUSH   (CDECL *pSelectBrush)(PHYSDEV,HBRUSH);
    BOOL     (CDECL *pSelectClipPath)(PHYSDEV,INT);
    HFONT    (CDECL *pSelectFont)(PHYSDEV,HFONT,HANDLE);
    HPALETTE (CDECL *pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    HPEN     (CDECL *pSelectPen)(PHYSDEV,HPEN);
    INT      (CDECL *pSetArcDirection)(PHYSDEV,INT);
    LONG     (CDECL *pSetBitmapBits)(HBITMAP,const void*,LONG);
    COLORREF (CDECL *pSetBkColor)(PHYSDEV,COLORREF);
    INT      (CDECL *pSetBkMode)(PHYSDEV,INT);
    COLORREF (CDECL *pSetDCBrushColor)(PHYSDEV, COLORREF);
    COLORREF (CDECL *pSetDCPenColor)(PHYSDEV, COLORREF);
    UINT     (CDECL *pSetDIBColorTable)(PHYSDEV,UINT,UINT,const RGBQUAD*);
    INT      (CDECL *pSetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    INT      (CDECL *pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    VOID     (CDECL *pSetDeviceClipping)(PHYSDEV,HRGN,HRGN);
    BOOL     (CDECL *pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    DWORD    (CDECL *pSetLayout)(PHYSDEV,DWORD);
    INT      (CDECL *pSetMapMode)(PHYSDEV,INT);
    DWORD    (CDECL *pSetMapperFlags)(PHYSDEV,DWORD);
    COLORREF (CDECL *pSetPixel)(PHYSDEV,INT,INT,COLORREF);
    BOOL     (CDECL *pSetPixelFormat)(PHYSDEV,INT,const PIXELFORMATDESCRIPTOR *);
    INT      (CDECL *pSetPolyFillMode)(PHYSDEV,INT);
    INT      (CDECL *pSetROP2)(PHYSDEV,INT);
    INT      (CDECL *pSetRelAbs)(PHYSDEV,INT);
    INT      (CDECL *pSetStretchBltMode)(PHYSDEV,INT);
    UINT     (CDECL *pSetTextAlign)(PHYSDEV,UINT);
    INT      (CDECL *pSetTextCharacterExtra)(PHYSDEV,INT);
    COLORREF (CDECL *pSetTextColor)(PHYSDEV,COLORREF);
    BOOL     (CDECL *pSetTextJustification)(PHYSDEV,INT,INT);
    BOOL     (CDECL *pSetViewportExtEx)(PHYSDEV,INT,INT,SIZE*);
    BOOL     (CDECL *pSetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (CDECL *pSetWindowExtEx)(PHYSDEV,INT,INT,SIZE*);
    BOOL     (CDECL *pSetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (CDECL *pSetWorldTransform)(PHYSDEV,const XFORM*);
    INT      (CDECL *pStartDoc)(PHYSDEV,const DOCINFOW*);
    INT      (CDECL *pStartPage)(PHYSDEV);
    BOOL     (CDECL *pStretchBlt)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,DWORD);
    INT      (CDECL *pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void*,const BITMAPINFO*,UINT,DWORD);
    BOOL     (CDECL *pStrokeAndFillPath)(PHYSDEV);
    BOOL     (CDECL *pStrokePath)(PHYSDEV);
    BOOL     (CDECL *pSwapBuffers)(PHYSDEV);
    BOOL     (CDECL *pUnrealizePalette)(HPALETTE);
    BOOL     (CDECL *pWidenPath)(PHYSDEV);

    /* OpenGL32 */
    BOOL     (CDECL *pwglCopyContext)(HGLRC,HGLRC,UINT);
    HGLRC    (CDECL *pwglCreateContext)(PHYSDEV);
    HGLRC    (CDECL *pwglCreateContextAttribsARB)(PHYSDEV,HGLRC,const int*);
    BOOL     (CDECL *pwglDeleteContext)(HGLRC);
    PROC     (CDECL *pwglGetProcAddress)(LPCSTR);
    HDC      (CDECL *pwglGetPbufferDCARB)(PHYSDEV,void*);
    BOOL     (CDECL *pwglMakeCurrent)(PHYSDEV,HGLRC);
    BOOL     (CDECL *pwglMakeContextCurrentARB)(PHYSDEV,PHYSDEV,HGLRC);
    BOOL     (CDECL *pwglSetPixelFormatWINE)(PHYSDEV,INT,const PIXELFORMATDESCRIPTOR*);
    BOOL     (CDECL *pwglShareLists)(HGLRC,HGLRC);
    BOOL     (CDECL *pwglUseFontBitmapsA)(PHYSDEV,DWORD,DWORD,DWORD);
    BOOL     (CDECL *pwglUseFontBitmapsW)(PHYSDEV,DWORD,DWORD,DWORD);
};

/* increment this when you change the DC function table */
#define WINE_GDI_DRIVER_VERSION 1

static inline PHYSDEV get_physdev_entry_point( PHYSDEV dev, size_t offset )
{
    while (!((void **)dev->funcs)[offset / sizeof(void *)]) dev = dev->next;
    return dev;
}

#define GET_DC_PHYSDEV(dc,func) get_physdev_entry_point( (dc)->physDev, FIELD_OFFSET(DC_FUNCTIONS,func))
#define GET_NEXT_PHYSDEV(dev,func) get_physdev_entry_point( (dev)->next, FIELD_OFFSET(DC_FUNCTIONS,func))

#endif /* __WINE_WINE_GDI_DRIVER_H */
