/*
 *	PostScript driver definitions
 *
 *	Copyright 1998  Huw D M Davies
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

#ifndef __WINE_PSDRV_H
#define __WINE_PSDRV_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ntgdi.h"
#include "winspool.h"

#include "unixlib.h"

#include "wine/list.h"

struct ps_bitblt_coords
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

struct ps_image_bits
{
    void   *ptr;       /* pointer to the bits */
    BOOL    is_copy;   /* whether this is a copy of the bits that can be modified */
    void  (*free)(struct ps_image_bits *);  /* callback for freeing the bits */
};

struct ps_brush_pattern
{
    BITMAPINFO          *info;     /* DIB info */
    struct ps_image_bits bits;     /* DIB bits */
    UINT                 usage;    /* color usage for DIB info */
};

typedef struct {
    LONG	    UV;
    const char     *name;
} UNICODEGLYPH;

typedef struct {
    float	llx, lly, urx, ury;
} AFMBBOX;

typedef struct _tagAFMLIGS {
    char		*successor;
    char		*ligature;
    struct _tagAFMLIGS	*next;
} AFMLIGS;

typedef struct {
    LONG     	    	UV;
    float		WX;
    AFMBBOX		B;
} OLD_AFMMETRICS;

typedef struct {
    WCHAR    	    	UV; 	    	    	/* Unicode value */
    SHORT   	    	WX; 	    	    	/* Advance width */
} AFMMETRICS;

typedef struct {
    USHORT    	    	usUnitsPerEm; 	    	/* head:unitsPerEm */
    SHORT   	    	sAscender;  	    	/* hhea:Ascender */
    SHORT   	    	sDescender; 	    	/* hhea:Descender */
    SHORT   	    	sLineGap;   	    	/* hhea:LineGap */
    SHORT   	    	sAvgCharWidth;	    	/* OS/2:xAvgCharWidth */
    USHORT  	    	usWinAscent;	    	/* OS/2:usWinAscent */
    USHORT  	    	usWinDescent;	    	/* OS/2:usWinDescent */
} WINMETRICS;

typedef struct _tagAFM {
    const char         *FontName;
    const WCHAR        *FamilyName;
    const WCHAR        *EncodingScheme;
    LONG		Weight;			/* FW_NORMAL etc. */
    float		ItalicAngle;
    BOOL		IsFixedPitch;
    AFMBBOX		FontBBox;
    WINMETRICS	    	WinMetrics;
    int			NumofMetrics;
    const AFMMETRICS	*Metrics;
} AFM;

/* Note no 'next' in AFM. Use AFMLISTENTRY as a container. This allows more than
   one list to exist without having to reallocate the entire AFM structure. We
   keep a global list of all afms (PSDRV_AFMFontList) plus a list of available
   fonts for each DC (dc->physDev->Fonts) */

typedef struct _tagAFMLISTENTRY {
    const AFM			*afm;
    struct _tagAFMLISTENTRY	*next;
} AFMLISTENTRY;

typedef struct _tagFONTFAMILY {
    WCHAR			*FamilyName; /* family name */
    AFMLISTENTRY		*afmlist;    /* list of afms for this family */
    struct _tagFONTFAMILY	*next;       /* next family */
} FONTFAMILY;

extern FONTFAMILY   *PSDRV_AFMFontList;
extern const AFM    *const PSDRV_BuiltinAFMs[];     /* last element is NULL */

typedef struct
{
    struct list          entry;
    char		*Name;
} FONTNAME;

typedef struct {
    float	llx, lly, urx, ury;
} IMAGEABLEAREA;

typedef struct {
    float	x, y;
} PAPERDIMENSION;

/* Solaris kludge */
#undef PAGESIZE
typedef struct _tagPAGESIZE {
    struct list         entry;
    char		*Name;
    WCHAR		*FullName;
    char		*InvocationString;
    IMAGEABLEAREA	*ImageableArea;
    PAPERDIMENSION	*PaperDimension;
    WORD		WinPage; /*eg DMPAPER_A4. Doesn't really belong here */
} PAGESIZE;

typedef struct
{
    struct list                 entry;
    char			*Feature1;
    char			*Value1;
    char			*Feature2;
    char			*Value2;
} CONSTRAINT;

typedef struct
{
    struct list                 entry;
    const char			*Name;
    const char			*FullName;
    char			*InvocationString;
    WORD			WinBin; /* eg DMBIN_LOWER */
} INPUTSLOT;

