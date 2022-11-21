/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#ifndef _NTGDI_
#define _NTGDI_

#include <wingdi.h>
#include <winternl.h>
#include <winspool.h>
#include <ddk/d3dkmthk.h>

typedef struct _GDI_HANDLE_ENTRY
{
    UINT64 Object;
    union
    {
        struct
        {
            USHORT ProcessId;
            USHORT Lock : 1;
            USHORT Count : 15;
        };
        ULONG Value;
    } Owner;
    union
    {
        struct
        {
            UCHAR ExtType : 7;
            UCHAR StockFlag : 1;
            UCHAR Generation;
        };
        USHORT Unique;
    };
    UCHAR Type;
    UCHAR Flags;
    UINT64 UserPointer;
} GDI_HANDLE_ENTRY, *PGDI_HANDLE_ENTRY;

#define GDI_MAX_HANDLE_COUNT 0x10000

#define NTGDI_OBJ_DC              0x010000
#define NTGDI_OBJ_ENHMETADC       0x210000
#define NTGDI_OBJ_REGION          0x040000
#define NTGDI_OBJ_SURF            0x050000
#define NTGDI_OBJ_METAFILE        0x260000
#define NTGDI_OBJ_ENHMETAFILE     0x460000
#define NTGDI_OBJ_METADC          0x660000
#define NTGDI_OBJ_PAL             0x080000
#define NTGDI_OBJ_BITMAP          0x090000
#define NTGDI_OBJ_FONT            0x0a0000
#define NTGDI_OBJ_BRUSH           0x100000
#define NTGDI_OBJ_PEN             0x300000
#define NTGDI_OBJ_EXTPEN          0x500000

/* Wine extension, native uses NTGDI_OBJ_DC */
#define NTGDI_OBJ_MEMDC           0x410000

#define NTGDI_HANDLE_TYPE_SHIFT    16
#define NTGDI_HANDLE_TYPE_MASK     0x007f0000
#define NTGDI_HANDLE_STOCK_OBJECT  0x00800000

typedef struct _GDI_SHARED_MEMORY
{
    GDI_HANDLE_ENTRY Handles[GDI_MAX_HANDLE_COUNT];
} GDI_SHARED_MEMORY, *PGDI_SHARED_MEMORY;

enum
{
    NtGdiArc,
    NtGdiArcTo,
    NtGdiChord,
    NtGdiPie,
};

enum
{
    NtGdiPolyPolygon = 1,
    NtGdiPolyPolyline,
    NtGdiPolyBezier,
    NtGdiPolyBezierTo,
    NtGdiPolylineTo,
    NtGdiPolyPolygonRgn,
};

enum
{
    NtGdiLPtoDP,
    NtGdiDPtoLP,
};

enum
{
    NtGdiSetMapMode = 8,
    /* not compatible with Windows */
    NtGdiSetBkColor = 100,
    NtGdiSetBkMode,
    NtGdiSetTextColor,
    NtGdiSetDCBrushColor,
    NtGdiSetDCPenColor,
    NtGdiSetGraphicsMode,
    NtGdiSetROP2,
    NtGdiSetTextAlign,
};

/* NtGdiGetDCDword parameter, not compatible with Windows */
enum
{
    NtGdiGetArcDirection,
    NtGdiGetBkColor,
    NtGdiGetBkMode,
    NtGdiGetDCBrushColor,
    NtGdiGetDCPenColor,
    NtGdiGetGraphicsMode,
    NtGdiGetLayout,
    NtGdiGetPolyFillMode,
    NtGdiGetROP2,
    NtGdiGetTextColor,
    NtGdiIsMemDC,
};

/* NtGdiGetDCPoint parameter, not compatible with Windows */
enum
{
    NtGdiGetBrushOrgEx,
    NtGdiGetCurrentPosition,
    NtGdiGetDCOrg,
};

enum
{
    NtGdiAnimatePalette,
    NtGdiSetPaletteEntries,
    NtGdiGetPaletteEntries,
    NtGdiGetSystemPaletteEntries,
    NtGdiGetDIBColorTable,
    NtGdiSetDIBColorTable,
};

#define NTGDI_GETCHARWIDTH_INT      0x02
#define NTGDI_GETCHARWIDTH_INDICES  0x08

#define NTGDI_GETCHARABCWIDTHS_INT      0x01
#define NTGDI_GETCHARABCWIDTHS_INDICES  0x02

