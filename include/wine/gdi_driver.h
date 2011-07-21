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

struct gdi_image_bits
{
    void   *ptr;       /* pointer to the bits */
    BOOL    is_copy;   /* whether this is a copy of the bits that can be modified */
    void  (*free)(struct gdi_image_bits *);  /* callback for freeing the bits */
    void   *param;     /* extra parameter for callback private use */
};

struct gdi_dc_funcs
{
    INT      (*pAbortDoc)(PHYSDEV);
    BOOL     (*pAbortPath)(PHYSDEV);
    BOOL     (*pAlphaBlend)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,BLENDFUNCTION);
    BOOL     (*pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    BOOL     (*pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pBeginPath)(PHYSDEV);
    INT      (*pChoosePixelFormat)(PHYSDEV,const PIXELFORMATDESCRIPTOR *);
    BOOL     (*pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pCloseFigure)(PHYSDEV);
    BOOL     (*pCreateBitmap)(PHYSDEV,HBITMAP,LPVOID);
    BOOL     (*pCreateDC)(HDC,PHYSDEV *,LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
    HBITMAP  (*pCreateDIBSection)(PHYSDEV,HBITMAP,const BITMAPINFO *,UINT);
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
    INT      (*pEnumICMProfiles)(PHYSDEV,ICMENUMPROCW,LPARAM);
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
    INT      (*pGetDeviceCaps)(PHYSDEV,INT);
    BOOL     (*pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    BOOL     (*pGetICMProfile)(PHYSDEV,LPDWORD,LPWSTR);
    DWORD    (*pGetImage)(PHYSDEV,HBITMAP,BITMAPINFO*,struct gdi_image_bits*,struct bitblt_coords*);
    COLORREF (*pGetNearestColor)(PHYSDEV,COLORREF);
    COLORREF (*pGetPixel)(PHYSDEV,INT,INT);
    INT      (*pGetPixelFormat)(PHYSDEV);
    UINT     (*pGetSystemPaletteEntries)(PHYSDEV,UINT,UINT,LPPALETTEENTRY);
    BOOL     (*pGetTextExtentExPoint)(PHYSDEV,LPCWSTR,INT,INT,LPINT,LPINT,LPSIZE);
    BOOL     (*pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    INT      (*pIntersectClipRect)(PHYSDEV,INT,INT,INT,INT);
    BOOL     (*pInvertRgn)(PHYSDEV,HRGN);
    BOOL     (*pLineTo)(PHYSDEV,INT,INT);
    BOOL     (*pModifyWorldTransform)(PHYSDEV,const XFORM*,DWORD);
    BOOL     (*pMoveTo)(PHYSDEV,INT,INT);
    INT      (*pOffsetClipRgn)(PHYSDEV,INT,INT);
    BOOL     (*pOffsetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (*pOffsetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (*pPaintRgn)(PHYSDEV,HRGN);
    BOOL     (*pPatBlt)(PHYSDEV,struct bitblt_coords*,DWORD);
    BOOL     (*pPie)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    BOOL     (*pPolyBezier)(PHYSDEV,const POINT*,DWORD);
    BOOL     (*pPolyBezierTo)(PHYSDEV,const POINT*,DWORD);
    BOOL     (*pPolyDraw)(PHYSDEV,const POINT*,const BYTE *,DWORD);
    BOOL     (*pPolyPolygon)(PHYSDEV,const POINT*,const INT*,UINT);
    BOOL     (*pPolyPolyline)(PHYSDEV,const POINT*,const DWORD*,DWORD);
    BOOL     (*pPolygon)(PHYSDEV,const POINT*,INT);
    BOOL     (*pPolyline)(PHYSDEV,const POINT*,INT);
    BOOL     (*pPolylineTo)(PHYSDEV,const POINT*,INT);
    DWORD    (*pPutImage)(PHYSDEV,HBITMAP,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,DWORD);
    UINT     (*pRealizeDefaultPalette)(PHYSDEV);
    UINT     (*pRealizePalette)(PHYSDEV,HPALETTE,BOOL);
    BOOL     (*pRectangle)(PHYSDEV,INT,INT,INT,INT);
    HDC      (*pResetDC)(PHYSDEV,const DEVMODEW*);
    BOOL     (*pRestoreDC)(PHYSDEV,INT);
    BOOL     (*pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    INT      (*pSaveDC)(PHYSDEV);
    BOOL     (*pScaleViewportExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    BOOL     (*pScaleWindowExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    HBITMAP  (*pSelectBitmap)(PHYSDEV,HBITMAP);
    HBRUSH   (*pSelectBrush)(PHYSDEV,HBRUSH);
    BOOL     (*pSelectClipPath)(PHYSDEV,INT);
    HFONT    (*pSelectFont)(PHYSDEV,HFONT,HANDLE);
    HPALETTE (*pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    HPEN     (*pSelectPen)(PHYSDEV,HPEN);
    INT      (*pSetArcDirection)(PHYSDEV,INT);
    LONG     (*pSetBitmapBits)(HBITMAP,const void*,LONG);
    COLORREF (*pSetBkColor)(PHYSDEV,COLORREF);
    INT      (*pSetBkMode)(PHYSDEV,INT);
    COLORREF (*pSetDCBrushColor)(PHYSDEV, COLORREF);
    COLORREF (*pSetDCPenColor)(PHYSDEV, COLORREF);
    UINT     (*pSetDIBColorTable)(PHYSDEV,UINT,UINT,const RGBQUAD*);
    INT      (*pSetDIBits)(PHYSDEV,HBITMAP,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    INT      (*pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,const BITMAPINFO*,UINT);
    VOID     (*pSetDeviceClipping)(PHYSDEV,HRGN,HRGN);
    BOOL     (*pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    DWORD    (*pSetLayout)(PHYSDEV,DWORD);
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
    COLORREF (*pSetTextColor)(PHYSDEV,COLORREF);
    BOOL     (*pSetTextJustification)(PHYSDEV,INT,INT);
    BOOL     (*pSetViewportExtEx)(PHYSDEV,INT,INT,SIZE*);
    BOOL     (*pSetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (*pSetWindowExtEx)(PHYSDEV,INT,INT,SIZE*);
    BOOL     (*pSetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    BOOL     (*pSetWorldTransform)(PHYSDEV,const XFORM*);
    INT      (*pStartDoc)(PHYSDEV,const DOCINFOW*);
    INT      (*pStartPage)(PHYSDEV);
    BOOL     (*pStretchBlt)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,DWORD);
    INT      (*pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void*,const BITMAPINFO*,UINT,DWORD);
    BOOL     (*pStrokeAndFillPath)(PHYSDEV);
    BOOL     (*pStrokePath)(PHYSDEV);
    BOOL     (*pSwapBuffers)(PHYSDEV);
    BOOL     (*pUnrealizePalette)(HPALETTE);
    BOOL     (*pWidenPath)(PHYSDEV);

    /* OpenGL32 */
    BOOL     (*pwglCopyContext)(HGLRC,HGLRC,UINT);
    HGLRC    (*pwglCreateContext)(PHYSDEV);
    HGLRC    (*pwglCreateContextAttribsARB)(PHYSDEV,HGLRC,const int*);
    BOOL     (*pwglDeleteContext)(HGLRC);
    HDC      (*pwglGetPbufferDCARB)(PHYSDEV,void*);
    PROC     (*pwglGetProcAddress)(LPCSTR);
    BOOL     (*pwglMakeContextCurrentARB)(PHYSDEV,PHYSDEV,HGLRC);
    BOOL     (*pwglMakeCurrent)(PHYSDEV,HGLRC);
    BOOL     (*pwglSetPixelFormatWINE)(PHYSDEV,INT,const PIXELFORMATDESCRIPTOR*);
    BOOL     (*pwglShareLists)(HGLRC,HGLRC);
    BOOL     (*pwglUseFontBitmapsA)(PHYSDEV,DWORD,DWORD,DWORD);
    BOOL     (*pwglUseFontBitmapsW)(PHYSDEV,DWORD,DWORD,DWORD);
};

/* increment this when you change the DC function table */
#define WINE_GDI_DRIVER_VERSION 6

static inline PHYSDEV get_physdev_entry_point( PHYSDEV dev, size_t offset )
{
    while (!((void **)dev->funcs)[offset / sizeof(void *)]) dev = dev->next;
    return dev;
}

#define GET_NEXT_PHYSDEV(dev,func) \
    get_physdev_entry_point( (dev)->next, FIELD_OFFSET(struct gdi_dc_funcs,func))

#endif /* __WINE_WINE_GDI_DRIVER_H */