typedef enum _RASTERIZEROPTION
  {RO_None, RO_Accept68K, RO_Type42, RO_TrueImage} RASTERIZEROPTION;

typedef struct
{
    struct list                 entry;
    char                        *Name;
    char                        *FullName;
    char                        *InvocationString;
    WORD                        WinDuplex; /* eg DMDUP_SIMPLEX */
} DUPLEX;

typedef struct
{
    struct list entry;
    int    resx, resy;
    char   *InvocationString;
} RESOLUTION;

/* Many Mac OS X based ppd files don't include a *ColorDevice line, so
   we use a tristate here rather than a boolean.  Code that
   cares is expected to treat these as if they were colour. */
typedef enum {
    CD_NotSpecified,
    CD_False,
    CD_True
} COLORDEVICE;

typedef struct {
    char		*NickName;
    int			LanguageLevel;
    COLORDEVICE	        ColorDevice;
    struct list         Resolutions;
    int			DefaultResolution;
    signed int		LandscapeOrientation;
    char		*JCLBegin;
    char		*JCLToPSInterpreter;
    char		*JCLEnd;
    char		*DefaultFont;
    struct list         InstalledFonts;
    struct list         PageSizes;
    PAGESIZE            *DefaultPageSize;
    struct list         Constraints;
    struct list         InputSlots;
    RASTERIZEROPTION    TTRasterizer;
    struct list         Duplexes;
    DUPLEX              *DefaultDuplex;
} PPD;

typedef struct
{
    struct list             entry;
    WCHAR                   *friendly_name;
    PPD			    *ppd;
    PSDRV_DEVMODE	    *Devmode;
    FONTFAMILY		    *Fonts;
    PPRINTER_ENUM_VALUESW   FontSubTable;
    DWORD		    FontSubTableSize;
} PRINTERINFO;

typedef struct {
    float		r, g, b;
} PSRGB;

typedef struct {
    float		i;
} PSGRAY;


/* def's for PSCOLOR.type */
#define PSCOLOR_GRAY	0
#define PSCOLOR_RGB	1

typedef struct {
    int			type;
    union {
        PSRGB  rgb;
        PSGRAY gray;
    }                   value;
} PSCOLOR;

typedef struct {
    const AFM           *afm;
    float               scale;
    TEXTMETRICW         tm;
} BUILTIN;

typedef struct tagTYPE42 TYPE42;

typedef struct tagTYPE1 TYPE1;

enum downloadtype {
  Type1, Type42
};

typedef struct _tagDOWNLOAD {
  enum downloadtype type;
  union {
    TYPE1  *Type1;
    TYPE42 *Type42;
  } typeinfo;
  char *ps_name;
  struct _tagDOWNLOAD *next;
} DOWNLOAD;

enum fontloc {
  Builtin, Download
};

typedef struct
{
    INT xx, xy, yx, yy;
} matrix;

enum fontset { UNSET = 0, HORIZONTAL_SET, VERTICAL_SET };

typedef struct {
    enum fontloc        fontloc;
    union {
        BUILTIN  Builtin;
        DOWNLOAD *Download;
    }                   fontinfo;

    matrix              size;
    PSCOLOR             color;
    enum fontset        set;    /* Have we done a setfont yet */
} PSFONT;

typedef struct {
    PSCOLOR                 color;
    BOOL                    set;
    struct ps_brush_pattern pattern;
} PSBRUSH;

#define MAX_DASHLEN 16

typedef struct {
    INT                 style;
    INT                 width;
    BYTE                join;
    BYTE                endcap;
    DWORD               dash[MAX_DASHLEN];
    DWORD               dash_len;
    PSCOLOR		color;
    BOOL		set;
} PSPEN;

enum passthrough
{
    passthrough_none,
    passthrough_active,
    passthrough_had_rect,  /* See the comment in PSDRV_Rectangle */
};

typedef struct {
    DWORD		id;             /* Job id */
    HANDLE              hprinter;       /* Printer handle */
    LPWSTR              doc_name;       /* Document Name */
    BOOL		OutOfPage;      /* Page header not sent yet */
    INT			PageNo;
    BOOL                quiet;          /* Don't actually output anything */
    enum passthrough    passthrough_state;
    BYTE                data[4096];
    int                 data_cnt;
} JOB;