#define MWT_SET  4

/* structs not compatible with native Windows */
#ifdef __WINESRC__

typedef struct DC_ATTR
{
    UINT      hdc;                 /* handle to self */
    LONG      disabled;            /* disabled flag, controlled by DCHF_(DISABLE|ENABLE)DC */
    int       save_level;
    COLORREF  background_color;
    COLORREF  brush_color;
    COLORREF  pen_color;
    COLORREF  text_color;
    POINT     cur_pos;
    INT       graphics_mode;
    INT       arc_direction;
    DWORD     layout;
    WORD      text_align;
    WORD      background_mode;
    WORD      poly_fill_mode;
    WORD      rop_mode;
    WORD      rel_abs_mode;
    WORD      stretch_blt_mode;
    INT       map_mode;
    INT       char_extra;
    DWORD     mapper_flags;        /* font mapper flags */
    RECT      vis_rect;            /* visible rectangle in screen coords */
    FLOAT     miter_limit;
    POINT     brush_org;           /* brush origin */
    POINT     wnd_org;             /* window origin */
    SIZE      wnd_ext;             /* window extent */
    POINT     vport_org;           /* viewport origin */
    SIZE      vport_ext;           /* viewport extent */
    SIZE      virtual_res;
    SIZE      virtual_size;
    UINT      font_code_page;
    RECTL     emf_bounds;
    UINT64    emf;                 /* client EMF record pointer */
    UINT64    abort_proc;          /* AbortProc for printing */
    UINT64    hspool;
    UINT64    output;
} DC_ATTR;

struct font_enum_entry
{
    DWORD            type;
    ENUMLOGFONTEXW   lf;
    NEWTEXTMETRICEXW tm;
};

/* flag for NtGdiGetRandomRgn to respect LAYOUT_RTL */
#define NTGDI_RGN_MIRROR_RTL   0x80000000

/* magic driver version that we use for win16 DCs with DIB surfaces */
#define NTGDI_WIN16_DIB  0xfafa000

#endif /* __WINESRC__ */

struct font_realization_info
{
    DWORD size;          /* could be 16 or 24 */
    DWORD flags;         /* 1 for bitmap fonts, 3 for scalable fonts */
    DWORD cache_num;     /* keeps incrementing */
    DWORD instance_id;   /* identifies a realized font instance */
    DWORD file_count;    /* number of files that make up this font */
    WORD  face_index;    /* face index in case of font collections */
    WORD  simulations;   /* 0 bit - bold simulation, 1 bit - oblique simulation */
};

struct char_width_info
{
    INT lsb;   /* minimum left side bearing */
    INT rsb;   /* minimum right side bearing */
    INT unk;   /* unknown */
};

struct font_fileinfo
{
    FILETIME writetime;
    LARGE_INTEGER size;
    WCHAR path[1];
};


INT      WINAPI NtGdiAbortDoc( HDC hdc );
BOOL     WINAPI NtGdiAbortPath( HDC hdc );
HANDLE   WINAPI NtGdiAddFontMemResourceEx( void *ptr, DWORD size, void *dv, ULONG dv_size,
                                           DWORD *count );
INT      WINAPI NtGdiAddFontResourceW( const WCHAR *str, ULONG size, ULONG files, DWORD flags,
                                       DWORD tid, void *dv );
BOOL     WINAPI NtGdiAlphaBlend( HDC hdc_dst, int x_dst, int y_dst, int width_dst, int height_dst,
                                 HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                                 BLENDFUNCTION blend_function, HANDLE xform );
BOOL     WINAPI NtGdiAngleArc( HDC hdc, INT x, INT y, DWORD radius, FLOAT start_angle,
                               FLOAT sweep_angle );
BOOL     WINAPI NtGdiArcInternal( UINT type, HDC hdc, INT left, INT top, INT right, INT bottom,
                                  INT xstart, INT ystart, INT xend, INT yend );
BOOL     WINAPI NtGdiBeginPath( HDC hdc );
BOOL     WINAPI NtGdiBitBlt( HDC hdc_dst, INT x_dst, INT y_dst, INT width, INT height, HDC hdc_src,
                             INT x_src, INT y_src, DWORD rop, DWORD bk_color, FLONG fl );