typedef struct
{
    HDC                 hdc;
    PSFONT		font;		/* Current PS font */
    DOWNLOAD            *downloaded_fonts;
    PSPEN		pen;
    PSBRUSH		brush;
    PSCOLOR		bkColor;
    JOB			job;
    PSDRV_DEVMODE	*Devmode;
    PRINTERINFO		*pi;
    int                 pathdepth;
    RECT                bbox;
} print_ctx;

extern print_ctx *create_print_ctx( HDC hdc, const WCHAR *device,
        const DEVMODEW *devmode );

/*
 *  The AGL encoding vector
 */

extern const INT    	    PSDRV_AGLbyNameSize; /* sorted by name */
extern const UNICODEGLYPH   PSDRV_AGLbyName[];	 /*  duplicates omitted  */

extern const INT    	    PSDRV_AGLbyUVSize;	 /* sorted by UV */
extern const UNICODEGLYPH   PSDRV_AGLbyUV[];	 /*  duplicates included */

extern HINSTANCE PSDRV_hInstance;
extern HANDLE PSDRV_Heap;
extern char *PSDRV_ANSIVector[256];

extern INPUTSLOT *find_slot( PPD *ppd, const DEVMODEW *dm );
extern PAGESIZE *find_pagesize( PPD *ppd, const DEVMODEW *dm );
extern DUPLEX *find_duplex( PPD *ppd, const DEVMODEW *dm );

/* GDI driver functions */
extern BOOL PSDRV_Arc( print_ctx *ctx, INT left, INT top, INT right, INT bottom,
                       INT xstart, INT ystart, INT xend, INT yend );
extern BOOL PSDRV_Chord( print_ctx *ctx, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend );
extern BOOL PSDRV_Ellipse( print_ctx *ctx, INT left, INT top, INT right, INT bottom);
extern INT PSDRV_EndPage( print_ctx *ctx );
extern INT PSDRV_ExtEscape( print_ctx *ctx, INT nEscape, INT cbInput, LPCVOID in_data,
                            INT cbOutput, LPVOID out_data );
extern BOOL PSDRV_ExtTextOut( print_ctx *ctx, INT x, INT y, UINT flags,
                              const RECT *lprect, LPCWSTR str, UINT count, const INT *lpDx );
extern BOOL PSDRV_FillPath( print_ctx *ctx );
extern BOOL PSDRV_LineTo(print_ctx *ctx, INT x, INT y);
extern BOOL PSDRV_PaintRgn( print_ctx *ctx, HRGN hrgn );
extern BOOL PSDRV_PatBlt(print_ctx *ctx, struct ps_bitblt_coords *dst, DWORD dwRop);
extern BOOL PSDRV_Pie( print_ctx *ctx, INT left, INT top, INT right, INT bottom,
                       INT xstart, INT ystart, INT xend, INT yend );
extern BOOL PSDRV_PolyBezier( print_ctx *ctx, const POINT *pts, DWORD count );
extern BOOL PSDRV_PolyBezierTo( print_ctx *ctx, const POINT *pts, DWORD count );
extern BOOL PSDRV_PolyPolygon( print_ctx *ctx, const POINT* pts, const INT* counts, UINT polygons );
extern BOOL PSDRV_PolyPolyline( print_ctx *ctx, const POINT* pts, const DWORD* counts, DWORD polylines );
extern DWORD PSDRV_PutImage( print_ctx *ctx, HRGN clip, BITMAPINFO *info,
                             const struct ps_image_bits *bits, struct ps_bitblt_coords *src,
                             struct ps_bitblt_coords *dst, DWORD rop );
extern BOOL PSDRV_Rectangle( print_ctx *ctx, INT left, INT top, INT right, INT bottom );
extern BOOL PSDRV_RoundRect( print_ctx *ctx, INT left, INT top, INT right,
                             INT bottom, INT ell_width, INT ell_height );