BOOL     WINAPI NtGdiCancelDC( HDC hdc );
BOOL     WINAPI NtGdiCloseFigure( HDC hdc );
INT      WINAPI NtGdiCombineRgn( HRGN dest, HRGN src1, HRGN src2, INT mode );
BOOL     WINAPI NtGdiComputeXformCoefficients( HDC hdc );
HBITMAP  WINAPI NtGdiCreateBitmap( INT width, INT height, UINT planes,
                                   UINT bpp, const void *bits );
HANDLE   WINAPI NtGdiCreateClientObj( ULONG type );
HBITMAP  WINAPI NtGdiCreateCompatibleBitmap( HDC hdc, INT width, INT height );
HDC      WINAPI NtGdiCreateCompatibleDC( HDC hdc );
HBRUSH   WINAPI NtGdiCreateDIBBrush( const void *data, UINT coloruse, UINT size,
                                     BOOL is_8x8, BOOL pen, const void *client );
HBITMAP  WINAPI NtGdiCreateDIBSection( HDC hdc, HANDLE section, DWORD offset, const BITMAPINFO *bmi,
                                       UINT usage, UINT header_size, ULONG flags,
                                       ULONG_PTR color_space, void **bits );
HBITMAP  WINAPI NtGdiCreateDIBitmapInternal( HDC hdc, INT width, INT height, DWORD init,
                                             const void *bits, const BITMAPINFO *data,
                                             UINT coloruse, UINT max_info, UINT max_bits,
                                             ULONG flags, HANDLE xform );
HRGN     WINAPI NtGdiCreateEllipticRgn( INT left, INT top, INT right, INT bottom );
HPALETTE WINAPI NtGdiCreateHalftonePalette( HDC hdc );
HBRUSH   WINAPI NtGdiCreateHatchBrushInternal( INT style, COLORREF color, BOOL pen );
HDC      WINAPI NtGdiCreateMetafileDC( HDC hdc );
HPALETTE WINAPI NtGdiCreatePaletteInternal( const LOGPALETTE *palette, UINT count );
HBRUSH   WINAPI NtGdiCreatePatternBrushInternal( HBITMAP hbitmap, BOOL pen, BOOL is_8x8 );
HPEN     WINAPI NtGdiCreatePen( INT style, INT width, COLORREF color, HBRUSH brush );
HRGN     WINAPI NtGdiCreateRectRgn( INT left, INT top, INT right, INT bottom );
HRGN     WINAPI NtGdiCreateRoundRectRgn( INT left, INT top, INT right, INT bottom,
                                         INT ellipse_width, INT ellipse_height );
HBRUSH   WINAPI NtGdiCreateSolidBrush( COLORREF color, HBRUSH brush );
BOOL     WINAPI NtGdiDeleteClientObj( HGDIOBJ obj );
BOOL     WINAPI NtGdiDeleteObjectApp( HGDIOBJ obj );
INT      WINAPI NtGdiDescribePixelFormat( HDC hdc, INT format, UINT size,
                                          PIXELFORMATDESCRIPTOR *descr );
LONG     WINAPI NtGdiDoPalette( HGDIOBJ handle, WORD start, WORD count, void *entries,
                                DWORD func, BOOL inbound );
BOOL     WINAPI NtGdiDrawStream( HDC hdc, ULONG in, void *pvin );
BOOL     WINAPI NtGdiEllipse( HDC hdc, INT left, INT top, INT right, INT bottom );
INT      WINAPI NtGdiEndDoc( HDC hdc );
BOOL     WINAPI NtGdiEndPath( HDC hdc );
INT      WINAPI NtGdiEndPage( HDC hdc );
BOOL     WINAPI NtGdiEnumFonts( HDC hdc, ULONG type, ULONG win32_compat, ULONG face_name_len,
                                const WCHAR *face_name, ULONG charset, ULONG *count, void *buf );
BOOL     WINAPI NtGdiEqualRgn( HRGN hrgn1, HRGN hrgn2 );
INT      WINAPI NtGdiExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom );
HPEN     WINAPI NtGdiExtCreatePen( DWORD style, DWORD width, ULONG brush_style, ULONG color,
                                   ULONG_PTR client_hatch, ULONG_PTR hatch, DWORD style_count,
                                   const DWORD *style_bits, ULONG dib_size, BOOL old_style,
                                   HBRUSH brush );