extern HBRUSH PSDRV_SelectBrush( print_ctx *ctx, HBRUSH hbrush, const struct ps_brush_pattern *pattern );
extern HFONT PSDRV_SelectFont( print_ctx *ctx, HFONT hfont, UINT *aa_flags );
extern HPEN PSDRV_SelectPen( print_ctx *ctx, HPEN hpen, const struct ps_brush_pattern *pattern );
extern COLORREF PSDRV_SetBkColor( print_ctx *ctx, COLORREF color );
extern COLORREF PSDRV_SetDCBrushColor( print_ctx *ctx, COLORREF color );
extern COLORREF PSDRV_SetDCPenColor( print_ctx *ctx, COLORREF color );
extern COLORREF PSDRV_SetPixel( print_ctx *ctx, INT x, INT y, COLORREF color );
extern COLORREF PSDRV_SetTextColor( print_ctx *ctx, COLORREF color );
extern BOOL PSDRV_StrokeAndFillPath( print_ctx *ctx );
extern BOOL PSDRV_StrokePath( print_ctx *ctx );

extern BOOL PSDRV_ResetDC( print_ctx *ctx, const DEVMODEW *lpInitData );
extern void PSDRV_MergeDevmodes(PSDRV_DEVMODE *dm1, const DEVMODEW *dm2,
			 PRINTERINFO *pi);
extern BOOL PSDRV_GetFontMetrics(void);
extern PPD *PSDRV_ParsePPD(const WCHAR *fname, HANDLE printer);
extern PRINTERINFO *PSDRV_FindPrinterInfo(LPCWSTR name);
extern const AFM *PSDRV_FindAFMinList(FONTFAMILY *head, LPCSTR name);
extern BOOL PSDRV_AddAFMtoList(FONTFAMILY **head, const AFM *afm,
	BOOL *p_added);
extern void PSDRV_FreeAFMList( FONTFAMILY *head );

extern INT PSDRV_XWStoDS( print_ctx *ctx, INT width );

extern BOOL PSDRV_Brush(print_ctx *ctx, BOOL EO);
extern BOOL PSDRV_SetFont( print_ctx *ctx, BOOL vertical );
extern BOOL PSDRV_SetPen( print_ctx *ctx );

extern void PSDRV_AddClip( print_ctx *ctx, HRGN hrgn );
extern void PSDRV_SetClip( print_ctx *ctx );
extern void PSDRV_ResetClip( print_ctx *ctx );

extern void PSDRV_CreateColor( print_ctx *ctx, PSCOLOR *pscolor,
		     COLORREF wincolor );
extern PSRGB rgb_to_grayscale_scale( void );
extern char PSDRV_UnicodeToANSI(int u);

extern INT PSDRV_WriteHeader( print_ctx *ctx, LPCWSTR title );
extern INT PSDRV_WriteFooter( print_ctx *ctx );
extern INT PSDRV_WritePageSize( print_ctx *ctx );
extern INT PSDRV_WriteNewPage( print_ctx *ctx );
extern INT PSDRV_WriteEndPage( print_ctx *ctx );
extern BOOL PSDRV_WriteMoveTo(print_ctx *ctx, INT x, INT y);
extern BOOL PSDRV_WriteLineTo(print_ctx *ctx, INT x, INT y);
extern BOOL PSDRV_WriteStroke(print_ctx *ctx);
extern BOOL PSDRV_WriteRectangle(print_ctx *ctx, INT x, INT y, INT width, INT height);
extern BOOL PSDRV_WriteRRectangle(print_ctx *ctx, INT x, INT y, INT width, INT height);
extern BOOL PSDRV_WriteSetFont(print_ctx *ctx, const char *name, matrix size, INT escapement,
                               BOOL fake_italic);
extern BOOL PSDRV_WriteGlyphShow(print_ctx *ctx, LPCSTR g_name);
extern BOOL PSDRV_WriteSetPen(print_ctx *ctx);
extern BOOL PSDRV_WriteArc(print_ctx *ctx, INT x, INT y, INT w, INT h,
			     double ang1, double ang2);