INT      WINAPI NtGdiExtEscape( HDC hdc, WCHAR *driver, INT driver_id, INT escape, INT input_size,
                                const char *input, INT output_size, char *output );
BOOL     WINAPI NtGdiExtFloodFill( HDC hdc, INT x, INT y, COLORREF color, UINT type );
BOOL     WINAPI NtGdiExtTextOutW( HDC hdc, INT x, INT y, UINT flags, const RECT *rect,
                                  const WCHAR *str, UINT count, const INT *dx, DWORD cp );
HRGN     WINAPI NtGdiExtCreateRegion( const XFORM *xform, DWORD count, const RGNDATA *data );
INT      WINAPI NtGdiExtGetObjectW( HGDIOBJ handle, INT count, void *buffer );
INT      WINAPI NtGdiExtSelectClipRgn( HDC hdc, HRGN region, INT mode );
BOOL     WINAPI NtGdiFillPath( HDC hdc );
BOOL     WINAPI NtGdiFillRgn( HDC hdc, HRGN hrgn, HBRUSH hbrush );
BOOL     WINAPI NtGdiFlattenPath( HDC hdc );
BOOL     WINAPI NtGdiFontIsLinked( HDC hdc );
BOOL     WINAPI NtGdiFlush(void);
BOOL     WINAPI NtGdiFrameRgn( HDC hdc, HRGN hrgn, HBRUSH brush, INT width, INT height );
BOOL     WINAPI NtGdiGetAndSetDCDword( HDC hdc, UINT method, DWORD value, DWORD *result );
INT      WINAPI NtGdiGetAppClipBox( HDC hdc, RECT *rect );
LONG     WINAPI NtGdiGetBitmapBits( HBITMAP bitmap, LONG count, void *bits );
BOOL     WINAPI NtGdiGetBitmapDimension( HBITMAP bitmap, SIZE *size );
UINT     WINAPI NtGdiGetBoundsRect( HDC hdc, RECT *rect, UINT flags );
BOOL     WINAPI NtGdiGetCharABCWidthsW( HDC hdc, UINT first, UINT last, WCHAR *chars,
                                        ULONG flags, void *buffer );
BOOL     WINAPI NtGdiGetCharWidthW( HDC hdc, UINT first_char, UINT last_char, WCHAR *chars,
                                    ULONG flags, void *buffer );
BOOL     WINAPI NtGdiGetCharWidthInfo( HDC hdc, struct char_width_info *info );
BOOL     WINAPI NtGdiGetColorAdjustment( HDC hdc, COLORADJUSTMENT *ca );
BOOL     WINAPI NtGdiGetDCDword( HDC hdc, UINT method, DWORD *result );
HANDLE   WINAPI NtGdiGetDCObject( HDC hdc, UINT type );
BOOL     WINAPI NtGdiGetDCPoint( HDC hdc, UINT method, POINT *result );
INT      WINAPI NtGdiGetDIBitsInternal( HDC hdc, HBITMAP hbitmap, UINT startscan, UINT lines,
                                        void *bits, BITMAPINFO *info, UINT coloruse,
                                        UINT max_bits, UINT max_info );
INT      WINAPI NtGdiGetDeviceCaps( HDC hdc, INT cap );
BOOL     WINAPI NtGdiGetDeviceGammaRamp( HDC hdc, void *ptr );
DWORD    WINAPI NtGdiGetFontData( HDC hdc, DWORD table, DWORD offset, void *buffer, DWORD length );
BOOL     WINAPI NtGdiGetFontFileData( DWORD instance_id, DWORD file_index, UINT64 *offset,
                                      void *buff, DWORD buff_size );
BOOL     WINAPI NtGdiGetFontFileInfo( DWORD instance_id, DWORD file_index, struct font_fileinfo *info,
                                      SIZE_T size, SIZE_T *needed );
DWORD    WINAPI NtGdiGetFontUnicodeRanges( HDC hdc, GLYPHSET *lpgs );
DWORD    WINAPI NtGdiGetGlyphIndicesW( HDC hdc, const WCHAR *str, INT count,
                                       WORD *indices, DWORD flags );
DWORD    WINAPI NtGdiGetGlyphOutline( HDC hdc, UINT ch, UINT format, GLYPHMETRICS *metrics,
                                      DWORD size, void *buffer, const MAT2 *mat2,
                                      BOOL ignore_rotation );
DWORD    WINAPI NtGdiGetKerningPairs( HDC hdc, DWORD count, KERNINGPAIR *kern_pair );
BOOL     WINAPI NtGdiGetMiterLimit( HDC hdc, FLOAT *limit );
COLORREF WINAPI NtGdiGetNearestColor( HDC hdc, COLORREF color );
UINT     WINAPI NtGdiGetNearestPaletteIndex( HPALETTE hpalette, COLORREF color );
UINT     WINAPI NtGdiGetOutlineTextMetricsInternalW( HDC hdc, UINT cbData,
                                                     OUTLINETEXTMETRICW *otm, ULONG opts );
INT      WINAPI NtGdiGetPath( HDC hdc, POINT *points, BYTE *types, INT size );
COLORREF WINAPI NtGdiGetPixel( HDC hdc, INT x, INT y );
INT      WINAPI NtGdiGetRandomRgn( HDC hdc, HRGN region, INT code );
BOOL     WINAPI NtGdiGetRasterizerCaps( RASTERIZER_STATUS *status, UINT size );
BOOL     WINAPI NtGdiGetRealizationInfo( HDC hdc, struct font_realization_info *info );
DWORD    WINAPI NtGdiGetRegionData( HRGN hrgn, DWORD count, RGNDATA *data );
INT      WINAPI NtGdiGetRgnBox( HRGN hrgn, RECT *rect );
DWORD    WINAPI NtGdiGetSpoolMessage( void *ptr1, DWORD data2, void *ptr3, DWORD data4 );
UINT     WINAPI NtGdiGetSystemPaletteUse( HDC hdc );
UINT     WINAPI NtGdiGetTextCharsetInfo( HDC hdc, FONTSIGNATURE *fs, DWORD flags );
BOOL     WINAPI NtGdiGetTextExtentExW( HDC hdc, const WCHAR *str, INT count, INT max_ext,
                                       INT *nfit, INT *dxs, SIZE *size, UINT flags );
INT      WINAPI NtGdiGetTextFaceW( HDC hdc, INT count, WCHAR *name, BOOL alias_name );
BOOL     WINAPI NtGdiGetTextMetricsW( HDC hdc, TEXTMETRICW *metrics, ULONG flags );
BOOL     WINAPI NtGdiGetTransform( HDC hdc, DWORD which, XFORM *xform );
BOOL     WINAPI NtGdiGradientFill( HDC hdc, TRIVERTEX *vert_array, ULONG nvert,
                                   void *grad_array, ULONG ngrad, ULONG mode );
HFONT    WINAPI NtGdiHfontCreate( const void *logfont, ULONG unk2, ULONG unk3,
                                  ULONG unk4, void *data );
DWORD    WINAPI NtGdiInitSpool(void);
INT      WINAPI NtGdiIntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom );
BOOL     WINAPI NtGdiInvertRgn( HDC hdc, HRGN hrgn );
BOOL     WINAPI NtGdiLineTo( HDC hdc, INT x, INT y );
BOOL     WINAPI NtGdiMaskBlt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                              HDC hdc_src, INT x_src, INT y_src, HBITMAP mask,
                              INT x_mask, INT y_mask, DWORD rop, DWORD bk_color );
BOOL     WINAPI NtGdiModifyWorldTransform( HDC hdc, const XFORM *xform, DWORD mode );
BOOL     WINAPI NtGdiMoveTo( HDC hdc, INT x, INT y, POINT *pt );
INT      WINAPI NtGdiOffsetClipRgn( HDC hdc, INT x, INT y );
INT      WINAPI NtGdiOffsetRgn( HRGN hrgn, INT x, INT y );
HDC      WINAPI NtGdiOpenDCW( UNICODE_STRING *device, const DEVMODEW *devmode,
                              UNICODE_STRING *output, ULONG type, BOOL is_display,
                              HANDLE hspool, DRIVER_INFO_2W *driver_info, void *pdev );
BOOL     WINAPI NtGdiPatBlt( HDC hdc, INT left, INT top, INT width, INT height, DWORD rop );
HRGN     WINAPI NtGdiPathToRegion( HDC hdc );
BOOL     WINAPI NtGdiPlgBlt( HDC hdc, const POINT *point, HDC hdc_src, INT x_src, INT y_src,
                             INT width, INT height, HBITMAP mask, INT x_mask, INT y_mask,
                             DWORD bk_color );