extern BOOL PSDRV_WriteCurveTo(print_ctx *ctx, POINT pts[3]);
extern BOOL PSDRV_WriteSetColor(print_ctx *ctx, PSCOLOR *color);
extern BOOL PSDRV_WriteSetBrush(print_ctx *ctx);
extern BOOL PSDRV_WriteFill(print_ctx *ctx);
extern BOOL PSDRV_WriteEOFill(print_ctx *ctx);
extern BOOL PSDRV_WriteGSave(print_ctx *ctx);
extern BOOL PSDRV_WriteGRestore(print_ctx *ctx);
extern BOOL PSDRV_WriteNewPath(print_ctx *ctx);
extern BOOL PSDRV_WriteClosePath(print_ctx *ctx);
extern BOOL PSDRV_WriteClip(print_ctx *ctx);
extern BOOL PSDRV_WriteRectClip(print_ctx *ctx, INT x, INT y, INT w, INT h);
extern BOOL PSDRV_WriteRectClip2(print_ctx *ctx, CHAR *pszArrayName);
extern BOOL PSDRV_WriteEOClip(print_ctx *ctx);
extern BOOL PSDRV_WriteHatch(print_ctx *ctx);
extern BOOL PSDRV_WriteRotate(print_ctx *ctx, float ang);
extern BOOL PSDRV_WriteIndexColorSpaceBegin(print_ctx *ctx, int size);
extern BOOL PSDRV_WriteIndexColorSpaceEnd(print_ctx *ctx);
extern BOOL PSDRV_WriteRGBQUAD(print_ctx *ctx, const RGBQUAD *rgb, int number);
extern BOOL PSDRV_WriteImage(print_ctx *ctx, WORD depth, BOOL grayscale, INT xDst, INT yDst,
			     INT widthDst, INT heightDst, INT widthSrc,
			     INT heightSrc, BOOL mask, BOOL top_down);
extern BOOL PSDRV_WriteBytes(print_ctx *ctx, const BYTE *bytes, DWORD number);
extern BOOL PSDRV_WriteData(print_ctx *ctx, const BYTE *byte, DWORD number);
extern DWORD PSDRV_WriteSpool(print_ctx *ctx, LPCSTR lpData, DWORD cch);
extern BOOL PSDRV_WriteDIBPatternDict(print_ctx *ctx, const BITMAPINFO *bmi, BYTE *bits, UINT usage);
extern BOOL PSDRV_WriteArrayPut(print_ctx *ctx, CHAR *pszArrayName, INT nIndex, LONG lCoord);
extern BOOL PSDRV_WriteArrayDef(print_ctx *ctx, CHAR *pszArrayName, INT nSize);

extern INT PSDRV_StartPage( print_ctx *ctx );

BOOL PSDRV_GetType1Metrics(void);
SHORT PSDRV_CalcAvgCharWidth(const AFM *afm);

extern BOOL PSDRV_WriteSetBuiltinFont(print_ctx *ctx);
extern BOOL PSDRV_WriteBuiltinGlyphShow(print_ctx *ctx, LPCWSTR str, INT count);

extern BOOL PSDRV_WriteSetDownloadFont(print_ctx *ctx, BOOL vertical);
extern BOOL PSDRV_WriteDownloadGlyphShow(print_ctx *ctx, const WORD *glyphs, UINT count);
extern BOOL PSDRV_EmptyDownloadList(print_ctx *ctx, BOOL write_undef);

extern BOOL flush_spool(print_ctx *ctx);
extern DWORD write_spool(print_ctx *ctx, const void *data, DWORD num);

#define MAX_G_NAME 31 /* max length of PS glyph name */
extern void get_glyph_name(HDC hdc, WORD index, char *name);

extern TYPE1 *T1_download_header(print_ctx *ctx, char *ps_name,
                                 RECT *bbox, UINT emsize);
extern BOOL T1_download_glyph(print_ctx *ctx, DOWNLOAD *pdl,
			      DWORD index, char *glyph_name);
extern void T1_free(TYPE1 *t1);

extern TYPE42 *T42_download_header(print_ctx *ctx, char *ps_name,
                                   RECT *bbox, UINT emsize);
extern BOOL T42_download_glyph(print_ctx *ctx, DOWNLOAD *pdl,
			       DWORD index, char *glyph_name);
extern void T42_free(TYPE42 *t42);

extern DWORD RLE_encode(BYTE *in_buf, DWORD len, BYTE *out_buf);
extern DWORD ASCII85_encode(BYTE *in_buf, DWORD len, BYTE *out_buf);

extern void passthrough_enter(print_ctx *ctx);
extern void passthrough_leave(print_ctx *ctx);

extern _locale_t c_locale;

static inline WCHAR *strdupW( const WCHAR *str )
{
    int size;
    WCHAR *ret;

    if (!str) return NULL;
    size = (lstrlenW( str ) + 1) * sizeof(WCHAR);
    ret = HeapAlloc( GetProcessHeap(), 0, size );
    if (ret) memcpy( ret, str, size );
    return ret;
}

#endif