BOOL     WINAPI NtGdiPolyDraw(HDC hdc, const POINT *points, const BYTE *types, DWORD count );
ULONG    WINAPI NtGdiPolyPolyDraw( HDC hdc, const POINT *points, const ULONG *counts,
                                   DWORD count, UINT function );
BOOL     WINAPI NtGdiPtInRegion( HRGN hrgn, INT x, INT y );
BOOL     WINAPI NtGdiPtVisible( HDC hdc, INT x, INT y );
BOOL     WINAPI NtGdiRectInRegion( HRGN hrgn, const RECT *rect );
BOOL     WINAPI NtGdiRectVisible( HDC hdc, const RECT *rect );
BOOL     WINAPI NtGdiRectangle( HDC hdc, INT left, INT top, INT right, INT bottom );
BOOL     WINAPI NtGdiRemoveFontMemResourceEx( HANDLE handle );
BOOL     WINAPI NtGdiRemoveFontResourceW( const WCHAR *str, ULONG size, ULONG files,
                                          DWORD flags, DWORD tid, void *dv );
BOOL     WINAPI NtGdiResetDC( HDC hdc, const DEVMODEW *devmode, BOOL *banding,
                              DRIVER_INFO_2W *driver_info, void *dev );
BOOL     WINAPI NtGdiResizePalette( HPALETTE palette, UINT count );
BOOL     WINAPI NtGdiRestoreDC( HDC hdc, INT level );
BOOL     WINAPI NtGdiRoundRect( HDC hdc, INT left, INT top, INT right,
                                INT bottom, INT ell_width, INT ell_height );
INT      WINAPI NtGdiSaveDC( HDC hdc );
BOOL     WINAPI NtGdiScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom,
                                        INT y_num, INT y_denom, SIZE *size );
BOOL     WINAPI NtGdiScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom,
                                       INT y_num, INT y_denom, SIZE *size );
HGDIOBJ  WINAPI NtGdiSelectBitmap( HDC hdc, HGDIOBJ handle );
HGDIOBJ  WINAPI NtGdiSelectBrush( HDC hdc, HGDIOBJ handle );
BOOL     WINAPI NtGdiSelectClipPath( HDC hdc, INT mode );
HGDIOBJ  WINAPI NtGdiSelectFont( HDC hdc, HGDIOBJ handle );
HGDIOBJ  WINAPI NtGdiSelectPen( HDC hdc, HGDIOBJ handle );
LONG     WINAPI NtGdiSetBitmapBits( HBITMAP hbitmap, LONG count, const void *bits );
BOOL     WINAPI NtGdiSetBitmapDimension( HBITMAP hbitmap, INT x, INT y, SIZE *prev_size );
BOOL     WINAPI NtGdiSetBrushOrg( HDC hdc, INT x, INT y, POINT *prev_org );
UINT     WINAPI NtGdiSetBoundsRect( HDC hdc, const RECT *rect, UINT flags );
BOOL     WINAPI NtGdiSetColorAdjustment( HDC hdc, const COLORADJUSTMENT *ca );
INT      WINAPI NtGdiSetDIBitsToDeviceInternal( HDC hdc, INT x_dst, INT y_dst, DWORD cx,
                                                DWORD cy, INT x_src, INT y_src, UINT startscan,
                                                UINT lines, const void *bits, const BITMAPINFO *bmi,
                                                UINT coloruse, UINT max_bits, UINT max_info,
                                                BOOL xform_coords, HANDLE xform );
BOOL     WINAPI NtGdiSetDeviceGammaRamp( HDC hdc, void *ptr );
DWORD    WINAPI NtGdiSetLayout( HDC hdc, LONG wox, DWORD layout );
BOOL     WINAPI NtGdiSetMagicColors( HDC hdc, DWORD magic, ULONG index );
INT      WINAPI NtGdiSetMetaRgn( HDC hdc );
BOOL     WINAPI NtGdiSetMiterLimit( HDC hdc, FLOAT limit, FLOAT *prev_limit );
COLORREF WINAPI NtGdiSetPixel( HDC hdc, INT x, INT y, COLORREF color );
BOOL     WINAPI NtGdiSetPixelFormat( HDC hdc, INT format );
BOOL     WINAPI NtGdiSetRectRgn( HRGN hrgn, INT left, INT top, INT right, INT bottom );
UINT     WINAPI NtGdiSetSystemPaletteUse( HDC hdc, UINT use );
BOOL     WINAPI NtGdiSetTextJustification( HDC hdc, INT extra, INT breaks );
BOOL     WINAPI NtGdiSetVirtualResolution( HDC hdc, DWORD horz_res, DWORD vert_res,
                                           DWORD horz_size, DWORD vert_size );
INT      WINAPI NtGdiStartDoc( HDC hdc, const DOCINFOW *doc, BOOL *banding, INT job );
INT      WINAPI NtGdiStartPage( HDC hdc );
BOOL     WINAPI NtGdiStretchBlt( HDC hdc, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                                 HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                                 DWORD rop, COLORREF bk_color );
INT      WINAPI NtGdiStretchDIBitsInternal( HDC hdc, INT x_dst, INT y_dst, INT width_dst,
                                            INT height_dst, INT x_src, INT y_src, INT width_src,
                                            INT height_src, const void *bits, const BITMAPINFO *bmi,
                                            UINT coloruse, DWORD rop, UINT max_info, UINT max_bits,
                                            HANDLE xform );
BOOL     WINAPI NtGdiStrokeAndFillPath( HDC hdc );
BOOL     WINAPI NtGdiStrokePath( HDC hdc );
BOOL     WINAPI NtGdiSwapBuffers( HDC hdc );
BOOL     WINAPI NtGdiTransparentBlt( HDC hdc, int x_dst, int y_dst, int width_dst, int height_dst,
                                     HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                                     UINT color );
BOOL     WINAPI NtGdiTransformPoints( HDC hdc, const POINT *points_in, POINT *points_out,
                                      INT count, UINT mode );
BOOL     WINAPI NtGdiUnrealizeObject( HGDIOBJ obj );
BOOL     WINAPI NtGdiUpdateColors( HDC hdc );
BOOL     WINAPI NtGdiWidenPath( HDC hdc );

NTSTATUS WINAPI NtGdiDdDDICheckVidPnExclusiveOwnership( const D3DKMT_CHECKVIDPNEXCLUSIVEOWNERSHIP *desc );
NTSTATUS WINAPI NtGdiDdDDICloseAdapter( const D3DKMT_CLOSEADAPTER *desc );
NTSTATUS WINAPI NtGdiDdDDICreateDCFromMemory( D3DKMT_CREATEDCFROMMEMORY *desc );
NTSTATUS WINAPI NtGdiDdDDICreateDevice( D3DKMT_CREATEDEVICE *desc );
NTSTATUS WINAPI NtGdiDdDDIDestroyDCFromMemory( const D3DKMT_DESTROYDCFROMMEMORY *desc );
NTSTATUS WINAPI NtGdiDdDDIDestroyDevice( const D3DKMT_DESTROYDEVICE *desc );
NTSTATUS WINAPI NtGdiDdDDIEscape( const D3DKMT_ESCAPE *desc );
NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromHdc( D3DKMT_OPENADAPTERFROMHDC *desc );
NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromDeviceName( D3DKMT_OPENADAPTERFROMDEVICENAME *desc );
NTSTATUS WINAPI NtGdiDdDDIOpenAdapterFromLuid( D3DKMT_OPENADAPTERFROMLUID *desc );
NTSTATUS WINAPI NtGdiDdDDIQueryStatistics( D3DKMT_QUERYSTATISTICS *stats );
NTSTATUS WINAPI NtGdiDdDDIQueryVideoMemoryInfo( D3DKMT_QUERYVIDEOMEMORYINFO *desc );
NTSTATUS WINAPI NtGdiDdDDISetQueuedLimit( D3DKMT_SETQUEUEDLIMIT *desc );
NTSTATUS WINAPI NtGdiDdDDISetVidPnSourceOwner( const D3DKMT_SETVIDPNSOURCEOWNER *desc );

/* Wine extensions */
extern BOOL CDECL __wine_get_brush_bitmap_info( HBRUSH handle, BITMAPINFO *info, void *bits,
                                                UINT *usage );
extern BOOL CDECL __wine_get_icm_profile( HDC hdc, BOOL allow_default, DWORD *size,
                                          WCHAR *filename );
extern BOOL CDECL __wine_get_file_outline_text_metric( const WCHAR *path,
                                                       OUTLINETEXTMETRICW *otm );

#endif /* _NTGDI_ */
